//===================================================================================================================== 
//
// inplacecombo.cpp : Implementation of class CInPlaceCombo, a dropdown list control specialized for "on-demand, in 
//                    place" modification of a parameter with a relatively short list of possible values.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CInPlaceCombo is designed for use as a sort of popup inplace CComboBox that can be used to choose among a finite 
// selection of fixed choices.  Unlike a typical combo box control, this control pops up on top of a parent window at 
// specified coordinates, and is extinquished as soon as it loses the input focus or the user presses certain keys. 
// Such a control would be useful to edit information in list/table/grid controls.
//
// ==> Usage.
// 1) Construct an instance, specifying the "owner" window that will receive the NM_KILLFOCUS notification when the 
// inplace combo box is extinguished.  The constructor creates the combo box HWND in an invisible state, so don't call 
// Create() on this control.  NOTE that CInPlaceCombo only supports the drop-down list (CBS_DROPDOWNLIST) style; any 
// attempt to modify this style will be rejected.
// 2) To initiate the inplace edit, invoke BeginEdit() with the following "configuration" attributes:
//    parent window  :  The window above which the control will sit.  Depending on the situation, this window may or 
//       may not be the same as the owner.
//    list values    :  A CStringArray describing the contents of the combo list, and a zero-based index indicating 
//       which item in the array should be initially selected.
//    control rect   :  A CRect indicating the bounds of the control in the parent window's client coords.
// 3) BeginEdit() displays the combo box at the specified location and obtains the input focus.  It handles keyboard 
// and mouse input as the user modifies the selection in the combo box.  It hides itself upon losing the keyboard focus 
// or when the user presses certain keys: ESC, RETURN, TAB, or CTRL+arrow key (the arrow keys alone navigate within the 
// drop down list) -- known as "exit characters"  The control's owner is then notified with NM_KILLFOCUS. 
// 4) The owner window should provide a handler for the NM_KILLFOCUS notification.  This handler can retrieve the new 
// zero-based index of the user's selection via CInPlaceCombo::GetCurrentSelection(), as well as the exit character (if 
// any) which extinguished the control (CInPlaceCombo::GetExitChar()).  It can also check if the user changed the 
// initial selection (CInPlaceCombo::IsChanged()).  !!CAVEAT!!  Do NOT use CComboBox::GetCurSel() to retrieve the 
// user's final choice.  Despite the fact that we programmatically set the initial selection when editing begins, this 
// method will return -1 (no selection) if the user extinguishes the control without changing the selection!!!
//
// ==> Notes.
// 1) We only allow the CBS_DROPDOWNLIST style.  The other two combo box styles include a child edit control.  When 
// that control gets the focus, the combo box parent loses the focus -- interfering with our usage of WM_KILLFOCUS to 
// extinguish the inplace combo box.  We could get around it by subclassing the child edit control, but it's not worth 
// the effort -- since CInPlaceCombo is intended to present a finite-set of fixed choices.
// 2) We had to reflect the CBN_CLOSEUP notification to handle the case in which a mousedown outside the list box of 
// the combo causes the list box to close up, while the combo box retains the focus.  There were two separate problems 
// with the default behavior.  See OnCloseUp().
//
// CREDITS:
// 1) Article by Chris Maunder [07/30/2000, www.codeproject.com/miscctrl/gridctrl_combo.asp] -- Using Combo Boxes in 
// the MFC Grid Control.  CInPlaceCombo's implementation is based on Maunder's CInPlaceList control, as well as lessons 
// learned from testing/debugging our own CInPlaceNumEdit control (which was based on Maunder's CInPlaceEdit!).
//
//
// REVISION HISTORY:
// 19apr2002-- Began development, based on design of CInPlaceNumEdit control class.
// 24apr2002-- First draft done.  Need to build & test.
// 01may2002-- First version done and appears to be working correctly.
// 30oct2002-- Handle VK_HOME and VK_END in same way as arrow keys.  Added IsChanged().  Cosmetic changes.
// 04dec2002-- Added GetCurrentSelection() to fix bug in baseline combo box implementation:  GetCurSel() returns -1 if 
//             the user did not make a selection prior to extinguishing the control -- even though we programmatically set 
//             the initial selection in BeginEdit()!
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "inplacecombo.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CInPlaceCombo, CComboBox)
   ON_WM_KILLFOCUS()
   ON_WM_CHAR()
   ON_WM_KEYDOWN()
   ON_CONTROL_REFLECT( CBN_CLOSEUP, OnCloseUp )
   ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
