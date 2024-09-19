//=====================================================================================================================
//
// cxsettings.cpp : Implementation of class CCxSettings, encapsulating MAESTRO application settings that are serialized
//                  in the MAESTRO "experiment" document (CCxDoc).
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxSettings is a "catch-all" object that holds the current values of all "persistent" application-level settings,
// such as the video display configuration, fixation requirements, and reward options.  By storing such settings in an
// object that is serialized with the MAESTRO experiment document (CCxDoc), the user does not have to reenter all the
// settings as appropriate to the experiment -- saving setup time.
//
// CCxSettings is not a MAESTRO data object like a target or trial object.  There exists only one CCxSettings object in
// each CCxDoc, and it is NOT part of the document's "object tree".  CCxDoc instantiates and provides access to the
// settings object.  Various views and control panel dialogs in the MAESTRO GUI display/modify the settings.
//
// ==> Description of application settings available in CCxSettings.
//
// 1) Video display configuration parameters.  The two modifiable target types in MAESTRO are realized on two kinds of
// video display hardware.  XYScope targets (type CX_XYTARG) are displayed on an XY oscilloscope driven by an in-house
// "dotter" board which, in turn, is controlled by a DSP card in the computer.  RMVideo targets (CX_RMVTARG) are
// realized on a high-resolution computer monitor driven by a modern video card controlled by the OpenGL application
// RMVideo, which runs on a separate Linux workstation and communicates with MAESTRO over a private, dedicated Ethernet
// link.  The user can adjust several parameters associated with each kind of video display:  display geometry,
// RMVideo display background color, and XY scope timing.  All such display parameters are maintained in CCxSettings.
//
// 2) Fixation requirements and reward options.  Included here are fixation requirements for ContMode:  horizontal and
// vertical fixation accuracy, fixation duration (length of time subject must maintain fixation to "earn" a reward),
// and pulse lengths for reward pulses 1 & 2.  Some of these parameters can apply to TrialMode as well.  The fixation
// duration sets the time between rewards when the "mid-trial rewards" feature is engaged in a trial.  In addition, if
// the "trial reward pulse override" is enabled, then the two reward pulse length settings override similar settings in
// each trial's definition.  (Note that variable reward pulse lengths are available only on rigs equipped with the
// required "adjustable reward" device.)
//
// A "global reward size multiplier" was added in V4.1.3 (May 2020) to globally scale any delivered reward pulse 
// without having to, e.g., change individual trial definitions. Its main purpose is to increase the liquid rewards
// delivered toward the end of an experimental session. Unlike the other application settings in CCxSettings, this
// parameter is NOT persisted in the experiment document. It is always initialized to 1.0 at application startup.
//
// To assist in training intractable animals, rewards can be randomly withheld IAW a "variable ratio" (VR) setting.
// A VR of N means that 1 of every N earned rewards is randomly withheld from the subject.  Allowed range of N is
// [1..10], where 1 disables random withholding.  Associated with this feature is the "audio" reward option.  Whenever
// a reward is earned, a brief tone of selected duration is played on a simple speaker (if the rig is so equipped) --
// whether or not the reward is randomly withheld.  Allowed choices for the tone's duration are limited to 0 (off),
// 100, 200, 300, 400, 500, and 1000 msecs.
//
// Finally, if the "reward beep" setting is enabled, MAESTRODRIVER will play a "beep" on the PC speaker whenever a
// physical reward is actually delivered to the animal (this beep has nothing to do with the audio reward option!).
//
// 3) Velocity stabilization window length. This parameter, originally introduced in Maestro v2.7.0, sets the window
// length for a sliding-average of H and V eye position to smooth out the effects of signal noise on the velocity
// stabilization feature. Persistence of this parameter began in Maestro v4.1.1.
//
// ==> Importing defn from an ASCII text file.
// MAESTRO succeeds the cross-platform cntrlxUNIX/PC application, in which the GUI was hosted on a UNIX workstation
// ("cntrlxUNIX") and the hardware controller resided on a WindowsNT PC ("cntrlxPC").  In that system, the various data
// objects (targets, channel configurations, trials, etc.) could be defined in ASCII-text "definition files".  MAESTRO
// supports importing MAESTRO data objects from such definition files IAW the procedure outlined in CCxImporter.  Each
// data class provides an Import() method that takes a CStringArray and reinitializes itself IAW the definition
// contained therein.  Thus, the details of translating the cntrlxUNIX-style text definition to the MAESTRO data object
// is encapsulated in the data object itself, but the details of opening text files and interacting with the user are
// handled elsewhere.
//
// In the case of the application settings object, only the video display settings could be defined in a text file in
// cntrlxUNIX.  CCxSettings::Import() supports importing these video display settings, but it also supports importing
// the fixation/reward settings as well.  See the method description for details.
//
// REVISION HISTORY:
// 16oct2002-- Began development.  Absorbs functionality of former CCxVideoDsp, which held video display parameters.
// 06dec2002-- Added ConvertXYPixToDeg().
// 13jan2002-- Got rid of GetNextXYDotSeed().  CXDRIVER is solely responsible for auto-generation of XY dot seeds.
// 30sep2003-- Added Copy() and Import() to support importing these application settings from a cntrlxUNIX-style text
//             definition file.
// 26jan2006-- Reward pulse length range limits changed so they are the same as for a trial object.
// 24mar2006-- Mods to support RMVideo in place of VSG-based framebuffer video.  RMVideo introduced in Maestro v2.0.
//             Required introduction of schema versioning.  Current schema version is 2.  CCxSettings object stored in
//             documents created prior to this change will have no schema version -- they are treated as version 1.
// 30mar2006-- Version control via VERSIONABLE_SCHEMA does not work b/c we call Serialize directly to serialize and
//             deserialize CCxSettings.  It's too late to introduce a serialized version number, because that would
//             break all existing Maestro documents.  Instead, we abandoned the versioning idea.  The only settings
//             affected are the RMVideo background color.  When a pre2.0.0 document is deserialized, any bkg color
//             with nonzero cmpts will get "mapped" to a different color b/c the valid range is now [0..255], not
//             [0..1000].  Since the framebuffer video display has not been used much since Maestro was first
//             released, we decided this wasn't too big a deal.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
// 20sep2018-- Added RMVideo display settings that govern the size and duration of a "time sync flash" in the top-left
//             corner of the screen. This flash can be presented at the start of any and all trial segments. Using a
//             photodiode, the detected "flash" is converted to a TTL pulse that can then be timestamped by Maestro,
//             thus serving as a more precise measure of when RMVideo began drawing the first frame for the relevant
//             trial segment.
//          -- This change required some way to implement versioning without the VERSIONABLE_SCHEMA approach and 
//             without breaking existing documents. Since the first item serialized in past releases is a DWORD with
//             flag bits only defined in bits 0-2, we decided to store a version number in bits 23-16. Because I never
//             initialized m_dwFlags to 0, m_dwFlags is archived in old documents as 0xCDCDCDCX, with the defined flag 
//             bits in bits 2-0. Therefore, in documents saved prior to this change, version = 0xCD!! As of Maestro
//             4.0.0 initial release for Win10 64-bit, m_dwFlags is first initialized to 0, then bits 23-16 are set to 
//             0x01, then the defined flag bits are set to 0 initially. From now on, m_dwFlags should be archived
//             correctly...
// 25sep2018-- Eliminated the sync flash "margin" setting. Flash duration range is now [1..9], and flash spot size is
// now in mm instead of pixels, with same range of [0..50]. A spot size of 0 disables the feature.
// 14aug2019-- Added velocity stabilization window length to CCxSettings as a persisted parameter. The version # 
// stored in m_dwFlags, bits 23-16, is now 2 (see entry dtd 20sep2018).
// 10feb2020-- Regression bug fixed: The changes implemented on 14aug2019 for 4.1.1 broke the versioning scheme 
// introduced in 4.0.0. To keep things cleaner, I extract the version number and use it when de-serializing, and I
// insert it into m_dwFlags only while serializing. The version # is not maintained in m_dwFlags.
// 26may2020-- Added new paramter specifying a global reward pulse length multiplier. This parameter always defaults to
// 1.0 and -- unlike other parameters in CCxSettings -- it is NOT persisted in the experiment document.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"

