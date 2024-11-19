//===================================================================================================================== 
//
// cxcontrun.h : Declaration of classes CCxContRun and CCxStimulus.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(CXCONTRUN_H__INCLUDED_)
#define CXCONTRUN_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxtempl.h"                        // for CTypedPtrList template 
#include "numedit.h"                         // for numeric edit format struct NUMEDITFMT
#include "treemap.h"                         // the CTreeMap/CTreeObj framework
#include "cxobj_ifc.h"                       // CNTRLX object "interface" defines, including run-specific defines 


//===================================================================================================================== 
// Declaration of class CCxStimulus
//===================================================================================================================== 
//
class CCxStimulus : public CObject
{
   DECLARE_SERIAL( CCxStimulus )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int NPARAMS[STIM_NTYPES][STIM_NMAXMODES];   // size of motion param list -- varies with type, mode
   static LPCTSTR STDMODESTRINGS[STIM_NSTDMODES];           // human-readable names for supported motion modes
   static LPCTSTR COMMONLBLSTRINGS[STIM_NCOMMON];           // labels for the common parameters


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
                              // the stimulus channel's "common" parameters:
   BOOL        m_bOn;         //    TRUE if stimulus should be played during the run 
   int         m_iMarker;     //    DO ch# for pulse marking stimulus start (0=OFF)
   int         m_iType;       //    stimulus channel type 
   int         m_iStdMode;    //    motion mode for the "standard" stimulus types (motion mode for XYseq & PSGM is 
                              //       stored with relevant motion parameter set
   int         m_tStart;      //    stimulus start time within run's duty cycle, in ms 

                              // the "motion" parameter sets -- which set is used depends on stim type & motion mode
   SINESTIM    m_sine;        //    for sinusoidal motion mode
   PULSESTIM   m_pulse;       //    for trapezoidal pulse motion mode


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxStimulus( const CCxStimulus& src );                // no copy constructor defined
   CCxStimulus& operator=( const CCxStimulus& src );     // no assignment operator defined

public:
   CCxStimulus() { SetDefaults(); }                      // default constructor for dyn object creation mechanism 
   ~CCxStimulus() {} 



//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   static int NumberOfCommonParameters()                 // first N parameters are common to all stimulus types, modes
   {
      return( STIM_NCOMMON );
   }
   static VOID GetCommonParameterLabel( int i, CString& str ) 
   {
      str.Empty();
      if( i >= 0 && i < STIM_NCOMMON ) str = COMMONLBLSTRINGS[i];
   }
   static int MaxNumberOfMotionParameters()              // worst-case # of motion parameters for any stim type, mode
   {
      return( MAXSTIMPARAMS-STIM_NCOMMON );
   }
   int NumberOfMotionParameters() const                  // size of motion parameter list (not including common parms) 
   {
      return( NPARAMS[GetType()][GetMotionMode()] );
   }
   int NumberOfParameters() const                        // total # of parameters defining stimulus
   {
      return( STIM_NCOMMON + NumberOfMotionParameters() );
   }
   BOOL IsValidParameter( int i ) const                  // is this a valid parameter index?
   {
      return( (i >= 0) && (i < NumberOfParameters()) );
   }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public: 
   VOID GetStimulusInfo( STIMCHAN& stim ) const;         // retrieve stimulus definition in CXDRIVER-compatible format
   VOID SetStimulusInfo( const STIMCHAN& src );          // set stimulus channel definition as a unit, w/ auto-correct 
   VOID Copy( const CCxStimulus& src );                  // copy the contents of the source stimulus channel to *this*
   void Serialize( CArchive& ar );                       // for reading/writing object from/to disk file


//===================================================================================================================== 
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//===================================================================================================================== 
public:
   BOOL IsOn() const { return( m_bOn ); }                // access to common parameters by name...
   VOID SetOn( BOOL bOn ) { m_bOn = bOn; }
   int GetType() const { return( m_iType ); } 
   int GetMotionMode() const { return( m_iStdMode ); } 
   int GetMarker() const { return( m_iMarker ); }
   int GetStartTime() const { return( m_tStart ); }

