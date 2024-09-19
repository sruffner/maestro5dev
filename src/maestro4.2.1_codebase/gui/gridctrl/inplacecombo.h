//===================================================================================================================== 
//
// inplacecombo.h : Declaration of class CInPlaceCombo.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(INPLACECOMBO_H__INCLUDED_)
#define INPLACECOMBO_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CInPlaceCombo : public CComboBox
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int     MAX_LINES_IN_DROPLIST;        // max # visible items in combo's droplist box


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   UINT  m_nExitChar;               // set to char key which extinguished inplace operation, 0 otherwise
   CRect m_rect;                    // combo box rect, including list box, in client coords of current parent
   int   m_initialSel;              // zero-based index of list item initially selected when inplace combo shown 
   BOOL  m_bRestoreParent;          // TRUE to restore parent to its original owner/parent when control is hidden 
   BOOL  m_bAlreadyEnding;          // gate flag prevents reentrancy in EndEdit()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CInPlaceCombo( CWnd* pOwner, DWORD dwStyle, UINT nID );
   ~CInPlaceCombo();


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnKillFocus( CWnd* pNewWnd );            // extinguish inplace ctrl when it loses focus
   afx_msg void OnChar( UINT nChar, UINT nRepCnt,        // catches char keys which should terminate inplace ctrl
                        UINT nFlags );
   afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt,     // terminate on arrow keys
                           UINT nFlags );
   afx_msg void OnCloseUp();                             // override certain default behaviors when droplist closes
   afx_msg UINT OnGetDlgCode() 
   { return( DLGC_WANTALLKEYS ); }

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL BeginEdit( CWnd* pParent, const CRect& rect,     // initiate an inplace op by showing & activating ctrl 
      BOOL bNoResize, const CStringArray& values, int iSel );
   void CancelEdit();                                    // cancel the inplace op and hide ctrl 
   UINT GetExitChar() { return( m_nExitChar ); }         // retrieve char key that may have extinguished the ctrl 
   int GetCurrentSelection();                            // retrieve user's current choice
   BOOL IsChanged();                                     // is the current selection different from the initial one?
   BOOL PreTranslateMessage( MSG* pMsg );                // technical bug workaround 
   BOOL ModifyStyle( DWORD dwRemove, DWORD dwAdd,        // override to enforce certain combo box style restrictions 
                     UINT nFlags );


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   void EndEdit();                                       // end the inplace operation and notify owner 

};
 

#endif // !defined(INPLACECOMBO_H__INCLUDED_)
