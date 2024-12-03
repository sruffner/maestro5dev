//=====================================================================================================================
//
// cxtrialseq.cpp : Implementation of class CCxTrialSequencer, Maestro's trial sequencer.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// In TrialMode, Maestro "instructs" CXDRIVER to present a sequence of trials drawn from a specified trial "set". 
// The order in which trials are selected for presentation depend upon the current "sequencer mode". Eight distinct
// sequencer modes are available: (1) repeated presentation of a currently selected trial; (2) presentation of each 
// trial in the trial set in order of appearance within the set; (3) same as (2), except that a failed trial is
// repeated until the subject successfully completes it; (4) a weighted, ordered presentation (using the trial 
// weights to set how many reps of a given trial are presented relative to the other trials in the set); (5) a weighted,
// randomized presentation of the trials; (6) a randomized presentation of trial "chains" (1-N consecutive reps of the 
// same trial, where N is the trial weight); (7) same as (6), except that a failed trial is repeated until the subject
// successfully completes it; and (8) a specialized "staircase" presentation to support psychophysical protocols. Most 
// sequencer modes are available with or without fixation ("no fix") requirements enforced. While the trial selection 
// algorithm is straightforward for the first five modes, the staircase mode is complex and requires a significant 
// amount of additional overhead (both computation & data objects). CCxTrialSequencer encapsulates all of these trial 
// selection algorithms.
//
// As of Maestro v3.1.2, CCxTrialSequencer supports two levels of sequencing. Trial sets can now contain individual
// trials and/or trial subsets, which are simply collections of related trials. When a set contains at least one
// non-empty subset, the sequencer supports 3 possible subset sequencing types: (1) SUBSETSEQ_OFF -- disabled; all 
// trials in the set, including those in subsets, are treated as a single group; (2) SUBSETSEQ_ORDERED -- subsets are 
// presented in order of appearance in the parent set's child list; (3) SUBSETSEQ_RANDOM -- subsets are presented in
// random order (and the order is re-shuffled after all the subsets have been sequenced). When subset sequencing is
// enabled, the trials within a given subset are sequenced IAW one of the trial sequencing modes described above. NOTE,
// however, that the "current trial", "chained", and "staircase" trial sequencing modes are not allowed when subset
// sequencing is engaged. Furthermore, when a trial set contains individual trials as immediate children as well as
// trial subsets, each of those trials is treated as a separate subset containing just one trial.
//
// CXDRIVER has no knowledge of Maestro data objects like CCxTrial and CCxTarget. Instead Maestro must convert 
// such data objects into a form that CXDRIVER "understands". Target definitions must be converted to the 
// CXTARGET struct as defined in CXOBJ_IFC.H, while a trial is conveyed to CXDRIVER as a sequence of trial codes, 
// detailed in CXTRIALCODES.H. [The trial codes scheme was adopted from Cntrlx, the original incarnation of Maestro, 
// in which the GUI ran on a UNIX machine and transmitted these trial codes (as well as other information and commands)
// over the network to a Windows PC that controlled the hardware and executed all experimental protocols.] 
// CCxTrialSequencer provides methods that handle these conversions.
//
// In short, CCxTrialSequencer is a "helper" class that relieves our TrialMode controller, CCxTrialMode, of the
// nitty-gritty details behind the sequencing of Maestro trials.
//
// ==> Usage.
//    Init() ==> Initialize the trial sequencer IAW specifed control parameters, as defined by the TRIALSEQCTRL struct.
//       Control parameters include the chosen trial set's object key, the "current" trial in that set, the subset and
//       trial sequencing modes, and some staircase sequence-specific parameters. Init() analyzes all the trials in the 
//       trial set, compiles the "target list" containing all targets used across the entire set, prepares for 
//       sequencing, and selects the first trial. This method must be called before any others. NOTE that it can fail 
//       for a myriad of reasons -- in which case it posts an error message using CCntrlxApp::LogMessage().
//    SelectNextTrial() ==> Selects the next trial in the currently defined sequence, based on results provided for the
//       last trial presented.
//    GetTargets() ==> Prepares an array of CXTARGET structs defining all Maestro targets in the "target list" for the
//       current trial set.
//    GetChannels() ==> Get object key of the channel config (if any) associated with the currently selected trial.
//    GetTrialInfo() ==>  Prepare an array of TRIALCODEs defining the currently selected trial, as well as the trial
//       target map. Participating targets are identified by their ordinal pos in this map which, in turn, points to
//       the location of that target's definition in the target list prepared by GetTargets().
//    IsNoFixMode() ==> Is the sequencer's current op mode one of the "no fix" modes?
//    IsStaircaseMode() ==> Is the sequencer currently configured to run a staircase sequence?
//
// ==> Design considerations.
// CCxTrialSequencer is designed as a "helper" class for the sole use of CCxTrialMode. Its design takes into account
// a number of important assumptions about how TrialMode works in Maestro:
//    1) While a trial sequence is actually running, the user cannot make any modifications to the Maestro document. If
// this was not the case, the sequencer's selection algorithms will fail (they should ASSERT in the DEBUG release).
//    2) The sequencer control parameters (TRIALSEQCTRL) provided in the call to Init() remain in effect until a
// subsequent call to Init() is made. Thus, the user cannot modify these control parameters while a trial sequence is
// running.
//    3) Before a trial sequence starts, CXDRIVER's "target list" is loaded with the definitions of all tgts that may be
// used by any trial in the trial set. We do this primarily because it was necessary to preload ALL VSG2/4 framebuffer 
// video targets participating in a trial sequence prior to starting that sequence. Certain framebuffer targets could 
// take several seconds to load -- which would result in an unacceptably long intertrial interval. **HOWEVER, RMVideo is 
// MUCH FASTER than the old VSG 2/4 framebuffer, which is long obsolete. So, while we still continue to load all targets
// in a trial set before starting the sequence, that may no longer be necessary!**
//    4) We have elected for now to use large arrays in the CXDRIVER IPC data structure to hold the entire
// target list for the current trial sequence, as well as the trial codes for the current trial. For efficiency's
// sake, GetTargets() and GetTrialInfo() were designed to write directly into these arrays. CCxRuntime::StartTrial()
// and CCxRuntime::LoadTargetList() invoke these methods on a CCxTrialSequencer object passed to them!
//
// REVISION HISTORY:
// 30nov2001-- Introduced CCxTrialSequencer as part of the CCxRuntime module.  It is designed to handle the hard work
//             of sequencing trials and preparing trial & target defns in the format that CXDRIVER "understands".
// 27dec2001-- First version of CCxTrialSequencer done.  More features to be added later.
// 02jan2001-- Moved CCxTrialSequencer to its own source files, CXTRIALSEQ.*.  Also, CCxTrialSequencer will now serve
//             as a helper class to CCxTrialPanel rather than CCxRuntime directly -- although this change is invisible
//             to CCxTrialSequencer.
// 10jan2002-- GetTrialCodes() renamed GetTrialInfo() and modified to prepare a "trial target map".  Now instead of
//             identifying targets in the trial codes by their position in the "current target list", we identify them
//             by their ordinal pos in the map which, in turn, points to their definition in the target list.
// 15jan2002-- GetTrialInfo() modified IAW changes in how open-loop feature is implemented in CCxTrial.  Now, the user
//             must designate a single "open-loop" segment during which the pursuit loop is opened on whatever target
//             is designated as fixation tgt #1 for that segment.  The TARGET_HOPEN trial code is sent only for that
//             segment, if one is specified.  Furthermore, GetTrialInfo() enforces some additional constraints that are
//             intrinsic to the implementation of the open-loop feature during trial runtime:  (1) open-loop is only
//             supported for CX_FIBER1, CX_FIBER2, and XY scope target types CENTER, SURROUND, and RECTDOT.  (2) the
//             tgt selected as fixation tgt #1 during the designated open-loop segment MUST also be selected as the
//             fixation tgt #1 for the remainder of the trial.
// 12feb2002-- A video display configuration object, CCxVideoDsp, now provides the seed used to generate random-dot
//             patterns for XY scope targets.  GetTrialInfo() modified to query that object for a seed.
// 04mar2002-- Implemented the PSGM_TC trial code group in GetTrialInfo().
// 20sep2002-- Updated GetTrialInfo() to support new sacc-trig'd op "select by fix 2".
// 18oct2002-- Removed TRIALSEQCTRL.nRewLen1.  Trial reward length override now in CCxSettings object.  We also use
//             CCxSettings to get the random seed for XY target patterns.  CCxVideoDsp has been superseded by the
//             CCxSettings object.... Also introduced a per-segment "periodic reward" enable flag, and separate H & V
//             fixation accuracies.  See GetTrialInfo() for relevant changes.
// 18dec2002-- Modified GetTrialInfo to encode trial target perturbations via the TARGET_PERTURB trial code group.
// 13jan2003-- RANDOM_SEED trial code now obsolete.  The fixed XY dot seed value and the "auto/fixed" seed flag are
//             sent to CXDRIVER via the CX_SETDISPLAY command, and CXDRIVER handles generation of new seeds when the
//             "auto" flag is set.  GetTrialInfo() updated accordingly.
// 23oct2003-- Updated GetTrialInfo() to support separately enabling velocity stabilization (TARGET_HOPEN trial code
//             group) on the H cmpt only, the V cmpt only, or both components.  See also CXTRIALCODES.H.
// 10mar2004-- Added support for a new control parameter, the global starting position for all targets participating in
//             the trial.  Prior to this change, it was implicitly assumed that all targets started at (0,0), and the
//             user had to set the target window position in segment 0 if s/he wanted the target(s) to start elsewhere.
//             Setting a global starting position allows the user to reuse the same trials to run an experiment in a
//             different part of the subject's or unit's receptive field.  Instead of including them in TRIALSEQCTRL,
//             the H & V components of the global starting position are maintained separately and are NOT reset by
//             the Reset() method.  I want to get rid of TRIALSEQCTRL eventually...
// 05apr2004-- Moved tgt pos/vel transform factors (scale, rotate) and the channel configuration override enable flag
//             and object key out of TRIALSEQCTRL, since these have nothing to do with controlling the sequencer state.
//             They can now be modified between trials even when a sequence is in progress.
// 07apr2004-- Introduced autostop feature.  Sequencer can be configured to autostop after a specified number of trials
//             or trial blocks have been completed.
// 03nov2004-- Mod GetTrialInfo() to support new saccade-trig'd special op, CCxTrial::IsDualFix().
// 25jan2005-- Mod GetTrialInfo() to support new trial code MIDTRIALREW, now required to communicate the mid-trial
//             reward interval and pulse length to CXDRIVER (we no longer use similar parameters from the CCxSettings
//             object!!).  A second version of mid-trial rewards is supported, in which the mid-trial rewards are
//             delivered at the end of each enabled segment rather than at a regular interval during that segment.
// 06apr2005-- Mod GetTrialInfo() to retrieve any tagged sections defined on the selected trial.
// 29apr2005-- Mod GetTrialInfo().  The trial's mark segments now describe a contiguous range of segments representing
//             that portion of the trial to be displayed in Maestro's data trace display.  If one or both is not a
//             valid segment, then the entire trial will be displayed.  GetTrialInfo() now prepares the displayed
//             interval [t0..t1] based on the values of the mark segments.
// 28jul2005-- Updated GetTrialInfo() to support seed parameter for random noise perturbations, and to handle the new
//             noise perturbation type PERT_ISGAUSS.  See CXTRIALCODES.H.
// 16sep2005-- Modified GetTrialInfo() to allow velocity stabilization for any XY scope target type except FCDOTLIFE,
//             FLOWFIELD, and NOISYDOTS.
//          -- Mod to GetNextStaircaseTrial() to force a repeat of last staircase trial if subject fails to respond at
//             all (ie, trial results flag bit CX_FT_NORESP is set).  LogStaircaseStatus() modified to generate a
//             status message that indicates the lack of a response.
// 05dec2005-- Modified GetTrialInfo() to support the new special operation SPECIAL_RPDISTRO and the accompanying
//             trial code RPDWINDOW, which defines one or two reward windows for the operation.
// 07jan2006-- Mods re: new XY tgt type NOISYSPEED.  Also, defined constant NOISYDOTS is now NOISYDIR.
// 11mar2006-- Mod IAW revisions to R/P Distro feature -- now there is only one reward window.
// 13mar2006-- Mod to extend velocity stabilization over a contiguous span of trial segments; change in TARGET_HOPEN
//             trial code usage.  All other restrictions on v.stab. feature still in effect.  See GetTrialInfo().
// 24mar2006-- Mod IAW replacement of VSG-based framebuffer video w/RMVideo.  Since RMVideo is updated on a per-frame 
//             basis like the XY scope, RMVideo targets can participate in any Trial mode "special feature" (most 
//             notably, velocity stabilization) that an XYScope target can.  GetTrialInfo() updated.
// 12apr2006-- GetTrialInfo() modified IAW complete rework of velocity stabilization:  It can now be engaged on a 
//             per-target, per-segment basis using the new TARGET_VSTAB trial code. TARGET_HOPEN is obsolete.
// 19may2006-- Modified to support disabling use of the global tgt pos/vel scale and rotate transforms on a per-trial 
//             basis.  New flags in CCxTrial determine whether or not a global transformation is ignored or not.
// 22sep2006-- GetTrialInfo() modified to fix bug in trial code preparation. If target window vel for seg N is the same 
//             as that for seg N-1, the method did not send a TARGET_*VEL* code, even if target acceleration was 
//             nonzero in seg N. Given the way MaestroDRIVER processes trial codes, the TARGET_*VEL* code is required 
//             if target acceleration is nonzero!
// 01dec2006-- Reordered sequencer modes so that all NOFIX modes are grouped at bottom of list.
// 03jan2007-- Fixed GetTrialInfo() IAW changes in how special operation is identified by CCxTrial -- as integer ID 
//             code TH_SOP_**, defined in CXOBJ_IFC.H. Also, introduced two new operations, TH_SOP_CHOOSEFIX1 and 
//             TH_SOP_CHOOSEFIX2, with implementation constraints like those of the "selectByFix" operations.
// 27feb2007-- Mod GetTrialInfo() to support new trial codes specifying pattern acceleration H,V: INSIDE_*ACC and 
//             INSIDE_*SLOACC.
// 24apr2007-- Updated comments in GetTrialInfo().
// 25apr2007-- Mod to support alternative response measures for RP Distro feature: In the second TRIALCODE of the 
//             SPECIALOP trial code group, when opType == SPECIAL_RPDISTRO, TRIALCODE.code is set to 
//             SPECIAL_RPDISTRO | (rpdRespType << 8), where rpdRespType is the response measure type. For all other 
//             special ops, TRIALCODE.code = opType.  Changed in GetTrialInfo().
// 17jul2008-- Added seq mode "Ordered (Repeat)". In this version of the "Ordered" mode, the sequencer repeatedly 
//             presents the same trial until the subject completes it (ie, satisfy fixation reqmts). Note that there's
//             no "Ordered (Repeat) NOFIX" mode, since that will behave exactly like "Ordered NOFIX".
// 11jan2010-- Mod to GetTrialInfo() to relax restrictions on the velocity stabilization feature.
// 24may2010-- Mod to GetTrialInfo() to support reporting state of trial flag bits to CXDRIVER.
// 11jun2010-- Mod to GetTrialInfo(): Rotate and scale factors are NOT applied to target position in seg 0 IF target is
//             positioned absolutely. For all other segments, factors are applied regardless.
// 21jun2010-- Added seq modes "Wt Ordered" and "Wt Ordered NOFIX" -- like "Ordered", but trial "weights" are respected.
//             Thus, a trial with weight N is presented N times in a row before moving on to the next trial in the
//             ordered list of trials in a set. If trial does not complete successfully, it is repeated -- in which case
//             it could be presented more thant N times in a row... InitRandomReps() -> InitWeightedReps() and 
//             GetNextRandomTrial() -> GetNextWeightedTrial().
// 04oct2010-- Fixed bug: "Wt Ordered NOFIX" mode was not behaving like a NOFIX mode because I had failed to identify
//             it as such in IsNoFixMode().
// 01feb2011-- Mod to GetTrialInfo() to support new special op "searchTask" (TH_SOP_SEARCH, SPECIAL_SEARCH).
// 14mar2011-- Made WasTrialCompleted() public so it can be called by CCxTrialMode::Service().
// 23sep2011-- Removed references to CX_FIBER* in GetTrialInfo(), as CX_FIBER* ande CX_REDLED* are no longer supported
// as of Maestro v3.0.
// 05sep2013-- GetTrialInfo() now aborts if calculated trial duration exceeds MAXTRIALDUR defined in cxtrialcodes.h.
// 01oct2013-- Starting mods to support new sequencer modes "Chained" and "Chained NOFIX". A "trial chain" is the 
//             sequential presentation of N reps of the same trial. In this mode, it is not the sequence of individual
//             trials in the set that are randomized, but the sequence of trial chains. For each trial A with weight N,
//             N chains are included in the sequence: 1A, 2A, 3A, .., NA. Similarly for trial B, C, and so on. 
//          -- Also, CCxTrialSequencer now accumulates the simple trial stats that were handled in CCxTrialStatsDlg,
//             which now keeps a reference to the sequencer and queries it for current stats.
// 04oct2013-- Mod to "Chained" modes: A "failed" trial (animal lost fixation "early enough" that data file was NOT
//             saved) should NOT break a sequence of consecutive reps of the same trial. The only events that reset
//             the consecutive reps counter are: start of sequence, resumption of paused sequence, and change in trial
//             identity.
// 04oct2013-- Mod to "Chained" modes: TRIALSEQCTRL.strChainLens is a parameter specific to the "Chained" seq mode. 
//             The string is a comma-delimited list of integers indicating the different trial chain lengths to run.
//             If empty, then the seq mode works as before: all possible chains 1A - NA, where N is the weight of trial
//             A. Else, it only includes chain lengths that are listed in the string AND are <= N. Thus, if the string
//             is "1,2,4,8" and N=5 for trial A; a block in the sequence will include chains 1A, 2A, and 4A. The string
//             can include the same integer more than once if you want to increase the frequency of one chain length
//             WRT the others: "1,1,1,2,4", e.g. The same string applies to all trials in the set.
// 04dec2014-- Began substantial mods to support two levels of sequencing: ordered or randomized sequencing of trial
// subsets, with ordered or randomized sequencing of the trials within each subset. The other trial sequencing modes 
// ("this trial", staircase, chained) are not allowed when subset sequencing is enabled. When subset sequencing is off, 
// all the trials in the set are sequenced as a single group (even if they're divided into subsets), and all supported 
// trial sequencing modes are available. Effec. v3.1.2.
// 26sep2016-- Updated Init() and GetTrialInfo() to support "trial random variables". Effec. v3.3.0.
// 10oct2016-- Updated Init() so that it calls srand() to seed the basic rand() RNG using the current time. This 
// ensures that each time you restart Maestro, you get a different trial sequence in any mode that uses randomization.
// Prior to this change, you would get the same sequence on successive restarts.
// 22feb2017-- Added seq mode "Randomized Repeat", which is just like the "Randomized" seq mode, except that a failed
// trial is repeated until it is successfully completed. Effec. v3.3.1 (22feb17 rev).
// 05sep2017-- Fix compiler issues while building for 64-bit Win 10 using VStudio 2017.
// 24sep2018-- Mod to GetTrialInfo() to implement RMVideo vertical sync spot flash feature. The previously unused field
// 'time1' in the PULSE_ON code group is set to 1 to request the spot flash at the time specified in 'time0'.
// 07nov2018-- Fixed bug in GetTrialInfo() that caused FIXEYE1 to not be delivered when the mid-trial reward enable
// flag changed (the code was comparing the flag in the current segment against itself instead of the previous segment).
// Regression bug introduced in Sep2016 for V3.3.0. 
// 21mar2019-- Updated to support new trial result flag CX_FT_RMVDUPE, indicating that trial aborted prematurely due to
// a duplicate frame or frames occurring on the RMVideo side. Until now, a duplicate RMVideo frame was treated as an
// CX_FT_ERROR, causing the trial sequence to stop. In the future, if we start using RMVideo displays with refresh
// rates of 120Hz and higher, the chances of the occasional duplicate frame increase -- so we decided that this 
// condition be treated similarly to CX_FT_LOSTFIX. Rather than stopping the trial sequence, the trial data file is
// discarded and the trial is repeated as appropriate to the current sequencer mode.
// 20may2019-- Updated Init() and GetTrialInfo() to implement random reward withholding variable ratio feature for 
// individual trials. CCxTrial manages the runtime state for the feature, but CCxTrialSequencer must call the relevant
// CCxTrial methods to initialize, query and update the runtime state as trials are sequenced. To signal a withheld
// reward to CXDRIVER, GetTrialInfo() sets the reward pulse length to 0 in the relevant trial code.
// 03nov2022-- Updated to support new trial result flag CX_FT_EYELINKERR, indicating that trial aborted prematurely due
// to an Eyelink tracker communication error. Like CX_FT_RMVDUPE, this is considered a non-fatal error that should not
// terminate trial sequencing. 
// 26sep2024-- Eliminating XYScope-specific code. The XYScope platform, unsupported since Maestro V4.0, has been
// removed entirely in V5.0.
// 19nov2024-- Support for the never-used PSGM module dropped in Maestro 5.0.2. PSGM_TC trial code no longer sent to
// CXDRIVER. Updated GetTrialInfo() accordingly.
// 02dec2024-- Updated to handle new special feature "findAndWait", which has restrictions similar to "searchTask".
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "stdlib.h"                          // for rand() & srand()
#include "time.h"                            // for time() - to seed rand() RNG
#include "math.h"
#include "cntrlx.h"                          // CCntrlxApp and resource defines

