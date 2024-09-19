//=====================================================================================================================
//
// cxvideodspdlg.cpp : Declaration of CCxVideoDspDlg, a MAESTRO control panel dialog page for modifying the current XY
//                     scope and RMVideo framebuffer display configurations.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// MAESTRO realizes the CX_XYTARG and CX_RMVTARG target types on two kinds of video displays:  an XY oscilloscope
// driven by a DSP and "dotter" board, and a computer monitor driven by the OpenGL application RMVideo (which runs on a
// separate Linux workstation and communicates with MAESTRO over a private, dedicated Ethernet link).  Certain
// configurable parameters are associated with each of these displays.  The CCxSettings object encapsulates some of 
// these parameters, among other application-level settings.
//
// CCxVideoDspDlg serves as the user's "window" into the current state of the video display settings.  We implement it
// as a dialog page within the MAESTRO master mode control panel (CCxControlPanel) so that we can provide the user with
// convenient access to the display configuration in any operational mode, as need be. Each MAESTRO op mode is governed 
// by a mode controller object derived from CCxModeControl. Mode control dialogs like CCxVideoDspDlg get access to the 
// current mode controller via the base class method CCxControlPanelDlg::GetCurrentModeCtrl().
//
// The video display configuration is sent to MAESTRODRIVER (via the current mode controller) whenever ANY video
// setting is changed.  While this is somewhat inefficient, it ensures that the settings shown on the dialog are always
// in synch with MAESTRODRIVER's video display hardware. Also note that, in certain MAESTRO runtime states, changes to
// the video display configuration are not permissible (see CCxModeControl::CanUpdateVideoCfg()). RMVideo's display mode
// and gamma-correction factors may only be changed in IdleMode. When a display parameter is not modifiable in the 
// current operational state, the relevant controls on this dialog will be disabled.
//
// ==> Summary of controls housed on the dialog
//
//    IDC_DISP_XY_DIST ... IDC_DISP_XY_SEED [numeric edit]:  Current XY scope display geometry, draw cycle timing
//       parameters, and the fixed seed value for generating the random-dot patterns of XY scope targets.
//    IDC_DISP_XY_AUTO and IDC_DISP_XY_FIXED [radio btn]:  This mutually exclusive pair of radio buttons selects the
//       mode for choosing the seed used to generate random-dot patterns.  Whenever a trial or continuous-mode run is
//       executed, we query the CCxVideoDsp object for a new seed.  If the fixed (or manual) mode is chosen, the same
//       fixed seed value is provided on every request.  If the auto mode is chosen, a different seed is randomly
//       generated for each request.
//    IDC_DISP_FB_DIST ... IDC_DISP_FB_BLU [numeric edit]:  Current RMVideo display geometry and background color.
//    IDC_DISP_FB_GRAY [check box]:  Often, users are only interested in presenting grayscale backgrounds on the
//       RMVideo display.  If this box is checked, they only enter the luminance value in one edit control, for the
//       RED component.  The other two components are updated to take on the same value, and the corresponding edit
//       controls are updated to reflect the value in the RED control.
//    IDC_DISP_XYFIELD, IDC_DISP_FBFIELD [readonly edit]: These readouts indicate the current "field of view" in visual 
//    degrees on the XY scope and RMVideo display, based on the current geometry.
//    IDC_DISP_RMVMODE [combo]: Combo box selects the RMVideo display mode. Available modes are listed in the dropdown.
//    IDC_DISP_GAMMA_R .. IDC_DISP_GAMMA_B [numeric edit]: Current RMVideo monitor gamma-correction factors. Values are
//    range-restricted to [0.800 .. 3.000].
//    IDC_DISP_SYNCDUR .. IDC_DISP_SYNCSZ [numeric edit]: Parameters governing RMVideo "time sync flash" that may be
//    presented in the top-left corner of screen during the first video frame following the start of any trial segment.
//    Flash is intended to drive a photodiode circuit which can then deliver a flash event pulse back to Maestro to
//    help synchronize segment starts with actual Maestro timeline. Flash duration is in # of video frames, while 
//    flash spot (square) size is in mm. If spot size is 0, the feature is disabled. CCxSettings restricts the allowed
//    range for these parameters.
//    
//
// [**NOTE:  All read-write edit controls on the dialog, IDC_DISP_XY_DIST...IDC_DISP_SYNCSZ, must span a contiguous
// range of values so that we can use the macro ON_CONTROL_RANGE in the message map.  The same is true for the pair
// of radio btn controls.]
//
// ==> The MAESTRO "Mode Control" Framework.
// MAESTRO's master mode control panel CCxControlPanel is implemented as a dockable dialog bar containing one or more
// tabbed dialogs.  All dialogs that affect runtime state in any MAESTRO operational mode are installed in this
// container, although only a subset of them will be accessible in any given mode.  In addition to its role as a
// dialog container, CCxControlPanel constructs a "mode controller" object for each op mode, and it handles mode
// switches by invoking appropriate methods on the relevant mode controllers.  Each mode controller, interacting with
// the operator via some subset of the mode control panel dialogs, encapsulates the runtime behavior of MAESTRO and
// MAESTRODRIVER in a particular operational mode.  To communicate with MAESTRODRIVER, it must invoke methods on the
// MAESTRO runtime interface, CCxRuntime.  By design, the mode controller should insulate the mode control dialogs from
// CCxRuntime.  In other words, it provides all the methods needed by the dialogs to realize the desired functionality
// of the operational mode that controller and the associated dialogs represent.  Multiple dialogs allow us to break up
// that functionality into logically grouped, more manageable chunks.
//
// We define two ABSTRACT classes that serve as the foundations for this "mode control" framework.  CCxModeControl is
// the base class for all MAESTRO mode controller objects, while CCxControlPanelDlg is the base class for any dialog
// that is installed in CCxControlPanel.  CCxModeControl handles tasks that are common to all mode controllers and
// defines a set of abstract methods that any realizable mode controller must implement; CCxControlPanelDlg does the
// same for mode control dialog objects.
//
// See the implementation files for CCxControlPanel, CCxControlPanelDlg, and CCxModeControl for more details.
//
//
// CREDITS:
// (1) CCxVideoDspDlg is ultimately built upon the CSizingTabDlgBar/CSzDlgBarDlg framework which, in turn, is based on
// the MFC extension CSizingControlBarCF by Cristi Posea.  See szdlgbar.cpp for credits.
//
// REVISION HISTORY:
// 08feb2002-- Began development.
// 11feb2002-- Initial development complete.
// 17oct2002-- CCxVideoDsp object replaced by more general CCxSettings object, which contains the video display params
//             as a subset.  Modified CCxVideoDspDlg accordingly.
// 07apr2003-- MAJOR redesign of the CNTRLX "mode control" framework.  There is now only a single mode control panel,
//             CCxControlPanel.  Mode control dialogs are still derived from the abstract class CCxControlPanelDlg, but
//             they interact with the "current" mode controller object (derived from CCxModeControl) rather than a
//             derivative of CCxControlPanel.  See also CCxControlPanel, CCxControlPanelDlg, and CCxModeControl.
// 24mar2006-- Mods IAW introduction of RMVideo as replacement for VSG-based framebuffer video.
// 21apr2006-- Rearranged controls on dialog.  Added readouts to report current field of view for each display, in
//             deg subtened at eye, based on the current geometry.  Also added readout to report display resolution in
//             pixels, frame rate in Hz, and color resolution (16- or 32-bit) for RMVideo platform.
// 03sep2009-- Begun changes to support switching RMVideo display modes and setting gamma-correction factors. The 
//             display mode is exposed via a combo box, while the R/G/B gamma factors are exposed in numeric edit 
//             fields. Unlike the other controls, these will be enabled ONLY in IdleMode.
// 20sep2018-- Added controls IDC_DISP_SYNC*** for displaying/editing RMVideo time sync flash duration, size, and 
// margin. As of Maestro v4.0.0. These new document settings are sent to MaestroDRIVER with the other video display
// settings.
// 25sep2018-- Removed sync flash margin setting and corresponding dialog control IDC_DISP_SYNCMARG.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for MAESTRO

