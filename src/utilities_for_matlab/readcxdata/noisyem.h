//===================================================================================================================== 
//
// noisyem.h : Constants and other declarations for NOISYEM.C
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(NOISYEM_H__INCLUDED_)
#define NOISYEM_H__INCLUDED_

#include "mex.h"
#include "wintypes.h"                     // some typical Windows typedefs that we need 
#include "cxobj_ifc_mex.h"                // common Maestro/CXDRIVER object definitions

                                       
#define EMU_NOISYDIR       0
#define EMU_NOISYSPD_ADD   1
#define EMU_NOISYSPD_MUL   2

typedef struct noisyTgtInfo            // defining information for noisy-dots target
{
   int type;                           // EMU_NOISYDIR, EMU_NOISYSPD_ADD, or EMU_NOISYSPD_MUL
   int level;                          // noise level N (interpretation depends on noise type)
   int updIntv;                        // noise update interval in integral # of milliseconds
   int nDots;                          // number of dots in target
                                       // applicable to RMVideo RMV_RANDOMDOTS target only
   int iFlags;                         // target flags
   int iPctCoherent;                  // % coherence [0..100] 
   float fDotLife;                     // finite dot life (0=infinite)
   int iSeed;                          // per-target seed for RNGs
   float fOuterW;                      // width of outer bounding rect in deg 
   float fOuterH;                      // height of outer bounding rect in deg
} NOISYTGTINFO, *PNOISYTGTINFO;

//===================================================================================================================== 
// "Public" functions defined in this module
//===================================================================================================================== 
void initNoisyDotsEmulator(BOOL isXY, int v, int len, int d, int w, int h, DWORD seed, double rmvFP);
void releaseNoisyDotsEmulator();
BOOL addNoisyDotsTarget(int tgt, PNOISYTGTINFO pInfo);
BOOL isNoisyDotsEmulatorEnabled();
void startNoisyDotsUpdate(int tick, int recTick, int xyFP);
void updateNoisyDotsTarget(int tgt, BOOL isOn, double dhPat, double dvPat, double dhWin, double dvWin);
void setNoisyDotsResults(mxArray* pOut);

#endif   // !defined(NOISYEM_H__INCLUDED_)



