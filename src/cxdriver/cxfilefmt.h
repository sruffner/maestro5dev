//=====================================================================================================================
//
// cxfilefmt.h :  Data structures defining the format of data records in Maestro trial- and cont-mode data files.
//
// AUTHOR:  saruffner, sglisberger, et al.
//
// DESCRIPTION:
// All Maestro data files saved during trial or continuous mode are binary files made up of a series of 1024-byte data
// records.  The first record is always a "header" containing some descriptive information useful to programs that
// analyze the data files.  The header record is followed by "data records" containing recorded analog and digital
// data, as well as "information records" that (along with the header record) define the conditions under which the
// data was recorded.  (In addition, the analysis program XWORK may augment the data file with a number of "analysis"
// records that "remember" previous analysis actions carried out on the raw data.  However, such records are not our
// concern here.)
//
// This file defines the data structures and related constants required to format the contents of the header, data,
// and information records found in Maestro data files.
//
// The format of the header record has changed significantly as Cntrlx -- Maestro's predecessor -- evolved, but the
// modifications have been made carefully in an attempt to ensure "backwards compatibility".  Thus, Maestro inherits
// the format that was originally laid out by Cntrlx (also known as cntrlxUNIX/PC).  We maintain a VERSION HISTORY with
// this file in an effort to assist those who are writing analysis programs that are attempting to read Maestro or
// Cntrlx data files of any "age".
//
//
// VERSION HISTORY from Cntrlx
// ---------------------------
// ==> VERSION 0 (??? - 28jan2002).
// [NOTE:  A version field was not introduced in the header record until 29jan2002.  Thus, since all unused bytes in
// the header are always set to 0, a version# of zero indicates that the data file was saved prior to the Cntrlx
// version dtd 29jan2002.]
//
// pre-16oct2001-- When I began maintaining cntrlxUNIX in aug1998, 'd_framerate' was the last defined field in the
// record; all bytes that followed were unused and always set to 0.
//
// 16oct2001-- Six new fields were allocated from the unused portion:  'iPosScale' through 'iRewLen2'.  Observe that we
// ensure "backwards compatibility" because we do not change existing fields but merely allocate new ones from the
// unused section.
//
// A new kind of "information record" was added to both TrialMode and ContMode data files in May 2000.  This "target &
// stimulus run" record held the definitions of all targets participating in a trial or stimulus run, and the complete
// definition of the stimulus run that was active at the time data recording began in ContMode.  The stimulus run defn
// was added to the record type in Nov 2001.  For a detailed version history and description of the record format,
// see TGDATREC.C in the Cntrlx archive.
//
// ==> VERSION 1 (effective 29jan2002).
// Two major changes were made with the 29jan2002 installation of Cntrlx.  Versioning was introduced, and a header was
// added to continuous-mode data files.  Files saved in continuous mode prior to this effective date had no header
// whatsoever.  The following fields were added with this version:
//
//    dayRecorded, monthRecorded, yearRecorded -- The date of recording.  Use with caution -- we rely on the Cntrlx
//       machine's date facility; if the date is incorrect, all bets are off!
//    version -- The version #.  As mentioned earlier, this will read 0 for data files recorded prior to 29jan2002.
//    flags -- Some useful additional information about the trial or cont-mode run.  See description of flag bits.
//       Note that we could define additional flag bits in future versions.  The following flags were defined with
//       this version:  CXHF_ISCONTINUOUS, CXHF_SAVEDSPIKES, CXHF_REWARDEARNED, CXHF_REWARDGIVEN.
//    nScanIntvUS -- Currently, the AI scan interval in Cntrlx is fixed at 1ms (1000us) in TrialMode and at 2ms in
//       ContMode.  This field was added in case we choose to support other scan intervals at a later date.  The scan
//       interval is the interval between samples on any given channel of the AI device.  It corresponds to a single
//       clock "tick" in the Cntrlx acquisition timeline.
//    nBytesCompressed -- Total # of compressed bytes of AI data saved in file.  Since continuous-mode runs can be
//       potentially very long, it was necessary to introduce an int-valued version of the existing field 'nchar'.
//       This field will also support larger trial data files.  REPLACES the 'nchar' field.  The 'nchar' field is still
//       set to short(nBytesCompressed); it will be invalid for nBytesCompressed > 32767!!
//    nScansSaved -- Total # of AI scans, or ticks, recorded during trial or cont-mode run.  For the same reason, this
//       int-valued field REPLACES the short-valued 'npdig'.  The 'npdig' field is still set to short(nScansSaved); it
//       will be invalid for nScansSaved > 32767!!
//    spikesFName[] -- Name of the spikes waveform file recorded in concert with this data file.  If a spikes waveform
//       was not recorded, the first character of this string will be the NULL character ('\0').
//
//
// VERSION HISTORY for Maestro
// ---------------------------
// ==> VERSION 2 (effective 27mar2003).  Maestro inherits the VERSION=1 data header record format from Cntrlx, with a
// few changes:
//
//    1) Header data structure type name changed from CNTRLX86_KEY to CXFILEHDR, and KEYF_*** flags renamed CXHF_***.
//       Comments updated and reorganized.
//
//    2) Header fields 'trhdir' and 'trvdir' are now OBSOLETE, as Maestro does not support randomizing on trial
//       direction; these fields are 0 always.
//
//    3) Spike trace data are now saved as records in trial- or cont-mode data files, rather than as separate files.
//       This is because the spikesPC functionality has been incorporated into Maestro!  The new spike trace data
//       record (CX_SPIKEWAVERECORD tag) is very similar to the "slow-sampled" AI data record in format; the same
//       compression algorithm is used for both.  In addition, several commensurate changes were made to the header:
//          spikesFName[] -- OBSOLETE.
//          nSpikeBytesCompressed -- Total # of compressed bytes of spike trace data saved in file.
//          nSpikeSampIntvUS -- Sample interval for spike trace data, in microsecs.
//
//    4) The Cntrlx "target & stimulus run" record type (tag ID = 64) has been split into separate information records:
//    a "target definition" record (CX_TGTRECORD) and a "stimulus run" record (CX_STIMRUNRECORD).  The format of the
//    "target definition" record has been modified considerably to take into account Maestro's very different target
//    numbering scheme, as well as changes in how XY scope & FB video targets are defined.
//
// ==> VERSION 3 (effective 19nov2003).  Added header field 'dwXYSeed', which is the seed value for the random number
// generators employed by the firmware running the XY scope controller.  The seed was formerly reported in the trial
// code RANDDOM_SEED, but that code was made obsolete in Maestro.  One use of this seed is to reconstruct the sequence
// of random dot offset directions chosen during animation of the "Opt Center w/Noisy Dots".
//
// ==> VERSION 4 (effective 01may2005).  Introduced a new information record (CX_TAGSECTRECORD) which stores the tag
// names and segment ranges of any "tagged sections" defined on a Maestro trial.  Tagged sections were introduced to
// help analysis programs partition trial data into smaller chunks for separate processing.  Added header flag
// CXHF_HASTAGSECTS to indicate whether or not a trial includes one or more tagged sections.
//
// ==> VERSION 5 (as of Maestro version 1.3.2).  No change in format.  This version change simply marks the fact that
// noise perturbations are now generated by Maestro in a reproducible manner.  Another perturbation type, Gaussian-
// distributed noise, was also introduced.  A seed is included in the TARGET_PERTURB trial code group for the uniform
// and Gaussian noise perts, and the random number generators driving these perturbations were updated.  Analysis
// programs that calculate the trajectories of perturbed trial targets must verify that the file version is greater
// than or equal to this number.  The perturbation algorithms will eventually be included in the Maestro User Guide.
//
// ==> VERSION 6 (as of Maestro version 1.4.2).  Added several information fields and flag bits in the header record
// that are relevant only to trials that use the special R/P Distro feature, introduced in v1.4.0.
//
// ==> VERSION 7 (as of Maestro version 1.5.0).  No format changes.  However, as of this version, there is only one
// R/P Distro reward window, so CXFILEHDR.iRPDWindows[2:3] are now unused.  Also, the TARGET_HOPEN trial code was
// changed slightly to extend velocity stabilization over a contiguous span of trial segments instead of only a single
// segment.  Downstream programs which analyze the trial codes stored in the Maestro data file must account for this
// change.
//
// ==> VERSION 8 (as of Maestro version 2.0.0).  RMVideo replaced the VSG as our framebuffer video display.  RMVideo
// target defn (RMVTGTDEF) is compeletely different from the old framebuffer tgt defn (FBPARMS), so the union
// U_TGPARMS, a field of CXTARGET defined in CXOBJ_IFC.H, has a different structure.  CX_TGTRECORD records in a data
// file use CXTARGET to format the target definitions, so analysis programs that parse such records must be updated
// accordingly!
//    ***This version # also serves to mark a major change in the velocity stabilization feature.  The TARGET_HOPEN
// trial code is obsolete, replaced by the new TARGET_VSTAB code.  Velocity stabilization can now be engaged on any
// trial target during any selected segment or segments of the trial.  Downstream programs which analyze trial codes
// stored in the Maestro data file must account for this new code.  See also cxtrialcodes.h.
//
// ==> VERSION 9 (as of Maestro version 2.0.1).  Two fields were added to the XYPARMS structure ('fInnerX', 'fInnerY') 
// to support an XY scope RECTANNU target with an off-center "hole".  The XYPARMS struct is *still* the smaller member 
// of the union U_TGPARMS, so the sizes of U_TGPARMS and CXTARGET are *unchanged*.  However, downstream programs need 
// to be aware of the new XYPARMS fields in order to extract the full defn of a RECTANNU target.
//
// ==> VERSION 10 (as of Maestro version 2.1.1). Added CXFILEHDR.iRPDRespType. When an RP Distro trial is run and a 
// behavioral response measured, this field indicates the response type -- one of the defined constants TH_RPD_*** in 
// cxobj_ifc.h.
//
// ==> VERSION 11 (as of Maestro version 2.1.2) No format changes. However, as of this version, Maestro supports 
// perturbation of trial target window speed (PERT_ON_SWIN in cxobj_ifc.h) and pattern speed (PERT_ON_SPAT) without 
// altering the direction of motion. Also, the XYScope implementation of the NOISYSPEED target was changed so that 
// percentage speed noise was selected randomly with a granularity of 1% instead of 0.1%.
//
// ==> VERSION 12 (as of Maestro version 2.1.3) No format changes. However, as of this version, Maestro supports 
// simultaneous perturbation a trial target's window AND pattern direction (PERT_ON_DIR in cxobj_ifc.h), or a target's 
// window AND pattern speed (PERT_ON_SPD). In addition, support was added to support a second algorithm for per-dot 
// speed noise in the implementations of the XYScope NOISYSPEED target and the analogous RMVideo RMV_RANDOMDOTS tgt.
// Also, the percentage speed noise granularity for the RMV_RANDOMDOTS tgt was changed to 1% instead of 0.1% to bring 
// it in line with the change to the NOISYSPEED target in v11.
//
// ==> VERSION 13 (as of Maestro version 2.5.0) As of this version, RMVideo supports playing back a video file as a
// trial target. RMVideo target definition structure RMVTGTDEF (in rmvideo_common.h) was altered to support the new 
// RMV_MOVIE target type (added two char[] fields). Deprecated target record structures are included here to support 
// parsing of data files generated prior to these changes. In an unrelated change, introduced flag RMV_F_ORIENTADJ for
// RMV_GRATING targets. When set, the grating's orientation adjusts during animation IAW the direction of the per-frame
// pattern displacement vector (note that H and V cmpts of the vector are significant when the flag is set; when unset,
// the H cmpt is the displacement along the grating drift axis and the V cmpt is ignored).
// 
// ==> VERSION 14 (as of Maestro version 2.5.2) No format changes. Introduced new flag RMV_F_WRTSCREEN, which affects
// behavior of the RMVideo RMV_RANDOMDOTS target only. If set, target pattern motion is specified WRT global (screen)
// frame of reference rather than WRT the target window. This change permitted a better emulation of the XYScope 
// NOISYDIR and NOISYSPEED targets when target window and dots are moving in the same direction at the same speed (ie, 
// the dots are not moving WRT the target window). In addition, relaxed restrictions to permit velocity stabilization 
// of all RMVideo and XYScope targets.
//
// ==> VERSION 15 (as of Maestro version 2.6.0) Added CXFILEHDR.iStartPosH, .iStartPosV to save the global target 
// starting position offset applied during the trial (Trial mode only). Normally (0, 0), this may be set to something
// else when centering a stimulus protocol about a unit's receptive field center. Also fixed a bug in implementation of
// the rectangular annulus aperture for RMVideo targets and increased limit on #dots permitted for an RMVideo random
// dot patch target from 1000 to 9999 (RMVideo version 4).
// 
// ==> VERSION 16 (as of Maestro version 2.6.1) Added CXFILEHDR.dwTrialFlags to expose the trial flags that cannot be
// gleaned from the trial codes stored elsewhere in the file (Trial mode only). See cxobj_ifc.h for a list of all 
// recognized trial bit flags.
//
// ==> VERSION 17 (as of Maestro version 2.6.5) Added flags CXHF_ISSEARCHTSK, CXHF_ST_OK, CXHF_ST_DISTRACTED. Added
// CXFILEHDR.iSTSelected, the index of the trial target selected during a "searchTask" trial.
//
// ==> VERSION 18 (as of Maestro version 2.7.0) Marks significant changes in XYScope target implementations, including
// significant bug fixes to the "Oriented Bar" and "Noisy Dots" targets. Also, for all XYScope targets with patterns
// that move independent of target window, target pattern velocity is now specified relative to the target window (as
// has always been the case for RMVideo targets), rather than the screen. Finally, we introduced an optional sliding
// window average to smooth out eye-position noise in the hopes of improving the velocity stabillization feature in
// Trial Mode. The user-specified length of the sliding window is stored in new header field CXFILEHDR.iVStabWinLen.
//
// ==> VERSION 19 (as of Maestro version 3.0.0) No format changes, but marks the release of Maestro 3.0.0, which 
// targets the Windows7/RTX2011 platform and introduces new up-to-date peripherals for the AI/AO/DIO device functions
// (the NatInst PCIe-6363 handles all three!) and the RMVideo NIC. In addition, trial mode was changed so that target
// trajectories are computed on the fly and data is streamed to file as the trial proceeds. This eliminates many large
// buffers and removes restrictions on trial length.
//     Revision for Maestro v3.0.3 (05sep2013): The constant CX_SPIKESORTREC_LAST was changed from 20 to 57 to allow 
// storage of up to 50 different "sorted spike trains". This is strictly for analysis programs that add sorted spike
// train records to the original data file, so Maestro itself is unaffected. Since record IDs 21 to 57 were never used
// before, it was not necessary to change the data file version to accommodate this revision.
//    Revision for Maestro v3.1.0 (oct2013): New THF_*** flags were added that are used to mark trials presented in a
// "chained" trial sequence and, in particular, those trials that mark the start of a new trial "chain" (a sequence of
// one or more presentations of the same trial consecutively). Data file format unchanged. No need to increment the
// file version.
//
// ==> VERSION 20 (as of Maestro version 3.2.0) Marks introduction of support for the Eyelink tracker. Added header
// flag CXHF_EYELINKUSED and field iELInfo[], which lists Eyelink calibration parameters, the velocity smoothing filter
// window width, and stats on the observed Eyelink inter-sample interval during the data recording. For various reasons
// the inter-sample interval could exceed 1ms (Trial mode) or 2ms (Cont mode) on occasion.
//    Also added two special Eyelink events to the "other events" record, tag 3: "blink start" and "blink end". These
// will always occur in pairs, so that the duration of a blink can be calculated by subtracting the "blink end" time
// from the "blink start" time. If the last "blink start" event in the file lacks a matching "blink end", it should be
// assumed the subject was mid-blink when recording ended. Likewise, if the first blink end lacks a matching blink 
// start, then the subject was mid-blink when recording began.
//
// ==> VERSION 21 (as of Maestro version 4.0.0) Marks the release of Maestro 4.0.0, which runs as a 64-bit application
// on Windows 10. Added information fields setName[] and subsetName[] that hold the names of the trial set -- and, if 
// applicable, subset -- to which the presented trial belongs. Also added information fields rmvSyncSz/Dur that hold
// the spot size and flash duration for the optional RMVideo "vertical sync" spot flash feature (Trial mode only).
//    Revision for Maestro 4.0.1 (Dec 2018): Added a timestamp to file in field 'timestampMS'. It indicates the time
// at which trial started (or recording began, for a continuous-mode file), in milliseconds since Maestro started.
// This is intended to provide a way to measure the time interval between two successive trials. NOTE: It is stored as
// a 32-bit integer, so it will "rollover" after approximately 24.5 days of continuous operation. Since Maestro should
// always be shutdown at the end of an experiment day, we don't check for the rollover condition. Since Maestro 4 is
// not yet widely used, I did not increment the file version. The timestamp will be 0 in all V<21 files and in all
// V=21 files recorded before this change was made.
//
// ==> VERSION 22 (as of Maestro version 4.0.5) Field d_framerate now stores the display refresh rate in micro-Hz
// instead of milli-Hz units. We've found that it is important to preserve RMVideo's estimate of refresh period to 
// roughly 10-nanosecond precision, and multiplying the refresh rate by 1.0e6 is sufficient to do this for rates 
// ranging from 60 to 500Hz. Added array rmvDupEvents[] storing information on up to 3 "repeat frame" events detected
// by RMVideo during a trial. As of Maestro 4.0.5/RMVideo v9, the user can elect to "tolerate" up to 3 duplicate frames
// during a trial without aborting it. Added flag CXHF_DUPFRAME, set if any duplicate frames were detected.
//
// ==> VERSION 23 (as of Maestro version 4.1.0) As of this version, RMVideo offers a "flicker" feature for all target
// types. RMVideo target definition structure RMVTGTDEF (in rmvideo_common.h) was altered to support the new flicker
// parameters (3 int fields). Deprecated target record structures are included here to support parsing of data files 
// with version numbers 13-22. NOTE that RMVideo (V10) was also updated extensively to conform to OpenGL 3.3 Core
// Profile. Also added per-trial random reward withholding variable ratio feature (WHVR), but this had no impact on
// the data file format.
// [04jun2021 -- Comment only: Modifying JMWork and read/editcxdata to support up to 200 (instead of 50) sorted spike
// train channels. This does NOT impact Maestro, as the sorted spike train records are added to the data file by the
// post-analysis software.
//
// ==> VERSION 24 (as of Maestro version 4.2.0) As of this version, the "searchTask" op in Trial mode supports 1 or 2
// "goal" targets in addition to any distractors. Added header flag CXHF_ST_2GOAL, and reused CXHF_FIX*SELECTED to
// indicate which goal target is selected. In the 2-goal search task, selecting Fix1 earns reward pulse 1, while 
// selecting Fix2 earns reward pulse 2 (normally smaller), and selecting a distractor or no target at all earns NO
// reward. In the single-goal target case, selecting a distractor earns reward pulse 2.
// 
// ==> VERSION 25 (as of Maestro version 5.0.2) As of this version, Maestro no longer writes "stimulus run records"
// to the data file. Cont-mode stimulus runs are a rarely if ever used feature at this point. XYScope support was
// dropped in Maestro 4.0, and support for the never-used PSGM was dropped in Maestro 5.0.2 -- so the only available
// stimulus run channel type uses the animal chair, which may not be available in any active rigs!
//=====================================================================================================================