#include "cxsettings.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_SERIAL( CCxSettings, CObject, 1 )


//=====================================================================================================================
// CONSTANTS INITIALIZED
//=====================================================================================================================

const DWORD CCxSettings::F_XYFIXSEED   = (DWORD)(1<<0);     // bit flag settings
const DWORD CCxSettings::F_TRIALREWOVR = (DWORD)(1<<1);
const DWORD CCxSettings::F_REWBEEPENA  = (DWORD)(1 << 2);

// this version number is stored in bits 23-16 of m_dwFlags during serialization and removed and checked during 
// deserialization. We could not use VERSIONABLE_SCHEMA b/c we had not introduced it from the beginning!
const DWORD CCxSettings::CURRVERSION = 2; 

const int CCxSettings::MINDIM             = 50;             // allowed range for display geometry parameters (mm)
const int CCxSettings::MAXDIM             = 5000;
const int CCxSettings::MINDELAY_XY        = 1;              // allowed range for XY scope timing params (100-ns ticks)
const int CCxSettings::MAXDELAY_XY        = 15;
const int CCxSettings::MINDUR_XY          = 1;
const int CCxSettings::MAXDUR_XY          = 254;
const int CCxSettings::MAXCYCLE_XY        = 255;
const int CCxSettings::MINRGB_RMV         = 0;              // allowed range for RMVideo background color specification
const int CCxSettings::MAXRGB_RMV         = 255;

const int CCxSettings::MINFIXDUR          = 100;            // allowed range for fixation duration (ms)
const int CCxSettings::MAXFIXDUR          = 10000;
const float CCxSettings::MINFIXACC        = 0.1f;           // allowed range for fixation accuracy (deg subtd at eye)
const float CCxSettings::MAXFIXACC        = 50.0f;
const int CCxSettings::MINREWLEN          = TH_MINREWLEN;   // allowed range for reward pulse lengths (ms)
const int CCxSettings::MAXREWLEN          = TH_MAXREWLEN;
const float CCxSettings::MINREWMULT       = 1.0f;           // allowed range for reward pulse multiplier
const float CCxSettings::MAXREWMULT       = 5.0f;
const int CCxSettings::MINWHVR            = 1;              // allowed range for reward withholding "variable ratio"
const int CCxSettings::MAXWHVR            = 10;
const int CCxSettings::MINAUDIOREWLEN     = 100;            // allowed range for audio reward pulse length (ms)
const int CCxSettings::MAXAUDIOREWLEN     = 1000;           // (any out-of-range value is corrected to 0 ==> "off")

