//=====================================================================================================================
//
// cxobjtree.cpp : Implementation of class CCxObjectTree, the Maestro object tree view.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The user creates experimental protocols within a Maestro "experiment document" (CCxDoc) by defining a variety of
// "data objects" and establishing relationships among those objects.  Every "data object" in Maestro has a name, a
// defined constant identifying its "abstract data class", possibly some state flags, and the parametric data which
// define how the object "behaves" in a Maestro experiment.  For instance, each Maestro "trial" defines trajectories
// of one or more "targets", which are defined separately.  The trial object also refers to a "channel config" object,
// defining which Maestro data channels should be sampled during that trial and how each channel's data trace should be
// displayed.  Trials, targets, and channel configurations are all examples of "abstract" data classes defined in
// Maestro.  The defined constants of all Maestro data types are found in the Maestro object interface file,
// CXOBJ_IFC.H.
//
// In addition to "data objects", Maestro defines "virtual collections" of other objects:  A "target set", for example,
// is a collection of individual target objects.  Such collection objects have been defined so that we can organize all
// of the real data objects into a hierarchical tree-like structure, the Maestro "object tree".  All targets, e.g., are
// stored in the "target subtree", all trials in the "trial subtree", and so on.
//
// CCxObjectTree is the CTreeView-based window by which the user manages the contents of the Maestro object tree,
// within constraints imposed by CCxDoc (and, to a certain extent, CCxObjectTree itself).  Typical operations supported
// by the view include:  adding an object to the tree, copying objects, deleting objects, moving objects from one part
// of the tree to another, renaming objects, and selecting an object for display.  Other view classes handle the
// details of displaying a selected object's definition.
//
// ==> Class Hierarchy:  CTreeView ==> CMultiDragTreeView ==> CCxObjectTree.
//
// CTreeView is the general-purpose tree view class provided by the Microsoft Foundation Class (MFC) library.  While
// providing a good foundation, it lacks built-in support for drag-n-drop and multiple-selection, and has some other
// deficiencies.  CMultiDragTreeView (see MDRGTREE.CPP) is an enhanced CTreeView that supports both drag-n-drop and
// multiple-item selections, as well as some other features.  By basing CCxObjectTree on CMultiDragTreeView rather than
// CTreeView itself, we get drag-n-drop and other capabilities without having to worry about the implementation
// details.  CCxObjectTree itself tailors CMultiDragTreeView for its specific usage in Maestro.
//
// ==> The Maestro "Object" Menu.
//
// The top-level main menu in Maestro (resource IDR_MAINFRAME) includes the dropdown submenu labeled "Object".  This
// menu lists operations that may be performed on the Maestro object tree via CCxObjectTree:
//    ID_OBJ_XYTGT ==> "New | XY Scope Target".  Insert a new Maestro "XY scope" target object.  If the tree item with
//                     the current focus is on or inside a target set, the new target is placed within that set.
//                     Otherwise, it is placed at the base of the "target" subtree (CX_TARGBASE).  If the focused tree
//                     item is a target object, the new target object is inserted before this object.  Always enabled.
//    ID_OBJ_FBTGT ==> "New | RMVideo Target".  Insert a new Maestro "RMVideo" framebuffer video target.  Analogous to 
//                     ID_OBJ_XYTGT.
//    ID_OBJ_TRIAL ==> "New | Trial".  Insert a new Maestro trial object.  If tree item with the current focus is on or
//                     inside a trial set or subset, new trial is placed in that set or subset.  Otherwise, operation 
//                     will create a new trial set under the "trial" subtree, and then insert the new trial within that 
//                     set -- enforcing the restriction that a Maestro trial be a child of a trial set (or subset). If 
//                     the focus is on a trial, the new trial is inserted before that trial, else it is appended to the
//                     childe list of the parent trial set or subset.  Always enabled.
//    ID_OBJ_RUN   ==> "New | Run".  Insert a new Maestro ContMode run object.  Insertion is similar to that for
//                     trials, except that runs are always children of "run set" objects.  Always enabled.
//    ID_OBJ_CHCFG ==> "New | Channel Config".  Insert a new Maestro channel configuration object.  There are no
//                     "channel config sets", so the new configuration is inserted at the base of the "channel config"
//                     subtree.  If the focus is on an existing channel configuration, the new channel configuration is
//                     inserted immediately before it; else it is appended.  Always enabled.
//    ID_OBJ_PERT  ==> "New | Perturbation".  Insert a new Maestro perturbation obj.  There are no "perturbation sets",
//                     so new perturbation is inserted at the base of the "perturbation" subtree.  If the focus is on
//                     an existing perturbation object, the new perturbation is inserted immediately before it, else it
//                     is appended.  Always enabled.
//    ID_OBJ_TGSET ==> "New | Target Set".  Insert a new target set in the "target" subtree (CX_TARGBASE).  The new
//                     target set is inserted before the target set item that currently has the focus; if the focus is
//                     not on a target set item, the new target set is appended to the target subtrees. Always enabled.
//    ID_OBJ_TRSET ==> "New | Trial Set".  Insert a new trial set within the "trial" subtree (CX_TRIALBASE).  Insertion
//                     rule is analogous to that for target sets.  Always enabled.
//    ID_OBJ_TRSUB ==> (As of Maestro v3.1.2) "New | Trial Subset".  Insert a new trial subset. If current focus is on 
//                     or inside a trial set, the new subset is placed in that set. Otherwise, the operation will create
//                     a new trial set under the "trial" subtree, and then insert the new trial subset within that set.
//                     If the focus is on a trial or trial subset, the new trial subset is inserted before the focus
//                     object, else it is appended to the child list of the parent trial set. Always enabled.
//    ID_OBJ_RUNSET==> "New | Run Set".  Insert a new ContMode run set within the "run" subtree (CX_CONTRUNBASE).
//                     Insertion rule is analogous to that for target sets.  Always enabled.
//
//          Whenever a new object is inserted into the tree as a result of one of the above operations, the
//          corresponding tree item (also new) receives both the focus and selection.  An inplace-edit is immediately
//          initiated so that the user can change the name that was supplied to the new object by default.
//
//    ID_OBJ_COPY  ==> "Copy".  Add copies of all currently selected objects (leaf objects as well as sets) to the
//                     Maestro object tree.  Enabled whenever the current tree selection is not empty.  Any predefined
//                     Maestro objects may NOT be copied.  Each new object is added as a sibling of the object copied,
//                     and it is generally inserted immediately AFTER the original object.  If only one object was
//                     copied, we immediately initiate an inplace-edit to give the user an opportunity to rename the
//                     copied item.  If multiple items are added, all are selected and the last item added gets the
//                     focus.
//    ID_OBJ_NAME  ==> "Rename".  Initiate an inplace-edit to rename the object that is currently focused.  Enabled
//                     only when a user-defined object has the focus and selection.
//    ID_OBJ_DEL   ==> "Delete".  Delete all currently selected objects (leaf objects as well as sets).  Enabled
//                     whenever the current tree selection is not empty.  Predefined objects may NOT be deleted by
//                     user.  In addition, user-defined objects that are "locked" (other Maestro data objects depend
//                     on the locked object's existence) may not be removed.  The native CTreeCtrl framework
//                     determines which item gets the focus/selection after the deletion.
//    ID_OBJ_CLEAR ==> "Remove All".  Remove all user-defined objects from the Maestro object tree.  Always enabled.
//                     After this operation, the object tree is restored to its initial state containing only
//                     predefined MAESTRO objects.  The user is prompted to make sure s/he wants to do this.
//    ID_OBJ_PROP  ==> "Show Properties".  Display the definition of the object that is currently selected and focussed
//                     in the tree.  Enabled only when the current selection & focus is on a Maestro data object.
//
//                !!!IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                ! The menu-initiated (WM_COMMAND) object tree operations are identified by !
//                ! resource ID.  Ensure that these IDs have consecutive values in the range !
//                ! [ID_OBJ_XYTGT ... ID_OBJ_PROP]. This is required so that we can use the  !
//                | ON_***_RANGE macros in the CCxObjectTree message map.                    !
//                !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// ==> Responsibilities of CCxObjectTree in the Maestro framework.
//
// 1) Content/structure of the Maestro object tree:  The user cannot insert Maestro data and set objects willy-nilly in
// the object tree.  CCxDoc strictly controls the structure of the tree by limiting the operations that can be
// performed.  However, CCxObjectTree is "aware" of this structure in the sense that it limits to some extent how the
// user can create new objects within the tree.
//
// 2) Handles all "Object" menu operations relevant to the tree, as described in the previous section.  These commands
// are initiated in a number of ways:
//    a) The top-level "Object" menu (CCxObjectTree must have the current input focus).
//    b) A context menu, identical to the "Object" menu, which pops up whenever user right-clicks on view.
//    c) The user can rename an object "in-place" by clicking on the tree item, holding the mouse button down
//       momentarily, and then releasing it.  This is essentially a short-cut for the ID_OBJ_NAME command.
//    d) Pressing the DELETE key will delete all items in the current selection that can be deleted.  A short-cut for
//       ID_OBJ_DEL.
//    e) A left double-click over a MAESTRO data object will cause that object's definition to be displayed in another
//       view.  A short-cut for ID_OBJ_PROP.  Note that predefined targets are not displayable.
//    f) The user can move or copy selected items from their current location to another in the tree by using the
//       view's drag-n-drop features.  CCxDoc prevents any "invalid" moves which would violate the structural
//       constraints of the object tree.
//
// (3) Each time the user attempts one of the above operations, the tree view queries the document to update the actual
// Maestro object tree accordingly.  If the document completes the operation successfully -- resulting in some sort of
// change to the tree--, the view updates itself appropriately, then informs all other document views of the change
// via CDocument::UpdateAllViews().
//
// (4) Update "Hints":  The CCxViewHint (see CXVIEWHINT.H) object is used to describe the document modification that
// the currently active view has just completed.  Every MAESTRO view should be able to build such an object, which is
// sent to all other views in the call to UpdateAllViews(), and every view should be able to update its appearance as
// necessary in its OnUpdate() handler based on the contents of the CCxViewHint object it receives.  The following
// hints are relevant to Maestro objects and the Maestro object tree in general.
//    a) CXVH_NEWOBJ  --> sent when new objects are created in the object tree.
//    b) CXVH_MOVOBJ  --> sent when existing objects are moved to a different location in the tree.
//    c) CXVH_NAMOBJ  --> sent when an existing object is renamed.
//    d) CXVH_DELOBJ  --> sent when objects are deleted from the tree.
//    e) CXVH_CLRUSR  --> sent when all user-defined objects are removed from the tree.
//    f) CXVH_DSPOBJ  --> the "signal" sent by CCxObjectTree to display a Maestro data obj's defn.
//    g) CXVH_MODOBJ  --> sent when a data object's defining parameters have been permanently modified.
//
//             !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//             !!! Some of the Maestro "control panels" (which are control bars managed by  !!!
//             !!! CCxMainFrame) display document data and thus must be notified of any     !!!
//             !!! change in document data initiated in a Maestro view.  Since these panels !!!
//             !!! are NOT views themselves, the default MFC doc/view architecture does not !!!
//             !!! provide a means for notifying them.  Our solution:  CCxObjectTree -- and !!!
//             !!! CCxObjectTree ONLY! -- passes on all Maestro update hints to the main    !!!
//             !!! frame window CCxMainFrame which, in turn, forwards them to the Maestro   !!!
//             !!! control panels.  In addition, a NULL hint is forwarded to the control    !!!
//             !!! panels to "simulate" CView::OnInitialUpdate().  See OnInitialUpdate(),   !!!
//             !!! OnUpdate(), and Notify().                                                !!!
//             !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// ==> Management and display of Maestro-specific item "icons".
//
// Tree controls are capable of displaying two different bitmap images to the left of each tree item's label:  an item
// icon and a state image.  All icons that appear in the control must be stored in a CImageList object and associated
// with the tree control (TVSIL_NORMAL image list).  Similarly, all state images that appear in the control must be
// stored in a second image list and again associated with the control (TVSIL_STATE image list).
//
// CCxObjectTree supports display of item icons for every defined Maestro object type; state images are not supported.
// Furthermore, CCxObjectTree changes the appearance of an item's icons under certain circumstances:
//    (1) For non-collection objects -- one icon is shown when the item is selected, another when it is not.
//    (2) For collection objects -- one icon is shown when the item is expanded, another when it is collapsed.
//
// All Maestro object type icons have been stored in the application in a single, wide bitmap resource IDB_CXICONS.  In
// its CView::OnInitialUpdate() override, CCxObjectTree loads all icons into an image list and then associates that
// list with the embedded tree control.  Method GetBitmapIDs() maps object type to the indices of the appropriate icons
// ("normal" and "selected" icons) in the installed image list.  These two indices are associated with a tree item when
// it is created.  For collection objects, the two indices are the same, but they take on one of two values depending
// on whether the parent item representing the collection is currently expanded or not.  When a parent item's state is
// toggled, the CMultiDragTreeView framework calls GetExpandBitmaps().  The CCxObjectTree override calls GetBitmapIDs()
// to retrieve the expanded or collapsed image index.
//
// ==> Important methods in CCxObjectTree.
//
// 1) CreateObjectItem():  Create a new user-defined Maestro object of specified type in the Maestro object tree, along
//    with a new item in the tree view that represents the object tree.
// 2) CopyObjectItems():  Copy a list of existing objects.  Useful for copying all currently selected objs in tree.
// 3) ClearUserObjectItems:  Remove all user-defined objects from tree.
// 4) ObjectToItem(), ItemToObject():  Maps Maestro object key to the tree item that represents that object, and vice
//    vice versa.  The object key is stored in the tree item's LPARAM member (see MFC's CTreeCtrl).
// 5) InsertObjItem():  Insert a new tree item representing the specified (and existing) Maestro object.
// 6) RefreshBranch():  Reload the entire tree view, or a portion thereof, by querying the document for each Maestro
//    object that is stored in the tree.  Useful when loading the tree view for the first time, when processing update
//    hints from other views, etc.
// 7) AcceptNewLabel() [CMultiDragTreeView override]:  Rename an existing object.
// 8) RealizeDrag() [CMultiDragTreeView override]:  Move or copy the dragged objects under the "drop target" item.
// 9) CustomDblClk() [CMultiDragTreeView override]:  When the user left double-clicks on a data object, send an
//    appropriate "display object" hint to other Maestro views.  The various data object form views respond to the hint
//    by displaying the defining parameters of the data object (such as a modifiable target, a trial table, etc).
// 10) RealizeDelete() [CMultiDragTreeView override]:  Delete all objects in the specified list.  Note that this is
//    called by the CMultiDragTreeView framework to delete the current selection when the user presses the DELETE key.
// 11) CanDrop() [CMultiDragTreeView override]:  Determine whether or not item can be the target of a drag-n-drop
//    operation.  This method allows all Maestro collection objects to serve as drop targets.  In addition, to allow
//    the user to change the order of siblings under a given node, it lets the children of a collection object serve as
//    a drop target only if the collection can handle the object types in the drag list.  In this special case, the
//    drop target specifies the INSERTION POSITION for the dragged items.
//
// ==> Drag-and-drop operations in CCxObjectTree.
//
// CCxObjectTree only supports dragging selected items from one part of the tree to another.  It does NOT currently
// support dragging or dropping objects to/from other Maestro views.  Essentially, the user can use the drag-n-drop
// features to rearrange and/or add to the contents of the Maestro object tree within the content constraints imposed
// by CCxDoc.  For example, the user could select a subset of trials from a given set and then drag them onto another
// set, either moving or copying them to that set.  In addition, if the user wants to rearrange the order of trials
// within a given set, then s/he can drag each trial to a particular insertion location.  In this latter usage, the
// "drop target" specifies the insertion position for the dragged items; the actual drop target is its parent!
//
// [NOTE that the user originally had no control over the order of sibling objects.  We changed this because order is
// actually important -- eg, when we want to control the order of presentation of trials within a trial set!]
//
//
// DEVELOPMENT NOTES:
// (1) Storage of tree control item labels.  Through a "callback" mechanism, tree controls are capable of using strings
// which are stored elsewhere.  Since all Maestro object names are stored in the document, we could take advantage of
// this mechanism to save memory resources.  However, it's tricky stuff, so I'll consider tackling it later...
//
//
// REVISION HISTORY:
// 14sep2000-- Created.
// 18sep2000-- Completed first version.
// 20sep2000-- Modified to support the tree control's image list and changing an item's icon depending on selection
//             state and expand/collapse state.  The image list itself is now a global available only in this module;
//             it is loaded only once during the runtime.  Since all CNTRLX item icons are found here, any view can
//             potentially display any icon.
// 21sep2000-- Added method ClearUserObjectItems().
// 25jan2001-
// 26jan2001-- !!!GUI REDESIGN!!! Instead of having separate tree views for each CNTRLX data class, there will only by
//             a single view -- CCxObjectTree (formerly CCxTreeVu).  There is now a single CNTRLX "object tree" in
//             CExperimentDoc, with root-level collections for each of the CNTRLX data classes ("TARGETS" subtree,
//             "TRIALS" subtree, and so on).  There is also a single top-level "Object" menu by which the user selects
//             an operation to perform on the tree (replacing the separate "Target", "Trial", etc. menus!).  Modified
//             CCxObjectTree to handle its new responsibilities.  The code from CCxTargTree and CCxTrialTree has been
//             incorporated here as well...
// 22mar2001-- CustomDblClk() now calls SwitchObjectTab() on the parent frame to ensure that the tab pane containing
//             the relevant data object definition form is brought to the front of the tab window.  The parent frame,
//             of course, must be CCxMainFrame!
// 26mar2001-- Calls LabelObjectTab() on the parent frame to update tab pane caption whenever a CNTRLX data object is
//             renamed, moved, or deleted.
// 27mar2001-- Reversed changes dtd 22mar2001 and 26mar2001.  Updates to tab panes are now handled by the views
//             installed in those tab panes.
// 27sep2001-- Cosmetic:  CExperimentDoc --> CCxDoc.
// 28sep2001-- Modified to forward all CNTRLX update hints (CCxViewHint) -- whether originated here or in another
//             CNTRLX view -- to CCxMainFrame.  CCxMainFrame forwards the hints to CNTRLX "doc-aware" control panels.
// 13nov2001-- OnInitialUpdate() now forwards a NULL update hint to CCxMainFrame and thence to the "doc-aware" control
//             panels, giving those panels an OnInitialUpdate()-like functionality.
// 08feb2002-- Modified to handle the new "video display configuration" object (type CX_VIDEODSP).  There is only one
//             of these objects in a CCxDoc, and it cannot be copied or renamed.  This object is modified via a dialog
//             page in a CNTRLX control panel (see CCxVideoDspDlg), rather than in the mainframe's client area.  Thus,
//             we DO NOT display it in the tree view.  See RefreshBranch().
// 22may2002-- Adding support for ContMode "runs" (type CX_CONTRUN) and associated "run sets" (type CX_CONTRUNSET).
// 18oct2002-- Eliminated changes dtd 08feb2002.  The "video display configuration" object is obsolete.  The video
//             display parameters are now stored in a CCxSettings object that is NOT part of the object tree.
// 17nov2002-- Adding support for perturbation objects (type CX_PERTURB) under the predefined CX_PERTBASE subtree.
//             Still need to create icons for CX_PERTURB objects...
// 15oct2003-- Eliminated alphabetical sorting of objects in the tree.  Users have indicated that, for example, the
//             order of trials in a set is significant to them, and so they want the user interface to always reflect
//             that order.
// 21oct2003-- Additional changes so that the user has full control over sibling order.  1) The currently focused tree
//             item now serves as the "insertion position" for a new Maestro object if it is of like type; otherwise,
//             the new object is appended to the end of the appropriate collection object's child list.  2) Users can
//             specify the "insertion position" for a drag-n-drop operation by dropping onto a particular child of the
//             collection in which the items are to be dropped.  Among other things, this gives the user complete
//             control over the order of user-defined sibling objects.
// 15mar2004-- Added icons for a perturbation object to the image list loaded by CCxObjectTree.
// 15mar2005-- Added support for copying the currently selected objects to another experiment document.  This action is
//             initiated by the ID_FILE_COPYREMOTE command (it's in the File menu because it involves file I/O!).  A
//             common file dialog gets the name of the file containing the destination document.  See OnCopyRemote().
//             The command is enabled only in IdleMode.
// 14mar2006-- Removed support for the CX_OKNDRUM target.
// 15mar2006-- Mods so that the "predefined" target objects CX_FIBER*, CX_REDLED* and CX_CHAIR are never loaded into
//             the target form view.  These targets have no defining parameters.
// 24mar2006-- Mods to replace VSG framebuffer video with RMVideo targets.
// 04dec2006-- Added function IsObjSelected() to expose selection state of any document object.
// 23sep2011-- Removed support for the CX_FIBER* and CX_REDLED* targets. Effective Maestro 3.0.
// 01dec2014-- Mods to support a new MAESTRO object type: a trial "subset", type ID = CX_TRIALSUBSET. Note that we use
// the same icons for both trial sets and subsets. Subset are always children of sets. Sets can contain both subsets 
// and individual trials, while subsets can only contain trials. Effective Maestro 3.1.2.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs

