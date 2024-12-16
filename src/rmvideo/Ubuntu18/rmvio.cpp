//=====================================================================================================================
//
// rmvio.cpp : Partial implementation of ABSTRACT class CRMVIo.
//
// AUTHOR:  saruffner.
//
// DESCRIPTION:
// CRMVDisplay, RMVideo's display and animation manager, sets up the GLX/OpenGL framework for drawing supported
// targets on a fullscreen computer display.  To work with Maestro, however, there must be some sort of communication
// interface established.  CRMVIo is an abstraction of this communication link.  It defines the set of methods that
// CRMVDisplay will invoke during runtime.
//
// Modus operandi:  RMVideo has three runtime states, "off", "idle" and "animate".  It starts up in the "off" state,
// basically waiting for Maestro to initiate a "command session" with RMVideo.  It is CRMVIo that must implement this
// wait state.  CRMVDisplay shuts off its connection to the X Display, turns off soft real-time processing, and
// calls CRMVIo::openSession().  When this function returns, it is assumed that a session has been established
// with a Maestro client and that Maestro has sent the RMV_CMD_STARTINGUP command.  CRMVDisplay opens a fullscreen
// display, turns on soft real-time processing, enters the "idle" state and signals Maestro (through CRMVIo)
// accordingly.
//
// In the idle state, the screen is filled with the current background color, there are no per-frame updates, and
// CRMVDisplay will poll CRMVIo roughly every 2ms, checking for the next command from Maestro.  To start an animation
// sequence on the RMVideo display, Maestro will send a RMV_CMD_LOADTARGETS command, followed by RMV_CMD_STARTANIMATE.
// The former command includes the definitions of all participating targets, while the latter has target displacement
// data for the first and second frames of the animation sequence.  Upon receipt of the latter command, CRMVDisplay
// renders the first display frame on the backbuffer, swaps buffers at the beginning of the monitor's vertical retrace,
// and then immediately notifies Maestro (through CRMVIo) that the animation has begun -- Maestro waits for this
// notification, providing for a crude sort of synchronization of Maestro's timeline with RMVideo's timeline.
//
// During the animation, CRMVDisplay "works ahead" by one display frame.  After notifying Maestro that the animation
// started, it processes the second set of target displacement vectors and begins rendering the second display frame on
// the backbuffer.  It then waits for the vertical retrace marking the start of the second frame, and swaps buffers.
// At this point it is ready to work on the third frame, so it polls CRMVIo for the next Maestro command, which should
// be an RMV_CMD_UPDATEFRAME command.  To maximize the time RMVideo has available to render display frame N, it is
// important that Maestro send the corresponding UPDATEFRAME command BEFORE the beginning of frame N-1. When the
// animation is finished, Maestro sends a RMV_CMD_STOPANIMATE command.  In response, CRMVDisplay unloads the animated
// target list, clears the screen to the current background color, and returns to the "idle" runtime state.  Again, it
// uses CRMVIo to signal Maestro when it has returned to "idle".
//
// If it takes longer than one frame period to render a particular display frame, then a "skipped frame" results.
// RMVideo will continue to work, but it informs Maestro of the error, again through the CRMVIo interface.  Maestro
// will typically abort the animation in this case.
//
// Multithreading considerations:  Observe that CRMVDisplay gets the next Maestro command by *polling* CRMVIo -- every
// ~2ms in the "idle" state, and at the beginning of each display frame during an animation sequence.  Maestro commands
// will arrive asynchronously, of course.  So a practical CRMVIo implementation may require a separate thread or
// process -- most likely a kernel driver since the communication interface will involve some sort of hardware
// device! -- that buffers Maestro commands as they are sent.  This kernel component must be fast and use as little
// CPU time as possible, because RMVideo's main thread needs to hog the CPU as much as possible (using POSIX soft
// real-time support) to maximize rendering performance and to meet the demands of vertical retrace synchronization.
// Possible implementations of CRMVIo may use a private point-to-point TCP/IP connection, a USB host-to-host link,
// etc...
//
// Maestro commands to RMVideo:  Each command takes the form of a 32-bit command ID followed by zero or more 32-bit
// integers of command data. With a few exceptions, the command reply is a single 32-bit integer. The commands are 
// described in detail in RMVIDEO_COMMON.H.  Any CRMVIo implementation must recognize and process each of these 
// commands, translating command data into a form retrieved by CRMVDisplay via the appropriate CRMVIo accessor method.
//
// USAGE:  See method headers below...
//
// REVISION HISTORY:
// 23aug2005-- Created.
// 02feb2006-- Introducing a variety of changes as we begin work on a practical implementation over a private TCP/IP
//             Ethernet connection.
// 24apr2006-- Modified to support new command, RMV_CMD_GETCOLORRES.
// 22jul2009-- Major changes. CRMVIo implementation no longer replies directly to any commands. The animation-related 
//             commands provide motion vectors for all loaded targets, so the 'bNoChange' argument has been removed 
//             from getMotionVector(). Added support for switching video mode via new cmd RMV_CMD_SETCURRVIDEOMODE, 
//             and removed support for deleted command RMV_CMD_GETCOLORRES. To support sending more than one 'int' to 
//             Maestro (needed for some new commands), added sendData().
// 27jul2009-- Adding support for commands related to downloading movie files to and retrieving movie info from 
//             RMVideo: RMV_CMD_GETMOVIEDIRS..RMV_CMD_PUTFILE.
// 24aug2009-- Eliminated getBkgColor(), getGeometry(), getVideoMode(), getMonitorGamma(); replaced by more generic
//             getCommandArg(), which applies to all commands that have a short list of 32-bit int arguments.
// 04oct2016-- Renamed getMovie***() methods as getMedia***(). The former RMVideo "movie store" is now a "media store"
//             that can store image as well as video files.
// 25sep2018-- In support of new "vertical sync spot flash" feature, added abstract method isSyncFlashRequested(). The
// RMV_CMD_STARTANIMATE command includes a "sync flash requested" flag; if nonzero, Maestro is requesting that the 
// spot flash begin on the very first frame of the animation sequence. Similarly, _UPDATEFRAME command includes the
// same flag to request the flash start on any subsequent frame. RMVIo implementations already must process these 
// commands to get the motion update vectors for each animation frame; they must be updated to get the sync flash
// request flag and expose its value via isSyncFlashRequested().
//=====================================================================================================================


