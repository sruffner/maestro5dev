//=====================================================================================================================
//
// cxtrial.cpp : Implementation of class CCxTrial, encapsulating a MAESTRO "trial object", and class CCxSegment, which
//               encapsulates a single "segment" within a trial.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This class encapsulates the definition of a MAESTRO "trial". It provides a single entity for storing the complete
// definition of the trial. It also provides a set of operations for accessing and/or modifying this definition. The
// trial object is, by far, the largest and most complex MAESTRO object. Its data composition:
//
//    1) A trial "header" containing a number of control flags and other parameters (trial weight, save/keep flag,
//       channel set designation, special operation, etc). See TRLHDR in cxobj_ifc.h.
//    2) One or more participating targets (up to MAX_TRIALTARGS).
//    3) One or more trial segments (up to MAX_SEGMENTS), containing...
//       a) Segment "header" parameters, such as min/max duration, designated fixation targets for that segment,
//          fixation requirements during that segment, etc. See SEGHDR in cxobj_ifc.h.
//       b) A target "trajectory record" for each target participating in the trial. This record contains motion
//          parameters that define how each target will behave during that segment.  See TRAJINFO in cxobj_ifc.h.
//    4) A list of perturbation objects that modulate the defined trajectories of selected velocity components of
//       selected targets in the trial.
//
// Because a trial segment is so complex, we encapsulate it by another class, CCxSegment, also defined here.  We put
// its definition with CCxTrial because it is intended only for use by a trial object. This design makes it relatively
// easy to work with segment objects as single entities -- given CCxSegment, the implementation of CCxTrial is much
// simplified. [NOTE: Trial segments are not Maestro objects! They do not exist apart from a containing trial object.
// While other classes could theoretically construct and manipulate a CCxSegment object, CCxTrial exercises complete
// control over its segments. To insert a segment into a trial, callers must invoke a CCxTrial method which, in turn,
// constructs and initializes a new CCxSegment object and inserts that object into its internal segment list. It is
// also possible to copy & paste a segment from one trial to another thru the CopySeg() and PasteSeg() methods.]
//
// ==> The Big Picture: Storage of CNTRLX data objects.
// The user creates experimental protocols within a CNTRLX "experiment document" (CCxDoc) by defining a variety of
// "data objects" and establishing relationships among those objects.  For instance, each CNTRLX "trial" defines the
// trajectories of one or more "targets", which are defined separately.  The trial object also refers to a "channel
// set" object, which contains the list of analog channels that should be sampled during that trial.  Trials, targets,
// and channel sets are examples of "abstract" data classes defined in CNTRLX.
//
// CNTRLX data objects are stored in the CNTRLX object trees, encapsulated by CCxTreeMap.  This "tree map" collection
// stores all the data objects in several different hierarchical trees (the "target tree", "trial tree", and so on).
// We chose this somewhat complex storage scheme in order to organize the different data objects in a logical manner,
// and to provide the potential for storing a large # of objects in a single document yet be able to access any
// individual object rapidly via a unique key value (hence the "map" in "tree map").  CCxTreeMap can store up to
// 65535 different objects, more than enough for our purposes.
//
// CCxTreeMap is derived from the generic CTreeMap class, which handles the low-level implementation details of the
// tree map (see TREEMAP.CPP).  CTreeMap itself handles one base data class, CTreeObj, which merely stores the object's
// name and abstract data type and serves as the starting point for building more complex data classes.  CCxTreeMap
// tailors the behavior of CTreeMap so it can handle all data types present in CNTRLX.  Each CNTRLX data class must
// satisfy these constraints in order to build the CNTRLX object trees on top of the CTreeMap/CTreeObj framework; see
// the CTreeMap/CTreeObj implementation file for an explanation of these constraints.  CCxTrial has been designed with
// these constraints in mind.
//
// There is a division of responsibilities among CCxDoc, CCxTreeMap, and the various CTreeObj-derived classes that
// represent the real CNTRLX data objects.  First, the CNTRLX data object classes provide methods for accessing,
// modifying, and validating the actual data which define how the object behaves in a CNTRLX experiment.  CCxTreeMap is
// the "intelligent" storage medium for these objects (leaf nodes in the tree-map) and "collections" of related
// objects. It must be "aware" of all the different types of CNTRLX data objects so that it can construct any given
// object by calling the appropriate constructor.  Furthermore, it controls the naming of the objects, allowing only
// characters from a valid character set (it uses the default char set provided by CTreeMap) and requiring that no two
// sibling objects have the same name.  Finally, of course, it encodes the tree connections among the objects and
// provides methods for adding objects to the trees, removing objects, etc.  However, it does NOT impose any
// restrictions on how objects are added to the tree-map; that is the responsibility of CCxDoc, in coordination with
// its various views.  As mentioned above, CCxDoc uses CCxTreeMap to store a number of different "CNTRLX object trees";
// CCxDoc methods implement the logic for constructing and restricting the exact composition of these object trees (see
// CCxDoc for details).
//
// CCxTrial represents the "data class" which handles all trials in CNTRLX.  It stores a single abstract data type,
// identified by the defined constant CX_TRIAL.
//
// ==> Using CCxTrial.
// As explained above, CCxTrial is designed for use with the CNTRLX object tree container CCxTreeMap and the
// underlying CTreeMap/CTreeObj framework.  Thus, the default constructor, destructor, and the Copy() and Initialize()
// methods are all protected.  The idea is that only CCxTreeMap can construct, copy, and destroy CCxTrial objects.  In
// addition, CCxTrial overrides CTreeObj::GetDependencies() because any useful trial object depends on at least one
// other CNTRLX object for its full definition.  Target objects, perturbation objects, and a channel configuration are
// all "independent" data objects upon which a trial's definition depends.  The keys of such "object dependencies" are
// stored within the trial object itself, and CTreeMap must "lock" these objects to prevent the user from deleting them
// -- which would make the trial's definition invalid.  See CTreeMap for more information on this issue.
//
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//       !!! Whenever views make a change to a trial object, they must inform CCxDoc via !!!
//       !!! CCxDoc::UpdateObjDep().  Otherwise, the dependency locking scheme will fail !!!
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// In the CNTRLX design scheme, a view can obtain a pointer to a particular trial object by querying CCxDoc, which
// includes a CCxTreeMap container for managing the CNTRLX object trees.  The view can then edit the trial by invoking
// various public methods.  Below is a summary of the allowed operations:
//
//    Get/SetHeader()         ==> For manipulating data in the trial's "header". SetHeader() corrects any illegal
//                                parameters and returns the new state of header.
//    Insert/RemoveTarget()   ==> Insert or remove a target at any position in the trial's participating target list.
//                                NOTE that trial is limited to MAX_TRIALTARGS targets.
//    Insert/RemoveSeg()      ==> Insert or remove a segment at any position in the trial's segment list.  NOTE that
//                                trial is limited to MAX_SEGMENTS segments.
//    Copy/PasteSeg()         ==> CopySeg() provides a **copy** (NOT a reference to) of a particular seg in the trial.
//                                The view can then call PasteSeg() to paste a **duplicate** of the paste segment to
//                                another pos in the trial, OR to a different trial.  The paste operation only works if
//                                the paste seg contains the same # of targets as the trial.
//    Get/SetSegHeader()      ==> For manipulating data in a particular segment's "header".  SetSegHeader() corrects
//                                any illegal parameters.
//    Get/SetSegTrajInfo()    ==> For manipulating data in a particular target's "trajectory record" within a
//                                particular segment.  SetSegTrajInfo() corrects any illegal parameters.
//    Clear()                 ==> Removes all targets and segments from the trial.
//    Seg/TargCount()         ==> Number of segments and participating targets currently defined in trial.
//
// Accessor methods are also provided to retrieve each of the individual parameters in the trial's definition.  See,
// e.g., GetWeight().  In addition, "SetXxxx()" methods are provided for changing the individual parameters within a
// trial segment.
//
// It is important to note that the trial object never provides DIRECT access to its segments or target list.  All
// changes must be made by invoking CCxTrial methods.  In particular, CopySeg() does not provide a pointer to the
// desired segment, but to a copy of that segment; thus, a view cannot modify the segment directly by invoking methods
// thru that pointer!  Similarly, PasteSeg() does not insert the paste segment itself into the trial's segment list;
// rather, it inserts a copy of that paste segment.
//
// ==> Perturbations in a trial.
// CCxTrial supports the application of up to MAX_TRIALPERTS velocity perturbations during a trial.  For each pert obj
// in its "perturbation list", CCxTrial stores the object key that uniquely identifies the perturbation in the CNTRLX
// object tree, the desired amplitude (in deg/s or deg) for the perturbation (all CNTRLX perturbations are defined with 
// unit amplitude), the zero-based index of the trial target to be perturbed, the affected quantity (horiz velocity, 
// vertical velocity, vector direction, or vector speed of the target's window or pattern velocities), and the segment 
// at which the perturbation starts.  Note that, with this scheme, it is perfectly reasonable to use the same 
// perturbation object in each entry of the perturbation list, or to apply multiple perturbations to a particular 
// quantity of a particular target during a particular segment....  Here is a list of trial operations related to 
// perturbations:
//
//    AppendPert()            ==> Append a perturbation to the trial's perturbation list.  NOTE that the trial is
//                                limited to MAX_TRIALPERTS perturbations.  Initially, the perturbation has no effect.
//                                It must be assigned to a particular segment, target, and trajectory velocity cmpt.
//    RemovePert()            ==> Remove a selected perturbation (or all of them) from the trial's perturbation list.
//    Get/SetPertKey()        ==> Get/set the identity (ie, object key) of a perturbation entry.
//    Get/SetPertAmp()        ==> Get/set the desired amplitude.
//    Get/SetPertTgt()        ==> Get/set zero-based pos of trial target affected.
//    Get/SetPertTrajCmpt()   ==> Get/set what velocity trajectory aspect is affected. Use the PERT_ON_** constants.
//    Get/SetPertSeg()        ==> Get/set zero-based pos of segment at which perturbation starts.
//
// Since we must specify the target affected by a perturbation and the segment at which the perturbation starts, each
// entry in the perturbation list is intimately connected to the segment table.  Thus, whenever we make a structural
// change to the segment table, we update the perturbation list as needed.  For example, if we delete a segment before
// the start segment of a perturbation, that start segment must be decremented.  Or, if we remove a target to which a
// perturbation was applied, then the tgt index associated with that perturbation becomes -1 -- which means that
// perturbation will no longer have an effect on the trial.
//
// ==> Support for response distribution-based reward contingency protocol:  CCxRPDistro.
//
// Maestro v1.4 introduced a new special operation, called "R/P Distro", that is part of a special protocol aimed at
// altering the *distribution* of responses through motivational techniques.  CCxTrial uses a CCxRPDistro object to
// store the runtime information (type of response measured, response distributions, reward window, reward/penalty 
// stats) during execution of this protocol. The object is exposed directly by the method GetRPDistro(). It is solely 
// for use during Trial mode runtime. It is not part of the trial's definition and is not serialized with CCxTrial. 
// GetRPDistro() will return a valid CCxRPDistro pointer only when the trial object is using the "R/P Distro" op.
//
// For more information, see CCxRPDistro.
//
// ==> Trial random variables (RV).
// Maestro v3.3.0 introduces support for up to 10 "random variables" in a trial object. A random variable takes on a 
// new value each time the trial is presented during a trial sequence, and the values are distributed IAW the RV's
// definition. Uniform, normal, exponential, and gamma distributions are supported, along with a function-type RV that
// is simply a function of other RVs defined on the trial.
//
// Rather than introducing RVs as another Maestro "object" which can be referenced by multiple trials, each trial has
// its own set of 10 RVs, any subset of which may be defined for use. The internal structure CRVEntry encapsulates the
// definition of an RV, and m_Vars[] is the set of 10 RVs available for use in the trial. If none are used, all will
// have RV type RV_NOTUSED.
//
// To use an RV, it must be assigned to a parameter in the trial's segment table. An RV can govern segment duration
// or any of the 10 floating-point target trajectory parameters. See the relevant methods on how to assign a segment 
// table parameter to an RV instead of a constant.
//
// With RV-assignable parameters, we must be careful to use the correct methods when editing the parameter or when
// getting its current value for a trial presentation. When such a parameter is not a constant but is assigned to an RV,
// its value will change each time the trial is presented. Before generating the trial codes for a trial, Maestro's
// trial sequencer (CCxTrialSequencer), will call CCxTrial::UpdateRVs() to generate new values for any defined RVs. It
// then calls GetCurr***() methods to get the current value for any RV-assignable parameter. When the parameter is just
// a constant, that constant value is returned; otherwise, the current value of the assigned RV is returned.
//
// ==> Random reward withholding variable ratio (as of Maestro 4.1.0)
// In behavioral paradigms, continuous reinforcement of the desired behavior during initial training is often followed
// by a partial reinforcement schedule, in which the subject does not get the reinforcement every time they perform the
// behavior. A common type of partial reinforcement is "variable ratio" N/D, in which the reward is withheld for a
// random selection of N correct responses out of every D chances.
// 
// CCxTrial now supports this feature for both reward pulse 1 and 2 (thus, random withholding can be used in normal
// trials and in those that involve selecting or choosing one of two possible fixation targets). The TRLHDR now 
// includes 3 parameters per reward pulse (TRLHDR.reward1[], .reward2[]) -- the pulse length in ms, plus the numerator
// N and denominator D for the reward withheld variable ratio (WHVR). N = 0 disables WHVR (the default). Otherwise,
// 0 < N < D <= TH_MAXWHVR.
//
// CCxTrial also manages the runtime state needed to implement the random withholding feature during trial sequencing.
// The trial object maintains a randomly shuffled list of integers for each of the two trial reward pulses. Two methods
// initialize and update these two lists during trial sequencing. At the start of trial sequencing, CCxTrialSequencer
// must call InitRewardWHVR() on each participating trial. This method will populate the list with D randomly shuffled
// integers, N of which are 0 and the rest 1; if N=0, the list remains empty and no withholding occurs. Each time a 
// trial is to be presented, CCxTrialSequencer calls UpdateRewardWHVR() to determine whether or not a reward is to be
// withheld for that trial rep. This method simply removes the head of the list and withholds the corresponding reward
// if it is zero, else the reward is given. Once the list is empty, it is reinitialized and shuffled again. This 
// ensures that, for every D repetitions of the trial, the reward is withheld N times. The reason we do not simply use
// a call to rand() each time a withholding decision is made is because that approach won't guaranteed N of every D
// trial reps are withheld.
//
// REVISION HISTORY:
// 02jun2000-- Created.  Only contains a dummy parameter.  Right now I'm working on creation of trial sets and
//             individual trials.  Note that trial name is not contained in object!
// 13sep2000-- Now derived from CTreeObj and methods modified for use in the new "tree map" framework founded upon
//             CTreeMap/CTreeObj.  The CNTRLX object trees are encapsulated by the CTreeMap-derived CCxTreeMap.
// 04oct2000-- Began development of CCxTrial and helper class CCxSegment in earnest.
// 20oct2000-- CCxSegment implementation revised to handle memory exceptions.  Also, to avoid possibility of an
//             exception occurring in a constructor or operator=, made the following changes:  (1) the copy constructor
//             and operator=() are unavailable.  (2) the only constructor is the default constructor, which does not
//             allocate any memory or do anything else which might cause an exception.  (3) Copy() method was added.
//             So to copy a segment object, first create one using the default constructor, then invoke Copy() passing
//             a reference to the segment obj to be copied.  Copy() performs the equivalent of an assignment.
// 27oct2000-- First version of CCxTrial completed.  Still to be built and tested in the existing CNTRLX framework.
// 11dec2001-- MAJOR mods to the structures in CXOBJ_IFC.H that encapsulate a trial's definition.  Modified CCxTrial
//             IAW these changes.  E.g., dropped support for perturbations (for now), and added support for multiple
//             staircase sequencing.  See also CXOBJ_IFC.H.
// 19dec2001-- Another mod to TRLHDR and TRAJINFO structs re: XY interleave feature.  Instead of flagging selected
//             targets for interleaving on a segment-by-segment basis (which the existing code from cntrlxPC does not
//             support), we specify a # of targets, N, for interleaving in the trial header.  The first N XY scope tgts
//             in the trial's target list are then marked for interleaving throughout the entire trial.
// 27dec2001-- Added TRLHDR member 'iSaccVt' to specify the saccade threshold velocity for trials w/ a saccade-trig'd
//             special operation.  Int-valued threshold in deg/sec is limited to range TH_MINSACCVT..TH_MAXSACCVT.
//          -- Added a complete set of accessor methods to any trial parameter, including trajectory parameters in any
//             segment.  An alternative to use of TRLHDR, SEGHDR, and TRAJINFO structs...
// 15jan2002-- Modifications to the "open pursuit loop" feature in a CNTRLX trial to better reflect actual constraints
//             in its practical implementation.  User can only designate a single segment for opening the loop and the
//             affected target is always that selected as fix tgt #1.  There are other implementation constraints (eg,
//             supported targets are limited to CX_FIBER1/2, and a few XY scope target types), but we enforce these
//             at runtime, when we attempt to generate the trial codes defining a trial.
// 02mar2002-- Modifications to support a new stimulus paradigm, the pulse stimulus generator module (SGM).  The SGM
//             parameters are stored in the TRLHDR struct -- see cxobj_ifc.h.
// 07mar2002-- Added utility to compare the contents of current trial header with another header, IsSameHeader().
//          -- SetHeader() modified to also set a flag indicating whether or not the call actually changed TRLHDR.
// 07may2002-- Introducing "SetXxxx" methods for setting the individual parameters in a trial segment.  These methods
//             also implement two "global" modify modes:  propagating a parameter change across all segments, or across
//             "matching" segments (if we're changing P from P0 to P1 in segment S, then we also change P to P1 in all
//             other segments where the current value of P is P0).
// 20sep2002-- Minor mods to introduce a third sacc-trig'd operation, "SelectByFix2".
// 18oct2002-- Mods to introduce new features: separate H & V fixation accuracies, and mid-trial periodic rewards. Both
//             of these are per-segment features found in the segment header.
// 12dec2002-- Began mods to introduce a list of up to MAX_TRIALPERTS perturbations that participate in the trial.  For
//             each perturbation object in use, we maintain its object key, desired amplitude (perturbation objects are
//             defined with unit amplitude), the selected velocity component and target perturbed, and the segment
//             at which the perturbation begins.
//          -- Introduced versioning of trial object:  Version 1 has no pertubation list; Version 2 does.
// 20dec2002-- Began work on CCxTrial::Import()....
// 10apr2003-- Began major mod:  Moving support for "global" modification modes to CCxTrialForm.  We're introducing a
//             a fourth mode which propagates changes across all trials in a set, which is not isolated to a single
//             trial.  Decided instead to keep all details of the special modification modes in CCxTrialForm...
// 29sep2003-- Misc mods so that various multi-choice parameters "wrap-around" in both directions in response to right
//             clicks.  See also CCxTrialForm.
// 23oct2003-- Added support to separately enable velocity stabilization ("opening pursuit loop") for the horizontal
//             and vertical directions.  This will allow user to stabilize one component of fix tgt #1 during the open
//             loop segment while the other component follows its precomputed trajectory.  It was not necessary to
//             change the trial object's schema version, since the changes were designed so that preexisting trial
//             objects will be read in with both components stabilized.
//          -- Import() modified to allow users to create cxUNIX-style trial definition files that can enable velocity
//             stabilization on H only, V only, or both components.  We do this only so that users who write scripts to
//             auto-generate these trial files can take advantage of this new feature.
// 20sep2004-- Added support for two new trial header parameters, TRLHDR.iMarkSeg1 and .iMarkSeg2.  Schema version for
//             CCxTrial changed from 2 to 3.
// 03nov2004-- Added support for new saccade-triggered special op, "Dual Fix". Schema version unchanged.
// 25jan2005-- Special feature "Dual Fix" renamed "Switch Fix"...  Added trial header flag for specifying mid-trial
//             reward mode as "periodic" or "delivered at end of enabled segments".  Also added fields for mid-trial
//             reward interval (periodic mode only) and reward length in ms.  CCxTrial schema version incr'd to 4.
// 15mar2005-- Added CopyRemoteObj() override.
// 30mar2005-- Adding support for "intra-trial tagged sections".  Each such section is a contiguous range of trial
//             segments with an associated tag up to 8 chars long.  The intent is that each tagged section has a
//             specific purpose, such as presenting a rightward-moving dot patch.  A trial would typically contain
//             multiple tagged sections, and the section info will help analysis programs "parse" an individual trial
//             into these smaller pieces... Kind of like trials within a trial.  CCxTrial schema version incr'd to 5.
// 28jul2005-- Perturbations may now affect target window or pattern direction (PERT_ON_DWIN, _DPAT) in addition to
//             any of the velocity components.  Modified CCxTrial accordingly.
// 24oct2005-- Added support for access to alternate per-trial XY dot seed, which can override dot seed spec in display
//             settings: -1 => ignore.  0 => auto-generate seed.  >0 => fixed seed.  Allowed range is [-1 .. +max_int].
//             Schema version incr'd to 6.
// 29nov2005-- Minor change to recognize new special op flag THF_RPDISTRO.
// 01dec2005-- Mods to maintain a CCxRPDistro object as a non-serialized member of CCxTrial WHEN it uses the "R/P
//             Distro" special operation (ie, THF_RPDISTRO flag is set).
// 13mar2006-- Mods to permit velocity stabilization over a contiguous span of trial segments.  Added field 'nOpenSegs'
//             to TRLHDR.  Schema version incr'd to 7.
// 12apr2006-- Complete rework of velocity stabilization to support changes on a per-target, per-segment basis. Fields
//             TRLHDR.iOpenSeg and nOpenSegs OBSOLETE, but remain to ease migration of existing documents.  New flags
//             SGTJF_* and SGTJ_* constatns introduced.  CCxTrial schema version incr'd to 8.  CCxSegment modified.
// 19may2006-- Exposed new trial header flags THF_IGNPOSSCALE, etc.  These let user selectively ignore the global 
//             target position/velocity vector scale and rotate factors on a per-trial basis.  No need to increment 
//             schema version, since these flags will all be effectively unset (ie, DON'T ignore) in existing 
//             documents.
// 03jan2007-- Added support for two new "special operations", "choose fix tgt #1" and "choose fix tgt #2". Also 
//             revamped how the special operation is stored in TRLHDR -- instead of using flag bits, we added a new 
//             field, TRLHDR.iSpecialOp. See also CXOBJ_IFC.H. Schema version incr'd to 9.
// 27feb2007-- Mods to support per-segment pattern acceleration cmpts H & V. TRAJINFO modified. Introduced schema 
//             versioning of CCxSegment. CCxSegment schema version incr'd to 2.
// 24apr2007-- Updated to use renamed SGM constants from cxobj_ifc.h. Introduction of SGM_BIPHASICTRAIN op mode req'd 
//             incrementing schema version to 10 (it has the old value of SGM_NOOP).
// 16jul2007-- Added support for perturbing target window or pattern speed -- ie, velocity vector AMPLITUDE -- without 
//             changing the direction of the velocity vector: PERT_ON_SWIN, PERT_ON_SPAT. Effective Maestro v2.1.2. No 
//             schema version change required.
// 29aug2007-- Added support for perturbing the direction or speed of BOTH a target's window and its pattern: 
//             PERT_ON_DIR, PERT_ON_SPD. Effective Maestro v2.1.3. No schema version change required.
// 23sep2011-- Non-modifiable targets CX_FIBER* and CX_REDLED* are no longer supported as of Maestro v3.0. CCxTrial
//             updated accordingly.
// 05sep2013-- Segment min/max duration can now be a 5-digit positive integer instead of a 4-digit one, and the max
//             allowed value is 32000ms. This lets the user create a very long trial segment. Overall trial length is
//             still restricted to a value less than MAX(short), but that restriction is not enforced until the trial
//             codes are prepared by CCxTrialSequencer.
// 01sep2016-- Begun extensive mods to introduce "random variables" into a trial. The trial contains 10 RVs, "x0..x9",
// any subset of which may be defined for use (if RV type is RV_NOTUSED, that RV is undefined). The following params in
// the segment table may be assigned to an RV: segment duration (both min and max duration are assigned to the RV, 
// which then controls the actual segment duration), target window pos, vel, acc (both H & V), and target pattern vel
// and acc (both H & V). During trial sequencing, a new value is generated for each defined RV before the trial is
// presented, and that RV's value is used for every parameter to which the RV is assigned. Thus, any parameter assigned
// to an RV will be "randomly distributed" over the course of multiple presentations of the trial. Also, this means
// the value of any such parameter for editing purposes (a constant value OR an RV assignment) is different from its
// value when preparing a trial for presentation (the constant value OR the current value of the assigned RV).
// 20sep2016-- Removed GetSegment(), which provided const access to a trial segment. This was only used in the class
// CCxTrialSequencer, and we want to force it to use CCxTrial methods only, which will offer methods for getting the
// current value of any segment table parameter that can be assigned to a random variable -- as that value will change
// each time the trial is actually presented.
// 21sep2016-- Modified CCxSegment and CCxTrial to support assigning a trial random variable (by its zero-based index)
// to min/max segment duration and any floating-point target trajectory parameter. The ***SegParam() methods were 
// modified to support editing these "RV-assignable" parameters as multi-choice (when an RV index is assigned) or as a
// numeric parameter (when the parameter is set to a numeric constant). The assignment to an RV is stored in SEGHDR
// or TRAJINFO in a somewhat unconvential fashion, and the prototypes of the Get/Set***Duration() and Get/SetTgt***()
// methods have been adjusted to distinguish RV assignment from a numeric constant.... Also, to get the current value
// assigned to an RV-assignable parameter, use GetCurr***(), which returns the constant value or the current value of
// the assigned RV, as appropriate.
// 05sep2017-- Fix compiler issues while compiling for 64 - bit Win 10 using VStudio 2017. Note: Replaced ::sscanf()
// with more secure version ::sscanf_s(), which requires buffer sizes for each string output...
// 20sep2018-- Mods to support RMVideo sync flash enable flag in SEGHDR...
// 15may2019-- Mods to support random reward withholding variable ratio for both reward pulse 1 and reward pulse 2.
// The variable ratio is defined by a numerator N and denominator D, such that the reward is withheld in N randomly
// selected presentations of the trial out of every D presentations. CCxTrial schema version = 12. Effective as of
// Maestro 4.1.0.
// 20may2019-- Implemented runtime state for the random reward withholding feature -- rather than implement it in
// CCxTrialSequencer. Key methods are InitRewardWHVR() and UpdateRewardWHVR().
// 16oct2024-- The XYScope target platform, not supported since Maestro v4.0, is officially removed a/o V5.0. Removed
// XYScope-specific parameters. However, we must still support deserialization of pre-V5.0 documents containing trials
// that use XYScope targets. Such targets and trials are removed from the document AFTER deserialization! Segment
// header parameter 'SEGHDR.iXYFrame' is no longer serialized (CCxSegment schema version 5).
// 31oct2024-- Began implementation of new special feature "selDurByFix", in which target selection during the special
// segment selects the duration of the subsequent segment -- either the min or max duration. So that the user can
// specify DIFFERENT RVs to randomly choose the min and the max duration of that subsequent segment, CCxTrial no
// longer forces the min/max duration params to be assigned to the same RV. Now each can be independently assigned to
// a constant value or any RV. Of course, this usage only really makes sense for the "selDurByFix" feature.
// 19nov2024-- Dropped support for the PSGM, which was never actually put into use in experiment rigs (a prototype
// was tested, then abandoned. TRLHDR.iSGMSeg and .sgm no longer exist (see cxobj_ifc.h).
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "time.h"                            // for system time to generate random seed for trial RVs

