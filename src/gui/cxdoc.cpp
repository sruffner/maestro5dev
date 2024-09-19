//=====================================================================================================================
//
// cxdoc.cpp : Implementation of class CCxDoc, which encapsulates the one-and-only Maestro document class.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// ==> Predefined target CX_OKNDRUM no longer supported as of Maestro v1.5.0.
// The OKNDRUM was never used in Maestro, and we decided to remove it for version 1.5.0.  However, because of the way
// in which experiment documents are serialized, all documents existing prior to v1.5.0 include a CCxTarget object that
// represents the OKNDRUM.  CCxDoc::Serialize() takes care of removing it from the object tree map when such documents
// are deserialized, but that only happens AFTER the CCxTarget object has been constructed and placed in the object
// tree map. So, the underlying framework still supports creation of the OKNDRUM CCxTarget instance.  Otherwise,
// deserialization would always throw an exception for pre-1.5.0 (CCxDoc::CURRVERSION < 2) documents!!
//
// ==> RMVideo server replaces VSG2/4-based framebuffer card as of Maestro v2.0.
// RMVideo is an OpenGL application that runs on a separate Linux workstation and communicates with Maestro over a
// private, dedicated Ethernet link.  It offers much better performance over the antiquated VSG2/4 graphics card,
// including the potential to work with any number of off-the-shelf graphics cards (as long as the vendor supports
// Linux!).  In Maestro v2.0, RMVideo supplanted the VSG2/4 as our framebuffer video display platform.  CCxTarget
// takes care of converting the old CX_FBTARG targets to CX_RMVTARG targets during deserialization.  However, we
// changed the version number on CCxDoc from 2 to 3 to mark this major change in Maestro.
//
// ==> Trial "subsets" introduced in Maestro v3.1.2.
// A trial subset is simply a grouping of related trials within a trial set. A trial set can now parent two distinct
// object types: CX_TRIAL and CX_TRIALSUBSET. A subset must contain at least one trial, but it cannot contain any
// trial subsets -- there's only one level of subsets! The user will be able to specify trial sequencing on two
// levels in Trial mode: the sequencing of subsets within a trial set, and the sequencing of trials within each subset.
// The available sequence modes for subsets are "none", "ordered", or "randomized"; when "none" is chosen, then Maestro
// simply collects all the trials across all subsets and treats them as a single set of trials.
//
// REVISION HISTORY:
//
// 04feb2000-- Created.  Just a skeleton for now.  Setting up a standard format for .H, .CPP files...
// 31mar2000-- Updated IAW recent changes to CCxTarget.  New strategy regarding the interfaces among the target-related
//             view CCxTargForm, CExperimentDoc, and a particular target object CCxTarget:  The view can now obtain
//             a pointer to any CCxTarget object by calling CExperimentDoc::GetTarget().  Similarly, a pointer to a
//             newly created target is returned by CExperimentDoc::AddTarget().  The view can then communicate
//             directly with the target to obtain access to the target's properties.  This is not the best arrangement,
//             since we need to ensure that all target names in a list are valid & unique, which is the responsibility
//             of CExperimentDoc.  Work on this later....
//          -- CExperimentDoc::AddTarget() modified to either copy an existing tgt or make a new one.  CopyTarget()
//             deleted.
// 13apr2000-- COMPLETE REWORK given new design strategy:  Since a CNTRLX experiment is going to be a relatively
//             complex document, I've decided to partition its work among a number of "manager" objects.  The majority
//             of document data is hidden within these managers, which are created as public members of CExperimentDoc
//             so that attached views have direct access to them.  For now, there is only one such class, CCxTgManager,
//             which manages the experiment doc's current target list.  Attached views which handle target information
//             deal directly with the target manager.
// 02jun2000-- As I begin to work on the user interface for CNTRLX trial objects, I'm reconsidering the use of the
//             "manager" objects.  In the case of target objects, all of the target-specific stuff is already handled
//             by the target object (CCxTarget) rather than the target manager.  I plan to handle all other CNTRLX
//             experiment objects (trials, channel sets, continuous stims, display options, etc.) in a similar fashion.
//             So why bother with the overhead (albeit small) of a manager object for each object type?  Instead,
//             declare a private collection object for each object type in CExperimentDoc, and provide function calls
//             for dealing with each collection.  A lot of these calls would be similar for different collections,
//             so several strategies are suggested:  (1) function overloading [Add( LPCTSTR name, CCxTarget* ),
//             Add( LPCTSTR name, CCxTrial* ), ...], (2) one function handling all object types [ Add( LPCTSTR name,
//             void*, UINT objType) ], or (3) separate function calls for each collection type [AddTrial(),
//             AddTrialSet(), etc.].  I'll try things out with trial names using the third strategy.  Eventually,
//             may get rid of the "target manager"....
//          -- Trials are grouped into trial sets, which adds a layer of complexity.  Currently, I'm trying out the
//             following construct.  The entire collection is represented by a typed-pointer map which maps strings to
//             CCxTrialMap* pointers.  These, in turn, are typed-pointer maps from a string to a CCxTrial* pointer.
//             The string keys are the name of the trial set and trial, respectively.  One implication of this choice
//             is that we cannot use a callback mechanism to provide a trial item's name to the view.  Memory is
//             wasted.  This could become significant if there are a lot of views using various CNTRLX object names.
//             Address that issue later....
// 05jun2000-- The CTypedPtrMap of CTypedPtrMap's construct (02jun2000) worked.  Need to test serialization....
// 22jun2000-- Comment:  Having separate MoveTrial and CopyTrial functions is inefficient.  Similarly with
//             MoveTrialSet, CopyTrialSet.
// 11jul2000-- Modified MoveTrialSet to return a CStringArray with the names of trials copied into the destination
//             trial set.
// 12jul2000-- Began work on the concept of a "CNTRLX object tree", embodied by objects of class CCxObject, which are
//             stored in a map collection CCxObjTree that is keyed by a unique WORD assigned to each CNTRLX object that
//             is stored in the document.  Will be testing out approach with just the trial tree for now.  THIS IS A
//             COMPLETE REWORK ON HOW WE DEAL WITH TRIAL OBJECTS.  ULTIMATELY WILL APPLY TO ALL CNTRLX OBJECTS, INCL.
//             TARGETS....
// 13jul2000-- Would like to customize the map collection so that time spent searching for a keyed object is
//             minimized.  FOR NOW:  Use CTypedPtrMap< CMapWordToOb ... >.  Keep track of the last key issued.  Incr
//             it by 16 (the CMapWordToOb hash algorithm divides the key by 16, then %'s the result by the hash table
//             size to determine which linked list in the hash table to search) each time an object is inserted.  Once
//             we reach 65520, wrap back around to 1, etc.  ASSERT when we reach 65535.  This is temporary, so we can
//             focus on developing and testing InsertObj(), etc.
//          -- Instead of having a single global object tree, I've decided to create a tree for each object type:  so
//             there's a trial root, target root, etc.  Views must use GetRootObject() to obtain the key of a root
//             object.  These root keys are stored as private members of the document...
// 19jul2000-- The CNTRLX object tree-map design with CCxObject is working.  Now working on refinements...
//          -- Provide fcn GetObjType() to obtain the object's type when necessary.  Views need not store the object
//             type, only the object's key.  How they store it is their business.
//          -- Replaced GetRootObject() with GetTrialRoot(), etc.  Views should have no knowledge of the root object
//             types CX_TRIALROOT, etc.
// 30jul2000-- REWORKED the CNTRLX object tree-map design AGAIN.  Now, CCxTreeMap is responsible for maintaining the
//             self-keyed tree-map and for maintaining valid & unique CNTRLX object names.  It also handles all the
//             details of creating, copying, and destroying the actual data associated with CNTRLX data objects like
//             targets, trials, etc.  CExperimentDoc is aware only of the CNTRLX object types.  It controls what kinds
//             of object trees are available (target tree, trial tree, etc.) and restrains their contents as the user
//             manipulates the tree via the views.  [For example, CCxTreeMap will allow ANY kind of object to be
//             inserted as a child of any other kind of object.  CExperimentDoc, and to a certain extent the CNTRLX
//             views, restrict insertions to those which make sense...].
//          -- Reworking CExperimentDoc to use CCxTreeMap -- only with the trial tree for now.  Will integrate the
//             "target tree" later...
// 03aug2000-- Finished reworking methods to use CCxTreeMap for the trial tree.  Modified prototypes of the iteration
//             routines GetFirstChildObj() and GetNextChildObj() [formerly, GetNextSiblingObj()].
//          -- Modified CExperimentDoc to use CCxTreeMap to manage the "target tree".  CCxTgManager no longer in use!
// 04aug2000-- There are now two mechanisms for traversing objects in the CNTRLX object tree-map.  The existing
//             GetFirst/NextChildObj() methods traverse the *immediate* children of an object.  The new methods
//             InitTraverseObj() and TraverseObj() are used to traverse the CNTRLX object tree, from a specified start
//             point, in a standard order recursively defined as:  "visit" node A, visit each of node A's children in
//             standard order, then move on to node A's next sibling.  All traversal routines are now inline functions,
//             encapsulating methods provided by CCxTreeMap.
//          -- Updated IAW technical changes to CCxTreeMap methods GetNode() and GetNextChild().
// 09aug2000-- Recursive RemoveObj() method replaced with non-recursive version.  Also changed return value to an int
//             indicating one of three possible outcomes.
//          -- Added helper function IsValidMove() which enforces the rules governing how CNTRLX objects may be moved
//             or copied within the CNTRLX object trees.
//          -- MoveObj() rewritten to make use of IsValidMove().  The code is now much simpler!
//          -- Made a number of simple data access methods (GetObjType(), etc.) inline -- defined in class declaration
//             in .H file.
//          -- Instead of returning a LPCTSTR, GetObjName() is now defined with a CString& argument to hold the object
//             name.  The problem with returning a LPCTSTR is that string it references must not go out of scope when
//             the method returns.  We circumvent that by using the CString& reference argument:  now the name gets
//             copied to the CString provided by the caller.  Analogous change made to GetObjData().
// 11aug2000-- Modified InitObjTrees() so that all predefined CNTRLX objects are hard-coded there -- e.g., no longer
//             use IDS_CXHARDTARGS resource to get names of the predefined targets.
// 15aug2000-- Added inline utilities GetParentObj() and ObjExists().
// 16aug2000-- Added utilities IsValidType() and IsCollectionObj().  Modified ValidChildType().
// 17aug2000-- All *Obj() methods which can modify the document now call SetModifiedFlag() if the doc is changed.
//             Views which change the document via these methods need not call SetModifiedFlag().  However, views
//             which modify a CNTRLX object DIRECTLY must do so (CNTRLX objects have no knowledge of the document to
//             which they belong).
// 14sep2000-- ANOTHER REWORK.  Still using CCxTreeMap, but some major changes have been made:  CCxTreeMap and the
//             CNTRLX data classes are built upon the reusable CTreeMap/CTreeObj framework (TREEMAP.CPP).  All object
//             pointers are now CTreeObj* rather than CObject*.  All CNTRLX data classes -- such as CCxTarget, etc. --
//             are derived from CTreeObj, which is, in turn, derived from CObject.  CTreeObj itself provides storage
//             for three items common to all objects (data or collection) stored in a tree hierarchy:  object name, a
//             data type identifier, and a general-purpose var intended to hold object state info.  Thus, the object
//             itself has access to its own name, data type, and state flags.  Also, the flags are no longer stored in
//             the high byte of the WORD-valued data type.  A unique WORD-valued key is still used to access an
//             object in the tree map, and CCxTreeMap is still self-keying.  However, the above changes required
//             numerous changes in the CCxTreeMap interface, so CExperimentDoc methods must be changed accordingly...
//          -- NOTE:  Now that object name, type & flags are stored in the object itself, methods like GetObjName(),
//             GetObjType(), IsUserObj(), etc. are not strictly necessary.  Views can use GetObject() to obtain a ptr
//             to the object itself and then call the appropriate methods CTreeObj::Name(), etc.  However, we have kept
//             these methods for convenient "one-call" access...
// 19oct2000-- Added UpdateObjDep() and IsLockedObj() to support object dependencies via the CTreeMap framework...
// 26jan2001-- Another change in the "big picture".  Instead of storing the CNTRLX target tree, trial tree, etc. as
//             separate trees in CCxTreeMap, they are now subtrees of a single CNTRLX "object tree".  InitObjTree()
//             [was InitObjTrees()] builds the predefined state of this tree.  There is no need for m_trialRoot,
//             m_targRoot, etc. -- we can always find these quickly by traversing the immediate children of the root
//             object, which we do save in m_objTreeRoot.  This design change is reflected on the view side:  the
//             separate tree views for different data classes (CCxTargTree, CCxTrialTree) have been replaced by the
//             CCxObjectTree class -- a single tree view by which the user modifies the contents of the object tree.
//          -- CCxObjectTree still needs to be able to retrieve the keys of the major subtrees, as well as the object
//             tree's root.  GetBaseObj(), which replaces GetRootObj(), serves this role.  Without a specified arg, it
//             retrieves the object tree root.  If the type of one of the subtrees is specified, then that object key
//             is returned.
// 27sep2001-- Cosmetic:  CExperimentDoc --> CCxDoc, and cxexpdoc.* --> cxdoc.*.
// 06feb2002-- Added new object type CX_VIDEODSP, encapsulating current video (XY & FB) display parameters.  There's
//             only one per CCxDoc, and it is located at the root of the object tree.  The display object is predefined
//             but modifiable (you can change the display parameters, but you cannot remove it, rename it, etc.).
// 22may2002-- Added new data object type CX_CONTRUN, representing "run" objects used in continuous mode.  Analogous to
//             trials, runs are organized in "run sets" (CX_CONTRUNSET) under the predefined CX_CONTRUNBASE node.
// 17oct2002-- Eliminated object type CX_VIDEODSP.  Instead, developed a CCxSettings object that contains all CNTRLX
//             "application-level" settings that will persist in the document object.  These settings include, so far,
//             the video display parameters, fixation requirements for ContMode, and various reward options.  There is
//             one and only one CCxSettings object per experiment doc, and it is NOT part of the object tree!
// 17nov2002-- Added new data object type CX_PERTURB, representing a "perturbation waveform" that can be applied to
//             trial target trajectories.  All perturbation objects are stored in the predefined CX_PERTBASE subtree.
//          -- Introduced version schema at version 1.  Previous version of CCxDoc had not CX_PERTBASE subtree, so we
//             must create it after deserialization.  IMPLEMENT_DYNCREATE() macro replaced by IMPLEMENT_SERIAL().
// 27nov2002-- Added helper methods PrepareKeyChain() and TreeInfoCB() for somewhat generic traversal of the CNTRLX
//             object tree.
// 10dec2002-- The MFC VERSIONABLE_SCHEMA scheme (see 17nov2002) does not work with CCxDoc, because the MFC framework
//             deserializes document by calling Serialize() directly, not indirectly via CArchive::ReadObject().  As a
//             result, CArchive::GetObjectSchema() always returned -1.  FIX:  serialize our own version #...
// 31dec2002-- Modified InsertObj() that takes an optional third argument specifying a suggested name for the new obj.
//             Existing usage of InsertObj() without the third argument is unaffected!
//          -- Added methods GetPredefinedTargets() and GetDefaultChannelConfig().
// 20oct2003-- Recently modified the object tree to eliminate alphabetical sorting of siblings and to append new
//             objects so that views reflect the actual order of siblings under a parent node.  This is particularly
//             important for the ordered sequencing of trials and stimulus runs.  Also modified the underlying
//             CTreeMap to support inserting new and existing nodes at particular locations in the destination's child
//             list (by specifying the sibling before which the insertion occurs).  Modified InsertObj(), MoveObj(),
//             and DuplicateObj() so that Maestro's object tree view can allow the user to insert new nodes or drag
//             existing nodes to a particular location under a destination node.  This will give the user complete
//             control over the order of trials in a trial set, runs in a run set, etc.  IsValidMove() was also
//             changed to permit an object to be moved around under its current parent.
// 16dec2003-- Fixed bug in DuplicateObj() that was introduced by changes dtd 20oct2003.
// 15mar2005-- Added CopySelectedObjectsFromDocument() to support copying selected objects from one experiment doc to
//             another.
// 14mar2006-- Removed support for CX_OKNDRUM target as of Maestro v1.5.0.  Document version changed from 1 to 2.  When
//             deserializing docs with version<2, we remove the object node representing the OKNDRUM target AFTER
//             deserialization is finished.  If the document has dependencies on OKNDRUM -- which it should not b/c no
//             one has used OKNDRUM in Maestro! -- then deserialization fails with an exception thrown.
// 24mar2006-- Minor changes to support introduction of RMVideo targets, which replace the old VSG framebuffer targets.
//             While no serialization changes in CCxDoc itself, we incremented the document version to 3 to mark this
//             major change in Maestro.
// 21sep2011-- Removed support for CX_FIBER* and CX_REDLED* targets as of Maestro 3.0. Document version incr to 4. 
// After deserializing an older document, it is migrated to version 4. See MigrateToVersion4() for details.
// 01dec2014-- Modified to support the notion of "trial subsets". Document version incr to 5. Versions prior to this
// lack a trial subset object, so there's no migration work to do.
// 02dec2014-- Added convenience function GetTrialKeysIn().
// 19sep2018-- Mods for Maestro 4: Added some additional RMVideo display settings related to a "time sync flash" in the
// display's TL corner. Also modified CCxTrial to include a per-segment flag that enables the time sync flash at the
// start of the segment. Document version incr to 6, mainly to mark release of Maestro 4 for Win10 64-bit. No migration
// changes needed.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include <ctype.h>
#include "cntrlx.h"                          // application-wide defines; resource defines

