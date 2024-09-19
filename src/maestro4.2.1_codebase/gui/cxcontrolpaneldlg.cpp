//===================================================================================================================== 
//
// cxcontrolpaneldlg.cpp : Implementation of CCxControlPanelDlg, the ABSTRACT base class for dialogs embedded in the 
//                         CNTRLX mode control panel, CCxControlPanel.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The ABSTRACT class CCxControlPanelDlg is part of the CNTRLX "mode control panel" framework.  As the base class for 
// all mode control panel dialogs, CCxControlPanelDlg encapsulates functionality common to all such dialogs:
//
//    Refresh() [pure virtual]:  CCxControlPanel invokes this method to refresh the appearance of each of its embedded 
//       dialogs when the runtime state changes.
//    OnUpdate() [virtual], SendUpdate():  Some CNTRLX control panel dialogs display and/or modify information that is 
//       serialized in the open CNTRLX document object (CCxDoc).  When a CNTRLX view changes the document, an extension 
//       of the doc/view framework informs CCxControlPanel which, in turn, invokes OnUpdate() on each of its embedded 
//       dialogs.  If a control panel dlg changes the CCxDoc object, it informs the doc/view framework by calling 
//       SendUpdate().
//
// All mode control panel dialogs are housed in the CNTRLX mode control panel, a tabbed dialog bar container defined 
// by the class CCxControlPanel.  More than just a container, CCxControlPanel manages a set of operational mode control 
// objects.  These "mode controllers" -- base class CCxModeControl -- coordinate with the CNTRLX runtime interface 
// CCxRuntime to implement the required functionality in each operational mode of CNTRLX.  The mode control panel 
// framework insulates the dialogs from the CNTRLX runtime interface.  The dialogs have no direct access to CCxRuntime; 
// rather, they do all their mode-related work by talking to a mode controller, accessed via GetCurrentModeCtrl() or 
// GetModeCtrl( int iOpMode ).  See also:  CCxControlPanel, CCxModeControl.
//
// ==> Implementing control panel dialogs based on CCxControlPanelDlg.
// 1) Design the control panel dialogs using MS Dev Studio's Resource Editor.  Do not set the "Visible" style, since 
// CCxControlPanel's dialog bar framework will control the visibility of the dialogs.  (If you leave this style set, 
// the dialogs do not page properly until each installed dialog has been selected once by the user!)
//
// 2) For each dialog, derive a class from CCxControlPanelDlg.  Supply the resource ID of the defining dialog template 
// to the CCxControlPanelDlg -- the framework will then take care of the rest of the creation process.  The dialog 
// class must be dynamically creatable, as the framework relies on that feature during control panel creation (see 
// CCxControlPanel::AddDlg()).
//
// 3) Some control panel dialogs may display and/or alter information that is stored in the CNTRLX document CCxDoc. 
// Thus, the CCxControlPanel framework is wired into the MFC doc/view update mechanism so that any control panel and 
// its associated dialogs are notified whenever the CNTRLX document changes.  The relevant methods are:
//
//    a) CCxControlPanelDlg::OnUpdate( CCxViewHint* pHint ).  When a new document is created/opened, OnUpdate() is 
// invoked with a NULL hint object; when the existing document is changed, a non-NULL CNTRLX view hint (CCxViewHint) is 
// provided.  Note the rough similarities to CView::OnInitialUpdate() and CView::OnUpdate(), respectively.  The base 
// class implementation of OnUpdate() does nothing; it is up to the derived class to provide the desired functionality. 
//
// TECH NOTE:  How do we wire CCxControlPanel/Dlg into the MFC doc/view update mechanism?  Since a control bar is NOT 
// based on CView, we must employ some trickery to get the same functionality in our control panels.  Our approach is 
// to use one of the CNTRLX views, CCxObjectTree, to notify our mainframe class which, in turn, notifies the mode 
// control panel object:  CCxObjectTree::OnUpdate(), OnInitialUpdate() --> CCxMainFrame::OnUpdate() --> 
// CCxControlPanel::OnUpdate() --> CCxControlPanelDlg::OnUpdate().
//
//    b) CCxControlPanelDlg::SendUpdate( CCxViewHint* pHint, bMod = TRUE ).  If document data is changed via a control 
// panel dlg, the dialog should invoke this method to inform CNTRLX views and other control panel dialogs of the 
// change.  For the most part, this is a wrapper for CDocument::UpdateAllViews(), which does the real work.  However, 
// CCxControlPanelDlg is NOT derived from CView, and the dialog pages are NOT installed in the document's list of 
// views.  Thus, a control panel dialog will receive the update hint even if it was sent by the dialog itself!  The 
// SendUpdate() method is a hack to deal with this problem.  It sets a guard flag before invoking UpdateAllViews().  
// A derived class can check the flag's state via CCxControlPanelDlg::InitiatedUpdate() and choose to ignore any update 
// hints initiated by the dialog itself.
//
//
// CREDITS:
// 1) CCxControlPanel is derived from the resizable dockable dialog bar CSizingTabDlgBar, which was built upon the MFC 
// extension Cristi Posea's CSizingControlBarCF.  See CSizingTabDlgBar implementation file for full credits.  All dlgs 
// housed in CSizingTabDlgBar must be derived from CSzDlgBarDlg, which is the base class of CCxControlPanelDlg. 
//
//
// REVISION HISTORY:
// 02apr2003-- MAJOR redesign of CCxControlPanel.  CCxControlPanelDlg implementation moved to this file, with some 
//             changes.  Most significantly, to affect runtime state in the current op mode, the dialogs "talk to" the 
//             current mode controller object (base class CCxModeControl), retrieved via GetModeCtrl().
//
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxviewhint.h"                      // CCxViewHint -- "hint" class for CNTRLX-specific doc/view updates
#include "cxdoc.h"                           // CCxDoc -- the CNTRLX document
#include "cxmodecontrol.h"                   // CCxModeControl -- base class for CNTRLX mode controller objects
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the CNTRLX mode control panel (dialog container)
#include "cxcontrolpaneldlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC( CCxControlPanelDlg, CSzDlgBarDlg )

