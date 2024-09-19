//===================================================================================================================== 
//
// cxmainframe.cpp : Implementation of class CCxMainFrame, which encapsulates CNTRLX's main application window.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
//
// CREDITS:
// (1) The "Visual Framework" library, by Zoran M. Todorovic [09/30/2000, www.codeproject.com/dialog/visualfx.asp] --
// We use this library as is to implement the view framework that appears in the CCxMainFrame client area.  This 
// framework makes more complex GUI layouts -- that would have been a bear to implement w/core MFC -- easy to build.
// (2) Some of the code for persisting the mainframe window's rect and maximized/minimized state was taken from an
// article by JR Skinner (codeguru.earthweb.com/misc/restore_mult_monitor.shtml, 12/12/1998).
//
// REVISION HISTORY:
// 04feb2000-- Created.  Just a skeleton for now.  Setting up a standard format for .H, .CPP files...
//          -- Initial GUI is a tab window with two tabs:  a "Main" tab and a "Targets" tab, both bearing the 
//             nascent CCxTargForm form view.  This will allow me to check whether changing one view is properly 
//             updating another view of the same data!!
// 28apr2000-- Brought up-to-date with the current state of the GUI.  There is only one tab implemented thus far:  the 
//             "Targets" tab is a splitter window containing two form views:  CCxTgListForm manages the target list, 
//             and CCxTgDataForm manages user input to a set of target parameter controls.  The values in these 
//             controls correspond to the target which has the focus in the target list.
// 17may2000-- CCxTgListForm replaced by CCxTgListView (instead of a form view containing a single list control, we 
//             now use a CListView-derived view which encapsulates the list control -- the list control IS the entire 
//             view!).
// 25may2000-- Added "Trial" tab as a splitter window with new class CCxTrialTree on the left-hand side and, for now, 
//             a blank, dummy CListView on the right-hand side. 
// 25sep2000-- CCxTgListForm replaced by CCxTargTree -- the CNTRLX "target tree" view class.  CCxTgDataForm replaced by 
//             CCxTargForm.
// 03jan2001-- Dummy CListView in "Trial" tab replaced by a nested splitter containing CCxTrialHdrForm and 
//             CCxTrialSegView; these two views are used to display/modify a selected trial's definition.
// 13jan2001-- !!!MAJOR REWORK!!! Switching to SDI model.  CCxMainFrame, formerly called CExperimentFrame, now acts as 
//             the main app window.  It sets up and manages the toolbars and control bars used in CNTRLX, and uses the 
//             Visual Framework library to manage its client area.
// 01feb2001-- The client area is now much simplified.  Instead of having a tabbed window with separate tabs for 
//             "Targets", "Trials", etc., the client area is filled by a single 1x2 static splitter.  The left-hand 
//             pane is the CCxObjectTree view, a hierarchical tree view summarizing all data & collection objects 
//             currently defined in the CNTRLX experiment document.  The right-hand pane is a custom "composite" view 
//             class, CCxMultiObjectView.  This view automatically switches to the appropriate form view whenever a 
//             different type of CNTRLX data object is selected for display.  We continue to use a Visual Framework 
//             object to handle the client area layout, even though the client area is very simple.  We may eventually 
//             return to a more complex layout as we add features to CNTRLX. 
// 15mar2001-- Installed recently developed implementations for the "Data Traces", "Message Log", and "Eye/Tgt Pos" 
//             control bars.
// 22mar2001-- Composite view CCxMultiObjectView did not work properly.  Decided to return to a more complex GUI 
//             layout.  A tab window in the right-hand splitter pane contains one tab pane for each data object type 
//             defined in CNTRLX:  a "Target" tab, "Trial" tab, etc.  The user can switch among the tabs as desired; 
//             programmatic switches are handled via the view classes.
// 13apr2001-- Added LogMessage() method as interface to the message log control bar for posting GUI messages.
//          -- Added EnableRunModes(), which enables/disables the control bars dedicated to CNTRLX protocol runtime.
//             When CXDRIVER is unavailable, these controls should be unavailable to user.  Updated the code for 
//             showing/hiding these control bars accordingly...
// 23apr2001-
// 27apr2001-- Mods to introduce runtime "mode control" panels (for trial, continuous, and test&cal modes), separate 
//             from the output panels.  While all output panels can be displayed simultaneously, only one mode panel 
//             can be displayed at a time, as it reflects the current runtime operational mode.  If no mode panel is 
//             open, then the app is in "idle" mode.  Currently, only test&cal mode control panel is available.
// 01jun2001-- Got rid of RefreshTestPanel(), which was no longer required.
// 15jun2001-- Got rid of m_pDriver.  All operational runtime mode-related info needed by CCxMainFrame is obtained by 
//             invoking methods on the application object, CCntrlxApp.
// 28sep2001-- Added OnUpdate() to forward CNTRLX view update hints to document-aware control panels.
// 11oct2001-- Brought in line w/ latest architectural rework. 
// 24oct2001-- The two output panels are registered w/ CCxRuntime in OnCreate().  This completes the link-up between 
//             the runtime interface and the GUI elements that reflect runtime info or modify runtime behavior.
// 21feb2002-- Incorporated the mode control panel for TrialMode.
// 27feb2002-- Added a timer in an attempt to ensure that we call CCxRuntime::Service() when the app is not in its 
//             normal message loop -- in which case OnIdle() is never called.  E.g., this happens when user is 
//             manipulating menus, resizing or moving windows, or working with a modal dialog.  Using WM_TIMER is NOT 
//             an ideal solution.  It is a low priority message and thus will be delayed if higher priority messages 
//             are streaming into the message queue.  Also, it fails to be sent at all when the user holds the mouse 
//             down on the title bar of a top-level window (for moving the window's pos on screen).  A second thread 
//             would be a better solution, but will require a significant redesign....
// 03jun2002-- Added CCxContRunForm to tabbed window.  This form edits continuous-mode run definitions (CX_CONTRUN).
// 23sep2002-- Incorporated the mode control panel for ContMode.
// 17nov2002-- Added CCxPertForm to tabbed window.  This form displays/edits perturbation objects (CX_PERTURB).
// 08apr2003-- Mode control panel framework redesigned.  Multiple control panels replaced by a single master control 
//             panel (CCxControlPanel).  Modified CCxMainFrame accordingly...
// 10apr2003-- Adding support to load/save control bar state info from the registry.
// 17apr2003-- Modified to incorporate the new "Help" control bar, CCxHelpPanel, in place of CMessageLogBar.
// 18apr2003-- Decided to implement a separate "help" system later.  CCxHelpPanel is replace by CCxMsgLogBar.
// 29apr2003-- Added support for loading/saving mainframe window's SW_SHOWNORMAL rect and its show state.  Window will 
//             come up in normal or maximized state (but NOT minimized state) at app startup.
// 06may2003-- Added a "Mode" menu command (ID_MODE_RESTART) which allows user to restart CXDRIVER at any time.  The 
//             new command is handled by OnModePanel(), OnUpdateModePanel().
// 15mar2004-- Added icons to the tab panes that hold the different object forms.  Also changed the order of those tab 
//             panes.  See OnCreateClient().
// 21apr2005-- Added new display bar, CCxSpikeHistBar, that will construct and display spike time histograms for 
//             tagged sections culled from an ongoing trial sequence.
// 17jun2005-- Add handler for keyboard accelerator ID_CM_TOGGLEFIX.  This will toggle fixation ON/off if Maestro is in 
//             Continuous mode.  Otherwise, it has no effect.
// 13mar2006-- Added handler for keyboard accelerator ID_CM_TOGGLETRACK.  In Cont mode, this will toggle the ON/off 
//             state of the cursor-tracking target, if one is defined.  Otherwise, it has no effect.  Both 
//             ID_CM_TOGGLEFIX and _TOGGLETRACK are handled by OnCMGlobalShortcut(), which replaces OnCMToggleFix().
// 01dec2006-- Added two more keyboard shortcuts not associated with a menu item, this time for Trial mode: 
//             ID_TM_TOGGLESTART (VK_F7) starts or stops the trial sequencer, while ID_TM_TOGGLEPAUSE (VK_F8) pauses or 
//             resumes the sequencer if it is running. OnCMGlobalShortcut() is now called OnGlobalShortcut(). Note 
//             that [ID_CM_TOGGLEFIX...ID_TM_TOGGLEPAUSE] is a contigous range of integers identifying the four 
//             mode control-related shortcuts.
// 04dec2006-- Added function IsObjectSelected(), which returns whether or not a Maestro object (identified by key) is 
//             currently selected in the Maestro object tree (CCxObjectTree).
// 11dec2006-- Added additional global shortcuts for incr/decrementing pattern speed and direction for the designated 
//             "Track" target in the active list (Continuous mode only).
// 24nov2014-- Modified to prevent user from closing the frame window via the system menu or the close button in the
//             title bar. The close button cannot be removed, but it is now effectively disabled. Exiting the app via
//             this close button very often caused Maestro to crash and would require a reboot before the app would
//             startup normally.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX

