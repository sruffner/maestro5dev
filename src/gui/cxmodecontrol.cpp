//=====================================================================================================================
//
// cxmodecontrol.cpp :  Implementation of CCxModeControl, the ABSTRACT base class for mode controller objects used by
//                      the MAESTRO mode control panel to manage relevant dialogs in each MAESTRO operational mode.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The ABSTRACT class CCxModeControl is part of the MAESTRO "mode control panel" framework.  It serves as base class
// for all mode control "helpers" created by the MAESTRO mode control panel, CCxControlPanel.  For each operational 
// mode in MAESTRO there is a dedicated "mode controller" that encapsulates MAESTRO functionality while in that mode.
// Dialogs housed in the mode control panel (base class CCxControlPanelDlg) rely on methods defined in the appropriate
// mode controller to carry out mode-specific actions.  The mode controller, in turn, interacts with the MAESTRO 
// runtime interface CCxRuntime to retrieve runtime state information or make changes to the runtime state (the dialogs
// themselves have no direct access to CCxRuntime).  CCxControlPanelDlg::GetModeCtrl() retrieves a CCxModeControl* ptr
// to the current mode controller, which can be then be cast to the derived class implementing a particular operational
// mode.  See also:  CCxControlPanel, CCxModeControl.
//
// Any realizable class derived from CCxModeControl -- say, CCxOpMode -- must provide a suitable constructor and
// implement all abstract CCxModeControl methods:
//
// CCxOpMode( CCxControlPanel* pPanel ) : CCxModeControl( MODEID, pPanel ):  Constructor must accept a pointer to the
// MAESTRO mode control panel, which it passes on to the CCxModeControl constructor, along with ID of the op mode it
// represents.  CCxControlPanel constructs each mode controller during the GUI creation phase at application startup.
//
// virtual BOOL InitDlgs():  This method is invoked by CCxControlPanel immediately after constructing the mode
//    controller.  Here the mode controller must add to the control panel any dialogs required for that operational
//    mode.  Since it is possible that some dialogs may be used in several different op modes, the mode controller MUST
//    check the runtime classes of dialogs already installed in the control panel to see if any of the dialogs it needs
//    are already installed.  At this time, the mode controller should save pointers to any dialogs it needs so it can
//    access them during runtime.  The dialog objects are NOT destroyed and recreated as needed; they remain available
//    for the application's lifetime.  Return FALSE if any dialog cannot be created -- this is a fatal error.  Once
//    added, do not remove dialogs from CCxControlPanel; at application exit, it will destroy its embedded dialogs.
//
// virtual VOID Service():  This method is frequently invoked by CCxControlPanel to update the runtime state of
//    MAESTRO/MAESTRODRIVER in the current operational mode.
//
// virtual BOOL Enter(), Exit():  CCxControlPanel will invoke these methods during an operational mode switch.  After
//    entering a new operational mode, it calls Enter() on the appropriate mode controller; prior to leaving that mode,
//    it calls Exit().  These methods permit the mode controller to do any required initializations upon entering the
//    mode, and any cleanup prior to exiting.
//
//    While all mode control dialogs are housed in one CCxControlPanel, not all dialogs are relevant in any particular
//    operational mode.  It is the mode controller's responsibility to ensure that only relevant dialogs are accessible
//    to the user in the mode control panel.  To do so, Exit() should invoke CCxControlPanel::HideDlg() to hide ALL
//    dialogs currently accessible in the control panel, while Enter() invokes CCxControlPanel::ShowDlg() for each
//    dialog that's relevant to the current operational mode.
//
// virtual BOOL CanUpdateVideoCfg():  Return TRUE when the current runtime state permits the update of the RMVideo
//    display configuration. Generally we want to prevent such updates when an experimental protocol is in progress.
//
// virtual BOOL CanUpdateFixRewSettings():  Return TRUE when the current runtime state permits changing the current
//    fixation/reward settings. Again, we usually want to prevent such changes while a protocol is running.
//
// virtual LPCTSTR GetModeTitle(): A short string describing this operational mode.
//
// ==> CCxNullMode.
// CCxNullMode serves as a "placeholder" mode controller governing MAESTRO runtime operations when MAESTRODRIVER is not
// available for whatever reason.  Since it really does nothing, its implementation is trivial -- so we've included it
// here rather than in a separate .H/.CPP module.
//
//
// REVISION HISTORY:
// 02apr2003-- MAJOR redesign of CCxControlPanel.  CCxControlPanel is no longer an abstract base class for multiple
//             CNTRLX mode control panels.  Instead, it is THE control panel for CNTRLX.  Functionality distributed
//             across the former CCxTestPanel, CCxTrialPanel, and CCxContRunPanel are now located in "mode controller
//             objects" created by CCxControlPanel.  All mode controllers are derived from CCxModeControl.
// 10mar2005-- Minor change IAW changes to CCxRuntime regarding reward statistics.
// 14mar2006-- Removed IsOKNAvailable().  OKNDRUM no longer supported as of Maestro v1.5.0.
// 24mar2006-- IsFBAvailable() change to IsRMVideoAvailable(), plus other cosmetic changes to emphasize replacement 
//             of VSG-based framebuffer video with RMVideo.
// 21apr2006-- Added wrappers to access RMVideo display properties via same-named CCxRuntime methods.
// 04sep2009-- Added wrappers to access RMVideo display mode, monitor gamma, and the "movie store".
// 11oct2016-- Updated signatures of wrappers that access RMVideo "media store" -- formerly the "movie store" -- IAW
//             same-dated changes in CCxRuntime.
// 26sep2024-- Removed all references to the XYScope. No longer supported a/o Maestro V4.0, it was removed entirely
//             for V5.0.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for MAESTRO

