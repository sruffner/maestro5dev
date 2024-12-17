/** rmvdisplay.cpp ====================================================================================================
 Implementation of CRMVDisplay, encapsulation of the RMVideo OpenGL-based display.

 CRMVDisplay is the "engine" for RMVideo. It sets up the fullscreen window and polls for and responds to commands 
 during a command session (either emulated via file or over the network with Maestro). 

 While CRMVDisplay manages all interactions with the OpenGL XWindows client (GLX), all OpenGL rendering is encapsulated
 in CRMVRenderer. CRMVRenderer draws the fullscreen background while in idle mode, manages the list of targets that
 participate in an animation sequence, and control the runtime loop during an animation sequence.

 Additional helper objects handles other important tasks in RMVideo: CRMVIoNet implements the Maestro/RMVideo
 communication protocol (see abstract class RMVIo for a description of the protocol), CRMVIoSim implements a simulated
 Maestro command stream stored in a text file, and CRMVMediaMgr manages the RMVideo media store (for RMV_IMAGE and
 RMV_MOVIE targets).

 As of version 9, RMVideo requires synchronization with the vertical blanking interval, which eliminates the 
 so-called tearing artifact. All prior releases required that VSync be turned off and used a relatively obscure 
 GLX extension, glXGetVideoSyncSGI(), to "busy-wait" on the blanking interval before performing a buffer swap. See
 discussion below on "SyncToVBlank".

 As of version 10, RMVideo conforms to the OpenGL 3.3 Core Profile. All prior releases only required OpenGL 1.1 and 
 relied on legacy "immediate mode" functionality for all rendering. Such functions are inefficient and considered 
 long obsolete, and one day may be removed altogether from the OpenGL standard (although they're still supported in
 NVidia's Linux drivers as of 2019). For additional details, see CRMVRenderer.

 As of version 11, RMVideo will request a GL visual with stereo support, if available. This is in support of 
 stereo experiments using any of the dots targets (RMV_POINT, RMV_RANDOMDOTS, RMV_FLOWFIELD). Requires an NVidia
 card configured to provide stereo support.

 ==> HISTORY on "SyncToVBlank" and the design of the animation loop.
 1) From 2009: On my machine (Linux Mandriva 10.1, XOrg 6.7, DRI, Matrox G550), vertical sync is off by default. In 
 this case, glXSwapBuffers() swaps ASAP, not during vertical retrace -- so tearing is always present. There are various
 GLX extensions out there that are supposed to turn on vertical sync, but I couldn't get anything to work. In the end,
 to get vertical sync and eliminate the tearing artifact, I employed the following hack: render frame; call glFinish()
 to ensure all commands have been executed; call extension glXGetVideoSyncSGI() repeatedly to detect when the video 
 sync counter increments; then call glXSwapBuffers() to perform the swap. Effectively, we're waiting for the vertical
 retrace in user mode! The hack worked ONLY because I am able to run in soft real-time mode via sched_setscheduler().
 One must have super-user privileges to run that function.
 2) Using glXGetVideoSyncSGI() is critical also because we use it do detect when RMVideo takes too long to render a
 frame, resulting in a "skipped frame"; or if it takes too long to receive motion data from Maestro for the next frame,
 resulting in a "duplicate frame".
 3) From May 2016: "SyncToVBlank" is an NVidia driver setting that is used to enable or disable vertical sync. It is 
 very important that SyncToVBlank be disabled for RMVideo to work correctly, given the way it counts frames using
 glXGetVideoSyncSGI(). If SyncToVBlank is ON, RMVideo will wait in glXSwapBuffers(), and that causes frame skip errors
 in Trial mode and worse behavior in Continuous mode. With 2016-era NVidia drivers and more up-to-date GLX libraries, 
 the GLX_EXT_swap_control extension now works, and I can use it to verify that SyncToVBlank is off, or turn it off if 
 it is on. See disableSyncToVBlank().
 4) Dec 2018: I need to investigate "SyncToVBlank" further. First of all, setting the swap interval to 0 with the
 glXSwapControlEXT() function does not appear to be the same as disabling SyncToVBlank. I ran a test in which the 
 fullscreen background color toggles between red and blue on every frame. With a fast enough refresh, the expected 
 perception is a purple screen that flashes somewhat (less so at higher refresh rate). If swap interval is 1, we tend
 to see red and blue flashing, and the test lasts twice as long -- because the color change is occurring every other
 refresh instead of every refresh. SO: we MUST set swap interval to 0. However, with SyncToVBlank=OFF, I would 
 occasionally see a blue band at the top of the mostly purple screen. This suggests that tearing is present, despite
 the fact that we're waiting for the video sync counter to increment before we call glXSwapBuffers(). When I turn on
 SyncToVBlank, the test is successful -- a purple screen for the duration, with no visible tearing. This test result
 suggests that tearing has been present all along, we just haven't noticed it. But based on my observations in 2016,
 I have to keep SyncToVBlank off to avoid frame skips. Yet there were no frame skips during the color toggle test.
 5) Jan-Feb 2019: Based on recommendations in an OpenGL forum, and after extensive testing of various approaches, I've
 settled on a new implementation of the animation loop:
    --> Ensure VSync is ON.
    --> In the animation loop: (1) render on backbuffer; (2) glXSwapBuffers(); (3) glFinish(); (4) detect a skipped
        frame when T-N*P ~= P or greater, where T = actual elapsed time, N = #frames elapsed, and P is our estimate of
        the monitor refresh period; (5) get next update from Maestro; (6) repeat.
 Testing showed that, with VSync ON, the driver waits in glFinish() [not glXSwapBuffers()!] for the vertical blanking 
 interval to begin. However, the driver may yield the CPU while waiting, stalling RMVideo's single thread. There is 
 latency in resuming the thread, and that can result in a frame skip if the latency is too large. Therefore, it is 
 vital that the Linux scheduler be configured to be as responsive as possible. Also note that we no longer use 
 glXGetVideoSyncSGI(); testing showed that the driver stalled in this function call, and the "resume latencies" tended 
 to be worse than with the above approach. Finally, the new implementation required a fix to CElapsedTime, which used
 the wrong clock source, CLOCK_PROCESS_CPUTIME_ID. It now uses CLOCK_MONOTONIC.
    NOTE that prior versions of RMVideo, with VSync off, exhibited tearing because of scheduler latencies in the calls
 to glXGetVideoSync(). Indeed, I was able to demonstrate this by toggling the background between red and blue on every
 frame. Usually the background was perceived as purple, but occasionally I would see a thin red or blue strip near the
 top of the screen. With VSync now ON, the tearing artifact is eliminated.
    Also, it is very likely that the prior versions of RMVideo failed to detect a skipped frame during the occasional
 trial because of scheduler latencies resulting in delays returning from glXGetVideoSync(). Also, our estimate of the
 refresh period was slightly off because we used the wrong clock source for CElapsedTime.
    The latest implementation avoids tearing altogether, provides a more accurate estimate of the monitor refresh
 period, and should be much more reliable in terms of detecting skipped frames.
 6) Feb-Mar 2019: Because the bug in CElapsedTime resulted in a slight underestimate of the refresh period, and 
 because Maestro relied on that estimate to time the delivery of UPDATEFRAME commands, Maestro would steadily "get
 ahead" of RMVideo over time, with UPDATEFRAME commands stacking up in the network receive buffer. This was not a
 problem in Trial mode because trials in practice are only 1-2 seconds long. However, RMVideo remains in the animate
 state for an indefinitely long time in Continuous mode (as long as there's an RMVideo target in the active list, even
 if that target is not on), and I observed an ever-growing lag when moving an active target around in CM as time went
 on. This led me to try a new approach to communications in the animate state, with RMVideo sending a request for a
 new target update as soon as it processes the previous UPDATEFRAME. THIS DID NOT WORK, and I had difficulty figuring
 out why, but it seemed to be a problem on the Maestro side. I decided to give up on that approach, hoping that the
 more accurate refresh period (after the CElapsedTime fix) will be sufficient to address the problem. I did make some
 changes to the communication protocol with Maestro to ensure the two apps "stay together". See animate() in 
 rmvrenderer.cpp.

 REVISION HISTORY:
 25jul2005-- Began development; mostly just trying things out, making sure we can sync to vertical retrace, etc.
 17aug2005-- Began development of CRMVDisplay as the display and animation manager for RMVideo.
 31jan2006-- Mod IAW change in CRMVIo interface.  Now, RMVideo opens the fullscreen window only after a command
 session is initiated by CRMVIo::openSession().  The window is closed when the session is ended, and the RMVideo calls
 CRMVIo::openSession() to wait for the next session to start.
 02feb2006-- Mod IAW further changes in CRMVIo interface...
 29mar2006-- A "frame drop" signal (either RMV_SIG_DUPFRAME or _SKIPFRAME) is sent only ONCE during an animation
 sequence, instead of every time a frame drop is detected.  This change makes it easier to support how Maestro uses
 RMVideo in Continuous mode (frame updates are sent to RMVideo only sporadically instead of being streamed at the 
 monitor frame rate, as is the case in Trial mode).
 30mar2006-- Another refinement seeking to put a limit on the worst-case responsiveness of RMVideo. During an 
 animation, RMVideo draws each target in turn on the backbuffer, swaps buffers during the vertical sync, then checks
 for a new command. If it is animating a lot of complex targets, it could take much longer than one frame period to 
 draw a display frame on the backbuffer. While this situation is detected and reported via the RMV_SIG_SKIPFRAME 
 signal, we want to abort more gracefully from untenably slow animations. To do so, we've decided that RMVideo will 
 abort any animation sequence if a given frame has taken more than RMV_MAXFRAMELAG actual frame periods to render. 
 With this change, the worst case response time to a Maestro command is max(time to draw one target, RMV_MAXFRAMELAG 
 x framePer). There's currently no mechanism for "interrupting" RMVideo while it draws a single target.
 24apr2006-- Mod to support 24- or 16-bit RGB color, 24-bit preferred. Added new query command RMV_CMD_GETCOLORRES,
 by which RMVideo reports its color resolution in #bits.  The command is actually handled by the CRMVIo implementation,
 since color resolution is a static display property like frame period and display res.
 27apr2006-- Now require a minimum of 3 units in texture pipeline, to support best implementation (thus far) of plaid
 targets. This means RMVideo will no longer run on the Matrox G550, since available OpenGL driver only supports two 
 texture units.
 21jul2009-- Begun revisions to support reporting available video modes with a minimum horizontal resolution of 1024
 pixels and a minimum refresh of 75Hz. Requires the XRandR extension. If that extension is not available or not 
 supported on the machine, RMVideo will still run but it won't be possible to change the display resolution. Also, 
 24-bit color is now required.
 22jul2009-- Major changes. CRMVIo implementation no longer replies directly to any commands, so CRMVDisplay must
 reply to RMV_CMD_GETFRAMEPER and RMV_CMD_GETDISPRES. The animation-related commands provide motion vectors for all
 loaded targets, so CRMVIo::getMotionVector() no longer has the 'bNoChange' argument. Added support for listing video
 modes via RMV_CMD_GETALLVIDEOMODES and switching video mode via RMV_CMD_SETCURRVIDEOMODE.
 24aug2009-- Mod IAW same dtd changes to CRMVIoNet. Also added support for new command RMV_CMD_GETVERSION, _RESTART.
 May2016 -- Sundry changes while trying to build RMVideo with newer libraries for an up-to-date Linux kernel. The 
 major changes thus far: (1) Redesigned how video display modes are enumerated and how we switch modes. NVidia's
 support for RandR 1.0/1.1 protocols reports metamode IDs instead of refresh rates, which breaks the way we 
 originally did the enumeration. (2) Revamp of rmvmovie.*, which used deprecated and obsolete API from libavformat,
 libavcodec. (3) Modified to allow a minimum refresh of 60Hz, in case someone needs to use a LCD panel that doesn't 
 do better than 60Hz. (4) At start-up attempts to verify that SyncToVBlank is disabled; if enabled, tries to disable
 it using GLX_EXT_swap_control extension. Prints warning message if verification fails. See disableSyncToVBlank().
 23may2016-- Auto-update mechanism no longer supported. RMVideo must be updated manually. downloadExec() removed.
 05oct2016-- Updated to support new target class RMV_IMAGE, which displays a static image at any location on the
 screen. The notion of a "media store" replaces that of a "movie store", since it will contain both image and video
 files. CRMVMovieMgr was replaced by CRMVMediaMgr.
 24sep2018-- Started mods to implement vertical sync spot flash that may be triggered during an  animation sequence.
 A white square of fixed size is presented in the TL corner of screen with an optional dark margin below and to the 
 right of the spot....
 25sep2018-- Decided to get rid of the margin parameter for the sync flash: Just spot size and flash duration.
 26sep2018-- Revised rendering of sync flash spot to be analogous to the RMV_SPOT target.

 07dec2018-- Started major modifications to eliminate dependence on legacy "immediate-mode" OpenGL functions. 
 Conforming to OpenGL 3.3 Core Profile. Using vertex and fragment shaders for all rendering.
 17jan2019-- IMPORTANT change to eliminate tearing artifact: vertical sync is enabled, and animation loop changed
 so that we only check for frame skip after the buffer swap in glXSwapBuffers(). disableSyncToVBlank() replaced by
 enableSyncToVBlank().
 06feb2019-- Finalized re-design of the animation loop that relies on enabling vertical sync and calling glFinish()
 after glXSwapBuffers() to synchronize the CPU with the GPU. Fixed clock source for CElapsedTime, improving the 
 accuracy of refresh period measurement in estimateFramePeriod(). Now use elapsed time to detect a skipped frame; no
 longer use the extension glXGetVideoSyncSGI().
 26mar2019-- Mods to animate() to implement changes in communication protocol with RMVideo in animate mode. This is
 for RMVideo release version 9; commensurate changes in Maestro v4.0.5.
 08apr2019-- Making widespread changes in CRMVTarget and its subclasses to employ a single shader program for all 
 target rendering. Shader program switching can be a performance drag in OpenGL, and so far the OGL 3.3 implementation
 of RMVideo exhibits more frequent duplicate frames than OGL 1.1 implementation.
 22apr2019-- Added new singleton CRMVRenderer that handles almost all rendering tasks, except for the buffer swap,
 which involves GLX resources created by CRMVDisplay. Some functions that were handled by CRMVDisplay have been moved
 to CRMVRenderer -- particularly the loadTargets(), unloadTargets() and animate() commands. In addition, CRMVTarget is
 now a concrete class that handles all RMVideo target types, rather than having a separate class for each type.
 Updating CRMVDisplay IAW all of these changes.
 22apr2019-- Removing old code related to downloading a new RMVideo exec and restarting. The feature was removed some
 time ago (Maestro 3.2.1, RMVideo V6).
 09may2019-- Decided to adopt OpenGL3.3 Core Profile, along with the restructuring changes (single concrete CRMVTarget
 class; CRMVRenderer to encapsulate all OpenGL rendering), in the next RMVideo release: V10 with Maestro 4.1.0 -- plan
 to release in Jun 2019.
 28jan2020-- Modified to supply nominal refresh rate to CRMVRenderer::measurePeriod().
 28jan2020-- In Lubuntu 18.04 under 4.15, 5.0, and 5.3 kernels, running RMVideo's main thread at maximum SCHED_FIFO
 caused multi-second hangs in network receive packet processing and in NVidia driver's implementation of glFinish()
 with VSync enabled. While SCHED_FIFO works in the 3.19 kernel under Lubuntu 14.04, it's clearly not an option going
 forward. On our 4-core development machine, it was not an issue to leave the primary thread at normal SCHED_OTHER
 priority. Modified open/closeDisplay(), eliminating the change to SCHED_FIFO priority but requiring that the thread be
 eligible to run on more than one CPU.
 16dec2024-- Modified openDisplay() to first request a double-buffered visual with stereo support, in which case stereo
 mode is enabled. This should only work if the NVidia card supports stereo. For Priebe lab.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>             // to verify primary thread is eligible to run on more than one CPU

#include "rmviosim.h"            // CRMVIoSim -- Emulation of communication link with Maestro
#include "rmvionet.h"            // CRMVIoNet -- Implementation of communication link over TCP/IP
#include "rmvdisplay.h"


const int CRMVDisplay::STATE_DYING     = -1;
const int CRMVDisplay::STATE_OFF       = 0;
const int CRMVDisplay::STATE_IDLE      = 1;
const int CRMVDisplay::STATE_ANIMATE   = 2;


//=== CRMVDisplay (constructor) =======================================================================================
//
//    Construct a CRMVDisplay with no display resources.  Call start() to open the display.
//
CRMVDisplay::CRMVDisplay()
{
   m_pDisplay = NULL;
   m_bWindowCreated = false;
   m_bWindowDisplayed = false;
   m_glxContext = NULL;
   m_blankCursor = (Cursor) -1;
   m_pXVInfo = NULL;
   m_bStereoEnabled = false;

   m_bAltVideoModesSupported = false;
   m_pScreenRes = NULL;
   m_pOutInfo = NULL;
   m_pCrtcInfo = NULL;
   m_numVideoModes = 0;
   m_pVideoModes = NULL; 
   m_originalModeID = None;
   m_idxCurrVideoMode = -1;
   
   m_iWidthPix = 1024;
   m_iHeightPix = 768;

   m_pIOLink = NULL;
   m_iState = STATE_OFF;
}

//=== ~CRMVDisplay (destructor) =======================================================================================
//
//    Ensures any resources (incl connection to the X server!) allocated by this CRMVDisplay object are released.
//
CRMVDisplay::~CRMVDisplay()
{
   // release all OpenGL and X window stuff
   closeDisplay();

   // release the communication interface
   if( m_pIOLink != NULL )
   {
      delete m_pIOLink;
      m_pIOLink = NULL;
   }
}

//=== start ===========================================================================================================
//
//    This is essentially the entry point for the RMVideo application.  The application's main() function merely
//    instantiates the CRMVDisplay object and invokes this method.  The method does not return unless a fatal error
//    occurs (eg, the host machine does not have the resources that RMVideo requires).   Upon return, all resources
//    allocated by the CRMVDisplay object will be released; most importantly, the display should return to normal!
//
//    DEVNOTE:  By design, RMVideo should be able to continue running through multiple sessions of Maestro. Currently, 
//    there's no way to tell RMVideo to "die" -- we need some way to do this!
//
//    ARGS:       useEmulator -- [in] If true, emulate the Maestro-RMVideo communication interface.  This is purely for
//                               testing purposes.  The emulator will process a text file describing the targets to
//                               animate and the frame-by-frame target motion info during the animation sequence.  The
//                               emulator will deliver a series of Maestro commands to drive RMVideo through the
//                               animation.
//    RETURNS:    NONE.
//
void CRMVDisplay::start( bool useEmulator )
{
   // load media store
   if(!mediaMgr.load()) return;
   
   // set up our OpenGL fullscreen display (but don't show it).  If this fails, RMVideo is useless!
   if(!openDisplay()) return;

   // set up communication link with Maestro, or use an emulator that delivers a command sequence stored in a file
   m_pIOLink = NULL;
   if(useEmulator) m_pIOLink = new CRMVIoSim();
   else m_pIOLink = new CRMVIoNet();
   if(m_pIOLink == NULL || !m_pIOLink->init())
   {
      fprintf(stderr, "ERROR: Unable to set up Maestro communication interface!\n");
      if(m_pIOLink != NULL)
      {
         delete m_pIOLink;
         m_pIOLink = NULL;
      }
      return;
   }

   // run until we're told to die.  We also die if a really bad error occurs.
   while(m_iState > STATE_DYING)
   {
      // wait for Maestro to start a new command session (we BLOCK here; should not be hogging CPU).  If an error
      // occurs, RMVideo dies.  Otherwise, show the fullscreen window and enter "idle" state.
      if(!m_pIOLink->openSession())
         m_iState = STATE_DYING;
      else
      {
         showDisplay(true);
         m_iState = STATE_IDLE;
      }

      // handle the current Maestro command session until we get the "shutting down" command from Maestro
      while(m_iState > STATE_OFF)
      {
         if(m_iState == STATE_IDLE)
            idle();
         if(m_iState == STATE_ANIMATE)
         {
            int res = m_renderer.animate();
            if(res==1) m_iState = STATE_IDLE;
            else if(res==0) m_iState = STATE_OFF;
            else m_iState = STATE_DYING;
         }
      }

      // if we're dying, tell Maestro (if possible) before we sever the connection
      if(m_iState == STATE_DYING) m_pIOLink->sendSignal(RMV_SIG_QUITTING);
      
      // hide our fullscreen window
      showDisplay(false);

      // close the current command session
      m_pIOLink->closeSession();
   }

   // release the communication interface
   if(m_pIOLink != NULL)
   {
      m_pIOLink->cleanup();
      delete m_pIOLink;
      m_pIOLink = NULL;
   }

   // release the OpenGL fullscreen display window
   closeDisplay();
}

//=== openDisplay =====================================================================================================
//
//    Create the RMVideo display: a fullscreen window with an associated GLX context for rendering OpenGL commands.
//    RMVideo requires a direct GL rendering context, double-buffering support, and 24-bit RGB color.  It also requires
//    support for high-resolution timing. And the calling thread -- RMVideo's main thread of execution -- must be 
//    eligible to run on a minimum of 2 processors (a multi-core machine is now a requirement!)
//
//    The method fails if RMVideo cannot get access to all of the resources it needs, and an appropriate error message
//    is written on the standard error stream. In this case, RMVideo should exit. Otherwise, the fullscreen window will
//    be ready for use but NOT displayed. This method should be called only during RMVideo startup. The method 
//    showDisplay() is called to display/hide the fullscreen window when a Maestro-RMVideo command session begins/ends.
//
//    If the Xrandr extension is supported on the system, the method will prepare a list of all video modes that are
//    1024 x 768 at 60Hz or better, and RMVideo will support switching among these modes. In this case, if the original
//    video mode does not meet the minimum requirements, the method switches to one of the conforming modes. The 
//    original video mode is stored internally so that it may be restored when RMVideo exits. If the Xrandr extension
//    is not available or no satisfactory modes are found, mode switching is disabled.
//
//    Among the initialization tasks completed here is an estimate of the monitor frame rate over a 500-frame period.
//    Thus, expect the method to not return for more than 500/HZ seconds, where HZ is the frame rate. If unable to 
//    estimate frame rate, or if the current video mode does not satisfy the minimum requirement and a mode switch was
//    not possible, method will fail.
//
//    27jan2020: Until I began testing RMVideo on Lubuntu 18.04 (4.15 and 5.x kernels), RMVideo's main thread of 
//    execution was configured here to run with maximum SCHED_FIFO priority. However, testing in 18.04 on a 4-core
//    machine demonstrated that, under certain circumstance, this caused mysterious multi-second hangs in network 
//    receieve packet processing and in the NVidia driver's implementation of glFinish() waiting on the vertical blank 
//    interval (with VSync enabled, the driver must wait on the vertical blank interval before initiating a buffer swap).
//    Leaving the main thread at normal SCHED_OTHER priority eliminated these hangs entirely. So we no longer require
//    soft realtime priority here, but we DO require that the main thread be eligible to run on 2 or more cores.
//
//    16dec2024: First requests a stereo-enabled visual via glXChooseVisual; if successful, stereo mode is enabled. Else
//    requests the usual double-buffered visual.
//
//    ARGS:       NONE.
//    RETURNS:    True if fullscreen window was successfully opened; false otherwise. 
bool CRMVDisplay::openDisplay()
{
   // just for safety  -- we should only call this once
   if(m_pDisplay != NULL) return( true );

   // we require high-res timing support.  Fail if it is not available.
   if( !CElapsedTime::isSupported() )
   {
      fprintf(stderr, "ERROR: High-res timing support not available!\n" );
      return( false );
   }

   // we require that RMVideo's main thread of execution (the calling thread) be eligible to run on 2 or more CPUs
   cpu_set_t cpu;
   CPU_ZERO(&cpu);
   int count = 0;
   if(0 == ::pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu))
   {
      for(int i=0; i<CPU_SETSIZE; i++) if(CPU_ISSET(i, &cpu)) ++count;
   }
   if(count < 1)
      ::fprintf(stderr, "ERROR: Unable to verify that RMVideo's primary thread can run on multiple cores!\n");
   else if(count < 2)
      ::fprintf(stderr, "ERROR: RMVideo requires a multi-processor or multi-core machine. Cannot continue.\n");
   else if(count < 4)
      ::fprintf(stderr, "WARNING: Primary thread is configured to run on only %d cores; at least 4 recommended.\n", count);
   if(count < 2)
      return(false);

   // open a connection to the X server
   m_pDisplay = XOpenDisplay( NULL );
   if( m_pDisplay == NULL )
   {
      fprintf(stderr, "ERROR: Could not open connection to X server\n");
      return( false );
   }

   // make sure OpenGL's GLX extension supported
   int errorBase;
   int eventBase;
   if( !glXQueryExtension( m_pDisplay, &errorBase, &eventBase ) )
   {
      fprintf(stderr, "ERROR: X server has no OpenGL GLX extension\n");
      return( false );
   }

   // specify a double-buffered visual with 24-bit color and support for alpha channel AND stereo
   int doubleBufferVisualStereo[] =
   {
      GLX_RGBA,           // Needs to support RGBA color
      GLX_RED_SIZE, 8,    // 24-bit color
      GLX_GREEN_SIZE, 8,
      GLX_BLUE_SIZE, 8,
      GLX_STEREO,         // stereo support
      GLX_DOUBLEBUFFER,   // Needs to support double-buffering
      None                // end of list
   };

   // specify a double-buffered visual with 24-bit color, alpha channel support, NO stereo
   int doubleBufferVisual[]  =
   {
      GLX_RGBA,           // Needs to support RGBA color
      GLX_RED_SIZE, 8,    // 24-bit color
      GLX_GREEN_SIZE, 8,
      GLX_BLUE_SIZE, 8,
      GLX_DOUBLEBUFFER,   // Needs to support double-buffering
      None                // end of list
   };

   m_pXVInfo = glXChooseVisual(m_pDisplay, DefaultScreen(m_pDisplay), doubleBufferVisualStereo);
   if(m_pXVInfo == NULL)
   {
      m_pXVInfo = glXChooseVisual(m_pDisplay, DefaultScreen(m_pDisplay), doubleBufferVisual);
      if(m_pXVInfo == NULL) 
      {
         fprintf(stderr, "ERROR: Graphics doesn't support 24-bit RGB color with alpha channel and double-buffering\n");
         return(false);
      }
      m_bStereoEnabled = false;
      fprintf(stderr, "===> Stereo Mode NOT available.\n");
   }
   else
   {
      m_bStereoEnabled = true;
      fprintf(stderr, "Stereo Mode ENABLED!!\n");
   }

   // enumerate available video modes that meet minimum requirement: 1024x768@60Hz. If current video mode does not
   // meet this requirement, attempt to switch to a video mode that does.
   enumerateVideoModes();

   // get the width and height of the screen, in pixels. Fail if we don't have the minimum 1024 x 768.
   if(m_bAltVideoModesSupported)
   {
      m_iWidthPix = m_pVideoModes[m_idxCurrVideoMode].wPix;
      m_iHeightPix = m_pVideoModes[m_idxCurrVideoMode].hPix;
   }
   else
   {
      m_iWidthPix = DisplayWidth(m_pDisplay, m_pXVInfo->screen);
      m_iHeightPix = DisplayHeight(m_pDisplay, m_pXVInfo->screen);
   }

   fprintf( stderr, "Screen W,H = %d, %d pixels\n", m_iWidthPix, m_iHeightPix );
   if(m_iWidthPix < 1024 || m_iHeightPix < 768)
   {
      fprintf(stderr, "ERROR: Screen size does not meet minimum requirement (1024x768). Aborting!\n");
      return(false);
   }

   // create our fullscreen window and bind it to the GL rendering context -- No OpenGL calls before this!!!
   // NOTE: OpenGL rendering resources that are bound to the context are also created here!
   createFullscreenWindow();
   if(!m_bWindowCreated) 
   {
      fprintf(stderr, "ERROR: Failed to create fullscreen window with GL render context!\n");
      return(false);
   }

   // verify that SyncToVBlank is enabled; fail if unable to verify
   if(!enableSyncToVBlank()) return(false);

   // obtain an accurate estimate of the frame rate. We rely on this to keep animations in sync with Maestro timeline.
   // Must be ~60Hz or better. We show display during the measurement so that users can verify there's no tearing.
   // The background is toggled between red and blue every frame, so expect a steady purple background!
   showDisplay(true);
   bool ok = m_renderer.measureFramePeriod(m_bAltVideoModesSupported ? m_pVideoModes[m_idxCurrVideoMode].rate : 0);
   showDisplay(false);
   return(ok);
}

/**
 Helper method for openDisplay(). It uses the RandR extension to query the available video modes on the system. If the
 current mode does not meet minimum requirements (1024x768@60Hz), then -- if possible -- switch to a mode that does.

 BACKGROUND: The 2009-era implementation used the RandR 1.0/1.1 protocols to query the available sizes via XRRSizes()
 and XRRRates(). However, later NVidia drivers, which implement the protocols, return "metamode IDs" instead of refresh
 rates via XRRRates(). The current implementation requires RandR 1.2 or better, and uses XRRGetScreenResources(), 
 XRRGetOutputInfo() and XRRGetCrtcInfo() to do the enumeration. Again, only modes 1024x768@60Hz or better are accepted.

 No action is taken if RandR 1.2 or better is not available. In this case, mode switching is disabled, and RMVideo 
 should abort during startup if the current video mode does not meet minimum requirements.

 CREDITS: See http://www.reddit.com/r/gamedev/comments/2w8ahe/changing_resolutions_for_fullscreen_in_x11_linux. We had
 to make some adjustments. In particular, we had to enumerate the video output ports to determine which one was 
 actually connected to a monitor.
*/
void CRMVDisplay::enumerateVideoModes()
{
   // check RandR protocol version. If not 1.2 or better, do not attempt to enumerate video modes.
   int iRRMajor;
   int iRRMinor;
   if((!XRRQueryVersion(m_pDisplay, &iRRMajor, &iRRMinor)) || iRRMajor < 1 || iRRMinor < 2) 
   {
      fprintf(stderr, "WARNING: RandR unavailable or too old. Cannot switch video modes.\n");
      return;
   }

   // get screen resources. Use current screen resources if available, since this is faster.
   Window rootW = RootWindow(m_pDisplay, m_pXVInfo->screen);
   if(iRRMajor >= 1 && iRRMinor >= 3) m_pScreenRes = XRRGetScreenResourcesCurrent(m_pDisplay, rootW);
   else m_pScreenRes = XRRGetScreenResources(m_pDisplay, rootW);
   if(m_pScreenRes == NULL)
   {
      fprintf(stderr, "WARNING: Unable to access screen resources with RandR. Cannot switch video modes.\n");
      return;
   }

   // video card may have multiple outputs. We ASSUME only one is connected, and we choose that one. It also should
   // have some available modes assigned to it!
   for(int i=0; i<m_pScreenRes->noutput;i++)
   {
      m_pOutInfo = XRRGetOutputInfo(m_pDisplay, m_pScreenRes, m_pScreenRes->outputs[i]);
      if(m_pOutInfo != NULL && m_pOutInfo->connection == RR_Connected && m_pOutInfo->nmode > 0) break;
      XRRFreeOutputInfo(m_pOutInfo);
      m_pOutInfo = NULL;
   }
   if(m_pOutInfo == NULL)
   {
      XRRFreeScreenResources(m_pScreenRes); m_pScreenRes = NULL;
      fprintf(stderr, "WARNING: Unable to access video controller output with RandR. Cannot switch video modes.\n");
      return;
   }

   // get info on the current source CRTC for the connected output. Verify that this CRTC has non-zero dimensions, at
   // least one associated output, and a valid mode ID. This should always be the case; abort if not.
   m_pCrtcInfo = XRRGetCrtcInfo(m_pDisplay, m_pScreenRes, m_pOutInfo->crtc);
   if(m_pCrtcInfo == NULL || m_pCrtcInfo->width == 0 || m_pCrtcInfo->height == 0 || m_pCrtcInfo->noutput <= 0 ||
         m_pCrtcInfo->mode == None)
   {
      if(m_pCrtcInfo != NULL) { XRRFreeCrtcInfo(m_pCrtcInfo); m_pCrtcInfo = NULL; }
      XRRFreeOutputInfo(m_pOutInfo); m_pOutInfo = NULL;
      XRRFreeScreenResources(m_pScreenRes); m_pScreenRes = NULL;
      fprintf(stderr, "WARNING: Unable to access video output CRTC with RandR. Cannot switch video modes.\n");
      return;
   }

   // scan through the available modes and determine how many satisfy minimum requirements. NOTE: We ASSUME that the
   // CRTC output is NOT rotated, and take mode width and height as is. We also get the maximum supported H and V
   // resolution among the supported video modes.
   fprintf(stderr, "Checking available video modes 1024x768@60Hz or better...\n");
   int nModes = 0;
   int maxW = 0;
   int maxH = 0;
   for(int i=0; i<m_pScreenRes->nmode; i++) 
   {
      XRRModeInfo modeInfo = m_pScreenRes->modes[i];
      double refresh = ((double) modeInfo.dotClock) / (((double) modeInfo.hTotal) * ((double) modeInfo.vTotal));
      if(modeInfo.width >= 1024 && modeInfo.height >= 768 && refresh >= 59.5)
      {
         // make sure video output supports this mode before accepting it. Modes are identified by opaque RRMode ID.
         bool found = false;
         for(int j=0; j<m_pOutInfo->nmode && !found; j++) found = (m_pOutInfo->modes[j] == modeInfo.id);

         if(found)
         {
            ++nModes;
            if(modeInfo.width > maxW) maxW = modeInfo.width;
            if(modeInfo.height > maxH) maxH = modeInfo.height;
         }
      }
   }
   fprintf(stderr, "--->On output %s, %d of %d available video modes satisfy RMVideo requirements:\n", 
      m_pOutInfo->name, nModes, m_pOutInfo->nmode);
   if(nModes > RMV_MAXVMODES) fprintf(stderr, "    (Only accepting the first %d of these).\n", RMV_MAXVMODES);

   // if we did not find any alternate video modes, we're done. Free the resources obtained above.
   if(nModes == 0)
   {
      XRRFreeScreenResources(m_pScreenRes); m_pScreenRes = NULL;
      XRRFreeOutputInfo(m_pOutInfo); m_pOutInfo = NULL;
      XRRFreeCrtcInfo(m_pCrtcInfo); m_pCrtcInfo = NULL;
      return;
   }

   // remember the current video mode ID -- if it is not among the accepted video modes, we will switch out of it, and
   // we need to switch back when RMVideo exits
   m_originalModeID = m_pCrtcInfo->mode;
   int origW = 0;
   int origH = 0;

   // now scan through the modes again and build the available mode list. Identify the current video mode -- it should 
   // be in this list unless it does not satisfy the minimum requirements.
   m_bAltVideoModesSupported = true;
   m_numVideoModes = (nModes > RMV_MAXVMODES) ? RMV_MAXVMODES : nModes;
   m_pVideoModes = (VideoMode*) ::calloc(m_numVideoModes, sizeof(VideoMode));

   nModes = 0;
   m_idxCurrVideoMode = -1;
   for(int i=0; (i < m_pScreenRes->nmode) && (nModes < m_numVideoModes); i++) 
   {
      XRRModeInfo modeInfo = m_pScreenRes->modes[i];
      double refresh = ((double) modeInfo.dotClock) / (((double) modeInfo.hTotal) * ((double) modeInfo.vTotal));
      if(modeInfo.width >= 1024 && modeInfo.height >= 768 && refresh >= 59.5)
      {
         // make sure video output supports this mode before accepting it. Modes are identified by opaque RRMode ID.
         bool found = false;
         for(int j=0; j<m_pOutInfo->nmode && !found; j++) found = (m_pOutInfo->modes[j] == modeInfo.id);

         if(found)
         {
            m_pVideoModes[nModes].wPix = modeInfo.width;
            m_pVideoModes[nModes].hPix = modeInfo.height;
            m_pVideoModes[nModes].rate = (short) (refresh + 0.5);
            m_pVideoModes[nModes].modeID = modeInfo.id;
            if(modeInfo.id == m_originalModeID) m_idxCurrVideoMode = nModes;
            ++nModes;
            fprintf(stderr, "   %4d x %4d @ %3d\n", modeInfo.width, modeInfo.height, (short)(refresh+0.5));
         }
      }

      // remember H and V resolution for the current video mode -- need this if it's not a valid mode and we 
      // must switch right away to a mode that meets minimum requirements.
      if(modeInfo.id == m_originalModeID) 
      {
         origW = modeInfo.width;
         origH = modeInfo.height;
      }
   }

   // if the current video mode is NOT among the acceptable modes, we immediately switch to an accepted mode. If this
   // switch fails, then RMVideo will abort and resources will be freed in closeDisplay(). We may have to change
   // screen size as well to ensure it accommodate both the current and the new video mode.
   if(m_idxCurrVideoMode == -1)
   {
      fprintf(stderr, "--->Current video mode does not meet RMVideo requirements; switching modes...\n");
      m_idxCurrVideoMode = 0;
      
      int maxW = (m_pVideoModes[0].wPix > origW) ? m_pVideoModes[0].wPix : origW;
      int maxH = (m_pVideoModes[0].hPix > origH) ? m_pVideoModes[0].hPix : origH;
      XRRSetScreenSize(m_pDisplay, rootW, maxW, maxH, m_pOutInfo->mm_width, m_pOutInfo->mm_height);
 
      Status stat = XRRSetCrtcConfig(m_pDisplay, m_pScreenRes, m_pOutInfo->crtc, CurrentTime, m_pCrtcInfo->x,
           m_pCrtcInfo->y, m_pVideoModes[0].modeID, m_pCrtcInfo->rotation, m_pCrtcInfo->outputs, m_pCrtcInfo->noutput);
      if(stat == Success)
         fprintf(stderr, "--->OK. Video mode is now %d x %d @ %d\n", m_pVideoModes[0].wPix, m_pVideoModes[0].hPix, 
               m_pVideoModes[0].rate);
      else
      {
         XRRSetScreenSize(m_pDisplay, rootW, origW, origH, m_pOutInfo->mm_width, m_pOutInfo->mm_height);
         fprintf(stderr, "ERROR: Mode switch failed (error code = %d). RMVideo cannot continue.\n", (int) stat);
         return;
      }
   }

   // to ensure that panning is not possible, set screen size to match the current mode's resolution.
   XRRSetScreenSize(m_pDisplay, rootW, m_pVideoModes[m_idxCurrVideoMode].wPix, m_pVideoModes[m_idxCurrVideoMode].hPix,
      m_pOutInfo->mm_width, m_pOutInfo->mm_height);
}

