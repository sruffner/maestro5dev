//===================================================================================================================== 
//
// logsplash.h : Declaration of class CLogSplash and helper class CLogSplashWnd.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(LOGSPLASH_H__INCLUDED_)
#define LOGSPLASH_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxmt.h>                     // CEvent and other synch objects for multithreading 

#include "splash.h"                    // base class CSplash
#include "logedit.h"                   // CLogEdit -- for the message log window 



//===================================================================================================================== 
// Declaration of class CLogSplash
//===================================================================================================================== 
//
class CLogSplash : public CSplash 
{
	DECLARE_DYNAMIC(CLogSplash)

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   CRect    m_rLogWnd;        // location of log window on parent splash wnd, in parent's coords (parts per 100) 
   CString  m_strMessage;     // next message to post on splash window (multi-threaded access!)
   BOOL     m_bTimeStamp;     // TRUE if next message should be prepended with time/date stamp
   CEvent*  m_pMsgPosted;     // event obj to control access to the message string


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CLogSplash( const CLogSplash& src );                  // no copy constructor defined
   CLogSplash& operator=( const CLogSplash& src );       // no assignment operator defined 

public:
	CLogSplash( UINT nIDRes, const CRect& rLogWnd,        // constructor -- splash thread created/started here 
	            UINT dur, WORD flags=0, 
	            CLogSplash** ppBackPtr=NULL ); 
   ~CLogSplash();


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   CWnd* OnCreateSplashWnd( UINT nIDRes, UINT dur,       // create splash thread's main window, the "splash" window
                            WORD flags );
   BOOL Log( LPCTSTR szMsg, BOOL bTime=FALSE,            // log a message to the splash window (BLOCKING)
             DWORD dwTimeOut=INFINITE );

protected:
   BOOL OnIdle( LONG lCount );                           // bkgnd processing; detect & post new log message. 

};




//===================================================================================================================== 
// Declaration of helper class CLogSplashWnd -- intended only for use as the main window for the CLogSplash GUI thread 
//===================================================================================================================== 
//
class CLogSplashWnd : public CSplashWnd
{
   DECLARE_DYNAMIC(CLogSplashWnd)

   friend CLogSplash;                                    // only CLogSplash is supposed to use this class!

protected:
   CLogEdit m_log;                                       // the read-only edit log for displaying msgs
   CFont    m_font;                                      // font for the log

   CLogSplashWnd() {}
   ~CLogSplashWnd() {}
   BOOL Create( UINT nIDRes, UINT dur, WORD flags,       // create splash window, w/ msg log as child 
                const CRect& rLogWnd );
   VOID Log( LPCTSTR szMsg, BOOL bTime=FALSE );          // log a message, with optional timestamp, to splash screen 

   DECLARE_MESSAGE_MAP()
};


#endif   // !defined(LOGSPLASH_H__INCLUDED_)
