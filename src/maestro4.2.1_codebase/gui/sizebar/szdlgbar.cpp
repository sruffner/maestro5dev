//===================================================================================================================== 
//
// szdlgbar.cpp : Implementation of class CSizingDialogBar, a resizable "dialog bar"; and CSizingTabDlgBar, a resizable 
//                tabbed dialog bar (somewhat like a property sheet inside a control bar).  Both house one or more
//                modeless, scrollable dialogs based on helper class CSzDlgBarDlg.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// MFC's CDialogBar is a CControlBar-derivative that houses a modeless dialog -- in other words, a "dialog bar".  Such 
// a GUI construct provides a means of managing a complex set of related controls on a dockable control bar.  However, 
// CControlBar has some significant limitations, which are inherited by CDialogBar. 
//
// CSizingDialogBar is a more flexible "dialog bar" that is based upon CSizingControlBar (see CREDITS), an extension of 
// MFC's CControlBar that implements a DevStudio-like resizable control bar.  Unlike CDialogBar, which is really a 
// control bar with dialog box-like properties, CSizingDialogBar is a control bar that houses a dialog box as a single 
// child window filling the parent bar's entire client area.  It provides a command/message routing framework to ensure 
// that the child dialog receives commands/messages intended for it.  It also handles destruction of the embedded 
// dialog when the control bar is destroyed.
//
// MFC does not provide a "tabbed dialog bar" class, ie, a control bar housing a tabbed dialog, aka "property sheet".  
// Furthermore, MFC's CPropertySheet is not designed to be fit inside a resizable control bar, as it does not support 
// scrolling the active dialog page when the visible area is too small.  Therefore, as a multi-dialog complement to 
// CSizingDialogBar, we provide CSizingTabDlgBar.  In this case, the control bar houses a child CTabCtrl along with one 
// or more modeless dialogs.  As with CSizingDialogBar, it provides a command/message routing framework to ensure that 
// the "active page" -- the dialog page that is currently "in front" -- receives commands and messages intended for it. 
// CSizingTabDlgBar distinguishes between dialog pages that are installed in the dialog bar and those that are 
// accessible from the bar.  To make a dialog page accessible to the user, one must install it in the tabbed dialog bar 
// via AddDlg(), then add it to the "tab list" via ShowDlg().  An accessible page can be later hidden by calling 
// HideDlg(), or destroyed entirely by calling RemoveDlg().  Thus, CSizingTabDlgBar facilitates dynamic modification of 
// the tabbed dialog bar's contents -- with or without repeated creation/destruction of the dialog objects.  When the 
// tabbed dialog bar is itself destroyed, it will also destroy the dialogs it contains.  Note that it currently 
// supports only text labels in the tab control, not icons.
//
// When CSizingTabDlgBar contains only a single accessible dialog page, the CTabCtrl is hidden and the dialog fills the 
// client area of the dialog bar -- in other words, it behaves like CSizingDialogBar (albeit with extra overhead).
//
// CSzDlgBarDlg serves as the base class for the child dialog(s) housed by CSizingDialogBar and CSizingTabDlgBar.  It 
// enforces a few restrictions on dialogs embedded in the parent dialog bars (they cannot be modal, e.g.), and it 
// provides scrolling support so that the user can scroll the dialog when its parent dialog bar is smaller than the 
// dialog template.
// 
// ==> Usage (CSizingDialogBar).
// (a) Derive a dialog class from CSzDlgBarDlg.  Add whatever functionality is required to support the set of controls 
// defined on the associated dialog template.  The class should define a parameterless constructor that passes the 
// resource ID of the dialog template to the base class constructor. THUS:
//
//    class CMyBarDlg : public CSzDlgBarDlg
//    {
//       DECLARE_DYNAMIC( CMyBarDlg )
//
//    protected:
//       static enum {IDD = IDD_MYBARDLGTEMPLATE};       // resource ID of dialog template
//
//    public:
//       CMyBarDlg() : CSzDlgBarDlg( IDD, NULL ) {}      // dialog's parent must be CSizingDialogBar or a derivative 
//
//       ...ETC...
//    };
//
// (b) Derive a dialog bar class, CMySizingDialogBar, from CSizingDialogBar.  The template class CSzDlgBarTemplate is 
// available to generate a lightweight derivative of CSizingDialogBar that is tailored to hold your dialog class.
//
// (c) In your mainframe class CMyMainFrame, add a member variable to represent the dialog bar:
//
//    CMySizingDialogBar m_dlgBar;   ...OR, using the template: CSzDlgBarTemplate<CMyBarDlg> m_dlgBar;
//
// (d) Then add the typical control bar creation code to CMyMainFrame::OnCreate().  For example:
//
//    if (!m_dlgBar.Create( _T("My Bar"), this, CSize(200, 100), TRUE, AFX_IDW_CONTROLBAR_FIRST + 32) )
//    { 
//       TRACE0( "Failed to create mybar\n" );
//       return( -1 );
//    }
//
//    m_dlgBar.SetBarStyle( m_dlgBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC );
//    m_dlgBar.EnableDocking( CBRS_ALIGN_ANY );
//    EnableDocking( CBRS_ALIGN_ANY );                                 // <---- needed only once for the frame
//    DockControlBar( &m_dlgBar, AFX_IDW_DOCKBAR_RIGHT );
//
// ==> Usage (CSizingTabDlgBar).
// (a) For each dialog you wish to house in the tabbed dialog bar, derive a class CMyDlg from CSzDlgBarDlg, in the 
// manner outlined above.  However, CMyDlg MUST support dynamic object creation (use the MFC macros DECLARE_DYNCREATE 
// and IMPLEMENT_DYNCREATE).  This is necessary so that CSizingTabDlgBar::AddDlg() can construct an instance of the 
// dialog object during runtime.
//
// (b) Derive a tabbed dialog bar class, CMySizingTabDlgBar, from CSizingTabDlgBar.  Include an OnCreate() override, in 
// which you call CSizingTabDlgBar::AddDlg() for each unique dialog that will be housed in the tabbed dialog bar, e.g.: 
// 
//    CMyPropDlg1* m_pDlg1 = (CMyPropDlg1*) AddDlg( _T("Property Page 1"), RUNTIME_CLASS(CMyPropDlg1) ); 
//    if( m_pDlg1 == NULL ) return( -1 );
//    CMyPropDlg2* m_pDlg2 = (CMyPropDlg2*) AddDlg( _T("Property Page 2"), RUNTIME_CLASS(CMyPropDlg2) ); 
//    if( m_pDlg2 == NULL ) return( -1 );
//    CMyPropDlg3* m_pDlg3 = (CMyPropDlg3*) AddDlg( _T("Property Page 3"), RUNTIME_CLASS(CMyPropDlg3) );
//    if( m_pDlg3 == NULL ) return( -1 );
// 
// Note that each dialog thus created is a child of the tabbed dialog bar, NOT the tab control (another child of the 
// dialog bar) that's used to navigate among the dialogs.  Also, dialogs CAN be dynamically added and removed after the 
// dialog bar creation phase using AddDlg() and RemoveDlg(), respectively.
//
// ALSO note that we "remember" the dialog ptr returned by AddDlg().  Use this ptr to identify the particular dlg page 
// in future calls to CSizingTabDlgBar methods. 
//
// Alternatively, you could use CSizingTabDlgBar as is, creating an instance in your mainframe class.  This means that 
// the mainframe would have to do all of the above work, and the tabbed dialog bar is not "self-contained".  Also, the 
// mainframe will not have access to the dialogs created by CSizingTabDlgBar, as access to the dialog objects is 
// protected -- so, designing a self-contained dialog bar class is encouraged.
//
// (c) As above (however, there's no template for generating classes based on CSizingTabDlgBar).
//
// (d) As above.
//
// (e) Show/HideDlg().  AddDlg() merely creates the dialog page.  To make the new dialog page accessible to the user, 
// one must invoke ShowDlg(), specifying the desired tab position for the dialog page; it can be subsequently hidden 
// from the user by invoking HideDlg().  In this case, the dialog page is not destroyed -- its associated tab is merely 
// removed from the embedded tab control.  Thus, the subset of installed dlg pages which are currently accessible in 
// the tabbed dialog bar can be dynamically changed via these two methods.
//
// (f) Other operations:  Call EnableDlg( pDlg, bEnable ) to enable/disable a particular dialog page in the tabbed 
// dialog bar (pages are created in the enabled state).  When disabled, the associated tab label is grayed out and the 
// user cannot bring the dialog page to the front.  To obtain the tab index of the currently displayed ("active") 
// dialog, call GetActiveTab().  To change the active dialog programmatically, call SetActiveDlg().
//
// DEVELOPMENT NOTES:
// 1) Tab control implementation in CSizingTabDlgBar.  MFC's CTabCtrl does NOT intrinsicly support enabling/disabling 
// tab pages (part of the problem is the conceptual design:  the tabbed "pages" are typically not children of the tab 
// control, but of some common parent window).  To gray out disabled tabs, we need to use an "owner-drawn" tab control 
// and do the drawing ourselves.  Thus, CSizingTabDlgBar handles the WM_DRAWITEM message to draw each tab label; tab 
// item images are NOT supported.  Also, the TCN_SELCHANGE notification handler prevents the user from selecting 
// disabled tabs.  Admittedly, this is not a very good solution.  A much better and more extensible approach involves 
// designing a CTabCtrl derivative that handles the enable/disable feature and draws the tab labels itself...
//
//
// CREDITS:
// 1) CSizingControlBar: resizable control bars, by Cristi Posea [03/31/2002, www.datamekanix.com/sizecbar] --  An 
// extension of MFC's CControlBar that implements a DevStudio-like docking window that can contain dialogs, controls, 
// even views (although not recommended).  CSizingDialogBar is derived from CSizingControlBarCF.  Freeware. V2.44.
//
// 2) Dynamic child window positioning, by Hans Buhler [4/27/2000, www.datamekanix.com/articles/dynwindow] -- This 
// article describes code implementing a CWnd-derivative that dynamically repositions its child windows whenever it is 
// resized.  While we're not interested in that functionality, the freeware code includes an example of how to use a 
// dialog within a CSizingControlBar.  We've adapted some of that code for use here.
//
// 3) A scrollable dialog class, by Felix Rivera [07/03/1999, codeguru.earthweb.com/dialog/ScrollDialog.shtml] -- 
// Rivera's CScrollDialog is a simple CDialog derivative that implements scrolling.  CSzDlgBarDlg uses elements of 
// CScrollDialog's implementation, but it is tailored for use with a non-resizable dialog window. 
//
// 4) Sizing TabControlBar, by Dirk Clemens [08/07/1998, codeguru.earthweb.com/docking/sizing_tabctrl_bar.shtml] --
// Clemens' CSizingTabCtrlBar is derived from an outdated version of CSizingControlBar and it houses CView-derived 
// objects rather than dialogs.  Still, it served as the starting point from which I built CSizingTabDlgBar.
//
// 5) Implementing an owner-drawn tab control, by Chris Maunder [01/24/1999, codeguru.earthweb.com/controls/
// ownerdraw_tab.shtml].  Implementation of CSizingTabDlgBar::OnDrawItem(), which draws the tab control labels, was 
// adapted from this article.  Maunder creates a CTabCtrl derivative that that draws itself, which is a more extensible 
// approach than ours here...
//
//
// REVISION HISTORY:
// 19apr2001-- Began development.
// 20apr2001-- Done, but scrolling implementation in CSzDlgBarDlg could use some improvements.
// 30apr2001-- Fixed some compile errors.  Otherwise, appears to be working...
// 24may2001-- Began work on tabbed dialog bar class, CSizingTabDlgBar.
// 31may2001-- CSizingTabDlgBar appears to be working, except for a bug in which the active dialog fails to be 
//             repainted after a resize event (may have something to do with fact that dialogs are NOT children of 
//             the tab control).  Working on it.  Also need to test with multiple dialogs...
// 05jun2001-- Fixed problem repainting active dialog.  Since the tab control and the currently active dialog are 
//             siblings of the parent dialog bar, one must ensure that the active dialog precedes the tab control in 
//             the z-order -- see SetActiveDlg().
// 27jun2001-- Added support for enabling/disabling selected tabs in CSizingTabDlgBar.  Also using a typed ptr array 
//             for the collection of tab dialog pages for simple indexed access to the individual pages.
// 04oct2001-- Modified CSizingTabDlgBar so that it "looks like" CSizingDialogBar when only one dialog page installed. 
// 01apr2003-- Modified CSizingTabDlgBar so that it is possible to show only a subset of the installed dialog pages.
//             ShowDlg() makes the specified dialog page "accessible" (by inserting a tab in the child tab ctrl), while 
//             HideDlg() makes it inaccessible (by removing that tab) but does NOT destroy the dialog object.  This 
//             provides a more efficient mechanism for dynamically changing the set of dlg pages visible in the bar 
//             while maintaining the state of those dialogs that are "invisible".
//          -- CSizingTabDlgBar::AddDlg() returns a unique dialog page ID for the created dialog.  This dialog page ID 
//             must be used to identify a particular dialog in calls to other CSizingTabDlgBar methods; it remains 
//             valid until the dialog is destroyed by RemoveDlg().
// 04apr2003-- Instead of a page ID, AddDlg() merely returns a CSzDlgBarDlg* pointer to the dialog created.  This makes 
//             more sense, since derived classes are going to need the pointers anyway to "talk to" the individual 
//             dialogs...
// 10apr2003-- Upgraded the CSizingControlBar framework from version 2.43 to 2.44.  New version includes a bug fix 
//             that showed up in WinXP.
// 15mar2004-- Added PreTranslateMessage() override to CSzDlgBarDlg to give parent frame window hierarchy first crack 
//             at translating messages because the dialog's default implementation (win32 SDK ::IsDialogMessage()) 
//             eats frame window accelerators.  Followed example of MFC's CFormView.
// 02jun2004-- Change dtd 15mar2004 fixed frame window accelerator problem, but it screwed up the basic dialog keybrd 
//             interface (eg., user could no longer tab among controls on the CSzDlgBarDlg).  Fixing.
// 31aug2017-- Various technical changes to fix warnings and errors for 64-bit compilation in VS 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "szdlgbar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// 
// Implementation of CSzDlgBarDlg
//
//===================================================================================================================== 

