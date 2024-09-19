//=====================================================================================================================
//
// rmvideo_common.h : Constants and structure definitions shared by Remote Maestro Video and its Maestro counterpart.
//
// REVISION HISTORY:
// 22feb2006-- Target types RMV_SINEGRAT and RMV_SQUAREGRAT replaced by RMV_GRATING, with waveform type determined by
//             flag bit RMV_F_ISSQUARE.  Similarly, RMV_SINEPLAID and RMV_SQUAREPLAID replaced by RMV_PLAID.  Also
//             added field fSigma to RMVTGTDEF, although Gaussian window is not yet implemented.
// 07mar2006-- Increased precision with which floating-pt fields in RMVTGTVEC are encoded from 3 to 6.  We were
//             observing significant errors in net position change during a constant-velocity motion.  Floating-pt
//             params in RMVTGTDEF are still scaled by 1000.
// 29mar2006-- Added comments re: change in how RMVideo works during an animation sequence.  A "frame drop" signal
//             (either RMV_SIG_DUPFRAME or RMV_SIG_SKIPFRAME) is sent only once during the animation, instead of every
//             time a frame drop is detected.  This change is intended to support how Maestro uses RMVideo in Contiuous
//             mode -- sending RMV_CMD_UPDATEFRAME only sporadically (when the user manually toggles the target on/off
//             or changes its position).  Before this change, when Maestro started an RMVideo animation in Cont mode,
//             RMVideo quickly filled up its socket send buffer with lots of RMV_SIG_DUPFRAME signals!
// 30mar2006-- Added comment re: change in how RMVideo works during an animation sequence.  RMVideo will stop
//             animating and return to idle if a given display frame actually takes more than 50 monitor frame periods
//             to render.
// 04apr2006-- Added comments re: application of Gaussian window to RMV_SPOT and _RANDOMDOTS target types in addition
//             to RMV_GRATING and _PLAID.
//          -- Added support for elliptical and 1-D Gaussian windows: Field RMVTGTDEF.fSigma is now a 2-element array,
//             where the first element is the standard deviation in x, and the second is the std dev in y.  If both
//             are zero, Gaussian windowing is disabled.
// 06apr2006-- RMVTGTDEF.iRGBMean and .iRGBCon are now 2-element arrays -- to support separate color specs for the
//             two grating components of the RMV_PLAID target.
//          -- Added comments re: change in how RMVTGTVEC is used to update pattern motion of grating/plaid targets.
//             For gratings, RMVTGTVEC.hPat is the position change of the grating ALONG THE DRIFT AXIS, and vPat is
//             ignored.  For plaids, hPat is the position change of grating#1 along its drift axis, and vPat is the
//             position change of grating#2 along its drift axis.
//          -- Added flag RMV_F_INDEPGRATS.  This flag applies only to RMV_PLAID.  If set, the two gratings move
//             independently, with RMVTGTVEC.hPat and .vPat holding the position changes of grating #1 and #2, resp.,
//             along their respective drift axis directions.  This essentially replicates the old FB target types
//             TWOSINGRATS and TWOSQGRATS!  If the flag is not set, then (hPat,vPat) describes the net motion of the
//             plaid as a single pattern.
// 24apr2006-- Added command RMV_CMD_GETCOLORRES, by which RMVideo returns its color resolution: 16- or 24-bit.
// 04dec2006-- Constant RMV_MAXNOISESPEED incr'd to 3000, permitting speed noise up to 300% of nominal speed in the 
//             RMV_RANDOMDOTS target class.
// 31aug2007-- Mods to support a second algorithm for dot speed noise generation in the RMV_RANDOMDOTS target class. 
//             RMV_MAXNOISESPEED changed to 300. This is to bring RMVideo in line with a recent (v2.1.2) change in 
//             the XYScope NOISYSPEED target, in which the additive speed noise offset range granularity was changed 
//             to 1% instead of 0.1%.
// 13jul2009-- Added target type RMV_MOVIE. Experimental for now. The only applicable parameter in RMVTGTDEF is the 
//             target type. Eventually, other params will be added. The only parameter in RMVTGTVEC that applies is
//             the target on/off flag.
// 22jul2009-- Begun revisions to support reporting available video modes with a minimum horizontal resolution of 1024
//             pixels and a minimum refresh of 75Hz. Requires the XRandR extension. If that extension is not available
//             or not supported on the machine, RMVideo will still run but it won't be possible to change the display
//             resolution. Also, 24-bit color is now required. Added commands RMV_CMD_GETALLVIDEOMODES,
//             RMV_SETCURRVIDEOMODE. Deleted command RMV_CMD_GETCOLORRES. Also, the STARTANIMATE and UPDATEANIMATE
//             command descriptions have been revised to indicate that motion vectors are always supplied for all 
//             loaded targets on every frame (Maestro has been doing this anyway, and it simplifies code on the
//             RMVideo side).
// 23jul2009-- Begun revisions to support downloading movie files from Maestro to RMVideo, and uploading information 
//             about all stored movie files from RMVideo to Maestro -- all in support of the new "movie" target class.
// 18aug2009-- Added commands to support changing the monitor gamma from Maestro: RMV_CMD_GET/SETGAMMA.
// 24aug2009-- Added commands to support restart of RMVideo and reporting the program version.
// 25aug2009-- Deleted commands RMV_CMD_GETFRAMEPER and RMV_CMD_GETDISPRES. Instead, Maestro will retrieve all video
//             modes via RMV_CMD_GETALLVIDEOMODES, change the video mode with RMV_CMD_SETCURRVIDEOMODE, and retrieve
//             the current video mode (including measured frame period) with RMV_CMD_GETCURRVIDEOMODE.
// 11sep2009-- Added flag RMV_F_ORIENTADJ. It applies only to a single-grating target. 
// 04dec2009-- The RMV_F_ORIENTADJ flag now applies to plaid as well as grating targets. Version = 2.
// 20jan2010-- Added flag RMV_F_WRTSCREEN, which applies only to RMV_RANDOMDOTS target type. If set, pattern motion is
//             relative to the screen rather than the target window. It was introduced so that RMV_RANDOMDOTS can more
//             precisely emulate the behavior of XYScope NOISYDIR and NOISYSPEED targets when the target window is
//             moving. Version = 3.
// 24mar2010-- Relaxed restriction on max # of dots in RMV_RANDOMDOTS from 1000 to 9999. 
//          -- Corrected description of RMVTGTDEF.fOuterW, fInnerW for RMV_FLOWFIELD. Maestro sets these to radii, NOT
//             diameters! RMV_FLOWFIELD implementation likewise corrected.
//          -- Version = 4.
// 24nov2014-- Modified defn of RMV_RANDOMDOTS to include RMVTGTDEF.iRGBCon[0]. If contrast C != 0, then that color
//             component takes on two values, L1= M(1+C) and L2=M(1-C), where M=RMVTGTDEF.iRGBMean[0] and C=[0..1].
//             When any color component's contrast is non-zero, then the dot patch is a two-color contrast patch: half
//             the dots are drawn in one color, the rest in the other color. 
//          -- Version = 6.
// 23may2016-- As of Maestro v3.2.1, the auto-update feature has been removed. Maestro can no longer update an older
//             version of RMVideo over their private Ethernet link. The RMV_CMD_PUT*** commands are now only used to 
//             download movie files to RMVideo. Comments updated accordingly. Version unchanged.
// 04oct2016-- Began modifications to implement a new RMVideo target that displays a static image loaded from a JPEG,
//             PNG, PSD, BMP or GIF file that has been downloaded to RMVideo's "media store", which replaces the prior
//             notion of a "movie store". Some constants added. As with the RMV_MOVIE target, the definition for the new
//             RMV_IMAGE target includes a media folder name and file name. As of Maestro 3.3.1.
//          -- RMVideo version = 7.
// 24sep2018-- Mods to implement new feature: "vertical sync spot flash". A white square spot may be optionally 
// flashed in the TL corner of screen starting during any frame of an animation sequence. The size of the square, the
// width of a dark margin below and right of it, and the duration of the flash are sent to RMVideo only in the idle
// state via the new RMV_CMD_SETSYNC command. A new integer flag accompanying the STARTANIMATE and UPDATEFRAME commands
// indicates whether or not flash is presented.
//          -- As of Maestro v4.0.0 and RMVideo v8.
// 25sep2018-- Eliminated the "margin" parameter for the spot flash feature.
//
// March 2019:
// -- Changes in the Maestro-RMVideo communication protocol while in animate mode. Replaced the FIRSTFRAME, 
// SKIPFRAME, and DUPFRAME signals with a single all-purpose RMV_SIG_ANIMATEMSG message. It is sent under 4 distinct
// circumstances. See details below. Also removed the RMV_MAX_FRAMELAG constant; for all practical purposes, we require
// that RMVideo be able to render any video frame in less than one refresh period. 
// -- Modified response to GET/SETCURRVIDEOMODE to return the measured frame period as an integer in nanosecs instead
// of microsecs. Recent testing has shown that the truncation error when converting to microsecs is significant.
// -- As of Maestro v4.0.5 and RMVideo v9.
//
// 06may2019: As of Maestro v4.1.0 and RMVideo v10.
// -- Updating RMVideo to the OGL3.3-compatible implementation. This implementation was developed and evaluated over
// a five-month period from Dec 2018 - May 2019.
// -- Introducing new "flicker" feature applicable to all RMVideo target types. Modified RMVTGTDEF to include new 
// parameters governing flicker, and preserved the deprecated structure in RMVTGTDEF_V22 to parse data files that
// contain RMVTGTDEF_V22 records (data file versions 13-22).
// 
// Aug 2019: As of Maestro v4.1.1 and RMVideo v10 (release "b").
// -- To address long download times and long load-from-disk times for large images (eg, 2560x1440), CRMVMediaMgr now
// implements an in-memory image cache that can grow to ~300MB, allocated on the heap. At startup, the image cache is
// preloaded with any images currently in the media store (until the capacity is reached). During runtime, the cache is
// managed as needed, removing oldest images if the cache capacity is reaches, deleting images that have been removed
// frome the media store, and adding images as they are downloaded from RMVideo.
// -- In comments, changed "max wait times" for selected RMV_CMD_* commands. Commensurate changes made in Maestro 
// V4.1.1.
// -- Changes don't affect Maestro-RMVideo coordination, so official RMVideo version is unchanged at V=10.
//=====================================================================================================================


