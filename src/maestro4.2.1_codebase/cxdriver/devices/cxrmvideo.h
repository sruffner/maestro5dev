//=====================================================================================================================
//
// cxrmvideo.h : Declaration of CCxRMVideo, representing Maestro's remote framebuffer video server, RMVideo.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXRMVIDEO_H__INCLUDED_)
#define CXRMVIDEO_H__INCLUDED_

#include <winsock2.h>               // Windows Sockets API
#include "rtapi.h"                  // the RTX API
#include "cxobj_ifc.h"              // general Maestro target-related constants
#include "device.h"                 // CDevice -- base class for CNTRLX device interfaces
#include "rmvideo_common.h"         // defines shared between RMVideo and Maestro

#include "cxmasterio.h"             // CCxMasterIO-- used by OpenEx


#define RMV_MAXCMDSIZE 2053         // longest command/reply (# of 32bit ints) we can send to/receive from RMVideo

//=====================================================================================================================
// Declaration of class CCxRMVideo
//=====================================================================================================================
//
class CCxRMVideo : public CDevice
{
public:
   CCxRMVideo();                                         // constructor
   ~CCxRMVideo();                                        // destructor

   int RTFCNDCL GetVersion();                            // get RMVideo application version number
   
   double RTFCNDCL GetFramePeriod();                     // get RMVideo monitor frame period in sec, with ns precision
   int RTFCNDCL GetScreenW_pix();                        // get RMVideo monitor display width, in pixels
   int RTFCNDCL GetScreenH_pix();                        // get RMVideo monitor display height, in pixels

   int RTFCNDCL GetNumModes();                           // get number of available video display modes
   BOOL RTFCNDCL GetModeInfo(int n,                      // get display mode info: w x h in pixels, frame rate in Hz
         int& w, int& h, int& rate);
   int RTFCNDCL GetCurrentMode();                        // get/set the current video display mode
   BOOL RTFCNDCL SetCurrentMode(int n);
   
   BOOL RTFCNDCL GetGeometry(int& d, int& w, int& h);    // get/set RMVideo display geometry in mm
   BOOL RTFCNDCL SetGeometry(int d, int w, int h);
   double RTFCNDCL GetDegToPix();                        // retrieve deg->pix conversion factor
   double RTFCNDCL GetScreenW_deg();                     // get RMVideo monitor display width, in deg subtended at eye
   double RTFCNDCL GetScreenH_deg();                     // get RMVideo monitor display height, in deg subtended at eye

   VOID RTFCNDCL GetMonitorGamma(int &r, int&g, int &b); // get/set RMVideo monitor gamma correction factors
   BOOL RTFCNDCL SetMonitorGamma(int r, int g, int b);
   
   // get/set RMVideo vertical sync spot flash settings
   int RTFCNDCL GetSyncFlashSpotSize() { return(m_syncFlashSize); }
   int RTFCNDCL GetSyncFlashDuration() { return(m_syncFlashDur); }
   BOOL RTFCNDCL SetSyncFlashParams(int sz, int dur);

   BOOL RTFCNDCL GetBkgColor(int& r, int& g, int& b);    // get/set RMVideo background color (each cmpt in [0..255])
   BOOL RTFCNDCL SetBkgColor(int r, int g, int b);

   BOOL RTFCNDCL Init();                                 // ensure RMVideo is in idle state with no targets loaded

   BOOL RTFCNDCL AddTarget( RMVTGTDEF tgt );             // add defn of a target to be loaded into RMVideo
   BOOL RTFCNDCL LoadTargets();                          // load all defined targets into RMVideo (max wait = 1sec)
   int RTFCNDCL GetNumTargets();                         // get number of targets currently defined
   
   // begin a target animation sequence
   BOOL RTFCNDCL StartAnimation(RMVTGTVEC* pVecsFrame0, RMVTGTVEC* pVecsFrame1, BOOL bSync);
   // deliver target motion update vectors for next frame of animation sequence
   BOOL RTFCNDCL UpdateAnimation(RMVTGTVEC* pVecs, BOOL bSync, int& framesElapsed);
   BOOL RTFCNDCL StopAnimation();                        // stop target animation sequence
   