IMPLEMENT_DYNAMIC( CSzDlgBarDlg, CDialog )

BEGIN_MESSAGE_MAP( CSzDlgBarDlg, CDialog )
   ON_WM_CLOSE()
   ON_WM_HSCROLL()
   ON_WM_VSCROLL()
   ON_WM_SIZE()
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== Create ========================================================================================================== 
//
//    This protected creation method is used to create dialog as a child of CSizingDialogBar or CSizingTabDlgBar.  It 
//    represents an attempt to enforce the requirement that dialogs used with the two dialog bar classes must be 
//    derived from CSzDlgBarDlg!!
//
//    ARGS:       pBar  -- [in] the parent dialog bar
//
//    RETURNS:    TRUE if successful, FALSE otherwise. 
//
BOOL CSzDlgBarDlg::Create( CWnd* pBar )
{
   ASSERT( pBar->IsKindOf( RUNTIME_CLASS(CSizingDialogBar) ) || pBar->IsKindOf( RUNTIME_CLASS(CSizingTabDlgBar) ) );
   return( CDialog::Create( m_nID, pBar ) );
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnHScroll, OnVScroll [base override] ============================================================================ 
//
//    Handle events on the scroll bars.
//
//    ARGS:       nSBCode     -- [in] scroll bar code indicating kind of scroll event that has occurred. 
//                nPos        -- [in] scroll-box position if SB_THUMBTRACK, SB_THUMBPOSITION; else not used. 
//                pScrollBar  -- [in] NULL if scroll event from the window's scroll bars; else, ptr to scroll bar that 
//                               generated the event. 
//
//    RETURNS:    NONE. 
//
void CSzDlgBarDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   int nInc;

   switch( nSBCode )
   {
      case SB_TOP:        nInc = -m_nHscrollPos;               break;
      case SB_BOTTOM:     nInc = m_nHscrollMax-m_nHscrollPos;  break;
      case SB_LINEUP:     nInc = -1;                           break;
      case SB_LINEDOWN:   nInc = 1;                            break;
      case SB_PAGEUP:     nInc = -HORZ_PTS;                    break;
      case SB_PAGEDOWN:   nInc = HORZ_PTS;                     break;
      case SB_THUMBTRACK: nInc = nPos - m_nHscrollPos;         break;
      default:            nInc = 0;
   }

   nInc = max(-m_nHscrollPos, min(nInc, m_nHscrollMax - m_nHscrollPos) );

   if( nInc )
   {
      m_nHscrollPos += nInc;
      int iMove = -HORZ_PTS * nInc;
      ScrollWindow( iMove, 0, NULL, NULL );
      SetScrollPos( SB_HORZ, m_nHscrollPos, TRUE );
   }

   CDialog::OnHScroll( nSBCode, nPos, pScrollBar );
}

