//=====================================================================================================================
//
// cxfilefmt_mex.h :  Data structs defining format of data records in Maestro/Cntrlx trial- and cont-mode data files.
//
// AUTHOR:  saruffner, sglisberger, et al.
//
// DESCRIPTION:
// This is a modified version of the Maestro source code header file CXFILEFMT.H.  It is required to build the MATLAB
// MEX function readcxdata(), which is used to read Maestro and Cntrlx data files into the MATLAB environment.
// MATLAB's MEX tool uses the GNU CC compiler; while GCC can handle C++ source code files, MEX handles only C files.
//
// It is the "const <type> IDENTIFIER = <value>" syntax that caused problems with the MEX build.  While such a constant
// definition is acceptable when <value> is a constant, it introduces several problems down the line:
//    1) The compiler cannot handle "const <type> IDENTIFIER = <constant identifier>" when <constant identifier> was
//       defined using the above syntax.  Here we must replace <constant identifer> with a constant literal or a
//       #define'd constant name.
//    2) Any constant defined using the above syntax CANNOT be used to indicate the dimensions of an array declared
//       outside a function.  Nor can they be used in "case <constant>" expressions within a "switch" clause.  We must
//       define these constants using the #define construct.
//
// Thus, I've replaced all "const <type> IDENTIFIER = <value>" constructs with "#define IDENTIFIER <value>" throughout
// this file.  I also commented out an #if-endif preprocessor clause specific to MS VC++.
//
// See also:  CXFILEFMT.H in Maestro development folder.  This includes a complete version history for data files
// created by Maestro and its earlier incarnation, Cntrlx.
//
//
// REVISION HISTORY:
// 28mar2003-- Modified from the original CXFILEFMT.H dtd 27mar2003.  Keep in synch w/ future changes!
// 19nov2003-- Updated to synch with changes to CXFILEFMT.H dtd 19nov2003 (a field added to header record).
// 26jan2005-- Updated to synch with changes to CXFILEFMT.H dtd 25jan2005.  No impact (just added some comments).
// 07apr2005-- Updated to synch with changes to CXFILEFMT.H dtd 06apr2005, related to the introduction of "tagged
//             sections" in trials.
// 29jul2005-- Updated to synch with version# change in CXFILEFMT.H dtd 29jul2005.  No format changes -- version
//             change marks changes in implementation of noise perturbations so that they can be reproduced by
//             readcxdata().
// 25jan2006-- Updated to synch with version# change in CXFILEFMT.H dtd 25jan2006.  Several new CXHF_ bit flags and
//             header information fields added to provide info relevant to the new "R/P Distro" feature.
// 18mar2006-- Updated to synch with version# change in CXFILEFMT.H for Maestro v1.5.0.  No format changes -- version
//             change marks change in TARGET_HOPEN trial code group to support velocity stabilization over a contiguous
//             span of trial segments instead of just one.
// 18apr2006-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.0.0 (data file version = 8).
//             Includes a significant change in the format of CX_TGTRECORD -- so we must check file version and use
//             the appropriate data structure when parsing target records in the file.  Also, new trial code
//             TARGET_VSTAB replaces TARGET_HOPEN; velocity stabilization can now occur on a per-target, per-segment
//             basis.
// 19jun2006-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.0.1 (data file version = 9).  Only 
//             change is the addition of fields 'fInnerX' and 'fInnerY' to XYPARM struct, defining offset of inner 
//             "hole" WRT to center of an XY scope RECTANNU target.  Size of enclosing union U_TGPARMS is *unchanged*.
// 16may2007-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.1.1 (data file version = 10). Added 
//             CXFILEHDR.iRPDRespType, which indicates the type of behavioral response measured during an R/P Distro 
//             trial (4 different measures are now supported, as defined by TH_RPD_** constants in cxobj_ifc.h).
// 16jul2007-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.1.2 (data file version = 11). No 
//             format changes. Marks introduction of support for perturbing trial target window or pattern speed, and 
//             a minor implementation change for the XYScope NOISYSPEED target: speed noise offset granularity is now 
//             1% instead of 0.1%.
// 06sep2007-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.1.3 (data file version = 12). No 
//             format changes. Marks introduction of support for simultaneous window/pattern speed or direction perts
//             PERT_ON_SPD, _ON_DIR in cxobj_ifc.h), and for multiplicative speed noise in XYScope NOISYSPEED tgt 
//             and RMVideo RMV_RANDOMDOTS tgt. Also, additive speed noise granularity is now 1% instead of 0.1% for the 
//             RMV_RANDOMDOTS tgt to bring it in line with NOISYSPEED.
// 17sep2009-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.5.0 (data file version = 13). The
//             RMVTGTDEF structure was altered to include new fields char[32] strFolder and char[] strFile, for the new
//             RMV_MOVIE target. Additional RMVideo target flags also defined, including RMV_F_ORIENTADJ for the
//             RMV_GRATING target. When this flag is set, the target does NOT ignore the V cmpt of pattern velocity 
//             during animation. The grating orientation adjusts frame-by-frame so that it is always perpendicular to
//             the pattern velocity vector.
// 20jan2010-- Updated to synch with version# change in CXFILEFMT.H for Maestro v2.5.2 (data file version = 14). No 
// format changes. Introduced new flag RMV_F_WRTSCREEN, which affects behavior of the RMVideo RMV_RANDOMDOTS target 
// only. If set, target pattern motion is specified in global (screen) frame of reference rather than WRT the target
// window. This change permitted a better emulation of the XYScope NOISYDIR and NOISYSPEED targets in the case where 
// target window and dots were moving in the same direction at the same speed (ie, the dots are not moving WRT the 
// target window). In addition, relaxed restrictions to permit velocity stabilization of all RMVideo and XYScope tgts.
// 29apr2010-- Updated to synch with changes in CXFILEFMT.H for Maestro v2.6.0 (data file version = 15). Added fields
// CXFILEHDR.iStartPosH, .iStartPosV. Also, increased max # dots for RMVideo RMV_RANDOMDOTS and RMV_FLOWFIELD targets
// from 1000 to 9999 (in rmvideo_common.h).
// 24may2010-- Update to synch with changes in CXFILEFMT.H for Maestro v2.6.1 (data file version = 16). Added header
// field CXFILEHDR.dwTrialFlags.
// 09mar2011-- Update to synch with changes in CXFILEFMT.H for Maestro v2.6.5 (data file version = 17). Added header
// field CXFILEHDR.iSTSelected and several header flags related to the new Trial-mode "searchTask" feature.
// 17may2011-- Update to synch with changes in CXFILEFMT.H for Maestro v2.7.0 (data file version = 18). Added header
// field CXFILEHDR.iVStabWinLen that indicates length of sliding window average of eye pos for VStab.
// 11may2012-- Update to synch with changes in CXFILEFMT.H for Maestro v3.0.0 (data file version = 19). No format 
// changes; this merely marks the release of Maestro 3, which runs on Windows 7/RTX 2011.
// 06sep2013-- Update to synch with changes in CXFILEFMT.H for Maestro v3.0.3 (data file version = 19, unchanged).
// Changed constant CX_SPIKESORTREC_LAST from 20 to 57, plus some relevant comments
// 24nov2015-- Update to synch with changes in CXFILEFMT.H for Maestro v3.2.0 (data file version = 20). This Maestro
// version introduces support for the Eyelink eye tracker, and the data file includes some Eyelink-specific info: a
// new header flag CXHF_EYELINKUSED, a header field iELInfo[], plus "blink start" and "blink end" events that are 
// stored in the same record type as digital pulses on DI<2..15>. The event masks for these blink events do not overlap
// with the digital input events, and event times are in ms rather than 10us ticks.
// 12jun2018—- Update to sync with changes in CXFILEFMT.H for Maestro v4.0.0 (data file version = 21). In addition to
// marking the release of 4.0, which migrates Maestro to 64-bit Win10/RTX64, two fields were added to the file header,
// CXFILEHDR.setName and CXFILEHDR.subsetName.
// 01oct2018-- Update to sync with additional changes in CXFILEFMT.H for Maestro v4.0.0, dtd 01oct2018. Two more fields
// were added: rmvSyncSz and rmvSyncDur.
// 04dec2018-- Update to sync with additional changes in CXFILEFMT.H for Maestro v4.0.1, dtd 04dec2018. Added the
// timestampMS field.
// 27mar2019-- Update to sync with changes in CXFILEFMT.H for Maestro v4.0.5 (date file version = 22) Added the 
// rmvDupEvents[CXH_RMVDUPEVTSZ] field, the CXHF_DUPFRAME flag, and noted the change in the multiplier for the 
// d_framerate field.
// 13may2019-- Update to sync with changes in CXFILEFMT.H for Maestro v4.1.0 (data file version = 23). The RMVTGTDEF
// structure was altered to include 3 new int fields related to target "flicker".
// 04jun2021 -- Modifying JMWork and read/editcxdata to support up to 200 (instead of 50) sorted spike train channels.
// This does NOT impact Maestro, as the sorted spike train records are added to the data file by the post-analysis
// software. Merely added comments in this file to document the change.
// 05nov2024 -- Update to sync with changes in CXFILEFMT.H for Maestro v4.2.0 (data file version = 24). Added header 
// flag CXHF_ST_2GOAL. NOTE that the XYScope display was dropped entirely for Maestro 5.0 (Oct 2024), but there were
// no changes to CXFILEFMT.H, and we left the data file version at 24.
// 19nov2024 -- Update to sync with changes in CXFILEFMT.H for Maestro v5.0.2 (data file version = 25). As of that
// version, Maestro no longer writes stimulus run records (id = CX_STIMRUNRECORD). The typedefs for CX_FILESTIM_U
// and CXFILESTIMRUNHDR have been REMOVED. ***Because READCXDATA was never able to report stimulus run definitions in
// its output, I elected to remove those typedefs here as well. READCXDATA will be modified to simply skip any data
// file record with id==CX_STIMRUNRECORD, regardless the data file version.***
// 18dec2024 -- Update to sync with additional changes in CXFILEFMT.H for Maestro 5.0.2; new param RMVTGTDEF.fDotDisp,
// affecting format of target definition record.
// 
//=====================================================================================================================

