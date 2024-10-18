//=====================================================================================================================
//
// cxtarget.cpp : Implementation of class CCxTarget, encapsulating a MAESTRO "target object".
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This class encapsulates the definition of a MAESTRO visual stimulus, or "target". It provides a single entity for
// storing the target's name, object data type***, and defining parameters (if any).  It also provides a set of
// operations for accessing and/or modifying this information.
//
// Below are the two categories of targets supported by MAESTRO, based on the actual hardware on which they are
// "realized":
//
//       1) CX_CHAIR. The animal chair is driven by a servo controller using a velocity command signal delivered on a
//          dedicated analog output channel. This is a predefined target; it will have it's CX_ISPREDEF flag set.
//          ****As of Maestro 3.0, this is the only remaining predefined, non-modifiable target supported. The
//          CX_OKNDRUM target was never used in Maestro and was dropped in v1.5. The CX_FIBER* and CX_REDLED* targets
//          were deprecated in v3.0.
//       2) CX_RMVTARG.  For Remote Maestro Video (RMVideo) target.  RMVideo replaced the old VSG2/4 as our framebuffer
//          video solution.  The RMVideo server runs on a separate Linux workstation; MAESTRODRIVER talks to it over
//          a private, dedicated Ethernet connection.  RMVideo implements a wide range of color targets, including
//          XYScope-like targets and all the old VSG targets.  Introduced in Maestro v2.0. Major new target type,
//          RMV_MOVIE, was introduced in Maestro v2.5 to support video playback in TrialMode. RMV_IMAGE target 
//          introduced in v3.3.1 (RMVideo v7) to support display of static images.
//
// [***NOTE:  Please do not confuse "MAESTRO object data type" with "target type", which refers to a specific kind of
// target available on the RMVideo display. All RMVideo target objects have a data type of CX_RMVTARG, but the
// physical type of the target is a modifiable parameter that takes on one of several possible values.  See also the
// MAESTRO object definition file CXOBJ_IFC.H.]
//
// The Big Picture:  Storage of MAESTRO data objects.
// -------------------------------------------------
//
// The user creates experimental protocols within a MAESTRO "experiment document" (CCxDoc) by defining a variety of
// "data objects" and establishing relationships among those objects.  For instance, each MAESTRO "trial" defines the
// trajectories of one or more "targets", which are defined separately.  The trial object also refers to a "channel
// set" object, which contains the list of analog channels that should be sampled during that trial.  Trials, targets,
// and channel sets are examples of "abstract" data classes defined in MAESTRO.
//
// MAESTRO data objects are stored in the MAESTRO object tree, encapsulated by CCxTreeMap.  This "tree map" collection
// stores all the data objects in several different hierarchical trees (the "target tree", "trial tree", and so on).
// We chose this somewhat complex storage scheme in order to organize the different data objects in a logical manner,
// and to provide the potential for storing a large # of objects in a single document yet be able to access any
// individual object rapidly via a unique key value (hence the "map" in "tree map").  CCxTreeMap can store up to
// 65535 different objects, more than enough for our purposes.
//
// CCxTreeMap is derived from the generic CTreeMap class, which handles the low-level implementation details of the
// tree map (see TREEMAP.CPP).  CTreeMap itself handles one base data class, CTreeObj, which merely stores the object's
// name and abstract data type and serves as the starting point for building more complex data classes.  CCxTreeMap
// tailors the behavior of CTreeMap so it can handle all data types present in MAESTRO.  Each MAESTRO data class must
// meet certain constraints in order to build the CNTRLX object trees on top of the CTreeMap/CTreeObj framework; see
// the implementation file for CTreeMap for an explanation of these constraints.  CCxTarget has been designed with
// these constraints in mind.
//
// There is a division of responsibilities among CCxDoc, CCxTreeMap, and the various CTreeObj-derived classes that
// represent the real MAESTRO data objects.  First, the MAESTRO data object classes provide methods for accessing,
// modifying, and validating the actual data which define how the object behaves in a MAESTRO experiment.  CCxTreeMap
// is the "intelligent" storage medium for these objects (leaf nodes in the tree-map) and "collections" of related
// objects. It must be "aware" of all the different types of MAESTRO data objects so that it can construct any given
// object by calling the appropriate constructor.  Furthermore, it controls the naming of the objects, allowing only
// characters from a valid character set (it uses the default char set provided by CTreeMap) and requiring that no two
// sibling objects have the same name.  Finally, of course, it encodes the tree connections among the objects and
// provides methods for adding objects to the trees, removing objects, etc.  However, it does NOT impose any
// restrictions on how objects are added to the tree-map; that is the responsibility of CCxDoc, in coordination with
// its various views.  As mentioned above, CCxDoc uses CCxTreeMap to store a number of different "object trees";
// CCxDoc methods implement the logic for constructing and restricting the exact composition of these object trees (see
// CCxDoc for details).
//
// The CCxTarget class represents the "data class" which handles all targets in MAESTRO.  It is a "multi-typed" data
// class in that there are a number of different target data types all handled by this class, as described in the
// previous section (CX_XYTARG, CX_RMVTARG, etc.).
//
//
// Dynamic allocation of modifiable parameters.
// --------------------------------------------
//
// While CX_CHAIR has no modifiable parameters, RMVideo targets (and the now-depecated XYScope targets) require a 
// variable number of parameters, depending on the hardware platform and the particular type of target: XYScope targets
// required 16-32 bytes of parameter storage, while RMVido targets need up to 136 bytes. We're faced with a design 
// decision -- provide member variables for each possible parameter, or dynamically allocate parameter storage based on
// the target's data type?  We chose a design in between these extremes:
//       -- A single 'void*' member variable, m_pvParms, points to modifiable parameter storage.
//       -- For XYScope targets, this points to an XYPARMS struct, which defines all possible XY target parameters.
//       -- For RMVideo targets, this points to an RMVTGTDEF struct, which defines all possible RMVideo tgt params.
//       -- For non-modifiable targets, m_pvParms is NULL.
// Observe that allocation of parameter storage in this scheme depends entirely on the target's data type. Target
// objects will typically be created by CCxTreeMap in a two-step process.  First, the default CCxTarget constructor is
// invoked to create the target object; this constructor does not allocate parameter storage.  Second, the Initialize()
// method is called to properly initialize the new target object, including param storage allocation if needed.
// Similarly, CCxTreeMap can construct a copy of another target object by first invoking the default constructor, then
// invoking the Copy( pSrc ) method on the new object, where pSrc points to the source target.  Only Initialize() and
// Copy() -- both of which are protected methods intended only for use by CCxTreeMap -- can allocate or reallocate
// the target object's parameter storage.  Allocation is kept outside of the constructor to make exception handling
// less tricky.  The one exception to this rule occurs during deserialization and is handled entirely within this
// class:  When a target object is "deserialized", the MFC serialization framework requires a default constructor
// (taking no arguments) to create an "empty" default object.  We provide a protected default constructor which creates
// an "empty" target.  The Serialize() override reads in the actual data type and allocates param storage accordingly.
//
// ==> Predefined target CX_OKNDRUM no longer supported as of Maestro v1.5.0.
// The OKNDRUM was never used in Maestro, and we decided to remove it for version 1.5.0.  However, because of the way
// in which experiment documents are serialized, all documents existing prior to v1.5.0 include a CCxTarget object that
// represents the OKNDRUM.  CCxDoc::Serialize() takes care of removing it from the object tree map when such documents
// are deserialized, but that only happens AFTER the CCxTarget object is constructed.  Therefore, CCxTarget can still
// create an instance representing target type CX_OKNDRUM -- else, deserialization would always fail for pre-1.5.0
// documents!!
// 
// ==> Predefined "optic bench" targets CX_FIBER* and CX_REDLED* no longer supported as of Maestro v3.0.
// The optic bench targets fell out of use years ago, and it was decided to eliminate support for them in v3.0. However,
// because of the way in which experiment documents are serialized, all documents existing prior to v3.0 include a 
// CCxTarget object for each of these four predefined targets. CCxDoc::Serialize() takes care of removing them from the
// object tree map when such documents are deserialized, but that only happens AFTER the CCxTarget object is constructed.
// Therefore, CCxTarget can still create an instance representing each of these target types -- else, deserialization 
// would always fail for pre-3.0 documents!!
// 
// ==> The XYScope platform has not been supported since Maestro 4.0, and it is removed as of v5.0. However, CCxTarget
// still supports deserializing an XYScope target object (CX_XYTARG) in order to be able to deserialize pre-5.0 
// experiment documents containing such targets. AFTER deserialization, CCxDoc removes any XYScope targets, along with
// any trials and stimulus runs that depend on them).
// 

