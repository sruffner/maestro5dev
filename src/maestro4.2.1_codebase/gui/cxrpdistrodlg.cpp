//=====================================================================================================================
//
// cxrpdistrodlg.cpp : Declaration of CCxRPDistroDlg, the Maestro Trial-mode control panel dialog page that administers
//                     a distribution-based reward/penalty contingency protocol.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxRPDistroDlg is the primary GUI element with which the user interacts to execute a special reward/penalty
// contingency protocol that is based on the subject's behavioral response relative to a previously collected response
// distribution.  Any trial that participates in this protocol uses the "R/P Distro" special operation, and the
// behavioral response is averaged over the designated special segment and may be one of four supported measures:
// eye velocity vector magnitude, eye velocity vector direction, H eye velocity, or V eye velocity. Such trials include
// a CCxRPDistro object that specifies the response measure type, the valid response range, and reward window; builds 
// up the response distribution; and tracks "pass/fail" statistics once a reward window is defined. For a more complete 
// discussion of the distribution-based protocol, see the CCxRPDistro class file.
//
// CCxRPDistroDlg provides a view of a trial's CCxRPDistro object. It renders current and previous sample distributions
// on a simple canvas, displays distribution "pass/fail" statistics, and includes controls for setting the response 
// measure type, the valid response range, and the reward window. A dropdown combo box selects which trial is viewed on
// the dialog. This combo box is populated with all trials in the current trial set that use the "R/P Distro" special 
// operation. During trial sequencing, each time an "R/P Distro" trial is completed, the Trial mode controller 
// CCxTrialMode submits it to CCxRPDistroDlg for display. This trial is "brought to the front" -- ie, it is selected in 
// the combo box, and the dialog controls are updated to reflect the current state of the trial's CCxRPDistro object.
//
// ==> Intended usage. The distribution-based protocol occurs in two distinct phases. In the "assessment" phase,
// a "R/P Distro" trial is presented multiple times in order to build up a distribution of responses to a particular
// stimulus paradigm. Prior to starting this phase, the user will select the behavioral response type and adjust the 
// valid response range for each "R/P Distro" trial using the relevant controls in CCxRPDistroDlg. As trials are 
// presented, the user may bring the CCxRPDistroDlg tab to the front to "watch" the distribution develop and decide 
// when enough samples have been collected. Then s/he would stop trial sequencing and use the relevant controls on 
// CCxRPDistroDlg to define a reward window and start collection of a new response distribution.  When trial sequencing
// resumes, the "reward/penalty" phase begins. Each time the subject's response falls within the defined reward window,
// the subject "passes" the distribution-based contingency (and gets an enhanced reward); otherwise, the subject 
// "fails" the test (and gets a penalty or reduced reward). Again, the user may bring up the CCxRPDistroDlg to "watch" 
// the new response distribution develop, monitor pass/fail statistics, etc.
//
// ==> Summary of controls housed on the dialog
//
//    IDC_RPD_CURR [combo box]: Lets user choose the trial for which response distributions and related information
//       are displayed on the rest of the dialog. Combo box will include the names of all trials that have been
//       submitted to the dialog for display and that are configured to participate in the distribution-based
//       reward/penalty contingency protocol. If a trial's definition is changed such that it does not include the
//       special "R/P Distro" feature, it will automatically be removed from the combo box.
//    IDC_RPD_NEWDIST [pushb]: Pressing this button starts a new response sample distribution for the trial currently
//       selected on the dialog. The trial's "current" distribution becomes the "previous" distribution, and the
//       "current" distribution is reset. Reward stats for that trial are also reset.
//    IDC_RPD_SAVE [pushb]: Press this btn to write to file a text summary of the current state of all "R/P Distro"
//       trials catalogued on this dialog. A file selection dialog lets user browse the filesystem to choose file.
//    IDC_RPD_REW_ENA [check box]: If checked, the reward window is enabled for trial currently selected on dialog.
//    IDC_RPD_REWMIN [numeric edit]: Specifies minimum bound (left edge) of reward window, in response sample units.
//    IDC_RPD_REWMAX [numeric edit]: Specifies maximum bound (right edge) of reward window, in response sample units.
//    IDC_RPD_REWSHIFT [num edit]: Specifies "dynamic shift" D, in response sample units. After N valid samples are
//       collected, Maestro may shift the reward window left or right: If D is positive and the mean response over the
//       N samples is greater than the left boundary of the window, the window is shifted left by D. If D is negative
//       and the mean response is less than the right boundary, the window is shifted right by D.
//    IDC_RPD_REWNUPD [num edit]: Specifies "dynamic update interval" N, ie, the number of valid samples collected
//       between dynamic updates of the reward window's location.
//    IDC_RPD_CURRMOSTREC [num edit]: The number of most recent valid samples to include when calculating stats and
//       displaying histogram for the "current" distribution. If zero, ALL valid samples are included.
//    IDC_RPD_PREVMOSTREC [num edit]: The number of most recent valid samples to include when calculating stats and
//       displaying histogram for the "previous" distribution. If zero, ALL valid samples are included.
//    IDC_RPD_RESPTYPE [combo box]: Selects the desired type of behavioral response measure. Any change in the 
//       response type automatically clears the accumulated current and previous distribution data.
//    IDC_RPD_RNG_MIN, IDC_RPD_RNG_MAX [num edit]:  Specifies "valid response range", in response sample units.
//
// [**NOTE: To use the ON_CONTROL_RANGE macro in the message map, the following sets of integer resource IDs must
// occupy a contiguous range: {IDC_RPD_REW_ENA...IDC_RPD_SAVE}, {IDC_RPD_REWCTR...IDC_RPD_PREVMOSTREC}, and 
// {IDC_RPD_CURR...IDC_RPD_RESPTYPE}.]
//
//    IDC_RPD_CURRMEAN, IDC_RPD_PREVMEAN, IDC_RPD_REWSTATCURR, IDC_REWSTATALL [static]:  These read-only static text
//       controls display distribution statistics (mean, std dev, #samples) and "pass/fail" statistics for the trial
//       currently selected on the dialog, as well as an overall "pass/fail" readout for all "R/P Distro" trials
//       presented since application startup. They do not respond to user input.
//    IDC_RPD_VIEW [custom static]:  This subclassed static control offers a simple graphical display of the "current"
//       and "previous" distributions (drawn as histograms) and any defined reward windows for the trial currently
//       selected on the dialog.  See CCxRPDistroView.
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
//
// CREDITS:
// (1) CCxRPDistroDlg is ultimately built upon the CSizingTabDlgBar/CSzDlgBarDlg framework which, in turn, is based on
// the MFC extension CSizingControlBarCF by Cristi Posea.  See szdlgbar.cpp for credits.
//
// REVISION HISTORY:
// 30nov2005-- Began development.
// 07dec2005-- Replaced grid control with static text controls for displaying distribution and pass/fail stats.
// 15dec2005-- Added controls for specifying the "valid response range", which sets the x-axis range for the graphical
//             display of the histograms and reward windows for the currently selected R/P Distro trial.
// 21dec2005-- Added IDC_RPD_SAVE pushbutton and method SaveSummaryToFile().
// 13mar2006-- Mods to implement revisions of the R/P Distro feature: one instead of 2 reward windows; window defined
//             by center and width; added "dynamic shift after every N valid samples"; int num edits specifying the
//             number of most recent valid samples to include when computing stats for current and previous distribs.
// 16mar2006-- Defining reward window by min/max bounds again, instead of center and width.
// 04jan2007-- Minor changes IAW recent changes in CCxTrial re: how special ops are identified.
// 25apr2007-- Mods to support four alternative behavioral response types for the RP Distro feature.
// 02dec2014-- Revised OnTrialSetChanged() to traverse all trials in the specified set, including any subsets of that 
// set. Trial subsets were introduced in v3.1.2.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for Maestro

