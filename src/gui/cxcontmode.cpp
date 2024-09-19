//=====================================================================================================================
//
// cxcontmode.cpp :  Declaration of the ContMode controller CCxContMode and ContMode-specific control panel dialogs,
//                   CCxContProtoDlg and CCxContFixTgtsDlg.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Each "operational mode" of CNTRLX has a "mode controller" which controls the runtime behavior of CNTRLX and CXDRIVER
// in that mode.  CCxContMode is the mode controller for CNTRLX's "Continuous Mode", during which (among various other
// actions) a set of defined "stimulus runs" may be presented to the subject in a manner proscribed by the operator via
// interactions with various dialogs housed in CNTRLX's master mode control panel, CCxControlPanel.  In this file we
// define CCxContMode as well as two ContMode-specific mode contol dialogs, CCxContProtoDlg and CCxContFixTgtsDlg.
//
// ==> The CNTRLX "Mode Control" Framework.
// CNTRLX's master mode control panel CCxControlPanel is implemented as a dockable dialog bar containing one or more
// tabbed dialogs.  All dialogs that affect runtime state in any CNTRLX operational mode are installed in this
// container, although only a subset of them will be accessible in any given mode.  In addition to its role as a
// dialog container, CCxControlPanel constructs a "mode controller" object for each op mode, and it handles mode
// switches by invoking appropriate methods on the relevant mode controllers.  Each mode controller, interacting with
// the operator via some subset of the mode control panel dialogs, encapsulates the runtime behavior of CNTRLX and
// CXDRIVER in a particular operational mode.  To communicate with CXDRIVER, it must invoke methods on the CNTRLX
// runtime interface, CCxRuntime.  By design, the mode controller should insulate the mode control dialogs from
// CCxRuntime.  In other words, it provides all the methods needed by the dialogs to realize the desired functionality
// of the operational mode that controller and the associated dialogs represent.  Multiple dialogs allow us to break up
// that functionality into logically grouped, more manageable chunks.
//
// We define two ABSTRACT classes that serve as the foundations for this "mode control" framework.  CCxModeControl is
// the base class for all CNTRLX mode controller objects, while CCxControlPanelDlg is the base class for any dialog
// that is installed in CCxControlPanel.  CCxModeControl handles tasks that are common to all mode controllers and
// defines a set of abstract methods that any realizable mode controller must implement; CCxControlPanelDlg does the
// same for mode control dialog objects.
//
// See the implementation files for CCxControlPanel, CCxControlPanelDlg, and CCxModeControl for more details.
//
// ==> CCxContMode.
// CCxContMode is the mode controller for ContMode operations.  It installs the ContMode-specific dialogs in the master
// mode control panel during GUI creation at application startup (see InitDlgs()), hides all ContMode control dialogs
// upon exiting ContMode (see Exit()), and reveals these same dialogs upon entering ContMode (see Enter()).  The
// dialogs accessible in the mode control panel during ContMode (see below) manage various GUI widgets/controls, and
// these dialogs call CCxContMode methods to carry out various operations, including all interactions with CXDRIVER via
// the CNTRLX runtime interface, CCxRuntime.
//
//
// ==> CCxContProtoDlg, the "Protocol" dialog.
// This dialog page houses the controls which manage the operational state of CNTRLX in ContMode:
//
//    IDC_CONT_GO [pushb]: Starts & stops a stimulus run.  PB label reads "START" while idled, "STOP" while a run is
//          executing, and "!!WAIT!!" after the user has pressed the PB to "soft-stop" the stimulus, waiting for the
//          current duty cycle to complete.  While waiting, the PB is disabled.  See CCxContMode::Go(), Halt().
//    IDC_CONT_ABORT [pushb]: Abort current run immediately.  In any non-MANUAL run mode, data rerecording is aborted
//          and the data file discarded.  Enabled when a stimulus run is in progress.  See CCxContMode::Abort().
//    IDC_CONT_RESTART [pushb]: A short-cut for performing an abort of the current run, then restarting it.  Enabled
//          when a stimulus run is in progress.
//    IDC_CONT_FIXATE [pushb]: Toggles fixation checking ON/off.  This control is enabled at all times.
//    IDC_CONT_SET [custom combo]: Selects the "active" stimulus run set -- a group of stimulus runs forming a coherent
//          experimental protocol.  Combo box contains the names of all run sets currently defined in the open CCxDoc.
//          Enabled when both stimulus run and data recording are off.  Implemented by CCxObjCombo, which see.
//    IDC_CONT_CURR [custom combo]: Selects stimulus run to be executed next.  Lists all stimulus runs in the currently
//          selected run set.  Enabled when both stimulus run & data recording are off.  Implemented by CCxObjCombo.
//
//             [DEVNOTE: For now, there is no equivalent of TrialMode's "trial sequencer" in ContMode.  The user must
//             select a stimulus run to present; however, we may later introduce a "run sequencer" that allows the user
//             to automatically sequence the runs in the selected run set in some fashion.]
//
//    IDC_CONT_EXECMODE [std combo]: Selects the stimulus run mode:  "Manual", "Auto Record", or "Single-run Repeat".
//          Enabled when both stimulus run & data recording are off.
//    IDC_CONT_CHANCFG [custom combo]: Selects the data channel configuration to use when recording and displaying data
//          traces during continuous mode.  Enabled when no stim running & recording OFF.  Implemented by CCxObjCombo.
//    IDC_CONT_REC [pushb]: Toggles data recording ON/off.  This control is enabled at all times in "Manual" mode, but
//          it is hidden in the other modes, in which the system automatically turns recording on & off.  Recording
//          cannot be initiated if the pathname in the accompanying edit ctrl (IDC_CONT_DATAPATH) is invalid.
//    IDC_CONT_DATAPATH [custom edit]: This read-only custom edit ctrl displays the full pathname where the next data
//          file will be stored.  It includes a "browse" button which invokes a dialog that lets the user choose a
//          different path.  A standard edit control is subclassed to CCxFileEdit to get the browsing functionality and
//          to enforce CNTRLX-specific constraints on the form of CNTRLX data file names.  See CCxFileEdit for details.
//          "Browse" button is disabled while recording data.  To increment the extension on the data filename shown in
//          the datapath edit ctrl), CCxContMode invokes the method CCxContProtoDlg::IncrementNextDataFile().
//    IDC_CONT_RECSPKS [chkbox]: If checked, a high-res spike trace is simultaneously recorded and saved to the data
//          file.  This check box is disabled while recording data.
//
// Note that IDC_CONT_SET, _CURR, _EXECMODE, _CHANCFG should represent a contiguous set of integers so we can use the
// ON_CONTROL_RANGE message macro.
//
// ==> CCxContFixTgtsDlg, the "Active Targets" dialog.
// CCxContFixTgtsDlg (laid out on dialog resource template IDD_CONTCP2) manages the "active target list" in continuous
// mode.  "Active" targets are used often during routine calibration of the circuitry that monitors the subject's eye
// position, and they may have other applications as well.  We use CLiteGrid, a derivative of the MFC grid control (see
// CREDITS) to display and manipulate the target list (IDC_CONT_TGTS).  CLiteGrid provides various inplace editor tools
// for changing an individual cell in the grid IAW the type of data displayed in that cell.  Columns in the grid show
// each active target's identity, usage ("None", "Fix1", "Fix2", "Both", or "Track"), ON/off state, horiz and vertical 
// position, and the radial speed and direction of pattern motion (for patterned targets).  Here is a summary of the 
// operations that the user can perform on the active target list:
//
//    1) Click the "Add" PB (IDC_CONT_TGT_ADD) to append a target (up to MAX_ACTIVETGTS) to the list.
//    2) Click the "Delete" PB (IDC_CONT_TGT_DEL) to remove a target from the active list.  This PB is enabled only
// when a "target name" cell (first column) has the focus within the grid control.
//    3) Click the "Remove All" PB (IDC_CONT_TGT_CLEAR) to clear the active target list.
//    4) Double-click on any cell in the active target list to edit that cell's contents with an appropriate inplace
// editor.
//    5) As a short-cut to inplace editing, the user can "mouse-click" on most cells in the grid to change the value of
// the corresponding parameter.  Clicking on a multiple-choice parameter (ON/off state, usage designation) will change
// the parameter to the next or previous legal choice, with "wrap-around".  A right-click on a target's horiz or vert
// pos will increment (or decrement, if SHIFT key is down) the value by CCxContFixTgtsDlg::INC_ACVTGTPOS. Similar 
// incr/decr actions apply to the target's pattern speed and direction.
//
// While the CLiteGrid control handles the display and inplace editing of grid cells, it relies on four different
// callback functions to provide cell display and editing information:  CCxContFixTgtsDlg::GridDispCB() provides cell
// display info, CCxContFixTgtsDlg::GridEditCB() provides cell edit info when an inplace operation is about to start,
// and CCxContFixTgtsDlg::GridEndEditCB() updates the active target list IAW the results of the inplace operation.  In
// addition, CCxDoc::TreeInfoCB() serves as the tree info callback -- allowing CLiteGrid's inplace tree control to
// traverse the current CNTRLX object tree (for target selection!).  All of these callbacks are installed when the
// dialog is set up in OnInitDialog().
//
// DESIGN NOTES:  1) CCxContFixTgtsDlg maintains current state of the active target list.  However, to communicate
// parameter changes to CXDRIVER, CCxContFixTgtsDlg must invoke methods on CCxContMode.  The design approach here is
// to keep parameter storage within the object responsible for displaying and manipulating the parameter's value, while
// the responsibility for communicating that information to CXDRIVER remains with the mode controller (which ultimately
// relies on CCxRuntime).  2) One cannot change the composition of the active tgt list when the system is "active" (ie,
// fixation or recording is ON, or a stimulus run is in progress).  CCxContFixTgtsDlg and CCxContMode are designed to
// enforce this rule.  3) Changes in an active target's on/off state, position, etcetera are only forwarded to CXDRIVER 
// when the system IS "active".
//
// ==> CCxFixRewDlg, the "Fix/Reward" dialog.
// This dialog page provides a window into the CNTRLX document's fixation and reward settings, a subset of the
// application level settings encapsulated by the CCxSettings object.  Included among these settings are the fixation
// duration and horizontal & vertical fixation accuracies applicable to ContMode.  Note, however, that the CCxFixRewDlg
// page is designed for use in any CNTRLX op mode, not just ContMode.  For details, see the files CXFIXREWDLG.*.
// CCxContMode will install this dialog in the mode control panel ONLY if it is not already there (see InitDlgs()).
//
// ==> CCxVideoDspDlg, the "Video Display" dialog.
// This dialog page is a window into the XY and FB video display parameters that are a subset of CNTRLX's application
// settings, also encapsulated by the CCxSettings object within the currently open CNTRLX doc.  Like CCxFixRewDlgs, the
// CCxVideoDspDlg page may be used in more than one CNTRLX op mode.  For details, see files CXVIDEODSPDLG.*.  Again,
// CCxContMode will install this dialog in the mode control panel ONLY if it is not already there.
//
// ==> CCxEyelinkDlg, the "Eyelink" dialog.
// Contains controls for connecting/disconnecting from the Eyelink 1000+ eye tracker, adjusting calibration parameters.
//
//
// CREDITS:
// (1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21.
//
//
// REVISION HISTORY:
// 07jun2002-- Began development, using CCxTrialPanel and its dialogs as a template....
// 31jul2002-- Completed initial code development -- lots of debugging and testing to do.
// 08oct2002-- CFileEditCtrl in CCxRunProtoDlg replaced by more self-contained CCxFileEdit.
// 17oct2002-- Modified to integrate the new CCxFixRewDlg page.  Like CCxVideoDspDlg, this page reflects application-
//             level settings that can be modified in any operational mode.  The fixation requirements that were part
//             of CCxRunFixTgtsDlg were incorporated into CCxFixRewDlg.
// 26nov2002-- Modified CCxRunFixTgtsDlg to use CGridCtrl derivative class CLiteGrid.
// 24jan2003-- Minor mods to CCxRunFixTgtsDlg so that, whenever the currently selected stimulus run is changed, its
//             defn is loaded onto the appropriate form.  Methods SetCurrentRun(), OnComboChange() modified; OnUpdate()
//             also modified so that it does not respond to updates initiated by the dialog itself!
// 04apr2003-- MAJOR redesign of the CNTRLX "mode control" framework.  There is now only a single mode control panel,
//             CCxControlPanel.  CCxContRunPanel is replaced by the ContMode controller object CCxContMode, which is
//             derived from the abstract base class CCxModeControl.  Mode control dialogs are still derived from the
//             abstract class CCxControlPanelDlg, but they interact with the "current" mode controller object rather
//             than a derivative of CCxControlPanel.  See also CCxControlPanel, CCxControlPanelDlg, and CCxModeControl.
//          -- Also renamed dlgs:  CCxRunProtoDlg --> CCxContProtoDlg; CCxRunFixTgtsDlgs --> CCxContFixTgtsDlg.
// 22sep2004-- Cont-mode data directory (appearing in file edit ctrl w/in CCxContProtoDlg) is set IAW a registry
//             setting at startup, then saved in the registry before GUI is destroyed.
// 03may2005-- Operational state is now maintained by CXDRIVER and read by Maestro via CCxRuntime::GetProtocolStatus().
//             Hopefully, this will address recent problems in which Maestro seemed to get out of synch with what was
//             happening on the CXDRIVER side...
// 15jun2005-- Whenever data file is on a mapped network drive, CCxContMode instructs CXDRIVER to write the data to a
//             "shadow file" on the local disk.  When recording is done, the shadow file is moved to the remote drive.
//             This change was required because RTX no longer supports file I/O to a remote drive as of version 5.1.1.
//             See CCntrlxApp::Get/MoveShadowFile().
//          -- Mods to fix possible synchronization bug with CXDRIVER re: operational state.
//          -- Mods to CCxContFixTgtsDlg and CCxContMode to introduce a new usage for an active target: tracking the
//             pos of the cursor in the eye-target position plot.  When CXDRIVER is active in Cont mode and an active
//             target is designated as the "Track" target, the target will roughly track the position of the mouse
//             cursor whenever it is inside the client area of the position plot panel.
// 08dec2006-- Adding support for specifying the speed and direction of motion for the pattern associated with an 
//             active target (if applicable). Now users will be able to display an active target with a moving pattern; 
//             accuracy of motion is not guaranteed as in Trial mode. Its main purpose is for probing a unit's response 
//             approximately using the "Track" target.
// 11dec2006-- Added HandleTrackingTargetPatternUpdate(), which supports keyboard shortcuts for incr/decrementing the 
//             pattern speed or direction of the "Track" target in the active list, if one is designated.
// 31aug2015-- Added Eyelink dialog page, CCxEyelinkDlg.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxdoc.h"                           // CCxDoc -- CNTRLX document class
#include "cxfixrewdlg.h"                     // CCxFixRewDlg -- the "Fix/Reward" dialog page
#include "cxvideodspdlg.h"                   // CCxVideoDspDlg -- the "Video Display" dialog page
#include "cxeyelinkdlg.h"                    // CCxEyelinkDlg -- the "Eyelink" dialog page
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the CNTRLX master mode control panel
#include "cxruntime.h"                       // CCxRuntime -- the CNTRLX/CXDRIVER runtime interface

