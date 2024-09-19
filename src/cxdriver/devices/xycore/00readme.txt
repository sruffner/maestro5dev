//=====================================================================================================================
//
// 00readme.txt --   Development history for XYCORE -- the DSP board-resident program that supports the XY scope target
//                   display platform in conjunction with a CXDRIVER host-side device object derived from the abstract
//                   CCxScope interface.
//
// Author:  saruffner.
//
// XYCORE is the XY scope control program that runs on one of three possible Spectrum Signal Processing DSP boards.
// It works in conjunction with the a CXDRIVER device-specific object on the host side  to display point-like and
// random-dot texture targets on a large XY oscilloscope.  This directory contains the source code files for XYCORE, as
// well as individual build environments for each version of the program:
//       XYDAKAR.OUT   ==> XYCORE for the embedded C44 processor on the Dakar F5 Carrier DSP.
//       XYDETROIT.OUT ==> XYCORE for the Detroit C6x DSP board.
//
//=====================================================================================================================


//=== NOTES ===========================================================================================================
//
==> The build environment.
      xycore.c       Main source code file used by all builds.  Tailored for each board by defined constants and
                     conditional build using #IFDEF's.  Includes constants & typedefs which are shared by XYCORE and
                     the host-side device object.

      mkxycore.bat   Run this batch file to rebuild all XYCORE versions.
      xy******.out   The most recent XYCORE builds.

      vDDMMMYYYY/    Recent previous versions of source code.

      dakar/         Build env for XYDAKAR.OUT, supporting Dakar F5 board.  See dakar/mkdakar.bat for details.
      detroit/       Build env for XYDETROIT.OUT, supporting Detroit C6x board.  See detroit/mkdetroit.bat for details.

==> I'm maintaining the build environment on a UNIX system because UNIX stations are regularly backed up (the PCs
    aren't).  However, the build must be performed on a PC, using the appropriate development system:  TI's C6x or
    C3x/C4x dev tools.
==> Each build directory contains all the additional files required to build the XYCORE for the target DSP board.  By
    keeping these files on the UNIX side, we keep the build environment backed up.  Keep in mind that the build tools
    run in an MS-DOS environment...
==> Rather than struggle with a MAKE utility and the unfathomable format of the associated make file, I've opted to
    use a batch file that invokes the appropriate build tools.  This is feasible since the program is small & a full
    rebuild does not take long.  To rebuild each executable, open an MS-DOS window under WinNT.  Set the current
    directory to the build directory for the executable you wish to make.  Then run the batch file in that directory.
    Alternatively, set the current directory to this directory and run mkxycore.bat, which automatically rebuilds all
    three XYCOREs.
==> [10jan2006] IMPORTANT.  With recent changes to XYCORE, the code became large enough that we ran into relocation
    problems during linking of XYDETROIT.OUT and XYDAYTONA.OUT.  The problem was in an assembly command in
    detroit/isfp6201.c and daytona/isfp6201.c.  IAW the TMS320C6000 Programmer's guide, I replaced the MVK mnemonic
    with MVKL to fix the problem.

//=== REVISION HISTORY ================================================================================================
//
26jan1999-- Added special target OPTMSQTEXD.  Unlike the other *TEXD target types -- for which the specified number
            of dots (PARAMETERS member "ndots") are drawn over the entire XY scope display --, the OPTMSQTEXD target
            type is designed to have fewer dots, all of which are drawn within the bounding rectangle on every
            display update.  In all other respects it is designed to behave like the MSQTEXD target.  However, the
            current implementation of this target type on the DSP board only works if the net displacement on a
            given update (window - dot) is not too large -- see 60dots.c, the source code for 60dots.out).
