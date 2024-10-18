//=====================================================================================================================
//
// cxtrialmode.cpp : Implementation of the TrialMode controller CCxTrialMode and seveal TrialMode-specific control 
//                   panel dialogs.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Each "operational mode" has a "mode controller" which controls the runtime behavior of MAESTRO and CXDRIVER in that
// mode.  CCxTrialMode is the mode controller for MAESTRO's "Trial Mode", in which a set of defined experimental trials
// are presented to the subject in a manner proscribed by the operator via interactions with various dialogs housed in
// the master mode control panel, CCxControlPanel. In this file we define CCxTrialMode as well as 3 TrialMode-specific 
// mode contol dialogs: CCxTrialProtoDlg, CCxTrialParmsDlg, and CCxTrialStatsDlg. Another TrialMode-specific dialog, 
// CCxRPDistroDlg, is defined in a separate class file.
//
// ==> The MAESTRO "Mode Control" Framework.
// The master mode control panel CCxControlPanel is implemented as a dockable dialog bar containing one or more
// tabbed dialogs.  All dialogs that affect runtime state in any MAESTRO operational mode are installed in this
// container, although only a subset of them will be accessible in any given mode.  In addition to its role as a
// dialog container, CCxControlPanel constructs a "mode controller" object for each op mode, and it handles mode
// switches by invoking appropriate methods on the relevant mode controllers.  Each mode controller, interacting with
// the operator via some subset of the mode control panel dialogs, encapsulates the runtime behavior of MAESTRO and
// CXDRIVER in a particular operational mode.  To communicate with CXDRIVER, it must invoke methods on the MAESTRO
// runtime interface, CCxRuntime.  By design, the mode controller should insulate the mode control dialogs from
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
// ==> CCxTrialMode.
// CCxTrialMode is the mode controller for TrialMode operations.  It installs the TrialMode-specific dialogs in the
// master mode control panel during GUI creation at application startup (see InitDlgs()), hides all TrialMode control
// dialogs upon exiting TrialMode (see Exit()), and reveals these same dialogs upon entering TrialMode (see Enter()).
// The dialogs accessible in the mode control panel during TrialMode (see below) manage various GUI widgets/controls,
// and these dialogs call CCxTrialMode methods to carry out various operations, including all interactions with
// CXDRIVER via the MAESTRO runtime interface, CCxRuntime.
//
// All of the hard work in TrialMode -- sequencing trials and transmitting trial & target info to CXDRIVER -- falls to
// CCxRuntime.  CCxTrialMode merely relays the trial sequencer control parameters to CCxRuntime (encapsulated by the
// helper class CCxTrialSequencer), starts the trial sequence, updates selected runtime status information after a
// trial ends, and initiates each subsequent trial in the sequence (or stops the sequence).
//
// Currently, we prevent the user from altering any control parameters while trials are running.  Thus, the dialog
// controls reflect the parameter values actually in effect, and the user must stop running in order to change any of
// the parameters.  Our mechanism for doing this:  CCxTrialMode refreshes TrialMode dialogs whenever it stops or starts
// running trials.  In turn, the dialogs' Refresh() override disables selected controls when trials are running.
//
// ==> CCxTrialProtoDlg, the "Protocol" dialog.
// This TrialMode-speciifc dialog page houses the controls which define the particular experimental protocol to be
// executed by the trial sequencer, as well as the all-important "START/STOP" pushbutton that initiates trial
// presentations:
//
//    IDC_TRIAL_GO [pushb]: Starts & stops trial sequencing.  Disabled only when trial sequencer is in "soft-stop",
//          waiting to stop at the end of the current trial.  See CCxTrialMode::Go(), Halt().
//    IDC_TRIAL_PAUSE [pushb]: Pauses & resumes a trial sequence that is already started.  When the "Pause" button is
//          pressed, the current trial finishes and then the Trial Mode controller enters the "paused" state, at which
//          point the button's label is "Resume".  Disabled when trials are not being sequenced, and between the time
//          the "Pause" button is pressed and the current trial finishes.
//    IDC_TRIAL_ABORT [pushb]: Abort trial sequencing immediately (do not wait for current trial to end -- that trial's
//          data is discarded).  See CCxTrialMode::Abort().
//    IDC_TRIAL_DELAY [numedit]: Specifies an intertrial delay in milliseconds.  Allowed range is 0..2000 ms.
//    IDC_TRIAL_IGT [numedit]: Specifies a threshold trial time in milliseconds.  If a trial aborts prior to this time,
//          it is interpreted as an "ignored" trial -- the # of trial "attempts" is NOT incremented.  Allowed range is
//          0..9999ms.
//
//    IDC_TRIAL_SET [custom combo]: Selects the trial set containing the trials and/or trial subsets that define the
//          experimental protocol to be performed. Contains the names of all trial sets currently defined in the open 
//          CCxDoc.  Implemented by CCxObjCombo, which see.
//    IDC_TRIAL_CURR [custom combo]: Lists all trials in the currently selected trial set.  During sequencing, the
//          selection is updated to indicate to the user which trial is running. When editing the protocol, the user
//          can use this combo box to select which trial to run repeatedly in the "Current trial" sequencer modes.
//          Again, implemented by CCxObjCombo.
//
//    IDC_TRIAL_SEQ_SUBSETS [std combo]: Selects the sequencing mode for trial subsets within the selected trial set. 
//          If the set lacks any non-empty subsets, then subset sequencing is unavailable, and this combo box is 
//          disabled. Otherwise, it offers the choices in CCxTrialSequencer::strSubsetSeqModes[].
//    IDC_TRIAL_SEQ_TRIALS [std como]: Selects the sequencing mode for individual trials. If subset sequencing is
//          engaged, then this combo box chooses how the trials within each subset are sequenced. If subset sequencing
//          is disabled or turned off, then all of the trials in the set -- including those ensconced in subsets -- are
//          treated as a single group of trials, and the combo box selects how those trials are sequenced. Note that 
//          some trial sequencing modes are not allowed when subset sequencing is engaged. If subset sequencing is
//          currently enabled and the sequencer does not support the trial sequencing mode selecteds, the "Start" 
//          button will be disabled. See CCxTrialSequencer::IsValidSeqMode().
//
//    IDC_TRIAL_STOP_MODE [std combo]: This combo box selects the auto-stop mode: off (manual stop), stop after a 
//          specified number of trials have been completed, or stop after a number of trial blocks are completed.
//
//    IDC_TRIAL_STOP_COUNT [numedit]: This numeric edit control sets the auto-stop count. It will be disabled in the
//          auto-stop feature is turned off.
//
//    IDC_TRIAL_PRES [pushb]: A read-only numeric edit ctrl [IDC_TRIAL_PRES_RO] serves as a counter tracking the total
//          # of trials that have been presented since the last reset.  Clicking this PB at any time will reset the
//          counter.
//    IDC_TRIAL_BLK [pushb]: A read-only numeric edit ctrl [IDC_TRIAL_BLK_RO] serves as a counter tracking the # of
//          trial blocks that have been presented since the last reset.  Clicking this PB at any time will reset the
//          counter.  A trial block is complete when all the trials in a set have been presented the # of times
//          indicated by their "weight" attribute.  Trial blocks are counted only in the "Ordered" and "Randomized"
//          sequencer modes.
//    IDC_TRIAL_ATT [pushb]: A read-only numeric edit ctrl [IDC_TRIAL_ATT_RO] serves as a counter tracking the # of
//          trials attempted since the last reset.  The # of attempted trials is the sum of those trials successfully
//          completed and those that aborted but NOT "ignored".  By convention, a trial was ignored if it aborted
//          prior to the "ignore threshold" time (in IDC_TRIAL_IGT) because the animal lost fixation.  Clicking this
//          PB at any time will reset the counter.
//    IDC_TRIAL_REW [pushb]: A read-only numeric edit ctrl [IDC_TRIAL_REW_RO] serves as a counter tracking the # of
//          trials that were successfully completed (ie, fixation reqmts met) since the last reset.  Clicking this PB
//          at any time will reset the counter.
//
//    IDC_TRIAL_RECDATA [chkbox]: If checked, trial data (as defined by each trial's "channel config" object) are saved
//          to the file path in the accompanying edit ctrl, IDC_TRIAL_DATAPATH.
//    IDC_TRIAL_DATAPATH [custom edit]: This read-only custom edit ctrl displays the full pathname where the next trial
//          data file will be stored.  It includes a "browse" button which invokes a dialog that lets the user choose a
//          different path.  A standard edit control is subclassed to CCxFileEdit to get the browsing functionality and
//          to enforce CNTRLX-specific constraints on the form of CNTRLX data file names.  See CCxFileEdit for details.
//    IDC_TRIAL_RECSPKS [chkbox]: If checked, a high-res spike trace is also recorded during the trial and saved to the
//          trial data file.  This check box should be disabled when trial data recording (IDC_TRIAL_RECDATA) is off.
//
// The dialog houses several controls whose contents are updated as trials are presented.  To that end, CCxTrialMode
// invokes a number of methods on CCxTrialProtoDlg:
//    IncrementAttempts(), IncrementSuccesses(), IncrementBlocks() ==> To increment the three counters reflecting the
//          # of trials attempted, #trials successful, and # of trial blocks completed.
//    IncrementNextDataFile() ==> To increment the extension on the trial data filename.  Upon saving data to a file,
//          we must increment extension to create a new (and supposedly nonexistent) file for the next trial's data.
//    SetCurrentTrial() ==> This updates the current trial selection to reflect the name of the trial now running.
//
// ==> CCxTrialParmsDlg, the "Other Params" dialog.
// This TrialMode-specific dialog page houses a number of control parameters for the trial sequencer:  several params
// that control the evolution of a staircase sequence, one parameter for the chained sequence mode, and some "global 
// override" params that the trial sequencer uses to modify or replace selected parameters in an individual trial. 
// Another widget displays/edits the length of the sliding window average of eye position used to smooth the effects of
// velocity stabilization in Trial Mode. The dialog restricts most parameter values to allowed ranges, provides a 
// one-step means to reset all parameters to default values, and provides accessor methods for retrieving individual 
// parameters. In addition, its Refresh() override will disable user input to the sequencer parameter controls whenever
// a trial sequence is running -- since these params cannot be changed while a sequence is in progress. All other 
// parameters in the dialog are enabled if the sequence is paused (sequencer running, but no trial in progress) or 
// stopped. The control parameters currently managed on this dialog page are summarized below:
//
//    staircase sequence control variables:
//       IDC_TRIAL_STAIR_STREN [numeric edit]: starting strength
//       IDC_TRIAL_STAIR_UP [spin w/buddy]: # of consecutive incorrect responses that trigger a step "up" the staircase
//       IDC_TRIAL_STAIR_DN [spin w/buddy]: # of consecutive correct responses triggering a step "down"
//       IDC_TRIAL_STAIR_REV [spin w/buddy]: # of reversals in staircase direc to halt the sequence (0 = manual stop)
//       IDC_TRIAL_IRREL [numeric edit]: % "irrelevant" trials presented
//    global "overrides":
//       IDC_TRIAL_POS_SCALE [numeric edit]: target position scale factor (no units)
//       IDC_TRIAL_POS_ROT [numeric edit]: target position rotation angle (deg)
//       IDC_TRIAL_VEL_SCALE [numeric edit]: target velocity scale factor (no units)
//       IDC_TRIAL_VEL_ROT [numeric edit]: target velocity rotation angle (deg)
//       IDC_TRIAL_CH_ENA [check box]: ena/disables trial channel configuration override
//       IDC_TRIAL_CHCFG [custom combo]: selects channel configuration that will be used for all trials
//       IDC_TRIAL_START_H [numeric edit]: H component of initial pos of all targets at start of trial (deg)
//       IDC_TRIAL_START_V [numeric edit]: V component of initial pos of all targets at start of trial (deg)
//    other:
//       IDC_TRIAL_VSTABWIN [numeric edit]: length of sliding window average of eye pos to smooth VStab effects (ms)
//       IDC_TRIAL_CHAINLEN [multiline edit]: User can enter a comma-delimited list of integers indicating what 
//           trial chain lengths to include in a "chained" sequence. Integer values should lie in [1..255], but they
//           can be repeated to increase the frequency of a given chain length WRT other chains. Invalid integers or
//           non-integers are simply ignored.
//
// Most of these parameters have a min-max range and a default value. CCxTrialParmsDlg is responsible for validating
// all user input.  When the trial sequencer starts, CCxTrialMode uses the accessor methods to obtain the current
// parameter values.
//
// NOTE: As of Maestro 4.1.1, VStab window length is persisted in the application settings object, CCxSettings. The
// persisted value is updated whenever the user changes it on this dialog.
//
// ==> CCxTrialStatsDlg, the "Statistics" dialog.
// This TrialMode-specific dialog page houses a read-only grid control that displays a very simple statistics summary
// for the currently running (or last finished) trial sequence.  The table is laid out one way when the sequencer mode
// is "Chained" (w/ or w/o fixation), and another way for all other modes. A static label above the grid gives the name
// of the trial set from which the trials were drawn for the latest trial sequence.
//
// ==> CCxRPDistroDlg, the "R/P Distro" dialog.
// This TrialMode-specific dialog page was introduced in Maestro v1.4 in support of a special reward/penalty protocol
// based upon the subject's response relative to a previously compiled response distribution.  See CCxRPDistroDlg
// class file.
//
// ==> CCxFixRewDlg, the "Fix/Reward" dialog.
// This dialog page provides a window into the CNTRLX document's fixation and reward settings, a subset of the
// application level settings encapsulated by the CCxSettings object. The CCxFixRewDlg page is designed for use in any
// MAESTRO op mode, not just TrialMode.  For details, see the files CXFIXREWDLG.*.  CCxTrialMode will install this
// dialog in the mode control panel ONLY if it is not already there (see InitDlgs()).
//
// ==> CCxVideoDspDlg, the "RMVideo Display" dialog.
// This dialog page is a window into the RMVideo display parameters that are a subset of Maestro's application
// settings, also encapsulated by the CCxSettings object within the currently open Maestro doc. Like CCxFixRewDlgs, the
// CCxVideoDspDlg page may be used in more than one Maestro op mode. For details, see files CXVIDEODSPDLG.*. Again,
// CCxTrialMode will install this dialog in the mode control panel ONLY if it is not already there.
//
// ==> CCxEyelinkDlg, the "Eyelink" dialog.
// Contains controls for connecting/disconnecting from the Eyelink 1000+ eye tracker, adjusting calibration parameters.
//
//
// REVISION HISTORY:
// 26oct2001-- Began development.
// 16nov2001-- Development continues.  CCxTrialSeqParms and CCxTrialSeqProtocol essentially ready, but CCxTrialPanel
//             is an empty shell....
// 29nov2001-- Began working on implementation of CCxTrialPanel.
// 30nov2001-- Got rid of IDC_TRIAL_FBPRE on the "Protocol" dialog (IDD_TRIALCP1).  Decided that user would not have a
//             choice regarding the preload of framebuffer video targets.  Preloading occurs while preparing to start
//             trial sequencing.
//          -- Done.  Most of the hard work re: trial sequencing has been put upon CCxRuntime.  All CCxTrialPanel does
//             in its Service() routine is to detect when a trial completes, update its dialogs accordingly, and then
//             start the next trial or halt the sequence.
// 02jan2002-- Shifted burden of trial sequencing from CCxRuntime to CCxTrialPanel.  Still, most of the hard work is
//             handled by the helper class CCxTrialSequencer.
// 10jan2002-- Modified StartTrial() IAW changes in CCxRuntime & CCxTrialSequencer re:  Trial targets now identified by
//             their position in the "trial target map" [0..MAX_TRIALTARGS].  The map, in turn, contains the pos of the
//             target's actual definition in the current tgt definition list in CXIPC. See CCxRuntime::AccessTrialInfo
//             and CCxTrialSequencer::GetTrialInfo.
// 06feb2002-- The tasks of loading the target list and starting a trial are now handled by CCxRuntime methods, which
//             are passed pointers to the trial sequencer object.  Revised CCxTrialPanel accordingly.
// 11feb2002-- Added "Video Display" dialog page to CCxTrialPanel.
// 18apr2002-- Mods to reflect fact that spike trace data will be saved in the trial data file itself, rather than in a
//             separate file.
// 08oct2002-- CFileEditCtrl in CCxTrialSeqProtocol replaced by more self-contained CCxFileEdit.
// 17oct2002-- Added "Fix/Reward" dialog page (CCxFixRewDlg) to CCxTrialPanel.
// 24jan2003-- Minor mod to CCxTrialSeqProtocol so that, whenever the SetCurrentTrial() is called to change the current
//             trial, a display hint is sent so that trial's defn is loaded onto the relevant form.  OnUpdate() also
//             modified so that it does not respond to updates initiated by the dialog itself!
// 04apr2003-- MAJOR redesign of the CNTRLX "mode control" framework.  There is now only a single mode control panel,
//             CCxControlPanel.  CCxTrialPanel is replaced by the TrialMode controller object CCxTrialMode, which is
//             derived from the abstract base class CCxModeControl.  Mode control dialogs are still derived from the
//             abstract class CCxControlPanelDlg, but they interact with the "current" mode controller object rather
//             than a derivative of CCxControlPanel.  See also CCxControlPanel, CCxControlPanelDlg, and CCxModeControl.
//          -- Also renamed dlgs:  CCxTrialSeqProtocol --> CCxTrialProtoDlg; CCxTrialSeqParms --> CCxTrialParmsDlg.
// 07may2003-- The typical intertrial delay is so short that the subject may be overwhelmed.  Thus, we've introduced a
//             numeric edit control on CCxTrialProtoDlg that allows the user to specify an added intertrial delay of
//             0 to 2000ms.  The delay is implemented by CCxTrialMode.  The delay is on top of any programmatic delay
//             (eg, the time it takes to save the trial data file).
// 13feb2004-- Introduced the ability to "pause/resume" trial sequencing.  Allows the user to attend to the animal,
//             give it a rest, change certain settings, etc. without resetting the trial sequencer.
// 10mar2004-- Added support for setting a global starting position for all targets participating in a trial.  This
//             parameter is encapsulated by the CCxTrialSequencer member of CCxTrialMode, and is exposed to the user
//             via widgets on the CCxTrialParmsDlg (IDC_TRIAL_START_H, IDC_TRIAL_START_V).  The widgets are enabled
//             whenever there's no trial sequence in progress or the current sequence is paused.  Some of the other
//             widgets on CCxTrialParmsDlg should behave similarly, but that will require more changes to
//             CCxTrialSequencer....
// 15mar2004-- The target velocity and position rotation angles on CCxTrialParmsDlg are now restricted to the unit
//             circle [0..360deg) instead of [-180..180].  Negative values are still permitted, but they are remapped
//             to the unit circle when entry is validated.
// 05apr2004-- Modified CCxTrialMode and CCxTrialParmsDlg so that some additional widgets on the "Other Params" dlg
//             are enabled when sequencer is off OR paused -- see entry dtd 10mar2004.  Now, any widget reflecting a
//             parameter that does NOT modify the sequencer's state is enabled when the sequencer is paused.
// 07apr2004-- Introduced "Auto-Stop" feature in CCxTrialMode.  User can choose to autostop after a specified number of
//             trials or trial blocks are completed, or disable the autostop feature.  Required controls are placed on
//             CCxTrialProtoDlg.  CCxTrialSequencer implements the feature.
// 07jul2004-- Introduced a feature to detect trials that are "ignored" by the subject.  If the subject loses fixation
//             and the trial aborts before the user-specified "ignore threshold time", then it is assumed that the
//             trial was ignored rather than attempted.  Now, the IDC_TRIAL_ATT_RO field displays the #trials actually
//             attempted, while the *new* IDC_TRIAL_PRES_RO field displays the total #trials presented.  If the ignore
//             threshold is 0, then the feature is effectively disabled:  no trials will be considered "ignored".
// 17sep2004-- Added CCxTrialStatsDlg, a rudimentary statistics summary that helps researcher evaluate whether the
//             subject is having inordinate difficulty completing any one particular trial or trials in a set.
// 22sep2004-- Trial-mode data directory (appearing in file edit ctrl w/in CCxTrialProtoDlg) is set IAW a registry
//             setting at startup, then saved in the registry before GUI is destroyed.
// 10mar2005-- CCxTrialMode::Service modified so that CCxTrialStatsDlg displays #attempts (ie, NOT ignored) rather than
//             #presentations of each trial.
// 14jun2005-- Whenever data file is on a mapped network drive, CCxTrialMode instructs CXDRIVER to write the data to a
//             "shadow file" on the local disk.  When trial is done, the shadow file is moved to the remote drive.
//             This change was required because RTX no longer supports file I/O to a remote drive as of version 5.1.1.
//             See CCntrlxApp::Get/MoveShadowFile().
// 05dec2005-- Added support for distribution-based reward/penalty protocol:  new CCxRPDistroDlg page, pass response
//             measure and trial result to the CCxRPDistro object exposed by CCxTrial...
// 28apr2010-- Modified Go() to use new version of CCxRuntime::SetTransform() which now also stores the global target
//             starting position offset (H & V) in IPC shared memory. Also, Resume() calls CCxRuntime::SetTransform()
//             as well -- which it should have all along!!
// 14mar2011-- Slight change in logic which determines whether or not #Completed statistic is incremented after a
//             trial. Now relies on CCxTrialSequencer::WasTrialCompleted().
// 11may2011-- Added widget IDC_TRIAL_VSTABWIN to display/edit length of sliding window average (in ms) used to smooth
//             the effects of VStab during a trial.
// 03dec2014-- Began mods in support of two levels of sequencing: by trial subset, and by individual trial. Subset
// sequencing can be turned off, in which case all trials in the set -- including any ensconced in subsets -- are 
// treated as a single group of trials.
// 31aug2015-- Added Eyelink dialog page, CCxEyelinkDlg.
// 21nov2016-- Increased max inter-trial delay from 2000 to 9999ms.
// 14aug2019-- Updated CCxTrialParmsDlg to update application settings object CCxSettings whenever user changes the 
// value of the VStab sliding window length. As of Maestro 4.1.1, that parameter is persisted as an app setting.
// 26sep2024-- Tab name for CCxVideoDspDlg is now "RMVideo Display". A/o V5.0, the XYScope platform -- unsupported 
// since V4.0 -- has been removed from Maestro.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"                            // runtime math stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX

