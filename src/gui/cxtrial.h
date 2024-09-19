//=====================================================================================================================
//
// cxtrial.h : Declaration of classes CCxTrial and CCxSegment.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXTRIAL_H__INCLUDED_)
#define CXTRIAL_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxtempl.h"                        // for CTypedPtrList template
#include "treemap.h"                         // the CTreeMap/CTreeObj framework
#include "cxobj_ifc.h"                       // CNTRLX object "interface" defines, including trial-specific defines
#include "numedit.h"                         // for NUMEDITFMT struct defining format constraints of numeric params
#include "cxrandomvar.h"                     // CCxRandomVar
#include "funcparser.h"                      // CFunctionParser

class CCxRPDistro;                           // holds response data and stats for distr-based reward/penalty protocol

//=====================================================================================================================
// Declaration of class CCxSegment
//=====================================================================================================================
//
typedef CTypedPtrList<CPtrList, PTRAJINFO> CTrajList;    // to manage list of target trajectory records

class CCxSegment : public CObject
{
   DECLARE_SERIAL( CCxSegment )

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
protected:
   SEGHDR      m_hdr;            // segment header parameters:  min/max dur, etc...
   CTrajList   m_trajRecs;       // list of target trajectory records, one for each target participating in trial.



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxSegment( const CCxSegment& src );                  // no copy constructor defined
   CCxSegment& operator=( const CCxSegment& src );       // no assignment operator defined
                                                         // *** for clean exception handling, use default constructor
                                                         // and the Copy() method.
public:
   CCxSegment() { AssignDefaultHeader(); }               // default constructor for dyn object creation mechanism
   ~CCxSegment() { RemoveAllTraj(); }                    // destroy segment -- release memory for target traj records!



//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   int TrajCount() const { return( static_cast<int>(m_trajRecs.GetCount()) ); }
   BOOL IsValidTraj( int iTg ) const { return( BOOL( iTg>=0 && iTg < TrajCount() ) ); }


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   VOID Copy( const CCxSegment& src );                   // copy the contents of the source segment to *this*
   BOOL AllocTraj( const int nAdd );                     // alloc & append some target trajectory records
   BOOL InsertTraj( const int iPos );                    // insert one target trajectory record at given pos
   BOOL RemoveTraj( const int iPos );                    // remove a target trajectory record at given pos in list
   VOID RemoveAllTraj();                                 // remove all target trajectory records in segment

   VOID GetHeader( SEGHDR& hdr ) const { hdr = m_hdr; }  // retrieve segment header (copied)
   BOOL SetHeader( SEGHDR& hdr );                        // modify segment header
   VOID GetTrajInfo( int iPos, TRAJINFO& traj ) const;   // retrieve a tgt traj record (copied)
   BOOL SetTrajInfo( int iPos, TRAJINFO& traj );         // modify a tgt traj record

   void Serialize( CArchive& ar );                       // for reading/writing segment info from/to disk file


//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================
public:
   // NOTE: Min and max segment duration may be assigned to a trial RV, x0..x9. When this is the case, both are always
   // assigned to the same RV, and Get***Duration() returns a negative integer N in [-10 .. -1], such that RV's index
   // in [0..9] is given by I = abs(N)-1.
   int GetMinDuration() const { return( m_hdr.iMinDur ); }
   BOOL SetMinDuration( int iVal );

   int GetMaxDuration() const { return( m_hdr.iMaxDur ); }
   BOOL SetMaxDuration( int iVal );

   int GetFixTarg1Pos() const { return( m_hdr.iFixTarg1 ); }
   BOOL SetFixTarg1Pos( int iVal );

   int GetFixTarg2Pos() const { return( m_hdr.iFixTarg2 ); }
   BOOL SetFixTarg2Pos( int iVal );

   double GetFixAccH() const { return( (double) m_hdr.fFixAccH ); }
   BOOL SetFixAccH( double dVal );

   double GetFixAccV() const { return( (double) m_hdr.fFixAccV ); }
   BOOL SetFixAccV( double dVal );

   int GetGracePeriod() const { return( m_hdr.iGrace ); }
   BOOL SetGracePeriod( int iVal );

   BOOL IsMidTrialRewEnable() const { return( m_hdr.bEnaRew ); }
   BOOL SetMidTrialRewEnable( BOOL bVal );

   int GetXYFramePeriod() const { return( m_hdr.iXYFrame ); }
   BOOL SetXYFramePeriod( int iVal );