#include "cxdoc.h"
#include "cxchannel.h"
#include "cxtarget.h"
#include "cxtrial.h"
#include "cxrpdistro.h"
#include "cxpert.h"
#include "cxsettings.h"

#include "cxtrialseq.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialSequencer
//
//=====================================================================================================================
//=====================================================================================================================

//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

const int CCxTrialSequencer::STAIR_SEQSTOP         = -2;             // staircase status codes:  stair seq stopped
const int CCxTrialSequencer::STAIR_ERRLOSTFIX      = -1;             // error or broke fix on last trial
const int CCxTrialSequencer::STAIR_NORESP          = 0;              // subject failed to respond to last trial
const int CCxTrialSequencer::STAIR_WRONG           = 1;              // incorrect response to last trial
const int CCxTrialSequencer::STAIR_OK              = 2;              // correct response to last trial
const int CCxTrialSequencer::STAIR_DONE            = 3;              // staircase N is finished

LPCTSTR CCxTrialSequencer::strSubsetSeqModes[] = {
   _T("OFF"), 
   _T("Ordered"), 
   _T("Randomized")
};

LPCTSTR CCxTrialSequencer::strTrialSeqModes[] =
{
   _T("Current trial"),
   _T("Ordered"),
   _T("Ordered (Repeat)"),
   _T("Wt Ordered"),
   _T("Randomized"),
   _T("Randomized (Repeat)"),
   _T("Chained"),
   _T("Staircase"),
   _T("Current trial NOFIX"),
   _T("Ordered NOFIX"),
   _T("Wt Ordered NOFIX"),
   _T("Randomized NOFIX"),
   _T("Chained NOFIX"),
   _T("Staircase NOFIX")
};

LPCTSTR CCxTrialSequencer::strAutoStopModes[] = 
{
   _T("Disabled"),
   _T("After N trials"),
   _T("After N blocks")
};