#include "cxdoc.h"                           // CCxDoc -- MAESTRO document class
#include "cxsettings.h"                      // CCxSettings -- MAESTRO application settings, including video dsp cfg
#include "cxmodecontrol.h"                   // CCxModeControl -- base class for MAESTRO mode controllers
#include "util.h"

#include "cxvideodspdlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CCxVideoDspDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxVideoDspDlg, CCxControlPanelDlg )
   ON_CONTROL( CBN_SELCHANGE, IDC_DISP_RMVMODE, OnRMVModeChange )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_DISP_XY_DIST, IDC_DISP_SYNCSZ, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_DISP_XY_AUTO, IDC_DISP_XY_FIXED, OnChange )
   ON_BN_CLICKED( IDC_DISP_FB_GRAY, OnGrayscale )
END_MESSAGE_MAP()


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

/**
 * Update the RMVideo display mode IAW the mode selected in the relevant combo box on this dialog. Switching display
 * modes takes up to 10 seconds because RMVideo must accurately re-measure the frame rate after the mode switch. Thus,
 * this operation is only permitted in IdleMode. While the command is processed, a wait cursor is displayed and the
 * application window is disabled.
 */
void CCxVideoDspDlg::OnRMVModeChange()
{
   CCxModeControl* pCtrl = GetCurrentModeCtrl();
   if(!pCtrl->CanUpdateRMV()) return;

   CWaitCursor wait;
   AfxGetMainWnd()->EnableWindow(FALSE);
   if(!pCtrl->SetCurrRMVideoMode(m_cbRMVMode.GetCurSel()))
      m_cbRMVMode.SetCurSel(pCtrl->GetCurrRMVideoMode());
   AfxGetMainWnd()->EnableWindow(TRUE);
}

