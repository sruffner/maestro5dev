/** rmvtarget.cpp =====================================================================================================
 Implementation of class CRMVTarget, representing any RMVideo target.

 CRMVTarget defines a target that can be drawn and animated on the RMVideo display. It handles initialization of the
 target object from the RMVideo target definition structure, RMVTGTDEF, including allocation of any special resources
 needed to render the target (texture, internal arrays for maintaining random-dot information, resources for streaming 
 frames from a video file, and so on). During an animation sequence, updateMotion() updates the target's state IAW
 the motion update vector provided, while draw() renders the target in accordance with its current state.

 In the OGL1.1-compatible incarnation of RMVideo, CRMVTarget was an abstract class, and a separate concrete class was
 developed for each target type. In the OGL3.3 implementation, the "one class per target type" design was discarded,
 and CRMVTarget was redesigned to handle all supported RMVideo target types. In addition, CRMVTarget is designed to
 work closely with CRMVRenderer, a singleton object which sets up the OpenGL rendering infrastructure and provides 
 methods for actually rendering the target into the current OpenGL "backbuffer". It hides the details of OpenGL from
 CRMVTarget. Through CRMVRenderer methods, CRMVTarget acquires and loads any texture object needed for rendering, 
 reserves a portion of the vertex array allocated at startup and shared among all targets during an animation sequence,
 and uploads vertex or texture data. During each animation frame, in its draw() call, CRMVTarget relies on CRMVRenderer
 methods to update the values of relevant shader uniform variables, then draw the primitives defined in the segment of
 the vertex array dedicated to the target.

 A note on coordinates/units: All RMVideo targets are sized, positioned, and animated in Maestro-centric coordinates
 -- visual degrees subtended at the subject's eye. The origin lies at the center of the screen, the horizontal axis
 increasing rightward, and the vertical axis increasing upward (the subject's line of sight, when fixating the origin,
 should intersect the plane of the screen at a normal angle). 

 ==> Implementation Notes for RMV_IMAGE
 All image source files, like the video files for RMV_MOVIE targets, are maintained in RMVideo's "media store", a 
 series of folders under the CRMVMediaMgr::MEDIASTOREDIR directory in RMVideo's installation directory, which is always
 the "current working directory" when RMVideo is running. Media files, whether images or videos, are typically
 downloaded from Maestro over its private Ethernet link with RMVideo, although it is certainly possible to copy files
 directly from CD or DVD media (or other means) onto the RMVideo machine's file system. A media file is identified by
 its file name "file.ext" and the name of its parent folder "folderName"; its file-system pathname is then
 "$RMVIDEO_HOME/media/folderName/file.ext". Media folder and file name restrictions apply. See the RMVideo media store
 store manager object CRMMediaMgr and RMVIDEO_COMMON.H for more information.

 At initialization, CRMVTarget attempts to load the image data from the image file specified by the folder and file
 names that are part of the target definition. If succesful, the image is bound to a texture (and thus stored in
 onboard memory on the graphics card). The image rectangle's width and height are computed in visual degrees subtended
 at the eye, using information from the RMVideo target renderer, CRMVRenderer.

 The image is always loaded in GL_RGBA pixel format, even if the source file is in a different format; thus,
 transparency/translucency IS supported. Supported image file formats: JPEG, PNG, BMP, PSD (Photoshop), and GIF. Note
 that CRMVMediaMgr relies on the STB_IMAGE.H library for image file loading; it does not support all the various 
 nuances of these image formats. It is always critical that users test their images before employing them in an actual
 experiment.

 ==> Implementation Notes for RMV_MOVIE
 All video files are maintained in RMVideo's "media store", as described above for the RMV_IMAGE target.

 At initialization, CRMVTarget relies on the renderer's video streamer, CVidBuffer, to open the video file and 
 initiate buffering of frames on a background thread. If the video stream is successfully opened, CVidBuffer returns
 a stream ID that is used in future calls to retrieve video frames sequentially. [CVidBuffer was introduced to enhance
 throughput for videos in hopes that RMVideo would be able to handle larger movie frame sizes at higher refresh 
 rates.] During an animation sequence, playback does not begin until the target is turned on (all targets are off at
 t=0). At that point, updateMotion() retrieves the first movie frame from the file and uploads the pixel data to the 
 OpenGL texture that was allocated during the initialization phase. When CRMVRenderer.drawTarget() renders the target,
 a simple quad is drawn equal to the size of the movie frame and the texture containing the movie frame is mapped onto
 it. The location of the quad can vary from frame to frame -- so it is possible to pan the "movie window" during 
 animation. This process repeats until the last frame is retrieved from the video file or the animation ends.

 With the video stream buffering in place, we hit another bottleneck in testing -- the speed with which frame data can
 be uploaded to the dedicated texture object on the video card. To address this problem, we introduced a round-robin 
 queue of 3 pixel buffer objects. Each video frame is uploaded to the texture via a pixel buffer. Frame N is copied to
 an available pixel buffer during the updateMotion() call for frame N-1; then during the call for frame N, that pixel
 buffer is uploaded to the texture. This scheme lets the driver DMA the pixel buffer contents to the video card at the
 same time that the CPU is memcpy()-ing the next frame to another pixel buffer. This change brought a significant 
 improvement in performance.

 Three flags in the target definition affect video playback behavior. They can be used in any combination.
    RMV_F_REPEAT: If this flag is set, the movie loops indefinitely during the animation sequence. CVidBuffer will
 always returns to the beginning of the video and starts over once it retrieves the last frame. This makes looping
 very simple. The movie will play over and over until the animation sequence stops. If the flag is not set, the movie 
 is played back only once.
    RMV_F_PAUSEWHENOFF: By design, Maestro trial targets continue to move when they are turned off. In keeping with 
 this principle, an RMV_MOVIE target continues "playing" the movie (but not rendering the frame) if the target is off
 -- unless this flag is set. In that case, playback is paused until the target is turned on again. Note that, 
 regardless the state of this flag, movie playback does not begin until the first time the target is turned on during
 an animation sequence.
    RMV_F_ATDISPRATE: The video file will typically include information about the rate at which movie frames should be
 presented. This rate is set when the video is created. Typical playback rates are 24 or 30Hz. If this flag is NOT set
 and the playback rate is known, CRMVTarget will attempt to play the movie at that rate as best it can. Since the 
 RMVideo monitor refresh rate may vary with the installation and with the selected video mode, it is likely that the
 refresh rate will not be an even multiple of the target playback rate, so some "jitter" is unavoidable. If this flag
 is set OR the playback rate is not available, CRMVTarget will draw a new movie frame for every display update, so the
 movie playback rate will equal the monitor refresh rate. Naturally, the playback rate chosen will affect the duration
 of the movie!

 ************* SUMMARY OF PEFORMANCE TESTING, Oct 2019 ****************************************************************
 24oct2019-- I spent the better part of a month trying various techniques to further improve the performance of 
 RMVideo when playing large videos (1024x768 or greater) at an update rate of 120Hz. The two changes which made the
 greatest impact:
   1) Implementing a round-robin queue of 3 pixel buffer objects to upload video frames to the target texture.
   2) Setting the scheduling policy of CVidBuffer's background buffering thread to SCHED_OTHER instead of SCHED_FIFO.
 It was also important that the video source contain only a single video stream and no audio. A 1280x720 video 
 containing an audio track exhibited much more frequent duplicate frame errors than one without.

 Here are some of the techniques I tried to further improve performance over the round-robin of 3 PBOs:
    a) Round-robin of 3 PBOs plus an alternating pair of texture objects -- so that one texture is loaded with the
 next video frame while the other is used to draw the current frame. 
    b) Instead of 3 PBOs, tried a single PBO large enough to hold 3 frames. Used GL_MAP_UNSYNCHRONIZED_BIT with 
 glMapBufferRange() and used fence sync objects with glClientWaitSync() to ensure we didn't write to a segment of
 the PBO that was still in use. Testing indicated that we never had to wait in glClientWaitSync().
    c) Tried a similar technique as (b), but using a persistently mapped pixel buffer. This requires OpenGL 4.4 (or
 the relevant extension), uses glBufferStorage() to allocate immutable storage for the pixel buffer. The memory 
 address for the buffer is mapped during target initialization via glMapBufferRange() with the flags GL_MAP_WRITE_BIT,
 MAP_PERSISTENT_BIT and MAP_COHERENT_BIT. Then that buffer can be used during target animation without constantly
 mapping and unmapping. It does again require the fence syncs to avoid writing to a segment of the buffer that is
 still in use by the OpenGL driver or the GPU.
    d) Tried to move the memcpy() that copies the next video frame into a PBO (or segment of a single 3x PBO) into
 the CVidBuffer thread. This required passing the mapped pixel buffer memory to CVidBuffer. It was rather tricky
 (got seg faults and race conditions initially), and it did not improve performance at all.
    e) Implemented a PBO pool -- the idea being that frequent allocation/deallocation of PBOs when running a trial
 set with lots of RMV_MOVIE targets was causing memory fragmentation that the driver had to "clean up" once in a
 while, leading to delays that resulted in duplicate frame errors.

28oct2019-- Tried one more idea: A single, large persistently mapped PBO that is segmented as needed to provide 
3-PBO queues for any RMV_MOVIE targets. The PBO was large enough to handle a single 2560x1440 or 1920x1080 video,
or multiple simultaneous videos at lower resolutions. This strategy did not measurably improve performance.

Again, NONE of these strategies offered a performance improvement over the basic round-robin queue of 3 PBOs (combined
with the CVidBuffer thread running with a normal scheduling policy). Furthermore, there were two test trials that,
by far, caused the most duplicate frame errors during testing. Both involved simultaneous playback of two movies. The
most problematic trial played two 1024x768 movies; the other had a 720x480 movie and a 352x288 movie. Trials playing
back a single video -- even 1920x1080 @ 120Hz -- rarely aborted. This suggests that there's a fair amount of overhead
in playing back two videos simultaneously (two asynchronous PBO->texture uploads, blitting from two different 
textures, more OGL state changes) that leads to poorer performance.

************* SUMMARY OF PEFORMANCE TESTING, Oct 2019 ****************************************************************

==> Implementation Notes for RMV_RANDOMDOTS
 RMVideo target type RMV_RANDOMDOTS represents a random-dot pattern windowed by a rectangular, elliptical, or annular
 aperture; a 2D Gaussian window may be applied (unless aperture is annular). The dot pattern moves independently of the
 target window itself, and the number of dots within the outer boundary of the aperture is a specified constant [but
 note that, for oval and annular apertures, some percentage of the dots will lie outside the aperture or be hidden by
 the "hole" in the aperture). Each dot in the pattern has the same RGB color (except in two-color constrast mode; see
 below), and dot size is restricted to [1..10] pixels. The target implements several special features:
   1) Percent coherence -- A specified percentage N% of the dots in the pattern move coherently during each update
 frame. This effect is achieved by randomly repositioning 100-N% of the dots on each update.
   2) Randomized finite dotlife -- If the target is assigned a nonzero maximum dot lifetime N, then each dot is
 initially assigned a random lifetime in [0..N]. Dotlife may be expressed in milliseconds or in degrees travelled. On
 each update frame, each dot's lifetime is decremented appropriately. When its dotlife expires, the dot is randomly
 repositioned within the target aperture, and its dotlife is reset to N.
   3) Per-dot speed or direction noise -- Normally, whenever the target pattern velocity V is nonzero, all dots in the
 pattern move with that velocity. However, if the target is assigned a nonzero noise update interval M, noise is added
 to the SPEED OR DIRECTION of each and every dot in the pattern. Every M milliseconds, the velocity vector of each dot
 is altered WRT the pattern velocity vector. In the case of direction noise, the dot velocity vector is the pattern
 velocity rotated by some angle D chosen randomly from the range [-N..N] (in whole degrees). N is the noise range limit
 and is restricted to [0..180]; the larger its value, the greater the directional noise. A uniform RNG is used to
 select the direction offset D for each dot.
    In the case of speed noise, there are now (as of Maestro v2.1.3) two alternative algorithms for noise generation. 
 In the original "additive" noise algorithm, the magnitude of the dot velocity vector is set to R=Ro + n*Ro/1000, where
 Ro is the pattern speed and n is the %-age noise factor randomly chosen from [-N:N] in 1% increments. Again, a uniform
 RNG is used, and N is restricted to [0..300]. In the multiplicative algorithm, R = Ro*2^n, where the exponent n is
 randomly chosen from [-N>>N], again using a uniform RNG. N is restricted to [1..7], and the range [-N..N] is divided
 into 100 equal bins.
    IMPORTANT: The per-dot direction or speed noise is selected every M milliseconds REGARDLESS the target's on/off
 state and even if its pattern velocity is zero. Take this into account in any offline computation of the target dots'
 trajectories during the animation!
   4) Flag RMV_F_WRTSCREEN (effective RMVideo v3, Jan 2010). When this flag is set, the target pattern displacment
 vector is specified WRT the screen center rather than the target window's center, as is normally the case. This flag
 was introduced so that RMV_RANDOMDOTS could better emulate the XYScope NOISYDIR and NOISYSPEED targets when both
 target and pattern are moving with the same nominal velocity vector. Prior to this change, you had to set pattern
 velocity to zero for the dots to move with the window, which meant that the dots would no longer be "noisy"!
   5) Two-color constrast mode. If the contrast C is non-zero for the target's R, G or B color component, then two
 distinct colors are defined: L1=M(1+C) and L2=M(1-C), where C=[0..1] and M is the nominal or mean luminance. Half the
 dots in the patch are drawn in one color, the rest in the other color.

 NOTE that the "aperture" is NOT implemented by alpha blending with an alpha mask texture -- as is the case for the
 RMV_SPOT, _GRATING, and _PLAID. Instead, on every frame, we compute each dot's alpha based on its current location and
 the aperture type. If a dot's center lies outside the aperture, its alpha is 0, else it is 1 (unless a Gaussian mask
 is specified, of course). The per-dot alpha is stored in the vertex attribute that normally holds the X-coordinate Tx
 of the dot's corresponding texel location. The monolithic fragment shader handles this special case, ignoring the 
 result of the texture operation and setting the fragment alpha to Tx.

==> Implementation Notes for RMV_FLOWFIELD
 The RMVideo target type RMV_FLOWFIELD represents a simple optical flow field of randomly located dots flowing radially
 toward or away from a "focus of expansion", or FOE. Each dot moves at a different speed depending on its radial
 distance from the FOE. The field is a circular annulus; the inner hole is necessary because dot velocity goes to zero
 as it approaches the FOE. This target recognizes only pattern motion; the FOE is ALWAYS located at the center of the
 screen, since the implementation assumes that the subject's line-of-sight passes through the screen plane at the FOE 
 at a perpendicular angle.
 
 Animation calculations: A single parameter, RMVTGTVEC.hPat, defines the per-frame motion of the flow field. This
 parameter is interpreted as deltaR, the change in radial position of a dot at a radius r2 = 1/2 the outer radius of
 the flow field. Both deltaR and r2 are specified in visual deg subtended at the eye. From these the method
 updateFlowField() calculates

    B = deltaR / (sin(r2) * cos(r2)).

 Then, the change in radial position dR of any dot at radius R in the flow field is dR = B*sin(R)*cos(R). After the new
 radial position of the dot is updated, we must check to make sure it has not exceeded the field's inner or outer radii
 (negative flows move toward the FOE, while positive flows emanate from it) -- in which case the dot is "recycled",
 because the number of dots in the flow field is a constant.

 Dot recycling is probably the trickiest part of the implementation; we adopted the same algorithms used for the flow
 field target implemented on Maestro's XY scope controller. The goal is to recycle the dots at an appropriate rate and
 in an appropriate manner so that the dot density over the field stays relatively uniform. For accelerating flows, this
 was not difficult -- when a dot's radius exceeds the outer edge, we merely relocate it anywhere in the field. 
 Decelerating flows (negative value for B) were not so easy. Because the dots slow down as they approach the FOE, they
 tend to accumulate there, and the previous recycling algorithm fails miserably. After some trial and error, we came up
 with a heuristic algorithm that yielded the best performance over the widest range of flowfield radii and values of B
 (which takes into account both frame rate and the flow velocity magnitude). Basically, the probability that a dot will
 be recycled during a given update frame is the product of two probabilities -- one that increases with the magnitude
 of B, and one that increases with decreasing radius. If it is recycled, the dot's angle is randomized in [0..360), 
 while its radius is randomized within an annulus at the outer edge of the field, between r = rOuter and r = rOuter -
 pos_change_at(rOuter). This last trick avoids the "rings" that would appear for larger values of B (b/c a relatively
 large fraction of the dots are recycled every update frame).


 REVISION HISTORY:
 15apr2019-- Began development. Instead of being the abstract base class for RMVideo targets, with a separate concrete
 class for each type, CRMVTarget now handles the implementation of all target types. Furthermore, OpenGL code is 
 isolated within the singleton render manager, CRMVRenderer, and the draw() method calls CRMVRenderer methods to render
 the target to the current OpenGL backbuffer.
 29apr2019-- Implementing a memory buffer pool for the backing storage required for RMV_RANDOMDOTS and _FLOWFIELD. 
 30apr2019-- Modified to use texture pool managed by CRMVRenderer. Also, maintain reference to the target renderer
 internally -- the reference is provided via initialize().
 06may2019-- Modified to implement new "flicker" feature for all target types. Whenever the target "on" flag is set,
 the target can flicker IAW set ON and OFF durations, with an initial delay before the first ON flicker. Target motion
 continues as usual, regardless the flicker state. Furthermore, the flicker state resets each time the target's "on/off"
 flag is toggled "on".
 04jun2019-- Bug fix in initTargetColors(): Failed to compute Lmin correctly for G,B components for RMV_RANDOMDOTS in
 the two-color contrast mode.
 04jun2019-- Revised implementation of aperture for RMV_RANDOMDOTS. Instead of using alpha mask, per-dot alpha is 
 computed every frame and delivered to the fragment shader via vertex attribute "Tx". The monolithic shader was 
 modified to handle this special case.
 18sep2019-- Refactor: CVidBuffer (accessed through CRMVRenderer) implements a scheme for opening video streams that
 are then buffered on a background thread to improve playback performance, particularly for movies that have a larger
 frame size. All LIBAV calls are now isolated in CVidBuffer.
 23sep2019-- Prior to the introduction of CVidBuffer, updateMovie() for RMV_MOVIE would block waiting until the next
 frame was read in. We implement the same block by waiting until CVidBuffer::getCurrentFrameData() returns non-NULL.
 However, if the video streaming failed, it will always return NULL. So we need to detect this condition, which must
 be considered fatal for the ongoing animation sequence. To communicate this to CRMVRenderer, updateMotion() now 
 returns FALSE if a fatal error has occurred.
 24sep2019-- To ensure that the texture object backing an RMV_IMAGE target is truly allocated and loaded onboard the
 GPU, we actually "draw" the image in the initialize() method. This is because testing has shown that the actual 
 allocation and loading of the texture object may be delayed by the OpenGL driver until the texture is actually used.
 Since allocation may take longer for a large texture (eg. for a 2560x1440 image), the delay may be significant
 enough that it causes a frame drop if it happens in the middle of an RMVideo animation sequence. By doing a draw
 in the initialize() method, we force the driver to allocate the texture object at that time, BEFORE the animation
 starts. Since drawing happens on the backbuffer, and since CRMVRenderer clears the backbuffer after loading all 
 targets, nothing appears onscreen.
 30sep2019-- Implemented a round-robin scheme using pixel buffer objects to upload frame data to the texture for the
 RMV_MOVIE implementation, rather than uploading directly from a normal CPU-side array. This brought a significant
 performance improvement, so that it is now feasible to playback a 1024x768 video at 144Hz.
 02oct2019-- Testing suggested that we're hitting the pixel transfer bottleneck for videos larger than 1024x768.
 Tried a new scheme with 3 PBOs and 3 texture objects. In each frame period, we draw tex N (which was uploaded 
 previously via PBO N), upload to tex N+1 via PBO N+1, and copy frame N+2 to PBO N+2. This scheme did not work at
 all; in fact, performance was poorer.
 17oct2019-- Implemented a pixel buffer object pool to see if reusing PBOs would improve performance. Test results
 were not conclusive. Commented out the PBO pool implementation, in case we want to revisit it at a later date.
 22oct2019-- Tried glMapBufferRange(GL_MAP_WRITE_BIT|GL_MAP_UNSYNCHRONIZED_BIT). In Chapter 28 of "OpenGL Insights", 
 testing suggested that -- with NVidia GPUs -- using triple buffering within a single buffer object (3x size needed) 
 via glMapBufferRange() and handling synch manually offered better performance over other techniques. I tried the 
 technique and it did NOT improve performance.
 23oct2019-- Tried similar technique but using immutable storage via glBufferStorage(), a persistently mapped 
 buffer via GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT. Requires OpenGL 4.4 or relevant extension.
 DID NOT IMPROVE PERFORMANCE. Then combined this technique with a PBO pool; performance was marginally worse.
 Returned to the round-robin scheme with 3 PBOs.
 23oct2019-- Tried round-robin queue of 3 PBOs with a pair of identical textures that were alternately uploaded and
 drawn during each refresh period. DID NOT IMPROVE PERFORMANCE.
 24oct2019-- Decided to stick with the round-robin queue of 3 PBOs for the RMV_MOVIE implementation, with no PBO pool
 (since it did not improve performance). Removed the code that implemented the PBO pool.
 28oct2019-- Tried a single massive persistently mapped PBO that was partitioned into 3-PBO queues for each movie
 target (until fixed capacity reached). DID NOT IMPROVE PERFORMANCE.
 04nov2019-- Wrapped up performance tweaking and testing. Releasing changes as RMVideo 10c.
 16dec2024-- To support stereo experiments with the dot targets RMV_POINT, _FLOWFIELD, and _RANDOMDOTS, draw() now
 takes a parameter 'eye' such that the target center is horizontally offset by 'eye*RMVTGTDEF.fDotDisp'. For all other
 target types and for dot targets with fDotDisp = 0, this argument has no effect.
*/

