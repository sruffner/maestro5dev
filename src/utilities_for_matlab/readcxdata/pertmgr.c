//=====================================================================================================================
//
// pertmgr.c : READCXDATA helper functions -- reproduction of Maestro perturbation waveforms modulating a trial target
// trajectory.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Maestro/MaestroDRIVER supports the perturbation of trial target trajectories via the application of one of several 
// kinds of perturbation waveforms, defined by the TARGET_PERTURB trial code group. This module encapsulates the 
// details of processing the TARGET_PERTURB trial codes and calculating the contributions of any defined perturbations
// on a tick-by-tick basis. It essentially reproduces the code from the class CCxPertHelper, which is part of 
// MaestroDRIVER's code base. It also includes the uniform and Gaussian random number generators that implement the 
// PERT_ISNOISE and PERT_ISGAUSS perturbation types.
//
// REVISION HISTORY:
// 29jul2005-- Began development.
// 16jul2007-- Added support for perturbing target window or pattern speed (PERT_ON_SWIN, PERT_ON_SPAT), which was 
//             introduced in Maestro 2.1.2.
// 06sep2007-- Added support for perturing target window AND pattern speed at the same time (PERT_ON_SPD), or target 
//             window and pattern direction at the same time (PERT_ON DIR). These were introduced in Maestro 2.1.3.
//=====================================================================================================================

#include "math.h"                      // for various math/trig functions

#ifndef M_PI                           // this is defined on UNIX/Linux/Mac, but not Windows
   #define M_PI 3.14159265359
#endif

#include "pertmgr.h"

#define TORADIANS(D) (((double)D) * M_PI / 180.0)


//=====================================================================================================================
// MODULE-PRIVATE FUNCTION PROTOTYPES
//=====================================================================================================================
double computePert( int iTime, PMPERTOBJ pPert );


//=====================================================================================================================
// FUNCTIONS FOR RANDOM NUMBER GENERATION
//
// These functions and the corresponding state information essentially reproduce the functionality of Maestro CXDRIVER
// classes CUniformRNG and CGaussRNG defined in cxdriver/utilities.cpp.  The header comments for those two classes are
// copied below...
//
// Implementation of class CUniformRNG
// -----------------------------------
// This class encapsulates a pseudo-random number generator that returns a sequence of uniformly distributed floating-
// point values in (0.0 .. 1.0), endpoints excluded.  It encapsulates the "ran1" algorithm presented on p.282 in:
// Press, WH; et al.  "Numerical recipes in C: the art of acientific computing".  New York:  Cambridge University
// Press, Copyright 1988-1992.
//
// The algorithm uses a 32-entry table to shuffle the output of a "Minimal Standard" linear congruential generator, of
// the form I(n+1) = A*I(n) % M (with A and M carefully chosen).  Schrage's method is used to compute I(n+1) without
// an integer overflow.  The 32-bit integers output by the algorithm fall in the range [1..M-1]; dividing by M=2^31
// gives a double-valued output in (0..1).  For algorithmic details, consult the book.  There are few comments here.
//
// Portability note:  We assume here that int is 32-bit!!
//
// IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form without
// obtaining the appropriate license; however, it may appear in an executable file that is distributed.
//
// Implementation of class CGaussRNG
// ---------------------------------
// This class encapsulates a pseudo-random number generator that returns a sequence of normally distributed floating-
// point values with zero mean and unit variance.  It encapsulates the "gasdev" algorithm presented on p.289 in:
// Press, WH; et al.  "Numerical recipes in C: the art of acientific computing".  New York:  Cambridge University
// Press, Copyright 1988-1992.
//
// The algorithm uses the polar form of the Box-Muller transformation to transform a sequence of uniform deviates to
// a sequence of Gaussian deviates.  We use CUniformRNG as the source of the uniform sequence.  For algorithmic
// details, consult the book.
//
// IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form without
// obtaining the appropriate license; however, it may appear in an executable file that is distributed.
//=====================================================================================================================

