//=====================================================================================================================
//
// cxruntime.cpp : Implementation of CCxRuntime, Maestro's master "controller", which handles all IPC interactions with
//                 MAESTRODRIVER; it serves as the "liaison" between the Maestro GUI and MAESTRODRIVER.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
//
// CREDITS:
// (1) The Real-Time eXtension (RTX) to WindowsNT, by VenturCom, Inc.
//
//
// REVISION HISTORY (CCxDriverIO):
// 05apr2001-- Development begun.
// 24apr2001-- Got rid of private method GetCntrlxApp().  This functionality now provided thru a static method of the
//             CNTRLX app object, CCntrlxApp.
// 01may2001-- Continuing dev of CX_TESTMODE control logic.  Instead of CXDRIVER executing a continuous 2ms DAQ all the
//             time, it only does so when AI is not paused....
// 01jun2001-- Introducing concept of the "active control panel dialog", somewhat analogous to an MFC document's active
//             view.  Via a common base class CCxControlPanelDlg, mode control panel dialogs set/unset the active ctrl
//             panel dlg by invoking CCxDriverIO::SetActiveCtrlDlg().  By design, only one dialog may be active at a
//             time.  CCxDriverIO uses the ptr passed in SetActiveCtrlDlg() to invoke the dialog's Refresh() method, to
//             refresh the dialog's appearance as needed during runtime.
// 11jun2001-- Began major redesign, as CCxDriverIO was doing "too much".  Now, CCxDriverIO merely encapsulates the
//             IPC interface to CXDRIVER, providing methods for opening & closing the interface, and accessing data
//             within the interface in a "thread-safe" manner.  CCxRuntime implements the CNTRLX operational modes,
//             mode switching, and services all transactions with CXDRIVER via calls to CCxDriverIO methods.  It also
//             contains separate "mode control" components that implement the runtime logic and control for the three
//             non-idle operational modes.  Introduced generic command/response communication path in IPC.
// 13jun2001-- Modified how operational mode switches take place.  CNTRLX must issue the CX_SWITCHMODE command to
//             switch op mode, then monitor CXDRIVER's op mode until it matches the requested mode.
// 26jun2001-- Added GetNumTDI(), GetNumTDO() to expose # of digital I/O channels available on event timer device.
// 11aug2001-- Adding support for data trace display facility in shared memory.  I'm also making another architectural
//             change.  To maximize the speed of the data trace facility, CCxDriverIO has direct access to the GUI
//             element which actually draws the traces.  A CNTRLX mode controller merely calls InitTraces() to
//             initialize the facility, and then regular & frequent calls to ServiceTraces() to update the trace
//             display (currently implemented as CGraphBar).  InitTraces() reads a CCxChannel object to obtain the
//             desired display trace attributes.
// 02oct2001-- Added ModifyTraces(), which permits on-the-fly modification of trace attributes without reinitializing
//             the data trace facility (as long as displayed channel set is unchanged).
// 03oct2001-- Another rework of the data trace facility.  CCxDriverIO maintains a copy of the CCxChannel object that
//             encapsulates what CNTRLX signals are drawn in the data trace facility, trace attributes, etc.  It also
//             keeps a copy of the CNTRLX object key identifying that CCxChannel object.  Use SetTraces() to attach a
//             different CCxChannel object to the data trace display (this will reinitialize the display).  GetTraces()
//             retrieves the key of the CCxChannel object currently attached to the data trace display -- this key is
//             CX_NULLOBJ_KEY when the data trace display is "off".  Use OnChangeTraces() to update the data trace
//             display IAW changes in the CCxChannel object -- it is NOT CCxDriverIO's responsibility to detect when
//             such changes occur!
// 08oct2001-- Folded into CCxRuntime!
//
// REVISION HISTORY:
// 11jun2001-- Began development as part of a major redesign in the program architecture.  CCxDriverIO is now more of a
//             low-level interface with CXDRIVER.  CCxRuntime and its component "mode controllers" define how CNTRLX
//             GUI components interact with CXDRIVER via CCxDriverIO, and vice versa.
// 02jul2001-- The idle mode controller is now CCxIdleMode.
// 06aug2001-- "Architectural" mod:  Moved ServiceTraceDisplay(), ServicePositionPlot(), and ServiceMessageQueue() to
//             CCxModeCtrl.  Calling CCxModeCtrl::Service() should handle all aspects of servicing the particular mode,
//             which may or may not involve updating the data trace, plot, and message facilities.
// 08oct2001-- Another major architectural mod.  Got rid of the separate non-GUI mode controller class CCxModeCtrl.
//             Now, the GUI "mode control panel" serves both as a dialog bar container and as mode controller.  The
//             base class for all such control panels is CCxControlPanel.  CCxRuntime is linked to the actual control
//             panels during app startup.... In addition, we merged CCxDriverIO and CCxRuntime.
// 11oct2001-- Added RegisterControlPanel() for linking up with the CNTRLX control panel objects.
// 24oct2001-- Implemented support for the "eye-target" position plot.
//          -- Added RegisterOutputPanels() to link up CCxRuntime with the data trace and position plot panels.
// 30nov2001-- Starting work on TrialMode.  Introduced the helper class CCxTrialSequencer, which will handle most of
//             the hard work of sequencing trials and preparing trial & target definitions in the format that CXDRIVER
//             "understands".
// 27dec2001-- First version of CCxTrialSequencer done.  More features to be added later.
// 28dec2001-- Began implementing TrialMode-specific methods in CCxRuntime.  SendCommand() also modified to handle
//             long-latency commands -- specifically to deal with the issue of preloading framebuffer video targets
//             onto our slow VSG hardware (the CX_TR_PRELOADFB command).
// 02jan2002-- Moved implementation of CCxTrialSequencer to a separate file (CXTRIALSEQ.*).
//          -- Moved TrialMode-specific methods from CCxRuntime to CCxTrialPanel; reworking the division of labor among
//             CCxRuntime, CCxTrialPanel & CCxTrialSequencer.
// 04jan2002-- Added methods for loading TrialMode-specific info in the CXDRIVER IPC interface structure.  We provide
//             direct access to the target and trial code arrays in the IPC interface ONLY when a trial is not running.
// 10jan2002-- AccessTrialCodes() replaced by AccessTrialInfo(), providing direct access to both the trial codes array
//             and the new trial target map in IPC.
// 06feb2002-- Revamped how we start a trial.  The methods for providing direct access to arrays in IPC were awkward.
//             Instead, we pass a const CCxTrialSequencer* ptr to CCxRuntime::LoadTargetList(), which handles the task
//             of loading the target list associated with a trial sequence; and StartTrial(), which loads a particular
//             trial defn into IPC and issues CX_TR_START to start the trial.
// 08feb2002-- Added UpdateVideoCfg(), which sends the current video display configuration to CXDRIVER via the
//             CX_SETDISPLAY command.  There is one video display configuration, CCxVideoDsp, per CCxDoc.  Also added
//             CanUpdateVideoCfg(), which determines whether or not a video display update is currently permissible.
//             This may depend on op mode; eg, in TrialMode, we can only change display parameters when idle (ie, trial
//             sequence is not running).
// 26mar2002-- Added inline methods to access/toggle two option flags in CXIPC: bEnableAudio and bChairPresent.  Just
//             added the latter flag to CXIPC.  Both are initialized to FALSE in InitIPC().
// 18apr2002-- Mods to reflect fact that spike trace data will be saved in the trial data file itself, rather than in a
//             separate file.
// 31jul2002-- Replaced ::GetTickCount()-based version of CElapsedTime with the more accurate RTX-based version used
//             on the CXDRIVER side.
//          -- Added methods supporting ContMode functionality.
// 18oct2002-- Added Get/ResetNumRewardsDelivered(), UpdateFixRewSettings().
//          -- Modified UpdateVideoCfg() to get video display configuration from the CCxSettings object, which replaces
//             CCxVideoDsp....
//          -- Got rid of CXIPC.bEnableAudio and associated CCxRuntime methods.  The "reward indicator beep" enable
//             flag is now part of the fixation/reward settings, sent via CCxRuntime::UpdateFixRewSettings();
// 23oct2002-- Mod data trace facility IAW change to CCxChannel:  Display offset and Y-axis range are now in mV, not
//             device-dependent "b2sAIVolts".  CCxRuntime converts the offset from mV to b2sAIVolts using a constant
//             that assumes a 12bit AI device (eventually, we should get the scale factor from CXDRIVER so that we can
//             support 12- or 16bit devices).  Data is still delivered to the trace display panel in b2sAIVolts.
// 13jan2003-- UpdateVideoCfg() modified IAW change in CX_SETDISPLAY command.  CXDRIVER now handles seed generation for
//             the random dot patterns.  Thus, CX_SETDISPLAY now includes two additional params:  the "fixed" seed
//             value, and an "autoseed" flag indicating whether a new seed is generated for each animation sequence or
//             the fixed seed is used every time.
// 14jan2003-- StartStimulusRun() modified:  CX_CM_RUNSTART command no longer needs to specify the XY dot seed, given
//             the changes dtd 13jan2003.
// 11mar2003-- Modified CCxRuntime to use CCxRtapi, which wraps selected functions exported from the RTX DLL.  The DLL
//             is now manually loaded at application startup so that CNTRLX can run on platforms lacking the RTX
//             environment ("explicit linking" instead of "implicit linking").
// 08apr2003-- Minor mods IAW a major redesign of CNTRLX's mode control panel framework.  We now use a single master
//             mode control panel with mode controller helpers.  The master mode control panel CCxControlPanel handles
//             the mode switch, and it insulates CCxRuntime from the rest of the mode control framework.
// 21apr2003-- RegisterGUIPanels() now called RegisterGUI().  Also added UnregisterGUI() so that the runtime object
//             does not attempt to update GUI elements when they're no longer available.
// 07oct2003-- Modified to use data trace display panel CGraphBar in "delayed display" mode during TrialMode.  After
//             the first trial in a sequence is over, the data traces for the entire trial are displayed in one go; the
//             data traces remain that way during the next trial, the traces for which are built in the background;
//             when that trial is over, the old display traces are replaced by the newly prepared traces.  This
//             behavior is more like what happened in the old cntrlxUNIX and is preferred by most users.  In ContMode
//             and TestMode, CGraphBar is configured in the normal display mode.
//          -- Modified position plot facility to handle a "secondary" eye locus called "Eye2(H)".  For now, there's no
//             vertical pos signal associated with this special purpose locus -- hence the "(H)" suffix.
// 02dec2003-- Fixed minor bugs in UpdateTraces().  In TrialMode, it should switch to the delayed trace set whenever
//             UpdateTraces() is called with an arg of -1, even if no traces were collected.  That way the trace
//             window is refreshed to visibly show that no traces were collected for the last trial!  Also got rid of a
//             misleading error message.
// 29dec2003-- Modified to permit Maestro installation in any chosen directory, which is specified by a registry entry
//             and made available by CCntrlxApp::GetHomeDirectory().  Before starting CXDRIVER, CCxRuntime stores the
//             home directory path in CXIPC.strDataPath, and CXDRIVER uses this to find required files.  See Open().
// 07jul2004-- Added method GetLastTrialLen(), which returns the elapsed time of the last trial presented in TrialMode.
//             CXDRIVER stores the elapsed time in CXIPC.iLastTrialLen when the trial ends (aborted or completed).
// 10mar2005-- Mods to report sample interval to CGraphBar::InitGraph().  Assume 1 ms for Trial mode and 2 ms for
//             Cont mode.
//          -- CXIPC.iTotalRewardMS was added to report the sum of reward pulse lengths delivered.  This statistic is
//             exposed by GetCumulativeReward().  ResetNumRewardsDelivered() was renamed ResetRewardStats(); it resets
//             the cumulative reward stat as well as the #rewards delivered.
// 07apr2005-- Modified GetTrialInfo() to retrieve a trial's tagged sections, if any.
//          -- Added support for consuming digital event timestamps streamed by CXDRIVER.  These timestamps will be
//             passed to another self-contained GUI element, analogous to CGraphBar, that will build and display spike
//             time histograms for any tagged sections found in a trial.
// 21apr2005-- Updated to make use of the new GUI element, CCxSpikeHistBar, that consumes digital event timestamps and
//             builds spike histograms for tagged sections culled from an ongoing trial sequence.  CCxRuntime inits the
//             histogram panel at the start of a trial sequence, delivers trial information before each trial starts,
//             and streams digital events to the panel object while a trial is running.  The histogram panel currently
//             applies only to Trial mode.
// 29apr2005-- Mods IAW same-dtd changes to CGraphBar: graph width is specified as interval [t0..t1] in Trial mode.  In
//             Cont mode (using CGraphBar in normal display mode), t0 is always 0.
// 15jun2005-- Added method GetCursorInPositionPlot() to facilitate following the cursor pos in the eye-target plot
//             with a "tracking" target in Cont mode.
//          -- Mod to support additional locus CX_TRACK, for the "cursor tracking" tgt in Cont mode.
// 10aug2005-- Dedicated ADC11 to "VEPOS2", vertical eye position for a "second eye".  Label associated with the
//             "second eye" locus in position plot now reads "Eye2" instead of "Eye2(H)".
// 05dec2005-- Added method GetRPDistroBehavResp() to support new response distribution-based reward protocol.
// 24mar2006-- Misc mods to support introduction of RMVideo in place of the old VSG framebuffer video.  Most notably,
//             we no longer issue CX_TR_PRELOADFB, which is considered obsolete.
// 21apr2006-- New "get" methods for accessing some RMVideo display properties just added to CXIPC.
// 28apr2010-- Changed SetTransform() method prototype and modified it to also store global starting target position
//             in new CXIPC fields CXIPC.fStartPosH, .fStartPosV.
// 24may2010-- Mod StartTrial() to support storing state of trial bit flags in new field CXIPC.dwTrialFlags.
// 11may2011-- Added Get/SetVStabSlidingWindow(), which get/set the length of the sliding window average of eye 
//             position used to smooth the effects of velocity stabilization in TrialMode. Restricted to [1..20] ms. A 
//             value of 1 disables the feature.
// 11oct2016-- Updated method signatures and comments to reflect fact that the RMVideo "movie store" is now a "media
//             store" that can contain both video files for the "movie" target and image files for the "image" target.
// 25oct2017-- Mods needed to build Maestro for Win10/RTX64. Fix compile errors found in VS Studio 2017.
// 10sep2018-- Mod to ServiceMessageQueue() to support playing the system default sound as a "reward indicator beep"
// when the message string from MAESTRODRIVER == "beep". Previously MAESTRODRIVER played a beep on the system speaker,
// but that has caused unacceptable AI ISR latencies on some Win10/RTX64 systems. And we no longer have to disable
// Windows audio to make Maestro work (the old "not enough IRQs" problem), so why not use it?!
// 24sep2018-- Mod to UpdateVideoCfg() to include parameters governing RMVideo vertical sync spot flash, a new feature
// in RMVideo v8. As of Maestro v4.0.0.
// 09sep2019-- Mod to Open(): Gets "SetDOBusyWaits" registry entry, parses it into 3 busy wait times, and stores these
// times in CXIPC for use by CXDRIVER. See CXIPC.H.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource defines

#include "cxrtapi.h"                         // CCxRtapi: static class exposes selected fcns from RTX DLL
#include "util.h"                            // for CElapsedTime -- elapsed time in microsecs
#include "graphbar.h"                        // CGraphBar -- the Maestro data trace display window
#include "cxspikehistbar.h"                  // CCxSpikeHistBar -- the Maestro spike histogram display
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the Maestro master mode control panel
#include "cxdoc.h"                           // CCxDoc -- the Maestro "experiment" document
#include "cxsettings.h"                      // CCxSettings -- the Maestro application settings object
#include "cxtarget.h"                        // CCxTarget -- the Maestro "target" object
#include "cxcontrun.h"                       // CCxContRun -- the Maestro "cont-mode stimulus run" object
#include "cxmainframe.h"                     // CCxMainFrame -- the Maestro mainframe window, access to GUI facilities
#include "cxtrialseq.h"                      // CCxTrialSequencer -- responsible for sequencing trials from a set
#include "cxtrial.h"                         // CCxTrial -- the Maestro "trial" object

#include "cxruntime.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNAMIC( CCxRuntime, CObject )


//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

const int  CCxRuntime::IdleMode     = CX_IDLEMODE;
const int  CCxRuntime::TestMode     = CX_TESTMODE;
const int  CCxRuntime::TrialMode    = CX_TRIALMODE;
const int  CCxRuntime::ContMode     = CX_CONTMODE;
const int  CCxRuntime::NullMode     = CX_NOTRUNNING;

LPCTSTR CCxRuntime::DRVR_EXECUTABLE = _T("cxdriver.rtss");

LPCTSTR CCxRuntime::WMSG_ORPHANDRVR =
   _T("!! WARNING: Detected Maestro hardware driver already on system; terminating it..." );

LPCTSTR CCxRuntime::EMSG_DATAPATHTOOLONG =
   _T("!! ERROR: A data file path is too long. !!");

LPCTSTR CCxRuntime::EMSG_CANTKILLORPHAN =
_T("!! ERROR: Unable to terminate orphaned Maestro hardware driver (cxdriver.rtss).");

LPCTSTR CCxRuntime::EMSG_CANTKILLORPHAN2 =
_T("   Recommendation: Exit Maestro. Use rtsskill to terminate cxdriver.rtss. Then restart.");

