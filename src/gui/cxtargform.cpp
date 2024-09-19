//=====================================================================================================================
//
// cxtargform.cpp : Implementation of class CCxTargForm.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxTargForm is a dialog-like form view that manages a set of input controls for displaying and adjusting any
// modifiable parameters associated with a particular MAESTRO target.  Encapsulated by the CCxTarget data class, a
// "target" represents some kind of stimulus that is presented to the subject in a MAESTRO experiment; see CCxTarget
// implementation file for details.  Currently, there are two kinds of user-defined, configurable "video" targets:
// "XY scope" (CX_XYTARG) and "remote Maestro framebuffer video", or "RMVideo", (CX_RMVTARG) targets.  The former
// target class is displayed on an XY oscilloscope driven by the outputs of an in-house "dotter" board which, in
// turn, is controlled by a DSP card.  The latter target class is displayed on a color CRT.  Target animation is
// handled by an OpenGL application, RMVideo, that runs on a separate Linux workstation (RMVideo replaced our old
// VSG2/4 framebuffer video card in Maestro v2.0).  In addition, MAESTRO offers several "predefined" non-video
// targets (CX_CHAIR, CX_FIBER1, etc) for which there are no configurable parameters.
//
// ==> Construction of form; layout of controls.
// The form's layout is defined as a dialog template resource IDD_TARGFORM.  Since we supply this ID in the default
// constructor, the MFC framework handles the details of loading the template and creating the view.  Use the Dev
// Studio Resource Editor to review the layout of IDD_TARGFORM.
//    The integer resource IDs below must represent a contiguous range of values so that we can use ON_CONTROL_RANGE
// in the message map.
//       (1) IDC_TARGF_TYPE...IDC_TARGF_SHAPE ==> The target type & aperture dropdown combo box controls.
//       (2) IDC_TARGF_DOTLF_MS...IDC_TARGF_WRTSCRN ==> A pair of radio buttons controlling "dot life" units, a pair
//           selecting directional or speed noise for a "Random-dot Patch" RMVideo target, a third pair choosing
//           either sinewave or squarewave gratings, a check box to make grating components independent for an
//           RMVideo "plaid" target, a check box that causes grating orientation to track pattern velocity direction
//           for an RMVideo "grating" target, a button to select the algorithm for generating per-dot speed noise, 
//           three check boxes to toggle flags governing behavior of the RMVideo RMV_MOVIE target, a check box that
//           sets frame of reference (target center or screen center) for the RMVideo RMV_RANDOMDOTS target.
//       (3) IDC_TARGF_ORECTW...IDC_TARGF_FLICKDELAY ==> Numeric edit controls.
//       (4) IDC_TARGF_MVFOLDER, _MVFILE ==> Edit controls specifying media source file folder and filename.
//
// ==> Interactions with CCxTarget, CCxDoc, other MAESTRO views.
// CCxTargForm must query CCxDoc whenever it must obtain a ptr to the target parameter record, CCxTarget, for a
// particular target.  CCxDoc also provides access to the target's name string.  CCxTargForm then queries the CCxTarget
// object directly to check the target's hardware platform and to get or set the target's modifiable params.  See
// implementation file of CCxTarget for more information on the kinds of targets available in MAESTRO.  Whenever it
// makes a change to the loaded CCxTarget, CCxTargForm must mark the doc as modified via CDocument::SetModifiedFlag()
// and inform other views of the change via CDocument::UpdateAllViews(), passing an appropriate CCxViewHint (see
// cxviewhint.h) describing the change.  CCxTargForm must also respond to updates initiated by other views in its
// OnUpdate() handler.  In particular, when it receives a "display target" hint, it must load the definition of the
// specified target onto the form.  See CCxTargForm::OnUpdate() for details.
//
// Each of the MAESTRO "object definition forms" has been designed for use in a "tabbed window" -- in particular, the
// TTabWnd class that is part of the "Visual Framework" library (see CREDITS).  The MAESTRO main frame window (see
// CCxMainFrame) installs each form in one of the tab panes of a TTabWnd.  The caption of the tab pane reflects the
// name of the MAESTRO data object currently loaded on the form.  This tab window is NOT a view, which presents a
// technical problem:  how do we update the tab window when the name of a loaded object changes, or when the user
// selects a different type of object for viewing (which requires bringing a different tab pane to the front).  Our
// solution:  All of the object definition forms (CCxTrialForm, CCxTargForm, etc.) are derived from TVTabPane, a simple
// CFormView-derivative that provides methods for telling the parent TTabWnd to update a tab caption or bring a
// particular tab to the front of the tab window.  TVTabPane is a supplement to the Visual Framework (see VISUALFX.*).
//
// ==> Enabling/disabling controls IAW target hardware platform and type.
// Not all of the controls laid out on IDD_TARGFORM are applicable to all target categories.  The "predefined" targets
// have no modifiable parameters -- and so CCxTargForm does not enable them.  Modifiable targets come in one of two
// flavors, "XY Scope" and "RMVideo" targets -- referring to the display platforms on which these targets are realized.
// There are several available target "types" within each of these categories, and the set of applicable parameters
// changes with target type.  Thus, every time the user changes the target type, CCxTargForm updates the enable state
// of all child controls appropriately -- so the user can change only those controls that are relevant.
//
// ==> "Use grayscale" button.
// RMVideo targets can be in color or grayscale at the user's discretion (RGB colorspace).  Specifying a color
// requires separate entries for each of the R, G and B components.  If the user only wants "grayscale", then entering
// the same value in three different controls is inefficient.  The "Use grayscale" button eliminates this problem.  If
// grayscale is selected, CCxTargForm disables the controls for the G and B components, and the data entered for the R
// component is automatically copied for the G and B components.  This feature is only available for RMVideo targets,
// as color is irrelevant on the monochrome XY scope.
//
// ==> Changes to target definition are applied immediately; DDX not used.
// Any change to a target parameter is handled as soon as it occurs, rather than waiting for the user to press an
// "Apply" button.  If the change is unacceptable, it is automatically corrected in some way and the controls are
// updated to reflect the corrections made.  Since we must catch parameter changes as they occur, we have elected not
// to use MFC's Dialog Data Exchange techniques in our implementation.  The routines OnChange() and StuffControls()
// handle the exchange of data between the loaded target object (CCxTarget) and the child controls on the CCxTargForm.
//
// Detection of a parameter change is VERY simplistic; essentially, we just catch notifications (EN_KILLFOCUS,
// CBN_CLOSEUP, BN_CLICKED) which *suggest* that a change has occurred.  In response, CCxTargForm loads the values
// from the controls into a U_TGPARMS struct (see cxobj_ifc.h) and queries the CCxTarget obj to update its parameters
// accordingly.  CCxTarget checks the validity of the new parameter set and makes any corrections; if corrections were
// made, CCxTargForm refreshes the parameter controls with the corrected parameter values.
//
// ==> Subclassed edit controls restrict user input -- CNumEdit.
// Some target parameters have "hard" range restrictions.  Many of them can never be negative.  Most are floating-pt
// values of limited precision. Many are numeric values that are displayed in an edit control capable of accepting
// many other characters.  To prevent the user from entering nonsense data, we derived the CNumEdit class from CEdit
// as a configurable integer/FP numeric edit control.  However, because the edit controls are laid out on a dialog
// template resource, the MFC framework initially associates them with standard CEdit objects.  To get the CNumEdit
// functionality, we must SUBCLASS each of the edit controls.  This is done just before the view is displayed, in
// OnInitialUpdate().
//
//
// CREDITS:
// (1) The "Visual Framework" library, by Zoran M. Todorovic [09/30/2000, www.codeproject.com/dialog/visualfx.asp] --
// We use this library to implement the view framework that appears in the MAESTRO frame window's (CCxMainFrame) client
// area.  This framework makes more complex GUI layouts -- that would have been a bear to implement w/core MFC -- easy
// to build.  This form class is derived from TVTabPane, a class that I added to the Visual Framework.
//
//
// REVISION HISTORY:
// 03feb2000-- Created.  Initial layout of form includes the target list control, three buttons to add, copy or
//             delete a configurable target, and a combo box to select target's hardware platform.  Experimenting...
// 31mar2000-- Target parameter controls have now been laid out.  CExperimentDoc and CCxTarget interfaces changed
//             significantly.  Begun work on CCxTargForm to bring it up to date.  Just started adding member vars
//             for the new controls in CxTargForm.h.
// 05apr2000-- Decided to break up the CCxTargForm (the "Targets" tab) into two form views.  The CCxTgListForm view
//             manages the target list and handles operations like "add", "copy", "rename", and "delete".  The user
//             also uses this view to select a target for editing purposes.  The form view defined here (CCxTgDataForm)
//             manages the input controls for specifying target parameters.  The main reason for this division of
//             labor:  some targets are not modifiable, and I wanted to be able to hide the parameter controls in one
//             go rather than having to hide each individual control...
// 20apr2000-- Begun work on first draft...
// 28apr2000-- Completed first version.  Still need to build/test.
// 12may2000-- Edit controls on form are now subclassed to CNumEdit, restricting data entry to nonnegative integers or
//             floating-point numbers.  The edit controls are also now tied into the standard edit commands usually
//             available from the Edit menu.  So far, we've implemented the Cut, Copy, Paste, and Undo commands.
// 16may2000-- Not such a good idea to hide the form view!  The portion of the splitter that the view is supposed to
//             occupy is not properly redrawn when you do this.  SO, in revised scheme, we just disable the window
//             when the "current target" is not modifiable.
// 17may2000-- If we disable view, then controls don't get grayed out (although they cannot receive mouse/key input).
//             Decided to abandon the idea of hiding/disabling view.  Instead, we call EnableRelevantControls in all
//             situations to update the enable state of the controls on the form.
//          -- Sending WM_ENABLE message to a control does NOT enable/disable mouse/key input to that control; it only
//             gives the control a chance to change its appearance.  To enable/disable mouse/key input, we must use
//             ::EnableWindow() with the control's actual handle, or we can use CWnd::EnableWindow() if we've
//             subclassed the control to a CWnd or CWnd-derived object.
// 17aug2000-- Began COMPLETE REWORK to bring view in line w/ latest CNTRLX design scheme.  Class renamed CCxTargForm,
//             which is unrelated to the original CCxTargForm from 31mar2000!
// 18aug2000-- Completed REWORK, except we still need to catch any change to the ctrls and mark target as modified
//             whenever a change occurs.
// 23aug2000-- Added facility for detecting any changes to the parametric controls.  Basically, we just catch EN_CHANGE
//             notifications from the read-write edit controls, BN_CLICKED notifications from the radio buttons that
//             govern dot life units, and CBN_SELCHANGE notifications from the two combo box controls.  A change in
//             the grayscale mode is also assumed to imply a change in target parameters.  In any of these cases, we
//             call MarkAsModified() to set the "target modified" flag and enable the Apply/Undo buttons, if we have
//             not already done so.  The approach is not foolproof, but much easier than comparing the current state of
//             the tgt params against the "documented" state every time the user makes a change!  Note use of the
//             ON_CONTROL_RANGE macro -- so relevant controls must have resource IDs in a contiguous range.
// 02jan2001-- Updated OnApplyChanges() IAW new version of CCxTarget::SetParams().  We're now designing all CNTRLX
//             objects to auto-correct any invalid parameter values...
// 26jan2001-- Updated implementation IAW recent changes to CCxViewHint and cxobj_ifc.h.  Also, the read-only control
//             for the target name now reflects the target object's "full path name".
// 27mar2001-- Removed read-only target name field.  Full name of loaded target is now displayed in the tab caption of
//             the tab pane in which CCxTargForm is installed.  CCxTargForm is now derived from TVTabPane, which is
//             designed to be a child of the Visual Framework's tab window class, TTabWnd.  TVTabPane provides methods
//             for signaling the parent TTabWnd to bring a tab pane to the front or update its caption.
// 28mar2001-- Completed major revision to "instantly" apply any changes made by user to a target's definition, INSTEAD
//             of having the "Apply" and "Undo" buttons.  This brings CCxTargForm in line with the other existing
//             CNTRLX object definition forms, CCxTrialForm and CCxChannelForm.  DDX no longer used.
// 27sep2001-- Cosmetic:  CExperimentDoc --> CCxDoc.
// 13nov2001-- Bug fix:  In SDI apps, each view's OnInitialUpdate() is called not once, but each time a new document is
//             created/opened.  Control subclassing in OnInitialUpdate() should only be done once!  MFC documentation
//             suggests putting one-time init code in OnCreate() instead, but I'm not sure that will work for form
//             views based on dialog templates.  Instead, I just include a check to avoid repeated subclassing....
// 14feb2002-- Change in representation of XY tgt subtype RECTDOT.  Instead of specifying a standard bounding rect, we
//             interpret the rect height as the spacing between dots in the regularly spaced rect dot array.  To
//             reflect this change on the form, the static text label IDC_TARGF_HTLBL now contains two possible
//             strings:  "Spacing(deg)" if we're displaying params for a RECTDOT; "Height(deg)" otherwise.  See method
//             UpdateControls().
// 15feb2002-- Mod to support the two newest XY tgt subtypes, FLOWFIELD and ORIENTEDBAR.  Like RECTDOT, both of these
//             subtypes use one or more of the bounding rect parameters in a unique way.  We now dynamically adjust the
//             show/hide state as well as the text appearing in each of the four labels that accompany the bounding
//             rect widgets -- so that they reflect the meaning of the parameters specified in those widgets.
// 21feb2002-- Updating the loaded target in response to EN_CHANGE is not a good idea -- since EN_CHANGE is issued even
//             when the user is in the midst of typing in the edit control.  Respond to EN_KILLFOCUS instead.  As a
//             result, we no longer need to worry about ignoring the EN_CHANGE notification that is sent with
//             programmatic changes of the edit controls' contents.
// 07mar2002-- Instead of responding to CBN_SELCHANGE notification, we look for CBN_CLOSEUP.
//          -- EN_KILLFOCUS is sent by a control with the focus when it is disabled.  This invoked OnChange(), which
//             goes ahead and notifies document that a change occurred, even if it has not.  Modified OnChange() to
//             inform the doc/view framework only if the loaded target object is actually changed.
// 10jun2002-- After further investigation, decided to respond to CBN_SELCHANGE rather than CBN_CLOSEUP.  The former is
//             also sent when the user changes the selection with arrow or char keys (droplist closed), and we need to
//             catch these selection changes as well!
// 21jul2003-- Mod to support a new XY tgt subtype not found in cntrlxUNIX: NOISYDOTS.  This is implemented like
//             FCDOTLIFE, but it has one additional parameter reflected in the IDC_TARGF_RNGDIR edit control.
// 15mar2004-- UpdateTitle() replaced by overriding TVTabPane::UpdateCaption() instead.  Also, we no longer display the
//             full "pathname" of an object b/c it makes the tab text too long...
// 13oct2004-- Mod to support new XY tgt subtype COHERENTFC.  The IDC_TARGF_RNGDIR edit control and associated dynamic
//             label IDC_TARGF_RNGDIRLBL now serve to display/edit the dot direction range offset when the subtype is
//             NOISYDOTS, and to display/edit percent coherence when the subtype is COHERENTFC.
// 11apr2005-- Mods to support add'l parameter, the "dot direction update interval", for NOISYDOTS target.  Added
//             IDC_TARGF_RNGUPD edit control and associated label.  Parameter is in milliseconds and is stored in
//             XYPARMS.fInnerH.
// 07jan2006-- Mod to support a new XY tgt subtype NOISYSPEED, which is similar to NOISYDIR (formerly, NOISYDOTS). Like
//             NOISYDIR, this tgt type uses IDC_TARGF_RNGDIR and IDC_TARGF_RNGUPD and associated labels, which have
//             been generalized to apply to both tgt types.
// 23mar2006-- Major rework of form to introduce RMVideo targets, replacing the old VSG framebuffer video targets.
// 04apr2006-- Mods to support Gaussian window on RMVideo target types RMV_SPOT and _RANDOMDOTS.
//          -- Mods to expose changes in RMVTGTDEF to support elliptical Gaussian windows: RMVTGTDEF.fSigma is now a
//             2-element array.  IDC_TARGF_SIGMA is now IDC_TARGF_XSIGMA and exposes the first array element; new edit
//             control IDC_TARGF_YSIGMA exposes the second element.
// 07apr2006-- Mods to support separate color specs for the two grating cmpts of an RMV_PLAID: RMVTGTDEF.iRBGBMean and
//             iRGBCon are now 2-element arrays.  The second element of each array applies to the second grating of a
//             plaid target.
//          -- Added checkbox to expose the new flag RMV_F_INDEPGRATS in the RMVideo target definition.
// 19jun2006-- Mods to display/edit x,y offset of inner rectangle for XY scope RECTANNU target, given by new fields 
//             XYPARMS.fInnerX, .fInnerY.  We reuse IDC_TARGF_XSIGMA and _YSIGMA for this purpose, and the accompanying 
//             label (IDC_TARGF_STDEVLBL) is now changed dynamically.
// 07dec2006-- Adding support for target modification modes similar to what's available on CCxTrialForm.
// 31aug2007-- Mods to support specifying one of two possible algorithms for generating per-dot speed noise for the 
//             XYScope NOISYSPEED tgt and the RMVideo RMV_RANDOMDOTS tgt. Other minor changes to UI. Effective v2.1.3.
// 09sep2009-- Added new section of widgets, labelled "Movie", that contains the defining parameters unique to the new
//             RMVideo RMV_MOVIE target type. CCxTargForm modified to handle the new controls.
// 11sep2009-- Added IDC_TARGF_ORIENTADJ check box, corres to new flag RMV_F_ORIENTADJ, applicable only to RMV_GRATING.
// 04dec2009-- IDC_TARGF_ORIENTADJ now applicable to RMV_PLAID also. Also, it and IDC_TARGF_INDEPGRATS act as a 
//             mutually exclusive pair, because the two features are not compatible. Effective v2.5.1.
// 13jan2010-- Added IDC_TARGF_WRTSCRN check box, corres to new flag RMV_F_WRTSCREEN, applic. only to RMV_RANDOMDOTS.
// 24nov2014-- Mods to account for change in defn of RMVideo RMV_RANDOMDOTS target: now, RMVTGTDEF.iRGBCon[0] now
//             specifies the contrast. If C != 0, then half the dots in the patch are drawn in color M(1+C) and half
//             in M(1-C), where M=iRGBMean[0]. Affected methods: UpdateControls, IsGrayscale(). Effective v3.1.2.
// 11oct2016-- Mods to support new RMVideo target type, RMV_IMAGE. Effective v3.3.1.
// 05sep2017-- Fix compiler issues while compiling for 64 - bit Win 10 using VStudio 2017.
// 08may2019-- Mods to support new "flicker" feature for RMVideo targets. Added a "Flicker" control group to the 
// target dialog IDD_TARGFORM, containing the 3 numeric edit controls to display/edit the duration of the flicker's ON
// and OFF phases and its initial delay.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for MAESTRO

