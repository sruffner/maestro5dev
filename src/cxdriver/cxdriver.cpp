/**=====================================================================================================================
 cxdriver.cpp : Implementation of class CCxDriver, the the RTX64-based hardware and experiment controller for Maestro.

 AUTHOR:  saruffner

 DESCRIPTION:

 CREDITS:
 1) Real-Time eXtension (RTX) to Windows by IntervalZero (www.intervalzero.com). The cxdriver.rtss app is designed to 
 run as an "RTSS" process within the RTX64 subsystem. RTX64 gives Windows real-time-like characteristics and provides 
 kernel access for direct communications with hardware -- obviating the need to write kernel-mode device drivers.


 REVISION HISTORY:
 29mar2001-- Began development.
 01may2001-- Continued development of "test & calibration" mode.
 10may2001-- Begun conversion to C++.
 17may2001-- The C-style TIMERBRD module replaced by the CCxEventTimer interface.
 18may2001-- The C-style DACBOARD module replaced by the CCxAnalogOut interface.
 21jun2001-- Major mods to implement redesign of test mode functionality and introduction of "command/response"
    communication framework between CNTRLX and CXDRIVER.
 29jun2001-- Added diagnostic test facilities for event timer device in RunTestMode().
 25jul2001-- The C-style ADCBOARD module replaced by the CCxAnalogIn interface.
 02aug2001-- Added feature to TestMode:  continuous-play of 1Hz sine wave "test waveform" on a selected AO channel.
 10aug2001-- Implemented data trace facility in TestMode.
 06jan2002-- Modified to handle new CNTRLX command CX_SAVECHANS.
          -- Copied over CNTRLX98 TrialMode methods RunTrialMode() and ExecuteSingleTrial() and began editing them to 
    fit into the CXDRIVER framework...
 13feb2002-- Completed an initial version of TrialMode implementation, though with various details yet to be 
    implemented.  Next up:  build & test & evaluate performance of trial runtime loop.
 26mar2002-- TrialMode appears to be working!  Continuing to test & refine.
          -- New option flag communicated by CNTRLX and accessed via CCxMasterIO::IsChairPresent().  If cleared, we
    assume the animal chair is not present in the system.
 02apr2002-- Working to integrate spikesPC functionality into CXDRIVER...
 18apr2002-- SpikesPC functionality up & running in TrialMode.  No need really to save spike trace data to a separate
    file -- modifying CCxDriver accordingly.
 18jun2002-- Introduced device manager object CCxDeviceMgr which handles instantiation and destruction of the device
    objects CCxEventTimer, CCxAnalogIn, and CCxAnalogOut.  Eventually, all hardware interfaces will be designed as 
    abstract interfaces with device-specific realizations, and CCxDeviceMgr will handle all device objects...
 01aug2002-- Copied over CNTRLX98's RunContinuousMode() method and began modifying it to work under CXDRIVER...
 06aug2002-- CCxAnalogOut now encapsulates CX_FIBER* and CX_CHAIR control. To move these targets, CCxDriver invokes the
    appropriate CCxAnalogOut method, specifying pos or vel in deg or deg/sec. The idea here is to isolate CCxDriver from
    the details of AO channel assignments, converting deg to b2sAOVolts, etc.
 14aug2002-- Replaced the m_vbFrameShift variable with m_viElapsedTicks, the # of "ticks" (aka, interrrupts from the AI 
    device) that have elapsed since the DAQ was last started.  This formulation still allows us to detect a single-frame
    shift during a trial; in addition, it allows us to detect when more than one scan set is ready in the AI FIFO.
 21oct2002-- Added certain reward options and separate H & V fixation accuracies, delivered by CX_FIXREWSETTINGS cmd
    and stored in CFixRewSettings structure -- replacing CX_CM_UPDFIXREQ and CFixRequirements struct.  Added command 
    CX_CM_UPDFIXTGTS, which applies only to ContMode.  Note that CX_FIXREWSETTINGS should be handled in Idle, Trial, or 
    ContModes (but not during a trial).
 01nov2002-- Discovered a long ISR latency issue on my PC. Adding debug code in an attempt to determine the source.
 18nov2002-- Finally determined that latency issue was due to a problem with RTX4.3. Fixed by installing RTX 4.3.1.
    However, I decided to add a CElapsedTime object to detect long ISR latencies in the future...
 19dec2002-- Modified ExecuteSingleTrial() to support perturbations, using perturbation mgr obj CCxPertHelper.
          -- Mod to RunContinuousMode(): Got rid of code that delayed command processing while the chair settled back to
    zero following a stimulus run. For now, we'll assume that CNTRLX and/or the user waits long enough for chair to get 
    back to zero.  If necessary, we may have to introduce some scheme by which CNTRLX verifies that the chair is back to
    zero...
 09jan2003-- Began integrating CCxScope device into CCxDriver, replacing the old C-style XYSCOPE module...
 24jan2003-- Integrated CCxServo device into CCxDriver, replacing old C-style MOTCNTRL module...
 12mar2003-- Modified to store trial target (TrialMode) or active target & XYseq target (ContMode) definitions in data 
    files using the CX_TGTRECORD record tag.  Format is distinct from a similar record type (ID=64) created by cxUNIX. 
    Stimulus run definition will be stored in a separate record type, CX_STIMRUNRECORD.
 23apr2003-- Integrated CCxFrameBuf device into CCxDeviceMgr and CCxDriver.  Also updated CCxDriver IAW additional
    changes in CCxDeviceMgr:  1) Device object pointers are exposed via CCxDeviceMgr::GetAI(), etc.  2)
    OpenHardwareResources() now calls CCxDeviceMgr::Startup() to open connections to all available devices.
 21jul2003-- Mod to support new XY scope target type NOISYDOTS, which is similar to FCDOTLIFE.
 25jul2003-- Changed fixation checking logic in ContMode so that we reward the animal when it fixates on a phantom
    target that has been turned off (for "VOR in the dark" and similar experiments).
 07oct2003-- Mod UpdateLoci() to report a "secondary eye" position using AI channel HGPOS2 for horiz pos and, for now, 
    VEPOS2 == 0 always.  We may dedicate another AI channel to VEPOS2 in the future.
 23oct2003-- Mod ExecuteSingleTrial() to support velocity stabilization (the "open loop" feature) of the H component of 
    motion only, the V cmpt only, or (as was previously the case) both components.  See CXTRIALCODES.H for the required 
    changes to the TARGET_HOPEN trial code group.
 04nov2003-- Mod ExecuteSingleTrial() and RunContinuousMode() to ensure we do not trigger marker pulses more frequently 
    than every 900us.
 05nov2003-- Mod ExecuteSingleTrial() to provide some generic information for synchronizing with an external app: record 
    "start" and "stop" marker pulses delivered on dedicated DO<11>; record "start", "stop", and "abort" character codes 
    written via the timer's "character writer" latched device. We also write the trial's name and the data filename. 
    RunContinuousMode() similarly modified to provide info about data recorded in that mode (of course, no trial name is
    provided).
 01mar2004-- Bug fix of V-only velocity stabilization in ExecuteSingleTrial().
 10mar2004-- Mod ExecuteSingleTrial() so that XY scope targets, like Fiber1/2, move while they're turned off.
 07jul2004-- Mod ExecuteSingleTrial() to report trial's length via CCxMasterIO::SetLastTrialLen().
 13oct2004-- Mods in ExecuteSingleTrial() and RunContinuousMode() to support new XY target type COHERENTFC, which is 
    similar to FASTCENTER except that only a percentage of dots move coherently.
 03nov2004-- Mod ExecuteSingleTrial() to introduce new sacc-trig'd op, "Dual Fix".
 04jan2005-- Fixed bug in mid-trial reward feature in ExecuteSingleTrial() -- reward pulse was continuously on b/c I had
    failed to reset the counter after each mid-trial reward was delivered.
 25jan2005-- Mods in ExecuteSingleTrial() to enhance mid-trial reward feature and to change implementation of the
    "Switch Fix" special op, formerly know as "Dual Fix".
 10mar2005-- Mods to what info we send to synch with an external app: added "lostFix" and "dataSaved" codes. Now "start" 
    and "stop" always bracket the epoch; and "abort" is issued only if an error occurs. The "lostFix" code only applies 
    in Trial mode and indicates that the animal broke fixation during trial. The "dataSaved" code indicates that data 
    file was successfully saved. Also, in Trial mode only, a "noDataFile" code appears in place of the trial data file 
    name when data was intentionally NOT saved. The dedicated DOUT11 marker pulses are now delivered at the very 
    beginning and end of the trial; before this, the first pulse was not delivered until recording was turned on!
 29mar2005-- Increased large buffers to handle 30-second trials.  MAX_SEGMENTS now 30 instead of 20.
 06apr2005-- Added support for "tagged sections" in a trial.  For now, SaveTrialDataFile() simply copies any TRIALSECT 
    structs in CXIPC into a new record, CX_TAGSECTRECORD, that's stored in the data file. The "tagged sections" feature 
    is intended to assist parsing of trials by analysis programs.
 15jun2005-- Mods to support an additional locus, CX_TRACK, in the eye-target plot. This displays position of the
    "cursor tracking" target, and it is relevant only to Cont mode.
 10aug2005-- ADC ch11 now dedicated as vertical eye pos signal for the "second" eye locus. UpdateLoci() modified
    accordingly. NOTE: The "second" eye is NOT used for fixation checking at all!
 16sep2005-- Mod to UpdateStimulusRun() to implement the (H,V) position offset for a stimulus run. The offset is only 
    applied to Fiber1/2 stimulus channels.
          -- Mod to ExecuteSingleTrial() so that velocity stabilization adjusts both pattern motion and window motion
    when the affected target is an XY scope target.
          -- Mod to ExecuteSingleTrial() to set CX_FT_NORESP results flag bit if instructed to check subject's response, 
    yet no response was detected, correct or otherwise (for staircase trials only).
 04jan2006-- Mods to support new "R/P Distro" protocol in Trial mode...
 10jan2006-- Mod to support new XY scope target type NOISYSPEED, which is similar to NOISYDIR (formerly NOISYDOTS).
 13mar2006-- Mod to ExecuteSingleTrial() to support velocity stabilization over a contiguous span of trial segments.
 14mar2006-- As of Maestro v1.5.0: Completely removed support for the OKNDRUM motion control servo platform!
 27mar2006-- Begun major changes for Maestro 2.0.0:  RMVideo replaces the VSG. RMVideo targets are updated in a manner 
    similar to the XYScope, except that MAESTRODRIVER must stay a full video frame ahead of RMVideo.
 12apr2006-- Complete rework of the velocity stabilization feature, making it completely general:  Any trial target may 
    be stabilized during any segment or segments of the trial.
 24apr2006-- Mod OpenHardwareResources() to expose some static RMVideo display properties via fields in CXIPC.
 11dec2006-- Mod RunContinuousMode() to support pattern motion (constant velocity vector) for any active target that 
    possesses an extended tgt pattern that can move independently of target window. The CX_CM_UPDACVTGT cmd was updated 
    to include pattern speed and direction as additional active tgt parameters.
 27feb2007-- Mod ExecuteSingleTrial() to support per-segment pattern ACCELERATION in H & V. New trial codes 
    INSIDE_***ACC are sent by Maestro to specify the pattern acceleration cmpts.
 25apr2007-- Mod ExecuteSingleTrial() to support 4 different behavioral response measures for RP Distro feature: eye 
    vector velocity magnitude in deg/s, H eye velocity in deg/s, V eye velocity in deg/s, or eye vector velocity 
    direction in deg CCW (0deg = rightward motion). The response type is one of the defined constants TH_RPD_*** in 
    cxobj_ifc.h [0..3], is bitwise-OR'd with the SPECIAL_RPDISTRO optype after left-shifting 8 bits.
 28apr2010-- Mod ExecuteSingleTrial() to save global target starting position offset (h & v) to data file header.
 24may2010-- Mod ExecuteSingleTrial() to store trial flag bits in data file header.
 01feb2011-- Mod ExecuteSingleTrial() to implement new "search task" feature (TH_SOP_SEARCH).
 01mar2011-- Add'l changes in ExecuteSingleTrial() to refine the new "search task" feature.
 11may2011-- XYScope target pattern velocities are now specified WRT the target window's reference frame rather than
    the XYScope screen. ExecuteSingleTrial() and RunContinuousMode(), along with CCxScope and XYCORE, have been updated 
    accordingly.
 21jun2011-- Begun major changes for Maestro 3: Introduced PCIe-6363 to handle AI, AO, and DIO timer functions all on
    a single board. Eliminated support for FIBER1/2 and REDLED1/2. The only non-video target platform supported is now
    the animal chair (CX_CHAIR). Minor changes were needed so that module would build under Visual Studio 2010 and the
    RTX2011 SDK.
 19sep2011-- Completed major changes for initial release of Maestro 3.
 27sep2011-- Modified RunTestMode() to compute running avg and standard deviation of the recorded signal on each AI
 channel while DAQ is running. CX_TM_GETAI response modified to report (last sample, avg, std dev) for each channel.
 05jan2012-- Major mods so that trial target trajectories are computed on the fly during runtime rather than being 
 precomputed. This eliminated a number of dynamically allocated buffers that were members of CCxDriver, so 
 Allocate/FreeMemoryResources were changed accordingly. Testing...
 15feb2012-- Major mod so that trial data is streamed to file as the trial proceeds, rather than being stored in large
 buffers. This eliminated the need for such buffers. Along with the changes dtd 05jan2012, it also eliminated the 30-sec
 restriction on trial length. There is still a practical limit on trial length, because trial codes use a short to
 indicate elapsed trial time in ms, so max trial length is 32767 ms!!!. Not a big deal practically speaking, since
 most researchers use trials that are quite short in duration.
          -- In the process of testing these changes, we ran into problems with RMVideo that ultimately had to do with
 send delays in the RT-TCP/IP stack. Following recommendations from IntervalZero, I increased the stack's # of receive
 and transmit buffers from 16 to 48, and realigned the priorities of the stack's timer, interrupt, and receive threads 
 so that they were greater than the priorities of MaestroDRIVER's main worker and file writer threads. Other technical
 changes were made to CRtSuspendMgr and CCxRMVideo in the course of addressing the issues. After all these changes, I
 successfully presented 1300 trials without stopping over 3 hours and no errors occurred.
 08feb2013-- Fixed a regression bug introduced by mods dtd 05jan2012 (on-the-fly computation of target trajectories).
 An RMVideo fixation target's position was changing one RMVideo frame before it should have, and this led to a trial
 aborting on a fixation break near the end of a segment when the fixation target position was to change abruptly in the
 following segment.
 04sep2013-- Minor changes in behavior when a trial terminates prematurely -- see ExecuteSingleTrial() for details.
 07nov2017-- Mods to fix compiler issues in VS2017 for Win10/RTX64 build.
 11sep2018-- Implemented the "reward indicator beep" on the Maestro side using Windows default system sound. 
 MaestroDRIVER requests the beep by logging the message "beep". This change was necessary because during testing we 
 discovered that, on 3 of 5 Win10 systems, playing the beep on the system speaker via CRtSpeaker caused frequent "AI
 ISR latency too long" errors. This was not due to thread conflicts, but to execution delays in the speaker port IO --
 due to bus hijacking -- likely by the PCI standard ISA bridge chip which connects legacy devices like the system 
 speaker to the PCIe bus. Modern workstations lack both ISA and PCI and require such bridge chips in order to integrate
 legacy devices. Class CRtSpeaker removed from MaestroDRIVER code base.
 24sep2018-- Mod response to CX_SETDISPLAY command to include 3 additional RMVideo params governing a vertical sync 
 "spot flash" that may be delivered during frame-by-frame animation sequence: spot size and margin in pixels; flash
 duration in # video frames. See UpdateVideoDisplays().
 25sep2018-- Mod CX_SETDISPLAY again, removing the margin parameter for the vertical sync spot flash feature.
 01oct2018-- RMVideo vertical sync spot size and flash duration are now saved to the data file header (TM only) -- if
 the RMVideo display is used (which as of Maestro 4.0.0 will always be the case).
 20mar2019-- Mods to ExecuteSingleTrial() and RunContinuousMode() IAW with changes in CCxRMVideo. Added support to 
 tolerate up to 3 duplicate frames during a trial, if new flag in CXIPC is set (see CCxMasterIO). See the two function
 descriptions for more details.
 20may2019-- Added comments in ExecuteSingleTrial() defining a zero reward pulse length (REWARDLEN trial code) as a 
 withheld reward -- to implement per-trial random reward withholding variable ratio feature for Maestro V4.1.0. Only
 code change is in CCxEventTimer::DeliverReward().
 02sep2019-- Mod to ExecuteSingleTrial: Moved delivery of RECORDMARKERPULSE so that it happens just before starting the
 trial's DAQ sequence. Changes to SetDO() to address missing marker pulse issue in Joshua lab. See CNI6363_DIO::SetDO().
 05sep2019-- Mod to OpenHardwareResources(): Three busy wait times specified by Maestro in CXIPC are communicated to 
 the DIO event timer via CCxEventTimer::SetDOBusyWaits(). This provides a mechanism by which the user can ultimately
 have some low-level control over the busy waits after each step in CCxEventTimer::SetDO(). See CCxEventTimer for
 details and rationale.
 24sep2024-- While support for the XYScope platform was dropped in Maestro 4.0, all XYScope-related code remained in
 cxdriver.*. Began removing all such code for Maestro 5.0. Various typedefs/structs/constants related to XYScope may
 remain...
 18nov2024-- Dropping support for: (1) The PSGM module, which was never actually built and put into use (except for
 testing a prototype). (2) Writing stimulus run definitions into the data file. Stimulus runs are rarely if ever used,
 and now that XYScope and PSGM support are dropped, the only available stimulus channel type uses the animal chair,
 which may not be available in any active rigs!
========================================================================================================================
*/

#include <WinSock2.h>                  // Don't want windows.h to include winsock.h, which collides with WinSock2.h !!
#include <windows.h>                   // standard Win32 includes
#include <stdio.h>                     // runtime C/C++ I/O library
#include <math.h>                      // runtime C/C++ math library
#include "rtapi.h"                     // the RTX API
#include "cxipc.h"                     // MaestroGUI/MaestroRTSS interprocess communications (IPC) interface
#include "cxdriver.h"


// The global MaestroRTSS "application" object
CCxDriver theDriverApp; 

/**
 main(): This is an extremely simple entry point for the MaestroRTSS process. It merely wraps a call to CCxDriver::Go(), 
 which represents the primary thread of the application.
*/
void main() { theDriverApp.Go(); }


LPCTSTR CCxDriver::WORKING_MUTEX = "cxdriver_working_mutex";

// 15feb2012(sar):
// RTX priorities assigned to important threads in driver. Set to be lower than IST/receive threads of RT-TCP/IP stack
// on recommendation of IntervalZero. Cxdriver's IST handler still runs at max priority, and timer handlers for the
// suspend managers and speaker beep run at max-1 priority.
const int CCxDriver::WORKER_PRIORITY = 50;
const int CCxDriver::FILEWRITER_PRIORITY = 45;

// important calibration factors. These assume 12-bit ADC and linear relationship between ADC code and voltage!
const float CCxDriver::POS_TOAIRAW = 40.0f;
const float CCxDriver::VEL_TOAIRAW =  10.88260708f;

// various fixed scan, sample intervals
const int CCxDriver::TRIALSCANINTVUS = 1000; 
const int CCxDriver::CONTSCANINTVUS = 2000;
const int CCxDriver::SPIKESAMPINTVUS = 40;

// minimum interval between triggered marker pulses
const double CCxDriver::MIN_MARKERINTVUS = 900.0; 

// various constants related to Maestro timeline synchronization w/external app (Plexon)
const DWORD CCxDriver::RECORDMARKER_MASK = ((DWORD) (1<<11));     // DO ch for record "start" and "stop" pulses
const char CCxDriver::START_CHARCODE = 0x02;                      // generic "start" char code
const char CCxDriver::STOP_CHARCODE = 0x03;                       // generic "stop" char code
const char CCxDriver::ABORT_CHARCODE = 0x0F;                      // generic "abort" char code
const char CCxDriver::LOSTFIX_CHARCODE = 0x0E;                    // "lostFix" char code (Trial mode only)
const char CCxDriver::NOFILE_CHARCODE = 0x07;                     // "noDataFile" char code (Trial mode only)
const char CCxDriver::DATASAVED_CHARCODE = 0x06;                  // generic "dataSaved" char code


/**
 Construct the MaestroRTSS application object and initialize it to the non-running state
*/
CCxDriver::CCxDriver()
{
   m_nSavedCh = 0;
   for( int i = 0; i < CX_AIO_MAXN; i++ ) m_iChannels[i] = -1;

   memset( &(m_shSlowBuf[0]), 0, CX_AIO_MAXN*2*sizeof(short) );
   m_pshLastScan = &(m_shSlowBuf[0]);
   memset( &(m_shFastBuf[0]), 0, CX_FASTBFSZ*sizeof(short) );
   m_nFast = 0;

   m_vbInterruptPending = FALSE;
   m_viElapsedTicks = 0;
   m_viScanInterval = 0;
   m_viPlotUpdateMS = 0;
   m_viFixChkMS = 0;
   m_vbStimOn = FALSE;
   m_viStimTicks = 0;
   m_viStimDutyCycle = 0;

   m_viServicedTicks = 0;
   m_vbFrameLag = FALSE;
   m_vbDelayedISR = FALSE;

   m_nFastBytes = 0;
   m_nSlowBytes = 0;

   m_bFixOn = FALSE;
   m_fixRewSettings.iDur = 1500;                                  // startup fixation/reward settings
   m_fixRewSettings.iRewLen1 = 25;
   m_fixRewSettings.iRewLen2 = 25;
   m_fixRewSettings.iWHVR = 1;
   m_fixRewSettings.iAudioRewLen = 0;
   m_fixRewSettings.iFix1 = -1;
   m_fixRewSettings.iFix2 = -1;
   m_fixRewSettings.iTrack = -1;
   m_fixRewSettings.bPlayBeep = FALSE;
   m_fixRewSettings.fPtAccuracy.Set( 2.0, 2.0 );
}

/**
 Destructor. Nothing to do here because application object state is created and freed in CCxDriver::Run()...
*/
CCxDriver::~CCxDriver()
{
}


/**
 Go(): As the "pseudo" entry point for MaestroRTSS, this method represents the primary thread of execution. It starts
 MaestroRTSS's runtime engine as a secondary thread, among other startup tasks:

 (1) Creates and claims a "stop" mutex that is held by MaestroRTSS as long as it is alive.  MaestroGUI opens a handle to
 this mutex shortly after spawning MaestroRTSS. It serves as a signal to MaestroGUI that MaestroRTSS has died.
 (2) Opens interprocess communications (IPC) with MaestroGUI.
 (3) Spawns the "runtime engine" thread that does all the work of MaestroRTSS. See CCxDriver::Run().
 (4) Spawns a suspension  management thread (CRtSuspendMgr) that periodically suspends the runtime engine so that it 
 does not starve Windows. With this scheme, we no longer need to disperse RtSleepFt() statements throughout the code to
 achieve starvation management. Obviously, the suspender thread must have a higher RT priority than the runtime engine.
 (5) Opens a handle to a second mutex, owned by the runtime engine thread, that becomes signaled when that thread dies. 
 The primary thread then waits indefinitely on this mutex -- so it wakes up only after the worker thread has died, at 
 which point it performs some clean up and exits.
 (6) Allocates thread & memory resources to the "file writer" object which is used to write data records to file "on the
 fly" during ContMode. We do this here rather than in the runtime engine thread merely to keep all thread creation and 
 priority assignment code in one location. Observe that the file writer thread is lower in priority than the runtime 
 engine thread, and its duty cycle is 10ms/8ms suspended. Thus, given that the runtime engine's duty cycle is 2ms/400us 
 suspended, the file writer thread will only get ~400us out of every 10ms of CPU time. Tests have shown that this is 
 adequate for the worst-case data stream requirements in ContMode.

 Once step (2) is completed successfully, MaestroRTSS can post status/error messages to MaestroGUI via the IPC 
 construct.  See CCxMasterIO::Message() for details.

 NOTE:  Limitations in the RTX API force us to resort to this awkward scheme: RTSS processes and threads cannot serve as
 synchronization objects, so MaestroGUI cannot wait on MaestroRTSS's process handle (in fact, the RTSSRUN utility which 
 starts an RTSS process does not even make the process handle available), nor can this primary thread wait on the worker
 thread's handle. We MUST use mutexes in both cases. Furthermore, an RTX thread cannot use suspend managment on ITSELF 
 because the RTX API does not allow it to obtain a copy of a "real" handle (vs the pseudohandle returned by 
 GetCurrentThread()) to itself. Hence, we must spawn a secondary thread to do all the work of MaestroRTSS, and pass its 
 real handle to the suspend manager, which is itself a secondary thread (an RTX timer thread).
*/
VOID RTFCNDCL CCxDriver::Go()
{
   // the runtime engine worker, mutex held by work until it dies, mutex held by this process until it dies
   HANDLE hWorker = (HANDLE)NULL, hWorkerMutex = (HANDLE)NULL, hStopMutex = (HANDLE)NULL;
   // flag set if worker thread terminated normally
   BOOL bWorkerDone = FALSE; 

   // error message in abnornal exit on startup; nonzero length indicates startup error
   char szErrMsg[100];
   ::strcpy_s(szErrMsg, ""); 

   // compiler limitation required I declare all variables before the GOTOs...
   int i; 
   DWORD dwID;
   
   // create and claim mutex that will signal when this process terminates. Abort on  failure.
   hStopMutex = ::RtCreateMutex(NULL, TRUE, CXIPC_STOPMUTEX);
   if(hStopMutex == (HANDLE) NULL) goto CLEANUP;

   // open IPC w/MaestroGUI; abort on failure. If OK, we can post startup progress messages via IPC.
   if(!m_masterIO.Open()) goto CLEANUP;

   // create runtime engine worker thread in suspended state. Abort on failure.
   hWorker = ::CreateThread(NULL, 0, CCxDriver::RunEntry, (LPVOID)this, CREATE_SUSPENDED, &dwID);
   if(hWorker == (HANDLE) NULL)
   {
      ::sprintf_s(szErrMsg, "(!!)Cannot start a thread (0x%08x)", ::GetLastError());
      goto CLEANUP;
   }

   // set this thread's priority higher than the worker's
   ::RtSetThreadPriority( ::GetCurrentThread(), CCxDriver::WORKER_PRIORITY ); 
   ::RtSetThreadPriority( hWorker, CCxDriver::WORKER_PRIORITY ); 

   // start worker under suspend management. Suspend mgr gets even higher priority.
   if(!m_suspendMgr.Start(hWorker, RT_PRIORITY_MAX-1))
   {
      ::sprintf_s(szErrMsg, "(!!)Suspend mgt thread failed (0x%08x)", ::GetLastError());
      goto CLEANUP;
   }

   // give runtime engine up to ~200ms to create and claim mutex it holds throughout its lifetime
   i = 0; 
   while((hWorkerMutex == (HANDLE)NULL) && (i++ < 200)) 
   {
      hWorkerMutex = ::RtOpenMutex(SYNCHRONIZE, FALSE, CCxDriver::WORKING_MUTEX);
      ::Sleep(1);
   }
   if(hWorkerMutex == (HANDLE) NULL)
   {
      ::sprintf_s(szErrMsg, "(!!)Failed to sync w/worker thread (0x%08x)", ::GetLastError());
      goto CLEANUP;
   }

   // allocate resources for file writer object. The writer's worker thread has a RT priority less than that of the
   // runtime engine, and is suspended 8ms of every 10. It has a 30KB internal queue for buffering writes.
   if(!m_writer.AllocateResources(CCxDriver::FILEWRITER_PRIORITY, 2000, 8000, 30))
   {
      ::sprintf_s(szErrMsg, "(!!)Unable to get file writer resources");
      goto CLEANUP;
   }

   // success. Now the primary thread just waits until the runtime engine thread dies!
   ::RtWaitForSingleObject(hWorkerMutex, INFINITE);
   bWorkerDone = TRUE;

CLEANUP:
   // kill suspend management
   m_suspendMgr.Stop(); 

   // if startup failed, post error message to MaestroGUI if possible.
   if(::strlen(szErrMsg) > 0) m_masterIO.Message(szErrMsg);
   
   // free file write resources and clean up after runtime engine thread; close IPC w/MaestroGUI
   m_writer.FreeResources(); 
   if(hWorker != (HANDLE) NULL) 
   {
      if(!bWorkerDone) ::TerminateThread(hWorker, 0);
      ::CloseHandle(hWorker);    // DON'T use RtCloseHandle() on a thrd handle!
   }
   if(hWorkerMutex != (HANDLE) NULL) 
   {
      ::RtReleaseMutex(hWorkerMutex);
      ::RtCloseHandle(hWorkerMutex);
   }
   m_masterIO.Close(); 

   // release stop mutex, indicating that the MaestroRTSS process has died, then exit (we don't use exit code).
   if(hStopMutex != (HANDLE) NULL)
   {
      ::RtReleaseMutex(hStopMutex);
      ::RtCloseHandle(hStopMutex);
   }
   ExitProcess(0);
}


/**
 Run(), RunEntry(): Entry point for worker thread procedure that handles all functionality of MaestroRTSS: Initializes 
 and controls hardware resources. Runtime logic and execution engine.

 MaestroGUI defines experimental protocols via IPC and requests that MaestroGUI execute those protocols; it also 
 provides GUI "services" to MaestroRTSS -- for displaying error messages, acquired data, and eye/target position. This 
 "runtime engine" thread of MaestroRTSS, in turn, handles all other aspects of the application: hardware communications,
 data manipulation, writing acquired data to disk file, time-critical runtime loops, etc.

 This method itself handles only startup and shutdown, and operational mode switches. The bulk of the real work is 
 handled by the operational mode control methods, Run****Mode(). The method aborts without entering Idle Mode if it 
 cannot find or initialize the minimum required hardware resources (AI, DIO timer).

 HACK: The thread entry point must be a static method, and static class methods do not get the implied THIS argument.
 Thus, Run() would not be able to access the non-static class members. To get around this, the static inline method 
 RunEntry() instead serves as the thread entry point. When spawned in Go(), it is passed a THIS ptr in its context 
 argument, which it then can use to invoke the non-static Run() method!
 
 @return Exit code (0 always -- not used).
*/
DWORD RTFCNDCL CCxDriver::Run()
{
   int iMode;

   // create and claim mutext that is held until this thread terminates -- to signal the primary thread in Go().
   HANDLE hAliveMutex = ::RtCreateMutex(NULL, TRUE, WORKING_MUTEX);
   if(hAliveMutex == (HANDLE) NULL)
   {
      m_masterIO.Message("(!!)Cannot create MT sync mutex");
      goto CLEANUP;
   }

   // init elapsed time since CXDRIVER started
   m_eRunTimeUS.Reset();

   // find and initialize all installed hardware
   if(!OpenHardwareResources()) goto CLEANUP; 

   // MODE-SWITCHING LOOP. We always start in Idle Mode.
   m_masterIO.SetMode(CX_IDLEMODE);
   iMode = CX_IDLEMODE;
   do
   {
      switch(iMode)
      {
         case CX_IDLEMODE :   RunIdleMode(); break;
         case CX_TESTMODE :   RunTestMode(); break;
         case CX_TRIALMODE:   RunTrialMode(); break;
         case CX_CONTMODE :   RunContinuousMode(); break;
         default          :   m_masterIO.Message("(!!)Unrecognized op mode - switching to idle!");
                              m_masterIO.SetMode(CX_IDLEMODE);
                              break;
      }
      iMode = m_masterIO.GetMode();
   } while(iMode >= CX_IDLEMODE);

CLEANUP:
   // here we do the exact reverse of the startup operations above...
   m_masterIO.Message("SHUTTING DOWN....");
   CloseHardwareResources(); 
   if(hAliveMutex != (HANDLE) NULL)
   {
      ::RtReleaseMutex(hAliveMutex);
      ::RtCloseHandle(hAliveMutex);
   }
   return(0);
}


/**
 Start the device manager, which creates device function "objects" for all Maestro-related hardware resources in the 
 host system. As of Maestro 4.0, there are really only two supported devices: the PCIe-6363 multifunction DAQ board
 providing AI, AO, and event timer DIO functionality in one device; and an interface to the RMVideo application, which 
 runs on a separate workstation. If no physical device is found for a device class, the device manager creates a 
 "placeholder" representing the absence of that device.
 
 The device manager is responsible for finding and acquiring the physical devices. The runtime engine accesses the
 hardware functionality only through abstract device interfaces (for AI, AO, DIO timer, etc); it has no knowledge of the
 actual hardware on which these functions are realized. See CCxDeviceMgr.
 
 Hardware is "registered" with the Maestro-CXDRIVER RTSS IPC object to tell MaestroGUI whether or not a given device 
 function is available. If no devices are found, CXDRIVER will shut down (this call returns FALSE). Note that runtime 
 operation (trial or continuous mode) is not possible without both the AI and DIO timer functionality; test and calib 
 mode is available as long as one of the AI, AO, or DIO timer functions is available.

 If the AI device supports "fast calibration" (eg, loading the board's calibration constants from non-volatile EEPROM),
 that calibration happens here during startup. This was important when Maestro used the PCI-MIO16E1 for AI -- since it
 is essential that its calibration constants be reloaded whenever the host system was power-cycled. Not applicable to
 the PCIe-6363.
  
 Warning/error/status messages are posted to MastroGUI via IPC.

 @return TRUE if at least ONE relevant hardware device was found & successfully configured; FALSE if no device was 
 found or if there was a fatal memory allocation error.
*/
BOOL RTFCNDCL CCxDriver::OpenHardwareResources()
{
   // set suspend duty cycle: 18ms on, 2ms suspended. Save old suspend params for restoring later
   int iOn, iOff; 
   m_suspendMgr.ChangeTiming(18000, 2000, &iOn, &iOff);

   m_masterIO.Message("Initializing hardware...MAY TAKE A WHILE");
   m_masterIO.ResetHardwareInfo(); 

   DWORD dwHWState = 0;

   // opend connection to all available devices. This fails only a fatal memory allocation error.
   if(!m_DevMgr.Startup( &m_masterIO )) return( FALSE ); 

   // if AI device available, install our ISR. Close AI device if cannot install ISR, since we cannot use it otherwise!
   CCxAnalogIn* pAI = m_DevMgr.GetAI(); 
   if(pAI->IsOn())
   {
      if(!pAI->SetInterruptHandler(CCxDriver::ServiceAI, (PVOID)this))
      {
         m_masterIO.Message("(!) Unable to install AI device interrupt -- AI not available");
         pAI->Close();
      }
      else
      {
         dwHWState |= CX_F_AIAVAIL;
         if(pAI->Is16Bit()) dwHWState |= CX_F_AI16BIT;
         if(pAI->CanCalibrate()) 
         {
            dwHWState |= CX_F_AICAL;
            pAI->Calibrate();
         }
      }
   }

   // if event timer device available, set the busy wait times for DO command deliver IAW settings specified by Maestro
   // in IPC...
   if((m_DevMgr.GetTimer())->IsOn())
   {
      (m_DevMgr.GetTimer())->SetDOBusyWaitTimes(
            m_masterIO.GetDOBusyWait(0), m_masterIO.GetDOBusyWait(1), m_masterIO.GetDOBusyWait(2));
      ::sprintf_s(m_strMsg, "Set DO busy wait times to: %.1f, %.1f, %.1f",
         m_masterIO.GetDOBusyWait(0), m_masterIO.GetDOBusyWait(1), m_masterIO.GetDOBusyWait(2));
      m_masterIO.Message(m_strMsg);
   }

   // set hardware status and capabilities info in IPC for access by Maestro GUI...
   if((m_DevMgr.GetAO())->IsOn())
   {
      dwHWState |= CX_F_AOAVAIL;
      if((m_DevMgr.GetAO())->Is16Bit()) dwHWState |= CX_F_AO16BIT;
   }

   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if((m_DevMgr.GetTimer())->IsOn()) dwHWState |= CX_F_TMRAVAIL;
   if(pRMV->IsOn()) dwHWState |= CX_F_RMVAVAIL;

   m_masterIO.SetHardwareStatus(dwHWState);
   m_masterIO.SetAIChannels(pAI->GetNumChannels());
   m_masterIO.SetAOChannels(m_DevMgr.GetAO()->GetNumChannels());
   m_masterIO.SetTDOChannels(m_DevMgr.GetTimer()->GetNumDO());
   m_masterIO.SetTDIChannels(m_DevMgr.GetTimer()->GetNumDI());

   // restore suspend mgr to prior state
   m_suspendMgr.ChangeTiming(iOn, iOff);
   return( BOOL((dwHWState & CX_F_AVAILMASK) != 0) );
}

/**
 Close and destroy all MaestroRTSS device objects created by OpenHardwareResources().
*/
VOID RTFCNDCL CCxDriver::CloseHardwareResources()
{
   m_DevMgr.Shutdown();                               // close all device connections and destroy device objects
   m_masterIO.ResetHardwareInfo();                    // reinitialize hardware status info in Maestro IPC interface
}


/**
 ServiceAI(): Respond to a hardware interrupt from the analog input (AI) board.

 MaestroRTSS enables only one kind of interrupt from the AI device, a "start-of-scan" interrupt that occurs once per 
 scan interval, <~100us before all available AI channels (the "slow scan set") have been scanned. This ISR responds to 
 and clears that interrupt, then updates certain critical runtime control variables used by the runtime loops in each 
 operational mode.

 Here is where MaestroRTSS's "real-time" requirement is enforced. To ensure that stimulus presentation stays in step 
 w/the data acquisition timeline, the Trial Mode runtime loop is designed to service each interrupt BEFORE the next one 
 occurs; otherwise, we have a "frame shift", in which the runtime loop has fallen at least one full cycle (the duration 
 of a scan interval) behind the actual run time. Here increment a "tick" counter that allows us to detect the frameshift
 condition during a trial. The tick counter is also useful in Continuous Mode, since we can use it to determine how many
 scan sets of "slow data" are ready in the AI FIFO (frame shifts are permissible in Continuous Mode).
 
 For this scheme to be successful, we must be confident that the system's interrupt latencies are much shorter than a 
 scan interval. THIS IS ONE REASON WE USE RTX! Frame shifts will NOT be reliably detected if this ISR is not invoked in 
 a timely fashion. We have therefore added a CElapsedTime object that measures the interval between ISR invocations, and
 sets a flag if the interval is greater than the expected interval by more than 500us. Protocol runtime loops can check 
 this flag, abort the protocol, and inform the user.

 @param pThisObj Ptr to THIS (static methods do not get the implied THIS arg!!).
 @return TRUE if AI board was source of interrupt; FALSE otherwise (to allow shared IRQ).
*/
BOOLEAN RTFCNDCL CCxDriver::ServiceAI(PVOID pThisObj)
{
   CCxDriver* pDrvr = (CCxDriver*)pThisObj;

   RtDisableInterrupts();

   BOOLEAN bIntAckd = FALSE;                                         // check for & ack the "start scan" INT on our AI
   if( pDrvr->m_DevMgr.GetAI()->IntAck() )                           // device; if it occurred, update runtime control
   {                                                                 // state:
      int iDelay = int( pDrvr->m_eTimeISR.GetAndReset() + 0.5 );     //    detect ISR latency > 500us
      if( pDrvr->m_viElapsedTicks > 0 )
         iDelay -= 1000 * pDrvr->m_viScanInterval;
      if( iDelay > 500 ) pDrvr->m_vbDelayedISR = TRUE;

      pDrvr->m_viElapsedTicks++;                                     //    increment # of INTs thus far
      pDrvr->m_vbInterruptPending = TRUE;                            //    set flag indicating another int has occurred
      pDrvr->m_viPlotUpdateMS -= pDrvr->m_viScanInterval;            //    update various runtime counters...
      pDrvr->m_viFixChkMS -= pDrvr->m_viScanInterval;
      if( pDrvr->m_vbStimOn )                                        //    when a cont-mode stimulus run in progress,
      {                                                              //    incr "tick" counter & reset to 0 when it
         pDrvr->m_viStimTicks++;                                     //    reaches end of duty cycle
         if( pDrvr->m_viStimTicks >= pDrvr->m_viStimDutyCycle )
            pDrvr->m_viStimTicks = 0;
      }
      bIntAckd = TRUE;                                               //    set flag indicating int was handled
   }

   RtEnableInterrupts();
   return ( bIntAckd );
}