#if !defined(RMVIDEO_COMMON_H_INCLUDED_)
#define RMVIDEO_COMMON_H_INCLUDED_


//=====================================================================================================================
// Maestro-RMVideo TCPIP Network Connection.  The Maestro and RMVideo hosts communicate over a private Ethernet
// connection using secondary NICs installed in the two machines.  Standard TCP/IP protocol is used, with BSD sockets-
// like programming.  On the Maestro side, the NIC is an RTX-supported device and we use the RTX TCP/IP protocol stack
// to ensure real-time communications at that end.  On the RMVideo side, we're relying on standard NIC drivers and
// hoping things work!Dyn
//
// This section lists shared constants in support of this dedicated network connection.
//=====================================================================================================================

#define RMVNET_RMVADDR        "10.1.1.1"     // network address assigned to NIC on RMVideo side
#define RMVNET_RMVPORT        42356          // port on which RMVideo listens for connection from Maestro
#define RMVNET_MAESTROADDR    "10.1.1.2"     // network address assigned to NIC on Maestro side
#define RMVNET_CMDCNTSZ       4              // # of bytes in the command byte count that precedes each Maestro cmd


//=====================================================================================================================
// Maestro commands to RMVideo.  Each command is a 32-bit ID followed by zero or more 32-bit ints of "command data".
// With the exception of RMV_CMD_UPDATEFRAME, Maestro expects a short reply from RMVideo.  If RMVideo fails to reply
// within a brief time (varies with the command), Maestro assumes it is no longer responding and severs the connection.
// See detailed description for each command.
//=====================================================================================================================

#define RMV_CMD_NONE          0
// This is a pseudo-command ID returned by the CRMVIo interface to tell RMVideo there's no Maestro command pending.
// It is never sent by Maestro.  All valid command IDs must be greater than this value.  In addition, if the command
// ID returned is LESS than this value, then the RMVideo-Maestro IO connection has experienced a fatal error.  In this
// case, RMVideo returns to the "off" state and waits for a new session to begin.  In doing so, it may die if the
// communication interface has failed altogether.
// DATA: N/A.
// REPLY: N/A.

#define RMV_CMD_GETVERSION    1
// Retrieve the RMVideo application version number. This was introduced in Aug 2009 with an initial version = 1. Older
// versions of RMVideo will not recognize the command and return RMV_SIG_CMDERR.
// DATA:  None.
// REPLY: Single 32-bit positive integer, the RMVideo version number. Max wait = 250 ms.

#define RMV_CURRENTVERSION    10           // current RMVideo version number (as of Aug 2019, Maestro v4.1.1)

