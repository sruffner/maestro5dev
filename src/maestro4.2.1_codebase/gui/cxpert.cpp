//===================================================================================================================== 
//
// cxpert.cpp : Implementation of class CCxPert, encapsulating a CNTRLX "perturbation waveform" data object. 
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CNTRLX trials (class CCxTrial) allow the velocity trajectories of trial targets to be "perturbed" IAW a specified 
// perturbation waveform.  CCxPert is the data class (CNTRLX data type CX_PERTURB) encapsulating the perturbation's  
// definition.  Currently, CCxPert supports four types of perturbations -- sinuosidal, trapezoidal pulse, uniform 
// random noise, and Gaussian-distributed noise.
//
// ==> Usage.
// CCxPert has been designed to facilitate the presentation of individual perturbation parameters in a table or grid 
// control.  To that end, each parameter is identified by a zero-based index.  The first N parameters are common to 
// all perturbation types, while the remaining M parameters are specific to the perturbation object's current type.
// 
//    NumberOfCommonParameters() ==> #parameters common to all perturbation types (static).
//    MaxNumberOfParameters() ==> worst-case total# of parameters defining a perturbation (static).
//    NumberOfParameters() ==> total # of parameters defining this perturbation object.
//    IsValidParameter( int i ) ==> is parameter index i valid for this perturbation object.
//
//    GetParameter***( .. ) ==> These methods retrieve information about a parameter identified by its index.
//    SetParameter( .. )   ==> These overloaded method modify a particular perturbation parameter, w/ auto-correction.
//       If the change affects the value of any other parameter ("side effect"), the methods return TRUE.
//
//    Get/SetPertInfo() ==> Perturbation definitions must be delivered to CXDRIVER using the PERT data structure.
//       These two methods retrieve/modify the entire perturbation using this data structure.
//
// ==> Interaction with other CNTRLX classes.
// 1) Storage of CCxPert objects in the CNTRLX "object tree", CCxTreeMap, within a CNTRLX "experiment document", CCxDoc 
//    -- see implementation files for these classes!  Only CCxTreeMap can construct and copy perturbation objects; it 
//    is a friend class to CCxPert.
// 2) CCxPertForm -- the CNTRLX view class which displays all CCxPert objects that currently exist under the predefined 
//    CX_PERTBASE node of the open CCxDoc.  It permits the user to modify perturbation defns in various ways.
//
// ==> Importing defn from an ASCII text file.
// CNTRLX succeeds the cross-platform cntrlxUNIX/PC application, in which the GUI was hosted on a UNIX workstation
// ("cntrlxUNIX") and the hardware controller resided on a WindowsNT PC ("cntrlxPC").  In that system, the various data 
// objects (targets, channel configurations, trials, etc.) could be defined in ASCII-text "definition files".  CNTRLX 
// supports importing CNTRLX data objects from such definition files via the dedicated CCxImportDialog.  This dialog 
// is responsible for interacting with the user, opening the text files and reading the definitions into an array of 
// CString's, and creating new data objects as appropriate.  Each data class provides an Import() method that takes 
// a CStringArray and reinitializes itself IAW the definition contained therein.  Thus, the details of translating the 
// cntrlxUNIX-style text definition to the CNTRLX data object is encapsulated in the data object itself, but the 
// details of opening text files and interacting with the user are handled by a user-interface object.
//
// In the case of the perturbation waveform object, cntrlxUNIX "perturbation files" defined one or more perturbations.
// Thus, CCxImportDialog is responsible for parsing out the lines defining each individual perturbation waveform, 
// creating a new CCxPert object, then invoking CCxPert::Import() to complete the import.  NOTE that cntrlxUNIX 
// perturbation support is quite different from what CNTRLX provides -- not all perturbation types available in 
// cntrlxUNIX are available in CNTRLX, and vice versa.  See CCxPert::Import() for details.
//
// REVISION HISTORY:
// 08nov2002-- Began development, modelling CCxPert after CCxStimulus.  However, CCxPert is derived from CTreeObj b/c 
//             it is designed to appear in the CNTRLX object tree.
// 05dec2002-- Adding Import() to support importing CCxPert defn from a cntrlxUNIX-style "perturbation" defn file.
// 15mar2005-- Added CopyRemoteObj() override.
// 28jul2005-- Updated to support new perturbation type PERT_ISGAUSS, and to handle a new "seed" parameter for both  
//             random noise perturbation types.  Schema version 2.
// 01sep2015-- Modified GetParameterFormat() to allow perturbation durations up to 99999 (5 digits instead of 4).
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource defines