#include "cntrlx.h"                          // CCntrlxApp -- the MAESTRO app object
#include "cxdoc.h"                           // CCxDoc -- the MAESTRO experiment document
#include "cxrpdistro.h"                      // CCxRPDistro -- stores runtime info for distr-based reward protocol
#include "cxtrial.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//=====================================================================================================================
// class CCxSegment
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxSegment, CObject, 5 | VERSIONABLE_SCHEMA )

//=== Copy ============================================================================================================
//
//    Copy the contents of the specified "source" segment to THIS segment object.
//
//    ARGS:       src   -- [in] the segment object to be copied.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, if unable to allocate additional traj records.  This will only happen when the
//                source segment contains more records than this segment.
//
VOID CCxSegment::Copy( const CCxSegment& src )
{
   ASSERT_VALID( &src );

   // # of target trajectory records currently in each segment
   int nSrcTraj = src.TrajCount(); 
   int nDstTraj = TrajCount(); 

   if ( nSrcTraj > nDstTraj )                               // append add'l traj records, init'd to default values
   {
      AllocTraj( nSrcTraj-nDstTraj );                       // ***** THROWS CMemoryException.  segment obj unchanged.
   }
   else for( int i = 0; i < nDstTraj-nSrcTraj; i++ )        // remove excess trajectory records
   {
      PTRAJINFO pTraj = m_trajRecs.RemoveHead();            // it doesn't matter which ones we remove, since all
      delete pTraj;                                         // remaining records will be set to values in src...
   }

   PTRAJINFO pDstTraj, pSrcTraj;                            // now copy trajectory data...
   POSITION srcPos = src.m_trajRecs.GetHeadPosition();
   POSITION dstPos = m_trajRecs.GetHeadPosition();
   while( srcPos != NULL )
   {
      pSrcTraj = src.m_trajRecs.GetNext( srcPos );
      pDstTraj = m_trajRecs.GetNext( dstPos );
      *pDstTraj = *pSrcTraj;
   }

   m_hdr = src.m_hdr;                                       // finally, copy header info
}


//=== AllocTraj =======================================================================================================
//
//    Allocate the specified number of target trajectory records and append to the segment's current trajectory record
//    list.  Each new record allocated is assigned default values.
//
//    ARGS:       nAdd  -- [in] the # of trajectory records to add.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (request exceeds max # of records allowed).
//
//    THROWS:     CMemoryException if unable to allocate a trajectory record.  Since it is possible that the memory
//                exception will occur after allocating a portion of the records, we catch the exception and restore
//                the trajectory record list to its original state before passing on the exception.
//
BOOL CCxSegment::AllocTraj( const int nAdd )
{
   int nTrajOld = TrajCount();
   if ( nTrajOld + nAdd > MAX_TRIALTARGS )            // too many target trajectories!!
      return( FALSE );

   int i;
   PTRAJINFO pTraj;
   try
   {
      for( i = 0; i < nAdd; i++ )                     // allocate trajectory records and append to segment's current
      {                                               // trajectory record list
         pTraj = NULL;
         pTraj = new TRAJINFO;                        // **** THROWS CMemoryException
         AssignDefaultTraj( pTraj );
         m_trajRecs.AddTail( pTraj );                 // **** THROWS CMemoryException
      }
   }
   catch( CMemoryException *e )
   {
      UNREFERENCED_PARAMETER(e);
      if ( pTraj != NULL )                            // this frees newly allocated record when AddTail() throws
         delete pTraj;
      while( m_trajRecs.GetCount() > nTrajOld )       // this frees any new records that were successfully appended
      {
         pTraj = m_trajRecs.RemoveTail();
         delete pTraj;
      }
      throw;                                          // pass on the exception!!
   }

   return( TRUE );
}


//=== InsertTraj ======================================================================================================
//
//    Insert a new target trajectory record at the specified position (zero-based index).  Existing records are shifted
//    down to make room, and the new record is initialized with default values.
//
//    Fixation tgts 1 and 2 are identified by their zero-based pos in the list of targets participating in a trial.
//    The act of inserting a target requires that the fixation target positions be adjusted.  We must do that here.
//
//    ARGS:       iPos     -- [in] zero-based insertion position; if invalid, append record.
//
//    RETURNS:    TRUE if successful; FALSE if trajectory record list is already full.
//
//    THROWS:     CMemoryException if unable to allocate the new trajectory record, or unable to insert it into the
//                trajectory record list.  In the latter case, we free the successfully allocated trajectory record
//                before passing on the exception to avoid memory leak!
//
BOOL CCxSegment::InsertTraj( const int iPos )
{
   int nCount = TrajCount();
   if ( nCount == MAX_TRIALTARGS )                             // trajectory record list is maxed out!
      return( FALSE );

   BOOL bAfter = FALSE;                                        // insert before record at specified pos, unless...
   int iNew = iPos;
   if ( nCount == 0 )                                          // ...traj list empty:  inserting first record
      iNew = 0;
   else if ( (iNew < 0) || (iNew >= nCount) )                  // ...invalid pos:  append by inserting after last rec
   {
      iNew = nCount-1;
      bAfter = TRUE;
   }

   PTRAJINFO pTraj = new TRAJINFO;                             // allocate the new trajectory record;
                                                               // **** THROWS CMemoryException upon failure...

   AssignDefaultTraj( pTraj );                                 // initialize w/ default values

   try                                                         // insert record ptr.  we must catch a memory excpt
   {                                                           // thrown during this process so that we can free the
                                                               // trajectory record just allocated....

      if ( nCount == 0 )                                       //    first record added
      {
         m_trajRecs.AddHead( pTraj );                          //    **** THROWS CMemoryException
      }
      else                                                     //    general insertion case:
      {
         POSITION pos = m_trajRecs.FindIndex( iNew );
         if ( bAfter )
            m_trajRecs.InsertAfter( pos, pTraj );              //    **** THROWS CMemoryException
         else
            m_trajRecs.InsertBefore( pos, pTraj );             //    **** THROWS CMemoryException
      }
   }
   catch( CMemoryException *e )                                // caught memory exception during ptr insertion; must
   {                                                           // release trajectory rec before passing on exception...
      UNREFERENCED_PARAMETER(e);
      delete pTraj;
      throw;
   }

   ASSERT( pTraj != NULL );                                    // if we get here, we must have been successful!
   if ( nCount > 0 )                                           // adjust pos of fixation targets as needed...
   {                                                           //    NOTE: if not in use, fixation target pos is
                                                               //    -1, so nothing will happen here!
      int iFix = m_hdr.iFixTarg1;
      if ( (iNew < iFix) || ((iNew == iFix) && (!bAfter)) )
         ++(m_hdr.iFixTarg1);

      iFix = m_hdr.iFixTarg2;
      if ( (iNew < iFix) || ((iNew == iFix) && (!bAfter)) )
         ++(m_hdr.iFixTarg2);
   }

   return( TRUE );
}


//=== RemoveTraj ======================================================================================================
//
//    Remove an existing target trajectory record at the specified position (zero-based index) in the segment's
//    trajectory list.
//
//    Fixation tgts 1 and 2 are identified by their zero-based pos in the list of targets participating in a trial.
//    The act of removing a target requires that the fixation target positions be adjusted.  We must do that here.  If
//    the target removed is a fixation target, then that fixation target is set to "none" (pos is negative).
//
//    ARGS:       iPos     -- [in] zero-based position of traj record to be removed.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos or list empty), in which case the trajectory record
//                list is unchanged.
//
BOOL CCxSegment::RemoveTraj( const int iPos )
{
   if ( (iPos < 0) || (iPos >= m_trajRecs.GetCount()) )     // invalid pos or list empty
      return( FALSE );

   POSITION pos = m_trajRecs.FindIndex( iPos );             // get ptr to record to be deleted
   PTRAJINFO pRec = m_trajRecs.GetAt( pos );
   m_trajRecs.RemoveAt( pos );                              // remove ptr from list
   delete pRec;                                             // free memory allocated to record

   if ( iPos < m_hdr.iFixTarg1 )                            // adjust pos of fixation targets as necessary...
      --(m_hdr.iFixTarg1);
   else if ( iPos == m_hdr.iFixTarg1 )
      m_hdr.iFixTarg1 = -1;
   if ( iPos < m_hdr.iFixTarg2 )
      --(m_hdr.iFixTarg2);
   else if ( iPos == m_hdr.iFixTarg2 )
      m_hdr.iFixTarg2 = -1;

   return( TRUE );
}


//=== RemoveAllTraj ===================================================================================================
//
//    Remove all existing target trajectory records from the segment.  Calling this method ensures that all memory
//    allocated by the segment object for target trajectory data is freed.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSegment::RemoveAllTraj()
{
   while( !m_trajRecs.IsEmpty() )
   {
      PTRAJINFO pTraj = m_trajRecs.RemoveHead();
      delete pTraj;
   }
}


//=== SetHeader =======================================================================================================
//
//    Modify the segment header.  Any invalid parameters are corrected.
//
//    ARGS:       hdr   -- [in] the new segment header
//                         [out] same, except that any invalid param values are corrected.
//
//    RETURNS:    TRUE if new header parameters were accepted w/o correction; FALSE if at least one param was changed.
//
BOOL CCxSegment::SetHeader( SEGHDR& hdr )
{
   BOOL bOk = TRUE;                                      // FALSE if we must correct any parameter in header

   // if either min or max dur is a negative integer N then, by design, segment duration is determined by a trial
   // random variable. N must lie in [-10 .. -1], and the index of the RV is I=abs(N)-1.
   if(hdr.iMinDur < 0)
   {
      if(hdr.iMinDur < -10) { hdr.iMinDur = -10; bOk = FALSE; }
   }
   if(hdr.iMaxDur < 0)
   {
      if(hdr.iMaxDur < -10) { hdr.iMaxDur = -10; bOk = FALSE; }
   }

   // if neither min nor max dur are assigned to an RV, then min dur cannot exceed max dur
   if(hdr.iMinDur > -1 && hdr.iMaxDur > -1 && hdr.iMinDur > hdr.iMaxDur) 
   {
      hdr.iMaxDur = hdr.iMinDur;
      bOk = FALSE;
   }

   if( hdr.iFixTarg1 >= TrajCount() )                    // if zero-based pos of fix target >= # of traj records in
   {                                                     // segment, then it's invalid!
      hdr.iFixTarg1 = -1;
      bOk = FALSE;
   }

   if( hdr.iFixTarg2 >= TrajCount() )                    // if zero-based pos of fix target >= # of traj records in
   {                                                     // segment, then it's invalid!
      hdr.iFixTarg2 = -1;
      bOk = FALSE;
   }

   if( hdr.fFixAccH < SGH_MINFIXACC )                    // fixation accuracy cannot be too small
   {
      hdr.fFixAccH = SGH_MINFIXACC;
      bOk = FALSE;
   }
   if( hdr.fFixAccV < SGH_MINFIXACC )
   {
      hdr.fFixAccV = SGH_MINFIXACC;
      bOk = FALSE;
   }

   // [deprecated] XY frame interval no longer used - XYScope platform removed in Maestro v5.0
   hdr.iXYFrame = SGH_MINXYFRAME;

   if( (hdr.iMarker < SGH_MINMARKER) ||                  // the marker pulse line designation has a limited range
       (hdr.iMarker > SGH_MAXMARKER) )
   {
      hdr.iMarker = SGH_NOMARKER;
      bOk = FALSE;
   }

   m_hdr = hdr;                                          // (possibly corrected) header becomes the current header
   return( bOk );
}


//=== GetTrajInfo =====================================================================================================
//
//    Retrieve the current state of a target's trajectory parameters for this segment.
//
//    ARGS:       iPos  -- [in] zero-based pos of the traj record to retrieve.  ASSERTs if invalid.
//                traj  -- [out] struct to hold copy of current parameter values in trajectory record
//
//    RETURNS:    NONE.
//
VOID CCxSegment::GetTrajInfo( int iPos, TRAJINFO& traj ) const
{
   POSITION pos = m_trajRecs.FindIndex( iPos );    // retrieve the specified target traj record
   ASSERT( pos != NULL );
   PTRAJINFO pTraj = m_trajRecs.GetAt( pos );
   traj = *pTraj;                                  // copy the traj parameters
}


//=== SetTrajInfo =====================================================================================================
//
//    Set the parameter values for a single trajectory record within the segment.  Any invalid params are corrected.
//
//    ARGS:       iPos  -- [in] zero-based pos of the traj record to be changed.  ASSERTs if invalid.
//                traj  -- [in] the new state of trajectory record.
//                         [out] same, except that any invalid param values are corrected.
//
//    RETURNS:    TRUE if new trajectory params were accepted w/o correction; FALSE if at least one param was changed.
//
BOOL CCxSegment::SetTrajInfo( int iPos, TRAJINFO& traj )
{
   POSITION pos = m_trajRecs.FindIndex( iPos );                         // retrieve the specified target traj record
   ASSERT( pos != NULL );
   PTRAJINFO pTraj = m_trajRecs.GetAt( pos );

   BOOL bOk = TRUE;                                                     // FALSE if we must correct any param in traj

   traj.fPosH = LimitTraj( traj.fPosH, SGTJ_POSMAX, bOk );              // enforce range restrictions on some params...
   traj.fPosV = LimitTraj( traj.fPosV, SGTJ_POSMAX, bOk );
   traj.fVelH = LimitTraj( traj.fVelH, SGTJ_VELMAX, bOk );
   traj.fVelV = LimitTraj( traj.fVelV, SGTJ_VELMAX, bOk );
   traj.fAccH = LimitTraj( traj.fAccH, SGTJ_ACCMAX, bOk );
   traj.fAccV = LimitTraj( traj.fAccV, SGTJ_ACCMAX, bOk );
   traj.fPatVelH = LimitTraj( traj.fPatVelH, SGTJ_VELMAX, bOk );
   traj.fPatVelV = LimitTraj( traj.fPatVelV, SGTJ_VELMAX, bOk );
   traj.fPatAccH = LimitTraj( traj.fPatAccH, SGTJ_ACCMAX, bOk );
   traj.fPatAccV = LimitTraj( traj.fPatAccV, SGTJ_ACCMAX, bOk );

   *pTraj = traj;
   return( bOk );
}


//=== Serialize [base override] =======================================================================================
//
//    Handles reading/writing the trial segment info from/to a disk file via a serialization archive.  Note that,
//    during deserialization, any invalid parameters are auto-corrected as they are stored into the segment object.
//
//    Version Control (using MFC "schema"):
//       1: Base version (thru Maestro v2.0.5)
//       2: Support for pattern acceleration H,V added. TRAJINFO modified to included fields fPatAccH, fPatAccV. These 
//          default to 0 when deserializing previous version.
//       3: Added support for assigning trial random variable to segment duration and trajectory parameters. No
//          fields added to SEGHDR or TRAJINFO, but usage is different.
//       4: Added support for RMVideo sync flash at segment start: new field BOOL SEGHDR.bEnaRMVSync (Maestro v4.0.0).
//       5: Header parameter 'iXYFrame' is depecated. While still part of the SEGHDR struct, it is no longer used nor
//          serialized.
//
//    ARGS:       ar -- [in] the serialization archive.
//
//    RETURNS:    NONE.
//
//    THROWS:     -- The archive may throw CMemoryException, CArchiveException, or CFileException.
//                -- This method itself may throw CMemoryException during deserialization, if unable to allocate the
//                   required # of target trajectory records.
//                -- If any exception occurs AFTER allocating trajectory records, we free all of the records and
//                   forward the exception.
//
void CCxSegment::Serialize ( CArchive& ar )
{
   UINT nSchema = ar.GetObjectSchema();                        // retrieve schema#
   CObject::Serialize( ar );

   int nTraj;
   BYTE ub;
   if ( ar.IsStoring() )                                       // BEGIN:  STORING TO ARCHIVE...
   {
      ar << m_hdr.iMinDur << m_hdr.iMaxDur;                    // (1) the segment header....
      ar << m_hdr.iFixTarg1 << m_hdr.iFixTarg2;
      ar << m_hdr.fFixAccH << m_hdr.fFixAccV;
      ar << m_hdr.iGrace << m_hdr.iMarker;
      ub = (BYTE) ((m_hdr.bChkResp) ? 1 : 0);                  // (no 'BOOL' overload for CArchive::operator<< )
      ar << ub;
      ub = (BYTE) ((m_hdr.bEnaRew) ? 1 : 0);
      ar << ub;
      ub = (BYTE) ((m_hdr.bEnaRMVSync) ? 1 : 0);
      ar << ub;

      nTraj = TrajCount();                                     // (2) # of target trajectories in segment
      ar << nTraj;

      POSITION pos = m_trajRecs.GetHeadPosition();             // (3) the target trajectories themselves, in order from
      while( pos != NULL )                                     //     head to tail in the trajectory list...
      {
         PTRAJINFO pTraj = m_trajRecs.GetNext( pos );

         ar << pTraj->dwFlags;
         ar << pTraj->fPosH << pTraj->fPosV;
         ar << pTraj->fVelH << pTraj->fVelV;
         ar << pTraj->fAccH << pTraj->fAccV;
         ar << pTraj->fPatVelH << pTraj->fPatVelV;
         ar << pTraj->fPatAccH << pTraj->fPatAccV;
      }
   }
   else                                                        // BEGIN:  READING FROM ARCHIVE...
   {
      if( nSchema < 1 || nSchema > 5 )                         // unsupported version
         ::AfxThrowArchiveException( CArchiveException::badSchema );
      
      ASSERT( m_trajRecs.GetCount() == 0 );                    // always deserialize to an initially empty segment!

      SEGHDR hdr;                                              // (1) the segment header -- first read params from file
      ar >> hdr.iMinDur >> hdr.iMaxDur;                        //     into a dummy header....
      ar >> hdr.iFixTarg1 >> hdr.iFixTarg2;
      ar >> hdr.fFixAccH >> hdr.fFixAccV;
      ar >> hdr.iGrace;
      
      // v=5: XYScope frame interval ignored - XYScope targets deprecated
      if(nSchema < 5)
         ar >> hdr.iXYFrame;
      else
         hdr.iXYFrame = 0;
      
      ar >> hdr.iMarker;
      ar >> ub;
      hdr.bChkResp = (ub != 0) ? TRUE : FALSE;
      ar >> ub;
      hdr.bEnaRew = (ub != 0) ? TRUE : FALSE;

      // v=4: RMVideo sync flash enable added
      if(nSchema < 4)
         hdr.bEnaRMVSync = FALSE;
      else
      {
         ar >> ub;
         hdr.bEnaRMVSync = (ub != 0) ? TRUE : FALSE;
      }

      ar >> nTraj;                                             // (2) # of target trajectories in segment -- attempt to
      ASSERT( nTraj <= MAX_TRIALTARGS );                       //     allocate memory for this many traj records
      AllocTraj( nTraj );                                      // **** THROWS CMemoryException

      SetHeader( hdr );                                        // once we know # of targets partic., we can actually
                                                               // set & auto-correct the header parameters!

      try
      {
         int i = 0;                                            // (3) the target trajectories themselves, in order from
         TRAJINFO traj;                                        //     head to tail in the trajectory list. -- each traj
         while( i < nTraj )                                    //     is read into a dummy record, then transferred to
         {                                                     //     the actual record w/ auto-correct....
            ar >> traj.dwFlags;
            ar >> traj.fPosH >> traj.fPosV;
            ar >> traj.fVelH >> traj.fVelV;
            ar >> traj.fAccH >> traj.fAccV;
            ar >> traj.fPatVelH >> traj.fPatVelV;

            if(nSchema < 2)                                    // v=2 : pattern acc H,V were introduced.
            {
               traj.fPatAccH = 0;
               traj.fPatAccV = 0;
            }
            else
               ar >> traj.fPatAccH >> traj.fPatAccV;

            // v=3 : introduced new flag bits in order to assign RVs to selected tgt traj params. Ensure those bits
            // are cleared when serializing a previous version
            if(nSchema < 3) traj.dwFlags &= ~SGTJF_ISRVMASK;

            SetTrajInfo( i, traj );
            ++i;
         }
      }
      catch( CException* e )                                   //    if excpt occurs while reading in target traj data,
      {                                                        //    we catch it so that we can remove all traj records
         UNREFERENCED_PARAMETER(e);                            //    before forwarding excpt
         RemoveAllTraj();
         throw;
      }
   }

   ASSERT_VALID( this );                                       // check validity AFTER serializing
}



//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================

/**
 Set the minimum or maximum segment duration. 

 These parameters may be assigned to one of the trial's random variables, or to an integer. There are 10 trial RVs 
 available, with indices 0-9. To assign the RV with index N, call either method with iVal = -N-1; in this usage, iVal 
 is restricted to [-10 .. -1].

 If iVal >= 0, then the minimum or maximum segment duration is set to the constant specified. In this usage, it is
 restricted to [0..32000]. In addition, if both minDur and maxDur are NOT assigned to RVs and the change is such that 
 minDur > maxDur, then the other endpoint is auto-corrected to always ensure minDur <= maxDur.

 NOTE: Prior to Maestro 5.0.1, these methods forced both min and max duration to be set to the same RV, and that RV
 was used to randomly select the segment duration. However, in the "selDurByFix" special feature -- introduced in 
 5.0.1 -- the target selection during the special segment determines which of two possible durations to use for the
 subsequent segment -- the min duration or the max duration. To allow for those two durations to be governed by
 different distributions, we no longer require min/max duration to be assigned to the same RV. This also means that
 one duration could be set to an integer while the other is set to  an RV!

 @param iVal The new minimum or maximum duration value. See notes above on limits and RVs.
 @return TRUE if change was accepted without auto-correction; else FALSE.
*/
BOOL CCxSegment::SetMinDuration( int iVal )
{
   BOOL bSideEffect = FALSE;
   m_hdr.iMinDur = (iVal < -10) ? -10 : (iVal > 32000 ? 32000 : iVal);
   if(m_hdr.iMinDur > -1 && m_hdr.iMaxDur > -1 && m_hdr.iMinDur > m_hdr.iMaxDur)
   {
      m_hdr.iMaxDur = m_hdr.iMinDur;
      bSideEffect = TRUE;
   }
   return(BOOL(m_hdr.iMinDur == iVal && !bSideEffect));
}

