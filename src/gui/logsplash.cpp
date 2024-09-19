//===================================================================================================================== 
//
// logsplash.cpp : Implementation of class CLogSplash, an MFC GUI thread that controls a message-logging splash screen. 
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// For some applications, "startup" time may be significantly protracted by various operations that must be completed 
// before user interactions begin:  database initializations, opening files, starting drivers, etc.  In such cases, 
// an application may throw up a "splash screen" as a clue to the user that it is coming up.  Typically, this is a 
// nice-looking bitmap with some static information.  MS DevStudio provides a simple MFC implementation of a splash 
// screen as part of its Component Gallery.  However, this implementation has some drawbacks, as described in Paul 
// DiLascia's article (CREDITS).  As an alternative, he offers CSplash, a splash screen class that runs in its own 
// thread and can handle device-independent bitmaps (DIBs).  However, CSplash cannot display application info messages 
// that keep the user up-to-date on what is happening while s/he stares at the splash screen.
//
// CLogSplash, with its accompanying private window class CLogSplashWnd, is an enhancement upon CSplash that allows 
// other threads (that have access to the CLogSplash thread) to post messages to a simple read-only edit window that 
// sits on top of a portion of the bitmap that gets installed as the splash window.
//
// ==> Usage.
// To open the splash screen, simply construct an instance of CLogSplash, passing the required information to the 
// constructor (resource ID of the splash bitmap, the splash screen's "lifetime", and some CSplash-inherited flags that 
// govern its behavior).  The constructor creates the splash screen thread, at which point CLogSplash takes care of all 
// the details of loading and displaying the splash screen.  See CSplash for more info on the constructor flags.
//
// You will probably want to control the placement and size of the log window, depending on the layout of the splash 
// bitmap.  A CRect argument to the CLogSplash constructor serves this very purpose.  The units for the CRect are 
// "parts per 100" of the bitmap's width (for left & right coords) and height (for top & bottom coords).  Thus, all 
// coords should lie in the range [0..100], with left<right and top<bottom.  For example, to specify a log window 
// that's half as long and half as wide as the bitmap and occupying the bottom left corner, use CRect(0,50,50,100).
// If the rectangle spec is invalid in any way, the log window will be sized to occupy the lower third of the splash 
// screen.
//
// Typically, CLogSplash is created in the application's CWinApp::InitInstance() method.  Once created, the app's  
// primary thread (in InitInstance()) will continue with the time-consuming work of starting up the application, while 
// the CLogSplash thread -- running in tandem -- loads and displays the message-logging splash screen.  The primary 
// thread can then log messages to the splash screen by calling the splash thread's Log() method.  When it's done with 
// the time-consuming startup work, the primary thread can then terminate the splash screen by calling the splash 
// thread's Kill() method, or it can let the splash thread extinquish itself when its lifetime is over.
// 
// ==> Multithreading issues.
// It is important to remember that CLogSplash represents a separate thread of execution; of necessity, it is an MFC 
// "GUI thread" with its own Windows message pump for proper handling of windows and (potentially) user input. 
// Internally, CLogSplash uses synchronization objects to avoid thread conflicts.  The consequence:  if Log() is 
// called a second time to post another message before the previous message was actually logged by CLogSplash, it will 
// BLOCK until the previous message is handled.  An optional timeout argument to Log() can be specified so that the 
// method does not block for too long.  Of course, if Log() times out, the message will not be posted!
//
// One of the parameters to the CLogSplash constructor is a "back pointer", the address of the thread pointer by which 
// the primary thread can reference the CLogSplash thread object that it creates.  When CLogSplash dies, it uses this 
// double-indirection pointer to set the reference to NULL.  Thus, it is important to check the thread pointer against 
// NULL before calling CLogSplash methods, in case the splash screen thread has already died. 
//
// DEV NOTE:  STILL, there is a very slight risk of getting a NULL pointer exception.  Given the code "if( pSplash ) 
// pSplash->Log( ... )" in the primary thread, suppose the primary thread is interrupted after the conditional test, 
// and in the next time slice the splash thread dies and sets "pSplash" to NULL.  Uh-oh.  Another concern:  what if 
// the splash thread dies while the primary thread is blocked in Log()??
//
//
// CREDITS:
// (1) Article/code by Paul DiLascia ("C++ Q&A", Microsoft Systems Journal, Oct 1999).  Provides a freeware, reusable 
// class for displaying a splash window in its own GUI thread -- CSplash.  CSplash implementation includes a "private" 
// window class CSplashWnd, and makes use of two other freeware classes by P. DiLascia:  CDib, for reading device 
// independent bitmaps from file or resource; and CCommandLineInfoEx, an extension of MFC's CCommandLineInfo that can 
// parse command options from the application command line.
//
// REVISION HISTORY:
// 04apr2001-- Completed initial version.
// 05apr2001-- Added timeout feature to CLogSplash::Log() -- so calling thread need not be blocked forever if there's 
//             a problem w/ the splash screen thread.
//          -- Added a CRect arg to the CLogSplash constructor to control the size and location of the log window rect 
//             on the splash window. 
// 13apr2001-- Modified to support optional timestamping of a displayed message string.
// 09apr2003-- Running on WinXP, we were getting occasional ASSERTs upon attempting to log a message in the splash 
//             window's CLogEdit window.  The ASSERT was on CLogEdit's HWND -- ie, ::IsWindow( HWND hwnd ))==FALSE in 
//             CWnd::GetWindowTextLength() <-- CLogEdit::LogMessage() <-- CLogSplashWnd::Log() <-- CLogSplash::OnIdle() 
//             <-- CLogSplash::Run().  Also, the display showed the splash window WITHOUT the CLogEdit.  This is 
//             strange, since CLogSplash will not enter the Run() until after InitInstance() succeeds, and that method 
//             should fail if we failed to create the CLogEdit HWND!!  Since the logging splash screen is not very 
//             important, I decided to check for a valid CLogEdit HWND in CLogSplashWnd::Log().  If the HWND proves to 
//             be invalid, then we don't attempt to log the message.
// 27oct2003-- Finally fixed bug alluded to in 09apr2003 entry.  The problem was that we allowed logging to begin 
//             *possibly* before the splash window was created.  Now, we prevent logging (by owning the CEvent synch 
//             object that guards access to the logging function) until after the splash window has been successfully 
//             created.
// 04nov2003-- NO, the 27oct2003 fix did not entirely fix problem.  Log window still did not show up on occasion.
//             Added a call in CLogSplashWnd::Create() to explicitly make the log window visible after it was created.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "logsplash.h" 


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// class CLogSplash -- GUI thread for a message-logging splash screen 
//===================================================================================================================== 
//
IMPLEMENT_DYNAMIC(CLogSplash, CSplash)

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CLogSplash [constructor] ======================================================================================== 
//
//    Construct the splash screen thread.  Thread creation occurs in the base class constructor.  Here we create a 
//    CEvent object that will be used to arbitrate our message-logging scheme. 
//
//    Any combination of the following flags may be specified:
//       CSplash::KillOnClick          ==>   any key/mouse dismisses splash
//       CSplash::IgnoreCmdLine        ==>   if not specified, splash thread will abort if command line has -nologo 
//       CSplash::NoWaitForMainWnd     ==>   expire even if app's main window has not been created yet 
//
//    The "back pointer" is a double-indirection reference that points to a pointer which, in turn, points to this 
//    thread object after creation.  The base class uses this "back pointer" to set the thread pointer to NULL when the 
//    splash thread expires.
//
//    ARGS:       nIDRes   -- [in] resource ID of the splash bitmap. 
//                rLogWnd  -- [in] desired loc of log window on parent splash window, in parent coords (parts per 100). 
//                dur      -- [in] minimum lifetime of splash window in msec (-1 == infinite lifetime).
//                flags    -- [in] creation flags inherited from base class (default = 0).
//                ppBackPtr-- [in] ptr to calling thread's ptr to this thread object (default = NULL ).
//
CLogSplash::CLogSplash( UINT nIDRes, const CRect& rLogWnd, UINT dur, WORD flags /* =0 */, 
                        CLogSplash** ppBackPtr /* =NULL */ )
   : CSplash( nIDRes, dur, flags, (CSplash**) ppBackPtr ) 
{
   m_pMsgPosted = new CEvent( TRUE, TRUE );                    // we own event initially b/c we're not ready to accept 
                                                               // msgs to log

   m_rLogWnd = rLogWnd;                                        // validate log window rect specification:
   if( (m_rLogWnd.left < 0) || (m_rLogWnd.left > 100) ||       //    all coords must be in "parts per 100".  we convert 
       (m_rLogWnd.right < 0) || (m_rLogWnd.right > 100) ||     //    to real coords by: x'=x*W/100, y'=y*H/100!!
       (m_rLogWnd.top < 0) || (m_rLogWnd.top > 100) ||
       (m_rLogWnd.bottom < 0) || (m_rLogWnd.bottom > 100) ||
       (m_rLogWnd.left >= m_rLogWnd.right) ||                  //    left side MUST be to the left of right side!
       (m_rLogWnd.top >= m_rLogWnd.bottom)                     //    top MUST be above the bottom!
     )
   {
      m_rLogWnd.left = 1; m_rLogWnd.right = 99;                // if invalid, force log window to occupy the bottom 
      m_rLogWnd.top = 66; m_rLogWnd.bottom = 99;               // one-third of the splash window 
   }
}


