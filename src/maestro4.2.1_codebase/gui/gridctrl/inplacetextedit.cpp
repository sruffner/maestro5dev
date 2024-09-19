//===================================================================================================================== 
//
// inplacetextedit.cpp : Implementation of class CInPlaceTextEdit, a single-line edit box designed for inplace editing.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CInPlaceTextEdit is designed for use as a sort of popup inplace edit control that can be used to edit text strings 
// in dialogs, grid-like controls, list controls, etc.  It is derived from MFC's CEdit control object.
//
// CInPlaceTextEdit's implementation was based upon the inplace edit control associated with Chris Maunder's MFC Grid 
// Control (CREDITS).  It essentially does the same thing as Maunder's version, but it is more generic and is designed 
// for reuse.  For example, after the control is extinguished, the parent window can query it for the new text string 
// and the exit character.  Maunder's CInPlaceEdit control provides this information via a notification code specific 
// to the MFC grid control; his control is also designed to destroy itself as soon as it is extinguished.
//
// ==> Usage.
// (1) Construct an instance, specifying the "owner" window that will receive the NM_KILLFOCUS notification when the 
// inplace edit is extinguished.  The constructor creates the edit control window in an invisible state, so don't call 
// Create() on this control.
// (2) To initiate the inplace edit, call BeginEdit(), specifying the edit control's parent window, the control's 
// window rect, and the initial text string to display.
// (3) BeginEdit() displays the edit ctrl at the specified location, obtains the input focus, and displays the initial 
// text (initially selected).  It then handles keyboard and mouse input from the user to change the value.  As with any 
// typical inplace-edit control, it hides itself when it loses the keyboard focus or the user presses certain keys:  
// ESC, RETURN, TAB, or CTRL+arrow key (the arrow keys alone are used to navigate within the edit control itself).  The 
// control's owner is then notified with NM_KILLFOCUS. 
// (4) The owner window should provide a handler for the NM_KILLFOCUS notification.  This handler can retrieve the new 
// text string (via CEdit::GetWindowText()), as well as the exit character (if any) which extinguished the edit control 
// (CInPlaceTextEdit::GetExitChar()).  It can also check whether or not the user changed the initial text string at all 
// (CInPlaceTextEdit::IsChanged()).
//
//
// CREDITS:
// 1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21. 
// CInPlaceTextEdit's implementation is based on Maunder's CInPlaceEdit control, which is included in the grid control 
// source code.  
//
//
// REVISION HISTORY:
// 31oct2002-- Adapted from CInPlaceNumEdit.  DEVNOTE:  It would be nice to have one control that handles both general 
//             text and numeric-only text...
// 11dec2002-- Modified BeginEdit() to optionally accept a first character that immediately replaces the initial text 
//             string (to support the initiation of inplace editing via a keypress).
//
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "ctype.h"                           // for isprint(), isspace()...

#include "inplacetextedit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CInPlaceTextEdit, CEdit)
   ON_WM_KILLFOCUS()
   ON_WM_CHAR()
   ON_WM_KEYDOWN()
   ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CInPlaceTextEdit [constructor] ================================================================================== 
//
//    Create the edit control, initially as a child of the specified "owner" window, and initially invisible.
//
//    ARGS:    pOwner   -- [in] the window object which "owns" the inplace edit control.
//             dwStyle  -- [in] window style.
//             nID      -- [in] child window ID. 
//
CInPlaceTextEdit::CInPlaceTextEdit( CWnd* pOwner, DWORD dwStyle, UINT nID )
{
   m_strInitial.Empty();
   m_nExitChar = 0;
   m_Rect.SetRect( 0, 0, 100, 30 );                                     
   m_bRestoreParent = FALSE;
   m_bAlreadyEnding = FALSE;

   if( (pOwner == NULL) ||                                              // we require a valid owner window
       !pOwner->IsKindOf(RUNTIME_CLASS(CWnd)) 
     ) 
   { 
      ASSERT( FALSE ); 
      TRACE0( "\nCInPlaceTextEdit requires valid owner wnd!" );
      return;
   }

   DWORD dwEditStyle = WS_BORDER|WS_CHILD|ES_AUTOHSCROLL | dwStyle;     // create the HWND: must be a child, w/ border 
   dwEditStyle &= ~WS_VISIBLE;                                          // and autohscroll, invisible initially
   if(!Create(dwEditStyle, m_Rect, pOwner, nID))
   {
      TRACE0("\nCInPlaceTextEdit HWND creation failed!");
   }
}