void CSzDlgBarDlg::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar ) 
{
   int nInc;

   switch( nSBCode )
   {
      case SB_TOP:        nInc = -m_nVscrollPos;               break;
      case SB_BOTTOM:     nInc = m_nVscrollMax-m_nVscrollPos;  break;
      case SB_LINEUP:     nInc = -1;                           break;
      case SB_LINEDOWN:   nInc = 1;                            break;
      case SB_PAGEUP:     nInc = min(-1, -m_nVertInc);         break;
      case SB_PAGEDOWN:   nInc = max(1, m_nVertInc);           break;
      case SB_THUMBTRACK: nInc = nPos - m_nVscrollPos;         break;
      default:            nInc = 0;
   }

   nInc = max( -m_nVscrollPos, min(nInc, m_nVscrollMax - m_nVscrollPos) );

   if( nInc )
   {
      m_nVscrollPos += nInc;
      int iMove = -VERT_PTS * nInc;
      ScrollWindow( 0, iMove, NULL, NULL );
      SetScrollPos( SB_VERT, m_nVscrollPos, TRUE );
   }

   CDialog::OnVScroll( nSBCode, nPos, pScrollBar );
}


//=== OnSize [base override] ========================================================================================== 
//
//    Response to WM_SIZE message.
//
//    Whenever the child dialog is resized, we must check to see whether or not the scroll bars need to appear, and 
//    adjust the scroll bar status info
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
void CSzDlgBarDlg::OnSize( UINT nType, int cx, int cy ) 
{
   CDialog::OnSize( nType, cx, cy );
   if( m_bInitialized )
   {
      ResetScrollbars();
      SetupScrollbars();
   }
}




//===================================================================================================================== 
// OPERATIONS/IMPLEMENTATION
//===================================================================================================================== 

//=== OnInitDialog [base override] ==================================================================================== 
//
//    Prepare dialog for display.  Here we initialize the scroll info based on the initial dimensions of the dialog 
//    (assumed to be == the dialog template's size).  We also attempt modify the window styles in an effort to remove 
//    title bars and borders from the dialog:  since it will be a single child filling the client area of the dialog 
//    bar parent, there's no need for these window elements!
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
BOOL CSzDlgBarDlg::OnInitDialog() 
{
   CDialog::OnInitDialog();

   m_nHscrollPos = 0;
   m_nVscrollPos = 0;

   GetWindowRect( &m_ClientRect );                       // NOTE that we use the **entire** window rect!...
   m_bInitialized = TRUE;

   ModifyStyle( WS_THICKFRAME | WS_OVERLAPPEDWINDOW,     // ....so, if either of these fail, scrolling may be screwy!
                WS_CHILD, 
               SWP_FRAMECHANGED );
   ModifyStyleEx( WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, 
                  0, SWP_FRAMECHANGED );

   return( TRUE );                                       // return TRUE unless you set the focus to a control
                                                         // EXCEPTION: OCX Property Pages should return FALSE
}


//=== PreTranslateMessage [base override] ============================================================================= 
//
//    CSzDlgBarDlg is intended for use within a dialog bar that is ultimately docked to the application's main frame 
//    window.  However, the base class CDialog implementation of this method eats frame window accelerators.  Thus, if 
//    the focus happens to be in CSzDlgBarDlg, frame window accelerators would not work unless we override the base 
//    class behavior.  This override does just that, giving the parent control bar's frame window hierarchy first crack 
//    at the message. 
//
//    ARGS:       pMsg -- [in] pending message to be processed.
// 
//    RETURNS:    TRUE if the message was handled, FALSE otherwise.
//
BOOL CSzDlgBarDlg::PreTranslateMessage( MSG* pMsg )
{
   // if the dlg is in a control bar parent, get that control bar's immediate parent frame, if any
   CWnd* pParent = GetParent();
   CFrameWnd* pFrame = NULL;
   if( pParent != NULL ) pFrame = pParent->GetParentFrame();

   // give the frame window hierarchy first crack at the message
   while( pFrame != NULL )
   {
      if( pFrame->PreTranslateMessage(pMsg) ) 
         return( TRUE );
      pFrame = pFrame->GetParentFrame();
   }

   return( CDialog::PreTranslateMessage( pMsg ) );
}


