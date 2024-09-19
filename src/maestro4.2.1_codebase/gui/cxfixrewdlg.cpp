//===================================================================================================================== 
//
// cxfixrewdlg.cpp : Declaration of CCxFixRewDlg, a CNTRLX control panel dialog page for modifying subject's fixation 
//                   requirements and reward settings.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// A single CCxSettings object in the CNTRLX experiment document (CCxDoc) encapsulates all "application-level" settings 
// in CNTRLX.  Among these are a variety of fixation requirements and reward options which are important in training 
// and motivating the animal subject during CNTRLX experimental protocols.  CCxFixRewDlg serves as the user's "window" 
// into the current state of these fixation/reward settings.
//
// See CCxSettings for a more detailed explanation of the individual fixation & reward settings.
//
// We implement CCxFixRewDlg as a dialog page within the CNTRLX master mode control panel (CCxControlPanel) so that we 
// can provide the user with convenient access to the settings in any operational mode, as need be.  Each CNTRLX op 
// mode is governed by a mode controller object derived from CCxModeControl.  Mode control dialogs like CCxFixRewDlg 
// get access to the current mode controller via the base class method CCxControlPanelDlg::GetCurrentModeCtrl().  By 
// design, whenever the user modifies any setting on the dialog, CCxFixRewDlg invokes a method on the current mode 
// controller, which will send the modified fixation & reward settings to CXDRIVER.  In certain CNTRLX runtime states, 
// changes to the fixation reward settings are inappropriate (see CCxModeControl::CanUpdateFixRewSettings()).  In these 
// circumstances, the controls on this dialog will be disabled.
//
// A static edit control (IDC_FIX_NREWARDS) on the dialog reflects the # of rewards that have been delivered to the 
// animal since the statistic was last reset.  User resets the #rewards by pressing the pushbutton IDC_FIX_RESETREW. 
// Parent control panel is responsible for updating the #rewards statistics, and provides a method for resetting the 
// statistic.
//
// ==> Summary of controls housed on the dialog
//
//    IDC_FIX_DUR [numeric edit]:  Required fixation duration (ms).
//    IDC_FIX_HACC, IDC_FIX_VACC [numeric edit]:  Horizontal and vertical fixation accuracies (deg subtended at eye). 
//    IDC_FIX_REW1, IDC_FIX_REW2 [numeric edit]:  Durations of reward pulses 1 & 2 (ms).
//    IDC_FIX_REWMULT [numeric edit]: Global reward pulse length multiplier.
//    IDC_FIX_VRATIO [numeric edit]:  Chooses the "variable ratio" for random withholding of physical rewards.  VR = N 
//       means that 1 of every N earned rewards is randomly withheld on average.  N=1 disables random withholding.
//    IDC_FIX_AUDIOREW [numeric edit]:  Specifies duration (in ms) of a separate audio "cue" that is played to subject 
//       whenever a "physical" reward (liquid) has been earned -- whether or not the reward was randomly withheld.
//
// [**NOTE: The above set of integer resource IDs should represent a contiguous range of values so that we can use the 
// macro ON_CONTROL_RANGE in the message map.]
//
//    IDC_FIX_REWOVR [check box]:  If checked, the reward pulse 1 & 2 durations in this dialog override the values 
//       specified in a trial definition when that trial is executed. 
//    IDC_FIX_REWBEEP [check box]:  If checked, a "beep" is played on the host PC's onboard speaker to inform the user 
//       that a physical reward was delivered (the beep is not played if the reward was withheld).
//    IDC_FIX_RESETREW [pushb]:  When pressed, the reward statistics (see below) are reset.
//
// [**NOTE: The above set of integer resource IDs should represent a contiguous range of values so that we can use the 
// macro ON_CONTROL_RANGE in the message map.]
//
//    IDC_FIX_NREWARDS [readonly numeric edit]:  Reports the total # of physical rewards that have been delivered to 
//       the subject since the reward statistics were last reset.
//    IDC_FIX_REWARDSUM [readonly numeric edit]:  Reports the sum of the pulse lengths of rewards delivered to the 
//       subject since the reward statistics were last reset.  Only accurate for rigs that use the "variable reward 
//       pulse" device to control reward delivery.
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
// (1) CCxFixRewDlg is ultimately built upon the CSizingTabDlgBar/CSzDlgBarDlg framework which, in turn, is based on 
// the MFC extension CSizingControlBarCF by Cristi Posea.  See szdlgbar.cpp for credits.
//
// REVISION HISTORY:
// 13oct2002-- Began development.
// 17oct2002-- Initial development complete.
// 07apr2003-- MAJOR redesign of the CNTRLX "mode control" framework.  There is now only a single mode control panel, 
//             CCxControlPanel.  Mode control dialogs are still derived from the abstract class CCxControlPanelDlg, but 
//             they interact with the "current" mode controller object (derived from CCxModeControl) rather than a 
//             derivative of CCxControlPanel.  See also CCxControlPanel, CCxControlPanelDlg, and CCxModeControl. 
// 10mar2005-- Added support for a readonly numeric edit IDC_FIX_REWARDSUM to display a second reward statistic, the 
//             cumulative sum of pulse lengths of the rewards delivered since the last reset. 
// 26may2020-- Added numeric edit (IDC_FIX_REWMULT) to specify the global reward pulse multiplier.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxdoc.h"                           // CCxDoc -- CNTRLX document class
#include "cxsettings.h"                      // CCxSettings -- CNTRLX "application settings" object
#include "cxmodecontrol.h"                   // CCxModeControl -- base class for CNTRLX mode controllers