#include "rmvio.h"

//=== init ============================================================================================================
//
//    Initialize the communication interface, allocating whatever resources as necessary.  Do NOT attempt to initiate a
//    connection with Maestro here.  If the initializations fail, post an appropriate error message to stderr.
//
//    ARGS:       NONE.
//    RETURNS:    True if successful; false if unable to setup resources for the communication interface.  In the
//                latter case, RMVideo will exit.
//bool init();

//=== cleanup =========================================================================================================
//
//    Destroy the communication interface, releasing any resources allocated in init().  Be sure to close the comm
//    link with Maestro first (if it is still open).  Implementing class destructor is responsible for ensuring that
//    any allocated resources are released -- so it is probably a good idea to call this method from the destructor.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
//void cleanup();

//=== openSession =====================================================================================================
//
//    Do whatever is necessary to open a connection and start a command session with a Maestro client, then BLOCK
//    waiting for a connection to be established. How a "connection" is made, of course, will depend on the
//    implementation. The waiting state should not hog the CPU!
//
//    RMVideo is designed to run forever as long as no fatal error occurs. When it first starts up, it does not take
//    control of the monitor. It must first wait for a Maestro client to start a "command session" with it -- which
//    is the sole purpose of this method. It should return as soon as the connection is established AND the
//    RMV_CMD_STARTINGUP command has been received from Maestro. Upon successful return from this method, CRMVDisplay
//    will display its fullscreen window on the monitor and enter the "idle" state, informing Maestro by sending the
//    RMV_SIG_IDLE signal. Maestro expects this signal to verify RMVideo is running.
//
//    When a command session is "terminated", CRMVDisplay closes the fullscreen window, calls closeSession() to close
//    the connection to the Maestro client, and calls openSession() again to wait for the next command session to begin.
//
//    ARGS:       NONE.
//    RETURNS:    True if Maestro command session established; false if a serious error occurred.  In the latter case,
//                RMVideo will exit.
//
//bool openSession();

//=== closeSession ====================================================================================================
//
//    Issue the RMV_SIG_BYE signal, then do whatever is necessary to close the current connection to a Maestro client.
//    This method is the complement to openSession() and will be invoked by CRMVDisplay in response to the
//    RMV_CMD_SHUTTINGDN command from the Maestro client.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
//void closeSession();

