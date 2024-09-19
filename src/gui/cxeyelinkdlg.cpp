//===================================================================================================================== 
//
// cxeyelinkdlg.cpp : Declaration of CCxEyelinkDlg, a CNTRLX control panel dialog page with controls pertaining to the
// Eyelink 1000+ eye tracker.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// The Eyelink 1000+ eye tracker is a high-speed camera-based system for recording eye position during an experiment.
// It is an alternative to the eye coil system normally employed in Maestro. That system is not appropriate in human
// psychophysics experiments, and improvements in the camera-based system make it a viable alternative to the eye
// coil implants in certain scenarios.
//
// The Eyelink is connected to Maestro via a dedicated Ethernet link, but the manufacturer does not offer support for
// direct access within the specialized RTX environment. Instead, we've developed a rudimentary interface that runs
// on the Win32 side of Maestro, using a worker thread that runs in time-critical mode whenever eye position data is
// being recorded and streamed from the Eyelink to Maestro. The Eyelink interface is encapsulated by Maestro's runtime
// controller, CCxRuntime.
//
// CCxEyelinkDlg is a simple control panel dialog that lets the user connect/disconnect from the Eyelink and set its
// calibration parameters. Sliders are used to adjust the calibration paramters, which may be changed at any time (even
// when not connected to the Eyelink). The user must be able to change these parameters while the subject is fixating
// on known target locations in order to adjust the offset and scale for the eye X and Y coordinates.
//
// NOTE: Eyelink raw pupil sample (px, py) is converted to eye position (ex, ey) in visual deg subtended at the eye by:
// ex = (px - offsetX) / gainX and ey = (py - offsetY) / gainY. The gain factor is a divisor and is limited to integer
// values in +/-[minG..maxG]. The gain factor sliders only reflect the absolute value of the gain and are configured to 
// span [0..(maxG-minG)], and the gain is set to maxG-sliderVal. This ensures that, as you move the slider thumb to the 
// left, the absolute value of the divisive gain decreases -- which means the absolute value of the Maestro eye 
// coordinate increases as the user would expect.

// ==> Summary of controls housed on the dialog
//
//    IDC_EL_CONNECT [pushbutton]: The Connect/Disconnect PB. Always enabled except upon initiating a connection
//    attempt; in this case, it is disabled and reads "...Connecting...".
//    IDC_EL_XPOL, IDC_EL_YPOL [pushbutton]: These toggle the polarity of the X gain and Y gain, respectively.
//    IDC_EL_XOFS_SLIDE ... IDC_EL_YGAIN_SLIDE [slider] : Slider controls that adjust X offset and gain, Y offset and
//    gain for the Eyelink's eye position coordinates. Always enabled.
//    IDC_EL_XOFS_RO ... IDC_EL_YGAIN_RO [numeric edit] : These read-only numeric edit controls display the current
//    value for each calibration parameter. All parameter values are integers in arbitrary units.
//    IDC_EL_VELFW [numeric edit] : This numeric edit control sets the width of the smoothing filter that smooths the
//    velocity signals calculated by differentiating the corresponding Eyelink position signals. Units = ms. Range-
//    limited to [EL_MINSMOOTHW .. EL_MAXSMOOTHW].
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
// (1) CCxEyelinkDlg is ultimately built upon the CSizingTabDlgBar/CSzDlgBarDlg framework which, in turn, is based on 
// the MFC extension CSizingControlBarCF by Cristi Posea.  See szdlgbar.cpp for credits.
//
// REVISION HISTORY:
// 31aug2015-- Began development.
// 25jan2016-- Revised to introduce pushbuttons to toggle the polarity of the X and Y calibration gains. The slider
// controls only reflect the absolute value of the gain, while the readouts display the signed value.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX
#include "cxruntime.h"                       // Maestro runtime object CCxRuntime -- encapsulates Eyelink interface
#include "cxmodecontrol.h"                   // CCxModeControl -- base class for CNTRLX mode controllers

#include "cxeyelinkdlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CCxEyelinkDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxEyelinkDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE(BN_CLICKED, IDC_EL_XPOL, IDC_EL_CONNECT, OnBtnClick)
   ON_CONTROL(EN_KILLFOCUS, IDC_EL_VELFW, OnSetFilterW)
   ON_WM_HSCROLL()
END_MESSAGE_MAP()


/**
 Handler called whenever one of three pushbuttons are clicked on the dialog panel.
 
 -- When the Connect/Disconnect button is pressed: If Eyelink is currently connected to Maestro, then this initiates a 
 disconnect through the Maestro runtime object, CCxRuntime. If it is not connected, an attempt is made to connect to 
 the Eyelink. In the latter case, the connection is made on a background thread, so the button is disabled and its 
 label set to "Connecting..." -- a transient state that will be resolved once the connection is completed (or fails).
 -- When the X or Y polarity ("+/-") button is pressed, the X or Y calibration gain is negated and the relevant readout
 control is updated. Note that relevant slider is unaffected, since it only displays the absolute value of the gain.
 */
