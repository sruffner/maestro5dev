//=====================================================================================================================
//
// cxcontmode.h : Declaration of the ContMode controller CCxContMode and ContMode-specific control panel dialogs,
//                CCxContProtoDlg and CCxContFixTgtsDlg.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXCONTMODE_H__)
#define CXCONTMODE_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "util.h"                // for RTX-based CElapsedTime

#include "cxipc.h"               // constants and IPC interface data structs for CNTRLX-CXDRIVER communications
#include "cxobj_ifc.h"           // CNTRLX object interface:  common constants and other defines
#include "cxviewhint.h"          // CCxViewHint -- for communicating doc/view changes...
#include "gridctrl\litegrid.h"   // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in
#include "cxobjcombo.h"          // CCxObjCombo -- for selecting among children of a parent obj defined in CCxDoc
#include "cxfileedit.h"          // CCxFileEdit -- a special edit ctrl for specifying path for CNTRLX data files
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs
#include "cxmodecontrol.h"       // CCxModeControl -- base class for CNTRLX mode controllers


class CCxContMode;               // forward declarations
class CCxFixRewDlg;
class CCxVideoDspDlg;
class CCxEyelinkDlg;
class CCxControlPanel;


//=====================================================================================================================
// Declaration of class CCxContProtoDlg  -- the "Protocol" dialog for ContMode
//=====================================================================================================================
//
class CCxContProtoDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxContProtoDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_CONTCP1;       // dialog template resource ID for this dialog


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxObjCombo       m_cbRunSet;             // combo box selects CNTRLX continuous-run set object to use
   CCxObjCombo       m_cbCurrRun;            // combo box selects/displays current run stimulus
   CCxObjCombo       m_cbChanCfg;            // combo box displays channel config to use during ContMode
   CComboBox         m_cbRunMode;            // combo box selects stimulus run execution mode
   CCxFileEdit       m_fecDataPath;          // special edit ctrl displays/selects path for next cont-mode data file
   CButton           m_btnRecordSpks;        // check box; if checked, spike waveform data is recorded & saved with
                                             // slow-sampled analog/digital data in the cont-mode data file


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxContProtoDlg(const CCxContProtoDlg& src);          // no copy constructor or assignment operator defined
   CCxContProtoDlg& operator=( const CCxContProtoDlg& src );

public:
   CCxContProtoDlg() : CCxControlPanelDlg( IDD ) {}
   ~CCxContProtoDlg() {}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnDestroy();                             // save MRU data dir to registry prior to destruction
   afx_msg void OnPreBrowse(NMHDR *pNMH, LRESULT *pRes); // to prevent data file browsing when not idle
   afx_msg void OnComboChange( UINT id );                // respond to change in one of the combo box controls
   afx_msg void OnBtnClicked( UINT id );                 // respond to btn click on PB controls
   DECLARE_MESSAGE_MAP()


//=====================================================================================================================
// ATTRIBUTES -- the current values of control parameters represented in the dialog
//=====================================================================================================================
public:
   WORD GetRunSet() { return( m_cbRunSet.GetObjKey() ); }
   WORD GetCurrentRun() { return( m_cbCurrRun.GetObjKey() ); }
   WORD GetChanCfg() { return( m_cbChanCfg.GetObjKey() ); }
   int GetRunMode() { return( m_cbRunMode.GetCurSel() ); }
   BOOL IsSaveSpikes() { return( BOOL(m_btnRecordSpks.GetCheck() != 0) ); }
   VOID GetNextDataFile( CString& strPath ) { strPath = m_fecDataPath.GetCurrentPath(); }


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // call this to refresh appearance of dialog
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change

                                                         // for changing selected controls within dlg:
   VOID IncrementNextDataFile()                          //    increment numeric ext of next cont-mode data file
   {
      m_fecDataPath.IncrementFileExt();
   }
   BOOL SetCurrentRun( const WORD wKey );                //    change the current stimulus run combo box selection
};





//=====================================================================================================================
// Declaration of class CCxContFixTgtsDlg -- the "Fixation & Targets" dialog for ContMode
//=====================================================================================================================
//
class CCxContFixTgtsDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxContFixTgtsDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_CONTCP2;             // dialog template resource ID for this dialog

   struct CActiveTgt                               // a target in the "active target list"
   {
      WORD wKey;                                   //    CNTRLX target object key
      BOOL bOn;                                    //    TRUE if target is currently turned ON
      double dHPos, dVPos;                         //    current target position (deg)
      double dSpeed, dDir;                         //    target pattern speed (deg/s) and direction (deg CCW)
   };