#include "cxobj_ifc.h"                       // MAESTRO object interface:  common constants and other defines
#include "cxmainframe.h"                     // CCxMainFrame -- Maestro's main frame window
#include "cxtarget.h"                        // CCxTarget -- encapsulating a MAESTRO target object
#include "cxviewhint.h"                      // the "hint" class used by all MAESTRO views
#include "cxtargform.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxTargForm, TVTabPane )


BEGIN_MESSAGE_MAP( CCxTargForm, TVTabPane )
   ON_CONTROL_RANGE( CBN_SELCHANGE, IDC_TARGF_TYPE, IDC_TARGF_SHAPE, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_TARGF_DOTLF_MS, IDC_TARGF_WRTSCRN, OnChange )
   ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_TARGF_ORECTW, IDC_TARGF_FLICKDELAY, OnChange)
   ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_TARGF_MVFOLDER, IDC_TARGF_MVFILE, OnChange)
   ON_COMMAND_RANGE( ID_TARGF_MODTHIS, ID_TARGF_MODSELECTED, OnChange )
   ON_BN_CLICKED( IDC_TARGF_GRAY, OnGrayscale )
   ON_BN_CLICKED( IDC_TARGF_MODMODE, OnChangeModMode )
   ON_UPDATE_COMMAND_UI_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnUpdateEditCommand )
   ON_COMMAND_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnEditCommand )
END_MESSAGE_MAP()


//=====================================================================================================================
// PRIVATE CONSTANTS & GLOBALS
//=====================================================================================================================
LPCTSTR CCxTargForm::strModifyModeDesc[] =
   {
      _T("Modify THIS Target (Alt+6)"),
      _T("Modify ALL Targets in Set (Alt+7)"),
      _T("Modify MATCHING Targets in Set (Alt+8)"),
      _T("Modify SELECTED Targets in Set (Alt+9)")
   };