#if !defined(CXFILEFMT_H__INCLUDED_)
#define CXFILEFMT_H__INCLUDED_

#include "cxtrialcodes.h"                       // definition of TRIALCODE structure
#include "cxobj_ifc.h"                          // defns of CXTARGET, STIMCHAN; RMVideo typedefs


// format of the header record that occupies the first kilobyte of trial- and continuous-mode data files
//
// NOTES:
// (0) Continuous-mode data files did not possess this header record until version 1 was introduced.
// (1) T = applies to trial mode only, C = cont mode only, TC = both modes; V = version number.
// (2) If a field is NOT used in one of the modes, it is set to ZERO by default, unless otherwise specified.
// (3) The target translation/rotation factors and the reward pulse lengths saved in the header record represent the
// values that were in effect when the trial codes were generated.
// (4) In continuous mode, all RELEVANT fields reflect the system state AT THE TIME RECORDING STARTED, except the
// counters 'nBytesCompressed', 'nScansSaved', and 'nScanIntvUS'.  These latter fields are set when recording ends.
// (5) PORTING issue:  We rely on these data type sizes:  char = 1 byte, short = 2 bytes, int = DWORD = 4 bytes.
//

const int CXH_NAME_SZ            = 40;                // max length of names in header, including terminating null char
const int CXH_MAXAI              = 16;                // max # of AI channels that can be recorded
const int CXH_EXTRAS             = 308;               // # of unused shorts in header record
const int CXH_CURRENTVERSION     = 25;                // the current version # (effective Maestro version 5.0.2)