#include "cxviewhint.h"                      // CCxViewHint -- the CNTRLX view hint
#include "cxtrial.h"                         // CCxTrial -- a trial object
#include "cxcontrun.h"                       // CCxContRun -- a stimulus run object
#include "cxdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL(CCxDoc, CObject, 1)

//=====================================================================================================================
// PRIVATE CONSTANTS/TYPEDEFS/GLOBALS/MACROS
//=====================================================================================================================



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

CCxDoc::CCxDoc()
{
   m_objTreeRoot = CX_NULLOBJ_KEY;
}


CCxDoc::~CCxDoc()
{
   DestroyObjTree();
}



//=====================================================================================================================
// GENERAL DOCUMENT OPERATIONS
//=====================================================================================================================

//=== OnNewDocument [base override] ===================================================================================
//
//    Perform any "per-instance" initializations of the CNTRLX experiment document.
//
//    The doc/view framework calls this method whenever the File|New command in selected.  In the SDI model, the
//    document object is REUSED.  Therefore, it is important that we put the initialization code for the document here
//    rather than the constructor.  We currently employ the SDI model for CNTRLX.
//
//    ARGS:       NONE
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL CCxDoc::OnNewDocument()
{
   if( !CDocument::OnNewDocument() ) return( FALSE );    // base class inits first -- empties SDI document
   InitObjTree();                                        // build initial, predefined state of the CNTRLX object tree
   m_settings.RestoreDefaults();                         // restore all persistent settings to default values
   return( TRUE );
}