#include "stdio.h"
#include "time.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rmvrenderer.h"
#include "rmvmediamgr.h"            // CRMVMediaMgr -- With methods for handling image/video files.
#include "vidbuffer.h"              // CVidBuffer -- Handles streaming of video frames on a background thread
#include "rmvtarget.h"

CRMVTarget::FloatBufNode* CRMVTarget::g_FloatBufPool = NULL;

/**
 Create a pool of memory buffers used to store per-dot vertex attributes, lifetimes and noise factors for the 
 random-dot target types RMV_RANDOMDOTS and RMV_FLOWFIELD.

 Unlike all of the other RMVideo target types, the random-dot targets can have a significant number of vertices -- one
 per dot --, and the vertex attributes of each dot typically change on a frame-by-frame basis. As a result, these 
 targets require a large CPU-side buffer to store the vertex attributes. Furthermore, any RMV_RANDOMDOTS target with 
 the "finite dot life" and "noise" features require additional buffers to track each dot's current lifetime and noise
 factor. Allocating these buffers each time a target is created (and deallocating them when the target is deleted) is
 inefficient and may be a performance drag when running trials involving random-dot targets in rapid succession. 

 Instead, CRMVTarget implements a static memory buffer pool so that previously allocated buffers can be reused.

 This method should be called during startup to create the initial buffer pool. It will preallocate 10 buffers, each
 of which can handle the vertex attributes for a random-dot target with the maximum allowed number of dots in Maestro.

 @return True if successful; false otherwise, in which case a brief error message is printed to stderr. On failure, 
 RMVideo should not continue.
*/
bool CRMVTarget::createBufferPool()
{
   if(g_FloatBufPool != NULL) return(true);

   bool ok = true;
   FloatBufNode* pParent = NULL;
   for(int i=0; ok && i<10; i++)
   {
      FloatBufNode* pNode = new FloatBufNode;
      ok = (pNode != NULL);
      if(ok)
      {
         pNode->size = 10000 * 4;
         pNode->inUse = false;
         pNode->pNext = NULL;
         pNode->pBuf = new float[pNode->size];
         if(pNode->pBuf == NULL)
         {
            delete pNode;
            ok = false;
         }
         else
         {
            if(pParent == NULL) g_FloatBufPool = pNode;
            else pParent->pNext = pNode;
            pParent = pNode;
         }
      }
   }

   if(!ok)
   {
      destroyBufferPool();
      fprintf(stderr, "ERROR(CRMVTarget): Failed to create memory pool for random-dot parameter storage\n");
   }

   return(ok);
}

/**
 Destroy the pool of memory buffers used to store per-dot vertex attributes, lifetimes and noise factors for the 
 random-dot target types RMV_RANDOMDOTS and RMV_FLOWFIELD. Call this method before RMVideo exits.
*/
void CRMVTarget::destroyBufferPool()
{
   while(g_FloatBufPool != NULL)
   {
      FloatBufNode* pNode = g_FloatBufPool;
      g_FloatBufPool = pNode->pNext;

      if(pNode->pBuf != NULL) delete [] pNode->pBuf;
      delete pNode;
   }
}

