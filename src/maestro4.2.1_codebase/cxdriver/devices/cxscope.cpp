//=====================================================================================================================
//
// cxscope.cpp : Abstract interface CCxScope, which defines the XY scope hardware device interface for CXDRIVER.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// The "XY scope" -- a large-screen, high-performance XY oscilloscope -- is an important target platform in CXDRIVER.
// A wide variety of visual targets are presented on this medium, including spots and various kinds of random-dot
// patterns.  The X, Y, and trigger inputs of the scope are driven by an in-house "dotter board" which, in turn, is
// controlled by a DSP-based hardware device residing in the host system.
//
// CCxScope is an ABSTRACT "interface" class that attempts to expose the XY scope controller's functionality in a
// device-independent manner.  It encapsulates the XY scope's display geometry and other important animation
// parameters.  It converts CNTRLX-style target definitions to an internal format that is used by all implementations
// of the interface.  Most importantly, it translates target window dimensions and positions from the CNTRLX/CXDRIVER
// coordinate system to the XY scope device's own "pixel"-based coordinate system.
//
// CCxScope treats the XY scope display as a grid of 65536x65536 pixels, with (0,0) at the bottom left corner of the
// screen and (65535, 65535) at the top right corner.  It is assumed that the subject's line-of-sight (LOS) passes
// through the center of the screen at a perpendicular angle, so that the center corresponds to the origin (0deg, 0deg)
// in CXDRIVER's coordinate system.  With this assumption and the display geometry (distance to eye, height and width
// of screen in mm), CCxScope can convert between the two coordinate systems.  See TranslateToDevCoords() for details.
//
// CCxScope is based on the C-style XYSCOPE module from the older "cntrlxPC" application.  That module targeted three
// DSP boards from Spectrum Signal Processing (SSP):  Detroit, Dakar F5, and Daytona.  A custom firmware program,
// "XYCORE", implements the "XY scope controller" functionality on each of these boards.  The "host" side merely writes
// the target and animation information to the board; XYCORE handles the details of representing the targets internally
// and drawing them via the dotter board.  We have designed practical implementations of CCxScope for each of these SSP
// boards.  Using a DSP solution is essential, given the stringent performance requirements imposed on the XY scope
// controller:
//
//    1) Provide a hardware interface with the in-house "dotter board".  The dotter board was originally designed to
// be compatible with the DSP~LINK3 interface specification used by the SSP boards.  It drives the analog input signals
// that control what is seen on the XY scope display: X and Y position (as single-ended or differential signals) and
// scope trigger "Z".  The fundamental "dot draw" cycle of the dotter board starts with a "delay" period of N clock
// cycles, followed by a "trigger ON" period of M clock cycles.  During the delay phase, the X and Y outputs are
// updated with the trigger off -- this moves the scope's electron beam to the position of the next dot to be drawn.
// The beam is then turned on during the "ON" phase, drawing the dot on the screen.  CCxScope (and its implementations)
// provide a means for changing these two timing parameters to adapt the system to the performance specifications of
// the oscilloscope in use.  For example, if the delay phase is too short, the dots will have "tails" on them because
// the beam is still moving when the trigger turns ON.
//    2) Dot drawing speed.  The dotter board can run at one of two clock rates, 10MHz or 1MHz.  With the 10MHz clock,
// a delay phase of 10 clock ticks and an "ON" phase of 1 tick, the minimum achievable dot draw cycle would be 1.1us.
// Longer cycles are attributed to processing time on the XY scope controller.  Thus, the device should employ a fast
// enough CPU and fast peripherals so that this processing time does not significantly lengthen the dot draw cycle.
// Tests on the Detroit board revealed dot draw cycles in the range of 1.3 to 4 us, varying with the complexity of the
// target type being drawn.  REMEMBER:  The faster the XY scope controller, the closer we get to achieving the minimum
// draw cycle, the more dots we can draw per display frame.  A faster controller permits more complex animations!
//    3) CCxScope pure virtual methods requiring implementation.
//       LoadParameters() ==> Downloads target definitions and other animation parameters, stored in the protected
// member CCxScope::m_Parameters, to the XY scope controller and prepares to begin frame-by-frame animation of the
// targets defined.  For the SSP boards mentioned above, we merely write the parameters structure to the board and
// issue a command to read the parameters and prepare all targets.  Any XYCORE program would need to build some sort of
// efficient representation of each target, initializing the positions of each of its "dots".
//       DoFrame() ==> Download set of per-target update records, stored in protected member CCxScope::m_NextUpdate[],
// and initiate a display frame update.
//    4) Be capable of animating each CCxScope target type -- see descriptions in later section.
//
// By creating a hardware interface class, we hope to isolate CXDRIVER's *usage* of a device from the details of how
// the device actually works, and how we communicate with it.  To create a "practical implementation" of the interface,
// we derive a new class from the abstract interface and implement each of its pure virtual methods (and possibly
// override other "non-pure" virtual methods) to support the target board.
//
// CXDRIVER is designed as an RTSS process, running in the Real-Time eXtension (RTX -- see CREDITS) subsystem for
// WinNT.  The RTX API provides kernel-level access, permitting direct communications with hardware and thus avoiding
// some of the complexities and limitations of writing or working with manufacturer-supplied device drivers.  As a
// component of CXDRIVER, target implementations of a hardware interface are essentially specialized device drivers.
// They use methods from the RTX API to perform the low-level operations typically found in a device driver (read and
// write to port I/O addresses, mapping device memory, searching the PCI bus, etc.).
//
// ==> Base class CDevice.
// All CXDRIVER hardware interfaces are derived from the abstract class CDevice.  CDevice defines several basic device
// operations, such as Open(), Close(), and Init().  It also has built-in support for PCI devices (most CXDRIVER boards
// are hosted on the PCI bus) and COFF loading support for DSP devices based on the TI C6x- or C4x-series processors.
// Note that derived classes inherit a number of pure virtual methods from this class.  Any practical device class must
// implement these methods, which handle a number of device-specific operations.  See CDevice for details.
//
// ==> Usage.
//    ChangeDisplay()   ==> Updates the XY scope device's display geometry, trigger timing (delay and "ON" phase
// durations), and policy for selecting a seed for generating target random-dot patterns.  Invoking this method will not
// affect an ongoing animation.
//
//    ClearTargets()    ==> Empties the list of targets animated by the XY scope device.
//    AddTargets()      ==> Appends a target to the XY scope target list.  NOTE that the order in which targets are
// added to the list is significant.  The per-target update records for a display frame are assumed to be in this order!
//
//    Load()            ==> Loads the current target list and any other animation parameters onto the XY scope device
// and prepares for frame-by-frame animation.
//    Update()          ==> Display frame update.  Caller provides the pos changes in each target's window and pattern
// since the last frame update, as well as the interval between the updates.  From these we prepare the per-target
// frame update records, with the motion information converted into pixel displacements, etc.  These records are
// written to the XY scope device and a "do frame" is issued to start the next display frame.  NOTE that CCxScope and
// its implementations do NOT animate the targets independently.  CXDRIVER must invoke Update() at regular intervals to
// achieve the desired animation!!  Update() is designed for use in time-critical code sections.  If the device is not
// ready to initiate a display frame update, the method fails and the requested display frame is "dropped".
//
// ==> Description of target types supported by CCxScope.
// CNTRLX currently offers 11 different XY scope target types.  CCxScope maps each target type to one of 13 "device-
// level" target types, which any practical implementation of CCxScope must realize.  The two additional device-level
// types are optimized realizations of selected CNTRLX XY target types, as explained below.  These optimized tgt types
// evolved from the development of XYCORE on the Spectrum DSP boards, but we decided to make them part of the CCxScope
// interface.  Note that most XY targets are characterized by both a target "window" and a target "pattern".
//
// 11may2011. IMPORTANT IMPLEMENTATION CHANGE effective in Maestro v2.7.0. ******************************************** 
// Target pattern displacement vectors sent by CXDRIVER are now specified in the target window's frame of reference, 
// ie, they specify how the pattern moves WRT the target window, NOT WRT the XYScope screen. All target types with an 
// independently moving target pattern were affected by this change (all target types except DOTARRAY, FULLSCREEN, 
// STATICANNU, ORIBAR, and OPTICFLOW; the dots already moved WRT the window in the OPTICFLOW target). Basically, we now 
// add the target window displacment vector when calculating the new dot positions during an update frame:
//    BEFORE: x(n) = x(n-1) + hPat; y(n) = y(n-1) + vPat
//       NOW: x(n) = x(n-1) + hWin + hPat; y(n) = y(n-1) + vWin + vPat
// ********************************************************************************************************************
// 
// CNTRLX type       Device-level Type       Description
// --------------------------------------------------------------------------------------------------------------------
// RECTDOT           CCxScope::DOTARRAY      A rectangular array of dots.  Defined by #dots, width of dot array, and
//                                           spacing between dots.  No target "pattern"; all dots move IAW "window"
//                                           trajectory.  Most common usage sets spacing to zero, resulting in a point-
//                                           like target the intensity of which depends on #dots.
//
// CENTER            CCxScope::RECTWINDOW,   Full-screen random-dot pattern visible only inside a rectangular window.
//                   CCxScope::FULLSCREEN    Pattern & window can move independently.  Defined by #dots in pattern, and
//                                           window W,H.  If window fills whole XY scope display and never moves, the
//                                           FULLSCREEN tgt type can be used instead of RECTWINDOW [see AddTarget()].
//
// SURROUND          CCxScope::RECTHOLE,     Full-screen random-dot pattern visible only outside a rectangular window.
//                   CCxScope::STATICANNU    Pattern & window can move independently.  Defined by #dots in pattern, and
//                                           window W,H.  If neither window nor pattern ever move during animation, the
//                                           STATICANNU tgt type can be used instead of RECTHOLE [see AddTarget()].
//
// RECTANNU          CCxScope::ANNULUS,      Full-screen random-dot pattern visible only inside a rectangular annulus.
//                   CCxScope::STATICANNU    Pattern & annular window can move independently.  Defined by #dots in
//                                           pattern, and W,H of inner & outer rects defining annulus.  Inner rect can 
//                                           be offset WRT target center. If neither window nor pattern move during 
//                                           animation, STATICANNU can be used instead of ANNULUS [see AddTarget()].
//
// FASTCENTER        CCxScope::OPTRECTWIN    Random-dot pattern limited to a rectangular window.  Pattern & window can
//                                           move independently.  Defined by #dots in pattern, window W&H.  Since all
//                                           dots are restricted to the window, this is a much faster implementation
//                                           than RECTWINDOW -- but has some animation artifacts at high speeds...
//
// FCDOTLIFE         CCxScope::DOTLIFEWIN    Same as OPTRECTWIN, except that each dot in target is assigned a random
//                                           "lifetime". When lifetime expires or dot hits a window border, the dot is
//                                           randomly repositioned within rect and its dot life is reset.  CNTRLX
//                                           specifies dot life in degrees travelled or milliseconds elapsed.  CCxScope
//                                           methods AddTarget() & Update() translate these to arbitrary device units.
//                                           Update() also packs the per-refresh "dot life decrement" for a target in
//                                           the update record.  See AddTarget() and Update() for details.
//
// FLOWFIELD         CCxScope::OPTICFLOW     Very different from the other XY scope target types:  Simulates an optic
//                                           flow field, with randomly-positioned dots moving radially away from or
//                                           toward a "focus of expansion" (FOE).  The boundaries of the target window
//                                           are circular rather than rectangular, and each dot moves at a different
//                                           speed depending upon its radial distance from the FOE.  A consequence of
//                                           the latter feature is that XYCORE must be responsible for animating dot
//                                           motion; CCxScope only provides a scaling factor which is used in per-dot
//                                           velocity calculations.  See methods AddTarget() & Update() for details.
//
// ORIENTEDBAR       CCxScope::ORIBAR        Oriented rectangular bar or thin line of randomly arranged dots.  Defined
//                                           by #dots, W,H of bar, and "drift axis" angle (specifies orientation).  No
//                                           target "pattern"; all dots move IAW "window" trajectory.
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation failed to account for "aspect ratio" -- ie, screen width and
//    height in mm not equal. The rotation of the bar's dots that is performed during target initialization in XYCORE
//    was done in device coordinates (pixels), where the grid is 65536x65536. If the screen width and height in mm 
//    are not the same, then the bar's orientation was actually smaller than it should have been. The corrected 
//    implementation performs the rotations in screen coordinates in mm, then converts each dot's coordinates in mm
//    to pixels. This fix requires sending the screen width and height in mm to XYCORE. PARAMETER structure was 
//    modified to include fields that report the display geometry (w, h, distToEye) in mm.
//    ***************************************************************************************************************
//
// NOISYDIR         CCxScope::DL_NOISEDIR    Similar to DOTLIFEWIN, but the individual dots move in randomly chosen
//                                           directions.  Target definition includes two special parameters, the "noise
//                                           update interval" M in milliseconds, and the "noise offset range" N in deg.
//                                           After every M milliseconds, each dot's direction of motion is recomputed
//                                           by adding, to the pattern direction angle, a random offset angle chosen
//                                           from [-N:N], in integer increments.  The dot moves along this direction
//                                           for the next M milliseconds, at which point a new random dir is computed.
//                                           To reduce overhead on the XYCORE side, CCxScope converts the per-frame
//                                           target pattern displacement vector from Cartesian pixel coordinates
//                                           (HV,VV) to polar screen coordinates: (Rmm, thetaDeg).  See Update() for 
//                                           details.  If N is 0 (no noise), then this tgt type is mapped to DOTLIFEWIN 
//                                           instead!!!
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation was incorrect on two counts: The displacement vector was 
//    converted to polar coordinates in device pixels, which fails to account for "aspect ratio" -- ie, screen width and
//    height in mm not equal. Instead, the displacement vector must be converted to Cartesian screen coordinates in mm
//    prior to conversion to polar coordinates. Then XYCORE must do its calculations in mm and finally convert back to
//    device pixels. Secondly, for typical velocities, the per-frame deltas in x or y will be a few pixels. With int
//    arithmetic, truncation error will have a significant effect on the observed velocities. Thus, it was necessary to
//    keep track of the (scaled) truncation error on a per-dot basis and add the truncation error from the previous
//    frame back in when calculating the displacement for the current frame.
//    ***************************************************************************************************************
//
// COHERENTFC        CCxScope::OPTCOHERENT   Similar to OPTRECTWIN, but introduces the notion of percent coherence.
//                                           If the target has N% coherence, then ON EVERY FRAME UPDATE, every target
//                                           dot has a N% chance of moving coherently -- ie, IAW the motion vector for
//                                           that frame -- and a (100-N)% chance of being randomly repositioned within
//                                           the target window.  This implementation is based upon a 1988 J.Neurosci.
//                                           paper by Newsome & Pare.
//
// NOISYSPEED        CCxScope::DL_NOISESPEED Similar to DL_NOISEDIR, but the noise is in dot speed rather than direc.
//                                           Here the "noise offset range" N is an integer percentage in [0..300].
//                                           After every M milliseconds, a number D is randomly chosen from [-N:N].
//                                           During each update frame, the dot's radial speed R is given by
//                                           Ro + D*Ro/100, where Ro is the nominal radial speed of the entire dot
//                                           pattern, in pixels/frame.  See Update() for details.  If N is 0 (no
//                                           noise), then this tgt type is mapped to DOTLIFEWIN instead!!!
//                                           [23jul2006] Increase max N from 100 to 300.  This means that an individual 
//                                           dot may move in the opposite direction from the nominal velocity vector 
//                                           of the target.
//                                           [16jul2007] Noise granularity is now in 1% increments instead of 0.1%, as 
//                                           of Maestro v2.1.2.
//                                           [04sep2007] Added support for a second, multiplicative speed noise 
//                                           algorithm. Ro = (R*2^X) / E(2^X), where X is uniformly chosen over [-N:N],
//                                           the "noise limit" N is restricted to an integer in [1..7], and E() is 
//                                           expected value of 2^X, which = [2^N - 2^(-N)] / [2 * N * ln(2)]. The 
//                                           field XYPARMS.fInnerX must be nonzero to select this method.
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation was incorrect on two counts -- SAME as described for the
//    NOISYDIR target type.
//    ***************************************************************************************************************
//
// CREDITS:
// 1) Real-Time eXtenstion (RTX) to WinNT by VenturCom, Inc (www.vci.com).  RTX gives the WinNT OS real-time-like
// characteristics and provides kernel access for direct communications with hardware devices.
//
//
// REVISION HISTORY:
// 25sep2002-- Adapting from C source code module XYSCOPE from an older version of CXDRIVER.  Implementation-specific
//             methods moved to separate derived classes...
// 21jul2003-- Introduced new target type DOTLFNOISY.
// 12sep2003-- Mod to DOTLFNOISY implementation.  The scale factor 2^6 on the radial component of the pattern
//             displacement vector was inappropriate to encode the large displacements resulting from instantaneous
//             position changes in the DOTLFNOISY target.  See Update() for changes.
// 29sep2003-- Mod to DOTLFNOISY implementation.  Dot direction offset range endpoint N can be 0, meaning no noise.  In
//             this case the target would behave exactly like DOTLIFEWIN, so we map it to that target type in
//             AddTarget().
// 13oct2004-- Introduced new target type COHERENTFC<-->OPTCOHERENT.
// 11apr2005-- Mod to DOTLFNOISY implementation.  Dot directions are no longer randomized on every refresh.  Instead
//             they are randomized once per "direction update interval", which is part of the DOTLFNOISY target's
//             definition.  This means that XYCORE must maintain a copy of each dot's current direction between frames.
// 07jan2006-- Introduced new target type NOISYSPEED<-->DL_NOISESPEED.  Also, NOISYDOTS/DOTLFNOISY is now
//             NOISYDIR/DL_NOISEDIR.
// 19jun2006-- AddTarget() modified to support offsetting inner rect of RECTANNU target WRT target center, using new 
//             fields XYPARMS.fInnerX, .fInnerY.  No change required to XYCORE!  Effective Maestro 2.0.1.
// 23jul2006-- Comments added to indicate that max noise offset range % for NOISYSPEED was changed from 100 to 300.
// 16jul2007-- Noise offset range, N, for NOISYSPEED is [0..300] with a granularity of 1% instead of 0.1%.
// 04sep2007-- Added support for alternative method of noise generation for NOISYSPEED target.
// 09may2011-- Corrected implementation of ORIENTEDBAR target. Rotation of bar dot locations to the specified angle
//             had been done in device coordinates, which is invalid because the device grid is artificially square,
//             65536 x 65536, when the real display width and height usually are not equal.
//          -- Added fields PARAMETERS.wWidthMM, .wHeightMM, .wDistMM to report display geometry to XYCORE.
// 10may2011-- Corrected implementation of NOISYDIR and NOISYSPEED targets. Computation of per-frame displacement 
//             vector in polar device coordinates was incorrect -- same reason as for ORIENTEDBAR. In addition, we now
//             keep track of the truncation error lost when doing integer arithmetic, and add it back in on the next
//             frame update. Since the dots move independently, we must do this on a per-dot basis.
// 11may2011-- Pattern displacement vectors from CXDRIVER are now specified WRT the target window's frame of reference,
//             not the screen's frame of reference. Adjusted the implementation of affected target types, ie, those
//             having a target "pattern" independent of the target window. Basically, on each update frame, we compute
//             each dot's new location as: xDot(n) = xDot(n-1) + hWin + hPat, yDot(n) = yDot(n-1) + vWin + vPat.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.
//=====================================================================================================================