LPCTSTR CCxRuntime::EMSG_IPCFAILED =
   _T("!! ERROR: Unable to create shared memory for IPC -- cannot start Maestro hardware driver");

LPCTSTR CCxRuntime::EMSG_DRVRSYNCFAILED =
   _T("!! ERROR: Unable to sync with Maestro hardware driver; driver probably failed in early startup..." );

LPCTSTR CCxRuntime::EMSG_DRVRDIEDINSTARTUP =
   _T("!! ERROR: Maestro hardware driver died during startup!" );

LPCTSTR CCxRuntime::EMSG_DRVRNOTRESPONDING =
   _T("!! ERROR: Maestro hardware driver is not responding; likely malfunction -- terminating it..." );

LPCTSTR CCxRuntime::EMSG_GRACEFULSTOPFAILED =
   _T("!! ERROR: Graceful shutdown of Maestro hardware driver failed; terminating it..." );





//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxRuntime [constructor] ========================================================================================
//
//    Constructed in an inactive state.  Must call Start() to spawn CXDRIVER and enable CNTRLX operational modes.
//
CCxRuntime::CCxRuntime()
{
   m_bDriverOn = FALSE;
   m_hRtStopMutex = NULL;
   m_hRtShared = NULL;
   m_pShm = NULL;

   m_pModePanel = NULL;
   m_pTracePanel = NULL;
   m_pPlotPanel = NULL;
   m_pHistPanel = NULL;

   m_wChanKey = CX_NULLOBJ_KEY;
   m_chDisplay.ClearAll();
   m_wNextChanKey = CX_NULLOBJ_KEY;
   m_chNext.ClearAll();
   
   m_nModes = 0;
   m_iCurrMode = -1;
   m_rmvFrameRate = 0.0f;
   m_nMediaFolders = -1;
}


//=== ~CCxRuntime [destructor] ========================================================================================
//
//    Return to CX_IDLEMODE and disconnect from CXDRIVER before dying.
//
CCxRuntime::~CCxRuntime()
{
   Stop( FALSE );
   
   // clear out RMVideo mode info and media store TOC
   m_nModes = 0;
   m_iCurrMode = -1;
   m_rmvFrameRate = 0.0f;
   for(int i=0; i<3; i++) m_rmvGamma[i] = 1.0f;
   m_nMediaFolders = -1;
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== RegisterGUI, UnregisterGUI ======================================================================================
//
//    These "glue" methods exist only to link/unlink the Maestro runtime interface object with important GUI elements
//    during application startup (the GUI is created AFTER the runtime interface, by design!).  After the GUI is
//    created, the Maestro mainframe window calls RegisterGUI() to register the GUI elements for operational mode
//    control, the data trace display, the eye-target position plot, and the spike histograms display.  When the
//    mainframe window is destroyed, it calls UnregisterGUI().
//
//    CCxRuntime will invoke methods on these GUI elements only while they're "registered".
//
//    ARGS:       iMode    -- [in] the operational mode governed by control panel.
//                pPanel   -- [in] ptr to the control panel object.
//                pGraph   -- [in] ptr to the data trace display panel.
//                pXYPlot  -- [in] ptr to the eye-target position plot panel.
//                pHist    -- [in] ptr to the spike histograms display panel.
//
//    RETURNS:    TRUE if successful, FALSE if bad args or if a control/output panel is already registered.
//
BOOL CCxRuntime::RegisterGUI(CCxControlPanel* pMode, CGraphBar* pGraph, CXYPlotBar* pXYPlot, CCxSpikeHistBar *pHist)
{
   ASSERT( m_pModePanel == NULL );
   if( (pMode != NULL) && (pGraph != NULL) && (m_pTracePanel == NULL) &&
       (pXYPlot != NULL) && (m_pPlotPanel == NULL) && (pHist != NULL) && (m_pHistPanel == NULL) )
   {
      m_pModePanel = pMode;
      m_pTracePanel = pGraph;
      m_pPlotPanel = pXYPlot;
      m_pHistPanel = pHist;
      ConfigurePositionPlot();
      return( TRUE );
   }
   else
      return( FALSE );
}

VOID CCxRuntime::UnregisterGUI()
{
   m_pModePanel = NULL;
   m_pTracePanel = NULL;
   m_pPlotPanel = NULL;
   m_pHistPanel = NULL;
}


//=== Start ===========================================================================================================
//
//    Start, or restart, CXDRIVER in CX_IDLEMODE.  If this method is called while CXDRIVER is already running, we force
//    a return to CX_IDLEMODE, shut down CXDRIVER, and restart it in CX_IDLEMODE.
//
//    This routine attempts to enable/disable GUI runtime-related components via CCxMainFrame::EnableRunModes();
//    however, if the CNTRLX mainframe window does not yet exist (such as during application startup), this task MUST
//    be handled by the caller.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE if startup failed.
//
BOOL CCxRuntime::Start()
{
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                     // the Maestro app object

   if( m_bDriverOn ) Stop();                                         // CXDRIVER already on, so stop & restart...
   ASSERT( !m_bDriverOn );

   CWaitCursor waitCursor;                                           // this may take a while!
   pApp->LogMessage( _T("Starting CXDRIVER...") );

   if( !Open() ) return( FALSE );                                    // attempt to start CXDRIVER...

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // MONITORING CXDRIVER STARTUP:
   // at this point in "runtime", CXDRIVER should be performing various startup tasks, including the detection and
   // initialization of hardware devices.  during this startup phase, it will post a number of status messages thru
   // CCxRuntime.  if a fatal error occurs during startup, it will terminate.  if inits are successful, the driver
   // will automatically enter CX_IDLEMODE.
   //
   // until then, Maestro must monitor the driver's state and service driver msgs.  in fact, the message requests
   // serve as an indicator that the driver is still alive and kicking.  if no such activity is detected for some
   // time and the driver has still not entered CX_IDLEMODE, then we assume an error has occurred and abort.
   //
   // NOTE that we sleep 20msec on each iteration:  Win32 processes that invoke the RTX API are put in the real-time
   // priority class.  If they do not give up processor time, certain base NT services will freeze...This problem is
   // fixed in RTX 5.0; Win32 processes that use RTX calls are left at normal priority.
   // 06may2003-- May not need to sleep anymore, since we switch process priority to normal in CCntrlxApp before
   //    starting up CCxRuntime...
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   CElapsedTime tWait;                                               // elapsed time since driver last active
   BOOL bDied = FALSE;
   BOOL bNotResponding = FALSE;
   while( GetMode() != CX_IDLEMODE )                                 // waiting for CXDRIVER to enter CX_IDLEMODE...
   {
      if( ServiceMessageQueue() )                                    // service CXDRIVER msg queue; if a message is
         tWait.Reset();                                              // handled, we reset the "inactivity" timer.

      if( !IsAlive() )                                               // has CXDRIVER died unexpectedly during startup?
      {
         bDied = TRUE;
         break;
      }

      if( tWait.Get() > 10e6 )                                       // driver may be "stuck" -- give up
      {
         bNotResponding = TRUE;
         break;
      }

//      Sleep( 20 );
   }

   while( ServiceMessageQueue() ) ;                                  // empty the message queue

   if( bDied || bNotResponding )                                     // died/stopped during startup sequence
   {
      if( bDied ) pApp->LogMessage( EMSG_DRVRDIEDINSTARTUP );
      else        pApp->LogMessage( EMSG_DRVRNOTRESPONDING );
      Close( FALSE );
      return( FALSE );
   }

   ASSERT( GetMode() == CX_IDLEMODE );                               // we should be in CX_IDLEMODE now

   // retrieve and cache RMVideo display info and media store contents 
   RetrieveRMVideoDisplayData(); 
   RetrieveRMVideoMediaStoreTOC();

   CCxMainFrame* pFrame = pApp->GetMainFrame();                      // successful start -- enable GUI runtime cmpts
   if( pFrame ) pFrame->EnableRunModes();                            // if they exist!

   return( TRUE );
}


//=== Stop ============================================================================================================
//
//    Halt master runtime controller.
//
//    ARGS:       bGraceful   -- [in] if TRUE, attempt "graceful" shutdown sequence; else, terminate CXDRIVER
//                               immediately [default = TRUE].
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::Stop( const BOOL bGraceful /* =TRUE */ )
{
   if( bGraceful && (GetMode() > CX_IDLEMODE) )                               // if graceful shutdown, attempt to
   {                                                                          // return to IdleMode first
      if( m_pModePanel != NULL ) m_pModePanel->SwitchMode( IdleMode );        //    use GUI framework if it's there
      else                                                                    //    else just send the raw cmd
      {
         int iMode = IdleMode;
         DWORD dwCmd = CX_SWITCHMODE;
         SendCommand( dwCmd, &iMode, NULL, 1, 0, 0, 0 );
      }
   }

   // disconnect from Eyelink tracker
   m_EyeLink.Disconnect();

   Close( bGraceful );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();         // hide runtime-related GUI components
   if( pFrame != NULL) pFrame->EnableRunModes();
}


//=== GetMode =========================================================================================================
//
//    Retrieve the current operational mode of Maestro-CXDRIVER.
//
//    ARGS:       NONE.
//
//    RETURNS:    the current operational mode, or CX_NOTRUNNING if CXDRIVER is not running.
//
int CCxRuntime::GetMode() const
{
   if( !m_bDriverOn ) return( CX_NOTRUNNING );
   else
   {
      ASSERT( m_pShm != NULL );
      return( m_pShm->iOpMode );
   }
}


//=== IsModeEnabled ===================================================================================================
//
//    Is specified Maestro/CXDRIVER operational mode enabled?  If CXDRIVER is not running, of course, no op modes are
//    available.  Otherwise, CX_IDLEMODE and CX_TESTMODE are enabled, while CX_TRIALMODE and CX_CONTMODE require the
//    presence of the analog input and event timer devices.
//
//    ARGS:       iMode -- [in] the operational mode ID.
//
//    RETURNS:    TRUE if CXDRIVER's state supports the specified operational mode.
//
BOOL CCxRuntime::IsModeEnabled( const int iMode ) const
{
   DWORD dwAvail;
   BOOL bEnable = FALSE;
   switch( iMode )
   {
      case CX_IDLEMODE  :
      case CX_TESTMODE  :
         bEnable = m_bDriverOn;
         break;
      case CX_TRIALMODE :
      case CX_CONTMODE  :
         dwAvail = CX_F_AIAVAIL | CX_F_TMRAVAIL;
         bEnable = m_bDriverOn;
         if( bEnable ) bEnable = BOOL((GetHWStatus() & dwAvail) == dwAvail);
         break;
   }
   return( bEnable );
}


//=== Service =========================================================================================================
//
//    If the driver is on, verify it has not terminated unexpectedly, then service any pending "transactions" between
//    Maestro and CXDRIVER in the current operational mode.  The runtime controller itself services the message queue,
//    data trace, position plot, digital event stream facilities.  Any digital events are passed on to the GUI's spike
//    histogram panel.  The master mode control panel's Service() routine is invoked to update the system state in the
//    current operational mode.
//
//    This routine MUST be called as frequently as possible to ensure timely interactions between Maestro and CXDRIVER.
//
//    ARGS:       NONE.
//
//    RETURNS:    FALSE if CXDRIVER is not running (or died unexpectedly); TRUE otherwise.
//
BOOL CCxRuntime::Service()
{
   if( !m_bDriverOn ) return( FALSE );
   else if( !IsAlive() )
   {
      Stop( FALSE );
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("!!ERROR: CXDRIVER died unexpectedly!!") );
      ::MessageBeep( MB_ICONEXCLAMATION );
      return( FALSE );
   }
   else
   {
      ServiceEyelink();
      ServiceTraces();
      ServiceEventStream();
      ServicePositionPlot();
      ServiceMessageQueue();
      if( m_pModePanel != NULL ) m_pModePanel->Service();
      return( TRUE );
   }
}

/**
 Helper method called by Service() to check for a change in the connection status of the Eyelink tracker interface. 
 Called regularly to detect when a connection is established at startup (or attempting to re-establish a connection), 
 and to detect when the connection is unexpectedly lost. The current mode controller is refreshed accordingly. Also,
 if the Eyelink tracker is connected while in Trial or Cont mode, recording on the Eyelink is initiated. This is
 because the Eyelink should always be recording and streaming eye position data in either of these modes.
 */
VOID CCxRuntime::ServiceEyelink()
{
   BOOL bChanged = m_EyeLink.CheckConnectionStatus();
   if(bChanged)
   {
      if(m_EyeLink.IsConnected()) 
      {
         if(GetMode() == TrialMode || GetMode() == ContMode)
            m_EyeLink.StartRecord();
      }
      if(m_pModePanel != NULL) m_pModePanel->Refresh();
   }
}

//=== IsEmptyMessageQueue =============================================================================================
//
//    Is the CXDRIVER pending message queue empty?
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if queue is empty or CXDRIVER is not on; FALSE otherwise.
//
BOOL CCxRuntime::IsEmptyMessageQueue()
{
   if( !m_bDriverOn ) return( TRUE );
   else
   {
      ASSERT( m_pShm != NULL );
      return( m_pShm->iNextMsgToPost != m_pShm->iLastMsgPosted );
   }
}


//=== ServiceMessageQueue =============================================================================================
//
//    If there's a pending message from CXDRIVER, post it to the GUI.
//
//    As of Maestro V4.0.0: If the message string is "beep", then the system default sound is played as an indication
//    to the user that a reward was delivered to the subject. This mechanism replaces the MAESTRODRIVER's use of the
//    onboard system speaker, which proved to cause significant AI ISR latencies on 3 of 5 Win10 workstations tested.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if message queue required servicing; FALSE if it was empty.
//
BOOL CCxRuntime::ServiceMessageQueue()
{
   if( !m_bDriverOn ) return( FALSE );                               // CXDRIVER not running!
   ASSERT( m_pShm != NULL );

   // queue is empty. (remember: queue is circular!)
   if(m_pShm->iLastMsgPosted == m_pShm->iNextMsgToPost) return(FALSE);

   // log next message (or reward beep) on GUI and update queue index
   CString strMsg; 
   int iPost = (m_pShm->iLastMsgPosted + 1) % CXIPC_MSGQLEN;
   strMsg.Format( "[CXDRIVER] %.*s", CXIPC_MSGSZ-11, m_pShm->szMsgQ[iPost] );
   m_pShm->iLastMsgPosted = iPost;
   if(strMsg.Compare("[CXDRIVER] beep") == 0) ::MessageBeep(MB_OK);
   else ((CCntrlxApp*)AfxGetApp())->LogMessage( strMsg );

   return( TRUE );
}


//=== GetTraces =======================================================================================================
//
//    Retrieve the CNTRLX object key of the channel configuration object (CCxChannel) currently associated with the
//    traces shown in the data trace display panel.  Applies only to non-Idle operational modes.
//
//    NOTE:  In TrialMode, the trace display panel (CGraphBar) is operated in "delayed display" mode.  The preceding
//    trial's data traces are displayed statically while the current trial runs and its configured data traces are
//    updated "invisibly".  When the current trial is over, its "delayed" set of traces become the displayed set, and
//    the old displayed trace set is lost.  This means there are TWO channel configurations (they could be the same)
//    in play during a trial sequence.  This method only returns the key for the channel configuration associated with
//    the *currently displayed* traces.
//
//    ARGS:       NONE.
//
//    RETURNS:    key value (CX_NULLOBJ_KEY if data trace display facility is not in use).
//
WORD CCxRuntime::GetTraces()
{
   int iMode = GetMode();
   if( iMode == CX_TESTMODE || iMode == CX_TRIALMODE || iMode == CX_CONTMODE ) return( m_wChanKey );
   else return( CX_NULLOBJ_KEY );
}


//=== SetTraces =======================================================================================================
//
//    Change the CNTRLX channel configuration obj (CCxChannel) associated with the CNTRLX/CXDRIVER data trace facility
//    and reinitializes that facility.  The CCxChannel object defines which CNTRLX input signals are selected for
//    display, as well as display attributes for each channel trace and the y-axis range for the GUI trace display.
//
//    Up to CX_NTRACES data channels may be "watched" on the GUI display (CGraphBar) in CNTRLX.  CXDRIVER streams the
//    requested channel data through "data trace buffers" in shared memory.  This method initializes the data trace
//    facility and configures it to display data IAW the info in the specified CCxChannel object.  A pointer to the
//    object is obtained by querying the one-and-only CNTRLX document (CCxDoc).
//
//    We save a copy of the CCxChannel object internally.  This internal copy allows us to reflect changes in trace
//    attributes or the vertical range without having to reinitialize the trace display -- see OnChangeTraces().
//
//    During TestMode, all so-called "computed" channels are ignored, as they have no meaning in that mode.
//
//    Since this method sends the CX_INITTRACE command to CXDRIVER, it may BLOCK for up to ~400msec.
//
//    SPECIAL CASE:  This method only changes the channel configuration associated with the *displayed* data traces.
//    During trial sequencing, the data traces for the current trial are delayed, prepared on the fly in a second
//    data trace set.  To keep things simple, the method fails during trial sequencing in TrialMode.
//
//    ARGS:       wKey  -- [in] unique key identifying channel cfg object to associate w/ data trace facility.
//                iDur  -- [in] the time period (in # of trace samples) to be displayed in data trace GUI window.
//
//    RETURNS:    if successful, returns supplied key; else returns value of key prior to invocation.  Data trace
//                facility will be reset upon catastrophic error.
//
WORD CCxRuntime::SetTraces( const WORD wKey, const int iDur )
{
   int iMode = GetMode();                                                     // data trace facility only available in
   if( iMode!=CX_TESTMODE && iMode!=CX_TRIALMODE && iMode!=CX_CONTMODE )      // active op modes!
      return( CX_NULLOBJ_KEY );

   if( IsTrialRunning() ) return( m_wChanKey );                               // not allowed while trial running!

   if( wKey != m_wChanKey )                                                   // change channel config only if nec:
   {
      if( wKey == CX_NULLOBJ_KEY )                                            // special case:  turn off trace display
      {
         m_wChanKey = wKey;
         m_chDisplay.ClearAll();
      }
      else
      {
         CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                // get ptr to the actual chan cfg
         if( (pDoc == NULL) || (!pDoc->ObjExists( wKey )) ||                  // invalid arg -- do nothing
             (pDoc->GetObjType( wKey ) != CX_CHANCFG) )
            return( m_wChanKey );

         CCxChannel* pChan = (CCxChannel*) pDoc->GetObject( wKey );           // make a private copy of new chan cfg
         ASSERT_KINDOF( CCxChannel, pChan );
         m_chDisplay.Copy( pChan );
         m_wChanKey = wKey;

         if( iMode == CX_TESTMODE )                                           // in TestMode, don't display "computed"
         {                                                                    // chan, which don't exist in that mode
            int iCh = -1;
            CCxChannel::ChInfo chInfo;
            while( m_chDisplay.GetNextDisplayed( iCh, chInfo ) )
            {
               if( (chInfo.chType == CCxChannel::CPChan) && chInfo.bDisplay )
                  m_chDisplay.ToggleDisplay( iCh );
            }
         }
      }
   }

   ModifyTraces( TRUE, iDur );                                                // reinit trace facility
   return( m_wChanKey );
}