/**
 Get a buffer for dot storage from buffer pool, allocating new one if necessary. The static buffer pool is created if 
 it does not yet exist.
 @param sz The buffer size required. 
 @return Pointer to the buffer node, or NULL on failure.
*/
CRMVTarget::FloatBufNode* CRMVTarget::getBufferNodeFromPool(int sz)
{
   if(!createBufferPool()) return(NULL);

   FloatBufNode* pNode = g_FloatBufPool;
   FloatBufNode* pParent = NULL;
   while(pNode != NULL && (pNode->inUse || pNode->size < sz))
   {
      pParent = pNode;
      pNode = pParent->pNext;
   }

   // no available nodes -- allocate a new one at the end of our singly linked list
   if(pNode == NULL)
   {
      pNode = new FloatBufNode;
      if(pNode != NULL)
      {
         pNode->pBuf = new float[sz];
         if(pNode->pBuf != NULL)
         {
            pNode->size = sz;
            pNode->pNext = NULL;
            pParent->pNext = pNode;
         }
         else
         {
            delete pNode;
            pNode = NULL;
         }
      }
   }

   // if successful, mark buffer node as in use
   if(pNode != NULL) pNode->inUse = true;

   return(pNode);
}

/**
 Release a buffer that was previoiusly used for dot storage back to that static buffer pool for reuse. This method will
 also trim down the buffer pool if it grows too large and has too many unused buffers.
*/
void CRMVTarget::releaseBufferNodeToPool(CRMVTarget::FloatBufNode* pNode)
{
   if(pNode == NULL) return;

   // make sure this is a node in the pool. At the same time, get pool size and current number of unused buffers.
   int nNodes = 0;
   int nUnused = 0;
   FloatBufNode* pCurr = g_FloatBufPool;
   while(pCurr != NULL)
   {
      if(pCurr == pNode) pCurr->inUse = false;

      ++nNodes;
      if(!pCurr->inUse) ++nUnused;
      pCurr = pCurr->pNext;
   }

   // release some nodes if pool contains more than 30 buffers and 5 or more unused buffers
   if(nNodes >= 30 && nUnused >= 5)
   {
      pCurr = g_FloatBufPool;
      FloatBufNode* pParent = NULL;
      while(pCurr != NULL && nNodes > 10)
      {
         if(!pCurr->inUse)
         {
            FloatBufNode* pDead = pCurr;
            pCurr = pDead->pNext;
            if(pParent != NULL) pParent->pNext = pCurr;
            else g_FloatBufPool = pCurr;
            
            delete [] pDead->pBuf;
            delete pDead;
            --nNodes;
         }
         else
         {
            pParent = pCurr;
            pCurr = pCurr->pNext;
         }
      }
   }
}


CRMVTarget::CRMVTarget()
{
   m_pRenderer = NULL;
   ::memset(&m_tgtDef, 0, sizeof(RMVTGTDEF));
   m_bOn = false;
   m_centerPt.Zero();
   for(int i=0; i<3; i++) m_rgb0[i] = m_rgb1[i] = m_rgbCon0[i] = m_rgbCon1[i] = 0.0;
   m_isTwoColor = false;
   m_flickerState = FLICKER_DISABLED;
   m_flickerFramesLeft = 0;

   m_texID = 0;
   m_vtxArrayStart = m_vtxArrayCount = 0;

   m_pfBufDots = m_pfBufDotLives = m_pfBufDotNoise = (CRMVTarget::FloatBufNode*) NULL;
   m_pDotRNG = m_pNoiseRNG = NULL;
   m_tUntilNoiseUpdate = 0.0f;
 
   for( int i=0; i<2; i++ )
   {
      m_fSpatialPerX[i] = m_fSpatialPerY[i] = 1.0f;
      m_fCurrOrient[i] = 0.0f;
      m_fCurrPhase[i] = 0.0f;
   }

   m_videoStreamID = -1;
   m_iMovieState = MOVIE_UNINITIALIZED;
   m_gotLastFrame = false;
   m_tElapsed = 0.0;
   m_tNextFrame = 0.0;
   m_tPlaybackIntv = -1.0;

   for(int i=0; i<NUMPBOS; i++) m_pboIDs[i] = 0;
   m_iCurrPBOIdx = -1;
}

CRMVTarget::~CRMVTarget()
{
   freeResources();
}

/**
 Initialize RMVideo target IAW the target definition specified. Validate target definition, allocate any additional 
 OpenGL resources required to render the target, position target at the origin, and turn it off initially.

 Target initialization can include GL calls that draw on the current backbuffer. As long as the backbuffer is not
 swapped before it is cleared, there will be no visible effect onscreen. In fact, CRMVRenderer::loadTargets() will 
 call glFinish() after loading and initializing all targets in the animated target list; it then redraws the idle
 background to ensure the backbuffer is in the expected state.

 Testing with large RMV_IMAGE targets has demonstrated that the driver may delay the allocation and loading operation 
 until the texture object is actually used; if this happens in the middle of an ongoing animation sequence (if the 
 target turns "ON" at some t>0), the time to allocate and load the texture object may result in a duplicate frame 
 error. To avoid this, we force texture allocation by calling draw() after completing target initialization.
 
 @param pRenderer [in] The RMVideo target renderer. Needed to allocate certain OpenGL resources dedicated to target.
 A reference to the renderer is maintained internally for later use.
 @param tgtDef [in] The target's definition. 
 @return True if successful; false if target definition is invalid or a required OpenGL resource was not allocated.
*/
bool CRMVTarget::initialize(CRMVRenderer* pRenderer, const RMVTGTDEF& tgtDef)
{
   if(pRenderer == NULL) return(false);

   m_pRenderer = pRenderer;
   m_tgtDef = tgtDef;
   if(!validateTargetDef()) return(false);

   initTargetColors();

   if(!allocateResources()) return(false);

   m_centerPt.Zero(); 
   setOn(false); 

   // pre-draw all targets using a texture to force allocation of texture object on the GPU before animation starts
   if(m_texID != 0)
   {
      setOn(true);
      
      // special case: RMV_MOVIE does not actually load the first frame's pixel data until it is turned on. Here we
      // retreive and load that first frame WITHOUT advancing the buffered video stream. Note that the first frame
      // MUST be ready, since the first 10 frames are buffered when the video stream is opened.
      if(m_tgtDef.iType == RMV_MOVIE)
      {
         // NOTE: We use a round-robin pixel buffer object for upload while animating, rather than uploading from 
         // a normal CPU-side array, as we do here.
         uint8_t* pDstBuf = m_pRenderer->m_vidBuffer.getCurrentFrameData(m_videoStreamID);
         int w = m_pRenderer->m_vidBuffer.getVideoWidth(m_videoStreamID);
         int h = m_pRenderer->m_vidBuffer.getVideoHeight(m_videoStreamID);
         m_pRenderer->uploadMovieFrameToTexture(m_texID, w, h, pDstBuf);
         m_iMovieState = MOVIE_GOTFRAME;
      }

      draw(0);
      setOn(false);

      // special case: restore state of RMV_MOVIE to "not started". In addition, we use a round-robin queue of
      // pixel buffer objects to upload frames to the GPU texture object. During each updateMotion() call, we copy
      // frame N+1 to one PBO and upload frame N from PBO to texture. So, prior to animation start, we need to
      // copy the very first frame to the current PBO, then advance to the next frame.
      if(m_tgtDef.iType == RMV_MOVIE) 
      {
         m_iMovieState = MOVIE_NOTSTARTED;
         uint8_t* pDstBuf = m_pRenderer->m_vidBuffer.getCurrentFrameData(m_videoStreamID);
         m_iCurrPBOIdx = 0;

         glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIDs[m_iCurrPBOIdx]);
         uint8_t* pPBO = (uint8_t*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
         if(pPBO == NULL)
         {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            ::fprintf(stderr, "ERROR(CRMVTarget): Unable to map pixel buffer object\n");
            return(false);
         }

         ::memcpy(pPBO, pDstBuf, m_pRenderer->m_vidBuffer.getVideoFrameSize(m_videoStreamID));
         glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
         glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
         m_pRenderer->m_vidBuffer.advanceToNextFrame(m_videoStreamID);
      }
   }

   return(true);
}

/**
 Update target's internal animation state IAW the specified motion.

 For all target types, the ON/OFF state and target window center are updated IAW the specified target update vector 
 (note that RMV_FLOWFIED is always centered on screen, however). For selected target types, some additional work is
 required. See the approriate helper methods for a description.

 @param tElapsed [in] Time elapsed since the previous update, ie, the display refresh period. In milliseconds.
 @param pTgtVec [in] The target motion update vector. If NULL, no action taken.
 @return True if successful, false if a fatal error occurred. In the latter case, the ongoing animation is terminated.
*/
bool CRMVTarget::updateMotion(float tElapsed, PRMVTGTVEC pVec)
{
   if(pVec==NULL) return(true);

   // update target ON/OFF state and window location. This is all we need to do for some target types!
   // (NOTE: Target center point not used for FLOWFIELD target, which is always centered on screen)
   setOn(pVec->bOn);
   if(m_tgtDef.iType != RMV_FLOWFIELD) m_centerPt.Offset(pVec->hWin, pVec->vWin);

   // update flicker state and frame count for current phase (if applicable)
   updateFlickerState();

   // certain target types need additional work...
   bool ok = true;
   switch(m_tgtDef.iType)
   {
   case RMV_GRATING:
   case RMV_PLAID: 
      updatePlaid(pVec);
      break;
   case RMV_RANDOMDOTS:
      updateRandomDots(tElapsed, pVec);
      break;
   case RMV_FLOWFIELD:
      updateFlowField(pVec);
      break;
   case RMV_MOVIE:
      ok = updateMovie(tElapsed, pVec);
      break;
   default: 
      break;
   }
   return(ok);
}

/**
 Helper method for updateMotion(): Update the target's flicker state.

 If the target is turned off or the flicker feature is disabled, no action is taken. Otherwise, it updates the flicker 
 frame countdown timer and status appropriately. When the flicker feature is enabled, the target is drawn only if it is
 currently turned on AND the flicker cycle is currently in its "ON" phase.

 NOTE: This operates on the assumption that updateMotion() is called once per video frame. If a duplicate frame occurs
 during an animation sequence with a flickering target, that target's flicker cycle will likely be altered.
*/
void CRMVTarget::updateFlickerState()
{
   if(m_flickerState == FLICKER_DISABLED || !m_bOn) return;
   if(m_flickerState == FLICKER_RESET)
   {
      if(m_tgtDef.iFlickerDelay > 0)
      {
         m_flickerFramesLeft = m_tgtDef.iFlickerDelay;
         m_flickerState = FLICKER_INITDELAY;
      }
      else
      {
         m_flickerFramesLeft = m_tgtDef.iFlickerOn;
         m_flickerState = FLICKER_ON;
      }
   }
   else
   {
      --m_flickerFramesLeft;
      if(m_flickerFramesLeft <= 0)
      {
         if(m_flickerState == FLICKER_INITDELAY || m_flickerState == FLICKER_OFF)
         {
            m_flickerState = FLICKER_ON;
            m_flickerFramesLeft = m_tgtDef.iFlickerOn;
         }
         else
         {
            m_flickerState = FLICKER_OFF;
            m_flickerFramesLeft = m_tgtDef.iFlickerOff;
         }
      }
   }
}

