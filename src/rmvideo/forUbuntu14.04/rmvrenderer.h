//=====================================================================================================================
//
// rmvrenderer.h : Declaration of class CRMVRenderer, which handles all OpenGL rendering in RMVideo.
//
//=====================================================================================================================


#if !defined(RMVRENDERER_H_INCLUDED_)
#define RMVRENDERER_H_INCLUDED_

#include "shader.h"                    // shader program support
#include "vidbuffer.h"                 // helper class buffers video on a background thread
#include "rmvideo_common.h"            // basic constants/definitions shared w/Maestro
#include "rmvtarget.h"                 // CRMVTarget -- Defines a generic RMVideo target.

class CRMVDisplay;                     // forward declaration


class CRMVRenderer
{
private: 
   // the vertex and fragment shader source code in string form
   static const char* VERTEXSHADERSRC;
   static const char* FRAGMENTSHADERSRC;

public:
   CRMVRenderer();
   virtual ~CRMVRenderer();

   // create GLSL shader programs and any other resources needed for target rendering. Invoke during startup.
   bool createResources(CRMVDisplay* pDsp);

   // release GLSL shader programs and other target rendering resources. Invoke at shutdown.
   void releaseResources();

   // reserve a contiguous portion of the shared vertex array for streaming vertex attributes
   int reserveSharedVertexArraySegment(int n);
   
   // upload vertex attributes to a specified portion of the shared vertex array (dot targets only)
   void uploadVertexData(int start, int count, float* pSrc);

   // prepare alpha mask texture object
   unsigned int prepareAlphaMaskTexture(int aperture, double w, double h, double iw, double ih, 
         double sigX, double sigY);

   // retrieve image from a specified source file in the RMVideo media store
   unsigned char* getImage(const char* folder, const char* file, int& w, int& h);

   // prepare texture object to hold image or movie frame
   unsigned int prepareImageTexture(bool rgba, int w, int h, unsigned char* pImg);

   // release an OpenGL texture object previously prepared via one of the prepare***Texture() methods
   void releaseTexture(unsigned int texID);

   // return # of KBs of texture memory currently reserved in the renderer's texture object pool
   double getTexturePoolKB() { return(m_texPoolBytes/1024.0); }
   // return total # of texture objects currently reserved in the renderer's texture object pool
   int getTexturePoolSize() { return(m_texPoolSize); }

   // uploads a movie frame to the specified texture object
   void uploadMovieFrameToTexture(unsigned int texID, int w, int h, unsigned char* pFrame);

   // obtain a precise measure of the monitor's vertical refresh period (over a 500-frame epoch)
   bool measureFramePeriod();
   
   // the monitor's vertical refresh period in seconds (with sub-microsec accuracy, hopefully!)
   double getFramePeriod() { return(m_dFramePeriod); }
   
   // update the current display geometry
   void updateDisplayGeometry(int w, int h, int d);

   // convert rectangular dimensions from pixels to visual deg subtended at eye, accounting for aspect ratio
   void convertPixelDimsToDeg(double& w, double& h);

   // convert a distance along X or Y axis from visual deg subtended at eye to device pixels
   double degToPixels(bool isX, double val);

   // the current background color 
   void getBkgColor(double& r, double& g, double& b)
   {
      r = m_bkgRGB[0]; g = m_bkgRGB[1]; b = m_bkgRGB[2];
   }
   void updateBkgColor(double r, double g, double b); 
   
   // update parameters for photodiode sync flash 
   void updateSyncFlashParams(int sz, int dur);

   // redraw idle state background: clear screen to current bkg color and draw sync spot if applicable
   void redrawIdleBackground();

   // load or unload the list of target participating an animation sequence
   bool loadTargets(); 
   void unloadTargets(); 
   int getNumTargetsLoaded() { return(m_nTargets); }

   // the runtime loop during an animation sequence
   int animate();

   // helper methods called by CRMVTarget to render a target
   void updateCommonUniforms(int type, float x, float y, float w, float h, float rot);
   void updateTargetColorUniform(double r, double g, double b);
   void updateGratingUniforms(float x, float y, bool isSine, double* pMean0, double* pCon0, 
      double* pMean1, double* pCon1, float* pAngle, float* pPeriodX, float *pPeriodY, float* pPhase);
   void bindTextureObject(unsigned int texID);
   void setPointSize(int sz);
   void drawPrimitives(bool isPts, bool isLine, int start, int n);

private:
   // a reference to the RMVideo display manager (to access display info, comm link, perform front-back buffer swap)
   CRMVDisplay* m_pDisplay;

