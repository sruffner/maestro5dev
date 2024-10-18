//===================================================================================================================== 
//
// cxchannel.cpp : Implementation of class CCxChannel, encapsulating a CNTRLX "channel configuration object". 
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CNTRLX offers analog input channels for recording data during an experiment, digital input channels for recording 
// spike trains and other kinds of pulse trains (event markers, etc), and some "computed" channels that represent the 
// ideal, computed trajectories of fixation targets during a CNTRLX trial.  Many of the analog input channels are 
// dedicated to specific signals that are conditioned by the CNTRLX experimental setup external to the PC box.  Digital 
// input 0 is dedicated to spike trains, while the other digital inputs are multi-purpose.  The current channel 
// designations ("horizontal gaze position", "vertical eye velocity", etc.) are fixed by the CNTRLX design and cannot 
// be modified by the user; the names are encapsulated by CCxChannel.  However, during any particular CNTRLX trial or 
// continuous-mode run, the user can select which channels are recorded (this does not apply to the computed channels, 
// of course; and all digital inputs are recorded whenever recording is on) and which are shown in the channel trace 
// display during runtime.  The user also controls the channel trace gain, offset, and color to ease the interpretation 
// of multiple traces during runtime.  The CNTRLX "channel configuration" refers to the complete set of these channel 
// attributes for all existing CNTRLX data channels.  The CCxChannel object encapsulates this configuration. 
//
// An individual data channel is identified in one of two ways:  by its "cardinal position" in the internal array 
// (actually, there are several arrays) containing channel attributes, or by the combination of the channel's type 
// (CCxChannel::ChType) and its physical channel #.  Accessor methods are available using either scheme.  Channel 
// attributes may be retrieved individually, or in a single CCxChannel::ChInfo record.
//
// CCxChannel is designed to restrict the "channel configuration" to states that are supported by CNTRLX.  Since CNTRLX 
// always records all digital input channel "events", CCxChannel methods do not allow the record flag to be cleared for 
// these channels.  Computed channels are never recorded, so their record flags cannot be set.  Display offset is 
// expressed in millivolts and determines the location of the trace's baseline along the Y-axis.  Display gain is 
// limited to powers of 2 between 2^-5 and 2^5 -- as there is no need for a fully flexible gain value.  Display color 
// is also limited to one of 12 possible colors (black is not one of them, as it is assumed that traces are displayed 
// on a black background).
//
// ==> Interaction with other CNTRLX classes.
// 1) Storage of CCxChannel objects in the CNTRLX "object tree", CCxTreeMap, within a CNTRLX "experiment document",
//    CCxDoc -- see implementation files for these classes!  Only CCxTreeMap can construct and copy channel config 
//    objects; it is a friend class to CCxChannel.
// 2) CCxChannelForm -- the CNTRLX view class which displays the contents of a CCxChannel and permits the user to 
//    modify those contents in certain ways. 
//
// ==> Importing defn from an ASCII text file.
// CNTRLX succeeds the cross-platform cntrlxUNIX/PC application, in which the GUI was hosted on a UNIX workstation
// ("cntrlxUNIX") and the hardware controller resided on a WindowsNT PC ("cntrlxPC").  In that system, the various data 
// objects (targets, channel configurations, trials, etc.) could be defined in ASCII-text "definition files".  CNTRLX 
// supports importing CNTRLX data objects from such definition files via the dedicated CCxImportDialog.  This dialog 
// is responsible for interacting with the user, opening the text files and reading the definitions into an array of 
// CString's, and creating new data objects as appropriate.  Each data class provides an Import() method that takes 
// a CStringArray and reinitializes itself IAW the definition contained therein.  Thus, the details of translating the 
// cntrlxUNIX-style text definition to the CNTRLX data object is encapsulated in the data object itself, but the 
// details of opening text files and interacting with the user are handled by a user-interface object.
//
// In the case of the channel configuration object, cntrlxUNIX "channel definition files" could define one or more 
// channel configurations.  Thus, CCxImportDialog is responsible for parsing out each individual channel configuration, 
// creating a new CCxChannel object with the assigned name, then invoking CCxChannel::Import() to complete the import.
// See CCxChannel::Import() for details.
//
// 
// DEV NOTE:  Maestro is designed to support 16-32 AI channels and 16-32 DI channels.  However, we do not yet have 
// hardware providing more than 16 channels.  CCxChannel needs mods to support up to 32 AI and 32 DI channels.
//
//
// REVISION HISTORY:
// 04jan2001-- Began development.
// 16jan2001-- Moved away from super-compact, hard-to-implement storage scheme toward a more straightforward one.
// 14mar2001-- Added the 16 digital input channels to CCxChannel -- a new feature not available in cntrlxUNIX/PC. 
//             Digital inputs are always recorded whenever recording is on, so the record flag is not relevant, nor is 
//             gain.  However, the offset and color values are used; in particular, the user can adjust the offset 
//             value to position the displayed pulse train conveniently w/in the channel trace display. 
// 13aug2001-- Began adding features to CCxChannel:  Vertical display range (yMin, yMax) assoc with channel config; 
//             12 possible trace colors instead of 10; a public struct, CCxChannel::CChInfo, summarizing a channel's 
//             attributes -- including a channel's "type" (AI, DI, or "computed") and its actual, physical channel #.
//             Methods added or modified to expose new features.  Also added method for enumerating the channels 
//             currently marked for display by the CCxChannel object.
// 16aug2001-- Added CCxTestMode as a friend class of CCxChannel -- so that the TestMode controller can construct a 
//             private channel configuration to manage what AI & DI channels are displayed in TestMode.
//          -- Added ClearAll().  Modified SpaceEvenly() to position all DI traces on a single baseline above the 
//             displayed AI and computed traces.
//          -- Added overloaded versions of the channel attribute methods that specify a channel by type & physical 
//             chan # rather than cardinal pos in the configuration object.
// 09oct2001-- CCxRuntime is now friend class of CCxChannel.  CCxTestMode deleted in architectural rework.
// 18dec2001-- Added convenience fcn GetRecordedAIChannels().
// 23oct2002-- Display offset and Y-axis range expressed in millivolts rather than b2sAIvolts.  Offset stored as an 
//             int instead of a short.  Introduced versionable schema to handle deserialization of previous versions. 
//             Made other cosmetic changes.
// 05nov2002-- Added GetTraceColorLabel(), Get/SetColorIndex(), Get/SetGainIndex().  Also, GetColorRange() replaced by 
//             GetNumTraceColors(), and GetGainRange() replaced by GetGainMin(), GetGainMax().
// 21nov2002-- Adding Import() to support importing CCxChannel defn from a cntrlxUNIX-style "channel definition" file.
// 28jul2003-- Revised channel labels.
// 15mar2005-- Added CopyRemoteObj() override.
// 10aug2005-- Dedicated ADC channel 11 to recording a second vertical eye position signal, VEPOS2.
// 17may2012-- Maestro 3 no longer supports optic-bench targets FIBER1/2. Thus, AI channels 4,5,9 and 10 -- formerly
// dedicated to recording the feedback position (H,V) of these two targets -- are now unused inputs. Channel labels in
// CCxChannel::strChanLbls[] updated accordingly.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource defines

