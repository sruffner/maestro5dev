//===================================================================================================================== 
//
// cxscope.h : Declaration of ABSTRACT hardware interface CCxScope, and "null" placeholder implementation CCxNullScope. 
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(CXSCOPE_H__INCLUDED_)
#define CXSCOPE_H__INCLUDED_

#include "device.h"                    // CDevice -- base class for CNTRLX device interfaces 
#include "util.h"                      // for utility classes like CFPoint & CRand16
#include "cxobj_ifc.h"                 // for XY scope target definition struct XYPARMS and related constants


//===================================================================================================================== 
// Declaration of ABSTRACT class CCxScope
//===================================================================================================================== 
//
class CCxScope : public CDevice
{
//===================================================================================================================== 
// CONSTANTS & DEFINITIONS
//===================================================================================================================== 
public:
   static const int MAX_TARGETS = 32;        // maximum # targets that can run simultaneously on XY scope device

protected:
   static const int NUMTARGTYPES = 14;       // supported "device-level" target types:
   static const WORD NO_TARGET = 0;          //    the "no target defined" placeholder
   static const WORD DOTARRAY = 1;           //    rectangular dot array or point target (no target pattern)
   static const WORD FULLSCREEN = 2;         //    full-screen random-dot pattern (no target window)
   static const WORD RECTWINDOW = 3;         //    movable rect window on a movable full-screen random-dot pattern
   static const WORD RECTHOLE = 4;           //    movable rect hole in a movable full-screen random-dot pattern
   static const WORD ANNULUS = 5;            //    movable rect annulus on a movable full-screen random-dot pattern
   static const WORD STATICANNU = 6;         //    optimized version of RECTHOLE or ANNULUS that does not move at all
   static const WORD OPTRECTWIN = 7;         //    movable random-dot pattern restricted to movable rect window -- this 
                                             //       optimized tgt is more efficient than RECTWINDOW
   static const WORD DOTLIFEWIN = 8;         //    same as OPTRECTWIN, but dot life is limited
   static const WORD OPTICFLOW = 9;          //    simulates an optical flow field of dots (circular bounds)
   static const WORD ORIBAR = 10;            //    oriented rect bar or line of randomly arranged dots (no tgt pattern) 
   static const WORD DL_NOISEDIR = 11;       //    same as DOTLIFEWIN, but dot directions are noisy
   static const WORD OPTCOHERENT = 12;       //    similar to OPTRECTWIN, but with percent coherence
   static const WORD DL_NOISESPEED = 13;     //    same as DOTLIFEWIN, but dot speeds are noisy

   struct Parameters                         // "device-level" configuration and target information (pos info in pixels 
   {                                         // [0..MAX_PIX], with screen center at (CTR_PIX,CTR_PIX)):
      DWORD dwDotSeed;                       //    seed used in generation of targets' random-dot textures
      WORD  wWidthMM;                        //    display width in mm
      WORD  wHeightMM;                       //    display height in mm
      WORD  wDistMM;                         //    distance from screen to subject's eye, in mm
      WORD  wNumTargets;                     //    #targets currently defined
      WORD  wDelayPerDot;                    //    dot draw cycle delay prior to turning "on" each dot, and the "on" 
      WORD  wOnTimePerDot;                   //    duration, in dotter board clock cycles (depends on dotter board)
      WORD  wFiller[2];                      //    so that arrays below start on 4-byte boundary
                                             //    for each target:
      WORD  wType[MAX_TARGETS],              //       the target
            wNumDots[MAX_TARGETS],           //       number of dots in target texture
            wRectR[MAX_TARGETS],             //       right, left, top & bottom edges of target "window"; dot pattern 
            wRectL[MAX_TARGETS],             //       is visible inside or outside this window (depending on tgt type). 
            wRectT[MAX_TARGETS], 
            wRectB[MAX_TARGETS], 
            wOuterR[MAX_TARGETS],            //       defn of "outer" rectangular window for annular tgt types
            wOuterL[MAX_TARGETS], 
            wOuterT[MAX_TARGETS], 
            wOuterB[MAX_TARGETS]; 
   };

   struct UpdateRec                          // "device-level" per-target motion update record:
   {
      short shWindowH, shWindowV;            //    pos change of target window, in pixels, WRT screen
      short shPatternH, shPatternV;          //    pos change of target pattern, in pixels, WRT target window
      short shNumReps;                       //    # times target should be "refreshed" during frame update
   };