// REVISION HISTORY:
// 24feb2000-- Created.  Initial layout of the CCxTarget class.  The derived classes will be created later.
// 10mar2000-- Change in strategy.  Rather than deriving classes from CCxTarget for each target category, we'll just
//             have a single target class that modifies its behavior based on the category of target it represents.
//             So we define two different structs to hold the XY scope and framebuffer target parameter sets, and
//             include pointers to these structs as member variables of CCxTarget.  If an instance of CCxTarget
//             represents one of the predefined targets, both of these pointers will be set to NULL.  If it is an
//             XY scope target, the constructor will allocate memory for the XY scope parameter set and assign the
//             appropriate pointer accordingly.  The destructor must then deallocate memory as needed!
// 31mar2000-- Defined constructor, copy constructor, operator=, and destructor -- since CCxTarget requires dynamic
//             memory allocation for the modifiable target types (TGTHW_XYSCOPE, TGTHW_FBVIDEO).
// 07apr2000-- Further wholesale changes.  Latest design has class CCxTgManager managing the currently defined set of
//             CCxTarget's.  Only certain functions in CCxTgManager have access to a target's name, since that class is
//             solely responsible for ensuring that all target names are unique and valid.
// 13apr2000-- Views will have access to currently defined targets via pointers, so they could modify a defined target
//             via *ccxtarg1 = *ccxtarg2.  To prevent this, we removed the operator= for this class.  It is safe for
//             views to use the constructor & copy constructor:  a CCxTarget object constructed in this manner will not
//             be part of the defined target list.  Only CCxTgManager can add targets to the target list!
// 27apr2000-- Added initial versions of the SetParams() and GetParams() member functions.  Calling fcns provide ptrs
//             to both FBPARMS and XYPARMS structs; which struct is used depends on the target's hardware platform.
//             This is an awkward implementation that would become unwieldy if we were to add other modifiable target
//             categories, so might want to modify the design eventually...
// 16may2000-- Added constructor with no args, as this is required for MFC dynamic object creation and serialization
//             mechanisms.
//          -- Made the entire CCxTgManager class a friend.  Restricting friend access to one member fcn of
//             CCxTgManager caused circular reference problems (cxtarget.h "#include'd" cxtgmanager.h, and vice versa).
// 17may2000-- Changed some of the default parameter values in the CCxTarget constructor.
// 11aug2000-- Began thorough rework to integrate CCxTarget with the "CNTRLX object tree" concept.  In this new design
//             scheme, there is no longer a CCxTgManager.  The TGTHW_* constants defining the target platforms have
//             been replaced by CNTRLX object types CX_CHAIR, etc.  These constants are defined in the "interface file"
//             CXOBJ_IFC.H.  [NOTE: Since CCxTarget saves the target platform ID internally, we're actually duplicating
//             storage for the object type...].  Also, target name is controlled outside the target class.
//          -- Tailored the debug support methods Dump() and AssertValid() to the CCxTarget class.
// 12aug2000-- Implemented Serialize() for the first time.
// 18aug2000-- Added HardwareInfo(), which provides a string describing the hardware platform, loaded from the CNTRLX
//             string table resource.  STILL IN WORK...
// 22aug2000-- Finished HardwareInfo().  Minor mod to SetParams().
// 13sep2000-- REWORK.  Now derived from CTreeObj to fit into the CTreeMap/CTreeObj framework.  The CNTRLX-specific
//             tree map class is CCxTreeMap, which must have friend access to CCxTarget...
// 25oct2000-- Mods to bring CCxTarget in line with recent changes in CTreeObj/CTreeMap framework.  Exception handling
//             issues worked out (I think).
// 20dec2000-- Modified SetParams() to auto-correct invalid parameters.  This follows the CCxTrial design model...
// 02jan2001-- Completed mods to SetParams() and Serialize() so that CCxTarget auto-corrects invalid parameters either
//             provided by the user or deserialized from a file.
//          -- Modified HardwareInfo() to use hard-coded strings, rather than string table resources.
// 28mar2001-- HardwareInfo() now just retrieves the h/w platform description.
// 19dec2001-- Added convenience method GetSubType().
// 14feb2002-- Defn of XY target subtype RECTDOT changed -- see CXOBJ_IFC.H.  SetParams() changed accordingly.
// 15feb2002-- Added support for two newer XY subtypes, FLOWFIELD and ORIENTEDBAR.  Both of these have at least one
//             unique parameter compared to the other target subtypes.  We reuse members of the XYPARMS struct to store
//             these parameters.  See CXOBJ_IFC.H for details.
// 07mar2002-- SetParams() modified to set a flag indicating whether or not the target parameters were actually changed
//             by the call.
// 05dec2002-- Adding Import() to support importing XY scope and FB video target definition from a cntrlxUNIX-style
//             ASCII-text definition file.
// 21jul2003-- Adding new XY scope target type NOISYDOTS.  This target type is the same as FCDOTLIFE, but the each
//             dot's direction is offset by a randomly chosen direction on every refresh.  The random direction is
//             limited to the range [-N:N] in whole degrees, where N is specified by XYPARMS.fInnerW...  Since we are
//             adding a new type and not changing the XYPARMS structure in any way for this new type, serialization of
//             existing documents is unaffected.
// 29sep2003-- Modified SetParams() to change allowed range for the dot direction offset range endpt N for NOISYDOTS.
//          -- Modified Import() to handle target definition files for the new NOISYDOTS target type.  A new parameter,
//             "DOTDIROFFSET", was added to set the value for the target's dot direction offset range endpoint.
// 13oct2004-- Adding new XY scope tgt type COHERENTFC.  Same as FASTCENTER, except that only a specified N% of the
//             target dots move coherently.  Hardware implementation:  On every XY frame, N% of the dots move IAW the
//             specified motion vector, while the rest get randomly repositioned in the target window.  Based on the
//             stimulus paradigm described in 1988 J.Neurosci. article by Newsome & Pare.  The percent coherence is
//             stored as an integer percentage [0..100] in XYPARMS.fInnerW....  Since we are adding a new type and not
//             changing the XYPARMS structure in any way for this new type, serialization of existing documents is
//             unaffected.
// 15mar2005-- Added CopyRemoteObj() override.
// 11apr2005-- Mods to support change in implementation of NOISYDOTS:  Use XYPARMS.fInnerH to store an update interval
//             in milliseconds.  Instead of randomizing the dot directions on every refresh, they are now randomized
//             once each update interval, which should be an integral multiple of the XY scope frame period.  This
//             change required introducing a versionable schema for CCxTarget.
// 07jan2006-- Mods to support new target type NOISYSPEED.  NOISYDOTS changed to NOISYDIR.  NOISYSPEED is similar to
//             NOISYDIR, except that noise is in dot speed instead of direction.  Noise offset range N is specified as
//             a percentage of the nominal speed V, and is limited to [MIN_SPEEDOFFSET..MAX_SPEEDOFFSET]. Schema 2.
// 14mar2006-- Removed support for the predefined target type CX_OKNDRUM (as of Maestro v1.5.0).
// 22mar2006-- Completely reworked to replace VSG-based FB video targets with RMVideo targets.  Schema version 3.
// 04apr2006-- Mods to allow RMVTGTDEF.fSigma to apply to RMVideo target types RMV_SPOT and _RANDOMDOTS in addition to
//             RMV_GRATING and _PLAID.  Schema version unchanged, since Maestro w/RMVideo has not yet been released.
//          -- Further mod to support elliptical Gaussian window: RMVTGTDEF.fSigma is now a 2-element array, with the
//             std dev in X as the first element and the std dev in Y as the second.
// 07apr2006-- Mod to support separate color specs for the two gratings of a plaid RMVideo target: RMVTGTDEF.iRGBMean
//             and iRGBCon are now 2-element arrays.  Schema version unchanged, since Maestro w/RMVideo not yet rel.
// 19jun2006-- Mod to support new XYPARMS fields fInnerX & fInnerY, which apply only to RECTANNU XY scope target -- to 
//             allow offsetting "hole" relative to target center. Schema version 4, effective Maestro 2.0.1.
// 31aug2007-- Mods to support an alternative algorithm for generating per-dot noise for the NOISYSPEED XY scope tgt 
//             and its analogous implementation in RMVideo. Also fixed granularity of speed noise for the RMVideo 
//             RMV_RANDOMDOTS tgt to match that of the NOISYSPEED tgt (was 0.1%, now 1%). Schema version 5, effective 
//             Maestro 2.1.3.
// 09sep2009-- Mods to support new RMVideo target type RMV_MOVIE. Two char[] fields were added to RMVTGTDEF,
//             specifying the name and containing folder for the video file played back by this target type. New flags
//             relevant only to this target type were also introduced. Schema version 6, effective Maestro 2.5.0.
// 11sep2009-- The RMV_GRATING target type was modified to recognize new flag RMV_F_ORIENTADJ. When this flag is set, 
//             the grating orientation follows the direction of the pattern displacement vector during an animation. In 
//             this case, both components of that vector are significant; when the flag is unset (original behavior), 
//             the H cmpt is the pattern displacement along the "drift axis", and V cmpt is ignored. Effective v2.5.0.
// 04dec2009-- Implementation of RMV_F_ORIENTADJ feature modified slightly to support RMV_PLAID as well as RMV_GRATING:
//             the direction of the pattern displacement vector is now added to each grating's defined orientation to
//             get the per-frame orientation; when the vector direction is undefined (zero components), the orientation
//             during the previous display frame is carried over to the current frame. For RMV_PLAID, RMV_F_ORIENTADJ
//             is mutually exclusive of flag RMV_F_INDEPGRATS. Effective v2.5.1.
// 13jan2010-- Comments noting introduction of new flag RMV_F_WRTSCREEN. No code changes required. Eff v2.5.2.
// 21sep2011-- Removed support for the predefined target types CX_FIBER*, CX_REDLED* -- as of Maestro v3.0.
// 24nov2014-- To support a two-color RMVideo RMV_RANDOMDOTS target, RMVTGTDEF.iRGBCon[0] is now persisted for that tgt
//             type. It is set to 0 when reading in pre-existing RMVideo targets, since C=0 implies a single-color dot
//             patch. Schema version 7, effective Maestro 3.1.2.
// 11oct2016-- Updated to support new RMVideo target type, RMV_IMAGE, representing a static image loaded from an image
//             source file in RMVideo's "media store" (formerly, "movie store"). Only two relevant parameters, 
//             RMVTGTDEF.strFolder and .strFile. Schema version 8, effective Maestro 3.3.1.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
// 07may2019-- Updated to support new "flicker" feature in RMVideo targets. Three int fields were added to RMVTGTDEF,
//             specifying the flicker ON duration, OFF duration, and initial delay -- all in # of video frame periods.
//             Feature is disabled if either the ON or OFF duration is zero. Schema version 9, eff. Maestro 4.1.0.
// 30sep2024-- XYScope targets, not supported since V4.0, are officially deprecated as of Maestro 5.0. CCxTarget will
//             still be able to create XYScope targets, however, in order to support deserialization and migration of
//             older experiment documents.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource defines

#include "cxdoc.h"                           // CCxDoc -- the Maestro document (req'd for importing purposes)
#include "util.h"
#include "cxtarget.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL( CCxTarget, CTreeObj, 9 | VERSIONABLE_SCHEMA )


//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
LPCTSTR CCxTarget::RMVTYPENAMES[] =                   // GUI names for the RMVideo target types
{
   "Point",
   "Random-Dot Patch",
   "Random-Dot Flow Field",
   "Solid Bar/Line",
   "Uniform Spot",
   "Grating",
   "Plaid",
   "Movie",
   "Image"
};

LPCTSTR CCxTarget::RMVSHAPENAMES[] =                  // GUI names for the possible RMVideo aperture shapes
{
   "rectangular",
   "elliptical",
   "rectangular annulus",
   "elliptical annulus"
};


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== ~CCxTarget [destructor] =========================================================================================
//
//    Destroy target.  Since memory may have been allocated for modifiable target parameters, we must be sure to
//    release that memory here.
//
//    NOTE:  The delete operator works on a void pointer.  Only requirement is that the memory block was originally
//    allocated by the new operator.
//
CCxTarget::~CCxTarget()
{
   if ( m_pvParms != NULL )
   {
      delete m_pvParms;
      m_pvParms = NULL;
   }
}