#include "cxfixrewdlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CCxFixRewDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxFixRewDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_FIX_DUR, IDC_FIX_REWMULT, OnChange ) 
   ON_CONTROL_RANGE( BN_CLICKED, IDC_FIX_REWOVR, IDC_FIX_RESETREW, OnChange )
END_MESSAGE_MAP()


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnChange ======================================================================================================== 
//
//    Update a fixation/reward setting IAW a detected change in the corresponding control.  We handle two different 
//    notifications here:
//       1) BN_CLICKED ==> User clicked one of the check boxes IDC_FIX_REW_OVR, IDC_FIX_REWBEEP -- in which case we 
//          update the corresponding setting and inform CXDRIVER via the parent mode control panel.  If the user clicks 
//          the pushbutton IDC_FIX_RESETREW, we reset the reward statistics.
//       2) EN_KILLFOCUS ==> When any of the numeric edit controls on the form loses the focus, we update the 
//          corresponding fixation/reward setting.  Any illegal value is auto-corrected.  If the value has changed, we
//          inform CXDRIVER via the parent mode control panel.
//
//    !!!IMPORTANT!!!
//    During GUI creation at application startup, an edit control in this dialog may lose the focus -- generating an 
//    EN_KILLFOCUS notification.  However, GUI creation occurs BEFORE the CCxDoc exists -- in which case there's no 
//    application settings object available.  In this case, OnChange() does nothing.
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification. 
// 
//    RETURNS:    NONE
//
void CCxFixRewDlg::OnChange( UINT id )
{
   CCxSettings* pSet = GetSettings();                                // retrieve current CNTRLX application settings
   if( pSet == NULL ) return;                                        // trap EN_KILLFOCUS during GUI creation phase

   int iNew, iOld, iCorr;                                            // new, old, & (possibly) corrected values of a 
   float fNew, fOld, fCorr;                                          // particular setting...
   BOOL bNew, bOld, bCorr;
   BOOL bChanged = FALSE;                                            // has the setting been changed?

   switch( id )                                                      // update setting; if it was corrected, update the 
   {                                                                 // corres. control with corrected value
      case IDC_FIX_DUR :                                             //    fixation duration
         iOld = pSet->GetFixDuration();
         iNew = m_edFixDur.AsInteger();
         iCorr = pSet->SetFixDuration( iNew );
         if( iNew != iCorr ) m_edFixDur.SetWindowText( iCorr );
         bChanged = BOOL(iOld != iCorr);
         break;
      case IDC_FIX_HACC :                                            //    horizontal fixation accuracy
         fOld = pSet->GetFixAccH();
         fNew = m_edFixAccH.AsFloat();
         fCorr = pSet->SetFixAccH( fNew );
         if( fNew != fCorr ) m_edFixAccH.SetWindowText( fCorr );
         bChanged = BOOL(fOld != fCorr);
         break;
      case IDC_FIX_VACC :                                            //    vertical fixation accuracy
         fOld = pSet->GetFixAccV();
         fNew = m_edFixAccV.AsFloat();
         fCorr = pSet->SetFixAccV( fNew );
         if( fNew != fCorr ) m_edFixAccV.SetWindowText( fCorr );
         bChanged = BOOL(fOld != fCorr);
         break;
      case IDC_FIX_REW1 :                                            //    reward pulse length 1
         iOld = pSet->GetRewardLen1();
         iNew = m_edRewLen1.AsInteger();
         iCorr = pSet->SetRewardLen1( iNew );
         if( iNew != iCorr ) m_edRewLen1.SetWindowText( iCorr );
         bChanged = BOOL(iOld != iCorr);
         break;
      case IDC_FIX_REW2 :                                            //    reward pulse length 2
         iOld = pSet->GetRewardLen2();
         iNew = m_edRewLen2.AsInteger();
         iCorr = pSet->SetRewardLen2( iNew );
         if( iNew != iCorr ) m_edRewLen2.SetWindowText( iCorr );
         bChanged = BOOL(iOld != iCorr);
         break;
      case IDC_FIX_REWMULT:                                          //    rewward pulse length multiplier
         fOld = pSet->GetRewardPulseMultiplier();
         fNew = m_edRewMult.AsFloat();
         fCorr = pSet->SetRewardPulseMultiplier(fNew);
         if(fNew != fCorr) m_edRewMult.SetWindowText(fCorr);
         bChanged = BOOL(fOld != fCorr);
         break;
      case IDC_FIX_VRATIO :                                          //    variable ratio for random reward withholding 
         iOld = pSet->GetVariableRatio();
         iNew = m_edVarRatio.AsInteger();
         iCorr = pSet->SetVariableRatio( iNew );
         if( iNew != iCorr ) m_edVarRatio.SetWindowText( iCorr );
         bChanged = BOOL(iOld != iCorr);
         break;
      case IDC_FIX_AUDIOREW :                                        //    audio reward pulse length
         iOld = pSet->GetAudioRewardLen();
         iNew = m_edAudioRewLen.AsInteger();
         iCorr = pSet->SetAudioRewardLen( iNew );
         if( iNew != iCorr ) m_edAudioRewLen.SetWindowText( iCorr );
         bChanged = BOOL(iOld != iCorr);
         break;
      case IDC_FIX_REWOVR :                                          //    trial reward pulse length override enable
         bOld = pSet->IsTrialRewLenOverride();
         bNew = BOOL(m_btnTrialRewOverride.GetCheck() != 0);
         bCorr = pSet->SetTrialRewLenOverride( bNew );
         if( bNew != bCorr ) m_btnTrialRewOverride.SetCheck( bCorr ? 1 : 0 );
         bChanged = BOOL(bOld != bCorr);
         break;
      case IDC_FIX_REWBEEP :                                         //    reward beep indicator enable
         bOld = pSet->IsRewardBeepEnabled();
         bNew = BOOL(m_btnRewBeepEna.GetCheck() != 0);
         bCorr = pSet->SetRewardBeepEnabled( bNew );
         if( bNew != bCorr ) m_btnRewBeepEna.SetCheck( bCorr ? 1 : 0 );
         bChanged = BOOL(bOld != bCorr);
         break;
      case IDC_FIX_RESETREW :                                        //    reset reward statistics
         m_edNRewards.SetWindowText( 0 );                            //    (these are NOT application settings!)
         m_edTotalRew.SetWindowText( 0 );
         GetCurrentModeCtrl()->ResetRewardStats();
         break;
   }

   if( bChanged )                                                    // when a setting has been changed:
   {
      GetCurrentModeCtrl()->UpdateFixRewSettings();                  //    inform CXDRIVER via parent mode ctrl panel
      Notify();                                                      //    notify doc/view framework
   }
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
//    THROWS:     NONE.
//
BOOL CCxFixRewDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_edFixDur.SubclassDlgItem( IDC_FIX_DUR, (CWnd*) this );                // subclass & restrict format of all numeric 
   m_edFixDur.SetFormat( TRUE, TRUE, 5, 0 );                               // edit ctrls on dialog
   m_edFixAccH.SubclassDlgItem( IDC_FIX_HACC, (CWnd*) this ); 
   m_edFixAccH.SetFormat( FALSE, TRUE, 5, 2 ); 
   m_edFixAccV.SubclassDlgItem( IDC_FIX_VACC, (CWnd*) this ); 
   m_edFixAccV.SetFormat( FALSE, TRUE, 5, 2 ); 
   m_edRewLen1.SubclassDlgItem( IDC_FIX_REW1, (CWnd*) this ); 
   m_edRewLen1.SetFormat( TRUE, TRUE, 3, 0 ); 
   m_edRewLen2.SubclassDlgItem( IDC_FIX_REW2, (CWnd*) this ); 
   m_edRewLen2.SetFormat( TRUE, TRUE, 3, 0 ); 
   m_edRewMult.SubclassDlgItem(IDC_FIX_REWMULT, (CWnd*)this);
   m_edRewMult.SetFormat(FALSE, TRUE, 3, 1);
   m_edVarRatio.SubclassDlgItem( IDC_FIX_VRATIO, (CWnd*) this ); 
   m_edVarRatio.SetFormat( TRUE, TRUE, 2, 0 ); 
   m_edAudioRewLen.SubclassDlgItem( IDC_FIX_AUDIOREW, (CWnd*) this ); 
   m_edAudioRewLen.SetFormat( TRUE, TRUE, 4, 0 ); 

   m_edNRewards.SubclassDlgItem( IDC_FIX_NREWARDS, (CWnd*) this );         // reward stats are readonly numedits
   m_edNRewards.SetFormat( TRUE, TRUE, 4, 0 ); 
   m_edNRewards.SetReadOnly();
   m_edNRewards.SetWindowText( 0 );
   m_edTotalRew.SubclassDlgItem( IDC_FIX_REWARDSUM, (CWnd*) this );
   m_edTotalRew.SetFormat( TRUE, TRUE, 7, 0 );
   m_edTotalRew.SetReadOnly();
   m_edTotalRew.SetWindowText( 0 );

   m_btnTrialRewOverride.SubclassDlgItem( IDC_FIX_REWOVR, (CWnd*) this );  // subclass button controls on dialog
   m_btnRewBeepEna.SubclassDlgItem( IDC_FIX_REWBEEP, (CWnd*) this );
   m_btnResetRew.SubclassDlgItem( IDC_FIX_RESETREW, (CWnd*) this );

   m_bEnabled = m_edFixDur.IsWindowEnabled();                              // initial enable state of controls

   return( TRUE ); 
}