/**
 Runtime loop while in "Idle Mode". In this mode, MaestroRTSS is -- for the most part -- idle, yielding most of the CPU 
 time to MaestroGUI (and any other processes running on the system, of course!). It does, however, execute and service a
 continuous DAQ in order to monitor chair position continuously & compensate for any position drift (chair should be at 
 "zero"). This feature allows the user to leave the chair servo in rate mode always while Maestro is running.

 Current duty cycle is 20ms, suspended 95% of the time.
*/
VOID RTFCNDCL CCxDriver::RunIdleMode()
{
   m_masterIO.Message("Entering idle mode...");

   // set suspend mgr duty cycle: 1ms on, 19ms suspended; save old suspend parameters to restore upon exiting Idle Mode.
   int iOldOn, iOldOff; 
   m_suspendMgr.ChangeTiming(1000, 19000, &iOldOn, &iOldOff);

   // chair velocity should be zero
   m_DevMgr.GetAO()->InitChair(); 

   // configure and start our prototypical AI sequence with a scan interval of 20 ms
   m_viScanInterval = 20; 
   ConfigureAISeq(); 
   StartAISeq();

   // the Idle Mode RUNTIME LOOP:
   int iCurrMode = CX_IDLEMODE;
   while(iCurrMode == CX_IDLEMODE) 
   {
      // upon detecting the start of the next AI scan, read in up to 2 scan's worth of data (it's OK to fall a little
      // behind -- we're not time-critical in Idle Mode!). Restart AI sequence if an error occurs; else, compensate
      // for chair drift.
      if(m_vbInterruptPending) 
      {
         m_vbInterruptPending = FALSE;  
         if(!UnloadNextAIScan())
         {
            ConfigureAISeq();
            StartAISeq();
         }
         else if(m_masterIO.IsChairPresent())
         {
            float fPosDeg = ((float) m_pshLastScan[HHPOS]) / POS_TOAIRAW;
            m_DevMgr.GetAO()->SettleChair( fPosDeg );
         }
      }

      // respond to select commands from MaestroGUI
      DWORD dwCmd = m_masterIO.GetCommand();
      if(dwCmd != CX_NULLCMD)
      {
         if(dwCmd == CX_SWITCHMODE)
         {
            m_masterIO.GetCommandData( &iCurrMode, NULL, 1, 0 );
            m_masterIO.AckCommand( dwCmd, NULL, NULL, 0, 0 );
         }
         else if(dwCmd == CX_SAVECHANS) UpdateAISaveList();
         else if(dwCmd == CX_SETDISPLAY) UpdateVideoDisplaysAndAck();
         else if(dwCmd == CX_FIXREWSETTINGS) UpdateFixRewSettings();
         else if(dwCmd == CX_RMV_GETMODES)  GetRMVDisplayModes();
         else if(dwCmd == CX_RMV_GETCURRMODE) GetCurrRMVDisplayMode();
         else if(dwCmd == CX_RMV_GETGAMMA) GetRMVMonitorGamma();
         else if(dwCmd == CX_RMV_SETGAMMA) SetRMVMonitorGamma();
         else if(dwCmd == CX_RMV_GETMDIRS) GetRMVMediaFolders();
         else if(dwCmd == CX_RMV_GETMFILES) GetRMVMediaFiles();
         else if(dwCmd == CX_RMV_GETMFINFO) GetRMVMediaFileInfo();
         else if(dwCmd == CX_RMV_SETCURRMODE || dwCmd == CX_RMV_DELMEDIA || dwCmd == CX_RMV_PUTMEDIA)
         {
            // these commands take a long time to execute, so we might as well stop the background AI. Also, the chair
            // will drift during this time.
            ConfigureAISeq();
            
            // the download command also requires a lot of work from Maestro driver, so we change the suspend manager
            // to give most of the time to the driver!
            if(dwCmd == CX_RMV_PUTMEDIA) m_suspendMgr.ChangeTiming(19000, 1000);
            
            // perform the requested long-running operation
            if(dwCmd == CX_RMV_SETCURRMODE) SetCurrRMVDisplayMode();
            else if(dwCmd == CX_RMV_DELMEDIA) DeleteRMVMediaFile();
            else DownloadRMVMediaFile();
            
            // restore suspend manager timing and restart background AI
            if(dwCmd == CX_RMV_PUTMEDIA) m_suspendMgr.ChangeTiming(1000, 19000);
            StartAISeq();
         }
         else
            m_masterIO.AckCommand( CX_UNRECOGCMD, NULL, NULL, 0, 0 );
      }
   }

   // reset relevant hardware to an "idle" state
   m_DevMgr.GetAO()->InitChair();  
   m_DevMgr.GetAI()->Init();

   // restore suspend mgr to prior state and switch to the new op mode
   m_suspendMgr.ChangeTiming(iOldOn, iOldOff); 
   m_masterIO.SetMode(iCurrMode);
}

/**
 The following are RunIdleMode() helpers that respond to Idle Mode-only commands specific to RMVideo. Each assumes that 
 the relevant RMVideo command was just received over CCxMasterIO. Each retrieves any command data, calls the relevant 
 CCxRMVideo method to fulfill the request, then acknowledges the command with any response data. See command 
 descriptions in CXIPC.H for details. Note that all commands should complete quickly (<300ms) except for 
 CX_RMV_SETCURRMODE (<= 10 secs), CX_RMV_DELMEDIA (<= 5 secs), and CX_RMV_PUTMEDIA (indefinite).
*/
VOID RTFCNDCL CCxDriver::GetRMVDisplayModes()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if(!pRMV->IsOn())
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      m_masterIO.Message("RMVideo is not available! Cannot retrieve display modes.");
   }
   else
   {
      int n = pRMV->GetNumModes();
      m_iCmdBuf[0] = n;
      int j = 1;
      for(int i=0; i<n; i++) 
      {
         pRMV->GetModeInfo(i, m_iCmdBuf[j], m_iCmdBuf[j+1], m_iCmdBuf[j+2]);
         j += 3;
      }
      m_masterIO.AckCommand(CX_RMV_GETMODES, m_iCmdBuf, NULL, 1+n*3, 0);
   }
}

VOID RTFCNDCL CCxDriver::GetCurrRMVDisplayMode()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if(!pRMV->IsOn())
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      m_masterIO.Message("RMVideo is not available! Cannot retrieve current display mode.");
   }
   else
   {
      int n = pRMV->GetNumModes();
      int mode = pRMV->GetCurrentMode();
      float rate = (float) pRMV->GetFramePeriod();
      if(rate != 0.0f) rate = 1.0f/rate;
      m_masterIO.AckCommand(CX_RMV_GETCURRMODE, &mode, &rate, 1, 1);
   }
}

// BLOCKS FOR UP TO 10 SECONDS WHILE RMVideo remeasures frame period after the mode switch!!!
VOID RTFCNDCL CCxDriver::SetCurrRMVDisplayMode()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   int mode = -1;
   m_masterIO.GetCommandData(&mode, NULL, 1, 0);
   if(!pRMV->SetCurrentMode(mode))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Cannot switch RMVideo display mode: %s", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
   {
      float rate = (float) pRMV->GetFramePeriod();
      if(rate != 0.0f) rate = 1.0f/rate;
      m_masterIO.AckCommand(CX_RMV_SETCURRMODE, NULL, &rate, 0, 1);
   }
}

VOID RTFCNDCL CCxDriver::GetRMVMonitorGamma()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if(!pRMV->IsOn())
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      m_masterIO.Message("(!!) RMVideo is not available!");
   }
   else
   {
      pRMV->GetMonitorGamma(m_iCmdBuf[0], m_iCmdBuf[1], m_iCmdBuf[2]);
      float rgb[3];
      for(int i=0; i<3; i++) rgb[i] = ((float) m_iCmdBuf[i]) / 1000.0f;

      m_masterIO.AckCommand(CX_RMV_GETGAMMA, NULL, rgb, 0, 3);
   }
}

VOID RTFCNDCL CCxDriver::SetRMVMonitorGamma()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   float rgb[3];
   m_masterIO.GetCommandData(NULL, rgb, 0, 3);
   for(int i=0; i<3; i++)
   {
      m_iCmdBuf[i] = (int) (1000.0f * rgb[i]);
      if(m_iCmdBuf[i] < RMV_MINGAMMA) m_iCmdBuf[i] = RMV_MINGAMMA;
      else if(m_iCmdBuf[i] > RMV_MAXGAMMA) m_iCmdBuf[i] = RMV_MAXGAMMA;
   }
   
   if(!pRMV->SetMonitorGamma(m_iCmdBuf[0], m_iCmdBuf[1], m_iCmdBuf[2]))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Cannot set RMVideo monitor gamma: %s", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
   {
      float rgb[3];
      for(int i=0; i<3; i++) rgb[i] = ((float) m_iCmdBuf[i]) / 1000.0f;

      m_masterIO.AckCommand(CX_RMV_SETGAMMA, NULL, rgb, 0, 3);
   }
}

VOID RTFCNDCL CCxDriver::GetRMVMediaFolders()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if(!pRMV->GetMediaFolders(m_iCmdBuf[0], m_cCmdBuf))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Cannot get RMVideo media store folders: %s", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
   {
      // find out total length of strings in character buffer, including terminal nulls!
      int n = 0;
      for(int i=0; i<m_iCmdBuf[0]; i++)
      {
         size_t len = ::strlen(&(m_cCmdBuf[n])) + 1;
         n += (int) len;
      }
      
      m_masterIO.AckCommand(CX_RMV_GETMDIRS, m_iCmdBuf, NULL, 1, 0, FALSE, m_cCmdBuf, n);
   }
}

VOID RTFCNDCL CCxDriver::GetRMVMediaFiles()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   char folder[RMV_MVF_LEN+1];
   m_masterIO.GetCommandData(NULL, NULL,0, 0, folder, RMV_MVF_LEN+1);
   if(!pRMV->GetMediaFiles(folder, m_iCmdBuf[0], m_cCmdBuf))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Cannot get RMVideo media file list for folder %s: %s", folder, 
         pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
   {
      // find out total length of strings in character buffer, including terminal nulls!
      int n = 0;
      for(int i=0; i<m_iCmdBuf[0]; i++)
      {
         size_t len = ::strlen(&(m_cCmdBuf[n])) + 1;
         n += (int) len;
      }
      m_masterIO.AckCommand(CX_RMV_GETMFILES, m_iCmdBuf, NULL, 1, 0, FALSE, m_cCmdBuf, n);
   }
}

VOID RTFCNDCL CCxDriver::GetRMVMediaFileInfo()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   m_masterIO.GetCommandData(NULL, NULL, 0, 0, m_cCmdBuf, 2*(RMV_MVF_LIMIT+1));
   int iStartFile = (int) (::strlen(m_cCmdBuf) + 1);
   float fInfo[2];
   if(!pRMV->GetMediaInfo(m_cCmdBuf, &(m_cCmdBuf[iStartFile]), m_iCmdBuf[0], m_iCmdBuf[1], fInfo[0], fInfo[1]))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Cannot get info on media file %s/%s: %s", m_cCmdBuf, &(m_cCmdBuf[iStartFile]),
            pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
      m_masterIO.AckCommand(CX_RMV_GETMFINFO, m_iCmdBuf, fInfo, 2, 2);
}

// BLOCKS FOR UP TO 5 SECONDS WHILE RMVideo removes the file or entire folder
VOID RTFCNDCL CCxDriver::DeleteRMVMediaFile()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   m_masterIO.GetCommandData(m_iCmdBuf, NULL, 1, 0, m_cCmdBuf, 2*(RMV_MVF_LIMIT+1));
   char* folder = m_cCmdBuf;
   char* file = NULL;
   if(m_iCmdBuf[0] == 0) 
   {
      int iStartFile = (int) (::strlen(m_cCmdBuf) + 1);
      file = &(m_cCmdBuf[iStartFile]);
   }
   
   if(!pRMV->DeleteMediaFile(folder, file))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      if(file != NULL)
         ::sprintf_s(m_strMsg, "(!!) Unable to remove media file %s/%s: %s", folder, file, pRMV->GetLastDeviceError());
      else
         ::sprintf_s(m_strMsg, "(!!) Unable to remove media folder %s: %s", folder, pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
      m_masterIO.AckCommand(CX_RMV_DELMEDIA, NULL, NULL, 0, 0);
}