   int GetMarker() const { return( m_hdr.iMarker ); }
   BOOL SetMarker( int iVal );

   BOOL IsResponseChecked() const { return( m_hdr.bChkResp ); }
   BOOL SetResponseChecked( BOOL bVal );

   BOOL IsRMVSyncFlashOn() const { return(m_hdr.bEnaRMVSync); }
   BOOL SetRMVSyncFlashOn(BOOL bOn);

   BOOL IsTgtOn( const int iTg ) const { return( (GetTraj( iTg )->dwFlags & SGTJF_ON) != 0 ); }
   BOOL SetTgtOn( const int iTg, BOOL bVal );

   BOOL IsAbsolutePos( const int iTg ) const { return( (GetTraj( iTg )->dwFlags & SGTJF_ABS) != 0 ); }
   BOOL SetAbsolutePos( const int iTg, BOOL bVal );

   int GetTgtVStabMode( const int iTg ) const { return( FLAGS_TO_VSTABMODE( GetTraj(iTg)->dwFlags ) ); }
   BOOL SetTgtVStabMode( const int iTg, int iMode );

   BOOL IsTgtVStabSnapToEye( const int iTg ) const { return( (GetTraj( iTg )->dwFlags & SGTJF_VSTABSNAP) != 0 ); }
   BOOL SetTgtVStabSnapToEye( const int iTg, BOOL bVal );

   double GetTgtTrajParam(int t, int p, BOOL& isRV) const;
   BOOL SetTgtTrajParam(int t, int p, double dVal, BOOL isRV);

//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
public:
#ifdef _DEBUG
   void Dump( CDumpContext& dc ) const;                  // dump the segment info
   void AssertValid() const;                             // validate internal consistency of segment object
#endif



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   PTRAJINFO GetTraj( const int iTg ) const              // retrieve ptr to a particular target traj record
   {
      ASSERT( IsValidTraj( iTg ) );
      return( m_trajRecs.GetAt( m_trajRecs.FindIndex( iTg ) ) );
   }

   VOID AssignDefaultHeader();                           // assign defaults to segment header
   VOID AssignDefaultTraj( PTRAJINFO pTraj );            // assign default values to a target traj record
   float LimitTraj( const float fVal, const float fLim,  // limit range of f-pt value to +/- limit; set flag FALSE
                    BOOL& bFlag ) const;                 // if a change was made, else leave flag unchanged.

};




//=====================================================================================================================
// Declaration of class CCxTrial
//=====================================================================================================================
//
typedef CTypedPtrList<CObList, CCxSegment*> CSegList;       // to manage list of segments in trial
typedef CTypedPtrList<CPtrList, PTRIALSECT> CSectList;      // to manage list of tagged sections in trial

class CCxTrial : public CTreeObj
{
   DECLARE_SERIAL( CCxTrial )

   friend class CCxTreeMap;                  // so that CNTRLX tree map can control naming of its data objects

//=====================================================================================================================
// CONSTANTS/DEFINITIONS
//=====================================================================================================================
public:
   typedef enum                              // parameter IDs for all parameters in the trial's segment table.
   {                                         // these are intended for use with the Get/SetSegParam***() methods.
      NOTAPARAM = -1,
      MINDURATION = 100,
      MAXDURATION,
      XYFRAMEPERIOD,
      RMVSYNCENA,
      FIXTARG1,
      FIXTARG2,
      FIXACCH,
      FIXACCV,
      FIXGRACE,
      REWENA,
      SEGMARKER,
      CHECKRESP,
      TGTONOFF,
      TGTPOSABS,
      TGTVSTABMODE, 
      TGTVSTABSNAP,
      TGTHPOS,
      TGTVPOS,
      TGTHVEL,
      TGTVVEL,
      TGTHACC,
      TGTVACC,
      PATHVEL,
      PATVVEL,
      PATHACC,
      PATVACC
   } ParamID;

   // random variable definition: type; up to 3 distribution parameters; and the function string for RV_FUNCTION
   struct CRVEntry
   {
      int iType;
      int iSeed;  // non-negative: 0 = random seed; else fixed for repeatability
      double dParams[3];
      CString strFunc;
   };

private:
   // an entry in the trial's perturbation list
   struct CPertEntry
   {
      WORD     wKey;                         //    object key identifying the perturbation (CX_NULLOBJ_KEY if not used)
      float    fAmp;                         //    perturbation amplitude in deg/sec
      char     cSeg;                         //    start segment index (if invalid, entry is ignored)
      char     cTgt;                         //    affected target index (if invalid, entry is ignored)
      char     cIdCmpt;                      //    ID of trajectory component modulated by pert (PERT_ON_* constant)
   };