//=== OnChangeTraces ==================================================================================================
//
//    Update the data trace facility in response to changes in the current channel configuration object associated
//    with the trace display.  If the # of channels displayed or the identity of the channels displayed changes in any
//    way, then the trace facility will be reinitialized.  However, if only trace attributes (color, etc) or the y-axis
//    ranges are modified, then we merely update the trace display IAW those changes.  Thus, the user can change the
//    gain, offset, or color of an existing trace without having it erased!!!
//
//    If the changes do NOT affect the currently displayed traces, then nothing is done (except to update our internal
//    copy of the channel config).  Note that, in TestMode, the CNTRLX "computed" channels are ignored.
//
//    This method is the means by which other GUI elements inform the runtime controller whenever changes occur to the
//    channel configuration object, which is maintained in CCxDoc and can be altered in certain CNTRLX views.
//
//    SPECIAL CASE:  This method only changes the channel configuration associated with the *displayed* data traces.
//    During trial sequencing, the data traces for the current trial are delayed, prepared on the fly in a second
//    data trace set.  To keep things simple, the method fails during trial sequencing in TrialMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::OnChangeTraces()
{
   int iMode = GetMode();                                                     // data trace facility only available in
   if( iMode!=CX_TESTMODE && iMode!=CX_TRIALMODE && iMode!=CX_CONTMODE )      // active op modes!
      return;

   if( m_wChanKey == CX_NULLOBJ_KEY ) return;                                 // no chan cfg assoc w/ trace facility

   if( IsTrialRunning() ) return;                                             // no changes allowed while trial running

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                      // if current chan cfg no longer exists,
   if( (pDoc == NULL) || (!pDoc->ObjExists( m_wChanKey )) ||                  // reset trace display!
       (pDoc->GetObjType( m_wChanKey ) != CX_CHANCFG) )
   {
      ResetTraces();
      return;
   }

   CCxChannel* pChan = (CCxChannel*) pDoc->GetObject( m_wChanKey );           // make copy of chan cfg in its new
   ASSERT_KINDOF( CCxChannel, pChan );                                        // state...
   CCxChannel newChan;
   newChan.Copy( pChan );

   int iCh = -1;
   CCxChannel::ChInfo chInfo;
   if( iMode == CX_TESTMODE )                                                 // turn off computed chans in TestMode,
   {                                                                          // as they have no meaning in that mode
      while( newChan.GetNextDisplayed( iCh, chInfo ) )
      {
         if( (chInfo.chType == CCxChannel::CPChan) && chInfo.bDisplay )
            newChan.ToggleDisplay( iCh );
      }
   }

   BOOL bInit = FALSE;                                                        // if TRUE, we must reinit trace disp
   BOOL bMod = FALSE;                                                         // if TRUE, we must modify trace disp
                                                                              // analyze changes in channel cfg:
   if( m_chDisplay.GetNDisplay() != newChan.GetNDisplay() )                   //    #ch displayed different
      bInit = TRUE;
   else
   {
      iCh = -1;
      while( m_chDisplay.GetNextDisplayed( iCh, chInfo ) )
      {
         CCxChannel::ChInfo chInfoNew;
         newChan.GetChannel( iCh, chInfoNew );
         if( !chInfoNew.bDisplay )                                            //    set of display ch#s is different
         {
            bInit = TRUE;
            break;
         }
         if( (chInfoNew.iOffset != chInfo.iOffset) ||                         //    a trace attrib was modified
             (chInfoNew.iGain != chInfo.iGain) ||
             (chInfoNew.crDisplay != chInfo.crDisplay) )
            bMod = TRUE;
      }

      if( !(bInit || bMod) )
      {
         int yMax1, yMax2, yMin1, yMin2;
         m_chDisplay.GetDispRange( yMin1, yMax1 );
         newChan.GetDispRange( yMin2, yMax2 );
         if( (yMin1 != yMin2) || (yMax1 != yMax2) ) bMod = TRUE;              //    only y-axis range was modified
      }
   }

   m_chDisplay.Copy( &newChan );                                              // update our internal copy

   if( bInit )                                                                // update data trace display IF needed:
      ModifyTraces( TRUE, 0 );                                                //    reinit using same graph width,
   else if( bMod )
      ModifyTraces( FALSE, 0 );                                               //    or just modify some trace attrib's
}


//=== ServiceTraces ===================================================================================================
//
//    Service the Maestro/CXDRIVER data trace display facility.
//
//    Up to CX_NTRACES data channels may be "watched" on a graphical display (CGraphBar) in Maestro.  CXDRIVER streams
//    the requested channel data through "data trace buffers" in shared memory.  The buffers are circular:  when
//    CXDRIVER reaches the end of the buffers, it wraps around to the beginning and continues streaming data.  Two
//    buffer indices 'iTraceEnd' and 'iTraceDrawn' track the location **after** the last sample ready to be drawn and
//    the location of the sample to be drawn next, respectively.  When iTraceDrawn == iTraceEnd, Maestro has "caught up
//    with" CXDRIVER and there's no data waiting to be displayed.  When (iTraceEnd+1) % traceBufSize = iTraceDrawn, the
//    buffers are full and any new data is lost -- an "overflow" error.
//
//    When the # of samples ready for display >= CX_TRSEGSZ, this method passes the new samples to the Maestro data
//    trace window object (CGraphBar) for plotting.  CX_TRSEGSZ (see CXIPC.H) is set to avoid too many updates of the
//    data trace window, which can be computationally intensive.
//
//    When the "overflow" error flag is set in shared memory, CXDRIVER halts data streaming -- the data trace facility
//    is essentially halted until it is reinitialized.  In this case, we flush the buffers and report the error.  No
//    further updates to the data trace window will occur until the facility in reset.
//
//    ARGS:       NONE.
//
//    RETURNS:    1 if data trace display updated; 0 if no update; -1 if trace buffers overflowed, or other error.
//
int CCxRuntime::ServiceTraces()
{
   if( !m_bDriverOn ) return( -1 );                                        // error: CXDRIVER not on

   if( m_pShm->bTraceOverflow )                                            // error: trace buffer overflow
   {
      if( m_pShm->nTracesInUse > 0 )                                       // the overflow error has just occurred!
      {
         UpdateTraces( -1 );                                               // flush buffers & halt data streaming
         ((CCntrlxApp*)AfxGetApp())->LogMessage(                           // notify user of error
            _T("Data trace buffer overflow; tracing halted!!") );
      }
      return( -1 );
   }

   if( m_pShm->nTracesInUse > 0 )
   {
      int nReady = m_pShm->iTraceEnd - m_pShm->iTraceDrawn;                // #trace samples ready for display
      if( nReady < 0 ) nReady += CX_TRBUFSZ;                               // handle wrap-around situation

      if( nReady < CX_TRSEGSZ ) return( 0 );                               // update when >=CX_TRSEGSZ samples ready

      UpdateTraces( CX_TRSEGSZ );
      return( 1 );
   }
   else
      return( 0 );
}


//=== ModifyTraces ====================================================================================================
//
//    Reinitialize the data trace facility, or modify trace attributes or y-axes range IAW the channel configuration
//    currently associated with the data trace display.
//
//    NOTE: In TrialMode the data trace display is operated in "delayed display" mode:  so the previous trial's data
//    traces are displayed while the currently running trial's traces are prepared in the background and displayed only
//    after the trial is done.  Thus, this method is not suited for reinitializing the data trace facility prior to
//    starting a trial.  See SetupDelayedTrialTraces().
//
//    ARGS:       bInit -- [in] if TRUE, data trace facility is reinitialized; else the trace display is redrawn IAW
//                         updated trace attributes.
//                iDur  -- [in] if trace facility is to be reinitialized, this is the graph width in #samples; if arg
//                         is <= 0 or we're not reinitializing, we attempt to use the current width of trace display.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::ModifyTraces( const BOOL bInit, const int iDur )
{
   ASSERT( m_pTracePanel != NULL );                                           // the trace display should exist!

   CGraphBar::CTrace traces[CX_NTRACES];                                      // trace display attrib's in reqd format

   if( bInit )                                                                // if we're reinitializing, stop updates
      m_pShm->nTracesInUse = 0;                                               // on the CXDRIVER side now

   int nDisp = 0;                                                             // # of channel traces to display
   int nMax = ((CCntrlxApp*)AfxGetApp())->MaxTraces();                        // don't exceed max allowed by CNTRLX

   int yMin = -500;                                                           // vertical display range
   int yMax = 500;

   if( m_wChanKey != CX_NULLOBJ_KEY )                                         // if channel cfg object specified,
   {                                                                          // extract info for all displayed chans,
      int iNext = -1;                                                         // converting to required format...
      CCxChannel::ChInfo chInfo;
      while( m_chDisplay.GetNextDisplayed( iNext, chInfo ) )
      {
         int iType = CX_AITRACE;                                              //    channel type
         if( chInfo.chType == CCxChannel::CPChan ) iType = CX_CPTRACE;
         else if( chInfo.chType == CCxChannel::DIChan ) iType = CX_DITRACE;
         m_pShm->iTraceType[nDisp] = iType;

         m_pShm->iTraceCh[nDisp] = chInfo.nCh;                                //    physical channel #

         int g = 0;                                                           //    trace gain: 0 for DI chans; else
         if( iType != CX_DITRACE )                                            //    g'= 2^G for G>=0, g'= -2^(-G) for
         {                                                                    //    G<0 (CGraphBar uses only integral
            g = (chInfo.iGain < 0) ? -chInfo.iGain : chInfo.iGain;            //    gains, where a negative gain is
            int gAct = 1;                                                     //    taken as a dividing factor)
            while( g-- > 0 ) gAct *= 2;
            g = (chInfo.iGain < 0) ? -gAct : gAct;
         }
         traces[nDisp].iGain = g;

         traces[nDisp].iOffset = int( float(chInfo.iOffset) / 4.8828f );      //    trace offset -- convert from mV to
                                                                              //    b2sAIVolts -- SHOULD get scale
                                                                              //    factor from CXDRIVER!!!!

         traces[nDisp].color = chInfo.crDisplay;                              //    trace color

         ++nDisp;                                                             //    move to next available trace slot
         if( nDisp == nMax ) break;
      }

      m_chDisplay.GetDispRange( yMin, yMax );                                 // desired vertical display range;
      yMin = int( float(yMin) / 4.8828f );                                    // again: mV-->b2sAIVolts (12bit assumed)
      yMax = int( float(yMax) / 4.8828f );
   }

   int w = iDur;                                                              // x-axis extent -- use existing extent
   if( (w <= 0) || !bInit ) w = m_pTracePanel->GetDisplayedGraphWidth();      // if not reinit'ing or arg <= 0.
   if( w < 200 ) w = 200;

   if( bInit )                                                                // reinitialize data trace facility:
   {
      int sampIntvMS = 1;
      if( GetMode() == ContMode ) sampIntvMS = 2;
      BOOL bOk = m_pTracePanel->InitGraph(                                    //    initialize the trace window
                     yMin, yMax, 0, w, sampIntvMS, nDisp, &(traces[0]) );
      m_pShm->nTracesInUse = (bOk) ? nDisp : 0;                               //    tell CXDRIVER to reinit data traces
      DWORD dwCmd = CX_INITTRACE;
      if( !SendCommand( dwCmd, NULL, NULL, 0, 0, 0, 0 ) )
      {
         TRACE1( "CX_INITTRACE failed, command error %d\n", dwCmd );
         m_pShm->nTracesInUse = 0;                                            //    if cmd fails, turn off all traces
         bOk = FALSE;
      }

      if( !bOk )                                                              //    reset trace facility on failure
      {
         m_pTracePanel->InitGraph( -100, 100, 0, 200, 1, 0, NULL );
         m_wChanKey = CX_NULLOBJ_KEY;
         m_chDisplay.ClearAll();
      }
   }
   else                                                                       // otherwise, just update trace attrib
   {                                                                          // and y-axis range
      ASSERT( nDisp == m_pTracePanel->GetDisplayedTraceCount() );
      m_pTracePanel->ModifyGraph( yMin, yMax, &(traces[0]) );
   }
}


//=== UpdateTraces ====================================================================================================
//
//    Display the specified # of samples in the data trace buffers, starting at the current index of the "next sample
//    to be drawn".  If the # of samples is negative, the buffers are flushed and the data trace facility is halted.
//    See ServiceTraces() for more information re the Maestro/CXDRIVER data trace facility.
//
//    ARGS:       nSamples -- [in] # of samples to be displayed, starting at current buffer index.  if < 0, the data
//                            trace facility is halted and the trace buffers are flushed to the display.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::UpdateTraces( const int nSamples )
{
   ASSERT( m_bDriverOn && (m_pTracePanel != NULL) );
   ASSERT( nSamples <= CX_TRBUFSZ );

   if( m_pShm->nTracesInUse > 0 )                                       // if there are data tracing is on:
   {
      int nTraces = m_pShm->nTracesInUse;
      int nDraw = nSamples;
      if( nDraw < 0 )                                                   // in this case...
      {
         m_pShm->nTracesInUse = 0;                                      // ...halt data trace facility
         nDraw = m_pShm->iTraceEnd - m_pShm->iTraceDrawn;               // ...and display remaining data
         if( nDraw < 0 ) nDraw += CX_TRBUFSZ;
      }

      int nRem = 0;                                                     // two parts if wrapping around...
      if( (m_pShm->iTraceDrawn + nDraw) >= CX_TRBUFSZ )
      {
         nRem = nDraw;
         nDraw = CX_TRBUFSZ - m_pShm->iTraceDrawn;
         nRem -= nDraw;
      }

      short* ppshBuf[CX_NTRACES];                                       // ptrs to the data segment for each trace

      int i;                                                            // draw the new data segments, in two pieces if
      for( i = 0; i < nTraces; i++ )                                    // wrapped around the trace buffers...
         ppshBuf[i] = &(m_pShm->shTraceBuf[i][m_pShm->iTraceDrawn]);
      m_pTracePanel->UpdateGraph( nDraw, ppshBuf );
      m_pShm->iTraceDrawn = (m_pShm->iTraceDrawn + nDraw) % CX_TRBUFSZ;
      if( nRem > 0 )
      {
         for( i = 0; i < nTraces; i++ ) ppshBuf[i] = &(m_pShm->shTraceBuf[i][0]);
         m_pTracePanel->UpdateGraph( nRem, ppshBuf );
         m_pShm->iTraceDrawn = nRem;
      }
   }

   if( nSamples < 0 && GetMode()==CX_TRIALMODE )                        // in TrialMode, show the delayed trace set as
   {                                                                    // soon as we halt data tracing -- WE DO THIS
      m_pTracePanel->ShowDelayedTraces();                               // EVEN IF THE DELAYED TRACE SET IS EMPTY!
      m_wChanKey = m_wNextChanKey;                                      //    the "delayed" trace set now becomes the
      m_chDisplay.Copy( &m_chNext );                                    //    "currently displayed" trace set
   }
}


