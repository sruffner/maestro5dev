//=====================================================================================================================
//
// rmvdisplay.h : Declaration of class CRMVDisplay
//
//=====================================================================================================================


#if !defined(RMVDISPLAY_H_INCLUDED_)
#define RMVDISPLAY_H_INCLUDED_

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1     // so we get function prototypes for GL extensions
#endif
#include <GL/gl.h>

#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1   // so we get function prototypes for GLX extensions
#endif
#include <GL/glx.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/xf86vmode.h>

#include "rmvio.h"            // CRMVIo -- Defines the communication link with Maestro.
#include "rmvrenderer.h"      // CRMVRenderer -- Helper class handles all OpenGL rendering in idle and animate states
#include "rmvmediamgr.h"      // CRMVMediaMgr -- Helper class manages the RMVideo media store.
#include "utilities.h"

class CRMVDisplay
{
public:
   CRMVDisplay();                                              // constructor
   ~CRMVDisplay();                                             // destructor

   void start( bool useEmulator );                             // starts the display manager after construction

   int getScreenWidth() { return(m_iWidthPix); }
   int getScreenHeight() { return(m_iHeightPix); }

   CRMVIo* getIOLink() { return(m_pIOLink); }
   void swap() { glXSwapBuffers(m_pDisplay, m_window); }

   CRMVMediaMgr* getMediaStoreManager() { return(&mediaMgr); }

   bool isStereoEnabled() { return(m_bStereoEnabled); }        // is stereo mode enabled for dot disparity feature?

private:
   static const int STATE_OFF;                                 // op state: off, waiting for start of cmd session
   static const int STATE_DYING;                               //    about to exit
   static const int STATE_IDLE;                                //    executing cmds, but not currently animating
   static const int STATE_ANIMATE;                             //    animating targets (Maestro in Trial or Cont mode)

   CRMVMediaMgr mediaMgr;                                      // media store manager
   CRMVRenderer m_renderer;                                    // handles all rendering in idle and animate states

   Display* m_pDisplay;                                        // the X display
   Window m_window;                                            // our fullscreen window
   Cursor m_blankCursor;                                       // we "hide" cursor by giving window a blank one!
   bool m_bWindowCreated;                                      // true when fullscreen window has been created
   bool m_bWindowDisplayed;                                    // true when fullscreen window is displayed
   GLXContext m_glxContext;                                    // the GLX rendering context assoc. with window
   XVisualInfo* m_pXVInfo;                                     // video configuration
   bool m_bStereoEnabled;                                      // true if stereo mode enabled

   struct VideoMode                                            // information on a video mode
   {
      int wPix;                                                //    screen width in pixels
      int hPix;                                                //    screen height in pixels
      short rate;                                              //    screen refresh rate in Hz (nearest whole #)
      RRMode modeID;                                           //    RandR mode ID
   };
   bool m_bAltVideoModesSupported;                             // flag set iff alternate video modes supported.
   XRRScreenResources* m_pScreenRes;                           // X screen resources and other information needed
   XRROutputInfo* m_pOutInfo;                                  //   to query and switch video modes
   XRRCrtcInfo* m_pCrtcInfo;
   int m_numVideoModes;                                        // list of video modes supporting minimum reqmts.
   VideoMode* m_pVideoModes;                                   //
   RRMode m_originalModeID;                                    // mode ID for original video mode (to restore at exit)
   int m_idxCurrVideoMode;                                     // the current video mode (index into mode list)

   int m_iWidthPix;                                            // current screen resolution in pixels
   int m_iHeightPix;

   CRMVIo* m_pIOLink;                                          // communication link with Maestro
   int m_iState;                                               // current operational state

   bool openDisplay();                                         // create all resources needed to run fullscreen display
   void enumerateVideoModes();                                 // get available video modes using RandR
   void createFullscreenWindow();                              // creates the fullscreen display window
   void closeDisplay();                                        // free all previously created resources
   void showDisplay(bool bShow);                               // show/hide our fullscreen display window

   bool checkGLXExtension( const char* extName );              // check availability of a GLX extension on host machine
   bool checkGLExtension( const char* extName );               // check availability of a GL extension on host machine
   bool enableSyncToVBlank();                                  // SyncToVBlank must be enabled for proper operation

   void idle();                                                // runtime loop in idle state
   void getAllVideoModes();                                    // helper methods for idle()
   void getCurrentVideoMode();
   void setCurrentVideoMode();
   void getGamma();
   void setGamma();
};


#endif // !defined(RMVDISPLAY_H_INCLUDED_)
