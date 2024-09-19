//===================================================================================================================== 
//
// cxtreemap.h : Declaration of class CCxTreeMap.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXTREEMAP_H__INCLUDED_)
#define CXTREEMAP_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "cxobj_ifc.h"                       // MAESTRO object interface:  common constants and other defines 
#include "treemap.h"                         // the CTreeMap/CTreeObj framework


//===================================================================================================================== 
// Declaration of class CCxSet 
//===================================================================================================================== 
//
class CCxSet : public CTreeObj 
{
   DECLARE_SERIAL( CCxSet )

   friend class CCxTreeMap;               // so that MAESTRO tree map can control naming of these objects

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxSet( const CCxSet& src );                          // copy constructor is NOT defined
   CCxSet& operator=( const CCxSet& src );               // assignment op is NOT defined

protected:
   CCxSet() { m_flags |= CX_ISSETOBJ; }                  // constructor required for dyn object creation mechanism 
   ~CCxSet() {}                                          // free any memory dynamically allocated to tree object 

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f)  // initialize set after default construction
   {                                                     //
      CTreeObj::Initialize( s, t, f );
      m_flags |= CX_ISSETOBJ;
   }
   VOID Copy( const CTreeObj* pSrc )                     // make THIS set a copy of the specified set
   {                                                     //
      ASSERT_VALID( (CCxSet*)pSrc ); 
      CTreeObj::Copy( pSrc ); 
   }



//===================================================================================================================== 
// ATTRIBUTES, OPERATIONS
//===================================================================================================================== 
public:
   BOOL CanRemove() const                                // prevent removal of "predefined" sets 
   { return( (m_flags & CX_ISPREDEF) == 0 ); }           //

   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a src obj from a different treemap 
         const CWordToWordMap& depKeyMap) 
   {
      return( TRUE );                                    // there's nothing to copy!
   }


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void AssertValid() const;                             // validate the MAESTRO collection object -- verify type  
#endif

};



//===================================================================================================================== 
// Declaration of class CCxTreeMap 
//===================================================================================================================== 
//
class CCxTreeMap : public CTreeMap 
{
   DECLARE_SERIAL( CCxTreeMap )

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxTreeMap& operator=( const CCxTreeMap& );           // decl only:  there is no operator= for this class
   CCxTreeMap( const CCxTreeMap& );                      // decl only:  there is no copy constructor for this class

public:
   CCxTreeMap();                                         // constructor required for dyn object creation mechanism 
   ~CCxTreeMap() {}                                      // destroy map -- base class takes care of EVERYTHING!! 



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected: 
   CTreeObj* ConstructData(                              // construct a data object with assigned name, type & flags 
      LPCTSTR name, const WORD type, const WORD flags );
   CTreeObj* CopyData( const CTreeObj* pSrc );           // construct a distinct copy of specified data object

};

#endif // !defined(CXTREEMAP_H__INCLUDED_)


