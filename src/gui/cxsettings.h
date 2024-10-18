//===================================================================================================================== 
//
// cxsettings.h : Declaration of class CCxSettings.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXSETTINGS_H__INCLUDED_)
#define CXSETTINGS_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cxobj_ifc.h"                       // MAESTRO object interface:  common constants and other defines 


//===================================================================================================================== 
// Declaration of class CCxSettings
//===================================================================================================================== 
//
class CCxSettings : public CObject
{
   DECLARE_SERIAL( CCxSettings )

//===================================================================================================================== 
// CONSTANTS AND TYPEDEFS
//===================================================================================================================== 
private:
   // [deprecated] seed generation for XYScope targets. Definition kept to deal with versioning support in Serialize()
   static const DWORD F_XYFIXSEED; 
   // if set, use reward pulse length settings here in place of similar settings in the individual trial definitions
   static const DWORD F_TRIALREWOVR;
   // bit set to enable reward indicator beep
   static const DWORD F_REWBEEPENA; 

   static const DWORD CURRVERSION;        // for versioning support in Serialize() [VERSIONABLE_SCHEMA not used]

   static const int MINDIM;               // allowed range for display geometry parameters (mm) 
   static const int MAXDIM;
   static const int MINRGB_RMV;           // allowed range for RMVideo background color specification
   static const int MAXRGB_RMV;

   static const int MINFIXDUR;            // allowed range for fixation duration (ms)
   static const int MAXFIXDUR;
   static const float MINFIXACC;          // allowed range for fixation accuracy (deg subtended at eye)
   static const float MAXFIXACC;
   static const int MINREWLEN;            // allowed range for reward pulse lengths (ms)
   static const int MAXREWLEN;
   static const float MINREWMULT;         // allowed range for reward pulse multiplier
   static const float MAXREWMULT;
   static const int MINWHVR;              // allowed range for random reward withholding "variable ratio"
   static const int MAXWHVR;
   static const int MINAUDIOREWLEN;       // allowed range for audio reward pulse length (ms)
   static const int MAXAUDIOREWLEN;       // 

   static const int MINSYNCFLASHSZ;       // allowed range for size of square RMVideo time sync flash (mm)
   static const int MAXSYNCFLASHSZ;
   static const int MINSYNCFLASHDUR;      // allowed range for RMVideo time sync flash duration (in #frames)
   static const int MAXSYNCFLASHDUR;

   static const int MIN_VSTABWIN;         // allowed range for VStab sliding-average window length (in ms)
   static const int MAX_VSTABWIN;

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   // various disabled/enabled settings -- see F_* flag bits.
   DWORD m_dwFlags;

   // RMVideo display parameters:
   int   m_iDistToEyeRMV;                 //    distance from display screen to eye along normal LOS, in mm
   int   m_iWidthRMV;                     //    width of display screen, in mm
   int   m_iHeightRMV;                    //    height of display screen, in mm
   int   m_iBkgColor[3];                  //    RGB triplet for background display color, w/ 8-bit res [0..255]
   // fixation requirements and reward options:
   int   m_iFixDur;                       //    fixation duration for ContMode & mid-trial rewards (ms)
   float m_fFixAccH;                      //    horizontal fixation accuracy (deg subtended at eye)
   float m_fFixAccV;                      //    vertical fixation accuracy (deg subtended at eye)
   int   m_iRewLen1;                      //    reward pulse length 1 (ms)
   int   m_iRewLen2;                      //    reward pulse length 2 (ms)
   float m_fRewMult;                      //    reward pulse length multiplier
   int   m_iVarRatio;                     //    variable ratio for random withholding 
   int   m_iAudioRewLen;                  //    audio reward pulse length (ms) 

   // parameters governing a time sync flash that is optionally presented at the start of trial segments in the top
   // left corner of RMVideo display -- for timestamping the start of a video frame in "Maestro time". A square patch
   // is reserved for the flash; it is black regardless the current RMVideo background color, then flashes white when
   // requested. The patch size is in mm, 0..50 (0 disables feature). Flash duration is [1..9] video frames.
   int m_iRMVSyncFlashSize;
   int m_iRMVSyncFlashDur;