#include "cxchannel.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//===================================================================================================================== 
// PRIVATE CONSTANTS & GLOBALS
//===================================================================================================================== 

const BYTE CCxChannel::F_REC           = (BYTE) (1<<0);  // channel record ON/OFF flag
const BYTE CCxChannel::F_DISP          = (BYTE) (1<<1);  // channel display ON/OFF flag

LPCTSTR CCxChannel::strChanLbls[] =                      // channel label assignments (fixed)
{
   _T("Horiz Gaze Pos: HGPOS"),
   _T("Vert Eye Pos: VEPOS"),
   _T("Horiz Eye Vel 25Hz: HEVEL"),
   _T("Vert Eye Vel: VEVEL"),
   _T("ADC Channel 4 (multi-use)"),
   _T("ADC Channel 5 (multi-use)"), 
   _T("Horiz Head Vel: HHVEL"),
   _T("Horiz Head Pos: HHPOS"),
   _T("Horiz Eye Vel 100Hz: HDVEL"),
   _T("ADC Channel 9 (multi-use)"),
   _T("ADC Channel 10 (multi-use)"),
   _T("Vert Eye Pos 2: VEPOS2"),
   _T("ADC Channel 12 (multi-use)"),
   _T("ADC Channel 13 (multi-use)"),
   _T("Horiz Gaze Pos 2: HGPOS2"),
   _T("Electrode Voltage at 25KHz: SPWAV"), 
   _T("H Vel, FixTgt 1 (computed)"), 
   _T("V Vel, FixTgt 1 (computed)"), 
   _T("H Vel, FixTgt 2 (computed)"), 
   _T("V Vel, FixTgt 2 (computed)"), 
   _T("H Pos, FixTgt 1 (computed)"), 
   _T("V Pos, FixTgt 1 (computed)"),
   _T("Spike Train (DI0)"),
   _T("Digital Input 1"),
   _T("Digital Input 2"),
   _T("Digital Input 3"),
   _T("Digital Input 4"),
   _T("Digital Input 5"),
   _T("Digital Input 6"),
   _T("Digital Input 7"),
   _T("Digital Input 8"),
   _T("Digital Input 9"),
   _T("Digital Input 10"),
   _T("Digital Input 11"),
   _T("Digital Input 12"),
   _T("Digital Input 13"),
   _T("Digital Input 14"),
   _T("Digital Input 15")
};

