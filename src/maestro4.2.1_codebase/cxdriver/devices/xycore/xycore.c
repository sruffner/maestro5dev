//===================================================================================================================== 
//
// xycore.c :  Source code for the XYCORE firmware, implementing CXDRIVER "XY scope controller" functionality on three 
//             different DSP boards from Spectrum Signal Processing:  the Detroit C6x, Daytona C6x, and Dakar F5 (C44). 
//             The XYCORE built with this code is designed to interface with the CXYDetroit, CXYDaytona, and CXYDakar 
//             device objects in CXDRIVER, respectively.
//
// AUTHOR:  saruffner, njpriebe, sglisberger.
//
// DESCRIPTION:
// The "XY scope" -- a large-screen, high-performance XY oscilloscope -- is an important target platform in CXDRIVER.
// A wide variety of visual targets are presented on this medium, including spots and various kinds of random-dot 
// patterns.  The X, Y, and trigger inputs of the scope are driven by an in-house "dotter board" which, in turn, is 
// controlled by a DSP-based hardware device residing in the host system.  The source code here defines the firmware 
// program XYCORE, which implements the "XY scope controller" functionality on the DSP board and interacts with a 
// host-side "device object" within CXDRIVER.
//
// ==> Supported boards; interaction with CXDRIVER.
// Three different DSP cards from Spectrum Signal Processing are supported as XY scope controllers.  Each DSP target 
// requires a slightly different XYCORE because of hardware differences.  However, the differences are relatively 
// minor, so this source code file has been modified to handle all three XYCOREs by using build flags and #IFDEF-#ENDIF 
// constructs to bracket target-specific code.  Each target interacts with a different CXDRIVER "device object" running 
// on the PC host's CPU.  All of these device objects are derived from a common interface, the abstract class CCxScope, 
// which encapsulates the XY scope controller from CXDRIVER's point of view.  The device object is responsible for 
// resetting the target DSP board, downloading XYCORE (an executable COFF file), writing target and animation info to 
// the board's memory, and issuing commands to XYCORE.  The table below lists the supported boards, along with the 
// corresponding build flags and CXDRIVER device objects.
//
//    SSP Board                                                XYCORE build flag       CXDRIVER Device Object
//    -------------------------------------------------------------------------------------------------------
//    Dakar F5 Carrier Board (embedded C44 node only)          _TGTDAKARF5             CXYDakar
//    Detroit C6x (single node)                                _TGTDETROIT             CXYDetroit
//    Daytona Dual C6x (NodeA only; NodeB unused)              _TGTDAYTONA             CXYDaytona
//
// NOTE that we do NOT support multiple processors on a board.  For the Dakar and Daytona, XYCORE is loaded and run on 
// a single node; any other DSP resources are wasted.  This could change in the future, but it would require much more 
// sophisticated programming.
//
// XYCORE and its corresponding host-side device object -- hereafter called "XYAPI" -- must work together to animate 
// XY scope targets.  The communication interface between XYCORE and XYAPI consists of a single Command/Status register 
// and a "SharedData" memory space.  XYAPI writes animation info to XYCORE via the SharedData space, and it issues 
// commands to XYCORE via the CmdStat register.  Before issuing a new command, the XYAPI waits for the XYCORE_READY
// status.  XYCORE continuously polls the CmdStat register until it contains a command.  It then immediately begins 
// executing the command, resetting the status to XYCORE_READY when it's done. 
//
// XYAPI initiates an animation sequence by writing target definitions and other animation info -- encapsulated in the 
// data structure PARAMETERS -- to the SharedData area.  It then issues the XYCORE_INIT command.  In response, XYCORE 
// reads the PARAMETERS structure into private memory, then begins building internal representations of each target 
// defined in that data structure.  All targets are initially positioned at the center of the XY scope display, which 
// is considered the origin (0,0) in CXDRIVER's coordinate system.  To "animate" the defined targets, XYAPI issues 
// "display frame updates" every N ms (N is generally 2-4ms, but other values are permitted).  On each update, XYAPI 
// writes a set of "motion update records" to the SharedData space, immediately after the PARAMETERS structure, which 
// has a static size.  There is (and XYCORE expects) one motion update record for each target defined in the last 
// XYCORE_INIT call, and the records appear in the same order as the targets are defined in the PARAMETERS structure. 
// After writing the update records, XYAPI checks the CmdStat register to see if XYCORE is ready for a new command.  If 
// not, the display frame is "dropped" -- indicating that the animation load is too severe for XYCORE to keep up 
// (display frame too short, too many total dots to draw, etc.).  Otherwise, XYAPI issues the XYCORE_DOFRAME command. 
// XYCORE responds by reading the motion records, updating the positions of every target's dots accordingly, redrawing 
// all targets, and finally resetting the CmdStat reg to XYCORE_READY when it has finished.  This sequence continues ad 
// infinitum, under complete control of XYAPI.
//
// Exactly how the CmdStat register and SharedData space are implemented depends on the specific board:
//
//    SSP Board            CmdStat Register                          SharedData Space
//    ------------------------------------------------------------------------------------------------
//    Dakar                Mailbox reg in PCI interface chip         Portion of "Far Global ASRAM"
//    Daytona              Mailbox reg in PCI interface chip         Portion of global ASRAM
//    Detroit              First DWORD in SSRAM                      Portion of SSRAM starting at 4th DWORD
//
// ==> Differences in integer data size.  !!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!! 
// The C6x processor on the Detroit & Daytona boards support integer data sizes like those on an Intel CPU (char = 1 
// byte, short = 2 bytes, int = 4 bytes).  However, the C44 processor on the Dakar only uses 32-bit words, so all 
// integer data types used in this code will map to INT32/UINT32!  This fact creates three complications in tailoring 
// XYCORE to the Dakar:
//    1) The "SharedData" interface between CXYDakar and the XYCORE_Dakar is different than that for the other two 
// boards.  To keep things as simple as possible, CXYDakar handles the burden of writing the PARAMETERS struct and 
// motion update records using 32-bit boundaries rather than 16-bit; thus, it is easy for the XYCORE_Dakar to read that 
// shared data -- the only difference is that each datum is stored in 32-bit memory locations.  Of course, this 
// requires us to download more bytes to the Dakar, which could have an impact on overall system performance!  
//    2) We must be careful with the sizeof() operator.  The TMS320C3x/C4x compiler's implementation of this operator 
// returns size in units of 32-bit words, not bytes.  Thus, sizeof(char) = sizeof(int) = 1 for the Dakar.
//    3) Finally, the Dakar's 32-bit datum size has an impact on the technique XYCORE_Dakar uses to animate targets. 
// We actually RELY on UINT16 arithmetic to automatically wrap target dot positions around the screen boundaries, which 
// are limited to [0..65535] ("pixels") on both horizontal & vertical axes by the dotter board circuitry.  Since the 
// Dakar maps UINT16 to UINT32, we have to simulate the UINT16 arithmetic in order to achieve the same effect -- thus 
// degrading the performance of XYCORE_Dakar relative to the other boards.  XYAPI should ensure that the per-frame 
// changes in dot positions are always less than MAX_INT16 in size, since the simulation of UINT16 math in XYCORE_Dakar 
// makes that assumption.  
//
// ==> Dotter board interface.
// Each of the above DSP boards is connected to the special-purpose "dotter" board over a DSP~LINK3 connector.  The 
// dotter board controls three analog signals that drive the XY oscilloscope:  XOUT, a voltage signal controlling 
// deflection of the scope's electron beam along the horizontal axis; YOUT, a voltage signal controlling deflection of 
// the beam along the vertical axis; and TRIG, a trigger signal that controls when the beam is "ON".  Three 32-bit 
// registers on the dotter board are addressed over the DSP~LINK3 interface:
//
//    Location register [4 bytes, write-only]:  When written, tells dotter board to move beam to specified location. 
//       The 2-byte x-coord is in the high word, while the 2-byte y-coord is in the low word.  Thus, the scope display 
//       is divided into 2^16 = 65536 [0..65535] columns and 65536 rows.  The dotter board converts these pixel-like 
//       (x,y) coordinates into the appropriate voltage signals Xout & Yout.  Accessed by WRITING to the first 32-bit 
//       memory location in the DSP~LINK3 (standard access) memory map.
//    Status register [4 bytes, read-only]:  Bit0 is cleared when the board is ready for a new beam position.  During 
//       each "dot draw" cycle, we poll this register until the bit is cleared; then it is safe to write new coords 
//       into the location register, thus starting a new draw cycle.  Accessed by READING the first 32-bit memory 
//       location in the DSP~LINK3 (standard access) memory map.
//    Timing register [4 bytes, write-only]:  Holds the timing parameters that determine the trigger signal's waveform. 
//       Timing data is stored quite awkwardly in the upper three nibbles of this register...
//          nib7 (bits31-28) --> lo-nibble of the one-byte trigger duration (= delay + beam "ON" time)
//          nib6 (bits27-24) --> hi-nibble of the trigger duration
//          nib5 (bits23-20) --> single nibble representing trigger delay
//       Register is accessed by writing to the second 32-bit memory location in the DSP~LINK3 memory map.
//
// ==> PERFORMANCE considerations.
// The key performance measure for XYCORE is the "average dot-draw cycle", which is the amount of time it takes to draw 
// one target dot on the scope.  This measure varies with target type:  the windowed types are more time-consuming 
// because we have to compare each dot's updated coords with the target window bounds.  The OPTICFLOW target is by far 
// the slowest given the calculations required.
// 
// Conditional expressions in the code are costly because branching instructions are accompanied by a significant # of 
// extra machine cycles to clear out the CPU's instruction pipeline (see TI DSP programming manuals for details).  So,
// to maximize the speed with which each target type is drawn, we try to minimize the number of conditional expressions 
// that must be evaluated per target dot.  as a result, there's quite a bit of code repetition in the response to the 
// XYCORE_DOFRAME command.  we also avoid most function calls -- except for calls to our random # generator.
//
// We were able to further reduce the average dot draw cycle for the windowed target types by using 'register' vars 
// judiciously.  A target's window boundaries are stored in 4 arrays (8 for the ANNULUS target type) in the PARAMETERS 
// struct.  Originally, our comparisons took the form (xPos > parameters->wRectR[d]), but accessing elements in the 
// arrays within a struct was eating up LOTS of machine cycles.  This was time wasted, since the window bounds are 
// updated before we enter the target's drawing loop, so they do not change for the duration of the loop.  By storing 
// the bounds in 'register' vars before entering the loop and adjusting the conditional expressions in the loop 
// accordingly, we enhanced performance by as much as 40% on the windowed target types.
// 
// Another optimization:  Typically each target is drawn N times per display frame (where N = UPDATEREC.shNumReps). 
// If, on each redraw, we repeated all the comparisons (to window boundaries) to determine whether or not a dot is 
// visible, we'd be wasting time unnecessarily.  Instead, we reserve a large section of onboard memory (of which we 
// have plenty) to store the "packed coordinates" (x,y) of all visible dots across all targets (by "packed", we mean 
// that the 16bit X and Y coordinates are stored in a single 4-byte word ready for writing directly to the dotter 
// board).  This "array of visible dots" is prepared during the first redraw cycle.  Subsequent redraws are therefore 
// much faster -- we merely draw every dot in the visible dot array!
//
// ==> Description of supported target types; implementation notes.
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
// Most XY scope targets have two components:  a rectangular window bounding the target's extent, and a random-dot 
// pattern that's visible only inside (or outside) that window.  The window and the pattern can move independently, and 
// the target is defined by the dimensions of the window and the # of dots in the pattern.  Other target types lack a 
// pattern component, and others are completely unique.  Here is a brief summary:
// 
// DOTARRAY       A rectangular array of dots.  Defined by #dots, width of dot array, and spacing between dots.  No 
//                target "pattern".  Most common usage sets spacing to zero, resulting in a point-like target the 
//                intensity of which depends on #dots.
//
// RECTWINDOW     Full-screen random-dot pattern visible only inside a rectangular window. 
//
// FULLSCREEN     Full-screen randow-dot pattern (no window) -- an optimized implementation of RECTWINDOW when the 
//                window does not move and fills the whole screen.
//
// RECTHOLE       Full-screen random-dot pattern visible only OUTSIDE a rectangular window.
//
// ANNULUS        Full-screen random-dot pattern visible only inside a rectangular annulus.  The annulus is defined by 
//                an inner and outer rectangle.
//
// STATICANNU     Can be used instead of RECTHOLE or ANNULUS when it is known apriori that neither the target window 
//                nor its underlying pattern will ever move during an animation sequence.  Rarely used.
// 
// OPTRECTWIN     An optimized version of RECTWINDOW that always restricts all target dots to the defined window.  Any 
//                dot which moves outside window on a display frame update is randomly repositioned on a window edge.
// If a dot exceeds the horiz limits of window, it is placed NEAR left(right) edge if horiz dot velocity >(<) horiz 
// window velocity; analogous logic applies to the vert direction.  How near the edge depends on the distance that the 
// dot has moved across the horizontal or vertical boundary.  E.g., suppose the dots are moving right wrt window and 
// that a dot moves 10 pixels beyond the right boundary.  That dot is now drawn 10 pixels to the right of the left 
// edge, with the yCoord randomized (so that the pattern does not merely "wrap-around" the window boundaries).  [NOTE 
// that, if we merely repositioned dots somewhere ON the opposite edge, we get a striated target appearance at higher 
// pattern velocities -- because a significant percentage of the dots get repositioned on the same display frame!]
//
// It is useful when displaying a lot of smaller RECTWINDOW targets.  E.g., suppose you want to display 25 RECTWINDOW 
// targets with #dots = 300, with each target sized to about 1/25th of the full screen.  on each update frame, ~30000
// comparisons (300 dots * 25 targets * 4 sides of a rect) must be made to determine which dots should actually be 
// drawn.  This is too large a computational load for an update period of 4 ms -- not to mention a lot of memory to 
// track the postions of dots that aren't currently within their target window!  Instead, use OPTRECTWIN targets, each 
// with 300/25 = 12 dots -- resulting in the same effective dot density.  Now, only 12 * 25 * 4 or 1200 comparisons 
// must be made -- a 20+-fold improvement.  Of course, the implementation is more complex, so the true performance 
// enhancement is not quite that much.  Also, this target will not work correctly when the pattern velocity >> window 
// velocity:  If all the dots move beyond the window boundaries on every frame, all you'll get is dots flashing at 
// random locations!  At somewhat lower velocities, the dots appear to cluster at the edge or corner of the window 
// toward which they're moving.  To avoid these artifacts, the difference between pattern and window velocities should 
// be such that only a small fraction of the total number of dots move outside the window on any given update frame! 
//
// This target type CANNOT wrap-around the screen boundaries. When a wrap starts, the target window rect coords no 
// longer specify a valid rectangle.  We detect this situation and essentially "turn off" the target.  Furthermore, if 
// the target window is NEAR a screen boundary and the dot pattern pos change (hv, vv) is such that currX+hv > MAX_PIX 
// or currY+vv > MAX_PIX, then the dot will wrap around the UINT16 screen bounds and the implementation will fail.
//
// DOTLIFEWIN     A specialized version of OPTRECTWIN for which a specified "dot life" limits how far a dot travels 
//                along the current velocity vector before it is randomly repositioned somewhere else in the visible 
// window.  Its implementation is much like that of OPTRECTWIN.  However, when a dot's lifetime expires, it is randomly 
// repositioned in the target window ("recycled"), and its current lifetime is reset to maximum.  NOTE that the 
// interpretation of dot life is completely invisible to XYCORE; CXDRIVER's CCxScope device interface specifies both 
// the max dot life and how much a dot's current lifetime decreases on each refresh.  The only limitations imposed by 
// XYCORE are that max dot life fall into the range [1..32767], and that dot life decrement and the target's "repeat" 
// count fall into the range [0..255].  The latter constraint results from the fact that, for DOTLIFEWIN, the "reps" 
// field in the target's motion update record holds both the per-refresh dot life decrement (upper byte) and the 
// repeat count (lower byte).
//
// ORIBAR         Implements a rectangular bar or thin line of dots that can be oriented at any angle in [0..360) deg. 
//                Defining parameters include #dots in the target, the width of the bar in the vertical orientation 
// (in pixels), the bar's height in the vertical orientation, and its drift axis angle in whole deg.  This last 
// parameter tells XYCORE how much to rotate the bar from the vertical orientation.  If width and height are nonzero, 
// dots are randomly drawn within the bar's area.  If width is zero, the dots are distributed evenly along a line; if 
// height is zero, the bar is not drawn.  Like DOTARRAY, this target type has no "pattern" component; all dots are 
// drawn on every frame & move with the bar.
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation failed to account for "aspect ratio" -- ie, screen width and
//    height in mm not equal. The rotation of the bar's dots that is performed during XYCORE_INIT processing was done
//    in device coordinates (pixels), where the grid is 65536x65536. If the screen width and height in mm are not the
//    same, then the bar's orientation was actually smaller than it should have been. The corrected implementation 
//    performs the rotations in screen coordinates in mm, then converts each dot's coordinates in mm to pixels. This 
//    fix requires the screen width and height in mm; they are delivered in PARAMETERS.wWidthMM and .wHeightMM, resp.
//    ***************************************************************************************************************
//
// OPTICFLOW      Very different from the other XY scope target types:  Simulates an optic flow field, with randomly 
//                positioned dots moving radially away from or toward a "focus of expansion" (FOE).  The boundaries of 
// the target window are circular rather than rectangular, and each dot moves at a different speed depending upon its 
// radial distance from the FOE.  A consequence of the latter feature is that XYCORE must be responsible for animating 
// dot motion; XYAPI only provides a scaling factor which is used in per-dot velocity calculations. 
//
// The window is bounded by an inner and outer radius centered on the FOE.  The animation calculations use each dot's 
// visual angle from the flow's FOE rather than its absolute (x,y) position in pixels.  Thus, each dot's current pos is 
// most efficiently maintained in polar coordinates (r,theta), where:  r is measured in deg/100 of visual angle under 
// the assumption that the subject's line-of-sight (LOS) always passes perpendicularly thru the FOE; and theta is the 
// direction of the ray from the FOE to the dot, measured CCW from the +x-axis.  The change in dot position per display 
// frame is then given by:
//
//          delt_r(deg/100) = B(deg/100) * sin( r ) * cos( r ), 
//
// where B is a scale factor provided in the frame update record for the target.  It is set by XYAPI and is related to 
// the velocity of a dot at 1/2 the outer radius.  After the new radial position of the dot is updated, we must check 
// to make sure it has not exceeded the field's inner or outer radii (negative flows move toward the FOE, while 
// positive flows emanate from it) -- in which case both of the dot's coords (r,theta) are randomized so that the dot 
// reappears somewhere else in the flow.  A non-zero inner radius is important, because dot velocity goes to zero at 
// the FOE.  Also note that it is NOT acceptable to just "wrap" dots around from the outer to the inner radius or vice 
// versa -- instead of a random dot flow field you get bursts of dots emanating from or gravitating toward the FOE. 
// 
// Once the new dot position (r,theta) for the current update frame has been calculated, we must then calculate the 
// (x,y) coordinates of the dot in pixels.  This requires knowledge of the display geometry, in order to convert deg of 
// visual angle to pixels.  The necessary conversion factors are: 
//
//          alphaX = Dmm/Wmm     and      alphaY = Dmm/Hmm, 
//
// where Dmm is the distance from eye to screen along the LOS, and Wmm x Hmm are the dimensions of the visible screen 
// on the XY scope.  Then, finally, we get -- with (xFOE,yFOE) = current coords of FOE in pixels:
//
//          x(pix) = 65536 * alphaX * tan( r ) * cos( theta ) + xFOE; and 
//          y(pix) = 65536 * alphaY * tan( r ) * sin( theta ) + yFOE.
//
// The calculations here involve trig functions and floating-point, which will run slow on the fixed-point DSP boards 
// we currently use.  To optimize the animation of this target type, we use lookup tables to implement the trig fcns 
// and ***scaled integer*** calculations instead of floating-point.  All scale factors are powers of 2, so that 
// removing the scale factor can be accomplished by a single-cycle right-shift!  THUS:
//
//    OPTICFLOW target parameters (in the PARAMETERS struct written by CXDRIVER/XYAPI):
//       rInner (in visual deg/100) ==> wRectR[].  XYAPI restricts to min value of 50; XYCORE does NOT check it!
//       rOuter (in visual deg/100) ==> wRectL[].  XYAPI restrict to max value of 4499; XYCORE does NOT check it!
//       alphaX (* 1024)            ==> wRectT[].
//       alphaY (* 1024)            ==> wRectB[].
//       xFOE (in pixels)           ==> wOuterR[].  This is updated in place during animation of OPTICFLOW!
//       yFOE (in pixels)           ==> wOuterL[].  This is updated in place during animation of OPTICFLOW!
//       #dots in field             ==> wNumDots[].  We always display this # of dots.
//
//    OPTICFLOW per-frame update record (in the UPDATEREC struct written by CXDRIVER/XYAPI):
//       shWindowH,V ==> change in pos of the flow field's FOE, in pixels.  Thus, it is possible to move the flow's FOE 
//          on a frame-by-frame basis, but this really is not the intended use of OPTICFLOW.
//       shPatternH  ==> B(deg/100) * 2^M.  CNTRLX picks exponent M so that this value fits into INT16 and tries to 
//          preserve 3 digits of precision.  B can vary dramatically depending on the situation.
//       shPatternV  ==> the value of M.  Can be negative for very large B's.
//       shNumReps   ==> #times target should be "refreshed" during frame update (same usage as for other tgt types).
//
// The trig lookup tables are initialized when XYCORE starts up; all values are scaled by 1024, thereby preserving 3 
// digits of precision.  The tanLUT[] and sincosLUT[] are indexed by visual angle in deg/100, ranging from 0..4499; the 
// sinLUT[] and cosLUT[] are indexed by theta in deg/10, ranging from 0..3599. 
//
// Algorithms for "recycling" dots:  The specified #dots are drawn every update frame for a OPTICFLOW tgt.  Eventually, 
// a dot will pass out of the field and must be recycled.  The goal is to recycle the dots at an appropriate rate and 
// in an appropriate manner so that the dot density over the field stays uniform.  For accelerating flows, this was not 
// difficult -- when a dot's radius exceeds the outer edge, we merely reposition it anywhere in field.  Decelerating 
// flows (negative value for B) were not so easy.  Because the dots slow down as they approach the FOE, they tend to 
// accumulate there, and the previous recycling algorithm fails miserably.  After some trial and error, we came up with 
// a probabilistic algorithm that yielded the best performance over the widest range of flowfield radii and values of 
// B*2^M (which takes into account both deltaT and the flow velocity magnitude).  Basically, the probability that a dot 
// will be recycled during a given update frame is the product of two probabilities -- one that increases with the 
// magnitude of B*2^M, and one that increases with decreasing radius.  If it is recycled, the dot's angle is 
// randomized in [0..360), while its radius is randomized within an annulus at the outer edge of the field, between 
// r = rOuter and r = rOuter - pos_change_at(rOuter).  This last trick avoids the "rings" that would appear for larger 
// values of B (b/c a relatively large fraction of the dots are recycled every update frame).
//
// DL_NOISEDIR    This target behaves like DOTLIFEWIN, with an additional twist.  Instead of moving identically as part 
//                of an extended dot pattern, each dot in this target's pattern will move at the same speed but in some 
// randomly chosen direction offset from the pattern's direction.  The desired effect is to introduce "directional 
// noise" in the motion of this target's random dot pattern.  A "noise offset range" parameter, N (deg), defines the 
// set of angles [-N:N] from which each dot's offset is chosen; this parameter determines how "noisy" the target is.  A 
// second parameter M, the "noise update interval" in milliseconds, determines how often the individual dot directions 
// are recomputed.  To reduce lookup table overhead for implementing this target, CXDRIVER supplies the per-frame 
// pattern displacement vector in polar rather than Cartesian screen coords. Since display width and height in mm are
// not necessarily the same, but the device pixel grid is 65536x65536, it is important that the vector amplitude be in
// mm, not pixels. XYCORE must handle the conversion back to pixels using display geometry included in the PARAMETERS
// structure. For large displacements (R >= 0.1), CXDRIVER provides
// 
//    UPDATEREC.shPatternH = 2^10 * (R in mm);
//    UPDATEREC.shPatternV = THETA in deg/10 + 10000 [range: 10000..13599]
//
// For small displacements, to preserve precision, CXDRIVER provides
//
//    UPDATEREC.shPatternH = 2^16 * (R in mm);
//    UPDATEREC.shPatternV = THETA in deg/10 [range: 0..3599]
//
// NOTE that the offset of 10000 in THETA is a signal from CXDRIVER that the radial component R is scaled by 2^10 rather
// than 2^16.  With the displacement vector in this form, we avoid having to calculate atan2(v,h), either using a lookup
// table which consumes memory, or using the atan2() runtime function which could run very slow on a fixed-point CPU!
// Calculating the H,V pixel displacements of a target dot requires the following steps:  1) If M milliseconds have 
// elapsed since the last direction update, then: (a) for each target dot, store a new random offset angle from [-N:N]; 
// (b) reset the direction update interval timer.  2) For each target dot, calculate dot's direction TH = THETA + 
// TH_OFFSET*10, where TH_OFFSET is dot's current offset angle in degrees. TH is restricted to [0..3599]. 3) Using 
// existing lookup tables, compute HV(pix) = R*cosLUT[TH]*(2^P) and VV(pix) = R*sinLUT[TH]*(2^P).  The 2^P scale factor 
// preserves precision  while avoiding floating-point multiplies. Since the trig tables are stored with a scale factor 
// of 2^10, P = 20 or 26.
//    It's not quite as simple as this because, on a given update frame, HV(pix) and VV(pix) are typically on the order
// of a few pixels, so integer arithmetic will mean a significant truncation error. For other targets, CXDRIVER handles
// this issue by keeping track of the fractional parts of H and V displacements, and adding them back in for the next
// update. We can't do that for this target, because the displacement vector is in polar screen coordinates in mm, and
// also because each dot moves differently! Instead, XYCORE must keep track of the (scaled) fractional part of the H
// and V pixel displacements FOR EACH DOT, and carry them over to the next frame update.
//    There's a loophole in DL_NOISEDIR implementation that disables the limited dot life feature.  If the maximum dot 
// life (part of the target defn) is set to 32767 (the maximum possible value), then all target dots have infinite 
// lifetimes.
//    To make it possible to reconstruct the dot offset directions chosen on each frame during animation of this  
// target, we use a second  random-number generator dedicated solely to choosing the offset directions.  The number 
// generator uses the same algorithm as the main generator used to create random-dot patterns, etc; and it is seeded 
// with the same value (PARAMETERS.dwDotSeed) during processing of the XYCORE_INIT command.  Each time a "noise update 
// interval" expires, exactly K random offset angles are chosen (from the set of angles [-N:N] in whole degs) -- one 
// for each target dot.  NOTE that, on any given frame, some dots will not actually move in the chosen direction 
// because their "lifetime" expired -- in which case they are randomly repositioned anywhere in the target window.
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation was incorrect in two ways: (1) It failed to account for non-unity
//    aspect ratio -- ie, screen width and height in mm not equal. (2) It failed to account for integer truncation
//    error in computing HV(pix) and VV(pix). The above describes how these issues have been addressed.
//    ***************************************************************************************************************
//
// OPTCOHERENT    This target is based on the implementation of OPTRECTWIN, but it introduces the notion of "percent 
//                coherence" as described in the 1988 J.Neurosci paper by Newsome & Pare.   On EVERY frame update, 
// EVERY target dot has an N% chance of moving coherently -- ie, IAW the motion vector for that frame -- and a 
// (100-N)% chance of being randomly repositioned within the target window.  NOTE that this is NOT the same as saying 
// that N% of the target dots move coherently!!! Thus, as long as the coherence is sufficiently smaller than 100%, 
// the chance of a given dot moving coherently for more than a few consecutive frames is very small -- so we get 
// a motion stimulus without "streaking" on the scope.  If the percent coherence is 100%, there's no point in using 
// OPTCOHERENT, since it will behave exactly like OPTRECTWIN.
//
// DL_NOISESPEED  This target is similar to DL_NOISEDIR, except that the "noise" is in dot speed rather than dot 
//                direction.  The "noise offset range" N is expressed as a percentage of the nominal vector magnitude 
//                (aka, speed) of the pattern as a whole, in units of 1%. Up to a 300% offset is permitted, so 
// N = [0..300]. (Note that, when N>100%, some individual dots will move in the opposite direction from the target's 
// nominal one.) CXDRIVER supplies the per-frame pattern displacement vector in polar rather than Cartesian coords, as 
// described above for DL_NOISEDIR. Calculating the per-frame H,V pixel displacements of a target dot requires the 
// following steps: 1) If M milliseconds have elapsed since the last noise update, then: (a) for each target dot, 
// store a new random speed noise factor D randomly chose from [-N:N]; (b) reset the noise update interval timer.  
// 2) For each target dot, calculate dot's radial displacement R = Ro + D*Ro/100, where Ro is the nominal pattern 
// displacement in mm on the screen, as sent by CXDRIVER. 3) Using existing lookup tables, compute HV(pix) = 
// R*cosLUT[TH]*(2^P) and VV(pix) = R*sinLUT[TH]*(2^P), as for the DL_NOISEDIR case. The same random-number generator
// used to choose per-dot noise offset directions for DL_NOISEDIR is used to choose the speed noise factor D for each 
// target dot.
//
// (as of Maestro v2.1.3) Introduced a second, multiplicative method of noise generation. To select this method, 
// PARAMETERS.wOuterB must be nonzero. In this case, the "noise range limit" N sets the range [-N..N] over which an 
// exponent X is randomly and uniformly chosen; the range is divided into 100 equal bins. Each dot will have a 
// different X. The dot's speed Ro = (R * 2^X) / E(2^X), where R is the nominal pattern speed and E(2^X) = 
// (2^N - 2^(-N))/(2 * N * ln(2)) is the expected value of 2^X when X is a uniform random variable over [-N..N].
//    **** IMPORTANT: Bug fix effective Maestro v2.7.0. *************************************************************
//    Prior to this fix, this target's implementation was incorrect in two ways -- just as described for the target
//    type DL_NOISEDIR.
//    ***************************************************************************************************************
//
// CREDITS:
// 1) Detroit C6x Development Package, with associated sample code.  Spectrum Signal Processing.
// 2) Daytona C6x Development Package, with associated sample code.  Spectrum Signal Processing.
// 3) Dakar F5 Carrier Board Development Package, with associated code.  Spectrum Signal Processing.
//
//
// REVISION HISTORY:    See 00README.TXT.
// 