//=== OnChange ========================================================================================================
//
//    Update a parameter in the video display configuration IAW a detected change in the corresponding control.  We
//    handle two different notifications here:
//       1) BN_CLICKED ==> User clicked one of the mutually exclusive radio-button pair IDC_DISP_XY_AUTO, _FIXED.  Our
//          only response is to update the pertinent flag in the display configuration.
//       2) EN_KILLFOCUS ==> When any of the edit controls on the form loses the focus, we update the corresponding
//          parameter in the display configuration. Any illegal value is auto-corrected.
//    If the selected video setting has actually changed as a result of the user's action, we inform the doc/view
//    framework and also send the new video display configuration to CXDRIVER.
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
void CCxVideoDspDlg::OnChange( UINT id )
{
   CCxSettings* pSet = GetSettings();                                // retrieve the application settings object
   if( pSet == NULL ) return;                                        // trap EN_KILLFOCUS during GUI creation phase

   int iNew, iOld, iCorr;                                            // new, old, & (possibly) corrected values of a
   BOOL bNew, bOld, bCorr;                                           // particular setting...

   BOOL bUpdateFovXY = (id==IDC_DISP_XY_DIST) || (id==IDC_DISP_XY_W) || (id==IDC_DISP_XY_H);
   BOOL bUpdateFovFB = (id==IDC_DISP_FB_DIST) || (id==IDC_DISP_FB_W) || (id==IDC_DISP_FB_H);

   // the RMVideo gamma-correction factors are handled differently, since they are NOT application settings
   if(id == IDC_DISP_GAMMA_R || id == IDC_DISP_GAMMA_G || id == IDC_DISP_GAMMA_B)
   {
      float r = m_edRMVGammaRed.AsFloat();
      float g = m_edRMVGammaGrn.AsFloat();
      float b = m_edRMVGammaBlu.AsFloat();
      if(!GetCurrentModeCtrl()->SetRMVGamma(r, g, b))
      {
         if(!GetCurrentModeCtrl()->GetRMVGamma(r, g, b))
            r = g = b = 1.0f;
      }
      
      m_edRMVGammaRed.SetWindowText(r);
      m_edRMVGammaGrn.SetWindowText(g);
      m_edRMVGammaBlu.SetWindowText(b);
      
      return;
   }
   
   switch( id )                                                      // update param; if it was corrected, update the
   {                                                                 // corres. control with corrected value
      case IDC_DISP_XY_DIST :
         iOld = pSet->GetXYDistToEye();
         iNew = m_edXYDistToEye.AsInteger();
         iCorr = pSet->SetXYDistToEye( iNew );
         if( iNew != iCorr ) m_edXYDistToEye.SetWindowText( iCorr );
         break;
      case IDC_DISP_XY_W :
         iOld = pSet->GetXYWidth();
         iNew = m_edXYWidth.AsInteger();
         iCorr = pSet->SetXYWidth( iNew );
         if( iNew != iCorr ) m_edXYWidth.SetWindowText( iCorr );
         break;
      case IDC_DISP_XY_H :
         iOld = pSet->GetXYHeight();
         iNew = m_edXYHeight.AsInteger();
         iCorr = pSet->SetXYHeight( iNew );
         if( iNew != iCorr ) m_edXYHeight.SetWindowText( iCorr );
         break;
      case IDC_DISP_XY_DELAY :
         iOld = pSet->GetXYDrawDelay();
         iNew = m_edXYDrawDelay.AsInteger();
         iCorr = pSet->SetXYDrawDelay( iNew );
         if( iNew != iCorr ) m_edXYDrawDelay.SetWindowText( iCorr );
         break;
      case IDC_DISP_XY_DUR :
         iOld = pSet->GetXYDrawDur();
         iNew = m_edXYDrawDur.AsInteger();
         iCorr = pSet->SetXYDrawDur( iNew );
         if( iNew != iCorr ) m_edXYDrawDur.SetWindowText( iCorr );
         break;
      case IDC_DISP_XY_SEED :
         iOld = int( pSet->GetFixedXYDotSeedValue() );
         iNew = m_edXYSeedVal.AsInteger();
         iCorr = int( pSet->SetFixedXYDotSeedValue( (DWORD) iNew ) );
         if( iNew != iCorr ) m_edXYSeedVal.SetWindowText( iCorr );
         break;

      case IDC_DISP_XY_AUTO :
      case IDC_DISP_XY_FIXED :
         bOld = pSet->IsXYDotSeedFixed();
         bNew = BOOL(m_btnIsFixed.GetCheck() != 0);
         bCorr = pSet->SetXYDotSeedFixed( bNew );
         if( bNew != bCorr )
         {
            m_btnIsFixed.SetCheck( bCorr ? 1 : 0 );
            m_btnIsAuto.SetCheck( bCorr ? 0 : 1 );
         }
         iOld = bOld ? 1 : 0;
         iCorr = bCorr ? 1 : 0;
         break;

      case IDC_DISP_FB_DIST :
         iOld = pSet->GetFBDistToEye();
         iNew = m_edFBDistToEye.AsInteger();
         iCorr = pSet->SetFBDistToEye( iNew );
         if( iNew != iCorr ) m_edFBDistToEye.SetWindowText( iCorr );
         break;
      case IDC_DISP_FB_W :
         iOld = pSet->GetFBWidth();
         iNew = m_edFBWidth.AsInteger();
         iCorr = pSet->SetFBWidth( iNew );
         if( iNew != iCorr ) m_edFBWidth.SetWindowText( iCorr );
         break;
      case IDC_DISP_FB_H :
         iOld = pSet->GetFBHeight();
         iNew = m_edFBHeight.AsInteger();
         iCorr = pSet->SetFBHeight( iNew );
         if( iNew != iCorr ) m_edFBHeight.SetWindowText( iCorr );
         break;
      case IDC_DISP_FB_RED :                                         // if grayscale in effect, we update the green &
         iOld = pSet->GetFBBkgRed();                                 // blue ctrls at the same time
         iNew = m_edFBBkgRed.AsInteger();
         if( m_btnIsGray.GetCheck() != 0 )
         {
            iCorr = pSet->SetFBBkgGrayscale( iNew );
            if( iCorr != iOld )
            {
               m_edFBBkgGrn.SetWindowText( iCorr );
               m_edFBBkgBlu.SetWindowText( iCorr );
            }
         }
         else
            iCorr = pSet->SetFBBkgRed( iNew );
         if( iNew != iCorr ) m_edFBBkgRed.SetWindowText( iCorr );
         break;
      case IDC_DISP_FB_GRN :
         iOld = pSet->GetFBBkgGrn();
         iNew = m_edFBBkgGrn.AsInteger();
         iCorr = pSet->SetFBBkgGrn( iNew );
         if( iNew != iCorr ) m_edFBBkgGrn.SetWindowText( iCorr );
         break;
      case IDC_DISP_FB_BLU :
         iOld = pSet->GetFBBkgBlu();
         iNew = m_edFBBkgBlu.AsInteger();
         iCorr = pSet->SetFBBkgBlu( iNew );
         if( iNew != iCorr ) m_edFBBkgBlu.SetWindowText( iCorr );
         break;
      case IDC_DISP_SYNCDUR:
         iOld = pSet->GetRMVSyncFlashDuration();
         iNew = m_edRMVSyncDur.AsInteger();
         iCorr = pSet->SetRMVSyncFlashDuration(iNew);
         if(iNew != iCorr) m_edRMVSyncDur.SetWindowText(iCorr);
         break;
      case IDC_DISP_SYNCSZ:
         iOld = pSet->GetRMVSyncFlashSize();
         iNew = m_edRMVSyncSize.AsInteger();
         iCorr = pSet->SetRMVSyncFlashSize(iNew);
         if(iNew != iCorr) m_edRMVSyncSize.SetWindowText(iCorr);
         break;
   }

   // if a video display setting has indeed changed, send the new video display cfg to MAESTRODRIVER; if necessary,
   // update one of the FOV readouts, and notify doc/view framework.
   if( iOld != iCorr )
   {
      GetCurrentModeCtrl()->UpdateVideoCfg();
      ReloadFieldOfView(bUpdateFovXY, bUpdateFovFB);
      Notify(); 
   }
}