//=== Initialize [base override] ======================================================================================
//
//    Initialize target object after default construction.
//
//    This method MUST be called directly after default construction to initialize the target IAW the specified target
//    name, MAESTRO target type, and state flags.  Memory is allocated for any modifiable parameters, which are then
//    initialized to default values.
//
//    If invoked on a target object that has already been initialized, the object is reinitialized IAW the specified
//    parameters.  Not really intended for such usage, but see Copy().
//
//    ARGS:       s  -- [in] the name assigned to target object
//                t  -- [in] the MAESTRO object data type -- MUST be a recognized MAESTRO target type
//                f  -- [in] the object's initial state flags -- CANNOT include CX_ISSETOBJ.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to allocate memory for modifiable target parameters.
//
VOID CCxTarget::Initialize( LPCTSTR s, const WORD t, const WORD f )
{
   ASSERT( ValidTargetType( t ) );                          // validate target object type and flags
   ASSERT( (f & CX_ISSETOBJ) == 0 );

   if ( ValidTargetType( m_type ) &&                        // reinitializing a valid target and the current param
        (m_pvParms != NULL) && (m_type != t) )              // alloc is not compatible w/ new type:  deallocate params
   {
      delete m_pvParms;
      m_pvParms = NULL;
   }

   CTreeObj::Initialize( s, t, f );                         // base class inits
   if ( m_pvParms == NULL )                                 // allocate storage for modifiable params if needed
   {
      if ( m_type == CX_XYTARG )
         m_pvParms = (PVOID) new XYPARMS;
      else if ( m_type == CX_RMVTARG )
         m_pvParms = (PVOID) new RMVTGTDEF;
   }

   AssignDefaultValues();                                   // init params to default values
}


//=== Copy [base override] ============================================================================================
//
//    Copy members of source target object to THIS (already constructed).  Since THIS target's type may not be the same
//    as the source type, we may have to reallocate memory for any modifiable parameters.
//
//    ARGS:       pSrc  -- [in] CTreeObj* ptr to the target to be copied.  MUST point to a valid CCxTarget object!
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to allocate memory for modifiable target parameters.
//
VOID CCxTarget::Copy( const CTreeObj* pSrc )
{
   ASSERT( pSrc->IsKindOf( RUNTIME_CLASS( CCxTarget ) ) );           // source object MUST be a CCxTarget obj!

   const CCxTarget* pSrcTgt = (CCxTarget*) pSrc;                     // validate source target
   ASSERT_VALID( pSrcTgt );

   Initialize( pSrcTgt->m_name, pSrcTgt->m_type, pSrcTgt->m_flags ); // reinitialize THIS tgt using src tgt info

   if ( m_type == CX_XYTARG )                                        // copy tgt params from source; note ptr casts!!
      *((PXYPARMS)m_pvParms) = *((PXYPARMS) pSrcTgt->m_pvParms);
   else if ( m_type == CX_RMVTARG )
      *((PRMVTGTDEF)m_pvParms) = *((PRMVTGTDEF) pSrcTgt->m_pvParms);

}


//=== CopyRemoteObj [base override] ===================================================================================
//
//    Copies the CCxTarget-specific definition of a target located in a different experiment document.
//
//    CopyRemoteObject was introduced to the CTreeObj/CTreeMap framework to overcome the problem of copying an object
//    from one treemap to another.  It is intended only for copying the internal information specific to a given
//    implementation of CTreeObj.
//
//    ARGS:       pSrc        -- [in] the object to be copied.  Must be an instance of CCxTarget.
//                depKeyMap   -- [in] maps keys of any objects upon which the source obj depends, which reside in the
//                               source doc, to the keys of the corresponding objects in the destination doc.
//
//    RETURNS:    TRUE if successful, FALSE if source object is not an instance of CCxTarget
//
BOOL CCxTarget::CopyRemoteObj(CTreeObj* pSrc, const CWordToWordMap& depKeyMap)
{
   if( pSrc == NULL || !pSrc->IsKindOf( RUNTIME_CLASS(CCxTarget) ) ) return( FALSE );

   const CCxTarget* pSrcTgt = (CCxTarget*) pSrc;
   ASSERT_VALID( pSrcTgt );

   if ( m_type == CX_XYTARG )
      *((PXYPARMS)m_pvParms) = *((PXYPARMS) pSrcTgt->m_pvParms);
   else if ( m_type == CX_RMVTARG )
      *((PRMVTGTDEF)m_pvParms) = *((PRMVTGTDEF) pSrcTgt->m_pvParms);
   return( TRUE );
}


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== GetParams =======================================================================================================
//
//    Retrieve target's current parameter set.
//
//    ARGS:       pTgt  -- [out] pointer to a union that can store an XY or RMVideo target parameter set.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (when called on a target that has no parameters).
//
BOOL CCxTarget::GetParams( PU_TGPARMS pTgt ) const
{
   ASSERT_VALID( this );
   ASSERT( AfxIsValidAddress( pTgt, sizeof(U_TGPARMS) ) );              // validate storage for params

   BOOL bRet = TRUE;
   if( m_type == CX_XYTARG )        pTgt->xy = *((PXYPARMS)m_pvParms);
   else if( m_type == CX_RMVTARG )  
   {
      ::memset(&(pTgt->rmv), 0, sizeof(RMVTGTDEF));      // so that char[] arrays are padded with nulls
      pTgt->rmv = *((PRMVTGTDEF)m_pvParms);
   }
   else                             bRet = FALSE;                       // target has no params to retrieve!

   return( bRet );
}