#include "cxcontmode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxContProtoDlg
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxContProtoDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxContProtoDlg, CCxControlPanelDlg )
   ON_WM_DESTROY()
   ON_NOTIFY( FEC_NM_PREBROWSE, IDC_CONT_DATAPATH, OnPreBrowse )
   ON_CONTROL_RANGE( CBN_CLOSEUP, IDC_CONT_SET, IDC_CONT_CHANCFG, OnComboChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_CONT_GO, IDC_CONT_FIXATE, OnBtnClicked )
END_MESSAGE_MAP()


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnDestroy (CWnd override) =======================================================================================
//
//    ON_WM_DESTROY handler.
//
//    Prior to destroying the dialog, we store the Continuous-mode data directory (from the file edit control) in the
//    current user's registry profile.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxContProtoDlg::OnDestroy()
{
   CString strDir;
   m_fecDataPath.GetCurrentDirectory( strDir );
   ((CCntrlxApp*)AfxGetApp())->SetMRUContDataDirectory( strDir );
   CCxControlPanelDlg::OnDestroy();
}


//=== OnPreBrowse =====================================================================================================
//
//    ON_NOTIFY handler for the custom edit control (CCxFileEdit) that displays/selects the file system path for
//    storing the next continuous run data file (IDC_CONT_DATAPATH).
//
//    OnPreBrowse() [FEC_NM_PREBROWSE notification code] is called just after the user clicks on the button that
//    invokes the browsing dialog.  This gives us a chance to prevent browsing entirely, and to further tailor the
//    appearance of the dialog, if desired.
//
//    Here we prevent browsing whenever we're executing a stimulus run or recording data.
//
//    ARGS:       pNMH  -- [in] ptr to CFileEditCtrl's FEC_NOTIFY struct, cast as a generic NMHDR*.
//                pRes  -- [out] return value.  for FEC_NM_PREBROWSE, set nonzero value to prevent browsing.
//
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
void CCxContProtoDlg::OnPreBrowse( NMHDR *pNMH, LRESULT *pRes )
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );  // get ptr to ContMode controller
   *pRes = (LRESULT) (pContMode->IsRecording() || pContMode->IsStimRunning());
}


//=== OnComboChange ===================================================================================================
//
//    Respond to the CBN_CLOSEUP notification from various combo box controls on this dialog.  The following control
//    IDs must constitute a contiguous range of integers:
//
//       IDC_CONT_SET      ==> Selects the current run set.
//       IDC_CONT_CURR     ==> Selects a run from the current run set.
//       IDC_CONT_EXECMODE ==> Selects the stimulus run mode.
//       IDC_CONT_CHANCFG  ==> Selects a channel cfg object for use throughout ContMode operation.
//
//    All of these controls MUST be disabled while a stimulus is running or while data recording is ON.
//
//    ARGS:       id -- [in] control ID.
//
//    RETURNS:    NONE.
//
void CCxContProtoDlg::OnComboChange( UINT id )
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );

   ASSERT( !(pContMode->IsStimRunning() ||                                 // we should never get here when a stim is
             pContMode->IsRecording()) );                                  // running or data recording is ON.

   WORD wKey;
   switch( id )
   {
      case IDC_CONT_SET:                     // reinit contents of "current run" CB whenever run set is changed; if no
         wKey = m_cbRunSet.GetObjKey();      // run set is selected, we can't run!
         if( m_cbCurrRun.GetParentKey() != wKey )
            m_cbCurrRun.InitContents( wKey, TRUE );
         Refresh();
         break;
      case IDC_CONT_CURR:                    // when user changes the "current run", we send a display hint so that
         wKey = m_cbCurrRun.GetObjKey();     // run's defn is displayed for user on the relevant form
         if( wKey != CX_NULLOBJ_KEY )
         {
            CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();
            CCxViewHint vuHint( CXVH_DSPOBJ, CX_CONTRUN, wKey );
            SendUpdate( &vuHint, FALSE );
         }
         break;
      case IDC_CONT_EXECMODE:                // changing the run mode can affect appearance of ctrl on this dlg
         Refresh();
         break;
      case IDC_CONT_CHANCFG:                 // reinit data trace facility IAW any change in channel cfg
         pContMode->ChangeTraces();
         break;
   }
}


//=== OnBtnClicked ====================================================================================================
//
//    Respond to the BN_CLICKED notification from various pushbutton controls on this dialog.  Control ids IDC_CONT_GO
//    to IDC_CONT_FIXATE must constitute a contiguous range of integers.
//
//       IDC_CONT_GO       ==> [PB] Start or "soft-stop" a stimulus run
//       IDC_CONT_RESTART  ==> [PB] Abort and restart the stimulus run in progress.
//       IDC_CONT_ABORT    ==> [PB] Abort the stimulus run in progress.
//       IDC_CONT_RECORD   ==> [PB] Start/stop data recording while in "Manual" seq/exec mode
//       IDC_CONT_FIXATE   ==> [PB] Enable/disable fixation checking in ContMode.
//
//    ARGS:       id -- [in] control ID.
//
//    RETURNS:    NONE.
//
void CCxContProtoDlg::OnBtnClicked( UINT id )
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );
   CString contDir;

   switch( id )
   {
      case IDC_CONT_GO :                                          // start/stop a stimulus run
         if( !pContMode->IsStimRunning() )
            pContMode->Go();
         else if( !pContMode->IsStimStopping() )
            pContMode->Halt();
         break;
      case IDC_CONT_RESTART :                                     // abort stimulus run immediately and restart it
         if( pContMode->IsStimRunning() )
            pContMode->Restart();
         break;
      case IDC_CONT_ABORT :                                       // abort stimulus run immediately
         if( pContMode->IsStimRunning() )
            pContMode->Abort();
         break;
      case IDC_CONT_REC :                                         // toggle data recording ON/off
         pContMode->ToggleRecord();
         break;
      case IDC_CONT_FIXATE :                                      // toggle subject fixation ON/off
         pContMode->ToggleFixate();
         m_fecDataPath.GetCurrentDirectory( contDir );
         ((CCntrlxApp*)AfxGetApp())->SetMRUContDataDirectory( contDir );
         break;
      default :
         TRACE0( "CCxRunProtoDlg: Unrecog ID in OnBtnClicked()\n" );
         break;
   }
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitDialog [base override] ====================================================================================
//
//    Prepare the dialog for display.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CString ops can throw CMemoryException.
//
BOOL CCxContProtoDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                               // let base class do its thing...

   m_cbRunSet.SubclassDlgItem( IDC_CONT_SET, (CWnd*) this );         // current run set selection (CNTRLX obj combo)
   m_cbCurrRun.SubclassDlgItem( IDC_CONT_CURR, (CWnd*) this );       // current run selection (CNTRLX obj combo)
   m_cbChanCfg.SubclassDlgItem( IDC_CONT_CHANCFG, (CWnd*) this );    // cont-mode channel cfg (CNTRLX obj combo)

   m_cbRunMode.SubclassDlgItem( IDC_CONT_EXECMODE, (CWnd*) this );   // stimulus run execution mode (std combo)
   m_cbRunMode.ModifyStyle( CBS_SORT, 0 );                           // load cb w/avail exec modes, unsorted
   m_cbRunMode.ResetContent();
   for( int i = 0; i < CCxContMode::NUMRUNMODES; i++ )
      m_cbRunMode.AddString( CCxContMode::strModes[i] );
   m_cbRunMode.SetCurSel( 0 );                                       // init to first selection in combo box

   m_fecDataPath.SubclassDlgItem( IDC_CONT_DATAPATH, (CWnd*)this );  // cont run data file path
   m_fecDataPath.SetFlags( 0 );                                      // to correctly pos browse btn in subclassed ctrl
   m_fecDataPath.InitializePath(
         ((CCntrlxApp*)AfxGetApp())->GetMRUContDataDirectory(),
         _T("run") );

   m_btnRecordSpks.SubclassDlgItem(IDC_CONT_RECSPKS, (CWnd*) this); // check box: record spike waveform on/OFF
   m_btnRecordSpks.SetCheck( 0 );

   return( TRUE );                                                   // set input focus to 1st ctrl in tab order
}


//=== Refresh [CCxControlPanelDlg override] ===========================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    All we do here is to update the enable state of most controls and the labels of selected controls IAW the current
//    ContMode operational state.
//
//    IDC_CONT_GO: PB label reads "START" while idled, "STOP" while a run is executing, and "!!WAIT!!" after user has
//       pressed PB to "soft-stop" the stimulus, waiting for current duty cycle to complete.  Disabled while waiting.
//    IDC_CONT_ABORT, _RESTART: Enabled when a stimulus run is in progress.
//    IDC_CONT_FIXATE: Enabled at all times.  PB label reads "Fixation ON(OFF)" when fixation is turned on(off).
//    IDC_CONT_SET, _CURR, _EXECMODE, _CHANCFG: Enabled only when both stimulus run & data recording are OFF.
//    IDC_CONT_REC: Enabled at all times in "Manual" mode, and PB label reads "Record ON(OFF)" when data recording is
//       turned on(off).  In all other modes, the PB is hidden to emphasize the fact that the system controls data
//       recording in these modes.
//    IDC_CONT_DATAPATH, _RECSPKS: Enabled only when recording is OFF.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CString ops may throw CMemoryException.
//
VOID CCxContProtoDlg::Refresh()
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );

   BOOL bOn = pContMode->IsStimRunning();                            // a stimulus run is in progress
   BOOL bStopping = pContMode->IsStimStopping();                     // in-progress run will stop at end of duty cycle
   BOOL bRecording = pContMode->IsRecording();                       // data recording in progress
   BOOL bIdle = !(bOn || bRecording);

   m_cbRunSet.EnableWindow( bIdle );
   m_cbCurrRun.EnableWindow( bIdle );
   m_cbRunMode.EnableWindow( bIdle );
   m_cbChanCfg.EnableWindow( bIdle );

   m_fecDataPath.EnableWindow( !bRecording );
   m_btnRecordSpks.EnableWindow( !bRecording );

   GetDlgItem( IDC_CONT_ABORT )->EnableWindow( bOn );
   GetDlgItem( IDC_CONT_RESTART )->EnableWindow( bOn );

   CWnd* pWnd = GetDlgItem( IDC_CONT_GO );
   pWnd->EnableWindow(                                               // "START/STOP" enabled iff
      ((!bOn) && (GetCurrentRun() != CX_NULLOBJ_KEY)) ||             //    run is OFF and a valid run obj is specified
      (bOn && !bStopping) );                                         //    run is ON but it is NOT "soft-stopping"
   CString strLabel = _T("START");                                   // PB label reflects operational state
   if( bStopping ) strLabel = _T("!!WAIT!!");
   else if( bOn ) strLabel = _T("STOP");
   GetDlgItem( IDC_CONT_GO )->SetWindowText( strLabel );

   pWnd = GetDlgItem( IDC_CONT_REC );                                // record PB hidden unless we're in "Manual" mode
   int nShow = (m_cbRunMode.GetCurSel() == CCxContMode::MANUAL) ? SW_SHOW : SW_HIDE;
   pWnd->ShowWindow( nShow );
   pWnd->SetWindowText( bRecording ? _T("Record ON") :               // record PB label reflects on/off recording state
                                     _T("Record OFF") );

   GetDlgItem( IDC_CONT_FIXATE )->SetWindowText(                     // fixate PB label reflects on/off fixation state
      pContMode->IsFixating() ? _T("Fixation ON") : _T("Fixation OFF") );
}