#if defined(_TGTDAKARF5)                                             // ensure that one & only one of the supported 
   #if (defined(_TGTDETROIT) || defined(_TGTDAYTONA))                // target build flags is defined. it is a fatal 
      #error more than one target build flag has been defined.       // compilation error to define more than one!
   #endif
#elif defined(_TGTDETROIT)
   #if (defined(_TGTDAKARF5) || defined(_TGTDAYTONA))
      #error more than one target build flag has been defined.
   #endif
#elif defined(_TGTDAYTONA)
   #if (defined(_TGTDETROIT) || defined(_TGTDAKARF5))
      #error more than one target build flag has been defined.
   #endif
#else
   #error no target build flag has been defined.
#endif


#include "math.h"                                                    // to init trig and 2^x LUTs...

#if defined(_TGTDAKARF5)                                             // board-specific includes....
   #include "f5_c4x.h"
   #include "f5.h"
#elif defined(_TGTDETROIT) 
   #include "sstype.h"
   #include "ssp_c6x.h"
   #include "de62c6x.h"
   #include "plx_def.h" 
#else    // _TGTDAYTONA
   #include "ftc6x.h"
#endif




//===================================================================================================================== 
// SHARED CONSTANTS/DEFINITIONS 
//
// This section defines the constants and data structures that are used both by XYCORE and the host-side CCxScope 
// device object.  Care must be taken to update these definitions when any relevant changes are made on the CCxScope 
// side and vice versa -- to keep the two sides "in synch"!  They should probably appear in a shared header file, but I 
// chose not to since the build tools for the TI C6x and C44 are very different than MS Dev Studio's C++ build tools... 
//
//===================================================================================================================== 