#include "cxdoc.h"                           // CCxDoc -- Maestro experiment document class
#include "cxviewhint.h"                      // CCxViewHint -- for communicating doc/view changes...
#include "cxtrial.h"                         // CCxTrial -- Maestro trial object
#include "cxrpdistro.h"                      // CCxRPDistro -- stores runtime info for distr-based reward protocol
#include "cxmodecontrol.h"                   // CCxModeControl -- base class for Maestro mode controllers
#include "cxtrialmode.h"                     // CCxTrialMode -- the mode controller for Trial mode

#include "cxrpdistrodlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CCxRPDistroDlg, CCxControlPanelDlg )


//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

BEGIN_MESSAGE_MAP( CCxRPDistroDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_RPD_REWMIN, IDC_RPD_PREVMOSTREC, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_RPD_REW_ENA, IDC_RPD_SAVE, OnChange )
   ON_CONTROL_RANGE( CBN_SELCHANGE, IDC_RPD_CURR, IDC_RPD_RESPTYPE, OnChange )
   ON_WM_SIZE()
END_MESSAGE_MAP()


//=== OnChange ========================================================================================================
//
//    Respond to user input to any of the edit, button, or combo box controls on this dialog.  We handle 3 different
//    notifications here:
//       1) BN_CLICKED ==> If user clicked the check box IDC_RPD_REW_ENA -- in which case we enable or disable reward
//          window for the trial currently selected in the dialog.  If the user clicks the pushbutton IDC_RPD_NEWDIST,
//          we start a new response distribution on the selected trial.  If user clicks pushbutton IDC_RPD_SAVE, we
//          bring up a file dialog to let user choose destination for the distribution summary file.
//       2) EN_KILLFOCUS ==> When any of the numeric edit controls on the form loses the focus, we update the
//          corresponding parameter of the CCxRPDistro object currently viewed on the dialog.
//       3) CBN_SELCHANGE ==> User selected a different trial via the combo box IDC_RPD_CURR, or a different behavioral
//          response type via IDC_RPD_RESPTYPE. The dialog is refreshed to reflect any changes.
//
//    !!!IMPORTANT!!!
//    During GUI creation at application startup, an edit control in this dialog may lose the focus -- generating an
//    EN_KILLFOCUS notification.  However, GUI creation occurs BEFORE the CCxDoc exists -- in which case there are no
//    trials to display.  In this case, OnChange() does nothing.
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification.
//    RETURNS:    NONE
//
void CCxRPDistroDlg::OnChange( UINT id )
{
   CCxRPDistro* pDist = GetCurrentRPDistro();
   if( pDist == NULL ) return;

   BOOL bEna = FALSE;
   float fTemp = 0.0f;
   int iSel = -1;
   
   switch( id )
   {
      case IDC_RPD_CURR :
         m_distroView.SetData( pDist );
         UpdateStatReadouts();
         ReloadRewardWindowControls();
         ReloadRespRangeControls();
         break;
      case IDC_RPD_RESPTYPE :
         iSel = m_cbRespType.GetCurSel();
         if(iSel >= 0 && iSel < TH_RPD_NRESPTYPES && iSel != pDist->GetResponseType())
         {
            pDist->SetResponseType(iSel);
            m_distroView.Rebuild();
            UpdateStatReadouts();
         }
         break;
      case IDC_RPD_NEWDIST :
         pDist->StartNewDistribution();
         m_distroView.Rebuild();
         UpdateStatReadouts();
         break;
      case IDC_RPD_SAVE :
         if( ((CCxTrialMode*) GetCurrentModeCtrl())->IsSeqOffOrPaused() )
            SaveSummaryToFile();
         break;
      case IDC_RPD_REW_ENA :
         bEna = BOOL(m_btnRewEna.GetCheck() == 1);
         pDist->SetRewardWinEnabled(bEna);
         m_distroView.Invalidate(TRUE);
         break;
      case IDC_RPD_REWMIN :
         pDist->SetRewardWinMinimum( m_edRewMin.AsFloat() );
         m_edRewMin.SetWindowText( pDist->GetRewardWinMinimum() );
         m_edRewMax.SetWindowText( pDist->GetRewardWinMaximum() );
         m_distroView.Invalidate(TRUE);
         break;
      case IDC_RPD_REWMAX :
         pDist->SetRewardWinMaximum( m_edRewMax.AsFloat() );
         m_edRewMin.SetWindowText( pDist->GetRewardWinMinimum() );
         m_edRewMax.SetWindowText( pDist->GetRewardWinMaximum() );
         m_distroView.Invalidate(TRUE);
         break;
      case IDC_RPD_REWSHIFT :
         pDist->SetRewardWinShift( m_edRewShift.AsFloat() );
         m_edRewShift.SetWindowText( pDist->GetRewardWinShift() );
         break;
      case IDC_RPD_REWNUPD :
         pDist->SetRewardWinUpdateIntv( m_edRewNUpd.AsInteger() );
         m_edRewNUpd.SetWindowText( pDist->GetRewardWinUpdateIntv() );
         break;
      case IDC_RPD_CURRMOSTREC :
         pDist->SetCurrentNumMostRecent( m_edCurrMostRecent.AsInteger() );
         m_edCurrMostRecent.SetWindowText( pDist->GetCurrentNumMostRecent() );
         UpdateStatReadouts();
         m_distroView.Rebuild();
         break;
      case IDC_RPD_PREVMOSTREC :
         pDist->SetPreviousNumMostRecent( m_edPrevMostRecent.AsInteger() );
         m_edPrevMostRecent.SetWindowText( pDist->GetPreviousNumMostRecent() );
         UpdateStatReadouts();
         m_distroView.Rebuild();
         break;
      case IDC_RPD_RNG_MIN :
      case IDC_RPD_RNG_MAX :
         pDist->SetResponseRange( m_edRngMin.AsFloat(), m_edRngMax.AsFloat() );

         ReloadRespRangeControls();                               // refresh all dlg ctrls b/c a change in valid resp
         ReloadRewardWindowControls();                            // range can affect everything...
         UpdateStatReadouts();
         m_distroView.Rebuild();
         break;
   }
}

