/** rmvrenderer.cpp ===================================================================================================
 Implementation of class CRMVRenderer, which handles all OpenGL rendering in RMVideo

 CRMVRenderer was the product of a complete rethinking of how RMVideo renders. Prior to RMVideo V10, OpenGL rendering
 was handled by CRMVDisplay and a bunch of independent implementations of the abstract CRMVTarget class -- one class
 for each RMVideo target type. One of the problems with this design is that, because each target object "knows"
 nothing of the current OGL state, each target does its own thing. The situation is worse in OGL3.3 because every
 target uses a shader and has a vertex array and possibly a texture. Alternatively, we could isolate all rendering
 (and all OpenGL calls) to a monolithic renderer object, CRMVRenderer. It manages the animated target list and controls
 the runtime loop during an animiation sequence. Each target object (now CRMVTarget is a concrete class that implements
 all target types) requests OpenGL sources (vertex array space, texture object) from CRMVRenderer, and relies on 
 CRMVRenderer methods to issue the OpenGL commands that render the target IAW its current state.
 
 With a monolithic renderer it was easier to do optimizations like these:

 ==> Use a single shader program. The program is compiled, loaded, and made active at startup and never changes --
 so no shader switching happens. A shader program is required to conform to OpenGL3.3 (prior versions of RMVideo used
 legacy "immediate mode" functions. The vertex and fragment shaders for RMVideo are very simple -- simple enough that
 the GLSL code could be maintained in static string constants rather than separate source files. See VERTEXSHADERSRC
 and FRAGMENTSHADERSRC.

 ==> Some RMVideo targets need one texture (image, movie, alpha mask for some targets), others don't. For targets that
 don't, we can assign a small alpha=1 texture. (a) This makes the texture application in the single fragment shader
 simpler. (b) Since we only use texture unit 0, we can set the uniform that identifies this unit at application
 startup and never change it. (c) We can enable GL_TEXTURE2D and GL_BLEND at app startup and leave them enabled always,
 with the standard blend function (src_alpha, 1-src_alpha). We still have to bind a texture object to texture unit 0
 for each target drawn, but if all of the targets use the default alpha=1 texture, we can bind that once and never
 change it again. 

 ==> Most of the RMVideo targets use a very small vertex array (two triangles forming a quad, one line segment, or
 even a single point). Only the RMV_FLOWFIELD and RMV_RANDOMDOTS targets have large vertex arrays for the individual
 dots. To avoid allocating/deallocating vertex arrays frequently, CRMVRenderer creates and binds a single large vertex
 array at application startup. The simple quad, line segment and point primitives are preloaded at the front of this
 array in normalized coordinates. They can be shared by any target that needs them; a target-specific transform 
 (passed as a uniform to the vertex shader) handles the transformation from normalized to screen coordinates. The 
 rest of the vertex array is reserved for storing the vertex attributes for any flowfield or random-dot patch targets,
 and each such object reserves a contiguous portion of the array during target initialization. See the methods 
 reserveSharedVertexArraySegment() and uploadVertexData().

 ==> CRMVRenderer manages a texture memory pool to avoid excessive allocation and reallocation of GPU-side texture
 objects for images, movie frames, and alpha masks.

PERFORMANCE NOTE: As of May 2019, this OpenGL3.3 implementation of RMVideo (V10) does not perform quite as well as the
original OpenGL1.1 implementation (RMVideo V9). This is based on running trials that involve multiple fullscreen 
plaids, gratings, or spots on a 2560x1440 monitor at 144Hz (Dell S2417DG). The results show that this implementation
exhibits many more frame drops on the test trials involving plaid and grating targets, and this is likely because I
chose to leave the grating calculations in the fragment shader instead of using the multi-texturing approach in the
OpenGL1.1 implementation. In the future I many update this implementation to also use textures for the gratings, but
at this point it seems overkill. First, practical experiments are unlikely to animate multiple fullscreen grating or
plaid targets in the same trial. Second, it may be a while yet before labs start using monitor rates of 100Hz or
greater.

 Support for streaming videos on a background thread: Testing demonstrated that RMVideo could not handle streaming
 a 1024x768 video even if the refresh rate was as low as 85Hz. Trials involving such large movie targets would fail
 frequently on a duplicate frame error. In an effort to address this problem, we've introduced a new singleton
 CVidBuffer, as a public member of CRMVRenderer. CVidBuffer supports buffering up to 5 different video streams on a 
 background thread. On a multi-core system, the buffering thread is configured to run on one CPU from which RMVideo's
 main thread of execution is excluded. This, combined with a 10-frame in-memory queue for each open video stream,
 will hopefully increase the frame size of videos that RMVideo can present reliably. The video streamer object is
 public so that CRMTarget objects can conveniently access it during target initialization and motion updates.

 ==> Stereo mode support added in RMVideo v11.
 In stereo mode operation (CRMVDisplay::isStereoEnabled()), the video card is configured to swap the left and right
 backbuffers on each frame (so the "stereo frame rate" is really one-half the actual frame rate). Any time we have
 to write to a backbuffer, in stereo mode we have to update both the left and right backbuffers. Methods affected:
 measureFramePeriod(), redrawIdleBackground(), and animate().

 Stereo mode is used to implement the stereo dot disparity feature when animating any dot-like targets -- RMV_POINT,
 RMV_RANDOMDOTS, and RMV_FLOWFIELD. When the targets are drawn to the left buffer, CRMVTarget::draw(float eye) is
 called with eye = -0.5; for the left buffer, eye = +0.5. When stereo mode is not enabled, we only draw to a single
 backbuffer and the 'eye' argument is always 0.0.

 REVISION HISTORY:
 11apr2019-- Began development.
 24apr2019-- Completed development and tested. Performance worse than previous iteration with multiple target classes,
 a single shader, and "quick-n-dirty" implementations of a texture memory pool and a float buffer pool for the dot-
 patch target. Suggests that we need both the single shader and the memory pools for best performance. 
 29apr2019-- Implemented a float buffer pool in CRMVTarget, and implemented a texture object pool -- for alpha mask,
 RGBA image, and RGB movie frame textures -- in this class.
 01may2019-- Performance worsened with the texture pool. The original texture pool implementation was a hack that 
 preallocated 10 512x512 alpha mask textures and used them, as is, for all alpha masks regardless the actual size
 required. Trying to improve the new texture implementation to be more flexible than that, yet reduce the frequency of
 alpha mask texture re-allocations.
 06may2019-- Tweeked RMV_GRATING/PLAID implementation in fragment shader, eliminating several calculations that were
 the same for all target fragments. Changed some of the grating-related uniforms in rmvtarget.fs and updated the
 method updateGratingUniforms() accordingly.
 08may2019-- Since the vertex and fragment shader code in rmvtarget.vs/.fs is relatively simple, I decided to put
 the code in static strings defined here -- so that users can't break RMVideo by accidentally deleting the shader
 source files.
 04jun2019-- Modified fragment shader, replacing "isImage" uniform with "special": =1 for RMV_IMAGE and RMV_MOVIE;
 =2 for RMV_RANDOMDOTS, and =0 otherwise. For RMV_RANDOMDOTS, per-dot alpha is stored in texel coordinate Tx and
 alpha mask is not used. Fragment shader code adjusted accordingly.
 18sep2019-- Integrating helper class CVidBuffer, which manages a background thread that buffers video streams for 
 RMV_MOVIE targets during an animation sequence. The CVidBuffer member object is publicly accessible so that 
 RMV_MOVIE targets can access it directly.
 23sep2019-- CRMVTarget::updateMotion() now returns a boolean. If false, then a fatal error has occurred. 
 CRMVRenderer::animate() modified accordingly.
 24sep2019-- Technical change in loadTargets(). Target objects may draw on current backbuffer during initializations,
 primarily to ensure requested GL resources (like texture objects) are allocated immediately on the GPU, rather than
 in the middle of an animation sequence. After target loading, a glFinish() ensures any queued GL commands are 
 executed.
 28jan2020-- Modified measureFramePeriod() to use supplied nominal rate to compute an initial estimate of the
 refresh period, which is then used to detect any skipped frames during the 500-frame measurement period. If a 
 nominal rate is not supplied, it instead calculates an initial estimate over 50 frames prior to commencing the
 500-frame measurement. Backported from Lubuntu 18.04 version of source code.
 16dec2024-- Changes to support "stereo mode" (Priebe lab) in measureFramePeriod(), redrawIdleBackground(), and
 animate().
*/

#include "stdio.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rmvdisplay.h"
#include "rmvrenderer.h"

/**
 Vertex shader source code for RMVideo (originally in source file rmvtarget.vs).

 All RMVideo target implementations pass the 2D vertex location and the corresponding 2D texture coordinates to this 
 vertex shader via vertex array. Target RGB color and transform are supplied via uniforms. The shader transforms the 
 vertex location to normalized space. It passes on the transformed vertex, the texture coordinates, and the target
 color to the fragment shader.
 
 NOTES: (1) For the RMV_RANDOMDOTS target's "two-color constrast mode", one half the dots are rendered in one color, 
 and the other half in the second color. So the dot patch is rendered in two parts in that mode. (2) Targets which 
 don't really need a texture are bound to a tiny alpha texture with alpha=1.0 for all texels.
*/
const char* CRMVRenderer::VERTEXSHADERSRC="#version 330 core\n"
"layout (location=0) in vec2 aPos;        // The vertex location (x,y) in 2D space.\n"
"layout (location=1) in vec2 aTexCoord;   // Corresponding texture coordinates.\n"
"uniform mat4 xfm;                        // Transforms vertex to normalized space.\n"
"uniform vec3 tgtC;                       // The target RGB color applied to the vertex.\n"
"out vec3 rgb;                            // RGB color forwarded to the fragment shader.\n"
"out vec2 TexCoord;                       // texture coordinates forwarded to the fragment shader.\n"
"void main()\n"
"{\n"
"   gl_Position = xfm * vec4(aPos, 0.0, 1.0);\n"
"   TexCoord = aTexCoord;\n"
"   rgb = tgtC;\n"
"}\0";