#include "cxobj_ifc.h"                       // MAESTRO object interface:  common constants and other defines
#include "cxmainframe.h"                     // CCxMainFrame -- the MAESTRO mainframe window class
#include "cxobjtree.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxObjectTree, CMultiDragTreeView )

BEGIN_MESSAGE_MAP( CCxObjectTree, CMultiDragTreeView )
   ON_COMMAND_RANGE( ID_OBJ_XYTGT, ID_OBJ_PROP, OnObjectOp )
   ON_COMMAND( ID_FILE_COPYREMOTE, OnCopyRemote )
   ON_UPDATE_COMMAND_UI( ID_FILE_COPYREMOTE, OnUpdObjectOps )
   ON_UPDATE_COMMAND_UI_RANGE( ID_OBJ_XYTGT, ID_OBJ_PROP, OnUpdObjectOps )
END_MESSAGE_MAP()


//=====================================================================================================================
// STATIC MEMBER INITIALIZATIONS
//=====================================================================================================================
//
COLORREF CCxObjectTree::BM_MASKCOLOR = RGB(0,128,128);

CImageList CCxObjectTree::m_imgIcons;


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

//=== OnObjectOp ======================================================================================================
//
//    ON_COMMAND_RANGE message handler which executes the requested operation from the "Object" menu.  See file header
//    for a complete description of the operations possible.
//
//    Some operations will work on multiply-selected items, while others only operate on the currently focused AND
//    selected item.  In each case, when the operation is completed, the previous multi-selection state of the tree is
//    cleared.  If a single item is added, an in-place rename operation is initiated immediately.
//
//    NOTE:  When copying or deleting a multi-selection, the "current selection set" could include items under an
//    already selected parent item.  This does not make much sense, since deleting/copying a parent item applies to all
//    of its descendants as well.  However, the CMultiDragTreeView method GetSelectedList -- which we use to obtain the
//    list of currently selected items -- removes all such "redundant items" from the selection list!  If this did not
//    happen, the copy and delete operations could malfunction....
//
//    ARGS:       cmdID -- [in] resource ID identifying which operation is to be performed.
//
//    RETURNS:    NONE
//
void CCxObjectTree::OnObjectOp( UINT cmdID )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                                  // pointer to the attached document

   CHTIList htiSelList;                                           // get the current selection set IF the command
   if ( cmdID == ID_OBJ_COPY || cmdID == ID_OBJ_DEL )             // operates on all selected items. This removes any
      GetSelectedList( htiSelList, TRUE );                        // descendants of selected items.

   SelectAll( FALSE );                                            // clear all selections
   ResetShiftSelect();                                            // reset SHIFT-initiated multi-select feature

   HTREEITEM htiFocus = GetFocusedItem();                         // the tree item that holds the focus (if any)
   HTREEITEM htiParent = NULL;                                    // parent for new item (if not the focus item)
   HTREEITEM htiInsPos = NULL;                                    // if NULL, new item is appended; else it is inserted
                                                                  // before this item

   HTREEITEM htiNew = NULL;                                       // a new tree item
   UINT nItems = 0;                                               // # of items added or removed
   WORD wType, wKey;
   WORD wParentType, wBaseType;
   switch( cmdID )                                                // process requested operation:
   {
      case ID_OBJ_XYTGT :                                         // insert a new XY scope or RMVideo target
      case ID_OBJ_FBTGT :
         wType = (cmdID==ID_OBJ_XYTGT) ? CX_XYTARG : CX_RMVTARG;  //    type of target to insert

         htiParent = htiFocus;                                    //    determine if focus is within an existing
         while( htiParent != NULL )                               //    container for the new target object...
         {
            wKey = ItemToObject( htiParent );
            if( pDoc->AcceptsSubObj( wKey, wType ) )
               break;
            htiParent = cTree.GetParentItem( htiParent );
            if( htiParent == TVI_ROOT ) htiParent = NULL;
         }

         if( htiParent == NULL )                                  //    otherwise, append it end of child list of the
         {                                                        //    target subtree's base node
            wKey = pDoc->GetBaseObj( CX_TARGBASE );
            htiParent = ObjectToItem( wKey );
         }

         if( (htiFocus != NULL) &&                                //    if focus item is child of the parent that will
             (cTree.GetParentItem( htiFocus ) == htiParent) )     //    contain new node, we insert new node before it
            htiInsPos = htiFocus;

         ASSERT( htiParent != NULL );
         htiNew = CreateObjectItem( htiParent, htiInsPos, wType );
         break;

      case ID_OBJ_TRIAL :                                         // insert a new trial or cont-mode run object, or a
      case ID_OBJ_TRSUB :                                         // a new trial subset
      case ID_OBJ_RUN :
         wType = (cmdID==ID_OBJ_RUN) ? CX_CONTRUN : (cmdID==ID_OBJ_TRIAL ? CX_TRIAL : CX_TRIALSUBSET);
         wParentType = (wType==CX_CONTRUN) ? CX_CONTRUNSET : CX_TRIALSET;
         wBaseType = (wType==CX_CONTRUN) ? CX_CONTRUNBASE : CX_TRIALBASE;

         htiParent = htiFocus;                                    //    determine if focus is within an existing
         while( htiParent != NULL )                               //    container for the object to be created...
         {
            wKey = ItemToObject( htiParent );
            if( pDoc->AcceptsSubObj( wKey, wType ) )
               break;
            htiParent = cTree.GetParentItem( htiParent );
            if( htiParent == TVI_ROOT ) htiParent = NULL;
         }

         if( htiParent != NULL )                                  //    focus is in a suitable collection, so put the
         {                                                        //    new object there...
            if( (htiFocus != NULL) && (cTree.GetParentItem( htiFocus ) == htiParent) )
               htiInsPos = htiFocus;
            htiNew = CreateObjectItem( htiParent, htiInsPos, wType );
         }
         else                                                     //    otherwise, we must create a new set to
         {                                                        //    contain it...
            wKey = pDoc->GetBaseObj( wBaseType );
            HTREEITEM hti = ObjectToItem( wKey );
            htiParent = CreateObjectItem( hti, NULL, wParentType );
            if( htiParent != NULL )
               htiNew = CreateObjectItem( htiParent, NULL, wType );
         }

         break;

      case ID_OBJ_CHCFG :                                         // insert other object types under the appropriate
      case ID_OBJ_PERT :                                          // subtree...
      case ID_OBJ_TGSET :
      case ID_OBJ_TRSET :
      case ID_OBJ_RUNSET :
         if( cmdID == ID_OBJ_CHCFG ) { wType = CX_CHANCFG; wBaseType = CX_CHANBASE; }
         else if( cmdID == ID_OBJ_PERT ) { wType = CX_PERTURB; wBaseType = CX_PERTBASE; }
         else if( cmdID == ID_OBJ_TGSET ) { wType = CX_TARGSET; wBaseType = CX_TARGBASE; }
         else if( cmdID == ID_OBJ_TRSET ) { wType = CX_TRIALSET; wBaseType = CX_TRIALBASE; }
         else { wType = CX_CONTRUNSET; wBaseType = CX_CONTRUNBASE; }

         htiParent = ObjectToItem( pDoc->GetBaseObj( wBaseType ) );
         if( (htiFocus != NULL) && (cTree.GetParentItem( htiFocus ) == htiParent) )
            htiInsPos = htiFocus;
         htiNew = CreateObjectItem( htiParent, htiInsPos, wType );
         break;

      case ID_OBJ_CLEAR :                                         // clear all user-defined objects
         ClearUserObjectItems();
         break;

      case ID_OBJ_COPY  :                                         // copy all selected objects (if possible)
         nItems = CopyObjectItems( htiSelList, htiNew );
         if ( htiNew != NULL)                                     //    handle of last item added by copy operation
         {
            FocusItem( htiNew );                                  //    the last item added also gets the focus
            if ( nItems > 1 ) htiNew = NULL;                      //    if more than one item added, don't allow rename
         }
         break;

      case ID_OBJ_DEL   :                                         // delete all selected objects (if possible)
         RealizeDelete( htiSelList );
         break;

      case ID_OBJ_NAME  :                                         // rename an object
         ASSERT( htiFocus );                                      //    only applies to focus item, which MUST exist!
         SetItemState( htiFocus, TVIS_SELECTED, TVIS_SELECTED );  //    it retains selection
         cTree.EditLabel( htiFocus );                             //    initiate in-place edit
         break;

      case ID_OBJ_PROP  :                                         // display an object's definition
         ASSERT( htiFocus );                                      //    only applies to focus item, which MUST exist!
         CustomDblClk( htiFocus );                                //    same as left double-clicking an item!
         break;

      default :                                                   // we should never get here!!
         ASSERT( FALSE );
         break;
   }


   if ( htiNew != NULL )                                          // when a single new item is inserted, let user
   {                                                              // change its default name immediately.
      cTree.EnsureVisible( htiNew );
      cTree.EditLabel( htiNew );
   }

}


