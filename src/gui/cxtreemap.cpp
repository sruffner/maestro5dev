//=====================================================================================================================
//
// cxtreemap.cpp : Implementation of self-keying "tree-map" class CCxTreeMap, in which all MAESTRO "object trees" of a
//                 MAESTRO document are stored.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The user creates experimental protocols within a MAESTRO "experiment document" (CCxDoc) by defining a variety of
// "data objects" and establishing relationships among those objects.  Every "data object" in MAESTRO has a name, a
// defined constant identifying its "abstract data class", possibly some state flags, and the parametric data which
// define how the object "behaves" in a MAESTRO experiment.  For instance, each MAESTRO "trial" defines trajectories
// of one or more "targets", which are defined separately.  The trial object also refers to a "channel set" object,
// which contains the list of analog channels that should be sampled during that trial.  Trials, targets, and channel
// sets are examples of "abstract" data classes defined in MAESTRO.  The defined constants for all MAESTRO data types 
// are found in the MAESTRO object interface file, CXOBJ_IFC.H.
//
// In addition to "data objects", MAESTRO defines "virtual collections" of other objects:  A "target set", for example,
// is a collection of individual target objects.  All MAESTRO collection objects are encapsulated by the CCxSet class
// defined here; it is a mere wrapper of the generic CTreeObj base class.  Such collection objects have been defined so
// that we can organize all of the real data objects into hierarchical tree-like structures, the MAESTRO "object 
// trees".  All targets, e.g., are stored in the "target tree", all trials in the "trial tree", and so on.  These trees 
// permit the user, via the appropriate MAESTRO view and within constraints imposed by CCxDoc, to organize the data 
// objects in a useful manner.
//
// While this hierarchical organization is important, we also must be able to access individual data (and collection)
// objects as quickly as possible:  Views are often accessing an individual object to get or set various attributes,
// change its name, etc.  They must store some sort of key that uniquely identifies each object as stored in the
// associated document.  This need for efficient keyed access suggests that we use a hash table map as the collection
// class for the MAESTRO objects.  So, we need a hash-table map collection that also embodies tree connections among
// the objects it contains -- a "tree map" collection class:  CCxTreeMap.
//
// CCxTreeMap is derived from the generic CTreeMap class, which handles the low-level implementation details of the
// tree map (see TREEMAP.CPP).  CTreeMap recognizes one base data class, CTreeObj, which merely stores the object's
// name, a constant identifying the object's abstract data type, and state flags.  CTreeObj, which is itself derived
// from MFC's CObject, serves as the starting point for building more complex data classes.  CCxTreeMap tailors the
// behavior of CTreeMap so it can handle all data types present in MAESTRO.  Each MAESTRO data class stored in the 
// MAESTRO object tree must satisfy certain constraints in order to build it on top of the CTreeMap/CTreeObj framework.
// For an explanation of these constraints, see CTreeMap/CTreeObj's implementation file.
//
// CCxTreeMap defines two key CTreeMap overrides, ConstructData() and CopyData(), which call the constructor or copy
// constructor corresponding to the object's data type.  CTreeMap handles EVERYTHING ELSE via pointers to the base
// data object CTreeObj -- taking advantage of C++ polymorphism.  TO FULLY UNDERSTAND CCxTreeMap, REVIEW CTreeMap's
// IMPLEMENTATION FILE!!
//
// Every object stored in the CCxTreeMap is associated with a unique WORD-valued key (CCxTreeMap is capable of storing
// up to 65535 objects; one key value, TM_NOKEY, is reserved as an error indication).  Unlike MFC's map collections,
// our map is self-keying.  Callers do not provide the key and thus do not have to worry about how to generate a
// unique key each time a new object is added.  Again, see CTreeMap's implementation file for details.
//
// One disadvantage of this scheme:  CCxTreeMap/CTreeMap is NOT type-safe.  Data objects are exposed to callers using
// CTreeObj pointers.  Callers -- that is, CCxDoc and its associated views -- must cast these pointers correctly to
// access the object's data.
//
// There is a division of responsibilities among CCxDoc, CCxTreeMap, and the various CTreeObj-derived classes that
// represent the real MAESTRO data objects.  First, the MAESTRO data object classes provide methods for accessing,
// modifying, and validating the actual parametric data which define how the object behaves in a MAESTRO experiment.
// CCxTreeMap is an "intelligent" storage medium for these objects (leaf nodes in the tree-map) as well as "collection"
// objects (parent nodes in the tree-map).  Collection objects allow the user to logically group together related data
// and/or other collection objects.  Since they contain no data, all MAESTRO collection objects are encapsulated by
// a simple wrapper of CTreeObj itself -- CCxSet.  CCxTreeMap must be "aware" of all the different types of MAESTRO 
// data and collection objects so that it can construct any given object by calling the appropriate constructor.
// Furthermore, it controls the naming of the objects, allowing only characters from a valid character set (it uses the
// default char set provided by CTreeMap) and requiring that no two sibling objects have the same name.  Finally, of
// course, it encodes the tree connections among the objects and provides methods for adding objects to the trees,
// removing objects, etc (almost all of these details are submerged in CTreeMap!).  However, CCxTreeMap does NOT impose
// any restrictions on how objects are added to the tree-map; that is the responsibility of CCxDoc, in coordination
// with its various views.  As mentioned above, CCxDoc uses CCxTreeMap to store a number of different "MAESTRO object
// trees"; CCxDoc methods implement the logic for constructing and restricting the exact composition of these object
// trees (see CCxDoc for details).
//
// ==> Predefined target CX_OKNDRUM no longer supported as of Maestro v1.5.0.
// The OKNDRUM was never used in Maestro, and we decided to remove it for version 1.5.0.  However, because of the way
// in which experiment documents are serialized, all documents existing prior to v1.5.0 include a CCxTarget object that
// represents the OKNDRUM.  CCxDoc::Serialize() takes care of removing it from the object tree map when such documents
// are deserialized, but that only happens AFTER the CCxTarget object is constructed.  Therefore,
// CCxTreemap::ConstructData() can still create a CCxTarget instance representing target type CX_OKNDRUM -- else,
// deserialization would always fail for pre-1.5.0 documents!!
//
//
// REVISION HISTORY:
// 21jul2000-- Created.  Implementation design described.  Began work on methods.
// 27jul2000-- Finished first version of CTreeMapWordToOb.  Began re-crafting as the more specialized CCxTreeMap...
// 30jul2000-- Finished first version of CCxTreeMap.  Only supports trial trees for now...
// 03aug2000-- Added GetParentKey(), HasChildren(), GetFirstChild(), GetNextChild().  Modified implementation of
//             GetNode() and InsertNode().
// 04aug2000-- Added public methods InitTraverse() and Traverse() for traversing tree in standard tree-traversal order.
//          -- Both Traverse() and GetNextChild() use default arguments to allow callers to request all or a subset of
//             the CNTRLX info (object type, name, and data pointer) stored in a node.
// 09aug2000-- Added overloaded versions of GetNode(), GetParentKey(), HasChildren(), and RemoveTree() that identify
//             the node using its actual pointer, cast as a generic POSITION.  Intended use -- to improve access speed
//             (don't have to search by key value!) during an ongoing tree traversal.  Also added GetParentPos().
// 10aug2000-- Use defined constant CX_NULLOBJ_KEY for the reserved key value 0.
//          -- Added method DoesContain(), which determines whether or not one node contains another in its subtree.
//          -- Method NewNode() modified to permit bypassing the self-keying scheme.  Uses a default argument, a key
//             value with defaults to CX_NULLOBJ_KEY -- causing the new node to be inserted using the self-keying
//             scheme.  Thus, typical usage of NewNode() is to call it with no argument!  If called with a different
//             key value, that key must not be used -- and new node is inserted in map IAW provided key.  This version
//             is used by Serialize()...
//          -- Modified Serialize() to store the node keys as well as nest level and the other data.  Uses the special
//             form of NewNode() to deserialize a tree-map from file.  This was necessary because, for example, a trial
//             will use key values to represent "virtual links" to the targets participating in the trial!!!
// 24aug2000-- Updated the CNTRLX object-specific routines InitData(), CopyData() and HasData() to support object types
//             that appear in the CNTRLX "target tree".
// 13sep2000-- COMPLETE REWORK!  All of the details of tree map implementation were moved to a generic, "reusable"
//             class, CTreeMap (TREEMAP.CPP/H).  CCxTreeMap is derived from CTreeMap and is tailored to represent the
//             CNTRLX object tree map.
// 25sep2000-- Added class CCxSet, which represents any collection object in the CNTRLX object trees.  Such collections
//             contain no data.  They are mere placeholders that are required to organize the real data objects in
//             hierarchical trees!   At first I used CTreeObj itself, but that would require we declare CCxTreeMap as a
//             friend class in CTreeObj -- which would make CTreeObj CNTRLX-specific!  CCxSet merely wraps CTreeObj.
// 25oct2000-- Mods to bring CCxSet and CCxTreeMap in line with recent changes in CTreeObj/CTreeMap framework.
// 21mar2001-- Mod to support channel config objects (CX_CHANBASE, CX_CHANCFG).
// 06feb2002-- Mod to handle the video display configuration object (CX_VIDEODSP, data class CCxVideoDsp).
// 22may2002-- Mod to support continuous-mode run objects (CX_CONTRUNBASE, CX_CONTRUNSET, CX_CONTRUN).
// 18oct2002-- Removed support for CX_VIDEODSP object (entry dtd 06feb2002), which has been absorbed into the
//             CCxSettings object -- which is NOT part of the CNTRLX object tree.
// 17nov2002-- Mod to support perturbation objects (CX_PERTBASE, CX_PERTURB).
// 14mar2006-- Removed support for the CX_OKNDRUM target type.
// 24mar2006-- Mods IAW replacement of VSG-based framebuffer video with RMVideo.
// 21sep2011-- Removed support for the CX_FIBER* and CX_REDLED* target types, which are deprecated as of Maestro 3.0.
// 01dec2014-- Mod to support trial "subset", a collection object (CX_TRIALSUBSET). Effec. v3.1.2.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff

