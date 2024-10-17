//=====================================================================================================================
//
// cxtrialform.cpp : Implementation of class CCxTrialForm.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CCxTrialForm encapsulates Maestro's trial editor, a form view through which the user modifies the definition of the
// Maestro trial object, CCxTrial. A trial is the most complex of all the Maestro data objects.  Its definition
// includes a number of "header" parameters, a list of perturbations used in the trial, and a variably-sized "segment
// table" that lists the targets participating in the trial and their motion trajectories during each segment of the
// trial. Since a trial can contain many different segments and participating targets, the segment table can grow
// relatively large. See CCxTrial for the details.
//
// ==> Construction of form; layout of controls; use of grid controls.
// The layout of the CCxTrialForm view is defined in several dialog template resources: IDD_TRIALFORM defines the
// overall layout, with a tab pane container (IDC_TRH_TABPROPS), a "partitions" grid (IDC_TRH_PARTITIONS), and the 
// segment table (IDC_TRH_SEGTABLE) arranged vertically; the "Main" properties tab pane (IDC_TRIALFORM_MAIN);
// the "Perturbations/PSGM" tab (IDC_TRIALFORM_OTHER), and the "Random Variables" tab (IDC_TRIALFORM_RV). Use the
// Visual Studio Resource Editor to review the layout of these templates. We use various "Windows common controls" on
// the "Main" and "Perts/PSGM" tab panes to represent the various parameters in the trial header.
//
// Of special note are four custom controls used to represent the segment table, a "partitions" grid that serves as a
// column header for the segment table, the perturbation table (IDC_TRH_PERTS), and the random variables table 
// (ID_TRV_GRID). These are instances of Chris Maunder's spreadsheet-like "MFC Grid Control" (see CREDITS), CGridCtrl.
// In the case of the segment table, it offers a compact way of laying out the trial's segments, each of which includes
// some general parameters (the "segment header") followed by a set of motion parameters ("target trajectory record") 
// for each tgt participating in the trial. We actually employ a derivative of CGridCtrl, CLiteGrid, which is designed 
// to work only in "virtual mode" and which provides a number of built-in "inplace editors" (text string, formatted 
// numeric text, combo box, and tree item selector) which are not available in CGridCtrl. The layout and usage of each 
// CLiteGrid is described later. Because it is a custom control, we must dynamically subclass each grid's HWND to a 
// CLiteGrid object before we use it. See CCxTrialForm::OnInitialUpdate().
//
// NOTE:  The integer resource IDs below must represent a contiguous range of values.
//    1) IDC_TRH_WTSPIN...IDC_TRH_SGMSEGSPIN ==> The spin controls for relative weight, first save seg, failsafe seg,
//       special operation seg, display marker segments, and the SGM op mode. 
//    2) IDC_TRH_KEEP...IDC_TRH_IGNVELROT ==> The PB controls for keep/toss flag, sacc-trig'd op selection, staircase
//       trial designation ("normal", or a member of one of 5 staircases), staircase response input selection, and
//       mid-trial reward mode ("periodic" or "segEnd") -- plus checkbox control for selecting whether or not an SGM
//       stimulus is externally triggered, and for selectively ignoring the global scale/rotate transforms for target 
//       position and velocity vectors.  The PB controls IDC_TRH_KEEP...IDC_TRH_MTRMODE must be a contiguous subset
//       of this range.
//    3) IDC_TRH_SACCVT...IDC_TRH_WHVR2DEN ==> Edit ctrls for saccade threshold velocity, staircase strength, pulse
//       length and WHVR numerator/denominator for reward pulses 1 and 2, mid-trial reward interval and pulse length, 
//       and trial weight. These all appear on the "Main" page of the form.
//    3a) IDC_TRH_SGMPA1...IDC_TRH_SGMNT ==> Edit controls for selected PSGM parameters appearing on the 
//       "Perturbations/PSGM" page of the form.
//    4) IDC_TRH_CHCFG...IDC_TRH_SGMOP ==> Combo box controls specifying the channel config associated with trial and
//       the SGM operational mode.
//    5) ID_GRID_INSERTTARG...ID_PERT_CLEAR ==> Command IDs for items in the context menus associated with the segment
//       table, partitions grid, and perturbation list grid.
//    6) IDC_TRH_SEGTABLE...IDC_TRV_GRID ==> Control IDs for the four grid controls on the form.
//
// ==> XYScope functionality removed for Maestro 5.0.
// The XYScope platform has been unsupported since Maestro 4.0, and all GUI elements related to the XYScope were
// removed in v5.0. On CCxTrialForm, two parameter controls were removed from the "Main" tab pane: IDC_TRH_XYILSPIN
// (spin ctrl, id=1304) and IDC_TRH_XYIL (read-only edit, id=?) controlled the # of interleaved XYScope targets in the 
// trial, and IDC_TRH_XYDOTSEED (edit ctrl, id=1397) specified the alternate random dot seed for the trial. 
// 
// ==> The segment table:  Presenting trial trajectory parameters in a CLiteGrid.
// Each segment is represented by a PAIR of columns in the grid.  The first ROWSINHDR rows of a column-pair contain all
// the "segment header" parameters (min & max duration, fixation targets, etc).  The next M * ROWSINTGT rows define
// "trajectory records" for the M targets participating in the trial.  The first column of the grid displays row
// headings that describe the contents of the corresponding row.  The cells at row# (ROWSINHDR + m*ROWSINTGT), for
// m = 0..M-1, are referred to as "target selector" cells.  They display the name of each target participating in the
// trial.  They are special because you can right-click on them to insert a new target at that location, replace that
// target, or delete it.
//
// When a trial contains many targets and/or segments, the segment table grid becomes very large.  Rather than attach
// scroll bars to the grid itself, the grid is resized so that no scroll bars are needed, and CCxTrialForm adjusts its
// own scroll bars so that the user can scroll any part of the grid into view.  This solution was deemed simpler than
// having two sets of scroll bars, one on CCxTrialForm and one on the segment grid itself.
//
// CLiteGrid is designed to use the underlying grid control in "virtual mode", a lightweight implementation that avoids
// the memory overhead of associating a CGridCell-derived object with each and every cell in the grid.  In this scheme,
// the grid ctrl uses a registered callback function to obtain info needed to repaint any grid cell.  This callback,
// which is part of the baseline CGridCtrl framework, almost exclusively governs the appearance of the grid.  See
// the display callback GridDispCB() for details on the contents of the segment grid.
//
// The default title-tip implementation of the MFC Grid Control (in virtual mode) only shows title tips if a cell's
// contents do not fit within its current bounds, and the title tip's text is identical to the cell's contents.  This
// was not adequate for our purposes, because we want the title tip for a target selector cell to show a target's full
// pathname as a quick clue to the user (one can use like-named targets as long as they are not siblings in the
// Maestro object tree).  We modified the grid control's title-tip implementation, introducing the GVIS_VIRTUALTITLETIP
// state flag as a hint to the grid callback function that the grid is requesting the title tip text for a cell rather
// than its cell contents.  GridDispCB() recognizes and responds to this title tip hint only for
// the target selector cells.
//
// ==> The "trial partitions" grid.
// The "intra-trial tagged section" feature was introduced in Maestro v1.3.0.  A tagged section is a contiguous range
// of segments associated with a short descriptive label.  Multiple tagged sections in a single trial cannot overlap.
// The feature was introduced to support the parsing of trials that contain multiple independent "mini-trials".  Since
// a tagged section spans one or more contiguous segments, a neat solution would include a column header in the segment
// table (the table used to have one in Maestro prior to v1.3.0!); a tagged section would be represented by merging the
// cells in the column header that correspond to the range of segments spanned by the section.  However, the MFC Grid
// Control framework does not support this.  Instead, we introduce another, single-row grid (IDC_TRH_PARTITIONS) that
// sits above the segment table and serves as its column header.
//
// The first cell in this grid displays the current modification mode for the segment grid, while the remaining cells
// display the trial's current partitions.  A partition is either a tagged section or an individual segment that is not
// part of a tagged section.  In the latter case, the relevant cell in the partitions grid lies immediately above and
// spans the corresponding column-pair in the segment grid and displays the segment number.  For a tagged section
// partition, the grid cell spans all of the segment grid column-pairs in the range of segments covered by the section.
// The cell displays the section tag name, followed by the range of segments it spans.
//
// To create a tagged section, the user simply left-clicks on any partition cell while holding the SHIFT key down.  This
// action initiates the selection of a range of partition cells; the cell clicked is highlighted in blue.  To complete
// the action, the user must SHIFT-left-click on another partition cell (it could be the same one) within five seconds.
// Maestro immediately creates merges the selected partitions into a single tagged section, giving it a default name.
// An inplace edit control appears, letting the user change the tag name of the new section, if desired.
//
// To rename a tagged section, the user double-clicks on the relevant partition cell, which brings up the same inplace
// edit control.  If an invalid section name is entered, Maestro will alert the user with a "beep" and reinitiate the
// inplace edit.  To remove a tagged section, right-click on the relevant partition cell and choose the appropriate
// command from the context menu that appears.
//
// Since the partitions grid serves as the column header for the segment table, right-clicking on the grid will invoke
// the context menu by which the user adds/deletes segments, etc.  See next section.
//
// ==> Operations on the segment table grid; CLiteGrid's use of "callback" functions for editing cell data inplace.
// Some segment table operations, such as target insertion/deletion and segment insertion/deletion, are initiated
// through a context menu.  Multiple-choice parameters (including two-state parameters) in the table can be changed
// merely by right-clicking on the associated cell, while all "editable" parameters can be modified "inplace" by
// invoking a suitable "popup" control.  We take advantage of the various inplace controls provided by CLiteGrid:  an
// edit control for short text strings, a specialized edit control for integer or floating-point numbers, a combo box
// for multiple-choice parameters, and a tree control for selecting a parameter's value from a heirarchical tree
// structure.  CLiteGrid also gives its parent window a chance to modify the contents of a cell in response to a right
// mouse click (with or w/o SHIFT key).  To take advantage of these facilities and tailor them to the kinds of data
// stored in the segment table, we must install three additional callback functions:
//    GridEditCB() -- Invoked when an inplace edit operation is about to take place (or when a rt click or SHIFT-rt clk
//       occurs in the cell).  Retrieves cell edit info, or modifies cell contents IAW mouse click.
//    GridEndEditCB() -- Called to update the segment table IAW results of an inplace operation just completed.
//    CCxDoc::TreeInfoCB() -- We rely on this CCxDoc method to provide CLiteGrid's inplace tree control with the info
//       required to traverse the CNTRLX targets subtree on demand -- allowing the user to choose a target to add to
//       the trial's target list or to replace a target in that list.
// See CLiteGrid for more information on how these callback methods are invoked and used.
//
// Here is a summary of the operations that the user can perform on the segment table grid:
//    -- Double-click on a target selector cell to change the identity of the participating target.
//    -- Right-click on multiple-choice parameters to change their state in place.  With SHIFT key down, the click is
//       interpreted as an decrement action; else, increment.
//    -- Double-click on any individual parameter cell in the segment header or target trajectory records to edit the
//       associated parameter "in place".  Inplace editing can be initiated on the current focus merely by hitting an
//       appropriate key on the keyboard.  After editing the parameter, the user can confirm the change by hitting
//       RETURN or merely clicking the mouse outside the inplace control's rectangle.  The control is then hidden, and
//       the new parameter value (auto-corrected if necessary) appears within the underlying cell.  The user can also
//       hit the ESCAPE key to cancel the inplace editing operation, or end the operation on the current cell and start
//       a new inplace edit on an adjacent cell by hitting one of the arrow keys or the TAB key in combination with the
//       CTRL key.  This reduces reliance on the  mouse for navigating around the grid.
//    -- Right-clicking on any cell in the first column of the segment table, or anywhere on the partitions grid,
//       invokes a context menu that offers an assortment of grid operations, some of which may be disabled depending
//       on the context (ie, the segment or target, if any, implied by where the user clicked!).  For example,
//       right-click on a target selector cell in the segment grid and choose the appropriate command to insert, delete
//       or change a participating target.  Or, right-click on the partitions grid and choose the appropriate command
//       to insert, delete, copy, paste or replace a segment in the trial.  The paste and replace operations will only
//       work if the current paste segment (saved by the last copy operation) is consistent with the # of trial targets
//       currently defined.  Since the paste segment is saved by CCxTrialForm itself, it is possible to copy a segment
//       from one trial and insert it into another trial having the same # of trial targets.  Other wholesale ops
//       available on the context menu include "Clear Table", "Remove All Targets", and "Remove All Segments".
//
// ==> Trial perturbation list displayed and modified via a CLiteGrid.
// A user can choose to apply up to MAX_TRIALPERTS perturbations to various target trajectories during a trial.  The
// CCxTrial object maintains several required attributes for each entry in its "perturbation list", and CCxTrialForm
// uses another CLiteGrid control (IDC_TRH_PERTS) to present this list for viewing and modification.  Each row of the
// grid corresponds to a trial perturbation, with the defining attributes in columns:
//
//    Col#  Attribute                     Presentation form in CLiteGrid
//    ----  ----------------------        ----------------------------------------
//     0    Unique object key             Perturbation's name
//     1    Amplitude (deg/sec)           Floating-pt ("1.00") value
//     2    Start segment index           Integer value
//     3    Affected target index         Integer value
//     4    Affected trajectory cmpt      One of "winH", "winV", "patH", "patV", "winDir", "patDir", "winSpd", 
//                                        "patSpd", "direc", or "speed".
//
// As with the segment table, we take advantage of CLiteGrid's built-in inplace editing tools to modify the different
// perturbation attributes.  Here's a summary of the available operations:
//    -- Right-click on any multiple-choice attribute (the last 3 columns) to change its state in place.  With SHIFT
//       key down, the click is interpreted as a decrement action; else, increment.  You can also increment/decrement
//       the perturbation amplitude in this manner.
//    -- Double-click on any attribute cell to invoke a suitable inplace tool for changing that attribute.
//    -- To append a perturbation entry to the list, remove a selected entry, or clear the list, right-click on any
//       cell in the first column and choose the appropriate item from the context menu that is displayed.
// For details on how the perturbation list is displayed and modified, see the grid callbacks PertGridDispCB(),
// PertGridEditCB(), and PertGridEndEditCB().
//
// Note that, whenever you make a structural change in the segment table, the "start segment" and "affected target"
// attributes in the perturbation list could be affected.  The CCxTrial object updates these appropriately; however, it
// is incumbent upon CCxTrialForm to reflect these adjustments by refreshing the perturbation list grid.
//
// ==> Trial "modification modes".
// When creating trial definitions, it is sometimes handy to be able to change an individual parameter across all
// segments in one go, or across all trials in a set.  Thus, CCxTrialForm supports six different "modification modes":
//
//    "Modify" ==> Modify the selected parameter only in the current segment.
//
//    "Modify ALL SEGS" ==> Propagate changes across all segments in trial.  After editing, selected parameter will
//       have the same value in all segments.
//
//    "Modify MATCHING SEGS" ==> Propagate changes across all like-valued parameters in all segments of trial.  If
//       selected parameter P in the current segment was changed from P0 to P1, the same change will be made in each
//       segment for which parameter P = P0 prior to the operation.
//
//    "Modify ALL TRIALS" ==> Propagate changes across all trials in the current trial's set.  A change in a trial
//       header parameter (including the perturbation list) is propagated across all trials for which the change is
//       permissible.  A change in a segment header parameter in segment N is propagated across all trials for which
//       segment N exists.  A change in the target trajectory parameter for the Mth target in segment N is propagated
//       across all trials for which there is an Mth target and an Nth segment.  Finally, structural changes in the
//       segment table are propagated only across trials that have the same # of targets and segments as the current
//       trial did PRIOR to the change.
//
//    "Modify MATCHING TRIALS" ==> Similar to "Modify ALL TRIALS", except that only those trials are modified for which
//       the selected parameter P matches the old value for the currently loaded trials (P0).
//
//    "Modify SELECTED TRIALS" ==> Propagate changes across all trials that are currently selected in the Maestro 
//       object tree (as long as they are in the same set as the currently edited trial!). The caveats applicable to 
//       the "Modify ALL TRIALS" mode apply here as well.
//
// The global modes "ALL SEGS" and "MATCHING SEGS" pertain ONLY to individual parameters within a segment header or
// target trajectory record in the segment table grid.  The "ALL TRIALS", "SELECT TRIALS", and "MATCHING TRIALS" modes 
// are obviously much more powerful, since they will propagate a change in ANY trial parameter across all or some 
// trials in the set -- as though you're modifying multiple trials in a set at the same time.  However, these modes 
// must be used with great care to avoid unintended changes across the entire trial set.  Currently, operations 
// involving tagged sections cannot be propagated across trials.
//
// The first cell in the partitions grid indicates the current modification mode.  In normal "Modify" mode it has the
// standard grey background; when the table is in one of the global modes, the background is red.  To change the mode,
// right-click anywhere on the partitions grid or on the first column of the segment table, then select the desired
// mode from the context menu.
//
// ==> Disabling controls for selected trial header parameters.
// Not all of the trial header parameters laid out in the trial editor property pages are applicable at all times. For 
// example, for "normal" (vs "staircase") trials, the staircase trial response channel and staircase strength have no 
// meaning. To handle such situations, CCxTrialForm disables any irrelevant controls as needed. When no trial is 
// loaded, all controls are disabled. The segment table is empty in this situation, and its associated context menu
// is disabled. See EnableControls().
//
// ==> Changes to trial definition are applied immediately; DDX not used.
// Any change made on the CCxTrialForm is handled as soon as it occurs, rather than waiting for the user to press an
// "Apply" button.  If the change is unacceptable, it is automatically corrected in some way and the controls are
// updated to reflect the corrections made.  Since we must catch parameter changes as they occur, we have elected not
// to use MFC's Dialog Data Exchange techniques in our implementation.
//
// ==> Subclassed edit controls restrict user input -- CNumEdit.
// The modifiable numeric parameters that appear in the trial header controls often have "hard" range restrictions.
// Some must be nonnegative.  Some are floating-pt values of limited precision, while others can only be integers.  All
// are numeric values that are displayed in an edit control capable of accepting many non-numeric characters.  To
// prevent the user from entering nonsense data, we derived CNumEdit from CEdit as a configurable integer/FP numeric
// edit control.  However, because the edit controls are laid out on a dialog template resource, the MFC framework
// initially treats them as standard edit controls.  To get the CNumEdit functionality, we must SUBCLASS each of the
// edit controls.  This is done just before the view is displayed, in OnInitialUpdate().
//
// ==> Updating a trial's "object dependencies.
// The trial definition laid out in this view is "dependent" on other Maestro data objects -- the participating targets
// and perturbations, and an associated channel config object. The trial's definition would be compromised if the user
// could delete these objects. Hence, we prevent user from doing so via a dependency locking mechanism available thru
// the Maestro document method CCxDoc::UpdateObjDeps(). This scheme requires cooperation by views. For example, after
// the user adds, deletes, or changes the identity of a dependent obj in the trial displayed on CCxTrialForm, we must
// call UpdateObjDeps(), passing it the old set of dependencies existing prior to the change. Hence, we keep track of
// the current trial's dependencies in a protected member, m_arDepObjs. Also note that, if a dependent object's name
// is changed outside this view, CCxTrialForm must update itself accordingly (see OnUpdate()).
//
// ==> Interactions with CCxTrial, CCxDoc, other CNTRLX views.
// CCxTrialForm must query CCxDoc whenever it must obtain a ptr to the trial definition object, CCxTrial, for a given
// trial.  CCxTrialForm then queries the CCxTrial object directly to access and modify the trial's defn.  Whenever it
// does so, CCxTrialForm must set the document's modified flag via CDocument::SetModifiedFlag(), and inform all other
// attached CNTRLX views by broadcasting an appropriate CNTRLX-specific CCxViewHint via CDocument::UpdateAllViews().
// Likewise, user actions in other views can affect the current contents of CCxTrialForm.  For example, if the user
// selects a different trial for display, the just-selected trial object must be loaded into this form.  If the user
// deletes the trial currently being displayed, CCxTrialForm must reset its contents.  In each case, the responsible
// view broadcasts a hint via UpdateAllViews(), and that signal is processed by the all-important CView override,
// OnUpdate().  See CCxTrialForm::OnUpdate() for details.
//
// Each of the CNTRLX "object definition forms" has been designed for use in a "tabbed window" -- in particular, the
// TTabWnd class that is part of the "Visual Framework" library (see CREDITS).  The CNTRLX main frame window (see
// CCxMainFrame) installs each form in one of the tab panes of a TTabWnd.  The caption of the tab pane reflects the
// "full name" of the CNTRLX data object currently loaded on the form.  This tab window is NOT a view, which presents a
// technical problem:  how do we update the tab window when the name of a loaded object changes, or when the user
// selects a different type of object for viewing (which requires bringing a different tab pane to the front).  Our
// solution:  All of the object definition forms (CCxTrialForm, CCxTargForm, etc.) are derived from TVTabPane, a simple
// CFormView-derivative that provides methods for telling the parent TTabWnd to update a tab caption or bring a
// particular tab to the front of the tab window.  TVTabPane is a supplement to the Visual Framework (see VISUALFX.*).
//
//
// CREDITS:
// (1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21.
// The segment layout would not be possible without this extremely useful control (CGridCtrl) and its underlying
// framework.  We use the derived class CLiteGrid rather than CGridCtrl itself.  CLiteGrid is intended only for use in
// the grid's "virtual mode", and it provides several different inplace controls for modifying different types of cell
// data.  NOTE that we have made several minor modifications to the underlying CGridCtrl framework to make CLiteGrid
// work properly and to detect right-clicks on fixed cells, a feature which is essential to our application here.
// (2) The "Visual Framework" library, by Zoran M. Todorovic [09/30/2000, www.codeproject.com/dialog/visualfx.asp] --
// We use this library to implement the view framework that appears in the CNTRLX frame window's (CCxMainFrame) client
// area.  This framework makes more complex GUI layouts -- that would have been a bear to implement w/core MFC -- easy
// to build.  CCxTrialForm is derived from TVTabPane, a class that I added to the Visual Framework.
//
//
// REVISION HISTORY:
// 31oct2000-- Created.  IDD_TRIALFORM dialog template laid out.  Begun work on implementation.
// 07nov2000-- NEW DESIGN:  Two views will work together to display a trial's definition:  CCxTrialForm has been
//             changed to CCxTrialHdrForm, which manages the controls defining the trial header.  CCxTrialSegView will
//             be a CGridCtrl encapsulated as a view and will manage the segment layout of the trial.
//          -- Completed except for implementing context menu for selecting a channel set obj to assoc w/ the trial.
// 21jan2001-
// 25jan2001-- Back to a single view for the entire trial definition.  CCxTrialHdrForm and CCxTrialSegView merged to
//             CCxTrialForm....  Also got rid of the trial-specific modification hint codes in CCxViewHint...
//          -- Added DoChannelConfigModify() to allow user to change the CNTRLX channel config assigned to the trial.
// 26jan2001-- Updated implementation IAW recent changes to CCxViewHint and cxobj_ifc.h.
// 27mar2001-- Removed read-only trial name field.  Full name of loaded trial is now displayed in the tab caption of
//             the tab pane in which CCxTrialForm is installed.  CCxTrialForm is now derived from TVTabPane, which is
//             designed to be a child of the Visual Framework's tab window class, TTabWnd.  TVTabPane provides methods
//             for signaling the parent TTabWnd to bring a tab pane to the front or update its caption.
//          -- UpdateTrialName() replaced by UpdateTitle().
// 09may2001-- Miscellaneous bug fixes.  Revised GridCallback() to return immediately if the grid is no longer there.
//             This can happen when the grid is destroyed during cleanup prior to app exit.
// 27sep2001-- Cosmetic:  CExperimentDoc --> CCxDoc.
// 06nov2001-- Updated to use a combo box (IDC_TRH_CHCFG) subclassed to CCxObjCombo to select the channel configuration
//             associated with the trial.
// 13nov2001-- Bug fix:  In SDI apps, each view's OnInitialUpdate() is called not once, but each time a new document is
//             created/opened.  Control subclassing in OnInitialUpdate() should only be done once!  MFC documentation
//             suggests putting one-time init code in OnCreate() instead, but I'm not sure that will work for form
//             views based on dialog templates.  Instead, I just include a check to avoid repeated subclassing....
// 12dec2001-- MAJOR mods to the structures in CXOBJ_IFC.H that encapsulate a trial's definition.  Modified CCxTrial
//             and, consequently, CCxTrialForm IAW these changes.  E.g., dropped support for perturbations (for now),
//             and added support for multiple staircase sequencing.  See also CXOBJ_IFC.H.
// 18dec2001-- MAJOR mod to how user selects XY scope targets for interleaving.  Reverted to more primitive scheme
//             used in cntrlxUNIX:  A spin control in the trial header specifies the #targets, N, to interleave.  The
//             first N XY scope targets defined in the segment table are then selected for interleaving.  The targets
//             are interleaved throughout the trial.  The only constraint placed on N by CCxTrial is that it be <= the
//             # of defined targets.  Other constraints are enforced at runtime.
// 27dec2001-- Added new widget IDC_TRH_SACCVT, and associated static label IDC_TRH_SACCVTLBL, to represent the
//             saccade threshold velocity required for the saccade-trig'd special operations.  This parameter had been
//             inadvertently omitted from the trial header region of the form.
// 15jan2002-- Modification in how user specifies the "open-loop" feature of a trial -- to better represent the
//             practical limitations of the current implementation of the feature during trial runtime!  Now, we
//             restrict the feature to a single, designated "open-loop" segment, specified in the trial header and
//             reflected by spin control IDC_TRH_OPENSEGSPIN and its auto-buddy read-only edit ctrl IDC_TRH_OPENSEG on
//             IDD_TRIALFORM.  The affected target is always that designated as "fixation tgt #1" during the open-loop
//             segment.  Formerly, the user could designate any target in any segment for this feature, as defined by
//             controls on the IDD_TRAJINFO template.  Those controls have been removed.  CCxTrialForm & CCxTrajRecDlg
//             were updated accordingly.
// 25feb2002-- Updating the loaded trial in response to EN_CHANGE is not a good idea -- since EN_CHANGE is issued even
//             when the user is in the midst of typing in the edit control.  Respond to EN_KILLFOCUS instead.
// 04mar2002-- Added controls to specify parameters for the new pulse stimulus generator module.
//          -- Instead of hiding controls that are currently irrelevant, we merely disable them.
// 07mar2002-- OnChange() modified so that it only informs doc/view framework when the trial record is actually changed
//             by the operation.
// 19apr2002-
// 30apr2002-- MAJOR REVISION.  Rather than displaying a modal dialog to modify parameters in a segment header record
//             or target trajectory record, it would be much cleaner to modify individual parameters in place.  To that
//             end, each segment column will be divided into two columns (the "Segment N" label will appear in the 1st
//             col), and the various parameters redistributed among the cells.  Several types of inplace editing ctrls
//             will be used to edit parameters "in place".
// 02may2002-- Replacing CCxTreePopup (a modal dialog with embedded tree ctrl) with non-modal CCxInPlaceTree.
// 08may2002-- Additional revisions to use the new SetXxxx() routines in CCxTrial that modify individual parameters in
//             the segment table and also implement the global "across-segment" modification modes.
// 10jun2002-- After further investigation, decided to respond to CBN_SELCHANGE rather than CBN_CLOSEUP.  The former is
//             also sent when the user changes the selection with arrow or char keys (droplist closed), and we need to
//             catch these selection changes as well!
// 20sep2002-- Integrated support for "SelectByFix2", a new sacc-trig'd operation.
// 01oct2002-- Fixed bug re: responding to WM_VSCROLL messages from the spin button controls.  The ON_CONTROL_RANGE
//             message map macro is not appropriate for use with WM_VSCROLL; instead, you must use the ON_WM_VSCROLL
//             macro with the message handler OnVScroll().
// 18oct2002-- Introducing add'l per-segment parameters:  separate H & V fixation accuracies, plus a "periodic reward
//             enable" flag.  Had to add a row to the segment header region of trial defn grid to make room...
// 02dec2002-- Began making modifications to use CLiteGrid instead of CGridCtrl for the trial table.  CLiteGrid offers
//             built-in support for inplace editing the grid in virtual mode, so CCxTrialForm will no longer have to
//             support the inplace controls.  Instead, it defines and installs several additional callback functions
//             that help CLiteGrid do its job...
// 12dec2002-- Began mods to display trial's "perturbation list" (just added to CCxTrial) in a second CLiteGrid.
// 17dec2002-- Done adding "perturbation list", but still debugging...
//          -- Target traj parameters "Window Vel H" and "Window Vel V" were on separate lines, to provide a cell to
//             specify a perturbation of each of those components.  Given our design for the perturbation list, these
//             cells are no longer needed -- so we rearranged target record rows so that the two components are on the
//             same line.
// 10apr2003-- Began major mod to introduce new modification mode "Modify TRIAL SET" that propagates a change in ANY
//             trial parameter across all "compatible" trials in the current trial's set.  In addition, we've moved
//             support for the existing modification modes "Modify ALL SEGS" and "Modify MATCHING SEGS" from CCxTrial
//             to CCxTrialForm.  Wanted all modification modes to be implemented in the same place....
// 23oct2003-- Added PB control IDC_TRH_OPENENA, which selects which motion components are velocity stabilized during
//             the "open loop" segment (if one is specified).  PB reads "H only", "V only", or "H and V".
// 27oct2003-- Introduced new modification mode "Modify MATCHING TRIALS" that propagates a change in ANY trial
//             parameter across all "compatible" trials  IF the parameter's value matches the old value of the param
//             that was changed in the particular trial currently loaded on the form.  This is analogous to "Modify
//             MATCHING SEGS" mode, except that it operates across the trial set rather than across the segments of the
//             loaded trial.
// 05mar2004-- Some users have been confused by the scrolling of the segment table grid.  If CCxTrialForm is currently
//             sized smaller than its design size, then it will have its own scroll bars present.  The two sets of
//             scroll bars are confusing in certain viewing situations.  This is an attempt to simply the display.
//             We'll dynamically resize the segment grid so that no scrolling is necessary.  At the same time, we must
//             adjust the size of CCxTrialForm whenever the grid is resized so that we can use the form's scroll bars
//             to look at any portion of the grid.  As part of this mod, we turned off resizing of the grid's columns.
// 15mar2004-- UpdateTitle() replaced by overriding TVTabPane::UpdateCaption() instead.  Also, we no longer display the
//             full "pathname" of an object b/c it makes the tab text too long...
// 20sep2004-- Adding support to display/edit new trial header parameters TRLHDR.iMarkSeg1 and TRLHDR.iMarkSeg2 that
//             specify "display marker" segments.  If set to a valid segment index, the data trace display will draw a
//             vertical line marking the start of the selected segment.  Intended to help users direct their attn to a
//             particular portion of the trial data...
// 03nov2004-- Added support for new saccade-trig'd special op, "dualFix".
// 25jan2005-- The "dualFix" operation is now called "switchFix"...  Also added UI support for enhancements to the
//             mid-trial reward feature.  There are now two alternative modes:  The original "periodic" mode, in which
//             rewards are delivered at a regular interval during all enabled segments, plus a new "segEnd" mode, in
//             which a reward is delivered at the end of each enabled segment.  Instead of getting the periodic reward
//             interval and pulse length from the application settings CCxSettings, there are now trial header params
//             for each of these quantities.
// 31mar2005-- Adding support for viewing/editing intra-trial "tagged sections" via a separate CLiteGrid that sits
//             immediately above the segment table: the "trial partitions" grid.
// 03may2005-- Changed static control IDC_TRH_WEIGHT to an edit control so that user can type in the weight value.
// 28jul2005-- Updated to support to additional options for what trajectory component is affected by a perturbation:
//             target direction and pattern direction.  By direction, we mean the angle of the velocity vector.  Thus,
//             we introduce the idea of a directional perturbation.  Note that CCxTrialForm does not prevent the user
//             from applying both a directional and a horiz and/or vert velocity perturbation to the same target, even
//             though it does not make much sense to do so!
// 24oct2005-- Added support to display/edit new trial header parameter TRLHDR.iXYDotSeedAlt, which can optionally
//             override the dot seed specified in the video display settings.
// 29nov2005-- EnableHdrControls() modified to enable both reward pulse widgets when the special op "RP Distro" is
//             selected.
// 07dec2005-- OnChange() and StuffHdrPB() updated to support the new "R/P Distro" special op.
// 13mar2006-- Added support to display/edit new trial header parameter TRLHDR.nOpenSegs; velocity stabilization can
//             now be active over a contiguous span of trial segments instead of just a single segment.
// 13apr2006-- MAJOR rework of the velocity stabilization feature.  It now can be engaged on a per-target, per-segment
//             basis. Trial header parameters TRLHDR.iOpenSeg and nOpenSegs are OBSOLETE, as are the trial header flags
//             TRH_OPEN***.  The relevant trial header controls have been removed from CCxTrialForm.  User now
//             specifies the velocity stabilization "mode" ("off", "H+V", "H ONLY", "V ONLY") and "snap to eye" flag
//             via cells in the segment table.  A new row has been added to each target record in the table to
//             accommodate specification of these parameters.
// 19may2006-- Added four checkbox controls, IDC_TRH_IGNPOSSCALE ... IDC_TRH_IGNVELROT, to expose new trial header 
//             flags that support selectively ignoring the global pos/vel vector scale and rotate transforms on a 
//             per-trial basis.
// 04dec2006-- Added new modification mode, ID_GRID_MODSELTRIALS, that modifies selected trials in the current trial's 
//             parent set (if any). Queries Maestro object tree (via CCxMainFrame) to determine whether a given trial 
//             is selected or not.
// 03jan2007-- Modified to handle change in TRLHDR: Special op is now stored as integer field TRLHDR.iSpecialOp 
//             rather than using bit flags in TRLHDR.dwFlags. Also, added two new special ops, "Choose Fix1" and 
//             "Choose Fix2".
// 27feb2007-- Mods to support specifying pattern acceleration H,V on a per-segment basis.
// 24apr2007-- SGM-related mods: Updated IAW renamed constants in cxobj_ifc.h. Spinner controls for #pulses, #trains
//             were replaced by edit controls. Fixed #digits allowed in IPI and ITI controls. Added op mode "Biphasic 
//             Train" to the relevant combo box's dropdown list.
// 16jul2007-- Mods to support two additional perturbable aspects of a trial target's trajectory, window speed
//             (PERT_ON_SWIN) and pattern speed (PERT_ON_SPAT).
// 29aug2007-- Mods to support two additional perturbable aspects of a trial target's trajectory, window AND pattern
//             direction together (PERT_ON_DIR), or window AND pattern speed together (PERT_ON_SPD). Effec v2.1.3.
// 01feb2011-- Minor mod to support new special operation "searchTask" (TH_SOP_SEARCH). Effective v2.6.5.
// 02mar2011-- Mod in EnableHdrControls() -- saccadic threshold velocity is enabled for "searchTask". If subject eye
//             fails to reach that velocity at some point during search task, then the subject did not "try" the task.
// 23aug2016-- Began major rework to organize all controls above the segment table into a modeless property sheet 
// control with 3 property pages: "Main", "Perturbations/PSGM", and "Random Variables". The third page will introduce
// support for defining random variables for a trial. These RVs can then be assigned to select quantities in the
// segment table.
// 12sep2016-- Simplified "Random Variables" tab. It contains only a grid control in which the 10 available random
// variables are listed. Variables that are not in use have type RV_NOTUSED. This is because CCxTrial does not create
// random variables on the heap, but maintains a 10-element list of variables, any subset of which may be used.
// 21sep2016-- Updating Grid***CB() and PropagateSegParam() to handle selected segment table parameters to which a 
// trial RV may be assigned. Note that user is allowed to assign an undefined RV (type RV_NOTUSED) to a parameter.
// Uses CCxTrial ***SegParam() methods to determine whether or not a particular parameter is RV-assignable and whether
// or not an RV is currently assigned to the parameter.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
// 13sep2018-- Mods to support raising the definition of the trial's chosen channel set, any perturbation in the
// perturbation list, or any participating target in the segment table. This is done by right-clicking on the relevant
// table cell (or on the channel combo box) while holding the Shift key down.
// 17sep2018-- Removed mods from 13sep2018. The Shift-Right Click gesture would occasionally cause Maestro to appear
// to enter an infinite loop; Windows would report that Maestro was not responding. Tried to debug to no avail. The
// feature wasn't worth the effort.
// 20sep2018-- Mod to segment table to include "RMV Sync" flag in segment header, in the right column adjacent to the
// XY frame period. If "ON", then RMVideo will present a time synchronization flash in a square box at the top-left
// corner of the RMVideo screen. The flash starts during the first video frame marking the start of the segment. The 
// box size, width of a dark border around the box, and the flash duration are all parameters specified with the 
// RMVideo display settings. Investigator can use a photodiode circuit to deliver a TTL pulse when flash is detected
// and route that pulse for timestamping by Maestro.
// 15may2019-- Mod to include 4 new trial header controls that define the random withhold variable ratio N/D for 
// reward pulses 1 and 2: IDC_TRH_WHVR1NUM .. IDC_TRH_WHVR2DEN.
// 17oct2024-- XYScope-specific parameters removed from CCXTrialForm for Maestro v5.0 (XYScope support was dropped in
// v4.0): #XYscope interleaved targets, alternate random dot seed, and the per-segment XY frame period.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX

