//=====================================================================================================================
//
// cxcontrun.cpp : Implementation of class CCxContRun, encapsulating a Maestro "stimulus run", and class CCxStimulus,
//                 which encapsulates a single "stimulus channel" within a run.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This class encapsulates the definition of a Maestro "stimulus run", the experimental protocol object for ContMode 
// (analogous to the "trial" object in TrialMode). CCxContRun provides a single entity for storing the complete defn of
// the run. It also provides a set of operations for accessing and/or modifying this definition. Each run is defined by
// a few general parameters (duty period in ms, etc.) and a set of "stimulus channels". Each channel describes a motion
// trajectory for a particular stimulus type. The following table details the types of stimulus channels currently
// supported, and the motion modes available for each channel type:
//
//    TYPE                 MOTION MODE             TRAJECTORY DESCRIPTION
//    ------------------   ---------------------   --------------------------------------------------------------------
//    CHAIR                Sinusoidal velocity     v(t) = A * sin( (t-t0)*2*PI/T + P ), for t-t0=[0..N*T]; where t0 =
//                                                    start time in ms, A = amp in deg/s, T = period in ms, P = phase
//                                                    in radians, and N = #cycles presented.
//                         Trapezoidal vel pulse   v(t) = (t-t0)*Vo/RD           for t-t0 = [0..RD),
//                                                      = Vo                     for t-t0 = [RD..RD+PD),
//                                                      = Vo - (t-t0-t1)*Vo/RD   for t-t0 = [RD+PD..2*RD+PD];
//                                                    where t0 = start time in ms, Vo = velocity in deg/s, RD = ramp
//                                                    duration in ms, PD = pulse duration in ms, and t1 = RD+PD.
//                                                    Another parameter, BLANKENA, allows user to optionally blank
//                                                    any active targets (eg, FIBER1/2, REDLED1/2) during the pulse.
//
//    ****** DEPRECATED -- The optic bench targets are no longer supported as of Maestro 3.0 *************************
//    Fiber1, Fiber2       Sinusoidal velocity     Similar to CHAIR, but remember there's H & V motion in this case.
//                                                 The user specifies a parameter, R, indicating the CCW angle between
//                                                 the x-axis and a line passing through the origin.  The sine motion
//                                                 occurs along this line. x(t) = v(t)*cos(R), y(t) = v(t)*sin(R).
//                         Trapezoidal vel pulse   Similar to CHAIR, except BLANKENA replaced by rot angle R.
//    
//    PSGM                 *** The PSGM is NOT a visual stimulus at all.  Rather, it delivers an electrical stimulus
//                         pulse train to the brain via a microelectrode.  With the prerequisite external hardware,
//                         CNTRLX can control the delivery of one, two, or a train of pulses.  Here we list the five
//                         kinds of PSGM stimulu supported, and the associated parameters:
//
//                         Single pulse            t0 = start time within duty cycle, EXTTRIG? = is stimulus started by
//                                                 ext trigger? (else software start), A1 = pulse amplitude in V, and
//                                                 W1 = pulse width in us.
//                         Dual pulse              t0, EXTTRIG?, A1 & A2, W1 & W2, and IPI = interpulse interval in ms.
//                         Biphasic pulse          t0, EXTTRIG?, A1 & A2, W1 & W2 (IPI=0).
//                         Pulse train             t0, EXTTRIG?, A1 & W1 (same for all pulses), IPI, ITI = intertrain
//                                                 intv in ms, NP = #pulses per trains, NT = #trains per stimulus
//                         Biphasic pulse trains   Same as for "Pulse train", plus A2 & W2 to characterize second phase 
//                                                 of the biphasic pulse.
//
//    ****** DEPRECATED -- XYScope not supported a/o Maestro 4.0. XYseq deprecated a/o V5.0. *************************
//    XYSeq                *** XYSeq is a very specialized visual stimulus that generates random motion over a defined
//                         set of XY scope targets.  It was originally introduced to efficiently characterize the
//                         receptive fields of MT cells in anaesthesized animals.  The user defined the set of XY tgts
//                         to form an NxN "checkerboard" grid of FASTCENTER or FCDOTLIFE patches covering the entire XY
//                         screen.  Then a series of XYseq runs would randomly vary the speed or direction of the dots
//                         in each patch (the patches themselves do NOT move) -- in order to evaluate the speed tuning
//                         and directional sensitivity of the cell at different locations in the receptive field.
//
//                         Only one XYseq channel can be enabled for each run, and the set of XY targets is included in
//                         the run definition, along with their center locations.  The stimulus is a "sequence" in the
//                         sense that it is made up of N segments of a specified duration; the direction or velocity of
//                         one or all XY target dot patterns are chosen randomly at the start of each segment.  Four
//                         motion modes are possible:
//
//                         Sparse direction        One XY target dot pattern, randomly chosen, moves per segment with a
//                                                 specified constant velocity; the direction of the pattern's motion
//                                                 is randomly chosen.
//                         Dense direction         All dot patterns move each segment with constant velocity; direction
//                                                 of each pattern's motion is randomly and independently chosen.
//                         Sparse velocity         One XY target dot pattern, randomly chosen, moves per segment in a
//                                                 specified direction; velocity of dot pattern is randomly chosen.
//                         Dense velocity          All dot patterns move each segment in specified direction; velocity
//                                                 of each pattern's motion is randomly and independently chosen.
//
// One of the most important characteristics of the CHAIR stimulus channel type is that you can enable more than
// one channel of each type, and all enabled channels are added to form a composite stimulus waveform.  Thus, the user
// can define a rich variety of relatively complex stimuli.  Of course, this feature does not apply to the PSGM or
// XYseq channel types; for these, only one channel may be enabled (although many can be defined) in any given run.
//
// Because a stimulus channel may be defined by as many as 15 parameters, we encapsulate it by another class,
// CCxStimulus, also defined here.  This design makes it relatively easy to work with channel objects as single
// entities -- simplifying CCxContRun tasks such as "insert channel", "cut/copy/paste channel", etc.  NOTE, however,
// that stimulus channels are not Maestro data objects: they are not found as child nodes under a run object in the
// Maestro object tree; rather, they are part of the "data" in a CCxContRun. Also, CCxContRun exercises complete control
// over its stimulus channels. For example, to insert a channel into the run's channel list, callers must invoke a 
// CCxContRun method which, in turn, constructs and initializes a new CCxStimulus object and inserts that object into 
// its channel list. To modify the parameters of an existing channel, callers must again use CCxContRun methods; the run
// object does not expose non-const references to its stimulus channel objects. The purpose of these restrictions is to 
// emphasize the fact that stimulus channels are meaningless outside a containing run object, and to prevent 
// modifications of a channel (such as deleting it!!) that make no sense in the context of the run in which it 
// participates. [Still, it is possible to copy & paste a stimulus channel from one run object to another; see the 
// CopyStimulus() and PasteStimulus() methods.]
//
// ==> The Big Picture:  Storage of Maestro data objects.
// The user creates experimental protocols within a Maestro "experiment document" (CCxDoc) by defining a variety of
// "data objects" and establishing relationships among those objects.  For instance, each CNTRLX "trial" defines the
// trajectories of one or more "targets", which are defined separately.  The trial object also refers to a "channel
// set" object, which contains the list of analog channels that should be sampled during that trial.  Trials, targets,
// and channel sets are examples of "abstract" data classes defined in Maestro.
//
// Maestro data objects are stored in the Maestro object trees, encapsulated by CCxTreeMap.  This "tree map" collection
// stores all the data objects in several different hierarchical trees (the "target tree", "trial tree", and so on).
// We chose this somewhat complex storage scheme in order to organize the different data objects in a logical manner,
// and to provide the potential for storing a large # of objects in a single document yet be able to access any
// individual object rapidly via a unique key value (hence the "map" in "tree map").  CCxTreeMap can store up to
// 65535 different objects, more than enough for our purposes.
//
// CCxTreeMap is derived from the generic CTreeMap class, which handles the low-level implementation details of the
// tree map (see TREEMAP.CPP).  CTreeMap itself handles one base data class, CTreeObj, which merely stores the object's
// name and abstract data type and serves as the starting point for building more complex data classes.  CCxTreeMap
// tailors the behavior of CTreeMap so it can handle all data types present in Maestro. Each Maestro data class must
// satisfy certain constraints in order to build the CNTRLX object trees on top of the CTreeMap/CTreeObj framework; see
// the CTreeMap/CTreeObj implementation file for an explanation of these constraints. CCxContRun has been designed
// with these constraints in mind.
//
// There is a division of responsibilities among CCxDoc, CCxTreeMap, and the various CTreeObj-derived classes that
// represent the real Maestro data objects. First, the Maestro data object classes provide methods for accessing,
// modifying, and validating the actual data which define how the object behaves in a Maestro experiment. CCxTreeMap is
// the "intelligent" storage medium for these objects (leaf nodes in the tree-map) and "collections" of related
// objects. It must be "aware" of all the different types of Maestro data objects so that it can construct any given
// object by calling the appropriate constructor. Furthermore, it controls the naming of the objects, allowing only
// characters from a valid character set (it uses the default char set provided by CTreeMap) and requiring that no two
// sibling objects have the same name.  Finally, of course, it encodes the tree connections among the objects and
// provides methods for adding objects to the trees, removing objects, etc.  However, it does NOT impose any
// restrictions on how objects are added to the tree-map; that is the responsibility of CCxDoc, in coordination with
// its various views. As mentioned above, CCxDoc uses CCxTreeMap to store a number of different "Maestro object trees";
// CCxDoc methods implement the logic for constructing and restricting the exact composition of these object trees (see
// CCxDoc for details).
//
// CCxContRun represents the "data class" which handles all continuous-mode stimulus runs in Maestro. It stores a single
// abstract data type, identified by the defined constant CX_CONTRUN.
//
// ==> Using CCxContRun.
// As explained above, CCxContRun is designed for use with the Maestro object tree container CCxTreeMap and the
// underlying CTreeMap/CTreeObj framework.  Thus, the default constructor, destructor, and the Copy() and Initialize()
// methods are all protected.  The idea is that only CCxTreeMap can construct, copy, and destroy CCxContRun objects.
// In addition, CCxContRun must override CTreeObj::GetDependencies() because it may "depend" on any XY scope target
// objects currently included in the XYseq target list.  The keys of such "object dependencies" are stored within the
// run object itself, and CTreeMap must "lock" these objects to prevent the user from deleting them -- which would make
// the run's definition invalid.  See CTreeMap for more information on this issue.
//
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//       !!! Whenever views make a change to a run object, they must inform CCxDoc via   !!!
//       !!! CCxDoc::UpdateObjDep().  Otherwise, the dependency locking scheme will fail !!!
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// In the Maestro design scheme, a view can obtain a pointer to a particular CCxContRun object by querying CCxDoc, which
// includes a CCxTreeMap container for managing the Maestro object trees. The view can then edit the run definition by
// invoking various public methods.
//
// It is important to note that the run object never provides DIRECT access to its stimulus channels. All changes must
// be made by invoking CCxContRun methods. For example, CopyStimulus() does not provide a non-const reference to the 
// desired channel, but a copy of it; thus, a view cannot modify any stimulus channel object directly by invoking 
// methods thru that pointer! Similarly, PasteStimulus() does not insert the provided channel into the run's stimulus 
// list, rather, it inserts a copy.
//
// DEVNOTES:
// 1) Maestro data objects developed prior to CCxContRun (such as CCxTrial, CCxTarget, ...) were designed under the
// assumption that they could not access the containing CCxDoc. I have relaxed that restriction on CCxContRun so that
// it could perform certain tasks. CCntrlxApp::GetDoc() exposes the ptr to the current (and only) CCxDoc!
//
//
// REVISION HISTORY:
// 10may2002-- Began development, following the model of CCxTrial/CCxSegment.
// 15may2002-- First version of CCxStimulus done.
// 20may2002-- Second version of CCxStimulus done (access both common & motion parameters by index).
// 21may2002-- First version of CCxContRun done.
// 06jun2002-- Debugging cont'd.  Modified CCxStimulus::SetParameter() and CCxContRun::SetStimParameter() to return
//             TRUE if the change may have had a side effect -- ie, it caused a change in another parameter, or it
//             turned "off" another channel(s).
// 18sep2002-- Major design flaw in CCxStimulus:  Storing parameters in a single float array regardless of type
//             creates the following problem.  When the user changes the stimulus type from "Chair" to PSGM, many of
//             the motion parameters are altered to remain consistent with the new type.  If the user decides to switch
//             back to "Chair", at least some of the motion parameters will now have different values -- which is VERY
//             annoying.  To fix, we must maintain separate storage for each stimulus type.  Still, we want to keep the
//             mapping of parameter index to actual parameter, since this makes it easier to edit a CCxContRun...
// 23dec2002-- Adding Import() to support importing CCxContRun defn from a cntrlxUNIX-style "run definition" file.
// 13mar2003-- Modified GetRunDefinition() IAW change to CXDRIVER-compatible CONTRUN structure.  XY scope tgt defns in
//             CONTRUN struct are represented by the more generic CXTARGET structure instead of XYPARMS.
// 15mar2005-- Added CopyRemoteObj() override.
// 14mar2006-- Removed support for STIM_ISOKN channel.  The OKNDRUM target platform is no longer supported as of
//             Maestro v1.5.0.  If an existing document contains a CCxContRun that includes one or more OKN stimulus
//             channels, each such channel gets remapped to the CHAIR stimulus channel.  A warning message is printed
//             in Maestro's message panel as well.
// 25apr2007-- Modified IAW recent changes to PSGM spec -- particularly introduction of SGM_BIPHASICTRAIN op mode. Also 
//             renamed some SGM-related constants in cxobj_ifc.h. 
// 21sep2011-- Removed support for the STIM_ISFIBER1 and STIM_ISFIBER2 channels. The optic bench targets are no longer
// supported as of Maestro 3.0. Now there are only 3 stimulus types; STIM_ISCHAIR, STIM_ISPSGM (not really supported),
// and STIM_ISXYSEQ. During deserialization of a pre-3.0 document, all STIM_ISFIBER1 and STIM_ISFIBER2 channels are
// converted to STIM_ISCHAIR channels, and a message is posted to notify the user. The stimulus channel type value for
// STIM_ISXYSEQ and STIM_ISPSGM are also adjusted to their new values. Extensive changes have been made to this module
// to remove support for the Fiber1/2 channels.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
// 30sep2024-- Made changes to reflect the fact that the XYseq stimulus -- which uses the now-deprecated XYScope
// platform -- is deprecated a/o Maestro 5.0. CCxContRun can still handle deserialization of stimulus runs using XYseq,
// but an exception is thrown if there's an attempt to save a run that uses XYseq.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CNTRLX application-wide defines; resource defines