//=== createFullscreenWindow ==========================================================================================
//
//    Destroys the current fullscreen window (if any), creates a new one, and binds the GL rendering context to it.
//    Should be called only by openDisplay() and when switching video modes. In the latter case, be sure to hide the
//    window before calling this.
//
//    In the OGL 3.3 implementation, various "global" objects are created that are bound to the rendering context: 
//    vertex arrays and buffers, shaders, etc. Since they are bound to the context, those resources are created here.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
void CRMVDisplay::createFullscreenWindow()
{
   fprintf(stderr, "Creating fullscreen window, OpenGL rendering context, and context-bound objects (shaders, etc)\n");

   // if it already exists, destroy it. We must create a new fullscreen window when switching video modes.
   if(m_bWindowCreated)
   {
      // release all OpenGL rendering resources that were created when the display was opened
      m_renderer.releaseResources();

      glXMakeCurrent(m_pDisplay, None, NULL);
      XDestroyWindow(m_pDisplay, m_window);
      m_bWindowCreated = false;
      
      glXDestroyContext(m_pDisplay, m_glxContext);
      m_glxContext = NULL;
   }
   
   // create an OpenGL rendering context.
   m_glxContext = glXCreateContext(m_pDisplay, m_pXVInfo, NULL, True);
   if(m_glxContext == NULL)
   {
      fprintf(stderr, "ERROR: Could not create rendering context\n");
      return;
   }

   // on the first call, create a blank (invisible) cursor to attach to our fullscreen window 
   if(m_blankCursor == (Cursor) -1)
   {
      XColor black;
      black.red = black.green = black.blue = 0;
      static char noData[] = { 0 };
      Pixmap bitmapNoData = XCreateBitmapFromData(m_pDisplay, RootWindow(m_pDisplay, m_pXVInfo->screen), noData, 1, 1);
      m_blankCursor = XCreatePixmapCursor(m_pDisplay, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
      XFreePixmap(m_pDisplay, bitmapNoData);
   }
   
   // create an X colormap since we're probably not using the default visual
   Colormap colorMap = XCreateColormap( m_pDisplay, RootWindow(m_pDisplay, m_pXVInfo->screen),
                                        m_pXVInfo->visual, AllocNone );
   // create an undecorated window that fills the screen.
   XSetWindowAttributes windowAttributes;
   windowAttributes.colormap = colorMap;
   windowAttributes.border_pixel = 0;
   windowAttributes.override_redirect = True;
   windowAttributes.event_mask =    ExposureMask           |
                                    VisibilityChangeMask   |
                                    KeyPressMask           |
                                    KeyReleaseMask         |
                                    ButtonPressMask        |
                                    ButtonReleaseMask      |
                                    PointerMotionMask      |
                                    StructureNotifyMask    |
                                    SubstructureNotifyMask |
                                    FocusChangeMask;
   windowAttributes.cursor = m_blankCursor;

   m_window = XCreateWindow( m_pDisplay, RootWindow(m_pDisplay, m_pXVInfo->screen), 0, 0, m_iWidthPix, m_iHeightPix, 0,
                             m_pXVInfo->depth, InputOutput, m_pXVInfo->visual,
                             CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect | CWCursor,
                             &windowAttributes );
   
   m_bWindowCreated = true;
   
   // bind the rendering context to the window
   glXMakeCurrent( m_pDisplay, m_window, m_glxContext );

   // we require OpenGL version 3.3 or greater 
   bool ok = true;
   if(atof( (const char *) glGetString(GL_VERSION) ) < 3.3)
   {
      fprintf(stderr, "ERROR: OpenGL version 3.3 or greater is required!\n");
      ok = false;
   }

   // we must get a direct context - we need direct rendering to maximize performance
   if(ok)
   {
      ok = glXIsDirect(m_pDisplay, m_glxContext);
      if(!ok) fprintf(stderr, "ERROR: GLX context is NOT direct!\n");
   }

   // our renderer must successfully allocate various OpenGL resources it needs
   if(ok) ok = m_renderer.createResources(this);

   // clean up on failure
   if(!ok)
   {
      m_renderer.releaseResources();

      glXMakeCurrent(m_pDisplay, None, NULL);
      XDestroyWindow( m_pDisplay, m_window );
      m_bWindowCreated = false;
      
      glXDestroyContext( m_pDisplay, m_glxContext );
      m_glxContext = NULL;
   }
}

//=== closeDisplay ====================================================================================================
//
//    Release the fullscreen window, OpenGL rendering context, and all other resources allocated in openDisplay(). The 
//    screen should return to normal (and to its original video mode) after calling this function.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
void CRMVDisplay::closeDisplay()
{
   // release current GLX context (if in fact there is one)
   if(m_pDisplay != NULL) glXMakeCurrent(m_pDisplay, None, NULL);

   // if we created the window, destroy it, and release all OpenGL resources allocated by our renderer
   if(m_bWindowCreated)
   {
      m_renderer.releaseResources();
      XDestroyWindow(m_pDisplay, m_window);
      m_bWindowCreated = false;
   }

   // free resources for our "blank" cursor
   if(m_blankCursor != (Cursor) -1)
   {
      XFreeCursor(m_pDisplay, m_blankCursor);
      m_blankCursor = (Cursor) -1;
   }

   // destroy the GLX rendering context
   if(m_glxContext != NULL)
   {
      glXDestroyContext(m_pDisplay, m_glxContext);
      m_glxContext = NULL;
   }

   // if necessary, restore original video mode. Destroy video mode-related structures.
   if(m_bAltVideoModesSupported)
   {
      if(m_idxCurrVideoMode > -1 && m_pVideoModes[m_idxCurrVideoMode].modeID != m_originalModeID)
      {
         XRRSetCrtcConfig(m_pDisplay, m_pScreenRes, m_pOutInfo->crtcs[0], CurrentTime, m_pCrtcInfo->x,
           m_pCrtcInfo->y, m_originalModeID, m_pCrtcInfo->rotation, m_pCrtcInfo->outputs, m_pCrtcInfo->noutput);
      }
        
      XRRFreeScreenResources(m_pScreenRes); m_pScreenRes = NULL;
      XRRFreeOutputInfo(m_pOutInfo); m_pOutInfo = NULL;
      XRRFreeCrtcInfo(m_pCrtcInfo); m_pCrtcInfo = NULL;
      ::free((void*) m_pVideoModes);
      m_pVideoModes = NULL;
      m_numVideoModes = 0;
      m_idxCurrVideoMode = -1;
      m_bAltVideoModesSupported = false;
      m_originalModeID = None;
   }
   
   // release structure holding information about the X visual
   if(m_pXVInfo != NULL)
   {
      XFree(m_pXVInfo);
      m_pXVInfo = NULL;
   }

   // close connection to X server -- this should free all resources
   if(m_pDisplay != NULL)
   {
      XCloseDisplay( m_pDisplay );
      m_pDisplay = NULL;
   }
}

//=== showDisplay =====================================================================================================
//
//    Show or hide the OpenGL fullscreen display window in which RMVideo targets are animated.
//
//    We create the fullscreen display in openDisplay() when RMVideo starts up.  However, between Maestro command
//    sessions, the fullscreen window should be hidden while RMVideo waits for a new connection request from a Maestro
//    client.  This method is used to show/hide the fullscreen window as needed.
//
//    ARGS:       bShow -- [in] If set, unveil the fullscreen window and bring it to the front.  Else, hide it.
//    RETURNS:    NONE.
//
void CRMVDisplay::showDisplay( bool bShow )
{
   // if window does not exist, or if it's already in the requested state, do nothing
   if((m_pDisplay == NULL) || (!m_bWindowCreated) || (bShow == m_bWindowDisplayed)) return;

   if(bShow)
   {
      // request the X window to be displayed on the screen, above all other windows
      int i = XMapRaised( m_pDisplay, m_window );
      
      // first, wait for expose event indicating that window is up and needs to be painted
      bool bExposed = false;
      unsigned long tSleep = 1000;
      XEvent event;
      while( !bExposed )
      {
         while( !XPending( m_pDisplay ) ) usleep(tSleep);
         XNextEvent( m_pDisplay, &event );
         if( event.type == Expose )
            bExposed = true;
      }
      
      m_bWindowDisplayed = true;

      // redraw idle background twice -- thereby clearing both front and back buffers to bkg color.
      // NOTE: If we don't do this before the first animation, the animation will sometimes fail on a "skipped frame" 
      // error on the first frame. Not sure what here is exactly necessary, but it appears to be working. 
      // (sar, 8/17/2009)
      m_renderer.redrawIdleBackground();
      m_renderer.redrawIdleBackground();
   }
   else
   {
      XWithdrawWindow(m_pDisplay, m_window, m_pXVInfo->screen);

      // we need to flush the event queue to make sure the window is withdrawn!
      XEvent event;
      while( XPending( m_pDisplay ) )
         XNextEvent( m_pDisplay, &event );
         
      m_bWindowDisplayed = false;
   }
}


//=== checkGLXExtension ===============================================================================================
//
//    Verify that the named GLX extension is supported on the host machine.
//
//    CREDIT: Adapted from the GL-related function 'isExtensionSupported' found on an OpenGL.org web page at
//    www.opengl.org/resources/features/OGLextensions/
//
//    ARGS:       extName  -- [in] official registered name of the GLX extension.
//    RETURNS:    True if extension is available; false otherwise.
bool CRMVDisplay::checkGLXExtension( const char* extName )
{
   const char *availExts = NULL;
   const char *start;
   char *where, *terminator;

   // make sure extension name has no spaces and is not empty
   where = (char *) strchr(extName, ' ');
   if( where || *extName == '\0' )
      return( false );

   availExts = glXQueryExtensionsString(m_pDisplay, m_pXVInfo->screen);
   start = availExts;
   for( ;; )
   {
      where = (char *) strstr( start, extName );
      if( !where ) break;
      terminator = where + strlen(extName);
      if( (where==start || *(where-1) == ' ') )
      {
         if( *terminator == ' ' || *terminator == '\0' )
            return( true );
      }
      start = terminator;
   }
   return( false );
}

//=== checkGLExtension ================================================================================================
//
//    Verify that the named OpenGL extension is supported on the host machine.
//
//    CREDIT: Adapted from the GL-related function 'isExtensionSupported' found on an OpenGL.org web page at
//    www.opengl.org/resources/features/OGLextensions/
//
//    ARGS:       extName  -- [in] official registered name of the GL extension.
//    RETURNS:    True if extension is available; false otherwise.
bool CRMVDisplay::checkGLExtension( const char* extName )
{
   const GLubyte *availExts = NULL;
   const GLubyte *start;
   GLubyte *where, *terminator;

   // make sure extension name has no spaces and is not empty
   where = (GLubyte*) strchr(extName, ' ');
   if( where || *extName == '\0' )
      return( false );

   availExts = glGetString(GL_EXTENSIONS);
   start = availExts;
   for( ;; )
   {
      where = (GLubyte*) strstr( (const char*) start, extName );
      if( !where ) break;
      terminator = where + strlen(extName);
      if( (where==start || *(where-1) == ' ') )
      {
         if( *terminator == ' ' || *terminator == '\0' )
            return( true );
      }
      start = terminator;
   }
   return( false );
}

/**
 Helper method for openDisplay(). It uses the GLX_EXT_swap_control extension to verify that "SyncToVBlank" is enabled,
 with a swap interval of one. If not, it will attempt to set the swap interval. If unable to verify that the swap 
 interval is one, it writes an error message indicating that RMVideo cannot run if the vertical sync is not enabled.
 @return True if successfully enabled "SyncToVBlank"; false otherwise. RMVideo must abort in the latter case.
*/
bool CRMVDisplay::enableSyncToVBlank()
{
   bool verified = false;
   if(checkGLXExtension("GLX_EXT_swap_control"))
   {
      uint swap = 3535; 
      GLXDrawable drawable = glXGetCurrentDrawable();
      if(drawable)
      {
         glXQueryDrawable(m_pDisplay, drawable, GLX_SWAP_INTERVAL_EXT, &swap);
         if(swap == 1) 
            verified = true;
         else
         {
            PFNGLXSWAPINTERVALEXTPROC pProc = 
               (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddress( (const GLubyte*)"glXSwapIntervalEXT" );
            if(pProc != NULL)
            {
               (*pProc)(m_pDisplay, drawable, 1);
               glXQueryDrawable(m_pDisplay, drawable, GLX_SWAP_INTERVAL_EXT, &swap);
               verified = (swap == 1);
            }
         }
      }
   }

   if(!verified) fprintf(stderr, "ERROR: Could not verify that vertical sync is on!\n");
   else fprintf(stderr, "Verified that vertical sync is on with a swap interval of 1.\n");

   return(verified);
}

//=== idle ============================================================================================================
//
//    Handles RMVideo's "idle" operational state, ie, when no animations are taking place.  RMVideo will be in this
//    state whenever Maestro is not using remote video targets.
//
//    RMVideo sleeps most of the time in this state, waking every 2ms to check for commands coming across the
//    communication interface with Maestro.  It handles commands to update the display geometry, change the current
//    background color, query or switch video modes, query movie store info, download movie files or the RMVideo 
//    executable, or load targets in preparation for an animation sequence.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
void CRMVDisplay::idle()
{
   double r, g, b;
   int w, h, d, packedRGB;

   // notify Maestro that we're entering idle state
   m_pIOLink->sendSignal(RMV_SIG_IDLE);

   m_renderer.redrawIdleBackground();

   while(m_iState == STATE_IDLE)
   {
      // check for and process next command from Maestro, if we got one
      int nextCmd = m_pIOLink->getNextCommand();
      if( nextCmd < RMV_CMD_NONE )
      {
         // the RMVideo-Maestro comm link has failed.  We must return to "off" state and see if we can open a new
         // command session.
         m_renderer.updateBkgColor(0, 0, 0);
         m_iState = STATE_OFF;
      }
      else if( nextCmd > RMV_CMD_NONE )
      {
         int iSig = 0;
         switch( nextCmd )
         {
            // get the current application version number
            case RMV_CMD_GETVERSION :
               iSig = (int) RMV_CURRENTVERSION;
               break;
               
            // whenever Maestro shuts down, we restore black bkg and return to "off" state
            case RMV_CMD_SHUTTINGDN :
               m_renderer.updateBkgColor(0, 0, 0);
               m_iState = STATE_OFF;
               iSig = RMV_SIG_BYE;
               break;

            // get all available video modes
            case RMV_CMD_GETALLVIDEOMODES :
               getAllVideoModes();
               break;
            
            case RMV_CMD_GETCURRVIDEOMODE :
               getCurrentVideoMode();
               break;
               
            // switch video modes (if supported) -- This can take up to ~7 seconds because the frame period is
            // remeasured after the mode switch!!
            case RMV_CMD_SETCURRVIDEOMODE :
               setCurrentVideoMode();
               break;
               
            // get or set the monitor's gamma correction factors
            case RMV_CMD_GETGAMMA :
               getGamma();
               break;
            case RMV_CMD_SETGAMMA :
               setGamma();
               break;
            
            // update parameters governing vertical sync spot flash in TL corner of screen (during animations)
            case RMV_CMD_SETSYNC :
               m_renderer.updateSyncFlashParams(m_pIOLink->getCommandArg(0), m_pIOLink->getCommandArg(1));
               iSig = RMV_SIG_CMDACK;
               break;

            // change the current background color
            case RMV_CMD_SETBKGCOLOR :
               packedRGB = m_pIOLink->getCommandArg(0);
               r = double(0x00FF & packedRGB)/255.0;
               packedRGB = (packedRGB >> 8);
               g = double(0x00FF & packedRGB)/255.0;
               packedRGB = (packedRGB >> 8);
               b = double(0x00FF & packedRGB)/255.0;
               
               m_renderer.updateBkgColor(r, g, b);
               iSig = RMV_SIG_CMDACK;
               break;

            // change the current display geometry; if invalid geometry, notify Maestro that command failed.
            case RMV_CMD_SETGEOMETRY :
               w = m_pIOLink->getCommandArg(0);
               h = m_pIOLink->getCommandArg(1);
               d = m_pIOLink->getCommandArg(2);
               m_renderer.updateDisplayGeometry(w, h, d);
               iSig = RMV_SIG_CMDACK;
               break;

            // retrieve list of all media store folders
            case RMV_CMD_GETMEDIADIRS :
               mediaMgr.replyGetMediaDirs(m_pIOLink);
               break;
            
            // retrieve media file listing for a particular folder in the media store
            case RMV_CMD_GETMEDIAFILES :
               mediaMgr.replyGetMediaFiles(m_pIOLink);
               break;
            
            // retrieve info on a particular image or video file in the media store
            case RMV_CMD_GETMEDIAINFO :
               mediaMgr.replyGetMediaInfo(m_pIOLink);
               break;

            // permanently remove a particular file or an entire folder from the media store
            case RMV_CMD_DELETEMEDIA :
               mediaMgr.replyDeleteMediaFile(m_pIOLink);
               break;

            // download a media file via a sequence of _PUTFILECHUNK commands and a terminal _PUTFILEDONE
            case RMV_CMD_PUTFILE :
               mediaMgr.downloadMediaFile(m_pIOLink);
               break;
            
            // we should NEVER see these here, as this would indicate they were sent before a download was initiated
            case RMV_CMD_PUTFILECHUNK :
            case RMV_CMD_PUTFILEDONE :
               fprintf(stderr, "(CRMVDisplay::idle) Got file chunk commands before a file download was initiated!\n");
               iSig = RMV_SIG_CMDERR;
               break;
               
            // create a target object for each target defined in this command, which is sent before starting an
            // animation. All targets should be initially turned off and positioned at the origin.
            case RMV_CMD_LOADTARGETS :
               iSig = m_renderer.loadTargets() ? RMV_SIG_CMDACK : RMV_SIG_CMDERR;
               break;

            // if targets have not been loaded, fail. Else, switch to "animate" mode immediately. The command data
            // is processed there.
            case RMV_CMD_STARTANIMATE :
               if(m_renderer.getNumTargetsLoaded() <= 0)
                  iSig = RMV_SIG_CMDERR;
               else
                  m_iState = STATE_ANIMATE;
               break;

            // we should not get this command in "idle", but if we do, simply make sure the target list is unloaded
            // and confirm that we're in the "idle" state.
            case RMV_CMD_STOPANIMATE :
               m_renderer.unloadTargets();
               iSig = RMV_SIG_IDLE;
               break;

            // program restart requested -- Restart is no longer supported, so we should never get this command.
            case RMV_CMD_RESTART :
               iSig = RMV_SIG_BYE;
               m_iState = STATE_DYING;
               break;
               
            // time to quit
            case RMV_CMD_EXIT :
               m_iState = STATE_DYING;
               break;

            // any other commands are unrecognized or should only be sent in "animation" mode
            default :
               iSig = RMV_SIG_CMDERR;
               break;
         }

         if(iSig != 0) m_pIOLink->sendSignal(iSig);
      }

      // sleep for 2ms between commands so we don't hog machine
      if(m_iState == STATE_IDLE)
         usleep( 2000 );
   }
}

//=== getAllVideoModes() ==============================================================================================
//    Helper method sends a reply to the RMV_CMD_GETALLVIDEOMODES command (in idle state only). If mode switching is 
//    supported, it sends the mode list prepared at startup. Otherwise, it sends a mode list of length 1 with the screen 
//    size and frame rate of the current (and only available) video mode.
void CRMVDisplay::getAllVideoModes()
{
   int* pReply = NULL;
   int len = 0;
   int errSig = RMV_SIG_CMDERR;
   if(m_bAltVideoModesSupported)
   {
      len = 2 + m_numVideoModes*3;
      pReply = (int*) ::calloc(len, sizeof(int));
      if(pReply == NULL)
      {
         len = 1;
         pReply = &errSig;
      }
      else
      {
         pReply[0] = RMV_SIG_CMDACK;
         pReply[1] = m_numVideoModes;
         int j=2;
         for(int i = 0; i < m_numVideoModes; i++)
         {
            pReply[j++] = m_pVideoModes[i].wPix;
            pReply[j++] = m_pVideoModes[i].hPix;
            pReply[j++] = m_pVideoModes[i].rate;
         }
      }
   }
   else
   {
      len = 5;
      pReply = (int*) ::calloc(len, sizeof(int));
      if(pReply == NULL)
      {
         len = 1;
         pReply = &errSig;
      }
      else
      {
         double framePer = m_renderer.getFramePeriod();
         pReply[0] = RMV_SIG_CMDACK;
         pReply[1] = 1;
         pReply[2] = m_iWidthPix;
         pReply[3] = m_iHeightPix;
         pReply[4] = (framePer <= 0.0) ? 60 : ((int) (0.5 + (1.0 / framePer)));
      }
   }
   
   m_pIOLink->sendData(len, pReply);
   if(len > 1) ::free(pReply);
}

//=== getCurrentVideoMode() ===========================================================================================
//    Helper method responds to the RMV_CMD_GETCURRVIDEOMODE command (in idle state only).
void CRMVDisplay::getCurrentVideoMode()
{
   double framePer = m_renderer.getFramePeriod();
   int reply[4];
   reply[0] = RMV_SIG_CMDACK;
   reply[1] = m_bAltVideoModesSupported ? (m_idxCurrVideoMode + 1) : 1;
   reply[2] = (int) (framePer * 1.0e9);
   m_pIOLink->sendData(3, reply);
}

//=== setCurrentVideoMode() ===========================================================================================
//    Helper method responds to the RMV_CMD_SETCURRVIDEOMODE command (in idle state only). If mode switching is 
//    supported, it gets the index of the desired mode from the comm link, performs the mode switch, measures the new
//    display frame period, and sends the appropriate reply. If mode switching is not supported, or if the requested
//    video mode is invalid, the command fails. 
//
//    NOTE that measuring the frame period can take up to 7 seconds!!! It is assumed that the Maestro side has been
//    setup to wait for this extended period when doing a mode switch.
//
//    On mode switching and panning: In RandR 1.2+, screen size is separate from video mode resolution. Screen size 
//    can be much larger than the video resolution, and the pointer movement would allow panning to different parts of
//    the "virtual screen". We do NOT want this behavior in RMVideo. Therefore, whenever we change modes, we ensure 
//    that screen size matches the resolution, so panning is effectively disabled. However, the order of operations is
//    important because the current video resolution can NEVER exceed screen size.
//
//    Let W,H be horizontal and vertical resolution in the current video mode and, by design, the current screen size.
//    Let W', H' be the resolution of the desired video mode. Then:
//       (1) If W' > W or H' > H, first set screen size to max(W,W') by max(H,H'). This new screen size is valid for
//    both the current and desired modes, which is required else the screen size change or mode switch would fail.
//       (2) Perform the mode switch. If that fails, restore the previous screen size.
//       (3) If W' < W or H' < H, set screen size to W',H' -- so no panning can happen in the new video mode.
//    Observe that two screen size updates may be required!
//
void CRMVDisplay::setCurrentVideoMode()
{
   int vm = m_pIOLink->getCommandArg(0);
   vm -= 1;
   if((!m_bAltVideoModesSupported) || vm < 0 || vm >= m_numVideoModes)
   {
      m_pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   if(vm != m_idxCurrVideoMode)
   {
      showDisplay(false);
      
      // if the H or V resolution in the new mode does not match the current screen dimensions, we update screen size 
      // to match -- thereby ensuring that panning cannot happen. If either dimension increases, we must update screen 
      // size to accommodate BOTH the before and after modes -- else the screen size change or mode switch will fail.
      // If either dimension decreases, we must update screen size after mode switch. Two screen size changes may be 
      // needed. During start-up, we assume screen size has been set to match resolution of the initial video mode.
      Window rootW = RootWindow(m_pDisplay, m_pXVInfo->screen);
      int oldW = m_pVideoModes[m_idxCurrVideoMode].wPix;
      int oldH = m_pVideoModes[m_idxCurrVideoMode].hPix;
      bool updBefore = (m_pVideoModes[vm].wPix > oldW || m_pVideoModes[vm].hPix > oldH);
      bool updAfter = (m_pVideoModes[vm].wPix < oldW || m_pVideoModes[vm].hPix < oldH);
      if(updBefore)
      {
         int wMax = m_pVideoModes[vm].wPix > oldW ? m_pVideoModes[vm].wPix : oldW;
         int hMax = m_pVideoModes[vm].hPix > oldH ? m_pVideoModes[vm].hPix : oldH;
         XRRSetScreenSize(m_pDisplay, rootW, wMax, hMax, m_pOutInfo->mm_width, m_pOutInfo->mm_height);
      }

      Status stat = XRRSetCrtcConfig(m_pDisplay, m_pScreenRes, m_pOutInfo->crtc, CurrentTime, m_pCrtcInfo->x,
           m_pCrtcInfo->y, m_pVideoModes[vm].modeID, m_pCrtcInfo->rotation, m_pCrtcInfo->outputs, m_pCrtcInfo->noutput);
      if(stat != Success)
      {
         fprintf(stderr, "[CRMVDisplay::setCurrentVideoMode] Mode switch failed, status=%d\n", stat);
         if(updBefore) XRRSetScreenSize(m_pDisplay, rootW, oldW, oldH, m_pOutInfo->mm_width, m_pOutInfo->mm_height);
         showDisplay(true);
         m_pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }

      m_idxCurrVideoMode = vm;
      m_iWidthPix = m_pVideoModes[vm].wPix;
      m_iHeightPix = m_pVideoModes[vm].hPix;
      
      // fix screen size if either dimension has decreased to ensure panning cannot happen.
      if(updAfter)
         XRRSetScreenSize(m_pDisplay, rootW, m_iWidthPix, m_iHeightPix, m_pOutInfo->mm_width, m_pOutInfo->mm_height);

      createFullscreenWindow();
      showDisplay(true);

      // obtain accurate estimate of frame rate.  We rely on this to keep animations in sync with Maestro timeline.
      m_renderer.measureFramePeriod(m_bAltVideoModesSupported ? m_pVideoModes[m_idxCurrVideoMode].rate : 0);
   }
   
   int reply[2];
   reply[0] = RMV_SIG_CMDACK;
   reply[1] = (int) (m_renderer.getFramePeriod() * 1.0e9);
   m_pIOLink->sendData(2, reply);
}

//=== getGamma(), setGamma() ==========================================================================================
//    Helper methods that reply to the RMV_CMD_GETGAMMA and RMV_CMD_SETGAMMA commands (in idle state only). Note that
//    gamma correction factors for _SETGAMMA are restricted to [RMV_MINGAMMA .. RMV_MAXGAMMA].
void CRMVDisplay::getGamma()
{
   XF86VidModeGamma gamma;
   if(XF86VidModeGetGamma(m_pDisplay, m_pXVInfo->screen, &gamma))
   {
      int reply[4];
      reply[0] = RMV_SIG_CMDACK;
      reply[1] = (int) (0.5f + 1000.0f * gamma.red);
      reply[2] = (int) (0.5f + 1000.0f * gamma.green);
      reply[3] = (int) (0.5f + 1000.0f * gamma.blue);
      m_pIOLink->sendData(4, reply);
   }
   else
      m_pIOLink->sendSignal(RMV_SIG_CMDERR);
}

void CRMVDisplay::setGamma()
{
   int r = m_pIOLink->getCommandArg(0);
   int g = m_pIOLink->getCommandArg(1);
   int b = m_pIOLink->getCommandArg(2);
   if(r < RMV_MINGAMMA || r > RMV_MAXGAMMA || g < RMV_MINGAMMA || g > RMV_MAXGAMMA ||
      b < RMV_MINGAMMA || b > RMV_MAXGAMMA)
   {
      fprintf(stderr, "[CRMVDisplay::setGamma] Out of range gamma factor(s): r=%d, g=%d, b=%d.\n", r, g, b);
      m_pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   XF86VidModeGamma gamma;
   gamma.red = ((float)r) / 1000.0f;
   gamma.green = ((float)g) / 1000.0f;
   gamma.blue = ((float)b) / 1000.0f;
   if(!XF86VidModeSetGamma(m_pDisplay, m_pXVInfo->screen, &gamma))
   {
      fprintf(stderr, "[CRMVDisplay::setGamma] Unable to adjust monitor gamma!\n");
      m_pIOLink->sendSignal(RMV_SIG_CMDERR);
   }
   else
   {
      fprintf(stderr, "Monitor gamma changed: r=%.2f, g=%.2f, b=%.2f\n", gamma.red, gamma.green, gamma.blue);

      // the current window is not affected by the gamma correction change, so we must hide and then show it
      // again to see the change
      showDisplay(false);
      showDisplay(true);

      m_pIOLink->sendSignal(RMV_SIG_CMDACK);
   }
}

