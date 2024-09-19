//=====================================================================================================================
//
// cxcontrolpanel.cpp : Implementation of CCxControlPanel, the CNTRLX master mode control panel.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// In each "operational mode" of CNTRLX, the user must interact with a variety of different controls to manipulate the
// runtime behavior of CNTRLX and CXDRIVER in that mode.  Some controls may be shared across several operational modes,
// but most are unique to a given mode.  Functionally related controls should be grouped together in a single dialog.
// Control sets irrelevant to a particular op mode should not be accessible to the user while CNTRLX is in that mode.
//
// CCxControlPanel and supporting classes implement a "mode control panel" framework addressing these considerations.
// CCxControlPanel itself is the CNTRLX master mode control panel, a "tabbed" dialog bar container offering a space-
// efficient arrangement of multiple dialogs containing sets of related controls (aka, a property sheet).  All dialogs
// in the mode control panel are derived from CCxControlPanelDlg, an abstract base class which encapsulates
// functionality common to all mode control dialogs.  While CCxControlPanel serves as a GUI container for these
// dialogs, it delegates mode-specific functions to "mode controller" objects -- one for each  CNTRLX op mode, derived
// from the abstract base class CCxModeControl.  While CNTRLX is in a given op mode, the associated "mode controller"
// manages the relevant dialogs and manipulates the runtime state of CNTRLX/CXDRIVER IAW the operator's interactions
// with dialog controls.  CCxControlPanel itself is responsible for handling operational mode switches (by changing the
// "current" mode controller!).
//
// To switch operational modes, CCxControlPanel must invoke methods on the CNTRLX runtime interface, CCxRuntime.  The
// individual mode controller objects invoke many CCxRuntime methods to obtain runtime information, to send commands to
// CXDRIVER, or to change the current runtime state in some fashion.  CCxRuntime, in turn, calls one of several
// CCxControlPanel methods to update the mode control panel framework:
//
//       Service()                  => Called very often to update runtime state & GUI in the current op mode.
//       CanUpdateVideoCfg()        => Returns TRUE when current state permits changing video display config.
//       CanUpdateFixRewSettings()  => Returns TRUE when current state permits changing fixation/reward settings.
//
// In each case, CCxControlPanel merely invokes the like-named CCxModeControl method on the mode controller object that
// is currently active.
//
// Observe that CCxControlPanel "isolates" CCxRuntime from the rest of the mode control panel framework.  Similarly,
// the individual mode controller objects isolate CCxControlPanel from the mode-specific details of the dialogs it
// contains.  CCxControlPanel does not create its embedded dialogs directly.  Instead, it constructs each of the mode
// controller objects, passing a pointer to itself in the constructor so that the mode controllers have access to it.
// It then calls CCxModeControl::InitDlgs() on each mode controller -- so the mode controllers are responsible for
// installing the mode control dialogs they need.  See CCxControlPanel::OnCreate().  Furthermore, when we switch
// operational modes, the set of dialogs relevant to the new op mode will change.  While CCxControlPanel provides the
// ability to dynamically change the subset of installed dialogs that are "accessible", the active mode controller
// object is responsible for making those changes when a mode switch occurs.  See CCxControlPanel::SwitchMode(),
// CCxModeControl::Enter(), CCxModeControl::Exit().
//
// Conversely, the mode controller objects largely isolate the individual dialogs from CCxControlPanel and CCxRuntime.
// The dialogs have no direct access to CCxRuntime; by design, they do all mode-related work by invoking methods on the
// current mode controller, which they can access via CCxControlPanelDlg::GetModeCtrl().
//
// ==> "Tabbed Dialog Bar" Framework.
// CCxControlPanel and CCxControlPanelDlg are built, respectively, upon the classes CSizingTabDlgBar and CSzDlgBarDlg,
// which we developed as an extension to Cristi Posea's resizable control bar (see CREDITS).  This framework hides the
// mundane details of housing a "property sheet"-like GUI within a resizable, dockable control bar.  In addition to
// control bar and tab control functionality, the framework handles dialog creation within the parent (tabbed) dialog
// bar, takes care of routing commands and background UI updates (ON_UPDATE_COMMAND_UI) to the "active" dialog, and
// displays scroll bars when the dialog bar's available "display area" is smaller than the dialog template size (unlike
// MFC form views, dialogs do NOT have built-in scrolling support).
//
// ==> Handling the control bar's "hide button".
// The resizable control bar framework provides for a "hide" button (an "X" in a small box) in the non-client area of
// the bar.  (The button is implemented by the ancestor class CSizingControlBarG, and cannot be optionally hidden,
// though that may be possible in a future release of the CSizingControlBar framework.)  When the user presses the
// button, the bar is hidden.  However, the CNTRLX mode control panel CCxControlPanel must ALWAYS be visible whenever
// we're in an operational mode other than IdleMode.  Thus, since we cannot hide the "hide" button (without a messy
// rewrite of the CSizingControlBar framework), we choose to interpret the user's pressing the button as a short-cut
// for returning to IdleMode (in which it is OK to hide the mode control panel).  CCxControlPanel::OnNcLButtonUp()
// handles this situation.
//
//       !!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//       !! CCxControlPanel can only handle the hide-button functionality if it receives the WM_NCLBUTTONUP
//       !! message.  This is not a problem when the dialog bar is docked.  When it is floating, however,
//       !! things are tricky.  Standard floating control bars are housed in a CMiniDockFrameWnd that
//       !! implements and handles its own "hide-button" in a different manner not involving WM_NCLBUTTONUP.
//       !! So, in order to capture the hide-button press event when the bar is floating, we MUST use
//       !! the custom miniframe class provided with the CSizingControlBar framework.  This custom class,
//       !! CSCBMiniDockFrameWnd, will hide its caption bar and "real" hide button, letting the contained
//       !! CSizingControlBar-derivative paint its own caption bar and "fake" hide button.  Regrettably,
//       !! these features are controlled at compile time.  Be sure to #define _SCB_REPLACE_MINIFRAME and
//       !! do NOT #define _SCB_MINIFRAME_CAPTION.  See CREDITS.
//       !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//
// CREDITS:
// 1) CCxControlPanel is derived from the resizable dockable dialog bar CSizingTabDlgBar, which was built upon the MFC
// extension CSizingControlBarCF, by Cristi Posea.  See CSizingTabDlgBar implementation file for full credits.  All
// dialogs housed in a CSizingTabDlgBar must be derived from CSzDlgBarDlg, the base class of CCxControlPanelDlg.
//
//
// REVISION HISTORY:
// 31may2001-- Created.  Based on earlier development of the "test & calibration mode" control panel.
//          -- Added OnShowWindow() override to "register/unregister" with the CCxDriverIO object...
// 06jun2001-- Redesign.  The OnShowWindow() technique for registering did not work:  the dialog never got the
//             WM_SHOWWINDOW message -- perhaps because it only gets sent to the parent dialog bar?  New strategy is
//             to implement base classes CCxCtrlPanel and CCxTabCtrlPanel, whose OnActivatePanel() method invokes
//             CCxControlPanelDlg::Activate() to alert the dialog of the change in status (either "activating" or
//             "deactivating").  This method, in turn, invokes CCxDriverIO::SetActiveCtrlDlg().  The OnActivatePanel()
//             method is called by our mainframe class, which ultimately handles hiding & showing the control panels
//             when a mode switch occurs.
// 20jun2001-- Mods IAW redesign of "mode control" architecture.  A control panel dialog is now linked with a "mode
//             controller" object, rather than the CXDRIVER interface object CCxDriverIO.
// 28sep2001-- Mod to route CNTRLX doc/view update hints (CCxViewHint) to the document-aware control panels.
// 05oct2001-- Major architectural mod:  ABSTRACT class CCxControlPanel replaces CCxCtrlPanel/CCxTabCtrlPanel and
//             absorbs the functionality of CCxModeCtrl.  The CNTRLX mode control panels, built upon CCxControlPanel,
//             now serve the dual purpose of mode controller and tabbed dialog container for the CCxControlPanelDlg-
//             based dialogs that manage the controls required for each CNTRLX operational mode.
// 13nov2001-- Changes in how the MFC doc/view update mechanism is wired into the CCxControlPanel framework...
// 15nov2001-- Some comments on the Refresh() mechanism.  CCxControlPanel::Refresh() now refreshes all embedded dlgs,
//             not just the "active" one.
// 11feb2002-- Added pure virtual CanUpdateVideoCfg() and non-virtual UpdateVideoCfg() -- in support of sending the
//             current video display configuration to the hardware.
// 21feb2002-- During idle processing, the framework will automatically disable PBs for which no ON_UPDATE_COMMAND_UI
//             handler is provided.  CCxControlPanel::OnUpdateCmdUI() override prevents this, since CNTRLX control
//             panels will use the Refresh() mechanism to update enable/disable states of ctrls on the panels.
// 31jul2002-- Replaced ::GetTickCount()-based version of CElapsedTime with the more accurate RTX-based version used
//             on the CXDRIVER side.
// 18oct2002-- Added pure virtual CanUpdateFixRewSettings() and non-virtual UpdateFixRewSettings() -- in support of
//             sending current fixation/reward settings to CXDRIVER via CCxRuntime.
//          -- Also added Get/ResetNumRewardsDelivered(), for accessing/resetting CXDRIVER's "#rewards delivered"
//             counter via the CCxRuntime object.
// 27jan2003-- Modified SendUpdate() so that it can send a view hint without setting the document's "modified" flag.
//             This is useful for sending the "display object" view hint (CX_DSPOBJ), for example.
// 07apr2003-- MAJOR redesign of CCxControlPanel.  CCxControlPanel is no longer an abstract base class for multiple
//             CNTRLX mode control panels.  Instead, it is THE control panel for CNTRLX.  Mode-specific functionality
//             is now handled by individual "mode controller" objects, derived from the abstract base class
//             CCxModeControl.  CCxControlPanel creates a mode controller for each of the CNTRLX operational modes,
//             and serves as the container for the mode-control dialogs with which those mode controllers interact.
//             CCxControlPanel does not have specific "knowledge" of the individual dialogs it contains -- it relies on
//             the mode controller objects to create these dialogs and manipulate them as needed.  The individual
//             mode control panel dialogs are still based on the abstract CCxControlPanelDlg class, which is now
//             implemented in a separate file.
// 17jun2005-- Added function ToggleFixInContinuousMode().
// 13mar2006-- ToggleFixInContinuousMode() replaced by HandleCMShortcut(), which handles one of two possible keyboard
//             accelerators relevant only in Continuous mode: ID_CM_TOGGLEFIX and the new ID_CM_TOGGLETRACK.
// 01dec2006-- HandleCMShortcut() replaced by HandleGlobalModeShortcut(), which now handles one of four possible 
//             opmode-related shortcuts forwarded by CCxMainFrame: ID_CM_TOGGLEFIX and ID_CM_TOGGLETRACK in Continuous 
//              mode; ID_TM_TOGGLESTART and ID_TM_TOGGLEPAUSE in Trial mode.
// 11dec2006-- HandleGlobalModeShortcut() modified to handle four additional keyboard shortcuts forwarded by 
//             CCxMainFrame: ID_CM_TRKSPEEDUP/DN, ID_CM_TRKDIRUP/DN.
//=====================================================================================================================


