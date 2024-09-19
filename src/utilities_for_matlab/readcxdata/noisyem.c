//=====================================================================================================================
//
// noisyem.c : Noisy dots target emulator -- reproduction of per-update, per-dot pixel displacements for the XYScope-
// and RMVideo-based "noisy dots" targets.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The XYScope "Noisy Dots (Direction)" and "Noisy Dots (Speed)" targets are unique in that the individual dots 
// constituting the random-dot patch do not move in the same way. Instead, a different random noise value is associated
// with each dot. This noise factor is randomly chosen for each dot after every U ms, U being the target's "noise update
// interval". The RMVideo "Random-Dot Patch" target can also be defined to introduce per-dot noise in either direction 
// or speed.
//
// This module contains code to emulate these noisy dots targets as closely as possible in order to reproduce the 
// instantaneous velocity for each target dot during each XYScope or RMVideo frame. For the XYScope targets, it emulates
// what goes on in XYCORE and CCxScope; for RMVideo, it emulates what happens in Maestro's CCxRMVideo and the RMVideo
// target class CRMVRandomDots. The XYScope and RMVideo implementations of the noisy dots targets are similar but 
// definitely different in the details!
//
// USAGE: This module is intended only for reproducing noisy-dots target behavior during the recorded portion of a
// Maestro trial.
// 1) Call initNoisyDotsEmulator() to initialize the emulator, passing information about the display platform. Note that
// the emulator does not support mixed use of XYScope and RMVideo targets. All are XYScope-based or all are on RMVideo.
// This is a sensible restriction, as experiment rigs are not capable of using both display platforms at once.

// 2) Call addNoisyDotsTarget() once for each noisy-dots target to be emulated during the animation sequence.

// 3) On every display frame update, call startNoisyDotsUpdate ONCE, then call updateNoisyDotsTarget() once per target 
// to emulate the motion of each dot in the target during that frame. The latter method stores, in internally allocated 
// buffers, the display frame timestamp in ms and the instantaneous velocity of each target dot during that frame. 
// Caller must supply the target's pattern displacement (horiz. and vert., in deg subtended at the eye) for that frame, 
// the current trial time, and other information.

// 4) Once the emulation has completed, call setNoisyDotsResults() to write the results to two fields of READCXDATA's
// output structure:
//    "xynoisytimes" is a 1xT vector holding the T times at which the target display was updated, in milliseconds since
// trial recording was turned on.
//    "xynoisydata" is Matlab array of structures, one for each noisy-dots target emulated. Each structure contains the
// following members:
//       "idx" : The target's ordinal position in the trial target list (0 for first target, and so on).
//       "type" : The dot-noise type (0 = direction noise; 1 = additive speed noise; 2 = multiplicative speed noise).
//       "level" : The noise level parameter, as specified in the original target definition.
//       "updIntv" : The noise update interval in ms, as specified in the original target definition.
//       "nDots" : The number of target dots, N; 
//       "dx dy" : Two NxT arrays holding the X and Y components for the "instantaneous" velocity of each target dot 
//          during each display frame during the recorded portion of the trial. The velocity is computed from the target
//          frame of reference, not the screen -- so you must compensate these if you want per-dot velocity WRT the
//          screen frame of reference. However, if the target is an RMVideo noisy dots target and its RMV_WRTSCREEN flag
//          set, then velocity is reported in the screen frame of reference. Each component is in deg/sec subtended at 
//          the subject's eye. The corresponding frame update times are found in the companion "xynoisytimes" vector.
//       "x y" : (RMVideo ONLY) Two NxT arrays holding the X and Y coordinates of the position of each target dot --
//          relative to the target's center (regardless the state of the RMV_WRTSCREEN flag) -- in deg subtended at the 
//          subject's eye. These arrays will be empty for XYScope targets, as we cannot keep track of absolute per-dot 
//          positions for the XYScope implementation. 
//    For RMVideo targets ONLY: If a dot is randomly repositioned during a frame update for whatever reason (left
// outer aperture, lifetime expired, incoherent motion), then its velocity for that frame is undefined (NaN). We can do
// this for RMVideo targets because each has its own private RNGs; for XYScope the RNGs are shared by all targets 
// presented during a trial, immensely complicating the emulation because all targets would have to be emulated 
// accurately.

// 5) Call releaseNoisyDotsEmulator() to release all memory allocated when the emulator was initialized.
//
// NOTES:
// (1) There are multiple errors in the implementation of the XYScope noisy-dots targets as of Maestro v2.6.5, and one 
// reason for developing this emulation was to be able to reproduce the erroneous implementation in order to evaluate 
// subject behavior and neural unit responses WRT what these noisy dots targets ACTUALLY did.
//
// (2) The problems with the XYScope noisy-dots targets were addressed in Maestro 2.7.0. This module, of course, can
// emulate the corrected implementation.
//
// (3) Does not emulate a mix of XYScope and RMVideo noisy-dots targets. This makes sense, since it is not possible to
// use the two video displays simultaneously.
//
// (3) For XYScope only: Absolute dot positions are not tracked. No attempt is made to simulate the effects of dots 
// being recycled when their dot life expires or when they cross out of the target window. The focus is entirely on 
// per-dot, per-frame pixel displacements. In addition, the XYScope target interleaving feature is NOT emulated.
//
// (4) IMPORTANT: This emulator only supports data files collected with Maestro v2.1.3 or later (file version >= 12).
// As of data file version 12, the implementation of the XYScope noisy-dots targets was in the form that remains
// applicable through Maestro v2.6.5; in addition, the RMVideo version of the noisy-dots target was in its current from
// as of Maestro 2.1.3.
//
// (5) IMPORTANT: If the system does not support NaN, then the velocity components of an RMVideo target dot that is
// randomly repositioned are set to 0 during that frame. This will affect the calculation of mean pattern velocity 
// across all dots during that frame.
//
// REVISION HISTORY:
// 11apr2011-- Began development.
// 17may2011-- Revised IAW implementation changes introduced in Maestro v2.7.0 (data file version = 18). These changes
// fixed the implementation errors discovered in Apr 2011. The module emulates either the old erroneous target 
// implementations or the current implementations, depending on the file version.
// 23may2011-- Bug fix: If a trial is saved even though it did not run to completion ("failsafe" segment feature), then
// trial code processing will run through the entire trial, including the portion that was not presented because the
// trial stopped prematurely. We failed to account for this possibility when incrementing nFrames, resulting in buffer
// overruns and, eventually, a seg fault. Fixed.
// 09sep2011-- ADDING SUPPORT for emulating the RMVideo version of the noisy-dots targets, ie, the "Random-Dot Patch"
// target with per-dot noise enabled. Renaming module files NOISYEM.*. EXTENSIVE CHANGES....
//    RMVideo implementation requires an implementation of its CUniformRNG class. Also, each RMVideo noisy-dots
// target has its own seed and its own RNG for per-dot noise. For the XYScope, there are just two RNGs, one for dot
// position and the other for per-dot noise, and they are seeded by the same initial seed value. The details of the
// RMVideo implementation are different. See rmvrandomdots.* in the RMVideo code base.
//    We will keep track of per-dot position for RMVideo targets because it's possible and we can therefore get
// a more accurate emulation. When a dot is randomly repositioned, its velocity is undefined (NaN)....
//    Declare the emulator object as a private global in this module, so we don't need to do so in readcxdata.c.
// Then adjust method invocations accordingly.
//    The relevant fields in the output structure are still called "xynoisytimes" and "xynoisy" to avoid breaking any
// existing user scripts...
// 30jan2012-- Completed coding changes to support RMVideo noisy-dots target emulation. Need to build and test...
// 11may2012-- Finally starting to integrate NOISYEM.* into READCXDATA. Revised public function prototypes so that it 
// would be clear they're from the NOISYEM module... Still need to build and test...
// 16may2012-- Finished test/debug. Decided to add fields xynoisy.x, xynoisy.y to report each dot's position WRT the
// target center on every display frame, but for RMVideo noisy-dots targets ONLY.
// 02oct2012-- Revised initNoisyDotsEmulator() to use mxGetNaN() to generate a NaN and mxIsNaN() to verify that the
// number generated is a NaN. The C99 runtime math functions isnan() and nan() are not available in Windows, which
// still does not bother to support the ANSI C99 standard.
// 09sep2013-- Corrected benign coding mistakes detected by newer MEX compiler. No functionality changed.
// 12aug2014-- Fixed updateRMVTgt() so that, when the RMV_WRTSCREEN flag is set, per-dot velocity components are
// reported in the screen frame of reference rather than WRT the target window. Per-dot position trajectory is always
// reported in the target window frame of reference.
//=====================================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#ifndef M_PI                           // this is defined on UNIX/Linux/Mac, but not Windows
   #define M_PI 3.14159265359
#endif

#include "noisyem.h"

#define TORADIANS(D) (((double)D) * M_PI / 180.0)
#define TODEGREES(D) (((double)D) * 180.0 / M_PI)