//=== OnUpdate [CCxControlPanelDlg override] ==========================================================================
//
//    CCxControlPanelDlg::OnUpdate() is a CNTRLX-specific extension of MFC's mechanism -- aka, CView::OnUpdate() -- for
//    informing all document views when one of those views causes a change in the active document's contents.  It
//    passes on the CNTRLX-specific doc/view hint (CCxViewHint) to the CNTRLX control panel dialogs, which may contain
//    document data.  When the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI apps,
//    this call is made each time a new document is created/opened -- giving us an opportunity to perform any "per-
//    document" initializations.
//
//    This dialog's IDC_CONT_SET, IDC_CONT_CURR, and IDC_CONT_CHANCFG combo boxes display CCxDoc-based data.  When the
//    hint is NULL -- meaning a new CCxDoc has just been created/opened --, we reinitialize the contents of the combo
//    boxes.  The run set and current run are set to "NONE", while the channel config is set to the first available
//    config object (CCxDoc guarantees at least one channel config is defined).  Otherwise we refresh the contents to
//    reflect any relevant changes in the current CCxDoc.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxContProtoDlg::OnUpdate( CCxViewHint* pHint )
{
   if( pHint == NULL )                                               // "per-document inits" -- reinitialize contents
   {                                                                 // of the CNTRLX object combo boxes:
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      ASSERT( pDoc != NULL );
      m_cbRunSet.InitContents( pDoc->GetBaseObj( CX_CONTRUNBASE ),   //    all run sets are children of this obj
                               TRUE );                               //    allow "NONE", which is selected initially
      m_cbCurrRun.InitContents( m_cbRunSet.GetObjKey(),              //    this cb always displays children of the
                                TRUE );                              //    currently selected run set.
      m_cbChanCfg.InitContents( pDoc->GetBaseObj( CX_CHANBASE ),     //    all channel cfgs are children of this obj;
                                FALSE );                             //    do not allow "NONE" choice
   }
   else if( !InitiatedUpdate() )                                     // update IAW change in current CCxDoc (unless dlg
   {                                                                 // itself initiated the update!):
      m_cbRunSet.RefreshContents();                                  //    refresh run set combo box contents
      WORD wCurrSet = m_cbRunSet.GetObjKey();                        //    current run combo box must always display
      if( wCurrSet != m_cbCurrRun.GetParentKey() )                   //    children of currently selected run set!
         m_cbCurrRun.InitContents( wCurrSet, TRUE );
      else
         m_cbCurrRun.RefreshContents();
      m_cbChanCfg.RefreshContents();
   }

   Refresh();                                                        // changes may affect appearance of other ctrls
}


//=== SetCurrentRun ===================================================================================================
//
//    Programmatically change the selection in the "current run" combo box (IDC_CONT_CURR) to the specified CNTRLX
//    object key.
//
//    ARGS:       wKey  -- [in] key of new selection.
//
//    RETURNS:    TRUE if successful, FALSE if key is not represented in the combo box.
//
BOOL CCxContProtoDlg::SetCurrentRun( const WORD wKey )
{
   return( BOOL(m_cbCurrRun.SetObjKey( wKey ) == wKey) );
}





//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxContFixTgtsDlg
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxContFixTgtsDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxContFixTgtsDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_CONT_TGT_ADD, IDC_CONT_TGT_CLEAR, OnBtnClicked )
   ON_UPDATE_COMMAND_UI( IDC_CONT_TGT_DEL, OnUpdTgtDel )
END_MESSAGE_MAP()


//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

const double CCxContFixTgtsDlg::MIN_ACVTGTPOS = -80.0; 
const double CCxContFixTgtsDlg::MAX_ACVTGTPOS = 80.0;
const double CCxContFixTgtsDlg::DEF_ACVTGTPOS = 0.0;
const double CCxContFixTgtsDlg::INC_ACVTGTPOS = 5.0; 

const double CCxContFixTgtsDlg::MIN_ACVTGTPATSPEED = -150.0;
const double CCxContFixTgtsDlg::MAX_ACVTGTPATSPEED = 150.0;
const double CCxContFixTgtsDlg::DEF_ACVTGTPATSPEED = 0.0;
const double CCxContFixTgtsDlg::INC_ACVTGTPATSPEED = 5.0; 

const double CCxContFixTgtsDlg::DEF_ACVTGTPATDIR = 0.0;
const double CCxContFixTgtsDlg::INC_ACVTGTPATDIR = 15.0;


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxContFixTgtsDlg, ~CCxContFixTgtsDlg [constructor, destructor] =================================================
//
//    Construct/destroy the dialog object (the destructor is inline).
//
CCxContFixTgtsDlg::CCxContFixTgtsDlg() : CCxControlPanelDlg( IDD )
{
   m_iFixTgt1 = -1;                             // initially, no fixation tgts and no "track" tgt
   m_iFixTgt2 = -1;
   m_iTrackTgt = -1;

   m_nActive = 0;                               // active target list is initially empty...
   for( int i = 0; i < MAX_ACTIVETGTS; i++ )
   {
      m_activeTgts[i].wKey = CX_NULLOBJ_KEY;
      m_activeTgts[i].bOn = FALSE;
      m_activeTgts[i].dHPos = DEF_ACVTGTPOS;
      m_activeTgts[i].dVPos = DEF_ACVTGTPOS;
      m_activeTgts[i].dSpeed = DEF_ACVTGTPATSPEED;
      m_activeTgts[i].dDir = DEF_ACVTGTPATDIR;
   }

   m_bAddingTarget = FALSE;
   m_wLastTgtKey = CX_NULLOBJ_KEY;
}



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnBtnClicked ====================================================================================================
//
//    Respond to the BN_CLICKED notification from various pushbutton controls on this dialog.  The control IDs below
//    must constitute a contiguous range of integers.
//
//       IDC_CONT_TGT_ADD  ==> Add a target to the active target list.
//       IDC_CONT_TGT_DEL  ==> Delete a target from the active target list.
//       IDC_CONT_TGT_CLEAR==> Clear the active target list.
//
//    NOTE:  For technical reasons related to the realization of XY scope and framebuffer video targets, we permit
//    changes in the active target list's composition (add, delete or replace a target) only when "inactive" -- ie,
//    when fixation and recording are OFF and no stimulus run is in progress.  The above controls should be enabled
//    only in this inactive operational state, but we check to be sure.  Since CXDRIVER does not use the active target
//    list while inactive during ContMode, there's no need to communicate each change in its composition; the entire
//    list is initialized just prior to leaving the inactive state.
//
//    ARGS:       id -- [in] control ID.
//
//    RETURNS:    NONE.
//
void CCxContFixTgtsDlg::OnBtnClicked( UINT id )
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );

   if( pContMode->IsActive() ) return;                            // ignore except when ContMode op state is inactive

   CCellID c;
   switch( id )
   {
      case IDC_CONT_TGT_ADD :                                     // if there's room, add a target -- first we must get
         if( m_nActive < MAX_ACTIVETGTS )                         // choice of target from user.  we do so by starting
         {                                                        // an inplace edit on grid cell (0,0).
            m_bAddingTarget = TRUE;                               //    setting this flag tags this as a special op
            m_grid.InitiateCellEdit( 0, 0 );
         }
         break;
      case IDC_CONT_TGT_DEL :
         c = m_grid.GetFocusCell();
         if( m_nActive > 0 && c.col == 0 && c.row > 0 )           // grid's focus cell must correspond to a target name
         {
            for( int i = c.row-1; i < m_nActive; i++ )            // since active target list is short, this is no
            {                                                     // big deal...
               m_activeTgts[i].wKey = m_activeTgts[i+1].wKey;
               m_activeTgts[i].bOn = m_activeTgts[i+1].bOn;
               m_activeTgts[i].dHPos = m_activeTgts[i+1].dHPos;
               m_activeTgts[i].dVPos = m_activeTgts[i+1].dVPos;
               m_activeTgts[i].dSpeed = m_activeTgts[i+1].dSpeed;
               m_activeTgts[i].dDir = m_activeTgts[i+1].dDir;
            }
            if( m_iFixTgt1 == c.row - 1 ) m_iFixTgt1 = -1;
            if( m_iFixTgt2 == c.row - 1 ) m_iFixTgt2 = -1;
            if( m_iTrackTgt == c.row - 1 ) m_iTrackTgt = -1;

            --m_nActive;

            m_grid.SetRowCount( m_nActive + 1 );
            m_grid.Refresh();
         }
         break;
      case IDC_CONT_TGT_CLEAR :
         if( m_nActive > 0 )
         {
            m_nActive = 0;
            m_iFixTgt1 = -1;
            m_iFixTgt2 = -1;
            m_iTrackTgt = -1;
            m_grid.SetRowCount( 1 );
            m_grid.Refresh();
         }
         break;
   }
}


//=== OnUpdTgtDel =====================================================================================================
//
//    ON_UPDATE_COMMAND_UI handler to update enable state of "Delete" button (IDC_CONT_TGT_DEL).  Unlike the other two
//    buttons, this button is enabled only when the focus cell is on the name of a target in the active list.  To
//    prevent changes in the active list's composition when the system is "active", the button is disabled.
//
//    ARGS:       pCmdUI   -- [in] represents UI item.
//
//    RETURNS:    NONE
//
void CCxContFixTgtsDlg::OnUpdTgtDel( CCmdUI* pCmdUI )
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );

   CCellID c = m_grid.GetFocusCell();                             // current focus cell on active tgt grid
   BOOL bEnable = (pCmdUI->m_nID == IDC_CONT_TGT_DEL) &&          // this is the only ctrl we deal with
                  (!pContMode->IsActive()) &&                     // system must NOT be active
                  (c.row > 0) && (c.col == 0);                    // focus cell must be a target name cell
   pCmdUI->Enable( bEnable );
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== SetAcvTgtOn =====================================================================================================
//
//    Update the ON/off state of the specified target in the active target list.  This method just updates the GUI;
//    caller must invoke appropriate method in CCxContMode to actually update the physical target in MaestroDRIVER.
//
//    ARGS:       i -- [in] index into active target list.
//                bOn -- [in] TRUE to turn on the target, FALSE to turn it off.
//    RETURNS:    TRUE if successful; FALSE if tgt index is invalid.
//
BOOL CCxContFixTgtsDlg::SetAcvTgtOn( int i, BOOL bOn )
{
   BOOL bOk = BOOL( i >= 0 && i < m_nActive );
   if( bOk && m_activeTgts[i].bOn != bOn)
   {
      m_activeTgts[i].bOn = bOn;
      m_grid.RedrawCell(i+1, 1);
   }
   return( bOk );
}

