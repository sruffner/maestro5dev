//===================================================================================================================== 
//
// treemap.cpp : Implementation of CTreeMap, a self-keying map collection, and CTreeObj, which encapsulates the data 
//               stored in each node of CTreeMap. 
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//    CTreeMap is a general-purpose collection class that is intended to store data objects in one or more hierarchical 
// trees.  Data objects are represented by the CObject-derived CTreeObj class, which hold the object's name, an app-
// defined constant identifying its "data type", and any app-defined state flags.  CTreeMap/CTreeObj are intended as 
// base classes for designing an application-specific "data object tree-map" (see section 6 below).  
//    While not derived from CMapWordToOb, CTreeMap's design evolved from an examination of its implementation.  It is 
// like CMapWordToOb in that objects are stored in a hash table using a WORD-valued key for rapid retrieval. Unlike 
// CMapWordToOb, the collection is SELF-KEYING (more on this later), maintains hierarchical tree-like relationships 
// among the data objects, and employs a different memory management scheme.  In addition, with the help of properly 
// defined data object classes, CTreeMap and its derivations handle the details of constructing, copying, & destroying 
// the data objects themselves.
//
// 1. Object Trees:
// ----------------
//
//               A (ROOT) --> B(of A)--> G(of B)
//                             | ^         | ^
//                             | |         V |
//                             | |       H(of B) --> K(of H)
//                             | |         | ^
//                             | |         | |
//                             | |       ...etc...
//                             | |
//                             V |
//                            C(of A) --> ...etc...
//                             | ^
//                             | |
//                            ...etc...
//                   
//                   
//               M (ROOT) --> N(of M) --> ...etc...
//                   ...etc...
//
// Every data object stored in CTreeMap is a "tree node".  Each node contains, in addition to the actual data and the 
// node's unique WORD-valued key, some additional "tree links" which embody the tree structure above: a "parent" link 
// gives each node quick access to its parent; a "first child" link points to the head of a node's doubly-linked list 
// of children, as represented by the "previous sibling" and "next sibling" links.  Also note that you can store any 
// number of "object trees" within the map collection; each such tree will be independent from the others (the 
// collection's methods do not permit links across trees!).  It is possible to store only root objects in the tree map, 
// but that would be a wasteful use of this class (none of the tree links are used!). 
// 
// Methods are provided for operating on this tree structure.  For example, CopyTree() creates a copy of the entire 
// tree rooted at a specified node, and MoveTree() moves a branch of the tree from one location in the tree-map to 
// another, etc... 
// 
// It is important to remember that this tree structure is built on top of the (key, object) map, which is implemented 
// using a hash table:
//
//       HASH BUCKET 0   --> node1 --> node2 --> ...
//       HASH BUCKET 1   --> node3 --> node4 --> ...
//          .....
//       HASH BUCKET N-1 --> node5 --> node6 --> ...
//
// "Node1", etc. are the tree nodes described earlier.  Thus, each node contains an additional "next node in bucket" 
// link to implement the singly-linked list of tree nodes in each "hash table bucket".  This structure provides rapid 
// access to a particular tree node given its WORD-valued "key":  a hash algorithm maps the key to one of the N 
// "buckets", reducing search time by a factor of N -- assuming the nodes are evenly distributed among the buckets, 
// i.e., the map remains "compact".  In fact, CTreeMap is SELF-KEYING to always keep the underlying map as compact as 
// possible in the face of numerous insertions and deletions from the collection, and to eliminate the problem of "key 
// collisions" when the key is generated externally. 
//
//
// 2. Self-keying Scheme:
// ----------------------
//
//    We could have based CTreeMap on MFC's CMapWordToOb, but this collection class requires the caller to handle the 
// task of generating a unique key when we insert a new object.  Assigning keys in order [0, 1, 2...] is not ideal, 
// given CMapWordToOb's hash algorithm.  Also, if we make lots of insertions and deletions, we'll eventually start 
// experiencing "collisions" -- specifying a key that already exists in the map.  We decided that the collection class 
// itself should assign a key to the new object, and so we developed a scheme for doing so in CTreeMap...
//
// The hash algorithm we use to map the WORD key to a particular bucket in the hash table is 
//
//       bucket# = (key >> 4) % N, where N is the hash size, or # of buckets in the hash table.
//
// Observe that the right shift is equivalent to integer-divide by 16.  So if we were to assign keys in a "no-delete" 
// map, the optimum "key generation sequence" would be {0, 16, 32, ..., 65520, 1, 17, 33, ..., 65521, 2, 18, 34, ..., 
// 65522, ......, 15, 31, 47, ..., 65535}:  this fills the buckets in a uniform manner.  Furthermore, if the hash size 
// is both a multiple of 16 and evenly divisible into 65536, then all buckets will contain the same # of objects when 
// the map is full, 65536/N.  So we demand that hash size be a power of 2 (this constraint also helps speed up some of 
// the computations below -- multiply/divide is replaced by left/right shift).  There is a simple, invertible 
// relationship between the key value and its "cardinal order" in the optimum sequence:
//
//       key = (order << 4) % 65536 + (order >> 12).  (note that 2^12 = 4096 = 65536/16).
//
//       order = (key % 16) * 4096 + (key >> 4).
// 
// Furthermore, the "cardinal order" can be obtained from the object's "bucket pos", i.e., its zero-indexed pos within 
// a bucket of the "no-delete" map:
//
//       order = bucket pos * N + bucket# = (bucket pos << M) + bucket#, where hash size N = 2^M. 
//
// THUS, given our assumptions, we have an exact relationship between an object's position in the "no-delete" map 
// and the optimum key value that should be assigned to it.  BUT how do we maintain a compact map in the face of 
// both insertions AND deletions?  For each bucket we maintain a count of the # of objects in that bucket, and we 
// always insert a new object in the bucket that is most empty.  In addition, the objects in a given bucket are 
// maintained in ascending cardinal order.  When we insert a new object, we traverse the linked list looking for the 
// lowest-order unused key.  This is best explained by a concrete example. We use a hash size of 64, which is the fixed
// size of CTreeMap's hash table:
//
//       BUCKET 1: 5, head --> key=16 --> key=1040 --> key=3088 --> key=4112 --> key=17
//                             ord=1      ord=65       ord=193      ord=257      ord=4097
//                             pos=0      pos=1        pos=3        pos=4        pos=64
//
// This example represents a map that's undergone thousands of insertions and deletions.  Suppose that we need to 
// insert a new object and that bucket 1 has the fewest objects.  The diagram clearly shows that the lowest-order 
// unused key has a bucket pos of 2.  How do we find it?  We start with pos=0, compute the order and key value using the 
// equations above, and compare that with the order (computed from its key!) of the object currently at pos 0.  If the
// existing order is > the computed order, then we're done:  we insert the new object in front of the existing object 
// and assign it the computed key! In the example above, we must traverse the linked list until pos = 2, at which point 
// we find a hole and insert the new object with key value = 2064.
//
// Clearly, this self-keying scheme comes with a cost:  storage for the bucket counts, and slower insertion speed in 
// order to generate the optimum key value for the new object.  The performance enhancements gained:  there are no 
// key "collisions", and lookup speed is optimized in the face of numerous insertions and deletions. 
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !! Zero (0) -- the defined constant TM_NOKEY -- is reserved to       !!
// !! represent a "NULL" key -- as a failure indication when the map is !!
// !! unable to store a new object.                                     !!
// !!                                                                   !!
// !! The NewNode() method circumvents self-keying scheme when a valid  !!
// !! key is specified.  This special form of NewNode() is intended     !!
// !! ONLY for deserializing a fully-compact tree-map from file.        !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 
// 3. Memory Management:
// --------------------- 
//
// In the CMapWordToOb implementation, additional memory is automatically allocated as the map grows in size, but it is 
// not released when the map shrinks!  Thus, if a large number of objects are removed from the map, a lot of memory is 
// wasted until the collection is destroyed.  CTreeMap maintains the size of the "free pool" of unused tree nodes 
// between nAllocSize and 2*nAllocSize.
//
// !!!DEV NOTE!!! CMapWordToOb's technique for allocating additional memory is probably faster because it allocates an 
// entire block of "nodes" at once, whereas we allocate one node at a time. 
//
//
// 4. Serialization and Tree Traversal Order:
// ------------------------------------------
//
// Like CMapWordToOb, CTreeMap is serializable.  However, the implementation is less straightforward because we must 
// somehow serialize the tree links which embody the relationships among the objects.  While in memory, these links are 
// "tree node pointers".  Pointers, of course, cannot be serialized.  We can avoid serializing the links altogether by 
// storing each node in "standard tree traversal order", described recursively as follows:  (1) visit the node itself; 
// (2) traverse each of its children in standard tree traversal order; (3) and go to node's next sibling.  For 
// example (nodes are visited in alphabetical order in the diagram):
//
//    Nest Level ==> 0       1       2       3    
//    ------------------------------------------- 
//                   A------>B------>C 
//                           |       | 
//                           |       | 
//                           |       D------>E 
//                           | 
//                           F------>G 
//                           | 
//                           | 
//                           H 
//
// To serialize this tree so that it can be reconstructed upon deserialization, we merely traverse it in the specified 
// order and store each "node" as the pair (nest level, data).  The nest level keeps track of where we are in the 
// traversal, allowing us to reconstruct the tree on the fly during deserialization.  NOTE: It is important that each 
// child node be appended to the end of a parent node's child list -- else the deserialized tree would be rebuilt in 
// reverse order, as shown in the right-hand diagram. 
//
// !!!! IMPORTANT!!!!! Since CTreeMap is self-keying, it is unnecessary to save each node's assigned key during 
// serialization.  However, some applications may wish to save key values elsewhere, since they represent a unique "ID" 
// for each data object.  THEREFORE:  (1) We save the keys of all objects when we serialize the tree-map; and (2) we 
// must bypass the self-keying mechanism during deserialization and store each object using the key value read from the 
// serialization archive.  The method NewNode() can handle inserting a new node under a specified key.  
//
// 
// 5. Naming Tree Objects:
// ----------------------- 
//
// CTreeMap and CTreeObj have been designed with the intent that CTreeMap exclusively controls the naming of objects 
// contained in it.  Thus:
//    (1) CTreeObj provides public read-access to its name and type, but not public write-access. 
//    (2) CTreeObj declares CTreeMap as a friend class, thereby granting it access to the tree object's name & type.
//    (3) When CTreeMap constructs a new CTreeObj, it assigns the object's type and name.  Users of CTreeMap "suggest" 
//        a name for each new object inserted into the map, but CTreeMap will modify that name as necessary to ensure 
//        that the following rules are satisfied:
//          (a) "uniqueness" -- no two sibling nodes can share the same name. 
//          (b) "validity" -- each name must be made up to N characters from the "valid character set".
//
// The "valid character set" and max name length N are represented by **private** member variables.  CTreeMap includes 
// upper- and lower-case letters, the digits 0-9, and selected punctuation marks in the valid character set; it 
// supports names up to TM_MAXOBJNAMELEN characters in length.  However, derived classes can tailor these variables 
// by calling the methods SetValidChars() and SetMaxNameLength() -- usually in the derived class constructor since 
// changes are allowed only when the map contains no objects!  Note, however, that any max name length outside the 
// range [TM_MIN_MAXOBJNAMELEN .. TM_MAXOBJNAMELEN] will be ignored.  Also, CTreeMap requires that the valid char set 
// always contain the digits 0-9.  This is required by GenerateName(), the method which modifies a node's name to 
// enforce the uniqueness constraint. 
//
// CAVEAT:  CTreeMap is designed to handle the object naming scheme itself, with the derived class tailoring the 
// behavior ONLY via the SetValidChars() and SetMaxNameLength() methods.  There are loopholes by which a derived class 
// can gain more control -- such as the ConstructData() and CopyData() overridables.  As a rule, design these overrides 
// so that they merely copy the data object name & type as provided, without modification.  Also, any methods added in 
// derived class should NOT modify obj name or type.  If these rules are not followed, unexpected behavior may result. 
//
//
// 6. Rudimentary Support for Object Dependencies/Crosslinks:
// ----------------------------------------------------------
//
// It is easy to conceive of applications where one object in the tree-map, OBJ_A, is dependent upon another object, 
// OBJ_B, for its proper implementation or definition.  The dependent object OBJ_A would then store a copy of OBJ_B's 
// key for later access to OBJ_B.  But what if OBJ_B were deleted at some point?  Then the key stored in OBJ_A would 
// reference nothing or, worse yet, a different object that was created some time after OBJ_B was deleted.  To prevent 
// such a disaster, OBJ_A must somehow "lock" OBJ_B to register this dependency and prevent OBJ_B's deletion.
//
// The problem is that the data objects themselves (CTreeObj and derived classes) have no knowledge of the tree-map 
// that contains them.  That could be remedied by storing a pointer to the tree-map in each data object when that 
// object is created.  Then, e.g., OBJ_A could call a CTreeMap method to lock or unlock OBJ_B as needed.  I found this 
// solution too cumbersome, so I chose a different approach...
//
// Object dependencies change under three circumstances:
//    1) When a dependent data object is deleted.  CTreeMap handles this situation, since it governs the deletion of 
// objs in the tree map.  Before object "D" is deleted, CTreeMap obtains a list of the objects upon which D depends 
// and "unlocks" each one.  Object D can then be safely destroyed.
//    2) When a dependent data obj is copied.  Again, CTreeMap handles this situation.  Let C be the new obj created 
// by the copy operation; it will inherit the dependencies of the object that was copied.  So after adding C to the 
// tree-map, CTreeMap locks each object upon which C is dependent.
//    3) When a dependent data obj is modified.  THIS IS THE DIFFICULT CASE, because the CTreeMap/CTreeObj framework 
// permits an outside entity to obtain a pointer to a data object so that it can modify that object directly.  If a 
// modification causes a change in one or more dependency links, how is that change communicated to the containing 
// CTreeMap?  We place the burden on the outside entity, which must perform the following sequence:  (1) Make a copy 
// of the object's current dependencies.  (2) Modify data object as needed.  (3) Call CTreeMap::UpdateDependencies(), 
// passing it the old dependency list so that any changes can be detected and registered properly.
//
// The "lock" placed on any given object is simple.  A "lock count" is maintained for each node.  When the associated 
// data object is locked, its lock count is incremented; when unlocked, the lock count is decremented.  A node/obj 
// remains locked (and so may not be deleted from the tree-map) as long as its lock count is > 0.  IsLocked() returns 
// the node's locked state.
//
// How does CTreeMap or the "outside entity" obtain a given data object's dependencies?  The data object must make that 
// info available; hence we define the virtual CTreeObj method GetDependencies().  Note that CTreeObj itself cannot be 
// a dependent obj -- it has no storage for other object's keys.  If a data class derived from CTreeObj can represent 
// a dependent object, that class MUST override GetDependencies() appropriately.
//
// Finally, note that creating dependencies can cause serious problems.  If two distinct data objects locked each 
// other, neither could be deleted (except when map is destroyed).  Also, if an object locks one of its descendants, 
// that descendant nor the object itself can be deleted (this is because the tree branch removal process must 
// necessarily remove all descendants of a node before removing the node itself).  While the CTreeMap dependency 
// methods will not create such locks, it is important to keep these caveats in mind when using CTreeMap/CTreeObj in 
// an application.  
//
// DEV NOTES:  This is a very difficult feature to implement, probably because the CTreeMap/CTreeObj framework is not 
// properly designed...  The current solution is NOT great, as we must rely heavilyt on the "outside entity" to do the 
// right thing.  However, it avoids the complexities involved with storing a CTreeMap* pointer in each data object, and 
// the data class's responsibility is simply a matter of reporting its current stored dependencies via the override of 
// CTreeObj::GetDependencies().
//
//
// 7. Deleting Tree Nodes; Non-removable Data Objects:
// ---------------------------------------------------
//
// Applications which allow the user to insert and delete the data objects stored in the tree map may want to protect 
// certain "read-only" or "application-defined" objects from being deleted by the user.  If such objects are children 
// of another node, then that parent node cannot be deleted either (even if it is not read-only).  CTreeMap/CTreeObj 
// provides a framework for implementing these "application-defined" objects.  CTreeMap's selective removal method, 
// RemoveTree(), traverses the tree branch in reverse standard order -- attempting to delete any given node's children 
// before trying to delete the node itself.  The following criteria must be satisfied to delete a node:
//       1) The node must be childless -- so if RemoveTree() was unable to delete any of the node's children, then 
//          the node itself must remain.
//       2) The node must be unlocked -- no other nodes in the map may be dependent upon it.
//       3) The node's data object must be removable, as determined by calling virtual method CTreeObj::CanRemove(). 
//
//
// 8. Notes on Exception Handling:
// -------------------------------
//
// Given that CTreeMap can be used to represent very large collections of data objects, it is conceivable that memory 
// exceptions could occur under a variety of circumstances.  The CTreeMap/CTreeObj framework uses C++ exception 
// handling in concert with the MFC exception classes (CMemoryException, etc.).  The only exception generated by 
// CTreeMap/CTreeObj methods is CMemoryException, although Serialize() may also generate file or archive exceptions. 
// In all cases, the exception is forwarded up the call stack frame; no exception object is handled AND deleted here. 
// However, some methods must catch a memory exception in order to do some clean up before forwarding the exception; 
// "clean up" usually involves freeing memory temporarily allocated on the heap, or restoring the treemap's state to 
// what it was before the method was called.
//
// 
// 9. Designing an Application-Specific Tree-map using CTreeMap/CTreeObj:
// ----------------------------------------------------------------------
//
// Derive one or more data object classes from CTreeObj, adding methods that will construct, manipulate, serialize, 
// dump, and destroy the actual data beyond the name & data type contained in the CTreeObj itself.  Then derive a 
// specific tree-map class from CTreeMap to handle these real data classes.  If you wish to change the default char set 
// and/or the default max name length, call SetValidCharSet() and SetMaxNameLength() in the derived class constructor. 
// The low-level details of implementing the tree-map are taken care of by CTreeMap; the derived class need only 
// override a few key virtual functions to achieve application-specific behavior: 
// 
//       ConstructData()      -- CTreeMap constructs a base CTreeObj and assigns it the specified name, data type, 
//                               and state flags.  Override to construct different data objects based on specified 
//                               type.  This method will be "aware" of the different data object classes available in 
//                               the application.  
//       CopyData()           -- CTreeMap constructs a new CTreeObj and equates it to the source CTreeObj.  Override to 
//                               copy each of the different data objects available in the application. 
// 
// Below are blueprints for these two methods, for CMyMap holding a single type of object, CMyObj:
//             CTreeObj* CMyMap::ConstructData( LPCTSTR name, const WORD type, const WORD flags ) 
//             {
//                CMyObj* pNew = new CMyObj;
//                try { pNew->Initialize( name, type, flags ); }
//                catch( CMemoryException* e ) { delete pNew; throw; }
//                return( (CTreeObj*)pNew );
//             }
//             CTreeObj* CMyMap::CopyData( const CTreeObj* pSrc ) 
//             {
//                CMyObj* pNew = new CMyObj;
//                try { pNew->Copy( (CMyObj*)pSrc ) }
//                catch( CMemoryException* e ) { delete pNew; throw; }
//                return( (CTreeObj*)pNew );
//             }
//       
// In both cases, we start by creating an instance of the data object via the default constructor.  This constructor 
// should not throw exceptions (how would you get rid of the already created object once in the constructor method?). 
// The next step is to initialize the object or make it a copy of the specified object.  CTreeObj defines two virtual 
// methods for doing so, Initialize() and Copy().  [Note that CTreeObj's copy constructor is deliberately hidden, as is 
// its operator=().  Copy() should be used instead.]  While CTreeObj's versions do not throw exceptions, more complex 
// data classes (like CMyObj in the example above) may have to allocate memory in these methods, which could cause an 
// exception.  So, in the example above, we guard these calls and catch the memory exception.  This gives us a chance 
// to delete the successfully created object before forwarding the exception -- thus avoiding a memory leak.  It is 
// for this reason that we recommend treemap data frameworks derived from CTreeMap/CTreeObj adopt the approach 
// exemplified here. 
//
// !!!IMPORTANT!!! CTreeMap stores pointers to data objects, not the objects themselves.  The pointers are stored 
// internally and exposed to callers as CTreeObj*.  Callers are responsible for casting the CTreeObj* to the actual 
// data object class in order to access the additional data encapsulated by that class.  We rely on C++ polymorphism to 
// delete objects via "delete pData", where pData is a CTreeObj*.  CTreeObj is derived directly from MFC's CObject. 
// 
// So that CTreeMap and its derivatives can construct, copy, destroy, and serialize the data objects they contain, the 
// following requirements are imposed upon any data class:
//    (1) Must be derived from CTreeObj.
//    (2) Must declare CTreeMap or derivative class as a friend.  Friend classes are NOT inherited! 
//    (3) Override CTreeObj::Serialize() to appropriately serialize the data associated with the object.  The override 
//        should first call CTreeObj::Serialize() to serialize the info stored in the base class. 
//    (4) Override CTreeObj::GetDependencies() if the data object can be dependent on other data objects that are 
//        stored in the same tree map.  See section 6 above.
//    (5) Override CTreeObj::CanRemove() to implement "application-defined" objects that cannot be removed from the 
//        treemap (until its destroyed, of course).
//    (6) Override CTreeObj::Initialize() to handle required inits of a data object after default construction.
//    (7) Override CTreeObj::Copy() to provide a means of duplicating an existing instance of the data class.
//    (8) A public or protected default constructor (no args) required for dynamic object creation/serialization.  The 
//        default construtor should not throw exceptions.
//    (9) A public or protected destructor. 
//    (10) As mentioned above, we recommend that the data class hide its copy constructor and operator=.  Copying a 
//        data object or creating a new copy of an object should be handled via Copy() and the default constructor. 
//    (11) Unfortunately, the existing framework is not well-suited to copying an object in one instance of CTreeMap 
//        to a second instance.  As a fudge, we introduced the method CTreeObj::CopyRemoteObj().  All data classes in 
//        the app must override this method, which should copy all data-specific information from the source object 
//        provided.  The method includes a "dependencies key map" which maps the keys of objects in the source doc to 
//        the keys of their copies in the destination document.  The CopyRemoteObj() override MUST use this map to 
//        store the correct keys of any objects upon which the data objects definition depends; failure to do so will 
//        corrupt the document!  If successful, the override should return TRUE.  The default implementation in 
//        CTreeObj returns FALSE.
// As shown above, the CTreeMap derivative should use the default constructor and Initialize() in the ConstructData() 
// override, and the default constructor and Copy() in the CopyData() override.  There is no DestroyData() override, 
// since CTreeMap itself handles the deletion via the polymorphic delete!
//
// NOTE that, by using a protected constructor and hiding the copy constructor and operator=, we restrict creation and 
// modification of data objects.  The idea is that only CTreeMap can create and copy instances of CTreeObj.  If you 
// derive a map class CMyMap from CTreeMap and associated data classes from CTreeObj, be sure to make CMyMap a friend 
// of each data class!
//
//
// CREDITS:
// (1) Microsoft Foundation Classes, Visual C++/Developer Studio package -- Implementation of class CMapWordToOb.
//
//
// DEVELOPMENT NOTES:
//
//
// REVISION HISTORY:
// 25aug2000-- Created. 
// 13sep2000-- Initial version finished.  Still need to build & test....
//          -- Added a generic "state flags" member to CTreeObj for use in derived classes...
// 21sep2000-- Streamlined Serialize() and Dump() somewhat by using available Traverse() method.
// 19oct2000-- New features:  maintaining object dependencies, or cross-links; support for "application-defined", 
//             non-removable objects.
// 24oct2000-- Modified approach to copying data objects.  CTreeObj's copy constructor and operator= are now hidden, 
//             and the Copy() method is provided.  CTreeMap::CopyData() now uses CTreeObj's default constructor in 
//             combination with Copy() to copy a data object.  It is recommended that treemaps and associated data 
//             classes based upon CTreeMap/CTreeObj adhere to this approach.  The idea is to avoid possible exceptions 
//             in the constructor.  While this won't happen with CTreeObj, it could happen for more complex derived 
//             classes.
//          -- Modifications to implement C++ exception handling where needed.  We use MFC exception classes.
// 25oct2000-- Added CTreeObj::Initialize() and modified CTreeMap::ConstructData() accordingly.
// 13dec2000-- DoesContain() reworked for better performance.
// 15oct2003-- Modified ConnectTree() so that it puts the tree's root at the TAIL rather than the HEAD of the specified 
//             parent node.  This requires iterating to the end of the parent's child list, which may impact 
//             performance.  However, it was necessary in order to preserve each tree's standard traversal order when 
//             serializing and then deserializing.  Before this change, each serialization-deserialization sequence 
//             reversed the order of the children under any parent node.
// 20oct2003-- Modified InsertNode() and MoveTree() to support inserting a new node or moving a tree of nodes to a 
//             particular location in the destination node's child list.
// 14mar2005-- Added method CTreeObj::CopyRemoteObj() to facilitate copying objects from one treemap to another.
// 31jul2009-- !!!!!BUG FIX!!!!! After six years, discovered a mistake in the self-keying algorithm. Originally, when 
//             doing an insert, we compared computed and existing key values to determine where to insert a new object
//             within a bucket. THIS WAS WRONG. You need to compare the computed 'cardinal order', not the key value 
//             itself. We only discovered this bug 6 years later because users generally have kept their documents 
//             fairly small. The bug does not unveil itself until the treemap contains ~4096 objects.
//                In investigating the fix, we found that you can compute the cardinal order from the key, which is
//             useful in finding the insertion position for a new object as quickly as possible, while keeping the
//             objects in cardinal order within a bucket. We also decided to have a constant hash size of 64, rather
//             than adjusting it. A compact treemap of size 16 will stay reasonably compact when reshaped with a 
//             hash size of 64.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "treemap.h" 