//=== getNextCommand ==================================================================================================
//
//    Poll for the next command from the Maestro client.  This method must execute as quickly as possible, returning
//    RMV_CMD_NONE if a complete command has not yet been received over the communication interface.  Otherwise, return
//    the command ID. The implementation must completely process the command's "payload" and provide access to command
//    data via getCommandArg() or one of the specialize accessors for retrieving target information or media files.
//
//    If the implementation determines that the received command is invalid or its payload is incorrectly formatted, it 
//    MUST immediately send a RMV_SIG_CMDERR signal to the Maestro client and return RMV_CMD_NONE. The invalid command 
//    is effectively ignored.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID, RMV_CMD_NONE if no command is pending, or RMV_CMD_NONE-1 if there's a fatal
//                error in the communication interface (or if the comm link is not yet established).
//
//int getNextCommand();

//=== getCommandArg ===================================================================================================
//
//    Retrieve one of the 32-bit integer arguments accompanying the most recent command. CRMVDisplay will only invoke
//    this method immediately after retrieving one of these commands (all of which have a short list of int args):
//    RMV_CMD_RESTART, _SETBKGCOLOR, _SETGEOMETRY, _SETGAMMA, _SETSYNC, and _SETCURRVIDEOMODE.
//
//    ARGS:       pos -- [in] The ordinal position of the command argument requested.
//    RETURNS:    The argument requested; -1 if invalid request (not relevant to last command or invalid arg pos).
//
//int getCommandArg(int pos);

//=== getNumTargets ===================================================================================================
//
//    Retrieve the number of target definitions accompanying the last valid RMV_CMD_LOADTARGETS command. CRMVDisplay 
//    will always invoke this method before retrieving the target definitions with getTarget().
//
//    ARGS:       NONE.
//    RETURNS:    The number of target definitions sent with the last RMV_CMD_LOADTARGETS command.
//
//int getNumTargets();

//=== getTarget =======================================================================================================
//
//    Retrieve a target defintion provided in the last valid RMV_CMD_LOADTARGETS command. Implementing class is
//    responsible for parsing the RMV_CMD_LOADTARGETS command to prepare each target record IAW the RMVTGTDEF struct.
//    See RMVIDEO_COMMON.H for details.
//
//    CRMVDisplay will invoke this method once for each defined target, immediately after getting the LOADTARGETS cmd.
//    Still, CRMVIo implementors are encouraged to maintain the target definitions internally as individual RMVTGTDEF
//    records, changing them only when a new LOADTARGETS command is processed.
//
//    ARGS:       iPos -- [in] The zero-based index of the target record to be retrieved.  Target definitions MUST be
//                   supplied in the same order they appeared in the original RMV_CMD_LOADTARGETS command, since that
//                   determines the order of drawing during any subsequent animation sequence!
//                tgt -- [out] This will contain the retrieved target definition.
//    RETURNS:    False iff the target index is invalid.
//
//bool getTarget(int iPos, RMVTGTDEF& tgt );

//=== getMotionVector =================================================================================================
//
//    Retrieve the motion vector describing the trajectory of the specified target during an animation frame.  The
//    RMV_CMD_STARTANIMATE command provides motion vectors for the first 2 frames of an animation sequence that's about
//    to start, while the RMV_CMD_UPDATEFRAME command supplies motion vectors for the next frame of an animation
//    already in progress.  Implementing class is responsible for parsing the two commands correctly and putting the
//    motion vectors in the format defined by the RMVTGTVEC struct.  See RMVIDEO_COMMON.H for details.
//
//    After getting the STARTANIMATE command, CRMVDisplay will invoke this method once for each loaded target to
//    retrieve the motion vectors for frame 0, then once more for each target to retrieve the motion vectors for frame
//    1.  For the UPDATEFRAME command, CRMVDisplay will invoke this method once for each target to get the motion
//    vectors for the next display frame.
//
//    ARGS:       iPos -- [in] The zero-based index of the target for which a motion vector is requested. The index
//                   reflects the order in which the targets were defined in the last RMV_CMD_LOADTARGETS command. The
//                   STARTANIMATE and UPDATEFRAME commands identify targets in this manner.
//                vec -- [out] The retrieved motion vector for the specified target.
//    RETURNS:    False iff the target index is invalid.
//
//bool getMotionVector(int iPos, RMVTGTVEC& vec)

