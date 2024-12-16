//=====================================================================================================================
//
// cxrmvideo.cpp : CCxRMVideo, the MaestroDriver device class that handles communication with the application Remote
//                 Maestro Video (RMVideo), which implements framebuffer video targets for Maestro.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Remote Maestro Video (RMVideo) was developed as a replacement for Maestro's ancient VSG2/4 framebuffer video card.
// That card, while flexibly programmable, was based on very old graphics technology and is very slow compared to
// today's commercial video cards.  RMVideo runs on a separate Linux workstation and uses OpenGL to take advantage of
// the power of a modern video card.  Maestro communicates with RMVideo over a private point-to-point Ethernet
// connection (so we don't have to worry about network bottlenecks). RMVideo was designed to run continuously, acting
// as a sort of video "server" to a single Maestro "client".  When Maestro starts up and signals it, RMVideo "wakes up"
// and opens a fullscreen OpenGL window, blanks it, and begins processing Maestro commands.  When Maestro shuts down,
// RMVideo releases its OpenGL window and goes to sleep, polling the network connection waiting for the next "session"
// to begin.
//
// CCxRMVideo is a MaestroDriver device class that "presents" RMVideo as just another device to Maestro.  It implements
// the Ethenet communication link using the RTX TCP/IP protocol stack, along with a simple command protocol to send all
// target definitions and per-frame motion vectors over the comm link, and to get status and other information (eg,
// display resolution and frame rate) back from RMVideo.
//
// ==> Base class CDevice.
// All CXDRIVER hardware interfaces are derived from the abstract class CDevice.  CDevice defines several basic device
// operations, such as Open(), Close(), and Init().  It also has built-in support for PCI devices (most CXDRIVER boards
// are hosted on the PCI bus) and COFF loading support for DSP devices based on the TI C6x- or C4x-series processors.
// Note that derived classes inherit a number of pure virtual methods from this class.  Any practical device class must
// implement these methods, which handle a number of device-specific operations.  See CDevice for details.
//
// As required, CCxRMVideo subclasses CDevice.  However, it does not fit the CDevice framework very well.  The network
// card driver is built into RTX itself (only a handful of NICs are supported).  CCxRMVideo uses the WinSock2 API and
// standard BSD Sockets calls, implemented in the RTX TCP/IP package, to open a connection to RMVideo via the
// RTX-controlled NIC (completely separate from the Maestro PC's other NIC, which is generally connected to a larger
// intranet), identified by the IP4 address in the defined constant RMVNET_MAESTROADDR from RMVIDEO_COMMON.H, and to
// communicate over that connection.  Even though the NIC is probably a PCI card, CCxRMVideo does not provide PCI info
// to the CDevice base class because it does not talk to the device directly.  The MapDeviceResources() and
// UnmapDeviceResources() methods do nothing.  In OnOpen(), the socket connection to RMVideo is opened and a command
// session is initiated by sending the RMV_CMD_STARTINGUP command.  In OnClose() the session is ended by the
// RMV_CMD_SHUTTINGDN command, and then the socket connection is closed..
//
// ==> RMVideo targets
// RMVideo originally introduced to replace the old VSG2/4 framebuffer card, but it has since replaced the XYScope 
// platform (albeit at a slower and fixed frame rate), which is obsolete and no longer supported a/o Maestro 4.0. It is
// capable of animating any of the XYScope targets in addition to most of the old framebuffer video targets. It also 
// supports Gaussian windowing of drifting gratings, something which was not possible with the old framebuffer solution.
// 
// As of Maestro 4.0, it is really the only stimulus platform in use.
// 
// Here's the current list of supported targets:
//
//    RMV_POINT         A single square dot of a specified width in pixels (between 1 and 10).
//    RMV_RANDOMDOTS    Random-dot pattern independent of target window aperture, with options for finite dotlife,
//                      per-dot speed or direction noise, and % coherence (single color patch, or two-color "contrast"
//                      patch mode). Four apertures supported: rectangle, oval, rectangular annulus, oval annulus.  
//                      Gaussian window also supported.
//    RMV_FLOWFIELD     Optical flow field of dots (single color).
//    RMV_BAR           Oriented line/bar (single color).
//    RMV_SPOT          Target window aperture filled uniformly with a single RGB color.  Four apertures supported:
//                      rectangle, oval, rectangular annulus, oval annulus.  Gaussian window also supported.
//    RMV_GRATING       Sine or squarewave grating that can drift independently of tgt window, with or without a
//                      Gaussian window.  Rectangle and oval apertures supported, but this target type is most likely
//                      to be used with a Gaussian window that fades its edges into the background.  Gaussian may be
//                      circular, elliptical, or even 1D (H or V).
//    RMV_PLAID         Drifting plaid composed of two sine or squarewave gratings, with or without a Gaussian window.
//                      Rectangle and oval apertures supported.  Gaussian may be circular, elliptical, or even 1D.
//                      The gratings may move independently, or the plaid may act as a single pattern.  Each grating 
//                      has its own RGB color spec, spatial frequency, spatial phase, and drift axis.
//    RMV_MOVIE         Movie playback during an animation sequence. This target allows a user to create arbitrarily 
//                      complex visual stimuli, as long as he/she has the means to create videos that can be read and
//                      played back by RMVideo. During an animation sequence, movie frames are retrieved one at a time 
//                      from the video file "on the fly" -- so any length movie is supported so long as the individual 
//                      frames can be retrieved from file faster than the RMVideo display's vertical refresh rate. The
//                      location of the movie window on the screen can be updated on a frame-by-frame basis.
//    RMV_IMAGE         Displays a static image, the location of which can be updated on a frame-by-frame basis. The
//                      image data is loaded from a source file in the "media store" on the RMVideo host machine.
//
// ==> Coordinate system transformations.
// All RMVideo target types are, in part, defined by a bounding rectangle.  Maestro specifies this rectangle in a
// "visual" coordinate system, where the subject's line-of-sight (LOS) passes thru the center of the RMVideo display,
// which is defined as the origin.  The units of this coordinate system are floating-point visual deg subtended at the
// eye, with x-coords increasing to the right and y-coords increasing upward.  Target position is defined as the coords
// of the target rectangle's center.  RMVideo itself transforms target size and position to internal "device"
// coordinates (pixels); CCxRMVideo sends RMVideo all target dimensions and motion vectors in visual deg and deg/sec.
//
// For RMVideo to perform these coordinate transformations, it must be sent the display geometry for the monitor
// driven by the video card installed in the RMVideo PC.  This is done at start up, and any time the display geometry
// is changed -- see SetGeometry().  Based on the display geometry and the assumption that the subject's LOS passes
// through the screen center normal to the screen plane, RMVideo computes the scale factor that converts measurements
// in visual deg to pixels on the RMVideo display.  CCxRMVideo also calculates the scale factor, which is exposed by
// GetDegToPix().  NOTE that RMVideo assumes the scale factor is approximately the same for both horizontal and
// vertical dimensions -- in fact, the calculated scale factor is the average of the horizontal and vertical factors.
// Users must take care to set up the FB display with this assumption in mind!
//
// ==> Description of overridable methods, required and optional.  Usage.
// 1) CDevice overrides.  As mentioned above, low-level methods MapDeviceResources() and UnmapDeviceResources() create
// and release the socket resources required for the RMVideo-Maestro communication link, while the RMVideo commands
// that open and close an "RMVideo command session" are sent in OnOpen() and OnClose(), respectively.  The Init()
// method merely ensures that RMVideo is in the idle state.
//
// 2) Frame period, color resolution, display resolution and geometry. Use GetFramePeriod() to get RMVideo's frame 
// period in seconds, with microsecond resolution. This is RMVideo's estimate of the actual frame rate, obtained by 
// measuring the elapsed time for 500 frames (performed when RMVideo is started). Use GetScreenW_pix() and 
// GetScreenH_pix() to get the display size in pixels. GetScreenW_deg() and GetScreenH_deg() give the display size in 
// visual degrees, based on the current display geometry. Use Get/SetGeometry() to retrieve or change the current 
// display width, height, and distance to subject's eye, in mm. The geometry can be changed only when RMVideo is in 
// the idle state.
//
// 3) Video modes and gamma-correction factors. Use Get/SetMonitorGamma() to retrieve or change the gamma correction
// factors currently in effect for the RMVideo monitor. Use GetNumModes() and GetModelInfo() to find out what video 
// modes (display resolution + frame rate combinations) are available on the RMVideo display. Use Get/SetCurrentMode() 
// to get/set the current video mode. Note that changing the video mode takes a while, since RMVideo will take 500 frames
// to measure the frame rate in the new mode. Video mode and monitor gamma may only be changed when RMVideo is idle.
//
// 4) Background color. RMVideo supports a uniform background filled with an RGB color. To retrieve or change this
// background color, use Get/SetBkgColor(). The color may be changed only when RMVideo is in the idle state.
//
// 5) Animations. RMVideo has two states: "idle" and "animate".  In the "idle" state, the display is filled with the
// current background color, and RMVideo is simply polling for Maestro commands. To animate targets on the RMVideo
// display, the targets must first be loaded. Call AddTarget() to add each participating target; when done, call
// LoadTargets() to send all defined targets to RMVideo. These commands may only be invoked while RMVideo is "idle";
// the LoadTargets() command may take some time to execute, particularly if there are any Gaussian-windowed grating
// targets. Once the targets are loaded, call StartAnimation() to enter the time-critical "animate" state. Arguments
// to this method include target motion updates for the first two display frames of the animation. RMVideo will
// prepare the first display frame on the back buffer. Upon reaching the vertical sync between frames, it will send
// a signal that the animation has begun. StartAnimation() returns as soon as this signal is received, thereby
// providing a rough but adequate synchronization of the RMVideo and MaestroDriver timelines. It immediately begins
// rendering the next display frame in the backbuffer, while the first frame is being drawn onscreen. At this point,
// RMVideo expects Maestro to send target motion update vectors for display frame N BEFORE the vertical sync for
// display frame N-1. That way, it always has a full display frame period to do rendering on the back buffer.  Call
// UpdateAnimation() to send the target motion vectors for the next frame.  Call StopAnimation() to terminate the
// animation sequence. RMVideo will automatically clear any loaded targets and return to the "idle" state.
//
// 6) Media store management. In support of the RMV_MOVIE and RMV_IMAGE targets, RMVideo maintains a "media store" 
// consisting of up to 50 folders each of which can hold up to 50 video and/or image source files. Use the methods 
// GetMediaFolders(), GetMediaFiles(), and GetMediaInfo() to query the current contents of the media store. The video
// file for an RMV_MOVIE target -- or the image file for RMV_IMAGE -- is identified by its relative path in this store.
// To download a new media file into the store via the Maestro-RMVideo network connection, use DownloadMediaFile(). To 
// remove a media file or an entire folder from the store, use DeleteMediaFile().
//
// 7) Error messages. Base class CDevice provides methods SetDeviceError() and GetLastDeviceError() for posting
// device error descriptions. As a rule, if any public method fails for whatever reason, an appropriate error msg
// should be posted via SetDeviceError().
//
//
// CREDITS:
// 1) Real-Time eXtension (RTX) to WinNT by VenturCom, Inc (www.vci.com).  RTX gives the WinNT OS real-time-like
// characteristics and provides kernel access for direct communications with hardware devices.
//
//
// REVISION HISTORY:
// 20mar2006-- Began development, adapting code from the test application TESTRMV.
// 30mar2006-- To support Maestro's Cont mode, RMVIDEO was modified to send the RMV_SIG_SKIPFRAME or RMV_SIG_DUPFRAME
//             signals just ONCE during an animation sequence -- b/c in Cont mode, calls to UpdateFrame() will be
//             sporadic.  Still, we can get problems if UpdateFrame() is never called before another command is sent
//             to RMVideo.  In this case, the reply code to the command is one of the above signals.  To address this
//             issue, CCxRMVideo will not send a command other than RMV_CMD_UPDATEFRAME or RMV_CMD_STOPANIMATE during
//             an animation sequence.  Furthermore, StopAnimation() includes code to keep checking for the expected
//             RMV_SIG_IDLE reply even if gets some other reply first.
//          -- Modified checkForRMVSignal() to return TRUE if it would block after receiving a partial packet.  This
//             should be a rare occurrence b/c the reply packets are so small, but initial testing suggested it might
//             be happening.
// 05apr2006-- Modified to support elliptical and 1D Gaussian windows:  RMVTGTDEF.fSigma is now a 2-element array,
//             with X std dev in the first element and Y std dev in the second. The corresponding parameter ID
//             RMV_TGTDEF_SIGMA is now followed by two parameter values, the X and Y standard deviations.
//          -- Gaussian windows now supported for RMV_SPOT and _RANDOMDOTS in addition to RMV_GRATING and _PLAID!
// 07apr2006-- Modified to support separate color specs for the two gratings in an RMV_PLAID target: RMVTGTDEF.iRGBMean 
//             and iRGBCon are now 2-element arrays.
//          -- Although the changes are transparent to CCxRMVideo, grating and plaid targets treat RMVTGTVEC.hPat and 
//             .vPat in a unique way.  See RMVIDEO_COMMON.H for details.
// 24apr2006-- Revised to support new query command RMV_CMD_GETCOLORRES, which reports RMVideo's color resolution -- 
//             either 24-bit or 16-bit.  Like the other static display properties, this is queried immediately after 
//             opening the connection with RMVideo.
// 24aug2009-- Beginning major revisions to support: retrieving available video modes and changing the current mode;
//             getting and setting the RMVideo monitor's gamma-correction factors; a new "movie" target class, along
//             with a "movie store" on the RMVideo machine; ability to check RMVideo program version, download the
//             latest version and restart.
// mar2012  -- General revisions attempting to address the rare "out-of-synch" errors that would occur when 
//             sendRMVCommand() failed in the middle of sending a command because the send buffers filled up. Also
//             moved WSAStartup() and WSACleanup() to CCxDeviceMgr, as these methods should be invoked only once by a
//             RTSS process, per RTX TCP-IP documentation.
// 24nov2014-- Modified LoadTargets to send RMVTGTDEF.iRGBCon[] as part of the defining params for the RMV_RANDOMDOTS
//             target. This dot patch target now supports a two-color contrast mode, when contrast C != 0 for any of 
//             the 3 color components R/G/B. In this mode, half the dots are drawn in color L1=M(1+C) and half in 
//             L2=M(1-C), where M=RMVTGTDEF.iRGBMean[0] and C=[0..1].
// 23may2016-- Removed auto-update feature. Auto-update complicated by the fact that there are now two different 
//             releases of the version-6 RMVideo executable, one built for a 2.6-era kernel and one for a 3.19 kernel.
// 11oct2016-- Updated method signatures, etc. IAW the generalization of the RMVideo "movie store" as a "media store" 
//             that can contain both video and image source files. Modified to support new target type, RMV_IMAGE, 
//             which displays a static image loaded from an image file in the media store.
// 24sep2018-- Mods to support new RMVideo feature: a vertical sync "spot flash" optionally presented in top-left
// corner of screen during an ongoing animation sequence. The flash may be presented starting with any video frame,
// unless a previous spot flash is still in progress. The feature settings (spot size, margin and flash duration) are 
// sent in the idle state. The flash can be triggered by setting the relevant argument in the StartAnimation() and
// UpdateAnimation() methods.
// 25sep2018-- Removed the spot flash "margin" setting. Spot size is in mm instead of pixels. Flash duration minimum
// is 1 instead of 0. Feature disabled by setting spot size to 0.
//
// 14mar2019-- Removed the re-sync feature introduced in Mar 2012. It never really worked IMO. 
// 14mar2019-- Mods IAW changes in the RMVideo-Maestro communication protocol when an animation sequence is running.
// RMVideo now uses the same signal RMV_SIG_ANIMATEMSG in 4 use cases: (1) To signal the start of the animation in
// response to the RMV_CMD_STARTANIMATE command. (2) To inform Maestro how many video frames have elapsed thus far.
// (3) To indicate a duplicate frame occurred because RMVideo did not receive a target update from CCxRMVideo (via the
// UPDATEFRAME command) in time. (4) To indicate that one or more duplicate frames occurred because of a rendering
// delay in RMVideo. UpdateAnimation() returns the elapsed frame count whenever it receives the appropriate message
// from RMVideo. It no longer aborts on a duplicate frame signal. Instead, CCxRMVideo tracks duplicate frame events and
// exposes the duplicate frame count via GetNumDuplicateFrames(). More detailed information on duplicate frame events
// is available via other public methods.
// 07may2019-- Mod LoadTargets() to send target flicker parameters if flicker is enabled for the target. The new
// feature is applicable to all target types.
// 14aug2019-- Increased the max wait time for the replies to several RMV_CMD_* commands, in particular LOADTARGETS
// and the commands related to downloading media files to th RMVideo side. This was because, in recent testing, we
// found that LoadTargets() would consistently fail on a reply timeout for 3 RMV_IMAGE targets -- one 640x360, one
// 1280x720, and one 2560x1440. At the same time, implemented an image cache on the RMVideo side that is preloaded when
// RMVideo starts up -- reducing the likelihood that RMVideo will have to read in the image from its source file 
// during the relpy to LOADTARGETS.
// 11dec2024-- Mod LoadTargets() to send new parameter RMVTGTDEF.fDotDisp, which specifies stereo dot disparity in
// visual deg. Applicable to the target types that draw dots.
//=====================================================================================================================