   // runtime state information on a trial random variable -- valid only during trial sequencing (not serialized)
   struct CRVState
   {
      CFunctionParser* pFunc;                // ptr to the function parser, or NULL
      CCxRandomVar* pRV;                     // ptr to the distributed random variable object, or NULL
      double dCurrVal;                       // the RV's current value
   };

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
protected:
   TRLHDR      m_hdr;                        // trial "header" -- general trial attributes, control parameters
   CWordArray  m_wArTargs;                   // list of unique keys identifying the targets participating in trial
   CSegList    m_Segments;                   // the trial segments
   int         m_nPerts;                     // # of entries in perturbation list
   CPertEntry  m_Perts[MAX_TRIALPERTS];      // the perturbation list
   CSectList   m_TaggedSections;             // list of tagged sections
   CRVEntry    m_Vars[MAX_TRIALRVS];         // the random variable list

   CRVState    m_VarState[MAX_TRIALRVS];     // runtime state of any RVs during trial sequencing. Not serialized.
   CCxRPDistro* m_pRPDistro;                 // if trial has "R/P Distrib" special op, this transient obj holds
                                             // behavioral response samples and reward/penalty stats. Not serialized.

   // runtime state for random reward withholding during trial sequencing: shuffle lists for reward #1, #2. See class
   // header for details.
   CList<int, int> m_rew1WHVRShuffleList;
   CList<int, int> m_rew2WHVRShuffleList;

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrial( const CCxTrial& src );                      // no copy constructor defined
   CCxTrial& operator=( const CCxTrial& src );           // no assignment operator defined

protected:                                               // CREATION ACCESS RESTRICTED!
   CCxTrial()                                            // constructor required for dyn object creation mechanism
   {
      AssignDefaultHeader();
      // ensure dynamic runtime random variable state is initialized
      m_pRPDistro = NULL;
      for(int i=0; i<MAX_TRIALRVS; i++)
      {
         m_VarState[i].pFunc = (CFunctionParser*) NULL;
         m_VarState[i].pRV = (CCxRandomVar*) NULL;
         m_VarState[i].dCurrVal = 0;
      }
   }
   ~CCxTrial() { Clear(); }                              // destroy trial object -- freeing any allocated memory

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f); // initialize trial object after default construction
   VOID Copy( const CTreeObj* pSrc );                    // make THIS trial object a copy of the specified trial

public:
   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a trial from a different treemap
         const CWordToWordMap& depKeyMap);


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   int SegCount() const { return( static_cast<int>(m_Segments.GetCount()) ); }
   BOOL IsValidSeg( int iSeg ) const { return( BOOL( iSeg>=0 && iSeg < SegCount() ) ); }
   int TargCount() const { return( static_cast<int>(m_wArTargs.GetSize()) ); }
   BOOL IsValidTarg( int iTg ) const { return( BOOL( iTg>=0 && iTg < TargCount() ) ); }
   BOOL IsValidTrajRecord( int iSeg, int iTg ) const { return( IsValidSeg( iSeg ) && IsValidTarg( iTg ) ); }

   BOOL CanPasteSeg( const CCxSegment* pSeg ) const      // can specified segment by pasted into this trial?
   {
      return( (pSeg != NULL) &&
              (SegCount() < MAX_SEGMENTS) &&
              (pSeg->TrajCount() == TargCount()) );
   }
   BOOL CanReplaceSeg( const CCxSegment* pSeg ) const    // can specified seg replace an existing seg in this trial?
   {
      return( (pSeg != NULL) &&
              (pSeg->TrajCount() == TargCount()) );
   }
   BOOL CanRemove() const                                // prevent removal of "predefined" CNTRLX trial
   { return( (m_flags & CX_ISPREDEF) == 0 ); }           //

   VOID GetDependencies( CWordArray& wArKeys ) const;    // return list of CNTRLX object currently ref'd by trial

   BOOL IsResponseChecked() const;                       // does trial definition check subject's resp (staircase seq)?