   // !!! IMPORTANT !!! Some XY scope tgt types do not fit the parameterizations implied in the CCxScope::Parameters 
   // and CCxScope::UpdateRec_Dev structures.  Here we list the exceptions:
   //    DOTARRAY:   wRectR      ==> desired width of dot array, in pixels.
   //                wRectL      ==> dot spacing in pixels, both horizontally & vertically (often 0, for point tgt)
   //    DOTLIFEWIN: wOuterR     ==> dot "lifetime" (arbitrary units)
   //                shNumReps   ==> upper byte = per-refresh decrement in dot life (arbitrary units); lower byte = 
   //                                #times target should be refreshed per update.  each limited to [0..255]
   //    OPTCOHERENT:wOuterL     ==> percent coherence, an integer in [0..100].
   //    OPTICFLOW:  wRectR      ==> inner radius of flow field in deg/100
   //                wRectL      ==> outer radius of flow field in deg/100
   //                wRectT      ==> 1024 * (dist to eye) / (width of XY scope display)
   //                wRectB      ==> 1024 * (dist to eye) / (height of XY scope display)
   //                wOuterR     ==> H-coord of focus of expansion, initially CTR_PIX
   //                wOuterL     ==> V-coord of FOE, initially CTR_PIX
   //                shWindowH,V ==> change in pos of the flow field's FOE, in pixels
   //                shPatternH  ==> velocity scale factor * 2^M, where M is set to facilitate integer-only calcs  
   //                shPatternV  ==> the value of M
   //    ORIBAR:     wRectR      ==> width of bar in vertical orientation, in pixels 
   //                wRectL      ==> height of bar in vertical orienation, in pixels 
   //                wRectT      ==> the drift axis angle, in deg CCW [0..360) 
   //    DL_NOISEDIR and DL_NOISESPEED:
   //                wOuterR     ==> dot "lifetime" (arbitrary units)
   //                wOuterL     ==> noise range N.  For DL_NOISEDIR, N is an angular offset in integer deg [0..180].
   //                                Each time noise update intv expires, an offset direc is randomly chosen from 
   //                                [-N:N] for each dot in tgt.  This offset is added to the pattern direct to get 
   //                                each dot's direction for subsequent frames.
   //                                For DL_NOISESPEED, there are now (as of Maestro v2.1.3) two choices for speed 
   //                                noise. wOuterB == 0 selects the original additive noise. Here, N is an integer 
   //                                percentage in 1% increments, in [0..300]. Each time noise update intv expires, 
   //                                each tgt dot is assigned a random offset% P in [-N:N]. The dot's radial 
   //                                displacement in pixels is then R + P*R/100, where R is the nominal radial 
   //                                displacement of the target dot pattern. wOuterB != 0 selects a form of 
   //                                multiplicative noise. In this case, N is an integer exponent in [1..7]. When 
   //                                the noise update intv expires, each tgt dot is assigned a random exponent X 
   //                                uniformly chosen from [-N:N] (in increments of 0.05). The dot's radial 
   //                                displacement is then (R * 2^X) / ((2^N - 2^(-N))/(2*N*ln(2))), where the 
   //                                divisor is the expected value of 2^X when X is a uniform R.V. over [-N:N].
   //                wOuterT     ==> Noise update interval M.  Dot noise is randomly regenerated every M ms.
   //                wOuterB     ==> For DL_NOISESPEED only, if this is nonzero, multiplicative noise selected; else 
   //                                the original additive %-age noise algorithm is used.
   //                shPatternH  ==> radial cmpt R of pattern pos change expressed in POLAR coords, in screen mm, scaled
   //                                by 2^10 if R>=0.1, 2^16 otherwise.
   //                shPatternV  ==> theta of pattern pos change expressed in POLAR coords, in deg/10.
   //                shNumReps   ==> upper byte = per-update decrement in dot life (arbitrary units); lower byte = 
   //                                #times target should be refreshed per update.  each limited to [0..255]

   static const double XYDEV_TIMEOUT;        // max time we'll wait for XY scope dev to be "ready" for next cmd (us)

private:
   static const int MIN_DISTTOEYE = 100;     // minimum distance to eye (mm)
   static const int MIN_DIMENSION = 50;      // minimum display width or height (mm)
   static const int MAX_TRIGLEN = 255;       // max trigger length (delay + "ON" time) (dotter board clock cycles)
   static const int MAX_TRIGDEL = 15;        // max delay to trigger "ON" (dotter board clock cycles)
   static const int MIN_UPDATEINTV = 2;      // minimum and maximum XY scope frame update intervals (ms)
   static const int MAX_UPDATEINTV = 256;
   static const WORD MAX_PIX = 65535;         // max "pixel" coord on XY scope display
   static const WORD CTR_PIX = 65535 / 2;     // screen center corresponds to middle of pixel range



//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   Parameters  m_Parameters;                 // current "device-level" configuration and target information 
   UpdateRec   m_NextUpdate[MAX_TARGETS];    // target update records for the next display frame
   
private:
   int         m_iDistToEye;                 // distance from eye to center of XY scope display (mm)
   int         m_iWidth;                     // width of visible display (mm)
   int         m_iHeight;                    // height of visible display (mm)
   int         m_iDrawDelay;                 // delay to rising edge of "ON" pulse, and dur of pulse.  sum of these 
   int         m_iDrawDur;                   // is the "dot draw cycle".  units = dotter board clock ticks. 
   CFPoint     m_DegToPix;                   // conversion factors for H,V cmpts: deg --> pixels.
   BOOL        m_bAutoSeed;                  // if TRUE, we randomly choose a new seed value for generating rand-dot 
   DWORD       m_dwFixedSeed;                // patterns at the start of animation; else, we use the fixed seed.

