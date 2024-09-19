//===================================================================================================================== 
//
// treemap.h : Declaration of classes CTreeObj and CTreeMap.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(TREEMAP_H__INCLUDED_)
#define TREEMAP_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "afxtempl.h"                     // for CMap template 

//===================================================================================================================== 
// PUBLIC CONSTANTS/TYPEDEFS/GLOBALS/MACROS 
//===================================================================================================================== 
typedef CMap<WORD,WORD,WORD,WORD> CWordToWordMap;

const int TM_NOKEY         = 0;           // CTreeMap reserves the zero key as an error signal. 


//===================================================================================================================== 
// Declaration of class CTreeObj  
//===================================================================================================================== 
//
class CTreeObj : public CObject 
{
   DECLARE_SERIAL( CTreeObj )

   friend class CTreeMap;                 // so that tree map can control naming of tree objects 

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected: 
   CString     m_name;                    // object's name (unique among its siblings in the tree map) 
   WORD        m_type;                    // object data type (derived classes govern implementation of data type) 
   WORD        m_flags;                   // object state flags (for use by derived classes)



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CTreeObj( const CTreeObj& src );                      // copy constructor is NOT defined
   CTreeObj& operator=( const CTreeObj& src );           // assignment op is NOT defined

protected:
   CTreeObj()                                            // constructor required for dyn object creation mechanism 
   { 
      m_name = _T("?"); 
      m_type = 0; 
      m_flags = 0;
   }
   virtual ~CTreeObj() {}                                // free any memory dynamically allocated to tree object 

   virtual VOID                                          // initialize tree object after default construction
   Initialize( LPCTSTR s, const WORD t, const WORD f )   //
   {
      m_name = s; 
      if ( m_name.IsEmpty() ) m_name = _T("?");
      m_type = t; 
      m_flags = f;
   }
   virtual VOID Copy( const CTreeObj* pSrc )             // make THIS tree object a copy of the specified object
   {                                                     //
      ASSERT_VALID( pSrc ); 
      m_name = pSrc->m_name; 
      m_type = pSrc->m_type; 
      m_flags = pSrc->m_flags; 
   }

public:
   virtual BOOL CopyRemoteObj(                           // copy the defn of a src obj from a different treemap:
         CTreeObj* pSrc,                                 //    the src obj, which does NOT reside in this treemap
         const CWordToWordMap& depKeyMap)                //    maps keys of src obj dependencies to appropriate keys
   {                                                     //    in the treemap that holds this object
      return( FALSE );                                   //    return TRUE if successful
   }


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   const CString& Name() const { return( m_name ); }     // access object's name as read-only string
   WORD DataType() const { return( m_type ); }           // access object's data type 
   WORD Flags() const { return( m_flags ); }             // access object's state flags 

   virtual BOOL CanRemove() const                        // override to implement "read-only", nonremovable objects 
   { return( TRUE ); }                                   //

   virtual VOID                                          // override if derived class stores other obj keys.  fill    
   GetDependencies( CWordArray& wArKeys ) const          // array with these keys, which represent dependencies on 
   { wArKeys.RemoveAll(); }                              // other objects within the same containing tree-map


//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 
public:
   virtual WORD SetFlags( const WORD newFlags )          // modify object's state flags; override to restrict access 
   {
      WORD old = m_flags;
      m_flags = newFlags;
      return( old );
   }

   void Serialize( CArchive& ar);                        // serialization of tree object  



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump the tree object
   void AssertValid() const;                             // validate the tree object  
#endif

};



//===================================================================================================================== 
// Declaration of class CTreeMap 
//===================================================================================================================== 
//
class CTreeMap : public CObject 
{
   DECLARE_SERIAL( CTreeMap )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int TM_MAXOBJNAMELEN;        // max data object name length supported by CTreeMap 
   static const int TM_MIN_MAXOBJNAMELEN;    // smallest allowed value for the max name length 
   static const int TM_HASHSIZE;             // number of buckets in CTreeMap's hash table
   static const int TM_HASHEXP;              // N such that 2^N = number of buckets in CTreeMap's hash table

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CString     m_validChars;                 // the set of chars that can appear in a data object's name 
   int         m_maxNameLen;                 // maximum length of a data object's name 

protected: 
   struct CTreeNode                          // representation of a single tree node:
   {
      WORD        key;                       //    the key value assigned to node for lookup access 
      CTreeObj*   pData;                     //    the data object stored in node, including name & obj "data type"
      WORD        wLocks;                    //    #locks on data obj ("lock" := another obj in map depends on it) 
      
