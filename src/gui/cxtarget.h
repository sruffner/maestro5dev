//=====================================================================================================================
//
// cxtarget.h : Declaration of class CCxTarget.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXTARGET_H__INCLUDED_)
#define CXTARGET_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "treemap.h"                         // the CTreeMap/CTreeObj framework
#include "cxobj_ifc.h"                       // MAESTRO object "interface" defines, including target-specific defines


//=====================================================================================================================
// Declaration of class CCxTarget
//=====================================================================================================================
//
class CCxTarget : public CTreeObj
{
   DECLARE_SERIAL( CCxTarget )

   friend class CCxTreeMap;                  // so that MAESTRO tree map can control naming of its data objects

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
public:
   static LPCTSTR RMVTYPENAMES[RMV_NUMTGTTYPES];      // GUI names for the RMVideo target types
   static LPCTSTR RMVSHAPENAMES[RMV_NUMTGTSHAPES];    // GUI names for the possible RMVideo aperture shapes

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
protected:
   // modifiable target parameters (dynamically allocated during Initialize() or Copy())
   PVOID m_pvParms;



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTarget( const CCxTarget& src );                    // no copy constructor defined
   CCxTarget& operator=( const CCxTarget& src );         // no assignment operator defined

protected:
   CCxTarget() { m_pvParms = NULL; }                     // constructor required for dyn object creation mechanism
   ~CCxTarget();                                         // destroy target object -- freeing any allocated memory

   VOID Initialize(LPCTSTR s,const WORD t,const WORD f); // initialize target object after default construction
   VOID Copy( const CTreeObj* pSrc );                    // make THIS target object a copy of the specified target

public:
   BOOL CopyRemoteObj(CTreeObj* pSrc,                    // copy the defn of a src target from a different treemap
         const CWordToWordMap& depKeyMap);


//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================
public:
   // return TRUE if target has modifiable parameters. NOTE: We still recognize the deprecated XYScope target bc
   // we need to be able to deserialize pre-V5.0 experiment documents containing XYScope target objects.
   BOOL IsModifiable() const
   {
      return( m_type == CX_XYTARG || m_type == CX_RMVTARG );
   }

   BOOL CanRemove() const                                // prevent removal of "predefined" targets
   { return( (m_flags & CX_ISPREDEF) == 0 ); }           //


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL GetParams( PU_TGPARMS pTgt ) const;              // retrieve target's current modifiable parameters
   BOOL SetParams( PU_TGPARMS pTgt, BOOL& bChanged );    // update target's parameters, with auto-correct

   void Serialize( CArchive& ar );                       // for reading/writing target parameters from/to disk file

private:
   static VOID ConvertOldFBVideoToRMVideo(               // translates old FB video tgt defn to a similar RMVideo tgt
      PFBPARMS pFB, PRMVTGTDEF pRMV );                   // (RMVideo introduced in Maestro v2.0)

//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
public:
#ifdef _DEBUG
   void Dump( CDumpContext& dc ) const;                  // dump the target's name, data type, and parameters (if any)
   void AssertValid() const;                             // validate the target object
#endif


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
protected:
   VOID AssignDefaultValues();                           // assign default values to modifiable target parameters

   // checks for recognized Maestro target type. NOTE that obsolete target types CX_FIBER1...CX_OKNDRUM and CX_XYTARG 
   // are still considered valid here -- because this test is used during object deserialization, and we must be able 
   // to open old experiment documents in order to migrate them to the current version...
   BOOL ValidTargetType( const WORD t ) const
   {
      return( (t==CX_XYTARG) || (t==CX_RMVTARG) ||
              ((t >= CX_CHAIR) && (t <= CX_OKNDRUM)) );
   }
};


#endif   // !defined(CXTARGET_H__INCLUDED_)