const COLORREF CCxTargForm::CLR_WARNGLOBALMODE = RGB(255, 0, 0); 

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTargForm [constructor] =======================================================================================
//
//    Construct the target data form view.
//
//    Almost all the work is handled by the framework, which loads the form's layout from a MAESTRO resource whose
//    identifier is stored in CCxTargForm's private member var IDD.  However, we do need to initialize certain
//    variables that track the form's state.
//
CCxTargForm::CCxTargForm() : TVTabPane( CCxTargForm::IDD )
{
   m_bOneTimeInitsDone = FALSE;
   m_wKey = CX_NULLOBJ_KEY;         // initially, no target object is loaded on form
   m_pTarg = NULL;
   m_bGrayScale = FALSE;            // grayscale mode OFF
   m_bXYtypes = TRUE;               // XY target types will be loaded initially into the target type combo box
   m_modifyMode = ATOMIC;
}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnChange ========================================================================================================
//
//    Update a parameter of the loaded target object IAW a detected change in the associated control.  We handle
//    several different notifications here:
//       1) BN_CLICKED ==> User pressed pushb IDC_TARGF_SPDALG, toggled a check box [IDC_TARGF_INDEPGRATS, _ORIENTADJ, 
//          _MVREP, _MVPAUSE, _MVRATE, _WRTSCRN], or clicked one among three pairs of mutually exclusive radio buttons 
//          controlling "dot life" units [IDC_TARGF_DOTLF_MS, _DOTLF_DEG], directional vs speed noise [_NOISEDIR, 
//          _NOISESPEED], or sinewave vs squarewave gratings [IDC_TARGF_SINE, _SQUARE].
//       2) CBN_SELCHANGE ==> User MAY have changed the current selection from one of the dropdown combo boxes
//          specifying target type (IDC_TARGF_TYPE) or window aperture shape (IDC_TARGF_SHAPE).
//       3) EN_KILLFOCUS ==> One of the numeric edit controls on this form [IDC_TARGF_ORECTW .. IDC_TARGF_FLICKDELAY]
//          or one of the standard edit controls [IDC_TARGF_MVFOLDER or _MVFILE] has lost the keyboard focus, so check
//          to see if its contents have changed. Note that this will also be sent by a control that loses the focus b/c
//          it is about to be disabled (in this case, the contents have not changed!).
//       4) WM_COMMAND from keyboard accelerators ID_TARGF_MODTHIS .. ID_TARGF_MODSELECTED ==> Changes the target 
//          modification mode. No effect on current state of loaded target.
//    In each case, we update the loaded target record accordingly.  If the modified parameter is invalid (or causes
//    other parameter values to become invalid), the target object auto-corrects the parameter(s).  In this case, we
//    refresh all controls to ensure the corrections are reflected.  Also, if the target type is changed, we update the
//    appearance of all controls IAW the new target type.
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification.
//
//    RETURNS:    NONE
//
void CCxTargForm::OnChange( UINT id )
{
   // special case: changing target modification mode -- no target need be loaded. We just set the mode and update 
   // appearance of relevant PB accordingly.
   if(id >= ID_TARGF_MODTHIS && id <= ID_TARGF_MODSELECTED)
   {
      if(id == ID_TARGF_MODTHIS) m_modifyMode = ATOMIC;
      else if(id == ID_TARGF_MODALL) m_modifyMode = ALLTGTS;
      else if(id == ID_TARGF_MODMATCHING) m_modifyMode = MATCHTGTS;
      else m_modifyMode = SELTGTS;
   
      m_btnModMode.SetColor(::GetSysColor(COLOR_BTNTEXT),
      (m_modifyMode == ATOMIC) ? ::GetSysColor(COLOR_BTNFACE) : CLR_WARNGLOBALMODE);
      m_btnModMode.SetWindowText( strModifyModeDesc[m_modifyMode] );

      return;
   }

   if( m_pTarg == NULL ) return;                                     // if no target loaded, ignore

   WORD wType = m_pTarg->DataType();                                 // check object data type...
   if( (wType != CX_XYTARG) && (wType != CX_RMVTARG) ) return;       // ...only XY and RMV tgts have adjustable params!
   BOOL bIsXY = BOOL(wType == CX_XYTARG);

   int iValue = 0;                                                   // get value in numeric edit ctrl just modified,
   float fValue = 0.0f;                                              // as an integer and as a float (if applicable)
   if(id >= IDC_TARGF_ORECTW && id <= IDC_TARGF_FLICKDELAY)
   {
      CNumEdit* pEdit = GetNumEdit( id );
      iValue = pEdit->AsInteger();
      fValue = pEdit->AsFloat();
   }

   BOOL bRestuff = FALSE;                                            // do we need to restuff controls on form?

   U_TGPARMS oldParms = m_tgParms;                                   // in case we propagate across matching targets!

   BOOL bChecked;
   int r, g, b, iGrat;
   CString str;
   switch( id )                                                      // update the parameter just modified...
   {
      case IDC_TARGF_DOTLF_MS :                                      //    target dot life units
      case IDC_TARGF_DOTLF_DEG :
         bChecked = BOOL(m_btnDotLifeMS.GetCheck()==1);
         if( bIsXY )
            m_tgParms.xy.iDotLfUnits = bChecked ? DOTLFINMS : DOTLFINDEG;
         else
         {
            if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_LIFEINMS;
            else m_tgParms.rmv.iFlags &= ~RMV_F_LIFEINMS;
         }
         bRestuff = TRUE;
         break;

      case IDC_TARGF_NOISEDIR :                                      //    target dot noise in direction or speed
      case IDC_TARGF_NOISESPEED :                                    //    (enabled ONLY for RMVideo targets)
         bChecked = BOOL(m_btnDotNoiseDir.GetCheck()==1);
         if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_DIRNOISE;
         else m_tgParms.rmv.iFlags &= ~RMV_F_DIRNOISE;
         bRestuff = TRUE;
         break;

      case IDC_TARGF_WRTSCRN :                                       //    toggle frame of reference for pattern
         if(!bIsXY)                                                  //    motion of RMV_RANDOMDOTS only
         {
            bChecked = BOOL(m_btnWRTScreen.GetCheck()==1);
            if(bChecked) m_tgParms.rmv.iFlags |= RMV_F_WRTSCREEN;
            else m_tgParms.rmv.iFlags &= ~RMV_F_WRTSCREEN;
         }
         break;
         
      case IDC_TARGF_SPDALG :                                        //    toggle dot speed noise algorithm
         if(bIsXY)
            m_tgParms.xy.fInnerX = (m_tgParms.xy.fInnerX != 0.0f) ? 0.0f : 1.0f;
         else if((m_tgParms.rmv.iFlags & RMV_F_SPDLOG2) != 0)
            m_tgParms.rmv.iFlags &= ~RMV_F_SPDLOG2;
         else
            m_tgParms.rmv.iFlags |= RMV_F_SPDLOG2;
         bRestuff = TRUE;
         break;
      
      case IDC_TARGF_SINE :                                          //    grating is sinewave or squarewave
      case IDC_TARGF_SQUARE :                                        //    (enabled ONLY for RMVideo targets)
         bChecked = BOOL(m_btnSquarewave.GetCheck()==1);
         if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_ISSQUARE;
         else m_tgParms.rmv.iFlags &= ~RMV_F_ISSQUARE;
         break;

      case IDC_TARGF_INDEPGRATS :                                    //    toggle "Independent gratings?" checkbox
         bChecked = BOOL(m_btnIndepGrats.GetCheck()==1);             //    (enabled ONLY for RMVideo plaid targets)
         if(bChecked) 
         {
            m_tgParms.rmv.iFlags |= RMV_F_INDEPGRATS;
            m_tgParms.rmv.iFlags &= ~RMV_F_ORIENTADJ;                //    _INDEPGRATS and _ORIENTADJ are mutually
            m_btnOrientAdj.SetCheck(0);                              //    exclusive
         }
         else m_tgParms.rmv.iFlags &= ~RMV_F_INDEPGRATS;
         break;

      case IDC_TARGF_ORIENTADJ :                                     //    toggle "Orientation tracks drift vector?" checkbox
         bChecked = BOOL(m_btnOrientAdj.GetCheck()==1);              //    (enabled for RMVideo grating/plaid targets)
         if(bChecked) 
         {
            m_tgParms.rmv.iFlags |= RMV_F_ORIENTADJ;
            if(m_btnIndepGrats.IsWindowEnabled())                    //    _INDEPGRATS and _ORIENTADJ are mutually 
            {                                                        //    exclusive (plaid targets only)
               m_tgParms.rmv.iFlags &= ~RMV_F_INDEPGRATS; 
               m_btnIndepGrats.SetCheck(0); 
            }
         }
         else m_tgParms.rmv.iFlags &= ~RMV_F_ORIENTADJ;
         break;

      // toggle one of the 3 checkboxes controlling behavior of an RMVideo movie (RMV_MOVIE) target
      case IDC_TARGF_MVREP : 
         bChecked = BOOL(m_btnMovieRepeat.GetCheck()==1); 
         if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_REPEAT;
         else m_tgParms.rmv.iFlags &= ~RMV_F_REPEAT;
         break;
      case IDC_TARGF_MVPAUSE : 
         bChecked = BOOL(m_btnMoviePause.GetCheck()==1); 
         if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_PAUSEWHENOFF;
         else m_tgParms.rmv.iFlags &= ~RMV_F_PAUSEWHENOFF;
         break;
      case IDC_TARGF_MVRATE : 
         bChecked = BOOL(m_btnMovieRate.GetCheck()==1); 
         if( bChecked ) m_tgParms.rmv.iFlags |= RMV_F_ATDISPRATE;
         else m_tgParms.rmv.iFlags &= ~RMV_F_ATDISPRATE;
         break;

      // update the media folder or file name for an RMVideo movie or image target
      case IDC_TARGF_MVFOLDER :
         m_edMediaFolder.GetWindowText(str);
         ::strncpy_s(m_tgParms.rmv.strFolder, (LPCTSTR)str, 31);
         m_tgParms.rmv.strFolder[31] = '\0';
         break;
      case IDC_TARGF_MVFILE :
         m_edMediaFile.GetWindowText(str);
         ::strncpy_s(m_tgParms.rmv.strFile, (LPCTSTR)str, 31);
         m_tgParms.rmv.strFile[31] = '\0';
          break;
         
      case IDC_TARGF_TYPE :                                          //    target type
         iValue = m_cbType.GetCurSel();
         if( bIsXY ) m_tgParms.xy.type = iValue;
         else m_tgParms.rmv.iType = iValue;
         bRestuff = TRUE;
         break;

      case IDC_TARGF_SHAPE :                                         //    aperture shape (RMVideo tgts only)
         m_tgParms.rmv.iAperture = m_cbAperture.GetCurSel();
         bRestuff = TRUE;
         break;

      case IDC_TARGF_ORECTW :                                        //    outer rect W (FLOAT, XY or RMV)
         if( bIsXY ) m_tgParms.xy.fRectW = fValue;
         else m_tgParms.rmv.fOuterW = fValue;
         break;

      case IDC_TARGF_ORECTH :                                        //    outer rect H (FLOAT, XY or RMV)
         if( bIsXY ) m_tgParms.xy.fRectH = fValue;
         else m_tgParms.rmv.fOuterH = fValue;
         break;

      case IDC_TARGF_IRECTW :                                        //    inner rect W (FLOAT, XY or RMV)
         if( bIsXY ) m_tgParms.xy.fInnerW = fValue;
         else
         {                                                           //    for RMVideo RMV_BAR target, we use IRECTW
            if( m_tgParms.rmv.iType == RMV_BAR )                     //    to edit bar's "drift axis"
               m_tgParms.rmv.fDriftAxis[0] = fValue;
            else
               m_tgParms.rmv.fInnerW = fValue;
         }
         break;

      case IDC_TARGF_IRECTH :                                        //    inner rect H (FLOAT, XY or RMV)
         if( bIsXY ) m_tgParms.xy.fInnerH = fValue;
         else m_tgParms.rmv.fInnerH = fValue;
         break;

      case IDC_TARGF_REDMEAN :                                       //    mean intensity for R cmpt (INT, RMV only).
      case IDC_TARGF_REDMEAN2 :                                      //    (for 2nd grating of a plaid).
         iGrat = (id==IDC_TARGF_REDMEAN) ? 0 : 1;
         r = cMath::rangeLimit(iValue, 0, 255);
         if( m_bGrayScale )                                          //    copied to G&B axes in grayscale mode!
         {
            m_tgParms.rmv.iRGBMean[iGrat] = (r<<16) + (r<<8) + r;
            bRestuff = TRUE;
         }
         else
         {
            m_tgParms.rmv.iRGBMean[iGrat] = (m_tgParms.rmv.iRGBMean[iGrat] & 0x00FFFF00) + r;
            bRestuff = BOOL(r!=iValue);
         }
         break;

      case IDC_TARGF_GRNMEAN :                                       //    mean intensity for G cmpt (INT, RMV only)
      case IDC_TARGF_GRNMEAN2 :                                      //    (for 2nd grating of a plaid)
         iGrat = (id==IDC_TARGF_GRNMEAN) ? 0 : 1;
         r = m_tgParms.rmv.iRGBMean[iGrat] & 0x00FF;
         g = cMath::rangeLimit(iValue, 0, 255);
         b = (m_tgParms.rmv.iRGBMean[iGrat] >> 16) & 0x00FF;
         m_tgParms.rmv.iRGBMean[iGrat] = (b<<16) + (g<<8) + r;
         bRestuff = BOOL(g!=iValue);
         break;

      case IDC_TARGF_BLUMEAN :                                       //    mean intensity for B cmpt (INT, RMV only)
      case IDC_TARGF_BLUMEAN2 :                                      //    (for 2nd grating of a plaid)
         iGrat = (id==IDC_TARGF_BLUMEAN) ? 0 : 1;
         r = m_tgParms.rmv.iRGBMean[iGrat] & 0x00FF;
         g = (m_tgParms.rmv.iRGBMean[iGrat] >> 8) & 0x00FF;
         b = cMath::rangeLimit(iValue, 0, 255);
         m_tgParms.rmv.iRGBMean[iGrat] = (b<<16) + (g<<8) + r;
         bRestuff = BOOL(b!=iValue);
         break;

      case IDC_TARGF_REDCON :                                        //    analogously for contrast on RGB cmpts....
      case IDC_TARGF_REDCON2 :
         iGrat = (id==IDC_TARGF_REDCON) ? 0 : 1;
         r = cMath::rangeLimit(iValue, 0, 100);
         if( m_bGrayScale )                                          //    copied to G&B axes in grayscale mode!
         {
            m_tgParms.rmv.iRGBCon[iGrat] = (r<<16) + (r<<8) + r;
            bRestuff = TRUE;
         }
         else
         {
            m_tgParms.rmv.iRGBCon[iGrat] = (m_tgParms.rmv.iRGBCon[iGrat] & 0x00FFFF00) + r;
            bRestuff = BOOL(r!=iValue);
         }
         break;

      case IDC_TARGF_GRNCON :
      case IDC_TARGF_GRNCON2 :
         iGrat = (id==IDC_TARGF_GRNCON) ? 0 : 1;
         r = m_tgParms.rmv.iRGBCon[iGrat] & 0x00FF;
         g = cMath::rangeLimit(iValue, 0, 100);
         b = (m_tgParms.rmv.iRGBCon[iGrat] >> 16) & 0x00FF;
         m_tgParms.rmv.iRGBCon[iGrat] = (b<<16) + (g<<8) + r;
         bRestuff = BOOL(g!=iValue);
         break;

      case IDC_TARGF_BLUCON :
      case IDC_TARGF_BLUCON2 :
         iGrat = (id==IDC_TARGF_BLUCON) ? 0 : 1;
         r = m_tgParms.rmv.iRGBCon[iGrat] & 0x00FF;
         g = (m_tgParms.rmv.iRGBCon[iGrat] >> 8) & 0x00FF;
         b = cMath::rangeLimit(iValue, 0, 100);
         m_tgParms.rmv.iRGBCon[iGrat] = (b<<16) + (g<<8) + r;
         bRestuff = BOOL(b!=iValue);
         break;

      case IDC_TARGF_NDOTS :                                         //    #dots in a tgt pattern
         if( bIsXY ) m_tgParms.xy.ndots = iValue;
         else m_tgParms.rmv.nDots = iValue;
         break;

      case IDC_TARGF_DOTSZ :                                         //    size of a "dot" in pixels (RMVideo only)
         m_tgParms.rmv.nDotSize = iValue;
         break;

      case IDC_TARGF_DOTLIFE :                                       //    dot life in deg or msecs (FLOAT)
         if( bIsXY ) m_tgParms.xy.fDotLife = fValue;
         else m_tgParms.rmv.fDotLife = fValue;
         break;

      case IDC_TARGF_COHER :                                         //    % coherence (INT)
         if( bIsXY ) m_tgParms.xy.fInnerW = fValue;                  //    (awkward field usage!)
         else m_tgParms.rmv.iPctCoherent = iValue;
         break;

      case IDC_TARGF_NOISERNG :                                      //    noise range limit (INT)
         if( bIsXY ) m_tgParms.xy.fInnerW = fValue;
         else m_tgParms.rmv.iNoiseLimit = iValue;
         break;

      case IDC_TARGF_NOISEUPD :                                      //    noise update intv (INT)
         if( bIsXY ) m_tgParms.xy.fInnerH = fValue;                  //    (awkward field usage!)
         else m_tgParms.rmv.iNoiseUpdIntv = iValue;
         break;

      case IDC_TARGF_GRAT1_DA :                                      //    1st grating drift axis in deg CCW (FLOAT,
         m_tgParms.rmv.fDriftAxis[0] = fValue;                       //    RMV only)
         break;

      case IDC_TARGF_GRAT2_DA :                                      //    similarly for 2nd grating (FLOAT, RMV only)
         m_tgParms.rmv.fDriftAxis[1] = fValue;
         break;

      case IDC_TARGF_GRAT1_SF :                                      //    1st grating spatial freq in cyc/deg
         m_tgParms.rmv.fSpatialFreq[0] = fValue;                     //    (FLOAT, RMV only)
         break;

      case IDC_TARGF_GRAT2_SF :                                      //    similarly for 2nd grating (FLOAT, RMV only)
         m_tgParms.rmv.fSpatialFreq[1] = fValue;
         break;

      case IDC_TARGF_GRAT1_PH :                                      //    1st grating spatial phase in deg (FLOAT,
         m_tgParms.rmv.fGratPhase[0] = fValue;                       //    RMV only)
         break;

      case IDC_TARGF_GRAT2_PH :                                      //    similarly for 2nd grating (FLOAT, RMV only)
         m_tgParms.rmv.fGratPhase[1] = fValue;
         break;

      case IDC_TARGF_XSIGMA :                                        //    X std dev of Gaussian window in subtended
         if( bIsXY )                                                 //    deg for RMVTarget, or X offset of "hole" for 
            m_tgParms.xy.fInnerX = fValue;                           //    XY scope RECTANNU target only (FLOAT)
         else
            m_tgParms.rmv.fSigma[0] = fValue;
         break;

      case IDC_TARGF_YSIGMA :                                        //    Y std dev of Gaussian window in subtended
         if( bIsXY )                                                 //    deg for RMVTarget, or Y offset of "hole" for 
            m_tgParms.xy.fInnerY = fValue;                           //    XY scope RECTANNU target only (FLOAT)
         else
            m_tgParms.rmv.fSigma[1] = fValue;
         break;

      case IDC_TARGF_RANDSEED:                                       //    seed for random-dot gen. (INT, RMV only)
         m_tgParms.rmv.iSeed = iValue;
         break;

      case IDC_TARGF_FLICKON:                                        //    flicker ON duration (INT, RMV only)
         m_tgParms.rmv.iFlickerOn = iValue;
         break;

      case IDC_TARGF_FLICKOFF:                                       //    flicker OFF duration (INT, RMV only)
         m_tgParms.rmv.iFlickerOff = iValue;
         break;

      case IDC_TARGF_FLICKDELAY:                                     //    flicker initial delay (INT, RMV only)
         m_tgParms.rmv.iFlickerDelay = iValue;
         break;

      default :                                                      //    we should NEVER get here!
         TRACE0( "Bad ID in CCxTargForm::OnChange!\n" );
         break;
   }

   BOOL bChanged = FALSE;
   BOOL bOk = m_pTarg->SetParams( &m_tgParms, bChanged );            // update the target record w/ new param value

   if( bRestuff || !bOk ) StuffControls();                           // restuff if necessary, or if correction made
   if( id == IDC_TARGF_TYPE || id == IDC_TARGF_SHAPE ||              // changes in these controls may affect the
       id == IDC_TARGF_NOISEDIR || id == IDC_TARGF_NOISESPEED ||     // enable state of other ctrls, or the state of
       id == IDC_TARGF_SPDALG )                                      // changeable labels
      UpdateControls();

   if( bChanged )                                                    // if target record was changed, inform doc/view.
   {                                                                 // Also, propagate change IAW current modify mode. 
      CCxDoc* pDoc = GetDocument(); 
      pDoc->SetModifiedFlag();

      CCxViewHint vuHint( CXVH_MODOBJ, wType, m_wKey );
      pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );

      Propagate(id, oldParms); 
   }
}


//=== OnGrayscale =====================================================================================================
//
//    Response to user clicking the "Use grayscale" button, resource ID# IDC_TARGF_GRAY.
//
//    The grayscale button changes its state automatically.  Here we toggle the form view's "grayscale flag" and update
//    appearance of the color specification controls for the GRN and BLU axes.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxTargForm::OnGrayscale()
{
   ASSERT( m_pTarg );                                             // grayscale only applies to RMVideo tgts
   ASSERT( m_pTarg->DataType() == CX_RMVTARG );

   m_bGrayScale = !m_bGrayScale;                                  // toggle the grayscale flag

   if( m_bGrayScale )                                             // if grayscale turned on, copy R cmpt to the G and
   {                                                              // B cmpt ctrls, as needed...
      U_TGPARMS oldParms = m_tgParms;                             // need this to propagate change across matching tgts

      PRMVTGTDEF pRMV = &(m_tgParms.rmv);

      BOOL bChanged = FALSE;
      for( int i=0; i<2; i++ )
      {
         int r = pRMV->iRGBMean[i] & 0x00FF;
         int g = (pRMV->iRGBMean[i] >> 8) & 0x00FF;
         int b = (pRMV->iRGBMean[i] >> 16) & 0x00FF;
         if( g != r )
         {
            bChanged = TRUE;
            GetNumEdit( (i==0) ? IDC_TARGF_GRNMEAN : IDC_TARGF_GRNMEAN2 )->SetWindowText(r);
         }
         if( b != r )
         {
            bChanged = TRUE;
            GetNumEdit( (i==0) ? IDC_TARGF_BLUMEAN : IDC_TARGF_BLUMEAN2 )->SetWindowText(r);
         }
         pRMV->iRGBMean[i] = (r<<16) + (r<<8) + r;

         r = pRMV->iRGBCon[i] & 0x00FF;
         g = (pRMV->iRGBCon[i] >> 8) & 0x00FF;
         b = (pRMV->iRGBCon[i] >> 16) & 0x00FF;
         if( g != r )
         {
            bChanged = TRUE;
            GetNumEdit( (i==0) ? IDC_TARGF_GRNCON : IDC_TARGF_GRNCON2 )->SetWindowText(r);
         }
         if( b != r )
         {
            bChanged = TRUE;
            GetNumEdit( (i==0) ? IDC_TARGF_BLUCON : IDC_TARGF_BLUCON2 )->SetWindowText(r);
         }
         pRMV->iRGBCon[i] = (r<<16) + (r<<8) + r;
      }

      if( bChanged )
      {
         bChanged = FALSE;
         if( !m_pTarg->SetParams( &m_tgParms, bChanged ) )
            StuffControls();

         if( bChanged )
         {
            CCxDoc* pDoc = GetDocument();
            pDoc->SetModifiedFlag();
            CCxViewHint vuHint( CXVH_MODOBJ, CX_RMVTARG, m_wKey );
            pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );
            Propagate(IDC_TARGF_GRAY, oldParms);
         }
      }
   }

   UpdateControls();                                              // update the color spec ctrls' enable state
}

//=== OnChangeModMode ================================================================================================= 
//
//    Handler invoked when the IDC_TARGF_MODMODE pushbutton is clicked. This merely switches to the next target 
//    modification mode in the sequence: ATOMIC, MODALL, MODMATCHING, MODSELECTED, ATOMIC, ... It then updates the 
//    appearance of the pushbutton to reflect the name of the modification mode now in effect. The pushbutton's bkg is 
//    made red whenever one of the global modification modes (anything other than ATOMIC) is in effect.
//
//    ARGS:    NONE.
//    RETURNS: NONE.
//
void CCxTargForm::OnChangeModMode()
{
   if(m_modifyMode == ATOMIC) m_modifyMode = ALLTGTS;
   else if(m_modifyMode == ALLTGTS) m_modifyMode = MATCHTGTS;
   else if(m_modifyMode == MATCHTGTS) m_modifyMode = SELTGTS;
   else m_modifyMode = ATOMIC;

   m_btnModMode.SetColor(::GetSysColor(COLOR_BTNTEXT),
      (m_modifyMode == ATOMIC) ? ::GetSysColor(COLOR_BTNFACE) : CLR_WARNGLOBALMODE);
   m_btnModMode.SetWindowText( strModifyModeDesc[m_modifyMode] );
}

