//===================================================================================================================== 
//
// mdrgtree.h : Declaration of class CMultiDragTreeView.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(MDRGTREE_H__INCLUDED_)
#define MDRGTREE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxcview.h>                        // CView and derived classes, including CTreeView 
#include <afxtempl.h>                        // for MFC template classes


typedef CTypedPtrList<CPtrList, HTREEITEM> CHTIList;     // list of currently selected items in tree 


// we use bit0 of the tree control's state/statemask for indicating the item which has the current focus.  it used
// to be defined anyway, before MS removed it.
//
#ifndef TVIS_FOCUSED
   #define TVIS_FOCUSED    1
#else
   #if TVIS_FOCUSED != 1
      #error TVIS_FOCUSED was assumed to be 1
   #endif
#endif


//===================================================================================================================== 
// Declaration of class CMultiDragTreeView
//===================================================================================================================== 
//
class CMultiDragTreeView : public CTreeView
{
   DECLARE_DYNCREATE( CMultiDragTreeView )

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   HTREEITEM   m_hItemDrop;                  // the current drop target item during a drag-n-drop operation 
   HCURSOR     m_hDropCursor;                // cursor shown when drop target is valid 
   HCURSOR     m_hNoDropCursor;              // cursor shown when drop target is invalid 

private:
   CImageList* m_pilDrag;                    // the dragged image list created and used to animate drag-n-drop 
   UINT_PTR    m_nTimerID;                   // the timer resource used during drag-n-drop animation
   int         m_nHover;                     // # consecutive timer ticks mouse hovers over parent item during drag 
   BOOL        m_bRtDrag;                    // TRUE if right drag in progress; FALSE for left drag

