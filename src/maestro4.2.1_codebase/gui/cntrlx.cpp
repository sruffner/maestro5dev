//=====================================================================================================================
//
// cntrlx.cpp : Implementation of class CCntrlxApp, which encapsulates the CNTRLX application.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CREDITS:
//
//
// REVISION HISTORY:
// 04feb2000-- Created.  Just a skeleton for now.  Setting up a standard format for .H, .CPP files...
// 10feb2000-- Moved CAboutDlg class to its own module.
// 17may2000-- Originally, the view class associated with the experiment document template was the custom class
//             CCntrlxView.  However, because we use the Visual Framework API to layout the multiple views in our GUI,
//             we do not need CCntrlxView.  Replaced by generic CView.
// 13jan2001-- Switching to SDI model...
// 04apr2001-- Developing support for prolonged app startup in InitInstance().  Added a message-logging splash screen
//             that runs in its own thread.  This thread will be used to display progress messages as CNTRLX starts,
//             particularly info/error messages that are posted as we attempt to start CXDRIVER.
// 13apr2001-- Added member CCxDriverIO that will encapsulate all direct interactions with CXDRIVER.  For now, this
//             object only provides methods for starting/stopping CXDRIVER and servicing messages posted by it.
//          -- Added LogMessage(), which routes the message to the either the mainframe window or the startup splash
//             screen for display, and also saves a copy of all startup messages (prior to mainframe creation) so they
//             can be posted to the main frame window once it's created (so user can review what happened!)
// 16apr2001-- Added handler for ID_VIEW_MODESW menu item command:  this is a TEMPORARY command I'm using to test out
//             switching between "idle" and "test" modes of CXDRIVER...
// 24apr2001-- Added public static member to expose the CNTRLX application object to other CNTRLX objects.
//          -- Removed the ID_VIEW_MODESW menu item command.  Mode switching is initially handled by CCxMainFrame,
//             which invokes CCntrlxApp::SwitchOpMode to do the work.
// 26apr2001-- Exposed CCxDriverIO member through public method GetDriver().  CCxMainFrame and the mode control panel
//             objects will use this in order to invoke CCxDriverIO methods directly.  So we can get rid of CCntrlxApp
//             methods which merely wrapped a CCxDriverIO method!
// 01jun2001-- Got rid of CCntrlxApp::RefreshTestMode(), which was part of a very awkward means by which CCxDriverIO
//             could tell a mode control panel dialog to refresh its appearance.  CCxDriverIO can now talk directly to
//             the "active" -- i.e., visible -- control dialog, which registers itself with CCxDriverIO whenever it is
//              made visible (by design, only one control dialog can be visible at a time).
// 15jun2001-- Bringing into line w/latest architecture redesign.  CCxDriverIO now a low-level object encapsulated by
//             a "master runtime controller", CCxRuntime.
// 27sep2001-- Cosmetic: CExperimentDoc --> CCxDoc.
// 11oct2001-- Added GetRuntime() and removed GetModeCtrl() plus a number of methods wrapping CCxRuntime functions --
//             IAW latest architectural rework.
// 26mar2002-- Added message handlers for new "Options" menu: ID_OPT_CHAIR and ID_OPT_REWBEEP control flags in CXDRIVER
//             IPC which, respectively, enable or disable use of the animal chair and the playing of a beep whenever a
//             reward is delivered to the animal.
// 15oct2002-- Simple overrides of CWinApp::OnFileNew() & OnFileOpen() to ensure video display params sent to CXDRIVER.
// 18oct2002-- Got rid of "Options" menu item ID_OPT_REWBEEP.  This is now part of the fixation/reward settings that
//             are encapsulated in a control panel dialog page, CCxFixRewDlg.
// 27dec2002-- Added OnFileImport() to handle File|Import menu item.  The entire process of importing cntrlxUNIX defn
//             files is encapsulated in this method and its supporting functions.
//          -- OnUpdateOptions() replaced by a more generic OnUpdateMainMenu(), which updates the enable state of
//             several items in the File menu as well.  These File menu items are enabled only when CXDRIVER is not
//             running or is running in IdleMode.
// 08jan2002-- Moving details of import process to a dedicated class, CCxImporter.
// 11mar2003-- The RTX DLL (rtapi_w32.dll) is now explicitly linked rather than implicitly linked at build time.  The
//             details are encapsulated in the static class CCxRtapi.
// 06may2003-- Minor changes to InitInstance().
// 29dec2003-- Added GetHomeDirectory(), which retrieves Maestro's installation directory from a registry key under
//             HKEY_LOCAL_MACHINE.  This will permit easier relocation of Maestro's program and configuration files.
//             Also added helper method GetLocalMachineAppRegistryKey().
// 22sep2004-- Added per-user registry entries to store the "most recently used" (MRU) Trial-mode data directory,
//             Continuous-mode data directory, and log file directory.
// 08jun2005-- To deal with the problem that CXDRIVER (ie, RTX) can no longer access files on a mapped network drive,
//             we added the functions GetShadowFile() and MoveShadowFile().  CXDRIVER saves the data file to the
//             shadow location on the local hard disk, then Maestro copies it to the remote location.  If the user
//             specifies a file path that is already on the local hard disk, then shadowing is unnecessary.
// 12aug2005-- Added a "remote file mover":  CCxMoveFileQueue member that queues shadow file move operations that are
//             dispatched in a background thread.
// 28apr2010-- Added ID_FILE_JMXIMPORT command: Import a Maestro experiment document stored in a JSON-formatted text
//             file.
// 18jan2012-- Modified GetLocalMachineAppRegistryKey() after I ran into problems opening the Maestro registry key that
//             holds the full path to the installation directory. See function header for details. Eff. Maestro v3.0.0.
// 04sep2013-- Bug fixed: MRU entries were still enabled when CXDRIVER was running in a mode other than Idle. Added 
//             OnUpdateRecentFileMenu() override to disable the MRU entries in the File menu when not in Idle.
//          -- An annoying debug assertion occurs under the hood in CWinApp::OpenDocumentFile(LPCTSTR) that involves
//             occasional failures of AfxFullPath() probably because it is unable to retrieve volume information from
//             a mapped network drive once in a while. Ignoring the assertion is OK, but users find those confusing --
//             and if they choose "Abort" or "Retry" from the debug assertion popup dialog, Maestro is terminated
//             badly, which messes up CXDRIVER and RMVideo. So we decided to finally switch to the "Release" config of
//             the program, in which all ASSERTs and TRACEs are no-ops. An override of OpenDocumentFile() handles the
//             issue mentioned by calling the base class method again if it fails the first time. In testing, this
//             seems to have resolved the problem.
// 24nov2014-- Removed File|Close menu item. It doesn't make sense to close the document in an SDI app, and invoking it
//             often caused Maestro to crash.
// 25oct2017-- Minor changes for Maestro 4.x: Win10/RTX64 platform.
// 11sep2018-- Mod to location of "shadow directory" for remote file ops. Instead of putting it under Maestro's "home"
// directory, it is now located at $ULD\Maestro\shadow\$DDMMMYYYY, where $ULD is the current user's local application 
// data folder. This was done because the new "installer" for Maestro 4 will by default install Maestro in 
// "C:\Program Files\Maestro4", and write access is restricted for file paths under "C:\Program Files".
// 20mar2019-- Added checkmark item ID_OPT_RMVDUPE to the Options menu. If checked, then CXDRIVER will allow as many as
// three duplicate RMVideo frames over the course of a trial before aborting that trial. The option is unchecked at
// startup and likely won't be necessary until we introduce higher refresh-rate RMVideo displays. Testing with a 144Hz
// monitor showed that occasional frame skips may occur.
// 05sep2019-- Introduced new registry entry under APP_KEY: DOTIMING_ENTRY is a string list of three floating-point
// numbers, representing the busy wait times after each of the 3 stages in which a digital command is delivered to
// "latched" external devices in the DIO interface. Each busy wait is in microseconds. This rather low-level control
// was added because some DIO interface designs in use in other labs require a minimum DataReady pulse width to
// successfully latch a command, and testing has shown that the low-level register writes that implement the 3 stages
// of delivering a DO command may be delayed for a few microseconds on the hardware.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "lmcons.h"                          // constants for Platform SDK ::GetUserName()
#include "cntrlx.h"                          // includes resource IDs for CNTRLX