const COLORREF CCxChannel::trColors[] =                  // available colors for channel traces
{
   RGB(255,255,255),                                     // white
   RGB(255,  0,  0),                                     // red
   RGB(  0,255,  0),                                     // green
   RGB(  0,  0,255),                                     // blue
   RGB(255,255,  0),                                     // yellow
   RGB(255,  0,255),                                     // magenta
   RGB(  0,255,255),                                     // cyan
   RGB(  0,128,  0),                                     // dk green
   RGB(255,128,  0),                                     // orange
   RGB(128,  0,255),                                     // purple
   RGB(255,128,192),                                     // pink
   RGB(128,128,128)                                      // med gray
};
LPCTSTR CCxChannel::strClrLbls[] =                       // short descriptive names for the trace colors
{
   _T("white"),
   _T("red"),
   _T("green"), 
   _T("blue"),
   _T("yellow"),
   _T("magenta"),
   _T("cyan"),
   _T("dk green"),
   _T("orange"),
   _T("purple"),
   _T("pink"),
   _T("med gray")
};



IMPLEMENT_SERIAL( CCxChannel, CTreeObj, 2 | VERSIONABLE_SCHEMA )


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== Initialize [base override] ====================================================================================== 
//
//    Initialize channel configuration object after default construction.
//
//    This method MUST be called directly after default construction to initialize the channel configuration IAW the 
//    specified name, CNTRLX object type, and state flags.  The channel configuration data is set to default state.
//
//    ARGS:       s  -- [in] the name assigned to target object
//                t  -- [in] the CNTRLX object data type -- MUST be a recognized CNTRLX target type 
//                f  -- [in] the object's initial state flags -- CANNOT include CX_ISSETOBJ. 
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to allocate memory for modifiable target parameters.
//
VOID CCxChannel::Initialize( LPCTSTR s, const WORD t, const WORD f )
{
   ASSERT( t == CX_CHANCFG );                      // validate object type and flags
   ASSERT( (f & CX_ISSETOBJ) == 0 );

   RestoreDefaults();
   CTreeObj::Initialize( s, t, f );                // base class inits
}


//=== Copy [base override] ============================================================================================ 
//
//    Copy a channel configuration object.
//
//    ARGS:       pSrc  -- [in] CTreeObj* ptr to the channel cfg to be copied.  MUST point to a valid CCxChannel obj! 
//
//    RETURNS:    NONE.
//
VOID CCxChannel::Copy( const CTreeObj* pSrc )
{
   ASSERT( pSrc != NULL );
   ASSERT( pSrc->IsKindOf( RUNTIME_CLASS( CCxChannel ) ) );

   const CCxChannel* pSrcChan = (CCxChannel*) pSrc;
   ASSERT_VALID( pSrcChan );

   CTreeObj::Initialize( pSrcChan->m_name, pSrcChan->m_type, pSrcChan->m_flags ); 

   for( int i = 0; i < NUMCHAN; i++ ) 
   {
      m_offset[i] = pSrcChan->m_offset[i];
      m_gain[i]   = pSrcChan->m_gain[i];
      m_color[i]  = pSrcChan->m_color[i];
      m_onoff[i]  = pSrcChan->m_onoff[i];
   }
   m_yDispMin = pSrcChan->m_yDispMin;
   m_yDispMax = pSrcChan->m_yDispMax;
}