#include "cxpert.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_SERIAL( CCxPert, CTreeObj, 2 | VERSIONABLE_SCHEMA )


//===================================================================================================================== 
// PRIVATE CONSTANTS & GLOBALS
//===================================================================================================================== 

const int   CCxPert::NPARAMS[] = { 2, 3, 3, 3 };
LPCTSTR     CCxPert::TYPESTRINGS[] = { _T("sinusoid"), _T("pulse train"), _T("uniform noise"), _T("gaussian noise") };
LPCTSTR     CCxPert::COMMONLBLS[] = { _T("Type"), _T("Dur(ms)") };

CRand16 CCxPert::m_seedRNG;                              // static member must be defined at file scope!



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== Initialize [base override] ====================================================================================== 
//
//    Initialize CNTRLX data/collection object after default construction (MUST be called directly after default 
//    construction to initialize the object IAW the specified name, CNTRLX object type, and state flags).
//
//    The perturbation parameters are set to a default state.
//
//    ARGS:       s  -- [in] the name assigned to target object
//                t  -- [in] the CNTRLX object data type -- MUST be a recognized CNTRLX target type 
//                f  -- [in] the object's initial state flags -- CANNOT include CX_ISSETOBJ. 
//
//    RETURNS:    NONE.
//
//    THROWS:     NONE.
//
VOID CCxPert::Initialize( LPCTSTR s, const WORD t, const WORD f )
{
   ASSERT( t == CX_PERTURB );                      // validate object type and flags
   ASSERT( (f & CX_ISSETOBJ) == 0 );

   SetDefaults();
   CTreeObj::Initialize( s, t, f );                // base class inits
}


//=== Copy [base override] ============================================================================================ 
//
//    Copy a CNTRLX perturbation waveform object.
//
//    ARGS:       pSrc  -- [in] CTreeObj* ptr to the channel cfg to be copied.  MUST point to a valid CCxPert obj! 
//
//    RETURNS:    NONE.
//
VOID CCxPert::Copy( const CTreeObj* pSrc )
{
   ASSERT( pSrc != NULL );
   ASSERT( pSrc->IsKindOf( RUNTIME_CLASS( CCxPert ) ) );

   const CCxPert* pSrcPert = (CCxPert*) pSrc;
   ASSERT_VALID( pSrcPert );

   CTreeObj::Initialize( pSrcPert->m_name, pSrcPert->m_type, pSrcPert->m_flags ); 

   m_iType = pSrcPert->m_iType;
   m_iDur = pSrcPert->m_iDur;
   m_sine = pSrcPert->m_sine;
   m_train = pSrcPert->m_train;
   m_noise = pSrcPert->m_noise;
}


