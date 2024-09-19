//===================================================================================================================== 
//
// cxperthelper.cpp : Implementation of helper class CCxPertHelper, which processes and updates velocity perturbations 
//                    of targets during a trial.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// MAESTRODRIVER supports the perturbation of trial target trajectories via the application of one of several kinds 
// of perturbation waveforms. MAESTRO communicates the definition of each perturbation to MAESTRODRIVER via the trial 
// code group TARGET_PERTURB (described in CXTRIALCODES.H). MAESTRODRIVER must appropriately perturb the affected 
// target's trajectory as prescribed by the perturbation's definition. Any given perturbation waveform can be applied 
// to one of 8 different components of a trial target's trajectory: horizontal or vertical window velocity, H or V 
// pattern velocity, the direction or amplitude ("speed") of the window velocity vector, and the direction or speed of
// the pattern velocity vector (vector direction was added as of v1.3.2, while vector speed was added in v2.1.2). Up 
// to MAX_TRIALPERTS perturbations can be in use during any single trial.
// 
// CCxPertHelper is a helper class for CCxDriver. It encapsulates the details of processing the TARGET_PERTURB trial 
// code group and calculating the contributions of any defined perturbations on a tick-by-tick basis. 
//
// ==> Usage:
// 1) Construct CCxPertHelper object before reading the trial's codes.  Or call Reset() on an existing helper obj.
// 2) Each time a TARGET_PERTURB trial code set is encountered, pass the set of codes to ProcessTrialCodes().  This 
// will create and initialize a "perturbation object" representing the perturbation defined by the trial code set.
// 3) During trial trajectory precomputation, invoke Perturb() on each target, passing its current window and pattern 
// velocities.  Perturb() will calculate any deltas to these "nominal" velocities if the specified target is affected 
// by any of the perturbation objects currently defined.  Perturbations are applied in the order they were defined by 
// the previous calls to ProcessTrialCodes(). The caller should then add the "deltas" to the nominal velocities to 
// "perturb" them.
// 4) While it is possible to apply more than one perturbation to a target at the same time, applying a directional 
// and a velocity or speed perturbation simultaneously is NOT advised. A directional perturbation does not add linearly 
// to a previously applied velocity perturbation; its effect is to rotate the velocity vector.
//
//
// REVISION HISTORY:
// 18dec2002-- Began development.
// 29jul2005-- Added support for a new perturbation type, PERT_ISGAUSS, representing Gaussian-distributed noise.
//          -- Added support for a 'seed' parameter for both PERT_ISNOISE and PERT_ISGAUSS.
//          -- Added support for perturbing target window or pattern direction (PERT_ON_DWIN, PERT_ON_DPAT).
//          -- Made all necessary modifications to ensure that target trajectories affected by any perturbation can be 
//             reproduced offline based on an examination of the trial codes.
// 16jul2007-- Added support for perturbing target window or pattern speed (PERT_ON_SWIN, PERT_ON_SPAT) while keeping 
//             the direction of motion constant. Effective Maestro v2.1.2.
// 29aug2007-- Added support for simultaneously and identically perturbing target window and pattern direction 
//             (PERT_ON_DIR), or target and pattern speed (PERT_ON_SPD). Effective Maestro v2.1.3.
//===================================================================================================================== 

#include <windows.h>                   // standard Win32 includes
#include "rtapi.h"                     // the RTX API
#include "math.h"                      // for sin()

#include "cxperthelper.h" 