/**
 Fragment shader source code for RMVideo (originally in source file rmvtarget.fs).

 RMV_BAR, RMV_SPOT, RMV_GRATING, RMV_PLAID: Target window maps to a single quad (vertices define two component
 triangles). Aperture shape and Gaussian blur are implemented by the alpha mask texture, which is pre-loaded before
 animation begins. If no blur and rectangular aperture, this texture is 4x4 with alpha = 1.0 for all texels. For 
 grating and plaid targets, a number of uniforms contain the parameters for the per-fragment grating
 calculations.

 RMV_MOVIE, RMV_IMAGE: Target window is a single quad, as above. Source texture is an RGBA texture containing the
 full image, or an RGB texture holding the video frame. The shader simply maps the texture onto the quad.

 RMV_POINT, RMV_FLOWFIELD, RMV_RANDOMDOTS: Vertices define individual dot locations (GL_POINTS) and are updated
 per-frame (for RMV_FLOWFIELD and _RANDOMDOTS, vertices are calculated on CPU side and then transformed in the
 vertex shader; for RMV_POINT, a fixed vertex at the origin is tranformed in the vertex shader). The fragment 
 color is found by combining the RGB color from the vertex shader with the alpha component from the alpha mask
 texture. For POINT and FLOWFIELD, this texture is always the 4x4 mask with alpha = 1.0 for all texels. For RANDOMDOTS,
 the alpha mask texture is NOT USED. Instead, each dot's alpha component is calculated every frame and delivered
 via the vertex attribute "Tx" representing the X-coordinate of the dot's corresponding texel location.

 All necessary information is passed via uniform variables. Note that, if a target implementation does not use a
 particular uniform, that variable will not be set by the client. For example, for the RMV_SPOT target, none of the
 uniforms related to the grating calcs apply.
*/
const char* CRMVRenderer::FRAGMENTSHADERSRC="#version 330 core\n"
"out vec4 FragColor;          // final fragment color, including alpha channel\n"
"in vec3 rgb;                 // opaque fragment color (forwarded from vertex shader)\n"
"in vec2 TexCoord;            // texture coordinates (forwarded from vertex shader)\n"
"// RMV_IMAGE, _MOVIE: image or current video frame. All others: alpha mask implementing aperture and Gaussian blur\n"
"uniform sampler2D tex;\n"
"uniform int special;         // 1 for RMV_IMAGE, RMV_MOVIE; 2 for RMV_RANDOMDOTS; else 0\n"
"uniform int nGrats;          // 2 for plaid, 1 for single grating; 0 otherwise\n"
"// all uniforms below this line apply only to grating calculations for grating/plaid targets\n"
"uniform vec2 ctr;            // current target center in screen coords (pixels WRT origin at TL corner)\n"
"uniform int isSine;          // (grating/plaid targets only) nonzero for sinewave, 0 for squarewave\n"
"uniform vec3 mean0;          // RGB mean color for grating 0 [0..1]\n"
"uniform vec3 con0;           // RGB contrast for grating 0 [0..1]\n"
"uniform vec3 mean1;          // RGB mean color for grating 1\n"
"uniform vec3 con1;           // RGB contrast for grating 1\n"
"uniform vec2 dx;             // projection of X spatial period onto line perpendicular to grating 0 and 1, in pixels\n"
"uniform vec2 dy;             // projection of Y spatial period onto line perpendicular to grating 0 and 1, in pixels\n"
"uniform vec2 phase;          // spatial phase of gratings 0 and 1, in normalized coordinates\n"
"\n"
"float when_eq(float x, float y)\n"
"{\n"
"   return 1.0f - abs(sign(x-y));\n"
"}\n"
"\n"
"float when_neq(float x, float y)\n"
"{\n"
"   return abs(sign(x-y));\n"
"}\n"
"\n"
"const float TWOPI = 6.28318531;\n"
"\n"
"void main()\n"
"{\n"
"   vec2 p;\n"
"   vec3 color = rgb;\n"
"   vec4 rgba = texture(tex, TexCoord);\n"
"\n"
"   // for targets other than RMV_IMAGE, _MOVIE, the texture is an alpha mask texture, with alpha in the R cmpt\n"
"   // special case: for RMV_RANDOMDOTS, per-dot alpha is in TexCoord.x and alpha mask texture is unused.\n"
"   float alpha = when_eq(special, 2)*TexCoord.x + when_neq(special, 2)*rgba.r;\n"
"\n"
"   // grating calcs to compute fragment RGB. The spatial period is in pixels, and we need to divide this into the\n"
"   // fragment coordinates. So we leave the fragment coordinates in pixels, but WRT origin at target center.\n"
"   if(nGrats > 0)\n"
"   {\n"
"      p = gl_FragCoord.xy - ctr;\n"
"      float frac = sin(TWOPI*(p.x*dx[0] + p.y*dy[0]) + phase[0]);\n"
"      frac = when_neq(isSine, 0)*frac + when_eq(isSine, 0)*(2.0*smoothstep(-0.02, 0.02, frac) - 1.0);\n"
"      color = mean0 * (1.0 + con0*frac);\n"
"      if(nGrats > 1)\n"
"      {\n"
"         frac = sin(TWOPI*(p.x*dx[1] + p.y*dy[1]) + phase[1]);\n"
"         frac = when_neq(isSine, 0)*frac + when_eq(isSine, 0)*(2.0*smoothstep(-0.02, 0.02, frac) - 1.0);\n"
"         color += mean1 * (1.0 + con1*frac);\n"
"      }\n"
"      color = clamp(color, 0.0, 1.0);\n"
"   }\n"
"\n"
"    FragColor = vec4(when_eq(special, 1))*rgba + vec4(when_neq(special, 1))*vec4(color, alpha);\n"
"}\0";


const int CRMVRenderer::ALPHAMASKTEX = 1;
const int CRMVRenderer::RGBAIMAGETEX = 2;
const int CRMVRenderer::RGBIMAGETEX = 3;

const int CRMVRenderer::MAXTEXMASKDIM = 512;
const int CRMVRenderer::MAXNUMVERTS = 50000;
const int CRMVRenderer::QUADINDEX = 0;
const int CRMVRenderer::QUADCOUNT = 6;
const int CRMVRenderer::VIDQUADINDEX = 6;
const int CRMVRenderer::VIDQUADCOUNT = 6;
const int CRMVRenderer::VLINEINDEX = 12;
const int CRMVRenderer::VLINECOUNT = 2;
const int CRMVRenderer::POINTINDEX = 14;
const int CRMVRenderer::POINTCOUNT = 1;
const int CRMVRenderer::DOTSTOREINDEX = 15;

const int CRMVRenderer::DEF_WIDTH = 400;
const int CRMVRenderer::DEF_HEIGHT = 300;
const int CRMVRenderer::DEF_DISTTOEYE = 800;
const int CRMVRenderer::DEF_WIDTH_PIX = 1024;
const int CRMVRenderer::DEF_HEIGHT_PIX = 768;

CRMVRenderer::CRMVRenderer()
{
   m_pDisplay = NULL;
   m_pShader = NULL;
   m_NoOpAlphaMaskID = 0;
   m_pMaskTexels = NULL;
   m_texPoolHead = NULL;
   m_texPoolSize = 0;
   m_texPoolBytes = 0.0;
   m_idVAO = 0;
   m_idVBO = 0;
   m_idxVertexArrayFree = 0;
   m_currBoundTexID = 0;

   m_dFramePeriod = 0;

   updateDisplayGeometry(DEF_WIDTH, DEF_HEIGHT, DEF_DISTTOEYE);
   for( int i=0; i<3; i++ ) m_bkgRGB[i] = 0.0;

   // photodiode sync flash spot feature disabled initially
   m_syncSpot.size = 0;
   m_syncSpot.flashDur = RMV_MINSYNCDUR;
   m_syncSpot.wDeg = m_syncSpot.hDeg = 0.0;
   m_syncSpot.nFramesLeft = 0;

   m_nTargets = 0;
   m_pTargetList = NULL;
}

CRMVRenderer::~CRMVRenderer()
{
   unloadTargets();
   releaseResources();
}

/**
 Create all OpenGL resources required to do all target rendering in RMVideo:

 1) Compile and load the single shader program that is used to do all rendering in RMVideo. The source code for the 
 shaders is defined in static strings VERTEXSHADERSRC and FRAGMENTSHADERSRC. Once loaded successfully, the shader 
 program is bound as the current program, since it never changes.
 2) Allocate buffer used to generate alpha mask textures and load them into GPU texture memory.
 3) Allocate the 50K vertex array/buffer that is used to transfer all vertex data (across all targets) to the vertex 
 shader during an RMVideo animation sequence. 
 4) Generate the 4x4 "alpha=1" default texture that is bound when rendering a target that does not need an alpha mask
 texture nor image texture.
 5) Create a reusable texture object pool that supplies alpha mask, RGBA image, or RGB movie frame textures for use
 by RMVideo targets, allocating new textures only when necessary. Initialize the pool with several alpha-mask textures,
 which are likely to be used most frequently.
 6) Allocate a memory pool for per-dot parameter storage required by the random-dot target types. This will avoid 
 frequent memory allocations/deallocations associated with those targets. See CRMVTarget::createBufferPool().

 This method must be called during RMVideo startup, and RMVideo should exit on failure. Error messages are written to
 the console. It also must be called each time RMVideo's fullscreen window is re-created -- which happens on any video
 mode switch.

 @param pDsp The RMVideo display manager. A reference to this object is kept in order to access display parameters and
 the RMVideo comm link.
 @return True if shader programs and any other OpenGL rendering resources were created; false otherwise.
*/
bool CRMVRenderer::createResources(CRMVDisplay* pDsp)
{
   if(m_pShader != NULL) return(true);
   if(pDsp == NULL) return(false);
   m_pDisplay = pDsp;

   // verify that texture dimension of MAXTEXMASKDIM is supported
   int maxTexSize = 0;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
   if(maxTexSize < MAXTEXMASKDIM)
   {
      fprintf(stderr, "ERROR(CRMVRenderer): GL version does not support texture size up to %d pix!\n", MAXTEXMASKDIM);
      m_pDisplay = NULL;
      return(false);
   }

   // allocate a large array in which alpha mask textures are calculated and loaded into GPU texture memory. We do this
   // once to avoid frequent large memory allocations that are not necessary. 
   m_pMaskTexels = new GLubyte[MAXTEXMASKDIM*MAXTEXMASKDIM];
   if(m_pMaskTexels == NULL)
   {
      fprintf(stderr, "ERROR(CRMVRenderer): Failed to allocate buffer for alpha mask texture generation!\n");
      m_pDisplay = NULL;
      return(false);
   }
   memset(m_pMaskTexels, 0, MAXTEXMASKDIM*MAXTEXMASKDIM*sizeof(GLubyte));

   // compile and load our monolithic shader program
   m_pShader = new Shader(CRMVRenderer::VERTEXSHADERSRC, CRMVRenderer::FRAGMENTSHADERSRC, false);
   bool ok = m_pShader->isUsable();
   if(!ok) fprintf(stderr, "ERROR(CRMVRenderer): Failed to create GLSL shader program\n");

   // create and load the small "no-op" alpha mask texture assigned to all targets that are not an image or movie and
   // that do not need an alpha mask.
   if(ok)
   {
      ok = generateNoOpAlphaMaskTexture();
      if(!ok) fprintf(stderr, "ERROR(CRMVRenderer): Failed to create and load default 'no-op' alpha mask texture\n");
   }

   // allocate the single shared vertex array buffer used to transfer vertex attributes of all target objects to 
   // the vertex shader. Fixed vertices defining a single quad, a flipped quad for RMV_MOVIE, a vertical line segment
   // for RMV_BAR, and a single vertex for RMV_POINT are pre-loaded at the front of this array. Only RMV_RANDOMDOTS
   // and RMV_FLOWFIELD stream vertex data to the array on a frame-by-frame basis; each such target reserves a 
   // contiguous segment of the array for this purpose.
   if(ok)
   {
      ok = allocateSharedVertexArray();
      if(!ok) fprintf(stderr, "ERROR(CRMVRenderer): Failed to allocated shared vertex array buffer\n");
   }

   // initialize the video streaming helper: launches the background thread that will buffer any videos when RMV_MOVIE
   // targets are animated.
   if(ok)
   {
      ok = m_vidBuffer.initialize();
      if(!ok) 
         ::fprintf(stderr, "ERROR[CRMVRenderer]: Failed to start background thread that buffers video streams.\n");
   }

   // create memory pool used for per-dot parameter storage associated with the random-dot target types
   if(ok) ok = CRMVTarget::createBufferPool();

   // if successful, activate shader now and set the uniform variable that selects texture unit 0. We only use the
   // one shader, and we always use texture unit 0. Also bind the shared vertex array and buffer; since we use that
   // array buffer always, there's no need to bind/unbind repeatedly. Finally, we set the line width and polygon
   // drawing mode, enable blending, and set the blend function now, as these state parameters never change.
   if(ok) 
   {
      m_pShader->use();
      m_pShader->setInt("tex", 0);
      glActiveTexture(GL_TEXTURE0);
      bindTextureObject(m_NoOpAlphaMaskID);
      glBindVertexArray(m_idVAO);
      glBindBuffer(GL_ARRAY_BUFFER, m_idVBO);

      glLineWidth((GLfloat)1);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // set "clear color" to the current bkg color.
      glClearColor((float) m_bkgRGB[0], (float) m_bkgRGB[1], (float) m_bkgRGB[2], 0.0f);

      // set up simple 2D viewport with units in visual degrees subtended at the eye (the preferred units for Maestro
      // target trajectories!). Note that this method is called every time a video mode change occurs, so we use
      // existing measured screen dimensions and distance to eye (in mm) to update display geometry.
      glViewport(0, 0, m_pDisplay->getScreenWidth(), m_pDisplay->getScreenHeight());
      updateDisplayGeometry(m_dspGeom.wMM, m_dspGeom.hMM, m_dspGeom.dMM);
   }
   else 
      releaseResources();

   return(ok);
}