//=== CopyRemoteObj [base override] =================================================================================== 
//
//    Copies the CCxPert-specific definition of a perturbation object located in a different experiment document.
//
//    CopyRemoteObject was introduced to the CTreeObj/CTreeMap framework to overcome the problem of copying an object 
//    from one treemap to another.  It is intended only for copying the internal information specific to a given 
//    implementation of CTreeObj.
//
//    ARGS:       pSrc        -- [in] the object to be copied.  Must be an instance of CCxPert.
//                depKeyMap   -- [in] maps keys of any objects upon which the source obj depends, which reside in the 
//                               source doc, to the keys of the corresponding objects in the destination doc.
//
//    RETURNS:    TRUE if successful, FALSE if source object is not an instance of CCxPert.
//
BOOL CCxPert::CopyRemoteObj(CTreeObj* pSrc, const CWordToWordMap& depKeyMap)
{
   if( pSrc == NULL || !pSrc->IsKindOf( RUNTIME_CLASS(CCxPert) ) ) return( FALSE );

   const CCxPert* pSrcPert = (CCxPert*) pSrc;
   ASSERT_VALID( pSrcPert );

   m_iType = pSrcPert->m_iType;
   m_iDur = pSrcPert->m_iDur;
   m_sine = pSrcPert->m_sine;
   m_train = pSrcPert->m_train;
   m_noise = pSrcPert->m_noise;

   return( TRUE );
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== GetRandomSeed =================================================================================================== 
//
//    Get a nonzero randomized seed value to seed the RNG for one of the noise perturbations.
//
//    When the author specifies a seed value of 0 for any of the noise perturbations (PERT_ISNOISE, PERT_ISGAUSS), 
//    then a random seed is chosen each time the perturbation is used in a trial.  This static method provides that 
//    random seed.  Internally, it concatenates two 16-bit random WORDs to form a 32-bit integer.
//
//    ARGS:       NONE.
//
//    RETURNS:    A nonzero pseudo-random 32-bit integer.
//
int CCxPert::GetRandomSeed()
{
   int nSeed = 0;
   while( nSeed == 0 ) nSeed = (int) MAKELONG(m_seedRNG.Generate(), m_seedRNG.Generate());

   return( nSeed );
} 


//=== GetPertInfo, SetPertInfo ======================================================================================== 
//
//    Get/set the perturbation waveform definition.
//
//    CXDRIVER uses a formatted data structure, PERT, to hold the relevant parameters IAW perturbation type.  CCxPert 
//    uses a very similar storage scheme, except that it maintains separate copies of each type-specific parameter set 
//    so that the object's type can be freely changed without having to revalidate the relevant parameter set. 
//
//    ARGS:       pert  -- [in/out] the stimulus channel definition struct compatible with CXDRIVER. 
//
//    RETURNS:    NONE. 
//
VOID CCxPert::GetPertInfo( PERT& pert ) const
{
   pert.iType = m_iType;
   pert.iDur = m_iDur;

   if( m_iType == PERT_ISSINE )        pert.sine = m_sine;
   else if( m_iType == PERT_ISTRAIN )  pert.train = m_train;
   else                                pert.noise = m_noise;
}

VOID CCxPert::SetPertInfo( const PERT& pert )
{
   m_iType = pert.iType;
   m_iDur = pert.iDur;

   if( pert.iType == PERT_ISSINE )        m_sine = pert.sine;
   else if( pert.iType == PERT_ISTRAIN )  m_train = pert.train;
   else                                   m_noise = pert.noise;

   Validate();                                                       // autocorrect the new defn
}


//=== Serialize [base override] ======================================================================================= 
//
//    Handles reading/writing the perturbation object from/to a disk file via a serialization archive, including 
//    version control.  Note that, after deserialization, any invalid parameters are auto-corrected. 
//
//    Version Control (using MFC "schema"):
//       1: Base version.
//       2: Added parameter 'iSeed' to NOISEPERT, representing the seed that initializes the underlying random number
//          generator (as of Maestro v1.3.2).  The seed applies to noise perts PERT_ISNOISE and PERT_ISGAUSS.
//
//    ARGS:       ar -- [in] the serialization archive.  
//
//    RETURNS:    NONE. 
//
//    THROWS:     -- The archive may throw CMemoryException, CArchiveException, or CFileException.
//                -- We throw CArchiveException if schema number is not recognized.
//
void CCxPert::Serialize ( CArchive& ar )
{
   UINT nSchema = ar.GetObjectSchema();                                          // retrieve schema# 
   CTreeObj::Serialize( ar );                                                    // serialize base class stuff first

   if( ar.IsStoring() )                                                          // STORE TO ARCHIVE...
   {
      ar << m_iType << m_iDur;
      if( m_iType == PERT_ISSINE ) 
         ar << m_sine.iPeriod << m_sine.fPhase;
      else if( m_iType == PERT_ISTRAIN )
         ar << m_train.iPulseDur << m_train.iRampDur << m_train.iIntv;
      else if( m_iType == PERT_ISNOISE || m_iType == PERT_ISGAUSS )
         ar << m_noise.iUpdIntv << m_noise.fMean << m_noise.iSeed;
   }
   else                                                                          // READ FROM ARCHIVE...
   {
      if( nSchema < 1 || nSchema > 2 )                                           // unsupported version
         ::AfxThrowArchiveException( CArchiveException::badSchema );

      SetDefaults();
      ar >> m_iType >> m_iDur;
      if( m_iType == PERT_ISSINE )
         ar >> m_sine.iPeriod >> m_sine.fPhase;
      else if( m_iType == PERT_ISTRAIN )
         ar >> m_train.iPulseDur >> m_train.iRampDur >> m_train.iIntv;
      else if( m_iType == PERT_ISNOISE || m_iType == PERT_ISGAUSS )
      {
         ar >> m_noise.iUpdIntv >> m_noise.fMean;
         if( nSchema >= 2 )                                                      // ver 2:  Added NOISEPERT.iSeed.  For 
            ar >> m_noise.iSeed;                                                 // earlier docs, defaults to 0 (seed
         else                                                                    // is randomly chosen)
            m_noise.iSeed = 0;
      }

      Validate();                                                                // validate the pert defn just read!
   }

   ASSERT_VALID( this );                                                         // check validity AFTER serializing
}


//=== Import ========================================================================================================== 
//
//    Reinitialize the perturbation object IAW a cntrlxUNIX-style, text-based definition.
//
//    CntrlxUNIX was the GUI side of CNTRLX's predecessor, a dual-platform application with the GUI running on a UNIX 
//    workstation and the hardware controller hosted on a Windows PC.  To facilitate the move from cntrlxUNIX/PC to 
//    CNTRLX, CNTRLX provides support for reading cntrlxUNIX object definition files.
//
//    In the case of perturbation objects, multiple objects could be defined in a single definition file.  The caller 
//    is responsible for reading and parsing this file into individual definitions, each stored in a separate 
//    CStringArray.  This method redefines the CCxPert object IAW the cntrlxUNIX-style definition.  It recognizes three 
//    of the five types of **velocity** perturbations defined in cntrlxUNIX and maps them as follows:
//
//       cntrlxUNIX type      CNTRLX type
//       ---------------      -----------
//        "sines"             PERT_ISSINE
//        "pulse"             PERT_ISTRAIN
//        "ramps"             PERT_ISTRAIN
//        "steps"             not supported
//        "G*sines"           not supported
//
//    The methods expects the following definition formats for each of the three supported cntrlxUNIX perturbation 
//    types (each line here represents one text string in the CStringArray argument):
//
//       PERTURBATION <i> sines              PERTURBATION <i> pulse              PERTURBATION <i> ramps
//       PULSE <n>                           PULSE <n>                           PULSE <n>
//       AMPLITUDE <f>                       AMPLITUDE <f>                       VELOCITY <f>
//       PERIOD <T>                          ADURATION <D>                       ADURATION <D>
//       PHASE <phi>                         VDURATION <R>                       VDURATION <R>
//       CYCLES <M> 
//       DC <A>
//
//    where <i> is a "perturbation channel #" (ignored here) and:
//
//       <n> = (int) marker pulse channel that's toggled when perturbation starts      IGNORED -- not supported
//       <f> = (float) amplitude of sinusoid, pulse, or ramp in deg/sec                IGNORED -- unit amplitude only 
//       <T> = (int) period of sinusoid in ms                                          PERT.sine.iPeriod
//       <phi> = (int) phase of sinusoid in whole deg                                  PERT.sine.fPhase
//       <M> = (int) # of complete cycles in sinusoidal perturbation                   PERT.iDur = M * T
//       <A> = (int) DC offset of sinusoid in deg/s                                    IGNORED -- not supported
//       <D> = (int) duration of accel/decel ramps in trapezoidal pulse in ms          PERT.train.iRampDur
//       <R> = (int) duration of constant-velocity phase of trapezoidal pulse, in ms   PERT.train.iPulseDur
//
//    Other than the first line, one or more lines in the definition can be omitted -- in that case, we just resort to 
//    default parameter values and make any auto-corrections as need be.  CNTRLX places restrictions on certain param 
//    values (see Validate()) that were not present in cntrlxUNIX, so we cannot guaranteed that the resulting pert obj 
//    will be identical to the cntrlxUNIX-style definition...  If the import fails, the perturbation object is restored 
//    to its initial state.
//
//    ARGS:       strArDefn   -- [in] the cntrlxUNIX-style definition as a series of text strings.
//                strMsg      -- [out] if there's an error, this should contain brief description of the error.
//
//    RETURNS:    TRUE if import successful; FALSE otherwise.
//
//    THROWS:     NONE.
//
BOOL CCxPert::Import( CStringArray& strArDefn, CString& strMsg )
{
   CCxPert savePert;                                                    // save current state in case import fails
   savePert.Copy( this );

   SetDefaults();                                                       // initialize all params to defaults

   PERT newDef;                                                         // get def defn in CXDRIVER-compatible format 
   ::ZeroMemory( (PVOID) &newDef, sizeof(PERT) ); 
   GetPertInfo( newDef );

   int i, j;
   char type[10];                                                       // holds cntrlxUNIX pert type name

   BOOL bOk = (strArDefn.GetSize() > 0);                                // empty defn is not acceptable
   if( bOk )                                                            // parse first line to get perturbation type
   {
      j = ::sscanf_s(strArDefn[0], "PERTURBATION %d %9s", &i, type, 10);
      bOk = BOOL(j==2);
      if( bOk )                                                         // map cntrlxUNIX pert type to CNTRLX type...
      {
         if( ::strcmp( type, "sines" ) == 0 ) 
            newDef.iType = PERT_ISSINE;
         else if( ::strcmp( type, "pulse" ) == 0 || 
                  ::strcmp( type, "ramps" ) == 0 )
            newDef.iType = PERT_ISTRAIN;
         else                                                           // ... and abort if it's an unsupported type
         {
            strMsg.Format( "Unsupported perturbation type: %s", type );
            Copy( &savePert ); 
            return( FALSE );
         }
      }
   }

   char param[12];                                                      // holds pert parameter name
   int nCycles = 1;                                                     // #cycles in cntrlxUNIX "sines" perturbation

   i = 1;
   while( bOk && i < strArDefn.GetSize() )                              // parse remaining definition IAW type
   { 
      int n = ::sscanf_s(strArDefn[i], "%11s %d", param, 12, &j);       //    parse next line for param name, value
      ++i;
      if( n < 1 ) { bOk = FALSE; break; }                               //    format error!!

      BOOL bSkip = BOOL( ::strcmp( param, "PULSE" ) == 0 ||             //    skip these params, which are ignored
                         ::strcmp( param, "AMPLITUDE" ) == 0 ||
                         ::strcmp( param, "VELOCITY" ) == 0 ||
                         ::strcmp( param, "DC" ) == 0 );
      if( bSkip ) continue;

      if( (!bSkip) && (n != 2) ) { bOk = FALSE; break; }                //    error -- did not read in param value 

      if( ::strcmp( param, "PERIOD" ) == 0 )                            //    assign parameter value appropriately...
      {
         if( newDef.iType == PERT_ISSINE ) newDef.sine.iPeriod = j;
         else bOk = FALSE;
      }
      else if( ::strcmp( param, "PHASE" ) == 0 )
      {
         if( newDef.iType == PERT_ISSINE ) newDef.sine.fPhase = float(j);
         else bOk = FALSE;
      }
      else if( ::strcmp( param, "CYCLES" ) == 0 )
      {
         if( newDef.iType == PERT_ISSINE ) nCycles = j;
         else bOk = FALSE;
      }
      else if( ::strcmp( param, "ADURATION" ) == 0 )
      {
         if( newDef.iType == PERT_ISTRAIN ) newDef.train.iRampDur = j;
         else bOk = FALSE;
      }
      else if( ::strcmp( param, "VDURATION" ) == 0 )
      {
         if( newDef.iType == PERT_ISTRAIN ) newDef.train.iPulseDur = j;
         else bOk = FALSE;
      }
      else                                                              //       unrecognized parameter name!!
         bOk = FALSE;
   }

   if( bOk )                                                            // if successful, set pert obj IAW new defn, 
   {                                                                    // performing any necessary corrections, then 
      SetPertInfo( newDef );                                            // adjust duration appropriately...
      GetPertInfo( newDef );
      if( newDef.iType == PERT_ISSINE )
         newDef.iDur = ((nCycles > 0) ? nCycles : 1) * newDef.sine.iPeriod;
      else
         newDef.iDur = newDef.train.iPulseDur + 2 * newDef.train.iRampDur;
      SetParameter( 1, double(newDef.iDur) );
   }

   if( !bOk )                                                           // if unsuccessful, there must have been a 
   {                                                                    // format error.  restore original state.
      Copy( &savePert );
      strMsg = _T("Unrecognized format");
   }
   return( bOk );
}


//=== Get/SetParameter... ============================================================================================= 
//
//    This group of methods provides generalized access to the perturbation's entire parameter list, including both 
//    common and type-specific parameters.  An individual parameter is identified by a zero-based index.  The methods 
//    provide enough information so that a view class can display and edit any parameter without requiring hard-coded 
//    knowledge of the parameter's identity, which changes IAW perturbation type.
//
//    All parameters fall into one of three classes:  a floating-point number, an integer, or a multiple-choice value. 
//    A multiple-choice value is merely an integer having a limited range [0..N-1], where N is the # of choices 
//    available; note that a BOOLean parameter can be thought of as a multiple-choice value with 0->FALSE, 1->TRUE.
//
//    A view class can retrieve the value of any parameter as a double, integer, or a CString via the GetParameter() 
//    and GetParameterAsInt() methods.  The CString form is best for ensuring that the current value is displayed in 
//    the most sensible fashion.  This is particularly important for multiple-choice parameters, since the CString 
//    value is a text label that is more meaningful than the zero-based choice index!  To edit the parameter, the view 
//    class should invoke GetParameterFormat() to obtain a numeric parameter's format constraints, or the set of 
//    available choices for a multi-choice parameter.  IsParameterMultiChoice() returns TRUE if parameter is multiple 
//    choice.  GetParameterLabel() provides a descriptive name (<=12 chars) of the specified parameter.  Finally, 
//    SetParameter() changes the current value of a parameter with built-in auto-correction.  The overloaded version 
//    accepting integer values is suitable only for int-valued parameters, while the version accepting float values 
//    works for all parameters.
//
//    DEVNOTE:  Auto-correction of parameter values is handled by Validate().  SetParameter() merely sets the specified 
//    parameter to the new value and calls Validate().  This is not efficient, since Validate() checks all relevant 
//    parameters -- not just the one changed.  However, with this approach we keep all of the auto-correction code in 
//    ONE method.
//
//    "Side effects" of SetParameter():  In certain cases, changing a particular parameter can indirectly cause a 
//    change in another parameter.  The primary example of such a side effect involves changing perturbation type, 
//    which usually changes the makeup of the perturbation's parameter set.  Whenever a parameter change could possibly 
//    have such a side effect, the SetParameter() routine returns TRUE.
//
//    If the parameter index does not specify a valid parameter for the current perturbation type:  GetParameter() and 
//    GetParameterAsInt() return 0, GetParameterLabel() retrieves an empty string, GetParameterFormat() retrieves a 
//    multi-choice parameter with no choices, IsParameterMultiChoice() returns FALSE, and SetParameter() has no effect.
//
//    The table below maps the zero-based "parameter index" to the parameter's identity.  The first two indices refer 
//    to parameters that are common to all perturbation types.  Indices >=2 refer to type-specific parameters.  Note 
//    that PERT_ISNOISE and PERT_ISGAUSS types have the same parameters.
//
//    index    PERT_ISSINE          PERT_ISTRAIN               PERT_ISNOISE and PERT_ISGAUSS 
//    -------------------------------------------------------------------------------------------------------------
//    0        m_iType              m_iType                    m_iType 
//    1        m_iDur               m_iDur                     m_iDur 
//
//    2        m_sine.iPeriod       m_train.iPulseDur          m_noise.iUpdIntv
//    3        m_sine.fPhase        m_train.iRampDur           m_noise.fMean
//    4        NOT USED             m_train.iIntv              m_noise.iSeed
//
//    ARGS:       i           -- [in] the index of desired parameter in the perturbation's parameter list. 
//                str         -- [out] string representation of parameter's value, or a descriptive name for parameter. 
//                bIsChoice   -- [out] TRUE if this is a multi-choice parameter; otherwise it's numeric.
//                choices     -- [out] set to available choices for a multi-choice parameter; else empty.
//                fmt         -- [out] numeric format constraints for a numeric parameter.
//                iVal, dVal  -- [in] new value for parameter.
//
//    RETURNS:    various
//
double CCxPert::GetParameter( int i ) const
{
   if( !IsValidParameter( i ) ) return( 0.0 );                       // merely return 0 if param index invalid

   double d = 0.0;                                                   // return each parameter formatted as a double
   switch( i )
   {
      case 0 : d = double(m_iType);    break;
      case 1 : d = double(m_iDur);     break;

      case 2 : 
         if( m_iType == PERT_ISSINE )        d = double(m_sine.iPeriod);
         else if( m_iType == PERT_ISTRAIN )  d = double(m_train.iPulseDur);
         else                                d = double(m_noise.iUpdIntv);
         break;
      case 3 :
         if( m_iType == PERT_ISSINE )        d = double(m_sine.fPhase);
         else if( m_iType == PERT_ISTRAIN )  d = double(m_train.iRampDur);
         else                                d = double(m_noise.fMean);
         break;
      case 4 :
         if( m_iType == PERT_ISTRAIN )       d = double(m_train.iIntv);
         else if( m_iType != PERT_ISSINE )   d = double(m_noise.iSeed);
         break;
      
   }

   return( d );
}

VOID CCxPert::GetParameter( int i, CString& str ) const
{
   str.Empty();
   if( !IsValidParameter( i ) ) return;                        // invalid parameter returned as empty string!

   BOOL bIsChoice = FALSE;                                     // get parameter display format
   CStringArray choices;
   NUMEDITFMT fmt;
   GetParameterFormat( i, bIsChoice, choices, fmt );

   if( bIsChoice )                                             // set parameter value as string IAW format
      str = (LPCTSTR) choices.GetAt( GetParameterAsInt( i ) );
   else if( (fmt.flags & NES_INTONLY) != 0 )
      str.Format( "%d", GetParameterAsInt( i ) );
   else
      str.Format( "%.*f", fmt.nPre, GetParameter( i ) );
}

int CCxPert::GetParameterAsInt( int i ) const
{
   return( int(0.5 + GetParameter( i )) );
}

VOID CCxPert::GetParameterLabel( int i, CString& str ) const         // all parameter labels <= 12 characters!
{
   str.Empty();
   if( !IsValidParameter( i ) ) return;                              // empty title string for an invalid parameter! 

   switch( i )
   {
      case 0 : 
      case 1 : 
         str = CCxPert::GetCommonParamLabel( i ); 
         break;

      case 2 :
         if( m_iType == PERT_ISSINE )        str = _T("Period(ms)"); 
         else if( m_iType == PERT_ISTRAIN )  str = _T("PulsDur(ms)"); 
         else                                str = _T("UpdIntv(ms)");
         break;
      case 3 :
         if( m_iType == PERT_ISSINE )        str = _T("Phase(deg)"); 
         else if( m_iType == PERT_ISTRAIN )  str = _T("RampDur(ms)"); 
         else                                str = _T("Mean Lvl");
         break;
      case 4 :
         if( m_iType == PERT_ISTRAIN )       str = _T("Intv(ms)"); 
         else if( m_iType != PERT_ISSINE )   str = _T("Seed(0=auto)");     // if seed=0, a different seed is randomly 
         break;                                                            // chosen each time noise pert is played
   }
}

VOID CCxPert::GetParameterFormat( int i, BOOL& bIsChoice, CStringArray& choices, NUMEDITFMT& fmt ) const
{
   choices.RemoveAll();
   bIsChoice = TRUE;                                                    // if parameter index invalid, format param 
   if( !IsValidParameter( i ) ) return;                                 // as multi-choice w/ an empty choice set!!

   if( i == 0 )                                                         // 0) perturbation type -- multiple-choice
   {
      for( int j=0; j < PERT_NTYPES; j++ )
         choices.Add( CCxPert::TYPESTRINGS[j] );
   }
   else                                                                 // all other params are numeric...
   {
      bIsChoice = FALSE;                                                // these default attributes apply to all 
      fmt.flags = NES_INTONLY | NES_NONNEG;                             // numeric parameters other than those 
      fmt.nPre = 1;                                                     // handled below...
      fmt.nLen = 4;
      if(i == 1)                                                        // pert dur range: [0..99999]
         fmt.nLen = 5;
      if( i == 3 && (m_iType==PERT_ISSINE  || m_iType==PERT_ISNOISE) )  // FP params: phase for sinewave, mean 
      {                                                                 // level for noise pert
         fmt.flags = 0;
         fmt.nPre = 2;
         fmt.nLen = (m_iType == PERT_ISSINE) ? 7 : 5;
      }
      else if(i==4 && (m_iType==PERT_ISNOISE || m_iType==PERT_ISGAUSS)) // seed for noise perts can be negative
      {                                                                 // ensure length can handle allowed range  
         fmt.flags = NES_INTONLY;                                       // of seed values [-9999999 .. 10000000]
         fmt.nLen = 8; 
      }
   }
}

BOOL CCxPert::IsParameterMultiChoice( int i ) const
{
   return( i == 0 );                                                    // only pert type is multi-choice
}

BOOL CCxPert::SetParameter( int i, double dVal )
{
   if( !IsValidParameter( i ) ) return( FALSE );                        // do nothing if parameter index invalid

   int iVal = int( dVal + ((dVal>=0) ? 0.5 : -0.5) );                   // integer version is rounded value

   BOOL bSideEffect = (i == 0) ||                                       // pert type affects index<->param mapping
         (m_iType == PERT_ISTRAIN && (i==2 || i==3));                   // changing pulse or ramp dur of pulse train 
                                                                        // can affect value of pulse interval
   switch( i )
   {
      case 0 : m_iType = iVal; break;
      case 1 : m_iDur = iVal;  break; 

      case 2 : 
         if( m_iType == PERT_ISSINE )        m_sine.iPeriod = iVal;
         else if( m_iType == PERT_ISTRAIN )  m_train.iPulseDur = iVal; 
         else                                m_noise.iUpdIntv = iVal;
         break;
      case 3 : 
         if( m_iType == PERT_ISSINE )        m_sine.fPhase = float(dVal);
         else if( m_iType == PERT_ISTRAIN )  m_train.iRampDur = iVal; 
         else                                m_noise.fMean = float(dVal);
         break;
      case 4 : 
         if( m_iType == PERT_ISTRAIN )       m_train.iIntv = iVal; 
         else if( m_iType != PERT_ISSINE )   m_noise.iSeed = iVal;
         break;

      default:                                                          // we should NEVER get here!
         ASSERT( FALSE );
         break;
   }

   Validate();                                                          // auto-correct any invalid entry
   return( bSideEffect );
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump perturbation waveform definiton in an easy-to-read form to the supplied dump context. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CCxPert::Dump( CDumpContext& dc ) const
{
   CTreeObj::Dump( dc );

   dc << _T("Perturbation, dur(ms)= ") << m_iDur << _T(": ");
   switch( m_iType )
   {
      case PERT_ISSINE :
         dc << _T("sinewave; period(ms)= ") << m_sine.iPeriod << _T(" phase(deg)= ") << m_sine.fPhase << _T("\n");
         break;
      case PERT_ISTRAIN :
         dc << _T("pulsetrain; pulseDur(ms)= ") << m_train.iPulseDur << _T(" rampDur(ms)= ") << m_train.iRampDur;
         dc << _T(" intv(ms)= ") << m_train.iIntv << _T("\n");
         break;
      case PERT_ISNOISE :
         dc << _T("uniform random noise; updIntv(ms)= ");
         dc << m_noise.iUpdIntv << _T(" mean= ") << m_noise.fMean << _T(" seed= ") << m_noise.iSeed << _T("\n");
         break;
      case PERT_ISGAUSS :
         dc << _T("gaussian random noise; updIntv(ms)= ");
         dc << m_noise.iUpdIntv << _T(" mean= ") << m_noise.fMean << _T(" seed= ") << m_noise.iSeed << _T("\n");
         break;
   }
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate the object's state.  
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxPert::AssertValid() const
{
   CTreeObj::AssertValid();
}

#endif //_DEBUG




//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== SetDefaults ===================================================================================================== 
//
//    Assign default values to all perturbation waveform definition parameters. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
VOID CCxPert::SetDefaults() 
{
   m_iType = PERT_ISSINE; 
   m_iDur = 1000;

   m_sine.iPeriod = 1000;              // sinewave: period (>=10ms), 
   m_sine.fPhase = 0.0f;               //    phase (in [-180.0..180.0])

   m_train.iPulseDur = 300;            // trapezoidal pulse train:  pulse dur (>= 10ms), 
   m_train.iRampDur  = 50;             //    duration of rising-edge and falling-edge ramps (>= 0ms),
   m_train.iIntv = 500;                //    pulse interval (> 2*rampD + pulsD)

   m_noise.iUpdIntv = 50;              // uniform or gaussian random noise:  intv between updates (>= 1ms),
   m_noise.fMean = 0.0f;               //    noise mean level (in [-1.0 .. 1.0]), zero seed (which means that seed 
   m_noise.iSeed = 0;                  //    is randomly chosen each time perturbation is used)
}


//=== Validate ======================================================================================================== 
//
//    Validate the current perturbation waveform definition. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
VOID CCxPert::Validate() 
{
   if( m_iType < 0 ) m_iType = PERT_NTYPES - 1;                         // perturbation type, T=[0..#types-1]; out-of-
   else if( m_iType >= PERT_NTYPES ) m_iType = 0;                       // range values "wrap around"

   if( m_iDur < 10 ) m_iDur = 10;                                       // dur of perturbation waveform (>= 10ms)

   if( m_iType == PERT_ISSINE )                                         // for sinewave pert:
   {
      if( m_sine.iPeriod < 10 ) m_sine.iPeriod = 10;                    //    period >= 10ms
      while( m_sine.fPhase < -180.0f ) m_sine.fPhase += 360.0f;         //    phase: restrict to [-180..180] deg 
      while( m_sine.fPhase > 180.0f ) m_sine.fPhase -= 360.0f; 
   }
   else if( m_iType == PERT_ISTRAIN )                                   // for pulsetrain pert:
   {
      if( m_train.iPulseDur < 10 ) m_train.iPulseDur = 10;              //    pulse duration in ms must be >= 10
      if( m_train.iRampDur < 0 ) m_train.iRampDur = 0;                  //    ramp duration in ms must be >= 0
      if( m_train.iIntv < m_train.iPulseDur + 2 * m_train.iRampDur )    //    pulse intv must be > pulsD + 2*rampD
         m_train.iIntv = 10 + m_train.iPulseDur + 2 * m_train.iRampDur;
   }
   else if( m_iType == PERT_ISNOISE || m_iType == PERT_ISGAUSS )        // for the noise perts:
   {
      if( m_noise.iUpdIntv < 1 ) m_noise.iUpdIntv = 1;                  //    noise update intv must be >= 1ms
      if( m_noise.fMean > 1.0f ) m_noise.fMean = 1.0f;                  //    restrict mean level to [-1.0 ... 1.0]
      else if( m_noise.fMean < -1.0f ) m_noise.fMean = -1.0f;
      if( m_noise.iSeed < -9999999 ) m_noise.iSeed = -9999999;          //    seed in [-9999999..10000000]
      else if( m_noise.iSeed > 10000000 ) m_noise.iSeed = 10000000;
   }
}