/**
 Is the specified combination of trial subset and individual trial sequencing modes valid? The sequencing modes must 
 be valid values; furthermore, if the subset sequencing mode is not SUBSETSEQ_OFF, then the trial sequencing mode can 
 only be one of: ORDERED, ORDERED_REPEAT, WT_ORDERED, RANDOM, RANDOM_REPEAT, ORDERED_NF, WT_ORDERED_NF, RANDOM_NF. 
 The other trial sequencing modes are not appropriate when subset sequencing is engaged.

 @param iSubsetSeq The trial subset sequencing mode.
 @param iTrialSeq The trial sequencing mode.
 @return True if the specified sequencing configuration is allowed.
*/
BOOL CCxTrialSequencer::IsValidSeqMode(int iSubsetSeq, int iTrialSeq)
{
   BOOL ok = iSubsetSeq >= 0 && iSubsetSeq < NUM_SUBSETSEQ;
   if(ok) ok = iTrialSeq >= 0 && iTrialSeq < NUM_TRIALSEQ;
   if(ok && iSubsetSeq != SUBSETSEQ_OFF)
      ok = (iTrialSeq >= ORDERED && iTrialSeq <= RANDOM_REPEAT) || (iTrialSeq >= ORDERED_NF && iTrialSeq <= RANDOM_NF);

   return(ok);
}


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTrialSequencer [constructor] =================================================================================
//
//    Here we preallocate some memory for the internal target and trial key arrays.  For typical experiment protocols,
//    it is unlikely we'll need any more memory than what's allocated here.  The arrays all have zero size, but there's
//    memory set aside for each.
//
//    THROWS:  CMemoryException
//
CCxTrialSequencer::CCxTrialSequencer()
{
   m_arTargets.SetSize( CX_MAXTGTS );
   m_arTargets.RemoveAt( 0, CX_MAXTGTS );          // ** this removes all elements without deallocating memory! **
   m_arTrials.SetSize( 256 );
   m_arTrials.RemoveAt( 0, 256 );
   m_arNumRepsLeft.SetSize( 256 );
   m_arNumRepsLeft.RemoveAt( 0, 256 );
   m_Stats.SetSize(256);
   m_Stats.RemoveAt(0, 256);

   Reset();                                        // init ctrl parameters to an "inactive" state

   m_dPosScale = 1.0;                              // default tgt/vel transform:  no transformation whatsoever
   m_dPosRotate = 0.0;
   m_dVelScale = 1.0;
   m_dVelRotate = 0.0;

   m_bUseChan = FALSE;                             // default chan cfg override: disabled, no override cfg specified
   m_wChanOvrKey = CX_NULLOBJ_KEY;
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Init ============================================================================================================
//
//    Prepare sequencer to select trials IAW the set of control parameters provided, and preselect the first trial.
//
//    ARGS:       tsqc    -- [in] the trial sequencer control parameters and trial overrides.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (eg, trial set does not support staircase sequencing).
//
BOOL CCxTrialSequencer::Init( const TRIALSEQCTRL& tsqc )
{
   // we need to access the experiment document to examine the trial set to be sequenced
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
   CCxDoc* pDoc = pApp->GetDoc(); 
   ASSERT(pDoc);
   ASSERT(pDoc->ObjExists(tsqc.wTrialSet) && (pDoc->GetObjType(tsqc.wTrialSet) == CX_TRIALSET));

   Reset(); 

   // verify that the subset and trial sequencing modes are valid. Certain trial sequencing modes are not allowed
   // when subset sequencing is on.
   if(!CCxTrialSequencer::IsValidSeqMode(tsqc.iSubsetSeq, tsqc.iTrialSeq))
   {
      pApp->LogMessage( "!! Trial and trial subset sequencing modes are invalid or incompatible. !!" );
      return(FALSE);
   }

   m_ctrl = tsqc; 

   // is trial subset sequencing enabled?
   BOOL bDoSubsets = (m_ctrl.iSubsetSeq != SUBSETSEQ_OFF) && pDoc->HasTrialSubsets(m_ctrl.wTrialSet);
   if(!bDoSubsets) m_ctrl.iSubsetSeq = SUBSETSEQ_OFF;

   // should zero-weight trials be ignored in the current trial sequencing mode?
   BOOL bIgnoreZeroWt = 
      (m_ctrl.iTrialSeq==RANDOM) || (m_ctrl.iTrialSeq==RANDOM_REPEAT) || (m_ctrl.iTrialSeq==RANDOM_NF) || 
      (m_ctrl.iTrialSeq==WT_ORDERED) || (m_ctrl.iTrialSeq==WT_ORDERED_NF) || 
      (m_ctrl.iTrialSeq==CHAINED) || (m_ctrl.iTrialSeq==CHAINED_NF);

   // prepare the key array listing the trials to be sequenced, in order of appearance in the trial set. We'll
   // take care of delineating the subsets, if any, later...
   CString errMsg;
   CWordArray wArKeys;
   pDoc->GetTrialKeysIn(m_ctrl.wTrialSet, wArKeys);
   CCxTrial* pTrial;
   for(int i=0; i<wArKeys.GetCount(); i++)
   {
      // exclude zero-wt trials in certain trial sequencing modes
      pTrial = (CCxTrial*) pDoc->GetObject(wArKeys[i]);
      if(bIgnoreZeroWt && (pTrial->GetWeight() == 0)) continue;

      // add trial key to array. Check trial's target list and save any participating target keys we don't have yet
      m_arTrials.Add(wArKeys[i]);
      for(int iTgt=0; iTgt<pTrial->TargCount(); iTgt++)
      {
         WORD wTgKey = pTrial->GetTarget(iTgt);
         BOOL bFound = FALSE;
         for(int j=0; j<m_arTargets.GetSize() && !bFound; j++) bFound = BOOL(wTgKey == m_arTargets[j]);
         if(!bFound) m_arTargets.Add(wTgKey);
      }

      // initialize any active random variables in the trial. On failure, report error in message log and abort.
      if(!pTrial->UpdateRVs(TRUE, errMsg))
      {
          pApp->LogMessage(errMsg);
          Reset();
          return(FALSE);
      }

      // initialize runtime state for random reward withholding for the trial's two possible reward pulses
      pTrial->InitRewardWHVR();

      // add a stats record for the trial. IF the trial is part of a trial subset rather than an immediate child of
      // the trial set, then the subset's name is included in the trial's name: "subset : trial".
      CStat* pStat = new CStat;
      if(pStat == NULL)
      {
         pApp->LogMessage(_T("ERROR: Memory exception."));
         Reset();
         return(FALSE);
      }

      WORD wParent = pDoc->GetParentObj(wArKeys[i]);
      if(wParent != m_ctrl.wTrialSet) pStat->name.Format("%s : %s", pDoc->GetObjName(wParent), pTrial->Name());
      else pStat->name = pTrial->Name();

      pStat->nAttempted = 0;
      pStat->nCompleted = 0;
      for(int j=0; j<MAX_CHAINLEN; j++) pStat->chainReps[j] = 0;

      m_Stats.Add(pStat);
   }

   // ERROR - no trials to sequence!
   if(m_arTrials.GetSize() == 0)
   {
      pApp->LogMessage("!! There are NO trials to sequence !!");
      Reset();
      return(FALSE);
   }

   // seed our basic RNG with a random seed based on the current time so we get a different sequence of numbers each 
   // time Maestro is run! 
   // IMPORTANT: After seeding, we call rand() a random # of times. This was necessary because testing showed that the 
   // first value from rand() after seeding increased monotonically as the time-based seed increased. Since rand()
   // generates integers in [0..RANDMAX] and we do rand() * N /RANDMAX to get the trial index, the first trial of a 
   // sequence would be the same each time a sequence started, until eventually that first rand() call produced a 
   // large enough value to move on to the next index.
   srand((unsigned int) time(NULL));
   int n = rand() % 20;
   if(n == 0) n = 1;
   for(int i=0; i<n; i++) rand();

   // if subset sequencing is enabled, prepare the list of trial subsets. NOTE that the trial list above is populated
   // in order, so it's simple to divide it into its constituent subsets. ALSO NOTE that each trial that is an 
   // immediate child of the trial set is treated as a subset containing a single trial!
   if(bDoSubsets)
   {
      WORD wCurrSS = CX_NULLOBJ_KEY;
      for(int i=0; i<m_arTrials.GetSize(); i++)
      {
         WORD wKey = m_arTrials[i];
         WORD wParent = pDoc->GetParentObj(wKey);

         if(wParent != wCurrSS)
         {
            // start a new subset 
            CSubset* pSubset = new CSubset;
            if(pSubset == NULL)
            {
               pApp->LogMessage(_T("ERROR: Memory exception."));
               Reset();
               return( FALSE );
            }
            pSubset->idxFirst = i;
            pSubset->nTrials = 1;
            
            // if the trial is an immediate child of trial set, it will stand alone as a subset containing one trial,
            // and its name is set to "::trialName".
            if(wParent == m_ctrl.wTrialSet)
               pSubset->name.Format("::%s", pDoc->GetObjName(wKey));
            else
            {
               pSubset->name = pDoc->GetObjName(wParent);
               wCurrSS = wParent;  // so we can detect the start of a new trial subset!
            }

            try 
            {
               m_Subsets.AddTail(pSubset);
            }
            catch(CMemoryException *e)
            {
               e->Delete();
               delete pSubset;
               pApp->LogMessage(_T("ERROR: Memory exception."));
               Reset();
               return(FALSE);
            }
         }
         else
         {
            // trial is part of current subset
            (m_Subsets.GetTail()->nTrials)++;
         }
      }

      // prepare to begin sequencing the subsets IAW the subset sequencing mode. If the subsets are to be randomized,
      // we shuffle the subset list here. We allow subset sequencing even if there's just one subset, although that
      // makes little sense!
      ShuffleSubsets();
      m_iCurrSubset = 0;
   }

   // at this point, we're ready to start seqencing trial subsets, if subset sequencing is on. But there's still
   // more to do for certain trial sequencing modes...
   switch(m_ctrl.iTrialSeq) 
   {
      // no additional setup required for these trial sequencing modes
      case THISTRIAL_NF : 
      case THISTRIAL : 
      case ORDERED_NF :  
      case ORDERED :
	   case ORDERED_REPEAT :
         break;

      // run trials randomly or in order, but taking into account possibly different trial weights. Zero-wt trials are
      // not presented. If subset sequencing is on, we prepare to present the trials in the current subset.
      case WT_ORDERED : 
      case WT_ORDERED_NF : 
      case RANDOM_NF : 
      case RANDOM : 
      case RANDOM_REPEAT :
         if((m_ctrl.iTrialSeq==RANDOM || m_ctrl.iTrialSeq==RANDOM_NF || m_ctrl.iTrialSeq==RANDOM_REPEAT) && 
            (m_arTrials.GetSize() < 2))
         {
            pApp->LogMessage( "!! Cannot run randomized sequence with only one trial !!" );
            Reset();
            return(FALSE);
         }
         InitWeightedReps();
         break;
      
      // run CHAINS of the individual trials randomly. For each trial A in the set, chains of length 1, 2, .., N are
      // included in the block, where N is the weight of trial A. Zero-wt trials are not presented; and the set must
      // contain at least two different non-zero-wt trials. NOTE that chained trial sequencing is not supported when
      // subset sequencing is engaged.
      case CHAINED :
      case CHAINED_NF :
         if(m_arTrials.GetSize() < 2)
         {
            pApp->LogMessage( "!! Cannot run chained sequence with only one trial !!" );
            Reset();
            return(FALSE);
         }
         if(!InitChainedReps())
         {
            Reset();
            return(FALSE);
         }
         break;

      // staircase sequence. Analyze trials and setup internal staircase control structures. This could fail if trials
      // in set do not support staircase sequencing. Also, staircase sequencing is not allowed when subset sequencing
      // is engaged.
      case STAIRCASE_NF : 
      case STAIRCASE : 
         if(!InitStaircases())
         { 
            Reset();
            return(FALSE);
         }
         break;

      default :
         TRACE0( "\nBad trial sequencer mode!" );
         ASSERT(FALSE);
         break;
   }

   // we're ready to sequence trials. Select the first trial to be presented.
   m_iSelected = -1; 
   m_bInitialized = TRUE; 
   m_bSeqStart = TRUE;
   DWORD dwRes = CX_FT_DONE; 
   WORD wKey = SelectNextTrial(dwRes);
   if(wKey == CX_NULLOBJ_KEY) Reset();
   return(BOOL(wKey != CX_NULLOBJ_KEY));
}


//=== GetTargets ======================================================================================================
//
//    Retrieve definitions of all targets used across all trials in the current trial set, in a format compatible w/
//    MAESTRODRIVER.
//
//    ARGS:       pN    -- [out] total # of targets used across all trials in set.
//                nMax  -- [in] the max # of targets that can be stored in provided buffer.
//                pTg   -- [out] buffer that will hold the target definitions in format compatible w/ CXDRIVER.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (eg, buffer not large enough).
//
BOOL CCxTrialSequencer::GetTargets( int* pN, const int nMax, CXTARGET* pTg ) const
{
   ASSERT( m_bInitialized );
   if( nMax < m_arTargets.GetSize() ) return( FALSE );                        // buffer not large enough!

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();                       // to access target objects

   for( int i = 0; i < m_arTargets.GetSize(); i++ )
   {
      CCxTarget* pTarg = (CCxTarget*) pDoc->GetObject( m_arTargets[i] );
      ASSERT_KINDOF( CCxTarget, pTarg );

      pTg[i].wType = pTarg->DataType();
      sprintf_s( pTg[i].name, "%.*s", CX_MAXOBJNAMELEN-1, LPCTSTR(pTarg->Name()) );
      pTarg->GetParams( &(pTg[i].u) );
   }
   *pN = static_cast<int>(m_arTargets.GetSize());
   return( TRUE );
}

//=== SelectNextTrial =================================================================================================
//
//    Select next trial to present in the currently defined trial sequence. If the sequencer mode is ORDERED, RANDOM
//    or CHAINED (with or w/o fixation) and the previous trial was the last of a "block", then the CX_FT_BLOCKDONE flag
//    is set in the trial results field. If mode is STAIRCASE (w/ or w/o fix) and the staircase sequence has just 
//    auto-stopped, we set the CX_FT_SEQSTOP flag in the results field AND return CX_NULLOBJ_KEY for the next trial. 
//    The sequence is also auto-stopped when the "autostop" feature is engaged and the required number of trials or 
//    trial blocks have been completed. If the CX_FT_SEQSTOP flag is already set, we do nothing and return 
//    CX_NULLOBJ_KEY.
//
//    17jul2008: ORDERED_REPEAT sequencer mode added. This is the same as ORDERED, except that a failed trial (LOST_FIX
//    result flag) is presented over and over until the subject "passes" the trial. Of course, this means that a trial 
//    "block" can contain many more trial presentations than there are trials defined in the current trial set!
//
//    21jun2010: WT_ORDERED, WT_ORDERED_NF modes added. Trials are presented in order, but each trial is presented N
//    times in a row, where N = trial's weight. Thus, zero-wt trials are not presented. Also, a trial may be presented
//    more than N times because it is repeated each time it fails to complete successfully.
//
//    02oct2013: CHAINED, CHAINED_NF modes added. See GetNextChainedTrial() for details. In addition, trial stats 
//    (#attempts, #completed, and #reps of a given chain length for CHAINED modes) are updated here. This works because
//    SelectNextTrial() is always called immediately after finishing a trial, successfully or not.
//
//    04dec2014: Introduced trial subsets and a second level of sequencing: ordered or random sequencing of the subsets
//    comprising the trial set. Two sequence modes are specified by user: the manner in which trial subsets are 
//    sequenced, and the way in which the trials in the current subset are sequenced. Some trial sequencing modes are
//    only available when subset sequencing is OFF.
//
//    22feb2017: RANDOM_REPEAT sequencer mode added. Same as RANDOM, except that a failed trial (LOST_FIX result flag)
//    is presented over and over until the subject "passes" the trial. 
//
//    21mar2019: New trial result flag bit CX_FT_RMVDUPE indicates that trial aborted because too many duplicate frames
//    were detected by RMVideo during the trial (either a single duplicate, or more than 3 if user selected the option
//    to allow as many as 3 duplicate frames). Prior to this, any duplicate frame was treated as a fatal error 
//    (CX_FT_ERROR), thereby terminating the trial sequence. Now, trial sequencing continues on the CX_FT_RMVDUPE 
//    result. For most sequencer modes, the failed trial is repeated (or not) IAW the mode's rules, same as for 
//    CX_FT_LOSTFIX. For the Chained and Staircase modes, a CX_FT_RMVDUPE trial is treated as though it never happened.
//
//    03nov2022: New trial result flag bit CX_FT_EYELINKERR indicates that trial aborted because of an Eyelink tracker
//    communication error. This is treated as a non-fatal error, same as CX_FT_RMVDUPE.
//
//    See also low-level implementing methods GetNextWeightedTrial(), GetNextChainedTrial(), GetNextStaircaseTrial().
//
//    ARGS:       dwTrialRes -- [in/out] results from last trial run (ignored when selecting first trial in sequence!).
//
//    RETURNS:    key of next trial to run; CX_NULLOBJ_KEY if a fatal error occurred, or if trial sequencer has
//                auto-stopped.
//
WORD CCxTrialSequencer::SelectNextTrial(DWORD& dwTrialRes)
{
   ASSERT(m_bInitialized);

   // if sequence already stopped, abort
   if((dwTrialRes & CX_FT_SEQSTOP) != 0) return(CX_NULLOBJ_KEY);

   // update trial statistics based on trial result. Ignore if we're just starting trial sequence
   BOOL success = WasTrialCompleted(dwTrialRes);
   if(m_iSelected>=0)
   {
      if(success) ++m_nTrialsDone;
      CStat* pStat = m_Stats[m_iSelected];
      ++(pStat->nAttempted);
      if(success) ++(pStat->nCompleted);

      // in CHAINED modes, we keep track of the # of consecutive successful reps of the same trial and maintain stats
      // on how many times we see N consecutive successful reps, where N=1,2,..,10, and 11+. NOTE that these "successful
      // chains" are subtly different from the programmed trial chains presented by the sequencer -- since a successful
      // chain can span across 2 (or more) consecutive programmed chains involving the same trial, or it could be 
      // shorter than a programmed chain b/c the sequence was paused mid-chain.
      // 
      // If a trial failed, we DO NOT reset the consecutive rep counter! This counter only gets reset when a trial 
      // sequence starts or resumes, or when a different trial is chosen for presentation.
      if(IsChainedMode())
      {
         if(success)
         {
            ++m_nConsecutiveRepsOK;
            CStat* pStat = m_Stats[m_iSelected];
            int bin = m_nConsecutiveRepsOK - 1; 
            if(bin >= MAX_CHAINLEN) bin = MAX_CHAINLEN-1;
            ++(pStat->chainReps[bin]);
         }
      }
      
   }

   switch(m_ctrl.iTrialSeq)
   {
      // play the "current trial" over and over (this trial seq mode not allowed with subset sequencing enabled).
      // On first call after init, find and save position of the current trial in trial key array. Else there's
      // nothing to do. If "current trial" is unspecified, choose first trial in key array.
      case THISTRIAL_NF :
      case THISTRIAL :
         if(m_iSelected < 0) 
         { 
			   m_iSelected = 0;
            for(int i = 0; i < m_arTrials.GetSize(); i++)
            {
			      if(m_arTrials[i] == m_ctrl.wCurrTrial) { m_iSelected = i; break; }
            }
         }
         break;

      // present the entire list of trials in the set -- or in the current subset if subset sequencing is on -- 
      // in the order in which they appear in the MAESTRO document tree. The "block done" flag is set when we've
      // presented all the trials in the set (it is not set after each subset of trials is presented!), and we
      // then start anew. In the ORDERED_REPEAT case, we do not advance to the next trial until the current trial
      // was successfully completed.
      case ORDERED_NF :
      case ORDERED : 
      case ORDERED_REPEAT :
         if(m_ctrl.iSubsetSeq != SUBSETSEQ_OFF && !m_Subsets.IsEmpty())
         {
            CSubset* pSubset = m_Subsets.GetAt(m_Subsets.FindIndex(m_iCurrSubset));
            int iSel;
            if(m_iSelected < 0) iSel = 0;
            else
            {
               iSel = m_iSelected - pSubset->idxFirst;
               if(success || (m_ctrl.iTrialSeq != ORDERED_REPEAT)) ++iSel;
               if(iSel >= pSubset->nTrials)
               {
                  // we've finished a trial subset, so move on to the next one. If we're done with the last subset, 
                  // then set the "block done" flag and start over
                  ++m_iCurrSubset;
                  if(m_iCurrSubset == m_Subsets.GetSize())
                  {
                     dwTrialRes |= CX_FT_BLOCKDONE;
                     ++m_nBlocksDone;
                     ShuffleSubsets(); 
                     m_iCurrSubset = 0;
                  }
                  pSubset = m_Subsets.GetAt(m_Subsets.FindIndex(m_iCurrSubset));
                  iSel = 0;
               }
            }

            // remember: m_iSelected is an index into the entire trial list, and the subsets could be randomly presented
            m_iSelected = pSubset->idxFirst + iSel;
         }
         else
         { 
		      if((m_iSelected < 0) || (m_ctrl.iTrialSeq != ORDERED_REPEAT) || success)
			      ++m_iSelected;
            if(m_iSelected >= m_arTrials.GetSize())
            {
               m_iSelected = 0;
               dwTrialRes |= CX_FT_BLOCKDONE;
               ++m_nBlocksDone;
            }
         }
         break;

      // run trials randomly or in order, but trial weight determines the number of times a trial is presented per 
      // "block" in the sequence.
      case WT_ORDERED_NF :
      case WT_ORDERED :
      case RANDOM_NF : 
      case RANDOM :
      case RANDOM_REPEAT :
         GetNextWeightedTrial(dwTrialRes);
         if((dwTrialRes & CX_FT_BLOCKDONE) != 0) ++m_nBlocksDone;
         break;

      // run trial chains (1 or more reps of the same trial, presented consecutively) randomly
      case CHAINED_NF :
      case CHAINED :
         GetNextChainedTrial(dwTrialRes);
         if((dwTrialRes & CX_FT_BLOCKDONE) != 0) ++m_nBlocksDone;
         break;

      // choose next staircase trial
      case STAIRCASE_NF : 
      case STAIRCASE :
         GetNextStaircaseTrial( dwTrialRes );
         break;

      default:
         TRACE0( "\nBad trial sequencing mode!" );
         ASSERT( FALSE );
         break;
   }

   // if auto-stop featue enabled, check to see if we're done
   if(m_iAutoStopMode != AUTOSTOP_OFF)
   {
      int n = m_nTrialsDone;
      if((m_iAutoStopMode==AUTOSTOP_BLKS) && DoesModeUseBlocks())
         n = m_nBlocksDone;

      if(n >= m_nAutoStopCount)
      {
         dwTrialRes |= CX_FT_SEQSTOP;
         m_iSelected = -1;
      }
   }

   if(m_iSelected < 0 || m_iSelected >= m_arTrials.GetSize()) return(CX_NULLOBJ_KEY);
   return(m_arTrials[m_iSelected] );
}


/** WasTrialCompleted =================================================================================================
Does the trial result flags indicate that the trial completed successfully? 

A trial is "completed" if no fatal error occurred, the trial did not stop prematurely because duplicate video frames 
occurred on the RMVideo display or because of an Eyelink tracker communication error, and the subject did not lose 
fixation (or the sequencer mode does not enforce fixation). In the CHAINED sequencer mode, the condition is somewhat 
weaker: the trial is "completed" so long as no error occurred and the data file was saved, even if the subject lost
fixation.

NOTE that when an error occurs, the trial sequence will be stopped. Prior to 21mar2019, a duplicate frame event in
RMVideo was treated as an error; now it is treated like a "lost fix" condition in most sequencer modes.

As of 03nov2022, an Eyelink tracker communication error is treated like an RMVideo duplicate frame event. These two 
kinds of "errors" generally are NOT fatal errors, so it is better to simply discard the failed trial and continue 
trial sequencing.

@param dwTrialRes The trial result flags.
@return TRUE if flags indicate that trial completed successfully, as described.
*/
BOOL CCxTrialSequencer::WasTrialCompleted( DWORD dwTrialRes ) const
{
   BOOL bOk = BOOL( (dwTrialRes & (CX_FT_ERROR|CX_FT_RMVDUPE|CX_FT_EYELINKERR)) == 0 );
   if(bOk)
   {
      bOk = BOOL( IsNoFixMode() || ((dwTrialRes & CX_FT_LOSTFIX) == 0) );
      if((!bOk) && IsChainedMode()) bOk = ((dwTrialRes & CX_FT_DATASAVED) != 0);
   }
   return( bOk );
}


//=== GetChannels =====================================================================================================
//
//    Retrieve the key of the channel configuration object for the currently selected trial -- either the one specified
//    in the trial's definition, or the "global" channel configuration if the global override is enabled in the
//    sequencer's control parameters.
//
//    ARGS:       NONE.
//
//    RETURNS:    key of channel config associated with current trial.  if no trial is currently selected, we return
//                CX_NULLOBJ_KEY.
//
WORD CCxTrialSequencer::GetChannels() const
{
   ASSERT( m_bInitialized );
   if( m_iSelected < 0 ) return( CX_NULLOBJ_KEY );                // a trial has not yet been selected!
   ASSERT( m_iSelected < m_arTrials.GetSize() );

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();           // to gain access to trial & channel config objects
   ASSERT( pDoc );

   WORD wKey;
   if( m_bUseChan ) wKey = m_wChanOvrKey;                         // global override in effect!
   else                                                           // else, retrieve channel config key from trial obj
   {
      CCxTrial* pTrial =
         (CCxTrial*) pDoc->GetObject( m_arTrials[m_iSelected] );
      ASSERT( pTrial );
      ASSERT_KINDOF( CCxTrial, pTrial );
      wKey = pTrial->GetChannels();
   }

   return( wKey );
}


//=== GetTrialInfo ====================================================================================================
//
//    Retrieve the trial target map & trial codes defining the currently selected trial, taking into account any
//    "global overrides" and translation/rotation factors in effect (part of the sequencer's control parameters).  Any
//    tagged sections defined on the selected trial are also provided.
//
//    Participating targets are identified within the trial codes by their ordinal position in the trial target map,
//    which is merely a list of indices pointing to the corresponding target's definition in the "loaded target list"
//    that is prepared by GetTargets().
//
//    On "special" operations: Special trial options like "skipOnSaccade" have implementation constraints that cannot
//    easily be enforced while the user is defining the trial. Instead, we check & enforce the constraints here, 
//    returning FALSE if a constraint is violated.  Briefly, the limitations are:
//       1) A trial involving a special operation OTHER THAN "R/P Distro" cannot use the turntable (CX_CHAIR); it is 
//          not compatible with this feature. The "R/P Distro" does NOT have this constraint because it does not 
//          involve changing the trial's timeline at runtime.
//       2) A "selByFix*", "chooseFix*", and "selDurByFix" trial MUST specify fixation targets 1 & 2 during the 
//          designated special segment. The same fixation targets must be specified for all remaining segments after 
//          the special segment.
//       3) A "searchTask" or "findAndWait" trial must have more than one participating target, must specify fixation 
//          target 1 during the special segment, must specify a non-zero grace period for that segment that is strictly 
//          less than the segment's minimum duration, and must specify the special segment as the LAST trial segment.
//
//
//    Velocity stabilization (TARGET_VSTAB) can be engaged on a per-target, per-segment basis, with some restrictions:
//       1) The RMVideo RMV_RANDOMDOTS target won't behave correctly when it has a finite dotlife expressed in 
//    degrees travelled. 
//       2) In RMVideo, target pattern motion is almost always WRT the target's center, not the screen center. So,
//    velocity stabilization of these targets is achieved simply by adjusting the target's window motion. There is one 
//    exception: the RMV_RANDOMDOTS target type with flag RMV_F_WRTSCREEN set. In this case, both pattern and window 
//    motion are WRT the screen center, and both are adjusted when velocity stabilization is engaged.
//       4) Velocity stabilization of CHAIR makes no sense and is not allowed. In this case, this method will not
//    generate the trial codes and will abort on an error.
//
//    Maximum trial duration check: Since v3.0, CXDRIVER calculates trial trajectories on the fly, eliminating the
//    need for large buffers and making very long trials possible. However, one obstacle remains: the trial codes store
//    elapsed trial times as short integers. Thus, we restrict trial length to a value, MAXTRIALDUR, less than the max
//    value of a signed 16-bit integer.
//
//    BACKGROUND: CXDRIVER has no knowledge of Maestro data objects like CCxTrial and CCxTarget. Instead Maestro 
//    must convert those data objects into a form that CXDRIVER "understands". A Maestro trial object is conveyed 
//    to CXDRIVER as a sequence of trial codes, which are detailed in CXTRIALCODES.H. This scheme was adopted 
//    from the original incarnation of Maestro, in which the GUI ran on a UNIX workstation and transmitted these trial 
//    codes (as well as other information and commands) over the network to a Windows PC that communicated with the 
//    Maestro hardware and executed all experimental protocols.
//
//    02oct2013: Trial state flags related to the new CHAINED sequencer modes. Added two flags, THF_CHAINED and 
//    THF_CHAINSTART, in CXOBJ_IFC.H. These are not set as part of the trial's definition. Instead, they are set only
///   by this sequencer object to (a) mark a trial that is part of a chained sequence, and (b) mark those trials in a
//    chained sequence that are the first trial in a chain (1 or more identical trials presented consecutively). Four
//    kinds of events mark the end of one trial chain and the beginning of another: (1) The trial sequence just 
//    started. (2) A paused trial sequence is resumed. (3) The last trial failed because subject broke fixation AND
//    the trial data file was NOT saved (failsafe segment not reached). (4) The previous trial was different.
//
//    26sep2016: Added support for trial random variables. We call CCxTrial::UpdateRVs() to get the next "value" for
//    each active random variable defined on the trial. This could fail if a function-type RV cannot be evaluated, in
//    which case trial sequencing should be aborted. Once the RVs have been updated, any segment table parameter that 
//    is assigned to an RV takes on that RV's current value for the purposes of generating the trial codes -- see the
//    CCxTrial::GetCurr***() methods. The parameters that may be governed by an RV: segment duration, or any of the 10
//    target trajectory parameters.
//
//    NOTE: We implicitly assume for now that the timebase for the trial is 1msec.  Hence, the projected duration in
//    msecs is the same as the duration in # of frames.
//
//    NOTE2: Maestro 5.0.1 introduces a special segment operation, "selDurByFix", in which target selection in the
//    special segment determines whether the min or max duration of the following segment is chosen. Thus, the
//    actual trial duration (if completed) will be one of two possibilities. The output argument piDur is set to
//    the larger of the two.
// 
//    ARGS:       pNT      -- [out] the total # of targets participating in the trial.
//                pTgMap   -- [out] the trial target map.  Caller is responsible for ensuring buffer is big enough.
//                pN       -- [out] the total # of trial codes prepared.
//                nMax     -- [in] size of the trial code buffer provided.
//                pCodes   -- [out] the trial code buffer (provided by caller).
//                pFlags   -- [out] state of the trial's bit flags.  
//                pNSects  -- [out] the total # of tagged section records stored in the section buffer.
//                pSections-- [out] the tagged section buffer.  Provided by caller.  Must be able to accommodate
//                            as many as MAX_SEGMENTS section records.
//                piDur    -- [out] projected duration of trial in millisecs (of course, trial may be cut short if
//                            subject breaks fixation, a hardware error occurs, a "skipOnSaccade" occurs, etc. Also
//                            see NOTE2)
//                piT0,piT1-- [out] the trial time interval to be displayed in Maestro trace window, [t0..t1].
//                            Normally, the entire trial is displayed, so t0=0 and t1=duration of trial.  However, if
//                            the trial's display mark segments segA <= segB are both valid, then t0 and t1 are set so
//                            that [t0..t1] spans the segment range [segA..segB].  If either endpoint is negative,
//                            then assume that the entire trial should be displayed.
//                bSave    -- [out] if TRUE, selected trial's "keep" flag is set; else FALSE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (ill-defined trial, buffer not large enough, trial too long).
//
//    THROWS:     CString ops may throw CMemoryException
//
BOOL CCxTrialSequencer::GetTrialInfo( int* pNT, int* pTgMap, int* pN, const int nMax, TRIALCODE* pCodes,
   DWORD* pFlags, int* pNSects, TRIALSECT* pSections, int* piDur, int* piT0, int* piT1,  BOOL& bSave )
{
   int i, j;

   ASSERT( m_bInitialized );
   ASSERT( m_iSelected >= 0 && m_iSelected < m_arTrials.GetSize() );

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                              // the app object
   CCxDoc* pDoc = pApp->GetDoc();                                             // the Maestro document obj
   CCxSettings* pSet = pDoc->GetSettings();                                   // document/application settings
   CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[m_iSelected] ); // the currently selected trial
   ASSERT_KINDOF( CCxTrial, pTrial );

   bSave = pTrial->IsSaved();                                                 // is recorded trial data to be saved?
   *piDur = 0;                                                                // projected trial duration

   *piT0 = -1;                                                                // initially, no display interval set
   *piT1 = -1;

   CString strErr;                                                            // for logging error msg

   // generate next random variate for each active random variable in trial (if any). This could fail if a function
   // RV is undefined, or if any segment table param has been assigned to an undefined RV...
   if(!pTrial->UpdateRVs(FALSE, strErr))
   {
      pApp->LogMessage(strErr);
      return(FALSE);
   }

   int nTargs = pTrial->TargCount();                                          // # of targets participating in trial
   if( nTargs == 0 )                                                          // there must be at least one!
   {
      strErr.Format( "!! No targets defined in trial _%s_ !!", pTrial->Name() );
      pApp->LogMessage( strErr );
      return( FALSE );
   }

   CCxTarget* pTargs[MAX_TRIALTARGS];                                         // ptrs to participating target objects

   *pNT = nTargs;                                                             // trial target map:  each tgt ID'd in
   for( i = 0; i < nTargs; i++ )                                              // trial codes by its pos in map which,
   {                                                                          // in turn, points to tgt's defn in the
      WORD wKey = pTrial->GetTarget( i );                                     // "loaded target list"...
      pTargs[i] = (CCxTarget*) pDoc->GetObject( wKey );                       //    tgt obj ptrs saved for use later
      ASSERT( pTargs[i] );
      ASSERT_KINDOF( CCxTarget, pTargs[i] );

      for( j = 0; j < m_arTargets.GetSize(); j++ )                            //    find tgt's pos in target list
      {
         if( wKey == m_arTargets[j] ) break;
      }
      ASSERT( j < m_arTargets.GetSize() );
      pTgMap[i] = j;                                                          //    ...and store that pos in the map
   }

   // if special op other than "R/P Distro" is used...
   int iSpecOp = pTrial->GetSpecialOp(); 
   BOOL bDoesSpecOp = BOOL(iSpecOp != TH_SOP_NONE); 
   if( bDoesSpecOp && (iSpecOp != TH_SOP_RPDISTRO) )
   {
      // (1) make sure we don't use turntable
      for( i = 0; i < nTargs; i++ ) 
      { 
         WORD wType = pTargs[i]->DataType();
         if( wType == CX_CHAIR )
         {
            strErr.Format( "!! Trial _%s_: Turntable incompatible with selected special op!!", pTrial->Name() );
            pApp->LogMessage( strErr );
            return( FALSE );
         }
      }

      // for ops other than "skipOnSacc", "searchTask", and "findAndWait" ...
      if(iSpecOp != TH_SOP_SKIP && iSpecOp != TH_SOP_SEARCH && iSpecOp != TH_SOP_FINDWAIT)
      {
         // both fix targets must be specified during the "special segment", AND..
         j = pTrial->GetSpecialSegPos(); 
         int iTg1 = pTrial->GetFixTarg1Pos( j ); 
         int iTg2 = pTrial->GetFixTarg2Pos( j );
         BOOL bOk = BOOL( iTg1 >= 0 && iTg2 >= 0 );

         // the same fix tgts must be chosen for all remaining segments of the trial
         while(bOk && (j < pTrial->SegCount())) 
         { 
            bOk = BOOL(pTrial->GetFixTarg1Pos( j ) == iTg1 && pTrial->GetFixTarg2Pos( j ) == iTg2);
            ++j;
         }

         if( !bOk )
         {
            strErr.Format( "!! Trial _%s_: FixE1 & FixE2 ill-defined for two fix tgt ops!!", pTrial->Name() );
            pApp->LogMessage( strErr );
            return( FALSE );
         }

         // for the "switchFix" and "selDurByFix" ops, there must be at least one seg after the special seg
         if((iSpecOp == TH_SOP_SWITCHFIX || iSpecOp==TH_SOP_SELDUR) && 
            (pTrial->GetSpecialSegPos() >= pTrial->SegCount()-1)) 
         {
            strErr.Format( 
               "!! Trial _%s_: 'switchFix' or 'selDurByFix' trial must have at least one seg after special seg!!",
               pTrial->Name() );
            pApp->LogMessage( strErr );
            return( FALSE );
         }
      }
      
      // (3) for the "searchTask" or "findAndWait" op: Fix1 and positive grace period must be specified during special 
      // segment, grace period must be less than min duration, must be more than 1 participating trial target, and 
      // special segment must be the last trial segment
      if(iSpecOp == TH_SOP_SEARCH || iSpecOp == TH_SOP_FINDWAIT)
      {
         j = pTrial->GetSpecialSegPos(); 
         if(nTargs < 2 || pTrial->GetFixTarg1Pos(j) < 0 || pTrial->GetGracePeriod(j) <= 0 || 
            pTrial->GetGracePeriod(j) >= pTrial->GetCurrMinDuration(j) || (j != pTrial->SegCount() - 1))
         {
            strErr.Format( 
               "!! Trial _%s_: Does not satisfy constraints for a 'searchTask' or 'findAndWait' trial!",
               pTrial->Name() );
            pApp->LogMessage( strErr );
            return( FALSE );
         }
      }
   }

   // if any tgt is to be subjected to velocity stablization at any point during the trial, make sure it is an
   // RMVideo target
   for( i=0; i<pTrial->TargCount(); i++ )  
   { 
      CCxTarget* pTgt = pTargs[i]; 
      WORD wType = pTgt->DataType(); 
      BOOL bSupported = BOOL(wType == CX_RMVTARG);
      if(!bSupported) for(j=0; j<pTrial->SegCount(); j++) 
      {
         if(pTrial->GetTgtVStabMode(j,i) != SGTJ_VSTABOFF)
         {
            strErr.Format( "!! Trial _%s_: V-Stab not supported for tgt %s !!", pTrial->Name(), pTgt->Name() );
            pApp->LogMessage( strErr );
            return( FALSE );
         }
      }
   }

   strErr.Format( "!! Trial _%s_: Trial code resources exceeded !!",          // error message if trial code buffer is
                  pTrial->Name() );                                           // too small

   short shFrame = 0;                                                         // elapsed time in timebase units
   short shSegDur = 0;                                                        // dur of a segment in same units

   int n = 0;                                                                 // # of trial codes prepared so far
   if( nMax < (n + 4 + nTargs*2) )                                            // abort if trial code buffer too small
   {
      *pN = 0;
      pApp->LogMessage( strErr );
      return( FALSE );
   }

   pCodes[n].code = STARTTRIAL;                                               // STARTTRIAL: always the first code
   pCodes[n++].time = shFrame;

   // REWARDLEN: The two reward pulse lengths associated with the trial. While most trials simply use reward #1 as the
   // end-of-trial reward, some special features require two reward pulses. Either reward may be randomly withheld, if
   // that feature is enabled in the trial's definition. If withheld, the pulse length is set to 0. Alternatively, if
   // the trial's reward pulse lengths are overridden in the application settings, the overrides are used instead.
   pCodes[n].code = REWARDLEN; 
   pCodes[n++].time = shFrame; 
   if( pSet->IsTrialRewLenOverride() ) 
   { 
      pCodes[n].code = short(pSet->GetScaledRewardPulseLen(pSet->GetRewardLen1()) );
      pCodes[n++].time = short(pSet->GetScaledRewardPulseLen(pSet->GetRewardLen2()) );
   }
   else 
   {
      BOOL bGiveRew1 = TRUE, bGiveRew2 = TRUE;
      pTrial->UpdateRewardWHVR(bGiveRew1, bGiveRew2);
      pCodes[n].code = short( bGiveRew1 ? pSet->GetScaledRewardPulseLen(pTrial->GetReward1PulseLen()) : 0 );
      pCodes[n++].time = short( bGiveRew2 ? pSet->GetScaledRewardPulseLen(pTrial->GetReward2PulseLen()) : 0 );
   }

   pCodes[n].code = MIDTRIALREW;                                              // MIDTRIALREW: defines parameters that
   pCodes[n++].time = shFrame;                                                // govern any mid-trial rewards delivered
   if( pTrial->IsMidTrialRewPeriodic() )
      pCodes[n].code = short( pTrial->GetMidTrialRewardIntv() );
   else
      pCodes[n].code = short(0);
   pCodes[n++].time = short( pSet->GetScaledRewardPulseLen(pTrial->GetMidTrialRewardLen()) );

   int iT0Seg = pTrial->GetMarkSeg1Pos();                                     // get segment range spanning portion of
   int iT1Seg = pTrial->GetMarkSeg2Pos();                                     // trial to be shown in trace display
   BOOL isFullTrialDisplayed = TRUE;
   int nSegs = pTrial->SegCount();
   if( iT0Seg >= 0 && iT0Seg < nSegs && iT1Seg >= 0 && iT1Seg < nSegs )
   {
      if( iT0Seg > iT1Seg )                                                   // the trial's "mark" segments may not
      {                                                                       // be in ascending order!
         int tmp = iT0Seg;
         iT0Seg = iT1Seg;
         iT1Seg = tmp;
      }
      if( iT0Seg > 0 || iT1Seg < nSegs - 1 )
         isFullTrialDisplayed = FALSE;
   }

   for( int iSeg = 0; iSeg < pTrial->SegCount(); iSeg++ )                     // BEGIN: Processing trial segments...
   {
      if( !isFullTrialDisplayed )                                             // remember start of first seg and END of
      {                                                                       // last seg spanning portion of trial to
         if( iSeg == iT0Seg ) *piT0 = (int) shFrame;                          // be shown on data trace display (no
         if( iSeg == iT1Seg+1 ) *piT1 = (int) shFrame;                        // effect on trial codes!)
      }

      for( j = 0; j < nTargs; j++ )                                           // BEGIN: Process tgt motion for all
      {                                                                       // participating tgts in current seg.
         if( (n + 20) > nMax )                                                //    abort if trial code buf too small
         {
            *pN = 0;
            pApp->LogMessage( strErr );
            return( FALSE );
         }

         WORD wType = pTargs[j]->DataType();                                  //    target category 

         // !!!IMPORTANT!!!  (effective 26apr2005) We ALWAYS send the TARGET_ON/OFF code for the first participating
         // target to guarantee that at least one trial code is sent per trial segment. Prior to this change, it was
         // possible to construct a trial for which this was not the case (ie, if nothing changed from one segment to
         // the next). We do this so that CXDRIVER and analysis code can reconstruct the trial segments by examining
         // trial codes.

         BOOL bIsOn = pTrial->IsTgtOn(iSeg, j);                               //    is tgt on this seg?
         BOOL bWasOn = (iSeg>0) && pTrial->IsTgtOn(iSeg-1, j);                //    was tgt on last seg?
         if(j==0 || (bIsOn != bWasOn))                                        //    TARGET_ON/OFF: see note above!
         {
            pCodes[n].code = bIsOn ? TARGET_ON : TARGET_OFF;
            pCodes[n++].time = shFrame;
            pCodes[n++].code = j;
         }

         BOOL bSmall;

         // INSIDE_***VEL: mark any change in target pattern velocity... only applies to RMVideo targets
         double dLastH = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::PATHVEL) : 0.0;  
         double dLastV = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::PATVVEL) : 0.0; 
         double dNowH  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::PATHVEL);
         double dNowV  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::PATVVEL);
         if((wType==CX_RMVTARG) && (dLastH!=dNowH || dLastV!=dNowV))
         {
            RotateAndScaleVector(dNowH, dNowV, FALSE, pTrial);                //    rot/scale vel vec IAW curr settings

            bSmall = BOOL((dNowH > -10.0) && (dNowH < 10.0));                 //    small values encoded differently...
            pCodes[n].code = bSmall ? INSIDE_HSLOVEL : INSIDE_HVEL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowH * (bSmall ? d_TC_SLOSCALE1 : d_TC_STDSCALE));

            bSmall = BOOL((dNowV > -10.0) && (dNowV < 10.0));
            pCodes[n].code = bSmall ? INSIDE_VSLOVEL : INSIDE_VVEL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowV * (bSmall ? d_TC_SLOSCALE1 : d_TC_STDSCALE));
         }

         // INSIDE_***ACC: mark any change in target pattern acceleration... only applies to RMVideo targets
         dLastH = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::PATHACC) : 0.0;
         dLastV = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::PATVACC) : 0.0;
         dNowH  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::PATHACC);
         dNowV  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::PATVACC);
         if( (wType == CX_RMVTARG) && (dLastH != dNowH || dLastV != dNowV) )
         {
            RotateAndScaleVector( dNowH, dNowV, FALSE, pTrial );              //    rot/scale acc vec IAW curr settings

            bSmall = BOOL((dNowH > -100.0) && (dNowH < 100.0));               //    small values encoded differently...
            pCodes[n].code = bSmall ? INSIDE_HSLOACC : INSIDE_HACC;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowH * (bSmall ? d_TC_SLOSCALE2 : 1.0));

            bSmall = BOOL((dNowV > -100.0) && (dNowV < 100.0));
            pCodes[n].code = bSmall ? INSIDE_VSLOACC : INSIDE_VACC;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowV * (bSmall ? d_TC_SLOSCALE2 : 1.0));
         }

         // TARGET_***VEL: mark any change in target window velocity...
         dLastH = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTHVEL) : 0.0;
         dLastV = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTVVEL) : 0.0;
         dNowH  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTHVEL);
         dNowV  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTVVEL);

         // BUG FIX: MUST send vel code if tgt accelerated during prev segment!
         BOOL bDidAccel = (iSeg>0) && ((pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTHACC) != 0) || 
               (pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTVACC) != 0));

         if(wType == CX_CHAIR) dLastV = dNowV = 0.0;                        //    (animal chair has no verti coord!)
         if(dLastH != dNowH || dLastV != dNowV || bDidAccel)
         {
            RotateAndScaleVector( dNowH, dNowV, FALSE, pTrial );              //    rot/scale vel vec IAW curr settings
            if( wType == CX_CHAIR ) dNowH = -dNowH;                           //    so chair rotates in the right dir!

            bSmall = BOOL((dNowH > -10.0) && (dNowH < 10.0) &&                //    small values encoded differently
                          (wType == CX_RMVTARG));                             //    for RMVideo targets...
            pCodes[n].code = bSmall ? TARGET_HSLOVEL : TARGET_HVEL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowH * (bSmall ? d_TC_SLOSCALE1 : d_TC_STDSCALE));

            bSmall = BOOL((dNowV > -10.0) && (dNowV < 10.0) && (wType == CX_RMVTARG));
            pCodes[n].code = bSmall ? TARGET_VSLOVEL : TARGET_VVEL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowV * (bSmall ? d_TC_SLOSCALE1 : d_TC_STDSCALE));
         }

         // TARGET_***ACC: mark any change in target window acceleration...
         dLastH = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTHACC) : 0.0; 
         dLastV = (iSeg>0) ? pTrial->GetCurrTgtTrajParam(iSeg-1, j, CCxTrial::TGTVACC) : 0.0;
         dNowH  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTHACC);
         dNowV  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTVACC);
         if( wType == CX_CHAIR ) dLastV = dNowV = 0.0;                        //    (animal chair has no verti coord!)
         if( dLastH != dNowH || dLastV != dNowV ) 
         { 
            RotateAndScaleVector( dNowH, dNowV, FALSE, pTrial );              //    rot/scale acc vec IAW curr settings
            if( wType == CX_CHAIR ) dNowH = -dNowH;                           //    so chair rotates in the right dir!

            bSmall = BOOL((dNowH > -100.0) && (dNowH < 100.0) &&              //    small values encoded differently
                          (wType == CX_RMVTARG));                             //    for RMVideo targets...
            pCodes[n].code = bSmall ? TARGET_HSLOACC : TARGET_HACC;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowH * (bSmall ? d_TC_SLOSCALE2 : 1.0));

            bSmall = BOOL((dNowV > -100.0) && (dNowV < 100.0) && (wType == CX_RMVTARG));
            pCodes[n].code = bSmall ? TARGET_VSLOACC : TARGET_VACC;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowV * (bSmall ? d_TC_SLOSCALE2 : 1.0));
         }

         // TARGET_*POS*** : mark any change in target window position. Note: Cannot control animal chair by pos.
         dNowH  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTHPOS);
         dNowV  = pTrial->GetCurrTgtTrajParam(iSeg, j, CCxTrial::TGTVPOS); 
         BOOL bAbs = pTrial->IsAbsolutePos(iSeg, j); 
         if( (wType != CX_CHAIR) &&
             (iSeg == 0 || bAbs || dNowH != 0.0 || dNowV != 0.0) )
         {
            // rotate/scale pos IAW curr settings -- UNLESS we're in segment 0 and target is positioned absolutely
            if(iSeg > 0 || !bAbs)
               RotateAndScaleVector( dNowH, dNowV, TRUE, pTrial ); 
            
            // have tgt start trial relative to global tgt starting pos -- UNLESS pos is set absolutely in seg 0
            if((iSeg == 0) && !bAbs) 
            { 
               dNowH += GetStartingPosH(); 
               dNowV += GetStartingPosV();
            }

            pCodes[n].code = bAbs ? TARGET_HPOSABS : TARGET_HPOSREL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowH * d_TC_SLOSCALE2);

            pCodes[n].code = bAbs ? TARGET_VPOSABS : TARGET_VPOSREL;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;
            pCodes[n++].time = (short) (dNowV * d_TC_SLOSCALE2);
         }

         int iMode = pTrial->GetTgtVStabMode(iSeg, j);                        //    TARGET_VSTAB:  mark any change in 
         int iLastMode =                                                      //    target's vel. stab. mode. ALWAYS 
            (iSeg>0) ? pTrial->GetTgtVStabMode(iSeg-1, j) : SGTJ_VSTABOFF;    //    send AFTER TARGET_*POS***!
         if( iMode != iLastMode )
         {
            pCodes[n].code = TARGET_VSTAB;
            pCodes[n++].time = shFrame;
            pCodes[n].code = j;

            WORD vsflags = 0;
            if( iMode != SGTJ_VSTABOFF )
            {
               vsflags |= (VSTAB_ON | VSTAB_H | VSTAB_V);
               if( iMode == SGTJ_VSTABHONLY ) vsflags &= ~VSTAB_V;
               else if( iMode == SGTJ_VSTABVONLY ) vsflags &= ~VSTAB_H;
               if( iLastMode==SGTJ_VSTABOFF && pTrial->IsTgtVStabSnapToEye(iSeg, j) )
                  vsflags |= VSTAB_SNAP;
            }
            pCodes[n++].time = (short) vsflags;
         }
      }                                                                       // END: Process tgt motion...


      for(j=0; j<pTrial->PertCount(); j++) if(pTrial->GetPertSeg(j) == iSeg)  // BEGIN: Process any perturbations
      {                                                                       // starting during this segment...
         if( (n + 5) > nMax )                                                 //    do we have enough codes left?
         {
            *pN = 0;
            pApp->LogMessage( strErr );
            return( FALSE );
         }

         PERT pertDef;                                                        //    get defn of relevant perturbation
         CCxPert* pPert = (CCxPert*) pDoc->GetObject(pTrial->GetPertKey( j ));
         ASSERT( pPert );
         ASSERT_KINDOF( CCxPert, pPert );
         pPert->GetPertInfo( pertDef );

         pCodes[n].code = TARGET_PERTURB;                                     //    prepare TARGET_PERTURB trial
         pCodes[n++].time = shFrame;                                          //    code group...
         pCodes[n].code = short(pTrial->GetPertTgt( j ));                     //       code1 = index in trial tgt map
         pCodes[n++].time = (short(pTrial->GetPertTrajCmpt( j )) << 4)        //       time1 = vel ID << 4 | pert type
                            | short(pertDef.iType);
         pCodes[n].code = short(10.0f * pTrial->GetPertAmp( j ));             //       code2 = pert amp * 10
         pCodes[n++].time = short(pertDef.iDur);                              //       time2 = pert dur in ms

         int iSeed = 0;                                                       //    seed for noise perts
         switch( pertDef.iType )
         {
            case PERT_ISSINE :                                                //    for sinewave perturbation...
               pCodes[n].code = short(pertDef.sine.iPeriod);                  //       code3 = period in ms
               pCodes[n++].time = short(100.0f * pertDef.sine.fPhase);        //       time3 = phase in deg/100
               pCodes[n].code = 0;                                            //       code4,time4 = not used
               pCodes[n++].time = 0;
               break;
            case PERT_ISTRAIN :                                               //    for trapezoidal pulse train pert...
               pCodes[n].code = short(pertDef.train.iPulseDur);               //       code3 = pulse dur in ms
               pCodes[n++].time = short(pertDef.train.iRampDur);              //       time3 = ramp dur in ms
               pCodes[n].code = short(pertDef.train.iIntv);                   //       code4 = pulse intv in ms
               pCodes[n++].time = 0;                                          //       time4 = not used
               break;
            case PERT_ISNOISE :                                               //    for uniform or gaussian random
            case PERT_ISGAUSS :                                               //    noise perturbations...
               pCodes[n].code = short(pertDef.noise.iUpdIntv);                //       code3 = update intv in ms
               pCodes[n++].time = short(1000.0f * pertDef.noise.fMean);       //       time3 = mean * 1000
               iSeed = pertDef.noise.iSeed;                                   //       if seed param is zero, then we
               if( iSeed == 0 )                                               //       generate a random seed each
                  iSeed = CCxPert::GetRandomSeed();                           //       time perturbation is used
               pCodes[n].code = HIWORD(iSeed);                                //       code4 = HIWORD(seed)
               pCodes[n++].time = LOWORD(iSeed);                              //       time4 = LOWORD(seed)
               break;
            default :
               ASSERT( FALSE );
               return( FALSE );
         }
      }                                                                       // END: Process any perturbations...


      if( (n + 26) > nMax )                                                   // make sure we won't exceed buffer size!
      {
         *pN = 0;
         pApp->LogMessage( strErr );
         return( FALSE );
      }

      if( iSeg == pTrial->GetSaveSegPos() )                                   // ADCON:  record data from start of the
      {                                                                       // "save segment" to the end of trial
         pCodes[n].code = ADCON;
         pCodes[n++].time = shFrame;
      }

      if( iSeg == pTrial->GetFailsafeSegPos() )                               // FAILSAFE:  set the failsafe time.  if
      {                                                                       // trial stops before this time, it is
         pCodes[n].code = FAILSAFE;                                           // discarded.
         pCodes[n++].time = shFrame;
      }

      if(m_ctrl.iTrialSeq == STAIRCASE || m_ctrl.iTrialSeq == STAIRCASE_NF)   // when running in a staircase mode, we
      {                                                                       // must check subject's response during
         BOOL bIsOn = pTrial->IsResponseChecked(iSeg);                        // selected segs...
         BOOL bWasOn = (iSeg>0) && pTrial->IsResponseChecked(iSeg-1);
         if( bIsOn && !bWasOn )                                               // CHECKRESPON:  turn resp-checking on
         {
            pCodes[n].code = CHECKRESPON;
            pCodes[n++].time = shFrame;
            pCodes[n].code = (short) pTrial->GetCorrectResponseChan();
            pCodes[n++].time = (short) pTrial->GetIncorrectResponseChan();
         }
         else if( bWasOn && !bIsOn )                                          // CHECKRESPOFF:  turn resp-checking off
         {
            pCodes[n].code = CHECKRESPOFF;
            pCodes[n++].time = shFrame;
         }
      }

      // PULSE_ON: mark start of segment with a TTL pulse on DOUT<N>, N > 0. Also (as of Maestro 4.0.0/RMVideo v8),
      // optionally trigger a vertical sync "spot flash" in top-left corner of RMVideo display.
      i = pTrial->GetMarker(iSeg); 
      BOOL bDoSync = (!pSet->IsRMVSyncFlashDisabled()) && pTrial->IsRMVSyncFlashOn(iSeg);
      if(i > 0 || bDoSync) 
      {
         pCodes[n].code = PULSE_ON;
         pCodes[n++].time = shFrame;
         pCodes[n].code = (short) i;
         pCodes[n++].time = (short) (bDoSync ? 1 : 0);
      }

      // FIXEYE1: specify fixation tgt#1 AND "mid-trial reward enabled" flag state -- if either have changed
      int iFixLast = (iSeg>0) ? pTrial->GetFixTarg1Pos(iSeg-1) : -1;  
      int iFixNow = pTrial->GetFixTarg1Pos(iSeg); 
      BOOL bRewEnaLast = (iSeg>0) ? pTrial->IsMidTrialRewEnable(iSeg-1) : FALSE; 
      BOOL bRewEnaNow = pTrial->IsMidTrialRewEnable(iSeg);
      if( (iSeg == 0) || (iFixLast != iFixNow) || (bRewEnaLast != bRewEnaNow) )
      {
         pCodes[n].code = FIXEYE1;
         pCodes[n++].time = shFrame;
         pCodes[n].code = (iFixNow < 0) ? ((short) -1) : iFixNow;             // (-1 == no fixation tgt defined)
         pCodes[n++].time = bRewEnaNow ? 1 : 0;
      }

      iFixLast = (iSeg>0) ? pTrial->GetFixTarg2Pos(iSeg-1) : -1;
      iFixNow = pTrial->GetFixTarg2Pos(iSeg);
      if( (iSeg == 0) || (iFixLast != iFixNow) )                              // FIXEYE2:  specify fixation tgt #2 for
      {                                                                       // this segment
         pCodes[n].code = FIXEYE2;
         pCodes[n++].time = shFrame;
         pCodes[n++].code = (iFixNow < 0) ? ((short) -1) : iFixNow;           // (-1 == no fixation tgt defined)
      }

      // !!!!! IMPORTANT !!!!!
      // We MUST send SPECIALOP trial codes after FIXEYE* codes, because CXDRIVER may need to know what the fixation
      // targets are for the special segment when SPECIALOP is processed.

      if( bDoesSpecOp && (iSeg == pTrial->GetSpecialSegPos()) )               // SPECIALOP: for certain "special" ops
      {                                                                       // during one designated seg of trial.
         pCodes[n].code = SPECIALOP;
         pCodes[n++].time = shFrame;
         if(iSpecOp == TH_SOP_SKIP)           pCodes[n].code = SPECIAL_SKIP;
         else if(iSpecOp == TH_SOP_SELBYFIX)  pCodes[n].code = SPECIAL_FIX;
         else if(iSpecOp == TH_SOP_SELBYFIX2) pCodes[n].code = SPECIAL_FIX2;
         else if(iSpecOp == TH_SOP_SWITCHFIX) pCodes[n].code = SPECIAL_SWITCHFIX;
         else if(iSpecOp == TH_SOP_RPDISTRO)
         {
            // for RP Distro op, behav resp type is included in optype code, left-shifted 8 bits
            pCodes[n].code = SPECIAL_RPDISTRO | ((pTrial->GetRPDistro()->GetResponseType()) << 8);
         }
         else if(iSpecOp == TH_SOP_CHOOSEFIX1) pCodes[n].code = SPECIAL_CHOOSEFIX1;
         else if(iSpecOp == TH_SOP_CHOOSEFIX2) pCodes[n].code = SPECIAL_CHOOSEFIX2;
         else if(iSpecOp == TH_SOP_SEARCH)     pCodes[n].code = SPECIAL_SEARCH;
         else if(iSpecOp == TH_SOP_SELDUR)     pCodes[n].code = SPECIAL_SELDURBYFIX;
         else                                  pCodes[n].code = SPECIAL_FINDANDWAIT;
         
         pCodes[n++].time = (short) pTrial->GetSaccadeThreshold();

         if( iSpecOp == TH_SOP_RPDISTRO )                                     // RPDWINDOW: For "R/P Distro" trials
         {                                                                    // with an enabled "reward window", send
            CCxRPDistro* pDistro = pTrial->GetRPDistro();                     // window bounds via this trial code grp.
            if( pDistro->IsRewardWinEnabled() )                               // Prior to v1.5.0, there were 2 windows,
            {                                                                 // now only 1.  But RPDWINDOW still has
               pCodes[n].code = RPDWINDOW;                                    // room for defining 2 windows.
               pCodes[n++].time = shFrame;

               float x0 = pDistro->GetRewardWinMinimum();
               float x1 = pDistro->GetRewardWinMaximum();
               pCodes[n].code = (short) (double(x0) * d_TC_STDSCALE);
               pCodes[n++].time = (short) (double(x1) * d_TC_STDSCALE);
               pCodes[n].code = (short) 0;                                    // 2nd window always disabled
               pCodes[n++].time = (short) 0;
            }
         }
      }
      
      // process segment duration:
      // 1) FAIL if min duration > max duration.
      // 2) IF the current segment is the one AFTER the special segment in a "selDurByFix" trial, the actual duration
      // of that segment will be the min value if Fix1 was selected, else it will be the max value. IN THIS ONE 
      // SCENARIO, we must send the two possible durations to CXDRIVER, and prepare the remaining trial codes UNDER THE
      // ASSUMPTION THAT THE MAX DURATION WAS CHOSEN. CXDRIVER will make the necessary adjustments during trial runtime
      // IF Fix1 was selected.
      // 3) OTHERWISE, if max == min, then that is the segment duration. If max > min, we (crudely) select a random
      // value within the range [min .. max].
      shSegDur = (short) pTrial->GetCurrMinDuration(iSeg); 
      short shMaxDur = (short) pTrial->GetCurrMaxDuration(iSeg);
      if(shSegDur > shMaxDur)
      {
         *pN = 0;
         strErr.Format("!! Trial _%s_: Segment %d has min duration > max !!", pTrial->Name(), iSeg);
         pApp->LogMessage(strErr);
         return(FALSE);
      }

      if((iSpecOp == TH_SOP_SELDUR) && (iSeg == pTrial->GetSpecialSegPos() + 1))
      {
         pCodes[n].code = SEGDURS; 
         pCodes[n++].time = shFrame;
         pCodes[n].code = shSegDur;
         pCodes[n++].time = shMaxDur;
         shSegDur = shMaxDur;
      }
      else if(shMaxDur > shSegDur)
      {
         i = rand() & 0x00ff;
         i *= (int) (shMaxDur-shSegDur);
         i /= 0x00ff;
         shSegDur += (short) i;
      }

      // check to see if elapsed trial time exceeds the allowed maximum. We're restricted b/c trial codes, including elapsed
      // times, are stored as short integers!
      int elapsedTime = (int) shFrame;
      elapsedTime += shSegDur;
      if(elapsedTime > MAXTRIALDUR)
      {
         *pN = 0;
         strErr.Format("!! Trial _%s_: Trial duration exceeds %d ms !!", pTrial->Name(), MAXTRIALDUR); 
         pApp->LogMessage( strErr );
         return(FALSE);
      }


      // The FIXACCURACY trial code group  !!!!! IMPORTANT !!!!!
      // We MUST compute the segment duration before sending the FIXACCURACY code, as the processing of that trial code
      // may require knowledge of the segment duration.  FIXACCURACY must be the last trial code sent for the current
      // segment.  This is because the grace time implementation is TRICKY.  When the grace time is nonzero and less
      // than the segment duration, two FIXACCURACY trial code groups are set.  One is sent at the start of the segment
      // and sets the accuracy to a huge number (so a fixation break cannot happen).  Then another is sent at
      // t = tSegStart + grace time!  If we sent another trial code after this one, the codes would no longer be in
      // chronological order!!!
      //
      // If either "select by fixation" op or "selDurByFix" is in effect during the current seg, normal fixation 
      // checking is disabled. The H,V fixation accuracy parameters, however, are used to specify the "selection 
      // window", so they must be sent to CXDRIVER -- even if we're running in one of the "nofix" modes. The grace 
      // time is ignored.
      //
      // For the "searchTask" and "findAndWait" ops, fixation checking is also disabled during the special segment. The
      // grace period is needed as well as the fixation accuracies, even in one of the "nofix" modes. For these ops,
      // fixation accuracies define the "target is selected" window, and the grace period indicates how long the
      // subject must stay on the target to satisfy the task.
      // 
      // The "choose fix tgt" ops are another special case. Here, the fixation window is used to determine if and 
      // when the eye is close enough to the correct target during the special segment. At that point, fixation 
      // checking is turned ON and the wrong target is turned off.  In "nofix" modes, we still send a very large 
      // fixation accuracy so that fixation checking is effectively disabled for the entire segment. As a result, the 
      // "correct" target is always chosen and the "wrong" target turned off at the start of the special segment. 
      // Also, the grace period is ignored when a "choose fix" op is in effect
      //
      // NOTE that FIXACCURACY transmits both H & V fixation accuracies!!

      BOOL bIsSelByFix = FALSE;
      BOOL bIsChooseFix = FALSE;
      BOOL bIsSearch = FALSE;
      if(iSeg == pTrial->GetSpecialSegPos())
      {
         bIsSelByFix = BOOL(iSpecOp == TH_SOP_SELBYFIX || iSpecOp == TH_SOP_SELBYFIX2 || iSpecOp == TH_SOP_SELDUR);
         bIsChooseFix = BOOL(iSpecOp == TH_SOP_CHOOSEFIX1 || iSpecOp == TH_SOP_CHOOSEFIX2);
         bIsSearch = BOOL(iSpecOp == TH_SOP_SEARCH || iSpecOp == TH_SOP_FINDWAIT);
      }
      short shAccH = (short) (pTrial->GetFixAccH(iSeg) * d_TC_SLOSCALE2);
      short shAccV = (short) (pTrial->GetFixAccV(iSeg) * d_TC_SLOSCALE2);
      short shGrace = (short) pTrial->GetGracePeriod(iSeg);
      if( IsNoFixMode() && !bIsSearch)
      {
         pCodes[n].code = FIXACCURACY;
         pCodes[n++].time = shFrame;
         pCodes[n].code = bIsSelByFix ? shAccH : 32000;
         pCodes[n++].time = bIsSelByFix ? shAccV : 32000;
      }
      else if( (shGrace > 0) && !(bIsSelByFix || bIsChooseFix) )
      {
         pCodes[n].code = FIXACCURACY;
         pCodes[n++].time = shFrame;
         pCodes[n].code = 32000;
         pCodes[n++].time = 32000;
         if( shGrace < shSegDur )
         {
            pCodes[n].code = FIXACCURACY;
            pCodes[n++].time = shFrame + shGrace;
            pCodes[n].code = shAccH;
            pCodes[n++].time = shAccV;
         }
      }
      else
      {
         pCodes[n].code = FIXACCURACY;
         pCodes[n++].time = shFrame;
         pCodes[n].code = shAccH;
         pCodes[n++].time = shAccV;
      }

      shFrame += shSegDur;                                                    // add seg's dur to trial's elapsed time
   }                                                                          // END: Processing trial segments

   if( n == nMax )
   {
      *pN = 0;
      pApp->LogMessage( strErr );
      return( FALSE );
   }

   pCodes[n].code = ENDTRIAL;                                                 // ENDTRIAL:  stop the trial
   pCodes[n++].time = shFrame;

   *piDur = (int) shFrame;                                                    // trial's total duration in ms
   *pN = n;                                                                   // total # of trial codes prepared

   if( (!isFullTrialDisplayed) && (iT1Seg == nSegs-1) )                       // if we're displaying only part of the
   {                                                                          // trial and the displayed part runs to
      *piT1 = shFrame;                                                        // end of trial, get endpt "t1"
   }

   *pNSects = pTrial->GetNumTaggedSections();                                 // store info on any tagged sections
   for( i=0; i<pTrial->GetNumTaggedSections(); i++ )                          // defined on the trial
   {
      VERIFY( pTrial->GetTaggedSection(i, pSections[i]) );
   }

   // get state of trial's bit flags
   TRLHDR hdr;
   pTrial->GetHeader(hdr);
   *pFlags = hdr.dwFlags;
   
   // append trial bit flags specific to the "chained" sequencer modes: to mark a trial as participating in a chained
   // sequence, and to mark those trials that constitute the start of a chain of consecutive reps of the same trial.
   // NOTE that this is subtly different from the programmed trial chains. For example, if the programmed chains are
   // presented as "3A, 4A, 2B, 2A" and all are successful, then the trials marked as "starting a chain" are the first
   // of "3A", the first of "2B", and the first of "2A". "3A,4A" is effectively a "7A" chain...
   if(IsChainedMode())
   {
      *pFlags = (*pFlags) | THF_CHAINED;

      // starting a new sequence or resuming a paused sequence always breaks any ongoing chain of successful reps of
      // the same trial.
      if(m_bSeqStart || m_bSeqPaused) m_nConsecutiveRepsOK = 0;

      if(m_nConsecutiveRepsOK == 0) *pFlags = (*pFlags) | THF_CHAINSTART;
   }

   m_bSeqStart = FALSE;
   m_bSeqPaused = FALSE;

   return( TRUE );
}