const int   CInPlaceCombo::MAX_LINES_IN_DROPLIST   = 7;



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CInPlaceCombo [constructor] ===================================================================================== 
//
//    Create the combo box, initially as a child of the specified "owner" window, and initially invisible.  We only 
//    allow the CBS_DROPDOWNLIST style, as this control is intended only to present a short list of **fixed** options.
//
//    ARGS:    pOwner   -- [in] the window object which "owns" the inplace combo box.
//             dwStyle  -- [in] window and combo-box specific style flags
//             nID      -- [in] child window ID assigned to this control
//
CInPlaceCombo::CInPlaceCombo( CWnd* pOwner, DWORD dwStyle, UINT nID )
{
   m_nExitChar = 0;                                                     // initializations
   m_initialSel = -1;
   m_bRestoreParent = FALSE;
   m_bAlreadyEnding = FALSE;

   if( (pOwner == NULL) ||                                              // we require a valid owner window
       !pOwner->IsKindOf(RUNTIME_CLASS(CWnd)) 
     ) 
   { 
      ASSERT( FALSE ); 
      TRACE0( "\nCInPlaceCombo requires valid owner wnd!" );
      return;
   }

   DWORD dwMyStyle = dwStyle | WS_BORDER|WS_CHILD|CBS_DROPDOWNLIST;     // create the HWND: must be a child, w/ border, 
   dwMyStyle &= ~WS_VISIBLE;                                            // dropdown list style, invisible initially
   if(!Create(dwMyStyle, CRect(0, 0, 0, 0), pOwner, nID))
   {
      TRACE0("\nCInPlaceCombo HWND creation failed!");
   }
}


//=== ~CInPlaceCombo [destructor] ===================================================================================== 
//
//    Since we create HWND in constructor, we make sure it's destroyed here.
//
CInPlaceCombo::~CInPlaceCombo()
{
   DestroyWindow(); 
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnKillFocus [base override] ===================================================================================== 
//
//    WM_KILLFOCUS message handler.  Here's where we extinguish the inplace control. 
//
//    ARGS:       pNewWnd  -- [in] the window that's receiving the focus.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::OnKillFocus( CWnd* pNewWnd )
{
   CComboBox::OnKillFocus( pNewWnd );
   EndEdit();
}


//=== OnChar [base override] ========================================================================================== 
//
//    WM_CHAR message handler.  Extinguishes inplace combo box when the TAB, RETURN, or ESC key is pressed. 
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnChar.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
   if( nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE )    // end editing 
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                                          // ultimately hides ctrl via OnKillFocus() 
      return;
   }

   CComboBox::OnChar(nChar, nRepCnt, nFlags);                           // else let base class handle
}


//=== OnKeyDown [base override] ======================================================================================= 
//
//    WM_KEYDOWN message handler.
//
//    The Windows combo box control uses "navigational" keys -- the arrow keys, "PageUp", "PageDn", "Home", "End" -- to 
//    navigate through the list of available choices in the control.  If the CTRL key is held down when one of these 
//    keys is depressed, then they serve as an "exit character" for our inplace combo box:  the control is extinguished 
//    and the parent window (eg, a grid or list view) can use the exit character as an implied direction for navigating 
//    to the next control to be edited.   OnKeyDown() implements this extended functionality.
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnKeyDown.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
   if( (nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_HOME || nChar == VK_END ||
        nChar == VK_DOWN  || nChar == VK_UP   || nChar == VK_RIGHT || nChar == VK_LEFT) &&
       (GetKeyState(VK_CONTROL) < 0) )
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                        // this ultimately hides the control via OnKillFocus()
      return;
   }

   CComboBox::OnKeyDown(nChar, nRepCnt, nFlags);
}