#include "cxrtapi.h"                         // CCxRtapi -- loads RTX DLL and exposes selected RTX API functions
#include "logsplash.h"                       // CLogSplash -- a separate GUI thread for startup msg-logging splash wnd
#include "cxruntime.h"                       // CCxRuntime -- runtime interface handles mode switches, manages CXDRIVER
#include "cxmainframe.h"                     // CCxMainFrame -- the CNTRLX main frame window (SDI)
#include "cxdoc.h"                           // CCxDoc -- the CNTRLX "experiment" document
#include "dirchooser.h"                      // CDirChooser -- for browsing directories in the file system
#include "cximporter.h"                      // CCxImporter -- encapsulates the process of importing cxUNIX defn files
#include "jmxdoc\jmxdocimporter.h"           // JMXDocImporter -- import a JSON-encoded Maestro eXperiment (JMX) doc
#include "cxabout.h"                         // CCxAbout -- "About CNTRLX" dialog
#include "cxmovefilequeue.h"                 // CCxMoveFileQueue -- Handles file I/O to remote drive location


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
// THE GLOBAL APPLICATION OBJECT
//=====================================================================================================================

CCntrlxApp theApp;



IMPLEMENT_DYNAMIC( CCntrlxApp, CWinApp )

BEGIN_MESSAGE_MAP( CCntrlxApp, CWinApp )
   ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
   ON_COMMAND( ID_HELP_USRGUIDE, OnOpenUserGuide )
   ON_COMMAND( ID_APP_EXIT, CWinApp::OnAppExit )
   ON_COMMAND_RANGE( ID_OPT_CHAIR, ID_OPT_RMVDUPE, OnOptions )
   ON_UPDATE_COMMAND_UI_RANGE( ID_OPT_CHAIR, ID_OPT_RMVDUPE, OnUpdateMainMenu )
   ON_COMMAND( ID_FILE_IMPORT, OnFileImport )
   ON_UPDATE_COMMAND_UI( ID_FILE_IMPORT, OnUpdateMainMenu )
   ON_COMMAND( ID_FILE_JMXIMPORT, OnFileJMXImport )
   ON_UPDATE_COMMAND_UI( ID_FILE_JMXIMPORT, OnUpdateMainMenu )
   ON_COMMAND( ID_FILE_NEW, OnFileNew )
   ON_UPDATE_COMMAND_UI( ID_FILE_NEW, OnUpdateMainMenu )
   ON_COMMAND( ID_FILE_OPEN, OnFileOpen )
   ON_UPDATE_COMMAND_UI( ID_FILE_OPEN, OnUpdateMainMenu )
   ON_UPDATE_COMMAND_UI( ID_FILE_SAVE, OnUpdateMainMenu )
   ON_UPDATE_COMMAND_UI( ID_FILE_SAVE_AS, OnUpdateMainMenu )
   ON_UPDATE_COMMAND_UI_RANGE( ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE5, OnUpdateRecentFileMenu )
END_MESSAGE_MAP()



//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================

LPCTSTR CCntrlxApp::APP_KEY = _T("SOFTWARE\\HHMI-LisbergerLab\\Maestro"); 
LPCTSTR CCntrlxApp::HOME_ENTRY = _T("Home"); 
LPCTSTR CCntrlxApp::DEFAULT_HOMEDIR = _T("c:\\Maestro"); 
LPCTSTR CCntrlxApp::DOTIMING_ENTRY = _T("SetDOBusyWaits");
LPCTSTR CCntrlxApp::DEFAULT_DOTIMING = _T("1,3,1");
LPCTSTR CCntrlxApp::SETTINGS_KEY = _T("Settings");
LPCTSTR CCntrlxApp::MRUTRIALDATADIR = _T("MRUTDataDir");
LPCTSTR CCntrlxApp::MRUCONTDATADIR = _T("MRUCDataDir"); 
LPCTSTR CCntrlxApp::MRULOGDIR = _T("MRULogDir"); 

LPCTSTR CCntrlxApp::FILEMVRBADMSG = _T("Remote file mover not working; recommend saving data to local disk!");


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

CCntrlxApp::CCntrlxApp()
{
   m_pSplashThrd = NULL;                                    // these need to be set up in InitInstance()...
   m_pRuntime = NULL;

   m_strImportDir.Empty();                                  // initial "import" directory is the curr sys temp dir;
   LPTSTR lptstr = m_strImportDir.GetBuffer( _MAX_PATH );   // make sure there's no trailing slash.
   ::GetTempPath( _MAX_PATH, lptstr );
   m_strImportDir.ReleaseBuffer();
   int last = m_strImportDir.GetLength() - 1;
   if( m_strImportDir[last] == _T('\\') )
      m_strImportDir.Delete( last );

   m_strHomeDir.Empty(); 
   m_strDOCmdTiming.Empty();

   m_bShadowFault = FALSE;
   m_strShadowDir.Empty();
   m_bFileMoverBad = FALSE;
   m_pFileMover = NULL;
}

CCntrlxApp::~CCntrlxApp()
{
}



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnFileNew, OnFileOpen ===========================================================================================
//
//    Whenever a new or existing CNTRLX document is opened, certain application settings (which are stored in CCxDoc)
//    may change.  We therefore must send the (possibly changed) settings to CXDRIVER.  Otherwise, we let the
//    framework handle the details of creating or opening the file.
//
//    Additionally we tell all views that our SDI document is about to be reinitialized -- so that forms can unload
//    any currently displayed user objects BEFORE the objects are actually destroyed!!
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCntrlxApp::OnFileNew()
{
   CCxDoc* pDoc = GetDoc();                                                // if the SDI doc object exists, tell views
   if( pDoc != NULL )                                                      // and mainframe that we are about to
   {                                                                       // reinitialize it...
      CCxViewHint vuHint( CXVH_CLRUSR, 0, CX_NULLOBJ_KEY );
      pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*) &vuHint );
      GetMainFrame()->OnUpdate( &vuHint );
   }

   CWinApp::OnFileNew();
   ASSERT( m_pRuntime );
   m_pRuntime->UpdateVideoCfg();
   m_pRuntime->UpdateFixRewSettings();
}