#include "cxdoc.h"                           // CCxDoc -- Maestro document class
#include "cxmainframe.h"                     // CCxMainFrame -- Maestro main frame window
#include "cxtrial.h"                         // CCxTrial -- Maestro trial object
#include "cxrpdistro.h"                      // CCxRPDistro -- holds runtime results for a "R/P Distro" trial
#include "cxfixrewdlg.h"                     // CCxFixRewDlg -- the "Fix/Reward" dialog page
#include "cxvideodspdlg.h"                   // CCxVideoDspDlg -- the "Video Display" dialog page
#include "cxeyelinkdlg.h"                    // CCxEyelinkDlg -- the "Eyelink" dialog page
#include "cxcontrolpanel.h"                  // CCxControlPanel -- the Maestro master mode control panel
#include "cxspikehistbar.h"                  // CCxSpikeHistBar -- the spike histogram display panel
#include "cxruntime.h"                       // CCxRuntime -- the Maestro/CXDRIVER runtime interface

#include "cxtrialmode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialProtoDlg
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxTrialProtoDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxTrialProtoDlg, CCxControlPanelDlg )
   ON_WM_DESTROY()
   ON_NOTIFY( FEC_NM_PREBROWSE, IDC_TRIAL_DATAPATH, OnPreBrowse )
   ON_CONTROL_RANGE( CBN_SELCHANGE, IDC_TRIAL_SET, IDC_TRIAL_STOP_MODE, OnChange )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_TRIAL_DELAY, IDC_TRIAL_IGT, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_TRIAL_GO, IDC_TRIAL_PRES, OnChange )
END_MESSAGE_MAP()


//=====================================================================================================================
// STATIC MEMBER INITIALIZATION
//=====================================================================================================================

const int CCxTrialProtoDlg::MIN_TRIALDELAY      = 0;           // allowed range for the inter-trial delay (ms)
const int CCxTrialProtoDlg::MAX_TRIALDELAY      = 9999;
const int CCxTrialProtoDlg::MIN_IGNORETIME      = 0;           // allowed range for ignore threshold time (ms)
const int CCxTrialProtoDlg::MAX_IGNORETIME      = 9999;
const int CCxTrialProtoDlg::MIN_AUTOSTOPCNT     = 1;           // allowed range for autostop trial or block count
const int CCxTrialProtoDlg::MAX_AUTOSTOPCNT     = 9999;


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnDestroy (CWnd override) =======================================================================================
//
//    ON_WM_DESTROY handler.
//
//    Prior to destroying the dialog, we store the Trial-mode data directory (from the file edit control) in the
//    current user's registry profile.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxTrialProtoDlg::OnDestroy()
{
   CString strDir;
   m_fecDataPath.GetCurrentDirectory( strDir );
   ((CCntrlxApp*)AfxGetApp())->SetMRUTrialDataDirectory( strDir );
   CCxControlPanelDlg::OnDestroy();
}


//=== OnPreBrowse =====================================================================================================
//
//    ON_NOTIFY handler for the custom edit control (CCxFileEdit) that displays/selects the file system path for
//    storing the next trial data file (IDC_TRIAL_DATAPATH).
//
//    OnPreBrowse() [FEC_NM_PREBROWSE notification code] is called just after the user clicks on the button that
//    invokes the browsing dialog.  This gives us a chance to prevent browsing entirely, and to further tailor the
//    appearance of the dialog, if desired.
//
//    ARGS:       pNMH  -- [in] ptr to CFileEditCtrl's FEC_NOTIFY struct, cast as a generic NMHDR*.
//                pRes  -- [out] return value.  for FEC_NM_PREBROWSE, set nonzero value to prevent browsing.
//
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
void CCxTrialProtoDlg::OnPreBrowse( NMHDR *pNMH, LRESULT *pRes )
{
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl( CCxRuntime::TrialMode );
   *pRes = (LRESULT) pTrialMode->IsSeqRunning();                           // can't browse while trial seq is running!
}

