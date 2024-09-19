//===================================================================================================================== 
//
// cxobjcombo.cpp : Implementation of CCxObjCombo, a combo box control allowing the user to select among the CNTRLX 
//                  object "children" of a specified "parent" object currently defined in the CNTRLX document.
//
// AUTHOR:  saruffner
//
//
// DESCRIPTION: 
// 
// CCxObjCombo is a droplist combo box by which the user selects among sibling CNTRLX objects (all having the same 
// object type) under a specified parent in the CNTRLX object tree.  All such "objects" (representing targets, trials, 
// trial sets, channel configurations, etc.) are stored in the CNTRLX experiment document, CCxDoc.  The intent here is 
// to relieve the parent dialog or view of the details of managing the contents of the combo box. 
//
// ==> Usage:
//    1) Using the MS Dev Studio resource editor, add a normal combo box to the dialog template for your form view or 
// dialog.  Use the "Drop List" type, since CCxObjCombo will enforce this type anyway.  Add a CCxObjCombo member to the 
// view/dialog class, and subclass the combo box control to this member in CFormView::OnInitialUpdate() or 
// CDialog::OnInitDialog().  Alternatively, you can create the HWND from scratch using CCxObjCombo::Create().  After 
// creating the control or subclassing it to an existing combo box, call InitContents() to assign the combo box to a 
// particular parent node in the CNTRLX object tree.  CCxObjCombo will load itself with the names of all the children 
// of that node; a "NONE" selection will also be included, if desired. 
//    2) CCxObjCombo does NOT automatically detect changes in CCxDoc that could affect its contents -- the parent view 
// or dialog must call InitContents() or RefreshContents() as appropriate.  RefreshContents() reloads the list of 
// children objects at the current parent node.  If that node no longer exists, the combo box will be emptied; in this 
// case, call InitContents() to assign the combo box to a different parent node.  If the combo box's current selection 
// (appearing in the read-only edit control) is renamed, CCxObjCombo will update itself accordingly.  If the currently 
// selected object no longer exists, CCxObjCombo will switch to the "NONE" selection if it exists, or to another 
// existing child object.
//    3) Handle the CBN_SELCHANGE notification from the combo box.  GetObjKey() returns key of the CNTRLX object whose 
// name is currently selected in the control.  If "NONE" is selected, CX_NULLOBJ_KEY is returned.  NOTE:  If the "NONE" 
// selection is not included in the combo box and the current parent node has no children, then the combo box is 
// empty.  In this situation, GetObjKey() still returns CX_NULLOBJ_KEY.  Use IsEmpty() to check whether or not there 
// are any entries in the combo box.
//    4) Use SetObjKey() to set the current selection of the combo box via CNTRLX object key.  If the key does not 
// reference a child of the currently assigned parent node, the call will fail.  Passing CX_NULLOBJ_KEY to SetObjKey() 
// will set the "NONE" selection, IF it is included in the combo box.
//    5) Special case: CCxObjCombo can be specially configured to display all trials in a a trial set, even if they
// are "grandchildren" of the set because they're ensconced in trial subsets under that set. See InitContents().
//
// REVISION HISTORY:
//
// 01nov2001-- Created, based on the more specific CCxChannelCombo, which will be replaced by CCxObjCombo.
// 03dec2014-- Mod to handle a special case. When the parent is a trial set, CCxObjCombo lists all trials in the set,
// even if some trials are ensconced in trial subset objects and are thus "grandchildren" of the set. In this scenario,
// the item string takes the form "subsetName : trialName". The trial subset objects themselves are NOT included in the
// list. A special configuration flag must be specified in InitContents() to get this behavior -- which is desired
// for the "current trial" combo box in TrialMode's "Protocol" dialog panel. Trial subsets were introduced in v3.1.2.
// 04dec2014-- Mod to set minimum drop list width to accommodate longest item string.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX 

#include "cxobj_ifc.h"                       // CNTRLX object interface:  common constants and other defines 
#include "cxdoc.h"                           // CCxDoc class
#include "cxobjcombo.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

CCxObjCombo::CCxObjCombo()
{
   m_wParentKey = CX_NULLOBJ_KEY;
   m_bAllowNone = FALSE;
   m_bTrialsOnly = FALSE;
}