//=== OnGrayscale =====================================================================================================
//
//    Handle BN_CLICKED message from IDC_DISP_FB_GRAY.  This check box toggles the use of grayscale mode to specify the
//    FB video background color in controls IDC_DISP_FB_RED..IDC_DISP_FB_BLU.  When grayscale mode is turned ON, the
//    current value for "red" luminance is copied to the "blue" and "green" components, and the "blue" and "green"
//    controls are disabled.  When grayscale mode is turned OFF, the components are left unchanged, but the "blue" and
//    "green" controls are re-enabled.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxVideoDspDlg::OnGrayscale()
{
   BOOL bIsGray = BOOL( m_btnIsGray.GetCheck() != 0 );                     // grayscale turned ON?

   if( bIsGray )                                                           // if grayscale turned ON and current bkg
   {                                                                       // color is NOT grayscale, we must alter grn
      CCxSettings* pSet = GetSettings();                                   // and/or blu cmpts, update corres ctrls,
      if( !pSet->IsFBBkgGray() )                                           // and inform MAESTRODRIVER and doc/views
      {                                                                    // of the change...
         int iVal = pSet->SetFBBkgGrayscale( m_edFBBkgRed.AsInteger() );
         m_edFBBkgRed.SetWindowText( iVal );
         m_edFBBkgGrn.SetWindowText( iVal );
         m_edFBBkgBlu.SetWindowText( iVal );

         GetCurrentModeCtrl()->UpdateVideoCfg();
         Notify();
      }
   }

   m_edFBBkgGrn.EnableWindow( !bIsGray );                                  // update enable state of blu & grn ctrls
   m_edFBBkgBlu.EnableWindow( !bIsGray );
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
BOOL CCxVideoDspDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_edXYDistToEye.SubclassDlgItem( IDC_DISP_XY_DIST, (CWnd*) this );      // subclass & restrict format of all numeric
   m_edXYDistToEye.SetFormat( TRUE, TRUE, 4, 0 );                          // edit ctrls on dialog
   m_edXYWidth.SubclassDlgItem( IDC_DISP_XY_W, (CWnd*) this );
   m_edXYWidth.SetFormat( TRUE, TRUE, 4, 0 );
   m_edXYHeight.SubclassDlgItem( IDC_DISP_XY_H, (CWnd*) this );
   m_edXYHeight.SetFormat( TRUE, TRUE, 4, 0 );
   m_edXYDrawDelay.SubclassDlgItem( IDC_DISP_XY_DELAY, (CWnd*) this );
   m_edXYDrawDelay.SetFormat( TRUE, TRUE, 2, 0 );
   m_edXYDrawDur.SubclassDlgItem( IDC_DISP_XY_DUR, (CWnd*) this );
   m_edXYDrawDur.SetFormat( TRUE, TRUE, 3, 0 );
   m_edXYSeedVal.SubclassDlgItem( IDC_DISP_XY_SEED, (CWnd*) this );
   m_edXYSeedVal.SetFormat( TRUE, TRUE, 8, 0 );

   m_edFBDistToEye.SubclassDlgItem( IDC_DISP_FB_DIST, (CWnd*) this );
   m_edFBDistToEye.SetFormat( TRUE, TRUE, 4, 0 );
   m_edFBWidth.SubclassDlgItem( IDC_DISP_FB_W, (CWnd*) this );
   m_edFBWidth.SetFormat( TRUE, TRUE, 4, 0 );
   m_edFBHeight.SubclassDlgItem( IDC_DISP_FB_H, (CWnd*) this );
   m_edFBHeight.SetFormat( TRUE, TRUE, 4, 0 );
   m_edFBBkgRed.SubclassDlgItem( IDC_DISP_FB_RED, (CWnd*) this );
   m_edFBBkgRed.SetFormat( TRUE, TRUE, 3, 0 );
   m_edFBBkgGrn.SubclassDlgItem( IDC_DISP_FB_GRN, (CWnd*) this );
   m_edFBBkgGrn.SetFormat( TRUE, TRUE, 3, 0 );
   m_edFBBkgBlu.SubclassDlgItem( IDC_DISP_FB_BLU, (CWnd*) this );
   m_edFBBkgBlu.SetFormat( TRUE, TRUE, 3, 0 );

   m_edRMVGammaRed.SubclassDlgItem(IDC_DISP_GAMMA_R, (CWnd*) this );
   m_edRMVGammaRed.SetFormat(FALSE, TRUE, 5, 3);
   m_edRMVGammaGrn.SubclassDlgItem(IDC_DISP_GAMMA_G, (CWnd*) this );
   m_edRMVGammaGrn.SetFormat(FALSE, TRUE, 5, 3);
   m_edRMVGammaBlu.SubclassDlgItem(IDC_DISP_GAMMA_B, (CWnd*) this );
   m_edRMVGammaBlu.SetFormat(FALSE, TRUE, 5, 3);

   m_edRMVSyncDur.SubclassDlgItem(IDC_DISP_SYNCDUR, (CWnd*)this);
   m_edRMVSyncDur.SetFormat(TRUE, TRUE, 1, 0);
   m_edRMVSyncSize.SubclassDlgItem(IDC_DISP_SYNCSZ, (CWnd*)this);
   m_edRMVSyncSize.SetFormat(TRUE, TRUE, 2, 0);

   m_cbRMVMode.SubclassDlgItem(IDC_DISP_RMVMODE, (CWnd*) this );

   m_btnIsFixed.SubclassDlgItem( IDC_DISP_XY_FIXED, (CWnd*) this );        // subclass button controls on dialog
   m_btnIsAuto.SubclassDlgItem( IDC_DISP_XY_AUTO, (CWnd*) this );
   m_btnIsGray.SubclassDlgItem( IDC_DISP_FB_GRAY, (CWnd*) this );

   m_btnIsGray.SetCheck( 0 );                                              // grayscale button unchecked initially

   m_bIsXYEnabled = m_edXYDistToEye.IsWindowEnabled();                     // initial enable state of controls
   m_bIsFBEnabled = m_edFBDistToEye.IsWindowEnabled();

   return( TRUE );
}


