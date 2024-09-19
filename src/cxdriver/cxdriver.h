//======================================================================================================================
// cxdriver.h : Declaration of MaestroRTSS's application object, CCxDriver.
//======================================================================================================================

#if !defined(CXDRIVER_H__INCLUDED_)
#define CXDRIVER_H__INCLUDED_

#include "cxmasterio.h"                // CCxMasterIO, encapsulating IPC interface with Maestro
#include "suspend.h"                   // CRtSuspendMgr, implements starvation mgt using RTX timer threads
#include "cxperthelper.h"              // CCxPertHelper -- manages perturbations of trial target trajectories

#include "cxdevicemgr.h"               // CCxDeviceMgr -- the device manager, including all device function interfaces

#include "cxfilefmt.h"                 // defines format for various Maestro data file records, incl "header"
#include "cxfilewriter.h"              // CCxFileWriter, for streaming data records to file on the fly in ContMode

#include "util.h"                      // general utility classes


//=====================================================================================================================
// AI channel assignements
//=====================================================================================================================
// This group of defines should become part of the CCxAnalogIn interface eventually.....

#define HGPOS                       0                          //    ch0 --> horizontal gaze (head-eye) position
#define VEPOS                       1                          //    ch1 --> vertical eye position
#define HEVEL                       2                          //    ch2 --> horizontal eye velocity
#define VEVEL                       3                          //    ch3 --> vertical eye velocity
#define HTPOS                       4                          //    ch4 --> feedback horiz pos of FIBER1 (OBSOLETE)
#define VTPOS                       5                          //    ch5 --> feedback verti pos of FIBER1 (OBSOLETE)
#define HHVEL                       6                          //    ch6 --> horizontal head velocity
#define HHPOS                       7                          //    ch7 --> horizontal head position (= chair pos!)
#define HDVEL                       8                          //    ch8 --> horizontal eye velocity, special filter
#define HTPOS2                      9                          //    ch9 --> feedback horiz pos of FIBER2 (OBSOLETE)
#define VTPOS2                      10                         //    ch10--> feedback verti pos of FIBER2 (OBSOLETE)
#define VEPOS2                      11                         //    ch11--> vert eye pos for "second" eye
#define HGPOS2                      14                         //    ch14--> horiz gaze pos for "second" eye
#define SPIKECHANNEL                15                         //    ch15--> for 25KHz sampling of spike waveform


//=====================================================================================================================
// Sizes of various buffers, most of which are allocated from nonpaged memory at startup. No large buffers as of v3.0!!
//=====================================================================================================================
#define EVENTBUFSZ                  300                        // buffer size for digital events during a single tick
#define CX_FASTBFSZ                 200                        // buffer size for 5ms worth of 25KHz channel data

#define MAXVSTABWINLEN              20                         // max len of sliding window avg used to smooth VStab

// bit flags applicable only to CCxDriver:CTrialSeg:tgtFlags, which also holds VStab-related flags in bits 0-3
#define TF_TGTON ((WORD) (1<<7))       // set if target is ON; unset if OFF
#define TF_TGTREL ((WORD) (1<<6))      // set if target positioning is RELative; unset if ABSolute

// maximum allowed Eyelink sample delay, in milliseconds (WRT Maestro's recorded timeline)
#define CX_MAXELSAMPDELAY 10

class CCxDriver
{
private:
   static const int EYEANIMATEINTV = 30;           // update interval for the eye-target position plot, in ms
   static const int FIXCHKINTV_CONT = 30;          // interval between fixation checks in ContMode, in ms
   static const int GRACEPERIOD_CONT = 90;         // fixation grace period in ContMode, in ms

   static LPCTSTR       WORKING_MUTEX;             // unique name assigned to mutex held by driver thrd while alive
   static const int     WORKER_PRIORITY;           // RTX priority assigned to entry thread and primary worker thread
   static const int     FILEWRITER_PRIORITY;       // RTX priority assigned to file writer thread

