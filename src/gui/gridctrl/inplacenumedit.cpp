//===================================================================================================================== 
//
// inplacenumedit.cpp : Implementation of class CInPlaceNumEdit, a numeric-edit control designed for inplace editing.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CInPlaceNumEdit is designed for use as a sort of popup inplace edit control that can be used to edit numeric entries 
// in dialogs, grid-like controls, list controls, etc.  It is derived from the numeric-only, format-restricted edit 
// control, CNumEdit.
//
// ==> Usage.
// (1) Construct an instance, specifying the "owner" window that will receive the NM_KILLFOCUS notification when the 
// inplace edit is extinguished.  The constructor creates the edit control window in an invisible state, so don't call
// Create() on this control.
// (2) To initiate the inplace edit, call BeginEdit(), specifying the edit control's parent window and window rect, the 
// initial numeric value to display, and numeric format constraints.
// (3) BeginEdit() displays the edit ctrl at the specified location, obtains the input focus, and displays the numeric 
// value (initially selected).  It then handles keyboard and mouse input from the user to change the value, restricting 
// input IAW the specified format constraints.  As with any typical inplace-edit control, it hides itself when it loses 
// the keyboard focus or the user presses certain keys:  ESC, RETURN, TAB, or CTRL+arrow key (the arrow keys alone are 
// used to navigate within the edit control itself).  The control's owner is then notified with NM_KILLFOCUS. 
// (4) The owner window should provide a handler for the NM_KILLFOCUS notification.  This handler can retrieve the new 
// numeric value (via base class CNumEdit methods), as well as the exit character (if any) which extinguished the edit 
// control (CInPlaceNumEdit::GetExitChar()).  It can also check whether or not the user changed the initial selection 
// (CInPlaceNumEdit::IsChanged()).
//
//
// CREDITS:
// 1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21. 
// CInPlaceNumEdit's implementation is based on Maunder's CInPlaceEdit control, which is included in the grid control 
// source code.
//
//
// REVISION HISTORY:
// 09jan2001-- Adapted Maunder's CInPlaceEdit as a numeric-only in-place edit control for more general-purpose use.
// 02may2001-- Modified BeginEdit and EndEdit to permit a temporary change in the control's parent.  While visible, 
//             it's important that the control be a child of the window it's sitting on top of; otherwise, I've found 
//             that it gets drawn over -- and i could not come up with any other way to prevent that from happening!
//             The control's "owner" and initial parent is always the window that was passed in the constructor.
//             BeginEdit() switches to the temporary parent (if one is provided), and EndEdit() switches back.  The 
//             critical NM_KILLFOCUS notification that indicates that the inplace edit control has extinguished is 
//             always sent to the owner!
// 10may2001-- Added CancelEdit() for programmed cancellation; EndEdit() is now protected.
// 30oct2002-- Added IsChanged().
// 11dec2002-- Modified BeginEdit() to optionally accept a first character that immediately replaces the initial 
//             numeric text -- if the character is valid (to support the initiation of inplace editing via a keypress). 
//
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "inplacenumedit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CInPlaceNumEdit, CNumEdit)
   ON_WM_KILLFOCUS()
   ON_WM_CHAR()
   ON_WM_KEYDOWN()
   ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CInPlaceNumEdit [constructor] =================================================================================== 
//
//    Create the edit control, initially as a child of the specified "owner" window, and initially invisible.
//
//    ARGS:    pOwner   -- [in] the window object which "owns" the inplace edit control.
//             dwStyle  -- [in] window style.
//             nID      -- [in] child window ID. 
//
CInPlaceNumEdit::CInPlaceNumEdit( CWnd* pOwner, DWORD dwStyle, UINT nID )
{
   m_nExitChar = 0;                                                     // initializations
   m_Rect.SetRect( 0, 0, 100, 30 );                                     
   m_bRestoreParent = FALSE;
   m_bAlreadyEnding = FALSE;

   if( (pOwner == NULL) ||                                              // we require a valid owner window
       !pOwner->IsKindOf(RUNTIME_CLASS(CWnd)) 
     ) 
   { 
      ASSERT( FALSE ); 
      TRACE0( "\nCInPlaceNumEdit requires valid owner wnd!" );
      return;
   }

   DWORD dwEditStyle = WS_BORDER|WS_CHILD|ES_AUTOHSCROLL | dwStyle;     // create the HWND: must be a child, w/ border 
   dwEditStyle &= ~WS_VISIBLE;                                          // and autohscroll, invisible initially
   if(!Create(dwEditStyle, m_Rect, pOwner, nID))
   {
      TRACE0("\nCInPlaceNumEdit HWND creation failed!");
   }
}