//=== DeleteContents [base override] ==================================================================================
//
//    Empty the document completely.
//
//    Thus far, all document data is stored in the CNTRLX object tree.  By destroying the tree, we empty the doc!
//
//    NOTE:  In the MDI model, a new document object is created each time -- in which case this fcn is of little use.
//    However, we use the SDI model, which reuses the same document object.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxDoc::DeleteContents()
{
   DestroyObjTree();
}


//=== Serialize [base override] =======================================================================================
//
//    Serialize document data through specified archive, including version control.
//
//       1:  Base version.
//       2:  Dropped support for CX_OKNDRUM target.  Corresponding CCxTarget node is removed from object tree map
//           AFTER deserialization of version<2 documents.  As of Maestro version 1.5.0.
//       3:  RMVideo replaces VSG2/4 as the framebuffer video display platform.  No substantive changes in CCxDoc.
//       4:  Dropped support for CX_FIBER* and CX_REDLED* targets. After deserialization of a version==3 doc, all
//           trials and stimulus runs employing any of these targets are removed, then the predefined CX_CHAIR
//           target is moved under CX_TARGBASE, and the "Predefined" target set is deleted. As of Maestro 3.0.
//       5:  Introduced notion of a trial subset, object type CX_TRIALSUBSET. No change to v4 doc, except for the
//           serialized version number.
//       6:  Marks initial release of Maestro 4 for Win10 64-bit. Some changes to CCxSettings and CCxTrial to 
//           implement a "time sync flash" in the top-left corner of the RMVideo display, but no changes to the
//           CCxDoc itself -- so no migration needed.
//
//    ARGS:       ar -- the serialization archive.
//
//    RETURNS:    NONE
//
//    THROWS:     -- The CArchive object can throw CMemoryException, CArchiveException, or CFileException.
//                -- We throw CArchiveException if we cannot convert an older version to the current version.
//
void CCxDoc::Serialize( CArchive& ar )
{
   CDocument::Serialize( ar );                                    // base class stuff first

   WORD wVersion = 0;
   if( ar.IsStoring() )                                           // doc version # and key assigned to tree root
   {
      ar << (WORD) CCxDoc::CURRVERSION;
      ar << m_objTreeRoot;
   }
   else
   {
      ar >> wVersion;
      if(wVersion < 1 || wVersion > CCxDoc::CURRVERSION) 
         ::AfxThrowArchiveException(CArchiveException::genericException);
      ar >> m_objTreeRoot;
   }
   m_settings.Serialize( ar );                                    // application settings
   m_Objects.Serialize(ar);                                       // MAESTRO object tree and all objects therein

   // remove the predefined CX_OKNDRUM target -- no longer supported!  Since the OKNDRUM was never used in Maestro,
   // there should be no dependencies upon it.  If there are, we throw an archive exception and post an error message.
   if( !ar.IsStoring() && (wVersion < 2) )
   {
      WORD wOknKey = GetOKNDrumTarget();
      if( wOknKey != CX_NULLOBJ_KEY )
      {
         if( m_Objects.IsLocked(wOknKey) )
         {
            ((CCntrlxApp*)AfxGetApp())->LogMessage("Doc contains locks on obsolete OKNDRUM target; cannot migrate!");
            ::AfxThrowArchiveException( CArchiveException::genericException );
         }
         m_Objects.RemoveTree(wOknKey, FALSE);
      }
   }

   // migrate from older version to version 4: CX_FIBER* and CX_REDLED* targets no longer supported. If migration fails,
   // post a message and throw an exception
   if(!ar.IsStoring() && (wVersion < 4))
   {
      if(!MigrateToVersion4())
      {
         ((CCntrlxApp*)AfxGetApp())->LogMessage("Unable to migrate Maestro 2.x document to Maestro 3!");
         ::AfxThrowArchiveException(CArchiveException::genericException);
      }
   }
   ASSERT_VALID( this );
}



//=====================================================================================================================
// OPERATIONS ON CNTRLX OBJECT TREES
//=====================================================================================================================

//=== GetBaseObj ======================================================================================================
//
//    Retrieve the unique key assigned to the root node of the Maestro object tree, or one of its predefined subtrees
//    (CX_TRIALBASE, CX_TARGBASE, etc) -- which are one of the immediate children of the root node.
//
//    ARGS:       type  -- [in] if 0, retrieve root node; otherwise, this specifies the type of subtree to retrieve.
//
//    RETURNS:    unique key of the specified base node, or CX_NULLOBJ_KEY if not found (invalid type, not allocated).
//
WORD CCxDoc::GetBaseObj( const WORD type /* = 0 */ ) const
{
   WORD baseKey = CX_NULLOBJ_KEY;
   if( type == 0 )                                             // retrieve root of CNTRLX object tree...
      baseKey = m_objTreeRoot;
   else                                                        // ...or traverse its immediate children for one of the
   {                                                           // predefined subtree types...
      POSITION pos = GetFirstChildObj( m_objTreeRoot );
      while( pos != NULL )
      {
         WORD kChild;
         CTreeObj* pChild;
         GetNextChildObj( pos, kChild, pChild );
         if( pChild->DataType() == type )
         {
            baseKey = kChild;
            break;
         }
      }
   }

   return( baseKey );
}