   static const float   POS_TOAIRAW;               // converts pos in deg,vel in deg/s to raw b2sAIvolts for comparison
   static const float   VEL_TOAIRAW;               // with pos & vel signals recorded by the CNTRLX AI device

   static const int     TRIALSCANINTVUS;           // TrialMode scan interval, in microsecs
   static const int     CONTSCANINTVUS;            // ContMode scan interval, in microsecs
   static const int     SPIKESAMPINTVUS;           // sample interval for high-res spike trace recording, in microsecs

   static const int     DEF_XYFRAME;               // the default XY refresh period in ContMode (ms)

   static const double  MIN_MARKERINTVUS;          // minimum spread between marker pulses triggered on any DO line
   static const DWORD   RECORDMARKER_MASK;         // record "start" and "stop" pulses are triggered on this dedicated
                                                   // line in timer's DO port

   static const char    START_CHARCODE;            // 8bit ASCII char code: trial or Cont-mode record about to begin
   static const char    STOP_CHARCODE;             // trial or Cont-mode record stopped (always sent)
   static const char    ABORT_CHARCODE;            // trial or Cont-mode record aborted for reason other than fix break
   static const char    LOSTFIX_CHARCODE;          // trial aborted on fixation break (Trial mode only)
   static const char    NOFILE_CHARCODE;           // in place of file name, indicates trial intentionally not saved
   static const char    DATASAVED_CHARCODE;        // trial or Cont-mode data file successfully saved


   // the CURRENT trajectory state of a target during trial runtime. Trajectories are computed on the fly as the trial
   // proceeds, and all relevant trajectory variables in this structure are updated on every trial tick as needed.
   struct CTrialTraj 
   {
      WORD     wType;                              //    target type - for quick reference
      int      iSubType;                           //    [CX_RMVTARG or CX_XYTARG only] - target subtype
      int      iFlags;                             //    [CX_RMVTARG only] - target flags
      
      CFPoint  pos;                                //    tgt position at start of current tick, in deg subtended at eye
      CFPoint  vel;                                //    tgt velocity during current and previous ticks, in deg/sec.
      CFPoint  prevVel;
      CFPoint  pertVelDelta;                       //    net offset due to any perturbations acting on tgt vel during
                                                   //    current tick, in deg/sec
      CFPoint  acc;                                //    tgt accel during current and previous ticks, in deg/sec^2.
      CFPoint  prevAcc;

      CFPoint  prevPos;                            //    tgt pos at start of previous tick, in degrees
      CFPoint  patVel;                             //    tgt pattern velocity during current and previous ticks, in
      CFPoint  prevPatVel;                         //       deg/sec
      CFPoint  pertPatVelDelta;                    //    net offset due to any perturbations acting on tgt pattern vel
                                                   //    during current tick, in deg/sec
      CFPoint  patAcc;                             //    tgt pattern accel during current & prev ticks, in deg/sec^2.
      CFPoint  prevPatAcc;

      CFPoint  ptPosWin;                           //    for accumulating tgt window/pattern motion of interleaved tgts
      CFPoint  ptPosPat;                           //       outside of tgt's interleave slot
      float    remDotLife;                         //    dot life "remainder" for FCDOTLIFE tgt when dotlife units are
                                                   //       in 0.01deg travelled.  units = 0.01deg/tick.
      int      iUpdatePos;                         //    ordinal pos of this tgt in the set of XY tgts partic in trial
      int      iILSlot;                            //    if interleaving, tgt is updated in this interleave slot #
      BOOL     bIsOn;                              //    if TRUE, target is currently turned ON
      BOOL     bIsMoving;                          //    if TRUE, then tgt window moves at some point during trial; for
                                                   //       RECTANNU & SURROUND, flag is set if dot pattern moves --
                                                   //       this helps us pick the best hardware implementation of tgt!
      BOOL     bIsDotLifeInMS;                     //    [FCDOTLIFE only] TRUE if dot life units are millisecs