#define NO_TARGET          0           // supported target types:  "no target" placeholder,
#define DOTARRAY           1           //    rectangular dot array or point target (no target pattern)
#define FULLSCREEN         2           //    full-screen random-dot pattern (no target window)
#define RECTWINDOW         3           //    movable rect window on a movable full-screen random-dot pattern
#define RECTHOLE           4           //    movable rect hole in a movable full-screen random-dot pattern
#define ANNULUS            5           //    movable rect annulus on a movable full-screen random-dot pattern
#define STATICANNU         6           //    optimized version of RECTHOLE or ANNULUS that does not move at all
#define OPTRECTWIN         7           //    movable random-dot pattern restricted to movable rect window
#define DOTLIFEWIN         8           //    same as OPTRECTWIN, but dot life is limited
#define OPTICFLOW          9           //    simulates an optical flow field of dots (circular bounds)
#define ORIBAR             10          //    oriented rect bar or line of randomly arranged dots (no tgt pattern) 
#define DL_NOISEDIR        11          //    same as DOTLIFEWIN, but with noisy dot directions 
#define OPTCOHERENT        12          //    same as OPTRECTWIN, but implements percent coherence
#define DL_NOISESPEED      13          //    same as DL_NOISEDIR, but noise is in dot speed rather than direction

#define MAX_PIX            65535       // dotter board treats XY scope screen as a 2^16 by 2^16 grid of pixels, with 
#define CTR_PIX            32767       // (0,0) at bottom left corner, x increasing to the rt, y increasing upward

#define MAX_TARGETS        32          // max # of targets that can be animated at any one time

typedef struct Parameters              // "device-level" configuration and target information (pos info in pixels 
{                                      // [0..MAX_PIX], with screen center at (CTR_PIX,CTR_PIX)):
   unsigned int  dwDotSeed;            //    seed used in generation of targets' random-dot textures
   unsigned short wWidthMM;            //    display width in mm
   unsigned short wHeightMM;           //    display height in mm
   unsigned short wDistMM;             //    distance from screen to subject's eye, in mm
   unsigned short wNumTargets;         //    #targets currently defined
   unsigned short wDelayPerDot;        //    dot draw cycle delay prior to turning "on" each dot, and the "on" 
   unsigned short wOnTimePerDot;       //    duration, in dotter board clock cycles (depends on dotter board)
   unsigned short wFiller[2];          //    filler so that we stay on 4-byte boundaries
                                       //    for each target:
   unsigned short wType[MAX_TARGETS],  //       the target
               wNumDots[MAX_TARGETS],  //       number of dots in target texture
               wRectR[MAX_TARGETS],    //       right, left, top & bottom edges of target "window"; dot pattern 
               wRectL[MAX_TARGETS],    //       is visible inside or outside this window (depending on tgt type). 
               wRectT[MAX_TARGETS], 
               wRectB[MAX_TARGETS], 
               wOuterR[MAX_TARGETS],   //       defn of "outer" rectangular window for annular tgt types
               wOuterL[MAX_TARGETS], 
               wOuterT[MAX_TARGETS], 
               wOuterB[MAX_TARGETS]; 
} PARAMETERS, *PPARAMETERS;

typedef struct UpdateRec               // "device-level" per-target motion update record:
{
   short shWindowH, shWindowV;         //    pos change of target window, in pixels, WRT screen
   short shPatternH, shPatternV;       //    pos change of target pattern, in pixels, WRT target window
   short shNumReps;                    //    # times target should be "refreshed" during frame update
} UPDATEREC, *PUPDATEREC;

// !!! IMPORTANT !!! Some XY scope tgt types do not fit the parameterizations implied in the CCxScope::Parameters 
// and CCxScope::UpdateRec_Dev structures.  Here we list the exceptions:
//    DOTARRAY:   wRectR      ==> desired width of dot array, in pixels.
//                wRectL      ==> dot spacing in pixels, both horizontally & vertically (often 0, for point tgt)
//    DOTLIFEWIN: wOuterR     ==> dot "lifetime" (arbitrary units)
//                shNumReps   ==> upper byte = per-refresh decrement in dot life (arbitrary units); lower byte = 
//                                #times target should be refreshed per update.  each limited to [0..255]
//    OPTCOHERENT:wOuterL     ==> percent coherence, an integer in [0..100].
//    OPTICFLOW:  wRectR      ==> inner radius of flow field in deg/100
//                wRectL      ==> outer radius of flow field in deg/100
//                wRectT      ==> 1024 * (dist to eye) / (width of XY scope display)
//                wRectB      ==> 1024 * (dist to eye) / (height of XY scope display)
//                wOuterR     ==> H-coord of focus of expansion, initially CTR_PIX
//                wOuterL     ==> V-coord of FOE, initially CTR_PIX
//                shWindowH,V ==> change in pos of the flow field's FOE, in pixels
//                shPatternH  ==> velocity scale factor * 2^M, where M is set to facilitate integer-only calcs  
//                shPatternV  ==> the value of M
//    ORIBAR:     wRectR      ==> width of bar in vertical orientation, in pixels 
//                wRectL      ==> height of bar in vertical orienation, in pixels 
//                wRectT      ==> the drift axis angle, in deg CCW [0..360) 
//    DL_NOISEDIR and DL_NOISESPEED:
//                wOuterR     ==> dot "lifetime" (arbitrary units)
//                wOuterL     ==> noise range N.  For DL_NOISEDIR, N is an angular offset in integer deg [0..180].
//                                Each time noise update intv expires, an offset direc is randomly chosen from 
//                                [-N:N] for each dot in tgt.  This offset is added to the pattern direct to get 
//                                each dot's direction for subsequent frames.
//                                For DL_NOISESPEED, there are now (as of Maestro v2.1.3) two choices for speed 
//                                noise. wOuterB == 0 selects the original additive noise. Here, N is an integer 
//                                percentage in 1% increments, in [0..300]. Each time noise update intv expires, 
//                                each tgt dot is assigned a random offset% P in [-N:N]. The dot's radial 
//                                displacement in pixels is then R + P*R/100, where R is the nominal radial 
//                                displacement of the target dot pattern. wOuterB != 0 selects a form of 
//                                multiplicative noise. In this case, N is an integer exponent in [1..7]. When 
//                                the noise update intv expires, each tgt dot is assigned a random exponent X 
//                                uniformly chosen from [-N:N] (in increments of 0.05). The dot's radial 
//                                displacement is then (R * 2^X) / ((2^N - 2^(-N))/(2*N*ln(2))), where the 
//                                divisor is the expected value of 2^X when X is a uniform R.V. over [-N:N].
//                wOuterT     ==> Noise update interval M.  Dot noise is randomly regenerated every M ms.
//                wOuterB     ==> For DL_NOISESPEED only, if this is nonzero, multiplicative noise selected; else 
//                                the original additive %-age noise algorithm is used.
//                shPatternH  ==> radial cmpt R of pattern pos change expressed in POLAR coords, in screen mm, scaled by 
//                                2^10 if R>=0.1, 2^16 otherwise.
//                shPatternV  ==> theta of pattern pos change expressed in POLAR coords, in deg/10.
//                shNumReps   ==> upper byte = per-update decrement in dot life (arbitrary units); lower byte = 
//                                #times target should be refreshed per update.  each limited to [0..255]


// XYCORE does not actually use the UPDATEREC structure -- which is taken from the CCxScope declaration.  For 
// performance reasons, XYCORE accesses the target motion update records as a single array DATA[] of 16bit integers 
// (32bit in the case of the Dakar).  The different fields of the record are accessed using the indices below.  Thus, 
// eg, target N's update record is located at DATA[ N*UPDRECSZ + WIN_H...NREPS ]...

#define UPDRECSZ           5           // # of 16bit words in per-frame motion update record for each target
#define WIN_H              0           // corresponds to UPDATEREC.shWindowH
#define WIN_V              1           // corresponds to UPDATEREC.shWindowV
#define PAT_H              2           // corresponds to UPDATEREC.shPatternH
#define PAT_V              3           // corresponds to UPDATEREC.shPatternV
#define NREPS              4           // corresponds to UPDATEREC.shNumReps

                                       // CmdStat register -- possible values:
#define XYCORE_READY       1           //    status: XYCORE idle, ready for next command
#define XYCORE_INIT        2           //    command:  read target defns, etc and prepare targets for animation 
#define XYCORE_DOFRAME     3           //    command:  read motion update records & update defined targets accordingly 
#define XYCORE_CLOSE       4           //    command:  shut down XY scope controller

#define MAX_TRIGLEN        255         // maximum scope trigger length (delay + "ON" time) in dotter board clock ticks 
#define MAX_TRIGDEL        15          // maximum trigger delay in dotter board clock ticks




//===================================================================================================================== 
// LOCAL CONSTANT DEFINITIONS
//===================================================================================================================== 

#define MAXTOTALDOTS       30000       // maximum number of dots (over all tgts) that can be stored
#define MAXDOTSPERFRAME    4000        // max #dots that may be drawn during a given refresh period


// Local memory addresses for accessing dotter board registers.  These addresses are at the start of the DSP~LINK3 
// memory map for standard access (250ns fixed access).  They will be different for each board, since each board's 
// local memory map is different.  REMEMBER that the C4x (Dakar) addresses refer to 32-bit memory locations, while C6x 
// (Detroit, Daytona) addresses refer to 8-bit data.
//
//    LOCADDR  ==> Location Register (32-bit, write-only)
//    STATADDR ==> Status Register (32-bit, read-only)
//    TIMADDR  ==> Timing Register (32-bit, write-only)
//
#if defined(_TGTDAKARF5) 
   #define  LOCADDR        0xC0010000
   #define  STATADDR       0xC0010000
   #define  TIMADDR        0xC0010001
#elif defined(_TGTDETROIT)
   #define  LOCADDR        0x01740000
   #define  STATADDR       0x01740000
   #define  TIMADDR        0x01740004
#else    // _TGTDAYTONA
   #define  LOCADDR        0x01640000
   #define  STATADDR       0x01640000
   #define  TIMADDR        0x01640004
#endif


// XYCORE memory allocation scheme.
//
// [NOTE:  In our development of XYCORE for the Detroit board, we attempted to use global array declarations such as 
// UINT32 array[ARRAYSIZE], rather than the pointer scheme described here.  The XYCORE build succeeded, but it did not 
// run correctly.  See main() for more info.]
// 
// XYCORE requires a number of large arrays for storing dot positions, etc.  We set up pointers to specific "base 
// addresses" in the processor's local memory map and "promise ourselves" not to violate the space allotted to each 
// array.  Listed below are the base addresses of the two memory regions that are carved up by XYCORE arrays and 
// structures.  Since each target DSP board's memory map is different, these base addresses are board-specific.
//
//    SHDATA_BASE:  Local base address of "SharedData" memory area, containing target PARAMETERS struct and array of 
// motion update records (these are written by XYAPI running on host PC and are read-only to XYCORE).  The amount of 
// contiguous memory required here:  sizeof(PARAMETERS) + MAX_TARGETS * UPDRECSZ * sizeof(UINT16).
//    LOCALDATA_BASE:  Local base address of "LocalData" region in which all local (i.e., not shared with XYAPI) XYCORE 
// arrays are stored.  See main() for details about the different arrays allocated in this region, which must be 
// distinct from the program RAM.  The amount of contiguous memory required here is given by...
// 
//          MAXTOTALDOTS * 2 * sizeof(UINT16)      [current (x,y)-coords of all target dots]
//       +  MAXTOTALDOTS * sizeof(UINT16)          [current lifetimes of target dots in any finite dotlife tgts]
//       +  MAXTOTALDOTS * sizeof(INT16)           [current noise factors for DL_NOISEDIR and DL_NOISESPEED tgts]
//       +  MAXTOTALDOTS * 2 * sizeof(INT16)       [per-dot scaled frac parts of (dx,dy) for DL_NOISEDIR, DL_NOISESPEED]
//       +  MAX_TARGETS * 5 * sizeof(UINT16)       [other info about currently animated targets]
//       +  MAXDOTSPERFRAME * sizeof(UINT32)       [packed (x,y)-coords of all visible dots during current frame]
//       +  sizeof( PARAMETERS )                   [local copy of the PARAMETERS struct]
//
// !!!IMPORTANT!!! The memory requirements listed above are in units of the fundamental word size associated with the 
// target processor.  For the Detroit/Daytona, that unit is one byte; but for the Dakar, it is a 32-bit word (so 
// UINT16 maps to UINT32, and sizeof(UINT32) = 1)!!!!
// 
// [NOTE:  We do not have access to the PCI mailbox registers on the Daytona.  Therefore, we elected to use the first 
// 32-bit word in Node A's SSRAM as the "command/status register".  The "SharedData" area starts 16 bytes into SSRAM.] 
//
#if defined(_TGTDAKARF5)
   #define  SHDATA_BASE       0xC0300900                       // start of Far Global SRAM, plus 0x0900 (<512K x 32bit) 
   #define  LOCALDATA_BASE    0x80000000                       // start of Near Global SRAM (512K x 32bit)
#elif defined(_TGTDETROIT)
   #define  SHDATA_BASE       DE62_C6X_GLOBAL_RAM_BASE         // start of Async Global SRAM (512K x 32bit)
   #define  LOCALDATA_BASE    DE62_C6X_LOCAL_SDRAM_START       // start of Local SDRAM (4M x 32bit)