const int CXH_RMVDUPEVTSZ        = 6;                 // array size for duplicate frame events in RMVideo in Trial mode

                                                      // currently defined header flag bits:
const DWORD CXHF_ISCONTINUOUS    = ((DWORD) (1<<0));  //    if set, file was collected in cont mode; else, trial mode
const DWORD CXHF_SAVEDSPIKES     = ((DWORD) (1<<1));  //    if set, 25KHz spike trace saved during this trial
const DWORD CXHF_REWARDEARNED    = ((DWORD) (1<<2));  //    [T] if set, subject did not break fixation during trial
const DWORD CXHF_REWARDGIVEN     = ((DWORD) (1<<3));  //    [T] if set, the earned reward was actually delivered (reward
                                                      //       may be randomly withheld.
const DWORD CXHF_FIX1SELECTED    = ((DWORD) (1<<4));  //    [T] if set, tgt#1 was selected in a trial's "selByFix*" or
                                                      //    "searchTask" op, or was INITIALLY selected in "switchFix".
const DWORD CXHF_FIX2SELECTED    = ((DWORD) (1<<5));  //    [T] if set, tgt#2 was selected in a trial's "selByFix*" or
                                                      //    "searchTask" op, or was INITIALLY selected in "switchFix".
const DWORD CXHF_ENDSELECT       = ((DWORD) (1<<6));  //    [T] if set, selection forced at end of "selByFix" segment
const DWORD CXHF_HASTAGSECTS     = ((DWORD) (1<<7));  //    [T] if set, trial has one or more tagged sections.  Data
                                                      //    file should include a CX_TAGSECTRECORD.