const int CCxSettings::MINSYNCFLASHSZ     = 0;              // RMVideo sync flash size range (mm; 0=disabled)
const int CCxSettings::MAXSYNCFLASHSZ     = 50; 
const int CCxSettings::MINSYNCFLASHDUR    = 1;              // RMVideo sync flash duration range (#frames)
const int CCxSettings::MAXSYNCFLASHDUR    = 9;

const int CCxSettings::MIN_VSTABWIN       = 1;              // length of sliding window average for smoothing effects 
const int CCxSettings::MAX_VSTABWIN       = 20;             // of signal noise on VStab feature


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Copy ============================================================================================================
//
//    Make this application settings object a clone of the specified settings object
//
//    ARGS:       src   -- the settings object to be copied.
//
//    RETURNS:    NONE.
//
VOID CCxSettings::Copy( const CCxSettings& src )
{
   m_dwFlags = src.m_dwFlags;

   m_iDistToEyeXY = src.m_iDistToEyeXY;
   m_iWidthXY = src.m_iWidthXY;
   m_iHeightXY = src.m_iHeightXY;
   m_iDrawDelay = src.m_iDrawDelay;
   m_iDrawDur = src.m_iDrawDur;
   m_dwDotSeed = src.m_dwDotSeed;

   m_iDistToEyeRMV = src.m_iDistToEyeRMV;
   m_iWidthRMV = src.m_iWidthRMV;
   m_iHeightRMV = src.m_iHeightRMV;
   for( int i=0; i<3; i++ ) m_iBkgColor[i] = src.m_iBkgColor[i];

   m_iFixDur = src.m_iFixDur;
   m_fFixAccH = src.m_fFixAccH;
   m_fFixAccV = src.m_fFixAccV;
   m_iRewLen1 = src.m_iRewLen1;
   m_iRewLen2 = src.m_iRewLen2;
   m_fRewMult = src.m_fRewMult;
   m_iVarRatio = src.m_iVarRatio;
   m_iAudioRewLen = src.m_iAudioRewLen;

   m_iRMVSyncFlashSize = src.m_iRMVSyncFlashSize;
   m_iRMVSyncFlashDur = src.m_iRMVSyncFlashDur;

   m_iVStabWinLen = src.m_iVStabWinLen;
}


//=== Set*** ==========================================================================================================
//
//    Change the value of an individual application setting, correcting any values that are out-of-range.
//
//    ARGS:       the desired value for the parameter.
//
//    RETURNS:    the actual value taken for the parameter, range-limited or possibly unchanged.
//
BOOL CCxSettings::SetXYDotSeedFixed( BOOL bEna )
{
   if( bEna ) m_dwFlags |= F_XYFIXSEED;
   else m_dwFlags &= ~F_XYFIXSEED;
   return( bEna );
}

BOOL CCxSettings::SetTrialRewLenOverride( BOOL bEna )
{
   if( bEna ) m_dwFlags |= F_TRIALREWOVR;
   else m_dwFlags &= ~F_TRIALREWOVR;
   return( bEna );
}

BOOL CCxSettings::SetRewardBeepEnabled( BOOL bEna )
{
   if( bEna ) m_dwFlags |= F_REWBEEPENA;
   else m_dwFlags &= ~F_REWBEEPENA;
   return( bEna );
}

int CCxSettings::SetXYDistToEye( int i )
{
   m_iDistToEyeXY = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iDistToEyeXY );
}

int CCxSettings::SetXYWidth( int i )
{
   m_iWidthXY = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iWidthXY );
}

int CCxSettings::SetXYHeight( int i )
{
   m_iHeightXY = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iHeightXY );
}

int CCxSettings::SetXYDrawDelay( int i )
{
   if( i + m_iDrawDur > MAXCYCLE_XY ) i = MAXCYCLE_XY - m_iDrawDur;        // draw cycle dur + delay cannot exceed max
   m_iDrawDelay = (i<MINDELAY_XY) ? MINDELAY_XY : ((i>MAXDELAY_XY) ? MAXDELAY_XY : i);
   return( m_iDrawDelay );
}

int CCxSettings::SetXYDrawDur( int i )
{
   if( i + m_iDrawDelay > MAXCYCLE_XY ) i = MAXCYCLE_XY - m_iDrawDelay;    // draw cycle dur + delay cannot exceed max
   m_iDrawDur = (i<MINDUR_XY) ? MINDUR_XY : ((i>MAXDUR_XY) ? MAXDUR_XY : i);
   return( m_iDrawDur );
}

DWORD CCxSettings::SetFixedXYDotSeedValue( DWORD dwSeed )
{
   m_dwDotSeed = dwSeed;
   return( m_dwDotSeed );
}

int CCxSettings::SetFBDistToEye( int i )
{
   m_iDistToEyeRMV = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iDistToEyeRMV );
}

int CCxSettings::SetFBWidth( int i )
{
   m_iWidthRMV = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iWidthRMV );
}

int CCxSettings::SetFBHeight( int i )
{
   m_iHeightRMV = (i<MINDIM) ? MINDIM : ((i>MAXDIM) ? MAXDIM : i);
   return( m_iHeightRMV );
}

int CCxSettings::SetFBBkgRed( int i )
{
   m_iBkgColor[FB_RED] = (i<MINRGB_RMV) ? MINRGB_RMV : ((i>MAXRGB_RMV) ? MAXRGB_RMV : i);
   return( m_iBkgColor[FB_RED] );
}

