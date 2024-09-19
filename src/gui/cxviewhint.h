//===================================================================================================================== 
//
// cxviewhint.h : Declaration of class CCxViewHint.
//
// ****** THERE IS CURRENTLY NO IMPLEMENTATION FILE.  ALL FUNCTIONS DEFINED IN CLASS DEFINITION ******
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//    In the Doc/View architecture, the active view can pass a "hint object" (derived from CObject) in the call to 
// CDocument::UpdateAllViews().  Other views can examine this hint to streamline the work they must do (if any) to 
// update themselves IAW the changes initiated by the active view.  The CCxViewHint class encapsulates the hint object 
// used by all CNTRLX views. 
//    Note that it is simply used as a container object and does not support serialization.  All views which process 
// hints should use this object. 
//
// 
// REVISION HISTORY:
// 14apr2000-- Created.  Initial version supports a few simple hints related to the current CNTRLX target list. 
// 14aug2000-- Updated IAW the new CNTRLX object tree design scheme and enhancements to the CNTRLX target tree, which 
//             has replaced the target list view.
// 18sep2000-- Major revision.  Many of the operations on the CNTRLX object trees are similar in nature.  So, rather 
//             than having separate codes for each tree ("deletion from tgt tree", "deletion from trial tree", etc.), 
//             we've added another field that indicates which tree has been changed (this is actually the type of the 
//             tree's root object -- CX_TARGROOT, etc.) and have made the op codes more general ("object deleted", 
//             "object renamed", etc.).
// 07nov2000-- Added more specific hints related to modifications of a CNTRLX trial definition.
// 28nov2000-- Hints added on 07nov2000 were pared down to two simple ones.  As I implemented the trial definition 
//             views, I found that it was too much work to respond efficiently to the more specific hints.
// 22jan2001-- Got rid of the trial-specific modification hint codes entirely.
// 25jan2001-- Replaced the 'm_tree' member that indicated the object tree (CX_TRIALROOT, etc) containing the object. 
//             It is now 'm_type', specifying the type of object(s) affected.
// 18oct2002-- Added new hint codes CXVH_VIDEOSETTINGS and CXVH_FIXREWSETTINGS.  These are sent when video display 
//             settings or fixtion/reward options, resp, have been changed in the CNTRLX application settings object, 
//             CCxSettings.  Note that the CCxSettings object is not part of the CNTRLX object tree, so the m_type and 
//             m_key fields are not applicable for these hint codes...
//
//===================================================================================================================== 


#if !defined(CXVIEWHINT_H__INCLUDED_)
#define CXVIEWHINT_H__INCLUDED_

#include "cxobj_ifc.h"                       // CNTRLX object interface:  common constants and other defines 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//===================================================================================================================== 
// Supported hint codes 
//===================================================================================================================== 
//
const WORD     CXVH_NONE               = 0;              // empty hint -- hint object contains no useful info
const WORD     CXVH_NEWOBJ             = 1;              // one or more objects added to CNTRLX object tree (if single 
                                                         //    object, then its type and key are given)
const WORD     CXVH_MOVOBJ             = 2;              // one or more objects have been moved within CNTRLX obj tree
const WORD     CXVH_NAMOBJ             = 3;              // an existing object has been renamed 
const WORD     CXVH_DELOBJ             = 4;              // one or more objects have been deleted 
const WORD     CXVH_CLRUSR             = 5;              // all user-defined objects have been cleared from tree 
const WORD     CXVH_DSPOBJ             = 6;              // display specified object's defining parameters 
const WORD     CXVH_MODOBJ             = 7;              // object's defining parameters have been modified in some way 

const WORD     CXVH_VIDEOSETTINGS      = 20;             // one or more video display settings have changed
const WORD     CXVH_FIXREWSETTINGS     = 21;             // one or more fixation/reward settings have changed


//===================================================================================================================== 
// Declaration of class CCxViewHint
//===================================================================================================================== 
//
class CCxViewHint : public CObject
{

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
public:
   WORD     m_code;     // identifies type of hint -- what kind of document modification has occurred 
   WORD     m_type;     // type of the CNTRLX object(s) modified.  0 if more than one type. 
   WORD     m_key;      // key ID of modified obj (if obj deleted, key not valid). CX_NULLOBJ_KEY if more than one obj.



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxViewHint& operator=( const CCxViewHint& );         // decl only:  there is no operator= for this class
   CCxViewHint( const CCxViewHint& );                    // decl only:  there is no copy constructor for this class 

public:
   CCxViewHint()                                         // constructs an empty hint (no useful info)
   {
      m_code = CXVH_NONE;
      m_type = 0;
      m_key = CX_NULLOBJ_KEY;
   }
   CCxViewHint( const WORD code,  const WORD type,       // constructs the specified hint  
                const WORD key )
   {
      m_code = code;
      m_type = type;
      m_key = key;
   }
   // use default destructor 



//===================================================================================================================== 
// ALL OTHER OPERATIONS
//===================================================================================================================== 
public:
   void Set( const WORD code, const WORD type,           // modify the hint object
             const WORD key ) 
   {
      m_code = code;
      m_type = type;
      m_key = key;
   }

};


#endif   // !defined(CXVIEWHINT_H__INCLUDED_)
