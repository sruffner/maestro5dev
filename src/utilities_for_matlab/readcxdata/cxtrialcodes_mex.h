//=====================================================================================================================
//
// cxtrialcodes_mex.h :  Format of trial codes defining the execution of a trial in MAESTRODRIVER
//
// AUTHOR:  saruffner, sglisberger, et al.
//
// DESCRIPTION:
// This is a modified version of the CNTRLX/CXDRIVER source code header file CXTRIALCODES.H.  It is required to build
// the MATLAB MEX function readcxdata(), which is used to read CNTRLX data files into the MATLAB environment.  MATLAB's
// MEX tool uses the GNU CC compiler; while GCC can handle C++ source code files, MEX handles only C files.
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
// this file.
//
// See also:  CXTRIALCODES.H in CXDRIVER development folder.
//
//
// REVISION HISTORY:
// 28mar2003-- Modified from the original CXTRIALCODES.H dtd 25mar2003.  Keep in synch w/ future changes!
// 03nov2004-- Modified IAW same-dtd changes in CXTRIALCODES.H.  Changes do not impact READCXDATA or EDITCXDATTA.
// 26jan2005-- Modified IAW changed in CXTRIALCODES.H dtd 25jan2005.  Changes DO impact trial code processing in
//             READCXDATA.  EDITCXDATA unaffected.
// 29jul2005-- Modified IAW changes in CXTRIALCODES.H dtd 29jul2005, re: content of TARGET_PERTURB code group.  Changes
//             will now permit reproduction of target trajectories perturbed by noise.  As of Maestro v1.3.2.
// 03jan2006-- Modified IAW changes in CXTRIALCODES.H dtd 06dec2005, re: new trial code group RPDWINDOW and new special
//             op code SPECIAL_RPDISTRO, in support of response distribution-based reward/penalty protocol.  As of
//             Maestro v1.4.0.
// 18mar2006-- Modified IAW changes in CXTRIALCODES.H dtd 13mar2006, re: TARGET_HOPEN.  Comments only.
// 18apr2006-- Modified IAW changes in CXTRIALCODES.H dtd 10apr2006, re: new trial code TARGET_VSTAB.
// 27feb2007-- Modified IAW changes in CXTRIALCODES.H dtd 04jan2007, re: new special op codes SPECIAL_CHOOSEFIX*.
// 27feb2007-- Modified IAW changes in CXTRIALCODES.h dtd 27feb2007, re: INSIDE_***ACC trial codes.
// 16may2007-- Modified IAW changes in CXTRIALCODES.h dtd 25apr2007, re: new usage for SPECIALOP codes pertaining to 
//             the SPECIAL_RPDISTRO optype.
// 06sep2013-- Modified IAW changes in CXTRIALCODES.h dtd 01feb2011 and 05sep2013, re: special op code SPECIAL_SEARCH,
// and new constant MAXTRIALDUR.
// 01oct2018-- Modified IAW changes in CXTRIALCODES.h dtd 24sep2018 -- comments re: usage of PULSE_ON code to trigger
// RMVideo "vertical sync" flash at the start of a trial segment
// 13may2019-- Modified IAW change in CXTRIALCODES.h dtd 30apr2019 -- need to enclose definition of VSTAB_MASK in 
// parentheses!
//=====================================================================================================================

#if !defined(CXTRIALCODES_MEX_H__INCLUDED_)
#define CXTRIALCODES_MEX_H__INCLUDED_


typedef struct tagTrialCode            // one trial code block contains:
{
   short code;                         //    [block 0] the trial code; [other blocks] usage varies.
   short time;                         //    [block 0] trial time; [other blocks] usage varies.
} TRIALCODE, *PTRIALCODE;


// scale factors used in trial-code processing to encode floating-point values as short integers:
#define d_TC_STDSCALE            10.0
#define d_TC_SLOSCALE1           500.0
#define d_TC_SLOSCALE2           100.0

// trial duration must be less than max value of a short integer b/c trial codes store elapsed time with short ints!
#define MAXTRIALDUR 32760

// Existing MAESTRO trial codes and their format.  LEGEND:
//    purpose of trial code (N = #blocks)
//    code1 = <description> --> contents of additional code blocks <1..N-1> described
//    time1 = <description>
//    ...
//
// Note that "target#" refers to the ordinal pos of the target's definition in the "trial target map" in IPC memory!
// That map, in turn, points to the target's actual definition in the "loaded target list", which also resides in IPC.
//
#define     TARGET_ON            1     // turn specified target on  (N=2); code1 = target#; time1 = notUsed
#define     TARGET_OFF           2     // turn specified target off (N=2); code1 = target#; time1 = notUsed
#define     TARGET_HVEL          3     // change target's horiz velocity (N=2)
                                       //    code1 = target#; time1 = (new velocity in deg/sec) * 10