//=== SetAcvTgtPos ====================================================================================================
//
//    Update the current position of the specified target in the active target list.  This method just updates the GUI;
//    caller must invoke appropriate method in CCxContMode to actually update the physical target in MaestroDRIVER.
//
//    ARGS:       i     -- [in] index into active target list.
//                x,y   -- [in] coordinates of new target position, in degrees.  These are restricted to the range
//                         [MIN_ACVTGTPOS .. MAX_ACVTGTPOS].
//
//    RETURNS:    TRUE if successful; FALSE if tgt index is invalid.
//
BOOL CCxContFixTgtsDlg::SetAcvTgtPos( int i, double x, double y )
{
   BOOL bOk = BOOL( i >= 0 && i < m_nActive );
   if( bOk && (cMath::abs(m_activeTgts[i].dHPos - x) > 0.005 || cMath::abs(m_activeTgts[i].dVPos - y) > 0.005) )
   {
      m_activeTgts[i].dHPos = x;
      RangeLimit( m_activeTgts[i].dHPos, MIN_ACVTGTPOS, MAX_ACVTGTPOS );
      m_grid.RedrawCell(i+1, 3);

      m_activeTgts[i].dVPos = y;
      RangeLimit( m_activeTgts[i].dVPos, MIN_ACVTGTPOS, MAX_ACVTGTPOS );
      m_grid.RedrawCell(i+1, 4);
   }
   return( bOk );
}

//=== SetAcvTgtPatSpeed ===============================================================================================
//
//    Update the current pattern speed of the specified target in the active target list. This method just updates the 
//    GUI; caller must invoke appropriate method in CCxContMode to update the physical target in MaestroDRIVER.
//
//    ARGS:       i     -- [in] index into active target list.
//                speed -- [in] tgt pattern speed in deg/sec. Restricted to [MIN_ACVTGTPATSPEED .. MAX_ACVTGTPATSPEED].
//
//    RETURNS:    TRUE if successful; FALSE if tgt index is invalid.
//
BOOL CCxContFixTgtsDlg::SetAcvTgtPatSpeed(int i, double speed)
{
   BOOL bOk = BOOL( i >= 0 && i < m_nActive );
   if( bOk && (cMath::abs(m_activeTgts[i].dSpeed - speed) > 0.005) )
   {
      m_activeTgts[i].dSpeed = speed;
      RangeLimit( m_activeTgts[i].dSpeed, MIN_ACVTGTPATSPEED, MAX_ACVTGTPATSPEED );
      m_grid.RedrawCell(i+1, 5);
   }
   return( bOk );
}

//=== SetAcvTgtPatDir =================================================================================================
//
//    Update the current direction of pattern motion for the specified target in the active target list. This method 
//    just updates the GUI; caller must invoke appropriate method in CCxContMode to update the physical target in 
//    MaestroDRIVER.
//
//    ARGS:       i     -- [in] index into active target list.
//                speed -- [in] direction of target pattern motion in deg CCW from positive x-axis. Restricted to 
//                         [0..360).
//
//    RETURNS:    TRUE if successful; FALSE if tgt index is invalid.
//
BOOL CCxContFixTgtsDlg::SetAcvTgtPatDir(int i, double dir)
{
   BOOL bOk = BOOL( i >= 0 && i < m_nActive );
   if( bOk && (cMath::abs(m_activeTgts[i].dDir - dir) > 0.005) )
   {
      m_activeTgts[i].dDir = cMath::limitToUnitCircleDeg(dir);
      m_grid.RedrawCell(i+1, 6);
   }
   return( bOk );
}


//=== OnInitDialog [base override] ====================================================================================
//
//    Prepare the dialog for display.
//
//    Here we subclass dlg resource template-defined controls to class members, format the numeric edit ctrls, prepare
//    the grid control that will represent the active target list, and initialize all to "start-up" conditions.  See
//    GridDispCB() for a detailed explanation of the grid control's makeup.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for required rows and columns.
//
BOOL CCxContFixTgtsDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                               // let base class do its thing...

   m_btnTgtAdd.SubclassDlgItem( IDC_CONT_TGT_ADD, (CWnd*) this );    // subclass PBs
   m_btnTgtDel.SubclassDlgItem( IDC_CONT_TGT_DEL, (CWnd*) this );
   m_btnTgtClear.SubclassDlgItem( IDC_CONT_TGT_CLEAR, (CWnd*) this );

   m_grid.SubclassDlgItem( IDC_CONT_TGTS, this );                    // prepare grid ctrl to rep active tgt list...
   m_grid.EnableDragAndDrop( FALSE );                                // disable drag-n-drop features
   m_grid.SetRowResize( FALSE );                                     // user may not resize rows or columns
   m_grid.SetColumnResize( FALSE );
   m_grid.EnableSelection( FALSE );                                  // cells in grid cannot be selected

   m_grid.SetCallbackFunc( GridDispCB, (LPARAM) this );              // set callbacks which govern appearance/editing
   m_grid.SetEditCBFcn( GridEditCB, (LPARAM) this );                 // of grid cells.  TRICK: we pass THIS reference
   m_grid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );           // b/c CB fcn must be static
   m_grid.SetTreeInfoCBFcn( CCxDoc::TreeInfoCB,                      // note that we rely on document for CNTRLX obj
               (LPARAM) ((CCntrlxApp*) AfxGetApp())->GetDoc() );     // tree info...

   m_grid.SetRowCount( 1 );                                          // init grid with a single fixed row for column
   m_grid.SetColumnCount( 7 );                                       // header labels
   m_grid.SetFixedRowCount( 1 );                                     // (may THROW CMemoryException)
   m_grid.SetFixedColumnCount( 0 );

   CGridCellBase* pCell = m_grid.GetDefaultCell( TRUE, TRUE );       // set default cell formats
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( TRUE, FALSE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( FALSE, TRUE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( FALSE, FALSE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );

   m_grid.SetGridLineColor( RGB(0,0,0) );                            // use black grid lines

   m_grid.SetColumnWidth( 0, 140 );                                  // set column widths (which will never change)
   m_grid.SetColumnWidth( 1, 45 );
   m_grid.SetColumnWidth( 2, 45 );
   m_grid.SetColumnWidth( 3, 50 );
   m_grid.SetColumnWidth( 4, 50 );
   m_grid.SetColumnWidth( 5, 50 );
   m_grid.SetColumnWidth( 6, 50 );

   CRect rGrid;                                                      // resize grid window so that we never have to
   m_grid.GetWindowRect( rGrid );                                    // scroll horizontally...
   ScreenToClient( rGrid );
   CRect rClient;
   m_grid.GetClientRect( rClient );

   int iAdj = rGrid.Width() - rClient.Width() +                      // ...must acct for window borders, width of the
              ::GetSystemMetrics( SM_CXVSCROLL ) + 2;                // vert SB, and some slop

   rGrid.right = rGrid.left + 430 + iAdj;
   m_grid.MoveWindow( rGrid );

   m_btnTgtAdd.SetFocus();                                           // the "Add" button gets the focus initially
   return( FALSE );
}

//=== Refresh [CCxControlPanelDlg override] ===========================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    Here we update the ena/disabled state of the three PBs that adds a target to, deletes a target from, or clears
//    the active target list.  Such operations are allowed only when the system is inactive (active tgts not in use).
//    Furthermore, the "delete" button is enabled only when the active target grid's focus cell is on a target name.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContFixTgtsDlg::Refresh()
{
   CCxContMode* pContMode = (CCxContMode*) GetModeCtrl( CCxRuntime::ContMode );

   BOOL bEnabled = !pContMode->IsActive();
   m_btnTgtAdd.EnableWindow( bEnabled );
   m_btnTgtClear.EnableWindow( bEnabled );

   CCellID c = m_grid.GetFocusCell();
   m_btnTgtDel.EnableWindow( bEnabled && c.row > 0 && c.col == 0 );
}

//=== OnUpdate [base override] ========================================================================================
//
//    CCxControlPanelDlg::OnUpdate() is a CNTRLX-specific extension of MFC's mechanism -- aka, CView::OnUpdate() -- for
//    informing all document views when one of those views causes a change in the active document's contents.  It
//    passes on the CNTRLX-specific doc/view hint (CCxViewHint) to the CNTRLX control panel dialogs, which may contain
//    document data.  When the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI apps,
//    this call is made each time a new document is created/opened -- giving us an opportunity to perform any "per-
//    document" initializations.
//
//    If a target appearing in the active target list is renamed, we immediately update the target name appearing in
//    the grid control.  When a new CCxDoc is opened, we make sure the active list is empty.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxContFixTgtsDlg::OnUpdate( CCxViewHint* pHint )
{
   if( pHint == NULL )                                               // "per-document inits" -- make sure active target
   {                                                                 // list is empty.
      m_nActive = 0;
      m_wLastTgtKey = CX_NULLOBJ_KEY;
      m_grid.SetTreeInfoCBFcn( CCxDoc::TreeInfoCB,                   // since we rely on doc obj for the tree info CB,
               (LPARAM) ((CCntrlxApp*) AfxGetApp())->GetDoc() );     // we reinstall it to be safe...
      m_grid.Refresh();
   }
   else if( pHint->m_code == CXVH_NAMOBJ )                           // update name of a target in the active list, if
   {                                                                 // necessary.
      for( int i = 0; i < m_nActive; i++ ) if( m_activeTgts[i].wKey == pHint->m_key )
      {
         m_grid.RedrawCell( i+1, 0 );
         break;
      }
   }
}


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== GridDispCB ======================================================================================================
//
//    Callback function queried by the "active targets" grid control to obtain the contents of each cell in the grid.
//
//    The active target list is an (N+1)-by-7 grid, where N is the # of active targets currently defined, with a
//    single fixed row at the top displaying column headings.  For each target we display:
//       col 0:  Target name
//       col 1:  "ON"  or "off" state
//       col 2:  Usage designation: "none", "FIX1", "FIX2", "BOTH", or "TRACK"
//       col 3:  Current horizontal position of target (deg)
//       col 4:  Current vertical position of target (deg)
//       col 5:  Current speed of target pattern (deg/sec)
//       col 6:  Current direction of motion for target pattern (deg CCW from pos x-axis)
//    NOTE: A callback function must be static.  As such, it does not have access to instance fields of the object.  To
//    circumvent this problem, we take advantage of the generic LPARAM argument, passing a reference to this dlg when
//    we register the callback fcn with the grid in OnInitDialog().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info.
//                lParam      -- [in] THIS (see NOTE).
//
//    RETURNS:    TRUE if display info was provided; FALSE otherwise.
//
BOOL CALLBACK CCxContFixTgtsDlg::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam )
{
   CCxContFixTgtsDlg* pThis = (CCxContFixTgtsDlg*)lParam;               // to access non-static data in the dlg obj
   CActiveTgt* pTgts = &(pThis->m_activeTgts[0]);                       // current state of the active tgt list
   CLiteGrid* pGrid = &(pThis->m_grid);                                 // the grid control showing active tgt list

   CCellID c;                                                           // the cell whose info is requested
   c.row = pDispInfo->item.row;
   c.col = pDispInfo->item.col;
   int iTgt = c.row - 1;                                                // index of relevant tgt in active tgt list;
                                                                        // -1 corresponds to the col header row!

   if( (pGrid->GetSafeHwnd() == NULL) ||                                // FAIL if grid control is gone, or
       (!pGrid->IsValid( c )) ||                                        // specified cell is non-existent, or
       (iTgt < -1) || (iTgt >= pThis->m_nActive) )                      // invalid tgt index (just in case)
      return( FALSE );

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                  // we don't use label tips on this grid
   {
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
      return( TRUE );
   }

   switch( c.col )
   {
      case 0 :                                                          // "TARGET NAME" col holds name of active tgt
         if( iTgt == -1 ) pDispInfo->item.strText = _T("TARGET NAME");
         else
         {
            CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();        //    we need to query doc for tgt name
            WORD wKey = pTgts[iTgt].wKey;
            if( (pDispInfo->item.nState & GVIS_VIRTUALTITLETIP) != 0 )  //    title tip text shows tgt's "full" name
               pDoc->GetFullObjName( wKey, pDispInfo->item.strText );
            else                                                        //    normal text just shows immediate name
               pDispInfo->item.strText = pDoc->GetObjName( wKey );
         }
         break;

      case 1 :                                                          // "ON/off" col displays tgt's on/off state
         if( iTgt == -1 ) pDispInfo->item.strText = _T("ON/off");
         else pDispInfo->item.strText = pTgts[iTgt].bOn ? _T("ON") : _T("off");
         break;

      case 2 :                                                          // "USAGE" col displays special usage for the
         if( iTgt == -1 ) pDispInfo->item.strText = _T("USAGE");        // active tgt...
         else
         {
            if( pThis->m_iFixTgt1 == iTgt && pThis->m_iFixTgt2 == iTgt )   pDispInfo->item.strText = _T("BOTH");
            else if( pThis->m_iFixTgt2 == iTgt )                           pDispInfo->item.strText = _T("FIX2");
            else if( pThis->m_iFixTgt1 == iTgt )                           pDispInfo->item.strText = _T("FIX1");
            else if( pThis->m_iTrackTgt == iTgt )                          pDispInfo->item.strText = _T("TRACK");
            else                                                           pDispInfo->item.strText = _T("none");
         }
         break;

      case 3 :                                                          // "Xo" col displays current horizontal pos
         if( iTgt == -1 ) pDispInfo->item.strText = _T("Xo(deg)");
         else pDispInfo->item.strText.Format( "%.2f", pTgts[iTgt].dHPos );
         break;

      case 4 :                                                          // "Yo" col displays current vertical pos
         if( iTgt == -1 ) pDispInfo->item.strText = _T("Yo(deg)");
         else pDispInfo->item.strText.Format( "%.2f", pTgts[iTgt].dVPos );
         break;

      case 5 :                                                          // "V" col displays current pattern speed
         if( iTgt == -1 ) pDispInfo->item.strText = _T("V(deg/s)");
         else pDispInfo->item.strText.Format( "%.2f", pTgts[iTgt].dSpeed );
         break;

      case 6 :                                                          // "Dir" col displays current patn direction
         if( iTgt == -1 ) pDispInfo->item.strText = _T("Dir(deg)");
         else pDispInfo->item.strText.Format( "%.2f", pTgts[iTgt].dDir );
         break;
   }

   if( c.col > 0 || (c.col == 0 && c.row == 0) )                        // always show title tip for target name cells;
      pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                  // otherwise, only show it if the cell's text
                                                                        // is too big to fit...
   return( TRUE );
}


