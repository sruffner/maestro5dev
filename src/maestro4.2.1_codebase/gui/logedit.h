//===================================================================================================================== 
//
// logedit.h : Declaration of class CLogEdit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(LOGEDIT_H__INCLUDED_)
#define LOGEDIT_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


//===================================================================================================================== 
// Declaration of class CLogEdit 
//===================================================================================================================== 
// 
class CLogEdit : public CEdit
{

//===================================================================================================================== 
// CONSTANTS
//=====================================================================================================================
   static const int LOLOGSIZE = 2000;        // allowed range for the max log size
   static const int HILOGSIZE = 30000;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   int m_nMaxBytes;                       // max # of bytes of text stored in log.  when this limit is reached, the 
                                          // oldest half of the log is discarded. 


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CLogEdit() { m_nMaxBytes = HILOGSIZE; }               // use default copy constructor & operator=
   ~CLogEdit() {}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnSetFocus( CWnd* pOldWnd )              // deny input focus -- no user manipulation allowed
   {
      CFrameWnd* pFrameWnd = GetParentFrame();
      if( pFrameWnd ) pFrameWnd->SetFocus();
   }

   DECLARE_MESSAGE_MAP()



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual BOOL PreCreateWindow( CREATESTRUCT& cs );     // overridden to enforce required edit control styles
   virtual void PreSubclassWindow();                     // overriden to enforce required edit control styles

   int SetMaxLogSize( const int nBytes );                // change max size of log (in #bytes stored)
   int GetMaxLogSize() const { return( m_nMaxBytes ); }  // retrieve max size of log
   VOID ClearLog();                                      // empty the edit ctrl's text buffer entirely
   VOID LogCurrentTime();                                // append a message line showing the current date/time
   VOID LogMessage( LPCTSTR pszMsg );                    // append new message line to edit ctrl
   VOID LogTimeStampedMsg( LPCTSTR pszMsg );             // append new msg line, with prepended date/time stamp

protected: 
   VOID DeleteLines( int nFrom, int nTo );               // delete all lines in specified range (inclusive)
};



#endif // !defined(LOGEDIT_H__INCLUDED_)