/**
 Helper method for updateMotion(): Handles motion update tasks specific to RMV_GRATING and RMV_PLAID targets.

 For single-grating targets, by design, RMVTGTVEC.hPat is the position change of the grating along its drift axis and
 RMVTGTVEC.vPat is ignored. HOWEVER, if RMV_F_ORIENTADJ flag is set, then (hPat,vPat) represents the vector position
 change of the grating target. The grating's per-frame orientation is its initial orientation (from the target def)
 offset by the vector's direction. The grating's spatial phase is updated by calculating the projection of the pattern
 velocity vector onto the vector perpendicular to the per-frame orientation. If hPat=vPat=0, the vector direction is
 undefined and its amplitude is zero. In this case, the grating's orientation and spatial phase in the previous display
 frame are carried over into the current frame.

 For plaid targets, the two grating components may move independently, or they may act together to form a single
 unified plaid pattern -- depending upon the state of the RMV_F_INDEPGRATS flag. In the former case, RMVTGTVEC.hPat is
 treated as the position change of grating #1 along its drift axis, while RMVTGTVEC.vPat is the position change of
 grating #2 along its drift axis. In the latter case, (hPat,vPat) represents the vector position change of the plaid
 pattern as a whole; we must compute the position drift for each grating that will achieve this net change position of
 the plaid. If the RMV_F_ORIENTADJ flag is set (in which case RMV_F_INDEPGRATS is unset, as these features are
 incompatible!), then the component gratings' orientations are adjusted on a per-frame basis as described above for the
 single-grating case.

 Position drift of each grating is converted into a shift in the grating's spatial phase.
*/
void CRMVTarget::updatePlaid(PRMVTGTVEC pVec)
{
   int nGrats = (m_tgtDef.iType==RMV_GRATING) ? 1 : 2;
   bool adjustOri = ((m_tgtDef.iFlags & RMV_F_ORIENTADJ) == RMV_F_ORIENTADJ);
   bool singlePattern = (nGrats==2 && adjustOri) || ((m_tgtDef.iFlags & RMV_F_INDEPGRATS) == 0);

   if(adjustOri)
   {
      // grating orientation = defined orientation offset by angle(hPat,vPat), and phase is updated IAW the magnitude
      // of the projection of vector(hPat,vPat) onto ray perpendicular to grating. When hPat=vPat=0, orientation and
      // spatial phase from previous frame carry over to this frame.
      if(pVec->hPat == 0.0f && pVec->vPat == 0.0f) return;
      
      float fRad = float( ::sqrt(pVec->hPat*pVec->hPat + pVec->vPat*pVec->vPat) );
      float fDir = cMath::atan2Deg(pVec->vPat, pVec->hPat);
      for(int i=0; i<nGrats; i++)
      {
         m_fCurrOrient[i] = m_tgtDef.fDriftAxis[i] + fDir;
         float fAngle = fDir - m_fCurrOrient[i];
         float fShift = -fRad * cMath::cosDeg(fAngle) * 360.0f * m_tgtDef.fSpatialFreq[i];
         m_fCurrPhase[i] = cMath::limitToUnitCircleDeg(m_fCurrPhase[i] + fShift);
      }
   }
   else if((nGrats == 1) || !singlePattern) for(int i=0; i<nGrats; i++)
   {
      // grating orientation constant, and each grating moves independently: hPat is pos change along drift axis for 
      // grating 1, and vPat is the same for grating 2. Convert each to a phase shift and update the spatial phase 
      // of each grating accordingly.
      m_fCurrOrient[i] = m_tgtDef.fDriftAxis[i];
      float fShift = -1.0f * ((i==0) ? pVec->hPat : pVec->vPat) * 360.0f * m_tgtDef.fSpatialFreq[i];
      m_fCurrPhase[i] = cMath::limitToUnitCircleDeg(m_fCurrPhase[i] + fShift);
   }
   else for(int i=0; i<nGrats; i++)
   {
      // plaid moves as a single pattern; grating orientation does not adjust dynamically. Project the pattern pos 
      // change vector onto each grating's drift axis to determine how the gratings must move to give the appearance 
      // of a unified pattern motion.
      m_fCurrOrient[i] = m_tgtDef.fDriftAxis[i];
      float fRad = float( ::sqrt(pVec->hPat*pVec->hPat + pVec->vPat*pVec->vPat) );
      float fAngle = cMath::atan2Deg(pVec->vPat, pVec->hPat) - m_fCurrOrient[i];
      float fShift = -fRad * cMath::cosDeg(fAngle) * 360.0f * m_tgtDef.fSpatialFreq[i];
      m_fCurrPhase[i] = cMath::limitToUnitCircleDeg(m_fCurrPhase[i] + fShift);
   }
}

/**
 Helper method for updateMotion(): Handles motion update tasks specific to the RMV_RANDOMDOTS target.

 For details, see implementation notes for RMV_RANDOMDOTS in the file header.
*/
void CRMVTarget::updateRandomDots(float tElapsed, PRMVTGTVEC pVec)
{
   // which special features, if any, are enabled?
   bool bEnaCoherence = (m_tgtDef.iPctCoherent < 100);
   bool bEnaNoise = (m_tgtDef.iNoiseUpdIntv > 0 && m_tgtDef.iNoiseLimit > 0);
   bool bEnaDotLife = (m_tgtDef.fDotLife != 0.0f);

   // pattern velocity in polar form
   double dPatVecAmpl = 0.0;
   double dPatVecTheta = 0.0;

   // aperture outer/inner half-width, half-height
   float fOuterHalfW = m_tgtDef.fOuterW / 2.0f;
   float fOuterHalfH = m_tgtDef.fOuterH / 2.0f;
   float fInnerHalfW = m_tgtDef.fInnerW / 2.0f;
   float fInnerHalfH = m_tgtDef.fInnerH / 2.0f;

   // buffer pointers
   float* pfDots = m_pfBufDots->pBuf;
   float* pfDotNoise = bEnaNoise ? m_pfBufDotNoise->pBuf : NULL;
   float* pfDotLives = bEnaDotLife ? m_pfBufDotLives->pBuf : NULL;

   // if per-dot noise enabled: (1) calculate the polar form of pattern velocity vector, and (2) choose new random 
   // noise factor for each dot whenever the noise update interval expires. We do this even if the target is off 
   // and/or not moving!
   if(bEnaNoise) 
   { 
      dPatVecAmpl = ::sqrt(pVec->hPat * pVec->hPat + pVec->vPat * pVec->vPat);
      dPatVecTheta = cMath::atan2Deg(pVec->vPat, pVec->hPat);
      m_tUntilNoiseUpdate -= tElapsed;
      if(m_tUntilNoiseUpdate <= 0.0f)
      {
         m_tUntilNoiseUpdate += float(m_tgtDef.iNoiseUpdIntv);
         for(int i = 0; i < m_tgtDef.nDots; i++)
         {
            double dNoise = m_pNoiseRNG->generate();              // (0..1)
            dNoise *= 2.0 * double(m_tgtDef.iNoiseLimit);         // (0..2N), where N is the noise range limit
            dNoise -= double(m_tgtDef.iNoiseLimit);               // (-N..N)
            pfDotNoise[i] = float(dNoise);
         }
      }
   }

   // if finite dotlife enabled, determine the change in dotlife for this update -- either elasped time in ms or 
   // distance travelled in degrees. We do this even if the target is off.
   float fDotLifeDelta = 0.0f; 
   if(bEnaDotLife) 
   { 
      if(m_tgtDef.iFlags & RMV_F_LIFEINMS) fDotLifeDelta = tElapsed;
      else fDotLifeDelta = ::sqrt(pVec->hPat * pVec->hPat + pVec->vPat * pVec->vPat);
   }

   // UPDATE INDIVIDUAL DOT POSITIONS:
   CFPoint fpDot; 
   int xyIndex = 0;   // index into the per-dot vertex attributes array; stride of 4: {x,y,Tx,Ty} per dot
   bool bIsDirNoise = bEnaNoise && ((m_tgtDef.iFlags & RMV_F_DIRNOISE) != 0);
   bool bIsSpdLog2 = (!bIsDirNoise) && ((m_tgtDef.iFlags & RMV_F_SPDLOG2) != 0);
   bool bWrtScreen = ((m_tgtDef.iFlags & RMV_F_WRTSCREEN) != 0);
   
   // this factor is the expected value of 2^X, where X is a uniform r.v chosen over (-N..N). It is needed only in 
   // the implementation of multiplicative per-dot speed noise: Rdot = (Rpat * 2^X) / E(2^X).
   double log2Fac = 1.0;
   if(bIsSpdLog2)
   {
      log2Fac = pow(2.0, double(m_tgtDef.iNoiseLimit)) - pow(2.0, double(-m_tgtDef.iNoiseLimit));
      log2Fac /= 2 * double(m_tgtDef.iNoiseLimit) * log(2.0);
   }

   for(int i = 0; i < m_tgtDef.nDots; i++)
   {
      // flag gets set if dot position was randomized on this frame
      bool bWasDotLocRandomized = false;
      
      // if coherence feature in play, on each update we randomly select a percentage of dots to be randomly 
      // repositioned w/in target window
      if(bEnaCoherence) 
      { 
         double dTest = m_pDotRNG->generate() * 100.0; 
         if(dTest >= m_tgtDef.iPctCoherent)
         {
            bWasDotLocRandomized = true;
            randomizeDotPos(pfDots[xyIndex], pfDots[xyIndex+1]);
         }
      }

      // if finite dotlife in play, decrement the dot's current lifetime and randomly repos dot if its lifetime expired
      if(bEnaDotLife) 
      { 
         pfDotLives[i] -= fDotLifeDelta;
         if(pfDotLives[i] < 0.0f)
         {
            pfDotLives[i] = m_tgtDef.fDotLife;
            if(!bWasDotLocRandomized)
            {
               bWasDotLocRandomized = true;
               randomizeDotPos(pfDots[xyIndex], pfDots[xyIndex+1]);
            }
         }
      }

      // if dot was not already randomly repositioned above, then move it appropriately
      if(!bWasDotLocRandomized) 
      { 
         float fx = pfDots[xyIndex];
         float fy = pfDots[xyIndex+1];
         if(!bEnaNoise)
         {
            fx += pVec->hPat;
            fy += pVec->vPat;
         }
         else if(bIsDirNoise)
         {
            // for dir noise, offset pat vel theta by noise factor in deg; then calc Cartesian cmpts
            double dDir = dPatVecTheta + pfDotNoise[i];
            fx += float( dPatVecAmpl * cMath::cosDeg(dDir) );
            fy += float( dPatVecAmpl * cMath::sinDeg(dDir) );
         }
         else if(!bIsSpdLog2) 
         { 
            // for additive speed noise, offset pat vel R by a pct based noise factor
            double dAmp = dPatVecAmpl * pfDotNoise[i] / 100.0f;
            dAmp += dPatVecAmpl;
            fx += float( dAmp * cMath::cosDeg(dPatVecTheta) );
            fy += float( dAmp * cMath::sinDeg(dPatVecTheta) );
         }
         else
         { 
            // (as of v2.1.3) for multiplicative speed noise, Rdot = (R*2^X)/E, where E is the mean of 2^X when X is a
            // uniform r.v. in (-N..N)
            double dAmp = dPatVecAmpl*pow(2.0, pfDotNoise[i]);
            dAmp /= log2Fac;
            fx += float( dAmp * cMath::cosDeg(dPatVecTheta) );
            fy += float( dAmp * cMath::sinDeg(dPatVecTheta) );
         }
         
         // (as of v2.5.2) if target pattern displacement is WRT screen rather than target window, then we must 
         // convert to window frame of reference by subtracting the window displacement during this update.
         if(bWrtScreen)
         {
            fx -= pVec->hWin;
            fy -= pVec->vWin;
         }
         
         // The code below implements an algorithm for recycling a dot that has just moved out of "bounds", ie,
         // beyond the outer bounds of the aperture. The idea here is to relocate the dot in a sensible way so that
         // the target acts like a window on a random-dot pattern of infinite extent. Dots are "recycled" when they
         // leave the aperture's bounding rectangle (which is larger than the visible window for all apertures
         // except "rect"!). If the dot has just advanced past the right edge of the rectangle by X degrees, then the
         // algorithm here will "recycle" the dot X degrees left of the window's left edge, with the y-coord
         // randomized since we don't want the same pattern to "wrap" around the window edges.
         float fRem;
         if(cMath::abs(fx) > fOuterHalfW)
         {
            fRem = ::fmodf(cMath::abs(fx) - fOuterHalfW, fOuterHalfW);
            if((fx - pfDots[xyIndex]) > 0)
               fx = -fOuterHalfW + fRem;
            else
               fx = fOuterHalfW - fRem;

            fy = float( m_pDotRNG->generate() * m_tgtDef.fOuterH ) - fOuterHalfH;
         }
         else if(cMath::abs(fy) > fOuterHalfH)
         {
            fRem = ::fmodf(cMath::abs(fy) - fOuterHalfH, fOuterHalfH);
            if((fy - pfDots[xyIndex+1]) > 0)
               fy = -fOuterHalfH + fRem;
            else
               fy = fOuterHalfH - fRem;

            fx = float( m_pDotRNG->generate() * m_tgtDef.fOuterW ) - fOuterHalfW;
         }

         pfDots[xyIndex] = fx;
         pfDots[xyIndex+1] = fy;
      }

      xyIndex += 4;  // remember: stride is 4 b/c vertex array includes 4 attributes per dot: x,y,Tx,Ty
   }

   // update alpha component of each dot's RGBA color IAW target aperture: If dot is outside aperture, A=0, else
   // A=exp( -[x*x/(2*SX*SX) + y*y/(2*SY*SY)] ), where SX,SY are the standard deviations of the elliptical Gaussian
   // window in X and Y (if SX=SY=0, A=1).  There's one situation where we can A=1 for all dots always: for the
   // rectangular aperture when Gaussian window is disabled (rem that A was init'd to 1 for all dots).
   double dInvTwoSigSqX = (m_tgtDef.fSigma[0]>0.0f) ? -1.0/(2.0 * m_tgtDef.fSigma[0] * m_tgtDef.fSigma[0]) : 0.0f;
   double dInvTwoSigSqY = (m_tgtDef.fSigma[1]>0.0f) ? -1.0/(2.0 * m_tgtDef.fSigma[1] * m_tgtDef.fSigma[1]) : 0.0f;
   bool bDoGauss = (m_tgtDef.fSigma[0] > 0.0f || m_tgtDef.fSigma[1] > 0.0f);
   double dASq = fOuterHalfW*fOuterHalfW;
   double dBSq = fOuterHalfH*fOuterHalfH;
   double dCSq = fInnerHalfW*fInnerHalfW;
   double dDSq = fInnerHalfH*fInnerHalfH;
   if(m_tgtDef.iAperture != RMV_RECT || bDoGauss) for( int i=0; i<m_tgtDef.nDots; i++ )
   {
      double x = pfDots[4*i];
      double y = pfDots[4*i+1];

      // "inside aperture" test: A=1 for inside, A=0 otherwise.  Note that all dots are inside the RMV_RECT aperture!
      bool isInside = false;
      switch( m_tgtDef.iAperture )
      {
         case RMV_RECT :
            isInside = true;
            break;
         case RMV_RECTANNU :
            if((fabs(x)>fInnerHalfW) || (fabs(y)>fInnerHalfH))
               isInside = true;
            break;
         case RMV_OVAL :
            if( x*x/dASq + y*y/dBSq <= 1.0 )
               isInside = true;
            break;
         case RMV_OVALANNU :
            if( (x*x/dASq + y*y/dBSq <= 1.0) && (x*x/dCSq + y*y/dDSq > 1.0) )
               isInside = true;
            break;
      }

      // if dot is outside aperture, A=0.  Else, if Gaussian std dev > 0, then compute A accordingly
      // NOTE: We store each dot's alpha in the vertex attribute "Tx", which otherwise represents the X-coordinate of 
      // the texel location. We don't use an alpha texture with RMV_RANDOMDOTS. "Ty" is unused.
      if(!isInside) pfDots[4*i+2] = 0.0f;
      else if(!bDoGauss) pfDots[4*i+2] = 1.0f;
      else pfDots[4*i+2] = (float) cMath::rangeLimit(exp(x*x*dInvTwoSigSqX + y*y*dInvTwoSigSqY), 0.0, 1.0);
   }

   // upload vertex data to the dedicated segment in the OpenGL shared vertex array
   m_pRenderer->uploadVertexData(m_vtxArrayStart, m_vtxArrayCount, pfDots);
}