public:
   static const double MIN_ACVTGTPOS;              // min/max/default/increment for active target position
   static const double MAX_ACVTGTPOS;
   static const double DEF_ACVTGTPOS;
   static const double INC_ACVTGTPOS; 

   static const double MIN_ACVTGTPATSPEED;         // min/max/default/increment for active target pattern speed
   static const double MAX_ACVTGTPATSPEED;
   static const double DEF_ACVTGTPATSPEED;
   static const double INC_ACVTGTPATSPEED; 

   static const double DEF_ACVTGTPATDIR;           // default/increment for active target pattern dir in deg CCW; 
   static const double INC_ACVTGTPATDIR;           // always restricted to unit circle [0..360deg).

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   int               m_iFixTgt1;                   // fixation tgt #1 (index into active tgt list, or -1 = "none"),
   int               m_iFixTgt2;                   // fixation tgt #2 (ditto)
   int               m_iTrackTgt;                  // cursor-tracking tgt (ditto)

   int               m_nActive;                    // the "active target list" for ContMode
   CActiveTgt        m_activeTgts[MAX_ACTIVETGTS]; //

   CButton           m_btnTgtAdd;                  // PBs:  add tgt to active target list
   CButton           m_btnTgtDel;                  //    delete tgt from active list
   CButton           m_btnTgtClear;                //    clear the active target list

   CLiteGrid         m_grid;                       // spreadsheet-like control displays the "active target list"
   BOOL              m_bAddingTarget;              // if TRUE, adding a tgt to active list; else, replace existing tgt
   WORD              m_wLastTgtKey;                // key of last target added to the active list


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxContFixTgtsDlg( const CCxContFixTgtsDlg& src );    // no copy constructor or assignment operator defined
   CCxContFixTgtsDlg& operator=( const CCxContFixTgtsDlg& src );

public:
   CCxContFixTgtsDlg();
   ~CCxContFixTgtsDlg() {}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnBtnClicked( UINT id );                 // handle a button press on dlg
   afx_msg void OnUpdTgtDel(CCmdUI* pCmdUI );            // update enable state of "Delete" button IDC_CONT_TGT_DEL

   DECLARE_MESSAGE_MAP()

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   int GetActiveFixTgt1()        { return( m_iFixTgt1 ); }
   int GetActiveFixTgt2()        { return( m_iFixTgt2 ); }
   int GetActiveTrackTgt()       { return( m_iTrackTgt ); }

   int GetNumActiveTgts()        { return( m_nActive ); }

   WORD GetAcvTgtKey( int i )    { return( (i < 0 || i >= m_nActive) ? CX_NULLOBJ_KEY : m_activeTgts[i].wKey ); }
   BOOL GetAcvTgtOn( int i )     { return( (i < 0 || i >= m_nActive) ? FALSE : m_activeTgts[i].bOn ); }
   BOOL SetAcvTgtOn( int i, BOOL bOn );
   double GetAcvTgtHPos( int i ) { return( (i < 0 || i >= m_nActive) ? 0.0 : m_activeTgts[i].dHPos ); }
   double GetAcvTgtVPos( int i ) { return( (i < 0 || i >= m_nActive) ? 0.0 : m_activeTgts[i].dVPos ); }
   BOOL SetAcvTgtPos( int i, double x, double y );

   double GetAcvTgtPatSpeed(int i) { return( (i < 0 || i >= m_nActive) ? 0.0 : m_activeTgts[i].dSpeed ); }
   BOOL SetAcvTgtPatSpeed(int i, double speed);
   
   double GetAcvTgtPatDir(int i) { return( (i < 0 || i >= m_nActive) ? 0.0 : m_activeTgts[i].dDir ); }
   BOOL SetAcvTgtPatDir(int i, double dir);
   
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // to refresh dlg appearance -- updates enable state of ctrls
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change

//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   static BOOL CALLBACK GridDispCB(                      // callback invoked by grid to get cell text & display info
            GV_DISPINFO *pDispInfo, LPARAM lParam );
   static BOOL CALLBACK GridEditCB(                      // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // callback invoked when upon termination of inplace editing
            ENDEDITINFO *pEEI, LPARAM lParam );

   static BOOL RangeLimit( int& i, int iMin, int iMax )  // limit integer value to a specified range; return TRUE iff
   {                                                     // value did not have to be adjusted to fit range
      if( i < iMin ) { i = iMin; return( FALSE ); }
      else if( i > iMax ) { i = iMax; return( FALSE ); }
      else return( TRUE );
   }
   static BOOL RangeLimit( double& d,                    // analogously for floating-point value
                    double dMin, double dMax )
   {
      if( d < dMin ) { d = dMin; return( FALSE ); }
      else if( d > dMax ) { d = dMax; return( FALSE ); }
      else return( TRUE );
   }

};





