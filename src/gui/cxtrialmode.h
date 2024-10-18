//=====================================================================================================================
//
// cxtrialmode.h :   Declaration of the TrialMode controller CCxTrialMode and TrialMode-specific control panel dialogs,
//                   CCxTrialProtoDlg and CCxTrialParmsDlg.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXTRIALMODE_H__)
#define CXTRIALMODE_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "util.h"                // for RTX-based CElapsedTime
#include "numedit.h"             // CNumEdit -- a restricted-format numeric-only edit control
#include "cxipc.h"               // constants and IPC interface data structs for CNTRLX-CXDRIVER communications
#include "cxobj_ifc.h"           // CNTRLX object interface:  common constants and other defines
#include "cxviewhint.h"          // CCxViewHint -- for communicating doc/view changes...
#include "gridctrl\litegrid.h"   // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in
#include "cxobjcombo.h"          // CCxObjCombo -- for selecting among children of a parent obj defined in CCxDoc
#include "cxfileedit.h"          // CCxFileEdit -- a special edit ctrl for specifying path for CNTRLX data files
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs
#include "cxmodecontrol.h"       // CCxModeControl -- base class for CNTRLX mode controllers
#include "cxtrialseq.h"          // CCxTrialSequencer -- helper class for CCxTrialMode

#include "cxrpdistrodlg.h"       // CCxRPDistroDlg -- the "R/P Distro" dialog page

class CCxTrialMode;              // forward declarations
class CCxFixRewDlg;
class CCxVideoDspDlg;
class CCxEyelinkDlg;
class CCxControlPanel;


//=====================================================================================================================
// Declaration of class CCxTrialProtoDlg  -- the "Protocol" dialog for TrialMode
//=====================================================================================================================
//
class CCxTrialProtoDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxTrialProtoDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TRIALCP1;      // dialog template resource ID for this dialog

   static const int MIN_TRIALDELAY;          // allowed range for the inter-trial delay (ms)
   static const int MAX_TRIALDELAY;
   static const int MIN_IGNORETIME;          // allowed range for ignore threshold time (ms)
   static const int MAX_IGNORETIME;
   static const int MIN_AUTOSTOPCNT;         // allowed range for autostop trial or block count
   static const int MAX_AUTOSTOPCNT;


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxObjCombo       m_cbTrialSet;           // combo box selects CNTRLX trial set object to use
   CCxObjCombo       m_cbCurrTrial;          // combo box selects/displays current trial running (or to be run next)
   CComboBox         m_cbSubsetSeq;          // combo box selects trial subset sequencing mode
   CComboBox         m_cbTrialSeq;           // combo box selects trial sequencing mode
   CCxFileEdit       m_fecDataPath;          // special edit ctrl displays/selects path for next trial data file
   CButton           m_btnRecordData;        // check box; if checked, trial data is recorded & saved to file
   CButton           m_btnRecordSpks;        // check box; if checked, spike waveform data is recorded & saved
   CNumEdit          m_edNTrials;            // r-o edit ctrl displays #trials presented (since last reset)
   CNumEdit          m_edNAttempts;          // r-o edit ctrl displays #trials attempted (NOT ignored)
   CNumEdit          m_edNSuccesses;         // r-o edit ctrl displays #trials completed
   CNumEdit          m_edNBlocks;            // r-o edit ctrl displays #trial blocks presented
   CNumEdit          m_edDelay;              // numeric edit control for setting the inter-trial delay (in ms)
   CNumEdit          m_edIgnore;             // numedit for setting the ignore threshold time (in ms)

   CNumEdit          m_edAutoStopCnt;        // numedit that specifies stop count N for auto-stop feature
   CComboBox         m_cbAutoStopMode;       // combo box selects the auto-stop mode


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialProtoDlg(const CCxTrialProtoDlg& src);        // no copy constructor or assignment operator defined
   CCxTrialProtoDlg& operator=( const CCxTrialProtoDlg& src );

public:
   CCxTrialProtoDlg() : CCxControlPanelDlg( IDD ) {}
   ~CCxTrialProtoDlg() {}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnDestroy();                             // save MRU data dir to registry prior to destruction
   afx_msg void OnPreBrowse(NMHDR *pNMH, LRESULT *pRes); // to prevent browsing when a trial seq is running
   afx_msg void OnChange(UINT id);                       // response to changes in selected dialog widgets
   DECLARE_MESSAGE_MAP()

private:
   // given current state of controls in Protocol dialog panel, can user start trial sequencing?
   BOOL CanStart();