#include "cxdoc.h"                           // CCxDoc -- the CNTRLX document object
#include "cxtarget.h"                        // CCxTarget -- the CNTRLX target object
#include "cxcontrun.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//=====================================================================================================================
// class CCxStimulus
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxStimulus, CObject, 4 | VERSIONABLE_SCHEMA )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================

const int CCxStimulus::NPARAMS[STIM_NTYPES][STIM_NMAXMODES] =     // # of motion parameters varies w/type & mode
{
   { 4, 4, 0, 0, 0 },                                             //    STIM_ISCHAIR
   { 3, 6, 5, 7, 9 },                                             //    STIM_ISPSGM
   { 7, 7, 8, 8, 0 }                                              //    STIM_ISXYSEQ
};

LPCTSTR CCxStimulus::STDMODESTRINGS[] = { "Sine", "Pulse" };
LPCTSTR CCxStimulus::PSGMMODESTRINGS[] = { "Single", "2Pulse", "Biphasic", "Train", "Biph Tr" };
LPCTSTR CCxStimulus::COMMONLBLSTRINGS[] = { "On/off", "Marker", "Type", "Motion", "t0 (ms)" };



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== GetStimulusInfo, SetStimulusInfo ================================================================================
//
//    Get/set the stimulus definition.
//
//    CXDRIVER uses a formatted data structure, STIMCHAN, to hold the relevant parameters IAW stimulus type and motion
//    mode. CCxStimulus uses a very similar storage scheme, except that it maintains separate copies of each possible
//    motion parameter set so that the object's type and motion mode can be freely changed without having to revalidate
//    the relevant parameter set.
//
//    30sep2024-- A/o Maestro 5.0, XYseq stimuli are deprecated.
//    ARGS:       stim  -- [in/out] the stimulus channel definition struct compatible with CXDRIVER.
//
//    RETURNS:    NONE.
//
VOID CCxStimulus::GetStimulusInfo( STIMCHAN& stim ) const
{
   stim.bOn = m_bOn;
   stim.iMarker = m_iMarker;
   stim.iType = m_iType;
   stim.tStart = m_tStart;

   if( m_iType == STIM_ISPSGM ) stim.sgm = m_sgm;
   else
   {
      stim.iStdMode = m_iStdMode;
      if( m_iStdMode == MODE_ISSINE ) stim.sine = m_sine;
      else stim.pulse = m_pulse;
   }
}

VOID CCxStimulus::SetStimulusInfo( const STIMCHAN& stim )
{
   m_bOn = stim.bOn;
   m_iMarker = stim.iMarker;
   m_iType = stim.iType;
   m_tStart = stim.tStart;

   ASSERT(stim.iType != STIM_ISXYSEQ);
   if( stim.iType == STIM_ISPSGM ) m_sgm = stim.sgm;
   else
   {
      m_iStdMode = stim.iStdMode;
      if( stim.iStdMode == MODE_ISSINE ) m_sine = stim.sine;
      else m_pulse = stim.pulse;
   }

   Validate();                                                       // autocorrect the new defn
}


//=== Copy ============================================================================================================
//
//    Copy the contents of the specified "source" stimulus channel to THIS stimulus channel object.  The previous
//    contents of this object are lost.
//
//    REMEMBER:  Unlike the STIMCHAN struct, CCxStimulus includes storage for the motion parameters of every supported
//    stimulus types.  We must copy ALL motion parameter sets, not just those applicable to the current stimulus type.
//
//    ARGS:       src   -- [in] the stimulus channel object to be copied.
//
//    RETURNS:    NONE.
//
VOID CCxStimulus::Copy( const CCxStimulus& src )
{
   ASSERT_VALID( &src );
   ASSERT(src.m_iType != STIM_ISXYSEQ);
   m_bOn = src.m_bOn;
   m_iMarker = src.m_iMarker;
   m_iType = src.m_iType;
   m_iStdMode = src.m_iStdMode;
   m_tStart = src.m_tStart;
   m_sine = src.m_sine;
   m_pulse = src.m_pulse;
   // m_xyseq = src.m_xyseq;
   m_sgm = src.m_sgm;
}