//=== OnSize ==========================================================================================================
//
//    Handler called after this dialog has been resized.  The custom control that displays the distribution histograms
//    occupies the left half of the dialog template, and we make it grow downward and rightward whenever the dialog is
//    made larger than the original template size.
//
//    ARGS:       See CWnd::OnSize().
//    RETURNS:    NONE.
//
void CCxRPDistroDlg::OnSize( UINT nType, int cx, int cy )
{
   // let base class do its thing.  If we haven't init'd dialog yet, don't do anything!
   CCxControlPanelDlg::OnSize( nType, cx, cy );
   if( !m_bInitialized ) return;

   // get the custom view control's rectangle in dialog's client coordinates
   CRect rView;
   m_distroView.GetWindowRect( rView );
   ScreenToClient( rView );

   // force view control's right and bottom edges to stick to the dialog's right and bottom edges whenever the dialog
   // is made larger than the original template size.  If it is made smaller, the view control keeps its original
   // template size.
   rView.right = rView.left + __max( cx-5-rView.left, m_minViewSize.cx );
   rView.bottom = rView.top + __max( cy-5-rView.top, m_minViewSize.cy );
   m_distroView.MoveWindow( &rView );
}


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitDialog [base override] ====================================================================================
//
//    Prepare the dialog for display.
//
//    Here we subclass dlg resource template-defined controls to class members, format the numeric edit ctrls, and
//    initialize all to "start-up" conditions.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
BOOL CCxRPDistroDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_cbCurrTrial.SubclassDlgItem(IDC_RPD_CURR, (CWnd*) this);              // subclass combo box that selects trial
   m_cbCurrTrial.ModifyStyle(CBS_SORT, CBS_DROPDOWNLIST);                  // unsorted list; readonly edit box

   m_cbRespType.SubclassDlgItem(IDC_RPD_RESPTYPE, (CWnd*) this);           // subclass combo box that selects type of 
   m_cbRespType.ModifyStyle( CBS_SORT, 0 );                                // behavioral resp measured. Load w/avail
   m_cbRespType.ResetContent();                                            // response types (index = type ID!)
   for(int i = 0; i < TH_RPD_NRESPTYPES; i++)
      m_cbRespType.AddString(CCxRPDistro::GetResponseTypeDesc(i));
   m_cbRespType.SetCurSel(0); 

   m_btnNewDist.SubclassDlgItem(IDC_RPD_NEWDIST, (CWnd*) this);            // subclass PB/check box controls
   m_btnSaveSummary.SubclassDlgItem(IDC_RPD_SAVE, (CWnd*) this);
   m_btnRewEna.SubclassDlgItem(IDC_RPD_REW_ENA, (CWnd*) this);

   m_edRewMin.SubclassDlgItem(IDC_RPD_REWMIN, (CWnd*) this);               // subclass & restrict format of numeric
   m_edRewMin.SetFormat( FALSE, FALSE, 7, 2 );                             // edit ctrls on dialog
   m_edRewMax.SubclassDlgItem(IDC_RPD_REWMAX, (CWnd*) this);
   m_edRewMax.SetFormat( FALSE, TRUE, 7, 2 );
   m_edRewShift.SubclassDlgItem(IDC_RPD_REWSHIFT, (CWnd*) this);
   m_edRewShift.SetFormat( FALSE, FALSE, 7, 2 );
   m_edRewNUpd.SubclassDlgItem(IDC_RPD_REWNUPD, (CWnd*) this);
   m_edRewNUpd.SetFormat( TRUE, TRUE, 3, 1 );
   m_edCurrMostRecent.SubclassDlgItem(IDC_RPD_CURRMOSTREC, (CWnd*) this);
   m_edCurrMostRecent.SetFormat( TRUE, TRUE, 3, 1 );
   m_edPrevMostRecent.SubclassDlgItem(IDC_RPD_PREVMOSTREC, (CWnd*) this);
   m_edPrevMostRecent.SetFormat( TRUE, TRUE, 3, 1 );
   m_edRngMin.SubclassDlgItem(IDC_RPD_RNG_MIN, (CWnd*) this);
   m_edRngMin.SetFormat( FALSE, FALSE, 7, 2 );
   m_edRngMax.SubclassDlgItem(IDC_RPD_RNG_MAX, (CWnd*) this);
   m_edRngMax.SetFormat( FALSE, FALSE, 7, 2 );

   m_distroView.SubclassDlgItem(IDC_RPD_VIEW, (CWnd*) this);               // subclass static control in which we draw
   CRect rView;                                                            // distribution histograms, reward windows.
   m_distroView.GetWindowRect( rView );                                    // Remember the ctrl's initial size, defined
   ScreenToClient( rView );                                                // by dlg template.
   m_minViewSize = rView.Size();

   m_roCurrMean.SubclassDlgItem(IDC_RPD_CURRMEAN, (CWnd*) this);           // subclass static text controls in which
   m_roPrevMean.SubclassDlgItem(IDC_RPD_PREVMEAN, (CWnd*) this);           // we display various statistics
   m_roCurrPassFail.SubclassDlgItem(IDC_RPD_REWSTATCURR, (CWnd*) this);
   m_roSummaryPassFail.SubclassDlgItem(IDC_RPD_REWSTATALL, (CWnd*) this);

   m_bEnabled = m_cbCurrTrial.IsWindowEnabled();                           // initial enable state of controls

   ReloadRewardWindowControls();                                           // init contents of reward window ctrls
   UpdateStatReadouts();                                                   // and static ctrls that display stats
   return( TRUE );
}