// The fields for each structure in the array "xynoisy" that holds results of noisy-dots target emulation.
const char *noisyFields[] = 
{
   "idx",                     //    target's ordinal position in the trial target map (zero-based index)
   "type",                    //    noise type: 0 = direc; 1 = additive speed; 2 = multiplicative speed noise
   "level",                   //    the noise level parameter, as specified in original target definition
   "updIntv",                 //    the noise update interval in ms
   "nDots",                   //    number of dots in target, N
   "dx",                      //    NxT arrays holding x- and y-cmpts of each target dot's velocity during each 
   "dy",                      //    update frame, in deg/sec subtended at the subject's eye
   "x",                       //    NxT arrays holding x- and y-coords of each target dot's position WRT the
   "y"                        //    target center during each update frame, in deg subtended at subject's eye
};
const int NUMNOISYFIELDS = 9;

//=== Uniform RNG =====================================================================================================
// The structures and functions declared in this section duplicate the implementation of C++ class CUniformRNG that
// is part of the implementation of the RMVideo RMV_RANDOMDOTS target type. We need it in order to replicate the
// behavior of that target.
//
// CUniformRNG encapsulates a pseudo-random number generator that returns a sequence of uniformly distributed floating-
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
//=====================================================================================================================

#define URNG_TABLESZ 32
#define URNG_LC_M 2147483647
#define URNG_LC_A 16807
#define URNG_LC_Q 127773
#define URNG_LC_R 2836
#define URNG_NDIV (1 + (URNG_LC_M - 1)/URNG_TABLESZ)
#define URNG_DSCALE ((double) (1.0/((double) URNG_LC_M)))

typedef struct uniformRNG_Struct
{
   int shuffle[URNG_TABLESZ];          // the shuffle table
   int lastOut;                        // the last integer spit out of shuffle table
   int curr;                           // current value I of the linear congruential generator
} UNIFORMRNG, *PUNIFORMRNG;

//=== setSeed =========================================================================================================
//
//    Initialize the random generator with the specified seed value.  The absolute value is used; if it is zero, the
//    the value 1 will be used instead.
//
//    ARGS:       seed  -- [in] seed value
//
//    RETURNS:    NONE.
//
void urng_setSeed(PUNIFORMRNG pRNG, int seed)
{
   int k, j;
   
   // start at strictly positive seed value
   pRNG->curr = (seed == 0) ? 1 : ((seed < 0) ? -seed : seed);          // start at strictly positive seed value

   // after discarding first 8 integers generated by algorithm, fill shuffle table with next TABLESZ integers
   for(j = URNG_TABLESZ+7; j >= 0; j--)
   {
      k = pRNG->curr/URNG_LC_Q;
      pRNG->curr = URNG_LC_A * (pRNG->curr - k*URNG_LC_Q) - k*URNG_LC_R;
      if( pRNG->curr < 0 ) pRNG->curr += URNG_LC_M;
      if( j < URNG_TABLESZ ) pRNG->shuffle[j] = pRNG->curr;
   }

   pRNG->lastOut = pRNG->shuffle[0];
}

//=== generate ========================================================================================================
//
//    Generate next random number in sequence, uniformly distributed in (0.0 .. 1.0).  Note that the endpoint values
//    are excluded.  The algorithm is such that we could see some skewing of the distribution at the largest float
//    value less than 1.0.
//
//    ARGS:       NONE.
//
//    RETURNS:    next value in a uniformly distributed random # sequence.
//
double urng_generate(PUNIFORMRNG pRNG)
{
   int k, index;
   
   // compute I(n+1) = A*I(n) % M using Schrage's method to avoid integer overflows
   k = pRNG->curr/URNG_LC_Q;
   pRNG->curr = URNG_LC_A * (pRNG->curr - k*URNG_LC_Q) - k*URNG_LC_R;
   if( pRNG->curr < 0 ) pRNG->curr += URNG_LC_M;

   // use last number retrieved from shuffle table to calc index of next number to retrieve. Replace that entry in 
   // table with the current output of LC generator, calc'd above.
   index = pRNG->lastOut / URNG_NDIV;
   pRNG->lastOut = pRNG->shuffle[index];
   pRNG->shuffle[index] = pRNG->curr;

   // convert int in [1..M-1] to floating-point output in (0..1)
   return( URNG_DSCALE * ((double) pRNG->lastOut) );
}

//=== END: Uniform RNG ================================================================================================


//=====================================================================================================================
// MODULE-PRIVATE TYPES, GLOBALS AND FUNCTION PROTOTYPES
//=====================================================================================================================
typedef struct noisyTgt                // per-target info and emulation variables:
{
   int tgt;                            //   index of target in the trial target map
   NOISYTGTINFO info;                  //   key defining parameters relevant to noisy-dots target 

   float tUntilUpdate;                 //   # msecs remaining until next noise update
   
   float *pNoise;                      //   current noise value assigned to each dot in this target
   
   int *pFracDX;                       //   per-dot frac pixel displacements * 2^4, carried over to next update (only
   int *pFracDY;                       //   for improved XYScope implementation introduced in Maestro v2.7.0)
                           

   float *pX;                          //   current x-coordinates of each target dot relative to center (RMVideo only)
   float *pY;                          //   current y-coordinates of each target dot relative to center (RMVideo only)
   float *pDotLives;                   //   current lifetime of each target dot, if relevant (RMVideo only)
   
   UNIFORMRNG rngDots;                 //   pseudo RNG for generating dot positions and other stuff (RMVideo only)
   UNIFORMRNG rngNoise;                //   pseudo RNG for generating noise for noisy-dots target (RMVideo only)

   int nBufLen;                        //   allocated length of buffers pdDX and pdDY.
   int nFrames;                        //   number of frame updates filled in so far.
   double* pdDX;                       //   dX in visual deg/s per dot, per update since trial recording began
   double* pdDY;                       //   dY in visual deg/s per dot, per update since trial recording began
   double* pdXPos;                     // position (X,Y) in visual deg per dot, per update since trial recording began
   double* pdYPos;
} NOISYTGT, *PNOISYTGT;

typedef struct noisyDotsEmulator
{
   BOOL     initialized;               // TRUE only if emulator object was successfully initialized
   BOOL     disabled;                  // TRUE if emulator disabled (error occurred while init. or adding targets)
   BOOL     isXY;                      // TRUE if target display is XYScope; else RMVideo
   int      version;                   // the data file version number. The implementation of the noisy-dots targets
                                       // changed as of file version 18 (Maestro 2.7.0)

   int      dist;                      // distance from eye to center of XYScope display, in mm
   int      width;                     // width of XYScope display, in mm
   int      height;                    // height of XYScope display, in mm
   long     seed;                      // current seed of RNG used to generate noise values for XYScope noisy-dots tgts

   int      xyFP;                      // current XYScope display frame period in ms (CAN CHANGE on a per-segment basis)
   double   rmvFP;                     // RMVideo display frame period in ms (NEVER CHANGES)

   int      trialLen;                  // recorded trial length in ms (assuming 1 "tick" per ms)
   
   BOOL     recOn;                     // set TRUE once emulation reaches recorded portion of trial
   BOOL     done;                      // set TRUE once emulation reaches end of recorded trial length; no further
                                       // calculations are done, as dot-velocity buffers are likely full once flag set.
   
   double   dQuietNaN;                 // quiet Not-a-Number, if available; if not, set to -999999. For RMVideo only,
                                       // a dot's velocity is NaN during any frame in which it is randomly repositioned.

   int      nTgts;                     // the list of noisy-dots targets currently being emulated
   PNOISYTGT pTargets[MAX_TRIALTARGS];
   
   double*  pTimes;                    // times at which noisy-dots targets were updated, in ms since recording began
   int      nFrames;                   // number of frame updates completed so far (since recording began)
   int      nFramesMax;                // max # frames during recorded portion of trial (buffer size for frame times)

   int      nRMVFrames;                // total elapsed trial time in # of RMVideo display frames. We use this to fill
                                       // the pTimes buffer when emulating RMVideo noisy-dots targets.
} NOISYDOTSEMU, *PNOISYDOTSEMU;


NOISYDOTSEMU m_Emulator;


void rmvRandomizeDotPos(PNOISYTGT pTgt, int idx);
int getNextRandomNumForXYDotNoise();
double radToUnitCircleDeg(double rad);
void updateXYTgt(PNOISYTGT pTgt, BOOL isOn, double patH, double patV);
void updateXYTgt_V18(PNOISYTGT pTgt, BOOL isOn, double patH, double patV);
void updateRMVTgt(PNOISYTGT pTgt, BOOL isOn, double dhPat, double dvPat, double dhWin, double dvWin);

//=====================================================================================================================
// PUBLIC FUNCTIONS DEFINED
//=====================================================================================================================