//=== ~CInPlaceNumEdit [destructor] =================================================================================== 
//
//    Since we create HWND in constructor, we make sure it's destroyed here.
//
CInPlaceNumEdit::~CInPlaceNumEdit()
{
   DestroyWindow(); 
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnKeyDown [base override] ======================================================================================= 
//
//    WM_KEYDOWN message handler. 
//
//    The Windows edit control uses "navigational" keys -- the arrow keys, "PageUp", "PageDn", "Home", "End" -- to 
//    navigate through characters in the control.  If the CTRL key is held down when one of these keys is depressed, 
//    then they serve as an "exit character" for our inplace edit ctrl:  the control is extinguished and the parent 
//    window (eg, a grid or list view) can use the exit character as an implied direction for navigating to the next 
//    control to be edited.   OnKeyDown() implements this extended functionality.
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnKeyDown.
//
//    RETURNS:    NONE
//
void CInPlaceNumEdit::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
   if( (nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_HOME || nChar == VK_END ||
        nChar == VK_DOWN  || nChar == VK_UP   || nChar == VK_RIGHT || nChar == VK_LEFT) &&
       (GetKeyState(VK_CONTROL) < 0) )
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                        // this ultimately hides the control via OnKillFocus()
      return;
   }

   CNumEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


//=== OnKillFocus [base override] ===================================================================================== 
//
//    WM_KILLFOCUS message handler.  Here's where we extinguish the inplace edit control. 
//
//    ARGS:       pNewWnd  -- [in] the window that's receiving the focus.
//
//    RETURNS:    NONE
//
void CInPlaceNumEdit::OnKillFocus( CWnd* pNewWnd )
{
   CNumEdit::OnKillFocus( pNewWnd );
   EndEdit();
}