//=== OnUpdateEditCommand =============================================================================================
//
//    ON_UPDATE_COMMAND_UI_RANGE handler for the predefined ID_EDIT_*** commands:  Update enable state of selected
//    items in the app's Edit menu depending on the current state of the clipboard and the edit control that currently
//    has the focus on this form.  An edit control must currently have the focus for any of the items to be enabled.
//
//    NOTE that only some of the ID_EDIT_*** commands are actually implemented.
//
//    [TECH NOTE: Ptr returned by CWnd::GetFocus() may be temporary; if so, it is released during the thread's idle
//    time.  we don't have to worry about that here, since we don't save it...but what about multithreading effects??]
//
//    ARGS:       pCmdUI -- [in] represents UI item.
//
//    RETURNS:    NONE
//
void CCxTargForm::OnUpdateEditCommand( CCmdUI* pCmdUI )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a
   if ( (pFocusWnd == NULL) ||                                 // CEdit obj, then disable all Edit cmds -- no other
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CEdit) )) )       // controls on this form support text editing.
   {
      pCmdUI->Enable( FALSE );
      return;
   }

   CEdit *pEditC = (CEdit *) pFocusWnd;                       // obj w/focus is a CEdit, so we can safely cast ptr

   BOOL bEnable = FALSE;                                       // enable state of edit command depends on current
   int iStart = 0;                                             // state of the CEdit ctrl w/ the input focus:
   int iEnd = 0;
   switch( pCmdUI->m_nID )
   {
      case ID_EDIT_CUT :                                       // ...at least one char must be selected
      case ID_EDIT_COPY :
         pEditC->GetSel( iStart, iEnd );
         bEnable = (iStart != iEnd);
         break;
      case ID_EDIT_PASTE :                                     // ...there must be appropriate clipboard data avail
         bEnable = ::IsClipboardFormatAvailable( CF_TEXT );
         break;
      case ID_EDIT_UNDO :
         bEnable = pEditC->CanUndo();
         break;
      default :
         break;
   }
   pCmdUI->Enable( bEnable );

   return;
}


//=== OnEditCommand ===================================================================================================
//
//    ON_COMMAND_RANGE handler for the ID_EDIT_*** commands:  Update state of the focussed edit control on this form
//    IAW the edit command given.
//
//    NOTE that only some of the ID_EDIT_*** commands are actually implemented.
//
//    [TECH NOTE: Ptr returned by CWnd::GetFocus() may be temporary; if so, it is released during the thread's idle
//    time.  we don't have to worry about that here, since we don't save it...but what about multithreading effects??]
//
//    ARGS:       nID -- [in] edit command ID.
//
//    RETURNS:    NONE
//
void CCxTargForm::OnEditCommand( UINT nID )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a
   if ( (pFocusWnd == NULL) ||                                 // CEdit obj, do nothing
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CEdit) )) )
      return;

   CEdit *pEditC = (CEdit *) pFocusWnd;                        // obj w/focus is a CEdit, so we can safely cast ptr

   switch( nID )                                               // process the operation by calling the appropriate
   {                                                           // CEdit method...
      case ID_EDIT_CUT :   pEditC->Cut();    break;
      case ID_EDIT_COPY :  pEditC->Copy();   break;
      case ID_EDIT_PASTE : pEditC->Paste();  break;
      case ID_EDIT_UNDO :  pEditC->Undo();   break;
      default :                              break;
   }
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitialUpdate [base override] =================================================================================
//
//    This function is called by the SDI doc/view framework each time a new document is created/opened.  Here we take
//    care of both one-time inits and per-document inits; the one-time inits are only performed the first time that
//    this method is invoked.
//
//    When this view is initially created, there is no "current target", all of the parameter controls are created in
//    a visible and enabled state, the grayscale button is not checked, and the target type and target aperture combo 
//    box dropdown lists have no labels in them. Here we load the aperture type labels (unchanging), and we initially 
//    load the XYScope type strings into the target type dropdown list, since future updates expect that one set of 
//    labels or the other is loaded (see ReloadTargetTypes()). We set up our internal state variables to reflect the 
//    initialized state of the form. Then, since there is no "current target", we must disable all controls.
//
//    In order to tailor behavior of the edit ctrls on this form, each one is subclassed to a CNumEdit or CEdit object 
//    held privately by this form view. That subclassing is done here. Also, the various checkboxes and radio buttons
//    are subclassed to CButton controls, and the two combo boxes to CComboBox controls.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxTargForm::OnInitialUpdate()
{
   if( !m_bOneTimeInitsDone )                                        // ONE-TIME INITIALIZATIONS:
   {
      m_edCtrls[0].SubclassDlgItem( IDC_TARGF_ORECTW, this );        // subclass add edit ctrls to CNumEdit's and init
      m_edCtrls[0].SetFormat(FALSE, TRUE, 6, 2);                     // format constraints...
      m_edCtrls[1].SubclassDlgItem( IDC_TARGF_ORECTH, this );
      m_edCtrls[1].SetFormat(FALSE, TRUE, 6, 2);
      m_edCtrls[2].SubclassDlgItem( IDC_TARGF_IRECTW, this );
      m_edCtrls[2].SetFormat(FALSE, TRUE, 6, 2);
      m_edCtrls[3].SubclassDlgItem( IDC_TARGF_IRECTH, this );
      m_edCtrls[3].SetFormat(FALSE, TRUE, 6, 2);
      m_edCtrls[4].SubclassDlgItem( IDC_TARGF_REDMEAN, this );
      m_edCtrls[4].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[5].SubclassDlgItem( IDC_TARGF_GRNMEAN, this );
      m_edCtrls[5].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[6].SubclassDlgItem( IDC_TARGF_BLUMEAN, this );
      m_edCtrls[6].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[7].SubclassDlgItem( IDC_TARGF_REDCON, this );
      m_edCtrls[7].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[8].SubclassDlgItem( IDC_TARGF_GRNCON, this );
      m_edCtrls[8].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[9].SubclassDlgItem( IDC_TARGF_BLUCON, this );
      m_edCtrls[9].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[10].SubclassDlgItem( IDC_TARGF_NDOTS, this );
      m_edCtrls[10].SetFormat(TRUE, TRUE, 4, 1);
      m_edCtrls[11].SubclassDlgItem( IDC_TARGF_DOTSZ, this );
      m_edCtrls[11].SetFormat(TRUE, TRUE, 2, 1);
      m_edCtrls[12].SubclassDlgItem( IDC_TARGF_DOTLIFE, this );
      m_edCtrls[12].SetFormat(FALSE, TRUE, 5, 2);
      m_edCtrls[13].SubclassDlgItem( IDC_TARGF_COHER, this );
      m_edCtrls[13].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[14].SubclassDlgItem( IDC_TARGF_NOISERNG, this );
      m_edCtrls[14].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[15].SubclassDlgItem( IDC_TARGF_NOISEUPD, this );
      m_edCtrls[15].SetFormat(TRUE, TRUE, 4, 1);
      m_edCtrls[16].SubclassDlgItem( IDC_TARGF_GRAT1_DA, this );
      m_edCtrls[16].SetFormat(FALSE, FALSE, 7, 2);
      m_edCtrls[17].SubclassDlgItem( IDC_TARGF_GRAT2_DA, this );
      m_edCtrls[17].SetFormat(FALSE, FALSE, 7, 2);
      m_edCtrls[18].SubclassDlgItem( IDC_TARGF_GRAT1_SF, this );
      m_edCtrls[18].SetFormat(FALSE, TRUE, 5, 2);
      m_edCtrls[19].SubclassDlgItem( IDC_TARGF_GRAT2_SF, this );
      m_edCtrls[19].SetFormat(FALSE, TRUE, 5, 2);
      m_edCtrls[20].SubclassDlgItem( IDC_TARGF_GRAT1_PH, this );
      m_edCtrls[20].SetFormat(FALSE, FALSE, 7, 2);
      m_edCtrls[21].SubclassDlgItem( IDC_TARGF_GRAT2_PH, this );
      m_edCtrls[21].SetFormat(FALSE, FALSE, 7, 2);
      m_edCtrls[22].SubclassDlgItem( IDC_TARGF_XSIGMA, this );
      m_edCtrls[22].SetFormat(FALSE, TRUE, 5, 2);
      m_edCtrls[23].SubclassDlgItem( IDC_TARGF_YSIGMA, this );
      m_edCtrls[23].SetFormat(FALSE, TRUE, 5, 2);
      m_edCtrls[24].SubclassDlgItem( IDC_TARGF_RANDSEED, this );
      m_edCtrls[24].SetFormat(TRUE, TRUE, 9, 1);
      m_edCtrls[25].SubclassDlgItem( IDC_TARGF_REDMEAN2, this );
      m_edCtrls[25].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[26].SubclassDlgItem( IDC_TARGF_GRNMEAN2, this );
      m_edCtrls[26].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[27].SubclassDlgItem( IDC_TARGF_BLUMEAN2, this );
      m_edCtrls[27].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[28].SubclassDlgItem( IDC_TARGF_REDCON2, this );
      m_edCtrls[28].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[29].SubclassDlgItem( IDC_TARGF_GRNCON2, this );
      m_edCtrls[29].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[30].SubclassDlgItem(IDC_TARGF_BLUCON2, this);
      m_edCtrls[30].SetFormat(TRUE, TRUE, 3, 1);
      m_edCtrls[31].SubclassDlgItem(IDC_TARGF_FLICKON, this);
      m_edCtrls[31].SetFormat(TRUE, TRUE, 2, 1);
      m_edCtrls[32].SubclassDlgItem(IDC_TARGF_FLICKOFF, this);
      m_edCtrls[32].SetFormat(TRUE, TRUE, 2, 1);
      m_edCtrls[33].SubclassDlgItem(IDC_TARGF_FLICKDELAY, this);
      m_edCtrls[33].SetFormat(TRUE, TRUE, 2, 1);

      m_btnModMode.SubclassDlgItem(IDC_TARGF_MODMODE, this);         // PB for changing tgt modification mode
      m_btnGrayscale.SubclassDlgItem(IDC_TARGF_GRAY, this);          // check box: grayscale on/OFF
      m_btnDotLifeMS.SubclassDlgItem(IDC_TARGF_DOTLF_MS, this);      // radio btn pair: units for dotlife -- ms or deg
      m_btnDotLifeDeg.SubclassDlgItem(IDC_TARGF_DOTLF_DEG, this);
      m_btnDotNoiseDir.SubclassDlgItem(IDC_TARGF_NOISEDIR, this);    // radio btn pair: dot noise in direc or speed
      m_btnDotNoiseSpeed.SubclassDlgItem(IDC_TARGF_NOISESPEED, this);
      m_btnWRTScreen.SubclassDlgItem(IDC_TARGF_WRTSCRN, this);       // check box: dot pattern motion WRT screen?
      m_btnSinewave.SubclassDlgItem(IDC_TARGF_SINE, this);           // radio btn pair: sinewave or squarewave gratings
      m_btnSquarewave.SubclassDlgItem(IDC_TARGF_SQUARE, this);
      m_btnIndepGrats.SubclassDlgItem(IDC_TARGF_INDEPGRATS, this);   // check box: indep gratings in plaid?
      m_btnOrientAdj.SubclassDlgItem(IDC_TARGF_ORIENTADJ, this);     // check box: orientation tracks drift vector?
      m_btnSpdNoiseAlg.SubclassDlgItem(IDC_TARGF_SPDALG, this);      // PB to toggle per-dot speed noise algorithm
      
      // check boxes and standard edit controls associate with params unique to RMVideo "movie" or "image" targets
      m_btnMovieRepeat.SubclassDlgItem(IDC_TARGF_MVREP, this); 
      m_btnMoviePause.SubclassDlgItem(IDC_TARGF_MVPAUSE, this);
      m_btnMovieRate.SubclassDlgItem(IDC_TARGF_MVRATE, this);
      m_edMediaFolder.SubclassDlgItem(IDC_TARGF_MVFOLDER, (CWnd*) this );
      m_edMediaFolder.SetLimitText(RMV_MVF_LEN);
      m_edMediaFolder.SetWindowText("");
      m_edMediaFile.SubclassDlgItem(IDC_TARGF_MVFILE, (CWnd*) this );
      m_edMediaFile.SetLimitText(RMV_MVF_LEN);
      m_edMediaFile.SetWindowText("");

      m_cbType.SubclassDlgItem(IDC_TARGF_TYPE, this);                // target type: load initially w/ XY tgt type
      m_cbType.ModifyStyle( CBS_SORT, 0 );                           // names, unsorted (list index = type ID!)
      m_cbType.ResetContent();
      int i;
      for( i = 0; i < NUMXYTYPES; i++ )
         m_cbType.AddString( CCxTarget::XYTYPENAMES[i] );
      m_cbType.SetCurSel( 0 );

      m_cbAperture.SubclassDlgItem(IDC_TARGF_SHAPE, this);           // window aperture type: load aperture shape
      m_cbAperture.ModifyStyle( CBS_SORT, 0 );                       // names, unsorted (list index = shape ID!)
      m_cbAperture.ResetContent();
      for( i = 0; i < RMV_NUMTGTSHAPES; i++ )
         m_cbAperture.AddString( CCxTarget::RMVSHAPENAMES[i] );
      m_cbAperture.SetCurSel( 0 );

      m_wKey = CX_NULLOBJ_KEY;                                       // there's no target loaded into view
      m_bGrayScale = FALSE;                                          // grayscale mode initially off
      m_bXYtypes = TRUE;                                             // XY target type names loaded into combo box

      m_btnModMode.SetColor(::GetSysColor(COLOR_BTNTEXT),            // init appearance of tgt modification mode PB
         (m_modifyMode == ATOMIC) ? ::GetSysColor(COLOR_BTNFACE) : CLR_WARNGLOBALMODE);
      m_btnModMode.SetWindowText( strModifyModeDesc[m_modifyMode] );

      m_bOneTimeInitsDone = TRUE;                                    // do NOT repeat these inits again!
   }

   LoadTarget( CX_NULLOBJ_KEY );                                     // initially, no target loaded on form

   TVTabPane::OnInitialUpdate();                                     // always call the base class version!
}


