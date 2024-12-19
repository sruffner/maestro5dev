//=====================================================================================================================
//
// readcxdata.h : Constant and type definitions for MATLAB MEX function readcxdata().
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(READCXDATA_H__INCLUDED_)
#define READCXDATA_H__INCLUDED_


#include "wintypes.h"                                    // some typical Windows typedefs that we need

#include "rmvideo_common.h"                              // RMVideo definitions (file NOT modified for MEX build)
#include "cxfilefmt_mex.h"                               // Maestro/Cntrlx data file fmt (file modified for MEX build)
#include "cxobj_ifc_mex.h"                               // Maestro/Cntrlx object interface (file mod. for MEX build)
#include "cxtrialcodes_mex.h"                            // Maestro/Cntrlx trial codes (file modified for MEX build)


// OBSOLETE trial codes:  CXTRIALCODES.H does not list some OBSOLETE trial codes, which are defined below in case we
// find them in older data files.  For each code set we indicate N=# of TRIALCODE blocks comprising the set.  Most of
// these codes are so old that they no longer appear in maintained source code for Cntrlx, so that I have no idea
// what N is (N=???).  If any of these codes are encountered, we abort any attempt to read the trial codes.  Else,
// use the value of N to skip over the TRIALCODE blocks pertaining to the obsolete code set.
//
#define     ENDFRAME             9                       // N=???
#define     TARGETOFF            15                      // N=???
#define     TARGET_ACTIVATE      17                      // N=???
#define     TARGET_VOPEN         22                      // N=???
#define     PAN_POS              23                      // N=???
#define     PAN_VEL              24                      // N=???
#define     SCROLL_POS           25                      // N=???
#define     SCROLL_VEL           26                      // N=???
#define     VRORIGIN             31                      // N=3
#define     TARGET_ZVEL          32                      // N=2
#define     TARGET_ZPOSREL       33                      // N=2
#define     TARGET_ZPOSABS       34                      // N=2
#define     TARGET_ZACC          35                      // N=2
#define     VRCOORDS             37                      // N=???

// This ancient trial code collides with INSIDE_VACC, introduced in Maestro v2.1.0. For file version >= 9, it 
// is treated as INSIDE_VACC. Otherwise, it is treated as VSYNC_PULSE, in which case we abort reading trial codes.
#define     VSYNC_PULSE          46                      // N=???

// !!!IMPORTANT!!!
// The usage of the TARGET_PERTURB trial code set was revamped entirely in Maestro.  The # of TRIALCODEs in this set is
// unchanged compared to Cntrlx, but the meaning of the codes is very different.  Programs that analyze the trial codes
// must check the data file version # to properly parse this code set.  For file versions >= 2, go by the description
// in CXTRIALCODES.H.  For file versions < 2, use this description:
//
// # define TARGET_PERTURB   20  // apply sinusoidal perturbation velocity waveform to target -- TURNTABLE, FIBER1 &
//                               // XYSCOPE targets only (N=5)
//                               // code1 = target#; time1 = target type (not used)
//                               // code2 = horiz perturbation amp; time2 = vert perturbation amp
//                               // code3 = period of sine wave (ms); time3 = phase delay (deg)
//                               // code4 = #complete cycles in waveform; time4 = DC component
//


#define MAXPATHNAMESZ            1024                 // file pathname size -- this should be more than enough!

#define ACTION_SACCUT            100                  // XWORK action types
#define ACTION_CUTIT             101
#define ACTION_NCHANS            103
#define ACTION_LEVEL1            104
#define ACTION_LEVEL2            105
#define ACTION_MARK              106
#define ACTION_SETMARK1          107
#define ACTION_SETMARK2          108
#define ACTION_RMUNIT            109
#define ACTION_ADDUNIT           110
#define ACTION_RMALL             111
#define ACTION_EDITEVENT         112

#define ACTION_REMOVESORTSPK     113                  // Action codes introduced in JMWork
#define ACTION_ADDSORTSPK        114 
#define ACTION_DEFTAG            115
#define ACTION_DISCARD           116
#define ACTION_ILLEGAL           55                   // in JMWork, used to fill rest of partial action record