/**
 Respond to various change notifications from selected widgets on the dialog:
 1) CBN_SELCHANGE: When the user changes the selection is certain combo boxes on the dialog (contiguous range 
 IDC_TRIAL_SET .. IDC_TRIAL_STOP_MODE), the contents or enable state of other widgets may be affected:
    IDC_TRIAL_SET => Whenever the user selects a different trial set, the contents of the "current trial" combo box 
       must be reloaded to list the trials in that set. Also, the enable state of the trial subset sequencing mode
       combo is updated -- subset sequencing is disabled if the set lacks any non-empty subsets.
    IDC_TRIAL_CURR => The "Start" button is disabled if the trial sequencing type is "Current trial" and no trial
       is selected.
    IDC_TRIAL_SEQ_SUBSETS, IDC_TRIAL_SEQ_TRIALS => The enable state of the "Start" PB is updated. When subset
       sequencing is enabled, certain trial sequencing types are disallowed; if one of the disallowed types is
       selected, the "Start" button is disabled.
    IDC_TRIAL_STOP_MODE => When auto-stop feature is on/off, the auto-stop count widget is enabled/disabled.
 2) EN_KILLFOCUS: When one of the numeric edit controls (contiguous range IDC_TRIAL_DELAY .. IDC_TRIAL_IGT) loses the
 keyboard focus, its contents may have changed. The handler merely validates user input, since each corresponding 
 parameter is restricted to a limited range.
 3) BN_CLICKED: This notification is sent by the various pushbutton and check-box controls on the dialog (contiguous
 range IDC_TRIAL_GO .. IDC_TRIAL_PRES). Response depends on the control:
     IDC_TRIAL_GO => [PB] Start or "soft-stop" the trial sequencer.
     IDC_TRIAL_ABORT => [PB] Abort the trial sequencer immediately.
     IDC_TRIAL_ATT => [PB] Clears the "#trials attempted, NOT ignored" counter (IDC_TRIAL_ATT_RO).
     IDC_TRIAL_REW => [PB] Clears the "#trials completed" counter (IDC_TRIAL_REW_RO).
     IDC_TRIAL_BLK => [PB] Clears the "#trial blocks presented" counter (IDC_TRIAL_BLK_RO).
     IDC_TRIAL_RECDATA => [ChkBox] Toggles flag to save trial data to file. Enable state of data path edit control
        and the "record spike waveform" check box are updated appropriately.
     IDC_TRIAL_RECSPKS => [ChkBox] Toggles flag to enable/disable recording of spike waveform. No action taken.
     IDC_TRIAL_PAUSE => [PB] Pauses or "resumes" an ongoing trial sequence.
     IDC_TRIAL_PRES => [PB] Clears the "#trials presented" counter (IDC_TRIAL_PRES_RO).

 @param id The control ID.
*/
void CCxTrialProtoDlg::OnChange(UINT id)
{
   int iVal, iCorr;
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl(CCxRuntime::TrialMode);

   switch( id )
   {
   case IDC_TRIAL_SET :
      if(!pTrialMode->IsSeqRunning())
      {
         // must reinit contents of the current trial CB whenever a different trial set is selected
         WORD wSet = m_cbTrialSet.GetObjKey();
         if(m_cbCurrTrial.GetParentKey() != wSet) m_cbCurrTrial.InitContents(wSet, TRUE, TRUE);

         // update state of subset seq type combo. If trial set lacks non-empty subsets, disable the combo box and 
         // make sure the current selection is "OFF"
         CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
         ASSERT(pDoc != NULL);
         if(!pDoc->HasTrialSubsets(wSet))
         {
            m_cbSubsetSeq.SetCurSel(CCxTrialSequencer::SUBSETSEQ_OFF);
            m_cbSubsetSeq.EnableWindow(FALSE);
         }
         else
            m_cbSubsetSeq.EnableWindow(TRUE);
         GetDlgItem(IDC_TRIAL_GO)->EnableWindow(CanStart());
         pTrialMode->TrialSetChanged();
      }
      break;
   case IDC_TRIAL_CURR :
   case IDC_TRIAL_SEQ_SUBSETS :
   case IDC_TRIAL_SEQ_TRIALS :
      if(!pTrialMode->IsSeqRunning())
      {
          GetDlgItem(IDC_TRIAL_GO)->EnableWindow(CanStart());
      }
      break;
   case IDC_TRIAL_STOP_MODE :
      m_edAutoStopCnt.EnableWindow((!pTrialMode->IsSeqRunning()) && 
         (m_cbAutoStopMode.GetCurSel() != CCxTrialSequencer::AUTOSTOP_OFF));
      break;

   case IDC_TRIAL_DELAY :
      iVal = m_edDelay.AsInteger();
      iCorr = cMath::rangeLimit(iVal, CCxTrialProtoDlg::MIN_TRIALDELAY, CCxTrialProtoDlg::MAX_TRIALDELAY);
      if(iCorr != iVal) m_edDelay.SetWindowText(iCorr);
      break;
   case IDC_TRIAL_STOP_COUNT :
      iVal = m_edAutoStopCnt.AsInteger();
      iCorr = cMath::rangeLimit(iVal, CCxTrialProtoDlg::MIN_AUTOSTOPCNT, CCxTrialProtoDlg::MAX_AUTOSTOPCNT);
      if(iCorr != iVal) m_edAutoStopCnt.SetWindowText(iCorr);
      break;
   case IDC_TRIAL_IGT :
      iVal = m_edIgnore.AsInteger();
      iCorr = cMath::rangeLimit(iVal, CCxTrialProtoDlg::MIN_IGNORETIME, CCxTrialProtoDlg::MAX_IGNORETIME);
      if(iCorr != iVal) m_edIgnore.SetWindowText(iCorr);
      break;

   case IDC_TRIAL_GO : 
      if(!pTrialMode->IsSeqRunning()) pTrialMode->Go();
      else if(!pTrialMode->IsSeqStopping()) pTrialMode->Halt();
      break;
   case IDC_TRIAL_ABORT : 
      if(pTrialMode->IsSeqRunning()) pTrialMode->Abort();
      break;
   case IDC_TRIAL_PRES : 
      m_edNTrials.SetWindowText(0);
      break;
   case IDC_TRIAL_ATT :
      m_edNAttempts.SetWindowText(0);
      break;
   case IDC_TRIAL_REW :
      m_edNSuccesses.SetWindowText(0);
      break;
   case IDC_TRIAL_BLK :
      m_edNBlocks.SetWindowText(0);
      break;
   case IDC_TRIAL_RECDATA :  
      m_fecDataPath.EnableWindow(IsSaveData());
      m_btnRecordSpks.EnableWindow(IsSaveData());
      break;
   case IDC_TRIAL_RECSPKS : 
      break;
   case IDC_TRIAL_PAUSE : 
      if(pTrialMode->IsSeqRunning()) 
      {
         if(pTrialMode->IsSeqPaused()) pTrialMode->Resume();
         else if(!pTrialMode->IsSeqPausing()) pTrialMode->Pause();
      }
      break;
   default :
      TRACE0( "CCxTrialProtoDlg: Unrecog ID in OnChange()\n" );
      break;
   }
}

/**
 Helper method checks whether or not the user can initiate a trial sequence via the "Start" push button, given the
 current operational state in Trial Mode and the current state of the widgets in the Protocol dialog panel. Roughly,
 the requirements are:
    -- A trial sequence cannot be running.
    -- A valid trial set containing at least one trial must be selected.
    -- The trial subset and trial sequencing modes must be compatible. If subset sequencing is engaged, then only 
       certain trial sequencing types are supported. See CCxTrialSequencer::IsValidSeqMode().
    -- If either of the "Current Trial" trial sequencing types is chosen, a trial must be currently selected in the
       IDC_TRIAL_CURR combo box.
 
 @return True if a trial sequence can be started. If not, the "Start" PB should be disabled.
*/
BOOL CCxTrialProtoDlg::CanStart()
{
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl(CCxRuntime::TrialMode);
   BOOL ok = !pTrialMode->IsSeqRunning();
   if(ok) ok = BOOL(m_cbTrialSet.GetObjKey() != CX_NULLOBJ_KEY);
   if(ok) ok = CCxTrialSequencer::IsValidSeqMode(GetSubsetSeqMode(), GetTrialSeqMode());
   if(ok)
   {
      int tsm = GetTrialSeqMode();
      if(tsm == CCxTrialSequencer::THISTRIAL || tsm == CCxTrialSequencer::THISTRIAL_NF)
         ok = BOOL(m_cbCurrTrial.GetObjKey() != CX_NULLOBJ_KEY);
   }

   return(ok);
}


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitDialog [base override] ====================================================================================
//
//    Prepare the dialog for display.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CString ops can throw CMemoryException.
//
BOOL CCxTrialProtoDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_cbTrialSet.SubclassDlgItem( IDC_TRIAL_SET, (CWnd*) this );            // current trial set selection
   m_cbCurrTrial.SubclassDlgItem( IDC_TRIAL_CURR, (CWnd*) this );          // current trial selection

   // combo box selects trial subset sequencing mode. Note that item index IS the sequencing mode!
   m_cbSubsetSeq.SubclassDlgItem(IDC_TRIAL_SEQ_SUBSETS, (CWnd*) this);
   m_cbSubsetSeq.ModifyStyle(CBS_SORT, 0);
   m_cbSubsetSeq.ResetContent();
   for(int i=0; i<CCxTrialSequencer::NUM_SUBSETSEQ; i++) 
      m_cbSubsetSeq.AddString(CCxTrialSequencer::strSubsetSeqModes[i]);
   m_cbSubsetSeq.SetCurSel(CCxTrialSequencer::SUBSETSEQ_OFF);

   // combo box selects trial sequencing mode. Again, item idex IS the sequencing mode. Initially, the 
   // "Randomized" mode is selected
   m_cbTrialSeq.SubclassDlgItem(IDC_TRIAL_SEQ_TRIALS, (CWnd*) this);
   m_cbTrialSeq.ModifyStyle(CBS_SORT, 0);
   m_cbTrialSeq.ResetContent();
   for(int i=0; i<CCxTrialSequencer::NUM_TRIALSEQ; i++)
      m_cbTrialSeq.AddString(CCxTrialSequencer::strTrialSeqModes[i]);
   m_cbTrialSeq.SetCurSel( CCxTrialSequencer::RANDOM ); 

   m_fecDataPath.SubclassDlgItem( IDC_TRIAL_DATAPATH, (CWnd*)this );       // trial data file path
   m_fecDataPath.SetFlags( 0 );                                            // pos browse btn in subclassed ctrl
   m_fecDataPath.InitializePath(
         ((CCntrlxApp*)AfxGetApp())->GetMRUTrialDataDirectory(),
         _T("trial") );

   m_btnRecordData.SubclassDlgItem(IDC_TRIAL_RECDATA, (CWnd*) this);       // check box: record trial data on/OFF
   m_btnRecordData.SetCheck( 0 );
   m_btnRecordSpks.SubclassDlgItem(IDC_TRIAL_RECSPKS, (CWnd*) this);       // check box: record spike waveform on/OFF
   m_btnRecordSpks.SetCheck( 0 );

   m_edNTrials.SubclassDlgItem( IDC_TRIAL_PRES_RO, (CWnd*) this );         // status counters are read-only edit boxes
   m_edNTrials.SetFormat( TRUE, TRUE, 5, 0 );                              // allowed count range is 0..99999
   m_edNTrials.SetWindowText( 0 );                                         // all counters initially read "0"
   m_edNAttempts.SubclassDlgItem( IDC_TRIAL_ATT_RO, (CWnd*) this );
   m_edNAttempts.SetFormat( TRUE, TRUE, 5, 0 );
   m_edNAttempts.SetWindowText( 0 );
   m_edNSuccesses.SubclassDlgItem( IDC_TRIAL_REW_RO, (CWnd*) this );
   m_edNSuccesses.SetFormat( TRUE, TRUE, 5, 0 );
   m_edNSuccesses.SetWindowText( 0 );
   m_edNBlocks.SubclassDlgItem( IDC_TRIAL_BLK_RO, (CWnd*) this );
   m_edNBlocks.SetFormat( TRUE, TRUE, 5, 0 );
   m_edNBlocks.SetWindowText( 0 );

   m_edDelay.SubclassDlgItem( IDC_TRIAL_DELAY, (CWnd*) this );             // intertrial delay in ms; integer value is
   m_edDelay.SetFormat( TRUE, TRUE, 4, 0 );                                // range limited.
   m_edDelay.SetWindowText( CCxTrialProtoDlg::MIN_TRIALDELAY );

   m_edIgnore.SubclassDlgItem( IDC_TRIAL_IGT, (CWnd*) this );              // "ignore" threshold time in ms; integer
   m_edIgnore.SetFormat( TRUE, TRUE, 4, 0 );                                // value is range limited.
   m_edIgnore.SetWindowText( CCxTrialProtoDlg::MIN_IGNORETIME );

   // the possible modes for auto-stop feature. Auto-stop disabled initially. Ensure CB does not sort entries, since 
   // index value is the mode constant.
   m_cbAutoStopMode.SubclassDlgItem(IDC_TRIAL_STOP_MODE, (CWnd*) this); 
   m_cbAutoStopMode.ModifyStyle(CBS_SORT, 0);  
   m_cbAutoStopMode.ResetContent();
   for(int i=0; i<CCxTrialSequencer::NUMAUTOSTOPMODES; i++) 
      m_cbAutoStopMode.AddString(CCxTrialSequencer::strAutoStopModes[i]);
   m_cbAutoStopMode.SetCurSel(CCxTrialSequencer::AUTOSTOP_OFF);

   // stop count for auto-stop feature
   m_edAutoStopCnt.SubclassDlgItem(IDC_TRIAL_STOP_COUNT, (CWnd*)this);
   m_edAutoStopCnt.SetFormat(TRUE, TRUE, 4, 0);
   m_edAutoStopCnt.SetWindowText(CCxTrialProtoDlg::MIN_AUTOSTOPCNT);

   return( TRUE );                                                         // set input focus to 1st ctrl in tab order
}


