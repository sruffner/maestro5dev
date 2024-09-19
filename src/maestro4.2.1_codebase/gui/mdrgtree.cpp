//===================================================================================================================== 
//
// mdrgtree.cpp : Implementation of class CMultiDragTreeView.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//    The Windows common tree view control (and its associated MFC encapsulation CTreeCtrl) has some undesirable 
// characteristics which limit its usefulness as a user interface element:
//
//    (1) Support for drag-drop operation is limited.  Programmers using CTreeCtrl must handle the details of animating 
// the drag, including automatic scrolling. 
//    (2) During in-place editing of a tree item label, the ESC and ENTER keys are not dispatched to the edit control, 
// so that the only way to end the operation is to click outside the edit control.  [NOTE:  This is not a problem when 
// the tree control is embedded in a CTreeView; not sure why.]
//    (3) In Windows, a right-click usually brings up a "context menu" with a list of relevant operations that can be 
// performed.  The normal way that a view or dialog accomplishes this is by including a WM_CONTEXTMENU handler fcn. 
// However, a single right-click fails to generate a WM_CONTEXTMENU message in the case of tree view controls.  The 
// tree view instead sends a NM_RCLICK notification to its parent.
//    (4) The tree view control does not provide support for multiple selection of items.
//
// The native tree control may lack these features because someone decided that they should be implemented by the 
// dialog or view containing the control.  Indeed, in a doc/view application like CNTRLX, these operations may require 
// interaction with the associated document.  I therefore decided to focus on CTreeView rather than CTreeCtrl.
//
// CMultiDragTreeView extends CTreeView, removing some of the above limitations on the tree control it "wraps".  The 
// view handles many of the typical tree control notifications via the ON_NOTIFY_REFLECT macro (we use this rather than 
// ON_NOTIFY since a CTreeView is a single CWnd object, not a view object containing another control object!).  In 
// addition, derived views can tailor their behavior by modifying the implementation of a number of key overridables, 
// as explained below.  The intent is to provide a better "base" tree view than offered by MFC's CTreeView.
//
// ==> On invoking the context menu:  To correct CTreeView's problem w/ context menu invocation, we bypass the normal 
// MFC message-handling infrastructure.  The PreTranslateMessage() and OnRClick() overrides detect the three standard 
// ways in which the context menu is invoked.  These overrides call the method RunContextMenu() rather than MFC's 
// OnContextMenu().  RunContextMenu() calls the virtual helper method GetContextMenu() to actually load the popup menu 
// that should be displayed.  Derived classes MUST override GetContextMenu() to implement context menu support.  Note 
// that context-menu framework provided by CMultiDragTreeView defines an "applicable" tree item at the time the 
// context menu is invoked:
//    (1) If invoked by a right-click, it is the item that was clicked (if any).
//    (2) If invoked by the keyboard (SHIFT-F10 or the Windows special short-cut key), it is the item having both the 
//        focus and selection at the time (if any).
// Thus, derived classes have an opportunity to throw up different context menus depending on the nature of this 
// applicable tree item -- if there is one.
//
// ==> On selecting "target items" for WM_COMMAND operations:  Derived classes must provide ON_COMMAND and (possibly) 
// ON_UPDATE_COMMAND_UI handlers to impart useful functionality on the underlying tree control.  When one of these 
// handlers is called, the question arises:  to which item(s) does the WM_COMMAND apply?  That will depend on the 
// application, of course.  Two possible guidelines:  (1) the command operates on the currently focused item (in 
// multi-selection mode, there's a difference between "focused" and "selected"; in single-selection, they're usually 
// the same); (2) the command operates on all selected items.  Both options are supported by CMultiDragTreeView:  use 
// GetFocusedItem() to get the focus item (there can only be one!), or use GetSelectedList() to get the entire list 
// of selected items when multi-select is enabled.
//
// ==> On default drag-n-drop behavior:  CMultiDragTreeView supports single- and multiple-selection drag-n-drop 
// operations initiated by both the left and right mouse buttons.  Regardless of which button initiated the drag, the 
// animation is the same, and one of three possible effects can be achieved:  
//       (1) Drag items (and their subtrees) are *moved* under the drop target. 
//       (2) Drag items (and their subtrees) are *copied* under the drop target. 
//       (3) The operation is cancelled. 
// The operation is cancelled whenever the drop target is invalid (including a drop point outside the view!).  In the 
// case of a left drag, (1) is executed unless the CTRL key is held down, in which case (2) is executed.  In the case 
// of a right drag, we throw up a context menu that explicitly lists the three options above.  The user then must 
// choose one of the options or click outside the context menu to cancel.  To customize the effects of a drag to a
// particular application, derived classes should override the fcn RealizeDrag(), which copies/moves drag items as 
// described in (1) & (2) above.
//    In a multiple-selection drag, dragging a parent item implies that all of its descendants are dragged with it, so 
// it makes no sense to include descendants of a dragged item in the drag operation.  Therefore, when a drag finishes 
// without being cancelled, PreRealizeDrag() is called to deselect any such descendants, construct a corrected "drag 
// list", and then deselect all items in the tree.  RealizeDrag() is then called with the corrected "drag list".  The 
// default version of RealizeDrag() selects all items which it adds to the tree, so the user has a visual cue about 
// what had changed.
//    The context menu is implemented by CMultiDragTreeView exclusively in the implementation file; no resource file is 
// required.  However, we must assign three command IDs (ID_MDTV_MOVE, ID_MDTV_COPY, ID_MDTV_CANCEL) to the three 
// options.  Derived classes should be careful not to use these IDs for mapping to their own command handlers!!!!  You 
// can disable drag-n-drop entirely by overriding PreCreateWindow() and setting the TVS_DISABLEDRAGDROP style.  To 
// disable right drags only, provide your own "do-nothing" TVN_BEGINRDRAG handler.  To replace the default right-drag 
// menu with a custom menu, override RunRightDragMenu() and provide your own command handlers. 
//    During a drag the cursor changes appearance depending on whether or not the current mouse point is over a valid 
// drop target.  We use the standard cursors IDC_ARROW and IDC_NO.
//
// ==> On ExpandEx():  CTreeCtrl::Expand() does NOT issue the TVN_ITEMEXPANDING/ED notifications; i.e., its behavior is 
// not identical to the use clicking on the +/- button next to the item label!  Since CMultiDragTreeView uses both 
// notifications to implement enhanced features, we needed a different version of Expand() that includes the 
// notifications -- hence ExpandEx().  Derived classes should normally use ExpandEx() in place of CTreeCtrl::Expand(). 
//
// ==> On DeleteItemEx() and the "cChildren" attribute of a parent item:  DeleteItemEx() attempts to avoid problems 
// that occur when the last child of an expanded parent item is deleted.  If you use CTreeCtrl::DeleteItem(), it fails 
// to collapse the expanded parent and remove the "children indicator" (the boxed '+' or '-' button to the left of the 
// item).  We would like the parent item to automatically collapse, accompanied by the TVN_ITEMEXPANDING/ED 
// notifications which this class handles as part of its enhanced tree view functionality.  One way to do this is to 
// manually reset the TVIS_EXPANDED flag and the "cChildren" attribute of the parent item when we delete its last 
// child.  This solution fails in a subtle way:  there's something screwy about the "cChildren" attribute.  If it is 
// set to 0 manually, the native tree control no longer properly inserts new subitems under that parent item!  The 
// CTreeCtrl::InsertItem() call returns successfully, but the item is placed at root level out of view -- definitely 
// a Platform SDK bug.  Also, if it is set to 1 manually, the +/- button remains next to a parent when its last child 
// item is deleted.  SOLUTION:  DeleteItemEx() first checks to see if we are deleting the last child of an expanded 
// parent item.  In that case, it calls ExpandEx() to collapse the parent item BEFORE deleting the child.  For this 
// solution to work properly, we strictly avoid use of the "cChildren" attribute, and derived classes should do 
// likewise.  Also, use DeleteItemEx() instead of CTreeCtrl::DeleteItem().
//    !!!DEV NOTE!!! We may want to revisit this issue, since the SDK documentation recommends use of the "cChildren" 
//    attribute to reduce the tree control's memory usage by removing child items when a parent is collapsed and 
//    reinserting them when it is expanded.
//
// ==> On multiple-selection:  The native tree control does NOT support multiple selection at all.  It provides for one 
// and at most one "selected item", the label of which is both highlighted and bordered by a focus rectangle.  We 
// implement multiple-selection by adding a new emulated item state, TVIS_FOCUSED.  Physically, an item with this state 
// is the focus item, having a focus rectangle drawn around its label; only one item can have the focus.  Items which 
// are selected have the TVIS_SELECTED bit set; their labels are highlighted with the system highlight color.  The 
// CMultiDragTreeView function SetItemState() handles the details of making these two states DISTINCT.  Also provided 
// are CTreeCtrl-like functions GetItemState(), and SelectItem(). *** Derived classes MUST AVOID USE OF THE CTreeCtrl 
// fcns of the same name; otherwise, the multiple-selection state could be corrupted. ***  In addition, a number of 
// useful public methods are provided to obtain/manipulate the focus item or the current selection set.  For example, 
// use SelectAll() to clear all items currently selected (or select all items in the tree!); use GetSelectedList() to 
// obtain a linked list of HTREEITEM handles representing the current selection set; use SelectChildren() to select or 
// deselect the immediate children (or all descendants) of a particular tree item; use FocusItem() to change the focus 
// item; and there are others...
//    To select more than one item, the user must hold down the SHIFT or CTRL keys; the behavior is similar to that of 
// the list control in Windows Explorer.  It is also possible to select multiple items via rubber banding, with or 
// without the SHIFT/CTRL keys.
//    The vast majority of the credit for these multi-selection features goes to Richard Hazelwood.  See CREDITS.
//
//
// OVERRIDABLES: 
//    CustomDblClk() ==> Does nothing.  Override to provide a customized response to the double-click.  [NOTE that this 
//       virtual method is called AFTER default processing by the native control, which toggles the expand/collapse 
//       state of parent items in the tree.  If you wish to prevent this toggling in certain instances, you can still 
//       do so with a little work:  reflect the NM_DBLCLK notification in your derived class and provide a handler for 
//       that reflected notification.  CustomDblClk() will be called regardless.
//    GetContextMenu() ==>  Does nothing.  Override to implement context menu for the tree view.  CMultiTreeDragView 
//       ensures that this function is invoked by a single right-click, SHIFT-F10 key combo, and the Windows context 
//       menu key. 
//    RealizeDrag() ==> Moves or copies the drag items to their new location.  Override to customize the consequences 
//       of a drag-n-drop operation, perform app-specific work, abort drag-n-drop ops that are not appropriate, etc. 
//    RunRightDragMenu() ==> Display right-drag context menu.  See description above.  Override to customize the 
//       appearance of the right-drag menu and the possible operations the user can select. 
//    RealizeDelete() ==> Deletes the currently selected item(s).  It is called when the tree control has the focus and 
//       the user presses the DELETE key.  Override to perform app-specific work, etc. 
//    AcceptNewLabel() ==> All non-empty item labels are accepted.  Override to further restrict what labels are 
//       accepted. 
//    GetExpandBitmaps() ==> Called after an item is expanded or collapsed.  No effect.  Override to implement 
//       "expanded" and "collapsed" icons for selected items in the tree.  Derived class must handle the details of 
//       associating an image list with the underlying tree control.
//    Sort() ==> Called after item label is edited.  Override to perform app-specific sort of the item's children. 
//    CanDrag() ==> All tree items may be dragged.  Override to prevent dragging of some (or all) items.  The item will 
//       appear in the drag image, but it will NOT be moved/copied since PreRealizeDrag() calls this function to 
//       screen out all non-draggable items. 
//    CanDrop() ==> All tree items may serve as drop targets.  Override to disable dropping onto some (or all) items. 
//    CanEdit() ==> All tree item labels may be edited.  Override to prevent in-place editing of some (or all) items. 
//    CanDelete() ==> All tree items may be deleted.  Override to prevent deletion of some (or all) items.  This only 
//       relates to user-initiated deletion via the DELETE key [see PretranslateMessage()].
//
//
// RELEVANT NOTES ON THE NATIVE WINDOWS TREE CONTROL AND CTreeCtrl:
// (1) By "visible" item, the MFC documentation refers to those items which are part of an expanded subtree.  The item 
//     may not actually be visible to the user depending on the scroll state of the tree.  There are no functions 
//     which directly determine whether a given HTREEITEM is currently within the viewable client area.  However, one 
//     can find out by calling CTreeCtrl::GetItemRect() and checking whether the "top" coordinate is negative or the 
//     "bottom" coordinate is greater than the client rect's "bottom" coordinate.
// (2) When bTextOnly = FALSE in CTreeCtrl::GetItemRect(), it returns the rect bounding the entire line occupied by the 
//     item -- SORT OF!  The left bound is always 0 and the right bound is always the width of the client area, 
//     regardless of the state of the horizontal scroll bar!!!  The vertical coords make sense, being negative when the 
//     item is scrolled above the view area.  When bTextOnly = TRUE, the vertical coords are the same as above.  The 
//     horizontal coords give the left & right bounds of the item label only.  But this time the left bound is adjusted 
//     depending upon the current horizontal scroll pos, and the right bound can go beyond the width of the client area 
//     for long label names.
//
//
// CREDITS:
// (1) Article by Christopher A. Snyder [08/06/1998, codeguru.earthweb.com/treeview/right_button_menu.shtml] -- 
// Originally adapted some of this code for altering the behavior of the tree view to single right-clicks.  However, 
// much of that code was replaced by code from (2).
// (2) Article by PJ Naughter [03/04/2000, codeguru.earthweb.com/treectrl/filetree.asp] -- Adapted the code for 
// supporting drag/drop.  Also adapted CTreeFileCtrl::PreTranslateMessage() for use here.
// (3) Article by Zafir Anjum [08/06/1998,  codeguru.earthweb.com/treeview/right_drag_drop.shtml] -- Describes 
// strategy for supporting a right-button drag-n-drop with popup context menu.  Adapted some of the code. 
// (4) Article by Richard Hazelwood [07/25/1999, codeguru.earthweb.com/treeview/CMultiTree.shtml] -- Adapted the 
// multi-select tree control class CMultiTree for use in this CTreeView-derived class.
// (5) Article by Frank Kobs [12/21/1999, codeguru.earthweb.com/listview/MultiDragImg.shtml] -- Adapted his ideas for  
// creating a multiple-selection drag image in a list control for use with a tree control.  Unlike his approach, we 
// just draw an outline representation of each selected item (like in Explorer's left pane).  See the function 
// CreateDragImageEx().
// (6) Article by Hao(David) Tran [5/1/2000, codeguru.earthweb.com/listview/DualListCtrl.shtml] -- Since the Kobs 
// solution (5) required VC++6.0, we reviewed this article, which adapts Kobs' approach for use in VC++5.0.
//
//
// REVISION HISTORY:
//
// 07jun2000-- Created.  I'll start by implementing the drag/drop-enabled tree without multiple selection, and then 
//             introduce multiple selection later.
// 08jun2000-- Still working on CMultiDragTreeView.  Still need to define CanDrag(), etc.  
// 20jun2000-- First version done.  Having problems with integration into CNTRLX doc/view paradigm... Decided to 
//             implement CMultiDragTreeView from CTreeView, instead of CMultiDragTreeCtrl from CTreeCtrl.  This makes 
//             integration into CNTRLX easier, but does not provide for usage in a dialog box or form view!
// 22jun2000-- First version working, though may want to do some fine-tuning.  Then work on adding support for 
//             multiple selection!
// 23jun2000-- Modified to support sorting siblings after an in-place edit operation:  virtual fcn Sort() added. 
//          -- Added support for right-button drag-n-drop with popup menu.
//          -- Other fine-tuning.
// 24jun2000-- Added support to dynamically update cursor during drag-n-drop depending on validity of drop target. 
//          -- Begun attempt to support multi-selection.  Saved previous state of file in save_mdrgtree.*. 
// 29jun2000-- Getting ready to test with multi-selection.  Decided to get rid of the "context change" stuff.  From 
//             now on, WM_COMMAND operations should apply to all selected items; if a particular command is not 
//             amenable to a multi-selection, it should be disabled.  Such details are left up to derived views...
//          -- Dragging of multiple items currently disabled while I test basic multi-selection stuff.
// 30jun2000-- Got multi-selection generally working, testing with CCxTrialTree.  Still having problems with items 
//             expanding/collapsing in PreTranslateMessage().  Also still have to implement dragging of multiple 
//             items.
// 05jul2000-- Still working on fix DeleteItemEx() to ensure parent is collapsed after its last child is deleted.  We 
//             cannot just call ExpandEx() to do the collapse, because the call to CTreeCtrl::Expand() fails when you 
//             try to collapse a childless parent....
// 06jul2000-- Finally worked out issues related to collapsing a parent item when its last child is deleted.  Noticed 
//             a problem during drag animation:  when you hover over a collapsed parent item long enough for it to 
//             expand, the first one or two child items may be corrupted in appearance.  Has something to do with 
//             the control trying to redraw itself while the area is "locked" by a CImageList dragging function...
//             NEXT STEP:  multiple-item drag-n-drop!
// 07jul2000-- Solved problem with corruption in appearance of first item or two under a parent that is auto-expanded 
//             during drag-n-drop:  We do NOT lock the tree view window during dragging.  However, to avoid corrupting 
//             the window's appearance during a drag, we use CImageList::DragShowNolock() to temporarily hide the drag 
//             image while we update the tree view window.  While we used CImageList::DragShowNolock() in this manner 
//             while expanding the parent item, the CTreeCtrl::Expand() function *posts* WM_PAINT to the message queue. 
//             As a result, that WM_PAINT does not get processed until after OnTimer() returns, at which point the drag 
//             image list, now visible again, interferes with the repaint.  To fix, we call UpdateWindow() immediately 
//             after expanding the parent and before revealing the drag image.
//          -- A BUG WITH LONG ITEM NAMES:  When the item name is partially truncated, a tooltip like display appears 
//             whenever the mouse passes over the item.  Tooltip is not erased properly during a drag-n-drop.  FIX.
//          -- Began modifications to implement multiple-item drag-n-drop.  First step is to implement a multiple-item 
//             drag image.  Currently developing CreateDragImageEx() toward that goal....
// 08jul2000-- Got CreateDragImageEx() working, though some refinements remain.
// 10jul2000-- Satisfied with CreateDragImageEx(), but the drag image flashes undesirably.  This was because the 
//             OnTimer() fcn would hide & show the drag image every time, even when scrolling did not occur.  Modified 
//             OnTimer() to only do so if we actually perform a scrolling operation.
//          -- Fixed bug (noted 07jul2000) regarding tooltip ctrl.  We just avoid calling the base class OnMouseMove() 
//             handler when a drag-n-drop is in progress.
//          -- Updated IsValidDropTarget(), OnMouseMove(), StartDragging(), EndDragging(), and RealizeDrag() to handle 
//             multiple-item drag-n-drop.  By definition, the "dragged items" are all items selected when a drag is 
//             initiated.  The protected variable m_hItemDrag has thus been dropped!  Note that we do not check whether 
//             a particular item is "draggable" or not (fcn CanDrag()) until the drag animation is over and the effect 
//             of the drag is executed with RealizeDrag().  Still need to finish changes to RealizeDrag() and test. 
// 11jul2000-- Updated RealizeDrag() to handle multiple-item drag.  Also added PreRealizeDrag(), which removes any 
//             invalid items -- including items that fail the CanDrag() screen -- and prepares the "valid drag list" 
//             which is then passed to RealizeDrag().  PreRealizeDrag() also clears all selected items, and 
//             RealizeDrag() selects any item that is added to the tree -- so the effect is a shift in the selection 
//             from the items that were initially dragged to the items that are added as a result of the move/copy. 
//          -- VERSION 1.0 COMPLETE.
// 19jul2000-- Added inline function ItemExists() to check for stale tree item handles.  Used this function in the  
//             Can***() overridables. 
// 20jul2000-- ItemExists() did not work and was removed.  In the version of MFC provided with VC++5.0, there's really 
//             no easy way to test for a stale (i.e., no longer existing) tree item handle.
//          -- Modified GetSelectedList() to provide the option to deselect all descendants of a selected item so that 
//             they are not included in the selected list.  The DELETE key-initiated removal of selected items uses 
//             this option to prevent an attempt to delete an already deleted item, which could happen if an item and 
//             one or more of its descendants are both selected.  See PreTranslateMessage(). 
// 09aug2000-- Replaced recursive methods CopyBranch() and SelectChildren() with non-recursive versions.
// 16aug2000-- Added mechanism for customizing the response to a left double-click in the tree view:  OnLButtonDblClk() 
//             and CustomDblClk().  Derived classes override CustomDblClk() to customize.  However, toggling of the 
//             expand/collapse state of parent items is still performed. 
// 17aug2000-- Implemented standard diagnostic routines Dump() and AssertValid().  The latter fcn merely calls the base 
//             class version, since there's nothing extra we can check for CMultiDragTreeView.  Dump() provides either 
//             a brief summary of the tree view contents, or it will do a more comprehensive dump of each item in tree. 
//          -- Cosmetic reorg.
// 15sep2000-- Revision in context menu support.  RunContextMenu() is no longer an overridable.  Instead, it calls the 
//             overridable GetContextMenu(), which loads the popup menu to be displayed.  Also enhanced context menu 
//             support by providing an "applicable item" to GetContextMenu() -- this is the item right-clicked or the 
//             item focused and selected upon keyboard invokation of the context menu (it could be NULL).
// 18sep2000-- RealizeDelete() modified to delete all items in a selected list rather than a single item. 
// 31aug2017-- UINT -> UINT_PTR and other changes to support 64-bit compilation in VS2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include <windowsx.h>                        // for GET_*_LPARAM macros 