void CCxEyelinkDlg::OnBtnClick(UINT id)
{
   CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();

   if(id == IDC_EL_CONNECT)
   {
      if(pRuntime->IsEyelinkConnected())
      {
         pRuntime->DisconnectEyelink();
         m_btnConnect.SetWindowText(_T("Connect"));
         m_btnConnect.EnableWindow(TRUE);
      }
      else if(pRuntime->ConnectEyelink())
      {
         m_btnConnect.SetWindowText(_T("...Connecting..."));
         m_btnConnect.EnableWindow(FALSE);
      }
   }
   else if(id == IDC_EL_XPOL || id == IDC_EL_YPOL)
   {
      BOOL isX = BOOL(id == IDC_EL_XPOL);
      int value = -(pRuntime->GetEyelinkCal(isX, FALSE));
      pRuntime->SetEyelinkCal(isX, FALSE, value);

      if(isX) m_edXGain.SetWindowText(value);
      else m_edYGain.SetWindowText(value);
   }
   
}

/**
 Handler called whenever the numeric edit control specifying the velocity smoothing filter width loses the keyboard 
 focus, or the user hits "Enter" key while focus in in that control. [Note that the latter situation is detected via 
 the default dialog box "OK" mechanism via OnOK() method, which does nothing in super class CSzDlgBarDlg -- since
 the dialog bar framework handles that differently.]
 */
void CCxEyelinkDlg::OnSetFilterW()
{
   CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();
   BOOL bOk = pRuntime->SetEyelinkVelFilterWidth(m_edVelFW.AsInteger());
   if(!bOk) m_edVelFW.SetWindowText(pRuntime->GetEyelinkVelFilterWidth());
}

/**
 Handler called whenever any of the four slider controls are manipulated by the user, either with the keyboard or the
 mouse. 
 @param nSBCode The slider control code: TB_THUMBPOSITION, etc.
 @param nPos For TB_THUMBPOSITION or TB_THUMBTRACK, this is the thumb position. If negative positions allowed, be sure
 to cast to an int.
 @param pSB Should be a pointer to the relevant slider control. Cast to CSliderCtrl*.
 */
void CCxEyelinkDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pSB)
{
   CSliderCtrl* pSlider = reinterpret_cast<CSliderCtrl*>(pSB);

   BOOL isX = (pSlider == &m_slideXOfs) || (pSlider == &m_slideXGain);
   BOOL isOfs = (pSlider == &m_slideXOfs) || (pSlider == &m_slideYOfs);
   
   if(nSBCode==TB_THUMBTRACK || nSBCode==TB_ENDTRACK)
   {
      int value = (nSBCode==TB_THUMBTRACK) ? ((int) nPos) : pSlider->GetPos();

      // for gain/scale parameter, the actual gain = max_gain - slider value. Thus, the divisive gain decreases 
      // (and the Maestro eye coordinate value increases) as the slider thumb moves to the left. In addition,
      // the gain may be positive or negative -- we have to check the current gain value to determine polarity;
      // the sliders only control the absolute value.
      CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();
      if(!isOfs)
      {
         value = EL_MAXGAIN - value;
         if(pRuntime->GetEyelinkCal(isX, isOfs) < 0) value = -value;
      }
      pRuntime->SetEyelinkCal(isX, isOfs, value);

      // update the corresponding numeric edit readout to reflect the new value
      if(isX)
      {
         if(isOfs) m_edXOfs.SetWindowText(value);
         else m_edXGain.SetWindowText(value);
      }
      else
      {
         if(isOfs) m_edYOfs.SetWindowText(value);
         else m_edYGain.SetWindowText(value);
      }
   }
}


/**
 Prepare the Eyelink dialog for display. Here we subclass dialog resource template-defined controls to class members,
 format the numeric edit controls, and initialize sliders and read-only edit control to reflect the current values for
 the Eyelink calibration parameters.
 @return TRUE to place initial input focus on the first ctrl in dialog's tab order; FALSE if we've already set the 
 input focus on another ctrl.
 */
