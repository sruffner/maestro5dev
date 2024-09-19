//===================================================================================================================== 
//
// inplacetree.cpp : Implementation of class CInPlaceTree, a tree control that has been specialized for "on demand, 
//                   in place" selection of an item that is part of an arbitrary keyed hierarchical tree.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CInPlaceTree is a "pop-up" CTreeCtrl that is used to select a single node from an arbitrary hierarchical tree of 
// nodes, where each node is represented by a text string for display purposes and a unique, nonzero 32-bit "key". 
// Unlike a typical tree control, this control pops up on top of a parent window at specified coordinates, and is 
// extinquished as soon as it loses the input focus or the user presses certain keys.  Such a control might be useful, 
// for example, to edit information in list/table/grid controls.
//
// CInPlaceTree populates itself "on demand", inserting a node's children just before it is expanded and deleting those 
// children when the node is collapsed (the "populate-on-demand" functionality is borrowed from a more elaborate tree 
// control by Paolo Messina -- see CREDITS).  To populate itself, CInPlaceTree requires a callback function which is 
// queried for information on a node's children (text label, key, and whether child also has children) whenever that 
// node is expanded.  Thus, the owner window is relieved of the mundane details of inserting items into the tree.  It 
// merely specifies the key of the root node when it initiates the inplace tree, and then provides children info as 
// needed via the callback function, which must be installed prior to using the control.
// 
// The callback function is responsible for ensuring the uniqueness of each tree node's 32-bit key, which is stored as 
// the associated tree item's "LPARAM" parameter.  If the keys are not unique, CInPlaceTree will not work correctly!
//
// ==> Usage.
// 1) Construct an instance, specifying the "owner" window that will receive the NM_KILLFOCUS notification when the 
// inplace tree is extinguished.  The constructor creates the tree's HWND in an invisible state, so don't call Create() 
// on this control.  NOTE that CInPlaceTree only supports certain tree control styles; any styles that are incompatible 
// with the implementation will be rejected.
//
// 2) Use SetCallback() to install the IPTREECB callback function.  The callback can be changed at any time so long as 
// the inplace control is not in use.  Invoking SetCallback() while the control is in use will fail silently.  The 
// callback function's prototype is:
// 
//    static int CALLBACK IpTreeCB( 
//       DWORD dwKey,                     -- [in] unique key identifying tree node
//       CStringArray* pstrArLbls,        -- [out] labels for the node's children (maybe NULL)
//       CDWordArray* pdwArKeys,          -- [out] unique keys identifying the node's children (maybe NULL)
//       CByteArray* pbArHasKids          -- [out] flag array: child[n] is a parent(1) or childless(0) (maybe NULL)
//       LPARAM lParam,                   -- [in] generic callback arg specified when callback installed
//       );
//
// This callback is used in two ways:  When the three array arguments are provided, the callback should fill them in 
// with the text labels, keys, and "hasKids" flags for each of the children under specified tree node, and return the 
// total # of children (CInPlaceTree expects all arrays to have the same length == total # kids!).  When the array args 
// are NULL, then the callback should merely return an indication of whether or not the specified node has any child 
// nodes at all (0 = childless, nonzero = parent).
//
// 3) To initiate the inplace tree, invoke BeginEdit() with the following "configuration" attributes:
//
//    parent window  :  The window above which the control will sit.  Depending on the situation, this window may or 
//       may not be the same as the owner.
//    control rect   :  A CRect indicating the bounds of the control in the parent window's client coords.
//    tree info      :  The unique key associated with the tree's root, and an optional "chain" of keys identifying a 
//                      node, relative to the root, that should be made visible and selected initially.
//
// 4) BeginEdit() displays the tree control at the specified location and obtains the input focus.  The inplace ctrl 
// handles keyboard and mouse input as the user manipulates the tree in search of the desired selection.  It hides 
// itself upon losing the keyboard focus, when the user double-clicks a data object (a leaf item), or when the user 
// presses certain keys, known as "exit characters": ESC, RETURN, TAB, CTRL-arrow.  When the user presses ESC, the 
// operation is cancelled and the owner is NOT notified.  Otherwise, we remember the item selected when the control 
// was extinguished, and notify the user via NM_KILLFOCUS.
//
// 5) The owner window should provide a handler for the NM_KILLFOCUS notification.  This handler can determine whether 
// or not the user chose a node different from the initial selection [IsChanged()], retrieve the key of the node that 
// was selected [GetSelectedKey()], and get the exit character (if any) which extinguished the control [GetExitChar()]. 
//
//
// CREDITS:
// 1) Article by Chris Maunder [07/30/2000, www.codeproject.com/miscctrl/gridctrl_combo.asp] -- Using Combo Boxes in 
// the MFC Grid Control.  I have modelled all of my "in place" controls after Maunder's own in-place edit and in-place 
// combo box controls...
// 2) Article by Paolo Messina [09/27/2002, www.codeproject.com/treectrl/waitingtreectrl.asp] -- CWaitingTreeCtrl. 
// This CTreeCtrl derivative combines "populate-on-demand" functionality with the ability to display a "wait" msg and 
// animate the control while waiting for a lengthy population process (such as network shares) to complete.  We chose 
// not to derive from CWaitingTreeCtrl, as CInPlaceTree is intended only for hierarchical tree data that can be 
// populated rapidly.  Instead, we merely implement CWaitingTreeCtrl's "populate-on-demand" functionality -- and a 
// simpler version that assumes that ALL nodes are dynamically populated when expanded and depopulated when collapsed. 
//
//
// REVISION HISTORY:
// 05nov2002-- Began development, based on the CNTRLX-specific class, CCxInPlaceTree.  CInPlaceTree is more generic, 
//             but it still requires a unique "key" that is stored with each node. 
// 05dec2002-- The "populate-on-demand" functionality did not work that well, mainly because the base class did not 
//             send TVN_ITEMEXPANDED for tree items that were empty, even though it did send TVN_ITEMEXPANDING!  Chose 
//             to put all the "populate" code in the TVN_ITEMEXPANDING call. 
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017. See GetKey(): had to cast
//             DWORD_PTR to DWORD to eliminate compiler warning. In 64-bit Windows, DWORD_PTR is 64-bit, while it is
//             32-bit in 32-bit Windows. The cast is OK because we do not expect such large keys (at least I hope).
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "inplacetree.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BEGIN_MESSAGE_MAP(CInPlaceTree, CTreeCtrl)
   ON_WM_KILLFOCUS()
   ON_WM_CHAR()
   ON_WM_KEYDOWN()
   ON_WM_LBUTTONDBLCLK()
   ON_NOTIFY_REFLECT( TVN_ITEMEXPANDING, OnItemExpanding )
   ON_NOTIFY_REFLECT( TVN_ITEMEXPANDED, OnItemExpanded )
   ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CInPlaceTree [constructor] ====================================================================================== 