//=== OnUpdObjectOps ==================================================================================================
//
//    Whenever the framework is about to display the "Object" menu listing possible object-tree operations, this method
//    is called to enable/disable the various object operations depending on the current view state.  If this view does
//    not have the focus, then all operations are disabled.  Otherwise, some or all ops will be enabled depending on
//    which tree items (if any) are selected or which item has the focus.
//
//    See file header for a complete description of the available "Object" menu commands and the conditions under which
//    they are enabled.
//
//    Also handles enable state of the File menu's ID_FILE_COPYREMOTE command.  This command is enabled only when the
//    current selection is non-empty and Maestro is not running in one of its time-critical active op modes.
//
//    ARGS:       pCmdUI -- [in] handle to object used to modify appearance of menu items
//
//    RETURNS:    NONE
//
void CCxObjectTree::OnUpdObjectOps( CCmdUI* pCmdUI )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                                  // pointer to the attached document

   CFrameWnd* pFrame = GetParentFrame();                          // if parent frame iconicized or treeview does not
   ASSERT( pFrame );                                              // have focus, then all ops are disabled...
   BOOL bEnable = ( !(pFrame->IsIconic()) &&
                    ( &cTree == (CTreeCtrl*) GetFocus() ) );

   HTREEITEM htiFocus;
   WORD cmd = pCmdUI->m_nID;
   UINT nSel = GetSelectedCount();
   if ( bEnable ) switch ( cmd )
   {
      case ID_OBJ_XYTGT :                                         // these operations are always enabled...
      case ID_OBJ_FBTGT :
      case ID_OBJ_TRIAL :
      case ID_OBJ_RUN :
      case ID_OBJ_CHCFG :
      case ID_OBJ_PERT :
      case ID_OBJ_TGSET :
      case ID_OBJ_TRSET :
      case ID_OBJ_TRSUB :
      case ID_OBJ_RUNSET :
      case ID_OBJ_CLEAR :
         break;

      case ID_OBJ_COPY  :                                         // enabled as long as at least one item is selected;
      case ID_OBJ_DEL   :                                         // if one item selected, it must be user-defined
         if ( nSel == 1 )
         {
            HTREEITEM hti = GetFirstSelectedItem();
            WORD key = ItemToObject( hti );
            bEnable = pDoc->IsUserObj( key );
         }
         else
            bEnable = BOOL( nSel > 0 );
         break;

      case ID_OBJ_NAME  :                                         // these cmds require a focussed & selected item...
      case ID_OBJ_PROP  :
         htiFocus = GetFocusedItem();
         bEnable = BOOL( htiFocus != NULL );
         if ( bEnable ) bEnable = IsSelected( htiFocus );
         if ( bEnable )
         {
            WORD key = ItemToObject( htiFocus );
            if( cmd == ID_OBJ_NAME )                              // ...only user-defined objects can be renamed
               bEnable = pDoc->IsUserObj( key );
            else                                                  // ...only data objs have properties for display,
            {                                                     // except for the predefined CHAIR target.
               bEnable = !pDoc->IsCollectionObj( key );
               if( bEnable )
               {
                  WORD wType = pDoc->GetObjType( key );
                  bEnable = BOOL(wType != CX_CHAIR);
               }
            }
         }
         break;
      
      case ID_FILE_COPYREMOTE :                                   // this cmd requires a non-empty selection, and it
         bEnable = BOOL( (nSel > 0) &&                            // is only enabled when runtime is idled
               !((CCntrlxApp*)AfxGetApp())->IsTimeCritical() );
         break;

      default :                                                   // we should never get here!
         bEnable = FALSE;
         break;
   }

   // set enabled state of corresponding menu item appropriately
   //
   pCmdUI->Enable( bEnable );
}