/**
 Initialize the noisy-dots target emulator. This method should be called by readcxdata() prior to using the emulator.
 If unsuccessful, attempts to use the emulator will fail.
 
 @param isXY TRUE if target display is the XYScope; else RMVideo. The two displays cannot be used simultaneously.
 @param v The data file version. This is needed to select between the old, erroneous implementation of the noisy-dots
 targets prior to Maestro v2.7.0 (v<18), and the more accurate implementation now used (v>=18). Also, the emulation 
 is disabled for data files generated prior to Maestro v2.1.3, ie, for v<12; in such cases, the method aborts without
 initializing the emulator.
 @param len The total length of the recorded portion of the trial during which noisy-dots targets will be emulated, in 
 ms. It is assumed that each trial "tick" is 1ms in duration. This determines the worst-case size of the buffers that 
 must be allocated for each target to keep track of per-dot, per-frame displacements.
 @param d The distance from the subject's eye to the center of the XYScope display, in mm. Ignored for RMVideo.
 @param w The width of the XYScope display, in mm. Ignored for RMVideo.
 @param h The height of the XYScope display, in mm. Ignored for RMVideo.
 @param seed The initial seed for the RNG used by the emulator to generate noise values for each XYScope noisy-dots 
 target. Ignored for RMVideo. Every RMVideo target has private RNGs for randomizing dot positions and for generating
 per-dot noise.
 @param rmvFP The display frame period in milliseconds for RMVideo. Ignored for XYScope.
*/
void initNoisyDotsEmulator(BOOL isXY, int v, int len, int d, int w, int h, DWORD seed, double rmvFP)
{
   int i, sz;
   PNOISYDOTSEMU pEmu;
   
   // emulator will not work for data files older than version 12
   pEmu = &m_Emulator;
   if(pEmu->initialized || v < 12) return;    // emulator should be initialized only once!
   
   pEmu->isXY = isXY;
   pEmu->version = v;
   
   pEmu->dist = d;
   pEmu->width = w;
   pEmu->height = h;
   pEmu->seed = (long) (0x00000000FFFFFFFF & ((long)seed));
   pEmu->trialLen = len;
   
   pEmu->recOn = FALSE;
   pEmu->done = FALSE;
   
   // we want to assign a non-signaling NaN value to the velocity components of any RMVideo target dot that is
   // randomly repositioned during a display frame. If NaN is not available on the system, we use -99999.0 instead.
   pEmu->dQuietNaN = mxGetNaN();
   if(!mxIsNaN(pEmu->dQuietNaN))
   {
      pEmu->dQuietNaN = -99999.0;
   }
   
   pEmu->xyFP = 2;      // this will get set during updates, whereas RMVideo frame period is a fixed FP value
   pEmu->rmvFP = rmvFP;
   
   pEmu->nTgts = 0;
   for(i=0; i<MAX_TRIALTARGS; i++) pEmu->pTargets[i] = (PNOISYTGT) NULL;
   
   // allocate buffer for update times. If this allocation fails, then emulator cannot be used.
   if(isXY)
   {
      // XYScope frame period can vary. Assume worst-case: minimum update period of 2ms throughout trial recording
      sz = 1 + (len/2);
   }
   else
   {
      // for RMVideo, frame period is fixed
      sz = 2 + ((double)len) / rmvFP;
   }
   pEmu->pTimes = (double*) malloc(sizeof(double) * sz);
   pEmu->initialized = (pEmu->pTimes != NULL);
   pEmu->disabled = !(pEmu->initialized);
   
   pEmu->nFrames = 0;
   pEmu->nFramesMax = pEmu->initialized ? sz : 0;
   
   pEmu->nRMVFrames = -1;     // incremented to 0 on first update, which takes place at trial's start
}

/**
 Release all memory allocated for the noisy-dots emulator. This method should be called by readcxdata() prior
 to terminating the program.
*/
void releaseNoisyDotsEmulator()
{
   int i;
   PNOISYTGT pTgt;
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   
   for(i=0; i<pEmu->nTgts; i++) if(pEmu->pTargets[i] != NULL)
   {
      // free allocated arrays for each noisy dots target emulated
      pTgt = pEmu->pTargets[i];
      if(pTgt->pNoise != NULL)
      {
         free(pTgt->pNoise);
         pTgt->pNoise = NULL;
      }
      if(pTgt->pFracDX != NULL)
      {
         free(pTgt->pFracDX);
         pTgt->pFracDX = NULL;
      }
      if(pTgt->pFracDY != NULL)
      {
         free(pTgt->pFracDY);
         pTgt->pFracDY = NULL;
      }
      if(pTgt->pX != NULL)
      {
         free(pTgt->pX);
         pTgt->pX = NULL;
      }
      if(pTgt->pY != NULL)
      {
         free(pTgt->pY);
         pTgt->pY = NULL;
      }
      if(pTgt->pDotLives != NULL)
      {
         free(pTgt->pDotLives);
         pTgt->pDotLives = NULL;
      }
      if(pTgt->pdDX != NULL)
      {
         free(pTgt->pdDX);
         pTgt->pdDX = NULL;
      }
      if(pTgt->pdDY != NULL)
      {
         free(pTgt->pdDY);
         pTgt->pdDY = NULL;
      }
      if(pTgt->pdXPos != NULL)
      {
         free(pTgt->pdXPos);
         pTgt->pdXPos = NULL;
      }
      if(pTgt->pdYPos != NULL)
      {
         free(pTgt->pdYPos);
         pTgt->pdYPos = NULL;
      }
      
      free(pEmu->pTargets[i]);
      pEmu->pTargets[i] = (PNOISYTGT) NULL;
   }
   pEmu->nTgts = 0;

   if(pEmu->pTimes != NULL)
   {
      free(pEmu->pTimes);
      pEmu->pTimes = NULL;
   }
   pEmu->initialized = FALSE;
   pEmu->disabled = TRUE;
   pEmu->nFrames = 0;
   pEmu->nFramesMax = 0;
   pEmu->nRMVFrames = -1;
}

/**
 Add a noisy-dots target to be simulated by the emulator. This method must be called once for each target, after 
 initialization but before the first call to updateTarget().
 NOTE: If this method fails for any reason, the emulator is henceforth DISABLED until it is released and reinitialized.
 
 @param tgt The target's index within the trial target map.
 @param pInfo Key defining parameters about the noisy-dots target, including the type, the noise level, update interval,
 number of dots, and other information needed to emulate the target correctly.
 @return TRUE if target was successfully added, FALSE otherwise, indicating that a target parameter was invalid, the
 target object could not be allocated, or the maximum number of targets has been reached.
*/
BOOL addNoisyDotsTarget(int tgt, PNOISYTGTINFO pInfo)
{
   int i;
   PNOISYTGT pTgt;
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;

   if(pEmu->disabled || !pEmu->initialized) return(FALSE);
   if(pEmu->nTgts == MAX_TRIALTARGS) { pEmu->disabled = TRUE; return(FALSE); }
   
   if(pInfo->type != EMU_NOISYDIR && pInfo->type != EMU_NOISYSPD_ADD && pInfo->type != EMU_NOISYSPD_MUL) 
   {
      pEmu->disabled = TRUE;
      return(FALSE);
   }
   if(pInfo->level < 1 || (pInfo->type==EMU_NOISYDIR && pInfo->level > 180) || 
         (pInfo->type==EMU_NOISYSPD_ADD && pInfo->level > 300) || (pInfo->type==EMU_NOISYSPD_MUL && pInfo->level > 7) ||
         (pInfo->updIntv < 2) || (pInfo->nDots < 1 || pInfo->nDots > 1000))
   {
      pEmu->disabled = TRUE;
      return(FALSE);
   }
   if(!pEmu->isXY)
   {
      if(pInfo->iPctCoherent < 0 || pInfo->iPctCoherent > 100 || pInfo->fOuterW < RMV_MINRECTDIM || 
            pInfo->fOuterW > RMV_MAXRECTDIM || pInfo->fOuterH < RMV_MINRECTDIM || pInfo->fOuterH > RMV_MAXRECTDIM ||
            pInfo->fDotLife < 0.0f)
      {
         pEmu->disabled = TRUE;
         return(FALSE);
      }
   }

   pTgt = (PNOISYTGT) malloc(sizeof(NOISYTGT));
   if(pTgt == NULL) {pEmu->disabled = TRUE; return(FALSE); }
   
   pEmu->pTargets[pEmu->nTgts] = pTgt;
   ++(pEmu->nTgts);

   pTgt->tgt = tgt;
   pTgt->info.type = pInfo->type;
   pTgt->info.level = pInfo->level;
   pTgt->info.updIntv = pInfo->updIntv;
   pTgt->info.nDots = pInfo->nDots;
   
   // we need some additional information for emulating noisy-dots targets on RMVideo
   if(!pEmu->isXY)
   {
      pTgt->info.iFlags = pInfo->iFlags;
      pTgt->info.iPctCoherent = pInfo->iPctCoherent;
      pTgt->info.fDotLife = pInfo->fDotLife;
      pTgt->info.iSeed = pInfo->iSeed;
      pTgt->info.fOuterW = pInfo->fOuterW;
      pTgt->info.fOuterH = pInfo->fOuterH;
   }
   
   // target dot noise values will be generated on first update during which target is ON
   pTgt->tUntilUpdate = 0.0f; 
   pTgt->pNoise = (float*) malloc(sizeof(float) * pTgt->info.nDots);
   if(pTgt->pNoise == NULL) { pEmu->disabled = TRUE; return(FALSE); }
   
   // per-dot fractional pixel displacements (scaled by 2^4) carried over to next update -- if needed XYScope only
   if(pEmu->isXY && pEmu->version >= 18)
   {
      pTgt->pFracDX = (int*) malloc(sizeof(int) * pTgt->info.nDots);
      if(pTgt->pFracDX == NULL) { pEmu->disabled = TRUE; return(FALSE); }
      pTgt->pFracDY = (int*) malloc(sizeof(int) * pTgt->info.nDots);
      if(pTgt->pFracDY == NULL) { pEmu->disabled = TRUE; return(FALSE); }
   }
   else
   {
      pTgt->pFracDX = NULL;
      pTgt->pFracDY = NULL;
   }

   // per-dot (x, y) positions and dot lives (RMVideo only)
   if(!pEmu->isXY)
   {
      pTgt->pX = (float*) malloc(sizeof(float) * pTgt->info.nDots);
      if(pTgt->pX == NULL) { pEmu->disabled = TRUE; return(FALSE); }
      pTgt->pY = (float*) malloc(sizeof(float) * pTgt->info.nDots);
      if(pTgt->pY == NULL) { pEmu->disabled = TRUE; return(FALSE); }
      
      pTgt->pDotLives = NULL;
      if(pTgt->info.fDotLife > 0.0f)
      {
         pTgt->pDotLives = (float*) malloc(sizeof(float) * pTgt->info.nDots);
         if(pTgt->pDotLives == NULL) { pEmu->disabled = TRUE; return(FALSE); }
      }
   }
   else
   {
      pTgt->pX = NULL;
      pTgt->pY = NULL;
      pTgt->pDotLives = NULL;
   }
   
   // worst-case buffer size needed to store per-dot velocities, positions
   pTgt->nBufLen = pTgt->info.nDots * pEmu->nFramesMax;
   pTgt->nFrames = 0;
   
   pTgt->pdDX = (double*) malloc(pTgt->nBufLen * sizeof(double));
   if(pTgt->pdDX == NULL) { pEmu->disabled = TRUE; return(FALSE); }
   pTgt->pdDY = (double*) malloc(pTgt->nBufLen * sizeof(double));
   if(pTgt->pdDY == NULL) { pEmu->disabled = TRUE; return(FALSE); }

   // RMVideo only: Buffer to store dot positions during each frame of recorded portion of trial
   if(!pEmu->isXY)
   {
      pTgt->pdXPos = (double*) malloc(pTgt->nBufLen * sizeof(double));
      if(pTgt->pdXPos == NULL) { pEmu->disabled = TRUE; return(FALSE); }
      pTgt->pdYPos = (double*) malloc(pTgt->nBufLen * sizeof(double));
      if(pTgt->pdYPos == NULL) { pEmu->disabled = TRUE; return(FALSE); }
   }
   else
   {
      pTgt->pdXPos = NULL;
      pTgt->pdYPos = NULL;
   }
   
   // for RMVideo noisy-dots target: seed the RNGs and randomize initial dot positions and, if necessary, dot lifetimes.
   if(!pEmu->isXY)
   {
      urng_setSeed(&(pTgt->rngDots), pTgt->info.iSeed);
      urng_setSeed(&(pTgt->rngNoise), pTgt->info.iSeed);
      
      for(i=0; i<pTgt->info.nDots; i++) rmvRandomizeDotPos(pTgt, i);
      
      if(pTgt->info.fDotLife > 0.0f) for(i=0; i<pTgt->info.nDots; i++)
      {
         pTgt->pDotLives[i] = (float) urng_generate(&(pTgt->rngDots)) * ((double) pTgt->info.fDotLife);
      }
   }

   return(TRUE);
}

