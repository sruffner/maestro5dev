//===================================================================================================================== 
//
// cxperthelper.h : Declaration of CCxPertHelper.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(CXPERTHELPER_H__INCLUDED_)
#define CXPERTHELPER_H__INCLUDED_

#include "cxobj_ifc.h"                 // common CNTRLX/CXDRIVER object definitions
#include "cxtrialcodes.h"              // CNTRLX/CXDRIVER trial codes
#include "util.h"                      // for CFPoint, CUniformRNG, and CGaussRNG utility classes
                                       

//===================================================================================================================== 
// Declaration of class CCxPertHelper
//===================================================================================================================== 
//
class CCxPertHelper
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   struct CPertObj                     // encapsulation of a perturbation object:
   {
      int   iTgt;                      //   index (in trial target map) of the affected target
      int   idCmpt;                    //   id of affected trajectory component (one of PERT_ON_** constants)
      int   iStart;                    //   start time (during trial) in ms
      float fAmp;                      //   perturbation amplitude
      PERT  def;                       //   parameters defining the unit-amplitude perturbation

      CRandomNG* pRandomNG;            //   private random# generator for a uniform or Gaussian noise perturbation
      double dLastRandom;              //   last random# generated -- since noise perts only update once in a while
   };

   int      m_nPerts;                  // the list of perturbations currently in effect
   CPertObj m_perts[MAX_TRIALPERTS]; 

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxPertHelper( const CCxPertHelper& src );            // no copy constructor or assignment operator defined
   CCxPertHelper& operator=( const CCxPertHelper& src ); 

public: 
   CCxPertHelper();
   ~CCxPertHelper() { Reset(); } 

//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   VOID RTFCNDCL Reset();                                // remove all currently defined perturbations
   BOOL RTFCNDCL ProcessTrialCodes( TRIALCODE* pCodes ); // add perturbation defined by TARGET_PERTURB trial code set 
   VOID RTFCNDCL Perturb( int iTgt, int iTime,           // perturb a tgt IAW currently defined perturbations
      const CFPoint& fpWin, const CFPoint& fpPat,
      CFPoint& fpPertWin, CFPoint& fpPertPat );

//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   double RTFCNDCL Compute(int iTime, CPertObj* pPert);  // compute perturbation velocity for specified time
};


#endif   // !defined(CXPERTHELPER_H__INCLUDED_)



