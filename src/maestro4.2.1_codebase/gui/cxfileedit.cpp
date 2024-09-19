//===================================================================================================================== 
//
// cxfileedit.cpp : Implementation of class CCxFileEdit.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// In the CNTRLX GUI, we use a "file edit control", CFileEditCtrl (CREDITS), to represent the pathname of a CNTRLX 
// data file or a message log file.  This control provides support for a "browse" button -- drawn within the control 
// itself -- that invokes a file dialog by which the user can chooose a new pathname.  While this control is very handy 
// for our purposes, we needed to customize its behavior for our particular application.
//
// CCxFileEdit is a simple class derived from CFileEditCtrl that provides the desired customizations -- enforcing 
// certain CNTRLX-specific restrictions on pathnames that appear in the client area of the control:
//    1) Only allows the control to represent a single filename (CFileEditCtrl flags FEC_FILE & ~FEC_MULTIPLE).  
//       Always includes the style flag FEC_BUTTONTIP.
//    2) Does not permit CEdit style ES_MULTILINE.
//    3) The "open file dialog" created by the baseline CFileEditCtrl requires that any pathname specified by the user 
//       must refer to an existing file.  We lift this restriction.  For CNTRLX data files, we actually want the 
//       pathname reflected in the control to refer to a NONEXISTENT file!
//    4) We enforce several restrictions on the pathname currently in the control, depending on whether it displays a 
//       log file name or a data file name.
//          a) The directory must exist.
//          b) For data files, the filename must end in a 4-digit numeric extension 1..9999.  For a CNTRLX message log, 
//             the extension is always ".log".
//          c) For data files, if at all possible, the pathname should reference a file that does not yet exist.  There 
//             is no such restriction for log files -- the user may wish to append new application messages to an 
//             existing log file.
//       Whenever the user attempts to alter the pathname, we enforce these restrictions.  In particular, if the path 
//       points to an existing data file, we'll advance the numeric ext (up to 9999) in an attempt to find a filename 
//       that does not yet exist.  Enforcement occurs if the user changes the pathname via the file dialog, or directly 
//       by typing into the edit control itself (if it is not ES_READONLY).
//    5) Provide a means for incrementing the numeric extension of the current filename (data file path controls only). 
//    6) When the control is created,  it provides a default pathname of the form %TEMP%\data_DDMMMYYYY.0001 for data 
//       files, and %TEMP%\cntrlx.log for a log file.
//    7) Use SetFileType() to set the type of CNTRLX file reflected in the path control, and thus the naming 
//       restrictions enforced.  By default, CCxFileEdit enforces restrictions for CNTRLX data filenames.
//    8) When you subclass a standard edit control in a dialog template to CCxFileEdit, you must call SetFlags() after 
//       subclassing in order to get the browse button correct (unless the button gets repositioned b/c of a sizing 
//       event).
//
// CREDITS:
// (1) Article by Pete Arends [10/14/2001, www3.telus.net/pja/CFileEditCtrl.htm] -- CFileEditCtrl.  This CEdit-derived 
// class provides a single control for browsing and editing pathnames.  It combines the functionality of CEdit, 
// CFileDialog, and SHBrowseForFolder() in one neat package.  It is the base class for CCxFileEdit.
//
//
// REVISION HISTORY:
// 07oct2002-- Began development.
// 17apr2003-- Modified to support CNTRLX message log file pathnames.  The only naming restriction in this case is that 
//             the file extension be ".log".
//          -- Also added OnChar() handler to revalidate pathname whenever user hits "Enter" key, and modified the 
//             OnPostBrowseHandler() to allow parent to also handle the FEC_NM_POSTBROWSE notification.
// 18apr2003-- OnChar() handler did not work.  When part of a dialog box, the dialog parent intercepts "Enter" keydown 
//             so that WM_CHAR is never generated!  To fix, we override CFileEditCtrl::PreTranslateMessage().
// 21apr2003-- STILL not seeing "Enter" keypresses!  OS must send keyboard messages to the dialog box parent FIRST 
//             rather than the control with the focus -- in order to implement Windows dialog box keyboard interface.
//             Instead of overriding PreTranslateMessage(), we override OnGetDlgCode() and send DLGC_WANTALLKEYS -- 
//             which tells parent dialog to let us process all key input.  We then process WM_KEYDOWN.  Note that 
//             setting the ES_WANTRETURN style probably won't work, since we intend CCxFileEdit to handle a single-line 
//             edit control, for which this style is irrelevant.
// 02may2003-- Get rid of PreSubclassWindow().  Was not using it the way it was intended.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include <stdafx.h>                          // standard MFC stuff
#include "cntrlx.h"