#include "cxruntime.h"                       // CCxRuntime -- the "master" runtime controller handles all operational 
                                             // modes and all interactions w/CNTRLX hardware devices
#include "cxobjtree.h"                       // CCxObjectTree -- view class that displays the CNTRLX object tree
#include "cxtargform.h"                      // CCxTargForm -- form view for defining/displaying CNTRLX targets
#include "cxchannelform.h"                   // CCxChannelForm -- form view for defining/displaying CNTRLX chan cfgs
#include "cxpertform.h"                      // CCxPertForm -- form view for defining/displaying CNTRLX perturbations
#include "cxtrialform.h"                     // CCxTrialForm -- form view for defining/displaying CNTRLX trials
#include "cxcontrunform.h"                   // CCxContRunForm -- form view for defining/displaying CNTRLX run objects 

#include "cxmainframe.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxMainFrame, CFrameWnd )


BEGIN_MESSAGE_MAP( CCxMainFrame, CFrameWnd )
   ON_WM_CREATE()
   ON_WM_TIMER()
   ON_WM_DESTROY()
   ON_COMMAND_RANGE( ID_VIEW_PLOTPANEL, ID_VIEW_HISTPANEL, OnOutputPanel )
   ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_PLOTPANEL, ID_VIEW_HISTPANEL, OnUpdateOutputPanel )
   ON_COMMAND_RANGE( ID_MODE_IDLE, ID_MODE_RESTART, OnModePanel )
   ON_UPDATE_COMMAND_UI_RANGE( ID_MODE_IDLE, ID_MODE_RESTART, OnUpdateModePanel )
   ON_COMMAND_RANGE( ID_CM_TOGGLEFIX, ID_CM_TRKDIRDN, OnGlobalShortcut )