VOID seedUniformRNG( PUNIFORMRNG pUnif, int seed )
{
   int j, k;

   pUnif->curr = (seed == 0) ? 1 : ((seed < 0) ? -seed : seed);      // start at strictly positive seed value

   for( j = URNG_TABLESZ+7; j >= 0; j-- )                            // after discarding first 8 integers generated
   {                                                                 // by the algorithm, fill shuffle table with
      k = pUnif->curr/URNG_Q;                                        // next TABLESZ integers generated
      pUnif->curr = URNG_A * (pUnif->curr - k*URNG_Q) - k*URNG_R;
      if( pUnif->curr < 0 ) pUnif->curr += URNG_M;
      if( j < URNG_TABLESZ ) pUnif->shuffle[j] = pUnif->curr;
   }

   pUnif->lastOut = pUnif->shuffle[0];
}

double getUniformRNG( PUNIFORMRNG pUnif )
{
   int k, index;

   k = pUnif->curr/URNG_Q;                                     // compute I(n+1) = A*I(n) % M using Schrage's method
   pUnif->curr = URNG_A * (pUnif->curr - k*URNG_Q) - k*URNG_R; // to avoid integer overflows
   if( pUnif->curr < 0 ) pUnif->curr += URNG_M;

   index = pUnif->lastOut / URNG_NDIV;                         // use last # retrieved from shuffle table to calc
   pUnif->lastOut = pUnif->shuffle[index];                     // index of next # to retrieve. Replace that entry in
   pUnif->shuffle[index] = pUnif->curr;                        // shuffle table with curr output of LC generator

   return( URNG_DSCALE * pUnif->lastOut );                     // convert int in [1..M-1] to double in (0..1)
}

VOID seedGaussRNG( PGAUSSRNG pGauss, int seed )
{
   seedUniformRNG( &(pGauss->uniformRNG), seed );
   pGauss->bGotNext = FALSE;
}

double getGaussRNG( PGAUSSRNG pGauss )
{
   double dVal = 0;
   double v1, v2, rsq, fac;

   if( !pGauss->bGotNext )
   {
      do                                                          // get two uniform deviates v1,v2 such that (v1,v2)
      {                                                           // lies strictly inside the unit circle, but not at
         v1 = 2.0 * getUniformRNG( &(pGauss->uniformRNG) ) - 1.0; // the origin
         v2 = 2.0 * getUniformRNG( &(pGauss->uniformRNG) ) - 1.0;
         rsq = v1*v1 + v2*v2;
      } while( rsq >= 1.0 || rsq == 0.0 );

      fac = sqrt( -2.0 * log(rsq) / rsq );                        // use Box-Muller transformation to transform the
      pGauss->dNext = v1*fac;                                     // the uniform deviates to two Gaussian deviates, one
      pGauss->bGotNext = TRUE;                                    // of which is saved for next call to this function!
      dVal = v2*fac;
   }
   else
   {
      dVal = pGauss->dNext;
      pGauss->bGotNext = FALSE;
   }

   return( dVal );
}


//=====================================================================================================================
// FUNCTIONS FOR THE PERTURBATION MANAGER
//
// These functions duplicate similarly named functions in the MaestroDRIVER class CCxPertHelper.  Each function has
// a header **copied from the CCxPertHelper source file**.  You can think of the PERTMGR pointer argument in these
// methods as roughly equivalent to the hidden 'this' pointer in C++.
//=====================================================================================================================

//=== Reset ===========================================================================================================
//
//    Remove all currently defined perturbations.  Release any random number generators that were created to implement
//    noise perturbations.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID resetPertManager( PMPERTMGR pPertMgr )
{
   pPertMgr->nPerts = 0;
}