/** [base override]
 Handles reading/writing the stimulus channel info from/to a disk file via a serialization archive.  Note that,
 after deserialization, any invalid parameters are auto-corrected.

 Version control:
   1: Base version.
   2: As of Maestro v1.5.0, the OKNDRUM target platform is no longer supported. Thus, the stimulus channel type
      STIM_ISOKN=1 no longer exists. If we encounter such a channel type in an earlier version, we map it to 
      STIM_ISCHAIR instead and post a message in Maestro's message panel. Channel type IDs for all channels except 
      STIM_ISCHAIR are decremented by 1 to bring them in line with this version.
   2: (25apr2007) Added SGM op mode SGM_BIPHASICTRAIN. No need to change schema version.
   3: As of Maestro v3.0, the optic bench targets are no longer supported. Thus, the stimulus channel types 
      STIM_ISFIBER1=1, STIM_ISFIBER2=2 are no longer allowed. If we encounter such a channel type in an earlier 
      version, we map it to STIM_ISCHAIR instead and post a message in Maestro's message panel. Channel type IDs 
      corresponding to the PSGM and XYSeq stimuli are decremented by 2 to bring them in line with this version.
   4: As of Maestro v5.0, XYseq stimuli may no longer be saved. The XYScope platform has not been supported since
      V4.0, and XYScope targets and XYseq are no longer allowed in experiment docs a/o V5.0. Still supports 
      deserializing XYseq stimuli in order to handle old documents containing them. AFTER deserialization, CCxDoc
      removes all stimlus runs and trial that depend on XYScope targets.

 @param ar The serialization archive.
 @throws The archive may throw CMemoryException, CArchiveException, or CFileException.
*/
void CCxStimulus::Serialize(CArchive& ar)
{
   UINT nSchema = ar.GetObjectSchema();                        // retrieve schema#
   CObject::Serialize(ar);

   if( ar.IsStoring() )                                        // STORE TO ARCHIVE...
   {
      if(m_iType == STIM_ISXYSEQ)
         ::AfxThrowArchiveException(CArchiveException::genericException);

      ar << int(m_bOn) << m_iMarker << m_iType << m_iStdMode << m_tStart;
      if( m_iType == STIM_ISPSGM )
      {
         ar << m_sgm.iOpMode << int(m_sgm.bExtTrig) << m_sgm.iAmp1 << m_sgm.iAmp2 << m_sgm.iPW1 << m_sgm.iPW2;
         ar << m_sgm.iPulseIntv << m_sgm.iTrainIntv << m_sgm.nPulses << m_sgm.nTrains;
      }
      else if( m_iStdMode == MODE_ISSINE )
      {
         ar << m_sine.iPeriod << m_sine.nCycles;
         ar << m_sine.fAmp << m_sine.fPhase << m_sine.fDirec;
      }
      else if( m_iStdMode == MODE_ISPULSE )
      {
         ar << int(m_pulse.bBlank) << m_pulse.iPulseDur << m_pulse.iRampDur;
         ar << m_pulse.fAmp << m_pulse.fDirec;
      }
   }
   else                                                        // READ FROM ARCHIVE...
   {
      if( nSchema < 1 || nSchema > 4 )                         // unsupported version
         ::AfxThrowArchiveException( CArchiveException::badSchema );

      SetDefaults();

      int i = 0;
      ar >> i >> m_iMarker >> m_iType >> m_iStdMode >> m_tStart;
      m_bOn = BOOL(i != 0);

      // schema 1 -> 2: STIM_ISOKN no longer supported. Need to adjust channel type ID.
      if(nSchema == 1) 
      {
         if( m_iType == 1 )
            ((CCntrlxApp*)AfxGetApp())->LogMessage("OKN stimulus channel no longer supported; changed to CHAIR!");
         if( m_iType > STIM_ISCHAIR )
            --m_iType;
         nSchema = 2;
      }

      // schema 2 -> 3: STIM_ISFIBER1 and _ISFIBER2 no longer supported. Again adjust channel typeID.
      if(nSchema == 2)
      {
         if(m_iType == 1 || m_iType == 2)
         {
            m_iType = STIM_ISCHAIR;
            ((CCntrlxApp*)AfxGetApp())->LogMessage("Fiber1/2 stimulus channel no longer supported; changed to CHAIR!");
         }
         else if(m_iType > 2) m_iType -= 2;
         nSchema = 3;
      }

      if(m_iType == STIM_ISXYSEQ)
      {
         // XYseq stimuli cannot appear in schema version 4 documents!
         if(nSchema == 4)
            ::AfxThrowArchiveException(CArchiveException::badSchema);
         ar >> m_xyseq.iOpMode >> m_xyseq.iRefresh >> m_xyseq.nSegs >> m_xyseq.iSegDur >> m_xyseq.iSeed;
         ar >> m_xyseq.nChoices >> m_xyseq.fAngle >> m_xyseq.fVel >> m_xyseq.fOffsetV;
      }
      else if( m_iType == STIM_ISPSGM )
      {
         ar >> m_sgm.iOpMode >> i >> m_sgm.iAmp1 >> m_sgm.iAmp2 >> m_sgm.iPW1 >> m_sgm.iPW2;
         ar >> m_sgm.iPulseIntv >> m_sgm.iTrainIntv >> m_sgm.nPulses >> m_sgm.nTrains;
         m_sgm.bExtTrig = BOOL(i != 0);
      }
      else if( m_iStdMode == MODE_ISSINE )
      {
         ar >> m_sine.iPeriod >> m_sine.nCycles;
         ar >> m_sine.fAmp >> m_sine.fPhase >> m_sine.fDirec;
      }
      else if( m_iStdMode == MODE_ISPULSE )
      {
         ar >> i >> m_pulse.iPulseDur >> m_pulse.iRampDur;
         ar >> m_pulse.fAmp >> m_pulse.fDirec;
         m_pulse.bBlank = BOOL(i != 0);
      }
      Validate();                                              // validate the stim chan defn just read in!
   }

   ASSERT_VALID( this );                                       // check validity AFTER serializing
}



//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================

//=== Get/SetParameter... =============================================================================================
//
//    This group of methods provides generalized access to the stimulus channel's entire parameter list, including both
//    "common" and "motion" parameters.  An individual parameter is identified by a zero-based index.  The methods
//    provide enough information so that a view class can display and edit any parameter without requiring hard-coded
//    knowledge of the parameter's identity, which changes IAW stimulus type and motion mode.
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
//    change in another parameter.  The primary example of such a side effect involves changing stimulus type or
//    motion mode, which usually changes the makeup of the channel's unique motion parameter set.  Whenever a
//    parameter change could possibly have such a side effect, the SetParameter() routine returns TRUE.
//
//    If the parameter index does not specify a valid parameter given the current stimulus type and motion mode:
//    GetParameter() and GetParameterAsInt() return "0", GetParameterLabel() retrieves an empty string,
//    GetParameterFormat() retrieves a multiple-choice parameter with no choices, IsParameterMultiChoice() returns
//    FALSE, and SetParameter() has no effect.
//
//    The table below maps the zero-based "parameter index" to the parameter's identity.  The first 5 indices refer to
//    the parameters that are common for all stimulus types and motion mode.  Indices >=5 refer to motion parameters,
//    the identities of which var with type and motion mode.
// 
//    *** (30sep2024) XYseq stimuli DEPRECATED a/o Maestro 5.0. ***
//
//    index    PSGM type                  Sines mode              Pulse mode
//    ---------------------------------------------------------------------------------------------------
//    0        m_bOn                      m_bOn                   m_bOn
//    1        m_iMarker                  m_iMarker               m_iMarker
//    2        m_iType                    m_iType                 m_iType
//    3        m_sgm.iOpMode              m_iStdMode              m_iStdMode
//    4        m_tStart                   m_tStart                m_tStart
//
//    5        m_sgm.bExtTrig             m_sine.fAmp             m_pulse.fAmp
//    6        m_sgm.iAmp1                m_sine.iPeriod          m_pulse.iPulseDur
//    7        m_sgm.iPW1                 m_sine.fPhase           m_pulse.iRampDur
//    8        m_sgm.iAmp2, or            m_sine.nCycles          m_pulse.bBlank 
//             m_sgm.iPulseIntv, or 
//             NOT USED
//
//    9        m_sgm.iPW2, or             NOT USED                NOT USED
//             m_sgm.nPulses, or 
//             NOT USED
//
//    10       m_sgm.iPulseIntv, or       NOT USED                NOT USED
//             m_sgm.iTrainIntv, or
//             NOT USED
//
//    11       m_sgm.nTrains, or          NOT USED                NOT USED
//             m_sgm.nPulses, or
//             NOT USED
//
//    12       m_sgm.iTrainIntv, or       NOT USED                NOT USED
//             NOT USED
//
//    13       m_sgm.nTrains, or          NOT USED                NOT USED
//             NOT USED
//
//    14       Reserved for future use.
//
//    ARGS:       i           -- [in] the index of desired parameter in the stimulus channel's parameter list.
//                str         -- [out] string representation of parameter's value, or a descriptive name for parameter.
//                bIsChoice   -- [out] TRUE if this is a multi-choice parameter; otherwise it's numeric.
//                choices     -- [out] set to available choices for a multi-choice parameter; else empty.
//                fmt         -- [out] numeric format constraints for a numeric parameter.
//                iVal, dVal  -- [in] new value for parameter.
//
//    RETURNS:    various
//
double CCxStimulus::GetParameter( int i ) const
{
   if( !IsValidParameter( i ) ) return( 0.0 );                       // merely return 0 if param index invalid

   BOOL bIsTrain = BOOL(m_iType == STIM_ISPSGM && m_sgm.iOpMode == SGM_TRAIN);
   BOOL bIsSine = BOOL(m_iStdMode == MODE_ISSINE);

   double d = 0.0;                                                   // return each parameter formatted as a double
   switch( i )
   {
      case 0 : d = m_bOn ? 1.0 : 0.0;  break;
      case 1 : d = double(m_iMarker);  break;
      case 2 : d = double(m_iType);    break;
      case 4 : d = double(m_tStart);   break;

      case 3 :                                                       // motion mode is stored in several diff places...
         if( m_iType == STIM_ISPSGM )   d = double(m_sgm.iOpMode);
         else                           d = double(m_iStdMode);
         break;

      case 5 :
         if( m_iType == STIM_ISPSGM )   d = m_sgm.bExtTrig ? 1.0 : 0.0;
         else if( bIsSine )             d = double(m_sine.fAmp);
         else                           d = double(m_pulse.fAmp);
         break;

      case 6 :
         if( m_iType == STIM_ISPSGM )   d = double(m_sgm.iAmp1);
         else if( bIsSine )             d = double(m_sine.iPeriod);
         else                           d = double(m_pulse.iPulseDur);
         break;

      case 7 :
         if( m_iType == STIM_ISPSGM )   d = double(m_sgm.iPW1);
         else if( bIsSine )             d = double(m_sine.fPhase);
         else                           d = double(m_pulse.iRampDur);
         break;

      case 8 :
         if( m_iType == STIM_ISPSGM )   d = double( bIsTrain ? m_sgm.iPulseIntv : m_sgm.iAmp2 );
         else if( bIsSine )             d = double(m_sine.nCycles);
         else                           d = m_pulse.bBlank ? 1.0 : 0.0;
         break;

      case 9 :
         if( m_iType == STIM_ISPSGM )   d = double( bIsTrain ? m_sgm.nPulses : m_sgm.iPW2 );
         break;

      case 10 :
         d = double(bIsTrain ? m_sgm.iTrainIntv : m_sgm.iPulseIntv);
         break;

      case 11 :
         d = double(bIsTrain ? m_sgm.nTrains : m_sgm.nPulses);
         break;

      case 12 : 
         d = double(m_sgm.iTrainIntv);
         break;

      case 13 :
         d = double(m_sgm.nTrains);
         break;
   }

   return( d );
}