#include "cxfileedit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxFileEdit, CFileEditCtrl )


BEGIN_MESSAGE_MAP( CCxFileEdit, CFileEditCtrl )
   ON_NOTIFY_REFLECT_EX( FEC_NM_POSTBROWSE, OnPostBrowse )
   ON_WM_KILLFOCUS()
   ON_WM_KEYDOWN()
   ON_WM_GETDLGCODE()
END_MESSAGE_MAP()


//===================================================================================================================== 
// MESSAGE HANDLERS 
//===================================================================================================================== 

//=== OnKillFocus [base override] ===================================================================================== 
//
//    Response to the WM_KILLFOCUS message.
//
//    After letting the base class handle the message, we revalidate the path currently shown in the control -- since 
//    the user may have edited the contents directly.
//
//    ARGS:       pNewWnd -- [in] window that is receiving the keyboard focus.
//
//    RETURNS:    NONE.  
//
void CCxFileEdit::OnKillFocus( CWnd* pNewWnd )
{
   CFileEditCtrl::OnKillFocus( pNewWnd );
   ValidateCurrentPath();
}


//=== OnPostBrowse ==================================================================================================== 
//
//    Response to the reflected FEC_NM_POSTBROWSE notification from the underlying CFileEditCtrl framework.
//
//    We merely revalidate the path currently shown in the control, which will have been updated after the browse op.
//
//    ARGS:       pNMH  -- [in] ptr to CFileEditCtrl's FEC_NOTIFY struct, cast as a generic NMHDR*.
//                pRes  -- [out] return value.  for FEC_NM_POSTBROWSE, return value is ignored.
//
//    RETURNS:    TRUE to allow parent window to handle notification as well; FALSE to prevent.
//
BOOL CCxFileEdit::OnPostBrowse( NMHDR* pNMH, LRESULT* pRes )
{
   ValidateCurrentPath();
   return( TRUE );
}


//=== OnKeyDown [base override] ======================================================================================= 
//
//    WM_KEYDOWN message handler. 
//
//    We revalidate the path currently shown in the control whenever the "Enter" key is pressed.
//
//    NOTE:  When an edit control is part of a dialog box, it will never receive WM_CHAR for the "Enter" key.  As part 
//    of the dialog box keyboard interface, the dialog box parent will intercept the WM_KEYDOWN message and generate an 
//    IDOK command message in its stead.  The WM_KEYDOWN message is never translated into a WM_CHAR!  To work around
//    this mechanism, we have overridden OnGetDlgCode() to inform the parent dialog box that this control wants to 
//    process all keyboard input.
//
//    ARGS:       nChar    -- [in] virtual-key code of key depressed. 
//                nRepCnt  -- [in] repeat count.
//                nFlags   -- [in] see CWnd::OnKeyDown.
//
//    RETURNS:    NONE
//
void CCxFileEdit::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
   if( nChar == VK_RETURN )                                    // "Enter" key press:  clear current selection, 
   {                                                           // since we may alter text, then revalidate the 
      SetSel( -1, -1 );                                        // path string
      ValidateCurrentPath();
   }
   else CFileEditCtrl::OnKeyDown( nChar, nRepCnt, nFlags );    // let base class handle everything else
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== SetFileType ===================================================================================================== 
//
//    Set the type of CNTRLX file reflected in this control.  The file type determines what naming restrictions are 
//    enforced upon the file's pathname.  The contents of the control are updated IAW the change in file type.
//
//    ARGS:       ft -- [in] the CNTRLX file type.
//
//    RETURNS:    NONE.
//
VOID CCxFileEdit::SetFileType( FType ft ) 
{
   if( ft != m_type ) 
   {
      m_type = ft;
      ValidateCurrentPath( TRUE );
   }
}


