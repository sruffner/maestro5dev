//===================================================================================================================== 
//
// cxtestmode.cpp :  Declaration of the TestMode controller CCxTestMode and TestMode-specific control panel dialogs,
//                   CCxAnalogIODlg and CCxEventTimerDlg.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Each "operational mode" of CNTRLX has a "mode controller" which controls the runtime behavior of CNTRLX and CXDRIVER 
// in that mode.  CCxTestMode is the mode controller for CNTRLX's "Test & Calibration Mode", or "TestMode" for short. 
// The sole purpose of this op mode is to provide some limited support for performing calibration procedures and 
// diagnostic tests on selected CNTRLX hardware devices outside the normal framework of CNTRLX experimental protocols. 
// In this file we define CCxTestMode as well as two TestMode-specific mode contol dialogs, CCxAnalogIODlg and 
// CCxEventTimerDlg.
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
// ==> CCxTestMode.
// CCxTestMode is the mode controller for TestMode operations.  It installs the TestMode-specific dialogs in the master 
// mode control panel during GUI creation at application startup (see InitDlgs()), hides all TestMode control dialogs 
// upon exiting TestMode (see Exit()), and reveals these same dialogs upon entering TestMode (see Enter()).  The 
// dialogs accessible in the mode control panel during TestMode (see below) manage various GUI widgets/controls, and 
// these dialogs call CCxTestMode methods to carry out various operations, including all interactions with CXDRIVER via 
// the CNTRLX runtime interface, CCxRuntime.
//
// In addition, CCxTestMode "disables" any dialog associated with a hardware component that is not available.  This is 
// important because CNTRLX is designed to run even if some supported hardware devices are not present in the system. 
// Since it is possible that a hardware component could become available or unavailable at any random time after the 
// TestMode dlgs have been created, we check for changes in hardware status and update each dialog's ena/disabled 
// state as needed -- see CCxTestMode::Service(). 
//
// The dialogs housed in CCxTestPanel are described below.  I plan to implement additional dialogs for several other 
// CNTRLX hardware components in the future....
//
// ==> CCxAnalogIODlg.
// The Analog I/O Test Panel dialog, CCxAnalogIODlg, is defined in the dlg template resource IDD_AIOCP.  It provides 
// access to the following test & diagnostic capabilities for CNTRLX's analog input & output devices:
//
//    1) Display of analog input (AI) and analog output (AO) voltage readings.  These are displayed on an MFC grid 
// control (see CREDITS), IDC_AIO_CHANS.  A toggle PB [IDC_AIO_PAUSE] allows user to pause or resume a DAQ op on the AI 
// board; when the DAQ is in progress, the AI readings in the grid control are periodically updated.  AO voltages can 
// be changed at any time, either by double-clicking on a selected cell and modifying the voltage n place, or by right-
// clicking on an AO cell and selecting an operation from a dedicated popup menu (second popup menu in IDR_CXPOPUPS). 
// Another PB [IDC_AIO_UNITS] selects the voltage display units: volts, millivolts, or raw DAC values.
//    2) Calibration of AI device.  IF the installed AI device supports a "quick" self-calibration in situ, then 
// pressing the IDC_AIO_AICAL button will execute the calibration.  The procedure should take very little time to 
// perform (less than a second or two) and does not require specific input conditions or user interaction.
//    3) Continuous playback of a "canned" test waveform on a **single** selected AO channel.  User selects the channel 
// for test waveform playback by right-clicking on the associated grid cell and selecting "Run Test Waveform" (cmd id 
// ID_TESTAO_WAVE) on the dedicated popup menu -- see resource IDR_CXPOPUPS.  When the test waveform is being played on 
// a channel, the corresponding grid cell reads "*test*" and the user cannot modify the voltage on that channel.  To 
// stop the waveform, right-click on the cell and choose "Stop Test Waveform".
//
// ==> CCxEventTimerDlg.
// The Event Timer DIO Test Panel dialog, CCxEventTimerDlg, is defined in the dlg template resource IDD_TMRCP.  It 
// provides access to the following test & diagnostic capabilities for CNTRLX's "event timer DIO device":
//
//    1) Control of logic state of each channel in the timer's digital output (DO) port.  The current state (1 or 0) is 
// displayed on an MFC grid control (see CREDITS), IDC_TMR_CHANS.  Left-clicking the grid cell for a DO chan will 
// toggle its current state (unless a loopback test is running -- see below).
//    2) Event "statistics" for each channel in the timer's digital input (DI) port.  Four separate columns in the 
// IDC_TMR_CHANS grid control report: most recent event mask (indicates which DI channels recorded an event), #events 
// since last reset, time of most recent event since reset, and the mean interval between events recorded since last 
// reset.  CNTRLX/CXDRIVER defines an "event" as the occurrence of a rising edge (a transition from TTL logic low to 
// logic high) on a DI channel.
//    3) Reset pushbutton (IDC_TMR_RESET).  Depressing this pushbutton resets the event timer device, clears all input 
// statistics, and sets all DO channels to logic low.
//    4) "Loopback test".  Depressing the "Loopback Test" pushbutton (IDC_TMR_LOOP) starts an automated "loopback" test 
// on the event timer board.  This diagnostic test verifies the event-recording functionality of the device and 
// requires a loopback cable connecting the device's digital outputs to its digital inputs.  While the test is in 
// progress, the pushbutton label is disabled and reads "TEST IN PROGRESS".  If the loopback test fails or completes 
// successfully, the button's label will read "TEST FAILED" or "TEST SUCCEEDED" as appropriate, and the button remains 
// disabled.  Once a loopback test is started, the only way to return to normal event timer diagnostic monitoring is to 
// press the "Reset" PB.  Note that during a loopback test the event timer's DO channels cannot be modified by the user 
// -- since CNTRLX is using them to drive the timer's digital inputs!
//    5) "Repeat writes on DO port" (IDC_TMR_REPEAT).  If this pushbutton is pressed, the 16-bit word representing the 
// state of the timer's 16 digital outputs is written to the timer board at regular intervals until the button is 
// pushed again, a loopback test is started, or the timer is reset.  This feature might be used, for example, in 
// testing the so-called "latched external devices" that reside on the timer's DO bus in a Maestro laboratory rig. Each 
// repeat write is actually two writes:  0 followed by the current value of the 16-bit DO word.
// 
// ==> Display of AI and DI channel data during TestMode.
// The user can display selected analog input and digital input channels in the CNTRLX data trace window by choosing 
// one of the channel configuration objects (CCxChannel) defined in the current CNTRLX document (CCxDoc).  To select 
// a channel configuration, use the combo box IDC_AIO_CHLIST on the Analog I/O Test Panel dialog.  Upon doing so, the 
// data trace facility is reinitialized to monitor the channels currently selected for display in the chosen CCxChannel 
// object.  CCxRuntime handles all the details; here we merely provide a means for choosing which CCxChannel object to 
// associate with the data trace facility when in TestMode.
//
// Note that CCxAnalogIODlg and the CNTRLX "mode control" framework is designed to detect CCxDoc changes which could 
// impact the currently displayed channel traces.  Thus, e.g., if you change the trace color associated with a given 
// channel, the change will be immediately reflected in the data trace window!  Similarly, if you delete the CCxChannel 
// object currently associated with TestMode, the data trace facility will be reset.  See CCxAnalogIODlg::OnUpdate().
//
// The data trace facility is active in TestMode only when the AI DAQ is running.  Thus, if the user pauses the DAQ, 
// the data trace display also pauses.
//
// [DEV NOTE:  A better design of TestMode would involve an additional "Control" dialog that contains controls which 
// apply to more than one device.  The combo box, IDC_AIO_CHLIST, is a case in point, as it affects the display of both 
// AI and DI cahnnels.]
//
//
// CREDITS:
// 1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21. 
// The channel grids in CCxAnalogIODlg and CCxEventTimerDlg are implemented with this useful control (CGridCtrl) and 
// its underlying framework.  Using a grid is much more compact and efficient than laying out a bunch of individual 
// edit controls!  Rather than using CGridCtrl itself, we use our own derived class CLiteGrid, which operates in the 
// lightweight "virtual" mode and provides four kinds of inplace editor tools -- relieving the parent dialog/form of 
// that burden.
//
//
// REVISION HISTORY (CCxTestMode):
// 11jun2001-- Created as part of a major "architectural" redesign.  CCxTestMode is one component of CNTRLX's master 
//             runtime controller, CCxRuntime.  It encapsulates the mode control logic, data, and CXDRIVER interactions 
//             for the "Test and Calibration" mode.
// 22jun2001-- Adding test facility for event timer device...
// 29jun2001-- Now tracking the DI event mask for the most recently recorded event on the event timer.
//          -- Loopback test now run by CCxTestMode itself, rather than being an intrinsic test run by CXDRIVER!
// 30jul2001-- Added CanCalibAI() to determine whether the installed device (optionally) supports in-situ self-calib, 
//             and CalibrateAI() to perform the actual operation in test mode.
// 02aug2001-- Added GUI support for running a test waveform on a single chosen AO channel -- CXDRIVER controls the 
//             details of how the waveform is generated.  Command CX_TM_AOWAVE.
// 16aug2001-- Added support for displaying data traces from selected AI or DI channels.
// 01oct2001-- The scheme for selecting AI and DI channels for display in TestMode was replaced entirely.  Now, a 
//             combo box on the control panel (CCxAnalogIODlg) selects an existing channel configuration object which 
//             is used to determine what input channels are displayed in TestMode.  CCxTestMode maintains an internal 
//             copy of the selected channel configuration, masks out all computed channels (which have no meaning in 
//             TestMode), and updates the data trace facility provided by CCxDriverIO as changes warrant.  See methods 
//             Get/Set/ModifyTraceList().
// 03oct2001-- Moved the actual implementation of ***TraceList() to CCxDriverIO.  CCxTestMode versions, now called 
//             ***Traces(), merely invoke analogous CCxDriverIO methods (anticipate getting rid of mode controller 
//             objects in near future). 
// 09oct2001-- Folded into CCxTestPanel.
//
// REVISION HISTORY:
// 24apr2001-- Begun development.
// 04may2001-- Context menu for changing AO is now handled entirely w/in OnGridRClk().  There were problems with menu 
//             item updating (MFC framework doesn't route CN_UPDATE_COMMAND_UI thru control bars), and it was just 
//             simpler to keep everything in one place.  If the popup menu gets more complex, may need to rethink.
// 30may2001-- Design change:  CCxTestPanel is now a tabbed dialog bar derived from CSizingTabDlgBar, with different 
//             dialog pages for different hardware components:  Analog I/O, Event Timer DIO, etc.  For now, there's 
//             just the single Analog I/O dialog, now encapsulated by CCxAnalogIODlg, formerly CCxTestPanelDlg.  A new 
//             CCxControlPanelDlg encapsulates functionality common to all CNTRLX mode control panel dialogs and 
//             serves as a "glue" to, or direct communication link with, CCxDriverIO.  CSizingTabDlgBar provides the 
//             tab control functionality.
// 06jun2001-- Another redesign.  Introduced base class CCxCtrlPanel for single-dialog bars and CCxTabCtrlPanel for 
//             tabbed dialog bars used in CNTRLX.  These base classes handle "common" functionality formerly handled 
//             by CCxTestPanel, which is now much simpler.
// 20jun2001-- Another redesign to conform with changes in CNTRLX's "mode control" architecture.  All activity in the 
//             "Test and Calibration" mode is now handled by the test mode controller, CCxTestMode, rather than the 
//             CXDRIVER interface CCxDriverIO.  In addition, among other technical changes in analog I/O test-mode 
//             functionality, we no longer support presentation of voltages as raw b2s values -- CXDRIVER reports all 
//             analog data as floating-point values in volts.  Instead, we allow user to switch units between "volts" 
//             and "millivolts".
// 25jun2001-- Added CCxEventTimerDlg, which is the control panel dialog that provides the user acces to test and 
//             diagnostic facilities for the event timer DIO device.
// 02jul2001-- Added another column to CCxEventTimerDlg's grid control, to display the most recent input event mask. 
// 30jul2001-- Separate "user" and "factory" calibration PBs on CCxAnalogIODlg replaced by single IDC_AIO_AICAL button. 
//             Enabled and implemented the quick calibration facility.
// 02aug2001-- Adding support for a canned test waveform on a single, selected AO channel.  Feature is accessed via the 
//             right-click context menu associated with the channel grid.  CXDRIVER controls the waveform; when a 
//             waveform is being generated on a channel, the corresponding grid cell reads "*test*" and that channel is 
//             not available for user modification!
// 16aug2001-- Modified CCxAnalogIODlg and CCxEventTimerDlg to allow user to select individual AI and DI channels for 
//             continuous display in the CNTRLX data trace display window.  A left-click on a cell corresponding to a 
//             AI or DI channel will toggle the display state of that channel.  A "*" is appended to the contents of 
//             the cell to indicate when the channel is selected for display, and the cell's background color reflects 
//             the display trace color assigned to that channel.  User does NOT have control of the colors or the 
//             position of the traces (all handled by CCxTestMode controller).
// 02oct2001-- Mod dtd 16aug2001 removed.  Instead, CCxAnalogIODlg contains a combo box (IDC_AIO_CHLIST) -- subclassed 
//             to CCxChannelCombo -- by which user can associate an existing channel configuration object (CCxChannel) 
//             with TestMode.  The TestMode controller uses that object to determine which AI & DI channels to display 
//             and what trace attributes to associate with those channels.  Changes in the selected CCxChannel are 
//             forwarded to the TestMode controller via CCxAnalogIODlg::OnUpdate(). 
// 03oct2001-- Updated IAW changes in CCxTestMode support for data trace display facility...
// 09oct2001-- As part of another architectural redesign, CCxTestPanel is now based upon CCxControlPanel and acts as 
//             both mode controller and dialog bar container.  All functionality formerly in CCxTestMode has been 
//             folded into CCxTestPanel.
// 05nov2001-- CCxChannelCombo superseded by the more generally useful CCxObjCombo.
// 14nov2001-- CCxAnalogIODlg::OnUpdate() modified to handle a NULL hint, which indicates that a different CCxDoc has 
//             just been installed -- an opportunity to carry out any per-document initializations (aka CView's 
//             OnInitialUpdate()).
// 15nov2001-- Got rid of ON_UPDATE_COMMAND_UI_RANGE() handlers in CCxAnalogIODlg & CCxEventTimerDlg.  Instead, ctrls 
//             are updated via the Refresh() mechanism.
// 10jun2002-- After further investigation, decided to respond to CBN_SELCHANGE rather than CBN_CLOSEUP.  The former is 
//             also sent when the user changes the selection with arrow or char keys (droplist closed), and we need to 
//             catch these selection changes as well!
// 31jul2002-- Replaced ::GetTickCount()-based version of CElapsedTime with the more accurate RTX-based version used 
//             on the CXDRIVER side.
// 02dec2002-- The CGridCtrl objects in CCxAnalogIODlg and CCxEventTimerDlg were replaced by the derived class 
//             CLiteGrid, which has built-in inplace editor tools and uses callbacks to get edit info and report the 
//             results of an inplace operation.
// 04apr2003-- MAJOR redesign of the CNTRLX "mode control" framework.  There is now only a single mode control panel, 
//             CCxControlPanel.  CCxTestPanel is replaced by the TestMode controller object CCxTestMode, which is 
//             derived from the abstract base class CCxModeControl.  Mode control dialogs are still derived from the 
//             abstract class CCxControlPanelDlg, but they interact with the "current" mode controller object rather 
//             than a derivative of CCxControlPanel.  See also CCxControlPanel, CCxControlPanelDlg, and CCxModeControl. 
// 30dec2003-- Added pushbutton IDC_TMR_REPEAT to CCxEventTimerDlg -- pressing this button causes CCxTestMode ctrlr to 
//             write the current timer DO word (as reflected in the grid on CCxEventTimerDlg) at every "refresh" intv, 
//             until the button is pushed again, the timer is reset, or a loopback test is started.  On each rewrite, 
//             we first write 0 to clear all outputs momentarily before writing the word again.
// 10mar2005-- Modified CCxAnalogIODlg to display voltages as raw DAC values as well as mV and V.
// 27sep2011-- Modified CxTestMode and CCxAnalogIODlg IAW changes to CX_TM_GETAI command. Maestro now keeps track of
//             the running average and standard deviation in the voltage signal on each AI channel in addition to the
//             last voltage sampled.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"                            // runtime math stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxdoc.h"                           // CCxDoc -- CNTRLX document class
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the CNTRLX master mode control panel
#include "cxruntime.h"                       // CCxRuntime -- the CNTRLX/CXDRIVER runtime interface