//
//    Create the tree ctrl, initially as a child of the specified "owner" window, and initially invisible.  We apply a 
//    few default tree control styles, and prevent the application of some styles unsuited to our implementation.
//
//    ARGS:    pOwner   -- [in] the window object which "owns" the inplace tree (receives NM_KILLFOCUS).
//             dwStyle  -- [in] window and tree ctrl-specific style flags
//             nID      -- [in] child window ID assigned to this control
//
CInPlaceTree::CInPlaceTree( CWnd* pOwner, DWORD dwStyle, UINT nID ) 
{
   m_nExitChar = 0;                                                     // initializations
   m_dwKeyRoot = 0;
   m_dwKeyInitial = 0;
   m_dwKeySelected = 0;
   m_bRestoreParent = FALSE;
   m_bAlreadyEnding = FALSE;
   m_bInitializing = FALSE;

   m_pfnTreeCB = NULL;
   m_lpTreeCBArg = 0;

   if( (pOwner == NULL) ||                                              // we require a valid owner window
       !pOwner->IsKindOf(RUNTIME_CLASS(CWnd)) 
     ) 
   { 
      ASSERT( FALSE ); 
      TRACE0( "\nCInPlaceTree requires valid owner wnd!" );
      return;
   }

   DWORD dwMyStyle = dwStyle | WS_BORDER|WS_CHILD;                      // create the HWND: must be a child, w/ border, 
   dwMyStyle |= WS_HSCROLL|WS_VSCROLL|TVS_DISABLEDRAGDROP;              // scroll bars, and no drag drop allowed.
   dwMyStyle |= TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT;            // cosmetic styles added by default
   dwMyStyle |= TVS_SINGLEEXPAND;                                       // makes for a compact presentation
   dwMyStyle &= ~(WS_VISIBLE|TVS_EDITLABELS|TVS_FULLROWSELECT);         // must be invisible initially, no editing
   if(!Create(dwMyStyle, CRect(0, 0, 0, 0), pOwner, nID))
   {
      TRACE0("\nCInPlaceTree HWND creation failed!");
   }
}


