//=====================================================================================================================
//
// cxtrialseq.h : Declaration of class CCxTrialSequencer.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXTRIALSEQ_H__INCLUDED_)
#define CXTRIALSEQ_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "cxipc.h"               // constants and IPC interface data structs for MAESTRO-MAESTRODRIVER communications
#include "util.h"                // for CFPoint
#include "afxtempl.h"            // for CTypedPtrList template 

class CCxTrial;                  // forward declaration


//=====================================================================================================================
// Declaration of class CCxTrialSequencer -- helper class that selects the next trial in a trial sequence and prepares
//    a "target list" and each trial definition in the form MAESTRODRIVER expects.
//=====================================================================================================================
//

typedef struct tagTrialSeqCtrl   // the trial sequencer's control parameters:
{
   WORD     wTrialSet;           //    object key of the MAESTRO trial set to use
   WORD     wCurrTrial;          //    object key of the "current" trial being presented (or to be presented next)

   int      iSubsetSeq;          //    trial subset sequencing mode
   int      iTrialSeq;           //    trial sequencing mode

   double   dStairStrength;      //    starting strength for a staircase sequence
   int      nStairIrrel;         //    % "irrelevant" (not part of a staircase) trials presented in a staircase seq
   int      nWrongUp;            //    #-in-a-row incorrect responses to trigger increment in staircase strength
   int      nRightDn;            //    #-in-a-row correct responses to trigger decrement in staircase strength
   int      nReversals;          //    # of staircase direction reversals to trigger auto-stop (0 = manual)

   // a comma-delimited list of integers indicating the trial chain lengths to be run during a chained sequence.
   // The same integer can appear more than once, thereby weighting that chain length more than others. Applies to
   // all trials in the set sequenced; however, the trial weight is respected as the maximum chain length for that
   // trial. Finally, if this string is empty, then a block includes all trial chain lengths 1-N, where N is the
   // trial's weight.
   CString strChainLens;
} TRIALSEQCTRL, *PTRIALSEQCTRL;



class CCxTrialSequencer
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int STAIR_SEQSTOP;        // staircase status codes (for LogStaircaseStatus())
   static const int STAIR_ERRLOSTFIX;
   static const int STAIR_NORESP;
   static const int STAIR_WRONG;
   static const int STAIR_OK;
   static const int STAIR_DONE;