// the number of different "sorted-spike train" channels that can be appended to a Maestro data file.
// 06sep2013: This was originally 13, a limit set by XWORK and mapped to data file record tags CX_SPIKESORTREC_FIRST to
// CX_SPIKESORTREC_LAST defined in cx_filefmt_mex.h. As of this date, CX_SPIKESORTREC_LAST was changed so that now
// 50 such channels are supported
// 04jun2021: Increased four-fold by using the second byte in the record tag to specify a "bank number" M in [0..3]
#define NUMSPIKESORTCH           ((CX_SPIKESORTREC_LAST-CX_SPIKESORTREC_FIRST+1) * 4)

#define HARDTARGS                16                   // in Cntrlx data files with version < 2 (pre-Maestro), targets
#define TURNTABLE                0                    // were identified by an ID that contained some indication of
#define REDLED1                  1                    // target type.  Targets w/ IDs listed here are the so-called
#define FIBER1                   2                    // "hard" targets.  The first 5 are used extensively, the rest
#define FIBER2                   3                    // have not been used since 1998.  XY scope and FB video tgts
#define REDLED2                  4                    // have target IDs >= HARDTARGS....  In Maestro data files
#define RENEAR                   5                    // (vers >= 2), these IDs are no longer used.  In trial codes,
#define LENEAR                   6                    // targets are identified by their ordinal pos in the trial
#define REMIDDLE                 7                    // tgt list instead of this tgt ID.  Target definitions are
#define LEMIDDLE                 8                    // stored in CX_TGTRECORDs in the data file; the defns are
#define REFAR                    9                    // stored in the same order in which the targets appear in the
#define LEFAR                    10                   // trial target list.
#define VRE1                     13
#define VRE2                     14
#define VIDEO                    15

#define HGPOS                    0                    // index of AI channel dedicated to horizontal gaze pos
#define VEPOS                    1                    // index of AI channel dedicated to vertical eye pos
#define HEVEL                    2                    // index of AI channel dedicated to horizontal eye velocity
#define VEVEL                    3                    // index of AI channel dedicated to vertical eye velocity
#define HDVEL                    8                    // channel dedicated to specially filtered version of HEVEL
#define POSAIRAW_TODEG           0.025                // multiply AI 12bit DAC sample by this factor to get degrees
#define VELAIRAW_TODEGPERSEC     0.0918892            // multiply AI 12bit DAC sample by this factor to get deg/sec


//=====================================================================================================================
// Contents of a Maestro/Cntrlx data file
//
// This "all-in-one" data structure encapsulates the data file contents.  Once the MEX function readCxData() has
// finished reading in and appropriately parsing the data file into this structure, we translate the data stored here
// into the MATLAB compatible output structure that is returned by the function.
//
//=====================================================================================================================