#else    // _TGTDAYTONA
   #define  SHDATA_BASE       0x00400010                       // start of Node A's SSRAM (128K x 32bit), plus 0x10 
   #define  LOCALDATA_BASE    0x02000000                       // start of Node A's local SDRAM (4M x 32bit)
#endif



//===================================================================================================================== 
// GLOBAL VARIABLE DEFINITIONS (these go in the .bss memory section of executable)
//===================================================================================================================== 

unsigned int   G_lastRandomNum = 1;          // for pseudorandom number generation
unsigned int   G_lastRand2 = 1;              // for random# generator for noise generation in DL_NOISEDIR/SPEED tgts



//===================================================================================================================== 
// FUNCTION DEFINITIONS
//===================================================================================================================== 

//=== SetSeed, SetSeed2 =============================================================================================== 
// 
//    Seed pseudorandom number generator with an unsigned integer. SetSeed() applies to the main generator for creating 
//    random dot patterns, etc.  SetSeed2() applies to a second generator dedicated to choosing random noise offset 
//    factors during animation of DL_NOISEDIR and DL_NOISESPEED targets.
//
//    (Adapted version of set_seed() from GNU C runtime library.)
//
//    ARGS:       seed -- [in] seed value.
//
//    RETURNS:    NONE.
//
void SetSeed( unsigned int seed )
{
   G_lastRandomNum = seed;
}

void SetSeed2( unsigned int seed )
{
   G_lastRand2 = seed;
}


//=== GetRandNum, GetRandNum2 ========================================================================================= 
//
//    Return next pseudorandom number, an unsigned short between 0 & the largest unsigned short int (platform-defined).
//    (Adapted version of random() from GNU C runtime library.)
//
//    We take the middle 16-bits of a 32-bit pseudorandom number. This is because we found that the sequence of 32-bit 
//    numbers follows the pattern {EVEN, ODD, EVEN, ODD, ...} or {ODD, EVEN, ODD, EVEN, ... } depending on whether the 
//    initial seed is odd or even, respectively.  On the Dakar, GetRandNum() returns a 32-bit unsigned int.  Since we 
//    mask out the top 16 bits, we're still guaranteed that the number returned lies in [0..65535].
//
//    GetRandNum() applies to the main generator for creating random dot patterns, etc.  GetRandNum2() applies to a 
//    second generator dedicated to choosing noise offsets during animation of DL_NOISEDIR and DL_NOISESPEED targets.
//
//    ARGS:       NONE.
//
//    RETURNS:    next pseudorandom # in the sequence.
//
unsigned short GetRandNum()
{
   unsigned int a = 2147437301, c = 453816981;
   
   G_lastRandomNum = a * G_lastRandomNum + c;
   return ( (unsigned short) (0x0000FFFF & (G_lastRandomNum >> 8)) );
}

unsigned short GetRandNum2()
{
   unsigned int a = 2147437301, c = 453816981;
   
   G_lastRand2 = a * G_lastRand2 + c;
   return ( (unsigned short) (0x0000FFFF & (G_lastRand2 >> 8)) );
}




//=== main ============================================================================================================ 
//
//    Drive the dotter board IAW target parameters furnished via the XYCORE_INIT command and per-frame motion update 
//    records provided via the XYCORE_DOFRAME command.  Set status to XYCORE_READY upon finishing a command; then wait 
//    for the next command.  Exit upon receipt of the XYCORE_CLOSE command.
//
void main() 
{
   INT16                hv, vv;           // horiz & vert displacement of random-dot texture associated with target
   INT16                hw, vw;           // horiz & vert displacement of target window
   INT16                nDotLifeDecr;     // per-refresh decrease in current dot lifetime

   register INT32       i32val;           // this (hopefully dedicated!) register var is used for the animation 
                                          // calculations implementing the OPTICFLOW target type
   INT32                x32, y32;
   INT16                i16Theta;
   INT16                i16Scale;
                                          // these are designated 'register' to speed-up boundary comparisons for 
                                          // certain tgt types during XYCORE_DOFRAME processing:
   register UINT16      xCoord;           // coordinates of a dot in pixel units [0..MAX_PIX]
   register UINT16      yCoord;           //
   register UINT16      rectR;            // the right, left, top and bottom sides of a rect 
   register UINT16      rectL;            // 
   register UINT16      rectU;            // 
   register UINT16      rectD;            // 

   UINT16               rectW;            // width, height of window for current target 
   UINT16               rectH;            //
   UINT16               screenW_mm;       // display width and height in mm
   UINT16               screenH_mm;
   UINT16               u16Type;          //
   UINT16               u16Dummy, u16tmp; // 
   UINT16               u16Over;
   UINT16               nMaxDotLife;      // max dot life [1..32767] (finite dotlife tgts only) 
   UINT32               command;          // new command from host XYAPI
   UINT32               status;           // used to reset CmdStat reg to XYCORE_READY
   UINT32               xyvals;           // the next beam location to write: [x15..x0 y15..y0]
   UINT32               timdurbyte;       // trigger duration
   UINT32               timdelnib;        // trigger delay
   UINT32               timvals;          // the trigger parameters packed appropriately for writing to timing reg
   UINT32               *locaddr;         // dotter board's location register
   UINT32               *stataddr;        // dotter board's status register
   UINT32               *timaddr;         // dotter board's timing register
   UINT32               i,j,k,l,m,d,cd;
   UINT32               dotPosOffset;     // offset into dot position arrays
   register UINT32      nTotalVisDots;    // total #dots that are visible in a given frame
   UINT32               maxRepeats;       // max # repeats in a given frame (targets may have different repeat values)
   double               dDummy;           // used to initialize trig and other LUTs

   // NOTE on array declarations:  We statically allocate all memory that we need.  Using an array declaration such as 
   // "UINT32 arrayName[ARRAYSIZE]" did not work because apparently it does not tell the special-purpose compiler where 
   // to allocate space for the array within the local processor's memory map.  We tried such a declaration during
   // program development (targeted for the Detroit C6x), and the resulting core program did not appear to function at 
   // all.  Hence, each array is defined by a pointer, each pointer is assigned to the array's start address in the 
   // local processors's memory map, and care is taken in assigning these start addresses so that the defined arrays do 
   // not overlap.  Then individual elements of the array may be accessed using ptrName[i] or *(ptrName + i).
   //
   register UINT16      *a;               // x-coord of next dot to be drawn (ptr into xdotpos)
   register UINT16      *b;               // y-coord of next dot to be drawn (ptr into ydotpos)
   register UINT16      *de;              // ptr into xdotpos, points to element AFTER x-coord of last dot to be drawn
   UINT16               *xdotpos;         // array of x-coords for dots of all targets, stored sequentially: target0,
                                          //    target1, ... [maxsize = MAXTOTALDOTS * sizeof(UINT16)]
   UINT16               *ydotpos;         // array of y-coords for dots of all targets, stored sequentially: target0,
                                          //    target1, ... [maxsize = MAXTOTALDOTS * sizeof(UINT16)]
   volatile INT16       *dotLife;         // [finite dotlife tgts] array of current lifetimes of dots of all finite  
                                          //    dotlife targets.  units=(set by host). counts down until negative.
                                          //    [maxsize = MAXTOTALDOTS * sizeof(INT16)]
   volatile INT16       *nextDotLife;     // [finite dotlife tgts] lifetime of next dot to be drawn (ptr into dotLife)
   volatile INT16       *dotNoise;        // [DL_NOISEDIR/SPEED] array of per-dot noise offsets. units = deg/10 for
                                          //    DL_NOISEDIR, or 1% increments for DL_NOISESPEED. 
                                          //    [maxsize=MAXTOTALDOTS * sizeof(INT16)] 
   volatile INT16       *nextDotNoise;    // [DL_NOISEDIR/SPEED] ptr into array 'dotNoise'
   volatile INT16       *fracDX;          // [DL_NOISEDIR/SPEED] array of per-dot fractional parts of x-coordinate
                                          //    displacements to be carried over to the next update. Scaled.
                                          //    [maxsize = MAXTOTALDOTS * sizeof(INT16)
   volatile INT16       *nextFracDX;      // [DL_NOISEDIR/SPEED] ptr into array 'fracDX'
   volatile INT16       *fracDY;          // [DL_NOISEDIR/SPEED] array of per-dot fractional parts of y-coordinate
                                          //    displacements to be carried over to the next update. Scaled.
                                          //    [maxsize = MAXTOTALDOTS * sizeof(INT16)
   volatile INT16       *nextFracDY;      // [DL_NOISEDIR/SPEED] ptr into array 'fracDY'
   volatile PARAMETERS  *parameters;      // local storage for target parameters obtained from host CPU [size = 
                                          //    sizeof(PARAMETERS)]
   volatile UINT16      *hsize;           // target window widths [size = MAX_TARGETS * sizeof(UINT16)]
   volatile UINT16      *vsize;           // target window heights [size = MAX_TARGETS * sizeof(UINT16)]
   volatile UINT16      *nRedrawsLeft;    // how many more times target must be redrawn during current refresh 
                                          //    period [size = MAX_TARGETS * sizeof(UINT16)]
   volatile UINT16      *nVisDotsPerTgt;  // #dots visible in current refresh period for each tgt 
                                          //    [size = MAX_TARGETS * sizeof(UINT16)]
   volatile INT16       *nNoiseUpdTicks;  // countdown timer for DL_NOISEDIR/SPEED noise update interval, in msecs 
                                          //    [size = MAX_TARGETS * sizeof(INT16)]
   volatile UINT32      *visibleDotsXY;   // packed (x,y)-coords of all dots visible in current refresh period 
                                          //    [size = MAXDOTSPERFRAME * sizeof(UINT32)]
   volatile UINT32      *SharedArray;     // points to start of shared memory, where host XYAPI stores target
                                          //    parameters followed by motion update records for the current frame
                                          //    [size = sizeof(PARAMETERS) + MAX_TARGETS * UPDRECSZ * sizeof(INT16)]
   volatile INT16       *data;            // points to start of target displacement data within shared memory
                                          //    [size = MAX_TARGETS * UPDRECSZ * sizeof(INT16)]

   volatile INT16       *tanLUT;          // lookup table for tan(P)*2^10, where P = [0..4499] in deg/100
                                          //    [size = 4500 * sizeof(INT16)]
   volatile INT16       *sincosLUT;       // lookup table for sin(P)*cos(P)*2^10, where P = [0..4499] in deg/100
                                          //    [size = 4500 * sizeof(INT16)]
   volatile INT16       *sinLUT;          // lookup table for sin(P)*2^10, where P = [0..3599] in deg/10
                                          //    [size = 3600 * sizeof(INT16)]
   volatile INT16       *cosLUT;          // lookup table for tan(P)*2^10, where P = [0..3599] in deg/10
                                          //    [size = 3600 * sizeof(INT16)]
   volatile INT32       *pow2LUT;         // lookup table for pow(2.0, (P-140)/20)*2^20, where P = [0..280]
                                          //    [size = 281 * sizeof(INT32)]
   volatile INT32       *speedNoiseAdj;   // lookup table for the multiplicative speed noise adj factor E(N) = 
                                          //    (2^10) * (2^N - 2^(-N)) / (2 * ln(2) * N), where N=1..7.
                                          //    [size = 7 * sizeof(INT32)]

/*
// DEBUG
#ifdef _TGTDAYTONA
   volatile UINT32      *ledReg;

ledReg = (UINT32 *)0x01700028;
d = 0;
#endif
*/

   #if defined(_TGTDAKARF5)                                             // inits: init board, PCI interface, and 
      C4X_Open( SHARED_SRAM_512K );                                     // DSPLINK3. not all operations are required 
      *((UINT32 *)0xC0200004) = 0x00000001;                             // for every DSP target... 
      *((UINT32 *)0xC0200004) = 0x00000000;
   #elif defined(_TGTDETROIT)
      C6x_OpenC6x( NO_FLAGS );
      C6x_OpenPlx( NO_FLAGS );
      C6x_ControlResetDspLink3( DE62_CONTROL_RELEASE_DL3_RESET );
      C6x_ControlLed( DE62_C6X_CONTROL_LED_GP_OFF );
   #else    // _TGTDAYTONA 
      C6x_OpenC6x( NO_FLAGS );
      C6x_OpenHurricane( NO_FLAGS );
      C6x_ControlResetDspLink3( FT_CONTROL_RELEASE_DL3_RESET );
      C6x_ControlLed( FT_C6X_LED_0_OFF | FT_C6X_LED_1_OFF );
   #endif

   locaddr = (UINT32 *) LOCADDR;                                        // set up addresses to access dotter board regs 
   stataddr = (UINT32 *) STATADDR;
   timaddr = (UINT32 *) TIMADDR;

   SharedArray = (UINT32*) SHDATA_BASE;                                 // "allocate" all program array in the local 
   data = (INT16*) ((UINT32)SHDATA_BASE + (UINT32)sizeof(PARAMETERS));  // processor's memory map by assigning the 
                                                                        // approp memory addresses to pointer vars...
   i = 0;
   #if defined(_TGTDETROIT)                                             // for the Detroit, dot pos arrays are stored 
      xdotpos = (UINT16 *) DE62_C6X_LOCAL_SSRAM_START;                  // in a faster "local memory" region than the 
      i += MAXTOTALDOTS * sizeof(UINT16);                               // other vars: SSRAM is ~5% faster than SDRAM 
      ydotpos = (UINT16 *) (DE62_C6X_LOCAL_SSRAM_START + i); 
      i = 0;
   #else    // _TGTDAYTONA, _TGTDAKARF5 
      xdotpos = (UINT16 *) LOCALDATA_BASE; 
      i += MAXTOTALDOTS * sizeof(UINT16);
      ydotpos = (UINT16 *) (LOCALDATA_BASE + i);
      i += MAXTOTALDOTS * sizeof(UINT16);
   #endif
   
   dotLife = (volatile INT16 *) (LOCALDATA_BASE + i);                   // REM: sizeof() returns sizes in units of the 
   i += MAXTOTALDOTS * sizeof(INT16);                                   // fundamental data size of tgt processor. On 
   dotNoise = (volatile INT16 *) (LOCALDATA_BASE + i);                  // the Detroit/Daytona, this is a byte, but on 
   i += MAXTOTALDOTS * sizeof(INT16);                                   // the Dakar it is a 4-byte word.  All arrays 
   fracDX = (volatile INT16 *) (LOCALDATA_BASE + i);                    // on the Dakar will be INT32 or UINT32, 
   i += MAXTOTALDOTS * sizeof(INT16);                                   // regardless of the types we use here!!!!!
   fracDY = (volatile INT16 *) (LOCALDATA_BASE + i);
   i += MAXTOTALDOTS * sizeof(INT16);
   hsize = (volatile UINT16 *) (LOCALDATA_BASE + i);
   i += MAX_TARGETS * sizeof(UINT16);
   vsize = (volatile UINT16 *) (LOCALDATA_BASE + i); 
   i += MAX_TARGETS * sizeof(UINT16); 
   nRedrawsLeft = (volatile UINT16 *) (LOCALDATA_BASE + i);
   i += MAX_TARGETS * sizeof(UINT16);
   nVisDotsPerTgt = (volatile UINT16 *) (LOCALDATA_BASE + i);
   i += MAX_TARGETS * sizeof(UINT16);
   nNoiseUpdTicks = (volatile INT16 *) (LOCALDATA_BASE + i);
   i += MAX_TARGETS * sizeof(INT16);
   visibleDotsXY = (volatile UINT32 *) (LOCALDATA_BASE + i);
   i += MAXDOTSPERFRAME * sizeof(UINT32);

   tanLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                    // lookup tables for OPTICFLOW animation calcs! 
   i += 4500 * sizeof(INT16);
   sincosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);
   i += 4500 * sizeof(INT16);
   sinLUT = (volatile INT16 *) (LOCALDATA_BASE + i);
   i += 3600 * sizeof(INT16);
   cosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);
   i += 3600 * sizeof(INT16);

   pow2LUT = (volatile INT32 *) (LOCALDATA_BASE + i);                   // special lookup tables for NOISYSPEED target's
   i += 281 * sizeof(INT32);                                            // multiplicative noise algorithm
   speedNoiseAdj = (volatile INT32 *) (LOCALDATA_BASE + i);
   i += 7 * sizeof(INT32);

   parameters = (volatile PARAMETERS *) (LOCALDATA_BASE + i);
   i += sizeof( PARAMETERS );


   
   command = status = XYCORE_READY;                                     // initialize CmdStat register: Detroit and 
   #if defined(_TGTDAKARF5)                                             // Dakar use PCI runtime mailbox reg#2 for 
      C4X_Control( PCI, SET_MAILBOX, 2, &status );                      // this, while Daytona (NodeA) uses the first 
   #elif defined(_TGTDETROIT)                                           // 32bit word in its SSRAM...
      C6x_WritePlx( MAILBOX2_OFFSET, status );                          // NOTE: We set "ready" status here so CXDRIVER 
   #else    // _TGTDAYTONA                                              // does not have to wait while we init the trig 
      *((UINT32 *)0x00400000) = status;                                 // tables, which takes a while!
   #endif


   for( i = 0; i < 4500; i++ )                                          // initialize all lookup tables...
   {
      dDummy = ((double) i) * 0.00017453293;                            //    convert integer deg/100 to radians
      tanLUT[i] = (INT16) floor( 1024.0 * tan( dDummy ) + 0.5 );
      sincosLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) * cos( dDummy ) + 0.5 );
   }
   for( i = 0; i < 3600; i++ )
   {
      dDummy = ((double) i) * 0.0017453293;                             //    convert integer deg/10 to radians
      sinLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) + 0.5 );
      cosLUT[i] = (INT16) floor( 1024.0 * cos( dDummy ) + 0.5 );
   }
   
   for( i = 0; i < 281; i++ )
   {
      dDummy = (((double) i) - 140) / 20.0;
      pow2LUT[i] = (INT32) floor(pow(2.0, dDummy + 20.0) + 0.5);
   }
   for( i = 0; i < 7; i++ )
   {
      j = i+1;
      dDummy = 1024.0 * (pow(2.0, (double)j) - pow(2.0, -((double)j)));
      dDummy /= (2.0 * ((double)j) * log(2.0));
      speedNoiseAdj[i] = (INT32) floor(dDummy + 0.5);
   }
   