#if !defined(CXFILEFMT_MEX_H__INCLUDED_)
#define CXFILEFMT_MEX_H__INCLUDED_

#include "cxtrialcodes_mex.h"                         // definition of TRIALCODE structure
#include "cxobj_ifc_mex.h"                            // defns of CXTARGET and STIMCHAN structures


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

#define CXH_NAME_SZ              40                   // max length of names in header, including terminating null char
#define CXH_MAXAI                16                   // max # of AI channels that can be recorded
#define CXH_EXTRAS               308                  // # of unused shorts in header record
#define CXH_CURRENTVERSION       25                   // the current version # (as of Maestro version 5.0.2)

#define CXH_RMVDUPEVTSZ          6                    // [V>=22] array size for RMVideo duplicate frame event info

                                                      // currently defined header flag bits:
#define CXHF_ISCONTINUOUS        ((DWORD) (1<<0))     //    if set, file was collected in cont mode; else, trial mode
#define CXHF_SAVEDSPIKES         ((DWORD) (1<<1))     //    if set, 25KHz spike trace saved during this trial
#define CXHF_REWARDEARNED        ((DWORD) (1<<2))     //    [T] if set, subject did not break fixation during trial
#define CXHF_REWARDGIVEN         ((DWORD) (1<<3))     //    [T] if set, the earned reward was actually delivered
                                                      //       (reward may be randomly withheld).