//=====================================================================================================================
// OPERATIONS -- GENERAL
//=====================================================================================================================
public:
   VOID GetHeader( TRLHDR& hdr ) const { hdr = m_hdr; }  // retrieve trial header (copied)
   BOOL SetHeader( TRLHDR& hdr, BOOL& bChanged );        // modify trial header, with auto-correction

   CCxRPDistro* GetRPDistro();                           // get obj that stores runtime info for "R/P Distro" operation

   int InsertSeg( const int iPos );                      // insert a segment at given pos in segment list
   BOOL RemoveSeg( const int iPos );                     // remove a segment at given pos in segment list
   CCxSegment* CutSeg( const int iPos );                 // remove seg from trial, w/o deleting the seg object
   CCxSegment* CopySeg( const int iPos );                // provide a duplicate of a given segment in trial
   int PasteSeg( const int iPos,                         // paste copy of provided segment into the trial's seg list
                 const CCxSegment* pSeg);                //
   BOOL ReplaceSeg( const int iPos,                      // replace existing segment's defn in place
                    const CCxSegment* pSeg );            //
   BOOL InsertTarget( const int iPos,                    // insert a target at given pos in target list
                      const WORD wTargKey );             //
   BOOL RemoveTarget( const int iPos );                  // remove a target at given pos in target list

   WORD GetTarget( const int iPos ) const                // get key/ID of target at given pos in target list
   {
      ASSERT( IsValidTarg( iPos ) );
      return( m_wArTargs[iPos] );
   }
   VOID GetTargetSet( CWordArray& arTargs ) const        // retrieve entire target list
   {
      arTargs.Copy( m_wArTargs );
   }
   BOOL SetTarget(const int iPos, const WORD wTargKey);  // replace an existing target with a different one

   VOID Clear();                                         // empty trial and set trial header to defaults
   void Serialize( CArchive& ar );                       // for reading/writing trial object from/to disk file
   BOOL Import( CStringArray& strArDefn,                 // set trial IAW cntrlxUNIX-style, text-based definition
      const CMap<CString, LPCTSTR, WORD, WORD>& importMap, CString& strMsg );


