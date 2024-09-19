//=====================================================================================================================
//
// vidbuffer.h : Helper class CVidBuffer manages video streaming on a background thread.
//
//=====================================================================================================================


#if !defined(VIDBUFFER_H_INCLUDED_)
#define VIDBUFFER_H_INCLUDED_

// need this so that definition of AV_NOPTS_VALUE in avcodec.h will compile here
#ifndef __STDC_CONSTANT_MACROS
#   define __STDC_CONSTANT_MACROS 1
#endif
#include <stdint.h>

// these libraries are pure C, so we need to tell the compiler that!
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/file.h>
}


class CVidBuffer
{
public:
   // open video file and get basic info about the first video stream it contains
   static bool getVideoInfo(const char* path, int& w, int& h, int& r, int& d, bool quiet);

   CVidBuffer();
   ~CVidBuffer();

   // on first call, start the worker thread that performs buffering. No videos are loaded and worker thread is in wait
   // wait state. Subsequent calls are equivalent to calling reset().
   bool initialize();

   // stop any buffering in progress, ensure any open video files are closed, and release all resources allocated
   // with the exception of the buffering thread
   void reset();

private:
   // stop buffering, ensure any open video files are closed, and terminate the buffering thread. Note that this method
   // will be invoked in the destructor.
   void terminate();

public:
   // is video buffering worker thread still alive?
   bool isRunning() { return(m_bAlive); }

   // open video file specified and prepare to stream video content. Buffering thread must be in wait state.
   int openVideoStream(const char* path, bool preload, bool stopOnEOF);

   // close all open video streams. Worker thread will be idled if it is not already.
   void closeAllVideoStreams();

   // some video information determined when source file is first opened. Safe to access until video closed.
   int getVideoWidth(int videoID);
   int getVideoHeight(int videoID);
   int getVideoFrameSize(int videoID);
   double getVideoPlaybackRate(int videoID);

   // direct access to the RGB pixel data buffer containing the current video frame. Safe to call while buffering.
   uint8_t* getCurrentFrameData(int videoID);

   // advance to next video frame. Safe to call while buffering.
   void advanceToNextFrame(int videoID);

   // returns true if an error has occurred while buffering stream; the stream is disabled.
   bool isVideoDisabled(int videoID);

   // returns true once EOF has been reached on specified stream
   bool gotEOF(int videoID);

   // enable buffering of all open video streams via the worker thread.
   bool startBuffering();
   // disable buffering of all open video streams; worker thread reenters wait state.
   void stopBuffering();

private:
   volatile bool m_bOn;         // master thread sets this before starting worker; resets it to tell worker to die
   volatile bool m_bAlive;      // master monitors this flag to verify that worker thread is running
   volatile bool m_bBufferEna;  // master sets/clears this flag to tell worker thread to start/stop buffering
   volatile bool m_bBuffering;  // worker thread sets this flag while buffering, clears it while in wait state

   static const int MAXPATHSZ = 256;
   static const int MAXSTREAMS = 5;
   static const int QSIZE = 10;
   static const int IOCTXBUFSZ = 32768;
   static const int READCHUNK = 2097152;
   static const int MAXPRELOADSZ = 31457280;

   // structure representing video source file stored in contiguous RAM for fast streaming w/o disk access
   struct MemFile
   {
      uint8_t* buf;
      int64_t bufSz;
      int64_t curr_pos;
   };

   // everything we need to stream and buffer video frames from a source file
   struct VideoStream
   {
      char path[MAXPATHSZ];            // path to video source file
      bool stopOnEOF;                  // if true, stop streaming once source file EOF is reached

      MemFile memFile;                 // (optional) memory buffer holding holding entire contents of video source file
      AVIOContext* pIOCtx;             // (optional) custom IO context that uses the in-memory file

      AVFormatContext* pFmtCtx;        // video source file format I/O context
      int streamIdx;                   // index of source video stream
      AVCodecContext* pCodecCtx;       // codec context, etc for source video stream
      struct SwsContext* pSwsCtx;      // software scaler context
      AVFrame* pDstFrame;              // structure used to read content from source into data buffer

      uint8_t* frameQueue[QSIZE];      // circular queue of buffered video frames (pixel data in RGBA format)
      volatile int iRead;              // index of current frame being read from buffered stream (read-only to worker)
      volatile int iWrite;             // index of frame being written by worker (read-only to master)
      int nBytes;                      // size of each data buffer in the queue

      bool disabledOnError;            // if true, buffering on this stream has been disabled by a previoius error
      bool gotEOF;                     // flag set when EOF is reached on video stream

      int width;                       // width of video frames, in pixels
      int height;                      // height of video frames, in pixels
      double rate;                     // video playback rate in Hz; 0 if playback rate not available in source file
   };

   // the currently buffered video streams
   int m_nStreams;
   VideoStream m_streams[MAXSTREAMS];


   // the worker thread function and its static entry point, which we need for pthread_create()
   void run();
   static void* runEntryPoint(void* thisPtr) { ((CVidBuffer*)thisPtr)->run(); }

   // helper method performs the work of reading in the next video frame for the specified video stream
   void readNextVideoFrame(VideoStream* pStream);

   // helper method closes an open video stream. Should not be called while buffering in progress.
   void closeVideoStream(VideoStream* pStream);

   // helper method loads the entire video file into a large memory buffer to avoid disk accesses during streaming
   static bool loadFileIntoBuf(const char* path, uint8_t** ppBuf, size_t* pBufSz);

   // custom AVIO context callback used to read a packet from a file preloaded into RAM
   static int readPacketCB(void* opaque, uint8_t* buf, int bufSz);

   // custom AVIO context callback used to seek within a file stored in RAM
   static int64_t seekCB(void* opaque, int64_t offset, int whence);
};


#endif // !defined(VIDBUFFER_H_INCLUDED_)