/**
 Release all OpenGL rendering resources that were created by createResources(). This method should be called when 
 RMVideo shuts down, after the OpenGL window is hidden.
*/
void CRMVRenderer::releaseResources()
{
   unloadTargets();

   m_vidBuffer.reset();

   CRMVTarget::destroyBufferPool();

   destroyTexturePool();

   glDisable(GL_BLEND);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
   if(m_idVAO != 0)
   {
      glDeleteVertexArrays(1, &m_idVAO);
      glDeleteBuffers(1, &m_idVBO);
      m_idVAO = m_idVBO = 0;
      m_idxVertexArrayFree = 0;
   }

   m_currBoundTexID = 0;
   glBindTexture(GL_TEXTURE_2D, 0);
   glDeleteTextures(1, &m_NoOpAlphaMaskID);
   m_NoOpAlphaMaskID = 0;

   glUseProgram(0);
   if(m_pShader != NULL)
   {
      delete m_pShader;
      m_pShader = NULL;
   }

   if(m_pMaskTexels != NULL)
   {
      delete m_pMaskTexels;
      m_pMaskTexels = NULL;
   }

   m_pDisplay = NULL;
}

/**
 Reserve a contiguous segment of the RMVideo's shared vertex array/buffer for streaming vertices to the OpenGL driver.

 In RMVideo, only the random-dot target types -- RMV_RANDOMDOTS and RMV_FLOWFIELD -- need to stream vertex data to the
 shader program on a frame-by-frame basis. All other targets use a very simple fixed primitive, either a quad, vertical
 line segment or single point. To avoid continually binding and unbinding vertex arrays during an animation sequence
 involving multiple targets of different types, our strategy is to allocate a single large vertex array/buffer that is
 shared among all targets animated.

 The fixed primitives are stored at the start of this shared array and their vertex attributes never change, so they
 can be used by multiple targets.

 A random-dot target, however, must reserve a contiguous segment within the shared array for its exclusive use. During
 an animation sequence, the target object will stream per-dot vertex attributes to this segment on a frame-by-frame
 basis by calling uploadVertexData().

 Call this method to reserve a segment within the shared vertex array. The array can support up to 50K vertices, which
 should be more than enough for any practical animation in RMVideo.

 @param n [in] The number of vertices needed. 
 @return The start index for a contiguous segment in the shared vertex array spanning the specified number of vertices.
 If the vertex array is unavailable or does not have room for the specified number of vertices, -1 is returned.
*/
int CRMVRenderer::reserveSharedVertexArraySegment(int n)
{
   if(m_idVAO == 0) return(-1);
   if(m_idxVertexArrayFree + n >= MAXNUMVERTS) return(-1);
   int out = m_idxVertexArrayFree;
   m_idxVertexArrayFree += n;
   return(out);
}

/**
 Upload vertex attributes to a specified portion of the shared vertex array (dot targets only). The array segment 
 identified by the 'start' and 'count' parameters should have been previously reserved by calling the method
 reserveSharedVertexArraySegment().

 @param start [in] The starting index within shared vertex array
 @param count [in] The number of vertices to upload.
 @param pSrc [in] Buffer containing the vertex attributes to be uploaded. There must be 4 float-valued attributes
 (x,y,Tx,Ty) per vertex, so this buffer's length in bytes must be >= count*4*sizeof(float).
*/
void CRMVRenderer::uploadVertexData(int start, int count, float* pSrc)
{
   if((pSrc == NULL) || (start < DOTSTOREINDEX) || (start + count > MAXNUMVERTS)) return;
   glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*start*4, sizeof(float)*count*4, pSrc);
}

/**
 Utility method that prepares a single-component texture defining the translucency ("alpha") mask that implements the
 three non-rectangular RMVideo apertures (RMV_OVAL, _RECTANNU, and _OVALANNU) as well as a 2D Gaussian spatial blur.
 The alpha mask texture essentially overlays the target's bounding rectangle when the target is drawn. Since the mask 
 never changes, it can be generated before the target is animated and applied as a texture on each draw cycle.

 The method computes and loads the alpha mask into texture memory and returns the texture ID. The texture format is
 GL_RED because we only need one color component, and GL_ALPHA is considered obsolete. In the fragment shader, the
 texture is sampled and the R component of the RGBA sample is assigned to the alpha component of the fragment color,
 thus "applying" the alpha mask (of course, alpha blending must be enabled).
 
 If the aperture is RMV_RECT and there's no Gaussian blur, then the target does not require an alpha mask. In this
 case, the method does nothing and returns 0. [Because our monolithic shader always applies a texture, CRMVRenderer
 binds a 4x4 "no-op" alpha mask texture, which has alpha=1 at all texel locations, and hence has no effect when 
 applied with glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).]

 To optimize texture memory use, CRMVRenderer maintains an OpenGL texture object pool. If the pool contains an already 
 allocated alpha mask texture object that meets or exceeds the required dimensions, that texture object will be 
 reused. Otherwise, a new alpha mask texture is allocated and added to the pool.

 When the texture is no longer needed, callers must release it back to the texture pool by calling releaseTexture().

 NOTE: We tried implementing the alpha mask in the fragment shader, but testing demonstrated that this approach was
 actually slower than using a pre-calculated alpha texture, as in the OGL 1.1 implementation of RMVideo. This makes 
 sense, since the alpha calculations in the shader would be done on every frame and for every fragment. The alpha 
 texture need only be calculated during target initialization and loaded into GPU memory; then, in the shader, the
 texture is simply sampled and that sample is applied to the fragment's alpha component -- no calculations needed.

 NOTE 2: We first tried the modern OpenGL "render to texture" approach to generate the alpha mask texture, but this
 did not work.

 NOTE 3: Initially, we did not restrict the dimensions of the texture, nor require they be powers of 2. However, we
 found that the OGL 1.1 implementation was superior, possibly because that implementation enforces power-of-2 texture
 dimensions not to exceed 512. Decided to do the same here.

 @param aperture Aperture type. If not a supported type, RMV_RECT is assumed.
 @param w,h The width and height of target window in logical coordinates (visual deg subtended at eye).
 @param iw,ih The width and height of the target hole for an annular aperture (visual deg subtended at eye). Both must
 be nonzero if aperture is RMV_RECTANNU or _OVALANNU.
 @param sigX,sigY The horizontal and vertical standard deviations of the 2D Gaussian blur applied to the target window.
 If both are zero, no blur is applied.
 @return If successful, the texture object's assigned OpenGL ID; else, 0. In the event of failure, a brief error 
 description is logged to the console window. Note that 0 is also returned if the aperture is RMV_RECT and 
 sigX=sigY=0 -- since no alpha mask is needed in this case.
*/
unsigned int CRMVRenderer::prepareAlphaMaskTexture(
   int aperture, double w, double h, double iw, double ih, double sigX, double sigY)
{
   // if no alpha mask needed, there's nothing to do!
   if(aperture==RMV_RECT && sigX <= 0 && sigY <= 0) return((unsigned int) 0);

   // compute texture dimensions in pixels. For better performance, we restrict each dimension to a power of 2 not to 
   // exceed MAXTEXMASKDIM
   int texWPix = 8, texHPix = 8;

   while(texWPix < (int)(w/m_dspGeom.degPerPixelX)) texWPix *= 2;
   if(texWPix > MAXTEXMASKDIM) texWPix = MAXTEXMASKDIM;

   while(texHPix < (int)(h/m_dspGeom.degPerPixelY)) texHPix *= 2;
   if(texHPix > MAXTEXMASKDIM) texHPix = MAXTEXMASKDIM;

   // get an available alpha max texture object from the texture pool that is large enough to accommodate the
   // desired texture dimensions, allocating a new texture if necessary
   TexNode* pNode = getTextureNodeFromPool(ALPHAMASKTEX, texWPix, texHPix);
   if(pNode == NULL)
   {
      fprintf(stderr, "ERROR(CRMVRenderer): Insufficient texture memory available for %dx%d alpha mask\n", 
            texWPix, texHPix);
      return((unsigned int) 0);
   }

   // compute the alpha mask texture and store in local array
   // the texture array fills from BL->TR, and it is assumed coord system has origin at target center!
   double x = -double(w) / 2.0;
   double y = -double(h) / 2.0;
   double dXIncr = double(w) / double(texWPix);
   double dYIncr = double(h) / double(texHPix);

   // compute constant factors -1/(2*sx*sx) and -1/(2*sy*sy) for Gaussian function.  Note that sigma=0 is really
   // treated as sigma=infinity!
   double dInvTwoSigSqX = (sigX > 0) ? -1.0 / (2.0 * sigX * sigX) : 0.0;
   double dInvTwoSigSqY = (sigY > 0) ? -1.0 / (2.0 * sigY * sigY) : 0.0;
   bool doGauss = (sigX > 0 || sigY > 0);

   // we need these for testing whether points are inside the outer ellipse or, for RMV_OVALANNU, outside the
   // inner ellipse: x*x/A*A + y*y/B*B <= 1, x*x/C*C + y*y/D*D > 1.
   double dASq = w * w / 4.0;
   double dBSq = h * h / 4.0;
   double dCSq = iw * iw / 4.0;
   double dDSq = ih * ih / 4.0;

   // we need these for testing whether points are inside the outer rect or, for RMV_RECTANNU, outside inner rect
   double dOuterHalfW = w / 2.0;
   double dOuterHalfH = h / 2.0;
   double dInnerHalfW = iw / 2.0;
   double dInnerHalfH = ih / 2.0;

   // we use these to create a little antialiasing effect near the elliptical aperture boundary
   double dXOffset[5];
   dXOffset[0] = 0;
   dXOffset[1] = -dXIncr;
   dXOffset[2] = -dXIncr;
   dXOffset[3] = dXIncr;
   dXOffset[4] = dXIncr;
   double dYOffset[5];
   dYOffset[0] = 0;
   dYOffset[1] = -dYIncr;
   dYOffset[2] = dYIncr;
   dYOffset[3] = -dYIncr;
   dYOffset[4] = dYIncr;

   // calculate mask(x,y) = (insideAperture ? 1.0 : 0.0) * Gaussian fcn
   for(int j = 0; j<texHPix; j++)
   {
      int k = j * texWPix;
      x = -double(w) / 2.0;
      for(int i = 0; i<texWPix; i++)
      {
         // "insidedness" test for aperture -- here's where the smoothing transition is implemented
         double dValue = 0.0;
         for(int m = 0; m<5; m++)
         {
            double xp = x + dXOffset[m];
            double yp = y + dYOffset[m];
            switch(aperture)
            {
            case RMV_OVAL:
               if(xp*xp / dASq + yp * yp / dBSq <= 1.0)
                  dValue += 1.0;
               break;
            case RMV_RECTANNU:
               if(fabs(xp) <= dOuterHalfW && fabs(yp) <= dOuterHalfH && (fabs(xp)>dInnerHalfW || fabs(yp)>dInnerHalfH) )
                  dValue += 1.0;
               break;
            case RMV_OVALANNU:
               if((xp*xp / dASq + yp * yp / dBSq <= 1.0) && (xp*xp / dCSq + yp * yp / dDSq > 1.0))
                  dValue += 1.0;
               break;
            case RMV_RECT: 
            default: 
               dValue += 1.0;
               break;
            }
         }
         dValue /= 5.0;

         // computation of Gaussian fcn, if necessary
         if(dValue > 0.0 && doGauss) dValue *= exp(x*x*dInvTwoSigSqX + y * y*dInvTwoSigSqY);

         m_pMaskTexels[k + i] = (GLubyte)cMath::rangeLimit(dValue * 255.0 + 0.5, 0.0, 255.0);
         x += dXIncr;
      }
      y += dYIncr;
   }

   // load the mask texture [NOTE: Use glTexSubImage2D() because texture is already allocated!]
   bindTextureObject(pNode->id);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWPix, texHPix, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)m_pMaskTexels);
   bindTextureObject(m_NoOpAlphaMaskID);
   return(pNode->id);
}