   CRand16     m_randGen;                    // pseudorandom# generator used to generate seeds for dot drawing

   CFPoint     m_FracPixWin[MAX_TARGETS];    // "fractional pixel displacements" due to truncation, carried over for 
   CFPoint     m_FracPixPat[MAX_TARGETS];    // inclusion in the target displacements for the next display frame


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxScope( const CCxScope& src );                      // no copy constructor or assignment operator defined
   CCxScope& operator=( const CCxScope& src ); 

public: 
   CCxScope(                                             // constructor 
      const CDevice::DevInfo& devInfo, int iDevNum );
   virtual ~CCxScope();                                  // destructor 


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   int RTFCNDCL GetMinUpdateInterval() { return( MIN_UPDATEINTV ); }
   int RTFCNDCL GetMaxUpdateInterval() { return( MAX_UPDATEINTV ); }
   int RTFCNDCL GetNumTargets()        { return( int(m_Parameters.wNumTargets) ); }
   int RTFCNDCL GetMaxTargets()        { return( MAX_TARGETS ); }


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:
   BOOL RTFCNDCL Init() {ClearTargets(); return(TRUE);}  // init XY scope dev to an idle state

   int RTFCNDCL GetDistToEye() { return(m_iDistToEye); } // retrieve current display parameters
   int RTFCNDCL GetScreenW() { return( m_iWidth ); }
   int RTFCNDCL GetScreenH() { return( m_iHeight ); }
   int RTFCNDCL GetDrawDelay() { return( m_iDrawDelay ); }
   int RTFCNDCL GetDrawDur() { return( m_iDrawDur ); }
   BOOL RTFCNDCL IsAutoSeed() { return( m_bAutoSeed ); }
   DWORD RTFCNDCL GetFixedSeed() { return( m_dwFixedSeed ); }

   DWORD RTFCNDCL GetCurrentSeed()                       // current value of seed that was or will be sent to the 
   {                                                     // XY scope controller
      return( m_Parameters.dwDotSeed );
   }

   double RTFCNDCL GetScreenW_deg();
   double RTFCNDCL GetScreenH_deg();
   
   VOID RTFCNDCL ChangeDisplay( int d, int w, int h,     // change the XY scope device's display configuration
      int iDelay, int iDur, BOOL bAutoSeed, DWORD dwSeed );

   VOID RTFCNDCL ClearTargets();                         // clear loaded target list
   BOOL RTFCNDCL AddTarget( const XYPARMS& tgt,          // append a target to the target list
               BOOL bOptimize, const CFPoint& initPos );

   BOOL RTFCNDCL Load( int iAltSeed = -1 );              // load target defns onto dev 
   BOOL RTFCNDCL Update( CFPoint* pFPtWin,               // display frame update
      CFPoint* pFPtPattern, WORD* pwTgtUpdateIntv );


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   VOID RTFCNDCL CalcConversionFactors();                // recalc deg->pix conv factors based on current disp geom
   VOID RTFCNDCL TranslateToDevCoords( CFRect& rect );   // convert rect in deg to pixels in coord sys of XY scope
   BOOL RTFCNDCL IsFullScreen( const CFRect& rect );     // does specified rect fill the entire XY scope display?

   virtual BOOL RTFCNDCL LoadParameters() = 0;           // download tgt defns & animation parameters to XY scope dev
   virtual BOOL RTFCNDCL DoFrame() = 0;                  // download tgt update records & initiate display frame update 
};




//===================================================================================================================== 
// Declaration of class CCxNullScope -- implements "no device found" placeholder for CCxScope interface
//===================================================================================================================== 
//
class CCxNullScope : public CCxScope
{
private:
   CCxNullScope( const CCxNullScope& src );              // no copy constructor or assignment operator defined
   CCxNullScope& operator=( const CCxNullScope& src ); 

public: 
   CCxNullScope( const CDevice::DevInfo& devInfo, int iDevNum ) : CCxScope( devInfo, iDevNum ) {}
   virtual ~CCxNullScope() {}

   BOOL RTFCNDCL Init() { SetDeviceError( CDevice::EMSG_DEVNOTAVAIL ); return( FALSE ); }

protected:
   BOOL RTFCNDCL MapDeviceResources() { return( FALSE ); }
   VOID RTFCNDCL UnmapDeviceResources() {}

   BOOL RTFCNDCL LoadParameters() { SetDeviceError( CDevice::EMSG_DEVNOTAVAIL ); return( FALSE ); }
   BOOL RTFCNDCL DoFrame() { SetDeviceError( CDevice::EMSG_DEVNOTAVAIL ); return( FALSE ); }
};


#endif   // !defined(CXSCOPE_H__INCLUDED_)