//=== Refresh [base override] =========================================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the MAESTRO runtime state changes.
//
//    Here we update the ena/disabled state of the dialog's controls as needed:
//       1) If the XY scope hardware is not available, the corresponding set of controls are disabled.  Analogously for
//          the RMVideo display.  If neither video display is available, ALL controls on form are disabled!
//       2) If the MAESTRO runtime state currently forbids updating the video display configuration, ALL controls on
//          form are disabled. The RMVideo display mode and monitor gamma may be changed only in IdleMode.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxVideoDspDlg::Refresh()
{
   CCxModeControl* pCtrl = GetCurrentModeCtrl();                           // the current mode controller

   BOOL bXYEnabled = FALSE;                                                // determine the new enable state for ctrls
   BOOL bFBEnabled = FALSE;
   if( pCtrl->CanUpdateVideoCfg() )                                        // if video display updates are possible:
   {
      bXYEnabled = pCtrl->IsXYAvailable();                                 //    enable XY ctrls if hardware is there
      bFBEnabled = pCtrl->IsRMVideoAvailable();                            //    enable RMVideo ctrls if it is there
   }

   if((bXYEnabled && !m_bIsXYEnabled) || (m_bIsXYEnabled && !bXYEnabled))  // update XY ctrls' enable state as needed
   {
      m_bIsXYEnabled = bXYEnabled;
      m_edXYDistToEye.EnableWindow( m_bIsXYEnabled );
      m_edXYWidth.EnableWindow( m_bIsXYEnabled );
      m_edXYHeight.EnableWindow( m_bIsXYEnabled );
      m_edXYDrawDelay.EnableWindow( m_bIsXYEnabled );
      m_edXYDrawDur.EnableWindow( m_bIsXYEnabled );
      m_edXYSeedVal.EnableWindow( m_bIsXYEnabled );
      m_btnIsFixed.EnableWindow( m_bIsXYEnabled );
      m_btnIsAuto.EnableWindow( m_bIsXYEnabled );
   }

   if((bFBEnabled && !m_bIsFBEnabled) || (m_bIsFBEnabled && !bFBEnabled))  // update FB ctrls' enable state as needed
   {
      m_bIsFBEnabled = bFBEnabled;
      m_edFBDistToEye.EnableWindow( m_bIsFBEnabled );
      m_edFBWidth.EnableWindow( m_bIsFBEnabled );
      m_edFBHeight.EnableWindow( m_bIsFBEnabled );

      m_edFBBkgRed.EnableWindow( m_bIsFBEnabled );
      BOOL bIsFullColor = BOOL(m_btnIsGray.GetCheck() == 0);               // if grayscale, B & G ctrls are disabled
      m_edFBBkgGrn.EnableWindow( m_bIsFBEnabled && bIsFullColor );
      m_edFBBkgBlu.EnableWindow( m_bIsFBEnabled && bIsFullColor );
      m_btnIsGray.EnableWindow( m_bIsFBEnabled );

      m_edRMVSyncDur.EnableWindow(m_bIsFBEnabled);
      m_edRMVSyncSize.EnableWindow(m_bIsFBEnabled);
   }
   
   // RMVideo display mode and monitor gamma are NOT part of the original video configuration (they are not application
   // settings). They can be manipulated only in IdleMode.
   bFBEnabled = pCtrl->CanUpdateRMV();
   m_edRMVGammaRed.EnableWindow(bFBEnabled);
   m_edRMVGammaGrn.EnableWindow(bFBEnabled);
   m_edRMVGammaBlu.EnableWindow(bFBEnabled);
   m_cbRMVMode.EnableWindow(bFBEnabled && (pCtrl->GetNumRMVideoModes() > 1));
}