/**
 Retrieve image from a specified source file in the RMVideo media store.
 @param folder Name of media folder containing image source file.
 @param file Name of image source file.
 @param width [out] On successful return, contains image width in pixels; else, 0.
 @param height [out] On successful return, contains image height in pixels; else, 0.
 @return A pointer to the image data buffer. Returns NULL on failure, in which case a very brief error description is 
 printed to stderr. DO NOT free() the buffer, nor maintain a reference to it!
*/
unsigned char* CRMVRenderer::getImage(const char* folder, const char* file, int& w, int& h)
{
   CRMVMediaMgr* pMgr = (m_pDisplay==NULL) ? NULL : m_pDisplay->getMediaStoreManager();
   return((pMgr==NULL) ? NULL : pMgr->getImage(folder, file, w, h));
}

/**
 Utility method prepares a GL_RGBA or GL_RGB texture object and optionally loads it with image data. The RMV_IMAGE
 target requires a GL_RGBA texture, while the RMV_MOVIE target uses a GL_RGB texture to load each movie frame.

 To optimize texture memory use, CRMVRenderer maintains an OpenGL texture object pool. If the pool contains an already
 allocated texture object of the right type that meets or exceeds the required dimensions, that texture object will be
 reused. Otherwise, a new texture object is allocated and added to the pool.

 When the texture is no longer needed, callers must release it back to the texture pool by calling releaseTexture().

 @param rgba If true, prepare a GL_RGBA texture (32-bit, 1 byte per component, ordered R,G,B,A); else, a GL_RGB
 texture (24-bit, 1 byte per component, R,G,B).
 @param w Width of image texture in pixels.
 @param h Height of image texture in pixels.
 @param pImg If not NULL, the image data. The order of data in the buffer must conform to the GL_RGBA or GL_RGB
 format. It is assumed that the buffer is N x w x h bytes long, where N is 3 or 4.
 @return If successful, the texture object's assigned OpenGL ID; else, 0. In the event of failure, a brief error 
 description is logged to the console window.
*/
unsigned int CRMVRenderer::prepareImageTexture(bool rgba, int w, int h, unsigned char* pImg)
{
   if(w < 0 || h < 0) 
   {
      fprintf(stderr, "ERROR(CRMVRenderer): Cannot allocate image texture with zero width or height!\n");
      return((unsigned int) 0);
   }

   // get an available texture object from the texture pool of the right type and large enough to accommodate the
   // desired image dimensions, allocating a new texture object if necessary
   TexNode* pNode = getTextureNodeFromPool(rgba ? RGBAIMAGETEX : RGBIMAGETEX, w, h);
   if(pNode == NULL)
   {
      fprintf(stderr, "ERROR(CRMVRenderer): Insufficient texture memory available for %dx%d image\n", w, h);
      return((unsigned int) 0);
   }

   // if image data provided, load texture accordingly [NOTE: Use glTexSubImage2D() because texture is already 
   // allocated!]
   if(pImg != NULL)
   {
      bindTextureObject(pNode->id);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, rgba ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)pImg);
      bindTextureObject(m_NoOpAlphaMaskID);
   }

   return(pNode->id);
}

/**
 Upload a movie frame to the specified OpenGL texture object.

 An RMV_MOVIE target calls this method to store the next displayed video frame in a dedicated texture object that was
 generated for the purpose during the target initialization phase.

 @param texID [in] The OpenGL ID of the texture object to which movie frame is to be uploaded.
 @param w,h [in] Frame dimensions in pixels.
 @param pFrame [in] Frame data buffer. Data is expected to be store in GL_RGB format, with 8 bits per color. Use 
 pFrame = NULL to upload frame data from the currently bound pixel buffer.
 component.
*/
void CRMVRenderer::uploadMovieFrameToTexture(unsigned int texID, int w, int h, unsigned char* pFrame)
{
   bindTextureObject(texID);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*) pFrame);
}

/**
 Release a texture object previously provided by a call to one of the generate***() methods. This method returns the
 texture object to an internally maintained texture pool for reuse.
 @param texID The OpenGL ID of the texture object to be destroyed.
*/
void CRMVRenderer::releaseTexture(unsigned int texID)
{
   TexNode* pCurr = m_texPoolHead;
   while(pCurr != NULL )
   { 
      if(pCurr->id==texID)
      {
         pCurr->inUse = false;
         return;
      }
      pCurr = pCurr->pNext;
   }
}

/** 
 Obtain an accurate measure of the vertical refresh period/rate by measuring the elapsed time for 500 frames. This 
 method takes 500/R seconds to complete, where R is the frame rate in Hz. At 60Hz, that's 8.33 seconds!

 During the measurement, it is assumed that the fullscreen window is displayed. The background color is toggled between
 red and blue on every frame. The perception should be a steady purple background, slightly flickering at refresh rates
 less than 80Hz or so. If this is not the case, then something is amiss! (eg, "tearing artifact" if VSync not enabled)

 Though unlikely, it is possible that an extra frame ("skipped frame") could occur during the measurement period. This
 would result in an overestimate of the refresh period P = T/500, since T would be the elapsed time for 501 or more
 frames. To guard against this possibility, the method uses the supplied nominal refresh rate to check for any skipped
 frames and adjust the frame count N=500 accordingly. If a nominal rate is not supplied, the method will add an
 additional 50 frames to the measurement period and use the first 50 frames to calculate an initial estimate of the
 refresh period.

 NOTE: When stereo mode is enabled, a red background is drawn on the left buffer and a blue on the right buffer. We
 only draw these once, and then do a swap each frame. In stereo mode, the left buffer is shown for one frame and the
 right buffer for the next. The frame period measurement is the same whether in stereo not -- and that is the frame
 period reported to Maestro. But, when stereo mode is enabled, the left and right buffers get alternated every frame,
 so the effective "stereo frame period" is one-half the actual frame period.

 @param nomRateHz The nominal refresh rate in Hz for the current video mode. <=0 if not available.
 @return True if successful; false otherwise. Will fail if estimated frame rate is less than 60Hz.
*/
bool CRMVRenderer::measureFramePeriod(int nomRateHz)
{
   if(m_pDisplay==NULL) return(false);
   if(!CElapsedTime::isSupported())
   {
      fprintf(stderr, "ERROR(CRMVRenderer): High-res timer required to accurately measure vertical refresh!\n");
      return(false);
   }
   CElapsedTime eTime;

   fprintf(stderr, "Estimating vertical refresh period/rate..." );

   double tInitFP = 0, tLast = 0;
   int nSkips = 0;

   if(!m_pDisplay->isStereoEnabled())
   {
      // need to do this to get in synch with display's refresh cycle, so we can start our timer at the beginning of a
      // refresh period. NOTE, however, that our measurement could be an overestimate if there's a delay out of the
      // second glFinish() call here, or an underestimate if there's a delay out of the last glFinish() for the 500th
      // frame...
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      m_pDisplay->swap();
      glFinish();
      eTime.reset();

      glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      m_pDisplay->swap();
      glFinish();
      eTime.reset();  // T=0

      // when a nominal refresh rate is not supplied, we calculate an initial estimate of the refresh period over 50 frames
      if(nomRateHz <= 0)
      {
         for(int i = 1; i <= 50; i++)
         {
            if(i % 2 == 0) glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
            else glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            m_pDisplay->swap();
            glFinish();
         }
         tInitFP = eTime.getAndReset() / 50.0;
      }
      else tInitFP = 1.0 / ((double)nomRateHz);

      // here we measure the elapsed time over 500 frames, using the initial estimate of the refresh period (or the estimate
      // based on the supplied nominal refresh rate) to detect any skipped frames
      for(int i = 1; i <= 500; i++)
      {
         if(i % 2 == 0) glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
         else glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
         glClear(GL_COLOR_BUFFER_BIT);
         m_pDisplay->swap();
         glFinish();

         double t = eTime.get();
         double d = (t - tLast) / tInitFP;
         while(cMath::abs(d) > 1.5) { ++nSkips; d = d - 1.0; }
         tLast = t;
      }
   }
   else
   {
      // when stereo mode is enabled, the left buffer has the red background and the right buffer has the blue...
      // and that means we don't have to redraw the backbuffer in the other color each frame. Since the swap() 
      // swaps the L and R backbuffers in stereo mode, all we have to do is execute a swap each time...
      glDrawBuffer(GL_BACK_LEFT);
      glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_pDisplay->swap();
      glFinish();
      eTime.reset();

      glDrawBuffer(GL_BACK_RIGHT);
      glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_pDisplay->swap();
      glFinish();
      eTime.reset();  // T=0

      if(nomRateHz <= 0)
      {
         for(int i = 1; i <= 50; i++)
         {
            m_pDisplay->swap();
            glFinish();
         }
         tInitFP = eTime.getAndReset() / 50.0;
      }
      else tInitFP = 1.0 / ((double)nomRateHz);

      for(int i = 1; i <= 500; i++)
      {
         m_pDisplay->swap();
         glFinish();

         double t = eTime.get();
         double d = (t - tLast) / tInitFP;
         while(cMath::abs(d) > 1.5) { ++nSkips; d = d - 1.0; }
         tLast = t;
      }
   }

   double tElapsed = eTime.get();

   m_dFramePeriod = tElapsed / (500.0 + nSkips);
   double rate = 1.0/m_dFramePeriod;
   bool ok = rate >= 59.9;
   if(ok) fprintf(stderr, "%.3f Hz (P = %.3f ms)\n", rate, m_dFramePeriod*1.0e3);
   else fprintf(stderr, "Measured rate, %.3f Hz, does not meet requirement!\n", rate);

   // restore current background
   redrawIdleBackground();

   return(ok);
}

/**
 Update the current display geometry.

 Target size and trajectory information from Maestro are measured in degrees subtended at the subject's eye. In order
 to convert these ultimately to monitor pixels, Maestro and RMVideo assume that the subject and monitor are situated 
 such that, when the subject is looking straight ahead (0deg, 0deg), the line-of-sight vector passes through the center
 of the fullscreen window controlled by RMVideo at a perpendicular angle. Given this LOS distance and the visible 
 screen dimensions in millimeters, along with the screen resolution in pixels, RMVideo can make the necessary 
 conversions.

 Since changing the display geometry will affect the appearance of the photodiode spot flash in the TL corner of the
 screen, the background will be redrawn if the spot flash size is currently non-zero. 

 @param w, h [in] Width, height of visible screen rectangle on monitor, in millimeters. Silently corrected to ensure
 both values are at least 100mm.
 @param d [in] Distance from subject's eye to center of visible screen rectangle, in millimeters. Silently corrected
 to ensure the distance is at least 100mm.
*/
void CRMVRenderer::updateDisplayGeometry(int w, int h, int d)
{
   m_dspGeom.wMM = (w < 100) ? 100 : w;
   m_dspGeom.hMM = (h < 100) ? 100 : h;
   m_dspGeom.dMM = (d < 100) ? 100 : d;

   m_dspGeom.wDeg = 2.0 * cMath::toDegrees( ::atan2( double(m_dspGeom.wMM)/2.0, double(m_dspGeom.dMM) ) );
   m_dspGeom.hDeg = 2.0 * cMath::toDegrees( ::atan2( double(m_dspGeom.hMM)/2.0, double(m_dspGeom.dMM) ) );

   double wPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenWidth() : DEF_WIDTH_PIX;
   double hPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenHeight() : DEF_HEIGHT_PIX;

   m_dspGeom.degPerPixelX = m_dspGeom.wDeg / wPix;
   m_dspGeom.degPerPixelY = m_dspGeom.hDeg / hPix;

   // changing the geometry could change the flash spot and location; redraw idle bkg if feature is enabled
   recalcSyncFlashGeometry();
   if(m_syncSpot.size > 0) redrawIdleBackground();
}

