//===================================================================================================================== 
//
// cxchannel.h : Declaration of class CCxChannel.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXCHANNEL_H__INCLUDED_)
#define CXCHANNEL_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "treemap.h"                         // the CTreeMap/CTreeObj framework
#include "cxobj_ifc.h"                       // CNTRLX object interface:  common constants and other defines 


//===================================================================================================================== 
// Declaration of class CCxChannel
//===================================================================================================================== 
//
class CCxChannel : public CTreeObj
{
   DECLARE_SERIAL( CCxChannel )

   friend class CCxTreeMap;                 
   friend class CCxRuntime;

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:

   static const int NUMCHAN = 38;            // # of existing CNTRLX data channels.
   static const int FIRSTAI = 0;             // index of first of 16 analog inputs in the channel set.
   static const int NUMAI = 16;
   static const int FIRSTCP = 16;            // index of first of 6 "computed" signals in the channel set. 
   static const int NUMCP = 6;
   static const int FIRSTDI = 22;            // index of first of 16 digital inputs in the channel set. 
   static const int NUMDI = 16;
   static const int CHANGAINMIN = -5;        // allowed range for channel gain, expressed as power of 2.
   static const int CHANGAINMAX = 5;
   static const int CHANOFFMIN = -90000;     // allowed range for channel offset (mV)
   static const int CHANOFFMAX = 90000;
   static const int NCOLORS = 12;            // # of available trace colors
   static const int VDISPMIN = -99999;       // allowed values for Y-axis min/max (mV)
   static const int VDISPMAX = 99999;
   static const int VDISPMINRNG = 500;       // min Y-axis range (mV)

   static const BYTE F_REC;                  // state flags:  channel record ON/OFF (bit set = "ON"),
   static const BYTE F_DISP;                 //    channel display ON/OFF (bit set = "ON")
   static LPCTSTR strChanLbls[NUMCHAN];      // name of signal represented by each channel, fixed in the CNTRLX design
   static const COLORREF trColors[NCOLORS];  // RGB values for the available trace colors
   static LPCTSTR strClrLbls[NCOLORS];       // and the corresponding names of those colors

public:
   typedef enum                              // the different types of channels supported by CNTRLX:
   {
      AIChan = 0,                            //    analog inputs
      DIChan,                                //    digital inputs
      CPChan                                 //    "computed" signals
   } ChType;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   int      m_offset[NUMCHAN];               // channel display offset in millivolts
   char     m_gain[NUMCHAN];                 // channel display gain, expressed as power of 2
   BYTE     m_color[NUMCHAN];                // channel display trace color (index into COLOREF array)
   BYTE     m_onoff[NUMCHAN];                // channel record/display ON/OFF flags 

   int      m_yDispMin, m_yDispMax;          // vertical display range in millivolts

public:
   struct ChInfo                             // for retrieving a channel's attributes in a single record
   {
      int      iPos;                         //    data channel's cardinal pos in the channel configuration 
      ChType   chType;                       //    type of data channel
      int      nCh;                          //    physical channel # (vs cardinal pos)
      BOOL     bDisplay;                     //    is channel tagged for display?
      BOOL     bRecord;                      //    is channel tagged for recording?
      int      iOffset;                      //    channel display offset
      int      iGain;                        //    channel gain (power of 2)
      COLORREF crDisplay;                    //    channel display RGB 
   };


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxChannel( const CCxChannel& src );                  // no copy constructor defined
   CCxChannel& operator=( const CCxChannel& src );       // no assignment operator defined 

protected:
   CCxChannel() { RestoreDefaults(); }                   // constructor required for dyn object creation mechanism 
   ~CCxChannel() {}                                      // destroy channel cfg obj -- freeing any allocated memory

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f); // initialize channel config after default construction
   VOID Copy( const CTreeObj* pSrc );                    // make THIS channel config a copy of the specified config