#define RMV_CMD_RESTART       2
// Exit and restart. This command was issued as part of the procedure to automatically update an old version of RMVideo
// by downloading the updated executable over the Ethernet link. THIS AUTO-UPDATE FEATURE IS NO LONGER AVAILABLE.
// DATA:  None.
// REPLY:  RMV_SIG_BYE to acknowledge request and indicate that RMVideo is shutting down. After releasing all resources,
// the RMVideo process will issue an exec() command to restart. Maestro should close its Ethernet connection and wait a
// minimum of 10 seconds before attempting to reconnect to the new instance of RMVideo. If it fails to do so, it will be
// necessary to perform a manual restart.

#define RMV_CMD_EXIT          5
// This command tells RMVideo to close the fullscreen window and quit.  Maestro will never issue this command, since
// RMVideo is intended to run "forever".  It is provided for testing purposes.
// DATA:  None.
// REPLY:  None.

#define RMV_CMD_STARTINGUP    10
// Maestro is starting up. Stop sleeping, display fullscreen window with black bkg, and enter idle state.
// DATA:  None.
// REPLY:  RMV_SIG_IDLE sent as soon as RMVideo has entered the idle state.  Max wait = 10 seconds.

#define RMV_CMD_SHUTTINGDN    15
// Maestro is shutting down.  Return to idle, hide fullscreen window, then enter sleep state, waiting for the next time
// that Maestro starts up WITHOUT being a CPU hog.  NOTE that RMVideo is designed to run continuously; users should not 
// have to restart it when Maestro restarts.
// DATA:  None.
// REPLY:  RMV_SIG_BYE sent to acknowledge the end of the command session.  Max wait = 10 seconds.

#define RMV_CMD_SETBKGCOLOR   20
// Set background RGB color. Invoke only when RMVideo is in the idle state.
// DATA:  Single 32-bit integer.  Byte 0 (LSB) is red luminance, byte 1 is green, and byte 2 is blue.  Byte 3 unused.
// On the RMVideo side, each color byte is divided by 255 to yield a floating-point value in [0..1].
// REPLY:  RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR otherwise.  Max wait = 250ms.

#define RMV_CMD_SETGEOMETRY   30
// Set display geometry. Invoke only when RMVideo is in the idle state.
// DATA:  W, H, D. RMVideo monitor's fullscreen extents, and LOS distance to subject's eye -- all in millimeters.
// REPLY:  RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR otherwise.  Max wait = 250ms.

#define RMV_CMD_GETALLVIDEOMODES   40
// Get RMVideo monitor's video modes. Returns only those modes that meet or exceed 1024x768 @ 75Hz. If RMVideo does
// not support mode switching, only the current video mode is returned. Invoke only when RMVideo is in idle state.
// DATA:  None.
// REPLY: If successful, returns RMV_SIG_CMDACK followed by (1 + N*3) 32-bit integers: {N, W, H, R, ...}. First integer 
// N is the number of alternate video modes available. Max allowed value is RMV_MAXVMODES. Each mode is described by an 
// integer triplet (W, H, R), where W is the screen width in pixels, H is the screen height in pixels, and R is the 
// refresh rate rounded to the nearest Hz. If mode switching is not supported, N will be 1 and (W,H,R) will be the 
// nominal screen size and refresh rate. On error, returns a single integer, RMV_SIG_CMDERR. Max wait = 1 second.

#define RMV_MAXVMODES 30            // maximum number of alternative RMVideo display modes

#define RMV_CMD_GETCURRVIDEOMODE   41
// Get RMVideo monitor's current video mode, including its frame period as measured over a 500-frame epoch. Invoke only 
// when RMVideo is in idle state.
// DATA:  None.
// REPLY: If successful, returns RMV_SIG_CMDACK followed by two 32-bit integers {N, FP}, where: N is the current video
// mode index (between 1 and the total # of available modes) and FP is the current frame period in nanoseconds as 
// measured over a 500-frame epoch at startup or the last time the video mode was changed. If an error occurred, 
// returns a single integer, RMV_SIG_CMDERR. Max wait = 250ms.

#define RMV_CMD_SETCURRVIDEOMODE   42
// Change the RMVideo monitor's current video mode. A successful switch will trigger a frame period measurement, which
// takes 500 frames, or 500/75Hz = 6.7 seconds at worst. Invoke only when RMVideo is in the idle state, and be sure to
// wait an adequate amount of time for a response.
// DATA:  Single 32-bit integer between 1 and N, where N is the number of available video modes as reported by the
// command RMV_CMD_GETALLVIDEOMODES.
// REPLY:  If successful, returns RMV_SIG_CMDACK followed by a 32-bit integer specifying the just-measured frame period 
// in nanoseconds. If unsuccessful (eg, mode switching is not possible), returns RMV_SIG_CMDERR. Max wait = 10 secs.

#define RMV_CMD_GETGAMMA   43
// Get RMVideo monitor's current display gamma.
// DATA:  None.
// REPLY:  RMV_SIG_CMDACK followed by three 32-bit integers [Rg Gg Bg], the monitor gamma correction factors for 
// the red, green and blue guns, respectively. Each is multiplied by 1000. Returns RMV_SIG_CMDERR if an error 
// occurred.  Max wait = 250ms.

#define RMV_CMD_SETGAMMA   44
// Get RMVideo monitor's current display gamma.
// DATA:  Three integers [Rg Gg Bg] holding the desired monitor gamma correction factors for the red,
// green and blue guns, respectively, scaled by 1000. All three must lie in the range [800 .. 3000].
// REPLY:  RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR if one or more factors was out of range or if an error
// occurred.  Max wait = 250ms.

#define RMV_MINGAMMA 800       // 1000*min for a gamma correction factor 
#define RMV_MAXGAMMA 3000      // 1000*max for a gamma correction factor

#define RMV_CMD_SETSYNC   45
#define RMV_MINSYNCSZ 0
#define RMV_MAXSYNCSZ 50
#define RMV_MINSYNCDUR 1
#define RMV_MAXSYNCDUR 9
// Set parameters governing the vertical sync spot flash that may optionally be presented during any frame in an
// animation sequence. The square white spot is flashed in the top-left corner of screen for the specified duration.
// At all other times, the spot is black -- regardless the current background color. A flag accompanying the 
// RMV_CMD_STARTANIMATE and RMV_CMD_UPDATEFRAME commands indicates whether or not the flash should be presented. 
//
// DATA: Two integers, in order: the size of the square spot in millimeters; and the flash duration, in # of video 
// frames. Spot size is restricted to [0..50]; a spot size of 0 disables the feature. The duration range is [1..9].
// REPLY: RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR if any parameter is out of range or an error occurred. Max
// wait = 250ms.

