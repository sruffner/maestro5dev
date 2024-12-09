//=====================================================================================================================
//
// cxipc.h : The shared memory interface for IPC between MAESTRO (Windows) and its hardware driver CXDRIVER (RTSS).
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Maestro is really two processes that work together: a Windows-based GUI "master" and an RTX64-based "driver" for
// hardware communication and "real-time" control of experimental protocols.
//
// "Maestro" refers to the Windows process that presents and manages the graphical user interface (GUI). It is built
// upon the document/view architecture with Microsoft Foundation Classes (MFC), and presents an elaborate set of views
// for designing experiment protocols. It also includes a number of different "output/display windows" for runtime
// display of: eye/fixation target position, actual acquired data traces, spike histograms, and error/information
// messages. Finally, two runtime control panels allow the user to start, stop, and sequence Maestro trials or
// continuous-stimulus runs, and a third control panel handles test & calibration of selected Maestro hardware.
//
// "CXDRIVER" is the runtime engine of Maestro, a driver-like RTSS process existing within a Real-Time eXtension 
// (RTX, from IntervalZero, Inc.) to Windows. It communicates with all data acquisition and stimulus-control hardware
// used by Maestro experimental protocols. More importantly, it configures and controls this hardware to execute the 
// protocols defined by the user on the GUI side.
//
// Naturally, these two processes must cooperate closely and share data between them.  We have elected to use a simple
// shared memory interface to implement this "interprocess communication" (IPC). That interface is represented by the
// data structures and constants defined in this file.
//
// In addition to definitions directly related to IPC, the file also includes "cxobj_ifc.h", which provides constants
// and data structures defining the various Maestro data objects, such as targets, trials, continuous stimuli, etc.
// These are necessary so that Maestro can efficiently "download" object information to CXDRIVER, which uses that 
// info to execute experimental protocols.
//
// ==> Synchronizing access to shared memory.
// Common practice in multithreading design is to use synchronization objects like semaphores, events, and mutexes to
// avoid conflicts between threads and processes accessing a shared resource. We decided early on to avoid the use
// of these mechanisms for performance reasons -- they introduce significant overhead. Instead, we rely on careful
// application design to avoid conflicts. One typical technique we employ is the use of "request" and "ack" flags w/in
// the shared memory object to regulate access to other data structs within shared memory. For example, consider the
// mechanism for posting changes in eye and fixation target position. The position data are "gated" by two flags:
// bReqPlot is "owned" by the driver and is read-only as far as the master is concerned; bAckPlot is owned by the
// master and is read-only to the driver. Here is how the flags are used to post an update to eye/target positions:
//
// bReqPlot       bAckPlot             Action
// --------       --------             ------------------------------------------------------------------------------
//    F              F                 Idle.  Neither side is accessing position data.
//    F              F                 CXDRIVER fills in the position data in preparation for an update.
//    T              F                 CXDRIVER "posts" request for a plot update and continues w/ other work.
//    T              F                 Later, Maestro detects the request and begins accessing the new position data
//                                     in order to process the request. If CXDRIVER needs to post another update
//                                     during this time, it detects that a previous update is still in progress -- so
//                                     it MUST DROP the pending update.
//    T              T                 Maestro sets the "ack" flag to indicate that it has finished processing request.
//                                     Still, a new update cannot be posted, b/c the flags must be reset.
//    F              T                 CXDRIVER has detected that the previous update has been serviced. It 
//                                     resets the request flag to acknowledge this.
//    F              F                 Once it detects the request flag reset, Maestro resets its ack flag.  We're now
//                                     back to "idle", and CXDRIVER is now free to post a new update.
//
// ==> The CXDRIVER "stop" mutex.
// Maestro should be able to recover if CXDRIVER should terminate unexpectedly, so it needs a means of detecting 
// such an occurrence. One way to do this is to create a monitoring thread which waits indefinitely on the process 
// handle. When the process dies, the handle becomes "signaled", waking up this thread, which can then send a message 
// to the Maestro app object. But, since CXDRIVER is an RTSS process -- which may NOT serve as a synchronization
// object -- we must employ a different approach: CXDRIVER immediately creates and claims a "driver-alive" mutex 
// when it starts up, and does not release it until it exits. Shortly after spawning CXDRIVER via RTSSRUN, 
// Maestro attempts to obtain a handle to that same mutex. If successful, this mutex can serve the same purpose as the 
// process handle, as earlier described.
//
// ==> Two settings from the Maestro key in the Windows registry are passed via IPC (Sep 2019).
// Since its inception, the full pathname for the Maestro installation directory is stored in an entry under a key in 
// the HKLM node. Maestro stores the path in CXIPC.strDataPath just prior to launching CXDRIVER; CXDRIVER needs this
// path to find certain program files. See CCxMasterIO::GetHomeDirectory().
//
// In Sep 2019, a new registry entry was added that lists the busy wait times (in microseconds) for the function that
// writes commands to external latched devices via DO<15..0>: CCxEventTimer::SetDO(). Testing in the Joshua lab has
// demonstrated that a memory-mapped register write can experience a brief delay of a few microseconds. The end result
// is the sequence of steps in SetDO() -- write DO command, set DataReady=0 to latch the data, then set DataReady=1
// afterwards -- may all get executed on the hardware in rapid succession, so that a latched device could "miss" the
// command entirely. On the other hand, the Lisberger lab has not seen this issue -- perhaps due to differences in
// workstation capabilites or DIO interface capabilities. To address this problem, the user can now adjust the busy 
// wait times via the aforementioned registry entry. Maestro handles the task of reading and parsing the entry and
// storing the 3 busy wait times in CXIPC.fDOBusyWaits[].
//
// NOTE: Maestro has no need for the busy wait times; CXDRIVER should be able to read the registry entry directly
// since RTX64 supports the needed Windows registry functions in the RTSS environment. HOWEVER, when I modified 
// CXDRIVER to do so, it consistently crashed the system on the RegQueryValueEx() call. We decided to let Maestro
// handle the registry and pass the settings via IPC.
//
//
// REVISION HISTORY:
// 29mar2001-- Began development, based on the original implementation in cntrlxPC, the predecessor to CNTRLX that
//             worked in concert with a UNIX-based GUI.
// 26apr2001-- Added support for analog I/O in test mode, plus some hardware status info...
// 01may2001-- More "gate" flags for commands/requests in test mode..
// 11jun2001-- Added structures for passing commands to CXDRIVER, and responses back to CNTRLX.  These are strictly for
//             work that can be executed very quickly (w/in 20-40msec) by CXDRIVER, as CNTRLX will block upon posting
//             a command until CXDRIVER's response is received.
//          -- Rather than using dedicated space in the shared memory structure for TestMode work, we've begun
//             converting the TestMode operations into commands...
// 02aug2001-- Adding support for generating a 1Hz sine wave on a selected AO channel in TestMode: CX_TM_AOWAVE.
// 06aug2001-- Working on implementation of data trace facility...
// 24oct2001-- Some adjustments to implement eye-target position plot facility...
// 06dec2001-- Began mods to support trial-mode operations...
// 10jan2002-- Continuing mods to support trial-mode ops...
// 28feb2002-- The "die" flag in CXIPC removed.  CNTRLX tells CXDRIVER to die by issuing CX_SWITCHMODE command with the
//             mode set to CX_STOPPING.
// 26mar2002-- Added 'bChairPresent' flag to CXIPC.  If FALSE, CXDRIVER assumes animal chair is not in use, and chair
//             compensation does not take place (outside IdleMode, that is).
// 18apr2002-- Added 'bSaveSpikeTrace' flag.  If set, CXDRIVER records AI channel 15 at 25KHz; this channel is assumed
//             to carry a "spike waveform" signal (prior to spike detection circuitry) from the microelectrode.
//          -- Spike trace data will be saved in the trial data file, NOT a separat file; removed strSpksPath[].
// 18oct2002-- Added cmd CX_FIXREWSETTINGS, which sends all fixation/reward settings to CXDRIVER in any op mode except
//             TestMode.  The "reward indicator beep" enable flag is one of these settings, so CXIPC.bEnableAudio was
//             removed.  Most of the info in the CX_CM_UPDFIXREQ command was also absorbed by CX_FIXREWSETTINGS.  The
//             CX_CM_UPDFIXREQ cmd is replaced by CX_CM_UPDFIXTGTS, which is applicable only to ContMode and which
//             reports the identities of fix tgts #1 and #2 (as indices in the active tgt list).
//          -- Added field CXIPC.iNumRewards, the #rewards delivered to the animal since last reset by host.  This
//             field is incremented by CXDRIVER each time a reward is delivered.
// 13jan2003-- Modified CX_SETDISPLAY command to include additional parameters for the XY scope display.
// 14jan2003-- CX_CM_RUNSTART command modified.  No longer needs to send XY dot seed.  CCxScope handles seed generation
//             IAW current display parameters as specified in the CX_SETDISPLAY command.
// 07oct2003-- Added secondary eye position CX_EYE2 (HGPOS2, VEPOS2) to the eye-target position plot.
// 07jul2004-- Added field by which CXDRIVER reports elapsed time of the most recent trial presented, 'iLastTrialLen'.
// 10mar2005-- Added field CXIPC.iTotalRewardMS, the sum of all reward pulses (in ms) delivered since last reset by
//             host.  Will not be accurate for rigs lacking the variable reward pulse device!
// 06apr2005-- Added fields CXIPC.nSections and CXIPC.trialSections[] to communicate a trial's tagged sections to
//             CXDRIVER.  This information is stored in the data file for later use by analysis programs.
// 15jun2005-- Command CX_CM_UPDFIXTGTS extended to include index of the active target designated for tracking cursor
//             position within the eye-target plot.  CXDRIVER does not really need knowledge of the "tracking" target
//             concept, but we don't want to bypass the fact that the eye-target plot on the Maestro side reflects
//             target position AS PROVIDED BY CXDRIVER.
//          -- Added new locus CX_TRACK to the position plot facility.
// 16sep2005-- Added trial result flag bit, CX_FT_NORESP, which indicates that subject did not respond at all (right or
//             wrong) to the trial.  This applies only to staircase trials.
// 24oct2005-- Added per-trial alternate XY dot seed value, CXIPC.iXYDotSeedAlt.  If negative, it is ignored and seed
//             is controlled by display settings.  If 0, then seed will be auto-generated for current trial.  If >0,
//             then value is taken as the fixed seed for XY target random dot generation.
// 05dec2005-- Added CXIPC.fResponse and result flags in support of the new distribution-based reward contingency
//             protocol (Maestro v1.4).
// 14mar2006-- Dropped support for OKN servo, which was obsolete and never used in Maestro anyway.  Removed defined
//             constant CX_F_OKNAVAIL.
// 24mar2006-- VSG FB video replaced by the RMVideo display server. 
// 21apr2006-- Added some RMVideo display properties (screen size in pixels, frame rate, 16 vs 32bit color).
// 11dec2006-- Modified CX_CMUPDACVTGT command to include target pattern speed and direction.
// 28apr2010-- Added CXIPC.fStartPosH,V to store global target starting position offset in deg (Trial mode only).
// 24may2010-- Added CXIPC.dwTrialFlags to store flag bits from trial definition (Trial mode only).
// 27sep2011-- CX_TM_GETAI now returns some stats on the AI channel data recorded in Test mode.
// 11oct2016-- Updated constant names and various comments to reflect the fact that the RMVideo "movie store" is a
//             "media store" that can include both image and video files.
// 25oct2017-- Changes to support Windows 10/RTX64 build: There is no notion of "process slot" in RTX64, so the 
// corresponding member of the CXIPC structure is removed. Launch and termination of CXDRIVER.RTSS is handled in a
// different way in Maestro 4.x, using new RTX64 API.
// 11jun2018-- Added CXIPC.strSet[], .strSubset[] to store name of trial set and, if applicable, subset.
// 24sep2018-- Modified CX_SETDISPLAY command to include parameters governing a vertical sync spot flash on RMVideo 
// monitor -- for better sync'ing of Maestro timeline with RMVideo animation (flash is detected by photodiode, flash 
// event pulse timestamped by Maestro).
// 25sep2018-- Removed sync spot flash "margin" parameter. Feature is defined by spot's (square) size in mm and the
// flash duration in # video frames.
// 19mar2019-- Added trial result flag CX_FT_RMVDUPE, indicating that trial stopped prematurely because RMVideo 
// detected duplicate frame(s) due to a rendering delay or a delay receiving target updates from RMVideo. As of Maestro
// v4.0.5, this result is treated like CX_FT_LOSTFIX -- instead of aborting the trial sequence, the trial is repeated
// as appropriate for the current sequencing mode. Also added CXIPC.bTolRMVDuplFrame.
// 05sep2019-- Three busy wait times for CCxEventTimer::SetDO() are now stored under the Maestro key in the Windows
// registry. Maestro reads and parses the entry at startup and stores the busy wait times in CXIPC.fDOBusyWaits[] just
// before launching CXDRIVER.
// 03nov2022-- Added trial result flag CX_FT_EYELINKERR, indicating that trial stopped prematurely because of an
// error getting the next Eyelink sample (eg, sample delay too long). This result is treated like an RMVideo dupe frame
// error -- instead of aborting the trial sequence, the trial is repeated as appropriate to the sequencing mode. The
// aborted trial's data is not saved.
// 26sep2024-- A/o Maestro 5.0, XYScope functionality removed. Eliminated CXIPC.iXYDotSeedAlt.
//=====================================================================================================================