//=====================================================================================================================
// ATTRIBUTES -- the current values of control parameters represented in the dialog
//=====================================================================================================================
public:
   WORD GetTrialSet() { return( m_cbTrialSet.GetObjKey() ); }
   WORD GetCurrentTrial() { return( m_cbCurrTrial.GetObjKey() ); }
   int GetSubsetSeqMode() { return( m_cbSubsetSeq.GetCurSel() ); }
   int GetTrialSeqMode() { return(m_cbTrialSeq.GetCurSel()); }   
   BOOL IsSaveData() { return( BOOL(m_btnRecordData.GetCheck() != 0) ); }
   BOOL IsSaveSpikes() { return( BOOL(m_btnRecordSpks.GetCheck() != 0) && IsSaveData() ); }
   VOID GetNextDataFile( CString& strPath ) { strPath = m_fecDataPath.GetCurrentPath(); }
   int GetNumTrials() { return( m_edNTrials.AsInteger() ); }
   int GetAttempts() { return( m_edNAttempts.AsInteger() ); }
   int GetSuccesses() { return( m_edNSuccesses.AsInteger() ); }
   int GetBlocks() { return( m_edNBlocks.AsInteger() ); }
   int GetInterTrialDelay() { OnChange(IDC_TRIAL_DELAY); return( m_edDelay.AsInteger() ); }
   int GetIgnoreTime() { OnChange(IDC_TRIAL_IGT); return( m_edIgnore.AsInteger() ); }
   int GetAutoStopMode() { return(m_cbAutoStopMode.GetCurSel()); }
   int GetAutoStopCount() { OnChange(IDC_TRIAL_STOP_COUNT); return( m_edAutoStopCnt.AsInteger() ); }

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // call this to refresh appearance of dialog
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change

                                                         // for changing selected controls while trial sequencer runs:
   VOID IncrementNextDataFile( CString& strPath )        //    increment numeric ext of next trial data file
   {
      strPath = m_fecDataPath.IncrementFileExt();
   }
   BOOL SetCurrentTrial( const WORD wKey );              //    change the current trial combo box selection
   VOID IncrementNumTrials();                            //    increment #trials counter
   VOID IncrementAttempts();                             //    increment #attempted trials counter
   VOID IncrementSuccesses();                            //    increment #completed trials counter
   VOID IncrementBlocks();                               //    increment #trial blocks counter
};





//=====================================================================================================================
// Declaration of class CCxTrialParmsDlg  -- the "Other Params" dialog for TrialMode
//=====================================================================================================================
//
class CCxTrialParmsDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxTrialParmsDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TRIALCP2;      // dialog template resource ID for this dialog

public:
   static const int     MIN_STAIRREVS;       // min/max/default vals for selected control parameters on this dialog
   static const int     MAX_STAIRREVS;
   static const int     DEF_STAIRREVS;
   static const int     MIN_STAIRINAROW;
   static const int     MAX_STAIRINAROW;
   static const int     DEF_STAIRINAROW;
   static const int     MIN_STAIRIRREL;
   static const int     MAX_STAIRIRREL;
   static const int     DEF_STAIRIRREL;
   static const double  MIN_STAIRSTREN;
   static const double  MAX_STAIRSTREN;
   static const double  DEF_STAIRSTREN;
   static const double  MIN_TGTSCALE;
   static const double  MAX_TGTSCALE;
   static const double  DEF_TGTSCALE;
   static const double  DEF_TGTROTATE;
   static const double  MIN_STARTPOS;
   static const double  MAX_STARTPOS;
   static const double  DEF_STARTPOS;


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxObjCombo       m_cbChanCfg;            // combo box selects CNTRLX chan cfg "global override" for TrialMode

   CNumEdit          m_edStren;              // numeric edits:  staircase seq starting strength,
   CNumEdit          m_edIrrel;              //    staircase seq % irrelevant trials,
   CNumEdit          m_edPosScale;           //    target pos scale factor (unitless),
   CNumEdit          m_edPosRot;             //    target pos rotation (degrees),
   CNumEdit          m_edVelScale;           //    target vel scale factor (unitless),
   CNumEdit          m_edVelRot;             //    target vel rotation (degrees),
   CNumEdit          m_edStartH;             //    H cmpt of starting pos for all participating tgts (deg)
   CNumEdit          m_edStartV;             //    V cmpt of starting pos for all participating tgts (deg)
   CNumEdit          m_edVStabWin;           //    len of sliding window avg of eye pos to smooth VStab effects (ms)

   CSpinButtonCtrl   m_spinReversals;        // spin buttons:  stop staircase seq after N reversals,
   CSpinButtonCtrl   m_spinWrongUp;          //    incr stair strength after this # of incorrect responses in a row,
   CSpinButtonCtrl   m_spinRightDn;          //    decr stair strength after this # of correct responses in a row

   CButton           m_btnChanEna;           // check boxes, PBs:  channel config override enable
   CButton           m_btnReset;             //    reset parameters to default values


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialParmsDlg(const CCxTrialParmsDlg& src);        // no copy constructor or assignment operator defined
   CCxTrialParmsDlg& operator=( const CCxTrialParmsDlg& src );

