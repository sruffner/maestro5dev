//===================================================================================================================== 
//
// pertmgr.h : Constants and other declarations for PERTMGR.C
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(PERTMGR_H__INCLUDED_)
#define PERTMGR_H__INCLUDED_

#include "wintypes.h"                     // some typical Windows typedefs that we need 
#include "cxobj_ifc_mex.h"                // common Maestro/CXDRIVER object definitions
#include "cxtrialcodes_mex.h"             // Maestro/CXDRIVER trial codes

                                       
//===================================================================================================================== 
// UNIFORMRNG:  Constants and state information for a pseudo-random number generator that returns a sequence of 
// uniformly distributed floating-point values in (0.0 .. 1.0), endpoints excluded.
//===================================================================================================================== 
#define   URNG_TABLESZ      32            // size of shuffle table
#define   URNG_M            2147483647    // parameters of interal linear congruential generator I' = A*I % M, using 
#define   URNG_A            16807         // Schrage's method (Q,R) to avoid integer overflow
#define   URNG_Q            127773
#define   URNG_R            2836
#define   URNG_NDIV         (1 + (URNG_M - 1)/URNG_TABLESZ)
#define   URNG_DSCALE       (1.0/URNG_M)

typedef struct uniformRNGObj
{
   int shuffle[URNG_TABLESZ];             // the shuffle table
   int lastOut;                           // the last integer spit out of shuffle table
   int curr;                              // current value I of the linear congruential generator
} UNIFORMRNG, *PUNIFORMRNG;

 
//===================================================================================================================== 
// GAUSSRNG:  State information for a pseudo-random number generator that returns a sequence of normally distributed
// floating-point values with zero mean and unit variance.
//===================================================================================================================== 

typedef struct gaussianRNGObj
{
   UNIFORMRNG uniformRNG;                 // uniform random-number generator from which gaussian distrib seq derived
   BOOL bGotNext;                         // since algorithm generates two numbers at a time, we only have to process
   double dNext;                          // the algorithm on every other call to Generate()
} GAUSSRNG, *PGAUSSRNG;


//===================================================================================================================== 
// MPERTMGR:  All information needed to process any and all perturbations presented during a single Maestro trial
//===================================================================================================================== 
//
typedef struct maestroPertObj          // encapsulation of a single perturbation waveform:
{
   int iTgt;                           //   index (in trial target map) of the affected target
   int idCmpt;                         //   id of affected trajectory component (one of PERT_ON_** constants)
   int iStart;                         //   start time (during trial) in ms
   float fAmp;                         //   perturbation amplitude
   PERT def;                           //   parameters defining the unit-amplitude perturbation

   UNIFORMRNG uniformRNG;              //   private random# generator for a uniform noise pert (PERT_ISNOISE)
   GAUSSRNG gaussRNG;                  //   private random# generator for a gaussian noise pert (PERT_ISGAUSS)
   double dLastRandom;                 //   last random# generated -- since noise perts only update once in a while
} MPERTOBJ, *PMPERTOBJ;

typedef struct maestroPertMgr
{
   int      nPerts;                    // the list of perturbations currently in effect
   MPERTOBJ perts[MAX_TRIALPERTS]; 
} MPERTMGR, *PMPERTMGR;




//===================================================================================================================== 
// "Public" functions defined in this module
//===================================================================================================================== 
VOID seedUniformRNG( PUNIFORMRNG pUnif, int seed );
double getUniformRNG( PUNIFORMRNG pUnif );

VOID seedGaussRNG( PGAUSSRNG pGauss, int seed );
double getGaussRNG( PGAUSSRNG pGauss );

VOID resetPertManager( PMPERTMGR pPertMgr );
BOOL processPertCodes( PMPERTMGR pPertMgr, TRIALCODE* pCodes );
VOID perturbTarget( PMPERTMGR pPertMgr, int iTgt, int iTime, 
   double winVelH, double winVelV, double patVelH, double patVelV, 
   double* pWinOffsetH, double* pWinOffsetV, double* pPatOffsetH, double* pPatOffsetV );


#endif   // !defined(PERTMGR_H__INCLUDED_)