#if !defined(CXIPC_H__INCLUDED_)
#define CXIPC_H__INCLUDED_

#include "cxobj_ifc.h"                             // data structures & constants related to Maestro targets, etc.
#include "cxtrialcodes.h"                          // CXDRIVER trial codes.  Maestro trial definition is converted 
                                                   // to a sequence of these trial codes.

#define CXIPC_SHM          "cx_ipc.sharedmem"      // unique name of the Maestro shared memory object
#define CXIPC_STOPMUTEX    "cx_ipc.stopmutex"      // unique name of the mutex that is signalled when driver stops


#define CXIPC_MSGSZ        150                     // max #chars (incl terminating '\0') in a posted message
#define CXIPC_MSGQLEN      20                      // # of messages that can be queued in shared memory

#define CX_NTRACES         10                      // max # of simultaneously updated data traces
#define CX_TRBUFSZ         4000                    // size of trace buffers.  time epoch rep by each sample in trace
                                                   // depends on operational mode (1ms in TrialMode, 2ms otherwise).
                                                   // MUST be a multiple of the trace buffer "segment size".
#define CX_TRSEGSZ         1000                    // trace data is displayed in "segments" this large.
#define CX_AITRACE         0                       // three types of data traces:  analog input, digital input "event",
#define CX_DITRACE         1                       //    or a "computed" signal
#define CX_CPTRACE         2

