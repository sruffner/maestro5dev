//===================================================================================================================== 
//
// logedit.cpp : Implementation of class CLogEdit, a simple extension of CEdit for displaying application messages.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CLogEdit is designed as a read-only edit control for displaying application messages line-by-line in a scrollable 
// window.  It includes the following extensions to CEdit:
//
//    SetMaxLogSize() ==> Sets the max # of bytes of text that can appear in the log.  This does NOT duplicate 
//       CEdit::SetLimitText(), which only restricts user input to the ctrl, not programmatic input.
//    GetMaxLogSize() ==> Retrieves the max log size.
//    ClearLog() ==> Empty the text buffer.
//    LogMessage() ==> The provided string is appended as a separate line in the edit control, at the end of the ctrl's 
//       current text buffer.  If the buffer is maxed out, the oldest half of the buffer is discarded to make room for 
//       new messages.
//    LogTimeStampedMessage() ==> Similar to LogMessage(), but a date/time string is prepended to the msg.
//    LogCurrentTime() ==> This method logs the current date and time in the control.
//    OnSetFocus() ==> Overridden to deny the keyboard focus at all times, enforcing the read-only nature of this ctrl. 
//
// USAGE NOTE: My attempts to make CLogEdit work correctly whether it's attached via subclassing or created from 
// scratch (see overrides of PreCreateWindow() and PreSubclassWindow()) did NOT fully work.  In particular, if you 
// subclass CLogEdit to a standard edit control created from a dialog template resource, be sure to set the styles 
// WS_VSCROLL and ES_AUTOHSCROLL in the template resource.  Otherwise, LogMessage() will fail to add the message line 
// properly nor get the vertical scroll bar right when the message falls outside the client area.
//
// CREDITS:
// 1) History Edit Control, by Ravi Bhavnani [1/11/2000, www.codeproject.com/editctrl/history_edit.asp] -- Adapted some 
// code/ideas from this edit control for use in CLogEdit.
//
//
// REVISION HISTORY:
// 15mar2001-- Created.  A very simple extension of CEdit.  May add further enhancements later...
// 09jul2001-- Bug fix:  Clear() failed to delete text programmatically selected w/ SetSel().  Used ReplaceSel() as an 
//             alternative.  Also, for a neater presentation, we delete text on line boundaries.  See DeleteLines().
// 18nov2002-- Bug fix:  OnSetFocus() shifts the focus to the mainframe rather than the previously focused window, 
//             which may not exist...
// 31aug2017-- Minor changes in header file to fix warnings during 64-bit compilation in VS2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff and extensions 
#include "logedit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CLogEdit, CEdit)
   ON_WM_SETFOCUS()
END_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== PreCreateWindow, PreSubclassWindow [CWnd overrides] ============================================================= 
//
//    These methods are called during creation or subclassing of a Windows control.  
//
//    Here we attempt to enforce certain edit control styles that are required for proper operation of the log:
//       ES_MULTILINE :    The log is pretty useless if it cannot contain multiple lines.
//       ES_READONLY :     The log is not intended to handle user input.
//       ES_AUTOVSCROLL :  Without this style, the log will fail to scroll down when a line is added below the current 
//                         visible window bounds.
//       ES_AUTOHSCROLL :  Not as crucial as ES_AUTOVSCROLL, but it does scroll text back to left side when a new line 
//                         of text is added.
//
//    ARGS:       cs -- [in/out] the window creation structure
//
//    RETURNS:    [PreCreateWindow] TRUE if successful, FALSE otherwise.  
//
BOOL CLogEdit::PreCreateWindow( CREATESTRUCT& cs )
{
   cs.style |= ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOHSCROLL;
   return( CEdit::PreCreateWindow( cs ) );
}

void CLogEdit::PreSubclassWindow()
{
   ModifyStyle( 0, ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOHSCROLL );
}