//=== OnCopyRemote() ==================================================================================================
//
//    OnCopyRemote() handles the ID_FILE_COPYREMOTE command from the "File" submenu of the main menu.  This command
//    allows the user to copy all of the currently selected objects into a second experiment document.  A common file
//    dialog is displayed to let the user browse and select the Maestro document file into which the selected objects
//    are to be copied.  The current document is unchanged by the operation.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxObjectTree::OnCopyRemote()
{
   CHTIList htiSelList;                                                    // get current selection set. removes any
   GetSelectedList( htiSelList, TRUE );                                    // descendants of selected items.  if there
   if( htiSelList.IsEmpty() )                                              // are no selections, abort.
      return;

   SelectAll( FALSE );                                                     // clear all selections, then select the
   ResetShiftSelect();                                                     // focussed item, if there is one.
   SelectItem( GetFocusedItem() );

   CString fileName;                                                       // get pathname of destination document
   if( !AfxGetApp()->DoPromptFileName(fileName,                            // from user via common file dlg.  If user
         IDS_COPYREMOTE_FILEDLGTITLE,                                      // cancels out of dlg, abort.
         OFN_HIDEREADONLY|OFN_FILEMUSTEXIST, TRUE, NULL) )
      return;

   CCxDoc destDoc;                                                         // open the selected document
   if( !destDoc.OnOpenDocument(fileName) )
      return;                                                              // user already notified of error

   CWordArray wArKeys;                                                     // get keys of all objects to be copied
   wArKeys.SetSize(10);                                                    // (no particular order guaranteed)
   while( !htiSelList.IsEmpty() )
   {
      HTREEITEM hSrc = htiSelList.RemoveHead();
      ASSERT( hSrc != NULL );
      wArKeys.Add( ItemToObject(hSrc) );
   }

   if( !destDoc.CopySelectedObjectsFromDocument(GetDocument(), wArKeys) )  // copy selected objects to destination doc
   {
      ::AfxMessageBox(_T("Unable to copy objects to selected document!"), MB_ICONEXCLAMATION);
      return;
   }

   if( destDoc.IsModified() )                                              // if something was actually copied, save
   {                                                                       // changes to destination document
      destDoc.OnSaveDocument(fileName);                                    // user notified if error on save
   }
}




//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== OnInitialUpdate [base override] =================================================================================
//
//    This function is called by the doc/view framework just before the view is displayed for the first time, or when a
//    new document is created/opened in an SDI app.  Here we can perform any per-document initializations to customize
//    the appearance of the tree view, insert default items, etc.
//
//    Here we load the *static member* image list object which holds all of the Maestro object item icons.  Since it is
//    static, it will be shared by all instances of CCxObjectTree (altho we only plan to use one instance!!) -- we
//    will only have to load it once.  The icons are stored in a single wide bitmap resource, IDB_CXICONS.
//
//    We also forward a NULL CCxViewHint to the Maestro control panels via CCxMainFrame (only CCxObjectTree does this!)
//    so that these panels can perform any per-document initializations.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxObjectTree::OnInitialUpdate()
{
   if ( m_imgIcons.m_hImageList == NULL )
      m_imgIcons.Create( IDB_CXICONS, BM_WIDTH, 4, BM_MASKCOLOR );

   GetTreeCtrl().SetImageList( &m_imgIcons, TVSIL_NORMAL );
   CMultiDragTreeView::OnInitialUpdate();
   ((CCntrlxApp*)AfxGetApp())->GetMainFrame()->OnUpdate( NULL );  // route "OnInitialUpdate()" to Maestro ctrl panels
}


//=== OnUpdate [base override] ========================================================================================
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    Update state of the object tree view IAW the information in the CCxViewHint object (if provided).  The hint helps
//    us do the minimum amount of work necessary to ensure that the view reflects the current document state.  If no
//    hint is provided (which will be the case when the MFC framework initiates the update), we completely empty the
//    tree and reinitialize it.  We do nothing to the document here, since it has been updated by the initiating view.
//
//    NOTE:  CCxObjectTree was chosen to pass on Maestro update hints to the main frame window, which manages some
//    non-view but document-aware GUI elements.  This method forwards hints from other views, while Notify() forwards
//    hints initiated by this view, and OnInitialUpdate() forwards a NULL hint.
//
//    ARGS:       pSender  -- [in] view which initiated the update
//                lHint    -- [in] an integer-valued hint (not used by Maestro views)
//                pHint    -- [in] if the initiating view provides a hint, this should be a CCxViewHint object.
//
//    RETURNS:    NONE
//
void CCxObjectTree::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   CTreeCtrl& cTree = GetTreeCtrl();                     // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();

   CCxViewHint noHint;                                   // if no Maestro view hint provided, use the empty one; else
   CCxViewHint* pVuHint;                                 // cast it appropriately.
   if ( pHint == NULL )
      pVuHint = &noHint;
   else
      pVuHint = (CCxViewHint*)pHint;

   HTREEITEM hti = NULL;                                 // handle to a tree item
   CString name;                                         // name of a Maestro object
   WORD key = pVuHint->m_key;                            // unique key identifying affected object (if only one)
   switch( pVuHint->m_code )                             // tailor update IAW provided hint (if any):
   {
      case CXVH_NEWOBJ :                                 // one or more objects have been added to the object tree,
      case CXVH_MOVOBJ :                                 // or moved within the tree...
         RefreshBranch( NULL );                          //    let's just refresh entire tree to play it safe
         break;

      case CXVH_NAMOBJ :                                 // an object was renamed -- update its label in view
         hti = ObjectToItem( key );
         if( hti != NULL )
         {
            pDoc->GetObjName( key, name );
            cTree.SetItemText( hti, name );
         }
         break;

      case CXVH_DELOBJ :                                 // one or more objects deleted from tree:
         if( key == CX_NULLOBJ_KEY )                     //    more than one object removed -- refresh entire tree!
            RefreshBranch( NULL );
         else                                            //    one object removed -- find corres tree item & remove it
         {
            hti = ObjectToItem( key );
            if( hti != NULL ) DeleteItemEx( hti );
         }
         break;

      case CXVH_CLRUSR :                                 // ...clear object tree, or no hint provided -- here we must
      case CXVH_NONE :                                   //    rebuild entire tree based on current doc contents.
         RefreshBranch( NULL );
         break;

      default :                                          // ...all other hints -- irrelevant to this view!
         break;
   }

   if( pVuHint->m_code != CXVH_NONE )                    // forward all Maestro-specific updates to our mainframe wnd!!
   {                                                     // CCxObjectTree is the only view that does this!!!
      CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
      pApp->GetMainFrame()->OnUpdate( pVuHint );
   }

   if ( pVuHint->m_code == CXVH_NONE )                   // call base class version only when no hint is provided!
      CMultiDragTreeView::OnUpdate( pSender, lHint, pHint );
}


