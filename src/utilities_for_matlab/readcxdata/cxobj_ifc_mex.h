//=====================================================================================================================
//
// cxobj_ifc_mex.h : CNTRLX object-related defines that are shared among two or more CNTRLX classes.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This is a modified version of the CNTRLX/CXDRIVER source code header file CXOBJ_IFC.H.  It is required to build the
// MATLAB MEX function readcxdata(), which is used to read CNTRLX data files into the MATLAB environment.  MATLAB's MEX
// tool uses the GNU CC compiler; while GCC can handle C++ source code files, MEX handles only C files.
//
// It is the "const <type> IDENTIFIER = <value>" syntax that caused problems with the MEX build.  While such a constant
// definition is acceptable when <value> is a constant, it introduces several problems down the line:
//    1) The compiler cannot handle "const <type> IDENTIFIER = <constant identifier>" when <constant identifier> was
//       defined using the above syntax.  Here we must replace <constant identifer> with a constant literal or a
//       #define'd constant name.
//    2) Any constant defined using the above syntax CANNOT be used to indicate the dimensions of an array declared
//       outside a function.  Nor can they be used in "case <constant>" expressions within a "switch" clause.  We must
//       define these constants using the #define construct.
//
// Thus, I've replaced all "const <type> IDENTIFIER = <value>" constructs with "#define IDENTIFIER <value>" throughout
// this file.  I also commented out an #if-endif preprocessor clause specific to MS VC++.
//
// See also:  CXOBJ_IFC.H in CNTRLX development folder.
//
//
// REVISION HISTORY:
// 28mar2003-- Modified from the original CXOBJ_IFC.H dtd 13mar2003.  Keep in synch w/ future changes to CXOBJ_IFC.H.
// 01oct2003-- Modified IAW recent changes to CXOBJ_IFC.H.
// 13oct2004-- Modified IAW same-dtd changes in CXOBJ_IFC.H.  Changes do not impact READCXDATA or EDITCXDATTA.
// 03nov2004-- Modified IAW same-dtd changes in CXOBJ_IFC.H.  Changes do not impact READCXDATA or EDITCXDATTA.
// 26jan2005-- Modified IAW changes in CXOBJ_IFC.H dtd 25jan2005.  No impact.
// 07apr2005-- Modified IAW changes in CXOBJ_IFC.H dtd 06apr2005.  Changes include increase in max trial length from
//             10000 to 30000ms, an increase in the # of MAX_SEGMENTS, plus additions to support "tagged sections" in
//             a Maestro trial.  READCXDATA modified accordingly.  EDITCXDATA unaffected.
// 29jul2005-- Modified IAW changes in CXOBJ_IFC.H dtd 28jul2005, re: implementation of PERT_ISGAUSS perturbation and
//             introduction of a seed value for both types of noise perts.
// 03jan2006-- Modified IAW changes in CXOBJ_IFC.H between 28jul2005 and 29nov2005.
// 11jan2006-- Modified IAW changes in CXOBJ_IFC.H dtd 07jan2006.  READCXDATA and EDITCXDATA unaffected.
// 18mar2006-- Modified IAW changes in CXOBJ_IFC.H for Maestro v1.5.0.  READCXDATA affected by changes in velocity
//             stabilization feature and removal of the OKNDRUM target.
// 18apr2006-- Modified IAW changes in CXOBJ_IFC.h for Maestro v2.0.0.  READCXDATA affected by changes in velocity
//             stabilization feature and the replacement of FB video by RMVideo targets...
// 19jun2006-- Modified IAW changes in CXOBJ_IFC.H for Maestro v2.0.1.  READCXDATA affected by addition of fields 
//             'fInnerX' and 'fInnerY' to XYPARMS structure.  Size of union U_TGPARMS and of structs CXTARGET and 
//             CXTARGET_V7 unchanged.  Data file version incr'd to 9.
// 27feb2007-- Modified IAW changes in CXOBJ_IFC.H dtd 23jul2006 and 03jan2007. These changes do not affect READCXDATA.
// 27feb2007-- Modified IAW changes in CXOBJ_IFC.H dtd 27feb2007, for Maestro 2.1.0. Changes do not affect READCXDATA.
// 16may2007-- Modified IAW changes in CXOBJ_IFC.H dts 25apr2007, for Maestro 2.1.1. READCXDATA need TH_RPD_*** 
//             constants to parse the new R/P Distro behavioral response type from the SPECIALOP trial code group.
// 16jul2007-- Modified IAW changes in CXOBJ_IFC.H dtd 16jul2007, for Maestro 2.1.2. READCXDATA's PERTMGR module needs 
//             the new PERT_ON_SWIN, _SPAT constants.
// 06sep2007-- Modified IAW changes in CXOBJ_IFC.H dtd 29aug2007, for Maestro 2.1.3. READCXDATA's PERTMGR module needs 
//             the new PERT_ON_SPD, _DIR constants.
//          -- Modified IAW changes in CXOBJ_IFC.H dtd 31aug2007, for Maestro 2.1.3. XYRECON affected.
// 17sep2009-- Modified IAW changes in CXOBJ_IFC.H dtd 08sep2009, for Maestro 2.5.0. New version of RMVTGTDEF, included
//             indirectly via rmvideo_common.h, which MEX can handle. Deprecated RMVTGTDEF is RMVTGTDEF_V12.
// 06sep2013-- Modified IAW changes in CXOBJ_IFC.H dtd 01feb2011 and 21sep2011. These did not affect readcxdata().
// 24nov2015-- Modified IAW changes in CXOBJ_IFC.H dtd from 21sep2011 to 24nov2015. No bearing on readcxdata().
// 16nov2016—- Modified IAW changes in CXOBJ_IFC.H in Fall2016 re: “trial random variables”. No impact on readcxdata().
// 01oct2018-- Modified IAW changes in CXOBJ_IFC.H dtd 20sep2018 re: SEGHDR.bEnaRMVSync.
// 13may2019-- Modified IAW changes in CXOBJ_IFC.H dtd 07may2019, for Maestro 4.1.0.  New version of RMVTGTDEF, included
//             indirectly via rmvideo_common.h, which MEX can handle. Deprecated RMVTGTDEF is RMVTGTDEF_V22, applicable
//             to data file versions 13-22.
// 05nov2024-- Modified IAW changes in CXOBJ_IFC.H dtd 15may2019 - 31oct2024.
//=====================================================================================================================


