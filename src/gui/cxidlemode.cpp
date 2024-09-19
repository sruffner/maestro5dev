//===================================================================================================================== 
//
// cxidlemode.cpp :  Declaration of the IdleMode controller CCxIdleMode.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Each "operational mode" of CNTRLX has a "mode controller" which controls the runtime behavior of CNTRLX and CXDRIVER 
// in that mode.  CCxIdleMode is the mode controller for CNTRLX's "IdleMode", in which CXDRIVER is idle and yielding 
// the vast majority of the CPU time to CNTRLX.  Operators create and edit experimental protocols in this mode. 
//
// ==> The CNTRLX "Mode Control" Framework.
// CNTRLX's master mode control panel CCxControlPanel is implemented as a dockable dialog bar containing one or more 
// tabbed dialogs.  All dialogs that affect runtime state in any CNTRLX operational mode are installed in this 
// container, although only a subset of them will be accessible in any given mode.  In addition to its role as a 
// dialog container, CCxControlPanel constructs a "mode controller" object for each op mode, and it handles mode 
// switches by invoking appropriate methods on the relevant mode controllers.  Each mode controller, interacting with 
// the operator via some subset of the mode control panel dialogs, encapsulates the runtime behavior of CNTRLX and 
// CXDRIVER in a particular operational mode.  To communicate with CXDRIVER, it must invoke methods on the CNTRLX 
// runtime interface, CCxRuntime.  By design, the mode controller should insulate the mode control dialogs from 
// CCxRuntime.  In other words, it provides all the methods needed by the dialogs to realize the desired functionality 
// of the operational mode that controller and the associated dialogs represent.  Multiple dialogs allow us to break up 
// that functionality into logically grouped, more manageable chunks.
//
// We define two ABSTRACT classes that serve as the foundations for this "mode control" framework.  CCxModeControl is 
// the base class for all CNTRLX mode controller objects, while CCxControlPanelDlg is the base class for any dialog 
// that is installed in CCxControlPanel.  CCxModeControl handles tasks that are common to all mode controllers and 
// defines a set of abstract methods that any realizable mode controller must implement; CCxControlPanelDlg does the 
// same for mode control dialog objects.
//
// See the implementation files for CCxControlPanel, CCxControlPanelDlg, and CCxModeControl for more details.
//
// ==> CCxIdleMode.
// CCxIdleMode is the mode controller for IdleMode operations.  Since CXDRIVER is idle in this mode, CCxIdleMode does 
// very little except to make available two dialogs that specify various application settings that can affect runtime 
// operation (see below), and a third that manages the contents of the RMVideo "movie store".
//
// ==> CCxFixRewDlg, the "Fix/Reward" dialog.
// This dialog page provides a window into the CNTRLX document's fixation and reward settings, a subset of the 
// application level settings encapsulated by the CCxSettings object. The CCxFixRewDlg page is designed for use in any 
// CNTRLX op mode, not just IdleMode.  For details, see the files CXFIXREWDLG.*.  CCxIdleMode will install this dialog 
// in the mode control panel ONLY if it is not already there (see InitDlgs()).
//
// ==> CCxVideoDspDlg, the "Video Display" dialog.
// This dialog page is a window into the XY and FB video display parameters that are a subset of CNTRLX's application 
// settings, also encapsulated by the CCxSettings object within the currently open CNTRLX doc.  Like CCxFixRewDlg, the 
// CCxVideoDspDlg page may be used in more than one CNTRLX op mode.  For details, see files CXVIDEODSPDLG.*.  Again, 
// CCxIdleMode will install this dialog in the mode control panel ONLY if it is not already there.
//
// ==> CCxEyelinkDlg, the "Eyelink" dialog.
// Contains controls for connecting/disconnecting from the Eyelink 1000+ eye tracker, adjusting calibration parameters.
//
// ==> CCxRMVStoreDlg, the "RMVideo Media" dialog.
// This dialog page, added in Sep 2009, lets the user view and manage the contents of the "media store" maintained on
// the RMVideo server. It was renamed "RMVideo Media" instead of "RMVideo Movies" when support for displaying static
// images was added in Maestro V3.3.1. The page is intended only for use in IdleMode. For details, see CXRMVSTOREDLG.H.
//
//
// REVISION HISTORY:
// 07apr2003-- Created as part of a major redesign of CNTRLX's "mode control panel" framework.  Prior to this redesign, 
//             there was no "mode controller" for IdleMode.
// 09sep2009-- Added new dialog, CCxRMVStoreDlg, that displays/manages RMVideo movie store content.
// 31aug2015-- Added Eyelink dialog page, CCxEyelinkDlg.
// 11oct2016-- Tab name for CCxRMVStoreDlg is now "RMVideo Media", reflecting the fact that RMVideo's "media store" can
//             contain both video files for the RMV_MOVIE target and image files for the RMV_IMAGE target.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"                            // runtime math stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxdoc.h"                           // CCxDoc -- CNTRLX document class
#include "cxfixrewdlg.h"                     // CCxFixRewDlg -- the "Fix/Reward" dialog page
#include "cxvideodspdlg.h"                   // CCxVideoDspDlg -- the "Video Display" dialog page
#include "cxrmvstoredlg.h"                   // CCxRMVStoreDlg -- the "RMVideo Media" dialog page
#include "cxeyelinkdlg.h"                    // CCxEyelinkDlg -- the "Eyelink" dialog page
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the CNTRLX master mode control panel
#include "cxruntime.h"                       // CCxRuntime -- the CNTRLX/CXDRIVER runtime interface