//=== GetCurrentPath ================================================================================================== 
//
//    Return absolute pathname currently reflected in the file edit control.
//
//    ARGS:       NONE
//
//    RETURNS:    current pathname.
//
const CString& CCxFileEdit::GetCurrentPath()
{
   ValidateCurrentPath();                                         // make sure current path is valid -- just in case!
   return( (const CString&) m_strPath );
}


//=== GetCurrentPath ================================================================================================== 
//
//    Return directory path currently reflected in the file edit control.
//
//    ARGS:       strDir   -- [out] CString that will hold the absolute pathname of directory reflected in the ctrl.
//
//    RETURNS:    NONE.
//
VOID CCxFileEdit::GetCurrentDirectory( CString& strDir )
{
   ValidateCurrentPath();                                         // make sure current path is valid -- just in case!
   strDir = m_strPath;                                            // get valid directory from path; any 
   LPTSTR lptstr = strDir.GetBuffer( _MAX_PATH ); 
   GetValidFolder(lptstr, _MAX_PATH);
   strDir.ReleaseBuffer();

   int last = strDir.GetLength() - 1;                             // get rid of trailing backslash
   if( last >= 0 && strDir[last] == _T('\\') ) 
      strDir.Delete( last );
}


//=== IncrementFileExt ================================================================================================ 
//
//    Increment the numeric file extension of the path currently reflected in the file edit control -- but ONLY if the 
//    control is configured to display the path of a CNTRLX data file.  This action does not apply to a log file path!
//
//    If the resulting pathname points to an existing file, keep incrementing until we construct a path that does not 
//    exist, or we reach 9999.  If the numeric extension is already 9999, do nothing.  NOTE that we cannot *guarantee* 
//    that the resultant pathname will point to a nonexistent file!!
//
//    ARGS:       NONE
//
//    RETURNS:    new pathname after extension is incremented.
//
const CString& CCxFileEdit::IncrementFileExt()
{
   if( m_type == CCxFileEdit::LOGFILE ) return( (const CString&) m_strPath );    // ignore for log file pathname

   ValidateCurrentPath();                                                        // make sure current path is valid

   TCHAR lpstrName[_MAX_FNAME];                                                  // parse the current pathname
   TCHAR lpstrExt[_MAX_EXT];
   TCHAR lpstrDir[_MAX_DIR];
   TCHAR lpstrDrive[_MAX_DRIVE];
   _tsplitpath_s( m_strPath, lpstrDrive, lpstrDir, lpstrName, lpstrExt );

   int nExt = 0;                                                                 // convert current ext to an int
   sscanf_s( lpstrExt, ".%d", &nExt );
   ASSERT( nExt >= 1 && nExt <= 9999 );

   if( nExt < 9999 )                                                             // if we can, incr ext & revalidate
   {
      ++nExt;
      CString strCurr;
      strCurr.Format( "%s%s%s.%04d", lpstrDrive, lpstrDir, lpstrName, nExt );
      SetWindowText( strCurr );
      ValidateCurrentPath();
   }

   return( (const CString&) m_strPath );
}