#include "cxviewhint.h"                      // the "hint" class used by all CNTRLX views
#include "cxmainframe.h"                     // CCxMainFrame -- Maestro's main frame window class
#include "cxtrialform.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxMainPage, CCxPertsPage, and CCxRandVarsPage
//
// These property pages are embedded in a modeless property sheet and serve to organize the many controls on the trial
// form in a more compact presentation. They serve only as the control containers; all of the functionality remains in
// CCxTrialForm. All relevant control notifications are merely forwarded to the relevant CCxTrialForm handler. The
// page classes are "friends" of CCxTrialForm and vice versa so that they can access each other's members.
//
// Because the property pages are really modeless dialogs, keyboard accelerators would normally not work when the focus
// is on a control within a page. To get around this, the property page classes override PreTranslateMessage to give
// our main frame window (CCxMainFrame) a chance to handle the accelerators. 
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxMainPage, CPropertyPage )

BEGIN_MESSAGE_MAP( CCxMainPage, CPropertyPage )
   ON_WM_VSCROLL()
   ON_CONTROL_RANGE( BN_CLICKED, IDC_TRH_KEEP, IDC_TRH_MTRMODE, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_TRH_IGNPOSSCALE, IDC_TRH_IGNVELROT, OnChange )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_TRH_SACCVT, IDC_TRH_WHVR2DEN, OnChange )
   ON_CONTROL( CBN_SELCHANGE, IDC_TRH_CHCFG, OnChanCfgSelect )
END_MESSAGE_MAP()


// NOTE: These message handlers simply forward to the relevant handler in CCxTrialForm.
void CCxMainPage::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pWnd )
{
   if(m_pTrialForm != NULL && pWnd != NULL) m_pTrialForm->OnVScroll(nSBCode, nPos, pWnd);
}
void CCxMainPage::OnChange(UINT id) { if(m_pTrialForm != NULL) m_pTrialForm->OnChange(id); }
void CCxMainPage::OnChanCfgSelect() { if(m_pTrialForm != NULL) m_pTrialForm->OnComboSelChange(IDC_TRH_CHCFG); }

BOOL CCxMainPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   BOOL bOk = m_spinWeight.SubclassDlgItem( IDC_TRH_WTSPIN, this );  
   bOk = bOk && m_spinSave.SubclassDlgItem( IDC_TRH_SAVESEGSPIN, this );
   bOk = bOk && m_spinFailsafe.SubclassDlgItem( IDC_TRH_FAILSAFESPIN, this );
   bOk = bOk && m_spinSpecial.SubclassDlgItem( IDC_TRH_SPECSEGSPIN, this );
   bOk = bOk && m_spinMark1.SubclassDlgItem( IDC_TRH_MARK1SPIN, this );
   bOk = bOk && m_spinMark2.SubclassDlgItem( IDC_TRH_MARK2SPIN, this );
   bOk = bOk && m_edSaccVt.SubclassDlgItem( IDC_TRH_SACCVT, this );
   bOk = bOk && m_edStairStren.SubclassDlgItem( IDC_TRH_STAIRSTREN, this );
   bOk = bOk && m_edRewP1.SubclassDlgItem( IDC_TRH_REWP1, this );
   bOk = bOk && m_edWHVR1Num.SubclassDlgItem(IDC_TRH_WHVR1NUM, this);
   bOk = bOk && m_edWHVR1Den.SubclassDlgItem(IDC_TRH_WHVR1DEN, this);
   bOk = bOk && m_edRewP2.SubclassDlgItem( IDC_TRH_REWP2, this );
   bOk = bOk && m_edWHVR2Num.SubclassDlgItem(IDC_TRH_WHVR2NUM, this);
   bOk = bOk && m_edWHVR2Den.SubclassDlgItem(IDC_TRH_WHVR2DEN, this);
   bOk = bOk && m_edMTRIntv.SubclassDlgItem( IDC_TRH_MTRINTV, this );
   bOk = bOk && m_edMTRLen.SubclassDlgItem( IDC_TRH_MTRLEN, this );
   bOk = bOk && m_edWeight.SubclassDlgItem( IDC_TRH_WEIGHT, this );
   bOk = bOk && m_cbSelChan.SubclassDlgItem( IDC_TRH_CHCFG, this );

   if( !bOk ) AfxThrowNotSupportedException(); 

   m_edSaccVt.SetFormat( TRUE, TRUE, 3, 1 );
   m_edStairStren.SetFormat( FALSE, TRUE, 7, 3 );
   m_edRewP1.SetFormat(TRUE, TRUE, 3, 1);
   m_edWHVR1Num.SetFormat(TRUE, TRUE, 2, 1);
   m_edWHVR1Den.SetFormat(TRUE, TRUE, 3, 1);
   m_edRewP2.SetFormat( TRUE, TRUE, 3, 1 );
   m_edWHVR2Num.SetFormat(TRUE, TRUE, 2, 1);
   m_edWHVR2Den.SetFormat(TRUE, TRUE, 3, 1);
   m_edMTRIntv.SetFormat( TRUE, TRUE, 4, 1 );
   m_edMTRLen.SetFormat( TRUE, TRUE, 3, 1 );
   m_edWeight.SetFormat( TRUE, TRUE, 3, 1 );

   m_spinWeight.SetRange( 0, 255 );

   return(TRUE);
}

/** 
* This override is necessary because a property page is really a dialog, and a dialog eats keyboard accelerators. Here 
* we give the main frame window a chance to catch those accelerators. 
*/
BOOL CCxMainPage::PreTranslateMessage(MSG* pMsg)
{
   if(pMsg->message == WM_KEYDOWN)
   {
      if(((CCntrlxApp*) AfxGetApp())->GetMainFrame()->PreTranslateMessage(pMsg)) return(TRUE);
   }
 
   return(CPropertyPage::PreTranslateMessage(pMsg));
}

//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxRandVarsPage
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxRandVarsPage, CPropertyPage )

BEGIN_MESSAGE_MAP( CCxRandVarsPage, CPropertyPage )
END_MESSAGE_MAP()


BOOL CCxRandVarsPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   // set up the RV grid that is the sole control on this tab page
   if(!m_rvGrid.SubclassDlgItem(IDC_TRV_GRID, this)) AfxThrowNotSupportedException();

   // no DnD, no row/col resize, no selection allowed
   m_rvGrid.EnableDragAndDrop(FALSE);
   m_rvGrid.SetRowResize(FALSE);
   m_rvGrid.SetColumnResize(FALSE);
   m_rvGrid.EnableSelection(FALSE);

   // install grid callbacks defined on parent CCxTrialForm; this ASSUMES m_pTrialForm is already set!
   m_rvGrid.SetCallbackFunc(CCxTrialForm::RVGridDispCB, (LPARAM) m_pTrialForm);
   m_rvGrid.SetEditCBFcn(CCxTrialForm::RVGridEditCB, (LPARAM) m_pTrialForm);
   m_rvGrid.SetEndEditCBFcn(CCxTrialForm::RVGridEndEditCB, (LPARAM) m_pTrialForm);

   // init grid with the number of rows and columns it will always have. First row serves as a column header,
   // and first column displays random variable names "x0" .. "x9"
   m_rvGrid.SetRowCount(1 + MAX_TRIALRVS);
   m_rvGrid.SetColumnCount(7);
   m_rvGrid.SetFixedRowCount(1);
   m_rvGrid.SetFixedColumnCount(1);
   m_rvGrid.GetDefaultCell(TRUE, TRUE)->SetFormat(DT_CENTER | DT_SINGLELINE); 
   m_rvGrid.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_SINGLELINE);
   m_rvGrid.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_SINGLELINE);
   m_rvGrid.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

   // set fixed column widths
   m_rvGrid.SetColumnWidth( 0, 30 ); 
   m_rvGrid.SetColumnWidth( 1, 100 );
   m_rvGrid.SetColumnWidth( 2, 80 );
   m_rvGrid.SetColumnWidth( 3, 80 );
   m_rvGrid.SetColumnWidth( 4, 80 );
   m_rvGrid.SetColumnWidth( 5, 80 );
   m_rvGrid.SetColumnWidth( 6, 300 );

   return(TRUE);
}

/** 
* This override is necessary because a property page is really a dialog, and a dialog eats keyboard accelerators.
* Here we give the main frame window a chance to catch those accelerators.
*/
BOOL CCxRandVarsPage::PreTranslateMessage(MSG* pMsg)
{
   if(pMsg->message == WM_KEYDOWN)
   {
      if(((CCntrlxApp*) AfxGetApp())->GetMainFrame()->PreTranslateMessage(pMsg)) return(TRUE);
   }
   return(CPropertyPage::PreTranslateMessage(pMsg));
}

//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxPertsPage
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxPertsPage, CPropertyPage )

BEGIN_MESSAGE_MAP( CCxPertsPage, CPropertyPage )
   ON_WM_VSCROLL()
   ON_CONTROL_RANGE( BN_CLICKED, IDC_TRH_SGMTRIG, IDC_TRH_SGMTRIG, OnChange )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_TRH_SGMPA1, IDC_TRH_SGMNT, OnChange )
   ON_CONTROL( CBN_SELCHANGE, IDC_TRH_SGMOP, OnSelectSGMOp )
   ON_NOTIFY_RANGE(NM_RCLICK, IDC_TRH_PERTS, IDC_TRH_PERTS, OnNMRClick)
END_MESSAGE_MAP()


// NOTE: These message handlers simply forward to the relevant handler in CCxTrialForm.
void CCxPertsPage::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pWnd )
{
   if(m_pTrialForm != NULL && pWnd != NULL) m_pTrialForm->OnVScroll(nSBCode, nPos, pWnd);
}
void CCxPertsPage::OnChange(UINT id) { if(m_pTrialForm != NULL) m_pTrialForm->OnChange(id); }
void CCxPertsPage::OnSelectSGMOp() { if(m_pTrialForm != NULL) m_pTrialForm->OnComboSelChange(IDC_TRH_SGMOP); }
void CCxPertsPage::OnNMRClick(UINT id, NMHDR* pNMHDR, LRESULT* pResult ) 
{
   if(m_pTrialForm != NULL) m_pTrialForm->OnNMRClick(id, pNMHDR, pResult);
}

BOOL CCxPertsPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   BOOL bOk = m_spinSgmSeg.SubclassDlgItem( IDC_TRH_SGMSEGSPIN, this );
   bOk = bOk && m_edSgmPulseAmp1.SubclassDlgItem( IDC_TRH_SGMPA1, this );
   bOk = bOk && m_edSgmPulseAmp2.SubclassDlgItem( IDC_TRH_SGMPA2, this );
   bOk = bOk && m_edSgmPulseWidth1.SubclassDlgItem( IDC_TRH_SGMPW1, this );
   bOk = bOk && m_edSgmPulseWidth2.SubclassDlgItem( IDC_TRH_SGMPW2, this );
   bOk = bOk && m_edSgmInterPulse.SubclassDlgItem( IDC_TRH_SGMIPI, this );
   bOk = bOk && m_edSgmInterTrain.SubclassDlgItem( IDC_TRH_SGMITI, this );
   bOk = bOk && m_edSgmNP.SubclassDlgItem( IDC_TRH_SGMNP, this );
   bOk = bOk && m_edSgmNT.SubclassDlgItem( IDC_TRH_SGMNT, this );
   bOk = bOk && m_cbSgmOp.SubclassDlgItem( IDC_TRH_SGMOP, this );
   bOk = bOk && m_pertGrid.SubclassDlgItem( IDC_TRH_PERTS, this );

   if( !bOk ) AfxThrowNotSupportedException();

   m_edSgmPulseAmp1.SetFormat( TRUE, FALSE, 6, 1 );
   m_edSgmPulseAmp2.SetFormat( TRUE, FALSE, 6, 1 );
   m_edSgmPulseWidth1.SetFormat( TRUE, TRUE, 4, 1 );
   m_edSgmPulseWidth2.SetFormat( TRUE, TRUE, 4, 1 );
   m_edSgmInterPulse.SetFormat( TRUE, TRUE, 3, 1 );
   m_edSgmInterTrain.SetFormat( TRUE, TRUE, 4, 1 );
   m_edSgmNP.SetFormat(TRUE, TRUE, 3, 1);
   m_edSgmNT.SetFormat(TRUE, TRUE, 3, 1);

   // stuff SGM op mode combo box with strings describing modes available -- order is important here!
   m_cbSgmOp.AddString( _T("Single Pulse") );
   m_cbSgmOp.AddString( _T("Two Pulses") );  
   m_cbSgmOp.AddString( _T("Biphasic Pulse") ); 
   m_cbSgmOp.AddString( _T("Pulse Train") );
   m_cbSgmOp.AddString( _T("Biphasic Train") );
   m_cbSgmOp.AddString( _T("Not In Use") );

   // configure the perturbation list grid control: No DnD, no row/col resize, no selection
   m_pertGrid.EnableDragAndDrop( FALSE ); 
   m_pertGrid.SetRowResize( FALSE ); 
   m_pertGrid.SetColumnResize( FALSE ); 
   m_pertGrid.EnableSelection( FALSE ); 

   // set callbacks for perturbation grid. This ASSUMES that m_pTrialForm has already been set!
   m_pertGrid.SetCallbackFunc( CCxTrialForm::PertGridDispCB, (LPARAM) m_pTrialForm );  
   m_pertGrid.SetEditCBFcn( CCxTrialForm::PertGridEditCB, (LPARAM) m_pTrialForm );  
   m_pertGrid.SetEndEditCBFcn( CCxTrialForm::PertGridEndEditCB, (LPARAM) m_pTrialForm ); 

   // init grid with only the fixed row header and set default cell formats
   m_pertGrid.SetRowCount( 1 );
   m_pertGrid.SetColumnCount( 5 ); 
   m_pertGrid.SetFixedRowCount( 1 );
   m_pertGrid.SetFixedColumnCount( 0 );
   m_pertGrid.GetDefaultCell( TRUE, TRUE )->SetFormat( DT_CENTER | DT_SINGLELINE ); 
   m_pertGrid.GetDefaultCell( TRUE, FALSE )->SetFormat( DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS );
   m_pertGrid.GetDefaultCell( FALSE, TRUE )->SetFormat( DT_RIGHT | DT_SINGLELINE | DT_PATH_ELLIPSIS );
   m_pertGrid.GetDefaultCell( FALSE,FALSE )->SetFormat( DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS );

   // set fixed column widths
   m_pertGrid.SetColumnWidth( 0, 120 ); 
   m_pertGrid.SetColumnWidth( 1, 60 );
   m_pertGrid.SetColumnWidth( 2, 30 );
   m_pertGrid.SetColumnWidth( 3, 100 );
   m_pertGrid.SetColumnWidth( 4, 60 );

   return(TRUE);
}

/** 
* This override is necessary because a property page is really a dialog, and a dialog eats keyboard accelerators.
* Here we give the main frame window a chance to catch those accelerators.
*/
BOOL CCxPertsPage::PreTranslateMessage(MSG* pMsg)
{
   if(pMsg->message == WM_KEYDOWN)
   {
      if(((CCntrlxApp*) AfxGetApp())->GetMainFrame()->PreTranslateMessage(pMsg)) return(TRUE);
   }
   return(CPropertyPage::PreTranslateMessage(pMsg));
}


//=====================================================================================================================
//=====================================================================================================================
//
// Implementation of CCxTrialForm
//
//=====================================================================================================================
//=====================================================================================================================

IMPLEMENT_DYNCREATE( CCxTrialForm, TVTabPane )


BEGIN_MESSAGE_MAP( CCxTrialForm, TVTabPane )
   ON_WM_VSCROLL()
   ON_WM_TIMER()
   ON_COMMAND_RANGE(ID_GRID_INSERTTARG, ID_PERT_CLEAR, OnGridOps)
   ON_UPDATE_COMMAND_UI_RANGE(ID_GRID_INSERTTARG, ID_PERT_CLEAR, OnUpdGridOps)
   ON_NOTIFY_RANGE(NM_RCLICK, IDC_TRH_SEGTABLE, IDC_TRH_PARTITIONS, OnNMRClick)
   ON_NOTIFY(NM_CLICK, IDC_TRH_PARTITIONS, OnNMClick)
   ON_UPDATE_COMMAND_UI_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnUpdateEditCommand )
   ON_COMMAND_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnEditCommand )
END_MESSAGE_MAP()



//=====================================================================================================================
// PRIVATE CONSTANTS & GLOBALS
//=====================================================================================================================
LPCTSTR CCxTrialForm::strPertListLabels[] =
   {
      _T("Pert Name"),
      _T("Gain"),
      _T("Seg"),
      _T("Target"),
      _T("Affected")
   };

LPCTSTR CCxTrialForm::strPertAffectedCmptLabels[] =
   {
      _T("winH"),
      _T("winV"),
      _T("patH"),
      _T("patV"),
      _T("winDir"),
      _T("patDir"),
      _T("winSpd"),
      _T("patSpd"),
      _T("direc"),
      _T("speed")
   };

LPCTSTR CCxTrialForm::strSegHdrLabels[] =
   {
      _T("Min & Max Duration (ms)"),
      _T("RMV Sync"),
      _T("Fixation Targets 1 & 2"),
      _T("H,V Fixation Accuracy (deg)"),
      _T("Grace(ms); Mid-trial Reward?"),
      _T("Marker pulse?/Check response?")
   };
LPCTSTR CCxTrialForm::strTrajLabels[] =
   {
      _T(" "),                                                          // (target name goes here)
      _T("Velocity Stabilization"),
      _T("Window Pos H,V (deg)"),
      _T("Window Vel H,V (deg/s)"),
      _T("Window Acc H,V (deg/s^2)"),
      _T("Pattern Vel H,V (deg/s)"),
      _T("Pattern Acc H,V (deg/s^2)")
   };

const COLORREF CCxTrialForm::clrYellow             = RGB(245,241,163);
const COLORREF CCxTrialForm::clrLtGrn              = RGB(144,238,144);
const COLORREF CCxTrialForm::clrBlue               = RGB(122,150,223);
const COLORREF CCxTrialForm::clrWhite              = RGB(255,255,255);
const COLORREF CCxTrialForm::clrMedGray            = RGB(160,160,160);
const COLORREF CCxTrialForm::clrRed                = RGB(255,  0,  0);
const int CCxTrialForm::SEGCOL_W                   = 50;
const UINT CCxTrialForm::SECTCREATE_TIMEOUT        = 5000;

LPCTSTR CCxTrialForm::strRVTypeLabels[] = {
   _T("UNUSED"), _T("UNIFORM"), _T("NORMAL"), _T("EXPON"), _T("GAMMA"), _T("FUNCTION")
};


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTrialForm [constructor] ======================================================================================
//
//    Construct the trial data form view.
//
//    Almost all the work is handled by the framework, which loads the form's layout from a CNTRLX resource whose
//    identifier is stored in CCxTrialForm's protected member var IDD.  However, we do need to init certain variables
//    that track the form's state.  Also, most of the controls on the form must be subclassed to CWnd-derived members
//    of CCxTrialForm -- but that's delayed until OnInitialUpdate().
//
CCxTrialForm::CCxTrialForm() : TVTabPane( CCxTrialForm::IDD )
{
   m_bOneTimeInitsDone = FALSE;
   m_bLoading = FALSE;
   m_wKey = CX_NULLOBJ_KEY;               // initially, no trial object is loaded on form
   m_pTrial = NULL;
   m_bEnable = FALSE;

   m_pPasteSeg = NULL;                    // no paste segment or context cell yet
   m_contextCell.row =
   m_contextCell.col = -1;
   m_iContextSeg = -1;
   m_nRightClickedGrid = 0;
   m_iInsPos = -1;                        // >= 0 only while choosing a tgt or pert obj to attach to the trial
   m_wLastTgtKey = CX_NULLOBJ_KEY;

   m_minGridSize = CSize(0,0);            // min segment grid size is determined in OnInitialUpdate()
   m_minScrollSize = CSize(0,0);          // likewise for the minimum scroll size of the form

   m_modifyMode = CCxTrialForm::ATOMIC;   // start up in normal modification mode (global operations disabled)

   m_tagSectAnchorCell.row =              // can't be a tagged section-create gesture in progress!
   m_tagSectAnchorCell.col = -1;
   m_nSectCreateTimerID = 0;

   m_nPartitions = 0;                     // there's no trial, so there are no partitions!

   // set up property pages with a reference to this form, so that they can forward control notifications
   m_mainPage.SetParentForm(this);
   m_pertsPage.SetParentForm(this);
   m_rvPage.SetParentForm(this);
}


//=== ~CCxTrialForm [destructor] ======================================================================================
//
//    When this view is destroyed, we must destroy anything we've dynamically created
//
CCxTrialForm::~CCxTrialForm()
{
   if ( m_pPasteSeg )
   {
      delete m_pPasteSeg;
      m_pPasteSeg = NULL;
   }

   CancelSectionCreateGesture();             // make sure timer resource has been released!

   if(m_pPropSheet != NULL)
   {
      delete m_pPropSheet;
      m_pPropSheet = NULL;
   }
}



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnVScroll =======================================================================================================
//
//    Handle WM_VSCROLL message from any of the spin controls [IDC_TRH_WTSPIN...IDC_TRH_SGMSEGSPIN] on the form.
//
//    For *vertical* spin controls, WM_VSCROLL is sent to the owner window after the position of the control has
//    changed.  We merely extract the control id and invoke OnChange() to update the trial object appropriately.  We
//    have to do things this way because the WM_VSCROLL message is not handled by the ON_CONTROL_RANGE() message map
//    macro!!
//
//    Note that we allow the base class to handle WM_VSCROLL first -- to handle scroll messages from the form's own
//    scroll bars (if any).
//
//    ARGS:       See CWnd::OnVScroll().
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pWnd )
{
   // if msg was NOT from one of the spin controls, then let base class handle scrolling of the form itself
   if(pWnd == NULL)
   {
      TVTabPane::OnVScroll( nSBCode, nPos, pWnd );
      return;
   }

   // if message was from one of our spin controls, forward to OnChange() for processing...
   UINT id = (UINT) pWnd->GetDlgCtrlID();                      
   if( id >= IDC_TRH_WTSPIN && id <= IDC_TRH_SGMSEGSPIN)
      OnChange( id );
}


