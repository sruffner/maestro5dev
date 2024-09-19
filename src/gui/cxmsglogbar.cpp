//===================================================================================================================== 
//
// cxmsglogbar.cpp : Implementation of the CNTRLX "Message Log" dialog CCxMsgLogDlg and its control bar container, 
//                   CCxMsgLogBar.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxMsgLogDlg is the ultimate destination for all CNTRLX error/warning/status messages.  In addition to displaying 
// each message in a read-only edit control, CCxMsgLogDlg provides the option of saving these messages to a log file. 
// The dialog is defined in a dialog template resource (IDD_MSGLOG), and it contains the following controls:
//
//    IDC_MSGLOG_LOG [edit]: An edit control in which all application messages are displayed.  It fills most of the 
//       dialog's client area, and is resized so that it's right and bottom edges "stick" to the dialog's respective 
//       edges whenever the dialog is resized BEYOND its template size.  Subclassed to CLogEdit, which turns edit 
//       control into a read-only, multiline message log.
//    IDC_MSGLOG_SAVE [chkbox]: Whenever this button is checked, all application messages displayed in the message log 
//       window are also saved to a log file, the path of which is specified in the next control.
//    IDC_MSGLOG_PATH [edit]: A specialized edit control in which the user specifies the pathname for a log file to 
//       which all application messages are streamed when enabled.  Subclassed to CCxFileEdit, which allows the user to 
//       browse the file system and which enforces certain restrictions on the log file's path.
//
// CCxMsgLogBar is little more than a resizable, dockable dialog bar container for CCxMsgLogDlg.  It provides several 
// wrapper methods for convenient access to the embedded CCxMsgLogDlg's capabilities.  Both are built upon Cristi 
// Posea's docking bar framework, CSzDlgBarDlg and CSizingDialogBar (see CREDITS). 
//
//
// CREDITS:
// 1) CCxMsgLogDlg/Bar are built from the resizable dockable dialog bar framework CSzDlgBarDlg/CSizingDialogBar which, 
// in turn, was built upon the MFC extension CSizingControlBarCF, by Cristi Posea.  See CSizingDialogBar implementation 
// file for full credits.  All dialogs housed in a CSizingDialogBar must be derived from CSzDlgBarDlg.
//
//
// REVISION HISTORY:
// 16apr2003-- Created.  Replaces an earlier incarnation (CMessageLogBar) which merely contained a read-only msg log.
// 22sep2004-- Log file directory (appearing in file edit ctrl w/in CCxMsgLogDlg) is set IAW a registry setting at 
//             startup, then saved in the registry before GUI is destroyed.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff and extensions 
#include "cntrlx.h"                          // the application object; resource defines

#include "cxruntime.h"                       // CCxRuntime -- the CNTRLX runtime interface
#include "cxmsglogbar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxMsgLogDlg
//
//===================================================================================================================== 
//===================================================================================================================== 

IMPLEMENT_DYNCREATE( CCxMsgLogDlg, CSzDlgBarDlg )

BEGIN_MESSAGE_MAP( CCxMsgLogDlg, CSzDlgBarDlg )
   ON_WM_DESTROY()
   ON_NOTIFY( FEC_NM_PREBROWSE, IDC_MSGLOG_PATH, OnPreBrowse )
   ON_CONTROL( BN_CLICKED, IDC_MSGLOG_SAVE, OnBtnClicked )
   ON_WM_SIZE()
END_MESSAGE_MAP()


//===================================================================================================================== 
// STATIC MEMBER INITIALIZATION
//===================================================================================================================== 

const int CCxMsgLogDlg::LOGBUFSIZE        = 1000;  



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxMsgLogDlg, ~CCxMsgLogDlg ===================================================================================== 
//
//    In the destructor, we make sure we've stopped logging messages to file and close the log file.
//
//    ARGS:       NONE.
//
CCxMsgLogDlg::CCxMsgLogDlg() : CSzDlgBarDlg( IDD ) 
{
   m_pFile = NULL;
}