      CTreeNode*  pParent;                   //    the tree links 
      CTreeNode*  pFirstChild;
      CTreeNode*  pPrevSib;
      CTreeNode*  pNextSib;

      CTreeNode*  pNext;                     //    points to next node in the hash table bucket's singly-linked list
   };

   struct CBucket                            // hash table is an array of buckets, each containing...
   {
      int         count;                     //    # of objects currently stored in this bucket 
      CTreeNode*  pHead;                     //    head of singly-linked list of objects in this bucket 
   };

   CBucket*    m_pHashTable;                 // the map's hash table
   int         m_nCount;                     // total # of objects currently stored in map 
   CTreeNode*  m_pFreeNodes;                 // head of linked list: "free pool" of allocated but unused tree nodes 
   int         m_nFreeCount;                 // # of nodes in free pool 
   int         m_nAllocSize;                 // # of tree nodes to allocate when the free pool is empty



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CTreeMap& operator=( const CTreeMap& src );           // decl only:  there is no operator= for this class
   CTreeMap( const CTreeMap& src );                      // decl only:  there is no copy constructor for this class

public:
   CTreeMap();                                           // constructor required for dyn object creation mechanism 
   ~CTreeMap();                                          // destroy map -- frees memory devoted to map overhead and 
                                                         //    AND to all tree objects stored in the map! 



//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   int GetCount() const { return( m_nCount ); }          // number of elements in map 
   BOOL IsEmpty() const { return(BOOL(m_nCount == 0)); } // is the map empty? 
   int GetAllocSize() const { return( m_nAllocSize ); }  // get current allocation block size 



//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 
public:
   BOOL SetValidChars( LPCTSTR s );                      // change valid char set for obj names (map must be empty!)
   BOOL SetMaxNameLength( int n );                       // change max data obj name length (map must be empty!)

   int SetAllocSize( int newSize );                      // change the allocation block size 

   BOOL NodeExists( const WORD key ) const               // does key point to an existing node in map?
   {                                                     //
      return( GetNodeAt( key ) != NULL );                //
   }                                                     //

   BOOL GetNode( const WORD key,                         // retrieve the data element stored in a node of the map 
                 CTreeObj*& pData ) const;               //  
   BOOL GetNode( const POSITION pos,                     // 
                 CTreeObj*& pData ) const;               //

   WORD GetParentKey( const WORD key ) const             // retrieve key of specified node's parent 
   { 
      return( GetParentKey((POSITION)GetNodeAt(key)) ); 
   } 
   WORD GetParentKey( const POSITION pos ) const 
   { 
      CTreeNode* pN = (CTreeNode*) pos; 
      if( pN != NULL )
      {
         ASSERT( AfxIsValidAddress(pN,sizeof(CTreeNode)) ); 
         if( pN->pParent != NULL ) 
            return( (pN->pParent)->key ); 
      }
      return( TM_NOKEY );
   } 

   POSITION GetParentPos( const POSITION pos ) const     // to back up one level in an iteration
   {
      CTreeNode* pN = (CTreeNode*) pos; 
      ASSERT( AfxIsValidAddress(pN,sizeof(CTreeNode)) ); 
      return( (POSITION) (pN->pParent) );
   }

   BOOL HasChildren( const WORD key ) const              // does specified node have any children?
   {                                                     //
      return( HasChildren((POSITION)GetNodeAt(key)) );   //
   }                                                     //
   BOOL HasChildren( const POSITION pos ) const          //
   {                                                     //
      CTreeNode* pN = (CTreeNode*) pos;                  //
      ASSERT( AfxIsValidAddress(pN,sizeof(CTreeNode)) ); //
      return( BOOL(pN->pFirstChild != NULL) );           //
   }                                                     //

   BOOL DoesContain( const POSITION basePos,             // does the tree branch contain the specified node?
                     const WORD test ) const;            //
   BOOL DoesContain( const WORD base,                    // 
                     const WORD test ) const             //
   {                                                     //
      CTreeNode* pBase = GetNodeAt( base );              //
      return( DoesContain( (POSITION)pBase, test ) );    //
   }                                                     //



   POSITION GetFirstChild( const WORD key ) const;       // to iterate the children of a specified parent node 
   VOID GetNextChild( POSITION& pos, WORD& key,          //
                      CTreeObj*& pData ) const;          //

   POSITION InitTraverse( const WORD key ) const;        // to iterate tree nodes in standard tree traversal order 
   VOID Traverse( POSITION& pos, int& delt, WORD& key,   // 
                  CTreeObj*& pData ) const;              //