//=== ~CLogSplash [destructor] ======================================================================================== 
//
//    Here we make sure our event object is signalled, releasing any threads waiting on it.  We then destroy it.
//
CLogSplash::~CLogSplash() 
{ 
   if( m_pMsgPosted != NULL )
   {
      m_pMsgPosted->SetEvent(); 
      delete m_pMsgPosted;
      m_pMsgPosted = NULL;
   }
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== OnCreateSplashWnd [base override] =============================================================================== 
//
//    Create the splash window, which serves as the main window for the splash screen thread. 
//
//    ARGS:       nIDRes   -- [in] resource ID of the splash bitmap. 
//                dur      -- [in] minimum lifetime of splash window in msec (-1 == infinite lifetime).
//                flags    -- [in] creation flags inherited from base class
//
//    RETURNS:    a ptr to the created window if successful, NULL otherwise.
//
CWnd* CLogSplash::OnCreateSplashWnd( UINT nIDRes, UINT dur, WORD flags )
{
   CLogSplashWnd *pSplashWnd = new CLogSplashWnd;
   if( pSplashWnd )
   {
      if( !pSplashWnd->Create( nIDRes, dur, flags, m_rLogWnd ) )        // note add'l param:  log wnd rect
      {
         delete pSplashWnd;
         pSplashWnd = NULL;
      }
   }

   if( pSplashWnd != NULL )                                             // now that splash window created we can start 
       m_pMsgPosted->SetEvent();                                        // logging messages!

   return( (CWnd*)pSplashWnd );
}


//=== Log ============================================================================================================= 
//
//    Log a message to the splash screen. 
//
//    ARGS:       szMsg       -- [in] the message to be posted. 
//                bTime       -- [in] if TRUE, prepend the current date/time to message (default = FALSE). 
//                dwTimeOut   -- [in] timeout period in msec (default = INFINITE). 
//
//    RETURNS:    TRUE if successful, FALSE otherwise (timed out before msg could be posted). 
//
BOOL CLogSplash::Log( LPCTSTR szMsg, BOOL bTime /* =FALSE */, DWORD dwTimeOut /* =INFINITE */ )
{
   if( m_pMsgPosted == NULL ) return( FALSE );           // the CEvent object is gone; thread may have died 
   CSingleLock waitForMsgPost( m_pMsgPosted, FALSE );    // block until previous msg posted in OnIdle(), or timeout
   if( waitForMsgPost.Lock( dwTimeOut ) )                // ready for next msg!  post it...
   {
      m_strMessage = szMsg;
      m_bTimeStamp = bTime;
      if( m_pMsgPosted )
         m_pMsgPosted->ResetEvent();                     // reset event obj so no more msgs can be posted
      return( TRUE );
   } 
   else
      return( FALSE );                                   // timed out w/o posting new msg!
}


//=== OnIdle [base override] ========================================================================================== 
//
//    Background processing for the splash screen thread.
//
//    An event object is used to signal the presence of a new string to be posted to the splash screen's message log. 
//    When the event is "nonsignaled" (reset), a new string is ready for posting.  After adding it to the message log, 
//    we set the event object to signaled, which releases a different thread to post another string in Log().
//
//    NOTE:  We NEVER yield idle mode!  If we did, we won't return to idle mode until the thread pumps a message -- 
//    which will delay our responding to new msg posts -- causing another thread to get stuck!  See CWinThread::Run(). 
//
//    ARGS:       lCount   -- [in] the current idle count. 
//
//    RETURNS:    TRUE if more idle processing is required; FALSE to yield the idle time. 
//
BOOL CLogSplash::OnIdle( LONG lCount )
{
   CSplash::OnIdle( lCount );                               // always let base class do its idle stuff first!
   if( lCount > 2 )
   {
      CSingleLock checkForNewMsg( m_pMsgPosted, FALSE );    // if there's a message to be posted, do it now:
      if( (!checkForNewMsg.Lock(0))                         //    (CSingleLock::IsLocked() is NOT appropriate here!)
          && m_pMainWnd )                                   //    (msg could be posted BEFORE we've finished creating 
      {                                                     //       the main window, so we check its handle!)
         ((CLogSplashWnd*)m_pMainWnd)->Log( m_strMessage, 
                                            m_bTimeStamp );
         m_pMsgPosted->SetEvent();
      }
   }
   return( TRUE );                                          // never yield IDLE mode
}






//===================================================================================================================== 
// class CLogSplashWnd -- designed for private use of CLogSplash  
//===================================================================================================================== 
//
IMPLEMENT_DYNAMIC(CLogSplashWnd, CSplashWnd)
BEGIN_MESSAGE_MAP(CLogSplashWnd, CSplashWnd)
END_MESSAGE_MAP()


//=== Create [base override] ========================================================================================== 
//
//    Create the message-logging splash window.  Base class handles creating the splash window and loading the splash 
//    bitmap.  Here we add a message logger, a read-only edit control that's positioned over a specified rectangle w/in 
//    the splash window. 
//
//    ARGS:       nIDRes   -- [in] resource ID of the splash bitmap. 
//                dur      -- [in] minimum lifetime of splash window in msec (-1 == infinite lifetime).
//                flags    -- [in] creation flags inherited from base class
//                rLogWnd  -- [in] desired loc of log window on parent splash window, in parent coords (parts per 100). 
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL CLogSplashWnd::Create( UINT nIDRes, UINT duration, WORD flags, const CRect& rLogWnd )
{
   if( !CSplashWnd::Create( nIDRes, duration, flags ) )              // base class creates splash window and loads the 
      return( FALSE );                                               // specified bitmap 

   CSize sz = m_dib.GetSize();                                       // calculate window rect for the message log
   CRect rLog;                                                       // in bitmap coords
   rLog.left   = (rLogWnd.left * sz.cx) / 100; 
   rLog.right  = (rLogWnd.right * sz.cx) / 100;
   rLog.top    = (rLogWnd.top * sz.cy) / 100;
   rLog.bottom = (rLogWnd.bottom * sz.cy) / 100;
   if( rLog.left < 0 || rLog.left >= rLog.right )                    // corrections in case invalid...
      rLog.left = 1;
   if( rLog.right <= rLog.left || rLog.right > sz.cx - 1 )
      rLog.right = sz.cx - 1;
   if( rLog.top < 0 || rLog.top >= rLog.bottom ) 
      rLog.top = 1;
   if( rLog.bottom <= rLog.top || rLog.bottom > sz.cy - 1 )
      rLog.bottom = sz.cy - 1;
   
   if( !m_log.CreateEx( WS_EX_CLIENTEDGE, _T("EDIT"), NULL,          // create the log window; if unsuccessful, we 
         WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOHSCROLL,        // must destroy the splash window created above... 
         rLog, (CWnd*)this, 100 ) )
   {
      DestroyWindow();
      return( FALSE );
   }

   m_log.ShowWindow( SW_SHOW );                                      // added this b/c SOMETIMES the log window did 
                                                                     // not appear!

   BOOL bGotFont = m_font.CreateStockObject( DEFAULT_GUI_FONT );     // use default GUI font in message log, if we can 
   if( !bGotFont ) 
      bGotFont = m_font.CreatePointFont( 80, "MS Sans Serif" );
   if( bGotFont ) 
      m_log.SetFont( &m_font );

   return( TRUE );
}


//=== Log ============================================================================================================= 
//
//    Log a message string in the child edit window within the splash screen, including an optional timestamp.
//
//    ARGS:       szMsg -- [in] the message to be logged.
//                bTime -- [in] if TRUE, the current time is prepended before message string (default=FALSE).
//
//    RETURNS:    NONE.
//
VOID CLogSplashWnd::Log( LPCTSTR szMsg, BOOL bTime /* =FALSE */ ) 
{ 
   if( !::IsWindow( m_log.GetSafeHwnd() ) ) return;                  // just in case the edit window isn't there!
   if( !bTime ) m_log.LogMessage( szMsg ); 
   else m_log.LogTimeStampedMsg( szMsg );
}