#include "stdafx.h"                 // standard MFC stuff
#include "cntrlx.h"                 // CCntrlxApp and resource IDs for CNTRLX
#include "util.h"                   // for RTX-based CElapsedTime

#include "cxdoc.h"                  // CCxDoc -- the CNTRLX document
#include "cxviewhint.h"             // CCxViewHint -- "hint" class for CNTRLX-specific doc/view updates
#include "cxobj_ifc.h"              // CNTRLX object interface:  common constants and other defines
#include "cxruntime.h"              // CCxRuntime -- the IPC interface to CXDRIVER

#include "cxmodecontrol.h"          // CCxModeControl -- base class for runtime mode controller objects
#include "cxidlemode.h"             // CCxIdleMode -- runtime mode controller during IdleMode
#include "cxtrialmode.h"            // CCxTrialMode -- runtime mode controller during TrialMode
#include "cxcontmode.h"             // CCxContMode -- runtime mode controller during ContMode
#include "cxtestmode.h"             // CCxTestMode -- runtime mode controller during TestMode

#include "cxcontrolpanel.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC( CCxControlPanel, CSizingTabDlgBar )

BEGIN_MESSAGE_MAP( CCxControlPanel, CSizingTabDlgBar )
   ON_WM_CREATE()
   ON_WM_NCLBUTTONUP()