/** Refresh [CCxControlPanelDlg override]

 Call this method to refresh the appearance of the dialog whenever the MAESTRO runtime state changes. We update the 
 enable state of most controls and the labels of selected controls IAW the current operational state. The idea is to 
 prevent the user from modifying trial sequencer control parameters while the sequencer is running.
*/
VOID CCxTrialProtoDlg::Refresh()
{
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl( CCxRuntime::TrialMode );

   BOOL bIsOff = !pTrialMode->IsSeqRunning();                        // is trial sequencer OFF (trials not running)?
   BOOL bStopping = pTrialMode->IsSeqStopping();                     // is trial seq stopping at end of current trial?
   BOOL bPausing = pTrialMode->IsSeqPausing();                       // is it entering paused state at end of trial?
   BOOL bPaused = pTrialMode->IsSeqPaused();                         // is trial sequencer paused?

   m_cbTrialSet.EnableWindow(bIsOff);                              // these controls are enabled only when seq is OFF
   m_cbCurrTrial.EnableWindow(bIsOff);
   m_cbTrialSeq.EnableWindow(bIsOff);
   m_btnRecordData.EnableWindow(bIsOff);
   m_fecDataPath.EnableWindow(bIsOff && IsSaveData());
   m_btnRecordSpks.EnableWindow(bIsOff && IsSaveData());
   m_cbAutoStopMode.EnableWindow(bIsOff);
   m_edAutoStopCnt.EnableWindow(bIsOff && (m_cbAutoStopMode.GetCurSel() != CCxTrialSequencer::AUTOSTOP_OFF));

   // the subset seq mode combo: Always disabled when seq is running. Else, if no trial set selected, or if selected 
   // set lacks any non-empty subsets, then disable the combo box and make sure the current selection is "OFF"
   BOOL bEna = bIsOff;
   if(bEna)
   {
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      ASSERT(pDoc != NULL);
      bEna = pDoc->HasTrialSubsets( m_cbTrialSet.GetObjKey() );
      if(!bEna) m_cbSubsetSeq.SetCurSel(CCxTrialSequencer::SUBSETSEQ_OFF);
   }
   m_cbSubsetSeq.EnableWindow(bEna);

   GetDlgItem(IDC_TRIAL_ABORT)->EnableWindow(!bIsOff); 
   GetDlgItem(IDC_TRIAL_GO)->EnableWindow( (bIsOff && CanStart()) || (!bIsOff && !bStopping) ); 
   GetDlgItem(IDC_TRIAL_PAUSE)->EnableWindow(!bIsOff && !bStopping && !bPausing);

   CString strLabel = _T("START (F7)");                              // adjust label of IDC_TRIAL_GO
   if(bStopping) strLabel = _T("!!WAIT!!");
   else if(!bIsOff) strLabel = _T("STOP (F7)");
   GetDlgItem(IDC_TRIAL_GO)->SetWindowText(strLabel);

   strLabel = _T("PAUSE (F8)");                                      // adjust label of IDC_TRIAL_PAUSE
   if(bPausing) strLabel = _T("!!WAIT!!");
   else if(bPaused) strLabel = _T("RESUME (F8)");
   GetDlgItem(IDC_TRIAL_PAUSE)->SetWindowText( strLabel );
}


/** OnUpdate [CCxControlPanelDlg override] 

 CCxControlPanelDlg::OnUpdate() is a MAESTRO-specific extension of MFC's mechanism -- aka, CView::OnUpdate() -- for
 informing all document views when one of those views causes a change in the active document's contents. It passes on 
 the MAESTRO-specific doc/view hint (CCxViewHint) to the control panel dialogs, which may contain document data. When 
 the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI apps, this call is made each time 
 a new document is created/opened -- giving us an opportunity to perform any "per-document" initializations.

 This dialog's IDC_TRIAL_SET and IDC_TRIAL_CURR combo boxes display CCxDoc-based data. The first selects the trial set 
 to be used by the trial sequencer, while the second displays the next trial to be executed within that set. When the 
 hint is NULL -- meaning a new CCxDoc has just been created/opened --, we reinitialize the contents of the combo boxes
 and set both to "NONE". Otherwise we refresh the contents to reflect any relevant changes in the current CCxDoc.

 @param pHint -- [in] ptr to the MAESTRO-specific doc/view hint.
*/
VOID CCxTrialProtoDlg::OnUpdate( CCxViewHint* pHint )
{
   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
   ASSERT(pDoc != NULL);

   if(pHint == NULL)
   { 
      // "per-document inits" -- reinitialize contents of the MAESTRO object combo boxes that display the list of 
      // defined trials sets and the list of trials in the selected set. Here the initial set selection will be "NONE",
      // so the trial combo box will have only one entry, "NONE".
      m_cbTrialSet.InitContents(pDoc->GetBaseObj(CX_TRIALBASE), TRUE);
      m_cbCurrTrial.InitContents(m_cbTrialSet.GetObjKey(), TRUE, TRUE);
      
      // since no trial set is selected initially, we turn off subset sequencing and disable the relevant combo box
      m_cbSubsetSeq.SetCurSel(CCxTrialSequencer::SUBSETSEQ_OFF);
      m_cbSubsetSeq.EnableWindow(FALSE);
   }
   else if(!InitiatedUpdate()) 
   {
      // update IAW change in current CCxDoc (unless this dlg itself initiated the update!). Both the trial set and
      // current trial combo box contents are refreshed. The latter must always display children of the currently
      // selected trial set.
      m_cbTrialSet.RefreshContents();
      WORD wCurrSet = m_cbTrialSet.GetObjKey(); 
      if(wCurrSet != m_cbCurrTrial.GetParentKey()) 
         m_cbCurrTrial.InitContents(wCurrSet, TRUE, TRUE);
      else
         m_cbCurrTrial.RefreshContents();

      // if there is no trial set selected, or if the selected set lacks any subsets, then make sure subset seq is 
      // turned off and the relevant combo box disabled.
      if(!pDoc->HasTrialSubsets(wCurrSet))
      {
         m_cbSubsetSeq.SetCurSel(CCxTrialSequencer::SUBSETSEQ_OFF);
         m_cbSubsetSeq.EnableWindow(FALSE);
      } 
      else
         m_cbSubsetSeq.EnableWindow(TRUE);
   }

   Refresh();
}


//=== SetCurrentTrial =================================================================================================
//
//    Change the selection in the "current trial" combo box (IDC_TRIAL_CURR) to the specified MAESTRO object key.
//
//    ARGS:       wKey  -- [in] key of new selection.
//
//    RETURNS:    TRUE if successful, FALSE if key is not represented in the combo box.
//
BOOL CCxTrialProtoDlg::SetCurrentTrial( const WORD wKey )
{
   return( BOOL(m_cbCurrTrial.SetObjKey( wKey ) == wKey) );
}


//=== IncrementNumTrials, IncrementAttempts, IncrementSuccesses, IncrementBlocks ======================================
//
//    Increment one of the status counters (#trials, #attempted (NOT ignored), #completed, #trial blocks) on the
//    dialog.  Upon the unlikely event of reaching 100000, the counter wraps back to 0.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialProtoDlg::IncrementNumTrials()
{
   int n = 1 + m_edNTrials.AsInteger();
   if( n > 99999 ) n = 0;
   m_edNTrials.SetWindowText( n );
}

VOID CCxTrialProtoDlg::IncrementAttempts()
{
   int n = 1 + m_edNAttempts.AsInteger();
   if( n > 99999 ) n = 0;
   m_edNAttempts.SetWindowText( n );
}

VOID CCxTrialProtoDlg::IncrementSuccesses()
{
   int n = 1 + m_edNSuccesses.AsInteger();
   if( n > 99999 ) n = 0;
   m_edNSuccesses.SetWindowText( n );
}

VOID CCxTrialProtoDlg::IncrementBlocks()
{
   int n = 1 + m_edNBlocks.AsInteger();
   if( n > 99999 ) n = 0;
   m_edNBlocks.SetWindowText( n );
}





//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialParmsDlg
//
// NOTE: As of Maestro v4.1.1, one of the parameters on this dialog is persisted as an application setting -- the
// VStab sliding-average window length.
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxTrialParmsDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxTrialParmsDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_TRIAL_STAIR_STREN, IDC_TRIAL_VSTABWIN, OnEditKillFocus )
   ON_BN_CLICKED( IDC_TRIAL_RSTPARMS, OnReset )
END_MESSAGE_MAP()


//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

                                                                     // min/max/default vals for various parameters:
const int      CCxTrialParmsDlg::MIN_STAIRREVS     = 0;              //    # of staircase direction reversals to
const int      CCxTrialParmsDlg::MAX_STAIRREVS     = 99;             //    trigger an auto-stop (0 = manual stop)
const int      CCxTrialParmsDlg::DEF_STAIRREVS     = 0;
const int      CCxTrialParmsDlg::MIN_STAIRINAROW   = 1;              //    # of correct(incorrect) responses in a row
const int      CCxTrialParmsDlg::MAX_STAIRINAROW   = 10;             //    req'd to decr(incr) staircase "strength"
const int      CCxTrialParmsDlg::DEF_STAIRINAROW   = 2;
const int      CCxTrialParmsDlg::MIN_STAIRIRREL    = 0;              //    % of "irrelevant" trials presented during a
const int      CCxTrialParmsDlg::MAX_STAIRIRREL    = 100;            //    staircase sequence
const int      CCxTrialParmsDlg::DEF_STAIRIRREL    = 0;
const double   CCxTrialParmsDlg::MIN_STAIRSTREN    = -9999.999;      //    starting strength for staircase sequence
const double   CCxTrialParmsDlg::MAX_STAIRSTREN    = 9999.999;
const double   CCxTrialParmsDlg::DEF_STAIRSTREN    = 1.0;
const double   CCxTrialParmsDlg::MIN_TGTSCALE      = -999.99;        //    target pos/vel scale factor (unitless)
const double   CCxTrialParmsDlg::MAX_TGTSCALE      = 999.99;
const double   CCxTrialParmsDlg::DEF_TGTSCALE      = 1.0;
const double   CCxTrialParmsDlg::DEF_TGTROTATE     = 0.0;            //    default tgt rotation angle (deg)
const double   CCxTrialParmsDlg::MIN_STARTPOS      = -80.0;          //    H,V starting pos for all targets (deg)
const double   CCxTrialParmsDlg::MAX_STARTPOS      = 80.0;
const double   CCxTrialParmsDlg::DEF_STARTPOS      = 0.0;


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnEditKillFocus =================================================================================================
//
//    ON_CONTROL_RANGE handler for EN_KILLFOCUS notifications from edit controls housed in the dialog.
//
//    This method gives us an opportunity to range-check user input into the numeric edit controls; any out-of-range
//    parameter is corrected.
//
//    ARGS:       id -- [in] resource ID of the edit control that sent the notification.
//
//    RETURNS:    NONE
//
void CCxTrialParmsDlg::OnEditKillFocus( UINT id )
{
   int      iVal, iCorr;
   double   dVal;

   // for updating VStab window length, which is persisted in application settings object
   CCxSettings* pSet; 

   switch( id )
   {
      case IDC_TRIAL_STAIR_STREN :
         dVal = m_edStren.AsDouble();
         if( !RangeLimit( dVal, MIN_STAIRSTREN, MAX_STAIRSTREN ) ) m_edStren.SetWindowText( dVal );
         break;
      case IDC_TRIAL_STAIR_IRREL :
         iVal = m_edIrrel.AsInteger();
         if( !RangeLimit( iVal, MIN_STAIRIRREL, MAX_STAIRIRREL ) ) m_edIrrel.SetWindowText( iVal );
         break;
      case IDC_TRIAL_POS_SCALE :
         dVal = m_edPosScale.AsDouble();
         if( !RangeLimit( dVal, MIN_TGTSCALE, MAX_TGTSCALE ) ) m_edPosScale.SetWindowText( dVal );
         break;
      case IDC_TRIAL_POS_ROT :
         dVal = m_edPosRot.AsDouble();
         if( !LimitRotationAngle( dVal ) ) m_edPosRot.SetWindowText( dVal );
         break;
      case IDC_TRIAL_VEL_SCALE :
         dVal = m_edVelScale.AsDouble();
         if( !RangeLimit( dVal, MIN_TGTSCALE, MAX_TGTSCALE ) ) m_edVelScale.SetWindowText( dVal );
         break;
      case IDC_TRIAL_VEL_ROT :
         dVal = m_edVelRot.AsDouble();
         if( !LimitRotationAngle( dVal ) ) m_edVelRot.SetWindowText( dVal );
         break;
      case IDC_TRIAL_START_H :
         dVal = m_edStartH.AsDouble();
         if( !RangeLimit( dVal, MIN_STARTPOS, MAX_STARTPOS ) ) m_edStartH.SetWindowText( dVal );
         break;
      case IDC_TRIAL_START_V :
         dVal = m_edStartV.AsDouble();
         if( !RangeLimit( dVal, MIN_STARTPOS, MAX_STARTPOS ) ) m_edStartV.SetWindowText( dVal );
         break;
      case IDC_TRIAL_VSTABWIN :
         iVal = m_edVStabWin.AsInteger();
         pSet = GetSettings();
         if(pSet != NULL)
         {
            iCorr = pSet->SetVStabWinLen(iVal);
            if(iCorr != iVal)  m_edVStabWin.SetWindowText(iCorr);
         }
         break;
   }
}