//=== GridEditCB ======================================================================================================
//
//    Callback invoked to initiate inplace editing of a cell in the active targets list, or to increment/decrement
//    the contents of a cell in response to a left or right mouse click.  Below is a summary of the possible operations
//    that this callback permits:
//
//       1) Cell in row 0  ==> These are merely read-only column labels.  Cannot be edited.
//       2) Cell in row 1-N, col 0 ==> Name of target N-1 in the active list.  Treated as a "treechoice" parameter, the
//          target name is changed using the grid's embedded tree control.  A left- or right-click has no effect.  See
//          also TargetTreeInfoCB().  Note that we allow target identity changes only when CNTRLX is inactive (ie, no
//          stimulus running, recording & fixation off).
//       3) Cell in row 1-N, col 1 ==> ON/off state for active target N-1.  Multichoice.  A left or right mouse click
//          will toggle its state.
//       4) Cell in row 1-N, col 2 ==> Usage designation for active target N-1.  Multichoice.  A left (right) click
//          increments (decrements) the current choice in the set: "none", "FIX1", "FIX2", "BOTH", "TRACK".  Note that
//          changing a target's usage designation may affect the usage designation of another target in the list!
//       5) Cell in row 1-N, col 3  ==> Horizontal position of active target N-1.  Floating-point numeric.  A left
//          (right) click will increment (decrement) the position by CCxRunFixTgtsDlg::INC_ACVTGTPOS.
//       6) Cell in row 1-N, col 4  ==> Vertical position of active target N-1.  Floating-point numeric.  A left
//          (right) click will increment (decrement) the position by CCxRunFixTgtsDlg::INC_ACVTGTPOS.
//       7) Cell in row 1-N, col 5  ==> Pattern speed for active target N-1.  Floating-point numeric.  A left
//          (right) click will increment (decrement) the position by CCxRunFixTgtsDlg::INC_ACVTGTPATSPEED.
//       6) Cell in row 1-N, col 6  ==> Pattern direction for active target N-1.  Floating-point numeric.  A left
//          (right) click will increment (decrement) the position by CCxRunFixTgtsDlg::INC_ACVTGTPATDIR, and the 
//          result is then wrapped into the range [0..360) deg.
//
//    SPECIAL CASE:  When adding a target to the active list, we set an internal flag and then programmatically start
//    an edit operation on cell(0,0), which is normally read-only.  A row is not added to the grid until the user
//    selects a unique target -- see GridEndEditCB().
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxContFixTgtsDlg::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxContFixTgtsDlg* pThis = (CCxContFixTgtsDlg*)lParam;               // to access non-static data in the dlg obj
   CCxContMode* pContMode =                                             // to access ContMode controller
      (CCxContMode*) pThis->GetModeCtrl( CCxRuntime::ContMode );
   CActiveTgt* pTgts = &(pThis->m_activeTgts[0]);                       // current state of the active tgt list
   CLiteGrid* pGrid = &(pThis->m_grid);                                 // the grid control showing active tgt list

   CCellID c = pEI->cell;                                               // the cell to be edited
   int iTgt = c.row - 1;                                                // index of relevant tgt in active tgt list

   ASSERT( (!pThis->m_bAddingTarget) || (c.row==0 && c.col==0) );       // internal consistency check

   if( (pGrid->GetSafeHwnd() == NULL) ||                                // FAIL if grid control is gone, or
       (!pGrid->IsValid( c )) ||                                        // specified cell is non-existent, or
       (iTgt < -1) || (iTgt >= pThis->m_nActive) )                      // invalid tgt index (just in case)
   {
      pThis->m_bAddingTarget = FALSE;
      return( FALSE );
   }

   if( (pThis->m_bAddingTarget || (c.row > 0 && c.col==0)) &&           // prevent change in COMPOSITION of active tgt
       pContMode->IsActive() )                                          // list if we're in an active state in ContMode
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
      pThis->m_bAddingTarget = FALSE;
      return( TRUE );
   }

   if( c.row == 0 && !pThis->m_bAddingTarget )                          // if cell is in col header row, then there's
   {                                                                    // nothing to edit.  EXCEPTION: when adding a
      pEI->iClick = 0;                                                  // tgt we force an inplace edit on cell(0,0)!!
      pEI->iType = LG_READONLY;
      return( TRUE );
   }

   double* pdCoord;

   switch( c.col )
   {
      case 0 :                                                          // target identity:
         if( pEI->iClick != 0 )                                         //    mouse clicks have no effect
            pEI->iClick = 0;
         else                                                           //    edited as a "treechoice" parameter: must
         {                                                              //    prepare a chain of CNTRLX obj keys from
            pEI->iType = LG_TREECHOICE;                                 //    root of target tree to the current tgt.
            WORD wSelKey = pThis->m_bAddingTarget ?                     //    if adding tgt, we select tree node for
                                    pThis->m_wLastTgtKey :              //    last tgt added to list; else we select
                                    pTgts[iTgt].wKey;                   //    the node for current target.
            CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();
            pDoc->PrepareKeyChain( pEI->dwArKeyChain, CX_TARGBASE, wSelKey );
         }
         break;

      case 1 :                                                          // target's ON/off state:
         if( pEI->iClick != 0 )                                         //    mouse click toggles state
            pTgts[iTgt].bOn = !(pTgts[iTgt].bOn);
         else                                                           //    edited as multichoice parameter with
         {                                                              //    two possible values, ON or off
            pEI->iType = LG_MULTICHOICE;
            pEI->strArChoices.Add( _T("off") );
            pEI->strArChoices.Add( _T("ON") );
            pEI->iCurrent = pTgts[iTgt].bOn ? 1 : 0;
         }
         break;

      case 2 :                                                          // target's usage designation:
         if( pEI->iClick != 0 )                                         //    if mouse clicked, advance to next choice:
         {
            BOOL bSideEffect = TRUE;                                    //       could this change have a side effect?
            if(pThis->m_iTrackTgt == iTgt)                              //       "track" ==> "none"
            {
               pThis->m_iTrackTgt = -1;
               bSideEffect = FALSE;
            }
            else if(pThis->m_iFixTgt1 == iTgt &&                        //       "both" ==> "track"
               pThis->m_iFixTgt2 == iTgt)
            {
               pThis->m_iFixTgt1 = pThis->m_iFixTgt2 = -1;
               pThis->m_iTrackTgt = iTgt;
            }
            else if( pThis->m_iFixTgt2 == iTgt )                        //       "fix2" ==> "both"
               pThis->m_iFixTgt1 = iTgt;
            else if( pThis->m_iFixTgt1 == iTgt )                        //       "fix1" ==> "fix2"
            {
               pThis->m_iFixTgt1 = -1;
               pThis->m_iFixTgt2 = iTgt;
            }
            else                                                        //       "none" ==> "fix1"
               pThis->m_iFixTgt1 = iTgt;

            if( bSideEffect && pThis->m_nActive > 1 )                   //    redraw entire column to reflect any
               pGrid->RedrawColumn( c.col );                            //    possible side effects of this change!
         }
         else                                                           //    edited as multichoice parameter...
         {
            pEI->iType = LG_MULTICHOICE;
            pEI->strArChoices.Add( _T("none") );
            pEI->strArChoices.Add( _T("FIX1") );
            pEI->strArChoices.Add( _T("FIX2") );
            pEI->strArChoices.Add( _T("BOTH") );
            pEI->strArChoices.Add( _T("TRACK") );

            if( pThis->m_iTrackTgt == iTgt ) pEI->iCurrent = 4;
            else if( pThis->m_iFixTgt1 == iTgt && pThis->m_iFixTgt2 == iTgt ) pEI->iCurrent = 3;
            else if( pThis->m_iFixTgt2 == iTgt ) pEI->iCurrent = 2;
            else if( pThis->m_iFixTgt1 == iTgt ) pEI->iCurrent = 1;
            else pEI->iCurrent = 0;
         }
         break;

      case 3 :                                                          // target's horizontal pos in deg, OR
      case 4 :                                                          // target's vertical pos in deg:
         if( c.col == 3 ) pdCoord = &(pTgts[iTgt].dHPos);
         else pdCoord = &(pTgts[iTgt].dVPos);

         if( pEI->iClick > 0 )                                          //    left click increments pos by a set
         {                                                              //    amount, subj to range restrictions
            if( *pdCoord == MAX_ACVTGTPOS )
               pEI->iClick = 0;
            else
            {
               *pdCoord += INC_ACVTGTPOS;
               RangeLimit( *pdCoord, MIN_ACVTGTPOS, MAX_ACVTGTPOS );
            }
         }
         else if( pEI->iClick < 0 )                                     //    right click decrements pos by a set
         {                                                              //    amount, subj to range restrictions
            if( *pdCoord == MIN_ACVTGTPOS )
               pEI->iClick = 0;
            else
            {
               *pdCoord -= INC_ACVTGTPOS;
               RangeLimit( *pdCoord, MIN_ACVTGTPOS, MAX_ACVTGTPOS );
            }
         }
         else                                                           //    edited as a FP numeric text string
         {
            pEI->iType = LG_NUMSTR;
            pEI->numFmt.flags = 0;
            pEI->numFmt.nLen = 6;
            pEI->numFmt.nPre = 2;
            pEI->dCurrent = *pdCoord;
         }
         break;

      case 5 :                                                          // pattern speed of target, in deg/sec
         if(pEI->iClick > 0)                                            //    L/R click incr/decr speed by a set 
         {                                                              //    amt, sub to range restrictions
            if(pTgts[iTgt].dSpeed == MAX_ACVTGTPATSPEED)
               pEI->iClick = 0;
            else
            {
               pTgts[iTgt].dSpeed += INC_ACVTGTPATSPEED;
               RangeLimit(pTgts[iTgt].dSpeed, MIN_ACVTGTPATSPEED, MAX_ACVTGTPATSPEED);
            }
         }
         else if(pEI->iClick < 0)
         {
            if(pTgts[iTgt].dSpeed == MIN_ACVTGTPATSPEED)
               pEI->iClick = 0;
            else
            {
               pTgts[iTgt].dSpeed -= INC_ACVTGTPATSPEED;
               RangeLimit(pTgts[iTgt].dSpeed, MIN_ACVTGTPATSPEED, MAX_ACVTGTPATSPEED);
            }
         }
         else                                                           //    edited as FP numeric text string
         {
            pEI->iType = LG_NUMSTR;
            pEI->numFmt.flags = 0;
            pEI->numFmt.nLen = 6;
            pEI->numFmt.nPre = 2;
            pEI->dCurrent = pTgts[iTgt].dSpeed;
         }
         break;

      case 6 :                                                          // direc of tgt pattern motion, in deg CCW
         if(pEI->iClick > 0)                                            //    L/R click incr/decr direction by a set 
         {                                                              //    amt, then wraps into [0..360)
            pTgts[iTgt].dDir = cMath::limitToUnitCircleDeg( pTgts[iTgt].dDir + INC_ACVTGTPATSPEED);
         }
         else if(pEI->iClick < 0)
         {
            pTgts[iTgt].dDir = cMath::limitToUnitCircleDeg( pTgts[iTgt].dDir - INC_ACVTGTPATSPEED);
         }
         else                                                           //    edited as FP numeric text string
         {
            pEI->iType = LG_NUMSTR;
            pEI->numFmt.flags = 0;
            pEI->numFmt.nLen = 6;
            pEI->numFmt.nPre = 2;
            pEI->dCurrent = pTgts[iTgt].dDir;
         }
         break;

      default :                                                         // we should NEVER get here!
         ASSERT( FALSE );
         break;
   }

   if( pEI->iClick != 0 && pContMode->IsActive() )                      // if a mouse click has modified an active tgt
   {                                                                    // param and system is in active state, we must
      if( c.col == 2 ) pContMode->UpdateActiveFixTargets();             // inform CXDRIVER via parent ctrl panel
      else pContMode->UpdateActiveTarget( iTgt );
   }

   return( TRUE );
}