/**
 Retrieve the keys identifying all trials in the specified trial set or subset. When the specified parent object is a 
 trial subset (CX_TRIALSUBSET type), the method returns the keys of all trials within that subset. If the parent is a
 trial set (CX_TRIALSET type), the method returns the keys of all trials in that set, including trials listed under any
 trial subsets.

 @param wParent Object key identifying a trial set or subset object.
 @param wArKeys This array will be emptied initially, then populated with the trial keys, in the order in which the 
 trials appear within the specified parent. If the parent does not exist, is not a trial collection, or is empty, then
 this array will be empty.
*/
VOID CCxDoc::GetTrialKeysIn(const WORD wParent, CWordArray& wArKeys) const
{
   // clear the trial key array initially
   wArKeys.SetSize(0, 20);

   // nothing to do if parent does not exist or is neither a trial set or subset.
   if(!ObjExists(wParent)) return;
   WORD type = GetObjType(wParent);
   if(type != CX_TRIALSET && type != CX_TRIALSUBSET) return;

   // traverse all descendants of the set or subset and collect the keys of any trials encountered. By design, a trial
   // set can contain trials and subset, while a trial subset can only contain trials
   POSITION pos = GetFirstChildObj(wParent);
   WORD wKid;
   CTreeObj* pObj;
   while(pos != NULL)
   {
      GetNextChildObj(pos, wKid, pObj);
      type = GetObjType(wKid);

      if(type == CX_TRIAL)
         wArKeys.Add(wKid);
      else if(type == CX_TRIALSUBSET)
      {
         // traverse all the trials in the subset, which can only contain trial objects
         POSITION pos2 = GetFirstChildObj(wKid);
         WORD wKid2;
         while(pos2 != NULL)
         {
            GetNextChildObj(pos2, wKid2, pObj);
            if(GetObjType(wKid2) == CX_TRIAL) wArKeys.Add(wKid2);
         }
      }
   }
}

/**
 Check the children of the specified trial subset collection to see if it contains any non-empty trial subsets.
 @param wSet Unique key identifying a trial set.
 @return True if specified trial set contains at least one subset with one or more trial objects; else false. Also 
 returns false if the specified key does not identify a trial set.
*/
BOOL CCxDoc::HasTrialSubsets(const WORD wSet) const
{
   if(!(ObjExists(wSet) && (GetObjType(wSet) == CX_TRIALSET))) return(FALSE);
   
   BOOL hasSubset = FALSE;
   POSITION pos = GetFirstChildObj(wSet);
   WORD wKid;
   CTreeObj* pObj;
   while(pos != NULL && !hasSubset)
   {
      GetNextChildObj(pos, wKid, pObj);
      hasSubset = (GetObjType(wKid) == CX_TRIALSUBSET) && (GetFirstChildObj(wKid) != NULL);
   }

   return(hasSubset);
}

/** GetChairTarget, GetDefaultChannelConfig
 Retrieve the unique key assigned to the predefined target object representing the animal chair, or the predefined 
 "default" channel configuration object.
 
 These two objects always exist in the Maestro experiment object tree. We search for the keys rather than remembering 
 them in member variables -- to avoid having to serialize the remembered keys.

 @return Unique key locating the predefined experiment object requested.
*/
WORD CCxDoc::GetChairTarget() const
{
   WORD wKey = CX_NULLOBJ_KEY;
   POSITION pos = GetFirstChildObj(GetBaseObj(CX_TARGBASE));
   while(pos != NULL)
   {
      WORD kChild;
      CTreeObj* pChild;
      GetNextChildObj(pos, kChild, pChild);
      if(pChild->DataType() == CX_CHAIR)
      {
         wKey = kChild;
         break;
      }
   }
   ASSERT(wKey != CX_NULLOBJ_KEY);
   return(wKey);
}

WORD CCxDoc::GetDefaultChannelConfig() const
{
   WORD wKey = CX_NULLOBJ_KEY;
   POSITION pos = GetFirstChildObj(GetBaseObj(CX_CHANBASE));
   while(pos != NULL)
   {
      WORD kChild;
      CTreeObj* pChild;
      GetNextChildObj(pos, kChild, pChild);
      if(pChild->DataType() == CX_CHANCFG && (pChild->Flags() & CX_ISPREDEF))
      {
         wKey = kChild;
         break;
      }
   }
   ASSERT(wKey != CX_NULLOBJ_KEY);
   return(wKey);
}

/**
 Prior to Maestro v1.5, there were 6 non-modifiable targets stored under an unmodifiable "Predefined" target
 set object. One of these was the "OKNDRUM" target. Support for that target was dropped in v1.5 (doc schema
 version 2). This method simply searches the object tree for the key of the obsolete OKNDRUM target. It is
 called only when opening an old document (schema version < 2). If invoked on a current experiment doc, it
 will always return a null key!

 @return If this is a pre-version 2 document, return the key of the predefined OKNDRUM target. Else, return
 CX_NULLOBJ_KEY.
*/
WORD CCxDoc::GetOKNDrumTarget() const
{
   // first, find the key of the old "Predefined" target set
   WORD wSet = CX_NULLOBJ_KEY; 
   POSITION pos = GetFirstChildObj(GetBaseObj(CX_TARGBASE));
   while(pos != NULL)
   {
      WORD kChild;
      CTreeObj* pChild;
      GetNextChildObj(pos, kChild, pChild);
      if(pChild->DataType() == CX_TARGSET && (pChild->Flags() & CX_ISPREDEF)
          && (::strcmp(pChild->Name(), _T("Predefined")) == 0))
      {
         wSet = kChild;
         break;
      }
   }

   // now find key of the obsolete OKNDRUM target
   if(wSet != CX_NULLOBJ_KEY)
   {
      pos = GetFirstChildObj(wSet);
      while(pos != NULL)
      {
         WORD kChild;
         CTreeObj* pChild;
         GetNextChildObj( pos, kChild, pChild );
         if(pChild->DataType() == CX_OKNDRUM) return(kChild);
      }
   }

   return(CX_NULLOBJ_KEY);
}


//=== InsertObj =======================================================================================================
//
//    Insert a new user-defined Maestro object under an existing parent (collection) object in the object-tree map.
//    The type of Maestro object that can be inserted is restricted by the helper function ValidChildType(), which
//    enforces rules on the content and structure of the Maestro object tree.  Another helper function,
//    GetObjBasename(), "suggests" a name for the new object based upon its type (if no name is provided).
//
//    If the key of one of the parent's existing children is specified, then the new object will be inserted before
//    this child in the parent's ordered child list; otherwise, the new object is appended to the list.
//
//    When a new trial is inserted, it will initially have the predefined channel configuration assigned to it.  That
//    channel configuration object ALWAYS exists.
//
//    This function cannot be used to insert new *root* objects into the Maestro object-tree map.
//
//    ARGS:       key   -- [in] parent's unique map key (it MUST exist).
//                type  -- [in] type of object to be inserted.
//                name  -- [in] suggested name for object (autocorrected to ensure uniqueness & remove invalid chars);
//                              if NULL, a name is provided IAW obj data type (default=NULL).
//                bef   -- [in] key of sibling object before which the new object should be inserted; if CX_NULLOBJ_KEY
//                         the new object is appended to end of parent's child list (default=CX_NULLOBJ_KEY).
//
//    RETURNS:    unique key of new object if successful; CX_NULLOBJ_KEY otherwise (illegal insertion or lack of mem).
//
WORD CCxDoc::InsertObj( const WORD key, const WORD type,
   LPCTSTR name /* =NULL */, const WORD bef /* =CX_NULLOBJ_KEY */ )
{
   CTreeObj *pObj;                                                            // get info on parent object...
   VERIFY( m_Objects.GetNode( key, pObj ) );                                  // ...which MUST exist!

   if( !ValidChildType( pObj->DataType(), type ) )                            // can parent accept object of this type?
      return( CX_NULLOBJ_KEY );

   CString str = (name==NULL) ? GetObjBasename( type ) : name;                // attempt to create & insert new obj
   WORD newKey = m_Objects.InsertNode( key, type, 0, str, bef );              // with the suggested name

   if( newKey == CX_NULLOBJ_KEY )                                             // inform user if insertion failed
      ::AfxMessageBox( "Maestro object space full, or insufficient memory" );
   else
   {
      SetModifiedFlag();
      if( type == CX_TRIAL )                                                  // when a new trial is created, init its
      {                                                                       // channel config to the predefined
         CCxTrial* pTrial = (CCxTrial*) GetObject(newKey);                    // default channel config
         pTrial->SetChannels( GetDefaultChannelConfig() );
         CWordArray emptyArray;
         UpdateObjDep(newKey, emptyArray);
      }
   }

   return( newKey );
}