void CCntrlxApp::OnFileOpen()
{
   CCxDoc* pDoc = GetDoc();                                                // if the SDI doc object exists, tell views
   if( pDoc != NULL )                                                      // and mainframe that we are about to
   {                                                                       // reinitialize it...
      CCxViewHint vuHint( CXVH_CLRUSR, 0, CX_NULLOBJ_KEY );
      pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*) &vuHint );
      GetMainFrame()->OnUpdate( &vuHint );
   }

   CWinApp::OnFileOpen();
   ASSERT( m_pRuntime );
   m_pRuntime->UpdateVideoCfg();
   m_pRuntime->UpdateFixRewSettings();
}


//=== OnFileImport ====================================================================================================
//
//    Handles the "File|Import" menu item command.  See CCxImporter for a description of the import process.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCntrlxApp::OnFileImport()
{
   ASSERT( m_pRuntime );
   if( (!m_pRuntime->IsOn()) || (m_pRuntime->GetMode() == CCxRuntime::IdleMode) )
   {
      CDirChooser chooser;
      if( chooser.Browse( m_pMainWnd->GetSafeHwnd(), _T("Select an import directory"), m_strImportDir ) )
      {
         m_strImportDir = chooser.GetChosenDirectory();
         CCxImporter importer;
         importer.DoImport( m_strImportDir );
      }
   }
}

/**
 * Handles the "File|Import JMX doc..." command. A "JMX document" is a Maestro experiment document created by the 
 * Matlab utility function maestrodoc() and saved as a JSON-encoded text file. It provides an avenue for programmatic
 * creation of complex or lengthy experimental protocols.
 *
 * The user is first given an opportunity to save the currently open document (if it has been modified). Unless the
 * user cancels at this point, a new document is opened and a file dialog requests the name of the JMX document file.
 * The contents of that file are then imported into the new document.
 *
 * For details on the JMX document format and the import process, see JMXDocImporter.
 */
void CCntrlxApp::OnFileJMXImport()
{
   ASSERT(m_pRuntime);
   if(m_pRuntime->IsOn() && (m_pRuntime->GetMode() != CCxRuntime::IdleMode)) return;
   
   // save any changes to the current document. If user cancels, abort.
   CCxDoc* pDoc = GetDoc();
   ASSERT(pDoc);
   if(!pDoc->SaveModified()) return;
   
   // get pathname of JMX document from user. If user cancels, abort.
   CFileDialog fileDlg(TRUE, _T("jmx"), NULL, OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,
         _T("JMX document(*.jmx)|*.jmx||"), GetMainFrame());
   if(IDCANCEL == fileDlg.DoModal()) return;
   CString jmxPath = fileDlg.GetPathName();
   
   // reinitialize the document. Inform all views that document has been reset.
   pDoc->GetDocTemplate()->SetDefaultTitle(pDoc);
   if(!pDoc->OnNewDocument()) return;
   CCxViewHint vuHint( CXVH_CLRUSR, 0, CX_NULLOBJ_KEY );
   pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*) &vuHint );
   GetMainFrame()->OnUpdate( &vuHint );
   // pDoc->UpdateAllViews(NULL, LPARAM(0), NULL);
   // GetMainFrame()->OnUpdate(NULL);
   
   // import the contents of JMX file into the reinitialized experiment document. If import fails, display
   // error message. Otherwise, inform all views that new objects have been added to the document and application
   // settings have changed.
   JMXDocImporter importer;
   CString errMsg = _T("");
   if(!importer.DoImport(jmxPath, pDoc, errMsg))
      ::AfxMessageBox(errMsg);
   else
   {
      pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*) &vuHint );
      //GetMainFrame()->OnUpdate( &vuHint );
      // pDoc->UpdateAllViews(NULL, LPARAM(0), NULL);
      GetMainFrame()->OnUpdate(NULL);
   }
}

//=== OnAppAbout ======================================================================================================
//
//    This function throws up a simple Maestro "About" box.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCntrlxApp::OnAppAbout()
{
   CCxAbout aboutDlg;
   aboutDlg.DoModal();
}


//=== OnOpenUserGuide =================================================================================================
//
//    Response to the ID_HELP_USRGUIDE menu command.  Attempts to open the home page for Maestro's online user's guide
//    in the default web browser using the shell command 'open'.  This is not sophisticated AT ALL.  For example, if
//    user repeatedly invokes the command, multiple browser windows will likely be the result... and they won't close
//    when the invoking instance of the application exits.
//
void CCntrlxApp::OnOpenUserGuide()
{
   if( m_pMainWnd == NULL || m_pMainWnd->m_hWnd == NULL ) return;

   CString usrGuideAddr;
   if( !usrGuideAddr.LoadString( IDS_USRGUIDE_HOME ) )
   {
      ::MessageBeep( MB_ICONEXCLAMATION );
      LogMessage( "Cannot find web address for user's guide!" );
   }

   ::ShellExecute( m_pMainWnd->m_hWnd, "open", usrGuideAddr, "", "", SW_SHOW );
}


/** OnOptions =========================================================================================================
Handle menu item selections from the "Options" submenu of the main menu. 

The Options submenu contains two "checked" options:
   ID_OPT_CHAIR: If checked, a flag in CXIPC is set to inform CXDRIVER that the animal chair/turntable is actually
present in the experiment rig and powered up. CXDRIVER has no other way of checking the state of the animal chair.
   ID_OPT_RMVDUPE: If checked, a flag in CXIPC is set telling CXDRIVER to permit as many as 3 RMVideo "duplicate 
frames" over the course of any trial without aborting the trial. Users may check this option if they are using a
higher refresh-rate RMVideo display (120Hz or higher) and are getting too many trial aborts due to skipped frames.

@param nID The command ID; one of the commands listed above.
*/
void CCntrlxApp::OnOptions(UINT nID)
{
   ASSERT( m_pRuntime );

   if(nID==ID_OPT_CHAIR) m_pRuntime->ToggleChairPresent();
   else if(nID==ID_OPT_RMVDUPE) m_pRuntime->ToggleAllowRMVDuplFrames();
}