      BOOL     bIsOnForSearch;                     //    TRUE iff target is ON during special seg of "searchTask" op
      
      // for RMVideo targets, 'pos' actually represents the target's position two video frames AFTER the current trial
      // time, because we must send the trajectory data for frame N+2 at the beginning of frame N. Since target pos is
      // needed for fixation checks, GUI display, and other uses, we need to know the current displayed position of each
      // RMVideo target. These variables hold target position for current frame N and the following two frames.
      CFPoint  posRMVCurr;
      CFPoint  posRMVNext[2];

      // if an RMVideo target is a designated fixation target, we need to keep track of the CURRENT target velocity. Since
      // vel represents the target velocity 2 video frames in the future, we use this variable instead. By convention, the
      // velocity of an RMVideo target for the CURRENT displayed frame is defined as the difference in its current position
      // and its position in the preceding frame, divided by the RMVideo frame period in seconds
      CFPoint  velRMVCurr;
   };

   struct CTrialSeg                                // selected state variables that can change from segment to segment
   {                                               // during a trial:
      int      tStart;                             //    segment's start time in ticks (msec)
      int      iXYUpdIntv;                         //    the XY scope frame update interval (ms) during this segment
      int      iPulseOut;                          //    if >= 0, pulse specified timer DOUT line at start of this seg
      CFPoint  fpFixAcc;                           //    H,V fix accuracy during this seg (in deg subtended at eye)
      int      tGrace;                             //    grace time (msec) for segment.  fixation is not enforced until
                                                   //       trial time >= this time.

      int      iCurrFix1, iCurrFix2;               //    current fix tgts (index into trial targ map); -1 = not used

      // if TRUE, a white spot is flashed in TL corner of RMVideo screen to mark the start of the first video frame
      // drawn at the start of this trial segment. This is intended to drive a photodiode circuit which delivers a
      // TTL pulse on detecting the flash. The pulse can then be timestamped to nail down when the segment really 
      // started on the RMVideo display!
      BOOL     bTrigRMVSyncFlash; 

      BOOL     bCheckResp;                         //    if TRUE, subject's response is checked during this seg
      BOOL     bRewEna;                            //    if TRUE, mid-trial rewards are enabled during this seg
      int      iChOk;                              //    look for correct response on this AI channel #
      int      iChWrong;                           //    look for incorrect response on this AI channel #

                                                   //    per-target trajectory variables for this segment:
      WORD     tgtFlags[MAX_TRIALTARGS];           //    flags: TF_TGTON, TF_TGTREL; VSTAB_* defined in cxtrialcodes.h
      CFPoint  tgtPos[MAX_TRIALTARGS];             //    target window position in deg
      CFPoint  tgtVel[MAX_TRIALTARGS];             //    target window velocity in deg/s
      CFPoint  tgtAcc[MAX_TRIALTARGS];             //    target window acceleration in deg/s^2
      CFPoint  tgtPatVel[MAX_TRIALTARGS];          //    target pattern velocity in deg/s
      CFPoint  tgtPatAcc[MAX_TRIALTARGS];          //    target pattern acceleration in deg/s^2
   };

   struct CActiveTgt                               // update info for "active" tgts in ContMode
   {
      CXTARGET tgtDef;                             //    target definition (for convenient access)
      CFPoint  posCurr;                            //    current window position of tgt
      CFPoint  posNext;                            //    next window position of tgt (becomes curr WHEN tgt turned ON)
      float    fPatSpeed;                          //    pattern speed in deg/sec (if applicable)
      float    fPatDir;                            //    direction of pattern velocity in deg CCW (if applicable)
      float    fRemDotLife;                        //    runtime: dot life remainder for XY scope tgt of relevant type
      BOOL     bOn;                                //    is target turned on?
   };