//=== GridEndEditCB ===================================================================================================
//
//    Callback invoked upon termination of inplace editing of the active targets grid.
//
//    Here we update the active targets list IAW the change made during the inplace operation that was configured in
//    GridEditCB().  We employ the grid's default navigation rules for initiating an inplace operation on a neighboring
//    cell IAW the exit character that extinguished the current inplace edit.
//
//    SPECIAL CASE:  A target is added to the active list by self-initiating an inplace operation on cell(0,0), which
//    is normally read-only.  If the user specifies the key of a target that is not already in the active list, we add
//    a row for that target and refresh the entire active target grid.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxContFixTgtsDlg::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   CCxContFixTgtsDlg* pThis = (CCxContFixTgtsDlg*)lParam;               // to access non-static data in the dlg obj
   CCxContMode* pContMode =                                             // to access ContMode controller
      (CCxContMode*) pThis->GetModeCtrl( CCxRuntime::ContMode );
   CActiveTgt* pTgts = &(pThis->m_activeTgts[0]);                       // current state of the active tgt list
   CLiteGrid* pGrid = &(pThis->m_grid);                                 // the grid control showing active tgt list

   BOOL bAddingTgt = pThis->m_bAddingTarget;                            // were we adding a tgt to the list?
   pThis->m_bAddingTarget = FALSE;                                      // reset transient flag in case we abort

   if( pEEI->nExitChar == VK_ESCAPE )                                   // user cancelled
      return( TRUE );

   CCellID c = pEEI->cell;                                              // the cell that was edited
   int iTgt = c.row - 1;                                                // index of relevant tgt in active tgt list

   if( (pGrid->GetSafeHwnd() == NULL) ||                                // FAIL if grid control is gone, or
       (!pGrid->IsValid( c )) ||                                        // specified cell is non-existent, or
       (iTgt < -1) || (iTgt >= pThis->m_nActive) )                      // invalid tgt index (just in case)
      return( FALSE );

   if( c.row == 0 && !bAddingTgt )                                      // we just finished editing a readonly cell!
   {
      ASSERT( FALSE );                                                  //    this should NEVER happen!
      pEEI->nExitChar = VK_ESCAPE;                                      //    prevent continued inplace editing
      pEEI->bNoRedraw = TRUE;                                           //    no need to redraw since no change made
      return( TRUE );
   }

   if( pEEI->bIsChanged || bAddingTgt )                                 // if user actually changed something, or was
   {                                                                    // adding a tgt (special case), update active
      double* pdCoord;                                                  // target list accordingly...
      WORD wTgKey;
      CCxDoc* pDoc;
      BOOL bOk;
      int i;
      switch( c.col )
      {
         case 0 :                                                       // target identity:
            wTgKey = LOWORD(pEEI->dwNew);                               //    the key of tgt chosen by user
            pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();               //    chosen key is invalid if...
            bOk = pDoc->ObjExists( wTgKey );                            //       the tgt obj does not exist
            if( bOk )
            {
               WORD wType = pDoc->GetObjType( wTgKey );
               bOk = (wType>=CX_FIRST_TARG) && (wType<=CX_LAST_TARG)    //       it is not a valid tgt type or it is
                     && (wType != CX_CHAIR);                            //       the animal chair
            }
            for( i = 0; bOk && i < pThis->m_nActive; i++ )              //       the tgt is already in the list!
            {
               if( pTgts[i].wKey == wTgKey && (iTgt < 0 || i != iTgt) )
                  bOk = FALSE;
            }

            if( !bOk )                                                  //    if new tgt key invalid, reject it --
            {                                                           //    forcing the inplace op to start again
               ((CCntrlxApp*) AfxGetApp())->LogMessage(
                  _T("(!!)Must specify a valid target not already in active list! Chair not allowed.") );
               pEEI->bReject = TRUE;
               pThis->m_bAddingTarget = bAddingTgt;                     //    restore state of "add tgt" flag
            }
            else                                                        //    otherwise:
            {
               if( iTgt >= 0 )                                          //    update identity of existing tgt, or...
                  pTgts[iTgt].wKey = wTgKey;
               else                                                     //    append a new tgt to the active list!
               {
                  ASSERT( pThis->m_nActive < MAX_ACTIVETGTS );
                  pTgts[pThis->m_nActive].wKey = wTgKey;
                  pTgts[pThis->m_nActive].bOn = FALSE;
                  pTgts[pThis->m_nActive].dHPos = DEF_ACVTGTPOS;
                  pTgts[pThis->m_nActive].dVPos = DEF_ACVTGTPOS;
                  pTgts[pThis->m_nActive].dSpeed = DEF_ACVTGTPATSPEED;
                  pTgts[pThis->m_nActive].dDir = DEF_ACVTGTPATDIR;
                  ++pThis->m_nActive;
                  pGrid->SetRowCount( pThis->m_nActive + 1 );
                  pGrid->Refresh();
                  pEEI->bNoRedraw = TRUE;
                  pEEI->nExitChar = 0;                                  //    prevent continuation after adding a tgt
               }

               pThis->m_wLastTgtKey = wTgKey;                           //    remember key of tgt that was selected!
            }
            break;

         case 1 :                                                       // target's on/off state:
            pTgts[iTgt].bOn = BOOL(pEEI->dwNew != 0);
            break;

         case 2 :                                                       // target's usage designation
            switch( pEEI->dwNew )
            {
               case 4 :                                                 //    "TRACK"
                  pThis->m_iTrackTgt = iTgt;
                  break;
               case 3 :                                                 //    "BOTH"
                  pThis->m_iFixTgt1 = pThis->m_iFixTgt2 = iTgt;
                  break;
               case 2 :                                                 //    "FIX2"
                  pThis->m_iFixTgt2 = iTgt;
                  if( pThis->m_iFixTgt1 == iTgt ) pThis->m_iFixTgt1 = -1;
                  break;
               case 1 :                                                 //    "FIX1"
                  pThis->m_iFixTgt1 = iTgt;
                  if( pThis->m_iFixTgt2 == iTgt ) pThis->m_iFixTgt2 = -1;
                  break;
               case 0 :                                                 //    "none"
                  if( pThis->m_iFixTgt1 == iTgt ) pThis->m_iFixTgt1 = -1;
                  if( pThis->m_iFixTgt2 == iTgt ) pThis->m_iFixTgt2 = -1;
                  if( pThis->m_iTrackTgt == iTgt ) pThis->m_iTrackTgt = -1;
                  break;
            }
            pGrid->RedrawColumn( c.col );                               // changing tgt's fix desig can affect others'
            pEEI->bNoRedraw = TRUE;                                     // since we already redrew entire column!
            break;

         case 3 :                                                       // target's H or V position
         case 4 :
            if( c.col == 3 ) pdCoord = &(pTgts[iTgt].dHPos);
            else pdCoord = &(pTgts[iTgt].dVPos);
            *pdCoord = pEEI->dNew;
            RangeLimit( *pdCoord, MIN_ACVTGTPOS, MAX_ACVTGTPOS );
            break;

         case 5 :                                                       // target's pattern speed
            pTgts[iTgt].dSpeed = pEEI->dNew;
            RangeLimit(pTgts[iTgt].dSpeed, MIN_ACVTGTPATSPEED, MAX_ACVTGTPATSPEED);
            break;

         case 6 :                                                       // target's pattern direction, in [0..360deg)
            pTgts[iTgt].dDir = cMath::limitToUnitCircleDeg(pEEI->dNew);
            break;

         default :                                                      // we should NEVER get here!
            ASSERT( FALSE );
            break;
      }

      if( pContMode->IsActive() && c.col > 0 )                          // inform CXDRIVER only when in active state
      {
         if( c.col == 2 ) pContMode->UpdateActiveFixTargets();
         else pContMode->UpdateActiveTarget( iTgt );
      }
   }

   return( TRUE );
}





//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxContMode
//
//=====================================================================================================================
//=====================================================================================================================

//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

const int CCxContMode::MANUAL                = 0;                 // stimulus run exec modes in ContMode
const int CCxContMode::AUTORECORD            = 1;
const int CCxContMode::REPEAT                = 2;

LPCTSTR CCxContMode::strModes[] =                                 // short names for each stimulus run mode
{
   _T("Manual"),
   _T("Auto Record"),
   _T("Single-run Repeat"),
};



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxContMode [constructor] =======================================================================================
//
CCxContMode::CCxContMode( CCxControlPanel* pPanel ) : CCxModeControl( CCxRuntime::ContMode, pPanel )
{
   m_pProtoDlg = NULL;
   m_pTgtsDlg = NULL;
   m_pFixRewDlg = NULL;
   m_pVideoDspDlg = NULL;
   m_pEyelinkDlg = NULL;
   m_dwLastOpState = 0;
   m_bWaiting = FALSE;
   m_strShadowPath.Empty();
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== InitDlgs [CCxModeControl override] ==============================================================================
//
//    Install, in the CNTRLX master mode control panel, those dialogs required for operator interactions in the CNTRLX
//    operational mode represented by this mode controller.
//
//    A total of five dialogs are currently required during ContMode.  We install the two ContMode-specific dialogs
//    here.  If the non-specific dialogs have not yet been installed, they are also installed here; else, we merely 
//    save pointers to them.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (failed to create one of the required dialogs).
//
BOOL CCxContMode:: InitDlgs()
{
   ASSERT( m_pCtrlPanel );                                              // verify our ptr to the mode control panel
   ASSERT_KINDOF( CCxControlPanel, m_pCtrlPanel );

   m_pProtoDlg = (CCxContProtoDlg*) m_pCtrlPanel->AddDlg( _T("Protocol"), RUNTIME_CLASS(CCxContProtoDlg) );
   if( m_pProtoDlg == NULL ) return( FALSE );

   m_pTgtsDlg = (CCxContFixTgtsDlg*) m_pCtrlPanel->AddDlg( _T("Active Targets"), RUNTIME_CLASS(CCxContFixTgtsDlg) );
   if( m_pTgtsDlg == NULL ) return( FALSE );

   m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxFixRewDlg) );
   if( m_pFixRewDlg == NULL )
   {
      m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->AddDlg( _T("Fix/Reward"), RUNTIME_CLASS(CCxFixRewDlg) );
      if( m_pFixRewDlg == NULL ) return( FALSE );
   }

   m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxVideoDspDlg) );
   if( m_pVideoDspDlg == NULL )
   {
      m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->AddDlg( _T("Video Display"), RUNTIME_CLASS(CCxVideoDspDlg) );
      if( m_pVideoDspDlg == NULL ) return( FALSE );
   }

   m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxEyelinkDlg) );
   if( m_pEyelinkDlg == NULL )
   {
      m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->AddDlg( _T("EyeLink"), RUNTIME_CLASS(CCxEyelinkDlg) ); 
      if( m_pEyelinkDlg == NULL ) return( FALSE );
   }

   return( TRUE );
}