typedef struct tagCxDataFile
{
   int nRecords;                                // # of records in file (including header)
   CXFILEHDR fileHdr;                           // the data file's header record.  see CXFILEFMT.H for complete desc,
                                                // including complete version history.

      // !!! IMPORTANT !!! ContMode data files recorded prior to version 1 lacked any header whatsoever.  We create
      // a virtual header in this case.  User MUST provide information on the data channels recorded in order to parse
      // the compressed AI data stored in CX_AIRECORDs!!!

   int nAIBytes;                                // buffer for compressed AI data extracted from CX_AIRECORDs.  This is
   int nAIBufSz;                                // the raw data sampled from selected AI channels at 1KHz in TrialMode
   char* pcAIData;                              // and 500Hz in ContMode.

   int nFastBytes;                              // buffer for compressed AI data from dedicated "fast" channel that
   int nFastBufSz;                              // records spike waveform at 25KHz in Trial or Cont modes.  Culled from
   char* pcFastData;                            // CX_SPIKEWAVERECORDs.  [Applies only to data files w/ version>=2.]

   int nSpikes;                                 // occurrence times (ms) of events on timer DI<0>, reserved for
   int nSpikesBufSz;                            // recording spike arrival times.  Culled from CX_EVENT0RECORDs.
   double* pdSpikes;
   long tLastSpike;                             // last spike arrival time in # 10us-ticks. For multi-record proc.
   
   int nEvents;                                 // occurrence times (ms) of events on timer DI<1>, which may record
   int nEventsBufSz;                            // a 2nd spike train or marker pulses.  Culled from CX_EVENT1RECORDs.
   double* pdEvents;
   long tLastEvent;
   
   int nOthers;                                 // occurrence times of events on timer DI<2..15>, in (pulse#, T)-pairs,
   int nOthersBufSz;                            // where pulse# = [2..15] and T = milliseconds since recording started.
   double* pdOthers;                            // Culled from CX_OTHEREVENTRECORDs.

   int nBlinkEvts;                              // start and end times of eye blinks detected when using Eyelink 
   int nBlinksBufSz;                            // tracker, in (startT, endT) pairs. Times in ms since recording 
   double *pdBlinks;                            // started. Culled from CX_OTHEREVENTRECORDs.

   int nSortedSpikes[NUMSPIKESORTCH];           // spike counts in each "sorted spike train" channel found in data file
   int nSortedBufSz[NUMSPIKESORTCH];            // buffer sizes allocated for each channel found
   double* pdSortedSpikes[NUMSPIKESORTCH];      // the allocated buffers for each channel found
   long tLastSortedSpike[NUMSPIKESORTCH];       // the last spike time in a record (if train spans multiple records)
   
   int nEdits;                                  // buffer that holds all XWORK actions culled from ACTIONBUFF records.
   int nEditsBufSz;                             // since an individual "action object" may be stored across two
   int* piEdits;                                // consecutive records, we must read in all such records before
                                                // processing them.

   int nTargets;                                // #target definitions or IDs
   int nTgtsBufSz;                              // size of target defn buffers
   CXFILETGT_V7* pTargets_V7;                   // the target definitions [2 <= data file version <= 7]
   CXFILETGT_V12* pTargets_V12;                 // the target definitions [8 <= data file version <= 12]
   CXFILETGT_V22* pTargets_V22;                 // the target definitions [13 <= data file version <= 22]
   CXFILETGT_V24* pTargets_V24;                 // the target definitions [23 <= data file version <= 24]
   CXFILETGT* pTargets;                         // the target definitions [data file version >= 25]

   int nTrialTgts;                              // old-style target IDs of targets participating in a CNTRLX trial
   int oldTgtIDs[MAX_TRIALTARGS];

                                                // information extracted ONLY from TrialMode data files:
   int nCodes;                                  //    buffer holding the trial codes that define a CNTRLX trial.  we
   int nCodesBufSz;                             //    read in all trial codes from any CX_TRIALCODERECORDs in the file
   TRIALCODE* pCodes;                           //    before attempting to process the codes to calc tgt trajectories.
   int nSections;                               //    the number of "tagged sections" defined on trial [file ver >= 4].
   TRIALSECT sections[MAX_SEGMENTS];            //    tagged section records culled from data file [file ver >= 4].

                                                // this trial info is prepared by processTrialCodes():
   int nSegments;                               //    number of segments in trial
   int segStart[MAX_SEGMENTS];                  //    start time for each segment; units = trial ticks. 
   int fix1[MAX_SEGMENTS];			               //    0-based index of fix tgt #1 for each segment (-1 = "none")
   int fix2[MAX_SEGMENTS];                      //    0-based index of fix tgt #2 for each segment (-1 = "none")
   int tRecordStarted;                          //    trial time at which recording started, in trial ticks.
   int tTrialLen;                               //    total trial length in ticks (NOT just recorded time!)
   BOOL bSkipOccurred;                          //    if TRUE, then trial included a "skip on saccade" op -- in which
                                                //    case the trial times here may be incorrect!

   // [20nov2024] Dropped support for reading CX_STIMRUNRECORDS -- regardless the data file version. READCXDATA was
   // never able to report stimulus run definitions in the output (technical issue with Matlab structs).
   // int nStims;                                  // relevant stimulus run defn found in data file.  only in ContMode
   // int nStimsBufSz;                             // files with version >= 2.  first CXFILESTIM_U obj in buffer contains
   // CXFILESTIM_U* pStimBuf;                      // header params for run, remaining ones are stim channel defns.

} CXFILEDATA, *PCXFILEDATA;


typedef struct tagMark                       // a "tag" annotation as defined by the ACTION_DEFTAG action code group
{
   int      time;                            //    the tag's timestamp, in ms since recording started
   char     label[20];                       //    null-terminated label for the tag
} TAGMARK, *PTAGMARK;

//
// constants defining the fields in the output structure returned by MEX function readcxdata()
//