//=== OnReset =========================================================================================================
//
//    Response to BN_CLICKED on "Reset" PB (IDC_TRIAL_RSTPARMS).  Restores all controls in dialog to a start-up state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxTrialParmsDlg::OnReset()
{
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl( CCxRuntime::TrialMode );

   m_edStren.SetWindowText( DEF_STAIRSTREN );
   m_edIrrel.SetWindowText( DEF_STAIRIRREL );

   m_edPosScale.SetWindowText( DEF_TGTSCALE );
   m_edPosRot.SetWindowText( DEF_TGTROTATE );
   m_edVelScale.SetWindowText( DEF_TGTSCALE );
   m_edVelRot.SetWindowText( DEF_TGTROTATE );

   m_edStartH.SetWindowText( DEF_STARTPOS );
   m_edStartV.SetWindowText( DEF_STARTPOS );

   m_spinReversals.SetPos( DEF_STAIRREVS );
   m_spinWrongUp.SetPos( DEF_STAIRINAROW );
   m_spinRightDn.SetPos( DEF_STAIRINAROW );

   GetDlgItem( IDC_TRIAL_CHAINLEN )->SetWindowText( _T("") );

   // velocity stabilization is a persisted application setting. Hitting "Reset" should not affect this parameter --
   // it's set to whatever is currently persisted in the application settings.
   CCxSettings* pSet = GetSettings();
   if(pSet != NULL) m_edVStabWin.SetWindowText(pSet->GetVStabWindowLen());

   m_btnChanEna.SetCheck( 0 );
   m_cbChanCfg.SetObjKey( CX_NULLOBJ_KEY );
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
BOOL CCxTrialParmsDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_cbChanCfg.SubclassDlgItem( IDC_TRIAL_CHCFG, (CWnd*) this );           // subclass combo box to show channel cfgs

   m_edStren.SubclassDlgItem( IDC_TRIAL_STAIR_STREN, (CWnd*) this );       // subclass & restrict format of all numeric
   m_edStren.SetFormat( FALSE, FALSE, 9, 3 );                              // edit ctrls on dialog
   m_edIrrel.SubclassDlgItem( IDC_TRIAL_STAIR_IRREL, (CWnd*) this );
   m_edIrrel.SetFormat( TRUE, TRUE, 3, 0 );
   m_edPosScale.SubclassDlgItem( IDC_TRIAL_POS_SCALE, (CWnd*) this );
   m_edPosScale.SetFormat( FALSE, FALSE, 7, 2 );
   m_edPosRot.SubclassDlgItem( IDC_TRIAL_POS_ROT, (CWnd*) this );
   m_edPosRot.SetFormat( FALSE, FALSE, 7, 2 );
   m_edVelScale.SubclassDlgItem( IDC_TRIAL_VEL_SCALE, (CWnd*) this );
   m_edVelScale.SetFormat( FALSE, FALSE, 7, 2 );
   m_edVelRot.SubclassDlgItem( IDC_TRIAL_VEL_ROT, (CWnd*) this );
   m_edVelRot.SetFormat( FALSE, FALSE, 7, 2 );
   m_edStartH.SubclassDlgItem( IDC_TRIAL_START_H, (CWnd*) this );
   m_edStartH.SetFormat( FALSE, FALSE, 6, 2 );
   m_edStartV.SubclassDlgItem( IDC_TRIAL_START_V, (CWnd*) this );
   m_edStartV.SetFormat( FALSE, FALSE, 6, 2 );
   m_edVStabWin.SubclassDlgItem( IDC_TRIAL_VSTABWIN, (CWnd*) this );
   m_edVStabWin.SetFormat( TRUE, TRUE, 3, 0 );

   m_spinReversals.SubclassDlgItem( IDC_TRIAL_STAIR_REV, (CWnd*) this );   // subclass & limit range of spin ctrls
   m_spinReversals.SetRange( MIN_STAIRREVS, MAX_STAIRREVS );
   m_spinWrongUp.SubclassDlgItem( IDC_TRIAL_STAIR_UP, (CWnd*) this );
   m_spinWrongUp.SetRange( MIN_STAIRINAROW, MAX_STAIRINAROW );
   m_spinRightDn.SubclassDlgItem( IDC_TRIAL_STAIR_DN, (CWnd*) this );
   m_spinRightDn.SetRange( MIN_STAIRINAROW, MAX_STAIRINAROW );

   m_btnChanEna.SubclassDlgItem( IDC_TRIAL_CH_ENA, (CWnd*) this );         // subclass check boxes, PB
   m_btnReset.SubclassDlgItem( IDC_TRIAL_RSTPARMS, (CWnd*) this );

   OnReset();                                                              // initialize ctrls to "start-up" defaults

   return( TRUE );                                                         // set input focus to 1st ctrl in tab order
}


//=== Refresh [CCxControlPanelDlg override] ===========================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    Here we update the ena/disabled state of the dialog's controls as needed.  When the trial sequencer is running,
//    the controls are disabled so that they reflect the sequencer control params in effect when the sequencer started.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialParmsDlg::Refresh()
{
   CCxTrialMode* pTrialMode = (CCxTrialMode*) GetModeCtrl( CCxRuntime::TrialMode );

   BOOL bEnabled = !pTrialMode->IsSeqRunning();
   m_edStren.EnableWindow( bEnabled );
   m_edIrrel.EnableWindow( bEnabled );
   m_spinReversals.EnableWindow( bEnabled );
   m_spinWrongUp.EnableWindow( bEnabled );
   m_spinRightDn.EnableWindow( bEnabled );
   m_btnReset.EnableWindow( bEnabled );
   GetDlgItem(IDC_TRIAL_CHAINLEN)->EnableWindow( bEnabled );

   bEnabled = pTrialMode->IsSeqOffOrPaused();            // these have no effect on sequencing, so they can be enabled
   m_edStartH.EnableWindow( bEnabled );                  // when the sequencer is paused or not running
   m_edStartV.EnableWindow( bEnabled );
   m_edPosScale.EnableWindow( bEnabled );
   m_edPosRot.EnableWindow( bEnabled );
   m_edVelScale.EnableWindow( bEnabled );
   m_edVelRot.EnableWindow( bEnabled );
   m_btnChanEna.EnableWindow( bEnabled );
   m_cbChanCfg.EnableWindow( bEnabled );
   m_edVStabWin.EnableWindow( bEnabled );
}


//=== OnUpdate [CCxControlPanelDlg override] ==========================================================================
//
//    CCxControlPanelDlg::OnUpdate() is a CNTRLX-specific extension of MFC's mechanism -- aka, CView::OnUpdate() -- for
//    informing all document views when one of those views causes a change in the active document's contents.  It
//    passes on the CNTRLX-specific doc/view hint (CCxViewHint) to the CNTRLX control panel dialogs, which may contain
//    document data.  When the hint object is NULL, the call is analogous to CView::OnInitialUpdate(); in SDI apps,
//    this call is made each time a new document is created/opened -- giving us an opportunity to perform any "per-
//    document" initializations.
//
//    Here we check for any change in CCxDoc that **might** affect the contents of combo box IDC_TRIAL_CHCFG, in which
//    case we refresh the contents of that box.  The combo box lists the names of all channel configuration objects
//    (CX_CHANCFG) currently defined in the document; the current selection indicates which channel configuration is
//    being used to override the channel config of individual trials -- IF that override is enabled.  Whenever a new
//    CCxDoc is created/opened, we reinitialize the contents of IDC_TRIAL_CHCFG.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxTrialParmsDlg::OnUpdate( CCxViewHint* pHint )
{
   if( pHint == NULL )                                               // "per-document inits" -- reinitialize contents
   {                                                                 // of IDC_TRIAL_CHCFG combo box:
      CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();
      ASSERT( pDoc != NULL );
      m_cbChanCfg.InitContents( pDoc->GetBaseObj( CX_CHANBASE ),     //    all channel cfgs are children of this obj
                                TRUE );                              //    allow "NONE", which is selected initially
   }
   else if(                                                          // refresh contents if a relevant change occurred
      (pHint->m_code == CXVH_NEWOBJ && (pHint->m_type == CX_CHANCFG || pHint->m_key == CX_NULLOBJ_KEY)) ||
      (pHint->m_code == CXVH_NAMOBJ && pHint->m_type == CX_CHANCFG) ||
      (pHint->m_code == CXVH_CLRUSR) ||
      (pHint->m_code == CXVH_DELOBJ && (pHint->m_type == CX_CHANCFG || pHint->m_key == CX_NULLOBJ_KEY))
      )
      m_cbChanCfg.RefreshContents();

   // just to be sure, since VStab window length is an application setting, we ensure the corresponding control 
   // reflects the current persisted value. We do this no matter what
   CCxSettings* pSet = GetSettings();
   if(pSet != NULL)
   {
      int i = pSet->GetVStabWindowLen();
      if(i != m_edVStabWin.AsInteger()) m_edVStabWin.SetWindowText(i);
   }
}



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================


//=== LimitRotationAngle ==============================================================================================
//
//    The target velocity and position rotation angles displayed on this dialog page are restricted to the unit circle,
//    ie, [0..360) degrees counterclockwise, where 0 is the rightward-pointing positive x-axis.  However, the user may
//    enter larger or negative rotation angles if desired.  This method remaps those values to the unit circle.
//
//    ARGS:       dAngle   -- [in] the rotation angle. [out] the equivalent rotation angle restricted to unit circle.
//
//    RETURNS:    FALSE if the angle argument's value had to be modified.
//
BOOL CCxTrialParmsDlg::LimitRotationAngle( double& dAngle )
{
   double d = cMath::limitToUnitCircleDeg( dAngle );
   if( d != dAngle )
   {
      dAngle = d;
      return( FALSE );
   }
   else return( TRUE );
}




//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialStatsDlg
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxTrialStatsDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxTrialStatsDlg, CCxControlPanelDlg )
END_MESSAGE_MAP()


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTrialStatsDlg, ~CCxTrialStatsDlg [constructor, destructor] ===================================================
//
//    Construct/destroy the dialog object.  Upon destruction, make sure that any memory allocated by the object has
//    been released.
//
CCxTrialStatsDlg::CCxTrialStatsDlg() : CCxControlPanelDlg( IDD )
{
   m_pSeq = NULL;
   m_gridWidth = 300;
}