#define RMV_CMD_LOADTARGETS   60
// Load definitions of targets to be animated.  This command can be invoked only when RMVideo is in the idle state.
// DATA:  The first integer after the command ID is the number N of targets to be loaded.  This is followed by N
// "target records", each defining a target that will participate in an animation.  See description below.  Targets are
// identified during animation by their order of definition in this command.
// REPLY:  RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR otherwise.  Max wait = 10 seconds.
//
// Format of each target record:  A series of (paramID, value)-pairs, where paramID identifies a member of the
// RMVTGTDEF structure (see below).  Value may be one or more integers, depending on whether the structure member is
// an atomic type or an array.  Note that some members of RMVTGTDEF are floating-point; for transmission in the
// RMV_CMD_LOADTARGETS command, they are scaled by 1000 and rounded to the nearest integer. In addition, two of the
// parameters are ASCII character strings. In this case, the integer paramID is followed by 32 bytes holding the 
// character string padded with nulls.
//
// This scheme lets Maestro transmit ONLY the relevant target parameters. If a paramID does not appear in a target 
// record, then it can be assumed the parameter is not relevant to the target type being defined.
//
// Below are the parameter IDs for all members currently defined in RMVTGTDEF.  The target record is terminated by a
// final "endOfTarget" ID, which has no value paired with it.
#define RMV_MAXTARGETS        100  // an upper limit on the # of targets loaded (current max in Maestro is 25)
#define RMV_TGTDEF_TYPE       1000
#define RMV_TGTDEF_APERTURE   1001
#define RMV_TGTDEF_FLAGS      1002
#define RMV_TGTDEF_RGBMEAN    1003 // followed by 2 integers (the second is ignored unless tgt type is RMV_PLAID)
#define RMV_TGTDEF_RGBCON     1004 // followed by 2 integers (the second is ignored unless tgt type is RMV_PLAID)
#define RMV_TGTDEF_OUTERW     1005
#define RMV_TGTDEF_OUTERH     1006
#define RMV_TGTDEF_INNERW     1007
#define RMV_TGTDEF_INNERH     1008
#define RMV_TGTDEF_NDOTS      1009
#define RMV_TGTDEF_NDOTSIZE   1010
#define RMV_TGTDEF_SEED       1011
#define RMV_TGTDEF_PCTCOHER   1012
#define RMV_TGTDEF_NOISEUPD   1013
#define RMV_TGTDEF_NOISELIM   1014
#define RMV_TGTDEF_DOTLIFE    1015
#define RMV_TGTDEF_SPATIALF   1016  // followed by 2 integers (one per grating)
#define RMV_TGTDEF_DRIFTAXIS  1017  // followed by 2 integers (one per grating)
#define RMV_TGTDEF_GRATPHASE  1018  // followed by 2 integers (one per grating)
#define RMV_TGTDEF_SIGMA      1019  // followed by 2 integers (sigma_X, sigma_Y)
#define RMV_TGTDEF_FOLDER     1020  // followed by the folder name string padded with nulls out to 32 bytes
#define RMV_TGTDEF_FILE       1021  // followed by the file name string padded with nulls out to 32 bytes
#define RMV_TGTDEF_FLICKER    1022  // follwed by 3 integers (ON dur, OFF dur, initial delay)
#define RMV_TGTDEF_END        1099  // end of target definition; NOT paired with a dummy value!

#define RMV_TGTDEF_F2I_F      1000.0f  // divide by this to recover floating-pt value of an int-coded target param

#define RMV_CMD_STARTANIMATE  70
#define RMV_TGTVEC_LEN        6     // # of integers in a target motion vector record

// Begin a target animation sequence. Before a sequence begins, all targets defined by the preceding "load targets"
// command set are assumed to be turned OFF and positioned at the origin (center of screen). This command's payload
// includes the target motion update records for the first two frames of the animation sequence. RMVideo updates all 
// targets IAW the motion update records, renders the first frame on the back buffer, and swaps the front and back
// buffers during the next vertical blanking interval (VSync enabled). It then begins work on the second frame.
//
// As soon as the first display frame begins, RMVideo will send the RMV_SIG_ANIMATEMSG signal back to Maestro (with
// no payload) Maestro busy-waits for this signal, then starts acquiring data. The idea here is to *roughly* sync 
// Maestro's data acquisition timeline to the start of the first display frame.
//
// If an error occurs while rendering the first video frame, RMVideo returns RMV_SIG_CMDERR, followed by RMV_SIG_IDLE
// to indicate that it has returned to the idle state. One possible error is that it just takes too long to render
// the first frame.
//
// NOTE: In the description of DATA below, the vector counts N aren't really necessary, nor are the target IDs in each
// 6-tuple. These are artifacts of a previous, more flexible definition of the command, in which some vectors
// could be omitted if there is no change from the previous display frame. Maestro has always sent motion vectors for
// all targets on every frame, and we decided to get rid of this flexibility (without changing the command format) to
// simplify coding on the RMVideo side.
//
// DATA:  SYNC?, N, {V(0) ... V(N-1)}, N, {V(0) ... V(N-1)}. Per-frame target motion vectors are supplied for
// the first and second frames of the animation sequence. Integer N is the number of targets being animated, ie, which
// must be the same as the number of target definitions that are loaded. Each motion vector V(n) is a set of 6 integers. 
// The first integer in the 6-tuple is the target ID, which is simply that target's ordered position in the target list 
// (as defined in the preceding "load targets" command). The remaining 5 integers correspond to the members of the
// RMVTGTVEC structure, in the order in which the members appear in the structure definition. Most of these members are
// floating-pt; they are scaled by 1.0e^6 and rounded to the nearest integer for transmission to RMVideo. 
// 
// The first integer datum is a flag indicating whether or not the "vertical sync spot flash" should be presented,
// starting on the first frame, IAW the spot flash settings last sent via the RMV_CMD_SETSYNC command. A nonzero value
// triggers the flash, unless the feature is disabled (zero spot size or zero flash duration. The spot flash is always
// rendered on top of any animated targets; it is intended to drive a photodiode circuit which in turn delivers a TTL
// pulse upon detecting the flash. That TTL pulse can be timestamped by Maestro to better synchronize the RMVideo and
// Maestro timelines.
//
// REPLY:  RMV_SIG_CMDERR if an error occurred, else RMV_SIG_ANIMATEMSG (no payload) should be sent at the BEGINNING of
// the first animation frame.  Max wait ~ 500ms.