   BOOL        m_bMulti;                     // enable/disable multiple selection feature 
   HTREEITEM   m_hSelect;                    // the current base item for a multi-select with SHIFT key 
   BOOL        m_bEmulated;                  // set whenever we emulate TVN_* notifications 



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CMultiDragTreeView();                                 // (NOTE: used by dynamic object creation mechanism) 
   ~CMultiDragTreeView();                                // release dyn alloc resources here 
   // use default copy constructor and operator=



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);  // animate an ongoing drag-n-drop 
   afx_msg void OnLButtonDblClk( UINT nFlags,            // captures left dbl-click to customize its effect 
                                 CPoint point );
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);  // finish an ongoing left-button drag-n-drop 
   afx_msg void OnLButtonDown(UINT nFlags,CPoint point); // handles multi-sel issues
   afx_msg void OnRButtonUp(UINT nFlags, CPoint point);  // finish an ongoing right-button drag-n-drop 
   afx_msg void OnRButtonDown(UINT nFlags,CPoint point); // handles multi-sel issues 
   afx_msg void OnTimer(UINT_PTR nIDEvent );             // handle auto-scrolling and auto-expand during drag-n-drop 
   afx_msg void OnSetFocus( CWnd* pOldWnd );             // ensure *all* selected items are updated with focus change 
   afx_msg void OnKillFocus( CWnd* pNewWnd );            //
   afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt,     // handle multi-sel issues with SHIFT/CTRL, up/down arrows
                           UINT nFlags );
   afx_msg void OnBeginLabelEdit                         // validate tree item for in-place label edit 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnEndLabelEdit                           // accept/reject the edited tree item label 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnItemExpanded                           // change bitmap of parent items (expanded vs collapsed image)  
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnItemExpanding                          // deselect children of a collapsing item 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnRClick                                 // captures single rt-clicks for proper context menu handling 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnBeginDrag                              // initiate a left-button drag-n-drop animation 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnBeginRDrag                             // initiate a right-button drag-n-drop animation 
                  ( NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnRightDragOption( UINT cmdID );         // respond to user's selection from right-drag menu 

   DECLARE_MESSAGE_MAP()



//===================================================================================================================== 
// ATTRIBUTES
//===================================================================================================================== 
   BOOL IsMultiSelect() const { return( m_bMulti ); }    // is multi-selection enabled? 
   BOOL IsEmulatedNotify() const                         // was this tree control notification emulated?
      { return( m_bEmulated ); }



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public: 
   BOOL PreCreateWindow( CREATESTRUCT& cs );             // overridden to set required styles for this tree ctrl 
   BOOL PreTranslateMessage( MSG* pMsg );                // overridden to customize response to some keydowns 

   BOOL SetMultiSelect( BOOL bMulti );                   // enable/disable multi-selection features
   void ResetShiftSelect()                               // reset the base item for a SHIFT-initiated multi-select 
   {
      m_hSelect = NULL;
   }

   BOOL IsSelected( HTREEITEM hti ) const                // specified item is currently selected 
   {
      UINT s = GetItemState( hti, TVIS_SELECTED );
      return( !!(TVIS_SELECTED & s) );
   }
   UINT GetSelectedCount() const;                        // # of tree items currently selected
   HTREEITEM GetFirstSelectedItem() const;               // for iterating through the current selection set 
   HTREEITEM GetNextSelectedItem( HTREEITEM hti ) const; //
   void GetSelectedList( CHTIList& htiList,              // get a list of currently selected tree items 
                         BOOL bNoDescend = FALSE ); 

   void SelectAll( BOOL bSelect = TRUE );                // select all visible items; doesn't affect focus
   void SelectRange( HTREEITEM hFirst, HTREEITEM hLast,  // select range of items
                     BOOL bOnly = TRUE );
   BOOL SelectChildren( HTREEITEM hParent,               // de/select immediate children or all descendants; returns 
                        BOOL bSelect = TRUE,             //    TRUE if focus was on a child item 
                        BOOL bAll = TRUE );

   HTREEITEM GetFocusedItem() const                      // get the item having the focus
   {
      return( GetTreeCtrl().GetSelectedItem() );
   }
   BOOL FocusItem( HTREEITEM hti );                      // transfer focus to specified item 

   // !!!IMPORTANT!!! -- SetItemState, GetItemState, SelectItem
   // Derived classes should not use the CTreeCtrl versions of these functions.  Use these instead!! 
   //
   BOOL SelectItem( HTREEITEM hti );                     // select specified item; behavior depends on multi-sel mode 
   BOOL SetItemState( HTREEITEM hItem,                   // for proper handling of added TVIS_FOCUSED state 
                      UINT nState, UINT nStateMask );    // 
   UINT GetItemState( HTREEITEM hti, UINT nSM ) const;   // 



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public:
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // diagnostic dump of current contents of tree view
   void AssertValid() const;                             // validate tree view state 
#endif



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   BOOL IsDragging() const { return(m_pilDrag!=NULL); }  // is a drag-n-drop operation in progress? 

   BOOL IsValidDropTarget( HTREEITEM hItem );            // is item a valid drop target?
   void StartDragging( HTREEITEM hti, CPoint pt );       // start a drag-n-drop animation 
   CImageList* CreateDragImageEx                         // creates drag image for both single- and multi-selection 
                  ( HTREEITEM htiDrag, CPoint& ptDrag );
   void EndDragging( CPoint point, BOOL bRtDrag );       // complete an ongoing drag-n-drop 

   HTREEITEM CopyItem( HTREEITEM hti, HTREEITEM htiDst,  // copy a single item to a new location 
                       HTREEITEM htiAfter = TVI_LAST );
   HTREEITEM CopyBranch( HTREEITEM hBr, HTREEITEM hDst,  // copy an entire branch of tree to a new location 
                         HTREEITEM hAfter = TVI_LAST); 

   void OnButtonDown                                     // handle multi-sel details upon a left- or right-mousedown 
            ( BOOL bLeft, UINT nFlags, CPoint point );   //
   void DoSelectAndTrack( HTREEITEM hItem, BOOL bLeft,   //
            UINT nFlags, CPoint point, DWORD dwTime );   //
   void DoBanding                                        // message-loop during "rubber-band" multi-selection 
            ( BOOL bLeft, UINT nFlags, CPoint point );   //

   void UpdateSelectionForRect                           // update selected set IAW change in rubber-band rectangle 
            (LPCRECT pRect, UINT nFlags, CHTIList& list);
   void SelectAllIgnore                                  // select all visible items, ignoring specified item 
            ( BOOL bSelect, HTREEITEM hIgnore );

   BOOL ExpandEx( HTREEITEM hti, UINT code );            // emulates user-initiated toggling of TVIS_EXPANDED state 
   BOOL DeleteItemEx( HTREEITEM hti );                   // collapses parent when its last child is deleted
   BOOL _SendNotify( LPNMHDR pNMHDR );                   // emulated tree control notifications are sent with this 

   virtual void CustomDblClk( const HTREEITEM hti ) {}   // override to customize action resulting from left dbl-click 
                                                         // on specified tree item (could be NULL) 

   void RunContextMenu( CWnd* pWnd, CPoint point,        // throw up & track app-specific context menu (if any)
                        HTREEITEM hti ); 
   virtual BOOL GetContextMenu(                          // must override to provide app-specific context menu 
                  HTREEITEM hti, CMenu& m, int& iSub );

   void PreRealizeDrag( CHTIList& htiDragList );         // removes invalid items from drag item list; clears selection 
   virtual void RealizeDrag                              // copy/move dragged items under the drop target 
                  ( CHTIList& htiDragList, BOOL bCopy ); 
   virtual void RunRightDragMenu( CPoint dropPt );       // display right-drag context menu 
   virtual int RealizeDelete( CHTIList& list );          // attempt to delete specified list of items
   virtual BOOL CanDrag( HTREEITEM hti );                // can user drag-n-drop this item? 
   virtual BOOL CanDrop( HTREEITEM hti );                // can user select this item as a drop target for drag-n-drop? 
   virtual BOOL CanEdit( HTREEITEM hti );                // can user edit this item's label? 
   virtual BOOL CanDelete( HTREEITEM hti );              // can user delete this item? 
   virtual BOOL AcceptNewLabel                           // accept new label for this item? 
                  ( HTREEITEM hti, CString& str );
   virtual void GetExpandBitmaps                         // get pos of item bitmaps in image list which reflect the 
                  ( HTREEITEM hti, BOOL bExpand,         //    expanded/collapsed state of item 
                    int* piImg, int* piSelImg );
   virtual void Sort( HTREEITEM hti ) {}                 // sort this item's childen -- default is no sort!

};


#endif   // !defined(MDRGTREE_H__INCLUDED_)