/** OnChange ==========================================================================================================
* Update a parameter in the loaded trial's header IAW a detected change in the associated control. We handle various 
* notifications here:
* 1) BN_CLICKED ==> User clicked one of the pushbutton/checkbox controls [IDC_TRH_KEEP...IDC_TRH_VELROT]. This action 
* modifies trial state flags (or other vars) in some way, and in most cases a PB label is updated to reflect the new 
* state.
* 2) WM_VSCROLL ==> User scrolled one of the spin controls [IDC_TRH_WTSPIN...IDC_TRH_SGMSEGSPIN]. Here we just need to 
* update the loaded trial's header with the new parameter value.
* 3) EN_KILLFOCUS ==> Keyboard focus has left a numeric edit ctrl [IDC_TRH_SACCVT ... IDC_TRH_WHVR2DEN, IDC_SGMPA1 ... 
* IDC_TRH_SGMNT], indicating contents may have changed. Update the corresponding parameter in the loaded trial's 
* header. Note that this will also be sent by a control that loses the focus b/c it is about to be disabled (in this 
* case, the contents have not changed!).
*
* NOTE that this method is now called by message handlers registered on the property page that contains the control!
*
* @param id -- [in] resource ID of the child control that sent the notification.
*/
void CCxTrialForm::OnChange(UINT id)
{
   if( m_pTrial == NULL ) return;                              // if no trial loaded, ignore

   TRLHDR hdr;                                                 // get current trial header; remember state prior to
   m_pTrial->GetHeader( hdr );                                 // change in case we need to propagate change across
   TRLHDR oldHdr = hdr;                                        // matching trials

   switch( id )                                                // update the associated parameter...
   {
      case IDC_TRH_KEEP :                                      //    toggle the 2-state keep/toss flag
         hdr.dwFlags ^= THF_KEEP;
         break;
      case IDC_TRH_TRITYP :                                    //    increment the staircase designation in the
         ++hdr.iStairNum;                                      //    range [0("normal")..5].
         if( hdr.iStairNum > MAX_STAIRS )
            hdr.iStairNum = 0;
         break;
      case IDC_TRH_STAIRRESP :                                 //    toggle the 2-state staircase response chan flag
         hdr.dwFlags ^= THF_STAIRRESP;
         break;
      case IDC_TRH_SPECOP :                                    //    increment special operation ID, with wrap-around
         ++hdr.iSpecialOp;
         if(hdr.iSpecialOp >= TH_NUMSPECOPS)
            hdr.iSpecialOp = TH_SOP_NONE;
         break;
      case IDC_TRH_MTRMODE :                                   //    toggle state of the mid-trial reward mode flag
         hdr.dwFlags ^= THF_MTRMODE;
         break;
      case IDC_TRH_SGMTRIG :                                   //    toggle state of the SGM "Ext Trig?" flag
         hdr.sgm.bExtTrig = !(hdr.sgm.bExtTrig);
         break;

      case IDC_TRH_IGNPOSSCALE :                               //    toggle state of "ignore tgt pos vector scale" and 
         hdr.dwFlags ^= THF_IGNPOSSCALE;                       //    analogous flags
         break;
      case IDC_TRH_IGNPOSROT : 
         hdr.dwFlags ^= THF_IGNPOSROT;
         break;
      case IDC_TRH_IGNVELSCALE : 
         hdr.dwFlags ^= THF_IGNVELSCALE;
         break;
      case IDC_TRH_IGNVELROT : 
         hdr.dwFlags ^= THF_IGNVELROT;
         break;

      case IDC_TRH_WTSPIN :                                    //    update the trial's relative weight -- via spin
      case IDC_TRH_WEIGHT :                                    //    control or editable buddy
         hdr.iWeight = m_mainPage.m_spinWeight.GetPos();
         break;
      case IDC_TRH_SAVESEGSPIN :                               //    update the first save segment index
         hdr.iStartSeg = m_mainPage.m_spinSave.GetPos();
         break;
      case IDC_TRH_FAILSAFESPIN :                              //    update the failsafe segment index
         hdr.iFailsafeSeg = m_mainPage.m_spinFailsafe.GetPos();
         break;
      case IDC_TRH_SPECSEGSPIN :                               //    update the special segment index
         hdr.iSpecialSeg = m_mainPage.m_spinSpecial.GetPos();
         break;
      case IDC_TRH_MARK1SPIN :                                 //    update display marker segment #1 index
         hdr.iMarkSeg1 = m_mainPage.m_spinMark1.GetPos();
         break;
      case IDC_TRH_MARK2SPIN :                                 //    update display marker segment #2 index
         hdr.iMarkSeg2 = m_mainPage.m_spinMark2.GetPos();
         break;
      case IDC_TRH_SGMSEGSPIN :                                //    update the SGM start segment index
         hdr.iSGMSeg = m_pertsPage.m_spinSgmSeg.GetPos();
         break;

      case IDC_TRH_SACCVT :                                    //    update saccade threshold velocity
         hdr.iSaccVt = m_mainPage.m_edSaccVt.AsInteger();
         break;
      case IDC_TRH_STAIRSTREN :                                //    update staircase trial's strength
         hdr.fStairStrength = m_mainPage.m_edStairStren.AsFloat();
         break;
      case IDC_TRH_REWP1 :                                     //    update reward pulse length, WHVR numer/denom
         hdr.reward1[0] = m_mainPage.m_edRewP1.AsInteger();
         break;
      case IDC_TRH_WHVR1NUM:
         hdr.reward1[1] = m_mainPage.m_edWHVR1Num.AsInteger();
         break;
      case IDC_TRH_WHVR1DEN:
         hdr.reward1[2] = m_mainPage.m_edWHVR1Den.AsInteger();
         break;
      case IDC_TRH_REWP2 :                                     //    update reward pulse length, WHVR numer/denom
         hdr.reward2[0] = m_mainPage.m_edRewP2.AsInteger();
         break;
      case IDC_TRH_WHVR2NUM:
         hdr.reward2[1] = m_mainPage.m_edWHVR2Num.AsInteger();
         break;
      case IDC_TRH_WHVR2DEN:
         hdr.reward2[2] = m_mainPage.m_edWHVR2Den.AsInteger();
         break;
      case IDC_TRH_SGMPA1 :                                    //    update SGM pulse 1 amplitude
         hdr.sgm.iAmp1 = m_pertsPage.m_edSgmPulseAmp1.AsInteger();
         break;
      case IDC_TRH_SGMPA2 :                                    //    update SGM pulse 2 amplitude
         hdr.sgm.iAmp2 = m_pertsPage.m_edSgmPulseAmp2.AsInteger();
         break;
      case IDC_TRH_SGMPW1 :                                    //    update SGM pulse 1 width
         hdr.sgm.iPW1 = m_pertsPage.m_edSgmPulseWidth1.AsInteger();
         break;
      case IDC_TRH_SGMPW2 :                                    //    update SGM pulse 2 width
         hdr.sgm.iPW2 = m_pertsPage.m_edSgmPulseWidth2.AsInteger();
         break;
      case IDC_TRH_SGMIPI :                                    //    update SGM interpulse interval
         hdr.sgm.iPulseIntv = m_pertsPage.m_edSgmInterPulse.AsInteger();
         break;
      case IDC_TRH_SGMITI :                                    //    update SGM intertrain interval
         hdr.sgm.iTrainIntv = m_pertsPage.m_edSgmInterTrain.AsInteger();
         break;
      case IDC_TRH_MTRINTV :                                   //    update mid-trial reward interval
         hdr.iMTRIntv = m_mainPage.m_edMTRIntv.AsInteger();
         break;
      case IDC_TRH_MTRLEN :                                    //    update mid-trial reward pulse length
         hdr.iMTRLen = m_mainPage.m_edMTRLen.AsInteger();
         break;
      case IDC_TRH_SGMNP :                                     //    update #pulses per SGM pulse train
         hdr.sgm.nPulses = m_pertsPage.m_edSgmNP.AsInteger();
         break;
      case IDC_TRH_SGMNT :                                     //    update #pulse trains per SGM stimulus
         hdr.sgm.nTrains = m_pertsPage.m_edSgmNT.AsInteger();
         break;
      default :                                                //    we should NEVER get here!
         TRACE0( "Bad ID in CCxTrialForm::OnChange!\n" );
         ASSERT( FALSE );
         return;
   }

   BOOL bChanged = FALSE;                                      // set TRUE if trial header is actually changed
   if( !m_pTrial->SetHeader( hdr, bChanged ) )                 // now query trial to make the change; if other params
   {                                                           // had to be corrected as a side effect, we refresh the
      StuffHdrControls();                                      // entire hdr to make sure we reflect all corrections...
      EnableHdrControls();
   }
   else if( (id >= IDC_TRH_KEEP) || (id <= IDC_TRH_MTRMODE) )  // ...else if we clicked a PB, we update its label
   {                                                           // and the show/hide state of selected controls
      StuffHdrPB( hdr, id );
      EnableHdrControls();
   }

   if( bChanged )                                              // if a change was actually made...
   {
      PropagateHeader( id, oldHdr );                           //    propagate change IAW modification mode
      InformModify();                                          //    inform doc/view framework
   }
}


//=== OnComboSelChange ================================================================================================
//
//    Response to the CBN_SELCHANGE notification from the channel configuration combo box IDC_TRH_CHCFG or the SGM op
//    mode combo box IDC_TRH_SGMOP.  Whenever the selected channel config changes, we update the trial's header and
//    list of object dependencies accordingly.  When the SGM op mode changes, we update the header and the enable state
//    of the SGM parameter controls.
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification.
//
//    RETURNS:    NONE.
//
void CCxTrialForm::OnComboSelChange( UINT id )
{
   ASSERT( m_pTrial != NULL );                                          // there must be a trial to edit!!
   TRLHDR hdr;                                                          // get current trial header; remember state
   m_pTrial->GetHeader( hdr );                                          // prior to change in case we must propagate
   TRLHDR oldHdr = hdr;                                                 // change across matching trials

   CCxDoc* pDoc = GetDocument();                                        // the MAESTRO document
   BOOL bChanged = FALSE;                                               // TRUE if trial has changed

   if( id == IDC_TRH_CHCFG )                                            // if channel config has been changed...
   {
      WORD wChanNew = m_mainPage.m_cbSelChan.GetObjKey();
      if( wChanNew != hdr.wChanKey )
      {
         hdr.wChanKey = wChanNew;                                       //    make the change to the trial
         m_pTrial->SetHeader( hdr, bChanged );

         pDoc->UpdateObjDep( m_wKey, m_arDepObjs );                     //    update trial's object dependencies
         m_pTrial->GetDependencies( m_arDepObjs );
      }
   }
   else if( (id == IDC_TRH_SGMOP) &&                                    // if SGM op mode has been changed...
            (hdr.sgm.iOpMode != m_pertsPage.m_cbSgmOp.GetCurSel()) )
   {
      hdr.sgm.iOpMode = m_pertsPage.m_cbSgmOp.GetCurSel();              //    update trial header
      if( !m_pTrial->SetHeader( hdr, bChanged ) ) StuffHdrControls();   //    restuff all ctrls if any auto-corrections
      EnableHdrControls();                                              //    adjust enable state of SGM controls
   }

   if( bChanged )                                                       // if a change was actually made...
   {
      PropagateHeader( id, oldHdr );                                    //    propagate change IAW mod mode
      InformModify();                                                   //    inform doc/view framework
   }
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
//    time.  We don't have to worry about that here, since we don't save it...but what about multithreading effects??]
//
//    ARGS:       pCmdUI -- [in] represents UI item.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnUpdateEditCommand( CCmdUI* pCmdUI )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a
   if ( (pFocusWnd == NULL) ||                                 // CNumEdit obj, then disable all Edit cmds -- no other
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CNumEdit) )) )    // controls on this form support editing.
   {
      pCmdUI->Enable( FALSE );
      return;
   }

   CNumEdit *pEditC = (CNumEdit *) pFocusWnd;                  // obj w/focus is a CNumEdit, so we can safely cast ptr

   BOOL bEnable = FALSE;                                       // enable state of edit command depends on current
   int iStart = 0;                                             // state of the CNumEdit ctrl w/ the input focus:
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
//    time.  We don't have to worry about that here, since we don't save it...but what about multithreading effects??]
//
//    ARGS:       nID -- [in] edit command ID.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnEditCommand( UINT nID )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a
   if ( (pFocusWnd == NULL) ||                                 // CNumEdit obj, do nothing -- all alterable edit ctrls
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CNumEdit) )) )    // on this form are attached to CNumEdit objects!
      return;

   CNumEdit *pEditC = (CNumEdit *) pFocusWnd;                  // obj w/focus is a CNumEdit, so we can safely cast ptr
   switch( nID )                                               // process the operation by calling the appropriate
   {                                                           // CNumEdit method...
      case ID_EDIT_CUT :   pEditC->Cut();    break;
      case ID_EDIT_COPY :  pEditC->Copy();   break;
      case ID_EDIT_PASTE : pEditC->Paste();  break;
      case ID_EDIT_UNDO :  pEditC->Undo();   break;
      default :                              break;
   }
}


//=== OnNMRClick ======================================================================================================
//
//    Response to the NM_RCLICK notification from the segment table or perturbation list grid controls.
//
//    When the user right-clicks on the first column or first row of the segment table grid, we pop up a context menu
//    (submenu 0 of the menu resource IDR_CXPOPUPS -- see CNTRLX resource file), which allows the user to select among
//    a number of different operations to perform on the grid.  Operations may be enabled/disabled depending on what
//    cell was right-clicked -- hence, we must save the coordinates of this "context cell" so that it can be accessed
//    later by the popup menu handling routines OnUpdGridOps() and OnGridOps().
//
//    Similarly, if the user right-clicks on the first column of the perturbation list grid, we pop up a different
//    menu (submenu 3 of IDR_CXPOPUPS) allowing the user to perform operations on that grid.  We again save the coords
//    of the "context cell" so that it can be accessed later by OnUpdGridOps and OnGridOps().
//
//    ARGS:       id       -- [in] control ID of the grid that was right-clicked
//                pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_RCLICK.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnNMRClick( UINT id, NMHDR* pNMHDR, LRESULT* pResult )
{
   NM_GRIDVIEW* pNMGV = (NM_GRIDVIEW*) pNMHDR;                    // save identity of right-clicked cell for later use
   m_contextCell.row = pNMGV->iRow;                               // by the popup menu handling routines...
   m_contextCell.col = pNMGV->iColumn;

   *pResult = TRUE;                                               // return value is irrelevant for NM_RCLICK
   if( m_pTrial == NULL ) return;                                 // ignore when no trial is loaded onto form!

   m_nRightClickedGrid = id;                                      // remember which grid sent the NM_RCLICK

   CPoint mousePt;                                                // get current pos of mouse cursor in screen coords
   ::GetCursorPos( &mousePt );

   CMenu menu;                                                    // load CNTRLX popup menus from resource
   if( !menu.LoadMenu(IDR_CXPOPUPS) )
   {
      m_nRightClickedGrid = 0;
      m_contextCell.row = m_contextCell.col = -1;
      m_iContextSeg = -1;
   }

   if( id == IDC_TRH_SEGTABLE )                                   // right-click on segment table grid:
   {
      m_segGrid.SetFocus();                                       // a right-click does not give grid ctrl the focus

      if( m_contextCell.col == 0 )                                // if clicked on first column, pop up context menu
      {
         CMenu* pPopup = menu.GetSubMenu( 0 );
         pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
               mousePt.x, mousePt.y, AfxGetMainWnd() );
      }
   }
   else if( id == IDC_TRH_PARTITIONS )                            // right-click on partitions grid:
   {
      m_partitionGrid.SetFocus();                                 // grab the focus

      CPoint point(mousePt);                                      // determine which segment is under the mouse
      m_partitionGrid.ScreenToClient( &point );
      point.x -= m_partitionGrid.GetColumnWidth(0);               // first col is header
      if( point.x <= 0 ) m_iContextSeg = -1;
      else m_iContextSeg = point.x / (SEGCOL_W * 2);

      CMenu* pPopup = menu.GetSubMenu( 0 );
      pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, mousePt.x, mousePt.y, AfxGetMainWnd());
   }
   else if( id == IDC_TRH_PERTS )                                 // right-click on perturbation list grid:
   {
      m_pertsPage.m_pertGrid.SetFocus();                          // a right-click does not give grid ctrl the focus

      if( m_contextCell.col == 0 )                                // if clicked on first column, pop up menu...
      {
         CMenu* pPopup = menu.GetSubMenu( 3 );                    // run the popup
         pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
               mousePt.x, mousePt.y, AfxGetMainWnd() );
      }
   }
   else                                                           // right-click from somewhere else? this should
   {                                                              // NEVER happen.  reset right-click context!
      ASSERT( FALSE );
      m_contextCell.row = m_contextCell.col = -1;
      m_iContextSeg = -1;
      m_nRightClickedGrid = 0;
   }
}


//=== OnNMClick =======================================================================================================
//
//    Response to the NM_CLICK notification from the trial partitions grid.
//
//    The method implements the following GUI gesture to create a tagged section:  When the user left-clicks on a cell
//    with the SHIFT key down, that cell is "remembered" as the anchor for specifying a range of cells that will
//    define a new tagged section in the loaded trial.  The anchor cell changes color and a system timer is started.
//    If the user SHIFT-left-clicks on the same cell or another cell before the timer expires, the selected range of
//    cells are merged into a new tagged section.  Note that the cell range could include any combination of
//    individual segments and existing tagged sections!
//
//    SEE ALSO:   HandleSectionCreateGesture().
//
//    ARGS:       pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_CLICK.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnNMClick( NMHDR* pNMHDR, LRESULT* pResult )
{
   *pResult = TRUE;                                               // return value is irrelevant for NM_CLICK
   if( m_pTrial == NULL ) return;                                 // ignore when no trial is loaded onto form!

   BOOL bShift = ::GetKeyState( VK_SHIFT ) & 0x80;                // is SHIFT key down?

   NM_GRIDVIEW* pNMGV = (NM_GRIDVIEW*) pNMHDR;                    // get identity of left-clicked cell
   CCellID clickedCell(pNMGV->iRow, pNMGV->iColumn);

   HandleSectionCreateGesture(bShift, clickedCell);               // inits, completes or cancels the gesture
}


//=== OnTimer =========================================================================================================
//
//    Response to the WM_TIMER message.
//
//    A system timer is used to timeout the "section create gesture" on the partitions grid.  If the user fails to
//    complete the gesture before this timer expires, the gesture is cancelled.  The timer event ID is NM_CLICK.
//
//    SEE ALSO:   Begin/CancelSectionCreateGesture().
//
//    ARGS:       pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_CLICK.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnTimer( UINT_PTR nEventID )
{
   if( nEventID==NM_CLICK )
      CancelSectionCreateGesture();
}


//=== OnGridOps =======================================================================================================
//
//    Menu command handler for the context menu that pops up when the user right-clicks on the first column of the
//    segment table grid ctrl, anywhere on the partitions grid, or the first column of the perturbation list grid
//    (see OnNMRClick()).  What commands are enabled depend on the "context cell" -- the grid cell that was
//    right-clicked -- and which grid contains that cell (see OnUpdGridOps()).  Note that most, but not all, segment
//    table ops (ID_GRID_***) are accessible by right-clicking the partitions grid or the first column of the segment
//    table grid itself.
//
//    Segment table commands:
//       ID_GRID_INSERTTARG ==> Insert a new participating target into trial.  If context cell is a "target selector",
//          the new target is inserted at that location in the table, and existing targets are moved down.  Otherwise,
//          the new target is appended at the bottom of the table.  Selection of this menu item only initiates the
//          grid's inplace tree control by which the user selects a target name; the insertion does not occur until
//          that ctrl is extinguished.  See callback methods GridEditCB() and GridEndEditCB().
//       ID_GRID_INSERTSEG ==> Insert a new segment into trial.  If right-click context identifies a valid segment,
//          then the new segment is inserted at that location in the trial table, and the remaining segments are
//          shifted to the right.  Otherwise, the new segment is appended at the right side of table.
//       ID_GRID_COPYSEG ==> Copy the segment indicated by the right-click context (where user clicked in the
//          partitions grid). A copy of the segment is saved by this view for pasting later; the trial is not changed.
//       ID_GRID_CUTSEG ==> Similar to ID_GRID_COPYSEG, except that the selected segment is removed.
//       ID_GRID_PASTESEG ==> Similar to ID_GRID_INSERTSEG, except that the new segment is a copy of the current
//          "paste" segment.  Enabled only if the current paste segment is compatible with the trial (i.e., the # of
//          traj records in the segment must == the # of targets participating in the trial).
//       ID_GRID_REPLACESEG ==> Similar to ID_GRID_PASTESEG, except that the selected segment is replaced by the
//          current paste segment.  Context cell must by a segment selector.
//       ID_GRID_REMOVESECT ==> If the context cell resides in the partitions grid and corresponds to a tagged section
//          in the trial, that tagged section is removed.
//       ID_GRID_DEL ==> Delete the target or segment referenced the right-click context (right-click on a target
//          selector in the segment table, or right-click in the partitions grid over a segment column).
//       ID_GRID_CLRTARGS, ID_GRID_CLRSEGS, ID_GRID_CLRALL ==>  Clear all participating tgts, all segments, or both
//          from the current trial definition.  In this case the right-click context is irrelevant.
//       ID_GRID_MODIFY...ID_GRID_MODSELTRIALS ==> Change the modification mode. The trial editor is unaffected, except 
//          that the appearance of cell (0,0) in the partitions grid is updated to reflect the new mode.
//
//    NOTE:  Observe that we insert and delete target rows merely by changing the grid control's row count.  This works
//    because the grid control operates in virtual mode, and the grid callback that refreshes the grid's contents
//    ensures that segment table info is stored in the appropriate rows.  Also remember that each segment is
//    represented by two columns, not one.
//
//    Perturbation list commands:
//       ID_PERT_APPEND ==> Append a perturbation waveform object to the trial's perturbation list.  Selection of this
//          menu item initiates the grid's inplace tree control, by which the user selects a perturbation object; the
//          object is not appended to the list until that ctrl is extinguished.  See callback methods PertGridEdtiCB()
//          and PertGridEndEditCB().
//       ID_PERT_REMOVE ==> Remove the perturbation object specified in context cell.
//       ID_PERT_CLEAR ==> Clear the trial's perturbation list.
//
//    !! CAVEAT !!  The code here assumes that the commands listed above represent a continguous range of integers!!
//
//    ARGS:       cmdID -- [in] the command ID
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnGridOps( UINT cmdID )
{
   ASSERT( (m_pTrial != NULL) || (cmdID>=ID_GRID_MODIFY &&     // a trial must be loaded on form, unless we're just
      cmdID<=ID_GRID_MODSELTRIALS) );                          // changing the modification mode

   int iPos, iPos2;                                            // zero-based pos in trial object's seg or tgt lists
   int nRows, nCols;
   BOOL bUpdate = FALSE;                                       // if TRUE, refresh grid and inform other views.
   BOOL bSegChange = FALSE;                                    // if TRUE, #segments in seg table were changed.
   BOOL bTgtChange = FALSE;                                    // if TRUE, #tgts in seg table were changed.

   int nT = 0;                                                 // remember #tgts, #segs, #perts in trial PRIOR to
   int nS = 0;                                                 // change, so we can propagate change across LIKE
   int nP = 0;                                                 // trials, if applicable in current modify mode
   if( m_pTrial != NULL )                                      // we could change mod mode when no trial loaded!
   {
      nT = m_pTrial->TargCount();
      nS = m_pTrial->SegCount();
      nP = m_pTrial->PertCount();
   }

   CCxTrialForm::CellType contextType = NOTACELL;             // BEGIN:  process the command...
   if( m_nRightClickedGrid==IDC_TRH_SEGTABLE )
      contextType = GetCellType( m_contextCell );
   switch( cmdID )
   {
      case ID_GRID_INSERTTARG :                                // 1) Insert/append a target object into trial table:
         if( contextType == TGTSELECT )                        //    if user rt-clk'd tgt selector cell, we'll insert
            m_iInsPos = CellToTarg( m_contextCell );           //    it there; else append it to the tgt list.  note
         else                                                  //    that a nonneg insert pos is what distinguishes a
         {                                                     //    tgt "add" op from a tgt "replace"!!
            m_iInsPos = m_pTrial->TargCount();
            m_contextCell.row = m_contextCell.col = 0;         //    for append, we "edit" the readonly cell(0,0)
         }
         m_segGrid.InitiateCellEdit( m_contextCell.row,        //    start inplace op allowing user to select the
                                     m_contextCell.col );      //    tgt to be added.
         m_contextCell.row = m_contextCell.col = -1;
         m_iContextSeg = -1;
         m_nRightClickedGrid = 0;
         return;                                               //    op is completed in GridEndEditCB()!

      case ID_GRID_INSERTSEG :                                 // 2) Insert/append a new segment, or a copy of the
      case ID_GRID_PASTESEG :                                  //    current paste segment:
         if( m_iContextSeg >= 0 )                              //    if there's a valid context segment, the new seg
            iPos = m_iContextSeg;                              //    is inserted befor it; else appended to seg list
         else
            iPos = -1;

         if( cmdID == ID_GRID_INSERTSEG )
            iPos2 = m_pTrial->InsertSeg( iPos );
         else
            iPos2 = m_pTrial->PasteSeg( iPos, m_pPasteSeg );

         if( iPos2 < 0 ) break;                                //    abort on failure

         nCols = 1 + (2 * m_pTrial->SegCount());               //    update number of columns in grid
         m_segGrid.SetColumnCount( nCols );
         bUpdate = bSegChange = TRUE;
         PropagateSegOp( nT, nS, iPos, cmdID );                //    repeat op for similar trials in TRIALSET mode
         break;

      case ID_GRID_COPYSEG :                                   // 3) Copy/cut selected seg as the new "paste" segment.
      case ID_GRID_CUTSEG :
         ASSERT( m_iContextSeg >= 0);
         iPos = m_iContextSeg;
         if( m_pPasteSeg != NULL )                             //    !! Be sure to free the old paste segment
         {
            delete m_pPasteSeg;
            m_pPasteSeg = NULL;
         }
         if( cmdID == ID_GRID_COPYSEG )
            m_pPasteSeg = m_pTrial->CopySeg( iPos );
         else
         {
            m_pPasteSeg = m_pTrial->CutSeg( iPos );
            if( m_pPasteSeg != NULL )                          //    if segment cut, reduce #cols in grid by two
            {
               nCols = 1 + (2 * m_pTrial->SegCount());
               m_segGrid.SetColumnCount( nCols );
               bUpdate = bSegChange = TRUE;
               PropagateSegOp( nT, nS, iPos, cmdID );          //    remove seg in similar trials in TRIALSET mode
            }
         }
         break;

      case ID_GRID_REPLACESEG :                                // 4) Replace existing segment w/ curr "paste" seg.
         ASSERT( m_iContextSeg >= 0 );
         iPos = m_iContextSeg;
         if( m_pTrial->ReplaceSeg( iPos, m_pPasteSeg ) )       //    NOTE that grid dim unchanged
         {
            bUpdate = TRUE;
            PropagateSegOp( nT, nS, iPos, cmdID );             //    repeat for similar trials in TRIALSET mode
         }
         break;

      case ID_GRID_REMOVESECT :                                // 4a) Remove an existing tagged section from trial.
         ASSERT(m_contextCell.col > 0 && m_nRightClickedGrid==IDC_TRH_PARTITIONS);
         iPos = m_partitions[m_contextCell.col-1].iSection;
         if( m_pTrial->RemoveTaggedSection(iPos) )
         {
            bUpdate = TRUE;
            RebuildPartitionGrid();
         }
         break;

      case ID_GRID_DEL :
         if( contextType == TGTSELECT )                        // 5) Delete a participating target from trial.
         {
            iPos = CellToTarg( m_contextCell );                //    remove specified target from trial; abort on fail
            if( !m_pTrial->RemoveTarget( iPos ) )
               break;
            nRows = ROWSINHDR +                                //    reduce # of grid rows appropriately
                    ROWSINTGT * m_pTrial->TargCount();
            m_segGrid.SetRowCount( nRows );
            bUpdate = bTgtChange = TRUE;
            PropagateTgtOp( nT, nS, iPos, cmdID );             //    repeat for similar trials in TRIALSET mode
         }
         else if( m_iContextSeg >= 0 )                         // 6) Delete an existing segment from trial.
         {
            iPos = m_iContextSeg;                              //    remove specified seg from trial; abort on failure
            if( !m_pTrial->RemoveSeg( iPos ) )
               break;
            nCols = 1 + (2 * m_pTrial->SegCount());            //    reduce # grid cols by two
            m_segGrid.SetColumnCount( nCols );
            bUpdate = bSegChange = TRUE;
            PropagateSegOp( nT, nS, iPos, cmdID );             //    repeat for similar trials in TRIALSET mode
         }

         break;

      case ID_GRID_CLRTARGS :                                  // 7) Remove all participating targets from trial.
         while( m_pTrial->TargCount() > 0 )                    //    clear the trial's target list
            m_pTrial->RemoveTarget( 0 );
         m_segGrid.SetRowCount( ROWSINHDR );                   //    remove all target rows from grid
         bUpdate = bTgtChange = TRUE;
         PropagateTgtOp( nT, nS, -1, cmdID );                  //    repeat for similar trials in TRIALSET mode
         break;

      case ID_GRID_CLRSEGS :                                   // 8) Remove all existing segments from trial.
         while( m_pTrial->SegCount() > 0 )                     //    clear the trial's segment list
            m_pTrial->RemoveSeg( 0 );
         m_segGrid.SetColumnCount( 1 );                        //    remove all segment columns from grid
         bUpdate = bSegChange = TRUE;
         PropagateSegOp( nT, nS, -1, cmdID );                  //    repeat for similar trials in TRIALSET mode
         break;

      case ID_GRID_CLRALL :                                    // 9) Remove all targets AND segments from trial.
         m_pTrial->Clear();                                    //    clear the trial table entirely
         m_segGrid.SetRowCount( ROWSINHDR );                   //    reset the grid
         m_segGrid.SetColumnCount( 1 );
         bUpdate = bSegChange = bTgtChange = TRUE;
         PropagateTgtOp( nT, nS, -1, cmdID );                  //    repeat for similar trials in TRIALSET mode
         break;

      case ID_GRID_MODIFY :                                    // 10) Change the parameter modification mode -- trial
         m_modifyMode = CCxTrialForm::ATOMIC;                  // object is unaffected, but we refresh appearance of
         m_partitionGrid.RedrawCell( 0, 0 );                   // of the visual cue in cell(0,0) of partitions grid
         break;
      case ID_GRID_MODALLSEGS :
         m_modifyMode = CCxTrialForm::ALLSEGS;
         m_partitionGrid.RedrawCell( 0, 0 );
         break;
      case ID_GRID_MODMATCHSEGS :
         m_modifyMode = CCxTrialForm::MATCHSEGS;
         m_partitionGrid.RedrawCell( 0, 0 );
         break;
      case ID_GRID_MODALLTRIALS :
         m_modifyMode = CCxTrialForm::ALLTRIALS;
         m_partitionGrid.RedrawCell( 0, 0 );
         break;
      case ID_GRID_MODMATCHTRIALS :
         m_modifyMode = CCxTrialForm::MATCHTRIALS;
         m_partitionGrid.RedrawCell( 0, 0 );
         break;
      case ID_GRID_MODSELTRIALS :
         m_modifyMode = CCxTrialForm::SELTRIALS;
         m_partitionGrid.RedrawCell( 0, 0 );
         break;

      case ID_PERT_APPEND :                                    // 11) Append a perturbation object to trial
         m_contextCell.row = m_contextCell.col = -1;           //    initiating edit on cell (0,0) distinguishes "add"
         m_pertsPage.m_pertGrid.InitiateCellEdit( 0, 0 );      //    operation from "replace" operation
         return;                                               //    ...op completed in PertGridEndEditCB()!

      case ID_PERT_REMOVE :                                    // 12) Remove selected obj from trial's perturb list.
         m_pertsPage.m_pertGrid.SetFocusCell( -1, -1 );        //    (because we will remove this cell)
         iPos = m_contextCell.row - 1;
         if( m_pTrial->RemovePert( m_contextCell.row - 1 ) )
         {
            m_pertsPage.m_pertGrid.SetRowCount( m_pTrial->PertCount() + 1 );
            bUpdate = TRUE;
            PropagatePertOp( cmdID, iPos, nP );                //    propagate change IAW current modify mode

         }
         break;

      case ID_PERT_CLEAR :                                     // 13) Clear trial's perturbation list.
         m_pertsPage.m_pertGrid.SetFocusCell( -1, -1 );        //    (because we will remove this cell)
         if( m_pTrial->PertCount() > 0 && m_pTrial->RemovePert( -1 ) )
         {
            m_pertsPage.m_pertGrid.SetRowCount( 1 );
            bUpdate = TRUE;
            PropagatePertOp( cmdID, -1, nP );                  //    propagate change IAW current modify mode
         }
         break;

      default :
         ASSERT( FALSE );
         break;
   }                                                           // END:  process the command...


   if( bUpdate )                                               // a change was successfully made, and the update wasn't
   {                                                           // handled elsewhere, so do it now:
      GetDocument()->UpdateObjDep( m_wKey, m_arDepObjs );      //    update trial's object dependencies
      m_pTrial->GetDependencies( m_arDepObjs );

      if( bSegChange || bTgtChange )                           //    resize seg grid whenever #rows or cols changes
         ResizeSegmentTable();

      if( cmdID >= ID_PERT_APPEND )                            //    refresh the affected grid
         m_pertsPage.m_pertGrid.Refresh();
      else if( cmdID != ID_GRID_REMOVESECT )                   //    segment table not affected by this command
         m_segGrid.Refresh();

      if( bSegChange || bTgtChange )                           //    changing # of targets or segments can affect state
      {                                                        //    of hdr ctrls & pert list, so refresh them.
         StuffHdrControls();
         m_pertsPage.m_pertGrid.Refresh();

         if( bSegChange )                                      //    rebuild trial partitions grid when #segs change
            RebuildPartitionGrid();
      }
      InformModify();                                          //    inform doc/views of change
   }

   m_contextCell.row = m_contextCell.col = -1;                 // invalidate right-click context info
   m_nRightClickedGrid = 0;
   m_iContextSeg = -1;
}