//=== SetParams =======================================================================================================
//
//    Replace current parameter set with the one provided, auto-correcting any invalid parameters. No effect on
//    non-modifiable targets.
//
//    ARGS:       pTgt     -- [in/out] ptr to a union containing either an XYScope or RMVideo target parameter set. 
//                            If any parameters are auto-corrected, those changes are reflected here.
//                bChanged -- [out] TRUE if operation changed the value of any parameter.
//
//    RETURNS:    TRUE if the proposed parameters were accepted w/o correction, FALSE if any corrections were made.
//                Also returns FALSE if target does not contain modifiable parameters...
//
BOOL CCxTarget::SetParams( PU_TGPARMS pTgt, BOOL& bChanged )
{
   ASSERT_VALID( this );
   ASSERT( AfxIsValidAddress( pTgt, sizeof(U_TGPARMS) ) );           // validate storage for params

   bChanged = FALSE;                                                 // set TRUE if any param is changed
   BOOL bOK = TRUE;                                                  // set FALSE if any param is auto-corrected

   if( !IsModifiable() ) return( FALSE );                            // cannot alter target!

   if( m_type == CX_XYTARG )                                         // copy proposed XY target parameters, correcting
   {                                                                 // any invalid values:
      PXYPARMS pXY = &(pTgt->xy);

      if( 0 == ::memcmp( m_pvParms, pXY, sizeof(XYPARMS) ) )         // no changes made!!
         return( TRUE );

      if( (pXY->type < 0) || (pXY->type >= NUMXYTYPES) ) { pXY->type = CENTER; bOK = FALSE; }

      if( pXY->ndots < 0 ) { pXY->ndots = 10; bOK = FALSE; }

      float fHalfDim = 0.0f;
      switch( pXY->type )
      {
         case CENTER :
         case SURROUND :
         case FASTCENTER :
         case FCDOTLIFE :
         case NOISYDIR :
         case NOISYSPEED :
         case COHERENTFC :
            if( pXY->fRectW < MINRECTDIM ) { pXY->fRectW = MINRECTDIM; bOK = FALSE; }
            if( pXY->fRectH < MINRECTDIM ) { pXY->fRectH = MINRECTDIM; bOK = FALSE; }
            if( pXY->type == FCDOTLIFE || pXY->type == NOISYDIR || pXY->type == NOISYSPEED )
            {
               if( (pXY->iDotLfUnits != DOTLFINMS) && (pXY->iDotLfUnits != DOTLFINDEG) )
               {
                  pXY->iDotLfUnits = DOTLFINDEG;
                  bOK = FALSE;
               }
               if( pXY->iDotLfUnits == DOTLFINMS )
               {
                  int iTrunc = int( pXY->fDotLife );
                  if( iTrunc < SGH_MINXYFRAME ) iTrunc = SGH_MINXYFRAME;
                  else if( iTrunc > MAX_DOTLFINMS ) iTrunc = MAX_DOTLFINMS;
                  if( pXY->fDotLife != float( iTrunc ) ) { pXY->fDotLife = float( iTrunc ); bOK = FALSE; }
               }
               else
               {
                  float f = pXY->fDotLife;
                  if( f < 0.01f ) f = 0.01f;
                  else if( f > MAX_DOTLFINDEG ) f = MAX_DOTLFINDEG;
                  if( f != pXY->fDotLife ) { pXY->fDotLife = f; bOK = FALSE; }
               }

               // validate noise offset range and noise update interval and restrict to integer values.
               // for NOISYSPEED, fInnerX selects from one of two noise strategies, additive or *(2^x)
               if( pXY->type == NOISYDIR || pXY->type == NOISYSPEED )
               {
                  int iTrunc = int( pXY->fInnerW );
                  int iMin = 
                     (pXY->type==NOISYDIR) ? MIN_DIROFFSET : ((pXY->fInnerX != 0.0f) ? MIN_SPDLOG2 : MIN_SPEEDOFFSET);
                  int iMax = 
                     (pXY->type==NOISYDIR) ? MAX_DIROFFSET : ((pXY->fInnerX != 0.0f) ? MAX_SPDLOG2 : MAX_SPEEDOFFSET);
                  if( iTrunc < iMin ) iTrunc = iMin;
                  else if( iTrunc > iMax ) iTrunc = iMax;
                  if( pXY->fInnerW != float(iTrunc) ) { pXY->fInnerW = float( iTrunc ); bOK = FALSE; }

                  iTrunc = int( pXY->fInnerH );
                  if( iTrunc < MIN_NOISEUPD ) iTrunc = MIN_NOISEUPD;
                  else if( iTrunc > MAX_NOISEUPD ) iTrunc = MAX_NOISEUPD;
                  if( pXY->fInnerH != float(iTrunc) ) { pXY->fInnerH = float( iTrunc ); bOK = FALSE; }
               }
            }
            if( pXY->type == COHERENTFC )       // percent coherence is integral value in [0..100]
            {
               int iTrunc = int( pXY->fInnerW );
               if( iTrunc < 0 ) iTrunc = 0;
               else if( iTrunc > 100 ) iTrunc = 100;
               if( pXY->fInnerW != float(iTrunc) ) { pXY->fInnerW = float( iTrunc ); bOK = FALSE; }
            }
            break;
         case RECTANNU :
            if( pXY->fRectW < MINRECTDIM ) { pXY->fRectW = MINRECTDIM; bOK = FALSE; }
            if( pXY->fRectH < MINRECTDIM ) { pXY->fRectH = MINRECTDIM; bOK = FALSE; }
            if( pXY->fInnerW < MINRECTDIM ) { pXY->fInnerW = MINRECTDIM; bOK = FALSE; }
            if( pXY->fInnerH < MINRECTDIM ) { pXY->fInnerH = MINRECTDIM; bOK = FALSE; }
            if( pXY->fInnerW > pXY->fRectW ) { pXY->fInnerW = pXY->fRectW; bOK = FALSE; }
            if( pXY->fInnerH > pXY->fRectH ) { pXY->fInnerH = pXY->fRectH; bOK = FALSE; }
            fHalfDim = pXY->fRectW/2.0f;
            if( (pXY->fInnerX + pXY->fInnerW/2.0) > fHalfDim || (pXY->fInnerX - pXY->fInnerW/2.0) < -fHalfDim )
            {
               pXY->fInnerX = 0.0f; bOK = FALSE;
            }
            fHalfDim = pXY->fRectH/2.0f;
            if( (pXY->fInnerY + pXY->fInnerH/2.0) > fHalfDim || (pXY->fInnerY - pXY->fInnerH/2.0) < -fHalfDim )
            {
               pXY->fInnerY = 0.0f; bOK = FALSE;
            }
            break;
         case RECTDOT :
            if( pXY->fRectW < MINRECTDIM ) { pXY->fRectW = MINRECTDIM; bOK = FALSE; }
            if( pXY->fRectH < 0.0f ) { pXY->fRectH = 0.0f; bOK = FALSE; }                    // dot spacing of 0 is OK
            break;
         case FLOWFIELD :
            // restrictions:  inner radius (fInnerW) and outer radius (fRectW) must lie in [FLOWMINRAD..FLOWMAXRAD],
            // and inner radius + FLOWDIFFRAD <= outer radius.
            //
            if( pXY->fInnerW < FLOWMINRAD || pXY->fInnerW > FLOWMAXRAD ) {pXY->fInnerW = FLOWMINRAD; bOK = FALSE; }
            if( pXY->fRectW < FLOWMINRAD || pXY->fRectW > FLOWMAXRAD ) {pXY->fRectW = FLOWMAXRAD; bOK = FALSE; }
            if( (pXY->fInnerW + FLOWDIFFRAD) > pXY->fRectW )
            {
               pXY->fRectW = pXY->fInnerW + FLOWDIFFRAD;
               if( pXY->fRectW > FLOWMAXRAD )
               {
                  pXY->fRectW = FLOWMAXRAD;
                  pXY->fInnerW = FLOWMAXRAD - FLOWDIFFRAD;
               }
               bOK = FALSE;
            }
            break;
         case ORIENTEDBAR :
            // bounding rect width can be zero, to specify a line rather than a rect bar.  drift axis limited to the
            // range [BAR_MINDA..BAR_MAXDA].
            //
            if( pXY->fRectW < 0.0f ) { pXY->fRectW = 0.0f; bOK = FALSE; }
            if( pXY->fRectH < MINRECTDIM ) { pXY->fRectH = MINRECTDIM; bOK = FALSE; }
            if( pXY->fInnerW < BAR_MINDA || pXY->fInnerW > BAR_MAXDA ) { pXY->fInnerW = BAR_MINDA; bOK = FALSE; }
            break;
      }

      if( 0 != ::memcmp( m_pvParms, pXY, sizeof(XYPARMS) ) )         // if any param actually changed, then update our
      {                                                              // internal copy of the parameter set
         bChanged = TRUE;
         *((PXYPARMS)m_pvParms) = pTgt->xy;
      }
   }
   else if ( m_type == CX_RMVTARG )                                  // copy proposed RMV target parameters, correcting
   {                                                                 // any invalid values:
      PRMVTGTDEF pRMV = &(pTgt->rmv);

      if( 0 == ::memcmp( m_pvParms, pRMV, sizeof(RMVTGTDEF) ) )      // no changes made!!
         return( TRUE );

      if( (pRMV->iType < 0) || (pRMV->iType >= RMV_NUMTGTTYPES) ) { pRMV->iType = RMV_POINT; bOK = FALSE; }

      if( (pRMV->iAperture < RMV_RECT) || (pRMV->iAperture > RMV_OVALANNU) )
      {
         pRMV->iAperture = RMV_RECT; bOK = FALSE;
      }
      if( (pRMV->iType == RMV_GRATING || pRMV->iType == RMV_PLAID) && (pRMV->iAperture > RMV_OVAL) )
      {
         pRMV->iAperture = RMV_RECT; bOK = FALSE;
      }
      
      // flicker parameters have a limited range
      if(pRMV->iFlickerOn < RMV_MINFLICKERDUR || pRMV->iFlickerOn > RMV_MAXFLICKERDUR)
      {
         pRMV->iFlickerOn = RMV_MINFLICKERDUR;
         bOK = FALSE;
      }
      if(pRMV->iFlickerOff < RMV_MINFLICKERDUR || pRMV->iFlickerOff > RMV_MAXFLICKERDUR)
      {
         pRMV->iFlickerOff = RMV_MINFLICKERDUR;
         bOK = FALSE;
      }
      if(pRMV->iFlickerDelay < RMV_MINFLICKERDUR || pRMV->iFlickerDelay > RMV_MAXFLICKERDUR)
      {
         pRMV->iFlickerDelay = RMV_MINFLICKERDUR;
         bOK = FALSE;
      }

      // RMV_F_INDEPGRATS and RMV_F_ORIENTADJ flags are mutually exclusive for RMV_PLAID
      if(pRMV->iType == RMV_PLAID)
      {
         if((pRMV->iFlags & (RMV_F_INDEPGRATS|RMV_F_ORIENTADJ)) == (RMV_F_INDEPGRATS|RMV_F_ORIENTADJ))
         {
            pRMV->iFlags &= ~RMV_F_INDEPGRATS; bOK = FALSE;
         }
      }

      int iOrig = pRMV->iRGBMean[0];
      pRMV->iRGBMean[0] = iOrig & 0x00FFFFFF;
      if( iOrig != pRMV->iRGBMean[0] ) bOK = FALSE;

      if( pRMV->iType == RMV_PLAID )
      {
         iOrig = pRMV->iRGBMean[1];
         pRMV->iRGBMean[1] = iOrig & 0x00FFFFFF;
         if( iOrig != pRMV->iRGBMean[1] ) bOK = FALSE;
      }

      // as of v3.1.2, support two-color random dot patch, with Lmin = Lmean(1-C) and Lmax = Lmean(1+C), C=[0..1].
      // Half the dots painted Lmin, half Lmax. When C=0, reverts to normal single-color dot patch.
      if( pRMV->iType == RMV_GRATING || pRMV->iType == RMV_PLAID || pRMV->iType == RMV_RANDOMDOTS)
      {
         iOrig = pRMV->iRGBCon[0];
         pRMV->iRGBCon[0] = cMath::rangeLimit(iOrig & 0x00FF, 0, 100) +
            (cMath::rangeLimit((iOrig >> 8) & 0x00FF, 0, 100) << 8) +
            (cMath::rangeLimit((iOrig >> 16) & 0x00FF, 0, 100) << 16);
         if( iOrig != pRMV->iRGBCon[0] ) bOK = FALSE;

         if( pRMV->iType == RMV_PLAID )
         {
            iOrig = pRMV->iRGBCon[1];
            pRMV->iRGBCon[1] = cMath::rangeLimit(iOrig & 0x00FF, 0, 100) +
               (cMath::rangeLimit((iOrig >> 8) & 0x00FF, 0, 100) << 8) +
               (cMath::rangeLimit((iOrig >> 16) & 0x00FF, 0, 100) << 16);
            if( iOrig != pRMV->iRGBCon[1] ) bOK = FALSE;
         }
      }

      float fMinOuterW = (pRMV->iType==RMV_BAR) ? 0.0f : RMV_MINRECTDIM;
      if( pRMV->fOuterW < fMinOuterW ) { pRMV->fOuterW = fMinOuterW; bOK = FALSE; }
      else if( pRMV->fOuterW > RMV_MAXRECTDIM ) { pRMV->fOuterW = RMV_MAXRECTDIM; bOK = FALSE; }

      if( pRMV->fOuterH < RMV_MINRECTDIM ) { pRMV->fOuterH = RMV_MINRECTDIM; bOK = FALSE; }
      else if( pRMV->fOuterH > RMV_MAXRECTDIM ) { pRMV->fOuterH = RMV_MAXRECTDIM; bOK = FALSE; }

      if( pRMV->fInnerW < RMV_MINRECTDIM ) { pRMV->fInnerW = RMV_MINRECTDIM; bOK = FALSE; }
      else if( pRMV->fInnerW > RMV_MAXRECTDIM ) { pRMV->fInnerW = RMV_MAXRECTDIM; bOK = FALSE; }

      if( pRMV->fInnerH < RMV_MINRECTDIM ) { pRMV->fInnerH = RMV_MINRECTDIM; bOK = FALSE; }
      else if( pRMV->fInnerH > RMV_MAXRECTDIM ) { pRMV->fInnerH = RMV_MAXRECTDIM; bOK = FALSE; }

      // make sure inner < outer when inner dimensions are relevant
      if( pRMV->iType == RMV_FLOWFIELD )
      {
         if( pRMV->fOuterW <= pRMV->fInnerW ) {pRMV->fOuterW = pRMV->fInnerW + 5.0f; bOK = FALSE; }
      }
      else if( pRMV->iType == RMV_RANDOMDOTS || pRMV->iType == RMV_SPOT )
      {
         if( pRMV->fOuterW <= pRMV->fInnerW ) {pRMV->fOuterW = pRMV->fInnerW + 5.0f; bOK = FALSE; }
         if( pRMV->fOuterH <= pRMV->fInnerH ) {pRMV->fOuterH = pRMV->fInnerH + 5.0f; bOK = FALSE; }
      }

      if( pRMV->iType == RMV_RANDOMDOTS || pRMV->iType == RMV_FLOWFIELD )
      {
         if( pRMV->nDots < 0 || pRMV->nDots > RMV_MAXNUMDOTS ) { pRMV->nDots = 100; bOK = FALSE; }
      }

      if( pRMV->iType == RMV_POINT || pRMV->iType == RMV_RANDOMDOTS || pRMV->iType == RMV_FLOWFIELD )
      {
         if( pRMV->nDotSize < RMV_MINDOTSIZE ) { pRMV->nDotSize = RMV_MINDOTSIZE; bOK = FALSE; }
         else if( pRMV->nDotSize > RMV_MAXDOTSIZE ) { pRMV->nDotSize = RMV_MAXDOTSIZE; bOK = FALSE; }
      }

      if( pRMV->iType == RMV_RANDOMDOTS )
      {
         if( pRMV->iPctCoherent < 0 ) {pRMV->iPctCoherent = 0; bOK = FALSE; }
         else if( pRMV->iPctCoherent > 100 ) {pRMV->iPctCoherent = 100; bOK = FALSE; }

         if( pRMV->iNoiseUpdIntv < 0 ) {pRMV->iNoiseUpdIntv = 0; bOK = FALSE; }

         if( (pRMV->iFlags & RMV_F_DIRNOISE) != 0 )
         {
            if( pRMV->iNoiseLimit < RMV_MINNOISELIMIT ) {pRMV->iNoiseLimit = RMV_MINNOISELIMIT; bOK = FALSE; }
            else if( pRMV->iNoiseLimit > RMV_MAXNOISEDIR ) {pRMV->iNoiseLimit = RMV_MAXNOISEDIR; bOK = FALSE; }
         }
         else 
         {
            int iMin = RMV_MINNOISELIMIT;
            int iMax = RMV_MAXNOISESPEED;
            if( (pRMV->iFlags & RMV_F_SPDLOG2) != 0 ) { iMin = RMV_MINSPDLOG2; iMax = RMV_MAXSPDLOG2; }
            
            if( pRMV->iNoiseLimit < iMin ) {pRMV->iNoiseLimit = iMin; bOK = FALSE; }
            else if( pRMV->iNoiseLimit > iMax ) {pRMV->iNoiseLimit = iMax; bOK = FALSE; }
         }

         if( pRMV->fDotLife < 0.0f ) {pRMV->fDotLife = 0.0f; bOK = FALSE; }
      }

      if( pRMV->iType == RMV_GRATING || pRMV->iType == RMV_PLAID )
      {
         if( pRMV->fSpatialFreq[0] < 0.01f ) { pRMV->fSpatialFreq[0] = 0.01f; bOK = FALSE; }

         float fOrig = pRMV->fGratPhase[0];
         pRMV->fGratPhase[0] = cMath::limitToUnitCircleDeg( fOrig );
         if( fOrig != pRMV->fGratPhase[0] ) bOK = FALSE;

         if( pRMV->iType == RMV_PLAID )
         {
            if( pRMV->fSpatialFreq[1] < 0.01f ) { pRMV->fSpatialFreq[1] = 0.01f; bOK = FALSE; }

            float fOrig = pRMV->fGratPhase[1];
            pRMV->fGratPhase[1] = cMath::limitToUnitCircleDeg( fOrig );
            if( fOrig != pRMV->fGratPhase[1] ) bOK = FALSE;
         }
      }
      else if( pRMV->iType == RMV_BAR )
      {
         float fOrig = pRMV->fDriftAxis[0];
         pRMV->fDriftAxis[0] = cMath::limitToUnitCircleDeg( fOrig );
         if( fOrig != pRMV->fDriftAxis[0] ) bOK = FALSE;
      }

      if(pRMV->iType==RMV_SPOT || pRMV->iType==RMV_RANDOMDOTS || pRMV->iType==RMV_GRATING || pRMV->iType==RMV_PLAID)
      {
         if( pRMV->fSigma[0] < 0.0f ) {pRMV->fSigma[0] = 0.0f; bOK = FALSE; }
         if( pRMV->fSigma[1] < 0.0f ) {pRMV->fSigma[1] = 0.0f; bOK = FALSE; }
      }

      // if media folder or file name is invalid, replace it with defaults "folderName", "mediaName"
      if(pRMV->iType == RMV_MOVIE || pRMV->iType == RMV_IMAGE)
      {
         int len = static_cast<int>(::strlen(pRMV->strFolder));
         if(len == 0 || len > RMV_MVF_LEN || len != (int) ::strspn(pRMV->strFolder, RMV_MVF_CHARS))
         {
            ::strncpy_s(pRMV->strFolder, "folderName", 32);
            bOK = FALSE;
         }
         len = static_cast<int>(::strlen(pRMV->strFile));
         if(len == 0 || len > RMV_MVF_LEN || len != (int) ::strspn(pRMV->strFile, RMV_MVF_CHARS))
         {
            ::strncpy_s(pRMV->strFile, "mediaName", 32);
            bOK = FALSE;
         }
      }
      
      if( 0 != ::memcmp( m_pvParms, pRMV, sizeof(RMVTGTDEF) ) )      // if any param actually changed, then update our
      {                                                              // internal copy of the parameter set
         bChanged = TRUE;
         *((PRMVTGTDEF)m_pvParms) = pTgt->rmv;
      }
   }

   return( bOK );
}