/**
 Is the noisy dots emulator currently enabled? This will be the case only if: (1) it has been initialized successfully,
 (2) at least one target has been added to the emulator, (3) and no errors occurred while adding targets.
 @return TRUE iff emulator is enabled, as described above.
*/
BOOL isNoisyDotsEmulatorEnabled()
{
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   return(pEmu->initialized && (!pEmu->disabled) && (pEmu->nTgts > 0));
}

/**
 Mark the beginning of the next display frame update during a trial that uses the noisy-dots emulator. This must be
 called ONCE per frame update, prior to invoking updateTarget() for each noisy-dots target being emulated. What it does:
 
 1) It increments the number of RMVideo display frames elapsed. This count is used to calculate the time of each
 frame update once recording begins; if XYScope targets are emulate, it saves the current frame period, which can vary.
 2) If we're in the recorded portion of the trial, it saves the time of the frame update in milliseconds relative to
 the start of recording.
 3) It checks to see if we've reached the end of the recorded portion of the trial. It is possible that a prematurely 
 stopped trial will be saved to file. In this case, trial code processing continues to trial's original end, BEYOND what
 was saved. However, this module only prepares results for the SAVED portion of the trial. Our buffers are only sized 
 for that! Once we've passed the end of the saved portion of the trial, we do nothing. Else a buffer overflow could 
 occur.
 
 @param tick The elapsed time since trial's start, in milliseconds.
 @param recTick The elapsed time at which recording began, in milliseconds. Will be -1 until we reach the recorded
 portion of the trial.
 @param xyFP The current XYScope display frame period in milliseconds. This can vary over the course of a trial, on a
 per-segment basis. Ignored when emulating RMVideo targets, since the RMVideo display frame is fixed.
*/
void startNoisyDotsUpdate(int tick, int recTick, int xyFP)
{
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;

   // check to see if we've reached the beginning or end of recorded portion of the trial (see note 3 in fcn header).
   // do nothing if emulator is disabled.
   if(pEmu->done || pEmu->disabled) return;
   if(recTick >= 0)
   {
      pEmu->recOn = TRUE;
      if(tick - recTick >= pEmu->trialLen)
      {
         pEmu->done = TRUE;
         return;
      }
   }
   
   ++(pEmu->nRMVFrames);
   pEmu->xyFP = xyFP;
   
   if(recTick >= 0)
   {
      if(pEmu->isXY) pEmu->pTimes[pEmu->nFrames] = tick - recTick;
      else pEmu->pTimes[pEmu->nFrames] = pEmu->rmvFP * pEmu->nRMVFrames - ((double) recTick);

      ++(pEmu->nFrames);
   }
}

/**
 Update the motion of each dot in the noisy-dots target specified for the current display frame update. This
 method must be called once for each noisy-dots target during each display frame update frame throughout the trial. 
 
 24jan2012: Calls one of three private modules functions: updateRMVTgt() handles RMVideo noisy-dots targets; 
 updateXYTgt() handles XYScope noisy-dots targets prior to Maestro 2.7.0 (file version < 18); and updateXYTgt_V18() 
 handles XYScope noisy-dots targets in Maestro v2.7.0 or later (version >= 18).
  
 @param tgt Trial target index. If this does not match a target in the emulator object, no action is taken.
 @param isOn TRUE if the specified target is currently ON; FALSE if it is OFF.
 @param dhPat, dvPat The target's H,V pattern displacement for the current update frame, in deg subtended at the eye.
 @param dhWin, dvWin The target's H,V window displacement for the current frame, in deg. Ignored for XYScope tgts.
*/
void updateNoisyDotsTarget(int tgt, BOOL isOn, double dhPat, double dvPat, double dhWin, double dvWin)
{
   int i;
   PNOISYDOTSEMU pEmu;
   PNOISYTGT pTgt;
   
   pEmu = &m_Emulator;
   
   if((!pEmu->initialized) || pEmu->disabled || pEmu->done || pEmu->nTgts == 0) return;
   
   // find target object corresponding to the target to be updated
   pTgt = NULL;
   for(i=0; i<pEmu->nTgts; i++) if(pEmu->pTargets[i]->tgt == tgt)
   {
      pTgt = pEmu->pTargets[i];
      break;
   }
   if(pTgt == NULL) return;

   // update the target: RMVideo, XYScope for V>=18, or XYScope for V<18
   if(!pEmu->isXY) updateRMVTgt(pTgt, isOn, dhPat, dvPat, dhWin, dvWin);
   else if(pEmu->version < 18) updateXYTgt(pTgt, isOn, dhPat, dvPat);
   else updateXYTgt_V18(pTgt, isOn, dhPat, dvPat);
}