/** OnUpdateMainMenu ==================================================================================================
ON_UPDATE_COMMAND_UI_RANGE handler which updates the enable and/or checked state of selected menu items in Maestro's 
main menu. 

File operations are disabled if CXDRIVER is running in any mode other than Idle. The ID_OPT_CHAIR item is enabled only 
if CXDRIVER is running, and is checked if the relevant flag in the CNTRLX runtime object is currently set. The
ID_OPT_RMVDUPE item is enabled only if CXDRIVER is in Idle Mode, and it is checked if the relevant flag in the CNTRLX 
runtime object is set.

@param pCmdUI This object handles the details of updating the menu item state.
*/
void CCntrlxApp::OnUpdateMainMenu( CCmdUI* pCmdUI )
{
   ASSERT( m_pRuntime );

   if( pCmdUI->m_nID == ID_OPT_CHAIR )
   {
      pCmdUI->Enable( m_pRuntime->IsOn() );
      pCmdUI->SetCheck( m_pRuntime->IsChairPresent() );
   }
   else if(pCmdUI->m_nID == ID_OPT_RMVDUPE)
   {
      pCmdUI->Enable(m_pRuntime->IsOn() && (m_pRuntime->GetMode() == CCxRuntime::IdleMode));
      pCmdUI->SetCheck(m_pRuntime->AllowRMVDuplFrames());
   }
   else pCmdUI->Enable( (!m_pRuntime->IsOn()) || (m_pRuntime->GetMode() == CCxRuntime::IdleMode) );
}

/**
 Override ensures that all items in the MRU list within File menu are DISABLED when CXDRIVER is running in any mode 
 other than Idle. Opening a file during one of the active operational modes can easily cause a crash.
*/
void CCntrlxApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	ENSURE_ARG(pCmdUI != NULL);
	if (m_pRecentFileList == NULL) // no MRU files
		pCmdUI->Enable(FALSE);
   else if((!m_pRuntime->IsOn()) || (m_pRuntime->GetMode() == CCxRuntime::IdleMode))
      m_pRecentFileList->UpdateMenu(pCmdUI);
   else
      pCmdUI->Enable(FALSE);
}


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================

//=== MaxTraces [static method] =======================================================================================
//
//    Maximum # of traces that can be displayed in the CNTRLX data trace window.  This depends on limitations in both
//    the trace window and the runtime interface.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
int CCntrlxApp::MaxTraces()
{
   int nMax = CCxRuntime::MaxTraces();
   if( CCxMainFrame::MaxTraces() < nMax ) nMax = CCxMainFrame::MaxTraces();
   return( nMax );
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== InitInstance [base override] ====================================================================================
//
//    Standard initialization of the application object.  Here's where we set up the SDI doc/view framework specific to
//    CNTRLX.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL CCntrlxApp::InitInstance()
{
   // construct the runtime controller; if this fails we must abort
   m_pRuntime = new CCxRuntime; 
   if(m_pRuntime == NULL) return(FALSE);

   // start splash screen thread. The splash bitmap is defined in resource. Force message area to bottom third. Splash
   // screen stays up until we terminate it...
   m_pSplashThrd = new CLogSplash(IDB_SPLASH, CRect(0,0,0,0), UINT(-1), 0, &m_pSplashThrd);

   LogCurrentTime();
   LogMessage( _T("Initializations...") );

   // the registry key for storing app settings
   SetRegistryKey(_T("MAESTRO.srscicomp.com"));

   // load standard INI file options, including 5 MRU files
   LoadStdProfileSettings(5);

   // register Maestro's SDI doc template. NOTE that view class is not used. We use "Visual Framework" for GUI layout.
   CSingleDocTemplate* pDocTemplate = 
      new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CCxDoc), RUNTIME_CLASS(CCxMainFrame), RUNTIME_CLASS(CView));
   AddDocTemplate(pDocTemplate);

   RegisterShellFileTypes(TRUE);

   // load RTX DLL. If successful, then try to start CXDRIVER. We start loading GUI while CXDRIVER starts up and checks
   // hardware. NOTE: By explicitly loading the RTX DLL, we can still run MAESTRO on a system that lacks RTX64. 
   if(!CCxRtapi::Open())
      LogMessage(_T("(!!) Unable to load RTX DLL; RTX not installed?"));
   else
   {
      // first call to RTX lib puts process in real-time priority clas; but we need normal priority for GUI tasks
      if(!::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS))  
         LogMessage(_T("(!!) Cannot switch process to normal priority!")); 
      if(!m_pRuntime->Start())
         LogMessage(_T("(!!) Unable to start runtime controller"));
   }

   LogMessage(_T("Loading GUI..."));
   Sleep(1000);

   // kill splash screen thread
   if(m_pSplashThrd) m_pSplashThrd->Kill(); 
   m_pSplashThrd = NULL;

   // parse command line (we don't use it)
   CCommandLineInfo cmdInfo;
   ParseCommandLine(cmdInfo);

   // here's where we prepare the GUID for display; mainframe window created here. Can't continue if this fails.
   if(!ProcessShellCommand(cmdInfo)) 
   { 
      m_pRuntime->Stop();
      return(FALSE);
   }

   // m_pMainWnd->DragAcceptFiles();  --> drag/drop document open NOT SUPPORTED

   // enable run modes depending on the state of the runtime controller
   GetMainFrame()->EnableRunModes(); 
   LogMessage( _T("...READY!") ); 

   // start remote file move in another background thread
   m_pFileMover = new CCxMoveFileQueue; 
   if(!m_pFileMover->Start())
   {
      m_bFileMoverBad = TRUE;
      LogMessage(CCntrlxApp::FILEMVRBADMSG);
   }

   return(TRUE);
}


//=== ExitInstance [base override] ====================================================================================
//
//    Perform any necessary clean-up prior to exiting the application.
//
//    Here we make sure that the splash screen thread is gone, and we terminate the runtime controller.
//
//    ARGS:       NONE.
//
//    RETURNS:    application's exit code.
//
int CCntrlxApp::ExitInstance()
{
   // save some per-user preferences in the registry
   WriteProfileString(SETTINGS_KEY, MRUTRIALDATADIR, m_strTDataDir);
   WriteProfileString(SETTINGS_KEY, MRUCONTDATADIR, m_strCDataDir);
   WriteProfileString(SETTINGS_KEY, MRULOGDIR, m_strLogDir);

   // kill splash screen thread if it is still around; free storage for startup messages
   if(m_pSplashThrd) m_pSplashThrd->Kill();  
   m_strStartupMsgs.RemoveAll(); 

   // stop remote file mover after flushing its job queue; then destory it
   if(m_pFileMover != NULL)
   {
      if(!m_bFileMoverBad)
      {
         CWaitCursor waitC;
         if(!m_pFileMover->Stop(10,TRUE))
         {
            ::MessageBeep(MB_ICONEXCLAMATION);
            ::AfxMessageBox(_T("WARNING: Some queued data files may have been left in shadow directory!!"));
            m_bFileMoverBad = TRUE;
            m_bShadowFault = TRUE;
         }
      }
      delete m_pFileMover;
      m_pFileMover = NULL;
   }

   // gracefully stop and then destroy the runtime controller. Then unload RTX DLL.
   if(m_pRuntime != NULL)
   {
      m_pRuntime->Stop(TRUE);
      delete m_pRuntime;
      m_pRuntime = NULL;
   }
   CCxRtapi::Close();

   RemoveShadowDirectory();                              // get rid of temporary shadow directory

   return(CWinApp::ExitInstance());                      // always call base class!
}