BOOL CCxSegment::SetMaxDuration( int iVal )
{
   BOOL bSideEffect = FALSE;
   m_hdr.iMaxDur = (iVal < -10) ? -10 : (iVal > 32000 ? 32000 : iVal);
   if(m_hdr.iMinDur > -1 && m_hdr.iMaxDur > -1 && m_hdr.iMinDur > m_hdr.iMaxDur)
   {
      m_hdr.iMinDur = m_hdr.iMaxDur;
      bSideEffect = TRUE;
   }
   return(BOOL(m_hdr.iMaxDur == iVal && !bSideEffect));
}

//=== SetXxxx =========================================================================================================
//
//    Set the value of a parameter in the segment header or a specified target trajectory record.
//
//    Any invalid value is auto-corrected.  
//
//    ARGS:       iTg   -- [in] index of target.  returns FALSE if invalid.
//                *Val  -- [in] the new value for the parameter (data type varies).
//
//    RETURNS:    TRUE if change was accepted without auto-correction or side effect; else FALSE.
//

// out-of-bounds values are auto-corrected to wrap forwards or backwards
BOOL CCxSegment::SetFixTarg1Pos( int iVal )
{
   m_hdr.iFixTarg1 = (iVal < -1) ? TrajCount()-1 : ((iVal >= TrajCount()) ? -1 : iVal);
   return( BOOL(m_hdr.iFixTarg1 == iVal) );
}

// out-of-bounds values are auto-corrected to wrap forwards or backwards
BOOL CCxSegment::SetFixTarg2Pos( int iVal )
{
   m_hdr.iFixTarg2 = (iVal < -1) ? TrajCount()-1 : ((iVal >= TrajCount()) ? -1 : iVal);
   return( BOOL(m_hdr.iFixTarg2 == iVal) );
}

BOOL CCxSegment::SetFixAccH( double dVal )
{
   float fVal = float(dVal);
   m_hdr.fFixAccH = (fVal < SGH_MINFIXACC) ? SGH_MINFIXACC : fVal;
   return( BOOL(m_hdr.fFixAccH == fVal) );
}

BOOL CCxSegment::SetFixAccV( double dVal )
{
   float fVal = float(dVal);
   m_hdr.fFixAccV = (fVal < SGH_MINFIXACC) ? SGH_MINFIXACC : fVal;
   return( BOOL(m_hdr.fFixAccV == fVal) );
}

BOOL CCxSegment::SetGracePeriod( int iVal )
{
   m_hdr.iGrace = (iVal < 0) ? 0 : iVal;
   return( BOOL(m_hdr.iGrace == iVal) );
}

BOOL CCxSegment::SetMidTrialRewEnable( BOOL bVal )
{
   m_hdr.bEnaRew = bVal;
   return( TRUE );
}

// out-of-bounds values are auto-corrected to wrap forwards or backwards
BOOL CCxSegment::SetMarker( int iVal )
{
   m_hdr.iMarker = (iVal < SGH_NOMARKER) ? SGH_MAXMARKER : ((iVal > SGH_MAXMARKER) ? SGH_NOMARKER : iVal);
   return( BOOL(m_hdr.iMarker == iVal) );
}

BOOL CCxSegment::SetResponseChecked( BOOL bVal )
{
   m_hdr.bChkResp = bVal;
   return( TRUE );
}

BOOL CCxSegment::SetRMVSyncFlashOn(BOOL bOn)
{
   m_hdr.bEnaRMVSync = bOn;
   return(TRUE);
}

BOOL CCxSegment::SetTgtOn( const int iTg, BOOL bVal )
{
   if( !IsValidTraj( iTg ) ) return( FALSE );
   PTRAJINFO pTraj = GetTraj( iTg );
   if( bVal ) pTraj->dwFlags |= SGTJF_ON;
   else pTraj->dwFlags &= ~SGTJF_ON;
   return( TRUE );
}

BOOL CCxSegment::SetAbsolutePos( const int iTg, BOOL bVal )
{
   if( !IsValidTraj( iTg ) ) return( FALSE );
   PTRAJINFO pTraj = GetTraj( iTg );
   if( bVal ) pTraj->dwFlags |= SGTJF_ABS;
   else pTraj->dwFlags &= ~SGTJF_ABS;
   return( TRUE );
}

// out-of-bounds values are auto-corrected to wrap forwards or backwards
BOOL CCxSegment::SetTgtVStabMode( const int iTg, int iMode )
{
   if( !IsValidTraj( iTg ) ) return( FALSE );
   int iCorrMode = (iMode < SGTJ_VSTABOFF) ? SGTJ_VSTABVONLY : ((iMode > SGTJ_VSTABVONLY) ? SGTJ_VSTABOFF : iMode);
   PTRAJINFO pTraj = GetTraj( iTg );
   pTraj->dwFlags &= ~SGTJF_VSTABMODE;
   pTraj->dwFlags |= VSTABMODE_TO_FLAGS(iCorrMode);
   return( BOOL(iCorrMode==iMode) );
}

BOOL CCxSegment::SetTgtVStabSnapToEye( const int iTg, BOOL bVal )
{
   if( !IsValidTraj( iTg ) ) return( FALSE );
   PTRAJINFO pTraj = GetTraj( iTg );
   if( bVal ) pTraj->dwFlags |= SGTJF_VSTABSNAP;
   else pTraj->dwFlags &= ~SGTJF_VSTABSNAP;
   return( TRUE );
}

/**
 Get the value of the target trajectory parameter specified for the purpose of displaying and editing that value. Since
 these trajectory parameters are random variable-assignable, their "value" may be either a numeric floating-point 
 constant or the zero-based integer index of the currently assigned random variable.

 @param t [in] Target index.
 @param p [in] Parameter index, using enumerated values in CCxTrial::ParamID: [TGTHPOS..PATACCV].
 @param isRV [out] This is set to TRUE if a trial RV is currently assigned to the parameter; else FALSE.
 @return The parameter value. If isRV==TRUE, cast this to an integer to get the zero-based index of the trial RV
 currently assigned to the parameter. If isRV==FALSE, this is the current numeric value of the parameter. If the
 arguments do not identify a valid target trajectory parameter, method returns 0 and isRV is set to FALSE.
*/
double CCxSegment::GetTgtTrajParam(int t, int p, BOOL& isRV) const
{
   isRV = FALSE;
   if(!(IsValidTraj(t) && p>=CCxTrial::TGTHPOS && p<=CCxTrial::PATVACC)) return(0.0);

   PTRAJINFO pTraj = GetTraj(t);
   DWORD rvFlagBit = (DWORD) (int(SGTJF_POSH_ISRV) << (p-CCxTrial::TGTHPOS));
   isRV = BOOL((pTraj->dwFlags & rvFlagBit) != 0);
   double dVal = 0;
   switch(p)
   {
   case CCxTrial::TGTHPOS : dVal = pTraj->fPosH; break;
   case CCxTrial::TGTVPOS : dVal = pTraj->fPosV; break;
   case CCxTrial::TGTHVEL : dVal = pTraj->fVelH; break;
   case CCxTrial::TGTVVEL : dVal = pTraj->fVelV; break;
   case CCxTrial::TGTHACC : dVal = pTraj->fAccH; break;
   case CCxTrial::TGTVACC : dVal = pTraj->fAccV; break;
   case CCxTrial::PATHVEL : dVal = pTraj->fPatVelH; break;
   case CCxTrial::PATVVEL : dVal = pTraj->fPatVelV; break;
   case CCxTrial::PATHACC : dVal = pTraj->fPatAccH; break;
   case CCxTrial::PATVACC : dVal = pTraj->fPatAccV; break;
   }

   return(dVal);
}

/**
 Set the value of the target trajectory parameter specified. Since these trajectory parameters are random variable-
 assignable, their "value" may be either a numeric floating-point constant or the zero-based integer index of the 
 currently assigned random variable.

 @param t [in] Target index.
 @param p [in] Parameter index, using enumerated values in CCxTrial::ParamID: [TGTHPOS..PATACCV]
 @param dVal [in] The new parameter value.
 @param isRV [in] If isRV==TRUE, dVal is cast to an integer and interpreted as the zero-based index of the trial RV to
 be assigned to the parameter. If the index is invalid, the parameter is set to a numeric constant of 0 (NOT the RV at
 index 0). If isRV==FALSE, dVal is the new numeric constant assigned to the parameter.
 @return TRUE if the new parameter value was accepted without correction; FALSE if it was auto-corrected. Also returns
 FALSE if arguments do not identify a valid target trajectory parameter in [TGTHPOS..PATACCV].
*/
BOOL CCxSegment::SetTgtTrajParam(int t, int p, double dVal, BOOL isRV)
{
   if(!(IsValidTraj(t) && p>=CCxTrial::TGTHPOS && p<=CCxTrial::PATVACC)) return(FALSE);
   BOOL bUncorr = TRUE;
   
   // if assigning an RV to the parameter, validate the index provided. If it is invalid, then the parameter is
   // set to a numeric constant of 0 instead.
   if(isRV)
   {
      int idx = (dVal < 0.0) ? int(dVal - 0.5) : int(dVal + 0.5);
      if(idx < 0 || idx >= MAX_TRIALRVS)
      {
         isRV = FALSE;
         dVal = 0.0;
         bUncorr = FALSE;
      }
      else dVal = double(idx);
   }

   PTRAJINFO pTraj = GetTraj(t);

   DWORD rvFlagBit = (DWORD) (int(SGTJF_POSH_ISRV) << (p-CCxTrial::TGTHPOS));
   if(isRV) pTraj->dwFlags |= rvFlagBit;
   else pTraj->dwFlags &= ~rvFlagBit;

   switch(p)
   {
   case CCxTrial::TGTHPOS : pTraj->fPosH = LimitTraj(float(dVal), SGTJ_POSMAX, bUncorr); break;
   case CCxTrial::TGTVPOS : pTraj->fPosV = LimitTraj(float(dVal), SGTJ_POSMAX, bUncorr); break;
   case CCxTrial::TGTHVEL : pTraj->fVelH = LimitTraj(float(dVal), SGTJ_VELMAX, bUncorr); break;
   case CCxTrial::TGTVVEL : pTraj->fVelV = LimitTraj(float(dVal), SGTJ_VELMAX, bUncorr); break;
   case CCxTrial::TGTHACC : pTraj->fAccH = LimitTraj(float(dVal), SGTJ_ACCMAX, bUncorr); break;
   case CCxTrial::TGTVACC : pTraj->fAccV = LimitTraj(float(dVal), SGTJ_ACCMAX, bUncorr); break;
   case CCxTrial::PATHVEL : pTraj->fPatVelH = LimitTraj(float(dVal), SGTJ_VELMAX, bUncorr); break;
   case CCxTrial::PATVVEL : pTraj->fPatVelV = LimitTraj(float(dVal), SGTJ_VELMAX, bUncorr); break;
   case CCxTrial::PATHACC : pTraj->fPatAccH = LimitTraj(float(dVal), SGTJ_ACCMAX, bUncorr); break;
   case CCxTrial::PATVACC : pTraj->fPatAccV = LimitTraj(float(dVal), SGTJ_ACCMAX, bUncorr); break;
   }

   return(bUncorr);
}


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump the trial segment info in an easy-to-read form to the supplied dump context.  Intelligent dump is
//    tailored to the specific contents of the segment.  Must specify a dump depth > 1 to dump each target trajectory
//    record in the segment.  Otherwise, only segment header parameters are dumped.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxSegment::Dump( CDumpContext& dc ) const
{
   ASSERT_VALID( this );                                                   // validate the segment object
   CObject::Dump( dc );                                                    // dump base class stuff first
   CString msg;

   msg.Format("Min/max dur = [%d, %d]\n", m_hdr.iMinDur, m_hdr.iMaxDur);   // dump segment header parameters....
   dc << msg;

   msg.Format( "Fix 1 & 2; accH,V (deg); grace (ms); rewEna; rmvSyncEna = [%d, %d; %.2f,%.2f; %d; %d; %d]\n",
      m_hdr.iFixTarg1, m_hdr.iFixTarg2, m_hdr.fFixAccH, m_hdr.fFixAccV, m_hdr.iGrace, int(m_hdr.bEnaRew), 
      int(m_hdr.bEnaRMVSync) );
   dc << msg;

   if( m_hdr.iMarker == SGH_NOMARKER )
      dc << "No marker pulse for this segment.\n";
   else
      dc << "Marker pulse on DOUT" << m_hdr.iMarker << ".\n";

   if( m_hdr.bChkResp )
      dc << "Response is checked during this segment (when part of staircase sequence).\n";

   msg.Format( "Has %d target trajectories...\n", TrajCount() );
   dc << msg;

   if ( dc.GetDepth() <= 0 ) return;                                       // if shallow dump, we're done

   POSITION pos = m_trajRecs.GetHeadPosition();                            // else, dump each target's traj record...
   int i = 0;
   while( pos != NULL )
   {
      PTRAJINFO pTraj = m_trajRecs.GetNext( pos );
      ++i;

      msg.Format( "Trajectory %d:  Flags = 0x%02x\n", i, pTraj->dwFlags );
      dc << msg;

      msg.Format( "Window pos, vel, acc: (%.2f, %.2f), (%.2f, %.2f), (%.2f, %.2f)\n",
         pTraj->fPosH, pTraj->fPosV, pTraj->fVelH, pTraj->fVelV, pTraj->fAccH, pTraj->fAccV );
      dc << msg;

      msg.Format( "Pattern vel, acc: (%.2f, %.2f), (%.2f, %.2f)\n", 
         pTraj->fPatVelH, pTraj->fPatVelV, pTraj->fPatAccH, pTraj->fPatAccV );
      dc << msg;
   }
}