#define CXHF_FIX1SELECTED        ((DWORD) (1<<4))     //    [T] if set, tgt#1 was selected in a trial's "selByFix*" op
                                                      //    or tgt#1 was INITIALLY selected in a "switchFix" op.
#define CXHF_FIX2SELECTED        ((DWORD) (1<<5))     //    [T] if set, tgt#2 was selected in a trial's "selByFix*" op
                                                      //    or tgt#2 was INITIALLY selected in a "switchFix" op.
#define CXHF_ENDSELECT           ((DWORD) (1<<6))     //    [T] if set, selection forced at end of "selByFix" segment
#define CXHF_HASTAGSECTS         ((DWORD) (1<<7))     //    [T] if set, trial has one or more tagged sections.  Data
                                                      //    file should include a CX_TAGSECTRECORD.
#define CXHF_ISRPDISTRO          ((DWORD) (1<<8))     //    [T, V>=6] if set, trial used the "R/P Distro" op.
#define CXHF_GOTRPDRESP          ((DWORD) (1<<9))     //    [T, V>=6] if set, then trial got past "R/P Distro" segment

#define CXHF_ISSEARCHTSK         ((DWORD) (1<<10))    //    [T, V>=17] if set, trial used the "searchTask" op.
#define CXHF_ST_OK               ((DWORD) (1<<11))    //    [T, V>=17] "searchTask" result: goal tgt selected,  
#define CXHF_ST_DISTRACTED       ((DWORD) (1<<12))    //    distractor selected, or no tgt selected.
#define CXHF_EYELINKUSED         ((DWORD) (1<<13))    //    [V>=20] if set, Eyelink tracker used to monitor eye traj
#define CXHF_DUPFRAME            ((DWORD) (1<<14))    //    [V>=22] if set, RMVideo detected one or more repeat frames
#define CXHF_ST_2GOAL            ((DWORD) (1<<15))    //    [V>=24] if set, trial performed 2-goal "searchTask" op.

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
   int iRPDResponse;                // [V>=6, T] avg eye vel magnitude during R/P Distro segment, in 0.001 deg/sec
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
//    include the old structure definitions CXTARGET_V12 and CXFILETGT_V12. These definitions depend on deprecated types 
//    RMVTGTDEF_V12 (located in rmvideo_common.h) and U_TGPARMS_V12 (in cxobj_ifc.h).
//
//       [VERSION >= 23] Introduced new "flicker" feature applicable to all RMVideo target types. Required introduction
//    of 3 new integer fields to the RMVideo target definition struct RMVTGTDEF, triggering changes to U_TGPARMS, 
//    CXTARGET, and CXFILETGT. To accommodate parsing of target records from data files with v=[13..22], we include
//    the old structure definitions CXTARGET_V22 and CXFILETGT_V22. These definitions depend on deprecated types 
//    RMVTGTDEF_V22 (located in rmvideo_common.h) and U_TGPARMS_V22 (in cxobj_ifc.h).
//
//       [VERSION >= 25] Introduced new "stereo disparity" feature for the RMV_POINT, RMV_RANDOMDOTS and RMV_FLOWFIELD
//    target types. Required introduction one new field, RMVTGTDEF.fDotDisp, triggering changes to U_TGPARMS, 
//    CXTARGET, and CXFILETGT. To accommodate parsing of target records from data files with v=[23..24], we include
//    the old structure definitions CXTARGET_V24 and CXFILETGT_V24. These definitions depend on deprecated structures 
//    RMVTGTDEF_V24 (located in rmvideo_common.h) and U_TGPARMS_V24 (in cxobj_ifc.h).
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