#include "cxidlemode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxIdleMode [constructor] =======================================================================================
//
CCxIdleMode::CCxIdleMode( CCxControlPanel* pPanel ) : CCxModeControl( CCxRuntime::IdleMode, pPanel )
{
   m_pFixRewDlg = NULL;
   m_pVideoDspDlg = NULL;
   m_pRMVStoreDlg = NULL;
   m_pEyelinkDlg = NULL;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== InitDlgs [CCxModeControl override] ============================================================================== 
//
//    Install, in the CNTRLX master mode control panel, those dialogs required for operator interactions in the CNTRLX 
//    operational mode represented by this mode controller.
//
//    Some of the dialogs available in IdleMode are also used in other op modes and may have already been installed by 
//    another mode controller. If they have not yet been installed, we install them here; else, we merely save 
//    pointers to them.
//
//    ARGS:       NONE. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise (failed to create one of the required dialogs).
//
BOOL CCxIdleMode:: InitDlgs()
{
   ASSERT( m_pCtrlPanel );                                              // verify our ptr to the mode control panel
   ASSERT_KINDOF( CCxControlPanel, m_pCtrlPanel );

   m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxFixRewDlg) );
   if( m_pFixRewDlg == NULL )
   {
      m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->AddDlg( _T("Fix/Reward"), RUNTIME_CLASS(CCxFixRewDlg) ); 
      if( m_pFixRewDlg == NULL ) return( FALSE );
   }

   m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxVideoDspDlg) );
   if( m_pVideoDspDlg == NULL )
   {
      m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->AddDlg( _T("Video Display"), RUNTIME_CLASS(CCxVideoDspDlg) ); 
      if( m_pVideoDspDlg == NULL ) return( FALSE );
   }

   m_pRMVStoreDlg = (CCxRMVStoreDlg*) m_pCtrlPanel->GetDlgByClass(RUNTIME_CLASS(CCxRMVStoreDlg));
   if(m_pRMVStoreDlg == NULL)
   {
      m_pRMVStoreDlg = (CCxRMVStoreDlg*) m_pCtrlPanel->AddDlg(_T("RMVideo Media"), RUNTIME_CLASS(CCxRMVStoreDlg)); 
      if(m_pRMVStoreDlg == NULL) return(FALSE);
   }

   m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxEyelinkDlg) );
   if( m_pEyelinkDlg == NULL )
   {
      m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->AddDlg( _T("EyeLink"), RUNTIME_CLASS(CCxEyelinkDlg) ); 
      if( m_pEyelinkDlg == NULL ) return( FALSE );
   }

   return(TRUE);
}


//=== Enter, Exit [CCxModeControl overrides] ========================================================================== 
//
//    Enter() should perform any initializations upon entering the operational mode represented by the mode controller, 
//    while Exit() handles any cleanup activities just prior to exiting the mode.  One task that the mode controller 
//    must handle is to update the subset of dialogs that are accessible on the mode control panel IAW the current op 
//    mode.  It is recommended that the mode controller "hide" all dialogs in Exit(), and "show" only the relevant 
//    dialogs in Enter().
//
//    Since CXDRIVER is idle in this mode, the primary task here is to update the subset of dlgs accessible on the 
//    master mode control panel. The "RMVideo Movies" dialog is reloaded just to ensure that its content is up-to-date
//    (in case CXDRIVER was restarted since the last time we were in IdleMode).
//
//    ARGS:       NONE. 
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxIdleMode::Enter()
{
   // must be in IdleMode
   ASSERT(m_pRuntime); 
   if(m_pRuntime->GetMode() != CCxRuntime::IdleMode) return(FALSE);

   // show the relevant dialog pages, with the "Video Display" dlg in front initially. The "RMVideo Media" tab dlg is
   // shown only if RMVideo is available; if so, that dialog is reloaded to make sure its contents are up-to-date (in
   // case user added/removed movies via direct interaction with the RMVideo machine...)
   ASSERT(m_pCtrlPanel);
   m_pCtrlPanel->ShowDlg(m_pFixRewDlg, -1);
   m_pCtrlPanel->ShowDlg(m_pVideoDspDlg, -1);
   if(IsRMVideoAvailable()) m_pCtrlPanel->ShowDlg(m_pRMVStoreDlg, -1);
   m_pCtrlPanel->ShowDlg(m_pEyelinkDlg, -1);
   m_pCtrlPanel->SetActiveDlg(m_pVideoDspDlg);
   if(IsRMVideoAvailable()) m_pRMVStoreDlg->Load();
   
   // refresh dialog's appearance (update control enable states; NOT a reload)
   Refresh();
   return(TRUE);
}

BOOL CCxIdleMode::Exit()
{
   ASSERT( m_pRuntime );                                                // MUST be in IdleMode!
   if( m_pRuntime->GetMode() != CCxRuntime::IdleMode ) return( FALSE );

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->HideDlg( NULL );                                       // hide all mode ctrl dlgs currently visible 

   return( TRUE );
}