//=== Refresh [base override] =========================================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    Whenever a trial sequence is in progress, all controls are disabled.  This is also the case if there are no
//    "R/P Distro" trials available for display in the dialog!
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::Refresh()
{
   BOOL bEna = ((CCxTrialMode*) GetCurrentModeCtrl())->IsSeqOffOrPaused() && (GetCurrentRPDistro() != NULL);
   if( (bEna && !m_bEnabled) || (m_bEnabled && !bEna) )
   {
      m_bEnabled = bEna;
      m_cbCurrTrial.EnableWindow( m_bEnabled );
      m_cbRespType.EnableWindow(m_bEnabled);
      m_btnNewDist.EnableWindow( m_bEnabled );
      m_btnSaveSummary.EnableWindow( m_bEnabled );
      m_btnRewEna.EnableWindow( m_bEnabled );
      m_edRewMin.EnableWindow( m_bEnabled );
      m_edRewMax.EnableWindow( m_bEnabled );
      m_edRewShift.EnableWindow( m_bEnabled );
      m_edRewNUpd.EnableWindow( m_bEnabled );
      m_edCurrMostRecent.EnableWindow( m_bEnabled );
      m_edPrevMostRecent.EnableWindow( m_bEnabled );
      m_edRngMin.EnableWindow( m_bEnabled );
      m_edRngMax.EnableWindow( m_bEnabled );
   }
}