   struct CFixRewSettings                          // fixation/reward settings
   {
      int      iDur;                               //    fixation duration in ms (for ContMode rewards)
      int      iRewLen1, iRewLen2;                 //    reward pulse lengths 1 & 2 in ms
      int      iWHVR;                              //    random withholding variable ratio (1=withholding disabled)
      int      iAudioRewLen;                       //    audio reward pulse length in ms (0=no audio reward)
      int      iFix1, iFix2, iTrack;               //    fixation and "cursor tracking" tgts in ContMode (index into
                                                   //       active tgt list; -1=none)
      BOOL     bPlayBeep;                          //    if TRUE, play reward indicator "beep" on PC speaker
      CFPoint  fPtAccuracy;                        //    H,V fixation accuracy in deg subtended at eye
   };

   struct CStimRunInfo                             // definition & runtime control of a ContMode "stimulus run"
   {
      CONTRUN     def;                             //    the run definition

      BOOL        bUsesChair;                      //    TRUE if corresponding stimulus platform is used during run
      BOOL        bUsesXYseq;
      BOOL        bUsesPSGM;

      int         tLastUpdate;                     //    timepoint (within duty cycle) of last trajectory update (ms)
      int         iCycles;                         //    #duty cycles completed (for auto-stop feature)
      BOOL        bSoftStopReq;                    //    if TRUE, stop at end of current duty cycle

      float       fChairVel;                       //    current chair velocity
      float       fExpectedChairPos;               //    current expected pos of chair
      DWORD       dwMarkers;                       //    marker pulses to be delivered on next update

      int         tStartPSGM;                      //    timepoint (within duty cycle) at which SGM seq is started (ms)

                                                   //    XYseq motion control variables:
      PSTIMCHAN   pXYseq;                          //    the one-and-only enabled XYseq stimulus in the current run
      CFPoint     ptVec[MAX_XYSEQVECS+1];          //    precomputed "per-refresh" pos displacement (H,V) in deg for
                                                   //    each possible motion vector; last entry is always (0,0)
      int         iCurrVec[MAXTGTSINXYSEQ];        //    index of current motion vector applied to each XY tgt in seq
      int         tCurrSeg,                        //    curr motion seg began at this time t'=tActual-tStart (ms)
                  iCurrSparseTgt;                  //    index of XY tgt currently moving in a "sparse" XYseq stimulus
      BOOL        bInitialUpdate;                  //    TRUE for first update of XYseq at "t=0" (to init tgt locs)
      BOOL        bXYseqOn;                        //    TRUE while XYseq running (may be off for part of duty cycle)
      BOOL        bSparse;                         //    TRUE for "sparse" seq, FALSE otherwise.
      CRand16     randGen;                         //    pseudo-rand# generator for XYseq
   };


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxMasterIO       m_masterIO;                   // encapsulates communication with MAESTRO
   CRtSuspendMgr     m_suspendMgr;                 // to manage CPU usage by main runtime engine thread

   CCxDeviceMgr      m_DevMgr;                     // hardware device mgr -- access to supported MAESTRODRIVER devices

   int               m_nSavedCh;                   // ordered list of AI chan saved to data files (CX_SAVECHANS cmd)
   int               m_iChannels[CX_AIO_MAXN];     //

   short             m_shSlowBuf[CX_AIO_MAXN*2];   // the 1-2 most recent "slow scans" of all AI chan (in b2sAIvolts)
   short*            m_pshLastScan;                // points to start of most recent scan set within slow data buf
   short             m_shFastBuf[CX_FASTBFSZ];     // most recently collected samples from 25KHz AI ch (spike waveform)
   int               m_nFast;                      // # of valid samples in the fast data buffer

   short             m_shLastComp[CX_AIO_MAXN+1];  // set of analog samples that were last compressed & saved; extra
                                                   // slot is used to compress the "fast" analog data stream