END_MESSAGE_MAP()


//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

LPCTSTR CCxControlPanel::EMSG_MODESWITCHFAILED        = _T("(!) CXDRIVER failed during mode switch!");
LPCTSTR CCxControlPanel::EMSG_OPMODEDISABLED          = _T("(!) That runtime op mode is not available!");



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxControlPanel [constructor], ~CCxControlPanel [destructor] ====================================================
//
//    Since we create the mode controller helper objects in OnCreate(), we destroy them when control panel is deleted.
//
CCxControlPanel::CCxControlPanel()
{
   m_pIdleMode = NULL;                             // mode controllers are constructed during OnCreate()
   m_pTrialMode = NULL;
   m_pContMode = NULL;
   m_pTestMode = NULL;
   m_pNullMode = NULL;

   m_pCurrMode = NULL;
}

CCxControlPanel::~CCxControlPanel()
{
   if( m_pIdleMode ) delete m_pIdleMode;
   if( m_pTrialMode ) delete m_pTrialMode;
   if( m_pContMode ) delete m_pContMode;
   if( m_pTestMode ) delete m_pTestMode;
   if( m_pNullMode ) delete m_pNullMode;
}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnCreate [base override] ========================================================================================
//
//    Response to WM_CREATE message.
//
//    After the base class method does its thing, we attempt to create each of the mode controller objects and have
//    them install any mode control dialogs they require.
//
//    !!!IMPORTANT!!! Implicit in this design is that the CCxRuntime object already exists when the mode control panel
//    is created.  Else the creation process will fail!
//
//    ARGS:       lpcs  -- [in] creation parameters
//
//    RETURNS:    -1 to indicate failure; 0 to let creation proceed.
//
int CCxControlPanel::OnCreate( LPCREATESTRUCT lpcs )
{
   if( CSizingTabDlgBar::OnCreate( lpcs ) < 0 ) return( -1 );
   CCxRuntime* pRuntime = ((CCntrlxApp*)AfxGetApp())->GetRuntime();
   if( pRuntime == NULL ) return( -1 );

   ASSERT( pRuntime->GetMode() <= CCxRuntime::IdleMode );         // CXDRIVER must be in IdleMode or not running

   m_pIdleMode = new CCxIdleMode( this );                         // construct mode controllers; abort on failure...
   if( m_pIdleMode == NULL ) return( -1 );
   m_pTrialMode = new CCxTrialMode( this );
   if( m_pTrialMode == NULL ) return( -1 );
   m_pContMode = new CCxContMode( this );
   if( m_pContMode == NULL ) return( -1 );
   m_pTestMode = new CCxTestMode( this );
   if( m_pTestMode == NULL ) return( -1 );
   m_pNullMode = new CCxNullMode( this );                         // this represents the "CXDRIVER not running" mode!
   if( m_pNullMode == NULL ) return( -1 );

   if( pRuntime->GetMode() == CCxRuntime::IdleMode )              // we always start out in IdleMode if CXDRIVER is
      m_pCurrMode = m_pIdleMode;                                  // actually running!
   else m_pCurrMode = m_pNullMode;

   if( !(m_pIdleMode->InitDlgs() && m_pTrialMode->InitDlgs() &&   // let mode controllers install required dialogs;
         m_pContMode->InitDlgs() && m_pTestMode->InitDlgs()) )    // abort on failure...
      return( -1 );

   SetWindowText( m_pCurrMode->GetModeTitle() );                  // set control panel title to reflect current mode
   m_pCurrMode->Enter();                                          // do any inits upon entering the mode
   return( 0 );
}