/**
 Helper method for updateMotion(): Handles motion update tasks specific to the RMV_FLOWFIELD target.

 For details, see implementation notes for RMV_FLOWFIELD in the file header.
*/
void CRMVTarget::updateFlowField(PRMVTGTVEC pVec)
{
   // hPat = deltaR of dot at 0.5*outerRad; from this get animation factor "B"
   float outerR = m_tgtDef.fOuterW;
   float innerR = m_tgtDef.fInnerW;
   float B = (float) (pVec->hPat / cMath::sincosDeg(0.5f*outerR)); 

   // empirically determined dot recycle rate for decelerating flows. It's proportional to B and limited to [0.001 ..
   // 0.4] out of (0..1).
   double dRecycleRate = cMath::rangeLimit(cMath::abs(B) / 30.0, 0.001, 0.4);
   
   // in decel flows (B neg), dots are recycled in in a radial ring of this size.
   float fRecycleDR = (float) (cMath::abs(B)*cMath::sincosDeg(outerR));      

   float* pfDots = m_pfBufDots->pBuf;

   // DECELERATING FLOWS: For each dot...
   if(pVec->hPat < 0) for(int i=0; i<m_tgtDef.nDots; i++)            // DECELERATING FLOWS...for each dot:
   {
      // calculate current polar coordinates [rem: 4 attributes per dot - x,y,Tx,Ty. Tx,Ty never change]
      int j = 4*i;
      float r = ::sqrt(pfDots[j]*pfDots[j] + pfDots[j+1]*pfDots[j+1]);
      float theta = cMath::atan2Deg(pfDots[j+1], pfDots[j]);

      // update dot radius IAW algorithm
      r += (float) (B*cMath::sincosDeg(r));

      // if radius is less than this random R, AND the next random# is <= empirical recycle rate, then recycle dot.
      // We always recycle dots that get too close to the FOE.
      float fRecycleBand = innerR + float(m_pDotRNG->generate()) * (outerR - innerR);
      bool bRecycle = (r < fRecycleBand) && (m_pDotRNG->generate() <= dRecycleRate);
      bRecycle = bRecycle || (r < innerR);

      // if dot is recycled, randomly choose new polar coords, with R restricted to a ring near outer edge of flow
      if(bRecycle)
      {
         theta = 360.0f * ((float) m_pDotRNG->generate());
         r = outerR - fRecycleDR * ((float)m_pDotRNG->generate());
      }

      // update dot (x,y) from new polar coordinates (theta,r)
      pfDots[j] = r*cMath::cosDeg(theta); 
      pfDots[j+1] = r*cMath::sinDeg(theta);
   }

   // ACCELERATING FLOWS: For each dot...
   else for(int i=0; i<m_tgtDef.nDots; i++)
   {
      // calculate current polar coordinates [rem: 4 attributes per dot - x,y,Tx,Ty. Tx,Ty never change]
      int j = 4*i; 
      float r = ::sqrt(pfDots[j]*pfDots[j] + pfDots[j+1]*pfDots[j+1]);
      float theta = cMath::atan2Deg(pfDots[j+1], pfDots[j]);

      // update dot radius IAW algorithm
      r += (float) (B*cMath::sincosDeg(r));

      // if still inside field, calc (x,y) coordinates from the new polar coordinates; else, "recycle" the dot by
      // repositioning it anywhere in the flow field
      if(r < outerR)
      {
         pfDots[j] = r*cMath::cosDeg(theta);
         pfDots[j+1] = r*cMath::sinDeg(theta);
      }
      else randomizeDotPosInFlowField(pfDots[j], pfDots[j+1]);
   }

   // upload vertex data to the dedicated segment in the OpenGL shared vertex array
   m_pRenderer->uploadVertexData(m_vtxArrayStart, m_vtxArrayCount, pfDots);
}

/**
 Helper method for updateMotion(): Handles motion update tasks specific to the RMV_MOVIE target.

 A video streamer object, CVidBuffer, streams and buffers all video sources. The RMV_MOVIE target maintains a reference
 to the video stream ID returned when the streamer opened the target's video file. Streaming and buffering occurs on a
 background thread to maximize performance. Ideally, the next frame will already be buffered and ready for retrieval 
 each time a new frame is needed. If not, this method will BLOCK until the frame is ready -- which will likely lead to
 a duplicate frame error in the ongoing animation. It is also possible that an error occurred while reading in a frame,
 in which case the video stream is disabled. This is considered a fatal error; the ongoing animation sequence should be
 terminated.

 For further details, see implementation notes for RMV_MOVIE in the file header.

 @return True if successful, false if a fatal error occurred while streaming frames from the video source.
*/
bool CRMVTarget::updateMovie(float tElapsed, PRMVTGTVEC pVec)
{
   // movie playback doesn't start until target is turned on.
   if(m_iMovieState == MOVIE_NOTSTARTED && isOn()) m_iMovieState = MOVIE_NEEDFRAME;
   
   // once movie playback has begun, we get the next frame on every call. This means the movie will play at the frame
   // rate of the monitor -- assuming we can keep up! We get the next frame even if the target is off -- meaning the
   // movie continues without being displayed -- UNLESS the pause-while-off flag is set.
   bool paused = (!isOn()) && ((m_tgtDef.iFlags & RMV_F_PAUSEWHENOFF) == RMV_F_PAUSEWHENOFF);
   if(paused || m_iMovieState != MOVIE_NEEDFRAME) return(true);

   // if we're playing movie at the playback rate specified in the video file, update our elapsed time and see if it
   // is really time for the next frame. If not, we revert to MOVIE_GOTFRAME -- so the last retrieved frame is drawn
   // again on the next monitor refresh. If we're playing at the monitor refresh rate, we retrieve a frame always.
   if(m_tPlaybackIntv > 0)
   {
      m_tElapsed += tElapsed;
      if(m_tElapsed < m_tNextFrame)
      {
         m_iMovieState = MOVIE_GOTFRAME;
         return(true);
      }
      else
         m_tNextFrame += m_tPlaybackIntv;
   }

   // when the movie does NOT loop, it should stop once the last frame is drawn. When EOF is reached, that last frame
   // is in the PBO slot and is uploaded to the texture but not yet drawn. The "gotLastFrame" flag is set at this point.
   // When a new frame is requested and the flag is set, then we know the last frame is drawn and the movie is "done"
   if(m_gotLastFrame)
   {
      m_iMovieState = MOVIE_DONE;
      return(true);
   }

   // get next frame from video streamer. BLOCK if a frame is not ready, UNLESS we've reached EOF and movie is not
   // configured to repeat indefinitely. FAIL if video stream disabled.
   struct timespec tSpec;
   tSpec.tv_sec = (time_t) 0;
   tSpec.tv_nsec = 100000;
   uint8_t* pDstBuf = NULL;
   do
   {
      pDstBuf = m_pRenderer->m_vidBuffer.getCurrentFrameData(m_videoStreamID);
      if(pDstBuf == NULL)
      {
         if(m_pRenderer->m_vidBuffer.isVideoDisabled(m_videoStreamID))
         {
            m_iMovieState = MOVIE_DONE;
            return(false);
         }
         else if(((m_tgtDef.iFlags & RMV_F_REPEAT) == 0) && m_pRenderer->m_vidBuffer.gotEOF(m_videoStreamID))
         {
            // at this point, the last video frame is loaded into the current PBO slot, but it needs to be 
            // uploaded to the texture and drawn. Here we just set a flag to indicate we got the last frame
            m_gotLastFrame = true;
            break;
         }
         ::nanosleep(&tSpec, NULL);
      }
   } 
   while(pDstBuf == NULL);

   // upload frame data in current PBO slot to the assigned GL texture.
   int w = m_pRenderer->m_vidBuffer.getVideoWidth(m_videoStreamID);
   int h = m_pRenderer->m_vidBuffer.getVideoHeight(m_videoStreamID);
   int nBytes = m_pRenderer->m_vidBuffer.getVideoFrameSize(m_videoStreamID);

   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIDs[m_iCurrPBOIdx]);
   m_pRenderer->uploadMovieFrameToTexture(m_texID, w, h, NULL);

   // if the movie does not loop and we detected EOF, then the last frame has now been queued for upload and
   // there are no more frames to get from the video stream
   if(m_gotLastFrame)
   {
      m_iMovieState = MOVIE_GOTFRAME;
      return(true);
   }

   // copy the frame just retrieved to the next PBO in the round-robin PBO queue, and advance streamer
   m_iCurrPBOIdx = (m_iCurrPBOIdx+1) % NUMPBOS;
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIDs[m_iCurrPBOIdx]);
   uint8_t* pPBO = (uint8_t*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
   if(pPBO == NULL)
   {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      ::fprintf(stderr, "ERROR(CRMVTarget): Unable to map pixel buffer object\n");
      return(false);
   }
   ::memcpy(pPBO, pDstBuf, nBytes);
   glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   m_pRenderer->m_vidBuffer.advanceToNextFrame(m_videoStreamID);

   m_iMovieState = MOVIE_GOTFRAME;
   return(true);
}