//=== OnUpdate [base override] ========================================================================================
//
//    CCxControlPanelDlg::OnUpdate() is a MAESTRO-specific extension of MFC's mechanism -- aka, CView::OnUpdate() --
//    for informing all document views when one of those views causes a change in the active document's contents.  It
//    passes on the MAESTRO-specific doc/view hint (CCxViewHint) to the MAESTRO control panel dialogs, which may
//    contain document data.  When the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI
//    apps, this call is made each time a new document is created/opened -- giving us an opportunity to perform any
//    "per-document" initializations.
//
//    Here we reload the dialog whenever a new document is created or opened, or if any video display settings have
//    been modified outside this dialog. We also send the settings to CXDRIVER.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxVideoDspDlg::OnUpdate( CCxViewHint* pHint )
{
   if( (pHint == NULL) || ((!InitiatedUpdate()) && pHint->m_code == CXVH_VIDEOSETTINGS) )
   {
      Load();
      GetCurrentModeCtrl()->UpdateVideoCfg();
   }
}



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== GetSettings  ====================================================================================================
//
//    Retrieve the current MAESTRO "application settings" object, which includes video display settings as a subset.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
CCxSettings* CCxVideoDspDlg::GetSettings()
{
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   return( (pDoc != NULL) ? pDoc->GetSettings() : NULL );
}


