//=====================================================================================================================
//
// cxobj_ifc.h : Maestro object-related defines that are shared among two or more Maestro classes.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// REVISION HISTORY:
// 30jul2000-- Created.  Purpose is to collect, in one place, those CNTRLX object-related constant and structure defs
//             which must be shared by two or more CNTRLX classes.
// 11aug2000-- Added CNTRLX target-related defines (formerly in ifc_target.h) to this file.
// 04jan2001-- Added CNTRLX object types for the "channels" tree and the "channel configurations" it will contain.
// 25jan2001-- Major design revision.  All CNTRLX data objects are now part of *one* "object tree".  CCxDoc initializes
//             this tree with several predefined branches, or subtrees -- one for each data class.  Thus, there's a
//             target subtree (CX_TARGBASE), trial subtree (CX_TRIALBASE), and so on.  Simplified hierarchy of the
//             target subtree:  CX_TARGBASE can contain CX_TARGSET's or any target obj type, and CX_TARGSET can contain
//             target obj type.  CCxDoc defines a "Predefined" CX_TARGSET that contains all of the predefined,
//             nonmodifiable targets.  There are no longer separate target "group" collections for the predefined, XY
//             scope, and FB video target categories.
// 06dec2001-- Got rid of include "treemap.h".  CX_NULLOBJ_KEY assigned value 0 instead of TM_NOKEY.  These changes
//             made so the file can be included in the compilation of CXDRIVER.
// 11dec2001-- MAJOR changes to structs containing trial info.  Eliminating support for perturbations (for now), adding
//             support for multiple staircase designations.  Also, replacing must CHAR and BYTE fields with 4-byte
//             entities.
// 18dec2001-- Added TRLHDR member 'nXYInterleave' to specify XY scope target interleaving in a trial.
// 27dec2001-- Added TRLHDR member 'iSaccVt' to specify the saccade threshold velocity to apply during trials that
//             contain a saccade-triggered "special operation".
// 15jan2002-- Modifications to TRLHDR and TRAJINFO to explicitly restrict the "open pursuit loop" feature to a single
//             designated segment during a trial.
// 06feb2002-- Adding new CNTRLX object type, CX_VIDEODSP, which represents the video display parameters for the XY
//             scope and framebuf video.  There exists only a single CX_VIDEODSP object in each CNTRLX experiment doc,
//             and it appears as a child of the object tree's root.  It is a predefined but modifiable object.
// 14feb2002-- Changing representation of XY target subtype RECTDOT to be more like that in the original cntrlxPC.
//             Instead of having a standard "bounding rect" like many other target types, it is specified by a width
//             and a dot spacing, both in deg.  See comments to XYPARMS struct.
// 15feb2002-- Added more recently introduced XY scope target types FLOWFIELD and ORIENTEDBAR.  Both of these have
//             somewhat unique parameters compared to the other target types.  See comments to XYPARMS.
// 10may2002-- Introducing constants, data structs related to continuous-mode XSTIM runs and channels.
// 20sep2002-- Added constant THF_SELBYFIX2 for new sacc-trig'd operation, "SelectByFix2".
// 18oct2002-- Eliminated CNTRLX obj type CX_VIDEODSP (see entry dtd 06feb2002).  The video display parameters are now
//             part of the CCxSettings object, which is stored in CCxDoc but is NOT part of the object tree.
//          -- Introduced per-segment VERTICAL fixation accuracy and per-segment "mid-trial" rewards to trial defn.
// 04nov2002-- Introduced "perturbation waveform" object type CX_PERTURB.
// 21jul2003-- Introduced new XY scope target type NOISYDOTS.
// 23oct2003-- Velocity stabilization ("opening the pursuit loop") feature for trials is now independently enabled for
//             the horizontal and vertical components.  Added trial flags THF_OPEN***.
// 04nov2003-- Modified to permit marker pulses on DO<10..1> instead of just DO<6..1>.  DO<11> is reserved as a
//             dedicated record "start/stop" pulse.  DO<0> is not used since DI<0> is reserved for timestamping spike
//             arrival times based on input from a window discriminator.
// 20sep2004-- Modified TRLHDR to include two additional segment indices, 'iMarkSeg1' and 'iMarkSeg2'.  When the
//             trial data is displayed in Maestro's data trace display, vertical markers are drawn at the start of each
//             of these segments (unless they're set to -1).  These marker may, for example, help direct the user's
//             attention to a particular section in the trace display.  See CCxTrial.
// 13oct2004-- Introduced new XY scope target type COHERENTFC.
// 03nov2004-- Added flag for new saccade-trig'd special operation during a trial, THF_DUALFIX.  See definitions for
//             TRLHDR.
// 25jan2005-- Renamed THF_DUALFIX as "THF_SWITCHFIX", because the goal of the feature is to encourage the animal to
//             switch from one fixation tgt to the other after the special segment.  Feature's implementation is being
//             updated accordingly.
//          -- Added flag and two integer fields to TRLHDR to enhance the "mid-trial rewards" feature.
// 30mar2005-- Added constants/structures in support of intra-trial tagged sections.
// 11apr2005-- Modified comments of XYPARMS struct to note use of XYPARMS.fInnerH as update interval for NOISYDOTS tgt;
//             added constants MIN_DOTDIRUPD and MAX_DOTDIRUPD.
// 28jul2005-- Modified PERT struct and added constants for new perturbation type, Gaussian-distributed noise.  Also
//             modified the existing uniform noise perturbation (NOISEPERT) to include a seed for the underlying random
//             number generator.  With commensurate changes to CXDRIVER, both noise perturbation types should be
//             reproducible offline.
// 24oct2005-- Added field iXYDotSeedAlt to TRLHDR.  Supports overriding display settings re: XY target dot seed on a
//             per-trial basis.
// 29nov2005-- Added new special operation, THF_RPDISTRO, as part of modifications to support a distribution-based
//             contingency protocol.  In the first phase of this protocol, the subject's behavioral response (eye
//             velocity magnitude) is measured during the special segment in repeated trial presentations.  A response
//             distribution is constructed, and the user specifies "reward windows" relative to this distribution.  In
//             the second phase, the trial(s) is presented again, and the subject is given a "double reward" if their
//             response falls inside a reward window, and a penalty (or reduced reward) otherwise.
// 07jan2006-- Mods to introduce new XY scope target NOISYSPEED, a variation on NOISYDIR (formerly, NOISYDOTS) in which
//             the noise is in dot speed rather than dot direction.
// 13mar2006-- Added field TRLHDR.nOpenSegs to support velocity stabilization over a contiguous span of trial segments.
// 22mar2006-- For Maestro v2.0.0, the VSG framebuffer will be replaced by the RMVideo server, which runs on a separate
//             machine.  CXTARGET redefined to use RMVTGTDEF instead of FBPARMS as the target definition for a
//             "framebuffer" target.  FBPARMS and related constants are still defined to assist in document migration
//             and backwards-compatibility issues.  Will use CX_RMVTARG==CX_FBTARG as the ID for an RMVideo target.
// 12apr2006-- Added SGTJF_* flags to support velocity stabilization on a per-segment, per-target basis.  OBSOLETE
//             TRLHDR fields iOpenSeg and nOpenSegs were left in place so it is easier to migrate existing Maestro
//             documents.  Effective Maestro v2.0.0.
// 19may2006-- Added TRH_* flags to support ignoring the global pos/vel scale and rotate transformations on a per-trial 
//             basis.  Effective Maestro v2.0.0.
// 19jun2006-- Added fields fInnerX, fInnerY to XYPARMS.  No effect on size of union U_TGPARMS, size XYPARMS is still 
//             smaller than the other cmpt, RMVTARGET (and its predecessor, FBTARGET).  Of course, these fields must 
//             be ignored when reading target info from Maestro data files created prior to this change.  Effective 
//             Maestro v2.0.1.
// 23jul2006-- Increased MAX_SPEEDOFFSET, the upper range limit for speed noise %, from 100 to 300. This change should 
//             not impact READCXDATA or XWORK.
// 03jan2007-- Added TRLHDR.iSpecialOp to specify the special operation in effect. Relevant THF_** flags rendered 
//             obsolete. The supported special operations are enumerated as TH_*** constants. Added two new special 
//             ops: TH_SOP_CHOOSEFIX1 and TH_SOP_CHOOSEFIX2. Effective Maestro v2.0.5.
// 27feb2007-- Mods to support pattern acceleration on a seg-by-seg basis during a trial. Added fields fPatAccH and 
//             fPatAccV to TRAJINFO. Effective Maestro v2.1.0.
// 25apr2007-- Revised some SGM-related constants and fixed comments to reflect changes to PSGM spec. Also added 
//             TH_RPD_*** constants enumerating the 4 response measure types for RP Distro feature. Eff. v2.1.1.
// 16jul2007-- Added PERT_ON_SWIN, _SPAT for perturbing the amplitude -- ie, "speed" -- of a trial target's window or 
//             pattern velocity vector without altering the direction of motion. Effective Maestro v2.1.2.
// 29aug2007-- Added PERT_ON_DIR, _SPD for simultaneously and identically perturbing both a trial target's window and 
//             pattern velocity vector. PERT_ON_DIR is for perturbing vector direction, _SPD for vector amplitude.
//             PERT_NCMPTS adjusted accordingly. Effective Maestro v2.1.3.
// 31aug2007-- Added ***_SPDLOG2 and comments to XYPARM toward supporting an alternative algorithm for generating 
//             speed noise on the NOISYSPEED XYScope target: Vdot = Vpat*2^x, x chosen from [-N..N].
// 08sep2009-- Effective version 2.5.0, CXTARGET is different because of changes in RMVTGTDEF to support the new target
//             type RMV_MOVIE. Added U_TGPARMS_V12 to support document schema migration and parsing of data files 
//             generated prior to these changes.
// 01feb2011-- Added new special op TH_SOP_SEARCH: A "search task" in which subject is expected to search for and 
//             fixate on a designated target among 1 or more distractor targets during the special segment. Effective
//             Maestro v2.6.5.
// 21sep2011-- Added comments indicating that targets CX_FIBER* and CX_REDLED* are no longer supported, effective v3.0.
// 02oct2013-- Added special flags THF_CHAINED, THF_CHAINSTART -- for new "Chained" trial sequencer mode. Effec. v3.1.
// 01dec2014-- Added new object type: CX_TRIALSUBSET -- a subset of trials within a trial set. Effec. v3.1.2.
// 24nov2015-- Added constants and structures to support Eyelink eye tracker. Effec. v3.2.0.
// Fall 2016-- Added constants related to "trial random variables". Effec. v3.3.0.
// 20sep2018-- Added flag SEGHDR.bEnaRMVSync. If TRUE, a time sync flash is presented in the top-left corner of the RMV
// display during the first video frame marking the start of the relevant segment. Size of the box flashed, the dark
// margin around it, and the flash duration are new application settings.
// 07may2019-- Effective version 4.1.0, CXTARGET is different because of changes in RMVTGTDEF to support a "flicker"
// feature for all RMVideo target types. Added U_TGPARMS_V22 to support document schema migration and parsing of data
// files generated prior to these changes.
// 15may2019-- Introducing new feature in trials: reward withholding variable ratio for both reward pulses. In TRLHDR,
// fields iRewLen, iRewLen2 replaced by 3-int arrays reward1[] and reward2[]. Each array holds, in order: the reward
// pulse len in ms, the numerator N for the variable ratio, and the denominator D for the variable ratio.  0<=N<D.
// 26sep2024-- Added comments to indicate that the XYScope platform is deprecated. XYScope-specific typedefs and
// constants must remain to handle migration of existing experiment documents, and analysis programs which must read
// data files generated by any release of Maestro.
//=====================================================================================================================