VOID CCxStimulus::GetParameter( int i, CString& str ) const
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

int CCxStimulus::GetParameterAsInt( int i ) const
{
   double d = GetParameter( i );
   return( (d < 0.0) ? int(d - 0.5) : int(d + 0.5) );
}

VOID CCxStimulus::GetParameterLabel( int i, CString& str ) const     // all parameter labels <= 12 characters!
{
   str.Empty();
   if( !IsValidParameter( i ) ) return;                              // empty title string for an invalid parameter!

   if( i < NumberOfCommonParameters() )
      GetCommonParameterLabel( i, str );
   if( m_iType == STIM_ISPSGM )
   {
      BOOL bIsTrain = BOOL( m_sgm.iOpMode == SGM_TRAIN );
      switch( i )
      {
         case 5 : str = _T("extTrig?"); break;
         case 6 : str = _T("amp1(mV)"); break;
         case 7 : str = _T("pulseW1(us)"); break;
         case 8 : str = bIsTrain ? _T("IPI (ms)") : _T("amp2(mV)"); break;
         case 9 : str = bIsTrain ? _T("#pulses")  : _T("pulseW2(us)"); break;
         case 10: str = bIsTrain ? _T("ITI (ms)") : _T("IPI (ms)"); break;
         case 11: str = bIsTrain ? _T("#trains")  : _T("#pulses"); break;
         case 12: str = _T("ITI (ms)"); break;
         case 13: str = _T("#trains"); break;
         default: break;
      }
   }
   else
   {
      BOOL bIsSine = BOOL( m_iStdMode == MODE_ISSINE );
      switch( i )
      {
         case 5 : str = _T("amp (deg/s)"); break;
         case 6 : str = bIsSine ? _T("period(ms)") : _T("pulseDur(ms)"); break;
         case 7 : str = bIsSine ? _T("phase(deg)") : _T("rampDur(ms)"); break;
         case 8 : str = bIsSine ? _T("#cycles")    : _T("blank?"); break;
         default: break;
      }
   }
}

VOID CCxStimulus::GetParameterFormat( int i, BOOL& bIsChoice, CStringArray& choices, NUMEDITFMT& fmt ) const
{
   int j;

   choices.RemoveAll();
   bIsChoice = TRUE;                                                       // if parameter index invalid, format param
   if( !IsValidParameter( i ) ) return;                                    // as multi-choice w/ an empty choice set!!

                                                                           // the multi-choice parameters:
   if( i == 0 )                                                            // 0) stimulus is "off"(0) or "ON"(1)
   {
      choices.Add( _T("off") );
      choices.Add( _T("ON") );
   }
   else if( i == 1 )                                                       // 1) marker pulse is OFF (0) or DOUT<N>
   {
      char label[10];
      choices.Add( _T("off") );
      for( j = 1; j <= STIM_NLASTMARKER; j++ )
      {
         ::sprintf_s( label, "DOUT%d", j );
         choices.Add( label );
      }
   }
   else if( i == 2 )                                                       // 2) stimulus type
   {
      choices.Add("Chair");
      choices.Add("PSGM");
   }
   else if( i == 3 )                                                       // 3) motion mode -- choices depend on
   {                                                                       // stimulus type
      if( m_iType == STIM_ISPSGM )
         for( j=0; j<STIM_NPSGMMODES; j++ ) choices.Add( PSGMMODESTRINGS[j] );
      else
         for( j=0; j<STIM_NSTDMODES; j++ ) choices.Add( STDMODESTRINGS[j] );
   }
   else if( (m_iType==STIM_ISCHAIR && m_iStdMode==MODE_ISPULSE && i==8)    // motion params "Blank?" and "extTrig?" are
            || (m_iType==STIM_ISPSGM && i==5) )                            // "NO/YES" choices
   {
      choices.Add( _T("NO") );
      choices.Add( _T("YES") );
   }
   else                                                                    // all other params are numeric...
   {
      bIsChoice = FALSE;                                                   // these default attributes apply to all
      fmt.flags = NES_INTONLY | NES_NONNEG;                                // numeric parameters other than those
      fmt.nPre = 1;                                                        // handled below...
      fmt.nLen = 6;
      if( m_iType == STIM_ISPSGM )
      {
         BOOL bIsTrain = BOOL(m_sgm.iOpMode == SGM_TRAIN);
         switch( i )
         {
            case 6 : fmt.flags = NES_INTONLY; fmt.nLen = 6; break;
            case 7 : fmt.nLen = 4; break;
            case 8 : if(bIsTrain) fmt.nLen = 3;
                     else { fmt.flags = NES_INTONLY; fmt.nLen = 6; }
                     break;
            case 9 : fmt.nLen = bIsTrain ? 3 : 4; break;
            case 10: fmt.nLen = (bIsTrain) ? 4 : 3; break;
            case 11: fmt.nLen = 3; break;
            case 12: fmt.nLen = 4; break;
            case 13: fmt.nLen = 3; break;
         }
      }
      else
      {
         BOOL bIsSine = BOOL(m_iStdMode == MODE_ISSINE);
         switch( i )
         {
            case 5 : fmt.flags = 0; fmt.nPre = 2; fmt.nLen = 8; break;
            case 7 : if( bIsSine ) fmt.flags = 0;
                     else fmt.nLen = 3;
                     break;
         }
      }
   }
}

BOOL CCxStimulus::IsParameterMultiChoice( int i ) const
{
   if( !IsValidParameter( i ) ) return( FALSE );

   return( (i<=3) ||                                                       // first 4 common params are multi-choice
           (i==8 && m_iType==STIM_ISCHAIR && m_iStdMode==MODE_ISPULSE) ||  // "Blank?" param for Chair in Pulse mode
           (i==5 && m_iType==STIM_ISPSGM) );                               // "extTrig?" param for PSGM in all modes
}