//=== ~CInPlaceTextEdit [destructor] ================================================================================== 
//
//    Since we create HWND in constructor, we make sure it's destroyed here.
//
CInPlaceTextEdit::~CInPlaceTextEdit()
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
void CInPlaceTextEdit::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
   if( (nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_HOME || nChar == VK_END ||
        nChar == VK_DOWN  || nChar == VK_UP   || nChar == VK_RIGHT || nChar == VK_LEFT) &&
       (GetKeyState(VK_CONTROL) < 0) )
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                        // this ultimately hides the control via OnKillFocus()
      return;
   }

   CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


//=== OnKillFocus [base override] ===================================================================================== 
//
//    WM_KILLFOCUS message handler.  Here's where we extinguish the inplace edit control. 
//
//    ARGS:       pNewWnd  -- [in] the window that's receiving the focus.
//
//    RETURNS:    NONE
//
void CInPlaceTextEdit::OnKillFocus( CWnd* pNewWnd )
{
   CEdit::OnKillFocus( pNewWnd );
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
void CInPlaceTextEdit::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
   if( nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE )    // end editing 
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                                          // ultimately hides ctrl via OnKillFocus() 
      return;
   }

   CEdit::OnChar(nChar, nRepCnt, nFlags);                               // else let base class handle

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
//    Initiate an inplace edit of a text string. 
//
//    To support the situation where an inplace edit operation is initiated by a key press, the method accepts an arg 
//    specifying the first character passed to the control.  If the character is nonzero, represents a printable 
//    character, and is not whitespace, then it replaces the initial text string.  Note that the initial text string 
//    should still be provided -- we compare the control's current contents with this string to determine if the user 
//    has made a change (see IsChanged()).  Also note that the method only accepts a character code, NOT associated key 
//    data such as repeat count, extended key information, etc.
//
//    NOTE:  "Owner" vs "parent" window.  By design, the window passed in the constructor is considered the "owner" 
//    window of the inplace edit control.  It is also the control's initial parent.  So that CInPlaceTextEdit can be 
//    used in a dialog (or form view) to perform inplace editing of a "sibling" dialog control, we allow the control to 
//    be temporarily assigned a different parent.  By making the "sibling" control the parent of the inplace edit ctrl, 
//    we ensure that it is not drawn over (I tried WM_CLIPSIBLINGS, etc., to no avail!).  The owner window, however, 
//    still receives the NM_KILLFOCUS notification when the inplace edit ctrl hides itself...
//
//    ARGS:       pParent     -- [in] parent window (if NULL, owner window is taken as parent).
//                rect        -- [in] control rect, in client coords of parent window. 
//                strInitial  -- [in] the initial text string to display.
//                nFirstChar  -- [in] first character to pass to ctrl after initiating (default = 0 -- ignored).
//
//    RETURNS:    TRUE if ctrl successfully displayed; FALSE if ctrl HWND does not exist, or ctrl was ALREADY visible. 
//
BOOL CInPlaceTextEdit::BeginEdit( CWnd* pParent, const CRect& rect, LPCTSTR strInitial, UINT nFirstChar /* =0 */ )
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
   m_strInitial = strInitial;                                  // init contents
   SetWindowText( m_strInitial );

   m_Rect = rect;                                              // set window size, pos; activate & show it
   SetWindowPos( NULL, m_Rect.left, m_Rect.top, m_Rect.Width(), 
                 m_Rect.Height(), SWP_SHOWWINDOW );
   SetFocus();
   SetSel( 0, -1 );                                            // select entire text initially

   if( nFirstChar != 0 && ::isprint( nFirstChar ) &&           // if first char is printable & not white space, send 
       !::isspace( nFirstChar ) )                              // char msg so that it replaces initial text!
      SendMessage( WM_CHAR, nFirstChar, 0 );                   // NOTE -- no "key data" available!

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
void CInPlaceTextEdit::CancelEdit()
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
BOOL CInPlaceTextEdit::PreTranslateMessage(MSG* pMsg) 
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
void CInPlaceTextEdit::EndEdit()
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