#if !defined(CXOBJ_IFC_H__INCLUDED_)
#define CXOBJ_IFC_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rmvideo_common.h"         // for RMVideo target-related definitions


//=====================================================================================================================
// GENERAL MAESTRO OBJECT DEFINITIONS
//=====================================================================================================================

//=====================================================================================================================
// MAESTRO Object Types (P = "predefined"; U = user-defined; C = collection obj; D = data obj)
//
// !!!IMPORTANT!!! We do rely on the order of these constants!
//=====================================================================================================================
const WORD     CX_ROOT              = 0x0010;         // [P,C] the root of the MAESTRO object tree

const WORD     CX_TRIALBASE         = 0x0011;         // [P,C] base of the trial subtree (immediate child of CX_ROOT)
const WORD     CX_TRIALSET          = 0x0012;         // [U,C] a set of individual trials
const WORD     CX_TRIAL             = 0x0013;         // [U,D] an individual trial object

const WORD     CX_TARGBASE          = 0x0014;         // [P,C] base of the target subtree (immediate child of CX_ROOT)
const WORD     CX_TARGSET           = 0x0015;         // [PU,C] a set of individual targets
const WORD     CX_CHAIR             = 0x0016;         // [P,D] the servo-controlled rotating animal chair
// as of Maestro 3, CX_FIBER* and CX_REDLED* are no longer supported. As of Maestro 4, CX_XYTARG is no longer supported.
// The constants remain defined so that the current Maestro release can read in and migrate experiment documents 
// generated by older releases.
const WORD     CX_FIBER1            = 0x0017;         // [P,D] spot tgts proj. on translucent screen; shuttered
const WORD     CX_FIBER2            = 0x0018;         //    fiber optic spots, pos cntrl'd by mirror galvonometers.
const WORD     CX_REDLED1           = 0x0019;         // [P,D] on-off, immovable spots projected on translucent
const WORD     CX_REDLED2           = 0x001A;         //    screen, using shuttered LEDs
const WORD     CX_OKNDRUM           = 0x001B;         // [P,D] NO LONGER SUPPORTED AS OF VERSION 1.5.0
const WORD     CX_XYTARG            = 0x001C;         // [U,D] an individual XY scope target -- UNSUPPORTED a/o V4.0!
const WORD     CX_FBTARG            = 0x001D;         // [U,D] an individual FB video target -- OBSOLETE as of V2.0!
const WORD     CX_RMVTARG           = CX_FBTARG;      // [U,D] an individual RMVideo target

const WORD     CX_CHANBASE          = 0x001E;         // [P,C] base of the "channel configurations" subtree
const WORD     CX_CHANCFG           = 0x001F;         // [PU,D] a channel configuration

const WORD     CX_CONTRUNBASE       = 0x0020;         // [P,C] base of ContMode run subtree (immed child of CX_ROOT)
const WORD     CX_CONTRUNSET        = 0x0021;         // [U,C] a set of continuous runs
const WORD     CX_CONTRUN           = 0x0022;         // [U,D] an individual continuous run object

const WORD     CX_PERTBASE          = 0x0023;         // [P,C] base of perturbation subtree (immed child of CX_ROOT)
const WORD     CX_PERTURB           = 0x0024;         // [U,D] an individual perturbation waveform defn