const DWORD CXHF_ISRPDISTRO      = ((DWORD) (1<<8));  //    [T, V>=6] if set, trial used the "R/P Distro" op.
const DWORD CXHF_GOTRPDRESP      = ((DWORD) (1<<9));  //    [T, V>=6] if set, then trial got past "R/P Distro" segment

const DWORD CXHF_ISSEARCHTSK     = ((DWORD) (1<<10)); //    [T, V>=17] if set, trial used the "searchTask" op.
const DWORD CXHF_ST_OK           = ((DWORD) (1<<11)); //    [T, V>=17] "searchTask" result: goal tgt selected,  
const DWORD CXHF_ST_DISTRACTED   = ((DWORD) (1<<12)); //    distractor selected, or no tgt selected.
const DWORD CXHF_EYELINKUSED     = ((DWORD) (1<<13)); //    [V>=20] if set, Eyelink tracker used to monitor eye traj
const DWORD CXHF_DUPFRAME        = ((DWORD) (1<<14)); //    [V>=22] if set, RMVideo detected 1 or more repeat frames
const DWORD CXHF_ST_2GOAL        = ((DWORD) (1<<15)); //    [V>=24] if set, trial performed 2-goal "searchTask" op.

typedef struct tagCxFileHdr
{
   char name[CXH_NAME_SZ];          // [T] name of trial (may be truncated). set to "**continuous_mode_run**" in all
                                    // continuous-mode data files.
   short trhdir;                    // [V<2, T] trial's H direction (RIGHT==1 or LEFT==2); [V>=2] OBSOLETE!
   short trvdir;                    // [V<2, T] trial's V direction (UP==4 or DOWN==8); [V>=2] OBSOLETE!

   short nchar;                     // [V<1, T] same as 'nBytesCompressed'; [V>=1] OBSOLETE!
   short npdig;                     // [V<1, T] same as 'nScansSaved'; [V>=1] OBSOLETE!

   short nchans;                    // [TC] the # of distinct AI channels recorded
   short chlist[CXH_MAXAI];         // [TC] the channel scan list -- lists the channel #s (0-15) that were recorded,
                                    // in the order that they were sampled per scan.  this info is critical in order to
                                    // properly decompress the analog channel data!

                                    // [T] video display (XYScope or RMVideo or oldVSG framebuffer) info:
   short d_rows;                    //    height of display in pixels
   short d_cols;                    //    width of display in pixels
   short d_crow;                    //    IGNORE
   short d_ccol;                    //    IGNORE
   short d_dist;                    //    distance from eye to screen, in mm
   short d_dwidth;                  //    width of display, in mm
   short d_dheight;                 //    height of display, in mm
   int d_framerate;                 //    frame rate in units of milli-Hz [V<=21] or micro-Hz [V>=22]

                                    // 16oct2001-- added these fields:
   int iPosScale;                   // [T] 1000 * "global" target position scale factor (set to 1.0 for cont mode)
   int iPosTheta;                   // [T] 1000 * "global" target position rotation angle (deg)
   int iVelScale;                   // [T] 1000 * "global" target velocity scale factor (set to 1.0 for cont mode)
   int iVelTheta;                   // [T] 1000 * "global" target velocity rotation angle (deg)
   int iRewLen1;                    // [TC] reward pulse length #1 (msec)
   int iRewLen2;                    // [T] reward pulse length #2 (msec)

                                    // 29jan2002-- new fields added, changes made to incorporate header into cont-mode
                                    // data files.  versioning introduced.  all fields above were present prior to
                                    // versioning (version == 0).  the fields in this section were added in version 1.
   int dayRecorded;                 // [TC] the day (1-31), month(1-12), and year that data was recorded.
   int monthRecorded;               //
   int yearRecorded;                //
   int version;                     // [TC] file version -- so analysis programs can more easily parse future headers.
   DWORD flags;                     // [TC] see CXHF_** flag bits
   int nScanIntvUS;                 // [TC] channel scan intv in microsecs; currently 1000 (1ms) for trial mode and
                                    // 2000 (2ms) for cont mode, but this allows us to support other intv's later
   int nBytesCompressed;            // [TC] total # of bytes of compressed analog data collected.
   int nScansSaved;                 // [TC] total # of channel scans saved.
   char spikesFName[CXH_NAME_SZ];   // [V<2, TC] name of spike waveform file saved when this data recorded;
                                    // [V>=2, TC] OBSOLETE! spike trace data saved in this file

                                    // 27mar2003-- these fields were added with the first release of Maestro (V=2).
   int nSpikeBytesCompressed;       // [V>=2, TC] total # of bytes of compressed 25KHz spike trace data
   int nSpikeSampIntvUS;            // [V>=2, TC] sample intv for the spike trace channel, in microsecs

                                    // 19nov2003-- Added one field (V=3).
   DWORD dwXYSeed;                  // [V>=3, T] number used to seed random# generation on XY scope controller

                                    // 25jan2006-- (V=6) Added fields relevant only to trials using R/P Distro feature.
   int iRPDStart;                   // [V>=6, T] start of R/P Distro designated seg, in ms rel to start of trial.
   int iRPDDur;                     // [V>=6, T] duration of R/P Distro designated seg, in ms.
   int iRPDResponse;                // [V>=6, T] 1000 X avg response during R/P Distro segment, in resp sample units.
   int iRPDWindows[4];              // [V>=6, T] reward windows for the R/P Distro trial: [a b c d]. [a b], a<=b, is the
                                    // the first window; if a==b, the window is not defined.  Similarly for the second
                                    // window, [c d]. Units = 0.001 deg/sec.
                                    // [V>=7, T] only one reward window defined, so now c==d==0 always.

   int iRPDRespType;                // [V>=10, T] R/P Distro behavioral response type (TH_RPD_*** const in cxobj_ifc.h)
   
   int iStartPosH;                  // [V>=15, T] 1000 * "global" target position horizontal offset (deg)
   int iStartPosV;                  // [V>=15, T] 1000 * "global" target position vertical offset (deg)

   DWORD dwTrialFlags;              // [V>=16, T] Trial flag bits (copy of TRLHDR.dwFlags -- see cxobj_ifc.h)
   int iSTSelected;                 // [V>=17, T] zero-based index of target selected during "searchTask" trial; -1
                                    // if no target selected; 0 if this is NOT a "searchTask" trial.

   int iVStabWinLen;                // [V>=18, T] length of sliding window used to average out eye-pos noise to improve
                                    // VStab performance, in ms (ie, no. of "ticks")
   
   // [V>=20, TC] Eyelink info: 0 = record type (EL_** const in cxobj_ifc.h); 1,2 = X,Y offset; 3,4 = X,Y gain; 
   // 5 = vel smoothing window width in ms; 6 = #repeat samples; 7=max inter-sample delay ms; 
   // 8 = 1000 * avg inter-sample delay in ms.
   int iELInfo[9];

   char setName[CXH_NAME_SZ];       // [V>=21, T] name of trial set containing the trial presented
   char subsetName[CXH_NAME_SZ];    // [V>=21, T] name of trial subset containing the trial presented, if applicable
   short rmvSyncSz;                 // [V>=21, T] spot size (mm) for RMVideo "vertical sync" flash; 0 = disabled
   short rmvSyncDur;                // [V>=21, T] duration (# video frames) for RMVideo "vertical sync" flash

   // [V>=21, TC] time at which trial or CM recording started, in milliseconds since Maestro started
   int timestampMS;

   // [V>=22, T] info on up to 3 duplicate frame events detected by RMVideo during trial. Each event is represented by
   // a pair of integers [N,M]. N>0 is the frame index of the first repeat frame in the event, and M is the number of
   // contiguous duplicate frames caused by a rendering delay on the RMVideo side. However, if M=0, then a single
   // duplicate frame occurred at frame N because RMVideo did not receive a target update in time.
   int rmvDupEvents[CXH_RMVDUPEVTSZ];

   short xtras[CXH_EXTRAS];         // UNUSED -- always set to zero
} CXFILEHDR, *PCXFILEHDR;