#include "mdrgtree.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// PRIVATE CONSTANTS/MACROS/GLOBALS 
//===================================================================================================================== 
// 
const UINT ID_MDTV_MOVE       =     32768;         // right-drag menu options 
const UINT ID_MDTV_COPY       =     32769;
const UINT ID_MDTV_CANCEL     =     32770;



IMPLEMENT_DYNCREATE( CMultiDragTreeView, CTreeView )


// NOTE the use of the ON_NOTIFY_REFLECT macro to reflect certain tree view control notifications back to the control. 
// otherwise, they would be sent to the parent window containing the control.  the tricky thing to keep in mind here is 
// that the "control" and the "view" are the same CWnd.  thus, we use ON_NOTIFY_REFLECT rather than ON_NOTIFY!
//
BEGIN_MESSAGE_MAP( CMultiDragTreeView, CTreeView )
   ON_WM_MOUSEMOVE()
   ON_WM_LBUTTONDBLCLK()
   ON_WM_LBUTTONUP()
   ON_WM_LBUTTONDOWN() 
   ON_WM_RBUTTONUP()
   ON_WM_RBUTTONDOWN() 
   ON_WM_TIMER()
   ON_WM_KEYDOWN() 
   ON_WM_SETFOCUS() 
   ON_WM_KILLFOCUS() 
   ON_NOTIFY_REFLECT( TVN_BEGINLABELEDIT, OnBeginLabelEdit )
   ON_NOTIFY_REFLECT( TVN_ENDLABELEDIT, OnEndLabelEdit ) 
   ON_NOTIFY_REFLECT( TVN_ITEMEXPANDED, OnItemExpanded ) 
   ON_NOTIFY_REFLECT( TVN_ITEMEXPANDING, OnItemExpanding ) 
   ON_NOTIFY_REFLECT( NM_RCLICK, OnRClick )
   ON_NOTIFY_REFLECT( TVN_BEGINDRAG, OnBeginDrag ) 
   ON_NOTIFY_REFLECT( TVN_BEGINRDRAG, OnBeginRDrag ) 
   ON_COMMAND_RANGE( ID_MDTV_MOVE, ID_MDTV_CANCEL, OnRightDragOption ) 

END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

CMultiDragTreeView::CMultiDragTreeView()
{
   m_hItemDrop = NULL; 

   m_pilDrag = NULL;
   m_nTimerID = 0; 
   m_nHover = 0; 
   m_bRtDrag = FALSE;

   CWinApp* pApp = AfxGetApp();
   m_hDropCursor = pApp->LoadStandardCursor( IDC_ARROW );
   m_hNoDropCursor = pApp->LoadStandardCursor( IDC_NO );

   m_bMulti = TRUE;                          // multiple selection initially enabled 
   m_hSelect = NULL; 
   m_bEmulated = FALSE;
}

CMultiDragTreeView::~CMultiDragTreeView() 
{ 
   if ( m_nTimerID != 0 )              // ensure that drag animation timer has been released 
      KillTimer( m_nTimerID );
   if ( m_pilDrag != NULL )            // make sure the last drag image list was deallocated 
   {
      delete m_pilDrag;
      m_pilDrag = NULL;
   }
} 



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnMouseMove [base override] ===================================================================================== 
//
//    Response to the WM_MOUSEMOVE message.  Overridden to animate an ongoing drag-n-drop operation. 
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnMouseMove
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   CTreeCtrl& cTree = GetTreeCtrl();         // get reference to underlying tree control 

   if ( IsDragging() )
   {
      CRect clientRect;
      GetClientRect( &clientRect );

      // move the drag image
      //
      CPoint screenPt( point );
      ClientToScreen( &screenPt );
      CImageList::DragMove( screenPt );

      // if cursor is over tree item within the client area, then highlight it as the drop target if it is not already 
      // AND if it is not one of the dragged items (multi-selection case).  if the cursor is NOT physically over an 
      // item, then be sure to clear the previous drop target, if any. 
      //
      // NOTE that we require the cursor to be over the item label or its associated bitmap (if any) 
      //
      HTREEITEM hItem = NULL;
      if ( clientRect.PtInRect( point ) )
      {
         UINT flags;
         hItem = cTree.HitTest( point, &flags ); 
         if ( hItem && 
              ( (GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED) ||
                !(flags & TVHT_ONITEM) )
            ) 
            hItem = NULL;
         if ( (m_hItemDrop != hItem) )
         {
            CImageList::DragShowNolock( FALSE );
            cTree.SelectDropTarget( hItem );          // should not affect TVIS_SELECTED state of items...
            m_hItemDrop = hItem;
            CImageList::DragShowNolock( TRUE );
         }
      }

      // modify the cursor's appearance to give the user feedback on whether or not the current drop target is 
      // valid; the two alternative cursors must be non-NULL if we are to do this...
      //
      if ( m_hDropCursor && m_hNoDropCursor )
      {
         if ( IsValidDropTarget( hItem ) )
            SetCursor( m_hDropCursor );
         else
            SetCursor( m_hNoDropCursor );
      }

   }
   else
      CTreeView::OnMouseMove( nFlags, point );        // if we're not animating drag, pass on to base class 
}


//=== OnLButtonDblClk ================================================================================================= 
// 
//    Response to the WM_LBUTTONDBLCLK message.  Overridden to customize the action resulting from a left-button 
//    double-click via the virtual handler fcn CustomDblClick(). 
//
//    We let the native tree control complete the default processing, which will toggle the expand/collapse state of 
//    parent items in the tree.  We then pass to CustomDblClick() the handle of the tree item that was double-clicked 
//    (or NULL if none). 
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnLButtonDblClk
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   CTreeView::OnLButtonDblClk( nFlags, point );                // complete native tree control processing 

   CTreeCtrl& cTree = GetTreeCtrl();                           // get reference to underlying tree control 

   UINT flags;
   HTREEITEM hti = cTree.HitTest( point, &flags );             // was an item actually double-clicked? 
   if ( (flags & TVHT_ONITEM) == 0 ) hti = NULL;               //    ...must be ON the item

   CustomDblClk( hti );                                        // customize response to dbl-click    
}


//=== OnLButtonUp [base override] ===================================================================================== 
//
//    Response to the WM_LBUTTONUP message.  Overridden to end an ongoing left-button drag-n-drop operation.  
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnLButtonUp
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   EndDragging( point, m_bRtDrag );
   CTreeView::OnLButtonUp( nFlags, point );
}


//=== OnLButtonDown [base override] =================================================================================== 
//
//    Response to the WM_LBUTTONDOWN message.  Overridden to handle multiple-selection via SHIFT or CTRL keys, or via 
//    a banding rectangle (when mousedown occurs in white space).  
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnLButtonDown
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   OnButtonDown( TRUE, nFlags, point );
}


//=== OnRButtonUp [base override] ===================================================================================== 
//
//    Response to the WM_RBUTTONUP message.  Overridden to end an ongoing right-button drag-n-drop operation.  Also 
//    detects
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnRButtonUp
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   EndDragging( point, m_bRtDrag );
   CTreeView::OnRButtonUp( nFlags, point );
}


//=== OnRButtonDown [base override] =================================================================================== 
//
//    Response to the WM_RBUTTONDOWN message.  Overridden to handle multiple-selection via SHIFT or CTRL keys, or via 
//    a banding rectangle (when mousedown occurs in white space).  
//
//    ARGS:       nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnRButtonDown
   (
   UINT nFlags, 
   CPoint point
   ) 
{
   OnButtonDown( FALSE, nFlags, point );
}


//=== OnTimer [base override] ========================================================================================= 
//
//    Response to the WM_TIMER message.  Overridden to implement auto-scrolling and auto-expansion of collapsed parent 
//    items during an ongoing drag-n-drop operation. 
//
//    NOTE:  We must hide the drag image whenever we update the underlying window as a result of an auto-scroll or 
//    auto-expand.  In the case of scrolling, we take care to verify that a scroll must be performed before hiding the 
//    drag image -- otherwise we observe unpleasant flashing of the drag image!
//
//    ARGS:       nIDEvent -- [in] timer identifier
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnTimer(UINT_PTR nIDEvent) 
{
   CTreeCtrl& cTree = GetTreeCtrl();         // get reference to underlying tree control 
   
   // pass it on if it's not our drag-n-drop timer 
   //
   if ( nIDEvent != m_nTimerID )
   {
      CTreeView::OnTimer( nIDEvent );
      return;
   }

   // get current mouse pos in screen coords and move the current drag image to that pos
   //
   CPoint mousePt;
   ::GetCursorPos( &mousePt ); 
   CImageList::DragMove( mousePt );

   // scroll tree control up or down, and left or right as needed:  if the cursor is above client area, scroll up; if 
   // below, scroll down; etc.  we unlock the drag image during a scroll, and ONLY IF a scroll actually performed --
   // else we get undesirable flashing of drag image when it's just standing still. 
   //
   CRect clientRect; 
   GetClientRect( &clientRect );
   ClientToScreen( &clientRect );

   DWORD dwStyle = GetStyle();                                 // to check for standard window scrolls 
   SCROLLINFO scrInfo;                                         // to obtain current scroll bar information
   scrInfo.cbSize = sizeof( SCROLLINFO );
   scrInfo.fMask = SIF_ALL;

   BOOL bScrollUp = FALSE;                                     // vertical scroll:
   BOOL bScrollDn = FALSE;
   CScrollBar* pScroll = GetScrollBarCtrl( SB_VERT );          // if there is a vertical scroll bar...
   if ( ((pScroll != NULL) && pScroll->IsWindowEnabled()) ||
        ((pScroll == NULL) && (dwStyle & WS_VSCROLL))
      )
   {
      GetScrollInfo( SB_VERT, &scrInfo, SIF_ALL );
      int nLim = scrInfo.nMax - __max( scrInfo.nPage-1, 0 );
      if ( (scrInfo.nMin < scrInfo.nMax) &&                    // if vertical scroll bar is movable...
           (scrInfo.nPage < UINT(scrInfo.nMax-scrInfo.nMin))
         ) 
      {
         bScrollUp = BOOL( (mousePt.y < clientRect.top) &&     // ...check to see if we can move up or down 
                           (scrInfo.nPos > scrInfo.nMin) );
         bScrollDn = BOOL( (mousePt.y >= clientRect.bottom) &&
                           (scrInfo.nPos < nLim) );
      }
   }

   BOOL bScrollLf = FALSE;                                     // horizontal scroll:
   BOOL bScrollRt = FALSE;
   pScroll = GetScrollBarCtrl( SB_HORZ );                      // if there is a horizontal scroll bar...
   if ( ((pScroll != NULL) && pScroll->IsWindowEnabled()) ||
        ((pScroll == NULL) && (dwStyle & WS_HSCROLL))
      )
   {
      GetScrollInfo( SB_HORZ, &scrInfo, SIF_ALL );
      int nLim = scrInfo.nMax - __max( scrInfo.nPage-1, 0 );
      if ( (scrInfo.nMin < scrInfo.nMax) &&                    // if horizontal scroll bar is movable...
           (scrInfo.nPage < UINT(scrInfo.nMax-scrInfo.nMin))
         ) 
      {
         bScrollLf = BOOL( (mousePt.x < clientRect.left) &&    // ...check to see if we can move left or right 
                           (scrInfo.nPos > scrInfo.nMin) );
         bScrollRt = BOOL( (mousePt.x >= clientRect.right) &&
                           (scrInfo.nPos < nLim) );
      }
   }

   
   if ( bScrollUp || bScrollDn || bScrollLf || bScrollRt ) 
   {
      CImageList::DragShowNolock( FALSE );

      if ( bScrollUp || bScrollDn )  cTree.SendMessage( WM_VSCROLL, (bScrollUp) ? SB_LINEUP : SB_LINEDOWN );
      if ( bScrollLf || bScrollRt )  cTree.SendMessage( WM_HSCROLL, (bScrollLf) ? SB_LINELEFT : SB_LINERIGHT );

      CImageList::DragShowNolock( TRUE ); 
   }

   // if the current drop target item has children and is collapsed on three successive timer events, then we expand 
   // that item.
   //
   // DEV NOTE:  the idea here is that the user hovers the mouse over the target item for a little while.  it is 
   // conceivable, however, that the mouse "happens" to be on different drop targets -- all of which have childen and 
   // are collapsed -- at each timer expiration.  this would cause the third drop target item to expand rather 
   // suddenly.  in practice, this is very unlikely.
   //
   if ( !m_hItemDrop ) 
   {
      m_nHover = 0;
      return;
   }
   if ( m_nHover == 3 )
   {
      m_nHover = 0;
      CImageList::DragShowNolock( FALSE );
      ExpandEx( m_hItemDrop, TVE_EXPAND );
      UpdateWindow();                        // update the window NOW; we don't want painting to occur once we relock! 
      CImageList::DragShowNolock( TRUE );
   }
   else
   {
      UINT s = GetItemState( m_hItemDrop, TVIS_EXPANDED ) & TVIS_EXPANDED;
      if ( (s != TVIS_EXPANDED) && (cTree.GetChildItem( m_hItemDrop ) != NULL) )
         m_nHover++;
      else
         m_nHover = 0;
   }

}


//=== OnSetFocus/OnKillFocus [base overrides] ========================================================================= 
//
//    Responses to the WM_SETFOCUS/WM_KILLFOCUS messages.  
//
//    When a tree control window gains(loses) the focus, the "selected" item is either ungreyed(greyed).  In our 
//    multi-select tree control, we are overriding the native tree control behavior to select multiple items; the 
//    native "selected" item is the "focus" item when multi-select is enabled.  To ensure that *all* selected items are 
//    ungreyed(greyed), we invalidate their item label rects.  This causes the native WM_PAINT handler to be called, 
//    which must ungrey(grey) the label rectangle when the window gains(loses) the focus. 
//
//    NOTE:  If TVS_SHOWSELALWAYS style is not in effect, the selected items are unhighlighted(highlighted) rather than 
//    ungreyed(greyed).
//
//    ARGS:       pOldWnd/pNewWnd -- [in] the CWnd which is gaining or losing the focus 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnSetFocus
   (
   CWnd* pOldWnd
   ) 
{
   CTreeCtrl& cTree = GetTreeCtrl();                  // get reference to underlying tree control 

   CTreeView::OnSetFocus( pOldWnd );                  // first call the base version 
   if ( m_bMulti )                                    // invalidate the label rects of all selected items 
   {
      HTREEITEM hItem = GetFirstSelectedItem();
      CRect rect;
      while ( hItem ) 
      {
         cTree.GetItemRect( hItem, &rect, TRUE );
         cTree.InvalidateRect( &rect );
         hItem = GetNextSelectedItem( hItem );
      }
   }
}

void CMultiDragTreeView::OnKillFocus
   (
   CWnd* pNewWnd
   ) 
{
   CTreeCtrl& cTree = GetTreeCtrl();                  // get reference to underlying tree control 

   CTreeView::OnKillFocus( pNewWnd );                 // first call the base version 
   if ( m_bMulti )                                    // invalidate the label rects of all selected items 
   {
      HTREEITEM hItem = GetFirstSelectedItem();
      CRect rect;
      while ( hItem ) 
      {
         cTree.GetItemRect( hItem, &rect, TRUE );
         cTree.InvalidateRect( &rect );
         hItem = GetNextSelectedItem( hItem );
      }
   }
}