const WORD     CX_TRIALSUBSET       = 0x0025;         // [U,C] a subset of individual trials (child of CX_TRIALSET)

const WORD     CX_FIRST_TYP         = CX_ROOT;        // for validating object type -- note contiguous range!
const WORD     CX_LAST_TYP          = CX_TRIALSUBSET;

const WORD     CX_FIRST_TARG        = CX_CHAIR;       // for validating a target object type
const WORD     CX_LAST_TARG         = CX_RMVTARG;


//=====================================================================================================================
// MAESTRO Object State Flags
//=====================================================================================================================
const WORD     CX_ISPREDEF          = 0x0001;         // this object is predefined; cannot be copied, removed, renamed
const WORD     CX_NOINSERT          = 0x0002;         // cannot add children to this predef *collection* object
const WORD     CX_OBJFLAGS          = 0x0003;         // for masking these flags
const WORD     CX_ISSETOBJ          = 0x0004;         // this flag set for all MAESTRO collection objects

const WORD     CX_NULLOBJ_KEY       = 0;              // reserved key indicates failure to insert or find object
const int      CX_MAXOBJNAMELEN     = 50;             // maximum name length for any data obj defined in MAESTRO



//=====================================================================================================================
// MAESTRO TARGET-SPECIFIC DEFINITIONS
//=====================================================================================================================

//=====================================================================================================================
// Parameter set for XY scope targets, and related constants
// 
// DEPRECATED!  As of Maestro 4.0, the XYScope platform is no longer supported, and a/o V5.0, XYScope-specific code has
// been mostly excised from Maestro and CXDRIVER. However, we must maintain these old definitions to support document 
// schema migration and analysis programs which must read in both old and new Maestro data files.
//
//=====================================================================================================================
const int NUMXYTYPES       = 11;       // XY scope target types:
const int RECTDOT          = 0;        //    rectangular dot array
const int CENTER           = 1;        //    full-screen random-dot pattern visible only inside defined rectangle
const int SURROUND         = 2;        //    full-screen random-dot pattern visible only outside defined rectangle
const int RECTANNU         = 3;        //    full-screen random-dot pattern visible only inside rectangular annulus
const int FASTCENTER       = 4;        //    rectangular random-dot pattern; optimized version of center draws all dots
                                       //       inside rect rather than distributing dot pos over entire screen
const int FCDOTLIFE        = 5;        //    same as FASTCENTER, but each dot is assigned a random "lifetime".  when
                                       //       life expires or dot hits border, its randomly repositioned within rect
                                       //       and its dot life is reset
const int FLOWFIELD        = 6;        //    optic flow field.  very different from other tgt types.
const int ORIENTEDBAR      = 7;        //    rect bar or line oriented at any angle in [0..360) deg.
const int NOISYDIR         = 8;        //    same as FCDOTLIFE, but each dot's direction vector is offset by a randomly
                                       //       chosen direction in [-N:N] deg every M milliseconds
const int COHERENTFC       = 9;        //    same as FASTCENTER, except that only a specified pct of tgt dots move
                                       //       coherently
const int NOISYSPEED       = 10;       //    similar to NOISYDIR, except dot speed is noisy

const int DOTLFINMS        = 0;        // ["dotlife" tgts] units of dot life, msec or deg.  note that these values
const int DOTLFINDEG       = 1;        //    serve as a zero-based index identifying the selected button in the radio
                                       //    control group by which user chooses the units of dot life!
const int MAX_DOTLFINMS    = 32767;    // maximum allowed dot life in msecs (ultimately encoded as a WORD in msecs)
const float MAX_DOTLFINDEG = 327.67f;  // maximum allowed dot life in deg (encoded as a WORD in deg/100)

const int MIN_DIROFFSET    = 0;        // [NOISYDIR only] allowed range for dot direction offset range (whole deg),
const int MAX_DIROFFSET    = 180;
const int MIN_SPEEDOFFSET  = 0;        // [NOISYSPEED only] dot speed noise can be additive or multiplicative. In the
const int MAX_SPEEDOFFSET  = 300;      // former case, the offset noise is randomly chosen from [0..N], where N is the
const int MIN_SPDLOG2      = 1;        // offset range as a % of pattern speed. Else, Vdot ~ Vpat*2^x, where x is
const int MAX_SPDLOG2      = 7;        // randomly chosen from [-N..N; granularity=0.05], where N is "noise power"
const int MIN_NOISEUPD     = 2;        // [NOISYDIR,NOISYSPEED] range for noise update interval, in milliseconds
const int MAX_NOISEUPD     = 1024;

const float FLOWMINRAD     = 0.5f;     // [FLOWFIELD only] limited range for flow field inner & outer radii (vis deg)
const float FLOWMAXRAD     = 44.99f;   //
const float FLOWDIFFRAD    = 2.0f;     // [FLOWFIELD only] min difference in inner & outer radii (vis deg)

const float BAR_MINDA      = 0.0f;     // [ORIENTEDBAR only] allowed range for drift axis in deg
const float BAR_MAXDA      = 359.99f;  //

const float MINRECTDIM     = 0.01f;    // minimum width or height of bounding rect for most XY target subtypes, in deg

typedef struct tagXYScopeParams
{
   int type;                           // target type
   int ndots;                          // # of dots in target

   int iDotLfUnits;                    // [FCDOTLIFE only] dot life units:  DOTLFINMS or DOTLFINDEG
   float fDotLife;                     // [FCDOTLIFE only] maximum lifetime of each target dot

   float fRectW;                       // [RECTDOT] width of rectangular dot array in deg subtended at eye
                                       // [RECTANNU] width of outer bounding rect in deg
                                       // [FLOWFIELD] outer radius of flow field in deg
                                       // [all others] width of bounding rect in deg

   float fRectH;                       // [RECTDOT] dot spacing in deg subtended at eye
                                       // [RECTANNU] height of outer bounding rect in deg
                                       // [FLOWFIELD] not used
                                       // [all others] height of bounding rect in deg

   float fInnerW;                      // [RECTANNU] width of inner bounding rect in deg subtended at eye
                                       // [FLOWFIELD] inner radius of flow field in deg subtended at eye
                                       // [ORIENTEDBAR] drift axis of bar in deg CCW [0..360)
                                       // [NOISYDIR] N in whole deg; dot dirs randomized in [-N:N]
                                       // [NOISYSPEED] noise offset range or noise power; see fInnerX.
                                       // [COHERENTFC] Pct coherence in whole %, [0..100]
                                       // [all others] not used

   float fInnerH;                      // [RECTANNU only] height of inner bounding rect in deg subtended at eye
                                       // [NOISYDIR,NOISYSPEED] dot noise update interval, in msecs.

   float fInnerX, fInnerY;             // [as of 2.0.1] [RECTANNU only] center coords of inner bounding rect (deg), 
                                       // RELATIVE to target center. NOTE: Does NOT impact size of U_TGPARAMS union, 
                                       // b/c XYPARMS is still smaller than RMVTARGET, the other cmpt of the union.
                                       // [as of 2.1.3] [NOISYSPEED only] fInnerX selects dot speed noise algorithm:
                                       //    zero: additive. fInnerW is offset range as % of nominal spd, [0..300].
                                       //    else: *2^N. fInnerW is noise power N, restricted to int in [1..7].
} XYPARMS, *PXYPARMS;