CCxTrialStatsDlg::~CCxTrialStatsDlg()
{
   m_pSeq = NULL;
}



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitDialog [base override] ====================================================================================
//
//    Prepare the dialog for display.
//
//    Here we subclass dlg resource template-defined controls to class members, prepare the grid control that will
//    represent the trial set statistics, and initialize all to "start-up" conditions.  See GridDispCB() for a detailed
//    explanation of the grid control's makeup.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE to place initial input focus on the first ctrl in dialog's tab order.
//                FALSE if we've already set the input focus on another ctrl.
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for required rows and columns.
//
BOOL CCxTrialStatsDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                               // let base class do its thing...

   m_setLabel.SubclassDlgItem( IDC_TSTAT_SET, (CWnd*) this );        // subclass static ctrl displaying trial set name

   m_grid.SubclassDlgItem( IDC_TSTAT_GRID, this );                   // prepare grid ctrl displaying trial set stats...
   m_grid.EnableDragAndDrop( FALSE );                                // disable drag-n-drop features
   m_grid.SetRowResize( FALSE );                                     // user may not resize rows or columns
   m_grid.SetColumnResize( FALSE );
   m_grid.EnableSelection( FALSE );                                  // cells in grid cannot be selected

   m_grid.SetCallbackFunc( GridDispCB, (LPARAM) this );              // set callbacks which govern appearance/editing
   m_grid.SetEditCBFcn( GridEditCB, (LPARAM) this );                 // of grid cells.  TRICK: we pass THIS reference
   m_grid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );           // b/c CB fcn must be static
   m_grid.SetTreeInfoCBFcn( CCxDoc::TreeInfoCB,                      // note that we rely on document for CNTRLX obj
               (LPARAM) ((CCntrlxApp*) AfxGetApp())->GetDoc() );     // tree info...

   // initially configure grid to display stats for seq modes other than the "chained" modes
   m_grid.SetRowCount( 1 ); 
   m_grid.SetColumnCount( 3 ); 
   m_grid.SetFixedRowCount( 1 ); 
   m_grid.SetFixedColumnCount( 0 );

   CGridCellBase* pCell = m_grid.GetDefaultCell( TRUE, TRUE );       // set default cell formats
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( TRUE, FALSE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( FALSE, TRUE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );
   pCell = m_grid.GetDefaultCell( FALSE, FALSE );
   pCell->SetFormat( DT_CENTER | DT_SINGLELINE );

   m_grid.SetGridLineColor( RGB(0,0,0) );                            // use black grid lines

   // get fixed width of stats grid, not including space for grid control's border and a vertical scroll bar
   CRect rGrid;  
   m_grid.GetWindowRect( rGrid ); 
   ScreenToClient( rGrid ); 
   CRect rClient;
   m_grid.GetClientRect( rClient );
   int rsvdW = (rGrid.Width() - rClient.Width()) + ::GetSystemMetrics(SM_CXVSCROLL) + 2;
   m_gridWidth = rGrid.Width() - rsvdW;

   // initially confiugre grid to display stats for seq modes other than "chained" modes
   m_grid.SetColumnWidth( 0, m_gridWidth - 2*MINCOLW );
   m_grid.SetColumnWidth( 1, MINCOLW );
   m_grid.SetColumnWidth( 2, MINCOLW );

   return( TRUE );
}


//=== Refresh [CCxControlPanelDlg override] ===========================================================================
//
//    Call this method to refresh the appearance of the dialog whenever the CNTRLX runtime state changes.
//
//    Since this dialog merely provides a simple read-only statistics summary for the most recent trial sequence,
//    we don't need to do anything here.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialStatsDlg::Refresh()
{
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
//    When a new CCxDoc is opened, we make sure the statistics table is emptied.
//
//    ARGS:       pHint -- [in] ptr to the CNTRLX-specific doc/view hint.
//
//    RETURNS:    NONE.
//
VOID CCxTrialStatsDlg::OnUpdate( CCxViewHint* pHint )
{
   if( pHint == NULL )                                               // "per-document inits"
   {
      m_grid.SetTreeInfoCBFcn( CCxDoc::TreeInfoCB,                   // since we rely on doc obj for the tree info CB,
               (LPARAM) ((CCntrlxApp*) AfxGetApp())->GetDoc() );     // we reinstall it to be safe (tho it's not used)
      Initialize( CX_NULLOBJ_KEY );                                  // empty stats table
   }
}


//=== Initialize ======================================================================================================
//
//    Reset the trial statistics table in this dialog, and reload to reflect the names of the trials in the specified
//    trial set. All trial counts are reset to zero.
//
//    The table is laid out in one of two configurations depending on the current sequencer mode. In the "chained"
//    sequencer modes, the names of the trials in the set (usually there are just a few, typically just two) appear in
//    the column headers for columns 1, 2, and so on. Rows 1-11 in column 0 display the # of reps of 1-10 consecutive
//    SUCCESSFUL reps of the same trial, plus "11+" as a catch-all for successful chains longer than 10. Row 12 
//    contains the total # of attempts for each trial in the set, while row 13 displays the total # of successful
//    individual trial reps.
//
//    In all other sequencer modes, the trial names are listed in rows 1-N of column 0, # attempts for each trial in
//    column 1, and # successfully complete trial reps in column 2.
//
//    If the key provided does not retrieve a valid, non-empty trial set, then the statistics table will be empty, and 
//    calls to UpdateStats() will have no effect. Trials are listed in the statistics table in the same order that they 
//    are retrieved from the current document.
//
//    ARGS:       wKeySet  -- [in] key to the trial set for which statistics are to be displayed.
//
//    RETURNS:    NONE.
//
VOID CCxTrialStatsDlg::Initialize( WORD wKeySet )
{
   // make sure trial set exists
   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc(); 
   BOOL isValidSet = (pDoc!=NULL) && pDoc->ObjExists(wKeySet) && (pDoc->GetObjType(wKeySet)==CX_TRIALSET);

   // configure grid as appropriate to the sequencer mode. It's possible we don't have a reference to the sequencer yet!
   if(m_pSeq == NULL)
   {
      m_grid.SetColumnCount(3);
      m_grid.SetFixedColumnCount(0);
      m_grid.SetRowCount(1);
      m_grid.SetColumnWidth( 0, m_gridWidth - 2*MINCOLW );
      m_grid.SetColumnWidth( 1, MINCOLW );
      m_grid.SetColumnWidth( 2, MINCOLW );
   }
   else if(m_pSeq->IsChainedMode())
   {
      m_grid.SetRowCount(1 + CCxTrialSequencer::MAX_CHAINLEN + 2);
      m_grid.SetColumnCount(isValidSet ? m_pSeq->GetNumTrialsSequenced() + 1 : 1);
      m_grid.SetFixedColumnCount(1);
      m_grid.SetColumnWidth(0, MINCOLW);
      if(m_grid.GetColumnCount() > 1)
      {
         int colW = (m_gridWidth - MINCOLW) / m_pSeq->GetNumTrialsSequenced();
         if(colW < MINNAMECOLW) colW = MINNAMECOLW;
         for(int i=1; i<m_grid.GetColumnCount(); i++) m_grid.SetColumnWidth(i, colW);
      }
   }
   else
   {
      m_grid.SetColumnCount(3);
      m_grid.SetFixedColumnCount(0);
      m_grid.SetRowCount(isValidSet ? m_pSeq->GetNumTrialsSequenced() + 1 : 1);
      m_grid.SetColumnWidth( 0, m_gridWidth - 2*MINCOLW );
      m_grid.SetColumnWidth( 1, MINCOLW );
      m_grid.SetColumnWidth( 2, MINCOLW );
   }
   m_grid.Refresh();

   CString label = _T("<none>");
   if(isValidSet) 
      label.Format("%s%s", pDoc->GetObjName( wKeySet ), ((m_pSeq != NULL) && m_pSeq->IsChainedMode()) ? " <chained>" : "");
   m_setLabel.SetWindowText( label );
}

//=== UpdateStats =====================================================================================================
//
//    Update the statistics table in this dialog. Depending on the current sequencer mode, this refreshes the column 
//    (for chained-mode stats) or row (all other modes) in the table corresponding to the trial specified. The actual
//    statistics are maintained in the trial sequencer object. See GridDisplayCB().
//
//    ARGS:       wTrial   -- [in] Key of trial for which stats need updating.
//    RETURNS:    NONE.
//
VOID CCxTrialStatsDlg::UpdateStats(WORD wTrial)
{
   if(m_pSeq == NULL) return;
   int idxTrial = m_pSeq->GetIndexForTrialKey(wTrial);
   
   if(m_pSeq->IsChainedMode())
   {
      int col = idxTrial + 1;
      if(col >= 1 && col < m_grid.GetColumnCount())
         m_grid.RedrawColumn(col);
   }
   else
   {
      int row = idxTrial + 1;
      if(row >= 1 && row < m_grid.GetRowCount())
         m_grid.RedrawRow(row);
   }
}


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== GridDispCB ======================================================================================================
//
//    Callback function queried by the "statistics table" grid control to obtain the contents of each cell in the grid.
//
//    The statistics table comes in two different layouts -- one for the "chained" trial sequencer modes, and one for
//    all other modes.
//
//    In modes other than the "chained" modes, the statistics table is an (N+1)-by-3 grid, where N is the # of trials 
//    in the trial set for which statistics are being displayed. The first, fixed row displays column headings. For 
//    each trial we display:
//       col 0:  Trial name
//       col 1:  # trial attempts
//       col 2:  # of those attempts that were successfully completed
//
//    The "chained" sequencer modes randomize "chains" (1-M consecutive presentations of the same trial) of trials 
//    rather than individual trials. Only a handful of trials, typically just 2, participate in such sequences, and
//    the goal is to observe "learning" over the repeated presentations. In these modes, the statistics table is a
//    14-by-(N+1) grid, where N is again the # of participating trials. The trial names are listed in columns 1-N of
//    the first, fixed row. For each trial we display:
//       row 1-10 : Number of times we observe a sequence of 1-10 consecutive reps of the trial (preceded by a 
//                  an event that resets the consecutive reps counter -- start of sequence, resumption of paused
//                  sequence, presentation of a different trial, a failed trial).
//       row 11   : Catchall for consecutive rep sequences longer than 10 (should be rare).
//       row 12   : Total # trial attempts
//       row 13   : Total # of those attempts that were successfully completed
//
//    CCxTrialStatsDlg does NOT maintain these statistics itself. Rather it queries the trial sequencer object, 
//    which is responsible for accumulating these stats.
//
//    NOTE: A callback function must be static.  As such, it does not have access to instance fields of the object.  To
//    circumvent this problem, we take advantage of the generic LPARAM argument, passing a reference to this dlg when
//    we register the callback fcn with the grid in OnInitDialog().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info.
//                lParam      -- [in] THIS (see NOTE).
//
//    RETURNS:    TRUE if display info was provided; FALSE otherwise.
//
BOOL CALLBACK CCxTrialStatsDlg::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam )
{
   CCxTrialStatsDlg* pThis = (CCxTrialStatsDlg*)lParam;                 // to access non-static data in the dlg obj
   CLiteGrid* pGrid = &(pThis->m_grid);                                 // the grid control showing statistics
   const CCxTrialSequencer* pSeq = pThis->m_pSeq;                       // the trial sequencer

   CCellID c;                                                           // the cell whose info is requested
   c.row = pDispInfo->item.row;
   c.col = pDispInfo->item.col;

   // FAIL if grid control is gone, or cell does not exist, or sequencer is not available
   if((pSeq == NULL) || (pGrid->GetSafeHwnd() == NULL) || !pGrid->IsValid(c))
      return(FALSE);

   // we don't use label tips on this grid
   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP ) 
   {
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
      return( TRUE );
   }

   // get index of relevant trial, retrieve requested stat from sequencer, and prepare string that appears in cell
   // accordingly. Trials are listed by column when sequencer is in a chained mode; else by row. -1 corresponds to 
   // the row or column header...
   int iTrial = -2;
   if(pSeq->IsChainedMode())
   {
      int iTrial = c.col - 1;
      if(iTrial < -1) return(FALSE);
      if(c.row == 0)
      {
         if(iTrial == -1) pDispInfo->item.strText = _T("");
         else pDispInfo->item.strText = pSeq->GetTrialName(iTrial);
      }
      else if(c.row <= 11)
      {
         if(iTrial == -1)
         {
            if(c.row == 11) pDispInfo->item.strText = _T("11+");
            else pDispInfo->item.strText.Format("%d", c.row);
         }
         else
            pDispInfo->item.strText.Format("%d", pSeq->GetNumSuccessfulChains(iTrial, c.row));
      }
      else if(c.row == 12)
      {
         if( iTrial == -1 ) pDispInfo->item.strText = _T("#tries");
         else pDispInfo->item.strText.Format("%d", pSeq->GetNumAttempted(iTrial));
      }
      else if(c.row == 13)
      {
         if( iTrial == -1 ) pDispInfo->item.strText = _T("#OK");
         else pDispInfo->item.strText.Format("%d", pSeq->GetNumCompleted(iTrial));
      }
   }
   else
   {
      int iTrial = c.row - 1;  
      if(iTrial < -1) return(FALSE);
      switch(c.col)
      {
      case 0 :
         if(iTrial == -1) pDispInfo->item.strText = _T("Trial Name");
         else pDispInfo->item.strText = pSeq->GetTrialName(iTrial);
         break;
      case 1 :
         if( iTrial == -1 ) pDispInfo->item.strText = _T("#tries");
         else pDispInfo->item.strText.Format("%d", pSeq->GetNumAttempted(iTrial));
         break;
      case 2 :
         if( iTrial == -1 ) pDispInfo->item.strText = _T("#OK");
         else pDispInfo->item.strText.Format("%d", pSeq->GetNumCompleted(iTrial));
         break;
      }
   }

   // only show title tip if the cell's text is too big to fit
   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;
   return( TRUE );
}