public:
   static const int MAX_CHAINLEN = 11;    // chain lengths 1-10, plus a catchall for any chain length > 10
   static const int MAX_TIERS = 100;      // max# of distinct strength tiers in a staircase trial sequence

   // the sequencing modes for trial subsets
   typedef enum {
      SUBSETSEQ_OFF = 0,                  //   subset sequencing off; ignore subsets and treat all trials as one group
      SUBSETSEQ_ORDERED = 1,              //   subsets sequenced in order of appearance in trial set
      SUBSETSEQ_RANDOM = 2                //   subsets are sequenced randomly
   } SubsetSeqMode;
   static const int NUM_SUBSETSEQ = 3;

   // short human-readable names for the different trial subset sequencing modes
   static LPCTSTR strSubsetSeqModes[NUM_SUBSETSEQ];

   // all possible sequencing modes for individual trials
   typedef enum {
      THISTRIAL = 0,                      //    run currently selected trial repeatedly, fixation requirements enforced
      ORDERED = 1,                        //    run trials in presentation order, fixation requirements enforced
      ORDERED_REPEAT = 2,                 //    as above, but repeat failed trial until fixation reqmts satisfied
      WT_ORDERED = 3,                     //    like ORDERED, but trial with weight N is presented N times in a row
      RANDOM = 4,                         //    weighted & randomized presentation, fixation requirements enforced
      RANDOM_REPEAT = 5,                  //    as above, but repeat failed trial until fixation reqmts satisfied
      CHAINED = 6,                        //    trial "chains" are randomized, fixation requirements enforced
      STAIRCASE = 7,                      //    staircase sequencing mode, fixation requirements enforced
      THISTRIAL_NF = 8,                   //    same modes as above, but fixation requirements NOT enforced
      ORDERED_NF = 9,
      WT_ORDERED_NF = 10,
      RANDOM_NF = 11,
      CHAINED_NF = 12,
      STAIRCASE_NF = 13
   } SeqMode;
   static const int NUM_TRIALSEQ = 14;

   // short human-readable names for the different trial sequencing modes
   static LPCTSTR strTrialSeqModes[NUM_TRIALSEQ]; 

   // the trial sequencer's auto-stop modes
   typedef enum {
      AUTOSTOP_OFF = 0,                   // auto-stop disabled (manual stop required)
      AUTOSTOP_TRIALS = 1,                // auto-stop after N trials completed
      AUTOSTOP_BLKS = 2                   // auto-stop after N trial blocks completed
   } AutoStopeMode;
   static const int NUMAUTOSTOPMODES = 3;

   // short human-readable names for the sequencer's auto-stop modes
   static LPCTSTR strAutoStopModes[NUMAUTOSTOPMODES];

   // is the specified combination of subset and trial sequencing modes allowed?
   static BOOL IsValidSeqMode(int iSubsetSeq, int iTrialSeq);

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   struct CTier                     // info about each distinct strength set, or "tier", of a given "staircase" in the
   {                                // current trial set (for staircase sequencing of trials):
      double   dStrength;           //    strength value for this tier
      int      n;                   //    number of trials in this tier
      int      wtSum;               //    sum of the weights for the trials in this tier
      int      first;               //    index into the sequencer's trial key array:  loc of first trial in this tier
   };

   struct CStair                    // definition of a single staircase sequence:
   {
                                    //    STATIC RUNTIME PARAMETERS:
      int      nTiers;              //    # of distinct trial strengths in the staircase sequence
      CTier    tier[MAX_TIERS];     //    strength sets participating in the sequence, in ascending order of strength

                                    //    DYNAMIC RUNTIME PARAMETERS:
      BOOL     bIsDone;             //    set to TRUE when staircase seq has satisfied stop condition
      int      nDone;               //    # of trials presented in the sequence thus far (merely for display purposes)
      int      nInARow;             //    >0 is #correct (<0, #incorrect) responses in a row since last strength change
      int      currDir;             //    current dir of staircase: 0 (undefined), >0 (incr strength), or <0 (decr)
      int      currTier;            //    current staircase strength set in use [0 .. #tiers-1]
      int      nRevSoFar;           //    # of strength reversals that have occurred thus far
      double   dAccum;              //    sum of tier strengths at each strength reversal, to calc avg at seq stop
   };

   struct CChain                    // representation of a sequential chain of repetitions of the same trial
   {
      int index;                    //    index of trial in the trial set being sequenced in "Chained" mode
      int nReps;                    //    the number of trial repetitions for this chain
   };

   struct CStat                     // simple statistics on trials participating in current sequence
   {
      CString name;                 //    the trial's name (for quick reference)
      int nAttempted;               //    #times this trial has been attempted thus far
      int nCompleted;               //    #times this trial has been completed thus far
      int chainReps[MAX_CHAINLEN];  //    # of successful trial chains of length 1 thru 10+ (CHAINED modes only)
   };

   // information on trial subset -- used when subset sequencing is enabled
   struct CSubset
   {
      CString name;                 // the subset's name (for quick reference)
      int idxFirst;                 // index and count defining the contiguous range of trial keys in the master trial
      int nTrials;                  //    key array corresponding to the trials in this subset
   };


   BOOL           m_bInitialized;         // TRUE when sequencer has been properly initialized to seq trials in a set
   TRIALSEQCTRL   m_ctrl;                 // sequencer's current control parameters
   int            m_iAutoStopMode;        // auto-stop mode (see enum above)
   int            m_nAutoStopCount;       // if autostop enabled, stop after this many trials or blocks are done

   CWordArray     m_arTargets;            // keys of all targets used by trials in the current trial set
   CWordArray     m_arTrials;             // keys of all trials in the current trial set
   int            m_iSelected;            // pos (in trial key array) of last trial selected by sequencer

                                          // these can change while a trial seq is in progress w/o affecting seq state:
   CFPoint        m_startTgtPos;          //    global starting pos for all targets participating in trial
   double         m_dPosScale;            //    target position & velocity scale & rotation factors
   double         m_dPosRotate;
   double         m_dVelScale;
   double         m_dVelRotate;
   BOOL           m_bUseChan;             //    channel configuration override enable
   WORD           m_wChanOvrKey;          //    key of overriding channel configuration (CX_NULLOBJ_KEY = "none")

   // additional runtime control parameters for RANDOM, WT_ORDERED modes only:
   CWordArray     m_arNumRepsLeft;        //    # of reps remaining per trial
   int            m_iTotalRepsLeft;       //    sum of remaining reps across all trials in set (or subset)

   // trial statistics
   CTypedPtrArray<CPtrArray, CStat*> m_Stats;
   
   // information on trial subsets being sequenced, if any
   CTypedPtrList<CPtrList, CSubset*> m_Subsets;
   // index of the trial subset currently being presented. -1 if subset sequencing is off.
   int m_iCurrSubset;

   // additional runtime control parameters for CHAINED, CHAINED_NF modes only:
   CTypedPtrList<CPtrList, CChain*> m_Chains;  // the (shuffled) list of trial chains in an ongoing chained sequence
   int m_iCurrChain;                           // index position of trial chain currently being presented
   int m_nCurrChainReps;                       // num reps of trial for the current selected chain

   // this counter keeps tracks of how many consecutive SUCCESSFUL reps of the same trial have occurred so far in
   // CHAINED mode. It is reset whenever a sequence of such reps is broken because the trial sequence was stopped or
   // paused, or a different trial was presented. NOTE that this counter could keep incrementing across the presentation 
   // of two chains involving the same trial: 3A followed by 4A, e.g. A "failed" trial does NOT break the chain.
   int m_nConsecutiveRepsOK;
                                          // additional runtime control parameters for STAIRCASE modes only:
   int            m_nStairs;              //    # of distinct staircase sequences running simultaneously
   int            m_iCurrStair;           //    the CStair containing the current trial (if -1, trial is "irrelevant")
   CStair         m_stairs[MAX_STAIRS];   //    parameters for each participating staircase sequence
   CTier          m_irrelTier;            //    "irrelevant" tier (all "NORMAL" trials in the current trial set)
   int            m_nIrrelevant;          //    total # of "irrelevant" trials presented thus far
   int            m_nCorrectIrrel;        //    # of those "irrelevant" trials to which subject responded correctly

   int            m_nTrialsDone;          // #trials successfully completed since the trial sequencer started
   int            m_nBlocksDone;          // #trial blocks successfully completed since the trial sequencer started

   // these transient flags are used to set certain trial flags that get stored in data file header
   BOOL           m_bSeqStart;            // if TRUE, then next trial marks the start of a trial sequence
   BOOL           m_bSeqPaused;           // if TRUE, then next trial will mark resumption of a paused trial sequence

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialSequencer( const CCxTrialSequencer& src );    // no copy constructor or assignment operator defined
   CCxTrialSequencer& operator=( const CCxTrialSequencer& src );