//=====================================================================================================================
// Parameter set for Framebuffer video targets, and related constants
//
// DEPRECATED!  As of Maestro 2.0, the VSG2/4 framebuffer video card was retired, replaced by the RMVideo server,
// which runs on a separate Linux workstation that talks to Maestro over a private, dedicated Ethernet connection.
// RMVideo has far superior graphic capabilities, and implements both XY Scope and the old FBVideo targets.  We are
// maintaining these old definitions to support document schema migration and analysis programs which must read in
// both old and new Maestro data files.
//=====================================================================================================================
// NOTES:
// -->Color specification.  We currently support only the RGB colorspace.  For a single-color target, the contrast
//    values are ignored, and the mean R/G/B triplet defines the target's color.  For multi-color targets, the mean and
//    contrast define the min-max range of luminance for each axis via the definitions:
//          Lmax = Lmean(1+C); Lmin = Lmean(1-C)
//    where C is %constrast/100.
// -->Gratings.  Two sets of grating parameters are provided.  For the single-grating targets, only the first set is
//    used.  For the two-grating targets, both sets are used.
//
const int NUMFBTYPES       = 8;        // FB scope target types:
const int PATCH            = 0;        //    simple uniform patch
const int SINEGRAT         = 1;        //    drifting sinewave grating
const int SQUAREGRAT       = 2;        //    drifting squarewave grating
const int SINEPLAID        = 3;        //    drifting plaid composed of two sinewave gratings
const int SQUAREPLAID      = 4;        //    drifting plaid composed of two squarewave gratings
const int TWOSINGRATS      = 5;        //    like SINEPLAID, but gratings move indepedently
const int TWOSQGRATS       = 6;        //    like SQUAREPLAID, but gratings move indepedently
const int STATICGABOR      = 7;        //    Gabor patch with stationary texture (grating cannot "drift")

const int RECTWIND         = 0;        // rectangular target aperture
const int OVALWIND         = 1;        // elliptical target aperture

const int FB_MAXLUM        = 1000;     // max and min luminance values (arbitrary scale) for color specification
const int FB_MINLUM        = 0;        //
const int FB_MAXCON        = 100;      // max and min percent contrast values for color specification
const int FB_MINCON        = 0;        //
const int FB_RED           = 0;        // the RGB color axes -- indices into the color spec arrays
const int FB_GRN           = 1;
const int FB_BLU           = 2;

typedef struct tagFBVideoParams
{
   int   type;                         // target type
   int   shape;                        // shape of target aperture
   int   csMean[3], csCon[3];          // RGB color specification:  mean and contrast for R(=0), G(=1), and B(=2) axes
                                       //   mean is 0-1000 (uniform scale); contrast is a percentage (0-100%)
   float fRectW, fRectH;               // dimensions of bounding rect in deg subtended at eye
   float fSigma;                       // standard deviation of circular Gaussian window for STATICGABOR target
   float fGratSF[2];                   // grating spatial frequency in cycles/deg subtended at eye
   float fGratAxis[2];                 // grating's drift axis in deg CCW.  grating orientation is this value + 90deg,
                                       //    restricted to the unit half circle [0..180) deg.
   float fGratPhase[2];                // grating's initial spatial phase in deg
} FBPARMS, *PFBPARMS;


//=====================================================================================================================
// A "Generic" MAESTRO target definition
//=====================================================================================================================
// NOTES: As of Maestro v2.0, old FBPARMS replaced by RMVTGTDEF, defined in RMVIDEO_COMMON.H. New fields were added to
// RMVTGTDEF in Maestro v2.5.0 and v4.1.0. These changes impact the structures U_TGPARMS and CXTARGET. The deprecated 
// versions of U_TGPARMS are defined here to support document schema migration. See CXFILEFMT.H for deprecated versions
// of CXTARGET; these are needed for parsing Maestro data files generated by older Maestro versions...
//

// TODO: I really should simplify CXTARGET to contain only wType, name and the RMVideo target def struct, since the
// XYScope is deprecated. This requires a data file format change -- I want to avoid that for now

typedef union tagTgParms
{
   XYPARMS xy;                         // [DEPRECATED] parameters for an XY scope target, OR...
   RMVTGTDEF rmv;                      // parameters for an RMVideo target
} U_TGPARMS, *PU_TGPARMS;

typedef struct tagCxTarget                         // generic MAESTRO target: to transmit target info to MAESTRODRIVER
{
   WORD        wType;                              //    target category/type:  CX_CHAIR ... CX_RMVTARG
   CHAR        name[CX_MAXOBJNAMELEN];             //    target's human-readable name
   U_TGPARMS   u;                                  //    [CX_XYTARG, CX_RMVTARG only] defining parameters
} CXTARGET, *PCXTARGET;

// [DEPRECATED] target parameters prior to data file version 8 (Maestro V 2.0.0): The old VSG framebuffer video card was 
// employed in Maestro versions prior to v2.0, and the old FBPARMS structure defined the VSG-based target parameters. 
typedef union tagTgParmsDeprecated
{
   XYPARMS xy;                         // parameters for an XY scope target, OR...
   FBPARMS fb;                         // parameters for an FB video target
} U_TGPARMS_OLD, *PU_TGPARMS_OLD;

// [DEPRECATED] target parameters prior to data file version 13 (Maestro V 2.5.0): Two char[] fields were added to 
// RMVTGTDEF (rmvideo_common.h) in Maestro v2.5. This deprecated version of U_TGPARMS uses the version of RMVTGTDEF that
// applied to data file versions 8 to 12.
typedef union tagTgParms_v12
{
   XYPARMS xy;                         // parameters for an XY scope target, OR...
   RMVTGTDEF_V12 rmv;                  // parameters for an RMVideo target (data file versions 8-12)
} U_TGPARMS_V12, *PU_TGPARMS_V12;

// [DEPRECATED] target parameters for data file versions 13-22 (Maestro V2.5.0 - V4.0.5): Three int fields were added 
// to RMVTGTDEF (rmvideo_common.h) in Maestro v4.1.0 (data file version 23). This deprecated version of U_TGPARMS uses
// the version of RMVTGTDEF that applied to data file versions 13-22.
typedef union tagTgParms_v22
{
   XYPARMS xy;                         // parameters for an XY scope target, OR...
   RMVTGTDEF_V22 rmv;                  // parameters for an RMVideo target (data file versions 13-22)
} U_TGPARMS_V22, *PU_TGPARMS_V22;



//=====================================================================================================================
// MAESTRO TRIAL-SPECIFIC DEFINITIONS
//=====================================================================================================================

const int   MAX_TRIALTARGS    = 25;    // maximum # targets that can participate in a trial
const int   MAX_SEGMENTS      = 30;    // maximum # of segments in a trial
const int   MAX_STAIRS        = 5;     // maximum # of distinct staircases
const int   MAX_TRIALPERTS    = 4;     // maximum # of perturbation waveforms that can be defined in a trial

// trial random variables
const int   MAX_TRIALRVS      = 10;    // maximum # of distinct random variable that can be defined in a trial
const int   RV_NOTUSED        = 0; 
const int   RV_UNIFORM        = 1;
const int   RV_NORMAL         = 2;
const int   RV_EXPON          = 3;
const int   RV_GAMMA          = 4;
const int   RV_FUNCTION       = 5;
const int   RV_NUMTYPES       = 6;     // including "not used"!