//=== GridEditCB ======================================================================================================
//
//    Callback invoked to initiate inplace editing of a cell in the statistics summary grid, or to increment/decrement
//    the contents of a cell in response to a left or right mouse click.  Since the grid is entirely read-only, this
//    method does very little.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialStatsDlg::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   pEI->iClick = 0;
   pEI->iType = LG_READONLY;
   return( TRUE );
}


//=== GridEndEditCB ===================================================================================================
//
//    Callback invoked upon termination of inplace editing of the statistics summary grid.
//
//    Since the grid is entirely read-only, this method should never be called.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialStatsDlg::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   return( TRUE );
}




//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialMode
//
//=====================================================================================================================
//=====================================================================================================================

//=====================================================================================================================
// STATIC CLASS MEMBER INITIALIZATION
//=====================================================================================================================

const DWORD CCxTrialMode::F_RUNNING             = ((DWORD)(1<<0));   // if set, trial seq is running
const DWORD CCxTrialMode::F_STOPPING            = ((DWORD)(1<<1));   // if set, seq will stop when current trial done
const DWORD CCxTrialMode::F_WAITING             = ((DWORD)(1<<2));   // if set, we're in the intertrial delay period
const DWORD CCxTrialMode::F_PAUSING             = ((DWORD)(1<<3));   // if set, deq will pause when current trial done
const DWORD CCxTrialMode::F_PAUSED              = ((DWORD)(1<<4));   // if set, trial seq is paused
const DWORD CCxTrialMode::F_RUNNINGMASK         = CCxTrialMode::F_RUNNING |
                                                  CCxTrialMode::F_STOPPING |
                                                  CCxTrialMode::F_WAITING |
                                                  CCxTrialMode::F_PAUSING |
                                                  CCxTrialMode::F_PAUSED;
const DWORD CCxTrialMode::F_RECDATA             = ((DWORD)(1<<5));   // if set, trial data is saved to file



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTrialMode [constructor] ======================================================================================
//
CCxTrialMode::CCxTrialMode( CCxControlPanel* pPanel ) : CCxModeControl( CCxRuntime::TrialMode, pPanel )
{
   m_pProtoDlg = NULL;
   m_pParmsDlg = NULL;
   m_pStatsDlg = NULL;
   m_pRPDistroDlg = NULL;
   m_pFixRewDlg = NULL;
   m_pVideoDspDlg = NULL;
   m_pEyelinkDlg = NULL;
   m_dwState = 0;
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== InitDlgs [CCxModeControl override] ==============================================================================
//
//    Install, in the CNTRLX master mode control panel, those dialogs required for operator interactions in the CNTRLX
//    operational mode represented by this mode controller.
//
//    A total of six dialogs are currently required during TrialMode.  We install the three TrialMode-specific dialogs
//    here.  If any non-specific dialogs have not yet been installed, they are also installed here; else, we merely 
//    save pointers to them.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (failed to create one of the required dialogs).
//
BOOL CCxTrialMode:: InitDlgs()
{
   ASSERT( m_pCtrlPanel );                                              // verify our ptr to the mode control panel
   ASSERT_KINDOF( CCxControlPanel, m_pCtrlPanel );

   m_pProtoDlg = (CCxTrialProtoDlg*) m_pCtrlPanel->AddDlg( _T("Protocol"), RUNTIME_CLASS(CCxTrialProtoDlg) );
   if( m_pProtoDlg == NULL ) return( FALSE );

   m_pParmsDlg = (CCxTrialParmsDlg*) m_pCtrlPanel->AddDlg( _T("Other Params"), RUNTIME_CLASS(CCxTrialParmsDlg) );
   if( m_pParmsDlg == NULL ) return( FALSE );

   m_pStatsDlg = (CCxTrialStatsDlg*) m_pCtrlPanel->AddDlg( _T("Statistics"), RUNTIME_CLASS(CCxTrialStatsDlg) );
   if( m_pStatsDlg == NULL ) return( FALSE );
   m_pStatsDlg->SetSequencer(m_seq);

   m_pRPDistroDlg = (CCxRPDistroDlg*) m_pCtrlPanel->AddDlg( _T("R/P Distro"), RUNTIME_CLASS(CCxRPDistroDlg) );
   if( m_pRPDistroDlg == NULL ) return( FALSE );

   m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxFixRewDlg) );
   if( m_pFixRewDlg == NULL )
   {
      m_pFixRewDlg = (CCxFixRewDlg*) m_pCtrlPanel->AddDlg( _T("Fix/Reward"), RUNTIME_CLASS(CCxFixRewDlg) );
      if( m_pFixRewDlg == NULL ) return( FALSE );
   }

   m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxVideoDspDlg) );
   if( m_pVideoDspDlg == NULL )
   {
      m_pVideoDspDlg = (CCxVideoDspDlg*) m_pCtrlPanel->AddDlg( _T("RMVideo Display"), RUNTIME_CLASS(CCxVideoDspDlg) );
      if( m_pVideoDspDlg == NULL ) return( FALSE );
   }

   m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->GetDlgByClass( RUNTIME_CLASS(CCxEyelinkDlg) );
   if( m_pEyelinkDlg == NULL )
   {
      m_pEyelinkDlg = (CCxEyelinkDlg*) m_pCtrlPanel->AddDlg( _T("EyeLink"), RUNTIME_CLASS(CCxEyelinkDlg) ); 
      if( m_pEyelinkDlg == NULL ) return( FALSE );
   }

   return( TRUE );
}


//=== Service [CCxModeControl override] ===============================================================================
//
//    Update runtime state in Trial Mode.  There's nothing to do here unless we're presenting trials.  When a trial
//    ends, we update status info & data file path on the embedded dialogs as appropriate, then start the next trial
//    -- unless the user has chosen to stop the sequence at trial's end or the sequence has auto-stopped.
//
//    NOTE:  For some sequencer modes, the results from the previous trial affect selection of the next trial!
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialMode::Service()
{
   ASSERT( m_pRuntime && (m_pRuntime->GetMode() == CX_TRIALMODE) );

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                              // for global stuff

   int n = GetNumRewardsDelivered();                                          // make sure reward statistics are
   int total = GetCumulativeReward();                                         // up-to-date
   m_pFixRewDlg->UpdateRewardStats( n, total );

   if( IsSeqPaused() )                                                        // if the trial sequencer has been paused
      ;                                                                       //    do NOTHING
   else if( IsSeqWaiting() )                                                  // if we're waiting to start next trial
   {                                                                          // in a seqence, start it if we've waited
      int iDelay = int( m_waitTime.Get() / 1000.0 );                          // long enough.  Disengage sequencing if
      if( iDelay >= m_pProtoDlg->GetInterTrialDelay() )                       // next trial fails to start.
      {
         m_dwState &= ~F_WAITING;
         CString strDest = m_strShadowPath.IsEmpty() ? m_strTrialPath : m_strShadowPath;
         if( !m_pRuntime->StartTrial( &m_seq, strDest, m_pProtoDlg->IsSaveSpikes() ) )
         {
            m_dwState = 0;
            Refresh();
         }
      }
   }
   else if( IsSeqRunning() && !m_pRuntime->IsTrialRunning() )                 // if a trial was running and has just
   {                                                                          // finished:
      m_pRuntime->StopTraces();                                               // flush & stop data trace display
      m_pRuntime->StopEventStream();                                          // flush & stop digital event stream

      WORD wLast = m_seq.GetCurrentTrialKey();                                // key of trial that just finished
      DWORD dwRes = m_pRuntime->GetProtocolStatus();                          // get trial results flags
      m_seq.SelectNextTrial( dwRes );                                         // select next trial NOW, given results;
                                                                              // note some flags may be altered here!

      if( (dwRes & (CX_FT_ERROR|CX_FT_ABORTED)) == 0 )                        // as long as trial did not abort on some
      {                                                                       // error...
         m_pProtoDlg->IncrementNumTrials();                                   // incr #trials presented
         BOOL bSuccess = m_seq.WasTrialCompleted(dwRes);                      // was trial completed successfully?
         if( bSuccess )                                                       // if so, incr #trials completed
            m_pProtoDlg->IncrementSuccesses();
         if( bSuccess ||                                                      // if success OR aborted trial len
             (m_pRuntime->GetLastTrialLen() > m_pProtoDlg->GetIgnoreTime()) ) // exceeded "ignore" threshold
         {
            m_pProtoDlg->IncrementAttempts();                                 // incr #trials attempted and update
            m_pStatsDlg->UpdateStats( wLast );                                // trial set stats ("ignored" trials not
         }                                                                    // included)

         CCxSpikeHistBar* pHist =                                             // commit spikes accum by hist facility
            pApp->GetMainFrame()->GetSpikeHistogramDisplay();
         pHist->Commit();

         UpdateRPDistroTrial( wLast, dwRes );                                 // if last trial used "R/P Distro" op,
                                                                              // update corres dialog page
      }

      if( (dwRes & CX_FT_BLOCKDONE) != 0 ) m_pProtoDlg->IncrementBlocks();    // a trial block just finished; incr ctr

      if( (m_dwState & F_RECDATA) && ((dwRes & CX_FT_DATASAVED) != 0) )       // if trial data file saved:
      {
         if( !m_strShadowPath.IsEmpty() )                                     //    if written to shadow file on local
         {                                                                    //    disk, move it to remote drive. We
            if( !pApp->MoveShadowFile(m_strTrialPath, m_strShadowPath) )      //    must abort and alert user if we're
            {                                                                 //    unable to do so!
               m_dwState = 0;
               Refresh();
               ::MessageBeep( MB_ICONEXCLAMATION );
               return;
            }
         }

         m_pProtoDlg->IncrementNextDataFile( m_strTrialPath );                // incr file name and get shadow path if
         if( !pApp->GetShadowFile(m_strTrialPath, m_strShadowPath) )          // shadowing is necessary
         {
            m_dwState = 0;
            Refresh();
            ::MessageBeep( MB_ICONEXCLAMATION );
            return;
         }
      }

      if( ((dwRes & (CX_FT_SEQSTOP|CX_FT_ERROR)) != 0) || IsSeqStopping() )   // if we're auto-stopping sequence, or an
      {                                                                       // error occurred, or user has elected to
         m_dwState = 0;                                                       // stop seq, then disengage sequencing!
         Refresh();
         if( dwRes & CX_FT_ERROR ) ::MessageBeep( MB_ICONEXCLAMATION );       // beep to alert user if error occurred
      }
      else                                                                    // else:
      {
         WORD wKey = m_seq.GetCurrentTrialKey();                              //    display name of next trial in
         VERIFY( m_pProtoDlg->SetCurrentTrial( wKey ) );                      //    "Protocol" dialog

         if( m_pProtoDlg->GetInterTrialDelay() > 0 )                          //    if there's a nonzero intertrial
         {                                                                    //    delay, adjust our state and reset
            m_waitTime.Reset();                                               //    the elapsed timer
            m_dwState |= F_WAITING;
         }

         if( IsSeqPausing() )                                                 //    if we're pausing, set the paused
         {                                                                    //    flag so we don't start next trial
            m_dwState &= ~F_PAUSING;
            m_dwState |= F_PAUSED;
            m_seq.SetPaused();
            Refresh();
         }

         if( IsSeqPaused() || IsSeqWaiting() )                                //    if paused or waiting out intertrial
            return;                                                           //    delay, do NOT start next trial

         CString strDest =                                                    //    otherwise, start the next trial
            m_strShadowPath.IsEmpty() ? m_strTrialPath : m_strShadowPath;     //    immediately. if we're unable to do
         if( !m_pRuntime->StartTrial( &m_seq, strDest,                        //    so, we disengage sequencing
                        m_pProtoDlg->IsSaveSpikes() ) )
         {
            m_dwState = 0;
            Refresh();
            ::MessageBeep( MB_ICONEXCLAMATION );
         }
      }
   }
}


//=== Enter, Exit [CCxModeControl overrides] ==========================================================================
//
//    Enter() should perform any initializations upon entering the operational mode represented by the mode controller,
//    while Exit() handles any cleanup activities just prior to exiting the mode.  One task that the mode controller
//    must handle is to update the subset of dialogs that are accessible on the mode control panel IAW the current op
//    mode.  It is recommended that the mode controller "hide" all dialogs in Exit(), and "show" only the relevant
//    dialogs in Enter().
//
//    We enter or leave Trial Mode in an "inactive" state, with no trial seq in progress and CXDRIVER essentially idle.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxTrialMode::Enter()
{
   ASSERT( m_pRuntime );                                                   // MUST be in CX_TRIALMODE!
   if( m_pRuntime->GetMode() != CCxRuntime::TrialMode ) return( FALSE );

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->ShowDlg( m_pProtoDlg, -1 );                               // show the relevant mode ctrl dlgs
   m_pCtrlPanel->ShowDlg( m_pParmsDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pStatsDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pRPDistroDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pFixRewDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pVideoDspDlg, -1 );
   m_pCtrlPanel->ShowDlg( m_pEyelinkDlg, -1 );
   m_pCtrlPanel->SetActiveDlg( m_pProtoDlg );                              // "Protocol" dlg is in front initially

   m_dwState = 0;                                                          // start out with no trials running
   m_pRuntime->ClearProtocolStatus();
   Refresh();
   UpdateVideoCfg();                                                       // make sure video display and fix/reward
   UpdateFixRewSettings();                                                 // settings are up-to-date on CXDRIVER side

   return( TRUE );
}