/**
 Render the target IAW its current state. If the target is not ON, then nothing is rendered.

 The RMVideo render manager, CRMVRenderer, uses a single shader program for all rendering. A single texture unit is 
 always bound to texture unit 0, with blending enabled with glBlendFunc(src_alpha, 1-src_alpha) -- although for some
 targets the texture is the trivial 4x4 "no-op" alpha=1 mask. A single vertex array is shared among all participating
 targets. This array is allocated at startup. Most targets use one of four simple, unchanging primitives that are 
 defined in the array at startup. Only the dot targets (RMV_RANDOMDOTS and _FLOWFIELD) must stream per-dot vertex 
 attributes to the array prior to rendering. During target initialization, a dot target reserves a contiguous segment
 of the array for its exclusive use. Then, during the "update motion" phase of a display frame, the per-dot vertex 
 attributes are recalculated for that frame and uploaded to that reserved segment. All that remains to do in the "draw"
 phase is to render the vertices in that segment IAW the target's properties.

 The idea behind this rendering approach is to minimize the number of OpenGL state changes that have to take place
 during an animation sequence. The only OpenGL state that changes from target to target are the values of certain
 uniform variables passed to the shader program, the ID of the bound texture object, and the kind of primitive(s) 
 rendered.

 @param eye For "stereo experiments" using dots targets, the target's stereo dot disparity is multiplied by this factor
 to compute a horizontal offset in the target's position. Applicable ONLY to RMV_POINT, RMV_RANDOMDOTS, and
 RMV_FLOWFIELD. Typical usage during a stereo experiment: eye = -0.5 while drawing a dots target in the GL_LEFT
 backbuffer, and eye = +0.5 while drawing a dots target in the GL_RIGHT backbuffer. No offset if eye = 0 or the target's
 dot disparity is zero.
*/
void CRMVTarget::draw(float eye)
{
   // do nothing if target is OFF -- or if it's a movie target and we didn't get the next frame
   if(!isOn()) return;
   if(m_tgtDef.iType==RMV_MOVIE && m_iMovieState != MOVIE_GOTFRAME) return;

   // set up uniform variables accessed in the vertex or fragment shaders
   bool isPts = (m_tgtDef.iType==RMV_POINT) || (m_tgtDef.iType==RMV_RANDOMDOTS) || (m_tgtDef.iType==RMV_FLOWFIELD);
   bool isLine = (m_tgtDef.iType==RMV_BAR) && (m_tgtDef.fOuterW <= 0.0f);
   m_pRenderer->updateCommonUniforms(m_tgtDef.iType,
      m_centerPt.GetH() + (isPts ? eye * m_tgtDef.fDotDisp : 0.0f), m_centerPt.GetV(),
      isLine ? 1.0f : (isPts ? 0.0f : m_tgtDef.fOuterW), isPts ? 0.0f : m_tgtDef.fOuterH,
      m_tgtDef.iType == RMV_BAR ? m_tgtDef.fDriftAxis[0] : 0.0f);
   m_pRenderer->updateTargetColorUniform(m_rgb0[0], m_rgb0[1], m_rgb0[2]);

   if(m_tgtDef.iType==RMV_GRATING || m_tgtDef.iType==RMV_PLAID)
      m_pRenderer->updateGratingUniforms(m_centerPt.GetH(), m_centerPt.GetV(), (m_tgtDef.iFlags & RMV_F_ISSQUARE) == 0,
         m_rgb0, m_rgbCon0, m_rgb1, m_rgbCon1, m_fCurrOrient, m_fSpatialPerX, m_fSpatialPerY, m_fCurrPhase);

   // bind the appropriate texture, set the point size (for dot targets), and draw the primitives (which are fixed
   // primitives in the shared array for most target types, else a set of point primitives that were updated in the
   // shared array during the last updateMotion() call. For the RMV_RANDOMDOTS two-color contrast mode, half the dots
   // are drawn in one color, and the other half in the second color.
   m_pRenderer->bindTextureObject(m_texID);
   if(isPts) m_pRenderer->setPointSize(m_tgtDef.nDotSize);
   
   if(!m_isTwoColor)
      m_pRenderer->drawPrimitives(isPts, isLine, m_vtxArrayStart, m_vtxArrayCount);
   else
   {
      int n = m_vtxArrayCount / 2;
      m_pRenderer->drawPrimitives(true, false, m_vtxArrayStart, n);
      m_pRenderer->updateTargetColorUniform(m_rgb1[0], m_rgb1[1], m_rgb1[2]);
      m_pRenderer->drawPrimitives(true, false, m_vtxArrayStart+n, m_vtxArrayCount - n);
   }

   // for movie target, once we draw a video frame, we need the next one!
   if(m_tgtDef.iType==RMV_MOVIE && m_iMovieState == MOVIE_GOTFRAME)
      m_iMovieState = MOVIE_NEEDFRAME;
}

/**
 Is this target currently displayed?  Note that any RMVideo target should still update its animation state every frame
 even if it is not displayed.

 A target is displayed if its "on" flag is set AND: (i) flicker is disabled, or (ii) the target is in the "ON" phase of
 its flicker cycle.

 @return True if target is currently displayed, false otherwise.
*/
bool CRMVTarget::isOn() { return(m_bOn && (m_flickerState==FLICKER_DISABLED || m_flickerState==FLICKER_ON)); }

/**
 Show/hide this CRMVTarget. The target's flicker state is reset whenever its ON/off flag is toggled. This ensures that
 the flicker cycle is restarted each time the target is toggled on.

 @param bOn [in] True to turn target on; false to turn it off.
*/
void CRMVTarget::setOn(bool bOn)
{ 
   if((m_flickerState != FLICKER_DISABLED) && (m_bOn != bOn)) m_flickerState = FLICKER_RESET;
   m_bOn = bOn;
}

/**
 Helper method for initialize(): Validate target definition and range-limit selected parameters IAW target type.
 @return False if target definition is invalid such target cannot be rendered.
*/
bool CRMVTarget::validateTargetDef()
{
   if(m_tgtDef.iType < 0 || m_tgtDef.iType >= RMV_NUMTGTTYPES)
   {
      fprintf(stderr, "ERROR(CRMVTarget): Bad target type (%d)\n", m_tgtDef.iType);
      return(false);
   }
   int t = m_tgtDef.iType;

   m_tgtDef.iAperture = cMath::rangeLimit(m_tgtDef.iAperture, RMV_RECT, RMV_OVALANNU);
   if(t==RMV_GRATING || t==RMV_PLAID)
   {
      if(m_tgtDef.iAperture != RMV_RECT && m_tgtDef.iAperture != RMV_OVAL)
      {
         fprintf(stderr, "ERROR(CRMVTarget): Grating/plaid does not support annular aperture\n");
         return(false);
      }
   }

   // target bounding rectangle, and inner dimensions for annular apertures. RMV_BAR is a special case...
   if(t==RMV_BAR)
   {
      // one (but not both!) of the outer dimensions can be zero, thereby defining a line instead of a bar.
      m_tgtDef.fOuterW = cMath::abs(m_tgtDef.fOuterW); 
      m_tgtDef.fOuterH = cMath::abs(m_tgtDef.fOuterH);
      if(m_tgtDef.fOuterW <= 0.0f && m_tgtDef.fOuterH <= 0.0f) 
      {
         fprintf(stderr, "ERROR(CRMVTarget): Bar cannot be zero-width and zero-height\n");
         return(false);
      }

      // internally, we require H >= W; if not, swap them and adjust bar's drift axis accordingly
      if(m_tgtDef.fOuterW > m_tgtDef.fOuterH) 
      {
         float fTemp = m_tgtDef.fOuterW;
         m_tgtDef.fOuterW = m_tgtDef.fOuterH;
         m_tgtDef.fOuterH = fTemp;
         m_tgtDef.fDriftAxis[0] += 90.0f;
      }

      // limit drift axis to [0..360)
      m_tgtDef.fDriftAxis[0] = cMath::limitToUnitCircleDeg(m_tgtDef.fDriftAxis[0]);
   }
   else
   {
      // otherwise, range limit outer and inner window dimensions
      m_tgtDef.fOuterW = cMath::rangeLimit(m_tgtDef.fOuterW, RMV_MINRECTDIM, RMV_MAXRECTDIM);
      m_tgtDef.fOuterH = cMath::rangeLimit(m_tgtDef.fOuterH, RMV_MINRECTDIM, RMV_MAXRECTDIM);
      m_tgtDef.fInnerW = cMath::rangeLimit(m_tgtDef.fInnerW, RMV_MINRECTDIM, RMV_MAXRECTDIM);
      m_tgtDef.fInnerH = cMath::rangeLimit(m_tgtDef.fInnerH, RMV_MINRECTDIM, RMV_MAXRECTDIM);

      // for annular apertures, inner dimension must be less than outer dimension!
      if((t==RMV_SPOT || t==RMV_RANDOMDOTS) && (m_tgtDef.iAperture==RMV_RECTANNU || m_tgtDef.iAperture==RMV_OVALANNU))
      {
         if(m_tgtDef.fInnerW >= m_tgtDef.fOuterW || m_tgtDef.fInnerH >= m_tgtDef.fOuterH)
         {
            fprintf(stderr, "ERROR(CRMVTarget): Invalid annular aperture (inner dim exceeds outer)\n");
            return(false);
         }
      }

      // for RMV_FLOWFIELD, the inner radius must be less than outer radius!
      if((t==RMV_FLOWFIELD) && (m_tgtDef.fInnerW >= m_tgtDef.fOuterW))
      {
         fprintf(stderr, "ERROR(CRMVTarget): Inner radius >= outer radius for flowfield target\n");
         return(false);
      }
   }

   // these 3 params apply only to the RMV_RANDOMDOTS and _FLOWFIELD targets; dot size applies to RMV_POINT.
   m_tgtDef.nDots = cMath::rangeLimit(m_tgtDef.nDots, 1, RMV_MAXNUMDOTS);
   m_tgtDef.nDotSize = cMath::rangeLimit(m_tgtDef.nDotSize, RMV_MINDOTSIZE, RMV_MAXDOTSIZE);
   m_tgtDef.iSeed = (m_tgtDef.iSeed == 0) ? 1 : m_tgtDef.iSeed;

   // these next 4 params apply only to RMV_RANDOMDOTS
   m_tgtDef.iPctCoherent = cMath::rangeLimit(m_tgtDef.iPctCoherent, 0, 100);
   m_tgtDef.iNoiseUpdIntv = (m_tgtDef.iNoiseUpdIntv < 0) ? 0 : m_tgtDef.iNoiseUpdIntv;
   int iMinNoise = RMV_MINNOISELIMIT;
   int iMaxNoise = RMV_MAXNOISEDIR;
   if((m_tgtDef.iFlags & RMV_F_DIRNOISE) == 0) 
   {
      if((m_tgtDef.iFlags & RMV_F_SPDLOG2) == 0)
         iMaxNoise = RMV_MAXNOISESPEED;
      else { iMinNoise = RMV_MINSPDLOG2; iMaxNoise = RMV_MAXSPDLOG2; }
   }
   m_tgtDef.iNoiseLimit = cMath::rangeLimit(m_tgtDef.iNoiseLimit, iMinNoise, iMaxNoise);
   m_tgtDef.fDotLife = (m_tgtDef.fDotLife < 0.0f) ? 0.0f : m_tgtDef.fDotLife;

   // std dev in X,Y for Gaussian blur: applicable to RMV_SPOT, _RANDOMDOTS, _GRATING, and _PLAID (0 = no blur)
   for(int i=0; i<2; i++) m_tgtDef.fSigma[i] = (m_tgtDef.fSigma[i] < 0.0f) ? 0.0f : m_tgtDef.fSigma[i];

   // these next 3 params apply only to RMV_GRATING and _PLAID targets -- except that RMV_BAR uses fDriftAxis[0]
   // (which we handled above)
   int nGrats = (t==RMV_GRATING) ? 1 : (t==RMV_PLAID ? 2 : 0);
   for(int i=0; i<nGrats; i++)
   {
      if(m_tgtDef.fSpatialFreq[i] <= 0.0f)
      {
         fprintf(stderr, "ERROR(CRMVTarget): Grating spatial frequency must be greater than 0\n");
         return(false);
      }

      // limit drift axis and spatial phase to [0..360) deg
      m_tgtDef.fDriftAxis[i] = cMath::limitToUnitCircleDeg(m_tgtDef.fDriftAxis[i]);     
      m_tgtDef.fGratPhase[i] = cMath::limitToUnitCircleDeg(m_tgtDef.fGratPhase[i]); 
   }

   // these last two params specify the folder and filename for the source file for RMV_IMAGE or RMV_MOVIE. Here
   // we validate the strings IAW RMVideo naming rules
   if(t==RMV_IMAGE || t==RMV_MOVIE)
   {
      int len = ::strlen(m_tgtDef.strFolder);
      if(len <= 0 || len > RMV_MVF_LEN || len != ::strspn(m_tgtDef.strFolder, RMV_MVF_CHARS))
      {
         fprintf(stderr, "ERROR(CRMVTarget): Invalid media folder name '%s'\n", m_tgtDef.strFolder);
         return(false);
      }
      len = ::strlen(m_tgtDef.strFile);
      if(len <= 0 || len > RMV_MVF_LEN || len != ::strspn(m_tgtDef.strFile, RMV_MVF_CHARS))
      {
         fprintf(stderr, "ERROR(CRMVTarget): Invalid media file name '%s'\n", m_tgtDef.strFile);
         return(false);
      }
   }

   // the flicker parameters apply to all target types
   m_tgtDef.iFlickerOn = cMath::rangeLimit(m_tgtDef.iFlickerOn, RMV_MINFLICKERDUR, RMV_MAXFLICKERDUR);
   m_tgtDef.iFlickerOff = cMath::rangeLimit(m_tgtDef.iFlickerOff, RMV_MINFLICKERDUR, RMV_MAXFLICKERDUR);
   m_tgtDef.iFlickerDelay = cMath::rangeLimit(m_tgtDef.iFlickerDelay, RMV_MINFLICKERDUR, RMV_MAXFLICKERDUR);
   m_flickerState = (m_tgtDef.iFlickerOn > 0 && m_tgtDef.iFlickerOff > 0) ? FLICKER_RESET : FLICKER_DISABLED;

   return(true);
}

