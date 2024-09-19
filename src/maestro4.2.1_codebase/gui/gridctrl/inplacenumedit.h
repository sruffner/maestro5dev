//===================================================================================================================== 
//
// inplacenumedit.h : Declaration of class CInPlaceNumEdit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(INPLACENUMEDIT_H__INCLUDED_)
#define INPLACENUMEDIT_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "numedit.h"                   // CNumEdit -- a numeric-only version of CEdit w/format constraints


class CInPlaceNumEdit : public CNumEdit
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   double   m_dInitial;                // initial value in the control when an inplace edit begins
   UINT     m_nExitChar;               // set to char key which extinguished inplace edit, 0 otherwise
   CRect    m_Rect;                    // current window rect -- to resize for more chars (see OnChar())
   BOOL     m_bRestoreParent;          // TRUE to restore parent to its original owner/parent when control is hidden 
   BOOL     m_bAlreadyEnding;          // gate flag prevents reentrancy in EndEdit()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CInPlaceNumEdit( CWnd* pOwner, DWORD dwStyle, UINT nID );
   ~CInPlaceNumEdit();


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
      BOOL bInt, BOOL bNonneg, UINT nLen, UINT nPre, 
      double dInitial, UINT nFirstChar = 0 );
   BOOL BeginEdit( CWnd* pParent, const CRect& rect,     // overloaded version using format struct
      const NUMEDITFMT& fmt, double dInitial, 
      UINT nFirstChar = 0 )
   {
      return( BeginEdit( pParent, rect, BOOL(fmt.flags & NES_INTONLY), BOOL(fmt.flags & NES_NONNEG), 
                         fmt.nLen, fmt.nPre, dInitial, nFirstChar ) );
   }
   void CancelEdit();                                    // cancel the inplace edit and hide ctrl 
   UINT GetExitChar() { return( m_nExitChar ); }         // retrieve char key that may have extinguished the ctrl 
   BOOL IsChanged()                                      // has user changed the value displayed in the inplace ctrl?
   {
      return( BOOL(m_dInitial != AsDouble()) );
   }
   BOOL PreTranslateMessage( MSG* pMsg );                // technical bug workaround 


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   void EndEdit();                                       // end the inplace edit op and notify owner 

};
 

#endif // !defined(INPLACENUMEDIT_H__INCLUDED_)