//=== OnUpdate [base override] ========================================================================================
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    This view must respond to a number of different "signals" broadcast by other views attached to the CCxDoc obj:
//       CXVH_DSPOBJ:   May need to load a different target definition onto the form.  Ignore if we get this for one
//                      of the predefined targets, which are not modifiable.
//       CXVH_MODOBJ:   If another view modifies a MAESTRO object, it may send this hint.  If the currently loaded
//                      target was the object modified, then we must reload it to ensure our view is up-to-date.
//       CXVH_MOVOBJ,
//       CXVH_NAMOBJ:   This signal is sent whenever a MAESTRO object is moved or renamed.  If the currently loaded tgt
//                      was affected, we update the assoc. tab pane caption to reflect tgt's new "full path" name.
//       CXVH_DELOBJ,
//       CXVH_CLRUSR:   If the currently loaded target is deleted, then the view must be reset.
//
//    NOTES:
//    (1) Whenever a hint is NOT provided, we only call the base class -- to handle lower-level update tasks.
//
//    ARGS:       pSender  -- [in] view which initiated the update
//                lHint    -- [in] an integer-valued hint (not used by MAESTRO views)
//                pHint    -- [in] if the initiating view provides a hint, this should be a CCxViewHint object.
//
//    RETURNS:    NONE
//
void CCxTargForm::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   if( pHint == NULL )                                   // no hint provided -- just call base class
   {
      TVTabPane::OnUpdate( pSender, lHint, pHint );
      return;
   }

   CCxViewHint* pVuHint = (CCxViewHint*)pHint;           // cast provided hint to the MAESTRO hint class
   switch( pVuHint->m_code )
   {
      case CXVH_DSPOBJ :                                 // display params of specified XYScope or RMVideo target
         if( (pVuHint->m_type == CX_XYTARG) ||           // (the predefined targets are not modifiable!)
             (pVuHint->m_type == CX_RMVTARG) )
         {
            BringToFront();                              // at least bring this view to front of tab window; if obj
            if( m_wKey != pVuHint->m_key )               // is diff from what's currently there, load the new obj
               LoadTarget( pVuHint->m_key );
         }
         break;

      case CXVH_MODOBJ :                                 // tgt params modified *outside* this view -- if specified tgt
         if ( m_wKey == pVuHint->m_key )                 // is currently displayed here, reload its params from doc
            LoadTarget( m_wKey );
         break;

      case CXVH_MOVOBJ :                                 // target renamed, or its location in obj tree changed, which
      case CXVH_NAMOBJ :                                 // might affect its "full path name"; update assoc. tab pane's
         if( (m_wKey == pVuHint->m_key) ||               // caption
             (pVuHint->m_code == CXVH_MOVOBJ) )
            UpdateCaption(NULL);
         break;

      case CXVH_DELOBJ :                                 // one or more targets deleted -- if currently displayed tgt
      case CXVH_CLRUSR :                                 // was the one deleted, then clear the form!
         if ( (m_wKey != CX_NULLOBJ_KEY) &&
              ((pVuHint->m_key == m_wKey) ||
               (!GetDocument()->ObjExists( m_wKey ))) )
            LoadTarget( CX_NULLOBJ_KEY );
         break;

      default :                                          // no response to any other hints...
         break;
   }
}



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump internal state vars associated with this target data form view.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxTargForm::Dump( CDumpContext& dc ) const
{
   TVTabPane::Dump( dc );

   if ( m_wKey == CX_NULLOBJ_KEY )
      dc << "No target displayed on form";
   else
   {
      CString info;
      CString moreInfo;
      info.Format( "Target key = %d\n", m_wKey );
      moreInfo.Format( "Grayscale flag = %d, XYtypes flag = %d\n", m_bGrayScale, m_bXYtypes );
      info += moreInfo;
      dc << info;
   }
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate internal consistency of the target data form view.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxTargForm::AssertValid() const
{
   TVTabPane::AssertValid();

   if( m_wKey == CX_NULLOBJ_KEY )
   {
      ASSERT( m_bGrayScale == FALSE );
   }
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== LoadTarget ======================================================================================================
//
//    Load specified target obj into the form view, updating the form's internal state vars and appearance accordingly.
//    If no target is specified (CX_NULLOBJ_KEY), the form is cleared.
//
//    24mar2006:  We no longer hide any controls, just enable/disable them.  But I think the problem below still
//    applies, because a disabled control cannot have the focus any more than a hidden control can...
//    BUG:  The fact that we reuse certain fields in the XYPARMS struct to store different quantities for different
//    target types (see description of XYPARMS struct in CX_OBJIFC.H) introduces subtle bugs.  If you enter a value
//    in an edit control and then display a different target type for which that control is hidden, the EN_KILLFOCUS
//    notification gets sent twice:  once when you double-click in the object tree to load a different target, and
//    again when you click in a visible control in the newly loaded and updated CCxTargForm.  Here's an example where
//    this causes a problem:  Suppose you change the drift axis of an ORIENTEDBAR tgt, then double-click on a NOISYDIR
//    target to load its definition onto the form.  The NOISYDIR "noise offset range" is stored in the same field
//    as the ORIENTEDBAR's drift axis (XYPARMS.fInnerW), even though it is displayed in a different control.  When the
//    NOISYDIR target is loaded, the control displaying drift axis (IDC_TARGF_IRECTW) is hidden -- BUT it apparently
//    keeps the focus.  So here's what happens.  When you double-click to load the NOISYDIR target, IDC_TARGF_IRECTW
//    sends EN_KILLFOCUS and the drift axis is updated for the ORIENTEDBAR target as it should be -- see OnChange().
//    Then the NOISYDIR target is loaded, which hides IDC_TARGF_IRECTW.  When the user clicks on a visible control in
//    the target form, the hidden control issues EN_KILLFOCUS *again*.  It still has the value entered earlier, and
//    OnChange() calls CCxTarget::SetParams() with that value placed in XYPARMS.fInnerW -- so whatever the drift axis
//    value was entered earlier, that becomes the "noise offset range".
//    SOLUTION:  Before loading a target onto the form, we nullify the old target pointer, put the focus on the
//    target type control (which is visible always, except when no target is loaded on form), and then load the new
//    target.  Then, if EN_KILLFOCUS gets issued by a hidden control as described above, it will have no effect (when
//    no target is loaded on the form, EN_KILLFOCUS is ignored!).
//    LONG-TERM SOLUTION:  DON'T reuse fields in XYPARMS!!
//
//    ARGS:       key   -- [in] the key of target obj (CX_NULLOBJ_KEY to clear form).
//
//    RETURNS:    NONE
//
VOID CCxTargForm::LoadTarget( const WORD key )
{
   CCxDoc* pDoc = GetDocument();                               // get reference to attached doc

   m_wKey = key;                                               // unique key of target to be displayed on form
   m_bGrayScale = FALSE;                                       // reset grayscale mode
   CString typeLabel = _T("");                                 // label for tgt type indicates platform: XY or RMV

   if( m_wKey == CX_NULLOBJ_KEY )                              // no target loaded; ctrls will be disabled
      m_pTarg = NULL;
   else                                                        // prepare to display a new target record...
   {
      m_pTarg = NULL;                                          // see description of BUG fix in function header
      GetDlgItem( IDC_TARGF_TYPE )->SetFocus();

      m_pTarg = (CCxTarget*) pDoc->GetObject( m_wKey );        // ...get ptr to target record
      ASSERT( m_pTarg );
      ASSERT_KINDOF( CCxTarget, m_pTarg );
      ASSERT( m_pTarg->IsModifiable() );                       // we only load modifiable targets!

      if(m_pTarg->DataType() == CX_XYTARG)                     // ...label for tgt type reflects display platform
         typeLabel = _T("XYScope Type");
      else
         typeLabel = _T("RMVideo Type");

      m_pTarg->GetParams( &m_tgParms );                        //    get a copy of the current param values
      m_bGrayScale = IsGrayscale(m_pTarg);                     //    are values consistent with grayscale mode?
   }

   SetDlgItemText(IDC_TARGF_TYPELBL, typeLabel);               // stuff target type label

   m_btnGrayscale.SetCheck( m_bGrayScale ? 1:0 );              // check/uncheck grayscale button as appropriate

   UpdateControls();                                           // update appearance of controls
   StuffControls();                                            // stuff modifiable params into relevant controls
   UpdateCaption(NULL);                                        // update caption of assoc tab pane w/ target name
}


//=== UpdateCaption [TVTabPane override] ==============================================================================
//
//    Update the caption of the tab item associated with this tab pane.  If a NULL argument is provided, the method
//    will use the name of the object currently loaded; if no obj is loaded, the placeholder title "Target" is used.
//
//    ARGS:       szCaption   -- [in] desired caption for this tab. Use NULL to get automated caption.
//
//    RETURNS:    NONE.
//
VOID CCxTargForm::UpdateCaption( LPCTSTR szCaption )
{
   CString strTitle;
   if( szCaption != NULL )
      strTitle = szCaption;
   else
      strTitle = (m_wKey!=CX_NULLOBJ_KEY) ? GetDocument()->GetObjName( m_wKey ) : _T("Target");
   TVTabPane::UpdateCaption( strTitle );
}


//=== StuffControls ===================================================================================================
//
//    Load current target parameters into the associated controls on this form view.  This method only "stuffs" those
//    parameters which are relevant, depending upon the target category (XY scope or RMVideo) and the specific target
//    type (e.g., RECTDOT, CENTER, etc...).
//
//    The RECTDOT, FLOWFIELD, ORIENTEDBAR, NOISYDIR and NOISYSPEED XY target subtypes use one or more of the bounding
//    rect parameters in non-intuitive ways.  See CXOBJ_IFC.H for the details.
//
//    The method assumes that the internal copy of target parameters is "in sync" with the currently loaded tgt object.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxTargForm::StuffControls()
{
   if( m_pTarg == NULL) return;                                                     // no tgt
   ASSERT( m_pTarg->DataType() == CX_XYTARG || m_pTarg->DataType() == CX_RMVTARG );

   BOOL bIsXY = BOOL(m_pTarg->DataType() == CX_XYTARG);

   if( bIsXY )                                                                      // LOAD AN XYSCOPE TARGET:
   {
      PXYPARMS pXY = &(m_tgParms.xy);
      m_cbType.SetCurSel( pXY->type );                                              // target subtype
      GetNumEdit(IDC_TARGF_NDOTS)->SetWindowText( pXY->ndots );                     // # of dots in target pattern

      GetNumEdit(IDC_TARGF_ORECTW)->SetWindowText( pXY->fRectW );                   // outer bounding rect; FLOWFIELD
      if( pXY->type != FLOWFIELD )                                                  // only uses rectW for outer radius
         GetNumEdit(IDC_TARGF_ORECTH)->SetWindowText( pXY->fRectH );

      if( pXY->type == RECTANNU || pXY->type == FLOWFIELD ||                        // inner rectW used by RECTANNU, as
          pXY->type == ORIENTEDBAR )                                                // inner radius by FLOWFIELD, and
         GetNumEdit(IDC_TARGF_IRECTW)->SetWindowText( pXY->fInnerW );               // as drift axis by ORIENTEDBAR
      if( pXY->type == RECTANNU )                                                   // inner rectH -- RECTANNU only
         GetNumEdit(IDC_TARGF_IRECTH)->SetWindowText( pXY->fInnerH );

      if( pXY->type == COHERENTFC )                                                 // percent coherence
         GetNumEdit(IDC_TARGF_COHER)->SetWindowText( pXY->fInnerW );

      if( pXY->type==NOISYDIR || pXY->type==NOISYSPEED )                            // noise offset and upd intv for
      {                                                                             // NOISYDIR/SPEED
         GetNumEdit(IDC_TARGF_NOISERNG)->SetWindowText( pXY->fInnerW );
         GetNumEdit(IDC_TARGF_NOISEUPD)->SetWindowText( pXY->fInnerH );

         m_btnDotNoiseDir.SetCheck( (pXY->type==NOISYDIR) ? 1:0 );                  // this radio btn pair is disabled,
         m_btnDotNoiseSpeed.SetCheck( (pXY->type==NOISYDIR) ? 0:1 );                // but it should be consistent.
      }

      // for XYScope targets, dot pattern motion is always specified WRT screen. This check box will be disabled.
      m_btnWRTScreen.SetCheck(1); 
      
      if( pXY->type==FCDOTLIFE || pXY->type==NOISYDIR || pXY->type==NOISYSPEED )    // finite dotlife tgts only:
      {
         CNumEdit* pDotLife = GetNumEdit( IDC_TARGF_DOTLIFE );                      // dyn. change format of dotlife
         if( pXY->iDotLfUnits == DOTLFINMS )                                        // ctrl IAW selected units...
            pDotLife->SetFormat( TRUE, TRUE, 5, 1 );                                //    msec: must be nonneg int
         else
            pDotLife->SetFormat( FALSE, TRUE, 5, 2 );                               //    deg: must be nonneg float
         pDotLife->SetWindowText( pXY->fDotLife );
         m_btnDotLifeMS.SetCheck( (pXY->iDotLfUnits==DOTLFINMS) ? 1:0 );            // we update both radio btns in
         m_btnDotLifeDeg.SetCheck( (pXY->iDotLfUnits==DOTLFINMS) ? 0:1 );           // pair b/c Windows doesn't!
      }

      if( pXY->type==RECTANNU )                                                     // RECTANNU only: x,y offset of
      {                                                                             // "hole" rel to tgt center
         GetNumEdit(IDC_TARGF_XSIGMA)->SetFormat(FALSE,FALSE,6,2);                  // must allow for negative values! 
         GetNumEdit(IDC_TARGF_YSIGMA)->SetFormat(FALSE,FALSE,6,2);                  // 
         GetNumEdit(IDC_TARGF_XSIGMA)->SetWindowText( pXY->fInnerX );
         GetNumEdit(IDC_TARGF_YSIGMA)->SetWindowText( pXY->fInnerY );
      }
   }
   else                                                                             // LOAD AN RMVIDEO TARGET:
   {                                                                                // (We load all controls, but only
      PRMVTGTDEF pRMV = &(m_tgParms.rmv);                                           // a subset applies to tgt type!)
      int t = pRMV->iType;
      m_cbType.SetCurSel( t );                                                      // target subtype
      m_cbAperture.SetCurSel( pRMV->iAperture );                                    // window aperture

      BOOL bSet = BOOL((pRMV->iFlags & RMV_F_LIFEINMS) != 0);                       // the radio btn pairs...
      m_btnDotLifeMS.SetCheck( bSet ? 1:0 );
      m_btnDotLifeDeg.SetCheck( bSet ? 0:1 );

      bSet = BOOL((pRMV->iFlags & RMV_F_DIRNOISE) != 0);
      m_btnDotNoiseDir.SetCheck( bSet ? 1:0 );
      m_btnDotNoiseSpeed.SetCheck( bSet ? 0:1 );

      bSet = BOOL(t == RMV_RANDOMDOTS && (pRMV->iFlags & RMV_F_WRTSCREEN) != 0);    // dot pattern motion WRT screen?
      m_btnWRTScreen.SetCheck( bSet ? 1:0 );                                        // (RMV_RANDOMDOTS only)

      bSet = BOOL((pRMV->iFlags & RMV_F_ISSQUARE) != 0);
      m_btnSquarewave.SetCheck( bSet ? 1:0 );
      m_btnSinewave.SetCheck( bSet ? 0:1 );

      bSet = BOOL((t==RMV_PLAID) && (pRMV->iFlags & RMV_F_INDEPGRATS) != 0);        // "Independent gratings?" chk box
      m_btnIndepGrats.SetCheck( bSet ? 1:0 );

      bSet = BOOL((pRMV->iFlags & RMV_F_ORIENTADJ) != 0);                           // "Dynamic orientation?" chk box
      if(bSet && (t==RMV_PLAID) && (pRMV->iFlags & RMV_F_INDEPGRATS) != 0)          // for plaids, _INDEPGRATS and 
         bSet = false;                                                              // _ORIENTADJ are mutually exclusive
      m_btnOrientAdj.SetCheck( bSet ? 1:0 );

      GetNumEdit(IDC_TARGF_ORECTW)->SetWindowText(pRMV->fOuterW);                   // target window dimensions
      GetNumEdit(IDC_TARGF_ORECTH)->SetWindowText(pRMV->fOuterH);
      GetNumEdit(IDC_TARGF_IRECTW)->SetWindowText(                                  // (we use IRECTW for RMV_BAR's
         (t==RMV_BAR) ? pRMV->fDriftAxis[0] : pRMV->fInnerW);                       // "drift axis")
      GetNumEdit(IDC_TARGF_IRECTH)->SetWindowText(pRMV->fInnerH);
      GetNumEdit(IDC_TARGF_REDMEAN)->SetWindowText(pRMV->iRGBMean[0] & 0x00FF);     // mean RGB color, RGB contrast
      GetNumEdit(IDC_TARGF_GRNMEAN)->SetWindowText((pRMV->iRGBMean[0]>>8) & 0x00FF);
      GetNumEdit(IDC_TARGF_BLUMEAN)->SetWindowText((pRMV->iRGBMean[0]>>16) & 0x00FF);
      GetNumEdit(IDC_TARGF_REDCON)->SetWindowText(pRMV->iRGBCon[0] & 0x00FF);
      GetNumEdit(IDC_TARGF_GRNCON)->SetWindowText((pRMV->iRGBCon[0]>>8) & 0x00FF);
      GetNumEdit(IDC_TARGF_BLUCON)->SetWindowText((pRMV->iRGBCon[0]>>16) & 0x00FF);
      GetNumEdit(IDC_TARGF_REDMEAN2)->SetWindowText(pRMV->iRGBMean[1] & 0x00FF);
      GetNumEdit(IDC_TARGF_GRNMEAN2)->SetWindowText((pRMV->iRGBMean[1]>>8) & 0x00FF);
      GetNumEdit(IDC_TARGF_BLUMEAN2)->SetWindowText((pRMV->iRGBMean[1]>>16) & 0x00FF);
      GetNumEdit(IDC_TARGF_REDCON2)->SetWindowText(pRMV->iRGBCon[1] & 0x00FF);
      GetNumEdit(IDC_TARGF_GRNCON2)->SetWindowText((pRMV->iRGBCon[1]>>8) & 0x00FF);
      GetNumEdit(IDC_TARGF_BLUCON2)->SetWindowText((pRMV->iRGBCon[1]>>16) & 0x00FF);
      GetNumEdit(IDC_TARGF_NDOTS)->SetWindowText(pRMV->nDots);
      GetNumEdit(IDC_TARGF_DOTSZ)->SetWindowText(pRMV->nDotSize);

      CNumEdit* pEdit = GetNumEdit( IDC_TARGF_DOTLIFE );                            // dyn. change format of dotlife
      if( (pRMV->iFlags & RMV_F_LIFEINMS) != 0 )                                    // ctrl IAW selected units...
         pEdit->SetFormat( TRUE, TRUE, 5, 1 );                                      //    msec: must be nonneg int
      else
         pEdit->SetFormat( FALSE, TRUE, 5, 2 );                                     //    deg: must be nonneg float
      pEdit->SetWindowText(pRMV->fDotLife);

      GetNumEdit(IDC_TARGF_COHER)->SetWindowText(pRMV->iPctCoherent);
      GetNumEdit(IDC_TARGF_NOISERNG)->SetWindowText(pRMV->iNoiseLimit);
      GetNumEdit(IDC_TARGF_NOISEUPD)->SetWindowText(pRMV->iNoiseUpdIntv);
      GetNumEdit(IDC_TARGF_GRAT1_DA)->SetWindowText(                                // RMV_BAR's "drift axis" is NOT
         (t==RMV_BAR) ? 0.0f : pRMV->fDriftAxis[0] );                               // edited here!
      GetNumEdit(IDC_TARGF_GRAT2_DA)->SetWindowText( pRMV->fDriftAxis[1] );
      GetNumEdit(IDC_TARGF_GRAT1_SF)->SetWindowText( pRMV->fSpatialFreq[0] );
      GetNumEdit(IDC_TARGF_GRAT2_SF)->SetWindowText( pRMV->fSpatialFreq[1] );
      GetNumEdit(IDC_TARGF_GRAT1_PH)->SetWindowText( pRMV->fGratPhase[0] );
      GetNumEdit(IDC_TARGF_GRAT2_PH)->SetWindowText( pRMV->fGratPhase[1] );
      GetNumEdit(IDC_TARGF_XSIGMA)->SetFormat(FALSE,TRUE,5,2);                      // b/c they can display negative 
      GetNumEdit(IDC_TARGF_YSIGMA)->SetFormat(FALSE,TRUE,5,2);                      // numbers in another context!
      GetNumEdit(IDC_TARGF_XSIGMA)->SetWindowText( pRMV->fSigma[0] );
      GetNumEdit(IDC_TARGF_YSIGMA)->SetWindowText( pRMV->fSigma[1] );
      GetNumEdit(IDC_TARGF_RANDSEED)->SetWindowText( pRMV->iSeed );
      GetNumEdit(IDC_TARGF_FLICKON)->SetWindowText(pRMV->iFlickerOn);
      GetNumEdit(IDC_TARGF_FLICKOFF)->SetWindowText(pRMV->iFlickerOff);
      GetNumEdit(IDC_TARGF_FLICKDELAY)->SetWindowText(pRMV->iFlickerDelay);

      // controls unique to the RMVideo "movie" target type
      bSet = BOOL((pRMV->iFlags & RMV_F_REPEAT) != 0);
      m_btnMovieRepeat.SetCheck( bSet ? 1:0 );
      bSet = BOOL((pRMV->iFlags & RMV_F_PAUSEWHENOFF) != 0);
      m_btnMoviePause.SetCheck( bSet ? 1:0 );
      bSet = BOOL((pRMV->iFlags & RMV_F_ATDISPRATE) != 0);
      m_btnMovieRate.SetCheck( bSet ? 1:0 );
      
      m_edMediaFolder.SetWindowText((LPCTSTR) ((t==RMV_MOVIE || t==RMV_IMAGE) ? pRMV->strFolder : _T("")));
      m_edMediaFile.SetWindowText((LPCTSTR) ((t==RMV_MOVIE || t==RMV_IMAGE) ? pRMV->strFile : _T("")));
   }

}


//=== UpdateControls ==================================================================================================
//
//    Update the enabled state of all parameter controls on the target data form.  Which controls should be enabled
//    depends on whether the "current target" is realized on the XYScope or RMVideo, the target's type, and the state
//    of the "Use Grayscale" button (RMVideo targets only).
//
//    Some labels are dynamically changed to accurately reflect the usage of the corresponding widgets: IDC_TARGF_WLBL,
//    _HLBL, _ORLBL, _IRLBL, and _STDEVLBL. The IDC_TARGF_SPDALG button's text is dynamically changed.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxTargForm::UpdateControls()
{
   if( m_pTarg == NULL )                                             // all controls disabled if no tgt loaded on form!
   {
      m_cbType.EnableWindow( FALSE );
      m_cbAperture.EnableWindow( FALSE );
      m_btnGrayscale.EnableWindow( FALSE );
      m_btnDotLifeMS.EnableWindow( FALSE );
      m_btnDotLifeDeg.EnableWindow( FALSE );
      m_btnDotNoiseDir.EnableWindow( FALSE );
      m_btnDotNoiseSpeed.EnableWindow( FALSE );
      m_btnWRTScreen.EnableWindow(FALSE);
      m_btnSinewave.EnableWindow( FALSE );
      m_btnSquarewave.EnableWindow( FALSE );
      m_btnIndepGrats.EnableWindow( FALSE );
      m_btnOrientAdj.EnableWindow( FALSE );
      m_btnSpdNoiseAlg.EnableWindow( FALSE );
      for( int i=0; i<NUMTGEDITC; i++ )
         m_edCtrls[i].EnableWindow( FALSE );
      m_btnMovieRepeat.EnableWindow(FALSE);
      m_btnMoviePause.EnableWindow(FALSE);
      m_btnMovieRate.EnableWindow(FALSE);
      m_edMediaFolder.EnableWindow(FALSE);
      m_edMediaFile.EnableWindow(FALSE);
   }
   else if( m_pTarg->DataType() == CX_XYTARG )                       // current target is an XYScope target
   {
      PXYPARMS pXY = &(m_tgParms.xy);
      int t = pXY->type;

      m_cbType.EnableWindow( TRUE );
      m_cbAperture.EnableWindow( FALSE );
      m_btnGrayscale.EnableWindow( FALSE );

      BOOL usesDotLf = BOOL(t==FCDOTLIFE || t==NOISYDIR || t==NOISYSPEED);
      m_btnDotLifeMS.EnableWindow( usesDotLf );
      m_btnDotLifeDeg.EnableWindow( usesDotLf );

      m_btnDotNoiseDir.EnableWindow( FALSE );
      m_btnDotNoiseSpeed.EnableWindow( FALSE );
      m_btnWRTScreen.EnableWindow(FALSE);
      m_btnSinewave.EnableWindow( FALSE );
      m_btnSquarewave.EnableWindow( FALSE );
      m_btnIndepGrats.EnableWindow( FALSE );
      m_btnOrientAdj.EnableWindow( FALSE );

      m_btnSpdNoiseAlg.EnableWindow( t==NOISYSPEED );
      
      // disable all numeric edit controls initially, then enable the relatively few that apply...
      for(int i = 0; i<NUMTGEDITC; i++) m_edCtrls[i].EnableWindow(FALSE);
      GetNumEdit(IDC_TARGF_ORECTW)->EnableWindow(TRUE);
      GetNumEdit(IDC_TARGF_ORECTH)->EnableWindow(TRUE);
      GetNumEdit(IDC_TARGF_IRECTW)->EnableWindow(
         t==RECTANNU || t==FLOWFIELD || t==ORIENTEDBAR || t==NOISYDIR || t==NOISYSPEED || t==COHERENTFC );
      GetNumEdit(IDC_TARGF_IRECTH)->EnableWindow( t==RECTANNU || t==NOISYDIR || t==NOISYSPEED );
      GetNumEdit(IDC_TARGF_NDOTS)->EnableWindow(TRUE);
      GetNumEdit(IDC_TARGF_DOTLIFE)->EnableWindow(usesDotLf);
      GetNumEdit(IDC_TARGF_COHER)->EnableWindow(t==COHERENTFC);
      GetNumEdit(IDC_TARGF_NOISERNG)->EnableWindow(t==NOISYDIR || t==NOISYSPEED);
      GetNumEdit(IDC_TARGF_NOISEUPD)->EnableWindow(t==NOISYDIR || t==NOISYSPEED);
      GetNumEdit(IDC_TARGF_XSIGMA)->EnableWindow(t==RECTANNU);
      GetNumEdit(IDC_TARGF_YSIGMA)->EnableWindow(t==RECTANNU);
      
      m_btnMovieRepeat.EnableWindow(FALSE);
      m_btnMoviePause.EnableWindow(FALSE);
      m_btnMovieRate.EnableWindow(FALSE);
      m_edMediaFolder.EnableWindow(FALSE);
      m_edMediaFile.EnableWindow(FALSE);
   }
   else if( m_pTarg->DataType() == CX_RMVTARG )                      // current target is an RMVideo target
   {
      PRMVTGTDEF pRMV = &(m_tgParms.rmv);
      int t = pRMV->iType;

      m_cbType.EnableWindow( TRUE );
      m_cbAperture.EnableWindow( t==RMV_RANDOMDOTS || t==RMV_SPOT || t==RMV_GRATING || t==RMV_PLAID );
      m_btnGrayscale.EnableWindow( t!=RMV_MOVIE && t!=RMV_IMAGE );

      m_btnDotLifeMS.EnableWindow( t==RMV_RANDOMDOTS );
      m_btnDotLifeDeg.EnableWindow( t==RMV_RANDOMDOTS );

      m_btnDotNoiseDir.EnableWindow( t==RMV_RANDOMDOTS );
      m_btnDotNoiseSpeed.EnableWindow( t==RMV_RANDOMDOTS );
      m_btnWRTScreen.EnableWindow(t==RMV_RANDOMDOTS);
      m_btnSinewave.EnableWindow( t==RMV_GRATING || t==RMV_PLAID );
      m_btnSquarewave.EnableWindow( t==RMV_GRATING || t==RMV_PLAID );
      m_btnIndepGrats.EnableWindow( t==RMV_PLAID );
      m_btnOrientAdj.EnableWindow( t==RMV_GRATING || t==RMV_PLAID );

      m_btnSpdNoiseAlg.EnableWindow( t==RMV_RANDOMDOTS );
      
      GetNumEdit(IDC_TARGF_ORECTW)->EnableWindow( t!=RMV_POINT && t!=RMV_MOVIE && t!=RMV_IMAGE );
      GetNumEdit(IDC_TARGF_ORECTH)->EnableWindow( t!=RMV_POINT && t!=RMV_FLOWFIELD && t!=RMV_MOVIE && t!=RMV_IMAGE);

      BOOL useInnerW = BOOL(t==RMV_FLOWFIELD || t==RMV_BAR || 
            ((t!=RMV_POINT) && (t!=RMV_MOVIE) && (t!=RMV_IMAGE) && (pRMV->iAperture>RMV_OVAL)));
      BOOL useInnerH = BOOL(pRMV->iAperture>RMV_OVAL && t!=RMV_POINT && t!=RMV_FLOWFIELD && t!=RMV_BAR && 
            t!=RMV_MOVIE && t!=RMV_IMAGE);

      GetNumEdit(IDC_TARGF_IRECTW)->EnableWindow( useInnerW );
      GetNumEdit(IDC_TARGF_IRECTH)->EnableWindow( useInnerH );
      GetNumEdit(IDC_TARGF_REDMEAN)->EnableWindow(t!=RMV_MOVIE && t!=RMV_IMAGE);
      GetNumEdit(IDC_TARGF_GRNMEAN)->EnableWindow(t!=RMV_MOVIE && t!=RMV_IMAGE && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_BLUMEAN)->EnableWindow(t!=RMV_MOVIE && t!=RMV_IMAGE && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_REDCON)->EnableWindow((t==RMV_GRATING || t==RMV_PLAID || t==RMV_RANDOMDOTS));
      GetNumEdit(IDC_TARGF_GRNCON)->EnableWindow((t==RMV_GRATING || t==RMV_PLAID || t==RMV_RANDOMDOTS) && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_BLUCON)->EnableWindow((t==RMV_GRATING || t==RMV_PLAID || t==RMV_RANDOMDOTS) && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_REDMEAN2)->EnableWindow(t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRNMEAN2)->EnableWindow(t==RMV_PLAID && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_BLUMEAN2)->EnableWindow(t==RMV_PLAID && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_REDCON2)->EnableWindow(t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRNCON2)->EnableWindow(t==RMV_PLAID && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_BLUCON2)->EnableWindow(t==RMV_PLAID && !m_bGrayScale);
      GetNumEdit(IDC_TARGF_NDOTS)->EnableWindow(t==RMV_RANDOMDOTS || t==RMV_FLOWFIELD);
      GetNumEdit(IDC_TARGF_DOTSZ)->EnableWindow(t==RMV_RANDOMDOTS || t==RMV_FLOWFIELD || t==RMV_POINT);
      GetNumEdit(IDC_TARGF_DOTLIFE)->EnableWindow(t==RMV_RANDOMDOTS);
      GetNumEdit(IDC_TARGF_COHER)->EnableWindow(t==RMV_RANDOMDOTS);
      GetNumEdit(IDC_TARGF_NOISERNG)->EnableWindow(t==RMV_RANDOMDOTS);
      GetNumEdit(IDC_TARGF_NOISEUPD)->EnableWindow(t==RMV_RANDOMDOTS);
      GetNumEdit(IDC_TARGF_GRAT1_DA)->EnableWindow(t==RMV_GRATING || t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRAT2_DA)->EnableWindow(t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRAT1_SF)->EnableWindow(t==RMV_GRATING || t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRAT2_SF)->EnableWindow(t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRAT1_PH)->EnableWindow(t==RMV_GRATING || t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_GRAT2_PH)->EnableWindow(t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_XSIGMA)->EnableWindow(t==RMV_SPOT || t==RMV_RANDOMDOTS || t==RMV_GRATING || t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_YSIGMA)->EnableWindow(t==RMV_SPOT || t==RMV_RANDOMDOTS || t==RMV_GRATING || t==RMV_PLAID);
      GetNumEdit(IDC_TARGF_RANDSEED)->EnableWindow(t==RMV_RANDOMDOTS || t==RMV_FLOWFIELD);
      GetNumEdit(IDC_TARGF_FLICKON)->EnableWindow(TRUE);
      GetNumEdit(IDC_TARGF_FLICKOFF)->EnableWindow(TRUE);
      GetNumEdit(IDC_TARGF_FLICKDELAY)->EnableWindow(TRUE);

      m_btnMovieRepeat.EnableWindow(t==RMV_MOVIE);
      m_btnMoviePause.EnableWindow(t==RMV_MOVIE);
      m_btnMovieRate.EnableWindow(t==RMV_MOVIE);
      m_edMediaFolder.EnableWindow(t==RMV_MOVIE || t==RMV_IMAGE);
      m_edMediaFile.EnableWindow(t==RMV_MOVIE || t==RMV_IMAGE);
   }
   else                                                              // we should NEVER get here!
      ASSERT(FALSE);

   ReloadTargetTypes();                                              // reload set of strings appearing in the
                                                                     // tgt-type dropdown combo box, if necessary

   if( (m_pTarg != NULL) && m_pTarg->IsModifiable() )                // update text of changeable labels only if a
   {                                                                 // modifiable target is loaded...
      BOOL bIsXY = BOOL(m_pTarg->DataType()==CX_XYTARG);

      CString strLbl = _T("Width(deg)");
      if( (bIsXY && m_tgParms.xy.type==FLOWFIELD) || (!bIsXY && m_tgParms.rmv.iType==RMV_FLOWFIELD) )
         strLbl = _T("Radius(deg)");
      SetDlgItemText( IDC_TARGF_WLBL, strLbl );

      strLbl = _T("Height(deg)");
      if( bIsXY && (m_tgParms.xy.type == RECTDOT) ) strLbl = _T("Spacing(deg)");
      SetDlgItemText( IDC_TARGF_HLBL, strLbl );

      strLbl = _T("Outer");
      if( (bIsXY && m_tgParms.xy.type==ORIENTEDBAR) || (!bIsXY && m_tgParms.rmv.iType==RMV_BAR) )
         strLbl = _T("Bar Rect");
      SetDlgItemText( IDC_TARGF_ORLBL, strLbl );

      strLbl = _T("Inner");
      if( (bIsXY && m_tgParms.xy.type==ORIENTEDBAR) || (!bIsXY && m_tgParms.rmv.iType==RMV_BAR) )
         strLbl = _T("Drift Axis");
      SetDlgItemText( IDC_TARGF_IRLBL, strLbl );

      strLbl = bIsXY ? _T("X,Y offset of hole (deg):") : _T("Gaussian std dev in X,Y (deg):");
      SetDlgItemText( IDC_TARGF_STDEVLBL, strLbl );
      
      // text of this button reflects the per-dot speed noise algorithm chosen. Button disabled when not applicable.
      strLbl = _T("additive");
      if((bIsXY && m_tgParms.xy.type == NOISYSPEED && m_tgParms.xy.fInnerX != 0.0f) ||
         ((!bIsXY) && m_tgParms.rmv.iType == RMV_RANDOMDOTS && ((m_tgParms.rmv.iFlags & RMV_F_SPDLOG2) != 0)))
         strLbl = _T("* 2^N");
      m_btnSpdNoiseAlg.SetWindowText(strLbl);
   }
}


//=== ReloadTargetTypes ===============================================================================================
//
//    The "target type" dropdown list (resource IDC_TARGF_TYPE ) must reflect the target type names associated with
//    the target display platform.  There are currently two platforms that support multiple target types, the XY scope
//    and framebuffer video displays.  The string names associated with each target type are found in static string
//    arrays in CCxTarget, indexed by target type ID.
//
//    An internal flag indicates which set of type strings is currently loaded.  This flag and the current target's
//    data type are checked to determine whether or not a reload is necessary.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxTargForm::ReloadTargetTypes()
{
   if( m_pTarg == NULL || !m_pTarg->IsModifiable() ) return;      // don't bother in this case

   WORD dspType = m_pTarg->DataType();
   if( m_bXYtypes && (dspType==CX_RMVTARG) )                      // switch from XY --> RMV target types
   {
      m_cbType.ResetContent();
      for( int i = 0; i < RMV_NUMTGTTYPES; i++ )
         m_cbType.AddString( CCxTarget::RMVTYPENAMES[i] );
      m_bXYtypes = FALSE;
   }
   else if( (!m_bXYtypes) && (dspType==CX_XYTARG) )               // switch from RMV --> XY target types
   {
      m_cbType.ResetContent();
      for( int i = 0; i < NUMXYTYPES; i++ )
         m_cbType.AddString( CCxTarget::XYTYPENAMES[i] );
      m_bXYtypes = TRUE;
   }
}


//=== IsGrayscale =====================================================================================================
//
//    Return TRUE only if the specified CCxTarget is realized on the RMVideo display and its color specification is
//    consistent with grayscale mode (same values for R, G, and B axes).  If current target is RMV_PLAID, the color
//    specs of both gratings must be grayscale; otherwise, we only check the "first" color spec. The target types 
//    RMV_MOVIE and RMV_IMAGE do not have a color spec, so the method returns FALSE in this case.
//
//    ARGS:       NONE
//
//    RETURNS:    TRUE if current target parameters are consistent w/ grayscale mode, FALSE otherwise.
//
BOOL CCxTargForm::IsGrayscale(CCxTarget* pTgt)
{
   if( (pTgt == NULL) || (pTgt->DataType() != CX_RMVTARG) ) return( FALSE );

   U_TGPARMS tgParms;
   pTgt->GetParams( &tgParms );
   if(tgParms.rmv.iType == RMV_MOVIE || tgParms.rmv.iType == RMV_IMAGE) return(FALSE);
   
   int nSpecs = (tgParms.rmv.iType==RMV_PLAID) ? 2 : 1;
   BOOL bSameMean = TRUE;
   BOOL bSameCon = TRUE;
   for( int i=0; i<nSpecs; i++ )
   {
      int r = tgParms.rmv.iRGBMean[i] & 0x00FF;
      int g = (tgParms.rmv.iRGBMean[i] >> 8) & 0x00FF;
      int b = (tgParms.rmv.iRGBMean[i] >> 16) & 0x00FF;
      bSameMean = bSameMean && r==g && r==b;

      r = tgParms.rmv.iRGBCon[i] & 0x00FF;
      g = (tgParms.rmv.iRGBCon[i] >> 8) & 0x00FF;
      b = (tgParms.rmv.iRGBCon[i] >> 16) & 0x00FF;
      bSameCon = bSameCon && r==g && r==b;
   }

   BOOL bGray = FALSE;
   if(tgParms.rmv.iType==RMV_GRATING || tgParms.rmv.iType==RMV_PLAID || tgParms.rmv.iType==RMV_RANDOMDOTS)
      bGray = bSameMean && bSameCon;
   else
      bGray = bSameMean;

   return( bGray );
}


//=== Propagate ======================================================================================================= 
//
//    This method propagates parameter changes in the currently loaded target's definition IAW the current modification 
//    mode. It supports the following "global" modification modes:
//
//       ALLTGTS: Change in parameter P is propagated across all COMPATIBLE targets in current target's parent set.
//       MATCHTGTS: Change in parameter P from P=P0->P1 is propagated across those COMPATIBLE targets in the parent set
//    for which P=P0.
//       SELTGTS:  Same as ALLTGTS, but applies only to targets in the edited target's parent set that are currently 
//    selected in Maestro's object tree.
//
//    What is a COMPATIBLE target? It must be implemented on the same display platform (XYScope or RMVideo). Also, it 
//    must have the same target type as the loaded target -- unless the parameter changed was the target type itself!
//
//    On MATCHTGTS modification mode and grayscale mode: When the user switches the loaded target to grayscale mode by 
//    checking the "Grayscale" (IDC_TARGF_GRAY) button, sibling targets are "matching" only if they have exactly the 
//    same color specification as the loaded target had prior to the switch to grayscale. If the grayscale button is 
//    already checked and the IDC_TARGF_REDMEAN parameter is changed (the corresponding green and blue controls are 
//    disabled in grayscale mode), the destination target will be changed only if it has the same RGB triplet in 
//    RMVTARGET.iRGBMean[0] as the loaded target had prior to the change. Similarly for _REDMEAN2, _REDCON, and 
//    _REDCON2 controls...
//
//    ARGS:       ctrlID   -- [in] ID of a CCxTargForm control that was just changed.
//                oldParms -- [in] parameter values for current target BEFORE the change was made.
//
//    RETURNS:    NONE.
//
VOID CCxTargForm::Propagate(UINT cid, U_TGPARMS oldParms)
{
   if(m_modifyMode == ATOMIC) return;
   ASSERT( m_pTarg );

   // to manipulate individual cmpts of packed RGB in place
   int redMask = 0x000000FF;
   int grnMask = 0x0000FF00;
   int bluMask = 0x00FF0000;

   // in SELTGTS mode, we query mainframe window to see if a given target is selected.
   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();
   if(m_modifyMode == SELTGTS && pFrame == NULL) 
      return;

   // target platform type: CX_XYTARG or CX_RMVTARG
   WORD wDspType = m_pTarg->DataType();
   BOOL bIsXY = BOOL(wDspType == CX_XYTARG);

   // traverse all target objects in immediate parent of target currently loaded in the form and propagate the change 
   // that was made to the loaded target to all such compatible targets IAW modification mode...
   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc(); 
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj( m_wKey ) ); 
   while(pos != NULL) 
   {
      // skip over the currently loaded target itself, and skip over any target that is not on the same display 
      // platform (XYScope or RMVideo) as the loaded target.
      WORD wKey;
      CTreeObj* pObj; 
      pDoc->GetNextChildObj( pos, wKey, pObj ); 
      if(pObj->DataType() != wDspType || wKey == m_wKey ) 
         continue;

      // in ALLTGTS mode, we modify all compatible targets, while in SELTGTS mode we modify compatible targets that are 
      // also selected in the Maestro object tree.
      BOOL bModify = BOOL(m_modifyMode == ALLTGTS);
      if(m_modifyMode == SELTGTS)
      { 
         if(!pFrame->IsObjectSelected(wKey)) continue;
         bModify = TRUE;
      }

      // get target's current parameters
      CCxTarget* pDstTgt = (CCxTarget*)pObj;
      U_TGPARMS dstParms; 
      pDstTgt->GetParams(&dstParms);

      // unless the changed parameter was the target type itself, a COMPATIBLE target must have the same type as the 
      // currently loaded target
      if(cid != IDC_TARGF_TYPE)
      {
         if( (bIsXY && (dstParms.xy.type != m_tgParms.xy.type)) || 
             ((!bIsXY) && (dstParms.rmv.iType != m_tgParms.rmv.iType)) ) 
            continue;
      }

      // handle the parameter that was just modified in the target that's loaded on form...
      int iGrat = 0;
      switch( cid ) 
      {
         // target dot life units
         case IDC_TARGF_DOTLF_MS : 
         case IDC_TARGF_DOTLF_DEG :
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.iDotLfUnits == oldParms.xy.iDotLfUnits))
                  dstParms.xy.iDotLfUnits = m_tgParms.xy.iDotLfUnits;
            }
            else
            {
               if( bModify || ((dstParms.rmv.iFlags & RMV_F_LIFEINMS) == (oldParms.rmv.iFlags & RMV_F_LIFEINMS)) )
               {
                  dstParms.rmv.iFlags &= ~RMV_F_LIFEINMS;
                  dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_LIFEINMS);
               }
            }
            break;

         // target dot noise in direction or speed (applicable only to RMVideo targets)
         case IDC_TARGF_NOISEDIR : 
         case IDC_TARGF_NOISESPEED : 
            if((!bIsXY) && 
               (bModify || ((dstParms.rmv.iFlags & RMV_F_DIRNOISE) == (oldParms.rmv.iFlags & RMV_F_DIRNOISE))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_DIRNOISE;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_DIRNOISE);
            }
            break;

         // RMV_RANDOMDOTS "Pattern motion WRT screen? flag
         case IDC_TARGF_WRTSCRN :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_RANDOMDOTS) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_WRTSCREEN) == (oldParms.rmv.iFlags & RMV_F_WRTSCREEN))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_WRTSCREEN;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_WRTSCREEN);
            }
            break;

         // algorithm for per-dot speed noise (one of two possible choices)
         case IDC_TARGF_SPDALG :
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerX == oldParms.xy.fInnerX))
                  dstParms.xy.fInnerX = m_tgParms.xy.fInnerX;
            }
            else
            {
               if( bModify || ((dstParms.rmv.iFlags & RMV_F_SPDLOG2) == (oldParms.rmv.iFlags & RMV_F_SPDLOG2)) )
               {
                  dstParms.rmv.iFlags &= ~RMV_F_SPDLOG2;
                  dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_SPDLOG2);
               }
            }
            break;

         // grating is sinewave or squarewave (applicable only to RMVideo targets)
         case IDC_TARGF_SINE : 
         case IDC_TARGF_SQUARE : 
            if((!bIsXY) && 
               (bModify || ((dstParms.rmv.iFlags & RMV_F_ISSQUARE) == (oldParms.rmv.iFlags & RMV_F_ISSQUARE))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_ISSQUARE;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_ISSQUARE);
            }
            break;

         // use independent gratings? (applicable only to RMVideo RMV_PLAID targets)
         case IDC_TARGF_INDEPGRATS :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_PLAID) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_INDEPGRATS) == (oldParms.rmv.iFlags & RMV_F_INDEPGRATS))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_INDEPGRATS;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_INDEPGRATS);
            }
            break;

         // orientation tracks drift vector direction? (applicable only to RMVideo grating and plaid targets)
         case IDC_TARGF_ORIENTADJ :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_GRATING || dstParms.rmv.iType == RMV_PLAID) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_ORIENTADJ) == (oldParms.rmv.iFlags & RMV_F_ORIENTADJ))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_ORIENTADJ;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_ORIENTADJ);
            }
            break;

         // RMV_MOVIE "repeat?" flag
         case IDC_TARGF_MVREP :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_MOVIE) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_REPEAT) == (oldParms.rmv.iFlags & RMV_F_REPEAT))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_REPEAT;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_REPEAT);
            }
            break;

         // RMV_MOVIE "pause when off?" flag
         case IDC_TARGF_MVPAUSE :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_MOVIE) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_PAUSEWHENOFF) == (oldParms.rmv.iFlags & RMV_F_PAUSEWHENOFF))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_PAUSEWHENOFF;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_PAUSEWHENOFF);
            }
            break;

         // RMV_MOVIE "at monitor frame rate?" flag
         case IDC_TARGF_MVRATE :  
            if((!bIsXY) && (dstParms.rmv.iType == RMV_MOVIE) &&
               (bModify || ((dstParms.rmv.iFlags & RMV_F_ATDISPRATE) == (oldParms.rmv.iFlags & RMV_F_ATDISPRATE))) )
            {
               dstParms.rmv.iFlags &= ~RMV_F_ATDISPRATE;
               dstParms.rmv.iFlags |= (m_tgParms.rmv.iFlags & RMV_F_ATDISPRATE);
            }
            break;

         // RMV_MOVIE or RMV_IMAGE media folder name
         case IDC_TARGF_MVFOLDER :
            if((!bIsXY) && (dstParms.rmv.iType == RMV_MOVIE || dstParms.rmv.iType == RMV_IMAGE) &&
               (bModify || (0 == ::strcmp(dstParms.rmv.strFolder, oldParms.rmv.strFolder))))
            {
               ::strcpy_s(dstParms.rmv.strFolder, oldParms.rmv.strFolder);
            }
            break;
            
         // RMV_MOVIE or RMV_IMAGE media file name
         case IDC_TARGF_MVFILE :
            if((!bIsXY) && (dstParms.rmv.iType == RMV_MOVIE || dstParms.rmv.iType == RMV_IMAGE) &&
               (bModify || (0 == ::strcmp(dstParms.rmv.strFile, oldParms.rmv.strFile))))
            {
               ::strcpy_s(dstParms.rmv.strFile, oldParms.rmv.strFile);
            }
            break;

         // target type
         case IDC_TARGF_TYPE : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.type == oldParms.xy.type))
                  dstParms.xy.type = m_tgParms.xy.type;
            }
            else
            {
               if(bModify || (dstParms.rmv.iType == oldParms.rmv.iType))
                  dstParms.rmv.iType = m_tgParms.rmv.iType;
            }
            break;

         // target aperture shape (RMVideo targets only)
         case IDC_TARGF_SHAPE : 
            if((!bIsXY) && (bModify || (dstParms.rmv.iAperture == oldParms.rmv.iAperture)))
               dstParms.rmv.iAperture = m_tgParms.rmv.iAperture;
            break;

         // width of outer bounding rectangle
         case IDC_TARGF_ORECTW : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fRectW == oldParms.xy.fRectW))
                  dstParms.xy.fRectW = m_tgParms.xy.fRectW;
            }
            else
            {
               if(bModify || (dstParms.rmv.fOuterW == oldParms.rmv.fOuterW))
                  dstParms.rmv.fOuterW = m_tgParms.rmv.fOuterW;
            }
            break;

         // height of outer bounding rectangle
         case IDC_TARGF_ORECTH : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fRectH == oldParms.xy.fRectH))
                  dstParms.xy.fRectH = m_tgParms.xy.fRectH;
            }
            else
            {
               if(bModify || (dstParms.rmv.fOuterH == oldParms.rmv.fOuterH))
                  dstParms.rmv.fOuterH = m_tgParms.rmv.fOuterH;
            }
            break;

         // width of inner bounding rectangle. For RMVideo RMV_BAR target, we use IRECTW ctrl to edit bar's drift axis
         case IDC_TARGF_IRECTW : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerW == oldParms.xy.fInnerW))
                  dstParms.xy.fInnerW = m_tgParms.xy.fInnerW;
            }
            else if(dstParms.rmv.iType == RMV_BAR)
            {
               if(bModify || (dstParms.rmv.fDriftAxis[0] == oldParms.rmv.fDriftAxis[0]))
                  dstParms.rmv.fDriftAxis[0] = m_tgParms.rmv.fDriftAxis[0];
            }
            else
            {
               if(bModify || (dstParms.rmv.fInnerW == oldParms.rmv.fInnerW))
                  dstParms.rmv.fInnerW = m_tgParms.rmv.fInnerW;
            }
            break;

         // height of inner bounding rectangle
         case IDC_TARGF_IRECTH : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerH == oldParms.xy.fInnerH))
                  dstParms.xy.fInnerH = m_tgParms.xy.fInnerH;
            }
            else
            {
               if(bModify || (dstParms.rmv.fInnerH == oldParms.rmv.fInnerH))
                  dstParms.rmv.fInnerH = m_tgParms.rmv.fInnerH;
            }
            break;

         // switched to grayscale mode. In this case, we set the entire color spec (mean and contrast for 1st and 2nd 
         // gratings) of destination target to that of the loaded tgt. In MATCHTGTS modification mode, this will happen 
         // ONLY if the destination target has the same color spec as the loaded tgt had prior to the switch!
         case IDC_TARGF_GRAY :
            if(!bIsXY)
            {
               if(!bModify)
               {
                  bModify = (dstParms.rmv.iRGBMean[0] == oldParms.rmv.iRGBMean[0]) &&
                     (dstParms.rmv.iRGBMean[1] == oldParms.rmv.iRGBMean[1]) &&
                     (dstParms.rmv.iRGBCon[0] == oldParms.rmv.iRGBCon[0]) &&
                     (dstParms.rmv.iRGBCon[1] == oldParms.rmv.iRGBCon[1]);
               }
               if(bModify)
               {
                  for(int i=0; i<2; i++)
                  {
                     dstParms.rmv.iRGBMean[i] = m_tgParms.rmv.iRGBMean[i];
                     dstParms.rmv.iRGBCon[i] = m_tgParms.rmv.iRGBCon[i];
                  }
               }
            }
            break;

         // red intensity, 0..255; _REDMEAN2 is for 2nd grating of a plaid (RMVideo targets only). If the grayscale btn 
         // is checked, the green and blue cmpts are also set. However, in this case, when the modification mode is 
         // MATCHTGTS, a matching target must have a grayscale color spec and must have the same RGB triplet as the 
         // loaded target had prior to the change!
         case IDC_TARGF_REDMEAN : 
         case IDC_TARGF_REDMEAN2 : 
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_REDMEAN) ? 0 : 1;
               if(m_bGrayScale)
               {
                  if(!bModify) 
                     bModify = IsGrayscale(pDstTgt) && (dstParms.rmv.iRGBMean[iGrat] == oldParms.rmv.iRGBMean[iGrat]);
                  if(bModify)
                     dstParms.rmv.iRGBMean[iGrat] = m_tgParms.rmv.iRGBMean[iGrat];
               }
               else if(bModify || 
                      ((dstParms.rmv.iRGBMean[iGrat] & redMask) == (oldParms.rmv.iRGBMean[iGrat] & redMask)))
               {
                  dstParms.rmv.iRGBMean[iGrat] = 
                     (dstParms.rmv.iRGBMean[iGrat] & (~redMask)) | (m_tgParms.rmv.iRGBMean[iGrat] & redMask);
               }
            }
            break;

         // green intensity, 0..255; _GRNMEAN2 is for 2nd grating of a plaid (RMVideo targets only) [not enabled in 
         // grayscale mode]
         case IDC_TARGF_GRNMEAN : 
         case IDC_TARGF_GRNMEAN2 : 
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_GRNMEAN) ? 0 : 1;
               if(bModify || ((dstParms.rmv.iRGBMean[iGrat] & grnMask) == (oldParms.rmv.iRGBMean[iGrat] & grnMask)))
               {
                  dstParms.rmv.iRGBMean[iGrat] = 
                     (dstParms.rmv.iRGBMean[iGrat] & (~grnMask)) | (m_tgParms.rmv.iRGBMean[iGrat] & grnMask);
               }
            }
            break;

         // blue intensity, 0..255; _BLUMEAN2 is for 2nd grating of a plaid (RMVideo targets only) [not enabled in 
         // grayscale mode]
         case IDC_TARGF_BLUMEAN : 
         case IDC_TARGF_BLUMEAN2 : 
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_BLUMEAN) ? 0 : 1;
               if(bModify || ((dstParms.rmv.iRGBMean[iGrat] & bluMask) == (oldParms.rmv.iRGBMean[iGrat] & bluMask)))
               {
                  dstParms.rmv.iRGBMean[iGrat] = 
                     (dstParms.rmv.iRGBMean[iGrat] & (~bluMask)) | (m_tgParms.rmv.iRGBMean[iGrat] & bluMask);
               }
            }
            break;

         // %contrast in red cmpt, 0..100; _REDCON2 is for 2nd grating of a plaid (RMVideo targets only). If the 
         // grayscale btn is checked, the green and blue cmpts are also set. However, in this case, when the 
         // modification mode is MATCHTGTS, a matching target must have a grayscale color spec and must have the same 
         // RGB contrast triplet as the loaded target had prior to the change!
         case IDC_TARGF_REDCON :  
         case IDC_TARGF_REDCON2 :
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_REDCON) ? 0 : 1;
               if(m_bGrayScale)
               {
                  if(!bModify) 
                     bModify = IsGrayscale(pDstTgt) && (dstParms.rmv.iRGBCon[iGrat] == oldParms.rmv.iRGBCon[iGrat]);
                  if(bModify)
                     dstParms.rmv.iRGBCon[iGrat] = m_tgParms.rmv.iRGBCon[iGrat];
               }
               else if(bModify || 
                      ((dstParms.rmv.iRGBCon[iGrat] & redMask) == (oldParms.rmv.iRGBCon[iGrat] & redMask)))
               {
                  dstParms.rmv.iRGBCon[iGrat] = 
                     (dstParms.rmv.iRGBCon[iGrat] & (~redMask)) | (m_tgParms.rmv.iRGBCon[iGrat] & redMask);
               }
            }
            break;

         // %contrast in green cmpt, 0..100; _GRNCON2 is for 2nd grating of a plaid (RMVideo targets only) [not 
         // enabled in grayscale mode]
         case IDC_TARGF_GRNCON :
         case IDC_TARGF_GRNCON2 :
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_GRNCON) ? 0 : 1;
               if(bModify || ((dstParms.rmv.iRGBCon[iGrat] & grnMask) == (oldParms.rmv.iRGBCon[iGrat] & grnMask)))
               {
                  dstParms.rmv.iRGBCon[iGrat] = 
                     (dstParms.rmv.iRGBCon[iGrat] & (~grnMask)) | (m_tgParms.rmv.iRGBCon[iGrat] & grnMask);
               }
            }
            break;

         // %contrast in blue cmpt, 0..100; _BLUCON2 is for 2nd grating of a plaid (RMVideo targets only) [not 
         // enabled in grayscale mode]
         case IDC_TARGF_BLUCON :
         case IDC_TARGF_BLUCON2 :
            if(!bIsXY)
            {
               iGrat = (cid==IDC_TARGF_BLUCON) ? 0 : 1;
               if(bModify || ((dstParms.rmv.iRGBCon[iGrat] & bluMask) == (oldParms.rmv.iRGBCon[iGrat] & bluMask)))
               {
                  dstParms.rmv.iRGBCon[iGrat] = 
                     (dstParms.rmv.iRGBCon[iGrat] & (~bluMask)) | (m_tgParms.rmv.iRGBCon[iGrat] & bluMask);
               }
            }
            break;

         // #dots in target's random-dot pattern
         case IDC_TARGF_NDOTS : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.ndots == oldParms.xy.ndots))
                  dstParms.xy.ndots = m_tgParms.xy.ndots;
            }
            else
            {
               if(bModify || (dstParms.rmv.nDots == oldParms.rmv.nDots))
                  dstParms.rmv.nDots = m_tgParms.rmv.nDots;
            }
            break;

         // size of a "dot" in pixels (RMVideo targets only)
         case IDC_TARGF_DOTSZ : 
            if((!bIsXY) && (bModify || (dstParms.rmv.nDotSize == oldParms.rmv.nDotSize)))
               dstParms.rmv.nDotSize = m_tgParms.rmv.nDotSize;
            break;

         // target dot life in deg or msecs
         case IDC_TARGF_DOTLIFE : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fDotLife == oldParms.xy.fDotLife))
                  dstParms.xy.fDotLife = m_tgParms.xy.fDotLife;
            }
            else
            {
               if(bModify || (dstParms.rmv.fDotLife == oldParms.rmv.fDotLife))
                  dstParms.rmv.fDotLife = m_tgParms.rmv.fDotLife;
            }
            break;

         // percent coherence. For XY scope targets, this is stored in the field XYPARMS.fInnerW.
         case IDC_TARGF_COHER : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerW == oldParms.xy.fInnerW))
                  dstParms.xy.fInnerW = m_tgParms.xy.fInnerW;
            }
            else
            {
               if(bModify || (dstParms.rmv.iPctCoherent == oldParms.rmv.iPctCoherent))
                  dstParms.rmv.iPctCoherent = m_tgParms.rmv.iPctCoherent;
            }
            break;

         // noise range limit. For XY scope targets, this is stored in the field XYPARMS.fInnerW.
         case IDC_TARGF_NOISERNG : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerW == oldParms.xy.fInnerW))
                  dstParms.xy.fInnerW = m_tgParms.xy.fInnerW;
            }
            else
            {
               if(bModify || (dstParms.rmv.iNoiseLimit == oldParms.rmv.iNoiseLimit))
                  dstParms.rmv.iNoiseLimit = m_tgParms.rmv.iNoiseLimit;
            }
            break;

         // noise update interval. For XY scope targets, this is stored in the field XYPARMS.fInnerH.
         case IDC_TARGF_NOISEUPD : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerH == oldParms.xy.fInnerH))
                  dstParms.xy.fInnerH = m_tgParms.xy.fInnerH;
            }
            else
            {
               if(bModify || (dstParms.rmv.iNoiseUpdIntv == oldParms.rmv.iNoiseUpdIntv))
                  dstParms.rmv.iNoiseUpdIntv = m_tgParms.rmv.iNoiseUpdIntv;
            }
            break;

         // 1st/2nd grating drift axis in deg CCW (RMVideo targets only)
         case IDC_TARGF_GRAT1_DA : 
         case IDC_TARGF_GRAT2_DA : 
            iGrat = (cid==IDC_TARGF_GRAT1_DA) ? 0 : 1;
            if((!bIsXY) && (bModify || (dstParms.rmv.fDriftAxis[iGrat] == oldParms.rmv.fDriftAxis[iGrat])))
               dstParms.rmv.fDriftAxis[iGrat] = m_tgParms.rmv.fDriftAxis[iGrat];
            break;

         // 1st/2nd grating spatial frequency in cyc/deg (RMVideo targets only)
         case IDC_TARGF_GRAT1_SF : 
         case IDC_TARGF_GRAT2_SF : 
            iGrat = (cid==IDC_TARGF_GRAT1_SF) ? 0 : 1;
            if((!bIsXY) && (bModify || (dstParms.rmv.fSpatialFreq[iGrat] == oldParms.rmv.fSpatialFreq[iGrat])))
               dstParms.rmv.fSpatialFreq[iGrat] = m_tgParms.rmv.fSpatialFreq[iGrat];
            break;

         // 1st/2nd grating spatial phase in deg (RMVideo targets only)
         case IDC_TARGF_GRAT1_PH : 
         case IDC_TARGF_GRAT2_PH : 
            iGrat = (cid==IDC_TARGF_GRAT1_PH) ? 0 : 1;
            if((!bIsXY) && (bModify || (dstParms.rmv.fGratPhase[iGrat] == oldParms.rmv.fGratPhase[iGrat])))
               dstParms.rmv.fGratPhase[iGrat] = m_tgParms.rmv.fGratPhase[iGrat];
            break;

         // the X-offset of the hole in the XYScope RECTANNU target only. For RMVideo targets, this is the X std dev 
         // of the Gaussian window
         case IDC_TARGF_XSIGMA : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerX == oldParms.xy.fInnerX))
                  dstParms.xy.fInnerX = m_tgParms.xy.fInnerX;
            }
            else
            {
               if(bModify || (dstParms.rmv.fSigma[0] == oldParms.rmv.fSigma[0]))
                  dstParms.rmv.fSigma[0] = m_tgParms.rmv.fSigma[0];
            }
            break;

         // the Y-offset of the hole in the XYScope RECTANNU target only. For RMVideo targets, this is the Y std dev 
         // of the Gaussian window
         case IDC_TARGF_YSIGMA : 
            if(bIsXY)
            {
               if(bModify || (dstParms.xy.fInnerY == oldParms.xy.fInnerY))
                  dstParms.xy.fInnerY = m_tgParms.xy.fInnerY;
            }
            else
            {
               if(bModify || (dstParms.rmv.fSigma[1] == oldParms.rmv.fSigma[1]))
                  dstParms.rmv.fSigma[1] = m_tgParms.rmv.fSigma[1];
            }
            break;

         // seed for random-dot generation (RMVideo targets only)
         case IDC_TARGF_RANDSEED : 
            if((!bIsXY) && (bModify || (dstParms.rmv.iSeed == oldParms.rmv.iSeed)))
               dstParms.rmv.iSeed = m_tgParms.rmv.iSeed;
            break;

         // flicker parameters (RMVideo targets only)
         case IDC_TARGF_FLICKON :
            if((!bIsXY) && (bModify || (dstParms.rmv.iFlickerOn == oldParms.rmv.iFlickerOn)))
               dstParms.rmv.iFlickerOn = m_tgParms.rmv.iFlickerOn;
            break;
         case IDC_TARGF_FLICKOFF :
            if((!bIsXY) && (bModify || (dstParms.rmv.iFlickerOff == oldParms.rmv.iFlickerOff)))
               dstParms.rmv.iFlickerOff = m_tgParms.rmv.iFlickerOff;
            break;
         case IDC_TARGF_FLICKDELAY :
            if((!bIsXY) && (bModify || (dstParms.rmv.iFlickerDelay == oldParms.rmv.iFlickerDelay)))
               dstParms.rmv.iFlickerDelay = m_tgParms.rmv.iFlickerDelay;
            break;

         default :                                                      //    we should NEVER get here!
            TRACE0( "Bad ID in CCxTargForm::Propagate!\n" );
            break;
      }

      // finally, update the destination target with the new set of parameters
      BOOL bChanged;
      pDstTgt->SetParams(&dstParms, bChanged); 
   }
}

