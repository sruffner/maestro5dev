//===================================================================================================================== 
//
// cxobjcombo.h : Declaration of class CCxObjCombo
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXOBJCOMBO_H__INCLUDED_)
#define CXOBJCOMBO_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CCxObjCombo : public CComboBox
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   WORD m_wParentKey;                                    // key of "parent" obj whose kids are listed in combo box
   BOOL m_bAllowNone;                                    // if TRUE, the "NONE" (CX_NULLOBJ_KEY) selection is included

   // special configuration: if set and the parent object is a trial set (CX_TRIALSET), then the combo box lists all
   // trials in the set, even those that are children of trial subsets under the specified set. In this configuration
   // the trial subsets are omitted from the combo box, and the item string includes both the subset name and the
   // trial name.
   BOOL m_bTrialsOnly; 

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CCxObjCombo();                                        // constructor 
   ~CCxObjCombo() {}                                     // destructor 

   BOOL Create( DWORD dwStyle, const RECT& rect,         // create combo box HWND -- restrict to "drop list" 
                CWnd* pParentWnd, UINT nID ); 
   BOOL ModifyStyle( DWORD dwRemove, DWORD dwAdd,        // to enforce "drop list" style.
                     UINT nFlags = 0 );
   BOOL SubclassDlgItem( UINT nID, CWnd* pParent );      // enfore "drop list" style and reset contents


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   // assign combo box to a parent node in MAESTRO object tree
   BOOL InitContents( WORD wParent, BOOL bAllowNone, BOOL bTrialsOnly = FALSE);

   VOID RefreshContents( BOOL bInit = FALSE );           // reload entire contents of combo box, preserving current 
                                                         // selection if possible -- unless bInit is TRUE
   BOOL IsEmpty() { return( BOOL(GetCount() > 0) ); }    // does currently assigned parent node have any children?
   WORD GetObjKey();                                     // retrieve object key corresponding to current selection
   WORD SetObjKey( const WORD wKey );                    // set combo box selection via object key
   WORD GetParentKey() { return( m_wParentKey ); }       // retrieve key of parent node currently assigned to combo box 


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   BOOL IsValidKey( const WORD wKey );                   // does specified key point to an existing child of currently 
                                                         // assigned parent node, or "NONE"?

   // set minimum width of drop down list to accommodate longest string
   VOID UpdateDropWidth();
};



#endif // !defined(CXOBJCOMBO_H__INCLUDED_)