   // get total number of duplicate frames that have occurred the ongoing or just ended animation sequence
   int RTFCNDCL GetNumDuplicateFrames() { return(IsDown() ? 0 : m_nDupFrames); }
   
   // retrieve information on duplicate frame events that occurred during the last animation sequence
   int RTFCNDCL GetNumDuplicateFrameEvents();
   BOOL RTFCNDCL GetDuplicateFrameEventInfo(int idx, int& frame, int& count);

   BOOL RTFCNDCL GetMediaFolders(int& n, char* pBuf);    // RMVideo "media store" access...
   BOOL RTFCNDCL GetMediaFiles(LPCTSTR strFolder, int&n, char* pBuf);
   BOOL RTFCNDCL GetMediaInfo(LPCTSTR strFolder, LPCTSTR strFile, int& w, int& h, float& rate, float& dur);
   BOOL RTFCNDCL DeleteMediaFile(LPCTSTR strFolder, LPCTSTR strFile);
   BOOL RTFCNDCL DownloadMediaFile(LPCTSTR srcPath, LPCTSTR strFolder, LPCTSTR strFile);
   
private:
   CCxRMVideo( const CCxRMVideo& src );                  // no copy constructor or assignment operator defined
   CCxRMVideo& operator=( const CCxRMVideo& src );

   static LPCTSTR EMSG_SENDERROR;                        // (errmsg) send command failed due to socket error
   static LPCTSTR EMSG_RECVERROR;                        // (errmsg) socket error while receiving reply
   static LPCTSTR EMSG_LOSTCONN;                         // (errmsg) connection lost while receiving reply
   static LPCTSTR EMSG_TIMEOUT;                          // (errmsg) timed out waiting for reply
   static LPCTSTR EMSG_CMDERROR;                         // (errmsg) RMVideo could not handle a command
   static LPCTSTR EMSG_INVALIDREPLY;                     // (errmsg) reply packet badly formed 
   static LPCTSTR EMSG_BADREPLY;                         // (errmsg) unexpected reply to a given command
   static LPCTSTR EMSG_SENDDELAY;                        // (errmsg) network send delay (buffer full?)
   static LPCTSTR EMSG_NOTWHILEANIM;                     // (errmsg) func not available during animation sequence
   static LPCTSTR EMSG_TGTLISTFULL;                      // (errmsg) animated target list is full
   static LPCTSTR EMSG_TGTLISTEMPTY;                     // (errmsg) animated target list is empty
   static LPCTSTR EMSG_UNRECOGTGT;                       // (errmsg) defined target type not recognized
   static LPCTSTR EMSG_ANIMSTARTFAIL;                    // (errmsg) unable to start animation sequence

   static const CDevice::DevInfo BLANKDEV;               // this device has no device info

   static const int MIN_DISTTOEYE;                       // minimum distance to eye (mm)
   static const int MIN_DIMENSION;                       // minimum visible display width or height (mm)
   static const int DEF_DISTTOEYE_MM;                    // default distance to eye (mm)
   static const int DEF_WIDTH_MM;                        // default visible display width and height (mm)
   static const int DEF_HEIGHT_MM;

   struct VideoMode                                      // information on an available video mode:
   {                                                     // 
      int w;                                             //    screen width in pixels
      int h;                                             //    screen height in pixels
      int rate;                                          //    nominal refresh rate in Hz
   };

   int m_nModes;                                         // number of alternative video modes available
   VideoMode m_videoModes[RMV_MAXVMODES];                // info on the available video modes
   int m_iCurrMode;                                      // the current video mode (zero-based index; -1 invalid)
   int m_gamma[3];                                       // RMVideo monitor's gamma correction factors, scaled by 1000

   // vsync spot flash settings last sent to RMVideo: spot size in mm, flash duration in # video frames
   int m_syncFlashSize;
   int m_syncFlashDur;