//=== SetupScrollbars, ResetScrollBars ================================================================================ 
//
//    Utilities for calculating the scrollbar status variables for the dialog, and for updating their visibility.
//
//    ARGS:       NONE. 
// 
//    RETURNS:    NONE
//
void CSzDlgBarDlg::SetupScrollbars()
{
   CRect tempRect;
   GetWindowRect( &tempRect );

   m_nHorzInc = (m_ClientRect.Width()  - tempRect.Width()) / HORZ_PTS;     // #scroll incr (neg = no scroll req'd!)  
   m_nHscrollMax = max( 0, m_nHorzInc );                                   // scroll between 0 and #scroll incr
   m_nHscrollPos = min( m_nHscrollPos, m_nHscrollMax );                    // adjust scroll bar pos so it's in range
   SetScrollRange( SB_HORZ, 0, m_nHscrollMax, FALSE );                     // range [0..0] ==> scroll bar is hidden!!
   SetScrollPos( SB_HORZ, m_nHscrollPos, TRUE );

   m_nVertInc = (m_ClientRect.Height() - tempRect.Height()) / VERT_PTS;    // analogously for vertical...
   m_nVscrollMax = max( 0, m_nVertInc ); 
   m_nVscrollPos = min( m_nVscrollPos, m_nVscrollMax );
   SetScrollRange( SB_VERT, 0, m_nVscrollMax, FALSE );
   SetScrollPos( SB_VERT, m_nVscrollPos, TRUE );

}

void CSzDlgBarDlg::ResetScrollbars()
{
   ScrollWindow( m_nHscrollPos*HORZ_PTS, 0, NULL, NULL );
   ScrollWindow( 0, m_nVscrollPos*VERT_PTS, NULL, NULL );
   m_nHscrollPos = 0;
   m_nVscrollPos = 0;
   SetScrollPos( SB_HORZ, m_nHscrollPos, TRUE );
   SetScrollPos( SB_VERT, m_nVscrollPos, TRUE );
}





//===================================================================================================================== 
// 
// Implementation of CSizingDialogBar
//
//===================================================================================================================== 

IMPLEMENT_DYNAMIC( CSizingDialogBar, CSizingControlBarCF )

BEGIN_MESSAGE_MAP( CSizingDialogBar, CSizingControlBarCF )
   ON_WM_CREATE()
END_MESSAGE_MAP()


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

int CSizingDialogBar::OnCreate( LPCREATESTRUCT lpcs ) 
{
   if( CSizingControlBarCF::OnCreate(lpcs) == -1 ) return( -1 );

   // set single child autosizing mode
   SetSCBStyle( GetSCBStyle() | SCBS_SIZECHILD );

   // create the child dialog
   ASSERT( m_rDlg.IsKindOf( RUNTIME_CLASS(CSzDlgBarDlg) ) );         // child dialog MUST be derived from CSzDlgBarDlg 
   if( !m_rDlg.Create( this ) ) return( -1 );

   ASSERT(::IsWindow(m_hWnd));
   ASSERT(::IsWindow(m_rDlg.m_hWnd));

   return( 0 );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== OnCmdMsg, OnCommand, OnUpdateCommandUI [base override] ========================================================== 
//
//    These methods are overridden to ensure command messages and command updates are routed to the child dialog hosed 
//    inside this control bar.
//
//    ARGS:       See CCmdTarget::OnCmdMsg, CWnd::OnCommand, and CControlBar::OnUpdateCmdUI. 
// 
//    RETURNS:    See CCmdTarget::OnCmdMsg, CWnd::OnCommand, and CControlBar::OnUpdateCmdUI. 
//
BOOL CSizingDialogBar::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )  
{
   if( (m_rDlg.GetSafeHwnd() != NULL) && m_rDlg.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
      return( TRUE );
   return( CSizingControlBarCF::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) );
}

BOOL CSizingDialogBar::OnCommand( WPARAM wParam, LPARAM lParam ) 
{
   if( (m_rDlg.GetSafeHwnd() != NULL) && m_rDlg.OnCommand(wParam, lParam) )
      return( TRUE );
   return( CSizingControlBarCF::OnCommand( wParam, lParam ) );
}

void CSizingDialogBar::OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler )
{
   CSizingControlBarCF::OnUpdateCmdUI( pTarget, bDisableIfNoHndler );
   if( m_rDlg.GetSafeHwnd() != NULL )
      m_rDlg.OnUpdateCmdUI( pTarget, bDisableIfNoHndler );
}





//===================================================================================================================== 
// 
// Implementation of CSizingTabDlgBar
//
//===================================================================================================================== 

IMPLEMENT_DYNAMIC( CSizingTabDlgBar, CSizingControlBarCF )

BEGIN_MESSAGE_MAP( CSizingTabDlgBar, CSizingControlBarCF )
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_NOTIFY(TCN_SELCHANGE, CSizingTabDlgBar::IDC_TABCTRL, OnTabSelChange)
   ON_WM_DRAWITEM()
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

CSizingTabDlgBar::CSizingTabDlgBar()
{
   m_nActiveTab = -1;
   m_pActiveDlg = NULL;
}