//=== DuplicateObj ====================================================================================================
//
//    Add a duplicate of a user-defined Maestro object under that object's parent.  The specified object MUST exist and
//    must have a parent in the tree-map (do not attempt to duplicate a root node!!).  The duplicate object is inserted
//    as the next sibling of the object that was duplicated.
//
//    ARGS:       kSrc  -- [in] the object's unique key.
//
//    RETURNS:    unique key of duplicated object, or CX_NULLOBJ_KEY if unsuccessful.
//
WORD CCxDoc::DuplicateObj( const WORD kSrc )
{
   WORD kDst = m_Objects.GetParentKey( kSrc );           // source object must exist and have a parent
   ASSERT( kDst != CX_NULLOBJ_KEY );

   BOOL bOK = MoveObj( kSrc, kDst, kSrc, TRUE );         // duplicate object under its parent such that duplicate is
                                                         // inserted BEFORE the original

   if( bOK )                                             // if successful, find key of the duplicate, then move the
   {                                                     // original object in front of the duplicate
      POSITION pos = m_Objects.GetFirstChild( kDst );
      WORD dupeKey = CX_NULLOBJ_KEY;
      while( pos != NULL )
      {
         WORD childKey;
         CTreeObj* pObj;
         m_Objects.GetNextChild( pos, childKey, pObj );
         if( childKey == kSrc ) break;
         dupeKey = childKey;
      }
      ASSERT( dupeKey != CX_NULLOBJ_KEY );

      MoveObj( kSrc, kDst, dupeKey, FALSE );
      return( dupeKey );
   }
   else
      return( CX_NULLOBJ_KEY );
}


//=== MoveObj =========================================================================================================
//
//    Move/copy a user-defined Maestro object to an existing destination object.  In some Maestro-specific situations,
//    only the children of the source object are moved/copied.
//
//    Calls the helper method IsValidMove() to verify that the proposed move/copy operation satisfies all structure &
//    content restrictions of the Maestro object tree.  This method also sets a flag if only the source's children
//    should be moved/copied to the destination.
//
//    ARGS:       src      -- [in] the source object's unique map key.  object must exist in the Maestro tree-map.
//                dst      -- [in] the destination object's unique map key.  also must exist.
//                bef      -- [in] if this key is a valid child of the destination, the source object is inserted
//                            immediately before it in the destination's child list; else src obj is appended.
//                bCopy    -- [in] if TRUE, object is copied; else it is moved.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxDoc::MoveObj( const WORD src, const WORD dst, const WORD bef, const BOOL bCopy )
{
   BOOL bKidsOnly = FALSE;                                  // move/copy children of source, but not src obj itself

   if( !IsValidMove( src, dst, bCopy, bKidsOnly ) )         // if operation is not valid, abort!
      return( FALSE );

   WORD newKey;
   if( bCopy )                                              // if copy, create new copy of src tree in tree-map...
      newKey = m_Objects.CopyTree( src );
   else                                                     // ...else we work from src tree itself.
      newKey = src;

   if( newKey == CX_NULLOBJ_KEY )                           // copy failed!  abort.
      return( FALSE );

   if( !bKidsOnly )                                         // move entire src (or its copy) under destination
      m_Objects.MoveTree( newKey, dst, bef );
   else                                                     // move only the kids of src (or its copy) under dst; then
   {                                                        // remove the now-childless source (or its copy). original
      POSITION pos = m_Objects.GetFirstChild( newKey );     // order of the moved/copied nodes is preserved
      while( pos != NULL )
      {
         WORD key;
         CTreeObj* pObj;
         m_Objects.GetNextChild( pos, key, pObj );
         m_Objects.MoveTree( key, dst, bef );
      }
      m_Objects.RemoveTree( newKey, FALSE );
   }

   SetModifiedFlag();                                       // mark document as modified
   return( TRUE );
}


//=== GetFullObjName ==================================================================================================
//
//    Get the complete "pathname" of specified Maestro object.  Similar to a file pathname, it reflects the exact
//    location of the object in the Maestro object tree.
//
//    For example, "targ1" is a child of "set1", which is a child of "Targets", the target subtree CX_TARGBASE.  While
//    GetObjName() retrieves the name "targ1", GetFullObjName() retrieves "//Targets/set1/targ1".  Note use of the
//    forward slash as a delimiter. The double-forward slash actually represents the Maestro object tree root; the root
//    is assigned a one-char name which is hidden from user; see InitObjTree().
//
//    ARGS:       key   -- [in] the object's unique map key.  object must exist in the CNTRLX tree-map.
//                s     -- [out] object's current full pathname is copied here.
//
//    RETURNS:    NONE.
//
VOID CCxDoc::GetFullObjName( const WORD key, CString& s ) const
{
   if( key == m_objTreeRoot )                                              // handle special case -- tree root itself
   {
      s = "//";
      return;
   }

   CTreeObj* pObj;                                                         // start with immediate name of object
   VERIFY( m_Objects.GetNode( key, pObj ) );
   s = pObj->Name();

   POSITION pos = m_Objects.GetParentPos( m_Objects.InitTraverse( key ) ); // traverse object's ancestry, prepending
   while( pos != NULL )                                                    // each ancestor to the name string
   {
      VERIFY( m_Objects.GetNode( pos, pObj ) );
      POSITION parentPos = m_Objects.GetParentPos( pos );
      if( parentPos != NULL )                                              // the root object is rep by "//"
         s = pObj->Name() + "/" + s;
      else
         s = "//" + s;
      pos = parentPos;
   }
}


//=== SetObjName ======================================================================================================
//
//    Rename the specified Maestro object.  Only *user-defined* objects may be renamed.
//
//    DESIGN NOTE:  Each Maestro object class is derived from CTreeObj and thus stores its own name, but it provides
//    only read access via CTreeObj::Name().  Object naming is strictly under the control of the tree-map collection
//    CCxTreeMap, which CCxDoc "hides" from Maestro view classes.  This method should be the only means by which views
//    (and thus the user) can rename an object.
//
//    ARGS:       key   -- [in] the object's unique map key.  object must exist in the CNTRLX tree-map.
//                s     -- [in] the proposed name for object.
//
//    RETURNS:    TRUE if object was renamed; FALSE otherwise (name invalid, or obj is not user-defined).
//
BOOL CCxDoc::SetObjName( const WORD key, const CString& s )
{
   BOOL bOK = FALSE;
   if ( IsUserObj( key ) )
      bOK = m_Objects.RenameNode( key, s );
   if ( bOK ) SetModifiedFlag();
   return( bOK );
}