END_MESSAGE_MAP()



//===================================================================================================================== 
// STATIC CLASS MEMBER INITIALIZATION
//===================================================================================================================== 

UINT CCxMainFrame::STATUSINDICATORS[4] =                    // status bar indicators
{
   ID_SEPARATOR,
   ID_INDICATOR_CAPS,
   ID_INDICATOR_NUM,
   ID_INDICATOR_SCRL,
};

LPCTSTR CCxMainFrame::BAR_KEY = _T("CtrlBarState");         // control bar state registry key
LPCTSTR CCxMainFrame::WND_KEY = _T("WndPlacement");         // frame window's placement info key



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnCreate [base override] ======================================================================================== 
//
//    Overridden to handle custom creation code, namely, to add various control bars to the frame window:
//
//       Status bar ==> Simple, generic status indicators.  Nothing special here.
//       Main tool bar ==> Defined in toolbar resource IDR_MAINFRAME, this contains buttons for typical commands 
//          like File|New,Open; Edit|Cut,Copy,Paste; etc.  In addition, it includes buttons for the commands which 
//          toggle the visibility of the CNTRLX runtime control/output panels.  Resizable, floating or docking (to the 
//          top of client area only) enabled, with tooltips.
// 
//       CNTRLX runtime control/output panels ==> Docking control bars that are very important during experiment 
//          runtime.
//
//    ARGS:       lpcs  -- [in] contains window creation information. 
//
//    RETURNS:    -1 on failure; 0 on success.
//
int CCxMainFrame::OnCreate( LPCREATESTRUCT lpcs )
{
   if( CFrameWnd::OnCreate( lpcs ) == -1 ) return( -1 );                // let base class do its work

   // remove system "close" command from system menu, disabling the close button in title bar.
   CMenu* pSysMenu = GetSystemMenu(FALSE);
   ASSERT(pSysMenu != NULL);
   VERIFY(pSysMenu->RemoveMenu(SC_CLOSE, MF_BYCOMMAND));

   if( !m_wndStatusBar.Create(this) ||                                  // the status bar
       !m_wndStatusBar.SetIndicators( STATUSINDICATORS, NUMINDIC ) )
   {
      TRACE0( "Failed to create status bar\n" );
      return( -1 ); 
   }

   DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |    // the main tool bar
                   CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;
   if( !m_wndToolBar.CreateEx( this, TBSTYLE_FLAT, dwStyle ) ||         // (note: new in VC++6.0!)
       !m_wndToolBar.LoadToolBar( IDR_MAINFRAME ) )
   {
      TRACE0( "Failed to create main toolbar\n" );
      return( -1 ); 
   }


   if( !m_plotPanel.Create( _T("Eye-Target Position"),this,IDT_PLOT ) ) // the CNTRLX runtime control/output panels
   {                                                                    // as docking control bars...
      TRACE0( "Failed to create eye/tgt plot panel\n" );
      return( -1 );
   }
   if( !m_dataPanel.Create( _T("Data Traces"), this, IDT_DATA ) )
   {
      TRACE0( "Failed to create data traces panel\n" );
      return( -1 );
   }
   if( !m_logPanel.Create( _T("Message Log"), this, IDT_LOG ) )
   {
      TRACE0( "Failed to create message log panel\n" );
      return( -1 );
   }
   if( !m_modePanel.Create( _T("Idle Mode"), this, IDT_CONTROL ) )
   {
      TRACE0( "Failed to create the master mode ctrl panel\n" );
      return( -1 );
   }
   if( !m_histPanel.Create( _T("Spike Histograms"), this, IDT_HIST ) )
   {
      TRACE0( "Failed to create the spike histograms display panel\n" );
      return( -1 );
   }

   dwStyle = CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;
   m_plotPanel.SetBarStyle( m_plotPanel.GetBarStyle() | dwStyle );
   m_dataPanel.SetBarStyle( m_dataPanel.GetBarStyle() | dwStyle );
   m_logPanel.SetBarStyle( m_logPanel.GetBarStyle() | dwStyle );
   m_modePanel.SetBarStyle( m_modePanel.GetBarStyle() | dwStyle );
   m_histPanel.SetBarStyle( m_histPanel.GetBarStyle() | dwStyle );


   m_wndToolBar.EnableDocking( CBRS_ALIGN_TOP );                        // main toolbar must dock at top (or float) 
   m_plotPanel.EnableDocking( CBRS_ALIGN_ANY );                         // output/mode ctrl panels can dock anywhere 
   m_dataPanel.EnableDocking( CBRS_ALIGN_ANY );
   m_logPanel.EnableDocking( CBRS_ALIGN_ANY );
   m_modePanel.EnableDocking( CBRS_ALIGN_ANY );
   m_histPanel.EnableDocking( CBRS_ALIGN_ANY );

   EnableDocking( CBRS_ALIGN_ANY );                                     // allow docking on any side of frame window

#ifdef _SCB_REPLACE_MINIFRAME                                           // if defined, we use this special mini-frame 
    m_pFloatingFrameClass = RUNTIME_CLASS(CSCBMiniDockFrameWnd);        // window when control panel floats...
#endif //_SCB_REPLACE_MINIFRAME

   DockControlBar( &m_wndToolBar );                                     // dock the toolbars & control panels NOW
   DockControlBar( &m_plotPanel, AFX_IDW_DOCKBAR_RIGHT );               // dock plot panel at right
   DockControlBar( &m_dataPanel, AFX_IDW_DOCKBAR_BOTTOM );              // dock data trace panel at bottom
   DockControlBar( &m_logPanel, AFX_IDW_DOCKBAR_BOTTOM );               // dock msg log panel at bottom
   DockControlBar( &m_modePanel, AFX_IDW_DOCKBAR_RIGHT );               // mode control panel docked at right 
   DockControlBar( &m_histPanel, AFX_IDW_DOCKBAR_BOTTOM );              // dock spike histograms panel at bottom

   if( VerifyBarState( BAR_KEY ) )                                      // if control bar state in registry is valid, 
   {                                                                    // load it now:
      CSizingControlBar::GlobalLoadState( this, BAR_KEY );              //    state info specific to CSizingControlBar 
      LoadBarState( BAR_KEY );
   }

   ((CCntrlxApp*)AfxGetApp())->GetRuntime()->RegisterGUI(               // register display & mode control panels with 
         &m_modePanel, &m_dataPanel, &m_plotPanel, &m_histPanel );      // the CNTRLX/CXDRIVER runtime interface

   m_idTimer = SetTimer( 1, 10, NULL );                                 // setup timer to ensure we service 
                                                                        // CXDRIVER when idle proc is suspended

   return( 0 );
}