   // the shader program used for all rendering in RMVideo 
   Shader* m_pShader;

   // texture ID for the default "no-op" alpha mask texture (4x4, alpha=1 for all texels)
   unsigned int m_NoOpAlphaMaskID;

   static const int MAXTEXMASKDIM;     // limit to either dimension of an alpha mask texture
   GLubyte* m_pMaskTexels;             // buffer for computing alpha mask textures

   // texture object pool intended to avoid frequent allocation/deallocation of texture objects. Implemented as a 
   // singly-linked list. Note that there are 3 distinct kinds of texture objects: alpha mask textures, RGBA image
   // textures, and RGB image textures for movie frames.
   static const int ALPHAMASKTEX;
   static const int RGBAIMAGETEX;
   static const int RGBIMAGETEX;
   struct TexNode
   {
      int type;
      int width;
      int height;
      unsigned int id;
      bool inUse;
      TexNode* pNext;
   };
   TexNode* m_texPoolHead;
   
   int m_texPoolSize;                  // total number of textures in pool
   double m_texPoolBytes;              // total number of bytes of texture memory reserved in pool
   
   // GL IDs for vertex array and backing buffer shared across all targets being animated
   unsigned int m_idVAO;
   unsigned int m_idVBO;

   // starting index of the unused portion of the vertex array shared across all targets being animated.
   int m_idxVertexArrayFree;

   static const int MAXNUMVERTS;       // maximum number of vertices than can be stored in shared vertex array
public:
   static const int QUADINDEX;         // start index of fixed quad primitive in shared vertex array
   static const int QUADCOUNT;         // vertex count for fixed quad primitive
   static const int VIDQUADINDEX;      // start index of fixed quad primitive for video frame image texture
   static const int VIDQUADCOUNT;      // vertex count for fixed quad primitive for video frame image texture
   static const int VLINEINDEX;        // start index of fixed vertical line primitive
   static const int VLINECOUNT;        // vertex count for fixed vertical line primitive
   static const int POINTINDEX;        // start index of fixed single-point primitive
   static const int POINTCOUNT;        // vertex count for fixed single-point primitive
   static const int DOTSTOREINDEX;     // starting index for dot primitives in shared vertex array

   // this video helper manages a dedicated background thread that buffers video for any movie target. It is
   // publicly accessible so that RMV_MOVIE target objects can access it directly.
   CVidBuffer m_vidBuffer;

private:
   // ID of texture object currently bound to texture unit 0 (we only use 1 texture unit). If 0, no binding.
   unsigned int m_currBoundTexID;

   // the current vertical refresh period in seconds (measured over a 500-frame epoch)
   double m_dFramePeriod;

   // parameters governing the geometry and logical coorindates for the fullscreen display window
   struct DspGeom
   {
      int wMM, hMM, dMM;     // width, height of window; distance from subject's eye to screen center (mm)
      double wDeg, hDeg;     // width and height of window in logical coordinates (visual deg subtended at eye)
      double degPerPixelX;   // for converting between logical and device coordinates (pixels)
      double degPerPixelY;
   };
   DspGeom m_dspGeom;

   static const int DEF_WIDTH;         // default window geometry
   static const int DEF_HEIGHT;
   static const int DEF_DISTTOEYE;
   static const int DEF_WIDTH_PIX;
   static const int DEF_HEIGHT_PIX;

   // the current RGB color for background
   double m_bkgRGB[3];

   // params defining synchronization flash in TL corner as stimulus for photodiode to detect start of frame
   struct SyncSpot
   {
      int size;            // spot size in mm; 0 = feature disabled
      int flashDur;        // flash duration in # of video frames
      double wDeg, hDeg;   // spot dimensions in logical coordinates (visual deg subtended at eye)
      int nFramesLeft;     // number of frames left before sync spot flash is extinguished
   };
   SyncSpot m_syncSpot;

   // current list of targets participating in an animation sequence
   int m_nTargets;
   CRMVTarget** m_pTargetList;

private:
   // recalculate logical dimensions of the photodiode sync flash spot
   void recalcSyncFlashGeometry();

   // render the photodiode sync flash spot
   void drawSyncFlashSpot();

   // create the default "no-op" alpha mask texture 
   bool generateNoOpAlphaMaskTexture();

   // allocate and initialize the vertex array and backing buffer shared across all targets being animated
   bool allocateSharedVertexArray();

   // manage a pool of texture objects used for alpha mask, RGBA image, and RGB movie frame textures
   void destroyTexturePool();
   TexNode* getTextureNodeFromPool(int type, int w, int h);
};


#endif // !defined(RMVRENDERER_H_INCLUDED_)