//=== isSyncFlashRequested
//
// Has Maestro requested that the sync spot flash start during the first animation frame (RMV_CMD_STARTANIMATE) or 
// any subsequence animation frame (RMV_CMD_UPDATEFRAME)?
//
// The sync spot flash feature was added in RMVideo v8 (Maestro 4.0.0) to provide a means to more precisely align
// the RMVideo and Maestro timelines. The spot is located in the top-left corner of the screen. In practice, a 
// photodiode assembly is used to detect the flash and generate a TTL pulse, which can then be timestamped by
// Maestro.
//
// RMVIo implementations must properly parse the _STARTANIMATE and _UPDATEFRAME commands and preserve the value of
// the request flag. For any other RMVideo command processed, this method should always return false.
//
// @return True if sync request flag was set in the command data for the RMV_CMD_STARTANIMATE or _UPDATEFRAME command;
// False if request flag was not set, or last command processed was neither of these commands. 
// bool isSyncFlashRequested()

//=== getMediaFolder ==================================================================================================
//
//    Retrieve the name of the media folder accompanying the last valid RMV_CMD_GETMEDIAFILES, _GETMEDIAINFO,
//    _DELETEMEDIA, or _PUTFILE command. CRMVDisplay will only invoke this method immediately after retrieving one of
//    these four commands.
//
//    ARGS:       NONE.
//    RETURNS:    The media folder name sent with the last command for which this parameter was relevant. Possibly an
//                empty string. Do not attempt to free or alter the value returned.
//
//const char* getMediaFolder();

//=== getMediaFile ====================================================================================================
//
//    Retrieve the name of the media file accompanying the last valid RMV_CMD_GETMEDIAINFO, _DELETEMEDIA, or _PUTFILE
//    command. CRMVDisplay will only invoke this method immediately after retrieving one of these three commands.
//
//    ARGS:       NONE.
//    RETURNS:    The media file name sent with the last command for which this parameter was relevant. Possibly an
//                empty string. Do not attempt to free or alter the value returned.
//
//const char* getMediaFile();

//=== downloadFile ====================================================================================================
//
//    Download the contents of a file over the communication interface and stream them into the open file specified.
//
//    CRMVDisplay or CRMVMovieMgr will call this method after successfully processing the RMV_CMD_PUTFILE command, 
//    which initiates a file download (from Maestro to RMVideo) sequence. The file contents are streamed in 2KB chunks 
//    via a sequence of RMV_CMD_PUTFILECHUNK packets, followed by a terminal RMV_CMD_PUTFILEDONE message. Receipt of 
//    any other commands is considered an error, terminating the download. Obviously, a file transfer may only occur 
//    when RMVideo is in the idle state.
//
//    Implementations should write each file chunk, in the order received, to the file descriptor provided and send 
//    RMV_SIG_CMDACK in response to get the next file chunk. If an IO error occurs, send RMV_SIG_CMDERR to let Maestro 
//    know that the transfer has failed. Upon receipt of the RMV_CMD_PUTFILEDONE message (and its cancel flag is NOT
//    set!), do NOT send a response to Maestro; the caller will examine the downloaded file and respond accordingly. If 
//    the cancel flag is set in the RMV_CMD_PUTFILEDONE message, send RMV_SIG_CMDACK to Maestro to acknowledge that the
//    download was indeed cancelled. Regardless the outcome, close the file before returning. The caller is responsible
//    for removing the file if the operation failed.
//
//    ARGS:       fd -- File descriptor of the new, open file to which the transferred content should be streamed. This
//                file descriptor must be closed upon returning from this method.
//    RETURNS:    True if file transfer completed successfully, in which case Maestro is still waiting for a response 
//                to the RMV_CMD_PUTFILEDONE command. False if an error occurred or if Maestro cancelled the transfer.
//
//bool downloadFile(FILE* fd);

//=== sendData ========================================================================================================
//
//    CRMVDisplay invokes this method to send command replies or signals back to Maestro. Most replies and all signals
//    consist of a single 32-bit integer, a few commands have longer replies. See RMVIDEO_COMMON.H for details. The
//    implementation of sendSignal() relies simply calls this method. CRMVIo implementations should send the payload as 
//    quickly as possible. Maestro will NOT acknowledge receipt. The method must NOT block. If the send fails, return 
//    silently (but post an error message to stderr).
//
//    ARGS:       len -- [in] Length of reply payload. If non-positive, no action taken.
//                pPayload -- [in] The reply payload buffer. 
//    RETURNS:    NONE.
//
//void sendData(int len, int* pPayload);

//=== sendSignal ======================================================================================================
//
//    Send a single 32-bit integer "signal" to Maestro. This is simply a shortcut for the paradigm:
//       int signal = signal_value;
//       pIO->sendData(1, &signal);
//
//    ARGS:       sig -- [in] The signal to be sent.
//    RETURNS:    NONE.
//
void CRMVIo::sendSignal(int sig)
{
   int i = sig;
   sendData(1, &i);
}