//=== OnChar [base override] ========================================================================================== 
//
//    WM_CHAR message handler.  Extinguishes inplace edit when the TAB, RETURN, or ESC key is pressed.  Otherwise, we 
//    allow the control rect to be extended to make room for additional characters. 
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnChar.
//
//    RETURNS:    NONE
//
void CInPlaceNumEdit::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
   if( nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE )    // end editing 
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                                          // ultimately hides ctrl via OnKillFocus() 
      return;
   }

   CNumEdit::OnChar(nChar, nRepCnt, nFlags);                            // else let base class handle

   CString str;                                                         // resize edit control if needed to make 
   GetWindowText( str );                                                // room for more input...

   str += _T("  ");                                                     //    control extended two spaces at a time

   CWindowDC dc( this );                                                //    measure size of current string + the two 
   CFont *pFontDC = dc.SelectObject( GetFont() );                       //    additional spaces
   CSize size = dc.GetTextExtent( str );
   dc.SelectObject( pFontDC );

   CRect ParentRect;                                                    //    rect of parent window: cannot extend 
   GetParent()->GetClientRect( &ParentRect );                           //    edit control beyond its right bound!

   if ( size.cx > m_Rect.Width() )                                      //    resize edit control if necessary and 
   {                                                                    //    there's space to do so.
      if( size.cx + m_Rect.left < ParentRect.right )
         m_Rect.right = m_Rect.left + size.cx;
      else
         m_Rect.right = ParentRect.right;
      MoveWindow( &m_Rect );
   }
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== BeginEdit ======================================================================================================= 
//
//    Initiate an inplace edit of a numeric integral or floating-point value. 
//
//    To support the situation where an inplace edit operation is initiated by a key press, the method accepts an arg 
//    specifying the first character passed to the control.  If the character is nonzero, a WM_CHAR message is passed 
//    to the control after setting it up.  If it is a valid char, it will replace the initial value in the control.  
//    Note that the initial numeric value should still be provided -- we compare the control's current contents with 
//    this value to determine if the user has made a change (see IsChanged()).  Also note that the method only accepts 
//    a character code, NOT associated key data such as repeat count, extended key information, etc.
//
//    NOTE:  "Owner" vs "parent" window.  By design, the window passed in the constructor is considered the "owner" 
//    window of the inplace edit control.  It is also the control's initial parent.  So that CInPlaceNumEdit can be 
//    used in a dialog (or form view) to perform inplace editing of a "sibling" dialog control, we allow the control to 
//    be temporarily assigned a different parent.  By making the "sibling" control the parent of the inplace edit ctrl, 
//    we ensure that it does not drawn over (I tried WM_CLIPSIBLINGS, etc., to no avail!).  The owner window, however, 
//    still receives the NM_KILLFOCUS notification when the inplace edit ctrl hides itself...
//
//    ARGS:       pParent     -- [in] parent window (if NULL, owner window is taken as parent).
//                rect        -- [in] control rect, in client coords of parent window. 
//                bInt        -- [in] if TRUE, restrict to integer input; else allow floating-point. 
//                bNonneg     -- [in] if TRUE, restrict to nonnegative value.
//                nLen        -- [in] max # of chars allowed in the edit box (incl decimal point & minus sign). 
//                nPre        -- [in] # of digits allowed after the decimal point (>= 1, ignored for integer input).
//                dInitial    -- [in] the initial value to display.
//                nFirstChar  -- [in] first character to pass to ctrl after initiating (default = 0 -- ignored).
//
//    RETURNS:    TRUE if ctrl successfully displayed; FALSE if ctrl HWND does not exist, or ctrl was ALREADY visible. 
//
BOOL CInPlaceNumEdit::BeginEdit( CWnd* pParent, const CRect& rect, BOOL bInt, BOOL bNonneg, UINT nLen, UINT nPre, 
   double dInitial, UINT nFirstChar /* =0 */ )
{
   if( GetSafeHwnd() == NULL ) return( FALSE );                // no HWND; can't do anything! 
   if( IsWindowVisible() ) return( FALSE );                    // if window already visible, can't begin editing!

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
   SetFormat( bInt, bNonneg, nLen, nPre );                     // set format constraints
   m_dInitial = dInitial;                                      // init contents
   SetWindowText( m_dInitial );

   m_Rect = rect;                                              // set window size, pos; activate & show it
   SetWindowPos( NULL, m_Rect.left, m_Rect.top, m_Rect.Width(), 
                 m_Rect.Height(), SWP_SHOWWINDOW );
   SetFocus();
   SetSel( 0, -1 );                                            // select entire text initially

   if( nFirstChar != 0 ) SendMessage(WM_CHAR, nFirstChar, 0);  // update ctrl with first char press, if valid

   m_nExitChar = 0;                                            // reset exit character
   return( TRUE );
}


//=== CancelEdit ====================================================================================================== 
//
//    Cancel an ongoing inplace edit operation, hiding the edit control (EndEdit() without the owner notification!).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceNumEdit::CancelEdit()
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


//=== PreTranslateMessage [base override] ============================================================================= 
//
//    This is a workaround for a Win95 accelerator key problem.  (It was present in Maunder's CInPlaceEdit class; I 
//    kept it as is.)
//
//    ARGS:       pMsg  -- [in] the message to be processed.
//
//    RETURNS:    TRUE if message was translated and should not be dispatched; FALSE otherwise. 
//
BOOL CInPlaceNumEdit::PreTranslateMessage(MSG* pMsg) 
{
   // Stupid win95 accelerator key problem workaround - Matt Weagle.
   // Catch the Alt key so we don't choke if focus is going to an owner drawn button
   if( pMsg->message == WM_SYSCHAR ) return( TRUE );

   return( CWnd::PreTranslateMessage( pMsg ) );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== EndEdit ========================================================================================================= 
//
//    Terminate the inplace edit.  We hide the HWND, restore the owner as the edit ctrl's parent (if necessary), and 
//    then notify the owner via NM_KILLFOCUS.
//
//    See also: BeginEdit().
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceNumEdit::EndEdit()
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