int CCxSettings::SetFBBkgGrn( int i )
{
   m_iBkgColor[FB_GRN] = (i<MINRGB_RMV) ? MINRGB_RMV : ((i>MAXRGB_RMV) ? MAXRGB_RMV : i);
   return( m_iBkgColor[FB_GRN] );
}

int CCxSettings::SetFBBkgBlu( int i )
{
   m_iBkgColor[FB_BLU] = (i<MINRGB_RMV) ? MINRGB_RMV : ((i>MAXRGB_RMV) ? MAXRGB_RMV : i);
   return( m_iBkgColor[FB_BLU] );
}

int CCxSettings::SetFBBkgGrayscale( int i )
{
   int iLum = (i<MINRGB_RMV) ? MINRGB_RMV : ((i>MAXRGB_RMV) ? MAXRGB_RMV : i);
   for( int j = 0; j < 3; j++ ) m_iBkgColor[j] = iLum;
   return( iLum );
}

int CCxSettings::SetFixDuration( int i )
{
   m_iFixDur = (i<MINFIXDUR) ? MINFIXDUR : ((i>MAXFIXDUR) ? MAXFIXDUR : i);
   return( m_iFixDur );
}

float CCxSettings::SetFixAccH( float f )
{
   m_fFixAccH = (f<MINFIXACC) ? MINFIXACC : ((f>MAXFIXACC) ? MAXFIXACC : f);
   return( m_fFixAccH );
}

float CCxSettings::SetFixAccV( float f )
{
   m_fFixAccV = (f<MINFIXACC) ? MINFIXACC : ((f>MAXFIXACC) ? MAXFIXACC : f);
   return( m_fFixAccV );
}

int CCxSettings::SetRewardLen1( int i )
{
   m_iRewLen1 = (i<MINREWLEN) ? MINREWLEN : ((i>MAXREWLEN) ? MAXREWLEN : i);
   return( m_iRewLen1 );
}

int CCxSettings::SetRewardLen2( int i )
{
   m_iRewLen2 = (i<MINREWLEN) ? MINREWLEN : ((i>MAXREWLEN) ? MAXREWLEN : i);
   return( m_iRewLen2 );
}

float CCxSettings::SetRewardPulseMultiplier(float f)
{
   m_fRewMult = (f < MINREWMULT) ? MINREWMULT : ((f > MAXREWMULT) ? MAXREWMULT : f);
   return(m_fRewMult);
}
int CCxSettings::SetVariableRatio( int i )
{
   m_iVarRatio = (i<MINWHVR) ? MINWHVR : ((i>MAXWHVR) ? MAXWHVR : i);
   return( m_iVarRatio );
}

int CCxSettings::SetAudioRewardLen( int i )                             // any out-of-range value is set to 0, which
{                                                                       // disables the audio reward feature!
   m_iAudioRewLen = (i<MINAUDIOREWLEN || i>MAXAUDIOREWLEN) ? 0 : i;
   return( m_iAudioRewLen );
}

// If size = 0, the time sync flash feature is effectively disabled.
int CCxSettings::SetRMVSyncFlashSize(int i)
{
   m_iRMVSyncFlashSize = (i<MINSYNCFLASHSZ) ? MINSYNCFLASHSZ : ((i>MAXSYNCFLASHSZ) ? MAXSYNCFLASHSZ : i);
   return(m_iRMVSyncFlashSize);
}
int CCxSettings::SetRMVSyncFlashDuration(int i)
{
   m_iRMVSyncFlashDur = (i<MINSYNCFLASHDUR) ? MINSYNCFLASHDUR : ((i>MAXSYNCFLASHDUR) ? MAXSYNCFLASHDUR : i);
   return(m_iRMVSyncFlashDur);
}

int CCxSettings::SetVStabWinLen(int i)
{
   m_iVStabWinLen = (i < MIN_VSTABWIN) ? MIN_VSTABWIN : ((i > MAX_VSTABWIN) ? MAX_VSTABWIN : i);
   return(m_iVStabWinLen);
}

/**
 This convenience method scales the specified reward pulse length by the current reward pulse multiplier and
 rounds the result to the nearest integer.
 @param len A reward pulse length in ms.
 @return The scaled pulse length -- unchanged in the current multiplier is 1.
 */
int CCxSettings::GetScaledRewardPulseLen(int len)
{
   if(m_fRewMult == 1.0f) return(len);
   return((int)(len*m_fRewMult + 0.5f));
}

//=== ConvertXYPixToDeg ===============================================================================================
//
//    Converts XY scope pixels to deg subtended at eye.  In the XY scope coordinate system, coordinates are expressed
//    in pixels restricted to the range [0..65535], with the bottom-left corner of the screen at (0,0) and the top-rt
//    corner at (65535, 65535).  In the hardware-independent CNTRLX coordinate system, coordinates are expressed in deg
//    subtended at the eye, with the origin at (0,0).  In the case of the XY scope, it is assumed that the subject's
//    line-of-sight passes through the center of the scope screen at a perpendicular angle -- so (32767,32767) in pix
//    corresponds to (0,0) deg.  This method uses the current XY scope display geometry to convert any arbitrary coord
//    in pixels to deg.
//
//    ARGS:       iPix  -- [in] a coord or measurement in the XY scope coord system, in pixels.
//                bHor  -- [in] TRUE if coord is along H axis; else V axis.
//
//    RETURNS:    the same coord translated into deg subtended at eye, based on curr XY display settings.
//
double CCxSettings::ConvertXYPixToDeg( int iPix, BOOL bHor ) const
{
   ASSERT( m_iDistToEyeXY > 0 );                               // this should ALWAYS be the case

   iPix = (iPix < 0) ? 0 : ((iPix > 65535) ? 65535 : iPix);    // pix restricted to [0..65535]
   double dRes = double(iPix)/65536.0;                         // pixels --> mm on display screen
   dRes *= double( bHor ? m_iWidthXY : m_iHeightXY );
   dRes = ::atan2( dRes, double(m_iDistToEyeXY) );             // mm --> radians subtended at eye
   dRes *= 180.0/3.141593;                                     // radians --> deg
   return( dRes );
}