//=== ~CInPlaceTree [destructor] ====================================================================================== 
//
//    Since we create HWND in constructor, we make sure it's destroyed here.
//
CInPlaceTree::~CInPlaceTree()
{
   DestroyWindow(); 
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnKillFocus [base override] ===================================================================================== 
//
//    WM_KILLFOCUS message handler.  Here's where we extinguish the inplace control, remember the selected key, and 
//    notify our owner window with NM_KILLFOCUS. 
//
//    ARGS:       pNewWnd  -- [in] the window that's receiving the focus.
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnKillFocus( CWnd* pNewWnd )
{
   if( m_bAlreadyEnding ) return;                                       // prevent reentrancy
   m_bAlreadyEnding = TRUE;

   HTREEITEM htiSel = GetSelectedItem();                                // remember key of object that was selected 
   if( htiSel != NULL ) m_dwKeySelected = GetKey( htiSel );

   ShowWindow( SW_HIDE );                                               // hide the tree ctrl and empty it 
   DeleteItem( TVI_ROOT );

   CWnd* pOwner = GetOwner();                                           // the window's owner 
   ASSERT( pOwner );

   if( m_bRestoreParent )                                               // if necessary, restore owner as wnd's parent 
   {
      SetParent( pOwner );
      m_bRestoreParent = FALSE;
   }

   CTreeCtrl::OnKillFocus( pNewWnd );                                   // note: this issues NM_KILLFOCUS for us!

   m_bAlreadyEnding = FALSE;
}


//=== OnChar [base override] ========================================================================================== 
//
//    WM_CHAR message handler.  Extinguishes inplace control when the TAB, RETURN, or ESC key is pressed. 
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnChar.
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
   if( nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE )    // end editing 
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                                          // ultimately hides ctrl via OnKillFocus() 
      return;
   }

   CTreeCtrl::OnChar(nChar, nRepCnt, nFlags);                           // else let base class handle
}


//=== OnKeyDown [base override] ======================================================================================= 
//
//    WM_KEYDOWN message handler.  Extinguishes inplace control when one of several possible "navigation" keys is 
//    depressed while the CTRL key is down.  When not accompanied by the CTRL key, the key down is passed on to the 
//    baseline tree ctrl for default processing -- so user can navigate within the tree itself!
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnKeyDown.
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
   if( (nChar == VK_PRIOR || nChar == VK_NEXT || nChar == VK_HOME || nChar == VK_END || 
        nChar == VK_DOWN  || nChar == VK_UP   || nChar == VK_RIGHT || nChar == VK_LEFT) &&
       (::GetKeyState(VK_CONTROL) < 0) )
   {
      m_nExitChar = nChar;
      GetParent()->SetFocus();                        // this ultimately hides the control via OnKillFocus()
      return;
   }

   CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


//=== OnLButtonDblClk ================================================================================================= 
//
//    WM_LBUTTONDBLCLK message handler.  If the node selected by the double-click is a leaf node, extinguish the tree 
//    control. 
//
//    ARGS:       nFlags   -- [in] see CWnd::OnLButtonDblClk().
//                p        -- [in] see CWnd::OnLButtonDblClk().
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnLButtonDblClk( UINT nFlags, CPoint p )
{
   HTREEITEM htiSel = GetSelectedItem();
   if( htiSel != NULL && IsLeaf( htiSel ) )
   {
      GetParent()->SetFocus();
      return;
   }

   CTreeCtrl::OnLButtonDblClk( nFlags, p );
}