//=== IsObjSelected ===================================================================================================
// 
//    Is specified document object currently selected in the object tree? The method returns TRUE if an item in the 
//    current selection set corresponds to the specified object key.
//
//    ARGS:       wKey -- [in] Unique key of the document object to be checked.
//
//    RETURNS:    TRUE if specified object is currently selected in object tree; FALSE otherwise.
//
BOOL CCxObjectTree::IsObjSelected(WORD wKey)
{
   CCxDoc* pDoc = GetDocument();                                  // get attached document
   if(!pDoc->ObjExists(wKey)) return(FALSE);                      // if obj does not exist, it can't be selected!

   HTREEITEM hItem = GetFirstSelectedItem();                      // search selected set for a match
   while(hItem != NULL)
   {
      if(wKey == ItemToObject(hItem)) return(TRUE);
      hItem = GetNextSelectedItem(hItem);
   }

   return(FALSE);

}


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== Notify  =========================================================================================================
//
//    Utility to notify other Maestro views and the Maestro mainframe window of a change initiated here.  It is assumed
//    that the protected member 'm_hint' has been set to the appropriate Maestro view hint (CCxViewHint).
//
//    ARGS:       bMod  -- [in] if TRUE, the change initiated in this view modified the document [default=TRUE].
//
//    RETURNS:    NONE.
//
VOID CCxObjectTree::Notify( const BOOL bMod /*= TRUE */ )
{
   if( bMod ) GetDocument()->SetModifiedFlag( TRUE );
   GetDocument()->UpdateAllViews( this, LPARAM(0), (CObject*) &m_hint );
   ((CCntrlxApp*)AfxGetApp())->GetMainFrame()->OnUpdate( &m_hint );
}


//=== CreateObjectItem ================================================================================================
//
//    Create a new Maestro object of the specified type, inserting it into the internal Maestro object tree managed by
//    CCxDoc.  If successful, insert a tree item in the view itself to represent the new object, and send an
//    appropriate hint to other views to signal the change that has occurred.
//
//    NOTE1:  This method should only be used when adding ONE object to the tree.  When multiple objects are added in
//    rapid succession using this method, performance may be poor because every view will get a chance to update itself
//    in response to each addition.  Better to delay the update until all the objects have been added.
//
//    NOTE2:  This method enforces the constraint that no user-defined objects can be inserted at the root level of the
//    tree view.  The predefined target, trial, etc. subtrees appear at this level...
//
//    ARGS:       htiDst   -- [in] tree item that will parent new object (must not be NULL or TVI_ROOT).
//                htiInsPos-- [in] tree item that is a child of the specified parent and indicates the insertion
//                            pos for the new object-item; if NULL, the new object item is appended.
//                newTyp   -- [in] Maestro object type for the object to be created.
//
//    RETURNS:    handle of new tree item representing Maestro object just created; NULL upon failure (memory except).
//
HTREEITEM CCxObjectTree::CreateObjectItem( const HTREEITEM htiDst, const HTREEITEM htiInsPos, const WORD newTyp )
{
   CCxDoc* pDoc = GetDocument();                                        // get attached document

   ASSERT( (htiDst != NULL) && (htiDst != TVI_ROOT) );                  // cannot insert at root level!

   WORD dstKey = ItemToObject( htiDst );                                // get key of obj assoc w/ parent item
   ASSERT( pDoc->AcceptsSubObj( dstKey, newTyp ) );                     // parent obj MUST accept specified obj type

   WORD sibKey = CX_NULLOBJ_KEY;                                        // if sibling tree item is specified as insert
   HTREEITEM htiAfter = TVI_LAST;                                       // pos, get key of assoc obj; the new object
   if( htiInsPos != NULL )                                              // will be inserted BEFORE this object.  else
   {                                                                    // the new object is appended. we must get prev
      sibKey = ItemToObject( htiInsPos );                               // sib of specified sib, b/c CTreeCtrl requires
      ASSERT( pDoc->GetParentObj( sibKey ) == dstKey );                 // tree item AFTER which insertion should be
      htiAfter = GetTreeCtrl().GetPrevSiblingItem( htiInsPos );         // made. if sib has no prev sib, then we must
      if( htiAfter == NULL ) htiAfter = TVI_FIRST;                      // be inserting at head of child list.
   }

   HTREEITEM hti = NULL;                                                // handle of tree item that will rep new obj
   WORD newKey = pDoc->InsertObj( dstKey, newTyp, NULL, sibKey );       // query document to create new object
   if( newKey != CX_NULLOBJ_KEY )                                       // if new object was created, try to insert
   {                                                                    // new tree item to represent it...
      hti = InsertObjItem( newKey, NULL, TVIS_FOCUSED|TVIS_SELECTED,
               htiDst, htiAfter );
      if ( hti != NULL )                                                // success -- send signal to other views...
      {
         m_hint.Set( CXVH_NEWOBJ, newTyp, newKey );
         Notify();
      }
      else                                                              // failure -- remove obj in doc as well!
         pDoc->RemoveObj( newKey );
   }

   return( hti );
}


//=== CopyObjectItems =================================================================================================
//
//    Duplicate the objects represented by the specified list of tree items.  Typically, this is the list of items
//    currently selected in the tree view.  Also send a "new object(s)" signal to other views attached to the Maestro
//    experiment document.  Each added item is marked as selected.
//
//    CCxDoc::DuplicateObj() inserts the duplicate object immediately after the source object in the parent object's
//    child list.  This method likewise inserts the tree item for the duplicate object immediately after the source
//    object item so that the treeview represents the actual structure of the Maestro object tree.
//
//    NOTE:  For proper operation, the list must NOT contain an item that is a descendant of another item in the list!
//
//    ARGS:       htiList  -- [in] list of items in tree view that are to be copied.
//                hLast    -- [out] handle to last tree item added to the view as a result of the copy operation.
//
//    RETURNS:    # of items in the list that were actually copied (since any item in list may represent a collection,
//                this is NOT the same as the # of individual objects added to the object tree!)
//
int CCxObjectTree::CopyObjectItems( CHTIList& htiList, HTREEITEM& hLast )
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                                  // get attached document

   hLast = NULL;                                                  // initialize last item added by the copy operation.
   int nItems = 0;                                                // #items in list that were successfully copied
   BOOL bSimple = TRUE;                                           // set only if copying a single non-collection obj

   while( !htiList.IsEmpty() )                                    // keep copying tree items until this list is empty:
   {
      HTREEITEM hSrc = htiList.RemoveHead();                      //    get next item to copy, removing it from list
      ASSERT( hSrc != NULL );

      WORD kNew = pDoc->DuplicateObj( ItemToObject( hSrc ) );     //    attempt duplication of object in document
      if( kNew == CX_NULLOBJ_KEY )                                //    if unsuccessful, move on to next item...
         continue;

      HTREEITEM hDst = cTree.GetParentItem( hSrc );               //    get handle of item that contains the src item
      if( hDst == NULL ) hDst = TVI_ROOT;

      HTREEITEM hti = InsertObjItem( kNew, NULL, TVIS_SELECTED,   //    insert tree item representing new object; it is
                                     hDst, hSrc );                //    inserted immediately after the item copied

      if( hti != NULL )                                           //    success -- if new item has descendants, build
      {                                                           //       its subtree as well
         hLast = hti;
         ++nItems;
         if ( pDoc->GetFirstChildObj( kNew ) != NULL )
         {
            RefreshBranch( hti );
            bSimple = FALSE;
         }
      }
      else
         pDoc->RemoveObj( kNew );
   }

   if( nItems > 0 )                                               // send "new object" hint to other attached views
   {
      if( bSimple && (nItems == 1) )
      {
         WORD key = ItemToObject( hLast );
         m_hint.Set( CXVH_NEWOBJ, pDoc->GetObjType( key ), key );
      }
      else
         m_hint.Set( CXVH_NEWOBJ, 0, CX_NULLOBJ_KEY );
      Notify();

      if( bSimple && (nItems == 1) )                              // also, when we copy a single non-collection object,
      {                                                           // display the copy in the appropriate form view
         m_hint.m_code = CXVH_DSPOBJ;
         Notify();
      }
   }

   return( nItems );
}


//=== ClearUserObjectItems ============================================================================================
//
//    Remove all user-defined objects from the Maestro object tree.  Update the view accordingly and inform all other
//    Maestro views of the change.  Predefined objects are unaffected.  Because this is a drastic operation, we first
//    throw up a confirmation dialog to make sure the user wants to do it.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxObjectTree::ClearUserObjectItems()
{
   CString msg;
   msg.Format( "This operation will completely reinitialize the experiment document!\n\nDo you wish to continue?" );

   UINT mbStyle = MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2;

   if( AfxMessageBox( msg, mbStyle ) == IDOK )
   {
      m_hint.Set( CXVH_CLRUSR, 0, CX_NULLOBJ_KEY );      // notify views that are displaying user objects to unload
      Notify();                                          // them BEFORE the objects are actually deleted!!
      GetDocument()->ClearUserObj();
      RefreshBranch( NULL );
   }
}