#define CX_EVTBUFSZ        500                     // size of digital event mask/time buffers
#define CX_EVTCHUNKSZ      100                     // Maestro consumes the digital event buffers in chunks this size

#define CX_CP_NCHANS       6                       // "computed" channels reflect expected trajectories of fix targets:
#define CX_CP_HVFIX1       0                       //    horiz velocity of fix tgt #1
#define CX_CP_VVFIX1       1                       //    verti velocity of fix tgt #1
#define CX_CP_HVFIX2       2                       //    analogously for fix tgt #2
#define CX_CP_VVFIX2       3                       //
#define CX_CP_HPFIX1       4                       //    horiz position of fix tgt #1
#define CX_CP_VPFIX1       5                       //    verti position of fix tgt #1

#define CX_NLOCI           6                       // position loci in the eye-target position display:
#define CX_EYE             0                       //    eye position - primary
#define CX_EYE2            1                       //    eye position - secondary (special purpose use)
#define CX_FIX1            2                       //    fixation target 1 position
#define CX_FIX2            3                       //    fixation target 2 position
#define CX_CHAIRPOS        4                       //    animal chair position (y-coord ignored)
#define CX_TRACK           5                       //    position of cursor tracking target

#define CX_AIO_MAXN        16                      // **upper limit** on the #ch (each) of analog input/output
#define CX_TMR_MAXN        32                      // **upper limit** on the #ch (each) of event timer dig input/output

                                                   // the operational modes of Maestro/CXDRIVER:
#define CX_IDLEMODE        0                       //    idle
#define CX_TESTMODE        1                       //    test & calibration
#define CX_TRIALMODE       2                       //    trials
#define CX_CONTMODE        3                       //    continuous
#define CX_STARTING        -1                      //    transient start-up phase for CXDRIVER
#define CX_STOPPING        -2                      //    transient shut-down phase for CXDRIVER
#define CX_NOTRUNNING      -3                      //    pseudo op mode indicates that CXDRIVER is not running

                                                   // hardware status flags from CXDRIVER:
#define CX_F_AIAVAIL       ((DWORD) (1<<0))        //    analog inputs available
#define CX_F_TMRAVAIL      ((DWORD) (1<<1))        //    event timer (and dig I/O) available
#define CX_F_AOAVAIL       ((DWORD) (1<<2))        //    analog outputs available
#define CX_F_XYAVAIL       ((DWORD) (1<<3))        //    [deprecated a/o Maestro 4.0] XY scope hardware available
#define CX_F_RMVAVAIL      ((DWORD) (1<<4))        //    RMVideo framebuffer display available
#define CX_F_AVAILMASK     (CX_F_AIAVAIL |   \
                            CX_F_TMRAVAIL |  \
                            CX_F_AOAVAIL |   \
                            CX_F_XYAVAIL |   \
                            CX_F_RMVAVAIL )
#define CX_F_AO16BIT       ((DWORD) (1<<6))        //    if set, analog outputs have 16-bit resolution; else, 12-bit
#define CX_F_AI16BIT       ((DWORD) (1<<7))        //    if set, analog inputs have 16-bit resolution; else, 12-bit
#define CX_F_AICAL         ((DWORD) (1<<8))        //    if set, AI supports rapid, internal calibration

                                                   // trial results flag bits:
const DWORD CX_FT_DONE        = ((DWORD) (1<<0));  //    trial done
const DWORD CX_FT_ABORTED     = ((DWORD) (1<<1));  //    trial prematurely terminated by CX_TR_ABORT command
const DWORD CX_FT_ERROR       = ((DWORD) (1<<2));  //    an error prematurely terminated protocol
const DWORD CX_FT_LOSTFIX     = ((DWORD) (1<<3));  //    subject broke fixation during trial,
const DWORD CX_FT_RESPOK      = ((DWORD) (1<<4));  //    subject's response to trial was "correct"
const DWORD CX_FT_DATASAVED   = ((DWORD) (1<<5));  //    data file saved upon trial completion
const DWORD CX_FT_SEQSTOP     = ((DWORD) (1<<7));  //    [Maestro use only] staircase seq has autostopped, or seq
                                                   //       stopped by catastrophic error