// format of data/information records in Maestro/Cntrlx data files
//
//       RECORD TAG (bytes0..7)        RECORD CONTENTS
//       ----------------------        ---------------
//       0  0  0  0  0  0  0  0        Compressed slow-sampled AI data.
//       1  0  0  0  0  0  0  0        Interevent intervals for events on timer device's DI<0> -- usually spikes!
//       2  0  0  0  0  0  0  0        Interevent intervals events on timer device's DI<1>.
//       3  0  0  0  0  0  0  0        Event masks & times for all other events, DI<15..2>, plus Eyelink "blinks"
//       4  0  0  0  0  0  0  0        Trial codes.
//     [64  0  X  X  X  X  X  X        Target defn/stimulus run info for VERSION <=1.  OBSOLETE.]
//      65  0  0  0  0  0  0  0        Definitions of participating targets.
//      66  0  0  0  0  0  0  0        Definition of active stimulus run (ContMode data files only).
//      67  0  0  0  0  0  0  0        Compressed spike trace data.
//      68  0  0  0  0  0  0  0        Trial tagged section info.
//
//       5  0  0  0  0  0  0  0        Editing action record appended to data file by analysis programs
//       N  M  0  0  0  0  0  0        Sorted spike train records appended to data file by analysis programs
//                                     (N=8..57, M=0..3 -- Supports 200 different channels in one file)
//
//    1) Compressed AI data (record tag 0).  The compressed, slow-sampled AI data are stored chronologically in one or
//    more records.  Since no compressed byte will have the value 0, we mark the end of the compressed data stream by
//    filling the empty portion of the last record with zeros.  Analysis programs must have knowledge of the
//    compression algorithm in order to decode these records.
//
//    2) Event records (record tags 1-3).  Maestro/Cntrlx records digital events (occurrence of rising edge) on any of
//    16 inputs DI<15..0> on the event timer device.  We store separate event records for events on DI<0> (usually
//    reserved for recording spikes), events on DI<1> (usually reserved for a second spike channel or a marker pulse),
//    and events on any of DI<15..2>.  In the first two cases, we save 32-bit interevent intervals (where one tick =
//    10microsec), thusly:
//          CXFILEREC.u.iData[0] = interval between event# 0 and 1,
//          CXFILEREC.u.iData[1] = interval between event# 1 and 2,
//          ....
//          CXFILEREC.u.iData[N] = interval between event# N and N+1, ....
//    where "event# 0" corresponds to the time at which recording began.  The unfilled portion of the last record is
//    filled with the integer 0x7FFFFFFF, which serves as an "endOfData" marker.  Events on DI<15..2> are all stored in
//    record tag 3.  Here we must save both the event mask and the absolute 32-bit event time (again, one tick = 10us),
//    as follows:
//          CXFILEREC.u.iData[0] = event mask for event#0 on any of DI<15..2> (multiple simultaneous events possible!)
//          CXFILEREC.u.iData[1] = time of occurrence of event#0
//          ....
//          CXFILEREC.u.iData[N] = event mask for event#(N/2) on any of DI<15..2>
//          CXFILEREC.u.iData[N+1] = time of occurrence of event #(N/2), ...
//    In this case, the "endOfData" marker is the sequence of two integers {0, 0x7FFFFFFF}, repeated until we reach the
//    end of the record.
//
//          [VERSION >=20 ] Record tag 3 is also used to store "blink start" and "blink end" events detected when using
//    the Eyelink tracker to measure the subject's eye trajectory. These two events have special event masks, 
//    CX_EL_BLINKSTARTMASK and CX_EL_BLINKENDMASK, that don't overlap with events on DI<15..2>. Event time is specified
//    in milliseconds rather than 10us ticks, since they're detected during runtime processing, not timestamped by the
//    event timer. They will occur in matching start-end pairs (although not necessarily adjacent, since there could be
//    DI events in between) -- except when the subject is mid-blink at the start or end of the data recording.
//
//    3) Trial codes (record tag 4).  We store the trial code array CODES[] that defines a trial in chronological order
//    in one or more records as needed.  So:
//          CXFILEREC.u.tc[0] = TC[0],
//          CXFILEREC.u.tc[1] = TC[1],
//          ....
//    If the last trial code record is only partially full, we set the remaining bytes to zero.  Note that the last
//    trial code is always ENDTRIAL, so there is no need to define a "endOfTrialCodes" byte!  New trial codes have been
//    introduced over the development history of Maestro and its UNIX-based predecessor Cntrlx.  See CXTRIALCODES.H and
//    review Cntrlx's revision history for details...  Of particular note is the TARGET_PERTURB trial code, since
//    support for perturbations was entirely overhauled in Maestro.
//
//    3a) Trial tagged sections (record tag 68). [VERSION >= 4].  A TrialMode data file will have one of these records
//    if any tagged sections are defined on the trial that was executed.  The record contains a set of TRIALSECT
//    structures, one for each distinct section in the trial.  All unused bytes in the record are set to zero, so the
//    first TRIALSECT structure with an empty tag name marks the end of the section list!
//
//    4) Target definitions (record tag 65). [VERSION >= 2]  In a TrialMode data file, these records will contain the
//    definitions of all targets participating in the trial.  Each target is "persisted" as a CXFILETGT data structure,
//    which is defined here.  The target definition itself is encapsulated by the CXTARGET structure, which is fully
//    described in the Maestro object interface file CXOBJ_IFC.H; additional info in the CXFILETGT struct applies only
//    to ContMode data files.  More than one target definition can appear in each target defn record, and the target
//    definitions are listed in the same order that they appear in the trial definition.  This ordering is SIGNIFICANT,
//    since each target participating in a trial is identified in the TRIALCODEs by the target's ordinal position in
//    the trial target list!!!
//       In a ContMode data file, this record type is used to report the definitions of any targets in the active
//    target list, followed by any targets participating in an XYseq.  Again, we use the CXFILETGT data structure to
//    describe each target, and the targets are stored in the record in the same order they appear in the active list
//    and XYseq target list.  Unlike for TrialMode targets, we also provide target position and some state information
//    for ContMode targets.  For XYseq targets, the target position gives the initial and final position of each tgt
//    during the stimulus run, since the XYseq stimulus does not permit target window motion (only pattern motion).
//
//       If the last target defn record is only partially full, we set the remaining bytes to zero.  The remaining
//    "dummy" target records will contain invalid data (in particular, 0 is an invalid target type).
//
//       [VERSION < 8] The CXTARGET structure, part of CXFILETGT, changed when RMVideo replaced the old VSG FB video 
//    in Maestro 2.0.0.  To accommodate the parsing of target records from data files with v <= 7, we include the old 
//    structure definitions:  CXTARGET_V7, CXFILETGT_V7. CXTARGET_V7 depends on U_TGPARMS_OLD and FBPARMS, which are 
//    defined in cxobj_ifc.h Analysis programs MUST check the data file version and use the correct structure to 
//    properly parse target records!!!
//
//       [VERSION >= 9] Two new fields were added to XYPARMS, which is one member of the union U_TGPARMS that appears 
//    in CXTARGET.  Since XYPARMS is still the smallest member of U_TGPARMS, the storage size of U_TGPARMS and CXTARGET 
//    are UNCHANGED.  However, external programs that need to fully parse the target records in a data file must be 
//    revised to handle the new fields, XYPARMS.fInnerX and XYPARMS.fInnerY.
//
//       [VERSION >= 11] Percentage speed noise granularity for the XYScope NOISYSPEED target was changed from 0.1% to 
//    1%. This was only an implementation change. The speed noise limit stored in XYPARMS.fInnerW was already stored 
//    as a whole %.
//
//       [VERSION >= 12] Percentage speed noise granularity for the RMVideo RMV_RANDOMDOTS target was changed from 
//    0.1% to 1%. RMVTGTDEF.iNoiseLimit (see rmvideo_common.h) now stores the speed noise limit as a whole % in 
//    [0..300], rather than in units of 0.1%. In addition, both the NOISYSPEED and RMV_RANDOMDOTS implementations were 
//    extended to introduce a second, multiplicative method of per-dot speed noise generation: Rdot ~ Rpat * 2^X, 
//    where X is uniformly chosen from [-N..N], where the "noise limit" N must be an integer in [1..7]. This noise 
//    limit is stored in XYPARMS.fInnerW or RMVTGTDEF.iNoiseLimit. To select the multiplicative speed noise algorithm, 
//    the field XYPARMS.fInnerX is nonzero, or RMVTGTDEF.iFlags includes the newly defined flag RMV_F_SPDLOG2.
//
//       [VERSION >= 13] Introduced RMVideo target type RMV_MOVIE. Required introduction of new fields to the RMVideo
//    target definition struct RMVTGTDEF (the char[] arrays RMVTGTDEF.strFolder and .strFile), triggering changes to 
//    U_TGPARMS, CXTARGET, and CXFILETGT. To accommodate parsing of target records from data files with v=[8..12], we 
//    include the old structure definitions CXTARGET_V12 and CXFILETGT_V12. These definitions depend on deprecated 
//    types RMVTGTDEF_V12 (located in rmvideo_common.h) and U_TGPARMS_V12 (in cxobj_ifc.h).
//
//       [VERSION >= 23] Introduced new "flicker" feature applicable to all RMVideo target types. Required introduction
//    of 3 new integer fields to the RMVideo target definition struct RMVTGTDEF, triggering changes to U_TGPARMS, 
//    CXTARGET, and CXFILETGT. To accommodate parsing of target records from data files with v=[13..22], we include
//    the old structure definitions CXTARGET_V22 and CXFILETGT_V22. These definitions depend on deprecated types 
//    RMVTGTDEF_V22 (located in rmvideo_common.h) and U_TGPARMS_V22 (in cxobj_ifc.h).
//
//    5) Stimulus run definition (record tag 66). [2 <= VERSION < 25]  A ContMode "stimulus run" is defined by some 
//    header parameters, a series of STIMCHAN channels (not all of which may be turned on), and a set of XY scope tgts
//    participating in the run's XYseq stimulus channel (if there is one).  The XYseq target set is reported in the
//    target definition record(s), as described above.  The stimulus run header, along with the defns of those stimulus
//    channels which were turned ON, are persisted using this record type.  The header and each stimulus channel defn
//    are stored as one or more CXFILESTIM_U structures within the 1024-byte record.  The first CXFILESTIM_U object in
//    the record is always the header info, while the remaining objects in the first (and, if necessary, a second)
//    record are the stimulus channel definitions.
//
//       !!! CAVEAT:  Target and stimulus run information saved here merely represent Maestro's state AT THE TIME
//       !!! RECORDING BEGAN.  Active target position and state can be interactively changed by the user as recording
//       !!! progresses.  Also, we save the current stimulus run information even if it is not actually running
//       !!! because, in typical usage, Maestro will "preload" the stimulus run, then start recording, then start the
//       !!! previously loaded run!  We include a flag that indicates whether or not the currently efined run is
//       !!! actually in progress.
//
//    As of file version 25 (Maestro 5.0.2), stimulus run definitions are no longer written to the data file. The
//    stimulus run feature is rarely if ever used, and the only remaining stimulus channel type available at this
//    point uses the animal chair -- which may not even be available any more AFAIK.
// 
//    6) Compressed spike trace data (record tag 67). [VERSION >= 2] Maestro dedicates a single channel to record the
//    raw electrode signal from which "spikes" (or "units", aka action potentials) are extracted.  The channel is
//    sampled at 25KHz in order to adequately capture each action potential.  "Fast" channel data is recorded at the
//    same time as the "slow-sampled" AI channels (record 0) by a single AI device, and is compressed in the same way.
//
//    7) Analysis action records (tag 5). These are NOT created by Maestro; rather, they are appended to the data file 
//    later by analysis programs like the now-obsolete XWork and its Java-based successor JMWork.
//
//    8) Sorted spike-train records (tag 8-57). These records are NOT created by Maestro, rather they are appended to
//    the data file later by analysis programs like XWork, JMWork, and the Matlab utility editcxdata(). These records 
//    represent interevent intervals for "spikes" detected in high-resolution spike waveforms recorded by Maestro or 
//    the Plexon system. Format is identical to that of spike event records (tag=1). Allows for up to 50 distinct spike
//    trains to be associated with the original recorded data file.
//         [VERSION >= 19, rev 05sep2013] Originally, XWork supported 13 distinct spike trains, with record tags 8-20.
//    JMWork and editcxdata() supported the same. JMWork and editcxdata() have been updated to support up to 50 
//    different sorted-spike trains, with record tags 8..57. Prior to this change, record tags 21-57 were never used.
//         [04jun2021] Modifying JMWork and read/editcxdata() to support up to 200 sorted-spike train channels. To do
//    so, the programs make use of the second byte in the record tag to specify M=0..3, along with N=8..57 in byte 0.
//    The channel # is M*50 + (N-8), which ranges from 0-199.
//    