/**
 Configure the trial sequencer's "auto-stop" feature. The sequencer can be configured to automatically stop after a 
 specified  number of trials OR trial blocks have been completed, or not at all. If the block count criterion is used 
 in a sequencer mode which does not count trial blocks (the "Current Trial" and "Staircase" modes), then a single
 trial is treated as a block. Also note that in the "Ordered" modes, the block count is incremented regardless of 
 whether or not all the trials in that block were completed successfully, since there's no facility for repeating 
 failed trials. The block count criterion is really only appropriate in the "Randomized" modes.

 Method should be called immediately before starting the trial sequence.

 @param mode The auto-stop mode: AUTOSTOP_OFF, AUTOSTOP_TRIALS, AUTOSTOP_BLKS. If invalid, AUTOSTOP_OFF is used.
 @param count The auto-stop count. Restricted to positive integer values.
*/
VOID CCxTrialSequencer::SetAutoStopParams(int mode, int count)
{
   m_iAutoStopMode = (mode < AUTOSTOP_OFF || mode >= NUMAUTOSTOPMODES) ? AUTOSTOP_OFF : mode;
   m_nAutoStopCount = (count <= 0) ? 1 : count;
}


//=== Get/SetStartingPosH/V ===========================================================================================
//
//    Get/set the global starting position for all targets participating in the next trial.  Each target will start
//    moving from this position at the beginning of the trial, unless the target's window position is nonzero in the
//    first segment of the trial.  This parameter may be modified between trials in a sequence, if desired.
//
//    ARGS:       hPos, vPos  -- [in] the horiz and vertical components of the desired global starting position (deg)
//
//    RETURNS:    NONE, or the requested component in degrees subtended at the eye.
//
double CCxTrialSequencer::GetStartingPosH() const
{
   return( (double) m_startTgtPos.GetH() );
}

