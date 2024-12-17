//=====================================================================================================================
//
// rmvtarget.h : Declaration of class CRMVTarget, representing any RMVideo target.
//
//=====================================================================================================================


#if !defined(RMVTARGET_H_INCLUDED_)
#define RMVTARGET_H_INCLUDED_

#include "utilities.h"                 // utility classes
#include "rmvideo_common.h"            // basic constants/definitions shared w/Maestro

class CRMVRenderer;                    // forward declaration

class CRMVTarget
{
public:
   // create/destroy a pool of buffers used for vertex attributes and other storage for dot-patch targets
   static bool createBufferPool();
   static void destroyBufferPool();

private:
   // a buffer pool for dot storage arrays (singly-linked list)
   struct FloatBufNode
   {
      int size;
      bool inUse;
      float* pBuf;
      FloatBufNode* pNext;
   };
   static FloatBufNode* g_FloatBufPool;

   // get a buffer for dot storage from buffer pool, allocating new one if necessary.
   static FloatBufNode* getBufferNodeFromPool(int sz);
   // release a buffer back to buffer pool
   static void releaseBufferNodeToPool(FloatBufNode* pNode);

public:
   CRMVTarget();
   ~CRMVTarget();

   // prepare target object for an animation sequence
   bool initialize(CRMVRenderer* pRenderer, const RMVTGTDEF& tgtDef);

   // update target's internal rep IAW specified motion. Returns false if animation seq should terminate on error.
   bool updateMotion(float tElapsed, PRMVTGTVEC pVec);

private:
   // updateMotion() helper methods
   void updateFlickerState();
   void updatePlaid(PRMVTGTVEC pVec);
   void updateRandomDots(float tElapsed, PRMVTGTVEC pVec);
   void updateFlowField(PRMVTGTVEC pVec);
   bool updateMovie(float tElapsed, PRMVTGTVEC pVec);

public:
   // render target IAW current state
   void draw(float eye);

   bool isOn(); 
   void setOn(bool bOn);

private:
   CRMVRenderer* m_pRenderer;          // reference to target renderer singleton
   RMVTGTDEF m_tgtDef;                 // target's defining parameters
   bool m_bOn;                         // flag indicates whether target is currently on/off
   CFPoint m_centerPt;                 // current location of target window's center point
   double m_rgb0[3];                   // RGB color 0; grating 0 mean color for RMV_GRATING, _PLAID (R=0,G=1,B=2)
   double m_rgb1[3];                   // RGB color 1 for RMV_RANDOMDOTS, grating 1 mean color for RMV_PLAID
   double m_rgbCon0[3];                // grating 0 contrast for RMV_GRATING, _PLAID
   double m_rgbCon1[3];                // grating 1 contrast for RMV_PLAID
   bool m_isTwoColor;                  // true for RMV_RANDOMDOTS two-color contrast mode

   // flicker status and duration countdown (in # of video frames)
   static const int FLICKER_DISABLED  = -1;     // target does not flicker
   static const int FLICKER_RESET     = 0;      // flicker state reset (target on/off flag is "off")
   static const int FLICKER_INITDELAY = 1;      // in the initial delay prior to first flicker ON phase
   static const int FLICKER_ON        = 2;      // in flicker ON phase
   static const int FLICKER_OFF       = 3;      // in flicker OFF phase

   int m_flickerState;
   int m_flickerFramesLeft;            // #video frames remaining in the current flicker phase

   // ID of assigned texture object, or 0 if texture not needed. For RMV_IMAGE or RMV_MOVIE, this will be an RGBA or
   // RGB texture. For targets with non-rectangular aperture and/or Gaussian blur, it will be a single-component
   // texture defining the target's "alpha mask".
   unsigned int m_texID;

   // start index and size of segment in OpenGL renderer's shared vertex array that's dedicated to this target
   int m_vtxArrayStart;
   int m_vtxArrayCount;

   // additional animation state information and resources for select target types...
   FloatBufNode* m_pfBufDots;             // RMV_RANDOMDOTS, _FLOWFIELD: vertex attrs {x, y, Tx, Ty}
   FloatBufNode* m_pfBufDotLives;         // RMV_RANDOMDOTS: current per-dot lifetimes, if applicable
   FloatBufNode* m_pfBufDotNoise;         // RMV_RANDOMDOTS: current per-dot noise factors, if applicable
   CRandomNG* m_pDotRNG;                  // RMV_RANDOMDOTS, _FLOWFIELD: for randomizing dot pos and other uses
   CRandomNG* m_pNoiseRNG;                // RMV_RANDOMDOTS: for speed/directional noise feature
   float m_tUntilNoiseUpdate;             // RMV_RANDOMDOTS: time until next noise update, in ms
 
   static const int MINGRATCYCLE = 8;     // minimum supported # pixels per grating cycle
   float m_fSpatialPerX[2];               // RMV_GRATING, _PLAID: grating spatial period along X axis
   float m_fSpatialPerY[2];               // RMV_GRATING, _PLAID: grating spatial period along Y axis
   float m_fCurrOrient[2];                // RMV_GRATING, _PLAID: current grating orientation (CCW from horiz, in deg)
   float m_fCurrPhase[2];                 // RMV_GRATING, _PLAID: current spatial phase of each grating

   static const int NUMPBOS = 3;          // RMV_MOVIE: Pixel buffer objects for uploading video frames to texture
   unsigned int m_pboIDs[NUMPBOS];
   int m_iCurrPBOIdx;                     // index of PBO currently being uploaded to texture

   int m_videoStreamID;                   // RMV_MOVIE: ID of open video stream
   int m_iMovieState;                     // RMV_MOVIE: playback state:
   static const int MOVIE_UNINITIALIZED = 0;  // need to open source file and prepare for playback
   static const int MOVIE_NOTSTARTED    = 1;  // initialized, but playback has not yet begun
   static const int MOVIE_NEEDFRAME     = 2;  // playback in progress: need next frame
   static const int MOVIE_GOTFRAME      = 3;  // playback in progress: frame ready to be drawn
   static const int MOVIE_DONE          = 4;  // playback completed (or failed)

   bool m_gotLastFrame;                   // RMV_MOVIE: set when next frame is the last frame (if movie does not loop)
   double m_tElapsed;                     // RMV_MOVIE: movie playback elapsed time in ms
   double m_tNextFrame;                   // RMV_MOVIE: elapsed time at which we should retrieve next frame, in ms
   double m_tPlaybackIntv;                // RMV_MOVIE if non-positive, playback at display update rate; else this 
                                          // is the movie's ideal frame interval in ms.

   bool validateTargetDef();           // validate target definition and range-limit various parameters
   void initTargetColors();            // convert target colors from packed RGB to normalized R,G,B components

   // allocate additional resources required to render/animate target (depends on target type)
   bool allocateResources();
   void freeResources();

   // for RMV_RANDOMDOTS, _FLOWFIELD: randomize dot location
   void randomizeDotPos(float& x, float& y);
   void randomizeDotPosInFlowField(float& x, float& y);
};


#endif // !defined(RMVTARGET_H_INCLUDED_)