//=== RestoreDefaultVideoSettings =====================================================================================
//
//    Restores those settings related to the MAESTRO video display configuration to their default values.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxSettings::RestoreDefaultVideoSettings()
{
   m_dwFlags &= ~F_XYFIXSEED;
   m_iDistToEyeXY = 800;
   m_iWidthXY = 300;
   m_iHeightXY = 300;
   m_iDrawDelay = 10;
   m_iDrawDur = 1;
   m_dwDotSeed = 0;
   m_iDistToEyeRMV = 800;
   m_iWidthRMV = 400;
   m_iHeightRMV = 300;
   for( int i = 0; i < 3; i++ ) m_iBkgColor[i] = 0;

   m_iRMVSyncFlashDur = 1;
   m_iRMVSyncFlashSize = 0;
}


//=== RestoreDefaults =================================================================================================
//
//    Restores all application settings to default values.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSettings::RestoreDefaults()
{
   RestoreDefaultVideoSettings();

   m_dwFlags &= ~(F_TRIALREWOVR|F_REWBEEPENA);
   m_iFixDur = 1500;
   m_fFixAccH = m_fFixAccV = 2.0f;
   m_iRewLen1 = m_iRewLen2 = 25;
   m_fRewMult = 1.0f;
   m_iVarRatio = 1;
   m_iAudioRewLen = 0;

   m_iVStabWinLen = MIN_VSTABWIN;

   m_randNumGen.SetSeed( 0x12345678 );
}


//=== Serialize [base override] =======================================================================================
//
//    Handles reading/writing application settings from/to a disk file via a serialization archive.
//
//    NOTE:  As of Maestro 2.0.0, old VSG FB video replaced with RMVideo server. RMVideo bkg color cmpts are restricted
//    to the range [0..255] instead of [0..1000].  Since we could not introduce schema versioning -- VERSIONABLE_SCHEMA
//    approach doesn't work when an object is serialized by calling Serialize() directly, as is the case for
//    CCxSettings --, we decided to leave it be.  SO the bkg color in pre-2.0.0 documents will not migrate correctly
//    unless all three components are zero.
//
//    As of Maestro 4.0.0, introduced a version number in bits 23-16 of m_dwFlags. This was needed in order to 
//    introduce new RMVideo display parameters governing a "time sync flash". The version number is kept in m_dwFlags. 
//    Prior to this change, the version number was 0xCD because I never initialized m_dwFlags to 0. 
//
//    IMPORTANT: (1) The version number is NOT ACTUALLY PART OF m_dwFlags! It is inserted into m_dwFlags during 
//    serialization, and removed and checked  during deserialization. (2) Since pre-4.0 documents will have a 
//    version # of 0xCD as explained above, take care with version checking. Using "version >= 1" by itself is 
//    INCORRECT, since 0x0CD passes that test! (3) The reward pulse multiplier (introduced in Maestro 4.1.3) is NOT
//    serialized. It always defaults to 1.0
//
//    Version 0xCD: Pre-Maestro 4.0.0 status. To migrate, all RMVideo time sync flash parameters are set to 0.
//    Version 0x01: (as of Maestro 4.0.0) Added RMVideo time sync flash duration and spot size settings.
//    Version 0x02: (as of Maestro 4.1.1) Added velocity stabilization window length.
//
//    ARGS:       ar -- [in] the serialization archive.
//
//    RETURNS:    NONE.
//
//    THROWS:     -- The CArchive object can throw CMemoryException, CArchiveException, or CFileException
//
void CCxSettings::Serialize( CArchive& ar )
{
   CObject::Serialize( ar );                                                     // serialize base class stuff first

   int i;
   float f;
   if ( ar.IsStoring() )                                                         // write to archive:
   {
      // insert version number into m_dwFlags when storing -- it's not actually part of the bit flags!
      DWORD dw = m_dwFlags | (CURRVERSION << 16);

      ar << dw << m_dwDotSeed;
      ar << m_iDistToEyeXY << m_iWidthXY << m_iHeightXY << m_iDrawDelay << m_iDrawDur;
      ar << m_iDistToEyeRMV << m_iWidthRMV << m_iHeightRMV;
      for( i = 0; i < 3; i++ ) ar << m_iBkgColor[i];
      ar << m_iFixDur << m_fFixAccH << m_fFixAccV;
      ar << m_iRewLen1 << m_iRewLen2 << m_iVarRatio << m_iAudioRewLen;
      ar << m_iRMVSyncFlashSize << m_iRMVSyncFlashDur << m_iVStabWinLen;
   }
   else                                                                          // read from archive, auto-correcting
   {                                                                             // each setting as it is read in...
      // remove version number from the bit flags. We only need it to correctly deserialize... see below.
      ar >> m_dwFlags;
      DWORD version = (m_dwFlags >> 16) & 0x0FF;
      m_dwFlags &= (F_XYFIXSEED | F_TRIALREWOVR | F_REWBEEPENA);

      ar >> m_dwDotSeed;
      ar >> i; SetXYDistToEye( i );
      ar >> i; SetXYWidth( i );
      ar >> i; SetXYHeight( i );
                                                                                 // have to be careful w/ XY timing
      ar >> i;                                                                   // params; values are interdependent!
      m_iDrawDelay = (i<MINDELAY_XY) ? MINDELAY_XY : ((i>MAXDELAY_XY) ? MAXDELAY_XY : i);
      ar >> i;
      m_iDrawDur = (i<MINDUR_XY) ? MINDUR_XY : ((i>MAXDUR_XY) ? MAXDUR_XY : i);
      if( m_iDrawDelay + m_iDrawDur > MAXCYCLE_XY )
         m_iDrawDelay = MAXCYCLE_XY - m_iDrawDur;

      ar >> i; SetFBDistToEye( i );
      ar >> i; SetFBWidth( i );
      ar >> i; SetFBHeight( i );
      ar >> i; SetFBBkgRed( i );
      ar >> i; SetFBBkgGrn( i );
      ar >> i; SetFBBkgBlu( i );

      ar >> i; SetFixDuration( i );
      ar >> f; SetFixAccH( f );
      ar >> f; SetFixAccV( f );
      ar >> i; SetRewardLen1( i );
      ar >> i; SetRewardLen2( i );
      ar >> i; SetVariableRatio( i );
      ar >> i; SetAudioRewardLen( i );

      // for parameters added since versioning began: Set all parameters to default values. Check version number. If it
      // is not a valid version number, then assume this is a pre-versioning CCxSettings, so we're done. Otherwise, 
      // parse IAW the version number found and the current version. See function header comments...
      m_iRMVSyncFlashDur = 1;
      m_iRMVSyncFlashSize = 0;
      m_iVStabWinLen = MIN_VSTABWIN;
      if(version >= 1 && version <= 2)
      {
         ar >> i; SetRMVSyncFlashSize(i);
         ar >> i; SetRMVSyncFlashDuration(i);

         if(version == 2) { ar >> i; SetVStabWinLen(i); }
      }
   }

   ASSERT_VALID( this );                                                         // check validity AFTER serializing
}