/**
 Adds the results of the noisy-dots target emulator to the "xynoisy" and "xynoisytimes" fields in the output structure 
 returned by readcxdata(). The latter field is a 1xT array of times at which the display frame updates occurred, in ms 
 since trial recording was turned ON. For XYScope, these times will be integral values, as the frame interval is always
 a multiple of 2ms. For RMVideo, they will be non-integral, since the RMVideo frame period is fixed and is generally not
 an integral # of milliseconds.
 
 The "noisy" field is an array of structures, one for each noisy-dots target emulated. Each structure contains the 
 following fields:
    1) 'idx' : The target's ordinal position in the trial target map (a zero-based index).
    2) 'type': The type of noisy-dots target: 0 = direction noise, 1 = additive speed noise, 2 = mult. speed noise.
    3) 'level' : The noise level parameter, as specified in the target definition.
    4) 'updIntv' : The noise update interval, as specified in the target definition.
    5) 'nDots' : The number of target dots, N.
    7) 'dx', 'dy' : Two NxT arrays holding the x and y components of the "instantaneous" velocity of each target dot 
    during each display frame during the recorded portion of the trial. Each component is in deg/sec subtended at the
    subject's eye. For RMVideo targets ONLY, a dot's velocity is "NaN" during any frame in which that dot is randomly
    repositioned. 
    
 If the list of noisy-dots targets emulated is empty, then both fields will be set to empty arrays. If the number of
 update frames filled in is not the same for all targets emulated or does not match the number of frame update times, 
 the fields will also be set to empty arrays, as this indicates improper usage of the emulator.
 
 @param pOut Ptr to the MATLAB structure array prepared by readcxdata().
*/
void setNoisyDotsResults(mxArray* pOut)
{
   BOOL ok;
   int i;
   double* pdArray;
   mxArray* pMX;
   PNOISYTGT pTgt;
   PNOISYDOTSEMU pEmu;

   pEmu = &m_Emulator;

   ok = (pEmu->initialized && pEmu->nTgts > 0 && pEmu->nFrames > 0);
   for(i=0; ok && i<pEmu->nTgts; i++) ok = (pEmu->pTargets[i]->nFrames == pEmu->nFrames);

   if(!ok)
   {
      // set relevant fields to empty arrays and return
       mxSetField(pOut, 0, "xynoisytimes", mxCreateDoubleMatrix(1, 0, mxREAL));
       mxSetField(pOut, 0, "xynoisy", mxCreateStructMatrix(1, 0, NUMNOISYFIELDS, noisyFields));
       return;
   }
   
   // fill the "xynoisytimes" field
   pMX = mxCreateDoubleMatrix(1, pEmu->nFrames, mxREAL);
   pdArray = mxGetPr(pMX);
   for(i=0; i<pEmu->nFrames; i++) pdArray[i] = pEmu->pTimes[i];
   mxSetField(pOut, 0, "xynoisytimes", pMX);
   
   // fill the "xynoisy" field
   pMX = mxCreateStructMatrix(1, pEmu->nTgts, NUMNOISYFIELDS, noisyFields);
   for(i=0; i<pEmu->nTgts; i++)
   {
      pTgt = pEmu->pTargets[i];
      mxSetField(pMX, i, "idx", mxCreateDoubleScalar(pTgt->tgt));
      mxSetField(pMX, i, "type", mxCreateDoubleScalar(pTgt->info.type));
      mxSetField(pMX, i, "level", mxCreateDoubleScalar(pTgt->info.level));
      mxSetField(pMX, i, "updIntv", mxCreateDoubleScalar(pTgt->info.updIntv));
      mxSetField(pMX, i, "nDots", mxCreateDoubleScalar(pTgt->info.nDots));

      mxSetField(pMX, i, "dx", mxCreateDoubleMatrix(pTgt->info.nDots, pTgt->nFrames, mxREAL));
      pdArray = mxGetPr(mxGetField(pMX, i, "dx"));
      memcpy((void*) pdArray, (void*) pTgt->pdDX, sizeof(double) * pTgt->info.nDots * pTgt->nFrames);
      
      mxSetField(pMX, i, "dy", mxCreateDoubleMatrix(pTgt->info.nDots, pTgt->nFrames, mxREAL));
      pdArray = mxGetPr(mxGetField(pMX, i, "dy"));
      memcpy((void*) pdArray, (void*) pTgt->pdDY, sizeof(double) * pTgt->info.nDots * pTgt->nFrames);
      
      // per-dot, per-update position trajectory. RMVideo only. Arrays are empty for XYScope targets
      if(pEmu->isXY)
      {
         mxSetField(pMX, i, "x", mxCreateDoubleMatrix(1, 0, mxREAL));
         mxSetField(pMX, i, "y", mxCreateDoubleMatrix(1, 0, mxREAL));
      }
      else
      {
         mxSetField(pMX, i, "x", mxCreateDoubleMatrix(pTgt->info.nDots, pTgt->nFrames, mxREAL));
         pdArray = mxGetPr(mxGetField(pMX, i, "x"));
         memcpy((void*) pdArray, (void*) pTgt->pdXPos, sizeof(double) * pTgt->info.nDots * pTgt->nFrames);
      
         mxSetField(pMX, i, "y", mxCreateDoubleMatrix(pTgt->info.nDots, pTgt->nFrames, mxREAL));
         pdArray = mxGetPr(mxGetField(pMX, i, "y"));
         memcpy((void*) pdArray, (void*) pTgt->pdYPos, sizeof(double) * pTgt->info.nDots * pTgt->nFrames);
      }
      
   }
   mxSetField(pOut, 0, "xynoisy", pMX);

}

//=====================================================================================================================
// MODULE-PRIVATE FUNCTIONS
//=====================================================================================================================

/** 
 Randomize the current position of the specified dot in the specified RMVideo noisy-dots target. Applies only when 
 RMVideo is the display platform. Do NOT invoke on XYScope noisy-dots target!
 @param pTgt The RMVideo noisy-dots target object.
 @param idx Zero-based index into the dot coordinates arrays. ASSUMED to lie in [0..#dots-1] !!
*/
void rmvRandomizeDotPos(PNOISYTGT pTgt, int idx)
{
   double x, y;
   
   // pick random coordinates in (0..1)
   x = urng_generate(&(pTgt->rngDots));
   y = urng_generate(&(pTgt->rngDots));
   
   // map to dimensions of target's bounding rectangle (coords are WRT to center of that rectangle)
   pTgt->pX[idx] = (float) ((x - 0.5) * ((double) pTgt->info.fOuterW));
   pTgt->pY[idx] = (float) ((y - 0.5) * ((double) pTgt->info.fOuterH));
}

/**
 Get the next integer in [0..65536] from the simple RNG used to generate noise values for XYScope noisy-dots targets.
 NOT applicable to RMVideo noisy-dots emulation.
 @return Number generated by the emulator's RNG for XYScope. Will be a 32-bit int in [0..65535].
*/
int getNextRandomNumForXYDotNoise()
{
   long temp;
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   temp = 2147437301L * pEmu->seed + 453816981L;
   pEmu->seed = 0x00000000FFFFFFFF & temp;
   return((int) (0x0000FFFF & (pEmu->seed >> 8)));         
}

/**
 Convert angle in radians to an angle in degrees in the unit circle.
 @param rad The angle in radians.
 @return The same angle in degrees, restricted to the unit circle, [0..360) deg.
*/
double radToUnitCircleDeg(double rad)
{
   double deg;
   
   deg = TODEGREES(rad);
   while(deg > 360) deg -= 360;
   while(deg < 0) deg += 360;
   
   return(deg);
}