   // length of sliding-average window for velocity stabilization feature in Trial Mode. Allowed range is [1..20].
   int m_iVStabWinLen;


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxSettings( const CCxSettings& src );                // no copy constructor defined
   CCxSettings& operator=( const CCxSettings& src );     // no assignment operator defined 

public:
   CCxSettings()                                         // constructor -- init all settings to default values
   { 
      m_dwFlags = 0;
      RestoreDefaults(); 
   }
   ~CCxSettings() {}                                     // destructor


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   BOOL IsTrialRewLenOverride() const { return( BOOL(m_dwFlags & F_TRIALREWOVR) ); }
   BOOL IsRewardBeepEnabled() const { return( BOOL(m_dwFlags & F_REWBEEPENA) ); }

   int GetFBDistToEye() const { return( m_iDistToEyeRMV ); }
   int GetFBWidth() const { return( m_iWidthRMV ); }
   int GetFBHeight() const { return( m_iHeightRMV ); }
   int GetFBBkgRed() const { return( m_iBkgColor[FB_RED] ); }
   int GetFBBkgGrn() const { return( m_iBkgColor[FB_GRN] ); }
   int GetFBBkgBlu() const { return( m_iBkgColor[FB_BLU] ); }
   BOOL IsFBBkgGray() const 
   {
      return( BOOL( (m_iBkgColor[FB_RED] == m_iBkgColor[FB_GRN]) && (m_iBkgColor[FB_GRN] == m_iBkgColor[FB_BLU]) ) );
   }

   int GetFixDuration() const { return( m_iFixDur ); }
   float GetFixAccH() const { return( m_fFixAccH ); }
   float GetFixAccV() const { return( m_fFixAccV ); }
   int GetRewardLen1() const { return( m_iRewLen1 ); }
   int GetRewardLen2() const { return( m_iRewLen2 ); }
   float GetRewardPulseMultiplier() const { return(m_fRewMult);  }
   int GetVariableRatio() const { return( m_iVarRatio ); }
   int GetAudioRewardLen() const { return( m_iAudioRewLen ); }

   int GetRMVSyncFlashSize() const { return(m_iRMVSyncFlashSize); }
   int GetRMVSyncFlashDuration() const { return(m_iRMVSyncFlashDur); }
   BOOL IsRMVSyncFlashDisabled() const { return(BOOL(m_iRMVSyncFlashSize == 0)); }

   int GetVStabWindowLen() const { return(m_iVStabWinLen);  }

//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public: 
   VOID Copy( const CCxSettings& src );                  // copy settings from another settings object

   // change individual settings, correcting or rejecting out-of-range values as necessary...
   BOOL SetTrialRewLenOverride( BOOL bEna );
   BOOL SetRewardBeepEnabled( BOOL bEna );

   int SetFBDistToEye( int i );
   int SetFBWidth( int i );
   int SetFBHeight( int i );
   int SetFBBkgRed( int i );
   int SetFBBkgGrn( int i );
   int SetFBBkgBlu( int i );
   int SetFBBkgGrayscale( int i );

   int SetFixDuration( int i );
   float SetFixAccH( float f );
   float SetFixAccV( float f );
   int SetRewardLen1( int i );
   int SetRewardLen2( int i );
   float SetRewardPulseMultiplier(float f);
   int SetVariableRatio( int i );
   int SetAudioRewardLen( int i );

   int SetRMVSyncFlashSize(int i);
   int SetRMVSyncFlashDuration(int i);

   int SetVStabWinLen(int i);

   // scales reward pulse length if the current multiplier exceeds 1
   int GetScaledRewardPulseLen(int len);

   VOID RestoreDefaultVideoSettings();                   // restore video display settings to default values 
   VOID RestoreDefaults();                               // restore all default settings
   void Serialize( CArchive& ar );                       // for reading/writing settings obj from/to disk file
   BOOL Import(CStringArray& strArDefn,CString& strMsg); // import settings from cntrlxUNIX-style, text-based defn


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
protected:
};


#endif   // !defined(CXSETTINGS_H__INCLUDED_)