/**
 * This override intercepts the system close command (SC_CLOSE) to prevent user from exiting Maestro by clicking the
 * close button in the title bar. Exiting Maestro this way in the past has consistently led to crashes that required a 
 * reboot to restore normal Maestro operation. [NOTE: We remove the SC_CLOSE command from the system menu in OnCreate.
 * While that also makes the close button in the title bar LOOK disabled, it is still operational. Hence the need for
 * this override.
 */
void CCxMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{ 
   if(nID == SC_CLOSE) return;

   CFrameWnd::OnSysCommand(nID, lParam);
}

//=== OnDestroy [base override] ======================================================================================= 
//
//    Response to the WM_DESTROY message, sent when frame window is destroyed.
//
//    Here's where destroy the Visual Framework object that manages the layout of the frame window's client area.  We 
//    also unregister the GUI runtime elements from the CNTRLX runtime interface object.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxMainFrame::OnDestroy() 
{
   if( m_idTimer ) KillTimer( m_idTimer ); 
   ((CCntrlxApp*) AfxGetApp())->GetRuntime()->UnregisterGUI();
   CFrameWnd::OnDestroy();
   m_FrameLayout.Destroy();
}


//=== OnTimer ========================================================================================================= 
//
//    Service CXDRIVER each time our 10ms timer expires.
//
//    BACKGROUND:  In an effort to avoid a multithreaded design, we chose to service CXDRIVER in CCntrlxApp::OnIdle(). 
//    The problem with this approach is that OnIdle() may not be called for extended periods of time if the app's 
//    message pump, CWinApp::Run(), is bypassed by a modal loop inside the Windows kernel.  This happens, eg, when the 
//    user moves or resizes a window or manipulates a menu.  In an attempt to ensure a minimum level of responsiveness 
//    to CXDRIVER service requests during such modal loops, we've set up a timer that expires every 10ms.  Upon 
//    expiration, the WM_TIMER message is posted to the thread queue and ultimately dispatched to this method.
//
//    This does NOT solve all the problems we observe:  the WM_TIMER message is a low-priority message, so its 
//    dispatch can be significantly delayed by heavy Windows message traffic (eg. WM_MOUSEMOVE).  Thus, the minimum 
//    level of service may not be 10ms.  In addition, I've observed at least one situation where the WM_TIMER 
//    messages are not dispatched at all:  if the user holds the mouse button down while on the title bar of a top-
//    level window without moving the mouse.  Fortunately, that is not a natural user behavior.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxMainFrame::OnTimer( UINT_PTR nIDEvent )
{
   if( nIDEvent == m_idTimer) ((CCntrlxApp*)AfxGetApp())->GetRuntime()->Service();
}