#include <winsock2.h>                  // we need this for all TCP/IP socket calls, including WSA extensions
#include <ws2tcpip.h>                  // for inet_pton(), which replaces deprecated inet_addr()
#include <windows.h>                   // standard Win32 includes
#include <stdio.h>                     // runtime C/C++ I/O library
#include <string.h>                    // runtime C/C++ string library
#include <math.h>                      // runtime C/C++ math library
#include "rtapi.h"                     // the RTX API

#include "util.h"                      // for utility classes like cMath
#include "cxrmvideo.h"

LPCTSTR CCxRMVideo::EMSG_SENDERROR = "Send socket error!";
LPCTSTR CCxRMVideo::EMSG_RECVERROR = "Receive socket error!";
LPCTSTR CCxRMVideo::EMSG_LOSTCONN = "RMVideo closed connection unexpectedly!";
LPCTSTR CCxRMVideo::EMSG_TIMEOUT = "Timed out waiting for reply to command!";
LPCTSTR CCxRMVideo::EMSG_CMDERROR = "RMVideo could not process command!";
LPCTSTR CCxRMVideo::EMSG_INVALIDREPLY = "Illegal reply packet format!";
LPCTSTR CCxRMVideo::EMSG_BADREPLY = "Got unexpected reply from RMVideo!";
LPCTSTR CCxRMVideo::EMSG_SENDDELAY = "Send failed due to network delay (buffer full?)";
LPCTSTR CCxRMVideo::EMSG_NOTWHILEANIM = "Function not available during animation sequence!";
LPCTSTR CCxRMVideo::EMSG_TGTLISTFULL = "Animated target list is full!";
LPCTSTR CCxRMVideo::EMSG_TGTLISTEMPTY = "Animated target list is empty!";
LPCTSTR CCxRMVideo::EMSG_UNRECOGTGT = "Unrecognized RMVideo target type";
LPCTSTR CCxRMVideo::EMSG_ANIMSTARTFAIL = "Timed out while waiting for animation sequence to start!";

const CDevice::DevInfo CCxRMVideo::BLANKDEV = { 0 };
const int CCxRMVideo::MIN_DISTTOEYE = 100;
const int CCxRMVideo::MIN_DIMENSION = 50;
const int CCxRMVideo::DEF_DISTTOEYE_MM = 800;
const int CCxRMVideo::DEF_WIDTH_MM = 339;
const int CCxRMVideo::DEF_HEIGHT_MM = 252;
const int CCxRMVideo::STATE_IDLE = 0;
const int CCxRMVideo::STATE_TGTSLOADED = 10;
const int CCxRMVideo::STATE_ANIMATING = 20;



/**
 * CCxRMVideo constructor. Unlike other device classes, there's no relevant device info for RMVideo (RTX handles the 
 * actual NIC device behind the BSD sockets implementation). So we pass a blank device info structure to the base class.
 */
CCxRMVideo::CCxRMVideo() : CDevice(BLANKDEV, 1)
{
   m_dFramePeriod = 0.0;
   m_nModes = 0;
   m_iCurrMode = -1;
   m_gamma[0] = m_gamma[1] = m_gamma[2] = 1000;

   m_iDistToEye = DEF_DISTTOEYE_MM;
   m_iWidth = DEF_WIDTH_MM;
   m_iHeight = DEF_HEIGHT_MM;
   m_bkgRGB = 0;

   m_syncFlashSize = 0;
   m_syncFlashDur = 1;

   m_dDegToPix = 1.0;

   m_iState = STATE_IDLE;
   m_nTargets = 0;

   m_bDisabled = FALSE;

   m_nDupEvents = 0;
   m_nDupFrames = 0;

   m_rmvSocket = INVALID_SOCKET;
   m_iReplyBytesRcvd = 0;
   m_iCmdBytesSent = 0;
}

/** Destructor. Here we just make sure that the TCP/IP socket closed. */
CCxRMVideo::~CCxRMVideo()
{
   if( m_rmvSocket != INVALID_SOCKET )
   {
      OnClose();
      m_rmvSocket = INVALID_SOCKET;
   }
}

/**
 * Get the RMVideo application version number. 
 * NOTE that during startup, we verify that the version number of the RMVideo server matches the version number on the
 * Maestro side. See OpenEx().
 *
 * @return The current version number (strictly positive); or -1 if RMVideo is currently unavailable.
 */
int RTFCNDCL CCxRMVideo::GetVersion() { return(IsOn() ? RMV_CURRENTVERSION : -1); }

/** 
 * Get RMVideo monitor frame period in seconds, with nanosecond precision. This is the frame period as measured over a 
 * 500-frame period when RMVideo starts or whenever there's a mode switch. 
 * @returns Frame period in seconds. Returns 0 if RMVideo is unavailable.
 */
double RTFCNDCL CCxRMVideo::GetFramePeriod() const
{
   return( m_dFramePeriod );
}

/**
 * Get current RMVideo monitor display width.
 * @return Display width in pixels; 0 if RMVideo is unavailable.
 */
int RTFCNDCL CCxRMVideo::GetScreenW_pix() const { return((m_iCurrMode >= 0) ? m_videoModes[m_iCurrMode].w : 0); }

/**
 * Get current RMVideo monitor display height.
 * @return Display height in pixels; 0 if RMVideo is unavailable.
 */
int RTFCNDCL CCxRMVideo::GetScreenH_pix() const { return((m_iCurrMode >= 0) ? m_videoModes[m_iCurrMode].h : 0);}

/**
 * Get the number of video display modes supported by RMVideo. RMVideo does not advertise all possible video modes, only
 * those that meet or exceed the minimum requirement of 1024x768 @ 75Hz.
 * @return Number of available video modes, or 0 if RMVideo is unavailable.
 */
int RTFCNDCL CCxRMVideo::GetNumModes() const { return(m_nModes); }

/**
 * Get information about one of the video display modes supported by RMVideo. The mode information (which does not 
 * change), is retrieved when first opening a command session with the remote RMVideo server.
 * @param n The index of the mode for which information is requested. Must be between 0 and N-1, where N is the number 
 * of available modes.
 * @param w, h, rate [out] The display width and height in pixels and the frame rate in Hz for the mode specified. All 
 * are set to 0 if RMVideo is unavailable or an invalid mode is specified.
 * @return TRUE if successful; FALSE if RMVideo is unavailable or the mode index was invalid.
 */
BOOL RTFCNDCL CCxRMVideo::GetModeInfo(int n, int& w, int& h, int& rate) const
{
   w = 0; h = 0; rate = 0;
   if(n < 0 || n >= m_nModes) return(FALSE);
   w = m_videoModes[n].w;
   h = m_videoModes[n].h;
   rate = m_videoModes[n].rate;
   return(TRUE);
}

/**
 * Get the current RMVideo display mode.
 * @return Index of the current display mode, between 0 and N-1, where N is the number of available modes. Returns -1 if
 * RMVideo is unavailable.
 */
int RTFCNDCL CCxRMVideo::GetCurrentMode() const { return(m_iCurrMode); }

/**
 * Set the current RMVideo display mode. This method BLOCKS a maximum of 10 seconds while waiting for RMVideo to switch 
 * to the display mode specified. Switching modes takes a while because RMVideo must re-measure the frame period after 
 * the switch. The measurement occurs over a period of 500 frames, or 6.67 seconds at the minimum frame rate of 75Hz. 
 *
 * @param n Index of the desired display mode. Must be between 0 and N-1, where N is the number of available modes.
 * @return TRUE if successful; FALSE if mode index is invalid, or the mode switch command failed for some reason. 
 * GetLastDeviceError() reports the reason for the failure. Returns TRUE immediately if the mode specified is already
 * the current mode.
 */
BOOL RTFCNDCL CCxRMVideo::SetCurrentMode(int n)
{
   if(IsDown()) return(FALSE);
   if(m_iState == STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(n < 0 || n >= m_nModes)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   else if(n == m_iCurrMode)
   {
      ClearDeviceError();
      return(TRUE);
   }
   
   // prepare and send the RMV_CMD_SETCURRVIDEOMODE command, then wait for RMVideo's reply. BLOCKS HERE!
   m_commandBuf[0] = 2;
   m_commandBuf[1] = RMV_CMD_SETCURRVIDEOMODE;
   m_commandBuf[2] = n+1;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(10000)) return(FALSE);

   // update internal state if command was successful
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 2 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_iCurrMode = n;
      m_dFramePeriod = ((double)m_replyBuf[2]) / 1.0e9;
      RecalcDegToPix();
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}

/**
 * Get the current RMVideo display geometry.
 * @param d, w, h [out] Distance from subject's eye to RMVideo screen center, screen width, and screen height, in mm.
 * @return TRUE if successful, FALSE otherwise (RMVideo unavailable).
 */
BOOL RTFCNDCL CCxRMVideo::GetGeometry(int& d, int& w, int& h)
{
   if(IsDown()) return(FALSE);

   d = m_iDistToEye;
   w = m_iWidth;
   h = m_iHeight;

   ClearDeviceError();
   return(TRUE);
}

/**
 * Set the current RMVideo display geometry.
 * @param d, w, h [in] Distance from subject's eye to RMVideo screen center, screen width, and screen height, in mm.
 * @return TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; arguments invalid).
 */