public:
   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a channel cfg from a different treemap 
         const CWordToWordMap& depKeyMap);


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   BOOL CanRemove() const                                // prevent removal of "predefined" objects
   { return( (m_flags & CX_ISPREDEF) == 0 ); }           //

                                                         // ranges for various channel configuration attributes...
   static VOID GetOffsetRange( int& nMin, int& nMax ) { nMin = (int)CHANOFFMIN; nMax = (int)CHANOFFMAX; }
   static int GetGainMin() { return( int(CHANGAINMIN) ); }
   static int GetGainMax() { return( int(CHANGAINMAX) ); }
   static int GetNumTraceColors() { return( int(NCOLORS) ); }

   static int GetNumChannels() { return( NUMCHAN ); }    // the # of data channels defined in CNTRLX

   static LPCTSTR GetLabel( const int iPos )             // return string constant describing specified channel
   {
      if( IsValid( iPos ) ) return( strChanLbls[iPos] );
      else { ASSERT( FALSE ); return( NULL ); }
   }
   
   static COLORREF GetTraceColor( const int i )          // return RGB color associated with color index specified
   {
      if( (i>=0) && (i<NCOLORS) ) return( trColors[i] );
      else { ASSERT( FALSE ); return( trColors[0] ); } 
   }
   static LPCTSTR GetTraceColorLabel( const int i )      // return descriptive name of trace color
   {
      if( (i>=0) && (i<NCOLORS) ) return( strClrLbls[i] );
      else { ASSERT( FALSE ); return( strClrLbls[0] ); }
   }

   static BOOL IsValid( const int iPos )                 // does cardinal pos identify an existing data channel?
   { return( iPos>=0 && iPos<NUMCHAN ); }
   static BOOL IsAnalog( const int iPos )                // does cardinal pos point to an analog input channel?
   { return( iPos>=0 && iPos<FIRSTCP ); }
   static BOOL IsComputed( const int iPos )              // does cardinal pos point to a "computed" data channel?
   { return( iPos>=FIRSTCP && iPos<FIRSTDI ); }
   static BOOL IsDigital( const int iPos )               // does cardinal pos point to a digital input channel?
   { return( iPos>=FIRSTDI && iPos<NUMCHAN ); }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public: 
   VOID RestoreDefaults();                               // initialize to default channel configuration 
   VOID ClearAll();                                      // no channels displayed or recorded, g=0, offset=0
   VOID SpaceEvenly( int nSpacing );                     // distribute displayed channel traces along y-axis

   int GetNDisplay() const;                              // # of channels currently selected for display
   int GetNRecord() const;                               // # of channels currently selected for recording

   int GetRecordedAIChannels( int* piChan ) const;       // retrieve list of AI channels selected for recording 

   BOOL GetChannel( const int iPos, ChInfo& chInfo ) const;    // retrieve all of a channel's attributes...
   BOOL GetChannel(ChType cht, int nCh, ChInfo& chInfo) const
   { return( GetChannel( GetPos( cht, nCh ), chInfo ) ); }

   BOOL GetNextDisplayed( int& iNext, ChInfo& chInfo ) const;  // for enumerating channels currently tagged for display

   int GetOffset( const int iPos ) const;                // methods for retrieving individual attrib (by cardinal pos)
   int GetGain( const int iPos ) const; 
   int GetGainIndex( const int iPos ) const;             // gain as zero-based index among available choices
   COLORREF GetColor( const int iPos ) const; 
   int GetColorIndex( const int iPos ) const;            // color as zero-based index among available choices
   BOOL IsRecorded( const int iPos ) const; 
   BOOL IsDisplayed( const int iPos ) const; 

   BOOL ToggleRecord( const int iPos );                  // toggle the record ON/OFF flag 
   BOOL ToggleDisplay( const int iPos );                 // toggle the display ON/OFF flag
   int IncrColor( const int iPos );                      // for cycling trace color among available choices 
   int DecrColor( const int iPos );                      // 
   int SetColorIndex( const int iPos, int nIndex );      // set trace color via zero-based index
   int IncrGain( const int iPos );                       // for cycling among available gain values
   int DecrGain( const int iPos );                       //
   int SetGainIndex( const int iPos, int nIndex );       // set gain via zero-based index
   int SetOffset( const int iPos, const int nOff );      // set a channel's display offset

                                                         // overloaded methods specify channel by type & physical #
   int GetOffset( ChType cht, const int nCh ) const { return( GetOffset( GetPos( cht, nCh ) ) ); }
   int GetGain( ChType cht, const int nCh ) const { return( GetGain( GetPos( cht, nCh ) ) ); }
   int GetGainIndex( ChType cht, const int nCh ) const { return( GetGainIndex( GetPos( cht, nCh ) ) ); }
   COLORREF GetColor( ChType cht, const int nCh ) const { return( GetColor( GetPos( cht, nCh ) ) ); }
   int GetColorIndex( ChType cht, const int nCh ) const { return( GetColorIndex( GetPos( cht, nCh ) ) ); }
   int IsRecorded( ChType cht, const int nCh ) const { return( IsRecorded( GetPos( cht, nCh ) ) ); }
   int IsDisplayed( ChType cht, const int nCh ) const { return( IsDisplayed( GetPos( cht, nCh ) ) ); }
   int ToggleRecord( ChType cht, const int nCh ) { return( ToggleRecord( GetPos( cht, nCh ) ) ); }
   int ToggleDisplay( ChType cht, const int nCh ) { return( ToggleDisplay( GetPos( cht, nCh ) ) ); }
   int IncrColor( ChType cht, const int nCh ) { return( IncrColor( GetPos( cht, nCh ) ) ); }
   int DecrColor( ChType cht, const int nCh ) { return( DecrColor( GetPos( cht, nCh ) ) ); }
   int SetColorIndex( ChType cht, const int nCh, int nIndex ) { return( SetColorIndex( GetPos(cht,nCh), nIndex ) ); }
   int IncrGain( ChType cht, const int nCh ) { return( IncrGain( GetPos( cht, nCh ) ) ); }
   int DecrGain( ChType cht, const int nCh ) { return( DecrGain( GetPos( cht, nCh ) ) ); }
   int SetGainIndex( ChType cht, const int nCh, int nIndex ) { return( SetGainIndex( GetPos(cht,nCh), nIndex ) ); }
   int SetOffset( ChType cht, const int nCh, const int nOff ) { return( SetOffset( GetPos( cht, nCh ), nOff ) ); }

   VOID GetDispRange( int& yMin, int& yMax ) const       // get the vertical display range
   { yMin = m_yDispMin; yMax = m_yDispMax; }
   BOOL SetDispRange( int& yMin, int& yMax );            // set the vertical display range

   void Serialize( CArchive& ar );                       // for reading/writing channel configuration from/to disk file


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
private:
   static int GetPos( ChType cht, const int nCh );       // translate chan type & physical # to internal cardinal pos 

};


#endif   // !defined(CXCHANNEL_H__INCLUDED_)