//=== OnOutputPanel, OnUpdateOutputPanel ============================================================================== 
//
//    OnOutputPanel() is the ON_COMMAND_RANGE handler which toggles the visibility state of the four Maestro runtime 
//    docking bar display panels.  OnUpdateOutputPanel() is the ON_UPDATE_COMMAND_UI_RANGE handler which updates the 
//    state of the corresponding menu items.
//
//    NOTE:  Three of the Maestro "display panels" are only available when GUI "runtime support" is enabled.  The 
//    message log is always available.
//
//    ARGS:       nID   -- [in] command ID. 
//                pCmdUI-- [in] this object handles the details of updating the menu item state.
//
//    RETURNS:    NONE.
//
void CCxMainFrame::OnOutputPanel( UINT nID ) 
{
   CControlBar* pBar = NULL;
   switch( nID )
   {
      case ID_VIEW_PLOTPANEL  : pBar = (CControlBar*) &m_plotPanel; break;
      case ID_VIEW_DATAPANEL  : pBar = (CControlBar*) &m_dataPanel; break;
      case ID_VIEW_LOGPANEL   : pBar = (CControlBar*) &m_logPanel; break;
      case ID_VIEW_HISTPANEL   : pBar = (CControlBar*) &m_histPanel; break;
      default : return;
   }
   ShowControlBar( pBar, !pBar->IsVisible(), FALSE );
}

void CCxMainFrame::OnUpdateOutputPanel( CCmdUI* pCmdUI ) 
{
   CControlBar* pBar = NULL;
   switch( pCmdUI->m_nID )
   {
      case ID_VIEW_PLOTPANEL  : pBar = (CControlBar*) &m_plotPanel; break;
      case ID_VIEW_DATAPANEL  : pBar = (CControlBar*) &m_dataPanel; break;
      case ID_VIEW_LOGPANEL   : pBar = (CControlBar*) &m_logPanel; break;
      case ID_VIEW_HISTPANEL   : pBar = (CControlBar*) &m_histPanel; break;
      default : return;
   }

   BOOL bEnable = ((CCntrlxApp*)AfxGetApp())->GetRuntime()->IsOn();
   pCmdUI->Enable( (pCmdUI->m_nID == ID_VIEW_LOGPANEL) ||                        // message log panel always enabled
                   bEnable );                                                    // others solely for runtime support 
   pCmdUI->SetCheck( pBar->IsVisible() );
}


//=== OnModePanel, OnUpdateModePanel ================================================================================== 
//
//    OnModePanel() is the ON_COMMAND_RANGE handler which switches CNTRLX's operational mode.  The master mode control 
//    panel (CCxControlPanel) is used to affect the runtime behavior of CNTRLX/CXDRIVER in any one of four op modes:
//    Idle, Trial, Cont, and TestMode.  If CXDRIVER does not have the required hardware to run experimental protocols, 
//    TrialMode and ContMode are disabled, and if CXDRIVER is not running, all op modes are disabled and the mode 
//    control panel is inaccessible.
//
//    OnUpdateModePanel() is the ON_UPDATE_COMMAND_UI_RANGE handler which updates the state of the corresponding menu 
//    items.
//
//    The master mode control panel handles all the details of a mode switch, but the mainframe window must hide or 
//    show the control panel, as appropriate.  Note that if the system is in IdleMode and the ID_MODE_IDLE command is 
//    given, we merely toggle the show/hide state of the master mode control panel.  In all other op modes, the mode 
//    control panel must be visible.
//
//    An additional "Mode" menu command, ID_MODE_RESTART, allows the user to restart CXDRIVER at any time. 
//
//    ARGS:       nID   -- [in] command ID. 
//                pCmdUI-- [in] this object handles the details of updating the menu item state.
//
//    RETURNS:    NONE.
//
void CCxMainFrame::OnModePanel( UINT nID ) 
{
   if( nID == ID_MODE_RESTART )                                            // restart CXDRIVER, or...
   {
      ((CCntrlxApp*)AfxGetApp())->GetRuntime()->Start();
      return;
   }

   int iOpMode = -1;                                                       // switch the current runtime op mode
   switch( nID )
   {
      case ID_MODE_IDLE :     iOpMode = CCxRuntime::IdleMode; break;
      case ID_MODE_TRIALS :   iOpMode = CCxRuntime::TrialMode; break;
      case ID_MODE_CONT :     iOpMode = CCxRuntime::ContMode; break;
      case ID_MODE_TEST :     iOpMode = CCxRuntime::TestMode; break;
      default :               TRACE0("\nUnrecognized op mode!"); return;   // should never get here!
   }

   if( m_modePanel.SwitchMode( iOpMode ) )                                 // mode control panel handles mode switch; 
   {                                                                       // if successful, we ensure the mode control 
      if( nID == ID_MODE_IDLE )                                            // panel is visible -- but in IdleMode we 
         ShowControlBar( &m_modePanel, !m_modePanel.IsVisible(), FALSE );  // always toggle the panel's visible state. 
      else if( !m_modePanel.IsVisible() )
         ShowControlBar( &m_modePanel, TRUE, FALSE );

      m_plotPanel.Activate( BOOL(iOpMode > CCxRuntime::IdleMode) );        // eye-tgt plot only active in non-idle mode 
   }
   else                                                                    // if mode switch failed, CXDRIVER may have  
      EnableRunModes();                                                    // died -- so update GUI accordingly
}