#define RMV_CMD_UPDATEFRAME   80
// Update target motion for the next display frame.
//
// Once an animation begins, RMVideo leaves the "idle" state and enters a time-critical "animating" state.  By design,
// RMVideo "works ahead" by one frame during animation of video targets. At the start of animation, it prepares and
// displays the first frame, then begins work on the second frame. It expects Maestro to send target motion update
// information for display frame N so that it is ready to be processed at the start of frame N-1. THIS TIMING IS 
// CRUCIAL; if RMVideo does not receive the target update for frame N by the start of frame N-1, it will assume there 
// was no change for frame N -- so frame N will be a duplicate of frame N-1. In this scenario, RMVideo sends an 
// RMV_SIG_ANIMATEMSG message with the payload [N,0], where N = # elapsed video frames and 0 indicates that RMVideo
// detected a missed target update.
//
// RMVideo can only do so much during a single display frame. If the vertical retrace occurs before RMVideo is done
// rendering the next frame N in the backbuffer, then frame N will be a duplicate of frame N-1. If the rendering delay
// lasts several refresh periods, then there will be several duplicate frames in a row. Upon detecting this scenario,
// RMVideo issues RMV_SIG_ANIMATEMSG with the payload [N,X], where N is the elapsed frame count and X>=1 is the number
// of contiguous duplicate frames resulting from the rendering delay.
//
// Whenever a duplicate frame occurs, RMVideo will remain in the animate state. It is up to Maestro to decide whether
// or not to terminate the animation on a duplicate frame event.
//
// As an added check to ensure that Maestro is not getting ahead of the RMVideo animation, RMVideo will send an 
// RMV_SIG_ANIMATEMSG once per second. In this usage, the payload is [N], where N is the # of elapsed video frames in 
// the animation thus far. Maestro can check N against its own account of the # of elapsed frames to determine whether
// it is getting ahead of RMVideo's timeline (in which case UPDATEFRAME commands are building up in the network 
// receive buffer on the RMVideo side).
//
// RMVideo could also send the RMV_SIG_CMDERROR signal if it encounters a problem processing an UPDATEFRAME command.
// It will remain in animate mode, but Maestro must terminate the animation sequence if such an error occurs.
//
// DATA:  SYNC?, N, {V(0) ... V(N-1)} -- Target motion update vectors for the next display frame.  See description of the
// arguments to the "start animation" command. The first integer datum is a flag indicating whether or not the "vertical
// sync spot flash" should be triggered during this video frame. However, the spot flash will not be presented if the
// feature is disabled (zero spot size or zero flash duration), or if a previously triggered flash is already in progress
// (the flash duration can be up to 9 video frames).
// 
// REPLY:  NONE. However, messages may be sent back to Maestro during an animation sequence, as already described. 
// Maestro will check for any pending RMVideo message after sending each RMV_CMD_UPDATEFRAME command.

#define RMV_CMD_STOPANIMATE   90
// Stop animation immediately and return to idle state.  All previously defined targets are "unloaded".
// DATA:  None.
// REPLY:  RMV_SIG_IDLE sent as soon as RMVideo has returned to the "idle" state.  Max wait = 1 second.


//=====================================================================================================================
// RMVideo Media Store (formerly called the "Movie Store")
// RMVideo stores videos on the host system for playing back with the RMV_MOVIE target class, as well as images that may
// be displayed with the RMV_IMAGE target. It keeps all such media files in folders within the "media" folder in its
// installation directory. Thus, each media file is "identified" by its filename and the name of its parent folder. No
// media files may be stored directly in the "media" folder itself (well, you can put them there manually, but RMVideo
// will ignore them).
//
// At startup, RMVideo scans the immediate subdirectories of the "media" directory and prepares an internal "table of
// contents" so that it can respond quickly to queries about the media store's content. This scan can take an indefinite
// period of time, depending on the total number of folders and files in the media store.
//
// Media files can be downloaded from the Maestro client via commands described below. They can also be stored directly
// (eg, copied from CD/DVD media) in the RMVideo media directory, and RMVideo will detect the new files upon restarting.
//
// To keep things simple, media folder and file names are restricted in length and character content -- only ASCII
// alphanumeric characters, the period ('.'), and the underscore ('_'). In addition, there is a limit to the number of
// folders in the store and the number of files per folder (to put a bound on the replies to queries about media store
// content).
//
// Of course, all of the commands listed in this section may be sent ONLY when RMVideo is in the idle state.
//=====================================================================================================================

#define RMV_MVF_LIMIT   50    // max # media files per folder, and max # folders in media store
#define RMV_MVF_LEN     30    // max length of a media file or folder name
#define RMV_MVF_CHARS   "._ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"


#define RMV_CMD_GETMEDIADIRS  100
// Get the names of all folders in the RMVideo media store.
// DATA:  None.
// REPLY: [RMV_SIG_CMDACK, N, folderNames]. N = number of media folders (32-bit int). folderNames = Null-separated list
// of folder names, always ending with a terminal null and padded with nulls as needed to ensure string length in 
// bytes is a multiple of 4. If an error occurs, only RMV_SIG_CMDERR is returned. Max wait = 1 second.

#define RMV_CMD_GETMEDIAFILES 101
// Get the names of all files within a specified folder in the RMVideo media store.
// DATA:  The name of a folder in the media store, ending with a terminal null and padded with nulls as needed to
// ensure that the total payload length in bytes is a multiple of 4.
// REPLY: [RMV_SIG_CMDACK, N, fileNames]. N = number of media files (32-bit int). fileNames = Null-separated list
// of file names, always ending with a terminal null and padded with nulls as needed to ensure string length in 
// bytes is a multiple of 4. If an error occurs, only RMV_SIG_CMDERR is returned. Max wait = 1 second.

#define RMV_CMD_GETMEDIAINFO  102
// Get information about a particular media file in the RMVideo media store.
// DATA:  The media's folder and file names, separated by a single null character. Payload ends with a null and is
// padded with nulls as needed to ensure the total payload length in bytes is a multiple of 4.
// REPLY: Five 32-bit integers, [RMV_SIG_CMDACK, W, H, R, D], where:  W = width of the image or movie frame in pixels,
// H = the image or movie frame height in pixels, R = the movie's ideal frame rate in milli-Hz (not applicable to an
// image), and D = approximate duration of movie in milliseconds (not applicable to an image). If any value is unknown,
// it will be set to 0. For an image file, both R and D are set to -1. If an error occurs or the specified file was not
// found in the store, only RMV_SIG_CMDERR is returned. Max wait = 1 second.

#define RMV_CMD_DELETEMEDIA   103
// Permanently remove a particular media file or the contents of an entire media folder from the RMVideo media store. If
// a file is removed and its the last media file in its parent folder, then that folder is removed as well.
// DATA:  To remove a media folder, specify the folder's name only. To remove a single media file, specify the parent
// folder's name followed by the media file's name, separated by a single null character. In either case, the payload
// must end with a null and is padded with nulls as needed to ensure the payload length in bytes is a multiple of 4.
// REPLY: RMV_SIG_CMDACK if successful, RMV_SIG_CMDERR otherwise. Possible reasons for error: file not found, or 
// unable to delete file. Max wait = 5 seconds.