//=== OnUpdate [base override] ========================================================================================
//
//    CCxControlPanelDlg::OnUpdate() is a CNTRLX-specific extension of MFC's mechanism -- aka, CView::OnUpdate() -- for
//    informing all document views when one of those views causes a change in the active document's contents.  It
//    passes on the CNTRLX-specific doc/view hint (CCxViewHint) to the CNTRLX control panel dialogs, which may contain
//    document data.  When the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI apps,
//    this call is made each time a new document is created/opened -- giving us an opportunity to perform any "per-
//    document" initializations.
//
//    Here we validate the object keys of all trials currently included in the combo box on this dialog.  If the key
//    no longer references an existing trial in the current document, or if that trial no longer uses the "R/P Distro"
//    special operation, the key is discarded.  The contents of the combo box and all other controls on the dialog
//    are refreshed if necessary.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::OnUpdate( CCxViewHint* pHint )
{
   int i;

   // if there are no trials yet catalogued in this dialog, then there's nothing to do.
   if( m_wArTrialKeys.GetSize() == 0 )
      return;

   // if a new document is created/opened, then any trial keys stored in this dialog must be removed
   if( pHint == NULL )
   {
      m_wArTrialKeys.RemoveAll();
      m_cbCurrTrial.ResetContent();
      m_cbRespType.SetCurSel(0);
      m_distroView.SetData(NULL);
      UpdateStatReadouts();
      ReloadRewardWindowControls();
      ReloadRespRangeControls();
      Refresh();                          // this will disable all controls
      return;
   }

   // remember the key of the trial that's currently selected, in case we remove it b/c the trial no longer exists or
   // no longer uses the "R/P Distro" special op
   int iSel = m_cbCurrTrial.GetCurSel();
   ASSERT( iSel >= 0 && iSel < m_wArTrialKeys.GetSize() );
   WORD wCurrKey = m_wArTrialKeys[iSel];

   // check every trial catalogued in this dialog.  Remove the key of any trial that no longer exists or no longer uses
   // the "R/P Distro" special operation.
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   ASSERT( pDoc != NULL );

   BOOL bChanged = FALSE;
   i=0;
   while( i < m_wArTrialKeys.GetSize() )
   {
      WORD w = m_wArTrialKeys[i];
      BOOL bRemove = !pDoc->ObjExists(w);
      if( !bRemove )
         bRemove = (pDoc->GetObjType(w) != CX_TRIAL);
      if( !bRemove )
         bRemove = !(((CCxTrial*) pDoc->GetObject(w))->GetSpecialOp() == TH_SOP_RPDISTRO);

      if( bRemove )
      {
         bChanged = TRUE;
         m_wArTrialKeys.RemoveAt(i);
      }
      else
         ++i;
   }

   // if we did not alter our catalogue of trial keys, then all that remains is to check to see if a trial was renamed
   // -- in which case we update the combo box control accordingly.
   if( !bChanged )
   {
      if( pHint->m_code == CXVH_NAMOBJ && pHint->m_type == CX_TRIAL )
      {
         int iWhich = -1;
         for( i=0; i<m_wArTrialKeys.GetSize(); i++ ) if( m_wArTrialKeys[i] == pHint->m_key )
         {
            iWhich = i;
            break;
         }

         if( iWhich >= 0 )
         {
            if( iWhich == iSel ) m_cbCurrTrial.SetCurSel(-1);
            m_cbCurrTrial.DeleteString( iWhich );
            m_cbCurrTrial.InsertString( iWhich, (LPCTSTR) pDoc->GetObjName(pHint->m_key) );
            if( iWhich == iSel ) m_cbCurrTrial.SetCurSel(iSel);
         }
      }
      return;
   }

   // if we get here, we've removed one or more trial keys.  Repopulate the combo box with the remaining keys.  If
   // the selected trial was one of the ones removed, then select first trial that remains (if any) and refresh the
   // rest of the dialog.
   m_cbCurrTrial.ResetContent();
   iSel = -1;
   for( i=0; i<m_wArTrialKeys.GetSize(); i++ )
   {
      m_cbCurrTrial.AddString( (LPCTSTR) pDoc->GetObjName(m_wArTrialKeys[i]) );
      if( wCurrKey == m_wArTrialKeys[i] )
         iSel = i;
   }

   if( m_wArTrialKeys.GetSize() > 0 )
      m_cbCurrTrial.SetCurSel( iSel >= 0 ? iSel : 0 );

   if( iSel < 0 )
   {
      m_distroView.SetData(GetCurrentRPDistro());
      UpdateStatReadouts();
      ReloadRewardWindowControls();
      ReloadRespRangeControls();
      if( m_wArTrialKeys.GetSize() == 0 ) Refresh();     // disable all controls if no trials remain!
   }
}