void CCxMainFrame::OnUpdateModePanel( CCmdUI* pCmdUI ) 
{
   if( pCmdUI->m_nID == ID_MODE_RESTART ) { pCmdUI->Enable(); return; }    // "restart" cmd always enabled

   CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();       // check current op mode 
   int iCurrMode = pRuntime->GetMode();

   int iMode;                                                              // map command ID to op mode ID
   switch( pCmdUI->m_nID )
   {
      case ID_MODE_IDLE    : iMode = CCxRuntime::IdleMode; break;
      case ID_MODE_TRIALS  : iMode = CCxRuntime::TrialMode; break;
      case ID_MODE_CONT    : iMode = CCxRuntime::ContMode; break;
      case ID_MODE_TEST    : iMode = CCxRuntime::TestMode; break;
      default: TRACE0( "Unrecognized CNTRLX op mode!\n" ); return;
   }

   pCmdUI->Enable( pRuntime->IsModeEnabled( iMode ) ); 
   pCmdUI->SetCheck( BOOL(iMode == iCurrMode) );
}


//=== OnGlobalShortcut ================================================================================================ 
//
//    Handler for eight "global" keyboard accelerator commands which are relevant only during Continuous or Trial mode, 
//    but which are NOT associated with a menu item:
//       1) ID_CM_TOGGLEFIX, which toggles fixation ON/off when Maestro is in Continuous mode.  This lets the user 
//          toggle fixation without having to click the relevant pushbutton on the Continuous mode control panel.
//       2) ID_CM_TOGGLETRACK, which toggles the ON/off state of the designated cursor-tracking target in Continuous 
//          mode (if there is an active target so designated).
//       3) ID_TM_TOGGLESTART, relevant only in Trial mode, will start the trial sequencer if it is not running, or 
//          stop it otherwise.
//       4) ID_TM_TOGGLEPAUSE, relevant only in Trial mode when the sequencer is running, will pause the sequencer if 
//          it is currently running, or resume it if it is paused.
//       5) ID_CM_TRKSPEEDUP, _TRKSPEEDDN : Relevant only in Continuous mode, these increment/decrement the pattern 
//          speed of the active target designated as the "Track" target (if there is one) by a set amount.
//       6) ID_CM_TRKDIRUP, _TRKDIRDN : Relevant only in Continuous mode, these increment/decrement the pattern 
//          direction of motion for the active target designated as the "Track" target by a set amount.
//
//    The method simply hands off the task to the master mode control panel object, CCxControlPanel.
//
//    ARGS:       nID -- [in] the shortcut command ID (one of the commands listed above).
//
//    RETURNS:    NONE.
//
void CCxMainFrame::OnGlobalShortcut( UINT nID )
{
   m_modePanel.HandleGlobalModeShortcut( nID );
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== OnCreateClient [base override] ================================================================================== 
// 
//    Layout a Visual Framework object to handle the views that appear in the main frame window.
//
//    PURPOSE:
//       Standard MFC provisions for multiple views of a document are somewhat limited.  Here we take advantage of the 
//    Visual Framework library (a free extension of MFC; see visualfx.*) to layout a relatively complex, multiple-view 
//    GUI for CNTRLX.  Our GUI consists of a 1x2 static splitter window with CCxObjectTree in the left pane and a tab 
//    window in the right pane.  A tab pane is installed in this tab window for each type of "data object" (targets, 
//    trials, etc) installed in CNTRLX. 
//
//    IMPORTANT:
//       (1) The Visual Framework object handles the entire client area for the frame window, so we must NOT call the 
//    base class version.  A consequence:  the view class which is bound to CCxMainFrame & CCxDoc in the document 
//    template does NOT get used; it is essentially a place-filler!  See CCntrlxApp implementation file. 
//       (2) Since we create the visual framework object here, it is important that we destroy it when the window is 
//    destroyed.  See OnDestroy().
//
//    ARGS:       lpcs     -- [in] pointer to creation struct, defining init params passed to the window proc of app. 
//                pContext -- [in] info used to connect the components that make up a doc & the views of its data. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise. 
//
BOOL CCxMainFrame::OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext ) 
{
   // first create the visual objects:
   //    ==> 1x2 static splitter window (root object)
   //    ==> left-hand pane is the CNTRLX object tree view
   //    ==> right-hand pane is a tab window.
   //    ==> there is a tab pane for each distinct type of CNTRLX data object
   //
   TVisualObject *pSplit = new TVisualObject( 1, "", 1, 2, pContext );
   TVisualObject *pSplitView1 = 
      new TVisualObject(IDC_OBJTREE, 0, 0, pContext, RUNTIME_CLASS(CCxObjectTree), CSize(200,400) );
   TVisualObject *pSplitView2 = 
      new TVisualObject(IDC_DATATABWND, 0, 1, pContext, RUNTIME_CLASS(TTabWnd), CSize(600,400), 
                           TVisualObject::TOS_TABTOP );

   TVisualObject *pTargTab = new TVisualObject(IDC_TARGTAB, _T("Target"), pContext, RUNTIME_CLASS(CCxTargForm) );
   TVisualObject *pChanTab = new TVisualObject( IDC_CHANTAB, _T("Chan Cfg"), pContext, RUNTIME_CLASS(CCxChannelForm) ); 
   TVisualObject *pPertTab = new TVisualObject(IDC_PERTTAB, _T("Perturbations"), pContext, RUNTIME_CLASS(CCxPertForm)); 
   TVisualObject *pTrialTab = new TVisualObject(IDC_TRIALTAB, _T("Trial"), pContext, RUNTIME_CLASS(CCxTrialForm) );
   TVisualObject *pRunTab = new TVisualObject( IDC_CONTRUNTAB, _T("Run"), pContext, RUNTIME_CLASS(CCxContRunForm) );

   // define icons for the tab panes
   pTargTab->SetIcon(IDI_TGT_ICON);
   pTrialTab->SetIcon(IDI_TRIAL_ICON);
   pChanTab->SetIcon(IDI_CHAN_ICON);
   pRunTab->SetIcon(IDI_RUN_ICON);
   pPertTab->SetIcon(IDI_PERT_ICON);

   // now add visual objects to framework 
   //
   m_FrameLayout.Add( pSplit );                       // splitter window is root object  
   m_FrameLayout.Add( pSplit, pSplitView1 );          // attach splitter panes to the parent splitter 
   m_FrameLayout.Add( pSplit, pSplitView2 );          //  
   m_FrameLayout.Add( pSplitView2, pTrialTab );       // attach tab panes to the tab window
   m_FrameLayout.Add( pSplitView2, pTargTab );
   m_FrameLayout.Add( pSplitView2, pRunTab );
   m_FrameLayout.Add( pSplitView2, pChanTab );
   m_FrameLayout.Add( pSplitView2, pPertTab );

   return( m_FrameLayout.Create( this ) );            // create the GUI 

   // since visual framework handles entire client area, we do not need to call the base class: 
   // return CFrameWnd::OnCreateClient(lpcs, pContext);
}


