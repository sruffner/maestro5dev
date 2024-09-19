//===================================================================================================================== 
//
// cxtestmode.h : Declaration of the TestMode controller CCxTestMode and TestMode-specific control panel dialogs,
//                CCxAnalogIODlg and CCxEventTimerDlg.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXTESTMODE_H__)
#define CXTESTMODE_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "util.h"                // for RTX-based CElapsedTime

#include "cxipc.h"               // constants and IPC interface data structs for CNTRLX-CXDRIVER communications
#include "cxobj_ifc.h"           // CNTRLX object interface:  common constants and other defines 
#include "cxviewhint.h"          // CCxViewHint -- for communicating doc/view changes...
#include "cxobjcombo.h"          // CCxObjCombo -- for selecting among existing "sibling" objects in CCxDoc
#include "gridctrl\litegrid.h"   // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs
#include "cxmodecontrol.h"       // CCxModeControl -- base class for CNTRLX mode controllers


class CCxTestMode;               // forward declarations
class CCxControlPanel;


//===================================================================================================================== 
// Declaration of class CCxAnalogIODlg  -- the Analog I/O test panel dialog
//===================================================================================================================== 
//
class CCxAnalogIODlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxAnalogIODlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_AIOCP;         // dialog template resource ID for this dialog
   typedef enum {                            // three alternative units for displaying voltages: V, mV, raw DAC
      VOLTS = 0,
      MILLIVOLTS,
      RAW
   } DSPUNIT;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CLiteGrid         m_chanGrid;             // grid control displaying analog input/output channel data
   CCxObjCombo       m_cbSelChan;            // combo box used to select displayed channel config for TestMode

   int               m_iUnits;               // units of displayed analog data: VOLTS, MILLIVOLTS, or RAW


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxAnalogIODlg( const CCxAnalogIODlg& src );          // no copy constructor or assignment operator defined
   CCxAnalogIODlg& operator=( const CCxAnalogIODlg& src ); 

public: 
   CCxAnalogIODlg();
   ~CCxAnalogIODlg() {}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnOp( UINT cmdID );                      // handle PB-initiated operations on this dialog
   afx_msg void OnChangeChanCfg();                       // changed selection of displayed channel configuration
   afx_msg void OnGridRClk( NMHDR* pNMHDR,               // handle right-click on channel grid -- popup menu selects 
                            LRESULT* pResult );          // an operation...

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // refresh dlg ctrls IAW current runtime state, data
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change
   VOID ClearGridFocus() { m_chanGrid.SetFocusCell(-1,-1); }

//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   static BOOL CALLBACK GridDispCB(                      // callback invoked by grid to get cell text & display info 
            GV_DISPINFO *pDispInfo, LPARAM lParam ); 
   static BOOL CALLBACK GridEditCB(                      // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // callback invoked when upon termination of inplace editing 
            ENDEDITINFO *pEEI, LPARAM lParam );

   BOOL IsValidAOCell( CCellID c );                      // does this cell display an analog output channel voltage?
   BOOL IsValidAICell( CCellID c );                      // does this cell display an analog input channel voltage?

};




//===================================================================================================================== 
// Declaration of class CCxEventTimerDlg  -- the Event Timer DIO test panel dialog
//===================================================================================================================== 
//
class CCxEventTimerDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxEventTimerDlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_TMRCP;         // dialog template resource ID for this dialog


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CLiteGrid         m_chanGrid;             // grid control displaying event timer diagnostic data 


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxEventTimerDlg( const CCxEventTimerDlg& src );      // no copy constructor or assignment operator defined
   CCxEventTimerDlg& operator=( const CCxEventTimerDlg& src ); 

public: 
   CCxEventTimerDlg(): CCxControlPanelDlg( IDD ) {}      // constructor
   ~CCxEventTimerDlg() {}                                // destructor


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnOp( UINT cmdID );                      // handle PB-initiated operations 

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // refresh dlg ctrls IAW current runtime state, data
   VOID ClearGridFocus() { m_chanGrid.SetFocusCell(-1,-1); }

//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   static BOOL CALLBACK GridDispCB(                      // callback invoked by grid to get cell text & display info 
            GV_DISPINFO *pDispInfo, LPARAM lParam ); 
   static BOOL CALLBACK GridEditCB(                      // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // callback invoked when upon termination of inplace editing 
            ENDEDITINFO *pEEI, LPARAM lParam );

   BOOL IsValidTDOCell( CCellID c );                     // does this cell correspond to a digital output channel?
   BOOL IsValidTDICell( CCellID c );                     // does this cell contain info on digital input events?

};




//===================================================================================================================== 
// Declaration of class CCxTestMode -- the mode controller for "Test & Calibration Mode" 
//===================================================================================================================== 
//
class CCxTestMode : public CCxModeControl
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const double REFRESHINTV;             // interval between refreshes of the active dlg (in microsecs)
   static const DWORD F_AI_RUNNING;             // test mode state flags
   static const DWORD F_TMRLOOPON;
   static const DWORD F_TMRLOOPDONE;
   static const DWORD F_TMRLOOPFAIL;
   static const DWORD F_TMRREPWRITE;