#include "cxtestmode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxAnalogIODlg
//
//===================================================================================================================== 
//===================================================================================================================== 

IMPLEMENT_DYNCREATE( CCxAnalogIODlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxAnalogIODlg, CCxControlPanelDlg )
   ON_COMMAND_RANGE( IDC_AIO_UNITS, IDC_AIO_AICAL, OnOp )
   ON_CONTROL( CBN_SELCHANGE, IDC_AIO_CHLIST, OnChangeChanCfg ) 
   ON_NOTIFY( NM_RCLICK, IDC_AIO_CHANS, OnGridRClk )
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxAnalogIODlg [constructor] ==================================================================================== 
//
//    Construct the analog I/O test panel dialog. 
//
//    Almost all the work is handled by the underlying framework, which loads dialog's layout from a CNTRLX resource 
//    whose identifier is stored in CCxAnalogIODlg's protected member var IDD.  However, we do need to initialize 
//    certain variables that track the (modeless) dialog's state.
//
CCxAnalogIODlg::CCxAnalogIODlg() : CCxControlPanelDlg( IDD ) 
{
   m_iUnits = CCxAnalogIODlg::VOLTS;            // initially, analog I/O signal data are displayed in volts
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnOp ============================================================================================================ 
//
//    ON_COMMAND_RANGE handler for the various button-initiated operations on the analog I/O test panel dialog:
//
//       IDC_AIO_UNITS     ==> Toggle display units among "volts, "mV", and "raw".
//       IDC_AIO_PAUSE     ==> Pause/resume periodic sampling of analog input channel readings. 
//       IDC_AIO_AICAL     ==> Perform quick, in-situ calibration of the analog input device. 
//
//    ARGS:       cmdID -- [in] the command ID
// 
//    RETURNS:    NONE
//
void CCxAnalogIODlg::OnOp( UINT cmdID )
{
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode );  // get ptr to current mode controller

   switch( cmdID )
   {
      case IDC_AIO_UNITS : 
         if( m_iUnits == CCxAnalogIODlg::VOLTS ) m_iUnits = CCxAnalogIODlg::MILLIVOLTS;
         else if( m_iUnits == CCxAnalogIODlg::MILLIVOLTS ) m_iUnits = CCxAnalogIODlg::RAW;
         else m_iUnits = CCxAnalogIODlg::VOLTS;
         Refresh(); 
         break;
      case IDC_AIO_PAUSE : 
         pTestMode->ToggleAISampling();
         Refresh();
         break;
      case IDC_AIO_AICAL : 
         pTestMode->CalibrateAI();
         break;
      default :
         TRACE0( "Unrecognized command op in test mode!\n" );
         break;
   } 

}


//=== OnChangeChanCfg ================================================================================================= 
//
//    Update the CNTRLX data trace display whenever the user finishes selecting a channel configuration from the 
//    drop-list combo IDC_AIO_CHLIST.  We respond to the CBN_SELCHANGE notification rather than CBN_CLOSEUP, because 
//    the user can change the selection via arrow or character keys when the combo box has the focus, even if the drop 
//    list is closed, and only CBN_SELCHANGE is sent in this case. 
//
//    NOTE:  In TestMode, the trace display width is always 5000 samples.
//
//    ARGS:       NONE.
// 
//    RETURNS:    NONE.
//
void CCxAnalogIODlg::OnChangeChanCfg()
{
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode ); 

   pTestMode->SetTraces( m_cbSelChan.GetObjKey(), 5000 );                  // if sel unchanged, nothing should happen!
}