   short m_hgposSlider[MAXVSTABWINLEN];            // sliding window storing last N raw samples of HGPOS, VEPOS. Used
   short m_veposSlider[MAXVSTABWINLEN];            // to smooth out noise in eye pos traces for better VStab. Impl as
                                                   // circular buffers. Trial Mode only.
   
                                                   // critical runtime control variables:
   volatile BOOL     m_vbInterruptPending;         //    TRUE if an ADC interrupt requires processing
   volatile int      m_viElapsedTicks;             //    # of ADC interrupts (ie, # of scans) since AI operation began
   volatile int      m_viScanInterval;             //    current ADC scan interval in milliseconds
   volatile int      m_viPlotUpdateMS;             //    # msec until next update of GUI eye/tgt pos plot
   volatile int      m_viXYUpdateMS;               //    # msec until next XY scope update
   volatile int      m_viFixChkMS;                 //    # msec until next fixation check (ContMode only)
   volatile BOOL     m_vbStimOn;                   //    TRUE if a cont-mode stimulus run is currently executing
   volatile int      m_viStimTicks;                //    # scans elapsed in duty cycle of a cont-mode stimulus run
   volatile int      m_viStimDutyCycle;            //    # scans per duty cycle of a cont-mode stimulus run
   volatile int      m_viServicedTicks;            //    # AI scans unloaded by runtime loop since AI operation started
   volatile BOOL     m_vbFrameLag;                 //    TRUE whenever runtime loop is lagging AI timeline by at least
                                                   //    one full frame (scan); in this case, slow data buffer should
                                                   //    contain two scan's worth of AI samples
   volatile BOOL     m_vbDelayedISR;               //    set TRUE if the ADC ISR latency >= 500us
   CElapsedTime      m_eTimeISR;                   //    elapsed time between ISRs -- to detect long ISR latencies

   int               m_nSlowBytes;                 // #bytes of "slow data stream" filled in current data record
   int               m_nFastBytes;                 // similarly for "fast data stream"

   int               m_nEvents;                    // # of digital events currently stored in these buffers:
   DWORD             m_events[EVENTBUFSZ];         //    event mask for each digital event that occurred
   DWORD             m_evtTimes[EVENTBUFSZ];       //    time of occurrence of each event, in timer ticks (10us)

   CCxPertHelper     m_pertMgr;                    // manages any perturbations that modulate target traj's in a trial

   CTrialTraj        m_traj[MAX_TRIALTARGS];       // used during precomputation of target trajectories for a trial
   CTrialSeg         m_seg[MAX_SEGMENTS];          // segment-based representation of selected trial state variables

   // XYScope target window and pattern pos change and update interval for the current or next display frame
   CFPoint           m_ptXYWindow[MAX_TRIALTARGS]; 
   CFPoint           m_ptXYPattern[MAX_TRIALTARGS]; 
   WORD              m_wXYUpdIntv[MAX_TRIALTARGS]; 
   
   // RMVideo target motion update vectors for current display frame and the next two frames
   RMVTGTVEC         m_RMVUpdVecs[3*MAX_TRIALTARGS];

   BOOL              m_bFixOn;                     // TRUE when fixation checking is ON in ContMode
   CActiveTgt        m_acvTgts[MAX_ACTIVETGTS];    // update info for active targets in ContMode
   CStimRunInfo      m_run;                        // defn & runtime control of a stimulus run in ContMode

   CFixRewSettings   m_fixRewSettings;             // fixation & reward settings

   char              m_strMsg[CXIPC_MSGSZ];        // general use: for constructing messages sent to CNTRLX via IPC
   char              m_string[CX_MAXPATH];         // general use: for handling pathnames, etc.
   
   int               m_iCmdBuf[CX_CMDLEN];         // buffers for retrieving command data, preparing response data
   char              m_cCmdBuf[CX_CDATALEN];       //
   