   // measured frame period of RMVideo monitor, in seconds with nano-second precision
   double m_dFramePeriod;

   int m_iDistToEye;                                     // distance from eye to center of RMVideo display (mm)
   int m_iWidth;                                         // width of visible display (mm)
   int m_iHeight;                                        // height of visible display (mm)
   int m_bkgRGB;                                         // current display bkg RGB color in packed format:  0x00BBGGRR

   double m_dDegToPix;                                   // converts deg --> pixels. Assumed to be the same for H,V!!!

   VOID RTFCNDCL RecalcDegToPix();                       // recalc deg->pix conv factor based on current disp geom

   static const int STATE_IDLE;                          // RMVideo not animating; no targets loaded
   static const int STATE_TGTSLOADED;                    // idle but targets loaded, ready to start animation sequence
   static const int STATE_ANIMATING;                     // target animation sequence in progress

   int m_iState;                                         // current state of CCxRMVideo -- see STATE_* constants
   int m_nTargets;                                       // number of targets currently defined
   RMVTGTDEF m_TargDefs[RMV_MAXTARGETS];                 // target definition buffer

   BOOL m_bDisabled;                                     // mark device as permanently disabled by prior error

   // duplicate frame events: Store up to 100 events. For each event, store [N,M], where N is the frame index at which
   // duplicate frame(s) started. When M=0, one duplicate frame occurred because a target update from CCxRMVideo was
   // late. Otherwise, M>0 is the number of consecutive duplicate frames that occurred due to a rendering delay on the
   // RMVideo side.
   static const int DUPBUFSZ = 100;
   int m_nDupEvents;
   int m_dupEvent[DUPBUFSZ][2];
   int m_nDupFrames;                                     // total # of duplicate frames since animation started

   char m_errMsg[100];                                   // for preparing formatted device error message

   SOCKET m_rmvSocket;                                   // our TCP/IP socket connection to RMVideo
   int m_commandBuf[RMV_MAXCMDSIZE];                     // commands to RMVideo are packaged in this buffer
   int m_iCmdBytesSent;                                  // #bytes of a command sent thus far
   int m_replyBuf[RMV_MAXCMDSIZE];                       // a reply from RMVideo is assembled in this buffer
   int m_iReplyBytesRcvd;                                // #bytes of a reply packet rcvd thus far

   BOOL RTFCNDCL MapDeviceResources() { return(TRUE); }  // these do nothing b/c we don't really talk to the NIC!
   VOID RTFCNDCL UnmapDeviceResources() {}               //

public:
   BOOL RTFCNDCL OpenEx(CCxMasterIO* pIO);               // establish connection to RMVideo

private:
   // TRUE if RMVideo is either temporarily or permanently unavailable
   BOOL RTFCNDCL IsDown() { return(!IsOn() || m_bDisabled); }

   BOOL RTFCNDCL OnOpen();                               // open socket connection and start RMVideo command session
   VOID RTFCNDCL OnClose();                              // end RMVideo command session and close socket connection

   BOOL RTFCNDCL putFile(                                // helper method that handles file transfer sequencer for 
      LPCTSTR srcPath, LPCTSTR mvDir, LPCTSTR mvFile);   // downloading a movie file or the RMVideo executable file

   BOOL RTFCNDCL sendRMVCommand();                       // send command (already prepared) to RMVideo
   BOOL RTFCNDCL receiveRMVReply(int timeOut,            // receive reply from RMVideo
   		BOOL& bGotReply);
   BOOL RTFCNDCL receiveRMVReply(int timeOut)            // wait a finite time for a reply from RMVideo
   {
      if(timeOut <= 0) timeOut = 10;
      BOOL bReplied = FALSE;
      return(receiveRMVReply(timeOut, bReplied));
   }

   // set device error and mark interface as permanently disabled
   VOID RTFCNDCL disableOnError(LPCTSTR strErr); 
};

#endif   // !defined(CXRMVIDEO_H__INCLUDED_)