//=== OnIdle [base override] ==========================================================================================
//
//    Application background processing.
//
//    The master runtime controller (CCxRuntime) handles operational mode state changes and all communications with
//    the hardware side of CNTRLX (CXDRIVER) through the CCxRuntime::Service() method.  That method is invoked here.
//
//    Using another thread for the master controller might be more responsive, but the MFC objects which are used to
//    display data from CXDRIVER all exist in the primary GUI thread, and manipulating MFC objects in a different
//    thread is very tricky and not recommended.
//
//    When the master controller is disabled (CXDRIVER is not running), it does nothing, and so we give up idle time to
//    save CPU cycles, as recommended in Windows documentation.  However, if the master controller is active, we never
//    yield the idle time -- otherwise we won't return to idle processing until a message is pumped by the primary
//    thread.  Still, if the user tries to do a lot of stuff on the GUI **while** an experiment is running, CNTRLX may
//    not respond with adequate speed to CXDRIVER service requests.
//
//    ARGS:       lCount   -- [in] the current idle count.
//
//    RETURNS:    TRUE to request more idle processing time; FALSE to yield it.
//
BOOL CCntrlxApp::OnIdle( LONG lCount )
{
   BOOL bMoreIdle = TRUE;                       // do we need more idle time?
   CWinApp::OnIdle( lCount );                   // CWinApp gets first crack!
   if( lCount >= 2 )                            // earlier idle cycles are devoted to CWinApp
   {
      bMoreIdle = m_pRuntime->Service();        // service CXDRIVER and update op mode ctrl state; if CXDRIVER not on,
                                                // we don't need more idle cycles
   }
   return( bMoreIdle );
}


/**
 This override attempts to handle an issue that occurs intermittently when opening a file via the MRU list. Under
 the hood, CWinApp::OnOpenRecentFile() ultimately calls this method, which ultimately calls 
 CDocManager::OpenDocumentFile(LPCTSTR, BOOL), which has been found to fail, returning NULL, even though the MRU file 
 path exists and is not too long. In the DEBUG configuration, a debug assertion occurs because AfxFullPath() fails in
 OpenDocumentFile(). In practice, this assertion usually occurs the first time I try to open an MRU file on a mapped 
 network drive, so I have a feeling there's a problem with the GetVolumeInformation call within AfxFullPath. In any 
 case, if I manually ignore the assertion via the pop-up input dialog, the application continues to run OK and the
 next attempt to open an MRU file located on a mapped network drive is successful.

 This behavior is REALLY ANNOYING, and there's no way to avoid the ASSERT(FALSE) statement that triggers the debug
 assertion in OpenDocumentFile(). For this reason, I decided to FINALLY distribute the "Release" configuration of
 Maestro. In this configuration, all ASSERT()s are no-ops and no debug assertions occur. However, the intermittent
 problem remains.
 
 This method tries to address it by calling the base-class CWinApp::OpenDocumentFile method a second time if the first
 attempt to open the file fails. After the first failure, a brief message is logged in the Messages window. Upon a
 second failure, the method gives up and returns NULL -- in which case the document/view framework should report the
 error to the user. 
*/
CDocument* CCntrlxApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
   CDocument* doc = NULL;
   int nTries = 0;
   while(nTries < 2)
   {
      try
      {
         doc = CWinApp::OpenDocumentFile(lpszFileName);
      }
      catch(CException* e)
      {
         e->Delete();
      }

      ++nTries;
      if(doc != NULL) break;
      if(doc == NULL && nTries < 2) LogMessage("Failed to open file; trying once more...");
   }

   return(doc);
}

//=== LogMessage ======================================================================================================
//
//    Display the specified message string on the GUI, with an optional time/date stamp.
//
//    Normally, the message is displayed in the CNTRLX main frame window.  However, during application startup, the
//    main frame window does not yet exist.  In this case, a splash screen should be present, and the message will be
//    displayed on that screen.  This feature keeps the user abreast of what is appening during a somewhat lengthy
//    startup phase.
//
//    Startup messages are stored in a string array so that they can be posted to the main frame window once it has
//    been created.
//
//    ARGS:       str   -- [in] the message string to be displayed.
//                bTime -- [in] if TRUE, message string is preceded by current time & date.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, if it cannot store message string during startup phase
//
VOID CCntrlxApp::LogMessage( LPCTSTR str, BOOL bTime /* =FALSE */ )
{
   CCxMainFrame* pFrame = GetMainFrame();                         // ptr to CNTRLX main frame window

   if( pFrame == NULL )                                           // startup: main frame wnd does not exist yet
   {
      m_strStartupMsgs.Add( str );                                // save msg for displaying in main frame later
      if( m_pSplashThrd ) m_pSplashThrd->Log( str, bTime );       // for now, display it on the splash screen
      return;
   }
   else
   {
      if( m_strStartupMsgs.GetSize() > 0 )                        // first call since startup completed; dump all
      {                                                           // startup messages to mainframe window
         for( int i = 0; i < m_strStartupMsgs.GetSize(); i++ )
            pFrame->LogMessage( m_strStartupMsgs[i] );
         m_strStartupMsgs.RemoveAll();
      }
      pFrame->LogMessage( str, bTime );                           // then handle the current msg string
      return;
   }

}


//=== GetMainFrame, GetDoc ============================================================================================
//
//    Convenience methods for application-wide access to CNTRLX's "one and only" mainframe window, CCxMainFrame, and
//    its one and only (SDI!) document object, CCxDoc.  These CAN return NULL if the corresponding objects have not yet
//    been created.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
CCxMainFrame* CCntrlxApp::GetMainFrame()
{
   if( m_pMainWnd != NULL )
      ASSERT_KINDOF( CCxMainFrame, m_pMainWnd );
   return( (CCxMainFrame*) m_pMainWnd );
}

CCxDoc* CCntrlxApp::GetDoc()
{
   CCxMainFrame* pFrame = GetMainFrame();
   return( (CCxDoc*) ((pFrame != NULL) ? pFrame->GetActiveDocument() : NULL) );
}


//=== IsTimeCritical ==================================================================================================
//
//    Maestro's active runtime modes (Trial, Continuous and Test modes) are considered "time-critical" -- meaning that
//    the GUI should not start any time-consuming operations like throwing up a modal dialog.  This convenience method
//    returns TRUE whenever Maestro is in a time-critical mode.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if Maestro is in one of its time-critical runtime modes; FALSE otherwise.
BOOL CCntrlxApp::IsTimeCritical()
{
   ASSERT( m_pRuntime );
   return( m_pRuntime->IsOn() && (m_pRuntime->GetMode() != CCxRuntime::IdleMode) );
}