//=== OnGridRClk ====================================================================================================== 
//
//    Response to the NM_RCLICK notification from the channel grid control.
//
//    Whenever the user right-clicks on a valid analog output cell in the channel grid, we throw up a popup context 
//    menu (submenu 1 of the menu resource IDR_CXPOPUPS -- see CNTRLX resource file),  which allows the user to perform 
//    a number of operations related to the AO device:
// 
//       ID_TESTAO_ZERO ==> Zero all AO channels.
//       ID_TESTAO_MAX  ==> Maximize voltage on all AO channels.
//       ID_TESTAO_MIN  ==> Minimize voltage on all AO channels.
//       ID_TESTAO_THIS ==> Set all AO channels to the voltage on the selected channel.
//       ID_TESTAO_WAVE ==> Start test waveform generation on the selected AO channel.  If it is already running on 
//                          this channel, then stop the waveform.
//
//    The MFC framework does not support routing of CN_UPDATE_COMMAND_UI from frame window to its control bars.  Thus, 
//    if we pass the main frame window to TrackPopupMenu(), the popup menu items will all be disabled.  Since this 
//    popup menu is ONLY for use on this dialog, we chose to update the menu item states AND process the selected 
//    command in this method.  For now, all menu items are always enabled, since we only show the menu if the 
//    right-clicked cell represents an analog output cell.
//
//    ARGS:       pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_RCLICK. 
// 
//    RETURNS:    NONE
//
void CCxAnalogIODlg::OnGridRClk( NMHDR* pNMHDR, LRESULT* pResult )
{
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode ); 

   NM_GRIDVIEW* pNMGV = (NM_GRIDVIEW*) pNMHDR; 
   *pResult = TRUE;                                               // return value is irrelevant for NM_RCLICK 

   CCellID clickCell( pNMGV->iRow, pNMGV->iColumn );              // the right-clicked cell
   if( !IsValidAOCell( clickCell ) ) return;                      // if its not a valid AO cell, ignore!

   m_chanGrid.SetFocus();                                         // a right-click does not give grid ctrl the focus... 

   CMenu menu;
   if ( menu.LoadMenu( IDR_CXPOPUPS ) )                           // load CNTRLX popup menus from resource
   {
      CPoint ptULC;                                               // popup's ULC will align with current mouse pos 
      ::GetCursorPos( &ptULC ); 

      int iCh = clickCell.row - 1;                                // the AO channel selected by right-click
      BOOL bTest = BOOL(pTestMode->GetTestWaveformCh() == iCh);   // is test waveform running on this channel?

      CMenu* pPopup = menu.GetSubMenu( 1 );                       // get the popup menu, and update label for menu 
      CString strLabel = (bTest) ? _T("Stop ") : _T("Run ");      // item ID_TESTAO_WAVE 
      strLabel += _T("Test &Waveform");
      pPopup->ModifyMenu( ID_TESTAO_WAVE, MF_STRING,
                          ID_TESTAO_WAVE, strLabel );

      UINT chosen = (UINT) pPopup->TrackPopupMenu(                // run the popup
               TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | 
               TPM_RETURNCMD,                                     // so TPM() returns cmd ID of item selected
               ptULC.x, ptULC.y, this );

      if( chosen != 0 )                                           // perform selected operation:
      {
         float fVolt;                                             //    get selected voltage -- for min/max, we just 
         switch( chosen )                                         //    select a value whose magnitude is very large 
         {
            case ID_TESTAO_ZERO  : fVolt = 0.0f; break;
            case ID_TESTAO_MAX   : fVolt = 100.0f; break;
            case ID_TESTAO_MIN   : fVolt = -100.0f; break;
            case ID_TESTAO_THIS  : fVolt = pTestMode->GetAOChannel( iCh ); break;
            case ID_TESTAO_WAVE  : pTestMode->RunTestWaveform( bTest ? -1 : iCh ); break;
            default : TRACE0( "Unrecognized channel grid cmd in test mode!\n" ); return;
         } 

         if( chosen != ID_TESTAO_WAVE )                           //    all other ops set all AO channels to voltage
            pTestMode->SetAOChannel( fVolt );                     //       selected above
         m_chanGrid.RedrawColumn( 2 );                            //    update AO column of grid to reflect changes
      }
   }
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== OnInitDialog [base override] ==================================================================================== 
//
//    Prepare the dialog for display.
//
//    Here we subclass many of the dialog controls to CWnd-based class members:  a grid control for displaying the 
//    analog input and output channel voltages; and a numeric edit control for setting all analog output channels to 
//    the same voltage. 
//
//    The I/O channel grid is setup as follows:  There is one fixed row containing the column headings "CH#", 
//    "INPUT", "MEAN", "STD", and "OUTPUT".  There is one fixed column containing the channel #.  There are four 
//    additional columns. Columns 1-3 apply to the AI channels: column 1 displays the current reading on each channel, 
//    column 2 displays the running average, and column 3 displays its standard deviation. Column 4 displays the current
//    voltage driven on each analog output channel. The # of additional rows = max(#inputs, #outputs).  We query the 
//    CNTRLX app object to determine these parameters. The grid is used in "virtual mode" to reduce its memory 
//    impact; thus, the contents of every grid cell are defined entirely by the callback fcn GridDispCB().
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CMemoryException if grid controls cannot allocate memory for initial rows and columns.
//
BOOL CCxAnalogIODlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                               // let base class do its thing...

   m_cbSelChan.SubclassDlgItem( IDC_AIO_CHLIST, this );              // subclass combo box for selecting chan cfg obj  
                                                                     // associated with TestMode. 
   
   m_chanGrid.SubclassDlgItem( IDC_AIO_CHANS, this );                // prepare grid ctrl to display AI/AO channels: 
   m_chanGrid.EnableDragAndDrop( FALSE );                            //    disable drag-n-drop features
   m_chanGrid.SetRowResize( FALSE );                                 //    user may not resize rows or cols
   m_chanGrid.SetColumnResize( FALSE );                              // 
   m_chanGrid.EnableSelection( FALSE );                              //    cells in grid cannot be selected

   m_chanGrid.SetCallbackFunc( GridDispCB, (LPARAM) this );          //    set CB fcns which govern appearance/editing 
   m_chanGrid.SetEditCBFcn( GridEditCB, (LPARAM) this );             //    of grid cells.  TRICK: we pass THIS 
   m_chanGrid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );       //    reference b/c CB fcn must be static 

   int nRows = GetModeCtrl( CCxRuntime::TestMode )->GetNumAI();      //    determine # rows required in grid 
   int nOut = GetModeCtrl( CCxRuntime::TestMode )->GetNumAO();
   if( nOut > nRows ) nRows = nOut;
   if( nRows == 0 ) nRows = 1;                                       //    at least one non-fixed row

   m_chanGrid.SetRowCount( nRows + 1 );                              //    set up rows & columns
   m_chanGrid.SetColumnCount( 5 );                                   //    ...may THROW CMemoryException. 
   m_chanGrid.SetFixedRowCount( 1 );
   m_chanGrid.SetFixedColumnCount( 1 );

   CGridCellBase* pCell = m_chanGrid.GetDefaultCell( TRUE, TRUE );   //    default format for fixed rowcol cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 

   pCell = m_chanGrid.GetDefaultCell( TRUE, FALSE );                 //    for fixed row-only cell
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 

   pCell = m_chanGrid.GetDefaultCell( FALSE, TRUE );                 //    for fixed column-only cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 
                     
   pCell = m_chanGrid.GetDefaultCell( FALSE,FALSE );                 //    for non-fixed cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );

   m_chanGrid.AutoSize();                                            //    first auto-size
   m_chanGrid.ExpandColumnsToFit( FALSE );                           //    non-fixed columns sized to fill control area 
   m_chanGrid.ExpandRowsToFit( FALSE );                              //    non-fixed rows sized to fill control area

   return( TRUE );                                                   // set input focus to 1st ctrl in tab order 
}