BOOL CCxTrialMode::Exit()
{
   ASSERT( m_pRuntime );                                                   // MUST be in CX_TRIALMODE!
   if( m_pRuntime->GetMode() != CCxRuntime::TrialMode ) return( FALSE );

   Abort();                                                                // stop trial sequencer NOW, if it's running

   ASSERT( m_pCtrlPanel );
   m_pCtrlPanel->HideDlg( NULL );                                          // hide all mode ctrl dlgs currently visible

   return( TRUE );
}


//=== Go ==============================================================================================================
//
//    Retrieve the current trial sequencer protocol/control parameters from the panel dialog pages and start trial
//    sequencing IAW those parameters.  We initialize the trial sequencer (CCxTrialSequencer) IAW the current control
//    parameters and start the first trial.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CString ops may throw CMemoryException.
//
VOID CCxTrialMode::Go()
{
   ASSERT( m_pRuntime );                                                // MUST be in CX_TRIALMODE, and must not be
   if( m_pRuntime->GetMode()!=CCxRuntime::TrialMode || IsSeqRunning() ) // running trials already!
      return;

   ASSERT( !m_pRuntime->IsTrialRunning() );                             // if trial sequencing is not on, then there
                                                                        // should never be a trial running!!

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();                        // for message logging and other global stuff

   if( m_pProtoDlg->GetTrialSet() == CX_NULLOBJ_KEY ) return;           // cannot run a trial seq if set not specified!

   m_dwState = F_RUNNING;                                               // switch to running state now & refresh dlgs;
   Refresh();                                                           // this will prevent further user input
   pApp->LogMessage( _T("Starting trial sequence..."), TRUE );

   TRIALSEQCTRL tsqc;                                                   // get trial sequencer ctrl params from dialogs
   tsqc.wTrialSet       = m_pProtoDlg->GetTrialSet();
   tsqc.wCurrTrial      = m_pProtoDlg->GetCurrentTrial();
   tsqc.iSubsetSeq      = m_pProtoDlg->GetSubsetSeqMode();
   tsqc.iTrialSeq       = m_pProtoDlg->GetTrialSeqMode();
   tsqc.dStairStrength  = m_pParmsDlg->GetStairStartStrength();
   tsqc.nStairIrrel     = m_pParmsDlg->GetStairPctIrrelevant();
   tsqc.nWrongUp        = m_pParmsDlg->GetStairNumWrongUp();
   tsqc.nRightDn        = m_pParmsDlg->GetStairNumRightDn();
   tsqc.nReversals      = m_pParmsDlg->GetStairNumReversals();
   m_pParmsDlg->GetTrialChainLengths(tsqc.strChainLens);

   m_seq.SetAutoStopParams(m_pProtoDlg->GetAutoStopMode(), m_pProtoDlg->GetAutoStopCount());

   m_seq.SetStartingPosH( m_pParmsDlg->GetStartingTgtPosH() );
   m_seq.SetStartingPosV( m_pParmsDlg->GetStartingTgtPosV() );
   m_seq.SetTgtPosScale( m_pParmsDlg->GetTgtPosScale() );
   m_seq.SetTgtPosRotate( m_pParmsDlg->GetTgtPosRotation() );
   m_seq.SetTgtVelScale( m_pParmsDlg->GetTgtVelScale() );
   m_seq.SetTgtVelRotate( m_pParmsDlg->GetTgtVelRotation() );
   m_seq.SetChanCfgOverride( m_pParmsDlg->IsChanCfgOverrideEnabled() );
   m_seq.SetChanCfgOverrideKey( m_pParmsDlg->GetChanCfgOverride() );

   CString strErr = _T("!! ERROR: Aborting trial sequence !!");

   if( !m_seq.Init( tsqc ) )                                            // init trial sequencer with ctrl params and
   {                                                                    // select first trial; abort on failure.
      m_dwState = 0;
      Refresh();
      pApp->LogMessage( strErr );
      return;
   }

   if( !m_pRuntime->LoadTargetList( &m_seq ) )                          // load all targets that will participate in any
   {                                                                    // trial of the sequence.  NOTE: FB preload
      m_dwState = 0;                                                    // happens here -- can take a while!
      Refresh();
      pApp->LogMessage( strErr );
      return;
   }

   m_strTrialPath.Empty();                                              // if saving data, get path; pathname is empty
   m_strShadowPath.Empty();                                             // if we're NOT saving data
   if( m_pProtoDlg->IsSaveData() )
   {
      m_dwState |= F_RECDATA;
      m_pProtoDlg->GetNextDataFile( m_strTrialPath );
      if( !pApp->GetShadowFile( m_strTrialPath, m_strShadowPath ) )     // if shadowing necessary but we cannot get a
      {                                                                 // shadow path, then abort
         m_dwState = 0;
         Refresh();
         return;
      }
   }

   // inform MAESTRODRIVER of transform parameters and VStab sliding window length to be used during this trial seq. The
   // transform parameter effect trial code computations. A sliding-window average is used to smooth out eye-position 
   // noise artifacts during VStabe. All of these get saved in data file header
   m_pRuntime->SetTransform(&m_seq);
   m_pRuntime->SetVStabSlidingWindow(m_pParmsDlg->GetVStabSlidingWindowLen());

   CCxSpikeHistBar* pHist =                                             // init spike histogram facility
      pApp->GetMainFrame()->GetSpikeHistogramDisplay();
   pHist->Initialize( m_pProtoDlg->GetTrialSet() );

   CString strDest =                                                    // start first trial; save data file to shadow
      m_strShadowPath.IsEmpty() ? m_strTrialPath : m_strShadowPath;     // location if necessary
   if( !m_pRuntime->StartTrial( &m_seq, strDest, m_pProtoDlg->IsSaveSpikes() ) )
   {
      m_dwState = 0;
      Refresh();
      pApp->LogMessage( strErr );
      return;
   }

   m_pStatsDlg->Initialize( m_pProtoDlg->GetTrialSet() );               // initialize trial set statistics

   WORD wKey = m_seq.GetCurrentTrialKey();                              // display name of trial just started in the
   VERIFY( m_pProtoDlg->SetCurrentTrial( wKey ) );                      // "Protocol" dialog
}


//=== Halt ============================================================================================================
//
//    If a trial sequence is currently running, stop the sequencer as soon as the current trial ends ("soft-stop").
//    All we do here is set a state flag and refresh the dialogs (so they can reflect the change in runtime state).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialMode::Halt()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::TrialMode) && IsSeqRunning() )
   {
      if( IsSeqPaused() || IsSeqWaiting() )                    // if seq is paused or we're waiting between trials,
      {                                                        // stop NOW.
         m_dwState = 0;
         Refresh();
      }
      else if( !IsSeqStopping() )                              // if we're not already stopping, set soft-stop flag
      {
         m_dwState |= F_STOPPING;
         Refresh();
      }
   }
}


//=== Pause ===========================================================================================================
//
//    If a trial sequence is currently running, pause the sequencer as soon as the current trial ends.  All we do here
//    is set a state flag and refresh the dialogs (so they can reflect the change in runtime state).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialMode::Pause()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::TrialMode) && IsSeqRunning() && !(IsSeqStopping() || IsSeqPaused()) )
   {
      if( IsSeqWaiting() )                                     // if we're waiting between trials, pause NOW
      {
         m_dwState |= F_PAUSED;
         m_dwState &= ~F_PAUSING;
         m_seq.SetPaused();
         Refresh();
      }
      else if( !IsSeqPausing() )                               // if we're not already pausing, set flag to pause at
      {                                                        // end of current trial
         m_dwState |= F_PAUSING;
         Refresh();
      }
   }
}


//=== Resume ==========================================================================================================
//
//    If a trial sequence is currently paused, then resume sequencing.  We go ahead and start the next trial right away
//    unless the IsSeqWaiting() flag set, in which case sequencing will resume when the waiting period expires.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialMode::Resume()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::TrialMode) && IsSeqPaused() )
   {
      // these parameters can be altered while trial sequence is paused, so make sure they're up-to-date!
      m_seq.SetStartingPosH( m_pParmsDlg->GetStartingTgtPosH() ); 
      m_seq.SetStartingPosV( m_pParmsDlg->GetStartingTgtPosV() );
      m_seq.SetTgtPosScale( m_pParmsDlg->GetTgtPosScale() );
      m_seq.SetTgtPosRotate( m_pParmsDlg->GetTgtPosRotation() );
      m_seq.SetTgtVelScale( m_pParmsDlg->GetTgtVelScale() );
      m_seq.SetTgtVelRotate( m_pParmsDlg->GetTgtVelRotation() );
      m_seq.SetChanCfgOverride( m_pParmsDlg->IsChanCfgOverrideEnabled() );
      m_seq.SetChanCfgOverrideKey( m_pParmsDlg->GetChanCfgOverride() );

      // these params can be changed while seq paused, so send them to MAESTRODRIVER before resuming!
      m_pRuntime->SetTransform(&m_seq);
      m_pRuntime->SetVStabSlidingWindow(m_pParmsDlg->GetVStabSlidingWindowLen());
      
      m_dwState &= ~(F_PAUSING|F_PAUSED);
      Refresh();

      if( !IsSeqWaiting() )
      {
         CString strDest = m_strShadowPath.IsEmpty() ? m_strTrialPath : m_strShadowPath;
         if( !m_pRuntime->StartTrial( &m_seq, strDest, m_pProtoDlg->IsSaveSpikes() ) )
         {
            m_dwState = 0;
            Refresh();
            ::MessageBeep( MB_ICONEXCLAMATION );
         }
      }
   }
}


//=== Abort ===========================================================================================================
//
//    Stop the trial sequencer IMMEDIATELY.  The currently running trial is stopped and the trial data is discarded.
//
//    It is possible that the currently running trial has just finished, but this fact has not yet been detected via
//    the frequently called Service() routine.  In this case, we merely invoke Halt() to prevent the next trial from
//    starting, then call Service() directly to properly account for the just-completed trial (for which data files
//    will have already been saved!).  It is also possible to abort during the intertrial delay period.  In this case,
//    we just invoke Halt() to disengage trial sequencing.
//
//    31mar2006:  Giving MaestroDRIVER 200ms to acknowledge CX_TR_ABORT.  When RMVideo targets were in use,
//    MaestroDRIVER can get hung up waiting for RMVideo to return to the idle state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialMode::Abort()
{
   ASSERT( m_pRuntime );
   if( (m_pRuntime->GetMode() == CCxRuntime::TrialMode) && IsSeqRunning() )
   {
      if( !m_pRuntime->IsTrialRunning() )
      {
         Halt();
         if( (m_dwState & F_RUNNINGMASK) != 0 ) Service();                       // in case a trial JUST finished.
      }
      else
      {
         DWORD dwCmd = CX_TR_ABORT;
         CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
         if( m_pRuntime->SendCommand( dwCmd, NULL,NULL,0,0,0,0, 200 ) )
            pApp->LogMessage( _T("User aborted trial sequence; current trial discarded."), TRUE );
         else
            pApp->LogMessage( _T("!! Problem occurred while trying to abort trial sequence !!") );
         m_dwState = 0;
         Refresh();
         m_pRuntime->StopTraces();                                               // flush data trace display
         m_pRuntime->StopEventStream();                                          // and digital event stream
      }
   }
}


//=== UpdateRPDistroTrial =============================================================================================
//
//    Helper method for Service().  Updates GUI after a "R/P Distro" trial is completed.
//
//    When a trial that uses the "R/P Distro" is finished, the CCxRPDistro object that encapsulates the response
//    distributions and reward windows for the trial must be updated IAW the results of the trial.  In addition, the
//    "R/P Distro" dialog page is updated to reflect the changes in that CCxRPDistro object.
//
//    NOTE: The trial's CCxRPDistro object is updated even it the trial did not run to completion -- as long as it got
//    past the special segment during which the behavioral response is measured.
//
//    ARGS:       wKey  -- [in] key of the trial just completed.
//                dwRes -- [in] result flags for the trial.
//    RETURNS:    NONE.
//
VOID CCxTrialMode::UpdateRPDistroTrial(WORD wKey, DWORD dwRes)
{
   if( (dwRes & CX_FT_GOTRPDRESP) == CX_FT_GOTRPDRESP )
   {
      // for safety's sake, make sure the trial key still references an existing "R/P Distro" trial!
      CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();
      if( !pDoc->ObjExists(wKey) ) return;
      if( pDoc->GetObjType(wKey) != CX_TRIAL ) return;
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(wKey);
      CCxRPDistro* pDistro = pTrial->GetRPDistro();
      if( pDistro == NULL ) return;

      // update the trial's private CCxRPDistro object, then update the "R/P Distro" dialog page
      pDistro->AddSample( m_pRuntime->GetRPDistroBehavResp() );
      m_pRPDistroDlg->OnTrialDone( wKey );
   }
}