#ifdef _DEBUG
#define new DEBUG_NEW                        // version of new operator for detecting memory leaks in debug release. 
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// class CTreeObj  
//===================================================================================================================== 
//
IMPLEMENT_SERIAL( CTreeObj, CObject, 1 )

//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 

//=== Serialize ======================================================================================================= 
//
//    Handles serialization of object.
//
//    ARGS:       ar -- [in] the serialization archive.  
//
//    RETURNS:    NONE. 
//
VOID CTreeObj::Serialize( CArchive& ar )
{
   CObject::Serialize( ar );                                   // serialize base class stuff first 

   if ( ar.IsStoring() ) 
      ar << m_type << m_flags << m_name;
   else
      ar >> m_type >> m_flags >> m_name;
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump contents of the tree data object in an easy-to-read form to the supplied dump context: "Name [type]".
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CTreeObj::Dump( CDumpContext& dc ) const
{
   ASSERT_VALID( this );                                       // validate the tree data object

   CObject::Dump( dc );                                        // dump base class first 
   dc << m_name << " [" << m_type << ", " << m_flags << "]\n"; // followed by "name [type, flags]"

   return;
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate the tree object.  Every tree object must have a non-NULL name.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CTreeObj::AssertValid() const
{
   CObject::AssertValid();
   ASSERT( !m_name.IsEmpty() ); 
}

#endif  // _DEBUG 




//===================================================================================================================== 
// class CTreeMap  
//===================================================================================================================== 
//
IMPLEMENT_SERIAL( CTreeMap, CObject, 1 )

//===================================================================================================================== 
// STATIC MEMBER INITIALIZATION
//===================================================================================================================== 
const int CTreeMap::TM_MAXOBJNAMELEN = 100; 
const int CTreeMap::TM_MIN_MAXOBJNAMELEN = 10; 
const int CTreeMap::TM_HASHSIZE = 64;
const int CTreeMap::TM_HASHEXP = 8;

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CTreeMap [constructor] ========================================================================================== 
//
//    Construct a new, empty tree-map with default hash size & allocation block size.  This is required for dynamic 
//    object creation during deserialization. 
//
CTreeMap::CTreeMap()
{
   m_nAllocSize = 20;                                             // default allocation block size 
   m_nCount = 0; 

   m_pHashTable = NULL;                                           // hash table not allocated until obj added to map 
   m_pFreeNodes = NULL;                                           // same for the free pool 
   m_nFreeCount = 0;

   m_validChars =  _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ");              // init valid char set, max len for data obj names
   m_validChars += _T("abcdefghijklmnopqrstuvwxyz0123456789");
   m_validChars += _T(".,_[]():;#@!$%*-+=<>?");
   m_maxNameLen = TM_MAXOBJNAMELEN;
}


//=== ~CTreeMap [destructor] ========================================================================================== 
//
//    Destroy the tree-map.  All resources devoted to map overhead are released.  All data objects stored in the tree 
//    map are themselves destroyed. 
//
CTreeMap::~CTreeMap()
{
   RemoveAll();
   ASSERT( (m_nCount == 0) && (m_nFreeCount == 0) );
}



//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 

//=== SetValidChars =================================================================================================== 
//
//    Change the set of valid characters that can appear in the name of a data object stored in the tree map.  All 
//    character sets will include the digits 0-9 as a minimum.
//
//    This attribute can only be changed when the map is empty -- the method is intended for use in a derived class's 
//    constructor.
//
//    ARGS:       s  -- [in] the new char set.  ignored if empty!
//
//    RETURNS:    TRUE if successful; FALSE otherwise (empty char set, or tree-map not empty). 
//
//    THROWS:     CMemoryException (when there's insufficient memory for the string manipulations performed here).
//                Note that we don't worry about cleaning up CString's if the exception occurs.  For MFC versions 3.0 
//                and later, CString exception clean up is supposedly handled by the framework.
//
BOOL CTreeMap::SetValidChars( LPCTSTR s )
{
   CString newSet = s; 
   if ( (m_nCount == 0) && !newSet.IsEmpty() )
   {
      CString digit;                               // make sure the new char set includes digits 0-9
      for( int i = 0; i <= 9; i++ )
      {
         digit.Format( "%d", i ); 
         if ( newSet.Find( digit ) < 0 )
            newSet += digit; 
      }

      m_validChars = newSet; 
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== SetMaxNameLength ================================================================================================ 
//
//    Change the max length of the name of a data object stored in the tree map. 
//    
//    This attribute can only be changed when the map is empty -- the method is intended for use in a derived class's 
//    constructor.
//
//    ARGS:       n --  [in] the new max name length.  Must be in range [TM_MIN_MAXOBJNAMELEN..TM_MAXOBJNAMELEN].
//
//    RETURNS:    TRUE if successful; FALSE otherwise (treemap not empty, or new length invalid). 
//
BOOL CTreeMap::SetMaxNameLength( int n )
{
   if ( (m_nCount == 0) && (n >= TM_MIN_MAXOBJNAMELEN) && (n <= TM_MAXOBJNAMELEN) ) 
   {
      m_maxNameLen = n;
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== SetAllocSize ==================================================================================================== 
//
//    Change the allocation block size, i.e., the number of "unused" tree nodes that are allocated at the same time 
//    when the free pool is emptied. 
//
//    ARGS:       newSize  -- [in] the new block size (must be > 0). 
//
//    RETURNS:    the previous block size. 
//
int CTreeMap::SetAllocSize( int newSize )
{
   ASSERT( newSize > 0 );
   int oldSize = m_nAllocSize;
   m_nAllocSize = newSize;
   return( oldSize );
}


//=== GetNode ========================================================================================================= 
//
//    Retrieve the data object stored in specified node of the map collection.
//
//    We provide two methods, depending on how the node is identified:  by key or by POSITION in an ongoing iteration. 
//    The second method is faster, since we do not have to search the map for the node!
//
//    ARGS:       key      -- [in] key of node to be retrieved... 
//           *OR* pos      -- [in] the node's pointer, cast as a generic POSITION. 
//
//                pData    -- [out] ptr to the data object stored in node (NULL if node not found). 
//
//    RETURNS:    TRUE if node was found; FALSE otherwise. 
//
BOOL CTreeMap::GetNode( const WORD key, CTreeObj*& pData ) const 
{
   pData = (CTreeObj*)NULL; 
   CTreeNode* pNode = GetNodeAt( key ); 
   if ( pNode == NULL )       
      return( FALSE );
   pData = pNode->pData;
   return( TRUE );
}

BOOL CTreeMap::GetNode( const POSITION pos, CTreeObj*& pData ) const 
{
   pData = (CTreeObj*)NULL; 
   CTreeNode* pNode = (CTreeNode*) pos; 
   if ( !AfxIsValidAddress( pNode, sizeof(CTreeNode) ) ) 
      return( FALSE );
   pData = pNode->pData;
   return( TRUE );
}


//=== DoesContain ===================================================================================================== 
//
//    Does the tree branch contain the specified node?
//
//    We provide two methods, depending on how base node is identified:  by key or by POSITION in an ongoing iteration. 
//    The keyed method, which is inlined, merely invokes the method defined here!
//
//    ARGS:       base     -- [in] key of base node of tree branch to search. 
//           *OR* basePos  -- [in] the base node's pointer, cast as a generic POSITION. 
//
//                test     -- [in] key of test node -- is this node part of tree branch?
//
//    RETURNS:    TRUE if branch contains test node; FALSE otherwise (including non-existent base node). 
//
BOOL CTreeMap::DoesContain( const POSITION basePos, const WORD test ) const
{
   CTreeNode* pBase = (CTreeNode*) basePos; 
   if ( !AfxIsValidAddress( pBase, sizeof(CTreeNode) ) )
      return( FALSE );

   CTreeNode* pTest = GetNodeAt( test );        // travel up tree from test node, comparing node itself and each of 
   while( pTest != NULL )                       // its ancestors to the specified base node...
   {
      if( pTest == pBase ) return( TRUE ); 
      pTest = pTest->pParent;
   } 

   return( FALSE );
}


//=== GetFirstChild, GetNextChild ===================================================================================== 
//
//    Traverse the list of children attached to a specified node.  Call GetFirstChild() to obtain the position of the 
//    first child in the list; this starts the iteration.  Each subsequent call to GetNextChild() retrieves the key and 
//    associated data object of the child at the current position, then advances that position to point to the next 
//    child node.  When we reach the end of the list, the position is set to NULL.
//
//    Intended usage:      POSITION pos = GetFirstChild( parentKey ); 
//                         while( pos != NULL ) { GetNextChild( pos, childKey, pData ); .... }
//
//    NOTES:
//    (1) GetFirstChild() cannot be called with an invalid key.
//    (2) GetNextChild() cannot be called with an invalid position.
//    (3) The "position" is really CTreeNode* cast to a POSITION.  The use of POSITION avoids requiring a static var to 
//        hold the current position in the iteration (making multi-threaded operation more feasible).  Also, by using 
//        the generic POSITION type, we avoid having to reveal details of the map's implementation. 
//
//    GetFirstChild():
//    ARGS:       key   -- [in] key of node whose children we wish to traverse (the node MUST exist in map). 
//
//    RETURNS:    position of first child (NULL if node is childless). 
//
//    GetNextChild():
//    ARGS:       pos   -- [in] ptr to current child node in iteration.  MUST be valid. 
//                      -- [out] ptr to current child's next sibling (NULL if end of children list reached). 
//                key   -- [out] current child's unique map key. 
//                pData -- [out] ptr to data obj assoc w/ current child. 
//
//    RETURNS:    NONE. 
//
POSITION CTreeMap::GetFirstChild( const WORD key ) const
{
   CTreeNode* pNode = GetNodeAt( key ); 
   ASSERT( pNode != NULL ); 
   return( (POSITION) pNode->pFirstChild );
}

VOID CTreeMap::GetNextChild( POSITION& pos, WORD& key, CTreeObj*& pData ) const 
{
   CTreeNode* pNode = (CTreeNode*) pos;                     // current pos is really a pointer to next child whose key 
   ASSERT( AfxIsValidAddress( pNode, sizeof(CTreeNode) ) ); // we must retrieve; make sure it is valid. 

   key = pNode->key;                                        // retrieve node's key & assoc. data object 
   pData = pNode->pData;

   pos = (POSITION) pNode->pNextSib;                        // move on to next sibling 
}


//=== InitTraverse, Traverse ========================================================================================== 
//
//    Traverse a tree contained in the tree-map in "standard tree-traversal order" (see file header for explanation). 
//    Call InitTraverse() to start the traversal at a particular node in the tree. Each subsequent call to Traverse() 
//    retrieves key value & associated data for the node at the current pos, advances the pos to the next node in 
//    standard tree-traversal order, and sets the change in nest level (0 to advance to next sibling, 1 to advance to 
//    first child, and a negative # when unwinding to a previous nest level).
//
//    Intended usage:      POSITION pos = InitTraverse( startKey ); 
//                         int nest = 0;
//                         int nestChange;
//                         do 
//                         { 
//                            Traverse( pos, nestChange, ... ); 
//                            nest+=nestChange;
//                            ....
//                         } while( (pos != NULL) && (nest > 0) )
//
//    NOTES:
//    (1) InitTraverse() cannot be called with an invalid key.
//    (2) Traverse() cannot be called with an invalid position.
//    (3) The "position" is really CTreeNode* cast to a POSITION.  The use of POSITION avoids requiring a static var to 
//        hold the current position in the iteration (making multi-threaded operation more feasible).  Also, by using 
//        the generic POSITION type, we avoid having to reveal details of the map's implementation. 
//    (4) If caller does not keep track of the current nest level as indicated in the example code segment, the 
//        traversal will continue until the last node of the tree is visited in standard tree-traversal order. 
//
//    InitTraverse():
//    ARGS:       key   -- [in] key of node at which to start traversal (the node MUST exist in map). 
//
//    RETURNS:    position of specified node.  
//
//    Traverse():
//    ARGS:       pos   -- [in] points to current node in iteration. MUST be valid. 
//                      -- [out] points to next node in standard order (NULL if we've reached the end). 
//                delt  -- [out] change in nesting level to get to next node. 
//                key   -- [out] current node's unique map key. 
//                pData -- [out] ptr to data obj assoc w/ current node. 
//
//    RETURNS:    NONE. 
//
POSITION CTreeMap::InitTraverse( const WORD key ) const
{
   CTreeNode* pNode = GetNodeAt( key ); 
   ASSERT( pNode != NULL ); 
   return( (POSITION) pNode ); 
}

VOID CTreeMap::Traverse( POSITION& pos, int& delt, WORD& key, CTreeObj*& pData ) const 
{
   CTreeNode* pNode = (CTreeNode*) pos;                     // current pos is really a ptr to current tree node  
   ASSERT( AfxIsValidAddress( pNode, sizeof(CTreeNode) ) ); // whose data we must retrieve

   key = pNode->key;                                        // retrieve current node's key & assoc. data object 
   pData = pNode->pData;

   StdTrav( pNode, delt );                                  // move on to next node in standard tree-traversal order
   pos = (POSITION)pNode;
}


//=== UpdateDependencies ============================================================================================== 
//
//    When a data object is modified externally, that object's "dependencies" (a dependent object stores the keys of 
//    other data objects upon which it depends) may be changed.  In this case, the modifying entity must invoke this 
//    method to ensure that the object's dependencies are brought up to date. 
//
//    Intended usage:  (1) Caller obtains a pointer to data object via CTreeMap::GetNode().  (2) Caller obtains the 
//    data object's current dependency list via CTreeObj::GetDependencies().  (3) Caller modifies the data object in 
//    some way that could change its dependency list.  (4) Caller invokes this method, passing the old dependency list 
//    so that any changes can be detected and registered.  Caller must ensure that no other changes in tree-map occur 
//    between steps (2) and (4), or unexpected behavior may occur!!
//
//    ARGS:       key   -- [in] key of node containing data obj whose dependencies are to be updated.  MUST be valid. 
//                wArOld-- [in] array of obj's dependent keys **before** data obj was modified.
//
//    RETURNS:    NONE.
// 
//    THROWS:     CMemoryException, if insufficient memory when growing CWordArray's.  If a memory exception occurs, 
//                the object's dependencies are unchanged.
//
VOID CTreeMap::UpdateDependencies( const WORD key, const CWordArray& wArOld ) 
{
   CTreeNode* pNode = GetNodeAt( key );
   ASSERT( pNode != NULL );

   CWordArray wArCurr;                                      // query node's data object for current dependencies
   pNode->pData->GetDependencies( wArCurr );                // **** THROWS CMemoryException

   BOOL bOldEmpty = BOOL( wArOld.GetSize() == 0 );
   BOOL bCurEmpty = BOOL( wArCurr.GetSize() == 0 );
   if ( bOldEmpty && bCurEmpty )                            // nothing to do in this case
      ;
   else if ( bOldEmpty && !bCurEmpty )                      // lock all current dependencies
      LockNodes( wArCurr, key );                            // **** THROWS CMemoryException
   else if ( (!bOldEmpty) && bCurEmpty )                    // unlock all old dependencies 
      LockNodes( wArOld, TM_NOKEY );
   else                                                     // general case -- may have to both lock & unlock nodes.
   {                                                        
      CWordArray wArUnlock;                                 // make copy of old dependency list
      wArUnlock.Copy( wArOld );                             // **** THROWS CMemoryException

      int i = 0;                                            // here, we remove keys which appear in both the old list 
      while( i < wArUnlock.GetSize() )                      // and current list (the unchanged dependencies).  after 
      {                                                     // doing so, the modified old list contains only "stale" 
         for( int j = 0; j < wArCurr.GetSize(); j++ )       // dependencies that need to be unlocked, while the 
         {                                                  // modified current list contains only new dependencies 
            if ( wArUnlock[i] == wArCurr[j] )               // that need to be locked.....
            {
               wArUnlock.RemoveAt(i);
               --i;
               wArCurr.RemoveAt(j);
               break;
            }
         }
         ++i;
      }

      if ( wArUnlock.GetSize() != 0 ) 
         LockNodes( wArUnlock, TM_NOKEY );
      if ( wArCurr.GetSize() != 0 ) 
         LockNodes( wArCurr, key );                         // **** THROWS CMemoryException 
   }

}


//=== InsertNode ====================================================================================================== 
//
//    Insert a single node into the map, either as a root node or as the child of an existing node. 
//
//    The data object associated with the node is also constructed IAW the data type specified, via the helper function 
//    ConstructData().  The suggested name provided will be assigned to the node unless it is invalid or not unique 
//    among the new node's siblings -- see GenerateName().
//
//    ARGS:       parent   -- [in] key of parent node for the new node (TM_NOKEY to insert root node; else, this node 
//                            MUST exist in the map). 
//                type     -- [in] the type of data object to be associated with the new node. 
//                flags    -- [in] state flags to be associated with data object. 
//                name     -- [in] a "suggested" name for the new data object. 
//                         -- [out] the name assigned to node if created (unchanged if suggested name is OK). 
//                beforeSib-- [in] key of sibling node BEFORE which the new node is inserted. if TM_NOKEY, OR if the 
//                            key does not belong to a valid sibling node, then the new node is appended to the end of 
//                            the parent node's child list (default = TM_NOKEY).
//
//    RETURNS:    key assigned to the new node (TM_NOKEY upon failure -- indicating map is full or bad parameter). 
// 
//    THROWS:     CMemoryException, if insufficient memory for the node or its associated data.  If the memory 
//                exception is thrown **after** we've successfully allocated a node, then we must first release that 
//                node before forwarding the exception. 
//
WORD CTreeMap::InsertNode( const WORD parent, const WORD type, const WORD flags, CString& name, 
   const WORD beforeSib /* =TM_NOKEY */ )
{
   if ( m_nCount == 65535 ) return( TM_NOKEY );          // tree map FULL 

   CTreeNode* pParent = NULL; 
   if ( parent != TM_NOKEY )                             // we're inserting a child node:  get specified parent... 
   { 
      pParent = GetNodeAt( parent ); 
      ASSERT( pParent != NULL );                         // ...which must exist! 
   }

   CTreeNode* pSib = NULL;                               // get sibling node (if any) before which we should insert 
   if( beforeSib != TM_NOKEY )                           // the new node; make sure the key actually references a 
   {                                                     // child of the specified parent node -- if not, we set the 
      pSib = GetNodeAt( beforeSib );                     // sibling node to NULL so that the new node is APPENDED.
      if( pSib != NULL && pSib->pParent != pParent )
         pSib = NULL;
   }

   CTreeNode* pNode = NewNode();                         // allocate new root node and place in map 
                                                         // **** THROWS CMemoryException

   try 
   {
      GenerateName( pParent, name );                     // derive valid & unique name based on suggestion (suggested 
                                                         // name is set to the name generated -- possibly unchanged) 
                                                         // **** THROWS CMemoryException

      pNode->pData = ConstructData( name, type, flags ); // attempt to construct data obj associated with node
                                                         // **** THROWS CMemoryException
   }
   catch( CMemoryException* e )                          // we must catch the above exceptions so that we can release 
   {                                                     // the previously allocated node.  exception is forwarded!! 
      UNREFERENCED_PARAMETER(e);
      FreeNode( pNode );
      throw;
   }
                  
   if ( pParent != NULL )                                // if not a root node, connect it to specified parent node 
      ConnectTree( pNode, pParent, pSib ); 

   return( pNode->key );                                 // return key value assigned to node 
}


//=== RenameNode ====================================================================================================== 
//
//    Rename an existing node in the map collection.  The new name must be valid and unique among the node's current 
//    siblings. 
//
//    NOTE:  Works for root nodes (NULL parent) -- IsUniqueName() returns TRUE immediately in this case. 
//
//    ARGS:       key   -- [in] the node's key 
//                s     -- [in] proposed name for node
//
//    RETURNS:    TRUE if successful; FALSE otherwise (node not found, or name not valid & unique among siblings). 
//
//    THROWS:     CMemoryException if there's insufficient memory for string manipulations here, in which case the 
//                node's name is unchanged. 
//
BOOL CTreeMap::RenameNode( const WORD key, const CString& s )
{
   CTreeNode* pNode = GetNodeAt( key );                  // search map for node...
   if ( pNode == NULL ) return( FALSE );                 // ...we did not find it! 

   ASSERT( pNode->pData != NULL );                       // every node has an associated data object!! 
   if ( s == pNode->pData->m_name ) return( TRUE );      // name unchanged!

   if ( IsValidName( s ) &&                              // check proposed name.  if OK, change it. 
        IsUniqueName( pNode->pParent, s ) )              // **** THROWS CMemoryException 
   {
      pNode->pData->m_name = s;                          // **** THROWS CMemoryException 
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== CopyTree ======================================================================================================== 
//
//    Copy tree branch originating at specified node in tree-map.  The copy is built as a distinct tree in the map.  
// 
//    !!!IMPORTANT!!! The data objects associated with each copied node are likewise copied.  The helper function 
//    CopyData() handles the details of copying data objects in the tree map. 
//
//    On updating object dependencies:  Each object copied may represent a "dependent object", which stores within 
//    its data one or more references to other existing objects in this tree-map.  Thus, after adding each new object 
//    to the map, we retrieve that object's dependencies (if any) and lock them...
//
//    ARGS:       srcKey   -- [in] key of node which serves as root of tree branch to be copied.  MUST be valid. 
//
//    RETURNS:    unique key of copied tree's root node if successful; otherwise, TM_NOKEY.  Fails if the tree-map's 
//                maximum capacity would be exceeded as a result of the copy operation.
//
//    THROWS:     CMemoryException if there is insufficient memory to complete the operation.  In this case, we undo 
//                any partially completed copy before forwarding the exception!
//
WORD CTreeMap::CopyTree( const WORD src )
{
   if ( m_nCount == 65535 ) return( TM_NOKEY );                // we've filled the tree map!!  abort.

   CTreeNode* pSrc = GetNodeAt( src );                         // get the root of the source tree branch... 
   ASSERT( pSrc != NULL );                                     // ...it must exist! 

   if ( (m_nCount + NumberInBranch( pSrc )) > 65535 )          // make sure we have enough room in map to do the copy 
      return( TM_NOKEY );

   int nestLevel = 0;                                          // current nesting level in tree traversal 
   BOOL bDone = FALSE; 
   CTreeNode* pRoot = NULL;                                    // the root node of the new tree
   CTreeNode* pDst = NULL;                                     // current parent node during build of new tree 
   CTreeNode* pNew = NULL;                                     // a newly allocated node, not yet connected to new tree 

   while( !bDone )                                             // traverse all nodes in source tree branch and build 
   {                                                           // a copy on the fly:
      try
      {
         pNew = NULL;
         pNew = NewNode();                                     //    allocate new unconnected node and put in map 
                                                               //    **** THROWS CMemoryException 

         pNew->pData = CopyData( pSrc->pData );                //    copy current src node's data obj to the new node 
                                                               //    **** THROWS CMemoryException 

         LockDependencies( pNew, TRUE );                       //    lock new object's dependencies...
                                                               //    **** THROWS CMemoryException
      }
      catch( CMemoryException* e )
      {
         UNREFERENCED_PARAMETER(e);
         if ( pNew != NULL )                                   //    free new but unconnected node; this also releases 
            FreeNode( pNew );                                  //    the node's data object if it, too, was created. 

         if ( pRoot != NULL )                                  //    remove partially built tree copy
            RemoveTree( (POSITION)pRoot, FALSE );

         throw;                                                //    pass on the exception
      }


      if ( nestLevel == 0 ) pRoot = pNew;                      //    save pointer to the new root node 

      if ( pDst != NULL ) ConnectTree( pNew, pDst );           //    append node to current parent in destination tree 

      if ( pSrc->pFirstChild != NULL )                         //    copy node's children if it has any..
      {
         ++nestLevel; 
         pSrc = pSrc->pFirstChild;
         pDst = pNew; 
      }
      else if ( (pSrc->pNextSib != NULL) &&                    //    else go to node's next sibling -- unless we're at 
                (nestLevel > 0) )                              //    the root of the source tree branch!
         pSrc = pSrc->pNextSib;
      else                                                     //    otherwise, unwind...
      {
         while( (nestLevel > 0) &&                             //    ...cannot unwind past root node of source tree!
                (pSrc->pNextSib == NULL) )                     //    ...unwind to a source node having a next sibling
         {
            --nestLevel;
            pSrc = pSrc->pParent; 
            pDst = pDst->pParent;
         }
         pSrc = pSrc->pNextSib;
         if ( nestLevel == 0 ) bDone = TRUE;                   //       back to root of source tree branch, so stop!
      }
   }

   return( pRoot->key );                                       // success:  return key assigned to new tree's root 
}


//=== MoveTree ======================================================================================================== 
//
//    Cut a tree branch from its current location and insert it before the specified sibling node under the specified 
//    destination.  It is possible to use this method to move a node to a different spot in its parent's child list!
//
//    ARGS:       src      -- [in] key of root node for tree branch to be moved.  MUST be valid. 
//                dst      -- [in] key of destination node for moved branch.  MUST be valid. 
//                beforeSib-- [in] key of sibling node BEFORE which the source node is inserted. if TM_NOKEY, OR if the 
//                            key does not belong to a valid sibling node, then the source node is appended to the end 
//                            of the destination node's child list (default = TM_NOKEY).
//
//    RETURNS:    NONE 
//
//    THROWS:     CMemoryException if insufficient memory for string manipulations here.  Treemap may be corrupted.
//
VOID CTreeMap::MoveTree( const WORD src, const WORD dst, const WORD beforeSib /* =TM_NOKEY */ ) 
{
   CTreeNode* pSrc = GetNodeAt( src );                         // get the root of the source tree branch... 
   ASSERT( pSrc != NULL );                                     // ...it must exist! 
   CTreeNode* pDst = GetNodeAt( dst );                         // get the destination node... 
   ASSERT( pDst != NULL );                                     // ...it must exist! 

   CTreeNode* pSib = NULL;                                     // get sibling node (if any) before which we should 
   if( beforeSib != TM_NOKEY )                                 // insert source node; make sure key actually references 
   {                                                           // child of the destinationi node -- if not, we set the 
      pSib = GetNodeAt( beforeSib );                           // sibling node to NULL so that src node is APPENDED.
      if( pSib != NULL && pSib->pParent != pDst )
         pSib = NULL;
   }

   DisconnectTree( pSrc );                                     // disconnect source tree root from its current loc 

   CString newName = pSrc->pData->m_name;                      // modify name of source tree root so that it is unique 
   GenerateName( pDst, newName );                              //    among the children of the destination node 
   pSrc->pData->m_name = newName;                              // **** THROWS CMemoryException.

   ConnectTree( pSrc, pDst, pSib );                            // reconnect source tree under destination node 
}


//=== RemoveTree ====================================================================================================== 
//
//    If possible, remove all nodes in the tree (or tree branch) sprouting from the specified node, which is also 
//    removed.  This method is designed for user-initiated deletions, as it prevents deletion of "locked" nodes or 
//    nodes containing data objects that should not be removed by the user -- as determined by calling the CTreeObj 
//    method CanRemove().  However, if the flag argument is FALSE, all nodes in the tree are removed from the map 
//    regardless.
//
//    NOTES:  
//    (1) We provide two methods, depending on how the node is identified:  by key or by POSITION in an ongoing 
//    iteration.  The keyed version is an inline method that calls the method defined here.
//    (2) Nodes are deleted in "reverse tree traversal order".  A node's children are removed first, then the node 
//    itself, then we move on to the dead node's next sibling. 
//    (3) !!!IMPORTANT!!! The data associated with each destroyed node are likewise destroyed.  For this to work, all 
//    data classes derived from CTreeObj must provide a public destructor.  See FreeNode() for details.
//    (4) Unlocking object dependencies:  Before deleting a node, we unlock any nodes upon which the dead node's data 
//    object had depended.
//    (5) If any node, A, in the tree branch cannot be removed, we still traverse node A's descendants and remove as 
//    many of them as possible.  Of course, we will be unable to remove A's direct ancestors in the branch, since A 
//    itself could not be removed.
//    (6) DEV NOTE:  This method would be much simpler to implement RECURSIVELY.  I have avoided doing so for fear 
//    of call stack overflow problems. 
//
//    ARGS:       basePos  -- [in] pointer to base node, cast as a generic POSITION value. MUST be valid.
//                bCheck   -- [in] if TRUE, only remove those nodes in the subtree which are removable (CanRemove())
//                            and not locked.  otherwise, remove all nodes.
//
//    RETURNS:    1  : the entire tree branch was successfully removed.  
//                0  : at least one of the base node's descendants was removed, but base node itself was not. 
//                -1 : nothing was removed. 
//
int CTreeMap::RemoveTree( const POSITION basePos, const BOOL bCheck ) 
{
   ASSERT_VALID( this );                                       // validate map 

   if ( m_pHashTable == NULL ) return( FALSE );                // nothing in map; abort 

   CTreeNode* pBase = (CTreeNode*) basePos;
   ASSERT( AfxIsValidAddress( pBase, sizeof(CTreeNode) ) );    // must be a valid node!!
   ASSERT( pBase->key != TM_NOKEY );                           // this is the case for nodes in free pool! 

   int nest = 0;                                               // current nest level in trav of base node's subtree 
   int delta;                                                  // change in nest level 
   CTreeNode* pCurr;                                           // current node in traversal
   CTreeNode* pNext;                                           // next node in traversal
   BOOL bDel = FALSE;                                          // TRUE if at least one obj in subtree was removed 
   BOOL bNoDel = FALSE;                                        // TRUE if at least one obj in subtree was not removed 

   pNext = pBase;                                              // delete all *descendants* of specified node:
   do
   {
      pCurr = pNext;                                           //    the current node
      StdTrav( pNext, delta );                                 //    advance to next node in standard traversal order 

      if ( delta == 1 )                                        //    curr node has children:  we must try to remove all 
         ++nest;                                               //    children before we remove the parent! 

      else while( (nest > 0) && (delta <= 0) )                 //    here we remove the curr node and (if delta<0) any 
      {                                                        //    now-childless ancestors as we trace back up tree:
         CTreeNode* pDel = pCurr;                              //    ...we will try to delete current node 
         pCurr = pDel->pParent;                                //    ...parent of current node will be current node 
         ASSERT( pCurr != NULL );                              //       on the next pass.  it must exist.

         if ( (!bCheck) ||                                     //    ...delete current node if we're NOT checking OR
              ( pDel->pData->CanRemove() &&                    //       if its data object is removable, it is not
                (pDel->wLocks == 0) &&                         //       locked, and it has no remaining children
                (pDel->pFirstChild == NULL) ) )      
         {
            LockDependencies( pDel, FALSE );
            DisconnectTree( pDel );
            FreeNode( pDel );
            bDel = TRUE;
         }
         else
            bNoDel = TRUE; 

         if ( delta < 0 ) --nest;                              //    ...unwind; parent of curr node is new curr node 
         ++delta;
      }

   } while( (pNext != NULL) && (nest > 0) ); 

   ASSERT( pCurr == pBase );                                   // we should be back at base node at this point!
   if ( (!bCheck) ||                                           // now attempt to delete base node itself 
        ( pBase->pData->CanRemove() && 
          (pBase->wLocks == 0) && 
          (pBase->pFirstChild == NULL) ) )      
   {
      LockDependencies( pBase, FALSE );
      DisconnectTree( pBase );
      FreeNode( pBase );
      bDel = TRUE;
   }
   else
      bNoDel = TRUE;

   int i = -1;                                                 // result:  nothing removed, or...
   if ( bDel && !bNoDel ) i = 1;                               // ...base node and all descendants were removed, or...
   if ( bDel && bNoDel ) i = 0;                                // ...base node not removed, but at least one 
                                                               //    descendant was.  

   return( i );
}


//=== RemoveAll ======================================================================================================= 
//
//    Remove all nodes in the entire tree map and clean up, deallocating both the hash table and the free pool.  Call 
//    this function prior to destroying the tree map collection.  Each node's data object is also destroyed. 
//
//    !!! IMPORTANT !!! To destroy each node's data object, we make certain assumptions about the object's class:  (1) 
//    It must be derived from CTreeObj, which inherits the virtual destructor of CObject.  (2) The class overrides the 
//    virtual destructor to handle the details of releasing memory allocated during construction of an instance of the 
//    class.  With these assumptions satisfied, we can safely use the delete operator on a CTreeObj* pointer to delete 
//    any type of data object!
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
VOID CTreeMap::RemoveAll()
{
   ASSERT_VALID( this );                                                // validate map 

   CBucket* pBuc;
   CTreeNode* pNode;
   for(int nBucket = 0; nBucket < TM_HASHSIZE; nBucket++)               // free all nodes in the map:  
   {
      if(m_nCount == 0) break;                                          //    stop as soon as map is empty! 

      pBuc = &m_pHashTable[nBucket];                                    //    free all nodes in this bucket...
      pNode = pBuc->pHead;
      while(pNode != NULL)
      {
         CTreeNode* pDead = pNode;                                      //    ...save ptr to next node in bucket 
         pNode = pDead->pNext;
         if(pDead->pData != NULL)                                       //    ...then delete current node's data obj 
         {
            delete (pDead->pData);                                      //       (** see note in function header **) 
            pDead->pData = NULL;
         }
         delete pDead;                                                  //    ...then delete current node itself
         --m_nCount;
      }
   }
   ASSERT( m_nCount == 0 );                                             // this should always be the case at this point 

   if(m_nFreeCount > 0)                                                 // free all nodes in the free pool. 
   {
      ASSERT( m_pFreeNodes != NULL );
      while(m_pFreeNodes != NULL)
      {
         pNode = m_pFreeNodes;
         m_pFreeNodes = m_pFreeNodes->pNext; 
         ASSERT( pNode->pData == NULL );                                // no node in free pool should have assoc data! 
         delete pNode;
         --m_nFreeCount;
      }
   }
   ASSERT( m_nFreeCount == 0 );
   ASSERT( m_pFreeNodes == NULL );
   
   if (m_pHashTable != NULL)                                            // free hash table itself 
   {
      delete[] m_pHashTable;
      m_pHashTable = NULL;
   }
}


//=== Serialize ======================================================================================================= 
//
//    Handles serialization of tree-map.
//
//    STRATEGY:  Each distinct tree (a root node with descendants) in the tree-map is stored in the archive in standard 
//    "tree traversal order":  [node itself --> its descendants --> its next siblings].  To keep track of where we are 
//    in the traversal, we store the nest level of the node (0 for the root node, 1 for its children, etc.).  To store 
//    a tree-map containing N objects in tree-traversal order, we archive N (nest level, data)-pairs.  When we read the 
//    map back from an archive, we use the stored nesting level to reconstruct each distinct tree on the fly.
//       We MUST serialize the key associated with each node, to support applications that store these keys elsewhere 
//    as "virtual cross-links", or "dependencies" across object trees.  As a consequence, when we rebuild the map 
//    during deserialization, we must insert each object in the map using its associated key -- bypassing the tree 
//    map's self-keying mechanism.  The method NewNode() handles this special situation.
//
//    NOTES:  
//    (1) Because we always append a new child node to the end of a parent's children list, sibling order is preserved 
//    in the reconstructed trees.
//    (2) When the map is "deserialized" from an archive, it is assumed we always start with an empty map:  The 
//    deserialization process dynamically creates the map object using the default constructor. 
//    (3) All data objects must themselves be serializable!!  Note also that we must use the CArchive << and >> 
//    operators to serialize the data objects, since we do not know the exact class of the object. 
//    (4) We have found that our dependency scheme (using lock counts) has been corrupted on occasion.  Rather than 
//    find the source of the corruption, we choose to cleanup dependencies when a treemap is deserialized.  See 
//    CleanupDependencies().
//
//    ARGS:       ar -- [in] the serialization archive.  
//
//    RETURNS:    NONE. 
//
//    THROWS:     -- CMemoryException, CFileException, CArchiveException thrown by MFC serialization framework. 
//                -- CMemoryException thrown if insufficient memory to build tree during deserialization.  
//                -- In its special usage here, NewNode() will return NULL without an exception if we attempt to add 
//                   an object key that already exists in the map.  This might occur if the file is corrupted or if 
//                   there's a problem with the serialization.  Obviously, we cannot continue under this circumstance; 
//                   we inform the user and then throw a generic archive exception.
//                -- If the node count read in during deserialization exceeds max capacity, then something is wrong 
//                   with the archive.  Again we inform user and throw a generic archive exception.
//                -- If an exception occurs during deserialization, we empty the partially built map.
//
VOID CTreeMap::Serialize( CArchive& ar )
{
   ASSERT_VALID( this );                                       // validate map 

   CObject::Serialize( ar );                                   // serialize base class stuff first 

   if ( ar.IsStoring() )                                       // BEGIN:  STORING TO ARCHIVE...
   {
      ar.WriteCount( m_nCount );
      if ( m_nCount == 0 ) return;                             // empty map -- nothing else to do 

      CTreeNode* pTreeRoot = NULL;                             // archive each tree stored in the map in tree-traversal 
      GetNextTreeRoot( pTreeRoot );                            // order, archiving info for each node in the tree...
      while( pTreeRoot != NULL )                               // 
      {
         int delta;
         CTreeNode* pNode = pTreeRoot;                         //    start at root node
         SHORT nest = 0;                                       //    current nesting level in tree traversal 
         do
         {
            ar << nest << pNode->key << pNode->wLocks;         //    archive current node: nest level, key, #locks, 
            ar << pNode->pData;                                //    and the data object itself
            StdTrav( pNode, delta );                           //    go to next node in tranversal 
            nest += (SHORT) delta;
         } while( (pNode != NULL) && (nest > 0) );

         GetNextTreeRoot( pTreeRoot );                         //    move on to next distinct tree within tree-map 
      }
   }
   else                                                        // BEGIN:  READING FROM ARCHIVE...
   {
      ASSERT( m_nCount == 0 );                                 // always deserialize to an initially empty map! 

      DWORD dwCount = static_cast<DWORD>(ar.ReadCount());      // total # of nodes in archived map 
      if ( dwCount > 65535 )                                   // serialized treemap must be corrupt.  inform user
      {                                                        // and throw a generic exception. 
         AfxMessageBox( "Bad treemap object count!" ); 
         AfxThrowArchiveException(
            CArchiveException::genericException, NULL );
      }

      InitHashTable();                                         // allocate the hash table
                                                               // **** THROWS CMemoryException.  Don't have to catch 
                                                               // this one, because map and free pool are unallocated 

      SHORT currNest = 0;                                      // nest level of current obj read from archive 
      SHORT prevNest = 0;                                      // nest level of previous obj read from archive 
      WORD key;                                                // key of current obj read from archive 
      WORD wLocks;                                             // #locks placed on current obj
      CTreeNode* pCurrParent = NULL;                           // current parent node in ongoing tree construction 
      CTreeNode* pNewNode = NULL;
      while( dwCount > 0 )                                     // read in (nest, data)-pairs and load into map, 
      {                                                        // building trees IAW nest level and stored order:
         try 
         {
            ar >> currNest >> key >> wLocks;                   //    get nest level, key, and #locks for next node 

            pNewNode = NewNode( key );                         //    insert empty node into map using stored key,  
                                                               //    bypassing the normal self-keying mechanism.  

            if( pNewNode == NULL)                              //    this failure indicates that serialized treemap  
            {                                                  //    is bad. We inform user and throw a generic archive  
               CTreeObj* pData;                                //    exception.
               ar >> pData;
               CString msg;
               msg.Format("Found duplicate key in treemap!:\ndwCount=%d, key=%d, name=%s, type=%d",
                     dwCount, key, pData->Name(), pData->DataType());
               AfxMessageBox( msg );
               AfxThrowArchiveException( CArchiveException::genericException, NULL );
            }
            
            ar >> pNewNode->pData;                             //    polymorphic reconstruct. of data obj from archive 
         }
         catch( CException* e )                                //    if any exception thrown, destroy partially built 
         {                                                     //    tree-map before forwarding the exception.
            UNREFERENCED_PARAMETER(e);
            RemoveAll();
            throw;
         }

         pNewNode->wLocks = wLocks;                            //    set #locks for node

         if ( currNest == 0 )                                  //    read in root node; starting a new tree 
            ; 
         else if ( currNest > prevNest )                       //    new node is child of current parent node 
            ConnectTree( pNewNode, pCurrParent ); 
         else if ( currNest == prevNest )                      //    new node is sibling of current parent node 
            ConnectTree( pNewNode, pCurrParent->pParent );
         else                                                  //    time to unwind...
         {
            while( prevNest >= currNest )
            {
               pCurrParent = pCurrParent->pParent;
               --prevNest;
            }
            ConnectTree( pNewNode, pCurrParent );
         }

         prevNest = currNest;
         pCurrParent = pNewNode;

         --dwCount;
      }

      CleanupDependencies();                                   // this fixes any bad lock counts
   }

}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump contents of the tree map in an easy-to-read form to the supplied dump context.  The content of the dump 
//    depends on the dump context depth:
//
//       depth <= 0 :   Only dump the base class (includes runtime class info) and # of nodes in the map.  
//       depth == 1 :   Dump only node keys, in "hash-table" order.  Useful for verifying self-keying scheme...
//       depth == 2 :   Traverse each tree contained in the map in "standard tree traversal order" and dump the 
//                      following info on each node:  nest level, key value, #locks, object name and type. 
//       depth > 2  :   Dump the entire data object, not just its name & type. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CTreeMap::Dump( CDumpContext& dc ) const
{
   ASSERT_VALID( this );                                    // validate the tree-map 

   CObject::Dump( dc );                                     // dump base class first 
   dc << "with " << m_nCount << " nodes\n";                 // followed by # of nodes stored in the map 

   if(m_nCount == 0) return;                                // no nodes to dump!

   int depth = dc.GetDepth();
   if(depth <= 0)                                           // shallowest dump -- we're done...
      return; 

   if(depth == 1)                                           // at this depth, we dump keys in "hash-table" order...
   {
      CBucket* pBuc;
      CTreeNode* pNode;
      for(int bkt = 0;  bkt < TM_HASHSIZE; bkt++) 
      {
         pBuc = &m_pHashTable[bkt];

         dc << "\nBUCKET " << bkt;                          //    dump heading for bucket, indicating node count 
         dc << "has " << pBuc->count << " nodes:";

         pNode = pBuc->pHead;
         int i = 0;
         while(pNode != NULL)                               //    then dump key of each node in bucket 
         {
            if(i % 10 == 0) dc << "\n";                     //    ten keys per line  
            dc << pNode->key << " ";
            pNode = pNode->pNext;
            ++i;
         }
      }

      dc << "\n\n";
      return;
   }

   CTreeNode* pTreeRoot = NULL;
   GetNextTreeRoot( pTreeRoot );                            // ...otherwise, traverse all distinct trees in the map 
   int nTrees = 0;                                          // and dump all nodes contained in them....
   while( pTreeRoot != NULL ) 
   {
      ++nTrees;                                 
      dc << "\n\n--- TREE " << nTrees << " ---";            //    dump heading for tree

      int delta;
      CTreeNode* pNode = pTreeRoot;                         //    starting w/ tree root, traverse the tree and dump 
      SHORT nest = 0;                                       //    contents at each node...
      do
      {
         CString msg;                                       //       dump current node's contents.  format is:
         msg.Format( "\n%d : [%d (locks=%d)] = ",           //          "nest level : [key (locks=N)] = "
                     nest, pNode->key, pNode->wLocks );
         dc << msg;

         if ( depth > 2 )                                   //       if deepest dump, we dump the entire data obj 
                                                            //       (format controlled by data class)
            dc << pNode->pData;
         else                                               //       else we just dump data obj type & name 
         {
            msg.Format( "(%s, type=%d)", 
                        pNode->pData->m_name,
                        pNode->pData->m_type );
            dc << msg;
         }

         StdTrav( pNode, delta );                           //       go to next node in tree
         nest += (SHORT) delta;
      } while( (pNode != NULL) && (nest > 0) );

      GetNextTreeRoot( pTreeRoot );                         //    move on to next distinct tree within tree-map 
   }

   dc << "\n\n";
   return;
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate the tree-map. As a minimum check, the map's allocation block size must be positive, and a non-empty map 
//    must have an allocated hash table. The attributes required for validating object names (valid char set, max name 
//    length) should also be set. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CTreeMap::AssertValid() const
{
   CObject::AssertValid();

   ASSERT( m_nAllocSize > 0 );
   ASSERT( (m_nCount == 0) || (m_pHashTable != NULL) ); 
   ASSERT( !m_validChars.IsEmpty() );
   ASSERT( (m_maxNameLen >= TM_MIN_MAXOBJNAMELEN) && (m_maxNameLen <= TM_MAXOBJNAMELEN) );
}

#endif  // _DEBUG 



//===================================================================================================================== 
// IMPLEMENTATION  
//===================================================================================================================== 

//=== InitHashTable =================================================================================================== 
//
//    Allocate memory for the hash table. If hash table is already allocated, method has no effect.
//
//    ARGS:       NONE.
//    RETURNS:    NONE. 
//    THROWS:     CMemoryException, if unable to allocate memory for the hash table
VOID CTreeMap::InitHashTable()
{
   ASSERT_VALID( this );
   if(m_pHashTable != NULL) return;
   m_pHashTable = new CBucket [TM_HASHSIZE];
   memset(m_pHashTable, 0, sizeof(CBucket) * TM_HASHSIZE );
}

//=== HashKey ========================================================================================================= 
//
//    Computes hash table bucket number from key value.
//
//    ARGS:       key -- [in] The key value.
//    RETURNS:    Index pos of hash table bucket that contains an object with the specified key.
int CTreeMap::HashKey(WORD key) const { return( int(DWORD(key) >> 4) % TM_HASHSIZE ); }

//=== NewNode ========================================================================================================= 
//
//    Insert a new empty, childless root node in the tree map.  The node's key is usually self-generated so that the 
//    map remains as compact as possible, UNLESS an unused key is specified. 
//
//    STRATEGY:  Retrieve an "unconnected" tree node from the free pool, allocating more nodes to the pool if it is 
//    empty.  Then insert it into the tree-map's hash table and set its key so that the map remains as compact as 
//    possible -- using the "self-keying" algorithm explained in the file header.  If a valid key is specified as an 
//    argument, then the self-keying is bypassed and the new node is inserted in the hash table IAW the key provided.
//    In either case, the new tree node has a NULL data obj ptr, and all tree links are NULL -- thus the node is 
//    initially an empty, childless root node. 
//
//    !!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    !! Normally, this method is called with no argument, so that the new node's key is generated  !!
//    !! automatically. Specifying a valid and unused key as an argument BYPASSES the "self-keying  !!
//    !! scheme".  This usage is intended only for deserializing a correct and fully compact        !!
//    !! tree-map previously serialized to disk.  USE WITH CARE!                                    !!
//    !!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//    DEV NOTE:  Not the most efficient allocation technique!  I'd like to be able to allocate an array of CTreeNodes 
//    in one go, then install pointers to each node of the array in the free pool.  But we then have to be able to 
//    deallocate the nodes individually, and I'm not sure that is appropriate.  MFC's CMapWordToOb uses a different 
//    and quite obtuse technique, but it does NOT dynamically deallocate memory if/when the free pool grows large due 
//    to many deletions from the map.
//
//    ARGS:       key   -- [in] if TM_NOKEY, then new node's key is self-generated. Otherwise, the new node is inserted
//                         IAW the specified key (if the key is already in use, no node is inserted!). 
//
//    RETURNS:    pointer to the empty tree node (NULL if map full, or specified key already in use). 
//
//    THROWS:     CMemoryException, if unable to allocate the hash table or additional free nodes as required. 
//
CTreeMap::CTreeNode* CTreeMap::NewNode ( const WORD key /* = TM_NOKEY */ )
{
   ASSERT_VALID( this );
   if(m_nCount == 65535) return(NULL);                         // map is FULL!

   CTreeNode* pNode;                                           // ptr to the new node, yet to be allocated. 

   if(m_pFreeNodes == NULL)                                    // allocate a block of nodes to the empty free pool; we 
   {                                                           // alloc one node at a time here. 
      ASSERT( m_nFreeCount == 0 );
      while(m_nFreeCount < m_nAllocSize)
      {
         pNode = new CTreeNode;                                // **** THROWS CMemoryException 

         pNode->pData = NULL;                                  // all free nodes are empty, childless, and not in tree! 
         pNode->pParent = NULL;
         pNode->pFirstChild = NULL;
         pNode->pPrevSib = NULL;
         pNode->pNextSib = NULL;
         pNode->key = TM_NOKEY; 
         pNode->wLocks = 0;

         pNode->pNext = m_pFreeNodes;
         m_pFreeNodes = pNode;
         ++m_nFreeCount;
      }
   }

   pNode = m_pFreeNodes;                                       // get a node from the free pool
   m_pFreeNodes = m_pFreeNodes->pNext;
   --m_nFreeCount;
   ASSERT( m_nFreeCount >= 0 );                                // guard against underflow 

   if(m_pHashTable == NULL) InitHashTable();                   // if hash table is not allocated, do so now! 

   int nBucket = -1;                                           // the hash table bucket that will contain new node, 
   CTreeNode* pPrevNode = NULL;                                // and the node which will precede it.  how we find 
                                                               // these depends on whether self-keying is in effect 
                                                               // or not...

   if(key == TM_NOKEY)                                         // BEGIN: find insertion point for new node IAW 
   {                                                           // self-keying scheme, maximizing map's compactness... 
   
      int minCount = 65535;                                    // find buc with fewest nodes, or first empty buc: 
      for(int nBuc = 0; nBuc < TM_HASHSIZE; nBuc++) 
      { 
         int count = m_pHashTable[nBuc].count;
         if(count == 0)                                        //    an empty bucket; stop immediately 
         {
            nBucket = nBuc;
            nBuc = TM_HASHSIZE;
         }
         else if(count < minCount)                             //    this bucket has fewer nodes
         {
            nBucket = nBuc;
            minCount = count;
         }
      }
      ASSERT( (nBucket >=0) && (nBucket < TM_HASHSIZE) ); 

      CTreeNode* pNextNode = m_pHashTable[nBucket].pHead;      // node that will come after new node
      int bucPos = (nBucket == 0) ? 1:0;                       // this excludes key value 0 (TM_NOKEY), which in our 
                                                               // self-keying scheme would be assigned to first pos in 
                                                               // first bucket of hash table. 
      while(pNextNode != NULL)
      {
         int ord = (bucPos << TM_HASHEXP) + nBucket;           // compute ideal cardinal order for this bucket pos

         int nk = (int) DWORD(pNextNode->key);                 // compute order of key currently stored there
         int nextOrd = ((nk % 16) << 12)  + (nk >> 4);
         
         if(ord < nextOrd) break;                              // found insertion point!

         pPrevNode = pNextNode;                                // else move on to next bucket pos 
         pNextNode = pNextNode->pNext;
         ++bucPos;
      }

      int ord = (bucPos << TM_HASHEXP) + nBucket;              // set node's key based on bucket pos & bucket #
      pNode->key = WORD(((ord << 4) % 65536) + (ord >> 12)); 
   }                                                           // END:  SELF-KEYING SCHEME 
   else                                                        // BEGIN: SELF-KEYING BYPASSED
   {
      // fail if key is already in use -- but first put the new node back into the free pool!
      if(GetNodeAt(key) != NULL)
      {
         pNode->pNext = m_pFreeNodes; 
         m_pFreeNodes->pNext = pNode;
         ++m_nFreeCount;
         return(NULL);
      }
      
      // find insertion point for new node IAW key provided. Nodes are stored in bucket in ascending order by the 
      // cardinal order value, NOT the key value itself. The cardinal order is computed from the key.
      nBucket = HashKey(key); 
      int nk = (int) DWORD(key);
      int ord = ((nk % 16) << 12) + (nk >> 4);
      CTreeNode* pNextNode = m_pHashTable[nBucket].pHead; 
      while(pNextNode != NULL)
      {
         nk = (int) DWORD(pNextNode->key);
         int nextOrd = ((nk % 16) << 12)  + (nk >> 4);
         if(nextOrd > ord) break;                              // new node should go before this node in bucket!

         pPrevNode = pNextNode;
         pNextNode = pNextNode->pNext;
      }

      pNode->key = key;                                        // assign provided key to the new node
   }                                                           // END:  SELF-KEYING BYPASSED


   if(pPrevNode == NULL)                                       // insert new node at head of bucket...
   {
      pNode->pNext = m_pHashTable[nBucket].pHead;
      m_pHashTable[nBucket].pHead = pNode;
   }
   else                                                        // ... or in middle or at end of bucket 
   {
      pNode->pNext = pPrevNode->pNext;
      pPrevNode->pNext = pNode;
   }
   ++(m_pHashTable[nBucket].count);

   ++m_nCount;                                                 // update total count of nodes in map 
   ASSERT( m_nCount > 0 );                                     // guard against overflow 

   return( pNode );
}


//=== FreeNode ======================================================================================================== 
//
//    Release an "unconnected" tree node from the hash table, returning it to the current free pool.  If the free pool 
//    is full (double the allocation block size), the node is deleted.  Before attempting to free a node, callers must 
//    ensure that the node's tree connections have been cut using DisconnectTree() and that the node is childless.
//
//    !!!IMPORTANT!!!
//    Any data object associated with the node is also released.  We assume that every data class associated with the 
//    tree map is derived from CTreeObj (which, in turn, is derived from MFC's CObject) and has overridden CTreeObj's 
//    virtual destructor to appropriately release memory allocated for the data.  We can then use polymorphism to 
//    delete the derived object by using the 'delete' operator on the stored CTreeObj* pointer. 
//
//    ARGS:       pNode -- [in] the tree node to be freed (MUST be empty and disconnected from any tree in map).
//
//    RETURNS:    NONE. 
//
VOID CTreeMap::FreeNode(CTreeMap::CTreeNode* pNode)
{
   ASSERT_VALID( this );                                    // validate the tree map 
   ASSERT( AfxIsValidAddress( pNode, sizeof(CTreeNode) ));  // validate tree node to be freed 
   ASSERT( pNode->pParent == NULL );                        // the node must have no tree connections!
   ASSERT( pNode->pFirstChild == NULL ); 
   ASSERT( pNode->pPrevSib == NULL ); 
   ASSERT( pNode->pNextSib == NULL ); 

   int nBucket = HashKey( pNode->key );                     // hash table bucket containing the node 

   CTreeNode* pPrevNode = NULL;                             // find node that precedes dead node 
   CTreeNode* pNextNode = m_pHashTable[nBucket].pHead; 
   while(pNextNode != NULL)
   {
      if(pNextNode == pNode) break;
      pPrevNode = pNextNode;
      pNextNode = pNextNode->pNext;
   }
   ASSERT( pNextNode == pNode );                            // we MUST find the node; else map is corrupted. 

   if(pPrevNode == NULL)                                    // free dead node from head of hash table bucket...
      m_pHashTable[nBucket].pHead = pNode->pNext;
   else                                                     // ...or somewhere in the middle of bucket. 
      pPrevNode->pNext = pNode->pNext;
   pNode->pNext = NULL;
   --(m_pHashTable[nBucket].count);

   --m_nCount;                                              // decrement total node count
   ASSERT( m_nCount >= 0 );                                 // guard against underflow 

   pNode->key = TM_NOKEY;                                   // reset key, #locks, and data stored in dead node 
   pNode->wLocks = 0;
   if(pNode->pData != NULL )
   {
      delete (pNode->pData);                                // !!polymorphic delete!!
      pNode->pData = NULL;
   }

   if(m_nFreeCount < m_nAllocSize * 2)                      // add dead node to the free pool...
   {
      pNode->pNext = m_pFreeNodes;
      m_pFreeNodes = pNode;
      ++m_nFreeCount;
   }
   else                                                     // ...or deallocate node if pool is full
      delete pNode;
}


//=== GetNodeAt ======================================================================================================= 
//
//    Find tree node with specified key (if it exists in map). 
//
//    NOTE:  Since nodes are stored in a particular bucket in ascending order by cardinal order value, we can stop 
//    searching the bucket as soon as an existing node's cardinal order is greater than that of the sought-for key!
//
//    ARGS:       key -- [in] the key of node to be found. 
//
//    RETURNS:    pointer to the tree node (NULL if it was not found).  
//
CTreeMap::CTreeNode* CTreeMap::GetNodeAt( const WORD key ) const
{
   ASSERT_VALID( this );                                       // validate the tree map 

   if(m_pHashTable == NULL) return(NULL);                      // no hash table has been allocated yet! 

   int nBucket = HashKey(key);                                 // get index of bucket containing the key 
   int nk = (int) DWORD(key);                                  // compute key's cardinal order
   int ord = ((nk % 16) << 12)  + (nk >> 4);
   
   CTreeNode* pNextNode = m_pHashTable[nBucket].pHead;         // search thru bucket for node with matching key
   while( pNextNode != NULL )
   {
      // found it!
      if(pNextNode->key == key) return(pNextNode); 
      
      // if order(existingKey) > order(keySought), then keySought is NOT in use!
      nk = (int) DWORD(pNextNode->key); 
      int nextOrd = ((nk % 16) << 12)  + (nk >> 4);
      if(nextOrd > ord) break; 
      
      pNextNode = pNextNode->pNext;
   }
   
   return(NULL);                                               // key not found 
}


//=== GetNextNode ===================================================================================================== 
//
//    Used to iterate over all the nodes contained in the tree map, in "hash table order":  we iterate over all the 
//    nodes in the singly-linked list for bucket 0, then bucket 1, and so on.
//
//    ARGS:       pNextNode   -- [in] the previous node in the iteration (NULL to start an iteration). 
//                            -- [out] the next node in the iteration (NULL if there aren't any more). 
//
//    RETURNS:    NONE.  
//
VOID CTreeMap::GetNextNode(CTreeMap::CTreeNode*& pNextNode) const
{
   ASSERT_VALID( this );                                          // validate tree map 

   if(m_nCount == 0)                                              // there are no nodes in map!
   {
      pNextNode = NULL;
      return;
   }

   if(pNextNode != NULL)                                          // validate a previous node 
   {
      ASSERT(AfxIsValidAddress( pNextNode, sizeof(CTreeNode) )); 
      ASSERT( pNextNode == GetNodeAt( pNextNode->key ) );
   }

   if(pNextNode == NULL)                                          // starting iteration...
   { 
      int buc = 0;                                                //    ...find first non-empty bucket
      while(buc < TM_HASHSIZE)
      {
         if(m_pHashTable[buc].count != 0) break;
         ++buc; 
      }
      ASSERT( buc < TM_HASHSIZE );                                //    ...map is NOT empty, so we MUST find a node!
      pNextNode = m_pHashTable[buc].pHead;                        //    ...first node in first non-empty bucket 
   }
   else                                                           // continuing iteration...
   {
      if(pNextNode->pNext != NULL)                                //    ...if it exists, go to next node within 
         pNextNode = pNextNode->pNext;                            //    the current bucket. 
      else                                                        //    ...else move on to next non-empty bucket
      {
         int buc = HashKey( pNextNode->key ) + 1;
         while(buc < TM_HASHSIZE)
         {
            if(m_pHashTable[buc].count != 0) break;
            ++buc; 
         }
         if(buc == TM_HASHSIZE)                                   //    ...if there are no more non-empty buckets, 
            pNextNode = NULL;                                     //    then the iteration is finished.
         else
            pNextNode = m_pHashTable[buc].pHead;
      }
   }
}


//=== GetNextTreeRoot ================================================================================================= 
//
//    Used to iterate over all the tree root nodes contained in the tree map. 
//
//    ARGS:       pNextRoot   -- [in] the previous root node in the iteration (NULL to start an iteration). 
//                            -- [out] the next root node in the iteration (NULL if there aren't any more). 
//
//    RETURNS:    NONE.  
//
VOID CTreeMap::GetNextTreeRoot( CTreeMap::CTreeNode*& pNextRoot ) const
{
   CTreeNode* currNode = pNextRoot;                      // get start node for search.  if starting iteration, we get 
   GetNextNode( currNode );                              // first node in map; else, node following previous root node. 
   while( currNode != NULL )
   {
      if ( IsRoot( currNode ) ) break;
      GetNextNode( currNode );
   }

   pNextRoot = currNode;
}


//=== StdTrav ========================================================================================================= 
//
//    Implements standard tree traversal:  Visit all of current node's children, then move on to next sibling.  If no 
//    next sibling, then "unwind" back up tree hierarchy until we find a node with a next sibling.
//
//    ARGS:       pNext -- [in] points to current node in the traversal.  MUST not be NULL.
//                      -- [out] the next root node in the traversal (NULL if there aren't any more). 
//                delt  -- [out] change in nesting level to get to next node. 
//
//    RETURNS:    NONE.  
//
VOID CTreeMap::StdTrav( CTreeNode*& pNext, int& delt ) const
{
   ASSERT( pNext != NULL ); 

   if ( pNext->pFirstChild != NULL )                        // move on to next node in standard tree-traversal order:
   {
      pNext = pNext->pFirstChild;                           //    first visit children if any...
      delt = 1;
   }
   else if ( pNext->pNextSib != NULL )                      //    else move on to next sibling...
   {
      pNext = pNext->pNextSib;
      delt = 0;
   }
   else                                                     //    ...then "unwind" when we reach end of a sib list 
   {
      delt = 0;
      while( (pNext->pParent != NULL) &&                    //    (cannot unwind past a tree root!)
             (pNext->pNextSib == NULL) ) 
      {
         --delt;
         pNext = pNext->pParent;
      }
      pNext = pNext->pNextSib;
   }
}




//=== ConnectTree ===================================================================================================== 
//
//    Insert a complete tree (i.e., a *root* node with or without children) as a child of the specified tree node.  
//    The tree's root node is either appended to the end of the specified parent's child list, or it is inserted 
//    before the specified sibling node.
//
//    This function only affects tree connections of the relevant nodes; the locations of these nodes in the map's hash 
//    table are unaffected. 
//
//    ARGS:       pNode       -- [in] the root node to be connected. 
//                pDst        -- [in] the new parent node for specified root node. 
//                pBeforeSib  -- [in] the sibling node before which the root node should be connected; if NULL, the 
//                               node is appended to end of child list (default = NULL).
//
//    RETURNS:    NONE.  
//
VOID CTreeMap::ConnectTree( CTreeMap::CTreeNode* pNode, CTreeMap::CTreeNode* pDst, CTreeNode* pBeforeSib /* =NULL */ )
{
   ASSERT( pNode != NULL );
   ASSERT( pDst != NULL );
   ASSERT( pNode == GetNodeAt( pNode->key ) );              // verify that nodes are in the tree map
   ASSERT( pDst == GetNodeAt( pDst->key ) );

   ASSERT( pNode->pParent == NULL );                        // verify state of root node (no parent or siblings) 
   ASSERT( pNode->pPrevSib == NULL );
   ASSERT( pNode->pNextSib == NULL );

   if( pBeforeSib && pBeforeSib->pParent == pDst )          // insert before specified sibling node, if it exists:
   {
      CTreeNode* pNewPrevSib = pBeforeSib->pPrevSib;
      pNode->pPrevSib = pNewPrevSib;
      pBeforeSib->pPrevSib = pNode;
      pNode->pNextSib = pBeforeSib;
      if( pNewPrevSib == NULL ) pDst->pFirstChild = pNode;  //    in case we insert at beginning of child list
      else pNewPrevSib->pNextSib = pNode;
   }
   else                                                     // else, we append the node to destination's child list: 
   {
      CTreeNode* pLastChild = pDst->pFirstChild;            //    find last child in parent's child list, if any 
      if( pLastChild != NULL )
      {
         while( pLastChild->pNextSib != NULL ) pLastChild = pLastChild->pNextSib;
      }

      if( pLastChild == NULL )                              //    if parent's child list is empty, the specified node 
         pDst->pFirstChild = pNode;                         //    becomes its first child; otherwise, the node is put 
      else                                                  //    at the end of the child list
      {
         pLastChild->pNextSib = pNode;
         pNode->pPrevSib = pLastChild;
      }
   }

   pNode->pParent = pDst; 
}


//=== DisconnectTree ================================================================================================== 
//
//    Disconnect specified tree node from its current location.  It becomes a root node:  no parent or siblings, but it 
//    could still have descendants! 
//
//    ARGS:       pNode -- [in] the node to be disconnected. 
//
//    RETURNS:    NONE.  
//
VOID CTreeMap::DisconnectTree( CTreeMap::CTreeNode* pNode )
{
   ASSERT( pNode != NULL );
   ASSERT( pNode == GetNodeAt( pNode->key ) );           // verify that node is in the tree map 

   if ( IsRoot( pNode ) ) return;                        // node is already a root node -- nothing to do 

   if ( pNode->pPrevSib != NULL )                        // excise node from doubly-linked sibling list 
      (pNode->pPrevSib)->pNextSib = pNode->pNextSib; 
   else
      (pNode->pParent)->pFirstChild = pNode->pNextSib;
   if ( pNode->pNextSib != NULL )
      (pNode->pNextSib)->pPrevSib = pNode->pPrevSib;

   pNode->pPrevSib = NULL;                               // cut tree connections to parent, siblings 
   pNode->pNextSib = NULL;
   pNode->pParent = NULL;
}


//=== NumberInBranch ================================================================================================== 
//
//    Count the number of nodes that are in the tree branch rooted at specified node.  Base node itself is included. 
//
//    ARGS:       pNode -- [in] the node. 
//
//    RETURNS:    # of nodes in tree branch.
//
int CTreeMap::NumberInBranch( CTreeNode* pNode ) const
{
   int nest = 0, delt = 0, count = 0;
   do
   {
      StdTrav( pNode, delt );
      nest += delt;
      ++count;
   } while( (pNode != NULL) && (nest > 0) );    // stop traversing as soon as we've traversed base node's last child 

   return( count ); 
}


//=== IsValidName ===================================================================================================== 
//
//    Does string represent a valid data object name?  Data object names are restricted in length and char content.
//
//    ARGS:       name  -- [in] the name to be tested. 
//
//    RETURNS:    TRUE if name is valid; FALSE otherwise. 
//
//    THROWS:     CMemoryException, if there is insufficient memory for string manipulations. 
//
BOOL CTreeMap::IsValidName( const CString& name ) const
{
   int len = name.GetLength(); 
   CString goodChars = name.SpanIncluding( m_validChars );  // **** THROWS CMemoryException

   if (     (len == 0) 
         || (len > m_maxNameLen)
         || (goodChars.GetLength() < len) 
      )
      return( FALSE );
   else
      return( TRUE );
}


//=== IsUniqueName ==================================================================================================== 
//
//    Is string unique among the object names of the children of specified node?  CTreeMap does not permit duplication 
//    of object names among siblings.  This constraint is checked/enforced by calling this method.
//
//    ARGS:       pParent  -- [in] ensure name is unique among this node's children . 
//                name     -- [in] the name to be tested. 
//
//    RETURNS:    TRUE if name is not a duplicate (or if parent node is NULL); FALSE otherwise. 
//
BOOL CTreeMap::IsUniqueName( CTreeMap::CTreeNode* pParent, const CString& name ) const 
{
   if ( pParent != NULL ) 
   {
      CTreeNode* pChild = pParent->pFirstChild;
      while( pChild != NULL )
      {
         ASSERT( pChild->pData != NULL );
         if ( pChild->pData->m_name == name ) return( FALSE );    // ...NOT unique!
         pChild = pChild->pNextSib;
      }
   }

   return( TRUE );                                                // if we get here, name is unique
}


//=== GenerateName ==================================================================================================== 
// 
//    Generate a valid and unique data object name with the specified base name.
//
//    ALGORITHM:  If base name is already valid & unique, do nothing.  Otherwise:  Remove any invalid characters.  Then 
//    append "<N>", where <N> is the string representation of an integer >= 1.  NOTE that this algorithm would fail if   
//    N reaches the largest 16-bit USHORT, but that cannot happen since the tree map can only hold that many items!  
// 
//    DEV NOTE:  For the above algorithm to work "guaranteed", the digits 0-9 must ALWAYS be valid characaters.  That 
//    is why digits 0-9 are always in the valid char set for classes derived from CTreeMap -- see SetValidChars(). 
//
//    ARGS:       pParent  -- [in] ensure name is unique among this node's children (if NULL, uniqueness not checked). 
//                base     -- [in] base name; [out] base modified so that it is valid & unique 
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, if there is insufficient memory for string manipulations. 
//
VOID CTreeMap::GenerateName( CTreeMap::CTreeNode* pParent, CString& base ) const 
{
   ASSERT_VALID( this );                                    // validate tree map

   if ( IsValidName( base ) &&                              // if base name is already OK, do nothing. 
        IsUniqueName( pParent, base ) ) 
      return;

   CString strTemp;                                         // truncate base name if too long; remove invalid chars  
   base = base.Left( m_maxNameLen );
   strTemp = base.SpanIncluding( m_validChars ); 
   while ( strTemp != base ) 
   {
      int i = strTemp.GetLength() + 1;
      if ( i == base.GetLength() )                          // the bad char is the last one in base:  we must handle 
         base = strTemp;                                    // this special case b/c MFC 6.0's CString::Mid(iFirst) 
      else                                                  // ASSERTs if iFirst is beyond end of string. ARGH! 
         base = strTemp + base.Mid( i );

      strTemp = base.SpanIncluding( m_validChars );
   }

   if ( base.IsEmpty() ) base = _T('0');                    // if base name empty, assign a default name -- note that 
                                                            // digit 0 is always a valid char!!
   USHORT i = 0;
   strTemp = base;                                          // append "<N>" to valid base name for <N> = "1", etc. 
   while( !IsUniqueName( pParent, strTemp ) )               // -- stopping once modified name is unique among children 
   {                                                        // of specified parent. 
      ++i;
      strTemp.Format( "%d", i );
      strTemp = 
         base.Left( m_maxNameLen - strTemp.GetLength() )    // may have to strip off characters to make room for suffix 
         + strTemp;
   } 
   base = strTemp;

   return;
}


//=== CleanupDependencies ============================================================================================= 
// 
//    This method was added as a HACK to ensure there are no nodes in the treemap that are "locked" when they should 
//    not be.  Since it traverses the entire treemap twice, it should only be called rarely.  Currently, we call it in 
//    Serialize() to cleanup any bad locks when a treemap is deserialized.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CTreeMap::CleanupDependencies()
{
   // pass 1:  clear all lock counts
   CTreeNode* pNode = NULL;
   GetNextNode( pNode );
   while( pNode != NULL )
   {
      pNode->wLocks = 0;
      GetNextNode( pNode );
   }

   // pass 2: update dependencies on all objects in treemap, which should fix all lock count problems
   CWordArray wArEmpty;
   pNode = NULL;
   GetNextNode( pNode );
   while( pNode != NULL )
   {
      UpdateDependencies( pNode->key, wArEmpty );
      GetNextNode( pNode );
   }
}

//=== LockDependencies ================================================================================================ 
// 
//    Query data object at specified node for a list of objects upon which it depends.  Then lock or unlock each obj 
//    in that list.  Each object in the dependency list must exist (otherwise, ASSERTs in DEBUG release).
//
//    ARGS:       pNode -- [in] lock(unlock) the nodes upon which this node depends 
//                bLock -- [in] lock if TRUE; unlock if FALSE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, if there is insufficient memory to grow the key arrays that contain a data 
//                object's dependencies
//
VOID CTreeMap::LockDependencies(CTreeNode* pNode, const BOOL bLock)
{
   CWordArray wArKeys; 
   pNode->pData->GetDependencies( wArKeys );                      // query for current dependencies...
   if ( wArKeys.GetSize() != 0 ) 
      LockNodes( wArKeys, ((bLock) ? pNode->key : TM_NOKEY) );    // ...and lock or unlock them
}


//=== LockNodes ======================================================================================================= 
//
//    Increment/decrement the lock count on each node/object whose key is contained in the array argument.  The act of 
//    locking a node indicates that another data object in this map is dependent on the data object at this node. 
//    Nodes with nonzero lock counts may not normally be deleted from the map (except when map itself is destroyed).
//
//    "Mutual locks" (A locks B, B locks A) and "descendant locks" (obj A locks an obj in its subtree) are considered 
//    illegal because they prevent removal of an object that would otherwise be removable.  LockNodes() ASSERTs in 
//    DEBUG version if an illegal lock is attempted; in release version, it merely ignores the lock attempt, which may
//    lead to unexpected behavior.
//
//    ARGS:       wArKeys  -- [in] array of map keys of the nodes to be locked or unlocked; ASSERTS in DEBUG release 
//                            if any key is invalid.
//                depKey   -- [in] if TM_NOKEY, unlock nodes; otherwise, this identifies the dependent node that is 
//                            creating the locks. 
//    RETURNS:    NONE.
// 
//    THROWS:     CMemoryException, in the **locking** case only, if insufficient memory to grow the key array that 
//                contains dependencies of an object to be locked (to prevent "mutual locks").  In this situation, 
//                we remove any locks that were made prior to the exception occurring.
//
VOID CTreeMap::LockNodes( const CWordArray& wArKeys, const WORD depKey )
{
   int i;

   if ( depKey == TM_NOKEY )                                      // UNLOCK each node in the key array
   {
      for( i = 0; i < wArKeys.GetSize(); i++ )
      {
         CTreeNode* pNode = GetNodeAt( wArKeys[i] );
         ASSERT( pNode != NULL );
         if ( pNode->wLocks > 0 ) 
            --(pNode->wLocks);
      }
      return;
   }                                                              // otherwise, LOCK:

   CTreeNode* pDepNode = GetNodeAt( depKey );                     // get ptr to node containing dependent data obj 
   ASSERT( pDepNode != NULL );

   CWordArray wArDeps;                                            // dependencies of an obj to be locked

   for( i = 0; i < wArKeys.GetSize(); i++ )                       // for each node/object to be locked:
   {
      BOOL bIllegalLock = FALSE;
      CTreeNode* pNode = GetNodeAt( wArKeys[i] ); 
      ASSERT( pNode != NULL );

      try                                                         //    ...get this object's dependencies so we can 
      {                                                           //    check for an illegal "mutual" lock. 
         pNode->pData->GetDependencies( wArDeps ); 
      }
      catch( CMemoryException* e )                                //    ...if a memory exception occurs while doing 
      {                                                           //    so, undo any locks we've made so far before 
         UNREFERENCED_PARAMETER(e);                               //    forwarding the exception.
         for( int k = 0; k < i-1; k++ )
         {
            pNode = GetNodeAt( wArKeys[i] );
            --(pNode->wLocks);
         }

         throw;
      }

      for( int j = 0; j < wArDeps.GetSize(); j++ )                //    ...prevent "mutual" lock
      {
         if ( wArDeps[j] == depKey )
         {
            bIllegalLock = TRUE;
            break;
         }
      }

      if ( !bIllegalLock )                                        //    ...prevent "descendant" lock
         bIllegalLock = 
            DoesContain( (POSITION)pDepNode, wArKeys[i] );

      if ( bIllegalLock )                                         //    ...if legal, then lock the node/object
         ASSERT( FALSE );
      else if ( pNode->wLocks < 65535 ) 
         ++(pNode->wLocks);
   }
}



//=== ConstructData [overridable] ===================================================================================== 
// 
//    Construct the data object that is associated with a new tree node.
//
//    !!!IMPORTANT!!!  This base class version can only construct a CTreeObj.  See file header for explanation on how 
//    to override this method to handle other data classes.
//
//    ARGS:       name  -- [in] the name assigned to the data object. 
//                type  -- [in] the object's "abstract" data type. 
//                flags -- [in] the object's state flags. 
//
//    RETURNS:    a pointer to the data object constructed (cast to CTreeObj*); NULL upon failure. 
//
//    THROWS:     CMemoryException if unable to allocate the new CTreeObj. 
//
CTreeObj* CTreeMap::ConstructData( LPCTSTR name, const WORD type, const WORD flags )
{
   CTreeObj* pObj = new CTreeObj;
   pObj->Initialize( name, type, flags ); 
   return( pObj );
}


//=== CopyData [overridable] ========================================================================================== 
// 
//    Construct a distinct copy of the specified data object.
//
//    !!!IMPORTANT!!!  This base class version can only copy-construct a CTreeObj.  See file header for explanation on 
//    how to override this method to handle other data classes.
//    
//    ARGS:       pSrc  -- [in] ptr to the data object to be copied. 
//
//    RETURNS:    a pointer to the data object constructed (cast to CTreeObj*); NULL upon failure. 
//
//    THROWS:     CMemoryException if unable to allocate the new CTreeObj. 
//
CTreeObj* CTreeMap::CopyData( const CTreeObj* pSrc )
{
   CTreeObj* pObj = new CTreeObj;
   pObj->Copy( pSrc );
   return( pObj );
}


