//=====================================================================================================================
//
// cxdoc.h : Declaration of class CCxDoc.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXDOC_H__INCLUDED_)
#define CXDOC_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cxtreemap.h"                       // CCxTreeMap: the Maestro object tree-map collection
#include "cxobj_ifc.h"                       // Maestro object interface:  common constants and other defines
#include "cxsettings.h"                      // CCxSettings: the Maestro application settings object



//=====================================================================================================================
// Declaration of class CCxDoc
//=====================================================================================================================
//
class CCxDoc : public CDocument
{
   DECLARE_SERIAL(CCxDoc)

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int CURRVERSION = 7;         // for versioning support in Serialize()


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
protected:
   CCxSettings          m_settings;          // application-level settings to persist with document
   CCxTreeMap           m_Objects;           // the self-keying collection of MAESTRO objects:  each obj is accessed
                                             // directly via its unique key.  in addition, each object is part the
                                             // MAESTRO object tree, a hierarchical tree structure for organizing the
                                             // different classes of objects within constraints imposed by the document

   WORD                 m_objTreeRoot;       // key to the root object for the MAESTRO object tree


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxDoc& operator=( const CCxDoc& );                   // decl only:  there is no operator= for this class
   CCxDoc( const CCxDoc& );                              // decl only:  there is no copy constructor for this class

public:
   CCxDoc();                                             // default constructor required for dynamic object creation
   virtual ~CCxDoc();



//=====================================================================================================================
// GENERAL DOCUMENT OPERATIONS
//=====================================================================================================================
public:
   virtual BOOL OnNewDocument();                         // handle opening a new experiment
   virtual void DeleteContents();                        // release memory resources allocated for document
   virtual void Serialize( CArchive& ar );               // handle serialization of experiment document


//=====================================================================================================================
// OPERATIONS ON CNTRLX OBJECT TREES
//=====================================================================================================================
public:
   WORD GetBaseObj( const WORD type = 0 ) const;         // get unique key of the MAESTRO object tree's root node (0),
                                                         // or one of the major subtrees (type specified)

   CCxSettings* GetSettings()                            // expose application settings object
   {
      return( &m_settings );
   }

   // get keys of all trials in specified trial set or subset, in order of appearance
   VOID GetTrialKeysIn(const WORD wParent, CWordArray& wArKeys) const;

   // checks if specified trial set contains one or more non-empty trial subsets
   BOOL HasTrialSubsets(const WORD wSet) const;

   // is trial set empty (or only contains empty trial subsets)
   BOOL IsTrialSetEmpty(const WORD wSet) const;
   // excise any trial sets that are empty (or contain only empty trial subsets)
   VOID RemoveEmptyTrialSets();

   // get key of the predefined "CHAIR" target or the predefined "default" channel configuration
   WORD GetChairTarget() const; 
   WORD GetDefaultChannelConfig() const;
   // get key of the OBSOLETE predefined "OKNDRUM" target (when opening docs w/schema version < 2 only)
   WORD GetOKNDrumTarget() const;

   WORD InsertObj( const WORD key, const WORD type,      // insert new obj under specified parent, either appending it
      LPCTSTR name = NULL,                               // or inserting it before a particular sibling object
      const WORD bef = CX_NULLOBJ_KEY );
   WORD DuplicateObj( const WORD kSrc );                 // insert duplicate of specified object under its parent
   BOOL MoveObj( const WORD src, const WORD dst,         // move or copy specified object
       const WORD bef, const BOOL bCopy );

   int RemoveObj( const WORD objKey )                    // remove specified object & all its descendants, if possible
   {                                                     //
      int i = m_Objects.RemoveTree( objKey, TRUE );      //
      if ( i > -1 ) SetModifiedFlag();                   //
      return( i );                                       //
   }                                                     //
   VOID ClearUserObj()                                   // removes ALL user-defined objects from the document,
   {                                                     // essentially resetting it to the "new document" state
      DestroyObjTree();
      InitObjTree();
      SetModifiedFlag();
   }