//=== OnTrialSetChanged ===============================================================================================
//
//    The Trial mode controller (CCxTrialMode) invokes this method each time the user selects a new trial set to be
//    sequenced.  CCxRPDistroDlg takes this opportunity to catalog any trials in the selected set that use the
//    "R/P Distro" special operation.  Then the user can use CCxRPDistroDlg to adjust the "valid response range" for
//    each "R/P Distro" trial prior to starting the trial sequence.
//
//    As of v3.1.2, a trial set can contain "subsets" of trials. This method was revised to ensure it traverses ALL
//    trials in the specified trial set, including trials that are ensconced within subsets of that set.
//
//    ARGS:       wSet  -- [in] Maestro object key for the trial set currently loaded in the Trial mode control panel.
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::OnTrialSetChanged( WORD wSet )
{
   // if the key provided does not reference a trial set object, do nothing
   if( wSet == CX_NULLOBJ_KEY ) return;

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   if( pDoc == NULL ) return;

   if( !pDoc->ObjExists(wSet) ) return;
   if( pDoc->GetObjType(wSet) != CX_TRIALSET ) return;

   // catalog each "R/P Distro" trial in the specified trial set -- unless we already have a reference to it.
   int nCatalogued = static_cast<int>(m_wArTrialKeys.GetSize());
   CWordArray wArTrials;
   pDoc->GetTrialKeysIn(wSet, wArTrials);
   for(int i=0; i<wArTrials.GetSize(); i++)
   {
      // skip trials that don't use the R/P Distro op
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(wArTrials[i]);
      if(pTrial->GetSpecialOp() != TH_SOP_RPDISTRO) continue;

      // is the trial already catalogued here?
      BOOL bFound = FALSE;
      for(int j=0; j<nCatalogued && !bFound; j++ ) bFound = BOOL(m_wArTrialKeys[j] == wArTrials[i]);

      // if not, add it!
      if(!bFound)
      {
         m_wArTrialKeys.Add(wArTrials[i]);
         m_cbCurrTrial.AddString(pTrial->Name());
      }
   }

   // if our catalog was empty initially and now we've added at least one trial, go ahead and select first one in list
   // and update the dialog accordingly
   if(nCatalogued == 0 && m_wArTrialKeys.GetSize() > 0)
   {
      m_cbCurrTrial.SetCurSel(0);
      m_distroView.SetData( GetCurrentRPDistro() );
      ReloadRewardWindowControls();
      ReloadRespRangeControls();
      UpdateStatReadouts();
      Refresh();                                       // enable controls since there are now some trials to show!
   }
}

//=== OnTrialDone =====================================================================================================
//
//    The Trial mode controller (CCxTrialMode) invokes this method each time a trial is presented that uses the
//    "R/P Distro" special operation.  This method updates the CCxRPDistroDlg accordingly:
//       1) It makes sure the specified trial is the one selected for display on the dialog.
//       2) Updates summary reward/penalty stats based on the trial result.
//       3) Refreshes all controls on the dialog as appropriate.
//
//    The method assumes the trial's CCxRPDistro object has already been updated IAW the trial outcome.  CCxRPDistroDlg
//    depends on that object to update its appearance.
//
//    ARGS:       wKey  -- [in] Maestro object key for the trial that was just presented.
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::OnTrialDone( WORD wKey )
{
   // if key does not refer to an existing trial using the "R/P Distro" object, ignore it
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   if( pDoc == NULL ) return;
   if( !pDoc->ObjExists(wKey) ) return;
   CTreeObj* pObj = pDoc->GetObject(wKey);
   if( pObj->DataType() != CX_TRIAL ) return;
   CCxTrial* pTrial = (CCxTrial*) pObj;
   if( pTrial->GetSpecialOp() != TH_SOP_RPDISTRO ) return;

   // it is assumed that the trial was just completed successfully.  Check last result and update overall "pass/fail"
   // stats accordingly:  res=0 means "fail", res=1 means "pass"; otherwise, reward window not yet defined.
   int iRes = pTrial->GetRPDistro()->GetLastResult();
   if( iRes == 0 ) ++m_nFailed;
   else if( iRes == 1 ) ++m_nPassed;

   // is trial key already among the list of keys for R/P Distro trials we've encountered before?
   int iFound = -1;
   for( int i=0; i<m_wArTrialKeys.GetSize(); i++ ) if( m_wArTrialKeys[i] == wKey )
   {
      iFound = i;
      break;
   }

   // if we already know about this trial, we make sure it is the currently selected trial in the dialog.  Otherwise,
   // we add it to our list of "R/P Distro" trials -- the dialog can track any number of such trials in any number of
   // trial sets!
   BOOL bSame = FALSE;
   if( iFound > -1 )
   {
      if( iFound == m_cbCurrTrial.GetCurSel() )
         bSame = TRUE;
      else
         m_cbCurrTrial.SetCurSel( iFound );
   }
   else
   {
      m_wArTrialKeys.Add( wKey );
      int pos = m_cbCurrTrial.AddString( pTrial->Name() );
      m_cbCurrTrial.SetCurSel( pos );
   }

   // update the rest of the dialog
   if( bSame )
   {
      m_distroView.RebuildCurrent();

      // we need to reload reward window and response range controls if the reward window has been auto-shifted
      CCxRPDistro* pDist = GetCurrentRPDistro();
      if( (pDist->GetRewardWinShift() != 0.0f) && (pDist->GetRewardWinMinimum() != m_edRewMin.AsFloat()) )
      {
         ReloadRewardWindowControls();
         ReloadRespRangeControls();      // response range is adjusted to accommodate shift in reward window!
      }
   }
   else
   {
      m_distroView.SetData( GetCurrentRPDistro() );
      ReloadRewardWindowControls();
      ReloadRespRangeControls();
   }
   UpdateStatReadouts();
}

