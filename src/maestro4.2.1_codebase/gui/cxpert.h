//===================================================================================================================== 
//
// cxpert.h : Declaration of class CCxPert.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXPERT_H__INCLUDED_)
#define CXPERT_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "treemap.h"                         // the CTreeMap/CTreeObj framework
#include "cxobj_ifc.h"                       // CNTRLX object "interface" defines 

#include "numedit.h"                         // for the NUMEDITFMT declaration
#include "util.h"                            // for CRand16

//===================================================================================================================== 
// Declaration of class CCxPert
//===================================================================================================================== 
//
class CCxPert : public CTreeObj
{
   DECLARE_SERIAL( CCxPert )

   friend class CCxTreeMap;                 

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int NCOMMON = 2 ;                        // # of common parameters applicable to all perturbation types 
   static const int NPARAMS[PERT_NTYPES];                // # of type-specific parameters for each perturbation type
   static LPCTSTR TYPESTRINGS[PERT_NTYPES];              // human-readable names for supported perturbation types
   static LPCTSTR COMMONLBLS[NCOMMON];                   // human-readable labels for the common perturbation params


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   static CRand16 m_seedRNG;                             // random# generator used to supply random seeds for the 
                                                         // noise perts on an as-needed basis

   int         m_iType;                                  // type of perturbation
   int         m_iDur;                                   // duration of perturbation in ms

   SINEPERT    m_sine;                                   // type-specific defining parameter sets -- maintaining copies 
   TRAINPERT   m_train;                                  // of all sets allows user to freely change type w/o losing 
   NOISEPERT   m_noise;                                  // info.  however, only the relevant set is serialized!


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxPert( const CCxPert& src );                        // no copy constructor defined
   CCxPert& operator=( const CCxPert& src );             // no assignment operator defined 

protected:
   CCxPert() { SetDefaults(); }                          // constructor required for dyn object creation mechanism 
   ~CCxPert() {}                                         // destroy obj -- freeing any allocated memory

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f); // initialize object after default construction
   VOID Copy( const CTreeObj* pSrc );                    // make THIS object a copy of the specified object

public:
   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a perturbation from a different treemap 
         const CWordToWordMap& depKeyMap);


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   BOOL CanRemove() const                                // prevent removal of "predefined" objects
   { return( (m_flags & CX_ISPREDEF) == 0 ); } 

   static int NumberOfCommonParameters()                 // first N parameters are common to all perturbation types
   {
      return( CCxPert::NCOMMON );
   }
   static int MaxNumberOfParameters()                    // worst-case total# of parameters defining a perturbation
   {
      return( CCxPert::NCOMMON + NPARAMS[PERT_ISTRAIN] );
   }
   static LPCTSTR GetCommonParamLabel( int i )           // retrieve label for a common parameter
   {
      return( (i>=0 && i < NCOMMON)  ? COMMONLBLS[i] : NULL );
   }
   
   int NumberOfUniqueParameters() const                  // size of type-specific param list (not incl common parms) 
   {
      return( NPARAMS[GetType()] );
   }
   int NumberOfParameters() const                        // total # of parameters defining perturbation
   {
      return( CCxPert::NCOMMON + NumberOfUniqueParameters() );
   }
   BOOL IsValidParameter( int i ) const                  // is this a valid parameter index?
   {
      return( (i >= 0) && (i < NumberOfParameters()) );
   }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public: 
   static int GetRandomSeed();                           // get a nonzero randomized seed value to seed the RNG for 
                                                         // one of the noise perturbations

   VOID GetPertInfo( PERT& pert ) const;                 // retrieve perturbation defn in CXDRIVER-compatible format
   VOID SetPertInfo( const PERT& pert );                 // set perturbation defn as a unit, w/ auto-correct 
   void Serialize( CArchive& ar );                       // for reading/writing object from/to disk file
   BOOL Import(CStringArray& strArDefn,CString& strMsg); // set pert obj IAW cntrlxUNIX-style, text-based definition

   int GetType() const { return( m_iType ); }            // access to common parameters by name...
   int GetDuration() const { return( m_iDur ); }

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
   void Dump( CDumpContext& dc ) const;                  // dump the object's name, data type, and parameters (if any) 
   void AssertValid() const;                             // validate the object 
#endif


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   VOID SetDefaults();                                   // initialize perturbation parameters to default state
   VOID Validate();                                      // validate perturbation defn in its current state
};

#endif   // !defined(CXPERT_H__INCLUDED_)