/**
 Update the state of the specified XYScope noisy-dots target for the current display frame. This method emulates 
 XYScope targets for data file versions < 18, prior to Maestro 2.7.0. There are some issues with the XYScope noisy-dots
 implementation, and this emulates that erroneous implementation. The problems are addressed in Maestro v2.7.0, and
 the revised implementation is handled by updateXYTgt_V18().
 
 If the target is not on, then nothing happens. Each target dot's DX and DY (in deg/sec) are set to 0 for that frame 
 and the target's noise update interval clock is NOT decremented. If it is on, then:
    1) If noise update interval clock is at 0 or less, it is reset and new noise values are generated for each target
    dot.
    2) The noise update interval clock is decremented by the current XYScope frame duration.
    3) The per-dot displacement velocities in pixels/ms are computed for the current XYScope frame, then converted to
    visual deg/sec and saved in internal buffers IF we've reached the recorded portion of the trial.
    
 @param pTgt Pointer to the target object.
 @param isOn TRUE if the specified target is ON for the current update frame; else it is OFF.
 @param patH, patV The target's H,V pattern displacement for the current update frame, in deg subtended at the eye.
*/
void updateXYTgt(PNOISYTGT pTgt, BOOL isOn, double patH, double patV)
{
   int i, j, n, dxPix, dyPix;
   double alphaX, alphaY, dX, dY, dR, theta, val, frameDur;
   short shDeltaR, shTheta;
   int scale, cosLUT, sinLUT, ev, iDeltaR, iTheta;
   double* pdArray;
   
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   frameDur = (double) pEmu->xyFP;
   
   //
   // compute pixel displacements for each dot during current frame for the specified target. If target is not ON, the 
   // displacements are zero. It is assumed that caller keeps track of any changes in pattern while the target is OFF, 
   // and then the net change will be accounted for when the target comes back ON. The per-dot pixel displacements are 
   // converted to velocities in visual deg/sec and saved in the internal buffers ONLY if we've reached the recorded 
   // portion of the trial being simulated.
   //
   if(isOn)
   {
      // step 1: Pattern displacement (dX,dY) in degrees subtended at eye to pixels on XYScope display
      alphaX = 32768 / radToUnitCircleDeg(atan2(((double)pEmu->width)/2.0, pEmu->dist));
      alphaY = 32768 / radToUnitCircleDeg(atan2(((double)pEmu->height)/2.0, pEmu->dist));
      dX = alphaX * patH;
      dY = alphaY * patV;

      // step 2: Convert to polar coordinates, then scale and discretize for sending to XYCORE
      dR = sqrt(dX*dX + dY*dY);
      theta = radToUnitCircleDeg(atan2(dY, dX));
      if(dR < 200) 
      {
         shDeltaR = (short) (dR * 64);
         shTheta = (short) (theta * 10);
      }
      else
      {
         shDeltaR = (short) dR;
         shTheta = ((short) (theta * 10));
         shTheta += (short) 10000;
      }
      
      // step 3: Now we're in XYCORE. First, determine the scale factor for the integer calcs: 2^16 or 2^10.
      scale = 16;
      if(shTheta > 10000) { scale = 10; shTheta -= (short) 10000; }
      if(shTheta < 0) { shTheta += (short) 3600; }
      else shTheta = (short) (shTheta % 3600);

      // speed-noise only: calculate scaled values of cos() and sin() for the (constant) direction of motion, as
      // computed in XYCORE LUTs. Note that direction must be positive integer in [0..3599] (deg/10).
      if(pTgt->info.type != EMU_NOISYDIR)
      {
         cosLUT = (int) floor(1024.0 * cos(shTheta * 0.0017453293) + 0.5);
         sinLUT = (int) floor(1024.0 * sin(shTheta * 0.0017453293) + 0.5);
      }
      
      // multiplicative speed noise only: Expected value of 2^x for uniform random number x chosen from [-N:N], for
      // N=1 to N=7. We only need to calculate this once.
      ev = 0;
      if(pTgt->info.type == EMU_NOISYSPD_MUL)
      {
         double val = 1024.0 * (pow(2.0, (double)pTgt->info.level) - pow(2.0, (double)(-pTgt->info.level)));
         val /= (2.0 * ((double)pTgt->info.level) * log(2.0));
         ev = (int) floor(val + 0.5);
      }
      
      // has target's noise update interval clock expired? If so, reset it and choose new noise values for each target
      // dot. For dir noise, choose noise offset direction from 10 * [-N:N]. For additive speed noise, choose speed % 
      // offset from [-N:N]. For multiplicative speed noise, choose index n from [-20N : 20N]; then exponent x = n/20.
      // Here N is the target's noise level parameter.
      if(pTgt->tUntilUpdate - 0.5f <= 0.0f)   // this is just in case we get a FP value very close to but > 0
      {
         n = (pTgt->info.type==EMU_NOISYDIR || pTgt->info.type==EMU_NOISYSPD_ADD) ? 
               (pTgt->info.level*2 + 1) : (pTgt->info.level*40 + 1);
         for(j=0; j<pTgt->info.nDots; j++) 
         {
            if(pTgt->info.type == EMU_NOISYDIR) 
               pTgt->pNoise[j] = 10 * ((getNextRandomNumForXYDotNoise() % n) - pTgt->info.level);
            else if(pTgt->info.type == EMU_NOISYSPD_ADD) 
               pTgt->pNoise[j] = (getNextRandomNumForXYDotNoise() % n) - pTgt->info.level;
            else 
               pTgt->pNoise[j] = (getNextRandomNumForXYDotNoise() % n) - pTgt->info.level * 20;
         }
      
         pTgt->tUntilUpdate = pTgt->info.updIntv;
      }
      
      // decrement the update interval clock IAW the current frame's duration
      pTgt->tUntilUpdate -= (float) frameDur;
      
      // for each dot in target, compute change in x- and y-coords in XYScope pixels IAW XYCORE code. We only have to
      // do this IF we're in the recorded portion of the trial...
      // IMPORTANT: We do ((int) pTgt->pNoise[j] + 0.5f) in the code below because the noise value is now FP to handle 
      // emulation of RMVideo targets. The rounding ensures we get the right integer value; in XYScope implementation,
      // the noise values are integers.
      if(pEmu->recOn) 
      {
         i = pTgt->nFrames;
         i *= pTgt->info.nDots;
         
         for(j=0; j<pTgt->info.nDots; j++)
         {
            iDeltaR = shDeltaR;
            if(pTgt->info.type == EMU_NOISYDIR)
            {
               iTheta = shTheta + ((int) (pTgt->pNoise[j] + 0.5f));
               if(iTheta < 0) iTheta += 3600;
               else iTheta = iTheta % 3600;
                  
               cosLUT = (int) floor(1024.0 * cos(iTheta * 0.0017453293) + 0.5);
               sinLUT = (int) floor(1024.0 * sin(iTheta * 0.0017453293) + 0.5);               
            }
            else if(pTgt->info.type == EMU_NOISYSPD_ADD)
            {
               iDeltaR = iDeltaR * ((int) (pTgt->pNoise[j] + 0.5f)) / ((int) 100);
               iDeltaR += shDeltaR;
            }
            else
            {
               iDeltaR = (int) floor( pow(2.0, 20.0 + ((double) pTgt->pNoise[j])/20.0) + 0.5 );
               iDeltaR /= ev;
               val = ((double) iDeltaR) * ((double) shDeltaR) / 1024.0;
               iDeltaR = (int) val;
            }
            
            
            dxPix = ((iDeltaR * cosLUT) >> scale);
            dyPix = ((iDeltaR * sinLUT) >> scale);
            
            if((i+j) < pTgt->nBufLen)
            {
               pTgt->pdDX[i+j] = ((double) dxPix) / ((double) frameDur);
               pTgt->pdDX[i+j] *= (1000.0 / alphaX);
               pTgt->pdDY[i+j] = ((double) dyPix) / ((double) frameDur);
               pTgt->pdDY[i+j] *= (1000.0 / alphaY);
            }
         }
         
         ++(pTgt->nFrames);
      }
   }
   else if(pEmu->recOn)
   {
      // special case: target OFF during recorded portion of trial. All target dot velocities are zero and must be 
      // stored. (While XY targets move while off, that's affected by accumulating the target displacement while off
      // and then updating it in one go once it comes on. This has a "bad" effect on the noisy dots targets!)
      i = pTgt->nFrames;
      i *= pTgt->info.nDots;
      for(j=0; j<pTgt->info.nDots; j++)
      {
         if((i+j) < pTgt->nBufLen)
         {
            pTgt->pdDX[i+j] = 0.0;
            pTgt->pdDY[i+j] = 0.0;
         }
      }
      ++(pTgt->nFrames);
   }
}