const DWORD CX_FT_BLOCKDONE   = ((DWORD) (1<<8));  //    [Maestro use only] last trial in "block" done (ordered &
                                                   //       randomized seq modes only)
const DWORD CX_FT_NORESP      = ((DWORD) (1<<9));  //    subject did not respond (right or wrong) to the trial
const DWORD CX_FT_GOTRPDRESP  = ((DWORD) (1<<10)); //    set if behavioral response was measured for an "R/P Distro"
                                                   //    trial (even if trial did not run to completion)
const DWORD CX_FT_RMVDUPE     = ((DWORD) (1<<11)); //    trial stopped on duplicate frame signal from RMVideo
const DWORD CX_FT_EYELINKERR  = ((DWORD) (1<<12)); //    trial stopped on Eyelink tracker sample delay or other error

                                                   // Continuous mode operational state flag bits:
const DWORD CX_FC_RUNON       = ((DWORD) (1<<16)); //    stimulus run in progress
const DWORD CX_FC_RUNSTOPPING = ((DWORD) (1<<17)); //    stimulus run stopping at end of current duty cycle
const DWORD CX_FC_RECORDING   = ((DWORD) (1<<18)); //    data recording in progress
const DWORD CX_FC_FIXATING    = ((DWORD) (1<<19)); //    fixation is ON
const DWORD CX_FC_SAVING      = ((DWORD) (1<<20)); //    saving data after recording stopped (in case file I/O blocks)

#define CX_MAXTGTS         400                     // max # of targets in the loaded target list
#define CX_MAXTC           3000                    // max # of trial codes defining a single Maestro trial
#define CX_MAXPATH         257                     // max length of file system pathname

// length of char array used for RMVideo media store command/responses. Sized for worst-case scenario
#define CX_CDATALEN (RMV_MVF_LIMIT * (RMV_MVF_LEN +1) + 10)

#define CX_MAXEL           1000                    // max # of Eyelink tracker samples in IPC (1.0 sec at 1KHz)
#define CX_ELSTAT_OFF      0                       // Eyelink tracker status: unconnected;
#define CX_ELSTAT_IDLE     1                       //   connected, idle (not recording);
#define CX_ELSTAT_REC      2                       //   1KHz recording in progress;
#define CX_ELSTAT_FAIL     3                       //   previous recording session aborted on error condition

                                                   // Maestro->CXDRIVER cmds; format described (I = IdleMode, T =
                                                   // TrialMode, C = ContMode, TE = TestMode, SLOW = long-latency cmd):
#define CX_CMDLEN          100                     //    size of generic data buffers associated with commands
#define CX_NULLCMD         ((DWORD) 0)             //    no command
#define CX_DRVROFF         ((DWORD) 1)             //    response to command request when CXODRIVER not running
#define CX_PENDINGCMD      ((DWORD) 2)             //    cannot send command b/c previous one is pending (by design,
                                                   //       this should NEVER happen!)
#define CX_ILLEGALCMD      ((DWORD) 3)             //    illegal command parameters
#define CX_TIMEDOUTCMD     ((DWORD) 4)             //    cmd/resp handshake not completed within ~300msec
#define CX_UNRECOGCMD      ((DWORD) 5)             //    cmd not recognized by CXDRIVER in current context
#define CX_FAILEDCMD       ((DWORD) 6)             //    CXDRIVER unable to process command in current context

#define CX_SWITCHMODE      ((DWORD) 100)           //    [any mode] switch to mode X = iData[0].
#define CX_INITTRACE       ((DWORD) 101)           //    [T,C,TE] initialize data trace facility.
#define CX_SAVECHANS       ((DWORD) 102)           //    [any mode] update list of AI channels saved to file, where N =
                                                   //       iData[0] is the # of channels saved and iData[1..N] are the
                                                   //       actual channel#s in the order saved.
                                                   // 
// NOTE: a/o Maestro 5.0, XYScope-related elements are removed from the Maestro GUI. We elected not to change the format
// of the CX_SETDISPLAY command, however. The first 7 integer parameters are always 0.
#define CX_SETDISPLAY      ((DWORD) 103)           //    [I,T,C] set new XY & RMVideo display parameters and update
                                                   //       displays accordingly.  iData[0..6] are XY display params,
                                                   //       respectively:  dist to eye (mm), screen width (mm), screen
                                                   //       height (mm), dot draw delay and dur (each in 100ns ticks),
                                                   //       auto-generate dot seed flag (0 or 1), fixed dot seed value.
                                                   //       iData[7..14] are RMVideo params, resp: distance to eye,
                                                   //       screen width and height (mm); background color R,G, and B
                                                   //       values (each in arbitrary units ranging 0-255); vsync spot
                                                   //       flash size in mm, flash dur in #frames.
                                                   //       NOTE: Restricted use in T,C modes.

#define CX_FIXREWSETTINGS  ((DWORD) 104)           //    [I,T,C] update fixation/reward settings, where:
                                                   //          iData[0] = fixDur in in ms,
                                                   //          iData[1,2] = fixRewLen1,2 in ms,
                                                   //          iData[3] = random withholding variable ratio (1=disable)
                                                   //          iData[4] = audio reward pulse len in ms (0=off)
                                                   //          iData[5] = reward indicator beep enable (0/1 = off/on)
                                                   //          fData[0,1] = H,V fix accuracy in visual deg
#define CX_INITEVTSTREAM   ((DWORD) 105)           //    [T] initialize digital event stream facility

#define CX_RMV_GETMODES    ((DWORD) 120)           //    [I] get a list of all available RMVideo display modes. Returns:
                                                   //       iData[0] = number of modes, N <= 30; and for n=[0..N-1],
                                                   //       iData[1+n*3] = screen width in pixels for mode n,
                                                   //       iData[2+n*3] = screen height in pixels for mode n,
                                                   //       iData[3+n*3] = approx. frame rate in Hz for mode n.
#define CX_RMV_GETCURRMODE ((DWORD) 121)           //    [I] get the current RMVideo display mode. Returns:
                                                   //       iData[0] = mode index n in [0..N-1], where N is the number
                                                   //       of available modes. 
                                                   //       fData[0] = measured frame rate in Hz (NOT the nominal rate).