//=== OnUpdGridOps ====================================================================================================
//
//    ON_UPDATE_COMMAND_UI_RANGE handler for context menu that pops up when the user right-clicks on a row header in
//    the segment table grid ctrl, anywhere on the partitions grid, or the first column of the perturbation list grid
//    (see OnNMRClick()).  What commands are enabled depend on the "context cell" -- the grid cell that was
//    right-clicked -- and which grid contains that cell.  Note that most, but not all, segment table ops (ID_GRID_***)
//    are accessible by right-clicking the partitions grid or the first column of the segment table grid itself.
//
//    Supported commands (see also: OnGridOps()):
//
//       ID_GRID_INSERTTARG ==> Enabled as long as there's room in the trial for another target.  If context cell is a
//          target selector in the segment table, then menu item label reads "Insert Target Here"; else it reads
//          "Append Target".
//       ID_GRID_INSERTSEG ==> Enabled as long as there's room in the trial for another segment.  If user right-clicked
//          in the partitions grid and the cursor position corresponds to a valid segment, then menu item label reads
//          "Insert Segment Here"; else it reads "Append Segment".
//       ID_GRID_COPYSEG ==> Enabled only if user right-clicked in the partitions grid and the cursor position
//          corresponds to a valid segment.  Menu item label reads "Copy segment <n>", where n is the zero-based index
//          of the segment.
//       ID_GRID_CUTSEG ==> Similar to ID_GRID_COPYSEG, except that label reads "Cut segment <n>".
//       ID_GRID_PASTESEG ==> Similar to ID_GRID_INSERTSEG, but it is disbled if the paste cannot be done.  Menu item
//          label reads "Paste Segment Here" if user right-clicked in partitions grid and cursor position corresponds
//          to a valid segment; else "Append Paste Segment".
//       ID_GRID_REPLACESEG ==> Similar to ID_GRID_PASTESEG, except there must be a "context segment", and the label
//          is "Replace Segment".
//       ID_GRID_REMOVESECT ==> Enabled only if user right-clicked in the partitions grid and the context cell
//          corresponds to a tagged section in the trial.  If so, menu item label reads "Remove section <tag>".
//       ID_GRID_DEL ==> Enabled only if the right-click context is a valid target or a valid segment in the trial.
//          Menu item label reads "Delete <targname>" or "Delete segment <n>".
//       ID_GRIDCLRTARGS, ID_GRID_CLRSEGS, ID_GRID_CLRALL ==>  Enabled as long as there is something appropropiate to
//          remove!
//       ID_GRID_MODIFY...ID_GRID_MODSELTRIALS ==> Always enabled.  These commands merely change the modification mode, 
//          so they do not require that a trial be loaded on the form, or that the form even be visible (note that 
//          these commands have associated keyboard accelerators in the application's accelerator table, which is 
//          loaded by main frame window).  One of the six menu items is checked to indicate the current modification 
//          mode for the segment table.
//
//       ID_PERT_APPEND ==> Enabled as long as there's room in the trial's perturbation list.
//       ID_PERT_REMOVE ==> Enabled as long as context cell contains identity of an existing pert object in the list.
//          Menu item label reads "Remove <name>", where <name> is the pert object's name.
//       ID_PERT_CLEAR ==> Enabled as long as there is at least one pert object in the trial's perturbation list.
//
//    ARGS:       pCmdUI   -- [in] represents UI item.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnUpdGridOps( CCmdUI* pCmdUI )
{
   CFrameWnd* pFrame = GetParentFrame();                          // if parent frame iconicized, or no trial loaded,
   ASSERT( pFrame );                                              // or relevant grid ctrl does not have the focus,
   BOOL bEnable = BOOL(!(pFrame->IsIconic()) && m_pTrial);        // then all ops are disabled...
   if( bEnable )
   {
      CLiteGrid* pFocus = &m_segGrid;
      if( m_nRightClickedGrid == IDC_TRH_PARTITIONS ) pFocus = &m_partitionGrid;
      else if( m_nRightClickedGrid == IDC_TRH_PERTS ) pFocus = &(m_pertsPage.m_pertGrid);

      bEnable = BOOL(pFocus == (CLiteGrid*) GetFocus());
   }

   if( pCmdUI->m_nID >= ID_GRID_MODIFY &&                         // these commands enabled always; they don't require
       pCmdUI->m_nID <= ID_GRID_MODSELTRIALS )                    // that a given grid have the focus or that a trial
      bEnable = TRUE;                                             // be loaded on the form!

   CString strItem;
   CString str;
   CCxTrialForm::CellType contextCellType = NOTACELL;
   if( m_nRightClickedGrid == IDC_TRH_SEGTABLE )
      contextCellType = GetCellType( m_contextCell );

   if( bEnable ) switch( pCmdUI->m_nID )
   {
      case ID_GRID_INSERTTARG :
         strItem = _T("Append Target");
         bEnable = BOOL( m_pTrial->TargCount() < MAX_TRIALTARGS );
         if( bEnable && (contextCellType == TGTSELECT) )
            strItem = _T("Insert Target Here");
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_INSERTSEG :
         strItem = _T("Append Segment");
         bEnable = BOOL( m_pTrial->SegCount() < MAX_SEGMENTS );
         if( bEnable && m_iContextSeg >= 0 )
            strItem = _T("Insert Segment Here");
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_COPYSEG :
      case ID_GRID_CUTSEG :
         bEnable = BOOL(m_iContextSeg >= 0);
         if( bEnable ) str.Format( "%d", m_iContextSeg );
         strItem = (pCmdUI->m_nID == ID_GRID_COPYSEG) ? _T("Copy segment ") : _T("Cut segment ");
         strItem += str;
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_PASTESEG :
         strItem = _T("Append Paste Segment");
         bEnable = m_pTrial->CanPasteSeg( m_pPasteSeg );
         if( bEnable && (m_iContextSeg >= 0) )
            strItem = _T("Paste Segment Here");
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_REPLACESEG :
         bEnable = (m_iContextSeg >= 0) && m_pTrial->CanReplaceSeg( m_pPasteSeg );
         break;

      case ID_GRID_REMOVESECT :
         strItem = _T("Remove section ");
         if( m_nRightClickedGrid==IDC_TRH_PARTITIONS && m_contextCell.col > 0 )
         {
            int iPart = m_contextCell.col - 1;
            if( (iPart < m_nPartitions) &&
                 m_pTrial->GetTaggedSectionName(m_partitions[iPart].iSection, str) )
            {
               bEnable = TRUE;
               strItem += str;
            }
         }
         else
            bEnable = FALSE;
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_DEL :
         strItem = _T("Delete ");
         if( contextCellType == TGTSELECT )
            strItem += m_segGrid.GetItemText( m_contextCell.row, m_contextCell.col );
         else if( m_iContextSeg >= 0 )
         {
            str.Format( "segment %d", m_iContextSeg );
            strItem += str;
         }
         else
            bEnable = FALSE;
         pCmdUI->SetText( strItem );
         break;

      case ID_GRID_CLRTARGS :
         bEnable = BOOL(m_pTrial->TargCount() > 0);
         break;

      case ID_GRID_CLRSEGS :
         bEnable = BOOL(m_pTrial->SegCount() > 0);
         break;

      case ID_GRID_CLRALL :
         bEnable = BOOL( (m_pTrial->TargCount() > 0 ) || (m_pTrial->SegCount() > 0) );
         break;

      case ID_GRID_MODIFY :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::ATOMIC) ? 1: 0 );
         break;
      case ID_GRID_MODALLSEGS :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::ALLSEGS) ? 1: 0 );
         break;
      case ID_GRID_MODMATCHSEGS :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::MATCHSEGS) ? 1: 0 );
         break;
      case ID_GRID_MODALLTRIALS :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::ALLTRIALS) ? 1: 0 );
         break;
      case ID_GRID_MODMATCHTRIALS :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::MATCHTRIALS) ? 1: 0 );
         break;
      case ID_GRID_MODSELTRIALS :
         pCmdUI->SetCheck( (m_modifyMode == CCxTrialForm::SELTRIALS) ? 1: 0 );
         break;

      case ID_PERT_APPEND :
         bEnable = BOOL(m_pTrial->PertCount() < MAX_TRIALPERTS);
         break;
      case ID_PERT_REMOVE :
         strItem = _T("Remove ");
         bEnable = BOOL(m_pTrial->IsValidPert( m_contextCell.row - 1 ));
         if( bEnable )
            strItem += m_pertsPage.m_pertGrid.GetItemText( m_contextCell.row, m_contextCell.col );
         break;
      case ID_PERT_CLEAR :
         bEnable = BOOL(m_pTrial->PertCount() > 0);
         break;
   }

   pCmdUI->Enable( bEnable );
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
//    As part of the one-time inits, we create the modeless property sheet that serves to provide a compact container
//    for the many different widgets that display and edit trial parameters. The property sheet is parented by a
//    placeholder window that's defined in the IDD_TRIALFORM dialog template resource. The actual widgets are located
//    on one of three property pages defined by the helper classes CCxMainPage, CCxPertsPage, CCxRandVarsPage.
// 
//    Below the property sheet is the segment table, consisting of two CLiteGrid objects -- one for the segment grid
//    itself, and one for the partitions grid that serves as a header for the segment table and manages the definition
//    of tagged sections. These are defined as custom controls on the dialog template resource and are dynamically 
//    subclassed to CLiteGrid during the one-time inits. 
//       1) The custom control IDC_TRH_SEGTABLE is subclassed to a CLiteGrid object.  CLiteGrid is derived from the
//          MFC Grid Control class CGridCtrl and is designed to work only in "virtual" mode.  It requires a number of
//          different callback methods to do its work -- those callbacks are installed here.  The grid control is
//          initialized to an "empty" state.
//       1a) Custom control IDC_TRH_PARTITIONS is subclassed to CLiteGrid and appropriate callbacks installed.
//       2) Custom control IDC_TRH_PERTS is subclassed to a CLiteGrid and appropriate callbacks installed.
//    The various widgets on the IDC_TRIALFORM_MAIN and IDC_TRIALFORM_OTHER property pages are subclassed in the 
//    parent page's OnInitDialog() call. Subclassing serves to simplify communication with all the various controls
//    and to take advantage of specialized functionality:
//       3) The combo box IDC_TRH_CHCFG is subclassed to CCxObjCombo, which selects among the CNTRLX child objects
//          under a specified parent.  We use it to select the channel configuration associated with the current trial.
//       4) The combo box IDC_TRH_SGMOP is subclassed to CComboBox.  We stuff the combo box with strings describing
//          the available operational modes for the pulse stimulus generator module, and set the initial selection.
//       5) Certain edit controls on the property pages are subclassed to CNumEdit objects in order to restrict the 
//          input to them. The format traits of these numeric edit controls are also set.
//       6) The spin controls are subclassed to CSpinButtonCtrl objects.
//
//    The "per-document" inits: ensure that the form is emptied each time this method is called (since the previously
//    loaded trial object, if any, was defined in a document that is no longer there!), and reinstall the "tree info"
//    grid callback for each grid (both are handled by a CCxDoc method).
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for initial rows and columns;
//                CNotSupportedException if any control subclassing fails (which should NOT happen!).
//
void CCxTrialForm::OnInitialUpdate()
{
   if( !m_bOneTimeInitsDone )                                              // ONE-TIME INITIALIZATIONS:
   {
      // build the property sheet
      CWnd* pWndSheetParent = GetDlgItem(IDC_TRH_TABPROPS);
      m_pPropSheet = new CPropertySheet(AFX_IDS_APP_TITLE, pWndSheetParent);
      m_pPropSheet->AddPage(&m_mainPage);
      m_pPropSheet->AddPage(&m_pertsPage);
      m_pPropSheet->AddPage(&m_rvPage);
      BOOL bOk = m_pPropSheet->Create(pWndSheetParent, WS_CHILD|WS_VISIBLE, 0);
      if(!bOk)
      {
         delete m_pPropSheet;
         m_pPropSheet = NULL;
      }
      else
      {
         // put tabs at the bottom instead of the top
         m_pPropSheet->GetTabControl()->ModifyStyle(0, TCS_BOTTOM);

         // ensure property sheet fills the placeholder parent window defined in the dialog template resource
         CRect rSheet;
         pWndSheetParent->GetWindowRect(rSheet);
         m_pPropSheet->SetWindowPos(NULL, 0, 0, rSheet.Width(), rSheet.Height(), SWP_NOZORDER|SWP_NOACTIVATE);

         // force creation of each page by making each one the active page. This is because we need to pre-load
         // or enable/disable various controls during start-up.
         m_pPropSheet->SetActivePage(&m_pertsPage);
         m_pPropSheet->SetActivePage(&m_rvPage);
         m_pPropSheet->SetActivePage(&m_mainPage);
      }

      bOk = bOk && m_segGrid.SubclassDlgItem( IDC_TRH_SEGTABLE, this );
      bOk = bOk && m_partitionGrid.SubclassDlgItem( IDC_TRH_PARTITIONS, this );

      if( !bOk ) AfxThrowNotSupportedException();                          // the above must succeed to continue...

      // SET UP THE SEGMENT TABLE GRID CTRL: disable DnD and row/col resizing. Turn off cell selection, and allow
      // focus on fixed cells because the segment header parameters occupy fixed rows.
      m_segGrid.EnableDragAndDrop( FALSE );
      m_segGrid.SetRowResize( FALSE );
      m_segGrid.SetColumnResize( FALSE );
      m_segGrid.SetDefCellWidth( SEGCOL_W );
      m_segGrid.EnableSelection( FALSE );
      m_segGrid.SetAllowFixedFocus( TRUE );

      // set callbacks to govern appearance/editing of grid cells. NOTE passed THIS ref; CB function must be static
      m_segGrid.SetCallbackFunc( GridDispCB, (LPARAM) this );
      m_segGrid.SetEditCBFcn( GridEditCB, (LPARAM) this );
      m_segGrid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );

      // init grid with only fixed rows & column, then configure cells occupying fixed vs non-fixed row/col
      m_segGrid.SetRowCount( ROWSINHDR );
      m_segGrid.SetColumnCount( 1 );
      m_segGrid.SetFixedRowCount( ROWSINHDR );
      m_segGrid.SetFixedColumnCount( 1 );

      m_segGrid.GetDefaultCell( TRUE, TRUE )->SetFormat( DT_RIGHT | DT_SINGLELINE ); 
      m_segGrid.GetDefaultCell( TRUE, FALSE )->SetFormat( DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS );
      m_segGrid.GetDefaultCell( FALSE, TRUE )->SetFormat( DT_RIGHT | DT_SINGLELINE | DT_PATH_ELLIPSIS );
      m_segGrid.GetDefaultCell( FALSE,FALSE )->SetFormat( DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS );

      m_segGrid.AutoSize();

      // save segment grid's initial size, defined in dlg template, to ensure grid is not made smaller than this
      CRect rGrid;
      m_segGrid.GetWindowRect( rGrid );
      ScreenToClient( rGrid );
      m_minGridSize = rGrid.Size();

      // HACK! For some reason, seg table is not drawn correctly initially. This hack fixes the problem.  When
      // the grid is resized again during the call to LoadTrial(), it gets sized correctly.
      rGrid.right = rGrid.left + m_minGridSize.cx + 100;
      rGrid.bottom = rGrid.top + m_minGridSize.cy + 100;
      m_segGrid.MoveWindow( rGrid );

      // SIMILARLY FOR THE TRIAL PARTITIONS GRID:
      m_partitionGrid.EnableDragAndDrop( FALSE );
      m_partitionGrid.SetRowResize( FALSE ); 
      m_partitionGrid.SetColumnResize( FALSE );
      m_partitionGrid.SetDefCellWidth( SEGCOL_W * 2 );
      m_partitionGrid.EnableSelection( FALSE );

      m_partitionGrid.SetCallbackFunc(PartitionGridDispCB, (LPARAM) this);
      m_partitionGrid.SetEditCBFcn( PartitionGridEditCB, (LPARAM) this );
      m_partitionGrid.SetEndEditCBFcn( PartitionGridEndEditCB, (LPARAM) this );

      m_partitionGrid.SetRowCount( 1 );
      m_partitionGrid.SetColumnCount( 1 );
      m_partitionGrid.SetFixedRowCount( 0 );
      m_partitionGrid.SetFixedColumnCount( 1 );

      m_partitionGrid.GetDefaultCell( FALSE, TRUE )->SetFormat( DT_CENTER | DT_SINGLELINE );
      m_partitionGrid.GetDefaultCell( FALSE,FALSE )->SetFormat( DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS ); 

      // so that partitions grid "lines up" with the segment table grid...
      m_partitionGrid.SetColumnWidth(0, m_segGrid.GetColumnWidth(0) );
      m_partitionGrid.SetRowHeight(0, m_segGrid.GetRowHeight(0) );

      // form's scroll size is init'd to size of dlg template in device pixels (form view uses MM_TEXT mapping mode).
      // We consider this the minimum scroll size.
      CSize szPage; 
      CSize szLine; 
      int iMapMode; 
      GetDeviceScrollSizes( iMapMode, m_minScrollSize, szPage, szLine ); 

      // all controls are created in an enabled state
      m_bEnable = TRUE; 

      m_bOneTimeInitsDone = TRUE; 
   }

   // combo box set up to list all channel configs defined in experiment document. Allow "NONE" option.
   m_mainPage.m_cbSelChan.InitContents(GetDocument()->GetBaseObj( CX_CHANBASE ), TRUE);

   // set up form in an "empty state". We reinstall "treeinfo" CBs each time, since we rely on a CCxDoc method!
   LoadTrial( CX_NULLOBJ_KEY );
   m_segGrid.SetTreeInfoCBFcn(CCxDoc::TreeInfoCB, (LPARAM)GetDocument());
   m_pertsPage.m_pertGrid.SetTreeInfoCBFcn(CCxDoc::TreeInfoCB, (LPARAM)GetDocument());
   m_wLastTgtKey = CX_NULLOBJ_KEY;

   // always call the base class version!
   TVTabPane::OnInitialUpdate();
}