void CCxTrialSequencer::SetStartingPosH( double hPos )
{
   m_startTgtPos.SetH( hPos );
}

double CCxTrialSequencer::GetStartingPosV() const
{
   return( (double) m_startTgtPos.GetV() );
}

void CCxTrialSequencer::SetStartingPosV( double vPos )
{
   m_startTgtPos.SetV( vPos );
}


//=== Get/SetTgtPos/VelScale/Rotate ===================================================================================
//
//    Get/set the global factors for scaling and rotating target window position and velocity vectors specified in each
//    trial's  segment table.  These factors may be modified between trials in an ongoing sequence, if desired.
//
//    ARGS:       d  -- [in] the new global position/velocity scale factor (unitless) or rotation (deg).
//
//    RETURNS:    NONE, or the requested scale factor (unitless) or rotation (deg)
//
double CCxTrialSequencer::GetTgtPosScale() const { return( m_dPosScale ); }
VOID CCxTrialSequencer::SetTgtPosScale( double d ) { m_dPosScale = d; }
double CCxTrialSequencer::GetTgtPosRotate() const { return( m_dPosRotate ); }
VOID CCxTrialSequencer::SetTgtPosRotate( double d ) { m_dPosRotate = d; }

double CCxTrialSequencer::GetTgtVelScale() const { return( m_dVelScale ); }
VOID CCxTrialSequencer::SetTgtVelScale( double d ) { m_dVelScale = d; }
double CCxTrialSequencer::GetTgtVelRotate() const { return( m_dVelRotate ); }
VOID CCxTrialSequencer::SetTgtVelRotate( double d ) { m_dVelRotate = d; }

