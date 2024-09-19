//===================================================================================================================== 
//
// cxobjtree.h : Declaration of class CCxObjectTree.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXOBJTREE_H__INCLUDED_)
#define CXOBJTREE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "mdrgtree.h"                     // base class CMultiDragTreeView 
#include "cxdoc.h"                        // the CCxDoc class encapsulates the associated document 
#include "cxviewhint.h"                   // CCxViewHint -- the "hint" class used by all MAESTRO views 



//===================================================================================================================== 
// Declaration of class CCxObjectTree
//===================================================================================================================== 
//
class CCxObjectTree : public CMultiDragTreeView
{
   DECLARE_DYNCREATE( CCxObjectTree )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int BM_WIDTH = 20;     // all object item icons have this width
   typedef enum                        // these indices correlate bitmap usage with pos in the image list associated 
   {                                   // with the embedded tree control:
      SUB_COLLAPSED = 0,               //    base of a MAESTRO subtree -- collapsed, expanded
      SUB_EXPANDED,
      TGSET_COLLAPSED,                 //    a target set -- collapsed, expanded
      TGSET_EXPANDED,
      TG_NORMAL,                       //    a target -- unselected, selected
      TG_SELECTED,
      TRSET_COLLAPSED,                 //    a trial set OR subset -- collapsed, expanded
      TRSET_EXPANDED,
      TR_NORMAL,                       //    a trial -- unselected, selected
      TR_SELECTED,
      CH_NORMAL,                       //    a channel configuration -- unselected, selected
      CH_SELECTED,
      CRSET_COLLAPSED,                 //    a continous-mode run set -- collapsed, expanded
      CRSET_EXPANDED,
      CR_NORMAL,                       //    a continuous-mode run -- unselected, selected
      CR_SELECTED,
      PERT_NORMAL,                     //    a perturbation object -- unselected, selected
      PERT_SELECTED
   } ObjItemState;
   static COLORREF BM_MASKCOLOR;       // mask color associated with the item icon bitmaps 

   static const int ID_OBJ_SUBMENU = 3; // zero-based index of "Object" submenu within the IDR_MAINFRAME app menu


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   static CImageList m_imgIcons;       // image list for displaying item icons for MAESTRO objects displayed in tree 
   CCxViewHint m_hint;                 // MAESTRO update hint for notifying other views, mainframe of doc change


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
protected:
   CCxObjectTree() {}                                    // (NOTE: used by dynamic object creation mechanism) 
   ~CCxObjectTree() {} 
                                                         // use default copy constructor and operator=



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnObjectOp( UINT cmdID );                // execute the requested operation on the object tree 
   afx_msg void OnUpdObjectOps( CCmdUI* pCmdUI);         // update state of "Object" menu command items dynamically 
   afx_msg void OnCopyRemote();                          // copy selected objects to another experiment doc (file op)
   DECLARE_MESSAGE_MAP()



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:
   void OnInitialUpdate();                               // prepare to display tree view in initial state 
   void OnUpdate( CView* pSender, LPARAM lHint,          // update appearance of the object tree 
                  CObject* pHint );

   BOOL IsObjSelected(WORD wKey);                        // is specified document obj currently selected in tree?


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only) -- We currently rely on base class implementations of Dump() & AssertValid()...
//===================================================================================================================== 
public:
#ifdef _DEBUG 
   // void Dump( CDumpContext& dc ) const; 
   // void AssertValid() const; 
#endif



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   CCxDoc* GetDocument() const                           // return attached document pointer cast to derived doc class 
   { 
      ASSERT( m_pDocument != NULL );
      ASSERT_KINDOF( CCxDoc, m_pDocument );
      return( (CCxDoc*) m_pDocument ); 
   }
   VOID Notify( const BOOL bMod = TRUE );                // inform doc/view and mainframe of change made in this vu 

   HTREEITEM CreateObjectItem(                           // create new MAESTRO obj AND assoc. tree view item; send 
      const HTREEITEM htiDst,                            // "update" hint to other views. 
      const HTREEITEM htiInsPos,
      const WORD newTyp );
   int CopyObjectItems(                                  // copy list of tree items AND assoc. MAESTRO objects; send 
      CHTIList& htiList, HTREEITEM& hLast );             // "update" hint to other views. 
   VOID ClearUserObjectItems();                          // remove all user-defined objs from MAESTRO obj tree 

   WORD ItemToObject( HTREEITEM h ) const                // get unique key of MAESTRO obj associated w/ tree item 
   { 
      ASSERT( h );
      return( LOWORD(GetTreeCtrl().GetItemData( h )) );
   }
   HTREEITEM ObjectToItem( WORD key,                     // find tree item assoc w/ specified MAESTRO obj
                  HTREEITEM htiBase = NULL,
                  BOOL bDeep = TRUE ) const;

   HTREEITEM                                             // insert tree item assoc w/ specified MAESTRO obj
   InsertObjItem( const WORD key, CTreeObj* pObj, 
      UINT nState, HTREEITEM htiDst, 
      HTREEITEM htiAfter = TVI_LAST );

   VOID RefreshBranch( HTREEITEM hti );                  // rebuild tree branch rooted at specified item  
   VOID GetBitmapIDs(                                    // retrieve bitmap IDs for tree item based on type of MAESTRO 
         const WORD objType, const BOOL bExpand,         // object that item represents 
         int* piImg, int* piSelImg ) const;

   void CustomDblClk( const HTREEITEM hti );             // display params of MAESTRO obj assoc. w/ dbl-clicked item 
   void RealizeDrag( CHTIList& htiDragList, BOOL bCopy); // update doc and object tree as a result of a drag-n-drop 
   int RealizeDelete( CHTIList& htiList );               // attempt to delete specified list of items from tree
   BOOL CanDrop( HTREEITEM hti );                        // can tree item serve as a target for drag-n-drop? 
   BOOL AcceptNewLabel( HTREEITEM hti, CString& str );   // accept/reject new label for this item; update doc 
   void GetExpandBitmaps( HTREEITEM hti, BOOL bExpand,   // toggle expand/collapse bitmap icons for parent tree item 
                          int* piImg, int* piSelImg ); 
   BOOL GetContextMenu(                                  // provide context menu for this tree view
            HTREEITEM hti, CMenu& m, int& iSub );

};


#endif   // !defined(CXOBJTREE_H__INCLUDED_)