//=== OnUpdate [base override] ========================================================================================
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    This form must respond to a number of different "signals" broadcast by other views attached to the CCxDoc obj:
//       CXVH_NEWOBJ:   We refresh the contents of embedded CCxObjCombo in case any new channel config's were created
//                      by the user's actions.
//       CXVH_DSPOBJ:   If a trial object is specified for display, load its definition.
//       CXVH_MODOBJ:   If another view modifies a MAESTRO object, it may send this hint.  If the currently loaded
//                      trial was the object modified, then we must reload the controls on this form from scratch to
//                      ensure that they reflect the current state of the trial.
//       CXVH_NAMOBJ,
//       CXVH_MOVOBJ:   This signal is sent whenever a MAESTRO object is renamed or when its pos in the MAESTRO object
//                      tree is altered, which can affect the "fully qualified" name of the object.  This form must
//                      respond not only to changes in the current trial's name, but also a name change to any
//                      "dependent object" currently assoc with the trial.
//       CXVH_DELOBJ,
//       CXVH_CLRUSR:   If the currently loaded trial is deleted, then the form must be reset.  If a channel config
//                      object is deleted, the embedded CCxObjCombo must be notified so that it can refresh its
//                      contents.  NOTE, however, that it should NOT be possible to delete the currently selected
//                      channel config -- as that would violate the trial's dependency list!
//
//    NOTES:
//    (1) Whenever a hint is NOT provided, we assume the document has been reset.
//
//    ARGS:       pSender  -- [in] view which initiated the update
//                lHint    -- [in] an integer-valued hint (not used by MAESTRO views)
//                pHint    -- [in] if the initiating view provides a hint, this should be a CCxViewHint object.
//
//    RETURNS:    NONE
//
void CCxTrialForm::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   if ( pHint == NULL )                                                 // no hint provided -- assume doc reset
   {
      m_mainPage.m_cbSelChan.InitContents(GetDocument()->GetBaseObj( CX_CHANBASE ), TRUE ); 
      LoadTrial(CX_NULLOBJ_KEY);
      return;
   }

   CCxDoc* pDoc = GetDocument();                                        // get reference to attached doc
   CCxViewHint* pVuHint = (CCxViewHint*)pHint;                          // cast provided hint to MAESTRO hint class

   switch( pVuHint->m_code )
   {
      case CXVH_NEWOBJ :                                                // refresh contents of combo box if it is
         if( pVuHint->m_key == CX_NULLOBJ_KEY ||                        // possible that a channel config(s) was
             pVuHint->m_type == CX_CHANCFG )                            // created
            m_mainPage.m_cbSelChan.RefreshContents();
         break;

      case CXVH_DSPOBJ :                                                // display definition of specified trial
         if( pVuHint->m_type == CX_TRIAL )
         {
            BringToFront();                                             // bring this view to front of tab window; if
            if( m_wKey != pVuHint->m_key )                              // obj is diff from what's currently there,
               LoadTrial( pVuHint->m_key );                             // load the new trial obj.
         }
         break;

      case CXVH_MODOBJ :                                                // trial modified *outside* this view; refresh
         if ( m_wKey == pVuHint->m_key )                                // all controls to make sure they reflect
         {                                                              // trial's current state
            StuffHdrControls();
            EnableHdrControls();

            BOOL bGridChange = FALSE;                                   // TRUE if #rows or #cols changes

            int nRows = ROWSINHDR + ROWSINTGT * m_pTrial->TargCount();  // update # of rows in seg table, if necessary
            if( nRows != m_segGrid.GetRowCount() )
            {
               m_segGrid.SetRowCount( nRows );
               bGridChange = TRUE;
            }

            int nCols = 1 + m_pTrial->SegCount() * 2;                   // update # of cols in seg table, if necessary
            if ( m_segGrid.GetColumnCount() != nCols )
            {
               m_segGrid.SetColumnCount( nCols );
               bGridChange = TRUE;
            }

            if( bGridChange )                                           // before refreshing grid, resize it if #rows
               ResizeSegmentTable();                                    // or #cols changed.

            m_segGrid.Refresh();

            nRows = m_pTrial->PertCount() + 1;                          // update perturbation list grid similarly...
            if( nRows != m_pertsPage.m_pertGrid.GetRowCount() )
               m_pertsPage.m_pertGrid.SetRowCount( nRows );
            m_pertsPage.m_pertGrid.Refresh();

            RebuildPartitionGrid();                                     // rebuild trial partitions grid

            m_pTrial->GetDependencies( m_arDepObjs );                   // make sure our list of trial's dependents is
                                                                        // up to date
         }
         break;

      case CXVH_NAMOBJ :                                                // handle name updates to loaded trial or its
      case CXVH_MOVOBJ :                                                // dependent objects.
         if( m_pTrial != NULL )
         {
            UpdateCaption(NULL);
            m_segGrid.Refresh();
            m_pertsPage.m_pertGrid.Refresh();
         }
         if( pVuHint->m_code == CXVH_NAMOBJ && pVuHint->m_type == CX_CHANCFG )
            m_mainPage.m_cbSelChan.RefreshContents();
         break;

      case CXVH_CLRUSR :                                                // entire document reinitialized; reset form
         m_mainPage.m_cbSelChan.InitContents(GetDocument()->GetBaseObj( CX_CHANBASE ), TRUE );
         LoadTrial(CX_NULLOBJ_KEY);
         break;

      case CXVH_DELOBJ :                                                // if loaded trial was deleted, reset form!
         if ( (m_wKey != CX_NULLOBJ_KEY) &&
              ((pVuHint->m_key==m_wKey) || (!pDoc->ObjExists(m_wKey))) )
            LoadTrial( CX_NULLOBJ_KEY );
         break;

      default :                                                         // no response to any other hints...
         break;
   }
}



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump internal state vars associated with this trial data form view.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxTrialForm::Dump( CDumpContext& dc ) const
{
   TVTabPane::Dump( dc );

   CString msg;
   msg.Format( "\nMin grid size = (%d, %d)", m_minGridSize.cx, m_minGridSize.cy );
   dc << msg;

   msg = _T("\nNo trial displayed currently");
   if ( m_wKey != CX_NULLOBJ_KEY )
      msg.Format( "\nDisplayed trial key = 0x%04x", m_wKey);
   dc << msg;

   msg.Format( "\nDependents array contains %d keys", m_arDepObjs.GetSize() );
   dc << msg;

   if ( m_pPasteSeg == NULL )
      dc << _T("\nThere is currently no paste segment");
   else
   {
      msg.Format( "\nCurrent paste segment contains %d target trajectories", m_pPasteSeg->TrajCount() );
      dc << msg;
   }

   msg.Format( "\nCurrent context cell in the seg table grid: row %d, col %d", m_contextCell.row, m_contextCell.col );
   dc << msg;
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate internal consistency of the trial form view.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxTrialForm::AssertValid() const
{
   TVTabPane::AssertValid();

   if( !m_bLoading )                               // don't enforce these assertions during transient phase as a trial
   {                                               // is loaded onto form or the form is reset
      if( m_wKey == CX_NULLOBJ_KEY )
      {
         ASSERT( m_pTrial == NULL );
         ASSERT( m_bEnable == FALSE );
      }
      else
      {
         ASSERT( m_pTrial != NULL );
         ASSERT( m_bEnable == TRUE );
      }
   }

   ASSERT( (m_pPasteSeg == NULL) ||
           m_pPasteSeg->IsKindOf( RUNTIME_CLASS(CCxSegment) ) );
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== LoadTrial =======================================================================================================
//
//    Load definition of specified trial obj into the form view, updating the form's internal state vars and appearance
//    accordingly.  If no trial is specified (CX_NULLOBJ_KEY), the form is reset to an "empty" state.
//
//    ARGS:       key   -- [in] the unique identifying key of MAESTRO trial obj (CX_NULLOBJ_KEY to clear form).
//
//    RETURNS:    NONE
//
VOID CCxTrialForm::LoadTrial( const WORD key )
{
   m_bLoading = TRUE;                                             // so grid display callbacks will not access stale
                                                                  // trial pointer while loading or resetting form

   m_segGrid.SetFocusCell( -1, -1 );                              // remove focus from a grid cell before modifying
   m_pertsPage.m_pertGrid.SetFocusCell( -1, -1 );                 // grids; avoids an ASSERT in SetRow/ColumnCount()
   m_partitionGrid.SetFocusCell( -1, -1 );                        // below when a different trial is loaded...
   m_rvPage.m_rvGrid.SetFocusCell(-1, -1);

   m_wKey = key;                                                  // unique key of trial to be displayed on form

   if( m_wKey != CX_NULLOBJ_KEY )                                 // if there is a trial to display:
   {
      m_pTrial = (CCxTrial*) GetDocument()->GetObject( m_wKey );  //    get ptr to the trial object
      ASSERT( (m_pTrial != NULL) &&
              (m_pTrial->IsKindOf( RUNTIME_CLASS(CCxTrial) )) );

      m_pTrial->GetDependencies( m_arDepObjs );                   //    get trial's current obj dependencies

      int nRows = ROWSINHDR + ROWSINTGT * m_pTrial->TargCount();  //    set up the right # of tgt rows & seg cols
      int nCols = m_pTrial->SegCount() * 2 + 1;                   //    REM: TWO columns per segment
      m_segGrid.SetRowCount( nRows );
      m_segGrid.SetColumnCount( nCols );

      m_pertsPage.m_pertGrid.SetRowCount(m_pTrial->PertCount()+1); //    set up right #rows in perturbation list grid
   }
   else                                                           // no trial to display: make sure form is cleared...
   {
      m_pTrial = NULL;
      m_arDepObjs.RemoveAll();
      m_segGrid.SetRowCount( ROWSINHDR );
      m_segGrid.SetColumnCount( 1 );
      m_pertsPage.m_pertGrid.SetRowCount( 1 );
   }

   m_bLoading = FALSE;                                            // re-enable grid display callbacks
   EnableHdrControls();                                           // here's where all the real work is done!
   StuffHdrControls();
   ResizeSegmentTable();
   m_segGrid.Refresh();
   m_pertsPage.m_pertGrid.Refresh();
   m_rvPage.m_rvGrid.Refresh();
   CancelSectionCreateGesture();
   RebuildPartitionGrid();
   m_contextCell.row = m_contextCell.col = -1;                    // make sure context cell is reset

   UpdateCaption(NULL);                                           // update assoc tab caption w/ name of trial loaded
}


//=== UpdateCaption [TVTabPane override] ==============================================================================
//
//    Update the caption of the tab item associated with this tab pane.  If a NULL argument is provided, the method
//    will use the name of the object currently loaded; if no obj is loaded, the placeholder title "Trial" is used.
//
//    ARGS:       szCaption   -- [in] desired caption for this tab. Use NULL to get automated caption.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::UpdateCaption( LPCTSTR szCaption )
{
   CString strTitle;
   if( szCaption != NULL )
      strTitle = szCaption;
   else
      strTitle = (m_wKey!=CX_NULLOBJ_KEY) ? GetDocument()->GetObjName( m_wKey ) : _T("Trial");
   TVTabPane::UpdateCaption( strTitle );
}


//=== StuffHdrControls ================================================================================================
//
//    Load all of the "header parameter controls" IAW the current state of the loaded MAESTRO trial.  If no trial is
//    loaded, controls are put in an initial default state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::StuffHdrControls()
{
   TRLHDR   hdr;                                                  // current trial header
   int      nSegs;                                                // # of segs currently defined
   int      nTargs;                                               // # of participating targets currently defined

   if ( m_pTrial != NULL )                                        // get header for currently loaded trial
   {
      m_pTrial->GetHeader( hdr );
      nSegs = m_pTrial->SegCount();
      nTargs = m_pTrial->TargCount();
   }
   else                                                           // or set up default header if no trial is loaded
   {
      hdr.dwFlags = THF_KEEP;
      hdr.iWeight = 1;
      hdr.iStairNum = 0;
      hdr.iStartSeg = 0;
      hdr.iFailsafeSeg = -1;
      hdr.iOpenSeg = -1;                                          // OBSOLETE as of Maestro v2.0.0
      hdr.nOpenSegs = 1;                                          // OBSOLETE as of Maestro v2.0.0
      hdr.iMarkSeg1 = -1;
      hdr.iMarkSeg2 = -1;
      hdr.iSpecialSeg = 0;
      hdr.iSpecialOp = TH_SOP_NONE;
      hdr.reward1[0] = hdr.reward2[0] = TH_DEFREWLEN;
      hdr.reward1[1] = hdr.reward2[1] = TH_DEFWHVR;
      hdr.reward1[2] = hdr.reward2[2] = TH_DEFWHVR + 1;
      hdr.wChanKey = CX_NULLOBJ_KEY;
      hdr.fStairStrength = TH_MINSTAIRSTR;
      hdr.iMTRIntv = TH_DEFREWINTV;
      hdr.iMTRLen = TH_DEFREWLEN;

      hdr.iSGMSeg = 0;
      hdr.sgm.iOpMode = SGM_NOOP;
      hdr.sgm.bExtTrig = FALSE;
      hdr.sgm.iAmp1 = hdr.sgm.iAmp2 = SGM_MAXPA * 80;
      hdr.sgm.iPW1 = hdr.sgm.iPW2 = SGM_MINPW * 10;
      hdr.sgm.iPulseIntv = SGM_MINIPI;
      hdr.sgm.iTrainIntv = SGM_MINITI * 10;
      hdr.sgm.nPulses = SGM_MINPULSES;
      hdr.sgm.nTrains = SGM_MINTRAINS;

      nSegs = 0;
      nTargs = 0;
   }

   // set current channel config selection
   VERIFY( hdr.wChanKey == m_mainPage.m_cbSelChan.SetObjKey(hdr.wChanKey) ); 

   // stuff labels for all PBs reflecting the state of an enumerated parameter
   StuffHdrPB( hdr );

   // load and set range of various spin controls on the "Main" property page
   m_mainPage.m_spinWeight.SetPos( hdr.iWeight );
   m_mainPage.m_spinSave.SetRange( 0, (nSegs==0) ? 0 : nSegs - 1 ); 
   m_mainPage.m_spinSave.SetPos( hdr.iStartSeg );
   m_mainPage.m_spinFailsafe.SetRange( -1, nSegs - 1 );
   m_mainPage.m_spinFailsafe.SetPos( hdr.iFailsafeSeg );
   m_mainPage.m_spinSpecial.SetRange( 0, (nSegs==0) ? 0 : nSegs - 1 );
   m_mainPage.m_spinSpecial.SetPos( hdr.iSpecialSeg );
   m_mainPage.m_spinMark1.SetRange( -1, nSegs - 1 );
   m_mainPage.m_spinMark1.SetPos( hdr.iMarkSeg1 );
   m_mainPage.m_spinMark2.SetRange( -1, nSegs - 1 ); 
   m_mainPage.m_spinMark2.SetPos( hdr.iMarkSeg2 );

   // load various numeric edit controls on the "Main" property page
   m_mainPage.m_edSaccVt.SetWindowText( hdr.iSaccVt ); 
   m_mainPage.m_edRewP1.SetWindowText(hdr.reward1[0]);
   m_mainPage.m_edWHVR1Num.SetWindowText(hdr.reward1[1]);
   m_mainPage.m_edWHVR1Den.SetWindowText(hdr.reward1[2]);
   m_mainPage.m_edRewP2.SetWindowText(hdr.reward2[0]);
   m_mainPage.m_edWHVR2Num.SetWindowText(hdr.reward2[1]);
   m_mainPage.m_edWHVR2Den.SetWindowText(hdr.reward2[2]);
   m_mainPage.m_edStairStren.SetWindowText( hdr.fStairStrength );
   m_mainPage.m_edMTRIntv.SetWindowText( hdr.iMTRIntv );
   m_mainPage.m_edMTRLen.SetWindowText( hdr.iMTRLen );

   // load SGM parameters into various controls on the "Perturbation/PSGM" property page
   m_pertsPage.m_cbSgmOp.SetCurSel( hdr.sgm.iOpMode ); 
   m_pertsPage.m_spinSgmSeg.SetRange( 0, (nSegs==0) ? 0 : nSegs - 1 );
   m_pertsPage.m_spinSgmSeg.SetPos( hdr.iSGMSeg );
   m_pertsPage.m_edSgmPulseAmp1.SetWindowText( hdr.sgm.iAmp1 );
   m_pertsPage.m_edSgmPulseAmp2.SetWindowText( hdr.sgm.iAmp2 );
   m_pertsPage.m_edSgmPulseWidth1.SetWindowText( hdr.sgm.iPW1 );
   m_pertsPage.m_edSgmPulseWidth2.SetWindowText( hdr.sgm.iPW2 );
   m_pertsPage.m_edSgmInterPulse.SetWindowText( hdr.sgm.iPulseIntv );
   m_pertsPage.m_edSgmInterTrain.SetWindowText( hdr.sgm.iTrainIntv );
   m_pertsPage.m_edSgmNP.SetWindowText( hdr.sgm.nPulses );
   m_pertsPage.m_edSgmNT.SetWindowText( hdr.sgm.nTrains );
   int m = hdr.sgm.bExtTrig ? BST_CHECKED : BST_UNCHECKED;
   m_pertsPage.SendDlgItemMessage( IDC_TRH_SGMTRIG, BM_SETCHECK, (WPARAM) m, 0 );

   // check/uncheck boxes reflecting state of the "ignore pos/vel scale/rotate" flags - on "Main" property page
   m = (hdr.dwFlags & THF_IGNPOSSCALE) ? BST_CHECKED : BST_UNCHECKED;
   m_mainPage.SendDlgItemMessage(IDC_TRH_IGNPOSSCALE, BM_SETCHECK, (WPARAM)m, 0);
   m = (hdr.dwFlags & THF_IGNPOSROT) ? BST_CHECKED : BST_UNCHECKED;
   m_mainPage.SendDlgItemMessage(IDC_TRH_IGNPOSROT, BM_SETCHECK, (WPARAM)m, 0);
   m = (hdr.dwFlags & THF_IGNVELSCALE) ? BST_CHECKED : BST_UNCHECKED;
   m_mainPage.SendDlgItemMessage(IDC_TRH_IGNVELSCALE, BM_SETCHECK, (WPARAM)m, 0);
   m = (hdr.dwFlags & THF_IGNVELROT) ? BST_CHECKED : BST_UNCHECKED;
   m_mainPage.SendDlgItemMessage(IDC_TRH_IGNVELROT, BM_SETCHECK, (WPARAM)m, 0);
}


//=== StuffHdrPB ======================================================================================================
//
//    Update labels of all pushbuttons on the form, or one particular PB specified by its resource ID.
//
//    ARGS:       hdr   -- [in] current state of trial header
//                id    -- [in] resource ID of the push button (if 0, all PBs in trial header are refreshed)
//
//    RETURNS:    NONE
//
VOID CCxTrialForm::StuffHdrPB( const TRLHDR& hdr, const UINT id /* = 0 */ )
{
   CString str;

   if ( (id == 0) || (id == IDC_TRH_KEEP) )                          // PB label for keep/toss flag
   {
      str = (hdr.dwFlags & THF_KEEP) ? _T("KEEP") : _T("TOSS");
      m_mainPage.SetDlgItemText( IDC_TRH_KEEP, str );
   }

   if ( (id == 0) || (id == IDC_TRH_TRITYP) )                        // PB label for normal/staircase1-5 designation
   {
      if( hdr.iStairNum == 0 )
         str = _T("NORMAL");
      else
         str.Format( "STAIR%d", hdr.iStairNum );
      m_mainPage.SetDlgItemText( IDC_TRH_TRITYP, str );
   }

   if ( (id == 0) || (id == IDC_TRH_STAIRRESP) )                     // PB label for staircase response channel
   {
      str = (hdr.dwFlags & THF_STAIRRESP) ? _T("ch13") : _T("ch12");
      m_mainPage.SetDlgItemText( IDC_TRH_STAIRRESP, str );
   }

   if ( (id == 0) || (id == IDC_TRH_SPECOP) )                        // PB label reflects special op that's in effect
   {
      str = _T("none");
      if ( hdr.iSpecialOp == TH_SOP_SKIP ) str = _T("skipOnSacc");
      else if ( hdr.iSpecialOp == TH_SOP_SELBYFIX ) str = _T("selByFix");
      else if ( hdr.iSpecialOp == TH_SOP_SELBYFIX2 ) str = _T("selByFix2");
      else if ( hdr.iSpecialOp == TH_SOP_SWITCHFIX ) str = _T("switchFix");
      else if ( hdr.iSpecialOp == TH_SOP_RPDISTRO ) str = _T("R/P Distro");
      else if ( hdr.iSpecialOp == TH_SOP_CHOOSEFIX1 ) str = _T("chooseFix1");
      else if ( hdr.iSpecialOp == TH_SOP_CHOOSEFIX2 ) str = _T("chooseFix2");
      else if ( hdr.iSpecialOp == TH_SOP_SEARCH ) str = _T("searchTask");
      m_mainPage.SetDlgItemText( IDC_TRH_SPECOP, str );
   }

   if ( (id == 0) || (id == IDC_TRH_MTRMODE) )                       // PB label reflects the mid-trial reward mode
   {
      str = (hdr.dwFlags & THF_MTRMODE) ? _T("atSegEnd") : _T("periodic");
      m_mainPage.SetDlgItemText( IDC_TRH_MTRMODE, str );
   }
}


//=== EnableHdrControls ===============================================================================================
//
//    Update enable state of selected "header parameter" controls on the form.
//
//    When a trial is loaded onto/cleared from form, all modifiable controls are enabled/disabled.  In addition, some
//    controls (and associated labels) are dynamically disabled/enabled based on the state of a related parameter:
//       1) if trial type is "normal", then all staircase trial parameter widgets are disabled
//       2) if no sacc-trig'd op is selected, then all related widgets are disabled.
//       3) the second reward pulse length is NOT relevant to the "skipOnSacc" special op; IDC_TRH_REWP2 is disabled
//          in this case. Also, the sacc threshold velocity does NOT apply to the "searchTask" special op.
//       4) the mid-trial reward intv widget is enabled only when the mid-trial reward mode is "periodic".
//       5) not all SGM parameters are relevant to all SGM op modes; only relevant controls are enabled.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxTrialForm::EnableHdrControls()
{
   HWND  hwnd;                                                          // "Windows handle" of a control on form

   if ( (m_pTrial == NULL) && m_bEnable )                               // trial just cleared; disable all modifiable
   {                                                                    // controls...
      m_bEnable = FALSE;

      m_mainPage.m_cbSelChan.EnableWindow( FALSE );
      m_mainPage.m_spinWeight.EnableWindow( FALSE );
      m_mainPage.m_edWeight.EnableWindow( FALSE );
      m_mainPage.m_spinSave.EnableWindow( FALSE );
      m_mainPage.m_spinFailsafe.EnableWindow( FALSE );
      m_mainPage.m_spinMark1.EnableWindow( FALSE );
      m_mainPage.m_spinMark2.EnableWindow( FALSE );
      m_mainPage.m_edRewP1.EnableWindow(FALSE);
      m_mainPage.m_edWHVR1Num.EnableWindow(FALSE);
      m_mainPage.m_edWHVR1Den.EnableWindow(FALSE);
      m_mainPage.GetDlgItem( IDC_TRH_KEEP, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_SPECOP, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_TRITYP, &hwnd );
      ::EnableWindow( hwnd, FALSE );

      m_mainPage.m_spinSpecial.EnableWindow( FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_SPECSEG, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.m_edSaccVt.EnableWindow( FALSE );

      m_mainPage.m_edRewP2.EnableWindow( FALSE );
      m_mainPage.m_edWHVR2Num.EnableWindow(FALSE);
      m_mainPage.m_edWHVR2Den.EnableWindow(FALSE);

      m_mainPage.GetDlgItem( IDC_TRH_MTRMODE, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.m_edMTRIntv.EnableWindow( FALSE );
      m_mainPage.m_edMTRLen.EnableWindow( FALSE );

      m_mainPage.m_edStairStren.EnableWindow( FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_STAIRRESP, &hwnd );
      ::EnableWindow( hwnd, FALSE );

      m_pertsPage.m_cbSgmOp.EnableWindow( FALSE );
      m_pertsPage.m_spinSgmSeg.EnableWindow( FALSE );
      m_pertsPage.m_edSgmPulseAmp1.EnableWindow( FALSE );
      m_pertsPage.m_edSgmPulseAmp2.EnableWindow( FALSE );
      m_pertsPage.m_edSgmPulseWidth1.EnableWindow( FALSE );
      m_pertsPage.m_edSgmPulseWidth2.EnableWindow( FALSE );
      m_pertsPage.m_edSgmInterPulse.EnableWindow( FALSE );
      m_pertsPage.m_edSgmInterTrain.EnableWindow( FALSE );
      m_pertsPage.m_edSgmNP.EnableWindow( FALSE );
      m_pertsPage.m_edSgmNT.EnableWindow( FALSE );
      m_pertsPage.GetDlgItem( IDC_TRH_SGMTRIG, &hwnd );
      ::EnableWindow( hwnd, FALSE );

      m_mainPage.GetDlgItem( IDC_TRH_IGNPOSSCALE, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_IGNPOSROT, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_IGNVELSCALE, &hwnd );
      ::EnableWindow( hwnd, FALSE );
      m_mainPage.GetDlgItem( IDC_TRH_IGNVELROT, &hwnd );
      ::EnableWindow( hwnd, FALSE );
   }

   if ( m_pTrial != NULL )                                              // trial is loaded: update enable state of
   {                                                                    // controls...
      TRLHDR   hdr;                                                     // get current trial header
      m_pTrial->GetHeader( hdr );

      if ( !m_bEnable )                                                 // trial was just loaded, reenable ctrls that
      {                                                                 // are not state-dependent
         m_bEnable = TRUE;

         m_mainPage.m_cbSelChan.EnableWindow( TRUE );
         m_mainPage.m_spinWeight.EnableWindow( TRUE );
         m_mainPage.m_edWeight.EnableWindow( TRUE );
         m_mainPage.m_spinSave.EnableWindow( TRUE );
         m_mainPage.m_spinFailsafe.EnableWindow( TRUE );
         m_mainPage.m_spinMark1.EnableWindow( TRUE );
         m_mainPage.m_spinMark2.EnableWindow( TRUE );
         m_mainPage.m_edRewP1.EnableWindow(TRUE);
         m_mainPage.m_edWHVR1Num.EnableWindow(TRUE);
         m_mainPage.m_edWHVR1Den.EnableWindow(TRUE);
         m_mainPage.GetDlgItem( IDC_TRH_KEEP, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.GetDlgItem( IDC_TRH_SPECOP, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.GetDlgItem( IDC_TRH_TRITYP, &hwnd );
         ::EnableWindow( hwnd, TRUE );

         m_mainPage.GetDlgItem( IDC_TRH_MTRMODE, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.m_edMTRLen.EnableWindow( TRUE );

         m_pertsPage.m_cbSgmOp.EnableWindow( TRUE );

         m_mainPage.GetDlgItem( IDC_TRH_IGNPOSSCALE, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.GetDlgItem( IDC_TRH_IGNPOSROT, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.GetDlgItem( IDC_TRH_IGNVELSCALE, &hwnd );
         ::EnableWindow( hwnd, TRUE );
         m_mainPage.GetDlgItem( IDC_TRH_IGNVELROT, &hwnd );
         ::EnableWindow( hwnd, TRUE );
      }

      // enable special op-related controls as appropriate...
      BOOL bEna = BOOL(hdr.iSpecialOp != TH_SOP_NONE);
      m_mainPage.m_spinSpecial.EnableWindow( bEna );
      m_mainPage.GetDlgItem( IDC_TRH_SPECSEG, &hwnd );
      ::EnableWindow( hwnd, bEna );
      m_mainPage.m_edSaccVt.EnableWindow( bEna );
      m_mainPage.m_edRewP2.EnableWindow(bEna && (hdr.iSpecialOp != TH_SOP_SKIP));
      m_mainPage.m_edWHVR2Num.EnableWindow(bEna && (hdr.iSpecialOp != TH_SOP_SKIP));
      m_mainPage.m_edWHVR2Den.EnableWindow(bEna && (hdr.iSpecialOp != TH_SOP_SKIP));

      bEna = BOOL((hdr.dwFlags & THF_MTRMODE) == 0);                    // mid-trial reward intv enabled only for
      m_mainPage.m_edMTRIntv.EnableWindow( bEna );                      // "periodic" mode

      bEna = BOOL(hdr.iStairNum != 0);                                  // disable staircase-related controls for
      m_mainPage.m_edStairStren.EnableWindow( bEna );                   // normal trials.
      m_mainPage.GetDlgItem( IDC_TRH_STAIRRESP, &hwnd );
      ::EnableWindow( hwnd, bEna );

      BOOL bSgmOn = BOOL( hdr.sgm.iOpMode != SGM_NOOP );                // enable/disable SGM controls depending on SGM
      m_pertsPage.m_spinSgmSeg.EnableWindow( bSgmOn );                  // operational mode
      m_pertsPage.m_edSgmPulseAmp1.EnableWindow( bSgmOn );
      m_pertsPage.m_edSgmPulseWidth1.EnableWindow( bSgmOn );
      m_pertsPage.GetDlgItem( IDC_TRH_SGMTRIG, &hwnd );
      ::EnableWindow( hwnd, bSgmOn );

      bEna = BOOL(bSgmOn && 
         (hdr.sgm.iOpMode == SGM_DUAL || hdr.sgm.iOpMode == SGM_BIPHASIC || hdr.sgm.iOpMode == SGM_BIPHASICTRAIN));
      m_pertsPage.m_edSgmPulseAmp2.EnableWindow( bEna );
      m_pertsPage.m_edSgmPulseWidth2.EnableWindow( bEna );

      bEna = BOOL(bSgmOn && 
         (hdr.sgm.iOpMode == SGM_DUAL || hdr.sgm.iOpMode == SGM_TRAIN || hdr.sgm.iOpMode == SGM_BIPHASICTRAIN));
      m_pertsPage.m_edSgmInterPulse.EnableWindow( bEna );

      bEna = BOOL(bSgmOn && (hdr.sgm.iOpMode == SGM_TRAIN || hdr.sgm.iOpMode == SGM_BIPHASICTRAIN));
      m_pertsPage.m_edSgmInterTrain.EnableWindow( bEna );
      m_pertsPage.m_edSgmNP.EnableWindow( bEna );
      m_pertsPage.m_edSgmNT.EnableWindow( bEna );
   }
}


//=== ResizeSegmentTable ==============================================================================================
//
//    Resize the segment table grid control so it does not need scroll bars, then update form's scroll size so that
//    user can scroll any part of the segment table into view.
//
//    BACKGROUND:  Since the segment table can support up to 30 segments and 25 targets, it can get to be very large.
//    Scrolling is clearly necessary.  However, since CCxTrialForm is set up to be scrolled as well, it can be
//    confusing having two sets of scroll bars present (plus, the grid's scroll bars may be hidden depending on the
//    scroll state of the trial form).  Because the segment table is THE most important control on the form, we decided
//    to adjust its window size whenever necessary (target or segment added/deleted; different trial definition loaded)
//    such that its own scroll bars are hidden.  The scroll sizes of the trial form are then adjusted so that the user
//    can use the form's scroll bars instead to scroll any part of the segment table into view.  ResizeSegmentTable()
//    handles this task.
//
//    The segment table is placed on the trial form's dialog template below all other controls.  Thus, it can freely
//    grow to the right and down without obscuring anything.  When the trial form is first created, we remember the
//    initial size of the segment grid as well as the form's initial scroll size -- these reflect the designed size of
//    grid and form on the dialog template resource.  The segment grid and the form's dynamic scroll size are never
//    made smaller than these sizes.
//
//    The trial partitions grid is resized also, since it should have the same width as the segment table (but a
//    constant height since it only has one row).
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//
VOID CCxTrialForm::ResizeSegmentTable()
{
   // compute segment table grid size required to display content without scroll bars; never let it be smaller than
   // a minimum size determined at startup
   CSize newGridSize( m_segGrid.GetVirtualWidth() + 4, m_segGrid.GetVirtualHeight() + 4 );
   if( newGridSize.cx < m_minGridSize.cx ) newGridSize.cx = m_minGridSize.cx;
   if( newGridSize.cy < m_minGridSize.cy ) newGridSize.cy = m_minGridSize.cy;

   // compute new scroll size for the trial form required so that user can scroll any part of grid into view
   CSize newScrollSize( m_minScrollSize.cx + newGridSize.cx - m_minGridSize.cx,
                        m_minScrollSize.cy + newGridSize.cy - m_minGridSize.cy );

   // first adjust form's scroll size
   SetScrollSizes( MM_TEXT, newScrollSize );

   // now change size of segment grid
   CRect rGridCurr;
   m_segGrid.GetWindowRect( rGridCurr );
   ScreenToClient( rGridCurr );
   rGridCurr.right = rGridCurr.left + newGridSize.cx;
   rGridCurr.bottom = rGridCurr.top + newGridSize.cy;
   m_segGrid.MoveWindow( rGridCurr );

   // change width of partitions grid to keep it in lock step with segment table.  Also make sure it is always tall
   // enough to accommodate the single row without base class inserting a vertical scroll bar!
   CRect rPartGrid;
   m_partitionGrid.GetWindowRect( rPartGrid );
   ScreenToClient( rPartGrid );
   rPartGrid.left = rGridCurr.left;
   rPartGrid.right = rGridCurr.right;
   rPartGrid.bottom = rPartGrid.top + m_partitionGrid.GetVirtualHeight() + 4;
   m_partitionGrid.MoveWindow( rPartGrid );
}


//=== InformModify ====================================================================================================
//
//    Invoke this method to inform the CNTRLX experiment document (CCxDoc) and other attached views that the currently
//    loaded trial object was just modified.
//
//    NOTE:  In the global modification modes ALLTRIALS, MATCHTRIALS, or SELTRIALS, we assume a change has been made 
//    not just to the current trial, but to some or all other trials in its set. In this case, we do not specify a 
//    trial key in the hint object -- indicating that more than one object was modified.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::InformModify()
{
   ASSERT( m_wKey != CX_NULLOBJ_KEY );
   CCxDoc* pDoc = GetDocument();
   pDoc->SetModifiedFlag();
   CCxViewHint vuHint( CXVH_MODOBJ, CX_TRIAL,
         (m_modifyMode==ALLTRIALS || m_modifyMode==MATCHTRIALS || m_modifyMode==SELTRIALS) ? CX_NULLOBJ_KEY : m_wKey );
   pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );
}


//=== PertGridDispCB ==================================================================================================
//
//    Callback function queried by the perturbation list grid ctrl to obtain the contents of each cell in the grid.
//
//    The perturbation list has five columns and N+1 rows, where N is the # of perturbations currently attached to the
//    trial.  The first row merely hold the column headings, while each of the remaining rows describes a perturbation.
//    This description includes:
//       Col 0:  The name of the perturbation object.
//       Col 1:  Gain applied to perturbation (all perturbations are defined as unit amplitude).
//       Col 2:  The start segment for the perturbation.  If not defined, the perturbation is effectively disabled.
//       Col 3:  The target to which the perturbation is applied.  If not defined, perturbation is disabled.
//       Col 4:  Short label indicating the trajectory component to which perturbation is applied.
//
//    When no trial is loaded, the perturbation list grid should be empty except for the fixed row header.  The
//    callback routine works in this case also.
//
//    NOTE:  Callback functions must be implemented as static.  Since it is a static class method, it does not have
//    access to instance fields and methods and it does not get the implied argument THIS.  To circumvent this problem,
//    we take advantage of the generic LPARAM argument, using it to pass a reference to THIS view!!  This is done when
//    we register the callback fcn with the grid in OnInitialUpdate().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info.
//                lParam      -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::PertGridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_pertsPage).m_pertGrid);               // the perturbation list grid
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );               // the grid cell of interest

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )           // ignore when no grid or cell not valid
      return( FALSE );

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                  // we don't use label tips on this grid
   {
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
      return( TRUE );
   }

   if( pThis->m_bLoading )                                              // disable callback while we're changing trial
      return( FALSE );                                                  // that's loaded on form

   int nPert = c.row - 1;                                               // the pos of pert obj in list (if applicable)

   ASSERT( (pTrial!=NULL && nPert<pTrial->PertCount()) || (nPert<0) );  // internal consistency checks
   ASSERT( c.col >= 0 && c.col < 5 );

   WORD wKey;
   int i;

   if( c.row == 0 )                                                     // provide labels for column header row
      pDispInfo->item.strText = strPertListLabels[c.col];
   else switch( c.col )                                                 // provide perturbation param value...
   {
      case 0 :                                                          //    col 0: name of perturbation object
         pDispInfo->item.strText = pThis->GetDocument()->GetObjName( pTrial->GetPertKey( nPert ) );
         break;
      case 1 :                                                          //    col 1: desired amplitude (gain)
         pDispInfo->item.strText.Format( "%.2f", pTrial->GetPertAmp( nPert ) );
         break;
      case 2 :                                                          //    col 2: start segment (0-based index)
         i = pTrial->GetPertSeg( nPert );
         if( i < 0 ) pDispInfo->item.strText = _T("NONE");
         else pDispInfo->item.strText.Format( "%d", i );
         break;
      case 3 :                                                          //    col 3: affected tgt (from 0-based index,
         wKey = pTrial->GetPertTgtKey( nPert );
         if( wKey == CX_NULLOBJ_KEY ) pDispInfo->item.strText = _T("NONE");
         else pDispInfo->item.strText = pThis->GetDocument()->GetObjName( wKey );
         break;
      case 4 :                                                          //    col 4: affected trajectory component
         i = pTrial->GetPertTrajCmpt( nPert );
         pDispInfo->item.strText = strPertAffectedCmptLabels[i] ;
         break;
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                     // show title tip if cell's text doesn't fit
   return( TRUE );
}


//=== PertGridEditCB ==================================================================================================
//
//    Callback invoked to initiate inplace editing of a cell on the perturbation list grid, or to increment/decrement
//    the contents of a cell in response to a right mouse click.
//
//    All "editable" cells in the perturbation list grid are multiple-choice or numeric parameters.  Briefly, this
//    routine permits the following operations on these cells:
//       Col 0 :  Identity of perturbation object.  Cannot be edited by a mouse click.  Edited as a treechoice param,
//          since perturbation objects are selected from the CNTRLX "Perturbations" subtree.  For "replace" operations,
//          we provide a "chain of keys" from the root of this subtree to the key of currently selected perturbation
//          object -- allowing CLiteGrid's inplace tree ctrl to "pop up" with that perturbation initially selected.
//          For "add" operations, the key chain only contains the key of the perturbation subtree root.
//       Col 1 :  Perturbation amplitude.  Right mouse click increments or decrements the current value by 1.  Edited
//          inplace as a floating-point numeric value with two digits' precision.
//       Col 2 :  Start segment.  Right mouse click increments or decrements zero-based index by 1, with wrap-around.
//          Edited as a multichoice param; choice list is "NONE" (-1), "seg0", "seg1", etc.
//       Col 3 :  Affected target.  Right mouse click increments/decrements zero-based index by 1, with wrap.  Edited
//          as multichoice param; choice list is "NONE" (-1) plus names of tgts currently participating in trial.
//       Col 4 :  Affected trajectory component.  Rt click increments/decrements zero-based index by 1, with wrap.
//          Edited as multichoice param; choice list is in CCxTrialForm::strPertAffectedCmptLabels[].
//
//    When a perturbation is being added to the trial, the transient member variable m_iInsPos will hold a nonnegative
//    nonnegative insertion position.  This is what distinguishes the "add" operation from the "replace" operation.
//    The "add" operation is always initiated on cell (0,0), which is normally a read-only cell!  See OnGridOps().
//
//    NOTE:  See also PertGridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::PertGridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_pertsPage).m_pertGrid);               // the perturbation list grid
   CCellID c = pEI->cell;                                               // the cell to be edited
   int nPert = c.row - 1;                                               // which perturbation entry

   if( (pTrial == NULL) || pGrid->GetSafeHwnd() == NULL ||              // if trial not loaded, or grid is gone, or
       !pGrid->IsValid( c ) )                                           // cell not valid, ignore!
      return( FALSE );

   if( c.col < 0 || c.col > 4 ||                                        // cannot edit if cell is not actually editable
       !( c==CCellID(0,0) || pTrial->IsValidPert( nPert ) ) )
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
      return( TRUE );
   }

   int i;
   float f;

   double dOld = 0.0;                                                   // remember old value of a changed param, if we
                                                                        // need to propagate across matching trials

   BOOL bUpdate = FALSE;                                                // set TRUE if trial defn is changed here.
   switch( c.col )
   {
      case 0 :                                                          // col 0:  identity of perturbation
         if( pEI->iClick != 0 )                                         //    cannot edit by mouse click
            pEI->iClick = 0;
         else
         {
            pEI->iType = LG_TREECHOICE;                                 //    edit inplace as "treechoice" parameter
            if( nPert < 0 )                                             //    in this case, we must be adding a pert!
               pEI->dwArKeyChain.Add(
                     pThis->GetDocument()->GetBaseObj( CX_PERTBASE ) );
            else                                                        //    else, we're replacing an existing pert:
               pThis->GetDocument()->PrepareKeyChain( pEI->dwArKeyChain,
                  CX_PERTBASE, pTrial->GetPertTgtKey( nPert ) );
         }
         break;

      case 1 :                                                          // col 1:  perturbation amplitude
         f = pTrial->GetPertAmp( nPert );
         dOld = double(f);
         if( pEI->iClick != 0 )                                         //    increment or decrement on mouse click
         {
            f += (pEI->iClick > 0) ? 1.0f : -1.0f;
            if( !pTrial->SetPertAmp( nPert, f ) ) pEI->iClick = 0;
            else bUpdate = TRUE;
         }
         else                                                           //    edit inplace as numeric text...
         {
            pEI->iType = LG_NUMSTR;
            pEI->numFmt.flags = 0;                                      //    pos or neg FP format, with 2 digits
            pEI->numFmt.nPre = 2;                                       //    precision and up to 7 chars max
            pEI->numFmt.nLen = 7;
            pEI->dCurrent = double( f );
         }
         break;

      case 2 :                                                          // col 2:  start segment
         i = pTrial->GetPertSeg( nPert );
         dOld = double(i);
         if( pTrial->SegCount() == 0 )                                  //    if no segs in trial, can't edit!
         {
            pEI->iClick = 0;
            pEI->iType = LG_READONLY;
         }
         else if( pEI->iClick != 0 )                                    //    increment or decrement on mouse click
         {
            i += (pEI->iClick > 0) ? 1 : -1;
            if( !pTrial->SetPertSeg( nPert, i ) ) pEI->iClick = 0;
            else bUpdate = TRUE;
         }
         else                                                           //    edit inplace as multichoice: "NONE",
         {                                                              //    "seg0", "seg1", ... "segM" for M+1 segs
            pEI->iType = LG_MULTICHOICE;
            pEI->iCurrent = i + 1;                                      //    "NONE" == -1 !!
            pEI->strArChoices.Add( _T("NONE") );
            for( i = 0; i < pTrial->SegCount(); i++ )
            {
               CString str;
               str.Format( "seg%d", i );
               pEI->strArChoices.Add( str );
            }
         }
         break;

      case 3 :                                                          // col 3:  affected target
         i = pTrial->GetPertTgt( nPert );
         dOld = double(i);
         if( pTrial->TargCount() == 0 )                                 //    if no tgts in trial, can't edit!
         {
            pEI->iClick = 0;
            pEI->iType = LG_READONLY;
         }
         else if( pEI->iClick != 0 )                                    //    increment or decrement on mouse click
         {
            i += (pEI->iClick > 0) ? 1 : -1;
            if( !pTrial->SetPertTgt( nPert, i ) ) pEI->iClick = 0;
            else bUpdate = TRUE;
         }
         else                                                           //    edit inplace as multichoice: use target
         {                                                              //    names as the choices; include "NONE"
            pEI->iType = LG_MULTICHOICE;
            pEI->iCurrent = i + 1;                                      //    "NONE == -1 !!
            pEI->strArChoices.Add( _T("NONE") );
            for( i = 0; i < pTrial->TargCount(); i++ )
               pEI->strArChoices.Add( pThis->GetDocument()->GetObjName( pTrial->GetTarget( i ) ) );
         }
         break;

      case 4 :                                                          // col 4:  affected trajectory component
         i = pTrial->GetPertTrajCmpt( nPert );
         dOld = double(i);
         if( pEI->iClick != 0 )                                         //    increment or decrement on mouse click
         {
            i += (pEI->iClick > 0) ? 1 : -1;
            if( !pTrial->SetPertTrajCmpt( nPert, i ) ) pEI->iClick = 0;
            else bUpdate = TRUE;
         }
         else                                                           //    edit inplace as multichoice
         {
            pEI->iType = LG_MULTICHOICE;
            pEI->iCurrent = i;
            for( int j=0; j < PERT_NCMPTS; j++ )
               pEI->strArChoices.Add( strPertAffectedCmptLabels[j] );
         }
         break;
   }

   if( bUpdate )                                                        // if trial defn was changed here,
   {
      pThis->PropagatePertParam( c.col, nPert, dOld );                  //    propagate change IAW modification mode
      pThis->InformModify();                                            //    notify doc/views that trial defn changed
   }

   return( TRUE );
}


//=== PertGridEndEditCB ===============================================================================================
//
//    Callback invoked upon termination of inplace editing of a cell in the perturbation list grid.
//
//    Here we update the loaded trial IAW the change made during the inplace operation that was configured in
//    PertGridEditCB().  When adding a perturbation to the list (in which case the edit cell is (0,0), we increment the
//    #rows in the grid and refresh it.  Inplace editing may continue at a nearby cell location, based on the exit char
//    that terminated the inplace tool and default CLiteGrid keyboard navigation rules.  However, we prevent this
//    continuation when a perturbation was added to the list.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::PertGridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_pertsPage).m_pertGrid);               // the perturbation list grid
   CCellID c = pEEI->cell;                                              // the cell that was edited
   int nPert = c.row - 1;                                               // which perturbation entry
   BOOL bAdd = BOOL( c == CCellID(0,0) );                               // if TRUE, then adding pert obj to the list

   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                   // user cancelled:  nothing to do

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )           // ignore when no grid or cell not valid
      return( FALSE );

   if( pTrial==NULL || c.col<0 || c.col>4 ||                            // cannot edit if cell not really editable
       !(bAdd || pTrial->IsValidPert( nPert )) )
   {
      return( FALSE );
   }

   double dOld = 0.0;                                                   // rem #perts, old value of a param prior to
   int nOld = pTrial->PertCount();                                      // change, in case we need to propagate change

   if( pEEI->bIsChanged || bAdd )                                       // if user actually changed something, or was
   {                                                                    // adding a pert (special case), update trial:
      if( c.col == 0 )                                                  // col 0: replacing/adding a pert obj...
      {
         CCxDoc* pDoc = pThis->GetDocument();                           //    we'll need access to the document here
         WORD wKey = LOWORD(pEEI->dwNew);                               //    the key of pert obj selected by user

         BOOL bOk = TRUE;
         if( bAdd )                                                     //    append new pert, making room on grid...
         {
            bOk = pTrial->AppendPert( wKey );
            if( bOk ) pGrid->SetRowCount( pTrial->PertCount() + 1 );
         }
         else                                                           //    or replace existing pert obj
         {
            dOld = double(pTrial->GetPertKey( nPert ));
            bOk = pTrial->SetPertKey( nPert, wKey );
         }

         if( !bOk )                                                     //    if user made a bad choice, reject it --
         {                                                              //    forcing inplace ctrl to reappear.
            ::MessageBeep( MB_ICONEXCLAMATION );
            pEEI->bReject = TRUE;
            return( TRUE );
         }

         pDoc->UpdateObjDep( pThis->m_wKey, pThis->m_arDepObjs );       //    update trial's object dependencies
         pTrial->GetDependencies( pThis->m_arDepObjs );

         if( bAdd )                                                     //    when we add a pert obj to list:
         {
            pGrid->Refresh();                                           //       refresh grid now
            pEEI->bNoRedraw = TRUE;
            pEEI->nExitChar = 0;                                        //       prevent continuation
         }
      }
      else if( c.col == 1 )                                             // col 1: changed pert amplitude...
      {
         dOld = double(pTrial->GetPertAmp( nPert ));
         pTrial->SetPertAmp( nPert, float(pEEI->dNew) );
      }
      else if( c.col == 2 )                                             // col 2: changed pert start segment...
      {
         dOld = double( pTrial->GetPertSeg( nPert ));
         pTrial->SetPertSeg( nPert, int(pEEI->dwNew) - 1 );             //    "NONE" == -1
      }
      else if( c.col == 3 )                                             // col 3: changed index of affected tgt...
      {
         dOld = double( pTrial->GetPertTgt( nPert ));
         pTrial->SetPertTgt( nPert, int(pEEI->dwNew) - 1 );             //    "NONE" == -1
      }
      else                                                              // col 4: changed id of affected traj cmpt...
      {
         dOld = double( pTrial->GetPertTrajCmpt( nPert ));
         pTrial->SetPertTrajCmpt( nPert, int(pEEI->dwNew) );
      }

      if( bAdd ) pThis->PropagatePertOp( ID_PERT_APPEND, -1, nOld );    // propagate the change IAW modify mode
      else pThis->PropagatePertParam( c.col, nPert, dOld );

      pThis->InformModify();                                            // notify doc/views of change in trial defn
   }

   return( TRUE );
}


//=== GridDispCB ======================================================================================================
//
//    Callback function queried by the embedded grid control to obtain the contents of each cell in the grid.
//
//    The trial table grid is quite complex. Each participating target's trajectory information is displayed in
//    ROWSINTGT rows, and each segment is represented by two columns. The target rows are preceded by ROWSINHDR fixed
//    rows, in which the segment "header" parameters are displayed. The first column is fixed and contains row labels
//    describing the the segment header parameters and the trial target trajectory parameters. The cell occupying the
//    first column in the first row of a target trajectory "rowset" is the "target selector cell". The cell displays
//    the target's name, while a title tip will appear displaying the full "path" name under the MAESTRO "Targets"
//    subtree whenever the mouse hovers over the cell.
//
//    This method is entirely responsible for the grid's appearance, with the help of a number of other methods:
//       GetCellType() ==> Determines the enumerated cell type of a grid cell based on the cell's location.
//       CellToTarg()  ==> Maps cell location to the zero-based index of the relevant target (-1 if not tgt-related).
//       CellToSeg()   ==> Maps cell location to the zero-base index of the relevant segment (-1 if not seg-related).
//       GetCellParamString() ==> Prepares string representation of any trial parameter (segment header field or
//          target trajectory field).
//
//    When no trial is loaded, the grid should be empty except for the segment header labels in the first, fixed column
//    (type SEGHLABEL). The callback routine works in this case also.
//
//    NOTE: Callback functions must be implemented as static. Since it is a static class method, it does not have
//    access to instance fields and methods and it does not get the implied argument THIS. To circumvent this problem,
//    we take advantage of the generic LPARAM argument, using it to pass a reference to THIS view!! This is done when
//    we register the callback fcn with the grid in OnInitialUpdate().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info.
//                lParam      -- [in] THIS (see NOTE)
//
//    RETURNS:    NONE
//
BOOL CALLBACK CCxTrialForm::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam )
{
   // get THIS from lParam and use it to access the grid control, the current trial, etc.
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;
   CCxTrial* pTrial = pThis->m_pTrial; 
   CLiteGrid* pGrid = &(pThis->m_segGrid); 
   CCellID c( pDispInfo->item.row, pDispInfo->item.col ); 

   // ignore when grid control is not realized or current grid cell is not valid
   if(pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid(c)) return(FALSE);

   // we don't use label tips on this grid
   if(pDispInfo->item.nState & GVIS_VIRTUALLABELTIP) { pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP; return(TRUE); }
   
   // disable callback while changing the trial that's loaded on form
   if(pThis->m_bLoading) return(FALSE);

   // indices of the target (if applicable) and segment associated with the current grid cell
   int nTarg = pThis->CellToTarg(c);
   int nSeg = pThis->CellToSeg(c);

   // grid cell type. If no trial loaded, only seg hdr label cells are visible
   CellType cType = pThis->GetCellType(c);
   ASSERT( (pTrial!=NULL) || (cType==NOTACELL) || (cType==SEGHLABEL) );

   CCxTrial::ParamID pID = pThis->GetCellParam(c);
   WORD wKey;
   switch(cType)
   {
      // nothing to do if it's not a cell!
      case NOTACELL :  
         return(FALSE); 
         break;

      // the segment header and trajectory info labels never change
      case SEGHLABEL : 
         pDispInfo->item.strText = strSegHdrLabels[c.row];
         break;
      case TGTJLABEL :
         ASSERT( (c.row - ROWSINHDR) % ROWSINTGT > 0 );
         pDispInfo->item.strText = strTrajLabels[ (c.row - ROWSINHDR) % ROWSINTGT ];
         break;

      // target names are left-aligned with a special bkg color to stand out. Title tip text shows targets's "full" 
      // name, while cell text just shows the base name, clipped if it does not fit into cell rect.
      case TGTSELECT :
         wKey = pTrial->GetTarget( nTarg );
         pDispInfo->item.nFormat = DT_LEFT;  
         pDispInfo->item.crBkClr = clrYellow;  
         if((pDispInfo->item.nState & GVIS_VIRTUALTITLETIP) != 0) 
            pThis->GetDocument()->GetFullObjName(wKey, pDispInfo->item.strText);
         else
         {
            pDispInfo->item.strText = pThis->GetDocument()->GetObjName( wKey );
            pDispInfo->item.nFormat |= DT_END_ELLIPSIS;
         }
         break;

      // format of cell depends on the trial parameter displayed. Bkg color depends on cell position -- and any
      // parameter currently assigned to a trial RV is highlighted with a special bkg color.
      case SEGHFIELD :
      case TGTJFIELD :  
         pTrial->GetSegParam(nSeg, nTarg, pID, pDispInfo->item.strText);
         if(cType == TGTJFIELD && ((c.row - ROWSINHDR) % ROWSINTGT) == 0) pDispInfo->item.crBkClr = clrYellow;
         else if(pTrial->IsRVAssignedToSegParam(nSeg, nTarg, pID)) pDispInfo->item.crBkClr = clrLtGrn;
         else pDispInfo->item.crBkClr = (nSeg % 2 == 0) ? clrWhite : clrMedGray;
         break;
   }

   // except for target selectors, we only show title tip if cell's text is too big to fit...
   if(cType != TGTSELECT) pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP; 
   return(TRUE);
}


//=== GridEditCB ======================================================================================================
//
//    Callback invoked to initiate inplace editing of a cell on the segment table grid, or to increment/decrement the
//    contents of a cell in response to a right mouse click.
//
//    With the exception of the "target selector cells" (CellType::TGTSELECT), all "editable" cells on the trial table
//    grid correspond to multiple-choice or numeric trial parameters.  For such cells, we retrieve a cell's current
//    value and other information required to edit the cell using the inplace editor tools provided by CLiteGrid.
//    Furthermore, this method allows mouse clicks (EDITINFO.iClick nonzero) to increment (right click) or decrement
//    (rt click + SHIFT) the current choice for any multichoice parameter.  Mouse clicks have no effect on any numeric
//    parameter.
//
//    Some trial parameters that are typically numeric can also be assigned one of the trial's 10 random variables.
//    Such parameters require special handling because they can be multi-choice OR numeric. A right-click on a cell
//    displaying an RV-assignable parameter updates the multi-choice index. If the parameter is currently set to a
//    numeric constant, then a right-click sets it to the RV with index 0. If it is already assigned RV at index N,
//    the index is updated to N-1 or N+1. If the new index is invalid, the parameter is restored to a numeric constant.
//    The inplace editor for such parameters will be a combo box or a numeric edit field, depending on the current
//    state of the parameter when editing was initiated.
//
//    The "parameter" displayed in a "target selector" cell is the identity of a participating target.  This is a
//    "treechoice" parameter, since the target object is selected from the MAESTRO "Targets" subtree.  When a target's
//    identity is being changed (a "target replace" operation), we prepare a "chain of keys" from the root of the
//    targets subtree to the key of the currently selected target object -- this allows CLiteGrid's inplace tree ctrl
//    to "pop up" with that target initially selected.  When adding a target, we prepare a similar chain of keys to
//    the last target chosen by the user ("remembered" in member variable m_wLastTgtKey).
//
//    When a target is being inserted or appended to the trial, the transient member variable m_iInsPos will hold a
//    nonnegative target insertion position.  This is what distinguishes the target "add" operation from a target
//    "replace" operation.  Also note that, when appending a target, the edit cell is (0,0), which is normally a
//    read-only cell.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   // get THIS from lParam and use it to access the grid control, the current trial, etc.
   CCxTrialForm* pThis = (CCxTrialForm*)lParam; 
   CCxTrial* pTrial = pThis->m_pTrial;
   CLiteGrid* pGrid = &(pThis->m_segGrid);
   CCellID c = pEI->cell; 

   // ignore if trial not loaded, grid control window is gone, or cell is invalid
   if(pTrial==NULL || pGrid->GetSafeHwnd()==NULL || !pGrid->IsValid(c) ) return(FALSE);

   // get target index, segment index, and ID of parameter displayed in grid cell (as applicable) 
   int nTarg = pThis->CellToTarg(c);  
   int nSeg = pThis->CellToSeg(c); 
   CCxTrial::ParamID pID = pThis->GetCellParam(c);

   // edit by mouse click? -- only if it's an RV assignable param or a multichoice param. L=incr, R=decr.
   if(pEI->iClick != 0) 
   {
      double dOld = 0.0;
      BOOL bChanged = FALSE;
      BOOL wasRV = FALSE;
      if(pTrial->CanAssignRVToSegParam(nSeg, nTarg, pID))
      {
         dOld = pTrial->GetSegParam(nSeg, nTarg, pID);
         int rvIdx = 0;
         if(pTrial->IsRVAssignedToSegParam(nSeg, nTarg, pID))
         {
            rvIdx = pTrial->GetSegParamAsInt(nSeg, nTarg, pID) + ((pEI->iClick > 0) ? 1 : -1);
            wasRV = TRUE;
         }
         pTrial->SetSegParam(nSeg, nTarg, pID, double(rvIdx), TRUE);
         bChanged = TRUE;
      }
      else if(pTrial->IsSegParamMultiChoice(pID))
      {
         int iOld = pTrial->GetSegParamAsInt(nSeg, nTarg, pID);
         dOld = double(iOld);
         pTrial->SetSegParam(nSeg, nTarg, pID, iOld + ((pEI->iClick > 0) ? 1 : -1));
         bChanged = BOOL(iOld != pTrial->GetSegParamAsInt(nSeg, nTarg, pID));
      }
      else
         pEI->iClick = 0;

      // if param changed, propagate IAW modification mode and inform doc/views of change in trial. Also refresh the
      // entire row if modification mode could affect other segments, or if seg min or max dur is changed (since
      // both could be altered)
      if(bChanged)
      {
         pThis->PropagateSegParam(nSeg, nTarg, pID, dOld, wasRV);

         if(pThis->m_modifyMode == CCxTrialForm::ALLSEGS || pThis->m_modifyMode == CCxTrialForm::MATCHSEGS ||
             pID == CCxTrial::MINDURATION || pID == CCxTrial::MAXDURATION)
            pThis->m_segGrid.RedrawRow(c.row);

         pThis->InformModify();
      }
      return(TRUE);
   }

   // get choice list for multi-choice param, num fmt for numeric param. NOTE: An RV-assignable param is only treated
   // as multi-choice when an RV is currently assigned to it; else, it is treated as a simple numeric param.
   BOOL bIsChoice = FALSE; 
   pTrial->GetSegParamFormat(pID, bIsChoice, pEI->strArChoices, pEI->numFmt);
   if(pTrial->CanAssignRVToSegParam(nSeg, nTarg, pID)) bIsChoice = pTrial->IsRVAssignedToSegParam(nSeg, nTarg, pID);

   // prepare for inplace edit: When replacing/inserting/appending a target, use a "treechoice" control. Prepare key
   // chain from "target tree" root to last tgt chosen if we're adding, or to the current tgt obj if replacing.
   if(pThis->IsTargetSelector(c) || (c.row==0 && c.col==0 && pThis->m_iInsPos >= 0))
   {
      // replacing/inserting/appending a target -- a "treechoice" param. Prepare key chain 
      pEI->iType = LG_TREECHOICE;
      WORD wKey = (pThis->m_iInsPos>=0) ? pThis->m_wLastTgtKey : pTrial->GetTarget(pThis->CellToTarg(c));
      pThis->GetDocument()->PrepareKeyChain(pEI->dwArKeyChain, CX_TARGBASE, wKey);
   }
   else if(pID == CCxTrial::NOTAPARAM)
      pEI->iType = LG_READONLY;
   else if(bIsChoice)
   {
      pEI->iType = LG_MULTICHOICE;
      pEI->iCurrent = pTrial->GetSegParamAsInt(nSeg, nTarg, pID);
   }
   else 
   {
      pEI->iType = LG_NUMSTR;
      pEI->dCurrent = pTrial->GetSegParam(nSeg, nTarg, pID);
   }

   return(TRUE);
}


