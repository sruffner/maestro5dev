//===================================================================================================================== 
//
// cxmsglogbar.h : Declaration of classes CCxMsgLogDlg and CCxMsgLogBar.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXMSGLOGBAR_H__INCLUDED_)
#define CXMSGLOGBAR_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizebar\szdlgbar.h"             // base classes CSzDlgBarDlg and CSizingDialogBar
#include "logedit.h"                      // CLogEdit -- for readonly message log in CCxMsgLogDlg
#include "cxfileedit.h"                   // CCxFileEdit -- special edit ctrl for specifying path for CNTRLX files 



//===================================================================================================================== 
// Declaration of class CCxMsgLogDlg 
//===================================================================================================================== 
//
class CCxMsgLogDlg : public CSzDlgBarDlg
{
   DECLARE_DYNCREATE( CCxMsgLogDlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_MSGLOG;        // dialog template resource ID for this dialog
   static const int LOGBUFSIZE;              // we flush message buffer to log file once it exceeds this size


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CString           m_strPending;           // buffer of messages that have been displayed but not logged to file
   CStdioFile*       m_pFile;                // the open log file for CNTRLX messages; NULL if logging is OFF

   CCxFileEdit       m_fecLogPath;           // special edit ctrl displays/selects path for message log file
   CButton           m_btnSave;              // check box; if checked, messages are saved to the specified log file
   CLogEdit          m_editLog;              // the readonly message log

   CSize             m_sizeEditLog;          // initial & minimum size of the msg log edit ctrl (as defined in templ)
   CSize             m_sizeEditPath;         // similarly for the log path edit ctrl


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxMsgLogDlg(const CCxMsgLogDlg& src);                // no copy constructor or assignment operator defined
   CCxMsgLogDlg& operator=( const CCxMsgLogDlg& src ); 

public: 
   CCxMsgLogDlg();
   ~CCxMsgLogDlg();


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnDestroy();                             // save MRU log file dir to registry prior to destruction
   afx_msg void OnPreBrowse(NMHDR *pNMH, LRESULT *pRes); // to prevent log file browsing under certain circumstances
   afx_msg void OnBtnClicked();                          // respond to btn click on PB control
   afx_msg void OnSize( UINT nType, int cx, int cy );    // resize selected controls when dialog is resized

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   VOID ClearLog();                                      // clear the message log
   VOID LogMessage( LPCTSTR pszMsg, BOOL bTimestamp );   // add a string to the message log, w/ optional timestamp 

//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   BOOL FlushLog();                                      // flush pending msg buffer to the log file
   VOID StartLogging();                                  // open log file & start logging app msgs to it
   VOID StopLogging();                                   // flush msg buffer to the log file and close it
};





//===================================================================================================================== 
// Declaration of class CCxMsgLogBar
//===================================================================================================================== 
//
class CCxMsgLogBar : public CSizingDialogBar
{
private:
   CCxMsgLogDlg   m_logDlg; 

public:
   CCxMsgLogBar() : CSizingDialogBar( m_logDlg ) {}
   ~CCxMsgLogBar() { m_logDlg.DestroyWindow(); CSizingDialogBar::DestroyWindow(); }

   VOID ClearLog() { m_logDlg.ClearLog(); }
   VOID LogMessage( LPCTSTR pszMsg, BOOL bTimestamp ) { m_logDlg.LogMessage( pszMsg, bTimestamp ); }

};


#endif // !defined(CXMSGLOGBAR_H__INCLUDED_)