/**
 Convert rectangular dimension WxH from pixels to RMVideo's drawing units, i.e., visual degrees subtended at eye.

 This conversion is important to handle rendering of the RMV_IMAGE and _MOVIE targets. Knowing the dimensions of the
 image and movie frame in pixels, we need to use the display geometry to convert them to visual degrees. The 
 calculation is complicated by two different aspect ratios: the screen aspect ratio (screen W in pixels / screen H 
 in pixels), and the "world aspect ratio" computed in visual degrees (screen W in deg / screen H in deg). These are
 generally not the same. To ensure that an NxN-pixel image looks "square" when displayed, I had to experiment to 
 find an approach that seemed to work consistently:
    ImageW_deg = ImageW_pix * ScreenDegPerPix_H;
    ImageW_deg *= screenAR / worldAR;
    ImageH_deg = ImageH_pix * ScreenDegPerPix_V;
 Only the image width is compensated for the aspect ratios. Of course, an NxN image will still look stretched if the
 display geometry is not accurate!!! For example, if you specify screen geometry (W and H in mm) such that the world
 aspect ratio is 4:3 when in reality it is 16:9, a square image (or a square RMV_SPOT target) will appear stretched
 horizontally by 16:9 / 4:3 = 1.333 : 1.

 Another way to compensate for aspect ratio is to adjust the projection matrix and use glOrtho(), but this messes up 
 all other target types that are dimensioned in visual degrees from the beginning.

 @param w [in] The width in pixels; [out] the width in visual degrees.
 @param h [in] The height in pixels; [out] the height in visual degrees.
*/
void CRMVRenderer::convertPixelDimsToDeg(double& w, double& h)
{
   double wPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenWidth() : DEF_WIDTH_PIX;
   double hPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenHeight() : DEF_HEIGHT_PIX;

   double pixAR = wPix / hPix;
   double worldAR = m_dspGeom.wDeg / m_dspGeom.hDeg;

   w = w * pixAR/worldAR;
   w *= m_dspGeom.degPerPixelX;
   h *= m_dspGeom.degPerPixelY;
}

/**
 Convert a distance along X or Y axis from visual deg subtended at eye to device pixels IAW current display geometry.
 @param isX True for X-axis distance, false for Y-axis
 @param val The distance measure in visual degrees along X- or Y-axis.
 @return The corresponding distance in pixels.
*/
double CRMVRenderer::degToPixels(bool isX, double val)
{
   return(val / (isX ? m_dspGeom.degPerPixelX : m_dspGeom.degPerPixelY));
}

/** 
 Update the current display background color. The screen will be cleared to the color specified.

 @param r,g,b [in] Red, green, and blue coordinate of the new background color. Each should be in the range 0.0 (min 
 luminance) to 1.0 (max luminance). Out-of-bounds values are clipped to this range.
*/
void CRMVRenderer::updateBkgColor(double r, double g, double b)
{
   // no changes necessary!
   if( (m_bkgRGB[0] == r) && (m_bkgRGB[1] == g) && (m_bkgRGB[2] == b) ) return;

   m_bkgRGB[0] = (r < 0) ? 0 : ((r>1) ? 1 : r);
   m_bkgRGB[1] = (g < 0) ? 0 : ((g>1) ? 1 : g);
   m_bkgRGB[2] = (b < 0) ? 0 : ((b>1) ? 1 : b);

   redrawIdleBackground();
}

/**
 Update the current settings for the photodiode sync flash spot. If the spot size is changed, the background will be
 redrawn to reflect the change.

 @param sz [in] The new spot size in mm. Silently corrected to the range [RMV_MINSYNCSZ .. RMV_MAXSYNCSZ].
 @param dur [in] The new flash duration in # of frames. Silently corrected to [RMV_MINSYNCDUR .. RMV_MAXSYNCDUR]. 
*/
void CRMVRenderer::updateSyncFlashParams(int sz, int dur)
{
   sz = cMath::rangeLimit(sz, RMV_MINSYNCSZ, RMV_MAXSYNCSZ);
   dur = cMath::rangeLimit(dur, RMV_MINSYNCDUR, RMV_MAXSYNCDUR);
   
   bool redraw = (sz != m_syncSpot.size);
   m_syncSpot.size = sz;
   m_syncSpot.flashDur = dur;

   // if spot size changed, we must recalc spot dimensions in degrees and redraw background to reflect the change
   if(redraw)
   {
     recalcSyncFlashGeometry();
     redrawIdleBackground();
   }
}

/**
 Redraw idle state background: clear screen to current bkg color and draw sync spot if applicable.

 This method must be called whenever the background color changes. In addition, if the sync spot flash feature is
 enabled, the spot patch is always black in the idle state regardless the background color, so the background must 
 also be redrawn whenever the sync spot size changes or the display geometry changes (which can effect spot size).

 In stereo mode, both L and R backbuffers are cleared to the current background color.

 Since the method waits for the next vertical retrace before swapping buffers, it can take as much as one full video
 refresh period to execute.
*/
void CRMVRenderer::redrawIdleBackground()
{
   if(m_pDisplay == NULL) return;

   if(!m_pDisplay->isStereoEnabled())
   {
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      drawSyncFlashSpot();
   }
   else
   {
      glDrawBuffer(GL_BACK_LEFT);
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawSyncFlashSpot();
      glDrawBuffer(GL_BACK_RIGHT);
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawSyncFlashSpot();
   }

   m_pDisplay->swap();
   glFinish();  // stalls here waiting for vertical blanking interval
}

/**
 Load the participating target list for an animation sequence in response to Maestro's "load targets" command. The
 method queries RMVideo's CRMVIo communication link for the number of targets to be animated and their definitions.
 If unable to create one or more of the targets, the target list is cleared and the method fails.

 During initialization, the target objects may draw on the current backbuffer in order to force the OpenGL driver
 to execute certain GL operations that might otherwise be postponed until the GL resource is actually used. An 
 example is texture objects. GL calls to allocate and load a texture object in GPU memory may be postponed by the
 driver until the texture object is actually needed. But texture allocation can be time-consuming, so we do NOT 
 want that to happen during an animation sequence.

 After all targets are initialized, the method calls redrawIdleBackground() to ensure that any "queued" GL operations 
 are executed. This will effectively clear the backbuffer, so any "drawing" done during target inits will be erased,
 and the backbuffer is returned to a known state.

 @return True if all defined targets were successfully created and initialized; false otherwise.
*/
bool CRMVRenderer::loadTargets()
{
   if(m_pDisplay == NULL || m_pDisplay->getIOLink() == NULL) return(false);

   // make sure target list is empty
   unloadTargets();

   // get number of targets defined
   m_nTargets = m_pDisplay->getIOLink()->getNumTargets();

   // allocate target list
   m_pTargetList = (CRMVTarget**) ::calloc(m_nTargets, sizeof(CRMVTarget*));
   if(m_pTargetList == NULL)
   {
      m_nTargets = 0;
      fprintf(stderr, "ERROR(CRMVRenderer): Memory allocation failed. Cannot create target list.\n");
      return(false);
   }
   for(int i=0; i<m_nTargets; i++) m_pTargetList[i] = NULL;

   // retrieve each target's defining parameters and create it
   bool bOk = true;
   RMVTGTDEF tgtDef;
   for(int i=0; bOk && (i<m_nTargets); i++)
   {
      ::memset( &tgtDef, 0, sizeof(RMVTGTDEF) );
      bOk = m_pDisplay->getIOLink()->getTarget(i, tgtDef);
      if(bOk)
      {
         m_pTargetList[i] = new CRMVTarget();
         bOk = (m_pTargetList[i] != NULL);
         if(!bOk) fprintf(stderr, "ERROR(CRMVRenderer): Memory allocation failed. Unable to create target object.\n");
         else bOk = m_pTargetList[i]->initialize(this, tgtDef);
      }
      else fprintf(stderr, "ERROR(CRMVRenderer): Failed to retrieve target definition from RMVideo IO link.\n");
   }

   // redraw idle background, forcing execution of any GL commands issued during target initializations. Since 
   // backbuffer is cleared, any drawing that happened in target inits gets erased. See comments in method header.;
   redrawIdleBackground();

   // clear target list if we failed
   if(!bOk) unloadTargets();

   return(bOk);
}

/** Empty the animated target list prepared for a previous animation sequence.  All target objects destroyed. */
void CRMVRenderer::unloadTargets()
{
   if(m_nTargets > 0)
   {
      if(m_pTargetList != NULL)
      {
         for(int i=0; i<m_nTargets; i++) if(m_pTargetList[i] != NULL)
         {
            delete m_pTargetList[i];
            m_pTargetList[i] = NULL;
         }

         ::free(m_pTargetList);
         m_pTargetList = NULL;
      }

      m_nTargets = 0;
   }

   // since there are no targets, then reset the free space index for the shared vertex array
   m_idxVertexArrayFree = DOTSTOREINDEX;

   // close any video streams that were opened (if any RMV_MOVIE targets were loaded and animated)
   m_vidBuffer.closeAllVideoStreams();
}