public:
   CCxTrialParmsDlg() : CCxControlPanelDlg( IDD ) {}
   ~CCxTrialParmsDlg() {}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnEditKillFocus( UINT id );              // for range-checking input to edit controls in dialog
   afx_msg void OnReset();                               // reset all control parameters on dlg to start-up values

   DECLARE_MESSAGE_MAP()


//=====================================================================================================================
// ATTRIBUTES -- the current values of control parameters represented in the dialog
//=====================================================================================================================
public:
   double GetStairStartStrength()   { return( m_edStren.AsDouble() ); }
   int GetStairPctIrrelevant()      { return( m_edIrrel.AsInteger() ); }
   int GetStairNumWrongUp()         { return( m_spinWrongUp.GetPos() ); }
   int GetStairNumRightDn()         { return( m_spinRightDn.GetPos() ); }
   int GetStairNumReversals()       { return( m_spinReversals.GetPos() ); }
   double GetStartingTgtPosH()      { return( m_edStartH.AsDouble() ); }
   double GetStartingTgtPosV()      { return( m_edStartV.AsDouble() ); }
   double GetTgtPosScale()          { return( m_edPosScale.AsDouble() ); }
   double GetTgtPosRotation()       { return( m_edPosRot.AsDouble() ); }
   double GetTgtVelScale()          { return( m_edVelScale.AsDouble() ); }
   double GetTgtVelRotation()       { return( m_edVelRot.AsDouble() ); }
   BOOL IsChanCfgOverrideEnabled()  { return( BOOL(m_btnChanEna.GetCheck() != 0) ); }
   WORD GetChanCfgOverride()        { return( m_cbChanCfg.GetObjKey() ); }
   int GetVStabSlidingWindowLen()   { return( m_edVStabWin.AsInteger() ); }

   VOID GetTrialChainLengths(CString& s)
   {
      GetDlgItem(IDC_TRIAL_CHAINLEN)->GetWindowTextA(s);
   }

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // to refresh dlg appearance -- updates enable state of ctrls
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   BOOL LimitRotationAngle( double& dAngle );            // map any rotation angle in deg to the unit circle [0..360)
   BOOL RangeLimit( int& i, int iMin, int iMax )         // limit integer value to a specified range; return TRUE iff
   {                                                     // value did not have to be adjusted to fit range
      if( i < iMin ) { i = iMin; return( FALSE ); }
      else if( i > iMax ) { i = iMax; return( FALSE ); }
      else return( TRUE );
   }
   BOOL RangeLimit( double& d,                           // analogously for floating-point value
                    double dMin, double dMax )
   {
      if( d < dMin ) { d = dMin; return( FALSE ); }
      else if( d > dMax ) { d = dMax; return( FALSE ); }
      else return( TRUE );
   }

   // retreive ptr to application settings, in which VStab window length is persisted. Other parameters on the dialog
   // are NOT persisted in application settings.
   CCxSettings* GetSettings()
   {
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      return((pDoc != NULL) ? pDoc->GetSettings() : NULL);
   }
};




//=====================================================================================================================
// Declaration of class CCxTrialStatsDlg -- the "Statistics" dialog for TrialMode
//=====================================================================================================================
//
class CCxTrialStatsDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxTrialStatsDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TRIALCP3;            // dialog template resource ID for this dialog
   static const int MINCOLW = 40;                  // minimum column width in trial stats grid
   static const int MINNAMECOLW = 100;             // min width for column containing a trial name

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CLiteGrid   m_grid;                             // spreadsheet-like control displays the trial statistics
   int m_gridWidth;                                // width of stats grid, not including border and vertical scrollbar
   CStatic     m_setLabel;                         // static label displays name of current trial set
   const CCxTrialSequencer* m_pSeq;                // reference to trial sequencer -- for retrieving trial statistics

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialStatsDlg( const CCxTrialStatsDlg& src );      // no copy constructor or assignment operator defined
   CCxTrialStatsDlg& operator=( const CCxTrialStatsDlg& src );

public:
   CCxTrialStatsDlg();
   ~CCxTrialStatsDlg();


//=====================================================================================================================
// MESSAGE MAP HANDLERS  -- NONE
//=====================================================================================================================
protected:

   DECLARE_MESSAGE_MAP()