#define RMV_CMD_PUTFILE      110
// Initiate the download of a media file from the Maestro client to a folder in the RMVideo media store. In response,
// RMVideo opens the new file in the destination specified. It then enters a special state in which it accepts a
// sequence of RMV_CMD_PUTFILECHUNK packets by which the file's content is transferred, ending with the command
// RMV_CMD_PUTFILEDONE. If any other command is received while in this state, that command fails, the file download
// fails, and RMVideo returns to the idle state.
// DATA: {folderName \0 folderName}. The payload includes the destination media folder name followed by the destination
// file name, separated by a single null character. The payload ends with a null and is padded with nulls as needed to
// ensure it is a multiple of 4 bytes in length. The media file must not already exist on the RMVideo host file system!
// If the specified media folder does not exist, a new folder with that name is created.
// REPLY: RMV_SIG_CMDACK if RMVideo is ready to receive file, RMV_SIG_CMDERR otherwise. Max wait = 2 secs.

#define RMV_CMD_PUTFILECHUNK  111
// Transfer a 2KB chunk of file as part of a download initiated by RMV_CMD_PUTFILE.
// DATA:  Number of file bytes N in payload (a 32-bit int), followed by those N bytes, followed by padding to ensure 
// the payload length in bytes in a multiple of 4.
// REPLY: RMV_SIG_CMDACK if RMVideo processed chunk and is ready for the next, RMV_SIG_CMDERR if an error occurs. In the
// latter case, the file download has failed and RMVideo returns to the idle state. Max wait = 2 secs.

#define RMV_CMD_PUTFILEDONE   112
// Terminate the download of a file initiated by RMV_CMD_PUTFILE.
// DATA: A single 32-bit integer. If nonzero, then file transfer is complete and file should be saved. Otherwise, the
// file transfer operation should be cancelled and the partially downloaded file should be removed.
// REPLY: RMV_SIG_CMDACK if file operation was completed successfully or if it was cancelled, RMV_SIG_CMDERR if it was
// not cancelled and did not complete successfully. This could happen if the downloaded file could not be opened or was
// not recognized as a supported video or image file. Max wait = 10 secs.


//=====================================================================================================================
// RMVideo messages sent to Maestro. Most messages are just a "signal code", a single 32-bit word. In some cases the 
// signal is accompanied by a payload; eg, in responses to Maestro command that "get" information from RMVideo. 
// RMVideo may send a message to indicate a change in state, or to notify Maestro that a serious error occurred while
// processing a previous command.
//=====================================================================================================================

#define RMV_SIG_IDLE          -10         // RMVideo has entered the "idle" state
#define RMV_SIG_CMDACK        -20         // RMVideo has successfully processed the last command
#define RMV_SIG_CMDERR        -30         // RMVideo was unable to process the last command

#define RMV_SIG_ANIMATEMSG    -40         // RMVideo message sent for various reasons during an animation sequence:
// (1) Sent as reply to RMV_CMD_STARTANIMATE signaling start of animation. No payload.
// (2) Send once per second to report elapsed time. Payload = [N], where N = # elapsed frames.
// (3) Sent if an UPDATEFRAME is too late. Payload = [N,0], where N = # elapsed frames.
// (4) Sent if frame(s) duplicated due to a rendering delay. Payload = [N,X], where X = # consec. duplicate frames.

#define RMV_SIG_BYE           -50         // Response to RMV_CMD_SHUTTINGDN, acknowledging end of connection/session
#define RMV_SIG_QUITTING      -100        // RMVideo is exiting, probably because of a fatal error.


//=====================================================================================================================
// Targets supported by RMVideo
//=====================================================================================================================

#define RMV_NUMTGTTYPES       9           // remote video target types:
#define RMV_POINT             0           //    a single dot of a specified size in pixels
#define RMV_RANDOMDOTS        1           //    random-dot pattern independent of target window aperture, with options
                                          //    for finite dotlife, per-dot speed or direction noise, and % coherence
                                          //    (single color or two-color, but can be windowed by Gaussian fcn)
#define RMV_FLOWFIELD         2           //    optical flow field (single color, aperture shape ignored)
#define RMV_BAR               3           //    oriented line/bar (single color, aperture shape ignored)
#define RMV_SPOT              4           //    target window aperture filled uniformly with a single RGB color, but
                                          //    also can be windowed by Gaussian fcn)
#define RMV_GRATING           5           //    sine or squarewave grating that can drift independently of tgt window,
#define RMV_PLAID             6           //    drifting plaid composed of two sine or squarewave gratings
#define RMV_MOVIE             7           //    arbitrary animation stored in a video file
#define RMV_IMAGE             8           //    a static image stored in a JPEG,PNG,PSD,BMP, or GIF file

#define RMV_NUMTGTSHAPES      4           // remote video target window aperture shapes:
#define RMV_RECT              0           //    rectangular
#define RMV_OVAL              1           //    elliptical
#define RMV_RECTANNU          2           //    rectangular annulus
#define RMV_OVALANNU          3           //    elliptical annulus

#define RMV_F_LIFEINMS        (1<<0)      // target flags: if set, dot life is in ms, else in deg travelled
#define RMV_F_DIRNOISE        (1<<1)      // if set, per-dot noise is in direction, else it is in speed
#define RMV_F_ISSQUARE        (1<<2)      // [RMV_GRATING, _PLAID] type of grating to use: square (1) or sine (0)
#define RMV_F_INDEPGRATS      (1<<3)      // [RMV_PLAID] if set, two gratings move independently; else, as single patn
#define RMV_F_SPDLOG2         (1<<4)      // [RMV_RANDOMDOTS] if set, per-dot speed noise Vdot = Vpat*2^x, where x is 
                                          // randomly chosen from [-N..N]. Else, it is the original additive noise, 
                                          // randomly chosen from [0..N], where N in [0..300] is a %-age of Vpat.
#define RMV_F_REPEAT          (1<<5)      // [RMV_MOVIE] if set, movie plays repeatedly during an animation if the 
                                          // animation sequence is longer than the movie playback time.
#define RMV_F_PAUSEWHENOFF    (1<<6)      // [RMV_MOVIE] if set, movie is paused when turned off during an animation;
                                          // else it continues to play while off. Regardless, the movie does not start
                                          // until the first time it is turned on.
#define RMV_F_ATDISPRATE      (1<<7)      // [RMV_MOVIE] if set, movie is played back at the display frame rate rather
                                          // rather than the playback rate culled from the video file. (If no playback
                                          // rate was found, then movie is played at the display frame rate.)