#include "cxruntime.h"                       // CCxRuntime -- the IPC interface to MAESTRODRIVER
#include "cxcontrolpanel.h"                  // CCxControlPanel -- MAESTRO mode control panel (dialog container)
#include "cxmodecontrol.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxModeControl
//
//=====================================================================================================================
//=====================================================================================================================

//=== CCxModeControl [constructor], ~CCxModeControl [destructor] ======================================================
//
//    Implementing class must specify the MAESTRO op mode it will manage.  We verify that mode and save it internally,
//    as well as pointers to the MAESTRO runtime interface and the MAESTRO mode control panel, which houses any 
//    dialogs with which the mode controller will interact.  Note that the MAESTRO runtime interface object must exist 
//    when the mode controller is constructed.
//
//    ARGS:       iMode -- [in] MAESTRO op mode that will be managed by this mode controller.
//                pPanel-- [in] ptr to the MAESTRO mode control panel.
//
CCxModeControl::CCxModeControl( int iMode, CCxControlPanel* pPanel )
{
   ASSERT( iMode == CCxRuntime::IdleMode || iMode == CCxRuntime::TrialMode ||
           iMode == CCxRuntime::ContMode || iMode == CCxRuntime::TestMode ||
           iMode == CX_NOTRUNNING );                                             // for CCxNullMode!
   ASSERT( pPanel );
   ASSERT_KINDOF( CCxControlPanel, pPanel );
   m_iOpMode = iMode;
   m_pCtrlPanel = pPanel;
   m_pRuntime = ((CCntrlxApp*)AfxGetApp())->GetRuntime();
   ASSERT( m_pRuntime );
   ASSERT_KINDOF( CCxRuntime, m_pRuntime );
}

CCxModeControl::~CCxModeControl() { }



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Refresh =========================================================================================================
//
//    Refresh appearance of all dialogs currently accessible in the mode control panel.  The MAESTRO mode control panel
//    framework uses this mechanism to keep dialog controls up-to-date instead of relying on the CWnd::OnUpdateCmdUI()
//    mechanism which is invoked during application idle time.  The mode control panel object handles this task.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxModeControl::Refresh()
{
   if( m_pCtrlPanel && m_pRuntime && m_pRuntime->GetMode() == m_iOpMode ) m_pCtrlPanel->Refresh();
}


//=== UpdateVideoCfg, etc. ============================================================================================
//
// These are convenience methods for sending commands to or retrieving runtime information from MAESTRODRIVER via
// CCxRuntime.  Note that actions which can change the runtime state are prevented if the current op mode does not
// match the mode handled by this mode controller object.
//
//=====================================================================================================================

BOOL CCxModeControl::UpdateVideoCfg()                    // sends video dsp cfg to MAESTRODRIVER to update video h/w
{
   if( m_pRuntime && m_pRuntime->GetMode() == m_iOpMode && CanUpdateVideoCfg() ) return(m_pRuntime->UpdateVideoCfg());
   else return( FALSE );
}

BOOL CCxModeControl::UpdateFixRewSettings()              // sends current fixation/reward settings to MAESTRODRIVER
{
   if( m_pRuntime && m_pRuntime->GetMode() == m_iOpMode && CanUpdateFixRewSettings() )
      return( m_pRuntime->UpdateFixRewSettings() );
   else return( FALSE );
}