//=== SetMaxLogSize =================================================================================================== 
//
//    Change max size of log to the specified value.  If current contents exceed requested size, the log is truncated 
//    from the oldest entry forward.
//
//    ARGS:       nBytes   -- [in] requested log size. 
//
//    RETURNS:    new log size, which may be different than requested size.
//
int CLogEdit::SetMaxLogSize( const int nBytes )
{
   m_nMaxBytes = nBytes;                                          // honor request, within range limits...
   if( m_nMaxBytes < LOLOGSIZE ) m_nMaxBytes = LOLOGSIZE;
   else if( m_nMaxBytes > HILOGSIZE ) m_nMaxBytes = HILOGSIZE;

   int len = GetWindowTextLength();                               // if current contents exceed new size, truncate...
   if( len > m_nMaxBytes )
   {
      DeleteLines( LineFromChar( m_nMaxBytes-1 ), GetLineCount()-1 );
      ASSERT( GetWindowTextLength() < m_nMaxBytes );
   }

   return( m_nMaxBytes );
}


//=== ClearLog ======================================================================================================== 
//
//    Empty the contents of log edit ctrl entirely.
//
//    ARGS:       NONE.  
//
//    RETURNS:    NONE.
//
VOID CLogEdit::ClearLog()
{
   DeleteLines( 0, GetLineCount()-1 );
}


//=== LogCurrentTime ================================================================================================== 
//
//    Log the current date & time, in a standard format:  "Fri 15 Mar 12:00:00 2001".
//
//    ARGS:       NONE.  
//
//    RETURNS:    NONE.
//
VOID CLogEdit::LogCurrentTime()
{
   CTime t = CTime::GetCurrentTime();
   CString str = t.Format( "%a %d %b %H:%M:%S %Y" );
   LogMessage( LPCTSTR(str) );
}


//=== LogMessage ====================================================================================================== 
//
//    Log specified message to the edit box, appending it at the end of the control's text buffer.  A carriage return 
//    and linefeed combo are appended to the end of the message to ensure that the next message appears on the next 
//    line.  If adding the message would exceed the desired max size of the control's buffer, we discard the oldest 
//    half of the buffer before appending the new message.
//
//    ARGS:       pszMsg   -- [in] the message string to be logged. 
//
//    RETURNS:    NONE.
//
VOID CLogEdit::LogMessage( LPCTSTR pszMsg )
{
   CString str = pszMsg;
   if( str.IsEmpty() ) return;                        // nothing to append!
   if( str.GetLength() > m_nMaxBytes/4 )              // truncate messages that are too long!
   {
      str.Empty();
      str.Format( "%.*s\0", m_nMaxBytes/4, pszMsg );
   }

   str += _T("\r\n");                                 // append carriage return linefeed combo

   int len = GetWindowTextLength();                   // if adding message would exceed allotted buffer size, throw  
   if( len + str.GetLength() > m_nMaxBytes )          // away the oldest half of the log...
   {
      DeleteLines( 0, LineFromChar( len/2 ) );
      len = GetWindowTextLength();
   }

   SetSel( len, len );                                // append the new message string
   ReplaceSel( str );
}


//=== LogTimeStampedMsg =============================================================================================== 
//
//    Similar to LogMessage(), except that the message is prepended with a time stamp.
//
//    ARGS:       pszMsg   -- [in] the message string to be logged. 
//
//    RETURNS:    NONE.
//
VOID CLogEdit::LogTimeStampedMsg( LPCTSTR pszMsg )
{
   CTime t = CTime::GetCurrentTime();
   CString str = t.Format( "%a %d %b %H:%M:%S %Y :" );
   str += pszMsg;
   LogMessage( LPCTSTR(str) );
}


//=== DeleteLines ===================================================================================================== 
//
//    Delete the indicated range of lines, inclusive.  If the range is empty, nothing happens.
//
//    ARGS:       nFrom, nTo  -- [in] zero-based indices of the first and last lines to delete.
//
//    RETURNS:    NONE.
//
VOID CLogEdit::DeleteLines( int nFrom, int nTo )
{
   int len = GetWindowTextLength();
   if( len == 0 ) return;                                   // no text to delete! GetLineCount() rtns 1 in this case. 

   int nLines = GetLineCount();                             // restrict range bounds to reasonable values 
   if( nFrom < 0 ) nFrom = 0;
   if( nTo >= nLines ) nTo = nLines - 1;
   if( nTo < nFrom ) return;                                // empty range 

   int nEraseFrom = LineIndex( nFrom );                     // erase from start of first line to end of last line
   int nEraseTo = (nTo == nLines-1) ? len : LineIndex( nTo + 1 );
   CString strNull;
   SetSel( nEraseFrom, nEraseTo );
   ReplaceSel( strNull );                                   // NOTE: passing NULL causes access violation in USER32!! 
}