const BYTE CX_AIRECORD           = 0;                    // record tag for compressed, slow-sampled AI data
const BYTE CX_EVENT0RECORD       = 1;                    // record tag for interevent intervals on DI<0>
const BYTE CX_EVENT1RECORD       = 2;                    // record tag for interevent intervals on DI<1>
const BYTE CX_OTHEREVENTRECORD   = 3;                    // record tag for event info on all other events, DI<15..2>,
                                                         // plus Eyelink "blink start" and "blink end" events.
const BYTE CX_TRIALCODERECORD    = 4;                    // record tag for trial codes
const BYTE CX_XWORKACTIONREC     = 5;                    // record tag reserved for "analysis action" record
const BYTE CX_SPIKESORTREC_FIRST = 8;                    // range of record tags for sorted-spike train channels
const BYTE CX_SPIKESORTREC_LAST  = 57;                   //
const BYTE CX_V1TGTRECORD        = 64;                   // record tag for tgt defn/stim run record for file vers <= 1
const BYTE CX_TGTRECORD          = 65;                   // record tag for target definitions

// [deprecated a/o V=25] record tag for stimulus run definition. Support for writing the stimulus run record to the 
// Maestro data file was removed in Maestro 5.0.2. Cont-mode is rarely used, and now the only available stimulus channel
// type is "Chair", which also may no longer be in use in any current experiment rigs.
// const BYTE CX_STIMRUNRECORD      = 66;