//=== Import ==========================================================================================================
//
//    Reinitialize the application settings object IAW a cntrlxUNIX-style, text-based definition.
//
//    CntrlxUNIX was the GUI side of MAESTRO's predecessor, a dual-platform application with the GUI running on a UNIX
//    workstation and the hardware controller hosted on a Windows PC.  To facilitate the move from cntrlxUNIX/PC to
//    MAESTRO, MAESTRO provides support for reading cntrlxUNIX object definition files.
//
//    In the case of application settings, cntrlxUNIX only supported defining the video display parameters via text
//    file.  For completeness, this method also supports defining the other application settings in the same text file.
//    The following line-by-line format is expected:
//
//       DISPLAY_FOR_CNTRLX86                This MUST be the first line.
//       VERSION <version#>                  This MUST be the second line; version# is irrelevant.
//
//    The remaining lines below may appear in any order, and all are optional.  Lines defining a given parameter can
//    appear more than once, in which case the last such line will hold the parameter value that's actually imported.
//    Any parameter that is NOT defined in the text file will retain whatever value it had prior to the import.
//
//       DISTANCE_XY <d>                     where d = distance from eye to XY scope screen, in mm (INT).
//       WIDTH_XY <w>                        where w = width of XY scope display, in mm (INT).
//       HEIGHT_XY <h>                       where h = height of XY scope display, in mm (INT).
//       DELAY_XY <del>                      where del = dot draw cycle delay, in 100-ns ticks (INT).
//       ONDUR_XY <dur>                      where dur = dot draw cycle "ON" duration, in 100-ns ticks (INT).
//       DISTANCE_FB <d>                     where d = distance from eye to RMVideo display, in mm (INT).
//       WIDTH_FB <w>                        where w = width of RMVideo display, in mm (INT).
//       HEIGHT_FB <h>                       where h = height of RMVideo display, in mm (INT).
//       BKG_RGB_FB <r> <g> <b>              where r,g,b = red, green, and blue luminance, on an arbitrary scale from
//                                              0-1000 (INTs).  All 3 luminances MUST be specified.  NOTE that the
//                                              range [0..1000] is a leftover from the old VSG framebuffer video. We
//                                              remap it to [0..255], which is the allowed range for RMVideo.
//
//    The above lines are IAW the format of the cntrlxUNIX "video display parameters" definition file.  In addition,
//    the method will parse the lines below to import the other parameters encapsulated by this application settings
//    object...
//
//       AUTO_XY  <auto>                     where <auto> = 0 (fixed seed) or nonzero (seed randomly selected each
//                                              time XY targets are generated). (INT)
//       SEED_XY  <seed>                     where <seed> = the fixed seed value for XY target generation (nonneg INT).
//       FIX_DUR  <dur>                      where <dur> = fixation duration in msecs (INT).
//       FIX_ACC  <h> <v>                    where <h> = horiz fixation accuracy in deg, <v> = V fix acc (FLOATs).
//       REWARD_LEN <r1> <r2>                where <r1>,<r2> = lengths of reward pulses 1 and 2, in msecs (INTs).
//       VAR_RATIO <vr>                      where <vr> = variable ratio for random withholding, in [1..10] (INT).
//       AUDIOREW_LEN <l>                    where <l> = length of audio reward pulse in ms (INT).
//       TRIAL_OVR <ovr>                     where <ovr> = trial reward pulse override flag (0=unset, nonzero=set).
//       BEEP_ENABLE <beep>                  where <beep> = reward indicator beep flag (0=disabled, nonzero=enabled).
//
//    Any lines starting with unrecognized keywords are simply skipped.  Any out-of-range parameter values will be
//    auto-corrected.  If the import fails b/c of a format error, the settings object is restored to its initial state.
//
//    ARGS:       strArDefn   -- [in] the cntrlxUNIX-style definition as a series of text strings.
//                strMsg      -- [out] if there's an error, this should contain brief description of the error.
//
//    RETURNS:    TRUE if import successful; FALSE otherwise.
//
//    THROWS:     NONE.
//
BOOL CCxSettings::Import( CStringArray& strArDefn, CString& strMsg )
{
   float f;

   static LPCTSTR BADFMTMSG = _T("Unrecognized format");

   BOOL bOk = (strArDefn.GetSize() > 2);                                // must have first two header lines at least
   if( bOk ) bOk = BOOL(strArDefn[0] == _T("DISPLAY_FOR_CNTRLX86"));    // this must be the first line in defn file
   if( bOk ) bOk = (1==::sscanf_s( strArDefn[1], "VERSION %f", &f ));   // this must be the second line in defn file
   if( !bOk )                                                           // abort if the header is bad
   {
      strMsg = BADFMTMSG; strMsg += _T("(hdr)"); return( FALSE );
   }

   CCxSettings saveSettings;                                            // make a copy of current state, in case
   saveSettings.Copy( *this );                                          // import fails and we must restore orig state

   char keyword[20];                                                    // keyword (e.g., "DISTANCE_XY") in text line
   float f1, f2, f3;                                                    // parameter values as floats

   int i = 2;                                                           // BEGIN: parse remaining lines
   while( bOk && i < strArDefn.GetSize() )
   {
      // tokenize line into keyword and up to 3 param values; every line has at least one param value, plus keyword
      int nTokens = ::sscanf_s(strArDefn[i], "%s %f %f %f", keyword, 20, &f1, &f2, &f3);
      if(nTokens < 2) 
      { 
         bOk = FALSE; strMsg = BADFMTMSG; strMsg += _T("(line)");
      }
      else if( ::strcmp( keyword, "DISTANCE_XY" ) == 0 )
         SetXYDistToEye( int(f1) );
      else if( ::strcmp( keyword, "WIDTH_XY" ) == 0 )
         SetXYWidth( int(f1) );
      else if( ::strcmp( keyword, "HEIGHT_XY" ) == 0 )
         SetXYHeight( int(f1) );
      else if( ::strcmp( keyword, "DELAY_XY" ) == 0 )
         SetXYDrawDelay( int(f1) );
      else if( ::strcmp( keyword, "ONDUR_XY" ) == 0 )
         SetXYDrawDur( int(f1) );
      else if( ::strcmp( keyword, "DISTANCE_FB" ) == 0 )
         SetFBDistToEye( int(f1) );
      else if( ::strcmp( keyword, "WIDTH_FB" ) == 0 )
         SetFBWidth( int(f1) );
      else if( ::strcmp( keyword, "HEIGHT_FB" ) == 0 )
         SetFBHeight( int(f1) );
      else if( ::strcmp( keyword, "BKG_RGB_FB" ) == 0 )                 //    there must be 3 parameters associated
      {                                                                 //    with the BKG_RGB_FB keyword
         if( nTokens < 4 )
         {
            bOk = FALSE; strMsg = BADFMTMSG; strMsg += _T("(BKG_RGB_FB)");
         }
         else
         {                                                              //    NOTE conversion from [0..1000] range to
            SetFBBkgRed( int( (f1*255.0f)/1000.0f ) );                  //    [0..255]
            SetFBBkgGrn( int( (f2*255.0f)/1000.0f ) );
            SetFBBkgBlu( int( (f3*255.0f)/1000.0f ) );
         }
      }
      else if( ::strcmp( keyword, "AUTO_XY" ) == 0 )
         SetXYDotSeedFixed( BOOL(f1==0.0f) );
      else if( ::strcmp( keyword, "SEED_XY" ) == 0 )
         SetFixedXYDotSeedValue( DWORD(f1) );
      else if( ::strcmp( keyword, "FIX_DUR" ) == 0 )
         SetFixDuration( int(f1) );
      else if( ::strcmp( keyword, "FIX_ACC" ) == 0 )                    //    2 params assoc w/ FIX_ACC keyword
      {
         if( nTokens < 3 )
         {
            bOk = FALSE; strMsg = BADFMTMSG; strMsg += _T("(FIX_ACC)");
         }
         else
         {
            SetFixAccH( f1 );
            SetFixAccV( f2 );
         }
      }
      else if( ::strcmp( keyword, "REWARD_LEN" ) == 0 )                 //    2 params assoc w/ REWARD_LEN keyword
      {
         if( nTokens < 3 )
         {
            bOk = FALSE; strMsg = BADFMTMSG; strMsg += _T("(REWARD_LEN)");
         }
         else
         {
            SetRewardLen1( int(f1) );
            SetRewardLen2( int(f2) );
         }
      }
      else if( ::strcmp( keyword, "VAR_RATIO" ) == 0 )
         SetVariableRatio( int(f1) );
      else if( ::strcmp( keyword, "AUDIOREW_LEN" ) == 0 )
         SetAudioRewardLen( int(f1) );
      else if( ::strcmp( keyword, "TRIAL_OVR" ) == 0 )
         SetTrialRewLenOverride( BOOL(f1 != 0.0f) );
      else if( ::strcmp( keyword, "BEEP_ENABLE" ) == 0 )
         SetRewardBeepEnabled( BOOL(f1 != 0.0f) );

      ++i;                                                              //    go to next line in string array
   }                                                                    // END: parse remaining lines

   if( !bOk ) Copy( saveSettings );                                     // on format error, restore original state
   return( bOk );
}



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump video display parameters in an easy-to-read form to the supplied dump context.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxSettings::Dump( CDumpContext& dc ) const
{
   CObject::Dump( dc );

   CString str;
   str.Format( "Bit flag settings: 0x%08x", m_dwFlags );
   dc << str;

   str.Format( "XY scope display geometry (mm): distToEye = %d, w = %d, h = %d\n",
               m_iDistToEyeXY, m_iWidthXY, m_iHeightXY );
   dc << str;
   str.Format( "XY scope timing (100-ns): delay to ON pulse = %d, ON pulse dur = %d\n", m_iDrawDelay, m_iDrawDur );
   dc << str;

   str.Format( "RMVideo display geometry (mm): distToEye = %d, w = %d, h = %d\n",
               m_iDistToEyeRMV, m_iWidthRMV, m_iHeightRMV );
   dc << str;
   str.Format( "RMVideo background color: R = %d, B = %d, G = %d\n",
               m_iBkgColor[FB_RED], m_iBkgColor[FB_GRN], m_iBkgColor[FB_BLU] );
   dc << str;
   str.Format("RMVideo sync flash: spot size = %d mm, dur = %d frames\n", m_iRMVSyncFlashSize, m_iRMVSyncFlashDur);
   dc << str;

   str.Format( "Fixation: dur(ms) = %d, accuracy H,V(deg) = %.2f, %.2f\n", m_iFixDur, m_fFixAccH, m_fFixAccV );
   dc << str;
   str.Format( "Rewards: pulse 1,2(ms) = %d, %d; multiplier=%.1f; withholding VR = %d; audio rew pulse len = %d\n",
               m_iRewLen1, m_iRewLen2, m_fRewMult, m_iVarRatio, m_iAudioRewLen );
   dc << str;

   str.Format("VStab window length (ms) = %d\n", m_iVStabWinLen);
   dc << str;
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate the video display configuration.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxSettings::AssertValid() const
{
   CObject::AssertValid();
   ASSERT( m_iDistToEyeXY >= MINDIM && m_iDistToEyeXY <= MAXDIM );
   ASSERT( m_iWidthXY >= MINDIM && m_iWidthXY <= MAXDIM );
   ASSERT( m_iHeightXY >= MINDIM && m_iHeightXY <= MAXDIM );
   ASSERT( m_iDrawDelay >= MINDELAY_XY && m_iDrawDelay <= MAXDELAY_XY );
   ASSERT( m_iDrawDur >= MINDUR_XY && m_iDrawDur <= MAXDUR_XY );
   ASSERT( (m_iDrawDelay + m_iDrawDur) <= MAXCYCLE_XY );

   ASSERT( m_iDistToEyeRMV >= MINDIM && m_iDistToEyeRMV <= MAXDIM );
   ASSERT( m_iWidthRMV >= MINDIM && m_iWidthRMV <= MAXDIM );
   ASSERT( m_iHeightRMV >= MINDIM && m_iHeightRMV <= MAXDIM );
   for( int i = 0; i < 3; i++ ) ASSERT( m_iBkgColor[i] >= MINRGB_RMV && m_iBkgColor[i] <= MAXRGB_RMV );

   ASSERT( m_iFixDur >= MINFIXDUR && m_iFixDur <= MAXFIXDUR );
   ASSERT( m_fFixAccH >= MINFIXACC && m_fFixAccH <= MAXFIXACC );
   ASSERT( m_fFixAccV >= MINFIXACC && m_fFixAccV <= MAXFIXACC );
   ASSERT( m_iRewLen1 >= MINREWLEN && m_iRewLen1 <= MAXREWLEN );
   ASSERT( m_iRewLen2 >= MINREWLEN && m_iRewLen2 <= MAXREWLEN );
   ASSERT(m_fRewMult >= MINREWMULT && m_fRewMult <= MAXREWMULT);
   ASSERT( m_iVarRatio >= MINWHVR && m_iVarRatio <= MAXWHVR );
   ASSERT( m_iAudioRewLen == 0 || (m_iAudioRewLen >= MINAUDIOREWLEN && m_iAudioRewLen <= MAXAUDIOREWLEN) );

   ASSERT(m_iRMVSyncFlashSize >= MINSYNCFLASHSZ && m_iRMVSyncFlashSize <= MAXSYNCFLASHSZ);
   ASSERT(m_iRMVSyncFlashDur >= MINSYNCFLASHDUR && m_iRMVSyncFlashDur <= MAXSYNCFLASHDUR);

   ASSERT(m_iVStabWinLen >= MIN_VSTABWIN && m_iVStabWinLen <= MAX_VSTABWIN);
}

#endif //_DEBUG




//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================