#define RMV_F_ORIENTADJ       (1<<8)      // [RMV_GRATING, _PLAID] (11sep09, rev 04dec09) If set, the orientation of
// the target grating (or gratings, in the case of RMV_PLAID) are offset by the direction angle of the pattern velocity
// vector on a frame-by-frame basis. If the velocity vector is zero-amplitude in a given frame (so its direction is 
// undefined), the grating stays at the same orientation it was in during the previous frame. NOTE that this flag is
// incompatible with RMV_F_INDEPGRATS for the RMV_PLAID target.
#define RMV_F_WRTSCREEN       (1<<9)      // [RMV_RANDOMDOTS only] (11jan10) In prior versions, dot pattern velocity 
// was always relative to the target window. Now, if this flag is set, the the pattern velocity is relative to the 
// screen. The main reason for introducing this flag was to make it possible to replicate the behavior of the XYScope
// NOISYDIR and NOISYSPEED targets when the target window is moving. In the XYScope, the frame of reference is the
// screen for both window and pattern motion.

#define RMV_MAXNUMDOTS        9999        // [RMV_RANDOMDOTS, _FLOWFIELD] max # dots in target's random-dot pattern
#define RMV_MINDOTSIZE        1           // [RMV_POINT, _RANDOMDOTS, _FLOWFIELD] min/max dot size in screen pixels
#define RMV_MAXDOTSIZE        10

#define RMV_MINRECTDIM        0.01f       // min/max dimension of bounding rectangle, in deg subtended at eye
#define RMV_MAXRECTDIM        120.0f

#define RMV_MINNOISELIMIT     0           // [RMV_RANDOMDOTS] min/max for noise range limit.  For directional noise,
#define RMV_MAXNOISEDIR       180         // it is expressed in whole deg. For additive speed noise, it is expressed
#define RMV_MAXNOISESPEED     300         // as an integer %-age of pattern speed. For multiplicative speed noise (as 
#define RMV_MINSPDLOG2        1           // of Maestro v2.1.3), it is an integer N chosen from [1..7]. In this case
#define RMV_MAXSPDLOG2        7           // Vdot = Vpat*2^x, where x is chosen from [-N..N, by 2N/100].

#define RMV_MINFLICKERDUR     0           // allowed range for flicker ON duration, OFF duration, and initial delay
#define RMV_MAXFLICKERDUR     99

typedef struct RMVTargetDef         // defining parameters for a video target
{
   int   iType;                     //    target type (see defined constants)
   int   iAperture;                 //    shape of target window (see defined constants)
   int   iFlags;                    //    target flags (see defined constants)
   int   iRGBMean[2];               //    RGB color spec: mean and constrast for R (byte0), G (byte1), and B (byte2)
   int   iRGBCon[2];                //    axes. For each coord, luminance is in [0..255], contrast in [0..100%].
                                    //    Contrast only applies to grating and plaid tgt types, and the RMV_RANDOMDOTS
                                    //    [RMV_PLAID] Both elements used, representing separate color specs for the
                                    //    component gratings. 2nd element is ignored for all other tgt types

   float fOuterW;                   //    dimensions of outer and inner bounding rect in deg subtended at eye.  Inner
   float fOuterH;                   //    dimensions used for annular apertures.
   float fInnerW;                   //    [RMV_FLOWFIELD] heights ignored; widths treated as inner and outer radii.
   float fInnerH;                   //

   int   nDots;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] # of dots in random-dot pattern
   int   nDotSize;                  //    [RMV_POINT, _RANDOMDOTS, _FLOWFIELD] dot size in pixels. Range= [1..10].
   int   iSeed;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] seed for random# generator that determines
                                    //    initial dot locs (also seeds a separate RNG for dot direc or speed noise).
   int   iPctCoherent;              //    [RMV_RANDOMDOTS] percent coherence in [0..100]
   int   iNoiseUpdIntv;             //    [RMV_RANDOMDOTS] noise update interval in ms.  If 0, no noise.
   int   iNoiseLimit;               //    [RMV_RANDOMDOTS] speed or direction noise range limit (see defined constants)
   float fDotLife;                  //    [RMV_RANDOMDOTS] maximum lifetime of each target dot (0 => infinite lifetime)

   float fSpatialFreq[2];           //    [gratings/plaids] grating spatial frequency in cycle/deg subtended at eye
   float fDriftAxis[2];             //    [gratings/plaids] grating drift axis in deg CCW. Grating orientation is drift
                                    //    axis + 90deg, restricted to the unit circle, [0..360) deg.
                                    //    [RMV_BAR] first element holds drift axis of bar/line; 2nd element unused
   float fGratPhase[2];             //    [gratings/plaids] grating's initial spatial phase in deg
   float fSigma[2];                 //    [RMV_SPOT, _RANDOMDOTS, _GRATING, _PLAID] standard deviations in X and Y for
                                    //    an elliptical Gaussian window, in deg subtended at eye

   char strFolder[32];              //    [RMV_MOVIE, _IMAGE] media store folder containing source media file
   char strFile[32];                //    [RMV_MOVIE, _IMAGE] name of source media file
                                    //    NOTE: We use 32 == 8 4-byte ints. RMV_MVF_LEN must be less than 32!!!!

   int iFlickerOn;                  //    [all] flicker ON duration in # of video frames (0 = feature disabled)
   int iFlickerOff;                 //    [all] flicker OFF duration in # of video frames (0 = feature disabled)
   int iFlickerDelay;               //    [all] initial delay prior to first flicker ON, in # of video frames
} RMVTGTDEF, *PRMVTGTDEF;