//=== Refresh ========================================================================================================= 
//
//    Refresh appearance of dialog controls -- typically called by the parent mode control panel when the runtime state 
//    and/or data changes.
//
//    Here we refresh the analog I/O channel grid and the dialog pushbuttons, as follows:
//
//       IDC_AIO_UNITS  ==> Label reads "volts" or "mV"
//       IDC_AIO_PAUSE  ==> Label reads "Pause Inputs" or "Resume Inputs". 
//       IDC_AIO_AICAL  ==> Enabled only if self-calibration is supported by the AI device. 
//
//    ARGS:       NONE.
// 
//    RETURNS:    NONE.
//
VOID CCxAnalogIODlg::Refresh() 
{ 
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode ); 

   m_chanGrid.Refresh();

   m_cbSelChan.SetObjKey( pTestMode->GetTraces() );                  // ensure chan cfg combo box shows chan cfg now 
                                                                     // in use by the data trace display facility

   CWnd* pWnd = GetDlgItem( IDC_AIO_UNITS );
   ASSERT( pWnd );
   CString units;
   if( m_iUnits == CCxAnalogIODlg::VOLTS ) units = _T("volts");
   else if( m_iUnits == CCxAnalogIODlg::MILLIVOLTS ) units = _T("mV");
   else units = "rawDAC";
   pWnd->SetWindowText( units );

   pWnd = GetDlgItem( IDC_AIO_PAUSE );
   ASSERT( pWnd );
   pWnd->SetWindowText( pTestMode->IsAIPaused() ? _T("Resume Inputs") : _T("Pause Inputs") );

   pWnd = GetDlgItem( IDC_AIO_AICAL );
   ASSERT( pWnd );
   pWnd->EnableWindow( pTestMode->CanCalibAI() );
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
//    Here we check for any change in CCxDoc that **might** affect the contents of combo box IDC_AIO_CHLIST, in which 
//    case we refresh the contents of that box and make sure that the current selection is still correct.  The combo 
//    box lists the names of all channel configuration objects (children of the predefined CX_CHANBASE object) defined 
//    in CCxDoc, and its current selection indicates which channel configuration, if any, is currently associated with 
//    the data trace facility during TestMode.
//
//    Each time a new CCxDoc is created/opened, we need to reinitialize the contents of the CNTRLX object combo box 
//    contained in this dialog IAW with the just-installed document.  The selection is reset to "NONE".
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxAnalogIODlg::OnUpdate( CCxViewHint* pHint )
{
   if( pHint == NULL )                                               // "per-document inits" -- reinitialize contents 
   {                                                                 // of the combo box selecting display channels
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      ASSERT( pDoc != NULL );
      m_cbSelChan.InitContents( pDoc->GetBaseObj( CX_CHANBASE ),     // all chan cfgs are children of this base obj
                                TRUE );                              // allow the "NONE" option
   }
   else if( 
      (pHint->m_code == CXVH_NEWOBJ && (pHint->m_type == CX_CHANCFG || pHint->m_key == CX_NULLOBJ_KEY)) ||
      (pHint->m_code == CXVH_NAMOBJ && pHint->m_type == CX_CHANCFG) ||
      (pHint->m_code == CXVH_CLRUSR) ||
      (pHint->m_code == CXVH_DELOBJ && (pHint->m_type == CX_CHANCFG || pHint->m_key == CX_NULLOBJ_KEY))
      )
   {
      m_cbSelChan.RefreshContents();
      WORD wKey = GetModeCtrl( CCxRuntime::TestMode )->GetTraces();
      if( wKey != m_cbSelChan.GetObjKey() )
      {
         m_cbSelChan.SetObjKey( wKey );
         ASSERT( wKey == m_cbSelChan.GetObjKey() );
      }
   }
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== GridDispCB ====================================================================================================== 
//
//    Callback function queried by the embedded channel grid to obtain the contents of each cell in the grid.
//
//    Here we provide the display info for each cell in the analog I/O channel grid, based on information queried from 
//    the CNTRLX app object (N = #channels in grid; NI,NO = #analog inputs,outputs available):
//       (1) Cell in fixed row 0 ==> Column labels: "CH#", "INPUT", "MEAN", "STD", "OUTPUT".
//       (2) Cell in fixed col 0 ==> Channel number.
//       (3) Cell in col 1       ==> Rows [1..NI] Most recent reading on analog input channel [0..NI-1], displayed in 
//           volts with 5 digits precision, in millivolts with two digits precision, or as an integral raw ADC value.
//                               ==> Rows [NI+1..N] "N/A".
//       (4) Cell in col 2       ==> Rows [1..NI] The running mean of samples recorded on AI channel [0..NI-1], 
//           displayed in the same units as in (3).
//                               ==> Rows [NI+1..N] "N/A".
//       (5) Cell in col 3       ==> Rows [1..NI] The standard deviation of the mean of samples recorded on AI channel
//           [0..NI-1], displayed in the same units as in (3).
//                               ==> Rows [NI+1..N] "N/A".
//       (6) Cell in col 4       ==> Rows [1..NO] Current value for analog output channel [1..M], displayed in the same
//           units as in (3). If AO device is not available, reads "N/A". If the canned test waveform is currently 
//           running on the channel, reads "*test*".
//                               ==> Rows [NO+1..N] "N/A".
//
//    NOTE: A callback function must be static.  As such, it does not have access to instance fields of the object.  To 
//    circumvent this problem, we take advantage of the generic LPARAM argument, passing a reference to this dlg when 
//    we register the callback fcn with the grid in OnInitDialog().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info. 
//                lParam      -- [in] THIS (see NOTE).
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxAnalogIODlg::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam ) 
{
   CCxAnalogIODlg* pThis = (CCxAnalogIODlg*)lParam;                        // to access non-static data!
   CCxTestMode* pTestMode =                                                // to access TestMode controller
      (CCxTestMode*) pThis->GetModeCtrl( CCxRuntime::TestMode ); 
   CLiteGrid* pGrid = &(pThis->m_chanGrid);                                // the channel grid
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );                  // the cell whose info is requested

   if( (pGrid->GetSafeHwnd() == NULL) || !pGrid->IsValid( c ) )            // FAIL if grid control is gone, or the 
      return( FALSE );                                                     // specified cell is non-existent 

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                     // we don't use label tips on this grid
   { 
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP; 
      return( TRUE );
   }

   CString& rStrCell = pDispInfo->item.strText;                            // ref to string to be displayed in cell

   int iCh = c.row - 1;                                                    // channel # (if applicable) 
   if( c.row == 0 )                                                        // column headings in the first row
   {
      switch( c.col ) 
      {
         case 0 : rStrCell = _T("CH#"); break;
         case 1 : rStrCell = _T("INPUT"); break;
         case 2 : rStrCell = _T("MEAN"); break;
         case 3 : rStrCell = _T("STD"); break;
         case 4 : rStrCell = _T("OUTPUT"); break;
         default: ASSERT( FALSE ); break;
      }
   }
   else if ( c.col == 0 )                                                  // row label is the channel number
      rStrCell.Format( "%d", iCh );
   else if(pThis->IsValidAICell(c)) 
   {
      // display requested statistic (last sample, mean, or std dev) for specified input channel
      CCxTestMode::AIDatum which = CCxTestMode::AID_LAST;
      if(c.col == 2) which = CCxTestMode::AID_AVG;
      else if(c.col == 3) which = CCxTestMode::AID_STD;
      
      if(pThis->m_iUnits == CCxAnalogIODlg::RAW)
      {
         int iVal = pTestMode->GetAIChannelRaw(iCh, which);
         rStrCell.Format("%d", iVal);
      }
      else
      {
         float fVal = pTestMode->GetAIChannel(iCh, which);
         int pre = 5;
         if(pThis->m_iUnits == CCxAnalogIODlg::MILLIVOLTS) 
         {
            fVal *= 1000.0f;
            pre = 2;
         }
         rStrCell.Format("%.*f", pre, fVal);
      }
   }
   else if( pThis->IsValidAOCell( c ) )                                    // display current voltage delivered on 
   {                                                                       // specified output channel, unless test 
      if( pTestMode->GetTestWaveformCh() == iCh )                          // waveform currently running on that chan 
         rStrCell = _T("*test*"); 
      else if( pThis->m_iUnits == CCxAnalogIODlg::RAW )
      {
         int iVal = pTestMode->GetAOChannelRaw( iCh );
         rStrCell.Format( "%d", iVal );
      }
      else
      {
         float fVal = pTestMode->GetAOChannel( iCh );
         int pre = 5;
         if( pThis->m_iUnits == CCxAnalogIODlg::MILLIVOLTS ) 
         {
            fVal *= 1000.0f;
            pre = 2;
         }
         rStrCell.Format( "%.*f", pre, fVal );
      }
   }
   else if(c.col >= 1 || c.col <= 4)                                       // any other cells in these cols read "N/A" 
      rStrCell = _T("N/A");


   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                        // show title tip only if text does not fit 
   return( TRUE );
}


//=== GridEditCB ====================================================================================================== 
//
//    Callback invoked to initiate inplace editing of a cell in the AIO channel grid.  We only support inplace editing 
//    of the cells that reflect the current AO channel voltages.  Mouse clicks do not affect cell contents.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info. 
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxAnalogIODlg::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxAnalogIODlg* pThis = (CCxAnalogIODlg*)lParam;                        // to access non-static data!
   CCxTestMode* pTestMode =                                                // to access TestMode controller
      (CCxTestMode*) pThis->GetModeCtrl( CCxRuntime::TestMode ); 
   CLiteGrid* pGrid = &(pThis->m_chanGrid);                                // the channel grid
   CCellID c = pEI->cell;                                                  // the cell to be edited
   int iCh = c.row - 1;                                                    // index of relevant AI or AO channel

   if( (pGrid->GetSafeHwnd() == NULL) || !pGrid->IsValid( c ) )            // FAIL if grid control is gone, or the 
      return( FALSE );                                                     // specified cell is non-existent 

   if( pEI->iClick == 0 &&                                                 // mouse clicks do not affect cell contents! 
       pThis->IsValidAOCell(c) && pTestMode->GetTestWaveformCh() != iCh )  // only AO voltage cells are editable; if 
   {                                                                       // test waveform is running on a channel, 
      pEI->iType = LG_NUMSTR;                                              // the corres cell cannot be edited...
      BOOL isRaw = BOOL(pThis->m_iUnits == CCxAnalogIODlg::RAW);
      pEI->numFmt.flags = isRaw ? NES_INTONLY : 0;
      pEI->numFmt.nLen = isRaw ? 6 : 9;
      pEI->numFmt.nPre = isRaw ? 1 : (pThis->m_iUnits==CCxAnalogIODlg::VOLTS ? 5 : 2); 
      if( isRaw )
         pEI->dCurrent = double(pTestMode->GetAOChannelRaw(iCh));
      else
      {
         double d = double( pTestMode->GetAOChannel(iCh) );
         if( pThis->m_iUnits==CCxAnalogIODlg::MILLIVOLTS ) d *= 1000.0;
         pEI->dCurrent = d;
      }
   }
   else
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
   }
   return( TRUE );
}


//=== GridEndEditCB =================================================================================================== 
//
//    Callback invoked upon termination of inplace editing on the AIO channel grid.
//
//    Here we update the drive voltage on the appropriate AO channel IAW the change made during the inplace operation 
//    that was configured in GridEditCB().  In addition, we determine the next cell (if any) at which to continue 
//    inplace editing IAW the "exit character" that extinguished the just-finished inplace operation.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxAnalogIODlg::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   CCxAnalogIODlg* pThis = (CCxAnalogIODlg*)lParam;                        // to access non-static data!
   CCxTestMode* pTestMode =                                                // to access TestMode controller
      (CCxTestMode*) pThis->GetModeCtrl( CCxRuntime::TestMode ); 
   CLiteGrid* pGrid = &(pThis->m_chanGrid);                                // the channel grid
   CCellID c = pEEI->cell;                                                 // the cell that was edited
   int iCh = c.row - 1;                                                    // index of relevant AI or AO channel

   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                      // user cancelled 

   if( (pGrid->GetSafeHwnd() == NULL) || (!pGrid->IsValid( c )) ||         // FAIL if grid control is gone, the 
       (!pThis->IsValidAOCell( c )) )                                      // specified cell is non-existent, or it 
      return( FALSE );                                                     // does not display an AO channel voltage.

   if( pEEI->bIsChanged )                                                  // update AO channel voltage IAW results of 
   {                                                                       // inplace operation...
      if( pThis->m_iUnits == CCxAnalogIODlg::RAW )
      {
         int iOut = int(pEEI->dNew);
         pTestMode->SetAOChannelRaw( iOut, iCh );
      }
      else
      {
         float fOut = float(pEEI->dNew);
         if( pThis->m_iUnits == CCxAnalogIODlg::MILLIVOLTS ) fOut /= 1000.0f; 
         pTestMode->SetAOChannel( fOut, iCh ); 
      }
   }

   pEEI->cellNext = c;                                                     // if exit char was an appropriate navig 
   switch( pEEI->nExitChar )                                               // key, allow user to edit the cell above or 
   {                                                                       // below the current cell, wrapping around 
      case VK_UP :                                                         // grid bottom or top edge as necessary
         if( --iCh < 0 ) pEEI->cellNext.row = pTestMode->GetNumAO();
         else pEEI->cellNext.row = iCh+1;
         break;
      case VK_DOWN :
      case VK_TAB :
         if( ++iCh >= pTestMode->GetNumAO() ) pEEI->cellNext.row = 1;
         else pEEI->cellNext.row = iCh+1;
         break;
      default :                                                            // (all other navig keys ignored)
         pEEI->cellNext.row = -1;
         break;
   }
   return( TRUE );
}