//=== ProcessTrialCodes ===============================================================================================
//
//    Translates a TARGET_PERTURB trial code set into a new perturbation object to be applied during the trial.  The
//    index of the affected target, the perturbation's start time within the trial, and the affected target velocity
//    component are all included in the trial code set, along with the parameters defining the perturbation itself.
//
//    ARGS:       pCodes -- [in] ptr to a set of five TRIALCODEs representing a TARGET_PERTURB code group.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL processPertCodes( PMPERTMGR pPertMgr, TRIALCODE* pCodes )
{
   PMPERTOBJ pNew;

   if( pPertMgr->nPerts == MAX_TRIALPERTS || pCodes[0].code != TARGET_PERTURB ) return( FALSE );

   pNew = &(pPertMgr->perts[ pPertMgr->nPerts ]);
   pNew->iTgt = (int) pCodes[1].code;
   pNew->idCmpt = (int) (pCodes[1].time >> 4);
   pNew->iStart = (int) pCodes[0].time;
   pNew->fAmp = ((float) pCodes[2].code) / 10.0f;
   pNew->def.iType = (int) (pCodes[1].time & 0x0F);
   pNew->def.iDur = (int) pCodes[2].time;

   switch( pNew->def.iType )                                      // translate type-specific perturbation parameters
   {
      case PERT_ISSINE :
         pNew->def.sine.iPeriod = (int) pCodes[3].code;
         pNew->def.sine.fPhase = ((float) pCodes[3].time)/100.0f;
         break;
      case PERT_ISTRAIN :
         pNew->def.train.iPulseDur = (int) pCodes[3].code;
         pNew->def.train.iRampDur = (int) pCodes[3].time;
         pNew->def.train.iIntv = (int) pCodes[4].code;
         break;
      case PERT_ISNOISE :
      case PERT_ISGAUSS :
         pNew->def.noise.iUpdIntv = (int) pCodes[3].code;
         pNew->def.noise.fMean = ((float) pCodes[3].time)/1000.0f;
         pNew->def.noise.iSeed = (int) MAKELONG(pCodes[4].time, pCodes[4].code);
         if( pNew->def.iType == PERT_ISNOISE )
            seedUniformRNG( &(pNew->uniformRNG), pNew->def.noise.iSeed );
         else
            seedGaussRNG( &(pNew->gaussRNG), pNew->def.noise.iSeed );
         pNew->dLastRandom = 0.0;
         break;
      default :
         return( FALSE );
   }

   ++(pPertMgr->nPerts);
   return( TRUE );
}