/**
 The runtime loop during an animation sequence: Renders RMVideo targets in response to per-frame updates from Maestro.

 After sending the relevant target definitions to RMVideo via the RMV_CMD_LOADTARGETS command, Maestro issues the
 RMV_CMD_STARTANIMATE command to start a frame-by-frame animation sequence. Upon receiving this command, RMVideo enters
 the "animate" state, which is managed entirely in this function. It retrieves the first set of target motion records 
 that come with the _STARTANIMATE command; these define changes in target state at the very beginning of the animation 
 sequence ("t=0"). Using this information, it renders the first display frame on the backbuffer. It then swaps buffers 
 and sends the RMV_SIG_ANIMATEMSG signal to Maestro. With VSync on, the execution thread stalls in the glFinish() call 
 after the buffer swap -- the driver waits for the vertical blanking interval to perform the swap. Thus, this signal is
 sent near the very beginning of the first display frame of the animation sequence -- "t=0". Since Maestro waits for 
 that signal, this scheme permits a *rough* but not perfect synchonization of the RMVideo and Maestro timelines. There
 is uncertainty due to Linux scheduler latency in returning from the wait in glFinish(), as well as the time it takes
 to send the ANIMATEMSG signal to Maestro and for Maestro to respond accordingly. [Because the synchronization cannot
 be exact, we introduced the sync spot flash feature so that researchers can use a photodiode to generate an external
 signal timestamped by Maestro.]

 RMVideo then retrieves the target motion records for the second display frame, which are also sent with the
 STARTANIMATE command. It immediately begins rendering the second display frame on the back buffer. Once the first
 display frame ends, it expects an RMV_CMD_UPDATEFRAME command to be already pending; this will contain the target 
 motion records for the third display frame. The animation sequence proceeds in this manner until Maestro sends 
 RMV_CMD_STOPANIMATE, at which point RMVideo will clear the screen, empty the target list, and return to the idle 
 state. Note that the only valid commands in the "animate" state are UPDATEFRAME and STOPANIMATE (STARTANIMATE is 
 detected in idle mode and processed upon entering animate mode).

 A key design requirement for RMVideo is that it detect duplicate frames, which may occur for two seperate reasons:
 (1) when it takes longer than a refresh period to render a single frame; or (2) when it does not receive the target
 motion update record for frame N by the start of frame N-1. In case (1), an RMV_SIG_ANIMATEMSG is sent with the
 payload [N,X], where N = # elapsed frames in the animation, and X>0 is the number of consecutive duplicate frames
 resulting from the rendering delay (the delay could last several refresh periods). The sequence of skipped frames 
 will have started at frame N-X. In case (2), RMV_SIG_ANIMATEMSG is sent with the payload [N,0]; in this case, frame 
 N+1 will be a duplicate of frame N.

 Finally, RMV_SIG_ANIMATEMSG is sent once per second in the animate state to notify Maestro of the "current" elapsed
 time on the RMVideo side. Here the payload is a single integer N, the # of frames elapsed since the animation sequence
 began. Maestro uses this information to verify that it is staying in sync with RMVideo and not getting too far ahead,
 in which case UPDATEFRAME commands build up in the receive buffer on the RMVideo side. (If Maestro falls behind,
 RMVideo will start missing target updates and send RMV_SIG_ANIMATE[N,0].)

 17jan-11feb2019: Since its inception, RMVideo exhibited a tearing artifact at the top of the screen because it 
 required that VSync be turned off. It tried to perform the swap during the vertical blanking interval by "busy 
 waiting" for the frame counter to increment, repeatedly calling glXGetVideoSync(), then immediately swapping buffers 
 once the counter had incremented. But this approach does not work perfectly, so the tearing artifact was evident at 
 the top of the screen. During testing and development of the OGL 3.3 implementation of RMVideo with a GTX1060 and Dell
 S2417DG display at 2560x1440@144Hz, we tried various strategies to address this problem and enhance performance. In
 the process, we discovered that CElapsedTime used the wrong clock source, resulting in a slight underestimation of the
 monitor's refresh period. We also discovered that RMVideo's execution thread sometimes stalled in glXGetVideoSync(),
 probably because the driver was busy or could not get immediate access to the vertical retrace counter.

 After a lot of testing and trying out various schemes, we decided on a new design strategy for the animation loop. We
 require that VSync be enabled, and rely on the fact that a glFinish() call after glXSwapBuffers() will stall until 
 the next vertical blanking interval, when the swap actually occurs. This stall synchronizes the CPU with the GPU and
 thus limits performance, but we actually WANT to ensure that we render exactly one new frame per refresh. To detect
 a duplicate frame caused by a rendering delay, we rely on our estimate of the refresh period P and compare the actual 
 elapsed time T to the expected elapsed time N*P. If T-N*P ~ P or greater, then a frame skip has occurred!

 Finally, we found that, because our estimate of the refresh period is not perfect, abs(T-N*P) would sometimes grow 
 over the course of a trial. If it was inaccurate enough, this could result in detecting a skipped frame when, in 
 fact, none had occurred. To address this issue, we now recalculate the refresh period as needed over the course of the
 animation sequence.

@return 1 if returning to idle state, 0 if ending command session because RMV_CMD_SHUTTINGDN was received or because the 
RMVideo-Maestro IO link failed, -1 if exiting RMVideo because RMV_CMD_EXIT was received.
*/
int CRMVRenderer::animate()
{
   // this should never happen!
   if(m_pDisplay == NULL || m_pDisplay->getIOLink() == NULL) return(1);

   // for sending RMV_SIG_ANIMATEMSG with payload
   int msg[3];
   msg[0] = RMV_SIG_ANIMATEMSG;

   // update targets IAW frame 0 motion vectors. If something goes wrong, report error in response to STARTANIMATE
   // and immediately return to idle state.
   RMVTGTVEC tgtVec;
   bool bOk = true;
   for(int i=0; i<m_nTargets && bOk; i++)
   {
      bOk = m_pDisplay->getIOLink()->getMotionVector(i, tgtVec);
      if(bOk) bOk = m_pTargetList[i]->updateMotion(0, &tgtVec);
   }
   if(!bOk)
   {
      m_pDisplay->getIOLink()->sendSignal(RMV_SIG_CMDERR);
      return(1);
   }

   // is the sync spot flash feature enabled? (non-zero spot size). If so, should the flash be triggered on frame 0?
   bool bSyncFlashEnabled = (m_syncSpot.size > 0);
   if(bSyncFlashEnabled && m_pDisplay->getIOLink()->isSyncFlashRequested())
      m_syncSpot.nFramesLeft = m_syncSpot.flashDur;

   // we use a high-performance timer to detect skipped frames due to a rendering delay during the animation. To check
   // for a skipped frame, we compare elapsed time T to N*P, where N is the number of frames elapsed and P is our 
   // estimate of the monitor's refresh period
   CElapsedTime elapsedTime;
   int nFrames;
   double dFramePeriodUS = m_dFramePeriod * 1.0e6;

   // if the difference between actual elapsed time T and expected elapsed time N*P starts to grow, then we need to 
   // recalculate the current estimate of the frame period. Our approach is to set a flag when abs(T-N*P) > 50us for 3
   // consecutive frames; once that flag is set, we recalculate P after a future loop iteration in which abs(T-N*P) <=
   // 1.5*(avg diff over the 3 frames) or after at most 5 additional frames. This complication is an effort to avoid
   // recalculating P when T-N*P is relatively large because of a long latency out of glFinish(), since that could 
   // result in an under- or over-estimate of P.
   double adjFramePeriodUS = dFramePeriodUS;
   int nAdjust = 0;
   double accumDiff = 0;

   // the difference T-N*P for the duration of the first display frame. This is the best estimate we can get of any 
   // scheduling latency returning from the glFinish() call that marks "time zero", the start of the first display 
   // frame. We use this estimated delay to correct elapsed time for the remainder of the animation sequence.
   double firstFrameOffsetUS = 0;

   // at this point, we don't know where we are in monitor's refresh cycle. To get sync'd up, we twice clear the back
   // buffer to the current background color and swap. With VSync ON, the glFinish() after the buffer swap should 
   // provide the synchronization. Since the render is simple, hopefully we get close to the start of a refresh cycle.
   if(!m_pDisplay->isStereoEnabled()) 
   {
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      drawSyncFlashSpot();
      m_pDisplay->swap();
      glFinish();
      elapsedTime.reset();

      glClear(GL_COLOR_BUFFER_BIT);
      drawSyncFlashSpot();
      m_pDisplay->swap();
      glFinish();
      elapsedTime.reset();
   }
   else 
   {
      glDrawBuffer(GL_BACK_LEFT);
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawSyncFlashSpot();
      glDrawBuffer(GL_BACK_RIGHT);
      glClearColor((float)m_bkgRGB[0], (float)m_bkgRGB[1], (float)m_bkgRGB[2], 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawSyncFlashSpot();
      m_pDisplay->swap();
      glFinish();
      elapsedTime.reset();

      m_pDisplay->swap();
      glFinish();
      elapsedTime.reset();
   }

   // render frame 0 on the back buffer. Rendering is simply a matter of drawing each target in order. The sync flash 
   // spot is always drawn last so it appears on top. Coord system in degrees subtended at eye, IAW display geometry.
   if(!m_pDisplay->isStereoEnabled()) 
   {
      glClear(GL_COLOR_BUFFER_BIT);
      for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(0.0);
      drawSyncFlashSpot();
   }
   else 
   {
      glDrawBuffer(GL_BACK_LEFT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(-0.5);
      drawSyncFlashSpot();
      glDrawBuffer(GL_BACK_RIGHT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(0.5);
      drawSyncFlashSpot();
   }

   // swap front and back buffers, then call glFinish() to wait for the vertical blank interval. This is "t=0" in the
   // animation timeline -- display frame 0 is now being drawn to the screen. Signal Maestro that the animation has
   // begun.
   m_pDisplay->swap();
   glFinish();
   elapsedTime.reset();
   m_pDisplay->getIOLink()->sendSignal(RMV_SIG_ANIMATEMSG);
   nFrames = 0;

   // elapsed time since we sent RMV_SIG_ANIMATEMSG to notify Maestro of elapsed time thus far. Sent once per second.
   double tLastPingUS = 0;

   // enable video stream buffering now
   m_vidBuffer.startBuffering();

   // here's the frame-by-frame animation:
   float fFrameMS = float(m_dFramePeriod * 1000.0);
   bool bUpdateReady = true;                           // motion vectors for frame 1 are included in 'startAnimate'
   int res = 2;                                        // set result code to 1, -1, or 0 when animation sequence ends
   while(res == 2)
   {
      // update target state/position IAW motion vectors supplied for next frame. If we did not get "updateFrame"
      // command in time, we'll essentially redraw previous frame because the targets' state will be left unchanged.
      if(bUpdateReady)
      {
         bOk = true;
         for(int i=0; i<m_nTargets && bOk; i++)
         {
            bOk = m_pDisplay->getIOLink()->getMotionVector(i, tgtVec);
            if(bOk) bOk = m_pTargetList[i]->updateMotion(fFrameMS, &tgtVec);
         }
         if(!bOk)
         {
            // this is a catastrophic error, so we signal Maestro to terminate the animation sequence
            m_pDisplay->getIOLink()->sendSignal(RMV_SIG_CMDERR);
         }
         
         // if sync spot flash feature enabled, turn it on if requested -- unless it is already on!
         if(bSyncFlashEnabled && m_pDisplay->getIOLink()->isSyncFlashRequested() && m_syncSpot.nFramesLeft <= 0)
            m_syncSpot.nFramesLeft = m_syncSpot.flashDur;
      }

      // render next frame on backbuffer
      if(!m_pDisplay->isStereoEnabled())
      {
         glClear(GL_COLOR_BUFFER_BIT);
         for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(0.0);
         drawSyncFlashSpot();
      }
      else 
      {
         glDrawBuffer(GL_BACK_LEFT);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(-0.5);
         drawSyncFlashSpot();
         glDrawBuffer(GL_BACK_RIGHT);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         for(int i = 0; i < m_nTargets; i++) m_pTargetList[i]->draw(0.5);
         drawSyncFlashSpot();
      }

      // backbuffer now holds the next display frame, so swap front and back buffers during the next vertical blanking
      // interval. With VSync ON, the glFinish() after the buffer swap should stall in the NVidia OpenGL driver until
      // the next vertical blank interval -- with some latency due to task switching and scheduling in the kernel. This
      // is how we sync the animation timeline to the monitor's refresh period.
      m_pDisplay->swap();
      glFinish();

      // at this point, ideally, we are at the start of the next display frame. Get the total elapsed time T, as well
      // as the difference between actual and expected elapsed time N*P, where N is #frames elapsed and P is our
      // current estimate of the refresh period.
      ++nFrames;
      double tNow = elapsedTime.get()*1.0e6 - firstFrameOffsetUS;
      double tDiff = tNow - nFrames*adjFramePeriodUS;
      if(nFrames == 1 && tDiff < -50) firstFrameOffsetUS = tDiff;

      // if the difference between actual and expected elapsed time exceeds one refresh period, then we have 
      // definitely skipped a frame. In practice, if we're still under but within 500us of a full period, then chances
      // are we have skipped a frame. Send message to inform Maestro of the duplicate frame(s).
      int nSkips = 0;
      while(tDiff > adjFramePeriodUS-500)
      {
         ++nSkips;
         ++nFrames;
         tDiff = tNow - nFrames*adjFramePeriodUS;
      }
      if(nSkips > 0)
      {
         msg[1] = nFrames;
         msg[2] = nSkips;
         m_pDisplay->getIOLink()->sendData(3, msg);
      }

      // once per second, notify Maestro of elapsed time since animation sequence began
      if(tNow - tLastPingUS >= 1.0e6)
      {
         msg[1] = nFrames;
         m_pDisplay->getIOLink()->sendData(2, msg);
         tLastPingUS = tNow;
      }

      // recalculate estimate of refresh period when needed. See algorithm description above.
      if(nAdjust < 3)
      {
         if(fabs(tDiff) < 50)
         {
            accumDiff = 0;
            nAdjust = 0;
         }
         else
         {
            accumDiff += tDiff;
            ++nAdjust;
            if(nAdjust == 3) accumDiff /= 3.0;
         }
      }
      else
      {
         ++nAdjust;
         if(fabs(tDiff) < 1.5*accumDiff || nAdjust >= 8)
         {
            adjFramePeriodUS = tNow / nFrames;
            accumDiff = 0;
            nAdjust = 0;
         }
      }

      // we are now ready to work on the next frame; Maestro should have sent us the next "updateFrame" command by now.
      // NOTE that we only retrieve one command per display frame!
      bUpdateReady = false;
      int cmd = m_pDisplay->getIOLink()->getNextCommand();
      if(cmd < RMV_CMD_NONE)
      {
         // the RMVideo-Maestro comm link has failed. Set return code to indicate that command session has ended.
         updateBkgColor(0, 0, 0);
         res = 0;
      }
      switch(cmd)
      {
         case RMV_CMD_UPDATEFRAME :
            bUpdateReady = true;
            break;
         case RMV_CMD_STOPANIMATE :   // animation is over -- go back to idle state
            res = 1;
            break;
         case RMV_CMD_SHUTTINGDN :   // we shouldn't get this, but if we do, command session ended
            updateBkgColor(0, 0, 0);
            res = 0;
            break;
         case RMV_CMD_EXIT :  // time to quit
            res = -1;
            break;
         default :   // illegal command received -- report error
            if(cmd > RMV_CMD_NONE) m_pDisplay->getIOLink()->sendSignal(RMV_SIG_CMDERR);
            break;
      }

      // if we're still animating but we did not get a target update at this point, then the display frame starting at
      // index N+1 will be a duplicate of the frame currently being drawn now (current # elapsed frames is N). Inform 
      // Maestro of the duplicate frame.
      if(res == 2 && !bUpdateReady)
      {
         msg[1] = nFrames;
         msg[2] = 0;
         m_pDisplay->getIOLink()->sendData(3, msg);
      }
   }

   // disable video stream buffering, unload target list and make sure sync spot flash is off
   m_vidBuffer.stopBuffering();
   unloadTargets();
   m_syncSpot.nFramesLeft = 0;

   return(res);
}

/**
 Update the uniform variables in RMVideo's monolithic shader which typically apply to all RMVideo targets: vertex
 transform "xfm", "special" identifier, and number of gratings "nGrats". Each CRMVTarget object will invoke this method
 in its draw() call to update these uniforms IAW its own properties.

 The vertex transform prepared here scales, translates and rotates each 2D vertex to its corresponding location in
 normalized screen coordinates. Here are the operations applied:
 1) Most target types use one of the fixed primitives, in which the vertices are normalized so they can be used for 
 multiple targets. So the first operation is to scale IAW the target's bounding width and height so that the vertices
 are now in logical coordinates (visual deg subtended at eye) WRT the target center.
 2) If the target needs to be rotated about the center, that rotation is applied next.
 3) Next, by translating to the target's center point, the vertex will be in visual deg WRT origin at screen center.
 4) Finally, to get normalized screen coordinates, we scale by (2.0/wDeg, 2.0/hDeg, 1.0), where wDeg and hDeg are the
 screen extents in visual deg.

 @param type [in] Target type. The integer uniform "special" =1 for RMV_IMAGE and RMV_MOVIE, =2 for RMV_RANDOMDOTS, and
 =0 otherwise; the integer uniform "nGrats" is set to 0, 1, or 2 IAW the target type.
 @param x,y [in] Target center coordinates in visual degrees subtended at eye, where (0,0) is the screen center.
 @param w,h [in] Target bounding rectangle width and height. If either <= 0, then target lacks a bounding rectangle.
 @param rot [in] Target rotation in degrees CCW.
*/
void CRMVRenderer::updateCommonUniforms(int type, float x, float y, float w, float h, float rot)
{
   if(m_pShader==NULL) return;

   glm::mat4 xfm(1.0f);
   xfm = glm::scale(xfm, glm::vec3(2.0 / m_dspGeom.wDeg, 2.0 / m_dspGeom.hDeg, 1.0f));
   xfm = glm::translate(xfm, glm::vec3(x, y, 0.0));
   if(rot != 0.0f) xfm = glm::rotate(xfm, glm::radians(rot), glm::vec3(0.0f, 0.0f, 1.0f));
   if(w > 0.0f && h > 0.0f) xfm = glm::scale(xfm, glm::vec3(w, h, 1.0f));
   m_pShader->setMat4("xfm", xfm);

   m_pShader->setInt("special", (type==RMV_IMAGE || type==RMV_MOVIE) ? 1 : (type==RMV_RANDOMDOTS ? 2:0));
   m_pShader->setInt("nGrats", type==RMV_GRATING ? 1 : (type==RMV_PLAID ? 2 : 0));
}

/**
 Update the shader program uniform variable holding the target color, "tgtC". Each CRMVTarget will invoke this method
 (if necessary) in its draw() call to update this uniform's value.

 @param r,g,b [in] Target RGB color; each component normalized in [0.0..1.0]. 
*/
void CRMVRenderer::updateTargetColorUniform(double r, double g, double b)
{
   if(m_pShader != NULL) m_pShader->setVec3("tgtC", glm::vec3(r, g, b));
}

/**
 Update the various shader program uniform variables that govern the rendering of the gratings in an RMV_GRATING or
 RMV_PLAID target. Note that these uniforms are ignored completely for any other target type and need not be set.
 
 Here is the list of uniforms updated in the fragment shader (rmvtarget.fs):
    vec2 ctr;            // current target center in screen coords (pixels WRT origin at TL corner
    int isSine;          // nonzero for sinewave, 0 for squarewave
    vec3 mean0;          // RGB mean color for grating 0 [0..1]
    vec3 con0;           // RGB contrast for grating 0 [0..1]
    vec3 mean1;          // RGB mean color for grating 1
    vec3 con1;           // RGB contrast for grating 1
    vec2 dx;             // projection of X spatial period onto line perpendicular to grating 0 and 1, in pixels
    vec2 dy;             // projection of Y spatial period onto line perpendicular to grating 0 and 1, in pixels
    vec2 phase;          // spatial phase of gratings 0 and 1, in normalized coordinates
 The projection dx = cos(angle)/spatialPerX, and dy = sin(angle)/spatialPerY, where angle is the grating orientation
 in radians and spatialPerX,Y are the grating's X and Y spatial periods in pixels.

 @param x,y [in] Coordinates of target center in degrees subtended at eye.
 @param isSine [in] True if grating(s) are sinusoidal, false for square-wave.
 @param pMean0 [in] Pointer to 3-element buffer [R,G,B] defining the mean color for the first grating. Each color
 component is assumed to be normalized to [0..1].
 @param pCon0 [in] Pointer to 3-element buffer [Cr, Cg, Cb] defining the per-component contrast for the first grating.
 Each component is assumed to be normalized to [0..1], where 1 = 100% contrast.
 @param pMean1, pCon1 [in] Similarly for the second grating.
 @param pAngle [in] Pointer to 2-element buffer holding the orientation angle (in deg) for each grating.
 @param pPeriodX, pPeriodY [in] Pointers to 2-element buffers holding the spatial period along X-axis and Y-axis (in
 pixels) for each grating.
 @param pPhase [in] Pointer to 2-element buffer holding the spatial phase (in deg) for each grating.
*/
void CRMVRenderer::updateGratingUniforms(float x, float y, bool isSine, double* pMean0, double* pCon0, 
      double* pMean1, double* pCon1, float* pAngle, float* pPeriodX, float *pPeriodY, float* pPhase)
{
   if(m_pShader == NULL || m_pDisplay==NULL) return;

   double xScrn = x*m_pDisplay->getScreenWidth()/m_dspGeom.wDeg + m_pDisplay->getScreenWidth()/2.0;
   double yScrn = y*m_pDisplay->getScreenHeight()/m_dspGeom.hDeg + m_pDisplay->getScreenHeight()/2.0;
   m_pShader->setVec2("ctr", glm::vec2(xScrn, yScrn));
   m_pShader->setBool("isSine", isSine);
   m_pShader->setVec3("mean0", glm::vec3(pMean0[0], pMean0[1], pMean0[2]));
   m_pShader->setVec3("con0", glm::vec3(pCon0[0], pCon0[1], pCon0[2]));
   m_pShader->setVec3("mean1", glm::vec3(pMean1[0], pMean1[1], pMean1[2]));
   m_pShader->setVec3("con1", glm::vec3(pCon1[0], pCon1[1], pCon1[2]));

   double proj0 = (pPeriodX[0]<=0) ? 0 : cMath::cosDeg(pAngle[0])/pPeriodX[0];
   double proj1 = (pPeriodX[1]<=0) ? 0 : cMath::cosDeg(pAngle[1])/pPeriodX[1];
   m_pShader->setVec2("dx", glm::vec2(proj0, proj1));

   proj0 = (pPeriodY[0]<=0) ? 0 : cMath::sinDeg(pAngle[0])/pPeriodY[0];
   proj1 = (pPeriodY[1]<=0) ? 0 : cMath::sinDeg(pAngle[1])/pPeriodY[1];
   m_pShader->setVec2("dy", glm::vec2(proj0, proj1));

   m_pShader->setVec2("phase", glm::vec2(cMath::toRadians(pPhase[0]), cMath::toRadians(pPhase[1])));
}

/**
 Bind the specified texture object to texture unit 0.

 All targets rendered in RMVideo use a texture bound to texture unit 0. For image and movie targets, the texture object
 contains the image or movie frame to be rendered onto the target quad. For targets with a non-trivial aperture, the
 texture object defines an alpha texture that, via blending, implements the desired aperture. For all other targets,
 no texture is really needed, but we assign a 4x4 "alpha=1" texture so that we can use a single shader program for all
 rendering.

 Internally, we keep track of the texture object currently bound to texture unit 0 in order to minimize the number of
 times we have to bind/unbind a texture -- particularly during an animation sequence. This method does nothing if the
 texture ID specified matches the ID of the texture object currently bound.

 @param texID [in] GL ID of the texture object to be bound to texture unit 0, which is set as the active texture unit
 in createResources(). However, if 0, then the default "no-op" alpha=1 texture is bound.
*/
void CRMVRenderer::bindTextureObject(unsigned int texID)
{
   if(texID == 0) texID = m_NoOpAlphaMaskID;
   if(texID == m_currBoundTexID) return;
   m_currBoundTexID = texID;
   glBindTexture(GL_TEXTURE_2D, m_currBoundTexID);
}

/**
 Set the current OpenGL point size (applicable only to targets rendered in GL_POINTS mode).
 @param sz [in] The desired point size in pixels.
*/
void CRMVRenderer::setPointSize(int sz) { glPointSize((GLfloat) sz); }

/**
 Draw the primitives stored in the specified segment of the allocated vertex array that is shared among all RMVideo
 targets. Each CRMVTarget object will invoke this method in its draw() call to draw the target IAW its definition,
 after updating the values of any shader uniforms and binding the appropriate texture object. 

 This method is simply a wrapper for glDrawArrays(mode, start, n). The 'mode' is either GL_POINTS, GL_LINES, or 
 GL_TRIANGLES, depending on the flag arguments. No action is taken if start < 0 or start+n > MAXNUMVERTS.

 @param isPts True if target is composed of point primitives (GL_POINTS).
 @param isLine True if target is composed of line primitives (GL_LINES). If isPts = isLine = true, point primitives
 are drawn. If both are false, then triangle primitives are drawn.
 @param start The starting index for a contiguous segment within the global shared vertex array.
 @param count The number of vertices in the contiguous segment. For line or triangle primitives, this should be a
 multiple of 2 or 3, respectively.
*/
void CRMVRenderer::drawPrimitives(bool isPts, bool isLine, int start, int n)
{
   if(start < 0 || start+n > MAXNUMVERTS) return;
   glDrawArrays(isPts ? GL_POINTS : (isLine ? GL_LINES : GL_TRIANGLES), start, n);
}


/**
 Recalculate the logical dimensions of the photodiode sync flash spot.

 The photodiode sync flash spot is a square in the top-left corner of screen. Maestro specifies the spot size in mm.
 But the width and height in logical coordinates (deg subtended at eye) will not be equal generally. This method 
 calculates the width and height of the rectangle in logical coordinates, taking into account the current display
 geometry and screen resolution. It must be called whenever the spot size, geometry or resolution changes!
*/
void CRMVRenderer::recalcSyncFlashGeometry()
{
   double wPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenWidth() : DEF_WIDTH_PIX;
   double hPix = (m_pDisplay != NULL) ? m_pDisplay->getScreenHeight() : DEF_HEIGHT_PIX;
   double sz = m_syncSpot.size;

   m_syncSpot.wDeg = sz * wPix / ((double) m_dspGeom.wMM);
   m_syncSpot.wDeg *= m_dspGeom.degPerPixelX;
   m_syncSpot.hDeg = sz * hPix / ((double) m_dspGeom.hMM);
   m_syncSpot.hDeg *= m_dspGeom.degPerPixelY;
}

/**
 Helper method draws the spot in the top-left corner of the screen that is reserved for the vertical sync flash. If the
 feature is disabled, no action is taken. Otherwise, the spot is filled with black if the flash is off, white if it is
 on -- regardless the current background color.

 This should always be the last drawing operation before swapping buffers at the vertical refresh, since the flash 
 should always be on top of whatever else is being drawn during an animation sequence. It must also be called when 
 updating the background in the idle state.
*/
void CRMVRenderer::drawSyncFlashSpot()
{
   // feature is disabled if the spot size is 0 or if shader program not available
   if(m_syncSpot.size == 0 || m_pShader == NULL) return;

   // transform scales primitive quad to "local" coordinates, then moves origin to the screen's top-left corner in 
   // "local" coordinates, then scales down to normalized coordinates [-1 .. 1] in both X and Y.  Note that, because
   // we put origin at TL corner, only 1/4 of the rectangular spot rendered in visible -- so we double the spot's
   // dimensions to get things right.
   glm::mat4 xfm(1.0f);
   xfm = glm::scale(xfm, glm::vec3(2.0 / m_dspGeom.wDeg, 2.0 / m_dspGeom.hDeg, 1.0f));
   xfm = glm::translate(xfm, glm::vec3(-m_dspGeom.wDeg/2.0, m_dspGeom.hDeg/2.0, 0.0));
   xfm = glm::scale(xfm, glm::vec3(m_syncSpot.wDeg * 2.0, m_syncSpot.hDeg * 2.0, 1.0f));

   // update the uniforms that our monolithic shader needs to render spot. 
   m_pShader->setMat4("xfm", xfm);
   m_pShader->setVec3("tgtC", glm::vec3(m_syncSpot.nFramesLeft > 0 ? 1.0f : 0.0f));
   m_pShader->setInt("special", 0);
   m_pShader->setInt("nGrats", 0);

   // render the sync spot
   bindTextureObject(m_NoOpAlphaMaskID);
   glDrawArrays(GL_TRIANGLES, QUADINDEX, QUADCOUNT);

   // if flash is on, decrement #frames remaining
   if(m_syncSpot.nFramesLeft > 0) --m_syncSpot.nFramesLeft;
}

/**
 Helper method for createResources(): Creates and loads the "no-op" alpha mask texture, a 4x4 texture with alpha=1 for
 all texels. It is assigned to any target that is neither an image nor movie nor requires an alpha mask. We need it 
 because our monolithic shader program applies an alpha texture to every target that is not an image or movie.
 @return True if texture object was created; false otherwise.
*/
bool CRMVRenderer::generateNoOpAlphaMaskTexture()
{
   glGenTextures(1, &m_NoOpAlphaMaskID);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, m_NoOpAlphaMaskID);
   int texWPix = 4, texHPix = 4;
   
   // try loading texture via proxy target to see if GL has enough texture memory
   glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RED, texWPix, texHPix, 0, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)m_pMaskTexels);
   GLint actualLen = 0;
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualLen);
   if(actualLen == 0)
   {
      glBindTexture(GL_TEXTURE_2D, 0);
      return(false);
   }

   // load the texture into memory with alpha=1 (255) for all texels
   for(int j = 0; j<texHPix; j++)
   {
      int k = j * texWPix;
      for(int i=0; i<texWPix; i++) m_pMaskTexels[k + i] = (GLubyte) 255;
   }
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texWPix, texHPix, 0, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)m_pMaskTexels);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glBindTexture(GL_TEXTURE_2D, 0);

   return(true);
}