//=== CopyRemoteObj [base override] =================================================================================== 
//
//    Copies the CCxChannel-specific definition of a channel configuration located in a different experiment document.
//
//    CopyRemoteObject was introduced to the CTreeObj/CTreeMap framework to overcome the problem of copying an object 
//    from one treemap to another.  It is intended only for copying the internal information specific to a given 
//    implementation of CTreeObj.
//
//    ARGS:       pSrc        -- [in] the object to be copied.  Must be an instance of CCxChannel.
//                depKeyMap   -- [in] maps keys of any objects upon which the source obj depends, which reside in the 
//                               source doc, to the keys of the corresponding objects in the destination doc.
//
//    RETURNS:    TRUE if successful, FALSE if source object is not an instance of CCxChannel.
//
BOOL CCxChannel::CopyRemoteObj(CTreeObj* pSrc, const CWordToWordMap& depKeyMap)
{
   if( pSrc == NULL || !pSrc->IsKindOf( RUNTIME_CLASS(CCxChannel) ) ) return( FALSE );

   const CCxChannel* pSrcChan = (CCxChannel*) pSrc;
   ASSERT_VALID( pSrcChan );

   for( int i = 0; i < NUMCHAN; i++ ) 
   {
      m_offset[i] = pSrcChan->m_offset[i];
      m_gain[i]   = pSrcChan->m_gain[i];
      m_color[i]  = pSrcChan->m_color[i];
      m_onoff[i]  = pSrcChan->m_onoff[i];
   }
   m_yDispMin = pSrcChan->m_yDispMin;
   m_yDispMax = pSrcChan->m_yDispMax;

   return( TRUE );
}



//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== RestoreDefaults ================================================================================================= 
//
//    Sets channel configuration to a default state:
//       Horizontal Gaze Position (AI0)   : record ON, display ON, gain = 1 (2^0), offset = 0, color = white.
//       Vertical Eye Position (AI1)      : record ON, display ON, gain = 1, offset = 0, color = yellow.
//       Horizontal Eye Velocity (AI2)    : record ON, display ON, gain = 1, offset = 0, color = white.
//       Vertical Eye Velocity (AI3)      : record ON, display ON, gain = 1, offset = 0, color = yellow.
//       Spike Train (DI0)                : record ON, display ON, gain = N/A, offset = 1000, color = red.
//       All other channels***            : record OFF, display OFF, gain = 1, offset = 0, color = white.
//
//       Vertical display range = (-5000..5500).
//
//    ***Except that the record flag is set for all digital input channels; it can never be changed, reflecting the 
//    fact that digital inputs are always recorded in CNTRLX.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxChannel::RestoreDefaults()
{
   ClearAll();

   m_offset[FIRSTDI] = 5000;                                   // DI0 default offset is 5000
   m_color[1] = m_color[3] = 4;                                // default color for ADC1 & ADC3 is yellow
   m_color[FIRSTDI] = 2;                                       // DI0 default color is red
   for( int i = 0; i < 4; i++ ) m_onoff[i] = F_REC | F_DISP;   // turn on record & display for ADC0-3 
   m_onoff[FIRSTDI] |= F_DISP;                                 // turn on display flag for DI0

   m_yDispMax = 5500;                                          // default vertical display range 
   m_yDispMin = -5000;
}


//=== ClearAll ======================================================================================================== 
//
//    Sets all channels as follows:  record OFF (except for DI, for which record flag is always set), display OFF, 
//    gain = 1 (2^0), offset = 0, color = white.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxChannel::ClearAll()
{
   for( int i = 0; i < NUMCHAN; i++ )                       // initialize all channels:
   {
      m_offset[i] = 0;                                      //    zero offset
      m_gain[i] = 0;                                        //    gain = 2^0 = 1.
      m_color[i] = 0;                                       //    color = white
      m_onoff[i] = (i >= FIRSTDI) ? F_REC : 0;              //    rec & dsp OFF, exc: digital inputs always recorded 
   }
}