/**
 Retrieve the full pathname to Maestro's installation directory as stored in the registry entry 
 HKLM\{$APP_KEY}\{$HOME_ENTRY}. If unable to find or read this entry, a default path is used which may not be correct.
 In such a case, Maestro will be unable to start its runtime controller.

 @returns The full pathname to Maestro's installation directory.
*/
const CString& CCntrlxApp::GetHomeDirectory()
{
   // on first call, we retrieve the value from the registry, then save it internally for subsequent calls
   if( m_strHomeDir.IsEmpty() )
   {
      GetRegistryEntryFromAppKey(CCntrlxApp::HOME_ENTRY, m_strHomeDir);
      if(m_strHomeDir.IsEmpty()) m_strHomeDir = CCntrlxApp::DEFAULT_HOMEDIR;
   }
   return( m_strHomeDir );
}

/**
 Retrieve the registry string value listing the busy wait times the runtime controller will use when delivering a 
 digital output command to external "latched" devices hanging on Maestro's DO port (DO<15..0>). It is stored in the 
 entry HKLM\{$APP_KEY}\{$DOTIMING_ENTRY}. If unable to find or read this entry, a default value is supplied.

 NOTE: This registry entry was introduced in Maestro v4.1.1 (Sep 2019) to provide the user some low-level control
 over timing of DO commands to latched devices like the "marker pulse delivery" device.

 @returns The registry string listing the DO command timing parameters.
*/
const CString& CCntrlxApp::GetDOCommandTiming()
{
   // on first call, we retrieve the value from the registry, then save it internally for subsequent calls
   if(m_strDOCmdTiming.IsEmpty())
   {
      GetRegistryEntryFromAppKey(CCntrlxApp::DOTIMING_ENTRY, m_strDOCmdTiming);
      if(m_strDOCmdTiming.IsEmpty()) m_strDOCmdTiming = CCntrlxApp::DEFAULT_DOTIMING;
   }
   return(m_strDOCmdTiming);
}

//=== Get/SetMRU***Directory ==========================================================================================
//
//    These methods get/set several per-user preference settings that are stored under the Maestro "Settings" registry
//    key:  the most recently used directory for trial-mode data files, the MRU directory for continuous-mode data
//    files, and the MRU directory for log data files.  If a registry entry is missing or does not point to a
//    directory, the getter methods will return the pathname to the current system temp directory.  The setter methods
//    ignore invalid or nonexistent directories.
//
//    Each setting is retrieved from the registry the first time it is requested.  All settings are preserved in the
//    registry only when the application exits -- see ExitInstance().
//
const CString& CCntrlxApp::GetMRUTrialDataDirectory()
{
   if( m_strTDataDir.IsEmpty() )
   {
      m_strTDataDir = GetProfileString( SETTINGS_KEY, MRUTRIALDATADIR );
      CFileStatus status;
      BOOL bOk = CFile::GetStatus( m_strTDataDir, status );
      if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
      if( !bOk ) GetSystemTempDirectory( m_strTDataDir );
   }
   return( m_strTDataDir );
}

void CCntrlxApp::SetMRUTrialDataDirectory( LPCTSTR strPath )
{
   CFileStatus status;
   BOOL bOk = CFile::GetStatus( strPath, status );
   if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
   if( bOk ) m_strTDataDir = strPath;
}

const CString& CCntrlxApp::GetMRUContDataDirectory()
{
   if( m_strCDataDir.IsEmpty() )
   {
      m_strCDataDir = GetProfileString( SETTINGS_KEY, MRUCONTDATADIR );
      CFileStatus status;
      BOOL bOk = CFile::GetStatus( m_strCDataDir, status );
      if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
      if( !bOk ) GetSystemTempDirectory( m_strCDataDir );
   }
   return( m_strCDataDir );
}

void CCntrlxApp::SetMRUContDataDirectory( LPCTSTR strPath )
{
   CFileStatus status;
   BOOL bOk = CFile::GetStatus( strPath, status );
   if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
   if( bOk ) m_strCDataDir = strPath;
}

const CString& CCntrlxApp::GetMRULogDirectory()
{
   if( m_strLogDir.IsEmpty() )
   {
      m_strLogDir = GetProfileString( SETTINGS_KEY, MRULOGDIR );
      CFileStatus status;
      BOOL bOk = CFile::GetStatus( m_strLogDir, status );
      if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
      if( !bOk ) GetSystemTempDirectory( m_strLogDir );
   }
   return( m_strLogDir );
}

void CCntrlxApp::SetMRULogDirectory( LPCTSTR strPath )
{
   CFileStatus status;
   BOOL bOk = CFile::GetStatus( strPath, status );
   if( bOk ) bOk = BOOL((status.m_attribute & CFile::directory) == CFile::directory);
   if( bOk ) m_strLogDir = strPath;
}


//=== GetSystemTempDirectory ==========================================================================================
//
//    A convenience method that gets the system temp directory and makes sure the trailing slash is removed.
//
VOID CCntrlxApp::GetSystemTempDirectory( CString& str )
{
   str.Empty();
   LPTSTR lptstr = str.GetBuffer( _MAX_PATH );
   ::GetTempPath( _MAX_PATH, lptstr );
   str.ReleaseBuffer();
   int last = str.GetLength() - 1;
   if( str[last] == _T('\\') ) str.Delete( last );
}