//=== Perturb =========================================================================================================
//
//    Calculate the offset vectors (DH, DV) that represent the net effect of any perturbations applied to the nominal
//    window and pattern velocities of the specified target.  If none of the currently defined perturbations affect
//    the specified target at the current time, then both offset vectors will be (0,0).
//
//    IMPORTANT:  By design, the two directional perturbations (PERT_ON_DWIN, PERT_ON_DPAT) rotate the *nominal*
//    velocity vector by some random angle. The offset vector returned gives the horizontal and vertical deltas
//    necessary to achieve this rotation. Applying a directional and a velocity component or speed perturbation at the 
//    same time would be rather confusing!
//
//    ARGS:       iTgt        -- [in] index of target in the trial target map.
//                iTime       -- [in] current trial time in ms.
//                fpWin       -- [in] current "nominal" window velocity of tgt
//                fpPat       -- [in] current "nominal" pattern velocity of tgt
//                fpPertWin   -- [out] net offset in "nominal" window velocity due to any defined perturbations
//                fpPertPat   -- [out] net offset in "nominal" pattern velocity due to any defined perturbations
//
//    RETURNS:    NONE.
//
VOID perturbTarget( PMPERTMGR pPertMgr, int iTgt, int iTime, double winVelH, double winVelV, double patVelH,
   double patVelV, double* pWinOffsetH, double* pWinOffsetV, double* pPatOffsetH, double* pPatOffsetV )
{
   int i;
   double dCurr;
   double dR, dTheta;                                             // for handling directional perts
   PMPERTOBJ pPert;

   *pWinOffsetH = 0.0;                                            // make sure net perturbations start at (0,0)!
   *pWinOffsetV = 0.0;
   *pPatOffsetH = 0.0;
   *pPatOffsetV = 0.0;

   for( i = 0; i < pPertMgr->nPerts; i++ )                        // for each active perturbation...
   {
      pPert = &(pPertMgr->perts[i]);
      if( iTgt == pPert->iTgt )                                   // ...if pert affects this tgt,
      {
         dCurr = computePert( iTime, pPert );                     //    compute the current value of the velocity
         if( dCurr == 0.0 ) continue;                             //    or directional pert.  if 0, no effect.

         switch( pPert->idCmpt )                                  //    if perturbed, update the appropriate offset
         {                                                        //    vector IAW what trajectory cmpt is perturbed
            case PERT_ON_HWIN :
               *pWinOffsetH += dCurr;
               break;
            case PERT_ON_VWIN :
               *pWinOffsetV += dCurr;
               break;
            case PERT_ON_HPAT :
               *pPatOffsetH += dCurr;
               break;
            case PERT_ON_VPAT :
               *pPatOffsetV += dCurr;
               break;
            case PERT_ON_DWIN :                                   //   NOTE: the directional pert is a rotation in
               dR = sqrt( winVelH*winVelH + winVelV*winVelV );    //   in degrees, not radians!
               dTheta = atan2( winVelV, winVelH );
               dTheta += TORADIANS(dCurr);
               *pWinOffsetH += dR * cos(dTheta) - winVelH;
               *pWinOffsetV += dR * sin(dTheta) - winVelV;
               break;
            case PERT_ON_DPAT :
               dR = sqrt( patVelH * patVelH + patVelV * patVelV );
               dTheta = atan2( patVelV, patVelH );
               dTheta += TORADIANS(dCurr);
               *pPatOffsetH += dR * cos(dTheta) - patVelH;
               *pPatOffsetV += dR * sin(dTheta) - patVelV;
               break;
            case PERT_ON_SWIN :
               dR = sqrt( winVelH*winVelH + winVelV*winVelV ); 
               dR += dCurr;
               dTheta = atan2( winVelV, winVelH );
               *pWinOffsetH += dR * cos(dTheta) - winVelH;
               *pWinOffsetV += dR * sin(dTheta) - winVelV;
               break;
            case PERT_ON_SPAT :
               dR = sqrt( patVelH * patVelH + patVelV * patVelV );
               dR += dCurr;
               dTheta = atan2( patVelV, patVelH );
               *pPatOffsetH += dR * cos(dTheta) - patVelH;
               *pPatOffsetV += dR * sin(dTheta) - patVelV;
               break;
            case PERT_ON_DIR :
               dR = sqrt( winVelH*winVelH + winVelV*winVelV ); 
               dTheta = atan2( winVelV, winVelH );
               dTheta += TORADIANS(dCurr);
               *pWinOffsetH += dR * cos(dTheta) - winVelH;
               *pWinOffsetV += dR * sin(dTheta) - winVelV;
               dR = sqrt( patVelH * patVelH + patVelV * patVelV );
               dTheta = atan2( patVelV, patVelH );
               dTheta += TORADIANS(dCurr);
               *pPatOffsetH += dR * cos(dTheta) - patVelH;
               *pPatOffsetV += dR * sin(dTheta) - patVelV;
               break;
            case PERT_ON_SPD :
               dR = sqrt( winVelH*winVelH + winVelV*winVelV ); 
               dR += dCurr;
               dTheta = atan2( winVelV, winVelH );
               *pWinOffsetH += dR * cos(dTheta) - winVelH;
               *pWinOffsetV += dR * sin(dTheta) - winVelV;
               dR = sqrt( patVelH * patVelH + patVelV * patVelV );
               dR += dCurr;
               dTheta = atan2( patVelV, patVelH );
               *pPatOffsetH += dR * cos(dTheta) - patVelH;
               *pPatOffsetV += dR * sin(dTheta) - patVelV;
               break;
         }
      }
   }
}