//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== GetCurrentModeCtrl, GetModeCtrl ================================================================================= 
//
//    Retrieve ptr to the mode controller object that handles the current CNTRLX operational mode, or the mode 
//    controller governing a specified op mode.  We retrieve this by querying the CNTRLX master mode control panel, 
//    which manages all mode controllers and associated dialogs.
//
//    ARGS:       iOpMode  -- [in] a CNTRLX operational mode.
//
//    RETURNS:    ptr to the mode controller object (NULL if op mode is invalid -- ASSERTs in debug version).
//
CCxModeControl* CCxControlPanelDlg::GetCurrentModeCtrl()
{
   CCxControlPanel* pPanel = (CCxControlPanel*) GetParent();
   ASSERT( pPanel );
   ASSERT_KINDOF( CCxControlPanel, pPanel );
   CCxModeControl* pMode = pPanel->GetCurrentModeCtrl();
   ASSERT( pMode );
   return( pMode );
}

CCxModeControl* CCxControlPanelDlg::GetModeCtrl( int iOpMode )
{
   CCxControlPanel* pPanel = (CCxControlPanel*) GetParent();
   ASSERT( pPanel );
   ASSERT_KINDOF( CCxControlPanel, pPanel );
   CCxModeControl* pMode = pPanel->GetModeCtrl( iOpMode );
   ASSERT( pMode );
   return( pMode );
}


//=== SendUpdate ====================================================================================================== 
//
//    Notify CNTRLX document, views, and other control panel dialogs of a change to document data initiated in this 
//    control panel dialog.  We optionally mark the document as modified, set the guard flag to indicate that the 
//    change was initiated here, and then invoke CDocument::UpdateAllViews() to pass on the view hint provided.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific view hint (possibly NULL). 
//                bMod  -- [in] if TRUE, set document's "modified" flag; else don't (default = TRUE).
//
//    RETURNS:    NONE.
//
VOID CCxControlPanelDlg::SendUpdate( CCxViewHint* pHint, BOOL bMod /* =TRUE */ )
{
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc(); 
   ASSERT( pDoc != NULL ); 
   if( bMod ) pDoc->SetModifiedFlag(); 
   m_bInitiatedUpdate = TRUE;
   pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*)pHint );
   m_bInitiatedUpdate = FALSE;
}