#define CX_AIRECORD              ((BYTE) 0)           // record tag for compressed, slow-sampled AI data
#define CX_EVENT0RECORD          ((BYTE) 1)           // record tag for interevent intervals on DI<0>
#define CX_EVENT1RECORD          ((BYTE) 2)           // record tag for interevent intervals on DI<1>
#define CX_OTHEREVENTRECORD      ((BYTE) 3)           // record tag event info on all other events, DI<15..2>
#define CX_TRIALCODERECORD       ((BYTE) 4)           // record tag for trial codes
#define CX_XWORKACTIONREC        ((BYTE) 5)           // record tag reserved for "analysis action" record
#define CX_SPIKESORTREC_FIRST    ((BYTE) 8)           // range of record tags for sorted-spike train channels
#define CX_SPIKESORTREC_LAST     ((BYTE)57)           //
#define CX_V1TGTRECORD           ((BYTE)64)           // record tag for tgt defn/stim run record for file vers <= 1
#define CX_TGTRECORD             ((BYTE)65)           // record tag for target definitions

// [deprecated a/o V=25] record tag for stimulus run definition. Support for writing the stimulus run record to the 
// Maestro data file was removed in Maestro 5.0.2. Cont-mode is rarely used, and now the only available stimulus channel
// type is "Chair", which also may no longer be in use in any current experiment rigs.
// While no longer defined in Maestro code base, it is still defined here so that READCXDATA can detect and merely
// SKIP over all such records in the data file. READCXDATA has never been able to report stimulus run definitions in
// its output, and stimulus runs are a rarely-if-ever used feature of Maestro...
#define CX_STIMRUNRECORD         ((BYTE)66)           // record tag for stimulus run definition