//=== GridEndEditCB ===================================================================================================
//
//    Callback invoked upon termination of inplace editing of a cell in the trial table.
//
//    Here we update the loaded trial IAW the change made during the inplace operation that was configured in
//    GridEditCB().  In addition, we determine which cell is edited next when the exit character that terminated the
//    inplace operation is a recognized keyboard navigation key:
//
//       Arrow key ==>  Go to adjacent (above, below, left, or right) grid cell -- which may or may not be editable.
//    if that cell is editable.
//       TAB key ==>  If editing a parameter in a segment column, go to the cell holding the same parameter in the next
//    segment, wrapping back to segment 0 if necessary.  If modifying the identity of a trial target (not adding a new
//    target), go to the target selector cell for the next target in the participating target list, again wrapping back
//    to the first target if necessary.
//
//    When inserting or appending a trial target, we add the chosen key to the loaded trial object, then add the
//    requisite number of rows required to represent that target's trajectory in the trial.  Remember that the member
//    variable m_iInsPos holds the desired insertion pos for the new target while the inplace op is in progress.  For
//    a target "replace" operation, the insertion pos is -1.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   // get THIS from lParam and use it to access the grid control, the current trial, etc.
   CCxTrialForm* pThis = (CCxTrialForm*)lParam; 
   CCxTrial* pTrial = pThis->m_pTrial;
   CLiteGrid* pGrid = &(pThis->m_segGrid);
   CCellID c = pEEI->cell; 

   // insertion position will be >=0 if inserting a target. Reset transient var in case we abort.
   int iInsPos = pThis->m_iInsPos; 
   pThis->m_iInsPos = -1;
   
   // user cancelled -- nothing to do.
   if(pEEI->nExitChar == VK_ESCAPE) return(TRUE);

   // ignore if trial not loaded, grid control window is gone, or cell is invalid
   if(pTrial==NULL || pGrid->GetSafeHwnd()==NULL || !pGrid->IsValid(c) ) return(FALSE);

   // if user actually changed something, or was adding a tgt (special case), then update trial...
   if(pEEI->bIsChanged || iInsPos >= 0) 
   {
      // get target index, segment index, and ID of parameter displayed in grid cell (as applicable) 
      int nTarg = pThis->CellToTarg(c); 
      int nSeg = pThis->CellToSeg(c); 
      CCxTrial::ParamID pID = pThis->GetCellParam(c); 

      // is the param multi-choice? Any param that was assigned to an RV when editing began is treated as such.
      BOOL wasRV = pTrial->IsRVAssignedToSegParam(nSeg, nTarg, pID);
      BOOL bIsChoice = wasRV || pTrial->IsSegParamMultiChoice(pID);

      // remember old value of parameter for propagating any change
      double dOld = pTrial->GetSegParam(nSeg, nTarg, pID);

      // grid should not redraw itself since we take care of that here
      pEEI->bNoRedraw = TRUE;
      BOOL bAdd = BOOL(iInsPos >= 0); 

      // complete the edit operation...
      if(pThis->IsTargetSelector( c ) || bAdd )                        //    replacing/inserting/appending a target...
      {
         // replace/insert/append target. Get key of target selected by user and validate it.
         CCxDoc* pDoc = pThis->GetDocument(); 
         WORD wTgKey = LOWORD(pEEI->dwNew);  
         BOOL bOk = BOOL((wTgKey != CX_NULLOBJ_KEY) && !pDoc->IsCollectionObj(wTgKey));
         if( bOk )
         {
            // target insertion or replacement pos. If appending, put at end of current target list.
            int nIns = iInsPos;
            if(nIns < 0) nIns = nTarg; 

            // replace existing target, or insert/append new one, making room for new tgt on grid. On replace, remember
            // the key of the target replaced (for propagating the change).
            if(!bAdd)
            {
               dOld = double(pTrial->GetTarget(nIns));
               bOk = pTrial->SetTarget(nIns, wTgKey);
            }
            else
            {
               bOk = pTrial->InsertTarget(nIns, wTgKey);
               if(bOk) pGrid->SetRowCount(ROWSINHDR + pTrial->TargCount() * ROWSINTGT);
            }

            // remember key of last tgt selected by user -- a starting point for the next tgt-select gesture
            pThis->m_wLastTgtKey = wTgKey; 
         }

         // if user made a bad choice, reject it -- forcing inplace ctrl to reappear.
         if(!bOk)
         { 
            ::MessageBeep(MB_ICONEXCLAMATION);
            pEEI->bReject = TRUE;
            pThis->m_iInsPos = iInsPos;
            return(TRUE);
         }

         // prevent continuation when we successfully add a target. Segment table must be resized.
         if(bAdd) 
         {
            pEEI->nExitChar = 0; 
            pThis->ResizeSegmentTable(); 
         }

         // refresh grid, trial header controls, and perturbation list to ensure trial form is visibly up-to-date.
         pGrid->Refresh();
         (pThis->m_pertsPage).m_pertGrid.Refresh();
         pThis->StuffHdrControls();

         // update trial's object dependencies
         pDoc->UpdateObjDep(pThis->m_wKey, pThis->m_arDepObjs);
         pTrial->GetDependencies(pThis->m_arDepObjs);
      }
      else if(bIsChoice)
         pTrial->SetSegParam(nSeg, nTarg, pID, int(pEEI->dwNew), wasRV);
      else 
         pTrial->SetSegParam(nSeg, nTarg, pID, pEEI->dNew);

      // propagate the change IAW the current modification mode. (NOTE: for tgt replace, pID = NOTAPARAM.)
      if(bAdd) pThis->PropagateTgtOp(pTrial->TargCount() - 1, pTrial->SegCount(), iInsPos, ID_GRID_INSERTTARG);
      else pThis->PropagateSegParam(nSeg, nTarg, pID, dOld, wasRV);

      // for seg table params other than the target key, we refresh only the cell or the entire row, depending on the
      // modification mode. Also, changing seg min or max dur may alter the other bound.
      if(pTrial->IsValidSegParam(nSeg, nTarg, pID))
      { 
         if(pThis->m_modifyMode == CCxTrialForm::ALLSEGS || pThis->m_modifyMode == CCxTrialForm::MATCHSEGS ||
             pID == CCxTrial::MINDURATION || pID == CCxTrial::MAXDURATION)
            pThis->m_segGrid.RedrawRow(c.row);
         else
            pThis->m_segGrid.RedrawCell(c);
      }

      pThis->InformModify(); 
   }

   // determine next cell to edit (if any)...
   if(pEEI->nExitChar == VK_UP) --c.row;
   else if(pEEI->nExitChar == VK_DOWN) ++c.row;
   else if(pEEI->nExitChar == VK_LEFT) --c.col; 
   else if(pEEI->nExitChar == VK_RIGHT) ++c.col;
   else if(pEEI->nExitChar == VK_TAB)
   {
      if(pThis->IsTargetSelector(c))
      {
         // if we just modified a target, tab to next target slot, wrapping to top if necessary
         int nTarg = pThis->CellToTarg( c );
         if(nTarg < pTrial->TargCount() - 1) c.row += ROWSINTGT;
         else c.row = ROWSINHDR;
      }
      else 
      {
         // if if we just modified a seg parameter, tab to that same parameter in the next seg. Each seg has two cols!
         int nSeg = pThis->CellToSeg(c);
         if(nSeg < pTrial->SegCount() - 1) c.col += 2;
         else c.col = (c.col % 2 == 0) ? 2 : 1;
      }

      // never navigate back to the cell just edited!
      if(c == pEEI->cell ) c.row = c.col = -1;
   }
   else
      c.row = c.col = -1;
   pEEI->cellNext = c;

   return(TRUE);
}