//=== ObjectToItem ====================================================================================================
//
//    Find handle of tree item associated with a specified Maestro object.  Since this method could search every item
//    in the embedded tree control until it finds the item (if any) holding the object's key, it can be slow when there
//    are a lot of objects stored in the tree.
//
//    DEV NOTES:  MFC's CTreeCtrl does not provide a convenient way to search ALL items in the tree control, whether
//    visible (ie, part of an expanded subtree) or not.  Hence, we use the tree relationships (child, parent, sibling)
//    to traverse the entire tree....  Possible way to enhance search speed:  Query document for the object's
//    "ancestry".  Then search tree for the upper-most ancestor.  If found, search for the next-level ancestor, etc.
//
//    ARGS:       key      -- [in] key of Maestro object that is sought in tree.
//                htiBase  -- [in] base node at which to start searching, if only searching a subtree within the tree
//                            control.  if NULL or TVI_ROOT, the entire tree control is searched. [default = NULL].
//                bDeep    -- [in] if TRUE, search entire subtree hierarchy; else, search only the base node and its
//                            immediate children (default = TRUE).
//
//    RETURNS:    if found, handle of the tree item; else NULL (item not found in tree, OR object not found in doc).
//
HTREEITEM CCxObjectTree::ObjectToItem( WORD key, HTREEITEM htiBase /* =NULL */, BOOL bDeep /* =TRUE */ ) const
{
   CTreeCtrl& cTree = GetTreeCtrl();                              // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                                  // get attached document

   if ( !pDoc->ObjExists( key ) ) return( NULL );                 // did not find CNTRLX obj in document!

   if( htiBase == NULL ) htiBase = TVI_ROOT;
   if( htiBase != TVI_ROOT && (key == ItemToObject(htiBase)) )    // the base node is the node sought!
      return( htiBase );

   HTREEITEM hti = cTree.GetChildItem( htiBase );                 // search the subtree for an item representing the
   while( hti != NULL )                                           // CNTRLX object:
   {
      if ( key == ItemToObject( hti ) )                           //    found it!
         break;

      HTREEITEM htiLast = hti;
      hti = bDeep ? cTree.GetChildItem( htiLast ) : NULL;         //    visit children next, if it's a deep search
      if ( hti == NULL )                                          //    if no children: visit next sib, if any
         hti = cTree.GetNextSiblingItem( htiLast );

      while( hti == NULL )                                        //    unwind one level at a time until we find a
      {                                                           //    next sib we have not yet visited...
         htiLast = cTree.GetParentItem( htiLast );
         if ( (htiLast == NULL) || (htiLast == htiBase) ) break;  //    we've searched entire subtree unsuccessfully!
         hti = cTree.GetNextSiblingItem( htiLast );
      }
   }
   return( hti );
}


//=== InsertObjItem ===================================================================================================
//
//    Insert an item in the tree view corresponding to a specified object in the attached Maestro document's internal
//    representation of the tree.  In order to "cement" the relationship between tree view item and the Maestro object,
//    the object's unique key is saved in the item's LPARAM member.
//
//    TODO:  Memory exception -- unable to allocate space for new tree item!!!!
//
//    ARGS:       key      -- [in] key of Maestro object to be associated with tree item.  MUST be valid!
//                pObj     -- [in] ptr to the actual object, for access to name & type; if NULL, query doc for ptr.
//                nState   -- [in] initial state of item (TVIS_SELECTED, etc.)
//                htiDst   -- [in] tree item which will parent the new tree item; if NULL, TVI_ROOT is used.
//                htiAfter -- [in] where to insert among parent item's children; if not supplied, TVI_LAST assumed.
//
//    RETURNS:    if successful, handle of inserted tree item; otherwise, NULL.
//
HTREEITEM CCxObjectTree::InsertObjItem(
   const WORD key,
   CTreeObj* pObj,
   UINT nState,
   HTREEITEM htiDst,
   HTREEITEM htiAfter   // = TVI_LAST
   )
{
   CTreeCtrl& cTree = GetTreeCtrl();                        // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                            // get attached document

   if( pObj == NULL )                                       // if necessary, query doc for obj -- it must be present!
   {
      pObj = pDoc->GetObject( key );
      ASSERT( pObj != NULL );
   }

   WORD objType = pObj->DataType();                         // get object's type and name
   CString name = pObj->Name();

   int iImg;                                                // get item bitmap IDs, which are based on object's type
   int iSelImg;
   GetBitmapIDs( objType, FALSE, &iImg, &iSelImg );

   UINT nMask = TVIF_TEXT | TVIF_PARAM;                     // prepare mask indicating what attributes will be set
   if( iImg >= 0 ) nMask |= TVIF_IMAGE;
   if( iSelImg >= 0 ) nMask |= TVIF_SELECTEDIMAGE;

   if( htiDst == NULL ) htiDst = TVI_ROOT;
   HTREEITEM htiNew =                                       // insert the tree item:
      cTree.InsertItem( nMask, name, iImg, iSelImg, 0, 0,
                        (LPARAM)MAKELONG(key, 0),           // ... object's key is saved with item for future access!
                        htiDst, htiAfter );

   if ( (nState != 0) && (htiNew != NULL) )                 // set initial item state -- REMEMBER: cannot use native
      SetItemState( htiNew, nState, nState );               // CTreeCtrl method!

   return( htiNew );
}


//=== RefreshBranch ===================================================================================================
//
//    Rebuild the Maestro object tree branch based at the specified node.  If NULL, rebuild the entire representation
//    of the object tree in this view.  The branch is first removed entirely, then the document is queried to recreate
//    the branch base node and all its descendants.
//
//    After a refresh, the branch is collapsed in appearance and no item within it is selected or focussed.
//
//    NOTES:
//    (1) The tree branch is rebuilt by traversing the document's internal representation of the branch in a particular
//    order, which may be described recursively:  visit node, visit node's children, move on to node's next sibling.
//    We keep track of the nesting level in order to stop the traversal after we've visited all of the branch base
//    node's children.
//    (2) A "refresh" is required in certain situations -- when the tree view is loaded from the attached doc, when an
//    attempt to remove an entire branch of the tree is only partially successful, etc.
//    (3) The specified tree item MUST still correspond to an existing Maestro object in the document!
//    (4) To refresh the *entire* tree, we query CCxDoc for the root object.  The tree root itself is not actually
//    displayed as an item in the tree view -- it corresponds to TVI_ROOT.
//
//    ARGS:       hti   -- [in] handle to base of tree branch that is to be "refreshed".
//
//    RETURNS:    NONE
//
void CCxObjectTree::RefreshBranch( HTREEITEM hti )
{
   CTreeCtrl& cTree = GetTreeCtrl();                           // a reference to the embedded tree control
   CCxDoc* pDoc = GetDocument();                               // get attached document

   WORD key;                                                   // key value of CNTRLX obj assoc w/ current tree item
   HTREEITEM htiParent;                                        // parent of current tree item
   HTREEITEM htiAfter = TVI_LAST;                              // previous sibling of tree branch to be refreshed

   if ( hti == NULL )                                          // refresh entire tree...
   {
      key = pDoc->GetBaseObj();                                //    start at tree root
      htiParent = NULL;                                        //    the root has no parent
      cTree.DeleteAllItems();                                  //    delete ALL items in tree view
   }
   else                                                        // ...or refresh only a branch of the tree...
   {
      key = ItemToObject( hti );                               //    get key of obj assoc w/ base item of branch
      htiParent = cTree.GetParentItem( hti );                  //    get base item's parent in tree view
      if( htiParent == NULL ) htiParent = TVI_ROOT;

      htiAfter = cTree.GetPrevSiblingItem( hti );              //    get prev sibling of base item so that we insert
      if( htiAfter == NULL ) htiAfter = TVI_FIRST;             //    the root node of branch where it's supposed to be

      DeleteItemEx( hti );                                     //    delete just the branch
   }


   int nest = 0;                                               // current nest level in traversal of tree branch
   POSITION pos = pDoc->InitTraverseObj( key );                // initiate nested traversal of tree starting at base
   ASSERT( pos != NULL );                                      // node of branch, which MUST exist in doc!
   do
   {
      int incrNest;                                            // change in nest level
      CTreeObj* pObj;                                          // ptr to CNTRLX obj assoc w/ current tree item
      pDoc->TraverseObj( pos, incrNest, key, pObj );           // get current obj in tree traversal and advance to pos
                                                               // of the next obj

      HTREEITEM htiNew;                                        // insert tree view item representing current obj --
      if( pObj->DataType() != CX_ROOT )                        //    unless current obj is the tree root!
         htiNew = InsertObjItem( key, pObj, 0, htiParent,
            (nest==0) ? htiAfter : TVI_LAST );                 //    reinsert root of tree *branch* at correct loc
      else
         htiNew = TVI_ROOT;

      nest += incrNest;                                        // update nest level & current parent:
      if ( incrNest == 0 ) ;                                   //    moving to next sib  -- no change
      else if (incrNest == 1 ) htiParent = htiNew;             //    moving to child -- curr obj becomes curr parent
      else while( incrNest < 0 )                               //    reached end of a list of children; unwinding one
      {                                                        //    or more nest levels...
         if ( htiParent == TVI_ROOT ) break;
         htiParent = cTree.GetParentItem( htiParent );
         ++incrNest;
      }

   } while( (pos != NULL) && (nest > 0) );                     // stop when we've run out of objs or returned to
                                                               // starting point

   return;
}


//=== GetBitmapIDs ====================================================================================================
//
//    Retrieve the image list indices of the "normal" and "selected" icons which reflect the specified type of Maestro
//    object.
//
//    NOTE:  Providing the expanded state of item allows us to present a different image when an item is expanded vs
//    collapsed.  This, of course, only applies to items having children -- ie, collection objects.
//
//    ARGS:       objType  -- [in] type of Maestro object associated w/ a tree item.
//                bExpand  -- [in] TRUE if item is expanded, FALSE if it is collapsed (collection objects only).
//                piImg    -- [out] pos of "normal" bitmap in image list.
//                piSelImg -- [out] pos of "selected" bitmap in image list.
//
//    RETURNS:    NONE
//
VOID CCxObjectTree::GetBitmapIDs( const WORD objType, const BOOL bExpand, int* piImg, int* piSelImg ) const
{
   switch( objType )
   {
      case CX_TARGBASE :
      case CX_TRIALBASE :
      case CX_CHANBASE :
      case CX_CONTRUNBASE :
      case CX_PERTBASE :
         if ( bExpand ) *piImg = *piSelImg = SUB_EXPANDED;
         else           *piImg = *piSelImg = SUB_COLLAPSED;
         break;

      case CX_TARGSET :
         if ( bExpand ) *piImg = *piSelImg = TGSET_EXPANDED;
         else           *piImg = *piSelImg = TGSET_COLLAPSED;
         break;

      case CX_TRIALSET :
      case CX_TRIALSUBSET :
         if ( bExpand ) *piImg = *piSelImg = TRSET_EXPANDED;
         else           *piImg = *piSelImg = TRSET_COLLAPSED;
         break;

      case CX_CONTRUNSET :
         if ( bExpand ) *piImg = *piSelImg = CRSET_EXPANDED;
         else           *piImg = *piSelImg = CRSET_COLLAPSED;
         break;

      case CX_CHAIR :
      case CX_XYTARG :
      case CX_RMVTARG :
         *piImg = TG_NORMAL;
         *piSelImg = TG_SELECTED;
         break;

      case CX_TRIAL :
         *piImg = TR_NORMAL;
         *piSelImg = TR_SELECTED;
         break;

      case CX_CONTRUN :
         *piImg = CR_NORMAL;
         *piSelImg = CR_SELECTED;
         break;

      case CX_CHANCFG :
         *piImg = CH_NORMAL;
         *piSelImg = CH_SELECTED;
         break;

      case CX_PERTURB :
         *piImg = PERT_NORMAL;
         *piSelImg = PERT_SELECTED;
         break;

      default :
         TRACE0( "Unrecognized Maestro object type!" );
         ASSERT( FALSE );
         break;
   }

}