//=== OnCloseUp [base override] ======================================================================================= 
//
//    Handles reflection of the CBN_CLOSEUP notification.  This handler fixes a couple of undesirable features in the 
//    default behavior of the combo box.  When the droplist is down, the combo box has the mouse captured (so the list 
//    caret is updated as the mouse moves within the list).  If the user mousedowns somewhere outside the droplist 
//    window, the default behavior is to hide the droplist but keep the focus.  In addition, IF the user has not yet 
//    made a selection from the drop list, the "current selection" is undefined -- and so the static text field showing 
//    the current selection becomes blank.  This latter effect happens despite the fact that we programmtically set the 
//    current selection in BeginEdit().  These default behaviors are overridden here:
//       1) If the mousedown that closed the list box occurred entirely outside the combo box control, the control is 
//    extinguished.  This ends -- but does NOT cancel -- the inplace operation.
//       2) If the mousedown that closed the list box occurred somewhere inside the combo box, we check to make sure 
//    that the current selection is defined.  If it is not, we set it to the initial selection -- indicating that the 
//    user has not yet changed the selection!
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::OnCloseUp()
{
   CPoint p;
   ::GetCursorPos( &p );
   GetParent()->ScreenToClient( &p );                       // rem: we saved control rect in parent window's coords 
   if( !m_rect.PtInRect( p ) ) GetParent()->SetFocus();
   else if( GetCurSel() == -1 ) SetCurSel( m_initialSel );
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== BeginEdit ======================================================================================================= 
//
//    Configure combo box and initiate inplace operation.
//
//    The specified control rectangle is interpreted in two ways:  (1) the rect for the entire combo box, including 
//    drop down list (no resizing is performed); or (2) the rect for only the combo box itself.  In the latter case, 
//    the rect is extended down to make room for up to MAX_LINES_IN_DROPLIST entries in the dropdown list (plus a 
//    horizontal scroll bar).  The control initially appears with the dropdown list shown.
//
//    NOTE:  "Owner" vs "parent" window.  By design, the window passed in the constructor is considered the "owner" 
//    window of the inplace control.  It is also the control's initial parent.  So that inplace control can be used in 
//    a dialog (or form view) to perform inplace editing of a "sibling" dialog control, we allow the control to be 
//    temporarily assigned a different parent.  By making the "sibling" control the parent of the inplace edit ctrl, 
//    we ensure that it does not get drawn over (I tried WM_CLIPSIBLINGS, etc., to no avail!).  The owner window, 
//    however, still receives the NM_KILLFOCUS notification when the inplace control hides itself...
//
//    ARGS:       pParent  -- [in] parent window (if NULL, owner window is taken as parent).
//                rect     -- [in] control rect, in client coords of parent window. 
//                bNoResize-- [in] if FALSE, rect specifies bounds of combo box only; else, it is interpreted as the 
//                            control rect for the entire combo box, incl dropdown list.
//                values   -- [in] contents of the combo box (in order of appearance).  must not be empty.
//                iSel     -- [in] zero-based index indicating position of the next selection; if invalid, the first 
//                            item in the combo box will be initially selected.
//
//    RETURNS:    TRUE if inplace control was successfully initiated; FALSE otherwise.
//
BOOL CInPlaceCombo::BeginEdit( CWnd* pParent, const CRect& rect, BOOL bNoResize, const CStringArray& values, int iSel ) 
{
   int i;

   if( GetSafeHwnd() == NULL ) return( FALSE );                // no HWND; can't do anything! 
   if( IsWindowVisible() ) return( FALSE );                    // if window already visible, can't begin editing!
   if( values.GetSize() < 1 ) return( FALSE );                 // there are no choices!!

   if( pParent != NULL )                                       // a parent wnd supplied...
   { 
      ASSERT_KINDOF( CWnd, pParent );
      CWnd* pOwner = GetOwner();                               // if different from owner window, make it the temporary 
      if( pOwner->GetSafeHwnd() != pParent->GetSafeHwnd() )    // parent:
      {
         SetParent( pParent );                                 //    this switches owner as well, 
         SetOwner( pOwner );                                   //    ...so we have to reset the original owner
         m_bRestoreParent = TRUE;
      }
   }

   SetFont( GetParent()->GetFont() );                          // font same as that of parent window

   ResetContent();                                             // initialize content of drop list
   for( i = 0; i < values.GetSize(); i++ )
      AddString( values[i] );

   m_rect = rect;
   if( !bNoResize )                                            // control rect is just for combo box itself; make room 
   {                                                           // for dropdown list, etc...
      int nHeight = m_rect.Height();                           //    height of the static text box
      SetItemHeight( -1, nHeight );

      // adjust rect to account for droplist box and horizontal scroll bar in droplist
      int n = static_cast<int>(values.GetSize()); 
      i = (n < MAX_LINES_IN_DROPLIST) ? n : MAX_LINES_IN_DROPLIST;
      m_rect.bottom = m_rect.bottom + i*nHeight +  ::GetSystemMetrics(SM_CYHSCROLL); 
   }

   SetWindowPos( NULL, m_rect.left, m_rect.top,                // set window size, pos; activate & show it
            m_rect.Width(), m_rect.Height(), SWP_SHOWWINDOW );
   SetFocus();                                                 // grab the focus 
   ShowDropDown();                                             // display the drop down list immediately

   if( iSel < 0 || iSel >= values.GetSize() ) iSel = 0;        // make the initial selection
   SetCurSel( iSel );
   m_initialSel = iSel;                                        // to fix undesirable default behavior; see OnCloseUp() 

   m_nExitChar = 0;                                            // reset exit character
   return( TRUE );
}


//=== CancelEdit ====================================================================================================== 
//
//    Cancel an ongoing inplace operation, hiding the combo box (EndEdit() without the owner notification!).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::CancelEdit()
{
   if( m_bAlreadyEnding ) return;                                       // prevent reentrancy
   m_bAlreadyEnding = TRUE;

   ShowWindow( SW_HIDE );                                               // hide the window 

   CWnd* pOwner = GetOwner();                                           // the window's owner 
   ASSERT( pOwner );

   if( m_bRestoreParent )                                               // if necessary, restore owner as wnd's parent 
   {
      SetParent( pOwner );
      m_bRestoreParent = FALSE;
   }

   m_bAlreadyEnding = FALSE;
}


//=== GetCurrentSelection, IsChanged ================================================================================== 
//
//    These methods check the current selection in the combo box (whether extinguished or not).  GetCurrentSelection() 
//    corrects a problem with the baseline method CComboBox::GetCurSel(), which returns -1 until the user makes a 
//    selection (even though we *programmatically* set the selection in BeginEdit()!!).  IsChanged() returns TRUE if 
//    the current selection is different from the initial selection.
//
//    ARGS:       NONE.
//
//    RETURNS:    GetCurrentSelection():  zero-base index of item currently selected in combo box (never -1!).
//                IsChanged():  TRUE is the current selection has changed.
//
int CInPlaceCombo::GetCurrentSelection()
{
   int iSel = CComboBox::GetCurSel();
   return( (iSel<0) ? m_initialSel : iSel );
}

BOOL CInPlaceCombo::IsChanged() { return( BOOL(m_initialSel != GetCurrentSelection()) ); }


//=== PreTranslateMessage [base override] ============================================================================= 
//
//    This is a workaround for a Win95 accelerator key problem.  (It was present in Maunder's CInPlaceEdit class; I 
//    kept it as is.)
//
//    ARGS:       pMsg  -- [in] the message to be processed.
//
//    RETURNS:    TRUE if message was translated and should not be dispatched; FALSE otherwise. 
//
BOOL CInPlaceCombo::PreTranslateMessage( MSG* pMsg ) 
{
   // Stupid win95 accelerator key problem workaround - Matt Weagle.
   // Catch the Alt key so we don't choke if focus is going to an owner drawn button
   if( pMsg->message == WM_SYSCHAR ) return( TRUE );

   return( CWnd::PreTranslateMessage( pMsg ) );
}


//=== ModifyStyle [base override] ===================================================================================== 
//
//    Override to enforce certain restrictions on the combo box styles that may be used with this inplace version.
//
//    ARGS:       see CComboBox::ModifyStyle().
//
//    RETURNS:    TRUE if style was successfully modified; FALSE otherwise.
//
BOOL CInPlaceCombo::ModifyStyle( DWORD dwRemove, DWORD dwAdd, UINT nFlags /* =0 */ )
{
   if( 0 != (dwRemove & (WS_BORDER|WS_CHILD|CBS_DROPDOWNLIST)) ) return( FALSE );
   if( 0 != (dwAdd & (CBS_SIMPLE|CBS_DROPDOWN)) ) return( FALSE );
   return( CComboBox::ModifyStyle( dwRemove, dwAdd, nFlags ) );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== EndEdit ========================================================================================================= 
//
//    Terminate the inplace operation.  We hide the HWND, restore the owner as the inplace ctrl's parent (if needed), 
//    and then notify the owner via NM_KILLFOCUS.
//
//    See also: BeginEdit().
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceCombo::EndEdit()
{
   if( m_bAlreadyEnding ) return;                                       // prevent reentrancy
   m_bAlreadyEnding = TRUE;

   ShowWindow( SW_HIDE );                                               // hide the window 

   CWnd* pOwner = GetOwner();                                           // the window's owner 
   ASSERT( pOwner );

   if( m_bRestoreParent )                                               // if necessary, restore owner as wnd's parent 
   {
      SetParent( pOwner );
      m_bRestoreParent = FALSE;
   }

   NMHDR nmhdr;                                                         // send notification to owner
   nmhdr.hwndFrom = GetSafeHwnd();
   nmhdr.idFrom   = GetDlgCtrlID();
   nmhdr.code     = NM_KILLFOCUS;
   pOwner->SendMessage( WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmhdr );

   m_bAlreadyEnding = FALSE;
}