                                                         // generic parameter access via an ordered "index"...
   double GetParameter( int i ) const;                   // get current value as a FP, string, or integer
   VOID GetParameter( int i, CString& str ) const; 
   int GetParameterAsInt( int i ) const;
   VOID GetParameterLabel( int i, CString& str ) const;  // get parameter label
   VOID GetParameterFormat( int i, BOOL& bIsChoice,      // get parameter format
         CStringArray& choices, NUMEDITFMT& fmt ) const;
   BOOL IsParameterMultiChoice( int i ) const;           // is it a multiple-choice parameter?
   BOOL SetParameter( int i, double dVal );              // set current value of parameter
   BOOL SetParameter( int i, int iVal )
   {
      return( SetParameter( i, double(iVal) ) );
   }


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump the object definition 
   void AssertValid() const;                             // validate internal consistency of object
#endif


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   VOID SetDefaults();                                   // set stimulus parameters to defaults
   VOID Validate();                                      // validate stimulus channel defn in its current state
};




//===================================================================================================================== 
// Declaration of class CCxContRun
//===================================================================================================================== 
//
typedef CTypedPtrList<CObList, CCxStimulus*> CCxStimuli; // to manage list of stimulus channels in the run

class CCxContRun : public CTreeObj
{
   DECLARE_SERIAL( CCxContRun )

   friend class CCxTreeMap;                  // so that CNTRLX tree map can control naming of its data objects  

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   int         m_iDutyPeriod;                // duty period in milliseconds
   int         m_iDutyPulse;                 // OFF (0), or DOUT ch# on which marker pulse is delivered per duty cycle 
   int         m_nAutoStop;                  // auto-stop the run after this many cycles elapsed (0 = no auto-stop)
   float       m_fHOffset;                   // horizontal position offset in deg subtended at eye
   float       m_fVOffset;                   // vertical position offset in deg subtended at eye
   CCxStimuli  m_Stimuli;                    // the currently defined stimulus channels in this run

   // deprecated but kept so we can deserialize existing documents containing runs with XYseq targets. After 
   // after deserialization, CCxDoc removes all stimulus runs with XYseq targets!!
   struct CXYseqTgt                          // for each XY target participating in an XYseq stim, we must save:
   {
      WORD  wKey;                            //    the target object's key
      float fCtrX, fCtrY;                    //    the (x,y) location of the target window's center, in subtended deg 
   };
   CTypedPtrList<CPtrList, CXYseqTgt*> m_XYseqTgts;


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxContRun( const CCxContRun& src );                  // no copy constructor defined
   CCxContRun& operator=( const CCxContRun& src );       // no assignment operator defined

protected:                                               // CREATION ACCESS RESTRICTED!
   CCxContRun() { SetDefaults(); }                       // constructor required for dyn object creation mechanism 
   ~CCxContRun() { Clear(); }                            // destroy run object -- freeing any allocated stim channels 

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f); // initialize run object after default construction
   VOID Copy( const CTreeObj* pSrc );                    // make THIS run object a copy of the specified run

public:
   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a stimulus run from a different treemap 
         const CWordToWordMap& depKeyMap);


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   static int GetMaxStimuli() { return( MAXSTIMULI ); }

   int GetStimulusCount() const { return( static_cast<int>(m_Stimuli.GetCount()) ); } 
   BOOL IsValidStimulus( int i ) const { return( BOOL( i>=0 && i < GetStimulusCount() ) ); }

   BOOL CanRemove() const                                // prevent removal of "predefined" CNTRLX run
   { return( (m_flags & CX_ISPREDEF) == 0 ); }           //

   const CCxStimulus* GetStimulus( int i ) const         // const access to individual stimulus channels in run 
   { 
      if( !IsValidStimulus( i ) ) return( NULL );
      else return( (const CCxStimulus*) RetrieveStimulus( i ) );
   }

   VOID GetDependencies( CWordArray& wArKeys ) const;    // return list of Maestro objects currently ref'd by run

//===================================================================================================================== 
// OPERATIONS -- GENERAL
//===================================================================================================================== 
public: 
   VOID GetDefinition( CONTRUN& runDef ) const;          // retrieve run definition in CXDRIVER-compatible format

   int InsertStimulus( int iPos );                       // insert a stimulus channel at given pos in list 
   BOOL RemoveStimulus( int iPos );                      // remove a stimulus channel -- stim obj deleted
   CCxStimulus* CutStimulus( int iPos );                 // remove stimulus channel -- stim obj NOT deleted
   CCxStimulus* CopyStimulus( int iPos ) const;          // provide a duplicate of a given stimulus channel
   int PasteStimulus(int iPos, const CCxStimulus* pStim);// paste copy of provided stimulus into the run's stim list 
   BOOL ReplaceStimulus( int iPos,                       // replace existing stimulus channel's defn in place 
                         const CCxStimulus* pStim );     //
   VOID ClearStimuli();                                  // empty the stimulus channel list

   VOID Clear();                                         // destroy all stimuli 
   void Serialize( CArchive& ar );                       // for reading/writing run object from/to disk file