const char* outputFields[] =  // the MATLAB-compatible output structure returned by MEX function readcxdata():
{
   "trialname",               //    name of trial as it appears in data file; ignore for ContMode data files
   "key",                     //    contents of data file header record (see below)
   "data",                    //    sampled data from analog input channels (1KHz in TrialMode, 500Hz in ContMode)
   "spikes",                  //    event arrival times on DI<0> relative to recording start time, in milliseconds
   "events",                  //    event arrival times on DI<1>, in milliseconds
   "other",                   //    Mx2 matrix: (pulse#, arrival time in millisecs) for pulses recorded on DI<2..15>
   "blinks",                  //    Mx2 matrix: blink epochs (start, end in milliseconds) from Eyelink tracker
   "mark1",                   //    list of ACTION_SETMARK1 actions taken on data using XWORK analysis program
   "mark2",                   //    list of ACTION_SETMARK2 actions taken on data using XWORK
   "cut",                     //    all saccade cuts performed on data using XWORK
   "marks",                   //    list of ACTION_MARK actions taken on data using XWORK
   "tags",                    //    (as of Sep 2010) list of labelled tags culled from ACTION_DEFTAG actions in file
   "discard",                 //    (as of Sep 2010) nonzero if data file contains a ACTION_DISCARD action
   "marked",                  //    nonzero if the data was modified by at least one ACTION_MARK action
   "targets",                 //    trial target trajectories -- TrialMode only (see below)

                              // available for Maestro data files with version >= 2...
   "tgtdefns",                //    defining parameters of relevant targets (see below)
   // "stimulusrun",          //    [REMOVED 20nov2024] ContMode stimulus run defined when recording started
   "spikewave",               //    sampled data from AI<15>, dedicated to 25KHz recording of spike waveform

   "sortedSpikes",            //    "sorted spike trains", a 1x200 cell array containing sorted spike trains culled
                              //    from high-resolution spike waveforms recorded in Maestro or on Plexon. These are
                              //    appended to the original Maestro data file by analysis programs like XWork and
                              //    JMWork, or by other Matlab utilities that rely on EDITCXDATA to augment the data
                              //    file. If no spike-sorting was performed on a given channel, the corresponding cell
                              //    will hold an empty matrix; otherwise it will hold a double array of the spike
                              //    arrival times, formatted in the same way as the "spikes" field described above.
                              //       Originally, there was support for only 13 distinct spike trains. However, as
                              //    of Sep 2013, JMWork and READ/EDITCXDATA were revised to support 50.
                              //       As of Jun 2021, increased number of distinct spike train channels to 200.
                              //       JMWork introduces actions that let user manually add or remove individual spikes
                              //    from a sorted-spike train channel. The sorted spike trains reported are what 
                              //    remains after such "manual spike-edits" are applied!

                              // available for Maestro data files with version >= 4...
   "tagSections",             //    tagged sections defined on a Maestro trial (see below).


                              // available for Maestro data files with version >= 10...
   // "psgm",                 //    [REMOVED 20nov2024] parameter of a PSGM sequence delivered during a Maestro trial
   
   "trialInfo",               // additional info about a trial: #segments, seg start times, etcetera
   
   "xynoisy",                 // results from emulating XYScope OR RMVideo noisy-dots targets during a trial; available 
   "xynoisytimes"             //    for Maestro data file w/version >= 12. See NOISYEM.H.
};
const int NUMOUTFIELDS = 22;  // the # of fields in the output structure

const char* headerFields[] =  // defines MATLAB structure mirroring the contents of the data file header record (the
{                             // field names are the same as corresponding members of the CXFILEHDR structure 
                              // -- see include file CXFILEFMT.H for a detailed description).
   "trhdir", "trvdir",
   "nchar", "npdig",
   "nchans", "chlist",
   "d_rows", "d_cols", "d_crow", "d_ccol", "d_dist", "d_dwidth", "d_dheight", "d_framerate",
   "iPosScale", "iPosTheta", "iVelScale", "iVelTheta",
   "iRewLen1", "iRewLen2",
   "dayRecorded", "monthRecorded", "yearRecorded",
   "version", "flags",
   "nScanIntvUS", "nBytesCompressed", "nScansSaved",
   "spikesFName",
   "nSpikeBytesCompressed", "nSpikeSampIntvUS",
   "dwXYSeed",
   "iRPDStart", "iRPDDur", "iRPDResponse", "iRPDWindows", "iRPDRespType",
   "iStartPosH", "iStartPosV",
   "dwTrialFlags",
   "iSTSelected", "iVStabWinLen", "iELInfo",
   "setName", "subsetName",
   "rmvSyncSz", "rmvSyncDur",
   "timestampMS",
   "rmvDupEvents"
};
const int NUMHDRFIELDS = 49;  // the # of fields in the header