//=== Serialize [base override] =======================================================================================
//
//    Handles reading/writing the target object from/to a disk file via a serialization archive.  For more efficient
//    file storage, we only serialize the *relevant* parameters of a modifiable target.
//
//    Version Control (using MFC "schema"):
//       1: Base version.  Valid thru Maestro version 1.2.7.
//       2: Modified implementation of NOISYDIR target to include an additional parameter, the noise update interval,
//          stored in XYPARMS.fInnerH.  (v1.4.1) This same parameter is now used for NOISYSPEED target as well.
//       2: Corrected benign mistake in serialization of ORIENTEDBAR and FLOWFIELD types -- the XYPARMS.fInnerH field
//          was unnecessarily serialized!
//       3: Old VSG FB video replaced with RMVideo server.  Old FB target types migrated to analogous RMVideo types.
//          Effective Maestro 2.0.0.
//       4: Added fields fInnerX, fInnerY to XYPARMS -- to support offset hole in XY scope RECTANNU target only.
//          Effective Maestro 2.0.1.
//       5: Added support for a second algorithm for generating per-dot speed noise in the XY scope NOISYSPEED tgt and 
//          the analogous implementation in the RMVideo tgt RMV_RANDOMDOTS. Also, made the speed noise offset-range 
//          granularity for the RMV_RANDOMDOTS target 1% instead of 0.1% -- to bring it in line with an earlier 
//          change to the XYScope NOISYSPEED target.
//       6: Two new char[] fields added to RMVTGTDEF to support new target type RMV_MOVIE. New flag introduced for type
//          RMV_GRATING, but no effect on serialization. Effective Maestro v2.5.0. 
//       6: Flag RMV_F_ORIENTADJ now also applies to target type RMV_PLAID, but no effect on serialization. Effective
//          Maestro v2.5.1.
//       6: New flag RMV_F_WRTSCREEN introduced, applicable only to RMV_RANDOMDOTS. No effect on serialization. 
//          Effective Maestro v2.5.2.
//       6: Non-modifiable targets CX_FIBER* and CX_REDLED* no longer supported, effective Maestro 3.0. No change here,
//          as we must allow these obsolete target objects to be deserialized from an old document. CCxDoc takes care
//          of removing these objects and any dependencies upon them.
//       7: RMVTGTDEF.iRGBCon[0] is now applicable to the RMV_RANDOMDOTS target. If 0 (the default), then all dots in
//          patch are a single color, as in prior versions. Otherwise, half the dots are painted L1 and half L2, where
//          L1 = Lmean(1+C) and L2 = Lmean(1-C), with C = iRGBCon[0]/100. Effective Maestro v3.1.2.
//       8: New target type RMV_IMAGE introduced. No changes in RMVTGTDEF struct, and no effect on deserialization of
//          existing documents. Effective Maestro v3.3.1.
//       9: Added three integer-valued flicker parameters to RMVTGTDEF, applicable to all target types. Effective 
//          Maestro 4.1.0.
//       9: (30sep2024) XYScope target CX_XYTARG officially deprecated, effective Maestro 5.0.0. NO CHANGE when
//          deserializing, as we must allow these obsolete target objects to be deserialized from an old document. 
//          CCxDoc takes care of removing these objects and any dependencies on them. Howver, any attempt to serialize an
//          XYScope target will fail.
//
//    ARGS:       ar -- [in] the serialization archive.
//
//    RETURNS:    NONE.
//
//    THROWS:     -- The CArchive object can throw CMemoryException, CArchiveException, or CFileException
//                -- This method itself may throw CMemoryException during deserialization if unable to allocate
//                   memory for target parameters.  If the loading process throws an exception AFTER allocating param
//                   storage, we catch it in order to free that storage; we then pass on the exception.
//
void CCxTarget::Serialize( CArchive& ar )
{
   UINT nSchema = ar.GetObjectSchema();                                          // retrieve schema#
   CTreeObj::Serialize( ar );                                                    // serialize base class stuff first:
                                                                                 // obj name, data type, and flags.

   if( ar.IsStoring() )                                                          // write to archive:
   {
      // cannot save XYScope targets, which are deprecated.
      if(m_type == CX_XYTARG)
         ::AfxThrowArchiveException(CArchiveException::genericException);

      // archive *relevant RMVideo target params. Save tgt type first; it determines what other params are relevant!
      if(m_type == CX_RMVTARG)
      {
         CString str;
         PRMVTGTDEF pRMV = (PRMVTGTDEF)m_pvParms;
         ar << pRMV->iType;
         switch( pRMV->iType )
         {
            case RMV_POINT :
               ar << pRMV->iRGBMean[0] << pRMV->nDotSize;
               break;
            case RMV_RANDOMDOTS :
               ar << pRMV->iAperture << pRMV->iFlags << pRMV->iRGBMean[0] << pRMV->iRGBCon[0];
               ar << pRMV->fOuterW << pRMV->fOuterH << pRMV->fInnerW << pRMV->fInnerH;
               ar << pRMV->nDots << pRMV->nDotSize << pRMV->iSeed << pRMV->iPctCoherent;
               ar << pRMV->iNoiseUpdIntv << pRMV->iNoiseLimit << pRMV->fDotLife;
               ar << pRMV->fSigma[0] << pRMV->fSigma[1];
               break;
            case RMV_FLOWFIELD :
               ar << pRMV->iRGBMean[0] << pRMV->fOuterW << pRMV->fInnerW;
               ar << pRMV->nDots << pRMV->nDotSize << pRMV->iSeed;
               break;
            case RMV_BAR :
               ar << pRMV->iRGBMean[0] << pRMV->fOuterW << pRMV->fOuterH << pRMV->fDriftAxis[0];
               break;
            case RMV_SPOT :
               ar << pRMV->iAperture << pRMV->iRGBMean[0];
               ar << pRMV->fOuterW << pRMV->fOuterH << pRMV->fInnerW << pRMV->fInnerH;
               ar << pRMV->fSigma[0] << pRMV->fSigma[1];
               break;
            case RMV_GRATING :
               ar << pRMV->iAperture << pRMV->iFlags << pRMV->iRGBMean[0] << pRMV->iRGBCon[0];
               ar << pRMV->fOuterW << pRMV->fOuterH << pRMV->fSpatialFreq[0] << pRMV->fDriftAxis[0];
               ar << pRMV->fGratPhase[0] << pRMV->fSigma[0] << pRMV->fSigma[1];
               break;
            case RMV_PLAID :
               ar << pRMV->iAperture << pRMV->iFlags << pRMV->iRGBMean[0] << pRMV->iRGBMean[1];
               ar << pRMV->iRGBCon[0] << pRMV->iRGBCon[1];
               ar << pRMV->fOuterW << pRMV->fOuterH << pRMV->fSpatialFreq[0] << pRMV->fSpatialFreq[1];
               ar << pRMV->fDriftAxis[0] << pRMV->fDriftAxis[1] << pRMV->fGratPhase[0] << pRMV->fGratPhase[1];
               ar << pRMV->fSigma[0] << pRMV->fSigma[1];
               break;
            case RMV_MOVIE :
               ar << pRMV->iFlags;
               str = pRMV->strFolder;
               ar << str;
               str = pRMV->strFile;
               ar << str;
               break;
            case RMV_IMAGE :
               str = pRMV->strFolder;
               ar << str;
               str = pRMV->strFile;
               ar << str;
               break;
         }

         // flicker parameters (as of schema 9, V4.1.0) -- applicable to all target types
         ar << pRMV->iFlickerOn << pRMV->iFlickerOff << pRMV->iFlickerDelay;
      }
   }
   else                                                                          // read from archive:
   {
      if( nSchema < 1 || nSchema > 9 )                                           // unsupported version
         ::AfxThrowArchiveException( CArchiveException::badSchema );

      ASSERT( ValidTargetType( m_type ) );                                       // validate target obj type

      if( m_type == CX_XYTARG )      m_pvParms = (PVOID) new XYPARMS;            // allocate storage IAW obj data type
      else if( m_type == CX_RMVTARG ) m_pvParms = (PVOID) new RMVTGTDEF;

      AssignDefaultValues();                                                     // init params w/ default values

      try
      {
         U_TGPARMS tgParms{};

         if( m_type == CX_XYTARG )                                               // read in relevant XY tgt params...
         {
            PXYPARMS pXY = &(tgParms.xy);
            ar >> pXY->type >> pXY->ndots >> pXY->fRectW >> pXY->fRectH;

            if( nSchema == 1 )
            {
               if( pXY->type == RECTANNU || pXY->type == FLOWFIELD || pXY->type == ORIENTEDBAR )
                  ar >> pXY->fInnerW >> pXY->fInnerH;
               else if( pXY->type == NOISYDIR || pXY->type == COHERENTFC )
                  ar >> pXY->fInnerW;
               if( pXY->type == NOISYDIR )                                       // to migrate older schema, must
                  pXY->fInnerH = MIN_NOISEUPD;                                   // init noise upd intv
            }
            else
            {
               if( pXY->type == RECTANNU || pXY->type == NOISYDIR || pXY->type == NOISYSPEED)
                  ar >> pXY->fInnerW >> pXY->fInnerH;
               else if( pXY->type == FLOWFIELD || pXY->type == ORIENTEDBAR || pXY->type == COHERENTFC )
                  ar >> pXY->fInnerW;
            }

            if( pXY->type == FCDOTLIFE || pXY->type == NOISYDIR || pXY->type == NOISYSPEED )
               ar >> pXY->iDotLfUnits >> pXY->fDotLife;

            pXY->fInnerX = pXY->fInnerY = 0.0f;                                  // handle new fields introduced in 
            if( pXY->type == RECTANNU && nSchema >= 4 )                          // schema version 4
               ar >> pXY->fInnerX >> pXY->fInnerY;
            
            if( pXY->type == NOISYSPEED && nSchema >= 5 )                        // new use for fInnerX in schema 5
               ar >> pXY->fInnerX;
            else
               pXY->fInnerX = 0.0f;
         }
         else if( m_type == CX_RMVTARG )                                         // read in relevant RMV tgt params...
         {
            PRMVTGTDEF pRMV = &(tgParms.rmv);

            if( nSchema < 3 )                                                    // read in old FB video target, then
            {                                                                    // convert to RMVideo target!
               FBPARMS fb;
               memset( &fb, 0, sizeof(FBPARMS) );
               ar >> fb.type >> fb.shape >> fb.fRectW >> fb.fRectH;
               for( int i=0; i<3; i++ ) ar >> fb.csMean[i];
               if( fb.type != PATCH )
               {
                  int i;
                  for( i=0; i<3; i++ ) ar >> fb.csCon[i];
                  for( i=0; i<2; i++ )
                  {
                     ar >> fb.fGratSF[i] >> fb.fGratAxis[i] >> fb.fGratPhase[i];
                     if( (fb.type == SINEGRAT) || (fb.type == SQUAREGRAT) || (fb.type == STATICGABOR) )
                        break;
                  }
                  if( fb.type == STATICGABOR ) ar >> fb.fSigma;
               }

               ConvertOldFBVideoToRMVideo( &fb, pRMV );
            }
            else
            {
               CString str;
               ar >> pRMV->iType;
               switch( pRMV->iType )
               {
                  case RMV_POINT :
                     ar >> pRMV->iRGBMean[0] >> pRMV->nDotSize;
                     break;
                  case RMV_RANDOMDOTS :
                     ar >> pRMV->iAperture >> pRMV->iFlags >> pRMV->iRGBMean[0];
                     
                     // as of schema 7, defn includes contrast to allow for two-color contrast dot patch. For
                     // pre-schema 7 targets, C is set to 0 to define a one-color patch
                     if(nSchema >= 7) ar >> pRMV->iRGBCon[0];
                     else pRMV->iRGBCon[0] = 0;

                     ar >> pRMV->fOuterW >> pRMV->fOuterH >> pRMV->fInnerW >> pRMV->fInnerH;
                     ar >> pRMV->nDots >> pRMV->nDotSize >> pRMV->iSeed >> pRMV->iPctCoherent;
                     ar >> pRMV->iNoiseUpdIntv >> pRMV->iNoiseLimit >> pRMV->fDotLife;
                     ar >> pRMV->fSigma[0] >> pRMV->fSigma[1];
                     
                     // prior to schema 5, speed noise granularity was 0.1%; now it's 1%
                     if((nSchema < 5) && ((pRMV->iFlags & RMV_F_DIRNOISE) == 0))
                        pRMV->iNoiseLimit /= 10;
                     break;
                  case RMV_FLOWFIELD :
                     ar >> pRMV->iRGBMean[0] >> pRMV->fOuterW >> pRMV->fInnerW;
                     ar >> pRMV->nDots >> pRMV->nDotSize >> pRMV->iSeed;
                     break;
                  case RMV_BAR :
                     ar >> pRMV->iRGBMean[0] >> pRMV->fOuterW >> pRMV->fOuterH >> pRMV->fDriftAxis[0];
                     break;
                  case RMV_SPOT :
                     ar >> pRMV->iAperture >> pRMV->iRGBMean[0];
                     ar >> pRMV->fOuterW >> pRMV->fOuterH >> pRMV->fInnerW >> pRMV->fInnerH;
                     ar >> pRMV->fSigma[0] >> pRMV->fSigma[1];
                     break;
                  case RMV_GRATING :
                     ar >> pRMV->iAperture >> pRMV->iFlags >> pRMV->iRGBMean[0] >> pRMV->iRGBCon[0];
                     ar >> pRMV->fOuterW >> pRMV->fOuterH >> pRMV->fSpatialFreq[0] >> pRMV->fDriftAxis[0];
                     ar >> pRMV->fGratPhase[0] >> pRMV->fSigma[0] >> pRMV->fSigma[1];
                     break;
                  case RMV_PLAID :
                     ar >> pRMV->iAperture >> pRMV->iFlags >> pRMV->iRGBMean[0] >> pRMV->iRGBMean[1];
                     ar >> pRMV->iRGBCon[0] >> pRMV->iRGBCon[1];
                     ar >> pRMV->fOuterW >> pRMV->fOuterH >> pRMV->fSpatialFreq[0] >> pRMV->fSpatialFreq[1];
                     ar >> pRMV->fDriftAxis[0] >> pRMV->fDriftAxis[1] >> pRMV->fGratPhase[0] >> pRMV->fGratPhase[1];
                     ar >> pRMV->fSigma[0] >> pRMV->fSigma[1];
                     break;
                  case RMV_MOVIE :
                     if(nSchema < 6) ::AfxThrowArchiveException( CArchiveException::badSchema );
                     ar >> pRMV->iFlags;
                     ar >> str;
                     if(str.GetLength() > RMV_MVF_LEN) 
                        ::AfxThrowArchiveException(CArchiveException::badSchema, "Media folder name too long!");
                     ::strncpy_s(pRMV->strFolder, (LPCTSTR) str, 32);
                     ar >> str;
                     if(str.GetLength() > RMV_MVF_LEN) 
                        ::AfxThrowArchiveException(CArchiveException::badSchema, "Media file name too long!");
                     ::strncpy_s(pRMV->strFile, (LPCTSTR) str, 32);
                     break;
                  case RMV_IMAGE :
                     if(nSchema < 8) ::AfxThrowArchiveException( CArchiveException::badSchema );
                     ar >> str;
                     if(str.GetLength() > RMV_MVF_LEN) 
                        ::AfxThrowArchiveException(CArchiveException::badSchema, "Media folder name too long!");
                     ::strncpy_s(pRMV->strFolder, (LPCTSTR) str, 32);
                     ar >> str;
                     if(str.GetLength() > RMV_MVF_LEN) 
                        ::AfxThrowArchiveException(CArchiveException::badSchema, "Media file name too long!");
                     ::strncpy_s(pRMV->strFile, (LPCTSTR) str, 32);
                     break;
               }

               // as of schema 9, target defn includes 3 flicker parameters
               if(nSchema >= 9)
                  ar >> pRMV->iFlickerOn >> pRMV->iFlickerOff >> pRMV->iFlickerDelay;
               else
                  pRMV->iFlickerOn = pRMV->iFlickerOff = pRMV->iFlickerDelay = 0;
            }
         }

         BOOL bChanged = FALSE;
         if( (m_type == CX_XYTARG) || (m_type == CX_RMVTARG) )                   // here we actually set the params,
            SetParams( &tgParms, bChanged );                                     //    with auto-correction.
      }
      catch( CException* e )                                                     // if any excpt occurs while loading
      {                                                                          // params, catch it so we can free
         UNREFERENCED_PARAMETER(e);                                              // param storage; then forward excpt!
         if( m_pvParms != NULL )
         {
            delete m_pvParms;
            m_pvParms = NULL;
         }
         throw;
      }
   }

   ASSERT_VALID( this );                                                         // check validity AFTER serializing
}