//=== IsValidAOCell, IsValidAICell ==================================================================================== 
//
//    Does this cell in the grid control IDC_AIO_CHANS display an analog output channel voltage? an AI voltage?  
//
//    ARGS:       c  -- [in] the grid cell to be checked.
//
//    RETURNS:    TRUE if it does display an AO (AI) voltage; else FALSE.
//
BOOL CCxAnalogIODlg::IsValidAOCell( CCellID c )
{
   int n = GetModeCtrl( CCxRuntime::TestMode )->GetNumAO();
   return( m_chanGrid.IsValid(c) && (c.col == 4) && (c.row > 0) && (c.row <= n) );
}

BOOL CCxAnalogIODlg::IsValidAICell( CCellID c )
{
   int n = GetModeCtrl( CCxRuntime::TestMode )->GetNumAI();
   return( m_chanGrid.IsValid(c) && (c.col > 0 && c.col < 4) && (c.row > 0) && (c.row <= n) );
}






//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxEventTimerDlg
//
//===================================================================================================================== 
//===================================================================================================================== 

IMPLEMENT_DYNCREATE( CCxEventTimerDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxEventTimerDlg, CCxControlPanelDlg )
   ON_COMMAND_RANGE( IDC_TMR_RESET, IDC_TMR_REPEAT, OnOp )
END_MESSAGE_MAP()


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnOp ============================================================================================================ 
//
//    ON_COMMAND_RANGE handler for the various PB-initiated operations on the event timer DIO test panel dialog:
//
//       IDC_TMR_RESET     ==> Reset the event timer device and clear current input event stats.
//       IDC_TMR_LOOP      ==> Start an automated "loopback" test on the event timer.
//       IDC_TMR_REPEAT    ==> Toggle the "repeat DO write" function on or off.
//
//    ARGS:       cmdID -- [in] the command ID
// 
//    RETURNS:    NONE
//
void CCxEventTimerDlg::OnOp( UINT cmdID )
{
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode ); 
   ASSERT( pTestMode->IsTimerAvailable() );
   switch( cmdID )
   {
      case IDC_TMR_RESET :  pTestMode->ResetTimer(); break;
      case IDC_TMR_LOOP  :  pTestMode->StartTimerLoop(); break;
      case IDC_TMR_REPEAT:  pTestMode->ToggleTimerRepetitiveWrite(); break;
      default : TRACE0( "Unrecognized command op in test mode!\n" ); return;
   } 
   Refresh();
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== OnInitDialog [base override] ==================================================================================== 
//
//    Prepare the dialog for display.
//
//    Here we subclass the grid ctrl to a CLiteGrid-based class member and set it up to display event timer diagnostic 
//    data:  There are five columns and 1+max(DI,DO) rows, where DI is the # of available digital inputs on the timer 
//    device and DO is the # of outputs.  The first (fixed) row contains column headings, and the remaining rows 
//    contain per-channel diagnostic data, as follows:
//
//       column 0 ==> (fixed) channel #.
//       column 1 ==> current states of event timer's digital outputs: 0 for logic low, 1 for logic high. 
//       column 2 ==> most recent input event bit mask: 1/0 = event did/did not occur on corresponding DI channel. 
//       column 3 ==> # events recorded on DI channel since last device reset. 
//       column 4 ==> time of last event recorded on DI channel since last device reset.
//       column 5 ==> mean "interevent" interval over all events recorded on DI channel since last reset.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for initial rows and columns.
//
BOOL CCxEventTimerDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                               // let base class do its thing...

   m_chanGrid.SubclassDlgItem( IDC_TMR_CHANS, this );                // prepare grid ctrl to display event tmr data...  
   m_chanGrid.EnableDragAndDrop( FALSE );                            // disable drag-n-drop features
   m_chanGrid.SetRowResize( FALSE );                                 // user may not resize rows or cols
   m_chanGrid.SetColumnResize( FALSE );                              // 
   m_chanGrid.EnableSelection( FALSE );                              // cells in grid cannot be selected

   m_chanGrid.SetCallbackFunc( GridDispCB, (LPARAM) this );          // set CB fcns which govern appearance/editing 
   m_chanGrid.SetEditCBFcn( GridEditCB, (LPARAM) this );             // of grid cells.  TRICK: we pass THIS 
   m_chanGrid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );       // reference b/c CB fcn must be static 

   int nRows = GetModeCtrl( CCxRuntime::TestMode )->GetNumTDI();     // determine # rows required in grid 
   int nOut = GetModeCtrl( CCxRuntime::TestMode )->GetNumTDO();
   if( nOut > nRows ) nRows = nOut;
   if( nRows == 0 ) nRows = 1;                                       // at least one non-fixed row

   m_chanGrid.SetRowCount( nRows + 1 );                              //  set up rows & columns
   m_chanGrid.SetColumnCount( 6 );                                   //  ...may THROW CMemoryException. 
   m_chanGrid.SetFixedRowCount( 1 );
   m_chanGrid.SetFixedColumnCount( 1 );

   CGridCellBase* pCell = m_chanGrid.GetDefaultCell( TRUE, TRUE );   // default format for fixed rowcol cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 

   pCell = m_chanGrid.GetDefaultCell( TRUE, FALSE );                 // for fixed row-only cell
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 

   pCell = m_chanGrid.GetDefaultCell( FALSE, TRUE );                 // for fixed column-only cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 
                     
   pCell = m_chanGrid.GetDefaultCell( FALSE,FALSE );                 // for non-fixed cell 
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );

   m_chanGrid.AutoSize();                                            // first auto-size
   m_chanGrid.ExpandColumnsToFit( FALSE );                           // non-fixed columns sized to fill control area 
   m_chanGrid.ExpandRowsToFit( FALSE );                              // non-fixed rows sized to fill control area

   int nExtra = m_chanGrid.GetColumnWidth( 1 ) - 20;                 // if column 1 is too wide, reduce its width and 
   if( nExtra > 12 )                                                 // pad the last three columns equally
   {
      int nAdd = nExtra / 3;
      m_chanGrid.SetColumnWidth( 1, 20 );
      m_chanGrid.SetColumnWidth( 3, nAdd + m_chanGrid.GetColumnWidth(3) );
      m_chanGrid.SetColumnWidth( 4, nAdd + m_chanGrid.GetColumnWidth(4) );
      nAdd = nExtra - (2*nAdd);
      m_chanGrid.SetColumnWidth( 5, nAdd + m_chanGrid.GetColumnWidth(5) );
   }

   nExtra = m_chanGrid.GetColumnWidth( 2 ) - 20;                     // if column 2 is too wide, reduce its width and 
   if( nExtra > 12 )                                                 // pad the last three columns equally
   {
      int nAdd = nExtra / 3;
      m_chanGrid.SetColumnWidth( 2, 20 );
      m_chanGrid.SetColumnWidth( 3, nAdd + m_chanGrid.GetColumnWidth(3) );
      m_chanGrid.SetColumnWidth( 4, nAdd + m_chanGrid.GetColumnWidth(4) );
      nAdd = nExtra - (2*nAdd);
      m_chanGrid.SetColumnWidth( 5, nAdd + m_chanGrid.GetColumnWidth(5) );
   }

   nExtra = m_chanGrid.GetColumnWidth( 3 ) - 50;                     // if column 3 is too wide, reduce its size and 
   if( nExtra > 8 )                                                  // pad the last two columns equally
   {
      int nAdd = nExtra / 2;
      m_chanGrid.SetColumnWidth( 3, 50 );
      m_chanGrid.SetColumnWidth( 4, nAdd + m_chanGrid.GetColumnWidth(4) );
      nAdd = nExtra - nAdd;
      m_chanGrid.SetColumnWidth( 5, nAdd + m_chanGrid.GetColumnWidth(5) );
   }

   return( TRUE );                                                   // set input focus to 1st ctrl in tab order 
}