//=== SetupDelayedTrialTraces =========================================================================================
//
//    Similar to SetTraces(), except this puts the data trace display in "delayed display" mode, which is used only in
//    TrialMode.  In this special mode, the traces for the previous trial remain on the display while the data for the
//    new trial is fed into the data trace display in the the background.  When the trial is done, the "delayed" trace
//    set replaces the old displayed trace set.
//
//    In the delayed mode, the data trace display spans a specific time interval [t0..t1] that may represent only a
//    portion of the trial presented!
//
//    ARGS:       wKey  -- [in] unique key identifying channel cfg object to associate w/ data trace facility.
//                lbl   -- [in] label associated with data to be collected. This will be the trial name.
//                t0,t1 -- [in] the interval of time (in # of trace samples) to be displayed in the data trace GUI
//                         window.  The trace display object CGraphBar maintains a notion of the current time and will
//                         ignore all data samples outside this interval -- but only in the "delayed" mode.  Method
//                         will correct values to ensure 0<=t0<t1 and that t1-t0 >= 200.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::SetupDelayedTrialTraces(WORD wKey, LPCTSTR lbl, int t0, int t1)
{
   ASSERT( m_pTracePanel != NULL );                                        // the trace display should exist!
   ASSERT( GetMode()==CX_TRIALMODE && !IsTrialRunning() );                 // only in Trial mode, between trials!

   if( wKey == CX_NULLOBJ_KEY )                                            // special case: the delayed trace set will
   {                                                                       // be empty!
      m_wNextChanKey = wKey;
      m_chNext.ClearAll();
   }
   else
   {
      CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                // get ptr to the actual chan cfg
      if( (pDoc == NULL) || (!pDoc->ObjExists( wKey )) ||                  // invalid arg -- again delayed trace set
          (pDoc->GetObjType( wKey ) != CX_CHANCFG) )                       // will be empty
      {
         m_wNextChanKey = CX_NULLOBJ_KEY;
         m_chNext.ClearAll();
      }
      else                                                                 // else, make a private copy of the chan cfg
      {
         CCxChannel* pChan = (CCxChannel*) pDoc->GetObject( wKey );
         ASSERT_KINDOF( CCxChannel, pChan );
         m_chNext.Copy( pChan );
         m_wNextChanKey = wKey;
      }
   }


   CGraphBar::CTrace traces[CX_NTRACES];                                   // trace display attrib's in reqd format

   m_pShm->nTracesInUse = 0;                                               // stops any trace updates by CXDRIVER

   int nDisp = 0;                                                          // # of channel traces to display
   int nMax = ((CCntrlxApp*)AfxGetApp())->MaxTraces();                     // don't exceed max allowed by CNTRLX

   int yMin = -500;                                                        // vertical display range
   int yMax = 500;

   if( m_wNextChanKey != CX_NULLOBJ_KEY )                                  // if channel cfg object specified,
   {                                                                       // extract info for all displayed chans,
      int iNext = -1;                                                      // converting to required format...
      CCxChannel::ChInfo chInfo;
      while( m_chNext.GetNextDisplayed( iNext, chInfo ) )
      {
         int iType = CX_AITRACE;                                           //    channel type
         if( chInfo.chType == CCxChannel::CPChan ) iType = CX_CPTRACE;
         else if( chInfo.chType == CCxChannel::DIChan ) iType = CX_DITRACE;
         m_pShm->iTraceType[nDisp] = iType;

         m_pShm->iTraceCh[nDisp] = chInfo.nCh;                             //    physical channel #

         int g = 0;                                                        //    trace gain: 0 for DI chans; else
         if( iType != CX_DITRACE )                                         //    g'= 2^G for G>=0, g'= -2^(-G) for
         {                                                                 //    G<0 (CGraphBar uses only integral
            g = (chInfo.iGain < 0) ? -chInfo.iGain : chInfo.iGain;         //    gains, where a negative gain is
            int gAct = 1;                                                  //    taken as a dividing factor)
            while( g-- > 0 ) gAct *= 2;
            g = (chInfo.iGain < 0) ? -gAct : gAct;
         }
         traces[nDisp].iGain = g;

         traces[nDisp].iOffset = int( float(chInfo.iOffset) / 4.8828f );   //    trace offset -- convert from mV to
                                                                           //    b2sAIVolts -- SHOULD get scale
                                                                           //    factor from CXDRIVER!!!!

         traces[nDisp].color = chInfo.crDisplay;                           //    trace color

         ++nDisp;                                                          //    move to next available trace slot
         if( nDisp == nMax ) break;
      }

      m_chNext.GetDispRange( yMin, yMax );                                 // desired vertical display range;
      yMin = int( float(yMin) / 4.8828f );                                 // again: mV-->b2sAIVolts (12bit assumed)
      yMax = int( float(yMax) / 4.8828f );
   }

   if( t1<t0 )                                                             // auto-correct time interval to ensure
   {                                                                       // that 0 <= t0 < t1 and (t1-t0) >= 200.
      int tmp = t0;
      t0 = t1;
      t1 = tmp;
   }
   if( t0 < 0 ) t0 = 0;
   if( (t0 + 200) > t1 ) t1 = t0 + 200;

                                                                           // reinitialize data trace facility:
   BOOL bOk = m_pTracePanel->InitGraph(                                    //    init data trace display panel in
            yMin, yMax, t0, t1, 1, nDisp, &(traces[0]), lbl, TRUE );       //    "delayed" display" mode

   m_pShm->nTracesInUse = (bOk) ? nDisp : 0;                               //    tell CXDRIVER to reinit data traces
   DWORD dwCmd = CX_INITTRACE;
   if( !SendCommand( dwCmd, NULL, NULL, 0, 0, 0, 0 ) )
   {
      TRACE1( "CX_INITTRACE failed, command error %d\n", dwCmd );
      m_pShm->nTracesInUse = 0;                                            //    if cmd fails, turn off all traces
      bOk = FALSE;
   }

   if( !bOk )                                                              //    reset trace facility on failure
   {
      m_pTracePanel->InitGraph( -100, 100, 0, 200, 1, 0, NULL, NULL, TRUE );
      m_wNextChanKey = CX_NULLOBJ_KEY;
      m_chNext.ClearAll();
   }
}


//=== StartEventStream ================================================================================================
//
//    Reinitialize streaming of digital event data from CXDRIVER to Maestro.  Since digital event streaming is
//    currently supported only in Trial mode, this method will fail if Maestro is in any other op mode.  It will also
//    fail if called while a trial is running.
//
//    The method will BLOCK waiting for CXDRIVER to respond to the IPC command to reinitialize the event streaming
//    facility.  In response to that command, CXDRIVER resets the event stream buffers and, if necessary, clears the
//    relevant overflow error flag.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if event streaming was started; FALSE otherwise (not in Trial mode, or trial running).
//
BOOL CCxRuntime::StartEventStream()
{
   if( (GetMode()!=CX_TRIALMODE) || IsTrialRunning() )            // event streaming only supported in Trial mode.
      return( FALSE );                                            // Cannot reinitialize while a trial is running!

   m_pShm->bEventEnable = FALSE;                                  // disable streaming momentarily

   DWORD dwCmd = CX_INITEVTSTREAM;                                // tell CXDRIVER to reinit event stream buffers; if
   if( SendCommand( dwCmd, NULL, NULL, 0, 0, 0, 0 ) )             // command succeeded, then reenable streaming
      m_pShm->bEventEnable = TRUE;
   else
      TRACE1( "CX_INITEVTSTREAM failed, command error %d\n", dwCmd );

   return( m_pShm->bEventEnable );
}


//=== ServiceEventStream ==============================================================================================
//
//    Service the digital event stream delivered by CXDRIVER through shared memory IPC.
//
//    The event stream was introduced so that Maestro can build and display spike time histograms for any tagged
//    sections defined on trials sequenced during Trial mode.  "Spike" events are assumed to occur only on digital
//    input channel DI0, but this facility provides event timestamps for all of the digital inputs monitored in
//    Maestro.
//
//    When the # of events ready for consumption >= CX_EVTCHUNKSZ, this method calls UpdateEventStream() to consume
//    the new events and pass them on to the histogram display panel.  If the relevant shared memory buffer overflows
//    because CCxRuntime failed to keep pace, CXDRIVER halts event streaming and this method flushes the buffer and
//    reports the error.  The event streaming facility must then be reinitialized to resume streaming.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE if event stream buffers overflowed, or other error.
//
BOOL CCxRuntime::ServiceEventStream()
{
   if( !m_bDriverOn ) return( FALSE );                                     // error: CXDRIVER not on

   if( m_pShm->bEventOverflow )                                            // error: event buffer overflow
   {
      if( m_pShm->bEventEnable )                                           // the overflow error has just occurred!
      {
         UpdateEventStream( -1 );                                          // flush buffers & stop event streaming
         ((CCntrlxApp*)AfxGetApp())->LogMessage(                           // notify user of error
            _T("ERROR: Digital event buffer overflow!!") );
      }
      return( FALSE );
   }

   if( m_pShm->bEventEnable )
   {
      int nReady = m_pShm->iEventEnd - m_pShm->iEventConsumed;             // #events ready for consumption
      if( nReady < 0 ) nReady += CX_EVTBUFSZ;                              // handle wrap-around situation

      if( nReady >= CX_EVTCHUNKSZ )                                        // update when enough events are ready
         UpdateEventStream( CX_EVTCHUNKSZ );
   }

   return( TRUE );
}

//=== UpdateEventStream ===============================================================================================
//
//    Consume the specified number of events from the event stream buffers in CXDRIVER shared-memory IPC and deliver
//    them to the spike histogram display facility in Maestro. If the number of events is negative, event streaming is
//    disabled, and all events left in the buffers are consumed.
//
//    ARGS:       nEvents  -- [in] # of events to be consumed, starting at current buffer index.  If < 0, event
//                            streaming is halted and the buffers are flushed.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::UpdateEventStream( const int nEvents )
{
   ASSERT( nEvents <= CX_EVTBUFSZ );

   if( m_pShm->bEventEnable )                                                 // if event streaming is enabled:
   {
      int nConsume = nEvents;
      if( nConsume < 0 )                                                      // in this case...
      {
         m_pShm->bEventEnable = 0;                                            // ...halt event streaming
         nConsume = m_pShm->iEventEnd - m_pShm->iEventConsumed;               // ...and consume all remaining events
         if( nConsume < 0 ) nConsume += CX_EVTBUFSZ;
      }

      int nRem = 0;                                                           // two parts if wrapping around...
      if( (m_pShm->iEventConsumed + nConsume) >= CX_EVTBUFSZ )
      {
         nRem = nConsume;
         nConsume = CX_EVTBUFSZ - m_pShm->iEventConsumed;
         nRem -= nConsume;
      }

      if( m_pHistPanel != NULL )                                              // if spike histogram panel available,
         m_pHistPanel->ConsumeSpikes( nConsume,                               // feed it the digital events
            &(m_pShm->dwEventMaskBuf[m_pShm->iEventConsumed]),
            &(m_pShm->iEventTimeBuf[m_pShm->iEventConsumed]) );

      m_pShm->iEventConsumed =                                                // update where we are in event buffers
         (m_pShm->iEventConsumed + nConsume) % CX_EVTBUFSZ;

      if( nRem > 0 )                                                          // handle second part if wrap around
      {

         if( m_pHistPanel != NULL )
            m_pHistPanel->ConsumeSpikes( nConsume, &(m_pShm->dwEventMaskBuf[0]), &(m_pShm->iEventTimeBuf[0]) );
         m_pShm->iEventConsumed = nRem;
      }
   }
}


//=== ServicePositionPlot =============================================================================================
//
//    Update GUI's eye/target position plot with new position data as needed.  This NON-BLOCKING method services a new
//    plot update request and completes the req/ack handshake for that request on separate calls.
//
//    NOTE:  Since we install loci symbols in the plot panel in the same order in which they're defined in the IPC
//    block, and since the plot extents are in the same units with which loci are reported in IPC, the update here is
//    very straightforward.  See ConfigurePositionPlot().
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if position plot request was serviced; FALSE otherwise.
//
BOOL CCxRuntime::ServicePositionPlot()
{
   if( (m_pPlotPanel == NULL) || !m_bDriverOn ) return( FALSE );
   ASSERT( m_pShm != NULL );

   if( m_pShm->bReqPlot && !m_pShm->bAckPlot )                    // service new request
   {
      m_pShm->ptLoci[CX_CHAIRPOS].y = -2450;                      // chair symbol always near bottom of display
      m_pPlotPanel->UpdateSymbols( &(m_pShm->ptLoci[0]) );
      m_pShm->bAckPlot = TRUE;
      return( TRUE );
   }
   else if( m_pShm->bAckPlot && !m_pShm->bReqPlot )               // complete handshake for a previous request
   {
      m_pShm->bAckPlot = FALSE;
      return( TRUE );
   }

   return( FALSE );
}

//=== GetCursorInPositionPlot =========================================================================================
//
//    If mouse cursor is currently within the client area of the eye-target position plot panel, this method returns
//    its coordinates in degrees.
//
//    [This method was introduced to facilitate tracking the cursor position with a visual target in Cont mode.]
//
//    ARGS:       fx,fy -- [out] IF function returns TRUE, these give the x,y coords of mouse cursor in visual degrees;
//                         if function returns FALSE, they are undefined.
//
//    RETURNS:    TRUE if mouse cursor is currently inside the eye-target plot panel's client area; FALSE otherwise.
//
BOOL CCxRuntime::GetCursorInPositionPlot( float& fx, float& fy )
{
   ASSERT( m_pPlotPanel );
   CPoint pt;
   BOOL bInside = m_pPlotPanel->GetCursorLogicalPos( pt );
   if( bInside )
   {
      fx = float(pt.x)/float(100);                                                  // logical coords of pos plot are
      fy = float(pt.y)/float(100);                                                  // in hundredth-deg in x & y
   }
   return( bInside );
}

//=== ConfigurePositionPlot ===========================================================================================
//
//    Install symbols for CNTRLX loci (eye position, secondary eye, fixation target 1/2, chair, "cursor tracking tgt")
//    in the position plot panel and set up the plot extents over which moving loci will be animated during runtime.
//    By design, CXDRIVER reports loci in hundredth-degrees of visual field.  We set the plot extents using these same
//    units so that we can use the CXDRIVER loci without transformation; +/-25deg visual field is displayed, with the
//    origin at the center.  Since the plot bar provides 10 divisions per axis, each division corresponds to 5 deg.
//
//    The symbols for eye position and fixation target 1 are initially turned ON here.  Users can change which targets
//    are on/off by interacting with the position plot panel itself.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::ConfigurePositionPlot()
{
   ASSERT( m_pPlotPanel );
   ASSERT( m_pPlotPanel->GetNumSymbols() == 0 );

   m_pPlotPanel->AddSymbol( CXYPlotBar::FILLBOX, RGB(255,255,255), _T("Eye") );       // note:  order identical to order
   m_pPlotPanel->AddSymbol( CXYPlotBar::FILLBOX, RGB(255,0,255), _T("Eye2") );          //    in CXDRIVER IPC array!
   m_pPlotPanel->AddSymbol( CXYPlotBar::BOX, RGB(255,255,0), _T("Fix 1") );
   m_pPlotPanel->AddSymbol( CXYPlotBar::BOX, RGB(255,0,0), _T("Fix 2") );
   m_pPlotPanel->AddSymbol( CXYPlotBar::FILLBOX, RGB(0,255,0), _T("Chair") );
   m_pPlotPanel->AddSymbol( CXYPlotBar::FILLBOX, RGB(0,255,255), _T("Track") );
   m_pPlotPanel->ShowSymbol( 0, TRUE );                                             // eye & fix1 symbols turned ON
   m_pPlotPanel->ShowSymbol( 2, TRUE );

   m_pPlotPanel->SetLogicalExtent( 5000, 5000 );
   m_pPlotPanel->EnableShowHide( TRUE );
}


//=== CanUpdateVideoCfg, UpdateVideoCfg, CanUpdateFixRewSettings, UpdateFixRewSettings ================================
//
//    The CNTRLX application settings object (CCxSettings) contains a variety of application settings, some of which
//    must be communicated to CXDRIVER whenever they change:
//       1) Video display configuration -- Controls the appearance of targets on the XY scope and RMVideo displays.
//       2) Fix/Reward settings -- Fixation requirements, and options governing how rewards are delivered to animal.
//    The user can modify these settings during any operational mode as long as the master mode control panel permits
//    it.  Invoke CanUpdate***() to check whether or not an update of the relevant settings is currently permissible,
//    and call Update***() to send those settings to CXDRIVER via the CX_SETDISPLAY or CX_FIXREWSETTINGS command.
//
//    ARGS:       NONE.
//
//    RETURNS:    CanUpdate***(): TRUE if update is possible in current operational state; FALSE otherwise.
//                Update***(): TRUE if update was successful; FALSE otherwise.
//
BOOL CCxRuntime::CanUpdateVideoCfg()
{
   if( (!m_bDriverOn) || m_pModePanel == NULL ) return( FALSE );  // CXDRIVER not running; update not possible
   else return( m_pModePanel->CanUpdateVideoCfg() );              // otherwise, it depends on the mode controller
}

BOOL CCxRuntime::UpdateVideoCfg()
{
   if( !CanUpdateVideoCfg() ) return( FALSE );                 // video display update not possible at this time

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();       // retrieve the applications settings from doc
   CCxSettings* pSet = (CCxSettings*) pDoc->GetSettings();
   ASSERT( pSet );

   m_pShm->iData[0] = pSet->GetXYDistToEye();                  // fill in int-valued data for CX_SETDISPLAY cmd
   m_pShm->iData[1] = pSet->GetXYWidth();
   m_pShm->iData[2] = pSet->GetXYHeight();
   m_pShm->iData[3] = pSet->GetXYDrawDelay();
   m_pShm->iData[4] = pSet->GetXYDrawDur();
   m_pShm->iData[5] = pSet->IsXYDotSeedFixed() ? 0 : 1;        // FALSE to use a fixed value for random pattern genr;
   m_pShm->iData[6] = int(pSet->GetFixedXYDotSeedValue());     // else a new seed is auto-generated for each animation

   m_pShm->iData[7] = pSet->GetFBDistToEye();
   m_pShm->iData[8] = pSet->GetFBWidth();
   m_pShm->iData[9] = pSet->GetFBHeight();
   m_pShm->iData[10] = pSet->GetFBBkgRed();
   m_pShm->iData[11] = pSet->GetFBBkgGrn();
   m_pShm->iData[12] = pSet->GetFBBkgBlu();
   m_pShm->iData[13] = pSet->GetRMVSyncFlashSize();            // vsync spot flash feature added in Maestro 4.0.0
   m_pShm->iData[14] = pSet->GetRMVSyncFlashDuration();

   DWORD dwCmd = CX_SETDISPLAY;                                // send the command
   if( !SendCommand( dwCmd, NULL, NULL, 0,0,0,0, 250 ) )
   {
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("(!!) Unable to update video displays!") );
      return( FALSE );
   }

   return( TRUE );
}