   VOID UpdateDependencies( const WORD key,              // update dependencies for recently modified data obj 
                            const CWordArray& wArOld );  //
      
   BOOL IsLocked( const WORD key ) const                 // is this node's lock count > 0?
   {                                                     //
      return( IsLocked((POSITION)GetNodeAt(key)) );      //
   }                                                     //
   BOOL IsLocked( const POSITION pos ) const             //
   {                                                     //
      CTreeNode* pN = (CTreeNode*) pos;                  //
      ASSERT( AfxIsValidAddress(pN,sizeof(CTreeNode)) ); //
      return( pN->wLocks > 0 );                          //
   }                                                     //

   WORD InsertNode( const WORD parent, const WORD type,  // insert a single node into the map, suggesting a name for it 
      const WORD flags, CString& name,
      const WORD beforeSib = TM_NOKEY ); 
   BOOL RenameNode( const WORD key, const CString& s );  // rename an existing node in the map
   WORD CopyTree( const WORD src );                      // copy tree branch as a separate tree in map 
   VOID MoveTree( const WORD src, const WORD dst,        // move tree branch (or leaf node) within the tree map
      const WORD beforeSib = TM_NOKEY );
   int RemoveTree( const POSITION basePos,               // remove tree branch (or leaf node) 
                   const BOOL bCheck );                  //
   int RemoveTree( const WORD baseKey,                   //
                   const BOOL bCheck )                   //  
   {
      CTreeNode* pRoot = GetNodeAt( baseKey );
      return( RemoveTree( (POSITION)pRoot, bCheck ) );
   }
   VOID RemoveAll();                                     // destroys the entire tree map; all memory is released

   void Serialize( CArchive& ar);                        // serialization of tree-map 



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump the tree map (depth>0 to dump individual nodes) 
   void AssertValid() const;                             // validate the tree map 
#endif



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected: 
   VOID InitHashTable();                                 // allocate hash table. Map must be empty when called.
   int HashKey( WORD key ) const;                        // transforms key value to hash table bucket # 
   BOOL IsRoot( CTreeNode* pNode ) const                 // is specified node a root node?
   {
      return( BOOL(pNode->pParent == NULL) );
   }
   CTreeNode* NewNode( const WORD key = TM_NOKEY );      // put a new root node in tree map.  node's key is usually 
                                                         // self-generated, unless a valid & unused key is specified. 
   VOID FreeNode( CTreeNode* pNode );                    // move specified node to free pool; adjust free pool
   CTreeNode* GetNodeAt( const WORD key ) const;         // search map by key 
   VOID GetNextNode( CTreeNode*& pNextNode ) const;      // for traversing all nodes in hash-table order 
   VOID GetNextTreeRoot( CTreeNode*& pNextRoot ) const;  // for traversing all root nodes in the tree-map 
   VOID StdTrav( CTreeNode*& pNext, int& delt ) const;   // for traversing tree-map in standard tree traversal order 
   VOID ConnectTree( CTreeNode* pNode, CTreeNode* pDst,  // connect specified node as child of specified parent, 
      CTreeNode* pBeforeSib = NULL );                    // before the specified sibling node
   VOID DisconnectTree( CTreeNode* pNode );              // disconnect tree branch rooted at specified node       

   int NumberInBranch( CTreeNode* pNode ) const;         // how many nodes in tree branch rooted at specified node?

   BOOL IsValidName( const CString& name ) const;        // does string represent a valid data object name? 
   BOOL IsUniqueName( CTreeNode* pParent,                // FALSE if specified name is already assigned to a child of 
                      const CString& name ) const;       //    of the specified node 
   VOID GenerateName( CTreeNode* pParent,                // modify base name so that it is valid & unique among 
                      CString& base ) const;             //    the children of specified node 

   VOID CleanupDependencies();                           // recomputes lock counts across entire treemap
   VOID LockDependencies( CTreeNode* pNode,              // lock/unlock specified node's dependencies, if any
                          const BOOL bLock );            // 
   VOID LockNodes( const CWordArray& wArKeys,            // this method does the actual locking/unlocking and prevents 
                   const WORD depKey );                  // illegal locks

   virtual CTreeObj* ConstructData(                      // construct a data object with assigned name, type & flags 
      LPCTSTR name, const WORD type, const WORD flags );
   virtual CTreeObj* CopyData( const CTreeObj* pSrc );   // construct a distinct copy of specified data object

};

#endif // !defined(TREEMAP_H__INCLUDED_)