#define CX_SPIKEWAVERECORD       ((BYTE)67)           // record tag for compressed, 25KHz-sampled spike trace
#define CX_TAGSECTRECORD         ((BYTE)68)           // record tag for trial tagged section record

#define CX_EL_BLINKSTARTMASK     ((DWORD)(1<<16))     // special "other event" masks: blink start and blink end on
#define CX_EL_BLINKENDMASK       ((DWORD)(1<<17))     //    Eyelink tracker device

#define EOD_EVENTRECORD          0x7fffffff           // "end of data" marker for digital event & spike-sorting records

#define CX_RECORDBYTES           1016                 // amount of data storable in data rec (not including idTag)
#define CX_RECORDSHORTS          (CX_RECORDBYTES/sizeof(SHORT))
#define CX_RECORDINTS            (CX_RECORDBYTES/sizeof(INT))
#define CX_RECORDCODES           (CX_RECORDBYTES/sizeof(TRIALCODE))
#define CX_RECORDSECTS           (CX_RECORDBYTES/sizeof(TRIALSECT))


typedef struct tagCxFileTgtRec                        // persistent storage format for Maestro target information
{                                                     // in CX_TGTRECORD:
   CXTARGET def;                                      //    the target definition

   DWORD dwState;                                     //    [ContMode only] tgt state (CXFTF_* flags)
   float fPosX, fPosY;                                //    [ContMode only] tgt pos in deg
} CXFILETGT, *PCXFILETGT;

#define CX_RECORDTARGETS         (CX_RECORDBYTES/sizeof(CXFILETGT))

#define CXFTF_ISACVTGT           ((DWORD) (1<<0))     // flag set for tgts in active list; cleared for XYseq tgts
#define CXFTF_TGTON              ((DWORD) (1<<1))     // active tgt was ON at start of recording
#define CXFTF_TGTISFIX1          ((DWORD) (1<<2))     // active tgt was designated "fixTgt1" at start of recording
#define CXFTF_TGTISFIX2          ((DWORD) (1<<3))     // active tgt was designated "fixTgt2" at start of recording

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

#define CX_RECORDTARGETS_V7      (CX_RECORDBYTES/sizeof(CXFILETGT_V7))

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

#define CX_RECORDTARGETS_V12    (CX_RECORDBYTES/sizeof(CXFILETGT_V12))

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

#define CX_RECORDTARGETS_V22    (CX_RECORDBYTES/sizeof(CXFILETGT_V22))