#define     TARGET_VVEL          4     // change target's verti velocity (N=2)
                                       //    code1 = target#; time1 = (new velocity in deg/sec) * 10
#define     TARGET_HPOSREL       5     // add specified delta to target's horiz pos (N=2)
                                       //    code1 = target#; time1 = (pos change in deg) * 100
#define     TARGET_VPOSREL       6     // add specified delta to target's verti pos (N=2)
                                       //    code1 = target#; time1 = (pos change in deg) * 100
#define     TARGET_HPOSABS       7     // change target's horiz coord to specified value (N=2)
                                       //    code1 = target#; time1 = (new absolute coord in deg) * 100
#define     TARGET_VPOSABS       8     // change target's verti coord to specified value (N=2)
                                       //    code1 = target#; time1 = (new absolute coord in deg) * 100

#define     ADCON                10    // start saving ADC channel data & recording timer events (N=1)
#define     ADCOFF               11    // stop saving ADC channel data & recording timer events (N=1) NO LONGER USED

#define     FIXEYE1              12    // change fixation tgt#1 (N=2)
                                       //    code1 = target# of selected fixation tgt
                                       //    time1 = nonzero value enables periodic rewards during the segment!
#define     FIXEYE2              13    // change fixation tgt#2 (N=2)
                                       //    code1 = target# of selected fixation tgt; time1 = notUsed
#define     FIXACCURACY          14    // change behavioral fixation accuracy (N=2)
                                       //    code1 = (new H fixation accuracy in deg) * 100;
                                       //    time1 = (new V fixation accuracy in deg) * 100;

#define     PULSE_ON             16    // turn on specified pulse for one scan period, and/or RMVideo sync flash (N=2)
                                       //    code1 = VSYNCPULSE (NO LONGER USED), or which timer digital output line
                                       //            should be pulsed (1 to 11)
                                       //    time1 = if nonzero, trigger RMVideo vertical sync spot flash
#define     VSYNCPULSE           32    //

#define     TARGET_HACC          18    // change target's horiz acceleration (N=2)
                                       //    code1 = target#; time1 = new acceleration in deg/sec^2
#define     TARGET_VACC          19    // change target's verti acceleration (N=2)
                                       //    code1 = target#; time1 = new acceleration in deg/sec^2

#define     TARGET_PERTURB       20    // apply velocity/directional perturbation waveform to a trial target (N=5).
                                       //    code1 = target#; time1 = (affected traj cmpt << 4) | pert type
                                       //    code2 = pert amplitude * 10 ; time2 = duration in ms
// The "affected traj cmpt" is one of the PERT_ON_* constants in CXOBJ_IFC.H, while "pert type" is one of the PERT_IS*
// constants.  Note that the perturbation's duration can be longer than the segment in which it starts!  The remaining
// (code,time) pairs in this code group are the defining parameters for the perturbation, as listed below.
//
// PERT_ISSINE:  code3 = period in ms; time3 = phase in deg/100. code4,time4 = not used.
// PERT_ISTRAIN: code3 = pulse dur in ms; time3 = ramp dur in ms; code4 = pulse interval in ms; time4 = not used.
// PERT_ISNOISE: code3 = update interval in ms; time3 = mean * 1000 (range [-1000..1000]); code4 = HIWORD(seed);
//               time4 = LOWORD(seed).
// PERT_ISGAUSS: same as for PERT_ISNOISE.
//

#define     TARGET_HOPEN         21    // start velocity stabilization on fix tgt #1 at specified time (N=2)
                                       //    code1 = # of contiguous segments over which vel stab is in effect
                                       //    time1, bit0 = 0 for "OPEN" mode, 1 for "OPN2" mode (same as "OPEN", except
                                       //       tgt does not "snap" to eye at start of open-loop seg).
                                       //    time1, bit2..1 = 01b to stabilize H cmpt only, 10b to stabilize V cmpt
                                       //       only; otherwise, both cmpts are stabilized
                                       // NOTE:  Prior to 13mar06, code1 held tgt ID, but that usage has been obsolete
                                       // since Maestro first came out b/c v. stab. restricted to fix tgt #1.
                                       // OBSOLETE AS OF MAESTRO V2.0.0.  REPLACED by TARGET_VSTAB