                                                   // data file records & assoc counters -- for streaming data to file
   CXFILEHDR         m_Header;                     //    header record
   CXFILEREC         m_Record;                     //    compressed analog slow data (also used as a generic data rec)
   CXFILEREC         m_SpikeRecord;                //    compressed analog fast data -- high-res spike waveform
   CXFILEREC         m_Evt0Record;                 //    interevent intervals for events on DI ch 0
   CXFILEREC         m_Evt1Record;                 //    interevent intervals for events on DI ch 1
   CXFILEREC         m_OtherEvtRecord;             //    (event mask, time)-pairs for events on all other DI channels

   int               m_nEvent0;                    //    #integers stored thus far in "event0" record
   int               m_nLastEvt0Time;              //    timestamp of last event on DI0 ("unit 0"), in timer ticks
   int               m_nEvent1;                    //    #integers stored thus far in "event1" record
   int               m_nLastEvt1Time;              //    timestamp of last event on DI1 ("unit 1"), in timer ticks
   int               m_nOther;                     //    #integers stored thus far in other events record

   CCxFileWriter     m_writer;                     // file writer:  writes data file on the fly in ContMode

   CUniformRNG       m_uniformRNG;                 // a uniform RNG generating floating-pt values in (0..1)

   // for random-dot RMVideo targets, this is the actual seed sent for the corresponding target in the loaded target
   // list.  MAESTRODRIVER auto-generates seed value on a per-use basis IF the defn from Maestro has a seed of zero!
   // We need to remember the seeds generated so that we can store them in the data file.
   int               m_iRMVSeed[MAX_TRIALTARGS];

   // elapsed time in microseconds since CXDRIVER started. Used to put a timestamp in data files.
   CElapsedTime      m_eRunTimeUS;

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxDriver( const CCxDriver& src );                    // no copy constructor or assignment operator defined
   CCxDriver& operator=( const CCxDriver& src );

public:
   CCxDriver();
   ~CCxDriver();


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   VOID RTFCNDCL Go();                                   // pseudo entry point -- call this in main()


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
private:
   static DWORD RTFCNDCL RunEntry( PVOID pThisObj )      // static helper method invokes non-static thread procedure!
   {
      return( ((CCxDriver*)pThisObj)->Run() );
   }
   DWORD RTFCNDCL Run();                                 // thread proc for the runtime engine thread

   BOOL RTFCNDCL OpenHardwareResources();                // find and initialize any supported hardware devices
   VOID RTFCNDCL CloseHardwareResources();               // free all hardware resources

   static BOOLEAN RTFCNDCL ServiceAI( PVOID pThisObj );  // ISR for interrupts from the analog input device

   VOID RTFCNDCL RunIdleMode();                          // runtime control in idle mode
   
   // RunIdleMode() helpers that respond to Idle Mode-only commands specific to RMVideo...
   VOID RTFCNDCL GetRMVDisplayModes();                   // CX_RMV_GETMODES command
   VOID RTFCNDCL GetCurrRMVDisplayMode();                // CX_RMV_GETCURRMODE command
   VOID RTFCNDCL SetCurrRMVDisplayMode();                // CX_RMV_SETCURRMODE command
   VOID RTFCNDCL GetRMVMonitorGamma();                   // CX_RMV_GETGAMMA command
   VOID RTFCNDCL SetRMVMonitorGamma();                   // CX_RMV_SETGAMMA command
   VOID RTFCNDCL GetRMVMediaFolders();                   // CX_RMV_GETMDIRS command
   VOID RTFCNDCL GetRMVMediaFiles();                     // CX_RMV_GETMFILES command
   VOID RTFCNDCL GetRMVMediaFileInfo();                  // CX_RMV_GETMFINFO command
   VOID RTFCNDCL DeleteRMVMediaFile();                   // CX_RMV_DELMEDIA command
   VOID RTFCNDCL DownloadRMVMediaFile();                 // CX_RMV_PUTMEDIA command
  

   VOID RTFCNDCL RunTestMode();                          // runtime control in test & calibration mode