//=== CustomDblClk [base override] ====================================================================================
//
//    This CMultiDragTreeView virtual helper function is called whenever the user double-clicks the left mouse button
//    within the tree view's client area.  It allows derived classes to provide a custom response to the action.
//
//    When a tree item representing a modifiable Maestro data object (ie, NOT a collection object and NOT one of the
//    predefined target objects) is double-clicked, the default ehavior is to send a "display object" signal to all
//    other attached views.  The form view which is used to efine/display objects of the specified type will respond
//    appropriately.
//
//    ARGS:       hti   -- [in] handle to the tree item that was double-clicked (possibly NULL).
//
//    RETURNS:    NONE
//
void CCxObjectTree::CustomDblClk( const HTREEITEM hti )
{
   if( hti == NULL ) return;                                      // double-click did not occur over an item label!

   CCxDoc* pDoc = GetDocument();                                  // get attached document
   WORD key = ItemToObject( hti );                                // get Maestro obj assoc w/ tree item
   WORD wType = pDoc->GetObjType(key);

   // if item dbl-clicked represents a modifiable data object, send signal to display object's data.
   if(wType != CX_CHAIR && !pDoc->IsCollectionObj(key)) 
   {
      m_hint.Set(CXVH_DSPOBJ, pDoc->GetObjType( key ), key);
      Notify( FALSE );                                            // doc is not changed!
   }
}


//=== RealizeDrag [base override] =====================================================================================
//
//    This CMultiDragTreeView virtual helper function is called when a drag operation has just finished.
//
//    In response, attempt to move or copy each of the items in the drag selection list into the destination or "drop
//    target item".  Of course, we must query the document to move/copy the object represented by each item -- ensuring
//    that the drag operation does not violate any of the organizational constraints of the object tree displayed in
//    the view.  After the operation is completed, the tree view is left in the following state:  the destination item
//    is expanded, all items added to it are selected, and the last item to be added receives the focus.
//
//    CMultiDragTreeView ensures that, if a parent item is included in the "drag list", none of its children will
//    appear in the list -- even if the user had selected both children & parent before starting the drag.
//
//    By design, the user should be able to control WHERE the drag items are moved or copied into the destination's
//    child list.  For this to be possible, the CHILDREN of the intended destination can serve as the "drop target" for
//    a drag-and-drop operation.  In such situations, the "drop target" defines the "before" insertion position for all
//    dragged items, and the child's parent item is the actual "drop target".  This complicates things, especially
//    since the drag list can be heterogeneous -- ie, it can contain different types of objects.  Take, for example,
//    dragging a target set plus some miscellaneous targets onto another target set.  The miscellaneous target objects
//    should be moved into the destination target set, but what about the dragged target set?  In a previous version of
//    Maestro, that set's targets were moved into the destination target set.  But that would prevent the user from
//    ever changing the order of target sets under the "Targets" subtree.  Therefore, we abide by the following rules:
//    (a) if the drop target is a collection that can parent the dragged item, then the dragged item is moved or copied
//    into the drop target, appended to the end of the drop target's child list; (b) otherwise, if the drop target's
//    parent can hold the dragged item, then the dragged item is moved or copied to the drop target's parent and is
//    inserted immediately before the drop target.  After the drag is realized, all items that were moved or copied are
//    highlighted in the drop target and/or the drop target's parent.  See also CanDrop().
//
//    DEV NOTES:
//    (1) CMultiDragTreeView requires that an item NOT be selected unless it is part of an expanded subtree; otherwise,
//    it will malfunction.  Therefore, if we are going to select all items added to the drop target, we must be
//    sure to expand that item before leaving this function.
//    (2) If you only want to update the "children indicator" so that a +/- button now appears in front of a previously
//    empty parent item, then invalidate the entire line containing that item's label AND be sure not to select any of
//    its children!!  Also note that manually setting the "cChildren" attribute can cause problems if not used
//    correctly; we avoid it altogether...
//    (3) Now that there is just a single Maestro object tree, it is possible to do some more powerful manipulations
//    with drag-n-drop.  For example, you could conceivably attach 10 targets to a trial if you could highlight those
//    targets and drag them onto the desired trial object.  The same could be done with the channel config assigned to
//    a trial.  Such operations are fundamentally different from what we're currently doing here.  Possible future
//    enhancement????
//
//    ARGS:       htiDragList -- [in] list of tree items that were dragged.
//                bCopy       -- [in] if TRUE, copy the dragged items; otherwise, move them.
//
//    RETURNS:    NONE
//
void CCxObjectTree::RealizeDrag( CHTIList& htiDragList, BOOL bCopy )
{
   ASSERT( m_hItemDrop );                                            // there must be a current drop target
   CTreeCtrl& cTree = GetTreeCtrl();                                 // get reference to embedded tree control
   CCxDoc* pDoc = GetDocument();                                     // pointer to the attached document
   WORD dropKey = ItemToObject( m_hItemDrop );                       // get obj key assoc with drop target item

   HTREEITEM htiParent = cTree.GetParentItem( m_hItemDrop );         // get parent of drop target item, and assoc key
   if( htiParent == NULL ) htiParent = TVI_ROOT;
   WORD dropParentKey = (htiParent == TVI_ROOT) ? pDoc->GetBaseObj() : ItemToObject( htiParent );

   HTREEITEM htiNew = NULL;                                          // handle of an added item
   HTREEITEM htiLast = NULL;                                         // the last item added
   int nChanges = 0;                                                 // >0 if at least one change was made
   BOOL bAddedToDropTgtParent = FALSE;                               // TRUE if added items to drop target's parent
   BOOL bAddedToDropTgt = FALSE;                                     // TRUE if added items to drop target itself

   while( !htiDragList.IsEmpty() )                                   // for each item in drag list, update doc as
   {                                                                 // appropriate:
      HTREEITEM htiSrc = htiDragList.RemoveHead();                   //    handle of source item
      WORD srcKey = ItemToObject( htiSrc );                          //    get obj key assoc with src item
      WORD srcType = pDoc->GetObjType( srcKey );

      if( pDoc->AcceptsSubObj( dropKey, srcType ) )                  //    if drop target can parent src object, then
      {                                                              //    append it to drop target's child list
         if( !pDoc->MoveObj(srcKey,dropKey,CX_NULLOBJ_KEY,bCopy) )   //    on failure, move to next drag item immed.
            continue;
         ++nChanges;
         bAddedToDropTgt = TRUE;
         if( !bCopy ) DeleteItemEx( htiSrc );                        //    if we moved src, delete original
      }
      else if( (pDoc->GetParentObj(srcKey) == dropParentKey) ||      //    else if drop target's parent can contain src
                pDoc->AcceptsSubObj( dropParentKey, srcType ) )      //    obj OR it already does so, then insert src
      {                                                              //    obj in parent's child list immediately before
         if( !pDoc->MoveObj(srcKey, dropParentKey, dropKey, bCopy) ) //    the drop tgt object
            continue;
         ++nChanges;
         bAddedToDropTgtParent = TRUE;
         if( !bCopy ) DeleteItemEx( htiSrc );                        //    if we moved src, delete original
      }
   }

   if( nChanges > 0 )                                                // if any changes were made, insert new entries in
   {                                                                 // tree view...
      if( bAddedToDropTgtParent )                                    // ...insert items for objs added to drop target's
      {                                                              // parent:
         POSITION pos = pDoc->GetFirstChildObj( dropParentKey );
         HTREEITEM htiAfter = NULL;                                  //    new item inserted after this tree item
         while( pos != NULL )
         {
            CTreeObj* pObj;
            WORD key;
            pDoc->GetNextChildObj( pos, key, pObj );                 //    get next child obj and its unique key
            HTREEITEM hti = ObjectToItem( key, htiParent, FALSE );   //    skip it if a corres tree item is already
            if( hti != NULL )                                        //    present -- this child was not just added!
            {
               htiAfter = hti;
               continue;
            }

            htiNew = InsertObjItem( key, pObj,                       //    insert tree item representing the obj in the
                        TVIS_SELECTED, htiParent,                    //    correct pos in drop tgt parent's child list,
                        (htiAfter==NULL) ? TVI_FIRST : htiAfter );   //    marking item as selected
            htiAfter = htiNew;                                       //    next new item should be inserted after this

            if( pDoc->GetFirstChildObj( key ) != NULL )              //    if new child has descendants, build its
               RefreshBranch( htiNew );                              //    subtree...

            htiLast = htiNew;                                        //    save handle of last item added
         }
      }

      if( bAddedToDropTgt )                                          // ...and insert items for objs added to the drop
      {                                                              // target itself (these items will have been
         POSITION pos = pDoc->GetFirstChildObj( dropKey );           // appended to end of drop target's child list):
         while( pos != NULL )
         {
            CTreeObj* pObj;
            WORD key;
            pDoc->GetNextChildObj( pos, key, pObj );                 //    get next child obj and its unique key
            HTREEITEM hti = ObjectToItem( key, m_hItemDrop, FALSE ); //    skip it if a corres tree item is already
            if( hti != NULL ) continue;                              //    present -- this child was not just added!

            htiNew = InsertObjItem( key, pObj,                       //    append tree item for the new obj to drop
                        TVIS_SELECTED, m_hItemDrop, TVI_LAST );      //    drop tgt's child list, selecting item

            if( pDoc->GetFirstChildObj( key ) != NULL )              //    if new child has descendants, build its
               RefreshBranch( htiNew );                              //    subtree...

            htiLast = htiNew;                                        //    save handle of last item added
         }
      }
   }

   if( bAddedToDropTgt && cTree.GetChildItem(m_hItemDrop) != NULL )  // if we added items to drop target, expand it if
   {                                                                 // it is not already expanded
      UINT s = GetItemState( m_hItemDrop, TVIS_EXPANDED );
      if ( (s & TVIS_EXPANDED) != TVIS_EXPANDED )
         ExpandEx( m_hItemDrop, TVE_EXPAND );
   }

   if( htiLast != NULL )                                             // set focus to last item added
   {
      HTREEITEM htiOld = GetFocusedItem();                           // but first take away both sel and focus from
      if ( htiOld != NULL )                                          // old focus item, otherwise that item will
         SetItemState( htiOld, 0, TVIS_FOCUSED|TVIS_SELECTED );      // remain selected even if it loses the focus
      FocusItem( htiLast );
   }

   if( nChanges > 0 )                                                // send view hint to inform other views of change
   {
      m_hint.Set( (bCopy) ? CXVH_NEWOBJ : CXVH_MOVOBJ, 0, CX_NULLOBJ_KEY );
      Notify();
   }
}