//=== CopySelectedObjectsFromDocument =================================================================================
//
//    UNDER DEVELOPMENT....
//
//    ARGS:       pSrcDoc  -- [in] the experiment document that is sourcing the objects to be copied into this doc.
//                wArKeys  -- [in] the keys of the objects in the source document that are to be copied.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL CCxDoc::CopySelectedObjectsFromDocument( CCxDoc* pSrcDoc, CWordArray& wArKeys )
{
   CWordToWordMap src2DestKeyMap;                                 // map key of each src obj to key of copy in this doc
   CWordArray wArDependencies;                                    // indep objects upon which an obj's defn depends;
                                                                  // these must be copied also

   BOOL success = TRUE;
   for( int i=0; i<wArKeys.GetSize(); i++ )
   {
      WORD srcKey = wArKeys[i];
      if( !pSrcDoc->ObjExists(srcKey) )                           // skip obj if it does not exist!
         continue;

      if( !pSrcDoc->IsUserObj(srcKey) )                           // if object is predefined, it is not copied...
      {
         POSITION pos = pSrcDoc->GetFirstChildObj(srcKey);        // ...but we add all of its children to the copy list
         while( pos != NULL )
         {
            WORD wChild;
            CTreeObj* pChild;
            pSrcDoc->GetNextChildObj(pos, wChild, pChild);
            wArKeys.Add(wChild);
         }

         continue;
      }

      WORD dstKey;                                                // skip if we have already copied this object
      if( src2DestKeyMap.Lookup(srcKey, dstKey) )
         continue;

      WORD dstParentKey;
      WORD srcParentKey = pSrcDoc->GetParentObj(srcKey);          // get parent of object to be copied.  If parent is a
      if( src2DestKeyMap.Lookup(srcParentKey, dstParentKey) )     // user-defined collection, we copy it as well --
         ;                                                        // UNLESS we've already done so!
      else if( pSrcDoc->IsUserObj(srcParentKey) )
      {
         CTreeObj* srcParent = pSrcDoc->GetObject(srcParentKey);
         WORD srcParentType = srcParent->DataType();
         WORD baseType;
         if( srcParentType == CX_TRIALSET ) baseType = CX_TRIALBASE;
         else if( srcParentType = CX_TARGSET ) baseType = CX_TARGBASE;
         else baseType = CX_CONTRUNBASE;
         WORD baseKey = GetBaseObj(baseType);
         ASSERT(baseKey != CX_NULLOBJ_KEY);

         dstParentKey = InsertObj( baseKey, srcParentType, srcParent->Name() );
         if( dstParentKey == CX_NULLOBJ_KEY )
         {
            success = FALSE;
            break;
         }
         else
            src2DestKeyMap.SetAt(srcParentKey, dstParentKey);
      }
      else                                                        // else, find the predefined collection in our doc
      {                                                           // that will parent the copied object
         dstParentKey =
            GetBaseObj( pSrcDoc->GetObjType(srcParentKey) );
      }
      ASSERT(dstParentKey != CX_NULLOBJ_KEY);

      CTreeObj* pSrcObj = pSrcDoc->GetObject(srcKey);             // get src object's dependencies.  If there are any,
      pSrcObj->GetDependencies( wArDependencies );                // we MUST copy these objects before copying the src
      BOOL bAddedDependencies = FALSE;                            // obj itself -- so we insert them into the key array
      for( int j=0; j<wArDependencies.GetSize(); j++ )            // before the src object.
      {
         WORD wKey;
         if( !src2DestKeyMap.Lookup(wArDependencies[j], wKey) )
         {
            if( pSrcDoc->IsUserObj(wArDependencies[j]) )          //    only user-defined dependencies are copied
            {
               bAddedDependencies = TRUE;
               wArKeys.InsertAt( i, wArDependencies[j] );
            }
            else                                                  //    if a pre-defined object is a dependency, map
            {                                                     //    its key in src doc to key in dst doc (they
               WORD w = pSrcDoc->GetObjType(wArDependencies[j]);  //    should be the same, but just in case!)
               if( w == CX_CHANCFG )                              //    the only predefined dependencies are the
                  wKey = GetDefaultChannelConfig();               //    the default channel config or the chair target
               else                                               //    predefined targets...
                  wKey = GetChairTarget();
               src2DestKeyMap.SetAt(wArDependencies[j], wKey);
            }
         }
      }
      if( bAddedDependencies )                                    // if at least one dependency object was inserted
      {                                                           // into the key array, we must step back one in the
         --i;                                                     // key array so we copy the inserted object(s) first!
         continue;
      }

      WORD wType = pSrcObj->DataType();                           // copy the object itself:
      dstKey = InsertObj(dstParentKey, wType, pSrcObj->Name());   //    create a blank copy of the approp type in this
      if( dstKey == CX_NULLOBJ_KEY )                              //    document, using same name as in src doc
      {
         success = FALSE;
         break;
      }

      CTreeObj* pDstObj = GetObject(dstKey);                      //    get copied obj's initial dependencies, if any
      pDstObj->GetDependencies( wArDependencies );

      if( !pDstObj->CopyRemoteObj( pSrcObj, src2DestKeyMap ) )    //    copy the src object's defn, fixing keys of all
      {                                                           //    dependencies in the object's defn
         success = FALSE;
         break;
      }

      UpdateObjDep(dstKey, wArDependencies);                      //    update copied object's dependencies

      src2DestKeyMap.SetAt(srcKey, dstKey);                       //    add entry to map of already copied objects!

      if( pSrcDoc->IsCollectionObj(srcKey) )                      // if source object just copied is a collection,
      {                                                           // add all of its children to the copy list!
         POSITION pos = pSrcDoc->GetFirstChildObj(srcKey);
         while( pos != NULL )
         {
            WORD wChild;
            CTreeObj* pChild;
            pSrcDoc->GetNextChildObj(pos, wChild, pChild);
            wArKeys.Add(wChild);
         }
      }
   }

   if( !success && !src2DestKeyMap.IsEmpty() )                    // on failure, remove any objects that were added
   {                                                              // to this document
      WORD srcKey;                                                //    this first pass will remove all unlocked objs,
      WORD dstKey;                                                //    unlocking their dependencies
      POSITION pos = src2DestKeyMap.GetStartPosition();
      while( pos != NULL )
      {
         src2DestKeyMap.GetNextAssoc(pos, srcKey, dstKey);
         if( ObjExists(dstKey) )
            RemoveObj( dstKey );
      }

      pos = src2DestKeyMap.GetStartPosition();                    //    second pass removes remaining objs, which
      while( pos != NULL )                                        //    were unlocked in the first pass
      {
         src2DestKeyMap.GetNextAssoc(pos, srcKey, dstKey);
         if( ObjExists(dstKey) )
            RemoveObj( dstKey );
      }
   }

   return( success );
}


//=== PrepareKeyChain =================================================================================================
//
//    Prepare chain of keys from a recognized major subtree node to a particular node within that subtree.
//
//    ARGS:       dwArKeys -- [out] array to hold chain of keys. Each key is stored in the loword of each DWORD.  The
//                            first key is that of the specified major subtree.  Each subsequent key descends the tree
//                            hierarchy to the specified tree node, which is the last key in the chain.  If the tree
//                            node is not found in the indicated subtree, then the key chain stops with the key of that
//                            subtree.
//                wBaseType-- [in] type of major subtree. If not recognized, CX_ROOT is used.
//                wLastKey -- [in] the key of Maestro object to be located within specified major subtree.
//
//    RETURNS:    NONE.
//
VOID CCxDoc::PrepareKeyChain( CDWordArray& dwArKeys, WORD wBaseType, WORD wLastKey ) const
{
   dwArKeys.RemoveAll();                                             // make sure key chain starts out empty

   WORD wBaseKey = GetBaseObj( wBaseType );                          // get key of major subtree; if not found, use
   if( wBaseKey == CX_NULLOBJ_KEY )                                  // the tree root's key
   {
      wBaseKey = m_objTreeRoot;
      ASSERT( wBaseKey != CX_NULLOBJ_KEY );                          // (the tree root should always be defined!)
   }

   if( !IsAncestorObj( wBaseKey, wLastKey ) )                        // if specified object not found in major subtree,
   {                                                                 // then chain only has key of the subtree root
      dwArKeys.Add( wBaseKey );
      return;
   }

   WORD wKey = wLastKey;                                             // build the key chain, starting from the end and
   while( wKey != wBaseKey )                                         // working our way back to the subtree root...
   {
      dwArKeys.InsertAt( 0, MAKELONG(wKey, 0) );
      wKey = GetParentObj( wKey );
   }
   dwArKeys.InsertAt( 0, MAKELONG(wBaseKey, 0) );
}


