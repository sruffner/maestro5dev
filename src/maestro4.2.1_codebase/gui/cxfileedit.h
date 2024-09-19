//===================================================================================================================== 
//
// cxfileedit.h : Declaration of class CCxFileEdit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXFILEEDIT_H__INCLUDED_)
#define CXFILEEDIT_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "fileeditctrl\fileeditctrl.h"    // base class CFileEditCtrl


//===================================================================================================================== 
// Declaration of class CCxFileEdit
//===================================================================================================================== 
//
class CCxFileEdit : public CFileEditCtrl
{
   DECLARE_DYNCREATE( CCxFileEdit )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
public:
   typedef enum FType                            // types of Maestro files handled by this control:
   {
      DATAFILE = 0,                             //    a trial- or cont-mode data file
      LOGFILE                                   //    a message log file
   } FType;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CString m_strPath;                                    // last valid pathname entered into edit ctrl
   FType m_type;                                         // type of CNTRLX file displayed -- sets naming constraints.


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CCxFileEdit()
   { 
      m_strPath = _T(""); 
      m_type = DATAFILE; 
   }
                                                         // use default copy const, op=, destructor


//===================================================================================================================== 
// MESSAGE HANDLERS 
//===================================================================================================================== 
protected:
   afx_msg void OnKillFocus( CWnd* pNewWnd );            // revalidate path when control loses focus
   afx_msg BOOL OnPostBrowse( NMHDR* pNMH,               // autocorrect path after user changes it via browse dialog
                              LRESULT* pRes );
   afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt,     // revalidate path when "Enter" key is presed
                           UINT nFlags );
   afx_msg UINT OnGetDlgCode()                           // so parent dialog doesn't eat "Enter" keypress in this ctrl 
   { return( DLGC_WANTALLKEYS ); }

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   VOID SetFileType( FType ft );                         // set type of CNTRLX file reflected in the edit ctrl
   const CString& GetCurrentPath();                      // return pathname currently reflected in the edit ctrl
   VOID GetCurrentDirectory( CString& strDir );          // return directory currently reflected in the edit ctrl
   const CString& IncrementFileExt();                    // incr numeric ext of pathname in ctrl (DATAFILE type only)
   VOID InitializePath(LPCTSTR strDir, LPCTSTR strBase); // initialize pathname in the edit ctrl

   BOOL SetFlags( DWORD dwFlags );                       // set functionality specific to file edit ctrl

   virtual BOOL PreCreateWindow( CREATESTRUCT& cs );     // overridden to enforce required edit control styles


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   VOID ValidateCurrentPath( BOOL bReqd = FALSE );       // ensure pathname satisfies current naming restrictions

};


#endif   // !defined(CXFILEEDIT_H__INCLUDED_)