//=== GetCellType =====================================================================================================
//
//    Returns enumerated type of cell based on its position in the segment table grid.
//
//    ARGS:       c  -- [in] the grid cell.
//
//    RETURNS:    type of info displayed in cell
//
CCxTrialForm::CellType  CCxTrialForm::GetCellType( const CCellID& c ) const
{
   int iTgt = CellToTarg( c );                                    // get index of tgt & seg corresponding to cell; we
   int iSeg = CellToSeg( c );                                     // need to make sure these indices are still valid
   int nTgts = (m_pTrial != NULL) ? m_pTrial->TargCount() : 0;    // for the loaded trial -- the trial & grid could
   int nSegs = (m_pTrial != NULL) ? m_pTrial->SegCount() : 0;     // be out of synch during a delete op, e.g.

   if( !m_segGrid.IsValid(c) )         return( NOTACELL );
   else if( iTgt >= nTgts )            return( NOTACELL );
   else if( iSeg >= nSegs )            return( NOTACELL );
   else if( c.col == 0 )
   {
      int i = c.row - ROWSINHDR;
      if( c.row < ROWSINHDR )          return( SEGHLABEL );
      else if( (i%ROWSINTGT) == 0 )    return( TGTSELECT );
      else                             return( TGTJLABEL );
   }
   else if( c.row < ROWSINHDR )        return( SEGHFIELD );
   else                                return( TGTJFIELD );
}


//=== GetCellParam ====================================================================================================
//
//    Returns enumerated type mapping segment grid cell to the type of segment table parameter displayed in that cell.
//
//    ARGS:       c  -- [in] the grid cell.
//
//    RETURNS:    ID indicating which trial parameter is displayed in the cell.
//
CCxTrial::ParamID CCxTrialForm::GetCellParam( const CCellID& c ) const
{
   if( c.col == 0 || GetCellType( c ) == NOTACELL )                  // invalid cell, or row heading
      return( CCxTrial::NOTAPARAM );

   BOOL bIsLeft = BOOL(c.col % 2 == 1);                              // in left or right col of seg col-pair?

   CCxTrial::ParamID paramID = CCxTrial::NOTAPARAM;
   if( c.row < ROWSINHDR ) switch( c.row )                           // segment header parameters...
   {
      case 0 : paramID = (bIsLeft) ? CCxTrial::MINDURATION : CCxTrial::MAXDURATION; break;
      case 1 : paramID = (bIsLeft) ? CCxTrial::NOTAPARAM : CCxTrial::RMVSYNCENA; break;
      case 2 : paramID = (bIsLeft) ? CCxTrial::FIXTARG1 : CCxTrial::FIXTARG2; break;
      case 3 : paramID = (bIsLeft) ? CCxTrial::FIXACCH : CCxTrial::FIXACCV; break;
      case 4 : paramID = (bIsLeft) ? CCxTrial::FIXGRACE : CCxTrial::REWENA; break;
      case 5 : paramID = (bIsLeft) ? CCxTrial::SEGMARKER : CCxTrial::CHECKRESP; break;
   }
   else switch( (c.row - ROWSINHDR) % ROWSINTGT )                    // target trajectory record parameters...
   {
      case 0 : paramID = (bIsLeft) ? CCxTrial::TGTONOFF : CCxTrial::TGTPOSABS; break;
      case 1 : paramID = (bIsLeft) ? CCxTrial::TGTVSTABMODE : CCxTrial::TGTVSTABSNAP; break;
      case 2 : paramID = (bIsLeft) ? CCxTrial::TGTHPOS : CCxTrial::TGTVPOS; break;
      case 3 : paramID = (bIsLeft) ? CCxTrial::TGTHVEL : CCxTrial::TGTVVEL; break;
      case 4 : paramID = (bIsLeft) ? CCxTrial::TGTHACC : CCxTrial::TGTVACC; break;
      case 5 : paramID = (bIsLeft) ? CCxTrial::PATHVEL : CCxTrial::PATVVEL; break;
      case 6 : paramID = (bIsLeft) ? CCxTrial::PATHACC : CCxTrial::PATVACC; break;
   }

   return( paramID );
}


//=== Propagate*** ====================================================================================================
//
//    These methods propagate changes in the currently loaded trial's definition IAW the current modification mode. We
//    support the following "global" modification modes:
//
//       ALLSEGS:    Change in a segment table parameter P is propagated across all segments of the current trial.
//       MATCHSEGS:  Change in seg table param P from P0->P1 is repeated for all segments such that P=P0.
//       ALLTRIALS:  Change in trial definition is propagated across all trials in the loaded trial's set.  This mode
//    applies to all aspects of the trial's definition.  A change in a trial header parameter (including perturbation
//    list) is propagated across all trials in the set.  A change in a segment header param in segment N is propagated
//    across all trials for which segment N exists.  A change in the target trajectory parameter for the Mth target in
//    segment N is propagated across all trials for which there is an Mth target and an Nth segment.  Finally, any
//    structural changes in the segment table are propagated only across trials that have the same # of targets and
//    segments as the loaded trial did PRIOR to the change, and any structural changes in the perturbation list are
//    propagated across those trials that have the same # of perturbations as the loaded trial had PRIOR to the change.
//       MATCHTRIALS: Change in trial definition is propagated across all trials in the set that have a MATCHING
//    parameter value.  A change in a trial header parameter from P=P0->P1 is propagated across those trials in the set
//    for which P=P0.  A change in a segment header param in segment N from P=P0->P1 is propagated across all trials
//    for which segment N exists AND for which P=P0.  A change in the target trajectory parameter P=P0->P1 for the Mth
//    target in segment N is propagated across all trials for which there is an Mth target and an Nth segment AND for
//    which P=P0.  Structural changes in segment table and perturbation list are propagated as in the ALLTRIALS mode.
//       SELTRIALS:  Same as ALLTRIALS, but applies only to trials in the edited trial's set that are currently 
//    selected in Maestro's object tree.
//
//    NOTE: We use PropagateSegParam() to propagate a change in a trial target's identity, even though this "parameter"
//    does not have an enumerated type.  For this special case, pID == CCxTrial::NOTAPARAM.  See GridEndEditCB().
//
//    ARGS:       ctrlID   -- [in] ID of a trial header parameter control that was just changed.
//                oldHdr   -- [in] loaded trial's header prior to making the change.
//                iSeg     -- [in] index of the relevant segment of trial.
//                iTgt     -- [in] index of the relevant target in the trial's target list.
//                pID      -- [in] enumerated type of the parameter modified in the segment table.
//                dOldVal  -- [in] old value of the param modified in segment table (for MATCHSEGS mode).
//                wasRV    -- [in] TRUE if modified param was assigned to an RV rather than a constant (so that dOldVal
//                            is actually the index of the RV to which it was assigned).
//                iPert    -- [in] index of the relevant perturbation in the trial's perturbation list.
//                iCol     -- [in] col# of the parameter that was modified in the perturbation table.
//                cmdID    -- [in] ID of the command just completed on the loaded trial's segment or pert table.
//                nT,nS,nP -- [in] #tgts, #segs, and #perts in the loaded trial PRIOR to the just-completed
//                            structural change in that trial's segment table or perturbation list.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::PropagateHeader( UINT ctrlID, TRLHDR& oldHdr )
{
   if( m_modifyMode != ALLTRIALS && m_modifyMode != MATCHTRIALS &&         // nothing to do; trial header changes are
       m_modifyMode != SELTRIALS )                                         // only propagated across trial set
      return;
   ASSERT( m_pTrial );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();      // in SELTRIALS mode, we query mainframe wnd
   if(m_modifyMode == SELTRIALS && pFrame == NULL)                         // to see if a given trial is selected.
      return;

   TRLHDR hdrLoaded;                                                       // loaded trial's current header parameters
   m_pTrial->GetHeader( hdrLoaded );

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                   // the current open document
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj( m_wKey ) );  // traverse all trials in set that holds the
   while( pos != NULL )                                                    // trial currently loaded in the form:
   {
      WORD wKey;
      CCxTrial* pTrial;
      pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
      if( wKey == m_wKey ) continue;                                       //    skip over the currently loaded trial!

      TRLHDR hdr;                                                          //    get trial's header param set
      pTrial->GetHeader( hdr );
      CWordArray wArDeps;                                                  //    in case we must update dependencies

      BOOL bModify = BOOL(m_modifyMode == ALLTRIALS);
      if(m_modifyMode == SELTRIALS)                                        //    in SELTRIALS mode, skip over trials 
      {                                                                    //    that aren't selected in object tree
         if(!pFrame->IsObjectSelected(wKey)) continue;
         bModify = TRUE;
      }

      int idx = 0;

      switch( ctrlID )                                                     //    update param changed so it = value of
      {                                                                    //    corres param in loaded trial...
         case IDC_TRH_KEEP :
            if( bModify || ((hdr.dwFlags & THF_KEEP) == (oldHdr.dwFlags & THF_KEEP)) )
            {
               hdr.dwFlags &= ~THF_KEEP;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_KEEP);
            }
            break;
         case IDC_TRH_STAIRRESP :
            if( bModify || ((hdr.dwFlags & THF_STAIRRESP) == (oldHdr.dwFlags & THF_STAIRRESP)) )
            {
               hdr.dwFlags &= ~THF_STAIRRESP;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_STAIRRESP);
            }
            break;
         case IDC_TRH_SPECOP :
            if( bModify || (hdr.iSpecialOp == oldHdr.iSpecialOp) )
               hdr.iSpecialOp = hdrLoaded.iSpecialOp;
            break;
         case IDC_TRH_MTRMODE :
            if( bModify || ((hdr.dwFlags & THF_MTRMODE) == (oldHdr.dwFlags & THF_MTRMODE)) )
            {
               hdr.dwFlags &= ~THF_MTRMODE;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_MTRMODE);
            }
            break;
         case IDC_TRH_IGNPOSSCALE :
            if( bModify || ((hdr.dwFlags & THF_IGNPOSSCALE) == (oldHdr.dwFlags & THF_IGNPOSSCALE)) )
            {
               hdr.dwFlags &= ~THF_IGNPOSSCALE;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_IGNPOSSCALE);
            }
            break;
         case IDC_TRH_IGNPOSROT :
            if( bModify || ((hdr.dwFlags & THF_IGNPOSROT) == (oldHdr.dwFlags & THF_IGNPOSROT)) )
            {
               hdr.dwFlags &= ~THF_IGNPOSROT;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_IGNPOSROT);
            }
            break;
         case IDC_TRH_IGNVELSCALE :
            if( bModify || ((hdr.dwFlags & THF_IGNVELSCALE) == (oldHdr.dwFlags & THF_IGNVELSCALE)) )
            {
               hdr.dwFlags &= ~THF_IGNVELSCALE;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_IGNVELSCALE);
            }
            break;
         case IDC_TRH_IGNVELROT :
            if( bModify || ((hdr.dwFlags & THF_IGNVELROT) == (oldHdr.dwFlags & THF_IGNVELROT)) )
            {
               hdr.dwFlags &= ~THF_IGNVELROT;
               hdr.dwFlags |= (hdrLoaded.dwFlags & THF_IGNVELROT);
            }
            break;
         
         case IDC_TRH_TRITYP :
            if( bModify || hdr.iStairNum == oldHdr.iStairNum )
               hdr.iStairNum = hdrLoaded.iStairNum;
            break;
         case IDC_TRH_SGMTRIG :
            if( bModify || hdr.sgm.bExtTrig == oldHdr.sgm.bExtTrig )
               hdr.sgm.bExtTrig = hdrLoaded.sgm.bExtTrig;
            break;
         case IDC_TRH_WTSPIN :
         case IDC_TRH_WEIGHT :
            if( bModify || hdr.iWeight == oldHdr.iWeight )
               hdr.iWeight = hdrLoaded.iWeight;
            break;
         case IDC_TRH_SAVESEGSPIN :
            if( bModify || hdr.iStartSeg == oldHdr.iStartSeg )
               hdr.iStartSeg = hdrLoaded.iStartSeg;
            break;
         case IDC_TRH_FAILSAFESPIN :
            if( bModify || hdr.iFailsafeSeg == oldHdr.iFailsafeSeg )
               hdr.iFailsafeSeg = hdrLoaded.iFailsafeSeg;
            break;
         case IDC_TRH_SPECSEGSPIN :
            if( bModify || hdr.iSpecialSeg == oldHdr.iSpecialSeg )
               hdr.iSpecialSeg = hdrLoaded.iSpecialSeg;
            break;
         case IDC_TRH_MARK1SPIN :
            if( bModify || hdr.iMarkSeg1 == oldHdr.iMarkSeg1 )
               hdr.iMarkSeg1 = hdrLoaded.iMarkSeg1;
            break;
         case IDC_TRH_MARK2SPIN :
            if( bModify || hdr.iMarkSeg2 == oldHdr.iMarkSeg2 )
               hdr.iMarkSeg2 = hdrLoaded.iMarkSeg2;
            break;
         case IDC_TRH_SGMSEGSPIN :
            if( bModify || hdr.iSGMSeg == oldHdr.iSGMSeg )
               hdr.iSGMSeg = hdrLoaded.iSGMSeg;
            break;

         case IDC_TRH_SACCVT :
            if( bModify || hdr.iSaccVt == oldHdr.iSaccVt )
               hdr.iSaccVt = hdrLoaded.iSaccVt;
            break;
         case IDC_TRH_STAIRSTREN :
            if( bModify || hdr.fStairStrength == oldHdr.fStairStrength )
               hdr.fStairStrength = hdrLoaded.fStairStrength;
            break;
         case IDC_TRH_REWP1 :
         case IDC_TRH_WHVR1NUM :
         case IDC_TRH_WHVR1DEN :
            idx = (ctrlID == IDC_TRH_WHVR1NUM) ? 1 : (ctrlID == IDC_TRH_WHVR1DEN ? 2 : 0);
            if( bModify || hdr.reward1[idx] == oldHdr.reward1[idx] )
               hdr.reward1[idx] = hdrLoaded.reward1[idx];
            break;
         case IDC_TRH_REWP2:
         case IDC_TRH_WHVR2NUM:
         case IDC_TRH_WHVR2DEN:
            idx = (ctrlID == IDC_TRH_WHVR2NUM) ? 1 : (ctrlID == IDC_TRH_WHVR2DEN ? 2 : 0);
            if(bModify || hdr.reward2[idx] == oldHdr.reward2[idx])
               hdr.reward2[idx] = hdrLoaded.reward2[idx];
            break;
         case IDC_TRH_MTRINTV :
            if( bModify || hdr.iMTRIntv == oldHdr.iMTRIntv )
               hdr.iMTRIntv = hdrLoaded.iMTRIntv;
            break;
         case IDC_TRH_MTRLEN :
            if( bModify || hdr.iMTRLen == oldHdr.iMTRLen )
               hdr.iMTRLen = hdrLoaded.iMTRLen;
            break;
         case IDC_TRH_SGMPA1 :
            if( bModify || hdr.sgm.iAmp1 == oldHdr.sgm.iAmp1 )
               hdr.sgm.iAmp1 = hdrLoaded.sgm.iAmp1;
            break;
         case IDC_TRH_SGMPA2 :
            if( bModify || hdr.sgm.iAmp2 == oldHdr.sgm.iAmp2 )
               hdr.sgm.iAmp2 = hdrLoaded.sgm.iAmp2;
            break;
         case IDC_TRH_SGMPW1 :
            if( bModify || hdr.sgm.iPW1 == oldHdr.sgm.iPW1 )
               hdr.sgm.iPW1 = hdrLoaded.sgm.iPW1;
            break;
         case IDC_TRH_SGMPW2 :
            if( bModify || hdr.sgm.iPW2 == oldHdr.sgm.iPW2 )
               hdr.sgm.iPW2 = hdrLoaded.sgm.iPW2;
            break;
         case IDC_TRH_SGMIPI :
            if( bModify || hdr.sgm.iPulseIntv == oldHdr.sgm.iPulseIntv )
               hdr.sgm.iPulseIntv = hdrLoaded.sgm.iPulseIntv;
            break;
         case IDC_TRH_SGMITI :
            if( bModify || hdr.sgm.iTrainIntv == oldHdr.sgm.iTrainIntv )
               hdr.sgm.iTrainIntv = hdrLoaded.sgm.iTrainIntv;
            break;
         case IDC_TRH_SGMNP :
            if( bModify || hdr.sgm.nPulses == oldHdr.sgm.nPulses )
               hdr.sgm.nPulses = hdrLoaded.sgm.nPulses;
            break;
         case IDC_TRH_SGMNT :
            if( bModify || hdr.sgm.nTrains == oldHdr.sgm.nTrains )
               hdr.sgm.nTrains = hdrLoaded.sgm.nTrains;
            break;
         
         case IDC_TRH_SGMOP :
            if( bModify || hdr.sgm.iOpMode == oldHdr.sgm.iOpMode )
               hdr.sgm.iOpMode = hdrLoaded.sgm.iOpMode;
            break;

         case IDC_TRH_CHCFG :
            pTrial->GetDependencies( wArDeps );                                  // this can change trial dependencies;
            if( bModify || hdr.wChanKey == oldHdr.wChanKey )                     // get dependencies list first so we
               hdr.wChanKey = hdrLoaded.wChanKey;                                // can update them afterwards.
            break;
      }

      BOOL bChanged;
      pTrial->SetHeader( hdr, bChanged );                                  // update trial w/ modified header
      if( ctrlID == IDC_TRH_CHCFG )                                        // in this case, we must update trial's
         pDoc->UpdateObjDep( wKey, wArDeps );                              // dependencies since they may have changed
   }
}

VOID CCxTrialForm::PropagateSegParam( int iSeg, int iTgt, CCxTrial::ParamID pID, double dOldVal, BOOL wasRV)
{
   if( m_modifyMode == ATOMIC ) return;                                    // changes aren't propagated in this mode
   ASSERT( m_pTrial );

   double dNewVal = m_pTrial->GetSegParam( iSeg, iTgt, pID );              // get new value of parameter
   BOOL asRV = m_pTrial->IsRVAssignedToSegParam(iSeg, iTgt, pID);          // is the parameter assigned to an RV?

   int i;
   if( m_modifyMode == ALLSEGS )                                           // propagate change across all segments of
   {                                                                       // of the currently loaded trial
      for( i = 0; i < m_pTrial->SegCount(); i++ )
         m_pTrial->SetSegParam( i, iTgt, pID, dNewVal, asRV);
      m_segGrid.Refresh();                                                 //    refresh grid to reflect all changes
   }
   else if( m_modifyMode == MATCHSEGS )                                    // propagate change across all matching
   {                                                                       // segments of the currently loaded trial
      for(i = 0; i < m_pTrial->SegCount(); i++)
         if(dOldVal==m_pTrial->GetSegParam(i, iTgt, pID) && wasRV==m_pTrial->IsRVAssignedToSegParam(i, iTgt, pID))
            m_pTrial->SetSegParam(i, iTgt, pID, dNewVal, asRV);
      m_segGrid.Refresh();                                                 //    refresh grid to reflect all changes
   }
   else if( m_modifyMode == ALLTRIALS || m_modifyMode == MATCHTRIALS ||    // propagate change across all compatible
            m_modifyMode == SELTRIALS )                                    // trials in currently loaded trial's set:
   { 
      CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();   //    in SELTRIALS mode, we query mainframe
      if(m_modifyMode == SELTRIALS && pFrame == NULL)                      //    to see if a given trial is selected.
         return;

      CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                //    the current open document
      POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj(m_wKey) ); //    traverse all trials in set that holds
      while( pos != NULL )                                                 //    trial currently loaded in the form:
      {
         WORD wKey;
         CCxTrial* pTrial;
         pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
         if( wKey == m_wKey ) continue;                                    //    skip over the currently loaded trial!

         BOOL bModify = BOOL(m_modifyMode == ALLTRIALS);
         if(m_modifyMode == SELTRIALS)                                     //    in SELTRIALS mode, skip over trials 
         {                                                                 //    that aren't selected in object tree
            if(!pFrame->IsObjectSelected(wKey)) continue;
            bModify = TRUE;
         }

         if( (m_pTrial->IsValidTarg(iTgt) && !pTrial->IsValidTarg(iTgt))   //    skip trials which don't have the
             || (m_pTrial->IsValidSeg(iSeg) && !pTrial->IsValidSeg(iSeg))  //    required target # or segment #
            )
            continue;

         if( pID == CCxTrial::NOTAPARAM )                                  //    special case: trial tgt replaced
         {
            if( bModify || ((WORD)dOldVal) == pTrial->GetTarget( iTgt ) )
               pTrial->SetTarget( iTgt, m_pTrial->GetTarget( iTgt ) );
         }
         else
         {
            if(bModify || (dOldVal==pTrial->GetSegParam(iSeg, iTgt, pID) && 
                  wasRV==pTrial->IsRVAssignedToSegParam(iSeg, iTgt, pID)))
               pTrial->SetSegParam(iSeg, iTgt, pID, dNewVal, asRV);
         }
      }
   }
}

VOID CCxTrialForm::PropagatePertParam( int iCol, int iPert, double dOldVal )
{
   if( m_modifyMode != ALLTRIALS && m_modifyMode != MATCHTRIALS &&         // nothing to do; pert list changes are
       m_modifyMode != SELTRIALS )                                         // only propagated across trial set
      return;
   ASSERT( m_pTrial );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();      // in SELTRIALS mode, we query mainframe wnd
   if(m_modifyMode == SELTRIALS && pFrame == NULL)                         // to see if a given trial is selected.
      return;

   int iOld = int(dOldVal);                                                // different params stored as diff types
   float fOld = float(dOldVal);
   WORD wOld = (WORD) dOldVal;

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                   // the current open document
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj(m_wKey) );    // traverse all trials in set that holds
   while( pos != NULL )                                                    // trial currently loaded in the form:
   {
      WORD wKey;
      CCxTrial* pTrial;
      pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
      if( wKey == m_wKey ) continue;                                       //    skip over the currently loaded trial!

      BOOL bModify = BOOL(m_modifyMode == ALLTRIALS);
      if(m_modifyMode == SELTRIALS)                                        //    in SELTRIALS mode, skip over trials 
      {                                                                    //    that aren't selected in object tree
         if(!pFrame->IsObjectSelected(wKey)) continue;
         bModify = TRUE;
      }

      if( !pTrial->IsValidPert( iPert ) )                                  //    skip over trial if relevent pert obj
         continue;                                                         //    is not present in trial's pert list!

      CWordArray wArDeps;                                                  //    in case we must update dependencies

      switch( iCol )                                                       //    update relevant pert param if approp
      {
         case 1 :
            if( bModify || fOld == pTrial->GetPertAmp( iPert ) )
               pTrial->SetPertAmp( iPert, m_pTrial->GetPertAmp( iPert ) );
            break;
         case 2 :
            if( bModify || iOld == pTrial->GetPertSeg( iPert ) )
               pTrial->SetPertSeg( iPert, m_pTrial->GetPertSeg( iPert ) );
            break;
         case 3 :
            if( bModify || iOld == pTrial->GetPertTgt( iPert ) )
               pTrial->SetPertTgt( iPert, m_pTrial->GetPertTgt( iPert ) );
            break;
         case 4 :
            if( bModify || iOld == pTrial->GetPertTrajCmpt( iPert ) )
               pTrial->SetPertTrajCmpt( iPert, m_pTrial->GetPertTrajCmpt( iPert ) );
            break;
         case 0 :
            if( bModify || wOld == pTrial->GetPertKey( iPert ) )
            {
               pTrial->GetDependencies( wArDeps );                         //    changing identity of a pert obj will
               pTrial->SetPertKey( iPert, m_pTrial->GetPertKey( iPert ) ); //    change trial's dependencies, so we
               pDoc->UpdateObjDep( wKey, wArDeps );                        //    must update them after making change.
            }
            break;
      }
   }
}

VOID CCxTrialForm::PropagatePertOp( UINT cmdID, int iPert, int nP )
{
   if( m_modifyMode != ALLTRIALS && m_modifyMode != MATCHTRIALS &&         // nothing to do; pert list changes are
       m_modifyMode != SELTRIALS )                                         // only propagated across trial set
      return;
   ASSERT( m_pTrial );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();      // in SELTRIALS mode, we query mainframe wnd
   if(m_modifyMode == SELTRIALS && pFrame == NULL)                         // to see if a given trial is selected.
      return;

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                   // the current open document
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj(m_wKey) );    // traverse all trials in set that holds
   while( pos != NULL )                                                    // trial currently loaded in the form:
   {
      WORD wKey;
      CCxTrial* pTrial;
      pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
      if( wKey == m_wKey ) continue;                                       //    skip over the currently loaded trial!

      if(m_modifyMode == SELTRIALS && !pFrame->IsObjectSelected(wKey))     //    in SELTRIALS mode, skip trials in set 
         continue;                                                         //    that are not selected

      if( pTrial->PertCount() != nP )                                      //    skip trials with pert count != loaded
         continue;                                                         //    trial's pert count PRIOR to change

      CWordArray wArDeps;                                                  //    get dependencies prior to change
      pTrial->GetDependencies( wArDeps );                                  //    in case we must update them afterwards
      BOOL bUpdate = FALSE;

      switch( cmdID )                                                      //    update pert list IAW specified op:
      {
         case ID_PERT_APPEND :
            if( pTrial->AppendPert( m_pTrial->GetPertKey( m_pTrial->PertCount()-1 ) ) ) bUpdate = TRUE;
            break;
         case ID_PERT_REMOVE :
            if( pTrial->IsValidPert( iPert ) ) { pTrial->RemovePert( iPert ); bUpdate = TRUE; }
            break;
         case ID_PERT_CLEAR :
            if( pTrial->PertCount() > 0 ) { pTrial->RemovePert( -1 ); bUpdate = TRUE; }
            break;
      }

      if( bUpdate ) pDoc->UpdateObjDep( wKey, wArDeps );                   //    if change made, update dependencies
   }
}

VOID CCxTrialForm::PropagateSegOp( int nT, int nS, int iSeg, UINT cmdID )
{
   if( m_modifyMode != ALLTRIALS && m_modifyMode != MATCHTRIALS &&         // nothing to do; segment-related ops are
       m_modifyMode != SELTRIALS )                                         // only propagated across trial set
      return;
   ASSERT( m_pTrial );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();      // in SELTRIALS mode, we query mainframe wnd
   if(m_modifyMode == SELTRIALS && pFrame == NULL)                         // to see if a given trial is selected.
      return;

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                   // the current open document
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj(m_wKey) );    // traverse all trials in set that holds
   while( pos != NULL )                                                    // trial currently loaded in the form:
   {
      WORD wKey;
      CCxTrial* pTrial;
      pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
      if( wKey == m_wKey ) continue;                                       //    skip over the currently loaded trial!

      if(m_modifyMode == SELTRIALS && !pFrame->IsObjectSelected(wKey))     //    in SELTRIALS mode, skip trials in set 
         continue;                                                         //    that are not selected

      if( pTrial->TargCount() != nT || pTrial->SegCount() != nS )          //    trial must have same #tgts and #segs
         continue;                                                         //    as loaded trial had prior to change!

      switch( cmdID )                                                      //    update each trial IAW specified op:
      {
         case ID_GRID_INSERTSEG :   pTrial->InsertSeg( iSeg ); break;
         case ID_GRID_PASTESEG :    pTrial->PasteSeg( iSeg, m_pPasteSeg ); break;
         case ID_GRID_CUTSEG :
         case ID_GRID_DEL :         pTrial->RemoveSeg( iSeg ); break;
         case ID_GRID_REPLACESEG :  pTrial->ReplaceSeg( iSeg, m_pPasteSeg ); break;
         case ID_GRID_CLRSEGS :     while( pTrial->SegCount() > 0 ) pTrial->RemoveSeg( 0 ); break;
      }
   }
}