//=====================================================================================================================
// Declaration of class CCxContMode -- the mode controller for ContMode
//=====================================================================================================================
//
class CCxContMode : public CCxModeControl
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
public:
   static const int NUMRUNMODES = 3;            // alternate stimulus run modes:
   static const int MANUAL;                     //    start/stop run manually, recording state entirely independent
   static const int AUTORECORD;                 //    start/stop recording with stimulus run
   static const int REPEAT;                     //    repeat current run indefinitely, with delay betw reps; record
                                                //       data -- including the delay prior to each presentation
   static LPCTSTR strModes[NUMRUNMODES];        // short human-readable names for each stimulus run mode


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxContProtoDlg*     m_pProtoDlg;            // the "Protocol" dlg page
   CCxContFixTgtsDlg*   m_pTgtsDlg;             // the "Active Targets" dlg page
   CCxFixRewDlg*        m_pFixRewDlg;           // the "Fix/Reward" dlg page
   CCxVideoDspDlg*      m_pVideoDspDlg;         // the "RMVideo Display" dlg page
   CCxEyelinkDlg*       m_pEyelinkDlg;          // the "Eyelink" dlg page

   DWORD                m_dwLastOpState;        // operational state of CXDRIVER the last time we checked
   BOOL                 m_bWaiting;             // if TRUE, waiting for next run presentation in Repeat exec mode
   CElapsedTime         m_waitTime;             // times the delay between run repetitions in Repeat exec mode

   CElapsedTime         m_trackUpdTime;         // times interval between updates of target tracking mouse cursor

   CString              m_strShadowPath;        // if recorded data file is to be written to a remote drive, CXDRIVER
                                                // writes it to this shadow file on the local disk (empty if not used)

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxContMode( const CCxContMode& src );                // no copy constructor or assignment operator defined
   CCxContMode& operator=( const CCxContMode& src );

public:
   CCxContMode( CCxControlPanel* pPanel );
   ~CCxContMode() {}                                     // (created dlgs destroyed by mode control panel container)


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   BOOL IsStimRunning() const { return( BOOL(0 != (m_dwLastOpState & (CX_FC_RUNON|CX_FC_RUNSTOPPING))) ); }
   BOOL IsStimWaiting() const { return( m_bWaiting ); }
   BOOL IsStimStopping() const { return( BOOL(0 != (m_dwLastOpState & CX_FC_RUNSTOPPING)) ); }
   BOOL IsRecording() const { return( BOOL(0 != (m_dwLastOpState & CX_FC_RECORDING)) ); }
   BOOL IsFixating() const { return( BOOL(0 != (m_dwLastOpState & CX_FC_FIXATING)) ); }
   BOOL IsActive() const { return( (m_dwLastOpState != 0) || m_bWaiting ); }


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   virtual BOOL InitDlgs();                              // add dlgs for this op mode to the control panel container

   virtual VOID Service();                               // update runtime state in this mode *** called frequently ***
   virtual BOOL Enter();                                 // do any inits upon entering this mode
   virtual BOOL Exit();                                  // clean up prior to exiting this mode

   virtual BOOL CanUpdateVideoCfg()                      // TRUE when update of the video display cfg is permissible
   {
      return( !IsActive() );
   }
   virtual BOOL CanUpdateFixRewSettings()                // TRUE when update of fixation/reward settings is permissible
   {
      return( TRUE );
   }
   virtual LPCTSTR GetModeTitle()                        // string constant describing this op mode
   {
      return( _T("Continuous Mode") );
   }

   VOID Go();                                            // start the currently selected stimulus run
   VOID Halt();                                          // stop the stimulus run in progress at end of curr duty cycle
   VOID Abort();                                         // abort stimulus run immediately
   VOID Restart();                                       // abort current stimulus run and restart it

   BOOL ToggleRecord();                                  // toggle data recording ON or OFF (MANUAL mode only)
   BOOL ToggleFixate();                                  // toggle fixation ON or OFF
   BOOL UpdateActiveFixTargets();                        // send active tgt fixation designations to CXDRIVER
   BOOL UpdateActiveTarget( const int iTgt );            // update on/off state, pos of specified tgt in active list
   BOOL ToggleCursorTrackingTarget();                    // toggle ON/off state of cursor tracking tgt, if it exists
   VOID HandleTrackingTargetPatternUpdate(UINT nID);     // handler for global keyboard shortcuts that incr/decr the 
                                                         //    pattern speed or dir of the "cursor tracking" tgt
   VOID ChangeTraces();                                  // change what's currently displayed in data trace window


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   VOID UpdateCursorTrackingTarget();                    // if "cursor tracking" tgt is on, make it follow mouse cursor
                                                         //    inside the eye-target position plot
   BOOL StartStimulusRun();                              // issue CXDRIVER cmd to start stimulus run
   BOOL StopStimulusRun( BOOL bStopNow, BOOL bStopRec ); // issue CXDRIVER cmd to stop stimulus run now or at end of
                                                         // duty cycle; optionally abort recording as well
   BOOL StartRecord();                                   // issue CXDRIVER cmd to start data recording in ContMode
   BOOL StopRecord( BOOL bSave );                        // issue CXDRIVER cmd to stop data recording in ContMode
};


#endif // !defined(CXCONTMODE_H__)

