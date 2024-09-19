//===================================================================================================================== 
//
// inplacetextedit.h : Declaration of class CInPlaceTextEdit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(INPLACETEXTEDIT_H__INCLUDED_)
#define INPLACETEXTEDIT_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000



class CInPlaceTextEdit : public CEdit
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   CString  m_strInitial;              // initial text string in the control when an inplace edit begins
   UINT     m_nExitChar;               // set to char key which extinguished inplace edit, 0 otherwise
   CRect    m_Rect;                    // current window rect -- to resize for more chars (see OnChar())
   BOOL     m_bRestoreParent;          // TRUE to restore parent to its original owner/parent when control is hidden 
   BOOL     m_bAlreadyEnding;          // gate flag prevents reentrancy in EndEdit()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CInPlaceTextEdit( CWnd* pOwner, DWORD dwStyle, UINT nID );
   ~CInPlaceTextEdit();


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnKillFocus( CWnd* pNewWnd );
   afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
   afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
   afx_msg UINT OnGetDlgCode() 
   { return( DLGC_WANTALLKEYS ); }

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL BeginEdit( CWnd* pParent, const CRect& rect,     // initiate an inplace edit op by showing & activating ctrl 
            LPCTSTR strInitial, UINT nFirstChar = 0 );
   void CancelEdit();                                    // cancel the inplace edit and hide ctrl 
   UINT GetExitChar() { return( m_nExitChar ); }         // retrieve char key that may have extinguished the ctrl 
   BOOL IsChanged()                                      // has user changed the text displayed in the inplace ctrl?
   {
      CString strNow;
      GetWindowText( strNow );
      return( BOOL(m_strInitial != strNow) );
   }
   BOOL PreTranslateMessage( MSG* pMsg );                // technical bug workaround 


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   void EndEdit();                                       // end the inplace edit op and notify owner 

};
 

#endif // !defined(INPLACETEXTEDIT_H__INCLUDED_)