#if !defined(CXOBJ_IFC_MEX_H__INCLUDED_)
#define CXOBJ_IFC_MEX_H__INCLUDED_

// #if _MSC_VER > 1000
// #pragma once
// #endif // _MSC_VER > 1000

#include "rmvideo_common.h"    // for RMVideo target-related definitions -- introduced in Maestro v2.0.0


//=====================================================================================================================
// GENERAL CNTRLX OBJECT DEFINITIONS
//=====================================================================================================================

//=====================================================================================================================
// CNTRLX Object Types (P = "predefined"; U = user-defined; C = collection obj; D = data obj)
//
// !!!IMPORTANT!!! We do rely on the order of these constants!
//=====================================================================================================================
#define        CX_ROOT              0x0010            // [P,C] the root of the CNTRLX object tree

#define        CX_TRIALBASE         0x0011            // [P,C] base of the trial subtree (immediate child of CX_ROOT)
#define        CX_TRIALSET          0x0012            // [U,C] a set of individual trials
#define        CX_TRIAL             0x0013            // [U,D] an individual trial object

#define        CX_TARGBASE          0x0014            // [P,C] base of the target subtree (immediate child of CX_ROOT)
#define        CX_TARGSET           0x0015            // [PU,C] a set of individual targets
#define        CX_CHAIR             0x0016            // [P,D] the servo-controlled rotating animal chair
// as of Maestro 3, CX_FIBER* and CX_REDLED* are no longer supported. The constants remain defined so that Maestro 2.x
// experiment documents can be read and migrated to Maestro 3.
#define        CX_FIBER1            0x0017            // [P,D] spot tgts proj. on translucent screen; shuttered
#define        CX_FIBER2            0x0018            //    fiber optic spots, pos cntrl'd by mirror galvonometers.
#define        CX_REDLED1           0x0019            // [P,D] on-off, immovable spots projected on translucent
#define        CX_REDLED2           0x001A            //    screen, using shuttered LEDs
#define        CX_OKNDRUM           0x001B            // [P,D] NO LONGER SUPPORTED AS OF MAESTRO V1.5.0
#define        CX_XYTARG            0x001C            // [U,D] an individual XY scope target -- UNSUPPORTED a/o V4.0!
#define        CX_FBTARG            0x001D            // [U,D] an individual FB video target -- OBSOLETE as of V2.0!
#define        CX_RMVTARG           CX_FBTARG         // [U,D] an individual RMVideo target

#define        CX_CHANBASE          0x001E            // [P,C] base of the "channel configurations" subtree
#define        CX_CHANCFG           0x001F            // [PU,D] a channel configuration

#define        CX_CONTRUNBASE       0x0020            // [P,C] base of ContMode run subtree (immed child of CX_ROOT)
#define        CX_CONTRUNSET        0x0021            // [U,C] a set of continuous runs
#define        CX_CONTRUN           0x0022            // [U,D] an individual continuous run object

#define        CX_PERTBASE          0x0023            // [P,C] base of perturbation subtree (immed child of CX_ROOT)
#define        CX_PERTURB           0x0024            // [U,D] an individual perturbation waveform defn

#define        CX_TRIALSUBSET       0x0025            // [U,C] a subset of individual trials (child of CX_TRIALSET)

#define        CX_FIRST_TYP         CX_ROOT           // for validating object type -- note contiguous range!
#define        CX_LAST_TYP          CX_TRIALSUBSET 

#define        CX_FIRST_TARG        CX_CHAIR          // for validating a target object type
#define        CX_LAST_TARG         CX_RMVTARG        //


//=====================================================================================================================
// CNTRLX Object State Flags
//=====================================================================================================================
#define        CX_ISPREDEF          0x0001            // this object is predefined; cannot be copied, removed, renamed
#define        CX_NOINSERT          0x0002            // cannot add children to this predef *collection* object
#define        CX_OBJFLAGS          0x0003            // for masking these flags
#define        CX_ISSETOBJ          0x0004            // this flag set for all CNTRLX collection objects

#define        CX_NULLOBJ_KEY       0                 // reserved key indicates failure to insert or find object
#define        CX_MAXOBJNAMELEN     50                // (MEX) maximum name length for any data obj defined in CNTRLX



//=====================================================================================================================
// CNTRLX TARGET-SPECIFIC DEFINITIONS
//=====================================================================================================================

//=====================================================================================================================
// Parameter set for XY scope targets, and related constants
// 
// DEPRECATED!  As of Maestro 4.0, the XYScope platform is no longer supported, and a/o V5.0, XYScope-specific code has
// been mostly excised from Maestro and CXDRIVER. However, we must maintain these old definitions to support document 
// schema migration and analysis programs which must read in both old and new Maestro data files.
//
//=====================================================================================================================
#define NUMXYTYPES        11           // XY scope target types:
#define RECTDOT            0           //    rectangular dot array
#define CENTER             1           //    full-screen random-dot pattern visible only inside defined rectangle
#define SURROUND           2           //    full-screen random-dot pattern visible only outside defined rectangle
#define RECTANNU           3           //    full-screen random-dot pattern visible only inside rectangular annulus
#define FASTCENTER         4           //    rectangular random-dot pattern; optimized version of center draws all dots
                                       //       inside rect rather than distributing dot pos over entire screen