//=== ~CSizingTabDlgBar [destructor] ================================================================================== 
//
//    Here we destroy all the dialogs that were created and maintained within the dialog bar, the tab control, and the 
//    dialog bar itself.
//
CSizingTabDlgBar::~CSizingTabDlgBar()
{
   for( int i = 0; i < m_tabPages.GetSize(); i++ )
   {
      CTabPage* pPage = m_tabPages[i];
      pPage->pDlg->DestroyWindow();
      delete pPage->pDlg;
      delete pPage;
   }
   m_tabPages.RemoveAll();
   m_tabCtrl.DestroyWindow();
   DestroyWindow();
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnCreate [base override] ======================================================================================== 
//
//    Response to WM_CREATE message.  
//
//    Here's where the control bar and its children are created.  In this case, all we do is create the tab control 
//    and attempt to set its font to the default GUI font.  The tabbed dialogs are added later using AddDlg().  NOTE 
//    that the tab control is created with the "owner-drawn" style, so that we can display tab labels corresponding to 
//    disabled dialog pages using grayed-out text.  Also, the tab control is initially invisible -- it is only made 
//    visible when more than one dialog page are installed in the bar.
//
//    ARGS:       lpcs  -- [in] creation parameters
// 
//    RETURNS:    -1 to indicate failure; 0 to let creation proceed.
//
int CSizingTabDlgBar::OnCreate( LPCREATESTRUCT lpcs ) 
{
   if( CSizingControlBarCF::OnCreate(lpcs) == -1 ) return( -1 );     // let base class handle control bar creation...

   CRect rect;                                                       // create tab control (it will be sized later!)
   DWORD dwStyle = WS_CHILD|TCS_BOTTOM|TCS_OWNERDRAWFIXED;
   if( !m_tabCtrl.Create( dwStyle, rect, this, IDC_TABCTRL ) )
      return( -1 );

   CFont *pFont = CFont::FromHandle( (HFONT)::GetStockObject(DEFAULT_GUI_FONT) );
   if( pFont != NULL ) m_tabCtrl.SetFont( pFont );

   ASSERT( ::IsWindow(m_hWnd) );
   ASSERT( ::IsWindow(m_tabCtrl.m_hWnd) );
   return( 0 );
}


//=== OnSize [base override] ========================================================================================= 
//
//    Response to WM_SIZE message.  See Resize() for details.
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
void CSizingTabDlgBar::OnSize( UINT nType, int cx, int cy ) 
{
   CSizingControlBarCF::OnSize(nType, cx, cy);           // let base class handle control-bar-specific stuff
   Resize( cx, cy );
}


//=== OnTabSelChange ================================================================================================== 
//
//    Response to TCN_SELCHANGE notification from the embedded tab control:  update currently displayed dialog page. 
//    If the user has selected a disabled tab page, then restore the selection to the current active page -- disabled 
//    pages may not be selected.
//
//    ARGS:       pNMHDR   -- [in] handle to generic notification struct. 
//                pResult  -- [out] return code.  ignored for TCN_SELCHANGE. 
//
//    RETURNS:    NONE
//
void CSizingTabDlgBar::OnTabSelChange( NMHDR* pNMHDR, LRESULT* pResult)
{
   ASSERT( pNMHDR->idFrom == (UINT) CSizingTabDlgBar::IDC_TABCTRL );
   int iPg = MapTabPosToPagePos( m_tabCtrl.GetCurSel() );
   if( iPg >= 0 ) 
   {
      if( m_tabPages[iPg]->bEnabled ) SetActiveDlg( m_tabPages[iPg]->pDlg );
      else m_tabCtrl.SetCurSel( (m_nActiveTab>=0) ? m_tabPages[m_nActiveTab]->iTabIdx : -1 );
   }
}


//=== OnDrawItem ====================================================================================================== 
//
//    Response to WM_DRAWITEM message from the embedded "owner-drawn" tab control.
//
//    We must draw the tab items ourselves so that we can "gray out" the tab labels corresponding to disabled dialog 
//    pages.  NOTE that there is no support here for drawing an image on the tab.
//
//    ARGS:       nID   -- [in] ID of child control that sent WM_DRAWITEM.  this must be ID of the embedded tab ctrl!
//                lpdis -- [in] information about tab item to be drawn.
//
//    RETURNS:    NONE
//
void CSizingTabDlgBar::OnDrawItem( int nID, LPDRAWITEMSTRUCT lpdis ) 
{
   if( nID != CSizingTabDlgBar::IDC_TABCTRL ) return;                // message is not from the embedded tab control! 

   int nTabIdx = lpdis->itemID;                                      // verify that tab index corres to a visible page
   int nTabPage = MapTabPosToPagePos( nTabIdx );
   if( nTabPage == -1 ) return;

   char label[TABLABELSZ];                                           // retrieve tab label text 
   TC_ITEM tci;
   tci.mask = TCIF_TEXT;
   tci.pszText = label;
   tci.cchTextMax = TABLABELSZ-1;
   if( !m_tabCtrl.GetItem( nTabIdx, &tci ) ) return;

   CDC* pDC = CDC::FromHandle( lpdis->hDC );                         // get device context and save its current state 
   if( !pDC ) return;
   int nSavedDC = pDC->SaveDC();

   CRect rect = lpdis->rcItem;                                       // the bounding rect for tab item
   rect.top += ::GetSystemMetrics( SM_CYEDGE );                      // a correction (don't know why!)

   pDC->SetBkMode( TRANSPARENT );                                    // erase bounding rect 
   pDC->FillSolidRect( rect, ::GetSysColor(COLOR_BTNFACE) );

   rect.top -= ::GetSystemMetrics( SM_CYEDGE );                      // undo correction before drawing the text 

   COLORREF cr;                                                      // label color reflects enable state of assoc. 
   if( m_tabPages[nTabPage]->bEnabled )                              // dialog page
      cr = ::GetSysColor(COLOR_BTNTEXT);
   else 
      cr = ::GetSysColor( COLOR_GRAYTEXT );
 
   pDC->SetTextColor( cr );                                          // draw tab label        
   pDC->DrawText( label, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER );

   pDC->RestoreDC( nSavedDC );                                       // restore device context to original state 
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== AddDlg ========================================================================================================== 
//
//    Create a modeless dialog and add it to the tabbed dialog bar. 
//
//    CSizingTabDlgBar is designed to contain modeless dialogs that support dynamic object creation and are derived 
//    from CSzDlgBarDlg.  After checking that these constraints are satisfied by the specified runtime class, this 
//    method constructs the dialog object and creates the associated HWND using the CSzDlgBarDlg's protected Create().
//    The dialog is initially inaccessible.  To make it accessible, with a corresponding tab in the dialog bar's tab 
//    control, use ShowDlg().  Use the dlg ptr returned by AddDlg() to reference the dialog in calls to other methods.
//
//    ARGS:       lpszLabel   -- [in] text label to appear on associated tab of the tab control (limited text len!); 
//                pDlgClass   -- [in] the runtime class of the dialog to be created. 
//
//    RETURNS:    ptr to the new dialog object, if successful; -1 on failure. 
//
CSzDlgBarDlg* CSizingTabDlgBar::AddDlg( LPCTSTR lpszLabel, CRuntimeClass* pDlgClass )
{
   ASSERT( AfxIsValidAddress( pDlgClass, sizeof(CRuntimeClass), FALSE ) );
   ASSERT( pDlgClass->IsDerivedFrom( RUNTIME_CLASS(CSzDlgBarDlg) ) );

   CTabPage* pPage = NULL; 
   try
   {
      pPage = new CTabPage;                                    // construct the dialog page
      if( pPage == NULL ) AfxThrowMemoryException();
      pPage->pDlg = (CSzDlgBarDlg*) pDlgClass->CreateObject(); // construct the dialog object
      if( pPage->pDlg == NULL ) AfxThrowMemoryException();
      if( !pPage->pDlg->Create( this ) )                       // create the dialog's HWND 
      {
         delete pPage->pDlg;
         delete pPage;
         return( NULL );
      }
      m_tabPages.Add( pPage );                                 // append dialog page to the internal array 
   }
   catch( CException* e )
   {
      TRACE0( "CSizingTabDlgBar: Memory excp in AddDlg!\n" ); 
      e->Delete();
      if( (pPage != NULL) && (pPage->pDlg != NULL) )           // (exception occurred while adding dlg to page array!) 
      {
         pPage->pDlg->DestroyWindow();
         delete pPage->pDlg;
         delete pPage;
      }
      return( NULL );
   }

   pPage->bEnabled = TRUE;                                     // dialog page is initially enabled, 
   pPage->iTabIdx = -1;                                        // but it is invisible b/c there's no assoc tab at first
   
   INT_PTR nPages = m_tabPages.GetSize();                      // #dialog pages after the addition

   pPage->strTabTitle = lpszLabel;                             // remember tab label for the dlg's associated tab.  if 
   if( pPage->strTabTitle.IsEmpty() )                          // no label was provided, generate one.  limit label sz. 
      pPage->strTabTitle.Format( "Dialog %d", nPages-1 );
   else if( pPage->strTabTitle.GetLength() > TABLABELSZ )
      pPage->strTabTitle.Delete( TABLABELSZ-1, pPage->strTabTitle.GetLength() );

   return( pPage->pDlg );                                      // success: return ptr to the new dlg page
}


//=== RemoveDlg ======================================================================================================= 
//
//    Remove an existing dialog page from the tabbed dialog bar.  The dialog object and associated HWND are destroyed. 
//    If the dialog was currently part of the tabbed page list, its associated tab is first removed from the dialog 
//    bar's tab control and any necessary adjustments are made -- see HideDlg().
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object to be removed.
//
//    RETURNS:    NONE. 
//
void CSizingTabDlgBar::RemoveDlg( CSzDlgBarDlg* pDlg )
{
   int iPg = MapDlgToPagePos( pDlg );                                // find dlg page that holds specified dlg object
   if( iPg == -1 ) return;                                           // dialog is not installed here!

   if( m_tabPages[iPg]->iTabIdx >= 0 )                               // if dlg page is part of the tab list, we must 
      HideDlg( pDlg );                                               // hide it first

   CTabPage* pDeadPage = m_tabPages[iPg];                            // retrieve dialog page
   m_tabPages.RemoveAt( iPg );                                       // and remove it from internal array 

   pDeadPage->pDlg->DestroyWindow();                                 // destroy dialog page and the dlg obj it contains 
   delete pDeadPage->pDlg;
   delete pDeadPage;
}


//=== ShowDlg ========================================================================================================= 
//
//    Make specified dialog page accessible to the user.
//
//    A dialog page installed in a CSizingTabDlgBar is accessible to the user only if they are "shown"; ie, a tab is 
//    included in the bar's child tab control corresponding to that dialog.  This permits programmatic control of which 
//    dialogs are currently visible in the dialog bar while maintaining the state of invisible dialogs.
//
//    The "shown" page does NOT become the active dialog page, unless it is the only one in the tabbed page list.  Upon 
//    adding a second dialog to the bar, the tab control is made visible and a resize is generated to ensure all dialog 
//    bar children are appropriately sized. 
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object to be shown.
//                iPos  -- [in] zero-based insertion pos for the dialog's associated tab; if invalid, tab is appended.
//
//    RETURNS:    TRUE if a tab for the page was added to tab ctrl; FALSE otherwise (bad dlg ptr, or error occurred 
//                adding tab to the child tab control).
//
BOOL CSizingTabDlgBar::ShowDlg( CSzDlgBarDlg* pDlg, int iPos )
{
   ASSERT( m_tabCtrl.GetSafeHwnd() != NULL );
   int iPg = MapDlgToPagePos( pDlg );                                // find dlg page that holds specified dlg object
   if( iPg == -1 ) return( FALSE );                                  // dialog is not installed here!

   CTabPage* pPage = m_tabPages[iPg];                                // retrieve dialog page
   if( pPage->iTabIdx >= 0 ) return( TRUE );                         // dlg page is already part of the tab list!


   int nTabs = m_tabCtrl.GetItemCount();                             // #pages currently "tab-able"
   int iInsPos = iPos;                                               // desired insertion pos
   if( iInsPos < 0 || iInsPos >= nTabs ) iInsPos = nTabs;            // if invalid insert pos, we'll append the tab

   iInsPos = (int)m_tabCtrl.InsertItem(iInsPos, pPage->strTabTitle); // add labeled tab for dialog to the tab ctrl;
   if( iInsPos == -1 )                                               // abort if this fails
   {
      TRACE0( "CSizingTabDlgBar: Cannot add item to tab ctrl\n" ); 
      return( FALSE );
   }

   pPage->iTabIdx = iInsPos;                                         // success! rem pos of tab in the tab list
   ++nTabs;

   CRect rClient;                                                    // get dialog bar's current client rect 
   GetClientRect( &rClient ); 

   if( nTabs == 1 )                                                  // only one dlg is "tab-able": size == client 
   {                                                                 // rect, and it becomes the active dlg page
      pPage->pDlg->MoveWindow( &rClient );
      SetActiveDlg( pDlg );
   }
   else if( nTabs == 2 )                                             // now we have two dialog pages: reveal tab ctrl 
   {                                                                 // and resize all...
      ASSERT( !m_tabCtrl.IsWindowVisible() );
      m_tabCtrl.ShowWindow( SW_SHOW ); 
      Resize( rClient.right, rClient.bottom ); 
      if( m_pActiveDlg != NULL )                                     // if there's an active dlg...
      {
         m_tabCtrl.SetWindowPos( (CWnd*)m_pActiveDlg, 0,0,0,0,       //    make sure it precedes tab ctrl in z-order
                                 SWP_NOMOVE|SWP_NOSIZE ); 
         m_tabCtrl.SetCurSel( m_tabPages[m_nActiveTab]->iTabIdx );   //    and select corres. item in the tab ctrl 
      }
   }
   else                                                              // >2 pages:  size the dialog just added to tab 
   {                                                                 // list to fit inside tab ctrl's "display area". 
      m_tabCtrl.AdjustRect( FALSE, &rClient );
      pPage->pDlg->MoveWindow( &rClient );
   }

   if( iInsPos < nTabs - 1 )                                         // if dlg page tab was not inserted at the end of 
      for( int n = 0; n < m_tabPages.GetSize(); n++ )                // tab order, we must adjust tab positions of 
   {                                                                 // those tabs that came after the insertion pos... 
      if( n != iPg && m_tabPages[n]->iTabIdx >= iInsPos )
         ++(m_tabPages[n]->iTabIdx);
   }

   return( TRUE );
}


//=== HideDlg ========================================================================================================= 
//
//    Remove specified dialog page from the tab control, essentially making it inaccessible to the user.  The dialog 
//    page itself is NOT destroyed.  If no dialog specified, all accessible dialogs are hidden.
//
//    A dialog page installed in a CSizingTabDlgBar is accessible to the user only if they are "shown"; ie, a tab is 
//    included in the bar's child tab control corresponding to that dialog.  This permits programmatic control of which 
//    dialogs are currently visible in the dialog bar while maintaining the state of invisible dialogs.
//
//    If the page to be hidden was the active dialog page, we bring one of the remaining pages in front, if possible. 
//    If we are left with only a single dialog page, the child tab control is hidden.
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object to be hidden.  If NULL, all currently accessible dialogs are 
//                         hidden from view.
//
//    RETURNS:    TRUE if page was hidden; FALSE otherwise (invalid page ID).
//
BOOL CSizingTabDlgBar::HideDlg( CSzDlgBarDlg* pDlg )
{
   ASSERT( m_tabCtrl.GetSafeHwnd() != NULL );
   int iPg = MapDlgToPagePos( pDlg );                                // find dlg page that holds specified dlg object
   if( pDlg == NULL )                                                // no dlg specified -- hide all accessible dlgs!
   {
      SetActiveDlg( NULL );
      m_tabCtrl.ShowWindow( SW_HIDE );
      m_tabCtrl.DeleteAllItems();
      for( int n = 0; n < m_tabPages.GetSize(); n++ ) 
         m_tabPages[n]->iTabIdx = -1;
      return( TRUE );
   }
   else if( iPg == -1 ) return( FALSE );                             // specified dialog is not installed here!

   CTabPage* pPage = m_tabPages[iPg];                                // retrieve dialog page
   if( pPage->iTabIdx < 0 ) return( TRUE );                          // dlg page is already hidden!


   int nTabs = m_tabCtrl.GetItemCount();                             // #pages currently "tab-able"
   INT_PTR nPages = m_tabPages.GetSize();                            // #pages currently installed in dlg bar
   if( m_nActiveTab == iPg )                                         // we're hiding the current active page, so find 
   {                                                                 // an alternate dlg page to make active...
	   INT_PTR n;
      for( n = 0; n < nPages; n++ )
      {
         if( (n != m_nActiveTab) && 
             m_tabPages[n]->iTabIdx >= 0 &&                          //    the alternate page must be accessible AND 
             m_tabPages[n]->bEnabled )                               //    it must be enabled
            break;
      }
      SetActiveDlg( (n < nPages) ? m_tabPages[n]->pDlg : NULL );     //    we may not find one!!
   }

   int iHideTabIdx = pPage->iTabIdx;                                 // pos of tab corres. to dlg page to be hidden
   m_tabCtrl.DeleteItem( iHideTabIdx );                              // delete associated tab from tab control 
   pPage->iTabIdx = -1;
   --nTabs;

   if( nTabs == 1 )                                                  // only 1 dlg page visible:  hide tab ctrl and 
   {                                                                 // resize remaining dlg to fill bar's client area 
      m_tabCtrl.ShowWindow( SW_HIDE );
      Resize();
   }

   for( int n = 0; n < nPages; n++ )                                 // adjust the tab positions of those dlg pages 
   {                                                                 // that are still visible, if necessary...
      if( m_tabPages[n]->iTabIdx > iHideTabIdx ) 
         --(m_tabPages[n]->iTabIdx);
   }

   return( TRUE );
}


//=== EnableDlg ======================================================================================================= 
//
//    Enable or disable the specified dialog page.  If the dialog page is currently active and it is being disabled, we 
//    move the active focus to another enabled page.  If no such page exists, the "active page" is undefined and no 
//    dialog will appear in the tab dialog bar.
//
//    If the specified dialog page is currently hidden (ie, not part of the tab list), this method will still update 
//    its enable/disable state.
//
//    ARGS:       pDlg     -- [in] ptr to the dialog object to be enabled/disabled.
//                bEnable  -- [in] TRUE to enable, FALSE to disable.
//
//    RETURNS:    NONE. 
//
void CSizingTabDlgBar::EnableDlg( CSzDlgBarDlg* pDlg, BOOL bEnable )
{
   int iPg = MapDlgToPagePos( pDlg );                                // find dlg page that holds specified dlg object
   if( iPg == -1 ) return;                                           // dialog is not installed here!

   CTabPage* pPage = m_tabPages[iPg];                                // retrieve tab page 
   if( pPage->bEnabled == bEnable ) return;                          // there's nothing to do!

   INT_PTR nPages = m_tabPages.GetSize();                            // #pages installed

   pPage->bEnabled = bEnable;                                        // update enabled state of page 
   if( (m_nActiveTab == iPg) && !bEnable )                           // if active page is being disabled, we must look 
   {                                                                 // for an alternate page to bring to the front:
	   INT_PTR n;
      for( n = 0; n < nPages; n++ )
      {
         if( (n != m_nActiveTab) && 
             m_tabPages[n]->iTabIdx >= 0 &&                          //    the alternate page must be accessible AND 
             m_tabPages[n]->bEnabled )                               //    it must be enabled
            break;
      }
      SetActiveDlg( (n < nPages) ? m_tabPages[n]->pDlg : NULL );     //    we may not find one!!
   }

   if( pPage->iTabIdx >= 0 && GetNumVisibleTabs() > 1 )              // if dlg page is currently in tab list and the 
   {                                                                 // tab control is visible, then force a redraw of 
      CRect rTabItem;                                                // the corresponding tab
      m_tabCtrl.GetItemRect( pPage->iTabIdx, &rTabItem );
      m_tabCtrl.InvalidateRect( rTabItem );
   }
}


//=== SetActiveDlg ==================================================================================================== 
//
//    Change the "active dialog", ie, the dialog that is currently visible in the dialog bar.  Attempts to make a 
//    disabled or invisible (ie, not in the tab list) dialog page the active dialog are ignored.
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object to be made active.  if the dlg is currently disabled or hidden 
//                         (ie, not in the tab list), we do nothing.  if the dlg ptr is invalid, the active dialog 
//                         becomes undefined, and nothing appears in the dialog bar.
//
//    RETURNS:    NONE. 
//
void CSizingTabDlgBar::SetActiveDlg( CSzDlgBarDlg* pDlg )
{
   int iPg = MapDlgToPagePos( pDlg );                                      // find dlg page that holds specified dlg 
                                                                           // -1 ==> no active dialog!

   if( iPg == m_nActiveTab ) return;                                       // no change in active tab
   if( iPg >= 0 &&                                                         // specified page is currently not tabbable 
       (m_tabPages[iPg]->iTabIdx < 0 || !(m_tabPages[iPg]->bEnabled))      // or is currently disabled
      )
      return; 

   if( m_nActiveTab >= 0 )                                                 // hide the previously active dialog page 
   {
      ASSERT( m_pActiveDlg );
      m_pActiveDlg->SetWindowPos( NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW );
      m_pActiveDlg = NULL;
   }

   m_nActiveTab = iPg;                                                     // show the new active dialog page, if 
   if( m_nActiveTab == -1 ) return;                                        // there is one... 

   m_pActiveDlg = m_tabPages[m_nActiveTab]->pDlg;
   ASSERT( m_pActiveDlg );
   m_pActiveDlg->SetWindowPos( NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW );
   m_pActiveDlg->SetFocus();

   m_tabCtrl.SetWindowPos( (CWnd*)m_pActiveDlg, 0,0,0,0,                   // active dlg MUST precede tab ctrl in
                           SWP_NOMOVE|SWP_NOSIZE );                        // z-order for proper repainting 
   m_tabCtrl.SetCurSel( m_tabPages[m_nActiveTab]->iTabIdx );               // make sure tab control is up-to-date
}


//=== IsEnabledDlg, IsVisibleDlg, GetDlgTabPos ======================================================================== 
//
//    Retrieve state information about a specified dialog page installed in the tabbed dialog bar.  
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object.
//
//    RETURNS:    IsEnabledDlg() returns TRUE if the dialog page is enabled, IsVisibleDlg() returns TRUE if the dialog 
//                page is accessible via the embedded tab control, and GetDlgTabPos() returns the zero-based position 
//                of the tab corresponding to the dialog page (or -1 if the page is currently inaccessible).
//
BOOL CSizingTabDlgBar::IsEnabledDlg( CSzDlgBarDlg* pDlg )
{
   int iPg = MapDlgToPagePos( pDlg );
   return( (iPg == -1) ? FALSE : m_tabPages[iPg]->bEnabled );
}

BOOL CSizingTabDlgBar::IsVisibleDlg( CSzDlgBarDlg* pDlg )
{

   return( BOOL(GetDlgTabPos( pDlg ) >= 0) );
}
   
int CSizingTabDlgBar::GetDlgTabPos( CSzDlgBarDlg* pDlg )
{
   int iPg = MapDlgToPagePos( pDlg );
   return( (iPg == -1) ? -1 : m_tabPages[iPg]->iTabIdx );
}


//=== GetNumTabs, GetNumVisibleTabs =================================================================================== 
//
//    Retrieve various attributes of the tabbed dialog bar.  
//
//    ARGS:       NONE.
//
//    RETURNS:    GetNumTabs() returns #dialog pages currently installed in dialog bar, while GetNumVisibleTabs() 
//                returns the # of pages which are accessible via the tab control. (There will never be very many tabs,
//                so it is safe to cast INT_PTR to int here.)
//
int CSizingTabDlgBar::GetNumTabs() { return( static_cast<int>(m_tabPages.GetSize()) ); }

int CSizingTabDlgBar::GetNumVisibleTabs()
{
   int n = 0;
   for( int i = 0; i < m_tabPages.GetSize(); i++ ) if( m_tabPages[i]->iTabIdx >= 0 ) ++n;
   return( n );
}


//=== GetDlg ========================================================================================================== 
//
//    Retrieve ptr to the dialog object installed on the specified dialog page.  Primarily for enumerating all dialogs 
//    currently installed.
//
//    ARGS:       iPage -- [in] page pos of requested dialog object.
//
//    RETURNS:    ptr of requested dialog, or NULL if page position is invalid.
//
CSzDlgBarDlg* CSizingTabDlgBar::GetDlg( int iPage )
{
   return( (iPage < 0 || iPage >= m_tabPages.GetSize()) ? NULL : m_tabPages[iPage]->pDlg );
}


//=== GetDlgByClass =================================================================================================== 
//
//    Retrieve ptr to the first instance of a dialog object with the specified runtime class. 
//
//    ARGS:       pClass   -- [in] the runtime class of the dialog requested. 
//
//    RETURNS:    ptr to first dialog (in page order) having the specified runtime class, or NULL if none is found.
//
CSzDlgBarDlg* CSizingTabDlgBar::GetDlgByClass( CRuntimeClass* pClass )
{
   CSzDlgBarDlg* pDlg = NULL;
   for( int i = 0; i < m_tabPages.GetSize(); i++ )
   {
      if( m_tabPages[i]->pDlg->IsKindOf( pClass ) ) { pDlg = m_tabPages[i]->pDlg; break; }
   }
   return( pDlg );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== OnCmdMsg, OnCommand, OnUpdateCommandUI [base override] ========================================================== 
//
//    These methods are overridden to ensure command messages and command updates are routed to the child dialog that 
//    is currently active in this tabbed dialog bar.
//
//    ARGS:       See CCmdTarget::OnCmdMsg, CWnd::OnCommand, and CControlBar::OnUpdateCmdUI. 
// 
//    RETURNS:    See CCmdTarget::OnCmdMsg, CWnd::OnCommand, and CControlBar::OnUpdateCmdUI. 
//
BOOL CSizingTabDlgBar::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )  
{
   if( (m_pActiveDlg != NULL) && (m_pActiveDlg->GetSafeHwnd() != NULL) &&
       m_pActiveDlg->OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) )
      return( TRUE );
   else
      return( CSizingControlBarCF::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) );
}