//=====================================================================================================================
// Trial "Header" Information
//=====================================================================================================================
const DWORD THF_KEEP       = (1<<0);      // save (1) or toss (0) data recorded during trial
const DWORD THF_STAIRRESP  = (1<<1);      // the correct response input for a staircase trial: 0 = ADC12, 1 = ADC13
const DWORD THF_MTRMODE    = (1<<8);      // mid-trial reward mode: 0 = periodic, 1 = at segment's end
const DWORD THF_IGNPOSSCALE= (1<<9);      // if set, global target position vector scaling is ignored for this trial
const DWORD THF_IGNPOSROT  = (1<<10);     // if set, global target position vector rotation is ignored for this trial
const DWORD THF_IGNVELSCALE= (1<<11);     // if set, global target velocity vector scaling is ignored for this trial
const DWORD THF_IGNVELROT  = (1<<12);     // if set, global target velocity vector rotation is ignored for this trial

// (as of Maestro 3.1.0) These flag bits are NOT set in the trial header, but they are added by the trial sequencer
// to mark trials participating in a "chained" trial sequence
const DWORD THF_CHAINED    = (1<<20);      // trial is part of a "chained" sequence
const DWORD THF_CHAINSTART = (1<<21);     // trial is the first in a trial "chain" ("chained" sequence mode only)

// OBSOLETE flag bits. As of Maestro 2.0.5 (trial schema version #9), special op is defined by an integer member 
// of the TRLHDR structure. We still need these to do schema migration!
const DWORD THF_SPECIALM   = (0x03<<2);   // 2-bit mask -- special operation during trial, 4 possible states:
const DWORD THF_NOSPECIAL  = (0x00<<2);   //    no special op
const DWORD THF_SACCSKIP   = (0x01<<2);   //    skip to end of special segment if saccade detected
const DWORD THF_SELBYFIX   = (0x02<<2);   //    "select" 1 of 2 fix tgts by fixating on it during special segment
const DWORD THF_SELBYFIX2  = (0x03<<2);   //    a slightly different version of "select by fix"
const DWORD THF_SPECIALMX  = (0x03<<16);  // 2-bit mask -- additional special operations:
const DWORD THF_SWITCHFIX  = (0x01<<16);  //    "switch fix": enforce fixation on either of two tgts during special seg
                                          //    and reward only if animal switches to other tgt by trial's end.
const DWORD THF_RPDISTRO   = (0x02<<16);  //    "RP distro": subj rewarded/penalized based upon behavioral response
                                          //    (eye vel magnitude avg'd over special seg) rel to a reward window(s).
const DWORD THF_SPECRSVD2  = (0x03<<16);  //    reserved for future use
const DWORD THF_SPECALL    = THF_SPECIALM|THF_SPECIALMX;

// OBSOLETE flag bits.  As of Maestro 2.0.0 (trial schema version #8), velocity stabilization is set on a per-target,
// per-segment basis.
const DWORD THF_SNAPTO     = (1<<4);      // if set, fix tg #1 snaps to current eye pos at start of the open-loop seg
const DWORD THF_OPENMASK   = (0x03<<5);   // 2-bit mask -- velocity stabilization (open-loop) enable flags:
const DWORD THF_OPEN_BOTH  = (0x00<<5);   //    both directions stabilized
const DWORD THF_OPEN_HONLY = (0x01<<5);   //    only H direction is stabilized
const DWORD THF_OPEN_VONLY = (0x02<<5);   //    only V direction is stabilized
const DWORD THF_OPEN_RSVD  = (0x03<<5);   //    reserved -- not used

const int   TH_MINWEIGHT   = 0;           // min & max trial weight
const int   TH_MAXWEIGHT   = 255;
const int   TH_MINREWLEN   = 1;           // min, default, and max reward pulse length in msecs
const int   TH_DEFREWLEN   = 10;
const int   TH_MAXREWLEN   = 999;
const int   TH_MINWHVR = 0;               // min, default, and max values for numerator or denominator of the
const int   TH_DEFWHVR = 0;               // variable ratio for random reward withholding
const int   TH_MAXWHVR = 100;
const int   TH_MINSACCVT   = 0;           // min & max saccade threshold velocity in deg/sec
const int   TH_MAXSACCVT   = 999;
const float TH_MINSTAIRSTR = 0.0f;        // minimum and maximum staircase strengths
const float TH_MAXSTAIRSTR = 999.999f;    //
const int   TH_MINREWINTV  = 100;         // min, default, and max mid-trial reward interval, in msecs
const int   TH_DEFREWINTV  = 1000;
const int   TH_MAXREWINTV  = 9999;

const int   TH_NUMSPECOPS     = 9;        // available special operations:
const int   TH_SOP_NONE       = 0;        //    no special operation in use
const int   TH_SOP_SKIP       = 1;        //    skip to end of special segment if saccade detected
const int   TH_SOP_SELBYFIX   = 2;        //    "select" 1 of 2 fix tgts by fixating on it during special segment
const int   TH_SOP_SELBYFIX2  = 3;        //    a slightly different version of "select by fix"
const int   TH_SOP_SWITCHFIX  = 4;        //    "switch fix": enforce fixation on either of two tgts during special seg
                                          //    and reward only if animal switches to other tgt by trial's end.
const int   TH_SOP_RPDISTRO   = 5;        //    "RP distro": subj rewarded/penalized based upon behavioral response
const int   TH_SOP_CHOOSEFIX1 = 6;        //    "choose fixation tgt #1: enforce fixation on fix #1 by end of spec seg
const int   TH_SOP_CHOOSEFIX2 = 7;        //    "choose fixation tgt #2: enforce fixation on fix #2 by end of spec seg
const int   TH_SOP_SEARCH     = 8;        //    "search": Search for a designated target among 1+ distractors.

const int   TH_RPD_NRESPTYPES = 4;        // alternative response measures for the "RP distro" feature:
const int   TH_RPD_EYEVEL     = 0;        //    eye velocity vector magnitude in deg/sec
const int   TH_RPD_HEVEL      = 1;        //    horizontal eye velocity in deg/sec
const int   TH_RPD_VEVEL      = 2;        //    vertical eye velocity in deg/sec
const int   TH_RPD_EYEDIR     = 3;        //    eye velocity vector direction in deg CCW from rightward motion

                                          // op modes for electrical pulse stimulus generator module (SGM):
const int   SGM_SINGLE        = 0;        //    single pulse presented, of specified amplitude and width
const int   SGM_DUAL          = 1;        //    two pulses of distinct amp & width, separated by interpulse interval
const int   SGM_BIPHASIC      = 2;        //    same as SGM_DUAL, but interpulse interval = 0
const int   SGM_TRAIN         = 3;        //    seq of identical pulses occurring in one or more pulse trains
const int   SGM_BIPHASICTRAIN = 4;        //    similar to SGM_TRAIN, except pulses are biphasic (most used mode)
const int   SGM_NOOP          = 5;        //    SGM not in use
const int   SGM_NMODES        = 6;        //    total # of supported operational modes

                                          // range limits for various SGM parameters:
const int   SGM_MINPA         = -128;     //    pulse amplitude min/max (in 80mV increments)
const int   SGM_MAXPA         = 127;
const int   SGM_MINPW         = 5;        //    pulse width min/max (in 10us increments)
const int   SGM_MAXPW         = 250;
const int   SGM_MINIPI        = 1;        //    interpulse interval min/max (in 1ms increments)
const int   SGM_MAXIPI        = 250;
const int   SGM_MINITI        = 1;        //    intertrain interval min/max (in 10ms increments)
const int   SGM_MAXITI        = 250;
const int   SGM_MINPULSES     = 1;        //    #pulses per train min/max
const int   SGM_MAXPULSES     = 250;
const int   SGM_MINTRAINS     = 1;        //    #trains min/max
const int   SGM_MAXTRAINS     = 250;