//=== TreeInfoCB ======================================================================================================
//
//    Callback function providing info on the immediate children of any node in the current Maestro object tree.
//
//    This method is specifically tailored for use by CLiteGrid's inplace tree control editing tool; CLiteGrid is used
//    in various CNTRLX views and control panels to display and edit parametric data.
//
//    ARGS:       dwKey       -- [in] loword is key of Maestro object for which child info is requested.
//                pArLbls     -- [out] holds names of all children under specified object (NULL if info not required).
//                pArKeys     -- [out] holds keys of all children under specified object (NULL if info not required).
//                               Maestro object keys are placed in the loword of each DWORD element.
//                pArHasKids  -- [out] for each child object, a nonzero value indicates that it also has children (NULL
//                               if info not required).
//                lParam      -- [in] THIS pointer, for accessing non-static members
//
//    RETURNS:    # of children under specified object
//
int CALLBACK CCxDoc::TreeInfoCB(
      DWORD dwKey, CStringArray* pArLbls, CDWordArray* pArKeys, CByteArray* pArHasKids, LPARAM lParam )
{
   CCxDoc* pThis = (CCxDoc*)lParam;                      // cast reference to THIS ptr, for accessing member vars

   WORD wKey = LOWORD(dwKey);                            // make sure object exists
   if( !pThis->ObjExists( wKey ) ) return( 0 );

   int nKids = 0;                                        // #children under specified node
   POSITION pos = pThis->GetFirstChildObj( wKey );       // traverse children and fill info arrays...
   while( pos != NULL )
   {
      CTreeObj* pObj;
      pThis->GetNextChildObj( pos, wKey, pObj );

      if( pArLbls ) pArLbls->Add( pObj->Name() );
      if( pArKeys ) pArKeys->Add( MAKELONG(wKey,0) );
      if( pArHasKids ) pArHasKids->Add( BOOL(pThis->GetFirstChildObj( wKey ) != NULL) );
      ++nKids;
   }
   return( nKids );
}



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump contents of the Maestro document in an easy-to-read form to the supplied dump context.  The content of the
//    dump will depend on the dump context depth.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxDoc::Dump( CDumpContext& dc ) const
{
   CDocument::Dump( dc );                          // base class
   dc << "Object tree root @ " << m_objTreeRoot;   // key of Maestro object tree's root node
   m_Objects.Dump( dc );                           // the entire Maestro object tree
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate the Maestro experiment document.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxDoc::AssertValid() const
{
   CDocument::AssertValid();                                            // check base class
   ASSERT( m_Objects.IsEmpty() || (m_objTreeRoot != CX_NULLOBJ_KEY) );  // root obj is always in non-empty obj tree
   m_Objects.AssertValid();                                             // validate the Maestro object tree-map
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

/**
 Create the initial, predefined state of the Maestro experiment object tree. All predefined experiment objects are 
 "hard-coded" in this function. We avoid the use of string resources, etc. to contain the names and other info on 
 predefined objects. Note that the tree root is assigned a single-char name, as it is invisible to the user.

 As of Maestro 3.0 (document version = 4), the only predefined objects are the roots of the target, trial, 
 channel configuration, stimulus run, and perturbation subtrees, along with a single predefined target representing
 the animal chair and a default channel configuration object.

 TODO:  If this should fail, we must close the document!!!
*/
VOID CCxDoc::InitObjTree()
{
   ASSERT( m_Objects.IsEmpty() );

   CString name;
   WORD dstKey;
   WORD key;

   // create the object tree root. It has no parents and allows not user-defined object insertions. It will NOT
   // be visible to the user.
   name = _T("0");
   m_objTreeRoot = m_Objects.InsertNode(CX_NULLOBJ_KEY, CX_ROOT, CX_ISPREDEF|CX_NOINSERT, name);
   if(m_objTreeRoot == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // the target subtree is an immediate child of the root. Target objects and/or target sets go here.
   name = _T("Targets");
   dstKey = m_Objects.InsertNode(m_objTreeRoot, CX_TARGBASE, CX_ISPREDEF, name);
   if(dstKey == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // target object representing the animal chair. It is inserted under the target subtree root -- as of Maestro 3
   // there is no "Predefined" target set.
   name = _T("CHAIR");
   key = m_Objects.InsertNode(dstKey, CX_CHAIR, CX_ISPREDEF, name);
   if(key == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // the trial subtree is an immediate child of the root. Trial sets go here, but not individual trials.
   name = _T("Trials");
   dstKey = m_Objects.InsertNode(m_objTreeRoot, CX_TRIALBASE, CX_ISPREDEF, name);
   if(dstKey == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // the stimulus run subtree is an immediate child of the root. Run sets go here, but not individual runs.
   name = _T("Stimulus Runs");
   dstKey = m_Objects.InsertNode(m_objTreeRoot, CX_CONTRUNBASE, CX_ISPREDEF, name);
   if(dstKey == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // the channel config subtree, plus a predefined but modifiable default channel configuration object.
   name = _T("Channels");
   dstKey = m_Objects.InsertNode(m_objTreeRoot, CX_CHANBASE, CX_ISPREDEF, name);
   if(dstKey == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   name = _T("default");
   key = m_Objects.InsertNode(dstKey, CX_CHANCFG, CX_ISPREDEF, name);
   if(key == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   // the perturbation waveforms subtree is an immediate child of the root. Perturbation objects go here.
   name = _T("Perturbations"); 
   dstKey = m_Objects.InsertNode(m_objTreeRoot, CX_PERTBASE, CX_ISPREDEF, name);
   if(dstKey == CX_NULLOBJ_KEY) goto FATAL_EXIT;

   return;

FATAL_EXIT:
   DestroyObjTree();
   // TODO:  Initiate closure of document, or...
   return;
}


/** Free all resources allocated in the Maestro experiment object tree. */
VOID CCxDoc::DestroyObjTree()
{
   // deallocate all data object in the tree-map, as well as any resources that had been allocated to the map itself.
   m_Objects.RemoveAll(); 
   ASSERT( m_Objects.IsEmpty() );
   m_objTreeRoot = CX_NULLOBJ_KEY;
}


//=== IsValidMove =====================================================================================================
//
//    Does the specified Maestro object move/copy operation satisfy existing rules?
//
//    This helper function defines the rules which restrict the ways in which views can move or copy objects in the
//    Maestro object tree.
//       (1) Predefined objects may not be copied.  They may be moved only if destination is the predefined object's
//           current parent (that is, you may change the predefined object's location in its parent's child list).
//       (2) A move/copy to a predefined collection object is allowed only if that collection permits user-defined
//           children.
//       (3) An object cannot be copied or moved to itself or any of its descendants (this is a general requirement of
//           a hierarchical tree).
//       (4) If the source and destination have the same Maestro object type, then only the children of the source are
//           copied or moved.  In this case the source object MUST have at least one child.  The effect of this rule
//           is to limit the # of hierarchical levels in the Maestro object tree.
//       (5) If the source and destination object types are different, then the source type must be a valid child type
//           for the destination object.  This rule is enforced by the helper function ValidChildType().  It further
//           restricts the content and structure of the Maestro object tree.
//
//    ARGS:       src   -- [in] key of source object.  MUST be valid.
//                dst   -- [in] key of destination object.  MUST be valid.
//                bCopy -- [in] TRUE if copy operation; otherwise it's a move.
//                bKids -- [out] TRUE if only children of the source object should be copied/moved.
//
//    RETURNS:    TRUE if destination type can accept the specified child type; FALSE otherwise.
//
BOOL CCxDoc::IsValidMove( const WORD src, const WORD dst, const BOOL bCopy, BOOL& bKids ) const
{
   bKids = FALSE;                                              // allow entire src obj subtree to be moved/copied.

   POSITION srcPos = InitTraverseObj( src );                   // get src pos in map so we only search for it once!
   ASSERT( srcPos != NULL );                                   // src object MUST exist!

   CTreeObj* pSrc;                                             // get src & dst objects
   CTreeObj* pDst;
   m_Objects.GetNode( srcPos, pSrc );
   VERIFY( m_Objects.GetNode( dst, pDst ) );                   // dst object MUST exist!


   if( pSrc->Flags() & CX_ISPREDEF )                           // predefined objects...
      return( (!bCopy) &&                                      //    cannot be copied
              (dst == m_Objects.GetParentKey( src )) );        //    and cannot be moved to a different parent

   if( pDst->Flags() & CX_NOINSERT )                           // predefined dst does not accept user-def children
      return( FALSE );

   if( m_Objects.DoesContain( srcPos, dst ) )                  // cannot move/copy src under itself or descendant!
      return( FALSE );

   WORD sType = pSrc->DataType();
   WORD dType = pDst->DataType();
   if( sType == dType )                                        // source & destination have same type -- move/copy
   {                                                           //    the source object's children, but not src itself.
      bKids = TRUE;
      return( m_Objects.HasChildren( srcPos ) );               //    src CANNOT be childless in this case!
   }
   else if( ValidChildType( dType, sType ) )                   // dst will accept src type as child -- move/copy OK
      return( TRUE );
   else                                                        // dst does not accept src type as child!
      return( FALSE );
}


//=== ValidChildType ==================================================================================================
//
//    Does the specified destination object type accept a **user-defined** child of the specified type?
//
//    This helper function defines the rules which restrict the content and structure of the Maestro object tree.  All
//    Maestro collection objects can only accept children of certain types, as defined by this method.
//
//    ARGS:       dstType  -- [in] Maestro- parent object type
//                type     -- [in] the child object type to be validated
//
//    RETURNS:    TRUE if dstType can accept the specified child type; FALSE otherwise.
//
BOOL CCxDoc::ValidChildType( const WORD dstType, const WORD type ) const
{
   ASSERT( IsValidType( dstType ) );                              // is destination object type recognized?!

   BOOL bOK = FALSE;
   switch( dstType )
   {
      case CX_TRIALBASE :                                         // only trial sets allowed in trial subtree
         bOK = (type == CX_TRIALSET);
         break;
      case CX_TRIALSET :                                          // only trials and trial subsets under a trial set
         bOK = (type == CX_TRIAL) || (type == CX_TRIALSUBSET);
         break;
      case CX_TRIALSUBSET :                                       // only trials allowed in a trial subset
         bOK = (type == CX_TRIAL);
         break;
      case CX_CONTRUNBASE :                                       // only run sets allowed in cont-run subtree
         bOK = (type == CX_CONTRUNSET);
         break;
      case CX_CONTRUNSET :                                        // only continuous runs allowed under a run set
         bOK = (type == CX_CONTRUN);
         break;
      case CX_TARGBASE :                                          // user-defined target types & target sets allowed
         bOK = (type == CX_TARGSET) || (type == CX_XYTARG) ||     // in the target subtree
               (type == CX_RMVTARG);
         break;
      case CX_TARGSET :                                           // all user-defined target types allowed in a tgt set
         bOK = (type == CX_XYTARG) || (type == CX_RMVTARG);
         break;
      case CX_CHANBASE :                                          // there are no "channel cfg sets"; only chan cfg
         bOK = (type == CX_CHANCFG);                              // data objects allowed in this subtree
         break;
      case CX_PERTBASE :                                          // there are no "perturbation sets"; only pert data
         bOK = (type == CX_PERTURB);                              // objects allowed in this subtree
         break;
      default :                                                   // ...all others do not accept user-def child
         break;
   }

   return( bOK );
}


//=== GetObjBasename ==================================================================================================
//
//    Provide an appropriate base name for a user-defined Maestro object of the specified type.
//
//    ARGS:       type     -- [in] the object type.
//
//    RETURNS:    suggested base name for Maestro objects of the specified type.
//
LPCTSTR CCxDoc::GetObjBasename( const WORD type ) const
{
   switch( type )
   {
      case CX_TRIALSET :   return( _T("trialset") );     break;
      case CX_TRIALSUBSET: return( _T("trGrp") );        break;
      case CX_TRIAL :      return( _T("trial") );        break;
      case CX_CONTRUNSET : return( _T("runset") );       break;
      case CX_CONTRUN :    return( _T("run") );          break;
      case CX_TARGSET :    return( _T("targset") );      break;
      case CX_XYTARG :     return( _T("xytarg") );       break;
      case CX_RMVTARG :    return( _T("rmvideoTgt") );   break;
      case CX_CHANCFG :    return( _T("chancfg") );      break;
      case CX_PERTURB :    return( _T("pert") );         break;
      default :            return( _T("new") );          break;
   }
}


/**
 This method is called by Serialize() to migrate a version 3 experiment document to version 4. Version 4 was introduced
 with the release of Maestro 3, which no longer supports the predefined optic bench targets (CX_FIBER* and CX_REDLED*).
 The following tasks are performed.
    (1) All trials that use any of the obsolete targets are removed. If a trial set becomes empty as a result, that set
 is also deleted.
    (2) The predefined CX_CHAIR target is moved under the root of the "Targets" subtree (CX_TARGBASE) in the object
 tree, as it is the only predefined target still supported. The key of the CX_CHAIR target is preserved. 
    (3) The old "Predefined" target set (along with CX_FIBER*, CX_REDLED* targets) is removed from the document.
 Note that, for any stimulus runs using a "Fiber1" or "Fiber2" stimulus channel, those channels are changed to stimulus
 type "Chair" and a message is posted to Maestro's message window so that the user is aware of the change. This happens
 during deserialization of each affected stimulus run object.

 @return True if migration is successful, false otherwise. In the event of failure, an exception should be thrown
 indicating that it was not possible to migrate the version 3 experiment document.
*/
BOOL CCxDoc::MigrateToVersion4()
{
   // find key of the old "Predefined" target set and each of the predefined targets CX_CHAIR, CX_FIBER*, CX_REDLED*
   WORD wPredef = CX_NULLOBJ_KEY;
   POSITION pos = GetFirstChildObj(GetBaseObj(CX_TARGBASE));
   while(pos != NULL)
   {
      WORD wChild;
      CTreeObj* pChild;
      GetNextChildObj(pos, wChild, pChild);
      if(pChild->DataType() == CX_TARGSET && (pChild->Flags() & CX_ISPREDEF) != 0)
      {
         wPredef = wChild;
         break;
      }
   }
   if(wPredef == CX_NULLOBJ_KEY) return(FALSE);

   WORD wChair = CX_NULLOBJ_KEY;
   WORD wFiber1 = CX_NULLOBJ_KEY;
   WORD wFiber2 = CX_NULLOBJ_KEY;
   WORD wLed1 = CX_NULLOBJ_KEY;
   WORD wLed2 = CX_NULLOBJ_KEY;
   pos = GetFirstChildObj(wPredef);
   while(pos != NULL)
   {
      WORD wChild;
      CTreeObj* pChild;
      GetNextChildObj(pos, wChild, pChild);
      if(pChild->DataType() == CX_CHAIR) wChair = wChild;
      else if(pChild->DataType() == CX_FIBER1) wFiber1 = wChild;
      else if(pChild->DataType() == CX_FIBER2) wFiber2 = wChild;
      else if(pChild->DataType() == CX_REDLED1) wLed1 = wChild;
      else if(pChild->DataType() == CX_REDLED2) wLed2 = wChild;
   }
   if(wChair == CX_NULLOBJ_KEY || wFiber1 == CX_NULLOBJ_KEY || wFiber2 == CX_NULLOBJ_KEY ||
         wLed1 == CX_NULLOBJ_KEY || wLed2 == CX_NULLOBJ_KEY)
      return(FALSE);

   // remove any trials that depend on any of the obsolete targets, plus any trial sets that are emptied as a result
   if(IsLockedObj(wFiber1) || IsLockedObj(wFiber2) || IsLockedObj(wLed1) || IsLockedObj(wLed2))
   {
      // keep track of any trial sets that are left empty. We remove them only after we're done iterating over all
      // the sets. Deleting while iterating may screw up the iteration.
      CWordArray setsToDelete;

      // iterate over all trial sets...
      pos = GetFirstChildObj(GetBaseObj(CX_TRIALBASE));
      while(pos != NULL)
      {
         WORD wSet;
         CTreeObj* pSet;
         GetNextChildObj(pos, wSet, pSet);

         // for each trial set, collect the keys of trials to be removed. Set a flag if any trial is NOT removed (in
         // which case we do NOT remove the trial set. We do NOT want to delete trials while we iterate over them,
         // as this may mess up the iteration.
         BOOL trialKept = FALSE;
         CWordArray trialsToDelete;
         trialsToDelete.RemoveAll();
         POSITION pos2 = GetFirstChildObj(wSet);
         while(pos2 != NULL)
         {
            WORD wTrial;
            CTreeObj* pTrial;
            GetNextChildObj(pos2, wTrial, pTrial);

            BOOL remove = FALSE;
            CWordArray wDepAr;
            wDepAr.RemoveAll();
            pTrial->GetDependencies(wDepAr);
            for(int i=0; i<wDepAr.GetSize(); i++)
            {
               WORD wType = GetObjType(wDepAr.GetAt(i));
               if(CX_FIBER1 <= wType && wType <= CX_REDLED2)
               {
                  remove = TRUE;
                  break;
               }
            }
            if(remove) trialsToDelete.Add(wTrial);
            else trialKept = TRUE;
         }

         for(int i=0; i<trialsToDelete.GetSize(); i++) RemoveObj(trialsToDelete.GetAt(i));
         if(!trialKept) setsToDelete.Add(wSet);
      }

      // remove any trial sets that are now empty
      for(int i=0; i<setsToDelete.GetSize(); i++) RemoveObj(setsToDelete.GetAt(i));
   }


   // move the chair target directly under the target subtree root, then delete the old "Predefined" target set along
   // with the predefined target objects that remain within it
   m_Objects.MoveTree(wChair, GetBaseObj(CX_TARGBASE));
   m_Objects.RemoveTree(wPredef, FALSE);

   return(TRUE);
}