#define CX_RMV_SETCURRMODE ((DWORD) 122)           //    [I,SLOW] change the RMVideo display mode:
                                                   //       iData[0] = (in) desired mode index n in [0..N-1], where N 
                                                   //       is the number of available modes.
                                                   //       fData[0] (out) measured frame rate in Hz. BLOCKS for up to 
                                                   //       10 secs because frame period is remeasured over an interval
                                                   //       of 500 frames after mode switch.
#define CX_RMV_GETGAMMA    ((DWORD) 123)           //    [I] get R,G,B gamma factors for RMVideo monitor. Returns:
                                                   //       fData[0..2] = the R,G,B gamma-correction factors.
#define CX_RMV_SETGAMMA    ((DWORD) 124)           //    [I] set R,G,B gamma factors for RMVideo monitor:
                                                   //       fData[0..2] = (in) the desired R,G,B gamma-correction
                                                   //       factors; (out) the actual factors set. Factors are range-
                                                   //       limited to [0.800 .. 3.000].

                                                   //    RMVideo media store commands. In all of these, media folder 
                                                   //    and file names are restricted in length and character content.
                                                   //    See rmvideo_common.h...
#define CX_RMV_GETMDIRS   ((DWORD) 130)            //    [I] get a list of all folders in the RMVideo media store. The
                                                   //       folder names are stored in cData[], separated by a single
                                                   //       null character. iData[0] = number of folders in list.
#define CX_RMV_GETMFILES  ((DWORD) 131)            //    [I] get a list of all media files in a specified folder in 
                                                   //       the RMVideo media store. The folder name is placed in
                                                   //       cData[], with a terminating null. Returns: iData[0]= number
                                                   //       of media files in specified folder; file names are stored 
                                                   //       in cData[], separated by a single null character.
#define CX_RMV_GETMFINFO  ((DWORD) 132)            //    [I] retrieve info on a particular media file in the RMVideo 
                                                   //       media store. The folder name and file name identifying the
                                                   //       video or image source are placed in cData[], including the 
                                                   //       terminating null characters; folder name is first. Returns:
                                                   //       iData[0..1] = w, h of the image or video frame in pixels.
                                                   //       fData[0] = ideal playback rate in Hz (videos only).
                                                   //       fData[1] = approximate duration of video in seconds. This
                                                   //       information is culled from the file itself; if any param
                                                   //       is not found in the file, it is set to 0. NOTE that fData[]
                                                   //       params are set to a negative value for an image file!
#define CX_RMV_DELMEDIA    ((DWORD) 133)           //    [I, SLOW] remove media file or an entire media folder from the
                                                   //       RMVideo media store. iData[0] = 0 if a single file is to be
                                                   //       deleted; nonzero for an entire folder. The folder name, and
                                                   //       file name if applicable, are placed in cData[], each with a
                                                   //       terminating null; the folder name is always first. BLOCKS
                                                   //       for up to 5 seconds.
#define CX_RMV_PUTMEDIA    ((DWORD) 134)           //    [I, SLOW] download a media file to the RMVideo media store.
                                                   //       cData[] should contain the following strings, in order, each
                                                   //       terminated by a null character: Absolute pathname of the
                                                   //       file to be downloaded (not to exceed CX_MAXPATH), the
                                                   //       destination folder name, destination file name. This command 
                                                   //       will take an indefinite amount of time to complete. If 
                                                   //       successful, returns the media info in iData[0..1] and 
                                                   //       fData[0..1], as described for the CX_RMV_GETMFINFO command.

#define CX_TM_PAUSEAI      ((DWORD) 200)           //    [TE] pause DAQ of all AI channels
#define CX_TM_RESUMEAI     ((DWORD) 201)           //    [TE] resume DAQ of all AI channels
#define CX_TM_SETAO        ((DWORD) 202)           //    [TE] set voltage on AO channel #N to X, where...
                                                   //       N = iData[0]; if -1, all channels are set to same value
                                                   //       X = fData[0] (volts); CXDRIVER sets fData[0] to 
                                                   //       nearest reproducible value.
#define CX_TM_AICAL        ((DWORD) 203)           //    [TE] perform internal calibration of AI board.
#define CX_TM_GETAI        ((DWORD) 204)           //    [TE] get channel stats from AI board. Let N = # of AI channels
                                                   //       available. Returns most recent voltage sample per channel in
                                                   //       fData[0..N-1], mean observed voltage in fData[N..2N-1], and
                                                   //       standard deviation in fData[2N..3N-1].
#define CX_TM_AOWAVE       ((DWORD) 205)           //    [TE] run 1Hz sine wave on AO chan #N, where N = iData[0];
                                                   //       if -1, the test waveform is stopped.
#define CX_TM_GETTMRSTATE  ((DWORD) 210)           //    [TE] get current state of event timer device:
                                                   //       #events recorded per DI chan in iData[0..N-1]; time (sec)
                                                   //       of last event in fData[0..N-1], and mean event intv (sec)
                                                   //       in fData[N..2N-1], where N = the # of DI chans available on
                                                   //       the event timer device.  the event mask for the most recent
                                                   //       event is placed in iData[N].
#define CX_TM_RESETTMR     ((DWORD) 211)           //    [TE] reset & restart event timer device.
#define CX_TM_SETTMRDO     ((DWORD) 212)           //    [TE] set digital output port on event timer device;
                                                   //       ((DWORD) iData[0]) contains new bit mask for the port.

#define CX_TR_PRELOADFB    ((DWORD) 300)           //    [TC,SLOW] if there are any FB video targets in the target
                                                   //       list, preload them onto the video hardware.
                                                   //    24mar2006: OBSOLETE. No longer needed with RMVideo!

#define CX_TR_START        ((DWORD) 301)           //    [T] start a trial.  trial & target info is preloaded into the
                                                   //       CXIPC (see below)
#define CX_TR_ABORT        ((DWORD) 302)           //    [T] abort the currently running trial and discard any trial
                                                   //       data. This is the only command to which CXDRIVER will
                                                   //       respond while running a trial.

#define CX_CM_FIXOFF       ((DWORD) 400)           //    [C] turn fixation checking OFF
#define CX_CM_FIXON        ((DWORD) 401)           //    [C] turn fixation checking ON
#define CX_CM_UPDFIXTGTS   ((DWORD) 402)           //    [C] update fixation designations for active targets, where:
                                                   //       iData[0,1] = indices of fix tgts #1,2 in active list, and
                                                   //       iData[2] = index of the cursor tracking tgt (an index of
                                                   //       -1 means "none").