/**
 Helper method for createResources(): Allocates the vertex array and backing buffer that is shared among all targets
 currently being animated.

 For any RMVideo target, the vertex array stores two attributes per vertex: the 2D vertex location (x,y) in normalized
 space, and the corresponding 2D texel location (Tx, Ty) in normalized texture space. For most target types, only 6
 vertices are required, defining two triangles that form a quad. The RMV_POINT target has just a single vertex, and the
 zero-width RMV_BAR target needs two vertices defining a vertical line segment. Furthermore, these three primitives 
 (quad, point, line) are fixed -- the vertex attributes never change. Only the RMV_FLOWFIELD and RMV_RANDOMDOTS targets
 potentially require lots of vertices (one vertex per dot), and these vertices must be updated on a frame-by-frame
 basis (GL_DYNAMIC_DRAW option).

 To avoid frequent vertex array/buffer object allocations and deallocations, as well as the overhead of binding and
 unbinding vertex arrays, we preallocate a single large vertex array that is shared by all targets being animated. The
 array can handle up to 50K vertices, which is likely way more than will ever be used. Four fixed fixed primitives
 (point, line, quad, and an inverted quad for movie frame image textures) are stored at the beginning of the array. 
 Again, they never change and so can be assigned to multiple targets; the normalized vertex coordinates are transformed
 in a target-specific way in the vertex shader. Each flowfield or random-dot patch target are assigned their own 
 segment (start index, count) in the vertex array and vertex data is uploaded to that segment prior to each frame draw.

 NOTE: Generally it is not recommended that static vertex data be stored in the same buffer as dynamic data, but that
 is because techniques for optimizing the buffer updates often involve buffer orphaning and re-specification. However,
 RMVideo requires that frame N-1 be completely rendered on the backbuffer before starting work on frame N. That is,
 RMVideo explicitly synchronizes the GPU and CPU at the end of every frame. So we don't have to worry about an
 implicit synchronization happening during a buffer update.
 
 @return True if successful; false otherwise.
*/
bool CRMVRenderer::allocateSharedVertexArray()
{ 
   // allocated vertex array and backing buffer for up to MAXNUMVERTS vertices with attributes {x,y}, {Tx,Ty}
   glGenVertexArrays(1, &m_idVAO);
   glGenBuffers(1, &m_idVBO);

   glBindVertexArray(m_idVAO);
   glBindBuffer(GL_ARRAY_BUFFER, m_idVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float)*MAXNUMVERTS*4, NULL, GL_DYNAMIC_DRAW);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
   glEnableVertexAttribArray(1);

   // store the 4 fixed primitives at the start of the array
   float verts[] = { 
      // two-triangle quad centered at origin: TR, BR, TL; BR, BL, TL. Start index = 0, count = 6.
      0.5f, 0.5f,      1.0f, 1.0f,
      0.5f, -0.5f,     1.0f, 0.0f,
      -0.5f, 0.5f,     0.0f, 1.0f,
      0.5f, -0.5f,     1.0f, 0.0f,
      -0.5f, -0.5f,    0.0f, 0.0f,
      -0.5f, 0.5f,     0.0f, 1.0f,

      // two-triangle quad at origin, w/inverted texture coords: TR, BR, TL; BR, BL, TL. Start index = 6, count = 6.
      // for RMV_MOVIE only: Because video frames are stored such that the "top-left corner" is the origin, we adjust
      // the texture coordinates so that the frame appears right-side up when mapped to the quad.
      0.5f, 0.5f,      1.0f, 0.0f,
      0.5f, -0.5f,     1.0f, 1.0f, 
      -0.5f, 0.5f,     0.0f, 0.0f,
      0.5f, -0.5f,     1.0f, 1.0f,
      -0.5f, -0.5f,    0.0f, 1.0f,
      -0.5f, 0.5f,     0.0f, 0.0f,

      // for zero-width RMV_BAR only: vertical line segment. Start index = 12, count = 2.
      0.0f, 0.5f,      0.5f, 1.0f,
      0.0f, -0.5f,     0.5f, 0.0f,

      // for RMV_POINT only: a single vertex. Start index = 14, count = 1.
      0.0f, 0.0f,      0.0f, 0.0f,
   };
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   // check to see if any of the preceding GL calls failed, in which case we assume the vertex array is not available
   bool ok = (glGetError() == GL_NO_ERROR);

   // the free portion of the vertex array starts immediately after the last fixed primitive
   m_idxVertexArrayFree = DOTSTOREINDEX;

   return(ok);
}