//=== Is/SetChanCfgOverride(), Get/SetChanCfgOverrideKey() ============================================================
//
BOOL CCxTrialSequencer::IsChanCfgOverride() const { return( m_bUseChan ); }
VOID CCxTrialSequencer::SetChanCfgOverride( BOOL on ) { m_bUseChan = on; }
WORD CCxTrialSequencer::GetChanCfgOverrideKey() const { return( m_wChanOvrKey ); }
VOID CCxTrialSequencer::SetChanCfgOverrideKey( WORD wKey ) { m_wChanOvrKey = wKey; }




//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== RotateAndScaleVector ============================================================================================
//
//    Rotate & scale a vector IAW current transform settings, UNLESS the trial object indicates that the transform 
//    should be ignored.
//
//    ARGS:       dH -- [in/out] the horiz component of vector.
//                dV -- [in/out] the verti component of vector.
//                bPos -- [in] if TRUE, use position transform; else use velocity transform.
//                pTrial -- [in] the particular trial for which tgt pos/vel vectors are being prepared.  Transforms 
//                          may be ignored on a per-trial basis.
//    RETURNS:    NONE.
//
VOID CCxTrialSequencer::RotateAndScaleVector( double& dH, double& dV, const BOOL bPos, const CCxTrial* pTrial ) const
{
   double dRot = 0.0;
   double dScale = 1.0;
   if( bPos )
   {
      if( !pTrial->IsScalePosIgnored() ) dScale = m_dPosScale;
      if( !pTrial->IsRotatePosIgnored() ) dRot = m_dPosRotate;
   }
   else
   {
      if( !pTrial->IsScaleVelIgnored() ) dScale = m_dVelScale;
      if( !pTrial->IsRotateVelIgnored() ) dRot = m_dVelRotate;
   }

   double dTheta = ::atan2( dV, dH );                                               // 0 radians if both cmpts are 0!
   dTheta += (dRot * 3.14159265) / 180.0;
   double dLen = ::sqrt( dH*dH + dV*dV );
   dLen *= dScale;
   dH = dLen * ::cos( dTheta );
   dV = dLen * ::sin( dTheta );
}


//=== Reset ===========================================================================================================
//
//    Reset all internal sequencer control parameters.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialSequencer::Reset()
{
   m_bInitialized = FALSE;

   // init sequencer control params to a default "empty" state -- no trial set defined. Other params set to defaults.
   m_ctrl.wTrialSet = CX_NULLOBJ_KEY; 
   m_ctrl.wCurrTrial = CX_NULLOBJ_KEY;
   m_ctrl.iSubsetSeq = SUBSETSEQ_OFF;
   m_ctrl.iTrialSeq = THISTRIAL_NF;
   m_ctrl.dStairStrength = 1.0;
   m_ctrl.nStairIrrel = 50;
   m_ctrl.nWrongUp = 2;
   m_ctrl.nRightDn = 2;
   m_ctrl.nReversals = 0;

   // last trial selected by sequencer is undefined on reset, indicating that first trial has not yet been selected.
   m_iSelected = -1; 

   // empty internal target and trial arrays without releasing memory allocated to them.
   m_arTargets.RemoveAt(0, m_arTargets.GetSize());
   m_arTrials.RemoveAt(0, m_arTrials.GetSize()); 
   m_arNumRepsLeft.RemoveAt(0, m_arNumRepsLeft.GetSize());
   m_iTotalRepsLeft = 0;

   // release memory for any stat objects (but without releasing memory for the array itself)
   for(int i=0; i<m_Stats.GetSize(); i++)
   {
      CStat* pStat = m_Stats.GetAt(i);
      if(pStat != NULL) delete pStat;
   }
   m_Stats.RemoveAt(0, m_Stats.GetSize());

   // release memory for any trial subset records
   while(!m_Subsets.IsEmpty())
   {
      CSubset* pSubset = m_Subsets.RemoveHead();
      if(pSubset != NULL) delete pSubset;
   }
   m_iCurrSubset = -1;

   // release memory for any chains
   while( !m_Chains.IsEmpty())
   {
      CChain* pChain = m_Chains.RemoveHead();
      if(pChain != NULL) delete pChain;
   }
   m_iCurrChain = -1;
   m_nCurrChainReps = 0;

   m_nConsecutiveRepsOK = 0;

   m_nStairs = 0;
   m_iCurrStair = -1;
   m_nIrrelevant = 0;
   m_nCorrectIrrel = 0;

   m_nTrialsDone = 0;
   m_nBlocksDone = 0;

   m_bSeqStart = TRUE;
   m_bSeqPaused = FALSE;
}


/**
 Helper method shuffles the list of trial subset records. It is called after the sequencer has finished presenting the
 trials in the last subset of the previous block and only when the subset sequencing mode is SUBSETSEQ_RANDOM.
*/
VOID CCxTrialSequencer::ShuffleSubsets()
{
   if(m_ctrl.iSubsetSeq == SUBSETSEQ_RANDOM && m_Subsets.GetSize() > 1)
   {
      int n = static_cast<int>(m_Subsets.GetSize());
      for(int i=0; i<n-1; i++)
      {
         int iPick = i + ((rand() * (n-i)) / RAND_MAX);
         if(iPick == n) iPick = n - 1;

         POSITION pos = m_Subsets.FindIndex(iPick);
         ASSERT(pos != NULL);
         CSubset* pSubset = m_Subsets.GetAt(pos);
         m_Subsets.RemoveAt(pos);
         m_Subsets.AddHead(pSubset);
      }
   }
}


//=== InitWeightedReps, GetNextWeightedTrial ===========================================================================
//
//    The RANDOM and WT_ORDERED sequencer modes take into account each trial's weight.
//
//    In the RANDOM modes, trials are presented randomly with frequency of presentation determined by the trial weight.
//    Thus, a trial with a weight of 10 is presented a total of 10 times over the course of a trial "block", while a
//    a trial with a weight of 1 is only presented once during that block, and a trial with weight 0 is not presented
//    at all. In the WT_ORDERED modes, the scenario is similar, except that trials are presented in the order listed in
//    the trial set, and each trial is presented N times in a row, where N is the trial's weight.
//
//    To implement the random selection scheme, an internal array tracks the # of reps remaining for each trial, 
//    as well as the total # of trial reps remaining in a trial "block".  At the start of a trial block, each array 
//    element is initialized to the corresponding trial's  weight.  To randomly choose a trial, we pick a random # 
//    between 0 and the total # of trial reps remaining (minus one).  We then step thru the "remaining reps" array, 
//    accumulating the #reps until we've reached our randomly selected #.  The position in the array at which we stop is
//    the index of the trial selected.
//
//    For the ordered and weighted selection scheme, we decrement the #reps remaining for a trial each time that trial
//    completes successfully. Once that hits zero, we step forward to the next trial in the set.
//
//    We only decrement the #reps remaining for a given trial when that trial is successfully completed (ie, no error
//    occurs and the subject satisfies the trial's fixation requirements, if any).  A trial "block" is complete when
//    the "remaining reps" array contains all zeros.  When this happens, we reinitialize the array and start over with
//    a new block, and the "block done" flag is set in the trial results field.
//
//    As of v3.1.2 (Dec 2014), the trial sequencer supports two levels of sequencing. A trial subset object was 
//    introduced in the MAESTRO object tree. Trials can now appear as children of trial sets or trial subsets. A trial
//    set can have any combination of trials and trial subsets as immediate children, although the intent is to group
//    trials into subsets when the experiment protocol warrants. 
//
//    When a trial set contains at least one non-empty subset (and typically more than one!!), the user specifies TWO
//    sequencing modes: one for the subsets and one for the trials within a subset. When subset sequencing is enabled,
//    these two methods perform similar work, but they act only on the trials within a subset. Once the trials for the
//    current subset have been presented IAW the trial sequencing mode, the next subset is chosen and the process
//    repeats. A trial "block" is completed when all of the defined subsets have been presented, at which point a new
//    block begins.
//
//    As of v3.3.1 (Feb 2017 revision), the RANDOM_REPEAT sequencer mode was added. This is just like RANDOM, except
//    that a failed trial is repeated over and over again until the subject completes it.
//
//    For GetNextWeightedTrial()--
//    ARGS:       dwTrialRes  -- [in/out] results from the last trial run (ignored when first trial of seq is chosen).
//
//    RETURNS:    NONE.
//
VOID CCxTrialSequencer::InitWeightedReps()
{
   ASSERT( m_arTrials.GetSize() >= 2 || m_ctrl.iTrialSeq == WT_ORDERED || m_ctrl.iTrialSeq == WT_ORDERED_NF);
   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc(); 
   
   // if subset sequencing is off, then the entire list of trials is treated as one group. Otherwise, we present the
   // trials in each subset IAW the trial sequencing mode, then move on to the next subset...
   int first = 0;
   int count = static_cast<int>(m_arTrials.GetSize());
   if(m_ctrl.iSubsetSeq != SUBSETSEQ_OFF && !m_Subsets.IsEmpty())
   {
      POSITION pos = m_Subsets.FindIndex(m_iCurrSubset);
      ASSERT(pos != NULL);
      CSubset* pSubset = m_Subsets.GetAt(pos);
      first = pSubset->idxFirst;
      count = pSubset->nTrials;
   }

   // for each trial, init #reps = trial's weight, and accumulate total #reps to be presented
   m_arNumRepsLeft.SetSize(count); 
   m_iTotalRepsLeft = 0; 
   for(int i = 0; i < count; i++)
   {
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[first+i] );
      ASSERT( pTrial );
      m_arNumRepsLeft[i] = pTrial->GetWeight();
      m_iTotalRepsLeft += pTrial->GetWeight();
   }
}

VOID CCxTrialSequencer::GetNextWeightedTrial(DWORD& dwTrialRes)
{
   ASSERT(m_bInitialized);
   ASSERT(m_ctrl.iTrialSeq == RANDOM_NF || m_ctrl.iTrialSeq == RANDOM || m_ctrl.iTrialSeq == RANDOM_REPEAT ||
            m_ctrl.iTrialSeq == WT_ORDERED || m_ctrl.iTrialSeq == WT_ORDERED_NF );

   BOOL bOk = WasTrialCompleted(dwTrialRes);

   // RANDOM_REPEAT mode: If last trial was not completed successfully in this mode, then it is presented again!
   if((m_ctrl.iTrialSeq == RANDOM_REPEAT) && (m_iSelected >= 0) && !bOk) return;

   // when sequencing subsets, we only are working with the trials in the current subset, NOT the entire trial list
   BOOL bIsSubset = (m_ctrl.iSubsetSeq != SUBSETSEQ_OFF) && !m_Subsets.IsEmpty();
   int first = 0;
   int count = static_cast<int>(m_arTrials.GetSize());
   if(bIsSubset)
   {
      POSITION pos = m_Subsets.FindIndex(m_iCurrSubset);
      ASSERT(pos != NULL);
      CSubset* pSubset = m_Subsets.GetAt(pos);
      first = pSubset->idxFirst;
      count = pSubset->nTrials;
   }

   // if last trial completed successfully (and we're not selecting the very first trial), decrement #reps left for 
   // that trial, as well as the total # of reps left until the end of the trial block or trial subset
   if((m_iSelected >= 0) && bOk)
   {
      // remember: m_iSelected is an index into the FULL trial list and we could be sequencing trials in a subset
      int iSel = m_iSelected-first;
      ASSERT(iSel < m_arNumRepsLeft.GetSize());
      --(m_arNumRepsLeft[iSel]);
      --m_iTotalRepsLeft;
   }

   // if we've finished the reps for the current subset, move on to the next subset. If we finished the last subset, 
   // then the trial block is done, in which case we start a new block. If subset sequencing is OFF, then there's
   // just one subset -- the entire trial list.
   if(m_iTotalRepsLeft == 0) 
   { 
      if(bIsSubset)
      {
         ++m_iCurrSubset;
         if(m_iCurrSubset >= m_Subsets.GetSize())
         {
            dwTrialRes |= CX_FT_BLOCKDONE;
            ShuffleSubsets();
            m_iCurrSubset = 0;
         }
      }
      else
         dwTrialRes |= CX_FT_BLOCKDONE;
      InitWeightedReps();
      if(bIsSubset)
      {
         POSITION pos = m_Subsets.FindIndex(m_iCurrSubset);
         ASSERT(pos != NULL);
         CSubset* pSubset = m_Subsets.GetAt(pos);
         first = pSubset->idxFirst;
         count = pSubset->nTrials;
      }

      m_iSelected = -1;
   }

   // pick the next trial to be presented from the subset currently being sequenced, or from the entire trial array if
   // subset sequencing is off.
   if(m_ctrl.iTrialSeq == RANDOM_NF || m_ctrl.iTrialSeq == RANDOM || m_ctrl.iTrialSeq == RANDOM_REPEAT)
   {
      // here we pick a random # between 0 and the #reps remaining - 1. We then step thru the remaining reps array,
      // summing up reps/trial until sum exceeds the randomly selected #.
      int iSum = 0;  
      int iPick = (rand() * m_iTotalRepsLeft) / RAND_MAX;
      int iSel = 0;
      while(iSel < m_arNumRepsLeft.GetUpperBound() ) 
      { 
         iSum += m_arNumRepsLeft[iSel]; 
         if(iPick < iSum) break;
         ++iSel;
      }

      m_iSelected = iSel + first;
   }
   else
   {
      int iSel = m_iSelected - first;
      if(m_iSelected < 0) iSel = 0;
      else if(m_arNumRepsLeft[iSel] <= 0)
         ++iSel;

      m_iSelected = iSel + first;
   }
}