/**
 Helper method for initialize(): Convert target color(s) from packed RGB format to normalized R,G,B components.

 The RMV_POINT, _BAR, _SPOT and _FLOWFIELD targets define a single target color. The RMV_RANDOMDOTS target defines one
 or two colors -- with two distinct colors in the "two-color constrast mode". The RMV_GRATING and RMV_PLAID targets
 define a color range via a mean and contrast for each color component. For the remaining target types, target color 
 does not apply (RMV_IMAGE, _MOVIE).
*/
void CRMVTarget::initTargetColors()
{
   int type = m_tgtDef.iType;
   if(type==RMV_IMAGE || type==RMV_MOVIE) return;

   int packedRGBCon = m_tgtDef.iRGBCon[0] & 0x00FFFFFF;
   int packedRGB = m_tgtDef.iRGBMean[0] & 0x00FFFFFF;
   if(type == RMV_GRATING || type == RMV_PLAID)
   {
      // unpack both RGB mean and contrast for first grating
      for(int i=0; i<3; i++) 
      {
         m_rgb0[i] = double(0x00FF & packedRGB) / 255.0;
         packedRGB = (packedRGB >> 8);
         m_rgbCon0[i] = cMath::rangeLimit( double(0x00FF & packedRGBCon) / 100.0, 0.0, 1.0);
         packedRGBCon = (packedRGBCon >> 8);
      }

      // for RMV_PLAID, repeat for second grating
      if(type == RMV_PLAID)
      {
         packedRGBCon = m_tgtDef.iRGBCon[1] & 0x00FFFFFF;
         packedRGB = m_tgtDef.iRGBMean[1] & 0x00FFFFFF;
         for(int i=0; i<3; i++) 
         {
            m_rgb1[i] = double(0x00FF & packedRGB) / 255.0;
            packedRGB = (packedRGB >> 8);
            m_rgbCon1[i] = cMath::rangeLimit( double(0x00FF & packedRGBCon) / 100.0, 0.0, 1.0);
            packedRGBCon = (packedRGBCon >> 8);
         }
      }
   }
   else if(type != RMV_RANDOMDOTS || packedRGBCon == 0)
   {
      // single target color defined in TGTDEF.iRGBMean[0]
      for(int i=0; i<3; i++) 
      {
         m_rgb0[i] = m_rgb1[i] = double(0x00FF & packedRGB) / 255.0;
         packedRGB = (packedRGB >> 8);
      }
   }
   else
   {
      // RMV_RANDOMDOTS in two-color contrast mode: The first N/2 dots are assigned to "color0", while the second N/2 
      // dots are rendered in "color1".
      m_isTwoColor = true;

      // "color0": Lmax = M(1+C)
      for(int i=0; i<3; i++)
      {
         int lum = (int)(0x00FF & packedRGB);
         lum = (lum * (100 + ((int)(0x00FF & packedRGBCon)))) / 100;
         if(lum > 255) lum = 255;
         m_rgb0[i] = double(0x00FF & lum) / 255.0;

         packedRGB = (packedRGB >> 8);
         packedRGBCon = (packedRGBCon >> 8);
      }

      // "color1": Lmin = M(1-C)
      packedRGBCon = m_tgtDef.iRGBCon[0] & 0x00FFFFFF;
      packedRGB = m_tgtDef.iRGBMean[0] & 0x00FFFFFF;
      for(int i=0; i<3; i++)
      {
         int lum = (int)(0x00FF & packedRGB);
         lum = (lum * (100 - ((int)(0x00FF & packedRGBCon)))) / 100;
         lum = (lum < 0) ? 0 : (lum > 255 ? 255 : lum);
         m_rgb1[i] = double(0x00FF & lum) / 255.0;

         packedRGB = (packedRGB >> 8);
         packedRGBCon = (packedRGBCon >> 8);
      }
   }
}