typedef struct tagSGMParams               // control parameters for the pulse stimulus generator module (SGM). Note
{                                         // that some parameters do not apply to all op modes.
   int      iOpMode;                      //    motion mode -- one of the SGM_* defined constants
   BOOL     bExtTrig;                     //    if TRUE, use external trig to initiate pulse seq; else, s/w start.
   int      iAmp1, iAmp2;                 //    pulse amplitude in mV.  range [-10240..10160mV], res = 80mV.
   int      iPW1, iPW2;                   //    pulse width in us.  range [50..2500us], res = 10us.
   int      iPulseIntv;                   //    interpulse interval in ms.  range [1..250ms], res = 1ms.
   int      iTrainIntv;                   //    intertrain interval in ms.  range [10..2500ms], res = 10ms.
   int      nPulses;                      //    #pulses per train.  range [1..250].  (train modes only)
   int      nTrains;                      //    #trains per stimulus.  range [1..250].  (train modes only)
} SGMPARMS, *PSGMPARMS;

typedef struct tagTrialHeader             // trial header contains general trial attributes and control parameters:
{
   DWORD    dwFlags;                      //    flag bits -- see THF_ flag bit definitions
   int      iWeight;                      //    trial weight, for purposes of random sequencing of all trials in a set
   int      iStairNum;                    //    staircase designation = 1..5; 0 if trial is not part of a staircase
   int      iStartSeg;                    //    if THF_KEEP, we save analog data from start of this segment until
                                          //    trial's end; if negative, entire trial is saved (same as 0).
   int      iFailsafeSeg;                 //    "failsafe" segment: if trial cut short b/c animal broke fix, trial data
                                          //    is still saved if we reached start of this seg (-1 = trial must finish)
   int      iSpecialSeg;                  //    "special" segment during which a saccade-trig'd op may take place
   int      iSpecialOp;                   //    [schema version >=9] special op id; see TH_SOP_*** constants

                                          //    OBSOLETE in trial schema version >= 8:
   int      iOpenSeg;                     //    start segment for velocity stabilization on fix tgt #1 (ignored if <0)
   int      nOpenSegs;                    //    [schema version >=7] # contigous segments in which v. stab. in effect

   int      iMarkSeg1;                    //    [schema version >=3] display marker segment: if valid, a marker is
                                          //    drawn in data trace display at the start time for this segment.
   int      iMarkSeg2;                    //    [schema version >=3] a 2nd display marker segment
   int      iMTRIntv;                     //    [schema version >=4] mid-trial reward interval in msecs
   int      iMTRLen;                      //    [schema version >=4] mid-trial reward pulse length in msecs
   int      iXYDotSeedAlt;                //    [schema version >=6] XY random dot seed optionally overrides display
                                          //    settings: -1 => use disp settings; 0 => auto-seed; >0 => fixed seed.
   int      nXYInterleave;                //    # of XY scope tgts to interleave during trial (0,1 ==> no interleave)
   int      iSaccVt;                      //    saccade threshold velocity in deg/sec (for saccade-trig'd ops)
   int      reward1[3];                   //    reward pulse #1: [len in ms, WHVR numerator, WHVR denominator]
   int      reward2[3];                   //    reward pulse #2: [len in ms, WHVR numerator, WHVR denominator]
   float    fStairStrength;               //    staircase strength (unitless) -- used by staircase trial sequencer
   WORD     wChanKey;                     //    MAESTRO "channel config" obj attached to this trial; if CX_NULLOBJ_KEY,
                                          //    no data is saved or displayed

   int      iSGMSeg;                      //    segment at which a pulse stimulus seq is initiated on SGM (if >= 0)
   SGMPARMS sgm;                          //    control params for the SGM pulse stimulus seq presented during trial
} TRLHDR, *PTRLHDR;

//=====================================================================================================================
// Segment "Header" Information
//=====================================================================================================================
const int   SGH_NOMARKER   = 0;        // indicates no marker pulse should be delivered
const int   SGH_MINMARKER  = 0;        // the range of valid marker pulse values; identifies the DOUT line on which
const int   SGH_MAXMARKER  = 10;       // pulse is delivered.
const float SGH_MINFIXACC  = 0.1f;

// [DEPRECATED] The XYScope is unsupported a/o V4.0, and implementation remove a/o V5.0.
const int   SGH_MINXYFRAME = 2;        // [deprecated] XY frame interval range; also must be a multiple of min value!
const int   SGH_MAXXYFRAME = 256;

typedef struct tagSegHeader   // the segment header parameters:
{
   // min & max duration of segment (ms). If different, actual segment duration is randomly generated via rand() for
   // each trial rep so that it lies in [min..max].
   // (as of v3.3.0) A negative value for duration indicates that a trial random variable x0..x9 has been assigned
   // to segment duration. Both min & max are ALWAYS set to the same RV -- so the value of the RV sets the segment 
   // duration. In this usage, allowed values are [-10 .. -1], and the index of the assigned RV is abs(dur) - 1.
   int      iMinDur;
   int      iMaxDur;

   int      iFixTarg1;        // fixation targets -- these are zero-based indices into the trial's participating
   int      iFixTarg2;        // target list.  if negative, then no fixation target is assigned.
   float    fFixAccH;         // required H,V fixation accuracies during segment (deg subtended at eye).
   float    fFixAccV;         //
   int      iGrace;           // grace period (after segment start) during which fixation is not checked (msec).
   int      iXYFrame;         // update interval for XY scope targets participating in trial (msec).
   int      iMarker;          // marker pulse delivered at start of segment (0 = no pulse)
   BOOL     bChkResp;         // if TRUE, check for correct response during this segment (staircase trials only)
   BOOL     bEnaRew;          // if TRUE, enable periodic "mid-trial" rewards during this segment (special feature)

   //(as of v4.0.0) if TRUE, enable RMVideo sync flash during video frame marking segment start
   BOOL     bEnaRMVSync;
} SEGHDR, *PSEGHDR;

//=====================================================================================================================
// Per-Segment, Per-Target Trajectory Information
//=====================================================================================================================
const DWORD SGTJF_ON          = (1<<0);      // bit flag: target on (set) or off (clear) during segment
const DWORD SGTJF_ABS         = (1<<1);      // bit flag: target initial pos is absolute (set) or relative (clear)
const DWORD SGTJF_VSTABMODE   = (0x03<<2);   // [trial schema v>=8]: velocity stabilization mode mask
const DWORD SGTJF_VSTABSNAP   = (1<<4);      // [trial schema v>=8]: snap tgt to eye when vel. stab turns on
const DWORD SGTJF_VSTABMASK   = (SGTJF_VSTABMODE|SGTJF_VSTABSNAP);

#define FLAGS_TO_VSTABMODE(w)   ((int) ((((DWORD)w) & SGTJF_VSTABMODE) >> 2))
#define VSTABMODE_TO_FLAGS(i)   ((((DWORD)i) << 2) & SGTJF_VSTABMODE)

const int SGTJ_VSTABOFF = 0;                 // [trial schema v>=8]: the possible velocity stabilization modes
const int SGTJ_VSTABBOTH = 1;
const int SGTJ_VSTABHONLY = 2;
const int SGTJ_VSTABVONLY = 3;