const BYTE CX_SPIKEWAVERECORD    = 67;                   // record tag for compressed, 25KHz-sampled spike trace
const BYTE CX_TAGSECTRECORD      = 68;                   // record tag for trial tagged section record

const int EOD_EVENTRECORD        = 0x7fffffff;           // "end of data" marker for digital event & spike-sorting recs

const DWORD CX_EL_BLINKSTARTMASK = (DWORD) (1<<16);      // special "other event" masks: blink start and blink end on
const DWORD CX_EL_BLINKENDMASK   = (DWORD) (1<<17);      //    Eyelink tracker device

const int CX_RECORDSZ          = 1024;                   // size of a Maestro header or data record, in bytes

const int CX_RECORDBYTES         = 1016;                 // amount of data storable in data rec (not including idTag)
const int CX_RECORDSHORTS        = CX_RECORDBYTES/sizeof(SHORT);
const int CX_RECORDINTS          = CX_RECORDBYTES/sizeof(INT);
const int CX_RECORDCODES         = CX_RECORDBYTES/sizeof(TRIALCODE);
const int CX_RECORDSECTS         = CX_RECORDBYTES/sizeof(TRIALSECT);


typedef struct tagCxFileTgtRec                           // persistent storage format for Maestro target information
{                                                        // in CX_TGTRECORD:
   CXTARGET def;                                         //    the target definition

   DWORD dwState;                                        //    [ContMode only] tgt state (CXFTF_* flags)
   float fPosX, fPosY;                                   //    [ContMode only] tgt pos in deg
} CXFILETGT, *PCXFILETGT;