//===================================================================================================================== 
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//===================================================================================================================== 
public: 
   int GetDutyPeriod() const { return( m_iDutyPeriod ); }
   VOID GetDutyPeriod( CString& str ) { str.Format( "%d", m_iDutyPeriod ); }
   VOID SetDutyPeriod( int iVal ) { m_iDutyPeriod = (iVal<0) ? 0 : iVal; }

   int GetDutyPulse() const { return( m_iDutyPulse ); }
   VOID GetDutyPulse( CString& str ) const 
   {
      if( m_iDutyPulse == 0 ) str = _T("OFF");
      else str.Format( "DOUT%d", m_iDutyPulse );
   }
   VOID SetDutyPulse( int iVal ) { m_iDutyPulse = (iVal<0 || iVal>STIM_NLASTMARKER) ? 0 : iVal; }
   static VOID GetDutyPulseChoices( CStringArray& choices );

   int GetAutoStop() const { return( m_nAutoStop ); }
   VOID GetAutoStop( CString& str ) { str.Format( "%d", m_nAutoStop ); }
   VOID SetAutoStop( int iVal ) { m_nAutoStop = (iVal<0) ? 0 : iVal; }

   double GetHOffset() const { return( m_fHOffset ); }
   VOID GetHOffset( CString& str ) { str.Format( "%.2f", m_fHOffset ); }
   VOID SetHOffset( double dVal ) { m_fHOffset = float( (dVal<-80.0) ? -80.0 : ((dVal>80.0) ? 80.0 : dVal) ); } 

   double GetVOffset() const { return( m_fVOffset ); }
   VOID GetVOffset( CString& str ) { str.Format( "%.2f", m_fVOffset ); }
   VOID SetVOffset( double dVal ) { m_fVOffset = float( (dVal<-80.0) ? -80.0 : ((dVal>80.0) ? 80.0 : dVal) ); } 


   BOOL IsValidStimParameter( int i, int j ) const       // access to parameters in a given stimulus channel...
   {
      return( IsValidStimulus( i ) ? GetStimulus( i )->IsValidParameter( j ) : FALSE );
   }

   int GetNumberOfStimParameters( int i ) const
   {
      return( IsValidStimulus( i ) ? GetStimulus( i )->NumberOfParameters() : 0 );
   }

   BOOL IsStimulusOn( int i ) const { return( IsValidStimulus( i ) ? GetStimulus( i )->IsOn() : FALSE ); }

   double GetStimParameter( int i, int j ) const 
   {
      return( IsValidStimulus( i ) ? GetStimulus( i )->GetParameter( j ) : 0 );
   }

   VOID GetStimParameter( int i, int j, CString& str ) const
   {
      str.Empty();
      if( IsValidStimulus( i ) ) GetStimulus( i )->GetParameter( j, str );
   }

   int GetStimParameterAsInt( int i, int j ) const 
   {
      return( IsValidStimulus( i ) ? GetStimulus( i )->GetParameterAsInt( j ) : 0 );
   }

   VOID GetStimParameterLabel( int i, int j, CString& str ) const
   {
      str.Empty();
      if( IsValidStimulus( i ) ) GetStimulus( i )->GetParameterLabel( j, str );
   }
   
   VOID GetStimParameterFormat( int i, int j, BOOL& bIsChoice, CStringArray& choices, NUMEDITFMT& fmt ) const
   {
      bIsChoice = TRUE;
      choices.RemoveAll();
      if( IsValidStimulus( i ) ) GetStimulus( i )->GetParameterFormat( j, bIsChoice, choices, fmt );
   }

   BOOL IsStimParameterMultiChoice( int i, int j ) const
   {
      return( IsValidStimulus( i ) ? GetStimulus( i )->IsParameterMultiChoice( j ) : FALSE );
   }

   BOOL SetStimParameter( int i, int j, double dVal );
   BOOL SetStimParameter( int i, int j, int iVal ) { return( SetStimParameter( i, j, double(iVal) ) ); }


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump the object definition 
   void AssertValid() const;                             // validate internal consistency of the object 
#endif


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   CCxStimulus* RetrieveStimulus( int i ) const          // retrieve ptr to a stimulus channel object
   {
      ASSERT( IsValidStimulus( i ) );
      return( m_Stimuli.GetAt( m_Stimuli.FindIndex( i ) ) );
   }

   VOID SetDefaults();                                   // init run's general parameters to default values 
};


#endif   // !defined(CXCONTRUN_H__INCLUDED_)