//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================
public:
   BOOL IsSaved() const { return( BOOL((m_hdr.dwFlags & THF_KEEP) != 0) ); }
   BOOL IsMidTrialRewPeriodic() const { return( BOOL((m_hdr.dwFlags & THF_MTRMODE) == 0) ); }
   BOOL IsMidTrialRewAtSegEnd() const { return( BOOL((m_hdr.dwFlags & THF_MTRMODE) == THF_MTRMODE) ); }
   BOOL IsScalePosIgnored() const { return( BOOL((m_hdr.dwFlags & THF_IGNPOSSCALE) == THF_IGNPOSSCALE) ); }
   BOOL IsRotatePosIgnored() const { return( BOOL((m_hdr.dwFlags & THF_IGNPOSROT) == THF_IGNPOSROT) ); }
   BOOL IsScaleVelIgnored() const { return( BOOL((m_hdr.dwFlags & THF_IGNVELSCALE) == THF_IGNVELSCALE) ); }
   BOOL IsRotateVelIgnored() const { return( BOOL((m_hdr.dwFlags & THF_IGNVELROT) == THF_IGNVELROT) ); }

   int GetCorrectResponseChan() const { return( ((m_hdr.dwFlags & THF_STAIRRESP) != 0) ? 13 : 12 ); }
   int GetIncorrectResponseChan() const { return( ((m_hdr.dwFlags & THF_STAIRRESP) != 0) ? 12 : 13 ); }
   int GetWeight() const { return( m_hdr.iWeight ); }
   int GetStairNum() const { return( m_hdr.iStairNum ); }
   int GetSaveSegPos() const { return( m_hdr.iStartSeg ); }
   int GetFailsafeSegPos() const { return( m_hdr.iFailsafeSeg ); }
   int GetSpecialSegPos() const { return( m_hdr.iSpecialSeg ); }
   int GetSpecialOp() const { return(m_hdr.iSpecialOp); }
   int GetMarkSeg1Pos() const { return( m_hdr.iMarkSeg1 ); }
   int GetMarkSeg2Pos() const { return( m_hdr.iMarkSeg2 ); }
   int GetMidTrialRewardIntv() const { return( m_hdr.iMTRIntv ); }
   int GetMidTrialRewardLen() const { return( m_hdr.iMTRLen ); }
   int GetNumXYInterleave() const { return( m_hdr.nXYInterleave ); }
   int GetSaccadeThreshold() const { return( m_hdr.iSaccVt ); }
   int GetReward1PulseLen() const { return( m_hdr.reward1[0] ); }
   int GetReward1WHVRNumerator() const { return(m_hdr.reward1[1]); }
   int GetReward1WHVRDenominator() const { return(m_hdr.reward1[2]); }
   int GetReward2PulseLen() const { return(m_hdr.reward2[0]); }
   int GetReward2WHVRNumerator() const { return(m_hdr.reward2[1]); }
   int GetReward2WHVRDenominator() const { return(m_hdr.reward2[2]); }
   double GetStairStrength() const { return( (double) m_hdr.fStairStrength ); }
   WORD GetChannels() const { return( m_hdr.wChanKey ); }
   VOID SetChannels( WORD wChanKey )  { m_hdr.wChanKey = wChanKey; }
   int GetSgmSegPos() const { return( m_hdr.iSGMSeg ); }
   VOID GetSgmParms( SGMPARMS& sgm ) const { sgm = m_hdr.sgm; }

   int GetAltXYDotSeed() const { return(m_hdr.iXYDotSeedAlt); }

   int GetFixTarg1Pos( int iSeg ) const { return( RetrieveSegment( iSeg )->GetFixTarg1Pos() ); }
   BOOL SetFixTarg1Pos( int iSeg, int iVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetFixTarg1Pos( iVal ) : FALSE );
   }

   int GetFixTarg2Pos( int iSeg ) const { return( RetrieveSegment( iSeg )->GetFixTarg2Pos() ); }
   BOOL SetFixTarg2Pos( int iSeg, int iVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetFixTarg2Pos( iVal ) : FALSE );
   }

   double GetFixAccH( int iSeg ) const { return( RetrieveSegment( iSeg )->GetFixAccH() ); }
   BOOL SetFixAccH( int iSeg, double dVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetFixAccH( dVal ) : FALSE );
   }

   double GetFixAccV( int iSeg ) const { return( RetrieveSegment( iSeg )->GetFixAccV() ); }
   BOOL SetFixAccV( int iSeg, double dVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetFixAccV( dVal ) : FALSE );
   }

   int GetGracePeriod( int iSeg ) const { return( RetrieveSegment( iSeg )->GetGracePeriod() ); }
   BOOL SetGracePeriod( int iSeg, int iVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetGracePeriod( iVal ) : FALSE );
   }

   BOOL IsMidTrialRewEnable( const int iSeg ) const { return( RetrieveSegment( iSeg )->IsMidTrialRewEnable() ); }
   BOOL SetMidTrialRewEnable( int iSeg, BOOL bVal  )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetMidTrialRewEnable( bVal ) : FALSE );
   }

   int GetXYFramePeriod( int iSeg ) const { return( RetrieveSegment( iSeg )->GetXYFramePeriod() ); }
   BOOL SetXYFramePeriod( int iSeg, int iVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetXYFramePeriod( iVal ) : FALSE );
   }

   BOOL IsRMVSyncFlashOn(int iSeg) const { return(RetrieveSegment(iSeg)->IsRMVSyncFlashOn()); }
   BOOL SetRMVSyncFlashOn(int iSeg, BOOL bOn)
   {
      return(IsValidSeg(iSeg) ? RetrieveSegment(iSeg)->SetRMVSyncFlashOn(bOn) : FALSE);
   }

   int GetMarker( int iSeg ) const { return( RetrieveSegment( iSeg )->GetMarker() ); }
   BOOL SetMarker( int iSeg, int iVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetMarker( iVal ) : FALSE );
   }

   BOOL IsResponseChecked( const int iSeg ) const { return( RetrieveSegment( iSeg )->IsResponseChecked() ); }
   BOOL SetResponseChecked( int iSeg, BOOL bVal )
   {
      return( IsValidSeg( iSeg ) ? RetrieveSegment( iSeg )->SetResponseChecked( bVal ) : FALSE );
   }

   BOOL IsTgtOn( int iSeg, int iTg ) const { return( RetrieveSegment( iSeg )->IsTgtOn( iTg ) ); }
   BOOL SetTgtOn( int iSeg, int iTg, BOOL bVal )
   {
      return( IsValidTrajRecord( iSeg, iTg ) ? RetrieveSegment( iSeg )->SetTgtOn( iTg, bVal ) : FALSE );
   }

   BOOL IsAbsolutePos( const int iSeg, const int iTg ) const { return( RetrieveSegment(iSeg)->IsAbsolutePos(iTg) ); }
   BOOL SetAbsolutePos( int iSeg, int iTg, BOOL bVal )
   {
      return( IsValidTrajRecord( iSeg, iTg ) ? RetrieveSegment( iSeg )->SetAbsolutePos( iTg, bVal ) : FALSE );
   }

   int GetTgtVStabMode(const int iSeg, const int iTg) const 
   { 
      return( RetrieveSegment(iSeg)->GetTgtVStabMode(iTg) ); 
   }
   BOOL SetTgtVStabMode(const int iSeg, const int iTg, int iMode)
   {
      return( IsValidTrajRecord( iSeg, iTg ) ? RetrieveSegment( iSeg )->SetTgtVStabMode( iTg, iMode ) : FALSE );
   }

   BOOL IsTgtVStabSnapToEye( int iSeg, int iTg ) const { return(RetrieveSegment( iSeg )->IsTgtVStabSnapToEye( iTg )); }
   BOOL SetTgtVStabSnapToEye( int iSeg, int iTg, BOOL bVal )
   {
      return( IsValidTrajRecord( iSeg, iTg ) ? RetrieveSegment( iSeg )->SetTgtVStabSnapToEye( iTg, bVal ) : FALSE );
   }