BOOL RTFCNDCL CCxRMVideo::SetGeometry(int d, int w, int h)
{
   if(IsDown()) return(FALSE);
   if(m_iState==STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(d < MIN_DISTTOEYE || w < MIN_DIMENSION || h < MIN_DIMENSION)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }

   // prepare and send the RMV_CMD_SETGEOMETRY command, then wait for RMVideo's reply
   m_commandBuf[0] = 4;
   m_commandBuf[1] = RMV_CMD_SETGEOMETRY;
   m_commandBuf[2] = w;
   m_commandBuf[3] = h;
   m_commandBuf[4] = d;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(250)) return(FALSE);

   // update internal state if command was successful
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_iWidth = w;
      m_iHeight = h;
      m_iDistToEye = d;
      RecalcDegToPix();
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}

/**
 * Get the multiplicative scale factor which converts visual degrees to pixels on the RMVideo display.
 * @return Scale factor in pixels per degree subtended at eye. If RMVideo is unavailable, returns 1.0.
 */
double RTFCNDCL CCxRMVideo::GetDegToPix() const { return(m_dDegToPix); }

/**
 * Get RMVideo display width in visual degrees IAW standard assumptions about display geometry.
 * @return Screen width in degrees subtended at eye. If RMVideo is unavailable, returns 0.
 */
double RTFCNDCL CCxRMVideo::GetScreenW_deg() const { return(double(GetScreenW_pix()) / m_dDegToPix); }

/**
 * Get RMVideo display height in visual degrees IAW standard assumptions about display geometry.
 * @return Screen height in degrees subtended at eye. If RMVideo is unavailable, returns 0.
 */
double RTFCNDCL CCxRMVideo::GetScreenH_deg() const { return(double(GetScreenH_pix()) / m_dDegToPix); }

/**
 * Get the red, green, and blue gamma-correction factors for the RMVideo monitor.
 * @param r, g, b [out] The red, green, and blue gamma-correction factors, scaled by 1000 and rounded to the nearest
 * integer. All are set to 1000 (1.0) if RMVideo is unavailable.
 */
VOID RTFCNDCL CCxRMVideo::GetMonitorGamma(int &r, int&g, int &b) const
{
   r = m_gamma[0]; g = m_gamma[1]; b = m_gamma[2];
}

/**
 * Set the red, green, and blue gamma-correction factors for the RMVideo monitor.
 * @param r, g, b [in] The desired red, green, and blue gamma-correction factors, scaled by 1000 and rounded to the 
 * nearest integer. All must lie in the range [RMV_MINGAMMA, RMV_MAXGAMMA].
 * @return TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; arguments invalid).
 */
BOOL RTFCNDCL CCxRMVideo::SetMonitorGamma(int r, int g, int b)
{
   if(IsDown()) return(FALSE);
   if(m_iState == STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(r < RMV_MINGAMMA || r > RMV_MAXGAMMA || g < RMV_MINGAMMA || g > RMV_MAXGAMMA || b < RMV_MINGAMMA || b > RMV_MAXGAMMA)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }

   // prepare and send the RMV_CMD_SETGAMMA command, then wait for RMVideo's reply
   m_commandBuf[0] = 4;
   m_commandBuf[1] = RMV_CMD_SETGAMMA;
   m_commandBuf[2] = r;
   m_commandBuf[3] = g;
   m_commandBuf[4] = b;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(250)) return(FALSE);

   // update internal state if command was successful
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_gamma[0] = r;
      m_gamma[1] = g;
      m_gamma[2] = b;
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}
   
/**
 Change the settings governing the vertical sync spot flash that may be optionally presented in the top-left corner
 of the RMVideo screen during an animation sequence.
 @param sz Size of square spot in mm. Restricted to [RMV_MINSYNCSZ..RMV_MAXSYNCSZ]. If 0, spot flash is disabled.
 @param dur Duration of spot flash in # of video frames. Restricted to [RMV_MINSYNCDUR..RMV_MAXSYNCDUR].
 @return TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; arguments invalid).
*/
BOOL RTFCNDCL CCxRMVideo::SetSyncFlashParams(int sz, int dur)
{
   if(IsDown()) return(FALSE);
   if(m_iState == STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(sz < RMV_MINSYNCSZ || sz > RMV_MAXSYNCSZ || dur < RMV_MINSYNCDUR || dur > RMV_MAXSYNCDUR)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }

   // prepare and send the RMV_CMD_SETSYNC command, then wait for RMVideo's reply
   m_commandBuf[0] = 3;
   m_commandBuf[1] = RMV_CMD_SETSYNC;
   m_commandBuf[2] = sz;
   m_commandBuf[3] = dur;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(250)) return(FALSE);

   // update internal state if command was successful.
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_syncFlashSize = sz;
      m_syncFlashDur = dur;
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}


//=== Get/SetBkgColor =================================================================================================
//
//    Get/set the current RMVideo display background color.  If RMVideo is unavailable, both methods fail.
//    SetBkgColor() should only be called when RMVideo is in the idle state; otherwise it will fail.
//
//    ARGS:       r,g,b -- [in or out] The red, green, and blue components of the background color.  On input, each is
//                         restricted to the range [0..255].
//    RETURNS:    TRUE if successful, FALSE otherwise (RMVideo unavailable, RMVideo currently animating).
//
BOOL RTFCNDCL CCxRMVideo::GetBkgColor(int& r, int& g, int& b)
{
   if(IsDown()) return(FALSE);

   r = m_bkgRGB & 0x00FF;
   g = (m_bkgRGB >> 8) & 0x00FF;
   b = (m_bkgRGB >> 16) & 0x00FF;

   ClearDeviceError();
   return(TRUE);
}

BOOL RTFCNDCL CCxRMVideo::SetBkgColor(int r, int g, int b)
{
   if(IsDown()) return(FALSE);
   if(m_iState == STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }

   // prepare and send the RMV_CMD_SETBKGCOLOR command, waiting for RMVideo's acknowledgment
   int rgbNew = cMath::rangeLimit(r,0,255) + (cMath::rangeLimit(g,0,255) << 8) + (cMath::rangeLimit(b,0,255) << 16);
   m_commandBuf[0] = 2;
   m_commandBuf[1] = RMV_CMD_SETBKGCOLOR;
   m_commandBuf[2] = rgbNew;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(250)) return(FALSE);

   // update internal state if command was successful
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_bkgRGB = rgbNew;
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}

//=== Init ============================================================================================================
//
//    Ensure RMVideo is in idle state with no targets loaded.  The background color and display geometry are left
//    unchanged.  This method merely invokes StopAnimation().
//
//    NEVER use in time-critical code.  StopAnimation() will wait as long as 1 second for RMVideo to return to the
//    idle state.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  If this method does not succeed, either RMVideo is not
//                available or a serious error has occurred.
//
BOOL RTFCNDCL CCxRMVideo::Init()
{
   return( StopAnimation() );
}

//=== AddTarget =======================================================================================================
//
//    Add a target to the animated target list, which is uploaded to RMVideo by a subsequent call to LoadTargets().
//    The list can hold as many targets as are supported in Maestro itself (whether RMVideo can actually animate that
//    many targets is another question!).
//
//    If this method is called after targets are uploaded to RMVideo but before an animation is started, it effectively
//    unloads the target list and starts anew. LoadTargets() must be called again to upload the new target list before
//    an animation can begin.
//
//    ARGS:       tgt -- [in] defn of target to be added to animated target list.
//    RETURNS:    TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; target list full).
//
BOOL RTFCNDCL CCxRMVideo::AddTarget(RMVTGTDEF tgt)
{
   if(IsDown()) return(FALSE);
   if(m_iState==STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }

   // if this method is called after targets have been uploaded but before animation begins, the target list is
   // effectively unloaded:  return to "idle" state and clear the target list.
   if(m_iState == STATE_TGTSLOADED)
   {
      m_iState = STATE_IDLE;
      m_nTargets = 0;
   }

   if(m_nTargets == RMV_MAXTARGETS)
   {
      SetDeviceError(CCxRMVideo::EMSG_TGTLISTFULL);
      return(FALSE);
   }

   memcpy(&(m_TargDefs[m_nTargets]), &tgt, sizeof(RMVTGTDEF));
   ++m_nTargets;

   ClearDeviceError();
   return(TRUE);
}

/**
 * Upload the animated target list to RMVideo, waiting for acknowledgement from RMVideo that it is prepared to start
 * animating the loaded targets. The method will wait a maximum of 10 seconds for the acknowledgment. Obviously, then, 
 * this method should only be called when MaestroDriver is not in a time-critical state. It will fail if invoked while 
 * an RMVideo animation sequence is in progress.
 *
 * Targets are uploaded in the same order they were added to the target list via AddTarget(). This order is VERY
 * important, because the calls to StartAnimation() and UpdateAnimation() assume that target motion vectors are supplied 
 * in the same order!
 * 
 * @return TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; target list empty; did not 
 * respond to command within 10 sec). If the current target list has already been loaded, the method does nothing and 
 * returns TRUE.
 */