//=== PreCreateWindow [CWnd override] ================================================================================= 
// 
//    Called by framework prior to window creation to permit window customization by derived classes.
//
//    We override this so that we can restore the frame window's rect and its maximized/normal state.  The window's 
//    control bar state is restored after it is created -- see OnCreate().
//
//    [CREDITS:  This function was adapted from code snippet by JR Skinner.]
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if window creation should continue; FALSE to abort window creation. 
//
BOOL CCxMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
   CWinApp* pApp = AfxGetApp();
   int iShow = pApp->GetProfileInt( WND_KEY, "Show", -1);

   if( iShow == -1 )                                           // frame window placement info not found; make frame 
   {                                                           // window 3/4 screen size & centered
       cs.cy = 3 * ::GetSystemMetrics( SM_CYSCREEN ) / 4; 
       cs.cx = 3 * ::GetSystemMetrics( SM_CXSCREEN ) / 4; 
       cs.y = cs.cy / 8; 
       cs.x = cs.cx / 8;
   }
   else                                                        // else use registry settings -- but we do NOT let the 
   {                                                           // window come up in the minimized state
      pApp->m_nCmdShow = (iShow == SW_SHOWMAXIMIZED) ? iShow : SW_SHOWNORMAL;
      cs.x = pApp->GetProfileInt( WND_KEY, "Left", 0);
      cs.y = pApp->GetProfileInt( WND_KEY, "Top", 0);
      cs.cx = pApp->GetProfileInt( WND_KEY, "Right", 0)  - cs.x;
      cs.cy = pApp->GetProfileInt( WND_KEY, "Bottom", 0) - cs.y;
   }

   return( CFrameWnd::PreCreateWindow(cs) );
}


//=== DestroyWindow [CWnd override] =================================================================================== 
// 
//    Destroy the HWND attached to the CWnd object.
//
//    We override this so that we can save the frame window's rect and its control bar state info in the registry prior 
//    to its destruction.  Note that neither the frame window nor any of its children are destroyed yet.
//
//    [CREDITS:  Code for preserving frame window rect by JR Skinner; for control bar state, C. Posea.]
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if window is destroyed, FALSE otherwise. 
//
BOOL CCxMainFrame::DestroyWindow()
{
   WINDOWPLACEMENT wp;                                            // get window's current placement information
   wp.length = sizeof(WINDOWPLACEMENT);
   GetWindowPlacement( &wp );

   CWinApp* pApp = AfxGetApp();                                   // and save it in the registry
   pApp->WriteProfileInt( WND_KEY, "Show", wp.showCmd);
   pApp->WriteProfileInt( WND_KEY, "Left", wp.rcNormalPosition.left);
   pApp->WriteProfileInt( WND_KEY, "Right", wp.rcNormalPosition.right);
   pApp->WriteProfileInt( WND_KEY, "Top",  wp.rcNormalPosition.top);
   pApp->WriteProfileInt( WND_KEY, "Bottom", wp.rcNormalPosition.bottom);

   CSizingControlBar::GlobalSaveState( this, BAR_KEY );
   SaveBarState( BAR_KEY );
   return( CFrameWnd::DestroyWindow() );
}


//=== EnableRunModes ================================================================================================== 
// 
//    Enable/disable GUI support for Maestro runtime.
//
//    Elements of the Maestro GUI are dedicated to the runtime control of a Maestro protocol, or to the display of data 
//    as the protocol progresses.  These include the master mode control panel, the eye/tgt position plot panel, the
//    data trace display panel, and the spike histograms panel.  If the Maestro hardware driver is not available, these 
//    elements should not be accessible to the user.  Use this method to enable or disable them as needed. 
// 
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxMainFrame::EnableRunModes()
{
   if( !(((CCntrlxApp*) AfxGetApp())->GetRuntime()->IsOn()) )     // if CXDRIVER is not running, make sure all GUI 
   {                                                              // elements that support runtime are hidden.
      if( m_plotPanel.IsVisible() ) ShowControlBar( &m_plotPanel, FALSE, FALSE );
      if( m_dataPanel.IsVisible() ) ShowControlBar( &m_dataPanel, FALSE, FALSE ); 
      if( m_histPanel.IsVisible() ) ShowControlBar( &m_histPanel, FALSE, FALSE ); 
      if( m_modePanel.IsVisible() ) ShowControlBar( &m_modePanel, FALSE, FALSE );
   }
}

