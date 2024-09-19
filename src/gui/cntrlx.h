//===================================================================================================================== 
//
// cntrlx.h : Declaration of the CNTRLX application class, CCntrlxApp.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CNTRLX_H__INCLUDED_)
#define CNTRLX_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif


#include "resource.h"                        // resource symbols and defines for CNTRLX

class CCxMainFrame;                          // forward declarations
class CCxDoc;
class CCxRuntime;
class CLogSplash;
class CCxMoveFileQueue;


//===================================================================================================================== 
// Declaration of class CCntrlxApp
//===================================================================================================================== 
//
class CCntrlxApp : public CWinApp
{
   DECLARE_DYNAMIC( CCntrlxApp )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private: 
   static LPCTSTR APP_KEY;                   // path to HKEY_LOCAL_MACHINE subkey for Maestro settings for all users
   static LPCTSTR HOME_ENTRY;                // registry entry under APP_KEY where installation directory is stored 
   static LPCTSTR DEFAULT_HOMEDIR;           // default installation dir if we can't get it from registry
   static LPCTSTR DOTIMING_ENTRY;            // registry entry under APP_KEY for DO command timing string
   static LPCTSTR DEFAULT_DOTIMING;          // default DO command timing string, if we can't get it from registry

   static LPCTSTR SETTINGS_KEY;              // name of user profile key under which some user prefs are stored
   static LPCTSTR MRUTRIALDATADIR;           // name of registry entry for most recently used Trial-mode data directory 
   static LPCTSTR MRUCONTDATADIR;            // name of registry entry for MRU Continuous-mode data directory
   static LPCTSTR MRULOGDIR;                 // name of registry entry for MRU log directory

   static LPCTSTR FILEMVRBADMSG;             // user message reported if remote file mover is not working

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   CLogSplash*    m_pSplashThrd;             // the startup splash screen thread (NULL except during startup)
   CStringArray   m_strStartupMsgs;          // array of messages posted prior to creation of main frame window
   CCxRuntime*    m_pRuntime;                // the CNTRLX runtime interface, incl all communications with CXDRIVER 

   CString        m_strImportDir;            // full pathname to directory specified for the last File|Import command
   CString        m_strHomeDir;              // full pathname to Maestro installation directory (from reg key)

   // a registry entry that lists microsecond busy waits for issuing software-timed digital output commands
   CString        m_strDOCmdTiming; 

   CString        m_strTDataDir;             // full pathname to MRU trial-mode data dir (from registry)
   CString        m_strCDataDir;             // full pathname to MRU continuous-mode data dir (from registry)
   CString        m_strLogDir;               // full pathname to MRU log file directory (from registry)

   BOOL           m_bShadowFault;            // if TRUE, failed to move at least one shadow file to remote destination
   CString        m_strShadowDir;            // temporary shadow directory for moving files to remote destination
   CCxMoveFileQueue* m_pFileMover;           // moves shadow files to remote drive via a background thread
   BOOL           m_bFileMoverBad;           // if TRUE, remote file mover has failed badly and cannot be restarted

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCntrlxApp( const CCntrlxApp& src );                  // no copy constructor defined
   CCntrlxApp& operator=( const CCntrlxApp& src );       // no assignment operator defined 

public:
   CCntrlxApp(); 
   ~CCntrlxApp(); 


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnFileNew();                             // response to standard cmd ID_FILE_NEW
   afx_msg void OnFileOpen();                            // response to standard cmd ID_FILE_OPEN
   afx_msg void OnFileImport();                          // handles the "File|Import" menu item
   afx_msg void OnFileJMXImport();                       // handles the "File|Import JMX doc" menu item
   afx_msg void OnAppAbout();                            // throws up simple "About..." dialog
   afx_msg void OnOpenUserGuide();                       // tries to open online user's guide in default browser
   afx_msg void OnOptions(UINT nID);                     // handles commands from the "Options" menu
   afx_msg void OnUpdateMainMenu( CCmdUI* pCmdUI );      // update checked/enable state selected items on the main menu
   afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);  // override to disable MRU file items when not in Idle mode
   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ATTRIBUTES
//===================================================================================================================== 
public:
   static int MaxTraces();                               // max # of displayed data traces supported by CNTRLX


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL InitInstance();                                  // start CXDRIVER and set up doc/view framework & GUI
   int ExitInstance();                                   // kill CXDRIVER and perform any other cleanup before exiting
   BOOL OnIdle( LONG lCount );                           // we service CXDRIVER in the application background...

   CDocument* OpenDocumentFile(LPCTSTR lpszFileName);    // override to catch an annoying bug in framework

   VOID LogMessage( LPCTSTR str, BOOL bTime=FALSE );     // display a message on the GUI 
   VOID LogCurrentTime() { LogMessage( NULL, TRUE ); }   // display current system time on the GUI 

   CCxMainFrame* GetMainFrame();                         // retrieve properly cast handle to the CNTRLX main frame wnd 
   CCxDoc* GetDoc();                                     // retrive properly cast handle to our single CNTRLX doc
   CCxRuntime* GetRuntime() { return( m_pRuntime ); }    // retrieve handle to the CNTRLX runtime interface 
   BOOL IsTimeCritical();                                // TRUE if Maestro is running in a time-critical op mode

   const CString& GetHomeDirectory();                    // retrieve full pathname to Maestro installation directory
   const CString& GetDOCommandTiming();                  // registry string listing busy waits for DO command deliver

   const CString& GetMRUTrialDataDirectory();            // get/set MRU trial-mode data directory
   void SetMRUTrialDataDirectory( LPCTSTR strPath );
   const CString& GetMRUContDataDirectory();             // get/set MRU continuous-mode data directory
   void SetMRUContDataDirectory( LPCTSTR strPath );
   const CString& GetMRULogDirectory();                  // get/set MRU log file directory
   void SetMRULogDirectory( LPCTSTR strPath );

   static void GetSystemTempDirectory( CString& str );   // get system temporary directory

   BOOL GetShadowFile( LPCTSTR strPath,                  // if specified path is not on a local hard disk, then construct 
      CString& strShadowPath );                          // a shadow file path on local disk
   BOOL MoveShadowFile( LPCTSTR strPath,                 // move shadow file on local disk to desired destination
      LPCTSTR strShadowPath );
private:
   BOOL CreateShadowDirectory( CString& strDir );        // create temp directory for shadow files on local hard disk
   VOID RemoveShadowDirectory();                         // remove shadow directory (called at applic exit)


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   // registry helpers
   HKEY GetLocalMachineAppRegistryKey(); 
   VOID GetRegistryEntryFromAppKey(LPCTSTR strName, CString& strVal);
};


#endif   // !defined(CNTRLX_H__INCLUDED_)