/**
 Implementation of updateXYTgt() for data files generated by Maestro 2.7.0 or later (file version >= 18). Maestro
 2.7.0 fixed two major bugs in the implementation of the XYScope noisy-dots targets. This method emulates the newer
 implementation, while updateXYTgt() handles the older, erroneous implementation.
 
 @param pTgt Pointer to the target object.
 @param isOn TRUE if the specified target is ON for the current update frame; else it is OFF.
 @param patH, patV The target's H,V pattern displacement for the current update frame, in deg subtended at the eye.
*/
void updateXYTgt_V18(PNOISYTGT pTgt, BOOL isOn, double patH, double patV)
{
   int i, j, n, dxPix, dyPix, saveDXPix, saveDYPix;
   double alphaX, alphaY, dX, dY, dR, theta, val, frameDur;
   short shDeltaR, shTheta;
   int scale, cosLUT, sinLUT, ev, iDeltaR, iTheta;
   double* pdArray;
   
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   frameDur = (double) pEmu->xyFP;
   
   //
   // compute pixel displacements for each dot during current frame for the specified target. If target is not ON, the 
   // displacements are zero. It is assumed that caller keeps track of any changes in pattern while the target is OFF, 
   // and then the net change will be accounted for when the target comes back ON. The per-dot pixel displacements are 
   // converted to velocities in visual deg/sec and saved in the internal buffers ONLY if we've reached the recorded 
   // portion of the trial being simulated.
   //
   alphaX = 32768 / radToUnitCircleDeg(atan2(((double)pEmu->width)/2.0, pEmu->dist));
   alphaY = 32768 / radToUnitCircleDeg(atan2(((double)pEmu->height)/2.0, pEmu->dist));
   if(isOn)
   {
      // step 1: Pattern displacement (dX,dY) in degrees subtended at eye to millimeters on XYScope display.
      dX = tan(TORADIANS(patH)) * pEmu->dist;
      dY = tan(TORADIANS(patV)) * pEmu->dist;

      // step 2: Convert to polar coordinates r(mm), theta(deg), then scale and discretize for sending to XYCORE. Scale 
      // displacement amplitudes < 0.1 by 2^16 and all others by 2^10 for more precision in the calculations on the 
      // XYCORE side. This essentially hands over responsibility for keeping track of fractional pixel displacements to 
      // XYCORE, which we have to do b/c each dot in the target moves differently! For the latter case, 10000 is added 
      // to THETA so that XYCORE will know which scale factor to use.
      // IMPORTANT: The scale factors and threshold were chosen to support a pattern velocity range of  0.1-200deg/sec 
      // and a distance-to-eye range of 250-1000mm.
      dR = sqrt(dX*dX + dY*dY);
      theta = radToUnitCircleDeg(atan2(dY, dX));
      if(dR < 0.1) 
      {
         shDeltaR = (short) (dR * 65536.0);
         shTheta = (short) (theta * 10);
      }
      else
      {
         shDeltaR = (short) (dR * 1024.0);
         shTheta = ((short) (theta * 10));
         shTheta += (short) 10000;
      }
      
      // step 3: Now we're in XYCORE. First, determine the power of 2 by which we must divide to get pixel displacement
      // scaled by 2^4.
      // STRATEGY: hv = Rmm*2^Q, where Q=16 for Rmm < 0.1, or Q=10 otherwise. If THETA>=10000, then Q=10. For the 
      // practical range of display geometries and pattern velocities, we can expect that Rmm < 2^5, so hv < 2^21 
      // worst-case. Since the trig lookup tables are pre-scaled by 2^10, we have:
      //   Xmm(scaled) = Rmm*2^Q*2^10*cos(TH) = Rmm*cos(TH)*2^(Q+10) = Xmm*2^(Q+10)
      //   Xpix(scaled)= Xmm*(2^16/screenW_mm)*2^(Q-6) = Xpix*2^(Q-6) = Xpix*2^(P), P=4 or 10.
      // When P=10, we divide by 2^6 so that we leave in a scale factor of 2^4. We then add in the fractional pixel
      // displacement from the previous frame update, also scaled by 2^4. We then save the fractional pixel displacement
      // for the next update and get the integer pixel displacement for this frame, Xpix. Analogously, for Ypix.
      scale = 6;
      if(shTheta > 10000) { scale = 0; shTheta -= (short) 10000; }
      if(shTheta < 0) { shTheta += (short) 3600; }
      else shTheta = (short) (shTheta % 3600);

      // speed-noise only: calculate scaled values of cos() and sin() for the (constant) direction of motion, as
      // computed in XYCORE LUTs. Note that direction must be positive integer in [0..3599] (deg/10).
      if(pTgt->info.type != EMU_NOISYDIR)
      {
         cosLUT = (int) floor(1024.0 * cos(shTheta * 0.0017453293) + 0.5);
         sinLUT = (int) floor(1024.0 * sin(shTheta * 0.0017453293) + 0.5);
      }
      
      // multiplicative speed noise only: Expected value of 2^x for uniform random number x chosen from [-N:N], for
      // N=1 to N=7. We only need to calculate this once.
      ev = 0;
      if(pTgt->info.type == EMU_NOISYSPD_MUL)
      {
         double val = 1024.0 * (pow(2.0, (double)pTgt->info.level) - pow(2.0, (double)(-pTgt->info.level)));
         val /= (2.0 * ((double)pTgt->info.level) * log(2.0));
         ev = (int) floor(val + 0.5);
      }
      
      // has target's noise update interval clock expired? If so, reset it and choose new noise values for each target
      // dot. For dir noise, choose noise offset direction from 10 * [-N:N]. For additive speed noise, choose speed % 
      // offset from [-N:N]. For multiplicative speed noise, choose index n from [-20N : 20N]; then exponent x = n/20.
      // Here N is the target's noise level parameter.
      if(pTgt->tUntilUpdate - 0.5f <= 0.0f)   // this is just in case we get a FP value very close to but > 0
      {
         n = (pTgt->info.type==EMU_NOISYDIR || pTgt->info.type==EMU_NOISYSPD_ADD) ? 
               (pTgt->info.level*2 + 1) : (pTgt->info.level*40 + 1);
         for(j=0; j<pTgt->info.nDots; j++) 
         {
            if(pTgt->info.type == EMU_NOISYDIR) 
               pTgt->pNoise[j] = 10 * ((getNextRandomNumForXYDotNoise() % n) - pTgt->info.level);
            else if(pTgt->info.type == EMU_NOISYSPD_ADD) 
               pTgt->pNoise[j] = (getNextRandomNumForXYDotNoise() % n) - pTgt->info.level;
            else 
               pTgt->pNoise[j] = (getNextRandomNumForXYDotNoise() % n) - pTgt->info.level * 20;
         }
      
         pTgt->tUntilUpdate = (float) pTgt->info.updIntv;
      }
      
      // decrement the update interval clock IAW the current frame's duration
      pTgt->tUntilUpdate -= (float) frameDur;
      
      // for each dot in target, compute change in x- and y-coords in XYScope pixels IAW XYCORE code.
      // IMPORTANT: We do ((int) pTgt->pNoise[j] + 0.5f) in the code below because the noise value is now FP to handle 
      // emulation of RMVideo targets. The rounding ensures we get the right integer value; in XYScope implementation,
      // the noise values are integers.
      i = pTgt->nFrames;
      i *= pTgt->info.nDots;
      
      for(j=0; j<pTgt->info.nDots; j++)
      {
         iDeltaR = shDeltaR;
         if(pTgt->info.type == EMU_NOISYDIR)
         {
            iTheta = shTheta + ((int) (pTgt->pNoise[j] + 0.5f));
            if(iTheta < 0) iTheta += 3600;
            else iTheta = iTheta % 3600;
                  
            cosLUT = (int) floor(1024.0 * cos(iTheta * 0.0017453293) + 0.5);
            sinLUT = (int) floor(1024.0 * sin(iTheta * 0.0017453293) + 0.5);               
         }
         else if(pTgt->info.type == EMU_NOISYSPD_ADD)
         {
            iDeltaR = iDeltaR * ((int) (pTgt->pNoise[j] + 0.5f)) / ((int) 100);
            iDeltaR += shDeltaR;
         }
         else
         {
            iDeltaR = (int) floor( pow(2.0, 20.0 + ((double) pTgt->pNoise[j])/20.0) + 0.5 );
            iDeltaR /= ev;
            iDeltaR *= (int) shDeltaR;
            iDeltaR >>= 10;
         }
            
         dxPix = iDeltaR * cosLUT;
         dxPix /= pEmu->width;
         dxPix >>= scale;
         dxPix += pTgt->pFracDX[j];
         saveDXPix = dxPix;
         dxPix >>= 4;
         
         dyPix = iDeltaR * sinLUT;
         dyPix /= pEmu->height;
         dyPix >>= scale;
         dyPix += pTgt->pFracDY[j];
         saveDYPix = dyPix;
         dyPix >>= 4;
         
         if((pEmu->recOn) && ((i+j) < pTgt->nBufLen))
         {
            pTgt->pdDX[i+j] = ((double) dxPix) / ((double) frameDur);
            pTgt->pdDX[i+j] *= (1000.0 / alphaX);
            pTgt->pdDY[i+j] = ((double) dyPix) / ((double) frameDur);
            pTgt->pdDY[i+j] *= (1000.0 / alphaY);
         }
         
         // here we save the fractional pixel displacement (scaled by 16) in X and Y for the next update frame
         pTgt->pFracDX[j] = saveDXPix - (dxPix << 4);
         pTgt->pFracDY[j] = saveDYPix - (dyPix << 4);
      }
      
      ++(pTgt->nFrames);
   }
   else if(pEmu->recOn)
   {
      // special case: target OFF during recorded portion of trial. All target dot velocities are zero and must be 
      // stored. (While XY targets move while off, that's done by accumulating the target displacement while off
      // and then updating it in one go once it comes on. This has a "bad" effect on the noisy dots targets!)
      i = pTgt->nFrames;
      i *= pTgt->info.nDots;
      for(j=0; j<pTgt->info.nDots; j++)
      {
         if((i+j) < pTgt->nBufLen)
         {
            pTgt->pdDX[i+j] = 0.0;
            pTgt->pdDY[i+j] = 0.0;
         }
      }
      ++(pTgt->nFrames);
   }
}