//=== Get/MoveShadowFile ==============================================================================================
//
//    Support for saving Maestro data files to a "shadow" location on the local hard drive.
//
//    Low-level changes to Windows and/or RTX (starting with RTX 5.1.1, I think) made it impossible to write a file on
//    a mapped network drive from the RTX environment.  Since many users prefer that Maestro write data files directly
//    to a remote drive, the concept of a "shadow file" was introduced.  Whenever the user-specified location for a
//    data file is on a remote drive, Maestro supplies its RTX-based hardware controller, CXDRIVER, with a similarly
//    named file located in "$HOME\shadow\$username\$DDMMYY", where $HOME is Maestro's installation directory,
//    $username is the current user's login name, and $DDMMYY is a current date string.  GetShadowFile() handles the
//    details of this operation.  After CXDRIVER has written the shadow file, the mode controller must call the
//    MoveShadowFile() method, which copies the shadow file to the user-specified destination, then deletes the
//    shadow file itself.
//
//    If the user-specified destination is on a "virtual drive" (a local disk location mapped to a drive letter via the
//    SUBST command), then shadowing is also required.  However, if the ultimate destination is already on the local
//    disk, then shadowing is unnecessary.  In this latter case, GetShadowFile() sets the shadow path to an empty
//    string.
//
//    Note that MoveShadowFile() will fail if the remote drive becomes unavailable, or if the user-specified file path
//    already exists.  Such failure should be brought to the user's IMMEDIATE attention.  The shadow file will not be
//    deleted unless the copy was successful.
//
//    Before Maestro dies, the CCntrlxApp object will attempt to remove the directory "$HOME\shadow\$username",
//    unless a shadow file copy operation failed at any point.  Thus, the user can recover a file that could not be
//    moved to the remote drive.  However, this means the \shadow directory could become quite cluttered over time!
//
//    DEV NOTE:  Currently, the copy operation takes place in the current thread.  If this results in significant
//    delays, it may be necessary to introduce a background thread and shadow file copy "queue"...
//
//    ARGS:       strPath        -- [in] the user-specified destination for a Maestro data file.  MUST be a fully
//                                  qualified path, or GetShadowFile() will fail.  It also fails if the destination
//                                  is neither a local fixed drive nor a remote drive.
//                strShadowPath  -- [in or out] a temporary location for the file that's always on the local disk so
//                                  that it can be written by CXDRIVER. This location is supplied by GetShadowFile()
//                                  and will not yet exist at the time of the call. The same location should be
//                                  supplied to MoveShadowFile() after the shadow file has been written by CXDRIVER. 
//                                  Essentially, it is the concatenation of the shadow directory with with filename 
//                                  extracted from the target path in 'strPath'.
//
//    RETURNS:    TRUE if function successful; FALSE otherwise.
//
BOOL CCntrlxApp::GetShadowFile(LPCTSTR strPath, CString& strShadowPath)
{
   strShadowPath = "";                                               // shadow file path empty ==> shadow unnecessary

   char lpstrName[_MAX_FNAME];                                       // parse the specified pathname
   char lpstrExt[_MAX_EXT];
   char lpstrDir[_MAX_DIR];
   char lpstrDrive[_MAX_DRIVE];
   _splitpath_s( strPath, lpstrDrive, lpstrDir, lpstrName, lpstrExt );

   if( ::strlen(lpstrDrive) == 0 || ::strlen(lpstrDir) == 0 ||       // expect full path name, including extension
       ::strlen(lpstrName) == 0 || ::strlen(lpstrExt) == 0 )
   {
      ASSERT( FALSE );
      return( FALSE );
   }

   CString str = lpstrDrive;                                         // get drive type
   str += "\\";
   UINT driveType = ::GetDriveType( str );
   if( driveType == DRIVE_FIXED )                                    // ultimate destination is on a local fixed drive
   {
      BOOL isVirtual = FALSE;                                        //    if drive has a symbolic link starting with
      CString strNTDevName;                                          //    "\??", then it's probably a virtual drive,
      LPTSTR buf = strNTDevName.GetBuffer( MAX_PATH );               //    in which case, we must shadow; otherwise,
      BOOL bOk= BOOL(0 < QueryDosDevice(lpstrDrive,buf,MAX_PATH-1)); //    there's no need to shadow.
      strNTDevName.ReleaseBuffer();
      if( !bOk || strNTDevName.Find( "\\??" ) != 0 )                 //    if we cannot get NT dev name, we let it go
         return( TRUE );
   }
   else if( driveType != DRIVE_REMOTE )                              // fail: dest not a fixed or remote drive!
   {
      LogMessage( "(!!) Data file must be written to local disk or remote drive!" );
      return( FALSE );
   }

   if( !CreateShadowDirectory( strShadowPath ) )                     // shadowing necessary; create shadow directory if
      return( FALSE );                                               // it is not already there

   str.Format("\\%s%s", lpstrName, lpstrExt);                        // shadow path = shadow_dir + fname + ext
   strShadowPath += str;

   CFileStatus fileStatus;                                           // if shadow path exists, fail
   if( CFile::GetStatus( strShadowPath, fileStatus ) )
   {
      str.Format( "(!!) Shadow file already exists: %s", strShadowPath );
      LogMessage( str );
      return( FALSE );
   }

   return( TRUE );
}

BOOL CCntrlxApp::MoveShadowFile(LPCTSTR strPath, LPCTSTR strShadowPath)
{
   static int iPctFullFileMover = 0;

   CString msg;

   if( !m_bFileMoverBad )                                            // use remote file mover if available, since it
   {                                                                 // works in the background...
      if( m_pFileMover->HasFailed() )                                // if it failed:
      {
         m_bShadowFault = TRUE;                                      //   we must NOT clean up shadow directory later
         CString errMsg;                                             //   inform user
         m_pFileMover->GetErrorMessage( errMsg );
         msg.Format( "(!!) Remote file mover failed (%s).  Restarting...", errMsg );
         LogMessage( msg );

         CWaitCursor waitC;                                          //   attempt to stop & restart file mover
         m_bFileMoverBad = !m_pFileMover->Stop(1, FALSE);
         if( !m_bFileMoverBad )
            m_bFileMoverBad = !m_pFileMover->Start();
         if( m_bFileMoverBad )
            LogMessage( CCntrlxApp::FILEMVRBADMSG );
      }

      if( !m_bFileMoverBad )
      {
         if( m_pFileMover->MoveFile(strPath, strShadowPath) )        //   if successful, check capacity of file
         {                                                           //   mover's queue and warn user as queue grows
            int iPctFull = m_pFileMover->GetPercentFilled();
            if( iPctFull >= iPctFullFileMover + 10 )
            {
               msg.Format( "WARNING: Remote file mover queue at %d percent!", iPctFull );
               LogMessage( msg );
            }
            iPctFullFileMover = iPctFull;
            return( TRUE );
         }
         else
         {
            m_pFileMover->Stop(1, FALSE);
            m_bFileMoverBad = TRUE;
            LogMessage( CCntrlxApp::FILEMVRBADMSG );
            // falls through to fallback code below
         }
      }
   }

   // fallback: make Maestro wait!
   CWaitCursor waitC;                                             //   this code could block for a while...
   CFileStatus fileStatus;                                        //   make sure shadow file is actually there
   if( !CFile::GetStatus( strShadowPath, fileStatus ) )
   {
      msg.Format( "(!!) Cannot move shadow file %s: file does not exist!", strShadowPath );
      LogMessage( msg );
      return( FALSE );
   }

   if( !CopyFile( strShadowPath, strPath, TRUE ) )                //   attempt to copy shadow to final dest; if
   {                                                              //   copy fails, set flag so we do not delete
      m_bShadowFault = TRUE;                                      //   shadow directory later on
      msg.Format( "(!!!) Failed to move shadow file %s to %s", strShadowPath, strPath );
      LogMessage( msg );
      return( FALSE );
   }

   DeleteFile( strShadowPath );                                   //   delete the shadow file

   return( TRUE );
}