const char *tagFields[] =     // each structure in the "tags" output field represents a labelled tag attached to file
{
   "time",                    //   the tag's timestamp: elapsed time since recording began, in ms (can be -1)
   "label"                    //   the tag's label (a Matlab string
};
const int NUMTAGFIELDS = 2;

const char *trajFields[] =    // trajectory data for targets participating in a trial (TrialMode files only):
{
   "on",                      //    cell array holding target ON epochs: [tOn1 tOff1 tOn2 tOff2 ...]
   "hpos",                    //    target pos & vel trajectories, as calculated from trial codes.  pos in deg, vel in
   "vpos",                    //    deg/sec.  each field is an MxN matrix, where the #tgts in trial = M, and the
   "hvel",                    //    recorded trial length = N "ticks" (1ms).  row 0 is the trajectory for the first
   "vvel",                    //    tgt in the trial tgt list, row 1 for the second tgt, and so...

   "patvelH",                 //    analogously for target pattern velocity trajectories. these really only apply to
   "patvelV",                 //    certain extended video targets on the XY scope and framebuffer platforms.

   "targnums",                //    old-style IDs of targets that participated in the trial (data file version < 2).
                              //    listed in same order that trajectories are stored above.  Prepared for all data
                              //    file versions, to avoid breaking existing programs that were based on an earlier
                              //    version of this MEX function.

   "nTrialLen",               //    complete trial length in # of trial ticks
   "tRecordOn"                //    the trial time at which recording began (may be > 0)
};
const int NUMTRAJFIELDS = 10; // the # of fields in the target trajectories' structure

const char *tgtFields[] =     // target definitions extracted from Maestro data files (version >= 2)
{
   "category",                //    CX_FIBER*, CX_REDLED*, CX_CHAIR, CX_XYTARG, CX_FBTARG, etc.
   "name",                    //    the target's human-readable name in Maestro
   "params",                  //    defining parameter structure -- only for CX_XYTARG and CX_FBTARG types; format
                              //    echoes that of the XYPARMS and FBPARMS structs defined in CXOBJ_IFC.H.
   "dwState",                 //    [ContMode only] target state in ContMode -- relevant flags defined in CXFILEFMT.H
   "hPos",                    //    [ContMode only] horizontal target pos when recording started.
   "vPos"                     //    [ContMode only] vertical target pos when recording started.
};
const int NUMTGTFIELDS = 6;

const char *xyTgtParams[] =   // defining parameters for a Maestro XY scope target ("category" = CX_XYTARG).  This is
{                             // essentially the XYPARMS struct as defined in CXOBJ_IFC.H...
   "type",                    //    target type
   "ndots",                   //    # of dots in target

   "iDotLfUnits",             //    [FCDOTLIFE only] dot life units:  DOTLFINMS or DOTLFINDEG
   "fDotLife",                //    [FCDOTLIFE only] maximum lifetime of each target dot

   "fRectW",                  //    tgt window dimensions in deg subtended at eye; meaning varies w/ tgt type...
   "fRectH",
   "fInnerW",
   "fInnerH",
   "fInnerX",                 //    [RECTANNU only] offset of center of "hole" WRT center of tgt bounding rect (deg)
   "fInnerY"
};
const int NUMXYTGTPARMS = 10;

                              // FOR DATA FILE VERSIONS < 8.  Starting w/ V=8, FB video tgts replaced by RMVideo tgts