//=== GetCurrentRPDistro ==============================================================================================
//
//    Retrieve response distribution object for the trial currently selected in this CCxRPDistroDlg.
//
//    ARGS:       NONE.
//    RETURNS:    Pointer to trial's CCxRPDistro object, encapsulating runtime response distribution data and reward
//                window bounds.  If there are no "R/P Distro" trials, this method returns NULL.
//
CCxRPDistro* CCxRPDistroDlg::GetCurrentRPDistro()
{
   int iSel = m_cbCurrTrial.GetCurSel();
   if( iSel < 0 || iSel >= m_wArTrialKeys.GetSize() )
      return( NULL );
   else
   {
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(m_wArTrialKeys[iSel]);
      return( pTrial->GetRPDistro() );
   }
}

//=== ReloadRewardWindowControls, ReloadRespRangeControls, UpdateStatReadouts =========================================
//
//    Helper methods that reload various controls on CCxRPDistroDlg to reflect information in the selected trial's 
//    CCxRPDistro object:  reward window parameters, response type and valid response range, and the distribution and 
//    pass/fail stats.
//
//    NOTE:  ReloadRewardWindowControls() also reloads the controls IDC_RPD_CURRMOSTREC and IDC_RPD_PREVMOSTREC, even
//    though these have nothing to do with the reward window. Similarly, ReloadRespRangeControls() reloads the combo 
//    box IDC_RPD_RESPTYPE, even though it has nothing to do with response range.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::ReloadRewardWindowControls()
{
   CCxRPDistro* pDist = GetCurrentRPDistro();
   if( pDist == NULL )
   {
      // if no trial is selected, then reset state of the reward window controls
      m_btnRewEna.SetCheck( 0 );
      m_edRewMin.SetWindowText( 0.0f );
      m_edRewMax.SetWindowText( 0.0f );
      m_edRewShift.SetWindowText( 0.0f );
      m_edRewNUpd.SetWindowText( 0 );

      // these have nothing to do with reward window, but didn't want to create another "reload" fcn for them
      m_edCurrMostRecent.SetWindowText( 0 );
      m_edPrevMostRecent.SetWindowText( 0 );
   }
   else
   {
      // reload controls to reflect current state of reward windows
      m_btnRewEna.SetCheck( pDist->IsRewardWinEnabled() ? 1 : 0 );
      m_edRewMin.SetWindowText( pDist->GetRewardWinMinimum() );
      m_edRewMax.SetWindowText( pDist->GetRewardWinMaximum() );
      m_edRewShift.SetWindowText( pDist->GetRewardWinShift() );
      m_edRewNUpd.SetWindowText( pDist->GetRewardWinUpdateIntv() );

      // these have nothing to do with reward window, but didn't want to create another "reload" fcn for them
      m_edCurrMostRecent.SetWindowText( pDist->GetCurrentNumMostRecent() );
      m_edPrevMostRecent.SetWindowText( pDist->GetPreviousNumMostRecent() );
   }
}

VOID CCxRPDistroDlg::ReloadRespRangeControls()
{
   CCxRPDistro* pDist = GetCurrentRPDistro();
   if( pDist == NULL )
   {
      // if no trial is selected, then reset state of the response range controls
      m_edRngMin.SetWindowText( 0.0f );
      m_edRngMax.SetWindowText( 0.0f );

      m_cbRespType.SetCurSel(0);
   }
   else
   {
      // reload controls to reflect the valid response range for the currently selected trial
      float fMin = 0.0f;
      float fMax = 0.0f;
      pDist->GetResponseRange(fMin, fMax);
      m_edRngMin.SetWindowText( fMin );
      m_edRngMax.SetWindowText( fMax );

      m_cbRespType.SetCurSel(pDist->GetResponseType());
   }
}