//=== Refresh [base override] ========================================================================================= 
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    If the CNTRLX runtime state currently forbids updating the fixation/reward settings, ALL controls are disabled. 
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxFixRewDlg::Refresh()
{
   BOOL bEna = GetCurrentModeCtrl()->CanUpdateFixRewSettings();      // ctrls enabled if runtime state permits it
   if( (bEna && !m_bEnabled) || (m_bEnabled && !bEna) ) 
   {
      m_bEnabled = bEna;
      m_edFixDur.EnableWindow( m_bEnabled );
      m_edFixAccH.EnableWindow( m_bEnabled );
      m_edFixAccV.EnableWindow( m_bEnabled );
      m_edRewLen1.EnableWindow( m_bEnabled );
      m_edRewLen2.EnableWindow( m_bEnabled );
      m_edRewMult.EnableWindow(m_bEnabled);
      m_edVarRatio.EnableWindow( m_bEnabled );
      m_edAudioRewLen.EnableWindow( m_bEnabled );

      m_btnTrialRewOverride.EnableWindow( m_bEnabled );
      m_btnRewBeepEna.EnableWindow( m_bEnabled );
      m_btnResetRew.EnableWindow( m_bEnabled );
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
//    Here we reload the dialog whenever a new document is created or opened, or if any fixation/reward settings have 
//    been modified outside this dialog. We also send the settings to CXDRIVER.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxFixRewDlg::OnUpdate( CCxViewHint* pHint )
{
   if( (pHint == NULL) || ((!InitiatedUpdate()) && pHint->m_code == CXVH_FIXREWSETTINGS) )
   {
      Load();
      GetCurrentModeCtrl()->UpdateFixRewSettings();
   }
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== GetSettings  ==================================================================================================== 
//
//    Retrieve the current CNTRLX "application settings" object, which includes fixation/reward settings as a subset.
// 
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
CCxSettings* CCxFixRewDlg::GetSettings()
{
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   return( (pDoc != NULL) ? pDoc->GetSettings() : NULL );
}


//=== Load ============================================================================================================ 
//
//    Reload the current fixation & reward settings into the appropriate controls in this dialog page.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxFixRewDlg::Load()
{
   CCxSettings* pSet = GetSettings();                                      // retrieve application settings object

   m_edFixDur.SetWindowText( pSet->GetFixDuration() );                     // load current settings into ctrls
   m_edFixAccH.SetWindowText( pSet->GetFixAccH() );
   m_edFixAccV.SetWindowText( pSet->GetFixAccV() );
   m_edRewLen1.SetWindowText( pSet->GetRewardLen1() );
   m_edRewLen2.SetWindowText( pSet->GetRewardLen2() );
   m_edRewMult.SetWindowText(pSet->GetRewardPulseMultiplier());
   m_edVarRatio.SetWindowText( pSet->GetVariableRatio() );
   m_edAudioRewLen.SetWindowText( pSet->GetAudioRewardLen() );

   m_btnTrialRewOverride.SetCheck( pSet->IsTrialRewLenOverride() ? 1 : 0 );
   m_btnRewBeepEna.SetCheck( pSet->IsRewardBeepEnabled() ? 1 : 0 );

   Refresh();                                                              // refresh enable state of controls 
}


//=== Notify ========================================================================================================== 
//
//    Notify the CNTRLX document and attached views (and other control panel dialogs) whenever any fixation/reward 
//    setting is changed in this dialog.
// 
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxFixRewDlg::Notify()
{
   CCxViewHint vuHint( CXVH_FIXREWSETTINGS, 0, 0 );
   SendUpdate( &vuHint );
}