const int CX_RECORDTARGETS       = CX_RECORDBYTES/sizeof(CXFILETGT);

const DWORD CXFTF_ISACVTGT       = (DWORD) (1<<0);       // flag set for tgts in active list; cleared for XYseq tgts
const DWORD CXFTF_TGTON          = (DWORD) (1<<1);       // active tgt was ON at start of recording
const DWORD CXFTF_TGTISFIX1      = (DWORD) (1<<2);       // active tgt was designated "fixTgt1" at start of recording
const DWORD CXFTF_TGTISFIX2      = (DWORD) (1<<3);       // active tgt was designated "fixTgt2" at start of recording

// [DEPRECATED] target record format prior to version 8: The old VSG framebuffer video card was employed in Maestro 
// versions prior to v2.0, and these deprecated versions of CXTARGET and CXFILETGT use the old FBPARMS structure in 
// which VSG FB video target parameters were defined. They are maintained solely to support backwards compatibility of 
// analysis programs that must handle data files generated by different versions of Maestro.
typedef struct tagCxTarget_V7
{ 
   WORD           wType;
   CHAR           name[CX_MAXOBJNAMELEN];
   U_TGPARMS_OLD  u;
} CXTARGET_V7, *PCXTARGET_V7;

typedef struct tagCxFileTgtRec_V7
{
   CXTARGET_V7 def;
   DWORD dwState;
   float fPosX, fPosY;
} CXFILETGT_V7, *PCXFILETGT_V7;

const int CX_RECORDTARGETS_V7    = CX_RECORDBYTES/sizeof(CXFILETGT_V7);

// [DEPRECATED] target record format for data file versions 8 to 12: As of Maestro v2.5.0 (data file version 13), two 
// char[] fields were added to the RMVTGTDEF structure (rmvideo_common.h) to support the new target type RMV_MOVIE 
// (video playback). These deprecated versions of CXTARGET and CXFILETGT use the previous version of RMVTGTDEF. They are
// maintained solely to support backwards compatibility of analysis programs that must handle data files generated by 
// different versions of Maestro.
typedef struct tagCxTarget_V12
{ 
   WORD           wType;
   CHAR           name[CX_MAXOBJNAMELEN];
   U_TGPARMS_V12  u;
} CXTARGET_V12, *PCXTARGET_V12;

typedef struct tagCxFileTgtRec_V12
{
   CXTARGET_V12 def;
   DWORD dwState;
   float fPosX, fPosY;
} CXFILETGT_V12, *PCXFILETGT_V12;

const int CX_RECORDTARGETS_V12    = CX_RECORDBYTES/sizeof(CXFILETGT_V12);

// [DEPRECATED] target record format for data file versions 13 to 22: As of Maestro v4.1.0 (data file version 23), 
// three int fields were added to the RMVTGTDEF structure (rmvideo_common.h) to define a target's "flicker" parameters.
// These deprecated versions of CXTARGET and CXFILETGT use the version of RMVTGTDEF that applied to data file versions
// 13-22. They are maintained solely to support backwards compatibility of analysis programs that must handle data 
// files generated by different versions of Maestro.
typedef struct tagCxTarget_V22
{
   WORD           wType;
   CHAR           name[CX_MAXOBJNAMELEN];
   U_TGPARMS_V22  u;
} CXTARGET_V22, *PCXTARGET_V22;

typedef struct tagCxFileTgtRec_V22
{
   CXTARGET_V22 def;
   DWORD dwState;
   float fPosX, fPosY;
} CXFILETGT_V22, *PCXFILETGT_V22;

const int CX_RECORDTARGETS_V22 = CX_RECORDBYTES / sizeof(CXFILETGT_V22);


/* [deprecated a/o file version 25, Maestro 5.0.2]
typedef struct tagCxFileStimRunHdr                       // persistent storage format for header information describing
{                                                        // a ContMode stimulus run:
   BOOL     bRunning;                                    //    was stimulus run in progress when recording started?
   int      iDutyPeriod;                                 //    duty period in milliseconds
   int      iDutyPulse;                                  //    DOUT ch# for duty cycle marker pulse (0 = no marker)
   int      nAutoStop;                                   //    auto-stop after this many cycles elapsed (0 = disabled)
   float    fHOffset;                                    //    horizontal position offset in deg subtended at eye
   float    fVOffset;                                    //    vertical position offset in deg subtended at eye
   int      nStimuli;                                    //    # of stimulus channels THAT ARE TURNED ON for this run
   int      nXYTgts;                                     //    #XY scope targets participating in an XYseq stimulus
} CXFILESTIMRUNHDR, *PCXFILESTIMRUNHDR;

typedef union tagCxFileStimRun                           // persistent storage format for Maestro stimulus run defn in
{                                                        // CX_STIMRUNRECORD:
   CXFILESTIMRUNHDR  hdr;                                //    first "chunk" in the record holds the run's hdr info
   STIMCHAN          stim;                               //    all remaining "chunks" are defns of "on" stim channels
} CXFILESTIM_U, *PCXFILESTIM_U;

const int CX_RECORDSTIMS         = CX_RECORDBYTES/sizeof(CXFILESTIM_U);
*/

typedef struct tagCxFileRec                              // generic format for Maestro file data/info records
{
   BYTE  idTag[8];                                       // byte 0 holds the record tag CX_***RECORD. other bytes = 0.
   union                                                 // the data, in various forms...
   {
      BYTE byteData[CX_RECORDBYTES];
      SHORT shData[CX_RECORDSHORTS];
      INT iData[CX_RECORDINTS];
      TRIALCODE tc[CX_RECORDCODES];                      //    for CX_TRIALCODERECORD
      TRIALSECT sects[CX_RECORDSECTS];                   //    for CX_TAGSECTRECORD
      CXFILETGT tgts[CX_RECORDTARGETS];                  //    for CX_TGTRECORD, v >= 23
      CXFILETGT_V7 tgtsV7[CX_RECORDTARGETS_V7];          //    for CX_TGTRECORD, v < 8
      CXFILETGT_V12 tgtsV12[CX_RECORDTARGETS_V12];       //    for CX_TGTRECORD, v = [8..12]
      CXFILETGT_V22 tgtsV22[CX_RECORDTARGETS_V22];       //    for CX_TGTRECORD, v = [13..22]
      // CXFILESTIM_U stims[CX_RECORDSTIMS];             //    [deprecated a/o v=25] for CX_STIMRUNRECORD
   } u;
} CXFILEREC, *PCXFILEREC;

#endif   // !defined(CXFILEFMT_H__INCLUDED_)