//=== AssertValid [base override] =====================================================================================
//
//    Validate the segment object.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
void CCxSegment::AssertValid() const
{
   CObject::AssertValid();
   m_trajRecs.AssertValid();
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== AssignDefaultHeader =============================================================================================
//
//    Assign default values to the segment header.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSegment::AssignDefaultHeader()
{
   m_hdr.iMinDur = 1000;            // min/max dur in msec
   m_hdr.iMaxDur = 1000;            //
   m_hdr.iFixTarg1 = -1;            // no fixation targets assigned, so no fixation checking will occur in seg
   m_hdr.iFixTarg2 = -1;            //
   m_hdr.fFixAccH = 5.0f;           // H,V fixation accuracies in deg (default gives us a large fixation window)
   m_hdr.fFixAccV = 5.0f;
   m_hdr.iGrace = 50;               // fixation grace period in msec
   m_hdr.bEnaRew = FALSE;           // mid-trial rewards disabled
   m_hdr.iXYFrame = SGH_MINXYFRAME; // [deprecated] update interval for XY scope targets ONLY (msec)
   m_hdr.iMarker = SGH_NOMARKER;    // no marker pulse delivered at start of segment
   m_hdr.bChkResp = FALSE;          // subject's response not checked during this segment (when part of staircase seq)
   m_hdr.bEnaRMVSync = FALSE;       // RMVideo sync flash not enabled
}


//=== AssignDefaultTraj ===============================================================================================
//
//    Assign default values to a target trajectory record.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSegment::AssignDefaultTraj( PTRAJINFO pTraj )
{
   ASSERT( AfxIsValidAddress( pTraj, sizeof(TRAJINFO) ) );

   pTraj->dwFlags = SGTJF_ON;                                  // tgt ON, relative pos, velocity stabilization disabled
   pTraj->fPosH = pTraj->fPosV = (float)0.0;                   // target window pos at seg start (deg)
   pTraj->fVelH = pTraj->fVelV = (float)0.0;                   // target window velocity during seg (deg/sec)
   pTraj->fAccH = pTraj->fAccV = (float)0.0;                   // target window acceleration during seg (deg/sec^2)
   pTraj->fPatVelH = pTraj->fPatVelV = (float)0.0;             // target pattern velocity during seg (deg/sec)
   pTraj->fPatAccH = pTraj->fPatAccV = (float)0.0;             // target pattern acceleration during seg (deg/sec^2)
}


//=== LimitTraj ========================================================================================================
//
//    Limit range a floating-point trajectory parameter to +/- the provided limit.
//
//    ARGS:       fVal   -- [in] the proposed value.
//                fLim   -- [in] allowed range is +/- this limiting value.
//                bFlag  -- [out] unchanged if proposed value OK; set FALSE if value had to be range-limited.
//
//    RETURNS:    proposed value, range-limited if necessary.
//
FLOAT CCxSegment::LimitTraj( const float fVal, const float fLim,  BOOL& bFlag ) const
{
   float f = (fVal < 0.0f) ? -fVal : fVal;
   float fMax = (fLim < 0.0f) ? -fLim : fLim;
   if ( f > fMax )
   {
      f = (fVal < 0.0f) ? -fMax : fMax;
      bFlag = FALSE;
   }
   else
      f = fVal;

   return( f );
}

//
//=====================================================================================================================
// END:  class CCxSegment
//=====================================================================================================================




//=====================================================================================================================
// class CCxTrial
//=====================================================================================================================
//
IMPLEMENT_SERIAL( CCxTrial, CTreeObj, 14 | VERSIONABLE_SCHEMA )


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== Initialize [base override] ======================================================================================
//
//    Initialize trial object after default construction.
//
//    This method MUST be called directly after default construction to initialize the newly constructed trial with the
//    specified name, CNTRLX object type, and state flags.
//
//    If invoked on a trial object that has already been initialized, the trial is cleared and reinitialized.
//
//    ARGS:       s  -- [in] the name assigned to trial object
//                t  -- [in] the CNTRLX object data type -- MUST be CX_TRIAL, the only recognized trial data type
//                f  -- [in] the object's initial state flags -- CANNOT include CX_ISSETOBJ.
//
//    RETURNS:    NONE.
//
VOID CCxTrial::Initialize( LPCTSTR s, const WORD t, const WORD f )
{
   ASSERT( t == CX_TRIAL );                        // validate trial object type and flags
   ASSERT( (f & CX_ISSETOBJ) == 0 );

   Clear();

   CTreeObj::Initialize( s, t, f );                // base class inits
}


//=== Copy [base override] ============================================================================================
//
//    Assign members of source trial object to THIS trial object (already constructed).  We do not require that both
//    src & dst trials have the same # of segments or participating targets, so this operation could change the memory
//    requirements of THIS trial object -- either requiring allocation of additional segment objects or deallocation of
//    extra segments.
//
//    For simplicity, we clear the trial completely and start from scratch; the danger in doing so is that, if we have
//    a problem allocating memory as we make THIS trial identical to the source trial, we cannot restore the trial to
//    its previous state!
//
//    If the source trial uses the THF_RPDISTRO special operation, we do NOT copy its CCxRPDistro member to this trial,
//    since that object only contains transient runtime information and is not part of a trial's intrinsic definition.
//
//    ARGS:       pSrc  -- [in] CTreeObj* ptr to the trial to be copied.  MUST point to a valid CCxTrial object!
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to allocate memory for segments, tagged section records, or participating
//                target keys.  Trial will NOT be in its original state if such an exception should occur.  Caller
//                should catch exception and delete this object!
//
VOID CCxTrial::Copy( const CTreeObj* pSrc )
{
   ASSERT( pSrc->IsKindOf( RUNTIME_CLASS( CCxTrial ) ) );         // source object MUST be a CCxTrial obj!

   const CCxTrial* pSrcTrial = (CCxTrial*) pSrc;                  // validate source trial object
   ASSERT_VALID( pSrcTrial );

   Clear();                                                       // start over!
   CTreeObj::Copy( pSrc );                                        // copy the base class stuff

   m_hdr = pSrcTrial->m_hdr;                                      // copy header
   m_nPerts = pSrcTrial->m_nPerts;                                // copy perturbation list
   for( int i = 0; i < m_nPerts; i++ )
      m_Perts[i] = pSrcTrial->m_Perts[i];

   // copy random variable list
   for(int i=0; i<MAX_TRIALRVS; i++)
   {
      m_Vars[i] = pSrcTrial->m_Vars[i];
   }

   m_wArTargs.Copy( pSrcTrial->m_wArTargs );                      // copy array of participating targs
                                                                  // **** THROWS CMemoryException

   POSITION pos = pSrcTrial->m_Segments.GetHeadPosition();        // copy segments...
   while( pos != NULL )
   {
      CCxSegment* pSrcSeg = pSrcTrial->m_Segments.GetNext( pos ); // current seg to be copied; advance to next seg.
      CCxSegment* pSeg = new CCxSegment;                          // create default, empty segment object
      ASSERT( pSeg != NULL );                                     // **** THROWS CMemoryException

      try
      {
         pSeg->Copy( *pSrcSeg );                                  // copy current src seg to the new empty segment obj
         m_Segments.AddTail( pSeg );                              // and append it to our segment list.
      }
      catch( CMemoryException* e )                                // if memory exception, must not leave the new seg
      {                                                           // dangling!  free it and pass on the exception.
         UNREFERENCED_PARAMETER(e);
         delete pSeg;
         throw;
      }
   }

   pos = pSrcTrial->m_TaggedSections.GetHeadPosition();           // copy tagged sections...
   while( pos != NULL )
   {
      PTRIALSECT pSrcSect = pSrcTrial->m_TaggedSections.GetNext(pos);
      PTRIALSECT pSect = new TRIALSECT;
      ASSERT( pSect != NULL );

      try
      {
         *pSect = *pSrcSect;
         m_TaggedSections.AddTail( pSect );
      }
      catch( CMemoryException *e )
      {
         UNREFERENCED_PARAMETER(e);
         delete pSect;
         throw;
      }
   }
}


//=== CopyRemoteObj [base override] ===================================================================================
//
//    Copies the CCxTrial-specific definition of a trial located in a different experiment document.
//
//    CopyRemoteObject was introduced to the CTreeObj/CTreeMap framework to overcome the problem of copying an object
//    from one treemap to another.  It is intended only for copying the internal information specific to a given
//    implementation of CTreeObj.
//
//    ARGS:       pSrc        -- [in] the object to be copied.  Must be an instance of CCxTrial.
//                depKeyMap   -- [in] maps keys of any objects upon which the source obj depends, which reside in the
//                               source doc, to the keys of the corresponding objects in the destination doc.
//
//    RETURNS:    TRUE if successful, FALSE if source object is not an instance of CCxTrial, or if the supplied
//                dependency key map is missing an entry for any dependencies in the source trial.
//
BOOL CCxTrial::CopyRemoteObj(CTreeObj* pSrc, const CWordToWordMap& depKeyMap)
{
   if( pSrc == NULL || !pSrc->IsKindOf( RUNTIME_CLASS(CCxTrial) ) ) return( FALSE );

   const CCxTrial* pSrcTrial = (CCxTrial*) pSrc;                     // validate source trial object
   ASSERT_VALID( pSrcTrial );

   int i;
   WORD dstKey;

   CWordArray deps;                                                  // make sure dependency key map has an entry for
   pSrcTrial->GetDependencies( deps );                               // every obj upon which the source trial depends!
   for( i=0; i<deps.GetSize(); i++ )
   {
      if( !depKeyMap.Lookup( deps[i], dstKey ) )
         return( FALSE );
   }

   Clear();                                                          // start with an empty trial defn

   m_hdr = pSrcTrial->m_hdr;                                         // copy header, and replace src doc key of channel
   if( pSrcTrial->m_hdr.wChanKey != CX_NULLOBJ_KEY )                 // cfg, if any, w/key obtained from dependency map
      depKeyMap.Lookup(pSrcTrial->m_hdr.wChanKey, m_hdr.wChanKey);

   m_nPerts = pSrcTrial->m_nPerts;                                   // copy perturbation list, replacing src doc keys
   for( i = 0; i < m_nPerts; i++ )                                   // of each defined perturbation object w/keys
   {                                                                 // obtained from dependency map
      m_Perts[i] = pSrcTrial->m_Perts[i];
      if( pSrcTrial->m_Perts[i].wKey != CX_NULLOBJ_KEY )
         depKeyMap.Lookup(pSrcTrial->m_Perts[i].wKey, m_Perts[i].wKey);
   }

   // copy random variable list
   for(int i=0; i<MAX_TRIALRVS; i++)
   {
      m_Vars[i] = pSrcTrial->m_Vars[i];
   }

   for( i = 0; i < pSrcTrial->m_wArTargs.GetSize(); i++ )            // copy array of participating targs, replacing
   {                                                                 // src doc keys with keys from dependency map
      depKeyMap.Lookup(pSrcTrial->m_wArTargs[i], dstKey);
      try { m_wArTargs.Add(dstKey); }
      catch( CMemoryException *e )                                   // if we run out of memory, empty trial defn
      {                                                              // and abort
         e->Delete();
         Clear();
         return( FALSE );
      }
   }

   POSITION pos = pSrcTrial->m_Segments.GetHeadPosition();           // copy segments...
   while( pos != NULL )
   {
      CCxSegment* pSrcSeg = pSrcTrial->m_Segments.GetNext( pos );    // current seg to be copied; advance to next seg.
      CCxSegment* pSeg = NULL;
      try
      {
         pSeg = new CCxSegment;                                      // create default, empty segment object
         pSeg->Copy( *pSrcSeg );                                     // copy current src seg to the new empty seg obj
         m_Segments.AddTail( pSeg );                                 // and append it to our segment list.
      }
      catch( CMemoryException* e )                                   // if we run out of memory, empty trial defn
      {                                                              // and abort
         e->Delete();
         if(pSeg != NULL) delete pSeg;
         Clear();
         return( FALSE );
      }
   }

   pos = pSrcTrial->m_TaggedSections.GetHeadPosition();              // copy tagged sections...
   while( pos != NULL )
   {
      PTRIALSECT pSrcSect = pSrcTrial->m_TaggedSections.GetNext(pos);
      PTRIALSECT pSect = NULL;
      try
      {
         pSect = new TRIALSECT;
         *pSect = *pSrcSect;
         m_TaggedSections.AddTail( pSect );
      }
      catch( CMemoryException *e )
      {
         e->Delete();
         if( pSect != NULL ) delete pSect;
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
//    Return a list of keys identifying those Maestro objects which are currently referenced by this trial. The trial
//    is "dependent" upon these objects for its complete definition: all participating target objects, any 
//    participating perturbations, and the channel configuration object assigned to the trial.
//
//    This method is required by the CTreeMap/CTreeObj framework in order to "lock" the "independent" objects in the
//    treemap -- providing a mechanism that prevents user from removing them and thereby corrupting the "dependent"
//    object's definition.
//
//    ARGS:       wArKeys -- [out] currently referenced Maestro object keys are stored here. If none, array emptied.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException if unable to grow array to sufficient size to hold all dependencies
//
VOID CCxTrial::GetDependencies( CWordArray& wArKeys ) const
{
   wArKeys.SetSize( 0, MAX_TRIALTARGS + MAX_TRIALPERTS + 1 );  // empty array, but set grow size sufficient to hold
                                                               // maximum # of dependencies
   if( m_hdr.wChanKey != CX_NULLOBJ_KEY )                      // the channel set attached to trial (if any)
      wArKeys.Add( m_hdr.wChanKey );
   wArKeys.Append( m_wArTargs );                               // participating targets attached to trial
   for( int i = 0; i < m_nPerts; i++ )                         // participating perturbations attached to trial...
   {
      WORD w = m_Perts[i].wKey;
      int j = 0; while( j < i && w != m_Perts[j].wKey ) j++;   // ....avoiding duplicate entries!
      if( j == i ) wArKeys.Add( w );
   }

}


//=== IsResponseChecked ===============================================================================================
//
//    During staircase trial sequencing, the subject's response is checked on "correct answer" and "incorrect answer"
//    response channels.  The response may be checked only during selected segments.  This method returns TRUE as long
//    as the response is checked during at least one segment of trial.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if subject's response is checked during one or more segments of trial; FALSE otherwise.
//
BOOL CCxTrial::IsResponseChecked() const
{
   POSITION pos = m_Segments.GetHeadPosition();
   while( pos != NULL )
   {
      if( m_Segments.GetNext( pos )->IsResponseChecked() ) return( TRUE );
   }
   return( FALSE );
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== SetHeader =======================================================================================================
//
//    Modify the trial header.  Any invalid parameters are corrected.
//
// [19nov2024]: PSGM dropped. TRLHDR fields related to the PSGM, .iSGMSeg and .seg, have been REMOVED from TRLHDR.
// 
//    ARGS:       hdr      -- [in] new trial header; [out] same, except that any invalid param values are corrected.
//                bChanged -- [out] TRUE if any header param was changed; FALSE otherwise.
//
//    RETURNS:    TRUE if new header parameters were accepted w/o correction; FALSE if at least one was corrected.
//
BOOL CCxTrial::SetHeader( TRLHDR& hdr, BOOL& bChanged )
{
   BOOL bOk = TRUE;                                            // FALSE if we must correct any parameter in header

   bChanged = FALSE;
   if( IsSameHeader( hdr ) ) return( bOk );                    // no changes!!

   if( hdr.iWeight < TH_MINWEIGHT )                            // out-of-range trial weight
   {
      hdr.iWeight = TH_MINWEIGHT;
      bOk = FALSE;
   }
   else if( hdr.iWeight > TH_MAXWEIGHT )
   {
      hdr.iWeight = TH_MAXWEIGHT;
      bOk = FALSE;
   }

   if( hdr.iStairNum < 0 || hdr.iStairNum > MAX_STAIRS )       // invalid staircase designation
   {
      hdr.iStairNum = 0;
      bOk = FALSE;
   }

   if( (SegCount() == 0 && hdr.iStartSeg != 0) ||              // validate "first save seg" index; when no segs, index
       hdr.iStartSeg < 0 || hdr.iStartSeg >= SegCount() )      // is always 0.
   {
      hdr.iStartSeg = 0;
      bOk = FALSE;
   }

   if( hdr.iFailsafeSeg<-1 || hdr.iFailsafeSeg>=SegCount() )   // invalid failsafe segment designation (-1 means that
   {                                                           // trial must run to completion)
      hdr.iFailsafeSeg = -1;
      bOk = FALSE;
   }

   if( (SegCount() == 0 && hdr.iSpecialSeg != 0) ||            // validate "sacc trig'd op seg" index; when no segs,
       hdr.iSpecialSeg < 0 || hdr.iSpecialSeg >= SegCount() )  // index is always 0.
   {
      hdr.iSpecialSeg = 0;
      bOk = FALSE;
   }

   if(hdr.iSpecialOp < 0 || hdr.iSpecialOp >= TH_NUMSPECOPS)   // validate "special operation" identifier
   {
      hdr.iSpecialOp = TH_SOP_NONE;
      bOk = FALSE;
   }

   // NOTE: these next two fields are NO LONGER USED as of Maestro v2.0.0, trial schema version# 8.
   if( hdr.iOpenSeg<-1 || hdr.iOpenSeg>=SegCount() )
   {
      hdr.iOpenSeg = -1;
      bOk = FALSE;
   }
   if( hdr.nOpenSegs < 0 )
   {
      hdr.nOpenSegs = 1;
      bOk = FALSE;
   }

   if( hdr.iMarkSeg1 < -1 || hdr.iMarkSeg1 >= SegCount() )     // invalid display marker segment #1 (-1 means that
   {                                                           // feature is not in use)
      hdr.iMarkSeg1 = -1;
      bOk = FALSE;
   }

   if( hdr.iMarkSeg2 < -1 || hdr.iMarkSeg2 >= SegCount() )     // invalid display marker segment #2 (-1 means that
   {                                                           // feature is not in use)
      hdr.iMarkSeg2 = -1;
      bOk = FALSE;
   }

   if( hdr.iMTRIntv < TH_MINREWINTV )                          // out-of-range mid-trial reward interval
   {
      hdr.iMTRIntv = TH_MINREWINTV;
      bOk = FALSE;
   }
   else if( hdr.iMTRIntv > TH_MAXREWINTV )
   {
      hdr.iMTRIntv = TH_MAXREWINTV;
      bOk = FALSE;
   }

   if( hdr.iMTRLen < TH_MINREWLEN )                            // out-of-range mid-trial reward pulse length
   {
      hdr.iMTRLen = TH_MINREWLEN;
      bOk = FALSE;
   }
   else if( hdr.iMTRLen > TH_MAXREWLEN )
   {
      hdr.iMTRLen = TH_MAXREWLEN;
      bOk = FALSE;
   }

   // [deprecated: XYScope platform removed in Maestro 5.0] XYScope alternate dot seed and #interleaved targets
   hdr.iXYDotSeedAlt = -1;
   hdr.nXYInterleave = 0;

   if( hdr.iSaccVt < TH_MINSACCVT )                            // out-of-range saccade threshold velocity
   {
      hdr.iSaccVt = TH_MINSACCVT;
      bOk = FALSE;
   }
   else if( hdr.iSaccVt > TH_MAXSACCVT )
   {
      hdr.iSaccVt = TH_MAXSACCVT;
      bOk = FALSE;
   }

   // reward pulses 1 and 2: check for out-of-range length or invalid WHVR
   int val = cMath::rangeLimit(hdr.reward1[0], TH_MINREWLEN, TH_MAXREWLEN);
   if(val != hdr.reward1[0])
   {
      hdr.reward1[0] = val;
      bOk = FALSE;
   }
   val = cMath::rangeLimit(hdr.reward1[1], TH_MINWHVR, TH_MAXWHVR - 1);
   int valD = cMath::rangeLimit(hdr.reward1[2], val + 1, TH_MAXWHVR);
   if(val != hdr.reward1[1] || valD != hdr.reward1[2])
   {
      hdr.reward1[1] = val;
      hdr.reward1[2] = valD;
      bOk = FALSE;
   }

   val = cMath::rangeLimit(hdr.reward2[0], TH_MINREWLEN, TH_MAXREWLEN);
   if(val != hdr.reward2[0])
   {
      hdr.reward2[0] = val;
      bOk = FALSE;
   }
   val = cMath::rangeLimit(hdr.reward2[1], TH_MINWHVR, TH_MAXWHVR - 1);
   valD = cMath::rangeLimit(hdr.reward2[2], val + 1, TH_MAXWHVR);
   if(val != hdr.reward2[1] || valD != hdr.reward2[2])
   {
      hdr.reward2[1] = val;
      hdr.reward2[2] = valD;
      bOk = FALSE;
   }

   if( hdr.fStairStrength < TH_MINSTAIRSTR )                   // out-of-range staircase strength
   {
      hdr.fStairStrength = TH_MINSTAIRSTR;
      bOk = FALSE;
   }
   else if( hdr.fStairStrength > TH_MAXSTAIRSTR )
   {
      hdr.fStairStrength = TH_MAXSTAIRSTR;
      bOk = FALSE;
   }

   if( !IsSameHeader( hdr ) )                                  // if proposed corrected hdr != current header, update
   {                                                           // internal copy of current header
      bChanged = TRUE;
      m_hdr = hdr;
   }
   return( bOk );
}


//=== GetRPDistro =====================================================================================================
//
//    Retrieve the object that encapsulates response distributions, reward windows, and reward/penalty statistics
//    collected during Trial mode runtime when the trial uses the "R/P Distro" special operation.
//
//    ARGS:       NONE.
//
//    RETURNS:    Pointer to the CCxRPDistro object.  Callers should not store the pointer and must NOT delete it!
//
CCxRPDistro* CCxTrial::GetRPDistro()
{
   // the object that stores runtime info is lazily created or destroyed!
   if( m_hdr.iSpecialOp == TH_SOP_RPDISTRO )
   {
      if( m_pRPDistro == NULL ) m_pRPDistro = new CCxRPDistro();
   }
   else
   {
      if( m_pRPDistro != NULL )
      {
         delete m_pRPDistro;
         m_pRPDistro = NULL;
      }
   }

   return( m_pRPDistro );
}


//=== InsertSeg =======================================================================================================
//
//    Insert a new segment at the specified position (zero-based index) in segment list.  Existing segments are moved
//    over to make room, and the new segment is initialized with default values and with sufficient trajectory records
//    to handle the # of targets currently participating in the trial.
//
//    If the insertion occurs before any of the designated segment positions (the failsafe segment, etc), then we must
//    increment its zero-based position so that it still refers to the same physical segment.  A similar adjustment
//    must also be applied to the segment indices stored in the trial's perturbation and tagged section lists.
//
//    ARGS:       iPos     -- [in] zero-based insertion position; if invalid pos, segment is appended.
//
//    RETURNS:    Zero-based position of new segment if successful; -1 if segment list is full.
//
//    THROWS:     CMemoryException if unable to allocate segment object or insert it into segment list.  In such a
//                case, we clean up any allocations that were made here prior to the exception to avoid memory leaks,
//                then pass on the exception.
//
int CCxTrial::InsertSeg( const int iPos )
{
   int nCount = SegCount();
   if ( nCount == MAX_SEGMENTS )                               // segment list is maxed out!
      return( -1 );

   BOOL bAfter = FALSE;                                        // insert before segment at specified pos, unless...
   int iNew = iPos;
   if ( nCount == 0 )                                          // ...seg list empty:  inserting first segment
      iNew = 0;
   else if ( (iNew < 0) || (iNew >= nCount) )                  // ...invalid pos:  append by inserting after last rec
   {
      iNew = nCount-1;
      bAfter = TRUE;
   }

   CCxSegment* pSeg = new CCxSegment;                          // create default, empty segment
                                                               // **** THROWS CMemoryException

   try                                                         // allocate required # of traj records in segment, then
   {                                                           // insert seg ptr in trial seg list.  guard againt mem
                                                               // exceptions...
      pSeg->AllocTraj( TargCount() );

      if ( nCount == 0 )                                       //    first segment (arg ignored in this case)
         m_Segments.AddHead( pSeg );                           //    **** THROWS CMemoryException
      else                                                     //    general insertion case:
      {
         POSITION pos = m_Segments.FindIndex( iNew );
         if ( bAfter )
            m_Segments.InsertAfter( pos, pSeg );               //    **** THROWS CMemoryException
         else
            m_Segments.InsertBefore( pos, pSeg );              //    **** THROWS CMemoryException
      }
   }
   catch( CMemoryException *e )                                // caught memory excpt; must release new seg obj before
   {                                                           // passing on exception so it's not left dangling!
      UNREFERENCED_PARAMETER(e);
      delete pSeg;
      throw;
   }

   ASSERT( pSeg != NULL );                                     // if we get here, insertion must have been successful!
   if( nCount > 0 )                                            // adjust pos of all segment indices in trial header as
   {                                                           // needed.  if an index is -1, it is not affected...
      int iSeg = m_hdr.iStartSeg;
      if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
         ++(m_hdr.iStartSeg);

      iSeg = m_hdr.iFailsafeSeg;
      if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
         ++(m_hdr.iFailsafeSeg);

      iSeg = m_hdr.iSpecialSeg;
      if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
         ++(m_hdr.iSpecialSeg);

      iSeg = m_hdr.iMarkSeg1;
      if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
         ++(m_hdr.iMarkSeg1);

      iSeg = m_hdr.iMarkSeg2;
      if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
         ++(m_hdr.iMarkSeg2);

      for( int i = 0; i < m_nPerts; i++ )                      // adjust pos of all segment indices in pert list...
      {
         iSeg = int(m_Perts[i].cSeg);
         if ( (iNew < iSeg) || ((iNew == iSeg) && (!bAfter)) )
            ++(m_Perts[i].cSeg);
      }

      UpdateTaggedSectionsOnSegInsert( bAfter ? (iNew+1) : iNew );
   }

   return( (bAfter) ? (iNew+1): iNew );
}


//=== RemoveSeg =======================================================================================================
//
//    Remove an existing segment at the specified position (zero-based index) in the trial's segment list.  Unlike
//    CutSeg(), this method frees the memory allocated to the segment object that was removed.
//
//    ARGS:       iPos  -- [in] zero-based position of segment to be deleted.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos or list empty), in which case seg list unchanged.
//
BOOL CCxTrial::RemoveSeg( const int iPos )
{
   CCxSegment* pDeadSeg = CutSeg( iPos );
   if( pDeadSeg != NULL )
   {
      delete pDeadSeg;
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== CutSeg ==========================================================================================================
//
//    Remove specified segment from the trial's segment list.  Instead of deleting the CCxSegment object, we return a
//    pointer to it, leaving its disposition to the caller's discretion.
//
//    If the deletion occurs before any of the designated segments (failsafe seg, etc) in the trial header, then we
//    must decrement its zero-based position so that it still refers to the same physical segment.  If a specially
//    designated segment is itself deleted, then we reassign the index to point to the segment after the deleted one
//    (or before, if we happen to delete last segment in list).  If the segment list is empty after the deletion, then
//    the designated segment index will be set to -1 or 0.  Also, if a special op was in effect and the last segment
//    is deleted, the special op is turned off.
//
//    A similar adjustment must be applied to the segment indices stored in the trial's perturbation and tagged
//    section lists.  However, when the zero-based pos of the deleted segment equals a perturbation's "start segment"
//    index, we do not reassign it to a different segment; rather, the start segment becomes undefined (-1),
//    effectively disabling the pert.
//
//    ARGS:       iPos     -- [in] zero-based position of the segment to be copied.
//
//    RETURNS:    Pointer to the segment object that was cut.  Caller is responsible for freeing the object when
//                finished with it!  If unsuccessful, returns NULL.
//
CCxSegment* CCxTrial::CutSeg( const int iPos )
{
   if( (iPos < 0) || (iPos >= SegCount()) )                       // invalid pos or list empty
      return( NULL );

   POSITION pos = m_Segments.FindIndex( iPos );                   // get ptr to segment to be deleted
   CCxSegment* pSeg = m_Segments.GetAt( pos );
   m_Segments.RemoveAt( pos );                                    // remove ptr from list

   BOOL bLastSegDel = (iPos == SegCount());
   int iSeg = m_hdr.iStartSeg;                                    // adjust "first save" segment as needed...
   if( iPos < iSeg )                                              // (remains at 0 when last seg deleted)
      --(m_hdr.iStartSeg);

   iSeg = m_hdr.iFailsafeSeg;                                     // adjust failsafe segment as needed...
   if( (iPos < iSeg) || ((iPos == iSeg) && bLastSegDel) )
      --(m_hdr.iFailsafeSeg);

   iSeg = m_hdr.iSpecialSeg;                                      // adjust special segment as needed...
   if( iPos < iSeg )                                              // (remains at 0 when last seg deleted)
      --(m_hdr.iSpecialSeg);

   iSeg = m_hdr.iMarkSeg1;                                         // adjust "display marker segments as needed...
   if( (iPos < iSeg) || ((iPos == iSeg) && bLastSegDel) )
      --(m_hdr.iMarkSeg1);
   iSeg = m_hdr.iMarkSeg2;
   if( (iPos < iSeg) || ((iPos == iSeg) && bLastSegDel) )
      --(m_hdr.iMarkSeg2);

   for( int i = 0; i < m_nPerts; i++ )                            // adjust start segment indices in pert list...
   {
      iSeg = int(m_Perts[i].cSeg);
      if( iPos == iSeg ) m_Perts[i].cSeg = -1;
      else if( iPos < iSeg ) --(m_Perts[i].cSeg);
   }

   UpdateTaggedSectionsOnSegRemove( iPos );                       // adjust seg indices in tagged section list

   if( SegCount() == 0 )                                          // if last seg deleted, turn off special op
      m_hdr.iSpecialOp = TH_SOP_NONE;

   return( pSeg );
}


//=== CopySeg =========================================================================================================
//
//    Make a duplicate of the segment at the specified pos in the trial's segment list.  This new segment object is NOT
//    inserted in the trial itself.  Instead, its pointer is passed to the caller, which can then "paste" the segment
//    at another position in this trial or a different (but compatible) trial via the PasteSeg() method.
//
//    ARGS:       iPos     -- [in] zero-based position of the segment to be copied.
//
//    RETURNS:    Pointer to the segment copy.  Caller is responsible for freeing the object when finished with it!
//                If unsuccessful, returns NULL.
//
//    THROWS:     CMemoryException if unable to allocate the new segment object.  In such a case, we clean up any
//                allocations made here to avoid memory leaks, then pass on the exception.
//
CCxSegment* CCxTrial::CopySeg( const int iPos )
{
   if ( (iPos < 0) || (iPos >= SegCount()) ) return( NULL );         // invalid segment index

   CCxSegment* pSeg = RetrieveSegment( iPos );                       // segment to be copied

   CCxSegment* pCopySeg = new CCxSegment;                            // allocate new segment

   try
   {
      pCopySeg->Copy( *pSeg );                                       // do the copy, which could fail...
   }
   catch( CMemoryException* e )                                      // in which case we need to free the new seg
   {                                                                 // obj before forwarding the exception!
      UNREFERENCED_PARAMETER(e);
      delete pSeg;
      throw;
   }

   return( pCopySeg );
}


//=== PasteSeg ========================================================================================================
//
//    Paste a duplicate of the provided segment into the trial's segment list at the specified position.  If the pos is
//    invalid, the segment is appended at the end of the segment list.
//
//    NOTE that we do NOT simply insert the provided segment ptr into the segment list, as this would leave the segment
//    unprotected (the caller could modify it without going through CCxTrial methods!).  We insert a new segment at
//    the specified pos and make it the same as the paste segment.
//
//    ARGS:       iPos  -- [in] zero-based insertion position; if invalid pos, segment is appended.
//                pSeg  -- [in] ptr to the segment to be pasted.
//
//    RETURNS:    Zero-based position of new segment if successful; -1 if paste op is not possible.
//
//    THROWS:     CMemoryException if unable to allocate segment object or insert it into segment list.
//
int CCxTrial::PasteSeg( const int iPos, const CCxSegment* pSeg )
{
   ASSERT_VALID( pSeg );                        // validate the segment object

   if( !CanPasteSeg( pSeg ) )                   // can we do the paste?
      return( -1 );

   int iIns = InsertSeg( iPos );                // insert a new segment (initialized with default values)
                                                // **** THROWS CMemoryException
   if ( iIns < 0 ) return( -1 );

   CCxSegment* pNew = RetrieveSegment( iIns );  // retrieve ptr to new segment and copy the paste seg to it
   pNew->Copy( *pSeg );                         // should NOT throw memory exception, since any allocs already done

   return( iIns );
}


//=== ReplaceSeg ======================================================================================================
//
//    Replace the definition of a specified segment in the trial's segment list.
//
//    NOTE that we do NOT simply insert the provided segment ptr into the segment list, as this would leave the segment
//    unprotected (the caller could modify it without going through CCxTrial methods!).
//
//    ARGS:       iPos  -- [in] zero-based position of segment to be replaced.
//                pSeg  -- [in] ptr to the replacement segment definition.
//
//    RETURNS:    TRUE if successful; -1 if replace op is not possible.
//
BOOL CCxTrial::ReplaceSeg( const int iPos, const CCxSegment* pSeg )
{
   ASSERT_VALID( pSeg );                              // validate the segment object

   if( (iPos >= 0) && (iPos < SegCount()) &&          // is seg position valid?
       CanReplaceSeg( pSeg ) )                        // can we do the replace?
   {
      CCxSegment* pReplace = RetrieveSegment( iPos ); // retrieve ptr to existing seg and copy replacement seg to it
      pReplace->Copy( *pSeg );
      return( TRUE );
   }
   else
      return( FALSE );
}


//=== InsertTarget ====================================================================================================
//
//    Insert Maestro target object into the trial's participating target list. Insert a target trajectory record for
//    the new target into each of the trial's currently existing segments. Existing records are shifted to make room
//    for the new record. The trajectory parameters are initialized to default values.
//
//    If the insertion occurs before or at any target index stored in the trial's perturbation list, that index must be
//    incremented so that it still refers to the same physical target.
//
//    ARGS:       iPos     -- [in] zero-based insertion position; if invalid insertion pos, target is appended.
//                wTargKey -- [in] key identifying the Maestro target object to be inserted.
//
//    RETURNS:    TRUE if successful; FALSE if we've reached target capacity or specified tgt is already in the trial.
//
//    THROWS:     CMemoryException, if unable to insert the new target key, or if unable to add trajectory records for
//                the new target to each existing segment of trial.  In either case, we ensure trial object is
//                restored to its original state before passing on the exception...
//
BOOL CCxTrial::InsertTarget( const int iPos, const WORD wTargKey )
{
   int nCount = TargCount();
   if ( nCount == MAX_TRIALTARGS )                       // target array is maxed out!
      return( FALSE );

   int i;
   for( i = 0; i < nCount; i++ )                         // make sure the key is not already there!
      if( m_wArTargs[i] == wTargKey )
         return( FALSE );

   int iInsert = iPos;                                   // insert at specified pos
   if ( (iInsert < 0) || (iInsert >= nCount) )           // if invalid pos, append at end of array
      iInsert = nCount;

   m_wArTargs.InsertAt( iInsert, wTargKey );             // insert target's key into participating target array
                                                         // **** THROWS CMemoryException

   try
   {
      POSITION pos = m_Segments.GetHeadPosition();       // insert default trajectory record at corresponding pos in
      while( pos != NULL )                               // each seg of trial
      {
         CCxSegment* pSeg = m_Segments.GetNext( pos );
         VERIFY( pSeg->InsertTraj( iInsert ) );          // **** THROWS CMemoryException
      }
   }
   catch( CMemoryException* e )                          // if memory exception occurs while adding traj records...
   {
      UNREFERENCED_PARAMETER(e);
      POSITION pos = m_Segments.GetHeadPosition();       //    remove new trajectory record from each segment that
      while( pos != NULL )                               //    got one.
      {
         CCxSegment* pSeg = m_Segments.GetNext( pos );
         if ( pSeg->TrajCount() > nCount )               //    as soon as this is not true, we're done checking segs.
            VERIFY( pSeg->RemoveTraj( iInsert ) );
         else
            break;
      }

      m_wArTargs.RemoveAt( iInsert );                    //    remove new target's key from participating tgt array

      throw;                                             //    pass on the exception!
   }

   for( i = 0; i < m_nPerts; i++ )                       // adjust tgt indices in the perturbation list as needed...
   {
      if( int(m_Perts[i].cTgt) >= iInsert )
         ++(m_Perts[i].cTgt);
   }

   return( TRUE );
}


//=== RemoveTarget ====================================================================================================
//
//    Remove target at the specified position (zero-based index) in the trial's participating target array.  Must also
//    remove the corresponding trajectory record from each segment in the trial's segment list.
//
//    If the deletion occurs before any target index stored in the trial's perturbation list, that index must be
//    decremented so that it still refers to the same physical target.  If the deletion occurs AT a target index in the
//    pert list, then that index becomes undefined (-1), effectively, disabling the perturbation.
//
//    ARGS:       iPos  -- [in] zero-based position of target to be deleted.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos or target array empty).
//
BOOL CCxTrial::RemoveTarget( const int iPos )
{
   if ( (iPos < 0) || (iPos >= TargCount()) )                     // invalid pos or target array empty
      return( FALSE );

   m_wArTargs.RemoveAt( iPos );                                   // remove target's key from target array

   POSITION pos = m_Segments.GetHeadPosition();                   // remove corresponding traj record from each seg
   while( pos != NULL )
   {
      CCxSegment* pSeg = m_Segments.GetNext( pos );
      VERIFY( pSeg->RemoveTraj( iPos ) );
   }

   for( int i = 0; i < m_nPerts; i++ )                            // adjust tgt indices in the pert list as needed...
   {
      int iTgt = int(m_Perts[i].cTgt);
      if( iTgt == iPos ) m_Perts[i].cTgt = -1;
      else if( iTgt > iPos ) --(m_Perts[i].cTgt);
   }

   return( TRUE );
}


//=== SetTarget =======================================================================================================
//
//    Replace existing target in the trial with a different one.  Since the participating target list CANNOT contain
//    any duplications, we check to make sure the new target is not already there.
//
//    ARGS:       iPos     -- [in] zero-based position of target to be replaced.
//                wTargKey -- [in] the key/ID of the new target.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos, or new target is already in the target list).
//
BOOL CCxTrial::SetTarget( const int iPos, const WORD wTargKey )
{
   if( (wTargKey == CX_NULLOBJ_KEY) || (iPos < 0) || (iPos >= TargCount()) )
      return( FALSE );

   for( int i = 0; i < TargCount(); i++ )
   {
      if( (i != iPos) && (m_wArTargs[i] == wTargKey) ) return( FALSE );
   }

   m_wArTargs[iPos] = wTargKey;
   return( TRUE );
}


//=== Clear ===========================================================================================================
//
//    Removes all segments, targets, perturbations, random variables, and tagged sections from the trial (deallocating 
//    memory accordingly) and resets trial header parameters to default values.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrial::Clear()
{
   while( !m_Segments.IsEmpty() )
   {
      CCxSegment* pSeg = m_Segments.RemoveHead();
      delete pSeg;
   }
   m_wArTargs.RemoveAll();
   m_nPerts = 0;
   RemoveAllRVs();
   RemoveAllTaggedSections();
   AssignDefaultHeader();

   if( m_pRPDistro != NULL )
   {
      delete m_pRPDistro;
      m_pRPDistro = NULL;
   }
   ClearRVRuntimeState();
}


//=== Serialize [base override] =======================================================================================
//
//    Handles reading/writing the trial object from/to a disk file via a serialization archive.
//
//    Version Control (using MFC "schema"):
//       1: Base version.
//       2: Perturbation list added.
//       2: Velocity stabilization (open loop segment) is now separately enabled for H, V, or both directions.  It was
//          NOT necessary to change the schema version, because the default state is to enable both directions -- so
//          trial objects serialized prior to this change will have both directions enabled.
//       3: Added display marker segments #1 and #2 (segment indices in TRLHDR).
//       3: Added special operation "Dual Fix", identified by flag bit THF_DUALFIX in TRLHDR.dwFlags.  It was NOT
//          necessary to change the schema version, since TRLHDR was not changed and since default is THF_NOSPECIAL.
//       3: Special op "Dual Fix" now called "Switch Fix", and flag bit was renamed THF_SWITCHFIX in TRLHDR.dwFlags.
//          No effect on existing documents.
//       4: Mid-trial reward feature enhanced.  Added integer fields 'iMTRIntv' and 'iMTRLen' to TRLHDR, plus flag bit
//          THF_MTRMODE.  If flag bit clear, mid-trial rewards are delivered periodically within enabled segments.  If
//          set, they are delivered at the end of each enabled segment.  The new fields define, respectively, the
//          reward interval for periodic mode and the reward pulse length for either mode.
//       5: Tagged sections added.
//       6: Added per-trial alternate XY dot seed.
//       6: Added special op "RP Distro", with corres. flag bit THF_RPDISTRO.  No effect on existing documents.
//       7: Added #segments over which velocity stabilization is in effect: TRLHDR.nOpenSegs.
//       8: Velocity stabilization reworked.  User can now control v.stab on a per-segment, per-target basis via the
//          SGTJF_VSTAB* flags in the trajectory record.  Fields TRLHDR.iOpenSeg and nOpenSegs are no longer used, but
//          they still exist -- which eases migration of documents existing prior to this schema change.
//       9: Added field for the special operation id: TRLHDR.iSpecialOp. The THF_** flags that were used to select the 
//          special operation are now obsolete. Two new special ops were added, TH_SOP_CHOOSEFIX1 & TH_SOP_CHOOSEFIX2.
//      10: Added SGM op mode SGM_BIPHASICTRAIN. This has the same value as SGM_NOOP in previous versions. Other
//          SGM-related changes involved less restrictive parameter changes; no adj needed in these cases.
//      10: (v2.1.2) Added two new perturbable trajectory quantities -- window speed (PERT_ON_SWIN) and pattern speed 
//          (PERT_ON_SPAT). No effect on existing documents.
//      10: (v2.1.3) Added two new perturbable trajectory quantities -- window AND pattern direction identically 
//          perturbed (PERT_ON_DIR); window AND pattern speed identically perturbed (PERT_ON_SPD). No effect on 
//          existing documents.
//      11: (v3.3.0) Added support for random variables in a trial. No effect on existing documents.
//      12: (v4.1.0) Added support for WHVR for reward pulses 1 and 2. No effect on existing documents.
//      13: (v5.0.0) XYScope support officially removed. Deprecated trial header parameters TRLHDR.iXYDotSeedAlt and
//          TRLHDR.nXYInterleave are no longer serialized and are ignored when a pre-v13 trial is deserialized.
//      14: (v5.0.2) PSGM support removed (the module was never actually used in experiments). PSGM-specific trial
//          header fields TRLHDR.iSGMSeg and TRLHDR.sgm no longer exist. When deserializing a pre-V14 trial, all 
//          PSGM parameters are simply read in and discarded.
//
//    ARGS:       ar -- [in] the serialization archive.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException, CArchiveException, CFileException.  When deserializing, framework should delete
//                the "partial" object if an exception occurs while it is being read in from file.
//
void CCxTrial::Serialize ( CArchive& ar )
{
   UINT nSchema = ar.GetObjectSchema();                     // retrieve schema#
   CTreeObj::Serialize( ar );                               // base class stuff first
   m_wArTargs.Serialize( ar );                              // the participating target list
   m_Segments.Serialize( ar );                              // the segments

   int i;

   if( ar.IsStoring() )                                     // the trial header fields
   {
      ar << m_hdr.dwFlags;
      ar << m_hdr.iWeight << m_hdr.iStairNum;
      ar << m_hdr.iStartSeg << m_hdr.iFailsafeSeg << m_hdr.iSpecialSeg << m_hdr.iSpecialOp;
      ar << m_hdr.iMarkSeg1 << m_hdr.iMarkSeg2 << m_hdr.iMTRIntv << m_hdr.iMTRLen;
      // DEPRECATED as of v13 (Maestro 5.0): ar << m_hdr.iXYDotSeedAlt << m_hdr.nXYInterleave;
      ar << m_hdr.iSaccVt;
      ar << m_hdr.reward1[0] << m_hdr.reward1[1] << m_hdr.reward1[2];
      ar << m_hdr.reward2[0] << m_hdr.reward2[1] << m_hdr.reward2[2];
      ar << m_hdr.fStairStrength << m_hdr.wChanKey;

      ar << m_nPerts;                                       // #perts in perturbation list
      for( i = 0; i < m_nPerts; i++ )                       // the perturbation list itself
      {
         CPertEntry* p = &(m_Perts[i]);
         ar << p->wKey << p->fAmp;
         BYTE b = (BYTE) p->cSeg; ar << b;
         b = (BYTE) p->cTgt; ar << b;
         b = (BYTE) p->cIdCmpt; ar << b;
      }

      // #tagged sections defined on trial, followed by the tagged section records, in order.
      //
      // NOTE: 32-bit -> 64-bit GOTCHA. I originially used this statement to archive the tagged section count.
      //     ar << m_TaggedSections.GetCount();
      // But the return value of CPtrList.GetCount() is INT_PTR, which is 64 bits on 64-bit system, but 32 on
      // a 32-bit system. So, on 64-bit system, 8 bytes get written. Yet the deserialization code reads the
      // count into an int variable, which is 4 bytes on both 32-bit and 64-bit systems. This immediately
      // messes up deserialization of anything after this point! FIX: Write count as 4-byte int.
      int nSects = (int) m_TaggedSections.GetCount();
      ar << nSects;
      POSITION pos = m_TaggedSections.GetHeadPosition();    // the tagged section records, in order
      while( pos != NULL )
      {
         PTRIALSECT pSect = m_TaggedSections.GetNext( pos );

         CString str = pSect->tag;
         ar << str;
         BYTE b = (BYTE) pSect->cFirstSeg;
         ar << b;
         b = (BYTE) pSect->cLastSeg;
         ar << b;
      }

      // serialize ONLY the RVs in use. Must serialize RV index since user can employ any subset of 10 available.
      int nUsed = 0;
      for(i=0; i<MAX_TRIALRVS; i++) if(IsRVInUse(i)) ++nUsed;
      ar << nUsed;

      if(nUsed > 0) 
      {
         for(i=0; i<MAX_TRIALRVS; i++) 
         {
            CRVEntry* pRV = &(m_Vars[i]);
            if(pRV->iType != RV_NOTUSED)
            {
               ar << i << pRV->iType << pRV->iSeed;
               ar << pRV->dParams[0] << pRV->dParams[1] << pRV->dParams[2] << pRV->strFunc;
            }
         }
      }
   }
   else
   {
      if(nSchema < 1 || nSchema > 14)                     // unsupported version
         ::AfxThrowArchiveException(CArchiveException::badSchema);

      TRLHDR hdr;                                           // first fill in temporary header from archive...
      ar >> hdr.dwFlags;
      ar >> hdr.iWeight >> hdr.iStairNum;
      ar >> hdr.iStartSeg >> hdr.iFailsafeSeg >> hdr.iSpecialSeg;

      // next field is the special operation ID, in schema version 9 and greater. In previous schemas, some flag 
      // bits were used to identify the special operation (if any) in use.
      if(nSchema >= 9)
         ar >> hdr.iSpecialOp;
      else
      {
         DWORD oldSpecOpFlags = (hdr.dwFlags & THF_SPECALL);
         hdr.dwFlags &= ~THF_SPECALL;

         if(oldSpecOpFlags == THF_SACCSKIP) hdr.iSpecialOp = TH_SOP_SKIP;
         else if(oldSpecOpFlags == THF_SELBYFIX) hdr.iSpecialOp = TH_SOP_SELBYFIX;
         else if(oldSpecOpFlags == THF_SELBYFIX2) hdr.iSpecialOp = TH_SOP_SELBYFIX2;
         else if(oldSpecOpFlags == THF_SWITCHFIX) hdr.iSpecialOp = TH_SOP_SWITCHFIX;
         else if(oldSpecOpFlags == THF_RPDISTRO) hdr.iSpecialOp = TH_SOP_RPDISTRO;
         else hdr.iSpecialOp = TH_SOP_NONE;
      }

      if(nSchema < 8)                                     // next two fields exist only in docs prior to schema 8.
      {                                                     // They are NOW obsolete, but we need them to migrate old
         ar >> hdr.iOpenSeg;                                // doc to the new way that v.stab is configured!
         if(nSchema >= 7) ar >> hdr.nOpenSegs;
         else hdr.nOpenSegs = 1;
      }

      if(nSchema >= 3)                                    // ver 3 includes display marker segments #1, #2 in hdr;
      {                                                     // for earlier docs, we default these to -1 (ie, no
         ar >> hdr.iMarkSeg1 >> hdr.iMarkSeg2;              // display markers)
      }
      else
      {
         hdr.iMarkSeg1 = -1;
         hdr.iMarkSeg2 = -1;
      }

      if(nSchema >= 4)                                    // ver 4 includes params for mid-trial reward feature
      {
         ar >> hdr.iMTRIntv >> hdr.iMTRLen;
      }
      else
      {
         hdr.iMTRIntv = TH_DEFREWINTV;
         hdr.iMTRLen = TH_DEFREWLEN;
      }

      // XYScope alternate dot seed: Added in v6, deprecated in v13
      if(nSchema >= 6 && nSchema < 13)
         ar >> hdr.iXYDotSeedAlt;
      else
         hdr.iXYDotSeedAlt = -1;

      // XYScope #interleaved targets: Deprecated. No longer serialized a/o v13.
      if(nSchema < 13)
         ar >> hdr.nXYInterleave;
      else
         hdr.nXYInterleave = 0;

      ar >> hdr.iSaccVt;

      // ver 12 adds WHVR numerator and denomintor for reward pulses 1 and 2. Use defaults for older schema
      if(nSchema >= 12)
      {
         ar >> hdr.reward1[0] >> hdr.reward1[1] >> hdr.reward1[2];
         ar >> hdr.reward2[0] >> hdr.reward2[1] >> hdr.reward2[2];
      }
      else
      {
         ar >> hdr.reward1[0] >> hdr.reward2[0];
         hdr.reward1[1] = hdr.reward2[1] = TH_DEFWHVR;
         hdr.reward1[2] = hdr.reward2[2] = TH_DEFWHVR + 1;
      }

      ar >> hdr.fStairStrength >> hdr.wChanKey;

      // PSGM support dropped in schema version 14. For pre-V14 trials, we simply read and discard the PSGM params.
      if(nSchema < 14)
      {
         int iSGMSeg = 0;
         SGMPARMS sgm{};

         ar >> iSGMSeg >> sgm.iOpMode;
         ar >> sgm.bExtTrig;
         ar >> sgm.iAmp1 >> sgm.iAmp2 >> sgm.iPW1 >> sgm.iPW2;
         ar >> sgm.iPulseIntv >> sgm.iTrainIntv;
         ar >> sgm.nPulses >> sgm.nTrains;
      }

      BOOL bChanged = FALSE;
      SetHeader( hdr, bChanged );                           // ...then set the real header w/ auto-correction

      if( nSchema >= 2 )                                    // ver >=2 includes perturbation list after trial header
      {
         ar >> m_nPerts;                                    // #perts in list
         for( i = 0; i < m_nPerts; i++ )                    // the perturbation entries themselves...
         {
            CPertEntry* p = &(m_Perts[i]);
            BYTE b;
            ar >> p->wKey >> p->fAmp;
            ar >> b; p->cSeg = char(b);
            ar >> b; p->cTgt = char(b);
            ar >> b; p->cIdCmpt = char(b);
         }
      }
      else m_nPerts = 0;                                    // prior to ver 2, there was no perturbation list

      if( nSchema >= 5 )                                    // ver>=5 includes tagged sections list
      {
         int nSects = 0;
         ar >> nSects;

         CString strTag;
         BYTE b;
         int s0, s1;
         for( i = 0; i<nSects; i++ )
         {
            ar >> strTag;
            ar >> b;
            s0 = int(b);
            ar >> b;
            s1 = int(b);
            CreateTaggedSection( s0, s1 );
            RenameTaggedSection( i, strTag);
         }
      }

      // trial random variables were added in version 11
      RemoveAllRVs();
      if(nSchema >= 11)
      {
         int nUsed, idx;
         ar >> nUsed;
         for(i=0; i<nUsed; i++)
         {
            ar >> idx;
            ASSERT(idx >= 0 && idx < MAX_TRIALRVS);
            CRVEntry* pRV = &(m_Vars[idx]);
            ar >> pRV->iType >> pRV->iSeed >> pRV->dParams[0] >> pRV->dParams[1] >> pRV->dParams[2] >> pRV->strFunc;
         }
      }
      
      if( nSchema < 8 && m_hdr.iOpenSeg >= 0 )              // migrate defn of pre-version 8 trials using v. stab...
      {
         int iSeg = m_hdr.iOpenSeg;
         int iTgt = GetFixTarg1Pos(iSeg);

         DWORD dwOldOpenMode = m_hdr.dwFlags & THF_OPENMASK;
         int iMode = 0;
         if( dwOldOpenMode == THF_OPEN_HONLY ) iMode = SGTJ_VSTABHONLY;
         else if( dwOldOpenMode == THF_OPEN_VONLY ) iMode = SGTJ_VSTABVONLY;
         else iMode = SGTJ_VSTABBOTH;

         BOOL bSnap = BOOL((m_hdr.dwFlags & THF_SNAPTO) != 0);

         for( i=0; i<m_hdr.nOpenSegs; i++ )
         {
            SetTgtVStabMode(iSeg+i, iTgt, iMode);
            if( i==0 ) SetTgtVStabSnapToEye(iSeg, iTgt, bSnap);
         }

         // obsolete fields and flags are reset after migration because they are no longer used.
         m_hdr.dwFlags &= ~(THF_SNAPTO|THF_OPENMASK);
         m_hdr.iOpenSeg = -1;
         m_hdr.nOpenSegs = 1;
      }
   }

   ASSERT_VALID( this );                                    // check validity AFTER serializing
}


//=====================================================================================================================
// OPERATIONS -- INDIVIDUAL PARAMETER ACCESS
//=====================================================================================================================

/**
 Get the value of the target trajectory parameter specified for the purpose of displaying and editing that value. Since
 these trajectory parameters are random variable-assignable, their "value" may be either a numeric floating-point 
 constant or the zero-based integer index of the currently assigned random variable.

 To retrieve the current floating-point value of the parameter for the purposes of presenting the trial, regardless
 whether or not an RV is assigned to the parameter, use the GetCurrTgt***H/V() accessor methods.

 @param s [in] Segment index.
 @param t [in] Target index.
 @param p [in] Enumerated parameter ID. Must be one of [TGTHPOS..PATVACC].
 @param isRV [out] This is set to TRUE if a trial RV is currently assigned to the parameter; else FALSE.
 @return The parameter value. If isRV==TRUE, cast this to an integer to get the zero-based index of the trial RV
 currently assigned to the parameter. If isRV==FALSE, this is the current numeric value of the parameter. If the
 arguments do not identify a valid target trajectory parameter, method returns 0 and isRV is set to FALSE.
*/
double CCxTrial::GetTgtTrajParam(int s, int t, ParamID p, BOOL& isRV) const
{
   isRV = FALSE;
   return(IsValidSeg(s) ? RetrieveSegment(s)->GetTgtTrajParam(t, p, isRV) : 0.0);
}

/**
 Set the value of the target trajectory parameter specified. Since these trajectory parameters are random variable-
 assignable, their "value" may be either a numeric floating-point constant or the zero-based integer index of the 
 currently assigned random variable.

 @param s [in] Segment index.
 @param t [in] Target index.
 @param p [in] Enumerated parameter ID. Must be one of [TGTHPOS..PATVACC].
 @param dVal [in] The new parameter value.
 @param asRV [in] If asRV==TRUE, dVal is cast to an integer and interpreted as the zero-based index of the trial RV to
 be assigned to the parameter. If the index is invalid, the parameter is set to a numeric constant of 0 (NOT the RV at
 index 0). If asRV==FALSE, dVal is the new numeric constant assigned to the parameter.
 @return TRUE if the new parameter value was accepted without correction; FALSE if it was auto-corrected.
*/
BOOL CCxTrial::SetTgtTrajParam(int s, int t, ParamID p, double dVal, const BOOL asRV)
{
   return(IsValidSeg(s) ? RetrieveSegment(s)->SetTgtTrajParam(t, p, dVal, asRV) : FALSE);
}

//=== IsValidSegParam, Get/SetSegParam... =============================================================================
//
//    This group of methods provides generalized access to parameters in the trial's segment table, including both
//    "segment header" and "target trajectory" parameters.  An individual parameter is identified by three indices:
//    the segment number, the target number (-1 for segment header parameters), and an enumerated ID identifying the
//    parameter. The methods provide enough information so that a view class can display and edit any parameter in the
//    segment table without requiring hard-coded knowledge of the parameter's identity.
//
//    All parameters fall into one of three classes:  a floating-point number, an integer, or a multiple-choice value.
//    A multiple-choice value is merely an integer having a limited range [0..N-1], where N is the # of choices
//    available.  The BOOLean-valued parameters are treated as two-choice parameters with choices 0 and 1.  However,
//    somewhat counter-intuitively, choice 0 is mapped to the parameter's "ON" or "TRUE" state, while choice 1 is
//    mapped to the "OFF" or "FALSE" state.
//
//    A view class can retrieve the value of any parameter as a double, integer, or a CString via the GetSegParam()
//    and GetSegParamAsInt() methods.  The CString form is best for ensuring that the current value is displayed in
//    the most sensible fashion.  This is particularly important for multiple-choice parameters, since the CString
//    value is a text label that is more meaningful than the zero-based choice index!  To edit the parameter, the view
//    class should invoke GetSegParamFormat() to obtain a numeric parameter's format constraints, or the set of
//    available choices for a multi-choice parameter.  IsSegParamMultiChoice() returns TRUE if parameter is multiple
//    choice.  GetSegParamLabel() provides a descriptive name of the specified parameter.  Finally, SetSegParam()
//    changes the current value of a parameter with built-in auto-correction.  The overloaded version accepting integer
//    values is suitable only for int-valued parameters, while the version accepting float values works for all params.
//
//    "Side effects" of SetSegParam():  Changing min or max segment duration can affect the other parameter, since we
//    require min <= max duration.  Currently, all other parameters in the segment table are independent, but invalid
//    parameters will be auto-corrected.  Whenever a parameter change is auto-corrected or has a side-effect, the
//    SetSegParam() routine returns TRUE.
//
//    IsValidSegParam() checks whether or not the <seg#, tgt#, paramID> triplet identifies an existing parameter in the
//    segment table.  If NOT:  GetSegParam() & GetSegParamAsInt() return "0", GetSegParamLabel() retrieves an empty
//    string, GetSegParamFormat() retrieves a multiple-choice parameter with no choices, IsSegParamMultiChoice()
//    returns FALSE, and SetSegParam() has no effect.
//
//    On trial random variables (introduced in v3.3.0, fall 2016):
//    Any parameter to which a trial random variable may be assigned is special. It can be thought of as a normal 
//    integer or numeric parameter, or it can be thought of as a multiple-choice value because any one of the 10
//    RVs available in a trial can be assigned to it, as well as a numeric constant. The choices are "x0" .. "x9" for
//    the 10 different RVs, plus "const"; this last choice is selected to restore a parameter to some constant value. 
//    The trial definition form must implement distinct gestures to distinguish between the scenarios (RV assignment or
//    numeric constant). The function CanAssignRVToSegParam() indicates whether or not a particular parameter can be 
//    mapped to an RV, while IsRVAssignedToSegParam() indicates whether an RV is currently assigned to a parameter. 
//
//    The behavior of the ***SegParam() methods is somewhat different when the parameter can have an RV assigned to it:
//       -- GetSegParam(s,t,p), GetSetParamAsInt() : When an RV is currently assigned to the parameter, these methods 
//    return the RV's zero-based index. If no RV is assigned, they return the parameter's current constant value. 
//    Callers must check IsRVAssignedToSegParam() to interpret the return value correctly.
//       -- GetSegParam(s,t,p,str) : When an RV is currently assigned to the parameter, the string is set to "x0" ..
//    "x9". Otherwise, the parameter's current constant value is converted to string form.
//       -- GetSegParamFormat() : For RV-assignable parameters, this method provides both a choice list and a numeric
//    format. 
//       -- IsSegParamMultiChoice() : Does NOT return TRUE for RV-assignable parameters, since they may be interpreted
//    as multi-choice or as numeric. Callers MUST use the RV-specific methods to determine the parameter's status.
//       -- SetSegParam() : Now has a fifth boolean parameter, "asRV", that defaults to FALSE. If TRUE, then the new
//    parameter value is cast to an integer and interpreted as the zero-based index of the RV to be assigned to the
//    parameter. If the parameter is NOT RV-assignable, the method takes no action and returns FALSE. Otherwise, the
//    specified RV is assigned to the parameter; if the index is invalid and the parameter is currently assigned to an
//    RV, then it is restored to a default numeric constant.
//
//
//    ARGS:       s, t, p     -- [in] segment index, target record index, and enumerated parameter ID. Together these
//                               uniquely identify a param of interest. For segment header params, tgt index ignored.
//                str         -- [out] string representation of parameter's value, or a descriptive name for parameter.
//                bIsChoice   -- [out] TRUE if this is a multi-choice parameter; otherwise it's numeric.
//                choices     -- [out] set to available choices for a multi-choice parameter; else empty.
//                fmt         -- [out] numeric format constraints for a numeric parameter.
//                iVal, dVal  -- [in] new value for parameter.
//                asRV        -- [in] if TRUE, treat new param value as the zero-based index of a trial random variable
//                               that is to be assigned to the specified parameter. If that index is out-of-range, the
//                               parameter is set to a default contant value (IF it is currently assigned to an RV).
//
//    RETURNS:    various
//
BOOL CCxTrial::IsValidSegParam( int s, int t, ParamID p ) const
{
   return( (p==NOTAPARAM) ? FALSE : ((p >= CCxTrial::TGTONOFF) ? IsValidTrajRecord( s, t ) : IsValidSeg( s )) );
}

double CCxTrial::GetSegParam( int s, int t, ParamID p ) const
{
   if( !IsValidSegParam( s, t, p ) ) return( 0.0 );                        // non-existent parameter!
   
   // if RV assigned to a parameter, return its index!
   BOOL isRV = IsRVAssignedToSegParam(s, t, p);

   // return each param formatted as a double (for BOOLS, 0->TRUE, 1->FALSE !!!)
   double d = 0.0;
   switch( p ) 
   {
      // min and max duration are RV-assignable. When RV is assigned, param value = -1-index.
      case MINDURATION :
         d = double(GetMinDuration( s )); 
         if(isRV) d = cMath::abs(d) - 1;
         break;
      case MAXDURATION :   
         d = double(GetMaxDuration( s ));  
         if(isRV) d = cMath::abs(d) - 1;
         break;

      case RMVSYNCENA:     d = IsRMVSyncFlashOn(s) ? 0.0 : 1.0;   break;
      case FIXTARG1 :      d = double(GetFixTarg1Pos( s ) + 1);   break;   // -1 = NONE is always first choice
      case FIXTARG2 :      d = double(GetFixTarg2Pos( s ) + 1);   break;   // -1 = NONE is always first choice
      case FIXACCH :       d = GetFixAccH( s );                   break;
      case FIXACCV :       d = GetFixAccV( s );                   break;
      case FIXGRACE :      d = double(GetGracePeriod( s ));       break;
      case REWENA :        d = IsMidTrialRewEnable(s) ? 0.0:1.0;  break;
      case SEGMARKER :     d = double(GetMarker( s ));            break;
      case CHECKRESP :     d = IsResponseChecked(s) ? 0.0 : 1.0;  break;
      case TGTONOFF :      d = IsTgtOn( s, t ) ? 0.0 : 1.0;       break;
      case TGTPOSABS :     d = IsAbsolutePos( s, t ) ? 0.0 : 1.0; break;
      case TGTVSTABMODE :  d = double(GetTgtVStabMode(s, t));     break;
      case TGTVSTABSNAP :  d = IsTgtVStabSnapToEye(s, t) ? 0.0 : 1.0;  break;

      // these target trajectory params are all RV-assignable. In this case, if an RV is currently assigned to the
      // param, the return value from the accessor method is set to the RV index (cast to a double)
      case TGTHPOS :
      case TGTVPOS :
      case TGTHVEL : 
      case TGTVVEL :
      case TGTHACC :
      case TGTVACC :
      case PATHVEL :
      case PATVVEL :
      case PATHACC :
      case PATVACC :
         d = GetTgtTrajParam(s, t, p, isRV);
         break;
   }
   return( d );
}

int CCxTrial::GetSegParamAsInt( int s, int t, ParamID p ) const
{
   double d = GetSegParam( s, t, p );
   return( (d < 0.0) ? int(d - 0.5) : int(d + 0.5) );
}

VOID CCxTrial::GetSegParam( int s, int t, ParamID p, CString& str ) const
{
   str.Empty();
   if( !IsValidSegParam( s, t, p ) ) return;                            // invalid parameter returned as empty string!

   BOOL bIsChoice = FALSE;                                              // get parameter display format
   CStringArray choices;
   NUMEDITFMT fmt;
   GetSegParamFormat( p, bIsChoice, choices, fmt );

   // set parameter value as string IAW format. An RV assignable parameter is interpreted as multi-choice WHEN an RV
   // is currently assigned to it; else it is numeric. If a floating-point param has no fractional part, show it more
   // compactly as an integer string.
   if(CanAssignRVToSegParam(s, t, p)) bIsChoice = IsRVAssignedToSegParam(s, t, p);
   if(bIsChoice) 
      str = (LPCTSTR) choices.GetAt( GetSegParamAsInt( s, t, p ) );
   else if( (fmt.flags & NES_INTONLY) != 0 )
      str.Format( "%d", GetSegParamAsInt( s, t, p ) );
   else                                                                 // if floating-pt param has no fractional part,
   {                                                                    // then show it more compactly as an integer
      double dVal = GetSegParam( s, t, p );
      if( cMath::abs( cMath::frac(dVal) ) < 0.0001 )
         str.Format( "%d", GetSegParamAsInt( s, t, p ) );
      else
         str.Format( "%.*f", fmt.nPre, dVal );
   }
}

VOID CCxTrial::GetSegParamLabel( ParamID p, CString& str ) const
{
   str.Empty();
   if( p == NOTAPARAM ) return;                                         // invalid parameter returned as empty string!

   switch( p )
   {
      case MINDURATION :   str = _T("Min Dur (ms)");              break;
      case MAXDURATION :   str = _T("Max Dur (ms)");              break;
      case RMVSYNCENA:     str = _T("RMV Sync");                  break;
      case FIXTARG1 :      str = _T("Fix Tgt 1");                 break;
      case FIXTARG2 :      str = _T("Fix Tgt 2");                 break;
      case FIXACCH :       str = _T("H Fix Accuracy (deg)");      break;
      case FIXACCV :       str = _T("V Fix Accuracy (deg)");      break;
      case FIXGRACE :      str = _T("Fix Grace Period (ms)");     break;
      case REWENA :        str = _T("Mid-trial Reward?");         break;
      case SEGMARKER :     str = _T("Marker Pulse");              break;
      case CHECKRESP :     str = _T("Check Response?");           break;
      case TGTONOFF :      str = _T("Tgt On/Off");                break;
      case TGTPOSABS :     str = _T("Tgt Pos Abs/Rel");           break;
      case TGTVSTABMODE:   str = _T("Tgt Vel Stabilize Mode");    break;
      case TGTVSTABSNAP:   str = _T("Tgt V.Stab Snap to Eye?");   break;
      case TGTHPOS :       str = _T("H Window Pos (deg)");        break;
      case TGTVPOS :       str = _T("V Window Pos (deg)");        break;
      case TGTHVEL :       str = _T("H Window Vel (deg/s)");      break;
      case TGTVVEL :       str = _T("V Window Vel (deg/s)");      break;
      case TGTHACC :       str = _T("H Window Acc (deg/s^2)");    break;
      case TGTVACC :       str = _T("V Window Acc (deg/s^2)");    break;
      case PATHVEL :       str = _T("H Pattern Vel (deg/s)");     break;
      case PATVVEL :       str = _T("V Pattern Vel (deg/s)");     break;
      case PATHACC :       str = _T("H Pattern Acc (deg/s^2)");   break;
      case PATVACC :       str = _T("V Pattern Acc (deg/s^2)");   break;
   }
}

VOID CCxTrial::GetSegParamFormat( ParamID p, BOOL& bIsChoice, CStringArray& choices, NUMEDITFMT& fmt ) const
{
   choices.RemoveAll();
   bIsChoice = TRUE;                                                    // if parameter index invalid, format param
   if( p == NOTAPARAM ) return;                                         // as multi-choice w/ an empty choice set!!

   bIsChoice = FALSE;
   fmt.nID = 0;                                                         // these constraints apply to most of the FP
   fmt.flags = 0;                                                       // numeric parameters, with exceptions handled
   fmt.nLen = 7;                                                        // below...
   fmt.nPre = 2;

   int i;
   CString str;
   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc();                // to retrieve names of trial targets
   switch( p )
   {
      case REWENA :                                                     // two-choice parameters (BOOLs)
      case CHECKRESP :
      case RMVSYNCENA :
      case TGTONOFF :
         bIsChoice = TRUE;
         choices.Add( _T("ON") );
         choices.Add( _T("OFF") );
         break;
      case TGTPOSABS :
         bIsChoice = TRUE;
         choices.Add( _T("ABS") );
         choices.Add( _T("REL") );
         break;
      case TGTVSTABSNAP :
         bIsChoice = TRUE;
         choices.Add( _T("w/SNAP") );
         choices.Add( _T(" -- ") );
         break;

      case TGTVSTABMODE :                                               // four possible velocity stabilization modes
         bIsChoice = TRUE;
         choices.Add( _T("OFF") );
         choices.Add( _T("H+V") );
         choices.Add( _T("H ONLY") );
         choices.Add( _T("V ONLY") );
         break;

      case FIXTARG1 :                                                   // here the choices are the names of the
      case FIXTARG2 :                                                   // targets participating in the trial!
         bIsChoice = TRUE;
         choices.Add( _T("NONE") );                                     // -1 ==> "NONE" is the first choice
         for( i = 0; i < TargCount(); i++ )
            choices.Add( pDoc->GetObjName( GetTarget( i ) ) );
         break;
      case SEGMARKER :                                                  // first choice is NO segment marker pulse;
         bIsChoice = TRUE;                                              // else, a pulse on DOUT lines <1..max>.
         choices.Add( _T("OFF") );
         for( i = 1; i <= SGH_MAXMARKER; i++ )
         {
            str.Format( "DOUT%d", i );
            choices.Add( str );
         }
         break;

      // min and max segment duration are RV-assignable, so they can be multi-choice or numeric
      case MINDURATION :
      case MAXDURATION : 
         fmt.flags = NES_INTONLY | NES_NONNEG; 
         fmt.nLen = 5;
         fmt.nPre = 1;
         bIsChoice = TRUE;
         for(i=0; i<MAX_TRIALRVS; i++)
         {
            str.Format(_T("x%d"), i);
            choices.Add(str);
         }
         choices.Add(_T("const"));
         break;

      // these target trajectory params are RV-assignable, so they can be multi-choice or numeric. Also, the
      // acceleration params have slightly different numeric format constraints than the defaults set above.
      case TGTHPOS:
      case TGTVPOS:
      case TGTHVEL:
      case TGTVVEL:
      case TGTHACC :
      case TGTVACC :
      case PATHVEL :
      case PATVVEL :
      case PATHACC :
      case PATVACC :
         if(p==TGTHACC || p==TGTVACC || p==PATHACC || p==PATVACC)
         {
            fmt.nLen = 8;
            fmt.nPre = 3;
         }
         bIsChoice = TRUE;
         for(i=0; i<MAX_TRIALRVS; i++)
         {
            str.Format(_T("x%d"), i);
            choices.Add(str);
         }
         choices.Add(_T("const"));
         break;

      // all other params are numeric; those with format different from the default above are set below...
      case FIXGRACE :
         fmt.flags = NES_INTONLY | NES_NONNEG;
         fmt.nLen = 4;
         fmt.nPre = 1;
         break;
      case FIXACCH :
      case FIXACCV :
         fmt.flags = NES_NONNEG;
         fmt.nLen = 6;
         fmt.nPre = 2;
         break;
   }
}

BOOL CCxTrial::IsSegParamMultiChoice( ParamID p ) const
{
   return( p==FIXTARG1 || p==FIXTARG2 || p==REWENA || p==CHECKRESP || p==SEGMARKER || p==TGTONOFF || p==TGTPOSABS ||
      p==TGTVSTABMODE || p==TGTVSTABSNAP || p==RMVSYNCENA );
}

BOOL CCxTrial::SetSegParam( int s, int t, ParamID p, double dVal, BOOL asRV)
{
   // do nothing if parameter does not exist, or attempt is made to assign RV to a parameter that does not allow it
   if(!IsValidSegParam(s, t, p)) return(FALSE); 
   if(asRV && !CanAssignRVToSegParam(s, t, p)) return(FALSE);

   BOOL isRV = IsRVAssignedToSegParam(s, t, p);

   // integer version is rounded value; for BOOLs: 0->TRUE, 1->FALSE, with forward & backward wrapping
   int iVal = int((dVal < 0.0) ? dVal - 0.5 : dVal + 0.5);
   BOOL bVal = (iVal < 0 || iVal == 1) ? FALSE : TRUE;

   // will be set FALSE if change is auto-corrected or it causes a change in another parameter
   BOOL bUncorr = TRUE; 
   switch(p)
   {
      // min and max segment duration are RV-assignable, so they require special treatment. When an RV index is 
      // specified, we must convert it to the value with which it is represented internally.
      case MINDURATION :   
      case MAXDURATION :
         if(asRV)
         {
            // if RV index is out-of-range, then set it to a default duration of 1000ms -- but ONLY if it is
            // currently assigned to an RV. This is how we restore these parameters to a numeric constant. If
            // it is a valid index, we need to convert it to how it's stored internally.
            if(iVal < 0 || iVal>=MAX_TRIALRVS) 
            {
               if(isRV) iVal = 1000;
               else return(FALSE);
            }
            else iVal = -iVal - 1;
         }
         bUncorr = BOOL((p == MINDURATION) ? SetMinDuration( s, iVal ) : SetMaxDuration(s, iVal));     
         break;

      case RMVSYNCENA:     bUncorr = SetRMVSyncFlashOn(s, bVal);    break;
      case FIXTARG1 :      bUncorr = SetFixTarg1Pos( s, iVal-1 );   break; // choice 0 = "NONE" ==> -1, etc.
      case FIXTARG2 :      bUncorr = SetFixTarg2Pos( s, iVal-1 );   break; // choice 0 = "NONE" ==> -1, etc.
      case FIXACCH :       bUncorr = SetFixAccH( s, dVal );         break;
      case FIXACCV :       bUncorr = SetFixAccV( s, dVal );         break;
      case FIXGRACE :      bUncorr = SetGracePeriod( s, iVal );     break;
      case REWENA :        bUncorr = SetMidTrialRewEnable(s,bVal);  break;
      case SEGMARKER :     bUncorr = SetMarker( s, iVal );          break;
      case CHECKRESP :     bUncorr = SetResponseChecked( s, bVal);  break;
      case TGTONOFF :      bUncorr = SetTgtOn( s, t, bVal );        break;
      case TGTPOSABS :     bUncorr = SetAbsolutePos( s, t, bVal );  break;
      case TGTVSTABMODE :  bUncorr = SetTgtVStabMode(s, t, iVal);   break;
      case TGTVSTABSNAP :  bUncorr = SetTgtVStabSnapToEye(s, t, bVal); break;

      // these target trajectory parameters are all RV-assignable
      case TGTHPOS :
      case TGTVPOS :
      case TGTHVEL :
      case TGTVVEL :
      case TGTHACC :
      case TGTVACC :
      case PATHVEL :
      case PATVVEL :
      case PATHACC :
      case PATVACC : 
         bUncorr = SetTgtTrajParam(s, t, p, dVal, asRV);
         break;
   }
   return(bUncorr);
}

/**
 Can one of the trial's random variables be assigned to the specified segment table parameter?

 When an RV is assigned to a parameter, that parameter takes on the RV's current value, which is updated on each trial
 presentation IAW the RV's definition. Currently, any floating-point target trajectory parameter can be assigned to an
 RV, as can the min or max duration of any segment.

 @param s Segment index.
 @param t Target index, if relevant.
 @param p Enumerated parameter ID.
 @return TRUE if specified parameter exists and can be assigned to a trial random variable. Else FALSE.
*/
BOOL CCxTrial::CanAssignRVToSegParam(int s, int t, ParamID p) const
{
   if(!IsValidSegParam(s, t, p)) return(FALSE);
   return(p==MINDURATION || p==MAXDURATION || (p>=TGTHPOS && p<=PATVACC));
}

/**
 Is one of the trial's random variables currently assigned to the specified segment table parameter? See 
 CanAssignRVToSegParam() for details.
 @param s Segment index.
 @param t Target index, if relevant.
 @param p Enumerated parameter ID.
 @return TRUE if the specified parameter exists and is currently set to the value of a random variable.
*/
BOOL CCxTrial::IsRVAssignedToSegParam(int s, int t, ParamID p) const
{
   if(!CanAssignRVToSegParam(s, t, p)) return(FALSE);

   if(p==MINDURATION) return(BOOL(GetMinDuration(s)<0));
   else if(p==MAXDURATION) return(BOOL(GetMaxDuration(s)<0));
   else if(p>=TGTHPOS && p<=PATVACC)
   {
      BOOL isRV = FALSE;
      GetTgtTrajParam(s, t, p, isRV);
      return(isRV);
   }

   return(FALSE);
}



//=====================================================================================================================
// OPERATIONS -- PERTURBATION LIST
//=====================================================================================================================

//=== AppendPert ======================================================================================================
//
//    Append an entry to the trial's perturbation list.
//
//    ARGS:       wKey  -- [in] the key/ID of the perturbation object to be added to the list.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid key, or perturbation list full).
//
BOOL CCxTrial::AppendPert( WORD wKey )
{
   if( m_nPerts == MAX_TRIALPERTS || wKey==CX_NULLOBJ_KEY ) // abort if perturbation list is maxed out, or NULL key
      return( FALSE );

   m_Perts[m_nPerts].wKey = wKey;                           // append entry in perturbation list; initially...
   m_Perts[m_nPerts].fAmp = 1.0f;                           //    unit amplitude
   m_Perts[m_nPerts].cSeg = -1;                             //    start seg undefined
   m_Perts[m_nPerts].cTgt = -1;                             //    affected tgt not defined
   m_Perts[m_nPerts].cIdCmpt = PERT_ON_HWIN;                //    modulates H window velocity
   ++m_nPerts;

   return( TRUE );
}


//=== RemovePert ======================================================================================================
//
//    Remove a selected entry (or all entries) from the trial's perturbation list.
//
//    ARGS:       iPos  -- [in] pos of entry to remove; if -1, all entries are removed.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos).
//
BOOL CCxTrial::RemovePert( int iPos )
{
   if( iPos != -1 && !IsValidPert( iPos ) ) return( FALSE );            // invalid pos specified
   if( iPos == -1 ) { m_nPerts = 0; return( TRUE ); }                   // clear the perturbation list

   for( int i = iPos+1; i < m_nPerts; i++ ) m_Perts[i-1] = m_Perts[i];  // remove the selected perturbation entry
   --m_nPerts;
   return( TRUE );
}


//=== SetPert =========================================================================================================
//
//    Modify the attributes of a selected entry in the trial's perturbation list.  Various restrictions are enforced
//    for the different attribute values:
//
//       perturbation key ==> cannot be CX_NULLOBJ_KEY (caller is responsible for ensuring the key points to a valid
//          perturbation in the current CNTRLX doument).
//       amplitude ==> range-restricted to +/-999.99 deg/sec.
//       affected tgt ==> must be a valid trial target index.
//       start segment ==> must be a valid segment index.
//       affected trajectory cmpt ==> must be one of [PERT_ON_HWIN, .., PERT_ON_SPD].
//
//    ARGS:       iPos  -- [in] pos of entry to remove; if -1, all entries are removed.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (invalid pos, invalid value; amplitude auto-corrected).
//
BOOL CCxTrial::SetPert( int iPos, WORD wKey, float fAmp, int iSeg, int iTgt, int idCmpt )
{
   if( !IsValidPert( iPos ) || wKey == CX_NULLOBJ_KEY ||
       iSeg < -1 || iSeg >= SegCount() || iTgt < -1 || iTgt >= TargCount() ||
       idCmpt < PERT_ON_HWIN || idCmpt > PERT_ON_SPD )
      return( FALSE );

   m_Perts[iPos].wKey = wKey;
   m_Perts[iPos].fAmp = (fAmp < -999.99f) ? -999.99f : ((fAmp > 999.99f) ? 999.99f : fAmp);
   m_Perts[iPos].cSeg = char(iSeg);
   m_Perts[iPos].cTgt = char(iTgt);
   m_Perts[iPos].cIdCmpt = char(idCmpt);
   return( TRUE );
}


//=== Get/SetPert***** ================================================================================================
//
//    Get or set individual attributes of a selected entry in the trial's perturbation list.  For the "setter" methods,
//    restrictions described in SetPert() are enforced.  For the "multi-choice" parameters, out-of-bounds values are
//    wrapped forward or backward.
//
//    ARGS:       iPos  -- [in] pos of perturbation entry.
//                wKey, fAmp, iTgt, iSeg, idCmpt -- [in] the new attribute value.
//
//    RETURNS:    getter methods:  the attribute's current value (or 0/-1 if invalid entry specified).
//                setter methods:  TRUE if successful; FALSE otherwise (invalid entry, etc)
//
WORD CCxTrial::GetPertKey( int iPos ) { return( IsValidPert( iPos ) ? m_Perts[iPos].wKey : CX_NULLOBJ_KEY ); }
BOOL CCxTrial::SetPertKey( int iPos, WORD wKey )
{
   if( !IsValidPert( iPos ) || wKey == CX_NULLOBJ_KEY ) return( FALSE );
   m_Perts[iPos].wKey = wKey;
   return( TRUE );
}

float CCxTrial::GetPertAmp( int iPos )  { return( IsValidPert( iPos ) ? m_Perts[iPos].fAmp : 0.0f ); }
BOOL CCxTrial::SetPertAmp( int iPos, float fAmp )
{
   if( !IsValidPert( iPos ) ) return( FALSE );
   m_Perts[iPos].fAmp = (fAmp < -999.99f) ? -999.99f : ((fAmp > 999.99f) ? 999.99f : fAmp);
   return( TRUE );
}

int CCxTrial::GetPertSeg( int iPos ) { return( IsValidPert( iPos ) ? int(m_Perts[iPos].cSeg) : -1 ); }
int CCxTrial::SetPertSeg( int iPos, int iSeg )
{
   if( !IsValidPert( iPos ) ) return( FALSE );
   int iCorr = (iSeg < -1) ? SegCount()-1 : ((iSeg >= SegCount()) ? -1 : iSeg);
   m_Perts[iPos].cSeg = char(iCorr);
   return( TRUE );
}

int CCxTrial::GetPertTgt( int iPos ) { return( IsValidPert( iPos ) ? int(m_Perts[iPos].cTgt) : -1 ); }
int CCxTrial::SetPertTgt( int iPos, int iTgt )
{
   if( !IsValidPert( iPos ) ) return( FALSE );
   int iCorr = (iTgt < -1) ? TargCount()-1 : ((iTgt >= TargCount()) ? -1 : iTgt);
   m_Perts[iPos].cTgt = char(iCorr);
   return( TRUE );
}

WORD CCxTrial::GetPertTgtKey( int iPos )
{
   int iTgt = GetPertTgt( iPos );
   return( IsValidTarg( iTgt ) ? GetTarget( iTgt ) : CX_NULLOBJ_KEY );
}

int CCxTrial::GetPertTrajCmpt( int iPos ) { return( IsValidPert( iPos ) ? int(m_Perts[iPos].cIdCmpt) : -1 ); }
int CCxTrial::SetPertTrajCmpt( int iPos, int idCmpt )
{
   if( !IsValidPert( iPos ) ) return( FALSE );
   int idCorr = (idCmpt < PERT_ON_HWIN) ? PERT_ON_SPD : ((idCmpt > PERT_ON_SPD) ? PERT_ON_HWIN : idCmpt);
   m_Perts[iPos].cIdCmpt = char(idCorr);
   return( TRUE );
}



//=====================================================================================================================
// OPERATIONS -- TAGGED SECTIONS
//=====================================================================================================================

//=== HasTaggedSections ===============================================================================================
//
//    Does this trial have any tagged sections?
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if one or more tagged sections are defined on this trial.
//
BOOL CCxTrial::HasTaggedSections() const
{
   return( !m_TaggedSections.IsEmpty() );
}

//=== GetNumTaggedSections ============================================================================================
//
//    Get number of tagged sections defined on this trial.
//
//    ARGS:       NONE.
//
//    RETURNS:    Number of tagged sections currently defined on this trial.
//
int CCxTrial::GetNumTaggedSections() const
{
   return( static_cast<int>(m_TaggedSections.GetCount()) );
}

//=== GetNumTaggedSegments ============================================================================================
//
//    Get number of segments in this trial that are part of a tagged section.
//
//    ARGS:       NONE.
//
//    RETURNS:    Number of segments in trial that are part a tagged section.  If there are no tagged sections in the
//                trial, 0 is returned.
//
int CCxTrial::GetNumTaggedSegments() const
{
   int nTagged = 0;
   POSITION pos = m_TaggedSections.GetHeadPosition();
   while( pos != NULL )
   {
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      nTagged += int(pSect->cLastSeg - pSect->cFirstSeg) + 1;
   }
   return( nTagged );
}

//=== GetTaggedSection ================================================================================================
//
//    Retrieve info (tag name, start and end segment indices) for a tagged section.
//
//    ARGS:       i     -- [in] zero-based index into list of trial's tagged sections.
//                sect  -- [out] info on tagged section is stored here, IF specified index is valid.
//
//    RETURNS:    TRUE if successful; FALSE if section index not valid.
//
BOOL CCxTrial::GetTaggedSection(int i, TRIALSECT& sect) const
{
   POSITION pos = m_TaggedSections.FindIndex( i );
   if( pos != NULL )
      sect = *(m_TaggedSections.GetAt( pos ));
   return( BOOL(pos != NULL) );
}

//=== GetTaggedSectionNameByName ======================================================================================
//
//    Retrieve the zero-based index locating a tagged section defined on this trial and having the given tag name.
//
//    ARGS:       tag   -- [in] name tag of the section sought
//
//    RETURNS:    The zero-based index of the section, or -1 if it was not found.
//
int CCxTrial::GetTaggedSectionByName(LPCTSTR tag) const
{
   int index = 0;
   int found = -1;
   PTRIALSECT pSect;
   POSITION pos = m_TaggedSections.GetHeadPosition();
   while( pos != NULL )
   {
      pSect = m_TaggedSections.GetNext(pos);
      if( ::strcmp(tag, pSect->tag) == 0 )
      {
         found = index;
         break;
      }
      ++index;
   }
   return( found );
}

//=== GetTaggedSectionName ============================================================================================
//
//    Retrieve the tag name for a tagged section.
//
//    ARGS:       i     -- [in] zero-based index into list of trial's tagged sections.
//                tag   -- [out] section's tag name is stored here, IF specified index is valid.
//
//    RETURNS:    TRUE if successful; FALSE if section index not valid.
//
BOOL CCxTrial::GetTaggedSectionName(int i, CString& tag) const
{
   POSITION pos = m_TaggedSections.FindIndex( i );
   if( pos != NULL )
   {
      PTRIALSECT pSect = m_TaggedSections.GetAt( pos );
      tag = pSect->tag;
   }
   return( BOOL(pos != NULL) );
}

//=== CreateTaggedSection =============================================================================================
//
//    Create a tagged section for this trial, spanning the specified (contiguous) range of segments. Since tagged
//    sections cannot overlap, the method will "fix" the existing set of sections to ensure there is no overlap.
//    Any section that is completely spanned by the new section is removed, while a section that partially overlaps
//    with the new section is suitably truncated.  The tagged section list is maintained in increasing order of
//    segment range; ie, a section spanning segments [1..3] will precede a section spanning [4..4].
//
//    The new section is provided with a default tag name.  The method ensures that it is unique among the existing set
//    of tagged sections defined on this trial.
//
//    ARGS:       s0,s1 -- [in] segment range spanned by section.  s0 must be less than or equal to s1, and both must
//                         be valid segment indices.
//
//    RETURNS:    TRUE if successful; FALSE if any argument is invalid.
//
//    THROWS:     CMemoryException if unable to allocate the new tagged section record or insert it into the section
//                list.  In such a case, we clean up any allocations that were made here prior to the exception to
//                avoid memory leaks, then pass on the exception.
//
BOOL CCxTrial::CreateTaggedSection(int s0, int s1)
{
   if( s0 < 0 || s0 >= SegCount() || s1 < 0 ||                 // abort if segment range indices are invalid
       s1 >= SegCount() || s0 > s1 )
      return( FALSE );

   CString strTag = _T("section");                             // provide a default tag that is NOT already in use by
   int i=1;                                                    // an existing tagged section
   while( GetTaggedSectionByName(strTag) >= 0 )
   {
      strTag.Format( "section%d", i++ );
   }

   PTRIALSECT pNewSect = new TRIALSECT;                        // allocate and fill out the new tagged section record
   strcpy_s( pNewSect->tag, LPCTSTR(strTag) );
   pNewSect->cFirstSeg = char(s0);
   pNewSect->cLastSeg = char(s1);

   POSITION pos = m_TaggedSections.GetHeadPosition();          // remove all sections completely spanned by new one
   while( pos != NULL )
   {
      POSITION pos2 = pos;
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      if( (pSect->cFirstSeg >= pNewSect->cFirstSeg) && (pSect->cLastSeg <= pNewSect->cLastSeg) )
      {
         m_TaggedSections.RemoveAt(pos2);
         delete pSect;
      }
   }

   POSITION posInsertBefore = NULL;                            // find insertion location for new section so that
   pos = m_TaggedSections.GetHeadPosition();                   // sections are maintained in order by their seg ranges
   while( pos != NULL )
   {
      POSITION pos2 = pos;
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      if( pSect->cLastSeg > pNewSect->cLastSeg )
      {
         posInsertBefore = pos2;
         break;
      }
   }

   POSITION posInsert = NULL;                                  // try to insert the new section
   try
   {
      if( posInsertBefore == NULL )
         posInsert = m_TaggedSections.AddTail( pNewSect );
      else
         posInsert = m_TaggedSections.InsertBefore(posInsertBefore, pNewSect );
   }
   catch( CMemoryException *e )                                // caught memory exception during ptr insertion; must
   {                                                           // release trajectory rec before passing on exception...
      UNREFERENCED_PARAMETER(e);
      delete pNewSect;
      throw;
   }

   if( posInsertBefore != NULL )                               // fix segment ranges of sections before and after the
   {                                                           // inserted section so that there is no overlap
      PTRIALSECT pNextSect = m_TaggedSections.GetAt(posInsertBefore);
      if( pNextSect->cFirstSeg <= pNewSect->cLastSeg )
         pNextSect->cFirstSeg = pNewSect->cLastSeg + 1;
   }
   pos = posInsert;
   m_TaggedSections.GetPrev(pos);
   if( pos != NULL )
   {
      PTRIALSECT pPrevSect = m_TaggedSections.GetAt(pos);
      if( pPrevSect->cLastSeg >= pNewSect->cFirstSeg )
         pPrevSect->cLastSeg = pNewSect->cFirstSeg - 1;
   }

   return( TRUE );
}

/**
 * Create a tagged section for this trial, spanning the specified (contiguous) range of segments. The operation fails
 * if the specified section overlaps an existing section, or if its tag name is invalid or duplicates that of an 
 * existing section.
 *
 * @param s0, s1 Index of first and last segments in contiguous range spanned by section. s0 must be less than or equal
 * to s1, and both must be valid indices. Must not overlap an existing section.
 * @param tag The tag label for the tagged section. Length must be in [1..SECTIONTAGSZ-1], and must not duplicate the
 * tag of an existing section.
 * @return TRUE if successful; FALSE if specified section is invalidly specified, if it overlaps an existing section, or
 * if its tag duplicates that of an existing section.
 * @throws CMemoryException if unable to allocate the new tagged section record or insert it into the section list. In 
 * such a case, we clean up any allocations that were made here prior to the exception to avoid memory leaks, then pass 
 * on the exception.
 */
BOOL CCxTrial::CreateTaggedSection(int s0, int s1, LPCTSTR tag)
{
   // fail if segment range indices are invalid
   if(s0 < 0 || s0 >= SegCount() || s1 < 0 || s1 >= SegCount() || s0 > s1)
      return(FALSE);

   // fail if tag is empty or too long, or if it duplicates the label of an existing tagged section
   if(::strlen(tag) <= 0 || ::strlen(tag) >= SECTIONTAGSZ) return(FALSE);
   if(GetTaggedSectionByName(tag) >= 0) return( FALSE );

   PTRIALSECT pNewSect = new TRIALSECT;                        // allocate and fill out the new tagged section record
   ::strcpy_s( pNewSect->tag, tag );
   pNewSect->cFirstSeg = char(s0);
   pNewSect->cLastSeg = char(s1);

   // find insertion location for the new tagged section so that sections are maintained in order by their segment
   // ranges. If new section overlaps an existing section, delete the new section and abort.
   POSITION posInsertBefore = NULL; 
   PTRIALSECT pSectBefore = NULL;
   PTRIALSECT pSectAfter = NULL;
   POSITION pos = m_TaggedSections.GetHeadPosition(); 
   while( pos != NULL )
   {
      POSITION pos2 = pos;
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      if(pSect->cLastSeg > pNewSect->cLastSeg)
      {
         posInsertBefore = pos2;
         pSectAfter = pSect;
         break;
      }
      else pSectBefore = pSect;
   }
   
   if((pSectBefore != NULL && pSectBefore->cLastSeg >= pNewSect->cFirstSeg) || 
         (pSectAfter != NULL && pSectAfter->cFirstSeg <= pNewSect->cLastSeg))
   {
      delete pNewSect;
      return(FALSE);
   }
   
   // insert the new section. If a memory exception occurs, release the new section before passing the exception.
   POSITION posInsert = NULL; 
   try
   {
      if(posInsertBefore == NULL) m_TaggedSections.AddTail(pNewSect);
      else m_TaggedSections.InsertBefore(posInsertBefore, pNewSect);
   }
   catch(CMemoryException *e)
   { 
      UNREFERENCED_PARAMETER(e);
      delete pNewSect;
      throw;
   }

   return(TRUE);
}


//=== RenameTaggedSection =============================================================================================
//
//    Rename an existing tagged section in this trial.
//
//    ARGS:       i     -- [in] zero-based index into list of trial's tagged sections.
//                tag   -- [in] the new name tag for the section.  It must contain at least one character, no more
//                         than SECTIONTAGSZ chars (including terminating null), and it must be different from the
//                         tags of any other section defined on this trial.
//    RETURNS:    TRUE if successful; FALSE if either argument is invalid.
//
BOOL CCxTrial::RenameTaggedSection(int i, LPCTSTR tag)
{
   POSITION selectedPos = m_TaggedSections.FindIndex( i );                 // retrieve the specified tagged section
   if( selectedPos == NULL ) return( FALSE );

   if( ::strlen(tag) <= 0 || ::strlen(tag) >= SECTIONTAGSZ )               // tag cannot be empty nor too long
      return( FALSE );

   if( GetTaggedSectionByName(tag) >= 0 )                                  // tag cannot be in use already
      return( FALSE );

   PTRIALSECT pSect = m_TaggedSections.GetAt(selectedPos);                 // OK.  rename the section!
   ::strcpy_s( pSect->tag, tag );
   return( TRUE );
}

//=== RemoveTaggedSection =============================================================================================
//
//    Remove a tagged section from this trial.  Memory allocated for that tagged section record is released.
//
//    ARGS:       i     -- [in] zero-based index into list of trial's tagged sections.
//
//    RETURNS:    TRUE if successful; FALSE if section index not valid.
//
BOOL CCxTrial::RemoveTaggedSection(int i)
{
   POSITION pos = m_TaggedSections.FindIndex( i );       // retrieve the specified tagged section
   if( pos != NULL )                                     // if valid index, copy the info for that tagged section
   {
      PTRIALSECT pDeadSect = m_TaggedSections.GetAt(pos);
      m_TaggedSections.RemoveAt( pos );
      delete pDeadSect;
   }
   return( BOOL(pos != NULL) );
}

//=== RemoveAllTaggedSections =========================================================================================
//
//    Remove any and all tagged sections defined on this trial.  All memory allocated for these tagged sections is
//    released.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrial::RemoveAllTaggedSections()
{
   while( !m_TaggedSections.IsEmpty() )
   {
      PTRIALSECT pSect = m_TaggedSections.RemoveHead();
      delete pSect;
   }
}

//=== UpdateTaggedSectionsOnSegRemove/Insert ==========================================================================
//
//    Whenever a segment is removed or inserted into the trial, the segment indices bounding any tagged sections may
//    need to be updated.  These functions handle the task.
//
//    ARGS:       iSeg     -- [in] the zero-based index of the segment that was removed or inserted.  In the case of
//                            removal, the index gives the location prior to removal.  In the case of insertion, the
//                            index gives the location of the new segment.
//
//    RETURNS:    NONE.
//
VOID CCxTrial::UpdateTaggedSectionsOnSegRemove( int iSeg )
{
   int nSegs = SegCount();
   if( nSegs == 0 )
   {
      RemoveAllTaggedSections();
      return;
   }

   POSITION posRemove = NULL;                            // if removed seg is the only segment in a section, we must
                                                         // remove that section also!
   POSITION pos = m_TaggedSections.GetHeadPosition();
   char cSeg = char(iSeg);
   while( pos != NULL )
   {
      POSITION posPrev = pos;
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      if( (pSect->cFirstSeg == pSect->cLastSeg) &&       // removed only segment in this section --> remove section!
          (pSect->cLastSeg == cSeg) )
         posRemove = posPrev;
      if( cSeg < pSect->cFirstSeg )                      // #segs in section unaffected; just decrement both indices
      {
         --pSect->cFirstSeg;
         --pSect->cLastSeg;
      }
      else if( cSeg == pSect->cFirstSeg )
      {
         if( pSect->cFirstSeg == pSect->cLastSeg )       // removed only segment in this section --> remove section!
            posRemove = posPrev;
         else                                            // removed first seg in section
            --pSect->cLastSeg;
      }
      else if( cSeg <= pSect->cLastSeg )                 // removed a seg within the section
         --pSect->cLastSeg;
   }

   if( posRemove != NULL )                               // remove the section that consisted only of the deleted seg
   {
      PTRIALSECT pDeadSect = m_TaggedSections.GetAt(posRemove);
      m_TaggedSections.RemoveAt(posRemove);
      delete pDeadSect;
   }
}

VOID CCxTrial::UpdateTaggedSectionsOnSegInsert( int iSeg )
{
   POSITION pos = m_TaggedSections.GetHeadPosition();
   char cSeg = char(iSeg);
   while( pos != NULL )
   {
      PTRIALSECT pSect = m_TaggedSections.GetNext(pos);
      if( cSeg <= pSect->cFirstSeg )                     // insertion before the tagged section: incr both indices
      {
         ++pSect->cFirstSeg;
         ++pSect->cLastSeg;
      }
      else if( cSeg <= pSect->cLastSeg )                 // insertion within tagged section: incr last index only,
         ++pSect->cLastSeg;                              // thus increasing #segments in the section.
   }
}


//=====================================================================================================================
// OPERATIONS -- Random Variables
//=====================================================================================================================

/**
 Retrieve the current definition of one of the trial's random variables. The trial object has 10 RVs, any subset of
 which may be in use. A random variable is represented by the CRVEntry structure, which has the following members:

    CRVEntry.iType : Type identifier - [RV_NOTUSED .. RV_FUNCTION].
    CRVEntry.iSeed : Non-negative seed. If 0, seed is randomly chosen at start of a trial sequence; else fixed. Only
 applies to RV_UNIFORM..RV_GAMMA.
    CRVEntry.dParams[] : Up to 3 distribution parameters. Will be 0 if not applicable. For RV_UNIFORM, there are 2
 parameters, the lower and upper bounds (in that order). For RV_NORMAL, there are 3 - mean ("mu"), standard deviation
 ("sigma"), and max (+/-) spread. For RV_EXPON - rate ("lambda") and max cutoff. For RV_GAMMA - shape ("kappa"), scale 
 ("theta"), and a max cutoff. Any unused parameter will be set to 0.
    CRVEntry.strFunc : The function definition string for RV_FUNCTION; for any other type, this is an empty string.

 @param idx Zero-based index of the random variable. Must lie in [0..MAX_TRIALRV].
 @param rv [out] Set to the definition of the RV specified. See description above.
 @return TRUE if successful, FALSE if index is invalid.
*/
BOOL CCxTrial::GetRV(int idx, CRVEntry& rv) const
{
   if(idx<0 || idx>=MAX_TRIALRVS) return(FALSE);

   rv.iType = m_Vars[idx].iType;
   rv.iSeed = m_Vars[idx].iSeed;
   for(int i=0; i<3; i++) rv.dParams[i] = m_Vars[idx].dParams[i];
   rv.strFunc = m_Vars[idx].strFunc;

   return(TRUE);
}

/**
 Set the definition of one of the trial's 10 random variables.

 @param idx Zero-based index of the random variable. Must lie in [0..MAX_TRIALRVS-1].
 @param rv The new definition for the RV. Must be valid as is. Note that a function RV can never depend on itself.
 @param bLast TRUE if this is the last RV updated. When TRUE, the method verifies that no defined function RV depends on
 another function RV or an unused RV. If any such function RVs are found, the function formula is set to "1" to ensure 
 the RV table is valid.
 @return TRUE if successful, FALSE if RV index or definition is invalid. Also returns FALSE if bLast==TRUE and an
 inspection of the current defined RVs finds one or more function RVs that depend on another function RV or an 
 unused RV.
*/
BOOL CCxTrial::SetRV(int idx, const CRVEntry& rv, BOOL bLast)
{
   if(idx < 0 || idx >= MAX_TRIALRVS) return(FALSE);
   
   CFunctionParser fp("");

   BOOL bOk = FALSE;
   switch(rv.iType)
   {
   case RV_NOTUSED:
      bOk = TRUE;
      break;
   case RV_UNIFORM:
      bOk = (rv.iSeed >= 0) && (rv.dParams[0] < rv.dParams[1]);
      break;
   case RV_NORMAL:
      bOk = (rv.iSeed >= 0) && (rv.dParams[1] > 0) && (rv.dParams[2] >= 3.0 * rv.dParams[1]);
      break;
   case RV_EXPON:
      bOk = (rv.iSeed >= 0) && (rv.dParams[0] > 0) && (rv.dParams[1] >= 3.0 / rv.dParams[0]);
      break;
   case RV_GAMMA:
      bOk = (rv.iSeed >= 0) && (rv.dParams[0] > 0) && (rv.dParams[1] > 0) &&
         (rv.dParams[2] >= rv.dParams[1] * (rv.dParams[0] + 3.0 * ::sqrt(rv.dParams[0])));
      break;
   case RV_FUNCTION:
   {
      fp.SetDefinition(rv.strFunc);
      bOk = fp.IsValid() && !fp.HasVariableX(idx);
   }
      break;
   default:
      break;
   }

   // if the RV definition is valid, store it at the specified index in the trial's interval RV list.
   if(bOk)
   {
      m_Vars[idx].iType = rv.iType;
      m_Vars[idx].iSeed = rv.iSeed;
      for(int i = 0; i < 3; i++) m_Vars[idx].dParams[i] = rv.dParams[i];
      m_Vars[idx].strFunc = (rv.iType == RV_FUNCTION) ? rv.strFunc : _T("");
   }

   // if this is the last RV to be updated, check that no defined function RVs depend on another function RV or an unused RV
   if(bOk && bLast)
   {
      for(int i = 0; i < MAX_TRIALRVS; i++) if(m_Vars[i].iType == RV_FUNCTION)
      {
         fp.SetDefinition(m_Vars[i].strFunc);
         for(int j=0; (i != j) && (j < MAX_TRIALRVS); j++) if(fp.HasVariableX(i))
         {
            if(m_Vars[j].iType == RV_NOTUSED || m_Vars[j].iType == RV_FUNCTION)
            {
               m_Vars[i].strFunc = _T("1");
               bOk = FALSE;
               break;
            }
         }
      }
   }

   return(bOk);
}

/**
 Update a defining parameter for one of the trial's random variables.

 If the supplied parameter value is invalid, the change is rejected. Otherwise, if the change has a side effect on
 the RV's other parameter values, those parameters are corrected commensurately.

 There are additional restrictions on a function-type RV: (1) It cannot depend on itself or on any other function-type
 RV; (2) it cannot depend on an undefined (RV_NOTUSED) RV. If the change would invalidate the specified RV or any
 other currently defined function-type RV, it is rjected.

 @param idx Zero-based index of the random variable that is to be updated. Must lie in [0..MAX_TRIALRVS-1].
 @param id Parameter ID: 1 = type; 2=seed; 3-5 = distribution RV params 1-3; 6 = function formula string.
 @param rv The new value for the parameter specified is contained in this structure.
 @param bSideEffect [out] Set TRUE if the parameter change affects another parameter's value, which is auto-corrected;
 FALSE if there are no such "side effects". Will always be set for a change in RV type.
 @return FALSE if variable index or parameter ID is invalid, if parameter value itself is invalid. Also returns FALSE
 if the change would invalidate an existing function-type RV.
*/
BOOL CCxTrial::SetRVParam(int idx, int id, const CRVEntry& rv, BOOL& bSideEffect)
{
   // check for invalid index or invalid parameter ID
   if(idx<0 || idx>=MAX_TRIALRVS || id < 1 || id > 6) return(FALSE);

   // verify: (1) identified parameter is applicable to the RV's current type (unless type is being changed); (2) the 
   // new parameter value is itself valid (regardless the state of other parameters.
   BOOL bOk = FALSE;
   int t = rv.iType;
   double d;
   switch(id)
   {
   case 1: 
      bOk = (t >= RV_NOTUSED) && (t < RV_NUMTYPES); 
      break;
   case 2: 
      bOk = (t >= RV_UNIFORM) && (t <= RV_GAMMA) && (rv.iSeed >= 0); 
      break;
   case 3: 
      bOk = (t==RV_UNIFORM) || (t==RV_NORMAL) || ((t==RV_EXPON || t==RV_GAMMA) && rv.dParams[0] > 0); 
      break;
   case 4: 
      bOk = (t==RV_UNIFORM) || ((t==RV_NORMAL || t==RV_GAMMA) && rv.dParams[1] > 0) || 
         (t==RV_EXPON && rv.dParams[1] >= 3.0/m_Vars[idx].dParams[0]);
      break;
   case 5:
      d = m_Vars[idx].dParams[1]*(m_Vars[idx].dParams[0] + 3.0*::sqrt(m_Vars[idx].dParams[0]));
      bOk = (t==RV_NORMAL && rv.dParams[2] >= 3.0*m_Vars[idx].dParams[1]) || (t==RV_GAMMA && rv.dParams[2] >= d);
      break;
   case 6:
      bOk = (t==RV_FUNCTION);
      if(bOk)
      {
         // the function formula must be valid, and the function may not depend on itself, another
         // function-type RV, or an unused RV
         CFunctionParser fp(rv.strFunc);
         bOk = fp.IsValid();
         for(int i=0; bOk && i<MAX_TRIALRVS; i++) if(fp.HasVariableX(i))
         {
            bOk = (i != idx) && (m_Vars[i].iType != RV_NOTUSED) && (m_Vars[i].iType != RV_FUNCTION);
         }
      }
      break;
   }
   if(!bOk) return(FALSE);

   // if RV type is changing to RV_NOTUSED or RV_FUNCTION, make sure no existing function-type RV depends on it!
   if(id==1 && (rv.iType == RV_NOTUSED || rv.iType == RV_FUNCTION))
   {
      CFunctionParser fp("x1");
      for(int i=0; i<MAX_TRIALRVS; i++) if(i != idx && m_Vars[i].iType == RV_FUNCTION)
      {
         fp.SetDefinition(m_Vars[i].strFunc);
         if(fp.HasVariableX(idx)) return(FALSE);
      }
   }

   // change the parameter value, then auto-correct any other parameter values affected by the change
   CRVEntry* pRV = &(m_Vars[idx]);
   bSideEffect = FALSE;
   if(id==1)
   {
      pRV->iType = rv.iType;
      switch(pRV->iType)
      {
      case RV_NOTUSED:
      case RV_FUNCTION:
         pRV->iSeed = 0; 
         pRV->dParams[0] = pRV->dParams[1] = pRV->dParams[2] = 0;
         pRV->strFunc = pRV->iType==RV_NOTUSED ? _T("") : _T("20.0");
         break;
      case RV_UNIFORM:
         if(pRV->dParams[1] <= pRV->dParams[0]) pRV->dParams[1] = pRV->dParams[0] + 1.0;
         pRV->dParams[2] = 0;
         pRV->strFunc = _T("");
         break;
      case RV_NORMAL:
         if(pRV->dParams[1] <= 0) pRV->dParams[1] = 1.0;
         if(pRV->dParams[2] < 3.0*pRV->dParams[1]) pRV->dParams[2] = 3.0*pRV->dParams[1];
         pRV->strFunc = _T("");
         break;
      case RV_EXPON:
         if(pRV->dParams[0] <= 0) pRV->dParams[0] = 1.0;
         if(pRV->dParams[1] < 3.0/pRV->dParams[0]) pRV->dParams[1] = 3.0/pRV->dParams[0];
         pRV->dParams[2] = 0;
         pRV->strFunc = _T("");
         break;
      case RV_GAMMA:
         if(pRV->dParams[0] <= 0) pRV->dParams[0] = 1.0;
         if(pRV->dParams[1] <= 0) pRV->dParams[1] = 1.0;
         d = pRV->dParams[1]*(pRV->dParams[0] + 3.0*::sqrt(pRV->dParams[0]));
         if(pRV->dParams[2] < d) pRV->dParams[2] = d;
         pRV->strFunc = _T("");
         break;
      }
      bSideEffect = TRUE;  // there's always a "side effect" when the type changes
   }
   else if(id==2)
      pRV->iSeed = rv.iSeed;
   else if(id==3)
   {
      pRV->dParams[0] = rv.dParams[0];
      t = pRV->iType;
      if(t==RV_UNIFORM && pRV->dParams[1] <= pRV->dParams[0])
      {
         // uniform(A,B): A < B
         pRV->dParams[1] = pRV->dParams[0] + 1.0;
         bSideEffect = TRUE;
      }
      else if(t==RV_EXPON && pRV->dParams[1] < 3.0/pRV->dParams[0])
      {
         // expon(L): max cutoff >= 3.0/L
         pRV->dParams[1] = 3.0/pRV->dParams[0];
         bSideEffect = TRUE;
      }
      else if(t==RV_GAMMA)
      {
         // gamma(K,T): max cutoff >= T*(K + 3*sqrt(K))
         d = pRV->dParams[1] * (pRV->dParams[0] + 3.0*::sqrt(pRV->dParams[0]));
         if(pRV->dParams[2] < d) 
         {
            pRV->dParams[2] = d;
            bSideEffect = TRUE;
         }
      }
   }
   else if(id==4)
   {
      pRV->dParams[1] = rv.dParams[1];
      t = pRV->iType;
      if(t==RV_UNIFORM && pRV->dParams[1] <= pRV->dParams[0])
      {
         // uniform(A,B): A < B
         pRV->dParams[0] = pRV->dParams[1] - 1.0;
         bSideEffect = TRUE;
      }
      else if(t==RV_NORMAL && pRV->dParams[2] < 3.0*pRV->dParams[1])
      {
         // normal(M,S): max spread >= 3.0*S
         pRV->dParams[2] = 3.0*pRV->dParams[1];
         bSideEffect = TRUE;
      }
      else if(t==RV_GAMMA)
      {
         // gamma(K,T): max cutoff >= T*(K + 3*sqrt(K))
         d = pRV->dParams[1] * (pRV->dParams[0] + 3.0*::sqrt(pRV->dParams[0]));
         if(pRV->dParams[2] < d) 
         {
            pRV->dParams[2] = d;
            bSideEffect = TRUE;
         }
      }
   }
   else if(id==5)
      pRV->dParams[2] = rv.dParams[2];
   else if(id==6)
      pRV->strFunc = rv.strFunc;

   return(TRUE);
}

/**
 Initalize or update the runtime state of any defined random variables in this trial.

 This method must be called once with bInit==TRUE prior to starting a trial sequence that includes this trial. If the
 trial contains any defined random variables, the method will prepare an internal runtime state object (independent of
 RV's definition) for each such RV. Prior to each presentation of this trial, the method is called with bInit==FALSE
 to generate the next value ("variate") for each random variable. In the latter case, only the internal runtime state
 objects are accessed -- so you cannot update trial random variables while sequencing trials (although you can change
 the assignment of RVs to segment table parameters, which could lead to some undefined behavior!).

 It is possible that a function-type RV cannot be evaluated -- e.g., 1/x0 is undefined if x0==0. Whenever this 
 happens, the method will return FALSE and set the error message accordingly. Trial sequencing should STOP in this
 event. Users must take care to define any RVs so that such errors do not occur.

 @param bInit [in] If TRUE, internal runtime state information is initialized for the trial's random variables. Else,
 new values are generated for the RVs based on existing runtime state information.
 @param strMsg [out] If method fails for any reason, this will contain a brief error message.
 @return TRUE if successful, FALSE if an error occurred. 
*/
BOOL CCxTrial::UpdateRVs(BOOL bInit, CString& errMsg)
{
   errMsg = _T("");
   if(bInit)
   {
      ClearRVRuntimeState();

      // use system time to seed a uniform RNG which, in turn, provides a random seed for any active trial RV that has
      // an initial seed of 0. This ensures that such RVs generate a different sequence of variates for each trial seq.
      time_t now = time(NULL);
      LONGLONG llSecs = Int32x32To64(now, 10000000) + 116444736000000000;
      CCxRandomVar seedGen(CCxRandomVar::UNIFORM, int(0x0FFFFFFFF & (llSecs >> 16)), 1000, 2147483647, 0);
      int n = cMath::abs(int(seedGen.Get())) % 10;
      for(int i=0; i<n; i++) seedGen.Get();

      for(int i=0; i<MAX_TRIALRVS; i++) if(m_Vars[i].iType != RV_NOTUSED)
      {
         if(m_Vars[i].iType == RV_FUNCTION)
         {
            m_VarState[i].pFunc = new CFunctionParser(m_Vars[i].strFunc);
            m_VarState[i].dCurrVal = 0;
         }
         else
         {
            CCxRandomVar::RVType t = CCxRandomVar::UNIFORM;
            if(m_Vars[i].iType == RV_NORMAL) t = CCxRandomVar::GAUSSIAN;
            else if(m_Vars[i].iType == RV_EXPON) t = CCxRandomVar::EXPONENTIAL;
            else if(m_Vars[i].iType == RV_GAMMA) t = CCxRandomVar::GAMMA;
            
            // if initial seed is zero, need to generate a random seed!
            int iSeed = m_Vars[i].iSeed;
            if(iSeed == 0) iSeed = int(seedGen.Get());

            m_VarState[i].pRV = new CCxRandomVar(t, iSeed, m_Vars[i].dParams[0], 
                  m_Vars[i].dParams[1], m_Vars[i].dParams[2]);
            m_VarState[i].dCurrVal = 0;
         }
      }
   }
   else
   {
      // first-pass: get next value for each distributed RV
      double vals[MAX_TRIALRVS];
      for(int i=0; i<MAX_TRIALRVS; i++) 
      {
         vals[i] = 0;
         CRVState* pState = &(m_VarState[i]);
         if(pState->pRV != NULL)
            vals[i] = pState->dCurrVal = pState->pRV->Get();
      }

      // second pass: get next value for each function RV, which can only be a constant or a function of distributed 
      // RVs (a function-type RV cannot depend on another function-type RV). FAIL if function cannot be evaluated.
      BOOL bOk = TRUE;
      for(int i=0; i<MAX_TRIALRVS; i++) if(m_VarState[i].pFunc != NULL)
      {
         CRVState* pState = &(m_VarState[i]);
         vals[i] = pState->dCurrVal = pState->pFunc->Evaluate(&(vals[0]), bOk);
         if(!bOk)
         {
            errMsg.Format(_T("Trial %s : Function %s cannot be evaluated for the current values of trial's RVs!"), 
               Name(), pState->pFunc->GetDefinition());
            return(FALSE);
         }
      }
   }

   // final check: make sure that, for any segment table parameter assigned to an RV, that RV is in use. 
   for(int s=0; s<SegCount(); s++)
   {
      if(IsRVAssignedToSegParam(s, -1, MINDURATION))
      {
         if(!IsRVInUse(GetSegParamAsInt(s, -1, MINDURATION)))
         {
            errMsg.Format(_T("Trial %s : Min duration of segment %d is governed by an undefined RV!"), Name(), s);
            return(FALSE);
         }
      }

      if(IsRVAssignedToSegParam(s, -1, MAXDURATION))
      {
         if(!IsRVInUse(GetSegParamAsInt(s, -1, MAXDURATION)))
         {
            errMsg.Format(_T("Trial %s : Max duration of segment %d is governed by an undefined RV!"), Name(), s);
            return(FALSE);
         }
      }

      for(int t=0; t<TargCount(); t++)
      {
         for(int p=TGTHPOS; p<=PATVACC; p++) if(IsRVAssignedToSegParam(s, t, (ParamID)p))
         {
            if(!IsRVInUse(GetSegParamAsInt(s, t, (ParamID)p)))
            {
               errMsg.Format(
                  _T("Trial %s : A trajectory parameter for tgt %d in seg %d is assigned to an undefined RV!"), 
                  Name(), s, t);
               return(FALSE);
            }
         }
      }
   }
   return(TRUE);
}

/**
 Get the current value for the minimum duration of the specified segment. If it is assigned a constant value, that
 value is returned. However, if it has been assigned to one of the trial's random variables, the method returns the
 current value for that RV.

 Be sure to call this method rather than GetMinDuration() when preparing to present the trial. When editing the
 parameter in the trial definition form, use Get/SetMinDuration().

 @param s The segment index.
 @return The minimum duration in ms. Will return 0 if segment index is invalid or if this parameter is currently
 assigned to an RV that is undefined (type RV_NOTUSED).
*/
int CCxTrial::GetCurrMinDuration(int s) const
{
   if(!IsValidSeg(s)) return(0);
   int dur = GetSegParamAsInt(s, -1, MINDURATION);
   if(IsRVAssignedToSegParam(s, -1, MINDURATION))
   {
      dur = cMath::rangeLimit((int) m_VarState[dur].dCurrVal, 0, 32000);
   }
   return(dur);
}

/**
 Get the current value for the maximum duration of the specified segment. If it is assigned a constant value, that
 value is returned. However, if it has been assigned to one of the trial's random variables, the method returns the
 current value for that RV.

 Be sure to call this method rather than GetMaxDuration() when preparing to present the trial. When editing the
 parameter in the trial's definition form, use Get/SetMaxDuration().

 @param s The segment index.
 @return The maximum duration in ms. Will return 0 if segment index is invalid or if this parameter is currently
 assigned to an RV that is undefined (type RV_NOTUSED).
*/
int CCxTrial::GetCurrMaxDuration(int s) const
{
   if(!IsValidSeg(s)) return(0);
   int dur = GetSegParamAsInt(s, -1, MAXDURATION);
   if(IsRVAssignedToSegParam(s, -1, MAXDURATION))
   {
      dur = cMath::rangeLimit((int) m_VarState[dur].dCurrVal, 0, 32000);
   }
   return(dur);
}

/**
 Get the approximate worst-case duration for a trial segment.

 The segment duration will vary across repeated presentations of the same trial in two circumstances: (1) When minimum
 and maximum duration are set to constant values such that min < max; and (2) when either or both parameters are governed 
 by a random variable. When the parameters are constant values, the worst-case duration is simply the specified maximum 
 duration. However, when assigned to an RV, the worst-case duration depends on the RV's distribution. The method uses 
 the distribution's max cutoff as the worst-case duration. If the RV is a function-type, it uses the max cutoffs for all 
 RVs on which the function RV depends -- which may not give a reasonable worst-case duration.

 This method is provided because Maestro's spike histogram facility uses the worst-case duration for each segment to
 prepare its histogram bins before trial sequencing starts.

 @param s The segment index.
 @return The worst-case segment duration, as described. Returns 0 if segment index is invalid.
*/
int CCxTrial::GetWorstCaseDuration(int s) const
{
   if(!IsValidSeg(s)) return(0);
   
   int dur = GetMaxDuration(s);
   if(dur >= 0) return(dur);

   // segment duration is governed by an RV. Use RV's max cutoff
   int idx = cMath::abs(dur) - 1;
   switch(m_Vars[idx].iType)
   {
      case RV_UNIFORM : 
      case RV_EXPON :
         dur = (int) m_Vars[idx].dParams[1];
         break;
      case RV_NORMAL :
      case RV_GAMMA :
         dur = (int) m_Vars[idx].dParams[2];
         break;
      case RV_FUNCTION :
         {
            // evaluate function when each RV upon which it depends is set to its max cutoff. By design, each 
            // independent RV must be one of the 4 distribution-type RVs...
            CFunctionParser fp(m_Vars[idx].strFunc);
            double vals[MAX_TRIALRVS];
            for(int i=0; i<MAX_TRIALRVS; i++)
            {
               if(fp.HasVariableX(i))
               {
                  if(m_Vars[i].iType == RV_UNIFORM || m_Vars[i].iType == RV_EXPON) vals[i] = m_Vars[i].dParams[1];
                  else vals[i] = m_Vars[i].dParams[2];
               }
               else
                  vals[i] = 0;
            }

            // if function cannot be evaluated, return zero duration
            BOOL bOk = TRUE;
            dur = (int) fp.Evaluate( &(vals[0]), bOk);
            if(!bOk) dur = 0;
         }
         break;
      default : // RV_NOTUSED
         dur = 0;
         break;
   }

   return(dur < 0 ? 0 : dur);
}

/**
 Get the current value for the specified target trajectory parameter during the specified segment. If it is assigned a 
 constant value, that value is returned. However, if it has been assigned to one of the trial's random variables, the 
 method returns the current value for that RV.

 Be sure to call this method rather than GetTgtTrajParam() when preparing to present the trial. When editing the
 parameter in the trial's definition form, use Get/SetTgtTrajParam().

 @param s The segment index.
 @param t The target index.
 @param p The trajectory parameter ID. Must lie in [TGTHPOS .. PATVACC].
 @return The requested target trajectory parameter. Units depend on the parameter: visual degrees for position, 
 deg/sec for velocity, and deg/(sec^2) for acceleration. Will return 0 if segment index, target index, or parameter ID 
 is invalid or if this parameter is currently assigned to an RV that is undefined (type RV_NOTUSED).
*/
double CCxTrial::GetCurrTgtTrajParam(int s, int t, ParamID p) const
{
   double out = 0.0;
   if(IsValidTrajRecord(s, t) && p >= TGTHPOS && p <= PATVACC)
   {
      BOOL isRV = FALSE;
      out = GetTgtTrajParam(s, t, p, isRV);
      if(isRV)
      {
         int idx = (int) out;

         double lim = SGTJ_POSMAX;
         if(p==TGTHVEL || p==TGTVVEL || p==PATHVEL || p==PATVVEL) lim = SGTJ_VELMAX;
         else if(p==TGTHACC || p==TGTVACC || p==PATHACC || p==PATVACC) lim = SGTJ_ACCMAX;
         out = cMath::rangeLimit(m_VarState[idx].dCurrVal, -lim, lim);
      }
   }
   return(out);
}

/**
 Clear the trial's random variable list. All RV types are reset to RV_NOTUSED. 
*/
VOID CCxTrial::RemoveAllRVs()
{
   for(int i=0; i<MAX_TRIALRVS; i++)
   {
      m_Vars[i].iType = RV_NOTUSED;
      m_Vars[i].iSeed = 0;
      for(int j=0; j<3; j++) m_Vars[i].dParams[j] = 0.0;
      m_Vars[i].strFunc = _T("");
   }
}

/**
 Destroy any runtime state information allocated to generate random variates during trial sequencing. Be sure to
 call this method when the CCxTrial object is destroyed.
*/
VOID CCxTrial::ClearRVRuntimeState()
{
   for(int i=0; i<MAX_TRIALRVS; i++)
   {
      CRVState* pState = &(m_VarState[i]);
      if(pState->pFunc != (CFunctionParser*)NULL) 
      {
         delete pState->pFunc;
         pState->pFunc = (CFunctionParser*)NULL;
      }
      else if(pState->pRV != (CCxRandomVar*)NULL)
      {
         delete pState->pRV;
         pState->pRV = (CCxRandomVar*)NULL;
      }
      pState->dCurrVal = 0;
   }
}

//=====================================================================================================================
// OPERATIONS - Random Reward Withholding (during trial sequencing)
//=====================================================================================================================

/**
 Initialize runtime state to implement random withholding of reward pulse #1 and/or #2. This method must be called
 just prior to starting a trial sequence (with both arguments TRUE). It also may be called to reshuffle the withholding
 pattern for either reward #1 or #2 (by setting only one of the arguments TRUE).

 No action if random withholding is not enabled, ie, if the numerator for the withholding variable ratio is 0.

 @param initRew1 [in] TRUE to initialize runtime state for random withholding of reward pulse #1.
 @param initRew2 [in] TRUE to initialize runtime state for random withholding of reward pulse #2.
*/
VOID CCxTrial::InitRewardWHVR(const BOOL initRew1 /* = TRUE */, const BOOL initRew2 /* = TRUE */)
{
   if(m_hdr.reward1[1] > 0 && initRew1)
   {
      // initialize shuffle list with N zeros and N-D ones
      m_rew1WHVRShuffleList.RemoveAll();
      int n = m_hdr.reward1[1], d = m_hdr.reward1[2];
      for(int i = 0; i < d; i++) m_rew1WHVRShuffleList.AddTail(i < n ? 0 : 1);

      // then shuffle it
      for(int i = 0; i < d - 1; i++)
      {
         int iPick = cMath::rangeLimit(i + ((rand() * (d - i)) / RAND_MAX), 0, d - 1);

         POSITION pos = m_rew1WHVRShuffleList.FindIndex(iPick);
         int val = m_rew1WHVRShuffleList.GetAt(pos);
         m_rew1WHVRShuffleList.RemoveAt(pos);
         m_rew1WHVRShuffleList.AddHead(val);
      }
   }

   if(m_hdr.reward2[1] > 0 && initRew2)
   {
      m_rew2WHVRShuffleList.RemoveAll();
      int n = m_hdr.reward2[1], d = m_hdr.reward2[2];
      for(int i = 0; i < d; i++) m_rew2WHVRShuffleList.AddTail(i < n ? 0 : 1);

      for(int i = 0; i < d - 1; i++)
      {
         int iPick = cMath::rangeLimit(i + ((rand() * (d - i)) / RAND_MAX), 0, d - 1);

         POSITION pos = m_rew2WHVRShuffleList.FindIndex(iPick);
         int val = m_rew2WHVRShuffleList.GetAt(pos);
         m_rew2WHVRShuffleList.RemoveAt(pos);
         m_rew2WHVRShuffleList.AddHead(val);
      }
   }
}

/**
 Update runtime state for random reward withholding and "decide" whether or not reward pulse #1 and/or #2 should be
 delivered during the next presentation of this trial. The method must be called just prior to presenting this trial
 during a trial sequence in progress.
 @param bGiveRew1 [out] True if reward pulse #1 should be given, false if it should be withheld.
 @param bGiveRew2 [out] True if reward pulse #2 should be given, false if it should be withheld.
*/
VOID CCxTrial::UpdateRewardWHVR(BOOL& bGiveRew1, BOOL& bGiveRew2)
{
   bGiveRew1 = TRUE;
   if(m_hdr.reward1[1] > 0)
   {
      if(m_rew1WHVRShuffleList.IsEmpty()) InitRewardWHVR(TRUE, FALSE);

      int val = m_rew1WHVRShuffleList.RemoveHead();
      bGiveRew1 = (val != 0);
   }

   bGiveRew2 = TRUE;
   if(m_hdr.reward2[1] > 0)
   {
      if(m_rew2WHVRShuffleList.IsEmpty()) InitRewardWHVR(FALSE, TRUE);

      int val = m_rew2WHVRShuffleList.RemoveHead();
      bGiveRew2 = (val != 0);
   }
}


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
#ifdef _DEBUG

//=== Dump [base override] ============================================================================================
//
//    Dump contents of the trial object in an easy-to-read form to the supplied dump context.  Intelligent dump is
//    tailored to the specific contents of trial.  To see a detailed dump of the participating target keys, all the
//    segment trajectory info and the perturbation list, the dump depth must be > 0.
//
//    ARGS:       dc -- [in] the current dump context.
//
//    RETURNS:    NONE.
//
void CCxTrial::Dump( CDumpContext& dc ) const
{
   CTreeObj::Dump( dc );

   dc << "********MAESTRO Trial Object********\n\nTrial Header Info:";

   CString msg;
   msg.Format( "\nFlags = 0x%08x, Wt = %d, Stair# = %d", m_hdr.dwFlags, m_hdr.iWeight, m_hdr.iStairNum );
   dc << msg;

   msg.Format( "\nFirst save seg = %d, failsafe seg = %d, special seg = %d, special op = %d", 
               m_hdr.iStartSeg, m_hdr.iFailsafeSeg, m_hdr.iSpecialSeg, m_hdr.iSpecialOp );
   dc << msg;

   msg.Format( "\nSacc Vt = %d deg/sec", m_hdr.iSaccVt );
   dc << msg;
   
   msg.Format("\nReward pulse 1: len = %d ms; WHVR = %d/%d. Reward pulse 2: len= %d ms; WHVR=%d/%d.", m_hdr.reward1[0],
      m_hdr.reward1[1], m_hdr.reward1[2], m_hdr.reward2[0], m_hdr.reward2[1], m_hdr.reward2[2]);

   msg.Format( "\nDisplay marker segments = %d, %d", m_hdr.iMarkSeg1, m_hdr.iMarkSeg2 );
   dc << msg;
   msg.Format( "\nMid-trial reward intv, len (ms) = %d, %d", m_hdr.iMTRIntv, m_hdr.iMTRLen);
   dc << msg;
   msg.Format( "\nChan cfg key = %d; stair strength = %.3f", m_hdr.wChanKey, m_hdr.fStairStrength );
   dc << msg;
   dc << "\n";
   dc << "\nParticipating target ID array:";
   m_wArTargs.Dump( dc );
   dc << "\n";
   dc << "\nTRIAL SEGMENTS:";
   m_Segments.Dump( dc );
   dc << "\n";

   if( dc.GetDepth() > 0 && m_nPerts > 0 )
   {
      dc << "Perturbation List: (key, amp, seg, tgt, velCmpt)\n";
      for( int i = 0; i < m_nPerts; i++ )
      {
         msg.Format( "0x%04x, %.2f, %d, %d, %d\n", m_Perts[i].wKey, m_Perts[i].fAmp, m_Perts[i].cSeg,
               m_Perts[i].cTgt, m_Perts[i].cIdCmpt );
         dc << msg;
      }
   }
   else
      dc << m_nPerts << " perturbations in use.";
   dc << "\n";

   // dump random variable list
   if(dc.GetDepth() > 0)
   {
      dc << "Random variable list: index -> type, p1, p2, p3, strFunc)\n";
      for(int i=0; i<MAX_TRIALRVS; i++)
      {
         msg.Format("%d -> %d, %.2f, %.2f, %.2f, %s\n", i, m_Vars[i].iType, m_Vars[i].dParams[0], 
            m_Vars[i].dParams[1], m_Vars[i].dParams[2], (LPCTSTR) m_Vars[i].strFunc);
         dc << msg;
      }
   }
   else 
   {
      int n=0; 
      for(int i=0; i<MAX_TRIALRVS; i++) if(m_Vars[i].iType != RV_NOTUSED) ++n;
      dc << n << " random variables in use.";
   }

   if( dc.GetDepth() > 0 && GetNumTaggedSections() > 0 )
   {
      dc << "Tagged Sections: (s0..s1 : tag)\n";
      for( int i = 0; i < GetNumTaggedSections(); i++ )
      {
         TRIALSECT sect;
         GetTaggedSection(i, sect);
         msg.Format( "%d..%d :%s\n", int(sect.cFirstSeg), int(sect.cLastSeg), sect.tag );
         dc << msg;
      }
   }
   else
      dc << GetNumTaggedSections() << " tagged sections defined on trial.";

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
void CCxTrial::AssertValid() const
{
   CTreeObj::AssertValid();                        // validate base class stuff
   ASSERT( m_type == CX_TRIAL );                   // this is the only CNTRLX obj type supported by this class
   m_wArTargs.AssertValid();                       // check target array and segment list containers
   m_Segments.AssertValid();                       //

   POSITION p = m_Segments.GetHeadPosition();      // traj rec count MUST = trial target count for every seg in trial!
   while( p != NULL )
   {
      CCxSegment* pSeg = m_Segments.GetNext( p );
      ASSERT( pSeg->TrajCount() == TargCount() );
   }
}

#endif //_DEBUG



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== AssignDefaultHeader =============================================================================================
//
//    Assign default values to the trial header.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxTrial::AssignDefaultHeader()
{
   m_hdr.dwFlags = THF_KEEP;                       // data saved; mid-trial rewards are periodic.
   m_hdr.iWeight = 1;                              // trial weight
   m_hdr.iStairNum = 0;                            // normal trial
   m_hdr.iStartSeg = 0;                            // no first save seg -- data is saved for the entire trial duration
   m_hdr.iFailsafeSeg = -1;                        // no failsafe seg -- if aborted by fixation break, trial not saved
   m_hdr.iSpecialSeg = 0;                          // no special op
   m_hdr.iSpecialOp = TH_SOP_NONE;
   m_hdr.iOpenSeg = -1;                            // OBSOLETE as of Maestro v2.0.0
   m_hdr.nOpenSegs = 1;                            // OBSOLETE as of Maestro v2.0.0
   m_hdr.iMarkSeg1 = -1;                           // no display marker segments designated
   m_hdr.iMarkSeg2 = -1;
   m_hdr.iMTRIntv = TH_DEFREWINTV;                 // mid-trial reward intv and len set to default values
   m_hdr.iMTRLen = TH_DEFREWLEN;
   m_hdr.iXYDotSeedAlt = -1;                       // [deprecated]
   m_hdr.nXYInterleave = 0;                        // [deprecated]
   m_hdr.iSaccVt = 100;                            // default saccade threshold is 100 deg/sec

   // reward pulse length and WHVR set to default values
   m_hdr.reward1[0] = m_hdr.reward2[0] = TH_DEFREWLEN;
   m_hdr.reward1[1] = m_hdr.reward2[1] = TH_DEFWHVR;
   m_hdr.reward1[2] = m_hdr.reward2[2] = TH_DEFWHVR + 1;

   m_hdr.wChanKey = CX_NULLOBJ_KEY;                // default channel set will be attached to this trial
   m_hdr.fStairStrength = (float)1.0;              // ignored since this is not a staircase trial
}


//=== IsSameHeader ====================================================================================================
//
//    Compare contents of current trial header with the one provided.
//
//    ARGS:       hdr -- [in] header to be compared with trial's current header.
//
//    RETURNS:    BOOL(current trial header == hdr).
//
BOOL CCxTrial::IsSameHeader( const TRLHDR& hdr ) const
{
   return( BOOL(0 == ::memcmp( &m_hdr, &hdr, sizeof( TRLHDR ) )) );
}

//
//=====================================================================================================================
// END:  class CCxTrial
//=====================================================================================================================