BOOL RTFCNDCL CCxRMVideo::LoadTargets()
{
   if(IsDown()) return(FALSE);
   if(m_iState==STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }

   // target list already uploaded -- there's nothing to do!
   if(m_iState==STATE_TGTSLOADED)
   {
      ClearDeviceError();
      return(TRUE);
   }

   // target list is empty -- there's nothing to load!
   if(m_nTargets == 0)
   {
      SetDeviceError(CCxRMVideo::EMSG_TGTLISTEMPTY);
      return(FALSE);
   }

   // prepare RMV_CMD_LOADTARGETS command in our command buffer IAW the defined target list. We encode each target
   // parameter as a set of integers (paramID, paramValue1, ...) in the command buffer; floating-point parameter values
   // are scaled and rounded. The two character-string parameters are treated specially, packed into 8 ints = 32 bytes
   // and padded with nulls as necessary. Only relevant target parameters are included, and each target "record" ends 
   // with the special paramID RMV_TGTDEF_END. For details, see RMVIDEO_COMMON.H.
   //
   m_commandBuf[1] = RMV_CMD_LOADTARGETS;
   m_commandBuf[2] = m_nTargets;
   int iCmdIdx = 3;

   for(int i=0; i<m_nTargets; i++)
   {
      RMVTGTDEF* pTgt = &(m_TargDefs[i]);
      m_commandBuf[iCmdIdx++] = RMV_TGTDEF_TYPE;
      m_commandBuf[iCmdIdx++] = pTgt->iType;

      if(pTgt->iFlickerOn > 0 && pTgt->iFlickerOff > 0)
      {
         m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FLICKER;
         m_commandBuf[iCmdIdx++] = pTgt->iFlickerOn;
         m_commandBuf[iCmdIdx++] = pTgt->iFlickerOff;
         m_commandBuf[iCmdIdx++] = pTgt->iFlickerDelay;
      }

      switch( pTgt->iType )
      {
         case RMV_POINT :
            
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NDOTSIZE;
            m_commandBuf[iCmdIdx++] = pTgt->nDotSize;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DOTDISP;
            m_commandBuf[iCmdIdx++] = int(RMV_TGTDEF_F2I_F * pTgt->fDotDisp + 0.5f);
            break;

         case RMV_RANDOMDOTS :
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_APERTURE;
            m_commandBuf[iCmdIdx++] = pTgt->iAperture;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FLAGS;
            m_commandBuf[iCmdIdx++] = pTgt->iFlags;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBCON;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBCon[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBCon[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_INNERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fInnerW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_INNERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fInnerH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NDOTS;
            m_commandBuf[iCmdIdx++] = pTgt->nDots;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NDOTSIZE;
            m_commandBuf[iCmdIdx++] = pTgt->nDotSize;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SEED;
            m_commandBuf[iCmdIdx++] = pTgt->iSeed;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_PCTCOHER;
            m_commandBuf[iCmdIdx++] = pTgt->iPctCoherent;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NOISEUPD;
            m_commandBuf[iCmdIdx++] = pTgt->iNoiseUpdIntv;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NOISELIM;
            m_commandBuf[iCmdIdx++] = pTgt->iNoiseLimit;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DOTLIFE;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fDotLife + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SIGMA;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[1] + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DOTDISP;
            m_commandBuf[iCmdIdx++] = int(RMV_TGTDEF_F2I_F * pTgt->fDotDisp + 0.5f);
            break;

         case RMV_FLOWFIELD :
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_INNERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fInnerW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NDOTS;
            m_commandBuf[iCmdIdx++] = pTgt->nDots;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_NDOTSIZE;
            m_commandBuf[iCmdIdx++] = pTgt->nDotSize;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SEED;
            m_commandBuf[iCmdIdx++] = pTgt->iSeed;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DOTDISP;
            m_commandBuf[iCmdIdx++] = int(RMV_TGTDEF_F2I_F * pTgt->fDotDisp + 0.5f);
            break;

         case RMV_BAR :
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DRIFTAXIS;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fDriftAxis[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = 0;
            break;

         case RMV_SPOT :
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_APERTURE;
            m_commandBuf[iCmdIdx++] = pTgt->iAperture;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_INNERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fInnerW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_INNERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fInnerH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SIGMA;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[1] + 0.5f );
            break;

         case RMV_GRATING :
         case RMV_PLAID :
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_APERTURE;
            m_commandBuf[iCmdIdx++] = pTgt->iAperture;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FLAGS;
            m_commandBuf[iCmdIdx++] = pTgt->iFlags;
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBMEAN;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBMean[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_RGBCON;
            m_commandBuf[iCmdIdx++] = pTgt->iRGBCon[0];
            m_commandBuf[iCmdIdx++] = pTgt->iRGBCon[1];
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERW;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterW + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_OUTERH;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fOuterH + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SPATIALF;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSpatialFreq[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSpatialFreq[1] + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_DRIFTAXIS;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fDriftAxis[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fDriftAxis[1] + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_GRATPHASE;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fGratPhase[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fGratPhase[1] + 0.5f );
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_SIGMA;
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[0] + 0.5f );
            m_commandBuf[iCmdIdx++] = int( RMV_TGTDEF_F2I_F * pTgt->fSigma[1] + 0.5f );
            break;

         case RMV_MOVIE:
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FLAGS;
            m_commandBuf[iCmdIdx++] = pTgt->iFlags;
            
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FOLDER;
            ::memset(&(m_commandBuf[iCmdIdx]), 0, 32);
            ::strcpy_s((char*) &(m_commandBuf[iCmdIdx]), 32, pTgt->strFolder);
            iCmdIdx += 8;

            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FILE;
            ::memset(&(m_commandBuf[iCmdIdx]), 0, 32);
            ::strcpy_s((char*) &(m_commandBuf[iCmdIdx]), 32, pTgt->strFile);
            iCmdIdx += 8;
            break;
            
         case RMV_IMAGE:
            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FOLDER;
            ::memset(&(m_commandBuf[iCmdIdx]), 0, 32);
            ::strcpy_s((char*) &(m_commandBuf[iCmdIdx]), 32, pTgt->strFolder);
            iCmdIdx += 8;

            m_commandBuf[iCmdIdx++] = RMV_TGTDEF_FILE;
            ::memset(&(m_commandBuf[iCmdIdx]), 0, 32);
            ::strcpy_s((char*) &(m_commandBuf[iCmdIdx]), 32, pTgt->strFile);
            iCmdIdx += 8;
            break;
            
         default :      // we should never get here, but just in case!
            SetDeviceError(CCxRMVideo::EMSG_UNRECOGTGT);
            return( FALSE );
            break;
      }

      m_commandBuf[iCmdIdx++] = RMV_TGTDEF_END;
   }

   // now send the prepared command to RMVideo and wait as long as 10 seconds for a reply
   m_commandBuf[0] = iCmdIdx-1;
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(10000)) return(FALSE);

   // if command acknowledged, update internal state; else fail
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      m_iState = STATE_TGTSLOADED;
      ClearDeviceError();
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   return(bOk);
}

//=== GetNumTargets ===================================================================================================
//
//    Get the number of targets currently defined in the animated target list. The method may be called at any time.
//
int RTFCNDCL CCxRMVideo::GetNumTargets() const { return(m_nTargets); }

/** StartAnimation 
Initiate a target animation sequence on the RMVideo display.

This method may be called only when RMVideo is in the idle state, AFTER the targets have been uploaded via a call to
LoadTargets(). This method prepares and sends the RMV_CMD_STARTANIMATE command, which includes target motion vectors
(RMVTGTVEC) for the first two display frames of the animation sequence. Upon receipt of the command, RMVideo prepares 
display frame 0 in the back buffer (RMVideo operates in double-buffered mode), then waits for the video card's vertical
sync before swapping front and back buffers. It sends a RMV_SIG_ANIMATEMSG signal as soon as the vertical sync period 
is entered. It then begins rendering display frame 1 on the back buffer while the video card draws frame 0 to the 
RMVideo display.

After sending the RMV_CMD_STARTANIMATE command, this method polls for the ANIMATEMSG signal and returns as soon as it 
is received, thereby providing a rough synchronization of Maestro and RMVideo timelines. If that signal does not arrive
within 500ms (which is MUCH longer than one frame period), then it is assumed that RMVideo cannot handle the animation 
sequence. In this situation, the method will invoke StopAnimation() to abort the animation sequence.

The method expects that N target motion vectors are supplied for display frames 0 and 1, where N is the number of
targets loaded. Also, the n-th motion vector will be applied to the trajectory of the n-th target loaded.

If the method fails for any reason, the animated target list is automatically cleared and RMVideo returns to the "idle"
state.

As of Maestro 4.0.0/RMVideo v8, support was added for triggering vertical sync "spot flash" in the top-left corner of
the screen during any frame of an animation sequence (as long as a flash is not currently in progress). 

@param pVecsFrame0 [in] Target motion vectors for all loaded targets for the first display frame of the animation.
@param pVecsFrame1 [in] Target motion vectors for all loaded targets for the second display frame.
@param bSync [in] If TRUE, the vertical sync spot flash is started during the first frame (cannot trigger the flash on 
the second frmae). Has no effect if the spot flash is disabled (zero spot size).

@return TRUE if successful, FALSE otherwise (RMVideo unavailable or currently animating; target list not loaded; 
RMVideo reported an errror or failed to send ANIMATEMSG signal within 500ms).
*/
BOOL RTFCNDCL CCxRMVideo::StartAnimation(RMVTGTVEC* pVecsFrame0, RMVTGTVEC* pVecsFrame1, BOOL bSync)
{
   int i;
   float fTemp;

   if(IsDown()) return(FALSE);
   if(m_iState == STATE_ANIMATING)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }

   // target list is not loaded!
   if(m_iState != STATE_TGTSLOADED)
   {
      SetDeviceError("RMVideo targets must be loaded prior to starting animation sequence!");
      return( FALSE );
   }

   // reset record of any duplicate frame events
   m_nDupFrames = 0;
   m_nDupEvents = 0;

   // is the vertical sync spot flash to be triggered on frame 0? Spot size must be non-zero.
   BOOL bEnaFlash = BOOL(bSync && (m_syncFlashSize > 0));

   // prepare the RMV_CMD_STARTANIMATE command: STARTANIMATE, SYNC?, N, V0(0), ..., V0(N-1), N, V1(0), ..., V1(N-1), 
   // where each "V" is the target index followed by the five parameters in the RMVTGTVEC structure...
   m_commandBuf[1] = RMV_CMD_STARTANIMATE;
   m_commandBuf[2] = bEnaFlash ? 1 : 0;
   m_commandBuf[3] = m_nTargets;
   int iCmdIdx = 4;
   for( i=0; i<m_nTargets; i++ )
   {
      m_commandBuf[iCmdIdx++] = i;
      m_commandBuf[iCmdIdx++] = (pVecsFrame0[i].bOn ) ? 1 : 0;
      fTemp = pVecsFrame0[i].hWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame0[i].vWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame0[i].hPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame0[i].vPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
   }

   m_commandBuf[iCmdIdx++] = m_nTargets;
   for( i=0; i<m_nTargets; i++ )
   {
      m_commandBuf[iCmdIdx++] = i;
      m_commandBuf[iCmdIdx++] = (pVecsFrame1[i].bOn ) ? 1 : 0;
      fTemp = pVecsFrame1[i].hWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame1[i].vWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame1[i].hPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecsFrame1[i].vPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
   }

   // now send the prepared command to RMVideo without waiting for a reply.  If we cannot send the command (comm link
   // problems), abort -- returning to idle state and clearing the target list.
   m_commandBuf[0] = iCmdIdx-1;
   if(!sendRMVCommand())
   {
      // serious network error: return to idle state and clear target list (device err msg already set)
      m_nTargets = 0;
      m_iState = STATE_IDLE;
      return(FALSE);
   }

   // wait up to 500ms for the "first frame" signal from RMVideo polling continuously
   BOOL bGotReply = FALSE;
   if(!receiveRMVReply(500, bGotReply))
   {
      // serious error: return to idle state
      m_nTargets = 0;
      m_iState = STATE_IDLE;
      return(FALSE);
   }

   // if no reply, the assumption is that it is taking too long to render the very first display frame.  This means
   // RMVideo cannot handle animation of the targets as defined, so we need to abort.  We call StopAnimation() in an
   // effort to return RMVideo to the idle state, then set the "device error" accordingly.
   if(!bGotReply)
   {
      if(StopAnimation()) SetDeviceError(CCxRMVideo::EMSG_ANIMSTARTFAIL);
      m_iState = STATE_IDLE;
      m_nTargets = 0;
      return(FALSE);
   }
   
   // if we got "first frame" signal in time, we're done; RMVideo is now in the "animating" state. Else, fail.
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = BOOL(len == 1 && sig == RMV_SIG_ANIMATEMSG);
   if(bOk)
   {
      ClearDeviceError();
      m_iState = STATE_ANIMATING;
      return(TRUE);
   }

   if(len == 1 && sig == RMV_SIG_CMDERR) 
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);

   m_nTargets = 0;
   m_iState = STATE_IDLE;
   return(FALSE);
}

/** UpdateAnimation ===================================================================================================
Deliver the target motion vectors for the next display frame in an ongoing RMVideo target animation sequence.

This method should be called only when RMVideo is in the "animating" state. To keep pace with RMVideo's frame rate, it
is essential to send the motion vectors for display frame N PRIOR to the start of display frame N-1; failure to do so 
will result in a duplicate frame on the RMVideo side (and RMVideo will send an RMV_SIG_ANIMATEMSG accordingly). This is
because RMVideo begins rendering display frame N on a back buffer as soon as the vertical sync for frame N-1 occurs.

After sending the supplied motion vectors to RMVideo, it checks for any message from RMVideo that's already in the 
network receive buffer (no waiting). If RMVideo has sent an error message, the device error is set and the method 
returns FALSE (but the animation sequence is not stopped). If RMVideo sent a duplicate frame message, it updates the
duplicate frame count for the ongoing animation and stores information about the duplicate frame event. It is up to
the caller to check the duplicate frame count (GetNumDuplicateFrames()) and decide whether or not to terminate the 
animation. Finally, RMVideo will send a "ping" message once per second, indicating the number of video frames that
have elapsed since the animation started. When this message is processed, the elapsed frame count is returned in the
argument provided; otherwise, the frame count is set to 0.

The method expects that the number of target motion vectors supplied in the argument is equal to the number of targets 
loaded. Also, the n-th motion vector will be applied to the trajectory of the n-th target loaded.

The animation sequence does not stop after calling this method (assuming it was running in the first place). Invoke
StopAnimation() to terminate the animation sequence. However, if the method fails due to a network error, this device
interface is disabled; the same is true if an unrecognized message is received from RMVideo. (It is still safe to call
StopAnimation() when the interface has been disabled; the call has no effect.)

As of Maestro 4.0.0/RMVideo v8, support was added for triggering vertical sync "spot flash" in the top-left corner of 
the screen during any frame of an animation sequence (as long as a flash is not currently in progress). 

@param pVecs [in] Target motion vectors for all loaded targets for the next display frame of the animation sequence.
@param bSync [in] If TRUE, the vertical sync spot flash is started during the next display frame, unless a previously
triggered flash is still in progress. Has no effect if the spot flash is disabled (zero spot size).
@param framesElapsed [out] The elapsed frame count for the animation sequence. As described above, this will be zero if
the requisite "ping" message was not received during this invocation of the function; if the ping was received, this will
contain the elapsed frame count.

@return TRUE if successful, FALSE otherwise (RMVideo unavailable or not currently animating; a network error occurred
while sending the motion vectors to RMVideo or receiving a message from RMVideo; received an error message from RMVideo).
*/
BOOL RTFCNDCL CCxRMVideo::UpdateAnimation(RMVTGTVEC* pVecs, BOOL bSync, int& framesElapsed)
{
   framesElapsed = 0;

   float fTemp;

   if(IsDown()) return(FALSE);
   if(m_iState != STATE_ANIMATING)
   {
      SetDeviceError("Attempted to update animation sequence on RMVideo when animation is not running!");
      return(FALSE);
   }

   // is the vertical sync spot flash to be triggered on this frame? Spot size must be non-zero.
   BOOL bEnaFlash = BOOL(bSync && (m_syncFlashSize > 0));

   // prepare the RMV_CMD_UPDATEFRAME command: UPDATEFRAME, SYNC?, N, V0(0), ..., V0(N-1), where each "V" is the target
   // index followed by the five parameters in the RMVTGTVEC structure...
   m_commandBuf[1] = RMV_CMD_UPDATEFRAME;
   m_commandBuf[2] = bEnaFlash ? 1 : 0;
   m_commandBuf[3] = m_nTargets;
   int iCmdIdx = 4;
   for( int i=0; i<m_nTargets; i++ )
   {
      m_commandBuf[iCmdIdx++] = i;
      m_commandBuf[iCmdIdx++] = (pVecs[i].bOn ) ? 1 : 0;
      fTemp = pVecs[i].hWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecs[i].vWin * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecs[i].hPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
      fTemp = pVecs[i].vPat * RMV_TGTVEC_F2I_F;
      m_commandBuf[iCmdIdx++] = int( (fTemp>0.0f) ? floor(fTemp+0.5f) : ceil(fTemp-0.5f) );
   }

   // now send the prepared command to RMVideo without waiting for a reply.  Return error indication if we could not
   // send command, but don't change state.
   m_commandBuf[0] = iCmdIdx-1;
   if(!sendRMVCommand()) { return(FALSE); }

   // check if there's a signal from RMVideo (without blocking). If we get an "error", "ping", or "duplicate frame" 
   // message from RMVideo, process it accordingly. Any other message is invalid, in which case we disable the device.
   BOOL bGotReply = FALSE;
   if(!receiveRMVReply(0, bGotReply)) return(FALSE);
   
   BOOL bOk = TRUE;
   if(bGotReply)
   {
      int len = m_replyBuf[0];
      int sig = m_replyBuf[1];
      
      if(sig == RMV_SIG_ANIMATEMSG && (len==2 || len==3))
      {
         if(len == 2)
            framesElapsed = m_replyBuf[2];
         else 
         {
            BOOL bMissedUpd = (m_replyBuf[3] == 0);
            m_nDupFrames += bMissedUpd ? 1 : m_replyBuf[3];
            if(m_nDupEvents < DUPBUFSZ)
            {
               m_dupEvent[m_nDupEvents][0] = bMissedUpd ? (m_replyBuf[2] + 1) :( m_replyBuf[2] - m_replyBuf[3]);
               m_dupEvent[m_nDupEvents][1] = bMissedUpd ? 0 : m_replyBuf[3];
               ++m_nDupEvents;
            }
         }
      }
      else
      {
         bOk = FALSE;
         if(len == 1 && sig == RMV_SIG_CMDERR)
            SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
         else
            disableOnError("Got unexpected reply to an 'update frame' command!");
      }
   }
   
   if(bOk) ClearDeviceError();
   return(bOk);
}

/** StopAnimation =====================================================================================================
Stop an ongoing RMVideo target animation sequence.

This method should be called only when RMVideo is in the "animating" state. It issues the RMV_CMD_STOPANIMATE command
and waits up to 1 second for RMVideo to return to the "idle" state. Normally, it should take less than one frame period
for RMVideo to respond, unless its hung up rendering a complex target scene (there's currently no way to "interrupt" 
RMVideo while it's drawing!). Whether it receives acknowledgement from RMVideo or not, the method effectively clears 
the animated target list and returns to the "idle" state. It should be considered a fatal error if StopAnimation() does
not succeed, since that means that CCxRMVideo is "out of synch" with RMVideo!

If called while RMVideo is "idle", the method will merely clear the animated target list if it is not already empty.

@return TRUE if successful, FALSE otherwise (RMVideo unavailable or the STOPANIMATE command failed).
*/
BOOL RTFCNDCL CCxRMVideo::StopAnimation()
{
   if(IsDown()) return(FALSE);

   // sleep for ~10ms intervals
   LARGE_INTEGER i64Sleep {};
   i64Sleep.QuadPart = (LONGLONG) 100000;

    // if RMVideo is animating, issue RMV_CMD_STOPANIMATE command and give RMVideo up to 10 seconds to return to the
   // "idle" state
   BOOL bOk = TRUE;
   if(m_iState == STATE_ANIMATING)
   {
      // first, send RMV_CMD_STOPANIMATE.
      m_commandBuf[0] = 1;
      m_commandBuf[1] = RMV_CMD_STOPANIMATE;
      int iReply = 0;
      bOk = sendRMVCommand();   // if this fails, device error is already set.
      if(bOk)
      {
         // now wait up to 1 second for RMVideo to confirm that it is in the idle state. If we have gotten out of sync
         // w/RMVideo, we may get multiple messages. Keep polling for RMVideo replies until a second has elapsed or we 
         // get RMV_SIG_IDLE.
         CElapsedTime eTime;
         BOOL bSocketOk = TRUE;
         BOOL bIdled = FALSE;
         while(eTime.Get() < 1000000.0 && bSocketOk && !bIdled)
         {
            // poll about once every 10 ms
            ::RtSleepFt(&i64Sleep);
            
            BOOL bGotReply = FALSE;
            bSocketOk = receiveRMVReply(0, bGotReply);
            bIdled = bSocketOk && bGotReply && (m_replyBuf[0] == 1) && (m_replyBuf[1] == RMV_SIG_IDLE);
         }

         // success only if RMVideo returned to idle state.  Else, if a socket error did not occur, then we timed out.
         bOk = bIdled;
         if(bSocketOk && !bOk)
            disableOnError(CCxRMVideo::EMSG_TIMEOUT);
      }
   }

   // clear target list and return to idle state no matter what
   m_iState = STATE_IDLE;
   m_nTargets = 0;

   if(bOk) ClearDeviceError();
   return(bOk);
}

/** GetNumDuplicateFrameEvents
Get the total number of duplicate frame events -- caused either by a missed target update or by a rendering delay -- that
occurred during the last animation sequence. Be sure to terminate the animation before calling this method.

NOTE: The number of duplicate frame events is not necessarily the same as the total number of duplicate frames, since a
rendering delay on the RMVideo side could last multiple refresh periods and thus result in a contiguous run of duplicate
frames reported as a single event. Also, CCxRMVideo will only store information on the first 100 duplicate frame events
during an animation sequence, but it will keep track of the total # of duplicate frames throughout.]

@return Number of duplicate frame events; returns 0 if device is disabled or if an animation sequence is ongoing.
*/
int RTFCNDCL CCxRMVideo::GetNumDuplicateFrameEvents()
{
   return((IsDown() || m_iState == STATE_ANIMATING) ? 0 : m_nDupEvents);
}

/** GetDuplicateFrameEventInfo
Retrieve information about a duplicate frame event that occurred during the last animation sequence. Be sure to stop 
the animation sequence before calling this method. 

@param idx [in] The event index, between 0 and N-1, where N is the value returned by GetNumDuplicateFrameEvents().
@param frame [out] The frame count at the start of the first duplicate frame for this event.
@param count [out] The number of consecutive duplicate frames for this event. If 0, then there is a single duplicate
frame because a target update from Maestro arrived too late; otherwise, this is the number of duplicate frames 
caused by a rendering delay in RMVideo.

@return TRUE if successful; FALSE if the device is disabled, an animation sequence is in progress, or the event index
is not valid. On failure, the event information is invalid.
*/
BOOL RTFCNDCL CCxRMVideo::GetDuplicateFrameEventInfo(int idx, int& frame, int& count)
{
   if(IsDown() || m_iState == STATE_ANIMATING || idx < 0 || idx >= m_nDupEvents) return(FALSE);

   frame = m_dupEvent[idx][0];
   count = m_dupEvent[idx][1];
   return(TRUE);
}

/**
 * Get a list of all folders currently present in RMVideo's media store. This store holds video files that played back
 * during an animation via the RMV_MOVIE target class, and/or image files containing images displayed as RMV_IMAGE 
 * targets. It is a simple file-based store in which all media files are stored in one of up to RMV_MVF_LIMIT folders. 
 * Each folder may contain up to RMV_MVF_LIMIT files. Folder names and file names are limited in length (RMV_MVF_LEN)
 * and character content. Any video file that can be read by the FFMPEG library used in RMVideo is a valid candidate
 * for the media store, as is any JPG, PNG, BMP or PSD file that can be read by the STB_IMAGE library. See RMVideo
 * source code for more information about video and image file support.
 *
 * CCxRMVideo does not maintain a "table of contents" for the RMVideo server's media store. Each time you call 
 * GetMediaFolders(), GetMediaFiles(), or GetMediaInfo(), a command is sent to the server to retrieve the requested 
 * information, waiting up to 1s for a response. Never invoke these methods during an animation sequence.
 * 
 * @param n [out] The number of folders present in the media store.
 * @param pBuf [out] This character buffer holds a null-separated list of the folder names. It is assumed that the 
 * buffer is large enough for the worst-case scenario: RMV_MVF_LIMIT * (RMV_MVF_LEN + 1).
 * @return TRUE if successful; FALSE otherwise (device error message set).
 */
BOOL RTFCNDCL CCxRMVideo::GetMediaFolders(int& n, char* pBuf) 
{
   if(IsDown()) return(FALSE);
   if(m_iState != STATE_IDLE)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_GETMEDIADIRS;
   
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(1000)) return(FALSE);
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = (len >= 2) && (sig == RMV_SIG_CMDACK);     // if movie store empty, len == 2 !!!
   if(bOk)
   {
      n = m_replyBuf[2];
      char* pSrc = (char*) &(m_replyBuf[3]);
      char* pDst = pBuf;
      for(int i=0; i<n; i++)
      {
         int j = (int) ::strlen(pSrc);
         if(j > 0 && j <= RMV_MVF_LEN && j == (int) ::strspn(pSrc, RMV_MVF_CHARS))
         {
            ::strcpy_s(pDst, j+1, pSrc);
            pSrc += (j+1);
            pDst += (j+1);
         }
         else
         {
            SetDeviceError("Invalid media folder name found in RMVideo reply!");
            return(FALSE);
         }
      }
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);
   
   return(bOk);
}