int CCxModeControl::GetNumRewardsDelivered()             // gets/resets MAESTRODRIVER counters that tracks reward stats
{
   return( (m_pRuntime == NULL) ? 0 : m_pRuntime->GetNumRewardsDelivered() );
}
int CCxModeControl::GetCumulativeReward()
{
   return( (m_pRuntime == NULL) ? 0 : m_pRuntime->GetCumulativeReward() );
}
BOOL CCxModeControl::ResetRewardStats()
{
   if( m_pRuntime && m_pRuntime->GetMode() == m_iOpMode ) return( m_pRuntime->ResetRewardStats() );
   else return( FALSE );
}

// methods accessing MAESTRODRIVER h/w state & capabilities...
int CCxModeControl::GetMode() { return( (m_pRuntime != NULL) ? m_pRuntime->GetMode() : CX_NOTRUNNING ); }
int CCxModeControl::GetNumAO() { return( (m_pRuntime != NULL) ? m_pRuntime->GetNumAO() : 0 ); }
int CCxModeControl::GetNumAI() { return( (m_pRuntime != NULL) ? m_pRuntime->GetNumAI() : 0 ); }
int CCxModeControl::GetNumTDO() { return( (m_pRuntime != NULL) ? m_pRuntime->GetNumTDO() : 0 ); }
int CCxModeControl::GetNumTDI() { return( (m_pRuntime != NULL) ? m_pRuntime->GetNumTDI() : 0 ); }
BOOL CCxModeControl::IsAOChan( int iCh ) { return( BOOL( (iCh >= 0) && (iCh < GetNumAO()) ) ); }
BOOL CCxModeControl::IsAIChan( int iCh ) { return( BOOL( (iCh >= 0) && (iCh < GetNumAI()) ) ); }
BOOL CCxModeControl::IsTDOChan( int iCh ) { return( BOOL( (iCh >= 0) && (iCh < GetNumTDO()) ) ); }
BOOL CCxModeControl::IsTDIChan( int iCh ) { return( BOOL( (iCh >= 0) && (iCh < GetNumTDI()) ) ); }
DWORD CCxModeControl::GetHWStatus() { return( (m_pRuntime != NULL) ? m_pRuntime->GetHWStatus() : 0 ); }
BOOL CCxModeControl::IsAIAvailable() { return( BOOL((GetHWStatus() & CX_F_AIAVAIL) != 0) ); }
BOOL CCxModeControl::IsAOAvailable() { return( BOOL((GetHWStatus() & CX_F_AOAVAIL) != 0) ); }
BOOL CCxModeControl::IsTimerAvailable() { return( BOOL((GetHWStatus() & CX_F_TMRAVAIL) != 0) ); }
BOOL CCxModeControl::IsRMVideoAvailable() { return( BOOL((GetHWStatus() & CX_F_RMVAVAIL) != 0) ); }
int CCxModeControl::GetRMVideoScreenW() { return( (m_pRuntime != NULL) ? m_pRuntime->GetRMVideoScreenW() : 0 ); } 
int CCxModeControl::GetRMVideoScreenH() { return( (m_pRuntime != NULL) ? m_pRuntime->GetRMVideoScreenH() : 0 ); }
float CCxModeControl::GetRMVideoFrameRate() { return((m_pRuntime!=NULL) ? m_pRuntime->GetRMVideoFrameRate() : 0.0f); }
BOOL CCxModeControl::CanCalibAI() { return( BOOL((GetHWStatus() & CX_F_AICAL) != 0) ); }


/**
 * Can the user update the RMVideo monitor display mode or remove/add content to the RMVideo movie store? 
 * These operations are allowed ONLY in the IdleMode operational state and only if RMVideo is available.
 * @return TRUE iff RMVideo display mode switches and movie store content changes are currently permitted
 */
BOOL CCxModeControl::CanUpdateRMV()
{
   return(m_pRuntime!=NULL && IsRMVideoAvailable() &&  m_pRuntime->GetMode() == m_iOpMode && m_iOpMode == CX_IDLEMODE);
}

// (sar) these wrappers were added in Sep2009 to expose new RMVideo-related functionality. Certain methods may block
// for an extended time and so are suitable only in IdleMode and demand that a wait cursor be displayed so the user
// is aware of the delay. See CCxRuntime for detailed method descriptions.
int CCxModeControl::GetNumRMVideoModes() 
{ 
   return((m_pRuntime != NULL) ? m_pRuntime->GetNumRMVideoModes() : 0);
} 
BOOL CCxModeControl::GetRMVideoModeDesc(int i, CString& desc) 
{ 
   return((m_pRuntime != NULL) ? m_pRuntime->GetRMVideoModeDesc(i, desc) : FALSE); 
} 
int CCxModeControl::GetCurrRMVideoMode()
{ 
   return((m_pRuntime != NULL) ? m_pRuntime->GetCurrRMVideoMode() : -1);
} 
BOOL CCxModeControl::SetCurrRMVideoMode(int i)     // BLOCKS FOR UP TO 10 SECONDS
{
   if(m_pRuntime != NULL && m_pRuntime->GetMode() == m_iOpMode && m_iOpMode == CX_IDLEMODE) 
      return(m_pRuntime->SetCurrRMVideoMode(i));
   else
      return(FALSE);
}