BOOL CCxRuntime::CanUpdateFixRewSettings()
{
   if( (!m_bDriverOn) || m_pModePanel == NULL ) return( FALSE );  // CXDRIVER not running; update not possible
   else return( m_pModePanel->CanUpdateFixRewSettings() );        // otherwise, it depends on the mode controller
}

BOOL CCxRuntime::UpdateFixRewSettings()
{
   if( !CanUpdateFixRewSettings() ) return( FALSE );           // fix/reward settings may not be updated at this time

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();       // retrieve the applications settings from doc
   CCxSettings* pSet = (CCxSettings*) pDoc->GetSettings();
   ASSERT( pSet );

   m_pShm->iData[0] = pSet->GetFixDuration();                  // fill in data for CX_FIXREWSETTINGS cmd
   m_pShm->iData[1] = pSet->GetScaledRewardPulseLen(pSet->GetRewardLen1());
   m_pShm->iData[2] = pSet->GetScaledRewardPulseLen(pSet->GetRewardLen2());
   m_pShm->iData[3] = pSet->GetVariableRatio();
   m_pShm->iData[4] = pSet->GetAudioRewardLen();
   m_pShm->iData[5] = (pSet->IsRewardBeepEnabled()) ? 1 : 0;
   m_pShm->fData[0] = pSet->GetFixAccH();
   m_pShm->fData[1] = pSet->GetFixAccV();

   DWORD dwCmd = CX_FIXREWSETTINGS;                            // send the command
   if( !SendCommand( dwCmd, NULL, NULL, 0,0,0,0, 250) )
   {
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("(!!) Unable to update fixation/reward settings!") );
      return( FALSE );
   }

   return( TRUE );
}


// 
// Support for RMVideo display modes, monitor gamma correction, and "media store" (added Sep 2009 -- sar)
//

/**
 * Helper method for Start(). Issues the necessary commands to retrieve all available RMVideo display modes, the
 * currently selected mode, and the current monitor gamma-correction factors. The retrieved information is stored 
 * internally for quick access. If any command fails, a message is posted to the message queue by MaestroDriver, so no 
 * additional error message is posted here. No action taken if RMVideo is not available.
 */
VOID CCxRuntime::RetrieveRMVideoDisplayData()
{
   if((GetHWStatus() & CX_F_RMVAVAIL) == 0) return;
   
   ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("Requesting RMVideo display info. PLEASE WAIT...") );
   
   // get all available display modes
   m_nModes = 0;
   int respBuf[1 + RMV_MAXVMODES*3];
   DWORD dwCmd = CX_RMV_GETMODES;
   if(SendCommand(dwCmd, respBuf, NULL, 0, 0, 1 + RMV_MAXVMODES*3, 0, 300) && respBuf[0] > 0 &&  
         respBuf[0] <= RMV_MAXVMODES)
   {
      m_nModes = respBuf[0];
      int j = 1;
      for(int i=0; i<m_nModes && i<RMV_MAXVMODES; i++)
      {
         RMVideoMode* pMode = &(m_rmvModes[i]);
         pMode->w = respBuf[j++];
         pMode->h = respBuf[j++];
         pMode->rate = respBuf[j++];
      }
   }
   
   if(m_nModes > 0)
   {
      m_iCurrMode = -1;
      m_rmvFrameRate = 0.0f;
      dwCmd = CX_RMV_GETCURRMODE;
      SendCommand(dwCmd, &m_iCurrMode, &m_rmvFrameRate, 0, 0, 1, 1, 300);
      if(m_iCurrMode < 0 || m_iCurrMode >= m_nModes)
      {
         m_iCurrMode = -1;
         m_rmvFrameRate = 0.0f;
         m_nModes = 0;
      }
   }
   
   if(m_nModes == 0)
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("WARNING: Unable to read RMVideo display modes!!") );

   dwCmd = CX_RMV_GETGAMMA;
   if(!SendCommand(dwCmd, NULL, m_rmvGamma, 0, 0, 0, 3, 300))
   {
      for(int i=0; i<3; i++) m_rmvGamma[i] = 1.0f;
   }
}

/**
 * Helper method for Start(). Issues the necessary commands to retrieve the table of contents for the RMVideo "media
 * store", in which all video and image files are stored on the RMVideo server machine. The table of contents is 
 * stored and maintained by CCxRuntime for quick access. If any command fails, a message is posted to the message queue
 * by MaestroDriver, so no additional error message is posted here. No action taken if RMVideo is not available.
 */
VOID CCxRuntime::RetrieveRMVideoMediaStoreTOC()
{
   if((GetHWStatus() & CX_F_RMVAVAIL) == 0) return;
   
   ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("Retrieving RMVideo media store contents. PLEASE WAIT...") );
   
   char strBuf[CX_CDATALEN];
   
   // empty the cached TOC
   m_nMediaFolders = -1;
   
   // get the names of all folders in the media store. On failure, give up and warn user.
   DWORD dwCmd = CX_RMV_GETMDIRS;
   if(!SendCommand(dwCmd, &m_nMediaFolders, NULL, 0, 0, 1, 0, 300, strBuf, 0, CX_CDATALEN))
   {
      m_nMediaFolders = -1;
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("WARNING: Unable to retrieve RMVideo media store content (1)!!") );
      return;
   }
   else 
   {
      size_t n = 0;
      for(int i = 0; i<m_nMediaFolders; i++)
      {
         ::strcpy_s(m_mediaFolders[i].name, &(strBuf[n]));
         size_t len = ::strlen(&(strBuf[n]));
         n += (len+1);
      }
   }
   
   // for each folder in the media store, get the names and summary info on each media file in that folder. On failure, 
   // clear the cached TOC and warn user.
   for(int i=0; i<m_nMediaFolders; i++)
   {
      RMVMediaFolder* pFolder = &(m_mediaFolders[i]);
      ::strcpy_s(strBuf, pFolder->name);
      int nFiles = 0;
      int nameLen = static_cast<int>(::strlen(pFolder->name)) + 1;
      dwCmd = CX_RMV_GETMFILES;
      if(!SendCommand(dwCmd, &nFiles, NULL, 0, 0, 1, 0, 300, strBuf, nameLen, CX_CDATALEN))
      {
         m_nMediaFolders = -1;
         ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("WARNING: Unable to retrieve RMVideo movie store content (2)!!") );
         return;
      }
      
      pFolder->nFiles = nFiles;
      size_t n = 0;
      int j;
      for(j=0; j<nFiles; j++)
      {
         ::strcpy_s(pFolder->files[j].name, &(strBuf[n]));
         size_t len = ::strlen(&(strBuf[n]));
         n += (len+1);
      }
      
      int iData[2];
      float fData[2];
      for(j=0; j<nFiles; j++)
      {
         RMVMediaFile* pMedia = &(pFolder->files[j]);
         ::sprintf_s(strBuf, "%s%c%s", pFolder->name, (char) '\0', pMedia->name);
         int len = static_cast<int>(::strlen(pFolder->name) + ::strlen(pMedia->name)) + 2;
         dwCmd = CX_RMV_GETMFINFO;
         if(!SendCommand(dwCmd, iData, fData, 0, 0, 2, 2, 300, strBuf, len, 0))
         {
            m_nMediaFolders = -1;
            ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("WARNING: Unable to retrieve RMVideo movie store content (3)!!") );
            return;
         }
         pMedia->width = iData[0];
         pMedia->height = iData[1];
         pMedia->rate = fData[0];
         pMedia->dur = fData[1];
      }
   }
}

/**
 * Get current width of the RMVideo display.
 * @return The RMVideo display width in pixels; 0 if RMVideo is unavailable.
 */
int CCxRuntime::GetRMVideoScreenW() const { return((m_iCurrMode < 0) ? 0 : m_rmvModes[m_iCurrMode].w); }

/**
 * Get current height of the RMVideo display.
 * @return The RMVideo display height in pixels; 0 if RMVideo is unavailable.
 */
int CCxRuntime::GetRMVideoScreenH() const { return((m_iCurrMode < 0) ? 0 : m_rmvModes[m_iCurrMode].h); }

/**
 * Get current frame rate of RMVideo display. This rate is measured over a 500-frame period at startup and whenever 
 * the display mode is changed.
 * @return The RMVideo frame rate in Hz; 0 if RMVideo is unavailable.
 */
float CCxRuntime::GetRMVideoFrameRate() const { return(m_rmvFrameRate); }

/**
 * Get number of alternate video modes available for the RMVideo display. All modes will meet or exceed 1024x768 @ 75Hz.
 * @return Number of available video modes; 0 if RMVideo is unavailable.
 */
int CCxRuntime::GetNumRMVideoModes() const { return(m_nModes); }

/**
 * Get a brief description (eg, "1024x768 @ 75Hz") of a specified video mode in RMVideo. 
 * @param i Zero-based index of the video mode requested.
 * @param desc The requested description. Will be set to an empty string if index is invalid or RMVideo is unavailable.
 * @return TRUE if successful, FALSE otherwise (bad index or RMVideo not available).
 */
BOOL CCxRuntime::GetRMVideoModeDesc(int i, CString& desc) const
{
   desc = "";
   if(i < 0 || i >= m_nModes) return(FALSE);
   const RMVideoMode* pMode = &(m_rmvModes[i]);
   desc.Format("%dx%d @ %dHz", pMode->w, pMode->h, pMode->rate);
   return(TRUE);
}

/**
 * Get the current video mode on the RMVideo display.
 * @return Zero-based index of the current video mode; -1 if RMVideo is unavailable.
 */
int CCxRuntime::GetCurrRMVideoMode() const { return(m_iCurrMode); }

/**
 * Change the current video mode on the RMVideo display. Since RMVideo always remeasures the frame rate -- over a
 * 500-frame interval -- after a mode switch, this operation BLOCKS for up to 10 seconds. The caller is responsible
 * for notifying the user of the extended delay (eg: wait cursor, modal wait dialog). This method may only be called
 * in IdleMode.
 * @param i Zero-based index of the target mode. Must be in [0..N-1], where N is the number of available video modes.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL CCxRuntime::SetCurrRMVideoMode(int i)
{
   if((GetMode() != CX_IDLEMODE) || ((GetHWStatus() & CX_F_RMVAVAIL) == 0) || (i < 0) || (i >= m_nModes)) 
      return(FALSE);
   
   if(i == m_iCurrMode) return(TRUE);
   
   int index = i;
   float fRate = 0.0f;
   DWORD dwCmd = CX_RMV_SETCURRMODE;
   if(SendCommand(dwCmd, &index, &fRate, 1, 0, 0, 1, 10000))
   {
      m_iCurrMode = i;
      m_rmvFrameRate = fRate;
      return(TRUE);
   }
   return(FALSE);
}
   
/**
 * Get the current gamma-correction factors for the RMVideo monitor.
 * @param r,g,b [out] The red, green, and blue gamma-correction factors.
 * @return TRUE if successful, FALSE otherwise (RMVideo not available).
 */
BOOL CCxRuntime::GetRMVGamma(float& r, float& g, float& b) const
{
   if((GetHWStatus() & CX_F_RMVAVAIL) == 0) return(FALSE);
   r = m_rmvGamma[0];
   g = m_rmvGamma[1];
   b = m_rmvGamma[2];
   return(TRUE);
}

/**
 * Set the current gamma-correction factors for the RMVideo monitor. May only be called in Idle Mode.
 * @param r,g,b [in] The requested red, green, and blue gamma-correction factors; [out] the actual values set, which 
 * will be range-limited to [0.800..3.000] with 3 digits precision.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL CCxRuntime::SetRMVGamma(float& r, float& g, float& b)
{
   if((GetMode() != CX_IDLEMODE) || ((GetHWStatus() & CX_F_RMVAVAIL) == 0)) return(FALSE);
   if((m_rmvGamma[0] == r) && (m_rmvGamma[1] == g) && (m_rmvGamma[2] == b)) return(TRUE);
   
   float fData[3];
   fData[0] = r;
   fData[1] = g;
   fData[2] = b;
   
   DWORD dwCmd = CX_RMV_SETGAMMA;
   if(SendCommand(dwCmd, NULL, fData, 0, 3, 0, 3, 300))
   {
      for(int i=0; i<3; i++) m_rmvGamma[i] = fData[i];
      r = fData[0];
      g = fData[1];
      b = fData[2];
      return(TRUE);
   }
   return(FALSE);
}


/**
 * Get total number of folders currently defined in RMVideo's "media store" -- where files are kept that serve as a 
 * video or image source for the RMV_MOVIE or RMV_IMAGE target classes, respectively. Individual files are 
 * identified by a parent folder and the media file name.
 * @return Number of folders in media store. 0 if store is empty. -1 if RMVideo is unavailable, or a problem occurred 
 * while retrieving media store contents at startup.
 */
int CCxRuntime::GetNumRMVMediaFolders() const { return(m_nMediaFolders); }

/**
 * Get the name of a particular folder in RMVideo's media store.
 * @param Zero-based index of requested folder. Must be in [0..N-1], where N is the number of existing folders.
 * @param folder Set to the name of the requested folder; empty string if index invalid.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL CCxRuntime::GetRMVMediaFolder(int i, CString& folder) const
{
   BOOL bOk = (i >= 0) && (i < m_nMediaFolders);
   folder = bOk ? ((LPCTSTR) m_mediaFolders[i].name) : "";
   return(bOk);
}

/**
 * Get total number of media files within a specified folder in RMVideo's "media store".
 * @param Zero-based index of requested folder. Must be in [0..N-1], where N is the number of existing folders.
 * @return Number of files in specified folder. 0 if folder is actually empty. -1 if index is invalid.
 */
int CCxRuntime::GetNumRMVMediaFiles(int i) const 
{ 
   return((i >= 0 && i < m_nMediaFolders) ? m_mediaFolders[i].nFiles : -1); 
}

/**
 * Get information on a particular media file in RMVideo's "media store".
 * @param i Zero-based index of the folder containing the file requested. Must be in [0..N-1], where N is the number 
 * of existing folders.
 * @param j Zero-based index of the file requested within the parent folder. Must be in [0..M-1], where M is the number
 * of media files in the folder.
 * @param name [out] Set to the name of the requested media file; empty string if file not found.
 * @param desc A brief description of the media file. For a video, it lists the width W and height H of a video frame 
 * in pixels, its playback rate R in Hz, and its approximate duration D in seconds when played at that rate: "WxH; 
 * D secs at R Hz". For an image, it lists the image size in pixels: "WxH image". An empty string if media file not 
 * found. If any parameter was not found in the media file, it will appear as "0" in the descriptor string. 
 * @return TRUE if successful, FALSE otherwise (bad indices, RMVideo not available).
 */
BOOL CCxRuntime::GetRMVMediaInfo(int i, int j, CString& name, CString& desc) const
{
   name = "";
   desc = "";
   if(i < 0 || i >= m_nMediaFolders) return(FALSE);
   if(j < 0 || j >= m_mediaFolders[i].nFiles) return(FALSE);
   const RMVMediaFile* pMedia = &(m_mediaFolders[i].files[j]);
   name = (LPCTSTR) pMedia->name;
   if(pMedia->rate < 0.0f && pMedia->dur < 0.0f)
      desc.Format("%dx%d image", pMedia->width, pMedia->height);
   else
      desc.Format("%dx%d; %.3f s at %.2f Hz", pMedia->width, pMedia->height, pMedia->dur, pMedia->rate);
   return(TRUE);
}