#define     OPENMODE_MASK        (1<<0)
#define     OPENMODE_SNAP        0
#define     OPENMODE_NOSNAP      1
#define     OPENENA_MASK         (0x03<<1)
#define     OPENENA_HONLY        2
#define     OPENENA_VONLY        4

#define     TARGET_HSLOVEL       27    // analogous to TARGET_HVEL, but time1 = (new velocity in deg/sec) * 500
#define     TARGET_VSLOVEL       28    // analogous to TARGET_VVEL, but time1 = (new velocity in deg/sec) * 500
#define     TARGET_HSLOACC       29    // analogous to TARGET_HACC, but time1 = (new accel in deg/sec^2) * 100
#define     TARGET_VSLOACC       30    // analogous to TARGET_VACC, but time1 = (new accel in deg/sec^2) * 100

#define     DELTAT               36    // change XY scope update interval (N=2)
                                       //    code1 = new update interval (ms); time1 = notUsed
#define     XYTARGETUSED         38    // specifies that a given target # will appear on XY scope (N=2)
                                       //    code1 = target#;
                                       //    time1 = 0 or #interleaved XY tgts in trial if tgt is to be interleaved

#define     INSIDE_HVEL          39    // change target horizontal pattern vel (N=2)
                                       //    code1 = target#; time1 = (new pattern velocity in deg/sec) * 10
#define     INSIDE_VVEL          40    // analogous to INSIDE_HVEL
#define     INSIDE_HSLOVEL       41    // analogous to INSIDE_HVEL, but time1 = (new velocity in deg/sec) * 500
#define     INSIDE_VSLOVEL       42    // analogous to INSIDE_HSLOVEL

                                       // [Effective Maestro v2.1.0, mar 2007:]
#define     INSIDE_HACC          45    // change target horizontal pattern acceleration (N=2)
                                       //    code1 = target#; time1 = new pattern acceleration in deg/sec^2
#define     INSIDE_VACC          46    // analogous to INSIDE_HACC
#define     INSIDE_HSLOACC       47    // analogous to INSIDE_HACC, but time1 = (new pat acc in deg/sec^2) * 100
#define     INSIDE_VSLOACC       48    // analogous to INSIDE_HSLOACC

// [25apr2007] Effective Maestro v2.1.1, the SPECIAL_RPDISTRO feature supports four alternative behavioral 
// response types (TH_RPD_*** in cxobj_ifc.h). To communicate the response type to MaestroDRIVER, the response 
// type ID [0..3] is left-shifted 8 bits and bitwise-OR'd into 'code1'.
#define     SPECIALOP            60    // perform special, saccade-triggered op during segment (N=2)
                                       //    code1 = optype, IF optype != SPECIAL_RPDISTRO (see below); ELSE
                                       //          = SPECIAL_RPDISTRO | (rpdRespType << 8), where rpdRespType 
                                       //            is the type of behavioral response to measure
                                       //    time1 = saccade threshold velocity in deg/sec
#define     SPECIAL_SKIP         1     //    optype = "skip on saccade"
#define     SPECIAL_FIX          2     //    optype = "select by fixation"
#define     SPECIAL_FIX2         3     //    optype = "select by fixation, version 2"
#define     SPECIAL_SWITCHFIX    4     //    optype = "switch fix"
#define     SPECIAL_RPDISTRO     5     //    [05dec2005] optype = "R/P Distro"
#define     SPECIAL_CHOOSEFIX1   6     //    [04jan2007] optype = "choose fixation tgt #1"
#define     SPECIAL_CHOOSEFIX2   7     //    [04jan2007] optype = "choose fixation tgt #2"
#define     SPECIAL_SEARCH       8     //    [01feb2011] optype = "search task"

#define     REWARDLEN            61    // reward pulse lengths; always sent at time0 = 0 (N=2)
                                       //    code1 = pulse length in msec; if SPECIAL_FIX trial, this pulse length
                                       //            applies when the subject "selects" the first fixation target. for
                                       //            all other trials, this sets the reward pulse length if fixation is
                                       //            maintained over the entire trial.
                                       //    time1 = 2nd pulse length in msec; applies to SPECIAL_FIX trial only --
                                       //            reward pulse of this length is given when the subject "selects"
                                       //            second fixation target.  for all other trials, this is ignored.