//=== Load ============================================================================================================
//
//    Reload the current MAESTRO video display settings into the controls on this form, and refresh the enable state of
//    all controls.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxVideoDspDlg::Load()
{
   CCxSettings* pSet = GetSettings();                                      // retrieve current application settings

   m_edXYDistToEye.SetWindowText( pSet->GetXYDistToEye() );                // load current video settings into ctrls
   m_edXYWidth.SetWindowText( pSet->GetXYWidth() );
   m_edXYHeight.SetWindowText( pSet->GetXYHeight() );
   m_edXYDrawDelay.SetWindowText( pSet->GetXYDrawDelay() );
   m_edXYDrawDur.SetWindowText( pSet->GetXYDrawDur() );
   m_edXYSeedVal.SetWindowText( (int) pSet->GetFixedXYDotSeedValue() );
   BOOL bIsFixed = pSet->IsXYDotSeedFixed();
   m_btnIsFixed.SetCheck( bIsFixed ? 1 : 0 );
   m_btnIsAuto.SetCheck( bIsFixed ? 0 : 1 );

   m_edFBDistToEye.SetWindowText( pSet->GetFBDistToEye() );
   m_edFBWidth.SetWindowText( pSet->GetFBWidth() );
   m_edFBHeight.SetWindowText( pSet->GetFBHeight() );
   m_edFBBkgRed.SetWindowText( pSet->GetFBBkgRed() );
   m_edFBBkgGrn.SetWindowText( pSet->GetFBBkgGrn() );
   m_edFBBkgBlu.SetWindowText( pSet->GetFBBkgBlu() );

   BOOL isGrayBtnChecked = BOOL(m_btnIsGray.GetCheck() != 0);              // if new RMVideo bkg color not grayscale,
   BOOL isBkgGray = pSet->IsFBBkgGray();                                   // we must uncheck the grayscale flag --
   if( isGrayBtnChecked != isBkgGray )                                     // and vice versa.  Also update enable
   {                                                                       // state of the blue/green ctrls if nec
      m_btnIsGray.SetCheck( isBkgGray ? 1 : 0 );
      BOOL bEnaGrnBlu = m_bIsFBEnabled && !isBkgGray;
      m_edFBBkgGrn.EnableWindow( bEnaGrnBlu );
      m_edFBBkgBlu.EnableWindow( bEnaGrnBlu );
   }

   // RMVideo time sync flash parameters; added in Maestro 4.0.0
   m_edRMVSyncDur.SetWindowText(pSet->GetRMVSyncFlashDuration());
   m_edRMVSyncSize.SetWindowText(pSet->GetRMVSyncFlashSize());

   ReloadFieldOfView( TRUE, TRUE );                                        // load field of view readouts

   // the RMVideo display mode and gamma-correction factors are NOT application settings
   CCxModeControl* pModeCtrl = GetCurrentModeCtrl();
   float r = 1.0f;
   float g = 1.0f;
   float b = 1.0f;
   if(pModeCtrl->IsRMVideoAvailable())
   {
      pModeCtrl->GetRMVGamma(r, g, b);
   }
   m_edRMVGammaRed.SetWindowText(r);
   m_edRMVGammaGrn.SetWindowText(g);
   m_edRMVGammaBlu.SetWindowText(b);
   
   int nModes = pModeCtrl->GetNumRMVideoModes();
   m_cbRMVMode.ModifyStyle(CBS_SORT, 0); 
   m_cbRMVMode.ResetContent();
   for(int i=0; i<nModes; i++)
   {
      CString desc = "";
      pModeCtrl->GetRMVideoModeDesc(i, desc);
      m_cbRMVMode.AddString((LPCTSTR) desc);
   }
   if(nModes > 0)
      m_cbRMVMode.SetCurSel(pModeCtrl->GetCurrRMVideoMode());
   
   Refresh();                                                              // refresh enable state of controls
}