public:
   CCxTrialSequencer();
   ~CCxTrialSequencer() { Reset(); }


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   // is current trial sequencing mode one of the "no fix" modes?
   BOOL IsNoFixMode() const { return(m_ctrl.iTrialSeq >= THISTRIAL_NF && m_ctrl.iTrialSeq < NUM_TRIALSEQ); }

   // does current trial sequencing mode count trial "blocks"?
   BOOL DoesModeUseBlocks() const
   {
      int i = m_ctrl.iTrialSeq;
      return(!(i == THISTRIAL || i == THISTRIAL_NF || i == STAIRCASE || i == STAIRCASE_NF));
   }

   // is current trial sequencing mode one of the "chained" modes?
   BOOL IsChainedMode() const { return(m_ctrl.iTrialSeq == CHAINED_NF || m_ctrl.iTrialSeq == CHAINED); }

   // is current trial sequencing mode one of the "staircase" modes?
   BOOL IsStaircaseMode() const { return(m_ctrl.iTrialSeq == STAIRCASE_NF || m_ctrl.iTrialSeq == STAIRCASE); }

   // get total number of trials participating in the current trial sequence
   int GetNumTrialsSequenced() const { return(m_bInitialized ? static_cast<int>(m_arTrials.GetSize()) : 0); }

   // get object key of the currently selected trial. Returns CX_NULLOBJ_KEY if no trial is selected.
   WORD GetCurrentTrialKey() const
   {
      WORD wKey = CX_NULLOBJ_KEY;
      if(m_bInitialized && (m_iSelected >= 0) && (m_iSelected < m_arTrials.GetSize())) wKey = m_arTrials[m_iSelected];
      return(wKey);
   }

   // get index position of trial key in the master list of trials currently being sequenced. -1 if not found
   int GetIndexForTrialKey(WORD wKey) const
   {
      int idx = -1;
      if(m_bInitialized)
      {
         for(int i=0; i<m_arTrials.GetSize(); i++) if(m_arTrials[i] == wKey)
         {
            idx =i;
            break;
         }
      }
      return(idx);
   }
   
   // get number of trials successfully completed since sequencing started
   BOOL GetTrialCount() const { return( m_bInitialized ? m_nTrialsDone : 0 ); }

   // trial stats: Returns 0 if trial index (zero-based ordinal position in trial set) invalid
   BOOL GetNumAttempted(int iTrial) const
   {
      if(iTrial < 0 || iTrial >= m_Stats.GetSize()) return(0);
      return(m_Stats[iTrial]->nAttempted);
   }
   BOOL GetNumCompleted(int iTrial) const
   {
      if(iTrial < 0 || iTrial >= m_Stats.GetSize()) return(0);
      return(m_Stats[iTrial]->nCompleted);
   }
   // returns 0 if trial index invalid, or chainLen not in [1..11], or not in chained seq mode
   BOOL GetNumSuccessfulChains(int iTrial, int chainLen) const
   {
      if(iTrial < 0 || iTrial >= m_Stats.GetSize() || chainLen < 1 || chainLen > MAX_CHAINLEN || !IsChainedMode())
         return(0);
      return(m_Stats[iTrial]->chainReps[chainLen-1]);
   }
   // returns "" if trial index invalid
   LPCTSTR GetTrialName(int iTrial) const
   {
      if(iTrial < 0 || iTrial >= m_Stats.GetSize()) return(_T(""));
      return((LPCTSTR) m_Stats[iTrial]->name);
   }

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL Init( const TRIALSEQCTRL& tsqc );                // init sequencer IAW new ctrl parms; select first trial
   BOOL GetTargets( int* pN, const int nMax,             // get defns of all targets used across all trials in seq
                    CXTARGET* pTg ) const;
   WORD SelectNextTrial( DWORD& dwTrialRes );            // select next trial in sequence
   BOOL WasTrialCompleted( DWORD dwTrialRes ) const;     // check trial result flags to see if trial completed w/o err
   WORD GetChannels() const;                             // get channel information associated with trial selected
   BOOL GetTrialInfo( int* pNT, int* pTgMap,             // retrieve trial target map, trial codes, and other info
      int* pN, const int nMax, TRIALCODE* pCodes,        // defining the currently selected trial
      DWORD* pFlags, int* pNSects, TRIALSECT* pSections,
      int* piDur, int* piT0, int* piT1,
      BOOL& bSave );

   // set mode and stop count for sequencer's auto-stop feature
   VOID SetAutoStopParams(int mode, int count);

   // call this to indicate ongoing sequence is paused. Internal state flag is cleared on the next call to 
   // GetTrialInfo(), and it is assumed the sequence has resumed.
   VOID SetPaused() { m_bSeqPaused = TRUE; } 

   double GetStartingPosH() const;                       // get/set global starting pos for trial tgts
   VOID SetStartingPosH( double hPos );
   double GetStartingPosV() const;
   VOID SetStartingPosV( double vPos );

   double GetTgtPosScale() const;                        // get/set global tgt pos scale & rotation
   VOID SetTgtPosScale( double d );
   double GetTgtPosRotate() const;
   VOID SetTgtPosRotate( double d );

   double GetTgtVelScale() const;                        // get/set global tgt vel scale & rotation
   VOID SetTgtVelScale( double d );
   double GetTgtVelRotate() const;
   VOID SetTgtVelRotate( double d );

   BOOL IsChanCfgOverride() const;                       // get/set global channel configuration override
   VOID SetChanCfgOverride( BOOL on );
   WORD GetChanCfgOverrideKey() const;
   VOID SetChanCfgOverrideKey( WORD wKey );


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   VOID RotateAndScaleVector( double& dH, double& dV,    // rotate & scale a vector IAW current position or velocity
      const BOOL bPos, const CCxTrial* pTrial ) const;   //    transform
   VOID Reset();                                         // reset trial staircase sequencer to an "empty" state
   VOID ShuffleSubsets();                                // shuffle trial subsets (SUBSETSEQ_RANDOM mode only)
   VOID InitWeightedReps();                              // init block of trials for RANDOM or WT_ORDERED seq
   VOID GetNextWeightedTrial( DWORD& dwTrialRes );       // select next trial in RANDOM or WT_ORDERED seq of trial set
   BOOL InitChainedReps();                               // analogously for CHAINED modes
   VOID GetNextChainedTrial( DWORD& dwTrialRes ); 
   BOOL InitStaircases();                                // prepare for STAIRCASE sequencing of trial set
   BOOL CheckStaircaseTrial( CCxTrial* pTrial );         // is trial defn compatible with staircase sequencing?
   VOID GetNextStaircaseTrial( DWORD& dwTrialRes );      // select next trial in STAIRCASE sequencing of trial set
   VOID LogStaircaseStatus( int iResult );               // log approp. status message re: staircase sequence
};


#endif   // !defined(CXTRIALSEQ_H__INCLUDED_)