#define FCDOTLIFE          5           //    same as FASTCENTER, but each dot is assigned a random "lifetime".  when
                                       //       life expires or dot hits border, its randomly repositioned within rect
                                       //       and its dot life is reset
#define FLOWFIELD          6           //    optic flow field.  very different from other tgt types.
#define ORIENTEDBAR        7           //    rect bar or line oriented at any angle in [0..360) deg.
#define NOISYDIR           8           //    same as FCDOTLIFE, but each dot's direction vector is offset by a randomly
                                       //       chosen direction in [-N:N] deg every M milliseconds
#define COHERENTFC         9           //    same as FASTCENTER, except that only a specified pct of tgt dots move
                                       //       coherently
#define NOISYSPEED         10          //    similar to NOISYDIR, except dot speed is noisy

#define DOTLFINMS          0           // ["dotlife" tgts] units of dot life, msec or deg.  note that these values
#define DOTLFINDEG         1           //    serve as a zero-based index identifying the selected button in the radio
                                       //    control group by which user chooses the units of dot life!
#define MAX_DOTLFINMS      32767       // maximum allowed dot life in msecs (ultimately encoded as a WORD in msecs)
#define MAX_DOTLFINDEG     327.67f     // maximum allowed dot life in deg (encoded as a WORD in deg/100)

#define MIN_DIROFFSET      0           // [NOISYDIR only] allowed range for dot direction offset range (whole deg)
#define MAX_DIROFFSET      180
#define MIN_SPEEDOFFSET    0           // [NOISYSPEED only] dot speed noise can be additive or multiplicative. In the
#define MAX_SPEEDOFFSET    300         // former case, the offset noise is randomly chosen from [0..N], where N is the
#define MIN_SPDLOG2        1           // offset range as a % of pattern speed. Else, Vdot ~ Vpat*2^x, where x is
#define MAX_SPDLOG2        7           // randomly chosen from [-N..N; granularity=0.05], where N is "noise power"
#define MIN_NOISEUPD       2           // [NOISYDIR,NOISYSPEED] range for noise update interval, in milliseconds
#define MAX_NOISEUPD       1024

#define FLOWMINRAD         0.5f        // [FLOWFIELD only] limited range for flow field inner & outer radii (vis deg)
#define FLOWMAXRAD         44.99f      //
#define FLOWDIFFRAD        2.0f        // [FLOWFIELD only] min difference in inner & outer radii (vis deg)

#define BAR_MINDA          0.0f        // [ORIENTEDBAR only] allowed range for drift axis in deg
#define BAR_MAXDA          359.99f     //

#define MINRECTDIM         0.01f       // minimum width or height of bounding rect for most XY target subtypes, in deg

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
#define NUMFBTYPES         8           // FB scope target types:
#define PATCH              0           //    simple uniform patch
#define SINEGRAT           1           //    drifting sinewave grating
#define SQUAREGRAT         2           //    drifting squarewave grating
#define SINEPLAID          3           //    drifting plaid composed of two sinewave gratings
#define SQUAREPLAID        4           //    drifting plaid composed of two squarewave gratings
#define TWOSINGRATS        5           //    like SINEPLAID, but gratings move indepedently
#define TWOSQGRATS         6           //    like SQUAREPLAID, but gratings move indepedently
#define STATICGABOR        7           //    Gabor patch with stationary texture (grating cannot "drift")

#define RECTWIND           0           // rectangular target aperture
#define OVALWIND           1           // elliptical target aperture

#define FB_MAXLUM          1000        // max and min luminance values (arbitrary scale) for color specification
#define FB_MINLUM          0           //
#define FB_MAXCON          100         // max and min percent contrast values for color specification
#define FB_MINCON          0           //
#define FB_RED             0           // the RGB color axes -- indices into the color spec arrays
#define FB_GRN             1
#define FB_BLU             2

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