//=====================================================================================================================
// ATTRIBUTES -- NONE
//=====================================================================================================================

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   VOID SetSequencer(const CCxTrialSequencer& seq) 
   {
      m_pSeq = &seq;
   }

   BOOL OnInitDialog();                                  // prepare dialog for display
   virtual VOID Refresh();                               // to refresh dlg appearance -- updates enable state of ctrls
   virtual VOID OnUpdate( CCxViewHint* pHint );          // refresh appearance IAW specified CNTRLX doc/view change

   VOID Initialize(WORD wKeySet);                        // reset statistics and init table w/trials in specified set
   VOID UpdateStats(WORD wTrial);                        // update statistics for specified trial


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
};





//=====================================================================================================================
// Declaration of class CCxTrialMode -- the mode controller for TrialMode
//=====================================================================================================================
//
class CCxTrialMode : public CCxModeControl
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const DWORD F_RUNNING;                // trial mode state flags
   static const DWORD F_STOPPING;
   static const DWORD F_WAITING;
   static const DWORD F_PAUSING;
   static const DWORD F_PAUSED;
   static const DWORD F_RUNNINGMASK;
   static const DWORD F_RECDATA;


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxTrialProtoDlg*    m_pProtoDlg;            // the "Protocol" dlg page
   CCxTrialParmsDlg*    m_pParmsDlg;            // the "Other Params" dlg page
   CCxTrialStatsDlg*    m_pStatsDlg;            // the "Statistics" dlg page
   CCxRPDistroDlg*      m_pRPDistroDlg;         // the "R/P Distro" dlg page
   CCxFixRewDlg*        m_pFixRewDlg;           // the "Fix/Reward" dlg page
   CCxVideoDspDlg*      m_pVideoDspDlg;         // the "RMVideo Display" dlg page
   CCxEyelinkDlg*       m_pEyelinkDlg;          // the "Eyelink" dlg page

   DWORD                m_dwState;              // trial mode state flags
   CString              m_strTrialPath;         // path to which current trial's data will be saved
   CString              m_strShadowPath;        // shadow file for current trial data (necessary b/c CXDRIVER cannot
                                                // write directly to a remote drive); empty if shadowing not needed

   CCxTrialSequencer    m_seq;                  // handles trial sequencing, packaging trial & targets for CXDRIVER
   CElapsedTime         m_waitTime;             // to introduce additional delay between trials


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialMode( const CCxTrialMode& src );              // no copy constructor or assignment operator defined
   CCxTrialMode& operator=( const CCxTrialMode& src );

public:
   CCxTrialMode( CCxControlPanel* pPanel );
   ~CCxTrialMode() {}                                    // (created dlgs destroyed by mode control panel container)


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   BOOL IsSeqRunning() const { return( BOOL(0 != (m_dwState & F_RUNNING)) ); }
   BOOL IsSeqStopping() const
   {
      DWORD dwStopping = F_RUNNING|F_STOPPING;
      return( BOOL( dwStopping == (m_dwState & dwStopping) ) );
   }
   BOOL IsSeqPausing() const
   {
      DWORD dwPausing = F_RUNNING|F_PAUSING;
      return( BOOL( dwPausing == (m_dwState & dwPausing) ) );
   }
   BOOL IsSeqPaused() const
   {
      DWORD dwPaused = F_RUNNING|F_PAUSED;
      return( BOOL( dwPaused == (m_dwState & dwPaused) ) );
   }
   BOOL IsSeqOffOrPaused() const
   {
      return( IsSeqPaused() || !IsSeqRunning() );
   }

private:
   BOOL IsSeqWaiting() const
   {
      DWORD dwWaiting = F_RUNNING|F_WAITING;
      return( BOOL( dwWaiting == (m_dwState & dwWaiting) ) );
   }


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
      return( IsSeqOffOrPaused() );
   }
   virtual BOOL CanUpdateFixRewSettings()                // TRUE when update of fixation/reward settings is permissible
   {
      return( IsSeqOffOrPaused() );
   }
   virtual LPCTSTR GetModeTitle()                        // string constant describing this op mode
   {
      return( _T("Trial Mode") );
   }

   VOID TrialSetChanged()                                // so CCxRPDistroDlg can load R/P Distro trials in that set!
   {
      WORD wSet = m_pProtoDlg->GetTrialSet();
      m_pRPDistroDlg->OnTrialSetChanged(wSet);
   }

   VOID Go();                                            // start sequencing trials based on current protocol, params
   VOID Halt();                                          // stop sequencing once current trial finishes
   VOID Pause();                                         // pause an ongoing trial sequence once current trial finishes
   VOID Resume();                                        // resume a paused trial sequence
   VOID Abort();                                         // abort sequencing immediately (current trial discarded)

private:
   VOID UpdateRPDistroTrial(WORD wKey, DWORD dwRes);     // update GUI after a "R/P Distro" trial completes

};


#endif // !defined(CXTRIALMODE_H__)