27jan1999-- Changed implementation of the target update data field REPEAT.  Previously, the program would draw the
            entire set of target dots (#repeats) times, updating the dot position only on the first iteration.  In
            the new implementation, the program first changes the trigger duration to BASE_TRIGDUR * (#repeats),
            then draws the dots once.  Each dot will be approximately (#repeats)-times as bright than if the trigger
            duration was just BASE_TRIGDUR.  This will increase the efficiency of windowed targets, since the
            conditional expressions which determine a given dot's visibility will be evaluated only once per dot,
            rather than (#repeats) times per dot.  See DOTS.H for definitions of these constants.  Note that the
            REPEAT value should be chosen so that BASE_TRIGDUR * REPEAT <= MAX_TRIGLEN.
01feb1999-- Slight mod to change dtd 27jan1999.  Since the trigger duration parameter that is written to the dotter
            board's timing register actually represents the sum of the delay and the beam "ON" time, this duration
            byte in now calculated using: trigDur = trigDelay + (#repeats) * BASE_TRIGDUR.  Clarified relevant
            comments in DOTS.H.
01feb1999-- Made necessary changes to implement target window boundaries as unsigned 2-byte integers rather than
            4-byte signed integers.  As a consequence, these boundaries always fall within the range of the screen
            bounds, [0..65535] -- hence, target windows will "wrap around" the screen edges just as the random-dot
            textures do.  When window starts to wrap around, it will disappear because the rectangle definition will
            become invalid (left edge < right edge or top edge < bottom edge); it will not reappear until the
            wrap-around is complete.  Such wrap-arounds should not occur, but that is the user's responsibility for
            now.  NOTE:  Only had to change defn of PARAMETERS structure in DOTS.H.
19feb1999-- Discovered problem with how we generate pseudorandom number sequence.  Repeated calls to rand_uint()
            yielded an {odd, even, odd, even, ... } or {even, odd, even, odd, ...} sequence of 32-bit integers,
            depending on whether initial seed was even or odd, respectively.  Since we only need 16-bit random
            numbers, replaced rand_uint() with GetRandNum(), which returns the middle 16 bits of the random 32-bit
            number generated by the algorithm.
25feb1999-- Modified SANTEXD target type so that it only draws dots which lie inside the rectangular annulus defined
            by the inner & outer rects.  Code does not check for a properly defined annulus.  Added TRUANTEXD target
            type, a true rect annulus which is stationary and windows a full-screen, possibly moving dot field.
         -- Technical changes made to consistently handles dots lying on edges of defining rects.  Rects are defined
            such that the edges are considered part of the rectangle.
26feb1999-- Replaced MAXDOTNUMBER with MAXTOTALDOTS, which indicates the total number of dot positions that may be
            stored by 60dots.c.  Ensure that 2*MAXTOTALDOTS, which is the number of UINT16's required to store the
            current dot positions (x,y) of all targets, does not exceed the local memory resources of the target
            DSP node.  Added MAXDOTSPERFRAME, the maximum number of dots that may be drawn in one refresh period.
01mar1999-- Testing on 26feb1999 revealed that our new method of implementing the REPEAT value causes problems
            with apparent-motion protocols.  As the refresh rate decreases, so did the intensity of target dots
            (most noticeable going from 2ms to 4ms updates -- REPEAT = 1 to 2).  Restored the original method,
            which redraws the target dots REPEAT times during the refresh period.  Optimized it somewhat by cycling
            thru the targets and saving information about the visible dots so that, on subsequent repeats, all we
            have to do is draw dots (no checks to see if dot is visible, etc.).  Also, rather than drawing target(0)
            REPEAT(0) times, then target(1) REPEAT(1) times, the program draws each target once (if it needs to be
            drawn), then starts over & draws each target again (if need be), until each target has been redrawn the
            specified number of times.
02mar1999-- Changed PARAMETERS structure.  Added fields 'delayPerDot' and 'onTimePerDot' to facilitate specification
            of these dotter board timing parameters via the READPARAMS command, rather than having them hard-coded.
14apr1999-- Implemented new target, DOTLIFETEXD:  a special SQTEXD-style target in which each dot has a limited "dot
            life".  Whenever the dot passes outside the visible rect or its dot life expires, the dot is randomly
            repositioned within the visible rect and its dot life is reset.  When the target is init'd, each dot
            starts with a randomly selected dot life in {0, max dot life}; otherwise, many dots would expire at the
            same time.  Host program must communicate the "max dot life" (in ms) for the target w/ the READPARAMS
            command.  For each target update, the host program must also store the refresh period (in ms) in the
            unused IHORIZ location -- so changes in refresh period can be handled correctly.
         -- NOTE:  There is currently no protection against attempting to draw more than MAXDOTSPERFRAME dots during
            a given update.  This will probably never happen, as long as the constant is set high enough such that a
            frame drop (host program attempts to send a new DOFRAME when the DSP board is still busy with the last
            one) will occur before MAXDOTSPERFRAME dots can be drawn.
15apr1999-- Generalized implementation of DOTLIFETEXD so that the interpretation of "dot life" is under the control
            of the host program.  This only required cosmetic changes in variable names and comments, since the
            actual code was already set up for this.  Basically: the host program controls what the max dot life is
            and how much a dot's current lifetime decreases per update.  Thus, how dot life is actually measured is
            completely invisible to 60dots!
19apr1999-- Fixed bug in implementation of DOFRAME for DOTLIFETEXD target.
26apr1999-- Changed implementation of DOTLIFETEXD:  Now both dot texture and visible window can move.  Dots are
            still randomly repositioned whenever their dot life expires or they reach the boundary of the visible
            window.  In this case, the IHORIZ & IVERT update data are required, so the dot life decrement must be
            stored elsewhere.  To avoid adding an additional short int to the array of update data, we will ASSUME
            that both dot life decrements and REPEAT values are always 0 <= n <= 255.  In this case we can use the
            lower byte of REPEAT field to hold the actual repeat count, while the upper byte holds the dot life
            decrement.  This special use of REPEAT applies only to DOTLIFETEXD.
10aug1999-- Changed drive for <Detroit root> from "d:" to "c:" (put build environment on a new PC).
06dec1999-- Found & fixed benign bug in code that handles DOFRAME processing (had to do with masking out the
            dot life decrement in the upper byte of the REPEAT field).
04apr2000-- Major cosmetic restructuring of the build environment to support XYDSPCORE builds for target boards
            other than the Detroit C6x.  60dots.c renamed dots.c.  Comments in dots.c/h generalized to emphasize
            that the source code supports multiple boards via conditional builds.  At this point in time we still
            only support the Detroit C6x, but we're preparing to introduce mods to handle the Dakar, and later, the
            Daytona.
06apr2000-
12apr2000-- Introduced conditional compilation using the build flags _TGTDETROIT, etc. in order to tailor the
            code to each target DSP board.
         -- Also had to modify some code that implicitly relied on UINT16 arithmetic.  This is because, on the Dakar
            board, all integer data types are stored as 32-bit words (UINT16 maps to UINT32, etc.).  In particular:
            modified TARGETD implementation so that it only draws as many dots as will fit in the first quadrant of
            the screen; simulated UINT16 arithmetic where needed if the target DSP is the Dakar.
11apr2000-- Changed organization of source code and supporting files to prepare to support multiple DSP boards.  Thus
            far we plan to support 3 different DSP platforms from Spectrum Signal Processing -- the Dakar, Detroit, and
            Daytona -- all of which use Texas Instruments DSP chips .  Because the XYDSPCORE program is quite simple,
            we can *for now* stick with a single source code file, which I will now call DOTS.C for generality (was
            "60dots.c", and "30dots.c" before that...).  DOTS.H  will continue to serve as the XYAPI<->XYDSPCORE
            interface file, containing constants and typedefs which are shared with the XYSCOPE API of CNTRLX
            (regardless of the particular DSP).  All DSP board-specific include files and libraries are maintained in
            separate subdirectories:  ./DAKAR, ./DETROIT, ./DAYTONA; the build environment for each board is also
            maintained in these subdirectories.  A different COFF executable file must be built for each supported
            board:  DOTS_DAK.OUT for the Dakar; DOTS_DET.OUT for the Detroit; and DOTS_DAY.OUT for the Daytona.
12apr2000-- Instead of using a MAKE utility with a makefile to build the .OUT files, I decided to go with a simpler
            batch file approach.  (Was having trouble getting things to work with the new build environment layout!)
17apr2000-- Minor mods to make code compatible with C3x/C4x build tools.
18apr2000-- When XYDSPCORE first starts, it must be sure to initialize the "command/status register" to READY.
            After downloading & starting XYDSPCORE, the XYAPI expects the status register == READY as a simple check
            that XYDSPCORE is running.
02may2000-- Mods to implement XYDSPCORE tailored for the Daytona's Node A C6x processor.  Removed blink() fcn, which
            was left over from early days of developing XYDSPCORE for the Detroit.

/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 22may2000 /////////////////////////////////////


11sep2000-- The major mods of 04apr-02may2000 introduced a significant performance hit in XYDSPCORE(Detroit), as
            reflected by a 10-30% reduction in the max # of dots that could be run under three different "benchmark"
            tests.  This was because I allocated the dot position arrays in Detroit's local SDRAM instead of SSRAM,
            where they were allocated prior to 04apr2000.  SSRAM is about 5% faster than SDRAM in DMA tests, and
            since the dot arrays are accessed multiple times in each draw cycle, that small performance hit leads to
            longer average per-dot draw cycles... FIXED.
         -- Analysis of the assembly listing showed that XYDSPCORE was NOT using all available registers, and that
            all of the comparisons going on to implement some of the target types during DOFRAME processing could
            benefit substantially by effective use of register vars.  We tested this idea out with the OPTMOV_CTR
            target type and got a whopping 40% increase in max dots that can be displayed in the benchmark test.  We
            therefore introduced several new register vars and reclassified some existing vars as 'register', and
            modified DOFRAME processing for all target types to take advantage of these register vars.  Performance of
            non-windowed types did not improve dramatically, but all windowed target types ran significantly faster.
12sep2000-- Modification in OPTMOV_CTR implementation to eliminate "striation effect" at pattern vel >~ 64deg/sec.
19sep2000-- Changes of 11-12sep2000 tested successfully.  Rebuilt all three versions of XYDSPCORE.


/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 19sep2000 /////////////////////////////////////
/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 17jan2001 /////////////////////////////////////

21feb2001-
20mar2001-- Major modifications to introduce a new, very different target type:  a random-dot simulation of an optical
            flow field (type OPTICFLOW in XYCORE, type FLOWFIELD in CNTRLX)...
            ==>Modified algorithm for saving positions of dots determined to be visible during the current animation
            frame (for repeated redraws).  Instead of saving each dot's index into the (x,y) coord arrays, a UINT16
            number, we now save the packed form of each coord in a UINT32 array called "visibleDotsXY".  This saves
            time during the redraws, since all we have to do is move each element of visibleDotsXY[] into the dotter
            board's "Location" register.  Also, the old algorithm was NOT suited to the new OPTICFLOW target type,
            which stores (radial pos, angular pos) instead of (x,y) in the (x,y) dot position arrays...
            ==> OPTICFLOW target representation:  All dots are drawn every frame.  Dot position is stored as (r,theta)
            in the (x,y) dot position arrays, where r = radial pos of dot from the field's focus of expansion (FOE),
            expressed in deg/100 of visual angle [limited to 0..4499], and theta = angular direction of ray from FOE to
            dot location, in CCW degrees from +x-axis [limited to 0..359].  Target window is bounded by a (nonzero)
            inner radius and (usually much larger) outer radius, which are stored in PARAMETERS.centerR[] and
            .centerL[], respectively.  This very different representation is ESSENTIAL to the animation of the
            OPTICFLOW target:  unlike the other target types, individual dots must move at different speeds and
            along different directions to achieve an optical flow effect.  To convert (r,theta) to raw (x,y) in pixels,
            we assume ALWAYS that the line of sight (LOS) passes thru the FOE at a perpendicular angle (NOTE that this
            will only be the case when the FOE is at the center of the XY scope display!), so that:
                  x = alphaX * tan(r) * cos(theta), y = alphaY * tan(r) * sin(theta).
            Here, alphaX and alphaY are conversion factors that depend on the current display geometry.  They are
            communicated to XYCORE in PARAMETERS.centerU[] and PARAMETERS.centerD[], respectively.  The initial pos
            of the FOE (in pixels) is stored in surrR[] (x-coord) and surrL[] (y-coord).  During animation, XYCORE
            updates the FOE's coordinates in place, so be sure to reinitialize!
            ==> OPTICFLOW animation:  All dots move radially away from(+ive flow velocity) or toward (-ive flow) the
            FOE at a speed which changes with the radial distance of the dot from the FOE via:

                  v(r) = B * sin( r ) * cos( r ),

            where r is the radial pos in visual deg, and B is a constant determined by the speed at half the outer
            radius of the field.
            ==> Optimizing animation speed -- the problem of floating-point and trig calculations.  Every animation
            frame, XYCORE must update the radial position of every dot in the OPTICFLOW target.  The calculations
            involve a number of trig functions and a lot of floating-point multiplies.  Our DSP boards are fixed-point
            devices, which must emulate floating-point.  Hence, we decided to avoid floating-point by scaling all the
            values to preserve at least 3 digits precision whenever possible, and we avoid the trig functions by using
            function lookup tables that are specifically tailored to the animation.  Thus, since we restrict radii to
            the range [0..45], we implement tan(r) as tanLUT[R] for R = [0..4499] deg/100.  Similarly for the fcn
            sin(r)*cos(r).  We also implement sin(theta), and cos(theta) as sinLUT[THETA], cos[THETA] for THETA =
            [0..359] in deg.  LUT initialization is handled by the utility routine FillTrigLUTs(), which is called
            during system startup.  The values in all of these LUTs are scaled by 1024 = 2^10, as are the scale factors
            alphaX and alphaY; the factor B is scaled by various powers of 2 depending on its scaled value, because it
            takes on a very wide range of actual values.  Scaling by a factor of 2 is important -- we can remove the
            scale factors from a product (2^10)*A * (2^10)*B merely by a right-shift -- which is MUCH faster than
            integer division!!

            ==> Stack overflow issue:  DOTS_DET.OUT failed due to a stack overflow.  This probably occurred during
            the fcn calls that initialized the trig LUTs.  I fixed it by increasing the stack size from 0x100 to 0x400
            bytes.  This change was made in the linker command files detroit/dots_det.cmd and daytona/dots_day.cmd;
            the Dakar's linker cmd file was different and already used the default stack size of 0x400.

            ==> Remaining problems as of 3/6/2001.  The implementation appears to be working, and we can handle up
            to 750-800 dots in a single OPTICFLOW target (no other targets animated) with a deltaT of 4ms and timing
            parameters dur = 100ns, delay = 1000ns.  However, the fact that dot velocity goes to zero as the dot
            approaches the FOE causes the density of the field to change as time goes on.  This uneven-density problem
            is particularly noticeable when the flow velocity is negative -- the dots tend to collect around the FOE.
            Still working on solutions to this problem....

            ==> [20mar2001].  After a lot of trial & error, finally developed an algorithm for "recycling" dots in
            decelerating flowfields such that dot density stays relatively uniform over the field.  Got reasonable
            performance across flow velocities of -100 to 100 deg/s, with deltaT's of 4, 16, 32ms and field radii of
            9.0 and 18.0 deg -- except for a couple of high-velocity, large-deltaT trials where the pos change per
            update was so large that each dot was recycled after only a few frame.  Max dots now 675 under the
            conditions mentioned above.


11apr2001. saruffner.

Another new target type added, "ORIBAR", a rectangular bar or thin line of dots that can be oriented at any angle in
[0..360).  It is defined as follows:

      PARAMETERS.dtype[]      ==> ORIBAR
      PARAMETERS.ndots[]      ==> # of dots in target.  ALL dots are drawn on every update frame.
      PARAMETERS.centerR[]    ==> width of bar in vertical orientation (pixels)
      PARAMETERS.centerL[]    ==> height of bar in vertical orienation (pixels)
      PARAMETERS.dspace[]     ==> the drift axis angle (degrees)

The key parameter here is the drift axis angle.  During READPARAMS processing, the bar's dots are initialized so that
the bar is centered at the screen origin (32767, 32767), but rotated from the vertical orientation by the drift axis
angle.  If width and height of bar are nonzero, the target dots are randomly distributed over the bar's area.  If the
width of the bar is zero, then the specified # of dots are evenly distributed along a thin line.  A height of <2 is
treated as an empty bar; in this case the bar will NOT be drawn.

Algorithm for drawing bar at an angle:  For each dot, we first determine its (x,y) coords in a pixel-coord system with
an origin at zero.  We then apply the rotation transformation to rotate the dot's position by the drift axis angle A:

      x' = x*cosA - y*sinA;  y' = x*sinA + y*cosA.

Lastly, we translate the coords (x',y') by (32767,32767) so that the bar starts off centered at the screen origin.  We
use the sin() and cos() look-up tables that were implemented earlier for the OPTICFLOW target.  This, of course,
requires that we do the calculations using 32-bit variables.

This target type is exactly like TARGETD in its motion implementation:  it has no separate "pattern" velocity, only a
window velocity at which all dots in the bar move in unison.


/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 11apr2001 /////////////////////////////////////
/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 05jun2001 /////////////////////////////////////
/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 16oct2001 /////////////////////////////////////
/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 29jan2002 /////////////////////////////////////
/////////////////////////////// OFFICIAL CNTRLXPC INSTALLATION UPDATED, 20aug2002 /////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// CHANGES SINCE INTEGRATION WITH CNTRLX/CXDRIVER, THE ALL-PC REPLACEMENT FOR CNTRLXUNIX/PC ///////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

04oct2002-22jan2003.  saruffner.

Integrating XYCORE with the new all-PC version of CNTRLX/CXDRIVER.  On host side, the XY scope device is encapsulated
by the CCxScope abstract interface, which is implemented by a device-specific class for each SSP board (eg, CXYDetroit
for the Detroit C6x).  In the process, we removed some obsolete fields from the PARAMETERS struct, changed the names of
the target types, removed the SQTEXD & ANTEXD types, and changed the implementation of the TRUANTEXD (now called
ANNULUS) target.  The XYCORE source code file is now called XYCORE.C rather than DOTS.C.  Constants and typedefs in
DOTS.H were moved into XYCORE.C -- we no longer "share" those definitions with the CXDRIVER build, but merely duplicate
them (carefully) in XYCORE.C.  Also updated the build-related files for each of the DSP targets...


21jul2003.  saruffner.

Introduced support for a new target type, DOTLFNOISY.  This variation on the DOTLIFEWIN target randomizes the motion of
each dot in the dot pattern by rotating the per-frame pattern velocity vector by an offset direction that is randomly
chosen for each dot on every frame.  The idea here is to introduce noise in the pattern velocity "signal".  Offset
directions are chosen from the range [-N:N] in degree increments, where N (in whole deg) is specified in the wOuterL[]
field of the PARAMETERS struct.  This target is probably the most time-intensive tgt type to date, since we have to do
significant calculations for every dot on every frame!

The implementation requires sine and cosine lookup tables for every tenth-degree.  The lookup tables provided for the
OPTICFLOW and ORIBAR implementations were restricted to whole degrees.  We modified those implementations to take
advantage of the higher precision lookup tables.

28jul2003.  saruffner.

As currently implemented, the "dot life" target (DOTLIFEWIN and DOTLFNOISY) dot patterns become non-uniform as the
pattern moves in one direction, with the dots accumulating in that direction.  This is because, instead of wrapping
each dot around the rectangular boundary as in OPTRECTWIN, the dot is randomly repositioned in the target rectangle.
Modified the implementations to behave like OPTRECTWIN when a dot passes across a boundary; a dot is randomly
repositioned only when its lifetime expires.

Mod DOTLFNOISY so that, when the "max dot life" assigned to the target is 32767 (the maximum possible value), the
limited dotlife feature is disabled and dots "live forever".


12sep2003.  saruffner.

Mod in representation of polar coordinates of DOTLFNOISY target's pattern displacement vector:  R is scaled by 2^6 only
if its unscaled value is small.  When it is NOT scaled, THETA is offset by 10000 as a "signal" to XYCORE.  DOFRAME
processing of XYCORE adjusted accordingly.

19nov2003.  saruffner.

Added a second random-number generator (same algorithm) that is solely dedicated to choosing the random dot offset
directions for the DOTLFNOISY target.  With this change, it is feasible to reconstruct offline the set of dot
directions on each update frame during animation of this target.  The new random-number generator is seeded with the
same value used to seed the main random-number generator.

13oct2004.  saruffner.

Introduced support for a new target type, OPTCOHERENT.  This variation on OPTRECTWIN implements "percent coherence"
as described in the 1988 J.Neurosci paper by Newsome & Pare.  On EVERY frame update, EVERY target dot has an N% chance
of moving coherently -- ie, IAW the motion vector for that frame -- and a (100-N)% chance of being randomly
repositioned within the target window.  NOTE that this is NOT the same as saying that N% of the target dots move
coherently.

24jan2005.  saruffner.

BUG FIX.  For larger per-frame radial displacements of the DOTLFNOISY tgt (ie, R >= 200), the update record field
UPDATEREC.shPatternV contains the displacement direction THETA in deg/10 PLUS 10000; thus, it ranges from 10000..13599.
Code tested for shPatternV > 10000; it should be shPatternV >= 10000.  Corrected.

11apr2005.  saruffner.

Modified implementation of DOTLFNOISY.  Additional parameter, "direction update interval", is now stored in
PARAMETERS.wOuterT[], in milliseconds.  XYCORE now maintains each dot's current direction (in deg/10).  These
directions are updated at the beginning of each "direction update interval".  Thus, XYCORE must also maintain a
countdown timer to determine when an update interval has expired.  The algorithm for computing each dot's direction
is unchanged.

07jan2006.  saruffner.

Introduced support for a new target type, DL_NOISESPEED.  This is similar to DOTLFNOISY, which is now labelled
DL_NOISEDIR, except that the noise is in the individual dot speeds rather than their directions.  Both tgt types use
two special parameters, the "noise update intv" M in msec and the "noise offset range limit" N.  For DL_NOISESPEED,
CCxScope delivers N as an integer in [0..1000], representing a percentage of nominal speed in 0.1% increments.  Each
dot is randomly assigned a speed factor D in [-N:N], and the dot's radial speed R = Ro + D*Ro/1000, where Ro is the
nominal speed of the target pattern in pixels/frame.

23jul2006.  saruffner.

Allowed range for the DL_NOISESPEED targets's "noise offset range limit" N is now [0..3000] instead of [0..1000].
If N > 1000, some dots in the target will move in the opposite direction from the nominal target direction. Had to 
modify the code to permit this (formerly, it forced R to 0 if it turned out negative!).

16jul2007.  saruffner.

Modified implementation of DL_NOISESPEED. The granularity in the random percentage speed noise is now 1% instead of 
0.1%. The "noise offset range limit" N is now an integer in [0..300] instead of [0..3000]. Effective Maestro v2.1.2.

04sep2007.  saruffner.

Modified implementation of DL_NOISESPEED. Introduced a second, multiplicative method of noise generation. To select 
this method, PARAMETERS.wOuterB must be nonzero. In this case, the "noise range limit" N sets the range [-N..N] over 
which an exponent X is randomly and uniformly chosen. Each dot will have a different X. The dot's speed 
Ro = (R * 2^X) / E(2^X), where R is the nominal pattern speed and E(2^X) = (2^N - 2^(-N))/(2 * N * ln(2)) is the 
expected value of 2^X when X is a uniform random variable over [-N..N].

09may2011-13may2011.  saruffner.
Corrected implementation of ORIBAR target. Rotation of bar dot locations to the specified angle had been done in device
coordinates (pixels), which is invalid because the device grid is artificially square, 65536 x 65536, when the real 
display width and height usually are not equal. Now, rotation calculations are done in screen coordinates in mm, and
then the coordinates are converted to pixels as a final step. Requires screen width and height in mm. Modified the 
PARAMETERS structure to transmit display geometry to XYCORE in new member fields .wWidthMM, .wHeightMM, and .wDistMM.

Corrected implementation of DL_NOISEDIR and DL_NOISESPEED targets. Conversion to polar coordinates with amplitude in
device pixels was incorrect when aspect ratio was not 1:1 (similar to the problem with ORIBAR); now the target 
displacement vector sent to XYCORE is in polar coordinates with amplitude in screen mm. All calculations are done with
scaled integers, using trig tables to ultimately recover the pixel changes in X and Y for each dot. In addition, we
keep track of the integer truncation error (the fractional pixel change) in X and Y for each dot, and add it back in
on the next update. This truncation error became quite evident in long-running motions in one direction.

Target pattern displacement vector sent by CXDRIVER now specifies target pattern motion in the target window's frame
of reference, ie, how the pattern moves WRT the target window, NOT the XYScope screen. All affected target types
were updated accordingly.

05jul2011.  saruffner.
Removed build environment for building XYDAYTONA.OUT, the XYCORE targeted for the Daytona C6x Dual-DSP board. This
implementation was never successfully debugged. There's no point in pursuing it further, as the XYScope platform is
on its way out: we only have one working large analog XY vector display left, and we can't find any more!