//=== OnItemExpanding ================================================================================================= 
//
//    Handler for the reflected TVN_ITEMEXPANDING notification.  If the tree is about to expand a collapsed node, we 
//    repopulate its children (if it has any).
//
//    ARGS:       pNMHDR   -- [in] ptr to NM_TREEVIEW struct, cast to a generic NMHDR.
//                pResult  -- [in] result code.  set TRUE to prevent item from expanding/collapsing.
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnItemExpanding( NMHDR* pNMHDR, LRESULT* pResult )
{
   *pResult = 0;                                            // always allow item expansion/collapse 
   if( m_bInitializing ) return;                            // prevent execution while populating at startup!

   NM_TREEVIEW* pNMTV = (NM_TREEVIEW*) pNMHDR;
   HTREEITEM hItem = pNMTV->itemNew.hItem;                  // handle to tree item about to expand/collapse

   if( pNMTV->action & TVE_EXPAND )
   {
      if( GetChildItem( hItem ) != NULL )                   // a collapsed item should not have any children; we make  
         DeleteChildren( hItem );                           // sure of this before expanding an item -- just in case.

      SetRedraw( FALSE );                                   // while adding children, avoid redraws
      PopulateItem( hItem );                                // populate the node
      if( GetChildItem( hItem ) == NULL )                   // if no children added, be sure to reset expanded state
         SetItemState( hItem, 0, TVIS_EXPANDED );
      SetRedraw( TRUE );                                    // we're done -- allow redraws
   }
}