/**
 * Get a list of all media files currently present within the specified folder in RMVideo's media store. Allow for the
 * possibility that there are no files in the folder. This could happen if the user creates a folder manually on the
 * RMVideo side and fails to remove it.
 * @param strFolder Name of the folder for which a media file listing is requested.
 * @param n [out] The number of files present in the folder specified.
 * @param pBuf [out] This character buffer holds a null-separated list of the media file names. It is assumed that the 
 * buffer is large enough for the worst-case scenario: RMV_MVF_LIMIT * (RMV_MVF_LEN + 1).
 * @return TRUE if successful; FALSE otherwise (device error message set).
 * @see CCxRMVideo::GetMediaFolders()
 */
BOOL RTFCNDCL CCxRMVideo::GetMediaFiles(LPCTSTR strFolder, int& n, char* pBuf)
{
   if(IsDown()) return(FALSE);
   if(m_iState != STATE_IDLE)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(strFolder == NULL || ::strlen(strFolder) == 0 || ::strlen(strFolder) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   
   int nBytes = (int) (::strlen(strFolder) + 1);
   while((nBytes % 4) != 0) nBytes++;
   
   m_commandBuf[0] = 1 + (nBytes/4);
   m_commandBuf[1] = RMV_CMD_GETMEDIAFILES;
   for(int i=0; i<(nBytes/4); i++) m_commandBuf[2+i] = 0;
   char* pStr = (char*) &(m_commandBuf[2]);
   ::strcpy_s(pStr, nBytes, strFolder);
   
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(1000)) return(FALSE);
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = (len >= 2) && (sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      n = m_replyBuf[2];
      char* pSrc = (char*) &(m_replyBuf[3]);
      char* pDst = pBuf;
      for(int i=0; i<n; i++)
      {
         int j = (int) ::strlen(pSrc);
         if(j > 0 && j <= RMV_MVF_LEN && j == (int) ::strspn(pSrc, RMV_MVF_CHARS))
         {
            ::strcpy_s(pDst, j+1, pSrc);
            pSrc += (j+1);
            pDst += (j+1);
         }
         else
         {
            SetDeviceError("Invalid movie file name found in RMVideo reply!");
            return(FALSE);
         }
      }
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);
   
   return(bOk);
}

/**
 * Get summary information on a particular media file in RMVideo's media store.
 *
 * @param strFolder Name of the folder containing the media file in question.
 * @param strFile Name of the media file in question.
 * @param w, h, rate, dur [out] For a video file, these params will hold the video frame width and height in pixels, 
 * the ideal playback rate in Hz, and the approximate duration of the video in seconds. For an image file, the first
 * two params hold the image width and height in pixels, while the last two are both negative to indicate that the 
 * file is NOT a video file. If RMVideo was unable to determine a parameter's value, that value is set to zero.
 * @return TRUE if successful; FALSE otherwise (device error message set).
 * @see CCxRMVideo::GetMediaFolders()
 */
BOOL RTFCNDCL CCxRMVideo::GetMediaInfo(LPCTSTR strFolder, LPCTSTR strFile, int& w, int& h, float& rate, float& dur)
{
   if(IsDown()) return(FALSE);
   if(m_iState != STATE_IDLE)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(strFolder == NULL || ::strlen(strFolder) == 0 || ::strlen(strFolder) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   if(strFile == NULL || ::strlen(strFile) == 0 || ::strlen(strFile) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   
   int nFolderLen = (int) (::strlen(strFolder) + 1);
   int nFileLen = (int) (::strlen(strFile) + 1);
   int nBytes = nFolderLen + nFileLen;
   while((nBytes % 4) != 0) nBytes++;
   
   m_commandBuf[0] = 1 + (nBytes/4);
   m_commandBuf[1] = RMV_CMD_GETMEDIAINFO;
   for(int i=0; i<(nBytes/4); i++) m_commandBuf[2+i] = 0;
   char* pStr = (char*) &(m_commandBuf[2]);
   ::strcpy_s(pStr, nFolderLen, strFolder);
   pStr += nFolderLen;
   ::strcpy_s(pStr, nFileLen, strFile);
   
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(1000)) return(FALSE);
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = (len == 5) && (sig == RMV_SIG_CMDACK);
   if(bOk)
   {
      w = m_replyBuf[2];
      h = m_replyBuf[3];
      rate = ((float) m_replyBuf[4]) / 1000.0f;
      dur = ((float) m_replyBuf[5]) / 1000.0f;
      if(rate < 0.0f || dur < 0.0f) { rate = -1.0f; dur = -1.0f; }   // indicates file contains an image, not a video
   }
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);
   
   return(bOk);
}