#define CX_CM_UPDACVTGT    ((DWORD) 403)           //    [C] update an active tgt: iData[0] = valid active tgt index,
                                                   //       iData[1] = on(1)/off(0) state, fData[0,1] = hPos,vPos in 
                                                   //       deg, and fData[2,3] = patn speed (deg/s) and dir (deg).
                                                   // OR:[C] initialize entire active tgt list:  iData[0] = -1,
                                                   //       iData[1..N] = on/off states of acv tgts 0..N-1,
                                                   //       fData[0,4,..4N] = hPos for acv tgts 0..N-1,
                                                   //       fData[1,5,..4N+1] = vPos for acv tgts 0..N-1,
                                                   //       fData[2,6,..4N+2] = pattern speed for acv tgts 0..N-1, and 
                                                   //       fData[3,7,..4N+3] = pattern dir for acv tgts 0..N-1
                                                   //       where N is the # of acv tgts currently defined in CXIPC.
#define CX_CM_RUNSTART     ((DWORD) 404)           //    [C] start the stimulus run currently defined in CXIPC
#define CX_CM_RUNSTOP      ((DWORD) 405)           //    [C] stop ongoing stimulus run, where iData[0] = 0 (stop at end
                                                   //       of duty cycle), 1 (stop NOW), or 2 (stop NOW, turn off
                                                   //       recording, and discard recorded data file).
#define CX_CM_RECON        ((DWORD) 406)           //    [C] start recording data to file listed in CXIPC
#define CX_CM_RECOFF       ((DWORD) 407)           //    [C] stop recording data, where iData[0] = 0 (discard recorded
                                                   //       data file), or 1 (save recorded data file). returns success
                                                   //       (1) or failure (0) of file save in iData[0].