//=== Compute =========================================================================================================
//
//    Compute value of specified perturbation waveform for the specified trial time.
//
//    ARGS:       iTime -- [in] trial time in ms.
//                pPert -- [in] the perturbation object
//
//    RETURNS:    perturbation value (either velocity in deg/s or a directional offset in deg)
//
double computePert( int iTime, PMPERTOBJ pPert )
{
   int t, t1, t2, t3;
   double dVal, dAmp, dTwoPi, dOmega_t, dRad, dSlope, dTime;

   if( iTime < pPert->iStart ||                                   // perturbation off
       iTime >= (pPert->iStart + pPert->def.iDur) )
      return( 0.0 );

   t = iTime - pPert->iStart;                                     // time since perturbation started
   dVal = 0.0;                                                    // computed value of perturbation waveform

   if( pPert->def.iType == PERT_ISSINE )                          // SINE: v(t) = A*sin(2PI*t/T + phi), where A = amp
   {                                                              // in deg/s, T = period in ms, phi = phase in deg.
      dAmp = (double) pPert->fAmp;
      dTwoPi = M_PI * 2.0;
      dOmega_t = dTwoPi * ((double) t) / ((double)pPert->def.sine.iPeriod);
      dRad = dOmega_t + TORADIANS( pPert->def.sine.fPhase );
      while( dRad >= dTwoPi ) dRad -= dTwoPi;

      dVal = dAmp * sin( dRad );
   }
   else if( pPert->def.iType == PERT_ISTRAIN )                    // TRAIN: Let D= pulse dur(ms), I= intv (ms),
   {                                                              // R= ramp dur(ms), and A = pulse amp(deg/s).
      t = t % pPert->def.train.iIntv;                             //    t' = time within a pulse presentation
      t1 = pPert->def.train.iRampDur;                             //    end of acceleration phase
      t2 = t1 + pPert->def.train.iPulseDur;                       //    end of constant-velocity phase
      t3 = t2 + pPert->def.train.iRampDur;                        //    end of deceleration phase

      dSlope = ((double)pPert->fAmp) * 1000.0;                    //    ramp "slope" = A/(R/1000) in deg/sec^2
      dSlope /= (double) pPert->def.train.iRampDur;

      dTime = ((double) t) / 1000.0;                              //    t' converted from ms --> sec

      if( t >= 0 && t < t1 ) dVal = dSlope * dTime;               //    for t' in [0..R), v(t') = slope * t'.
      else if( t >= t1 && t < t2 ) dVal = ((double) pPert->fAmp); //    for t' in [R..R+D), v(t') = A.
      else if( t >= t2 && t < t3 )                                //    for t' in [R+D..2R+D),
         dVal = dSlope * (((double)t3)/1000.0 - dTime);           //       v(t') = slope * (2R+D-t' in sec)
   }
   else if( pPert->def.iType == PERT_ISNOISE )                    // Uniform NOISE: Steplike waveform changes once per
   {                                                              // update interval.
      if( t % pPert->def.noise.iUpdIntv == 0 )
      {
         dVal = 2.0*getUniformRNG( &(pPert->uniformRNG) ) - 1.0;  //    2*U(0..1) - 1 ==> U(-1..1)
         dVal += (double) pPert->def.noise.fMean;                 //    U(-1_mean .. 1+mean)
         dVal *= (double) pPert->fAmp;                            //    U(-1+mean .. 1+mean)*amplitude
         pPert->dLastRandom = dVal;                               //    remember value until next update
      }
      else
         dVal = pPert->dLastRandom;
   }
   else if( pPert->def.iType == PERT_ISGAUSS )                    // Gaussian NOISE: Steplike waveform changes once per
   {                                                              // update interval.
      if( t % pPert->def.noise.iUpdIntv == 0 )
      {
         dVal = getGaussRNG( &(pPert->gaussRNG) );                //    N(0,1) [Gaussian w/zero mean and 1 std dev]
         dVal *= (double) pPert->fAmp;                            //    N(0,amplitude)
         dVal += (double) (pPert->def.noise.fMean * pPert->fAmp); //    N(mean*amplitude, amplitude)
         pPert->dLastRandom = dVal;                               //    remember value until next update
      }
      else
         dVal = pPert->dLastRandom;
   }

   return( dVal );
}