BOOL CSizingTabDlgBar::OnCommand( WPARAM wParam, LPARAM lParam ) 
{
   if( (m_pActiveDlg != NULL) && (m_pActiveDlg->GetSafeHwnd() != NULL) && m_pActiveDlg->OnCommand( wParam, lParam ) )
      return( TRUE );
   else
      return( CSizingControlBarCF::OnCommand( wParam, lParam ) );
}

void CSizingTabDlgBar::OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler )
{
   CSizingControlBarCF::OnUpdateCmdUI( pTarget, bDisableIfNoHndler );
   if( (m_pActiveDlg != NULL) && (m_pActiveDlg->GetSafeHwnd() != NULL) )
      m_pActiveDlg->OnUpdateCmdUI( pTarget, bDisableIfNoHndler );
}


//=== MapDlgToPagePos ================================================================================================= 
//
//    Find the zero-based page pos of the installed dialog page that holds the specified dialog object.
//
//    ARGS:       pDlg  -- [in] ptr to the dialog object.
// 
//    RETURNS:    page pos of the requested dialog page; or -1 if specified dialog is not installed.
//
int CSizingTabDlgBar::MapDlgToPagePos( CSzDlgBarDlg* pDlg )
{
   int iPage = -1;                                                            // search list of dlg pages until we 
   for( int i = 0; i < m_tabPages.GetSize(); i++ )                            // find one with matching ID...
   {
      if( m_tabPages[i]->pDlg == pDlg ) { iPage = i; break; }
   }
   return( iPage );
}