//=== RealizeDelete [base override] ===================================================================================
//
//    This CMultiDragTreeView virtual helper function is called when the user tries to delete selected tree items using
//    the DELETE key.
//
//    We override this method to handle the details of querying the document to remove each object and sending an
//    appropriate hint as a result of the operation.  While CMultiDragTreeView will only call this method in response
//    to the DELETE key, derived classes can also use it to respond to a menu-initiated "delete" command.  That way the
//    effect of the DELETE key will be the same as the menu-initiated "delete".  This is exactly the behavior we want
//    for CCxObjectTree -- see also OnObjectOp().
//
//    ARGS:       htiList  -- [in] list of tree items to be deleted.
//
//    RETURNS:    1  : all items were successfully removed.
//                0  : some but not all items were successfully removed.
//                -1 : no object was removed.
//
int CCxObjectTree::RealizeDelete( CHTIList& htiList )
{
   CCxDoc* pDoc = GetDocument();                                  // pointer to the attached document
   int nItems = static_cast<int>(htiList.GetCount());             // # of items selected for deletion

   BOOL bSimple = FALSE;                                          // "simple" delete -- single obj w/o descendants
   WORD saveKey = CX_NULLOBJ_KEY;                                 // saved key, type in case of simple delete
   WORD saveType = 0;
   if ( nItems == 1 )
   {
      HTREEITEM hti = htiList.GetAt( htiList.GetHeadPosition() );
      saveKey = ItemToObject( hti );
      if ( (!pDoc->IsCollectionObj( saveKey )) ||
           (pDoc->GetFirstChildObj( saveKey ) == NULL) )
      {
         bSimple = TRUE;
         saveType = pDoc->GetObjType( saveKey );
      }
   }

   BOOL bDel = FALSE;                                             // at least one item was deleted
   BOOL bNoDel = FALSE;                                           // at least one item was not deleted
   while ( !htiList.IsEmpty() )                                   // attempt to delete each item in the list:
   {
      HTREEITEM hti = htiList.RemoveHead();
      WORD key = ItemToObject( hti );                             //    key of CNTRLX object assoc w/ tree item
      int iRes = GetDocument()->RemoveObj( key );                 //    query doc to remove the obj
      if ( iRes > 0 )                                             //    success -- delete associated tree item
      {
         DeleteItemEx( hti );
         bDel = TRUE;
      }
      else if ( iRes == 0 )                                       //    partial success -- at least one descandant
      {                                                           //       removed, so refresh the tree branch.
         RefreshBranch( hti );
         bDel = bNoDel = TRUE;
      }
      else
         bNoDel = TRUE;
   }

   if ( bDel )                                                    // if at least one item deleted, send hint to other
   {                                                              // views. if a simple delete, include obj key, type.
      m_hint.Set( CXVH_DELOBJ, (bSimple ? saveType : 0),
                  (bSimple ? saveKey : CX_NULLOBJ_KEY) );
      Notify();
   }

   if ( bDel && !bNoDel )     return( 1 );
   else if ( bDel && bNoDel ) return( 0 );
   else                       return( -1 );
}


//=== CanDrop [base override] =========================================================================================
//
//    This CMultiDragTreeView virtual helper fcn allows a derived class to prevent certain tree items from serving as
//    drop targets for a drag-n-drop operation.
//
//    To allow the user to insert the dragged items at a particular location in the destination's child list, we must
//    loosen the definition of a drop target.  Thus, a drop target in CCxObjectTree is valid if it OR its parent can
//    contain AT LEAST ONE ITEM in the current drag list.  The "drag list" is the list of currently selected items in
//    the tree, which can easily contain items referring to many different types of Maestro objects.
//
//    ARGS:       hti   -- [in] handle of tree item.
//
//    RETURNS:    TRUE if operation is permitted on specified item, FALSE otherwise.
//
BOOL CCxObjectTree::CanDrop( HTREEITEM hti )
{
   CHTIList dragList;                                                      // get the current list of dragged items
   GetSelectedList( dragList );

   CCxDoc* pDoc = GetDocument();                                           // pointer to the attached document

   WORD dropKey = ItemToObject( hti );                                     // obj key assoc. w/ putative drop target
   if( !pDoc->IsCollectionObj( dropKey ) )                                 // if drop tgt not a collection obj, then
      dropKey = CX_NULLOBJ_KEY;                                            //    we only need to check its parent

   WORD dropParentKey = CX_NULLOBJ_KEY;                                    // obj key assoc. w/ its parent, if any
   HTREEITEM htiParent = GetTreeCtrl().GetParentItem( hti );
   if( htiParent != NULL ) dropParentKey = ItemToObject( htiParent );
   else dropParentKey = pDoc->GetBaseObj();                                // the Maestro object tree root

   while( !dragList.IsEmpty() )                                            // if we find one item in the drag list
   {                                                                       // that can be contained by the drop tgt
      HTREEITEM hti = dragList.RemoveHead();                               // or its parent, then the drop tgt is valid
      WORD key = ItemToObject( hti );
      WORD type = pDoc->GetObjType( key );

      if( (dropKey!=CX_NULLOBJ_KEY) && pDoc->AcceptsSubObj(dropKey, type) )
         return( TRUE );
      if( (pDoc->GetParentObj( key ) == dropParentKey) ||                  // in case of drop tgt parent, it's valid
          pDoc->AcceptsSubObj( dropParentKey, type ) )                     // also if parent already holds drag item!
         return( TRUE );
   }

   return( FALSE );
}


//=== AcceptNewLabel [base override] ==================================================================================
//
//    This CMultiDragTreeView overridable is called after the user completes an in-place edit in order to validate the
//    new label.  It is called *before* the item's name is actually changed.
//
//    We override this function to coordinate any changes in the name of a Maestro object.  We must query the document
//    to accept/reject the new name.  If accepted, we must then send an appropriate "object renamed" signal to all
//    other attached views.
//
//    ARGS:       hti   -- [in] tree item handle.
//                str   -- [in] proposed new label for the item.
//
//    RETURNS:    TRUE to accept the new label, FALSE to reject it.
//
BOOL CCxObjectTree::AcceptNewLabel( HTREEITEM hti, CString& str )
{
   CCxDoc* pDoc = GetDocument();                                              // ptr to the attached document
   WORD objKey = ItemToObject( hti );                                         // ID of Maestro obj assoc w/ tree item
   if( pDoc->SetObjName( objKey, str ) )                                      // attempt rename; if successful, send
   {                                                                          // "rename" hint to other views...
      m_hint.Set( CXVH_NAMOBJ, pDoc->GetObjType( objKey ), objKey );
      Notify();
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== GetExpandBitmaps [base override] ================================================================================
//
//    This CMultiDragTreeView overridable is called immediately after a parent item is expanded or collapsed.
//
//    We override this fcn to associate distinct bitmaps with the expanded vs collapsed state of items representing
//    Maestro "collection" objects -- objects which can "parent" other objects.  The function should never be called
//    on items representing non-collection objects.  If no image list is installed, both bitmap pos are set to -1.
//
//    ARGS:       hti      -- [in] tree item handle.
//                bExpand  -- [in] TRUE if item was just expanded, FALSE if it was just collapsed.
//                piImg    -- [out] pos of "unselected" bitmap in image list.
//                piSelImg -- [out] pos of "selected" bitmap in image list.
//
//    RETURNS:    NONE
//
void CCxObjectTree::GetExpandBitmaps( HTREEITEM hti, BOOL bExpand, int* piImg, int* piSelImg )
{
   if ( GetTreeCtrl().GetImageList( TVSIL_NORMAL ) != NULL )
   {
      WORD objKey = ItemToObject( hti );
      ASSERT( GetDocument()->IsCollectionObj( objKey ) );
      GetBitmapIDs( GetDocument()->GetObjType( objKey ), bExpand, piImg, piSelImg );
   }
   else
   {
      *piImg = -1;
      *piSelImg = -1;
   }
}


//=== GetContextMenu [base override] ==================================================================================
//
//    This CMultiDragTreeView override is called in the event of a context-menu invocation in order to load an approp.
//    short-cut menu.
//
//    In the current Maestro GUI design, in the main frame menu IDR_MAINFRAME, the "Object" submenu is used to perform
//    operations on the CNTRLX object tree via CCxObjectTree.  The zero-based index of the "Object" submenu is
//    CCxObjectTree::ID_OBJ_SUBMENU.
//
//    ARGS:       hti   -- [in] the applicable tree item at the time of context-menu invocation (possibly NULL).
//                m     -- [in] an empty menu object;
//                      -- [out] the loaded context menu, or another menu containing it as a submenu.
//                iSub  -- [out] if >= 0, indicates position of submenu that represents the context menu to display;
//                         else the menu itself will be taken as the context menu.
//
//    RETURNS:    TRUE if a context menu was loaded; FALSE otherwise.
//
BOOL CCxObjectTree::GetContextMenu( HTREEITEM hti, CMenu& m, int& iSub )
{
   ASSERT( m.GetSafeHmenu() == NULL );                // provided menu object should be empty initially!

   iSub = -1;
   if ( m.LoadMenu( IDR_MAINFRAME ) )
   {
      iSub = CCxObjectTree::ID_OBJ_SUBMENU;
      return( TRUE );
   }
   else
      return( FALSE );
}