CCxMsgLogDlg::~CCxMsgLogDlg()
{
   if( m_pFile != NULL )
   {
      FlushLog(); 
      StopLogging();
   }
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnDestroy (CWnd override) ======================================================================================= 
//
//    ON_WM_DESTROY handler.  
//
//    Prior to destroying the dialog, we store the log file directory (from the file edit control) in the current 
//    user's registry profile.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxMsgLogDlg::OnDestroy()
{
   CString logDir;
   m_fecLogPath.GetCurrentDirectory( logDir );
   ((CCntrlxApp*)AfxGetApp())->SetMRULogDirectory( logDir );
   CSzDlgBarDlg::OnDestroy();
}


//=== OnPreBrowse ===================================================================================================== 
//
//    ON_NOTIFY handler for the custom edit control (CFileEditCtrl) that displays/selects the file system path for 
//    storing the message log file (IDC_MSGLOG_PATH).
//
//    OnPreBrowse() [FEC_NM_PREBROWSE notification code] is called just after the user clicks on the button that 
//    invokes the browsing dialog.  This gives us a chance to prevent browsing entirely, and to further tailor the 
//    appearance of the dialog, if desired. 
//
//    Here we prevent browsing whenever CNTRLX is in a non-idle operational mode.
//
//    ARGS:       pNMH  -- [in] ptr to CFileEditCtrl's FEC_NOTIFY struct, cast as a generic NMHDR*.
//                pRes  -- [out] return value.  for FEC_NM_PREBROWSE, set nonzero value to prevent browsing.  for 
//                         FEC_NM_POSTBROWSE, the return value is ignored.
//
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
void CCxMsgLogDlg::OnPreBrowse( NMHDR *pNMH, LRESULT *pRes )
{ 
   CCxRuntime* pRuntime = ((CCntrlxApp*)AfxGetApp())->GetRuntime();              // access to CNTRLX runtime interface 
   *pRes = (LRESULT) (pRuntime->GetMode() > CCxRuntime::IdleMode); 
}


//=== OnBtnClicked ==================================================================================================== 
//
//    Respond to the BN_CLICKED notification from the "Save Log?" PB control (IDC_MSGLOG_SAVE).
//
//    Start or stop logging messages to file IAW state of the PB.  When logging starts, we disable the log file path 
//    control to prevent user from changing the log file pathname while actively logging.
//
//    ARGS:       NONE
// 
//    RETURNS:    NONE.
//
afx_msg void CCxMsgLogDlg::OnBtnClicked()
{
   if( m_btnSave.GetCheck() != 0 ) 
   {
      StartLogging(); 
      if( m_pFile == NULL ) m_btnSave.SetCheck( 0 );        // NOTE: we may fail to start logging if file I/O error
   }
   else if( m_pFile != NULL )
   {
      FlushLog();
      StopLogging();
   }

   m_fecLogPath.EnableWindow( BOOL(m_pFile==NULL) );        // enable log file path control only when not logging
}


//=== OnSize [base override] ========================================================================================== 
//
//    Response to WM_SIZE message.
//
//    Whenever the dialog is resized, we resize the message log window and the log file path edit control to neatly 
//    fill the available space to the right and bottom of the dialog box.  We assume a layout in which the logging 
//    enable button and the path control lie side by side above the message log.  Thus, we adjust the right and bottom 
//    edges of the log window, but only the right edge of the path control.  We never resize them to be smaller than 
//    their initial sizes as defined in the dialog template.  These initial sizes are determined during dlg creation.
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
void CCxMsgLogDlg::OnSize( UINT nType, int cx, int cy ) 
{
   CSzDlgBarDlg::OnSize( nType, cx, cy );                         // let base class do its stuff (updates scroll bars)
   if( !m_bInitialized ) return;                                  // we haven't determined initial sizes yet

   if( m_fecLogPath.GetSafeHwnd() == NULL ||                      // one or both ctrl HWNDs aren't there!
       m_editLog.GetSafeHwnd() == NULL ) return;

   CRect rClient;                                                 // get dialog's client area; this should not include 
   GetClientRect( rClient );                                      // scroll bars even when they're present

                                                                  // for the log file path control:
   CRect rCurr;                                                   //    get current rect in dialog's coords (to account 
   m_fecLogPath.GetWindowRect( rCurr );                           //    for possible scrolling of this dialog!)
   ScreenToClient( rCurr );

   CRect rNew = rCurr;                                            //    adjust ctrl size IAW sizing event:
   int iSide = rClient.right - 7;                                 //    stick right side of ctrl to right side of dlg, 
   if( (iSide - rNew.left) >= m_sizeEditPath.cx )                 //    unless minimum ctrl width would be violated
      rNew.right = iSide;
   else
      rNew.right = rNew.left + m_sizeEditPath.cx;

   if( rNew != rCurr ) m_fecLogPath.MoveWindow( rNew );           //    make the adjustment now, if any

   m_editLog.GetWindowRect( rCurr );                              // repeat for the msg log window, except that both 
   ScreenToClient( rCurr );                                       // right and bottom sides are adjusted...

   rNew = rCurr;
   if( (iSide - rNew.left) >= m_sizeEditLog.cx ) rNew.right = iSide;
   else rNew.right = rNew.left + m_sizeEditLog.cx;

   iSide = rClient.bottom - 7;
   if( (iSide - rNew.top) >= m_sizeEditLog.cy ) rNew.bottom = iSide;
   else rNew.bottom = rNew.left + m_sizeEditLog.cy;

   if( rNew != rCurr ) m_editLog.MoveWindow( rNew );
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== OnInitDialog [base override] ==================================================================================== 
//
//    Prepare the dialog for display.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CNotSupportedException if any control subclassing fails (which should NOT happen!).
//
BOOL CCxMsgLogDlg::OnInitDialog()
{
   CSzDlgBarDlg::OnInitDialog();                                              // let base class do its thing...

   BOOL bOk = m_fecLogPath.SubclassDlgItem( IDC_MSGLOG_PATH, (CWnd*)this );   // subclass controls on dialog...
   bOk = m_btnSave.SubclassDlgItem( IDC_MSGLOG_SAVE, (CWnd*)this );
   bOk = m_editLog.SubclassDlgItem( IDC_MSGLOG_LOG, (CWnd*)this );

   if( !bOk ) AfxThrowNotSupportedException();                                // the above must succeed to continue... 

   m_fecLogPath.SetFlags( 0 );                                                // to init pos of browse btn in ctrl 

   m_btnSave.SetCheck( 0 );                                                   // initially, logging to file disabled
   m_fecLogPath.EnableWindow();                                               // we can change log file path only when 
                                                                              // we're not logging msgs to file
   m_fecLogPath.SetFileType( CCxFileEdit::LOGFILE );                          // cfg path ctrl to enforce rules for 
   m_fecLogPath.InitializePath(                                               // CNTRLX log file names
         ((CCntrlxApp*)AfxGetApp())->GetMRULogDirectory(),
         _T("maestro") );

   m_editLog.ModifyStyle( 0, WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL );     // ensure msg log has desired styles

   CRect rCtrl;                                                               // save minimum size of msg log window & 
   m_editLog.GetWindowRect( rCtrl );                                          // log file path ctrl, defined by dlg 
   ScreenToClient( rCtrl );                                                   // template, to ensure we do not make 
   m_sizeEditLog = rCtrl.Size();                                              // the controls smaller than this when 
   m_fecLogPath.GetWindowRect( rCtrl );                                       // the dialog gets resized...
   ScreenToClient( rCtrl );
   m_sizeEditPath = rCtrl.Size();

   return( TRUE );                                                            // set focus to 1st ctrl in tab order 
}


//=== ClearLog ======================================================================================================== 
//
//    Empty the log display window in this dialog.  Has no effect on log file's message buffer.
//
//    ARGS:       NONE.
// 
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
VOID CCxMsgLogDlg::ClearLog()
{
   m_editLog.ClearLog();
}


//=== LogMessage ====================================================================================================== 
//
//    Display specified message in the message window, with an optional timestamp.  IF the log file is open, queue the 
//    new message in the pending message buffer and flush the message buffer to the log file when it gets big enough.
//
//    ARGS:       pszMsg      -- [in] the message to be displayed/logged.
//                bTimestamp  -- [in] if TRUE, prepend the message with a current timestamp.
//
//    RETURNS:    NONE.
//
//    THROWS:     CString operations may throw a CMemoryException.
//
VOID CCxMsgLogDlg::LogMessage( LPCTSTR pszMsg, BOOL bTimestamp )
{
   if( pszMsg == NULL && !bTimestamp ) return;                    // in this case, the message string is empty!

   CString strMsg;                                                // prepare msg string, possibly timestamped
   if( bTimestamp )
      strMsg = CTime::GetCurrentTime().Format( "%a %d %b %H:%M:%S %Y :" );
   strMsg += pszMsg;

   m_editLog.LogMessage( strMsg );                                // display in our readonly edit log window

   if( m_pFile != NULL )                                          // if we're logging msgs to file...
   {
      strMsg += _TCHAR('\n');                                     //    append newline to separate msgs in the file
      m_strPending += strMsg;                                     //    put the new msg in our buffer
      if( m_strPending.GetLength() > LOGBUFSIZE )                 //    and flush buffer to file once it's big enuf
      {
         if( !FlushLog() )                                        //    if the flush fails, the log file is closed, so 
         {                                                        //    we need to update the relevant controls
            m_btnSave.SetCheck( 0 );
            m_fecLogPath.EnableWindow();
         }
      }
   }
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== FlushLog ======================================================================================================== 
//
//    If there are any pending message in our message buffer, flush them to the open log file.  If a file I/O error 
//    occurs, immediately stop logging and report error in the log display window.
//
//    ARGS:       NONE
// 
//    RETURNS:    TRUE if successful; FALSE on file I/O error (in which case msg log file is closed).
//
//    THROWS:     NONE.
//
BOOL CCxMsgLogDlg::FlushLog()
{
   if( m_strPending.IsEmpty() ) return( TRUE );                         // msg buffer empty; nothing to do
   ASSERT( m_pFile );

   BOOL bOk = TRUE;                                                     // write string buffer to the open log file
   try { m_pFile->WriteString( m_strPending ); }                        // on file I/O error, stop logging
   catch( CFileException* e )
   {
      e->Delete();
      bOk = FALSE;
      m_editLog.LogMessage( _T("(!!) Log file I/O error occurred.  Logging aborted!") );
      StopLogging();
   }

   m_strPending = _T("");                                               // empty the msg buffer
   return( bOk );
}


//=== StartLogging, StopLogging ======================================================================================= 
//
//    Start and stop logging to a log file the application messages that are displayed in this dialog.
//
//    To start logging, we open the specified log file.  If the file already exists, we seek to the end of the file so 
//    that new messages are appended to it.  To stop logging, we flush message buffer to the open file and close it. 
//    Any file I/O exceptions are caught and an appropriate error message is posted to the log display window.
//
//    ARGS:       NONE
// 
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
VOID CCxMsgLogDlg::StartLogging()
{
   CString str;

   if( m_pFile != NULL ) return;                                        // the log file is already open!
   m_pFile = new CStdioFile;                                            // create file object 

   UINT nOpenFlags = CFile::modeCreate | CFile::modeNoTruncate |        // open new OR existing log file
                     CFile::shareExclusive |                            // share it with no one
                     CFile::modeWrite | CFile::typeText;                // open it as write-only in text mode
   if( !m_pFile->Open(m_fecLogPath.GetCurrentPath(), nOpenFlags ) )     // if we fail to open file, report & abort...  
   {
      delete m_pFile;                                                   //    since file's not open, this will not 
      m_pFile = NULL;                                                   //    throw an exception.
      str.Format( "(!!) Unable to open log file %s", 
                  m_fecLogPath.GetCurrentPath() );
      m_editLog.LogMessage( str );
      return;
   }

   try { m_pFile->SeekToEnd(); }                                        // seek to end of file so we don't write over 
   catch( CFileException *e )                                           // existing log
   {
      e->Delete();
      m_editLog.LogMessage( _T("(!!) Log file I/O error occurred.  Logging aborted!") );
      StopLogging();
      return;
   }

   CTime t = CTime::GetCurrentTime();                                   // success!  queue initial message indicating 
   str = t.Format( "%a %d %b %H:%M:%S %Y :" );                          // that we've started logging to file
   str += _T("  Message logging started.");
   m_editLog.LogMessage( str );
   str += _TCHAR('\n');                                                 // REM: in log file, msgs separated by newlines 
   m_strPending = str;
      
}

VOID CCxMsgLogDlg::StopLogging()
{
   if( m_pFile == NULL ) return;                                        // we're not logging messages to file!

   try                                                                  // destroy file object (which closes the file!) 
   { 
      delete m_pFile;
   } 
   catch( CFileException* e ) 
   {
      e->Delete();
      m_editLog.LogMessage( _T("(!!) Log file I/O error occurred on file close!") );
   }

   m_pFile = NULL;
}