// flag set if corresponding target trajectory parameter is assigned to a trial random variable rather than a numeric
// constant. IF set, the corresponding field in TRAJINFO is cast to an integer to retrieve the RV's zero-based index.
const DWORD SGTJF_POSH_ISRV   = (1<<10);
const DWORD SGTJF_POSV_ISRV   = (1<<11);
const DWORD SGTJF_VELH_ISRV   = (1<<12);
const DWORD SGTJF_VELV_ISRV   = (1<<13);
const DWORD SGTJF_ACCH_ISRV   = (1<<14);
const DWORD SGTJF_ACCV_ISRV   = (1<<15);
const DWORD SGTJF_PATVELH_ISRV= (1<<16);
const DWORD SGTJF_PATVELV_ISRV= (1<<17);
const DWORD SGTJF_PATACCH_ISRV= (1<<18);
const DWORD SGTJF_PATACCV_ISRV= (1<<19);
const DWORD SGTJF_ISRVMASK = (0x03FF<<10);

const float SGTJ_POSMAX    = 999.99f;     // these are just to limit input to restricted ranges; these restrictions do
const float SGTJ_VELMAX    = 999.99f;     // NOT guarantee that trial will run properly...
const float SGTJ_ACCMAX    = 9999.99f;

typedef struct tagTgSegTraj            // the per-segment, per-target trajectory info:
{
   DWORD    dwFlags;                   // trajectory flags -- see SGTJF_ flag bit definitions

   float    fPosH, fPosV;              // target window initial position at segment start (deg subtended at eye)
   float    fVelH, fVelV;              // target window's constant velocity during segment (deg/sec)
   float    fAccH, fAccV;              // target window's constant acceleration during segment (deg/sec^2)

   float    fPatVelH, fPatVelV;        // target pattern's constant velocity during segment (deg/sec)
   float    fPatAccH, fPatAccV;        // [seg schema >= 2] pattern's constant acceleration during segment (deg/sec^2)
} TRAJINFO, *PTRAJINFO;


//=====================================================================================================================
// Intra-trial Tagged Section Descriptor
//=====================================================================================================================
const int   SECTIONTAGSZ     = 18;        // max #chars in a trial section tag, INCLUDING the terminating null char

typedef struct tagTrialSection            // a tagged section of contiguous segments within a trial
{
   char     tag[SECTIONTAGSZ];            //    null-terminated name tag for segment
   char     cFirstSeg;                    //    index of first segment in section
   char     cLastSeg;                     //    index of last segment in section
} TRIALSECT, *PTRIALSECT;


//=====================================================================================================================
// MAESTRO CONTINUOUS RUN-SPECIFIC DEFINITIONS
//=====================================================================================================================

const int   MAXSTIMULI        = 20;    // maximum # of stimulus channels per continuous-mode run
const int   MAXTGTSINXYSEQ    = 25;    // maximum # of XY scope targets participating in the XYSEQ stimulus
const int   MAX_XYSEQVECS     = 32;    // max # of different motion vectors for XY targets in 'XYseq' xstim
const int   MAX_ACTIVETGTS    = 5;     // maximum # of targets in ContMode's "active target list"

const int   STIM_NLASTMARKER  = SGH_MAXMARKER;    // marker pulses delivered on DOUT<1..max>; 0 ==> "OFF"

const int   STIM_NTYPES       = 3;     // available stimulus channel types:
const int   STIM_ISCHAIR      = 0;     //    animal chair (trial target CX_CHAIR)
// const int   STIM_ISFIBER1     = 1;     //    NO LONGER SUPPORTED AS OF Maestro 3.0: fiber-optic target #1 (CX_FIBER1)
// const int   STIM_ISFIBER2     = 2;     //    NO LONGER SUPPORTED AS OF Maestro 3.0: fiber-optic target #2 (CX_FIBER2)
const int   STIM_ISPSGM       = 1;     //    pulse stimlus generator module
// [deprecated] specialized random-motion sequence on a set of XYScope targets
const int   STIM_ISXYSEQ      = 2;

const int   STIM_NSTDMODES    = 2;     // motion modes for STIM_ISCHAIR
const int   MODE_ISSINE       = 0;     //    sinuosoidal
const int   MODE_ISPULSE      = 1;     //    trapezoidal pulse (with non-zero rise & fall times)

const int   STIM_NPSGMMODES   = SGM_NMODES-1;   // (SGM_NOOP is not used in stimulus runs!)

const int   STIM_NXYSEQMODES  = 4;     // [deprecated] motion modes applicable to the XYSEQ stimulus type:
const int   MODE_ISSPARSEDIR  = 0;     //    direction randomized.  one XY tgt, chosen randomly, moves per "segment"
const int   MODE_ISDENSEDIR   = 1;     //    all targets move, directions separately randomized each "segment"
const int   MODE_ISSPARSEVEL  = 2;     //    velocity randomized.  one XY tgt, randomly chosen, moves per "segment"
const int   MODE_ISDENSEVEL   = 3;     //    all targets move, velocities separately randomized each "segment"

const int   STIM_NMAXMODES    = 5;     // maximum # of motion modes for any type

typedef struct tagXYseqMotion       // [deprecated] the motion parameters for an XYseq stimulus channel:
{
   int   iOpMode;                   //    motion mode -- MODE_ISSPARSEDIR, etc.
   int   iRefresh;                  //    XY scope refresh period, in millisecs
   int   nSegs;                     //    # of distinct segments of random motion
   int   iSegDur;                   //    duration of each segment, in ms (must be multiple of refresh period)
   int   iSeed;                     //    seed for generating random directions or velocities
   int   nChoices;                  //    # of different directions (or velocities) randomized
   float fAngle;                    //    offset angle (for direction modes) or direction of motion (for vel modes)
   float fVel;                      //    velocity of motion (for dir modes) or max velocity (for vel modes)
   float fOffsetV;                  //    offset velocity (for vel modes only)
} XYSEQSTIM, *PXYSEQSTIM;

typedef struct tagSineMotion        // the motion parameters for sinewave stimuli:
{
   int   iPeriod;                   //    period in msecs (>= 10ms)
   int   nCycles;                   //    # of complete cycles in stimulus (>=1)
   float fAmp;                      //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   float fPhase;                    //    phase in deg: [-180.0 .. 180.0]
   float fDirec;                    //    direction of motion, CCW angle from x-axis [-180.0..180.0]
                                    //    THIS LAST PARAMETER APPLIED ONLY TO Fiber1/2; NO LONGER USED.
} SINESTIM, *PSINESTIM;

typedef struct tagPulseMotion       // the motion parameters for trapezoidal pulse stimuli:
{
   BOOL  bBlank;                    //    if TRUE, active targets are blanked during pulse (for CHAIR stimulus)
   int   iPulseDur;                 //    duration of pulse in ms (>= 2ms)
   int   iRampDur;                  //    duration of rising-edge and falling-edge ramps (>= 2ms)
   float fAmp;                      //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   float fDirec;                    //    direction of motion, CCW angle from x-axis [-180.0..180.0]
                                    //    THIS LAST PARAMETER APPLIED ONLY TO Fiber1/2; NO LONGER USED.
} PULSESTIM, *PPULSESTIM;