// [DEPRECATED] target record format for data file versions 23 to 24: As of Maestro v5.0.2 (data file version 25), 
// one field was added to the RMVTGTDEF structure (rmvideo_common.h) to define "dot disparity" for stereo experiments
// using RMV_POINT, RMV_RANDOMDOTS, and RMV_FLOWFIELD. These deprecated versions of CXTARGET and CXFILETGT use the 
// version of RMVTGTDEF that applied to data file versions 23-24. They are maintained solely to support backwards 
// compatibility of analysis programs that must handle data  files generated by different versions of Maestro.
typedef struct tagCxTarget_V24
{
   WORD           wType;
   CHAR           name[CX_MAXOBJNAMELEN];
   U_TGPARMS_V24  u;
} CXTARGET_V24, * PCXTARGET_V24;

typedef struct tagCxFileTgtRec_V24
{
   CXTARGET_V24 def;
   DWORD dwState;
   float fPosX, fPosY;
} CXFILETGT_V24, * PCXFILETGT_V24;

const int CX_RECORDTARGETS_V24 = CX_RECORDBYTES / sizeof(CXFILETGT_V24);


/* [deprecated a/o file version 25, Maestro 5.0.2]
typedef struct tagCxFileStimRunHdr                    // persistent storage format for header information describing
{                                                     // a ContMode stimulus run:
   BOOL     bRunning;                                 //    was stimulus run in progress when recording started?
   int      iDutyPeriod;                              //    duty period in milliseconds
   int      iDutyPulse;                               //    DOUT ch# for duty cycle marker pulse (0 = no marker)
   int      nAutoStop;                                //    auto-stop after this many cycles elapsed (0 = disabled)
   float    fHOffset;                                 //    horizontal position offset in deg subtended at eye
   float    fVOffset;                                 //    vertical position offset in deg subtended at eye
   int      nStimuli;                                 //    # of stimulus channels THAT ARE TURNED ON for this run
   int      nXYTgts;                                  //    #XY scope targets participating in an XYseq stimulus
} CXFILESTIMRUNHDR, *PCXFILESTIMRUNHDR;

typedef union tagCxFileStimRun                        // persistent storage format for Maestro stimulus run defn in
{                                                     // CX_STIMRUNRECORD:
   CXFILESTIMRUNHDR  hdr;                             //    first "chunk" in the record holds the run's hdr info
   STIMCHAN          stim;                            //    all remaining "chunks" are defns of "on" stim channels
} CXFILESTIM_U, *PCXFILESTIM_U;

#define CX_RECORDSTIMS           (CX_RECORDBYTES/sizeof(CXFILESTIM_U))
*/

typedef struct tagCxFileRec                           // generic format for Maestro file data/info records
{
   BYTE  idTag[8];                                    // byte 0 holds the record tag CX_***RECORD. other bytes = 0.
   union                                              // the data, in various forms...
   {
      BYTE byteData[CX_RECORDBYTES];
      SHORT shData[CX_RECORDSHORTS];
      INT iData[CX_RECORDINTS];
      TRIALCODE tc[CX_RECORDCODES];                   //    for CX_TRIALCODERECORD
      TRIALSECT sects[CX_RECORDSECTS];                //    for CX_TAGSECTRECORD
      CXFILETGT tgts[CX_RECORDTARGETS];               //    for CX_TGTRECORD, v >= 25
      CXFILETGT_V7 tgtsV7[CX_RECORDTARGETS_V7];       //    for CX_TGTRECORD, v < 8
      CXFILETGT_V12 tgtsV12[CX_RECORDTARGETS_V12];    //    for CX_TGTRECORD, v = [8..12]
      CXFILETGT_V22 tgtsV22[CX_RECORDTARGETS_V22];    //    for CX_TGTRECORD, v = [13..22]
      CXFILETGT_V24 tgtsV24[CX_RECORDTARGETS_V24];    //    for CX_TGTRECORD, v = [23..24]
      // CXFILESTIM_U stims[CX_RECORDSTIMS];          //    [deprecated a/o v=25] for CX_STIMRUNRECORD
   } u;
} CXFILEREC, *PCXFILEREC;

#endif   // !defined(CXFILEFMT_MEX_H__INCLUDED_)