/**
 * Delete a particular media file or an entire folder in RMVideo's media store. This method will wait up to 5 seconds
 * for an acknowledgement from the RMVideo server that the operation was completed.
 *
 * @param strFolder Name of the folder containing the medi file to remove.
 * @param strFile Name of the media file to remove. If NULL, all files in the specified folder are deleted. Otherwise,
 * the folder is removed if this is the last media file in it.
 * @return TRUE if successful; FALSE otherwise (device error message set).
 * @see CCxRMVideo::GetMovieFolders()
 */
BOOL RTFCNDCL CCxRMVideo::DeleteMediaFile(LPCTSTR strFolder, LPCTSTR strFile)
{
   if(IsDown()) return(FALSE);
   if(m_iState != STATE_IDLE)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(strFolder == NULL || ::strlen(strFolder) == 0 || ::strlen(strFolder) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   if(strFile != NULL && (::strlen(strFile) == 0 || ::strlen(strFile) > RMV_MVF_LEN))
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   
   int nFolderLen = (int) (::strlen(strFolder) + 1);
   int nFileLen = (strFile != NULL) ? ((int) (::strlen(strFile) + 1)) : 0;
   int nBytes = nFolderLen + nFileLen;
   while((nBytes % 4) != 0) nBytes++;
   
   m_commandBuf[0] = 1 + (nBytes/4);
   m_commandBuf[1] = RMV_CMD_DELETEMEDIA;
   for(int i=0; i<(nBytes/4); i++) m_commandBuf[2+i] = 0;
   char* pStr = (char*) &(m_commandBuf[2]);
   ::strcpy_s(pStr, nFolderLen, strFolder);
   if(nFileLen > 0)
   {
      pStr += nFolderLen;
      ::strcpy_s(pStr, nFileLen, strFile);
   }
   
   if(!sendRMVCommand()) return(FALSE);
   if(!receiveRMVReply(5000)) return(FALSE);
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   BOOL bOk = (len == 1) && (sig == RMV_SIG_CMDACK);
   if(bOk) 
      ClearDeviceError();
   else if(len == 1 && sig == RMV_SIG_CMDERR)
      SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
   else
      disableOnError(CCxRMVideo::EMSG_BADREPLY);
   
   return(bOk);
}


/**
 * Download a file to the RMVideo's media store. This method will take an indefinite period of time to finish, 
 * depending on the size of the file to be downloaded. It may only be used in the idle state.
 *
 * @param srcPath File system pathname for the media file to be downloaded. If this file does not exist or is not a 
 * video or image file that RMVideo can handle, the operation fails.
 * @param strFolder Name of the destination folder in the RMVideo media store. If the folder does not exist, it will be
 * created -- unless the media store already contains RMV_MVF_LIMIT folders, in which case the operation fails.
 * @param strFile Name of the media file within the specified folder. If the folder already contains a media file with
 * this name, the operation fails.
 * @return TRUE if successful; FALSE otherwise (device error message set).
 * @see CCxRMVideo::GetMovieFolders()
 */
BOOL RTFCNDCL CCxRMVideo::DownloadMediaFile(LPCTSTR srcPath, LPCTSTR strFolder, LPCTSTR strFile)
{
   if(IsDown()) return(FALSE);
   if(m_iState != STATE_IDLE)
   {
      SetDeviceError(CCxRMVideo::EMSG_NOTWHILEANIM);
      return(FALSE);
   }
   if(srcPath == NULL || ::strlen(srcPath) == 0)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   if(strFolder == NULL || ::strlen(strFolder) == 0 || ::strlen(strFolder) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   if(strFile == NULL || ::strlen(strFile) == 0 || ::strlen(strFile) > RMV_MVF_LEN)
   {
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }

   return(putFile(srcPath, strFolder, strFile));
}


//=== RecalcDegToPix ==================================================================================================
//
//    Based on the current geometry for the RMVideo display, this method calculates the multiplicative factor that
//    converts position in deg to pixels.  It should be called whenever the display geometry is modified.
//
//    The RMVideo display is treated as an NxM array of pixels, where N and M are retrieved from RMVideo at startup and
//    whenever the video mode changes. To compute the conversion factor for the H(V) component, we divide the display's 
//    half-width (half-height) in pixels by the angle (in deg) subtended at the eye by that half-width (half-height). 
//    However, it is a BASIC ASSUMPTION in CCxRMVideo and RMVideo itself that the display geometry is such that the H 
//    and V factors are nearly the same, and we use the average of the two as a single composite conversion factor. 
//    Targets will be stretched in one direction or another if this assumption is not satisfied.
//
//    This is the same calculation performed by RMVideo itself (all target parameters and motion update vectors are
//    sent to RMVideo in visual units (deg or deg/sec).
//
//    DEVNOTE:  The conversion factor is only reasonable for small pos changes.  For large pos changes, one should
//    do the trigonometric calculations... perhaps we should get rid of the (assumed linear) multiplicative factors
//    entirely and do the trigonometry every time.  But this could have too negative an impact on performance???
//
//    Since we divide by this factor to convert pix to deg, we set it to 1 if it is calculated to be 0 -- this only
//    happens if the display geometry is not valid (zero W,H; or zero dist to eye).
//
VOID RTFCNDCL CCxRMVideo::RecalcDegToPix()
{
   double d1 = double(GetScreenW_pix()) / 2.0;

   double d2 = ::atan2( double(m_iWidth)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD;
   m_dDegToPix = (d2 != 0.0) ? d1/d2 : 1.0;

   d1 = double(GetScreenH_pix()) / 2.0;
   d2 = ::atan2( double(m_iHeight)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD;
   m_dDegToPix += (d2 != 0.0) ? d1/d2 : 1.0;

   m_dDegToPix /= 2.0;
}

//=== OpenEx ==========================================================================================================
// Open the connection to RMVideo over a RTX-controlled TCP/IP socket and enter the idle state.
// 
// This method must be called by the device manager rather than CDevice::Open(). The method will, in fact, invoke
// Open() to perform the basic tasks of setting up the socket connection and issuing the RMV_CMD_STARTINGUP command 
// that awakens RMVideo. It then queries the application version number (RMV_CMD_GETVERSION) to make sure it matches 
// the version expected on this end. If not, RMVideo will not be available.
//
// Once the command session is setup and the version number validated, the method retrieves RMVideo's current and all 
// available video modes (frame rate and display resolution), retrieves the current monitor gamma, initializes the 
// display geometry to default values, and sets the initial background color to black.
//
// @param pIO IO link for posting messages to Maestro's GUI process during initializations.
// @return TRUE if successful; FALSE otherwise, in which case CCxRMVideo is not available (and the failure description
// is available by calling GetLastDeviceError()).
// @see OnOpen()
BOOL RTFCNDCL CCxRMVideo::OpenEx(CCxMasterIO *pIO)
{
   // open the "device"
   if(!Open()) return(FALSE);
   
   pIO->Message("Starting session with RMVideo...");
   
   // retrieve version number. If reply is RMV_SIG_CMDERR, then version is 0.
   int ver = -1;
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_GETVERSION;
   BOOL bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(250);
   if(bOk)
   {
      int len = m_replyBuf[0];
      ver = m_replyBuf[1];
      if(len == 1 && ver == RMV_SIG_CMDERR) ver = 0;
      bOk = (len == 1) && (ver >= 0);
      if(!bOk) SetDeviceError(CCxRMVideo::EMSG_BADREPLY);
   }
   if(!bOk)
   {
      pIO->Message("==> Unable to validate RMVideo program version -- disconnecting...");
      ::strcpy_s(m_errMsg, GetLastDeviceError());
      Close();
      SetDeviceError(m_errMsg);
      return(FALSE);
   }

   // if RMVideo version is not what we expect, disconnect.
   if(ver == RMV_CURRENTVERSION)
   {
      ::sprintf_s(m_errMsg, "==> Verified RMVideo version: %d", ver);
      pIO->Message(m_errMsg);
   }
   else
   {
      ::sprintf_s(m_errMsg, "==> RMVideo version (%d) is invalid or out of date -- disconnecting...", ver);
      pIO->Message(m_errMsg);
      Close();
      SetDeviceError("RMVideo version mismatch");
      return(FALSE);
   }
   
   // retrieve all available video modes for the RMVideo display. NOTE that we only expose up to RMV_MAXVMODES; it is
   // very unlikely a given RMVideo system will even have that many.
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_GETALLVIDEOMODES;
   bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(1000);
   int len = m_replyBuf[0];
   int sig = m_replyBuf[1];
   int n = m_replyBuf[2];
   if(bOk) 
   {
      bOk = (sig == RMV_SIG_CMDACK) && (len == (2 + n*3));
      if(!bOk) SetDeviceError(CCxRMVideo::EMSG_BADREPLY);
   }
   if(bOk)
   {
      m_nModes = (n <= RMV_MAXVMODES) ? n : RMV_MAXVMODES;
      int j = 3;
      for(int i=0; i<m_nModes; i++)
      {
         m_videoModes[i].w = m_replyBuf[j++];
         m_videoModes[i].h = m_replyBuf[j++];
         m_videoModes[i].rate = m_replyBuf[j++];
      }
      ::sprintf_s(m_errMsg, "==> Found %d available video modes that meet or exceed minimum requirements.", m_nModes);
      pIO->Message(m_errMsg);
   }
   else
   {
      pIO->Message("==> Failed while retrieving available video modes -- disconnecting...");
      ::strcpy_s(m_errMsg, GetLastDeviceError());
      Close();
      SetDeviceError(m_errMsg);
      return(FALSE);
   }

   // retrieve the current mode. Even if only one mode available, do this to retrieve measured frame period!
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_GETCURRVIDEOMODE;
   bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(250);
   if(bOk)
   {
      bOk = (m_replyBuf[0] == 3) && (m_replyBuf[1] == RMV_SIG_CMDACK);
      if(bOk)
      {
         m_iCurrMode = m_replyBuf[2] - 1;
         m_dFramePeriod = ((double) m_replyBuf[3]) / 1.0e9;
         bOk = m_iCurrMode >= 0 && m_iCurrMode < m_nModes && m_dFramePeriod > 0;
      }
      if(!bOk) SetDeviceError(CCxRMVideo::EMSG_BADREPLY);
   }
   if(bOk)
   {
      VideoMode* pMode = &(m_videoModes[m_iCurrMode]);
      ::sprintf_s(m_errMsg, "==> Using mode %d: %d x %d @ %d Hz (frame period = %.4f ms)...", m_iCurrMode, 
         pMode->w, pMode->h, pMode->rate, m_dFramePeriod * 1.0e3);
      pIO->Message(m_errMsg);
   }
   else
   {
      pIO->Message("==> Failed while retrieving current video mode -- disconnecting...");
      ::strcpy_s(m_errMsg, GetLastDeviceError());
      Close();
      SetDeviceError(m_errMsg);
      return(FALSE);
   }
   
   // retrieve the current monitor gamma
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_GETGAMMA;
   bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(250);
   if(bOk) 
   {
      bOk = (m_replyBuf[0] == 4) && (m_replyBuf[1] == RMV_SIG_CMDACK);
      for(int i=0; bOk && i<3; i++)
      {
         m_gamma[i] = m_replyBuf[2+i];
         bOk = (m_gamma[i] >= RMV_MINGAMMA) && (m_gamma[i] <= RMV_MAXGAMMA);
      }
      if(!bOk) SetDeviceError(CCxRMVideo::EMSG_BADREPLY);
   }
   if(bOk)
   {
      ::sprintf_s(m_errMsg, "==> Current monitor gamma: r=%.2f, g=%.2f b=%.2f...", ((double)m_gamma[0])/1000.0,
         ((double)m_gamma[1])/1000.0, ((double)m_gamma[2])/1000.0);
      pIO->Message(m_errMsg);
   }
   else
   {
      pIO->Message("==> Failed while retrieving current monitor gamma -- disconnecting...");
      ::strcpy_s(m_errMsg, GetLastDeviceError());
      Close();
      SetDeviceError(m_errMsg);
      return(FALSE);
   }
   
   // initialize display geometry and background color
   if( !SetGeometry(DEF_DISTTOEYE_MM, DEF_WIDTH_MM, DEF_HEIGHT_MM) || !SetBkgColor(0, 0, 0) )
   {
      pIO->Message("==> Failed while initializing display geometry and background color -- disconnecting...");
      ::strcpy_s(m_errMsg, GetLastDeviceError());
      Close();
      SetDeviceError(m_errMsg);
      return(FALSE);
   }

   return(TRUE);
}

//=== OnOpen ==========================================================================================================
//
//    Establish a non-blocking TCP/IP socket connection with RMVideo and start a command session.
//
//    BACKGROUND:
//    The RMVideo application runs "forever" on a Linux workstation.  By design, Maestro and RMVideo will communicate
//    over a dedicated, private Ethernet connection; each workstation will have a second NIC that participates in this
//    private point-to-point connection. RMVideo acts as a "server" that handles only a single Maestro "client" at a
//    time. Whenever a Maestro command session is not in progress, RMVideo simply waits quietly, ready to accept a
//    connection on its dedicated IPV4 address and port# (see RMVIDEO_COMMON.H). Once a connection is established, it
//    expects an RMV_CMD_STARTINGUP command, at which point it opens a fullscreen window on the RMVideo monitor and
//    enters the "idle" state, sending the RMV_SIG_IDLE signal back to Maestro. It then services all Maestro commands
//    until it receives RMV_CMD_SHUTTINGDN, at which point it issues RMV_SIG_BYE, terminates the connection, and then
//    starts waiting for a new connection.
//
//    This method handles the steps involved in initiating a command session with the RMVideo server:  making the
//    connection, sending the RMV_CMD_STARTINGUP command, and then waiting for the RMV_SIG_IDLE signal from RMVideo.
//    We use a non-blocking TCP/IP socket, and allow up to 2 seconds to make a connection and up to 10 seconds to
//    get the RMV_SIG_IDLE signal.
//
//    NOTE: WSAStartup() is NOT called here. It should only be invoked once by an RTSS proces, so I've moved the call
//    to CCxDeviceMgr::Startup().
//
//    ARGS:       NONE.
//    RETURNS:    True if successful; false otherwise (an error message is available via GetLastDeviceError()).
//
BOOL RTFCNDCL CCxRMVideo::OnOpen()
{
   // create a socket for our connection
   m_rmvSocket = socket(PF_INET, SOCK_STREAM, 0);
   if( m_rmvSocket == INVALID_SOCKET )
   {
      ::sprintf_s( m_errMsg, "Unable to create TCPIP socket (%d)!", WSAGetLastError() );
      SetDeviceError(m_errMsg);
      return( FALSE );
   }

   // make the socket non-blocking
   u_long enable = (u_long) 1;
   int iRes = ioctlsocket( m_rmvSocket, FIONBIO, &enable );
   if( iRes == SOCKET_ERROR )
   {
      ::sprintf_s( m_errMsg, "Unable to make socket non-blocking (%d)!", WSAGetLastError() );
      SetDeviceError(m_errMsg);
      closesocket( m_rmvSocket );
      m_rmvSocket = INVALID_SOCKET;
      return( FALSE );
   }

   // disable Nagle algorithm so we can send short commands without delay
   int noDelay = 1;
   iRes = setsockopt(m_rmvSocket, IPPROTO_TCP, TCP_NODELAY, (const char *) &noDelay, sizeof(int));
   if( iRes == SOCKET_ERROR )
   {
      ::sprintf_s( m_errMsg, "Unable to disable Nagle algorithm (%d)!", WSAGetLastError() );
      SetDeviceError(m_errMsg);
      closesocket( m_rmvSocket );
      m_rmvSocket = INVALID_SOCKET;
      return( FALSE );
   }

   // specify socket buffer space reserved for sends.  Make it large enough to accommodate a worst-case size command.
   // We don't worry about recv buffer space, because RMVideo sends very little back to us!
   int iSendBufSize = (int) (RMV_MAXCMDSIZE + 1)*sizeof(int);
   iRes = setsockopt(m_rmvSocket, SOL_SOCKET, SO_SNDBUF, (const char *) &iSendBufSize, sizeof(int));
   if( iRes == SOCKET_ERROR )
   {
      ::sprintf_s( m_errMsg, "Unable to set socket send buffer size (%d)!", WSAGetLastError() );
      SetDeviceError(m_errMsg);
      closesocket( m_rmvSocket );
      m_rmvSocket = INVALID_SOCKET;
      return( FALSE );
   }

   // bind socket to our dedicated, private IP address.  We don't care about the port#.  RMVideo will check address of
   // any client that connects to it and will reject the connection if the IP address is not what it expected!
   struct sockaddr_in localAddr {};
   localAddr.sin_family = AF_INET;
   iRes = inet_pton(AF_INET, RMVNET_MAESTROADDR, &localAddr.sin_addr);
   if(iRes != 1)
   {
      ::sprintf_s(m_errMsg, "Bad IP address for Maestro; inet_pton() returned %d", iRes);
      SetDeviceError(m_errMsg);
      closesocket(m_rmvSocket);
      m_rmvSocket = INVALID_SOCKET;
      return(FALSE);
   }
   localAddr.sin_port = htons(0);
   memset( &(localAddr.sin_zero), '\0', 8);
   iRes = bind(m_rmvSocket, (const struct sockaddr *) &localAddr, sizeof(localAddr));
   if( iRes == SOCKET_ERROR )
   {
      ::sprintf_s( m_errMsg, "Unable to bind socket to %s (%d)!", RMVNET_MAESTROADDR, WSAGetLastError() );
      SetDeviceError(m_errMsg);
      closesocket( m_rmvSocket );
      m_rmvSocket = INVALID_SOCKET;
      return( FALSE );
   }

   // now try to connect to RMVideo server, at a dedicated IP address and on a dedicated port.  If we don't connect
   // within 2 seconds, give up.
   struct sockaddr_in serverAddr {};
   serverAddr.sin_family = AF_INET;
   iRes = inet_pton(AF_INET, RMVNET_RMVADDR, &serverAddr.sin_addr);
   if (iRes != 1)
   {
      ::sprintf_s(m_errMsg, "Bad IP address for RMVideo; inet_pton() returned %d", iRes);
      SetDeviceError(m_errMsg);
      closesocket(m_rmvSocket);
      m_rmvSocket = INVALID_SOCKET;
      return(FALSE);
   }
   serverAddr.sin_port = htons(RMVNET_RMVPORT);
   memset( &(serverAddr.sin_zero), '\0', 8);
   iRes = connect(m_rmvSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
   if( iRes == SOCKET_ERROR )
   {
      // we expect to end up here b/c the socket is non-blocking.  If that's NOT the reason, then something's wrong!
      int iError = WSAGetLastError();

      // DEVNOTE: if socket is non-blocking, the connect() call returns WSAEINPROGRESS when it should return
      // WSAEWOULDBLOCK.  This is an error in the RTX implementation!
      if( iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS )
      {
         ::sprintf_s( m_errMsg, "Unable to connect to RMVideo server (%d)!", iError );
         SetDeviceError(m_errMsg);
         closesocket( m_rmvSocket );
         m_rmvSocket = INVALID_SOCKET;
         return( FALSE );
      }

      // wait up to 2 seconds for the connection to be established
      fd_set writefds{};
      FD_ZERO(&writefds);
      FD_SET(m_rmvSocket, &writefds);
      struct timeval connectTO {};
      connectTO.tv_sec = 2;
      connectTO.tv_usec = 0;
      iRes = select(0, NULL, &writefds, NULL, &connectTO);   // note that first param is ignored in Winsock impl
      if( iRes == SOCKET_ERROR )
      {
         ::sprintf_s( m_errMsg, "Error occurred while connecting to RMVideo server (%d)!", WSAGetLastError() );
         SetDeviceError(m_errMsg);
         closesocket( m_rmvSocket );
         m_rmvSocket = INVALID_SOCKET;
         return( FALSE );
      }
      else if( iRes == 0 )
      {
         SetDeviceError( "Timed out while trying to connect to RMVideo server!" );
         closesocket( m_rmvSocket );
         m_rmvSocket = INVALID_SOCKET;
         return( FALSE );
      }
   }

   // ok, prepare and send RMV_CMD_STARTINGUP and wait up to 10 seconds for the RMV_SIG_IDLE reply
   m_commandBuf[0] = 1;
   m_commandBuf[1] = RMV_CMD_STARTINGUP;
   BOOL bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(10000);
   if(bOk) 
   {
      bOk = (m_replyBuf[0] == 1) && (m_replyBuf[1] == RMV_SIG_IDLE);
      if(!bOk) 
      {
         ::sprintf_s(m_errMsg, "Bad reply from RMVideo at startup (len = %d sigCode=%d)!", 
            m_replyBuf[0], m_replyBuf[1]);
         SetDeviceError(m_errMsg);
      }
   }
   
   if(!bOk)
   {
      closesocket(m_rmvSocket);
      m_rmvSocket = INVALID_SOCKET;
   }
   else
   {
      m_iState = STATE_IDLE;
      m_nTargets = 0;
      ClearDeviceError();
   }
   return(bOk);
}

//=== OnClose =========================================================================================================
//
//    Terminate current RMVideo command session, then close and release the socket connection to the RMVideo server.
//
//    SEE ALSO: OnOpen().
//
//    This method will send the RMV_CMD_SHUTTINGDN command to inform RMVideo that we're terminating the connection
//    at this end.  It waits up to 10 seconds to get the RMV_SIG_BYE acknowledgement from RMVideo, then closes the
//    TCIP/IP socket.
//
//    NOTE: WSACleanup() is NOT called here. It should only be invoked once by an RTSS proces, so I've moved the call
//    to CCxDeviceMgr::Shutdown().
//
//    ARGS:       NONE.
//    RETURNS:    NONE (prints an info message with RtPrintf).
//
VOID RTFCNDCL CCxRMVideo::OnClose()
{
   // just in case the socket is already closed
   if( m_rmvSocket == INVALID_SOCKET ) return;

   // send RMV_CMD_SHUTTINGDN and wait up to 10 sec for RMV_SIG_BYE acknowledgement from RMVideo.  We do not
   // confirm that acknowledgement! Don't bother with this if interface is disabled by a prior problem.
   if(!m_bDisabled)
   {
      m_commandBuf[0] = 1;
      m_commandBuf[1] = RMV_CMD_SHUTTINGDN;
      sendRMVCommand();
      receiveRMVReply(10000);
   }

   // close the socket
   closesocket( m_rmvSocket );
   m_rmvSocket = INVALID_SOCKET;

   // reset internal state info
   m_dFramePeriod = 0;
   m_nModes = 0;
   m_iCurrMode = -1;
   m_gamma[0] = m_gamma[1] = m_gamma[2] = 1000;

   m_iDistToEye = DEF_DISTTOEYE_MM;
   m_iWidth = DEF_WIDTH_MM;
   m_iHeight = DEF_HEIGHT_MM;
   m_bkgRGB = 0;

   m_dDegToPix = 1.0;

   m_iState = STATE_IDLE;
   m_nTargets = 0;

   m_iReplyBytesRcvd = 0;
   m_iCmdBytesSent = 0;

   m_bDisabled = FALSE;

   m_nDupEvents = m_nDupFrames = 0;
}


//=== putFile =========================================================================================================
// Helper method handles a file download from Maestro to RMVideo using the RMV_CMD_PUTFILE, _PUTFILECHUNK, _PUTFILEDONE
// command sequence. Media files are transferred to the RMVideo workstation using this method.
//
// @param srcPath The media file's source path on host machine.
// @param mvDir Name of destination media folder.
// @param mvFile Name of destination media file.
// @return TRUE if successful, FALSE otherwise (device error message set)
BOOL RTFCNDCL CCxRMVideo::putFile(LPCTSTR srcPath, LPCTSTR mvDir, LPCTSTR mvFile)
{
   // check arguments
   char path[256];
   BOOL bOk = (mvDir != NULL) && (mvFile != NULL) && (srcPath != NULL) && (::strlen(srcPath) < 256);
   if(bOk)
   {
      size_t len = ::strlen(mvDir);
      bOk = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(mvDir, RMV_MVF_CHARS));
   }
   if(bOk)
   {
      size_t len = ::strlen(mvFile);
      bOk = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(mvFile, RMV_MVF_CHARS));
   }
   if(!bOk)
   {
      SetDeviceError("RMVideo file download failed: Bad source path, or bad media folder or file name!");
      return(FALSE);
   }
   else
      ::strcpy_s(path, srcPath);

   // attempt to open the file
   HANDLE hFile = ::CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
   if(hFile == INVALID_HANDLE_VALUE)
   {
      SetDeviceError("RMVideo file download failed: Unable to open source file!");
      return(FALSE);
   }
   
   // send RMV_CMD_PUTFILE to initiate file transfer
   int dirLen = (int) (::strlen(mvDir) + 1);
   int fileLen = (int) (::strlen(mvFile) + 1);
   int nBytes = dirLen + fileLen;
   while((nBytes % 4) != 0) nBytes++;
   m_commandBuf[0] = 1 + (nBytes/4);
   m_commandBuf[1] = RMV_CMD_PUTFILE;
   for(int i=0; i<(nBytes/4); i++) m_commandBuf[2+i] = 0;
   char* pStr = (char*) &(m_commandBuf[2]);
   ::strcpy_s(pStr, dirLen, mvDir);
   pStr += dirLen;
   ::strcpy_s(pStr, fileLen, mvFile);

   bOk = sendRMVCommand();
   if(bOk) bOk = receiveRMVReply(2000);
   if(!bOk)
   {
      // communications failure. Device error already set. Resync may be attempted.
      ::CloseHandle(hFile);
      return(FALSE);
   }
   else bOk = (m_replyBuf[0] == 1) && (m_replyBuf[1] == RMV_SIG_CMDACK);

   if(!bOk)
   {
      ::CloseHandle(hFile);
      if(m_replyBuf[0] == 1 && m_replyBuf[1] == RMV_SIG_CMDERR)
         SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
      else
         disableOnError(CCxRMVideo::EMSG_BADREPLY);
      return(FALSE);
   }

   // read in file contents and transfer in 2KB chunks using RMV_CMD_PUTFILECHUNK
   m_commandBuf[1] = RMV_CMD_PUTFILECHUNK;
   char* pBytes = (char*) &(m_commandBuf[3]);
   BOOL done = FALSE;
   DWORD dwBytesRead = 0;
   BOOL sendCancel = FALSE; 
   while(!done)
   {
      // 23may2016: IMPORTANT - This Win32 call FAILS to behave as advertised when EOF is reached - the method returns
      // TRUE, but dwBytesRead is set to some very large number that is negative when cast to an int. Thus, we verify
      // that dwBytesRead is in (0,2048] before sending bytes to RMVideo!!!
      bOk = ::ReadFile(hFile, (LPVOID) pBytes, (DWORD) 2048, &dwBytesRead, NULL);
      if(!bOk)
      {
         SetDeviceError("RMVideo file download failed: IO error occurred while reading file on Maestro host!");
         sendCancel = TRUE;
         done = TRUE;
      }
      else if(dwBytesRead <= 0 || dwBytesRead > 2048)
         done = TRUE;  // EOF reached
      else
      {
         m_commandBuf[2] = (int) dwBytesRead;
         while(dwBytesRead % 4 != 0) pBytes[dwBytesRead++] = 0;
         m_commandBuf[0] = 2 + (dwBytesRead/4);
         bOk = sendRMVCommand();
         if(bOk) bOk = receiveRMVReply(2000);
         if(!bOk)
         {
            // communications failure. Device error already set.
            done = TRUE;
         }
         else 
         {
            bOk = (m_replyBuf[0] == 1) && (m_replyBuf[1] == RMV_SIG_CMDACK);
            if(!bOk)
            {
               if(m_replyBuf[0] == 1 && m_replyBuf[1] == RMV_SIG_CMDERR)
                  SetDeviceError(CCxRMVideo::EMSG_CMDERROR);
               else
                  disableOnError(CCxRMVideo::EMSG_BADREPLY);
               done = TRUE;
            }
         }
      }
   }
   
   ::CloseHandle(hFile);
   
   // if successful, or if we must cancel because of a problem at this end, send RMV_CMD_PUTFILEDONE. We do NOT send
   // this if the file transfer failed because of an error return from a RMV_CMD_PUTFILECHUNK message.
   if(bOk || sendCancel)
   {
      m_commandBuf[0] = 2;
      m_commandBuf[1] = RMV_CMD_PUTFILEDONE;
      m_commandBuf[2] = sendCancel ? 0 : 1;
      BOOL bSent = sendRMVCommand();
      if(bSent) bSent = receiveRMVReply(10000); 
      // if command handshake fails, device error already set. Resync may be attempted.
      if(!bSent) return(FALSE);
      else bSent = (m_replyBuf[0] == 1) && (m_replyBuf[1] == RMV_SIG_CMDACK);
      if(bOk && !bSent)
      {
         bOk = FALSE;
         if(m_replyBuf[0] == 1 && m_replyBuf[1] == RMV_SIG_CMDERR)
            SetDeviceError("RMVideo file download failed: RMVideo probably could not read media file!");
         else
            disableOnError(CCxRMVideo::EMSG_BADREPLY);
      }
   }
   
   if(bOk) ClearDeviceError();
   return(bOk);
}

//=== sendRMVCommand ==================================================================================================
// Send the (already prepared) command buffer to the RMVideo server.
// 
// See RMVIDEO_COMMON.H for the entire set of commands recognized by RMVideo and the possible replies it may return. 
// Some RMVideo commands do not warrant a reply, and RMVideo may also send an error signal that is not in response to 
// any particular command (during an animation sequence, or if a catastrophic failure has occurred). Both RMVideo 
// commands and replies are formatted as a sequence of 1+ 32-bit integers, preceded by the command length, also a 32-bit 
// integer. However, selected commands and replies contain character strings. These always start on 4-byte boundaries 
// and are padded with nulls as necessary so that they end on 4-byte boundaries.
//
// Exception conditions:
// 1) Socket error occurs during send. In this case, the socket connection to RMVideo should be considered no longer
// viable. The CCxRMVideo device is disabled immediately. No further communications with RMVideo are possible until
// MaestroDRIVER is restarted, since a socket error usually indicates a hardware problem.
//
// 2) RTX TCP-IP send buffers are full (in our experience, this has happened only when RMVideo is in the animating
// state in Trials mode, with Maestro sending target updates on every display frame). Since our socket is non-blocking, 
// the socket send() function will fail with an error code of WSAEWOULDBLOCK. Rather than fail immediately, this method 
// will sleep ONCE for 500us in an effort to give RT-TCPIP (runs as a separate RTX process) a little time to empty the 
// buffer. If send() fails again because the buffers are still full, then:
//    (a) CCxRMVideo and RMVideo are considered to be "out of synch". The fact that the send buffers are full suggests
//        that some sort of network delay has occurred, whether on the Maestro end or the RMVideo end.
//    (b) CCxRMVideo is disabled temporarily, and a thread is spawned in an effort to restore normal operation. This
//        method then returns with an error condition set, and further calls to CCxRMVideo will fail until normal
//        operation is restored, if possible.
//    (c) See Resync() for an explanation of what the "re-synching" thread does.
//
// @return TRUE if successful; FALSE otherwise -- device error message set accordingly.
//
BOOL RTFCNDCL CCxRMVideo::sendRMVCommand()
{
   // recompute command length in bytes
   m_commandBuf[0] = m_commandBuf[0] * sizeof(int);

   // the entire buffer we're shipping out includes the command byte count preceding the command itself.
   int nBytesToSend = m_commandBuf[0] + sizeof(int);

   // if send() fails on EWOULDBLOCK, we'll sleep ONCE for 500us
   BOOL hasSlept = FALSE;
   
   // send it, perhaps in pieces, but without blocking!
   char* pByteBuf = (char *) &(m_commandBuf[0]);
   m_iCmdBytesSent = 0;
   while(nBytesToSend-m_iCmdBytesSent > 0)
   {
      int iRes = send(m_rmvSocket, &(pByteBuf[m_iCmdBytesSent]), nBytesToSend-m_iCmdBytesSent, 0);

      if(iRes == SOCKET_ERROR)
      {
         int iErr = WSAGetLastError();
         if(iErr == WSAEWOULDBLOCK)
         {
            if(hasSlept)
            {
               // network send delay. Send buffers likely full, which is bad condition. Disable CCxRMVideo.
               disableOnError(CCxRMVideo::EMSG_SENDDELAY);
               return(FALSE);
            }
            else
            {
               LARGE_INTEGER i64Sleep {};
               i64Sleep.QuadPart = (LONGLONG) 5000;
               ::RtSleepFt(&i64Sleep);
               hasSlept = TRUE;
            }
         }
         else
         {
            // network error. Disable CCxRMVideo -- something is probably wrong with hardware or the RT-TCP/IP stack...
            ::sprintf_s(m_errMsg, "%s, code=%d", CCxRMVideo::EMSG_SENDERROR, iErr);
            disableOnError(m_errMsg);
            return(FALSE);
         }
      }
      else
      {
         m_iCmdBytesSent += iRes;
      }
   }

   m_iCmdBytesSent = 0;
   ClearDeviceError();
   return( TRUE );
}

/**
 Receive a reply from the RMVideo server.

 This method checks to see if there's any data waiting to be read from RMVideo, optionally waiting if a nonzero 
 timeout is specified. Once data is waiting, the method expects to read in the entire reply without blocking (the 
 first 4 bytes is an integer specifying the number of 4-byte integers in the reply, not including the reply length).

 For most command/reply handshakes, a nonzero timeout is specified to give RMVideo time to do a task and send the 
 reply. The one exception to this is during animation: Maestro cannot wait for a reply and RMVideo will not normally
 send one. However, the first time a skipped frame or duplicate frame error occurs, RMVideo will send the appropriate
 signal to Maestro. In this situation, receiveRMVReply could indeed read a complete reply even though the timeout
 period is zero. 

 Exception conditions:
 1) Socket connection is closed on the RMVideo side (recv() returns 0). The CCxRMVideo device is disabled immediately.
 No further communications with RMVideo are possible until MaestroDRIVER is restarted.
 2) Socket error occurs during receive. In this case, the socket connection to RMVideo should be considered no longer
 viable. The CCxRMVideo device is disabled immediately. No further communications with RMVideo are possible until
 MaestroDRIVER is restarted, since a socket error usually indicates a hardware problem.
 3) There's nothing available in the RTX TCP-IP receive buffers when recv() is called. This will always happen since
 it takes a finite time for RMVideo to receive a command, do the work, and send a suitable reply. Response depends on
 whether a timeout is specified.
    (a) For most use cases, a nonzero timeout is specified, and a reply is EXPECTED. The function will poll recv() once
    per 500us, sleeping in between. Once the complete reply is received, the function returns. If the timeout is 
    exceeded before the reply is received, then we assume there's a serious problem and disable the CCxRMVideo device.
    (b) As mentioned above, no timeout is specified during an animation sequence. During this sequence, the command 
    RMV_CMD_UPDATEFRAME is sent once per display frame. Typically, RMVideo sends RMV_SIG_ANIMATE once per second and
    if a duplicate frame event is detected. It will also send RMV_SIG_CMDERR if an error has occurred during the 
    animation. Otherwise, it sends no message. In this situation, if no reply is available, the function returns 
    normally. If a complete reply can be read without blocking, the function does so and again returns normally. But if
    a partial reply is read in and then recv() fails because it would block, then the function STILL returns normally, 
    indicating that no reply was received. Internal variables keep track of the partial reply received thus far. On the
    next call to the method, if the rest of the reply is available, that reply is returned. Since this situation only 
    happens during an animation sequence, and since StopAnimation() will keep polling for replies until it gets the 
    expected reply, we don't think this will be an issue.

 @params timeOut Timeout period in ms. If positive, the method will fail if the entire reply has not been read within
 the specified period. Otherwise, the method returns immediately if no data is waiting to be read.
 @param bGotReply [out] This flag is set only if a reply was read successfully.
 @return TRUE if successful; FALSE otherwise -- device error message set accordingly. In the blocking case (nonzero
 timeout specified), a return value of TRUE means a reply was received. In the non-blocking case, one must check the
 'bGotReply' argument.
*/
BOOL RTFCNDCL CCxRMVideo::receiveRMVReply(int timeOut, BOOL& bGotReply)
{
   bGotReply = FALSE;
   double dTimeOutUS = timeOut * 1000;
   CElapsedTime elapsed;
   
   // for 500-us sleeps
   LARGE_INTEGER i64Sleep {};
   i64Sleep.QuadPart = (LONGLONG) 5000;

   // retrieve the next reply (if there is one). We COULD be in the middle of receiving a reply, which this code
   // can handle (although that should happen only during an animation sequence and should get cleared up by 
   // StopAnimation()). We get the first 4 bytes containing the reply length, then get the rest of the reply.
   char* pByteBuf = (char*) &(m_replyBuf[0]);
   int iBytesTotal = sizeof(int) * ((m_iReplyBytesRcvd >= sizeof(int)) ? (m_replyBuf[0] + 1) : 1);
   while(iBytesTotal-m_iReplyBytesRcvd > 0)
   {
      int iRes = recv(m_rmvSocket, &(pByteBuf[m_iReplyBytesRcvd]), iBytesTotal-m_iReplyBytesRcvd, 0);
      if(iRes == 0)
      {
         // RMVideo has unexpectedly closed the connection; fail
         disableOnError(CCxRMVideo::EMSG_LOSTCONN);
         return(FALSE);
      }
      else if(iRes == SOCKET_ERROR)
      {
         int iError = WSAGetLastError();
         if(iError == WSAEWOULDBLOCK || iError == WSAEINPROGRESS)
         {
            // recv() call would block. An error occurs only if a nonzero timeout is specified and we've exceeded the
            // timeout period.
            if(timeOut <= 0)
            {
               ClearDeviceError();
               return(TRUE);
            }
            else if(elapsed.Get() < dTimeOutUS)
            {
               ::RtSleepFt(&i64Sleep);
               continue;
            }
            else
            {
               // timed out waiting on reply. Disable CCxRMVideo.
               disableOnError(CCxRMVideo::EMSG_TIMEOUT);
               return(FALSE);
            }
         }
         else
         {
            // a real socket error has occurred!
            disableOnError(CCxRMVideo::EMSG_RECVERROR);
            return(FALSE);
         }
      }
      else
      {
         // Once we get the first 4 bytes, read the reply length and calculate how much more we need to retrieve
         m_iReplyBytesRcvd += iRes;
         if(iBytesTotal == sizeof(int) && m_iReplyBytesRcvd == iBytesTotal)
         {
            if(m_replyBuf[0] <= 0)
            {
               disableOnError(CCxRMVideo::EMSG_INVALIDREPLY);
               return(FALSE);
            }
            iBytesTotal = sizeof(int) * (m_replyBuf[0] + 1);
         }
      }
   }

   // we got a complete reply, so reset #bytes rcvd so we're ready for the next one.
   bGotReply = TRUE;
   m_iReplyBytesRcvd = 0;

   ClearDeviceError();
   return(TRUE);
}

/**
 Set the device error message and mark RMVideo as permanently disabled. This should be called whenever a serious socket
 communications error renders the Maestro-RMVideo link unusable. It should also be called if unable to restore normal
 communications after a network delay. Once called, all future public calls into the CCxRMVideo interface will fail
 without changing the error message set here. While the device remains "on" in the sense of CDevice::IsOn(), it is
 unusable. Method has no effect if the device is not "on".
 
 @param strErr A description of the fatal error that requires disabling the RMVideo "device".
*/
VOID RTFCNDCL CCxRMVideo::disableOnError(LPCTSTR strErr)
{
   if(IsOn() && !m_bDisabled)
   {
      m_bDisabled = TRUE;
      SetDeviceError(strErr);
   }
}