//=== Refresh ========================================================================================================= 
//
//    Refresh appearance of dialog controls -- typically called by the parent mode control panel when the runtime state 
//    and/or data changes.
//
//    Here we refresh the timer I/O channel grid, update the pushbutton IDC_TMR_LOOP, whose enable state & label 
//    reflects the status of the timer loopback test, and update the pushbutton IDC_TMR_REPEAT, whose label reflects 
//    the on/off state of the "repetitive timer write" test function.  This latter PB is disabled when the timer 
//    loopback test is engaged.
//
//    ARGS:       NONE.
// 
//    RETURNS:    NONE.
//
VOID CCxEventTimerDlg::Refresh()
{
   CCxTestMode* pTestMode = (CCxTestMode*) GetModeCtrl( CCxRuntime::TestMode ); 

   m_chanGrid.Refresh(); 

   BOOL bEnable = FALSE;
   CString strLabel;
   switch( pTestMode->GetTimerLoopStatus() )
   {
      case CCxTestMode::TLBS_NOTRUNNING :  strLabel = _T("Loopback Test"); bEnable = TRUE; break;
      case CCxTestMode::TLBS_RUNNING    :  strLabel = _T("TEST IN PROGRESS"); break;
      case CCxTestMode::TLBS_DONE       :  strLabel = _T("TEST SUCCEEDED!"); break;
      case CCxTestMode::TLBS_FAILED     :  strLabel = _T("TEST FAILED!"); break;
   }
   CWnd* pWnd = GetDlgItem( IDC_TMR_LOOP );
   ASSERT( pWnd );
   pWnd->SetWindowText( strLabel );
   pWnd->EnableWindow( bEnable );

   pWnd = GetDlgItem( IDC_TMR_REPEAT );
   ASSERT( pWnd );
   pWnd->SetWindowText( pTestMode->IsTimerRepetitiveWriteOn() ? _T("Exit Repeat") : _T("Repeat Write") );
   pWnd->EnableWindow( bEnable );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== GridDispCB ====================================================================================================== 
//
//    Callback function queried by the embedded grid control to obtain the contents of each cell in the grid.
//
//    Here we provide the display info for each cell in the event timer diagnostic data grid, based on information 
//    queried from the CNTRLX Test Mode ctrl panel (N = #channels in grid; NI,NO = #digital inputs,outputs available):
//       Cell in fixed row 0 ==> Label of attribute displayed in that column.
//       Cell in fixed col 0 ==> Channel number.
//       Cell in col 1       ==> Current state of timer device's corresponding DO channel -- "0" or "1".
//       Cell in col 2       ==> Most recent DI event mask -- "0" if no event on DI chan, "1" if event occurred.
//       Cell in col 3       ==> # of input "events" that have been recorded on corresponding DI channel. 
//       Cell in col 4       ==> Time of occurrence of most recent event on corres DI channel.
//       Cell in col 5       ==> Mean "interevent" interval for events recorded on corres DI channel.
//    Note that we support the possibility that there could be more inputs than outputs, or vice versa.  In that case, 
//    if a non-fixed row cell does not correspond to a real channel, the cell reads "N/A".
//
//    NOTE: A callback function must be static.  As such, it does not have access to instance fields of the object.  To 
//    circumvent this problem, we take advantage of the generic LPARAM argument, passing a reference to this dlg when 
//    we register the callback fcn with the grid in OnInitDialog().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info. 
//                lParam      -- [in] THIS (see NOTE).
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxEventTimerDlg::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam ) 
{
   CCxEventTimerDlg* pThis = (CCxEventTimerDlg*)lParam;                    // to access non-static data!
   CCxTestMode* pTestMode =                                                // to access TestMode controller
      (CCxTestMode*) pThis->GetModeCtrl( CCxRuntime::TestMode ); 
   CLiteGrid* pGrid = &(pThis->m_chanGrid);                                // the channel grid
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );                  // the cell whose info is requested

   if( (pGrid->GetSafeHwnd() == NULL) || !pGrid->IsValid( c ) )            // FAIL if grid control is gone, or the 
      return( FALSE );                                                     // specified cell is non-existent 

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                     // we don't use label tips on this grid
   { 
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP; 
      return( TRUE );
   }

   CString& rStrCell = pDispInfo->item.strText;                            // ref to string to be displayed in cell

   int iCh = c.row - 1;                                                    // channel # (if applicable) 
   if( c.row == 0 )                                                        // column headings in the first row
   {
      switch( c.col ) 
      {
         case 0 : rStrCell = _T("CH#"); break;
         case 1 : rStrCell = _T("DO"); break;
         case 2 : rStrCell = _T("DI"); break;
         case 3 : rStrCell = _T("#Events"); break;
         case 4 : rStrCell = _T("Last Event Time"); break;
         case 5 : rStrCell = _T("Mean Event Intv"); break;
         default: ASSERT( FALSE ); break;
      }
   }
   else if ( c.col == 0 )                                                  // row label is channel #: "N"
      rStrCell.Format( "%d", iCh );
   else if( pThis->IsValidTDOCell( c ) )                                   // display current logic state of DO chan 
   { 
      rStrCell.Format( "%d", pTestMode->IsTDOChannelOn( iCh ) ? 1 : 0 );
   }
   else if( pThis->IsValidTDICell( c ) )                                   // display an event stat for DI chan...
   { 
      if( c.col == 2 ) 
         rStrCell.Format( "%d", pTestMode->IsTDILastEventOn( iCh ) ? 1 : 0 );  
      else if( c.col == 3 )
         rStrCell.Format( "%d", pTestMode->GetTimerInputEvents( iCh ) );
      else 
      {
         float fTime = (c.col == 4) ? pTestMode->GetTimerLastEventTime( iCh ) : 
                                      pTestMode->GetTimerMeanEventIntv( iCh );
         if( fTime < 60.0f )
            rStrCell.Format( "%.6f", fTime );
         else                                                              // format time values >= 1 min
         {
            int iHrs = (int) floor( (double) (fTime/3600.0f) );
            fTime -= ((float) iHrs) * 3600.0f;
            int iMin = (int) floor( (double) (fTime/60.0f) );
            fTime -= ((float) iMin) * 60.0f;
            rStrCell.Format( "%d:%02d:%09.6f", iHrs, iMin, fTime );
         }
      }
   }
   else if( c.col > 0 && c.col < 6 )                                       // any other cells in these cols read "N/A" 
      rStrCell = _T("N/A");

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                        // show title tip only if text does not fit 
   return( TRUE );
}


//=== GridEditCB ====================================================================================================== 
//
//    GridEditCB() is the callback invoked to initiate inplace editing of a cell in the timer channel grid, while 
//    GridEndEditCB() is the callback invoked upon termination of the inplace operation.
//
//    Here we only support toggling the state of any timer DO channel in response to a left or right mouse click in the 
//    associated grid cell.  Toggling a timer output is NOT allowed when a loopback test is in progress.  Mouse clicks 
//    in any other cell have no effect, and no inplace operations are permitted.  Thus, GridEndEditCB() is merely a 
//    placeholder function and should never be invoked!
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info. 
//                pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxEventTimerDlg::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxEventTimerDlg* pThis = (CCxEventTimerDlg*)lParam;                    // to access non-static data!
   CCxTestMode* pTestMode =                                                // to access TestMode controller
      (CCxTestMode*) pThis->GetModeCtrl( CCxRuntime::TestMode ); 
   CLiteGrid* pGrid = &(pThis->m_chanGrid);                                // the channel grid
   CCellID c = pEI->cell;                                                  // the cell to be edited

   if( (pGrid->GetSafeHwnd() == NULL) || !pGrid->IsValid( c ) )            // FAIL if grid control is gone, or the 
      return( FALSE );                                                     // specified cell is non-existent 


   if( pEI->iClick != 0 &&                                                 // if invoked by a mouse click, and 
       pThis->IsValidTDOCell( c ) &&                                       // affected cell displays timer DO ch, and 
       pTestMode->GetTimerLoopStatus() == CCxTestMode::TLBS_NOTRUNNING )   // the timer loopback test is NOT running, 
   {                                                                       // then toggle the assoc DO channel state
      pTestMode->ToggleTimerOut( c.row - 1 );
   }
   else                                                                    // no other edit operations are permitted!
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
   }

   return( TRUE );
}

BOOL CALLBACK CCxEventTimerDlg::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   return( TRUE );
}


//=== IsValidTDOCell, IsValidTDICell ==================================================================================== 
//
//    Does this cell in the grid control IDC_TMR_CHANS display info on a valid DO channel? a valid DI channel? 
//
//    ARGS:       c  -- [in] the grid cell to be checked.
//
//    RETURNS:    TRUE if cell does represent a valid DO (DI) channel; else FALSE.
//
BOOL CCxEventTimerDlg::IsValidTDOCell( CCellID c ) 
{
   int n = GetModeCtrl( CCxRuntime::TestMode )->GetNumTDO();
   return( m_chanGrid.IsValid(c) && (c.col == 1) && (c.row > 0) && (c.row <= n) );
}

BOOL CCxEventTimerDlg::IsValidTDICell( CCellID c ) 
{
   int n = GetModeCtrl( CCxRuntime::TestMode )->GetNumTDI();
   return( m_chanGrid.IsValid(c) && (c.col>1) && (c.col<6) && (c.row>0) && (c.row <= n) ); 
}






//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxTestMode
//
//===================================================================================================================== 
//===================================================================================================================== 

//===================================================================================================================== 
// STATIC CLASS MEMBER INITIALIZATION
//===================================================================================================================== 