BOOL CCxStimulus::SetParameter( int i, double dVal )
{
   ASSERT(m_iType != STIM_ISXYSEQ);                                     // XYseq stimulus DEPRECATED

   if( !IsValidParameter( i ) ) return( FALSE );                        // do nothing if parameter index invalid
   
   int iVal = int(dVal + 0.5);                                          // integer version is rounded value
   BOOL bSideEffect = FALSE;                                            // TRUE if param change may have side effect

   BOOL bIsTrain = BOOL(m_iType == STIM_ISPSGM && m_sgm.iOpMode == SGM_TRAIN);
   BOOL bIsSine = BOOL(m_iStdMode == MODE_ISSINE);

   switch( i )
   {
      case 0 : m_bOn = (iVal<=0 || iVal>1) ? FALSE : TRUE; break;
      case 1 : m_iMarker = iVal; break;
      case 2 : m_iType = iVal; bSideEffect = TRUE; break;               // stim type affects index<->param mapping
      case 4 : m_tStart = iVal; break;

      case 3 :                                                          // motion mode stored in several diff places...
         if( m_iType == STIM_ISPSGM )   m_sgm.iOpMode = iVal;
         else                           m_iStdMode = iVal;
         bSideEffect = TRUE;                                            // motion mode affects index<->param mapping
         break;

      case 5 :
         if( m_iType == STIM_ISPSGM )   m_sgm.bExtTrig = (iVal<=0 || iVal>1) ? FALSE : TRUE;
         else if( bIsSine )             m_sine.fAmp = float(dVal);
         else                           m_pulse.fAmp = float(dVal);
         break;

      case 6 :
         if( m_iType == STIM_ISPSGM )   m_sgm.iAmp1 = iVal;
         else if( bIsSine )             m_sine.iPeriod = iVal;
         else                           m_pulse.iPulseDur = iVal;
         break;

      case 7 :
         if(m_iType == STIM_ISPSGM)   
         {
            m_sgm.iPW1 = iVal; 
            bSideEffect = bIsTrain || (m_sgm.iOpMode == SGM_BIPHASICTRAIN) || (m_sgm.iOpMode == SGM_DUAL);
         }
         else if(bIsSine) m_sine.fPhase = float(dVal);
         else m_pulse.iRampDur = iVal;
         break;

      case 8 :
         if( m_iType == STIM_ISPSGM )
         {
            if( bIsTrain ) 
            {
               m_sgm.iPulseIntv = iVal;
               bSideEffect = TRUE;
            }
            else m_sgm.iAmp2 = iVal;
         }
         else if( bIsSine ) m_sine.nCycles = iVal;
         else m_pulse.bBlank = (iVal<=0 || iVal>1) ? FALSE : TRUE;
         break;

      case 9 :
         if( m_iType == STIM_ISPSGM )
         {
            if( bIsTrain ) m_sgm.nPulses = iVal;
            else m_sgm.iPW2 = iVal;
            bSideEffect = bIsTrain || (m_sgm.iOpMode == SGM_BIPHASICTRAIN);
         }
         break;

      case 10 :
         if( bIsTrain ) m_sgm.iTrainIntv = iVal;
         else { m_sgm.iPulseIntv = iVal; bSideEffect = TRUE; }
         break;

      case 11 :
         if(bIsTrain) m_sgm.nTrains = iVal;
         else { m_sgm.nPulses = iVal; bSideEffect = TRUE; }
         break;

      case 12 :                              
         m_sgm.iTrainIntv = iVal;
         break;

      case 13 :
         m_sgm.nTrains = iVal; 
         break;
      
      default:
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
//    Dump the stimulus channel definition in an easy-to-read form to the supplied dump context.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxStimulus::Dump( CDumpContext& dc ) const
{
   ASSERT_VALID( this );                                                   // validate the object
   CObject::Dump( dc );                                                    // dump base class stuff first

   CString str;
   for( int i = 0; i < NumberOfParameters(); i++ )
   {
      GetParameterLabel( i, str );
      dc << (str + _T(": "));
      GetParameter( i, str );
      dc << (str + _T("  "));
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
void CCxStimulus::AssertValid() const
{
   ASSERT( IsKindOf( RUNTIME_CLASS(CCxStimulus) ) );
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== SetDefaults =====================================================================================================
//
//    Assign default values to stimulus channel parameters.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxStimulus::SetDefaults()
{
   m_bOn = FALSE;                      // stimulus channel is OFF
   m_iMarker = 0;                      // no marker pulse specified
   m_iType = STIM_ISCHAIR;             // sinusoidal motion using CX_CHAIR tgt
   m_iStdMode = MODE_ISSINE;
   m_tStart = 0;                       // starts at beginning of run's duty cycle

   m_sine.iPeriod = 100;               // single-cycle sinusoidal velocity trajectory v(t) = A*sin(2*pi*t/T + P),
   m_sine.nCycles = 1;                 // where: A=10deg/s, T=100ms, P=0deg, and t is in ms
   m_sine.fAmp = 10.0f;
   m_sine.fPhase = 0.0f;
   m_sine.fDirec = 0.0f;

   m_pulse.bBlank = FALSE;             // trapezoidal pulse motion param set:  blanking OFF,
   m_pulse.iPulseDur = 300;            //    duration of pulse in ms (>= 10ms)
   m_pulse.iRampDur  = 50;             //    duration of rising-edge and falling-edge ramps (>= 10ms)
   m_pulse.fAmp = 10.0f;               //    velocity amplitude, in deg/sec: [-500.0 .. 500.0].
   m_pulse.fDirec = 0.0f;              //    direction of motion, CCW angle from x-axis [-180.0..180.0]

   // DEPRECATED: We maintain m_xyseq in order to deserialize older documents containing XYseq runs...
   m_xyseq.iOpMode = MODE_ISSPARSEDIR; // for XYseq channel type:  sparse-direction motion mode,
   m_xyseq.iRefresh = 4;               //    refresh period in ms,
   m_xyseq.nSegs = 100;                //    # of distinct segments of random motion
   m_xyseq.iSegDur = 64;               //    duration of each segment, in ms (must be multiple of refresh period)
   m_xyseq.iSeed = 1;                  //    seed for generating random directions or velocities
   m_xyseq.nChoices = 8;               //    # of different directions (or velocities) randomized
   m_xyseq.fAngle = 0.0f;              //    offset angle (for direction modes) or direction of motion (for vel modes)
   m_xyseq.fVel = 128.0f;              //    velocity of motion (for dir modes) or max velocity (for vel modes)
   m_xyseq.fOffsetV = 0.0f;            //    offset velocity (for vel modes only)

   m_sgm.iOpMode = SGM_SINGLE;         // for PSGM stim channel type: single-pulse motion mode,
   m_sgm.bExtTrig = FALSE;             //    if TRUE, use external trig to initiate pulse seq; else, s/w start.
   m_sgm.iAmp1 = m_sgm.iAmp2 = 10000;  //    pulse amplitude.  range [-10240..10160mV], res = 80mV.
   m_sgm.iPW1 = m_sgm.iPW2 = 50;       //    pulse width.  range [50..2500us], res = 10us.
   m_sgm.iPulseIntv = 1;               //    interpulse interval.  range [1..250ms].
   m_sgm.iTrainIntv = 10;              //    intertrain interval.  range [10..2500ms], res = 10ms.
   m_sgm.nPulses = 1;                  //    #pulses per train.  range [1..250]. 
   m_sgm.nTrains = 1;                  //    #trains per stimulus.  range [1..250].
}


//=== Validate ========================================================================================================
//
//    Validate the current stimulus channel definition.
//
//    30sep2024: XYseq stimuli (STIM_ISXYSEQ) are no longer validated. XYseq-containing stimulus runs may be read in
//    during deserialization of pre-V5.0 docs, but they are ultimately deleted by CCxDoc after deserialization.
// 
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxStimulus::Validate()
{
   if( m_iMarker < 0 || m_iMarker > STIM_NLASTMARKER ) m_iMarker = 0;   // marker pulse DO<N>, N=[0..max]
   if( m_iType < 0 || m_iType >= STIM_NTYPES ) m_iType = 0;             // stimulus type, T=[0..#types-1]

   if( m_iStdMode < 0 || m_iStdMode >= STIM_NSTDMODES )                 // motion mode: limited # of choices, depending
       m_iStdMode = 0;                                                  // on stimulus type...
   if( m_sgm.iOpMode < 0 || m_sgm.iOpMode >= STIM_NPSGMMODES )
      m_sgm.iOpMode = 0;

   if( m_tStart < 0 ) m_tStart = 0;                                     // start time t0 must be >= 0

   if( m_iType == STIM_ISPSGM )                                    // validate ALL motion params for PSGM stim
   {
      int iVal = m_sgm.iAmp1 / 80;                                      //    amp1,2: [min..max] in 80mV increments
      if(iVal < SGM_MINPA) iVal = SGM_MINPA;
      else if(iVal > SGM_MAXPA) iVal = SGM_MAXPA;
      m_sgm.iAmp1 = iVal * 80;

      iVal = m_sgm.iAmp2 / 80;
      if(iVal < SGM_MINPA) iVal = SGM_MINPA;
      else if(iVal > SGM_MAXPA) iVal = SGM_MAXPA;
      m_sgm.iAmp2 = iVal * 80;

      iVal = m_sgm.iPW1 / 10;                                           //    pulseW1,2: [min..max] in 10us increments
      if(iVal < SGM_MINPW) iVal = SGM_MINPW;
      else if(iVal > SGM_MAXPW) iVal = SGM_MAXPW;
      m_sgm.iPW1 = iVal * 10;

      iVal = m_sgm.iPW2 / 10;
      if(iVal < SGM_MINPW) iVal = SGM_MINPW;
      else if(iVal > SGM_MAXPW) iVal = SGM_MAXPW;
      m_sgm.iPW2 = iVal * 10;

      iVal = m_sgm.iPulseIntv;                                          //    IPI: [min..max] in 1ms increments
      if(iVal < SGM_MINIPI) iVal = SGM_MINIPI;
      else if(iVal > SGM_MAXIPI) iVal = SGM_MAXIPI;
      m_sgm.iPulseIntv = iVal;

      iVal = m_sgm.iTrainIntv / 10;                                     //    ITI: [min..max] in 10ms increments
      if(iVal < SGM_MINITI) iVal = SGM_MINITI;
      else if(iVal > SGM_MAXITI) iVal = SGM_MAXITI;
      m_sgm.iTrainIntv = iVal * 10;

      if(m_sgm.nPulses < SGM_MINPULSES)                                 //    #pulses per train: [min..max]
         m_sgm.nPulses = SGM_MINPULSES;
      else if(m_sgm.nPulses > SGM_MAXPULSES)
         m_sgm.nPulses = SGM_MAXPULSES;

      if(m_sgm.nTrains < SGM_MINTRAINS)                                 //    #trains in sequence: [min..max]
         m_sgm.nTrains = SGM_MINTRAINS;
      else if(m_sgm.nTrains > SGM_MAXTRAINS)
         m_sgm.nTrains = SGM_MAXTRAINS;
   
      // adjust IPI, ITI, and #pulses as needed to ensure PA(+PB) < IPI and IPI*#pulses < ITI, depending on op mode...
      if(m_sgm.iOpMode == SGM_DUAL || m_sgm.iOpMode == SGM_TRAIN || m_sgm.iOpMode == SGM_BIPHASICTRAIN)
      {
         int pw = m_sgm.iPW1;
         if(m_sgm.iOpMode == SGM_BIPHASICTRAIN) pw += m_sgm.iPW2;
         while(m_sgm.iPulseIntv * 1000 <= pw) ++m_sgm.iPulseIntv;
      }

      if(m_sgm.iOpMode == SGM_TRAIN || m_sgm.iOpMode == SGM_BIPHASICTRAIN)
      {
         while(m_sgm.iPulseIntv * m_sgm.nPulses >= SGM_MAXITI * 10) --m_sgm.nPulses;
         while(m_sgm.iPulseIntv * m_sgm.nPulses >= m_sgm.iTrainIntv) m_sgm.iTrainIntv += 10;
      }
   }
   else if( m_iStdMode == MODE_ISSINE )                                 // validate motion params for SINE stim:
   {
      if( m_sine.iPeriod < 10 ) m_sine.iPeriod = 10;                    //    period in ms must be >= 10
      if( m_sine.nCycles < 1 ) m_sine.nCycles = 1;                      //    #cycles must be >= 1

      if( m_sine.fAmp < -9999.0f ) m_sine.fAmp = -9999.0f;              //    velocity amp in deg/s: restrict to
      else if( m_sine.fAmp > 9999.0f ) m_sine.fAmp = 9999.0f;           //    [-9999..9999].

      while( m_sine.fPhase < -180.0f ) m_sine.fPhase += 360.0f;         //    phase: restrict to [-180..180] deg
      while( m_sine.fPhase > 180.0f ) m_sine.fPhase -= 360.0f;

      while( m_sine.fDirec < -180.0f ) m_sine.fDirec += 360.0f;         //    direction: restrict to [-180..180] deg
      while( m_sine.fDirec > 180.0f ) m_sine.fDirec -= 360.0f;
   }
   else                                                                 // validate motion params for PULSE stim:
   {
      if( m_pulse.fAmp < -9999.0f ) m_pulse.fAmp = -9999.0f;            //    velocity pulse amp in deg/s: restrict to
      else if( m_pulse.fAmp > 9999.0f ) m_pulse.fAmp = 9999.0f;         //    [-9999..9999].

      if( m_pulse.iPulseDur < 2 ) m_pulse.iPulseDur = 2;                //    pulse duration in ms must be >= 2
      if( m_pulse.iRampDur < 2 ) m_pulse.iRampDur = 2;                  //    ramp duration in ms must be >= 2

      while( m_pulse.fDirec < -180.0f ) m_pulse.fDirec += 360.0f;       //    direction: restrict to [-180..180] deg
      while( m_pulse.fDirec > 180.0f ) m_pulse.fDirec -= 360.0f;
   }
}

//
//=====================================================================================================================
// END:  class CCxStimulus
//=====================================================================================================================




//=====================================================================================================================
// class CCxContRun
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxContRun, CTreeObj, 1 )


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== Initialize [base override] ======================================================================================
//
//    Initialize CNTRLX object after default construction.
//
//    This method MUST be called directly after default construction to initialize the newly constructed object with
//    the specified name, CNTRLX object type, and state flags.
//
//    If invoked on a run object that has already been initialized, the run is cleared and reinitialized.  Not really
//    intended for such usage.
//
//    ARGS:       s  -- [in] the name assigned to data object
//                t  -- [in] the CNTRLX object data type -- MUST be CX_CONTRUN.
//                f  -- [in] the object's initial state flags -- CANNOT include CX_ISSETOBJ.
//
//    RETURNS:    NONE.
//
VOID CCxContRun::Initialize( LPCTSTR s, const WORD t, const WORD f )
{
   ASSERT( t == CX_CONTRUN );                                        // validate run object type and flags
   ASSERT( (f & CX_ISSETOBJ) == 0 );

   if(GetStimulusCount() > 0)      // reinitialize to empty, default state
   {
      Clear();
      SetDefaults();
   }

   CTreeObj::Initialize( s, t, f );                                  // base class inits
}


//=== Copy [base override] ============================================================================================
//
//    Copy members of specified CNTRLX data object to THIS object (already constructed).
//
//    We do not require that both src & dst run object have the same # of stimuli, so this operation could change the
//    memory requirements of THIS run object -- either requiring allocation of additional stimulus channels or 
//    deallocation of extra channels.
//
//    For simplicity, we clear the run completely and start from scratch; the danger in doing so is that, if we have
//    a problem allocating memory as we make THIS run identical to the source run, we cannot restore the run to its
//    previous state!
//
//    ARGS:       pSrc  -- [in] CTreeObj* ptr to the run to be copied.  MUST point to a valid CCxContRun object!
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to allocate memory for stimuli. Run will NOT be in its original state if 
//                such an exception should occur. Caller should catch exception and delete this object!
//
VOID CCxContRun::Copy( const CTreeObj* pSrc )
{
   ASSERT( pSrc->IsKindOf( RUNTIME_CLASS( CCxContRun ) ) );       // source object MUST be a continuous-mode run!

   const CCxContRun* pSrcRun = (CCxContRun*) pSrc;                // validate source stimulus run
   ASSERT_VALID( pSrcRun );

   Clear();                                                       // start over!
   CTreeObj::Copy( pSrc );                                        // copy the base class stuff

   m_iDutyPeriod = pSrcRun->GetDutyPeriod();                      // copy general run parameters
   m_iDutyPulse = pSrcRun->GetDutyPulse();
   m_nAutoStop = pSrcRun->GetAutoStop();
   m_fHOffset = float( pSrcRun->GetHOffset() );
   m_fVOffset = float( pSrcRun->GetVOffset() );

   for(int i = 0; i < pSrcRun->GetStimulusCount(); i++)           // copy list of stimulus channels...
   {
      CCxStimulus* pStim = new CCxStimulus;                       // construct new, default stimulus channel object
      ASSERT( pStim != NULL );                                    // **** THROWS CMemoryException

      try
      {
         pStim->Copy( *(pSrcRun->GetStimulus( i )) );             // copy definition from source channel, and append
         m_Stimuli.AddTail( pStim );                              // object to the stimulus channel list
      }
      catch( CMemoryException* e )                                // if memory exception, must not leave the new stim
      {                                                           // channel dangling!  free it and pass on exception.
         UNREFERENCED_PARAMETER(e);
         delete pStim;
         throw;
      }
   }
}


//=== CopyRemoteObj [base override] ===================================================================================
//
//    Copies the CCxContRun-specific definition of a stimulus run object located in a different experiment document.
//
//    CopyRemoteObject was introduced to the CTreeObj/CTreeMap framework to overcome the problem of copying an object
//    from one treemap to another.  It is intended only for copying the internal information specific to a given
//    implementation of CTreeObj.
//
//    ARGS:       pSrc        -- [in] the object to be copied.  Must be an instance of CCxContRun.
//                depKeyMap   -- [in] maps keys of any objects upon which the source obj depends, which reside in the
//                               source doc, to the keys of the corresponding objects in the destination doc.
//
//    RETURNS:    TRUE if successful, FALSE if source object is not an instance of CCxContRun, or if the supplied
//                dependency key map is missing an entry for any dependencies in the source stimulus run.
//
BOOL CCxContRun::CopyRemoteObj(CTreeObj* pSrc, const CWordToWordMap& depKeyMap)
{
   if( pSrc == NULL || !pSrc->IsKindOf( RUNTIME_CLASS(CCxContRun) ) ) return( FALSE );

   const CCxContRun* pSrcRun = (CCxContRun*) pSrc;                   // validate source stimulus run
   ASSERT_VALID( pSrcRun );

   int i;
   WORD dstKey;

   CWordArray deps;                                                  // make sure dependency key map has an entry for
   pSrcRun->GetDependencies( deps );                                 // every obj upon which the source run depends!
   for( i=0; i<deps.GetSize(); i++ )
   {
      if( !depKeyMap.Lookup( deps[i], dstKey ) )
         return( FALSE );
   }

   Clear();                                                          // start with an empty run defn

   m_iDutyPeriod = pSrcRun->GetDutyPeriod();                         // copy general run parameters
   m_iDutyPulse = pSrcRun->GetDutyPulse();
   m_nAutoStop = pSrcRun->GetAutoStop();
   m_fHOffset = float( pSrcRun->GetHOffset() );
   m_fVOffset = float( pSrcRun->GetVOffset() );

   for( i = 0; i < pSrcRun->GetStimulusCount(); i++ )                // copy list of stimulus channels...
   {
      CCxStimulus* pStim = NULL;
      try
      {
         pStim = new CCxStimulus;                                    // construct new, default stimulus channel object
         pStim->Copy( *(pSrcRun->GetStimulus( i )) );                // copy definition from source channel, and append
         m_Stimuli.AddTail( pStim );                                 // object to the stimulus channel list
      }
      catch( CMemoryException* e )                                   // if memory exception, clear defn and abort.
      {
         UNREFERENCED_PARAMETER(e);
         if( pStim != NULL ) delete pStim;
         Clear();
         return( FALSE );
      }
   }

   return( TRUE );
}



//=====================================================================================================================
// ATTRIBUTES
//=====================================================================================================================

//=== GetDependencies [base override] =================================================================================
//
//    Return a list of keys identifying those Maestro data objects which are currently referenced by this object.
//    This method is required by the CTreeMap/CTreeObj framework in order to "lock" the "independent" objects in the
//    treemap -- providing a mechanism that prevents user from removing them and thereby corrupting the "dependent"
//    object's definition.
//
//    A continuous-mode run is "dependent" only upon any XY scope targets appearing in its XYseq target list.
// 
//    30sep2024: The XYScope platform is unsupported since Maestro 4.0 and is removed a/o Maestro 5. So there is no
//    such thing as the XYSeq target list. However, in order to handle reading in and migrating pre-V5.0 documents that
//    contained XYScope targets and XYseq stimulus runs, CCxContRun still maintains a list of XYSeq targets. During
//    deserialization of a pre-V5 document, this list will get loaded with the keys of the XYScope targets in the list,
//    and during migration, CCxDoc calls this method to identify all stimulus runs with XYScope target dependencies;
//    these runs are removed from the document during migration.
//
//    ARGS:       wArKeys -- [out] currently referenced CNTRLX object keys are stored here.  if none, array emptied.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to grow array to sufficient size to hold all dependencies
//
VOID CCxContRun::GetDependencies( CWordArray& wArKeys ) const
{
   wArKeys.SetSize( 0, MAXTGTSINXYSEQ );           // empty array, grow size sufficient to hold max# dependencies

   POSITION pos = m_XYseqTgts.GetHeadPosition();
   while( pos != NULL )
   {
      CXYseqTgt* pTgt = m_XYseqTgts.GetNext( pos );
      wArKeys.Add( pTgt->wKey );
   }
}


//=====================================================================================================================
// OPERATIONS - GENERAL
//=====================================================================================================================

//=== GetDefinition ===================================================================================================
//
//    Recasts the definition of the continuous-mode run object as a CONTRUN structure, which is suitable for storage in
//    the Maestro-CXDRIVER shared-memory interface (C++/MFC objects cannot be passed across that interface!).
//
//    NOTE: Suited only for passing the run definition to CXDRIVER in preparation for a run. Any inactive stimuli are
//    ignored. Do NOT use this method to obtain a complete copy of the run's current contents.
//
//    30sep2024: No longer supports XYseq stimuli - deprecated a/o Maestro 5.0.
//
//    ARGS:       runDef   -- [out] run definition in CXDRIVER-compatible format.
//
//    RETURNS:    NONE.
//
VOID CCxContRun::GetDefinition( CONTRUN& runDef ) const
{
   int i;

   runDef.iDutyPeriod = m_iDutyPeriod;                                  // the general run parameters
   runDef.iDutyPulse = m_iDutyPulse;
   runDef.nAutoStop = m_nAutoStop;
   runDef.fHOffset = m_fHOffset;
   runDef.fVOffset = m_fVOffset;

   int nActive = 0;                                                     // stimulus channel list:  load only
   for( i = 0; i < GetStimulusCount(); i++ ) if( IsStimulusOn( i ) )    // those channels that are active...
   {
      GetStimulus( i )->GetStimulusInfo( runDef.stim[nActive] );
      ++nActive;
   }
   runDef.nStimuli = nActive;

   runDef.nXYTgts = 0;   // XYseq stimulus DEPRECATED
}


//=== InsertStimulus ==================================================================================================
//
//    Insert a new stimulus channel at the specified position (zero-based index) in the stimulus list.  Existing stims
//    are moved down to make room, and the new stimulus is initialized with default values.
//
//    ARGS:       iPos  -- [in] zero-based insertion position; if invalid pos, stimulus channel is appended.
//
//    RETURNS:    Zero-based position of new stimulus channel if successful; -1 if stimulus list is full.
//
//    THROWS:     CMemoryException if unable to allocate stimulus channel object or insert it into the list.  In such a
//                case, we clean up any allocations that were made here prior to the exception to avoid memory leaks,
//                then pass on the exception.
//
int CCxContRun::InsertStimulus( int iPos )
{
   int nCount = GetStimulusCount();
   if( nCount == MAXSTIMULI ) return( -1 );                    // stimulus channel list is maxed out!

   BOOL bAfter = FALSE;                                        // insert before stimulus at specified pos, unless...
   int iNew = iPos;
   if( nCount == 0 )                                           // ...list empty:  inserting first stim channel
      iNew = 0;
   else if ( (iNew < 0) || (iNew >= nCount) )                  // ...invalid pos:  append by inserting after last rec
   {
      iNew = nCount-1;
      bAfter = TRUE;
   }

   CCxStimulus* pStim = new CCxStimulus;                       // create default stimulus channel
                                                               // **** THROWS CMemoryException

   try                                                         // insert stimulus ptr into stim channel list.  guard
   {                                                           // againt memory exceptions...
      if( nCount == 0 )                                        //    first segment (arg ignored in this case)
         m_Stimuli.AddHead( pStim );                           //    **** THROWS CMemoryException
      else                                                     //    general insertion case:
      {
         POSITION pos = m_Stimuli.FindIndex( iNew );
         if( bAfter ) m_Stimuli.InsertAfter( pos, pStim );     //    **** THROWS CMemoryException
         else m_Stimuli.InsertBefore( pos, pStim );            //    **** THROWS CMemoryException
      }
   }
   catch( CMemoryException *e )                                // caught memory excpt; must release new stim obj before
   {                                                           // passing on exception so it's not left dangling!
      UNREFERENCED_PARAMETER(e);
      delete pStim;
      throw;
   }

   return( (bAfter) ? (iNew+1): iNew );
}


//=== RemoveStimulus ==================================================================================================
//
//    Remove an existing stimulus channel at the specified position (zero-based index) in the stimulus list.  Unlike
//    CutStimulus(), this method frees the memory allocated to the stimulus channel that was removed.
//
//    ARGS:       iPos  -- [in] zero-based position of stimulus channel to be deleted.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos or list empty), in which case stim list unchanged.
//
BOOL CCxContRun::RemoveStimulus( int iPos )
{
   CCxStimulus* pDeadStim = CutStimulus( iPos );
   if( pDeadStim != NULL )
   {
      delete pDeadStim;
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== CutStimulus =====================================================================================================
//
//    Remove specified stimulus channel from the stimulus list.  Instead of deleting the CCxStimulus object, we return
//    a pointer to it, leaving its disposition to the caller's discretion.
//
//    ARGS:       iPos     -- [in] zero-based position of the stimulus channel to be cut.
//
//    RETURNS:    Pointer to the stimulus object that was cut.  Caller is responsible for freeing the object when
//                finished with it!  If unsuccessful, returns NULL.
//
CCxStimulus* CCxContRun::CutStimulus( int iPos )
{
   if( (iPos < 0) || (iPos >= GetStimulusCount()) ) return( NULL );  // invalid pos or list empty

   POSITION pos = m_Stimuli.FindIndex( iPos );                       // get ptr to segment to be deleted
   CCxStimulus* pStim = m_Stimuli.GetAt( pos );
   m_Stimuli.RemoveAt( pos );                                        // remove ptr from list
   return( pStim );
}


//=== CopyStimulus ====================================================================================================
//
//    Make a duplicate of the stimulus channel at the specified pos in the stimulus list.  This new stim channel is NOT
//    inserted in the run itself.  Instead, its pointer is passed to the caller, which can then "paste" the stimulus
//    into this run or another run object via the PasteStimulus() method.
//
//    ARGS:       iPos     -- [in] zero-based position of the stimulus channel to be copied.
//
//    RETURNS:    Pointer to the stimulus copy.  Caller is responsible for freeing the object when finished with it!
//                If unsuccessful, returns NULL.
//
//    THROWS:     CMemoryException if unable to allocate the new stimulus channel object.
//
CCxStimulus* CCxContRun::CopyStimulus( int iPos ) const
{
   if( (iPos < 0) || (iPos >= GetStimulusCount()) ) return( NULL );  // invalid pos

   CCxStimulus* pStim = RetrieveStimulus( iPos );                    // stimulus channel to be copied
   CCxStimulus* pCopyStim = new CCxStimulus;                         // allocate new stimulus channel
   pCopyStim->Copy( *pStim );                                        // and do the copy
   return( pCopyStim );
}


//=== PasteStimulus ===================================================================================================
//
//    Paste a duplicate of the provided stimulus channel into the stimulus list at the specified position.  If the pos
//    is invalid, the new stimulus object is appended to the list.
//
//    NOTE that we do NOT simply insert the provided stimulus object ptr into the list, as this would leave the object
//    unprotected (the caller could delete it without going through CCxContRun methods!).  We insert a new stimulus at
//    the specified pos and make it the same as the pasted stimulus channel.
//
//    If the pasted stimulus is an active PSGM, we must turn off the currently active PSGM before pasting -- enforcing 
//    the rule that only one PSGM can be active in the run.
//
//    ARGS:       iPos  -- [in] zero-based insertion position; if invalid pos, stimulus channel is appended.
//                pStim -- [in] ptr to the stim channel to be pasted.
//
//    RETURNS:    Zero-based position of new stimulus channel if successful; -1 if paste op is not possible.
//
//    THROWS:     CMemoryException if unable to allocate stimulus channel or insert it into list.
//
int CCxContRun::PasteStimulus( int iPos, const CCxStimulus* pStim )
{
   ASSERT_VALID( pStim );                                   // validate the stimulus channel object
   if( GetStimulusCount() == MAXSTIMULI ) return( -1 );     // if stimulus list is full, we cannot do the paste

   int iIns = InsertStimulus( iPos );                       // insert a new stim channel (**** THROWS CMemoryException)
   if ( iIns < 0 ) return( -1 );                            //
   CCxStimulus* pNew = RetrieveStimulus( iIns );            // get ptr to new stimulus and copy the paste stim to it
   pNew->Copy( *pStim );

   int iType = pNew->GetType();                             // if pasted stimulus is an active PSGM channel
   if(pNew->IsOn() && (iType == STIM_ISPSGM))               // deactivate all other PSGM channels
      DeactivateAllOthers( pNew );

   return(iIns);
}


//=== ReplaceStimulus =================================================================================================
//
//    Replace the definition of a specified stimulus channel in the stimulus list.
//
//    NOTE that we do NOT simply insert the provided stimulus channel ptr into the list, as this would leave the stim
//    channel object unprotected (the caller could delete it without going through CCxContRun methods!).
//
//    If the replacement stimulus is an active PSGM, we must turn off any other active PSGM -- enforcing the rule that 
//    only one PSGM can be active in the run.
//
//    ARGS:       iPos  -- [in] zero-based position of stimulus channel to be replaced.
//                pStim -- [in] ptr to the replacement channel definition.
//
//    RETURNS:    TRUE if successful; FALSE if replace op is not possible.
//
BOOL CCxContRun::ReplaceStimulus( int iPos, const CCxStimulus* pStim )
{
   ASSERT_VALID( pStim );                                      // validate the replacement stimulus channel object

   if( IsValidStimulus( iPos ) )
   {
      CCxStimulus* pS = RetrieveStimulus( iPos );              // retrieve ptr to existing stimulus
      pS->Copy( *pStim );                                      // and copy replacement stimulus into it.

      int iType = pS->GetType();                               // if replacement is an active PSGM channel,
      if(pS->IsOn() && (iType == STIM_ISPSGM))                 // deactivate all other PSGM channels
         DeactivateAllOthers( pS );

      return( TRUE );
   }
   else
      return( FALSE );
}


//=== ClearStimuli ====================================================================================================
//
//    Empty the stimulus channel list
//
//    ARGS:       NONE
//
//    RETURNS:    NONE.
//
VOID CCxContRun::ClearStimuli()
{
   while( !m_Stimuli.IsEmpty() )
   {
      CCxStimulus* pStim = m_Stimuli.RemoveHead();
      delete pStim;
   }
}


//=== Clear ===========================================================================================================
//
//    Deletes all stimuli from the stimulus channel list, and resets general run parameters to their default values.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContRun::Clear()
{
   ClearStimuli();
   SetDefaults();
}


//=== Serialize [base override] =======================================================================================
//
//    Handles reading/writing the Maestro stimulus run object from/to a disk file via a serialization archive.
//
//    NOTE: The XYScope platform, unsupported since V4.0, is dropped for V5.0. Hence, the XYSeq stimlus type is also
//    deprecated. This method throws an exception if there is an attempt to save a run using the XYSeq stimulus
//    channel, but it still supports reading in such an object so that Maestro can load and migrate older experiment
//    documents containing XYSeq runs.
// 
//    ARGS:       ar -- [in] the serialization archive.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, CArchiveException, CFileException.  When deserializing, framework should delete
//                the "partial" object if an exception occurs while it is being read in from file.
//
void CCxContRun::Serialize ( CArchive& ar )
{
   CTreeObj::Serialize( ar );                               // base class stuff first
   m_Stimuli.Serialize( ar );                               // the stimulus channels

   int i, nTgts;
   if(ar.IsStoring())
   {
      ar << m_iDutyPeriod << m_iDutyPulse << m_nAutoStop;   // the general run parameters...
      ar << m_fHOffset << m_fVOffset;
   }
   else
   {
      ASSERT( m_XYseqTgts.GetCount() == 0 );                // (always deserialize to an initially empty tgt list!)
      ar >> nTgts;
      try
      {
         for( i = 0; i < nTgts; i++ )
         {
            CXYseqTgt* pTgt = new CXYseqTgt;
            ar >> pTgt->wKey >> pTgt->fCtrX >> pTgt->fCtrY;
            m_XYseqTgts.AddTail( pTgt );
         }
      }
      catch( CException* e )
      {
         UNREFERENCED_PARAMETER(e);
         while(!m_XYseqTgts.IsEmpty())
         {
            CXYseqTgt* pTgt = m_XYseqTgts.RemoveHead();
            delete pTgt;
         }
         throw;
      }

      ar >> i; SetDutyPeriod( i );                          // auto-correct bad data; just in case...
      ar >> i; SetDutyPulse( i );
      ar >> i; SetAutoStop( i );

      float f;
      ar >> f; SetHOffset( f );
      ar >> f; SetVOffset( f );
   }

   ASSERT_VALID( this );                                    // check validity AFTER serializing
}


//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================

//=== GetDutyPulseChoices =============================================================================================
//
//    Initialize a string array containing all the possible choices for the run's duty marker pulse channel.
//
//    ARGS:       choices  -- [out] string array that will contain the choices.
//
//    RETURNS:    NONE.
//
VOID CCxContRun::GetDutyPulseChoices( CStringArray& choices )
{
   choices.RemoveAll();
   choices.Add( _T("OFF") );
   for( int i = 1; i <= STIM_NLASTMARKER; i++ )
   {
      CString str;
      str.Format( "DOUT%d", i );
      choices.Add( str );
   }
}


//=== SetStimParameter ================================================================================================
//
//    Change the value of the specified parameter for the specified stimulus channel. Illegal parameter values are
//    auto-corrected.
//
//    Only one PSGM stimulus channel can be turned ON at any time (though more than one PSGM channel can be defined).
//    We enforce this restriction here by turning OFF any other active PSGM stimulus channel.
//
//    NOTE:  Any view class that displays CCxContRun should be aware of the possible "side effects" of changing a
//    single stimulus parameter. As mentioned above, turning ON a PSGM stimulus will automatically turn OFF any other 
//    PSGM channel. In addition, changing certain stimulus channel parameters may affect other parameter(s) in that 
//    channel's definition -- see CCxStimulus::SetParameter(). We return TRUE here if a side effect has or may have 
//    occurred.
//
//    ARGS:       i           -- [in] index of stimulus channel.
//                j           -- [in] index of parameter to be modified.
//                dVal        -- [in] the new value for the parameter.
//
//    RETURNS:    TRUE if a "side effect" has or may have occurred; FALSE otherwise.
//
BOOL CCxContRun::SetStimParameter( int i, int j, double dVal )
{
   if( !IsValidStimulus( i ) ) return( FALSE );
   CCxStimulus* pStim = RetrieveStimulus( i );
   int iOldType = pStim->GetType();
   BOOL bWasOff = !pStim->IsOn();
   BOOL bSideEffect = pStim->SetParameter( j, dVal );

   int iType = pStim->GetType();
   if((iType == STIM_ISPSGM) && pStim->IsOn() && (iType != iOldType || bWasOff) )
   {
      if(DeactivateAllOthers(pStim)) bSideEffect = TRUE;
   }

   return( bSideEffect );
}



//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump contents of the run object in an easy-to-read form to the supplied dump context.  Intelligent dump is
//    tailored to the specific contents of trial.  To see a detailed dump of the stimulus channel definitions and the
//    XYseq participating target list, the dump depth must be > 0.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxContRun::Dump( CDumpContext& dc ) const
{
   CTreeObj::Dump( dc );

   dc << "********Maestro Run Object********";

   CString msg;
   msg.Format( "\nDuty period = %d ms, Duty pulse = %d, Autostop = %d, HOffset = %.2f, VOffset = %.2f",
               m_iDutyPeriod, m_iDutyPulse, m_nAutoStop, m_fHOffset, m_fVOffset );
   dc << msg;

   msg.Format("\nContains %d stimulus channels", GetStimulusCount());
   dc << msg;

   if( dc.GetDepth() > 0 )
   {
      dc << _T("\nSTIMULUS CHANNEL DEFINITIONS:");
      m_Stimuli.Dump( dc );
   }
   dc << "\n\n";
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate the trial object.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxContRun::AssertValid() const
{
   CTreeObj::AssertValid();                        // validate base class stuff
   ASSERT( m_type == CX_CONTRUN );                 // this is the only CNTRLX obj type supported by this class
   m_XYseqTgts.AssertValid();                      // check the list containers
   m_Stimuli.AssertValid();                        //
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== SetDefaults =====================================================================================================
//
//    Assign default values to the run's general parameters.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContRun::SetDefaults()
{
   m_iDutyPeriod = 10000;
   m_iDutyPulse = 0;
   m_nAutoStop = 0;
   m_fHOffset = 0.0;
   m_fVOffset = 0.0;
}


//=== DeactivateAllOthers =============================================================================================
//
//    Deactivate (ie, turn "OFF") all stimulus channels in the run that are of the same type as the specified channel.
//    The specified channel is unaffected. This convenience method is used to enforce the rule that a run may have
//    only one active ("ON") PSGM stimulus; however, it can be used for any stimulus type.
//
//    ARGS:       pStim -- [in] the stimulus channel that will remain "ON"; all other channels OF THE SAME TYPE are
//                         turned OFF.
//
//    RETURNS:    TRUE if at least one stimulus channel was turned "OFF"; FALSE otherwise.
//
BOOL CCxContRun::DeactivateAllOthers( CCxStimulus* pStim )
{
   BOOL bDeactivated = FALSE;
   int iType = pStim->GetType();
   POSITION pos = m_Stimuli.GetHeadPosition();
   while( pos != NULL )
   {
      CCxStimulus* pS = m_Stimuli.GetNext( pos );
      if( (pS != pStim) && (pS->GetType() == iType) && pS->IsOn() )
      {
         pS->SetOn( FALSE );
         bDeactivated = TRUE;
      }
   }
   return( bDeactivated );
}

//
//=====================================================================================================================
// END:  class CCxContRun
//=====================================================================================================================