//=== ConvertOldFBVideoToRMVideo (static) =============================================================================
//
//    Translates the old VSG2/4-based framebuffer video target definition to a similar RMVideo target.  The RMVideo
//    "server" is an OpenGL-based application that runs on a separate Linux workstation and communicates with Maestro
//    over a private, dedicated Ethernet link.  It can take advantage of hardware acceleration on modern video cards,
//    making it a far superior and more flexible framebuffer video solution.  It replaced the VSG in Maestro v2.0.
//
//    RMVideo supports all of the old FB video targets.  Both RMV_GRATING and RMV_PLAID support a Gaussian window over
//    a drifting grating -- something which was not possible with the VSG.
//
//    ARGS:       pFB -- [in] pointer to the old FB video target defn.  Must not be null.
//                pRMV -- [out] pointer to RMVideo target defn to which the old FB video target is converted.  Must
//                        not be null.
//    RETURNS:    NONE.
//
VOID CCxTarget::ConvertOldFBVideoToRMVideo( PFBPARMS pFB, PRMVTGTDEF pRMV )
{
   ASSERT( pFB != NULL && pRMV != NULL );

   // clear the RMVideo target defn
   memset( pRMV, 0, sizeof(RMVTGTDEF) );

   // choose the analogout RMVideo target type.
   if( pFB->type == PATCH )
      pRMV->iType = RMV_SPOT;
   else if( pFB->type==SINEGRAT || pFB->type==SQUAREGRAT || pFB->type==STATICGABOR )
      pRMV->iType = RMV_GRATING;
   else
      pRMV->iType = RMV_PLAID;

   // FB video only supported rectangular or oval aperture, relevant to all FB tgt types
   pRMV->iAperture = (pFB->shape==RECTWIND) ? RMV_RECT : RMV_OVAL;

   // RMVideo tgt defn uses a flag to distinguish between sinewave and squarewave grating/plaid
   if( pFB->type==SQUAREGRAT || pFB->type==SQUAREPLAID || pFB->type==TWOSQGRATS )
      pRMV->iFlags |= RMV_F_ISSQUARE;

   // RMVideo tgt defn uses a flag to distinguish between a true plaid and one in which the gratings act independently
   if( pFB->type==TWOSINGRATS || pFB->type==TWOSQGRATS )
      pRMV->iFlags |= RMV_F_INDEPGRATS;

   // "mean" color applicable to all target types, but RMVideo restricts range of each cmpt to [0..255]!
   // Old FB did not support separate color specs for the two gratings of plaid targets.  Also, they were implemented
   // by frame interleaving, while RMVideo additively blends the two in a single frame.  So, for RMV_PLAID, we set
   // mean to 1/2 the old FB mean -- for both grating components.
   int r = cMath::rangeLimit((pFB->csMean[0] * 255) / FB_MAXLUM, 0, 255);
   int g = cMath::rangeLimit((pFB->csMean[1] * 255) / FB_MAXLUM, 0, 255);
   int b = cMath::rangeLimit((pFB->csMean[2] * 255) / FB_MAXLUM, 0, 255);
   if( pRMV->iType != RMV_PLAID )
   {
      pRMV->iRGBMean[0] = r + (g<<8) + (b<<16);
      pRMV->iRGBMean[1] = 0;
   }
   else
   {
      r /= 2;
      g /= 2;
      b /= 2;
      pRMV->iRGBMean[0] = r + (g<<8) + (b<<16);
      pRMV->iRGBMean[1] = pRMV->iRGBMean[0];
   }

   // contrast spec applies to all except the old PATCH tgt.  For RMV_PLAID, we apply same contrast to both
   // gratings -- old FB did not support separate color specs.
   if( pFB->type != PATCH )
   {
      r = cMath::rangeLimit(pFB->csCon[0], 0, 100);
      g = cMath::rangeLimit(pFB->csCon[1], 0, 100);
      b = cMath::rangeLimit(pFB->csCon[2], 0, 100);
      pRMV->iRGBCon[0] = r + (g<<8) + (b<<16);
      pRMV->iRGBCon[1] = (pRMV->iType==RMV_PLAID) ? pRMV->iRGBCon[0] : 0;
   }

   // copy bounding rect dimensions
   pRMV->fOuterW = pFB->fRectW;
   pRMV->fOuterH = pFB->fRectH;

   // copy the Gaussian window's standard deviation for the STATICGABOR target, which is handled by RMV_GRATING.
   // The STATICGABOR window was always circular, whereas RMVideo supports elliptical Gaussian windows.  Also note
   // that in RMVideo, it is possible to drift the grating under the window!
   if( pFB->type == STATICGABOR )
   {
      pRMV->fSigma[0] = pFB->fSigma;
      pRMV->fSigma[1] = pFB->fSigma;
   }

   // copy the grating parameters, as applicable
   if( pFB->type != PATCH ) for( int i=0; i<2; i++ )
   {
      pRMV->fSpatialFreq[i] = pFB->fGratSF[i];
      pRMV->fDriftAxis[i] = pFB->fGratAxis[i];
      pRMV->fGratPhase[i] = pFB->fGratPhase[i];
   }
}


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump contents of the target in an easy-to-read form to the supplied dump context.  Intelligent dump is tailored
//    to the object data type (predefined, XYScope, RMVideo) and, for XYScope or RMVideo targets, the specific target
//    type.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxTarget::Dump( CDumpContext& dc ) const
{
   CTreeObj::Dump( dc );

   PXYPARMS pXY;
   PRMVTGTDEF pRMV;
   CString str;
   switch( m_type )
   {
      case CX_CHAIR :      dc << "hard target CHAIR\n";     break;

      case CX_XYTARG :
         pXY = (PXYPARMS) m_pvParms;
         dc << "XY scope tgt type ";
         switch( pXY->type )
         {
            case RECTDOT :    dc << "RECTDOT";     break;
            case CENTER :     dc << "CENTER";      break;
            case SURROUND :   dc << "SURROUND";    break;
            case RECTANNU :   dc << "RECTANNU";    break;
            case FASTCENTER : dc << "FASTCENTER";  break;
            case FCDOTLIFE :  dc << "FCDOTLIFE";   break;
            case FLOWFIELD :  dc << "FLOWFIELD";   break;
            case ORIENTEDBAR: dc << "ORIENTEDBAR"; break;
            case NOISYDIR:    dc << "NOISYDIR";   break;
            case COHERENTFC:  dc << "COHERENTFC";  break;
            case NOISYSPEED:  dc << "NOISYSPEED";   break;
         }
         dc << " with " << pXY->ndots << " dots:\n";
         if( pXY->type == RECTDOT )
            dc << "   Width of rect array(deg) = " << pXY->fRectW << "; dot spacing(deg) = " << pXY->fRectH << "\n";
         else if( pXY->type == FLOWFIELD )
            dc << "   Outer,inner radii(deg) = " << pXY->fRectW << ", " << pXY->fInnerW << "\n";
         else
            dc << "   Bounding rect: " << pXY->fRectW << " x " << pXY->fRectH << " deg\n";
         if ( pXY->type == RECTANNU )
         {
            dc << "   Inner rect: " << pXY->fInnerW << " x " << pXY->fInnerH << " deg, ";
            dc << "at " << pXY->fInnerX << "," << pXY->fInnerY << "rel to tgt ctr\n";
         }
         if ( pXY->type == FCDOTLIFE || pXY->type == NOISYDIR || pXY->type == NOISYSPEED )
         {
            dc << "   Dot life limited to " << pXY->fDotLife;
            if ( pXY->iDotLfUnits == DOTLFINMS )
               dc << " msecs\n";
            else
               dc << " deg travelled\n";
            if( pXY->type == NOISYDIR || pXY->type == NOISYSPEED )
            {
               dc << "   Noise update interval is " << int(pXY->fInnerH) << " msec\n";
               if( pXY->type == NOISYDIR )
                  dc << "   Direction noise offset range is +/-" << int(pXY->fInnerW) << " deg\n";
               else if( pXY->fInnerX == 0.0f )
                  dc << "   Speed noise offset range is +/-" << int(pXY->fInnerW) << "% of nominal speed\n";
               else
                  dc << "   Speed noise multiplier is 2^x, x in [-N..N], where N = " << int(pXY->fInnerW) << " \n";
            }
         }
         if( pXY->type == ORIENTEDBAR )
            dc << "  Drift axis = " << pXY->fInnerW << " deg\n";
         if( pXY->type == COHERENTFC )
            dc << "   Percent coherence is " << int(pXY->fInnerW) << "%\n";
         break;

      case CX_RMVTARG :
         pRMV = (PRMVTGTDEF) m_pvParms;
         dc << "RMVideo tgt type ";
         switch( pRMV->iType )
         {
            case RMV_POINT :
               dc << "RMV_POINT with:\n";
               dc << "  dot size = " << pRMV->nDotSize << " pixels\n";
               dc << "  rgbColor = " << pRMV->iRGBMean[0] << "\n";
               break;
            case RMV_RANDOMDOTS :
               dc << "RMV_RANDOMDOTS with:\n";
               str = "rectangle";
               if( pRMV->iAperture == RMV_OVAL ) str = "ellipse";
               else if( pRMV->iAperture == RMV_RECTANNU ) str = "rectangular annulus";
               else if( pRMV->iAperture == RMV_OVALANNU ) str = "elliptical annulus";
               dc << "  aperture = " << str << "\n";
               dc << "  flags = " << pRMV->iFlags << "\n";
               dc << "  rgbMean = " << pRMV->iRGBMean[0] << "\n";
               dc << "  rgbCon = " << pRMV->iRGBCon[0] << "\n";
               dc << "  outerRect = " << pRMV->fOuterW << " by " << pRMV->fOuterH << " deg\n";
               dc << "  innerRect = " << pRMV->fInnerW << " by " << pRMV->fInnerH << " deg\n";
               dc << "  nDots = " << pRMV->nDots << ", dotSize = " << pRMV->nDotSize << " pixels\n";
               dc << "  seed for RNG = " << pRMV->iSeed << "\n";
               dc << "  coherence = " << pRMV->iPctCoherent << "%\n";
               dc << "  noise updIntv = " << pRMV->iNoiseUpdIntv << " ms\n";
               if( pRMV->iNoiseUpdIntv > 0 )
               {
                  if( (pRMV->iFlags & RMV_F_DIRNOISE) != 0 )
                     dc << "  directional noise limit = " << pRMV->iNoiseLimit << " deg\n";
                  else if( (pRMV->iFlags & RMV_F_SPDLOG2) == 0 )
                     dc << "  speed noise limit = " << pRMV->iNoiseLimit << " % of pattern speed\n";
                  else
                     dc << "  speed noise multiplier is 2^x, x in [-N..N], where N = " << pRMV->iNoiseLimit << " \n";
               }
               if( pRMV->fDotLife > 0.0f )
               {
                  str = ((pRMV->iFlags & RMV_F_LIFEINMS) != 0) ? " ms\n" : " deg travelled\n";
                  dc << "  dot life = " << pRMV->fDotLife << str;
               }
               dc << "  sigma in x,y = " << pRMV->fSigma[0] << "," << pRMV->fSigma[1] << " deg\n";
               break;
            case RMV_FLOWFIELD :
               dc << "RMV_FLOWFIELD with:\n";
               dc << "  rgbColor = " << pRMV->iRGBMean[0] << "\n";
               dc << "  inner diam = " << pRMV->fInnerW << ", outer = " << pRMV->fOuterW << " deg\n";
               dc << "  nDots = " << pRMV->nDots << ", dotSize = " << pRMV->nDotSize << " pixels\n";
               dc << "  seed for RNG = " << pRMV->iSeed << "\n";
               break;
            case RMV_BAR :
               dc << "RMV_BAR with:\n";
               dc << "  rgbColor = " << pRMV->iRGBMean[0] << "\n";
               dc << "  dimensions = " << pRMV->fOuterW << " by " << pRMV->fOuterH << " deg\n";
               dc << "  orientation angle = " << pRMV->fDriftAxis[0] << " deg CCW from pos x-axis\n";
               break;
            case RMV_SPOT :
               dc << "RMV_SPOT with:\n";
               str = "rectangle";
               if( pRMV->iAperture == RMV_OVAL ) str = "ellipse";
               else if( pRMV->iAperture == RMV_RECTANNU ) str = "rectangular annulus";
               else if( pRMV->iAperture == RMV_OVALANNU ) str = "elliptical annulus";
               dc << "  aperture = " << str << "\n";
               dc << "  rgbColor = " << pRMV->iRGBMean[0] << "\n";
               dc << "  outerRect = " << pRMV->fOuterW << " by " << pRMV->fOuterH << " deg\n";
               dc << "  innerRect = " << pRMV->fInnerW << " by " << pRMV->fInnerH << " deg\n";
               dc << "  sigma in x,y = " << pRMV->fSigma[0] << "," << pRMV->fSigma[1] << " deg\n";
               break;
            case RMV_GRATING :
               str = ((pRMV->iFlags & RMV_F_ISSQUARE) != 0) ? "Squarewave" : "Sinewave";
               dc << str << " RMV_GRATING with:\n";
               str = (pRMV->iAperture == RMV_RECT) ? "rectangle" : "ellipse";
               dc << "  aperture = " << str << "\n";
               dc << "  rgbMean = " << pRMV->iRGBMean[0] << ", rgbCon = " << pRMV->iRGBCon[1] << "\n";
               dc << "  dimensions = " << pRMV->fOuterW << " by " << pRMV->fOuterH << " deg\n";
               dc << "  freq = " << pRMV->fSpatialFreq[0] << " cyc/deg, drift axis = " << pRMV->fDriftAxis[0];
               dc << "  deg CCW, initial phase = " << pRMV->fGratPhase[0] << " deg\n";
               dc << "  sigma in x,y = " << pRMV->fSigma[0] << "," << pRMV->fSigma[1] << " deg\n";
               str = ((pRMV->iFlags & RMV_F_ORIENTADJ) != 0) ? "true" : "false";
               dc << "  orientation tracks pattern velocity vector direction = " << str << "\n";
               break;
            case RMV_PLAID :
               str = ((pRMV->iFlags & RMV_F_ISSQUARE) != 0) ? "Squarewave" : "Sinewave";
               dc << str << " RMV_GRATING with:\n";
               str = (pRMV->iAperture == RMV_RECT) ? "rectangle" : "ellipse";
               dc << "  aperture = " << str << "\n";
               dc << "  dimensions = " << pRMV->fOuterW << " by " << pRMV->fOuterH << " deg\n";
               dc << "  rgbMean = " << pRMV->iRGBMean[0] << "," << pRMV->iRGBMean[1] << "\n";
               dc << "  rgbCon = " << pRMV->iRGBCon[0] << "," << pRMV->iRGBCon[1] << "\n";
               dc << "  freq = " << pRMV->fSpatialFreq[0] << ", " << pRMV->fSpatialFreq[1] << " cyc/deg\n";
               dc << "  drift axis = " << pRMV->fDriftAxis[0] << ", " << pRMV->fDriftAxis[1] << " deg CCW\n";
               dc << "  initial phase = " << pRMV->fGratPhase[0] << ", " << pRMV->fGratPhase[1] << " deg\n";
               dc << "  sigma in x,y = " << pRMV->fSigma[0] << "," << pRMV->fSigma[1] << " deg\n";
               str = ((pRMV->iFlags & RMV_F_INDEPGRATS) != 0) ? "true" : "false";
               dc << "  plaid grating components are independent = " << str << "\n";
               str = ((pRMV->iFlags & RMV_F_ORIENTADJ) != 0) ? "true" : "false";
               dc << "  orientation tracks pattern velocity vector direction = " << str << "\n";
               break;
            case RMV_MOVIE :
               dc << "RMV_MOVIE with:\n";
               str.Format("Folder = %s, file = %s\n", pRMV->strFolder, pRMV->strFile);
               dc << str;
               str.Format("repeat = %s, pause_when_off = %s, playback_at_display_rate = %s\n",
                  ((pRMV->iFlags & RMV_F_REPEAT) != 0) ? "true" : "false",
                  ((pRMV->iFlags & RMV_F_PAUSEWHENOFF) != 0) ? "true" : "false",
                  ((pRMV->iFlags & RMV_F_ATDISPRATE) != 0) ? "true" : "false");
               dc << str;
               break;
            case RMV_IMAGE :
               dc << "RMV_IMAGE with:\n";
               str.Format("Folder = %s, file = %s\n", pRMV->strFolder, pRMV->strFile);
               dc << str;
               break;
         }
         str.Format("Flicker: ON=%d, OFF=%d, delay=%d (in video frames)\n", pRMV->iFlickerOn,
            pRMV->iFlickerOff, pRMV->iFlickerDelay);
         dc << str;
         break;
   }
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate the target object.  The memory allocated for modifiable parameters should be consistent with target
//    object's abstract data type.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxTarget::AssertValid() const
{
   CTreeObj::AssertValid();

   if      ( m_type == CX_XYTARG )  ASSERT( AfxIsValidAddress( m_pvParms, sizeof(XYPARMS) ) );
   else if ( m_type == CX_RMVTARG )  ASSERT( AfxIsValidAddress( m_pvParms, sizeof(RMVTGTDEF) ) );
   else                             ASSERT( m_pvParms == NULL );
}

#endif //_DEBUG




//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== AssignDefaultValues =============================================================================================
//
//    Sets the default values of a target's modifiable parameters (if any!).
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
VOID CCxTarget::AssignDefaultValues()
{
   ASSERT_VALID( this );

   if ( m_type == CX_XYTARG )                   // default XYScope tgt:
   {
      PXYPARMS pXY = (PXYPARMS) m_pvParms;
      pXY->type = RECTDOT;                      // a regularly spaced array of 10 dots, with width of 0.1 deg and
      pXY->ndots = 10;                          // spacing of 0 (so all dots are drawn on top of each other!)
      pXY->fRectW = float(0.1);
      pXY->fRectH = float(0.0);
      pXY->fInnerW = float(0);                  // remaining params do not apply to this XY tgt type...
      pXY->fInnerH = float(0);
      pXY->fDotLife = float(4);
      pXY->iDotLfUnits = DOTLFINMS;
      pXY->fInnerX = float(0);
      pXY->fInnerY = float(0);
   }
   else if ( m_type == CX_RMVTARG )             // default RMVideo tgt:  a white point-like tgt 2 pixels wide
   {
      PRMVTGTDEF pRMV = (PRMVTGTDEF) m_pvParms;
      memset( pRMV, 0, sizeof(RMVTGTDEF) );
      pRMV->iType = RMV_POINT;
      pRMV->iRGBMean[0] = 0x00FFFFFF;
      pRMV->nDotSize = 2;
      pRMV->iPctCoherent = 100;
   }
}