/**
 Destroy the managed pool of texture objects used to prepare alpha mask, RGBA image, and RGB movie frame textures for
 an RMVideo target. Call this method before RMVideo exits.
*/
void CRMVRenderer::destroyTexturePool()
{
   while(m_texPoolHead != NULL)
   {
      TexNode* pDead = m_texPoolHead;
      m_texPoolHead = pDead->pNext;

      glDeleteTextures(1, &(pDead->id));
      delete pDead;
   }
   m_texPoolSize = 0;
   m_texPoolBytes = 0.0;
}

/**
 Find an unused OpenGL texture object in CRMVRenderer's texture pool that matches the requirements specified, or 
 allocate a new texture object and append it to the pool.

 It is likely that the pool will not contain an existing texture object that matches the requested dimensions exactly
 -- particularly for the RGBAIMAGETEX and RGBIMAGETEX textures backing an RMV_IMAGE and RMV_MOVIE target, respectively.
 In that situation, the method checks the total bytes of texture memory in the pool. If that exceeds 50MB, then it
 will cull unused textures to bring the byte count below 45MB (if possible) before allocating the new texture object.

 @param type The requested texture type, one of ALPHAMASKTEX, RGBAIMAGETEX, and RGBIMAGETEX.
 @param w,h The requested texture width and height in pixels/texels.
 @return A texture pool node containing information about an allocated OpenGL texture object that matches the type
 and dimensions specified. The node is marked as "in use".
*/
CRMVRenderer::TexNode* CRMVRenderer::getTextureNodeFromPool(int type, int w, int h)
{
   // find an available texture object of the right type with dimensions matching those requested
   TexNode* pNode = m_texPoolHead;
   TexNode* pParent = NULL;
   while(pNode != NULL && (pNode->inUse || pNode->type != type || pNode->width != w || pNode->height != h))
   {
      pParent = pNode;
      pNode = pParent->pNext;
   }

   // if there was no available texture node matching the requirements, AND there's too much texture memory already
   // allocated in the pool, delete some unused textures...
   if(pNode == NULL && m_texPoolBytes > 5.0e7)
   {
      TexNode* pCurr = m_texPoolHead;
      pParent = NULL;
      while(pCurr != NULL && m_texPoolBytes > 4.5e7)
      {
         if(!pCurr->inUse)
         {
            TexNode* pDead = pCurr;
            pCurr = pDead->pNext;
            if(pParent != NULL) pParent->pNext = pCurr;
            else m_texPoolHead = pCurr;
            
            double byteCount = pDead->width * pDead->height;
            byteCount *= (pDead->type==RGBAIMAGETEX) ? 4 : (pDead->type==RGBIMAGETEX ? 3 : 1);

            glDeleteTextures(1, &(pDead->id));
            delete pDead;
            m_texPoolBytes -= byteCount;
            --m_texPoolSize;
         }
         else
         {
            pParent = pCurr;
            pCurr = pCurr->pNext;
         }
      }
   }

   // no available texture object matches the requirements, so allocate a new one that does and append it to the pool
   if(pNode == NULL)
   {
      pNode = new TexNode;
      if(pNode != NULL)
      {
         unsigned int texID = 0;
         glGenTextures(1, &texID);
         bindTextureObject(texID);
         GLint internFmt = (type==ALPHAMASKTEX) ? GL_RED : (type==RGBAIMAGETEX ? GL_RGBA8 : GL_RGB8);
         GLenum fmt = (type==ALPHAMASKTEX) ? GL_RED : (type==RGBAIMAGETEX ? GL_RGBA : GL_RGB);
         GLint actualLen = 0;
         glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internFmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, (GLvoid*)NULL);
         glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualLen);
         if(actualLen == 0)
         {
            delete pNode;
            pNode = NULL;
            bindTextureObject(0);
            glDeleteTextures(1, &texID);
         }
         else
         {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, internFmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, (GLvoid*)NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            bindTextureObject(0);

            pNode->type = type;
            pNode->id = texID;
            pNode->width = w;
            pNode->height = h;
            pNode->inUse = false;
            pNode->pNext = NULL;

            // append new node to end of singly linked list. The list could be empty!
            if(pParent == NULL) m_texPoolHead = pNode;
            else pParent->pNext = pNode;
            double byteCount = pNode->width * pNode->height;
            byteCount *= (pNode->type==RGBAIMAGETEX) ? 4 : (pNode->type==RGBIMAGETEX ? 3 : 1);
            m_texPoolBytes += byteCount;
            ++m_texPoolSize;
         }
      }
   }

   // if successful, mark the texture node as in use
   if(pNode != NULL) pNode->inUse = true;
   return(pNode);
}