// The definition of RMVTGTDEF prior to data file version 23 (Jun 2019). This obsolete version is maintained so that 
// analysis programs can properly parse target information from Maestro data files.
typedef struct RMVTargetDef_v22     // defining parameters for a video target (data file version = [13..22])
{
   int   iType;                     //    target type (see defined constants)
   int   iAperture;                 //    shape of target window (see defined constants)
   int   iFlags;                    //    target flags (see defined constants)
   int   iRGBMean[2];               //    RGB color spec: mean and constrast for R (byte0), G (byte1), and B (byte2)
   int   iRGBCon[2];                //    axes. For each coord, luminance is in [0..255], contrast in [0..100%].
                                    //    Contrast only applies to grating and plaid tgt types, and the RMV_RANDOMDOTS
                                    //    [RMV_PLAID] Both elements used, representing separate color specs for the
                                    //    component gratings. 2nd element is ignored for all other tgt types

   float fOuterW;                   //    dimensions of outer and inner bounding rect in deg subtended at eye.  Inner
   float fOuterH;                   //    dimensions used for annular apertures.
   float fInnerW;                   //    [RMV_FLOWFIELD] heights ignored; widths treated as inner and outer radii.
   float fInnerH;                   //

   int   nDots;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] # of dots in random-dot pattern
   int   nDotSize;                  //    [RMV_POINT, _RANDOMDOTS, _FLOWFIELD] dot size in pixels. Range= [1..10].
   int   iSeed;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] seed for random# generator that determines
                                    //    initial dot locs (also seeds a separate RNG for dot direc or speed noise).
   int   iPctCoherent;              //    [RMV_RANDOMDOTS] percent coherence in [0..100]
   int   iNoiseUpdIntv;             //    [RMV_RANDOMDOTS] noise update interval in ms.  If 0, no noise.
   int   iNoiseLimit;               //    [RMV_RANDOMDOTS] speed or direction noise range limit (see defined constants)
   float fDotLife;                  //    [RMV_RANDOMDOTS] maximum lifetime of each target dot (0 => infinite lifetime)

   float fSpatialFreq[2];           //    [gratings/plaids] grating spatial frequency in cycle/deg subtended at eye
   float fDriftAxis[2];             //    [gratings/plaids] grating drift axis in deg CCW. Grating orientation is drift
                                    //    axis + 90deg, restricted to the unit circle, [0..360) deg.
                                    //    [RMV_BAR] first element holds drift axis of bar/line; 2nd element unused
   float fGratPhase[2];             //    [gratings/plaids] grating's initial spatial phase in deg
   float fSigma[2];                 //    [RMV_SPOT, _RANDOMDOTS, _GRATING, _PLAID] standard deviations in X and Y for
                                    //    an elliptical Gaussian window, in deg subtended at eye

   char strFolder[32];              //    [RMV_MOVIE, _IMAGE] media store folder containing source media file
   char strFile[32];                //    [RMV_MOVIE, _IMAGE] name of source media file
                                    //    NOTE: We use 32 == 8 4-byte ints. RMV_MVF_LEN must be less than 32!!!!
} RMVTGTDEF_V22, *PRMVTGTDEF_V22;

// The definition of RMVTGTDEF prior to data file version 13 (Sep 2009). This obsolete version is maintained so that 
// analysis programs can properly parse target information from Maestro data files.
typedef struct RMVTargetDef_v12     // defining parameters for an RMVideo target (data file version = [8..12])
{
   int   iType;                     //    target type (see defined constants)
   int   iAperture;                 //    shape of target window (see defined constants)
   int   iFlags;                    //    target flags (see defined constants)
   int   iRGBMean[2];               //    RGB color spec: mean and constrast for R (byte0), G (byte1), and B (byte2)
   int   iRGBCon[2];                //    axes. For each coord, luminance is in [0..255], contrast in [0..100%].
                                    //    Contrast only applies to grating and plaid tgt types.
                                    //    [RMV_PLAID] Both elements used, representing separate color specs for the
                                    //    component gratings. 2nd element is ignored for all other tgt types

   float fOuterW;                   //    dimensions of outer and inner bounding rect in deg subtended at eye.  Inner
   float fOuterH;                   //    dimensions used for annular apertures.
   float fInnerW;                   //    [RMV_FLOWFIELD] heights ignored; widths treated as inner and outer radii.
   float fInnerH;                   //

   int   nDots;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] # of dots in random-dot pattern
   int   nDotSize;                  //    [RMV_POINT, _RANDOMDOTS, _FLOWFIELD] dot size in pixels. Range= [1..10].
   int   iSeed;                     //    [RMV_RANDOMDOTS, _FLOWFIELD] seed for random# generator that determines
                                    //    initial dot locs (also seeds a separate RNG for dot direc or speed noise).
   int   iPctCoherent;              //    [RMV_RANDOMDOTS] percent coherence in [0..100]
   int   iNoiseUpdIntv;             //    [RMV_RANDOMDOTS] noise update interval in ms.  If 0, no noise.
   int   iNoiseLimit;               //    [RMV_RANDOMDOTS] speed or direction noise range limit (see defined constants)
   float fDotLife;                  //    [RMV_RANDOMDOTS] maximum lifetime of each target dot (0 => infinite lifetime)

   float fSpatialFreq[2];           //    [gratings/plaids] grating spatial frequency in cycle/deg subtended at eye
   float fDriftAxis[2];             //    [gratings/plaids] grating drift axis in deg CCW. Grating orientation is drift
                                    //    axis + 90deg, restricted to the unit circle, [0..360) deg.
                                    //    [RMV_BAR] first element holds drift axis of bar/line; 2nd element unused
   float fGratPhase[2];             //    [gratings/plaids] grating's initial spatial phase in deg
   float fSigma[2];                 //    [RMV_SPOT, _RANDOMDOTS, _GRATING, _PLAID] standard deviations in X and Y for
                                    //    an elliptical Gaussian window, in deg subtended at eye
} RMVTGTDEF_V12, *PRMVTGTDEF_V12;

// The conditional compilation here was necessary so that we could use the header file as is when building MEX function
// READCXDATA. In that environment, the 'bool' type identifier does not exist
#if !defined(bool)
typedef struct RMVTargetMotionVec
{
   int bOn;
   float hWin;
   float vWin;
   float hPat;
   float vPat;
} RMVTGTVEC, *PRMVTGTVEC;
#else
typedef struct RMVTargetMotionVec   // "per-frame" motion vector updates tgt on/off state, window pos, and pattern pos:
{
   bool  bOn;                       //    is target on? If not, target will still be updated, but not drawn.
   float hWin;                      //    change in H,V coords of target window pos, in "visual degrees"
   float vWin;
   float hPat;                      //    analogously for pos of target pattern, BUT see NOTE below
   float vPat;
} RMVTGTVEC, *PRMVTGTVEC;
#endif
// NOTE: Specifying pattern motion for RMV_GRATING and RMV_PLAID targets.  A grating can only drift along its drift
// axis, so RMVTGTVEC.hPat and .vPat are interpreted uniquely for these two target types.  For RMV_GRATING, vPat is
// ignored, and hPat is treated as position change along the grating's drift axis.  For RMV_PLAID, there are two
// possibilities depending upon the state of the RMV_F_INDEPGRATS flag.  If the flag is cleared, then hPat and vPat
// are interpreted in the normal way -- as the horizontal and vertical position changes of the plaid pattern as a
// whole.  If the flag is set, the gratings move indepedently:  hPat is the position change of grating #1 along its
// drift axis, while vPat is the position change of grating #2 along its drift axis.

#define RMV_TGTVEC_F2I_F 1000000.0f // divide by this to recover floating-pt value of an int-coded tgt motion param

#endif // !defined(RMVIDEO_COMMON_H_INCLUDED_)