/**
 Creates a shadow directory located at "$ULD\Maestro\shadow\$DDMMYYYY", where $ULD is the current user's local 
 application data folder, "$USER_PROFILE\AppData\Local" and $DDMMYYYY is the current date. If the shadow directory was
 already created in a previous call, it merely provides the pathname to that directory.

 When the user elects to save data files to a remote network drive, Maestro must work around the fact that CXDRIVER 
 cannot do so because RTX lacks access to networked resources. Instead, Maestro supplies CXDRIVER with a file path
 in this shadow directory. Once the data file has been saved there, Maestro posts a job to CCxMoveFileQueue to move
 the file from the shadow directory to the original remote destination. Then, if any file move operation fails, the
 user can always recover the data file from this shadow directory.

 This operation will require creating the subdirectories in the shadow directory path. If any operation fails, the
 shadow directory cannot be created, in which case remote file operations will not work. Accordingly, a message is 
 posted in the application message log to warn the user.

 @param strDir [out] Full pathname of the temporary shadow directory.
 @return True if function successful; else false.
*/
BOOL CCntrlxApp::CreateShadowDirectory( CString& strDir )
{
   // if shadow directory already created, simply return it
   if( !m_strShadowDir.IsEmpty() )
   {
      strDir = m_strShadowDir;
      return( TRUE );
   }

   TCHAR szPath[MAX_PATH];
   BOOL bOk = (S_OK == ::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szPath));
   if(!bOk)
   {
      LogMessage("(!!) Unable to create local shadow directory for remote file ops (cannot find local appdata folder)");
      return(FALSE);
   }

   // create intermediate folders ..\Maestro\shadow under the AppData\Local directory (if they are not there yet)
   strDir = szPath;
   strDir += "\\Maestro";
   CFileStatus fileStatus;
   if(!CFile::GetStatus(strDir, fileStatus)) bOk = ::CreateDirectory(strDir, NULL);
   if(bOk)
   {
      strDir += "\\shadow";
      if(!CFile::GetStatus(strDir, fileStatus)) bOk = ::CreateDirectory(strDir, NULL);
   }

   // finally, create the shadow directory itself using the current date string
   if(bOk) 
   {
      CTime time = CTime::GetCurrentTime();
      strDir += "\\" + time.Format( "%d%b%Y" );
      if( !CFile::GetStatus(strDir, fileStatus) ) bOk = ::CreateDirectory( strDir, NULL );
   }

   // warn user if unable to create the shadow directory
   if(!bOk)
   {
      CString msg;
      msg.Format("(!!) Failed to create shadow file directory [%s]for remote drive file ops", strDir);
      LogMessage(msg);
      return(FALSE);
   }

   m_strShadowDir = strDir;
   return(TRUE);
}

/**
 Attempts to remove the local shadow directory that was used to temporarily store data files being streamed to a remote
 network location via MoveShadowFile(). However, if MoveShadowFile() failed at any point since application startup, the
 method does nothing. This method must be called when Maestro exits.

 The shadow directory path is $ULD\Maestro\shadow\$DDMMMYYYY, where $ULD is the user's local application data directory
 and $DDMMMYYYY is the current date string. Only the folder $DDMMMYYYY is removed, since future shadow directories will
 always be placed in $ULD\Maestro\shadow.

 @return True if function successful; else false.
*/
VOID CCntrlxApp::RemoveShadowDirectory()
{
   // if no shadow directory created, or if we failed to move any shadow file to its remote destination, do nothing.
   if(m_strShadowDir.IsEmpty() || m_bShadowFault ) return; 

   CString dir = m_strShadowDir;
   m_strShadowDir.Empty();

   // remove contents of the shadow directory, if any
   CFileFind fileFind; 
   CString path = dir + "\\*";
   BOOL bWorking = fileFind.FindFile( path );
   BOOL bOk = TRUE;
   while( bWorking && bOk )
   {
      bWorking = fileFind.FindNextFile();
      if( !fileFind.IsDirectory() )
         bOk = ::DeleteFile( fileFind.GetFilePath() );
   }
   fileFind.Close();

   // remove shadow directory itself
   if(bOk) bOk = ::RemoveDirectory( dir );
}



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================


//=== GetLocalMachineAppRegistryKey ===================================================================================
//
//    Helper method opens a registry key in HKEY_LOCAL_MACHINE that contains Maestro application settings that are
//    machine-specific: HKEY_LOCAL_MACHINE\SOFTWARE\${COMPANY_KEY}\${APP_KEY}.
//
//    18jan2012: Prior to this date, this function opened the SOFTWARE key, then used that to open ${COMPANY_KEY}, and
//    in turn used that to open ${APP_KEY}. However, on 13jan2012, during testing of Maestro 3.0, this stopped working.
//    I could still open the SOFTWARE key, but I could no longer pass that to RegOpenKeyEx() to open ${COMPANY_KEY}. 
//    During debugging, I found that I could enumerate all the keys under HKLM\SOFTWARE, but I could only open a few of
//    them using the name provided by RegEnumKeyEx(). In addition, the enumeration returned one key that I did not see
//    when I examined the registry with REGEDT32.EXE. Later I discovered that I could open ${COMPANY_KEY} and 
//    ${APP_KEY} with a single call to RegOpenKeyEx(HKEY_LOCAL_MACHINE, ${pathToKey}, ...), where ${pathToKey} is the
//    path for the key relative to the top-level HKEY_LOCAL_MACHINE node.
//       I'm not sure why this happened on 1/13/2012, and remain concerned that the registry was mildly corrupted that
//    day. However, as a result of what I found, I decided to use the single call to RegOpenKeyEx to open the registry
//    key holding Maestro-specific settings....
//
//    Callers are responsible for closing the key handle returned by this method.
//
//    ARGS:       NONE.
//
//    RETURNS:    handle to the registry key, or NULL if method fails
//
HKEY CCntrlxApp::GetLocalMachineAppRegistryKey()
{
   HKEY hAppKey = NULL;
   LONG lRes;
   if((lRes = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, APP_KEY, 0, KEY_READ, &hAppKey)) != ERROR_SUCCESS)
   {
      CString msg;
      msg.Format("ERROR: Failed to open registry key HKEY_LOCAL_MACHINE\\%s; error code = %d", APP_KEY, lRes);
      LogMessage(msg);
   }
   return(hAppKey);
}

/**
 Helper method retrieves one of the Maestro-specific application settings that are stored as registry values under
 the Maestro application key HKLM\SOFTWARE\{$APP_KEY}. The registry value type must be REG_SZ.
 @param strName [in] The name of the registry entry.
 @param strVal [out] If successful, this will contain the entry's REG_SZ value; else it will be empty.
*/
VOID CCntrlxApp::GetRegistryEntryFromAppKey(LPCTSTR strName, CString& strVal)
{
   strVal.Empty();

   BOOL bOk = FALSE;
   HKEY hLMAppKey = GetLocalMachineAppRegistryKey();
   if(hLMAppKey != NULL)
   {
      DWORD dwType, dwCount;
      LONG lRes = ::RegQueryValueEx(hLMAppKey, (LPTSTR)strName, NULL, &dwType, NULL, &dwCount);
      if(lRes == ERROR_SUCCESS && dwType == REG_SZ)
      {
         lRes = ::RegQueryValueEx(hLMAppKey, (LPTSTR)strName, NULL, &dwType, 
               (LPBYTE)strVal.GetBuffer(dwCount / sizeof(TCHAR)), &dwCount);
         strVal.ReleaseBuffer();
         if(lRes == ERROR_SUCCESS) bOk = TRUE;
      }
      ::RegCloseKey(hLMAppKey);
   }

   if(!bOk) strVal.Empty();
}

