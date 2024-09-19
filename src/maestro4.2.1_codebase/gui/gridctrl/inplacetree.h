//===================================================================================================================== 
//
// inplacetree.h : Declaration of class CInPlaceTree.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(INPLACETREE_H__INCLUDED_)
#define INPLACETREE_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


// prototype for the callback function used by inplace tree control to get children info for populating a tree node:
//
//    DWORD dwKey                -- [in] unique key identifying tree node.
//    CStringArray* pstrArLbls   -- [out] labels for the node's children (maybe NULL).
//    CDWordArray* pdwArKeys     -- [out] unique keys identifying the node's children (maybe NULL).
//    CByteArray* pbArHasKids    -- [out] flag array indicating if each child also has children (maybe NULL).
//    LPARAM lParam              -- [in] generic callback arg -- value specified by the callback provider
//
//    Return value               -- if arrays provided, return # of children under specified node.
//                               -- if arrays are NULL, return 0 if node is childless, nonzero otherwise.
//
typedef int (CALLBACK* IPTREECB)( DWORD, CStringArray*, CDWordArray*, CByteArray*, LPARAM );


class CInPlaceTree : public CTreeCtrl
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   UINT        m_nExitChar;            // set to char key which extinguished inplace operation, 0 otherwise
   DWORD       m_dwKeyRoot;            // key associated with TVI_ROOT item in tree control (root item not shown)
   DWORD       m_dwKeyInitial;         // key associated with tree item initially selected when inplace tree shown
   DWORD       m_dwKeySelected;        // key associated with tree item selected AFTER inplace tree extinguished
   BOOL        m_bRestoreParent;       // TRUE to restore parent to its original owner/parent when control is hidden 
   BOOL        m_bAlreadyEnding;       // gate flag prevents reentrancy in EndEdit()
   BOOL        m_bInitializing;        // gate flag set during BeginEdit() to populate tree out to a selected node

   IPTREECB    m_pfnTreeCB;            // callback fcn required to populate the tree on demand
   LPARAM      m_lpTreeCBArg;          // generic arg for callback fcn, specified by callback provider


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CInPlaceTree( CWnd* pOwner, DWORD dwStyle, UINT nID );
   ~CInPlaceTree();


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnKillFocus( CWnd* pNewWnd );            // extinguish inplace ctrl when it loses focus
   afx_msg void OnChar( UINT nChar, UINT nRepCnt,        // catches char keys which should terminate inplace ctrl
                        UINT nFlags );
   afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt,     // terminate on arrow keys
                           UINT nFlags );
   afx_msg void OnLButtonDblClk( UINT nFlags,CPoint p ); // if user double-clicks a leaf node, extinguish tree ctrl 
   afx_msg UINT OnGetDlgCode() 
   { return( DLGC_WANTALLKEYS ); }

   afx_msg void OnItemExpanding(                         // handle reflected TVN_ITEMEXPANDING notification 
                     NMHDR* pNMHDR, LRESULT* pResult );
   afx_msg void OnItemExpanded(                          // handle reflected TVN_ITEMEXPANDED notification 
                     NMHDR* pNMHDR, LRESULT* pResult );

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ATTRIBUTES
//===================================================================================================================== 
public:
   VOID SetCallback( IPTREECB pCB, LPARAM lParam )       // install the tree info callback fcn -- NEVER invoke when 
   {                                                     // inplace tree is in use!
      if( GetSafeHwnd() != NULL && IsWindowVisible()) { ASSERT(FALSE); return; }
      m_pfnTreeCB = pCB; 
      m_lpTreeCBArg = lParam; 
   }
   IPTREECB GetCallback() { return( m_pfnTreeCB ); }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL BeginEdit( CWnd* pParent, const CRect& rect,     // initiate an inplace op by showing & activating ctrl 
                   const CDWordArray& dwArInitChain );
   void CancelEdit();                                    // cancel the inplace op and hide ctrl 

   DWORD GetSelectedKey();                               // retrieve the key of the currently selected node in tree
   UINT GetExitChar() { return( m_nExitChar ); }         // retrieve char key that may have extinguished the ctrl 
   BOOL IsChanged()                                      // has selection changed since inplace op started?
   { return( BOOL(m_dwKeyInitial != GetSelectedKey()) ); }

   BOOL PreTranslateMessage( MSG* pMsg );                // technical bug workaround 
   BOOL ModifyStyle( DWORD dwRemove, DWORD dwAdd,        // override to enforce certain style restrictions 
                     UINT nFlags );


//===================================================================================================================== 
// IMPLEMENTATION
//=====================================================================================================================
private: 
   void PopulateRoot();                                  // populate and expand the root of the tree ctrl 
   BOOL PopulateItem( HTREEITEM hParent );               // populate specified tree node 

   void DeleteChildren( HTREEITEM hParent );             // delete all children of the specified tree item 
   BOOL IsLeaf( HTREEITEM hItem );                       // is it really a leaf node? (invokes callback fcn to check!) 
   DWORD GetKey( HTREEITEM hItem );                      // get key value associated with an existing tree item
};
 

#endif // !defined(INPLACETREE_H__INCLUDED_)