private:
   // the target trajectory parameters and segment min/max duration are RV-assignable parameters. As such, getting
   // and setting their values for the purpose of editing is unconventional. So these methods are private. Use the
   // Get/SetSegParam() methods to edit these params, and use the GetCurr***() methods to get their current value for
   // the purposes of preparing for runtime trial presentation.
   double GetTgtTrajParam(int s, int t, ParamID p, BOOL& isRV) const;
   BOOL SetTgtTrajParam(int s, int t, ParamID p, double dVal, const BOOL asRV);

   int GetMinDuration(int s) const { return(IsValidSeg(s) ? RetrieveSegment(s)->GetMinDuration() : 0); }
   BOOL SetMinDuration(int s, int iVal) { return(IsValidSeg(s) ? RetrieveSegment(s)->SetMinDuration(iVal) : FALSE ); }

   int GetMaxDuration(int s) const { return(IsValidSeg(s) ? RetrieveSegment(s)->GetMaxDuration() : 0); }
   BOOL SetMaxDuration(int s, int iVal) { return(IsValidSeg(s) ? RetrieveSegment(s)->SetMaxDuration(iVal) : FALSE ); }

public:
   // generic segment table parameter access via parameter ID...
   BOOL IsValidSegParam(int s, int t, ParamID p) const; 
   double GetSegParam( int s, int t, ParamID p ) const;
   int GetSegParamAsInt( int s, int t, ParamID p ) const;
   VOID GetSegParam( int s, int t, ParamID p, CString& str ) const;
   VOID GetSegParamLabel( ParamID p, CString& str ) const;
   VOID GetSegParamFormat( ParamID p, BOOL& bIsChoice, CStringArray& choices, NUMEDITFMT& fmt ) const;
   BOOL IsSegParamMultiChoice( ParamID p ) const;
   BOOL SetSegParam( int s, int t, ParamID p, double dVal, BOOL asRV=FALSE);
   BOOL SetSegParam( int s, int t, ParamID p, int iVal )
   {
      return( SetSegParam( s, t, p, double(iVal) ) );
   }
   BOOL CanAssignRVToSegParam(int s, int t, ParamID p) const;
   BOOL IsRVAssignedToSegParam(int s, int t, ParamID p) const;