const double CCxTestMode::REFRESHINTV        = 1000000.0;         // intv betw refreshes of active dlg (in usecs)
const DWORD CCxTestMode::F_AI_RUNNING        = ((DWORD)(1<<0));   // state flags: if set, AI DAQ is running
const DWORD CCxTestMode::F_TMRLOOPON         = ((DWORD)(1<<1));   //    if set, timer loopback test is in progress
const DWORD CCxTestMode::F_TMRLOOPDONE       = ((DWORD)(1<<2));   //    if set, loopback test completed successfully
const DWORD CCxTestMode::F_TMRLOOPFAIL       = ((DWORD)(1<<3));   //    if set, loopback test failed
const DWORD CCxTestMode::F_TMRREPWRITE       = ((DWORD)(1<<4));   //    if set, repetitive DO write function is engaged 


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxTestMode [constructor] =======================================================================================
//
CCxTestMode::CCxTestMode( CCxControlPanel* pPanel ) : CCxModeControl( CCxRuntime::TestMode, pPanel )
{
   m_pAIODlg = NULL;
   m_pTmrDlg = NULL;
   Initialize();
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== InitDlgs [CCxModeControl override] ============================================================================== 
//
//    Install, in the CNTRLX master mode control panel, those dialogs required for operator interactions in the CNTRLX 
//    operational mode represented by this mode controller.
//
//    Two dialogs, both unique to TestMode, are installed here.  They are the only dialogs used in TestMode and they 
//    are not relevant to any other op mode.
//
//    ARGS:       NONE. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise (failed to create one of the required dialogs).
//
BOOL CCxTestMode:: InitDlgs()
{
   ASSERT( m_pCtrlPanel );                                              // verify our ptr to the mode control panel
   ASSERT_KINDOF( CCxControlPanel, m_pCtrlPanel );

   m_pAIODlg = (CCxAnalogIODlg*) m_pCtrlPanel->AddDlg( _T("Analog I/O"), RUNTIME_CLASS(CCxAnalogIODlg) ); 
   if( m_pAIODlg == NULL ) return( FALSE );

   m_pTmrDlg = (CCxEventTimerDlg*) m_pCtrlPanel->AddDlg( _T("Event Timer DIO"), RUNTIME_CLASS(CCxEventTimerDlg) ); 
   if( m_pTmrDlg == NULL ) return( FALSE );

   return( TRUE );
}


//=== Service [CCxModeControl override] =============================================================================== 
//
//    Update runtime state in TestMode.  At each "refresh" interval, we perform the following tasks:
//
//    1) Update AI channel data.  When not paused, CXDRIVER runs a continuous DAQ of the analog inputs in TestMode. 
//    This method "peeks" at the most recently acquired samples by sending the CX_TM_GETAI command to CXDRIVER.
//
//    2) Event timer update.  In TestMode, CXDRIVER continuously monitors "events" on the event timer device's digital 
//    input (DI) channels and tracks various statistics for each channel.  We "peek" at the current input event 
//    statistics by issuing the CX_TM_GETTMRSTATE command.  The current event stats are copied to internal buffers.  
//    In addition, IF the repetitive DO write function is engaged, the timer DO port is cleared to zero and then the 
//    current DO word is written to the port.
//
//    3) Event timer "loopback" test.  CCxTestMode can perform this test on the event timer under the assumption that
//    a "loopback" cable connects the device's digital outputs to its inputs.  At each refresh interval (so the user 
//    can monitor the progress of the test step by step!!), we apply one of N+1 different bit patterns to the output 
//    port.  The first N bit patterns test the channels individually (selected channel = 1; all other outputs are 0), 
//    while the last "all-1s" output pattern verifies that "simultaneous" events are registered correctly.  If at any 
//    point the applied bit pattern on the output is not registered on the input, the test fails and the event timer is 
//    halted until the user resets.  If the test finishes successfully, the timer is also halted until the next reset. 
//
//    4) The active dialog is refreshed, ensuring that the data updated in steps (1) and (2) are posted to the user.
//
//    5) If neither the AI nor the AO device are available, we make sure the "Analog I/O" mode control dialog is 
//    disabled.  If the event timer device is unavailable, we ensure the "event Timer DIO" dialog is disabled.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
VOID CCxTestMode::Service()
{
   int i;

   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ) return;                // should only be called in TestMode

   if( m_tSinceLastRefresh.Get() < REFRESHINTV ) return;                      // it's not time to refresh yet

   ASSERT( m_pCtrlPanel );                                                    // disable dlgs if associated h/w is not 
   m_pCtrlPanel->EnableDlg( m_pAIODlg, IsAIAvailable() || IsAOAvailable() );  // available...
   m_pCtrlPanel->EnableDlg( m_pTmrDlg, IsTimerAvailable() );

   // get AI channel statistics if AI DAQ is not paused
   if(IsAIAvailable() && ((m_dwState & F_AI_RUNNING) != 0))
   {
      DWORD dwCmd = CX_TM_GETAI;
      if(!m_pRuntime->SendCommand(dwCmd, NULL, &(m_fAIData[0]), 0, 0, 0, 3 * CX_AIO_MAXN))
      {
         TRACE1("CX_TM_GETAI failed, returning %d\n", dwCmd);
      }
   }

   if( IsTimerAvailable() )                                                   // update event timer diagnostics...
   {
      int nDI = GetNumTDI();
      int nDO = GetNumTDO();
      int nLoopCh = (nDO < nDI ) ? nDO : nDI;                                 //    just in case #DI != #DO 

      TLBStatus tlbStat = GetTimerLoopStatus();                               //    if loopback test on, clear prev and 
      if( tlbStat == TLBS_RUNNING )                                           //    apply next bit pattern to DO port
      {
         if( m_nLoopTest > 0 ) SetTimerDOPort( 0 );
         if( m_nLoopTest < nLoopCh ) 
            SetTimerDOPort( (DWORD) (1<<m_nLoopTest) );                       //    ...first test each chan by itself  
         else                                                                 //    ...finally, all chan's at once!
         {
            DWORD dwMask = 0;
            for( i = 0; i < nLoopCh; i++ ) dwMask |= (DWORD) (1<<i);
            SetTimerDOPort( dwMask );
         }
         ++m_nLoopTest;
      }

      if( tlbStat < TLBS_DONE )                                               //    get event stats unless a running 
      {                                                                       //    loopback test has halted
         float fBuf[2*CX_TMR_MAXN];
         int iBuf[CX_TMR_MAXN + 1];
         DWORD dwCmd = CX_TM_GETTMRSTATE;
         if(!m_pRuntime->SendCommand(dwCmd, &(iBuf[0]), &(fBuf[0]), 0, 0, nDI + 1, 2 * nDI))
         {
            TRACE1("CX_TM_GETTMRSTATE failed, returning %d\n", dwCmd);
         }
         else
         {
            m_dwDIn = (DWORD) iBuf[nDI];
            for( i = 0; i < nDI; i++ ) 
            {
               m_nEvents[i] = iBuf[i];
               m_fTLastEvent[i] = fBuf[i];
               m_fMeanIEI[i] = fBuf[nDI+i];

            }
         }
      }

      if( tlbStat == TLBS_NOTRUNNING && IsTimerRepetitiveWriteOn() )          //    if repetitive write function is 
      {                                                                       //    engaged, first clear the timer's 
         DWORD dwDOut = GetTDOChanVec();                                      //    DO port to all zeros then write 
         SetTimerDOPort( 0 );                                                 //    the current DO word again.
         SetTimerDOPort( dwDOut );
      }

      if( tlbStat == TLBS_RUNNING )                                           //    if loopback test on, check that 
      {                                                                       //    last bit pattern was recorded...
         if( m_dwDIn != m_dwDOut ) m_dwState |= F_TMRLOOPFAIL;
         else if( m_nLoopTest > nLoopCh ) m_dwState |= F_TMRLOOPDONE;
      }

   }

   Refresh();                                                                 // refresh dlgs installed in ctrl panel
   m_tSinceLastRefresh.Reset();                                               // reset timer for next refresh intv
}


//=== Enter, Exit [CCxModeControl overrides] ========================================================================== 
//
//    Enter() should perform any initializations upon entering the operational mode represented by the mode controller, 
//    while Exit() handles any cleanup activities just prior to exiting the mode.  One task that the mode controller 
//    must handle is to update the subset of dialogs that are accessible on the mode control panel IAW the current op 
//    mode.  It is recommended that the mode controller "hide" all dialogs in Exit(), and "show" only the relevant 
//    dialogs in Enter().
//
//    We enter or leave Test Mode in this "inactive" state:  1) The AI DAQ is paused.  2) All AO are zeroed; test 
//    waveform generation is stopped.  3) Timer DI event stats are reset, and timer DO channels are zeroed.
//
//    ARGS:       NONE. 
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxTestMode::Enter()
{
   ASSERT( m_pRuntime );                                                // MUST be in TestMode!
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ) return( FALSE );

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->ShowDlg( m_pAIODlg, -1 );                              // show the relevant mode ctrl dlgs
   m_pCtrlPanel->ShowDlg( m_pTmrDlg, -1 );
   m_pCtrlPanel->SetActiveDlg( m_pAIODlg );                             // "Analog I/O" dlg is in front initially

   Initialize();                                                        // initialize runtime state 
   SetAOChannel( 0.0f );                                                // make sure CXDRIVER has zero'd AO channels
   m_pRuntime->ResetTraces();                                           // ensure data trace display facility is reset 
   Refresh();                                                           // refresh dialogs to reflect these changes 

   return( TRUE );
}

BOOL CCxTestMode::Exit()
{
   ASSERT( m_pRuntime );                                                // MUST be in TestMode!
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ) return( FALSE );

   m_pRuntime->ResetTraces();                                           // ensure data trace display facility is reset
   if( m_iAOWave != -1 ) RunTestWaveform( -1 );                         // turn off AO test waveform, if it is running
   SetAOChannel( 0.0f );                                                // make sure CXDRIVER has zero'd AO channels 
   if( !IsAIPaused() ) ToggleAISampling();                              // pause AI updates
   ResetTimer();                                                        // reset event timer and associated stats
   Refresh();                                                           // refresh dialogs to reflect these changes

   // before leaving this mode, make sure the grids in the two panel dialogs do NOT have a focus cell
   // 21nov2016(sar): THIS IS IMPORTANT. Without doing this, Maestro would mysteriously crash on File|Exit. Presumably,
   // this is because the GridDispCB() callbacks would be called during shutdown, yet the runtime object is no longer
   // valid. 
   m_pAIODlg->ClearGridFocus();
   m_pTmrDlg->ClearGridFocus();

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->HideDlg( NULL );                                       // hide all mode ctrl dlgs currently visible 

   return( TRUE );
}


//=== ToggleAISampling ================================================================================================ 
//
//    In TestMode, CXDRIVER can execute a continuous DAQ of all AI channels that is stopped/started by this method.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
VOID CCxTestMode::ToggleAISampling()
{
   ASSERT( m_pRuntime );                                             // MUST be in TestMode!
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ) return;

   DWORD dwCmd = (IsAIPaused()) ? CX_TM_RESUMEAI : CX_TM_PAUSEAI;    // send approp cmd to CXDRIVER to pause/resume AI 
   if( m_pRuntime->SendCommand( dwCmd, NULL, NULL, 0, 0, 0, 0 ) )
      m_dwState ^= F_AI_RUNNING;
   else
      TRACE1( "CX_TM_RESUME,PAUSEAI failed, returning %d\n", dwCmd );
}


//=== SetAOChannel, SetAOChannelRaw =================================================================================== 
//
//    Update the voltage driven on one specified AO channel or all channels simultaneously. 
//
//    ARGS:       fVolt -- [in] requested voltage in volts.
//                iDac  -- [in] requested voltage as a raw DAC value.
//                iCh   -- [in] AO channel #; if invalid, all channels set to same voltage [default: -1].
//
//    RETURNS:    requested voltage, or nearest voltage that can be reproduced on the AO device; if op mode is not 
//                TestMode or command fails, returns 0. 
//
float CCxTestMode::SetAOChannel( float fVolt, int iCh /* = -1 */ ) 
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode || !IsAOAvailable() ) // not Test Mode, or no AO dev present!
      return( 0.0f ); 

   DWORD dwCmd = CX_TM_SETAO;                                              // prepare command data...
   BOOL bAll = !IsAOChan( iCh );                                           // if ch# invalid, set all ch's to same val 
   int iChan = (bAll) ? -1 : iCh;
   float fActual = fVolt;
   if( !m_pRuntime->SendCommand( dwCmd, &iChan, &fActual, 1,1,0,1 ) )      // send the command & wait for response 
   {
      TRACE1( "CX_TM_SETAO failed, returning %d\n", dwCmd );
      return( 0.0f );
   }

   int iFirst = (bAll) ? 0 : iCh;                                          // update internal buf of AO chan voltages 
   int iLast = (bAll) ? GetNumAO()-1 : iCh;
   for( int i = iFirst; i <= iLast; i++ ) m_fAO[i] = fActual;

   return( fActual );
}

int CCxTestMode::SetAOChannelRaw( int iDac, int iCh /* = -1 */ )
{
   ASSERT( m_pRuntime );
   float fVolt = m_pRuntime->ConvertRawToVolts( iDac, FALSE );
   fVolt = SetAOChannel( fVolt, iCh );
   return( m_pRuntime->ConvertVoltsToRaw( fVolt, FALSE ) );
}


//=== CalibrateAI ===================================================================================================== 
//
//    If supported by the installed hardware, perform a quick, in-situ calibration of the AI circuitry.  This is 
//    intended only for *internal* calibration that is independent of any signal connections to the AI device. 
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE if calibration is not supported or command failed. 
//
BOOL CCxTestMode::CalibrateAI()
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode || !IsAIAvailable() ) // not Test Mode, or no AI device present
      return( FALSE ); 

   DWORD dwCmd = CX_TM_AICAL;                                              // send command and wait for response
   if( !m_pRuntime->SendCommand( dwCmd, NULL, NULL, 0,0,0,0 ) )
   {
      TRACE1( "CX_TM_AICAL failed, returning %d\n", dwCmd );
      return( FALSE );
   }

   return( TRUE );
}