/*
// DEBUG
*ledReg = 0x00000003;
*/



   do                                                                   // BEGIN: runtime loop -- process cmds from 
   {                                                                    // host XYAPI until XYCORE_CLOSE is received.
      do                                                                // wait for next command in CmdStat reg
      {
         #if defined(_TGTDAKARF5)
            C4X_Control( PCI, GET_MAILBOX, 2, &command );
         #elif defined(_TGTDETROIT)
            C6x_ReadPlx( MAILBOX2_OFFSET, &command );
         #else    // _TGTDAYTONA
            command = *((UINT32 *)0x00400000);

/*
// DEBUG
++d;
if ( d > 1000000 )
{
d = 0;
if ( (*ledReg) & 0x00000002 )
   *ledReg &= ~0x00000002;
else
   *ledReg |= 0x00000002;
}
*/

         #endif
      } while( command == XYCORE_READY );


      if( command == XYCORE_INIT )                                      // BEGIN:  process XYCORE_INIT command
      {
         memcpy( (void*) parameters, (void*) SharedArray,               // copy params into local mem for faster access 
                 sizeof(PARAMETERS) );

         SetSeed( parameters->dwDotSeed );                              // seed both random# generators using the seed 
         SetSeed2( parameters->dwDotSeed );                             // value provided

         dotPosOffset = 0;                                              // protect against overflow of dot pos arrays: 
         for( d = 0; d < parameters->wNumTargets; d++ )                 // if necessary, reduce #targets processed so  
         {                                                              // that total #dots to be stored falls under 
            if( dotPosOffset + parameters->wNumDots[d] > MAXTOTALDOTS ) // the maximum allowed limit...
               break;
            dotPosOffset += parameters->wNumDots[d];
         }
         parameters->wNumTargets = d;


         dotPosOffset = 0;                                              // generate & store the initial (x,y)-coords of  
         for( d = 0; d < parameters->wNumTargets; d++ )                 // dots for all tgts in PARAMETERS struct.
         { 
            u16Type = parameters->wType[d];                             // this target's type

            if( u16Type == NO_TARGET )                                  // NO_TARGET: not a target; nothing to do here. 
               ;

            else if ( (u16Type == DOTARRAY) &&                          // DOTARRAY: Nonrandom, rect array of regularly 
                      (parameters->wNumDots[d] > 0) )                   // spaced dots.
            {
               if( parameters->wRectR[d] > 32768 )                      //    width of array.  enforce maximum value.
                  parameters->wRectR[d] = 32768;
               if( parameters->wRectL[d] > 32768 )                      //    dot spacing.  enforce maximum value.
                  parameters->wRectL[d] = 32768;

               cd = parameters->wRectR[d] / 2;                          //    draw array from L->R, B->T starting w/dot 
               xdotpos[dotPosOffset] = CTR_PIX - cd;                    //    at lower left corner. init pos of this 
               ydotpos[dotPosOffset] = CTR_PIX - cd;                    //    dot so that array is ctr'd at origin.

               m = CTR_PIX + cd;                                        //    right boundary of array

               for( i = 1; i < parameters->wNumDots[d]; i++ )           //    draw remaining dots from left to right, 
               {                                                        //    a row at a time (H = V spacing):
                  j = (UINT32) xdotpos[dotPosOffset + i-1];             //       loc of last dot
                  k = (UINT32) ydotpos[dotPosOffset + i-1];
                  l = (UINT32) parameters->wRectL[d];
                  if( j + l >= m )                                      //       move up to next row of dots
                  { 
                     if( k + l > MAX_PIX ) break;                       //       out of room in upper-right quad. stop!
                     xdotpos[dotPosOffset + i] = CTR_PIX - cd; 
                     ydotpos[dotPosOffset + i] = (UINT16) (k + l); 
                  } 
                  else                                                  //       move to next dot in row
                  { 
                     xdotpos[dotPosOffset + i] = (UINT16) (j + l); 
                     ydotpos[dotPosOffset + i] = (UINT16) k; 
                  } 
               }
               parameters->wNumDots[d] = i;                             //       #dots reduced if array did not fit in
                                                                        //       upper-right quadrant!
            }

            else if( u16Type == ORIBAR )                                // ORIBAR: rect bar or line of dots oriented at 
            {                                                           // a specific angle in [0..360)...
               hw = (INT16) (parameters->wRectR[d] >> 1);               //    half-width of bar
               vw = (INT16) (parameters->wRectL[d] >> 1);               //    half-height of bar

               if( parameters->wRectT[d] >= 360 )                       //    drift axis angle, limited to [0..360)
                  parameters->wRectT[d] = 0;
               xCoord = 10 * parameters->wRectT[d];                     //    convert to deg/10

               hv = (INT16) sinLUT[xCoord];                             //    1024*sin(A), where A = drift axis angle
               vv = (INT16) cosLUT[xCoord];                             //    1024*cos(A)

               if( vw == 0 )                                            //    if zero half-height, bar is NOT drawn!
               {                                                        //    We put all the dots at (0,0), but we
                  for(k=0; k<parameters->wNumDots[d]; k++)              //    don't draw them in DOFRAME processing.
                  {
                     xdotpos[dotPosOffset + k] = (UINT16) 0;
                     ydotpos[dotPosOffset + k] = (UINT16) 0;
                  }
               }
               else if( hw == 0 )                                       //    if zero half-width, bar is just a line:
               {
                  for( k = 0; k < parameters->wNumDots[d]; k++ )
                  {
                     // bar half-ht in pixels -> 2^10 x (half-ht in mm)
                     y32 = (INT32) vw;
                     y32 *= parameters->wHeightMM;
                     y32 >>= 6;
                     
                     // yMM*2^10: dots uniformly distributed in [-h/2..h/2] along y-axis (x-coord is 0), h in mm.
                     i32val = y32;
                     i32val *= (INT32) 2 * k;
                     i32val /= parameters->wNumDots[d];
                     i32val -= y32;
                     y32 = i32val;
                     
                     // now do rotation transformation in true screen coords: (0,yMM) -> (xMM', yMM'). Then convert 
                     // back to pixels: (xPx', yPx'). Note that we have to remove scale factors 64 and 1024 as we do
                     // the calcs. These scale factors let us do integer arithmetic w/o losing too much precision in 
                     // the final result.
                     i32val *= (-hv);                                   // -(yMM*2^10)*(2^10)*sinA = xMM' * 2^20
                     i32val /= (INT32) parameters->wWidthMM;            // xMM'*2^4*(2^16/screenW_mm) = xPix'*2^4
                     i32val >>= 4;                                      // xPix'
                     i32val += 32767;                                   // translate to device origin
                     xdotpos[dotPosOffset + k] = (UINT16)i32val;
                     
                     i32val = y32 * vv;                                 // (yMM*2^10)*(2^10)*cosA = yMM' * 2^20
                     i32val /= (INT32) parameters->wHeightMM;           // yMM'*2^4*(2^16/screenH_mm) = yPix'*2^4
                     i32val >>= 4;                                      // yPix'
                     i32val += 32767;                                   // translate to device origin
                     ydotpos[dotPosOffset + k] = (UINT16)i32val;
                  }
               }
               else                                                     //    general case: a rect bar w/ random dots: 
               {
                  for( k = 0; k < parameters->wNumDots[d]; k++ )
                  {
                     xCoord = ((UINT16) GetRandNum());                  // random x-coord xPix in [-w/2 .. w/2]
                     x32 = (INT32) (xCoord % parameters->wRectR[d]);
                     x32 -= (INT32) hw;
                     x32 *= parameters->wWidthMM;                       // xPix -> 2^6 * xMM
                     x32 >>= 10;
                     
                     yCoord = ((UINT16) GetRandNum());                  // random y-coord yPix in [-h/2 .. h/2]
                     y32 = (INT32) (yCoord % parameters->wRectL[d]);
                     y32 -= (INT32) vw;
                     y32 *= parameters->wHeightMM;                      // yPix -> 2^6 * yMM
                     y32 >>= 10;

                     // rotation transformation: (xMM*2^6)*1024*cosA - (yMM*2^6)*1024*sinA = (xMM*cosA - yMM*sinA)*2^16
                     // = xMM'*2^16. xMM'*2^16/screenW_mm = xPix'. Translate to device origin. Analgously for y-coord,
                     // (xMM*2^6)*1024*sinA + (yMM*2^6)*1024*cosA = yMM'*2^16. yMM'*2^16/screenH_mm = yPix'.
                     i32val = x32 * vv - y32 * hv;
                     i32val /= (INT32) parameters->wWidthMM; 
                     i32val += 32767; 
                     xdotpos[dotPosOffset + k] = (UINT16)i32val; 

                     i32val = x32 * hv + y32 * vv;
                     i32val /= (INT32) parameters->wHeightMM; 
                     i32val += 32767; 
                     ydotpos[dotPosOffset + k] = (UINT16)i32val; 
                  }
               }
            }

            else if( u16Type == STATICANNU )                            // STATICANNU: Optimized implementation of rect 
            {                                                           // annulus when neither window nor dots move.
               l = 0;                                                   // dots always stay at their initial positions! 
               for ( k = 0; k < parameters->wNumDots[d]; k++ )          // we generate dots randomly, then drop all 
               {                                                        // those which are outside the annular window!
                  xCoord = (UINT16) GetRandNum(); 
                  yCoord = (UINT16) GetRandNum(); 
                  if( (xCoord >= parameters->wOuterL[d]) && (xCoord <= parameters->wOuterR[d]) &&
                      (yCoord >= parameters->wOuterB[d]) && (yCoord <= parameters->wOuterT[d]) &&
                      ((xCoord > parameters->wRectR[d]) || (xCoord < parameters->wRectL[d]) ||
                       (yCoord > parameters->wRectT[d]) || (yCoord < parameters->wRectB[d])) )
                  {
                     xdotpos[dotPosOffset + l] = xCoord;
                     ydotpos[dotPosOffset + l] = yCoord;
                     l++;
                  }
               }
               parameters->wNumDots[d] = l;
            }

            else if( u16Type == OPTRECTWIN || u16Type == OPTCOHERENT || // OPTRECTWIN, etc: tgt dots randomly distrib. 
                     u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR || // within boundaries (incl edges) of the 
                     u16Type == DL_NOISESPEED )                         // visible window...
            { 
               hsize[d] = parameters->wRectR[d] - parameters->wRectL[d] + 1;
               vsize[d] = parameters->wRectT[d] - parameters->wRectB[d] + 1;
               for( k = 0; k < parameters->wNumDots[d]; k++ )
               {
                  xCoord = (UINT16) GetRandNum();
                  yCoord = (UINT16) GetRandNum();
                  xdotpos[dotPosOffset + k] = parameters->wRectL[d] + (xCoord % hsize[d]);
                  ydotpos[dotPosOffset + k] = parameters->wRectB[d] + (yCoord % vsize[d]);
               }

               // for these types, we also assign a random lifetime between 1 and target's maximum dot life. We also
               // make sure the per-dot fractional pixel displacements are initially 0 (appl to noisy tgts only).
               if(u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||  u16Type == DL_NOISESPEED) 
               { 
                  nMaxDotLife = (UINT16) parameters->wOuterR[d];
                  if( nMaxDotLife < 1 ) nMaxDotLife = 1;
                  else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767; 
                  for( k = 0; k < parameters->wNumDots[d]; k++ )
                  {
                     xCoord = (UINT16) GetRandNum();
                     dotLife[dotPosOffset + k] = (xCoord % nMaxDotLife) + 1;
                     
                     fracDX[dotPosOffset + k] = 0;
                     fracDY[dotPosOffset + k] = 0;
                  }
               }

               if( u16Type == DL_NOISEDIR || u16Type == DL_NOISESPEED ) //    noise update timer reset so that per-dot 
                  nNoiseUpdTicks[d] = 0;                                //    noise factors are randomly chosen on the 
                                                                        //    very first update frame!
            }

            else if( u16Type == OPTICFLOW )                             // OPTICFLOW: flow field. dot pos stored in 
            {                                                           // polar coords (r,TH) rather than (x,y) pix:
               rectR = parameters->wRectR[d];                           //    inner radius in deg/100 of visual angle
               ++rectR;                                                 //    no dots AT inner or outer rad initially
               rectL = parameters->wRectL[d];                           //    outer radius in deg/100 of visual angle
               rectL -= rectR;                                          //    difference in deg/100

               for( k = 0; k < parameters->wNumDots[d]; k++ ) 
               { 
                  xCoord = (UINT16) GetRandNum();                       //    init random radial pos in visual deg/100 
                  xdotpos[dotPosOffset + k] = rectR + (xCoord % rectL); 
                  yCoord = (UINT16) GetRandNum();                       //    init angular pos in deg/10
                  ydotpos[dotPosOffset + k] = (yCoord % 3600); 
                  dotLife[dotPosOffset + k] = 0;                        //    reset frac pos change (<1/100deg)
               } 
            }

            else                                                        // ALL OTHER TYPES:  random-dot texture drawn 
            {                                                           // to fill the entire screen...
               for ( k = 0; k < parameters->wNumDots[d]; k++ ) 
               { 
                  xdotpos[dotPosOffset + k] = (UINT16) GetRandNum();
                  ydotpos[dotPosOffset + k] = (UINT16) GetRandNum();
               } 
            }
            
            dotPosOffset += parameters->wNumDots[d];                    // move offset into dot pos arrays so that it 
                                                                        // points to loc after current target's dots 
         } 
      }                                                                 // END:  process XYCORE_INIT command


      else if( command == XYCORE_DOFRAME )                              // BEGIN:  process XYCORE_DOFRAME command...
      {
         timdelnib = (UINT32) parameters->wDelayPerDot;                 // write trig timing params to dotter board 
         if ( timdelnib > MAX_TRIGDEL )                                 // timing register.  check values.
            timdelnib = MAX_TRIGDEL;
         timdurbyte = timdelnib + ((UINT32) parameters->wOnTimePerDot);
         if ( timdurbyte > MAX_TRIGLEN ) 
            timdurbyte = MAX_TRIGLEN;
         timvals = ((timdurbyte & 0x00F0) << 20) | (timdelnib << 20) | ((timdurbyte & 0x000F) << 28);
         *timaddr = timvals;
         
         maxRepeats = 0;                                                // find largest "#reps per frame" across all 
         for( d = 0; d < parameters->wNumTargets; d++ )                 // defined targets.  B/c "dotlife" tgts use the 
         {                                                              // upper byte of NREPS field to store dot life 
            cd = d * UPDRECSZ;                                          // decrement, we must mask that out here!
            u16Dummy = (UINT16) data[cd + NREPS];
            if( parameters->wType[d] == DOTLIFEWIN || 
                parameters->wType[d] == DL_NOISEDIR || 
                parameters->wType[d] == DL_NOISESPEED ) 
               u16Dummy &= 0x000000FF;
            if ( ((UINT32) u16Dummy) > maxRepeats ) 
               maxRepeats = (UINT32) u16Dummy;
         }

         // these are only used by DL_NOISEDIR and DL_NOISESPEED. We put the values in local vars so we're not
         // constantly accessing the PARAMETERS struct in shared memory (slower access)
         screenW_mm = parameters->wWidthMM;
         screenH_mm = parameters->wHeightMM;
         
         dotPosOffset = 0;                                              // BEGIN:  first pass thru all targets
         nTotalVisDots = 0;
         if( maxRepeats > 0 ) for( d = 0; d < parameters->wNumTargets; d++ ) 
         {
            cd = d * UPDRECSZ;                                          // offset into array of motion update records;
                                                                        // locates start of record for this tgt 
            u16Type = parameters->wType[d];                             // this target's type

            u16Dummy = (UINT16) data[cd + NREPS];                       // #reps per frame for this tgt (mask out dot 
            if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // life decr in upper byte if "dotlife" type) 
                u16Type == DL_NOISESPEED )
               u16Dummy &= 0x000000FF;

            if( (u16Type == NO_TARGET) ||                               // if "non-target", if #dots = 0, or if #reps 
                (parameters->wNumDots[d] == 0) || (u16Dummy == 0) ||    // is zero for this target, or if it's a 
                (u16Type == ORIBAR && parameters->wRectL[d] == 0) )     // zero-height ORIBAR target, then skip to 
            {                                                           // next target...
               nRedrawsLeft[d] = 0;
               nVisDotsPerTgt[d] = 0;
               dotPosOffset += parameters->wNumDots[d];
               continue;
            }

            hw = (INT16) data[cd + WIN_H];                              // target's window pos change for current frame 
            vw = (INT16) data[cd + WIN_V];
            hv = (INT16) data[cd + PAT_H];                              // tgt's pattern pos change for current frame
            vv = (INT16) data[cd + PAT_V];
                                                                        // deal with special cases:
            if( u16Type == STATICANNU )                                 //    STATICANNU - no window or pattern motion 
               hw = vw = hv = vv = 0;
            if( u16Type == FULLSCREEN )                                 //    FULLSCREEN - no window
               hw = vw = 0;
            if( (u16Type == DOTARRAY) || (u16Type == ORIBAR) )          //    DOTARRAY/ORIBAR - dots move together as 
            {                                                           //    an object. there's no window or pattern 
               hv = hw;                                                 //    in the sense of the windowed tgt types; 
               vv = vw;                                                 //    all dots drawn. assign "window" to 
               hw = vw = 0;                                             //    "pattern" vel, so we can implement like 
            }                                                           //    the FULLSCREEN tgt type (see below)

            if( u16Type != OPTICFLOW )                                  // update target window location.
            {                                                           // !!! UINT16 arithmetic! Windows wrap around 
               parameters->wRectR[d] += hw;                             // screen on Detroit/Daytona.  Won't happen on 
               parameters->wRectL[d] += hw;                             // Dakar, b/c UINT16 is actually UINT32.  It is 
               parameters->wRectT[d] += vw;                             // considered an error on user's part to have a 
               parameters->wRectB[d] += vw;                             // target window go past screen bounds!!!

               rectR = parameters->wRectR[d];                           // save current window bounds in register vars 
               rectL = parameters->wRectL[d];                           // to speed up comparisons which must be 
               rectU = parameters->wRectT[d];                           // performed for all dots
               rectD = parameters->wRectB[d];

               if( u16Type == ANNULUS )                                 // must update outer rect as well for ANNULUS; 
               {                                                        // note that we DO NOT assign register vars to 
                  parameters->wOuterR[d] += hw;                         // the bounds of the outer rect.
                  parameters->wOuterL[d] += hw;
                  parameters->wOuterT[d] += vw;
                  parameters->wOuterB[d] += vw;
               }
            }
            else                                                        // OPTICFLOW target is very different: window 
            {                                                           // is moved by changing coords of the FOE...
               rectR = (UINT16) parameters->wRectR[d];                  //    inner radius in visual deg/100
               rectL = (UINT16) parameters->wRectL[d];                  //    outer radius in visual deg/100
               rectU = (UINT16) parameters->wRectT[d];                  //    alphaX geometric conversion fac (* 1024) 
               rectD = (UINT16) parameters->wRectB[d];                  //    alphaY geometric conversion fac (* 1024)

               xCoord = parameters->wOuterR[d];                         //    update coords of the FOE now...
               xCoord += hw; 
               yCoord = parameters->wOuterL[d];
               yCoord += vw; 
               #ifdef _TGTDAKARF5                                       //    ensure UINT16 arith on 32-bit-only DAKAR
                  if( xCoord > 0x0000FFFF )
                  {
                     if( hw > 0 ) xCoord -= 65536;
                     else         xCoord &= 0x0000FFFF;
                  }
                  if( yCoord > 0x0000FFFF )
                  {
                     if( vw > 0 ) yCoord -= 65536;
                     else         yCoord &= 0x0000FFFF;
                  }
               #endif
               parameters->wOuterR[d] = xCoord;                         //    the new FOE coords are also preserved 
               parameters->wOuterL[d] = yCoord;                         //    in the register vars (xCoord, yCoord)...
                                                                        //    also, reg vars (hv,vv) = (B*2^M, M)!
            }

            de = &xdotpos[ dotPosOffset + parameters->wNumDots[d] ];    // set ptrs into target's dot position, dot 
            a  = &xdotpos[dotPosOffset];                                // lifetime, dot noise, and fracDX,DY arrays
            b  = &ydotpos[dotPosOffset]; 
            nextDotLife = &dotLife[dotPosOffset];
            nextDotNoise = &dotNoise[dotPosOffset];
            nextFracDX = &fracDX[dotPosOffset];
            nextFracDY = &fracDY[dotPosOffset];
            dotPosOffset += parameters->wNumDots[d];                    // now points to start of next target's dots


            if( (u16Type == DOTARRAY) ||                                // DOTARRAY/FULLSCREEN/ORIBAR:  Every tgt dot 
                (u16Type == FULLSCREEN) ||                              // is always drawn -- there's no "window" that 
                (u16Type == ORIBAR) )                                   // is distinct from the dot pattern.  
            { 
               nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    #dots in "visible dots" array for tgt 
               while( a < de ) 
               {
                  *a = *a + hv;
                  *b = *b + vv; 
                  #ifdef _TGTDAKARF5
                     if( *a > 0x0000FFFF )
                     {
                        if( hv > 0 ) *a -= 65536;
                        else         *a &= 0x0000FFFF;
                     }
                     if( *b > 0x0000FFFF )
                     {
                        if( vv > 0 ) *b -= 65536;
                        else         *b &= 0x0000FFFF;
                     }
                  #endif

                  xyvals = ((*a << 16) | *b);                           //    draw the dot
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed (X,Y) pos of each visible dot 
                  a++; 
                  b++; 
               }
            } 
            else if( u16Type == STATICANNU )                            // STATICANNU: Neither window nor pattern move, 
            {                                                           // so no need to update dot pos nor to make 
               nVisDotsPerTgt[d] = parameters->wNumDots[d];             // sure that dot is visible...
               while( a < de ) 
               { 
                  xyvals = ((*a << 16) | *b);
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;
                  a++; 
                  b++;
               }
            } 
            else if( u16Type == RECTWINDOW )                            // RECTWINDOW:  Independent pattern & window 
            {                                                           // motion.  Visible dots lie inside window.
               nVisDotsPerTgt[d] = 0;
               while( a < de ) 
               {
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + pattern displacement!
                  *a = *a + hw + hv;
                  *b = *b + vw + vv;
                  #ifdef _TGTDAKARF5
                     if( *a > 0x0000FFFF )
                     {
                        if(hw+hv > 0) *a -= 65536;
                        else          *a &= 0x0000FFFF;
                     }
                     if ( *b > 0x0000FFFF )
                     {
                        if(vw+vv > 0) *b -= 65536;
                        else          *b &= 0x0000FFFF;
                     }
                  #endif

                  if( (*a <= rectR) && (*a >= rectL) && (*b <= rectU) && (*b >= rectD) )
                  {
                     xyvals = ((*a << 16) | *b);
                     while( *stataddr & 0x1 );
                     *locaddr = xyvals;	
                     visibleDotsXY[nTotalVisDots++] = xyvals;
                     nVisDotsPerTgt[d]++;
                  }
                  a++; 
                  b++;
               } 
            } 
            else if( u16Type == RECTHOLE )                              // RECTHOLE:  Independent window & pattern 
            {                                                           // motion.  Visible dots lie outside window.
               nVisDotsPerTgt[d] = 0;
               while( a < de ) 
               {
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + pattern displacement!
                  *a = *a + hw + hv;
                  *b = *b + vw + vv;
                  #ifdef _TGTDAKARF5
                     if( *a > 0x0000FFFF )
                     {
                        if(hw+hv > 0) *a -= 65536;
                        else          *a &= 0x0000FFFF;
                     }
                     if( *b > 0x0000FFFF )
                     {
                        if(vw+vv > 0) *b -= 65536;
                        else          *b &= 0x0000FFFF;
                     }
                  #endif

                  if( (*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD) )
                  {
                     xyvals = ((*a << 16) | *b);
                     while( *stataddr & 0x1 );
                     *locaddr = xyvals;	
                     visibleDotsXY[nTotalVisDots++] = xyvals;
                     nVisDotsPerTgt[d]++;
                  }
                  a++; 
                  b++; 
               }
            } 
            else if( u16Type == ANNULUS )                               // ANNULUS:  Independent window and pattern 
            {                                                           // motion.  Visible dots lie inside annulus. 
               nVisDotsPerTgt[d] = 0;
               while( a < de ) 
               {
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + pattern displacement!
                  *a = *a + hw + hv;
                  *b = *b + vw + vv;
                  #ifdef _TGTDAKARF5
                     if( *a > 0x0000FFFF )
                     {
                        if(hw+hv > 0) *a -= 65536;
                        else          *a &= 0x0000FFFF;
                     }
                     if( *b > 0x0000FFFF )
                     {
                        if(vw+vv > 0) *b -= 65536;
                        else          *b &= 0x0000FFFF;
                     }
                  #endif

                  if( (*a <= parameters->wOuterR[d]) && (*a >= parameters->wOuterL[d]) && 
                      (*b <= parameters->wOuterT[d]) && (*b >= parameters->wOuterB[d]) &&
                      ((*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD)) )
                  {
                     xyvals = ((*a << 16) | *b);
                     while( *stataddr & 0x1 );
                     *locaddr = xyvals;	
                     visibleDotsXY[nTotalVisDots++] = xyvals;
                     nVisDotsPerTgt[d]++;
                  }
                  a++; 
                  b++; 
               }
            }
            else if( u16Type == OPTRECTWIN )                            // OPTRECTWIN: Independent pattern & window 
            {                                                           // motion, but all dots restricted to window... 
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               while ( a < de )                                         //    for each dot in target:
               {
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + pattern displacement!
                  xCoord = *a + hw + hv;                                //       update its position
                  yCoord = *b + vw + vv;
                  if( (xCoord > rectR) || (xCoord < rectL) )            //       if tgt has violated horizontal bounds: 
                  {
                     if( xCoord > rectR ) u16Over = xCoord - rectR;     //          compute positive distance by which 
                     else                 u16Over = rectL - xCoord;     //          dot has moved beyond border
                     u16Over = u16Over % rectW;                         //          in case distance > window width!

                     if( hv > 0 )  xCoord = rectL + u16Over;            //          if dots moving right wrt window, 
                     else          xCoord = rectR - u16Over;            //          offset from left edge, else right

                     yCoord = (UINT16) GetRandNum();                    //          and randomize the vertical coord
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (yCoord > rectU) || (yCoord < rectD) )       //       else if tgt violated vertical bounds: 
                  {
                     if( yCoord > rectU ) u16Over = yCoord - rectU;     //          dist dot moved beyond border...
                     else                 u16Over = rectD - yCoord;
                     u16Over = u16Over % rectH;

                     if( vv > 0 )  yCoord = rectD + u16Over;            //          if dots moving up wrt window, 
                     else          yCoord = rectU - u16Over;            //          offset from bottom edge, else top 

                     xCoord = (UINT16) GetRandNum();                    //          and randomize the horizontal coord 
                     xCoord = rectL + (xCoord % rectW);
                  }
                  *a = xCoord;
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++; 
                  b++;
               } 
            } 
            else if( u16Type == OPTCOHERENT )                           // OPTCOHERENT: Like OPTRECTWIN, but implements 
            {                                                           // percent coherence... 
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...
               u16Dummy = parameters->wOuterL[d];                       //    percent coherence in [0..100]

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               while ( a < de )                                         //    for each dot in target:
               {
                  u16tmp = ((UINT16) GetRandNum()) % 100;               //       if random choice >= %coherence, 
                  if( u16tmp >= u16Dummy )                              //       then randomly reposition dot
                  {
                     xCoord = (UINT16) GetRandNum();
                     yCoord = (UINT16) GetRandNum();
                     xCoord = rectL + (xCoord % rectW);
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else                                                  //       OTHERWISE, move coherently (same 
                  {                                                     //       algorithm as for OPTRECTWIN!):
                     // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement
                     // is window displacement + pattern displacement!
                     xCoord = *a + hw + hv; 
                     yCoord = *b + vw + vv;
                     if( (xCoord > rectR) || (xCoord < rectL) ) 
                     {
                        if( xCoord > rectR ) u16Over = xCoord - rectR; 
                        else                 u16Over = rectL - xCoord; 
                        u16Over = u16Over % rectW; 

                        if( hv > 0 )  xCoord = rectL + u16Over; 
                        else          xCoord = rectR - u16Over; 

                        yCoord = (UINT16) GetRandNum(); 
                        yCoord = rectD + (yCoord % rectH);
                     }
                     else if( (yCoord > rectU) || (yCoord < rectD) ) 
                     {
                        if( yCoord > rectU ) u16Over = yCoord - rectU; 
                        else                 u16Over = rectD - yCoord;
                        u16Over = u16Over % rectH;

                        if( vv > 0 )  yCoord = rectD + u16Over; 
                        else          yCoord = rectU - u16Over; 

                        xCoord = (UINT16) GetRandNum(); 
                        xCoord = rectL + (xCoord % rectW);
                     }
                  }

                  *a = xCoord;
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++; 
                  b++;
               } 
            } 
            else if( u16Type == DOTLIFEWIN )                            // DOTLIFEWIN: Similar to OPTRECTWIN, but dots 
            {                                                           // have a limited lifetime...
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               u16Dummy = (UINT16) data[cd + NREPS];                    //    extract dot life decrement from upper 
               u16Dummy = u16Dummy >> 8;                                //    byte of NREPS field in motion update rec 
               u16Dummy &= 0x000000FF;
               nDotLifeDecr = (INT16) u16Dummy;

               nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot life, restricted to [1..32767]
               if( nMaxDotLife < 1 ) nMaxDotLife = 1;
               else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767; 

               while( a < de )                                          //    for each dot in target:
               {
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + pattern displacement!
                  xCoord = *a + hw + hv;                                //       update its position
                  yCoord = *b + vw + vv;
                  *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update its current lifetime

                  if( *nextDotLife < 0 )                                //       if dot's lifetime has expired, then 
                  {                                                     //       randomly repos dot in tgt window...
                     *nextDotLife = nMaxDotLife;
                     xCoord = (UINT16) GetRandNum();
                     yCoord = (UINT16) GetRandNum();
                     xCoord = rectL + (xCoord % rectW);
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (xCoord > rectR) || (xCoord < rectL) )       //       otherwise, behaves like OPTRECTWIN... 
                  {                                                     //       (see detailed comments above)
                     if( xCoord > rectR ) u16Over = xCoord - rectR; 
                     else                 u16Over = rectL - xCoord; 
                     u16Over = u16Over % rectW; 

                     if( hv > 0 )  xCoord = rectL + u16Over; 
                     else          xCoord = rectR - u16Over; 

                     yCoord = (UINT16) GetRandNum(); 
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (yCoord > rectU) || (yCoord < rectD) ) 
                  {
                     if( yCoord > rectU ) u16Over = yCoord - rectU; 
                     else                 u16Over = rectD - yCoord;
                     u16Over = u16Over % rectH;

                     if( vv > 0 )  yCoord = rectD + u16Over; 
                     else          yCoord = rectU - u16Over; 

                     xCoord = (UINT16) GetRandNum(); 
                     xCoord = rectL + (xCoord % rectW);
                  }
                  *a = xCoord;
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++;                                                  //       move on to next dot
                  b++;
                  nextDotLife++;
               } 
            } 
            else if( u16Type == DL_NOISEDIR )                           // DL_NOISEDIR: Similar to DOTLIFEWIN, but dir    
            {                                                           // of each dot is randomly offset from pat dir.  
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               u16Dummy = (UINT16) data[cd + NREPS];                    //    extract dot life decrement from upper 
               u16Dummy = u16Dummy >> 8;                                //    byte of NREPS field in motion update rec 
               u16Dummy &= 0x000000FF;
               nDotLifeDecr = (INT16) u16Dummy;

               nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot life, restricted to [1..32767]
               if( nMaxDotLife < 1 ) nMaxDotLife = 1;
               else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767; 

               if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimited dot life if max life=32767!

               u16tmp = (UINT16) parameters->wOuterL[d];                //    dir noise offset range, N, in whole deg
               u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of integer choices in [-N:N]

               if( nNoiseUpdTicks[d] <= 0 )                             //    if noise update intv expired, choose 
               {                                                        //    new random offset directions for each dot
                  j = nVisDotsPerTgt[d];
                  for( k=0; k<j; k++ )
                  {
                     i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose random offset dir in [-N:N] 
                     i32val -= (INT32) u16tmp;                          //       NOTE USE OF DEDICATED RAND# GENERATOR 
                     i32val *= 10;                                      //       offset dir in deg/10
                     *(nextDotNoise+k) = (INT16) i32val;                //       save new offset dir for every dot
                  }

                  nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload noise update intv timer
               }

               nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decrement noise update intv timer

               // STRATEGY: hv = Rmm*2^Q, where Q=16 for Rmm < 0.1, or Q=10 otherwise. If THETA>=10000, then Q=10. For 
               // the practical range of display geometries and pattern velocities, we can expect that Rmm < 2^5, so 
               // hv < 2^21 worst-case. Since the trig lookup tables are pre-scaled by 2^10, we have:
               //   Xmm(scaled) = Rmm*2^Q*2^10*cos(TH) = Rmm*cos(TH)*2^(Q+10) = Xmm*2^(Q+10)
               //   Xpix(scaled)= Xmm*(2^16/screenW_mm)*2^(Q-6) = Xpix*2^(Q-6) = Xpix*2^(P), P=4 or 10.
               // When P=10, we divide by 2^6 so that we leave in a scale factor of 2^4. We then add in the fractional
               // pixel displacement from the previous frame update, also scaled by 2^4. We then save the fractional
               // pixel displacement for the next update and get the integer pixel displacement for this frame, Xpix.
               // Analogously, for Ypix.
               i16Scale = 6;
               if( vv >= 10000 )
               {
                  vv -= 10000;
                  i16Scale = 0;
               }

               while( a < de )                                          //    for each dot in target:
               {
                  *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update dot's current lifetime; if it 
                  if( *nextDotLife < 0 )                                //       has expired, reset it and randomly 
                  {                                                     //       reposition dot in tgt window BEFORE 
                     *nextDotLife = nMaxDotLife;                        //       MOVING IT!
                     xCoord = (UINT16) GetRandNum();
                     yCoord = (UINT16) GetRandNum();
                     *a = rectL + (xCoord % rectW);
                     *b = rectD + (yCoord % rectH);
                  }

                  i32val = (INT32) *nextDotNoise;                       //       get noise offset dir for this dot 
                  i32val += (INT32) vv;                                 //       dot theta = offset + pattern theta
                  if( i32val < 0 ) i32val += 3600;                      //       ensure dir lies in [0..3600) deg/10 
                  else i32val = i32val % 3600;
                  i16Theta = (INT16) i32val;

                  i32val = (INT32) hv;                                  //       Rmm*2^Q, Q=10 or 16
                  i32val *= (INT32) cosLUT[i16Theta];                   //       (Rmm*cos(theta)) * 2^(Q+10)
                  i32val /= screenW_mm;                                 //       Xmm*2^(4+K)*(2^16/screenW_mm) 
                                                                        //       = Xpix*2^(4+K), K=0 or 6
                  i32val >>= i16Scale;                                  //       Xpix*2^4
                  i32val += *nextFracDX;                                //       add in fracDX*2^4 from last frame
                  y32 = i32val;
                  i32val >>= 4;                                         //       xPix for this frame
                  xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld + hWin + Xpix
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + per-dot displacement!

                  i32val <<= 4;                                         //       save fracDX*2^4 for next frame
                  *nextFracDX = (INT16) (y32 - i32val); 

                  // analogously for y-coordinate...
                  i32val = (INT32) hv; 
                  i32val *= (INT32) sinLUT[i16Theta];
                  i32val /= screenH_mm;
                  i32val >>= i16Scale; 
                  i32val += *nextFracDY;
                  y32 = i32val;
                  i32val >>= 4;
                  yCoord = *b + vw + ((INT16) i32val); 
                  i32val <<= 4;
                  *nextFracDY = (INT16) (y32 - i32val);

                  if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot is now outside tgt window, 
                  {                                                     //       wrap it around as in the OPTRECTWIN 
                     if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...
                     else                 u16Over = rectL - xCoord; 
                     u16Over = u16Over % rectW; 

                     if( (xCoord - *a) > hw )                           //       (each dot is displaced differently 
                        xCoord = rectL + u16Over;                       //       every frame in this target!)
                     else xCoord = rectR - u16Over; 

                     yCoord = (UINT16) GetRandNum(); 
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (yCoord > rectU) || (yCoord < rectD) ) 
                  {
                     if( yCoord > rectU ) u16Over = yCoord - rectU; 
                     else                 u16Over = rectD - yCoord;
                     u16Over = u16Over % rectH;

                     if( (yCoord - *b) > vw )                           //       (each dot is displaced differently 
                        yCoord = rectD + u16Over;                       //       every frame in this target!)
                     else yCoord = rectU - u16Over; 

                     xCoord = (UINT16) GetRandNum(); 
                     xCoord = rectL + (xCoord % rectW);
                  }

                  *a = xCoord;                                          //       remember the new dot location!
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++;                                                  //       move on to next dot
                  b++;
                  nextDotLife++;
                  nextDotNoise++;
                  nextFracDX++;
                  nextFracDY++;
               } 
            } 
            else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISESPEED #1: Like DL_NOISEDIR, but 
                    (parameters->wOuterB[d] == 0))                      // Rdot = Rpat + U*Rpat/100, where U is chosen 
            {                                                           // randomly from [-N..N].
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               u16Dummy = (UINT16) data[cd + NREPS];                    //    extract dot life decrement from upper 
               u16Dummy = u16Dummy >> 8;                                //    byte of NREPS field in motion update rec 
               u16Dummy &= 0x000000FF;
               nDotLifeDecr = (INT16) u16Dummy;

               nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot life, restricted to [1..32767]
               if( nMaxDotLife < 1 ) nMaxDotLife = 1;
               else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767; 

               if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimited dot life if max life=32767!

               u16tmp = (UINT16) parameters->wOuterL[d];                //    speed noise offset range, N, as %-age of 
                                                                        //    nominal speed, in 1% increments
               u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of integer choices in [-N:N]

               if( nNoiseUpdTicks[d] <= 0 )                             //    if noise update intv expired, choose 
               {                                                        //    new random offset speed %s for each dot
                  j = nVisDotsPerTgt[d];
                  for( k=0; k<j; k++ )
                  {
                     i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose random offset speed % in [-N:N] 
                     i32val -= (INT32) u16tmp;                          //       NOTE USE OF DEDICATED RAND# GENERATOR 
                     *(nextDotNoise+k) = (INT16) i32val; 
                  }

                  nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload noise update intv timer
               }

               nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decrement noise update intv timer

               // STRATEGY: hv = Rmm*2^Q, where Q=16 for Rmm < 0.1, or Q=10 otherwise. If THETA>=10000, then Q=10. For 
               // the practical range of display geometries and pattern velocities, we can expect that Rmm < 2^5, so 
               // hv < 2^21 worst-case. Since the trig lookup tables are pre-scaled by 2^10, we have:
               //   Rmm*2^Q + N*Rmm*2^Q/100 = (Rmm + N*Rmm/100)*2^Q = Rmm(dot) * 2^Q
               //   Xmm(scaled) = Rmm(dot)*2^Q*2^10*cos(TH) = Rmm(dot)*cos(TH)*2^(Q+10) = Xmm*2^(Q+10)
               //   Xpix(scaled)= Xmm*(2^16/screenW_mm)*2^(Q-6) = Xpix*2^(Q-6) = Xpix*2^(P), P=4 or 10.
               // When P=10, we divide by 2^6 so that we leave in a scale factor of 2^4. We then add in the fractional
               // pixel displacement from the previous frame update, also scaled by 2^4. We then save the fractional
               // pixel displacement for the next update and get the integer pixel displacement for this frame, Xpix.
               // Analogously, for Ypix.
               i16Scale = 6; 
               if( vv >= 10000 ) 
               { 
                  vv -= 10000;
                  i16Scale = 0;
               }

               while( a < de )                                          //    for each dot in target:
               {
                  *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update dot's current lifetime; if it 
                  if( *nextDotLife < 0 )                                //       has expired, reset it and randomly 
                  {                                                     //       reposition dot in tgt window BEFORE 
                     *nextDotLife = nMaxDotLife;                        //       MOVING IT!
                     xCoord = (UINT16) GetRandNum();
                     yCoord = (UINT16) GetRandNum();
                     *a = rectL + (xCoord % rectW);
                     *b = rectD + (yCoord % rectH);
                  }

                  i32val = (INT32) *nextDotNoise;                       //       get offset speed %age N for this dot 
                  i32val *= (INT32) hv;                                 //       compute dot R=2^Q*(patR + N*patR/100).
                  i32val /= 100;
                  i32val += (INT32) hv;
                  x32 = i32val;                                         //       save cuz we're going to change i32val

                  i32val *= (INT32) cosLUT[vv];                         //       (Rmm*cos(theta)) * 2^(Q+10)
                  i32val /= screenW_mm;                                 //       Xmm*2^(4+K)*(2^16/screenW_mm) 
                                                                        //       = Xpix*2^(4+K), K=0 or 6
                  i32val >>= i16Scale;                                  //       Xpix*2^4
                  i32val += *nextFracDX;                                //       add in fracDX*2^4 from last frame
                  y32 = i32val;
                  i32val >>= 4;                                         //       xPix for this frame
                  xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld + hWin + Xpix
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + per-dot displacement!
                  
                  i32val <<= 4;                                         //       save fracDX*2^4 for next frame
                  *nextFracDX = (INT16) (y32 - i32val);

                  // analogously for y-coordinate...
                  i32val = x32;
                  i32val *= (INT32) sinLUT[vv];
                  i32val /= screenH_mm;
                  i32val >>= i16Scale; 
                  i32val += *nextFracDY;
                  y32 = i32val;
                  i32val >>= 4;
                  yCoord = *b + vw + ((INT16) i32val); 
                  i32val <<= 4;
                  *nextFracDY = (INT16) (y32 - i32val);

                  if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot is now outside tgt window, 
                  {                                                     //       wrap it around as in the OPTRECTWIN 
                     if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...
                     else                 u16Over = rectL - xCoord; 
                     u16Over = u16Over % rectW; 

                     if( (xCoord - *a) > hw )                           //       (each dot is displaced differently 
                        xCoord = rectL + u16Over;                       //       every frame in this target!)
                     else xCoord = rectR - u16Over; 

                     yCoord = (UINT16) GetRandNum(); 
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (yCoord > rectU) || (yCoord < rectD) ) 
                  {
                     if( yCoord > rectU ) u16Over = yCoord - rectU; 
                     else                 u16Over = rectD - yCoord;
                     u16Over = u16Over % rectH;

                     if( (yCoord - *b) > vw )                           //       (each dot is displaced differently 
                        yCoord = rectD + u16Over;                       //       every frame in this target!)
                     else yCoord = rectU - u16Over; 

                     xCoord = (UINT16) GetRandNum(); 
                     xCoord = rectL + (xCoord % rectW);
                  }

                  *a = xCoord;                                          //       remember the new dot location!
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++;                                                  //       move on to next dot
                  b++;
                  nextDotLife++;
                  nextDotNoise++;
                  nextFracDX++;
                  nextFracDY++;
               } 
            } 
            else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISESPEED #2: Like DL_NOISESPEED #1, but 
                    (parameters->wOuterB[d] != 0))                      // Rdot = (Rpat*2^U)/E(2^U), where U is chosen 
            {                                                           // randomly from [-N..N]...
               rectW = hsize[d];                                        //    so we don't do repeat array accesses in 
               rectH = vsize[d];                                        //    the draw loop below...

               if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off target if target rect is invalid 
               {                                                        //    due to a screen wrap-around
                  nVisDotsPerTgt[d] = 0;
                  a = de;
               }
               else                                                     //    otherwise, all dots are drawn, since all 
                  nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restricted to the tgt window.

               u16Dummy = (UINT16) data[cd + NREPS];                    //    extract dot life decrement from upper 
               u16Dummy = u16Dummy >> 8;                                //    byte of NREPS field in motion update rec 
               u16Dummy &= 0x000000FF;
               nDotLifeDecr = (INT16) u16Dummy;

               nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot life, restricted to [1..32767]
               if( nMaxDotLife < 1 ) nMaxDotLife = 1;
               else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767; 

               if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimited dot life if max life=32767!

               m = (UINT32) parameters->wOuterL[d];                     //    N = max speed noise exp, in [1..7]
               u16tmp = (UINT16) (m * 20);                              //    20N
               u16Dummy = (UINT16) u16tmp * 2 + 1;                      //    # choices in [-20N:20N]

               if( nNoiseUpdTicks[d] <= 0 )                             //    if noise update intv expired, get new 
               {                                                        //    new random index into the pow2LUT array
                  j = nVisDotsPerTgt[d];                                //    for each dot.
                  for( k=0; k<j; k++ )
                  {
                     i32val = (INT32) (GetRandNum2() % u16Dummy);       //    [0..40N]
                     i32val += (INT32) (140 - u16tmp);                  //    140 + [-20N..20N]
                     *(nextDotNoise+k) = (INT16) i32val; 
                  }

                  nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload noise update intv timer
               }

               nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decrement noise update intv timer

               // STRATEGY: Analogous to the additive speed noise case above, except that we implement the 
               // multiplicative speed noise algorithm here. 
               i16Scale = 6;
               if( vv >= 10000 ) 
               {
                  vv -= 10000;
                  i16Scale = 0;
               }

               while( a < de )                                          //    for each dot in target:
               {
                  *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update dot's current lifetime; if it 
                  if( *nextDotLife < 0 )                                //       has expired, reset it and randomly 
                  {                                                     //       reposition dot in tgt window BEFORE 
                     *nextDotLife = nMaxDotLife;                        //       MOVING IT!
                     xCoord = (UINT16) GetRandNum();
                     yCoord = (UINT16) GetRandNum();
                     *a = rectL + (xCoord % rectW);
                     *b = rectD + (yCoord % rectH);
                  }

                  i32val = pow2LUT[ *nextDotNoise ];                    //       R = 2^(x+20), x in [-N..N], N=[1..7]
                  i32val /= speedNoiseAdj[m-1];                         //       R = 2^20 * 2^x / (E(2^x) * 2^10) 
                  i32val *= hv;                                         //       R = Rpat_mm*2^Q * 2^10 * 2^x / E(2^x)
                  i32val >>= 10;                                        //       R = Rdot_mm*2^Q, Q=10 or 16
                  x32 = i32val;                                         //       save cuz we're going to change i32val

                  i32val *= (INT32) cosLUT[vv];                         //       Rdot_mm*cos(theta) * 2^(Q+10)
                  i32val /= screenW_mm;                                 //       Xmm*2^(4+K)*(2^16/screenW_mm) 
                                                                        //       = Xpix*2^(4+K), K=0 or 6
                  i32val >>= i16Scale;                                  //       Xpix*2^4
                  i32val += *nextFracDX;                                //       add in fracDX*2^4 from last frame
                  y32 = i32val;
                  i32val >>= 4;                                         //       xPix for this frame
                  xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld + hWin + Xpix
                  // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target window, so dot displacement is
                  // window displacement + per-dot displacement!
                  
                  i32val <<= 4;                                         //       save fracDX*2^4 for next frame
                  *nextFracDX = (INT16) (y32 - i32val);

                  // analogously for y-coordinate...
                  i32val = x32;
                  i32val *= (INT32) sinLUT[vv];
                  i32val /= screenH_mm;
                  i32val >>= i16Scale; 
                  i32val += *nextFracDY;
                  y32 = i32val;
                  i32val >>= 4;
                  yCoord = *b + vw + ((INT16) i32val); 
                  i32val <<= 4;
                  *nextFracDY = (INT16) (y32 - i32val);

                  if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot is now outside tgt window, 
                  {                                                     //       wrap it around as in the OPTRECTWIN 
                     if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...
                     else                 u16Over = rectL - xCoord; 
                     u16Over = u16Over % rectW; 

                     if( (xCoord - *a) > hw )                           //       (each dot is displaced differently 
                        xCoord = rectL + u16Over;                       //       every frame in this target!)
                     else xCoord = rectR - u16Over; 

                     yCoord = (UINT16) GetRandNum(); 
                     yCoord = rectD + (yCoord % rectH);
                  }
                  else if( (yCoord > rectU) || (yCoord < rectD) ) 
                  {
                     if( yCoord > rectU ) u16Over = yCoord - rectU; 
                     else                 u16Over = rectD - yCoord;
                     u16Over = u16Over % rectH;

                     if( (yCoord - *b) > vw )                           //       (each dot is displaced differently 
                        yCoord = rectD + u16Over;                       //       every frame in this target!)
                     else yCoord = rectU - u16Over; 

                     xCoord = (UINT16) GetRandNum(); 
                     xCoord = rectL + (xCoord % rectW);
                  }

                  *a = xCoord;                                          //       remember the new dot location!
                  *b = yCoord;

                  xyvals = ((*a << 16) | *b);                           //       draw the dot 
                  while( *stataddr & 0x1 );
                  *locaddr = xyvals;   
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //       save packed coords in visi dots array 
                  a++;                                                  //       move on to next dot
                  b++;
                  nextDotLife++;
                  nextDotNoise++;
                  nextFracDX++;
                  nextFracDY++;
               } 
            } 
            else if( u16Type == OPTICFLOW )                             // OPTICFLOW:  Dot speed varies w/ radial pos, 
            {                                                           // calc'd every frame.  See file header for 
                                                                        // explanation of implementation...
               nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    every dot in target is visible 

               if( hv < 0 )                                             //    FOR DECELERATING FLOWS:
               { 
                  rectH = (UINT16) (-hv);                               //    this factor in the recycle rate incr with 
                  rectH >>= vv;                                         //    B ~ deltaT * flow vel / 3.  the scaling 
                  rectH /= 3;                                           //    by 1/3 was determined heuristically.
                  if( rectH < 1 ) rectH = 1;                            //    rate limited to [1..400] parts per 1000.
                  if( rectH > 400 ) rectH = 400;

                  i32val = hv * sincosLUT[ rectL ];                     //    change in radial pos at outer edge; repos 
                  i32val >>= (10 + vv);                                 //    dots in the band between outer radius and 
                  i32val += (INT32) rectL;                              //    r = rOuter - radial chng at outer edge...
                  u16tmp = (UINT16) i32val;

                  while( a < de ) 
                  {
                     i32val = hv * sincosLUT[ *a ];                     //    dr*2^(10+M)= [B*2^M]*[sin(r)*cos(r)*2^10]
                     i32val >>= (2 + vv);                               //    dr*2^(10+M) --> dr*2^8
                     i32val += (INT32) *nextDotLife;                    //    accum fractional pos chng (deg/100/2^8)  
                                                                        //    from last update -- NOTE usage of the 
                                                                        //    "dotlife" array for this purpose!

                     *nextDotLife = 0xFF00 | (0x00FF & i32val);         //    carry over frac pos chng for next update
                     i32val >>= 8;                                      //    dr*2^8 --> dr 
                     ++i32val;                                          //    -1 maps to 0 for neg flows
                     i32val += (INT32) *a;                              //    r' = r + dr
                     *a = (UINT16) i32val;

                     u16Dummy = ((UINT16) GetRandNum()) % 1000;         //    algorithm for choosing dots to recycle:
                     rectW = rectR +                                    //       1) dot has entered hole at FOE, or 
                        (((UINT16) GetRandNum()) % (rectL-rectR));      //       2) is randomly selected for recycle.
                     if( (i32val < (INT32) rectR) ||                    //    if chosen for recycle, randomly choose 
                         ((u16Dummy<rectH) && (i32val<(INT32)rectW))    //    polar coords (r,theta) so that dot is 
                        )                                               //    repos in band near outer edge of field
                     {
                        *a = u16tmp + (((UINT16) GetRandNum()) % (rectL-u16tmp)); 
                        *b = (((UINT16) GetRandNum()) % 3600); 
                     }

                     hw = tanLUT[ *a ];                                 //    convert new polar coords to (x,y) pix: 
                     i32val = rectU * hw;                               //    r*2^20= [alphaX*2^10] * [tan(rDeg)*2^10] 
                     i32val >>= 10;                                     //    r*2^20 --> r*2^10
                     i32val *= cosLUT[ *b ];                            //    x*2^20= [r*2^10] * [cos(theta) * 2^10]
                     i32val >>= 4;                                      //    x(pix) = [x*2^20]/16 = x*65536
                     i32val += xCoord;                                  //    offset by FOE's x-coord
                     xyvals = ((0x0000FFFF & i32val) << 16);            //    pack x-coord for download to dotter brd; 
                                                                        //    "wraps" dots that exceed [0..65535]!

                     i32val = rectD * hw;                               //    analogously for the y-coord, except we 
                     i32val >>= 10;                                     //    use the alphaY conversion factor, and 
                     i32val *= sinLUT[ *b ];                            //    sin(theta)...
                     i32val >>= 4; 
                     i32val += yCoord;                                  //    ... and we offset by FOE's y-coord
                     xyvals |= (0x0000FFFF & i32val); 

                     while( *stataddr & 0x1 );                          //    finally:  draw the dot!
                     *locaddr = xyvals;	
                     visibleDotsXY[nTotalVisDots++] = xyvals;           //    save packed coords in visi dots array 

                     ++a;                                               //    move on to next dot
                     ++b;
                     ++nextDotLife;
                  }
               }
               else while( a < de )                                     //    FOR ACCELERATING FLOWS (simpler):
               {
                  i32val = hv * sincosLUT[ *a ];                        //    dr*2^(10+M)= [B*2^M]*[sin(r)*cos(r)*2^10] 
                  i32val >>= (2 + vv);                                  //    dr*2^(10+M) --> dr*2^8
                  i32val += (INT32) *nextDotLife;                       //    accum fractional pos change from last upd 

                  *nextDotLife = 0x00FF & i32val;                       //    carry over frac pos chng for next update
                  i32val >>= 8;                                         //    dr*2^8 --> dr 
                  i32val += (INT32) *a;                                 //    r' = r + dr
                  *a = (UINT16) i32val;                                 //    update new radial pos
                  if( i32val > (INT32) rectL )                          //    randomly repos dots that pass outer edge
                  {
                     *a = rectR + 
                        (((UINT16) GetRandNum()) % (rectL-rectR)); 
                     *b = (((UINT16) GetRandNum()) % 3600); 
                  }

                  hw = tanLUT[ *a ];                                    //    convert new polar coords to (x,y) pix: 
                  i32val = rectU * hw;                                  //    r*2^20= [alphaX*2^10] * [tan(rDeg)*2^10] 
                  i32val >>= 10;                                        //    r*2^20 --> r*2^10
                  i32val *= cosLUT[ *b ];                               //    x*2^20= [r*2^10] * [cos(theta) * 2^10]
                  i32val >>= 4;                                         //    x(pix) = [x*2^20]/16 = x*65536
                  i32val += xCoord;                                     //    offset by FOE's x-coord
                  xyvals = ((0x0000FFFF & i32val) << 16);               //    pack x-coord for download to dotter brd

                  i32val = rectD * hw;                                  //    analogously for y-coord, except we use 
                  i32val >>= 10;                                        //    alphaY and sin(theta)...
                  i32val *= sinLUT[ *b ]; 
                  i32val >>= 4; 
                  i32val += yCoord;                                     //    ... and we offset by FOE's y-coord
                  xyvals |= (0x0000FFFF & i32val); 

                  while( *stataddr & 0x1 );                             //    finally:  draw the dot!
                  *locaddr = xyvals;	
                  visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed coords in visi dots array

                  ++a;                                                  //    move on to next dot
                  ++b;
                  ++nextDotLife;
               }
            } 


            u16Dummy = (UINT16) data[cd + NREPS];                       // decrement #reps for this tgt
            if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // (be sure to mask out dot life decr in reps 
                u16Type == DL_NOISESPEED )                              // field for the "limited dot life" tgt types)
               u16Dummy &= 0x000000FF; 
            nRedrawsLeft[d] = u16Dummy;
            nRedrawsLeft[d]--;
         }                                                              // END:  first pass thru all targets
         
         if( maxRepeats > 0 ) for( i = 0; i < maxRepeats-1; i++ )       // now complete remaining reps for all tgts 
         {                                                              // by using the visible dots array we prepared 
            nTotalVisDots = 0;                                          // in the first pass!!!  This implementation 
            for( d = 0; d < parameters->wNumTargets; d++ )              // allows for different NREPS values for 
            {                                                           // different tgts.  Note that its is important 
               if( nRedrawsLeft[d] > 0 )                                // to cycle thru the targets rather than 
               {                                                        // redrawing tgt1 N times, tgt2 M times, etc. 
                  k = nTotalVisDots + nVisDotsPerTgt[d];                // the former approach move evenly distributes 
                  for( j = nTotalVisDots; j < k; j++ )                  // individual dot refreshes over update period.  
                  {
                     while( *stataddr & 0x1 );
                     *locaddr = visibleDotsXY[j];	
                  }
                  nRedrawsLeft[d]--;
               }
               nTotalVisDots += nVisDotsPerTgt[d];
            }
         }
            
      }                                                                 // END:  process XYCORE_DOFRAME command...
         

      status = XYCORE_READY;                                            // write XYCORE_READY into CmdStat register to 
      #if defined(_TGTDAKARF5)                                          // inform host XYAPI we're ready for next cmd. 
         C4X_Control( PCI, SET_MAILBOX, 2, &status );
      #elif defined(_TGTDETROIT)
         C6x_WritePlx( MAILBOX2_OFFSET, status );
      #else    // _TGTDAYTONA
         *((UINT32 *)0x00400000) = status;
      #endif

   } while( command != XYCORE_CLOSE );                                  // END runtime loop

}                                                                       // END main()