#include "cxtarget.h"                        // CCxTarget -- the target data class
#include "cxtrial.h"                         // CCxTrial -- the trial data class
#include "cxchannel.h"                       // CCxChannel -- the signal channel configuration data class
#include "cxcontrun.h"                       // CCxContRun -- the "continuous run" data class
#include "cxpert.h"                          // CCxPert --- the perturbation waveform data class
#include "cxtreemap.h"


#ifdef _DEBUG
#define new DEBUG_NEW                        // version of new operator for detecting memory leaks in debug release.
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//=====================================================================================================================
// class CCxSet
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxSet, CTreeObj, 1 )


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== AssertValid [base override] =====================================================================================
//
//    Validate the MAESTRO collection object.  The object's CX_ISSETOBJ flag must be set.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxSet::AssertValid() const
{
   CTreeObj::AssertValid();
   ASSERT( (m_flags & CX_ISSETOBJ) == CX_ISSETOBJ );
}

#endif  // _DEBUG




//=====================================================================================================================
// class CCxTreeMap
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxTreeMap, CTreeMap, 1 )


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxTreeMap [constructor] ========================================================================================
//
//    Construct a new, empty MAESTRO object tree map.
//
//    We essentially rely on the CTreeMap constructor.  The default "valid char set" offered by CTreeMap suits our
//    needs, but we restrict object name size to CX_MAXOBJNAMELEN.
//
CCxTreeMap::CCxTreeMap()
{
   SetMaxNameLength( CX_MAXOBJNAMELEN );
}



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== ConstructData [base override] ===================================================================================
//
//    Construct a new data object and assign it default values as appropriate to its data type.
//
//    Here is where we tailor CTreeMap's behavior to store MAESTRO data objects in the tree map!  Based on the data 
//    type ID, it creates an instance of one of the MAESTRO data classes.  These data classes are derived from 
//    CTreeObj, as explained in the file header.
//
//    Some MAESTRO data objects are actually "collection objects".  They represent parent nodes in an object tree and
//    contain no parametric data.  For example, the root node of the MAESTRO "trial tree" contains the CX_TRIALROOT
//    collection object.  This root node parents one or more CX_TRIALSET nodes. These, in turn, are collections of
//    CX_TRIAL and CX_TRIALSUBSET nodes. The former are leaf nodes holding the actual trial objects that are defined by 
//    the user, while the latter are collections of related trials.  All such collection objects are encapsulated by 
//    CCxSet, which is essentially the same as CTreeObj except that it sets a flag bit to indicate the object is a 
//    collection.
//
//    To handle memory leaks correctly, each MAESTRO data class provides a default constructor which does not throw a
//    memory exception.  After default construction, the data object is initialized by its Initialize() method, which
//    may involve some memory allocations.
//
//    ARGS:       name  -- [in] the name assigned to the data object.
//                type  -- [in] the object's "abstract" data type.
//                flags -- [in] the object's state flags.
//
//    RETURNS:    a pointer to the data object constructed (cast to CTreeObj*)
//
//    THROWS:     CMemoryException if default construction or object initialization fails due to insufficient memory.
//                If init failed, we catch the exception and delete the constructed object before forwarding exception.
//
CTreeObj* CCxTreeMap::ConstructData( LPCTSTR name, const WORD type, const WORD flags )
{
   CTreeObj* pObj = NULL;
   switch( type )
   {
      case CX_TRIAL :                                             // a CNTRLX trial
         pObj = (CTreeObj*) new CCxTrial;
         try
         {
            ((CCxTrial*)pObj)->Initialize( name, type, flags );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxTrial*) pObj);
            throw;
         }
         break;

      // one of the MAESTRO target object types. Details of object construction vary eith the particular target type.
      // NOTES: As of Maestro v1.5.0, CX_OKNDRUM no longer supported. As of v2.0, RMVideo replaced the old VSG video.
      // As of v3.0, CX_FIBER* and CX_REDLED* no longer supported. We still allow the deprecated targets to be
      // constructed so that deserialization of old experiment documents will still succeed. During migration, the
      // obsolete target objects are eventually removed.
      case CX_CHAIR : 
      case CX_FIBER1 : 
      case CX_FIBER2 : 
      case CX_REDLED1 :
      case CX_REDLED2 :
      case CX_OKNDRUM :  
      case CX_XYTARG :
      case CX_RMVTARG : 
         pObj = (CTreeObj*) new CCxTarget;
         try
         {
            ((CCxTarget*)pObj)->Initialize( name, type, flags );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxTarget*) pObj);
            throw;
         }
         break;

      case CX_CHANCFG :                                           // a MAESTRO channel configuration object
         pObj = (CTreeObj*) new CCxChannel;
         try
         {
            ((CCxChannel*)pObj)->Initialize( name, type, flags );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxChannel*) pObj);
            throw;
         }
         break;

      case CX_CONTRUN :                                           // the MAESTRO continuous-mode run object
         pObj = (CTreeObj*) new CCxContRun;
         try
         {
            ((CCxContRun*)pObj)->Initialize( name, type, flags );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxContRun*) pObj);
            throw;
         }
         break;

      case CX_PERTURB :                                           // a MAESTRO perturbation waveform object
         pObj = (CTreeObj*) new CCxPert;
         try
         {
            ((CCxPert*)pObj)->Initialize( name, type, flags );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxPert*) pObj);
            throw;
         }
         break;

      case CX_ROOT :                                              // collection obj types contain no data -- we use
      case CX_TRIALBASE :                                         // a simple wrapper of CTreeObj itself
      case CX_TRIALSET :
      case CX_TRIALSUBSET :
      case CX_TARGBASE :
      case CX_TARGSET :
      case CX_CHANBASE :
      case CX_CONTRUNBASE :
      case CX_CONTRUNSET :
      case CX_PERTBASE :
         pObj = (CTreeObj*) new CCxSet;
         ((CCxSet*)pObj)->Initialize( name, type, flags );        // does not throw exceptions
         break;

      default :
         TRACE0( "Unrecognized MAESTRO object type!\n" );
         ASSERT( FALSE );
         break;
   }

   return( pObj );
}