/**
 * Delete a particular media file or an entire media folder from the RMVideo media store. If the last media file in a 
 * folder is deleted, the folder is likewise removed. Since it may take some time to delete the contents of an entire 
 * folder, this operation may BLOCK for up to 5 seconds. The caller is responsible for notifying the user of the 
 * extended delay (eg: wait cursor, modal wait dialog). This method may only be called in IdleMode.
 * @param i Zero-based index of a folder in media store. Must be in [0..N-1], where N = number of existing folders.
 * @param j If -1, the identified folder is removed. Otherwise, this zero-based index identifies the media file to be 
 * deleted within the parent folder specified. Must be in [0..M-1], where M is the number of files in the folder.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL CCxRuntime::DeleteRMVMediaFile(int i, int j)
{
   if((GetMode() != CX_IDLEMODE) || ((GetHWStatus() & CX_F_RMVAVAIL) == 0)) return(FALSE);
   if(i < 0 || i >= m_nMediaFolders) return(FALSE);
   if(j < -1 || j >= m_mediaFolders[i].nFiles) return(FALSE);
   
   char strBuf[2*(RMV_MVF_LEN+1)];
   ::memset(strBuf, 0, sizeof(char)*2*(RMV_MVF_LEN+1));
   
   int len = 0;
   int isFolderDel = 0;
   if(j == -1)
   {
      isFolderDel = 1;
      ::strcpy_s(strBuf, m_mediaFolders[i].name);
      len = static_cast<int>(::strlen(m_mediaFolders[i].name)) + 1;   // don't forget terminating null!!!!
   }
   else
   {
      ::sprintf_s(strBuf, "%s%c%s", m_mediaFolders[i].name, (char)'\0', m_mediaFolders[i].files[j].name);
      len = static_cast<int>(::strlen(m_mediaFolders[i].name) + ::strlen(m_mediaFolders[i].files[j].name)) + 2;
   }
   
   DWORD dwCmd = CX_RMV_DELMEDIA;
   BOOL bOk = SendCommand(dwCmd, &isFolderDel, NULL, 1, 0, 0, 0, 5000, strBuf, len, 0);
   if(bOk && (isFolderDel || m_mediaFolders[i].nFiles == 1))
   {
      for(int k = i+1; k < m_nMediaFolders; k++)
         ::memmove(&(m_mediaFolders[k-1]), &(m_mediaFolders[k]), sizeof(RMVMediaFolder));
      --m_nMediaFolders;
   }
   else if(bOk && !isFolderDel)
   {
      RMVMediaFolder* pFolder = &(m_mediaFolders[i]);
      for(int k = j+1; k < pFolder->nFiles; k++)
         ::memmove(&(pFolder->files[k-1]), &(pFolder->files[k]), sizeof(RMVMediaFile));
      --(pFolder->nFiles);
   }
   
   return(bOk);
}

/**
 * Download a video or image file for storage in the RMVideo media store. This method may only be called in IdleMode.
 *
 * TODO: The file download takes an indeterminate amount of time to complete. We really need a different scheme than
 * the simple command/response paradigm. For now, this operation may block for up to 120 seconds. The caller is 
 * responsible for notifying the user of the extended delay (eg: wait cursor, modal wait dialog).
 *
 * Folder and file names may only contain ASCII alphanumeric characters, the underscore or period, and they may not
 * exceed RMV_MVF_LEN characters in length.
 *
 * @param path Absolute file system pathname of the video or image file to be downloaded.
 * @param iFolder Zero-based index of the destination media folder. If -1, a new folder name is specified in the
 * 'folderNew' argument; otherwise, the folder index must be valid.
 * @param folderNew Ignored unless iFolder == -1, in which case this is the name of a new folder that will contain the 
 * downloaded media file. It must not duplicate the name of an existing folder. If the media store already contains 
 * RMV_MVF_LIMIT folders, a new folder cannot be added.
 * @param file Name of the media file within the specified folder. This must not duplicate the name of any existing 
 * file in that folder.
 * @return TRUE if successful, FALSE otherwise. The download can fail for many reasons: Bad arguments, IO error during
 * the download, media store is full (new folder cannot be added) or specified media folder is full, downloaded media
 * file cannot be read or is not recognized as a RMVideo-supported video or image file. A message is posted to the 
 * Maestro application message log.
 */
BOOL CCxRuntime::DownloadRMVMediaFile(LPCTSTR path, int iFolder, LPCTSTR folderNew, LPCTSTR file)
{
   // argument checking!!
   BOOL bOk = (GetMode() == CX_IDLEMODE) && ((GetHWStatus() & CX_F_RMVAVAIL) == CX_F_RMVAVAIL);
   if(bOk) bOk = (path != NULL) && ::strlen(path) > 0;
   if(bOk) bOk = (iFolder >= -1) && (iFolder < m_nMediaFolders);
   if(bOk && (iFolder == -1))
   {
      size_t len = (folderNew != NULL) ? ::strlen(folderNew) : 0;
      bOk = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(folderNew, RMV_MVF_CHARS));
      for(int i=0; bOk && i<m_nMediaFolders; i++) bOk = (::strcmp(folderNew, m_mediaFolders[i].name) != 0);
   }
   if(bOk)
   {
      size_t len = (file != NULL) ? ::strlen(file) : 0;
      bOk = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(file, RMV_MVF_CHARS));
      if(bOk && iFolder != -1)
      {
         RMVMediaFolder* pFolder = &(m_mediaFolders[iFolder]);
         for(int i=0; bOk && i<pFolder->nFiles; i++) bOk = (::strcmp(file, pFolder->files[i].name) != 0);
      }
   }
   if(!bOk)
   {
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("File download arguments invalid!!") );
      return(FALSE);
   }
   else if(iFolder == -1 && m_nMediaFolders == RMV_MVF_LIMIT)
   {
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("Cannot add a new folder because media store is at capacity") );
      return(FALSE);
   }
   else if(iFolder != -1 && m_mediaFolders[iFolder].nFiles == RMV_MVF_LIMIT)
   {
      ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("Cannot download media file; specified folder is full!") );
      return(FALSE);
   }

   char strBuf[CX_MAXPATH + 2*(RMV_MVF_LEN+1) + 4];
   const char* strFolder = (iFolder == -1) ? folderNew : m_mediaFolders[iFolder].name;
   ::sprintf_s(strBuf, "%s%c%s%c%s", path, (char) '\0', strFolder, (char) '\0', file);
   int n = static_cast<int>(::strlen(path) + ::strlen(strFolder) + ::strlen(file)) + 3;
   
   int iData[2];
   float fData[2];

   DWORD dwCmd = CX_RMV_PUTMEDIA;
   bOk = SendCommand(dwCmd, iData, fData, 0, 0, 2, 2, 120000, strBuf, n, 0);
   if(bOk)
   {
      // if we created a new media folder, append it to the end of the current folder list
      RMVMediaFolder* pFolder = NULL;
      if(iFolder == -1)
      {
         pFolder = &(m_mediaFolders[m_nMediaFolders]);
         ::strcpy_s(pFolder->name, folderNew);
         pFolder->nFiles = 0;
         ++m_nMediaFolders;
      }
      else pFolder = &(m_mediaFolders[iFolder]);
      
      // append the movie info to the folder's movie list
      RMVMediaFile* pMedia = &(pFolder->files[pFolder->nFiles]);
      ::strcpy_s(pMedia->name, file);
      pMedia->width = iData[0];
      pMedia->height = iData[1];
      pMedia->rate = fData[0];
      pMedia->dur = fData[1];
      ++(pFolder->nFiles);
   }
   
   return(bOk);
}


//=== LoadTargetList ==================================================================================================
//
//    Load target list in IPC with all targets participating in a trial sequence.
//
//    BACKGROUND:  Why not load trial target definitions prior to the start of each trial, rather than before the trial
//    sequence starts?  This is a vestige of how Maestro worked with the old VSG-based framebuffer video device.  Some
//    video targets (gratings and gabors, eg) could take a long to load on the VSG (>1sec).  This made the interval
//    between trials unacceptably large.  To avoid it, we had to preload all framebuffer video targets participating in
//    ANY trial of a trial set BEFORE we started sequencing trials from that set.
//
//    RMVideo was introduced in Maestro v2.0 to replace the VSG.  Targets load much faster on RMVideo, so a preload is
//    no longer necessary.  However, we still prepare the list of all targets participating in a trial sequence and
//    store that list in CXIPC.  We no longer issue the CX_TR_PRELOADFB command, which is considered obsolete.
//
//    ARGS:       pCtrl    -- [in] the trial sequencer:  we obtain all tgt defns from this object.
//
//    RETURNS:    TRUE if target list was successfully loaded, FALSE otherwise.
//
BOOL CCxRuntime::LoadTargetList( const CCxTrialSequencer *pCtrl )
{
   ASSERT( GetMode() == TrialMode );                              // may only be called during TrialMode!
   ASSERT( !IsTrialRunning() );                                   // never call while a trial is running!

   UpdateVideoCfg();                                              // make sure display params are up to date

   BOOL bOk = pCtrl->GetTargets(                                  // load defns of all tgts partic. in trial sequence:
                           &(m_pShm->nTgts),                      //    #tgts in list
                           CX_MAXTGTS,                            //    max #tgts that can be loaded
                           &(m_pShm->targets[0]) );               //    the target list itself

   return( bOk );
}

//=== StartTrial ======================================================================================================
//
//    If possible, start the next trial (already selected) in an ongoing trial sequence.  The following tasks must be
//    performed:
//       1) "Download" important trial info to the CXDRIVER runtime interface:  the "trial codes" that define it, the
//          list of participating targets, any "tagged sections" defined on the trial, the ADC channel list to be saved
//          during the trial, the trial's name, and the path name for the trial data file, if it is to be saved (path
//          must be empty otherwise).  NOTE:  For efficiency, we write directly into fields maintained within the
//          CXDRIVER shared-memory interface.
//       2) Prepare the data trace display IAW the channel configuration associated with the trial.
//       3) Prepare spike histogram display to process event stream data generated during trial.
//       4) Issue CX_TR_START command instructing CXDRIVER to execute the trial.
//
//    ARGS:       pCtrl    -- [in] the trial sequencer:  handles all details of sequencing trials from a trial set,
//                            including preparation of trial codes and trial targets.  we invoke methods on this object
//                            to obtain all necessary trial info.
//                strData  -- [in] if not empty, the pathname to which the trial data file should be saved.
//                spikesOn -- [in] if TRUE, high-res spike trace is also recorded and saved in trial data file; this
//                            flag is ignored if no trial data file will be saved for this file!
//
//    RETURNS:    TRUE if trial was successfully started, FALSE otherwise.
//
BOOL CCxRuntime::StartTrial( CCxTrialSequencer* pCtrl, LPCTSTR strData, BOOL bSpikesOn )
{
   ASSERT( GetMode() == TrialMode );                              // may only be called during TrialMode!
   ASSERT( !IsTrialRunning() );                                   // never call while a trial is running!
   ASSERT( pCtrl->GetCurrentTrialKey() != CX_NULLOBJ_KEY );       // next trial to run must be defined!

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                  // for posting messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // for accessing trial & channel config objects
   WORD wTrialKey = pCtrl->GetCurrentTrialKey();
   CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( wTrialKey );

   BOOL bSave = FALSE;                                            // save this trial's data?
   int nDurMS = 0;                                                // projected dur of trial in msecs
   int t0 = -1;                                                   // portion of trial [t0..t1] to be displayed in data
   int t1 = -1;                                                   // trace display; if neg, then show entire trial

   if( !pCtrl->GetTrialInfo(                                      // load trial info into CXDRIVER IPC:
            &(m_pShm->nTrialTgts),                                //    #tgts participating in trial
            &(m_pShm->iTgMap[0]),                                 //    trial target index map
            &(m_pShm->nCodes),                                    //    #codes defining the trial
            CX_MAXTC,                                             //    size of the trial code buffer
            &(m_pShm->trialCodes[0]),                             //    the trial code buffer
            &(m_pShm->dwTrialFlags),                              //    state of trial's bit flags
            &(m_pShm->nSections),                                 //    #tagged sections defined on the trial
            &(m_pShm->trialSections[0]),                          //    the tagged sections buffer
            &nDurMS,                                              //    projected duration
            &t0, &t1,                                             //    display interval (if shorter than trial)
            bSave ) )                                             //    save this trial's data?
      return( FALSE );

   m_pShm->iXYDotSeedAlt = pTrial->GetAltXYDotSeed();             //    alt. XY dot seed (>=0 overrides disp settings)

   memset( &(m_pShm->strDataPath[0]), 0, CX_MAXPATH );            // clear previous pathname
   if( bSave )                                                    // if we're saving trial data...
   {
      if( ::strlen( strData ) > CX_MAXPATH-1 )                    //    copy trial filename to IPC (if valid)
      {
         pApp->LogMessage( EMSG_DATAPATHTOOLONG );
         return( FALSE );
      }
      ::strcpy_s( m_pShm->strDataPath, strData );

   }
   m_pShm->bSaveSpikeTrace = bSave && bSpikesOn;                  // optionally record high-res spike trace
   ::strcpy_s(m_pShm->strProtocol, pDoc->GetObjName(wTrialKey));  // copy trial name to IPC
            
   // copy the names of the trial's parent set -- and, if applicable, subset -- to IPC
   memset(&(m_pShm->strSubset[0]), 0, CX_MAXOBJNAMELEN);
   memset(&(m_pShm->strSet[0]), 0, CX_MAXOBJNAMELEN);
   WORD wSetKey = pDoc->GetParentObj(wTrialKey);
   if(wSetKey != CX_NULLOBJ_KEY)
   {
	  if(pDoc->GetObjType(wSetKey) == CX_TRIALSUBSET)
	  {
         ::strcpy_s(m_pShm->strSubset, pDoc->GetObjName(wSetKey));
		 wSetKey = pDoc->GetParentObj(wSetKey);
      }
	  ::strcpy_s(m_pShm->strSet, pDoc->GetObjName(wSetKey));
   }

   WORD wChanKey = pCtrl->GetChannels();                          // prepare list of AI channels to be saved during
   int iCh[CX_AIO_MAXN+1];                                        // trial, if any, and send list to CXDRIVER via the
   iCh[0] = 0;                                                    // CX_SAVECHANS command
   if( wChanKey != CX_NULLOBJ_KEY )
   {
      CCxChannel* pCh = (CCxChannel*) pDoc->GetObject( wChanKey );
      iCh[0] = pCh->GetRecordedAIChannels( &(iCh[1]) );
   }
   DWORD dwCmd = CX_SAVECHANS;
   if( !SendCommand( dwCmd, &(iCh[0]), NULL, iCh[0]+1, 0,0,0 ) )
   {
      pApp->LogMessage( _T("!! Unable to update AI channel save list !!") );
      return( FALSE );
   }

   if( t0 < 0 || t1 < 0 || t1 <= t0 )                             // set up trace display in delayed mode.  It will
   {                                                              // only store that portion of the trial in [t0..t1],
      t0 = 0;                                                     // which may or may not span the entire trial.
      t1 = nDurMS;                                                // The traces are displayed at trial's end.
   }
   SetupDelayedTrialTraces(wChanKey, pTrial->Name(), t0, t1);

   if( m_pHistPanel != NULL )                                     // start event streaming and prepare spike histogram
   {                                                              // facility to consume any spike events streamed
      if( !StartEventStream() )                                   // during the trial
      {
         pApp->LogMessage( _T("!! Unable to initialize spike histogram display !!") );
         return( FALSE );
      }

      m_pHistPanel->PrepareForNextTrial(
         m_pShm->nCodes, &(m_pShm->trialCodes[0]),
         m_pShm->nSections, &(m_pShm->trialSections[0])
         );
   }

   // if the Eyelink tracker is in use, it should be recording. However, if recording aborted previously for any reason
   // we try to re-start recording here. If unable to do so, don't start trial.
   if(IsEyelinkConnected() && !IsEyelinkRecording())
   {
      if(!m_EyeLink.StartRecord())
      {
         pApp->LogMessage( _T("!! Eyelink tracker in use, but unable to restart recording on Eyelink !!") );
         return( FALSE );
      }
   }

   SetCurrentDateStamp();                                         // update the date stamp stored in IPC

   dwCmd = CX_TR_START;                                           // start the trial!
   if( !SendCommand( dwCmd, NULL, NULL, 0,0,0,0 ) )
   {
      pApp->LogMessage( _T("(!!) CXDRIVER failed to start the trial!!") );
      return( FALSE );
   }

   return( TRUE );
}


/**
 * MAESTRO uses several "global" transform parameters to offset, rotate and scale target trajectories in trial mode.
 * These factores are reported in the trial data file header, which is written by MAESTRODRIVER. Thus, MAESTRO must
 * communicate these factors over IPC. This method updates the current values of the transform parameters in the IPC
 * shared memory resource.
 *
 * @param pCtrl The trial sequencer, which handles all details of sequencing trials from a trial set, including 
 * preparation of trial codes and trial targets. The sequencer is queried for the current values of the various
 * transform parameters.
 */
VOID CCxRuntime::SetTransform(const CCxTrialSequencer* pCtrl)
{
   if(m_pShm != NULL && pCtrl != NULL)
   {
      m_pShm->fPosScale = float(pCtrl->GetTgtPosScale());
      m_pShm->fPosRotate = float(pCtrl->GetTgtPosRotate());
      m_pShm->fVelScale = float(pCtrl->GetTgtVelScale());
      m_pShm->fVelRotate = float(pCtrl->GetTgtVelRotate());
      m_pShm->fStartPosH = float(pCtrl->GetStartingPosH());
      m_pShm->fStartPosV = float(pCtrl->GetStartingPosV());
   }
}


//=== IsTrialRunning ==================================================================================================
//
//    When a trial is running in TrialMode, the trial result field in IPC is zeroed.  Once CXDRIVER has finished
//    executing the trial (and saved any data to file, as appropriate), that field will be set to some nonzero value.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if we're in TrialMode & a trial is running; FALSE otherwise.
//
BOOL CCxRuntime::IsTrialRunning() const
{
   return( BOOL((GetMode() == TrialMode) && (m_pShm->dwResult == 0)) );
}


//=== GetLastTrialLen =================================================================================================
//
//    Return the length of the last trial presented in TrialMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    elapsed time of last trial presented in TrialMode, in milliseconds.  If not in TrialMode, or if a
//                trial is currently in progress, the method returns 0.
//
int CCxRuntime::GetLastTrialLen() const
{
   if( (GetMode() == TrialMode) && (m_pShm->dwResult != 0) )
      return( m_pShm->iLastTrialLen );
   else
      return( 0 );
}