//=== OnKeyDown [base override] ======================================================================================= 
//
//    Response to the WM_KEYDOWN message.  
//
//    Overridden to handle changes in the multi-selection state when the up/down arrow keys are depressed.  When the 
//    SHIFT key is depressed with an arrow key, the SHIFT-selected selection range is updated appropriately.  When the 
//    CTRL key is depressed with an arrow key, the focus item is moved up/down *without being selected*.  If neither of 
//    these keys is depressed, any previous multi-selection (including any ongoing SHIFT-select) is reset, and the 
//    default behavior occurs (focus and selection shifts up/down one item).  
//
//    DEV NOTE:  While the user could depress both SHIFT and CTRL at the same time, we do not handle that situation 
//    here. 
//
//    ARGS:       nChar    -- [in] character key code
//                nRepCnt  -- [in] keystroke repeat count 
//                nFlags   -- [in] flags indicate whether various virtual keys are down
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnKeyDown
   (
   UINT nChar, 
   UINT nRepCnt, 
   UINT nFlags
   ) 
{
   CTreeCtrl& cTree = GetTreeCtrl();                     // get reference to underlying tree control 

   if ( !m_bMulti )                                      // if multi-selection disabled, let native control handle it 
   {
      CTreeView::OnKeyDown( nChar, nRepCnt, nFlags );
      return;
   }

   BOOL bCtrl = (GetKeyState( VK_CONTROL ) & 0x8000);    // state of CTRL and SHIFT keys 
   BOOL bShift = (GetKeyState( VK_SHIFT ) & 0x8000);     // 

   BOOL bDir = FALSE;                                    // arrow up (TRUE) or down (FALSE) 
   HTREEITEM hFocus = NULL;                              // item with the focus (could be NULL) 
   switch( nChar ) 
   {
      case VK_UP:
         bDir = TRUE;                                    // !!! FALL THROUGH !!! 

      case VK_DOWN:
         hFocus = GetFocusedItem();                      // get current focus item

         if ( !m_hSelect )                               // if there isn't a SHIFT-select base item yet, then set it to 
            m_hSelect = hFocus;                          //    the current focus item 

         if ( !bCtrl && !bShift )                        // if neither SHIFT nor CTRL is down, clear any previous 
         {                                               //    multi-selection, as well as the SHIFT-select base item. 
            m_hSelect = NULL; 
            SelectAll( FALSE );
         }
         break;
   }

   CTreeView::OnKeyDown( nChar, nRepCnt, nFlags );       // call base version; for VK_UP/DOWN, this will move the 
                                                         //    native selection (both focus and select highlight) to 
                                                         //    the next or previous item as appropriate 

   if ( !hFocus || (!bCtrl && !bShift) )                 // if it wasn't VK_UP/DOWN, or neither CTRL nor SHIFT were on, 
      return;                                            //    then there's nothing else to do! 

   HTREEITEM hNext = bDir ?                              // get the tree item above or below the focus item
      cTree.GetPrevVisibleItem( hFocus ) :
      cTree.GetNextVisibleItem( hFocus );
   if ( !hNext )                                         // can't go beyond the top or bottom of the tree!
      hNext = hFocus;

   if ( bShift )                                         // if SHIFT on, then select all items between the new focus 
      SelectRange( m_hSelect, hNext, TRUE );             //    and the current SHIFT-select base item 
   else if ( bCtrl )                                     // otherwise, if CTRL on, just shift the focus (no select!) 
      SetItemState( hNext, TVIS_FOCUSED, TVIS_FOCUSED );
}


//=== OnBeginLabelEdit ================================================================================================ 
// 
//    Response to reflected TVN_BEGINLABELEDIT notification.
//
//    We call the virtual helper function CanEdit() to determine whether or not the given item may be edited.  If not, 
//    we cancel the in-place edit.
//
//    DEV NOTE:  Eventually modify to limit the total number of characters that may be entered.
//
//    ARGS:       pNMHDR   -- [in] for TVN_BEGINLABELEDIT notification, this is a handle to a TV_DISPINFO struct. 
//                pResult  -- [out] return code.  TRUE to cancel label editing, FALSE to allow it. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnBeginLabelEdit
   (
   NMHDR* pNMHDR,
   LRESULT* pResult
   )
{
   TV_DISPINFO* pTVDI = (TV_DISPINFO*)pNMHDR;            // recast notification header 
   HTREEITEM hItem = pTVDI->item.hItem;                  // the item whose label is to be edited 

   *pResult = BOOL(!CanEdit( hItem ));
}


//=== OnEndLabelEdit ================================================================================================== 
// 
//    Response to reflected TVN_ENDLABELEDIT notification.
//
//    Here we call the virtual function AcceptNewLabel() to accept or reject the new item label.  If accepted, the 
//    new item label is updated here (rather than relying on the tree control to do it for us when we return from this 
//    notification handler), and we call the virtual function Sort() to possibly resort the item and its siblings. 
//
//    NOTE:  The TVM_ENDEDITLABELNOW message is NOT documented in the Platform SDK! 
//
//    ARGS:       pNMHDR   -- [in] for TVN_ENDLABELEDIT notification, this is a handle to a TV_DISPINFO struct. 
//                pResult  -- [out] return code.  TRUE to accept the new text, FALSE to reject. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnEndLabelEdit 
   (
   NMHDR* pNMHDR,
   LRESULT* pResult
   )
{
   TV_DISPINFO* pTVDI = (TV_DISPINFO*)pNMHDR;            // recast notification header 
   HTREEITEM hItem = pTVDI->item.hItem;                  // the item whose label was edited 
   CTreeCtrl& cTree = GetTreeCtrl();                     // get reference to underlying tree control 

   *pResult = TRUE;
   if ( pTVDI->item.pszText != NULL )                    // if NULL, the in-place edit was cancelled  
   {
      CString testStr = LPCTSTR(pTVDI->item.pszText); 
      if ( !AcceptNewLabel( hItem, testStr ) )
         *pResult = FALSE;
      else
      {
         cTree.SetItemText( hItem, LPCTSTR(testStr) );
         ::SendMessage( GetSafeHwnd(), TVM_ENDEDITLABELNOW, WPARAM(TRUE), LPARAM(0) );

         HTREEITEM htiParent = cTree.GetParentItem( hItem ); 
         if ( htiParent == NULL ) htiParent = TVI_ROOT;
         Sort( htiParent ); 
      }
   }

}


//=== OnItemExpanded ================================================================================================== 
// 
//    Response to the reflected TVN_ITEMEXPANDED notification.
//
//    Here we provide an opportunity to change a parent's item bitmap to reflect its expanded or collapsed state.  If 
//    the control has a non-empty image list, it calls the virtual helper function GetExpandBitmaps().  This function 
//    returns the normal & selected bitmap image IDs (pos in image list) that should be associated with the expanded or 
//    collapsed tree item.
//    
//    ARGS:       pNMHDR   -- [in] for TVN_ITEMEXPANDED notification, this is a handle to a NM_TREEVIEW struct
//                pResult  -- [out] return code.  ignored for TVN_ITEMEXPANDED. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnItemExpanded
   (
   NMHDR* pNMHDR,
   LRESULT* pResult
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                        // get reference to underlying tree control 

   *pResult = FALSE;                                        // return code is ignored 
   NM_TREEVIEW* pNMTV = (NM_TREEVIEW*)pNMHDR;               // recast notification header 
   HTREEITEM hItem = pNMTV->itemNew.hItem;                  // the item expanded or collapsed 
   UINT action = pNMTV->action;                             // the action taken 
   action &= TVE_TOGGLE;                                    // we only look for the TVE_COLLAPSE or TVE_EXPAND flags 

   CImageList* pIL = cTree.GetImageList( TVSIL_NORMAL );    // abort if there's no action or no image list installed 
   if ( (!action) || (hItem == NULL) || (pIL == NULL) )
      return;

   BOOL bExpanded = TRUE;                                   // was the item expanded or collapsed? 
   if ( action == TVE_TOGGLE )
   {
      UINT uiState;
      if ( pNMTV->itemNew.stateMask & TVIS_EXPANDED )
         uiState = pNMTV->itemNew.state;
      else
         uiState = GetItemState( hItem, TVIS_EXPANDED ); 
      if ( !(uiState & TVIS_EXPANDED) )
         bExpanded = FALSE;
   }
   else if ( action == TVE_COLLAPSE )
      bExpanded = FALSE;

   int nImage = -1;                                         // get the normal & selected image pos in image list 
   int nSelImage = -1;
   GetExpandBitmaps( hItem, bExpanded, &nImage, &nSelImage ); 

   int imgCount = pIL->GetImageCount();                     // set the item images if valid 
   if ( (nImage >= 0) && (nImage < imgCount) 
        && (nSelImage >= 0) && (nSelImage < imgCount) 
      ) 
      cTree.SetItemImage( hItem, nImage, nSelImage );

}


//=== OnItemExpanding ================================================================================================= 
// 
//    Response to the reflected TVN_ITEMEXPANDING notification.
//
//    If multiple-selection is enabled, we deselect the descendants of a collapsing item.  This enforces the policy 
//    that an item must be visible to be selected.
//
//    If a descendant of the collapsing item had the focus -- which equates to being "selected" as far as the native  
//    tree control is concerned--, the focus is normally transferred to the collapsing parent.  We override this 
//    native behavior as follows:
//       (1)  If the item was already selected, then we let the default behavior occur; the item will obtain the 
//            focus when the collapse is completed by the native tree control. 
//       (2)  If the item was NOT selected but a descendant had the focus, then we prevent it from being selected when 
//            its subtree is collapsed by setting the "focused" item to NULL.  As a result, no item will have the 
//            focus after the collapse is completed. 
//
//    NOTE that we permit all items to expand/collapse.  Since we use ON_NOTIFY_REFLECT to trap this notification, 
//    parent windows will not have an opportunity to override.  Use a derived class to change this behavior. 
//   
//    ARGS:       pNMHDR   -- [in] for TVN_ITEMEXPANDING notification, this is a handle to a NM_TREEVIEW struct
//                pResult  -- [out] return code.  set TRUE to prevent expand/collapse, FALSE to permit. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnItemExpanding
   (
   NMHDR* pNMHDR, 
   LRESULT* pResult
   ) 
{
   *pResult = FALSE;                                           // allow any item to expand/collapse 

   if ( m_bMulti )
   {
      LPNMTREEVIEW pNMTreeView = (LPNMTREEVIEW)pNMHDR;
      if ( (pNMTreeView->action == TVE_COLLAPSE) || 
           (pNMTreeView->action == TVE_COLLAPSERESET) ) 
      {
         ASSERT( pNMTreeView->itemNew.hItem );

         BOOL bWasSel = IsSelected( pNMTreeView->itemNew.hItem );
         BOOL bHadFocus = SelectChildren( pNMTreeView->itemNew.hItem, FALSE, TRUE );
         if ( bHadFocus && !bWasSel )
            GetTreeCtrl().SelectItem(NULL);                    // stop parent from gaining selection; focus item lost 
      }
   }

}


//=== OnRClick ======================================================================================================== 
// 
//    Response to reflected NM_RCLICK notification.
//
//    CTreeCtrl apparently does not receive WM_CONTEXTMENU when the user right-clicks in its client area.  To get the 
//    "standard" behavior -- context menu appearing with a single right-click -- we've added this NM_RCLICK handler to 
//    obtain the current mouse pos and forward it to the virtual handler fcn RunContextMenu().  If the mouse was on top 
//    of an item at the time of the mouse click, then a handle to that item is also forwarded to RunContextMenu().
//
//    ARGS:       pNMHDR   -- [in] handle to a NMHDR struct
//                pResult  -- [out] return code.  ignored for NM_RCLICK. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnRClick
   (
   NMHDR* pNMHDR,
   LRESULT* pResult
   )
{
   CPoint mousePt;                                                   // get current cursor pos in screen coords 
   ::GetCursorPos( &mousePt ); 

   CPoint pt = mousePt;                                              // get item under mouse at time of rt-click 
   ClientToScreen( &pt );
   UINT uiHitFlags = 0; 
   HTREEITEM htiContext = GetTreeCtrl().HitTest( pt, &uiHitFlags );
   if ( !(uiHitFlags & TVHT_ONITEM) ) htiContext = NULL;

   RunContextMenu( this, mousePt, htiContext );                      // popup the context menu 

   *pResult = FALSE;
}


//=== OnBeginDrag, OnBeginRDrag ======================================================================================= 
// 
//    Response to reflected TVN_BEGINDRAG, TVN_BEGINRDRAG notifications.
//
//    Overridden to initiate either a left-button or right-button drag-n-drop animation.  Regardless of which button 
//    initiated the drag, the animation is the same.  While TVN_BEGINDRAG is issued only if the mouse-down was over the 
//    item or its bitmap, TVN_BEGINRDRAG is issued for a mouse-down anywhere on an item's line.  To make the behaviors 
//    identical, we ignore TVN_BEGINRDRAG unless the mouse is actually on top of the item. 
//
//    ARGS:       pNMHDR   -- [in] handle to an NM_TREEVIEW struct
//                pResult  -- [out] return code.  ignored for TVN_BEGINDRAG, TVN_BEGINRDRAG. 
// 
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnBeginDrag
   (
   NMHDR* pNMHDR, 
   LRESULT* pResult
   ) 
{
   NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   *pResult = FALSE;

   m_bRtDrag = FALSE; 
   if ( GetSelectedCount() > 0 )
      StartDragging( pNMTreeView->itemNew.hItem, pNMTreeView->ptDrag );
}

void CMultiDragTreeView::OnBeginRDrag
   (
   NMHDR* pNMHDR, 
   LRESULT* pResult
   ) 
{
   NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   *pResult = FALSE;

   UINT uiHitFlags = 0;                                           // drag point MUST be actually ON the item
   GetTreeCtrl().HitTest( pNMTreeView->ptDrag, &uiHitFlags );
   if ( uiHitFlags & TVHT_ONITEM ) 
   {
      m_bRtDrag = TRUE;
      if ( GetSelectedCount() > 0 )
         StartDragging( pNMTreeView->itemNew.hItem, pNMTreeView->ptDrag ); 
   }
}