//=== CopyData [base override] ========================================================================================
//
//    Construct a distinct copy of the specified data object.
//
//    Here again we tailor CTreeMap's behavior to handle MAESTRO data object types.  Based on the data type identifier
//    of the source object, we call the default constructor followed by the Copy() method of the appropriate CNTRLX
//    data class, each of which is derived from CTreeObj.
//
//    ARGS:       pSrc  -- [in] ptr to the data object to be copied.
//
//    RETURNS:    a pointer to the data object constructed (cast to CTreeObj*).
//
//    THROWS:     CMemoryException if default construction or object copying fails due to insufficient memory.  If copy
//                failed, we catch the exception and delete the constructed object before forwarding the exception.
//
CTreeObj* CCxTreeMap::CopyData( const CTreeObj* pSrc )
{
   CTreeObj* pObj = NULL;
   switch( pSrc->DataType() )                                  // data type determines which copy constructor to use!
   {
      case CX_TRIAL :                                          // a MAESTRO trial
         pObj = (CTreeObj*) new CCxTrial;
         try
         {
            ((CCxTrial*)pObj)->Copy( pSrc );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxTrial*) pObj);
            throw;
         }
         break;

      // one of the supported Maestro target object types. 
      // NOTES: Only CX_XYTARG and CX_RMVTARG can really be copied, but we don't enforce that here. Also, 
      // CX_OKNDRUM was dropped in v1.5, and CX_FIBER* and CX_REDLED* were dropped in v3.0.
      case CX_CHAIR : 
      case CX_XYTARG :
      case CX_RMVTARG :                                        // RMVideo replaced old VSG FB video in v2.0
         pObj = (CTreeObj*) new CCxTarget;
         try
         {
            ((CCxTarget*)pObj)->Copy( pSrc );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxTarget*) pObj);
            throw;
         }
         break;

      case CX_CHANCFG :                                        // a MAESTRO channel configuration object
         pObj = (CTreeObj*) new CCxChannel;
         try
         {
            ((CCxChannel*)pObj)->Copy( pSrc );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxChannel*) pObj);
            throw;
         }
         break;

      case CX_CONTRUN :                                        // a MAESTRO continuous-mode run object
         pObj = (CTreeObj*) new CCxContRun;
         try
         {
            ((CCxContRun*)pObj)->Copy( pSrc );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxContRun*) pObj);
            throw;
         }
         break;

      case CX_PERTURB :                                        // a MAESTRO perturbation waveform object
         pObj = (CTreeObj*) new CCxPert;
         try
         {
            ((CCxPert*)pObj)->Copy( pSrc );
         }
         catch( CMemoryException* e )
         {
            UNREFERENCED_PARAMETER(e);
            delete ((CCxPert*) pObj);
            throw;
         }
         break;

      case CX_ROOT :                                           // collection obj types contain no data -- we use
      case CX_TRIALBASE :                                      // a simple wrapper of CTreeObj itself
      case CX_TRIALSET :
      case CX_TRIALSUBSET :
      case CX_TARGBASE :
      case CX_TARGSET :
      case CX_CHANBASE :
      case CX_CONTRUNBASE :
      case CX_CONTRUNSET :
      case CX_PERTBASE :
         pObj = (CTreeObj*) new CCxSet;
         ((CCxSet*)pObj)->Copy( pSrc );                        // does not throw exceptions
         break;

      default :
         TRACE0( "Unrecognized MAESTRO object type!\n" );
         ASSERT( FALSE );
         break;
   }

   return( pObj );
}