const char *fbTgtParams[] =   // defining parameters for a Maestro framebuf video target ("category" = CX_FBTARG).
{                             // This is essentially the FBPARMS struct as defined in CXOBJ_IFC.H...
   "type",                    //    target type
   "shape",                   //    shape of target aperture
   "csMean",                  //    RGB color specification:  mean and contrast for R(=0), G(=1), and B(=2) axes
   "csCon",                   //   mean is 0-1000 (uniform scale); contrast is a percentage (0-100%)
   "fRectW",                  // dimensions of bounding rect in deg subtended at eye
   "fRectH",
   "fSigma",                  // standard deviation of circular Gaussian window for STATICGABOR target
   "fGratSF",                 // grating spatial frequency in cycles/deg subtended at eye (1x2 matrix, for 2 gratings)
   "fGratAxis",               // grating's drift axis in deg CCW (1x2 matrix)
   "fGratPhase"               // grating's initial spatial phase in deg (1x2 amtrix)
};
const int NUMFBTGTPARMS = 10;

const char *rmvTgtParams[] =  // defining parameters for a Maestro RMVideo target (CX_RMVTARG), file versions >= 8
{                             // We use the same field names as in the RMVTGTDEF struct defined in RMVIDEO_COMMON.H.
   "iType",
   "iAperture",
   "iFlags",
   "iRGBMean",                // (1x2 matrix, for 2 gratings)
   "iRGBCon",                 // (1x2 matrix)
   "fOuterW",
   "fOuterH",
   "fInnerW",
   "fInnerH",
   "nDots",
   "nDotSize",
   "iSeed",
   "iPctCoherent",
   "iNoiseUpdIntv",
   "iNoiseLimit",
   "fDotLife",
   "fSpatialFreq",            // (1x2 matrix)
   "fDriftAxis",              // (1x2 matrix)
   "fGratPhase",              // (1x2 matrix)
   "fSigma",                  // (1x2 matrix)
   "strFolder",               // string; added 17sep2009, v >= 13. An empty string if v < 13 or type != RMV_MOVIE.
   "strFile",                 // string; added 17sep2009, v >= 13. An empty string if v < 13 or type != RMV_MOVIE.
   "iFlickerOn",              // added 13may2019, v >= 23. 0 if v < 23.
   "iFlickerOff",             // ditto.
   "iFlickerDelay",           // ditto.
   "fDotDisp"                 // added 18dec2024, v >= 25. 0 if v < 25.
};
const int NUMRMVTGTPARMS = 26;