//=== OnRightDragOption =============================================================================================== 
// 
//    ON_COMMAND_RANGE message handler which responds to a user selection from the popup menu displayed after a 
//    right-button drag-n-drop.  The following options are supported:
//       ID_MDTV_MOVE            -- Move drag item and its subtree under the drop target. 
//       ID_MDTV_COPY            -- Copy drag item and its subtree under the drop target. 
//       ID_MDTV_CANCEL          -- Cancel the drag operation entirely. 
//
//    ARGS:       cmdID -- [in] command ID identifying which option was selected.
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::OnRightDragOption
   ( 
   UINT cmdID
   )
{
   ASSERT( GetSelectedCount() > 0 );   // there should be at least one item selected (all selected items are dragged) 
   ASSERT( m_hItemDrop );              // the "current" drop item should be valid!

   if ( (cmdID == ID_MDTV_MOVE) || (cmdID == ID_MDTV_COPY) )
   {
      CHTIList htiDragList;                                       // the list of valid drag items
      PreRealizeDrag( htiDragList );                              // constructs drag list and clears selection 
      RealizeDrag( htiDragList, BOOL(cmdID == ID_MDTV_COPY) );    // realize effects of the drag  
   }

   m_hItemDrop = NULL;
   m_bRtDrag = FALSE;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== PreCreateWindow [base override] ================================================================================= 
//
//    Called by MFC framework during window creation.  Provides an opportunity to modify the window creation structure 
//    (CREATESTRUCT) before the Windows object is actually created. 
//
//    Here we set the required tree control styles for CMultiDragTreeView.  It is enabled for drag-n-drop and in-place 
//    editing of item labels. 
//
//    ARGS:       cs -- [in/out] the window creation structure
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CMultiDragTreeView::PreCreateWindow
   ( 
   CREATESTRUCT& cs 
   )
{
   cs.style &= 0xFFFF0000;             // mask out all tree view control styles and set the desired style flags: 
   cs.style |= TVS_HASBUTTONS |        //    includes single-click buttons for expanding parent items 
               TVS_HASLINES |          //    draws lines to illustrate hierarchy
               TVS_LINESATROOT |       //    lines & expand/contract buttons shown at root level 
               TVS_EDITLABELS;         //    allow in-place editing of item labels 
               
   return( CTreeView::PreCreateWindow( cs ) );
}


//=== PreTranslateMessage [base override] ============================================================================= 
//
//    This function allows the window object to intercept a Windows message before it is dispatched to 
//    ::TranslateMessage() and ::DispatchMessage().
//
//    We override the CTreeView version to customize our tree's response to certain virtual keys. 
//
//    ARGS:       pMsg -- [in/out] the Windows message 
//
//    RETURNS:    TRUE if message should not be dispatched by framework; FALSE if it should. 
//
BOOL CMultiDragTreeView::PreTranslateMessage
   (
   MSG* pMsg
   ) 
{
   CTreeCtrl& cTree = GetTreeCtrl();      // get reference to tree control

   // when we're in the middle of a drag operation and receive conflicting mouse or keyboard input, then cancel 
   // the drag operation right away and process the new input.  note that we allow user to depress the CTRL key 
   // during a left drag, since that allows user to copy rather than move the selection. 
   //
   if ( IsDragging() )
   {
      BOOL bCancel = (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && 
                     (m_bRtDrag || (pMsg->wParam != VK_CONTROL));
      if ( !bCancel )
         bCancel = (pMsg->message == WM_CONTEXTMENU);
      if ( !bCancel )
         bCancel = m_bRtDrag && (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP);
      if ( !bCancel )
         bCancel = (!m_bRtDrag) && (pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_RBUTTONUP); 

      if ( bCancel )
         EndDragging( CPoint(-1,-1), FALSE );                  // invalid drop point ensures that drag is cancelled 
   }

   // when an item is being edited in-place, let CTreeView take care of it.  when a tree control is wrapped by 
   // CTreeView, it properly handles the ESC and ENTER keydowns (unlike CTreeCtrl on its own)
   //
   if ( cTree.GetEditControl() != NULL )
      return( CTreeView::PreTranslateMessage( pMsg ) );

   // keyboard invocation of context menu:  SHIFT-F10 or Windows "context" key.  in this case, we must specify the 
   // point where the context menu should appear.  if there is a focused and selected item, we put the menu's ULC near 
   // the center of that item's rect and make that item the "applicable item" wrt the context menu; if not, we put 
   // the menu's ULCnear the center of the client area, and the "applicable item" is undefined. 
   //
   if (     (   (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) 
             && (pMsg->wParam == VK_F10) 
             && (::GetKeyState( VK_SHIFT ) & ~1)
            )
        ||  (pMsg->message == WM_CONTEXTMENU)
      ) 
   {
      CRect rect;
      HTREEITEM htiFocus = GetFocusedItem();
      if ( (htiFocus != NULL) && (GetItemState( htiFocus, TVIS_SELECTED ) & TVIS_SELECTED) )
         cTree.GetItemRect( htiFocus, &rect, TRUE ); 
      else
      {
         htiFocus = NULL;
         GetClientRect( &rect ); 
      }
      ClientToScreen( &rect );

      RunContextMenu( this, rect.CenterPoint(), htiFocus );
      return( TRUE );
   }

   // other virtual keys we handle...
   //
   if ( pMsg->message == WM_KEYDOWN )
   {
      // hitting the ENTER key toggles the expand/collapse state of the focused parent item.  since the native 
      // CTreeCtrl::Expand() fcn fails to issue TVN_ITEMEXPANDING/ED notifications, we CANNOT use it here.  
      //
      if ( pMsg->wParam == VK_RETURN )
      {
         ExpandEx( GetFocusedItem(), TVE_TOGGLE );
         return( TRUE );
      }

      // hitting the delete key deletes the currently selected (and removable) items in the tree
      //
      else if ( pMsg->wParam == VK_DELETE )
      {
         CHTIList htiSelList;
         GetSelectedList( htiSelList, TRUE );      // deselect descendants of selected items, since they will be 
                                                   // deleted when their ancestor is! 
         RealizeDelete( htiSelList );
         return( TRUE );
      }

      // hitting the BACKSPACE key moves the focus and selects the parent (if any).  if CTRL is down, the parent is 
      // added to the current multi-selection.  if SHIFT is down, we select all visible items from the SHIFT-select 
      // base item to the parent.  if neither key is down, any previous multi-selection is cleared. 
      //
      else if ( pMsg->wParam == VK_BACK ) 
      {
         HTREEITEM htiParent = cTree.GetParentItem( GetFocusedItem() );
         if ( htiParent != NULL )
         {
            SetItemState( htiParent, TVIS_FOCUSED|TVIS_SELECTED, TVIS_FOCUSED|TVIS_SELECTED );

            BOOL bShift = ::GetKeyState( VK_SHIFT ) & ~1;
            BOOL bCtrl = ::GetKeyState( VK_CONTROL ) & ~1; 
            if ( !bShift && !bCtrl )
               SelectAllIgnore( FALSE, htiParent );
            else if ( bShift )
            {
               if ( m_hSelect )
                  SelectRange( htiParent, m_hSelect );
               else 
                  m_hSelect = htiParent;
            }
         }
         return( TRUE );
      }

      // hitting the F2 key invokes in-place edit of the current focus item (if any)
      //
      else if ( pMsg->wParam == VK_F2 )
      {
         HTREEITEM htiFocus = GetFocusedItem();
         if ( htiFocus != NULL )
            cTree.EditLabel( htiFocus );
         return( TRUE );
      }
   }

   // OTHERWISE, we let the base class handle the message 
   //
   return( CTreeView::PreTranslateMessage( pMsg ) );
}


//=== SetMultiSelect ================================================================================================== 
//
//    Toggle multi-selection mode on/off. 
//
//    ARGS:       bMulti -- [in] TRUE to enable multi-selection, FALSE to disable. 
//
//    RETURNS:    previous state (TRUE for enabled, FALSE for disabled).
//
BOOL CMultiDragTreeView::SetMultiSelect
   (
   BOOL bMulti
   )
{
   if ( (bMulti && m_bMulti) || (!bMulti && !m_bMulti) )    // no change!
      return( m_bMulti );

   if ( !bMulti )                                           // if disabling, restore tree to single-select state: 
   {
      HTREEITEM hItem = GetFocusedItem();                   // the current focus item 
      if ( hItem && !IsSelected( hItem ) )                  // if focus item exists but is not selected, we want to 
         hItem = NULL;                                      //    deselect it.
      SelectAllIgnore( FALSE, hItem );                      // deselect all except focus item (IF it was selected) 
      if ( hItem )                                          // if there was a focused & selected item, it becomes the 
         GetTreeCtrl().SelectItem( hItem );                 //    one & only selected item. 
   }

   BOOL b = m_bMulti;                                       // change mode and return old mode 
   m_bMulti = bMulti;
   return( b );
}


//=== GetSelectedCount ================================================================================================ 
//
//    Returns number of currently selected items in tree
//
//    ARGS:       NONE
//
//    RETURNS:    # of selected items (could be 0)
//
UINT CMultiDragTreeView::GetSelectedCount() const
{
   UINT nCount = 0;

   if ( m_bMulti )                                             // multi-sel off; count all selected items 
   {
      HTREEITEM hItem = GetFirstSelectedItem();
      while ( hItem ) 
      {
         nCount++;
         hItem = GetNextSelectedItem( hItem );
      }
   }
   else                                                        // multi-sel off; there's at most one selection 
   {
      if ( GetTreeCtrl().GetSelectedItem() != NULL ) nCount++;
   }

   return( nCount );
}


//=== GetFirstSelectedItem, GetNextSelectedItem ======================================================================= 
//
//    For enumerating the selected items in a tree. 
//
//    DEV NOTE:  Enumerating with CTreeCtrl's GetNextVisibleItem() searches all nodes that are currently expanded; it 
//    has nothing to do with whether the item is actually "visible" to the user!
//
//    ARGS:       GetFirstSelectedItem:   NONE
//                GetNextSelectedItem:    hti   -- [in] handle of a tree item. 
//
//    RETURNS:    handle of first/next selected item in tree; NULL if no such item is found. 
//
HTREEITEM CMultiDragTreeView::GetFirstSelectedItem() const
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // get reference to embedded tree control

   HTREEITEM hti = cTree.GetRootItem(); 
   if ( hti != NULL )
   {
      if ( !IsSelected( hti ) ) hti = GetNextSelectedItem( hti );
   }
   return( hti );
}

HTREEITEM CMultiDragTreeView::GetNextSelectedItem
   (
   HTREEITEM hti
   ) const
{
   CTreeCtrl& cTree = GetTreeCtrl();               // get reference to embedded tree control

   hti = cTree.GetNextVisibleItem( hti );
   while ( hti ) 
   {
      if ( IsSelected( hti ) ) break;
      hti = cTree.GetNextVisibleItem( hti );
   }
   return( hti );
}


//=== GetSelectedList ================================================================================================= 
//
//    Obtain a list of HTREEITEM handles for all currently selected items in the tree.  Optionally deselects any 
//    descendants of a selected item so that they are not also included in the list. 
//
//    DEV NOTE:  We make one pass through the current selection set to deselect descendants of all selected items, and 
//    then a second pass to build the list of remaining selected items.  This may seem like overkill, since the 
//    routines which iterate over the selected items are based on MFC's GetFirst/NextVisibleItem(), which search the 
//    tree from top to bottom, going down the subtree of any expanded item before moving on to its next sib.  However, 
//    we are not **guaranteed** a particular search order!!  If we built the list and deselected descendants in one 
//    pass, we would include in the selected list any selected descendants that came before their parent in the search! 
//
//    ARGS:       htiList     -- [out] the current selection set (a singly-linked list). 
//                bNoDescend  -- [in] if TRUE, descendants of selected items are deselected and not included in list. 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::GetSelectedList
   (
   CHTIList& htiList,
   BOOL bNoDescend      // = FALSE 
   ) 
{
   htiList.RemoveAll();                                              // initialize selected list as empty 

   HTREEITEM hItem;
   if ( bNoDescend )                                                 // optional first pass to deselect descendants of 
   {                                                                 // each selected item 
      hItem = GetFirstSelectedItem();
      while ( hItem ) 
      {
         BOOL bFocusWasHere = SelectChildren( hItem, FALSE, TRUE ); 
         if ( bFocusWasHere ) FocusItem( hItem );
         hItem = GetNextSelectedItem( hItem );
      }
   }

   hItem = GetFirstSelectedItem();                                   // second pass to put remaining selected items in 
   while ( hItem )                                                   // the list 
   {
      htiList.AddTail( hItem ); 
      hItem = GetNextSelectedItem( hItem );
   }
}


//=== SelectAll ======================================================================================================= 
//
//    Select or deselect all visible items in the tree (i.e., all items that are part of an expanded subtree).
//
//    ARGS:       bSelect -- [in] if TRUE, all items are selected; otherwise, any selected items are deselected. 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::SelectAll
   (
   BOOL bSelect   // = TRUE 
   )
{
   SelectAllIgnore( bSelect, NULL );
}


//=== SelectRange ===================================================================================================== 
//
//    Select or deselect all visible items in the tree between the specified items (inclusive). 
//
//    ARGS:       hFirst, hLast  -- [in] first and last items in range (reversed order is OK; both MUST be visible).  
//                bOnly          -- [in] if TRUE, deselect any visible items not in the specified range. 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::SelectRange
   (
   HTREEITEM hFirst, 
   HTREEITEM hLast, 
   BOOL bOnly        // = TRUE 
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                                 // get reference to embedded tree control

   HTREEITEM hItem = cTree.GetRootItem();                            // locate & select either first or last item
   while (hItem)                                                     // (so we can handle reversed order)
   {
      if ( (hItem == hFirst) || (hItem == hLast) ) 
      {
         if ( hFirst != hLast )                                      // if range of 1, that one item is selected later 
         {
            if ( !IsSelected( hItem ) )
               SetItemState( hItem, TVIS_SELECTED, TVIS_SELECTED );
            hItem = cTree.GetNextVisibleItem( hItem );
         }
         break;
      }

      if ( bOnly && IsSelected( hItem ) )                            // if requested, deselect items not in range
         SetItemState(hItem, 0, TVIS_SELECTED);

      hItem = cTree.GetNextVisibleItem(hItem);
   }

   while( hItem )                                                    // select all items until we reach other end of 
   {                                                                 // range
      if ( !IsSelected( hItem ) )
         SetItemState( hItem, TVIS_SELECTED, TVIS_SELECTED );

      if ( (hItem == hFirst) || (hItem == hLast) ) 
      {
         hItem = cTree.GetNextVisibleItem(hItem);
         break;
      }

      hItem = cTree.GetNextVisibleItem(hItem);
   }

   if ( !bOnly ) return;                                             // if requested, deselect any remaining selected 
                                                                     // items that are outside the range 
   while( hItem ) 
   {
      if ( IsSelected( hItem ) )
         SetItemState( hItem, 0, TVIS_SELECTED );
      hItem = cTree.GetNextVisibleItem( hItem );
   }

}


//=== SelectChildren ================================================================================================== 
//
//    Select or deselect all immediate children or all expanded descendants of parent item. 
//
//    NOTES:
//    (1) The original version of this method (R. Hazelwood's CMultiTree class) was recursive.  I decided to avoid 
//    recursion in case this class was ever used with tree views having many hierarchical levels.  This non-recursive 
//    version traverses the specified parent's subtree in the same standard order.  Let A, B,... be the immediate 
//    children of the parent node, in sibling order.  We visit node A, then all of node A's children in standard order, 
//    then node B, etc...
//    (2) Only expanded descendants are affected; by design, no items in a collapsed subtree can be selected!
//
//    ARGS:       hParent  -- [in] handle of specified parent.   
//                bSelect  -- [in] if TRUE, select; if FALSE, deselect. 
//                bAll     -- [in] if TRUE, traverse all expanded descendants; else traverse immediate children only. 
//
//    RETURNS:    TRUE if a descendant had the focus, FALSE otherwise. 
//
BOOL CMultiDragTreeView::SelectChildren
   (
   HTREEITEM hParent, 
   BOOL bSelect,        // = TRUE 
   BOOL bAll            // = TRUE 
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                                    // get reference to embedded tree control
   UINT nS = bSelect ? TVIS_SELECTED : 0;                               // select or deselect
   BOOL bFocusWasInHere = FALSE;                                        // set TRUE if child had focus 

   HTREEITEM hItem = cTree.GetChildItem( hParent );                     // traverse items in parent node's "subtree"... 
   int nest = 1;                                                        // (current nest level in subtree) 
   while( (hItem != NULL) && (nest > 0)  ) 
   {
      UINT nState = GetItemState( hItem,                                // ...get item's select/expand/focus state  
                        TVIS_SELECTED|TVIS_EXPANDED|TVIS_FOCUSED);

      if ( (nState & TVIS_SELECTED) != nS )                             // ...adjust select state as needed 
         SetItemState(hItem, nS, TVIS_SELECTED);

      bFocusWasInHere |= (nState & TVIS_FOCUSED);                       // ...set flag if item had focus 

      if ( bAll && (nState & TVIS_EXPANDED) )                           // ...traverse item's subtree if expanded
      {
         ++nest;
         hItem = cTree.GetChildItem( hItem );
      }
      else                                                              // ...else move on to next sibling, unwinding
      {                                                                 //    if we reach end of a sibling list:
         HTREEITEM hNext = cTree.GetNextSiblingItem( hItem ); 
         while( (hNext == NULL) && (nest > 0) )
         {
            --nest;
            hItem = cTree.GetParentItem( hItem );
            hNext = cTree.GetNextSiblingItem( hItem );
         }
         hItem = hNext;
      }
   }
   return( bFocusWasInHere );
}


//=== FocusItem ======================================================================================================= 
//
//    Transfer the focus to specified item.  If multi-selection is disabled, then this function makes the specified 
//    item the current selection. 
//
//    ARGS:       hti   -- [in] the item which gains the focus; if NULL, the focus is lost.    
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CMultiDragTreeView::FocusItem
   (
   HTREEITEM hti
   )
{
   BOOL bRet = FALSE;

   if ( m_bMulti )                                                      // in multi-selection mode, we...
   {
      if ( hti )                                                        // transfer "focus" only...
         bRet = SetItemState( hti, TVIS_FOCUSED, TVIS_FOCUSED );
      else                                                              // ...or remove focus entirely
      {
         hti = GetFocusedItem();
         if ( hti )
            bRet = SetItemState( hti, 0, TVIS_FOCUSED );
      }
   }
   else                                                                 // in single-selection mode, we transfer the 
      bRet = SelectItem( hti );                                         // selection 

   return( bRet );
}


//=== SelectItem ====================================================================================================== 
//
//    Select the specified item.  If multi-selection is enabled, then the specified item is selected without affecting 
//    the focus item or any other selected items; a NULL item is ignored.  If multi-selection is disabled, then the 
//    native tree control selection method is invoked; if the item is NULL, the current selection is cleared. 
//
//    !!! IMPORTANT !!!  Use instead of CTreeCtrl method SelectItem().  But beware of different behavior when 
//    multi-selection is enabled.
//
//    ARGS:       hti   -- [in] the item to be selected.    
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CMultiDragTreeView::SelectItem
   (
   HTREEITEM hti 
   )
{
   BOOL bRet = FALSE;

   if ( m_bMulti )                                                      // in multi-selection mode, we...
   {
      if ( hti && !IsSelected( hti ) )                                  // select the specified item if it is not
         bRet = SetItemState( hti, TVIS_SELECTED, TVIS_SELECTED );      // already selected.  NULL items are ignored. 
   }
   else                                                                 // in single-selection mode, we...
      bRet = GetTreeCtrl().SelectItem( hti );                           // call the native native method.  

   return( bRet );
}


//=== SetItemState ==================================================================================================== 
//
//    Update the state of specified tree item.
//
//    !!! IMPORTANT !!!  Use instead of CTreeCtrl method SetItemState().  This handles the normal functionality of 
//    the native method in single-select mode, as well as implementing the TVIS_FOCUSED state and multi-selection. 
//
//    The "focus" item in the native tree control is the selected item.  In order to implement a separate TVIS_FOCUSED 
//    state and select multiple items, we have to bypass some of the behavior of the native control.  This is kind of 
//    tricky stuff, so peruse carefully.  Note that we use the native SelectItem() method to move the focus.  This will 
//    generate TVN_SELCHANGING(ED) notifications.  Also, when we change the selection state of an item in multi-select 
//    mode, we must emulate these same notifications.  For parent windows to properly decipher what has actually 
//    happened, they must call IsEmulatedNotify() to determine whether the notification was generated by the native 
//    control or emulated.  Then (E = IsEmulatedNotify(); O = itemOld.hItem != NULL; N = itemNew.hItem != NULL):
//
//    E  O  N
//    ~~~~~~~
//    0  1  0     A focus loss on itemOld
//    0  0  1     A focus/selection gain on itemNew
//    0  1  1     A focus loss on itemOld, a focus/selection gain on itemNew
//    1  1  0     A selection loss on itemOld
//    1  0  1     A selection gain on itemNew
// 
//
//    ARGS:       hItem       -- [in] handle of tree item. 
//                nState      -- [in] specifies new states for item. 
//                nStateMask  -- [in] which states are to be modified. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CMultiDragTreeView::SetItemState
   (
   HTREEITEM hItem, 
   UINT nState, 
   UINT nStateMask
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // get reference to embedded tree control

   ASSERT( hItem );

   if ( !m_bMulti )                                               // if multi-selection off, just call native method 
      return( cTree.SetItemState( hItem, nState, nStateMask ) );  //    and we're done! 


   HTREEITEM hFocus = GetFocusedItem();                           // get item which currently has the focus
   BOOL bHadFocus = (hFocus == hItem);                            // specified item also had the focus?
   BOOL bFocusWasSel = hFocus && IsSelected( hFocus );            // old focus item was also selected?
   BOOL bWasSel = IsSelected( hItem );                            // specified item was selected?

   UINT nS = nState & ~TVIS_FOCUSED;                              // state & state mask without the TVIS_FOCUSED bit 
   UINT nSM = nStateMask & ~TVIS_FOCUSED;

   if ( nStateMask & TVIS_FOCUSED )                               // STEP 1:  Handle TVIS_FOCUSED state...
   {
      if ( nState & TVIS_FOCUSED )                                // BEGIN -- set focus to this item:
      {
         if ( !bHadFocus && bFocusWasSel )                        // transfer focus but keep old focus item selected
         {
                                                                  // because native SelectItem() would deselect the 
                                                                  // current 'real' selection (one with focus), need to 
                                                                  // make tree ctrl think there is no 'real' selection,
                                                                  // but still keep the the old item selected. 
                                                                  // otherwise the TVN_SELCHANGING/ED notification 
                                                                  // handlers wouldn't be able to get the proper list 
                                                                  // of selected items...
            cTree.SelectItem( NULL );                             // will notify, but is taken as focus loss
            cTree.SetItemState( hFocus, TVIS_SELECTED,            // reselect old focus item (notif. NOT generated)
                                        TVIS_SELECTED );
            UpdateWindow();
         }

         if ( !cTree.SelectItem( hItem ) )                        // set focus to specified item.  this will fail if 
            return( FALSE );                                      // parent traps TVN_SELCHANGING and denies change. 
            
                                                                  // the call above will also select item, if not 
                                                                  // already focused.  thus, we may have to fix 
                                                                  // selected state.  we take care of that now:

         if ( nStateMask & TVIS_SELECTED )                        // if we wanted to alter the select state...
         { 
            if ( nState & TVIS_SELECTED )                         // ... and we wanted to select item...
            {                                                     
               if ( !bHadFocus || bFocusWasSel )                  //    if item did not already have focus, or the old
               {                                                  //    focus item was selected, then the new focus 
                                                                  //    item will already be selected... 
                  nS &= ~TVIS_SELECTED;                           //    ... so we're done updating the select state
                  nSM &= ~TVIS_SELECTED;
               }                                                  //     otherwise, handle in STEP 2. 
            }
                                                                  // ... and we wanted to deselect it, handle in 
                                                                  // STEP 2. 
         }
         else                                                     // if we did NOT want to alter the select state...
         {
            if ( !bWasSel )                                       //    if item had not been selected...
            { 
               nS &= ~TVIS_SELECTED;                              //    ... adjust state & statemask to deselect it 
               nSM |= TVIS_SELECTED;                              //    in STEP 2. 
            }
                                                                  //    else item is still selected, so nothing to do. 
         } 
      }                                                           // END -- set focus to this item. 

      else                                                        // BEGIN -- clear focus from this item. 
      {
         if ( bHadFocus )                                         // if item did not have focus, there's nothing to do. 
         {                                                        // otherwise: 
            cTree.SelectItem( NULL );                             // this removes the focus (only one item can have 
                                                                  // it); however, if item was also selected, this 
                                                                  // will also deselect item, so we must correct:

            if ( !(nStateMask & TVIS_SELECTED) )                  // if we did NOT want to alter the select state...
            {
               if ( bWasSel )                                     // ... and it had been selected, we need to restore 
               {                                                  //    selection.  we do it now rather than in STEP 2  
                                                                  //    to avoid double-notify.
                  ASSERT( !(nSM & TVIS_SELECTED) );
                  cTree.SetItemState( hItem, TVIS_SELECTED, 
                                             TVIS_SELECTED );
               }
            }
            else if ( nState & TVIS_SELECTED )                    // else if we wanted to select item (but clear 
            {                                                     // the focus)...
               if ( bWasSel )                                     //    if it had been selected, restore selection. 
                  cTree.SetItemState(hItem, TVIS_SELECTED, 
                                            TVIS_SELECTED );

               nS &= ~TVIS_SELECTED;                              //    either way, we're done updating select state. 
               nSM &= ~TVIS_SELECTED;
            }
         }
      }                                                           // END -- clear focus from this item. 
   }                                                              // END STEP 1. 


   if ( !nSM )                                                    // if there are no other states to alter, we're done! 
      return( TRUE ); 

   if ( nSM & TVIS_SELECTED )                                     // STEP 2:  Alter select state if requested.  In this 
   {                                                              // case we need to emulate TVN_SELCHANGING and stop 
                                                                  // if parent denies change... 
      NMTREEVIEW nmtv;
      nmtv.hdr.hwndFrom = m_hWnd;
      nmtv.hdr.idFrom = ::GetDlgCtrlID( m_hWnd );
      nmtv.hdr.code = TVN_SELCHANGING;
      nmtv.itemOld.mask = nmtv.itemNew.mask = 0;
      nmtv.itemOld.hItem = nmtv.itemNew.hItem = NULL;
      TVITEM& item = (nS & TVIS_SELECTED) ? nmtv.itemNew : nmtv.itemOld;
      item.mask = TVIF_HANDLE|TVIF_PARAM;
      item.hItem = hItem;
      item.lParam = cTree.GetItemData( hItem );

      if ( _SendNotify( &nmtv.hdr ) )
         return( FALSE );                                         // parent stopped selection change 

      VERIFY( 
         cTree.SetItemState( hItem, nS, nSM & TVIS_SELECTED ) );  // update selection state only

      nmtv.hdr.code = TVN_SELCHANGED;                             // send emulated TVN_SELCHANGED notification 
      _SendNotify( &nmtv.hdr );                                   // (return value is ignored) 

      nS &= ~TVIS_SELECTED;                                       // done updating selection state 
      nSM &= ~TVIS_SELECTED;
   }

   if ( !nSM ) return( TRUE );                                    // no other states to update 

   return( cTree.SetItemState( hItem, nS, nSM ) );                // native tree control handles all other states 
}


//=== GetItemState ==================================================================================================== 
//
//    Get the current state of specified tree item.
//
//    !!! IMPORTANT !!!  Use instead of CTreeCtrl method GetItemState().  This handles the normal functionality of 
//    the native method in single-select mode, as well as implementing the TVIS_FOCUSED state and multi-selection. 
//
//    ARGS:       hti   -- [in] handle of tree item. 
//                nSM   -- [in] which states are to be retrieved. 
//
//    RETURNS:    current states indicated in state mask.   
//
UINT CMultiDragTreeView::GetItemState
   (
   HTREEITEM hti, 
   UINT nSM
   ) const
{
   UINT n = GetTreeCtrl().GetItemState( hti, nSM & ~TVIS_FOCUSED );     // omit emulated TVIS_FOCUSED flag 

   if ( nSM & TVIS_FOCUSED )                                            // check focus state if requested 
      if ( GetFocusedItem() == hti ) n |= TVIS_FOCUSED;

   return( n );
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump contents of the tree view in an easy-to-read form to the supplied dump context.  If depth context <= 0, we 
//    only dump a summary.  If depth > 0, we also dump the text label and the app-defined data ('lParam') associated 
//    with each item currently in the tree. 
//
//    NOTE:  For the comprehensive dump, the tree view is traversed in "standard order", starting w/ the first child of 
//    TVI_ROOT.  "Standard order" is described recursively as:  visit node, visit each of node's children in standard 
//    order, then move on to node's next sibling and continue until there is no next sib. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::Dump( CDumpContext& dc ) const
{
   CTreeView::Dump( dc );                                // base class stuff first...

   CTreeCtrl& cTree = GetTreeCtrl();                     // get reference to tree control

   dc << "Contains " << cTree.GetCount() << " items.\n"; // brief summary...
   if ( m_bMulti )
   {
      dc << "Multi-select feature enabled; ";
      dc << GetSelectedCount() << " items selected.\n"; 
   }
   else
      dc << "Multi-select feature disabled.\n";

   if ( dc.GetDepth() <= 0 ) return;                     // shallow dump only -- we're done! 


   int nest = 0;
   HTREEITEM hti = cTree.GetChildItem( TVI_ROOT );       // display info on every item in tree, traversed in standard 
   while( hti != NULL )                                  // order...
   {
      dc << nest << " : " << cTree.GetItemText( hti );   //    dump current item as "nest : label [lParam]" 
      dc << " [" << cTree.GetItemData( hti ) << "]\n"; 

      HTREEITEM htiLast = hti;                           //    go to next item in traversal:
      hti = cTree.GetChildItem( hti );
      if ( hti != NULL )                                 //       current item has children; visit them next
         ++nest;
      else                                               //       no children; go to current item's next sib or 
      {                                                  //       unwind one or more nest levels to an item that 
         hti = cTree.GetNextSiblingItem( htiLast );      //       does have a next sib... 
         while( hti == NULL )
         {
            --nest;
            if ( nest < 0 ) break;                       //       we've finished the traversal!
            htiLast = cTree.GetParentItem( htiLast );
            if ( htiLast == NULL ) htiLast = TVI_ROOT;
            hti = cTree.GetNextSiblingItem( htiLast );
         }
      }
   }

}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate the tree view's state.  Validating the base class is good enough. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::AssertValid() const
{
   CTreeView::AssertValid();
}

#endif //_DEBUG



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== IsValidDropTarget =============================================================================================== 
//
//    Returns TRUE only if the specified item is a valid drop target for the items currently being dragged.  Call this 
//    function only during drag-n-drop animation. 
//
//    To be valid, the drop target must exist, must not be a drag item nor the immediate parent of a drag item.  Also, 
//    the drop target cannot be a descendant of any drag item.  Finally, the virtual helper fcn CanDrop() indicates 
//    whether or not the particular item is enabled as a drop target.  By definition, all selected items are 
//    considered drag items during the course of a drag-n-drop operation.
//
//    ARGS:       hItem -- [in] handle of the tree item to be tested (could be NULL)
//
//    RETURNS:    TRUE if specified item is a valid drop target; FALSE otherwise.   
//
BOOL CMultiDragTreeView::IsValidDropTarget
   (
   HTREEITEM hItem
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                                       // get reference to embedded tree control

   int i, j;
   if ( (hItem != NULL) &&  cTree.GetItemImage( hItem, i, j ) &&           // make sure item exists in the control 
        CanDrop( hItem ) &&                                                // item is drop-enabled 
        !(GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED)            // item is NOT a drag item 
      )
   {
      HTREEITEM htiParent = hItem;                                         // item cannot be descendant of a drag item 
      while ( (htiParent = cTree.GetParentItem( htiParent )) != NULL )
      {
         if ( GetItemState( htiParent, TVIS_SELECTED ) 
                  & TVIS_SELECTED ) 
            return( FALSE );
      }

      CHTIList htiDragList;                                                // item cannot be immediate parent of any 
      GetSelectedList( htiDragList );                                      // drag item 
      POSITION pos = htiDragList.GetHeadPosition();
      while( pos != NULL )
      {
         HTREEITEM hti = htiDragList.GetNext( pos );
         if ( hItem == cTree.GetParentItem( hti ) )
            return( FALSE );
      }

      return( TRUE );                                                      // passed all tests -- it's a valid drop tgt 
   }
   return( FALSE );
}


//=== StartDragging =================================================================================================== 
//
//    Initiates a left-button or right-button drag-n-drop operation.  
//
//    ARGS:       hti   -- [in] the item under the mouse when the drag was initiated 
//                pt    -- [in] the current mouse pos in client coords 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::StartDragging
   (
   HTREEITEM hti,
   CPoint pt
   ) 
{
   CPoint offsetPt = pt;
   m_pilDrag = CreateDragImageEx( hti, offsetPt );    // create drag image & get offset pt; if this fails, abort 
   if ( m_pilDrag == NULL ) return;

   m_hItemDrop = NULL;

   m_pilDrag->BeginDrag( 0, offsetPt );               // initialize dragging 
   ClientToScreen( &pt );                             // will drag in desktop context; so convert to screen coords
   m_pilDrag->DragEnter( NULL, pt );                  // begin drawing the drag image 
   SetCapture();                                      // capture mouse in case it moves outside view 
   m_nTimerID = SetTimer( 1, 300, NULL );             // countdown timer for auto-scroll and auto-expand 
}


//=== CreateDragImageEx =============================================================================================== 
//
//    Create a suitable drag bitmap based upon the currently selected item(s).
//
//    This extension corrects the following deficiencies in CTreeCtrl::CreateDragImage():
//       (1) Does not work for tree items that do not have item bitmap icons (the TVSIL_NORMAL image list).
//       (2) Does not support multiple selected items.
//
//    We choose an approach similar in some respects to that of the list view in Explorer's left pane...
//       ...When a single item is selected and it has an associated bitmap icon, we use CTreeCtrl::CreateDragImage().
//       ...For a single item *without* a bitmap icon, we represent it using a dotted rectangular outline about the 
//          size of the item's label.
//       ...For a multiple-item selection, we use a similar outline representation for each selected item.  Items that 
//          do not have a bitmap icon are shown as above.  Items with a bitmap icon appear as a dotted rectangle for 
//          the icon and a horizontal line for the item label.
//
//    In the latter two cases, we prepare a bitmap representation of the drag image in a memory device context.  This 
//    bitmap can get quite big if the virtual client area is very large and there are selected items that are scrolled 
//    outside the viewable client rect.  We have chosen, therefore, only to represent VISIBLE selected items in the 
//    drag bitmap.
//
//    If the drag bitmap is successfully created, a CImageList object is created and the drag bitmap is added to it 
//    using a transparency mask color so that only the rectangular outlines we drew appear.  BE SURE TO DELETE THE 
//    DRAG IMAGE LIST WHEN THE DRAG IS FINISHED!  We also offset the specified drag point so that it represents the 
//    (x,y)-offset of the drag point from the ULC of the drag bitmap.  As a result, the drag image initially overlays 
//    (well, approximately) the selected items. 
//
//    All computations here are in client coordinates in units of pixels. 
//
//    ARGS:       htiDrag  -- [in] the item under the mouse when the drag was initiated by user. 
//                ptDrag   -- [in] the current mouse pos in client coords (pix). 
//                            [out] the offset from the ULC of the drag bitmap to the current mouse pos (pix); not 
//                            modified if function is unsuccessful. 
//
//    RETURNS:    pointer to an image list object that holds the drag image.  the image list must be destroyed upon 
//                completion of the drag-n-drop operation.  NULL if function is unsuccessful
//
CImageList* CMultiDragTreeView::CreateDragImageEx
   ( 
   HTREEITEM htiDrag, 
   CPoint& ptDrag 
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                           // get reference to embedded tree control

   CImageList* pIL = cTree.GetImageList( TVSIL_NORMAL );       // # of bitmap icons associated with tree 
   int nIcons = (pIL != NULL) ? pIL->GetImageCount(): 0;

   UINT nSel = GetSelectedCount();                             // # items selected
   int iNorm;                                                  // image index for normal bitmap icon
   int iSel;                                                   // image index for selected bitmap icon 
   if ( nSel <= 0 )                                            // abort if no items are selected 
      return( NULL ); 
   if ( nSel == 1 )                                            // single-selection... 
   {
      iSel = -1;                                               // does the item have a selected bitmap icon?
      cTree.GetItemImage( htiDrag, iNorm, iSel );
      if ( (iSel >= 0) && (iSel < nIcons) )                    // if so, use default version...
      {
         ptDrag.x = -8;                                        // place ULC of bitmap below and right of drag pt 
         ptDrag.y = -8;
         return( cTree.CreateDragImage( htiDrag ) );
      }
   }


   CRect rect;
   CRect rect1;
   CRect rectClient;                                           // the client area 
   GetClientRect( &rectClient );

   CRect rectStart;                                            // bounding rect of the item under cursor...
   BOOL bOK = cTree.GetItemRect( htiDrag, &rectStart, TRUE );  // ...item must exist and be part of an expanded subtree 
   if ( bOK )
      bOK = rect.IntersectRect( &rectClient, &rectStart );     // ...and item must be visible within client area
   if ( !bOK )return( NULL );                                  // -- else abort!

   CRect rectSelect(0,0,0,0);                                  // rect bounding all selected items which are at least 
                                                               // partly visible (within client rect)
   
   CHTIList htiSelList;                                        // obtain list of selected items 
   GetSelectedList( htiSelList );

   UINT nIconIndent = 0;                                       // dist (pix) between left edges of item bitmap & label; 
   if ( pIL != NULL ) nIconIndent = cTree.GetIndent();         //    remains 0 if item bitmaps are not used 

   bOK = FALSE;                                                // set TRUE if we successfully create drag img list

                                                               // these declarations are here to permit use of 'goto'...
   CClientDC dcClient( this );                                 // client device context for this view 
   CDC dcMem;                                                  // a memory device context 
   CBitmap bmDrag;                                             // the drag bitmap we will draw in the memory DC 
   CPen pen;                                                   // drawing tools we will use
   CBrush brush;                                               //
   CBitmap* pOldBitmap = NULL;                                 
   CPen* pOldPen = NULL;
   CBrush* pOldBrush = NULL;
   CImageList* pDragIL = NULL;                                 // the drag image list we're trying to create 
   UINT i = 0;
   UINT nVis = 0;                                              // we'll only include visible items in the drag bitmap
   POSITION pos = NULL;                                        // for iterating the list of selected items 

   CRect* pItemRect = new CRect[nSel];                         // array of bounding rects of selected item labels 
   BOOL* pbHasIcon = new BOOL[nSel];                           // does selected item have an associated bitmap icon? 
   if ( (pItemRect == NULL) || (pbHasIcon == NULL) )           // if allocations fail, abort 
      goto CLEANUP0;

   pos = htiSelList.GetHeadPosition();                         // determine rect bounding the entire visible multi-sel 
   while( pos != NULL )                                        // in client coords:
   {
      HTREEITEM hti = htiSelList.GetNext( pos ); 
      if ( cTree.GetItemRect( hti, &rect, TRUE ) )             //    get item label rect
      {
         iSel = -1;                                            //    if item has a selected bitmap icon...
         BOOL bIcon = FALSE;
         cTree.GetItemImage( htiDrag, iNorm, iSel );
         if ( (iSel >= 0) && (iSel < nIcons) ) 
         {
            rect.left -= nIconIndent;                          //    ...then adjust rect to include the icon 
            bIcon = TRUE;                                      //    ...and remember that fact...
         }

         if ( rect1.IntersectRect( &rectClient, &rect ) )      //    if item rect is at least partially visible, save 
         {                                                     //    it and adjust selection rect to include it. 
            rectSelect.UnionRect( rectSelect, rect ); 
            pItemRect[nVis] = rect;
            pbHasIcon[nVis] = bIcon;
            ++nVis;
         }
      }
   }
   rectSelect.left -= 2;                                       // a little padding all around boundary... 
   rectSelect.top -= 2;
   rectSelect.right += 2;
   rectSelect.bottom += 2;


   if ( !dcMem.CreateCompatibleDC( &dcClient ) )               // create memory DC compatible with our view DC
      goto CLEANUP0;

   if ( !bmDrag.CreateCompatibleBitmap( &dcClient,             // create bitmap compatible with our view DC... 
           rectSelect.Width(), rectSelect.Height() ) )
      goto CLEANUP0;

   if ( !pen.CreatePen( PS_DOT, 0, RGB(0,0,0) ) )              // create a 1-pixel-wide black dotted pen 
      goto CLEANUP1;

   if ( !brush.CreateSolidBrush( RGB(0,255,0) ) )              // create green brush for filling rectangles; green is 
      goto CLEANUP1;                                           // our chosen "mask" color for transparency!

   pOldBitmap = dcMem.SelectObject( &bmDrag );                 // select bitmap, pen & brush into memory DC 
   pOldPen = dcMem.SelectObject( &pen );
   pOldBrush = dcMem.SelectObject( &brush );

   dcMem.FillSolidRect( 0, 0,                                  // fill bitmap with green as "mask" color for 
                        rectSelect.Width(),                    // transparency 
                        rectSelect.Height(), 
                        RGB(0,255,0) 
                      );

   for( i = 0; i < nVis; i++ )                                 // now draw a outline representation of all the rects we 
   {                                                           // created earlier:
      rect = pItemRect[i];
      rect.left -= rectSelect.left;                            //    convert to coords in bitmap
      rect.top -= rectSelect.top;
      rect.right -= rectSelect.left; 
      rect.bottom -= rectSelect.top;

      if ( pbHasIcon[i] )                                      //    if item has a selected bitmap icon, draw a rect line
      {                                                        //    about size of icon and a line for for item label 
                                                               //    REM: rect already includes space for bitmap!
         int j = rect.top + (rect.Height()/2);
         dcMem.MoveTo( rect.left + nIconIndent, j );
         dcMem.LineTo( rect.right, j );
         rect.right = rect.left + nIconIndent - 3;
         dcMem.Rectangle( &rect );
      }
      else                                                     //    if item does NOT have selected bitmap icon, reduce 
      {                                                        //    vertical dimension slightly and draw outline of 
         ++rect.top;                                           //    the label rect 
         --rect.bottom;
         dcMem.Rectangle( &rect ); 
      }
   }


   dcMem.SelectObject( pOldBitmap );                           // restore old bitmap, pen & brush to memory DC
   dcMem.SelectObject( pOldPen );
   dcMem.SelectObject( pOldBrush );

   pDragIL = NULL;                                             // alloc & create img list to hold composite drag bm...
   pDragIL = new CImageList; 
   if ( pDragIL == NULL ) 
      goto CLEANUP1;
   if ( !pDragIL->Create( rectSelect.Width(), 
                          rectSelect.Height(), 
                          ILC_COLOR | ILC_MASK, 0, 1 ) )       // ...using a mask bitmap for transparency 
      goto CLEANUP1;

   bOK = BOOL(pDragIL->Add( &bmDrag, RGB(0, 255, 0) ) == 0);   // add bitmap to the image list, using green for mask 


CLEANUP1:                                                      // release allocated resources...
   bmDrag.DeleteObject();
   pen.DeleteObject(); 
   brush.DeleteObject();
CLEANUP0:
   if ( pItemRect != NULL )
      delete [] pItemRect;
   if ( pbHasIcon != NULL )
      delete [] pbHasIcon;

   if ( !bOK )                                                 // failed to install bitmap in image list, so delete it 
   {
      if ( pDragIL != NULL )
      {
         delete pDragIL;
         pDragIL = NULL; 
      }
   }
   else
   {
      ptDrag.x -= rectSelect.left;                             // convert drag point in client coords to coord system 
      ptDrag.y -= rectSelect.top;                              // of the drag bitmap
   }

   return( pDragIL );
}


//=== EndDragging ===================================================================================================== 
//
//    Terminates a left-button or right-button drag-n-drop operation that ended at the specified point.  The operation 
//    is cancelled if the point is outside the view's client area. 
//
//    We release the resources that were allocated to the animation (timer, drag image list) and, if the operation was 
//    not cancelled, call helper functions to handle the details of executing the drag-n-drop:
//       ==> Left-button drag:  RealizeDrag() moves/copies the drag item to the drop target. 
//       ==> Right-button drag:  RunRightDragMenu() displays a popup context menu to prompt user to complete the 
//                               drag's effect or cancel. 
//
//    ARGS:       point    -- [in] mouse cursor position in client window coords 
//                bRtDrag  -- [in] TRUE if right-button drag; otherwise it's a left-button drag 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::EndDragging
   (
   CPoint point,
   BOOL bRtDrag
   )
{
   if ( !IsDragging() ) return; 

   CTreeCtrl& cTree = GetTreeCtrl();                  // get reference to tree control

   KillTimer( m_nTimerID );                           // release the timer
   m_nTimerID = 0;
   m_nHover = 0;

   CImageList::DragLeave( this );                     // stop the drag 
   CImageList::EndDrag();
   ReleaseCapture();                                  // release the mouse capture 

   delete m_pilDrag;                                  // release the drag image list 
   m_pilDrag = NULL;

   cTree.SelectDropTarget( NULL );                    // remove drop target highlighting 

   CRect clientRect;                                  // get client area 
   GetClientRect( &clientRect );

   if ( clientRect.PtInRect( point ) &&               // if drop pt is inside client area and drop target is valid,  
        IsValidDropTarget( m_hItemDrop )              //    then realize the effect of the drag 
      )
   {
      if ( bRtDrag )                                  // popup context menu in response to right drag 
      {
         ClientToScreen( &point );
         RunRightDragMenu( point ); 
      }
      else
      {
         BOOL bCopy = (::GetKeyState( VK_CONTROL )    // we copy item if CTRL key is held down when left drag ends, 
                     & 0x8000);                       // else we move the item. 

         CHTIList htiDragList;                        // the list of valid drag items
         PreRealizeDrag( htiDragList );               // constructs drag list and clears selection 
         RealizeDrag( htiDragList, bCopy );           // realize effects of the drag  
      }
   }
   else
   {
      m_hItemDrop = NULL;                             // reset drag-n-drop status vars 
      m_bRtDrag = FALSE;
   }
}


//=== CopyItem ======================================================================================================== 
//
//    Copy a single childless item. 
//
//    ARGS:       hti      -- [in] the item to be copied
//                htiDst   -- [in] the item which will parent the copied item
//                htiAfter -- [in] the sibling after which the copied item is inserted 
//
//    RETURNS:    handle of copied item, if successful; NULL otherwise. 
//
HTREEITEM CMultiDragTreeView::CopyItem
   (
   HTREEITEM hti, 
   HTREEITEM htiDst, 
   HTREEITEM htiAfter   // = TVI_LAST
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                                    // get reference to tree control

   TV_INSERTSTRUCT tvstruct;                                            // get attributes of the item to be copied 
   tvstruct.item.hItem = hti;
   tvstruct.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | 
                        TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
   cTree.GetItem( &(tvstruct.item) );
   CString sText = cTree.GetItemText( hti );
   tvstruct.item.cchTextMax = sText.GetLength();
   tvstruct.item.pszText = sText.GetBuffer( tvstruct.item.cchTextMax );

   tvstruct.hParent = htiDst;                                           // insert new item at specified location 
   tvstruct.hInsertAfter = htiAfter;
   tvstruct.item.mask |= TVIF_TEXT;
   HTREEITEM hNewItem = cTree.InsertItem( &tvstruct );

   sText.ReleaseBuffer();                                               // release CString buffer 

   return( hNewItem );
}


//=== CopyBranch ====================================================================================================== 
//
//    Copy an entire branch of tree (an item with all its descendants). 
//
//    NOTES: 
//    (1) This method does NOT verify that operation is feasible.  Caller must ensure that we are not copying a branch 
//    onto itself, for example!!!
//    (2) The original version of this method (R. Hazelwood's CMultiTree class) was recursive.  I decided to avoid 
//    recursion in case this class was ever used with tree views having many hierarchical levels.  This non-recursive 
//    version traverses the copied parent nodes's subtree in the same standard order.  Let A, B,... be the immediate 
//    children of the copied node, in sibling order.  We copy node A, then all of node A's children in standard order, 
//    then node B, etc...
//
//    ARGS:       hBr      -- [in] the branched item to be copied 
//                hDst     -- [in] the item which will parent the copied branch 
//                hAfter   -- [in] the sibling after which the copied branch is inserted 
//
//    RETURNS:    handle of copied item, if successful; NULL otherwise. 
//
HTREEITEM CMultiDragTreeView::CopyBranch
   (
   HTREEITEM hBr, 
   HTREEITEM hDst, 
   HTREEITEM hAfter  // = TVI_LAST 
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                        // get reference to tree control

   HTREEITEM hNewBranch = CopyItem( hBr, hDst, hAfter );    // copy base node of branch to the specified parent 
   if ( hNewBranch == NULL ) return( NULL );                // abort immediately if copy fails

   HTREEITEM hChild = cTree.GetChildItem( hBr );            // copy all descendants of base node....
   HTREEITEM hNewDst = hNewBranch;
   int nest = 1;
   BOOL bAbort = FALSE;
   while ( nest > 0 )
   {
      HTREEITEM hNewChild =                                 // ...copy the child 
         CopyItem( hChild, hNewDst, TVI_LAST );
      if ( hNewChild == NULL )                              // ...abort if copy fails! 
      {
         bAbort = TRUE;
         break;
      }

      HTREEITEM hNext = cTree.GetChildItem( hChild );
      if ( hNext != NULL )                                  // ...if child has children, copy them next...
      {
         ++nest;
         hChild = hNext;
         hNewDst = hNewChild;
      }
      else                                                  // ...child has no children, so move on to its next sib, 
      {                                                     //    unwinding thru src & dst trees as necessary...
         hNext = cTree.GetNextSiblingItem( hChild ); 
         while( (hNext == NULL) && (nest > 0) )
         {
            --nest;
            hChild = cTree.GetParentItem( hChild );
            hNewDst = cTree.GetParentItem( hNewDst );
            hNext = cTree.GetNextSiblingItem( hChild );
         }
         hChild = hNext;
         
      }
   }

   if ( bAbort )                                            // if unable to copy entire branch, remove partial branch 
   {
      DeleteItemEx( hNewBranch );
      return( NULL );
   }
   else
      return( hNewBranch );
}


//=== OnButtonDown ==================================================================================================== 
//
//    Here is where we modify the response of the embedded tree control to a left- or right-mousedown.  If multiple 
//    selection is not enabled, the default CTreeCtrl handlers are called.  Otherwise the response depends on where the 
//    mouse down occurred:
//       (1) On item button, or the item state icon (if TVS_CHECKBOXES style) -- use default handlers. 
//       (2) On item label -- response depends on the item's state, the state of the SHIFT and CTRL keys, and which 
//           mouse button was used.  see DoSelectAndTrack() for details. 
//       (3) Otherwise -- the button was depressed on white space, so we initiate a rubber-banding operation to 
//           select multiple items in tree.  see DoBanding() for details. 
//
//    ARGS:       bLeft    -- [in] TRUE if left mouse-down, otherwise it's a right-mousedown 
//                nFlags   -- [in] flags indicate whether various virtual keys are down
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::OnButtonDown
   (
   BOOL bLeft, 
   UINT nFlags, 
   CPoint point
   )
{
   DWORD downTime = ::GetTickCount();                 // approx time that user depressed mouse button; required when 
                                                      // when user mousedowns on an item label

   UINT nHF;                                          // hit flags
   HTREEITEM hItem = NULL;                            // hit item 
   CTreeCtrl& cTree = GetTreeCtrl();                  // reference to embedded tree control

   BOOL bBase = !m_bMulti;                            // even w/ multi-sel enabled, the base can handle certain hits:
   if ( !bBase ) 
   {
      hItem = cTree.HitTest( point, &nHF );
      if ( hItem ) 
      {
         bBase = (nHF & TVHT_ONITEMBUTTON);              // base class always handles expand/collapse of items 
         if ( !bBase && bLeft &&                         // base class can handle check box change -- won't affect 
              (cTree.GetStyle() & TVS_CHECKBOXES)        //    multi-selection status 
            ) 
            bBase = (nHF & TVHT_ONITEMSTATEICON);
      }
   }

   if ( bBase )                                       // if base class can handle the mouse down, pass it on... 
   {
      if ( bLeft )
         CTreeView::OnLButtonDown( nFlags, point );
      else
         CTreeView::OnRButtonDown( nFlags, point );
      return;
   }

   if ( !hItem ||                                     // if user clicked in white-space, enter message-loop for 
        (nHF & (TVHT_ONITEMRIGHT |                    //    rubber-band selection of items 
                TVHT_NOWHERE | 
                TVHT_ONITEMINDENT)
        )
      ) 
   {
      DoBanding( bLeft, nFlags, point );
      return;
   }

   // otherwise, user must have clicked on item label/bitmap; in this case, we may need to add the item to the current 
   // selection set, change the focused item, etc.  we also have to trap WM_MOUSEMOVE and send TVN_BEGIN(R)DRAG 
   // notifications. 
   //
   ASSERT( nHF & (TVHT_ONITEM | TVHT_ONITEMSTATEICON) ); 
   DoSelectAndTrack( hItem, bLeft, nFlags, point, downTime ); 
}


//=== DoSelectAndTrack ================================================================================================ 
//
//    Called when the user mouse-downs on an item label, this function adjusts the item's state and the multi-selection 
//    state depending on which mouse button was depressed, the state of the SHIFT and CTRL keys, and the item's 
//    previous state.  It then must take over the message loop and track the mouse in order to detect the start of a 
//    drag operation or a simple left or right-click.  We send the NM_(R)CLICK or TVN_BEGIN(R)DRAG notifications, or 
//    initiate an in-place edit, depending on what the user does. 
//
//    More on initiating label-edit:  The native control behavior is different.  The user can merely single-click 
//    (i.e., depress *and* release quickly) the label and wait.  Once the double-click time has expired, and in-place 
//    edit is initiated.  Because we have overridden CTreeView/CTreeCtrl's default WM_LBUTTONDOWN handler, we have to 
//    replicate this behavior.  The default handler set a timer that expired after the double-click time and then 
//    initiated an in-place edit if the subsequent WM_TIMER msg was received without an intervening WM_LBUTTONDOWN.  
//    We don't do this in CMultiDragTreeView because of possible interference with left drag.  Instead, the user must 
//    HOLD DOWN the left button STATIONARY for at least the double-click time.  Then, upon releasing the button, this 
//    routine will initiate an in-place edit operation instead of sending a click or drag notification.
//
//    ARGS:       hItem    -- [in] the tree item which was selected by the user mousedown
//                bLeft    -- [in] TRUE if left mousedown, otherwise right 
//                nFlags   -- [in] the flags delivered with the WM_L(R)BUTTONDOWN msg 
//                point    -- [in] mouse cursor position in client window coords 
//                dwTime   -- [in] tick count (Windows time in msec) when user mouse-downed on label 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::DoSelectAndTrack
   (
   HTREEITEM hItem, 
   BOOL bLeft, 
   UINT nFlags,
   CPoint point,
   DWORD dwTime
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // reference to embedded tree control
   BOOL bShiftOn = BOOL(nFlags & MK_SHIFT);                       // state of SHIFT and CTRL keys
   BOOL bCtrlOn = BOOL(nFlags & MK_CONTROL);                      //

   // PHASE 1:  Update the multi-selection state and the state 
   // mousedown item appropriately
   //
   if ( bLeft )                                                   // handle left mousedown:
   {
      if ( bShiftOn )                                             // if SHIFT key down, we must select all items 
      {                                                           // between the SHIFT-select base item and this item:
         if ( !m_hSelect )                                        //    if there's no SHIFT-sel base item, use the 
            m_hSelect = GetFocusedItem();                         //    currently focused item
         SelectRange( m_hSelect, hItem, !bCtrlOn);                //    if CTRL on, any other selected items remain 
                                                                  //    selected; else, they're deselected. 
         SetItemState( hItem, TVIS_FOCUSED, TVIS_FOCUSED );       //    also, the item clicked gets the focus. 
      }
      else                                                        // if SHIFT key was NOT down...
      { 
         m_hSelect = NULL;                                        // clear the SHIFT-select base item 
         if ( !bCtrlOn )                                          // if CTRL key down, selection is delayed until mouse 
         {                                                        // up, otherwise:
            if ( !IsSelected( hItem ) )                           //    if the item wasn't selected already, clear the 
               SelectAllIgnore( FALSE, hItem );                   //    current multi-selection (ignoring this item)
            SetItemState( hItem, TVIS_SELECTED|TVIS_FOCUSED,      //    the clicked item is selected and focused 
                                 TVIS_SELECTED|TVIS_FOCUSED );
         }
      }
   }
   else                                                           // handle right mousedown:
   { 
      if ( bShiftOn || bCtrlOn )                                  // if SHIFT or CTRL key is down, do nothing except 
      {
         if ( !bShiftOn ) m_hSelect = hItem;                      //    update the SHIFT-select base item to this item. 
      } 
      else                                                        // otherwise: 
      {
         if ( !IsSelected( hItem ) )                              //    if the item wasn't selected already, clear the 
            SelectAllIgnore( FALSE, hItem );                      //    current multi-selection (ignoring this item) 
         SetItemState( hItem, TVIS_SELECTED|TVIS_FOCUSED,         //    the clicked item is selected and focused 
                              TVIS_SELECTED|TVIS_FOCUSED );
      }
   }
   //
   // END PHASE 1


   // PHASE 2:  Track the mouse to detect a drag or click.
   //
   ::SetCapture( m_hWnd );                                        // capture the mouse 
   ASSERT( ::GetCapture() == m_hWnd );                            // 

   CSize sizeDrag( ::GetSystemMetrics( SM_CXDRAG ),               // how far mouse must move to be considered a drag 
                   ::GetSystemMetrics( SM_CYDRAG ) );

   MSG msg;
   UINT nAction = 0;
   CPoint pt;
   DWORD mouseUpTime = 0;                                         // when left mouse button released (Windows time)

   while ( !nAction && ::GetMessage( &msg, NULL, 0, 0 ) )         // message loop: 
   {
      if ( ::GetCapture() != m_hWnd ) break;                      // if we lose the capture, then exit loop without
                                                                  //    sending a notification (nAction == 0)
      switch( msg.message ) 
      {
         case WM_MOUSEMOVE:                                       // if the mouse has moved far enough, initiate a 
                                                                  // drag operaton (nAction == 2);
            pt.x = GET_X_LPARAM(msg.lParam);                      // this mouse position is in *client* coords!
            pt.y = GET_Y_LPARAM(msg.lParam);
            if ( (abs(pt.x - point.x) > sizeDrag.cx) || 
                 (abs(pt.y - point.y) > sizeDrag.cy) 
               )
               nAction = 2; 
            break;

         case WM_LBUTTONDOWN:                                     // ignore any activity on the *other* mouse button!
         case WM_RBUTTONDOWN: 
         case WM_LBUTTONDBLCLK:
         case WM_RBUTTONDBLCLK: 
            break;

         case WM_LBUTTONUP:                                       // if we're waiting on the left mouse button and it 
            if ( bLeft )                                          // is released, get the tick count.  we issue 
            {                                                     // NM_CLICK, or initiate an in-place edit if the 
               nAction = 1;                                       // user has held the button long enough 
               mouseUpTime = ::GetTickCount(); 
            }                                                     // else, ignore it. 
            break; 

         case WM_RBUTTONUP:                                       // analogously for the right mouse button, but no  
            if ( !bLeft ) nAction = 1;                            // in-place edit. 
            break;

         default:                                                 // all other messages are dispatched as usual 
            ::DispatchMessage( &msg );
            break;
      }
   }

   ::ReleaseCapture();                                            // release the mouse 
   ASSERT( ::GetCapture() != m_hWnd );
   //
   // END PHASE 2.


   // PHASE 3:  if appropriate, send tree cntrl notification
   //
   if ( nAction ) 
   {
      NMTREEVIEW nmtv;                                            // construct tree notification info
      nmtv.hdr.hwndFrom = m_hWnd;
      nmtv.hdr.idFrom = ::GetDlgCtrlID( m_hWnd );
      nmtv.itemNew.mask = TVIF_HANDLE|TVIF_PARAM;
      nmtv.itemNew.hItem = hItem;
      nmtv.itemNew.lParam = cTree.GetItemData( hItem );
      DWORD dwStyle = cTree.GetStyle();

      if ( nAction == 1 )                                            // mouse click:  send NM_CLICK/NM_RCLICK, or 
      {                                                              //    an in-place edit...
         if ( (!bShiftOn) && bLeft )                                 // if !SHIFT && CTRL, then item selection is 
         {                                                           //    delayed until mouse click detected. 
            UINT nState = TVIS_SELECTED;
            if ( bCtrlOn )
               nState ^= (GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED);
            else
               SelectAllIgnore( FALSE, hItem );
            SetItemState( hItem, TVIS_FOCUSED|nState, TVIS_FOCUSED|TVIS_SELECTED );
         }
         if ( ::GetFocus() != m_hWnd )                               // the tree control should have the focus
            ::SetFocus(m_hWnd);

         BOOL bEdit = bLeft && (dwStyle & TVS_EDITLABELS);           // initiate in-place edit if it's appropriate...
         if ( bEdit ) bEdit = (!bShiftOn) && (!bCtrlOn);             // ... and there's no current multi-selection 
         if ( bEdit )                                                // ... and left mouse button was held down for at 
         {                                                           // least one dbl-click interval 
            if ( mouseUpTime > dwTime )                              // protect against Windows timer wrap-around 
               bEdit = ( UINT(mouseUpTime-dwTime) > 
                         ::GetDoubleClickTime() );
            else 
               bEdit = FALSE;
         }

         if ( bEdit )
            cTree.EditLabel( hItem );                                // either in-place edit, or...
         else
         {
            nmtv.hdr.code = bLeft ? NM_CLICK : NM_RCLICK;            // NM_(R)CLICK 
            _SendNotify(&nmtv.hdr); 
         }
      }
      else                                                           // mouse drag (nAction == 2):  send TVN_BEGIN(R)DRAG 
      {
         SetItemState( hItem, TVIS_FOCUSED|TVIS_SELECTED, 
                              TVIS_FOCUSED|TVIS_SELECTED );
         if ( !(dwStyle & TVS_DISABLEDRAGDROP) ) 
         {
            nmtv.hdr.code = bLeft ? TVN_BEGINDRAG : TVN_BEGINRDRAG;
            nmtv.ptDrag = point;
            _SendNotify(&nmtv.hdr);
         }
      }
   }

   return;
}


//=== DoBanding ======================================================================================================= 
//
//    Called when the user mouse-downs on white space in the tree control, this function animates the "rubber-band" 
//    multiple-selection of tree items.  It captures the mouse and takes over the message loop, responding to all 
//    relevant messages and forwarding all others.  The animation continues until the user releases the mouse button by 
//    which the banding selection was initiated, the mouse capture is lost, or the ESCAPE key is pressed.  As the 
//    banding rectangle moves, we toggle the selection state (some items may have already been selected prior to 
//    the start of banding!) of an item whenever it becomes part of or leaves the banding rectangle.  Pressing the 
//    ESC key de-selects all items in the control. 
//
//    If banding was initiated with the right-mouse button, we issue an NM_RCLICK notification to give derived views 
//    an opportunity to popup a context menu. 
//
//    NOTE:  We ASSUME all items in the tree have the same height.  May not work correctly if this is not the case.
//
//
//    ARGS:       bLeft    -- [in] TRUE if left mousedown, otherwise right 
//                nFlags   -- [in] the flags delivered with the WM_L(R)BUTTONDOWN msg 
//                point    -- [in] mouse cursor position in client window coords 
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::DoBanding
   (
   BOOL bLeft, 
   UINT nFlags, 
   CPoint point
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // reference to embedded tree control
   BOOL bShiftOn = BOOL(nFlags & MK_SHIFT);                       // state of SHIFT and CTRL keys at time of mousedown 
   BOOL bCtrlOn = BOOL(nFlags & MK_CONTROL);                      //
   
   if ( ::GetFocus() != m_hWnd )                                  // make sure we have the focus and mouse capture 
      ::SetFocus( m_hWnd );
   ::SetCapture( m_hWnd );

   CHTIList list;                                                 // the "locked" list of already selected items; this  
   if ( bShiftOn || bCtrlOn )                                     // list may have some items if SHIFT or CTRL key is  
      GetSelectedList( list );                                    // down; "locked" items get special treatment.  

   CClientDC dc( this );                                          // we'll be drawing in window, so get device context 
   CRect rectCli;                                                 // and the client rect 
   GetClientRect( &rectCli );

   CSize sizeDrag( ::GetSystemMetrics( SM_CXDRAG ),               // how far mouse must move to be considered a drag 
                   ::GetSystemMetrics( SM_CYDRAG ) );

   CRect rect(0, 0, 0, 0);                                        // get tree item height; if no items, height is zero. 
   UINT itemHt = 0; 
   HTREEITEM hItem = cTree.GetRootItem();
   if ( hItem ) 
   {
      cTree.GetItemRect( hItem, &rect, FALSE ); 
      itemHt = rect.Height();
   }

   CPoint ptScr( cTree.GetScrollPos( SB_HORZ ),                   // get the current scroll pos (vertical pos is in 
                 cTree.GetScrollPos( SB_VERT ) );                 //    (terms of #items "above" client area!)
   ptScr.y *= itemHt;                                             // convert Vpos to pixels; we ASSUME equal-ht items! 

   CPoint startPt = point;                                        // save banding start point in *virtual* client 
   startPt += ptScr;                                              //    coords (offset by scroll pos in pixels) 

   UINT_PTR nTimer = SetTimer( 2, 75, NULL );                     // start timer for auto-scrolling 

   MSG msg;
   CPoint pt;
   CSize sizeEdge(1, 1);                                          // thickness of banding rectangle 
   BOOL bDrag = FALSE;                                            // set TRUE as soon as banding has begun 
   BOOL bDone = FALSE;

   while ( (!bDone) && ::GetMessage( &msg, NULL, 0, 0 ) )         // message loop: 
   {
      if ( ::GetCapture() != m_hWnd ) break;                      // stop immediately if we lose the mouse capture 

      switch( msg.message )
      {
         case WM_TIMER:                                           // if it is our scroll timer and the mouse pos falls
            pt = msg.pt;                                          // outside ctrl, then fall through to WM_MOUSEMOVE;
            ScreenToClient( &pt );                                // otherwise, dispatch the message. 
            if ( nTimer != UINT_PTR(msg.wParam) || 
                 rectCli.PtInRect( pt ) ) 
            {
               ::DispatchMessage(&msg);
               break;
            }
            msg.lParam = MAKELPARAM(pt.x, pt.y);                  // for WM_MOUSEMOVE, mouse pos in client coords are 
                                                                  // contained in lParam member of MSG struct!
                                                                  // !!! FALL THROUGH !!! 

         case WM_MOUSEMOVE:
            pt.x = GET_X_LPARAM( msg.lParam );                    // client coords of current mouse pos (could be 
            pt.y = GET_Y_LPARAM( msg.lParam );                    //    outside window)

            if ( !bDrag )                                         // initiate drag if mouse has moved far enough: 
            {
               if ( (abs(pt.x - point.x) <= sizeDrag.cx) && 
                    (abs(pt.y - point.y) && sizeDrag.cy) 
                  ) 
                  break;                                          //    haven't moved far enough, so do nothing more

               bDrag = TRUE; 
               if ( !(bShiftOn && bCtrlOn) )                      //    if neither SHIFT nor CTRL is down, deselect 
                  SelectAll( FALSE );                             //       any and all tree items. 
               UpdateWindow();                                    //    force immediate repaint 
               rect.SetRect( point, point );                      //    initial banding rect starts at original 
               dc.DrawDragRect( rect, sizeEdge, NULL, sizeEdge ); //       mousedown point 
            }

            dc.DrawDragRect( rect, sizeEdge, NULL, sizeEdge );    // drag in progress; erase previous banding rect 

            if ( pt.y < rectCli.top )                             // scroll if new mouse pos outside client area
               ::SendMessage( m_hWnd, WM_VSCROLL, SB_LINEUP, 0 );
            else if ( pt.y >= rectCli.bottom )
               ::SendMessage( m_hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
            if ( pt.x < rectCli.left )
               ::SendMessage( m_hWnd, WM_HSCROLL, SB_LINELEFT, 0 );
            else if ( pt.x >= rectCli.right )
               ::SendMessage( m_hWnd, WM_HSCROLL, SB_LINERIGHT, 0 );

            ptScr = startPt;                                      // start point in *virtual* client coords 
            ptScr.x -= cTree.GetScrollPos( SB_HORZ );             // compensate for scroll pos, thus converting to 
            ptScr.y -= cTree.GetScrollPos( SB_VERT ) * itemHt;    //    client coords 

            rect.SetRect( ptScr, pt );                            // new banding rect extends from current mouse pos to 
            rect.NormalizeRect();                                 //    original pos 

            UpdateSelectionForRect( rect, nFlags, list );         // update items selected by new banding rect 
            dc.DrawDragRect( rect, sizeEdge, NULL, sizeEdge );    // draw new banding rect 
            break;

         case WM_LBUTTONDOWN:                                     // ignore any activity on the *other* mouse button!
         case WM_RBUTTONDOWN: 
         case WM_LBUTTONDBLCLK:
         case WM_RBUTTONDBLCLK: 
            break;

         case WM_LBUTTONUP:                                       // if we're waiting on the left mouse button and it 
            if ( bLeft ) bDone = TRUE;                            // is released, then we're done; otherwise ignore.  
            break; 

         case WM_RBUTTONUP:                                       // analogously for the right mouse button.  
            if ( !bLeft ) bDone = TRUE;
            break;

         case WM_KEYDOWN:                                         // if ESC key, deselect all items and stop banding 
                                                                  //    otherwise, dispatch as usual.
            if ( LOWORD(msg.wParam) == VK_ESCAPE ) 
            {
               SelectAll(FALSE);
               bDone = TRUE;
               break;
            }                                                     // !!! FALL THROUGH !!!

         default:                                                 // all other messages are dispatched as usual. 
            ::DispatchMessage(&msg);
            break;
      }
      // END switch( )...

   }
   //
   // END message loop....


   KillTimer( nTimer );                                           // release the auto-scroll timer and the mouse 
   ::ReleaseCapture();                                            // 

   if ( bDrag )                                                   // if we did any banding, erase the last banding rect 
      dc.DrawDragRect( rect, sizeEdge, NULL, sizeEdge );
   else if ( !(bShiftOn && bCtrlOn) )                             // otherwise, deselect all items if neither SHIFT 
      SelectAll( FALSE );                                         //    nor CTRL key was depressed 


   if ( !bLeft && (msg.message == WM_RBUTTONUP) )                 // right-button banding ended -- issue NM_RCLICK...
   {
      NMHDR nmhdr;                                                //    construct NM_RCLICK notification header
      nmhdr.hwndFrom = m_hWnd;
      nmhdr.idFrom = ::GetDlgCtrlID( m_hWnd );
      nmhdr.code = NM_RCLICK;
      if ( ::GetFocus() != m_hWnd )                               //    the tree control should have the focus
         ::SetFocus( m_hWnd );
      _SendNotify( &nmhdr );                                      //    return value is ignored
   }
   else
      ::DispatchMessage( &msg );                                  // ...else dispatch last msg which ended banding. 
}


//=== UpdateSelectionForRect ========================================================================================== 
//
//    Update the selection state of items which newly intersect or which no longer intersect the specified "banding 
//    rectangle".  The "intersect" test uses only the item label rect rather than the entire line occupied by the item. 
//
//    The change in selection state will depend also on the state of the SHIFT and CTRL keys at the time banding was 
//    initiated, and whether or not the item is found in the "locked list". 
//
//    NOTE:  We traverse the entire list of *visible* items; invisible items (including children of collapsed parents) 
//    are unaffected.
//
//    ARGS:       pRect    -- [in] the current banding rectangle 
//                nFlags   -- [in] the flags delivered with the original WM_L(R)BUTTONDOWN msg that started banding 
//                list     -- [in] the current "locked" list of tree items
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::UpdateSelectionForRect
   (
   LPCRECT pRect, 
   UINT nFlags, 
   CHTIList& list
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // reference to embedded tree control
   BOOL bShiftOn = BOOL(nFlags & MK_SHIFT);                       // state of SHIFT and CTRL keys at time of mousedown 
   BOOL bCtrlOn = BOOL(nFlags & MK_CONTROL);                      //
   CRect rect;
   BOOL bSel;
   POSITION pos;

   HTREEITEM hItem = cTree.GetRootItem();
   while ( hItem )                                                // for each currently visible item in tree:
   {
      bSel = IsSelected( hItem );                                 // is item currently selected?
      pos = list.Find( hItem );                                   // is item in the "locked" list?
      cTree.GetItemRect( hItem, &rect, TRUE );                    // get item label's rect in client coords 

      if ( rect.IntersectRect(rect, pRect) )                      // if item intersects banding rect...
      {
         if ( !bSel && !pos )                                     //    if it is neither selected or locked, select it 
            SetItemState( hItem, TVIS_SELECTED, TVIS_SELECTED );

         else if ( bCtrlOn && pos )                               //    item in "locked" list was originally selected; 
            SetItemState( hItem, 0, TVIS_SELECTED );              //    if CTRL on, it is deselected upon intersect.  

         else if ( bShiftOn && pos )                              //    if SHIFT on and item "locked", it is unlocked  
            list.RemoveAt(pos);                                   //    the first time it intersects band rect. 
      }
      else                                                        // if item does not intersect banding rect... 
      {
         if ( bSel && !pos )                                      //    if it is selected but not locked, deselect it 
            SetItemState( hItem, 0, TVIS_SELECTED );
         else if ( pos )                                          //    if it is in locked list, always select it
            SetItemState( hItem, TVIS_SELECTED, TVIS_SELECTED );
      }

      hItem = cTree.GetNextVisibleItem( hItem );                  // move on to next visible item in tree 
   }

   UpdateWindow();                                                // force an immediate repaint 
}


//=== SelectAllIgnore ================================================================================================= 
//
//    A special, protected version of SelectAll() -- to avoid multiple notifications for a particular tree item.  It 
//    selects or deselects all visible items (i.e., items in an expanded subtree) in the tree, ignoring specified item. 
//
//    ARGS:       bSelect  -- [in] if TRUE, select all items 
//                hIgnore  -- [in] item to be skipped  
//
//    RETURNS:    NONE. 
//
void CMultiDragTreeView::SelectAllIgnore
   (
   BOOL bSelect, 
   HTREEITEM hIgnore
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // reference to embedded tree control

   bSelect = !!bSelect;                                           // ensure 0 or 1
   UINT nState = bSelect ? TVIS_SELECTED : 0;                     // the selection state 
   HTREEITEM hItem = cTree.GetRootItem();
   while ( hItem )                                                // select/deselect all visible items in tree 
   {
      if ( hItem != hIgnore )                                     // ... except the ignored item 
      {
         if ( IsSelected( hItem ) != bSelect )
         SetItemState( hItem, nState, TVIS_SELECTED ); 
      }
      hItem = cTree.GetNextVisibleItem( hItem );
   }
}


//=== ExpandEx ======================================================================================================== 
//
//    Wraps CTreeCtrl::Expand() with the TVN_ITEMEXPANDING and TVN_ITEMEXPANDED notifications.  Use this to emulate 
//    what happens when the user clicks on the expand/collapse button next to a parent item.  CTreeCtrl::Expand() does 
//    NOT issue these notifications. 
//
//    ARGS:       hti   -- [in] handle to a tree item
//                code  -- [in] the type of action to be taken, same as for CTreeCtrl::Expand()
//
//    RETURNS:    TRUE if fcn completed successfully; FALSE otherwise
//
BOOL CMultiDragTreeView::ExpandEx
   ( 
   HTREEITEM hti,
   UINT code
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // get reference to embedded tree control 

   if ( hti == NULL ) return( FALSE );                            // invalid item handle 
   
   code &= (TVE_TOGGLE|TVE_COLLAPSERESET);                        // restrict action code to allowed values 
                                                                  // TVE_TOGGLE = TVE_EXPAND|TVE_COLLAPSE
   if ( (code == 0) ||                                            // bad action code 
        ( ((code & TVE_COLLAPSERESET) == TVE_COLLAPSERESET) &&    // TVE_COLLAPSE must accompany TVE_COLLAPSERESET  
          ((code & TVE_COLLAPSE) != TVE_COLLAPSE) )
      )
      return( FALSE );

   TV_ITEM tvi;                                                   // make sure item exists and get its expand state
   tvi.mask = TVIF_HANDLE | TVIF_STATE;
   tvi.hItem = hti;
   tvi.state = 0;
   tvi.stateMask = TVIS_EXPANDED;
   if ( !cTree.GetItem( &tvi ) )                                  // couldn't find item!
      return( FALSE );

   BOOL bWasExpanded = FALSE;                                     // deny operations which make no sense...
   if ( (tvi.state & TVIS_EXPANDED) == TVIS_EXPANDED ) 
      bWasExpanded = TRUE;
   UINT expandCode = code & TVE_TOGGLE; 
   if ( ( bWasExpanded && (expandCode == TVE_EXPAND) ) ||         // ...cannot expand an already expanded item 
        ( (!bWasExpanded) && (expandCode == TVE_COLLAPSE) )       // ...cannot collapse an already collapsed item
      ) 
      return( FALSE );


   NMTREEVIEW nmtv;                                               // set up notification info 
   nmtv.hdr.hwndFrom = m_hWnd;
   nmtv.hdr.idFrom = ::GetDlgCtrlID( m_hWnd );
   nmtv.itemNew.mask = TVIF_HANDLE|TVIF_PARAM|TVIF_STATE;
   nmtv.itemNew.hItem = hti;
   nmtv.itemNew.lParam = cTree.GetItemData( hti );
   nmtv.itemNew.state = 
      GetItemState( hti, TVIS_EXPANDED ) & TVIS_EXPANDED;
   nmtv.itemNew.stateMask = TVIS_EXPANDED;
   nmtv.action = code;

   nmtv.hdr.code = TVN_ITEMEXPANDING;
   if ( _SendNotify( &(nmtv.hdr) ) )                              // send TVN_ITEMEXPANDING 
      return( FALSE );
   else                                                           // parent allowed the operation to proceed
   {
      if ( !cTree.Expand( hti, code ) )                           // expand/collapse the item 
         return( FALSE );
      else
      {                                                           // success! send TVN_ITEMEXPANDED 
         nmtv.itemNew.state = 
            GetItemState( hti, TVIS_EXPANDED ) & TVIS_EXPANDED;
         nmtv.hdr.code = TVN_ITEMEXPANDED;
         _SendNotify( &(nmtv.hdr) );
         return( TRUE );
      }
   }
   
}


//=== DeleteItemEx ==================================================================================================== 
//
//    Fix for CTreeCtrl::DeleteItem().  Derived classes which need to use CTreeCtrl::DeleteItem() MUST use this 
//    function instead.
//
//    If we should empty an expanded parent in the process of deleting a child, the native CTreeCtrl::DeleteItem() call 
//    fails to clear the parent's TVIS_EXPANDED flag and erase the children indicator (TVS_HASBUTTONS style).  It also 
//    does not send a TVN_ITEMEXPANDED notification to indicate the change.  Use this function to correct the behavior. 
//    If the specified item has no parent or still has other siblings, we merely delete it in the normal fashion.  
//    Otherwise, we collapse the parent using ExpandEx() [which sends the TVN_ITEMEXPANDING/ED notifications] before 
//    performing the deletion.
//
//    NOTE:  It is VITAL to collapse the parent before deleting its last child.  If done in the opposite order, 
//    we run into a bug in the Windows tree view control implementation such that new children can no longer be 
//    inserted correctly into the parent item.
//
//    ARGS:       hti   -- [in] handle to tree item to be deleted; if TVI_ROOT, all items are deleted.
//
//    RETURNS:    TRUE if fcn completed successfully; FALSE otherwise
//
BOOL CMultiDragTreeView::DeleteItemEx
   (
   HTREEITEM hti
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                                    // get reference to embedded tree control 

   HTREEITEM htiP = cTree.GetParentItem( hti );                         // if item has a parent...
   if ( htiP != NULL )                                                  
   {
      HTREEITEM htiChild = cTree.GetChildItem( htiP );
      if ( (htiChild == hti) &&                                         // ...and it's the parent's last child...
           (cTree.GetNextSiblingItem( htiChild ) == NULL) )
      {
         UINT s = GetItemState( htiP, TVIS_EXPANDED ) & TVIS_EXPANDED; 
         if ( s == TVIS_EXPANDED )                                      // ...and the parent is expanded...
            ExpandEx( htiP, TVE_COLLAPSE );                             // ...then collapse parent FIRST!
      }
   }

   return( cTree.DeleteItem( hti ) );                                   // delete the item 
}


//=== _SendNotify ===================================================================================================== 
//
//    Helper function to distinguish between default CTreeCtrl-generated notifications and those notifications which 
//    were emulated by this class.  If a derived class or parent window handles the TVN_SELCHANGED(ING) notifications, 
//    it can call IsEmulatedNotify() to determine what's actually going on.  See SetItemState() comment header.
//
//    ARGS:       pNMHDR   -- [in] pointer to an NM_TREEVIEW struct (see Platform SDK doc). 
//
//    RETURNS:    TRUE or FALSE, depends on the particular notification sent; sometimes ignored.  
//
BOOL CMultiDragTreeView::_SendNotify
   (
   LPNMHDR pNMHDR
   )
{
   ASSERT( ::GetParent(m_hWnd) ); 

   BOOL b = m_bEmulated;
   m_bEmulated = TRUE;
   BOOL bRes = static_cast<BOOL>(::SendMessage( ::GetParent( m_hWnd ), WM_NOTIFY, (WPARAM)pNMHDR->idFrom, (LPARAM)pNMHDR ));
   m_bEmulated = b;
   return( bRes );
}


//=== RunContextMenu ================================================================================================== 
//
//    This function is called whenever the view detects a user input event that should invoke a context menu.  It 
//    obtains the context menu from the helper function GetContextMenu().  Derived classes must override that method in 
//    order to provide support for a context menu.
//
//    TECHNICAL NOTES: 
//       (1) See PreTranslateMessage() override and OnRClick() handler for the routes by which this method is invoked. 
//       (2) The "applicable" tree item is provided so that derived classes have an opportunity to tailor the context 
//    menu depending upon the nature of that item.  When the context menu is invoked by the keyboard, the "applicable" 
//    item is the focused & selected item, or the first selected item.  When invoked by a right-click, it is the item 
//    under the mouse.  In both cases, the applicable tree item could be NULL!!
//       (3) To tranparently support automatic menu item updating in a 
//    derived class via the ON_UPDATE_COMMAND_UI message map macro, we must give the TrackPopupMenu() method a pointer 
//    to the main application window, which is equipped to handle and pass on the relevant messages.
//
//
//    ARGS:       pWnd  -- [in] handle to the CWnd in which "context menu" event occurred.
//                point -- [in] where to place top-left corner of popup menu, in screen coords.  context menu will 
//                         only be displayed if this pos is inside the client area. 
//                hti   -- [in] tree item to which "context menu" could apply (possibly NULL)
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::RunContextMenu
   ( 
   CWnd* pWnd, 
   CPoint point,
   HTREEITEM hti
   )
{
   CRect rect;
   GetTreeCtrl().GetWindowRect( &rect );                       // get window rect of tree control (screen coords). 

   if ( rect.PtInRect( point ) )                               // inside window:  run context menu if there is one...
   { 
      CMenu menu;
      int iSubMenu;
      if ( GetContextMenu( hti, menu, iSubMenu ) )
      {
         CMenu* pContext;
         if ( iSubMenu >= 0 ) 
            pContext = menu.GetSubMenu( iSubMenu );
         else
            pContext = &menu;

         if ( pContext != NULL )
            pContext->TrackPopupMenu( 
               TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
               point.x, point.y, AfxGetMainWnd() );
      }
   }
   else                                                        // outside window:  pass on to base class...
      CTreeView::OnContextMenu( pWnd, point );
}


//=== GetContextMenu [overridable] ==================================================================================== 
//
//    This method is called by RunContextMenu to load an application-specific context menu.
//
//    Derived classes MUST override this method to display a context menu.  All we do here is return FALSE, indicating 
//    that no context menu was loaded. 
//
//    TECHNICAL NOTES:  
//       (1) We provide an "applicable item" to give a derived class the opportunity to tailor the context menu to the 
//    nature of that item.  If the context menu was invoked by a right-click, the applicable tree item is the item 
//    clicked.  If invoked by keyboard, it is the item which is both focused and selected at the time.  In both cases, 
//    it is possible that there may be no such item (NULL).  
//       (2) The CMenu object passed to this method has no real menu (HMENU) attached to it.  It is the job of this 
//    method to do so.  Often menus are created as resources, and it may be the case that the desired context menu is 
//    actually a submenu of another menu.  We provide the third argument in support of that scenario.
//
//    ARGS:       hti   -- [in] the applicable tree item at the time of context-menu invocation (possibly NULL). 
//                m     -- [in] an empty menu object;
//                      -- [out] the loaded context menu, or another menu containing it as a submenu. 
//                iSub  -- [out] if >= 0, indicates position of submenu that represents the context menu to display;
//                         else the menu itself will be taken as the context menu. 
//
//    RETURNS:    TRUE if a context menu was loaded; FALSE otherwise. 
//
BOOL CMultiDragTreeView::GetContextMenu( HTREEITEM hti, CMenu& m, int& iSub )
{
   return( FALSE );
}


//=== PreRealizeDrag ================================================================================================== 
//
//    Constructs the list of valid drag items from the current selection set and clears the entire selection.  A valid 
//    drag item cannot be a descendant of another drag item, and it must pass the test of the virtual fcn CanDrag().
//    This function should only be called when a drag-n-drop operation has just finished, and just prior to calling 
//    RealizeDrag().
//
//    ARGS:       htiDragList -- [out] the list of valid drag items. 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::PreRealizeDrag
   ( 
   CHTIList& htiDragList 
   )
{
   GetSelectedList( htiDragList );                          // get the current selection set 

   POSITION pos = htiDragList.GetHeadPosition();            // iterate through list: 
   while( pos != NULL )
   {
      HTREEITEM hti = htiDragList.GetNext( pos );
      if ( CanDrag( hti ) )                                 //    if item is draggable... 
         SelectChildren( hti, FALSE, TRUE );                //    ...then deselect all of its descendants 
      else
         SetItemState( hti, 0, TVIS_SELECTED );             //    otherwise we only deselect the item itself (its 
                                                            //    descendants could be draggable) 
   }

   GetSelectedList( htiDragList );                          // get the revised selection set, which should only include 
                                                            // valid drag items 
   SelectAll( FALSE );                                      // deselect all selected items 
}


//=== RealizeDrag [overridable] ======================================================================================= 
//
//    Modify the tree when the user completes a drag-n-drop operation.  The default response is to move or copy the 
//    dragged items (including all descendants) to the new location.  When called, the previous selection state of the 
//    tree (during the drag) has been cleared.  We leave the tree in the following state:  the drop target item is 
//    expanded, all items which were added to it are selected, and the last item added receives the focus. 
//
//    NOTE:  PreRealizeDrag() must be called immediately prior to this function to prepare a valid drag item list and 
//    to clear the previous selection state of the tree!
//
//    Derived classes can override this fcn for app-specific behavior.  For example, the override might perform some 
//    app-specific work required by the drag-n-drop operation, then call the base class to update the tree if that 
//    work was completed successfully. 
//
//    ARGS:       htiDragList -- [in] the items that were dragged (non-draggable items and descendants of drag 
//                                    items have been removed)
//                bCopy       -- [in] if TRUE, copy the drag items and their subtrees; otherwise, move them.  
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::RealizeDrag
   ( 
   CHTIList& htiDragList,
   BOOL bCopy
   )
{
   ASSERT( m_hItemDrop );                                      // there must be a current drop target 

   if ( htiDragList.IsEmpty() ) return;                        // no items were dragged; there's nothing to do!
   
   CTreeCtrl& cTree = GetTreeCtrl();                           // get reference to embedded tree control 

   HTREEITEM htiLast = NULL;                                   // the last item added to the drop target 
   POSITION pos = htiDragList.GetHeadPosition();               // iterate thru list of valid drag items: 
   while( pos != NULL )
   {
      HTREEITEM hti = htiDragList.GetNext( pos ); 
      HTREEITEM htiNew = CopyBranch( hti, m_hItemDrop );       //    copy the entire subtree under the drop target 
      if ( htiNew != NULL )                                    //    if successful, select the item and delete it from 
      {                                                        //    its original location if we're doing a "move" 
         SetItemState( htiNew, TVIS_SELECTED, TVIS_SELECTED );
         if ( !bCopy ) DeleteItemEx( hti ); 
         htiLast = htiNew;
      }
   }

   Sort( m_hItemDrop );                                        // sort the drop target's children 

   UINT s = GetItemState( m_hItemDrop, TVIS_EXPANDED );        // if drop target is not expanded, do so
   if ( (s & TVIS_EXPANDED) != TVIS_EXPANDED )
      ExpandEx( m_hItemDrop, TVE_EXPAND );

   if ( htiLast != NULL )                                      // put focus on the last item added to drop target 
   {
      HTREEITEM htiOldFocus = GetFocusedItem();                // first be sure to take away both selection and focus 
      SetItemState( htiOldFocus,                               // from the old focus item, otherwise that item will 
                    0, TVIS_FOCUSED|TVIS_SELECTED );           // remain selected even if it loses the focus 
      SetItemState( htiLast, TVIS_FOCUSED, TVIS_FOCUSED );
   }
}


//=== RunRightDragMenu [overridable] ================================================================================== 
//
//    Called when the user completes a valid right-button drag-n-drop.  Displays a simple right-drag context menu 
//    prompting user to either move or copy the drag item, or cancel altogether.
//
//    Derived classes can override the fcn to customize the appearance and effects of a right drag.  In such a case, 
//    the derived class must provide the appropriate command handlers for each option in the custom menu. 
//
//    ARGS:       ptDrop   -- [in] drop location in screen coords.  We ASSUME this point is within the view rect. 
//
//    RETURNS:    NONE
//
void CMultiDragTreeView::RunRightDragMenu
   ( 
   CPoint dropPt
   )
{
   // create the popup menu.  we set the cancel option apart w/ a separator. 
   //
   CMenu menu;
   menu.CreatePopupMenu();
   menu.AppendMenu( MF_STRING, ID_MDTV_MOVE, _T("Move") );
   menu.AppendMenu( MF_STRING, ID_MDTV_COPY, _T("Copy") ); 
   menu.AppendMenu( MF_SEPARATOR );
   menu.AppendMenu( MF_STRING, ID_MDTV_CANCEL, _T("Cancel") ); 

   // display the popup menu. 
   //
   menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, dropPt.x, dropPt.y, this );
}


//=== RealizeDelete [overridable] ===================================================================================== 
//
//    Delete the tree item(s) in the specified list.  The default response is to delete all items which can be deleted 
//    according to helper fcn CanDelete(). 
//
//    Derived classes can override this fcn for app-specific behavior.  This method is called internally by 
//    CMultiDragTreeView when the user tries to delete a the current selection using the DELETE key.  Derived classes 
//    can also use it to implement menu-initiated delete operations.
//
//    !!!IMPORTANT!!!  Derived classes should use CMultiDragTreeView::GetSelectedList() to obtain the list of currently 
//    selected items.  This method ensures that, if an item is selected, none of its descendants are.  When we delete 
//    an item, all of its descendants are deleted, so it does not make sense to include an item and one or more of its 
//    descendants in the selected list...
//
//    ARGS:       list   -- [in] list of tree items to be deleted. 
//
//    RETURNS:    1  : all items were successfully removed.  
//                0  : some but not all items were successfully removed. 
//                -1 : no object was removed. 
//
int CMultiDragTreeView::RealizeDelete( CHTIList& list ) 
{
   BOOL bDel = FALSE;                              // at least one item deleted 
   BOOL bNoDel = FALSE;                            // at least one item could not be deleted 
   while ( !list.IsEmpty() )
   {
      HTREEITEM hti = list.RemoveHead();
      if ( CanDelete( hti ) ) 
      {
         bDel = TRUE;
         DeleteItemEx( hti );
      }
      else
         bNoDel = TRUE;
   }

   if ( bDel && !bNoDel )     return( 1 );
   else if ( bDel && bNoDel ) return( 0 );
   else                       return( -1 );
}




//=== CanDrag, CanDrop, CanEdit, CanDelete [overridable] ============================================================== 
//
//    These overridables allow a derived class to disable various user-initiated operations for individual items in the 
//    tree control.  The base class versions enable the operations for all items.  
//
//    ARGS:       hti   -- [in] handle of tree item. 
//
//    RETURNS:    TRUE if operation is permitted on specified item, FALSE otherwise. 
//
BOOL CMultiDragTreeView::CanDrag( HTREEITEM hti ) { return( BOOL(hti != NULL) ); }
BOOL CMultiDragTreeView::CanDrop( HTREEITEM hti ) { return( BOOL(hti != NULL) ); }
BOOL CMultiDragTreeView::CanEdit( HTREEITEM hti ) { return( BOOL(hti != NULL) ); }
BOOL CMultiDragTreeView::CanDelete( HTREEITEM hti ) { return( BOOL(hti != NULL) ); }


//=== AcceptNewLabel [overridable] ==================================================================================== 
//
//    Accept/reject a new label for the specified tree item.  The default response is to accept all non-empty labels. 
//
//    This overridable is called after the user completes an in-place edit.  Derived classes can override this fcn to 
//    reject label names if necessary, perform any other work resulting from the label change, etc.
//
//    ARGS:       hti   -- [in] tree item handle. 
//                str   -- [in] proposed new label for the item. 
//
//    RETURNS:    TRUE to accept the new label, FALSE to reject it. 
//
BOOL CMultiDragTreeView::AcceptNewLabel
   ( 
   HTREEITEM hti, 
   CString& str 
   ) 
{
   return( !str.IsEmpty() );
}


//=== GetExpandBitmaps [overridable] ================================================================================== 
//
//    This overridable is called immediately after a parent item is expanded or collapsed.  Derived classes which use 
//    image lists can override this fcn to associate particular bitmaps with an item when it is expanded or collapsed. 
//    The default behavior is to return invalid image position values, since CMultiDragTreeView does not associate 
//    image lists with the control. 
//
//    ARGS:       hti      -- [in] tree item handle. 
//                bExpand  -- [in] TRUE if item was just expanded, FALSE if it was just collapsed.  
//                piImg    -- [out] pos of "unselected" bitmap in image list. 
//                piSelImg -- [out] pos of "selected" bitmap in image list. 
//
//    RETURNS:    NONE 
//
void CMultiDragTreeView::GetExpandBitmaps
   ( 
   HTREEITEM hti, 
   BOOL bExpand, 
   int* piImg,
   int* piSelImg
   )
{
   *piImg = -1;
   *piSelImg = -1;
}