typedef union tagTgParms
{
   XYPARMS xy;                         // parameters for an XY scope target, OR...
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

#define     MAX_TRIALTARGS       25          // maximum # targets that can participate in a trial
#define     MAX_SEGMENTS         30          // maximum # of segments in a trial
#define     MAX_STAIRS           5           // maximum # of distinct staircases
#define     MAX_TRIALPERTS       4           // maximum # of perturbation waveforms that can be defined in a trial

// trial random variables
#define     MAX_TRIALRVS         10          // maximum # of distinct random variable that can be defined in a trial
#define     RV_NOTUSED           0
#define     RV_UNIFORM           1
#define     RV_NORMAL            2
#define     RV_EXPON             3
#define     RV_GAMMA             4
#define     RV_FUNCTION          5
#define     RV_NUMTYPES          6           // including "not used"!


//=====================================================================================================================
// Trial "Header" Information
//=====================================================================================================================
#define     THF_KEEP             (1<<0)      // save (1) or toss (0) data recorded during trial
#define     THF_STAIRRESP        (1<<1)      // the correct response input for a staircase trial: 0 = ADC12, 1 = ADC13
#define     THF_MTRMODE          (1<<8)      // mid-trial reward mode: 0 = periodic, 1 = at segment's end
#define     THF_IGNPOSSCALE      (1<<9)      // if set, global target position vector scaling is ignored for this trial
#define     THF_IGNPOSROT        (1<<10)     // if set, global target position vector rotation is ignored for this trial
#define     THF_IGNVELSCALE      (1<<11)     // if set, global target velocity vector scaling is ignored for this trial
#define     THF_IGNVELROT        (1<<12)     // if set, global target velocity vector rotation is ignored for this trial

// (as of Maestro 3.1.0) These flag bits are NOT set in the trial header, but they are added by the trial sequencer
// to mark trials participating in a "chained" trial sequence
#define     THF_CHAINED          (1<<20)     // trial is part of a "chained" sequence
#define     THF_CHAINSTART       (1<<21)     // trial is the first in a trial "chain" ("chained" sequence mode only)


// OBSOLETE flag bits. As of Maestro 2.0.5 (trial schema version #9), special op is defined by an integer member 
// of the TRLHDR structure. We still need these to do schema migration!
#define     THF_SPECIALM         (0x03<<2)   // 2-bit mask -- special operation during trial, 4 possible states:
#define     THF_NOSPECIAL        (0x00<<2)   //    no special op
#define     THF_SACCSKIP         (0x01<<2)   //    skip to end of special segment if saccade detected
#define     THF_SELBYFIX         (0x02<<2)   //    "select" 1 of 2 fix tgts by fixating on it during special segment
#define     THF_SELBYFIX2        (0x03<<2)   //    a slightly different version of "select by fix"
#define     THF_SPECIALMX        (0x03<<16)  // 2-bit mask -- additional special operations:
#define     THF_SWITCHFIX        (0x01<<16)  //    "switch fix": enforce fixation on either of two tgts during special
                                             //    seg and reward only if animal switches to other tgt by trial's end.
#define     THF_RPDISTRO         (0x02<<16)  //    "RP distro": subj rewarded/penalized based upon behavioral response
                                             //    (eye vel magnitude avg'd over special seg) rel to a reward window(s).
#define     THF_SPECRSVD2        (0x03<<16)  //    reserved for future use
#define     THF_SPECALL          THF_SPECIALM|THF_SPECIALMX

// OBSOLETE flag bits.  As of Maestro 2.0.0 (trial schema version #8), velocity stabilization is set on a per-target,
// per-segment basis.
#define     THF_SNAPTO           (1<<4)      // if set, fix tg #1 snaps to current eye pos at start of open-loop seg
#define     THF_OPENMASK         (0x03<<5)   // 2-bit mask -- velocity stabilization (open-loop) enable flags:
#define     THF_OPEN_BOTH        (0x00<<5)   //    both directions stabilized
#define     THF_OPEN_HONLY       (0x01<<5)   //    only H direction is stabilized
#define     THF_OPEN_VONLY       (0x02<<5)   //    only V direction is stabilized
#define     THF_OPEN_RSVD        (0x03<<5)   //    reserved -- not used

#define     TH_MINWEIGHT         0           // min & max trial weight
#define     TH_MAXWEIGHT         255
#define     TH_MINREWLEN         1           // min, default, and max reward pulse length in msecs
#define     TH_DEFREWLEN         10
#define     TH_MAXREWLEN         999
#define     TH_MINSACCVT         0           // min & max saccade threshold velocity in deg/sec
#define     TH_MAXSACCVT         999
#define     TH_MINSTAIRSTR       0.0f        // minimum and maximum staircase strengths
#define     TH_MAXSTAIRSTR       999.999f    //
#define     TH_MINREWINTV        100         // min, default, and max mid-trial reward interval, in msecs
#define     TH_DEFREWINTV        1000
#define     TH_MAXREWINTV        9999

#define     TH_NUMSPECOPS        10          // available special operations:
#define     TH_SOP_NONE          0           //    no special operation in use
#define     TH_SOP_SKIP          1           //    skip to end of special segment if saccade detected
#define     TH_SOP_SELBYFIX      2           //    "select" 1 of 2 fix tgts by fixating on it during special segment
#define     TH_SOP_SELBYFIX2     3           //    a slightly different version of "select by fix"
#define     TH_SOP_SWITCHFIX     4           //    "switch fix": enforce fixation on either of two tgts during special seg
                                             //    and reward only if animal switches to other tgt by trial's end.
#define     TH_SOP_RPDISTRO      5           //    "RP distro": subj rewarded/penalized based upon behavioral response
#define     TH_SOP_CHOOSEFIX1    6           //    "choose fixation tgt #1: enforce fixation on fix #1 by end of spec seg
#define     TH_SOP_CHOOSEFIX2    7           //    "choose fixation tgt #2: enforce fixation on fix #2 by end of spec seg
#define     TH_SOP_SEARCH        8           //    "search": Search for a designated target among 1+ distractors.
#define     TH_SOP_SELDUR        9           //    like "selByFix", but selection determines dur of seg after special seg

#define     TH_RPD_NRESPTYPES    4           // alternative response measures for the "RP distro" feature:
#define     TH_RPD_EYEVEL        0           //    eye velocity vector magnitude in deg/sec
#define     TH_RPD_HEVEL         1           //    horizontal eye velocity in deg/sec
#define     TH_RPD_VEVEL         2           //    vertical eye velocity in deg/sec
#define     TH_RPD_EYEDIR        3           //    eye velocity vector direction in deg CCW from rightward motion

                                             // op modes for electrical pulse stimulus generator module (SGM):
#define     SGM_SINGLE           0           //    single pulse presented, of specified amplitude and width
#define     SGM_DUAL             1           //    two pulses of distinct amp & width, separated by interpulse interval
#define     SGM_BIPHASIC         2           //    same as SGM_DUAL, but interpulse interval = 0
#define     SGM_TRAIN            3           //    seq of identical pulses occurring in one or more pulse trains
#define     SGM_BIPHASICTRAIN    4           //    similar to SGM_TRAIN, except pulses are biphasic (most used mode)
#define     SGM_NOOP             5           //    SGM not in use
#define     SGM_NMODES           6           //    total # of supported operational modes

                                             // range limits for various SGM parameters:
#define     SGM_MINPA            -128        //    pulse amplitude min/max (in 80mV increments)
#define     SGM_MAXPA            127
#define     SGM_MINPW            5           //    pulse width min/max (in 10us increments)
#define     SGM_MAXPW            250
#define     SGM_MINIPI           1           //    interpulse interval min/max (in 1ms increments)
#define     SGM_MAXIPI           250
#define     SGM_MINITI           1           //    intertrain interval min/max (in 10ms increments)
#define     SGM_MAXITI           250
#define     SGM_MINPULSES        1           //    #pulses per train min/max
#define     SGM_MAXPULSES        250
#define     SGM_MINTRAINS        1           //    #trains min/max
#define     SGM_MAXTRAINS        250

typedef struct tagSGMParams                  // control parameters for the pulse stimulus generator module (SGM).  note
{                                            // that some parameters do not apply to all op modes.
   int      iOpMode;                         //    motion mode -- one of the SGM_* defined constants
   BOOL     bExtTrig;                        //    if TRUE, use external trig to initiate pulse seq; else, s/w start.
   int      iAmp1, iAmp2;                    //    pulse amplitude in mV.  range [-10240..10160mV], res = 80mV.
   int      iPW1, iPW2;                      //    pulse width in us.  range [50..2500us], res = 10us.
   int      iPulseIntv;                      //    interpulse interval in ms.  range [1..250ms], res = 1ms.
   int      iTrainIntv;                      //    intertrain interval in ms.  range [10..2500ms], res = 10ms.
   int      nPulses;                         //    #pulses per train.  range [1..250].  (train modes only)
   int      nTrains;                         //    #trains per stimulus.  range [1..250].  (train modes only)
} SGMPARMS, *PSGMPARMS;

typedef struct tagTrialHeader                // trial header contains general trial attributes and control parameters:
{
   DWORD    dwFlags;                         //    flag bits -- see THF_ flag bit definitions
   int      iWeight;                         //    trial weight, for purposes of random sequencing of all trials in set
   int      iStairNum;                       //    staircase designation = 1..5; 0 if trial is not part of a staircase
   int      iStartSeg;                       //    if THF_KEEP, we save analog data from start of this segment until
                                             //    trial's end; if negative, entire trial is saved (same as 0).
   int      iFailsafeSeg;                    //    "failsafe" seg: if trial cut short b/c animal broke fix, trial data
                                             //    still saved if we reached start of this seg (-1 = trial must finish)
   int      iSpecialSeg;                     //    "special" segment during which a saccade-trig'd op may take place
   int      iSpecialOp;                      //    [schema version >=9] special op id; see TH_SOP_*** constants

                                             //    OBSOLETE in trial schema version >= 8:
   int      iOpenSeg;                        //    start seg for velocity stabilization on fix tgt #1 (ignored if <0)
   int      nOpenSegs;                       //    [schema version >=7] # contigous segs in which v. stab. in effect

   int      iMarkSeg1;                       //    [schema version >=3] display marker segment: if valid, a marker is
                                             //    drawn in data trace display at the start time for this segment.
   int      iMarkSeg2;                       //    [schema version >=3] a 2nd display marker segment
   int      iMTRIntv;                        //    [schema version >=4] mid-trial reward interval in msecs
   int      iMTRLen;                         //    [schema version >=4] mid-trial reward pulse length in msecs
   int      iXYDotSeedAlt;                   //    [schema version >=6] XY random dot seed optionally overrides display
                                             //    settings: -1 => use disp settings; 0 => auto-seed; >0 => fixed seed.
   int      nXYInterleave;                   //    # of XYscope tgts to interleave during trial (0,1 ==> no interleave)
   int      iSaccVt;                         //    saccade threshold velocity in deg/sec (for saccade-trig'd ops)
   int      reward1[3];                      //    reward pulse #1: [len in ms, WHVR numerator, WHVR denominator]
   int      reward2[3];                      //    reward pulse #2: [len in ms, WHVR numerator, WHVR denominator]
   float    fStairStrength;                  //    staircase strength (unitless) -- used by staircase trial sequencer
   WORD     wChanKey;                        //    CNTRLX "channel config" obj atch'd to this trial; if CX_NULLOBJ_KEY,
                                             //    no data is saved or displayed

   int      iSGMSeg;                         //    segment at which a pulse stimulus seq is initiated on SGM (if >= 0)
   SGMPARMS sgm;                             //    control params for the SGM pulse stim seq presented during trial
} TRLHDR, *PTRLHDR;

//=====================================================================================================================
// Segment "Header" Information
//=====================================================================================================================
#define     SGH_NOMARKER         0           // indicates no marker pulse should be delivered
#define     SGH_MINMARKER        0           // the range of valid marker pulse values; identifies the DOUT line on
#define     SGH_MAXMARKER        10          // which pulse is delivered.
#define     SGH_MINFIXACC        0.1f

// [DEPRECATED] The XYScope is unsupported a/o V4.0, and implementation removed a/o V5.0.
#define     SGH_MINXYFRAME       2           // [deprecated] XY frame range; also must be a multiple of min value!
#define     SGH_MAXXYFRAME       256

typedef struct tagSegHeader                  // the segment header parameters:
{
   // min & max duration of segment (ms). If different, actual segment duration is randomly generated via rand() for
   // each trial rep so that it lies in [min..max].
   // (as of v3.3.0) A negative value for duration indicates that a trial random variable x0..x9 has been assigned
   // to segment duration. Both min & max are ALWAYS set to the same RV -- so the value of the RV sets the segment 
   // duration. In this usage, allowed values are [-10 .. -1], and the index of the assigned RV is abs(dur) - 1.
   // (as of v5.0.1) Min & max duration can now be separately assigned to an integer or an RV, and need not be
   // assigned to the same RV. This change supports impl of the new "selDurByFix" special feature.
   int      iMinDur;
   int      iMaxDur;

   int      iFixTarg1;        // fixation targets -- these are zero-based indices into the trial's participating
   int      iFixTarg2;        // target list.  if negative, then no fixation target is assigned.
   float    fFixAccH;         // required H,V fixation accuracies during segment (deg subtended at eye).
   float    fFixAccV;         //
   int      iGrace;           // grace period (after segment start) during which fixation is not checked (msec).
   int      iXYFrame;         // [DEPRECATED a/o V5.0] update interval for XY scope targets participating in trial (ms)
   int      iMarker;          // marker pulse delivered at start of segment (0 = no pulse)
   BOOL     bChkResp;         // if TRUE, check for correct response during this segment (staircase trials only)
   BOOL     bEnaRew;          // if TRUE, enable periodic "mid-trial" rewards during this segment (special feature)

   //(as of v4.0.0) if TRUE, enable RMVideo sync flash during video frame marking segment start
   BOOL     bEnaRMVSync;
} SEGHDR, *PSEGHDR;

//=====================================================================================================================
// Per-Segment, Per-Target Trajectory Information
//=====================================================================================================================
#define     SGTJF_ON             (1<<0)      // bit flag: target on (set) or off (clear) during segment
#define     SGTJF_ABS            (1<<1)      // bit flag: target initial pos is absolute (set) or relative (clear)
#define     SGTJF_VSTABMODE      (0x03<<2)   // [trial schema v>=8]: velocity stabilization mode mask
#define     SGTJF_VSTABSNAP      (1<<4)      // [trial schema v>=8]: snap tgt to eye when vel. stab turns on
#define     SGTJF_VSTABMASK      (SGTJF_VSTABMODE|SGTJF_VSTABSNAP)

#define FLAGS_TO_VSTABMODE(w)    ((int) ((((DWORD)w) & SGTJF_VSTABMODE) >> 2))
#define VSTABMODE_TO_FLAGS(i)    ((((DWORD)i) << 2) & SGTJF_VSTABMODE)

#define     SGTJ_VSTABOFF        0           // [trial schema v>=8]: the possible velocity stabilization modes
#define     SGTJ_VSTABBOTH       1
#define     SGTJ_VSTABHONLY      2
#define     SGTJ_VSTABVONLY      3

// flag set if corresponding target trajectory parameter is assigned to a trial random variable rather than a numeric
// constant. IF set, the corresponding field in TRAJINFO is cast to an integer to retrieve the RV's zero-based index.
#define     SGTJF_POSH_ISRV      (1<<10)
#define     SGTJF_POSV_ISRV      (1<<11)
#define     SGTJF_VELH_ISRV      (1<<12)
#define     SGTJF_VELV_ISRV      (1<<13)
#define     SGTJF_ACCH_ISRV      (1<<14)
#define     SGTJF_ACCV_ISRV      (1<<15)
#define     SGTJF_PATVELH_ISRV   (1<<16)
#define     SGTJF_PATVELV_ISRV   (1<<17)
#define     SGTJF_PATACCH_ISRV   (1<<18)
#define     SGTJF_PATACCV_ISRV   (1<<19)
#define     SGTJF_ISRVMASK       (0x03FF<<10)

#define     SGTJ_POSMAX          999.99f     // these are just to limit input to restricted ranges; these restrictions
#define     SGTJ_VELMAX          999.99f     // do NOT guarantee that trial will run properly...
#define     SGTJ_ACCMAX          9999.99f

typedef struct tagTgSegTraj                  // the per-segment, per-target trajectory info:
{
   DWORD    dwFlags;                         // trajectory flags -- see SGTJF_ flag bit definitions

   float    fPosH, fPosV;                    // target window initial position at segment start (deg subtended at eye)
   float    fVelH, fVelV;                    // target window's constant velocity during segment (deg/sec)
   float    fAccH, fAccV;                    // target window's constant acceleration during segment (deg/sec^2)

   float    fPatVelH, fPatVelV;              // target pattern's constant velocity during segment (deg/sec)
   float    fPatAccH, fPatAccV;              // [seg schema >= 2] pattern's constant acc during segment (deg/sec^2)
} TRAJINFO, *PTRAJINFO;


//=====================================================================================================================
// Intra-trial Tagged Section Descriptor
//=====================================================================================================================
#define     SECTIONTAGSZ         18          // max #chars in a trial section tag, INCLUDING the terminating null char

typedef struct tagTrialSection               // a tagged section of contiguous segments within a trial
{
   char     tag[SECTIONTAGSZ];               //    null-terminated name tag for segment
   char     cFirstSeg;                       //    index of first segment in section
   char     cLastSeg;                        //    index of last segment in section
} TRIALSECT, *PTRIALSECT;



//=====================================================================================================================
// MAESTRO CONTINUOUS RUN-SPECIFIC DEFINITIONS
//=====================================================================================================================

#define     MAXSTIMULI           20          // maximum # of stimulus channels per continuous-mode run
#define     MAXTGTSINXYSEQ       25          // maximum # of XY scope targets participating in the XYSEQ stimulus
#define     MAX_XYSEQVECS        32          // max # of different motion vectors for XY targets in 'XYseq' xstim
#define     MAX_ACTIVETGTS       5           // maximum # of targets in ContMode's "active target list"

#define     STIM_NLASTMARKER     SGH_MAXMARKER           // marker pulses delivered on DOUT<1..max>; 0 ==> "OFF"

#define     STIM_NTYPES          5           // available stimulus channel types:
#define     STIM_ISCHAIR         0           //    animal chair (trial target CX_CHAIR)

// OKNDRUM NO LONGER SUPPORTED FOR DATA FILE VERSIONS >= 7.  FOR DATA FILE VERSIONS < 7, we need to increment the
// channel types below by 1 to get the correct value; also STIM_NTYPES = 6 in this case.
// #define     STIM_ISOKN           1

#define     STIM_ISFIBER1        1           //    fiber-optic target #1 (trial target CX_FIBER1)
#define     STIM_ISFIBER2        2           //    fiber-optic target #2 (trial target CX_FIBER2)
#define     STIM_ISPSGM          3           //    pulse stimlus generator module
// [deprecated] specialized random-motion sequence on a set of XYScope targets
#define     STIM_ISXYSEQ         4           //    specialized random-motion seq on a set of XY scope targets; only one
                                             //    instance of this stimulus type is allowed per continous-mode run!!

#define     STIM_NSTDMODES       2           // motion modes for "standard" stim types (_ISCHAIR, _ISFIBER*, _ISOKN)
#define     MODE_ISSINE          0           //    sinuosoidal
#define     MODE_ISPULSE         1           //    trapezoidal pulse (with non-zero rise & fall times)

#define     STIM_NPSGMMODES      SGM_NMODES-1   // (SGM_NOOP is not used in stimulus runs!)

#define     STIM_NXYSEQMODES     4           // [deprecated]motion modes applicable to the XYSEQ stimulus type:
#define     MODE_ISSPARSEDIR     0           //    direction randomized.  one randomly chosen XY tgt moves each seg
#define     MODE_ISDENSEDIR      1           //    all targets move, directions separately randomized each seg
#define     MODE_ISSPARSEVEL     2           //    velocity randomized.  one randomly chose XY tgt moves each seg
#define     MODE_ISDENSEVEL      3           //    all targets move, velocities separately randomized each seg

#define     STIM_NMAXMODES       5           // maximum # of motion modes for any type

typedef struct tagXYseqMotion                // [deprecated] the motion parameters for an XYseq stimulus channel:
{
   int   iOpMode;                            //    motion mode -- MODE_ISSPARSEDIR, etc.
   int   iRefresh;                           //    XY scope refresh period, in millisecs
   int   nSegs;                              //    # of distinct segments of random motion
   int   iSegDur;                            //    duration of each segment, in ms (must be multiple of refresh period)
   int   iSeed;                              //    seed for generating random directions or velocities
   int   nChoices;                           //    # of different directions (or velocities) randomized
   float fAngle;                             //    offset angle (MODE_IS**DIR) or direction of motion (MODE_IS**VEL)
   float fVel;                               //    velocity of motion (for dir modes) or max velocity (for vel modes)
   float fOffsetV;                           //    offset velocity (for vel modes only)
} XYSEQSTIM, *PXYSEQSTIM;

typedef struct tagSineMotion                 // the motion parameters for sinewave stimuli:
{
   int   iPeriod;                            //    period in msecs (>= 10ms)
   int   nCycles;                            //    # of complete cycles in stimulus (>=1)
   float fAmp;                               //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   float fPhase;                             //    phase in deg: [-180.0 .. 180.0]
   float fDirec;                             //    direction of motion, CCW angle from x-axis [-180.0..180.0]
} SINESTIM, *PSINESTIM;

typedef struct tagPulseMotion                // the motion parameters for trapezoidal pulse stimuli:
{
   BOOL  bBlank;                             //    if TRUE, active targets are blanked during pulse (for CHAIR stim)
   int   iPulseDur;                          //    duration of pulse in ms (>= 2ms)
   int   iRampDur;                           //    duration of rising-edge and falling-edge ramps (>= 2ms)
   float fAmp;                               //    velocity amplitude, in deg/sec: [-9999 .. 9999].
   float fDirec;                             //    direction of motion, CCW angle from x-axis [-180.0..180.0]
} PULSESTIM, *PPULSESTIM;

#define     STIM_NCOMMON         5           // # of common parameters in a stimulus channel definition
#define     MAXSTIMPARAMS        15          // max # of total params ("common" + "motion") defining a stimulus channel
typedef struct tagRunChannel                 // defn of a stim chan w/in a ContMode run, in a CXDRIVER-compatible form
{                                            //
   BOOL     bOn;                             //    TRUE = stimulus should be played during run; FALSE = stim disabled
   int      iMarker;                         //    OFF (0), or DOUT ch# on which pulse is delivered at stimulus start
   int      iType;                           //    type of stimulus:  see STIM_IS** constants
   int      iStdMode;                        //    motion mode for the "standard" stim types: MODE_ISSINE or _ISPULSE
   int      tStart;                          //    start time of stimulus traj within the run's duty cycle, in ms.
   union                                     //    motion param list -- usage varies with stimulus type & motion mode.
   {
      SINESTIM    sine;
      PULSESTIM   pulse;
      SGMPARMS    sgm;
      XYSEQSTIM   xy;
   };
} STIMCHAN, *PSTIMCHAN;


typedef struct tagRun                        // definition of a ContMode run in a CXDRIVER-compatible form
{
   int      iDutyPeriod;                     //    duty period in milliseconds
   int      iDutyPulse;                      //    OFF (0), or DOUT ch# on which pulse is delivered per duty cycle
   int      nAutoStop;                       //    auto-stop the run after this many cycles elapsed (0 = no auto-stop)
   float    fHOffset;                        //    horizontal position offset in deg subtended at eye
   float    fVOffset;                        //    vertical position offset in deg subtended at eye
   int      nStimuli;                        //    # of stimulus channels defined for this run
   STIMCHAN stim[MAXSTIMULI];                //    the individual stimulus channel definitions
   int      nXYTgts;                         //    # of XY scope targets participating in an XYseq stimulus in this run
   CXTARGET xyTgts[MAXTGTSINXYSEQ];          //    defns of those targets (in format used for storing to file)
   float    fCtrX[MAXTGTSINXYSEQ];           //    center location of each XY target's window
   float    fCtrY[MAXTGTSINXYSEQ];           //
} CONTRUN, *PCONTRUN;



//=====================================================================================================================
// MAESTRO PERTURBATION WAVEFORM-SPECIFIC DEFINITIONS
//=====================================================================================================================

#define     PERT_NTYPES          4           // available perturbation waveform types (NOTE: all have unit amplitude):
#define     PERT_ISSINE          0           //    sinusoidal waveform
#define     PERT_ISTRAIN         1           //    pulse train
#define     PERT_ISNOISE         2           //    uniform random noise
#define     PERT_ISGAUSS         3           //    (v1.3.2) Gaussian-distributed noise w/zero mean and unit variance

#define     PERT_NCMPTS          10          // # of different trajectory cmpts that can be affected by a perturbation
#define     PERT_ON_HWIN         0           // a pert can affect any of one of these cmpts in a trial tgt's traj:
#define     PERT_ON_VWIN         1           //    horiz or verti window velocity
#define     PERT_ON_HPAT         2           //    horiz or verti pattern velocity
#define     PERT_ON_VPAT         3
#define     PERT_ON_DWIN         4           //    (as of v1.3.2) the direction of a target's window or pattern
#define     PERT_ON_DPAT         5           //    velocity vector (was introduced to provide directional noise)
#define     PERT_ON_SWIN         6           //    (as of v2.1.2) the amplitude of a target's window or pattern 
#define     PERT_ON_SPAT         7           //    velocity vector (was introduced to provide speed noise)
#define     PERT_ON_DIR          8           //    (as of v2.1.3) perturb direction of BOTH window and pattern vel vecs
#define     PERT_ON_SPD          9           //    (as of v2.1.3) perturb amplitude of BOTH window and pattern vel vecs

typedef struct tagSinePert                   // defining parameters for sinuosoidal perturbation:
{
   int   iPeriod;                            //    period in msecs (>= 10ms)
   float fPhase;                             //    phase in deg: [-180.0 .. 180.0]
} SINEPERT, *PSINEPERT;

typedef struct tagPulsePert                  // defining parameters for trapezoidal pulse train perturbation:
{
   int   iPulseDur;                          //    duration of pulse in ms (>= 10ms)
   int   iRampDur;                           //    duration of rising-edge and falling-edge ramps (>= 0ms)
   int   iIntv;                              //    interval between pulses in ms (> 2*rampD + pulsD)
} TRAINPERT, *PTRAINPERT;

typedef struct tagNoisePert                  // defining params for uniform or Gaussian random noise perturbation:
{
   int   iUpdIntv;                           //    update interval in ms (>= 1ms)
   float fMean;                              //    mean noise level [-1..1]
   int   iSeed;                              //    (as of v1.3.2) seed for RNG.  if 0, the seed is randomly chosen
} NOISEPERT, *PNOISEPERT;

typedef struct tagPertDef                    // complete definition of a perturbation
{
   int iType;                                //    perturbation type -- one of the PERT_IS*** constants
   int iDur;                                 //    duration of the perturbation in ms (>= 10ms)
   union                                     //    type-specific defining parameters
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
#define EL_DEFOFS   0
#define EL_MINOFS  -2000
#define EL_MAXOFS   2000
#define EL_DEFGAIN  300
#define EL_MINGAIN  50
#define EL_MAXGAIN  2000

// default value and allowed range for the width of the "sliding-average" window used to smooth velocity signals 
// generated by differentiating (center-point difference) Eyelink position data (in #samples, which should be ms)
#define EL_DEFSMOOTHW 20
#define EL_MINSMOOTHW 3
#define EL_MAXSMOOTHW 50

// recording type -- off (Eyelink not in use), monocular left or right, or binocular
#define EL_NOTINUSE   0
#define EL_MONO_LEFT  1
#define EL_MONO_RIGHT 2
#define EL_BINOCULAR  3

typedef struct tagELCoord
{
   float fx;
   float fy;
} ELCOORD, *PELCOORD;

// Eyelink sample data passed to MaestroDRIVER from worker thread in Maestro (when Eyelink tracker in use)
typedef struct tagELSample
{
   // sample timestamp when tracker camera imaged eye (ms since current recording session started on tracker)
   // NOTE - changed from UINT32 to DWORD for MEX-file compilation
   DWORD ts; 
   // flags indicating whether or not sample includes data for L=0,R=1 eyes
   BOOL gotEye[2];
   // calibrated gaze position in visual degrees for L=0, R=1 eyes
   ELCOORD pos[2];
   // computed gaze velocity in deg/sec (differentiated and smoothed) for L=0, R=1 eyes
   ELCOORD vel[2];
} ELSAMP, *PELSAMP;

// indices into position and velocity arrays in Eyelink sample data structure
#define EL_LEFT   0
#define EL_RIGHT  1

#endif   // !defined(CXOBJ_IFC_MEX_H__INCLUDED_)