//=== Service [CCxModeControl override] ===============================================================================
//
//    Update runtime state in Continuous Mode.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::Service()
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CCxRuntime::ContMode) );

   int n = GetNumRewardsDelivered();                              // make sure reward statistics are up-to-date
   int total = GetCumulativeReward();
   m_pFixRewDlg->UpdateRewardStats( n, total );

   DWORD dwOldOpState = m_dwLastOpState;                          // remember CXDRIVER's operational state the last
   BOOL bWasRunning = IsStimRunning();                            // time we checked...
   BOOL bWasStopping = IsStimStopping();

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();             // now get current operational state -- so we can
   BOOL bRefresh = BOOL(dwOldOpState != m_dwLastOpState);         // detect a change in state

   if( m_bWaiting )                                               // if waiting to start stim run in "Repeat" mode:
   {
      if( (m_pProtoDlg->GetRunMode() != REPEAT) ||                // we should be in Repeat mode w/ recording on. If
          !IsRecording() )                                        // this is not the case -- perhaps b/c of an
      {                                                           // unexpected error on CXDRIVER side -- then we
         m_bWaiting = FALSE;                                      // cannot start the stimulus!
         bRefresh = TRUE;
      }
      else if( m_waitTime.Get() > 500000.0 )                      // else, if 500ms have elapsed since recording
      {                                                           // started, start a new stimulus run. On failure,
         m_bWaiting = FALSE;                                      // stop recording and discard data file.
         if( !StartStimulusRun() )
            StopRecord( FALSE );
         bRefresh = FALSE;
      }
   }

   if( bWasRunning && !IsStimRunning() )                          // if a stimulus run has just stopped:
   {
      int iRunMode = m_pProtoDlg->GetRunMode();                   // if not in "Manual" mode, we must stop recording,
      if( iRunMode != MANUAL )                                    // saving the data recorded. Then, if in Repeat mode,
      {                                                           // we start a new rep unless the seq was stopped or
         BOOL bOk = StopRecord( TRUE );                           // we were unable to save the last data file.
         if( (iRunMode == REPEAT) && bOk && !bWasStopping )
            Restart();
         bRefresh = FALSE;
      }
   }

   if( bRefresh ) Refresh();                                      // refresh panel's appearance if necessary

   UpdateCursorTrackingTarget();                                  // update pos of cursor tracking tgt, if any
}


//=== Enter, Exit [CCxModeControl overrides] ==========================================================================
//
//    Enter() should perform any initializations upon entering the operational mode represented by the mode controller,
//    while Exit() handles any cleanup activities just prior to exiting the mode.  One task that the mode controller
//    must handle is to update the subset of dialogs that are accessible on the mode control panel IAW the current op
//    mode.  It is recommended that the mode controller "hide" all dialogs in Exit(), and "show" only the relevant
//    dialogs in Enter().
//
//    We enter or leave ContMode in an "inactive" state, with no stimulus running, recording off, and fixation off.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxContMode::Enter()
{
   ASSERT( m_pRuntime );                                                // MUST be in ContMode!
   if( m_pRuntime->GetMode() != CCxRuntime::ContMode ) return( FALSE );

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->ShowDlg( m_pProtoDlg, -1 );                            // show the relevant mode ctrl dlgs
   m_pCtrlPanel->ShowDlg( m_pTgtsDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pFixRewDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pVideoDspDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pEyelinkDlg, -1 );
   m_pCtrlPanel->SetActiveDlg( m_pProtoDlg );                           // "Protocol" dlg is in front initially

   m_dwLastOpState = 0;                                                 // inactive upon entering mode
   Refresh();                                                           // force a refresh when we first enter mode
   UpdateVideoCfg();                                                    // make sure video display and fixation/reward
   UpdateFixRewSettings();                                              // settings are up-to-date on CXDRIVER side
   UpdateActiveTarget( -1 );                                            // update the current active tgt list
   UpdateActiveFixTargets();
   SetTraces( m_pProtoDlg->GetChanCfg(), 5000 );                        // start trace display

   return( TRUE );
}

BOOL CCxContMode::Exit()
{
   ASSERT( m_pRuntime );                                                // MUST be in ContMode!
   if( m_pRuntime->GetMode() != CCxRuntime::ContMode ) return( FALSE );


   Abort();                                                             // stop any stimulus run in progress
   StopRecord( FALSE );                                                 // stop recording NOW (data is discarded)
   if( IsFixating() ) ToggleFixate();                                   // stop fixating
   SetTraces( CX_NULLOBJ_KEY, 5000 );                                   // stop trace display

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->HideDlg( NULL );                                       // hide all mode ctrl dlgs currently visible

   return( TRUE );
}


//=== Go ==============================================================================================================
//
//    Start a stimulus run in ContMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CString ops may throw CMemoryException.
//
VOID CCxContMode::Go()
{
   ASSERT( m_pRuntime );

   int iRunMode = m_pProtoDlg->GetRunMode();
   WORD wRunKey = m_pProtoDlg->GetCurrentRun();
   if( (m_pRuntime->GetMode() != CCxRuntime::ContMode) ||               // must be in ContMode & not running a stimulus
       IsStimRunning() || (wRunKey == CX_NULLOBJ_KEY) ||                // a stimulus run must be selected by user
       (iRunMode != CCxContMode::MANUAL && IsRecording()) )             // recording must be off to start in any mode
   {                                                                    // other than manual
      TRACE0( "\nInconsistent state in CCxContRunPanel::Go()" );
      ASSERT( FALSE );
      return;
   }

   if( !m_pRuntime->LoadStimulusRun( wRunKey,                           // upload stim run defn to CXDRIVER; in REPEAT
                                     BOOL(iRunMode==REPEAT) ) )         // mode, autostop # must be nonzero!
      return;

   if( !IsActive() )                                                    // if we're not already in "active" state, we
   {                                                                    // must load entire active target list before
      if( !UpdateActiveTarget( -1 ) ) return;                           // starting the stimulus run
   }

   Restart();                                                           // start run -- as appropriate for run mode
}


//=== Halt ============================================================================================================
//
//    If a stimulus run is in progress, issue command to stop it at the end of the current duty cycle ("soft-stop").
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::Halt()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::ContMode) && ((m_dwLastOpState & CX_FC_RUNON) != 0) )
   {
      StopStimulusRun( FALSE, FALSE );
   }
}


//=== Abort ===========================================================================================================
//
//    If a stimulus run is in progress, issue command to stop it now.  In AUTORECORD and REPEAT modes, also stop
//    recording immediately and discard any data collected.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::Abort()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::ContMode) && IsStimRunning() )
   {
      BOOL bAbortRec = BOOL(m_pProtoDlg->GetRunMode() != CCxContMode::MANUAL);
      StopStimulusRun( TRUE, bAbortRec );
   }
}


//=== Restart =========================================================================================================
//
//    Abort and restart the currently defined stimulus run IAW the current run mode.  We assume here that a stimulus
//    run defn and the current active target list have already been uploaded to CXDRIVER.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::Restart()
{
   Abort();                                                             // abort ongoing run

   switch( m_pProtoDlg->GetRunMode() )                                  // how we start up depends on run mode:
   {
      case CCxContMode::MANUAL :                                        // in "Manual" mode, just start the defined run
         StartStimulusRun();
         break;
      case CCxContMode::AUTORECORD :                                    // in "Auto Record", we start recording, then
         if( StartRecord() )                                            // start the run
         {
            if( !StartStimulusRun() ) StopRecord( FALSE );
         }
         break;
      case CCxContMode::REPEAT :                                        // in "Single-run Repeat", we start recording,
         if( StartRecord() )                                            // then set timer for a delayed run start
         {
            m_bWaiting = TRUE;
            m_waitTime.Reset();
            Refresh();                                                  // because we set wait flag
         }
         break;
      default :
         TRACE0( "\nIllegal run mode in CCxContMode::Restart()" );
         ASSERT( FALSE );
         break;
   }
}


//=== ToggleRecord ====================================================================================================
//
//    Turn recording on/off during ContMode -- in the MANUAL execution mode only!  This is because the recording state
//    is independent of stimulus run control only in MANUAL mode.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE may indicate catastrophic failure w/ CXDRIVER.
//
BOOL CCxContMode::ToggleRecord()
{
   if( m_pProtoDlg->GetRunMode() != MANUAL ) return( FALSE );

   BOOL bOk;
   if( IsRecording() )
     bOk = StopRecord( TRUE );
   else
   {
      if( !IsActive() )                                                 // if not in "active" state, load entire active
         UpdateActiveTarget( -1 );                                      // target list first
      bOk = StartRecord();
   }

   return( bOk );
}


//=== ToggleFixate ====================================================================================================
//
//    Turn fixation checking on/off during ContMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE indicates catastrophic failure w/ CXDRIVER.
//
BOOL CCxContMode::ToggleFixate()
{
   ASSERT( m_pRuntime && m_pRuntime->GetMode() == CCxRuntime::ContMode );

   DWORD dwCmd;
   BOOL bOk = TRUE;
   if( IsFixating() )                                                   // turn OFF fixation checking
   {
      dwCmd = CX_CM_FIXOFF;
      bOk = m_pRuntime->SendCommand( dwCmd, NULL, NULL, 0,0,0,0 );
   }
   else                                                                 // turn ON fixation checking
   {
      if( !IsActive() )                                                 // if not in "active" state, load entire active
         bOk = UpdateActiveTarget( -1 );                                // target list first

      if( bOk ) bOk = UpdateActiveFixTargets();                         // ensure fix tgt designations are up-to-date
      dwCmd = CX_CM_FIXON;
      if( bOk )                                                         // send cmd to enable fixation checking
         bOk = m_pRuntime->SendCommand( dwCmd, NULL, NULL, 0,0,0,0 );
   }

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();                   // refresh internal copy of op state and
   Refresh();                                                           // refresh appearance of panel dialogs
   return( bOk );
}


//=== UpdateActiveFixTargets ==========================================================================================
//
//    Send identities of fixation targets #1 and #2 and the "cursor tracking" target (as indices into ContMode's
//    "active target list") to CXDRIVER via the CX_CM_UPDFIXTGTS command.  This command may be sent at any time during
//    ContMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE indicates catastrophic failure w/ CXDRIVER.
//
BOOL CCxContMode::UpdateActiveFixTargets()
{
   ASSERT( m_pRuntime && m_pRuntime->GetMode() == CCxRuntime::ContMode );

   DWORD dwCmd;
   int iArg[3];

   dwCmd = CX_CM_UPDFIXTGTS;
   iArg[0] = m_pTgtsDlg->GetActiveFixTgt1();                            // index of fix tgt #1 in active target list
   iArg[1] = m_pTgtsDlg->GetActiveFixTgt2();                            // index of fix tgt #2 in active target list
   iArg[2] = m_pTgtsDlg->GetActiveTrackTgt();                           // index of cursor tracking tgt in list

   return( m_pRuntime->SendCommand( dwCmd, &(iArg[0]), NULL, 3,0,0,0 ) );
}


//=== UpdateActiveTarget ==============================================================================================
//
//    Upload the entire active target list to CXDRIVER, or update the on/off state, window position, and pattern motion
//    velocity vector of a specific target within the active list.
//
//    The CX_CM_UPDACVTGT command is used to update a target in the current active list.  Uploading the entire active
//    list involves two tasks:  writing the target definitions into CXIPC, then issuing the CX_CM_UPDACVTGT command
//    with the on/off state, window position, and pattern velocity vector of all defined targets.  Format of the 
//    CX_CM_UPDACVTGT command is described in CXIPC.H.
//
//    BACKGROUND:  The "active target list" is a short list of targets that the user can manually control during 
//    Continuous mode. Active targets are generally used as fixation targets, particularly during calibration of the 
//    subject's eye position. One more recently introduced usage is the "tracking" target -- a specially designated 
//    active target that follows the mouse cursor whenever it is inside the eye-target position window. This feature 
//    helps users quickly assess the response properties of a unit that is being monitored. For technical reasons, one 
//    cannot change the COMPOSITION of this list  when the system is "active" -- ie, fixation or recording is ON or a 
//    stimulus run is in progress.  Thus, UpdateActiveTarget() can be invoked with iTgt==-1 only when the system is NOT 
//    active.
//
//    When the entire active target list is uploaded, this method may block up to 1 second (give some time for
//    complex RMVideo targets to load).
//
//    ARGS:       iTgt  -- [in] index of active target to be updated; if -1, entire active list is uploaded.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (illegal parameter value, or CXDRIVER not responding).
//
BOOL CCxContMode::UpdateActiveTarget( const int iTgt )
{
   ASSERT( m_pRuntime && m_pRuntime->GetMode() == CCxRuntime::ContMode );
   ASSERT( iTgt >= -1 && iTgt < m_pTgtsDlg->GetNumActiveTgts() );

   if( iTgt == -1 && IsActive() )                                    // cannot change COMPOSITION of active target list
   {                                                                 // while system is active
      TRACE0("\nTried to change active tgt list while system active!" );
      ASSERT( FALSE );
      return( FALSE );
   }

   int nActive = m_pTgtsDlg->GetNumActiveTgts();                     // # of active targets (could be zero -- which
                                                                     // will clear the active tgt list!)
   if( iTgt == -1 )                                                  // upload active target defns to CXDRIVER
   {
      WORD wKeys[MAX_ACTIVETGTS];
      for( int i = 0; i < nActive; i++ )
         wKeys[i] = m_pTgtsDlg->GetAcvTgtKey( i );

      if( !m_pRuntime->LoadActiveTargets( nActive, &(wKeys[0]) ) )
      {
         TRACE0("\nUnable to upload active tgt list!" );
         return( FALSE );
      }
   }

   int iArg[MAX_ACTIVETGTS + 1];                                     // prepare arguments for CX_CM_UPDACVTGT command
   int niArgs;
   float fArg[4*MAX_ACTIVETGTS];
   int nfArgs;
   if( iTgt == -1 )                                                  // CASE 1: init entire active target list
   {
      iArg[0] = -1;
      for( int i = 0; i < nActive; i++ )
      {
         iArg[i+1] = m_pTgtsDlg->GetAcvTgtOn( i ) ? 1 : 0;
         fArg[4*i] = float( m_pTgtsDlg->GetAcvTgtHPos(i) );
         fArg[4*i+1] = float( m_pTgtsDlg->GetAcvTgtVPos(i) );
         fArg[4*i+2] = float( m_pTgtsDlg->GetAcvTgtPatSpeed(i) );
         fArg[4*i+3] = float( m_pTgtsDlg->GetAcvTgtPatDir(i) );
      }
      niArgs = nActive + 1;
      nfArgs = 4*nActive;
   }
   else                                                              // CASE 2: update an existing active target
   {
      iArg[0] = iTgt;
      iArg[1] = m_pTgtsDlg->GetAcvTgtOn( iTgt ) ? 1 : 0;
      fArg[0] = float( m_pTgtsDlg->GetAcvTgtHPos(iTgt) );
      fArg[1] = float( m_pTgtsDlg->GetAcvTgtVPos(iTgt) );
      fArg[2] = float( m_pTgtsDlg->GetAcvTgtPatSpeed(iTgt) );
      fArg[3] = float( m_pTgtsDlg->GetAcvTgtPatDir(iTgt) );
      niArgs = 2;
      nfArgs = 4;
   }

   DWORD dwCmd = CX_CM_UPDACVTGT;                                    // send the command
   return( m_pRuntime->SendCommand( dwCmd, &(iArg[0]), &(fArg[0]), niArgs, nfArgs, 0, 0, (iTgt==-1) ? 1000 : 50 ) );
}