//=== InitializePath ================================================================================================== 
//
//    Initialize the pathname appearing in the control so that it reads:
//
//       DATAFILE:   dir\baseDDMMMYYYY.NNNN
//       LOGFILE:    dir\base.log
//
//    where:  "dir" is the provided directory, "base" is the string provided, DDMMMYYYY is the current date, 
//    and NNNN=0001, or the first index such that the composed pathname points to a nonexistent file.  Whatever path 
//    was previously in the control is replaced.  If the specified file system directory does not exist, the system 
//    temporary directory will be used.
//
//    ARGS:       strDir   -- [in] full pathname to an existing file system directory.
//                strBase  -- [in] file "basename" allows limited customization of the initial pathname.
//
//    RETURNS:    NONE.
//
VOID CCxFileEdit::InitializePath( LPCTSTR strDir, LPCTSTR strBase )
{
   m_strPath.Empty();                                       // reinitialize our private copy of path

   CFileStatus status;                                      // if specified dir does not exist, use system temp dir
   BOOL bOk = CFile::GetStatus( strDir, status ); 
   if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
   if( bOk )
      m_strPath = strDir;
   else
      CCntrlxApp::GetSystemTempDirectory( m_strPath );
   
   if( m_strPath[m_strPath.GetLength()-1] != _T('\\') )     // make sure trailing slash is there
      m_strPath += _T("\\");

   m_strPath += strBase;                                    // tack on file basename

   if( m_type == CCxFileEdit::DATAFILE )                    // for data files, form "dir\baseDDMMMYYYY.0001"
   {
      CTime time = CTime::GetCurrentTime(); 
      m_strPath += time.Format( "%d%b%Y" );
      m_strPath += _T(".0001");
   }
   else m_strPath += _T(".log");                            // for log files, form "dir\base.log"

   SetWindowText( m_strPath );                              // store in edit ctrl and validate (in case we must adjust  
   ValidateCurrentPath( TRUE );                             // numeric ext so that path points to a nonexistent file)
}


//=== SetFlags [base override] ======================================================================================== 
//
//    Change the flags governing the behavior of the file edit control.
//
//    Base class CFileEditCtrl specifies a number of flags (FEC_*) that customize its behavior.  Some flags are not 
//    appropriate for the intended usage of CCxFileEdit:
//       FEC_MULTIPLE   ==> CCxFileEdit must display only a single filename.
//       FEC_FOLDER     ==> CCxFileEdit must diaplay a file, NOT a directory.
//       FEC_WILDCARDS, FEC_NODEREFERENCELINKS ==> Neither of these are appropriate with CCxFileEdit.
//    If any of these flags are specified, the method fails; else, we merely call the base class version.  We also 
//    make sure that the FEC_BUTTONTIP flag is *always* set.
//
//    In addition, we also clear the flag OFN_FILEMUSTEXIST from the OPENFILENAME struct associated with the control's 
//    file dialog object.  This will allow the user to enter a nonexistent filename in the browse dialog -- which is 
//    the desired behavior for CCxFileEdit!
// 
//    Finally, we initialize the data path appearing in the control -- if it has not already been initialized.  We can 
//    do this in this method b/c the underlying CFileEditCtrl framework always calls SetFlags() during control 
//    creation or subclassing.
//
//    ARGS:       dwFlags -- [in] configuration flags (CFileEditCtrl's FEC_* constants) for the file edit control.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CCxFileEdit::SetFlags( DWORD dwFlags ) 
{
   DWORD dwRestrict = FEC_MULTIPLE | FEC_FOLDER | FEC_WILDCARDS | FEC_NODEREFERENCELINKS;
   if( (dwFlags & dwRestrict) != 0 ) return( FALSE );

   dwFlags |= FEC_FILE|FEC_BUTTONTIP;
   BOOL bOk = CFileEditCtrl::SetFlags( dwFlags );
   if( bOk )
   {
      OPENFILENAME* ofn = GetOpenFileName();
      ofn->Flags &= ~OFN_FILEMUSTEXIST;
   }

   if( m_strPath.IsEmpty() ) InitializePath( _T(""), (m_type == DATAFILE) ? _T("data_") : _T("maestro") ); 
   return( bOk );
}