BOOL CCxEyelinkDlg::OnInitDialog()
{
   // let base class do its thing...
   CCxControlPanelDlg::OnInitDialog(); 

   // the Maestro runtime object encapsulates the Eyelink tracker
   CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();
   BOOL isConn = pRuntime->IsEyelinkConnected();
   int xOfs = pRuntime->GetEyelinkCal(TRUE, TRUE);
   int xGain = pRuntime->GetEyelinkCal(TRUE, FALSE);
   int yOfs = pRuntime->GetEyelinkCal(FALSE, TRUE);
   int yGain = pRuntime->GetEyelinkCal(FALSE, FALSE);
   int velFW = pRuntime->GetEyelinkVelFilterWidth();

   // subclass connect/disconnect button and set its initial label: Connect/Connecting.../Disconnect
   m_btnConnect.SubclassDlgItem(IDC_EL_CONNECT, (CWnd*) this);
   m_btnConnect.SetWindowText(isConn ? _T("Disconnect") : _T("Connect"));

   // subclass and restrict format of the numeric edit controls, all of which are read-only
   m_edXOfs.SubclassDlgItem(IDC_EL_XOFS_RO, (CWnd*) this); 
   m_edXOfs.SetFormat(TRUE, FALSE, 5, 0); 
   m_edXOfs.SetReadOnly();
   m_edXOfs.SetWindowText(xOfs);

   m_edXGain.SubclassDlgItem(IDC_EL_XGAIN_RO, (CWnd*) this); 
   m_edXGain.SetFormat(TRUE, FALSE, 5, 0); 
   m_edXGain.SetReadOnly();
   m_edXGain.SetWindowText(xGain);

   m_edYOfs.SubclassDlgItem(IDC_EL_YOFS_RO, (CWnd*) this); 
   m_edYOfs.SetFormat(TRUE, FALSE, 5, 0); 
   m_edYOfs.SetReadOnly();
   m_edYOfs.SetWindowText(yOfs);

   m_edYGain.SubclassDlgItem(IDC_EL_YGAIN_RO, (CWnd*) this); 
   m_edYGain.SetFormat(TRUE, FALSE, 5, 0); 
   m_edYGain.SetReadOnly();
   m_edYGain.SetWindowText(yGain);

   // subclass and initialize corresponding slider controls
   m_slideXOfs.SubclassDlgItem(IDC_EL_XOFS_SLIDE, (CWnd*) this);
   m_slideXOfs.SetRange(EL_MINOFS, EL_MAXOFS, TRUE);
   m_slideXOfs.SetLineSize(40);
   m_slideXOfs.SetPageSize(400);
   m_slideXOfs.SetPos(xOfs);

   m_slideXGain.SubclassDlgItem(IDC_EL_XGAIN_SLIDE, (CWnd*) this);
   m_slideXGain.SetRange(0, EL_MAXGAIN-EL_MINGAIN, TRUE);
   m_slideXGain.SetLineSize(10);
   m_slideXGain.SetPageSize(50);
   m_slideXGain.SetPos(EL_MAXGAIN-cMath::abs(xGain));

   m_slideYOfs.SubclassDlgItem(IDC_EL_YOFS_SLIDE, (CWnd*) this);
   m_slideYOfs.SetRange(EL_MINOFS, EL_MAXOFS, TRUE);
   m_slideYOfs.SetLineSize(40);
   m_slideYOfs.SetPageSize(400);
   m_slideYOfs.SetPos(yOfs);

   m_slideYGain.SubclassDlgItem(IDC_EL_YGAIN_SLIDE, (CWnd*) this);
   m_slideYGain.SetRange(0, EL_MAXGAIN-EL_MINGAIN, TRUE);
   m_slideYGain.SetLineSize(10);
   m_slideYGain.SetPageSize(50);
   m_slideYGain.SetPos(EL_MAXGAIN-cMath::abs(yGain));

   // subclass and initialize the numeric edit control that sets velocity smoothing filter width
   m_edVelFW.SubclassDlgItem(IDC_EL_VELFW, (CWnd*) this); 
   m_edVelFW.SetFormat(TRUE, TRUE, 2, 0); 
   m_edVelFW.SetWindowText(velFW);

   return(TRUE); 
}

/**
 Call this method whenever the connection status of the Eyelink changes. The Connect/Disconnect pushbutton is updated 
 accordingly. Also, that PB is disabled whenever recording is in progress in either Trial or Cont mode.
*/
VOID CCxEyelinkDlg::Refresh()
{
   CCxRuntime* pRuntime = ((CCntrlxApp*) AfxGetApp())->GetRuntime();
   BOOL isConn = pRuntime->IsEyelinkConnected();
   BOOL isRec = pRuntime->IsEyelinkRecording();

   BOOL enaConn = TRUE;
   if(pRuntime->GetMode() == CCxRuntime::TrialMode) 
      enaConn = !pRuntime->IsTrialRunning();
   else if(pRuntime->GetMode() == CCxRuntime::ContMode)
      enaConn = BOOL(0 == (pRuntime->GetProtocolStatus() & CX_FC_RECORDING));

   m_btnConnect.SetWindowText(isConn ? _T("Disconnect") : _T("Connect"));
   m_btnConnect.EnableWindow(enaConn);
   m_edVelFW.EnableWindow(!isRec);
}