//=== ReloadFieldOfView ===============================================================================================
//
//    Whenever the display geometry for the XY scope or RMVideo display changes, the effective field of view (in deg
//    subtended at the subject's eye, which are the units the user works with on the Maestro GUI) covered by the
//    display will change.  This method calculates the current field of view for each display platform based on the
//    current geometry and stuffs strings of the form "0.00 x 0.00 deg" in the readout controls IDC_DISP_XYFIELD and
//    IDC_DISP_FBFIELD.
//
//    ARGS:       bXY -- [in] if set, the method updates the "field of view" readout for the XY scope.
//                bFB -- [in] if set, the method updates the "field of view" readout for the RMVideo display.
//
//    RETURNS:    NONE.
//
VOID CCxVideoDspDlg::ReloadFieldOfView( BOOL bXY, BOOL bFB )
{
   CCxSettings* pSet = GetSettings();                                      // retrieve current application settings
   CString str;
   if( bXY )                                                               // update XY scope FOV readout
   {
      double d = pSet->GetXYDistToEye();
      double w = pSet->GetXYWidth();
      double h = pSet->GetXYHeight();
      str.Format("%.2f x %.2f deg", cMath::toDegrees(atan2(w/2.0, d)) * 2.0, cMath::toDegrees(atan2(h/2.0, d)) * 2.0);
      SetDlgItemText( IDC_DISP_XYFIELD, str );
   }

   if( bFB )                                                               // update RMVideo FOV readout
   {
      double d = pSet->GetFBDistToEye();
      double w = pSet->GetFBWidth();
      double h = pSet->GetFBHeight();
      str.Format("%.2f x %.2f deg", cMath::toDegrees(atan2(w/2.0, d)) * 2.0, cMath::toDegrees(atan2(h/2.0, d)) * 2.0);
      SetDlgItemText( IDC_DISP_FBFIELD, str );
   }
}

//=== Notify ==========================================================================================================
//
//    Notify the MAESTRO document and attached views (and other control panel dialogs) whenever video display settings
//    are changed in this dialog.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxVideoDspDlg::Notify()
{
   CCxViewHint vuHint( CXVH_VIDEOSETTINGS, 0, 0 );
   SendUpdate( &vuHint );
}