//===================================================================================================================== 
// CONSTANTS INITIALIZED
//===================================================================================================================== 



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxPertHelper [constructor] ===================================================================================== 
//
//    Constructed with no perturbations in effect.
//
CCxPertHelper::CCxPertHelper()
{
   for( int i=0; i<MAX_TRIALPERTS; i++ )
   {
      m_perts[i].pRandomNG = NULL;
   }
   m_nPerts = 0;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== Reset =========================================================================================================== 
//
//    Remove all currently defined perturbations.  Release any random number generators that were created to implement 
//    noise perturbations.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxPertHelper::Reset()
{
   m_nPerts = 0;
   for( int i=0; i<MAX_TRIALPERTS; i++ )
   {
      if( m_perts[i].pRandomNG != NULL )
      {
         delete m_perts[i].pRandomNG;
         m_perts[i].pRandomNG = NULL;
      }
   }
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
BOOL RTFCNDCL CCxPertHelper::ProcessTrialCodes( TRIALCODE* pCodes )
{
   if( m_nPerts == MAX_TRIALPERTS || pCodes[0].code != TARGET_PERTURB ) return( FALSE );

   CPertObj* pNew = &(m_perts[m_nPerts]);
   pNew->iTgt = int(pCodes[1].code);
   pNew->idCmpt = int(pCodes[1].time >> 4);
   pNew->iStart = int(pCodes[0].time);
   pNew->fAmp = float(pCodes[2].code) / 10.0f;
   pNew->def.iType = int(pCodes[1].time & 0x0F);
   pNew->def.iDur = int(pCodes[2].time);

   
   switch( pNew->def.iType )                                      // translate type-specific perturbation parameters
   {
      case PERT_ISSINE :
         pNew->def.sine.iPeriod = int(pCodes[3].code);
         pNew->def.sine.fPhase = float(pCodes[3].time)/100.0f;
         break;
      case PERT_ISTRAIN :
         pNew->def.train.iPulseDur = int(pCodes[3].code);
         pNew->def.train.iRampDur = int(pCodes[3].time);
         pNew->def.train.iIntv = int(pCodes[4].code);
         break;
      case PERT_ISNOISE :
      case PERT_ISGAUSS :
         pNew->def.noise.iUpdIntv = int(pCodes[3].code);
         pNew->def.noise.fMean = float(pCodes[3].time)/1000.0f;
         pNew->def.noise.iSeed = (int) MAKELONG(pCodes[4].time, pCodes[4].code);
         if( pNew->pRandomNG != NULL )
         {
            delete pNew->pRandomNG;
            pNew->pRandomNG = NULL;
         }
         if( pNew->def.iType == PERT_ISNOISE )
            pNew->pRandomNG = new CUniformRNG;
         else
            pNew->pRandomNG = new CGaussRNG;
         if( pNew->pRandomNG == NULL )
            return( FALSE );
         pNew->pRandomNG->SetSeed( pNew->def.noise.iSeed );
         pNew->dLastRandom = 0.0;
         break;
      default :
         return( FALSE );
   }

   ++m_nPerts;
   return( TRUE );
}


//=== Perturb ========================================================================================================= 
//
//    Calculate the offset vectors (DH, DV) that represent the net effect of any perturbations applied to the nominal 
//    window and pattern velocities of the specified target.  If none of the currently defined perturbations affect 
//    the specified target at the current time, then both offset vectors will be (0,0).
//
//    IMPORTANT:  By design, the two directional perturbations (PERT_ON_DWIN, PERT_ON_DPAT) rotate the *nominal* 
//    velocity vector by some random angle.  The offset vector returned gives the horizontal and vertical deltas 
//    necessary to achieve this rotation.  Applying a directional and a velocity component or speed perturbation at 
//    the same time would be rather confusing!
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
VOID RTFCNDCL CCxPertHelper::Perturb( int iTgt, int iTime, const CFPoint& fpWin, const CFPoint& fpPat, 
   CFPoint& fpPertWin, CFPoint& fpPertPat )
{
   fpPertWin.Zero();                                              // make sure net perturbations start at (0,0)!
   fpPertPat.Zero();

   for( int i = 0; i < m_nPerts; i++ )                            // for each active perturbation...
   {
      CPertObj* pPert = &(m_perts[i]);
      if( iTgt == pPert->iTgt )                                   // ...if pert affects this tgt, 
      { 
         double dCurr = Compute( iTime, pPert );                  //    compute the current value of the velocity 
         if( dCurr == 0.0 ) continue;                             //    or directional pert.  if 0, no effect.

         CFPoint fpRotated;                                       //    for directional perts

         switch( pPert->idCmpt )                                  //    if perturbed, update the appropriate offset  
         {                                                        //    vector IAW what trajectory cmpt is perturbed 
            case PERT_ON_HWIN : 
               fpPertWin.OffsetH( dCurr ); 
               break;
            case PERT_ON_VWIN : 
               fpPertWin.OffsetV( dCurr ); 
               break;
            case PERT_ON_HPAT : 
               fpPertPat.OffsetH( dCurr ); 
               break;
            case PERT_ON_VPAT : 
               fpPertPat.OffsetV( dCurr ); 
               break;
            case PERT_ON_DWIN : 
               fpRotated.SetPolar( (double) fpWin.GetR(), (double) fpWin.GetTheta() + dCurr );
               fpPertWin.Offset( fpRotated.GetH() - fpWin.GetH(), fpRotated.GetV() - fpWin.GetV() );
               break;
            case PERT_ON_DPAT : 
               fpRotated.SetPolar( (double) fpPat.GetR(), (double) fpPat.GetTheta() + dCurr );
               fpPertPat.Offset( fpRotated.GetH() - fpPat.GetH(), fpRotated.GetV() - fpPat.GetV() );
               break;
            case PERT_ON_SWIN :
               fpRotated.SetPolar( (double) fpWin.GetR() + dCurr, (double) fpWin.GetTheta() );
               fpPertWin.Offset( fpRotated.GetH() - fpWin.GetH(), fpRotated.GetV() - fpWin.GetV() );
               break;
            case PERT_ON_SPAT :
               fpRotated.SetPolar( (double) fpPat.GetR() + dCurr, (double) fpPat.GetTheta() );
               fpPertPat.Offset( fpRotated.GetH() - fpPat.GetH(), fpRotated.GetV() - fpPat.GetV() );
               break;
            case PERT_ON_DIR : 
               fpRotated.SetPolar( (double) fpWin.GetR(), (double) fpWin.GetTheta() + dCurr );
               fpPertWin.Offset( fpRotated.GetH() - fpWin.GetH(), fpRotated.GetV() - fpWin.GetV() );
               fpRotated.SetPolar( (double) fpPat.GetR(), (double) fpPat.GetTheta() + dCurr );
               fpPertPat.Offset( fpRotated.GetH() - fpPat.GetH(), fpRotated.GetV() - fpPat.GetV() );
               break;
            case PERT_ON_SPD :
               fpRotated.SetPolar( (double) fpWin.GetR() + dCurr, (double) fpWin.GetTheta() );
               fpPertWin.Offset( fpRotated.GetH() - fpWin.GetH(), fpRotated.GetV() - fpWin.GetV() );
               fpRotated.SetPolar( (double) fpPat.GetR() + dCurr, (double) fpPat.GetTheta() );
               fpPertPat.Offset( fpRotated.GetH() - fpPat.GetH(), fpRotated.GetV() - fpPat.GetV() );
               break;
         }
      }
   }
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== Compute ========================================================================================================= 
//
//    Compute value of specified perturbation waveform for the specified trial time.
//
//    ARGS:       iTime -- [in] trial time in ms.
//                pPert -- [in] the perturbation object 
//
//    RETURNS:    perturbation value (either velocity in deg/s or a directional offset in deg)
//
double RTFCNDCL CCxPertHelper::Compute( int iTime, CPertObj* pPert )
{
   if( iTime < pPert->iStart ||                                   // perturbation off
       iTime >= (pPert->iStart + pPert->def.iDur) )
      return( 0.0 );

   int t = iTime - pPert->iStart;                                 // time since perturbation started
   double dVal = 0.0;                                             // computed value of perturbation waveform

   if( pPert->def.iType == PERT_ISSINE )                          // SINE: v(t) = A*sin(2PI*t/T + phi), where A = amp
   {                                                              // in deg/s, T = period in ms, phi = phase in deg.
      double dAmp = double(pPert->fAmp);                          // NOTE conversion of ms --> sec.
      double dOmega = cMath::TWOPI * 1000.0 / double(pPert->def.sine.iPeriod);
      double dOmega_t = dOmega * double(t) / 1000.0;
      double dRad = dOmega_t + cMath::toRadians( double(pPert->def.sine.fPhase) );
      while( dRad >= cMath::TWOPI ) dRad -= cMath::TWOPI;

      dVal = dAmp * ::sin( dRad );
   }
   else if( pPert->def.iType == PERT_ISTRAIN )                    // TRAIN: Let D= pulse dur(ms), I= intv (ms), 
   {                                                              // R= ramp dur(ms), and A = pulse amp(deg/s).
      t = t % pPert->def.train.iIntv;                             //    t' = time within a pulse presentation
      int t1 = pPert->def.train.iRampDur;                         //    end of acceleration phase
      int t2 = t1 + pPert->def.train.iPulseDur;                   //    end of constant-velocity phase
      int t3 = t2 + pPert->def.train.iRampDur;                    //    end of deceleration phase

      double dSlope = double(pPert->fAmp) * 1000.0;               //    ramp "slope" = A/(R/1000) in deg/sec^2 
      dSlope /= double(pPert->def.train.iRampDur);

      double dTime = double(t) / 1000.0;                          //    t' converted from ms --> sec

      if( t >= 0 && t < t1 ) dVal = dSlope * dTime;               //    for t' in [0..R), v(t') = slope * t'.
      else if( t >= t1 && t < t2 ) dVal = double(pPert->fAmp);    //    for t' in [R..R+D), v(t') = A.
      else if( t >= t2 && t < t3 )                                //    for t' in [R+D..2R+D), 
         dVal = dSlope * (double(t3)/1000.0 - dTime);             //       v(t') = slope * (2R+D-t' in sec)
   }
   else if( pPert->def.iType == PERT_ISNOISE )                    // Uniform NOISE: Steplike waveform changes once per 
   {                                                              // update interval.  
      if( t % pPert->def.noise.iUpdIntv == 0 ) 
      {
         dVal = 2.0 * pPert->pRandomNG->Generate() - 1.0;         //    2*U(0..1) - 1 ==> U(-1..1)
         dVal += double(pPert->def.noise.fMean);                  //    U(-1_mean .. 1+mean)
         dVal *= double(pPert->fAmp);                             //    U(-1+mean .. 1+mean)*amplitude
         pPert->dLastRandom = dVal;                               //    remember value until next update
      }
      else
         dVal = pPert->dLastRandom;
   }
   else if( pPert->def.iType == PERT_ISGAUSS )                    // Gaussian NOISE: Steplike waveform changes once per 
   {                                                              // update interval.  
      if( t % pPert->def.noise.iUpdIntv == 0 ) 
      {
         dVal = pPert->pRandomNG->Generate();                     //    N(0,1) [Gaussian w/zero mean and 1 std dev]
         dVal *= double(pPert->fAmp);                             //    N(0,amplitude)
         dVal += double(pPert->def.noise.fMean * pPert->fAmp);    //    N(mean*amplitude, amplitude)
         pPert->dLastRandom = dVal;                               //    remember value until next update
      }
      else
         dVal = pPert->dLastRandom;
   }

   return( dVal );
}