//=== GetRPDistroBehavResp ============================================================================================
//
//    Return behavioral response to the "R/P Distro" trial just presented in TrialMode.
//
//    Callers are responsible for checking that the CX_FT_GOTRPDRESP flag is set in the results field returned by
//    GetProtocolStatus().  If it is not set, then there is no response sample available, and this method returns 0.
//
//    ARGS:       NONE.
//    RETURNS:    Measured response, currently eye velocity magnitude in visual deg/sec.  Returned value is valid only
//                after the presentation of an "R/P Distro" trial in TrialMode, and only if the trial progressed past
//                the segment during which the response is average.  Method returns 0 otherwise.
//
float CCxRuntime::GetRPDistroBehavResp() const
{
   if( (GetMode() == TrialMode) && ((m_pShm->dwResult & CX_FT_GOTRPDRESP) == CX_FT_GOTRPDRESP) )
      return( m_pShm->fResponse );
   else
      return( 0.0f );
}


//=== GetProtocolStatus ===============================================================================================
//
//    Access to the protocol status/results field in IPC.
//
//    ARGS:       NONE.
//
//    RETURNS:    current value of protocol status/results field, or 0 if we're not in Trial or Cont mode.
//
DWORD CCxRuntime::GetProtocolStatus() const
{
   int iMode = GetMode();
   return( (iMode == TrialMode || iMode == ContMode) ? m_pShm->dwResult : 0 );
}


//=== LoadActiveTargets ===============================================================================================
//
//    Load the "active target list" into CXDRIVER IPC.  For use in ContMode only.
//
//    In ContMode, any target may be assigned as an active target.  The user can arbitrarily position such targets,
//    turn them on/off at will, and designate one as a fixation target.  All active targets are stored in the "loaded
//    target list" in CXIPC and identified by their position in that list.
//
//    NOTE:  We used to issue CX_TR_PRELOADFB here to preload framebuffer video targets.  However, b/c RMVideo is much
//    faster than the old VSG-based framebuffer video, this preload is no longer necessary.
//
//    ARGS:       n     -- [in] # of active targets.
//                pwKeys-- [in] ptr to list of target object keys.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxRuntime::LoadActiveTargets( int n, WORD* pwKeys )
{
   ASSERT( GetMode() == ContMode );

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();                       // to access target objects

   for( int i = 0; i < n; i++ )
   {
      CCxTarget* pTarg = (CCxTarget*) pDoc->GetObject( pwKeys[i] );
      ASSERT_KINDOF( CCxTarget, pTarg );

      const CString& tgName = pTarg->Name();
      m_pShm->targets[i].wType = pTarg->DataType();
      if(tgName.GetLength() < CX_MAXOBJNAMELEN) ::strcpy_s(m_pShm->targets[i].name, tgName);
      else
      {
         // target name exceeds char buffer size in IPC, so we must truncate it
         for(int j = 0; j < CX_MAXOBJNAMELEN - 1; j++) m_pShm->targets[i].name[j] = tgName.GetAt(j);
         m_pShm->targets[i].name[CX_MAXOBJNAMELEN - 1] = '\0';
      }

      pTarg->GetParams( &(m_pShm->targets[i].u) );
   }
   m_pShm->nTgts = n;

   return( TRUE );
}


//=== LoadStimulusRun =================================================================================================
//
//    Load a stimulus run definition into MAESTRODRIVER IPC.  For use in ContMode only.
//
//    ARGS:       wKey  -- [in] object key of the stimulus run to be written to CXIPC.
//                bStop -- [in] if TRUE, defined run must autostop after finite # duty cycles
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad parameter).
//
BOOL CCxRuntime::LoadStimulusRun( WORD wKey, BOOL bStop )
{
   ASSERT( GetMode() == ContMode );

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();                       // to access the run object
   CCxContRun* pRun = (CCxContRun*) pDoc->GetObject( wKey );
   if( pRun->DataType() != CX_CONTRUN ) return( FALSE );

   pRun->GetDefinition( m_pShm->runDef );                                     // store CXDRIVER-compatible defn in IPC
   if( bStop && (m_pShm->runDef.nAutoStop == 0) )                             // if autostop is required, make sure
      m_pShm->runDef.nAutoStop = 1;                                           // loaded run has a nonzero autostop!
   return( TRUE );
}


//=== StartStimulusRun ================================================================================================
//
//    Issue command CX_CM_RUNSTART to start the stimulus run currently defined in CXDRIVER IPC.  For ContMode only.
//
//    ARGS:       NONE
//
//    RETURNS:    TRUE if successful; FALSE otherwise (CXDRIVER may not be responding).
//
BOOL CCxRuntime::StartStimulusRun()
{
   ASSERT( GetMode() == ContMode );

   DWORD dwCmd = CX_CM_RUNSTART;
   return( SendCommand( dwCmd, NULL, NULL, 0,0,0,0 ) );
}


//=== StopStimulusRun =================================================================================================
//
//    Issue command CX_CM_RUNSTOP to stop an ongoing stimulus run now or at end of duty cycle.  In the latter case, the
//    method does NOT wait for the run to stop before returning!  For ContMode only.
//
//    ARGS:       bNow     -- [in] if TRUE, abort run; else, stop at end of duty cycle.
//                bStopRec -- [in] ignored unless bNow == TRUE.  if TRUE, recording is also stopped and recorded data
//                            file is discarded.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (CXDRIVER may not be responding).
//
BOOL CCxRuntime::StopStimulusRun( BOOL bNow, BOOL bStopRec )
{
   ASSERT( GetMode() == ContMode );

   DWORD dwCmd = CX_CM_RUNSTOP;                             // integer arg for CX_CM_RUNSTOP: 0=stop at end of duty
   int iArg = 0;                                            // cycle; 1=abort; 2=abort run, stop rec, & discard data
   if( bNow ) iArg = (bStopRec) ? 2 : 1;
   return( SendCommand( dwCmd, &iArg, NULL, 1,0,0,0 ) );
}


//=== StartRecord =====================================================================================================
//
//    Initiate data recording in ContMode.
//
//    [NOTE that data trace facility is NOT reinitialized; it runs continuously in ContMode and is reset only when the
//    user changes the channel configuration.  See CXCONTRUNPANEL.*.]
//
//    ARGS:       strData     -- [in] the pathname to which the cont-mode data file should be saved.
//                bSpikesOn   -- [in] if TRUE, high-res spike trace is also recorded and saved in data file.
//                wRunKey     -- [in] key of stimulus run assoc with this recording, or CX_NULLOBJ_KEY if none.
//                wChanKey    -- [in] object key of the channel config specifying channels to record.
//
//    RETURNS:    TRUE if trial was successfully started, FALSE otherwise.
//
BOOL CCxRuntime::StartRecord( LPCTSTR strData, BOOL bSpikesOn, WORD wRunKey, WORD wChanKey )
{
   ASSERT( GetMode() == ContMode );

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                  // for posting messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // for channel config object

   memset( &(m_pShm->strDataPath[0]), 0, CX_MAXPATH );            // copy data pathname to IPC (if valid)
   if(::strlen( strData ) > CX_MAXPATH-1)
   {
      pApp->LogMessage( EMSG_DATAPATHTOOLONG );
      return( FALSE );
   }
   ::strcpy_s( m_pShm->strDataPath, strData );

   m_pShm->bSaveSpikeTrace = bSpikesOn;                           // optionally record high-res spike trace

   memset( &(m_pShm->strProtocol[0]), 0, CX_MAXOBJNAMELEN );      // if a stim run is associated with this recording,
   if( wRunKey != CX_NULLOBJ_KEY )                                // copy its name to IPC, else clear it to ""
      ::strcpy_s(m_pShm->strProtocol, pDoc->GetObjName(wRunKey));

   int iCh[CX_AIO_MAXN+1];                                        // prepare list of AI channels to be saved while
   iCh[0] = 0;                                                    // recording, and send list to CXDRIVER via the
   if( wChanKey != CX_NULLOBJ_KEY )                               // CX_SAVECHANS command
   {
      CCxChannel* pCh = (CCxChannel*) pDoc->GetObject( wChanKey );
      iCh[0] = pCh->GetRecordedAIChannels( &(iCh[1]) );
   }
   DWORD dwCmd = CX_SAVECHANS;
   if( !SendCommand( dwCmd, &(iCh[0]), NULL, iCh[0]+1, 0,0,0 ) )
   {
      pApp->LogMessage( _T("!! Unable to update AI channel save list !!") );
      return( FALSE );
   }

   // if the Eyelink tracker is in use, it should be recording. However, if recording aborted previously for any reason
   // we try to re-start recording here. If unable to do so, don't start recording.
   if(IsEyelinkConnected() && !IsEyelinkRecording())
   {
      if(!m_EyeLink.StartRecord())
      {
         pApp->LogMessage( _T("!! Eyelink tracker in use, but unable to restart recording on Eyelink !!") );
         return( FALSE );
      }
   }

   SetCurrentDateStamp();                                         // update the date stamp stored in IPC

   dwCmd = CX_CM_RECON;                                           // start recording
   if( !SendCommand( dwCmd, NULL, NULL, 0,0,0,0 ) )
   {
      pApp->LogMessage( _T("!! CXDRIVER failed to start recording !!") );
      return( FALSE );
   }

   return( TRUE );
}


//=== StopRecord ======================================================================================================
//
//    Stop data recording in ContMode.
//
//    ARGS:       bSave -- [in] if TRUE, the recorded data file should be kept; else it is discarded.
//                      -- [out] FALSE if CXDRIVER was unable to save the data file; else TRUE (ignore if data file is
//                         not to be saved.
//
//    RETURNS:    TRUE if successful; FALSE indicates a catastrophic error in CXDRIVER.
//
BOOL CCxRuntime::StopRecord( BOOL& bSave )
{
   ASSERT( GetMode() == ContMode );

   DWORD dwCmd = CX_CM_RECOFF;
   int iArg = bSave ? 1 : 0;
   BOOL bOk = SendCommand( dwCmd, &iArg, NULL, 1,0,1,0, 500 ); // give extra time to flush & close data file
   bSave = BOOL(iArg == 1);                                    // was data file successfully saved by CXDRIVER?
   return( bOk );
}


//=== SendCommand =====================================================================================================
//
//    Send a command to CXDRIVER and wait T msecs for a response.  If CXDRIVER fails to acknowledge the command within
//    the required time, we return CX_TIMEDOUTCMD.  An additional 50ms is alloted to complete the command-response
//    handshake once CXDRIVER acknowledges the command (which means it has completed the task).
//
//    However, most commands can be completed in very little time, perhaps microseconds.  So that we do not wait too
//    long for a response, we sleep for 200us intervals using RtSleepFt().
//
//    This method does NOT throw up a wait cursor for long waits -- that's up to the caller.
//
//    02sep2009(sar) -- Modified to include buffer for character data associated with the command and/or response.
//
//    ARGS:       dwCmd       -- [in] ID of the command to be sent;
//                               [out] if command was executed, this is unchanged; else, an error ID <= CX_FAILEDCMD.
//                piData      -- [in] buffer containing integer data associated with the command (can be NULL).
//                            -- [out] contains integer data associated with the response to the command.
//                pfData      -- [in] buffer containing floating-point data associated with the command (can be NULL).
//                            -- [out] contains floating-point data associated with the response to the command.
//                pcData      -- [in] buffer containing character data associated with the command (can be NULL).
//                            -- [out] contains character data associated with the response to the command.
//                niCmd,niRsp -- [in] # of integers associated with command, response (ignored if integer buf is NULL).
//                nfCmd,nfRsp -- [in] # of floats associated with command, response (ignored if float buf is NULL).
//                ncCmd,ncRsp -- [in] # of characters associated with command, response (ignored if char buf is NULL).
//                tWait       -- [in] max time to wait for acknowledge [default & minimum = 50msec].
//
//    RETURNS:    TRUE if CXDRIVER responded successfully to the command, FALSE otherwise.
//
BOOL CCxRuntime::SendCommand(DWORD& dwCmd, int *piData, float *pfData, int niCmd, int nfCmd, int niRsp, int nfRsp, 
      int tWait /* = 50 */, char *pcData /* = NULL */, int ncCmd /* = 0 */, int ncRsp /* = 0 */)
{
   if( !m_bDriverOn )                                                   // CXDRIVER not running; cannot send command!
   {
      dwCmd = CX_DRVROFF;
      return( FALSE );
   }
   ASSERT( m_pShm != NULL );

   if( m_pShm->bReqCmd || m_pShm->bAckCmd )                             // previous cmd still pending
   {
      ASSERT( FALSE );                                                  // this condition SHOULD NEVER OCCUR, since
      dwCmd = CX_PENDINGCMD;                                            // this method waits for response fr CXDRIVER!
      return( FALSE );
   }

   if( ((piData != NULL) &&                                             // illegal command
        ((niCmd < 0) || (niCmd > CX_CMDLEN) || (niRsp < 0) || (niRsp > CX_CMDLEN))
       ) ||
       ((pfData != NULL) &&
        ((nfCmd < 0) || (nfCmd > CX_CMDLEN) || (nfRsp < 0) || (nfRsp > CX_CMDLEN))
       ) ||
       ((pcData != NULL) &&
        ((ncCmd < 0) || (ncCmd > CX_CDATALEN) || (ncRsp < 0) || (ncRsp > CX_CDATALEN))
       )
     )
   {
      dwCmd = CX_ILLEGALCMD;
      return( FALSE );
   }

   DWORD origCmd = dwCmd;  // preserve command ID to report timeout error below

   // when switching modes and Eyelink tracker is in use, we need to tell Eyelink to start or stop recording. Note 
   // that we do this before the mode switch. If the mode switch fails, both driver and Eyelink interface will be
   // reset anyway. Recording is on in Trial/Cont modes, off otherwise.
   if(dwCmd == CX_SWITCHMODE && m_EyeLink.IsConnected())
   {
      int mode = GetMode();
      BOOL isIdle = BOOL(mode == CCxRuntime::IdleMode || mode == CCxRuntime::TestMode);
      BOOL willBeIdle = BOOL(piData != NULL && niCmd == 1 && 
            (*piData == CCxRuntime::IdleMode || *piData == CCxRuntime::TestMode));
      if(isIdle && !willBeIdle)
         m_EyeLink.StartRecord();
      else if(willBeIdle && !isIdle)
         m_EyeLink.StopRecord();
   }

   m_pShm->dwCommand = dwCmd;                                           // copy command data to IPC interface
   if(piData != NULL)
      memcpy(&(m_pShm->iData[0]), piData, sizeof(int)*niCmd);
   if(pfData != NULL)
      memcpy(&(m_pShm->fData[0]), pfData, sizeof(float)*nfCmd);
   if(pcData != NULL)
      memcpy(&(m_pShm->cData[0]), pcData, sizeof(char)*ncCmd);
      
   double dWait = (double) ((tWait < 50) ? 50 : tWait) * 1000;          // minimum wait time is 50msecs
   LARGE_INTEGER i64Sleep;                                              // for sleeping in 1ms bursts
   i64Sleep.QuadPart = 10000;

   CElapsedTime usWaiting;                                              // send command and wait for resp
   m_pShm->bReqCmd = TRUE;
   while( (usWaiting.Get() < dWait) && !m_pShm->bAckCmd )
      CCxRtapi::RtSleepFt( &i64Sleep );
   BOOL bTimeOut = !m_pShm->bAckCmd;                                    // did CXDRIVER fail to respond?

   usWaiting.Reset();                                                   // reset command and wait up to 50ms more for
   m_pShm->bReqCmd = FALSE;                                             // CXDRIVER to complete handshake
   if( !bTimeOut )
   {
      while( (usWaiting.Get() < 50000.0) && m_pShm->bAckCmd )
         CCxRtapi::RtSleepFt( &i64Sleep );
      bTimeOut = m_pShm->bAckCmd;                                       // did CXDRIVER fail to complete handshake?
   }
   m_pShm->bAckCmd = FALSE;

   dwCmd = (bTimeOut) ? CX_TIMEDOUTCMD : m_pShm->dwCommand;             // copy response data from IPC interface
   if(dwCmd > CX_FAILEDCMD)                                             // if CXDRIVER responded successfully
   {
      if(piData != NULL)
         memcpy(piData, &(m_pShm->iData[0]), sizeof(int)*niRsp);
      if(pfData != NULL)
         memcpy(pfData, &(m_pShm->fData[0]), sizeof(float)*nfRsp);
      if(pcData != NULL)
         memcpy(pcData, &(m_pShm->cData[0]), sizeof(char)*ncRsp);
   }
   m_pShm->dwCommand = CX_NULLCMD;

   // report timeout error in application message log
   if(bTimeOut)
   {
      CString strMsg;
      strMsg.Format("DBG: CCxRuntime::SendCommand() timed out on command id=%d", origCmd);
      ((CCntrlxApp*)AfxGetApp())->LogMessage(strMsg);
   }

   return( BOOL(dwCmd > CX_FAILEDCMD) );
}


//=== ConvertVoltsToRaw, ConvertRawToVolts ============================================================================
//
//    Converts an AI or AO sample from a voltage (in volts) to the corresponding DAC value, or vice versa.  The
//    conversion depends upon whether a 12bit or 16bit analog device is in use.  The method assumes that the device is
//    configured as bipolar with the available resolution covering a voltage range of +/-10V.
//
//    ARGS:       fVolt -- [in] AI or AO datum in volts
//                iDac  -- [in] AI or AO datum as raw DAC value
//                isAI  -- [in] if TRUE, datum is an analog input; else, analog output
//
//    RETURNS:    The AI or AO datum in volts or as a DAC value, appropriately range-limited.
//
int CCxRuntime::ConvertVoltsToRaw( float fVolt, BOOL isAI )
{
   DWORD resFlagBit = isAI ? CX_F_AI16BIT : CX_F_AO16BIT;
   BOOL use16Bit = BOOL((GetHWStatus() & resFlagBit) == resFlagBit);
   int minDac = use16Bit ? -32768 : -2048;
   int maxDac = use16Bit ? 32767 : 2047;

   int iDac = int( fVolt * (use16Bit ? 65536.0f : 4096.0f) / 20.0f );
   if( iDac < minDac ) iDac = minDac;
   else if( iDac > maxDac ) iDac = maxDac;

   return( iDac );
}