#include "cxscope.h"


const double CCxScope::XYDEV_TIMEOUT   = 300000.0;

CCxScope::CCxScope( const CDevice::DevInfo& devInfo, int iDevNum )
   : CDevice( devInfo, iDevNum )
{
   ::memset( (PVOID) &m_Parameters, 0, sizeof(CCxScope::Parameters) );
   ::memset( (PVOID) &(m_NextUpdate[0]), 0, MAX_TARGETS*sizeof(CCxScope::UpdateRec) );

   m_iDistToEye = 700;
   m_iWidth = 300;
   m_iHeight = 300;
   m_iDrawDelay = 10;
   m_iDrawDur = 1;
   CalcConversionFactors();
   m_bAutoSeed = TRUE;
   m_dwFixedSeed = 0x01234567;
}

CCxScope::~CCxScope()
{
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

/**
 Get width of XYScope display screen in visual degrees subtended at the eye, based on current display geometry.
 @return Display width in visual degrees subtended at eye.
*/
double RTFCNDCL CCxScope::GetScreenW_deg()
{
   return(2.0 * ::atan2( double(m_iWidth)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD);
}

/**
 Get height of XYScope display screen in visual degrees subtended at the eye, based on current display geometry.
 @return Display height in visual degrees subtended at eye.
*/
double RTFCNDCL CCxScope::GetScreenH_deg()
{
   return(2.0 * ::atan2( double(m_iHeight)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD);
}

//=== ChangeDisplay ===================================================================================================
//
//    Update the XY scope display configuration.  Should be called only when the XY scope controller is idle.  Any
//    parameters violating range restrictions are "silently" corrected.
//
//    ARGS:       d        -- [in] distance to eye in mm [MIN_DISTTOEYE..no max].
//                w        -- [in] display width in mm [MIN_DIMENSION..no max].
//                h        -- [in] display height in mm [MIN_DIMENSION..no max].
//                iDelay   -- [in] start of draw cycle to trigger "ON", in dotter board clock cycles [1..MAX_TRIGDEL].
//                iDur     -- [in] trigger "ON" duration, in dotter board clock cycles [1..MAX_TRIGLEN-iDelay].
//                bAutoSeed-- [in] if TRUE, seed used by XY scope controller to generate targets' random-dot patterns
//                            is randomly chosen each time tgts loaded. else, "fixed" seed value is used each time.
//                dwSeed   -- [in] value for the "fixed" seed.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxScope::ChangeDisplay( int d, int w, int h, int iDelay, int iDur, BOOL bAutoSeed, DWORD dwSeed )
{
   m_iDistToEye = (d < MIN_DISTTOEYE) ? MIN_DISTTOEYE : d;
   m_iWidth = (w < MIN_DIMENSION) ? MIN_DIMENSION : w;
   m_iHeight = (h < MIN_DIMENSION) ? MIN_DIMENSION : h;
   m_iDrawDelay = (iDelay < 1) ? 1 : ((iDelay > MAX_TRIGDEL) ? MAX_TRIGDEL : iDelay);
   m_iDrawDur = (iDur < 1) ? 1 : ((iDur > MAX_TRIGLEN-m_iDrawDelay) ? MAX_TRIGLEN-m_iDrawDelay : iDur);
   m_bAutoSeed = bAutoSeed;
   m_dwFixedSeed = dwSeed;
   CalcConversionFactors();
}


//=== ClearTargets, AddTarget =========================================================================================
//
//    These methods control the composition of the XY scope target list.  ClearTargets() empties the list, while
//    AddTarget() appends a new target to the end of the list.
//
//    AddTarget() also translates the Maestro-specific target definition to an internal format expected by the XY scope
//    controller.  Below is a summary of the translations made, some of which are not straightforward:
//
//    1) Target type mapping: Most Maestro->XYCORE target type mappings are straightforward -- see file header. Maestro
//       tgt types CENTER, SURROUND, and RECTANNU are usually mapped to RECTWINDOW, RECTHOLE, and ANNULUS, resp.
//       However, under certain special circumstances, an "optimized" type can be used:
//
//       a. SURROUND and RECTANNU can be represented by a static rectangular annulus (internal type STATICANNU) IF both
//       the target window and the target pattern are stationary throughout the animation.  In this case, the caller
//       should set the 'bOptimize' flag AND provide the target's initial (and final!) position.  Normally, all targets
//       are drawn initially centered on the display.  However, STATICANNU will *never* move, so we must account for
//       any position offset when preparing its border rectangles.
//
//       b. CENTER may be represented as a moving, full-screen dot pattern (internal type FULLSCREEN) IF the target
//       window never moves.  In this case, the caller should again set the 'bOptimize' flag and provide the target's
//       initial position.  Based on the target window's pos and dimensions, we determine whether or not it will cover
//       the entire screen.  If so, CENTER is mapped to FULLSCREEN; else, it still gets mapped to RECTWINDOW.
//
//    1a) If the noise offset range N = 0 for the NOISYDIR or NOISYSPEED target types, then the target is mapped to the
//    implementation DOTLIFEWIN.  When N = 0, there is no noise, and NOISYDIR and NOISYSPEED should behave exactly like
//    FCDOTLIFE.
//
//    1b) If the percent coherence for the COHERENTFC target type is 100%, then it is mapped to the implementation
//    OPTRECTWIN rather than the less efficient OPTCOHERENT.  When coherence is 100%, none of the dots are randomly
//    repositioned each frame.
//
//    2) Target window in "device coordinates".  Most XY target types have a rectangular "target window"; for RECTANNU,
//    the window is an annulus defined by an inner and outer rectangle (the center of the inner rectangle may be offset 
//    WRT the center of the outer rectangle, however). The Maestro-formatted XYPARMS struct defines the window 
//    rectangle(s) in terms of width & height only (with offset coords for center pt of RECTANNU's inner rect) -- 
//    because it is assumed that all targets start out at the center of the scope display.  AddTarget() uses this 
//    assumption and the provided width & height in visual deg to compute the initial coordinates of the bounding 
//    rectangle(s) in device "pixels".  See TranslateToDevCoords().
//
//    3) Non-intuitive storage of parameters for selected target types:  See comments under declaration of structure
//    CCxScope::Parameters, which holds configuration and target information in "device" format.
//
//
//    ARGS:       tgt         -- [in] Maestro-formatted definition of an XY scope target
//                bOptimize   -- [in] if TRUE, then faster -- but restricted -- versions of the CENTER, SURROUND, and
//                               RECTANNU target types may be used.
//                initPos     -- [in] pos of non-moving optimized target (ignored if bOptimize is FALSE).
//
//    RETURNS:    TRUE if target was successfully added; FALSE otherwise.
//
VOID RTFCNDCL CCxScope::ClearTargets()
{
   m_Parameters.wNumTargets = 0;
}

BOOL RTFCNDCL CCxScope::AddTarget( const XYPARMS& tgt, BOOL bOptimize, const CFPoint& initPos )
{
   int iPos = int(m_Parameters.wNumTargets);                         // new target will be appended to tgt list,
   if( iPos == MAX_TARGETS )                                         // unless it is full!
   {
      SetDeviceError( "XY scope target list is full!" );
      return( FALSE );
   }

   int iType = tgt.type;                                             // CNTRLX-designated XY tgt type

   CFRect rect;                                                      // bounding rect in deg, centered at (0,0)
   rect.Set( tgt.fRectW, tgt.fRectH );
   CFRect rInner;                                                    // inner rect for RECTANNU, in deg, possibly 
   rInner.Set( tgt.fInnerW, tgt.fInnerH );                           // offset WRT ctr of bounding rect!
   rInner.Offset(tgt.fInnerX, tgt.fInnerY);

   if( bOptimize && (iType == SURROUND || iType == RECTANNU) )       // if we can use optimized realization of these
   {                                                                 // target types, we must offset tgt from (0,0) to
      rect.Offset( initPos );                                        // the specified position
      if( iType == RECTANNU ) rInner.Offset( initPos );
   }

   if( bOptimize && (iType == CENTER) )                              // we can use FULLSCREEN for CENTER only if the
   {                                                                 // optimize flag is set & the target's window will
      CFRect r2 = rect;                                              // fill the entire XY scope display
      r2.Offset( initPos );
      if( !IsFullScreen( r2 ) ) bOptimize = FALSE;
   }

   TranslateToDevCoords( rect );                                     // convert rects to XY scope coord system and
   TranslateToDevCoords( rInner );                                   // store in appropriate fields in "device-level"
   if( iType != RECTANNU )                                           // parameter struct.
   {
      m_Parameters.wRectT[iPos] = (WORD) rect.Top();
      m_Parameters.wRectB[iPos] = (WORD) rect.Bot();
      m_Parameters.wRectR[iPos] = (WORD) rect.Right();
      m_Parameters.wRectL[iPos] = (WORD) rect.Left();
      m_Parameters.wOuterT[iPos] = (WORD) MAX_PIX;                   // "outer rect" ignored for all tgt types
      m_Parameters.wOuterB[iPos] = (WORD) 0;                         // except RECTANNU
      m_Parameters.wOuterR[iPos] = (WORD) MAX_PIX;
      m_Parameters.wOuterL[iPos] = (WORD) 0;
   }
   else
   {
      m_Parameters.wRectT[iPos] = (WORD) rInner.Top();
      m_Parameters.wRectB[iPos] = (WORD) rInner.Bot();
      m_Parameters.wRectR[iPos] = (WORD) rInner.Right();
      m_Parameters.wRectL[iPos] = (WORD) rInner.Left();
      m_Parameters.wOuterT[iPos] = (WORD) rect.Top();
      m_Parameters.wOuterB[iPos] = (WORD) rect.Bot();
      m_Parameters.wOuterR[iPos] = (WORD) rect.Right();
      m_Parameters.wOuterL[iPos] = (WORD) rect.Left();
   }

   m_Parameters.wNumDots[iPos] = (WORD) tgt.ndots;                   // #dots in target dot pattern

   int iScale;
   switch( iType )                                                   // BEGIN: target type-specific translations...
   {
      case RECTDOT :                                                 // an array of regularly spaced dots.  here, while
         m_Parameters.wType[iPos] = DOTARRAY;                        // rectW is desired width of dot array, rectH is
         m_Parameters.wRectR[iPos] = (WORD) rect.Width();            // interpreted as the desired spacing between dots
         m_Parameters.wRectL[iPos] = (WORD) rect.Height();           // in the array, both horizontally & vertically.
         break;                                                      // the standard bounding rect is ignored!
      case CENTER :
         m_Parameters.wType[iPos] = bOptimize ? FULLSCREEN : RECTWINDOW;
         break;
      case SURROUND :
         m_Parameters.wType[iPos] = bOptimize ? STATICANNU : RECTHOLE;
         break;
      case RECTANNU :
         m_Parameters.wType[iPos] = bOptimize ? STATICANNU : ANNULUS;
         break;
      case FASTCENTER :
         m_Parameters.wType[iPos] = OPTRECTWIN;
         break;
      case FCDOTLIFE :
      case NOISYDIR :
      case NOISYSPEED :
         m_Parameters.wType[iPos] = (iType==FCDOTLIFE) ? DOTLIFEWIN :
            ((iType==NOISYDIR) ? DL_NOISEDIR : DL_NOISESPEED);

         if( tgt.iDotLfUnits == DOTLFINMS )                          // store max dot lifetime in msecs, or in deg/100
            m_Parameters.wOuterR[iPos] = (WORD) tgt.fDotLife;
         else
            m_Parameters.wOuterR[iPos] = (WORD) (tgt.fDotLife * 100.0f);

         if( iType == NOISYDIR || iType == NOISYSPEED )              // store add'l params for these tgt types:
         {
            m_Parameters.wOuterL[iPos] = (WORD) tgt.fInnerW;         // noise range limit.

            m_Parameters.wOuterT[iPos] = (WORD) tgt.fInnerH;         // store noise upd intv in msecs

            if(iType == NOISYSPEED && tgt.fInnerX != 0.0f)           // "flag" selects multiplicative instead of 
               m_Parameters.wOuterB[iPos] = (WORD) 1;                //  additive speed noise

            if( m_Parameters.wOuterL[iPos] == 0 ||                   // if noise range or upd intv is 0, use DOTLIFEWIN
                m_Parameters.wOuterT[iPos] == 0 )                    // implementation (effectively, no noise).
               m_Parameters.wType[iPos] = DOTLIFEWIN;
         }
         break;
      case FLOWFIELD :                                               // optic flow field has a non-standard param set:
         m_Parameters.wType[iPos] = OPTICFLOW;
         m_Parameters.wRectR[iPos] = (WORD) (tgt.fInnerW * 100.0f);  //    inner radius in deg/100
         m_Parameters.wRectL[iPos] = (WORD) (tgt.fRectW * 100.0f);   //    outer radius in deg/100

         iScale = m_iDistToEye * 1024;                               //    (distToEyeInMM/screenWidthInMM),
         iScale /= m_iWidth;                                         //    *1024 to preserve precision
         m_Parameters.wRectT[iPos] = (WORD) iScale;
         iScale = m_iDistToEye * 1024;                               //    (distToEyeInMM/screenHeightInMM),
         iScale /= m_iHeight;                                        //    *1024 to preserve precision
         m_Parameters.wRectB[iPos] = (WORD) iScale;

         m_Parameters.wOuterR[iPos] = CTR_PIX;                       //    initial coord of FOE forced to screen ctr
         m_Parameters.wOuterL[iPos] = CTR_PIX;
         break;
      case ORIENTEDBAR :                                             // oriented bar has a non-standard param set:
         m_Parameters.wType[iPos] = ORIBAR;
         m_Parameters.wRectR[iPos] = (WORD) rect.Width();            //    width of bar rect in pixels
         m_Parameters.wRectL[iPos] = (WORD) rect.Height();           //    height of bar rect in pixels
         m_Parameters.wRectT[iPos] = (WORD) tgt.fInnerW;             //    drift axis angle to nearest deg
         break;
      case COHERENTFC :                                              // COHERENTFC->OPTCOHERENT.  store pct coherence.
         m_Parameters.wType[iPos] = OPTCOHERENT;                     // BUT if 100%, use OPTRECTWIN instead.
         m_Parameters.wOuterL[iPos] = (WORD) tgt.fInnerW;
         if( m_Parameters.wOuterL[iPos] == 100 )
         {
            m_Parameters.wOuterL[iPos] = 0;
            m_Parameters.wType[iPos] = OPTRECTWIN;
         }
         break;

      default :                                                      // error: unrecognized target type!
         SetDeviceError( "Unrecognized XY scope target type!" );
         return( FALSE );
   }                                                                 // END: target type-specific translations

   ++(m_Parameters.wNumTargets);
   return( TRUE );
}


//=== Load ============================================================================================================
//
//    Load the current XY scope configuration and target definition parameters onto the XY scope device, then command
//    it to prepare all defined targets for subsequent frame updates.  The XY scope controller reads the parameters and
//    creates an internal representation of each defined target.  Times out if the device fails to respond.
//
//    ARGS:       iAltSeed -- [in] one-time XY dot seed control overriding current display settings.  If <0, ignored.
//                            If 0, seed is auto-generated. Else, arg is the seed value [default = -1].
//
//    RETURNS:    TRUE if successful; FALSE indicates that no targets are defined, the XY scope device is unavailable,
//                or it failed to respond ("hardware timeout").
//
BOOL RTFCNDCL CCxScope::Load( int iAltSeed /* = -1 */)
{
   if( m_Parameters.wNumTargets == 0 )                               // fail if there are no targets to load
   {
      SetDeviceError( "No targets defined for XY scope device!" );
      return( FALSE );
   }

   if( !IsOn() )                                                     // fail if device is not available
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }

   // fill in general info in parameters structure that is NOT target-specific
   m_Parameters.wWidthMM = m_iWidth;
   m_Parameters.wHeightMM = m_iHeight;
   m_Parameters.wDistMM = m_iDistToEye;
   m_Parameters.wDelayPerDot = m_iDrawDelay; 
   m_Parameters.wOnTimePerDot = m_iDrawDur;  
   if( iAltSeed >= 0 )                                               //    one-time override of dot seed!
   {
      if( iAltSeed > 0 )
         m_Parameters.dwDotSeed = (DWORD) iAltSeed;
      else
      {
         m_Parameters.dwDotSeed =  0x0000FFFF & ((DWORD) m_randGen.Generate());
         m_Parameters.dwDotSeed |= ((DWORD) m_randGen.Generate()) << 16;
      }
   }
   else
   {
      if( !m_bAutoSeed )
         m_Parameters.dwDotSeed = m_dwFixedSeed;
      else
      {
         m_Parameters.dwDotSeed =  0x0000FFFF & ((DWORD) m_randGen.Generate());
         m_Parameters.dwDotSeed |= ((DWORD) m_randGen.Generate()) << 16;
      }
   }

   for( int i = 0; i < int(m_Parameters.wNumTargets); i++ )          // reset "fractional pixel displacements" for all
   {                                                                 // targets that will be animated
      m_FracPixWin[i].Zero();
      m_FracPixPat[i].Zero();
   }

   if( !LoadParameters() )                                           // load parameters onto XY scope device
   {
      SetDeviceError( CDevice::EMSG_DEVTIMEOUT );
      return( FALSE );
   }

   return( TRUE );
}


//=== Update ==========================================================================================================
//
//    Initiate a display frame update on the XY scope device.
//
//    The method translates target displacement data for the next display frame from a CNTRLX-centric coordinate system
//    (visual deg subtended at eye) to XY scope device coordinates (pix).  It then writes the translated displacement
//    data to the scope controller and issues a "doframe" command to begin drawing the next display frame.  The method
//    will fail if the controller is not ready to handle a "doframe" -- the assumption here is that the device is
//    unable to animate the defined targets:  there are too many targets, or too many total dots to draw, or too short
//    an update interval.  In this case, we report the "dropped frame" error.
//
//    The conversion from floating-point degrees to integer-valued pixels introduces a truncation error which can
//    become significant over an extended motion in one direction.  To eliminate this problem, CCxScope maintains --
//    for both H&V coordinates of each target's window and pattern displacements -- the "fractional pixel" remaining
//    after truncation.  This is carried over to the next display frame.
//
//    Note that the arrays of "update info" provided must contain entries for every target currently defined on the XY
//    scope device IN THE ORDER in which the targets were defined (via successive calls to AddTarget()).  For a few
//    unusual target types, the update information has a "non-standard" meaning:
//
//    1) DOTLIFEWIN (FCDOTLIFE) and DOTLFNOISY (NOISYDOTS).  The upper byte of the target update interval must hold the
//    target's per-refresh decrement in dot life (arbitrary units).  The lower byte contains the number of times the
//    target should be drawn during each refresh. Both quantities are necessarily restricted to [0..255] in this case.
//
//    2) OPTICFLOW (FLOWFIELD). The (H,V) pos change in target window refers to the change in pos of the flow field's
//    focus of expansion (FOE).  These are converted from degrees to pixels and stored in UpdateRec.shWindowH,V -- as
//    for other standard targets.  Next, the H component (V is ignored) of the target pattern pos change is interpreted
//    as deltaR, the change in radial pos of a dot at a radius r2 = 1/2 the outer radius of the flow field (note that
//    CNTRLX specifies radial pos in the flow field in deg subtended at the eye).  From this the factor B = deltaR /
//    (sin(r2) * cos(r2)) is calculated in units of deg/100.  This factor is scaled by 2^M, where M is chosen so that
//    B*2^M fits in a 16bit int and 3 digits precision are preserved (if possible).  B*2^M is stored in the shPatternH
//    field of the device-level update record, and M in the shPatternV field.  Since precision is preserved in this
//    unusual representation, there is no need to worry about "truncation error" for this target type.
//
//    3) DL_NOISEDIR (NOISYDIR) and DL_NOISESPEED (NOISYSPEED).  The target pattern pos change is expressed in POLAR
//    SCREEN coordinates (radius in mm, theta in degrees) rather than Caretesian pixels, which is the default for most
//    other target types.  The reason: it reduces overhead on the XYCORE side, which must calculate the displacement
//    vector for each individual dot.  For NOISYDIR, it must rotate the displacement vector for each dot by some
//    randomly chosen offset direction; for NOISYSPEED, it must offset or scale the vector's magnitude to adjust the 
//    speed of each dot. This is easiest to do if the pattern displacement vector is in polar coords, with the vector
//    amplitude in screen millimeters, NOT device pixels (because, unless screen width = screen height, a "pixel" is
//    not square on the XYScope display even though its device grid is 65536x56636). To preserve precision when doing 
//    the math on the XYCORE side, radial component R is scaled by 2^16 IF its unscaled value is < 0.1, and 2^10 
//    otherwise. [The scale factors and threshold have been chosen to support a velocity range of 0.1-200 deg/sec and 
//    a distance-to-eye range of 250-1000mm.) Displacement angle ("theta") is stored in deg/10, restricted to the range
//    [0..3599]. However, if the scale factor is 2^10, we add 10000 to "theta" as a flag informing XYCORE so that it 
//    knows which scale factor to use. This unusual representation of the pattern displacement vector preserves 
//    precision for smaller per-frame displacements. Ultimately, XYCORE must remove the scale factor to get the x- and
//    y-pixel change for each dot, so it must keep track of the fractional pixel left over -- on a per-dot basis!
//
// 11may2011: As of Maestro v2.7.0, target pattern displacments are specified WRT the target window's frame of 
// reference, NOT the global screen frame of reference. XYCORE's implementation has been updated accordingly. No
// changes needed here.
//
//    ARGS:       pFPtWin        -- [in] pos changes (H,V) in target windows for this frame (deg), WRT screen.
//                pFPtPattern    -- [in] pos changes (H,V) in target patterns for this frame (deg), WRT window.
//                pwTgtUpdateIntv-- [in] update interval for each target (ms); may not be the same for all targets, and
//                                  will be 0 for targets that should not be drawn.
//
//    RETURNS:    TRUE if successful; FALSE if XY scope device is not ready to start a frame update.
//
BOOL RTFCNDCL CCxScope::Update( CFPoint* pFPtWin, CFPoint* pFPtPattern, WORD* pwTgtUpdateIntv )
{
   if( m_Parameters.wNumTargets == 0 ) return( TRUE );               // there's nothing to update!!

   if( !IsOn() )                                                     // fail if device is not available
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }

   for( int i = 0; i < int(m_Parameters.wNumTargets); i++ )          // prepare block of device-level update records
   {                                                                 // for the new display frame...
      if( m_Parameters.wType[i] == DOTLIFEWIN ||                     //    for these tgt types, upper byte of update
          m_Parameters.wType[i] == DL_NOISEDIR ||                    //    intv is actually dot life decrement -- so we
          m_Parameters.wType[i] == DL_NOISESPEED )                   //    have to be careful here.  regardless, we
      {                                                              //    need to compute #refreshes per frame = tgt's
         WORD w = 0x00FF & pwTgtUpdateIntv[i];                       //    update intv/min update interval...
         w /= ((WORD) MIN_UPDATEINTV);
         w |= 0xFF00 & pwTgtUpdateIntv[i];
         m_NextUpdate[i].shNumReps = short(w);
      }
      else
         m_NextUpdate[i].shNumReps = short(pwTgtUpdateIntv[i]) / MIN_UPDATEINTV;

      m_FracPixWin[i] += pFPtWin[i] * m_DegToPix;                    //    store change in tgt window pos in pixels,
      m_NextUpdate[i].shWindowH = short( m_FracPixWin[i].GetH() );   //    taking into account frac pix from prev
      m_NextUpdate[i].shWindowV = short( m_FracPixWin[i].GetV() );   //    frame, and carrying over the new frac pix to
      m_FracPixWin[i].DiscardIntegerPart();                          //    the next frame...

                                                                     //    similarly for tgt pattern pos change...
      if( m_Parameters.wType[i] == OPTICFLOW )                       //    ...but OPTICFLOW tgt is a special case...
      {                                                              //    (see comments in fcn header for details):
         double d = double(m_Parameters.wRectL[i]) / 200.0;          //       0.5 * outer radius of flow field, in deg
         d = double(pFPtPattern[i].GetH()) * 100.0 /                 //       calculate factor B
                  cMath::sincosDeg( d );

         double dAbs = cMath::abs( d );                              //       determine M such that B*2^M is in range
                                                                     //       of a short int, and M <= 10...
         if( (dAbs < 0.01) || (dAbs > 1.0e6) )                       //       beyond these extremes, OPTICFLOW tgt does
         {                                                           //       not work:  B forced to 0!
            m_NextUpdate[i].shPatternH = 0;
            m_NextUpdate[i].shPatternV = 0;
         }
         else if( dAbs < 10.0 )
         {
            m_NextUpdate[i].shPatternH = (short) (d * 1024.0);
            m_NextUpdate[i].shPatternV = 10;
         }
         else if( dAbs < 100.0 )
         {
            m_NextUpdate[i].shPatternH = (short) (d * 256.0);
            m_NextUpdate[i].shPatternV = 8;
         }
         else if( dAbs < 1000.0 )
         {
            m_NextUpdate[i].shPatternH = (short) (d * 32.0);
            m_NextUpdate[i].shPatternV = 5;
         }
         else if( dAbs < 10000.0 )
         {
            m_NextUpdate[i].shPatternH = (short) (d * 2.0);
            m_NextUpdate[i].shPatternV = 1;
         }
         else if( dAbs < 100000.0 )
         {
            m_NextUpdate[i].shPatternH = (short) (d / 4.0);
            m_NextUpdate[i].shPatternV = -2;
         }
         else
         {
            m_NextUpdate[i].shPatternH = (short) (d / 32.0);
            m_NextUpdate[i].shPatternV = -5;
         }
      }
      else if( m_Parameters.wType[i] == DL_NOISEDIR ||               // these tgt types are special, too...
               m_Parameters.wType[i] == DL_NOISESPEED )
      {
         // convert pattern displacement vector in visual deg to Cartesian screen coords in mm, then to polar screen
         // coords: r(mm), theta(deg)
         CFPoint fPt;
         fPt.Set(cMath::tanDeg(pFPtPattern[i].GetH()) * m_iDistToEye, cMath::tanDeg(pFPtPattern[i].GetV()) * m_iDistToEye);
         float r = fPt.GetR(); 
         float theta = fPt.GetTheta();
         
         // scale displacement amplitudes < 0.1 by 2^16 and all others by 2^10 for more precision in the calculations
         // on the XYCORE side. This essentially hands over responsibility for keeping track of fractional pixel
         // displacements to XYCORE, which we have to do b/c each dot in the target moves differently! For the latter
         // case, 10000 is added to THETA so that XYCORE will know which scale factor to use.
         // IMPORTANT: The scale factors and threshold were chosen to support a pattern velocity range of 
         // 0.1-200deg/sec and a distance-to-eye range of 250-1000mm.
         if(r < 0.1f) 
         {
            m_NextUpdate[i].shPatternH = short( r * 65536.0f );
            m_NextUpdate[i].shPatternV = short( theta * 10.0f );
         }
         else
         {
            m_NextUpdate[i].shPatternH = short( r * 1024.0f );
            m_NextUpdate[i].shPatternV = short( theta * 10.0f ) + 10000;
         }
      }
      else                                                           //    for all other target types...
      {
         m_FracPixPat[i] += pFPtPattern[i] * m_DegToPix;
         m_NextUpdate[i].shPatternH = short( m_FracPixPat[i].GetH() );
         m_NextUpdate[i].shPatternV = short( m_FracPixPat[i].GetV() );
         m_FracPixPat[i].DiscardIntegerPart();
      }

   }

   if( !DoFrame() )                                                  // write the device-level update records to the
   {                                                                 // XY scope controller and initiate frame update;
      SetDeviceError( "Dropped frame on XY scope!" );                // assume dropped frame on error
      return( FALSE );
   }

   return( TRUE );
}



//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================

//=== CalcConversionFactors ===========================================================================================
//
//    Based on the current geometry for the XY scope display, this method calculates the multiplicative factors (for
//    (H & V components, separately) that convert position in deg to pixels.  It should be called whenever the
//    display geometry is modified.
//
//    The XY scope display is treated as an NxN array of pixels, where N = MAX_PIX + 1;  Thus, a single "pixel" on the
//    XY scope is a rectangle that is W/N mm wide and H/N mm high, where W & H are the screen width & height in mm,
//    respectively -- even though such fine resolution is not actually attainable.  To compute the conversion factor
//    for the horizontal (vertical) component, we divide the display's half-width (half-height) in pixels by the angle
//    (in deg) subtended at the eye by that half-width (half-height).  Units of each conversion factor are pix/deg.
//
//    DEVNOTE:  This conversion factor is only reasonable for small pos changes.  For large pos changes, one should
//    do the trigonometric calculations... perhaps we should get rid of the (assumed linear) multiplicative factors
//    entirely and do the trigonometry every time.  But this could have too negative an impact on performance???
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxScope::CalcConversionFactors()
{
   double d1 = (double(MAX_PIX) + 1.0) / 2.0;

   double d2 = ::atan2( double(m_iWidth)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD;
   m_DegToPix.SetH( d1/d2 );

   d2 = ::atan2( double(m_iHeight)/2.0, double(m_iDistToEye) ) / cMath::DEGTORAD;
   m_DegToPix.SetV( d1/d2 );
}


//=== TranslateToDevCoords ============================================================================================
//
//    Most XY scope target types are, in part, defined by a bounding rectangle (for RECTANNU, there is both an outer
//    and inner rect).  CNTRLX specifies this rectangle in a "visual" coordinate system, where the animal's line-of-
//    sight (LOS) passes thru the center of the XY scope display, which is defined as the origin.  The units of this
//    coordinate system are floating-point visual deg subtended at the eye.  The XY scope DSP controller, on the other
//    hand, uses a pixellated coordinate system, with the lower-left corner of the screen designated as the origin
//    (0,0) and the upper-right corner at (MAX_PIX, MAX_PIX) pixels.
//
//    This method is responsible for translating rectangles specified in the visual coordinate system to the pixel
//    coordinate system of the XY scope.  The conversion requires, of course, the XY scope's current display geometry.
//    Let D = LOS distance from eye to screen, in mm; W = visible display width, in mm; and H = visible display height,
//    in mm.  Then, the translation of the rectangle's coords proceeds as follows:
//       1) Restrict range of coords to [-80..80] deg.  It is unlikely we'll ever encounter visual angles that large,
//          in any case.  A value of 90 deg maps to +infinity!
//       2) Each coord in mm = D * tan( coord in deg ).
//       3) Translate origin to lower-left corner:  offset rect by (W/2, H/2).
//       4) Convert to pixels:  scale by N/W along horiz axis and by N/H along vert axis, truncating to get int-valued
//          coords.  Here N = the extent of the controller's square coord system: MAX_PIX + 1.
//       5) Restrict range of pixel coords to [0..MAX_PIX].
//
//    We use a simple CFRect object to represent the rectangle's coordinates -- see UTIL.H.  Note that CFRect is
//    guaranteed to be normalized.
//
//    ARGS:       rect  -- [in] rectangle in visual coord system (deg subtended at eye, origin = screen ctr).
//                      -- [out] rectangle in XY scope coord system (int-valued pixels, origin = corres to LL corner).
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxScope::TranslateToDevCoords( CFRect& rect )
{
   rect.RangeRestrict( -80, 80 );                           // limit range of coord values in [-80..80] deg

   float f = (float) m_iDistToEye;                          // deg --> mm
   rect.Set( f * cMath::tanDeg( rect.Left() ),
             f * cMath::tanDeg( rect.Top() ),
             f * cMath::tanDeg( rect.Right() ),
             f * cMath::tanDeg( rect.Bot() ) );

   double w = (double) m_iWidth;                            // translate origin from screen ctr to lower-left corner
   double h = (double) m_iHeight;
   rect.Offset( w/2.0, h/2.0 );

   rect.Scale( double(MAX_PIX+1)/w, double(MAX_PIX+1)/h );  // mm --> pixels
   rect.Truncate();
   rect.RangeRestrict( 0, MAX_PIX );
}


//=== IsFullScreen ====================================================================================================
//
//    Does specified rectangle fill the entire XY scope display?
//
//    ARGS:       rect  -- [in] rectangle in visual coord system (deg subtended at eye, (0,0) corres to screen ctr).
//
//    RETURNS:    TRUE if rectangle fills entire XY scope display; FALSE otherwise.
//
BOOL RTFCNDCL CCxScope::IsFullScreen( const CFRect& rect )
{
   CFRect r = rect;
   TranslateToDevCoords( r );
   return( (0 == (WORD) r.Left()) && (MAX_PIX == (WORD) r.Right()) &&
           (0 == (WORD) r.Bot()) && (MAX_PIX == (WORD) r.Top()) );
}