#define     PSGM_TC              62    // defining params for SGM electrical pulse stimulus seq (N=6)
                                       //    code1 = op mode; time1 = external trig (1) or s/w start (0).
                                       //    code2 = pulse 1 amplitude; time2 = pulse 2 amplitude.
                                       //    code3 = pulse 1 width; time3 = pulse 2 width.
                                       //    code4 = interpulse interval; time4 = intertrain interval.
                                       //    code5 = #pulses per train; time5 = #trains per sequence.
                                       // NOTE:  all params are sent regardless of mode, even though not all apply to
                                       // all modes.  params sent in non-encoded format (see SGMPARMS in cxobj_ifc.h)

#define     CHECKRESPON          63    // begin checking subject's response (N=2) [staircase sequences only]
                                       //    code1 = ADC channel # to monitor for correct response
                                       //    time1 = ADC channel # to monitor for incorrect response
#define     CHECKRESPOFF         64    // stop checking subject's response (N=1) [staircase sequences only]

#define     FAILSAFE             65    // set "failsafe" time (N=1).  if trial stops before this time, the collected
                                       // data from the trial is discarded.

#define     MIDTRIALREW          66    // [25jan2005] mid-trial reward parameters; always sent at time0 = 0 (N=2)
                                       //    code1: If <=0, use "atSegEnd" mode; rewards delivered at end of enabled
                                       //           segments.  Else use "periodic" mode, in which case code1 is the
                                       //           reward interval in ms.
                                       //    time1: Midtrial reward pulse length in ms.

// This trial code is always sent after the SPECIALOP code for a trial that uses the "R/P Distro" operation and has at
// least one defined behavioral response window.  It sets the bounds for up to two such windows, where the behavioral
// response is eye velocity magnitude in deg/sec, averaged over the course of the "special segment".  The reward(s)
// delivered to the subject depend on the definition of these "reward windows".  If none are defined, the subject only
// gets the regular end-of-trial reward, using reward pulse #1 as usual.  If one or both windows are defined AND the
// measured response falls within one such window, then the subject has "passed the test" and gets two rewards:
// reward pulse #2 is delivered immediately after the special segment [NOT the same as the "atSegEnd" midtrial reward.
// Users are advised not to enable the "atSegEnd" midtrial reward for the "R/P Distro" special segment] and reward
// pulse #1 is delivered as the end-of-trial reward.  If the measured response falls outside the defined reward
// window(s), the subject has "failed the test" and gets only reward pulse #2 as an end-of-trial reward.  Of course,
// to properly motivate the subject, reward pulse #2 should be shorter than reward pulse #1.
//
// NOTE: If neither reward window is defined on the "R/P Distro" trial, then this trial code is NOT sent!
//
#define     RPDWINDOW            67    // [05dec2005] reward window(s) for an "R/P Distro" operation (N=3)
                                       //    code1, time1: [min,max] for reward window #1, in deg/s * 10.
                                       //    code2, time2: [min,max] for reward window #2, in deg/s * 10.
                                       // If a window is not defined, min==max==0.

#define     TARGET_VSTAB         68    // alter velocity stabilization of specified tgt at specified time (N=2).
                                       //    code1 = target#
                                       //    time1 = velocity stabilization flag bits
                                       // AS OF MAESTRO V2.0.0.  REPLACES TARGET_HOPEN code. The code is sent whenever
                                       // v-stab's effect on a target's trajectory changes.

                                       // Velocity stabilization flag bits for TARGET_VSTAB:
#define     VSTAB_ON            (1<<0) //    turn stabilization of target ON (set) or off (unset)
#define     VSTAB_SNAP          (1<<1) //    if set AND stabilization is turning ON (ie, it was off during previous
                                       //    segment), then tgt is snapped to current eye pos.
#define     VSTAB_H             (1<<2) //    enable (set) or disable (unset) stabilization of H component of motion
#define     VSTAB_V             (1<<3) //    enable (set) or disable (unset) stabilization of V component of motion
#define     VSTAB_MASK          (VSTAB_ON|VSTAB_SNAP|VSTAB_H|VSTAB_V)

#define     RANDOM_SEED          97    // specify seed for XY scope random # generator (N=2)
                                       //    code1 = HIWORD(randSeedLong); time1 = LOWORD(randSeedLong)
                                       // 13jan2003:  OBSOLETE!

#define     STARTTRIAL           98    // first trial code  (N=1)
#define     ENDTRIAL             99    // specifies time at which trial stops (N=1)


#endif   // !defined(CXTRIALCODES_MEX_H__INCLUDED_)