float CCxRuntime::ConvertRawToVolts( int iDac, BOOL isAI )
{
   DWORD resFlagBit = isAI ? CX_F_AI16BIT : CX_F_AO16BIT;
   BOOL use16Bit = BOOL((GetHWStatus() & resFlagBit) == resFlagBit);

   int minDac = use16Bit ? -32768 : -2048;
   int maxDac = use16Bit ? 32767 : 2047;
   if( iDac < minDac ) iDac = minDac;
   else if( iDac > maxDac ) iDac = maxDac;

   float fVolt = float(iDac) * 20.0f / (use16Bit ? 65536.0f : 4096.0f);
   return( fVolt );
}

/**
 Get the current length of the sliding window average used to smooth recorded eye position and thereby reduce noise
 introduced during velocity stabilization of targets in Trial Mode.
 @return The window length in ms. Restricted to 1..20. A value of 1 effectively disables the sliding average.
*/
int CCxRuntime::GetVStabSlidingWindow() const
{
   return( (m_pShm!=NULL) ? m_pShm->iVStabSlidingWindow : 1 );
}

/**
 Set the current length of the sliding window average used to smooth recorded eye position and thereby reduce noise
 introduced during velocity stabilization of targets in Trial Mode.
 @param sz The window length in ms. Restricted to 1..20. A value of 1 effectively disables the sliding average.
 @return True if successful, false if specified window length is invalid or could not be updated.
*/
BOOL CCxRuntime::SetVStabSlidingWindow(int sz)
{
   if(sz < 1 || sz > 20 || m_pShm==NULL) return(FALSE);
   m_pShm->iVStabSlidingWindow = sz;
   return(TRUE);
}
   



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump internal state for debugging purposes.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxRuntime::Dump( CDumpContext& dc ) const
{
   CObject::Dump( dc );
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate internal consistency for debugging purposes.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxRuntime::AssertValid() const
{
   CObject::AssertValid();
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== Open ============================================================================================================
//
//    Prepare the IPC shared memory interface for communicating with CXDRIVER, then spawn CXDRIVER and wait until we
//    get a handle to the "stop mutex" that should be created and claimed by CXDRIVER as soon as it starts.
//
//    If called when the driver is still on, this method will kill the driver and attempt to restart it.  If we are
//    unable to start the driver for whatever reason, all IPC resources are released and a non-NULL error message is
//    returned.
//
//    ** Terminating an "orphaned" CXDRIVER instance. If an instance of MAESTRO crashes or is terminated abruptly, the
//    CXDRIVER side may be left running on the system. If, subsequently, MAESTRO starts again and attempts to spawn
//    another instance of CXDRIVER, the system will crash (RTX "green screen" STOP). To avoid this situation, MAESTRO
//    must be able to detect and terminate the "orphaned" driver. While RTX64 lets you enumerate running RTSS processes,
//    we instead store CXDRIVER's process ID in the CXIPC shared memory object. Before spawning the CXDRIVER, we attempt 
//    to open a handle to its unique "stop mutex". If we obtain a valid handle, then we know that an orphaned instance 
//    is still running. We then open a handle to the orphaned shared memory IPC object (which should never have been 
//    released b/c the driver is still using it!), obtain its process ID from that, then acquire a handle to the process
//    and call RtTerminateProcess() to terminate the orphaned driver. NOTE: This is similar to the procedure that was
//    used in RTX, but in RTX64 there are convenient APIs for spawning and terminating RTSS processes that were not
//    available in RTX.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL CCxRuntime::Open()
{
   if( m_bDriverOn ) Close();                                              // if driver already on, kill it & restart

   ASSERT( !m_bDriverOn );                                                 // validate pre-startup state
   ASSERT( m_hRtStopMutex == NULL );
   ASSERT( m_hRtShared == NULL );
   ASSERT( m_pShm == NULL );

   char strMsg[200];
   CWaitCursor waitCursor;                                                 // this may take a while!

   CString strWarn;                                                        // possible warning message
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                           // for posting error/warning messages

   // detect and terminate an "orphaned" instance of the CXDRIVER RTSS process. If an orphaned instance is running and
   // we fail to terminate it, then we cannot spawn a new instance.
   m_hRtStopMutex = CCxRtapi::RtOpenMutex(SYNCHRONIZE, FALSE, CXIPC_STOPMUTEX); 
   if(m_hRtStopMutex != (HANDLE) NULL)
   { 
      pApp->LogMessage( WMSG_ORPHANDRVR );
      m_hRtShared = CCxRtapi::RtOpenSharedMemory(SHM_MAP_ALL_ACCESS, FALSE, CXIPC_SHM, (VOID **) &m_pShm);
      BOOL bSuccess = BOOL(m_hRtShared != (HANDLE)NULL);
      if(bSuccess)
      {
         HANDLE hProc = CCxRtapi::RtOpenProcess(PROCESS_TERMINATE, FALSE, m_pShm->dwProcessID);
         if(hProc != (HANDLE)NULL)
         {
            bSuccess = CCxRtapi::RtTerminateProcess(hProc, (UINT)0);
            CCxRtapi::RtCloseHandle(hProc);
         }
         else bSuccess = FALSE;
      }
         
      CCxRtapi::RtCloseHandle(m_hRtStopMutex); 
      m_hRtStopMutex = (HANDLE)NULL;
      if(m_hRtShared != (HANDLE)NULL)
      {
         CCxRtapi::RtCloseHandle(m_hRtShared);
         m_hRtShared = (HANDLE)NULL;
      }
      m_pShm = (PCXIPCSM)NULL;

      if(!bSuccess) 
      {
         pApp->LogMessage(EMSG_CANTKILLORPHAN);
         pApp->LogMessage(EMSG_CANTKILLORPHAN2);
         return( FALSE );
      }
   }

   // create shared memory object for IPC with CXDRIVER; abort on failure.
   m_hRtShared = CCxRtapi::RtCreateSharedMemory(PAGE_READWRITE, 0, sizeof(CXIPCSM), CXIPC_SHM, (VOID **) &m_pShm); 
   if(m_hRtShared == (HANDLE) NULL)
   {
      ::sprintf_s(strMsg, "%s (error = 0x%08x)", EMSG_IPCFAILED, ::GetLastError());
      pApp->LogMessage(strMsg);
      Close(FALSE);
      return(FALSE);
   }

   // initialized IPC shared memory and copy Maestro install directory into it so CXDRIVER can find program files
   InitIPC(); 
   ::strcpy_s(m_pShm->strDataPath, pApp->GetHomeDirectory()); 

   // parse registry string listing busy waits in microseconds for static DO command timing in CXDRIVER. If parsing
   // fails, use defaults. Store busy wait times in dedicated field in IPC. Note that don't do any range-checking
   // here, except to veto negative times. CXDRIVER will limit the wait times to 0-20us.
   float fWait1, fWait2, fWait3;
   int nRes = ::sscanf_s(pApp->GetDOCommandTiming(), "%f,%f,%f", &fWait1, &fWait2, &fWait3);
   m_pShm->fDOBusyWaits[0] = (nRes > 0 && fWait1 >= 0.0f) ? fWait1 : 0.5f;
   m_pShm->fDOBusyWaits[1] = (nRes > 0 && fWait2 >= 0.0f) ? fWait2 : 2.5f;
   m_pShm->fDOBusyWaits[2] = (nRes > 0 && fWait3 >= 0.0f) ? fWait3 : 0.5f;

   // attempt to launch CXDRIVER
   STARTUPINFO          sInfo;
   PROCESS_INFORMATION	pInfo;
   ::ZeroMemory(&sInfo, sizeof(sInfo));
   sInfo.cb = sizeof(sInfo);
   ::ZeroMemory(&pInfo, sizeof(pInfo));
   char szCmdLine[] = { '\0' };
   
   BOOL bOk = CCxRtapi::RtCreateProcess(DRVR_EXECUTABLE, &(szCmdLine[0]), NULL, NULL, FALSE, (DWORD)0, NULL, 
      pApp->GetHomeDirectory(), &sInfo, &pInfo);
   if(!bOk)
   {
      ::sprintf_s(strMsg, "Failed to launch Maestro RTX64 driver (error = 0x%08x). Check with developer",
         ::GetLastError());
      pApp->LogMessage(strMsg);

      Close(FALSE);
      return(FALSE);
   }

   // if CXDRIVER launched, save process ID in shared memory (in case we have to terminate it). Close process and
   // main thread handles because we don't need them.
   ::CloseHandle(pInfo.hThread);
   ::CloseHandle(pInfo.hProcess);
   m_pShm->dwProcessID = pInfo.dwProcessId;
   m_bDriverOn = TRUE;

   // give CXDRIVER up to 200ms to open the mutex it holds throughout its lifetime. If we cannot get a handle to this
   // "stop" mutex, then CXDRIVER probably failed early in startup.
   CElapsedTime tWait;
   while(m_hRtStopMutex == (HANDLE)NULL) 
   {
      if(tWait.Get() > 200000.0) break;
      m_hRtStopMutex = CCxRtapi::RtOpenMutex(SYNCHRONIZE, FALSE, CXIPC_STOPMUTEX);
   }
   if((m_hRtStopMutex == (HANDLE)NULL) || (CCxRtapi::RtWaitForSingleObject(m_hRtStopMutex, 0) != WAIT_TIMEOUT))
   {
      pApp->LogMessage(EMSG_DRVRSYNCFAILED);
      Close(FALSE);
      return(FALSE);
   }

   // CXDRIVER successfully started!
   return(TRUE);
}


//=== Close ===========================================================================================================
//
//    Stop CXDRIVER, release all IPC resources, and clean up. We attempt a graceful shutdown first. If the driver
//    fails to shut down after two seconds, we terminate it.
//
//    ARGS:       bGraceful   -- [in] if TRUE, attempt coordinated shutdown of CXDRIVER; else terminate it
//                               immediately (default:  TRUE).
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::Close( const BOOL bGraceful /* =TRUE */ )
{
   // if CXDRIVER appears to be ON, stop it. If a graceful shutdown requested, tell the driver to "die" and wait up to
   // 2 seconds for it to do so (must have the "stop" mutex handle to do this). If graceful shutdown fails or is not
   // requested, terminate CXDRIVER.
   if(m_bDriverOn)
   {
      BOOL bStopped = FALSE;
      if(bGraceful && (m_hRtStopMutex != NULL)) 
      { 
         ASSERT( (m_hRtShared != NULL) && (m_pShm != NULL) );
         int iMode = CX_STOPPING; 
         DWORD dwCmd = CX_SWITCHMODE;
         SendCommand(dwCmd, &iMode, NULL, 1, 0, 0, 0);

         CElapsedTime tWait; 
         while( IsAlive() && tWait.Get() < 2e6 ) ;
         bStopped = !IsAlive();
      }

      if(!bStopped)
      {
         HANDLE hProc = CCxRtapi::RtOpenProcess(PROCESS_TERMINATE, FALSE, m_pShm->dwProcessID);
         if(hProc != (HANDLE)NULL) 
         {
            CCxRtapi::RtTerminateProcess(hProc, (UINT)0);
            CCxRtapi::RtCloseHandle(hProc);
         }
      }
   }

   m_bDriverOn = FALSE;
   if(m_hRtStopMutex != (HANDLE)NULL)
   {
      CCxRtapi::RtCloseHandle(m_hRtStopMutex);
      m_hRtStopMutex = NULL;
   }

   if(m_hRtShared != (HANDLE)NULL)
   {
      CCxRtapi::RtCloseHandle(m_hRtShared);
      m_hRtShared = NULL;
   }
   m_pShm = NULL;
}


//=== IsAlive =========================================================================================================
//
//    If driver is on, check its stop mutex to see if it has died unexpectedly (mutex will be signalled or abandoned in
//    this case).
//
//    ARGS:       NONE.
//
//    RETURNS:    FALSE if driver is not loaded to begin with, or if it was loaded but has died; TRUE otherwise.
//
BOOL CCxRuntime::IsAlive()
{
   if( !m_bDriverOn ) return( FALSE );                                  // driver has not been started
   else
   {
      ASSERT( m_hRtStopMutex != (HANDLE)NULL );
      DWORD waitRes = CCxRtapi::RtWaitForSingleObject( m_hRtStopMutex, 0 );
      if( waitRes != WAIT_TIMEOUT )
         return( FALSE );                                               // stop mutex is signalled or abandoned!
      else
         return( TRUE );                                                // driver still alive
   }
}


//=== InitIPC =========================================================================================================
//
//    Initialize IPC shared memory object to an idle "startup" state.
//
//    The IPC shared memory object is used for CNTRLX-CXDRIVER communications.  This method sets up the state variables
//    in shared memory such that there are no pending service requests from CXDRIVER, and no pending command requests
//    from CNTRLX.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::InitIPC()
{
   ASSERT( m_pShm != NULL );                                   // make sure shared memory is there
   if( m_pShm == NULL ) return;

   m_pShm->iOpMode = CX_NOTRUNNING;                            // CXDRIVER sets this to CX_IDLEMODE after startup inits

   int i;

   m_pShm->iNextMsgToPost = 0;                                 // empty message queue
   m_pShm->iLastMsgPosted = 0;
   for( i = 0; i < CXIPC_MSGQLEN; i++ )
      m_pShm->szMsgQ[i][0] = '\0';

   m_pShm->bReqPlot = FALSE;                                   // no pending eye/target position plot update
   m_pShm->bAckPlot = FALSE;
   for( i = 0; i < CX_NLOCI; i++ )
   {
      m_pShm->ptLoci[i].x = 0;
      m_pShm->ptLoci[i].y = 0;
   }

   m_pShm->nTracesInUse = 0;                                   // no activity in the trace display service
   m_pShm->iTraceEnd = 0;
   m_pShm->iTraceDrawn = 0;
   m_pShm->bTraceOverflow = FALSE;

   // Eyelink tracker not in use
   m_pShm->iELStatus = CX_ELSTAT_OFF;
   m_pShm->iELLast = 0;
   m_pShm->iELNext = 0;

   m_pShm->bEventEnable = FALSE;                               // no activity in the event stream service
   m_pShm->iEventEnd = 0;
   m_pShm->iEventConsumed = 0;
   m_pShm->bEventOverflow = FALSE;

   m_pShm->bReqCmd = FALSE;                                    // no pending CNTRLX command
   m_pShm->bAckCmd = FALSE;
   m_pShm->dwCommand = CX_NULLCMD;
   for( i = 0; i < CX_CMDLEN; i++ )
   {
      m_pShm->iData[i] = 0;
      m_pShm->fData[i] = 0.0f;
   }

   m_pShm->nTgts = 0;                                          // trial info cleared.  not in TrialMode
   m_pShm->nCodes = 0;
   m_pShm->nSections = 0;
   memset( &(m_pShm->targets[0]), 0, CX_MAXTGTS*sizeof(CXTARGET) );
   memset( &(m_pShm->trialCodes[0]), 0, CX_MAXTC*sizeof(TRIALCODE) );
   memset( &(m_pShm->trialSections[0]), 0, MAX_SEGMENTS*sizeof(TRIALSECT) );

   m_pShm->dwResult = CX_FT_DONE;                              // other protocol info initialized
   m_pShm->iNumRewards = 0;
   m_pShm->iTotalRewardMS = 0;
   m_pShm->iLastTrialLen = 0;
   memset( &(m_pShm->strDataPath[0]), 0, CX_MAXPATH*sizeof(char) );
   memset( &(m_pShm->strProtocol[0]), 0, CX_MAXOBJNAMELEN*sizeof(char) );

   m_pShm->bChairPresent = TRUE;                               // init other signals from CNTRLX -> CXDRIVER
   m_pShm->bSaveSpikeTrace = FALSE;
   SetCurrentDateStamp();

   m_pShm->dwHWStatus = 0;                                     // CXDRIVER prepares this hardware status info
   m_pShm->nAOChannels = 0;
   m_pShm->nAIChannels = 0;
   m_pShm->nTDOChannels = 0;
   m_pShm->nTDIChannels = 0;

   // note: 0 is never assigned as a process ID by Windows. Not sure what happens in RTX64 environment.
   m_pShm->dwProcessID = (DWORD)0; 

   memset(&(m_pShm->strSet[0]), 0, CX_MAXOBJNAMELEN * sizeof(char));
   memset(&(m_pShm->strSubset[0]), 0, CX_MAXOBJNAMELEN * sizeof(char));
}

//=== SetCurrentDateStamp =============================================================================================
//
//    A date stamp is maintained in IPC for use by CXDRIVER, which does not have access to the current date in the RTX
//    environment.  Call this method to update that date stamp.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRuntime::SetCurrentDateStamp()
{
   if( m_pShm != NULL )
   {
      CTime t = CTime::GetCurrentTime();
      m_pShm->iDay = t.GetDay();
      m_pShm->iMonth = t.GetMonth();
      m_pShm->iYear = t.GetYear();           // good till the year 2038, acc to MFC documentation :)
   }
}