/**
 Update the state of the specified RMVideo noisy-dots target for the current display frame. This method emulates 
 RMVideo RMV_RANDOMDOTS targets for data file versions >= 12 (Maestro 2.1.3 or later).
 
 Unlike the XYScope noisy-dots target emulation, this keeps track of target dot positions and detects when dots are
 recycled by random repositioning them in the target window. When this happens, the dot's velocity is NaN. In addition,
 the RMV_RANDOMDOTS target has "percent coherence" and "finite dotlife" features that could be enabled. This method
 must emulate those features as well to get the dot velocities right. As a bonus, we are able to also report the
 position trajectories of every dot in the target -- which we cannot do for XYScope.
    
 @param pTgt Pointer to the target object.
 @param isOn TRUE if the specified target is ON for the current update frame; else it is OFF.
 @param dhPat, dvPat The target's H,V pattern displacement for the current update frame, in deg subtended at the eye.
 @param dhWin, dvWin The target's H,V window displacement for the current update frame, in deg subtended at the eye.
 This is required in case the target's RMV_WRTSCREEN flag is set, in which case the pattern displacement is specified
 WRT the screen rather than the target window.
*/
void updateRMVTgt(PNOISYTGT pTgt, BOOL isOn, double dhPat, double dvPat, double dhWin, double dvWin)
{
   int i, offset;
   double dPatVecAmpl, dPatVecTheta, dNoise, dLog2Fac, dTest, dDir, dAmp, dVal, dDotDeltaX, dDotDeltaY;
   float fOuterHalfW, fOuterHalfH, fDotLifeDelta, fx, fy, fRem;
   BOOL bIsDirNoise, bIsSpdLog2, bWrtScreen, bEnaCoh, bEnaDotLife, bWasDotLocRandomized;
   PNOISYDOTSEMU pEmu;
   
   pEmu = &m_Emulator;
   
   // outer half-width and half-height of aperture's bounding rectangle
   fOuterHalfW = pTgt->info.fOuterW / 2.0f;
   fOuterHalfH = pTgt->info.fOuterH / 2.0f;

   // calculate the polar form of pattern velocity vector
   dPatVecAmpl = sqrt(dhPat * dhPat + dvPat * dvPat);
   dPatVecTheta = TODEGREES(atan2(dvPat, dhPat));
   
   // choose new random noise factor for each dot whenever the noise update interval expires. We do this even if the 
   // target is off and/or not moving!
   pTgt->tUntilUpdate -= (float) pEmu->rmvFP;
   if(pTgt->tUntilUpdate <= 0.0f)
   {
      pTgt->tUntilUpdate += (float) pTgt->info.updIntv;
      for(i = 0; i < pTgt->info.nDots; i++)
      {
         dNoise = urng_generate(&(pTgt->rngNoise));            // (0..1)
         dNoise *= 2.0 * ((double) pTgt->info.level);          // (0..2N), where N is the noise range limit
         dNoise -= (double) pTgt->info.level;                  // (-N..N)
         pTgt->pNoise[i] = (float) dNoise;
      }
   }

   bIsDirNoise = (pTgt->info.type == EMU_NOISYDIR);
   bIsSpdLog2 = (pTgt->info.type == EMU_NOISYSPD_MUL);
   bWrtScreen = ((pTgt->info.iFlags & RMV_F_WRTSCREEN) != 0);
   bEnaCoh = (pTgt->info.iPctCoherent < 100);
   bEnaDotLife = (pTgt->info.fDotLife > 0.0f);
   
   // if finite dotlife enabled, determine the change in dotlife for this update -- either elasped time in ms or 
   // distance travelled in degrees. We do this even if the target is off.
   // NOTE: Technically speaking, this implementation is incorrect when dotlife is in distance travelled. Since per-dot
   // noise is enabled, each dot may move a different distance when the noise is in speed rather than direction.
   // However, this reflects the current implementation in RMVideo!
   fDotLifeDelta = 0.0f; 
   if(bEnaDotLife) 
   { 
      if((pTgt->info.iFlags & RMV_F_LIFEINMS) != 0) fDotLifeDelta = (float) pEmu->rmvFP;
      else fDotLifeDelta = dPatVecAmpl;
   }

   // this factor is the expected value of 2^X, where X is a uniform r.v chosen over (-N..N). It is needed only in 
   // the implementation of multiplicative per-dot speed noise: Rdot = (Rpat * 2^X) / E(2^X).
   dLog2Fac = 1.0;
   if(bIsSpdLog2)
   {
      dLog2Fac = pow(2.0, (double) pTgt->info.level) - pow(2.0, (double) -pTgt->info.level);
      dLog2Fac /= 2 * ((double) pTgt->info.level) * log(2.0);
   }

   // UPDATE INDIVIDUAL DOT POSITIONS:
   offset = pTgt->nFrames;
   offset *= pTgt->info.nDots;
   for(i = 0; i < pTgt->info.nDots; i++)
   {
      // flag gets set if dot position was randomized on this frame
      bWasDotLocRandomized = FALSE;
      
      // if coherence feature in play, on each update we randomly select a percentage of dots to be randomly 
      // repositioned w/in target window
      if(bEnaCoh) 
      { 
         dTest = urng_generate(&(pTgt->rngDots)) * 100.0; 
         if(dTest >= (double) pTgt->info.iPctCoherent)
         {
            bWasDotLocRandomized = TRUE;
            rmvRandomizeDotPos(pTgt, i);
         }
      }

      // if finite dotlife in play, decrement the dot's current lifetime and randomly repos dot if its lifetime expired
      if(bEnaDotLife) 
      { 
         pTgt->pDotLives[i] -= fDotLifeDelta;
         if(pTgt->pDotLives[i] < 0.0f)
         {
            pTgt->pDotLives[i] = pTgt->info.fDotLife;
            if(!bWasDotLocRandomized)
            {
               bWasDotLocRandomized = TRUE;
               rmvRandomizeDotPos(pTgt, i);
            }
         }
      }

      // if dot was randomly repositioned and we're in the recorded portion of the trial, then dot velocities are NaN.
      if(bWasDotLocRandomized && pEmu->recOn && (offset + i < pTgt->nBufLen))
      {
         pTgt->pdDX[offset + i] = pEmu->dQuietNaN;
         pTgt->pdDY[offset + i] = pEmu->dQuietNaN;

         pTgt->pdXPos[offset + i] = pTgt->pX[i];
         pTgt->pdYPos[offset + i] = pTgt->pY[i];
      }

      // if dot was not already randomly repositioned above, then move it appropriately.
      if(!bWasDotLocRandomized) 
      { 
         // compute change in dot position during the current update frame IAW noise algorithm selected
         dDotDeltaX = 0.0;
         dDotDeltaY = 0.0;
         if(bIsDirNoise)
         {
            // for dir noise, offset pat vel theta by noise factor in deg; then calc Cartesian cmpts
            dDir = dPatVecTheta + pTgt->pNoise[i];
            dDotDeltaX = dPatVecAmpl * cos(TORADIANS(dDir));
            dDotDeltaY = dPatVecAmpl * sin(TORADIANS(dDir));
         }
         else if( !bIsSpdLog2 ) 
         { 
            // for additive speed noise, offset pat vel R by a pct based noise factor
            dAmp = dPatVecAmpl * (pTgt->pNoise[i] / 100.0f);
            dAmp += dPatVecAmpl;
            dDotDeltaX = dAmp * cos(TORADIANS(dPatVecTheta));
            dDotDeltaY = dAmp * sin(TORADIANS(dPatVecTheta));
         }
         else
         { 
            // (as of v2.1.3) for multiplicative speed noise, Rdot = (R*2^X)/E, where E is the mean of 2^X when X is a
            // uniform r.v. in (-N..N)
            dAmp = dPatVecAmpl*pow(2.0, pTgt->pNoise[i]);
            dAmp /= dLog2Fac;
            dDotDeltaX = dAmp * cos(TORADIANS(dPatVecTheta));
            dDotDeltaY = dAmp * sin(TORADIANS(dPatVecTheta));
         }
         
         // update dot position relative to target center. (as of v2.5.2) if target pattern displacement is specified
         // WRT screen rather than target window, then we must subtract the window displacement during this update to
         // convert dot position to the target frame of reference.
         fx = pTgt->pX[i] + ((float) dDotDeltaX);
         fy = pTgt->pY[i] + ((float) dDotDeltaY);
         if(bWrtScreen)
         {
            fx -= dhWin;
            fy -= dvWin;
         }
         
         // The code below implements an algorithm for recycling a dot that has just moved out of "bounds", ie,
         // beyond the outer bounds of the aperture.  The idea here is to relocate the dot in a sensible way so that
         // the target acts like a window on a random-dot pattern of infinite extent.  Dots are "recycled" when they
         // leave the aperture's bounding rectangle (which is larger than the visible window for all apertures
         // except "rect"!) If the dot has just advanced past the right edge of the rectangle by X degrees, then the
         // algorithm here will "recycle" the dot X degrees left of the window's left edge, with the y-coord
         // randomized since we don't want the same pattern to "wrap" around the window edges.

         if(fabsf(fx) > fOuterHalfW)
         {
            bWasDotLocRandomized = TRUE;
            fRem = fmodf(fabsf(fx) - fOuterHalfW, fOuterHalfW);
            if((fx - pTgt->pX[i]) > 0)
               fx = -fOuterHalfW + fRem;
            else
               fx = fOuterHalfW - fRem;

            dVal = urng_generate(&(pTgt->rngDots)) - 0.5;
            fy = (float) (dVal * fOuterHalfH * 2.0);
         }
         else if(fabsf(fy) > fOuterHalfH)
         {
            bWasDotLocRandomized = TRUE;
            fRem = fmodf(fabsf(fy) - fOuterHalfH, fOuterHalfH);
            if((fy - pTgt->pY[i]) > 0)
               fy = -fOuterHalfH + fRem;
            else
               fy = fOuterHalfH - fRem;

            dVal = urng_generate(&(pTgt->rngDots)) - 0.5;
            fx = (float) (dVal * fOuterHalfW * 2.0);
         }

         // if we're in the recorded portion of the trial, save the dot's instantaneous velocity during this frame in
         // deg/sec, and its position in deg. If dot was recycled upon exiting target's bounding rect, velocity is NaN.
         if(pEmu->recOn && (offset + i < pTgt->nBufLen))
         {
            if(bWasDotLocRandomized)
            {
               pTgt->pdDX[offset + i] = pEmu->dQuietNaN;
               pTgt->pdDY[offset + i] = pEmu->dQuietNaN;
            }
            else
            {
               // when target pattern motion is specified WRT screen, then we want to report dot velocities WRT the
               // screen as well. Otherwise, we report velocities WRT the target center.
               if(!bWrtScreen)
               {
                  dDotDeltaX = (double) (fx - pTgt->pX[i]);
                  dDotDeltaY = (double) (fy - pTgt->pY[i]);
               }
               pTgt->pdDX[offset + i] = dDotDeltaX * 1000.0 / pEmu->rmvFP;
               pTgt->pdDY[offset + i] = dDotDeltaY * 1000.0 / pEmu->rmvFP;
            }
            
            // dot position trajectories are ALWAYS reported WRT target center, regardless the bWrtScreen flag!
            pTgt->pdXPos[offset + i] = fx;
            pTgt->pdYPos[offset + i] = fy;
         }
         
         // finally, save the new position of the dot
         pTgt->pX[i] = fx;
         pTgt->pY[i] = fy;
      }
   }

   if(pEmu->recOn) ++pTgt->nFrames;
}