//=== OnItemExpanded ================================================================================================== 
//
//    Handler for reflected TVN_ITEMEXPANDED notification.  If tree has just collapsed a node, we depopulate that node. 
//
//    ARGS:       pNMHDR   -- [in] ptr to NM_TREEVIEW struct, cast to a generic NMHDR.
//                pResult  -- [in] result code.  ignored for TVN_ITEMEXPANDED.
//
//    RETURNS:    NONE
//
void CInPlaceTree::OnItemExpanded( NMHDR* pNMHDR, LRESULT* pResult )
{
   if( m_bInitializing ) return;                            // prevent execution while populating at startup!

   NM_TREEVIEW* pNMTV = (NM_TREEVIEW*)pNMHDR;

   if( pNMTV->action & TVE_COLLAPSE ) DeleteChildren( pNMTV->itemNew.hItem );
   *pResult = 0;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== BeginEdit ======================================================================================================= 
//
//    Prepare tree control and initiate inplace operation at the specified coords. 
//
//    The initial state of the tree is specified by a "chain of keys" from the root node to the initially selected node 
//    in the tree.  A tree node's "key" is a nonzero 32-bit value that CInPlaceTree requires to uniquely identify the
//    node in the callback fcn queried when populating that node.  The provided key chain must contain at least one 
//    node, which is taken as the root of the tree to be displayed.  Any additional keys are interpreted as a tree path 
//    from the root node to a descendant node (the last node in the chain) that should be initially selected when the 
//    tree control is displayed. 
//
//    If only a root node is provided, the tree control will simply display the children of that node, with no node 
//    selected initially.  Otherwise, we expand each successive node in the key chain until we get to the last node, 
//    which is then selected.
//
//    NOTES:
//    "Owner" vs "parent" window.  By design, the window passed in the constructor is considered the "owner" window of 
//    the inplace control.  It is also the control's initial parent.  For inplace control to be used in a dialog (or 
//    form view) to perform inplace editing of a "sibling" control, we allow the control to be temporarily assigned a 
//    different parent.  By making the "sibling" control the parent of the inplace ctrl, we ensure that it does not get 
//    drawn over (I tried WM_CLIPSIBLINGS, etc., to no avail!).  The owner window, however, still receives the 
//    NM_KILLFOCUS notification when the inplace control hides itself...
//
//
//    ARGS:       pParent        -- [in] parent window (if NULL, owner window is taken as parent).
//                rect           -- [in] control rect, in client coords of parent window. 
//                dwArInitChain  -- [in] a chain of keys from the root node to the initially selected tree node.
//
//    RETURNS:    TRUE if inplace control was successfully initiated; FALSE otherwise.
//
BOOL CInPlaceTree::BeginEdit( CWnd* pParent, const CRect& rect, const CDWordArray& dwArInitChain )
{
   if( GetSafeHwnd() == NULL || m_pfnTreeCB == NULL ||         // without HWND or required callback, we can't do it!
       IsWindowVisible() ||                                    // if window already visible, can't begin editing!
       dwArInitChain.GetSize() == 0 || dwArInitChain[0] == 0 ) // no root node provided!!
      return( FALSE ); 

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

   m_nExitChar = 0;                                            // reset exit character & selected key -- these are set 
   m_dwKeySelected = 0;                                        // when tree is extinguished

   m_bInitializing = TRUE;                                     // ignore TVN_ITEMEXPANDING/ED during initial phase

   m_dwKeyRoot = dwArInitChain[0];                             // populate the tree at root level
   PopulateItem( TVI_ROOT );
   if( GetChildItem( TVI_ROOT ) == NULL )                      // abort if the root has no children!
   {
      m_bInitializing = FALSE;
      return( FALSE );
   }

   m_dwKeyInitial = 0;                                         // if a key chain is provided, expand tree to display 
   HTREEITEM hLast = TVI_ROOT;                                 // and select the last node in the chain that EXISTS: 
   for( int i = 1; i < dwArInitChain.GetSize(); i++ ) 
   {
      DWORD dwNextKey = dwArInitChain[i];                      //    find tree item corresponding to next key in chain 
      HTREEITEM hChild = GetChildItem( hLast );
      while( hChild != NULL )
      {
         if( dwNextKey == GetKey( hChild ) ) break; 
         hChild = GetNextSiblingItem( hChild );
      }

      if( hChild == NULL )                                     //    if we did NOT find it, give up; the chain is bad!
         break;
      else                                                     //    if we found it, then move selection to it...
      {
         m_dwKeyInitial = dwNextKey;
         hLast = hChild;
         if( i < dwArInitChain.GetUpperBound() )               //    ...and if we're NOT at the end of chain, expand 
         {                                                     //    the found item to continue the search.
            PopulateItem( hLast );
            if( GetChildItem( hLast ) != NULL )
               Expand( hLast, TVE_EXPAND );
         }
      }
   }
   if( m_dwKeyInitial != 0 )                                   // select the last node that we found in the key chain, 
   {                                                           // if we found any
      ASSERT( hLast != TVI_ROOT && hLast != NULL );
      SelectItem( hLast );
      EnsureVisible( hLast );
   }

   SetWindowPos( NULL, rect.left, rect.top, rect.Width(),      // set window size, pos; activate & show it
                 rect.Height(), SWP_SHOWWINDOW );
   SetFocus();                                                 // grab the focus 
   InvalidateRect( NULL );                                     // repaint window to ensure it's up-to-date
   UpdateWindow();

   m_bInitializing = FALSE;                                    // initialization phase complete!
   return( TRUE );
}


//=== CancelEdit ====================================================================================================== 
//
//    Cancel an ongoing inplace operation, hiding the tree control without notifying the owner window
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
void CInPlaceTree::CancelEdit()
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


//=== GetSelectedKey ================================================================================================== 
//
//    Retrieve the key of the currently selected node in the inplace tree control.
//
//    If the tree is still visible, we return the key associated with the currently selected item, or 0 if no item is 
//    selected.  Once the tree has been hidden, we return the key that was saved the moment the ctrl was extinguished.
//
//    ARGS:       NONE.
//
//    RETURNS:    key of selected node (0 if none).
//
DWORD CInPlaceTree::GetSelectedKey()
{
   return( IsWindowVisible() ? GetKey( GetSelectedItem() ) : m_dwKeySelected );
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
BOOL CInPlaceTree::PreTranslateMessage( MSG* pMsg ) 
{
   // Stupid win95 accelerator key problem workaround - Matt Weagle.
   // Catch the Alt key so we don't choke if focus is going to an owner drawn button
   if( pMsg->message == WM_SYSCHAR ) return( TRUE );

   return( CWnd::PreTranslateMessage( pMsg ) );
}


//=== ModifyStyle [base override] ===================================================================================== 
//
//    Override to enforce certain restrictions on the tree ctrl styles that may be used with this inplace version.
//
//    ARGS:       see CWnd::ModifyStyle().
//
//    RETURNS:    TRUE if style was successfully modified; FALSE otherwise.
//
BOOL CInPlaceTree::ModifyStyle( DWORD dwRemove, DWORD dwAdd, UINT nFlags /* =0 */ )
{
   if( 0 != (dwRemove & (WS_BORDER|WS_CHILD|TVS_DISABLEDRAGDROP|WS_HSCROLL|WS_VSCROLL)) ) return( FALSE );
   if( 0 != (dwAdd & (TVS_EDITLABELS|TVS_FULLROWSELECT)) ) return( FALSE );
   return( CTreeCtrl::ModifyStyle( dwRemove, dwAdd, nFlags ) );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== PopulateRoot ==================================================================================================== 
//
// *********** CAN WE GET RID OF THIS?? Test it.... ************88
//    Populate and expand the tree control at the root level.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CInPlaceTree::PopulateRoot()
{
   PopulateItem( TVI_ROOT );
   SetRedraw( FALSE );                                // force update; don't scroll
   SCROLLINFO si;
   GetScrollInfo( SB_HORZ, &si );
   EnsureVisible( GetChildItem( TVI_ROOT ) );
   SetScrollInfo(SB_HORZ, &si, FALSE);
   SetRedraw();
}


//=== PopulateItem ==================================================================================================== 
//
//    Populate the specified node in the tree by querying the installed callback function for the node's children info. 
//
//    ARGS:       hParent  -- [in] the "parent" tree item to be populated.
//
//    RETURNS:    TRUE  ==> disable expansion of this node if it has no children.
//                FALSE ==> always let node be expandable, even if it has no children currently (we ALWAYS do this!) 
//
BOOL CInPlaceTree::PopulateItem( HTREEITEM hParent )
{
   ASSERT( m_pfnTreeCB != NULL && hParent != NULL );
   DWORD dwKey = GetKey( hParent );                   // retrieve item's key
   ASSERT( dwKey != 0 );
                                                      // query callback for info on any children: 
   CStringArray strAr;                                //    text label, 
   CDWordArray dwAr;                                  //    unique key, and 
   CByteArray bAr;                                    //    flag indicating whether or not child also has children 
   int nKids = m_pfnTreeCB( dwKey, &strAr, &dwAr, &bAr, m_lpTreeCBArg );

   if( nKids > 0 )                                    // insert tree items for the parent's children, if any
   {
      ASSERT( strAr.GetSize() == nKids && dwAr.GetSize() == nKids && bAr.GetSize() == nKids );
      TV_INSERTSTRUCT tvi;
      tvi.hParent = hParent;
      tvi.hInsertAfter = TVI_LAST;
      for( int i = 0; i < nKids; i++ )
      {
         tvi.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN;
         tvi.item.pszText = (LPTSTR) ((LPCTSTR) strAr[i]);
         tvi.item.lParam = (LPARAM) dwAr[i];
         tvi.item.cChildren = (bAr[i] != 0) ? 1 : 0;
         InsertItem( &tvi );
      }
   }

   return( FALSE );
}


//=== DeleteChildren ================================================================================================== 
//
//    Remove a tree item's children.
//
//    ARGS:       hParent  -- [in] the tree item to be populated.
//
//    RETURNS:    NONE.
//
void CInPlaceTree::DeleteChildren( HTREEITEM hParent )
{
   HTREEITEM hChild = GetChildItem( hParent );
   HTREEITEM hNext;

   while( hChild != NULL )
   {
      hNext = GetNextSiblingItem( hChild );
      DeleteItem( hChild );
      hChild = hNext;
   }
}


//=== IsLeaf ========================================================================================================== 
//
//    Is specified tree item really childless?  Since we dynamically populate the tree's contents on demand, we cannot 
//    use base class functionality (eg, GetChildItem(hItem) == NULL) to determine if any node is childless.  In our 
//    implementation, any collapsed node will be childless!
//
//    To determine whether or not a node is really childless, we invoke the callback fcn without the array arguments. 
//
//    ARGS:       hItem  -- [in] the tree item to be checked.
//
//    RETURNS:    TRUE if tree item corresponds to a leaf node IAW the callback fcn; FALSE otherwise.
//
BOOL CInPlaceTree::IsLeaf( HTREEITEM hItem )
{
   ASSERT( m_pfnTreeCB != NULL && hItem != NULL );
   DWORD dwKey = GetKey( hItem );                  // retrieve item's key
   ASSERT( dwKey != 0 );

   return( BOOL(m_pfnTreeCB( dwKey, NULL, NULL, NULL, m_lpTreeCBArg ) == 0) );
}


//=== GetKey ========================================================================================================== 
//
//    Get key value associated with an existing tree item. 
//
//    ARGS:       hItem  -- [in] the tree item to be checked.
//
//    RETURNS:    the tree item's key, or 0 if not found.
//
DWORD CInPlaceTree::GetKey( HTREEITEM hItem )
{
   if( hItem == NULL ) return( 0 );
   if( hItem == TVI_ROOT ) return( m_dwKeyRoot );        // the root item is a special case!
   return(static_cast<DWORD>(GetItemData( hItem )));
}