BOOL CCxModeControl::GetRMVGamma(float& r, float& g, float& b)
{
   return((m_pRuntime != NULL) ? m_pRuntime->GetRMVGamma(r, g, b) : -1);
}
BOOL CCxModeControl::SetRMVGamma(float& r, float& g, float& b)
{
   if(m_pRuntime != NULL && m_pRuntime->GetMode() == m_iOpMode && m_iOpMode == CX_IDLEMODE) 
      return(m_pRuntime->SetRMVGamma(r, g, b));
   else
      return(FALSE);
}

int CCxModeControl::GetNumRMVMediaFolders()
{
   return((m_pRuntime != NULL) ? m_pRuntime->GetNumRMVMediaFolders() : -1);
}
BOOL CCxModeControl::GetRMVMediaFolder(int i, CString& folder)
{
   return((m_pRuntime != NULL) ? m_pRuntime->GetRMVMediaFolder(i, folder) : FALSE);
}
int CCxModeControl::GetNumRMVMediaFiles(int i)
{
   return((m_pRuntime != NULL) ? m_pRuntime->GetNumRMVMediaFiles(i) : -1);
}
BOOL CCxModeControl::GetRMVMediaInfo(int i, int j, CString& name, CString& desc)
{
   return((m_pRuntime != NULL) ? m_pRuntime->GetRMVMediaInfo(i, j, name, desc) : FALSE);
}

// NOTE: BLOCKS for up to 5 seconds
BOOL CCxModeControl::DeleteRMVMediaFile(int i, int j) 
{
   if(m_pRuntime != NULL && m_pRuntime->GetMode() == m_iOpMode && m_iOpMode == CX_IDLEMODE) 
      return(m_pRuntime->DeleteRMVMediaFile(i, j));
   else
      return(FALSE);
}
// NOTE: BLOCK for up to 120 seconds
BOOL CCxModeControl::DownloadRMVMediaFile(LPCTSTR path, int iFolder, LPCTSTR folderNew, LPCTSTR file)
{
   if(m_pRuntime != NULL && m_pRuntime->GetMode() == m_iOpMode && m_iOpMode == CX_IDLEMODE) 
      return(m_pRuntime->DownloadRMVMediaFile(path, iFolder, folderNew, file));
   else
      return(FALSE);
}


//=== Get/Set/OnChangeTraces ==========================================================================================
//
//    Access to the data trace facility managed by the MAESTRODRIVER runtime interface.  As all mode control panels
//    share the same facility, a particular panel only has access when we're in the op mode managed by that panel.
//
//    GetTraces()       -- returns key of channel config object currently assoc. w/ data trace facility.
//    SetTraces()       -- reinitialize data trace facility using the specified channel configuration.
//    OnChangeTraces()  -- inform runtime interface of a change in the channel config assoc. w/ data trace facility.
//
//    ARGS:       wKey  -- [in] key of channel config object.
//                iDur  -- [in] desired graph width for data trace, in # of input samples.
//
//    RETURNS:    key of channel configuration object (CCxChannel) now associated w/ data trace facility.  a value of
//                CX_NULLOBJ_KEY indicates data trace facility is not currently in use by the mode control panel.
//
WORD CCxModeControl::GetTraces()
{
   return( (m_pRuntime && m_pRuntime->GetMode() == m_iOpMode) ? m_pRuntime->GetTraces() : CX_NULLOBJ_KEY );
}

WORD CCxModeControl::SetTraces( const WORD wKey, const int iDur )
{
   return( (m_pRuntime && m_pRuntime->GetMode() == m_iOpMode) ? m_pRuntime->SetTraces( wKey, iDur ) : CX_NULLOBJ_KEY );
}

VOID CCxModeControl::OnChangeTraces()
{
   if( m_pRuntime && m_pRuntime->GetMode() == m_iOpMode ) m_pRuntime->OnChangeTraces();
}



//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxNullMode (mostly inline)
//
//=====================================================================================================================
//=====================================================================================================================

//=== CCxNullMode [constructor] =======================================================================================
//
CCxNullMode::CCxNullMode( CCxControlPanel* pPanel ) : CCxModeControl( CCxRuntime::NullMode, pPanel )
{
}