const int   STIM_NCOMMON      = 5;  // # of common parameters in a stimulus channel definition
const int   MAXSTIMPARAMS     = 15; // max # of total parameters ("common" + "motion") defining a stimulus channel
typedef struct tagStimChannel       // defn of a stim channet w/in a ContMode run, in a MAESTRODRIVER-compatible form
{                                   //
   BOOL     bOn;                    //    TRUE = stimulus should be played during the run; FALSE = stim disabled
   int      iMarker;                //    OFF (0), or DOUT ch# on which marker pulse is delivered at stimulus start
   int      iType;                  //    type of stimulus:  see STIM_IS** constants
   int      iStdMode;               //    motion mode for the "standard" stim types: MODE_ISSINE or _ISPULSE
   int      tStart;                 //    start time of stimulus trajectory within the run's duty cycle, in millisecs
   union                            //    motion parameter list -- usage varies with stimulus type & motion mode.
   {
      SINESTIM    sine;
      PULSESTIM   pulse;
      SGMPARMS    sgm;
      XYSEQSTIM   xy;
   };
} STIMCHAN, *PSTIMCHAN;


typedef struct tagRun               // definition of a ContMode run in a CXDRIVER-compatible form
{
   int      iDutyPeriod;            //    duty period in milliseconds
   int      iDutyPulse;             //    OFF (0), or DOUT ch# on which marker pulse is delivered per duty cycle
   int      nAutoStop;              //    auto-stop the run after this many cycles elapsed (0 = no auto-stop)
   float    fHOffset;               //    horizontal position offset in deg subtended at eye
   float    fVOffset;               //    vertical position offset in deg subtended at eye
   int      nStimuli;               //    # of stimulus channels defined for this run
   STIMCHAN stim[MAXSTIMULI];       //    the individual stimulus channel definitions
   int      nXYTgts;                //    # of XY scope targets participating in an XYseq stimulus in this run
   CXTARGET xyTgts[MAXTGTSINXYSEQ]; //    defns of those targets (in format used for storing to file)
   float    fCtrX[MAXTGTSINXYSEQ];  //    center location of each XY target's window
   float    fCtrY[MAXTGTSINXYSEQ];  //
} CONTRUN, *PCONTRUN;



//=====================================================================================================================
// MAESTRO PERTURBATION WAVEFORM-SPECIFIC DEFINITIONS
//=====================================================================================================================

const int   PERT_NTYPES       = 4;  // available perturbation waveform types (NOTE: all have unit amplitude):
const int   PERT_ISSINE       = 0;  //    sinusoidal waveform
const int   PERT_ISTRAIN      = 1;  //    pulse train
const int   PERT_ISNOISE      = 2;  //    uniform random noise
const int   PERT_ISGAUSS      = 3;  //    (v1.3.2) Gaussian-distributed random noise with zero mean and unit variance

const int   PERT_NCMPTS       = 10; // # of different trajectory components that can be affected by a perturbation
const int   PERT_ON_HWIN      = 0;  // a pert can affect any one of these components of a trial tgt's trajectory:
const int   PERT_ON_VWIN      = 1;  //    horiz or verti window velocity
const int   PERT_ON_HPAT      = 2;  //    horiz or verti pattern velocity
const int   PERT_ON_VPAT      = 3;
const int   PERT_ON_DWIN      = 4;  //    (as of v1.3.2) the direction of a target's window or pattern velocity vector
const int   PERT_ON_DPAT      = 5;  //    (was introduced to provide directional noise)
const int   PERT_ON_SWIN      = 6;  //    (as of v2.1.2) the amplitude of a target's window or pattern velocity vector
const int   PERT_ON_SPAT      = 7;  //    (was introduced to provide speed noise)
const int   PERT_ON_DIR       = 8;  //    (as of v2.1.3) perturb direction of BOTH window and pattern vel vectors
const int   PERT_ON_SPD       = 9;  //    (as of v2.1.3) perturb amplitude (speed) of BOTH window and pattern vel vecs

typedef struct tagSinePert          // defining parameters for sinuosoidal perturbation:
{
   int   iPeriod;                   //    period in msecs (>= 10ms)
   float fPhase;                    //    phase in deg: [-180.0 .. 180.0]
} SINEPERT, *PSINEPERT;

typedef struct tagPulsePert         // defining parameters for trapezoidal pulse train perturbation:
{
   int   iPulseDur;                 //    duration of pulse in ms (>= 10ms)
   int   iRampDur;                  //    duration of rising-edge and falling-edge ramps (>= 0ms)
   int   iIntv;                     //    interval between pulses in ms (> 2*rampD + pulsD)
} TRAINPERT, *PTRAINPERT;

typedef struct tagNoisePert         // defining parameters for uniform or Gaussian (unit var) random noise pert:
{
   int   iUpdIntv;                  //    update interval in ms (>= 1ms)
   float fMean;                     //    mean noise level [-1..1]
   int   iSeed;                     //    (as of v1.3.2) seed for underlying RNG.  if 0, the seed is randomly chosen
} NOISEPERT, *PNOISEPERT;

typedef struct tagPertDef           // complete definition of a perturbation
{
   int iType;                       //    perturbation type -- one of the PERT_IS*** constants
   int iDur;                        //    duration of the perturbation in ms (>= 10ms)
   union                            //    type-specific defining parameters
   {
      SINEPERT    sine;
      TRAINPERT   train;
      NOISEPERT   noise;
   };
} PERT, *PPERT;


//=====================================================================================================================
// EYELINK TRACKER-SPECIFIC DEFINITIONS
//=====================================================================================================================

// default values and range limits for offset and gain factors converting Eyelink raw pupil location in integer camera 
// coordinates to calibrated gaze position in visual degrees. Note that the gain factors are divisors:
// Vis deg = (pupil - offset) / gain. Eyelink raw pupil coordinate is typically between 200-400 units/vis deg.
// NOTE that gain can be negative, to invert raw coordinate.
const int EL_DEFOFS = 0;
const int EL_MINOFS = -2000;
const int EL_MAXOFS = 2000;
const int EL_DEFGAIN = 300;
const int EL_MINGAIN = 50;       // minimum absolute value -- gain can be negative
const int EL_MAXGAIN = 2000;     // maximum absolute value -- gain can be negative

// default value and allowed range for the width of the "sliding-average" window used to smooth velocity signals 
// generated by differentiating (center-point difference) Eyelink position data (in #samples, which should be ms)
const int EL_DEFSMOOTHW = 20;
const int EL_MINSMOOTHW = 3;
const int EL_MAXSMOOTHW = 50;

// recording type -- off (Eyelink not in use), monocular left or right, or binocular
const int EL_NOTINUSE = 0;
const int EL_MONO_LEFT = 1;
const int EL_MONO_RIGHT = 2;
const int EL_BINOCULAR = 3;

typedef struct tagELCoord
{
   float fx;
   float fy;
} ELCOORD, *PELCOORD;

// Eyelink sample data passed to MaestroDRIVER from worker thread in Maestro (when Eyelink tracker in use)
typedef struct tagELSample
{
   // sample timestamp when tracker camera imaged eye (ms since current recording session started on tracker)
   UINT32 ts;                     
   // flags indicating whether or not sample includes data for L=0,R=1 eyes
   BOOL gotEye[2];
   // calibrated gaze position in visual degrees for L=0, R=1 eyes
   ELCOORD pos[2];
   // computed gaze velocity in deg/sec (differentiated and smoothed) for L=0, R=1 eyes
   ELCOORD vel[2];
} ELSAMP, *PELSAMP;

// indices into position and velocity arrays in Eyelink sample data structure
const int EL_LEFT = 0;
const int EL_RIGHT = 1;

#endif   // !defined(CXOBJ_IFC_H__INCLUDED_)