typedef struct tagCxIpc                            // encapsulation of Maestro IPC shared memory
{
   //
   // OPERATIONAL MODE.  By design, CXDRIVER comes up in CX_IDLEMODE, after a transient CX_STARTING state during 
   // which hardware initializations take place.  Mode changes occur only when MAESTRO issues the CX_SWITCHMODE cmd. 
   // While CXDRIVER will respond immediately to CX_SWITCHMODE, it may take a little while to clean up before 
   // actually entering the requested mode. Thus, after sending CX_SWITCHMODE, Maestro should continue checking this 
   // variable until it changes to the requested mode ID.
   //
   int         iOpMode;                            //    fr driver: current operational mode of CXDRIVER

   //
   // MESSAGE QUEUE.  As a "service" to CXDRIVER, Maestro displays error/information messages posted to this 
   // *circular* queue by CXDRIVER.  The queue needs servicing whenever the index of the last msg posted does not 
   // equal the index of the next msg to post.  CXDRIVER stops posting when (iNext+1) % queueLength == iLast; the 
   // new message is lost. Also, CXDRIVER increments iNext *after* preparing the message.  These rules ensure that 
   // Maestro never tries to read a message while CXDRIVER is still writing it.
   //
   // REWARD INDICATOR BEEP. If the message posted == "beep", Maestro will play the "system default sound" as an 
   // indicator to the user that a reward was just delivered to the subject. Note that it is important for the user to 
   // configure the system to use a very short audio file for the system default sound, since rewards may be delivered
   // roughly 1-2 seconds apart in many experimental paradigms. The reward beep will be "late" if there's a backlog of
   // messages being posted, but that would be a rare scenario and likely means that an error has occurred.
   // [Prior to Maestro4, CXDRIVER played a beep on the onboard system speaker (IO port 0x61). However, this is a 
   // very antiquated device, and may cause AI ISR latency issues on some hardware; so a new solution was warranted.
   //
   char        szMsgQ[CXIPC_MSGQLEN][CXIPC_MSGSZ]; //    fr driver: the circular queue of messages
   int         iNextMsgToPost;                     //    fr driver: queue index of next message to post
   int         iLastMsgPosted;                     //    to driver: last message posted by Maestro

   //
   // EYE/TARGET POSITION PLOT.  This is another "service" provided by Maestro.  Whenever it detects a "plot update"
   // request from CXDRIVER, it reads the positions stored here and updates the GUI's position plot accordingly.
   // CXDRIVER will drop any plot updates that occur while Maestro is still servicing a previous update.
   // ** all loci are reported in hundredth-degrees of visual field **
   //
   BOOL        bReqPlot;                           //    fr driver: update eye/tgt position plot
   BOOL        bAckPlot;                           //    to driver: finished servicing plot update
   POINT       ptLoci[CX_NLOCI];                   //    fr driver: new eye/tgt positions to be plotted

   //
   // DATA TRACE DISPLAY.  Maestro displays acquired data that CXDRIVER streams thru the data trace buffers here. 
   // There are three kinds of acquired data:  analog input waveforms (CX_AITRACE), computed waveforms (CX_CPTRACE), 
   // and digital input pulse trains (CX_DITRACE).  All signals are sampled at a rate that depends on operational mode
   // (1ms for TrialMode, 2ms for TestMode and ContMode) -- hence the time period covered by the trace buffers varies.
   // For the analog input & computed waveforms, samples are stored as raw AI binary 2s complement values.  For the
   // pulse trains, a nonzero sample indicates that a digital input "event" occurred during that sample period.
   //
   // Maestro sets the # of traces to display, their types & channel #s, then issues CX_INITTRACE. CXDRIVER then
   // begins streaming data into the buffers.  The buffers are treated like a circular queue.  Whenever iTraceEnd !=
   // iTraceDrawn, there is data available for display.  Maestro will typically wait for a "chunk" of data to be ready
   // before performing an update; it will then advance iTraceDrawn by the # of samples processed. CXDRIVER must 
   // stop streaming data when (iTraceEnd+1) % CX_TRBUFSZ == iTraceDrawn, as this indicates that the buffers are full. 
   // In this case, CXDRIVER sets the overflow error flag and streaming is halted.
   //
   // The data trace facility, as designed, can only work when a DAQ is running on the AI board, since the DAQ itself
   // establishes a real timeline for the data!
   //
   int         nTracesInUse;                       //    to driver: # of data channels to "watch"
   int         iTraceType[CX_NTRACES];             //    to driver: type of data trace channel
   int         iTraceCh[CX_NTRACES];               //    to driver: channel # for data trace
   short       shTraceBuf[CX_NTRACES][CX_TRBUFSZ]; //    fr driver: the data trace buffers
   int         iTraceEnd;                          //    fr driver: next slot for putting new trace samples
   int         iTraceDrawn;                        //    to driver: next draw cycle starts with samples in this slot
   BOOL        bTraceOverflow;                     //    fr driver: error -- trace buffers overflowed.

   //
   // EYELINK TRACKER DATA. A worker thread running in Maesstro polls the Ethernet link to an external Eyelink 1000+
   // eye tracker using SR Research's Eyelink SDK. The thread loops at least once per millisecond to get the next
   // tracker sample and place it in the circular buffer here. CXDRIVER, in turn, pulls the tracker samples out
   // of the buffer. This is one example in which the data flow is from Maestro to CXDRIVER! Samples are available 
   // whenever iELNext != iELLast. Maestro sets the overflow flag and stops posting samples when (iELNext+1) % bufLen 
   // == iELLast; additional samples are lost. Also, Maestro increments iELNext *after* preparing the next sample. The 
   // buffer is large enough to hold a half-second worth of tracker samples, but we should never need that much.
   //
   // When an Eyelink recording session is in progress, there could occasionally be delays in the expected 1KHz sample
   // stream due to delays in the Ethernet link with the Eyelink host machine, or short delays in the worker thread that
   // polls the link queue. On the Win32 side, Maestro will terminate an Eyelink recording session (iELStatus is set to
   // CX_ELSTAT_FAIL) if there is an inter-sample delay exceeding 50ms. 
   //
   // The five Eyelink params in iELParams[] are: 0,1=X,Y offset; 2,3=X,Y gain; 4=velocity smoothing filter window 
   // width in ms. These are NOT used on the CXDRIVER side; they're here so they can be included in data file.
   //
   int iELStatus;                                  // to driver: Eyelink tracker status (see CX_ELSTAT_*** constants)
   int iELRecType;                                 // to driver: record type - monocular L/R or binocular
   int iELParams[5];                               // to driver: (info only) calib params and vel smoothing window width
   int iELNext;                                    // to driver: index of next tracker sample to consume
   int iELLast;                                    // fr driver: index of last tracker sample consumed
   ELSAMP elSamples[CX_MAXEL];                     // to driver: the tracker sample buffer (circular)

   //
   // DIGITAL EVENT DATA STREAM.  During presentation of a trial, CXDRIVER streams digital event data (event mask 
   // and time to Maestro using the circular buffers defined here.  Maestro consumes the event data to build a spike 
   // time histogram for any "tagged sections" found in the trials presented.  These histograms continue to accumulate 
   // event data until trial sequence stops.  The scheme for avoiding concurrent access by Maestro and CXDRIVER 
   // mirrors that used for the data trace display buffers.  However, these buffers are used only during execution of 
   // a trial.
   //
   // CXDRIVER timestamps digital events at 10us resolution.  However, Maestro does not require this resolution to
   // build the spike time histograms -- which are intended only to give the user 'rough-n-ready' feedback during
   // runtime.  The event timestamps reported here are trial times in milliseconds.
   //
   // NOTE that this facility provides timestamps for all digital inputs, not just DI channel 0, which is dedicated to
   // monitoring "spikes" timestamped by the hardware.
   //
   BOOL        bEventEnable;                       //    to driver: if TRUE, Maestro is accepting event data
   DWORD       dwEventMaskBuf[CX_EVTBUFSZ];        //    fr driver: digital event mask buffer
   int         iEventTimeBuf[CX_EVTBUFSZ];         //    fr driver: digital event timestamp buffer (units = 1ms)
   int         iEventEnd;                          //    fr driver: next event is put in this location in buffers
   int         iEventConsumed;                     //    to driver: last event consumed was at this location
   BOOL        bEventOverflow;                     //    fr driver: error -- event data buffers overflowed.

   //
   // COMMAND/RESPONSE FACILITY.  Maestro issues a variety of commands to CXDRIVER during runtime, using the 
   // fields listed in this section.  The different commands, and their format, are described above.  Most supported 
   // commands involve actions that CXDRIVER can reliably perform quickly, with the complete command/response 
   // handshake taking no more than ~300ms.  For these commands, it is safe for Maestro to block awaiting completion of 
   // the command. For a few commands, the response time can be much longer -- such long-latency commands are 
   // highlighted in the command descriptions above.
   //
   BOOL        bReqCmd;                            //    to driver: service new command
   BOOL        bAckCmd;                            //    fr driver: command has been serviced
   DWORD       dwCommand;                          //    to/fr driver: the command code; if failed, set to error code
   int         iData[CX_CMDLEN];                   //    to/fr drv: data associated with command/response
   float       fData[CX_CMDLEN];                   //
   char        cData[CX_CDATALEN];

   //
   // CURRENT TARGET DEFINITIONS.  Maestro is designed to store the definitions of potentially a great many distinct
   // targets.  However, when presenting a typical trial sequence or a continuous-mode run, only a small subset of
   // those targets participate.  That subset is referred to as the "current loaded target list".
   //
   // In TrialMode, this list is initialized just prior to the start of a trial sequence, and includes all targets
   // participating in any trial from that sequence.
   //
   // In ContMode, this list is used only to define targets in the "active target list".  Active targets are generally
   // used as fixation targets in ContMode, particularly during calibration of subject's eye position.  Their positions
   // and on/off states are controlled by the CX_CM_UPDACVTGT command.
   //
   // The current target list is always read-only to CXDRIVER. Maestro "guarantees" that it will only modify the 
   // list when no protocol is running on CXDRIVER, and CXDRIVER only accesses the list while executing a 
   // protocol. This should prevent any errors due to simultaneous access...
   //
   int         nTgts;                              //    to driver: the # of targets in the "loaded" target list
   CXTARGET    targets[CX_MAXTGTS];                //    to driver: the "loaded" target list

   //
   // TRIALMODE-SPECIFIC DATA.  Defining information about a Maestro trial and participating targets.  Current usage:
   //
   // 1) Current target list is filled before a trial sequence starts. 
   // 
   // [NOTE: With the old VSG framebuffer, the target list was prefilled with all targets used by ANY trial in the trial
   // set sequenced, and CX_TR_PRELOADFB was issued to preload the targets onto the VSG2/4 hardware, which took a while.
   // Now that RMVideo has replaced the VSG, preloading before the sequencing starts is no longer required. However, 
   // Maestro still fills the target list with all all targets that participate in any trial within the set sequenced.]
   //
   // 2) Before each trial is started, Maestro prepares a list of codes (see CXTRIALCODES.H) that completely defines
   // the trial and an accompanying "trial target map".  Participating targets are identified by their position in this
   // map which, in turn, points to their definitions in the loaded target list.  If the trial contains any tagged
   // sections, Maestro stores the section info here so that it can be saved in the data file.  When recorded trial
   // data is to be saved to a file, several fields in the "PROTOCOL INFO" section are also updated:  translation and
   // rotation factors used to transform target trajectories during trial code generation, full pathname of the trial
   // file in 'strDataPath' (if trial is NOT to be saved, this is NULL), and trial's name in 'strProtocol'.  Finally,
   // CX_SAVECHANS is issued to identify which ADC channels should be saved during trial, followed by CX_TR_START to
   // start the trial.
   //
   // 3) Once CXDRIVER starts running a trial, Maestro "guarantees" it will not change any of this info until the 
   // trial is completed. Likewise, CXDRIVER only accesses the target & trial info while it is running a trial. 
   // These "rules" ensure data integrity without the need for synch objects or handshake flags.
   //
   // 4) The "trial result" is a bit-flags field (see "PROTOCOL INFO") set by CXDRIVER upon completion (or 
   // termination) of the trial. The field is cleared (0) by CXDRIVER in response to the CX_TR_START command and 
   // remains so until the trial is over. Thus, a nonzero flag field signals Maestro that the trial is over and 
   // CXDRIVER has returned to the intertrial "idle" state of TrialMode.
   //
   int         nTrialTgts;                         //    to driver: # of targets participating in trial
   int         iTgMap[MAX_TRIALTARGS];             //    to driver: pos of trial target's defn in the loaded tgt list
   int         nCodes;                             //    to driver: # of trial codes defining the current trial
   TRIALCODE   trialCodes[CX_MAXTC];               //    to driver: the trial codes themselves
   int         nSections;                          //    to driver: # of tagged sections defined on current trial
   TRIALSECT   trialSections[MAX_SEGMENTS];        //    to driver: tagged section records
   char        strSet[CX_MAXOBJNAMELEN];           //    to driver: name of set to which trial belongs
   char        strSubset[CX_MAXOBJNAMELEN];        //    to driver: name of subset, if any, to which trial belongs

   //
   // CONTMODE-SPECIFIC DATA.
   //
   CONTRUN     runDef;                             //    to driver: defn of the "current" stimulus run

   //
   // PROTOCOL INFO/RESULTS.  Information to/from CXDRIVER that's used in Trial and/or ContMode.
   //
   // The field 'dwResult' is used differently in Trial vs Cont mode. In Trial mode, CXDRIVER clears the field 
   // when the trial begins, sets it when the trial is done, and leaves it alone otherwise. The field is read-only to
   // Maestro. In Cont mode, CXDRIVER uses the field to report state information on a continuing basis. Maestro 
   // must watch this field for state changes (stimulus run started/stopped, recording on/off, etc) and update itself
   // accordingly.
   //
   DWORD       dwResult;                           //    fr driver: trial results in Trial mode; op state in Cont mode
   int         iNumRewards;                        //    fr driver: total #rewards delivered since last reset by host
   int         iTotalRewardMS;                     //    fr driver: cumulative reward delivered in ms since last reset
                                                   //       by host side
   int         iLastTrialLen;                      //    fr driver: elapsed time of last trial presented (ms)
   float       fResponse;                          //    fr driver: behavioral response measured for an "RP Distro"
                                                   //       trial. Valid only if CX_FT_GOTRPDRESP result bit is set.
                                                   //       Units depend on type of behavioral response measured.

   char        strDataPath[CX_MAXPATH];            //    to driver: full path for data file; if NULL, data file is not
                                                   //       saved for current trial or stimulus run
                                                   //       AT STARTUP: Holds Maestro installation directory.
   char        strProtocol[CX_MAXOBJNAMELEN];      //    to driver: name of trial or stimulus run
   BOOL        bTolRMVDuplFrame;                   //    to driver: if TRUE, tolerate up to 3 duplicate RMVideo frames
                                                   //       trial; if FALSE or more than 3 dupes, abort trial.
   float       fPosScale;                          //    to driver: factors used to translate & rotate the trajectories
   float       fPosRotate;                         //       of targets during a trial. these must be saved to the
   float       fVelScale;                          //       trial data file along with trial codes to reconstruct what
   float       fVelRotate;                         //       happened during the trial
   float       fStartPosH;
   float       fStartPosV;
   
   DWORD       dwTrialFlags;                       //    to driver: flag bits in definition of last trial presented.
                                                   //       Saved in trial data file header.

   //
   // OTHER SIGNALS AND INFO FROM Maestro
   //
   BOOL        bSaveSpikeTrace;                    //    to driver: if TRUE, 25KHz spike waveform is recorded
   BOOL        bChairPresent;                      //    to driver: if FALSE, animal chair is ignored
   int         iDay;                               //    to driver: the current day of the month in [1..31]
   int         iMonth;                             //    to driver: the current month of the year in [1..12]
   int         iYear;                              //    to driver: the current 4-digit year
   int         iVStabSlidingWindow;                //    to driver: length of sliding window avg of eye pos to
                                                   //       smooth VStab effects in TM (milliseconds)
   float       fDOBusyWaits[3];                    //    to driver: busy waits (us) for SetDO() [see file header]

   //
   // HARDWARE STATUS INFO FROM CXDRIVER
   //
   DWORD       dwHWStatus;                         //    hardware status flags
   int         nAOChannels;                        //    # of AO channels available 
   int         nAIChannels;                        //    # of AI channels available 
   int         nTDOChannels;                       //    # of DO channels available on event timer device
   int         nTDIChannels;                       //    # of DI channels available on event timer device

   // process ID assigned to the CXDRIVER RTSS process when it was spawned by Maestro. At startup, before it launches
   // CXDRIVER, Maestro will check for an "orphaned" CXDRIVER process (this can happen if Maestro is terminated 
   // unexpectedly) by looking for this shared IPC memory object CXIPC_SHM. If that object exists, it will try to 
   // access this process ID, obtain a handle to the orphaned process, and terminate it. NOTE that this ID is 
   // different from the notion of "RTSS process slot" that applied to 32-bit RTX.
   DWORD dwProcessID;
} CXIPCSM, *PCXIPCSM;



#endif   // !defined(CXIPC_H__INCLUDED_)