//=== SpaceEvenly ===================================================================================================== 
//
//    Reworks the configuration attributes of all channels to evenly space out the currently displayed channel traces 
//    over the y-axis in a very specific manner.  The following changes are made:
//       1) For all channels that are NOT displayed -- gain, offset, and color are set to default values.
//       2) For all AI and computed channels that are displayed -- gain is set to 1 (2^0), offset is set such that the 
//          displayed traces are spaced out evenly along the y-axis, and color is set so that a different color is 
//          selected for each channel (until all available trace colors are used!).
//       3) For all DI channels that are displayed -- offset is set such that all DI traces appear above the AI/comp 
//          traces, along the same baseline.
//       3) Vertical display range is set so that all trace baselines are visible, plus top and bottom margins = 
//          one-half of desired spacing (or at least 100).
//
//    ARGS:       nSpacing -- [in] desired offset spacing for AI/computed traces, in mV.
//
//    RETURNS:    NONE
//
VOID CCxChannel::SpaceEvenly( int nSpacing )
{
   int i;
   int nSpaces = 0;                                               // # of distinct trace baselines
   for( i = 0; i < FIRSTDI; i++ )                                 // separate baseline for each AI/computed trace 
   {
      if( IsDisplayed( i ) ) ++nSpaces;
   }
   for( i = FIRSTDI; i < NUMCHAN; i++ )                           // single baseline for all displayed DI traces
   {
      if( IsDisplayed( i ) ) { ++nSpaces; break; }
   }
   
   if( nSpacing < 0 ) nSpacing = -nSpacing;                       // want positive spacing

   int nFirstOffset = 0;                                          // we'll set offsets starting from most pos offset to 
   if( nSpaces > 0 )                                              // most negative; ensure that spacing isn't too big. 
   {
      nFirstOffset = nSpacing * (nSpaces / 2);
      while( nFirstOffset > CHANOFFMAX )
      {
         nSpacing /= 2;
         nFirstOffset = nSpacing * (nSpaces / 2);
      }
   }

   int nMargin = nSpacing / 2;                                    // size of top and bottom margins
   if( nMargin < 100 ) nMargin = 100;
   m_yDispMax = nFirstOffset + nMargin;                           // display max is largest pos offset + margin
   m_yDispMin = nFirstOffset - nMargin;                           // display min is largest neg offset - margin
   if( nSpaces > 0 ) m_yDispMin -= nSpacing * (nSpaces-1);

   int nFirstColor = 0;                                           // we'll set colors starting from 0...
   for( i = NUMCHAN-1; i >= 0; i-- )                              // set up all the channels, starting w/ DIs...
   {
      if( IsDisplayed( i ) )
      {
         m_offset[i] = nFirstOffset;
         if( i <= FIRSTDI ) nFirstOffset -= nSpacing;             //    any & all DI traces appear at top baseline!
         m_gain[i] = 0;
         m_color[i] = nFirstColor;
         if( ++nFirstColor == NCOLORS ) 
            nFirstColor = 0;
      }
      else
      {
         m_offset[i] = 0;
         m_gain[i] = 0;
         m_color[i] = 0;
      }
   }

}


//=== GetNDisplay, GetNRecord ========================================================================================= 
// 
//    Determine # of channels currently tagged for display or recording.
//
//    ARGS:       NONE
//
//    RETURNS:    # of channels.
//
int CCxChannel::GetNDisplay() const
{
   int n = 0;
   for( int i = 0; i < NUMCHAN; i++ ) { if( IsDisplayed( i ) ) ++n; }
   return( n );
}

int CCxChannel::GetNRecord() const
{
   int n = 0;
   for( int i = 0; i < NUMCHAN; i++ ) { if( IsRecorded( i ) ) ++n; }
   return( n );
}


//=== GetRecordedAIChannels =========================================================================================== 
// 
//    Retrieve the channel#s of all AI channels currently selected for recording.
//
//    ARGS:       piChan   -- [out] buffer that will hold the AI channel#s; caller must ensure the buffer is large 
//                            enough to hold CX_AIO_MAXN elements.
//
//    RETURNS:    # of channels.
//
int CCxChannel::GetRecordedAIChannels( int* piChan ) const
{
   int n = 0;
   for( int i = 0; i < FIRSTCP; i++ ) if( IsRecorded( i ) )
   {
      piChan[n] = i;
      ++n;
   }
   return( n );
}


//=== GetChannel ====================================================================================================== 
// 
//    Retrieve attribute record (CCxChannel::ChInfo) of a particular channel, which is identified by its cardinal 
//    position in the channel configuration object.  An overloaded version identifies channel by its type and physical 
//    channel #, such as AI ch#3.
//
//    ARGS:       iPos     -- [in] position of channel in the internal channel array. 
//                chInfo   -- [out] the channel attribute record
//
//    RETURNS:    TRUE if successful, FALSE if specified channel pos does not exist. 
//
BOOL CCxChannel::GetChannel( const int iPos, ChInfo& chInfo ) const
{
   if( !IsValid( iPos ) ) return( FALSE );

   chInfo.iPos = iPos;
   chInfo.chType = (iPos<FIRSTCP) ? AIChan : ((iPos<FIRSTDI) ? CPChan : DIChan);
   chInfo.nCh = (iPos<FIRSTCP) ? iPos : ((iPos<FIRSTDI) ? (iPos-FIRSTCP) : (iPos-FIRSTDI));
   chInfo.bDisplay = IsDisplayed( iPos );
   chInfo.bRecord = IsRecorded( iPos );
   chInfo.iOffset = m_offset[iPos];
   chInfo.iGain = (int) m_gain[iPos];
   chInfo.crDisplay = GetTraceColor( (int) m_color[iPos] );
   return( TRUE );
}