//=== Create, ModifyStyle, SubclassDlgItem [base overrides] =========================================================== 
//
//    We override these methods to enforce the "drop list" style on the combo box.  Also, we reset the contents of the 
//    combo box in SubclassDlgItem(), in case we're subclassed to a non-empty combo box HWND.
//
//    ARGS:       see base class versions.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//    
BOOL CCxObjCombo::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
   dwStyle &= ~(CBS_SIMPLE | CBS_DROPDOWN);
   dwStyle |= CBS_DROPDOWNLIST;
   return( CComboBox::Create( dwStyle, rect, pParentWnd, nID ) );
}

BOOL CCxObjCombo::ModifyStyle( DWORD dwRemove, DWORD dwAdd, UINT nFlags /* = 0 */ )
{
   if( (dwRemove & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST ) return( FALSE );
   dwAdd |= CBS_DROPDOWNLIST;
   return( CComboBox::ModifyStyle( dwRemove, dwAdd, nFlags ) );
}

BOOL CCxObjCombo::SubclassDlgItem( UINT nID, CWnd* pParent )
{
   if( !CWnd::SubclassDlgItem( nID, pParent ) ) return( FALSE );
   ModifyStyle( 0, CBS_DROPDOWNLIST );
   InitContents( CX_NULLOBJ_KEY, FALSE );
   return( TRUE );
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

/** InitContents ====================================================================================================== 
 Reinitialize combo box contents to display the names of the specified MAESTRO object's children.  Optionally include 
 a "NONE" choice, corresponding to the CX_NULLOBJ_KEY key value. If specified parent object has no children, the combo 
 box will contain no entries (except for "NONE", if included). The initial selection is set to the first entry in the 
 list, which will be "NONE" or the first child under specified parent.

 @param wParent Object key of parent whose children populate the combo box list; if CX_NULLOBJ_KEY, contents are reset.
 @param bAllowNone If TRUE, a "NONE" choice is included in the list.
 @param bTrialsOnly. If TRUE, then the combo box behaves differently when parent object is a trial set (CX_TRIALSET).
 A trial set can contain both trial subsets and trials. Normally, only immediate children are listed. But if this flag 
 is set, then the combo box lists all trials in the set, even those ensconced within a trial subset under the parent 
 set. In this scenario the child subsets are omitted, and the item string for a "grandchild" trial will  include the 
 name of its subset parent: "subsetName : trialName".

 @return True if successful, false otherwise (parent object does not exist in MAESTRO document, or no doc).
*/
BOOL CCxObjCombo::InitContents( WORD wParent, BOOL bAllowNone, BOOL bTrialsOnly /* = FALSE */ )
{
   // if we can't get a reference to the MAESTRO experiment doc (should never happen), then reset the combo box.
   // If the specified parent object does not exist, leave combo box unchanged.
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc(); 
   if((wParent != CX_NULLOBJ_KEY) && (pDoc == NULL))  
   {
      ASSERT(FALSE); 
      ResetContent();
      m_wParentKey = CX_NULLOBJ_KEY;
      m_bAllowNone = FALSE;
      m_bTrialsOnly = FALSE;
      return(FALSE);
   }
   if((wParent != CX_NULLOBJ_KEY) && !pDoc->ObjExists(wParent))
      return( FALSE );
   
   // refresh contents based on new parent object
   m_wParentKey = wParent; 
   m_bAllowNone = bAllowNone;
   m_bTrialsOnly = bTrialsOnly;
   RefreshContents(TRUE);
   return(TRUE);
}


//=== RefreshContents ================================================================================================= 
//
//    Reload the contents to reflect the names of the children of the CNTRLX parent object that is currently assigned 
//    to this combo box.
//
//    If there is no CNTRLX experiment document (CCxDoc), if no parent is assigned, or if the assigned parent has no 
//    children, the combo box will be empty (except for the "NONE" choice, when allowed).  For each child object found 
//    under the assigned parent, its name is stored as a string entry in the list box, and the object key is saved 
//    in the LOWORD of the 32-bit datum associated with that entry.
//
//    ARGS:       bInit -- [in] if TRUE, the current selection is reset to "NONE", or the first available child; else, 
//                         we try to preserve the currently selected object, if it still exists (default = FALSE). 
//
//    RETURNS:    NONE.
//
VOID CCxObjCombo::RefreshContents( BOOL bInit /* = FALSE */ )
{
   // if we can't get a reference to the MAESTRO experiment doc (should never happen), then reset the combo box.
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   if((m_wParentKey != CX_NULLOBJ_KEY) && (pDoc == NULL))
   {
      ASSERT(FALSE);
      ResetContent();
      m_wParentKey = CX_NULLOBJ_KEY;
      m_bAllowNone = FALSE;
      UpdateDropWidth();
      return;
   }

   // if we're not initializing the combo box contents, then try to preserve the current selection
   WORD wKeySel = CX_NULLOBJ_KEY; 
   if(!bInit)
   {
      int iSel = GetCurSel();
      if(iSel != CB_ERR)
      {
         wKeySel = LOWORD(GetItemData(iSel));
         if(!IsValidKey(wKeySel)) wKeySel = CX_NULLOBJ_KEY; 
      }
   }

   // reset combo box content and re-popuplate it from scratch...
   ResetContent();
   int iCurrSel = -1;

   if(m_bAllowNone) 
   {
      AddString( _T("NONE") ); 
      SetItemData( 0, MAKELONG(CX_NULLOBJ_KEY,0) );
      if(wKeySel == CX_NULLOBJ_KEY) iCurrSel = 0;
   }

   // assigned parent no longer exists. In this case, combo box remains empty (unless the "NONE" choice was added
   if((m_wParentKey!=CX_NULLOBJ_KEY) && !pDoc->ObjExists(m_wParentKey))
   { 
      m_wParentKey = CX_NULLOBJ_KEY;
      if(m_bAllowNone) iCurrSel = 0;
   }

   // assigned parent exists, so populate the combo box with its children. Each item in the box represents a child
   // object; the item string is the object's name, and its key is stored in the LOWORD of the item data.
   if(m_wParentKey != CX_NULLOBJ_KEY)
   { 
      if(m_bTrialsOnly && (pDoc->GetObjType(m_wParentKey)==CX_TRIALSET))
      {
         CWordArray wArKeys;
         pDoc->GetTrialKeysIn(m_wParentKey, wArKeys);
         for(int i=0; i<wArKeys.GetCount(); i++)
         {
            CString strItem;
            WORD wParent = pDoc->GetParentObj(wArKeys[i]);
            if(wParent != m_wParentKey)
               strItem.Format("%s : %s", pDoc->GetObjName(wParent), pDoc->GetObjName(wArKeys[i]));
            else 
               strItem = pDoc->GetObjName(wArKeys[i]);

            int idx = AddString(strItem);
            SetItemData(idx, MAKELONG(wArKeys[i], 0));
            if(wArKeys[i] == wKeySel) iCurrSel = idx;
         }
      }
      else
      {
         POSITION pos = pDoc->GetFirstChildObj(m_wParentKey);
         CTreeObj* pObj; 
         WORD wKey;
         while(pos != NULL)
         {
            pDoc->GetNextChildObj(pos, wKey, pObj);
            int i = AddString(pObj->Name());
            SetItemData(i, MAKELONG(wKey,0));
            if(wKey == wKeySel) iCurrSel = i;
         }
      }

      // if current selection no longer present in repopulated combo, select first item in list (unless empty)
      if((iCurrSel == -1) && (GetCount() > 0)) iCurrSel = 0;
   }

   UpdateDropWidth();

   // update the current selected item (-1 will clear the current selection)
   SetCurSel(iCurrSel);
}


//=== GetObjKey ======================================================================================================= 
//
//    Retrieve the MAESTRO object key associated with current selection of combo box, or CX_NULLOBJ_KEY if "NONE" is 
//    selected or if there is no selection (no entries in combo box).  RefreshContents() saves the key in the LOWORD of 
//    the 32-bit datum assoc. w/ each entry.
//
//    ARGS:       NONE. 
//
//    RETURNS:    key of selected CNTRLX object.
//
WORD CCxObjCombo::GetObjKey()
{
   WORD wKey = CX_NULLOBJ_KEY;
   int iSel = GetCurSel();
   if(iSel != CB_ERR)
   {
      wKey = LOWORD(GetItemData(iSel));
      if(!IsValidKey(wKey)) 
      {
         RefreshContents(TRUE);
         wKey = CX_NULLOBJ_KEY;
         if(GetCount() > 0)
         {
            wKey = LOWORD(GetItemData(0));
            ASSERT(IsValidKey(wKey));
         }
      }
   }
   return(wKey);
}


//=== SetObjKey ======================================================================================================= 
//
//    Set current selection of combo box to reflect the name of the child object with the specified key.  If the key is 
//    not valid, the operation will fail.  If the key is valid, but there is not a corresponding entry in the combo 
//    box, the contents of the combo box will be refreshed.  Check the return value to determine whether or not the 
//    change was successful.
//
//    ARGS:       wKey  -- [in] the MAESTRO obj key of the desired selection; if CX_NULLOBJ_KEY, selection is set to 
//                         the "NONE" entry -- if that entry exists.
//
//    RETURNS:    key of selected CNTRLX object (whether op succeeds or not). 
//
WORD CCxObjCombo::SetObjKey(const WORD wKey)
{
   // if key not valid, return the key for the currently selected object
   if(!IsValidKey(wKey)) return(GetObjKey());

   // search for valid key. If not found, contents must be stale. In this case, refresh contents and search once more.
   int idx = -1;
   for(int i=0; i<GetCount(); i++) 
   {
      if(wKey == LOWORD(GetItemData(i))) { idx = i; break; }
   }
   if(idx < 0) 
   { 
      RefreshContents();
      for(int i=0; i<GetCount(); i++) 
      {
         if(wKey == LOWORD(GetItemData(i))) { idx = i; break; }
      }
   }
   ASSERT(idx < GetCount());

   SetCurSel(idx);
   return(wKey);
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== IsValidKey ====================================================================================================== 
//
//    Returns TRUE if specified MAESTRO object key refers to an existing child of the currently assigned parent node 
//    defined in the MAESTRO document. A key value of CX_NULLOBJ_KEY is accepted only if the "NONE" entry is allowed.
//
//    When the parent is a trial set object and the combo box is configured to list both trials that are immediate 
//    children and those that are "grandchildren" (children of trial subsets under the set object), then the method
//    returns TRUE so long as the object exists and is a descendant of the assigned parent node.

//    ARGS:       wKey  -- [in] MAESTRO object key to be checked. 
//
//    RETURNS:    TRUE if key valid; FALSE otherwise.
//
BOOL CCxObjCombo::IsValidKey( const WORD wKey )
{
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc(); 
   if( wKey == CX_NULLOBJ_KEY )                                                     // valid only if "NONE" allowed
      return( m_bAllowNone );
   else if( m_wParentKey == CX_NULLOBJ_KEY )                                        // no parent node!
      return( FALSE );
   else if( pDoc == NULL )                                                          // no doc avail when one is 
   {                                                                                // required!
      ASSERT( FALSE );
      return( FALSE );
   }
   else if(m_bTrialsOnly && (pDoc->GetObjType(m_wParentKey)==CX_TRIALSET))
   {
      // special case: Object must be a trial and it can be a descendant of the assigned parent
      return(BOOL(pDoc->GetObjType(wKey)==CX_TRIAL && pDoc->IsAncestorObj(m_wParentKey, wKey)));
   }
   else
      return( BOOL(m_wParentKey == pDoc->GetParentObj( wKey )) );
}

/**
 Helper method called by RefreshContents() after repopulating the combo box's dropdown list. It computes the length of 
 the longest string in the list, which determines the minimum width of the dropdown list.
*/
VOID CCxObjCombo::UpdateDropWidth()
{
   // special case: drop list is empty
   if(GetCount() == 0)
   {
      SetDroppedWidth(0);
      return;
   }

   int iMaxLen = 0;

   CClientDC dc(this);
   HFONT hFont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
   HFONT saveFont = (HFONT) dc.SelectObject(hFont);
   
   CSize sz;
   for(int i=0; i<GetCount(); i++)
   {
      CString strItem;
      GetLBText(i, strItem);
      sz = dc.GetTextExtent(strItem);
      if(sz.cx > iMaxLen) iMaxLen = sz.cx;
   }
   
   sz = dc.GetTextExtent("M");
   iMaxLen += sz.cx;

   dc.SelectObject(saveFont);

   SetDroppedWidth(iMaxLen + ::GetSystemMetrics(SM_CXVSCROLL));
}