//=== ToggleCursorTrackingTarget ======================================================================================
//
//    If a target in the active target list is currently designated as the "cursor tracking" target, then toggle its
//    ON/off state.  Else, do nothing
//
//    ARGS:       NONE.
//    RETURNS:    True if successful; false if an error occurred OR if there is no active target currently designated
//       as the "cursor tracking" target.
//
BOOL CCxContMode::ToggleCursorTrackingTarget()
{
   int iTrackTgt = m_pTgtsDlg->GetActiveTrackTgt();                        // do nothing if there is no cursor tracking
   if( iTrackTgt < 0 ) return(FALSE);                                      // target in the active target list

   BOOL bOnFlag = !m_pTgtsDlg->GetAcvTgtOn(iTrackTgt);                     // toggle the ON/off state in the GUI
   m_pTgtsDlg->SetAcvTgtOn(iTrackTgt, bOnFlag);
   BOOL bOk = UpdateActiveTarget(iTrackTgt);                               // tell MaestroDRIVER to update actual tgt
   if( !bOk )                                                              // on failure, restore GUI
      m_pTgtsDlg->SetAcvTgtOn(iTrackTgt, !bOnFlag);

   return( bOk );
}

//=== HandleTrackingTargetPatternUpdate =============================================================================== 
//
//    Handler for global keyboard shortcuts that increment/decrement the current pattern speed or direction of the 
//    "cursor tracking" target. If no target in the active list is designated as the "Track" tgt, the method has no 
//    effect. 
//
//    ARGS:       nID -- command ID for the keyboard shortcut: ID_CM_TRKSPEEDUP, _TRKSPEEDDN, _TRKDIRUP, _TRKDIRDN.
//    RETURNS:    NONE.
//
VOID CCxContMode::HandleTrackingTargetPatternUpdate(UINT nID)
{
   // get index of tracking target. If there is none, then do nothing.
   int iTrackTgt = m_pTgtsDlg->GetActiveTrackTgt(); 
   if( iTrackTgt < 0 ) return; 

   // incr/decr the relevant parameter IAW command ID
   BOOL bUpdate = FALSE;
   double dOld = 0.0;
   if(nID == ID_CM_TRKSPEEDUP || nID == ID_CM_TRKSPEEDDN)
   {
      dOld = m_pTgtsDlg->GetAcvTgtPatSpeed(iTrackTgt);
      double dChange = ((nID==ID_CM_TRKSPEEDUP) ? 1.0 :-1.0) * CCxContFixTgtsDlg::INC_ACVTGTPATSPEED;
      m_pTgtsDlg->SetAcvTgtPatSpeed(iTrackTgt, dOld + dChange);
      bUpdate = BOOL(dOld != m_pTgtsDlg->GetAcvTgtPatSpeed(iTrackTgt));
   }
   else if(nID == ID_CM_TRKDIRUP || nID == ID_CM_TRKDIRDN)
   {
      dOld = m_pTgtsDlg->GetAcvTgtPatDir(iTrackTgt);
      double dChange = ((nID==ID_CM_TRKDIRUP) ? 1.0 : -1.0) * CCxContFixTgtsDlg::INC_ACVTGTPATDIR;
      m_pTgtsDlg->SetAcvTgtPatDir(iTrackTgt, dOld + dChange);
      bUpdate = BOOL(dOld != m_pTgtsDlg->GetAcvTgtPatDir(iTrackTgt));
   }

   // if a change was made, tell MaestroDRIVER to update the track tgt accordingly. On failure, restore GUI.
   if(bUpdate)
   {
      BOOL bOk = UpdateActiveTarget(iTrackTgt); 
      if( !bOk ) 
      {
         if(nID==ID_CM_TRKSPEEDUP || nID==ID_CM_TRKSPEEDDN)
            m_pTgtsDlg->SetAcvTgtPatSpeed(iTrackTgt, dOld);
         else
            m_pTgtsDlg->SetAcvTgtPatDir(iTrackTgt, dOld);
      }
   }

}

//=== ChangeTraces ====================================================================================================
//
//    If the user changes the channel configuration for ContMode (on the CCxContProtoDlg), update the data trace
//    facility accordingly.  The data trace facility runs continually in ContMode.  This is the only way to change its
//    contents.  The width of the traces are fixed at 5000 samples (10sec w/ a 2ms sample interval).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::ChangeTraces()
{
   if( IsStimRunning() || IsRecording() ) return;           // cannot mess with data trace facility right now!

   WORD wKey = m_pProtoDlg->GetChanCfg();                   // if the chan cfg has changed, update data traces
   if( wKey != GetTraces() ) SetTraces( wKey, 5000 );
}


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== UpdateCursorTrackingTarget ======================================================================================
//
//    If the active target list (CCxContFixTgtsDlg) designates a "tracking target" that is currently on, we make that
//    target follow the position of the mouse cursor whenever the cursor is in the client area of Maestro's eye-target
//    position plot.
//
//    The method is invoked by Service(), but it will only update the target's position every ~20ms at best.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContMode::UpdateCursorTrackingTarget()
{
   int iTrackTgt = m_pTgtsDlg->GetActiveTrackTgt();                        // do nothing if there is no cursor tracking
   if( iTrackTgt < 0 ) return;                                             // target turned on in the active tgt list
   if( !m_pTgtsDlg->GetAcvTgtOn(iTrackTgt) ) return;

   if( m_trackUpdTime.Get() > 20000.0 )                                    // update tgt pos every ~20ms
   {
      m_trackUpdTime.Reset();                                              //    reset update intv timer

      float fxTrack = 0;                                                   //    do nothing if cursor is not inside the
      float fyTrack = 0;                                                   //    position plot
      if( !m_pRuntime->GetCursorInPositionPlot(fxTrack, fyTrack) )
         return;

      m_pTgtsDlg->SetAcvTgtPos(iTrackTgt, double(fxTrack), double(fyTrack) );
      UpdateActiveTarget( iTrackTgt );
   }
}


//=== Start/StopStimulusRun ===========================================================================================
//
//    Issue CXDRIVER command to start or stop a stimulus run.  When stopping a stimulus run, it is possible to abort
//    the run immediately, or have it stop at the end of the current duty cycle ("soft-stop").  In the latter case, we
//    do not wait for the run to stop before returning.
//
//    ARGS:       bStopNow -- [in] if TRUE, stim run is aborted; else "soft-stop".
//                bStopRec -- [in] ignored if we're NOT aborting stim run; if we are, set TRUE to also stop recording;
//                            in this case, the recorded data is discarded.
//
//    RETURNS:    TRUE if successful; FALSE usually indicates catastrophic failure w/ CXDRIVER.
//
BOOL CCxContMode::StartStimulusRun()
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CCxRuntime::ContMode) );
   if( IsStimRunning() ) return( TRUE );                                      // run already in progress!

   BOOL bOk = m_pRuntime->StartStimulusRun();

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();                         // refresh internal copy of op state and
   Refresh();                                                                 // refresh appearance of panel dialogs
   return( bOk );
}

BOOL CCxContMode::StopStimulusRun( BOOL bStopNow, BOOL bStopRec )
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CCxRuntime::ContMode) );

   if( !IsStimRunning() ) return( TRUE );                                     // run is already stopped!

   BOOL bCmdOK = m_pRuntime->StopStimulusRun( bStopNow, bStopRec );

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();                         // refresh internal copy of op state and
   Refresh();                                                                 // refresh appearance of panel dialogs
   return( bCmdOK );
}


//=== Start/StopRecord ================================================================================================
//
//    Issue CXDRIVER command to start or stop data recording.  When we start recording, we must specify (among other
//    things) the data file for the recorded data, which are written to the file "on the fly" in ContMode.  When we
//    stop recording, we can choose to keep the recorded data file or discard it (eg, if a runtime error occurred).
//
//    ARGS:       bSave -- [in] if TRUE, recorded data file is discarded.
//
//    RETURNS:    TRUE if successful; FALSE usually indicates catastrophic failure w/ CXDRIVER.
//
BOOL CCxContMode::StartRecord()
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CCxRuntime::ContMode) );
   if( IsRecording() ) return( TRUE );                                     // we're already recording

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();

   CString strPath;                                                        // data file pathname
   m_pProtoDlg->GetNextDataFile( strPath );
   if( !pApp->GetShadowFile( strPath, m_strShadowPath ) )                  // if shadowing necessary but we cannot get
      return( FALSE );                                                     // a shadow path, then abort

   WORD wRun = CX_NULLOBJ_KEY;                                             // associated stimulus run, if any
   if( m_pProtoDlg->GetRunMode() != CCxContMode::MANUAL )
      wRun = m_pProtoDlg->GetCurrentRun();

   BOOL bSpikes = m_pProtoDlg->IsSaveSpikes();                             // save high-res spike trace
   WORD wChan = m_pProtoDlg->GetChanCfg();                                 // recorded/displayed channel cfg

   BOOL bOk = m_pRuntime->StartRecord(                                     // start recording
      m_strShadowPath.IsEmpty() ? strPath : m_strShadowPath,
      bSpikes, wRun, wChan );

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();                      // refresh internal copy of op state and
   Refresh();                                                              // refresh appearance of panel dialogs
   return( bOk );
}

BOOL CCxContMode::StopRecord( BOOL bSave )
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CCxRuntime::ContMode) );
   if( !IsRecording() ) return( TRUE );                                    // recording is already stopped

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();

   BOOL bSaveWasOK = bSave;                                                // issue cmd to stop recording & optionally
   BOOL bCmdOK = m_pRuntime->StopRecord( bSaveWasOK );                     // save the data file

   if( bSave && !bCmdOK )                                                  // if cmd failed, is CXDRIVER blocked trying
   {                                                                       // to save data file?  This is primarily for
      DWORD dwState = m_pRuntime->GetProtocolStatus();                     // test/debug purposes.
      if( (dwState & CX_FC_SAVING) != 0 )
         pApp->LogMessage( "WARNING: CXDRIVER blocked on file save!" );
   }

   if( bCmdOK && bSave && bSaveWasOK )                                     // if data file successfully saved:
   {
      if( !m_strShadowPath.IsEmpty() )                                     // if file was shadowed to local disk, we
      {                                                                    // need to copy it to user-specified path
         CString strDest;
         m_pProtoDlg->GetNextDataFile( strDest );
         bCmdOK = pApp->MoveShadowFile(strDest, m_strShadowPath);
         m_strShadowPath.Empty();
      }

      if( bCmdOK )                                                         // increment the data file extension
         m_pProtoDlg->IncrementNextDataFile();                             // (unless we failed to move shadow file)
   }

   m_dwLastOpState = m_pRuntime->GetProtocolStatus();                      // refresh internal copy of op state and
   Refresh();                                                              // refresh appearance of panel dialogs
   return( bCmdOK );
}