//=== GetNextDisplayed ================================================================================================ 
// 
//    Enumeration method retrieves the attribute records for all channels currently tagged for display.  To enumerate, 
//    initially invoke method with iNext < 0.  If no channels are displayed, the method will return FALSE on this first 
//    call.  Else, it sets iNext to the cardinal pos of the first displayed channel and stores the channel's attributes 
//    in the record provided.  Subsequent calls advance through the internal channel array until there are no more 
//    channels tagged for display.
//
//    ARGS:       iNext    -- [in] cardinal pos of last channel enumerated (<0 to start enumeration).
//                         -- [out] cardinal pos of the next channel tagged for display, if any.
//                chInfo   -- [out] attribute record for the next channel tagged for display, if any.
//
//    RETURNS:    TRUE if another displayed channel was found; FALSE otherwise (end of enumeration). 
//
BOOL CCxChannel::GetNextDisplayed( int& iNext, ChInfo& chInfo ) const
{
   if( !IsValid( iNext ) ) iNext = 0;                                      // start enumeration
   else ++iNext;                                                           // continue enumeration at next channel pos 

   while( (iNext < NUMCHAN) && !IsDisplayed( iNext ) ) ++iNext;            // get info on next displayed chan, if any
   return( (iNext==NUMCHAN) ? FALSE : GetChannel( iNext, chInfo ) );
}


//=== Get***, Is***, Toggle***, Incr/Decr***, Set*** ================================================================== 
// 
//    Convenience functions for getting/setting individual configuration attributes of a specified channel. 
//
//    In Get/SetGainIndex() and Get/SetColorIndex(), the attribute is specified by a zero-based index among an ordered 
//    list of choices, rather than the actual value of the attribute.
//
//    NOTE:  Gain does not apply to the display of the pulse trains recorded on the digital input channels.  For these 
//    channels, gain is always 2^0 = 1 and cannot be changed.  All pulse trains are recorded whenever recording is on, 
//    so the record flag is always set for the digital input channels.  Computed channels are never recorded, so the
//    record flag is always cleared for these channels.
//
//    ARGS:       iPos  -- [in] cardinal pos of channel whose configuration attribute is to be get/set.
//                nOff  -- [in] the new channel offset value. 
//
//    RETURNS:    current or new value of configuration attribute.
//
int CCxChannel::GetOffset( const int iPos ) const 
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else return( m_offset[iPos] );
}

int CCxChannel::GetGain( const int iPos ) const
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else return( (int) m_gain[iPos] );
}

int CCxChannel::GetGainIndex( const int iPos ) const 
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else return( int(m_gain[iPos]) - CHANGAINMIN );
}

COLORREF CCxChannel::GetColor( const int iPos ) const
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( GetTraceColor( 0 ) ); }
   else return( GetTraceColor( (int) m_color[iPos] ) );
}

int CCxChannel::GetColorIndex( const int iPos ) const 
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else return( int(m_color[iPos]) );
}

BOOL CCxChannel::IsRecorded( const int iPos ) const
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( FALSE ); }
   else return( BOOL((m_onoff[iPos] & F_REC) == F_REC) );
}

BOOL CCxChannel::IsDisplayed( const int iPos ) const
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( FALSE ); }
   else return( BOOL((m_onoff[iPos] & F_DISP) == F_DISP) );
}

BOOL CCxChannel::ToggleRecord( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( FALSE ); }
   else if( IsComputed( iPos ) ) return( FALSE );                       // record flag always off for computed ch's!
   else if( IsDigital( iPos ) ) return( TRUE );                         // record flag always on for digi input ch's! 
   else
   {
      m_onoff[iPos] ^= F_REC;
      return( BOOL((m_onoff[iPos] & F_REC) == F_REC) );
   }
}

BOOL CCxChannel::ToggleDisplay( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( FALSE ); }
   else 
   {
      m_onoff[iPos] ^= F_DISP;
      return( BOOL((m_onoff[iPos] & F_DISP) == F_DISP) );
   }
}

int CCxChannel::IncrColor( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      ++m_color[iPos];
      if( m_color[iPos] == NCOLORS ) m_color[iPos] = 0;
      return( (int) m_color[iPos] );
   }
}