//=====================================================================================================================
// OPERATIONS -- PERTURBATION LIST
//=====================================================================================================================
public:
   int PertCount() { return( m_nPerts ); }               // #entries currently in trial's perturbation list
   BOOL IsValidPert( int iPos )                          // is this a valid entry in perturbation list
   {
      return( BOOL( iPos>=0 && iPos<m_nPerts ) );
   }
   BOOL AppendPert( WORD wKey );                         // append an entry to the perturbation list
   BOOL RemovePert( int iPos );                          // remove a selected entry from the perturbation list

   BOOL SetPert( int iPos, WORD wKey, float fAmp,        // modify pertubation entry
                 int iSeg, int iTgt, int idVel );

   WORD GetPertKey( int iPos );                          // get/set identity (ie, object key) of a perturbation entry
   BOOL SetPertKey( int iPos, WORD wKey );
   float GetPertAmp( int iPos );                         // get/set desired amplitude for a perturbation entry
   BOOL SetPertAmp( int iPos, float fAmp );
   int GetPertSeg( int iPos );                           // get/set index of perturbation's start segment
   BOOL SetPertSeg( int iPos, int iSeg );
   int GetPertTgt( int iPos );                           // get/set index of trial tgt affected by a perturbation entry
   BOOL SetPertTgt( int iPos, int iTgt );
   WORD GetPertTgtKey( int iPos );                       // get key of perturbed trial tgt
   int GetPertTrajCmpt( int iPos );                      // get/set ID of traj cmpt affected by a pertubation entry
   BOOL SetPertTrajCmpt( int iPos, int idCmpt );

//=====================================================================================================================
// OPERATIONS -- TAGGED SECTIONS
//=====================================================================================================================
public:
   BOOL HasTaggedSections() const;                       // does this trial have any tagged sections?
   int GetNumTaggedSections() const;                     // number of tagged sections defined for this trial
   int GetNumTaggedSegments() const;                     // total #segments in trial that are part of a tagged section
   BOOL GetTaggedSection(int i, TRIALSECT& sect) const;  // retrieve info for a tagged section
   int GetTaggedSectionByName(LPCTSTR tag) const;        // retrieve index of tagged section with the given tag name
   BOOL GetTaggedSectionName(int i, CString& tag) const; // retrieve label for a tagged section
   BOOL CreateTaggedSection(int s0, int s1);             // create a tagged section containing segs [s0..s1]
   BOOL CreateTaggedSection(int s0, int s1, LPCTSTR tag);
   BOOL RenameTaggedSection(int i, LPCTSTR tag);         // rename an existing tagged section
   BOOL RemoveTaggedSection(int i);                      // remove a tagged section
   VOID RemoveAllTaggedSections();                       // remove all tagged sections from trial

private:
   VOID UpdateTaggedSectionsOnSegRemove( int iSeg );
   VOID UpdateTaggedSectionsOnSegInsert( int iSeg );

//=====================================================================================================================
// OPERATIONS -- RANDOM VARIABLES
// NOTE: Unlike the pertubation list, each trial has 10 RVs, any subset of which may be in use. There is no Add/Remove
// operation for RVs -- you simply set the type to RV_NOTUSED to "remove" it.
//=====================================================================================================================
public:
   BOOL IsRVInUse(int idx) const { return(idx>=0 && idx<MAX_TRIALRVS && m_Vars[idx].iType != RV_NOTUSED); }
   BOOL GetRV(int idx, CRVEntry& rv) const;
   BOOL SetRVParam(int idx, int id, const CRVEntry& rv, BOOL& bSideEffect);

   // initialize or update runtime state of any defined RVs before trial sequencing begins
   BOOL UpdateRVs(BOOL bInit, CString& errMsg);

   // get current value of any segment table parameter that might be assigned to a random variable - these methods
   // must be used when preparing the trial for presentation!
   int GetCurrMinDuration(int s) const;
   int GetCurrMaxDuration(int s) const;
   int GetWorstCaseDuration(int s) const;
   double GetCurrTgtTrajParam(int s, int t, ParamID p) const;

private:
   VOID RemoveAllRVs();
   VOID ClearRVRuntimeState();

// random reward withhholding during trial seqencing
public:
   VOID InitRewardWHVR(const BOOL initRew1 = TRUE, const BOOL initRew2 = TRUE);
   VOID UpdateRewardWHVR(BOOL& bGiveRew1, BOOL& bGiveRew2);

//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
public:
#ifdef _DEBUG
   void Dump( CDumpContext& dc ) const;                  // dump the trial info
   void AssertValid() const;                             // validate internal consistency of the trial object
#endif



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   CCxSegment* RetrieveSegment( const int i ) const      // retrieve ptr to segment at zero-based pos in segment list
   {
      ASSERT( IsValidSeg( i ) );
      return( m_Segments.GetAt( m_Segments.FindIndex( i ) ) );
   }
   VOID AssignDefaultHeader();                           // assign defaults to trial header
   BOOL IsSameHeader( const TRLHDR& hdr ) const;         // TRUE if current trial header is identical to provided hdr

};


#endif   // !defined(CXTRIAL_H__INCLUDED_)