   VOID GetObjName( const WORD key, CString& s ) const   // get name of specified obj (MUST exist in tree-map)
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      s = pObj->Name();
   }
   const CString& GetObjName( const WORD key ) const     // this version gives direct access to stored string!
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( pObj->Name() );
   }

   VOID GetFullObjName( const WORD key,                  // "pathname" of obj; includes ancestor names with forward
                        CString& s ) const;              // slash delimiters
   BOOL SetObjName( const WORD key, const CString& s );  // rename specified obj (MUST exist & MUST be user-defined)

   BOOL CopySelectedObjectsFromDocument(CCxDoc* pSrcDoc, // copy selected objects from a different experiment doc to
         CWordArray& wArKeys );                          // this one

   VOID UpdateObjDep( const WORD key,                    // update dependencies of specified object
                      const CWordArray& wArOld )         //
   {                                                     //
      m_Objects.UpdateDependencies( key, wArOld );       //
   }
   BOOL IsLockedObj( const WORD key )                    // is specified object locked (another obj depends on it)?
   {                                                     //
      return( m_Objects.IsLocked( key ) );               //
   }

   BOOL ObjExists( const WORD key ) const                // does key identify a valid MAESTRO object?
   {
      return( m_Objects.NodeExists( key ) );
   }
   WORD GetParentObj( const WORD key ) const             // get key of specified object's parent, if any
   {
      return( m_Objects.GetParentKey( key ) );
   }
   BOOL IsAncestorObj( const WORD w,                     // is one object a direct ancestor of another obj?
                       const WORD child ) const
   {
      return( m_Objects.DoesContain( w, child ) );
   }
   BOOL IsUserObj( const WORD key ) const                // is this object defined by user? (obj MUST exist)
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( !(pObj->Flags() & CX_OBJFLAGS) );
   }
   BOOL IsCollectionObj( const WORD key ) const          // is this a MAESTRO collection object (user- or pre-defined)?
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( BOOL(pObj->Flags() & CX_ISSETOBJ) );
   }
   BOOL AcceptsSubObj( const WORD key,                   // can specified obj be a parent to the specified obj type?
                       const WORD typ ) const
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( ((pObj->Flags() & CX_NOINSERT) == 0) &&
              ValidChildType( pObj->DataType(), typ ) );
   }
   WORD GetObjType( const WORD key ) const               // retrieve MAESTRO object's native type (obj MUST exist)
   {
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( pObj->DataType() );
   }
   CTreeObj* GetObject( const WORD key ) const           // obtain ptr to existing MAESTRO obj (caller must recast to
   {                                                     //    appropriate MAESTRO data class!)
      CTreeObj* pObj;
      VERIFY( m_Objects.GetNode( key, pObj ) );
      return( pObj );
   }

   POSITION GetFirstChildObj( const WORD key ) const     // to traverse immediate children of an object...
   {
      return( m_Objects.GetFirstChild( key ) );
   }
   VOID GetNextChildObj( POSITION& pos, WORD& key,
                         CTreeObj*& pObj ) const
   {
      m_Objects.GetNextChild( pos, key, pObj );
   }

   POSITION InitTraverseObj( const WORD key ) const      // to traverse a MAESTRO object tree in standard order...
   {
      return( m_Objects.InitTraverse( key ) );
   }
   VOID TraverseObj( POSITION& pos, int& dt, WORD& key,
                     CTreeObj*& pObj ) const
   {
      m_Objects.Traverse( pos, dt, key, pObj );
   }

   VOID PrepareKeyChain( CDWordArray& dwArKeys,          // prepare chain of keys from a recognized major subtree node
                  WORD wBaseType, WORD wLastKey ) const; // to a particular node within that subtree
   static int CALLBACK TreeInfoCB( DWORD dwKey,          // callback provides child info for any node in object tree
      CStringArray* pArLbls, CDWordArray* pArKeys,
      CByteArray* pArHasKids, LPARAM lParam );


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
public:
#ifdef _DEBUG
   virtual void Dump( CDumpContext& dc ) const;          // dump document contents
   virtual void AssertValid() const;                     // validate the document object
#endif



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   VOID InitObjTree();                                   // create initial, predefined state of MAESTRO object tree
   VOID DestroyObjTree();                                // free *all* resources allocated in MAESTRO object tree

   BOOL                                                  // can specified MAESTRO src object be copied/moved under the
   IsValidMove( const WORD src, const WORD dst,          //    specified destination?
                const BOOL bCopy, BOOL& bKids ) const;
   BOOL IsValidType( const WORD t ) const                // is obj type recognized by MAESTRO?
   {
      return( (t>=CX_FIRST_TYP) && (t<=CX_LAST_TYP) );
   }
   BOOL ValidChildType( const WORD dstType,              // can specified obj "parent" the other? -- used to restrict
                        const WORD type ) const;         //    the content/structure of the MAESTRO object tree
   LPCTSTR GetObjBasename( const WORD type ) const;      // provides suggested label for specified MAESTRO obj type

   // migrates version 3 experiment doc to version 4, removing all references to Fiber* and REDLED* targets
   BOOL CCxDoc::MigrateToVersion4();
   // migrates to version 7, removing all XYScope targets and the trials and stimulus runs that used them.
   BOOL CCxDoc::MigrateToVersion7();
   // helper method for MigrateToVersion7()
   BOOL CCxDoc::DoesTrialOrRunUseXYScope(CTreeObj* pTreeObj) const;
};

#endif // !defined(CXDOC_H__INCLUDED_)