int CCxChannel::DecrColor( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      if( m_color[iPos] == 0 ) m_color[iPos] = NCOLORS-1;
      else --m_color[iPos];
      return( (int) m_color[iPos] );
   }
}

int CCxChannel::SetColorIndex( const int iPos, int nIndex ) 
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else
   {
      if( nIndex < 0 || nIndex >= NCOLORS ) nIndex = 0;
      m_color[iPos] = (BYTE) nIndex;
      return( nIndex );
   }
}

int CCxChannel::IncrGain( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      if( iPos < FIRSTDI )                                                 // cannot alter gain for digi input ch's 
      {
         ++m_gain[iPos];
         if( m_gain[iPos] > CHANGAINMAX ) m_gain[iPos] = CHANGAINMIN;
      }
      return( (int) m_gain[iPos] );
   }
}

int CCxChannel::DecrGain( const int iPos )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      if( iPos < FIRSTDI )                                                 // cannot alter gain for digi input ch's 
      {
         if( m_gain[iPos] == CHANGAINMIN ) m_gain[iPos] = CHANGAINMAX;
         else --m_gain[iPos];
      }
      return( (int) m_gain[iPos] );
   }
}

int CCxChannel::SetGainIndex( const int iPos, int nIndex )
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      if( iPos < FIRSTDI )                                                 // cannot alter gain for digi input ch's 
      {
         int n = nIndex + CHANGAINMIN;                                     // convert zero-based index to gain value 
         if( n < CHANGAINMIN ) n = CHANGAINMIN;
         else if( n > CHANGAINMAX ) n = CHANGAINMAX;
         m_gain[iPos] = char(n);
      }
      return( (int) m_gain[iPos] );
   }
}

int CCxChannel::SetOffset( const int iPos, const int nOff ) 
{
   if( !IsValid( iPos ) ) { ASSERT(FALSE); return( 0 ); }
   else 
   {
      m_offset[iPos] = (nOff < CHANOFFMIN) ? CHANOFFMIN : ((nOff > CHANOFFMAX) ? CHANOFFMAX : nOff);
      return( m_offset[iPos] );
   }
}


//=== SetDispRange ==================================================================================================== 
//
//    Set the vertical display range associated with this channel configuration.  Correct any illegal range spec.
//
//    ARGS:       yMin  -- [in] desired minimum display value; [out] value corrected, if necessary. 
//                yMax  -- [in] desired maximum display value; [out] value corrected, if necessary. 
//
//    RETURNS:    TRUE if range spec was accepted as given; FALSE if any corrections made. 
//
BOOL CCxChannel::SetDispRange( int& yMin, int& yMax )
{
   m_yDispMin = yMin;
   m_yDispMax = yMax;

   if( yMin < VDISPMIN ) yMin = VDISPMIN;                               // ensure new range bounds are valid...
   if( (yMin + VDISPMINRNG) > yMax ) yMax = yMin + VDISPMINRNG;         // correct as needed
   if( yMax > VDISPMAX ) yMax = VDISPMAX;
   if( (yMax - VDISPMINRNG) < yMin ) yMin = yMax - VDISPMINRNG;

   if( (m_yDispMin == yMin) && (m_yDispMax == yMax) ) return( TRUE );   // no changes made
   m_yDispMin = yMin;
   m_yDispMax = yMax;
   return( FALSE );
}