//=== OnNcLButtonUp [base override] ===================================================================================
//
//    Response to the WM_NCLBUTTONUP message -- mouse-button up in the non-client area.
//
//    If the user pressed the hide button on the gripper, the base class handler will hide the bar.  We want to make
//    the hide button press a "short-cut" for a return to IdleMode, so we send the ID_MODE_IDLE command to the main
//    frame window in response to the HTCLOSE hit test code.  Any other hit test codes are passed to the base class.
//
//    ARGS:       nHitTest -- [in] hit-test code.
//                pt       -- [in] mouse pos at button-up, in screen coords.
//
//    RETURNS:    NONE
//
void CCxControlPanel::OnNcLButtonUp( UINT nHitTest, CPoint pt )
{
   if( nHitTest == HTCLOSE ) AfxGetMainWnd()->SendMessage( WM_COMMAND, (WPARAM)ID_MODE_IDLE, (LPARAM)0 );
   else CSizingTabDlgBar::OnNcLButtonUp( nHitTest, pt );
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnUpdate ========================================================================================================
//
//    Mechanism for forwarding CNTRLX doc/view updates to embedded control panel dialogs -- which may display CCxDoc
//    document data.  Also, if it is possible that the channel configuration (CX_CHANCFG) object assoc. w/the data
//    trace facility in the current op mode has been changed or deleted, we update the data trace display.
//
//    ARGS:       pHint -- [in] the CNTRLX doc/view update hint.  When the hint is NULL, it is assumed that a new
//                         CCxDoc has just been created/opened (analogous to CView::OnInitialUpdate()).
//
//    RETURNS:    NONE.
//
VOID CCxControlPanel::OnUpdate( CCxViewHint* pHint )
{
   CCxRuntime* pRuntime = ((CCntrlxApp*)AfxGetApp())->GetRuntime();
   ASSERT( pRuntime );
   if( pRuntime->GetMode() > CCxRuntime::IdleMode )
   {
      WORD wKey = m_pCurrMode->GetTraces();
      if( wKey != CX_NULLOBJ_KEY )
      {
         if( pHint == NULL )
            m_pCurrMode->SetTraces( CX_NULLOBJ_KEY, 1000 );
         else if( (pHint->m_code == CXVH_CLRUSR) ||
                  ((pHint->m_code == CXVH_DELOBJ) && (pHint->m_key == CX_NULLOBJ_KEY || pHint->m_key == wKey)) ||
                  (pHint->m_code == CXVH_MODOBJ && pHint->m_key == wKey)
                )
            m_pCurrMode->OnChangeTraces();
      }
   }

   for( int i = 0; i < GetNumTabs(); i++ )
   {
      CCxControlPanelDlg* pDlg = (CCxControlPanelDlg*) GetDlg( i );
      if( pDlg != NULL ) pDlg->OnUpdate( pHint );
   }
}


//=== SwitchMode ======================================================================================================
//
//    Change the current CNTRLX operational mode.  Blocks up to 5 secs or until CXDRIVER completes swtich.
//
//    ARGS:       iMode -- [in] desired op mode
//
//    RETURNS:    TRUE if mode switch was successful; FALSE otherwise.
//
BOOL CCxControlPanel::SwitchMode( const int iOpMode )
{
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                        // for posting error message, if necessary
   CCxRuntime* pRuntime = pApp->GetRuntime();                           // access to CNTRLX/CXDRIVER runtime interface
   ASSERT( pRuntime );

   if( !pRuntime->IsOn() ) { m_pCurrMode= m_pNullMode; return(FALSE); } // CXDRIVER is not running!!

   if( (iOpMode < CCxRuntime::IdleMode) ||                              // invalid op mode
       (iOpMode > CCxRuntime::ContMode) )
   {
      TRACE0( "Unrecognized op mode -- ignored!\n" );
      return( FALSE );
   }

   if( pRuntime->GetMode() == iOpMode ) return( TRUE );                 // nothing to do: we're already in that mode!

   if( !pRuntime->IsModeEnabled( iOpMode ) )                            // CXDRIVER does not support this mode
   {
      pApp->LogMessage( EMSG_OPMODEDISABLED );
      return( FALSE );
   }

   CWaitCursor waitCursor;                                              // just in case this takes a while...

   BOOL bOk = m_pCurrMode->Exit();                                      // clean up prior to exiting current op mode

   if( bOk )                                                            // command CXDRIVER to switch to new op mode
   {
      int i = iOpMode;
      DWORD dwCmd = CX_SWITCHMODE;
      bOk = pRuntime->SendCommand( dwCmd, &i, NULL, 1, 0, 0, 0 );
      if(!bOk) { TRACE1("Mode switch command failed, command error %d\n", dwCmd); }
   }

   if( bOk )                                                            // wait up to 5 secs for mode switch to be
   {                                                                    // completed on the CXDRIVER side...
      CElapsedTime tWait;
      while( pRuntime->GetMode() != iOpMode )
      {
         ::Sleep( 10 );
         if( tWait.Get() > 5.0e+6 ) break;
      }
      bOk = BOOL(pRuntime->GetMode() == iOpMode );
      if(!bOk) { TRACE0("Timed out waiting for CXDRIVER to switch modes!\n"); }
   }

   if( bOk )                                                            // switch to mode ctrlr for the new op mode
   {
      if( iOpMode == CCxRuntime::IdleMode )        m_pCurrMode = m_pIdleMode;
      else if( iOpMode == CCxRuntime::TrialMode )  m_pCurrMode = m_pTrialMode;
      else if( iOpMode == CCxRuntime::ContMode )   m_pCurrMode = m_pContMode;
      else                                         m_pCurrMode = m_pTestMode;

      bOk = m_pCurrMode->Enter();                                       // and do inits upon entering the new mode
   }

   if( !bOk  )                                                          // if a mode switch fails, something is wrong.
   {                                                                    // we kill CXDRIVER and inform user.
      pApp->LogMessage( EMSG_MODESWITCHFAILED );                        // runtime interface and inform user.
      pRuntime->Stop( FALSE );
      m_pCurrMode = m_pNullMode;
   }
   else                                                                 // if mode switch succeeded, update title on
      SetWindowText( m_pCurrMode->GetModeTitle() );                     // control bar to reflect the current op mode

   return( bOk );
}


//=== Service, CanUpdateVideoCfg, CanUpdateFixRewSettings =============================================================
//
//    These methods, which are invoked by CCxRuntime, merely wrap similar calls to the current mode controller -- in
//    order to isolate CCxRuntime from the rest of the CNTRLX mode control panel framework.
//       Service() :  Service any pending transactions with CXDRIVER in the current op mode.
//       CanUpdateVideoCfg() : Return TRUE if video display configuration may be updated now; FALSE otherwise.
//       CanUpdateFixRewSettings() : Return TRUE if fixation/reward settings may be updated now; FALSE otherwise.
//
//    ARGS:       NONE.
//
//    RETURNS:    see above
//
VOID CCxControlPanel::Service()
{
   m_pCurrMode->Service();
}

BOOL CCxControlPanel::CanUpdateVideoCfg()
{
   return( m_pCurrMode->CanUpdateVideoCfg() );
}

BOOL CCxControlPanel::CanUpdateFixRewSettings()
{
   return( m_pCurrMode->CanUpdateFixRewSettings() );
}


//=== Refresh =========================================================================================================
//
//    Refresh appearance of all "visible" (ie, currently accessible via the tab control) dialogs in the mode control
//    panel.  Mode controllers will invoke this method to refresh the appearance of their associated dialogs.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxControlPanel::Refresh()
{
   for( int i = 0; i < GetNumTabs(); i++ )
   {
      CCxControlPanelDlg* pDlg = (CCxControlPanelDlg*) GetDlg( i );
      if( IsVisibleDlg( pDlg ) ) pDlg->Refresh();
   }
}


//=== GetModeCtrl =====================================================================================================
//
//    Retrieve ptr to one of the mode controller objects created by the master mode control panel.
//
//    ARGS:       iOpMode  -- [in] CNTRLX op mode governed by the mode controller.
//
//    RETURNS:    ptr to the mode controller requested, or NULL if op mode unrecognized.
//
CCxModeControl* CCxControlPanel::GetModeCtrl( int iOpMode )
{
   CCxModeControl* pMode = NULL;
   if( iOpMode == CCxRuntime::NullMode ) pMode = m_pNullMode;
   else if( iOpMode == CCxRuntime::IdleMode ) pMode = m_pIdleMode;
   else if( iOpMode == CCxRuntime::TrialMode ) pMode = m_pTrialMode;
   else if( iOpMode == CCxRuntime::ContMode ) pMode = m_pContMode;
   else if( iOpMode == CCxRuntime::TestMode ) pMode = m_pTestMode;

   return( pMode );
}


//=== HandleGlobalModeShortcut ======================================================================================== 
//
//    Handles keyboard accelerator shortcut commands, forwarded by the mainframe window CCxMainFrame, that are relevant 
//    to Continuous or Trial mode. In some cases, they provide an alternative to clicking the relevant button on the 
//    opmode's "Protocol" tab.
//       1) ID_CM_TOGGLEFIX (Ctrl + space) -- Toggle fixation ON/off in Cont mode.
//       2) ID_CM_TOGGLETRACK (F3 key) -- Toggle the ON/off state of the "cursor-tracking" target during Cont mode, if 
//       one is so designated in the active target list.
//       3) ID_TM_TOGGLESTART (F7 key) -- Toggle the running state (start/stop) of the trial sequencer in Trial mode.
//       4) ID_TM_TOGGLEPAUSE (F8 key) -- If the trial sequencer is currently running in Trial mode, toggle the 
//       sequencer's pause/resume state.
//       5) ID_CM_TRKSPEEDUP (F4 key) -- In Continuous mode, increment pattern speed of the active target designated 
//       as the "Track" target by a set amount.
//       6) ID_CM_TRKSPEEDDN (Shift + F4) -- In Continuous mode, decrement pattern speed of the active target 
//       designated as the "Track" target by a set amount.
//       7) ID_CM_TRKDIRUP (F5 key) -- In Continuous mode, increment the pattern direction of the active target 
//       designated as the "Track" target by a set amount.
//       7) ID_CM_TRKDIRDN (Shift + F5) -- In Continuous mode, increment the pattern direction of the active target 
//       designated as the "Track" target by a set amount.
//    The commands have no effect if Maestro is not in the relevant operational mode.
//
//    ARGS:       nID -- [int] ID of the keyboard accelerator command.  Should be one of the commands listed above.
//
//    RETURNS:    NONE.
//
VOID CCxControlPanel::HandleGlobalModeShortcut( UINT nID )
{
   if(m_pCurrMode == (CCxModeControl*) m_pContMode)
   {
      switch( nID )
      {
         case ID_CM_TOGGLEFIX :     m_pContMode->ToggleFixate(); break;
         case ID_CM_TOGGLETRACK :   m_pContMode->ToggleCursorTrackingTarget(); break;
         case ID_CM_TRKSPEEDUP : 
         case ID_CM_TRKSPEEDDN :
         case ID_CM_TRKDIRUP :
         case ID_CM_TRKDIRDN :      m_pContMode->HandleTrackingTargetPatternUpdate(nID); break;
      }
   }
   else if(m_pCurrMode == (CCxModeControl*) m_pTrialMode)
   {
      switch( nID )
      {
         case ID_TM_TOGGLESTART :
            if( !m_pTrialMode->IsSeqRunning() ) 
               m_pTrialMode->Go();
            else if( !m_pTrialMode->IsSeqStopping() )
               m_pTrialMode->Halt();
            break;
         case ID_TM_TOGGLEPAUSE :
            if( m_pTrialMode->IsSeqRunning() )
            {
               if( m_pTrialMode->IsSeqPaused() )
                  m_pTrialMode->Resume();
               else if( !m_pTrialMode->IsSeqPausing() )
                  m_pTrialMode->Pause();
            }
            break;
      }
   }

}