//=== RunTestWaveform ================================================================================================= 
//
//    CXDRIVER supports the generation of a test waveform (update interval 2ms) on a single selected AO channel when in 
//    TestMode.  Call method to start or stop the test waveform.  Waveform can be run only on ONE channel at a time.
//
//    ARGS:       iCh   -- [in] AO channel on which to generate test waveform; if invalid, stop waveform generation.
//
//    RETURNS:    TRUE if successful; FALSE if command failed. 
//
BOOL CCxTestMode::RunTestWaveform( int iCh )
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode || !IsAOAvailable() ) //  not Test Mode, or no AO device present! 
      return( FALSE ); 

   int iNewCh = (IsAOChan( iCh )) ? iCh : -1;                              // if ch# invalid, waveform stopped 
   if( iNewCh == m_iAOWave ) return( TRUE );                               // nothing to do in this case!

   DWORD dwCmd = CX_TM_AOWAVE;                                             // send the command & wait for response...
   if( !m_pRuntime->SendCommand( dwCmd, &iNewCh, NULL, 1,0,0,0 ) ) 
   {
      TRACE1( "CX_TM_AOWAVE failed, returning %d\n", dwCmd );
      return( FALSE );
   }

   if( m_iAOWave != -1 )                                                   // if waveform was running on another chan, 
      m_fAO[m_iAOWave] = 0.0f;                                             // that channel is reset to zero volts!
     
   m_iAOWave = iNewCh;                                                     // remember AO ch# on which waveform is run 
   return( TRUE );
}


/**
 Get the current raw DAC value for the voltage being driven on the specified analog output channel. Since Maestro may
 use 12-bit or 16-bit analog devices, the method defers to CCxRuntime to convert between volts and raw DAC values.
 @param iCh The AO channel number.
 @return The raw DAC value, or 0 if channel # is invalid.
*/
int CCxTestMode::GetAOChannelRaw(int iCh)
{
   if(!IsAOChan(iCh)) return(0);

   ASSERT(m_pRuntime);
   return(m_pRuntime->ConvertVoltsToRaw(m_fAO[iCh], FALSE));
}

/**
 Get the request statistic for a specified analog input channel. In TestMode, Maestro keeps track of the last voltage
 sampled, the running average, and the standard deviation in the mean measured on each AI channel.
 @param iCh The AI channel number.
 @param which Enum identifying which statistic is requested.
 @return The requested value, in volts. Returns 0 if channel # is invalid
*/
float CCxTestMode::GetAIChannel(int iCh, AIDatum which)
{
   if(!IsAIChan(iCh)) return(0.0f);
   int idx = iCh;
   if(which == AID_AVG) idx += GetNumAI();
   else if(which == AID_STD) idx += 2*GetNumAI();
   
   return(m_fAIData[idx]);
}

/** 
 Get the request statistic for a specified analog input channel as a raw ADC value. Since Maestro may use 12- or 16-bit 
 analog devices, the method defers to CCxRuntime to convert between volts and raw DAC values.
 @param iCh The AI channel number.
 @param which Enum identifying which statistic is requested.
 @return The requested value, as a raw ADC value. Returns 0 if channel # is invalid
*/
int CCxTestMode::GetAIChannelRaw(int iCh, AIDatum which)
{
   if( !IsAIChan(iCh) ) return( 0 );

   ASSERT( m_pRuntime );
   return(m_pRuntime->ConvertVoltsToRaw(GetAIChannel(iCh, which), TRUE));
}


//=== ResetTimer ====================================================================================================== 
//
//    Reset the event timer device:  zero the digital output port, clear all tracked "input event" statistics.  After 
//    reset, the event timer is restarted to register new input events.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTestMode::ResetTimer()
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode()!=CCxRuntime::TestMode || !IsTimerAvailable() )   // not Test Mode, or no timer dev avail! 
      return;


   DWORD dwCmd = CX_TM_RESETTMR;                                              // send cmd to CXDRIVER; if it succeeds, 
   if( m_pRuntime->SendCommand( dwCmd, NULL, NULL, 0, 0, 0, 0 ) )             // reset input stats and timer DO vector. 
   {
      for( int i = 0; i < CX_TMR_MAXN; i++ ) 
      {
         m_nEvents[i] = 0;
         m_fTLastEvent[i] = 0.0f;
         m_fMeanIEI[i] = 0;
      }
      m_dwDOut = 0;
      m_dwDIn = 0;
      m_dwState &= ~(F_TMRLOOPON|F_TMRLOOPDONE|F_TMRLOOPFAIL|F_TMRREPWRITE);
   }
   else
      TRACE1( "CX_TM_RESETTMR failed, returning %d\n", dwCmd );
}


//=== ToggleTimerOut ================================================================================================== 
//
//    Toggle the state (1 or 0) of the specified digital output channel on the event timer device.  If channel # is 
//    invalid, this method does nothing.  In addition, Service() takes over the digital outputs during the loopback 
//    test, so this method has no effect during that test.
//
//    ARGS:       iCh   -- [in] channel #. 
//
//    RETURNS:    NONE.
//
VOID CCxTestMode::ToggleTimerOut( int iCh )
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ||              // we're not in Test Mode, or invalid ch #, 
       (!IsTDOChan( iCh )) || ((m_dwState & F_TMRLOOPON) != 0) )     // or loopback test in progress 
      return;

   SetTimerDOPort( m_dwDOut ^ ((DWORD) (1<<iCh)) );
}


//=== GetTimerLoopStatus, StartTimerLoop ============================================================================== 
//
//    When a "loopback" cable is installed connecting the event timer's digital outputs to its digital inputs, TestMode  
//    can execute a "loopback test" to verify the event timer's operation.  StartTimerLoop() is called to start a 
//    loopback test.  GetTimerLoopStatus() returns an enumerated constant indicating the current state of the loopback 
//    test facility:
//       TLBS_NOTRUNNING   ==> the loopback test is not on. 
//       TLBS_RUNNING      ==> loopback test in progress.
//       TLBS_DONE         ==> loopback test completed successfully.
//       TLBS_FAILED       ==> loopback test failed.
//    Once a loopback test starts, the timer must be reset to resume normal diagnostic monitoring -- even if the test 
//    completes normally.
//
//    See Service() for more details.
//
//    ARGS:       NONE. 
//
//    RETURNS:    see above.
//
CCxTestMode::TLBStatus CCxTestMode::GetTimerLoopStatus() 
{ 
   TLBStatus tlbs;
   if( (m_dwState & F_TMRLOOPON) != F_TMRLOOPON )
      tlbs = TLBS_NOTRUNNING;
   else if( (m_dwState & F_TMRLOOPDONE) == F_TMRLOOPDONE )
      tlbs = TLBS_DONE;
   else if( (m_dwState & F_TMRLOOPFAIL) == F_TMRLOOPFAIL )
      tlbs = TLBS_FAILED;
   else
      tlbs = TLBS_RUNNING;
   return( tlbs );
} 

VOID CCxTestMode::StartTimerLoop()
{
   ASSERT( m_pRuntime );
   if( m_pRuntime->GetMode() != CCxRuntime::TestMode ||                    // not in Test Mode, or no event timer dev, 
       (!IsTimerAvailable()) || ((m_dwState & F_TMRLOOPON) != 0) )         // or loopback test already started
      return; 

   ResetTimer();                                                           // cmd CXDRIVER to reset the event timer 
   m_dwState |= F_TMRLOOPON;                                               // loopback test has begun
   m_nLoopTest = 0;                                                        // counts # of bit patterns applied
}


//=== IsTimerRepetitiveWriteOn, ToggleTimerRepetitiveWrite ============================================================ 
//
//    CCxTestMode supports rewriting the current 16bit DO word to the event timer's DO port each time the refresh 
//    interval expires.  This "repetitive write" function may be useful to test the "latched digital devices" that 
//    reside on the timer's DO bus in a typical Maestro lab setup and are selectively addressed by the uppermost nibble 
//    (DO<15..12>) of the DO word.
//
//    ToggleTimerRepetitiveWrite() toggles the on/off state of this test functionality, and IsTimerRepetitiveWriteOn() 
//    returns TRUE only when the function is engaged.  The function cannot be turned on during a loopback test.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
BOOL CCxTestMode::IsTimerRepetitiveWriteOn()
{
   return( BOOL((m_dwState & F_TMRREPWRITE) != 0) );
}

VOID CCxTestMode::ToggleTimerRepetitiveWrite()
{
   ASSERT( m_pRuntime );

   if( IsTimerRepetitiveWriteOn() ) 
      m_dwState &= ~F_TMRREPWRITE;
   else if( (m_pRuntime->GetMode() == CCxRuntime::TestMode) && IsTimerAvailable() &&
            (GetTimerLoopStatus() == TLBS_NOTRUNNING) )
      m_dwState |= F_TMRREPWRITE;
}



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== Initialize ====================================================================================================== 
//
//    Initialize TestMode's runtime state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTestMode::Initialize()
{
   m_dwState = 0;
   m_tSinceLastRefresh.Reset();

   for( int i = 0; i < CX_AIO_MAXN; i++ ) 
   {
      m_fAO[i] = 0.0f;
      m_fAIData[i] = 0.0f;
      m_fAIData[CX_AIO_MAXN + i] = 0.0f;
      m_fAIData[2*CX_AIO_MAXN + i] = 0.0f;
   }

   m_iAOWave = -1;

   for( int i = 0; i < CX_TMR_MAXN; i++ ) 
   {
      m_nEvents[i] = 0;
      m_fTLastEvent[i] = 0.0f;
      m_fMeanIEI[i] = 0;
   }
   m_dwDOut = 0;
   m_dwDIn = 0;
}


//=== SetTimerDOPort ================================================================================================== 
//
//    This method applies a new bit pattern to the event timer's digital output port.
//
//    ARGS:       dwVec -- [in] the new DO bit pattern, where bitN indicates logic state of DO channel N. 
//
//    RETURNS:    TRUE if op was successful; FALSE otherwise.
//
BOOL CCxTestMode::SetTimerDOPort( DWORD dwVec )
{
   int iCopy = (int) dwVec;
   DWORD dwCmd = CX_TM_SETTMRDO;                                        // send cmd to CXDRIVER; updates entire port
   BOOL bOk = m_pRuntime->SendCommand( dwCmd, &iCopy, NULL, 1,0,0,0 );
   if( !bOk )
      TRACE1( "CX_TM_SETTMRDO failed, returning %d\n", dwCmd );
   else
      m_dwDOut = dwVec;                                                 // success! update our internal copy of DO port 
   return( bOk );
}