/* [20nov2024] Eliminated the output fields 'stimulusrun' and ''psgm'.
const char *runFields[] =     // stimulus run definition extracted from Maestro data files
{
   "bRunning",                //    was stimulus run in progress when recording started?
   "iDutyPeriod",             //    duty period in milliseconds
   "iDutyPulse",              //    DOUT ch# for duty cycle marker pulse (0 = no marker)
   "nAutoStop",               //    auto-stop after this many cycles elapsed (0 = disabled)
   "fHOffset",                //    horizontal position offset in deg subtended at eye
   "fVOffset",                //    vertical position offset in deg subtended at eye
   "nXYTgts",                 //    #XY scope targets participating in an XYseq stimulus
   "stimuli"                  //    definitions of the active stimulus channels participating in the run (see below)
};
const int NUMRUNFIELDS = 8;

const char *stimFields[] =    // defn of a single stimulus channel within a stimulus run (data file vers >= 2):
{
   "bOn",                     //    should always be TRUE, since only active channels are saved in data file
   "iMarker",                 //    OFF (0), or DOUT ch# on which marker pulse is delivered at stimulus start
   "iType",                   //    type of stimulus:  see STIM_IS** constants in CXOBJ_IFC.H
   "iStdMode",                //    motion mode for the "standard" stim types: MODE_ISSINE or _ISPULSE
   "tStart",                  //    start time of stimulus trajectory within the run's duty cycle, in millisecs
   "params"                   //    structure containing type-specific stimulus channel parameters (see below)
};
const int NUMSTIMFIELDS = 6;

const char *xyseqFields[] =   // parameters specific to the STIM_ISXYSEQ stimulus channel (aka XYSEQSTIM struct):
{
   "iOpMode",                 //    motion mode -- MODE_ISSPARSEDIR, etc.
   "iRefresh",                //    XY scope refresh period, in millisecs
   "nSegs",                   //    # of distinct segments of random motion
   "iSegDur",                 //    duration of each segment, in ms (must be multiple of refresh period)
   "iSeed",                   //    seed for generating random directions or velocities
   "nChoices",                //    # of different directions (or velocities) randomized
   "fAngle",                  //    offset angle (for direction modes) or direction of motion (for vel modes)
   "fVel",                    //    velocity of motion (for dir modes) or max velocity (for vel modes)
   "fOffsetV"                 //    offset velocity (for vel modes only)
};
const int NUMXYSEQFIELDS = 9;

const char *sgmFields[] =     // parameters specific to the STIM_ISPSGM stimulus channel (aka SGMPARMS struct);
{							         // also used for the output field 'psgm' :
   "tStart",                  //    trial tick at which PSGM started ('psgm' field only)
   "iOpMode",                 //    motion mode -- one of the SGM_* defined constants
   "bExtTrig",                //    if TRUE, use external trig to initiate pulse seq; else, s/w start.
   "iAmp1", "iAmp2",          //    pulse amplitude in mV.  range [-10240..10160mV], res = 80mV.
   "iPW1", "iPW2",            //    pulse width.  range [50..2500us], res = 10us.
   "iPulseIntv",              //    interpulse interval.  range [1..250ms].
   "iTrainIntv",              //    intertrain interval.  range [10..2500ms], res = 10ms.
   "nPulses",                 //    #pulses per train.  range [1..250].  (train modes only)
   "nTrains"                  //    #trains per stimulus.  range [1..250].  (train modes only)
};
const int NUMSGMFIELDS = 11;

const char *sineFields[] =    // parameters specific for sinewave stimulus channels (aka SINESTIM struct):
{
   "iPeriod",                 //    # of complete cycles in stimulus (>=1)
   "fAmp",                    //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   "fPhase",                  //    phase in deg: [-180.0 .. 180.0]
   "fDirec"                   //    direction of motion, CCW angle from x-axis [-180.0..180.0]
};
const int NUMSINEFIELDS = 4;

const char *pulseFields[] =   // parameters specific for trapezoidal pulse stimulus channels (aka PULSESTIM struct):
{
   "bBlank",                  //    if TRUE, active targets are blanked during pulse (for CHAIR stimulus)
   "iPulseDur",               //    duration of pulse in ms (>= 2ms)
   "iRampDur",                //    duration of rising-edge and falling-edge ramps (>= 2ms)
   "fAmp",                    //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   "fDirec"                   //    direction of motion, CCW angle from x-axis [-180.0..180.0]
};
const int NUMPULSEFIELDS = 5;
*/


const char *tagSectFields[] = // information provided for each tagged section defined on a Maestro trial:
{
   "tag",                     //    the name of the tagged section
   "firstSeg", "lastSeg",     //    the range of trial segments spanned by the tagged section (zero-based indices)
   "tStart",                  //    the time at which the tagged section began, RELATIVE to when recording started.
                              //    Units = # of trial "ticks".  Will be -1 if unable to determine!
   "tLen"                     //    length of tagged section, in # of trial "ticks".  -1 if unable to determine!
};
const int NUMTAGSECTFIELDS = 5;

const char *trialInfoFields[] =  // additional information provided for a Maestro trial:
{
   "segStart",                //    1xN array of trial segment start times in trial ticks. N = #trial segments.
   "fix1",                    //    1XN array holding 0-based index of fix tgt #1 for each segment. -1 = "none"
   "fix2",                    //    ... and analogously for fix tgt #2
   "duration",                //    total trial length in #ticks. Greater than or equal to recorded trial length!
   "tRecord",                 //    elapsed trial time (#ticks) at which recording began
   "perts"                    //    1xN array of structs, where N is the number of perturbations applied 
                              //    during trial. See pertInfoFields[] for structure members.
};
const int NUMTRIALINFOFIELDS = 6;

const char *pertInfoFields[] =   // descriptive information on a Maestro trial perturbation
{
   "tgt",                     //    ordinal position of affected target in the trial target list
   "cmpt",                    //    ID of affected trajectory component (PERT_ON_*** constant)
   "start",                   //    elapsed trial time (#ticks) at which perturbation began
   "amp",                     //    perturbation amplitude
   "type",                    //    perturbation type (PERT_IS_*** constant)
   "dur"                      //    duration of perturbation in milliseconds
};
const int NUMPERTINFOFIELDS = 6;

#endif   // !defined(READCXDATA_H__INCLUDED_)