//=== InitChainedReps, GetNextChainedTrial ============================================================================
//
//    In the CHAINED modes, instead of presenting the individual trials in a set randomly, trial "chains" are presented
//    randomly. A trial chain is a sequence of 1 or more consecutive presentations of the SAME trial. The number of 
//    different-length chains is determined by two parameters: the string TRIALSEQCTRL.strChainLens and the trial's
//    weight.
//   
//    The string parameter is a comma-delimited list of integers indicating the different chain lengths to be included
//    in one block of a chained sequence. This list applies to all participating trials, but any chain length larger
//    than the trial's weight is ignored. Furthermore, if a particular length appear N times in the list, then that 
//    chain length will be represented N times in the block of chains, for each trial with weight W >= N. This gives 
//    the user control over which trial chains are presented, and the relative frequency of different-length chains.
//    The minimum integer allowed is 1, and the maximum is 255. Any integer outside this range is skipped.
//
//    If the string parameter is empty (or contains no valid integers), then the sequencer will include chains of
//    length 1, 2, .., W for a trial with weight W, and analogously for all other trials in the sequenced set. 
//
//    An example should clarify these concepts: Let the trial set contain 3 trials: A (wt=5), B (wt=10), C (wt=1).
//    If the strChainLens parameter is an empty string, then one block in the sequence contains the chains 1A, 2A, 3A,
//    4A, 5A, 1B, 2B, .., 10B, and 1C. If strChainLens="1,2,2,4,8", then the chains in one block are: 1A, 2A, 2A, 4A,
//    1B, 2B, 2B, 4B, 8B, 1C.
//
//    The motivation behind this sequencer mode (introduced in V3.1.0, Oct 2013) is to detect improvement in the
//    subject's performance of a task with repetition.
//
//    To initialize the chained sequence, we generate the list of distinct chains to be presented in one "block". A
//    single chain is characterized by the key of the trial to be presented, and the number of times it should be
//    presented in a row. This list is then shuffled in random order. During sequencing, we step through this list
//    from beginning to end; for each chain in the list, we present the specified trial the specified number of times 
//    in a row before moving on to the next chain. Once all chains have been presented, the block is complete, and 
//    we reshuffle the same list to start the next block.
//
//    NOTE that all the individual trials in a particular chain are presented before moving onto the next chain, even 
//    if the animal does not complete one or more trials in that chain. Unlike in the RANDOM and WT_ORDERED modes, the 
//    trial result usually has NO bearing on what the sequencer does next.
//
//    [21mar2019] There is one exception. If the trial did not complete because a duplicate frame was detected on the
//    RMVideo display (result flag CX_FT_RMVDUPE), then this trial is treated as though it never happened. The current
//    chain's length is not incremented, nor is the current success chain length.
//
//    ARGS:       dwTrialRes  -- [in/out] results from the last trial run (ignored when first trial of seq is chosen).
//
//    RETURNS:    TRUE if successful; FALSE if an error occurred while initializing chained sequence.
//
BOOL CCxTrialSequencer::InitChainedReps()
{
   ASSERT( m_arTrials.GetSize() >= 2 );
   ASSERT( m_ctrl.iSubsetSeq == SUBSETSEQ_OFF );
   CCntrlxApp* pApp = ((CCntrlxApp*) AfxGetApp());
   CCxDoc* pDoc = pApp->GetDoc(); 

   // empty our chain list if it is not already
   while(!m_Chains.IsEmpty())
   {
      CChain* pChain = m_Chains.RemoveHead();
      delete pChain;
   }

   // parse string containing comma-delimited list of chain lengths to be presented
   CWordArray arChainLengths;
   if(!m_ctrl.strChainLens.IsEmpty())
   {
      int currPos = 0;
      CString token = m_ctrl.strChainLens.Tokenize(_T(","), currPos);
      while(!token.IsEmpty())
      {
         int len = _tstoi(token);
         if(len > 0 && len < 256)
            arChainLengths.Add((WORD) len);

         token = m_ctrl.strChainLens.Tokenize(_T(","), currPos);
      }
   }

   // populate the chains
   for(int i=0; i<m_arTrials.GetSize(); i++)
   {
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[i] );
      ASSERT(pTrial);
      int nReps = pTrial->GetWeight();
      ASSERT(nReps > 0);

      if(!arChainLengths.IsEmpty())
      {
         for(int j=0; j<arChainLengths.GetSize(); j++)
         {
            int len = (int) arChainLengths[j];
            if(len > nReps) continue;

            CChain* pChain = new CChain;
            if(pChain == NULL)
            {
               pApp->LogMessage(_T("ERROR: Memory exception."));
               return( FALSE );
            }
            pChain->index = i;
            pChain->nReps = len;

            try 
            {
               m_Chains.AddTail(pChain);
            }
            catch(CMemoryException *e)
            {
               e->Delete();
               delete pChain;
               pApp->LogMessage(_T("ERROR: Memory exception."));
               return( FALSE );
            }

         }
      }
      else for(int j=1; j<=nReps; j++)
      {
         CChain* pChain = new CChain;
         if(pChain == NULL)
         {
            pApp->LogMessage(_T("ERROR: Memory exception."));
            return( FALSE );
         }
         pChain->index = i;
         pChain->nReps = j;

         try 
         {
            m_Chains.AddTail(pChain);
         }
         catch(CMemoryException *e)
         {
            e->Delete();
            delete pChain;
            pApp->LogMessage(_T("ERROR: Memory exception."));
            return( FALSE );
         }
      }
   }

   // it's possible that no trial chains were generated: all trial weights are zero, or less than the integers listed
   // in the strChainLens parameter!
   if(m_Chains.IsEmpty())
   {
      pApp->LogMessage(_T("Empty chained sequence; check your trials and selected chain lengths."));
      return(FALSE);
   }

   // report the set of trial chains being sequenced
   CString line;
   pApp->LogMessage(_T("Generated set of trial chains to be sequenced (order is shuffled for each block)"));
   for(POSITION pos = m_Chains.GetHeadPosition(); pos != NULL;)
   {
      CChain* pChain = m_Chains.GetNext(pos);
      line.Format("   %d - %s", pChain->nReps, pDoc->GetObjName( m_arTrials[pChain->index] ));
      pApp->LogMessage(line);
   }

   // shuffle the chains
   int nChains = static_cast<int>(m_Chains.GetSize());
   for(int i=0; i<nChains-1; i++)
   {
      int iPick = i + ((rand() * (nChains-i)) / RAND_MAX);
      if(iPick == nChains) iPick = nChains - 1;

      POSITION pos = m_Chains.FindIndex(iPick);
      ASSERT(pos != NULL);
      CChain* pChain = m_Chains.GetAt(pos);
      m_Chains.RemoveAt(pos);
      m_Chains.AddHead(pChain);
   }

   // start with the first chain
   m_iCurrChain = 0;
   m_nCurrChainReps = -1;

   return(TRUE);
}

VOID CCxTrialSequencer::GetNextChainedTrial( DWORD& dwTrialRes )
{
   ASSERT( m_bInitialized );
   ASSERT( m_ctrl.iTrialSeq == CHAINED_NF || m_ctrl.iTrialSeq == CHAINED );
   ASSERT( m_iCurrChain >= 0 && m_iCurrChain < m_Chains.GetSize() );

   // special case: If trial aborted prematurely due to a duplicate frame occurring on the RMVideo display or an
   // Eyelink tracker communication error, then that trial is treated as though it never happened! The trial is 
   // repeated, and we DON'T increment the #reps for the current chain.
   //
   // Both the RMVideo duplicate frame error and the Eyelink tracker communication error are typically NON-fatal
   // recoverable errors that should not terminate trial sequencing.
   if((dwTrialRes & (CX_FT_RMVDUPE|CX_FT_EYELINKERR)) != 0) return;

   // increment #reps for the current chain. If we're not done with that chain, then we present same trial again.
   ++m_nCurrChainReps;
   POSITION pos = m_Chains.FindIndex(m_iCurrChain);
   ASSERT(pos != NULL);
   CChain* pChain = m_Chains.GetAt(pos);
   if(m_nCurrChainReps < pChain->nReps)
   {
      m_iSelected = pChain->index;
      return;
   }

   // move on to the next chain in the shuffled list. If we just finished the last chain, then we've completed a block.
   // In that case, reshuffle the chains and start a new block.
   ++m_iCurrChain;
   if(m_iCurrChain >= m_Chains.GetSize())
   {
      dwTrialRes |= CX_FT_BLOCKDONE;

      int nChains = static_cast<int>(m_Chains.GetSize());
      for(int i=0; i<nChains-1; i++)
      {
         int iPick = i + ((rand() * (nChains-i)) / RAND_MAX);
         if(iPick == nChains) iPick = nChains - 1;

         POSITION pos = m_Chains.FindIndex(iPick);
         ASSERT(pos != NULL);
         CChain* pChain = m_Chains.GetAt(pos);
         m_Chains.RemoveAt(pos);
         m_Chains.AddHead(pChain);
      }

      m_iCurrChain = 0;
   }
   m_nCurrChainReps = 0;

   // get index position in trial list of the next trial to present. If it is different from the preceding trial,
   // then we have to reset the counter that track the number of successful consecutive reps of the same trial,
   // potentially across two or more consecutive chains of that trial.
   int oldSel = m_iSelected;
   pos = m_Chains.FindIndex(m_iCurrChain);
   ASSERT(pos != NULL);
   pChain = m_Chains.GetAt(pos);
   m_iSelected = pChain->index;

   if(oldSel != m_iSelected) m_nConsecutiveRepsOK = 0;
}


//=== InitStaircases ==================================================================================================
//
//    Prepare runtime parameters for sequencing trials in one of the STAIRCASE modes, based on the current staircase
//    sequence control parameters.
//
//    BACKGROUND:  The staircase trial sequence is intended for visual psychophysics protocols in which the next trial
//    selected is based upon the subject's response to the last trial.  This function sets up a scheme which governs
//    trial selection.  Each trial can be designated as a member of one of MAX_STAIRS different "staircases", or as a
//    "NORMAL" trial (id=0).  Trials in the former category are staircase trials, marked for participation in one of
//    up to MAX_STAIRS different interleaved staircase sequences.  Each trial in a particular staircase has a 'stimulus
//    strength' associated with it.  Trials of the same strength make up a single 'tier' of that staircase.  All NORMAL
//    trials in the current trial set do not participate in any staircase and form the so-called "irrelevant tier".
//    The main job of this function is to build this multiple-staircase, tiered representation from the original list
//    of trials in the current trial set.   Here's how we do it:
//       1) On first pass thru trial set, we segregate the trials into the irrelevant tier or one of the staircases.
//          Trials belonging to a staircase are sorted into different tiers based on the "staircase stimulus strength"
//          associated with the trial.  Each time we encounter a staircase trial to which a new stimulus strength is
//          assigned, we must create a new tier; the new tier is inserted into the staircase's tier array  so that
//          tiers are in ascending order of stimulus strength.  If a staircase trial belongs to an existing tier, then
//          the #trials in that tier is incremented.
//       2) On second pass, we use the tier info to resort the trials in the set into the following prescribed order:
//          [ irrelevant tier trials, stair 1 tier 1 trials (weakest), ..., stair 1 tier M trials (strongest),
//          stair 2 tier 1, etc].  We also sum the trial weights for each tier.
//    The staircase and tier info, along with the sorted array of trial keys, is later used to randomly select a trial
//    from a given tier IAW the assigned trial weights.
//
//    Note that the algorithm is designed to preserve memory at the expense of computational time.
//
//    A staircase trial sequence will not work correctly if the trials are not properly defined.  Every trial (incl
//    NORMAL ones) must satisfy certain constraints, as validated by CheckStaircaseTrial().
//
//    RETURNS:  TRUE if it successfully resorted trials and initialized runtime params for the staircase sequencing;
//              FALSE if the current trial set does not support staircase sequencing.
//
BOOL CCxTrialSequencer::InitStaircases()
{
   ASSERT( m_ctrl.iSubsetSeq == SUBSETSEQ_OFF );

   int i, j, k;
   CString strMsg;

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                     // so we can post user-error messages
   CCxDoc* pDoc = pApp->GetDoc();                                    // so we can retrieve trial objects from doc
   ASSERT( pDoc );

   if( m_arTrials.GetSize() < 3 )                                    // cannot define a staircase seq w/ <3 trials!
   {
      pApp->LogMessage( _T("!! Insufficient # of trials in set for staircase sequencing !!") );
      return( FALSE );
   }

   m_nStairs = 0;                                                    // reset all staircase seq-related state vars
   m_iCurrStair = -1;
   for( i = 0; i < MAX_STAIRS; i++ )
   {
      CStair* pStair = &(m_stairs[i]);
      pStair->nTiers = 0;
      for( j = 0; j < MAX_TIERS; j++ )
      {
         pStair->tier[j].wtSum = 0;
         pStair->tier[j].n = 0;
      }
      pStair->bIsDone = FALSE;
      pStair->nDone = 0;
      pStair->nInARow = 0;
      pStair->currDir = 0;
      pStair->currTier = 0;
      pStair->nRevSoFar = 0;
      pStair->dAccum = 0.0;
   }
   m_irrelTier.wtSum = 0;
   m_irrelTier.n = 0;
   m_nIrrelevant = 0;
   m_nCorrectIrrel = 0;


   // BEGIN PASS 1 through trial key array:  create the staircases and their tiers and check that each trial's
   // definition is compatible with staircase sequencing.  a new tier is added to a given staircase for each
   // different stimulus strength found.  the tiers are sorted into ascending order of strength on the fly.
   //
   for( i = 0; i < m_arTrials.GetSize(); i++ )
   {
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[i] );
      ASSERT( pTrial );

      if( !CheckStaircaseTrial( pTrial ) )                                 // is trial appropriately defined?
      {
         strMsg.Format( "!! Trial _%s_ not compatible with staircase sequencing !!", pTrial->Name() );
         pApp->LogMessage( strMsg );
         return( FALSE );
      }

      int iStairNum = pTrial->GetStairNum();
      if( iStairNum > 0 )                                                  // a STAIRCASE trial....
      {
         ASSERT( iStairNum <= MAX_STAIRS );
         if( iStairNum > m_nStairs ) m_nStairs = iStairNum;                // update # of participating staircases
         CStair* pStair = &(m_stairs[iStairNum-1]);                        // retrieve approp staircase record

         j = 0;                                                            // within staircase, find tier to which
         double dStrength = pTrial->GetStairStrength();                    // this trial belongs...
         while( j < pStair->nTiers )
         {
            if( dStrength <= pStair->tier[j].dStrength ) break;
            j++;
         }

         if((j<pStair->nTiers) && (dStrength==pStair->tier[j].dStrength))  // if found, we just incr #trials in tier
            ++(pStair->tier[j].n);
         else                                                              // else we must make a new tier:
         {
            if( pStair->nTiers == MAX_TIERS )                              //    too many tiers!!!
            {
               strMsg.Format( "!! Too many tiers in staircase #%d !!", iStairNum );
               pApp->LogMessage( strMsg );
               return( FALSE );
            }
            for( k = pStair->nTiers - 1; k >= j; k-- )                     // shift higher-strength tiers up to make
            {                                                              // room for new tier
               pStair->tier[k+1].n = pStair->tier[k].n;
               pStair->tier[k+1].dStrength = pStair->tier[k].dStrength;
            }
            ++(pStair->nTiers);
            pStair->tier[j].n = 1;
            pStair->tier[j].dStrength = dStrength;
         }
      }
      else                                                                 // all NORMAL trials in "irrelevant" tier
         ++(m_irrelTier.n);
   }
   //
   // END PASS 1.


   if( m_nStairs == 0 )                                                    // check for ill-defined staircases...
   {
      pApp->LogMessage( "!! No staircase trials are defined in current trial set !!" );
      return( FALSE );
   }
   for( i = 0; i < m_nStairs; i++ ) if( m_stairs[i].nTiers < 3 )
   {
      strMsg.Format( "!! Staircase #%d does not include at least 3 strength tiers !!", i+1 );
      pApp->LogMessage( strMsg );
      return( FALSE );
   }


   // determine start location of each staircase tier in the yet-to-be-resorted array of trial keys.  note that the
   // irrelevant and staircase tiers will appear in the following order in the sorted array:
   //    irrelevant tier trials, stair 1 tier 1, stair 1 tier 2, ..., stair 1 tier M, stair 2 tier 1, ...
   // staircase tiers appear in ascending order, as determined in pass 1 above.
   //
   m_irrelTier.first = 0;
   k = m_irrelTier.n;
   for( i = 0; i < m_nStairs; i++ )
   {
      CStair* pStair = &(m_stairs[i]);
      for( j = 0; j < pStair->nTiers; j++ )
      {
         pStair->tier[j].first = k;
         k += pStair->tier[j].n;
      }
   }


   // BEGIN PASS 2 thru trial key array:  resort trial key array into ordered tiers and compute sum of trial weights
   // for each tier.  a binary search finds to which tier a staircase trial belongs (tiers are sorted in ascending
   // order at this point).
   //
   m_irrelTier.n = 0;                                                      // reset #trials in all tiers
   for( i = 0; i < m_nStairs; i++ )
   {
      CStair* pStair = &(m_stairs[i]);
      for( j = 0; j < pStair->nTiers; j++ ) pStair->tier[j].n = 0;
   }

   m_arNumRepsLeft.SetSize( m_arTrials.GetSize() );                        // we'll use this array for sorting

   for( i = 0; i < m_arTrials.GetSize(); i++ )                             // DO THE SORT:
   {
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[i] );     //    get trial attributes needed
      ASSERT( pTrial );
      int iStairNum = pTrial->GetStairNum();
      int iWeight = pTrial->GetWeight();
      double dStrength = pTrial->GetStairStrength();

      CTier* pTier;                                                        //    find tier to which trial belongs...
      if( iStairNum == 0 )                                                 //    ...either the irrelevant tier,
         pTier = &m_irrelTier;
      else                                                                 //    ...or a tier in a staircase...
      {
         CStair* pStair = &(m_stairs[iStairNum-1]);
         int iFirst = 0;                                                   //    use binary search to find tier
         int iLast = pStair->nTiers-1;
         while( iLast - iFirst > 1 )
         {
            j = (iFirst + iLast) / 2;
            if( dStrength < pStair->tier[j].dStrength )        iLast = j;
            else if( dStrength > pStair->tier[j].dStrength )   iFirst = j;
            else                                               iLast = iFirst = j;
         }
         j = iFirst;
         if( dStrength > pStair->tier[j].dStrength ) j = iLast;            //    handles possible extreme case
         pTier = &(pStair->tier[j]);
      }

      m_arNumRepsLeft[pTier->first + pTier->n] = m_arTrials[i];            //    sorted pos in array for this trial!
      ++(pTier->n);
      pTier->wtSum += iWeight;                                             //    accumulate wts of all trials in tier
   }

   for( i = 0; i < m_arTrials.GetSize(); i++ )                             // copy sorted array to our trial keys array
      m_arTrials[i] = m_arNumRepsLeft[i];
   //
   // END PASS 2


   for( i = 0; i < m_nStairs; i++ )                                        // find "current tier" for each staircase,
   {                                                                       // ie, whichever tier's strength is closest
      CStair* pStair = &(m_stairs[i]);                                     // and >= sequencer's starting strength...
      for( j = 0; j < pStair->nTiers; j++ )
      {
         if( m_ctrl.dStairStrength <= pStair->tier[j].dStrength ) break;
      }
      if( j == pStair->nTiers ) --j;                                       // all tiers may be < starting strength!
      pStair->currTier = j;
   }


   strMsg.Format( "==> Starting staircase trial sequence with %d staircases...", m_nStairs );
   pApp->LogMessage( strMsg, TRUE );
   strMsg.Format( "Start strength = %.3f; %irrelevant = %d.", m_ctrl.dStairStrength, m_ctrl.nStairIrrel );
   pApp->LogMessage( strMsg );
   strMsg.Format( "Decision algorithm: %d-up, %d-down.  Stop after %d reversals.",
                  m_ctrl.nWrongUp, m_ctrl.nRightDn, m_ctrl.nReversals );
   pApp->LogMessage( strMsg );

   return( TRUE );
}