VOID CCxTrialForm::PropagateTgtOp( int nT, int nS, int iTgt, UINT cmdID )
{
   if( m_modifyMode != ALLTRIALS && m_modifyMode != MATCHTRIALS &&         // nothing to do; target-related ops are
       m_modifyMode != SELTRIALS )                                         // only propagated across trial set
      return;
   ASSERT( m_pTrial );

   CCxMainFrame* pFrame = ((CCntrlxApp*)AfxGetApp())->GetMainFrame();      // in SELTRIALS mode, we query mainframe wnd
   if(m_modifyMode == SELTRIALS && pFrame == NULL)                         // to see if a given trial is selected.
      return;

   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                   // the current open document
   POSITION pos = pDoc->GetFirstChildObj( pDoc->GetParentObj(m_wKey) );    // traverse all trials in set that holds
   while( pos != NULL )                                                    // trial currently loaded in the form:
   {
      WORD wKey;
      CCxTrial* pTrial;
      pDoc->GetNextChildObj( pos, wKey, (CTreeObj*&)pTrial );
      if( wKey == m_wKey ) continue;                                       //    skip over the currently loaded trial!

      if(m_modifyMode == SELTRIALS && !pFrame->IsObjectSelected(wKey))     //    in SELTRIALS mode, skip trials in set 
         continue;                                                         //    that are not selected

      if( pTrial->TargCount() != nT || pTrial->SegCount() != nS )          //    trial must have same #tgts and #segs
         continue;                                                         //    as loaded trial had prior to change!

      CWordArray wArDeps;                                                  //    get dependencies prior to change; we
      pTrial->GetDependencies( wArDeps );                                  //    must update them afterwards

      switch( cmdID )                                                      //    update each trial IAW specified op:
      {
         case ID_GRID_INSERTTARG :  pTrial->InsertTarget( iTgt, m_pTrial->GetTarget( iTgt ) ); break;
         case ID_GRID_DEL :         pTrial->RemoveTarget( iTgt ); break;
         case ID_GRID_CLRTARGS :    while( pTrial->TargCount() > 0 ) pTrial->RemoveTarget( 0 ); break;
         case ID_GRID_CLRALL :      pTrial->Clear(); break;
      }

      pDoc->UpdateObjDep( wKey, wArDeps );                                 //    update trial dependencies after change
   }
}


//=== PartitionGridDispCB =============================================================================================
//
//    Callback function queried by the partitions grid to obtain the contents of each cell in the grid.
//
//    The trial partitions grid sits above the segment table and has the same width as that table.  It contains a single
//    row, the cells of which display the current partitions of the trial.  A "partition" is either a group of
//    contiguous segments, known as a "tagged section", or an individual untagged segment.
//
//    If a cell corresponds to a tagged section, the name of that section is displayed in the cell, along with the
//    range of segments in brackets (eg, "mySection [1:3]").  The background color is a yellowish hue to make tagged
//    sections stand out from untagged segments.  If a cell corresponds to an individual untagged segment, the segment
//    index is displayed in the cell with the default bkg color (white).
//
//    The first cell in the grid displays a label reflecting the current modification mode for the segment table.
//    Whenever one of the non-atomic modification modes is in effect, the cell's background is painted red.
//
//    NOTE:  Callback functions must be implemented as static.  Since it is a static class method, it does not have
//    access to instance fields and methods and it does not get the implied argument THIS.  To circumvent this problem,
//    we take advantage of the generic LPARAM argument, using it to pass a reference to THIS view!!  This is done when
//    we register the callback fcn with the grid in OnInitialUpdate().
//
//    ARGS:       pDispInfo   -- [in] ptr to a struct we need to fill with the appropriate display info.
//                lParam      -- [in] THIS (see NOTE)
//
//    RETURNS:    NONE
//
BOOL CALLBACK CCxTrialForm::PartitionGridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &(pThis->m_partitionGrid);                        // the grid control
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );               // the grid cell of interest

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )           // ignore when no grid or cell not valid
      return( FALSE );

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                  // we don't use label tips on this grid
   {
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
      return( TRUE );
   }

   if( pThis->m_bLoading )                                              // disable callback while we're changing trial
      return( FALSE );                                                  // that's loaded on form

   int iPart = c.col - 1;                                               // cell(0,0) displays current modification mode
   if( iPart < 0 )
   {
      if( pThis->m_modifyMode == ATOMIC )
         pDispInfo->item.strText = _T("Modify THIS SEG");
      else
      {
         pDispInfo->item.crBkClr = clrRed;
         if( pThis->m_modifyMode == MATCHSEGS )
            pDispInfo->item.strText = _T("Modify MATCHING SEGS");
         else if( pThis->m_modifyMode == ALLSEGS )
            pDispInfo->item.strText = _T("Modify ALL SEGS");
         else if( pThis->m_modifyMode == ALLTRIALS )
            pDispInfo->item.strText = _T("Modify ALL TRIALS");
         else if( pThis->m_modifyMode == MATCHTRIALS )
            pDispInfo->item.strText = _T("Modify MATCHING TRIALS");
         else
            pDispInfo->item.strText = _T("Modify SELECTED TRIALS");
      }
   }
   else
   {
      CPartition* p = &(pThis->m_partitions[iPart]);
      if( p->iSection >= 0 )                                            // partition is a tagged section
      {
         CString tag;
         pTrial->GetTaggedSectionName(p->iSection, tag);
         pDispInfo->item.strText.Format( "%s [%d:%d]", tag, p->iFirstSeg, p->iLastSeg );
         pDispInfo->item.crBkClr = clrYellow;
      }
      else                                                              // partition is a single untagged segment
         pDispInfo->item.strText.Format( "%d", pThis->m_partitions[iPart].iFirstSeg );

      if( pGrid->IsValid(pThis->m_tagSectAnchorCell) &&                 // during a tagged-section create gesture, the
          (c==pThis->m_tagSectAnchorCell) )                             // anchor cell has blue bkg and white txt
      {
         pDispInfo->item.crBkClr = clrBlue;
         pDispInfo->item.crFgClr = clrWhite;
      }
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                     // only show the title tip if the cell's text
                                                                        // is too big to fit...
   return( TRUE );
}


//=== PartitionGridEditCB =============================================================================================
//
//    Callback invoked to initiate inplace editing of a cell on the trial partitions grid.  Only those cells that
//    represent a tagged section are editable.  For such cells, the callback requires that the CLiteGrid's inplace text
//    editor be used to edit the section's tag name, which is provided as the initial contents of the editor.
//
//    NOTE:  See also PartitionGridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::PartitionGridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &(pThis->m_partitionGrid);                        // the grid control
   CCellID c = pEI->cell;                                               // the cell to be edited

   if( (pTrial == NULL) || pGrid->GetSafeHwnd() == NULL ||              // if trial not loaded, or grid is gone, or
       !pGrid->IsValid( c ) )                                           // cell not valid, ignore!
      return( FALSE );

   int iPart = c.col - 1;                                               // the only editable cells in the grid
   if( iPart < 0 || iPart >= pThis->m_nPartitions ||                    // are those that represent tagged sections!
       pThis->m_partitions[iPart].iSection < 0 )
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
   }
   else if( pEI->iClick != 0 )                                          // cannot edit any cell via a mouse click
   {
      pEI->iClick = 0;
   }
   else                                                                 // use text string editor init'd w/ current
   {                                                                    // name of the tagged section
      if( pTrial->GetTaggedSectionName(
               pThis->m_partitions[iPart].iSection, pEI->strCurrent) )
         pEI->iType = LG_TEXTSTR;
      else
         pEI->iType = LG_READONLY;
   }

   return( TRUE );
}


//=== PartitionGridEndEditCB ==========================================================================================
//
//    Callback invoked upon termination of inplace editing of a cell in the trial partitions grid.
//
//    The only editable cells in the partitions grid correspond to tagged sections in the loaded trial.  If the user
//    provided a new tag name for the relevant section, this method attempts to make the name change.  If the new tag
//    is not a valid one, it is rejected and the inplace editor reappears.  Otherwise, the grid cell is refreshed to
//    show the new name.
//
//    NOTE:  See also PartitionGridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
//
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxTrialForm::PartitionGridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &(pThis->m_partitionGrid);                        // the grid control
   CCellID c = pEEI->cell;                                              // the cell that was edited

   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                   // user cancelled; nothing to do

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )           // ignore when no grid or cell not valid
      return( FALSE );

   if( pEEI->bIsChanged )                                               // if user provided a new tag name, attempt to
   {                                                                    // make the change.  If name is invalid,
      int iSect = pThis->m_partitions[c.col-1].iSection;                // force the inplace edit ctrl to reappear...
      if( pTrial->RenameTaggedSection(iSect, pEEI->strNew) )
      {
         pThis->InformModify();
      }
      else
      {
         ::MessageBeep( MB_ICONEXCLAMATION );
         pEEI->bReject = TRUE;
      }
   }
   pEEI->cellNext = CCellID(-1,-1);                                     // navigation to "next cell" not supported
   pEEI->nExitChar = 0;
   return( TRUE );
}


//=== RebuildPartitionGrid ============================================================================================
//
//    Sets the number of cells in the single-row trial partitions grid, along with their sizes, to reflect any tagged
//    sections in the current trial, as well as any individual segments that are not members of a tagged section.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::RebuildPartitionGrid()
{
   // step1:  build internal representation of the loaded trial's partitions
   m_nPartitions = 0;
   if( m_pTrial != NULL )
   {
      int nSections = m_pTrial->GetNumTaggedSections();

      TRIALSECT section;
      int iSeg = 0;
      int iPart = 0;
      int iSect = 0;
      while( iSect < nSections )
      {
         m_pTrial->GetTaggedSection(iSect, section);                       // get next tagged section -- they are
                                                                           // indexed in ascending order
         while( iSeg < int(section.cFirstSeg) )                            // add a partition for each untagged segment
         {                                                                 // preceding the next tagged section
            m_partitions[iPart].iFirstSeg = iSeg;
            m_partitions[iPart].iLastSeg = iSeg;
            m_partitions[iPart].iSection = -1;
            ++iPart;
            ++iSeg;
         }

         m_partitions[iPart].iFirstSeg = int(section.cFirstSeg);           // add partition for the tagged section
         m_partitions[iPart].iLastSeg = int(section.cLastSeg);
         m_partitions[iPart].iSection = iSect;
         ++iPart;
         iSeg = int(section.cLastSeg) + 1;

         ++iSect;
      }

      while( iSeg < m_pTrial->SegCount() )                                 // add a partition for each untagged segment
      {                                                                    // following the last tagged section
         m_partitions[iPart].iFirstSeg = iSeg;
         m_partitions[iPart].iLastSeg = iSeg;
         m_partitions[iPart].iSection = -1;
         ++iPart;
         ++iSeg;
      }

      m_nPartitions = iPart;
      ASSERT(m_nPartitions == (nSections + m_pTrial->SegCount() - m_pTrial->GetNumTaggedSegments()));
   }

   // step 2:  set #columns for partitions grid and the column sizes based upon the current partitioning of trial
   m_partitionGrid.SetColumnCount(1+m_nPartitions);
   for( int i=0; i<m_nPartitions; i++ )
   {
      int colWidth = (m_partitions[i].iLastSeg - m_partitions[i].iFirstSeg + 1) * SEGCOL_W * 2;
      m_partitionGrid.SetColumnWidth(1+i, colWidth);
   }
   m_partitionGrid.Refresh();
}


//=== HandleSectionCreateGesture ======================================================================================
//
//    The GUI gesture that creates a new tagged section involves left-clicking on two cells in the trial partitions
//    grid with the SHIFT key down and within a certain amount of time.  HandleSectionCreateGesture() handles both
//    stages of the gesture, cancelling it if necessary.
//
//    In response to the initial "SHIFT left-click", the method makes the clicked cell the anchor cell for specifying
//    the range of cells in the grid that will be spanned by the new tagged section.  (NOTE that any cell in the grid
//    may correspond to an individual segment OR an existing tagged section!).  It then starts a system timer (with
//    event ID NM_CLICK) that gives user a short time to complete the gesture by SHIFT left-clicking on a second cell.
//
//    In response to the second "SHIFT left-click", the method forms a new tagged section that spans the range of cells
//    between the anchor cell and the cell that was clicked second (they could be the same!).  If the user selected a
//    single cell that is already a tagged section, the operation is cancelled.  After modifying the loaded trial, the
//    partitions grid is rebuilt and refreshed to reflect the new partitioning of the trial's segments.  The new
//    tagged section is given a default valid name, and an inplace edit is initiated to let user change that name.
//
//    ARGS:       bShift      -- [in] TRUE if SHIFT key was down when the left-click notification was received.  If
//                               FALSE, a gesture will not be initiated and if a gesture was in progess, it is
//                               cancelled.
//                clickedCell -- [in] the cell in the partitions grid that was under the mouse when the click occurred.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::HandleSectionCreateGesture(BOOL bShift, CCellID clickedCell)
{
   if( (!bShift) || !m_partitionGrid.IsValid(clickedCell) )       // the SHIFT key must have been down and the clicked
   {                                                              // cell must be valid -- else we cancel gesture
      CancelSectionCreateGesture();
      return;
   }
   ASSERT(m_pTrial != NULL);                                      // a trial must be loaded onto form

   if( !m_partitionGrid.IsValid(m_tagSectAnchorCell) )            // if anchor cell not valid, then start the gesture:
   {
      CancelSectionCreateGesture();                               //    just to be sure
      m_tagSectAnchorCell.row = clickedCell.row;                  //    remember anchor cell
      m_tagSectAnchorCell.col = clickedCell.col;

      m_nSectCreateTimerID =                                      //    start the system timer
         SetTimer(NM_CLICK, SECTCREATE_TIMEOUT, NULL);
      if( m_nSectCreateTimerID == 0 )                             //    if we can't get a timer, abort
      {
         m_tagSectAnchorCell.row = -1;
         m_tagSectAnchorCell.col = -1;
      }
      else                                                        //    else, repaint anchor cell bkg
         m_partitionGrid.Refresh();
   }
   else
   {
      int iPart0 = m_tagSectAnchorCell.col - 1;                   //    get indices of partitions to be merged; make
      int iPart1 = clickedCell.col - 1;                           //    sure they're in ascending order.
      if( iPart0 > iPart1 )
      {
         int temp = iPart0;
         iPart0 = iPart1;
         iPart1 = temp;
      }

      CancelSectionCreateGesture();                               //    reset the create gesture framework

      if( iPart0==iPart1 && m_partitions[iPart0].iSection >= 0 )  //    if one partition selected and it is already a
      {                                                           //    tagged section, then abort
         m_partitionGrid.Refresh();
         return;
      }

      int s0 = m_partitions[iPart0].iFirstSeg;                    //    create a new tagged section spanning the
      int s1 = m_partitions[iPart1].iLastSeg;                     //    selected partitions.  if successful, rebuild
      if( m_pTrial->CreateTaggedSection(s0, s1) )                 //    the partitions grid and initiate an inplace
      {                                                           //    edit to let user change tag name of the new
         RebuildPartitionGrid();                                  //    section...
         InformModify();
         m_partitionGrid.InitiateCellEdit(0, iPart0 + 1);
      }
   }
}

//=== CancelSectionCreateGesture ======================================================================================
//
//    Cancel the tagged section-create gesture on the trial partitions grid, if there is one in progress.  The method
//    invalidates the partitions grid cell that was clicked to initiate the gesture (the "anchor cell"), and stops and
//    releases a system timer that measures the timeout period for the gesture.  The partitions grid is repainted to
//    ensure the anchor cell's bkg color is restored to the normal bkg color.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrialForm::CancelSectionCreateGesture()
{
   if( m_nSectCreateTimerID != 0 )
   {
      KillTimer(m_nSectCreateTimerID);
      m_nSectCreateTimerID = 0;
   }
   if( m_tagSectAnchorCell.row != -1 )
   {
      m_tagSectAnchorCell.row = -1;
      m_tagSectAnchorCell.col = -1;
      m_partitionGrid.Refresh();
   }
}


/**
 Callback function queried to obtain the contents of each cell in the "Random Variables" grid control, found on the tab
 page of the same name. 
 
 The random variables grid has exactly 7 columns and N+1 rows, where N=MAX_TRIALRVS is the number of distinct random 
 variables that can be used in a trial. The first row displays column headings, while each of the remaining rows 
 describes a random variable. The first column is the (fixed) variable name, while the other columns display the 
 defining parameters for a random variable. Usage depends on the RV type:

 Col 0: The random variable name: "x0" .. "x(N-1)", where N = MAX_TRIALRVS. Currently N = 10. 
 Col 1: Variable type: RV_NOTUSED .. RV_FUNCTION. CCxTrialForm supplies a human-readable name for each type. 
 Col 2: The seed for a distributed RV (not applicable to RV_FUNCTION). Non-negative integer. If 0, a different value is
        used to seed the RV each time a trial sequence is initiated.
 Col 3-5: Up to 3 defining parameters for a distributed RV (not applicable to RV_FUNCTION). See CCxTrial::GetRV() for a
        description of these parameters.
 Col 6: For the RV_FUNCTION type only, this is the function formula string. 
 
 When the parameter displayed in a particular cell is not applicable to the RV's current type, that cell will be left 
 blank and have the same background as a fixed cell in the row or column header. When no trial is loaded, all cells in
 the grid are blank and uneditable (although the fixed labels in the row and column headers are shown).

 NOTE: Callback functions must be implemented as static. Since it is a static class method, it does not have access to 
 instance fields and methods and it does not get the implied argument THIS. To circumvent this problem, we take 
 advantage of the generic LPARAM argument, using it to pass a reference to CCxTrialForm!! This is done when the 
 callback fcn is registered with the grid.
 
 @param pDispInfo [in] Ptr to a struct we need to fill with the appropriate display info.
 @param lParam    [in] THIS (see NOTE)
 @return TRUE if successful, FALSE otherwise (invalid cell, e.g.)
*/
BOOL CALLBACK CCxTrialForm::RVGridDispCB(GV_DISPINFO *pDispInfo, LPARAM lParam)
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_rvPage).m_rvGrid);                    // the random variables list grid
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );               // the grid cell of interest

   // ignore when there's no grid or cell is not valid; or while we're changing the trial that's loaded on form
   if(pGrid->GetSafeHwnd() == NULL || (!pGrid->IsValid(c)) || pThis->m_bLoading) return(FALSE);

   // special case: no trial loaded on form. Show labels in row and column headers, but everything else blank.
   if(pTrial==NULL)
   {
      if(c.col==0 && c.row>0) pDispInfo->item.strText.Format("x%d", c.row-1);
      else if(c.col>0 && c.row==0)
      {
         if(c.col==1) pDispInfo->item.strText = _T("Type"); 
         else if(c.col==2) pDispInfo->item.strText = _T("Seed");
         else if(c.col>=3 && c.col<=5) pDispInfo->item.strText.Format("param%d", c.col-2);
         else if(c.col==6) pDispInfo->item.strText = _T("Formula");
      }
      else pDispInfo->item.strText = _T("");

      CGridCellBase* pFixed = pGrid->GetDefaultCell( TRUE, TRUE );
      pDispInfo->item.crBkClr = pFixed->GetBackClr();
      return(TRUE);
   }

   // get the trial random variable for the specified row (except first row is column header!)
   int idx = c.row - 1;
   CCxTrial::CRVEntry rv;
   if(idx >= 0 && idx < MAX_TRIALRVS) 
   {
      if(!pTrial->GetRV(idx, rv)) return(FALSE);
   }

   // we use label tips only for the 2-3 defining parameters of a distributed RV
   if(pDispInfo->item.nState & GVIS_VIRTUALLABELTIP) 
   {
      if(idx < 0)
         pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
      else if(rv.iType == RV_UNIFORM && (c.col == 3 || c.col == 4))
         pDispInfo->item.strText = _T((c.col==3) ? "Lower Bound" : "Upper Bound");
      else if(rv.iType == RV_NORMAL && (c.col >= 3 && c.col <= 5))
         pDispInfo->item.strText = _T((c.col==3) ? "Mean" : (c.col==4 ? "Standard Deviation" : "+/- Max Spread"));
      else if(rv.iType == RV_EXPON && (c.col == 3 || c.col == 4))
         pDispInfo->item.strText = _T((c.col==3) ? "Rate (lambda)" : "Max Cutoff");
      else if(rv.iType == RV_GAMMA && (c.col >= 3 && c.col <= 5))
         pDispInfo->item.strText = _T((c.col==3) ? "Shape (kappa)" : (c.col==4 ? "Scale (theta)" : "Max Cutoff"));
      else
         pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
   }
   else if(idx < 0)
   {
      // static labels appear in the column header
      switch(c.col)
      {
      case 1 : pDispInfo->item.strText = _T("Type"); break;
      case 2 : pDispInfo->item.strText = _T("Seed"); break;
      case 3: case 4: case 5: pDispInfo->item.strText.Format("param%d", c.col-2); break;
      case 6 : pDispInfo->item.strText = _T("Formula"); break;
      default :  pDispInfo->item.strText = _T(""); break;
      }
   }
   else 
   {
      // for any cell corresponding to a parameter that is irrelevant for the current RV type, contents are empty
      // and bkg color matches that of a fixed cell
      pDispInfo->item.strText = _T(""); 
      CGridCellBase* pFixed = pGrid->GetDefaultCell( TRUE, TRUE );
      pDispInfo->item.crBkClr = pFixed->GetBackClr();
      switch(c.col)
      {
      case 0: 
         // the row header contains the (fixed) variable label: "x0" .. "x9"
         pDispInfo->item.strText.Format("x%d", idx);
         break;
      case 1:
         // the RV type as a human-readable label
         pDispInfo->item.strText = strRVTypeLabels[rv.iType];
         pDispInfo->item.crBkClr = clrWhite;
         break;
      case 2:
         // the seed value for distributed RVs
         if(rv.iType >= RV_UNIFORM && rv.iType <= RV_GAMMA) 
         {
            pDispInfo->item.strText.Format("%d", rv.iSeed);
            pDispInfo->item.crBkClr = clrWhite;
         }
         break;
      case 3: 
      case 4: 
         // value of parameter 1, 2 for any distributed RV
         if(rv.iType >= RV_UNIFORM && rv.iType <= RV_GAMMA) 
         {
            pDispInfo->item.strText.Format("%.2f", rv.dParams[c.col-3]);
            pDispInfo->item.crBkClr = clrWhite;
         }
         break;
      case 5:
         // value of parameter 3 for the RV_NORMAL and RV_GAMMA distributed RVs
         if(rv.iType == RV_NORMAL || rv.iType == RV_GAMMA) 
         {
            pDispInfo->item.strText.Format("%.2f", rv.dParams[c.col-3]);
            pDispInfo->item.crBkClr = clrWhite;
         }
         break;
      case 6:
         // the function definition string for RV_FUNCTION type only
         if(rv.iType == RV_FUNCTION) 
         {
            pDispInfo->item.strText = rv.strFunc;
            pDispInfo->item.crBkClr = clrWhite;
         }
         break;
      }
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                     // show title tip if cell's text doesn't fit
   return(TRUE);
}


/**
 Callback invoked to initiate inplace editing of a cell in the random variables grid. Most "editable" cells in this
 grid are numeric parameters, with the exception of the RV type and the formula for an RV_FUNCTION variable. Briefly,
 this method permits the following operations:

 Col 1 : The random variable type. Edited as a multi-choice param. Choices are the labels in strRVTypeLabels[],
         indexed by RV type.
 Col 2 : The random variable seed, for distributed RVs only. Edited as an integer numeric value.
 Col 3-5 : Defining parameters for distributed RVs only. Each is edited as a floating-point numeric value with two
         digits' precision.
 Col 6 : The function formula, for RV_FUNCTION only. Edited as a plain text string.

 See also RVGridDispCB(). Note that right-click initiated operations are not supported for the random variables grid.

 @param pEI [in/out] Ptr to a struct we need to fill with the required edit info.
 @param lParam [in] Reference to THIS so that we can access the CCxTrialForm instance.
 @return TRUE if successful, FALSE otherwise (invalid cell, e.g.)
*/
BOOL CALLBACK CCxTrialForm::RVGridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_rvPage).m_rvGrid);                    // the random variables list grid
   CCellID c = pEI->cell;                                               // the cell to be edited
   int idx = c.row - 1;                                                 // index of RV to be edited

   // do nothing if trial not loaded, grid does not exist, or cell is not valid
   if((pTrial == NULL) || pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid(c)) return(FALSE);

   // retrieve the current random variable corresponding to the cell's row. Cannot edit a cell that does not contain 
   // editable content. 
   CCxTrial::CRVEntry rv;
   BOOL canEdit = (idx >= 0 && idx < MAX_TRIALRVS && c.col > 0 && c.col <= 6);
   if(canEdit) canEdit = pTrial->GetRV(idx, rv);
   if(canEdit) switch(rv.iType)
   {
   case RV_NOTUSED : canEdit = (c.col == 1); break;
   case RV_FUNCTION : canEdit = (c.col == 1 || c.col == 6); break;
   case RV_UNIFORM :
   case RV_EXPON :
      canEdit = (c.col >= 1 && c.col <= 4); 
      break;
   default : // RV_NORMAL, RV_GAMMA
      canEdit = (c.col >= 1 && c.col <= 5);
      break;
   }
   if(!canEdit)
   {
      pEI->iClick = 0;
      pEI->iType = LG_READONLY;
      return(TRUE);
   }

   // prepare the inplace editor to edit the cell contents. Right-click instance edits not supported for RV grid.
   pEI->iClick = 0; 
   switch(c.col)
   {
   case 1 : 
      // type of random variable -- a multi-choice value
      pEI->iType = LG_MULTICHOICE;
      pEI->iCurrent = rv.iType;
      for(int i=0; i<RV_NUMTYPES; i++) 
         pEI->strArChoices.Add(strRVTypeLabels[i]);
      break;
   case 2 :
      // seed for a distributed RV -- a non-negative integer with up to 8 digits
      pEI->iType = LG_NUMSTR;
      pEI->numFmt.flags = NES_INTONLY|NES_NONNEG;
      pEI->numFmt.nPre = 0;
      pEI->numFmt.nLen = 8;
      pEI->dCurrent = double(rv.iSeed);
      break;
   case 3 : case 4 : case 5 :
      // a defining parameter for a distributed RV -- a floating-point value with up to 8 total characters
      pEI->iType = LG_NUMSTR;
      pEI->numFmt.flags = 0;
      pEI->numFmt.nPre = 2;
      pEI->numFmt.nLen = 8;
      pEI->dCurrent = rv.dParams[c.col-3];
      break;
   case 6 :
      // the function formula for RV_FUNCTION -- a plain text string
      pEI->iType = LG_TEXTSTR;
      pEI->strCurrent = rv.strFunc;
   }

   return(TRUE);
}


/**
 Callback invoked upon termination of inplace editing of a cell in the random variables grid.

 Here we update the loaded trial IAW the change made during the inplace operation configured in RVGridEditCB(). If the
 edit operation is rejected (bad parameter value, invalid function formula, etc), a "beep" is played and the inplace
 control reappears to emphasize that the edit was unsuccessful.

 Whenever the type of a random variable is modified, it could affect the contents and appearance of other cells on
 the same row. For example, if you change the type back to RV_NOTUSED, then all the other cells become uneditable. In
 this scenario, we refresh the grid to ensure any such "side-effects" are reflected in it. 

 Inplace editing may continue at a nearby cell location, based on the exit char that terminated the inplace tool and 
 default CLiteGrid keyboard navigation rules. However, we prevent this continuation when the grid is refreshed after
 a change in the type of a random variable.

 @param pEEI [in/out] Ptr to a struct containing results of inplace edit op, and our response.
 @param lParam [in] Reference to THIS so that we can access the CCxTrialForm instance.
 @return TRUE if successful, FALSE otherwise (invalid cell, e.g.)
*/
BOOL CALLBACK CCxTrialForm::RVGridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   CCxTrialForm* pThis = (CCxTrialForm*)lParam;                         // to access non-static data in the view
   CCxTrial* pTrial = pThis->m_pTrial;                                  // the currently loaded trial
   CLiteGrid* pGrid = &((pThis->m_rvPage).m_rvGrid);                    // the random variables list grid
   CCellID c = pEEI->cell;                                              // the cell that was edited
   int idx = c.row - 1;                                                 // index of corresponding trial RV

   // if user cancelled the operation or made no change, there's nothing to do.
   if(pEEI->nExitChar == VK_ESCAPE || !pEEI->bIsChanged) return(TRUE);

   // ignore when grid control is not realized or grid cell is invalid
   if(pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid(c)) return(FALSE);

   // retrieve the current random variable corresponding to the cell's row. Cannot edit a cell that does not contain 
   // editable content!
   CCxTrial::CRVEntry rv;
   BOOL canEdit = BOOL(pTrial != NULL && idx >= 0 && idx < MAX_TRIALRVS && c.col > 0 && c.col <= 6);
   if(canEdit) canEdit = pTrial->GetRV(idx, rv);
   if(canEdit) switch(rv.iType)
   {
   case RV_NOTUSED : canEdit = (c.col == 1); break;
   case RV_FUNCTION : canEdit = (c.col == 1 || c.col == 6); break;
   case RV_UNIFORM :
   case RV_EXPON :
      canEdit = (c.col >= 1 && c.col <= 4); 
      break;
   default : // RV_NORMAL, RV_GAMMA
      canEdit = (c.col >= 1 && c.col <= 5);
      break;
   }
   if(!canEdit)
   {
      pEEI->nExitChar = VK_ESCAPE;
      pEEI->bNoRedraw = TRUE;
      return(TRUE);
   }

   // apply the edit
   switch(c.col)
   {
   case 1:
      rv.iType = int(pEEI->dwNew);
      break;
   case 2:
      rv.iSeed = int(pEEI->dNew);
      break;
   case 3: case 4: case 5:
      rv.dParams[c.col-3] = pEEI->dNew;
      break;
   case 6:
      rv.strFunc = pEEI->strNew;
      break;
   }

   // if edit is rejected, alert user and force inplace edit control to reappear.
   BOOL bSideEffect = FALSE;
   if(!pTrial->SetRVParam(idx, c.col, rv, bSideEffect))
   {
      ::MessageBeep(MB_ICONEXCLAMATION);
      pEEI->bReject = TRUE;
      return(TRUE);
   }

   // if any other parameter is affected by the change, refresh grid now. Don't prevent continuation
   if(bSideEffect)
   {
      pGrid->Refresh(); 
      pEEI->bNoRedraw = TRUE;
      // pEEI->nExitChar = 0;
   }

   // trial definition has changed. NOTE that propagation of RV edits is NOT supported
   pThis->InformModify();

   return(TRUE);
}