VOID RTFCNDCL CCxDriver::DownloadRMVMediaFile()
{
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   m_masterIO.GetCommandData(NULL, NULL, 0, 0, m_cCmdBuf, CX_MAXPATH + 1 + 2*(RMV_MVF_LIMIT+1));
   
   // extract the 3 string arguments from character buffer and make sure they're reasonable
   char* path = m_cCmdBuf;
   char* folder = NULL;
   char* file = NULL;
   int len = (int) ::strlen(path);
   BOOL bOk = (len > 0 && len <= CX_MAXPATH);
   if(bOk)
   {
      folder = &(m_cCmdBuf[len+1]);
      int len2 = (int) ::strlen(folder);
      bOk = (len2 > 0) && (len2 <= RMV_MVF_LIMIT) && (len2 == (int) ::strspn(folder, RMV_MVF_CHARS));
      len += len2 + 1;
   }
   if(bOk)
   {
      file = &(m_cCmdBuf[len+1]);
      int len2 = (int) ::strlen(file);
      bOk = (len2 > 0) && (len2 <= RMV_MVF_LIMIT) && (len2 == (int) ::strspn(file, RMV_MVF_CHARS));
      len += len2 + 1;
   }
   if(!bOk)
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      m_masterIO.Message("Media file download failed; bad source path, or bad destination folder or file name.");
      return;
   }

   if(!pRMV->DownloadMediaFile(path, folder, file))
   {
      m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
      ::sprintf_s(m_strMsg, "(!!) Media file download failed: %s", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   else
   {
      float fInfo[2];
      if(!pRMV->GetMediaInfo(folder, file, m_iCmdBuf[0], m_iCmdBuf[1], fInfo[0], fInfo[1]))
      {
         m_iCmdBuf[0] = m_iCmdBuf[1] = 0;
         fInfo[0] = fInfo[1] = 0.0f;
         m_masterIO.Message("Media file download successful, but failed while retrieving media info!");
      }
      m_masterIO.AckCommand(CX_RMV_PUTMEDIA, m_iCmdBuf, fInfo, 2, 2);
   }
}


/**
 Runtime loop while MaestroRTSS is in Test and Calibration Mode.
 
 CREDIT: The recurrence relation used to iteratively compute mean and variance for the analog data was obtained on
 the web at PlanetMath.org and is attributed to:
    B. P. Welford. "Note on a Method for Calculating Corrected Sums of Squares and Products" Technometrics, Vol. 4, 
    No. 3 (Aug., 1962), p. 419-420.
*/
VOID RTFCNDCL CCxDriver::RunTestMode()
{
   m_masterIO.Message("Entering test mode...");

   // set suspend mgr duty cycle: 2ms, 70% suspended; save old suspend params to restore upon exiting Test&Cal mode
   int iOldOn, iOldOff; 
   m_suspendMgr.ChangeTiming(600, 1400, &iOldOn, &iOldOff);

   // for iterative computation of mean and variance of the signal sampled on each AI channel
   int nSamples = 0;
   double dMean[CX_AIO_MAXN];
   double dVar[CX_AIO_MAXN];
   for(int i=0; i<CX_AIO_MAXN; i++) { dMean[i] = dVar[i] = 0.0; }
   
   // this array is used to report AI channel data to Maestro in response to CX_TM_GETAI command
   // most recent AI channel scan vector, in volts
   float fAIData[3*CX_AIO_MAXN]; 
   for(int i=0; i<3*CX_AIO_MAXN; i++) { fAIData[i] = 0.0f; }

   // scan intv in ms; AO channel on which test waveform is output (none); AO test waveform time epoch (ms)
   m_viScanInterval = 2; 
   int iTestWaveCh = -1;
   int tWave = 0; 

   // get pointers to the device objects we'll use in Test&Cal mode
   CCxEventTimer* pTimer = m_DevMgr.GetTimer(); 
   CCxAnalogIn* pAI = m_DevMgr.GetAI(); 
   CCxAnalogOut* pAO = m_DevMgr.GetAO();

   // initially, AI DAQ disabled; start monitoring event stats on the DIO event timer device
   BOOL bAIPaused = TRUE;
   pTimer->StartMonitor(); 
   int iOpMode = CX_TESTMODE;
   DWORD dwEventsInScan = 0;

   while(iOpMode == CX_TESTMODE)
   {
      // update event stats on DIO event timer device
      pTimer->ServiceMonitor( dwEventsInScan );

      // at start of next ADC scan, unload the new scan's worth of AI data; restart on frameshift or an AI error.
      if(m_vbInterruptPending)
      {
         m_vbInterruptPending = FALSE;
         BOOL bOk = UnloadNextAIScan();
         if(m_vbFrameLag || !bOk)
         {
            if(!bOk) ::sprintf_s(m_strMsg, "(!!) AI dev error (%s)  Restarted AI device.", pAI->GetLastDeviceError());
            else ::sprintf_s(m_strMsg, "(!!) Frameshift occurred.  Restarted AI device.");
            m_masterIO.Message( m_strMsg );
            ConfigureAISeq();
            StartAISeq();
         }

         // iterative calculation of mean and variance of AI channel signals. Reset if an error occurred or if we've
         // reached 10^7 samples.
         ++nSamples;
         if(nSamples >= 10000000 || m_vbFrameLag || !bOk)
         {
            nSamples = 0;
            for(int i=0; i<CX_AIO_MAXN; i++) { dMean[i] = dVar[i] = 0; }
         }
         else if(nSamples == 1)
         {
            for(int i=0; i<CX_AIO_MAXN; i++) dMean[i] = pAI->ToVolts(m_pshLastScan[i]);
         }
         else for(int i=0; i<CX_AIO_MAXN; i++)
         {
            double m = dMean[i];
            double v = dVar[i];
            double x = pAI->ToVolts(m_pshLastScan[i]);
            
            dMean[i] = ((nSamples-1)*m + x) / ((double)nSamples);
            dVar[i] = v + (x - dMean[i]) * (x - m);
         }
         
         // update data traces on MaestroGUI thru IPC
         m_masterIO.UpdateTrace(m_pshLastScan, NULL, dwEventsInScan); 
         dwEventsInScan = 0;

         // if running, update canned test waveform on specified AO channel: 5.0*sin(2*pi*(1Hz)*(0.001s/ms)*(t in ms))
         if(iTestWaveCh != -1)
         {
            pAO->Out(iTestWaveCh, (float) (5.0*sin( 0.006283185 * ((double)tWave) )));
            tWave += m_viScanInterval;
            if(tWave >= 1000) tWave = 0;
         }

      }

      // process next command from MaestroGUI...
      DWORD dwCmd = m_masterIO.GetCommand(); 
      if(dwCmd != CX_NULLCMD) switch(dwCmd)
      {
         // switch to another operational mode
         case CX_SWITCHMODE : 
            m_masterIO.GetCommandData(&iOpMode, NULL, 1, 0);
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // update AI channel save list
         case CX_SAVECHANS: 
            UpdateAISaveList();
            break;
         // pause DAQ of all AI channels
         case CX_TM_PAUSEAI : 
            if(!bAIPaused)
            {
               pAI->Init();
               m_vbInterruptPending = FALSE;
               bAIPaused = TRUE;
            }
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // resume DAQ of all AI channels; reset iterative calcluation of mean and std dev of recorded signal
         case CX_TM_RESUMEAI : 
            if(bAIPaused)
            {
               ConfigureAISeq();
               StartAISeq();
               bAIPaused = FALSE;
               dwEventsInScan = 0;
               nSamples = 0;
               for(int i=0; i<CX_AIO_MAXN; i++) { dMean[i] = dVar[i] = 0; }
            }
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // get AI channel data stats: last voltage sampled on each channel, followed by the average voltage recorded
         // on each channel, followed by the std dev of the voltage recorded on each channel.
         case CX_TM_GETAI : 
            for(int i = 0; i < CX_AIO_MAXN; i++)
            {
               fAIData[i] = pAI->ToVolts(m_pshLastScan[i]);
               fAIData[CX_AIO_MAXN + i] = (float) dMean[i];
               if(nSamples < 2) fAIData[2*CX_AIO_MAXN + i] = 0.0f;
               else fAIData[2*CX_AIO_MAXN + i] = (float) ::sqrt(dVar[i] /((double) nSamples - 1));
            }
            m_masterIO.AckCommand(dwCmd, NULL, &(fAIData[0]), 0, 3*CX_AIO_MAXN);
            break;
         // quick internal calibration of AI dev
         case CX_TM_AICAL : 
            m_suspendMgr.Bypass();
            pAI->Init();
            pAI->Calibrate();
            
            // since we had to reset AI subsystem, resume DAQ if it was running before
            if(!bAIPaused) 
            { 
               ConfigureAISeq();
               StartAISeq();
               nSamples = 0;
               for(int i=0; i<CX_AIO_MAXN; i++) { dMean[i] = dVar[i] = 0; }
            }
            m_suspendMgr.Resume();
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // set voltage on one or all AO channels, returning the voltage actually set
         case CX_TM_SETAO :  
            if(pAO->IsOn()) 
            {
               float fAOVolt;
               int iCh;
               m_masterIO.GetCommandData(&iCh, &fAOVolt, 1, 1);
               fAOVolt = pAO->NearestVolts(fAOVolt);

               // if test waveform is running on an AO channel, we must be careful not to modify it!
               if(iTestWaveCh == -1 || (iCh != -1 && iCh != iTestWaveCh)) pAO->Out(iCh, fAOVolt);
               else if(iCh == -1)
               {
                  for(int j = 0; j < pAO->GetNumChannels(); j++)
                  {
                     if(j != iTestWaveCh) pAO->Out(j, fAOVolt);
                  }
               }
               m_masterIO.AckCommand(dwCmd, NULL, &fAOVolt, 0, 1);
            }
            else
               m_masterIO.AckCommand( CX_FAILEDCMD, NULL, NULL, 0, 0 );
            break;
         // start or stop the canned test waveform on a selected AO channel
         case CX_TM_AOWAVE : 
            if(pAO->IsOn()) 
            {
               int iCh;
               m_masterIO.GetCommandData(&iCh, NULL, 1, 0);
               if(iCh < 0 || iCh >= pAO->GetNumChannels()) iCh = -1;    // an invalid ch# turns off waveform
               if(iTestWaveCh != -1) pAO->Out(iTestWaveCh, 0);          // zero ch on which waveform was running
               iTestWaveCh = iCh;                                       // switch to new channel
               tWave = 0;
               m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            }
            else
               m_masterIO.AckCommand( CX_FAILEDCMD, NULL, NULL, 0, 0 );
            break;
         // get current event stats from the event timer device
         case CX_TM_GETTMRSTATE : 
            if(pTimer->IsOn())
            {
               int iBuf[CX_TMR_MAXN + 1];                               // #event per channel
               float fBuf[CX_TMR_MAXN*2];                               // last event time, mean IEI per channel
               DWORD dwEvtMask;                                         // event mask for most recent event
               int n = pTimer->GetNumDI();
               pTimer->GetMonitor(&(iBuf[0]), &(fBuf[0]), &(fBuf[n]), dwEvtMask);
               iBuf[n] = (int) dwEvtMask;
               m_masterIO.AckCommand(dwCmd, iBuf, fBuf, n+1, 2*n);
            }
            else
               m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // reset event timer and recorded stats
         case CX_TM_RESETTMR : 
            if(pTimer->IsOn())
            {
               pTimer->StopMonitor();
               pTimer->StartMonitor();
               m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            }
            else
               m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // set DO port on event timer device
         case CX_TM_SETTMRDO : 
            if(pTimer->IsOn())
            {
               int iVal;
               m_masterIO.GetCommandData(&iVal, NULL, 1, 0);
               pTimer->SetDO((DWORD) iVal);
               m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            }
            else
               m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // reinitialize data trace facility
         case CX_INITTRACE : 
            m_masterIO.AckCommand( m_masterIO.InitTrace() ? dwCmd : CX_FAILEDCMD, NULL, NULL, 0, 0 );
            break;
         // any other command is not recognized in Test Mode!
         default :  
            m_masterIO.AckCommand( CX_UNRECOGCMD, NULL, NULL, 0, 0 );
            break;
      }

   }

   // reset hardware used. We reset the latched devices controlled by the DO port of the DIO timer, just in case the
   // user inadvertently manipulated any of them during TestMode.
   pAO->Init();  
   pTimer->StopMonitor();
   pTimer->ResetLatchedDevices(); 
   pAI->Init();

   // restore suspend mgr to previous state and switch to the new op mode
   m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
   m_masterIO.SetMode(iOpMode); 
}


/**
 Runtime loop while idling between trials in Trial Mode. As in Idle Mode, MaestroRTSS runs a continuous AI sequence to 
 handle chair drift compensation as needed. However, it also keeps the eye-target position plot up-to-date and handles 
 MaestroGUI commands that are unique to Trial Mode. The command CX_TR_START actually starts a trial. Upon receipt, it
 is assumed that the target list and trial codes have already been prepared in the Maestro IPC shared memory structure,
 and control passes to the monster function ExecuteSingleTrial(), which handles all the details of running a single
 MaestroRTSS trial.

 Current duty cycle (between trials, NOT during one!) is 10ms, suspended 95% of the time.
*/
VOID RTFCNDCL CCxDriver::RunTrialMode()
{
   m_masterIO.Message("Entering trial mode...");

   // set suspend mgr duty cycle: 10ms, 95% suspended; save old suspend params to restore upon exiting Trial Mode
   int iOldOn, iOldOff; 
   m_suspendMgr.ChangeTiming(500, 9500, &iOldOn, &iOldOff);

   // chair velocity should be zero
   m_DevMgr.GetAO()->InitChair(); 

   // scan interval during inter-trial interval is 10ms
   m_viScanInterval = 10; 
   // zero the plot update countdown timer so we update the eye-target pos plot immediately
   m_viPlotUpdateMS = 0; 

   // flag set whenever a blink causes Eyelink tracker data to be missing
   BOOL bInBlink = FALSE;

   // configure and start our prototypical AI sequence
   ConfigureAISeq(); 
   StartAISeq(); 

   // INTER-TRIAL RUNTIME LOOP:
   int iOpMode = CX_TRIALMODE;
   while(iOpMode == CX_TRIALMODE) 
   {
      // AI scan start: read in up to 2 scan's worth of data; restart on AI error. Compensate for chair drift.
      if(m_vbInterruptPending)
      {
         m_vbInterruptPending = FALSE; 
         if(!UnloadNextAIScan())
         {
            ConfigureAISeq();
            StartAISeq();
         }
         else if(m_masterIO.IsChairPresent())
         {
            float fPosDeg = ((float) m_pshLastScan[HHPOS]) / POS_TOAIRAW;
            m_DevMgr.GetAO()->SettleChair(fPosDeg);
         }

         // if Eyelink tracker in use, unload next tracker sample and use it for HGPOS/VEPOS. We flush the sample buffer
         // each time, and ignore blinks and any sample delays because we're not recording.
         UnloadEyelinkSample(&bInBlink, -1);
      }

      // update pos of eye & chair periodically; targets are irrelevant and should be "offscreen"
      CFPoint fPt(180); 
      UpdateLoci(fPt, fPt, fPt);

      // handle command from MaestroGUI...
      DWORD dwTrialRes;
      DWORD dwCmd = m_masterIO.GetCommand(); 
      if( dwCmd != CX_NULLCMD ) switch( dwCmd )
      {
         // switch to another operational mode
         case CX_SWITCHMODE : 
            m_masterIO.GetCommandData(&iOpMode, NULL, 1, 0);
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0, TRUE);
            break;
         // change the current list of AI channels saved
         case CX_SAVECHANS : 
            UpdateAISaveList();
            break;
         // change current video display parameters
         case CX_SETDISPLAY :
            UpdateVideoDisplaysAndAck();
            break;
         // update fixation/reward settings
         case CX_FIXREWSETTINGS : 
            UpdateFixRewSettings();
            break;

         // execute the trial currently defined in IPC
         case CX_TR_START : 
            // clear previous trial result; complete command handshake now -- we won't poll for cmds for a while!
            m_masterIO.ClearResult(); 
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0, TRUE); 
            
            // reset hardware that operated during inter-trial interval
            m_DevMgr.GetAO()->InitChair(); 
            m_DevMgr.GetAI()->Init();

            // run the trial, then report result to MaestroGUI, indicating that we're ready for next trial.
            dwTrialRes = ExecuteSingleTrial(); 
            m_masterIO.SetResult( dwTrialRes ); 

            // restart slower inter-trial DAQ, but check op mode first -- user COULD switch during trial.
            iOpMode = m_masterIO.GetMode(); 
            if(iOpMode != CX_TRIALMODE) break;
            m_viScanInterval = 10;
            m_viPlotUpdateMS = 0;
            ConfigureAISeq();
            StartAISeq();
            break;

         // reinitialize the data trace facility
         case CX_INITTRACE : 
            m_masterIO.AckCommand(m_masterIO.InitTrace() ? dwCmd : CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // reinitialize digital event streaming
         case CX_INITEVTSTREAM : 
            m_masterIO.AckCommand(m_masterIO.InitEventStream() ? dwCmd : CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // any other command is not recognized in Trial Mode!
         default :
            m_masterIO.AckCommand(CX_UNRECOGCMD, NULL, NULL, 0, 0);
            break;
      }
   }

   // reset hardware used during inter-trial period in Trial Mode, restore suspend mgr params, and switch to new mode
   m_DevMgr.GetAI()->Init(); 
   m_DevMgr.GetAO()->InitChair();
   m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
   m_masterIO.SetMode(iOpMode);
}


/**=== ExecuteSingleTrial ==============================================================================================
 Conduct a single experimental trial IAW the runtime "trial codes" that should have already been prepared by Maestro and
 stored in the IPC shared memory area.

 BACKGROUND: This is an extremely long & complex function that has been adapted from its same-named counterpart in 
 Maestro's predecessor, Cntrlx. In Cntrlx, a UNIX-based GUI ("cntrlxUNIX") sent a trial's definition to the PC-based 
 hardware controller ("cntrlxPC") over the network as a chronological sequence of "trial codes". While we could improve 
 on this scheme here, we have elected to stick with the trial-code mechanism for a number of reasons: (1) To get this 
 method up & running as quickly as possible. (2) Existing cntrlxUNIX data files use the trial code set to store a 
 trial's definition, and some existing analysis programs make use of this information. For now we'd like to keep the 
 Maestro trial data file format as unchanged as possible! For a complete listing & description of all trial codes 
 available in Maestro, see CXTRIALCODES.H.

 ON INTEGRATING DATA RECORDING WITH AN EXTERNAL SYSTEM. MaestroRTSS uses the event timer's "character writer" and 
 "marker pulse" functions to deliver information useful for external synchronization and offline integration of a 
 recorded Maestro trial with data collected by a separate data acquisition system:
    -- Before trial code preprocessing begins, START_CHARCODE is written to the character writer device, followed by the
 null-terminated trial name, then the null-terminated data file name. If no file name was specified by Maestro (meaning 
 that data file is intentially NOT saved), a single NOFILE_CHARCODE character appears in place of the file name (still 
 followed by a terminating null!).
    -- When trial starts, a marker pulse is issued on timer DO<11>, which is dedicated exclusively for this purpose.
    -- When trial ends (normally or otherwise), a second marker pulse is triggered on DO<11>.
    -- If the trial aborted because the animal broke fixation, the LOSTFIX_CHARCODE character is written to the
 character device. If the trial aborted for any other reason, the ABORT_CHARCODE is written.
    -- If trial data file was successfully saved, the DATASAVED_CHARCODE is written. If this character is NOT written, 
 then data was deliberately not saved or a file I/O error occurred.
    -- Regardless of how the trial terminates, the STOP_CHARCODE character is written to the character device to
 terminate the sequence.
    -- It is possible that the trial may abort during trial code preprocessing. In that case, the "abort" and "stop" 
 characters will be issued but, of course, the marker pulses on DO<11> will not be!
 
 05jan2012: As of this date, target trajectories are computed on the fly rather than being precomputed and stored in
 large buffers allocated at startup. This eliminated the need for the large buffers and, once we implement data 
 streaming to file as in Continuous mode, will allow for indefinitely long trials.
 
 DESCRIPTION OF SOME SPECIAL FEATURES AVAILABLE IN TRIAL MODE:
 1) "Special segment" operations. During a single, designated segment of a trial, the user can engage one of several 
    possible special features. Some of these operations are triggered by the occurrence of a horizontal or vertical 
    saccade (ie, recorded eye velocity exceeds a specified threshold velocity); others have nothing to do with saccades.

    In the "SkipOnSaccade" operation, the occurrence of a saccade causes trial execution to skip to the end of the
    "saccade" segment.

    If "SelectByFix" is chosen, two fixation targets must be specified during the "saccade" segment and all segments 
    thereafter. When a saccade occurs within the designated segment, we determine which fixation target is closest to 
    eye AT THE END of the saccade (when eye velocity falls back below threshold). If it's close enough, that target is 
    "selected". The unselected target is turned OFF for the remainder of the trial. If neither target is close enough, 
    we continue attempting to select a target after each saccade until the special segment ends. If neither target is 
    selected by the segment's end, we go ahead and select the target nearest the eye. The identity of the target 
    selected determines the length of reward pulse delivered at trial's end: reward pulse length 1 is used if the first
    fixation tgt is selected, else pulse length 2 is used. Finally, note that fixation checking is disabled for the 
    *entire* "SelectByFix" seg.

    "SelectByFix2" is a variant of "SelectByFix". In this version, a fixation target T is selected if, after a saccade, 
    the eye is closest to the current pos of T, **OR** what its current pos would be had a displacement NOT been added 
    at the start of special seg. In all other respects, the two versions are the same. In fact, if neither fixation 
    target is displaced at the beginning of the special segment, "SelectByFix2" works exactly the same as the original 
    version.

    [as of v2.0.5] "ChooseFix1" and "ChooseFix2" implement a forced-choice protocol. The subject is presented with two 
    fixation targets during the special segment -- a "correct" target and an "incorrect" one. The two  ops differ only 
    in the identity of the "correct" target. Fixation checking is disabled during the special segment UNTIL the eye 
    falls within the fixation window of the "correct" target. The "incorrect" target is then turned off (and remains off
    for the remainder of the trial), reward pulse #2 is immediately delivered, a marker pulse is delivered on DOUT6, and
    fixation checking is reenabled. If the subject completes the trial, reward pulse #1 is delivered as usual. However, 
    if the subject fails to fixate on the "correct" target, the trial will abort at the end of the special segment, and 
    the subject gets no reward whatsover. Note that the eye position is compared with that of the correct target during 
    every msec, rather than after saccades.

    "SwitchFix" is another "special" operation, but it is not saccade-triggered and its conditions apply to the special 
    segment and all segments thereafter. The purpose of this operation is to encourage the subject to switch from one 
    fixation target to the other AFTER the special segment, while allowing the subject to select either target as the 
    "initial selection" during the special segment. Like the "SelectByFix" operations, two fixation targets must be 
    specified for the special segment and remain unchanged for the remaining trial segs. Unlike "SelectByFix", no target
    is turned off and fixation checking rules are unique. For the special segment AND ALL SEGMENTS THEREAFTER, fixation 
    checking is enforced EXCEPT during saccades.  Furthermore, the fixation requirement is considered satisfied so long 
    as the eye is within the fixation window of EITHER target. Thus, the animal may freely "switch" between the two 
    designated fixation targets.

    However, the rule for getting a reward at trial's end is unique: To get the reward, the animal MUST be fixating on 
    the target that it was NOT fixating at the end of the special segment. In other words, between the end of the 
    special segment and the end of the trial, the animal must have switched its fixation from one target to the other.

    "R/P Distro" is a new (as of Maestro v1.4) special operation that was introduced to support a reward/penalty
    contingency protocol that is based on the subject's behavioral response. Currently, that response is the eye 
    velocity magnitude averaged over the duration of the "special" segment. At the end of the segment, the average 
    response (in deg/sec) is reported to Maestro via IPC and a trial results flag (CX_FT_GOTRPDRESP) is set to indicate 
    that the response was measured (even if the trial did not run to  completion). If the trial completes, reward pulse 
    #1 is delivered as usual. However, the reward paradigm is special if "reward windows" were defined by the RPDWINDOW 
    trial code. This code group defines the bounds of one or two such windows, in deg/sec.  If the averaged response at 
    the end of the special segment falls w/in one of these windows, then the subject is said to have "passed the test"; 
    otherwise, it is a "failure". In the former case, reward pulse #2 is delivered at the end of the special segment, 
    AND reward pulse #1 is delivered at trial's end. In the latter case, reward pulse #2 is delivered at trial's end. 
    Normally, the user will make reward pulse #2 significantly shorter than reward pulse #1 -- so the subject gets an 
    enhanced reward for "passing" and a reduced reward for "failing".

    [as of v2.6.5] "SearchTask" is a special operation that implements a simple search task: The subject's goal is to 
    search for and fixate upon a designated target among 1 or more "distractor" targets. The search task occurs entirely
    during the "special segment", and the trial will always stop during or at the end of this segment, even if 
    additional segments are defined! The target designated "Fix1" during the special segment is the sought-for target; 
    all other participating targets are "distractors". The search task is "complete", and trial ends, when:
       a) the subject's eye is close enough to the sought target for N contiguous ms, in which case reward pulse 1 is 
    delivered;
       b) the subject's eye is close enough to a distractor target for N contiguous ms, in which case reward pulse 2 is 
    delivered;
       c) the eye wanders outside the target display window (RMVideo only), in which case NO reward is 
    delivered;
       d) the end of the special segment is reached without any of (1)-(3) occuring, in which case NO reward is 
    delivered.
    
    Here, the special segment's "grace period" defines the required fixation time N (which is a bit of misuse of that 
    segment header parameter!), and the segment's H,V fixation accuracy indicate how close the eye must be to the sought
    target or a distractor.

    [as of v4.2.0] The "SearchTask" feature now supports either 1 or 2 search targets. If "Fix2" is designated during 
    the special segment and is ON, then it is the second search target. In this scenario, the trial ends when either
    search target or a distractor is fixated on for the required N contiguous ms. But the reward delivered is different:
    reward pulse 1 for "finding" the Fix1 target, reward pulse 2 for "finding" Fix2, and NO reward for finding a
    distractor.

    The animal is considered to have "tried" the task so long as the special segment begins and, at some point during 
    that segment, the animal's eye velocity reaches the saccadic threshold. The trial is marked as "completed" (ie, 
    subject satisfied fixation requirements during the trial) so long as the task was "tried", even if no target was 
    selected. This result is unique to "search task" trials!

    Implementation constraints (these are enforced by Maestro, not here!): With the exception of "R/P Distro", these 
    special operations are incompatible with trials involving the turntable. If any of the two-target operations is in 
    effect, the user must specify fixation tgt#1 and #2 for the special segment, and those same fixation tgts must be 
    used during all subsequent segments. Also, the "SwitchFix" operation requires there be at least one trial segment 
    after the special segment. The "SearchTask" operation requires a non-zero grace period and a valid "Fix1" 
    designation for the special segment; also, there must be 2+ trial targets.

    The implementation delivers a pulse on the event timer's DOUT6 when a saccade is detected. In the case of
    "SelectByFix" (both versions), we only pulse DOUT6 if a target is selected. In the case of the "ChooseFix" ops, we 
    only pulse DOUT6 if the "correct" target was selected. No pulse is delivered for the "SwitchFix", "R/P Distro", or 
    "SearchTask" ops. Users must be made aware of this, so that they don't assign DOUT6 to some other use!!!

 2) Velocity stabilization. This feature stabilizes a target's trajectory with respect to that of the eye -- the
    target's position is offset by any change in eye position while "VStab" is engaged. Thus, the target's trajectory 
    relative to the eye will be its precomputed trajectory as specified in the trial codes. In earlier versions of 
    Maestro, the feature was restricted to a single target and a single segment or a contiguous range of segments. As of
    Maestro v2.0.0, any trial target or targets may be stabilized during any one segment or segments of the trial.

    Here's how it works: When VStab is engaged at t=t0, let eye position = e(t0). If the target is to be "snapped" to 
    the eye at t0, we introduce an offset D1 = e(t0)-p(t0) in the target's precomputed trajectory p(t); otherwise, D1=0. 
    During the intv [t0..t1] that velocity stabilization is in effect, the modified tgt trajectory is p*(t) = p(t) + D1 
    + e(t)-e(t0), or p*(t) = p(t) - p(t0) + e(t), where p(t) is the precomputed tgt pos and e(t)-e(t0) is the net change
    in eye pos over [t0..t]. When velocity stabilization ends, the target pos should continue smoothly from wherever it 
    ended up.  Therefore, it is necessary to ADD the offset D1 + E to the precomputed trajectory p(t) for the remainder 
    of the trial -- where E = e(t1)-e(t0) is net change in eye pos over the entire open-loop period.

    The user can choose to velocity stabilize only the H component of target motion, the V cmpt only, or both H & V 
    components. If the target is snapped to the eye when velocity stabilization begins, both cmpts of target position 
    are "snapped", regardless of which component(s) are stabilized during open-loop.

    As of 16sep2005, velocity stabilization applies to the motion of the target pattern as well as the target window for
    XY scope targets. Maestro now allows stabilization on more XY scope target types:  RECTDOT, CENTER, SURROUND, 
    RECTANNU, FASTCENTER, ORIENTEDBAR, and COHERENTFC. It is not appropriate for XY target types FCDOTLIFE, FLOWFIELD, 
    NOISYDIR and NOISYSPEED.

    As of 13mar2006, velocity stabilization can now remain in effect over a contiguous span of trial segments instead of
    just one segment.

    As of 10apr2006, velocity stabilization is no longer restricted to the target designated as fixation tgt #1 during 
    the first open-loop segment and all remaining segments of the trial. Any single target may be subject to v. stab.; 
    stabilization of multiple targets is NOT supported. However, if the stabilized tgt is fix tgt#1 or fix tgt #2, it 
    must remain so from the start of v. stab until end of trial

    As of 20jan2006 (v2.5.2), most target restrictions on velocity stabilization have been relaxed: Stabilization of
    any XYScope or RMVideo target is permitted. However, some target types are really not appropriate for stabilization 
    and may not behave as intended. For example, XYScope targets with a finite dotlife specified in distance travelled 
    will not work correctly with velocity stabilization on (think: the dots won't likely have random lifetimes any more 
    if the target is adjusted to follow a saccade!). Only the target window motion of the XYScope FLOWFIELD target is 
    stabilized WRT the eye; the pattern velocity (ie, the flow velocity of the optical flow field) is unaffected. All 
    RMVideo targets are stabilized simply by adjusting the trajectory of the target window, because pattern motion is 
    specified WRT the target's center, NOT the screen center (one exception: RMV_RANDOMDOTS with RMV_F_WRTSCREEN set).

    Implementation constraints (these are enforced by Maestro, not here!): Velocity stabilization is supported for any 
    RMVideo or XYScope target.

 3) "Interleaving" of XY scope targets. The DELTAT trial code specifies the XY scope frame period (in msecs) for each 
    segment of the trial. If the interleaving feature is engaged, a single "XY frame" of DELTAT msecs is divided into N 
    "interleave slots" of DELTAT/N msecs each (must be multiple of the minimum XY frame period), where N is the # of XY 
    scope targets to be interleaved. We obtain N and the IDs of the interleaved targets by processing the XYTARGETUSED 
    trial codes that appear before any segment-related codes. Each interleaved target is assigned (in the order 
    specified by the XYTARGETUSED codes) to a different slot; all non-interleaved tgts are assigned to slot 0. During 
    the trial, the XY scope is updated every DELTAT/N msecs instead of every DELTAT msecs. The "repeat" value for the 
    target is set to 0 during those slots when it should not be drawn, and to DELTAT/MIN_XYUPDATEINTV during the slot 
    that it is drawn..... This interleaving allows us to implement composite "apparent motion" targets that are drawn 
    over the entire DELTAT interval instead of at the beginning of each DELTAT. This reduces the "flashing effect" 
    associated with the larger (>~16ms) DELTAT's. NOTE that CNTRLX is responsible for ensuring that each segment's 
    DELTAT is compatible with interleaving; we do not perform any such validation checks here.

    Implementation constraints (again, enforced by MAESTRO, not here!): Let N = # of XY tgts to be interleaved during 
    trial, and segDT(m) be the XY scope frame period during segment m. Then it must be true that: a) There are at least 
    N XY scope targets participating in the trial, not including any of the type RECTDOT, which is exempt from 
    interleaving. b) For all segments of the trial, segDT(m) % N == 0 and segDT(m)/N is a multiple of the minimum frame
    period SGH_MINXYFRAME.

 4) The XY scope target type "FLOWFIELD". This target type is intended to represent an optical flow field of random 
    dots. Unlike the other XY target types, the animation calculations for dot motion are implemented in XYCORE. To 
    simulate an optical flow of random dots, each dot's velocity varies with its radial distance from the focus of 
    expansion, expressed in units of visual angle:

         v( r ) = B * sin( r ) * cos( r ),

    where r is the visual angle between FOE and dot (assuming perpendicular line-of-sight passes through the FOE), and B 
    is a scale factor related to the velocity of dots at 1/2 the outer radius of the flow:

         B = v(r2) / (sin(r2)*cos(r2), where r2 = 0.5*rOuter.

    Given a update interval deltaT, the above velocity equation can be transformed to:

         delt_r( r ) = deltaT * v( r ) = B' * sin( r ) * cos( r ), where B' = deltaT * B.

    XYCORE uses a form of this equation to update the radial position of each dot in the flow field. All calculations 
    are done using scaled integers in order to avoid slow floating-pt computations, so XYCORE expects the following info
    in the update record for a FLOWFIELD target:
       HORIZ    ==> change in horiz coord of the FOE, in pixels
       VERT     ==> change in verti coord of the FOE, in pixels
       IHORIZ   ==> B' * 2^M, where M is adjusted in an attempt to maintain 3 digits precision in the floating-pt value 
          B'. We use powers of 2 for scaling so that the scale factors can be efficiently removed by right-shifting 
          (instead of division, which is slow!). A negative value means the dots move toward the FOE (flow is away from 
          observer). The expected units of B' are visual deg/100.
       IVERT    ==> M. Note that M can be negative for very large values of B'.
      REPEAT   ==> Same meaning as for other XY target types.

    When a flow field target is initialized at the start of a trial, the initial FOE is always at the center of the XY 
    scope display. Trial codes TARGET_*POS, TARGET_*VEL, and TARGET_*ACC are used to move the FOE. While you can move 
    the FOE smoothly during a trial, this is not really the intended use of the FLOWFIELD tgt. The INSIDE_HVEL trial
    code is used to communicate the speed of the optical flow, which we define as the speed of a dot at 1/2 the outer 
    radius. It is this value which the precomputation "engine" uses to determine the frame-to-frame value of B'. The 
    INSIDE_VVEL trial code is ignored for this tgt type.

 5) "Checking" the subject's response. MAESTRODRIVER provides support for implementing a two-choice response to a trial 
    (currently, MAESTRO only does this in conjunction w/staircase trial sequences for human psychophysics). The relevant 
    hardware consists of two debounced pushbuttons -- a "correct" choice PB and an "incorrect" one --,  the TTL outputs 
    of which are connected to two different AI channels. We assume that, when a PB is NOT depressed, the voltage on the 
    corresponding AI channel is much less than 2.0V; when depressed, it should be much greater than 2.0V. We also assume
    we do not have to be concerned about noise -- so that one sample > 2.0V is sufficient to "decide" that a PB was 
    depressed.

    The CHECKRESPON/CHECKRESPOFF trial codes bracket segment(s) of the trial during which we monitor the subject's
    response. We refer to the union of these segments as the "valid check window". The CHECKRESPON code group includes 
    the identities of the two AI channels corresponding to the correct and incorrect responses. A subject's response to 
    the trial is correct only if: (i) the "correct" PB is depressed at some point within the valid check window; AND 
    (ii) the "incorrect" PB is NOT depressed at any time within the valid check window. If the subject fails to respond 
    at all, the trial result flag bit CX_FT_NORESP is set to indicate that fact.

 6) "Mid-trial Rewards". [25jan2005] Formerly, mid-trial rewards were only delivered periodically during enabled 
 segments, using the fixation duration and reward 1 pulse length as defined by the CX_FIXREWSETTINGS command for the 
 reward interval and pulse length. Now, these are defined by trial header parameters on the GUI and are defined by the 
 new MIDTRIALREW trial code. In addition, a second mode of delivery is defined: when the reward interval specified by 
 the MIDTRIALREW code group is <= 0, then the mid-trial rewards are delivered at the end of each enabled segment, rather
 than at a regular interval.

 When periodic mid-trial rewards are enabled for the last segment of the trial, no attempt is made to skip a reward
 delivered close to the trial's end, where it would conflict with the end-of-trial reward. User is responsible for
 designing the trial correctly. If an end-of-segment mid-trial reward is enabled for the last segment, that mid-trial 
 reward will NEVER be delivered. The end-of-trial reward is ALWAYS delivered if fixation requirements were satisfied 
 throughout the trial.

 7) (11may2011) Motion of XYScope target patterns WRT target window. As of Maestro v2.7.0, the target pattern velocities
 that the user specifies in the GUI's trial table will indicate the pattern motion WRT the target window rather than the
 XYScope screen. This is how RMVideo's "Random-Dot Patch" target works in general. The change also simplifies velocity 
 stabilization of XYScope targets. A number of miscellaneous changes were made in this function to account for this 
 "frame of reference" change. XYCORE, of course, was updated accordingly.

 8) (04sep2013) Revision of terminal behavior when trial ends prematurely due to user abort or runtime error.
    -- The animal is no longer rewarded (prior to this change, it was). Runtime errors (including RMVideo frame drops)
 are exceedingly rare. The most common cause of a premature termination other than lost fixation is user abort, and
 users typically stop the trial if the animal is not doing the task.
    -- The trial file will NOT be saved if it stops prematurely due to user abort OR a runtime error, even if the
 failsafe time is set and was exceeded. Prior to this change, the file was saved after a runtime error if the failsafe
 time was exceeded, and this caused confusion.
    -- If the trial terminates prematurely on an RMVideo error (most likely a duplicate or skipped frame error), a 
 logged error message indicates how many such errors have occurred since startup.

 9) (20mar2019) Revisions related to RMVideo V9. Based on extensive testing with a 144Hz RMVideo display, it is likely
 that occasional duplicate frames will occur due to a rendering delay on the RMVideo side. In order to support higher
 refresh rates in the future, we made two changes: (a) Maestro user can elect to allow as many as 3 duplicate frames
 over the course of a trial. This option is communicated to CXDRIVER via an IPC flag -- see CCxMasterIO function
 AllowRMVDuplicateFramesDuringTrial(). (b) A new trial result flag was introduced, CX_FT_RMVDUPE. If too many duplicate
 frames are detected -- 0 or more than 3, depending on whether the option in (a) is enabled --, then the trial is 
 aborted and the CX_FT_RMVDUPE flag is set. Rather than aborting the trial sequence (as in the past), Maestro will 
 discard the trial data and repeat the trial as appropriate to the current sequencing mode.
    Other changes: (c) Mods IAW changes in CCxRMVideo interface. (d) RMVideo refresh rate is now stored in data file 
 header in micro-Hz instead of milli-Hz, for greater accuracy. (e) When up to 3 duplicate frames are tolerated, info
 about the duplicate frame events are stored in the trial data file. (f) A warning message is delivered once if 
 CXDRIVER gets 3 frames ahead of the RMVideo timeline. Ideally, it should stay 2 frames ahead, but any inaccuracy in
 the refresh period measured on the RMVideo side would cause CXDRIVER to fall behind or get ahead over time; falling
 behind will result in duplicate frames, while getting ahead would result in target update commands building up in
 RMVideo's network receive buffer.

 10) (20may2019) Maestro V4.1.0 introduced random reward withholding variable ratio feature for individual trials.
 Much of the feature is implemented on the Maestro side, but a withheld reward (for either reward pulse) is signaled
 to CXDRIVER by setting the relevant pulse length to 0 in the REWARDLEN trial code group. No changes were needed here,
 just a minor change in CCxEventTimer::DeliverReward(). Analysis programs can detect whether or not reward is withheld
 by checking for a reward pulse length of zero in the data file header. Note that this feature should not be used 
 simultaneously with the reward withholding variable ratio that's declared in the global fixation/reward settings in 
 CXIPC. When using per-trial WHVR, the global WHVR setting should be disabled. When a reward is earned yet withheld, 
 the field CXFILEHDR.flags will contain CXHF_REWARDEARNED but not CXHF_REWARDGIVEN. The reward pulse lengths are 
 stored in CXFILEHDR.iRewLen1, .iRewLen2. For per-trial WHVR only, the pulse length will be zero if the reward was
 withheld.

 11) (02nov2022) Binocular fixation checking for "stereo" experiments. As of Maestro V4.2.0, binocular fixation 
 checking is possible under specific conditions: (a) The EyeLink tracker is used in binocular mode to stream left
 (HGPOS, VEPOS) and right (HGPOS2, VEPOS2) eye position data to Maestro during the trial. (b) In any segment in which
 binocular fixation requirements are to be enforced, both Fix1 and Fix2 must be defined. (c) No special operation can
 be used during the trial, with the exception of the searchTask feature. Many of these special operations use Fix1 and
 Fix2 in unusual ways.
 
 If enabled, the binocular fixation check will compare the L eye to Fix1 and the R eye to Fix2. If only Fix1 is defined,
 then the normal fixation check occurs. If only Fix2 is defined, NO fixation check occurs -- so you can't check only 
 fixation for the R eye.

 12) (24sep2024) Removing all XYScope-related code for Maestro 5.x. The XYScope platform and related hardware devices 
 have not been supported since the release of Maestro 4.0. Historical comments above relevant to XYScope may be ignored.
 Leaving XYScope-related typedefs, constants and flag bits alone to make sure we don't break code.

 13) (04nov2024) Added new special op "selDurByFix". Similar to "selByFix", except that the target selection during the
 special segment determines the DURATION of the subsequent segment. For this special op only, Maestro delivers a new
 trial code group, SEGDURS, only for the segment after the special segment. This code group specifies the minimum and 
 maximum duration for the segment. The minimum duration will be used if the Fix1 target is selected, or the maximum
 duration if Fix2 is selected. Maestro prepares all remaining trial codes UNDER THE ASSUMPTION that the maximum 
 duration is used, so CXDRIVER must adjust the elapsed start times for all remaining segments if Fix1 gets selected!
 If Fix2 is selected, then no adjustment is needed.


 @returns The trial result (some combination of IPC flag bits (CX_FT_DONE, etc.).
======================================================================================================================*/
// The following flag bits are used only in this fcn!!!!
//
const DWORD T_USERMV     = ((DWORD) (1<<0)); // RMVideo is used for target display during trial
const DWORD T_USEXY     = ((DWORD) (1<<1));  // [deprecated] XY scope video is used for target display during trial
const DWORD T_USECHAIR  = ((DWORD) (1<<2));  // "hard" target CX_CHAIR is used during the trial
const DWORD T_USEAO     = ((DWORD) (1<<5));  // AO device is required during trial
const DWORD T_CHECKSACC = ((DWORD) (1<<7));  // enables saccade checking during one of the saccade-related special ops
const DWORD T_ISSKIP    = ((DWORD) (1<<8));  // the special operation in effect is "skipOnSaccade"
const DWORD T_ISFIX1    = ((DWORD) (1<<9));  // the special operation in effect is "selectByFix"
const DWORD T_ISFIX2    = ((DWORD) (1<<10)); // the special operation in effect is "selectByFix2"
const DWORD T_SKIPPED   = ((DWORD) (1<<11)); // part of special segment was skipped b/c of "skipOnSaccade"
const DWORD T_SELECTED  = ((DWORD) (1<<12)); // tgt selected during "selByFix" or "chooseFix" op
const DWORD T_ENDSEL    = ((DWORD) (1<<13)); // set if tgt selection forced at end of special seg in "selByFix" ops
const DWORD T_INSACCADE = ((DWORD) (1<<14)); // in the midst of a saccade during "selectByFix" segment
const DWORD T_DELAYSKIP = ((DWORD) (1<<15)); // "skipOnSaccade" action delayed til next tick
const DWORD T_ISSEARCH  = ((DWORD) (1<<16)); // the special operation in effect is "searchTask"
const DWORD T_SOUGHT    = ((DWORD) (1<<17)); // "searchTask" op only: if set, then eye vel >= sacc thresh during task
const DWORD T_CHKRESP   = ((DWORD) (1<<18)); // subject resp checking enabled for at least one seg of trial
const DWORD T_HITOKPB   = ((DWORD) (1<<19)); // if set, the correct resp channel was activated during a checkresp seg
const DWORD T_ISCHFIX1  = ((DWORD) (1<<20)); // the special operation in effect is "choose Fix #1"
const DWORD T_ISCHFIX2  = ((DWORD) (1<<21)); // the special operation in effect is "choose Fix #2"
const DWORD T_ISCHFIX   = T_ISCHFIX1|T_ISCHFIX2;
const DWORD T_ISSWFIX   = ((DWORD) (1<<22)); // special operation in effect is "switchFix"
const DWORD T_ISRPDIST  = ((DWORD) (1<<23)); // special operation in effect is "R/P Distro"
const DWORD T_HASRPDWIN = ((DWORD) (1<<24)); // for "R/P Distro" operation, this flag set if reward window(s) defined
const DWORD T_RPDPASS   = ((DWORD) (1<<25)); // for "R/P Distro": set flag if behav resp inside a reward window.
const DWORD T_ST_2GOAL  = ((DWORD) (1<<26)); // "search task" op has 2 goal targets, Fix1 and Fix2.
const DWORD T_ISSELDUR = ((DWORD)(1 << 27)); // the special op in effect is "selDurByFix".
const DWORD T_ISFIX    = T_ISFIX1 | T_ISFIX2 | T_ISSELDUR; // mask for the "sel*ByFix*" variants

DWORD RTFCNDCL CCxDriver::ExecuteSingleTrial()
{
   // for general use
   int i, j;
   CFPoint fPt1;
   CFPoint fPt2;
   float f1, f2;

   // set suspend duty cycle: 1ms, 20% suspended; save old suspend params for restoring upon exit
   int iOldOn, iOldOff;
   m_suspendMgr.ChangeTiming(800, 200, &iOldOn, &iOldOff); 

   // bypass suspend manager during precomputation phase -- we want this done as quickly as possible
   m_suspendMgr.Bypass(); 

   // reset elapsed trial time in IPC shared memory
   m_masterIO.SetLastTrialLen(0);

   // BEGIN INITIALIZATION PHASE: Initialize the trajectory state record for all targets participating in the trial,
   // and ensure that the hardware required to realize the trial targets is available in the system.
   //
   int nRMVTgts = 0;                                                    // # of RMVideo tgts participating in trial
   DWORD dwTrialRes = 0;                                                // trial result communicated to CNTRLX via IPC
   DWORD dwFlags = 0;                                                   // trial status flags (state of T_* flag bits)
   
   // FALSE = abort trial on first duplicate frame; TRUE = abort after 4th duplicate frame
   int nRMVDupesAllowed = m_masterIO.AllowRMVDuplicateFramesDuringTrial() ? 3 : 0;

   // ideally, CXDRIVER must send a target motion update to RMVideo two full video frame periods, so CXDRIVER must lead
   // RMVideo by 2-3 video frames over the course of the trial. Once that lead reaches 4 frames, a warning message is 
   // delivered whenever the lead grows by another frame. We can calculate the lead once per second, when RMVideo sends
   // a message with its current animation frame count.
   int nRMVFramesAhead = 4;

   // get #targets in trial and #codes defining trial. Abort if these are out of bounds (restore suspend mgr timing!).
   int nTgs = m_masterIO.GetNumTrialTargets(); 
   int nCodes = m_masterIO.GetNumTrialCodes(); 
   if(nTgs <= 0 || nTgs > MAX_TRIALTARGS || nCodes <= 0 || nCodes > CX_MAXTC)
   {
      m_masterIO.Message("(!!) Current trial is ill-defined!");
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // prepare target trajectory state information for each target participating in the trial
   int idxChair = -1;
   CXTARGET tgt; 
   for(i = 0; i < nTgs; i++)
   {
      // check target type to see what hardware is required. Abort if target type is not recognized
      m_masterIO.GetTrialTarget(i, tgt); 
      if(tgt.wType == CX_CHAIR) { dwFlags |= (T_USEAO | T_USECHAIR); idxChair = i; }
      else if(tgt.wType == CX_RMVTARG) dwFlags |= T_USERMV;
      else 
      {
         m_masterIO.Message("(!!) Trial target type not recognized or no longer supported!");
         m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
         return(CX_FT_ERROR | CX_FT_DONE);
      }

      CTrialTraj* pTraj = &(m_traj[i]);
      pTraj->wType = tgt.wType;
      pTraj->iSubType = -1;
      pTraj->iFlags = 0;
      if( tgt.wType == CX_RMVTARG ) 
      {
         pTraj->iSubType = tgt.u.rmv.iType;
         pTraj->iFlags = tgt.u.rmv.iFlags;
      }
      
      pTraj->pos.Zero();
      pTraj->prevPos.Zero();
      pTraj->vel.Zero();
      pTraj->prevVel.Zero();
      pTraj->pertVelDelta.Zero();
      pTraj->acc.Zero();
      pTraj->prevAcc.Zero();
      pTraj->patVel.Zero();
      pTraj->prevPatVel.Zero();
      pTraj->pertPatVelDelta.Zero();
      pTraj->patAcc.Zero();
      pTraj->prevPatAcc.Zero();
      pTraj->ptPosWin.Zero();
      pTraj->ptPosPat.Zero();
      pTraj->remDotLife = 0.0f;

      // init ordinal pos of each target in RMVideo's animated target list.
      pTraj->iUpdatePos = -1; 
      if(tgt.wType == CX_RMVTARG)
      {
         pTraj->iUpdatePos = nRMVTgts;
         ++nRMVTgts;
      }

      pTraj->bIsOn = FALSE;
      pTraj->bIsMoving = FALSE;
   }

   // if any required hardware is missing, abort trial
   if( (!m_masterIO.IsAIAvailable()) || (!m_masterIO.IsTMRAvailable()) || 
       ((dwFlags & T_USEAO) && !m_masterIO.IsAOAvailable()) || ((dwFlags & T_USERMV) && !m_masterIO.IsRMVAvailable())
     )
   {
      m_masterIO.Message("(!!) At least one hardware device needed to run trial is not available!");
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // zero buffers used to store current update vectors for any RMVideo targets
   ::memset(&(m_RMVUpdVecs[0]), 0, 3*MAX_TRIALTARGS*sizeof(RMVTGTVEC));

   // reset perturbation manager: no perturbations in use
   m_pertMgr.Reset();

   // pointers to our hardware device objects
   CCxEventTimer* pTimer = m_DevMgr.GetTimer(); 
   CCxAnalogIn* pAI = m_DevMgr.GetAI(); 
   CCxAnalogOut* pAO = m_DevMgr.GetAO();
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   
   // for integration w/external system: write "start" char, followed by null-terminated trial name and trial data file
   // name (NOT the full path). If null data file, write special "noDataFile" code instead, still followed by null-term.
   pTimer->WriteChar(START_CHARCODE);
   m_masterIO.GetProtocolName(m_string, CX_MAXPATH);
   pTimer->WriteString(m_string, (int) (::strlen(m_string) + 1));
   m_masterIO.GetDataFileName(m_string, CX_MAXPATH);
   if(::strlen(m_string) > 0) 
      pTimer->WriteString(m_string, (int) (::strlen(m_string) + 1));
   else
   {
      pTimer->WriteChar(NOFILE_CHARCODE);
      pTimer->WriteChar(0);
   }
   //
   // END INITIALIZATION PHASE

   // BEGIN PREPROCESSING PHASE:  Preprocess all trial codes to prepare a representation of the segmented trial that we
   // will use to compute all target trajectories on the fly as the trial is running. 
   //
   // To fully understand the processing of individual trial codes, please review the documentation in CXTRIALCODES.H!
   //
   
   // AI scan interval = 1 trial "tick" = 1ms; tick in second in H&V, for trajectory calcs
   m_viScanInterval = 1; 
   const CFPoint dT = CFPoint(0.001 * m_viScanInterval);  

   int nSegs = 0;                                                       // # of segments in trial
   int iCurrSeg = -1;                                                   // the current segment
   int iSaveSeg = -1;                                                   // if >= 0, save data rec from this seg onward
   int iSaccSeg = -1;                                                   // if >= 0, special op occurs in this seg

   // if trial stops before this tick, data is NOT saved; if -1, trial must go to competion!
   int iFailsafeTime = -1;

   // saccade threshold vel in raw AI dev units
   int iSaccThresh = 0;

   // "R/P Distro" only: behavorial response, response type, two reward windows ([min..max]; ignore if min==max==0).
   float fBehavResp = 0.0f; 
   int iBehavRespType = -1; 
   float fRPDWindow[4]; 
   for(i=0; i<4; i++) fRPDWindow[i] = 0.0f;  

   // "selDurByFix" only: the min and maxx durations of the segment AFTER the special segment (SEGDURS trial code)
   int selectSegDurMin = 0;
   int selectSegDurMax = 0;

   // init reward pulse lengths; mid-trial reward pulse length and intv (intv<=0: deliver at END of each enabled seg).
   // NOTE: Reward pulse 1 or 2 may be zero-length -- indicating that reward is withheld even if fixation rqmts met.
   int nRewPulse1 = TH_DEFREWLEN; 
   int nRewPulse2 = TH_DEFREWLEN; 
   int nMTRLen = TH_DEFREWLEN;
   int nMTRIntv = TH_DEFREWINTV;

   int nTrialTime = 0;                                                  // the current elapsed trial time in "ticks"
   int iCode = 0;                                                       // index of current trial code being processed
   TRIALCODE tc = m_masterIO.GetTrialCode(iCode);                       // get the first trial code

   TRIALCODE pertTC[5];                                                 // set of trial codes defining a perturbation
   CTrialSeg* pSeg = NULL;                                              // ptr to the state vars for curr trial segment
   CTrialTraj* pTraj = NULL;                                            // ptr to a trial target's trajectory record

   BOOL bDone = FALSE;
   double dDummy;
   do
   {
      // BEGIN:  CHECK FOR NEW SEGMENT AND PROCESS ITS TRIAL CODES. All trial codes -- with two exceptions -- are
      // associated with the start of a trial segment. The trial code processing here depends on it!! One exception is
      // ENDTRIAL, which marks the end of the last segment of the trial. The other is FIXACCURACY, which may be
      // associated with a trial time in the middle of a segment in order to specify a "grace  period".
      //
      // So -- except in the cases noted above -- this code section is executed only at segment boundaries.
      //
      if((tc.time == nTrialTime) && (tc.code != ENDTRIAL) && (tc.code != FIXACCURACY))
      {
         // advance to next segment. Abort if there are too many. Note terminal sequence to integrate w/external system.
         ++nSegs; 
         if(nSegs > MAX_SEGMENTS) 
         {
            m_masterIO.Message("(!!) Too many segments in trial... aborting.");
            pTimer->WriteChar(ABORT_CHARCODE);
            pTimer->WriteChar(STOP_CHARCODE);
            m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
            return(CX_FT_ERROR | CX_FT_DONE);
         }
         ++iCurrSeg;
         pSeg = &(m_seg[iCurrSeg]);

         // preserve segment's start time and init state variables for the segment. Initially, there's no grace period
         // but the fixation window is very large (fixation not enforced). Otherwise, most state variables are inherited
         // from the previous segment.
         pSeg->tStart = nTrialTime; 
         pSeg->iPulseOut = -1;
         pSeg->fpFixAcc.Set(300.0, 300.0);
         pSeg->tGrace = nTrialTime;
         pSeg->bTrigRMVSyncFlash = FALSE;
         if(nSegs > 1)
         {
            pSeg->iCurrFix1 = m_seg[iCurrSeg-1].iCurrFix1;
            pSeg->iCurrFix2 = m_seg[iCurrSeg-1].iCurrFix2;
            pSeg->bCheckResp = m_seg[iCurrSeg-1].bCheckResp;
            pSeg->bRewEna = m_seg[iCurrSeg-1].bRewEna;
            pSeg->iChOk = m_seg[iCurrSeg-1].iChOk;
            pSeg->iChWrong = m_seg[iCurrSeg-1].iChWrong;

            // per-target information: trajectory variables and VStab. Note that VStab may continue over seg boundary.
            // IMPORTANT: Instantaneous target position change is NOT inherited from previous segment; TARGET_*POS* will
            // be sent only if the position change at a segment's start is absolute or if it's nonzero and relative. SO,
            // we must zero target position and set the TF_TGTREL flag at each segment start
            for(i=0; i<nTgs; i++) 
            {
               pSeg->tgtFlags[i] = m_seg[iCurrSeg-1].tgtFlags[i] | TF_TGTREL;
               pSeg->tgtPos[i].Zero();
               pSeg->tgtVel[i] = m_seg[iCurrSeg-1].tgtVel[i];
               pSeg->tgtAcc[i] = m_seg[iCurrSeg-1].tgtAcc[i];
               pSeg->tgtPatVel[i] = m_seg[iCurrSeg-1].tgtPatVel[i];
               pSeg->tgtPatAcc[i] = m_seg[iCurrSeg-1].tgtPatAcc[i];
            }
         }
         else
         {
            pSeg->iCurrFix1 = -1;
            pSeg->iCurrFix2 = -1;
            pSeg->bCheckResp = FALSE;
            pSeg->bRewEna = FALSE;
            pSeg->iChOk = 12;
            pSeg->iChWrong = 13;

            // at beginning of trial: all targets are off, at the origin, and not moving; VStab is off.
            for(i=0; i<nTgs; i++) 
            {
               pSeg->tgtFlags[i] = TF_TGTREL;
               pSeg->tgtPos[i].Zero();
               pSeg->tgtVel[i].Zero();
               pSeg->tgtAcc[i].Zero();
               pSeg->tgtPatVel[i].Zero();
               pSeg->tgtPatAcc[i].Zero();
            }
         }
      }

      while((!bDone) && (tc.time == nTrialTime))
      {
         // BEGIN: Process next trial code group (possibly more than one code!)
         switch(tc.code) 
         { 
            case STARTTRIAL :
               break;

            case ENDTRIAL : // NOTE:  this is ALWAYS the last trial code!
               bDone = TRUE;
               break;

            case REWARDLEN :
               tc = m_masterIO.GetTrialCode( ++iCode );
               nRewPulse1 = (int) tc.code;
               nRewPulse2 = (int) tc.time;
               break;

            case MIDTRIALREW :
               tc = m_masterIO.GetTrialCode( ++iCode );
               nMTRIntv = (int) tc.code;
               nMTRLen = (int) tc.time;
               break;

            // ignored for CHAIR, which is not visual and is considered to be "on" at all times
            case TARGET_ON : 
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]);
               if(pTraj->wType == CX_RMVTARG) pSeg->tgtFlags[tc.code] |= TF_TGTON;
               break;
            case TARGET_OFF :
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]);
               if(pTraj->wType == CX_RMVTARG) pSeg->tgtFlags[tc.code] &= ~TF_TGTON;
               break;

            // these pertain to target "pattern" velocity, applicable only to RMVideo targets...
            case INSIDE_HVEL : 
            case INSIDE_HSLOVEL : 
            case INSIDE_VVEL : 
            case INSIDE_VSLOVEL :
               i = (int) tc.code;
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]); 

               // convert encoded velocity to deg/sec and save in target's state for the current segment
               dDummy = (double) tc.time; 
               dDummy /= ((i==INSIDE_HVEL || i==INSIDE_VVEL) ? d_TC_STDSCALE : d_TC_SLOSCALE1);

               if(i == INSIDE_HVEL || i == INSIDE_HSLOVEL) pSeg->tgtPatVel[tc.code].SetH(dDummy);
               else pSeg->tgtPatVel[tc.code].SetV(dDummy);

               break;

            // these pertain to target "pattern" acceleration, applicable only to the RMVideo targets...
            case INSIDE_HACC : 
            case INSIDE_HSLOACC : 
            case INSIDE_VACC : 
            case INSIDE_VSLOACC :
               i = (int) tc.code;
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]); 

               // convert encoded acceleration to deg/sec^2 and save in target's state for the curent segment
               dDummy = (double) tc.time; 
               if(i == INSIDE_HSLOACC || i == INSIDE_VSLOACC) dDummy /= d_TC_SLOSCALE2;

               if(i == INSIDE_HACC || i == INSIDE_HSLOACC) pSeg->tgtPatAcc[tc.code].SetH(dDummy);
               else pSeg->tgtPatAcc[tc.code].SetV(dDummy);

               break;

            case TARGET_HVEL : 
            case TARGET_HSLOVEL :
            case TARGET_VVEL :
            case TARGET_VSLOVEL :
               i = (int) tc.code;
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]); 

               // convert encoded velocity to deg/sec and save in target's state for the current segment
               dDummy = (double) tc.time; 
               dDummy /= ((i==TARGET_HVEL || i==TARGET_VVEL) ? d_TC_STDSCALE : d_TC_SLOSCALE1);

               if(i == TARGET_HVEL || i == TARGET_HSLOVEL) pSeg->tgtVel[tc.code].SetH(dDummy);
               else pSeg->tgtVel[tc.code].SetV(dDummy);

               break;

            case TARGET_HACC : 
            case TARGET_HSLOACC :
            case TARGET_VACC :
            case TARGET_VSLOACC :
               i = (int) tc.code;
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]); 

               // convert encoded acceleration to deg/sec^2 and save in target's state for the current segment
               dDummy = (double) tc.time; 
               if(i == TARGET_HSLOACC || i == TARGET_VSLOACC) dDummy /= d_TC_SLOSCALE2;

               if(i == TARGET_HACC || i == TARGET_HSLOACC) pSeg->tgtAcc[tc.code].SetH(dDummy);
               else pSeg->tgtAcc[tc.code].SetV(dDummy);

               break;

            // ignored for CHAIR, which cannot be instantaneously repositioned. NOTE that if you get HPOSREL, you won't
            // get VPOSABS; ie, the REL/ABS flag cannot be independently applied to H,V.
            case TARGET_HPOSREL :
            case TARGET_HPOSABS :
            case TARGET_VPOSREL :
            case TARGET_VPOSABS :
               i = (int) tc.code;
               tc = m_masterIO.GetTrialCode( ++iCode );
               pTraj = &(m_traj[tc.code]);
               if(pTraj->wType == CX_CHAIR) break;

               // convert encoded position to degrees and update target's trajectory record with the new position. NOTE
               // that when we make an absolute position change, we must clear the previous target velocity component.
               dDummy = ((double) tc.time) / d_TC_SLOSCALE2; 
               switch(i)
               {
                  case TARGET_HPOSREL : 
                     pSeg->tgtFlags[tc.code] |= TF_TGTREL; 
                     pSeg->tgtPos[tc.code].SetH(dDummy); 
                     break;
                  case TARGET_VPOSREL : 
                     pSeg->tgtFlags[tc.code] |= TF_TGTREL; 
                     pSeg->tgtPos[tc.code].SetV(dDummy); 
                     break;
                  case TARGET_HPOSABS : 
                     pSeg->tgtFlags[tc.code] &= ~TF_TGTREL; 
                     pSeg->tgtPos[tc.code].SetH(dDummy); 
                     break;
                  case TARGET_VPOSABS : 
                     pSeg->tgtFlags[tc.code] &= ~TF_TGTREL; 
                     pSeg->tgtPos[tc.code].SetV(dDummy); 
                     break;
               }

               break;

            // perturbation waveform. Read in the defining codes and pass them to the perturbation mgr.
            case TARGET_PERTURB :
               pertTC[0] = tc;
               for(i = 1; i <= 4; i++) pertTC[i] = m_masterIO.GetTrialCode(++iCode);

               pTraj = &(m_traj[pertTC[1].code]); 
               m_pertMgr.ProcessTrialCodes(&(pertTC[0])); 
               break;

            // segment marker pulse on DO<1..11>. Optionally start RMVideo vertical sync spot flash ('time1' != 0).
            case PULSE_ON : 
               tc = m_masterIO.GetTrialCode(++iCode);
               if(tc.code >= 1 && tc.code <= 11) pSeg->iPulseOut = (int) tc.code;
               if(tc.time != 0) pSeg->bTrigRMVSyncFlash = TRUE;
               break;

            // change tgt designated as fixation tgt #1; also used to set the "mid-trial rewards" enable flag
            case FIXEYE1 :  
               tc = m_masterIO.GetTrialCode(++iCode); 
               pSeg->iCurrFix1 = tc.code;
               pSeg->bRewEna = BOOL(tc.time != 0);
               break;
            // change tgt designated as fixation tgt #2
            case FIXEYE2 : 
               tc = m_masterIO.GetTrialCode(++iCode);
               pSeg->iCurrFix2 = tc.code;
               break;

            // H,V fixation accuracy and grace period for segment. If grace period is nonzero, a second FIXACCURACY
            // code group is sent mid-segment! Accuracy specified in unit of deg/100; converted to degrees.
            case FIXACCURACY : 
               tc = m_masterIO.GetTrialCode( ++iCode );
               pSeg->fpFixAcc.Set(0.01f * cMath::abs(float(tc.code)), 0.01f * cMath::abs(float(tc.time)));
               pSeg->tGrace = nTrialTime;
               break;

            // save target's VStab state for the current segment.
            case TARGET_VSTAB : 
               tc = m_masterIO.GetTrialCode(++iCode);
               pSeg->tgtFlags[tc.code] &= ~(VSTAB_MASK);
               pSeg->tgtFlags[tc.code] |= (WORD) (VSTAB_MASK & tc.time);
               break;

            // mark segment at which we start recording and saving data (NOTE: ADCOFF obsolete).
            case ADCON : 
               iSaveSeg = iCurrSeg; 
               break;

            // perform one of the special feature operations during this segment
            case SPECIALOP : 
               iSaccSeg = iCurrSeg; 
               tc = m_masterIO.GetTrialCode(++iCode);
               i = tc.code & 0x00FF;                                    // RPD resp type may be in bits 15..8!
               if(i == SPECIAL_SKIP) dwFlags |= T_ISSKIP;
               else if(i == SPECIAL_SWITCHFIX) dwFlags |= T_ISSWFIX;
               else if(i == SPECIAL_CHOOSEFIX1)  dwFlags |= T_ISCHFIX1;
               else if(i == SPECIAL_CHOOSEFIX2) dwFlags |= T_ISCHFIX2;
               else if(i == SPECIAL_FIX) dwFlags |= T_ISFIX1;
               else if(i == SPECIAL_FIX2) dwFlags |= T_ISFIX2;
               else if(i == SPECIAL_RPDISTRO)
               {
                  // RPDistro response type is in bits 15..8 of second trial code. Disable if resp type invalid.
                  dwFlags |= T_ISRPDIST;
                  iBehavRespType = (int)(tc.code >> 8);
                  if(iBehavRespType < 0 || iBehavRespType >= TH_RPD_NRESPTYPES) dwFlags &= ~T_ISRPDIST;
               }
               else if(i == SPECIAL_SEARCH) dwFlags |= T_ISSEARCH;
               else if(i == SPECIAL_SELDURBYFIX) dwFlags |= T_ISSELDUR;
               
               // saccade threshold velocity in deg/sec. Use absolute value and convert to raw ADC code
               iSaccThresh = tc.time; 
               if(iSaccThresh < 0) iSaccThresh = -iSaccThresh; 
               iSaccThresh = (int) (VEL_TOAIRAW * ((float)iSaccThresh));
               break;

            // the two alternate durations assigned to the segment AFTER the special segment in "selDurByFix" op ONLY.
            // THIS SHOULD ONLY BE SENT FOR a "selDurByFix" trial, and ONLY for the seg AFTER the special seg
            case SEGDURS:
               tc = m_masterIO.GetTrialCode(++iCode);
               if(((dwFlags & T_ISSELDUR) != 0) && (iSaccSeg > -1) && (iCurrSeg == iSaccSeg + 1))
               {
                  selectSegDurMin = tc.code;  // min duration in ms - used if Fix1 selected during special segment
                  selectSegDurMax = tc.time;  // max duration in ms - used if Fix2 selected during special segment
               }
               break;

            // define reward windows for the R/P Distro special feature
            case RPDWINDOW : 
               dwFlags |= T_HASRPDWIN;
               tc = m_masterIO.GetTrialCode( ++iCode );
               fRPDWindow[0] = (float) (((double) tc.code) / d_TC_STDSCALE);
               fRPDWindow[1] = (float) (((double) tc.time) / d_TC_STDSCALE);
               tc = m_masterIO.GetTrialCode( ++iCode );
               fRPDWindow[2] = (float) (((double) tc.code) / d_TC_STDSCALE);
               fRPDWindow[3] = (float) (((double) tc.time) / d_TC_STDSCALE);
               break;

            // current segment's start time becomes the failsafe time
            case FAILSAFE :
               iFailsafeTime = nTrialTime;
               break;

            // enable/disable monitoring of subject's response during current segment (staircase sequencing)
            case CHECKRESPON : 
               tc = m_masterIO.GetTrialCode( ++iCode );
               pSeg->bCheckResp = TRUE;
               pSeg->iChOk = (int) tc.code;
               pSeg->iChWrong = (int) tc.time;
               dwFlags |= T_CHKRESP;
               break;
            case CHECKRESPOFF : 
               pSeg->bCheckResp = FALSE; 
               break;

            // unrecognized trial code -- ABORT!
            default : 
               m_masterIO.Message("(!!) Unrecognized trial code!  Trial is ill-defined.");
               pTimer->WriteChar(ABORT_CHARCODE);
               pTimer->WriteChar(STOP_CHARCODE);
               m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
               return(CX_FT_ERROR | CX_FT_DONE);
         }
         // END: process next trial code group...

         // proceed to next trial code group
         ++iCode; 
         if(iCode < nCodes) tc = m_masterIO.GetTrialCode(iCode);
      }
      //
      // END:  CHECK FOR NEW SEGMENT AND PROCESS ITS TRIAL CODES.

      // advance to next trial "tick"
      ++nTrialTime; 
   } while(!bDone);
   
   // BEGIN: PREPARE FIRST THREE RMVIDEO FRAMES
   // When we begin animation on RMVideo, we must send it video frames 0 and 1 before starting the trial. Once animation
   // has begun (frame 0 displayed), we must immediately send frame 2; thereafter, we just send frame N+2 at the start 
   // of frame N. Thus, RMVideo target computations are always about two RMVideo frame periods ahead of the all other
   // target computations during trial runtime. The update vectors for the first three video frames are prepared here. 
   // We assume that a trial will at least be longer than 3 RMVideo frame periods -- a reasonable assumption!
   //
   // RMVideo trial time in ms. During the trial, this will always "lead" the actual trial time by ~2 video frames.
   int nRMVLeadTime = 0;
   // which slot if RMVideo target update vector buffer is being prepared currently (there are 3 slots in buffer).
   int iRMVFrameSlot = 0;
   // RMVideo monitor frame period in ms (nanosec resolution); start of next RMVideo frame in ms
   double dRMVFramePerMS = pRMV->GetFramePeriod() * 1000.0; 
   double dRMVNextUpdateMS = 0; 
   if(nRMVTgts > 0)
   {
      iCurrSeg = -1;
      pSeg = NULL;
      while(nRMVLeadTime < 2*dRMVFramePerMS)
      {
         // when we reach the start of a new segment, update trajectory variables for RMVideo targets
         if((iCurrSeg+1 < nSegs) && (m_seg[iCurrSeg+1].tStart == nRMVLeadTime))
         {
            ++iCurrSeg;
            pSeg = &(m_seg[iCurrSeg]);
            for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
            {
               pTraj = &(m_traj[i]);
               
               pTraj->bIsOn = (pSeg->tgtFlags[i] & TF_TGTON) != 0;
               if((pSeg->tgtFlags[i] & TF_TGTREL) != 0) pTraj->pos.Offset(pSeg->tgtPos[i]);
               else
               {
                  pTraj->pos = pSeg->tgtPos[i];
                  pTraj->prevVel.Zero();
               }
               pTraj->vel = pSeg->tgtVel[i];
               pTraj->acc = pSeg->tgtAcc[i];
               pTraj->patVel = pSeg->tgtPatVel[i];
               pTraj->patAcc = pSeg->tgtPatAcc[i];
            }
         }
         
         // piecewise integrate RMVideo target trajectories over the previous tick to compute values for current tick
         // NOTE: This is OK at t==0 because vel and acceleration and pos are all zero for the "previous" tick.
         for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
         {
            pTraj = &(m_traj[i]);

            // P(T) = P(T-1) + V(T-1) * dT; V(T) = V(T-1) + A(T-1) * dT
            pTraj->pos += pTraj->prevVel * dT;
            pTraj->vel += pTraj->prevAcc * dT;
            pTraj->patVel += pTraj->prevPatAcc * dT;
         
            // modulate nominal velocity vectors by any installed perturbations
            m_pertMgr.Perturb(i, nRMVLeadTime, pTraj->vel, pTraj->patVel, pTraj->pertVelDelta, pTraj->pertPatVelDelta);
            pTraj->vel += pTraj->pertVelDelta;
            pTraj->patVel += pTraj->pertPatVelDelta;

            // update motion of tgt window and pattern during 1ms tick. For RMV_RANDOMDOTS with RMV_F_WRTSCREEN set,
            // tgt pattern motion is WRT screen center not target center. In this case, pattern is adjusted to account
            // for a large displacement of target window at the segment boundary
            pTraj->ptPosWin += (pTraj->pos - pTraj->prevPos); 
            pTraj->ptPosPat += pTraj->prevPatVel * dT; 
            if((pSeg->tStart == nRMVLeadTime) && (pTraj->iSubType == RMV_RANDOMDOTS) && 
               ((pTraj->iFlags & RMV_F_WRTSCREEN) == RMV_F_WRTSCREEN))
               pTraj->ptPosPat += (pTraj->pos - pTraj->prevPos); 
               
            // location of target's update vector in buffer (holds enough for three frames with max # targets allowed)
            j = iRMVFrameSlot * nRMVTgts + pTraj->iUpdatePos;
            
            m_RMVUpdVecs[j].bOn = pTraj->bIsOn ? 1 : 0;       // target's on/off state
            m_RMVUpdVecs[j].hWin += pTraj->ptPosWin.GetH();   // accumulate window & pattern deltas in motion
            m_RMVUpdVecs[j].vWin += pTraj->ptPosWin.GetV();   // update record for tgt for current frame. We
            m_RMVUpdVecs[j].hPat += pTraj->ptPosPat.GetH();   // do this regardless of tgt's on/off state.
            m_RMVUpdVecs[j].vPat += pTraj->ptPosPat.GetV();   // RMVideo handles "off" tgts properly...
            pTraj->ptPosWin.Zero();
            pTraj->ptPosPat.Zero();

            // save current P, V, and A for piecewise integration during the next tick
            pTraj->prevPos = pTraj->pos;
            pTraj->prevVel = pTraj->vel;
            pTraj->prevAcc = pTraj->acc;
            pTraj->prevPatVel = pTraj->patVel;
            pTraj->prevPatAcc = pTraj->patAcc;
         
            // recover nominal velocity vectors in case they were perturbed during the current tick!
            pTraj->vel -= pTraj->pertVelDelta;
            pTraj->patVel -= pTraj->pertPatVelDelta;
         }
         
         // if we've reached the beginning of the next RMVideo update frame, begin working on next frame
         if(nRMVLeadTime >= dRMVNextUpdateMS)
         {
            // we'll always be two frames ahead with RMVideo calculations, so we need to remember the current and next
            // two displayed positions of every RMVideo target, and the current target velocity. Here we set these
            // target positions WRT to a time BEFORE the trial starts. Before the trial starts, current pos = 0, 
            // next pos = target position during frame 0, and the next-next pos = target position during frame 1.
            // Then, during the first scan epoch (t==0), an RMVideo update occurs and: posCurr <-- posNext[0], 
            // posNext[0] <-- posNext[1], and posNext[2] <-- m_traj[].pos.
            for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
            {
               if(iRMVFrameSlot == 0) 
               {
                  m_traj[i].posRMVCurr.Zero();
                  m_traj[i].velRMVCurr.Zero();
                  m_traj[i].posRMVNext[0] = m_traj[i].pos;
               }
               else if(iRMVFrameSlot == 1) m_traj[i].posRMVNext[1] = m_traj[i].pos;
            }

            ++iRMVFrameSlot;
            if(iRMVFrameSlot == 3) iRMVFrameSlot = 0;
            dRMVNextUpdateMS += dRMVFramePerMS;
         }
         ++nRMVLeadTime;
      }
   }
   //
   // END: PREPARE FIRST THREE RMVIDEO FRAMES
   //
   // END: PREPROCESSING PHASE

   // BEGIN:  CONFIGURE HARDWARE AND START TRIAL
   //
   // current and previous (1ms earlier) positions of fixation targets #1 and #2, in visual deg. Initially offscreen.
   CFPoint fix1PosCurr(180, 180);
   CFPoint fix1PosLast(180, 180);
   CFPoint fix2PosCurr(180, 180);
   CFPoint fix2PosLast(180, 180);
   
   // expected chair position in deg; starts at 0 always; calculated by velocity integration
   float fExpectedChairPos = 0.0f;

   // target not selected by a "selByFix*" or "selDurByFix" op, or turned off by a "chooseFix" op
   int nUnselectedTgt = -1;
   
   // "switchFix" only: which fixation target must be fixated by trial's end; which is being fixated currently
   BOOL bSwitchToFix1 = FALSE; 
   BOOL bIsFixing1 = FALSE; 

   // instantaneous pos displacements of fixation tgts at start of special seg -- for "SelectByFix2" feature only.
   CFPoint posDelta1_SBF2; 
   CFPoint posDelta2_SBF2; 

   // "searchTask" only: tgt index currently "fixated" (-1 if none), fixation duration, fixation duration required to
   // satisfy task, final result, search bounds
   int iSearchTgt = -1;
   int iSearchDur = 0;
   int iSearchReqDur = 0;
   DWORD dwSearchRes = 0;
   CFPoint searchBounds;
   if(dwFlags & T_ISSEARCH)
   {
      // if Fix2 target defined and ON, then it's a 2-goal search task. Fix1 must be defined and ON (enforced by UI).
      int iFix2 = m_seg[iSaccSeg].iCurrFix2;
      if((iFix2 > -1) && ((m_seg[iSaccSeg].tgtFlags[iFix2] & TF_TGTON) != 0))
         dwFlags |= T_ST_2GOAL;

      // set the search boundaries to the size of the video display on which the sought-for target is shown.
      // NOTE: In 2-goal version of the task, Fix2 is also involved, but it will always be on the same display.
      double w = 90.0;
      double h = 90.0;
      if(m_traj[m_seg[iSaccSeg].iCurrFix1].wType == CX_RMVTARG)
      {
         w = pRMV->GetScreenW_deg() / 2.0;
         h = pRMV->GetScreenH_deg() / 2.0;
      }
      searchBounds.Set(w, h);
      
      // we reuse the segment grace period to specify the required DURATION that eye must be on a target to satisfy
      // the search task. Here we get the value for that duration.
      iSearchReqDur = m_seg[iSaccSeg].tGrace - m_seg[iSaccSeg].tStart;
   }
   
   // fixation check information: #consecutive ticks fixation is broken, curr pos of 2 eyes in degrees (init: 0.0).
   // NOTE that 2nd eye position (HGPOS2, VEPOS2) is only used when the Eyelink is in use in binocular mode, but ONLY
   // if none of the "special segment operations" -- other than "searchTask" -- are in use. This permits a stereo 
   // fixation check : left eye(HGPOS, VEPOS) against Fix1 and right eye (HGPOS2, VEPOS2) against Fix2.
   int nBrokeFixTicks = 0;
   CFPoint currEyePos;
   CFPoint currEyePos2;
   BOOL enableStereoFixCheck = 
      (m_masterIO.GetEyelinkRecordType() == EL_BINOCULAR) && ((iSaccSeg < 0) || (dwFlags & T_ISSEARCH));

   // prepare to keep track of sliding window average of HGPOS, VEPOS to smooth out effects of eye pos noise on VStab
   CFPoint vStabEyePos;
   CFPoint vStabEyePosLast;
   for(i=0; i<MAXVSTABWINLEN; i++)
   {
      m_hgposSlider[i] = (short) 0;
      m_veposSlider[i] = (short) 0;
   }
   int oldestSliderIdx = 0;
   int sliderLen = m_masterIO.GetVStabSlidingWindow();
   
   // counter for periodic reward delivery mid-trial (enabled on a per-segment basis)
   int nMTRPeriodTicks = 0; 

   // pending marker pulses 
   DWORD dwMarkers = 0; 

   // load definitions of RMVideo targets participating in trial; ABORT on failure (error msg already posted).
   if((nRMVTgts >0) && !LoadRMVideoTargets()) 
   {
      pTimer->WriteChar(ABORT_CHARCODE);
      pTimer->WriteChar(STOP_CHARCODE);
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // zero the last sample from each AI channel that was processed by compression algorithm
   for(i = 0; i < CX_AIO_MAXN+1; i++) m_shLastComp[i] = 0; 

   // reset #events detected so far; reset event mask for current tick: bitN=1 ==> event occurred on DI<N>
   m_nEvents = 0; 
   DWORD dwEventsThisTick = 0; 

   // arm eye-tgt plot update countdown timer
   m_viPlotUpdateMS = EYEANIMATEINTV;

   // if no failsafe segment specified, then data is saved only if trial finishes
   if(iFailsafeTime < 0) iFailsafeTime = nTrialTime; 

   // save trial length; then reset trial tick counter and trial segment counter
   int nTrialLength = nTrialTime; 
   nTrialTime = 0; 
   iCurrSeg = -1; 
   pSeg = NULL;

   // if first save segment unspecified, it is set to 0: the entire trial is recorded and saved
   if(iSaveSeg < 0) iSaveSeg = 0;

   // to avoid "seg >= 0" checks during runtime, set unused segment indices to a number > max #segments
   if(iSaccSeg < 0) iSaccSeg = MAX_SEGMENTS + 2;

   // related to subject's response -- relevant only to staircase sequencing
   dwTrialRes |= CX_FT_RESPOK;                                          // flag is cleared if subj's response incorrect
   dwTrialRes |= CX_FT_NORESP;                                          // flag is cleared once a resp is detected
   const short cshRespThresh = (short) pAI->ToRaw( 2.0f );              // "resp PB pressed" TTL level in b2sAIVolts

   // configure DI event timestamping: 10us clock, DI<15..0> enabled
   pTimer->Configure( 10, 0x0000FFFF );

   // save high-resolution spike trace?
   BOOL bSpikesOn = m_masterIO.IsSpikeTraceOn() && m_masterIO.IsSavingTrialFile();

   // are we using the Eyelink tracker to record eye trajectory? If so, we track blink state so we can record
   // "blink start" and "blink end" events
   BOOL bUsingEL = m_masterIO.IsEyelinkInUse();
   BOOL bWasInBlink = FALSE;
   BOOL bInBlink = FALSE;

   // set up AI device to scan all AI channels, possibly including high-res spike trace
   ConfigureAISeq( bSpikesOn );

   // resume suspend management
   m_suspendMgr.Resume(); 

   // if we'll be saving recorded data to file, open the file now for streaming data as the trial proceeds
   BOOL bStreaming = m_masterIO.IsSavingTrialFile();
   m_masterIO.GetDataFilePath(m_string, CX_MAXPATH);
   if(bStreaming && !OpenStream(m_string))
   {
      m_masterIO.Message("(!!) Unable to open file for streaming recorded data. Trial ABORTED!");
      pTimer->WriteChar(ABORT_CHARCODE);
      pTimer->WriteChar(STOP_CHARCODE);
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // start RMVideo animation if used. Supply motion vectors for first 2 display frames and wait for start of first 
   // frame. Optionally trigger VSync spot flash at start of frame 0. MaestroRTSS always supplies display frame N 
   // while display frame N-2 is being drawn to screen and frame N-1 is being prepared on the backbuffer.
   if((nRMVTgts > 0) && !pRMV->StartAnimation(&(m_RMVUpdVecs[0]), &(m_RMVUpdVecs[nRMVTgts]), m_seg[0].bTrigRMVSyncFlash))
   {
      ::sprintf_s(m_strMsg, "(!!) Failed to start trial on RMVideo: %s.  Trial ABORTED!", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
      pTimer->WriteChar(ABORT_CHARCODE);
      pTimer->WriteChar(STOP_CHARCODE);
      CloseStream(FALSE);
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // REMEMBER: We've already prepared frame 2, which will be sent to RMVideo during the first tick of the trial
   // timeline. RMVideo target trajectory calculations LEAD the trial timeline by 3 RMVideo frame periods. Here are
   // important vars for keeping track:
   //    iRMVFrameSlot : Indicates which frame slot is being prepared in the 3-slot frame buffer. Modulo 3.
   //    nRMVLeadTime : Where we are in the computation of RMVideo target trajectories, in millisecs. This leads
   // the actual trial time by ~3 RMVideo frame periods.
   //    dRMVNextUpdateMS : Indicates when it is time to deliver the next frame to RMVideo (in ms, with us resolution).
   // The next frame is sent when nRMVLeadTime exceeds this value.

   // if we're using the Eyelink tracker, flush the queue and get the tracker sample immediately prior to starting trial.
   // We do this to synch the timelines as best we can. If this fails, don't start trial.
   if(bUsingEL && !SyncWithEyelink())
   {
      if(nRMVTgts > 0) pRMV->StopAnimation();
      ::sprintf_s(m_strMsg, "(!!) Failed to start trial due to Eyelink tracker error. Trial ABORTED!");
      m_masterIO.Message(m_strMsg);
      pTimer->WriteChar(ABORT_CHARCODE);
      pTimer->WriteChar(STOP_CHARCODE);
      CloseStream(FALSE);
      m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
      return(CX_FT_ERROR | CX_FT_DONE);
   }

   // trigger dedicated marker pulse to tell external system that trial has begun
   pTimer->TriggerMarkers(RECORDMARKER_MASK);

   // GO!
   StartAISeq(); 
   double tsTrialStartUS = m_eRunTimeUS.Get();
   //
   // END CONFIGURE HARDWARE AND START TRIAL

   // BEGIN TRIAL RUNTIME LOOP...
   //
   // elapsed time object implements timeout in case AI device freezes; timeout = 2 AI scan intervals, in microsecs
   CElapsedTime eTime;  
   double dTimeout = ((double)m_viScanInterval) * 2000.0; 
   do
   {

      // **WAIT** for start of next "scan epoch"; timeout mechanism prevents deadlock if AI device stops functioning
      eTime.Reset();
      while( (!m_vbInterruptPending) && (eTime.Get() < dTimeout) ) ;
      m_vbInterruptPending = FALSE; 
      
      // abort on an excessively long ISR latency
      if(m_vbDelayedISR) 
      {
         m_masterIO.Message("(!!) AI ISR latency too long. Trial ABORTED!");
         dwTrialRes |= CX_FT_ERROR;
         break;
      }

      // unload next scan's worth from AI device. ABORT on AI error or frame shift (runtime loop one full cycle late).
      if(!UnloadNextAIScan()) 
      { 
         ::sprintf_s(m_strMsg, "(!!) %s at t=%d ticks.  Trial ABORTED!", pAI->GetLastDeviceError(), nTrialTime);
         m_masterIO.Message(m_strMsg);
         dwTrialRes |= CX_FT_ERROR;
         break;
      }
      if(m_vbFrameLag)
      {
         ::sprintf_s(m_strMsg, "(!!) Frameshift at t=%d ticks. Trial ABORTED!", nTrialTime);
         m_masterIO.Message(m_strMsg);
         dwTrialRes |= CX_FT_ERROR;
         break;
      }
      
      // if we're getting eye trajectory data from Eyelink tracker, unload next sample, update blink status. Terminate 
      // trial on error. Report if failure is because Eyelink sample delay exceeded limits.
      if(bUsingEL)
      {
         bWasInBlink = bInBlink;
         if(!UnloadEyelinkSample(&bInBlink, nTrialTime))
         {
            if(m_maxELSampDelay >= CX_MAXELSAMPDELAY)
               ::sprintf_s(m_strMsg, 
                  "(!!) Eyelink sample delay (=%d ms) exceeded limits. Trial ABORTED!", m_maxELSampDelay);
            else
               ::sprintf_s(m_strMsg, "(!!) Eyelink tracker error at t=%d ticks. Trial ABORTED!", nTrialTime);
            m_masterIO.Message(m_strMsg);
            dwTrialRes |= CX_FT_EYELINKERR;
            break;
         }
      }
      

      // current eye positions in degrees. The 2nd eye pos is only used for stereo fixation checking with Eyelink.
      currEyePos.Set(m_pshLastScan[HGPOS], m_pshLastScan[VEPOS]);
      currEyePos *= (1.0/POS_TOAIRAW);
      currEyePos2.Set(m_pshLastScan[HGPOS2], m_pshLastScan[VEPOS2]);
      currEyePos2 *= (1.0/POS_TOAIRAW);

      // update current sliding-window average of eye position, which is used to smooth out eye position noise for
      // better VStab performance. NOTE that we use a circular buffer strategy to store the last N raw samples of
      // HGPOS and VEPOS, where N is the sliding window length in ms (== #ticks). The oldest sample is written over by 
      // the just collected sample, and the index of the oldest sample is incremented with wrap-around. Then we simply 
      // sum over the first N samples of each array, divide by N, and convert to visual deg. Of course, during the 
      // first N ticks of the trial, we divide by the #ticks elapsed instead of N. If the sliding-window length is 1,
      // then this feature is disabled!
      // NOTE that this sliding-window average is not available for the 2nd eye pos.
      if(sliderLen <= 1) 
         vStabEyePos = currEyePos;
      else
      {
         m_hgposSlider[oldestSliderIdx] = m_pshLastScan[HGPOS];
         m_veposSlider[oldestSliderIdx++] = m_pshLastScan[VEPOS];
         if(oldestSliderIdx == sliderLen) oldestSliderIdx = 0;
         
         vStabEyePos.Zero();
         j = (sliderLen < nTrialTime) ? sliderLen : nTrialTime+1;
         for(i=0; i<j; i++) vStabEyePos.Offset(m_hgposSlider[i], m_veposSlider[i]);
         vStabEyePos *= (1.0f/((float)j));
         vStabEyePos *= (1.0/POS_TOAIRAW);
      }

      // BEGIN: IMPLEMENT DELAYED SKIP
      // The "skipOnSaccade" feature is tricky for RMVideo targets, since their update intervals are longer than a 
      // trial "tick". In the case of the now-deprecated XYScope, the skip was delayed so that it occurred on an 
      // XYScope update frame boundary. In the case of RMVideo, we don't have to delay, but we do have to adjust the
      // target update vector currently being prepared so that it reflects the skip in time. There will still be a 
      // delay of 2 frames before the RMVideo targets respond, but that's unavoidable.
      //
      // If there are no RMVideo targets in use, then there's nothing to do except trigger the DO pulse that
      // marks the skip.
      if((dwFlags & T_DELAYSKIP) != 0)
      {
         dwFlags &= ~T_DELAYSKIP;
         dwFlags |= T_SKIPPED;

         // skip forward to start of next seg; if special seg is last one, skip to last trial frame.
         int nOld = nTrialTime;
         nTrialTime = (iCurrSeg+1 < nSegs) ? m_seg[iCurrSeg+1].tStart : (nTrialLength - 1);

         // trigger marker pulse on DO<6> to mark the skip
         dwMarkers |= (DWORD) (1<<6);
         
         // catch up with RMVideo target trajectories. We've leaped forward in time, but we haven't accounted for 
         // RMVideo target motion in the interval [nTrialTime..nRMVLeadTime]. We do that here. All of the motion will
         // be accounted for in the target update vectors currently being prepared, and these won't be sent until it's
         // time for the next scheduled RMVideo update.
         if(nRMVTgts > 0) 
         {
            nRMVLeadTime += (nTrialTime - nOld);
            dRMVNextUpdateMS += (nTrialTime - nOld);

            for(int t = nTrialTime; t<nTrialLength-1 && t<nRMVLeadTime; t++)
            {
               // when we reach the start of a new segment, update trajectory variables for RMVideo targets
               int iTmpCurrSeg = iCurrSeg;
               CTrialSeg* pTmpSeg = pSeg;
               if((iTmpCurrSeg+1 < nSegs) && (m_seg[iTmpCurrSeg+1].tStart == t))
               {
                  ++iTmpCurrSeg;
                  pTmpSeg = &(m_seg[iTmpCurrSeg]);
                  for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
                  {
                     pTraj = &(m_traj[i]);
                  
                     pTraj->bIsOn = (pTmpSeg->tgtFlags[i] & TF_TGTON) != 0;
                     if((pTmpSeg->tgtFlags[i] & TF_TGTREL) != 0) pTraj->pos.Offset(pTmpSeg->tgtPos[i]);
                     else
                     {
                        pTraj->pos = pTmpSeg->tgtPos[i];
                        pTraj->prevVel.Zero();
                     }
                     pTraj->vel = pTmpSeg->tgtVel[i];
                     pTraj->acc = pTmpSeg->tgtAcc[i];
                     pTraj->patVel = pTmpSeg->tgtPatVel[i];
                     pTraj->patAcc = pTmpSeg->tgtPatAcc[i];
                  }
               }
            
               // piecewise integrate RMVideo target trajectories over the previous tick to compute values for current tick
               for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
               {
                  pTraj = &(m_traj[i]);

                  // P(T) = P(T-1) + V(T-1) * dT; V(T) = V(T-1) + A(T-1) * dT
                  pTraj->pos += pTraj->prevVel * dT;
                  pTraj->vel += pTraj->prevAcc * dT;
                  pTraj->patVel += pTraj->prevPatAcc * dT;
            
                  // modulate nominal velocity vectors by any installed perturbations
                  m_pertMgr.Perturb(i, t, pTraj->vel, pTraj->patVel, pTraj->pertVelDelta, pTraj->pertPatVelDelta);
                  pTraj->vel += pTraj->pertVelDelta;
                  pTraj->patVel += pTraj->pertPatVelDelta;

                  // update motion of tgt window and pattern during 1ms tick. For RMV_RANDOMDOTS with RMV_F_WRTSCREEN set,
                  // tgt pattern motion is WRT screen center not target center. In this case, pattern is adjusted to account
                  // for a large displacement of target window at the segment boundary
                  pTraj->ptPosWin += (pTraj->pos - pTraj->prevPos); 
                  pTraj->ptPosPat += pTraj->prevPatVel * dT; 
                  if((pTmpSeg->tStart == t) && (pTraj->iSubType == RMV_RANDOMDOTS) && 
                     ((pTraj->iFlags & RMV_F_WRTSCREEN) == RMV_F_WRTSCREEN))
                        pTraj->ptPosPat += (pTraj->pos - pTraj->prevPos); 
                  
                  // location of target's update vector in buffer (holds enough for three frames with max # targets allowed)
                  j = iRMVFrameSlot * nRMVTgts + pTraj->iUpdatePos;
               
                  m_RMVUpdVecs[j].bOn = pTraj->bIsOn ? 1 : 0;       // target's on/off state
                  m_RMVUpdVecs[j].hWin += pTraj->ptPosWin.GetH();   // accumulate window & pattern deltas in motion
                  m_RMVUpdVecs[j].vWin += pTraj->ptPosWin.GetV();   // update record for tgt for current frame. We
                  m_RMVUpdVecs[j].hPat += pTraj->ptPosPat.GetH();   // do this regardless of tgt's on/off state.
                  m_RMVUpdVecs[j].vPat += pTraj->ptPosPat.GetV();   // RMVideo handles "off" tgts properly...
                  pTraj->ptPosWin.Zero();
                  pTraj->ptPosPat.Zero();

                  // save current P, V, and A for piecewise integration during the next tick
                  pTraj->prevPos = pTraj->pos;
                  pTraj->prevVel = pTraj->vel;
                  pTraj->prevAcc = pTraj->acc;
                  pTraj->prevPatVel = pTraj->patVel;
                  pTraj->prevPatAcc = pTraj->patAcc;
            
                  // recover nominal velocity vectors in case they were perturbed during the current tick!
                  pTraj->vel -= pTraj->pertVelDelta;
                  pTraj->patVel -= pTraj->pertPatVelDelta;
               }
            }
         }
      }
      //
      // END: IMPLEMENT DELAYED SKIP

      // BEGIN: SEGMENT BOUNDARY WORK
      if((iCurrSeg+1 < nSegs) && (m_seg[iCurrSeg+1].tStart == nTrialTime))
      {
         // move to next seg: this updates state variables that can change each segment
         ++iCurrSeg; 
         pSeg = &(m_seg[iCurrSeg]); 

         // if there's a marker pulse for this segment, present it (see below)
         if(pSeg->iPulseOut >= 0) dwMarkers |= (DWORD) (1 << pSeg->iPulseOut);

         // reset associated counter iff mid-trial rewards disabled. Thus we keep reward spacing across seg boundary.
         if(!(pSeg->bRewEna)) nMTRPeriodTicks = 0;

         // start event time-stamping once we start recording data
         if(iCurrSeg == iSaveSeg)  pTimer->Start();

         // start or end of special segment. Enable saccade checking at start. More work to do at segment's end...
         if(iCurrSeg == iSaccSeg && ((dwFlags & (T_ISSWFIX|T_ISCHFIX|T_ISSEARCH))==0))
            dwFlags |= T_CHECKSACC;
         else if(iCurrSeg == iSaccSeg+1)
         { 
            // disabled saccade checking
            dwFlags &= ~T_CHECKSACC; 
            dwFlags &= ~T_INSACCADE;
            
            // "searchTask" trial ALWAYS stops at the end of the special segment.
            if(dwFlags & T_ISSEARCH) 
               break; 
            // for "selByFix*/selDurByFix" ops, select nearest target if no target has been selected yet.
            else if((dwFlags & T_ISFIX) && !(dwFlags & T_SELECTED)) 
            { 
               // present marker pulse on DO<6>; set flag to indicate that selected occurred at segment's end
               dwFlags |= T_SELECTED|T_ENDSEL; 
               dwMarkers |= (DWORD) (1<<6);

               // for each fix tgt, compute min[(E-T)^2, (E-(T-dP))^2], where E = eye pos, T = tgt pos, and dP=
               // displacement in tgt pos at start of special seg. The second measure is the dist-squared to the fix
               // tgt's "pos" had no displacement occurred. This measure applies only to SelByFix2; for SelByFix1, dP
               // is always set to (0,0).
               f1 = min( currEyePos.DistSquared(fix1PosCurr), currEyePos.DistSquared(fix1PosCurr - posDelta1_SBF2) );
               f2 = min( currEyePos.DistSquared(fix2PosCurr), currEyePos.DistSquared(fix2PosCurr - posDelta2_SBF2) );

               // deselect whichever fixation target is further away
               nUnselectedTgt = (f1 <= f2) ? m_seg[iSaccSeg].iCurrFix2 : m_seg[iSaccSeg].iCurrFix1;
            }
            // for "chooseFix" op, trial is aborted if correct target was not chosen by segment's end
            else if((dwFlags & T_ISCHFIX) && !(dwFlags & T_SELECTED)) 
            { 
               dwTrialRes |= CX_FT_LOSTFIX;
               break;
            }
            // for "switchFix" op, we just figure out which fixation target the animal must switch to by trial's end
            else if(dwFlags & T_ISSWFIX)
            {
               bIsFixing1 = BOOL(currEyePos.DistSquared(fix1PosCurr) <= currEyePos.DistSquared(fix2PosCurr));
               bSwitchToFix1 = !bIsFixing1;
            }
            // work to do at the end of the "R/P Distro" special segment:
            else if(dwFlags & T_ISRPDIST) 
            {
               // compute the average behavioral response and report it to MaestroGUI via IPC
               j = m_seg[iCurrSeg].tStart - m_seg[iSaccSeg].tStart; 
               fBehavResp /= float(j); 
               m_masterIO.SetRPDistroBehavResp(fBehavResp);
               dwTrialRes |= CX_FT_GOTRPDRESP;

               // if reward window(s) defined, deliver reward pulse #2 if response is inside a reward window
               if(dwFlags & T_HASRPDWIN)
               { 
                  BOOL bPass = BOOL((fRPDWindow[0]!=fRPDWindow[1]) && (fBehavResp>=fRPDWindow[0]) && 
                        (fBehavResp<=fRPDWindow[1]));
                  if(!bPass) bPass = BOOL((fRPDWindow[2]!=fRPDWindow[3]) && (fBehavResp>=fRPDWindow[2]) && 
                        (fBehavResp<=fRPDWindow[3]));
                  if(bPass)
                  {
                     dwFlags |= T_RPDPASS;
                     if(pTimer->DeliverReward(m_fixRewSettings.iWHVR, nRewPulse2, m_fixRewSettings.iAudioRewLen))
                     {
                        m_masterIO.IncrementNumRewards();
                        m_masterIO.AccumulateRewardPulse(nRewPulse2);
                        if(m_fixRewSettings.bPlayBeep) m_masterIO.Message("beep");
                     }
                  }
               }
            }

            // during a "selDurByFix" trial, if Fix1 was selected during the special segment, then the duration of the
            // next segment will be the minimum duration rather than the max. Since Maestro prepares all subsequent
            // trial codes on the assumption that the max duration was used, we must adjust (1) the overall trial
            // length, and (2) the start times of any segments beyond (iSaccSeg + 1)
            if((dwFlags & T_ISSELDUR) && (nUnselectedTgt == m_seg[iSaccSeg].iCurrFix2))
            {
               int delta = selectSegDurMax - selectSegDurMin;
               nTrialLength -= delta;

               for(int i = iSaccSeg + 2; i < nSegs; i++)
                  m_seg[i].tStart -= delta;
            }
         }
      }
      // END: SEGMENT BOUNDARY WORK

      /** BEGIN: UPDATE TARGET TRAJECTORIES FOR CURRENT TICK, INCLUDING VELOCITY STABILIZATION.
       Using the trajectory info maintained for each trial target, we "piecewise integrate" to obtain the position of
       the target at the start of the current tick, and the velocity that will be in effect for the current tick:
          V(T) = V(T-1) + A(T-1) * dT;
          P(T) = P(T-1) + V(T-1) * dT in the middle of a segment, OR
               = dP + P(T-1) + V(T-1) * DT at the start of a segment, with relative position change dP, OR
               = Pabs at the start of a segment, with absolute position Pabs as delivered by TARGET_*POSABS codes;
       where P = pos, V = vel, A = accel, T = time in AI scan periods, dT = scan period. The calculations are done in
       deg, deg/sec, or deg/sec^2, as appropriate. The special cases for P(T) are handled at segment boundaries -- see
       previous section.
       
       Unlike the animal chair, RMVideo targets are not updated every trial tick. For these, we still piecewise 
       integrate every tick, but accumulate target window and pattern displacements in platform-specific
       structures which get sent only when it's time for a frame update. Furthermore, RMVideo target trajectory 
       computations must LEAD the actual trial timeline by 2 RMVideo frames because we must supply the target update
       vectors for frame N+2 at the beginning of frame N!
       
       Since we're computing target trajectories on the fly, we implement velocity stabilization by adjusting the
       current position of each stabilized target by the change in eye position. If we're at the start of a segment and
       the "snap to eye" flag is set, then the current position is set to the current eye position. Remember, however,
       that the effects of VStab will be delayed by two video frames for RMVideo targets.
      */
      // update all target trajectories...
      for(i=0; i<nTgs; i++)
      {
         pTraj = &(m_traj[i]);

         // RMVideo target trajectory computations LEAD the actual trial timeline by 2 RMVideo frame periods!!
         int t = (pTraj->wType == CX_RMVTARG) ? nRMVLeadTime : nTrialTime;
         
         // update target trajectory variables at segment boundary. We have to be careful for RMVideo targets, since
         // the RMVideo timeline leads the actual trial timeline!
         BOOL bSegStart = (pSeg->tStart == nTrialTime);
         if(pTraj->wType == CX_RMVTARG) bSegStart = (iCurrSeg+1 < nSegs) && (m_seg[iCurrSeg+1].tStart == nRMVLeadTime);
         if(bSegStart)
         {
            CTrialSeg* pSegTmp = (pTraj->wType == CX_RMVTARG) ? &(m_seg[iCurrSeg+1]) : pSeg;
            
            pTraj->bIsOn = (pSegTmp->tgtFlags[i] & TF_TGTON) != 0;
            if((pSegTmp->tgtFlags[i] & TF_TGTREL) != 0) pTraj->pos.Offset(pSegTmp->tgtPos[i]);
            else
            {
               pTraj->pos = pSegTmp->tgtPos[i];
               pTraj->prevVel.Zero();
            }
            pTraj->vel = pSegTmp->tgtVel[i];
            pTraj->acc = pSegTmp->tgtAcc[i];
            pTraj->patVel = pSegTmp->tgtPatVel[i];
            pTraj->patAcc = pSegTmp->tgtPatAcc[i];
         }

         // at start of the "SelByFix2" segment, save instantaeous pos displacements of the designated fixation tgts. We
         // need these to implement this special feature. Remember, for RMVideo targets, we're getting this information 
         // in advance of the actual elapsed timeline!
         if((dwFlags & T_ISFIX2) && bSegStart &&
            ((pTraj->wType == CX_RMVTARG && iCurrSeg+1 == iSaccSeg) || (pTraj->wType != CX_RMVTARG && iCurrSeg==iSaccSeg)))
         {
            if(pTraj->wType == CX_RMVTARG)
            {
               if(i == m_seg[iCurrSeg+1].iCurrFix1) posDelta1_SBF2 = pTraj->pos - pTraj->prevPos;
               else if(i == m_seg[iCurrSeg+1].iCurrFix2) posDelta2_SBF2 = pTraj->pos - pTraj->prevPos;
            }
            else
            {
               if(i == pSeg->iCurrFix1) posDelta1_SBF2 = pTraj->pos - pTraj->prevPos;
               else if(i == pSeg->iCurrFix2) posDelta2_SBF2 = pTraj->pos - pTraj->prevPos;
            }
         }
         
         // P(T) = P(T-1) + V(T-1) * dT; V(T) = V(T-1) + A(T-1) * dT
         pTraj->pos += pTraj->prevVel * dT;
         pTraj->vel += pTraj->prevAcc * dT;
         pTraj->patVel += pTraj->prevPatAcc * dT;
         
         // if target is velocity stabilized, adjust its target position accordingly. 
         // 
         // IMPORTANT: For RMVideo targets, the trajectory calculation is 3 RMVideo frames ahead of the actual trial
         // timeline. We do the same with VStab compensation of RMVideo target trajectories. Of course, that means the
         // compensation is based on eye position samples recorded up to 3 display frames before the first VStab actually
         // takes effect.
         BOOL inNextSeg = ((pTraj->wType == CX_RMVTARG) && (iCurrSeg+1 < nSegs) && 
            (m_seg[iCurrSeg+1].tStart <= nRMVLeadTime));
         BOOL bVStabOn = inNextSeg ? ((m_seg[iCurrSeg+1].tgtFlags[i] & VSTAB_ON) != 0) : 
            ((pSeg->tgtFlags[i] & VSTAB_ON) != 0);
         if(bVStabOn)
         {
            CTrialSeg* pSegTmp = inNextSeg ? &(m_seg[iCurrSeg+1]) : pSeg;
            BOOL bSnap = BOOL((pSegTmp->tgtFlags[i] & VSTAB_SNAP) == VSTAB_SNAP);
            BOOL bVStabH = BOOL((pSegTmp->tgtFlags[i] & VSTAB_H) == VSTAB_H);
            BOOL bVStabV = BOOL((pSegTmp->tgtFlags[i] & VSTAB_V) == VSTAB_V);
            
            // was VStab just turned ON for this target? Remember that for RMVideo targets we'll be looking ahead at
            // the flags for the next segment!
            BOOL bTurningOn = bSegStart && (iCurrSeg==0 || 
               (m_seg[inNextSeg ? iCurrSeg : iCurrSeg-1].tgtFlags[i] & VSTAB_ON) == 0); 

            // snap to current eye position at seg start, if enabled; else, adjust target position by the most recent
            // change in eye position. In the latter case, we may stabilize either component of motion, or both.
            // When "snapping to eye" and a nonzero position change (Xo,Yo) is specified, set target position to 
            // the current eye position offset by (Xo,Yo) -- REGARDLESS if the position change is RELative or ABSolute.
            if(bTurningOn && bSnap)
            {
               pTraj->pos = vStabEyePos;
               pTraj->pos.Offset(pSegTmp->tgtPos[i]);
            }
            else
            {
               fPt1 = vStabEyePos - vStabEyePosLast;
               if(!bVStabH) fPt1.SetH(0);
               if(!bVStabV) fPt1.SetV(0);
               pTraj->pos += fPt1;
            }
         }
         
         // modulate nominal velocity vectors by any installed perturbations. REM: RMVideo target computations LEAD
         // the actual trial timeline!
         m_pertMgr.Perturb(i, t, pTraj->vel, pTraj->patVel, pTraj->pertVelDelta, pTraj->pertPatVelDelta);
         pTraj->vel += pTraj->pertVelDelta;
         pTraj->patVel += pTraj->pertPatVelDelta;

         /** BEGIN:  UPDATE RMVideo TARGET'S MOTION FOR THE CURRENT RMVideo FRAME
          RMVideo targets are updated once per RMVideo frame period. The fixed RMVideo frame period is not an exact 
          integral multiple of the 1ms trial "tick", so we have to keep an accurate measure of RMVideo's timeline as 
          we progress.
          
          The biggest complication is that RMVideo target computations LEAD the actual trial timeline by 2 frame periods
          so that we can supply the target update vectors for frame N+2 at the beginning of frame N! That's taken care
          of above, when we set the elapsed time t=nRMVLeadTime. Once the lead time exeeds the trial length, update
          vectors should still be sent to RMVideo, but all targets are turned OFF since trial will be over!
                    
          Here we accumulate the position displacements (H,V) of an RMVideo target's "window" and/or "pattern" over the
          previous 1msec tick. Once we reach the end of the current RMVideo frame period, we have computed the total 
          displacement over that frame.
          
          RMVideo was introduced in Maestro 2.0.0. RMVideo targets "move" while they are turned off. RMVideo itself 
          keeps track of target motion, so we simply send it the motion vec for each frame regardless of the target's 
          on/off state.
         
          ABOUT UNITS: This code section implicitly assumes that the trial "tick" is 1ms!! Be aware of how the RMVideo 
          target-specific trajectory variables are used here, and their units...
         */
         if(pTraj->wType == CX_RMVTARG)
         {
            // location of target's update vector in buffer (holds enough for three frames with max # targets allowed)
            j = iRMVFrameSlot * nRMVTgts + pTraj->iUpdatePos;
            
            // update motion of tgt window and pattern during 1ms tick. For RMV_RANDOMDOTS with RMV_F_WRTSCREEN set,
            // tgt pattern motion is WRT screen center not target center. In this case, pattern is adjusted to account
            // for a large displacement of target window at the segment boundary
            pTraj->ptPosWin += (pTraj->pos - pTraj->prevPos); 
            pTraj->ptPosPat += pTraj->prevPatVel * dT; 
            if(bSegStart && (pTraj->iSubType == RMV_RANDOMDOTS) && ((pTraj->iFlags & RMV_F_WRTSCREEN) != 0))
               pTraj->ptPosPat += (pTraj->pos - pTraj->prevPos); 
            
            // target on/off state: All targets off if the lead time >= the trial's length!!
            m_RMVUpdVecs[j].bOn = (pTraj->bIsOn && (nRMVLeadTime < nTrialLength)) ? 1 : 0; 

            m_RMVUpdVecs[j].hWin += pTraj->ptPosWin.GetH();   // accumulate window & pattern deltas in motion
            m_RMVUpdVecs[j].vWin += pTraj->ptPosWin.GetV();   // update record for tgt for current frame. We
            m_RMVUpdVecs[j].hPat += pTraj->ptPosPat.GetH();   // do this regardless of tgt's on/off state.
            m_RMVUpdVecs[j].vPat += pTraj->ptPosPat.GetV();   // RMVideo handles "off" tgts properly...
            pTraj->ptPosWin.Zero();
            pTraj->ptPosPat.Zero();
         }
         //
         // END:  UPDATE RMVideo TARGET'S MOTION FOR THE CURRENT RMVideo FRAME
         
         // save current P, V, and A for piecewise integration during the next tick
         pTraj->prevPos = pTraj->pos;
         pTraj->prevVel = pTraj->vel;
         pTraj->prevAcc = pTraj->acc;
         pTraj->prevPatVel = pTraj->patVel;
         pTraj->prevPatAcc = pTraj->patAcc;
         
         // recover nominal velocity vectors in case they were perturbed during the current tick!
         pTraj->vel -= pTraj->pertVelDelta;
         pTraj->patVel -= pTraj->pertPatVelDelta;
         
         // for search task, we want to ignore targets that are turned OFF during the search segment. Note that we do
         // NOT use CTrialTraj for the target's ON state, since this will be looking ahead for RMVideo targets!
         if((dwFlags & T_ISSEARCH) && (iCurrSeg == iSaccSeg) && (pSeg->tStart == nTrialTime))
            pTraj->bIsOnForSearch = (pSeg->tgtFlags[i] & TF_TGTON) != 0;
      }
      //
      // END: UPDATE TARGET TRAJECTORIES FOR THE CURRENT TRIAL TICK

      // during R/P Distro sepcial seg, calculate behavioral response sample for this tick and accumulate
      if((iCurrSeg == iSaccSeg) && (dwFlags & T_ISRPDIST))
      {
         // eye velocity (H,V): convert ADC code to deg/sec
         fPt1.Set(m_pshLastScan[HEVEL], m_pshLastScan[VEVEL]);
         fPt1 *= (1.0/VEL_TOAIRAW); 
         if(iBehavRespType == TH_RPD_EYEVEL) fBehavResp += fPt1.Distance();   // vector eye speed
         else if(iBehavRespType == TH_RPD_HEVEL) fBehavResp += fPt1.GetH();   // H eye velocity
         else if(iBehavRespType == TH_RPD_VEVEL) fBehavResp += fPt1.GetV();   // V eye velocity
         else fBehavResp += fPt1.GetTheta();                                  // vector eye direction
      }

      // BEGIN: HANDLE RECORDED DATA...
      // If recording has started, we need to unload any digital events that have occurred recently and form a bit mask
      // of all events unloaded. This mask is sent to the GUI data trace display. In addition, if the data is being
      // saved, we stream new analog and event data to the file, aborting if an error occurs. If the Eyelink tracker is
      // used, then check for a blink start or blink end event and stream that as well...
      dwEventsThisTick = 0; 
      if(iCurrSeg >= iSaveSeg) 
      {
         m_nEvents = pTimer->UnloadEvents(201, &(m_events[0]), &(m_evtTimes[0]));
         for(j = 0; j < m_nEvents; j++) dwEventsThisTick |= m_events[j]; 

         if(bStreaming)
         {
            bStreaming = StreamAnalogData() && StreamEventData();
            if(bStreaming && bUsingEL && (bWasInBlink != bInBlink))
               bStreaming = StreamEyelinkBlinkEvent(bInBlink, nTrialTime - m_seg[iSaveSeg].tStart);

            if(!bStreaming)
            {
               m_masterIO.Message("(!!) File IO error!  Trial aborted -- data file discarded");
               dwTrialRes |= CX_FT_ERROR;
               break;
            }
         }
      } 
      // END: HANDLE RECORDED DATA

      // BEGIN: UPDATE TARGET MOTION/STATE ON HARDWARE
      // if target was selected in a "selByFix", "selDurByFix" or "chooseFix" op, turn OFF "unselected" target
      // for remainder of trial.
      if(dwFlags & T_SELECTED) 
      {
         pTraj = &(m_traj[nUnselectedTgt]);
         if(pTraj->wType == CX_RMVTARG) m_RMVUpdVecs[iRMVFrameSlot*nRMVTgts + pTraj->iUpdatePos].bOn = 0;
      }

      // at start of each RMVideo frame N, send target motion vectors for frame N+2. ABORT if update fails. Also abort
      // if more than the allowed number of duplicate frames occurs on the RMVideo display. Maestro allows 0 or up to 3
      // duplicate frames. NOTE: Updates are sent for frames after trial ends, but all targets are turned OFF.
      if((nRMVTgts > 0) && (nRMVLeadTime >= dRMVNextUpdateMS))
      { 
         // are we sending update for the first frame following start of the next trial segment? If so, check to see if
         // VSync spot flash should be presented. Remember that we're sending these updates in advance!
         BOOL bRMVFlashOnNextUpdate = FALSE;
         if((iCurrSeg + 1 < nSegs) && (m_seg[iCurrSeg + 1].tStart <= nRMVLeadTime))
         {
            bRMVFlashOnNextUpdate = m_seg[iCurrSeg + 1].bTrigRMVSyncFlash;
            m_seg[iCurrSeg + 1].bTrigRMVSyncFlash = FALSE;  // so we don't send it again for that segment!
         }

         // NOTE: We suspend the file writer thread during this update, because we have to send data over RT-TCPIP.
         // Testing indicated that, on rare occasions, the socket send() call in CRMVideo::sendRMVCommand() blocked
         // until the file writer thread yielded the CPU (assuming all RTX threads/processes sharing the same CPU.
         if(bStreaming) m_writer.Pause();
         j = iRMVFrameSlot * nRMVTgts;
         int nFramesElapsed = -1;
         BOOL bOk = pRMV->UpdateAnimation(&(m_RMVUpdVecs[j]), bRMVFlashOnNextUpdate, nFramesElapsed);
         if(!bOk)
         {
            ::sprintf_s(m_strMsg, "(!!) RMVideo error (%s) at t=%d ticks. Trial ABORTED!",
               pRMV->GetLastDeviceError(), nTrialTime);
            m_masterIO.Message(m_strMsg);
            dwTrialRes |= CX_FT_ERROR;
         }
         else
         {
            int nDupes = pRMV->GetNumDuplicateFrames();
            bOk = (nDupes <= nRMVDupesAllowed);
            if(!bOk)
            {
               m_masterIO.GetProtocolName(m_string, CXH_NAME_SZ);
               if(nRMVDupesAllowed == 0)
                  ::sprintf_s(m_strMsg, "(!!) Duplicate RMVideo frame detected (t=%d ms). Trial '%s' ABORTED!", 
                     nTrialTime, m_string);
               else
                  ::sprintf_s(m_strMsg, "(!!) Got %d duplicate RMVideo frames (t=%d ms). Trial '%s' ABORTED!", 
                     nDupes, nTrialTime, m_string);
               m_masterIO.Message(m_strMsg);
               dwTrialRes |= CX_FT_RMVDUPE;
            }
         }
         if(bStreaming) m_writer.Resume();

         // abort trial immediately on RMVideo error
         if(!bOk) break;       

         // CXDRIVER should remain 2-3 video frames ahead of RMVideo. Once the lead exceeds 3 frames, deliver a
         // warning message each time the lead grows by another full frame.
         if(nFramesElapsed > 0)
         {
            double diff = dRMVNextUpdateMS - nFramesElapsed * dRMVFramePerMS;
            if(diff > nRMVFramesAhead * dRMVFramePerMS)
            {
               ::sprintf_s(m_strMsg, "WARNING: Maestro leads RMVideo by %d+ video frames: diff = %.2f ms", nRMVFramesAhead, diff);
               m_masterIO.Message(m_strMsg);
               ++nRMVFramesAhead;
            }
         }

         // elapsed time for next RMVideo update, frame slot; reset target motion vectors for next frame
         dRMVNextUpdateMS += dRMVFramePerMS;
         ++iRMVFrameSlot;
         if(iRMVFrameSlot == 3) iRMVFrameSlot = 0;
         ::memset(&(m_RMVUpdVecs[iRMVFrameSlot*nRMVTgts]), 0, nRMVTgts*sizeof(RMVTGTVEC));

         // update the current and next positions, and the current velocity for all RMVideo targets animated
         for(i=0; i<nTgs; i++) if(m_traj[i].wType == CX_RMVTARG)
         {
            // by convention, curr vel = (currPos - nextPos) / framePeriodInSec
            m_traj[i].velRMVCurr = m_traj[i].posRMVNext[0] - m_traj[i].posRMVCurr;
            m_traj[i].velRMVCurr *= ((float) (1000.0/dRMVFramePerMS));

            m_traj[i].posRMVCurr = m_traj[i].posRMVNext[0];
            m_traj[i].posRMVNext[0] = m_traj[i].posRMVNext[1];
            m_traj[i].posRMVNext[1] = m_traj[i].pos;
         }
      }

      // update chair velocity IF chair is present (even if it is not used, we need to compensate for drift).
      if(m_masterIO.IsChairPresent()) 
      {
         // current chair position (== subject's head position!) in degrees; commanded chair vel in deg/sec
         float fChairPos = ((float) m_pshLastScan[HHPOS])/POS_TOAIRAW; 
         float fChairVel = (idxChair < 0) ? 0.0f : m_traj[idxChair].vel.GetH();
         
         // compute expected chair pos by velocity integration (note use of "-=").
         if((idxChair >= 0) && (nTrialTime > 0)) fExpectedChairPos -= m_traj[idxChair].prevVel.GetH() * dT.GetH();

         pAO->UpdateChair(fChairVel, fChairPos, fExpectedChairPos);
      }
      // END: UPDATE TARGET MOTION/STATE ON HARDWARE

      // BEGIN: CHECK FIXATION REQUIREMENTS...

      // remember positions of fixation targets #1, #2 during previous tick (so we can compute velocity later), then
      // determine their positions for the current tick. If a fixation target is not defined for the current epoch,
      // then its "position" is way offscreen!
      // NOTE that we do this work AFTER target positions have been updated on the hardware for this tick. For an
      // RMVideo target acting as a fixation target, this means that the target position will be the actual current
      // displayed position, or the position at which it is now being drawn, if a frame update began during this
      // scan epoch.
      fix1PosLast = fix1PosCurr;
      fix1PosCurr.Set(180,180);
      fix2PosLast = fix2PosCurr;
      fix2PosCurr.Set(180,180);

      for(i=0; i<nTgs; i++)
      {
         pTraj = &(m_traj[i]);

         // if this tgt is "fix tgt #1" (or #2), then update the current pos of "fix tgt #1" (#2) accordingly. For
         // RMVideo targets, we use the actual displayed current position, since our trajectory calculations LEAD the
         // trial timeline...
         if(i == pSeg->iCurrFix1) fix1PosCurr = (pTraj->wType==CX_RMVTARG) ? pTraj->posRMVCurr : pTraj->pos;
         if(i == pSeg->iCurrFix2) fix2PosCurr = (pTraj->wType==CX_RMVTARG) ? pTraj->posRMVCurr : pTraj->pos;
      }

      // "searchTask" op: Standard fixation checking disabled throughout the special segment. Instead, we look for
      // eye to stay on a target for N contiguous ticks, where N is the grace period for the special segment....
      if((iCurrSeg == iSaccSeg) && (dwFlags & T_ISSEARCH))
      {
         // if eye was "fixating" a tgt during previous tick..
         if(iSearchTgt > -1)
         {
            // if it is still close enough to the same tgt, increment ticks elapsed. If not, reset. If eye has
            // fixated the same tgt -- either a goal tgt or a distractor -- for the required duration, STOP!
            pTraj = &(m_traj[iSearchTgt]);
            fPt1 = (pTraj->wType == CX_RMVTARG) ? pTraj->posRMVCurr : pTraj->pos;
            if(currEyePos.IsNear(fPt1, pSeg->fpFixAcc))
            {
               ++iSearchDur;
               if(iSearchDur >= iSearchReqDur)
               {
                  if((iSearchTgt == pSeg->iCurrFix1) || (iSearchTgt == pSeg->iCurrFix2))
                     dwSearchRes = CXHF_ST_OK;
                  else
                     dwSearchRes = CXHF_ST_DISTRACTED;
                  break;
               }
            }
            else
            {
               iSearchTgt = -1;
               iSearchDur = 0;
            }
         }
         
         // if eye is not still "on" a target, scan all targets to see if one is close enough. If more than one is
         // close enough, choose the "closest". Do NOT check targets that are turned OFF. If eye has wandered outside 
         // the search boundaries, STOP immediately.
         if(iSearchTgt == -1)
         {
            if(currEyePos.IsOutside(searchBounds))
               break;

            float minDistSq = 40000.0f;
            for(i=0; i<nTgs; i++) 
            {
               pTraj = &(m_traj[i]);
               fPt1 = (pTraj->wType == CX_RMVTARG) ? pTraj->posRMVCurr : pTraj->pos;
               if(pTraj->bIsOnForSearch && currEyePos.IsNear(fPt1, pSeg->fpFixAcc))
               {
                  float dSq = currEyePos.DistSquared(fPt1);
                  if(dSq < minDistSq)
                  {
                     iSearchTgt = i;
                     minDistSq = dSq;
                  }
               }
            }
         }
         
         // subject is considered to have "tried" the search task so long as its eye velocity reaches the specified
         // saccadic threshold velocity at some point during the task.
         if((dwFlags & T_SOUGHT) == 0)
         {
            if((cMath::abs(m_pshLastScan[HEVEL]) > iSaccThresh) || (cMath::abs(m_pshLastScan[VEVEL]) > iSaccThresh))
               dwFlags |= T_SOUGHT;
         }
      }
      
      // if we're in a "chooseFix" segment and the correct target has not yet been selected, see if the eye is now
      // close enough. If so, deliver reward pulse 2, deliver marker on DOUT6, and turn off the other target.
      else if((dwFlags & T_ISCHFIX) && (iCurrSeg == iSaccSeg) && !(dwFlags & T_SELECTED))
      {
         fPt1 = (dwFlags & T_ISCHFIX1) ? fix1PosCurr : fix2PosCurr; 
         if(fPt1.IsNear(currEyePos, pSeg->fpFixAcc)) 
         {
            dwFlags |= T_SELECTED;
            nUnselectedTgt = (dwFlags & T_ISCHFIX1) ? pSeg->iCurrFix2 : pSeg->iCurrFix1;
            dwMarkers |= (DWORD) (1<<6);
         
            if(pTimer->DeliverReward(m_fixRewSettings.iWHVR, nRewPulse2, m_fixRewSettings.iAudioRewLen)) 
            { 
               m_masterIO.IncrementNumRewards(); 
               m_masterIO.AccumulateRewardPulse(nRewPulse2); 
               if(m_fixRewSettings.bPlayBeep) m_masterIO.Message("beep");
            }
         }
      }
      
      // if grace period not yet exceeded, or if no fix tgt #1 designated, or we're in the "selByFix*/selDurByFix" 
      // segment, then fixation checking is disabled
      else if((nTrialTime < pSeg->tGrace) || (pSeg->iCurrFix1 < 0) || ((dwFlags & T_ISFIX) && (iCurrSeg == iSaccSeg)))
         nBrokeFixTicks = 0;
      
      // during "switchFix" op, fixation checking is suspended in the middle of a saccade. Else, fixation is OK so long
      // as eye position is within fixation window of EITHER target. ABORT on "lost fix" if this is not the case for 
      // two consecutive ticks.
      else if((iCurrSeg>=iSaccSeg) && (dwFlags & T_ISSWFIX))
      {
         BOOL bSacc = (cMath::abs(m_pshLastScan[HEVEL])>iSaccThresh) || (cMath::abs(m_pshLastScan[VEVEL])>iSaccThresh);
         if(bSacc)
            nBrokeFixTicks = 0;
         else 
         { 
            if(fix1PosCurr.IsNear(currEyePos, pSeg->fpFixAcc) || fix2PosCurr.IsNear(currEyePos, pSeg->fpFixAcc))
               nBrokeFixTicks = 0;
            else
            {
               ++nBrokeFixTicks;
               if(nBrokeFixTicks > 1)
               {
                  dwTrialRes |= CX_FT_LOSTFIX;
                  break;
               }
            }

            // keep track of which target is closer to animal's current eye position
            bIsFixing1 = BOOL(currEyePos.DistSquared(fix1PosCurr) <= currEyePos.DistSquared(fix2PosCurr));
         }
      }
      
      // if binocular fixation checking is possble and Fix2 is defined in the current segment, then check Fix1 against
      // the left eye (HGPOS, VEPOS) and Fix2 against the right (HGPOS2, VEPOS2). We assume that Fix2 is turned ON.
      else if(enableStereoFixCheck && (pSeg->iCurrFix2 > -1))
      {
         if(fix1PosCurr.IsFar(currEyePos, pSeg->fpFixAcc) || fix2PosCurr.IsFar(currEyePos2, pSeg->fpFixAcc))
         {
            ++nBrokeFixTicks;
            if(nBrokeFixTicks > 1)
            {
               dwTrialRes |= CX_FT_LOSTFIX;
               break;
            }
         }
         else nBrokeFixTicks = 0;
      }

      // IF none of the other cases above apply, then do the normal fixation checking. We check eye position against
      // fix tgt #1, UNLESS fix#1 was "unselected" in a "selByFix" or "chooseFix" op. Fixation is broken and trial is
      // aborted if eye is outside of fixation window of the fixation target for two consecutive ticks.
      else 
      {
         if((nUnselectedTgt<0) || (nUnselectedTgt==pSeg->iCurrFix2)) fPt1 = fix1PosCurr;
         else fPt1 = fix2PosCurr;

         if(fPt1.IsFar(currEyePos, pSeg->fpFixAcc))
         {
            ++nBrokeFixTicks;
            if(nBrokeFixTicks > 1)
            {
               dwTrialRes |= CX_FT_LOSTFIX;
               break;
            }
         }
         else nBrokeFixTicks = 0;
      }
      // END: CHECK FIXATION REQUIREMENTS

      // BEGIN: MID-TRIAL REWARDS...
      if(pSeg->bRewEna && !(dwTrialRes & CX_FT_LOSTFIX))
      {
         // two modes, periodic or at segment's end. In periodic mode, reward is delivered at regular intervals during
         // enabled segment(s). Else, one reward is delivered at the end of the enabled segment (but NOT the last one).
         BOOL doReward = FALSE;
         if(nMTRIntv > 0) 
         { 
            ++nMTRPeriodTicks; 
            if(nMTRPeriodTicks >= nMTRIntv)
            {
               doReward = TRUE;
               nMTRPeriodTicks = 0;
            }
         }
         else doReward = (iCurrSeg+1 < nSegs) && (m_seg[iCurrSeg+1].tStart == nTrialTime+1);

         // if it's time to do so, deliver reward using pulse length defined by MIDTRIALREW code, possibly subject to
         // random withholding and accompanied by an "audio" reward. If delivered, update MaestroGUI's reward stats via
         // IPC and optionally cue user with a "beep" on the system's speaker.
         if(doReward)
         {
            if(pTimer->DeliverReward(m_fixRewSettings.iWHVR, nMTRLen, m_fixRewSettings.iAudioRewLen))
            {
               m_masterIO.IncrementNumRewards();
               m_masterIO.AccumulateRewardPulse(nMTRLen);
               if(m_fixRewSettings.bPlayBeep) m_masterIO.Message("beep");
            }
         }
      }
      // END: MID-TRIAL REWARDS

      // update displayed data for MaestroGUI: (1) position plot of eye, chair, and fixation targets; (2) "computed"
      // channels reflecting vel & pos of fixation targets #1 and #2 (V=P=0 if no fix tgt designated); (3) recorded
      // analog and digital event data...
      UpdateLoci(fix1PosCurr, fix2PosCurr, CFPoint(180));

      short shComputed[CX_CP_NCHANS];
      for(i = 0; i < CX_CP_NCHANS; i++) shComputed[i] = 0; 
      if(pSeg->iCurrFix1 > -1)
      {
         shComputed[CX_CP_HPFIX1] = short(fix1PosCurr.GetH() * POS_TOAIRAW);
         shComputed[CX_CP_VPFIX1] = short(fix1PosCurr.GetV() * POS_TOAIRAW);
         if(nTrialTime > 0)
         {
            pTraj = &(m_traj[pSeg->iCurrFix1]);
            if(pTraj->wType == CX_RMVTARG)
            {
               // when RMVideo target is a fixation target, fix1PosCurr is set to the position in the current displayed
               // frame, which is updated once per display frame, NOT every MS. So we also keep track of the target
               // velocity, which again is updated once per display frame.
               fPt1 = pTraj->velRMVCurr;
               fPt1 *= VEL_TOAIRAW;
            }
            else
            {
               fPt1 = fix1PosCurr - fix1PosLast;
               fPt1 *= (VEL_TOAIRAW / dT.GetH());
            }
            shComputed[CX_CP_HVFIX1] = short(fPt1.GetH());
            shComputed[CX_CP_VVFIX1] = short(fPt1.GetV());
         }
      }
      if(pSeg->iCurrFix2 > -1 && nTrialTime > 0)
      {
         pTraj = &(m_traj[pSeg->iCurrFix2]);
         if(pTraj->wType == CX_RMVTARG)
         {
            // see comments above for Fix1.
            fPt1 = pTraj->velRMVCurr;
            fPt1 *= VEL_TOAIRAW;
         }
         else
         {
            fPt1 = fix2PosCurr - fix2PosLast;
            fPt1 *= (VEL_TOAIRAW / dT.GetH());
         }
         shComputed[CX_CP_HVFIX2] = short(fPt1.GetH());
         shComputed[CX_CP_VVFIX2] = short(fPt1.GetV());
      }

      m_masterIO.UpdateTrace(m_pshLastScan, &(shComputed[0]), dwEventsThisTick);
      m_masterIO.UpdateEventStream(dwEventsThisTick, nTrialTime);

      // BEGIN: HANDLE SACCADE-TRIGGERED SPECIAL SEGMENT FEATURES
      if(dwFlags & T_CHECKSACC)
      {
         // we are "in a saccade" if H or V eye velocity is above specified threshold
         BOOL bSacc = (cMath::abs(m_pshLastScan[HEVEL])>iSaccThresh) || (cMath::abs(m_pshLastScan[VEVEL])>iSaccThresh);

         // "skipOnSaccade": Upon detecting saccade, set flag to perform skip at start of next tick. Also, disable any 
         // further saccade-checking.
         if(dwFlags & T_ISSKIP)
         {
            if(bSacc) 
            {
               dwFlags &= ~T_CHECKSACC;
               dwFlags |= T_DELAYSKIP;
            }
         }
         
         // "selByFix*" or "selDurByFix": Select target at "end" of saccade.
         else if(dwFlags & T_ISFIX) 
         {
            // detect start of saccade
            if(!(dwFlags & T_INSACCADE)) 
            {
               if( bSacc ) dwFlags |= T_INSACCADE;
            }
            // at saccade's end: select nearest fixation target IF it is within fixation bounds of current eye pos
            else if(!bSacc ) 
            {
               dwFlags &= ~T_INSACCADE;
               fPt1 = fix1PosCurr;
               fPt2 = fix2PosCurr;

               // NOTE: In SelByFix2, we define a fix tgt's "ghost" pos as the pos where the tgt would be had no 
               // displacement occurred at the start of the special segment. Eye pos is compared to both the actual tgt
               // pos and the ghost pos. In SelByFix1 and SelDurByFix, the pos displacements posDelta*_SBF2 are always 
               // (0,0) -- so the calculations below are correct for all SelByFix variants. The fixation accuracy is 
               // enforced as a rect window around the current eye pos. Thus, one tgt can be out of bounds yet still 
               // closer to the eye than the other tgt (think: ellipse inside a rectangle). We are careful here to 
               // avoid this mistake.
               //
               BOOL bFix1Ok = currEyePos.IsNear( fPt1, pSeg->fpFixAcc ) ||
                              currEyePos.IsNear( fPt1-posDelta1_SBF2,  pSeg->fpFixAcc );
               BOOL bFix2Ok = currEyePos.IsNear( fPt2, pSeg->fpFixAcc ) ||
                              currEyePos.IsNear( fPt2-posDelta2_SBF2,  pSeg->fpFixAcc );

               if(!bFix1Ok) f1 = 10000.0f;
               else f1 = min(currEyePos.DistSquared(fPt1), currEyePos.DistSquared(fPt1 - posDelta1_SBF2));

               if(!bFix2Ok) f2 = 10000.0f;
               else f2 = min(currEyePos.DistSquared(fPt2), currEyePos.DistSquared(fPt2 - posDelta2_SBF2));

               // IF eye is "close enough" to one of the fixation targets or its "ghost": disable saccade checking,
               // present marker pulse on DO<6>, and deselect the target that's further away.
               if(bFix1Ok || bFix2Ok) 
               {
                  dwFlags &= ~T_CHECKSACC;
                  dwFlags |= T_SELECTED; 
                  dwMarkers |= (DWORD) (1<<6); 

                  if(f1 <= f2) nUnselectedTgt = pSeg->iCurrFix2;
                  else nUnselectedTgt = pSeg->iCurrFix1;
               }
            }
         }
      } 
      // END: HANDLE SACCADE-TRIGGERED SPECIAL SEGMENT FEATURES

      // trigger any marker pulses for this tick
      if(dwMarkers != 0)
      {
         pTimer->TriggerMarkers(dwMarkers);
         dwMarkers = 0;
      } 

      // if applicable, check subject's response to trial if it's still "correct" to this point. Response takes the
      // form of pressing a "correct" or "incorrect" pushbutton. The PBs are monitored by simple circuitry that feeds
      // into two AI channels. Pressing a button closes the circuit and raises the voltage above 2V. Result flags are 
      // set to indicate that a response was detected, whether or not the response is correct, and whether the 
      // "correct" pushbutton was depressed. Once an "incorrect" response is registered, further checking is disabled.
      if(pSeg->bCheckResp && (dwTrialRes & CX_FT_RESPOK))
      {
         if(m_pshLastScan[pSeg->iChOk] > cshRespThresh) 
         {
            dwFlags |= T_HITOKPB;
            dwTrialRes &= ~CX_FT_NORESP;
         }
         if(m_pshLastScan[pSeg->iChWrong] > cshRespThresh)
         {
            dwTrialRes &= ~CX_FT_RESPOK;
            dwTrialRes &= ~CX_FT_NORESP;
         }
      }

      // check for a command from MaestroGUI. When a trial is running, the only commands recognized are those that
      // switch operational mode or abort the trial. All other commands are simply ignored.
      DWORD dwCmd = m_masterIO.GetCommand(); 
      if(dwCmd == CX_SWITCHMODE || dwCmd == CX_TR_ABORT) 
      {
         if( dwCmd == CX_SWITCHMODE )
         {
            int iOpMode;
            m_masterIO.GetCommandData(&iOpMode, NULL, 1, 0);
            m_masterIO.SetMode(iOpMode);
         }
         m_masterIO.AckCommand( dwCmd, NULL, NULL, 0, 0);
         m_masterIO.Message("(!!) Trial aborted by user!");
         dwTrialRes |= CX_FT_ABORTED;
         break;
      }
      else m_masterIO.AckCommand(CX_UNRECOGCMD, NULL, NULL, 0, 0);

      // advance to next trial "tick". Remember the previous eye position for use in VStab implementation.
      ++nTrialTime; 
      ++nRMVLeadTime;
      vStabEyePosLast = vStabEyePos; 
   } while(nTrialTime < nTrialLength);
   //
   // END:  TRIAL RUNTIME LOOP

   // stop AI sequence and reset AI function (disabling interrupts!)
   pAI->Init();

   // deliver second marker pulse on dedicated DO line to tell external system that the trial is over.
   pTimer->TriggerMarkers(RECORDMARKER_MASK);

   // stop DIO event timestamping
   pTimer->Stop(); 

   // different error conditions that result in premature trial termination (broken fixation is not an error)
   DWORD dwErrResFlags = CX_FT_ABORTED | CX_FT_ERROR | CX_FT_RMVDUPE | CX_FT_EYELINKERR;

   // special handling of the "lost fix" flag...
   if((dwTrialRes & (CX_FT_LOSTFIX|dwErrResFlags)) == 0)
   {
      // for "switchFix" trials only, NO reward if subject did not switch from tgt fixated at end of special segment 
      // to the other target by trial's end.
      if((dwFlags & T_ISSWFIX) && (bSwitchToFix1 != bIsFixing1)) dwTrialRes |= CX_FT_LOSTFIX;
      
      // for "searchTask" trials only, NO reward if subject did not successfully "find" any target, OR if subject found
      // a "distractor" in the 2-goal (Fix1 and Fix2 in use) version of the task
      if((dwFlags & T_ISSEARCH) != 0)
      {
         if((dwSearchRes == 0) || ((dwFlags & T_ST_2GOAL) && (dwSearchRes == CXHF_ST_DISTRACTED)))
            dwTrialRes |= CX_FT_LOSTFIX;
      }
   }

   // reward animal if fixation was not broken AND no runtime error occurred AND the trial was not aborted.
   // (NOTE: This logic was changed on 04sep2013 for V3.0.3. Previously, a reward was delivered so long as
   // fixation requirements were met.)
   BOOL bRewardGiven = FALSE; 
   if(!(dwTrialRes & (CX_FT_LOSTFIX|dwErrResFlags)))
   { 
      // in various select-by-fix scenarios, reward pulse depends on which target was selected
      int iAdjRewDur = nRewPulse1;
      if(((dwFlags & T_ISFIX) != 0) && (nUnselectedTgt == m_seg[iSaccSeg].iCurrFix1)) iAdjRewDur = nRewPulse2;
      
      // for R/P Distro, if animal failed the "test", reward pulse 2 is delivered instead of reward pulse 1
      if((dwFlags & T_ISRPDIST) && (dwFlags & T_HASRPDWIN) && !(dwFlags & T_RPDPASS))  iAdjRewDur = nRewPulse2;

      // for 2-goal "searchTask" op, #1 is delivered if Fix1 target was "found", #2 if Fix2 was found. For 1-goal
      // searchTask, #1 is delivered if goal target (Fix1) was found, #2 if distractor was found.
      if(dwFlags & T_ST_2GOAL) iAdjRewDur = (iSearchTgt == m_seg[iSaccSeg].iCurrFix1) ? nRewPulse1 : nRewPulse2;
      else if(dwFlags & T_ISSEARCH) iAdjRewDur = (dwSearchRes == CXHF_ST_OK) ? nRewPulse1 : nRewPulse2;
      
      // deliver reward, possibly subject to random withholding and accompanied by "audio" reward. If reward was indeed
      // delivered, update MaestroGUI reward stats in IPC and cue user with a speaker beep.
      // NOTE: Two forms of random withholding here: the global WHVR in m_fixRewSettings, which is implemented in the
      // DeliverReward() call, and the per-trial WHVR, which is implemented by setting the reward pulse length to 0.
      if(pTimer->DeliverReward(m_fixRewSettings.iWHVR, iAdjRewDur, m_fixRewSettings.iAudioRewLen))
      {
         bRewardGiven = TRUE;
         m_masterIO.IncrementNumRewards();
         m_masterIO.AccumulateRewardPulse(iAdjRewDur);
         if(m_fixRewSettings.bPlayBeep) m_masterIO.Message("beep");
      }
   }

   // for the 1-goal target version of the search task, the trial is considered completed if the animal "tried", even
   // if no reward is delivered. In the 2-goal version, no reward is delivered if a distractor is selected, and the
   // trial is complete even if the animal "did not try" (unlikely given how these tasks are defined). In either case,
   // we need to clear the "lost fix" flag that was set above to deny the reward.
   if((dwFlags & T_ISSEARCH) && (dwTrialRes & CX_FT_LOSTFIX))
   {
      if((dwFlags & T_SOUGHT) || ((dwFlags & T_ST_2GOAL) && (dwSearchRes == CXHF_ST_DISTRACTED)))
         dwTrialRes &= ~CX_FT_LOSTFIX;
   }

   // char codes tell external system if animal broke fixation or if trial was aborted by user or stopped
   // prematurely on a runtime error
   if(dwTrialRes & CX_FT_LOSTFIX) pTimer->WriteChar(LOSTFIX_CHARCODE);
   if(dwTrialRes & dwErrResFlags) pTimer->WriteChar(ABORT_CHARCODE);

   // stop/reinit: chair; RMVideo animation
   pAO->InitChair(); 
   if(nRMVTgts > 0)
   {
      // if trial aborted, we sleep here so that MaestroGUI received TR_ABORT ack in time, because the StopAnimation()
      // call may block for a while!
      if(dwTrialRes & CX_FT_ABORTED) Sleep(10); 
      if(!pRMV->StopAnimation())
      {
         ::sprintf_s(m_strMsg, "(!!) Unable to communicate with RMVideo to stop animation sequence: %s", 
               pRMV->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }
   }

   // if we're saving recorded data, unload any remaining DI events from the DIO event timer. Reset event timer.
   dwEventsThisTick = 0; 
   if(iCurrSeg >= iSaveSeg)
   {
      m_nEvents = pTimer->UnloadEvents(201, &(m_events[0]), &(m_evtTimes[0]));
      for(j = 0; j < m_nEvents; j++) dwEventsThisTick |= m_events[j]; 

      if(bStreaming && m_nEvents > 0)
      {
         if(!StreamEventData())
         {
            m_masterIO.Message("(!!) File IO error!  Trial aborted -- data file discarded");
            dwTrialRes |= CX_FT_ERROR;
            bStreaming = FALSE;
         }
      }
   }
   pTimer->Init(); 

   // final update of MaestroGUI's data trace display (via IPC)
   m_masterIO.UpdateTrace(m_pshLastScan, NULL, dwEventsThisTick);

   // for successful "selByFix*/selDurByFix" trials, post message indicating which reward pulse was delivered
   if((dwFlags & T_ISFIX) && !(dwTrialRes & (CX_FT_LOSTFIX|dwErrResFlags)))
   {
      if(nUnselectedTgt == pSeg->iCurrFix2) { i = 1; j = nRewPulse1; } 
      else { i = 2; j = nRewPulse2; }
      if(dwFlags & T_ISSELDUR)
         ::sprintf_s(m_strMsg, "SelDurByFix: Fix Tgt #%d selected, rew len = %d ms.", i, j);
      else
         ::sprintf_s(m_strMsg, "SelByFix*: Fix Tgt #%d selected, rew len = %d ms.", i, j);
      m_masterIO.Message(m_strMsg);
   }

   // store elapsed trial time in IPC memory
   m_masterIO.SetLastTrialLen(nTrialTime);

   // save trial data IF: (1) we're supposed to save trial's data to file and streaming was successful so far; (2) trial 
   // was NOT aborted and no error occurred; (3) some data was recorded; and (4) elapsed trial time exceeded the 
   // failsafe time.
   if(bStreaming && ((dwTrialRes & dwErrResFlags) == 0) && (iCurrSeg >= iSaveSeg) 
         && (nTrialTime >= iFailsafeTime))
   {
      // BEGIN: Fill in header record. NOTE that the streaming functions have already taken care of nScansSaved,
      // nBytesCompressed, and nSpikeBytesCompressed! So DON'T clear the header at this point!

      // name of trial, trial set, trial subset (will be empty string if not applicable) and list of AI channels recorded
      m_masterIO.GetProtocolName(m_Header.name, CXH_NAME_SZ); 
      m_masterIO.GetTrialSetName(m_Header.setName, CXH_NAME_SZ);
      m_masterIO.GetTrialSubsetName(m_Header.subsetName, CXH_NAME_SZ);
      m_Header.nchans = (short) m_nSavedCh; 
      for(i = 0; i < m_nSavedCh; i++) m_Header.chlist[i] = (short) m_iChannels[i];

      // RMVideo display parameters, if applicable
      if( dwFlags & T_USERMV )
      {
         m_Header.d_rows = (short) pRMV->GetScreenH_pix();
         m_Header.d_cols = (short) pRMV->GetScreenW_pix();
         int d, w, h;
         pRMV->GetGeometry(d, w, h);
         m_Header.d_dist = (short) d;
         m_Header.d_dwidth = (short) w;
         m_Header.d_dheight = (short) h;
         
         // convert frame period in seconds --> frame rate in micro-Hz
         dDummy = pRMV->GetFramePeriod(); 
         m_Header.d_framerate = (int) (1.0e6 / dDummy);

         // settings for the "vertical sync" spot flash feature
         m_Header.rmvSyncSz = (short) pRMV->GetSyncFlashSpotSize();
         m_Header.rmvSyncDur = (short) pRMV->GetSyncFlashDuration();

         // save information on duplicate frame events. This happens only if 1-3 duplicate frames occurred during the
         // trial, AND user elected to tolerate up to 3 such duplicate frames.
         int n = pRMV->GetNumDuplicateFrameEvents();
         if(n > 0)
         {
            m_Header.flags |= CXHF_DUPFRAME;
            for(int i = 0; i < n && i < CXH_RMVDUPEVTSZ / 2; i++)
            {
               int frameIdx = -1, count = -1;
               if(pRMV->GetDuplicateFrameEventInfo(i, frameIdx, count))
               {
                  m_Header.rmvDupEvents[i*2] = frameIdx;
                  m_Header.rmvDupEvents[i * 2 + 1] = count;
               }
            }
         }
      }

      // target position and velocity transformation factors
      m_Header.iPosScale = int(1000.0 * m_masterIO.GetPosScale()); 
      m_Header.iPosTheta = int(1000.0 * m_masterIO.GetPosRotate());
      m_Header.iVelScale  = int(1000.0 * m_masterIO.GetVelScale());
      m_Header.iVelTheta = int(1000.0 * m_masterIO.GetVelRotate());
      m_Header.iStartPosH = int(1000.0 * m_masterIO.GetStartPosH());
      m_Header.iStartPosV = int(1000.0 * m_masterIO.GetStartPosV());
      
      // trial bit flags, reward pulse lengths in ms, datestamp, timestamp in ms, current file version#
      m_Header.dwTrialFlags = m_masterIO.GetTrialFlags(); 
      m_Header.iRewLen1 = nRewPulse1; 
      m_Header.iRewLen2 = nRewPulse2;
      m_Header.dayRecorded = m_masterIO.GetDayOfMonth(); 
      m_Header.monthRecorded = m_masterIO.GetMonthOfYear();
      m_Header.yearRecorded = m_masterIO.GetYear();
      m_Header.timestampMS = (int) (tsTrialStartUS / 1000.0);
      m_Header.version = CXH_CURRENTVERSION; 

      // trial result
      if(!(dwTrialRes & CX_FT_LOSTFIX))
      {
         m_Header.flags |= CXHF_REWARDEARNED;
         if(bRewardGiven) m_Header.flags |= CXHF_REWARDGIVEN;
      }

      // result of selByFix*/selDurByFix variants
      if(dwFlags & T_ISFIX)
      {
         if(nUnselectedTgt == m_seg[iSaccSeg].iCurrFix2) m_Header.flags |= CXHF_FIX1SELECTED;
         else m_Header.flags |= CXHF_FIX2SELECTED;
         if(dwFlags & T_ENDSEL) m_Header.flags |= CXHF_ENDSELECT;
      }

      // for "switchFix" op, store identity of target that was "selected" at end of special segment
      if((dwFlags & T_ISSWFIX) && iCurrSeg > iSaccSeg) 
         m_Header.flags |= (bSwitchToFix1 ? CXHF_FIX2SELECTED : CXHF_FIX1SELECTED);

      // for "searchTask" op, set appropriate flags and selected target index
      if(dwFlags & T_ISSEARCH) 
      {
         m_Header.flags |= (CXHF_ISSEARCHTSK | dwSearchRes);
         if(dwFlags & T_ST_2GOAL) m_Header.flags |= CXHF_ST_2GOAL;
         if(dwSearchRes == CXHF_ST_OK)
            m_Header.flags |= ((iSearchTgt == m_seg[iSaccSeg].iCurrFix1) ? CXHF_FIX1SELECTED : CXHF_FIX2SELECTED);
         m_Header.iSTSelected = (dwSearchRes == 0) ? -1 : iSearchTgt;
      }
      
      // the length of sliding window average used to smooth eye position trace for VStab.
      m_Header.iVStabWinLen = sliderLen;
      
      // save pertinent information for R/P Distro op, if applicable
      if(dwFlags & T_ISRPDIST) 
      {
         m_Header.flags |= CXHF_ISRPDISTRO;
         m_Header.iRPDStart = m_seg[iSaccSeg].tStart;
         i = (iSaccSeg+1 < nSegs) ? m_seg[iSaccSeg+1].tStart : nTrialLength;
         m_Header.iRPDDur = i - m_Header.iRPDStart;
         if(dwTrialRes & CX_FT_GOTRPDRESP)
         {
            m_Header.flags |= CXHF_GOTRPDRESP;
            m_Header.iRPDResponse = int(1000.0f * fBehavResp);
            m_Header.iRPDRespType = iBehavRespType;
         }
         for(i=0; i<4; i++) m_Header.iRPDWindows[i] = int(1000.0f *fRPDWindow[i]);
      }
      
      // if spike trace data ("fast data") included in file, save pertinent information. NOTE that streaming function
      // updated nSpikeBytesCompressed throughout the trial, so don't touch it!
      if(bSpikesOn)
      {
         m_Header.flags |= CXHF_SAVEDSPIKES;
         m_Header.nSpikeSampIntvUS = SPIKESAMPINTVUS;
      }

      // if Eyelink tracker was used, save cal params and inter-sample interval stats
      if(bUsingEL && (m_nELSamples > 0))
      {
         m_Header.flags |= CXHF_EYELINKUSED;
         m_Header.iELInfo[0] = m_masterIO.GetEyelinkRecordType();
         m_Header.iELInfo[1] = m_masterIO.GetEyelinkOffset(TRUE);
         m_Header.iELInfo[2] = m_masterIO.GetEyelinkOffset(FALSE);
         m_Header.iELInfo[3] = m_masterIO.GetEyelinkGain(TRUE);
         m_Header.iELInfo[4] = m_masterIO.GetEyelinkOffset(FALSE);
         m_Header.iELInfo[5] = m_masterIO.GetEyelinkVelocityWindowWidth();
         m_Header.iELInfo[6] = m_nELRepeats;
         m_Header.iELInfo[7] = m_maxELSampDelay;
         m_Header.iELInfo[8] = (int) (1000.0 * ((double) m_accumELSampDelay) / ((double) m_nELSamples));
      }

      // if trial contains tagged sections, set relevant flag in header
      if(m_masterIO.GetNumTaggedSections() > 0) m_Header.flags |= CXHF_HASTAGSECTS;

      m_Header.nScanIntvUS = TRIALSCANINTVUS;      // AI "slow" scan interval in microsecs
      // DON'T TOUCH THESE 4 FIELDS. They're handled by the streaming functions
      // m_Header.nBytesCompressed  
      // m_Header.nScansSaved
      // m_Header.nchar
      // m_Header.npdig
      //
      // END: PREPARE HEADER RECORD
      
      // close the stream and save the file. This will flush any records that haven't been written, and write the
      // header record as it was prepared above. If successful, we inform the external system that file was saved.
      if(CloseStream(TRUE)) 
      {
         dwTrialRes |= CX_FT_DATASAVED;
         pTimer->WriteChar(DATASAVED_CHARCODE);
      }
      else
      {
         m_masterIO.Message("(!!) File IO error occurred at trial's end. Data file discarded");
         dwTrialRes |= CX_FT_ERROR; 
      }
   }
   else
   {
      // if we don't save data, we might still have started streaming. Make sure stream is closed and file discarded.
      CloseStream(FALSE);
   }

   // send "stop" char marking end of char sequence for this trial: for integration with external system
   pTimer->WriteChar(STOP_CHARCODE); 

   // if chair was used during trial, restore it to the rest (zero) position
   if(m_masterIO.IsChairPresent() && (dwFlags & T_USECHAIR)) RestoreChair();

   // done! Restore suspend manager to prior state.
   dwTrialRes |= CX_FT_DONE; 
   m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
   return(dwTrialRes);
}


/**
 Runtime controller for "Continuous Mode" operation.

 DESIGN NOTES:
    1) In the old cntrlxUNIX/PC, we did not wait for an AI "start-scan" interrupt at the beginning of each iteration of
 the runtime loop. This was because of the complicated scheme for queueing tasks such as compressing analog data, 
 queueing data packets for network transfer, and so on. With faster machines, this should no longer be required. In 
 particular, the time required to compress one scan's worth of analog data (16 "slow" samples + 40 "fast" samples) is 
 on the order of a few us or less, and network transfers are no longer a consideration for Maestro. Thus, we now 
 operate on the assumption that we can usually complete all the "per-scan" tasks within the 2ms scan period, and so we 
 wait for the "start-scan" interrupt at the beginning of each loop iteration. However, we've built in the ability to 
 read in 2 scans' worth of data at a time in order to catch up when necessary -- see the method UnloadNextAIScan().
    2) The "active target" list. "Active targets" are a limited set of targets that are displayed statically when
 fixation checking is ON. The user can arbitrarily reposition and turn on/off any active target, set the (constant) 
 direction and speed of target pattern motion (for targets that have an independent tgt pattern!), or designate it as 
 fixation tgt #1, #2 (or both!), or as the special "cursor tracking" target. Static targets may be used during eye-coil
 calibration prior to the start of an experiment, while the tracking target is typically used to probe the response 
 properties of a neural unit that is being monitored. Any target type (XYscope, RMVideo) except the chair can be 
 included in the active target list. The tgt designation may be changed at any time via the CX_CM_UPDFIXTGTS command.
 The parameters (on/off state, window position, pattern speed & dir) of a single active target may be changed at any 
 time during ContMode via the CX_CM_UPDACVTGT command. However, the COMPOSITION of the active list can be altered only 
 when the system is not "active":  no stimulus running, fixation OFF, and recording OFF. There are two reasons for this 
 restriction. If an RMVideo target is part of the new active list, it must be loaded onto the video hardware -- which 
 could take a while for certain RMVideo targets, such as a grating windowed by a circular Gaussian. The delay would 
 certainly disrupt an ongoing stimulus run, fixation checking, or data recording. Secondly, XYScope targets can also 
 participate in "XYseq" stimulus runs. All XYScope targets in the XYseq and the active list must be loaded in hardware 
 prior to turning fixation ON or starting a stimulus run -- which are two independent events. The only way to ensure 
 this is to prevent changes in the active list (or the stimulus run, for that matter) unless fixation is OFF and no 
 stimulus is running.

      11dec2006: Support for nonzero pattern motion was introduced in Maestro v2.0.4, mainly so the "tracking" target 
      could include a moving pattern in order to better stimulate the neural unit. The pattern motion is likely not to 
      be as accurate as in Trial mode; in particular, RMVideo frame drops are ignored.

    3) In the old cntrlxUNIX/PC, cntrlxPC implemented a "state machine" for a special recording mode known as
 "XYseq-as-trials". In this mode, data recorded during each cycle of an XYseq XSTIM (ie, stimulus run) was saved to a 
 separate data file -- essentially like running in TrialMode. CntrlxPC also automatically turned off data recording 
 when a stimulus run "auto-stopped". All of this functionality is now handled by MaestroGUI rather than MaestroRTSS.
    4) In the old cntrlxUNIX/PC, cntrlxUNIX precomputed the trajectories for Targ1, OKN, and Chair stimuli and sent
 the trajectories to the PC side. Here we update all stimulus trajectories on the fly!! ALSO, as of Maestro version 
 1.5.0, the OKN target is no longer supported!
    5) XYScope targets in ContMode. XYScope tgts may participate in an "XYseq" stimulus run, or they may be used as
 "active" targets, which are visible only when fixation is turned ON (see note 1). We allow fixation to be turned 
 on/off during an XYseq, and we allow individual XY active targets (which do NOT participate in the XYseq itself) to be
 turned on/off & moved during an XYseq. To handle all of these possibilities properly, we must recalculate the 
 CXYUpdateRecord field 'reps' every update, and after the update we must clear the 'hv' & 'vv' fields for all XYScope 
 targets in the active list (not participating in XYseq) that are currently on.
    6) RMVideo targets in ContMode. RMVideo replaced the old VSG framebuffer in Maestro v2.0. RMVideo "animates" a set
 of loaded "targets" on a frame-by-frame basis via calls to CCxRMVideo::UpdateAnimation(). In ContMode, RMVideo targets 
 can appear in the "active target" list and, as such, can be turned on/off or moved IAW user-initiated commands from 
 MaestroGUI. This, of course, is NOT frame-by-frame animation. RMVideo will generate "duplicate frame" errors b/c we do 
 not call UpdateAnimation() on every frame. We simply choose to ignore those errors. RMVideo will continue the 
 "animation" despite such errors.

 INTEGRATING DATA RECORDING WITH AN EXTERNAL SYSTEM.  MaestroRTSS uses the DIO event timer's "character writer" and
 "marker pulse" functions to deliver information useful for external synchronization and offline integration of a
 recorded Maestro continuous-mode data file with data collected by a separate data acquisition system, such as the 
 Plexon Multi-Acquisition Processor:
    -- Before recording begins, the START_CHARCODE character is written to the character writer device, followed by the
 null-terminated data file name. Immediately after starting the event timer's digital timestamping function, a marker 
 pulse is issued on timer DO<11>, which is dedicated exclusively for this purpose.
    -- When recording ends (normally or otherwise), a second marker pulse is triggered on DO<11>.
    -- If the recording aborts prematurely for whatever reason, the ABORT_CHARCODE character is written.
    -- If the data file is successfully saved, the DATASAVED_CHARCODE is written.
    -- Finally, after the recording ends and the data file is saved (successfully or not), the STOP_CHARCODE is written.
 Thus, START_CHARCODE and STOP_CHARCODE always bracket the sequence of characters sent during a single recording in 
 Continuous Mode.

 20mar2019-- Mods IAW changes in CCxRMVideo. If there are any active RMVideo targets, RMVideo will remain in animate
 mode for an indefinite period of time. Due to a bug in RMVideo, the refresh period was slightly underestimated and,
 as a result, Maestro would send frame updates faster than RMVideo could consume them -- leading to a backlog of 
 updates in the RMVideo network receive buffer that grew the longer RMVideo stayed in the animate state. This bug
 became apparent after a few minutes of continous operation, as it would take a while for a target change initiated on
 the Maestro side to occur on the RMVideo display. Eventually, the delay got so long that, when trying to return to
 Idle mode, CXDRIVER would be waiting for RMVideo to respond to the stop-animate command (meanwhile, RMVideo is simply
 processing the backlog of frame updates!), Maestro timed out on the "switch mode" command and terminated CXDRIVER for
 failing to respond.
    While RMVideo's estimate of refresh period is now more accurate, I still added code to adjust -- when necessary --
 how often frame updates are sent in Continuous mode so that CXDRIVER stays about 2-4 frames ahead of the RMVideo 
 animation timeline. 
 25sep2024-- Removing all XYScope-related code for Maestro 5.x. The XYScope platform and related device hardware have
 not been supported since Maestro 4.0. At this point, Continuous mode is really only used for initial calibration of
 subject eye position at the start of an experiment, AFAIK.
 18nov2024-- Removed support for the PSGM stimulus channel. The PSGM module was never actually put into use (a prototype
 was tested, then abandoned). With this change, the only available stimlulus channel type uses the animal chair, which
 may not be available in any active rigs. We also decided to drop support for writing stimulus run definitions to the
 Cont-mode data file. Analysis software (readcxdata, JMWork) has always skipped over stimulus run records, anyway.
*/
VOID RTFCNDCL CCxDriver::RunContinuousMode()
{
   int i, j; 
   BOOL bOk;
   CFPoint fPt1;
   CFPoint fPt2;
   CFPoint fPt3;
   CFPoint fPtTrack;

   // BEGIN: INITIALIZATIONS...
   m_masterIO.Message("Entering continuous mode...");

   // set suspend duty cycle: 2ms, 20% suspended; save old suspend params for restoring upon mode switch
   int iOldOn, iOldOff; 
   m_suspendMgr.ChangeTiming(1600, 400, &iOldOn, &iOldOff);

   // current operational state: no stimulus running, no recording in progress, not fixating
   DWORD dwOpState = 0; 
   m_vbStimOn = FALSE;
   BOOL bRecordOn = FALSE;
   m_bFixOn = FALSE;
   m_masterIO.ClearResult(); 

   // scan interval = "tick" is 2ms in Cont Mode
   m_viScanInterval = 2; 
   const float dT = 0.001f * float(m_viScanInterval); 
   
   // zero countdown timer for eye-target pos plot updates so we update the plot immediately
   m_viPlotUpdateMS = 0; 
   // bit mask indicating what DI events occurred during the current "tick"
   DWORD dwEventsThisTick = 0;  
   // #duty cycles elapsed since start of stim run
   int nRunCycles = 0;

   // all active targets OFF, located at origin, with zero pattern velocity
   for(i = 0; i < MAX_ACTIVETGTS; i++) 
   { 
      ::memset( &(m_acvTgts[i].tgtDef), 0, sizeof(CXTARGET) );
      m_acvTgts[i].posCurr.Zero();
      m_acvTgts[i].posNext.Zero();
      m_acvTgts[i].fPatSpeed = 0.0f;
      m_acvTgts[i].fPatDir = 0.0f;
      m_acvTgts[i].bOn = FALSE;
   }

   // contiguous time periods (in ms) that fixation was outside or within prescribed bounds
   int iBrokeFixDur = 0;
   int iHeldFixDur = 0; 

   // current position of eye and animal chair in degrees; prev pos of fixation targets; all zero'd initially
   CFPoint currEyePos; 
   float fCurrChairPos = 0.0f;
   CFPoint lastFix1Pos;
   CFPoint lastFix2Pos;

   // pointers to some of our device objects
   CCxEventTimer* pTimer = m_DevMgr.GetTimer();
   CCxAnalogIn* pAI = m_DevMgr.GetAI();
   CCxAnalogOut* pAO = m_DevMgr.GetAO();
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();

   // some RMVideo-specific state...
   BOOL bUsingRMV = FALSE;                                   // flag set whenever RMVideo tgt is in active list
   CElapsedTime eRMVTime;                                    // elapsed time in us since RMVideo animation started
   double dRMVNextUpdateMS = 0;                              // elapsed time in ms when we should send the next tgt upd
   int nRMVFramesSent;                                       // number of frame updates sent to RMVideo since start

   // RMVideo monitor frame period in ms. If we get more than 4 frames ahead of RMVideo or fall less than 2 frames ahead,
   // then we adjust the monitor frame period on the fly so that we send frame updates less or more frequently.
   double dRMVFramePerMS = pRMV->GetFramePeriod() * 1000.0; 

   // elapsed timer used to enforce a minimum interval between consecutive marker pulse presentations
   CElapsedTime etLastMarker; 

   // is Eyelink tracker recording in progress? We check this regularly because the user can connect/disconnect
   // from tracker while in Cont mode. When connected, the Eyelink will always be recording in Cont Mode.
   BOOL bUsingEL = FALSE;

   // these variables used during a data recording to detect and timestamp "blink start" and "blink end" events in 
   // Eyelink data stream, and to check inter-sample delay 
   BOOL bInBlink = FALSE;
   BOOL bWasInBlink = FALSE;
   int nRecTimeMS = 0;

   //
   // END: INITIALIZATIONS

   // elapsed timer used to implement a timeout in case AI device freezes; timeout period = 2 scan intervals (in us)
   CElapsedTime eTime; 
   double dTimeout = ((double)m_viScanInterval) * 2000.0; 

   // configure event timer (10us clk, DI15..0 enabled); reset "fixation status" signal; start prototypical AI sequence
   pTimer->Configure(10, 0x0000FFFF); 
   pTimer->ClearFixationStatus();
   UnloadEyelinkSample(&bInBlink, -1);
   ConfigureAISeq();
   StartAISeq();

   // BEGIN: CONTINUOUS MODE RUNTIME LOOP...
   int iOpMode = CX_CONTMODE; 
   while(iOpMode == CX_CONTMODE)
   {
      // wait for start of next "scan epoch"; timeout mechanism prevents deadlock if AI device stops working
      eTime.Reset();
      while((!m_vbInterruptPending) && (eTime.Get() < dTimeout)) ;
      m_vbInterruptPending = FALSE;

      // unload next 1 or 2 scans of analog data. If an AI error occurred: (1) report it to MaestroGUI via IPC; 
      // (2) return to the inactive state. If recording was in progress, recorded data is discarded, and record stop
      // marker pulse and "abort" and "stop" char codes are issued for synching with an external system. (3) Restart
      // the background AI sequence to resume Continuous-mode operation in the inactive state.
      if(!UnloadNextAIScan())
      {
         ::sprintf_s(m_strMsg,  "(!!) AI device error (%s)", pAI->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
         
         if(bRecordOn || m_vbStimOn || m_bFixOn)
         {
            m_vbStimOn = FALSE;
            m_bFixOn = FALSE;
            if(bRecordOn)
            {
               m_masterIO.Message("(!!) Recording aborted -- data file discarded.");
               CloseStream(FALSE);

               while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
               pTimer->TriggerMarkers(RECORDMARKER_MASK);
               etLastMarker.Reset();
               pTimer->WriteChar(ABORT_CHARCODE);
               pTimer->WriteChar(STOP_CHARCODE);

               pTimer->Init();
               bRecordOn = FALSE;
            }
         }

         dwOpState = 0; 
         m_masterIO.SetResult(dwOpState);

         UnloadEyelinkSample(&bInBlink, -1);
         ConfigureAISeq();
         StartAISeq();
      }

      // if Eyelink tracker in use, unload latest tracker sample and use it for HGPOS/VEPOS. Note that user can
      // connect and disconnect from tracker while in Cont mode (except when data recording is in progress), so 
      // we have to check regularly whether or not the tracker is in use.
      BOOL bWasUsingEL = bUsingEL;
      bUsingEL = m_masterIO.IsEyelinkInUse();
      if(bWasUsingEL || bUsingEL)
      {
         bOk = TRUE;
         if((bWasUsingEL != bUsingEL) && bRecordOn)
         {
            bOk = FALSE;
            ::sprintf_s(m_strMsg, "(!!) Eyelink tracker %s while data recording in progress!", 
               bWasUsingEL ? "disconnected" : "connected");
            m_masterIO.Message(m_strMsg);
         }

         if(bOk && bUsingEL)
         {
            bWasInBlink = bInBlink;
            bOk = UnloadEyelinkSample(&bInBlink, bRecordOn ? nRecTimeMS : -1);
            if(!bOk)
            {
               if(m_maxELSampDelay >= CX_MAXELSAMPDELAY)
                  ::sprintf_s(m_strMsg, "(!!) Eyelink sample delay (=%d ms) exceeded limits.", m_maxELSampDelay);
               else
                  ::sprintf_s(m_strMsg, "(!!) Eyelink tracker error occurred!");
               m_masterIO.Message(m_strMsg);
            }
         }

         if(!bOk)
         {
            if(bRecordOn || m_vbStimOn || m_bFixOn)
            {
               m_vbStimOn = FALSE;
               m_bFixOn = FALSE;
               if(bRecordOn)
               {
                  m_masterIO.Message("(!!) Data recording aborted and data file discarded");
                  CloseStream(FALSE);

                  while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
                  pTimer->TriggerMarkers(RECORDMARKER_MASK);
                  etLastMarker.Reset();
                  pTimer->WriteChar(ABORT_CHARCODE);
                  pTimer->WriteChar(STOP_CHARCODE);

                  pTimer->Init();
                  bRecordOn = FALSE;
               }
            }

            dwOpState = 0; 
            m_masterIO.SetResult(dwOpState);

            UnloadEyelinkSample(&bInBlink, -1);
            ConfigureAISeq();
            StartAISeq();
         }
      }

      // get current eye position and rotational chair position; convert to degrees subtended at eye
      currEyePos.Set(m_pshLastScan[HGPOS], m_pshLastScan[VEPOS]);
      currEyePos *= (1.0/POS_TOAIRAW);
      fCurrChairPos = ((float) m_pshLastScan[HHPOS])/POS_TOAIRAW;

      // if a long ISR latency occurs AND we're not in the inactive state: (1) report error to MaestroGUI; (2) return 
      // to the inactive state. If recording was in progress, recorded data is discarded, and record stop marker pulse 
      // and "abort" and "stop" char codes are issued for synching with an external system. (3) Restart the background 
      // AI sequence to resume Continuous-mode operation in the inactive state.
      if(m_vbDelayedISR && (bRecordOn || m_vbStimOn || m_bFixOn))
      {
         m_masterIO.Message("(!!) AI ISR latency too long!");

         m_vbStimOn = FALSE;
         m_bFixOn = FALSE;
         if(bRecordOn)
         {
            m_masterIO.Message("(!!) Recording aborted -- data file discarded.");
            CloseStream(FALSE);

            while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
            pTimer->TriggerMarkers(RECORDMARKER_MASK);
            etLastMarker.Reset();
            pTimer->WriteChar(ABORT_CHARCODE);
            pTimer->WriteChar(STOP_CHARCODE);

            pTimer->Init();
            bRecordOn = FALSE;
         }

         dwOpState = 0; 
         m_masterIO.SetResult(dwOpState);

         UnloadEyelinkSample(&bInBlink, -1);
         ConfigureAISeq(); 
         StartAISeq();
      }

      // BEGIN: WHILE RECORDING...
      dwEventsThisTick = 0; 
      if(bRecordOn)
      {
         // unload any digital event occurring recently; form bit mask of all events unloaded -- for data trace display
         m_nEvents = pTimer->UnloadEvents(201, &(m_events[0]), &(m_evtTimes[0]));
         for(j = 0; j < m_nEvents; j++) dwEventsThisTick |= m_events[j]; 

         // stream new analog and event data to file, plus a detected blink start or blink end event from the Eyelink.
         // If an error occurs, return to the inactive state in the usual manner (abort recording, discard data file, 
         // report error, synch with external system, resume bkg DAQ).
         BOOL bOk = StreamAnalogData() && StreamEventData(); 
         if(bOk && bUsingEL && (bWasInBlink != bInBlink)) bOk = StreamEyelinkBlinkEvent(bInBlink, nRecTimeMS);
         nRecTimeMS += m_viScanInterval;  // do this after call to stream fcn so we get blink event timestamp right
         if(!bOk)
         {
            m_vbStimOn = FALSE;
            m_bFixOn = FALSE;
            m_masterIO.Message("(!!) File IO error!  Recording aborted -- data file discarded");
            CloseStream(FALSE);
            while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
            pTimer->TriggerMarkers(RECORDMARKER_MASK);
            etLastMarker.Reset();
            pTimer->WriteChar(ABORT_CHARCODE);
            pTimer->WriteChar(STOP_CHARCODE);
            pTimer->Init();
            bRecordOn = FALSE;
            dwOpState = 0;
            m_masterIO.SetResult(dwOpState);

            UnloadEyelinkSample(&bInBlink, -1);
            ConfigureAISeq();
            StartAISeq();
         }
      }
      // END: WHILE RECORDING...

      // BEGIN: WHILE STIMULUS RUN IS IN PROGRESS...
      if(m_vbStimOn)
      {
         // get current time within run's "duty cycle". At the end of a duty cycle: increment #cycles done and reset 
         // duty cycle time to indicate the start of a new cycle. If it's time to stop (stop requested or autostop), 
         // end the stimulus run and notify MaestroGUI of the state change.
         int tCurrent = m_viStimTicks * m_viScanInterval; 
         if(tCurrent < m_run.tLastUpdate) 
         {
            ++m_run.iCycles; 
            m_run.tLastUpdate = -1; 

            if(m_run.bSoftStopReq || (m_run.def.nAutoStop > 0 && m_run.iCycles >= m_run.def.nAutoStop))
            {
               m_vbStimOn = FALSE;
               dwOpState &= ~(CX_FC_RUNON|CX_FC_RUNSTOPPING);
               m_masterIO.SetResult(dwOpState); 
               continue;
            }
         }

         // update current state of stimulus run: marker pulses, calc trajectories, etc.
         UpdateStimulusRun(tCurrent); 
         m_run.tLastUpdate = tCurrent; 
      }
      // END: WHILE STIMULUS RUN IN PROGRESS...

      // BEGIN: RMVIDEO FRAME UPDATE
      double dRMVTimeNowMS = eRMVTime.Get() / 1000.0;
      if(bUsingRMV && (dRMVTimeNowMS >= dRMVNextUpdateMS))
      { 
         // NOTE: Pattern motion is NOT intended to be accurate for RMVideo active targets. We ASSUME here that 
         // exactly one video frame has elapsed since the last update. This will not always be the case!
         int nRMVTgts = 0;
         for(i=0; i<m_masterIO.GetNumTargets(); i++ ) if(m_acvTgts[i].tgtDef.wType == CX_RMVTARG)
         {
            memset(&(m_RMVUpdVecs[nRMVTgts]), 0, sizeof(RMVTGTVEC));
            if(m_bFixOn && m_acvTgts[i].bOn)
            {
               m_RMVUpdVecs[nRMVTgts].bOn = 1;
               fPt1 = m_acvTgts[i].posNext - m_acvTgts[i].posCurr;
               m_acvTgts[i].posCurr = m_acvTgts[i].posNext;
               m_RMVUpdVecs[nRMVTgts].hWin = fPt1.GetH();
               m_RMVUpdVecs[nRMVTgts].vWin = fPt1.GetV();

               // update pattern velocity for tgts that support it. For flowfield and grating w/no orientation adj, 
               // the pattern dir is ignored; pattern speed = flow or drift velocity. For grating w/ orientation
               // adj, for plaid without indep gratings, and for random dot patch, pattern speed and direction
               // are converted to pattern H,V displacements. Plaid targets with "independent gratings" are not 
               // supported -- for such targets the pattern will NOT move.
               int rmvType = m_acvTgts[i].tgtDef.u.rmv.iType;
               BOOL isOriAdj = BOOL((m_acvTgts[i].tgtDef.u.rmv.iFlags & RMV_F_ORIENTADJ) == RMV_F_ORIENTADJ);
               BOOL isWrtScrn = BOOL((m_acvTgts[i].tgtDef.u.rmv.iFlags & RMV_F_WRTSCREEN) == RMV_F_WRTSCREEN);
               BOOL isIndepGrats = BOOL((m_acvTgts[i].tgtDef.u.rmv.iFlags & RMV_F_INDEPGRATS) == RMV_F_INDEPGRATS);

               float fPatDelta = m_acvTgts[i].fPatSpeed * 0.001f * float(dRMVFramePerMS);

               if(rmvType==RMV_FLOWFIELD || (rmvType==RMV_GRATING && !isOriAdj))
                  m_RMVUpdVecs[nRMVTgts].hPat = fPatDelta;
               else if((rmvType == RMV_RANDOMDOTS) || (rmvType==RMV_GRATING && isOriAdj) || 
                        (rmvType==RMV_PLAID && !isIndepGrats))
               {
                  fPt1.SetPolar(fPatDelta, m_acvTgts[i].fPatDir);
                  m_RMVUpdVecs[nRMVTgts].hPat = fPt1.GetH();
                  m_RMVUpdVecs[nRMVTgts].vPat = fPt1.GetV();
               }
               
               // if the random-dot patch pattern velocity is WRT screen, then we have to add the target window
               // displacement to the pattern displacement vector!
               if(rmvType == RMV_RANDOMDOTS && isWrtScrn)
               {
                  m_RMVUpdVecs[nRMVTgts].hPat += m_RMVUpdVecs[nRMVTgts].hWin;
                  m_RMVUpdVecs[nRMVTgts].vPat += m_RMVUpdVecs[nRMVTgts].vWin;
               }
            }
            ++nRMVTgts;
         }

         // perform the update, but ignore duplicate frames. If the update fails for another reason, then return to the
         // inactive state in the usual manner (abort recording, discard data file, report error, synch with external 
         // system, resume bkg DAQ). We also stop using RMVideo, since the error is probably serious. To try to use 
         // RMVideo again, user must reload the active target list.
         int nFramesElapsed = -1;
         if(!pRMV->UpdateAnimation(m_RMVUpdVecs, FALSE, nFramesElapsed))
         {
            ::sprintf_s(m_strMsg, "(!!) RMVideo error: %s", pRMV->GetLastDeviceError());
            m_masterIO.Message(m_strMsg);

            pRMV->Init();
            bUsingRMV = FALSE; 

            m_vbStimOn = FALSE; 
            m_bFixOn = FALSE; 
            if(bRecordOn)
            {
               m_masterIO.Message("(!!) Data recording also aborted; data file discarded");
               CloseStream(FALSE);
               while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
               pTimer->TriggerMarkers(RECORDMARKER_MASK); 
               etLastMarker.Reset();
               pTimer->WriteChar(ABORT_CHARCODE);
               pTimer->WriteChar(STOP_CHARCODE);
               pTimer->Init();
               bRecordOn = FALSE;
            }
            dwOpState = 0; 
            m_masterIO.SetResult(dwOpState);

            UnloadEyelinkSample(&bInBlink, -1);
            ConfigureAISeq();
            StartAISeq();
            continue;
         }

         ++nRMVFramesSent;

         // once per second RMVideo sends back its elapsed frame count, which we compare with the number of frames sent
         // thus far. Ideally, Maestro should lead RMVideo by 2-4 frames. If it gets too far ahead or starts to fall 
         // behind that range, then it may be that our current estimate of the RMVideo refresh period is too small or 
         // too large. So we recompute it based on the elapsed frame count and the elapsed time.
         if(nFramesElapsed > 0)
         {
            int diff = nRMVFramesSent - nFramesElapsed;
            if(diff < 2 || diff > 4)
            {
               dRMVFramePerMS = dRMVTimeNowMS / nFramesElapsed;
               m_masterIO.Message("WARNING: Maestro falling behind or getting too far ahead of RMVideo timeline:");
               ::sprintf_s(m_strMsg, "#frames sent = %d, #elapsed = %d, nDups = %d, adjFP = %.5f ms", nRMVFramesSent,
                  nFramesElapsed, pRMV->GetNumDuplicateFrames(), dRMVFramePerMS);
               m_masterIO.Message(m_strMsg);
            }
         }

         // the time of the next update is one refresh period beyond the time of the previous update
         dRMVNextUpdateMS += dRMVFramePerMS;
      }
      // END: RMVIDEO UPDATE

      // present any marker pulses that should be presented during the current tick, but enforce the minimum
      // interval between consecutive marker pulse presentations (this intv is less than a scan interval).
      if(m_vbStimOn && (m_run.dwMarkers != 0))
      {
         while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
         pTimer->TriggerMarkers( m_run.dwMarkers );
         etLastMarker.Reset();
      }

      // update chair velocity command IAW current stimulus run trajectory; or drive toward zero otherwise
      if(m_masterIO.IsChairPresent())
      {
         if(m_vbStimOn && m_run.bUsesChair) pAO->UpdateChair(m_run.fChairVel, fCurrChairPos, m_run.fExpectedChairPos);
         else pAO->SettleChair(fCurrChairPos);
      }

      // BEGIN: DURING FIXATION...
      // current positions of fixation targets and "cursor tracking" target (put off-screen if targets not in use!)
      fPt1.Set(180, 180);
      fPt2.Set(180, 180); 
      fPtTrack.Set(180, 180); 
      // "computed" channels reflecting vel & pos of fixation targets for data trace facility. Zero'd if not in use.
      short shComputed[CX_CP_NCHANS]; 
      for(i = 0; i < CX_CP_NCHANS; i++) shComputed[i] = 0;

      if(m_bFixOn)
      {
         // update current position of fixation target #1, if it is defined. Use "commanded" position stored in 
         // designated target's entry in the active target list. Update computed fixation target position and
         // velocity for use in the GUI's data trace display. We keep track of the target's position in the previous
         // tick in order to compute velocity....
         i = m_fixRewSettings.iFix1;
         if(i >= 0)
         {
            fPt1 = m_acvTgts[i].posCurr;
            shComputed[CX_CP_HPFIX1] = short(fPt1.GetH()*POS_TOAIRAW);
            shComputed[CX_CP_VPFIX1] = short(fPt1.GetV()*POS_TOAIRAW);
            CFPoint fPtVel = fPt1 - lastFix1Pos; 
            fPtVel *= (VEL_TOAIRAW / dT);
            shComputed[CX_CP_HVFIX1] = short(fPtVel.GetH());
            shComputed[CX_CP_VVFIX1] = short(fPtVel.GetV());
            lastFix1Pos = fPt1;
         }

         // analogously for fixation target #2, except that we do not need its position for the data trace display
         i = m_fixRewSettings.iFix2; 
         if(i >= 0)
         {
            fPt2 = m_acvTgts[i].posCurr;
            CFPoint fPtVel = fPt2 - lastFix2Pos;
            fPtVel *= (VEL_TOAIRAW / dT);
            shComputed[CX_CP_HVFIX2] = short(fPtVel.GetH());
            shComputed[CX_CP_VVFIX2] = short(fPtVel.GetV());
            lastFix2Pos = fPt2;
         }

         // check subject's fixation periodically 
         if(m_viFixChkMS <= 0)
         {
            // reload fixation check countdown timer
            m_viFixChkMS = FIXCHKINTV_CONT; 

            // is eye "close enough" to fixation target #1 AND #2 (if used)? At least one fixation tgt must be ON.
            if(m_fixRewSettings.iFix1 >= 0 || m_fixRewSettings.iFix2 >= 0)
            {
               BOOL bFixOk = TRUE;
               if(m_fixRewSettings.iFix1 >= 0) bFixOk = fPt1.IsNear(currEyePos, m_fixRewSettings.fPtAccuracy);
               if(m_fixRewSettings.iFix2 >= 0) bFixOk = bFixOk && fPt2.IsNear(currEyePos,m_fixRewSettings.fPtAccuracy);

               // if fixation OK, reset "fixation broken" duration, increment "fixation held" duration, and raise the
               // "fixation OK" signal. Else, increment the "broken" duration. If it is less than a fixed "grace time",
               // fixation is considered OK and the "held" duration is increment; else, reset the "held" duration and 
               // lower the "fixation OK" signal.
               if(bFixOk)
               {
                  iBrokeFixDur = 0;
                  iHeldFixDur += FIXCHKINTV_CONT;
                  pTimer->SetFixationStatus();
               }
               else
               {
                  iBrokeFixDur += FIXCHKINTV_CONT;
                  if(iBrokeFixDur <= GRACEPERIOD_CONT)
                     iHeldFixDur += FIXCHKINTV_CONT;
                  else
                  {
                     iHeldFixDur = 0;
                     pTimer->ClearFixationStatus();
                  }
               }

               // if subject has fixated target for the required duration, reset duration count and deliver a reward, 
               // possibly subject to random withholding. If it is indeeded delivered, update reward stats in IPC and
               // optionally beep the onboard speaker to cue the user that a reward was delivered.
               if(iHeldFixDur > m_fixRewSettings.iDur)
               {
                  iHeldFixDur = 0;
                  if(pTimer->DeliverReward(m_fixRewSettings.iWHVR, m_fixRewSettings.iRewLen1, 
                     m_fixRewSettings.iAudioRewLen))
                  {
                     m_masterIO.IncrementNumRewards();
                     m_masterIO.AccumulateRewardPulse(m_fixRewSettings.iRewLen1);
                     if(m_fixRewSettings.bPlayBeep) m_masterIO.Message("beep");
                  }
               }
            }
         }

         // we're done with current fixation target positions in fPt1 and fPt2, as far as fixation checking goes. But
         // these vars are used to update fix tgt pos in the GUI's eye-target position plot (see later code section).
         // IF fixation target is turned OFF, we need to put this position offscreen so we don't see it in pos plot!
         i = m_fixRewSettings.iFix1;
         if(i >= 0 && !m_acvTgts[i].bOn) fPt1.Set(180, 180);
         i = m_fixRewSettings.iFix2;
         if(i >= 0 && !m_acvTgts[i].bOn) fPt2.Set(180, 180);

         // update position of cursor tracking target -- a tracking target must be defined and turned on
         i = m_fixRewSettings.iTrack; 
         if(i >= 0 && m_acvTgts[i].bOn) fPtTrack = m_acvTgts[i].posCurr;
      }
      // END: DURING FIXATION

      // now we're ready to update the GUI's eye-target position plot and data trace display for the current tick
      UpdateLoci(fPt1, fPt2, fPtTrack);
      m_masterIO.UpdateTrace(m_pshLastScan, &(shComputed[0]), dwEventsThisTick);

      // BEGIN: PROCESS NEXT COMMAND FROM MAESTROGUI...
      DWORD dwCmd = m_masterIO.GetCommand(); 
      if(dwCmd != CX_NULLCMD) switch(dwCmd)
      {
         // switch to a different operational mode
         case CX_SWITCHMODE : 
            m_masterIO.GetCommandData(&iOpMode, NULL, 1, 0);
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0, TRUE);
            break;
         // change current list of AI channels saved when recording -- ignore if recording is currently in progress
         case CX_SAVECHANS :  
            if(!bRecordOn) UpdateAISaveList();
            else m_masterIO.AckCommand(CX_FAILEDCMD, NULL,NULL,0,0);
            break;
         // change current video display parameters, but ONLY in the inactive state
         case CX_SETDISPLAY : 
            if(m_bFixOn || bRecordOn || m_vbStimOn) m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);
            else if(!bUsingRMV) UpdateVideoDisplaysAndAck();
            else 
            {
               // when RMVideo targets are in active target list, then RMVideo is animating even though all targets
               // are off. We MUST stop animation, update the display params, then resume animation. Since this takes
               // a while, we ack command first. And afterwards, we restart the background DAQ
               int iParams[15];
               m_masterIO.GetCommandData(&(iParams[0]), NULL, 15, 0);
               m_masterIO.AckCommand(CX_SETDISPLAY, NULL, NULL, 0, 0, TRUE);

               // flush Eyelink sample buffer now because this command may take a while
               UnloadEyelinkSample(&bInBlink, -1);

               pRMV->Init();
               UpdateVideoDisplays(&(iParams[0]));
               bUsingRMV = LoadRMVideoTargets();
               UnloadEyelinkSample(&bInBlink, -1);
               if(bUsingRMV)
               {
                  memset(m_RMVUpdVecs, 0, MAX_ACTIVETGTS*sizeof(RMVTGTVEC));
                  bUsingRMV = pRMV->StartAnimation(m_RMVUpdVecs, m_RMVUpdVecs, FALSE);
                  // upon starting the animation, reset elapsed time and restore nominal refresh period.
                  // Note that 2 frames are sent initially by StartAnimation().
                  eRMVTime.Reset();
                  dRMVNextUpdateMS = 0;
                  nRMVFramesSent = 2;
                  dRMVFramePerMS = pRMV->GetFramePeriod() * 1000.0; 
                  if(!bUsingRMV)
                  {
                     ::sprintf_s(m_strMsg, "(!!) RMVideo error on startAnimation: %s", pRMV->GetLastDeviceError());
                     m_masterIO.Message(m_strMsg);
                  }
               }
               UnloadEyelinkSample(&bInBlink, -1);
               ConfigureAISeq();
               StartAISeq();
            }
            break;
         // reinitialize MaestroGUI's data trace facility
         case CX_INITTRACE : 
            m_masterIO.AckCommand(m_masterIO.InitTrace() ? dwCmd : CX_FAILEDCMD, NULL, NULL, 0, 0);
            break;
         // enable fixation checking: inform MaestroGUI of state change, arm fixation check interval timer, reset
         // fixation duration counts, and ack the command. If fixation checking already enabled, merely ack the command.
         case CX_CM_FIXON : 
            if(!m_bFixOn) 
            {
               m_bFixOn = TRUE;
               dwOpState |= CX_FC_FIXATING;
               m_masterIO.SetResult(dwOpState);
               m_viFixChkMS = FIXCHKINTV_CONT;
               iBrokeFixDur = iHeldFixDur = 0;
            }
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // disable fixation checking, if it is currently enabled.
         case CX_CM_FIXOFF : 
            if(m_bFixOn) 
            {
               m_bFixOn = FALSE;
               dwOpState &= ~CX_FC_FIXATING; 
               m_masterIO.SetResult(dwOpState);
            }
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // update fixation/reward settings
         case CX_FIXREWSETTINGS : 
            UpdateFixRewSettings();
            break;
         // update fixation target indices; which are in the command data (ASSUME they're valid)
         case CX_CM_UPDFIXTGTS : 
            m_masterIO.GetCommandData(&(m_fixRewSettings.iFix1), NULL, 3, 0);
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // update the active target list
         case CX_CM_UPDACVTGT : 
            m_masterIO.GetCommandData( &i, NULL, 1, 0 );                // get target ID
            if(i == -1 )  
            {
               // wholesale update -- composition of active list has changed. Only allowed when the system is inactive: 
               // not recording, not fixating, no stimulus run in progress
               if(bRecordOn || m_bFixOn || m_vbStimOn)
               {
                  m_masterIO.AckCommand(CX_ILLEGALCMD, NULL, NULL, 0, 0);
                  break;
               }

               // get #tgts in active list; get type, on/off state, etc for each of the targets in the list
               int nTgts = m_masterIO.GetNumTargets(); 
               float fPos[MAX_ACTIVETGTS*4]; 
               int iOn[MAX_ACTIVETGTS+1]; 
               m_masterIO.GetCommandData(&(iOn[0]), &(fPos[0]), nTgts+1, nTgts*4);
               bUsingRMV = FALSE;
               for(i = 0; i < nTgts; i++)
               {
                  m_masterIO.GetTarget(i, m_acvTgts[i].tgtDef);
                  if(m_acvTgts[i].tgtDef.wType == CX_RMVTARG) bUsingRMV = TRUE;
                  m_acvTgts[i].bOn = BOOL( iOn[i+1] );
                  m_acvTgts[i].posCurr.Zero();
                  m_acvTgts[i].posNext.Set( fPos[4*i], fPos[4*i+1] );
                  m_acvTgts[i].fPatSpeed = fPos[4*i+2];
                  m_acvTgts[i].fPatDir= fPos[4*i+3];
               }
               
               // flush EyeLink sample buffer now because the next tasks may take some time
               UnloadEyelinkSample(&bInBlink, -1);

               // if any RMVideo targets in active list and RMVideo device is available, load the target definitions and
               // start "animating" with all targets OFF and at the origin. They won't get move or turned on until
               // fixation checking is enabled. Because these operations can take a while, we must restart the 
               // background AI sequence when we're done!!
               if(bUsingRMV)
               {
                  if(!pRMV->IsOn())
                  {
                     bUsingRMV = FALSE;
                     m_masterIO.Message("(!!) RMVideo not available; RMVideo targets ignored.");
                  }
                  else
                  {
                     bUsingRMV = LoadRMVideoTargets(); 
                     UnloadEyelinkSample(&bInBlink, -1);
                     if(bUsingRMV)
                     {
                        memset(m_RMVUpdVecs, 0, MAX_ACTIVETGTS*sizeof(RMVTGTVEC));
                        bUsingRMV = pRMV->StartAnimation(m_RMVUpdVecs, m_RMVUpdVecs, FALSE);
                        // upon starting the animation, reset elapsed time and restore nominal refresh period.
                        // Note that 2 frames are sent initially by StartAnimation().
                        eRMVTime.Reset();
                        dRMVNextUpdateMS = 0;
                        nRMVFramesSent = 2;
                        dRMVFramePerMS = pRMV->GetFramePeriod() * 1000.0;
                        if(!bUsingRMV)
                        {
                           ::sprintf_s(m_strMsg, "(!!) RMVideo error on startAnimate: %s", pRMV->GetLastDeviceError());
                           m_masterIO.Message(m_strMsg);
                        }
                     }

                     UnloadEyelinkSample(&bInBlink, -1);
                     ConfigureAISeq();
                     StartAISeq();
                  }
               }
            }
            else
            { 
               // update on/off state, pos, etc for a single specified tgt currently in the active list
               int iOn[2];
               float fPos[4];
               m_masterIO.GetCommandData(&(iOn[0]), &(fPos[0]), 2, 4);
               if(i < 0 || i >= m_masterIO.GetNumTargets() || i >= MAX_ACTIVETGTS)
               {
                  m_masterIO.AckCommand(CX_ILLEGALCMD, NULL, NULL, 0, 0);
                  break;
               }
               m_acvTgts[i].bOn = BOOL( iOn[1] );
               m_acvTgts[i].posNext.Set( fPos[0], fPos[1] );
               m_acvTgts[i].fPatSpeed = fPos[2];
               m_acvTgts[i].fPatDir = fPos[3];
            }
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // start the stimulus run, unless one is already in progress
         case CX_CM_RUNSTART : 
            if(m_vbStimOn) 
            {
               m_masterIO.AckCommand(CX_ILLEGALCMD, NULL, NULL, 0, 0);
               break;
            }

            dwOpState |= CX_FC_RUNON; 
            m_masterIO.SetResult(dwOpState);
            StartStimulusRun(); 
            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // stop the stimulus run, if one is currently in progress
         case CX_CM_RUNSTOP : 
            if(!m_vbStimOn)
            {
               m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
               break;
            }

            m_masterIO.GetCommandData( &i, NULL, 1, 0 );                // get stop condition
            if(i == 0)
            {
               // case 1: stop at end of current duty cycle. We merely set a flag to request the stop.
               dwOpState |= CX_FC_RUNSTOPPING;
               m_masterIO.SetResult(dwOpState);
               m_run.bSoftStopReq = TRUE;
            }
            else 
            {
               // case 2,3: stop NOW
               m_vbStimOn = FALSE; 
               dwOpState &= ~(CX_FC_RUNON|CX_FC_RUNSTOPPING);
               m_masterIO.SetResult(dwOpState);

               // case 3: abort requested. If we were recording, stop, discard data file, and resume idle bkg DAQ
               if(i == 2 && bRecordOn)
               {
                  m_masterIO.Message("(!!) Data recording aborted by user; data file discarded");
                  CloseStream(FALSE);
                  
                  // the usual stuff to sync with external system
                  while(etLastMarker.Get() < MIN_MARKERINTVUS) ;
                  pTimer->TriggerMarkers(RECORDMARKER_MASK);
                  etLastMarker.Reset();
                  pTimer->WriteChar(ABORT_CHARCODE);
                  pTimer->WriteChar(STOP_CHARCODE);

                  pTimer->Init();
                  bRecordOn = FALSE;
                  dwOpState &= ~CX_FC_RECORDING;
                  m_masterIO.SetResult(dwOpState);

                  UnloadEyelinkSample(&bInBlink, -1);
                  ConfigureAISeq();
                  StartAISeq();
               }
            }

            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0); 
            break;
         // turn ON data recording, unless it is already on
         case CX_CM_RECON : 
            if(bRecordOn) 
            {
               m_masterIO.AckCommand(CX_ILLEGALCMD, NULL, NULL, 0, 0);
               break;
            }

            // !!!IMPORTANT!!! Remember that a stimulus run may be in progress when this cmd is received.  We want to
            // "freeze" the run's timeline while doing the work required to start recording. This was important
            // for XYseq runs -- you can sometimes cut short an XY frame if you fail to stop the AI timeline here!! But
            // XYseq runs are not supported anymore since the XYScope platform is no longer supported.
            pAI->Init();

            // get data file name, open file stream, init stream buffers, and write the file header
            m_masterIO.GetDataFilePath(m_string, CX_MAXPATH);
            if(!OpenStream(m_string))
            {
               m_masterIO.Message("(!!) Unable to open data file. Invalid path or file already exists?");
               m_masterIO.AckCommand(CX_FAILEDCMD, NULL, NULL, 0, 0);

               // resume the bkg DAQ if we cannot open file
               pTimer->Init();
               UnloadEyelinkSample(&bInBlink, -1);
               ConfigureAISeq();
               StartAISeq();
               break;
            }

            // reinitialize and configure event timer to timestamp events on DI<15..0> with 10us res, but don't start
            pTimer->Init();
            pTimer->Configure(10, 0x0000FFFF);

            // send 'start' code and data file name to sync with an external system
            pTimer->WriteChar(START_CHARCODE); 
            m_masterIO.GetDataFileName(m_string, CX_MAXPATH); 
            pTimer->WriteString(m_string, (int) (::strlen(m_string) + 1));

            // we don't want to trigger the record start marker pulse to soon after another marker pulse was delivered
            while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 

            // start recording, then issue record start pulse for sync with external system. Note that timestamp stored
            // in data file header marks the moment after AI sequence started but before the "start" pulse is triggered.
            bRecordOn = TRUE; 
            dwOpState |= CX_FC_RECORDING; 
            m_masterIO.SetResult(dwOpState);
            UnloadEyelinkSample(&bInBlink, -1);
            ConfigureAISeq(m_masterIO.IsSpikeTraceOn());
            pTimer->Start();
            StartAISeq();
            m_Header.timestampMS = (int)(m_eRunTimeUS.Get() / 1000.0);
            pTimer->TriggerMarkers(RECORDMARKER_MASK);
            etLastMarker.Reset();

            // reset elapsed recording time. This is used only to compare against Eyelink tracker sample timestamps.
            // NOTE that we do not try to sync with the Eyelink timeline, as we do at the start of a trial.
            nRecTimeMS = 0;

            m_masterIO.AckCommand(dwCmd, NULL, NULL, 0, 0);
            break;
         // turn OFF data recording if it is currently on
         case CX_CM_RECOFF : 
            if(!bRecordOn)
            {
               m_masterIO.AckCommand(CX_ILLEGALCMD, NULL, NULL, 0, 0);
               break;
            }

            // issue record stop marker pulse to sync with an external system, and stop recording data IMMEDIATELY after
            while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
            pTimer->TriggerMarkers(RECORDMARKER_MASK); 
            etLastMarker.Reset();

            bRecordOn = FALSE; 
            pAI->Stop();
            pTimer->Stop();

            // this is a hack to make sure any pending interrupts are handled (RTX int latency typ < 20us)
            eTime.Reset(); 
            while( eTime.Get() < 50.0 ) ; 

            // inform MaestroGUI that we stopped recording and may be blocked while saving data file
            dwOpState &= ~CX_FC_RECORDING; 
            dwOpState |= CX_FC_SAVING; 
            m_masterIO.SetResult(dwOpState);

            // if we are saving data file, unload and store remaining analog data and digital event timestamps
            bOk = TRUE;
            m_masterIO.GetCommandData(&i, NULL, 1, 0); 
            if(i != 0)
            {
               while(bOk && (m_viElapsedTicks > m_viServicedTicks))
               {
                  // don't fail if we lose a little bit of data at the end of recording -- it's highly unlikely anyway.
                  if(!UnloadNextAIScan(FALSE)) break; 
                  bOk = StreamAnalogData();
               }
               if(bOk)
               {
                  m_nEvents = pTimer->UnloadEvents(EVENTBUFSZ, &(m_events[0]), &(m_evtTimes[0]));
                  bOk = StreamEventData();
               }
            }

            // the above may have taken a little while -- be sure to unload Eyelink sample buffer
            UnloadEyelinkSample(&bInBlink, -1);
            
            // if Eyelink tracker was used, record stats on in inter-sample interval in data file header
            if(bUsingEL && (m_nELSamples > 0))
            {
               m_Header.iELInfo[6] = m_nELRepeats;
               m_Header.iELInfo[7] = m_maxELSampDelay;
               m_Header.iELInfo[8] = (int) (1000.0 * ((double) m_accumELSampDelay) / ((double) m_nELSamples));
            }

            // close file writer: discard file if we're not saving it OR if a file IO error occurred above
            if(!CloseStream( BOOL(bOk && i!=0) )) bOk = FALSE;

            // if a file IO error prevented data save, inform user and clear "save OK" flag; else, if data was saved,
            // send char code informing external system of this fact
            if(!bOk) 
            {
               m_masterIO.Message("(!!) File IO error at end of recording -- data file discarded");
               i = 0;
            }
            else if(i != 0) pTimer->WriteChar(DATASAVED_CHARCODE);

            // tells external system that recording sequence is complete
            pTimer->WriteChar(STOP_CHARCODE);

            // tells MaestroGUI we're done with file I/O
            dwOpState &= ~CX_FC_SAVING; 
            m_masterIO.SetResult(dwOpState);

            // ack command, returning state of the "save OK" flag
            m_masterIO.AckCommand(dwCmd, &i, NULL, 1, 0);

            // restart idle bkg DAQ
            UnloadEyelinkSample(&bInBlink, -1);
            ConfigureAISeq(); 
            StartAISeq(); 
            break;
         // command not recognized in this mode
         default : 
            m_masterIO.AckCommand(CX_UNRECOGCMD, NULL, NULL, 0, 0);
            break;
      }
      // END: PROCESS NEXT COMMAND
   }
   // END: CONTINUOUS-MODE RUNTIME LOOP

   // CLEAN UP:
   // if user leaves Continuous mode while recording is on, inform user and discard data file
   if(bRecordOn)
   {
      m_masterIO.Message("(!!) Recording aborted by mode switch -- data file discarded");
      CloseStream(FALSE);
      bRecordOn = FALSE;

      // notify external system that data recording was aborted
      while(etLastMarker.Get() < MIN_MARKERINTVUS) ; 
      pTimer->TriggerMarkers(RECORDMARKER_MASK);
      etLastMarker.Reset();
      pTimer->WriteChar(ABORT_CHARCODE);
      pTimer->WriteChar(STOP_CHARCODE);
   }

   UnloadEyelinkSample(&bInBlink, -1);
   m_vbStimOn = FALSE;                                                  // stimulus run stopped
   m_bFixOn = FALSE;                                                    // disable fixation checking
   pTimer->ClearFixationStatus();                                       // reset fixation status signal
   pAO->InitChair();                                                    // zero chair velocity drive signal
   pAI->Init();                                                         // reset AI device
   pTimer->Init();                                                      // reset event timer device
   if(pRMV->IsOn() && !pRMV->Init())                                    // return RMVideo to idle state (no tgts)
   {
      ::sprintf_s(m_strMsg, "(!!)Failed to reinit RMVideo upon exiting Cont mode (%s)", pRMV->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
   }
   UnloadEyelinkSample(&bInBlink, -1);

   // restore suspend manager to prior state and switch to new op mode
   m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
   m_masterIO.SetMode(iOpMode);
}


/**
 Helper method for RunContinuousMode(). Reads the stimulus run definition currently in CXIPC, initializes runtime 
 control info for the stimulus run, and starts it.

 [18nov2024] As of Maestro 5.0.2, the only available channel type in a stimulus run is STIM_ISCHAIR; PSGM and
 XYseq support both dropped.
*/
VOID RTFCNDCL CCxDriver::StartStimulusRun()
{
   int i;

   // clear stimulus run enable flag and get a private copy of the stimulus run definition from IPC
   m_vbStimOn = FALSE; 
   m_masterIO.GetStimRunDef(m_run.def);

   // initialize runtime control state variables
   m_run.bUsesChair = FALSE;
   m_run.tLastUpdate = -1;                   // this signals the beginning of a new duty cycle!
   m_run.iCycles = 0;
   m_run.bSoftStopReq = FALSE;
   m_run.fChairVel = 0.0f;
   m_run.fExpectedChairPos = 0.0f;
   m_run.dwMarkers = 0;

   BOOL bAOAvail = m_masterIO.IsAOAvailable();

   // find which stimulus platforms are used and enable stimulus only if required hardware device is present; if not,
   // turn the stimulus OFF. The XYseq stimulus run is DEPRECATED, as the XYScope is no longer supported. The PSGM has
   // also been deprecated -- so the ONLY supported stimulus channel is STIM_ISCHAIR!
   for(i = 0; i < m_run.def.nStimuli; i++)
   {
      PSTIMCHAN pStim = &(m_run.def.stim[i]);
      if(pStim->bOn && (pStim->iType == STIM_ISCHAIR))
      {
         m_run.bUsesChair = bAOAvail && m_masterIO.IsChairPresent();
         pStim->bOn = m_run.bUsesChair;
      }
   }

   m_viStimTicks = -1;                                               // reset elapsed time: 1st "tick" ==> T=0!!
   m_viStimDutyCycle = m_run.def.iDutyPeriod / m_viScanInterval;     // end of duty cycle in #scans ("ticks")
   m_vbStimOn = TRUE;                                                // start stimulus run!
}


/**
 Helper method for RunContinuousMode(). Advances the trajectories of all active stimulus channels to the specified time 
 point in an ongoing stimulus run.

 [18nov2024] As of Maestro 5.0.2, the only available channel type in a stimulus run is STIM_ISCHAIR; PSGM and
 XYseq support both dropped.

 @param tCurrent Current elapsed time (ms) in duty cycle of the stimulus run.
*/
VOID RTFCNDCL CCxDriver::UpdateStimulusRun(int tCurrent)
{
   int i;

   // reset "per-update" motion vars. These are used to update the relevant hardware device on each update, so they are
   // recomputed from scratch at every timepoint.
   m_run.fChairVel = 0.0f; 
   m_run.fExpectedChairPos = 0.0f; 
   m_run.dwMarkers = 0;

   // issue duty cycle marker pulse at the start of each duty cycle
   if(m_run.tLastUpdate == -1 && m_run.def.iDutyPulse > 0) m_run.dwMarkers |= (DWORD) (1 << m_run.def.iDutyPulse);

   // for each channel defined in the stimulus run...
   for(i = 0; i < m_run.def.nStimuli; i++)
   {
      // ignore any channel that's OFF
      PSTIMCHAN pStim = &(m_run.def.stim[i]);
      if(!(pStim->bOn)) continue;

      // raise associated marker pulse (if any) when stimulus channel starts 
      if((pStim->iMarker > 0) && (tCurrent >= pStim->tStart) && (m_run.tLastUpdate < pStim->tStart))
         m_run.dwMarkers |= (DWORD) (1 << pStim->iMarker);

      // time relative to the start of this stimulus channel (ms)
      int t = tCurrent - pStim->tStart; 

      // calculate trajectory for the CHAIR stimulus channel
      if(pStim->iType == STIM_ISCHAIR) 
      { 
         // stimulus channel params define a velocity trajectory, but we must integrate to get the chair's expected
         // position so that we can do drift compensation during the stimulus run
         double dVel = 0.0;
         double dPos = 0.0;
         BOOL bMoving = FALSE;

         if(pStim->iStdMode == MODE_ISSINE) 
         {
            // SINE: v(t) = A*sin(2*PI*t'/T + phi) for t'=t-tStart in [0..N*T] (ms), where N = #cycles, A = amplitude in
            // deg/s, T = period in ms, and phi = phase in deg. Outside [0..N*T] there's not motion. NOTE conversion of
            // msec --> sec.
            int tEnd = pStim->sine.nCycles * pStim->sine.iPeriod;
            if(t >= 0 && t <= tEnd) 
            { 
               double dAmp = double(pStim->sine.fAmp); 
               double dOmega = cMath::TWOPI * 1000.0 / double(pStim->sine.iPeriod);
               double dOmega_t = dOmega * double(t) / 1000.0;
               double dRad = dOmega_t + cMath::toRadians(double(pStim->sine.fPhase));
               while(dRad >= cMath::TWOPI) dRad -= cMath::TWOPI;

               dVel = dAmp * sin(dRad);
               dRad -= cMath::PI/2.0;
               dPos = (dAmp/dOmega) * (sin(dRad) - sin(dRad-dOmega_t));
               bMoving = TRUE;
            }
         }
         else if(pStim->iStdMode == MODE_ISPULSE) 
         { 
            // PULSE: Let t'=t-tStart in sec, D = pulse dur in ms, R = ramp dur in ms and A = pulse amp in deg/sec.
            int t1 = pStim->pulse.iRampDur; 
            int t2 = t1 + pStim->pulse.iPulseDur;
            int t3 = t2 + pStim->pulse.iRampDur;

            // ramp "slope" = A/R in deg/sec^2. NOTE conversion of ramp dur in ms to sec!
            double dSlope = double(pStim->pulse.fAmp) * 1000.0; 
            dSlope /= double(pStim->pulse.iRampDur);

            // t', converted from ms --> sec
            double dTime = double(t) / 1000.0; 

            bMoving = TRUE;
            if(t >= 0 && t < t1)
            {
               // for t' in [0..R): v(t') = slope * t'; p(t') = 0.5 * slope * (t')^2
               dVel = dSlope * dTime;
               dPos = dVel * dTime / 2.0;
            }
            else if(t >= t1 && t < t2)
            {
               // for t' in [R..R+D): v(t') = A, p(t') = A * (t'- R/2)
               dVel = double(pStim->pulse.fAmp);
               dPos = dVel * (dTime - double(pStim->pulse.iRampDur)/2000.0);
            }
            else if(t >= t2 && t < t3)                                //       for t' in [R+D..2R+D)...
            {
               // for t' in [R+D..2R+D): v(t') = slope * dT, p(t') = A*(R+D in secs) = slope * (dT^2) / 2.0, where
               // dT = 2R+D - t' in secs
               double d2 = double(pStim->pulse.fAmp)*double(t2)/1000.0;    // A*(R+D in secs)
               double dT = double(t3)/1000.0 - dTime; 
               dVel = dSlope * dT; 
               dPos = d2 - dSlope * dT * dT / 2.0; 
            }
            else 
            { 
               // for all other t', v(t') = 0; for t'>2R+D, p(t') = A*(R+D) in secs
               if(t >= t3) dPos = double(pStim->pulse.fAmp)*double(t2)/1000.0;
               bMoving = FALSE;
            }
         }

         // now add contribution of this stimulus channel to the overall motion of the chair. Note that more than one
         // stimulus channel affecting the chair can be active at the same time! Also note that the polarity of the
         // chair position is opposite that of the chair drive velocity signal.
         m_run.fChairVel += float(dVel);
         m_run.fExpectedChairPos += float(-dPos);
      }
   }
}


/**
 OpenStream, StreamAnalog/EventData, CloseStream

 These methods implement "on-the-fly" data streaming to store analog and DI event data recorded during a trial or while
 in Continuous Mode: compressed analog "slow" data (CX_AIRECORD), compressed analog "fast" data (representing a 25KHz 
 recording of the spike waveform, CX_SPIKEWAVERECORD), interevent intervals on DI ch 0 and 1 (CX_EVENT0RECORD and 
 CX_EVENT1RECORD), and (event mask, time)-pairs on DI ch 2-15 (CX_OTHEREVENTRECORD). See also CXFILEFMT.H.

 STRATEGY: A separate record is prepared for each type of data, the record tag field is set appropriately, and the rest 
 of the record is initially empty. During recording, StreamAnalogData() and StreamEventData() are called on a regular 
 basis to "move" recorded data from temporary buffers to the appropriate formatted record. In the process, analog data 
 are compressed, while event data is stored either as interevent intervals or as (event mask, time)-pairs. A counter 
 variable associated with each record keeps track of where the next datum is stored. Once a record is full, it is 
 written to file via the CCxFileWriter object, which copies the record to an internal queue and transparently writes it 
 to file in a background thread. The record can thus be safely reused to stream additional data as it becomes available.

 The data file "header record" is prepared and written to the file when it is first opened, in OpenStream(). There are 
 three counters in this header record that are continuously updated by the StreamAnalogData() method, tracking: (1) 
 total # of "slow" scan sets saved, (2) total #bytes compressed of "slow" scan data, and (3) total #bytes compressed of 
 the 25KHz spike waveform data. The counters are initialized when we prepare the header in OpenStream(). To serialize 
 these counter values at the end of recording, CloseStream() writes the header record again in its final form. (When a
 trial finishes, the trial file's header record is written in its entirety prior to calling CloseStream().)

 Several other "information records" are also written by OpenStream(): target definitions and trial codes for a Trial
 mode data file; or target definitions and stimulus run information for a Continuous mode file.

 OpenStream(): Open the specified file using the CCxFileWriter object. Initialize all data buffers and records for the 
 data streaming operation. Write the header record to file. Then write any relevant "information records":
 
    1) Trial Mode. Write a CX_TAGSECTRECORD record listing defined tagged sections in the trial, if any; one or more
 CX_TGTRECORD records defining the targets participating in the trial; and one or more CX_TRIALCODERECORD records
 containing the complete list of codes that defines the trial executed.
    
    2) Continuous Mode. Write any CX_STIMRUNRECORD record(s) that "persist" the definition of the current stimulus run, 
 IF a run is defined; only stimulus channels that are turned ON are included. Also, write CX_TGTRECORD record(s) storing
 definitions of all targets CURRENTLY in the active target list, followed by all targets in the currently defined 
 stimulus run's XYseq target list, but only if the run uses the XYseq stimulus.
    !!! CAVEAT: Target and stimulus run information saved here merely represent MaestroRTSS's state AT THE TIME
    !!! RECORDING BEGAN. In Continuous mode, active target info can be interactively changed by the user as recording
    !!! progresses. Also, we save the current stimulus run information even if it is not actually running
    !!! because, in typical usage, MaestroGUI will download the stimulus run to MaestroRTSS, then start recording,
    !!! then start the previously loaded run! We include a flag that indicates whether or not the currently
    !!! defined run is actually in progress.
    !!!
    !!! (25sep2024): The XYScope has not been supported since Maestro 4.0, and we're removing all related code for the
    !!! Maestro 5.0 release.
    !!! (18nov2024): Dropped support for writing CX_STIMRUNRECORD records a/o Maestro 5.0.2.

 CloseStream(): Fill any remaining space in non-empty data records with "end-of-data" values, write those records to the 
 file writer, write the final version of the header record, and close the file writer. This will flush all "queued" 
 records to the disk file and close the file. Since this could take a little while (depending on how many records are 
 still in file writer queue), do not call CloseStream() in time-critical code. If a file IO error has occurred or occurs
 during the final flush, the data file is automatically discarded!

 StreamAnalogData(): Compress and store analog data in both the "slow" and "fast" data buffers. The "slow" data buffer 
 will contain one or two scan's worth of samples from all CX_AIO_MAXN analog input channels. However, we only process 
 samples from channels in the current "save" list. The "fast" data buffer will hold samples recorded at 25KHz on the 
 dedicated SPIKECHANNEL -- but only if spike waveform recording is enabled.
    !!! NOTE: We compress the data by saving only the difference between successive samples. The algorithm used 
    !!! here IMPLICITLY REQUIRES that the raw AI samples have 12-bit resolution, range -2048..2047.

 StreamEventData(): Empty the current events buffer, storing event info in one of the three different event data
 records (see above).

 @param strPath Full pathname of data file to be opened (must not yet exist).
 @param bSave TRUE to save the data file upon "closing" the streams; FALSE to discard data file.
 @return TRUE if successful; FALSE if file writer could not open file or a write operation failed.
*/
BOOL RTFCNDCL CCxDriver::OpenStream(LPCTSTR strPath)
{
   int i, j;
   BOOL isCont = BOOL(m_masterIO.GetMode() == CX_CONTMODE);

   // openf file with file writer object.
   if(!m_writer.Open(strPath)) return(FALSE);

   // prepare data file's header record. For TM, we just zero it. It will be filled just prior to closing the 
   // stream, at which point it will be written again to file. For CM, we go ahead and fill out most of the relevant
   // fields now... 
   ::memset((PVOID) &m_Header, 0, sizeof(CXFILEHDR)); 
   if(isCont)
   {
      ::strcpy_s(m_Header.name, "**continuous_mode_run**");
      m_Header.nchans = (short) m_nSavedCh;
      for(i = 0; i < m_nSavedCh; i++) m_Header.chlist[i] = (short) m_iChannels[i];

      m_Header.iRewLen1 = m_fixRewSettings.iRewLen1;

      m_Header.dayRecorded = m_masterIO.GetDayOfMonth();
      m_Header.monthRecorded = m_masterIO.GetMonthOfYear();
      m_Header.yearRecorded = m_masterIO.GetYear();
      m_Header.version = CXH_CURRENTVERSION;

      m_Header.flags |= CXHF_ISCONTINUOUS; 
      
      if(m_masterIO.IsSpikeTraceOn())
      {
         m_Header.flags |= CXHF_SAVEDSPIKES;
         m_Header.nSpikeSampIntvUS = SPIKESAMPINTVUS;
      }

      m_Header.nScanIntvUS = CONTSCANINTVUS;

      // if Eyelink tracker is used, store record type (monoL/R vs binocular), cal params, etc
      if(m_masterIO.IsEyelinkInUse())
      {
         m_Header.flags |= CXHF_EYELINKUSED;
         m_Header.iELInfo[0] = m_masterIO.GetEyelinkRecordType();
         m_Header.iELInfo[1] = m_masterIO.GetEyelinkOffset(TRUE);
         m_Header.iELInfo[2] = m_masterIO.GetEyelinkOffset(FALSE);
         m_Header.iELInfo[3] = m_masterIO.GetEyelinkGain(TRUE);
         m_Header.iELInfo[4] = m_masterIO.GetEyelinkOffset(FALSE);
         m_Header.iELInfo[5] = m_masterIO.GetEyelinkVelocityWindowWidth();
         // note: inter-sample stats will be set when recording ends
      }
   }

   // write the header record to the file stream. It must ALWAYS be the first record in the file. We will write over
   // it with its final form just prior to closing the stream.
   if(!m_writer.Write((PVOID) &m_Header))
   {
      m_writer.Close(FALSE);
      return(FALSE);
   }

   // write any relevant "information records". These are different for a Continuous-mode vs Trial-mode file.
   if(isCont)
   {
      // write defns of targets in the current active target list in one or more "target definition records".
      ::memset((PVOID) &m_Record, 0, sizeof(CXFILEREC));
      m_Record.idTag[0] = CX_TGTRECORD;
      int nTgts = m_masterIO.GetNumTargets();

      i = 0; 
      j = 0; 
      while(i < nTgts)
      {
         // store next active target defn in next available slot in the record.
         CXFILETGT* pFT = &(m_Record.u.tgts[j]); 
         m_masterIO.GetTarget(i, pFT->def);

         // for a random-dot RMVideo target, we need to store actual seed used IF CXDRIVER randomly chose it.
         if (pFT->def.wType == CX_RMVTARG)
         {
            PRMVTGTDEF pRMV = &(pFT->def.u.rmv);
            if ((pRMV->iType == RMV_RANDOMDOTS || pRMV->iType == RMV_FLOWFIELD) && (pRMV->iSeed == 0))
               pRMV->iSeed = m_iRMVSeed[i];
         }

         // active target state information, current position
         DWORD dwState = CXFTF_ISACVTGT;
         if (m_acvTgts[i].bOn) dwState |= CXFTF_TGTON;
         if (i == m_fixRewSettings.iFix1) dwState |= CXFTF_TGTISFIX1;
         if (i == m_fixRewSettings.iFix2) dwState |= CXFTF_TGTISFIX2;
         pFT->dwState = dwState;
         pFT->fPosX = m_acvTgts[i].posCurr.GetH();
         pFT->fPosY = m_acvTgts[i].posCurr.GetV();

         ++i;
         ++j;

         // stream a full tgt defn record or the last partial record to disk. Abort on failure; else, clear the record
         // to prepare it for more tgt defns. Because the record always starts out w/all zeros, any "dummy" tgt defns in
         // the last partial record are recognized as invalid b/c the tgt type will be 0.
         if(j == CX_RECORDTARGETS || i == nTgts)
         {
            if(!m_writer.Write((PVOID) &m_Record))
            {
               m_writer.Close(FALSE);
               return(FALSE);
            }
            memset(&m_Record, 0, sizeof(CXFILEREC));
            m_Record.idTag[0] = CX_TGTRECORD;
            j = 0;
         }
      }
   }
   else
   {
      // Trial mode file: (1) Write a single record listing any tagged section in the trial, if any.
      int nSections = m_masterIO.GetNumTaggedSections(); 
      if(nSections > 0)
      {
         memset(&m_Record, 0, sizeof(CXFILEREC));
         m_Record.idTag[0] = CX_TAGSECTRECORD;
         for(i=0; i<nSections; i++) m_masterIO.GetTaggedSection(i, m_Record.u.sects[i]);

         if(!m_writer.Write((PVOID) &m_Record))
         {
            m_writer.Close(FALSE);
            return(FALSE);
         }
      }

      // (2) Write one or more target definition records listing the trial targets in order. "Dummy" tgt definitions 
      // following the last participating target contain all zeros (thus, tgt type will be invalid). If there are 
      // many participating targets, more than one record will be written. When a record is filled, we stream it to
      // file, then reset the record to all zeros before continuing.
      memset(&m_Record, 0, sizeof(CXFILEREC));
      m_Record.idTag[0] = CX_TGTRECORD;
      i = 0; j = 0;
      while(i < m_masterIO.GetNumTrialTargets())
      {
         // retrieve next tgt defn from IPC memory. If it's a random-dot RMVideo tgt, we need to store the actual 
         // seed used if MaestroRTSS randomly chose it.
         m_masterIO.GetTrialTarget(i, m_Record.u.tgts[j].def);
         if(m_Record.u.tgts[j].def.wType == CX_RMVTARG)
         {
            PRMVTGTDEF pRMV = &(m_Record.u.tgts[j].def.u.rmv);
            if((pRMV->iType == RMV_RANDOMDOTS || pRMV->iType == RMV_FLOWFIELD) && (pRMV->iSeed == 0))
               pRMV->iSeed = m_iRMVSeed[i];
         }

         ++i; ++j;

         // write a full target defn record, or the last (possibly) partial tgt defn record
         if(j==CX_RECORDTARGETS || i==m_masterIO.GetNumTrialTargets()) 
         {
            if(!m_writer.Write((PVOID) &m_Record))
            {
               m_writer.Close(FALSE);
               return(FALSE);
            }
            memset(&m_Record, 0, sizeof(CXFILEREC));
            m_Record.idTag[0] = CX_TGTRECORD;
            j = 0;
         }
      }
      
      // (3) Write one or more trial-code records listing the codes that define the trial about to be executed. The 
      // last partial record is padded with zeros after the last valid trial code is stored.
      m_Record.idTag[0] = CX_TRIALCODERECORD;
      i = 0;
      int nCodes = m_masterIO.GetNumTrialCodes();
      while(i < nCodes)
      {
         j = 0;
         while(j < CX_RECORDCODES)
         {
            if(i >= nCodes) m_Record.u.iData[j++] = 0;
            else m_Record.u.tc[j++] = m_masterIO.GetTrialCode(i++);
         }

         if(!m_writer.Write((PVOID) &m_Record))
         {
            m_writer.Close(FALSE);
            return(FALSE);
         }
      }
   }

   // prepare each of the records that will be used to stream recorded analog and digital data on the fly
   ::memset((PVOID) &m_Record, 0, 8); 
   m_Record.idTag[0] = CX_AIRECORD;
   ::memset((PVOID) &m_SpikeRecord, 0, 8);
   m_SpikeRecord.idTag[0] = CX_SPIKEWAVERECORD;
   ::memset((PVOID) &m_Evt0Record, 0, 8);
   m_Evt0Record.idTag[0] = CX_EVENT0RECORD;
   ::memset((PVOID) &m_Evt1Record, 0, 8);
   m_Evt1Record.idTag[0] = CX_EVENT1RECORD;
   ::memset((PVOID) &m_OtherEvtRecord, 0, 8);
   m_OtherEvtRecord.idTag[0] = CX_OTHEREVENTRECORD;

   // reset all relevant bookkeeping variables
   m_nSlowBytes = 0; 
   m_nFastBytes = 0;
   m_nEvent0 = 0;
   m_nEvent1 = 0;
   m_nOther = 0;
   m_nLastEvt0Time = 0;
   m_nLastEvt1Time = 0;
   for(i = 0; i < CX_AIO_MAXN+1; i++) m_shLastComp[i] = 0;

   return(TRUE);
}

BOOL RTFCNDCL CCxDriver::CloseStream( BOOL bSave )
{
   int i;

   if( bSave && (m_nSlowBytes > 0) )                              // fill partial "analog slow data" record with zeroes
   {                                                              // and queue it to the file writer; add size of
      for( i = m_nSlowBytes; i < CX_RECORDBYTES; i++ )            // partial record to get total compressed byte count,
         m_Record.u.byteData[i] = 0;                              // which is saved in data file header record
      m_Header.nBytesCompressed += m_nSlowBytes;
      bSave = m_writer.Write( (PVOID) &m_Record );                // (if a file IO error occurs, there's no use trying
   }                                                              // further)

   if( bSave && (m_nFastBytes > 0) )                              // similarly for partial "analog fast data" record
   {
      for( i = m_nFastBytes; i < CX_RECORDBYTES; i++ )
         m_SpikeRecord.u.byteData[i] = 0;
      m_Header.nSpikeBytesCompressed += m_nFastBytes;
      bSave = m_writer.Write( (PVOID) &m_SpikeRecord );
   }

   if( bSave && (m_nEvent0 > 0) )                                 // fill partial "event0" record with 0x7FFFFFFF (an
   {                                                              // unlikely event interval!) and queue it
      for( i = m_nEvent0; i < CX_RECORDINTS; i++ )
         m_Evt0Record.u.iData[i] = 0x07FFFFFFF;
      bSave = m_writer.Write( (PVOID) &m_Evt0Record );
   }

   if( bSave && (m_nEvent1 > 0) )                                 // similarly for partial "event1" record
   {
      for( i = m_nEvent1; i < CX_RECORDINTS; i++ )
         m_Evt1Record.u.iData[i] = 0x07FFFFFFF;
      bSave = m_writer.Write( (PVOID) &m_Evt1Record );
   }

   if( bSave && (m_nOther > 0) )                                  // fill partial "other event" record with the pairs
   {                                                              // (0, 0x7FFFFFFF) and queue it
      for( i = m_nOther; i < CX_RECORDINTS-1; i+=2 )
      {
         m_OtherEvtRecord.u.iData[i] = 0;
         m_OtherEvtRecord.u.iData[i+1] = 0x07FFFFFFF;
      }
      bSave = m_writer.Write( (PVOID) &m_OtherEvtRecord );
   }

   m_Header.nchar = short(m_Header.nBytesCompressed);             // set OBSOLETE short-valued fields for backwards
   m_Header.npdig = short(m_Header.nScansSaved);                  // compatibility; valid only if # <= 32767!!

   if( bSave ) m_writer.Write( (PVOID) &m_Header, 0 );            // rewrite final version of header record (at
                                                                  // beginning of file!)

   return( m_writer.Close( bSave ) );                             // close file writer to flush remaining records
}

BOOL RTFCNDCL CCxDriver::StreamAnalogData()
{
   int i, j, k;
   short shTemp;

   int nSlowScans = m_vbFrameLag ? 2 : 1;                         // compress & store 1 or 2 scan sets from slow data
   for( k = 0; k < nSlowScans; k++ )                              // buf -- depends on whether we're lagging behind DAQ
   {
      short* pshBuf = &(m_shSlowBuf[k*CX_AIO_MAXN]);              //    start of 1st or 2nd scan set in slow data buf
      for( i = 0; i < m_nSavedCh; i++ )                           //    save selected AI channel samples for the
      {                                                           //    current tick, COMPRESSING as we go:
         j = m_iChannels[i];                                      //    next AI channel to be saved
         shTemp = pshBuf[j];                                      //    current sample from that AI channel
         shTemp -= m_shLastComp[j];                               //    save *difference* from prev sample
         m_shLastComp[j] += shTemp;                               //    save curr sample for next iteration

         if( cMath::abs( shTemp ) < 64 )                          //    -63..63 ==> 0x01..0x7F: can save as a single
         {                                                        //    byte, with bit7 = 0 always!
            shTemp += 64;
            m_Record.u.byteData[m_nSlowBytes++] = (char) shTemp;  //    save compressed byte in approp data record;
         }
         else                                                     //    -2048..-64 ==> 0x8800..0x8FC0, and 64..2047 ==>
         {                                                        //    0x9140..0x97FF. Two bytes saved, with high byte
            shTemp += 4096;                                       //    first. high byte ALWAYS has bit7 = 1!!  we save
            shTemp |= 0x8000;                                     //    each byte to data record, writing rec when full
            m_Record.u.byteData[m_nSlowBytes++] = (char) ((shTemp >> 8) & 0x00FF);
            if( m_nSlowBytes == CX_RECORDBYTES )                  //    if record is full, write it & start again; note
            {                                                     //    that we update count of #bytes compressed that
               if( !m_writer.Write( (PVOID) &m_Record ) )         //    is maintained in the data file header record!
                  return( FALSE );
               m_nSlowBytes = 0;
               m_Header.nBytesCompressed += CX_RECORDBYTES;
            }
            m_Record.u.byteData[m_nSlowBytes++] = (char) (shTemp & 0x00FF);
         }
         if( m_nSlowBytes == CX_RECORDBYTES )                     //    if record is full, write it & start again;
         {                                                        //    again update count of #bytes compressed...
            if( !m_writer.Write( (PVOID) &m_Record ) )
               return( FALSE );
            m_nSlowBytes = 0;
            m_Header.nBytesCompressed += CX_RECORDBYTES;
         }
      }
   }
   m_Header.nScansSaved += nSlowScans;                            // update # of slow scans saved thus far, maintained
                                                                  // in field within data file header record!

   for( i = 0; i < m_nFast; i++ )                                 // store new samples from fast data stream: compress
   {                                                              // in the same manner, and write each record when it
      shTemp = m_shFastBuf[i] - m_shLastComp[CX_AIO_MAXN];        // becomes full... NOTE that we also update a count
      m_shLastComp[CX_AIO_MAXN] = m_shFastBuf[i];                 // field in header record that reflects total # of
      if( cMath::abs( shTemp ) < 64 )                             // "fast data" bytes compressed!
      {
         shTemp += 64;
         m_SpikeRecord.u.byteData[m_nFastBytes++] = (char) shTemp;
      }
      else
      {
         shTemp += 4096;
         shTemp |= 0x8000;
         m_SpikeRecord.u.byteData[m_nFastBytes++] = (char) ((shTemp >> 8) & 0x00FF);
         if( m_nFastBytes == CX_RECORDBYTES )
         {
            if( !m_writer.Write( (PVOID) &m_SpikeRecord ) ) return( FALSE );
            m_nFastBytes = 0;
            m_Header.nSpikeBytesCompressed += CX_RECORDBYTES;
         }
         m_SpikeRecord.u.byteData[m_nFastBytes++] = (char) (shTemp & 0x00FF);
      }
      if( m_nFastBytes == CX_RECORDBYTES )
      {
         if( !m_writer.Write( (PVOID) &m_SpikeRecord ) ) return( FALSE );
         m_nFastBytes = 0;
         m_Header.nSpikeBytesCompressed += CX_RECORDBYTES;
      }
   }
   m_nFast = 0;                                                   // we've emptied the fast data buffer

   return( TRUE );
}

BOOL RTFCNDCL CCxDriver::StreamEventData()
{
   int iEvtTime;
   int iEvtMask;

   BOOL bOk = TRUE;
   for( int i = 0; bOk && (i < m_nEvents); i++ )
   {
      iEvtTime = (int) m_evtTimes[i];
      iEvtMask = (int) m_events[i];
      if( iEvtMask & 0x0001 )
      {
         m_Evt0Record.u.iData[m_nEvent0++] = iEvtTime - m_nLastEvt0Time;
         m_nLastEvt0Time = iEvtTime;
         if( m_nEvent0 == CX_RECORDINTS )
         {
            bOk = m_writer.Write( (PVOID) &m_Evt0Record );
            m_nEvent0 = 0;
         }
      }
      else if( iEvtMask & 0x0002 )
      {
         m_Evt1Record.u.iData[m_nEvent1++] = iEvtTime - m_nLastEvt1Time;
         m_nLastEvt1Time = iEvtTime;
         if( m_nEvent1 == CX_RECORDINTS )
         {
            bOk = m_writer.Write( (PVOID) &m_Evt1Record );
            m_nEvent1 = 0;
         }
      }
      else if( iEvtMask & (~0xFFFF0003) )
      {
         m_OtherEvtRecord.u.iData[m_nOther++] = iEvtMask;
         m_OtherEvtRecord.u.iData[m_nOther++] = iEvtTime;
         if( m_nOther == CX_RECORDINTS )
         {
            bOk = m_writer.Write( (PVOID) &m_OtherEvtRecord );
            m_nOther = 0;
         }
      }
   }

   return( bOk );
}

/**
 Stream a "blink start" or "blink end" event to the recorded data file. Applicable only when the Eyelink tracker is 
 used to monitor eye position.
 @param isStart TRUE for blink start, FALSE for blink end event.
 @param tCurr The current elapsed time during a data recording, in milliseconds.
 @return TRUE if successful. Fails only if the "other event" buffer is filled but is unsuccessfully streamed to file.
 */
BOOL RTFCNDCL CCxDriver::StreamEyelinkBlinkEvent(BOOL isStart, int tCurr)
{
   // save the blink event in buffer. Write buffer to file if full. Note that time is in ms, NOT 10us ticks.
   m_OtherEvtRecord.u.iData[m_nOther++] = (int) (isStart ? CX_EL_BLINKSTARTMASK : CX_EL_BLINKENDMASK);
   m_OtherEvtRecord.u.iData[m_nOther++] = tCurr;

   BOOL bOk = TRUE;
   if(m_nOther == CX_RECORDINTS)
   {
      bOk = m_writer.Write( (PVOID) &m_OtherEvtRecord );
      m_nOther = 0;
   }

   return(bOk);
}

/**
 ConfigureAISeq, StartAISeq 

 Configure/start the prototypical MaestroRTSS AI data acquisition sequence (most configuration details are handled by
 the CCxAnalogIn device object):
    -- Sample all available AI channels in sequence at the current AI scan interval. The channels are sampled as rapidly
 as possible at the start of the scan epoch. This constitutes the "slow" data stream.
    -- Optionally sample the dedicated SPIKECHANNEL at 25KHz -- this "fast" data stream provides a high-resolution
 recording of the spike waveform.
    -- Generate an interrupt at the start of each scan interval.

 Certain runtime state variable are also reset in this method: zero the slow & fast data buffers used to unload data 
 from the AI device on a scan-by-scan basis; reset "tick" counters that keep track of the # of AI scans that have been 
 unloaded by the runtime engine and the # of AI scans that have actually elapsed since the start of the AI sequence; an 
 "AI interrupt pending" flag; and a flag set whenever the runtime engine -- regardless of the current MaestroRTSS 
 operational mode -- falls at least one full scan (or "frame") behind the AI timeline.

 The method StartAISeq() resets a CElapsedTime object dedicated to the detection of long ISR latencies that could break 
 the expected timeline of the MaestroRTSS experimental protocol, then starts the AI sequence.

 @param bSpikeCh If TRUE, configure AI device to record "fast" data [default = FALSE].
 @return TRUE if successful, FALSE otherwise.
*/
BOOL RTFCNDCL CCxDriver::ConfigureAISeq( BOOL bSpikeCh /* =FALSE */ )
{
   // configure the AI sequence. This will reset an AI operation that had been in progress
   BOOL bOk = m_DevMgr.GetAI()->Configure(CX_AIO_MAXN, m_viScanInterval * 1000, bSpikeCh ? SPIKECHANNEL : -1, TRUE);
   
   // reset runtime variables associated with the AI sequence and zero all samples in the slow and fast data buffers
   m_vbInterruptPending = FALSE; 
   m_vbFrameLag = FALSE;
   m_viElapsedTicks = 0;
   m_viServicedTicks = 0;
   m_vbDelayedISR = FALSE;
   memset(m_shSlowBuf, 0, CX_AIO_MAXN*2*sizeof(short));
   memset(m_shFastBuf, 0, CX_FASTBFSZ*sizeof(short));
   m_pshLastScan = &(m_shSlowBuf[0]);

   return(bOk);
}

VOID RTFCNDCL CCxDriver::StartAISeq()
{
   m_eTimeISR.Reset();
   (m_DevMgr.GetAI())->Start();
}


/**
 Service the ongoing MaestroRTSS AI data acquisition sequence by unloading up to two full scan's of "slow data" and any 
 accompanying "fast data". The data is stored in dedicated buffers, which should be copied or otherwise used prior to 
 invoking the method again. Relevant runtime variables -- see ConfigureAISeq() -- are also updated.

 NOTES:
 1) The CCxAnalogIn implementation handles all the details of segregating the two data streams.
 2) Call this method with bWait=TRUE only when an AI sequence is actually in progress, and only when at least one
 complete scan's worth of data is pending in the AI FIFO. If bWait=TRUE and the expected number of samples are not in 
 the FIFO, the function will block until this is the case, or a device timeout occurs.

 @param bWait -- If TRUE, wait to unload a full scan's worth (or two) of samples [default=TRUE].
 @return TRUE if successful, FALSE if an AI device error occurred or if bWait was FALSE and a full scan's worth of data 
 was not immediately available.
*/
BOOL RTFCNDCL CCxDriver::UnloadNextAIScan( BOOL bWait /* =TRUE */ )
{
   // is there a lag of at least one full scan? If so, collect two scans of slow data and any accompanying fast data
   m_vbFrameLag = BOOL((m_viElapsedTicks - m_viServicedTicks) > 1);
   int nSlowScans = m_vbFrameLag ? 2 : 1;
   int nSlow = nSlowScans * CX_AIO_MAXN;
   m_nFast = CX_FASTBFSZ;

   // if unloading two scans, the most recent scan is second
   m_pshLastScan = &(m_shSlowBuf[ (m_vbFrameLag ? CX_AIO_MAXN : 0) ]); 
   
   // update #scans unloaded since DAQ start
   m_viServicedTicks += nSlowScans; 

   // actually do the work!
   BOOL bOk = m_DevMgr.GetAI()->Unload(m_shSlowBuf, nSlow, m_shFastBuf, m_nFast, bWait);
   if(bOk && !bWait) bOk = BOOL(nSlow == nSlowScans * CX_AIO_MAXN);
   return(bOk);
}


/**
 Update the list of AI channel #s that should be saved whenever analog data is recorded and saved to file. This method 
 is called to service the CX_SAVECHANS command from MaestroGUI, which may be issued in any operating mode. The channel 
 data is saved in the same order that the channel #s are specified in the CX_SAVECHANS command.

 The integer-valued command data associated with CX_SAVECHANS is iData[0..N], where iData[0]=N is the # of distinct
 channels to save, and iData[1..N] is the ordered list of channel #s.
*/
VOID RTFCNDCL CCxDriver::UpdateAISaveList()
{
   int iCh[CX_AIO_MAXN+1];
   m_masterIO.GetCommandData( &(iCh[0]), NULL, CX_AIO_MAXN+1, 0 );
   if( iCh[0] < 0 ) iCh[0] = 0;

   DWORD dwAck = CX_SAVECHANS;
   if( iCh[0] <= m_masterIO.GetAIChannels() )
   {
      m_nSavedCh = iCh[0];
      for(int i = 0; i < m_nSavedCh; i++ ) m_iChannels[i] = iCh[i+1];
      for(int i = m_nSavedCh; i < CX_AIO_MAXN; i++ ) m_iChannels[i] = -1;
   }
   else
      dwAck = CX_ILLEGALCMD;
   m_masterIO.AckCommand( dwAck, NULL, NULL, 0, 0 );
}


/**
 At the end of a trial, the animal chair could be left far from the centered pos (zero). By design, all Maestro
 protocols assume that the chair starts at zero. This method is designed to bring the chair back to zero fairly rapidly.
 It stops as soon as the chair is within +/- 0.125deg of 0, or ~2 seconds have expired. It does not respond to any 
 commands from MaestroGUI, so use with care.
*/
VOID RTFCNDCL CCxDriver::RestoreChair()
{
   // set suspend duty cycle: 1ms, 70% suspended; save old suspend params for restoring later.
   int iOldOn, iOldOff; 
   m_suspendMgr.ChangeTiming(300, 700, &iOldOn, &iOldOff);

   // zero chair velocity initially
   m_DevMgr.GetAO()->InitChair(); 

   // configure and start the prototypical AI sequence with 1ms scan interval
   m_viScanInterval = 1; 
   ConfigureAISeq(); 
   StartAISeq(); 

   // run until chair pos is restored to zero, or ~2 seconds have elapsed. We wait for each "start of scan" interrupt,
   // read in one scan's worth of data (restarting AI silently if an error occurs), then apply a velocity drive signal
   // to push chair toward zero aggressively. See CCxAnalogOut::SettleChair().
   BOOL bRestored = FALSE;
   CElapsedTime eTime;
   while((eTime.Get() < 2.0e6) && !bRestored) if(m_vbInterruptPending)
   {
      m_vbInterruptPending = FALSE;
      if(!UnloadNextAIScan())
      {
         ConfigureAISeq();
         StartAISeq();
         continue;
      }
      float fChairPos = ((float) m_pshLastScan[HHPOS]) / POS_TOAIRAW; 
      m_DevMgr.GetAO()->SettleChair( fChairPos );
      bRestored = BOOL(0.125f >= cMath::abs( fChairPos )); 
   }

   // reset hardware used; inform user if we could not zero chair; restore suspend mgr to prior state
   m_DevMgr.GetAO()->InitChair(); 
   m_DevMgr.GetAI()->Init();
   if(!bRestored )  m_masterIO.Message("(!!) Unable to restore chair to zero pos. Hardware problem??");

   m_suspendMgr.ChangeTiming(iOldOn, iOldOff);
}


/**
 Send information used to update GUI plot display showing current locations of the eye, a "secondary" eye, the animal 
 chair, current fixation targets #1 and #2, and the "cursor tracking" target (Continuous Mode only).

 During Trial or Continuous Mode, MaestroGUI displays the current positions of the aforementioned loci in an XY 
 coordinate plot. This routine is called frequently (roughly every 1-2ms) to update the plot every EYEANIMATEINTV
 millisecs. It must be called more frequently than every EYEANIMATEINTV ms to ensure that the IPC handshaking involved 
 with a previous plot request is completed before it is time for the next request. The plot update countdown timer, 
 m_viPlotUpdateMS, is decremented only in the AI end-of-scan ISR, so the routine will not have an effect unless an AI 
 acquisition sequence is in progress!

 The primary eye position (HGPOS, VEPOS), secondary eye position (HGPOS2, VEPOS2), and chair position (HHPOS, 0) are 
 taken from the last recorded AI scan vector, while the positions of the fixation and cursor tracking targets are 
 provided as function arguments. To avoid displaying any of the fixation and/or tracking targets, merely position it 
 well off-screen (the GUI plot display is set to +/-25deg in both dimensions).

 @param fp1, fp2 Current positions (x,y) of fixation tgt #1 and #2, in deg subtended at eye.
 @param track Current position of tracking tgt, in deg subtended at eye.
*/
VOID RTFCNDCL CCxDriver::UpdateLoci(const CFPoint& fp1, const CFPoint& fp2, const CFPoint& track)
{
   // if countdown timer has expired, post a plot update request and rearm the countdown timer. All int-valued 
   // coordinates are expressed in units of deg/100. If timer has not expired, merely complete any IPC handshaking 
   // from a previous update request (if necessary).
   if(m_viPlotUpdateMS > 0)  m_masterIO.UpdatePosPlot(NULL);
   else
   {
      m_viPlotUpdateMS = EYEANIMATEINTV;
      POINT ptLoci[CX_NLOCI];
      float f = 100.0f / POS_TOAIRAW;                             // converts raw ADC codes to hundredth-degrees
      ptLoci[CX_EYE].x = (int) (f * ((float) m_pshLastScan[HGPOS]));
      ptLoci[CX_EYE].y = (int) (f * ((float) m_pshLastScan[VEPOS]));
      ptLoci[CX_EYE2].x = (int) (f * ((float) m_pshLastScan[HGPOS2]));
      ptLoci[CX_EYE2].y = (int) (f * ((float) m_pshLastScan[VEPOS2]));

      ptLoci[CX_CHAIRPOS].x = (int) (f * ((float) m_pshLastScan[HHPOS]));
      ptLoci[CX_FIX1].x = (int) (fp1.GetH() * 100.0f);
      ptLoci[CX_FIX1].y = (int) (fp1.GetV() * 100.0f);
      ptLoci[CX_FIX2].x = (int) (fp2.GetH() * 100.0f);
      ptLoci[CX_FIX2].y = (int) (fp2.GetV() * 100.0f);
      ptLoci[CX_TRACK].x = (int) (track.GetH() * 100.0f);
      ptLoci[CX_TRACK].y = (int) (track.GetV() * 100.0f);
      m_masterIO.UpdatePosPlot( &(ptLoci[0]) );
   }
}


/** 
 Update the current settings governing fixation behavior and rewards to the animal. This method is called to service the
 CX_FIXREWSETTINGS command from MaestroGUI, which may be issued in Idle, Trial, or Continuous modes. The data associated 
 with the command specify the new settings.

 All setting values are ASSUMED valid! Note that if any reward pulse length is >= the fixation duration, the reward 
 delivery system could be compromised
*/
VOID RTFCNDCL CCxDriver::UpdateFixRewSettings()
{
   // retrieve new values for settings from the command data buffers, then ack the command
   int iData[6]; 
   float fData[2];
   m_masterIO.GetCommandData(&(iData[0]), &(fData[0]), 6, 2);
   m_masterIO.AckCommand(CX_FIXREWSETTINGS, NULL, NULL, 0, 0, TRUE);

   // replace old setttings with the new values (see description of CX_FIXREWSETTINGS command in CXIPC.H
   m_fixRewSettings.iDur = iData[0];
   m_fixRewSettings.iRewLen1 = iData[1]; 
   m_fixRewSettings.iRewLen2 = iData[2];
   m_fixRewSettings.iWHVR = iData[3];
   m_fixRewSettings.iAudioRewLen = iData[4];
   m_fixRewSettings.bPlayBeep = (iData[5]==0) ? FALSE : TRUE;
   m_fixRewSettings.fPtAccuracy.Set( fData[0], fData[1] );
}


/**
 UpdateVideoDisplaysAndAck, UpdateVideoDisplays 
 
 Update the current display parameters for the RMVideo display, and update the associated display devices 
 accordingly. This method is called to service the CX_SETDISPLAY command from MaestroGUI, which may be issued in Idle, 
 Trial, or Continuous modes (but see NOTE). The int-valued command data, iData[0..14], associated with CX_SETDISPLAY 
 list the new values for the display parameters. The values provided are ASSUMED to be valid!

 NOTE:  Care must be taken when updating RMVideo's display parameters. Any changes in background color, e.g., are
 reflected immediately in the display. Furthermore, if the display geometry changes, certain targets will be rendered 
 incorrectly (because their definition depends upon the display geometry). Thus, CX_SETDISPLAY should only be issued 
 when RMVideo targets are not actively in use!

 NOTE 2: The XYScope is no longer supported a/o Maestro 4.0, and we're removing XYScope-related code a/o Maestro 5.0.
 We're not changing the format of the CX_SETDISPLAY command. The first 7 integer parameters of the command, which are
 for the XYScope, are simply IGNORED.

 @param piParms A 15-element array representing the command data accompanying the CX_SETDISPLAY command.
*/
VOID RTFCNDCL CCxDriver::UpdateVideoDisplaysAndAck()
{
   // get the display parameters, ack the command, then call helper function to actually update the hardware
   int iParams[15];
   m_masterIO.GetCommandData( &(iParams[0]), NULL, 15, 0 );
   m_masterIO.AckCommand(CX_SETDISPLAY, NULL, NULL, 0, 0, TRUE);

   UpdateVideoDisplays( &(iParams[0]) );
}

VOID RTFCNDCL CCxDriver::UpdateVideoDisplays(int* piParms)
{
   // update RMVideo display only if it is available, since we must error-check in this case...
   CCxRMVideo* pRMV = m_DevMgr.GetRMVideo();
   if(pRMV->IsOn()) 
   {
      if(!pRMV->SetGeometry(piParms[7], piParms[8], piParms[9]))
      {
         ::sprintf_s(m_strMsg, "(!!) Problem updating RMVideo display geometry: %s", pRMV->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }
      if(!pRMV->SetBkgColor(piParms[10], piParms[11], piParms[12]))
      {
         ::sprintf_s(m_strMsg, "(!!) Problem updating RMVideo bkg color: %s", pRMV->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }
      if(!pRMV->SetSyncFlashParams(piParms[13], piParms[14]))
      {
         ::sprintf_s(m_strMsg, "(!!) Problem updating RMVideo sync spot flash params: %s", pRMV->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }
   }
}



/**
 Before animating targets on the RMVideo display, we must send each target's definition to RMVideo. This method handles 
 the task for two situations: (1) loading all RMVideo targets participating in a trial prior to starting that trial; or 
 (2) loading any RMVideo targets in Continuous Mode's "active target list". In both cases, the target definitions are 
 obtained from the Maestro IPC interface (CCxMasterIO). Any ongoing animation is automatically halted and any existing
 targets destroyed prior to loading the new targets.

 Note that the ORDER in which targets are loaded onto RMVideo is important. During animation, RMVideo assumes that 
 target motion update vectors are provided for the targets in the same order in which target defs were loaded!

 On the random-number generator seed supplied for the RMV_RANDOMDOTS and RMV_FLOWFIELD targets: If the target definition
 supplied in Maestro IPC has a zero seed, then MaestroRTSS must auto-generate a seed on a per-use basis. This method 
 handles that task, saving the seed values generated in an internal array indexed by the target's ordinal position in 
 the trial target list (Trial Mode) or the active target list (Continuous Mode).

 @return TRUE if successful; FALSE otherwise.
*/
BOOL RTFCNDCL CCxDriver::LoadRMVideoTargets()
{
   // verify that we're in Trial or Continuous mode
   int iOpMode = m_masterIO.GetMode();
   if(iOpMode != CX_TRIALMODE && iOpMode != CX_CONTMODE) return(FALSE);
   BOOL isTrialMode = BOOL(iOpMode==CX_TRIALMODE);

   // make sure RMVideo is available
   CCxRMVideo* pRMVideo = m_DevMgr.GetRMVideo();
   if(!pRMVideo->IsOn()) return(FALSE);

   // stop any ongoing animation and clear animated target list
   if(!pRMVideo->Init())
   {
      ::sprintf_s(m_strMsg, "(!!) Error reinitializing RMVideo: %s", pRMVideo->GetLastDeviceError());
      m_masterIO.Message(m_strMsg);
      return(FALSE);
   }

   // load the definitions of RMVideo targets to be animated
   int nTgs = isTrialMode ? m_masterIO.GetNumTrialTargets() : m_masterIO.GetNumTargets();
   int nRMVTgts = 0;
   BOOL bOk = TRUE;
   for(int i=0; bOk && i<nTgs; i++)
   {
      CXTARGET tgt;
      if(isTrialMode) m_masterIO.GetTrialTarget(i, tgt);
      else m_masterIO.GetTarget(i, tgt);
      if(tgt.wType != CX_RMVTARG) continue;

      // if required, auto-generate seed value for a random-dot target. We store the seed value generated in an
      // internal array so that we can write to the data file later!
      if(tgt.u.rmv.iType == RMV_RANDOMDOTS || tgt.u.rmv.iType == RMV_FLOWFIELD)
      {
         int iSeed = tgt.u.rmv.iSeed;
         if(iSeed == 0)
         {
            iSeed = int(2147483648.0 * m_uniformRNG.Generate());
            if(iSeed == 0) iSeed = 1;
            tgt.u.rmv.iSeed = iSeed;
         }
         m_iRMVSeed[i] = tgt.u.rmv.iSeed;
      }

      ++nRMVTgts;
      bOk = pRMVideo->AddTarget(tgt.u.rmv);
      if(!bOk)
      {
         ::sprintf_s(m_strMsg, "(!!) Problem adding RMVideo target %s : %s", tgt.name, pRMVideo->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }

      // target will start off at (0,0). Init active target info accordingly (Cont mode only).
      if(!isTrialMode)  m_acvTgts[i].posCurr.Zero();
   }

   if(bOk && nRMVTgts > 0)
   {
      bOk = pRMVideo->LoadTargets();
      if(!bOk)
      {
         ::sprintf_s(m_strMsg, "(!!) Problem loading RMVideo targets: %s", pRMVideo->GetLastDeviceError());
         m_masterIO.Message(m_strMsg);
      }
   }

   return(bOk);
}


/**
 If the Eyelink tracker is in use and connected to the Maestro GUI, eye position data is streamed at 1KHz over IPC. 
 This helper method retrieves the LATEST tracker sample from IPC and uses that sample to set the current eye position
 and velocity in m_pshLastScan[HGPOS, VEPOS, HEVEL, VEVEL]. Thus, the tracker data replaces the analog recorded data --
 so this method should be called ONLY when the tracker is in use.

 The Eyelink tracker may monitor one eye (monocular mode) or both (binocular). In monocular mode, the trajectory data
 for the single eye (L or R) is mapped to the channels HEPOS, VEPOS, HEVEL, VEVEL. In binocular mode, the left eye
 data is placed in those channels, and right eye position in HGPOS2, VEPOS2 (there is no HEVEL2 or VEVEL2 channel).

 The Eyelink data stream is transmitted from the tracker's "Host PC" to the Maestro workstation over an Ethernet 
 connection, and Maestro relies on a Win32 worker thread to service that connection and forward the eye position 
 samples to MaestroDRIVER over IPC. As such there is inevitably going to be occasional short delays in the Eyelink
 data stream. When the second argument tCurr is non-negative, it represents the current time in milliseconds during a
 Maestro data recording in progress (during a trial, or when recording data in Continuous mode) -- this is "Maestro
 time", not "Eyelink time". Received Eyelink samples are timestamped in Maestro time because we cannot compare the
 two timelines over long periods; even a 0.1% difference in clock rates is significant over tens of seconds of
 recording time! 
 
 The method maintains the Maestro timestamp of the last retrieved sample so that, when the next sample is available, 
 it can compute the sample-to-sample delay -- again, in "Maestro time". Ideally, since the Eyelink is assumed to run
 at 1KHz at all times, while Maestro's AI runs at 1KHz or 500Hz, the sample-to-sample delay should never exceed 2ms.
 However, delays in the Eyelink data stream will occur -- a new EL sample may not be ready every time this method is
 invoked (which should be once per Maestro AI sample period), in which case the previous sample is supplied again -- 
 a so-called "repeat". The method tracks the maximum and average observed sample-to-sample delay, as well as the number
 of "repeats". These stats can be written to the Maestro data file when recording stops. At the start of a recording, 
 this method must be called with tCurr==0 in order to reset the statistics.

 @param pbBlink [out] True if the eye position coordinates are undefined because the subject is in the middle of an
 eye blink; else false
 @param tCurr Ignored if negative. Otherwise, this is interpreted as the current time in milliseconds during a Maestro 
 data recording in progress. See above.
 @return True if successful; false if tracker is disconnected or idle, if the Eyelink recording session aborted on an 
 error, or if the Eyelink sample-to-sample delay is CX_MAXELSAMPDELAY milliseconds or more. Note that the method does 
 not check sample-to-sample delay if tCurr is negative.
 */
BOOL RTFCNDCL CCxDriver::UnloadEyelinkSample(BOOL* pbBlink, int tCurr)
{
   // we always flush the Eyelink's sample buffer to get the latest sample. This is important when we're actively
   // recording in Maestro and the Eyelink data stream has fallen behind.
   ELSAMP sNow;
   int res = m_masterIO.GetNextEyelinkSample(sNow, TRUE);
   if(res < 0) return(FALSE);

   int type = m_masterIO.GetEyelinkRecordType();
   int i = (type==EL_BINOCULAR || type==EL_MONO_LEFT) ? EL_LEFT : EL_RIGHT;

   m_pshLastScan[HGPOS] = (short) cMath::rangeLimit(sNow.pos[i].fx * POS_TOAIRAW, -2048, 2047);
   m_pshLastScan[VEPOS] = (short) cMath::rangeLimit(sNow.pos[i].fy * POS_TOAIRAW, -2048, 2047);
   m_pshLastScan[HEVEL] = (short) cMath::rangeLimit(sNow.vel[i].fx * VEL_TOAIRAW, -2048, 2047);
   m_pshLastScan[VEVEL] = (short) cMath::rangeLimit(sNow.vel[i].fy * VEL_TOAIRAW, -2048, 2047);

   *pbBlink = !sNow.gotEye[i];

   if(type == EL_BINOCULAR)
   {
      m_pshLastScan[HGPOS2] = (short) cMath::rangeLimit(sNow.pos[EL_RIGHT].fx * POS_TOAIRAW, -2048, 2047);
      m_pshLastScan[VEPOS2] = (short) cMath::rangeLimit(sNow.pos[EL_RIGHT].fy * POS_TOAIRAW, -2048, 2047);
   }

   if(tCurr == 0)
   {
      m_tsLastELSample = 0;
      m_nELSamples = 0;     // note: we don't count the first sample during a data recording!
      m_accumELSampDelay = 0;
      m_maxELSampDelay = 0;
      m_nELRepeats = 0;
   }
   else if(tCurr > 0)
   {
      if(res == 0) 
         ++m_nELRepeats;
      else
      {
         ++m_nELSamples;
         int delay = tCurr - m_tsLastELSample;
         m_tsLastELSample = tCurr;
         m_accumELSampDelay += delay;
         if(delay > m_maxELSampDelay) m_maxELSampDelay = delay;
         if(delay >= CX_MAXELSAMPDELAY) return(FALSE);
      }
   }

   return(TRUE);
}

/**
 Helper method invoked to synchronize the start of a Maestro data recording to the receipt of an eye data sample from 
 the Eyelink tracker. It flushes the Eyelink sample buffer, then sleeps in 100us bursts while checking for the arrival 
 of the next Eyelink sample, returning as soon as that sample is retrieved.

 This only permits a rough synchronization, because Eyelink samples are retrieved on the Win32 side over an Ethernet
 connection with the tracker.

 @return True if successful; false if an Eyelink error occurred, or no sample was received within ~10ms.
*/
BOOL RTFCNDCL CCxDriver::SyncWithEyelink()
{
   ELSAMP sNow;
   int res = m_masterIO.GetNextEyelinkSample(sNow, TRUE);
   if(res < 0) return(FALSE);

   // poll for next sample every 100us over the next ~10ms
   LARGE_INTEGER i64Sleep;
   i64Sleep.QuadPart = (LONGLONG) 1000;
   for(int i=0; i<100; i++)
   {
      ::RtSleepFt(&i64Sleep);
      res = m_masterIO.GetNextEyelinkSample(sNow, FALSE);
      if(res < 0) return(FALSE);
      if(res > 0) return(TRUE);
   }

   return(FALSE);
}