//=== CheckStaircaseTrial =============================================================================================
//
//    Check trial definition to make sure it is compatible with staircase sequencing.  The following constraints must
//    be satisfied:   (1) at least one segment of the trial (with a nonzero duration) must be marked for checking the
//    subject's response.  (2) the trial must not involve one of the special, saccade-triggered operations.
//
//    NOTE:  The subject's response is checked by CXDRIVER on the fly as the trial progresses.  Even if the two
//    response channels are not recorded, CXDRIVER will still sample these channels (currently, ADC ch12 & ch13) to
//    check the response
//
//    ARGS:       pTrial   -- [in] ptr to the trial object.
//
//    RETURNS:    TRUE if trial definition is OK, FALSE otherwise.
//
BOOL CCxTrialSequencer::CheckStaircaseTrial( CCxTrial* pTrial )
{
   ASSERT_KINDOF( CCxTrial, pTrial );
   return( pTrial->IsResponseChecked() && (pTrial->GetSpecialOp() == TH_SOP_NONE) );
}


//=== GetNextStaircaseTrial ===========================================================================================
//
//    Update the sequencer's runtime state in the STAIRCASE modes and select the next trial to run, taking into account
//    subject's response to the last trial presented in the sequence.  If all staircases satisfy the stop condition,
//    the sequencer auto-stops -- setting the CX_SEQSTOP in the trial results field -- and the next trial is undefined.
//
//    The sequencer can interleave up to MAX_STAIRS distinct staircases simultaneously.  In addition, trials in the set
//    that are not associated with a staircase ("irrelevant" trials) can be presented randomly a specified percentage
//    of the time.  See InitStaircases() for a more detailed description.
//
//    Currently, we use an "N-up, M-down" decision algorithm to step up or down each staircase.  When the subject
//    responds incorrectly to N consecutive trials from a given staircase tier, we take a step up the staircase to the
//    next higher-strength tier.  When the subject responds correctly to M consecutive trials from a tier, we take a
//    step down the staircase to the next lower-strength tier.  A staircase is considered "done" when the staircase
//    direction (up, down) has reversed a specified # of times (part of the sequencer's control params).
//
//    If the subject fails to respond to a staircase trial, correctly or incorrectly, then the trial is repeated.
//
//    [21mar2019] A new trial result flag, CX_FT_RMVDUPE, indicates that the trial stopped prematurely because a
//    duplicate frame (or frames) was detected on the RMVideo display. If trial is "irrelevant", it is not repeated and
//    the staircase message will indicate and error occurred.
//
//    [03nov2022] Added trial result flag, CX_FT_EYELINKERR, indicating the trial stopped prematurely because of an
//    Eyelink communication tracker error. Like the RMVideo duplicate frame error, this is typically a transient,
//    non-fatal error that does NOT terminate trial sequencing. Both errors are treated the same way by the staircase
//    trial sequencer: a failed irrelevant trial is repeated, but a failed staircase trial is.
//
//    ARGS:       dwTrialRes -- [in/out] results from the last trial run (ignored when first trial of seq is chosen).
//
//    RETURNS:    NONE.
//
VOID CCxTrialSequencer::GetNextStaircaseTrial( DWORD& dwTrialRes )
{
   ASSERT( m_bInitialized );

   BOOL bNoResp = BOOL((dwTrialRes & CX_FT_NORESP) != 0);    // subj failed to respond to trial.
   BOOL bRespOK = BOOL((dwTrialRes & CX_FT_RESPOK) != 0);    // if there was a response, was it correct?
   BOOL bError  = BOOL((dwTrialRes & CX_FT_ERROR) != 0);     // a fatal error occurred during last trial
   BOOL bNonFatal = BOOL((dwTrialRes & (CX_FT_RMVDUPE|CX_FT_EYELINKERR)) != 0); // non-fatal error during last trial
   BOOL bLostFix= BOOL((dwTrialRes & CX_FT_LOSTFIX) != 0);   // subject lost fixation during last trial

   //
   // PART 1:  Update staircase sequence runtime state IAW the subject's response to the last trial (if there was one!)
   //
   if( m_iSelected >= 0 )
   {
      if( m_iCurrStair < 0 )                                   // last trial was an "irrelevant" one; just update stats
      {                                                        // Note that irrelevant trials are never repeated.
         ++m_nIrrelevant;
         if( bError || bNonFatal || (bLostFix && !IsNoFixMode()) )
            LogStaircaseStatus( STAIR_ERRLOSTFIX );
         else
         {
            if( bRespOK && !bNoResp ) ++m_nCorrectIrrel;
            LogStaircaseStatus( bNoResp ? STAIR_NORESP : (bRespOK ? STAIR_OK : STAIR_WRONG) );
         }
      }
      else                                                     // last trial was a staircase trial...
      {
         CStair* pStair = &(m_stairs[m_iCurrStair]);           // the staircase containing the trial

         // incr #trials presented in this staircase, UNLESS last trial aborted on a non-fatal error
         if(!bNonFatal) ++(pStair->nDone); 

         // repeat last trial if it aborted on a fatal error (in this case, trial sequencing stops), a non-fatal
         // error, or if the subject broke fixation
         if( bError || bNonFatal || (bLostFix && !IsNoFixMode()) ) 
         { 
            LogStaircaseStatus( STAIR_ERRLOSTFIX );
            return;
         }

         // otherwise, repeat the trial if subject failed to respond
         if( bNoResp ) 
         {
            LogStaircaseStatus( STAIR_NORESP );
            return;
         }

         LogStaircaseStatus(bRespOK ? STAIR_OK : STAIR_WRONG); // log results message for this staircase trial

                                                               // the N-up, M-down decision algorithm:
         if( bRespOK )                                         // if resp to trial OK, update the # of consecutive
         {                                                     // correct responses (>0)
            if( pStair->nInARow >= 0 )  (pStair->nInARow)++;
            else                       pStair->nInARow = 1;
         }
         else                                                  // else, update the # of consecutive incorrect (<0)
         {
            if( pStair->nInARow <= 0 )  (pStair->nInARow)--;
            else                       pStair->nInARow = -1;
         }

         int iChange = 0;                                      // when the # of consecutive correct response reaches
         if( pStair->nInARow == m_ctrl.nRightDn )              // the user-defined value, go down one strength tier.
            iChange = -1;
         else if( pStair->nInARow == -(m_ctrl.nWrongUp) )      // vice-versa for consecutive incorrect responses
            iChange = 1;

         if( iChange != 0 )                                    // reset #consecutive correct(incorrect) whenever we
            pStair->nInARow = 0;                               // step up or down the staircase

         if( (pStair->currDir == 0) && (iChange != 0) )        // if current direction is undefined (at start of seq)
         {                                                     // and we're stepping up or down, make the direction of
            if( ((pStair->currTier == pStair->nTiers-1) &&     // the step our current dir unless that would knock us
                 (iChange == 1) ) ||                           // off the top or bottom of the staircase!
                ((pStair->currTier == 0) && (iChange == -1))
              )
               iChange = 0;
            else
               pStair->currDir = iChange;
         }
         else if( (pStair->currDir * iChange) < 0 )            // otherwise, if a direction reversal has occurred:
         {
            pStair->currDir = iChange;                         //    set the new direction
            ++(pStair->nRevSoFar);                             //    increment the # of direction reversals so far
            if( pStair->nRevSoFar == m_ctrl.nReversals )       //    the auto-stop condition is met; stop running this
               pStair->bIsDone = TRUE;                         //       staircase
            pStair->dAccum +=                                  //    accum tier strength at each reversal so we can
               pStair->tier[pStair->currTier].dStrength;       //       calculate strength "threshold" later.
         }

         if( !pStair->bIsDone )                                // step up or down the staircase, taking care not to
         {                                                     // step off the staircase entirely!!
            if( (iChange < 0) && (pStair->currTier > 0) )
               --(pStair->currTier);
            else if( (iChange > 0) && (pStair->currTier < pStair->nTiers - 1) )
               ++(pStair->currTier);
         }
         else                                                   // if stair just finished, post a msg to this effect
            LogStaircaseStatus( m_iCurrStair + STAIR_DONE );
      }
   }


   //
   // PART 2:  Select the next trial to present.  We must make up to 3 random choices here:
   // 1) If an irrelevant tier exists, we randomly choose (a specified %age of the time) to run either an irrelevant
   //    trial or a staircase trial.
   // 2) If we are to run a staircase trial and more than one staircase is defined, then we must select from one of the
   //    staircases (each with equal probability).
   // 3) Once we've selected a staircase, we must select a trial from the current tier within that staircase.
   //
   // If more than one staircase is running and one staircase has already satisfied the stop condition, that "stopped"
   // staircase is excluded from the selection process.  When all staircases have satisfied the stop condition, the
   // staircase sequencer is stopped.
   //
   int i, j;
   int nStairsOn = 0;                                       // save array indices of staircases that are still running
   int idxActiveStairs[MAX_STAIRS];
   for( i = 0; i < m_nStairs; i++ )
   {
      if( !(m_stairs[i].bIsDone) )
      {
         idxActiveStairs[nStairsOn] = i;
         ++nStairsOn;
      }
   }
   if( nStairsOn == 0 )                                     // if there are no active staircases, we're done -- the
   {                                                        // sequencer has auto-stopped and there is no next trial!
      LogStaircaseStatus( STAIR_SEQSTOP );
      dwTrialRes |= CX_FT_SEQSTOP;
      m_iSelected = -1;                                     // next trial is undefined!!
      return;
   }

   CTier* pTierChosen = NULL;                               // the strength tier selected

   if( (m_ctrl.nStairIrrel > 0) && (m_irrelTier.n > 0) )    // CHOICE 1: if there are irrelevant trials defined, decide
   {                                                        // whether next trial will be irrelevant or not...
      i = (100 * rand()) / RAND_MAX;                        // (NOTE: % irrelevant is in whole percentage points.)
      if( i <= m_ctrl.nStairIrrel )
      {
         pTierChosen = &(m_irrelTier);
         m_iCurrStair = -1;
      }
   }

   if( pTierChosen == NULL )                                // CHOICE 2: if we did not choose irrelevant tier, select
   {                                                        // randomly among the remaining *active* staircases and
      i = (nStairsOn * rand()) / RAND_MAX;                  // choose the current tier of the selected staircase.
      if( i >= nStairsOn ) i = nStairsOn - 1;

      m_iCurrStair = idxActiveStairs[i];
      i = m_stairs[m_iCurrStair].currTier;
      pTierChosen = &(m_stairs[m_iCurrStair].tier[i]);
   }

   if( pTierChosen->n == 1 )                                // CHOICE 3: randomly select a trial from the chosen tier.
      m_iSelected = pTierChosen->first;                     // if there's just one trial in the tier, things are easy!
   else                                                     // otherwise, we make a *weighted* choice as follows:
   {
      int wtPick = (pTierChosen->wtSum * rand())/RAND_MAX;  //    choose a random int in [0..wtSum-1]

      i = pTierChosen->first;                               //    [i:j] is the portion of our sorted trial key array
      j = i + pTierChosen->n - 1;                           //    representing the trials in the chosen tier.

      CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc(); //    need access to CNTRLX doc to get trial weights
      ASSERT( pDoc );

      int wtSum = 0;                                        //    step thru this range, summing the trial weights until
      while( i < j )                                        //    the sum exceeds the random weight chosen.
      {
         CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( m_arTrials[i] );
         if( (wtSum + pTrial->GetWeight()) > wtPick ) break;
         wtSum +=  pTrial->GetWeight();
         ++i;
      }

      m_iSelected = i;                                      //    at last, this is the index of the selected trial!
   }

}


//=== LogStaircaseStatus ==============================================================================================
//
//    Posts messages regarding the status of an ongoing staircase sequence.  Possible status codes:
//
//       STAIR_SEQSTOP  ==> Sequence stopped.  Compute estimated stimulus strength threshold (if possible) for each
//          staircase that was running and post appropriate message.  We define the threshold as the average strength
//          over the # of reversals that have occurred.  if no reversals have occurred, this threshold is undefined.
//       STAIR_ERRLOSTFIX ==> Error or fixation break.  If last trial was not "irrelevant", it is repeated.
//       STAIR_NORESP   ==> Subject failed to respond to last trial.  If not "irrelevant", trial will  be repeated.
//       STAIR_WRONG    ==> Incorrect response to the last trial presented.
//       STAIR_OK       ==> Correct response to the last trial.
//       STAIR_DONE + N ==> Staircase sequence #(N+1) has just satisfied stop condition (N = [0..MAX_STAIRS-1]).
//
//    The status msg composed for status codes _REDO, _WRONG, and _OK contains the following fields:
//       ch 0-6   ==> "N:MMMM ", where N is the staircase # (1-5) and MMMM is the # of trials from this staircase that
//                    have been presented so far.  N = 0 for irrelevant trials.
//       ch 7-26  ==> first 20 chars of trial name (left-aligned, completed with blanks)
//       ch 27-29 ==> ":  "
//       ch 30-44 ==> for staircase trials, this lists the strength of the trial (right-aligned, out to 3 sig figs,
//                    preceded with blanks) and one of four possible results:  "norsp", "right", "wrong", or "error".
//                    The first of these results indicates that subject failed to respond at all.  The last indicates
//                    that an error occurred, or the subject lost fixation.  In either case, the trial is repeated.
//                    For irrelevant trials, this reads "**irrel*" followed by "norsp", "right", "wrong", or "error".
//                    Note that irrelevant trials are not repeated.
//       ch 45-46 ==> ", "
//       ch 47-53 ==> "r=%03d <": the # of strength reversals that have occurred thus far and the current staircase
//                    direction (< for decreasing, > for increasing strength) -- for the staircase from which current
//                    trial was taken.  if the current trial is irrelevant, this reads "c=%03d  ", where the integer
//                    indicates the # of irrelevant trials for which the subject responded "correctly".
//
//    ARGS:       iResult  -- [in] status/result code for last trial presented.
//
//    RETURNS:    NONE.
//
//    THROWS:     CString ops may throw CMemoryException.
//
VOID CCxTrialSequencer::LogStaircaseStatus( int iResult )
{
   ASSERT( m_bInitialized );

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();         // the app object handles details of posting msgs to user
   CString strStatus;
   CString strBuf;
   int i;
   CStair* pStair;

   switch( iResult )
   {
      case STAIR_SEQSTOP :
         for( i = 0; i < m_nStairs; i++ )
         {
            pStair = &(m_stairs[i]);
            if( pStair->nRevSoFar > 0 )
               strStatus.Format( "Stair %d: Threshold ~= %.3f", i+1, pStair->dAccum/((double) pStair->nRevSoFar) );
            else
               strStatus.Format( "Stair %d: Est threshold is not defined!", i+1 );
            pApp->LogMessage( strStatus );
         }

         if( m_nIrrelevant > 0 )
            strStatus.Format( "Pct irrelevant trials correct = %.2f%%",
                              100.0 * ((double) m_nCorrectIrrel) / ((double) m_nIrrelevant) );
         else
            strStatus.Format( "No irrelevant trials were presented." );
         pApp->LogMessage( strStatus );
         break;

      case STAIR_ERRLOSTFIX :                                           // for these, prepare a status msg field by
      case STAIR_NORESP :                                               // field IAW format described above...
      case STAIR_WRONG :
      case STAIR_OK :
         ASSERT( m_iSelected >= 0 && m_iSelected < m_arTrials.GetSize() );
         if( m_iCurrStair >= 0 )
            pStair = &(m_stairs[ m_iCurrStair ]);                       // staircase of which current trial is a member
         else
            pStair = NULL;                                              // no staircase; trial is "irrelevant"

         strStatus.Format( "%1d:%04d %.*s",
                           m_iCurrStair+1,
                           (pStair==NULL) ? m_nIrrelevant : pStair->nDone,
                           20, pApp->GetDoc()->GetObjName( m_arTrials[m_iSelected] ) );
         while( strStatus.GetLength() < 27 ) strStatus += ' ';

         if( pStair == NULL )
            strBuf = _T(":  **irrel*, ");
         else
            strBuf.Format( ":  %8.3f, ", pStair->tier[pStair->currTier].dStrength );
         strStatus += strBuf;

         if( iResult == STAIR_ERRLOSTFIX )   strStatus += _T("error, ");
         else if( iResult == STAIR_NORESP )  strStatus += _T("norsp, ");
         else if( iResult == STAIR_WRONG )   strStatus += _T("wrong, ");
         else                                strStatus += _T("right, ");

         if( pStair == NULL )
            strBuf.Format( "c=%03d", m_nCorrectIrrel );
         else
            strBuf.Format( "r=%03d", pStair->nRevSoFar );
         strStatus += strBuf;

         if( pStair == NULL )             strStatus += _T("  ");
         else if( pStair->currDir > 0 )   strStatus += _T(" >");
         else if( pStair->currDir < 0 )   strStatus += _T(" <");
         else                             strStatus += _T("  ");

         ASSERT( strStatus.GetLength() == 54 );
         pApp->LogMessage( strStatus );
         break;

      default :
         i = iResult - STAIR_DONE;                                      // result code = STAIRDONE + N, N>=0
         if( (i>=0) && (i < m_nStairs) )
         {
            strStatus.Format( "Staircase #%d STOPPED after %d trials", i, m_stairs[i].nDone );
            pApp->LogMessage( strStatus );
         }
         break;
   }
}