//=== PreCreateWindow [CWnd override] ================================================================================= 
//
//    This method is called during creation of a Windows control.  
//
//    Here we attempt to enforce certain edit control styles that are required for proper operation of the path ctrl:
//       ~ES_MULTILINE :   We require that edit control display only a single line.
//       ES_AUTOHSCROLL :  In case user needs to type in a long pathname that doesn't fit in visible ctrl window.
//
//    ARGS:       cs -- [in/out] the window creation structure
//
//    RETURNS:    [PreCreateWindow] TRUE if successful, FALSE otherwise.  
//
BOOL CCxFileEdit::PreCreateWindow( CREATESTRUCT& cs )
{
   cs.style &= ~ES_MULTILINE;
   cs.style |= ES_AUTOHSCROLL;
   return( CFileEditCtrl::PreCreateWindow( cs ) );
}



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== ValidateCurrentPath ============================================================================================= 
//
//    Ensure that pathname currently reflected in edit control satisfies rules for the type of CNTRLX file (DATAFILE, 
//    LOGFILE) for which the control is configured. 
//
//    ARGS:       bReqd -- [in] if TRUE, we revalidate even if path has not changed [default = FALSE].
//
//    RETURNS:    NONE.
//
VOID CCxFileEdit::ValidateCurrentPath( BOOL bReqd /* =FALSE */ ) 
{
   CString strCurr;                                                        // get pathname currently appearing in ctrl; 
   GetWindowText( strCurr );                                               // if it hasn't changed and revalidation is 
   if( m_strPath == strCurr && !bReqd ) return;                            // not required, return now.

   CString strDir = strCurr;                                               // get valid directory from path; any 
   LPTSTR lptstr = strDir.GetBuffer( _MAX_PATH );                          // nonexistent subdirectories are removed. 
   GetValidFolder(lptstr, _MAX_PATH);
   strDir.ReleaseBuffer();
   if( strDir.IsEmpty() )                                                  // if path does not specify any valid 
   {                                                                       // directory, we look for valid directory in 
      strDir = m_strPath;                                                  // the last valid pathname...
      lptstr = strDir.GetBuffer( _MAX_PATH );
      GetValidFolder(lptstr, _MAX_PATH);
      strDir.ReleaseBuffer();
      if( strDir.IsEmpty() )                                               // ...if that also fails, then set the dir 
      {                                                                    // to the system temp directory
         lptstr = strDir.GetBuffer( _MAX_PATH ); 
         ::GetTempPath( _MAX_PATH, lptstr );
         strDir.ReleaseBuffer();
         if( strDir[strDir.GetLength()-1] != _T('\\') ) 
            strDir += _T("\\");
      }
   }

   TCHAR lpstrName[_MAX_FNAME];                                            // parse path to get file name & ext
   TCHAR lpstrExt[_MAX_EXT];
   TCHAR lpstrDrive[_MAX_DRIVE];
   TCHAR lpstrDir[_MAX_DIR];
   _tsplitpath_s( strCurr, lpstrDrive, lpstrDir, lpstrName, lpstrExt );

   if( m_type == LOGFILE )                                                 // for CNTRLX message log file, ext is 
      m_strPath.Format( "%s%s.log", strDir, lpstrName );                   // always ".log"; file may or may not exist 
   else                                                                    // for CNTRLX data file...
   {
      int nExt = 0;                                                        //    express extension as int in [1..9999] 
      if((::sscanf_s(lpstrExt, ".%d", &nExt) < 1) || (nExt<1) || (nExt>9999)) 
         nExt = 1;

      CFileFind fs;                                                        //    1st nonexistent "<path>.NNNN" becomes 
      while( nExt < 9999 )                                                 //    the new "last valid" path.  we "give 
      {                                                                    //    up" if we reach N=9999!!
         m_strPath.Format( "%s%s.%04d", strDir, lpstrName, nExt );
         if( !fs.FindFile( m_strPath ) )
            break;
         ++nExt;
      }
      m_strPath.Format( "%s%s.%04d", strDir, lpstrName, nExt );            // (this handles extreme case of N=9999)
   }

   SetWindowText( m_strPath );                                             // update edit ctrl to reflect valid path 
}