   VOID RTFCNDCL RunTrialMode();                         // runtime control between trials in trial mode
   DWORD RTFCNDCL ExecuteSingleTrial();                  // run a trial -- response to CX_TR_START cmd in trial mode

   VOID RTFCNDCL RunContinuousMode();                    // runtime control in continuous mode
   VOID RTFCNDCL StartStimulusRun();                     // initialize runtime control info & start a stimulus run
   VOID RTFCNDCL UpdateStimulusRun( int tCurrent );      // update trajectory of tgts participating in a stimulus run
   BOOL RTFCNDCL OpenStream( LPCTSTR strPath );          // open file for streaming recorded data during ContMode
   BOOL RTFCNDCL CloseStream( BOOL bSave );              // flush all data remaining in stream buffers and close file
   BOOL RTFCNDCL StreamAnalogData();                     // stream analog slow and fast data to file on the fly
   BOOL RTFCNDCL StreamEventData();                      // stream digital event data to file on the fly

   // stream an Eyelink blink start or end event to file on the fly
   BOOL RTFCNDCL StreamEyelinkBlinkEvent(BOOL isStart, int tCurr);

   BOOL RTFCNDCL ConfigureAISeq(BOOL bSpikeCh = FALSE);  // configure the prototypical AI acquisition sequence
   VOID RTFCNDCL StartAISeq();                           // start the prototypical AI acquisition sequence
   BOOL RTFCNDCL UnloadNextAIScan( BOOL bWait = TRUE );  // unload 1 or 2 slow scans of AI samples, along with any fast
                                                         // data, during a prototypical AI sequence (as cfg'd above)

   VOID RTFCNDCL UpdateAISaveList();                     // update list of AI ch to save to file (CX_SAVECHANS cmd)

   VOID RTFCNDCL RestoreChair();                         // attempt to restore chair to zero pos within 2 secs

   VOID RTFCNDCL UpdateLoci( const CFPoint& fp1,         // update GUI plot displaying selected loci
                             const CFPoint& fp2,
                             const CFPoint& track );

   VOID RTFCNDCL UpdateFixRewSettings();                 // update fixation/reward settings (CX_FIXREWSETTINGS cmd)

   VOID RTFCNDCL UpdateVideoDisplaysAndAck();            // update display params for XY & RM video (CX_SETDISPLAY cmd)
   VOID RTFCNDCL UpdateVideoDisplays(int* piParms);      // alternate version

   BOOL RTFCNDCL LoadRMVideoTargets();                   // load any RMVideo tgts to be animated in Trial or Cont mode
   BOOL RTFCNDCL SendXYScopeParameters_TM();             // load targets onto XY scope device and prepare for animation
   BOOL RTFCNDCL SendXYScopeParameters_CM();             //    sequence in Trial and Cont Modes

   // if Eyelink tracker in use, retrieve latest tracker sample and use it to update eye trajectory (HGPOS, VEPOS, etc)
   BOOL RTFCNDCL UnloadEyelinkSample(BOOL* pbBlink, int tCurr);
   // empty Eyelink sample buffer and return as soon as the next sample is available in the buffer
   BOOL RTFCNDCL SyncWithEyelink();

   // number of Eyelink samples delivered during a Maestro data recording
   int m_nELSamples;
   // timestamp (in ms, "Maestro time") of the last Eyelink sample delivered during a Maestro data recording.
   int m_tsLastELSample;
   // accumulate observed Eyelink inter-sample delay (in ms, "Maestro time") during a data recording.
   long m_accumELSampDelay;
   // maximum observed Eyelink inter-sample delay (in ms, "Maestro time") during a data recording
   int m_maxELSampDelay;
   // #times no Eyelink sample was ready during a Maestro data recording (so previous sample was repeated).
   int m_nELRepeats;
};


#endif   // !defined(CXDRIVER_H__INCLUDED_)