/**
 Helper method for initialize(): Initialize any additional state information relevant to the target type implemented,
 and allocate any additional OpenGL resources required to render the target.

 1) Alpha mask texture. RMV_SPOT, _GRATING, _PLAID and _RANDOMDOTS all support a non-rectangular aperture and a 2D
 Gaussian blur. Both aperture and blur are implemented by an alpha mask texture object generated here -- but NOT for
 the RMV_RANDOMDOTS target.

 For RMV_RANDOMDOTS, each dot is either inside or outside the aperture, regardless the dot size; it cannot be partly
 obscured at the aperture boundary). The per-dot alpha component is computed every frame based on the dot's location,
 which typically changes during an animation. That alpha component is transmitted to the fragment shader via the
 vertex attribute "Tx", the X-coordinate of the dot's texel location in the applied texture -- which will be the
 default "alpha=1" mask. In the fragment shader, the result of the texturing operation is ignored, and the fragment
 alpha is set to the value of Tx. Note that, if the aperture is RMV_RECT and there's no Gaussian window, then all
 dots have alpha = 1 always.

 2) RMV_IMAGE. An RGBA texture object is allocated and loaded with the image data here.

 3) RMV_MOVIE. The video source file is opened and all of the infrastructure for streaming video frames from the file
 is prepared. An RGB texture object is also allocated here to hold each movie frame image streamed from the file.
 Video streaming is handled by a utility class, CVidBuffer, on a background thread. When the video stream is opened
 here, a stream ID is assigned. CRMVTarget uses that ID to retrieve frames during playback. In addition, to get 
 better performance uploading frame data to the texture object, we employ a round-robin queue of pixel buffer objects.
 During frame N, we copy pixel data for frame N+1 to a PBO and while the data from frame N is being uploaded from 
 another PBO to the texture object.

 4) Segment of shared vertex array in which target's vertices are stored. For all except the two random-dot target
 types, the target primitive is extremely simple: a single quad, a line segment, or a single point. CRMVRenderer 
 allocates a shared vertex array and backing buffer at startup that is large enough to hold 50K vertices. It stores 
 these fixed target primitives at the beginning of the array. The array segment defining a particular fixed primitive 
 is identified by a start index and vertex count, which are exposed as public constants of CRMVRenderer. Each vertex 
 has two attributes -- normalized location (x,y) and corresponding texel location (Tx, Ty) -- as required by the 
 monolithic OpenGL shader program that handles all rendering in RMVideo. The attributes of the fixed primitives never
 change, so they can be shared by multiple targets during an animation sequence. A target-specific transform converts
 each normalized vertex location to its rendered location in the vertex shader.

 5) For RMV_RANDOMDOTS and RMV_FLOWFIELD, the target must query CRMVRenderer for a dedicated segment (start index and
 count) of the shared vertex array in which the individual dot attributes are stored. Furthermore, internal arrays
 are allocated to store per-dot vertex attributes (and, for RMV_RANDOMDOTS, possibly per-dot lifetimes and noise 
 factors). In updateMotion(), the vertex attributes are updated every frame and uploaded to the dedicated segment in
 the shared vertex array.

 6) For RMV_RANDOMDOTS and RMV_FLOWFIELD, the pseudo random number generator used to randomize dot locations is 
 created and initialized here. Also, a second RNG is prepared for any RMV_RANDOMDOTS target using the dot speed or
 direction noise feature.

 7) For RMV_GRATING and RMV_PLAID, the spatial period of each component grating is calculated. If the spatial period
 is too small, the target cannot be rendered and this method fails with an appropriate error message.

 @return True if resource allocation was successful; false otherwise. In the event of an error, a brief error
 message is written to the console via fprintf(stderr, ...).
*/
bool CRMVTarget::allocateResources()
{
   int t = m_tgtDef.iType;

   // for all but the dot patch target types, attach to the segment of the shared vertex array in which the appropriate
   // fixed primitive is defined
   switch(t)
   {
   case RMV_POINT: 
      m_vtxArrayStart = CRMVRenderer::POINTINDEX; 
      m_vtxArrayCount = CRMVRenderer::POINTCOUNT;
      break;
   case RMV_BAR:
      m_vtxArrayStart = (m_tgtDef.fOuterW <= 0.0f) ? CRMVRenderer::VLINEINDEX : CRMVRenderer::QUADINDEX;
      m_vtxArrayCount = (m_tgtDef.fOuterW <= 0.0f) ? CRMVRenderer::VLINECOUNT : CRMVRenderer::QUADCOUNT;
      break;
   case RMV_SPOT:
   case RMV_GRATING:
   case RMV_PLAID:
   case RMV_IMAGE:
      m_vtxArrayStart = CRMVRenderer::QUADINDEX; 
      m_vtxArrayCount = CRMVRenderer::QUADCOUNT;
      break;
   case RMV_MOVIE: 
      m_vtxArrayStart = CRMVRenderer::VIDQUADINDEX; 
      m_vtxArrayCount = CRMVRenderer::VIDQUADCOUNT;
      break;
   default:
      break;
   }

   // for the target types that define a non-rectangular aperture or Gaussian blur, prepare the alpha mask texture.
   // REM: RMV_RANDOMDOTS calculates per-dot alpha and transmits to fragment shader via vertex attribute "Tx"; it
   // does not use the alpha mask texture.
   bool needAlphaMask = m_tgtDef.iAperture != RMV_RECT || m_tgtDef.fSigma[0] > 0.0f || m_tgtDef.fSigma[1] > 0.0f;
   if(needAlphaMask && (t==RMV_SPOT || t==RMV_GRATING || t==RMV_PLAID))
   {
      m_texID = m_pRenderer->prepareAlphaMaskTexture(m_tgtDef.iAperture, m_tgtDef.fOuterW, m_tgtDef.fOuterH, 
            m_tgtDef.fInnerW, m_tgtDef.fInnerH, m_tgtDef.fSigma[0], m_tgtDef.fSigma[1]);
      if(m_texID == 0)
      {
         ::fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate and load alpha mask texture\n");
         return(false);
      }
   }

   // for grating/plaid targets, we need to set up some additional runtime state. We also need to make sure that
   // the spatial period is not too small (too few pixels per cycle)
   if(t==RMV_GRATING || t==RMV_PLAID)
   {
      int nGrats = (t==RMV_GRATING) ? 1 : 2;
      float pixPerDegX = (float) m_pRenderer->degToPixels(true, 1.0);
      float pixPerDegY = (float) m_pRenderer->degToPixels(false, 1.0);
      for(int i=0; i<nGrats; i++)
      {
         m_fSpatialPerX[i] = pixPerDegX / m_tgtDef.fSpatialFreq[i];
         m_fSpatialPerY[i] = pixPerDegY / m_tgtDef.fSpatialFreq[i];
         if((m_fSpatialPerX[i] < float(MINGRATCYCLE)) || (m_fSpatialPerY[i] < float(MINGRATCYCLE)))
         {
            fprintf(stderr, "ERROR(CRMVTarget): Grating spatial period less than %d pixels is not supported\n",
                  MINGRATCYCLE);
            return(false);
         }

         m_fCurrOrient[i] = m_tgtDef.fDriftAxis[i];
         m_fCurrPhase[i] = m_tgtDef.fGratPhase[i];
      }
   }

   // RMV_RANDOMDOTS, _FLOWFIELD: Acquire internal buffer maintaining current per-dot vertex attributes. Request
   // section of shared GPU-side vertex array to which vertex attributes are streamed during animation. Create and
   // initialize any random-number generators needed. For _RANDOMDOTS, if applicable, acquire additional internal
   // buffers that store per-dot lifetimes and noise factors.
   if(t==RMV_RANDOMDOTS || t==RMV_FLOWFIELD)
   {
      // allocate array for vertex data: (x,y), (Tx, Ty) -- 4 floats per vertex
      m_pfBufDots = CRMVTarget::getBufferNodeFromPool(m_tgtDef.nDots*4);
      if(m_pfBufDots == NULL)
      {
         fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate internal per-dot vertex attribute array\n");
         return(false);
      }

      // reserve segment in shared vertex array for streaming per-dot vertex attributes for this target
      int idx = m_pRenderer->reserveSharedVertexArraySegment(m_tgtDef.nDots);
      if(idx < 0)
      {
         fprintf(stderr, "ERROR(CRMVTarget): Insufficient room in shared vertex attribute array\n");
         return(false);
      }
      m_vtxArrayStart = idx;
      m_vtxArrayCount = m_tgtDef.nDots;

      // the RNG for randomizing dot locations
      m_pDotRNG = new CUniformRNG;
      if(m_pDotRNG == NULL)
      {
         fprintf(stderr, "ERROR(CRMVTarget): Failed to create RNG for randomizing dot positions\n");
         return(false);
      }

      // the _RANDOMDOTS target may need additional arrays, RNG for certain features
      bool enaDotLife = (t==RMV_RANDOMDOTS) && (m_tgtDef.fDotLife != 0.0f);
      if(enaDotLife)
      {
         m_pfBufDotLives = CRMVTarget::getBufferNodeFromPool(m_tgtDef.nDots);
         if(m_pfBufDotLives == NULL)
         {
            fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate internal per-dot lifetimes array\n");
            return(false);
         }
      }

      bool enaDotNoise = (t==RMV_RANDOMDOTS) && (m_tgtDef.iNoiseUpdIntv > 0) && (m_tgtDef.iNoiseLimit > 0);
      if(enaDotNoise)
      {
         m_pfBufDotNoise = CRMVTarget::getBufferNodeFromPool(m_tgtDef.nDots);
         if(m_pfBufDotNoise == NULL)
         {
            fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate internal per-dot noise array\n");
            return(false);
         }

         m_pNoiseRNG = new CUniformRNG;
         if(m_pNoiseRNG == NULL)
         {
            fprintf(stderr, "ERROR(CRMVTarget): Failed to create RNG for randomizing dot noise\n");
            return(false);
         }
      }

      // seed the random-number generator(s)
      m_pDotRNG->setSeed(m_tgtDef.iSeed); 
      if(enaDotNoise) m_pNoiseRNG->setSeed(m_tgtDef.iSeed);

      // generate initial random-dot pattern. Note that texel coordinates (Tx, Ty) are included for each dot, so the
      // "stride" is 4.
      float* pfDots = m_pfBufDots->pBuf;
      if(t==RMV_FLOWFIELD)
      {
         // RMV_FLOWFIELD does not have an aperture per-se. The texture assigned to it is always the trivial "alpha=1"
         // texture. Hence Tx=Ty=0.5f for all dots -- corresponding to the texture center. These never change --
         // ensuring that the alpha mask texture will yield alpha=1 for all dots at all times.
         for(int i = 0; i < m_tgtDef.nDots; i++)
         {
            randomizeDotPosInFlowField(pfDots[i * 4], pfDots[i * 4 + 1]);
            pfDots[i * 4 + 2] = pfDots[i * 4 + 3] = 0.5f;
         }
      }
      else
      {
         // RMV_RANDOMDOTS does not use an alpha mask texture to implement the various apertures or the Gaussian window.
         // Instead, per-dot alpha is computed every frame and stored in Tx, which here is initialized to 1. When the
         // aperture is RMV_RECT and there's no Gaussian window, alpha = 1 for all dots at all times.
         for(int i = 0; i < m_tgtDef.nDots; i++)
         {
            randomizeDotPos(pfDots[i * 4], pfDots[i * 4 + 1]);
            pfDots[i * 4 + 2] = pfDots[i * 4 + 3] = 1.0f;
         }

         // if finite dotlife feature enabled, randomize current lifetime of all dots
         if(enaDotLife)
         {
            float* pfDotLives = m_pfBufDotLives->pBuf;
            for(int i=0; i<m_tgtDef.nDots; i++)
               pfDotLives[i] = float(m_pDotRNG->generate() * m_tgtDef.fDotLife);
         }
      }

      // so dir/speed noise updated on first frame (if enabled)
      m_tUntilNoiseUpdate = 0.0f; 
   }

   // RMV_IMAGE: Load image data from source file, then allocate RGBA texture object and load image data into it.
   // Since all drawing is done in logical coordinates -- "visual degrees", convert image W,H in pixels to visual
   // degrees and store in relevant members of the target definition. 
   if(t==RMV_IMAGE)
   {
      // load the image data in RGBA format. If source file is in another format (RGB, monochrome), it is converted to 
      // RGBA format. NOTE: Image data buffer is cached by media store -- do not free and do not save reference!
      int wPix = 0, hPix = 0;
      unsigned char* pImg = m_pRenderer->getImage(m_tgtDef.strFolder, m_tgtDef.strFile, wPix, hPix);
      if(pImg == NULL || wPix <= 0 || hPix <= 0)
      {
         ::fprintf(stderr, "ERROR(CRMVTarget): Failed to load image data from media file '%s/%s'\n", 
            m_tgtDef.strFolder, m_tgtDef.strFile);
         return(false);
      }

      // generate texture object and load image data into it.
      m_texID = m_pRenderer->prepareImageTexture(true, wPix, hPix, pImg);
      if(m_texID == 0)
      {
         ::fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate and load image texture\n");
         return(false);
      }

      // since drawing coordinates are in visual degrees, we compute the image's rectangular dimensions in those
      // coordinates and store them as the "target window" dimensions. 
      double w = wPix;
      double h = hPix;
      m_pRenderer->convertPixelDimsToDeg(w, h);
      m_tgtDef.fOuterW = (float) w;
      m_tgtDef.fOuterH = (float) h;
   }

   // RMV_MOVIE: Open video source file and prepare infrastructure for streaming video frame. Allocate image
   // texture object onto which video frames are blitted during playback. Initialize playback state.
   if(t==RMV_MOVIE)
   {
      char path[256];
      ::sprintf(path, "%s/%s/%s", CRMVMediaMgr::MEDIASTOREDIR, m_tgtDef.strFolder, m_tgtDef.strFile);
      m_videoStreamID = m_pRenderer->m_vidBuffer.openVideoStream(path, false, (m_tgtDef.iFlags & RMV_F_REPEAT)==0);
      if(m_videoStreamID < 0)
      {
         ::fprintf(stderr, "ERROR(CRMVTarget): Failed to open and buffer video stream\n");
         return(false);
      }

      int wPix = m_pRenderer->m_vidBuffer.getVideoWidth(m_videoStreamID);
      int hPix = m_pRenderer->m_vidBuffer.getVideoHeight(m_videoStreamID);
      m_texID = m_pRenderer->prepareImageTexture(false, wPix, hPix, NULL);
      if(m_texID == 0)
      {
         ::fprintf(stderr, "ERROR(CRMVTarget): Failed to allocate and load image texture for movie frames\n");
         return(false);
      }

      // since drawing coordinates are in visual degrees, we compute the movie frame's rectangular dimensions in those
      // coordinates and store them as the "target window" dimensions. 
      double w = wPix;
      double h = hPix;
      m_pRenderer->convertPixelDimsToDeg(w, h);
      m_tgtDef.fOuterW = (float) w;
      m_tgtDef.fOuterH = (float) h;

      // if we are to playback movie at the frame rate specified in the video file, calculate the ideal frame intv in 
      // ms. If the playback rate is not available, then we automatically playback at the display refresh rate.
      m_tElapsed = 0.0;
      m_tNextFrame = 0.0;
      m_tPlaybackIntv = -1.0;
      if((m_tgtDef.iFlags & RMV_F_ATDISPRATE) == 0)
      {
         double rateHz = m_pRenderer->m_vidBuffer.getVideoPlaybackRate(m_videoStreamID);
         if(rateHz > 0) m_tPlaybackIntv = 1000.0/rateHz;
      }
      m_iMovieState = MOVIE_NOTSTARTED;

      // create a round-robin queue of pixel buffer objects used to upload video frames to the texture object
      glGenBuffers(NUMPBOS, m_pboIDs);
      for(int i=0; i<NUMPBOS; i++)
      {
         glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIDs[i]);
         glBufferData(GL_PIXEL_UNPACK_BUFFER, m_pRenderer->m_vidBuffer.getVideoFrameSize(m_videoStreamID), 
               NULL, GL_STREAM_DRAW);
      }
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   } 

   return(true);
}

/** Free any resources that were successfully allocated in a previous call to allocateResources(). */
void CRMVTarget::freeResources()
{
   if(m_pRenderer != NULL && m_texID != 0) m_pRenderer->releaseTexture(m_texID);
   m_pRenderer = NULL;
   m_texID = 0;

   m_vtxArrayStart = m_vtxArrayCount = 0;

   if(m_pfBufDots != NULL)
   {
      CRMVTarget::releaseBufferNodeToPool(m_pfBufDots);
      m_pfBufDots = NULL;
   }
   if(m_pfBufDotLives != NULL)
   {
      CRMVTarget::releaseBufferNodeToPool(m_pfBufDotLives);
      m_pfBufDotLives = NULL;
   }
   if(m_pfBufDotNoise != NULL)
   {
      CRMVTarget::releaseBufferNodeToPool(m_pfBufDotNoise);
      m_pfBufDotNoise = NULL;
   }
   if(m_pDotRNG != NULL)
   {
      delete m_pDotRNG;
      m_pDotRNG = NULL;
   }
   if(m_pNoiseRNG != NULL)
   {
      delete m_pNoiseRNG;
      m_pNoiseRNG = NULL;
   }

   // note: The renderer handles closing of all open video streams. Here we simply discard the stream ID.
   m_videoStreamID = -1;
   m_iMovieState = MOVIE_UNINITIALIZED;
   m_gotLastFrame = false;
   m_tElapsed = 0.0;
   m_tNextFrame = 0.0;
   m_tPlaybackIntv = -1.0;

   glDeleteBuffers(NUMPBOS, m_pboIDs);
   for(int i=0; i<NUMPBOS; i++) m_pboIDs[i] = 0;
   m_iCurrPBOIdx = -1;
}


/**
 Pick a new random point that is within the bounding rectangle of a RMV_RANDOMDOTS target. The coordinates generated 
 are in the target's frame of reference -- ie, with the origin lying at the target's center point. Units are visual 
 degrees subtended at the eye.

 Dots are randomized within the rectangle that bounds the target aperture. Thus, except for the rectangular aperture,
 some dots will be positioned outside the aperture. Of course, this means that the number of dots actually visible at
 any time will be less than the defined number of dots for all apertures except the rectangular one.

 @param x [out] horizontal coordinate of point.
 @param y [out] vertical coordinate of point.
*/
void CRMVTarget::randomizeDotPos(float& x, float& y)
{
   double dH = m_pDotRNG->generate();                                      // pick random coords in (0..1)
   double dV = m_pDotRNG->generate();

   dH = (dH-0.5) * m_tgtDef.fOuterW;                                       // map to dims of bounding rectangle
   dV = (dV-0.5) * m_tgtDef.fOuterH;

   x = float(dH);                                                          // return coords by reference
   y = float(dV);
}

/**
 Pick a new random point that is within the annulus described by the inner and outer radii of a RMV_FLOWFIELD target.
 By definition, the coordinates are with respect to the origin (0,0), which is assumed to lie at the center of the
 screen. Units are visual degrees subtended at the eye.

 Since the flow field is a circular annulus, the method randomly chooses an angle, then a radial value lying between
 the field's inner and outer radii. From these polar coordinates, calculation of the x,y coords is straightforward.

 @param x [out] horizontal coordinate of point.
 @param y [out] vertical coordinate of point.
*/
void CRMVTarget::randomizeDotPosInFlowField(float& x, float& y)
{
   double dR = m_pDotRNG->generate();
   double dTheta = m_pDotRNG->generate();

   dR = double(m_tgtDef.fInnerW) + dR*(m_tgtDef.fOuterW - m_tgtDef.fInnerW); 
   dTheta *= 360.0; 

   x = float(dR*cMath::cosDeg(dTheta));
   y = float(dR*cMath::sinDeg(dTheta));
}