VOID CCxRPDistroDlg::UpdateStatReadouts()
{
   CCxRPDistro* pDist = GetCurrentRPDistro();
   CString msg = _T("N/A");
   if( pDist == NULL )
   {
      // if there is no current trial selected, most readouts are "N/A"
      m_roCurrMean.SetWindowText( msg );
      m_roPrevMean.SetWindowText( msg );
      m_roCurrPassFail.SetWindowText( msg );

      msg.Format( "pass = %d, fail = %d", m_nPassed, m_nFailed );
      m_roSummaryPassFail.SetWindowText( msg );
   }
   else
   {
      // if "# most recent" is nonzero, then statistics reflect the # of most recent valid samples, NOT all valid samples.
      int n = pDist->GetNumValidCurrentSamples();
      int nMostRec = pDist->GetCurrentNumMostRecent();
      if( nMostRec > 0 ) n = __min(n,nMostRec);
      msg.Format( "%.1f +/- %.2f (N=%d)", pDist->GetCurrentMean(), pDist->GetCurrentStdDev(), n );
      m_roCurrMean.SetWindowText( msg );

      n = pDist->GetNumValidPreviousSamples();
      nMostRec = pDist->GetPreviousNumMostRecent();
      if( nMostRec > 0 ) n = __min(n,nMostRec);
      msg.Format( "%.1f +/- %.2f (N=%d)", pDist->GetPreviousMean(), pDist->GetPreviousStdDev(), n );
      m_roPrevMean.SetWindowText( msg );

      msg.Format( "pass = %d, fail = %d", pDist->GetNumPassed(), pDist->GetNumFailed() );
      m_roCurrPassFail.SetWindowText( msg );

      msg.Format( "pass = %d, fail = %d", m_nPassed, m_nFailed );
      m_roSummaryPassFail.SetWindowText( msg );
   }
}


//=== SaveSummaryToFile ===============================================================================================
//
//    Save summary of all R/P Distro trials currently catalogued in this CCxRPDistroDlg.  Statistics are saved to a
//    text file, the path of which is obtained from the user via a standard file "save" dialog.
//
//    NOTE:  Since this method writes to the file system, it must never be invoked when Maestro is actively sequencing
//    trials during Trial mode.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxRPDistroDlg::SaveSummaryToFile()
{
   CString strMsg;
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();

   // set up standard file save dialog
   CFileDialog fileDlg( FALSE, _T("txt"), _T("rpdsummary.txt"), OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
      _T("Text Files (*.txt)|*.txt||"), this);
   fileDlg.m_ofn.lpstrInitialDir = (LPCTSTR) pApp->GetMRUTrialDataDirectory();
   fileDlg.m_ofn.lpstrTitle = _T("Save R/P Distribution Summary");

   // let user select path to which data is saved; abort if user cancelled
   INT_PTR iRes = fileDlg.DoModal();
   if( iRes != IDOK ) return;
   CString strPath = fileDlg.GetPathName();

   // open the file; if it exists, its existing content is lost
   CStdioFile file;
   UINT nOpenFlags = CFile::modeCreate |                                // create new file or truncate existing one
                     CFile::shareExclusive |                            // share it with no one
                     CFile::modeWrite | CFile::typeText;                // open it as write-only in text mode
   if( !file.Open(strPath, nOpenFlags ) )
   {
      strMsg.Format( "(!!) Unable to open text file %s for R/P distro summary", strPath);
      ::AfxMessageBox( strMsg );
      pApp->LogMessage( strMsg );
      return;
   }

   // we need to retrieve each trial from the current experiment document
   CCxDoc* pDoc = pApp->GetDoc();
   ASSERT( pDoc != NULL );

   // write summary info for all R/P Distro trials to the file
   BOOL bOk = TRUE;
   try
   {
      // overall pass/fail stats
      strMsg.Format( "Overall:  pass=%d, fail=%d\n\n", m_nPassed, m_nFailed );
      file.WriteString( strMsg );

      // text summary for each R/P Distro trial
      for( int i=0; i<m_wArTrialKeys.GetSize(); i++ )
      {
         CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(m_wArTrialKeys[i]);
         if( pTrial->GetSpecialOp() == TH_SOP_RPDISTRO )
         {
            strMsg.Format( "Trial name: %s\n", pTrial->Name() );
            file.WriteString( strMsg );

            pTrial->GetRPDistro()->GetTextSummary( strMsg, m_distroView.GetNumHistogramBins() );
            strMsg += _T("\n\n");
            file.WriteString( strMsg );
         }
      }
   }
   catch( CFileException *fe )
   {
      fe->Delete();
      bOk = FALSE;
   }

   // close the file
   try
   {
      file.Close();
   }
   catch (CFileException *fe) { fe->Delete(); }

   // if operation failed, warn user
   if( !bOk )
   {
      strMsg = _T("(!!) File I/O exception occurred while writing R/P distro summary.");
      ::AfxMessageBox( strMsg );
      pApp->LogMessage( strMsg );
   }
}