//=== Serialize [base override] ======================================================================================= 
//
//    Handles reading/writing the channel configuration object from/to a disk file via a serialization archive, 
//    including version control.
//
//       Version 1:  Channel display offsets were stored as 16bit integers.  Offsets & Y-axis range were assumed to be 
//                   in "b2sAIVolts", the encoded format for an AI device's A-to-D converter.
//       Version 2:  Channel display offsets stored as 32bit integers.  Offsets & Y-axis range in mV.
//
//    ARGS:       ar -- [in] the serialization archive.  
//
//    RETURNS:    NONE. 
//
//    THROWS:     -- The CArchive object can throw CMemoryException, CArchiveException, or CFileException.
//                -- We throw CArchiveException if schema number is not recognized.
//
void CCxChannel::Serialize( CArchive& ar )
{
   UINT nSchema = ar.GetObjectSchema();                                          // retrieve schema# 
   CTreeObj::Serialize( ar );                                                    // serialize base class stuff first

   if( ar.IsStoring() )                                                          // write to archive:
   {
      ar.Write( &(m_offset[0]), NUMCHAN * sizeof(int) );
      ar.Write( &(m_gain[0]), NUMCHAN * sizeof(char) );
      ar.Write( &(m_color[0]), NUMCHAN * sizeof(BYTE) );
      ar.Write( &(m_onoff[0]), NUMCHAN * sizeof(BYTE) );
      ar << m_yDispMax << m_yDispMin;
   }
   else                                                                          // read from archive: 
   {
      ASSERT( (m_type == CX_CHANCFG) && ((m_flags & CX_ISSETOBJ) == 0) );        //    validate obj type & flags
      int i,j;
      short shOff;
      switch( nSchema )                                                          //    handle the different schemas! 
      {
         case 1 :
            for( i = 0; i < NUMCHAN; i++ )
            {
               ar >> shOff;                                                      //    approx conversion fr b2sAIVolts 
               SetOffset( i, int(shOff) * 5 );                                   //    to mV, assuming 12bit AI dev
            }
            ar.Read( &(m_gain[0]), NUMCHAN * sizeof(char) );
            ar.Read( &(m_color[0]), NUMCHAN * sizeof(BYTE) );
            ar.Read( &(m_onoff[0]), NUMCHAN * sizeof(BYTE) );
            ar >> i >> j;                                                        //    convert Y-axis range to mV 
            i *= 5; j *= 5;
            SetDispRange( j, i ); 
            break;
         case 2 :
            ar.Read( &(m_offset[0]), NUMCHAN * sizeof(int) );
            ar.Read( &(m_gain[0]), NUMCHAN * sizeof(char) );
            ar.Read( &(m_color[0]), NUMCHAN * sizeof(BYTE) );
            ar.Read( &(m_onoff[0]), NUMCHAN * sizeof(BYTE) );
            ar >> m_yDispMax >> m_yDispMin;
            break;
         default :
            ::AfxThrowArchiveException( CArchiveException::badSchema );
            break;
      }
   }

   ASSERT_VALID( this );                                                         // check validity AFTER serializing
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump channel configuration data in an easy-to-read form to the supplied dump context. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CCxChannel::Dump( CDumpContext& dc ) const
{
   CTreeObj::Dump( dc );

   dc << _T("Channel Configuration: name (ch#): offset (mV), gain, color code, display on/off, record on/off\n");
   for( int i = 0; i < NUMCHAN; i++ ) 
   {
      dc << strChanLbls[i] << _T(" (") << i << _T("): ");
      dc << m_offset[i] << _T(", ") << INT(m_gain[i]) << _T(", ") << INT(m_color[i]);

      BYTE b = m_onoff[i];
      dc << _T(", ") << (((b & F_DISP) != 0) ? _T("ON") : _T("OFF"));
      dc << _T(", ") << (((b & F_REC) != 0) ? _T("ON\n") : _T("OFF\n"));
   }

   dc << _T("Vertical display range (mV): (") << m_yDispMin << _T(", ") << m_yDispMax << _T(")");
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate the channel configuration.  
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxChannel::AssertValid() const
{
   CTreeObj::AssertValid();

   for( int i = 0; i < NUMCHAN; i++ )
   {
      int j = m_offset[i];
      ASSERT( (j >= CHANOFFMIN) && (j <= CHANOFFMAX) );
      j = (int) m_gain[i];
      ASSERT( (j >= CHANGAINMIN) && (j <= CHANGAINMAX) );
      j = (int) m_color[i];
      ASSERT( (j >= 0) && (j < NCOLORS) );
   }

   ASSERT( (m_yDispMin >= VDISPMIN) && (m_yDispMin <= m_yDispMax - VDISPMINRNG) && (m_yDispMax <= VDISPMAX) );
}

#endif //_DEBUG




//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== GetPos ========================================================================================================== 
//
//    Get internal "cardinal position" of a CNTRLX data channel specified by channel type & physical channel #.
//
//    ARGS:       cht   -- [in] enumerated channel type. 
//                nCh   -- [in] physical channel #.
//
//    RETURNS:    cardinal pos of channel, or -1 if not valid
//
int CCxChannel::GetPos( ChType cht, const int nCh ) 
{
   int iPos = -1;
   if( (cht == AIChan) && (nCh >= 0) && (nCh < NUMAI) ) iPos = nCh;
   else if( (cht == CPChan) && (nCh >= 0) && (nCh < NUMCP) ) iPos = nCh + FIRSTCP;
   else if( (cht == DIChan) && (nCh >= 0) && (nCh < NUMDI) ) iPos = nCh + FIRSTDI;

   return( iPos );
}