//=== MapTabPosToPagePos ============================================================================================== 
//
//    Find the zero-based page pos of the installed dialog page corresponding to specified position in the tab control.
//
//    ARGS:       iTabIdx  -- [in] zero-based position of a tab in the dialog bar's tab control.
// 
//    RETURNS:    page pos of the corresponding installed dialog page; or -1 if tab position is invalid.
//
int CSizingTabDlgBar::MapTabPosToPagePos( int iTabIdx )
{
   if( iTabIdx < 0 || iTabIdx >= m_tabCtrl.GetItemCount() ) return( -1 );     // invalid tab position

   int iPage = -1;                                                            // search list of dlg pages until we 
   for( int i = 0; i < m_tabPages.GetSize(); i++ )                            // find one with matching tab pos...
   {
      if( m_tabPages[i]->iTabIdx == iTabIdx ) { iPage = i; break; }
   }
   return( iPage );
}


//=== Resize ========================================================================================================== 
//
//    When the control bar is resized, we need to resize all of its children appropriately.  When there is more than 
//    one tab page, the tab control is first adjusted to fill the control bar's client area, while the individual 
//    dialogs (which are children of the control bar, NOT the tab control!) are adjusted to fit inside the tab 
//    control's "display area".  If only one dialog page is installed, the tab control is invisible, and the single 
//    dialog fills the control bar's client area.
//
//    ARGS:       cx,cy -- [in] width and height of dialog bar's client rect.  if either < 0, both are ignored and we 
//                         retrieve the dialog bar's client rect directly [default = -1].
// 
//    RETURNS:    NONE.
//
void CSizingTabDlgBar::Resize( int cx /* = -1 */, int cy /* = -1 */ )
{
   int nVisibleDlgs = GetNumVisibleTabs();               // if no visible dialog pages, then there's nothing to resize! 
   if( nVisibleDlgs <= 0 ) return; 

   CRect rect( 0, 0, cx, cy );                           // the control bar's client rect
   if( cx < 0 || cy < 0 ) GetClientRect( &rect );

   if( nVisibleDlgs > 1 )                                // tab control visible only when there's >1 dialog pages!
   {
      m_tabCtrl.MoveWindow( &rect );                     //    tab control fills client rect
      m_tabCtrl.AdjustRect( FALSE, &rect );              //    calculate new display area for the tabbed dialogs
   }

   for( int i = 0; i < m_tabPages.GetSize(); i++ )       // adjust the visible dialogs to fit inside new display area; 
      if( m_tabPages[i]->iTabIdx >= 0 )                  // if just one dlg, display area == bar's client area!
         m_tabPages[i]->pDlg->MoveWindow( &rect );
}