public:
   typedef enum {                               // timer loopback test status
      TLBS_NOTRUNNING = 0,
      TLBS_RUNNING,
      TLBS_DONE,
      TLBS_FAILED 
  } TLBStatus;

   // when recording AI data, Test Mode keeps track of the last sample recorded, the average, and the std deviation.
   typedef enum { AID_LAST = 0, AID_AVG, AID_STD } AIDatum;

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CCxAnalogIODlg*   m_pAIODlg;                 // ptr to the "Analog I/O" dialog
   CCxEventTimerDlg* m_pTmrDlg;                 // ptr to the "Event Timer DIO" dialog

   DWORD          m_dwState;                    // current value of state flags
   CElapsedTime   m_tSinceLastRefresh;          // elapsed time since the active dlg was refreshed

   float          m_fAO[CX_AIO_MAXN];           // current voltages on analog output channels (in volts)
   
   // AI channel stats: Let N=#channels. [0..N-1] is the most recent voltage sample recorded per channel, [N..2N-1] is
   // the average voltage per channel, and [2N..3N-1] is the standard deviation. All are in volts.
   float          m_fAIData[3*CX_AIO_MAXN]; 

   int            m_iAOWave;                    // current AO channel on which test waveform is being generated.

                                                // DIO input event statistics (since last reset of event timer)
   int            m_nEvents[CX_TMR_MAXN];       // # of pulses detected 
   float          m_fTLastEvent[CX_TMR_MAXN];   // time of occurrence of last event (in seconds)
   float          m_fMeanIEI[CX_TMR_MAXN];      // mean interevent interval (in seconds)
   DWORD          m_dwDIn;                      // DI event "mask" for the last event recorded
   DWORD          m_dwDOut;                     // the current digital output vector (bit0=ch0, etc.)

   int            m_nLoopTest;                  // current iteration of loopback test


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxTestMode( const CCxTestMode& src );                // no copy constructor or assignment operator defined
   CCxTestMode& operator=( const CCxTestMode& src ); 

public: 
   CCxTestMode( CCxControlPanel* pPanel );
   ~CCxTestMode() {}                                     // (created dlgs destroyed by mode control panel container)


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual BOOL InitDlgs();                              // add dlgs for this op mode to the control panel container

   virtual VOID Service();                               // update runtime state in this mode *** called frequently *** 
   virtual BOOL Enter();                                 // do any inits upon entering this mode 
   virtual BOOL Exit();                                  // clean up prior to exiting this mode

   virtual BOOL CanUpdateVideoCfg() { return(FALSE); }   // TRUE when update of the video display cfg is permissible 
   virtual BOOL CanUpdateFixRewSettings()                // TRUE when update of fixation/reward settings is permissible
   { 
      return( FALSE ); 
   }
   virtual LPCTSTR GetModeTitle()                        // string constant describing this op mode
   {
      return( _T("Test and Calibration Mode") );
   }

   VOID ToggleAISampling();                              // pause/resume periodic sampling of AI channels in test mode 
   float SetAOChannel( float fVolt, int iCh = -1 );      // set new voltage on specified AO channel
   int SetAOChannelRaw( int iDac, int iCh = -1 );        // set new voltage on specified AO channel as raw DAC value
   BOOL CalibrateAI();                                   // perform quick, in-situ calibration of AI dev (if possible) 

   BOOL RunTestWaveform( int iCh );                      // start/stop continuous test waveform on selected AO channel 
   int GetTestWaveformCh()                               // AO channel on which test waveform currently running; -1 if 
   { return( m_iAOWave ); }                              // test waveform is not on

   BOOL IsAIPaused() { return( BOOL((m_dwState & F_AI_RUNNING) == 0) ); }
   float GetAOChannel( int iCh ) { return( IsAOChan(iCh) ? m_fAO[iCh] : 0.0f ); }
   int GetAOChannelRaw( int iCh );
   float GetAIChannel(int iCh, AIDatum which);
   int GetAIChannelRaw(int iCh, AIDatum which);

   VOID ResetTimer();                                    // reset event timer and clear input stats, DO channels 
   VOID ToggleTimerOut( int iCh );                       // toggle state of DO channel on event timer device 
   TLBStatus GetTimerLoopStatus();                       // "loopback" test status: off/on/succeeded/failed
   VOID StartTimerLoop();                                // start "loopback" test on event timer device 
   BOOL IsTimerRepetitiveWriteOn();                      // TRUE if the "repetitive timer write" test fcn engaged
   VOID ToggleTimerRepetitiveWrite();                    // toggle on/off state of the "repetitive timer write fcn

   BOOL IsTDOChannelOn( int iCh ){ return( IsTDOChan(iCh) ? BOOL( (m_dwDOut & ((DWORD) (1<<iCh))) != 0 ) : FALSE ); }
   DWORD GetTDOChanVec() { return( m_dwDOut ); }
   int GetTimerInputEvents( int iCh )  { return( IsTDIChan(iCh) ? m_nEvents[iCh] : 0 ); }
   float GetTimerLastEventTime( int iCh ) { return( IsTDIChan(iCh) ? m_fTLastEvent[iCh] : 0.0f ); }
   float GetTimerMeanEventIntv( int iCh )  { return( IsTDIChan(iCh) ? m_fMeanIEI[iCh] : 0.0f ); }
   DWORD GetTDILastEventMask() { return( m_dwDIn ); }
   BOOL IsTDILastEventOn( int iCh ) { return( IsTDIChan(iCh) ? BOOL( (m_dwDIn & ((DWORD) (1<<iCh))) != 0 ) : FALSE ); } 


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   VOID Initialize();                                    // initialize test mode's runtime state
   BOOL SetTimerDOPort( DWORD dwVec );                   // set bit pattern on event timer's DO port 

};


#endif // !defined(CXTESTMODE_H__)