//=== IsObjectSelected ================================================================================================ 
//
//    Is the specified Maestro document object currently selected in the object tree?
//
//    This method is a HACK to let other GUI components determine the selection state of any object in the object tree 
//    (CCxObjectTree). A GUI component can get a reference to CCxMainFrame via CWnd::GetParentFrame() or 
//    CCntrlxApp::GetMainFrame().
//
//    ARGS:       wKey -- [in] Unique key of the document object to be checked.
//
//    RETURNS:    TRUE if specified object is currently selected in object tree; FALSE otherwise.
//
BOOL CCxMainFrame::IsObjectSelected(WORD wKey)
{
   CCxObjectTree* pTree = (CCxObjectTree*) m_FrameLayout.GetObject((DWORD)IDC_OBJTREE);
   return((pTree == NULL) ? FALSE : pTree->IsObjSelected(wKey));
}


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump internal state for debugging purposes.  Not tailored to CCxMainFrame.
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CCxMainFrame::Dump( CDumpContext& dc ) const
{
   CFrameWnd::Dump( dc ); 
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate internal consistency for debugging purposes.  Not tailored to CCxMainFrame.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxMainFrame::AssertValid() const
{
   CFrameWnd::AssertValid();
}

#endif //_DEBUG



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== GetTabPaneID ==================================================================================================== 
// 
//    Maps CNTRLX data object type to the ID of the tab pane in which objects of that type are displayed/defined.
//
//    ARGS:       wObjType -- [in] the CNTRLX data object type. 
//
//    RETURNS:    0 if object type not recognized; else ID of associated tab pane. 
//
DWORD CCxMainFrame::GetTabPaneID( const WORD wObjType ) const
{
   if( (wObjType >= CX_FIRST_TARG) && (wObjType <= CX_LAST_TARG) )   return( (DWORD) IDC_TARGTAB );
   else if( wObjType == CX_CHANCFG )                                 return( (DWORD) IDC_CHANTAB );
   else if( wObjType == CX_PERTURB )                                 return( (DWORD) IDC_PERTTAB );
   else if( wObjType == CX_TRIAL )                                   return( (DWORD) IDC_TRIALTAB );
   else if( wObjType == CX_CONTRUN )                                 return( (DWORD) IDC_CONTRUNTAB );
   else
   {
      TRACE0( "Unrecognized CNTRLX data object type\n" );
      return( (DWORD) 0 );
   }
}


//=== VerifyBarState ================================================================================================== 
// 
//    [This function is Copyright (C) 2000, Cristi Posea.  Minor changes by saruffner.]
//
//    The methods which persist control bar state information in the registry identify the individual control bars by 
//    their control ID.  Thus, not only must we ensure that each control bar gets a different ID, but we also have to 
//    deal with out-of-data control bar settings:  If the application is modified such that one or more control bar 
//    IDs are changed, or at least one control bar is removed, then existing control bar settings in the registry will 
//    contain at least one invalid control bar.  When CFrameWnd::LoadBarState() attempts to load the state of this 
//    non-existent control bar, it will crash.  
//
//    VerifyBarState() lets us avoid this problem.  It examines the control bar state info and returns FALSE if it 
//    finds any invalid control bar IDs.  In such a case, we must avoid loading the control bar state.
//    
//
//    ARGS:       lpszProfileName   -- [in] name of registry key under which control bar state info is stored.
//
//    RETURNS:    TRUE if stored control bar state info is valid, FALSE otherwise.
//
BOOL CCxMainFrame::VerifyBarState( LPCTSTR lpszProfileName )
{
   CDockState state;
   state.LoadState( lpszProfileName );

   for( int i = 0; i < state.m_arrBarInfo.GetSize(); i++ )
   {
      CControlBarInfo* pInfo = (CControlBarInfo*) state.m_arrBarInfo[i];
      ASSERT(pInfo != NULL);
      int nDockedCount = static_cast<int>(pInfo->m_arrBarID.GetSize());
      if( nDockedCount > 0 )                                               // dockbar -- check all control bars that 
      {                                                                    // are docked to it...
         for( int j = 0; j < nDockedCount; j++ )
         {
            UINT nID = (UINT) pInfo->m_arrBarID[j];
            if( nID == 0 ) continue;                                       // row separator
            if( nID > 0xFFFF ) nID &= 0xFFFF;                              // placeholder -- get the ID
            if( GetControlBar(nID) == NULL )                               // invalid bar ID -- ABORT!
               return( FALSE );
         }
      }

      if( !pInfo->m_bFloating )                                            // floating dockbars can be created later
         if( GetControlBar( pInfo->m_nBarID ) == NULL )
            return( FALSE ); 
   }

   return( TRUE );
}
