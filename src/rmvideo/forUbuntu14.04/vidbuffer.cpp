/*=====================================================================================================================
 vidbuffer.cpp : Singleton class CVidBuffer manages streaming of RMVideo movies on a background thread.

 AUTHOR:  saruffner.

 BACKGROUND:
 Testing in 2019 (on a 4-core 3.4GHz machine with 8GB RAM and a 7200rpm SATA drive) demonstrated that RMVideo, as it 
 was currently designed, could not "keep up" when presenting a trial that involves a movie target if the movie size 
 is 1024x768 or larger. At a playback rate of 50Hz, it took 149 attempts to successfully complete 50 trials. The 
 other 99 attempts aborted on duplicate frame errors.

 Essentially, it was taking too long to "read in the next frame". 

 In RMVideo V10b (Sep 2019), there is a single thread of execution. During an animation sequence, the method 
 CRMVTarget::updateMotion() called on that thread and is responsible for reading in the next movie frame from file 
 and uploading it to a dedicated GL texture. If it takes too long to read in one frame, then a duplicate frame error
 results. The larger the size of a frame, the more likely and more frequently this will happen.

 RMVideo's main thread of execution will stall in CRMVRenderer::animate() while waiting for the next vertical sync. So,
 say we preloaded 10 frames' worth of the video prior to starting the animation sequence, then used a background thread
 to stream frames into the buffer as needed, could this worker thread get enough CPU time to keep pace with the 
 animation timeline?

 To answer this question, I introduced a worker thread that opened a 1024x768 video file and, when a flag is set, 
 continuously reads in one frame after another until the flag is cleared. While "streaming" the video, it would 
 measure how long it took to read in every 10 frames and keep track of some statistics: total # of 10-frame blocks 
 read, min/max/avg time to read a 10-frame block. Over a set of 50 5-sec trials, the results were encouraging. For
 most trials, the average time to read in 10 frames was 20ms, or only 2ms per frame. However, there were some trials 
 when it appeared that disk IO slowed down. For example, after one of the 50 trials, the reported stats were N=65
 blocks, min/max/avg= 41/88/76 ms. That's 7.6ms per frame read on average over an extended period of 5 seconds. That
 could be problematic if playing the frames at 120Hz (8.33ms per frame). Nevertheless, I decided the results were 
 encouraging enough to pursue a complete implementation of CVidBuffer.

 DESCRIPTION:
 CVidBuffer is a singleton helper class that manages streaming of up to 5 different videos at once. It is NOT a
 general purpose class; rather, it is designed specifically to work with the RMVideo renderer and target classes,
 CRMVRenderer and CRMVTarget.

 CVidBuffer allocates infrastructure for each open video stream: the various LIBAV stuff needed to read in, decode,
 and convert each frame to RGB24 format, a queue of 10 pixel data buffers, and other state information. The design
 ASSUMES that there are only two threads of execution that may touch this infrastructure: RMVideo's main thread of
 execution thread (the "master" thread), and the "worker" thread launched by CVidBuffer that handles the task of
 reading frames from each open stream in the background. If this assumption is satisfied, the implementation 
 ensures that the two threads access the streaming infrastructure safely.

 Ideally, the RMVideo workstation should have a multi-core processor. When the worker thread is launched in 
 initialize(), the processor affinities of both the calling thread (which is assumed to be the "master"!) and the
 worker thread are modified so that the master can run on all but the last processor, while the worker thread runs
 only on that last processor. The worker thread runs under the normal SCHED_OTHER scheduling policy, while the master
 thread uses SCHED_FIFO. [NOTE: Initially, I configured the worker thread with SCHED_FIFO policy, but performance was
 poor; switching to SCHED_OTHER dramatically improved overall performance!] If the workstation only has a single 
 processor, then the worker thread necessarily must run on that processor. This means it will run only when the 
 master thread yields the CPU. During an animation sequence, this only happens while waiting on the vertical sync. 
 This is not great, as there will be lots of context switches occurring, and it is unlikely that CVidBuffer will be 
 able to handle streaming large movies on a single processor system.

 USAGE:
 Call initialize() during RMVideo startup. The worker thread is launched and starts waiting until video
 buffering is enabled. During the target loading phase before an animation sequence, each RMV_MOVIE target must 
 call openVideoStream() to open the video source file and buffer the first 10 video frames. This method returns a
 video ID which must be supplied to access the video stream. Prior to starting the animation, call startBuffering()
 to wake up the worker thread, which will begin streaming video frames as space comes available in the buffer queue.
 During animation, each movie target object calls getCurrentFrameData() to retrieve the next video frame; NULL will
 be returned if the stream is disabled on a prior error or if the stream's buffer queue is empty (because the 
 worker thread could not keep up with the animation timeline). The target should keep requesting the next frame 
 until it gets it; of course, this will likely lead to a duplicate frame error, as it should. Once done with the
 frame data, be sure to call advanceToNextFrame() to release that data and make room in the stream's buffer queue
 for another frame. Finally, once the animation ends, call stopBuffering() to idle the worker thread, then 
 closeAllVideoStreams() to close all open streams and release any associated resources.

 **************** SUMMARY OF PERFORMANCE TESTING, OCT 2019 ************************************************************
 10/9/19: Testing has suggested that streaming frames from the video file during playback is limiting the bandwidth 
 available on the PCIe bus for uploading each frame to the texture when animating a movie target. When I modified
 CRMVTarget so that it created each frame in memory rather than retrieving it from an open video stream via CVidBuffer,
 then RMVideo had no trouble playing back such a "fake" video at 1920x1080@120Hz.

 In an effort to eliminate the PCIEe load due to disk reading by CVidBuffer, tried to use av_file_map() to memory map
 the video source file. This will allocate a buffer large enough for the entire file and read the file into it. We 
 then have to use a custom AVIOContext to use this memory-mapped file instead of the file itself.

 10/10/19: av_file_map() will use mmap() if it's available. That will not load the entire file at once into RAM, which
 means that disk accesses will happen, still. When I tested, it did NOT improve performance when playing a 1920x1080 
 video @120Hz (file size 18MB). So then I tried to load file directly into buffer... 

 10/14/19: Preloading file into RAM before starting animation did not help either. Finally, I changed the initialize()
 method so that the worker thread runs with the normal SCHED_OTHER policy with a priority level of 0 (on my system, 
 you couldn't set a static priority level other than this). And that led to dramatic improvement -- so that 
 1920x1080@120Hz had no duplicate frames over 95% of trial reps! But the 1280x720 movie still aborted frequently. 
 Discovered that it contained an audio stream, whereas the 1920x1080 video file only had the single video stream. When
 I tested with a different 1280x720 movie that had just the single video stream, over 98% of trials completed 
 successfully. Note I also tested whether restricting the CVidBuffer thread to one core made a difference, and it did.
 When I let it run on all cores with SCHED_OTHER policy, the 1280x720 movie (containing the audio stream) aborted every
 time over a sequence of 30 reps. 

 10/15/19: Given that it was changing the scheduling policy that dramatically improved performance, and that preloading
 video file into RAM did not, I restored the normal way of reading the file to see if there's any significant 
 performance loss now that the scheduling policy for the CVidBuffer thread is SCHED_OTHER. While trying to verify that
 preloading the video file into RAM made a difference or not, I ran into another problem. Each time I switch video 
 modes, CVidBuffer::initialize() is called. This would terminate the worker thread and launch a new one. But doing so 
 hampered performance, at least for the 1920x1080@120Hz test. For example, I launched RMVideo and Maestro, toggled from
 120Hz to 144Hz and back to 120Hz without running any tests. then I tried to run the 1920x1080@120Hz test and every rep
 aborted, always about 325ms after the movie came on. I then modified initialize() so that the worker thread is only 
 launched on the first invocation. After making that change, toggling the video mode did not impact the 1920x1080@120Hz
 test!! Really don't understand why, though.

 Decided to preserve the code that preloads the video file into RAM. Added 'preload" flag to openVideoStream() method,
 so that caller can decide whether or not to preload the video into RAM. Also, preloading will be disabled if the 
 source file exceeds 30MB.

 10/18/19: One more attempt to improve performance: For larger videos, the time to memcpy() the next frame to the 
 mapped buffer associated with a pixel buffer object (in CRMVTarget::updateMovie) can approach 1ms or more. Latest 
 tests showed that a trial involving two 1024x768 movies played simultaneously aborted far more often than a single 
 1920x1080 movie, even though the per-frame memory buffer is 1.5MB larger than the sum of the two buffers for the 
 former! 

 I wanted to see if it would help to perform the memcpy() in the CVidBuffer thread rather than in the main render
 thread. The idea is that the GL commands for the upcoming display frame would be issued sooner in the refresh cycle,
 so there would be a better chance of not getting a duplicate frame error. This required making changes both in 
 CVidBuffer and CRMVTarget...

 10/21/19: Haven't had any luck so far. Initially, I got segmentation faults. I didn't trace the cause, but I thought
 it was likely that I was trying to memcpy() to a NULL buffer in the worker thread -- perhaps due to thread sync issue.
 I tried using a mutex to sync access to the RGB pixel buffer passed to the CVidBuffer thread for filling. I no longer
 saw a seg fault, but RMVideo appeared to deadlock on the first call to updateMovie() for the first target of a trial
 displaying two movies simultaneously. A single-movie trial involving a 320x240 or 1280x720 movie ran to completion, 
 but a single-movie trial with a 1920x1080 movie consistently aborted on a dupe frame error 2.8 sec into the trial. I 
 then tried using a separate mutex for each open video stream, and I got a seg fault again. This may be due to a coding
 error, but I'm giving up on using a mutex.

 I finally got it working, having figured out why it would deadlock when running more than one movie. However, the
 performance was inferior. The movie_1920x1080 trial aborted consistently at 2859ms for reasons unknown. Excluding that
 trial from the "TestingRMVMovies" set, I ran 20 blocks and got 28 aborts, 17 of which were for the trial
 "two_1024x768Movies_Simultaneous". This compares to 20 aborts when the memcpy() is in CRMVTarget::updateMovie(). I've
 given up on this idea.

 10/28/19: Trying again! Testing showed that RMVideo can handle the TestingRMVMovies trial set without any dupe frame
 errors so long as these two trials are omitted: two_1024x768Movies_Simultaneous and movie_1920x1080. If either trial
 is included in the set, you get roughly 20 aborts over 50 blocks of the trials in Rand NOFIX mode, and the aborts are
 dispersed among all the different trials. Why does inclusion of either of those two trials cause the other trials to
 abort? One possibility is that CVidBuffer has to malloc ~62MB for the frame queue when streaming the 1920x1080 movie,
 and 47MB for the two 1024x768 movies. This memory gets freed at the end of each trial. Perhaps all this memory 
 thrashing is causing brief hiccups in the system that lead to the occasional dupe frame errors? 

 To assess this, I allocated a large pool of memory when CVidBuffer is initialized, and that pool was used for the
 frame queues during streaming. The pool's capacity was fixed, large enough to handle a single 2560x1440 movie, or
 several movies at lower resolution. THERE WAS NO MEASURABLE IMPROVEMENT IN PERFORMANCE.
 **************** SUMMARY OF PERFORMANCE TESTING, OCT 2019 ************************************************************

 REVISION HISTORY:
 12sep2019-- Started work on a simple implementation to assess feasibility...
 16sep2019-- Starting work on a full implementation...
 18sep2019-- Finished implementation. Moved static function getVideoInfo() from CRMVTarget to this class.
 23sep2019-- Test/debug. Added gotEOF() to notify callers that EOF has been reached at least once. The streamer will
 "wrap around" and start over when hitting EOF, unless configured to stop on EOF in openVideoStream().
 01oct2019-- Extensive performance testing/tweaking. See summary above.
 24oct2019-- Done with performance testing/tweaking. The one change that led to dramatic performance improvement was
 to set the scheduling policy of the CVidBuffer thread to SCHED_OTHER instead of SCHED_FIFO.
 28oct2019-- Tried a fixed-capacity memory pool to supply the memory for each stream's frame queue. DID NOT improve
 performance.
 04nov2019-- Wrapped up performance tweaking and testing. Releasing changes as RMVideo 10c.
//===================================================================================================================*/

#include <stdio.h>
#include "sched.h"
#include "time.h"
#include "pthread.h"
#include "utilities.h"

#include "vidbuffer.h"


/**
 * Open the specified video file and retrieve information about the first video stream therein. This method is used
 * to verify that a file in the RMVideo media store can be read and processed as a video. If any problems are
 * encountered, the method optionally prints a brief error description to stderr.
 * @param path Pathname of the file to open.
 * @param w, h, r, d [out] Information returned about the video: frame width and height in pixels, frame rate in
 * milli-Hz, and the approximate movie duration (at specified frame rate) in milliseconds. Any unknown values will
 * be set to 0.
 * @param quiet [in] If the method fails, this flag determines whether or not the method prints a brief error
 * description to stderr. If true, nothing is printed.
 * @return True if the file was successfully opened and the video info retrieved; false otherwise.
 */
bool CVidBuffer::getVideoInfo(const char* path, int& w, int& h, int& r, int& d, bool quiet)
{
   // make sure we've registered all available formats and codecs (after first invocation, method has no effect)
   av_register_all();

   // open the source video file
   AVFormatContext* pFormatCtx = NULL;
   int errcode = avformat_open_input(&pFormatCtx, path, NULL, NULL);
   if(errcode != 0)
   {
      if(!quiet) ::fprintf(stderr, "ERROR(CVidBuffer::getVideoInfo) Cannot open source file, errno=%d\n", errcode);
      return(false);
   }

   // retrieve stream information from the source file
   errcode = avformat_find_stream_info(pFormatCtx, NULL);
   if(errcode < 0)
   {
      if(!quiet)
         ::fprintf(stderr,
                 "ERROR(CVidBuffer::getVideoInfo) Cannot retrieve stream information from source, errno=%d\n", 
                 errcode);
      avformat_close_input(&pFormatCtx);
      return(false);
   }

   // find the first video stream in source
   int iStream = -1;
   for(int i=0; i < pFormatCtx->nb_streams; i++) if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
   {
      iStream = i;
      break;
   }
   if(iStream == -1)
   {
      if(!quiet) ::fprintf(stderr, "ERROR(CVidBuffer::getVideoInfo) Found no video streams in source file!\n");
      avformat_close_input(&pFormatCtx);
      return(false);
   }
   AVStream* pStream = pFormatCtx->streams[iStream];
   
   // open the codec that we'll need to decode the video stream
   AVCodecContext* pCodecCtx = pStream->codec;
   AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
   if(pCodec == NULL)
   {
      if(!quiet)
         ::fprintf(stderr, "ERROR(CVidBuffer::getVideoInfo) Video source uses unsupported codec: id=%d\n", 
              pCodecCtx->codec_id);
      avformat_close_input(&pFormatCtx);
      return(false);
   }

   // for come codecs, need to open codec to get frame width and height...
   errcode = avcodec_open2(pStream->codec, pCodec, NULL);
   if(errcode < 0)
   {
      if(!quiet) ::fprintf(stderr, "ERROR(CVidBuffer::getVideoInfo): Failed to open codec, errno=%d\n", errcode);
      avformat_close_input(&pFormatCtx);
      return(false);
   }

   // retrieve info about the video stream: frame width and height, frame rate in milliHz, dur in ms. A value of zero
   // indicates the information was not available (or too large to represent as a 32-bit int!)
   w = pCodecCtx->width;
   h = pCodecCtx->height;
   
   double rateHz = 0;
   if(pStream->r_frame_rate.den && pStream->r_frame_rate.num) rateHz = av_q2d(pStream->r_frame_rate);
   else if(pStream->time_base.den && pStream->time_base.num) rateHz = 1.0/av_q2d(pStream->time_base);
   else if(pCodecCtx->time_base.den && pCodecCtx->time_base.num) rateHz = 1.0/av_q2d(pCodecCtx->time_base);
   r = (rateHz == 0) ? 0 : ((int) (0.5 + 1000.0 * rateHz));
   d = 0;
   if(pFormatCtx->duration != AV_NOPTS_VALUE)
   {
      double durMS = 1000.0 * ((double)pFormatCtx->duration) / ((double)AV_TIME_BASE);
      d = (durMS <= 2147483647) ? ((int) durMS) : 0;
   }
   
   avcodec_close(pCodecCtx);
   avformat_close_input(&pFormatCtx);
   return(true);
}


CVidBuffer::CVidBuffer()
{
   m_bOn = m_bAlive = m_bBufferEna = m_bBuffering = false;

   m_nStreams = 0;
   for(int i=0; i<CVidBuffer::MAXSTREAMS; i++)
   {
      VideoStream* pStream = &(m_streams[i]);

      ::memset(pStream->path, 0, sizeof(char)*CVidBuffer::MAXPATHSZ);
      pStream->stopOnEOF = false;

      pStream->pFmtCtx = NULL;
      pStream->streamIdx = -1;
      pStream->pCodecCtx = NULL;
      pStream->pSwsCtx = NULL;
      pStream->pDstFrame = NULL;
      for(int j=0; j<CVidBuffer::QSIZE; j++) pStream->frameQueue[j] = NULL;
      pStream->iRead = 0;
      pStream->iWrite = 0;
      pStream->nBytes = 0;
      pStream->disabledOnError = true;
      pStream->gotEOF = false;
      pStream->width = 0;
      pStream->height = 0;
      pStream->rate = 0;
   }
}

CVidBuffer::~CVidBuffer()
{
   terminate();
   m_bOn = m_bAlive = m_bBufferEna = m_bBuffering = false;
}

/**
 Initialize the video streamer object.

 On the first invocation of this method, the background buffering thread is launched IAW the following sequence of 
 steps (based on considerable testing):
   1) Check the processor affinity mask for the calling thread -- the "master". If it can run on multiple processors, 
 then remove the last numbered proc from its affinity mask.
   3) Configure select parameters for the worker thread. Its scheduling policy is set to the normal SCHED_OTHER, with 
 the minimum static priority level for that policy. In a multi-CPU scenario, it will be restricted to run on the last
 numbered processor.
   4) Launch the worker thread. It will simply enter an idle state until there's at least one open stream and buffering
 is enabled.

 Later invocations of the method will simply call reset() to ensure buffering is disabled and any open video streams
 are closed. Testing found that repeatedly terminating and relaunching the buffering thread led to performance
 degradation (not sure why!).

 @return True if successful, false otherwise. In the latter case, a brief error message with an internal error code
 is printed to stderr.
*/
bool CVidBuffer::initialize()
{
   // if the buffering thread is running, ensure buffering is disabled and all open video streams are closed
   reset();

   // if buffering thread is alive, there's nothing more to do.
   if(m_bAlive) return(true);

   cpu_set_t cpu;
   CPU_ZERO(&cpu);
   int errCode = 0;

   // get master thread's processor affinity. If it can run on multiple processors, then remove the last numbered 
   // processor from its affinity mask
   pthread_t masterThrd = pthread_self();
   int last = -1;
   bool isSingleCPU = false;
   bool ok = (0 == ::pthread_getaffinity_np(masterThrd, sizeof(cpu_set_t), &cpu));
   if(ok)
   {
      int count = 0;
      for(int i=0; i<CPU_SETSIZE; i++) if(CPU_ISSET(i, &cpu)) { ++count; last = i; }

      ok = (last > -1);
      isSingleCPU = (count == 1);
      if(ok && !isSingleCPU)
      {
         CPU_CLR(last, &cpu);
         ok = (0 == ::pthread_setaffinity_np(masterThrd, sizeof(cpu_set_t), &cpu));
      }
   }
   if(!ok) errCode = 1;

   // set up thread attributes for the buffering thread: SCHED_OTHER scheduling policy with minimum static priority
   // level. In multi-core scenario, configured to run only on the last numbered processor as determined above. We
   // have to set these attributes explicitly, b/c otherwise it will inherit the attributes of the calling thread.
   pthread_attr_t attr;
   if(ok)
   {
      ok = (0 == ::pthread_attr_init(&attr));
      if(ok && !isSingleCPU)
      {
         CPU_ZERO(&cpu);
         CPU_SET(last, &cpu);
         ok = (0 == ::pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu));
      }
      if(ok)
      {
         int iVal = PTHREAD_EXPLICIT_SCHED;
         ok = (0 == ::pthread_attr_setinheritsched(&attr, iVal));
         if(ok)
         {
            iVal = SCHED_OTHER;
            struct sched_param sp;
            sp.sched_priority = ::sched_get_priority_min(iVal);
            ok = (0 == ::pthread_attr_setschedpolicy(&attr, iVal)) && (0 == ::pthread_attr_setschedparam(&attr, &sp));
         }
      }
   }
   if(!ok) errCode = 2;

   // start the worker thread. It should set the "alive" flag and open the video file. Wait up to 1 second for
   // this to happen.
   pthread_t workerThrd;
   if(ok)
   {
      m_bOn = true;
      ok = (0 == ::pthread_create(&workerThrd, &attr, CVidBuffer::runEntryPoint, this));
      if(!ok) 
         errCode = 3;
      else
      { 
         CElapsedTime eTime;
         volatile long count = 0;
         while(eTime.get() < 1.0 && !m_bAlive) ++count;

         if(!m_bAlive)
         {
            m_bOn = false;
            ok = false;
            errCode = 4;
         }
      }
   }

   ::pthread_attr_destroy(&attr);

   if(!ok) ::fprintf(stderr, "[CVidBuffer] Failed to start background thread for video streaming, err=%d\n", errCode);
   return(ok);
}

/**
 Reset the video streamer object. Buffering is disabled, and the background buffering thread enters a wait state. Any 
 open video streams are closed, releasing any memory allocated when the streams were opened.
*/
void CVidBuffer::reset()
{
   stopBuffering();
   closeAllVideoStreams();
}

/**
 Terminate the background buffering thread, close any open video files, and ensure any resources allocated by the video
 streamer object are released. Will wait up to 1 second for the worker thread to die.
*/
void CVidBuffer::terminate()
{
   reset();

   // tell worker to die and wait up to 1 second for that to happen
   m_bOn = false;
   CElapsedTime eTime;
   volatile long count = 0;
   while(m_bAlive && eTime.get() < 1.0) ++count;

   if(m_bAlive) ::fprintf(stderr, "[CVidBuffer.reset()] WARNING: Worker thread failed to terminate!\n");
}

/**
 Open the video file specified and prepare to stream video content. The first 10 frames of the video are buffered and
 will be immediately available when this method returns. 

 @param Full file system path to the video source file.
 @param preload If true, the entire video file will be read into memory. The idea here is to optimize performance by 
 (hopefully) avoiding any disk IO during streaming. Limitation: If the file size exceeds 30MB, this flag is ignored,
 and the file will NOT be preloaded into RAM.
 @param stopOnEOF. If true, the video stream is stopped once EOF is reached on the video source file. Otherwise, upon
 reaching EOF, the streamer will seek to the beginning of the file and resume streaming from there -- so that the video
 "loops" indefinitely.
 @return A non-negative ID assigned to the buffered video stream -- used to access video stream information (width, 
 height, playback rate) and to retrieve buffered frames in sequence until stream is closed. If operation fails, a 
 negative error code is returned: -1 = video stream object is not initialized; -2 = buffering is in progress; 
 -3 = too many open video streams; -4 = failed to open stream for any other reason (file not found, memory allocation
 failure, video format or codec not supported). On failure, a brief error message is printed to stderr.
*/
int CVidBuffer::openVideoStream(const char* path, bool preload, bool stopOnEOF)
{
   if(!(m_bOn && m_bAlive)) 
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Video streamer is not initialized.\n");
      return(-1);
   }
   if(m_bBuffering)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Cannot open a new video stream while buffering is in progress.\n");
      return(-2);
   }
   if(m_nStreams == CVidBuffer::MAXSTREAMS) 
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Reached capacity. Cannot open any more video streams.\n");
      return(-3);
   }

   VideoStream* pStream = &(m_streams[m_nStreams]);

   // make sure we've registered all available formats and codecs (after first invocation, method has no effect)
   av_register_all();

   // if requested, attempt to preload source file into RAM, which requires specifying a custom IO context in order to
   // read packets from the in-memory file. If preload fails, revert to normal streaming from disk.
   if(preload)
   {
      size_t bufSz = 0;
      preload = CVidBuffer::loadFileIntoBuf(path, &(pStream->memFile.buf), &bufSz);
      if(!preload)
         ::fprintf(stderr, "WARNING(CVidBuffer): Failed to load video %s into RAM! Will stream from disk\n", path);
      else
         pStream->memFile.bufSz = (int64_t) bufSz;

      // to use the in-memory source file, we need to specify a custom IO context and store it in the format context
      uint8_t* ioCtxBuf = NULL;
      if(preload)
      {
         pStream->pFmtCtx = avformat_alloc_context();
         preload = (pStream->pFmtCtx != NULL);
      }
      if(preload)
      {
         ioCtxBuf = (uint8_t*) av_malloc(IOCTXBUFSZ);
         preload = (ioCtxBuf != NULL);
         if(preload)
         {
            pStream->pIOCtx = avio_alloc_context(ioCtxBuf, IOCTXBUFSZ, 0, &(pStream->memFile), 
               &CVidBuffer::readPacketCB,
               NULL,   // read-only -- no write_packet callback
               &CVidBuffer::seekCB);
            preload = (pStream->pIOCtx != NULL);
         }
      }
      if(!preload)
      {
         ::fprintf(stderr, "WARNING(CVidBuffer): Failed to create custom IO context for in-memory file %s\n", path);
         ::fprintf(stderr, "  Will try to stream from disk instead.\n");
         if(ioCtxBuf != NULL) av_free(ioCtxBuf);
         closeVideoStream(pStream);
      }
      else
      {
         pStream->pFmtCtx->pb = pStream->pIOCtx;
         pStream->pFmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
      }
   }


   // open the source. If file was preloaded into memory, specify dummy filename. Otherwise the format context gets 
   // allocated here.
   int errcode = avformat_open_input(&(pStream->pFmtCtx), preload ? "" : path, NULL, NULL);
   if(errcode != 0)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Unable to open video in %s; errno=%d\n", path, errcode);
      closeVideoStream(pStream);
      return(-4);
   }
   ::strncpy(pStream->path, path, CVidBuffer::MAXPATHSZ-1);
   pStream->path[CVidBuffer::MAXPATHSZ-1] = '\0';

   // retrieve stream information from the source file
   errcode = avformat_find_stream_info(pStream->pFmtCtx, NULL);
   if(errcode < 0)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Unable to get video stream info from %s, errno=%d\n", path, errcode);
      closeVideoStream(pStream);
      return(-4);
   }

   // find the first video stream in source
   pStream->streamIdx = -1;
   for(int i=0; i < pStream->pFmtCtx->nb_streams; i++) 
      if(pStream->pFmtCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
   {
      pStream->streamIdx = i;
      break;
   }
   if(pStream->streamIdx == -1)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Found no video streams in %s!\n", path);
      closeVideoStream(pStream);
      return(-4);
   }

   // open the codec that we'll need to decode the video stream
   pStream->pCodecCtx = pStream->pFmtCtx->streams[pStream->streamIdx]->codec;
   AVCodec *pCodec = avcodec_find_decoder(pStream->pCodecCtx->codec_id);
   if(pCodec == NULL)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Video file %s uses unsupported codec:\n   %s\n", path,
               (pCodec->long_name != NULL ? pCodec->long_name : pCodec->name));
      closeVideoStream(pStream);
      return(-4);
   }

   errcode = avcodec_open2(pStream->pCodecCtx, pCodec, NULL);
   if(errcode < 0)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Failed to open codec in %s, errno=%d\n", path, errcode);
      closeVideoStream(pStream);
      return(-4);
   }

   // prepare software scaler context to handle src->dst pixel format conversion and scaling as needed
   int w = pStream->pCodecCtx->width;
   int h = pStream->pCodecCtx->height;
   pStream->pSwsCtx = sws_getContext(w, h, pStream->pCodecCtx->pix_fmt, w, h, PIX_FMT_RGB24, SWS_BICUBIC, 
         NULL, NULL, NULL);
   if(pStream->pSwsCtx == NULL) 
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Cannot initialize the software scaler context for %s!\n", path);
      closeVideoStream(pStream);
      return(-4);
   }

   // allocate the "destination" video frame that we reuse to read in each frame from file and convert it to the 
   // desired RGB24 format. Also allocate the QSIZE pixel data buffers that hold the buffered frames.. 
   pStream->pDstFrame = avcodec_alloc_frame();
   bool ok = (pStream->pDstFrame != NULL);
   pStream->nBytes = avpicture_get_size(PIX_FMT_RGB24, w, h);

   for(int i=0; ok && i<CVidBuffer::QSIZE; i++)
   {
      pStream->frameQueue[i] = (uint8_t*) av_malloc(pStream->nBytes*sizeof(uint8_t));
      ok = (pStream->frameQueue[i] != NULL);
   }
   if(!ok)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Failed to allocate streaming buffers for %s!\n", path);
      closeVideoStream(pStream);
      return(-4);
   }

   // the queue is initially empty, and the "read" and "write" indices both point to the first slot
   pStream->iRead = pStream->iWrite = 0;
   pStream->disabledOnError = false;
   pStream->gotEOF = false;
   pStream->stopOnEOF = stopOnEOF;

   // fill in video info; playback rate is 0 if we cannot find it in the file
   pStream->width = w;
   pStream->height = h;

   AVStream* pAVStream = pStream->pFmtCtx->streams[pStream->streamIdx];
   if(pAVStream->r_frame_rate.den && pAVStream->r_frame_rate.num)
      pStream->rate = av_q2d(pAVStream->r_frame_rate);
   else if(pAVStream->time_base.den && pAVStream->time_base.num) 
      pStream->rate = 1.0/av_q2d(pAVStream->time_base); 
   else if(pStream->pCodecCtx->time_base.den && pStream->pCodecCtx->time_base.num) 
      pStream->rate = 1.0/av_q2d(pStream->pCodecCtx->time_base);
   else
      pStream->rate = 0;

   // buffer the first QSIZE frames 
   while(ok && ((pStream->iWrite + 1) % CVidBuffer::QSIZE) != pStream->iRead)
   {
      readNextVideoFrame(pStream);
      ok = !pStream->disabledOnError;

      // if video does not loop and we've already reached EOF, then stop!
      if(ok && pStream->stopOnEOF && pStream->gotEOF) break;
   }
   if(!ok)
   {
      ::fprintf(stderr, "ERROR(CVidBuffer): Failed to buffer first 10 frames in %s!\n", path);
      closeVideoStream(pStream);
      return(-4);
   }

   // success! Return the ordinal ID of the initialized video stream
   int id = m_nStreams;
   ++m_nStreams;

   return(id);
}

/**
 Close all open video streams and release all associated resources. If buffering is in progress, it will be stopped 
 before closing the streams.
*/
void CVidBuffer::closeAllVideoStreams()
{
   stopBuffering();
   for(int i=0; i<m_nStreams; i++) closeVideoStream(&(m_streams[i]));
   m_nStreams = 0;
}

/**
 Return width of video frames for the specified open video stream.
 @param videoID ID of open video stream
 @return Frame width in pixels. Returns 0 if stream ID is invalid.
*/
int CVidBuffer::getVideoWidth(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].width : 0);
}

/**
 Return height of video frames for the specified open video stream.
 @param videoID ID of open video stream
 @return Frame height in pixels. Returns 0 if stream ID is invalid.
*/
int CVidBuffer::getVideoHeight(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].height : 0);
}

/**
 Return the size of the data buffer for a single frame drawn from the specified open video stream.
 @param videoID ID of open video stream

 @return Frame size in bytes. Returns 0 if stream ID is invalid.
*/
int CVidBuffer::getVideoFrameSize(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].nBytes : 0);
}

/**
 Return playback rate for the specified open video stream.
 @param videoID ID of open video stream
 @return Playback rate in Hz. Returns 0 if stream ID is invalid, OR if no playback rate is available in source file.
*/
double CVidBuffer::getVideoPlaybackRate(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].rate : 0);
}

/**
 Get a reference to the buffer containing the pixel data for the current video frame, ie, the oldest buffered frame in
 the specified open video stream. The pixel data is stored in the RGB24 format, in the form required for uploading to 
 a OGL texture.

 This method is safe to call while the video buffering is in progress. Do NOT change buffer contents nor delete the 
 buffer. After copying or otherwise using the buffer, be sure to call advanceToNextFrame() to update the video stream 
 state -- otherwise the same buffer will be returned on a subsequent call.
 
 @param videoID ID of open video stream
 @return Pointer to buffer. Returns NULL if stream ID is invalid, if no frame data is available, or if an error has
 occurred on the video stream.
*/
uint8_t* CVidBuffer::getCurrentFrameData(int videoID)
{
   uint8_t* out = NULL;
   if(videoID >= 0 && videoID < m_nStreams)
   {
      VideoStream* pStream = &(m_streams[videoID]);
      if((pStream->iRead != pStream->iWrite) && !pStream->disabledOnError)
         out = pStream->frameQueue[pStream->iRead];
   }
   return(out);
}

/**
 Advance the specified open video stream to the next oldest buffered frame.

 This method is safe to call while the video buffering is in progress. It must be invoked as soon as the caller is
 finished with the pixel data buffer retrieved in the last call to getCurrentFrameData(), else the same buffer is 
 returned by the next invocation of that function. If an error has occurred on the video stream, or there are no more
 filled buffers queued in the stream, then no action is taken.

 @param videoID ID of open video stream
*/
void CVidBuffer::advanceToNextFrame(int videoID)
{
   if(videoID >= 0 && videoID < m_nStreams)
   {
      VideoStream* pStream = &(m_streams[videoID]);
      if((pStream->iRead != pStream->iWrite) && !pStream->disabledOnError)
         pStream->iRead = (pStream->iRead + 1) % CVidBuffer::QSIZE;
   }
}

/**
 Returns true if the specified video stream has been disabled by a previous error. A brief error message is printed to 
 stderr when the error is detected, and no further buffering of the video stream will occur.

 @param videoID ID of open video stream
 @return True if specified stream is disabled, else false. Also returns true if the stream ID is invalid!
*/
bool CVidBuffer::isVideoDisabled(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].disabledOnError : true);
}

/**
 Returns true if the specified video stream has reached "end of file". The video streamer does not stop upon reaching 
 EOF; it simply seeks to the beginning of the video source file and continues streaming frames.

 @param videoID ID of open video stream
 @return True if specified stream has reached EOF at least once, else false. Returns false if the stream ID is invalid!
*/
bool CVidBuffer::gotEOF(int videoID)
{
   return((videoID >= 0 && videoID < m_nStreams) ? m_streams[videoID].gotEOF : false);
}


/**
 Enable buffering of all open video streams on a background worker thread.
 @return True if buffering enabled; false if background thread is not running or there are no open video streams.
*/
bool CVidBuffer::startBuffering()
{
   if(m_bBuffering) return(true);
   if(m_nStreams <= 0 || !m_bAlive) return(false);
   m_bBufferEna = true;
   return(true);
}

/**
 Disable buffering of all open video streams. Waits up to 100ms for worker thread to return to idle wait state.
*/
void CVidBuffer::stopBuffering()
{
   m_bBufferEna = false;
   CElapsedTime eTime;
   long count = 0;
   while(m_bBuffering && eTime.get() < 0.1) count++;
}


/**
 Background thread runtime function handles buffering of all open video streams defined in the video streamer object.
 
 When buffering is disabled, the worker thread simply waits until the next time it is enabled, checking the guard flag
 roughly once a millisecond. When buffering is enabled, the thread services each open stream in sequence, reading in a
 full frame and storing the pixel data in the stream's frame queue, then moving on to the next stream.

 If an error occurs while reading in or processing an individual frame, an internal error code is set on that stream
 and no further buffering will occur.

 See file header for a discussion of the design strategy to keep CVidBuffer pseudo thread-safe, along with the key
 assumptions in that design.
*/
void CVidBuffer::run()
{
   // this flag remains set while the worker thread is alive
   m_bAlive = true;

   while(m_bOn)
   {
      // wait until buffering is enabled, checking guard flag every 1ms
      struct timespec tSpec;
      tSpec.tv_sec = (time_t) 0;
      tSpec.tv_nsec = 1000000;
      while(m_bOn && !m_bBufferEna)
         ::nanosleep(&tSpec, NULL);

      // while buffering is enabled, continuously cycle through the streams, reading in and buffering one frame from
      // each (if there's room in the stream's buffer queue). Guard flags are checked after each completed frame read 
      // to ensure the worker thread responds quickly to any requests from the "master thread".
      m_bBuffering = true;
      while(m_bOn && m_bBufferEna)
      {
         for(int i=0; m_bOn && m_bBufferEna && i<m_nStreams; i++)
         {
            readNextVideoFrame( &(m_streams[i]) );
         }
      }
      m_bBuffering = false;
   }

   ::fprintf(stderr, "====> [CVidBuffer] Worker thread exiting.\n");
   m_bAlive = false;
}


/**
 Helper method performs the work of reading in the next video frame from the source file for the specified video
 stream, converting it to the PIX_FMT_RGB24 format, and storing it in the next available slot in the stream's buffer
 queue. It takes no action if the stream has been disabled by a previous error, if stream is configured to stop on EOF
 and EOF is reached, or if the stream's buffer queue is full. Otherwise, the method does not return until one complete
 frame has been read in; execution time will vary depending on the size of a video frame, the speed of the storage 
 medium, etc.

 @param Pointer to the video stream from which to read the next frame.
*/
void CVidBuffer::readNextVideoFrame(VideoStream* pStream)
{
   // abort on error condition, buffer queue full, or stop on EOF
   if(pStream == NULL || pStream->disabledOnError) return;
   if(((pStream->iWrite + 1) % CVidBuffer::QSIZE) == pStream->iRead) return;
   if(pStream->stopOnEOF && pStream->gotEOF) return;
 
   // clear the current write buffer and install it in the destination AVFrame
   ::memset((void*)pStream->frameQueue[pStream->iWrite], 0, pStream->nBytes);
   avpicture_fill((AVPicture *)pStream->pDstFrame, pStream->frameQueue[pStream->iWrite], 
         PIX_FMT_RGB24, pStream->width, pStream->height);

   // allocate a source video frame; its pixel data buffer is allocated as packets are decoded into it
   AVFrame* pSrcFrame = avcodec_alloc_frame();
   if(pSrcFrame == NULL)
   {
      pStream->disabledOnError = true;
      ::fprintf(stderr, "ERROR(CVidBuffer): Memory allocation error while streaming %s\n", pStream->path);
      return;
   }

   // read and decode packets until we've loaded the next movie frame...
   AVPacket packet;
   av_init_packet(&packet);
   int frameFinished = 0;
   bool gotFrame = false;
   while(!(pStream->disabledOnError || gotFrame))
   {
      int res = av_read_frame(pStream->pFmtCtx, &packet);
      if(res < 0)
      {
         // no more frames available: we're either at EOF or an error occurred.
         int err = pStream->pFmtCtx->pb->error;
         bool eof = (pStream->pFmtCtx->pb->eof_reached != 0);
         if(eof || (err == 0))
         {
            // we've reached normal EOF. Seek to beginning of stream and start over.
            avcodec_flush_buffers(pStream->pCodecCtx);
            if(av_seek_frame(pStream->pFmtCtx, pStream->streamIdx, (int64_t) 0, AVSEEK_FLAG_BACKWARD) < 0)
            {
               ::fprintf(stderr, "ERROR(CVidBuffer): Failed while rewinding video source %s\n", pStream->path);
               pStream->disabledOnError = true;
            }
            pStream->gotEOF = true;
            if(pStream->stopOnEOF) break;
         }
         else
         {
            // terminate playback on an error
            ::fprintf(stderr, "ERROR(CVidBuffer): Error while retrieving next frame from %s (code=%d)\n", 
                  pStream->path, err);
            pStream->disabledOnError = true;
         }
         continue;
      }

      if(packet.stream_index == pStream->streamIdx) 
      {
         // decode the packet just received. Usually this contains a whole frame, but perhaps not.
         avcodec_decode_video2(pStream->pCodecCtx, pSrcFrame, &frameFinished, &packet);
            
         // if we have a complete frame, do colorspace conversion and put results in destination frame buffer
         if(frameFinished != 0) 
         {
            sws_scale(pStream->pSwsCtx, pSrcFrame->data, pSrcFrame->linesize, 0, pStream->height, 
                  pStream->pDstFrame->data, pStream->pDstFrame->linesize);
            gotFrame = true;
         }

         av_free_packet(&packet);
         av_init_packet(&packet);
      }
         
   }
   av_free_packet(&packet);

   // if we got the frame, increment the write index
   if(gotFrame) pStream->iWrite = (pStream->iWrite + 1) % CVidBuffer::QSIZE;

   // release allocated source video frame
   if(pSrcFrame != NULL) av_free(pSrcFrame);
}

/**
 Helper method closes an open video stream. Must not be called while buffering is in progess on the worker thread!
*/
void CVidBuffer::closeVideoStream(VideoStream* pStream)
{
   if(pStream == NULL) return;

   if(pStream->pSwsCtx != NULL)
   {
      sws_freeContext(pStream->pSwsCtx);
      pStream->pSwsCtx = NULL;
   }
   if(pStream->pCodecCtx != NULL)
   {
      avcodec_close(pStream->pCodecCtx);
      pStream->pCodecCtx = NULL;
   }
   if(pStream->pIOCtx != NULL)
   {
      // NOTE: The IO context buffer was initially av_malloc'd and supplied to avio_alloc_context(), but LIBAVFORMAT may
      // free it and replace it during operation. So we have to free the buffer stored in the context!
      av_freep(&(pStream->pIOCtx->buffer)); 
      av_freep(&(pStream->pIOCtx));
   }
   if(pStream->pFmtCtx != NULL)
   {
      avformat_close_input(&(pStream->pFmtCtx));
      pStream->pFmtCtx = NULL;
   }

   for(int i=0; i<CVidBuffer::QSIZE; i++) if(pStream->frameQueue[i] != NULL)
   {
      av_free(pStream->frameQueue[i]);
      pStream->frameQueue[i] = NULL;
   }

   if(pStream->pDstFrame != NULL)
   {
      av_free(pStream->pDstFrame);
      pStream->pDstFrame = NULL;
   }

   if(pStream->memFile.buf != NULL)
   {
      ::free(pStream->memFile.buf);
      pStream->memFile.buf = NULL;
      pStream->memFile.bufSz = 0;
      pStream->memFile.curr_pos = 0;
   }

   ::memset(pStream->path, 0, sizeof(char)*CVidBuffer::MAXPATHSZ);
   pStream->stopOnEOF = false;
   pStream->streamIdx = -1;
   pStream->iRead = 0;
   pStream->iWrite = 0;
   pStream->disabledOnError = true;
   pStream->gotEOF = false;
   pStream->width = 0;
   pStream->height = 0;
   pStream->rate = 0;
}

/**
 Static helper method loads an entire file into a large memory buffer, up to 30MB. 

 Based on code snippet at stackoverflow.com/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer. 
 Note that the file is read in 2MB chunks and the memory buffer is reallocated as needed. Any allocated memory is 
 released if the method fails.

 @param path The file pathname.
 @param ppBuf This will hold the starting address of the allocated byte buffer containing all the bytes read in from 
 the file. Will be NULL if method fails.
 @param pBufSz Point to the total size of the buffer in bytes. Will be set to 0 if method fails.
 @return True if successful; false otherwise. 
*/
bool CVidBuffer::loadFileIntoBuf(const char* path, uint8_t** ppBuf, size_t* pBufSz)
{
   if(path==NULL || ppBuf==NULL || pBufSz==NULL) return(false);
   *ppBuf = NULL;
   *pBufSz = 0;

   FILE* fd = ::fopen(path, "rb");
   if(fd == NULL) return(false);

   uint8_t* data = NULL;
   uint8_t* temp = NULL;
   size_t size = 0, used = 0, n = 0;

   while(true)
   {
      if(used + READCHUNK + 1 > size)
      {
         size = used + READCHUNK + 1;

         // check for overflow. Very unlikely.
         if(size <= used)
         {
            ::free(data);
            ::fclose(fd);
            return(false);
         }

         temp = (uint8_t*) ::realloc(data, size);
         if(temp == NULL)
         {
            ::free(data);
            ::fclose(fd);
            return(false);
         }
         data = temp;
      }

      n = ::fread(data + used, 1, READCHUNK, fd);
      if(n == 0) break;

      used += n;

      // cannot handle files larger than MAXPRELOADSZ
      if(used > MAXPRELOADSZ)
      {
         ::free(data);
         ::fclose(fd);
         return(false);
      }
   }
   
   if(::ferror(fd))
   {
      ::free(data);
      ::fclose(fd);
      return(false);
   }

   temp = (uint8_t*) ::realloc(data, used);
   if(temp == NULL)
   {
      ::free(data);
      ::fclose(fd);
      return(false);
   }
   data = temp;

   ::fclose(fd);

   *ppBuf = data;
   *pBufSz = used;
   return(true);
}


/**
 This custom AVIOContext callback reads a packet from a video source file that was stored in a memory buffer when the 
 video stream was opened.

 @oaram opaque A pointer to the CVidBuffer::MMapFile structure which holds the starting address of the memory buffer 
 holding the file's content, its size in bytes, and the current position within the file.
 @param buf The buffer into which packet is to be read.
 @param bufSz The size of the above buffer. Up to this many bytes are transferred.
 @return The number of bytes actually read. Returns AVERROR_EOF if there are no bytes left to read; -1 if an error
 occurred.
*/
int CVidBuffer::readPacketCB(void* opaque, uint8_t* buf, int bufSz)
{
   MemFile* pMemFile = (MemFile*) opaque;

   if(pMemFile->curr_pos >= pMemFile->bufSz) return(AVERROR_EOF);

   if(pMemFile->curr_pos + bufSz > pMemFile->bufSz) bufSz = pMemFile->bufSz - pMemFile->curr_pos;

   ::memcpy(buf, pMemFile->buf + pMemFile->curr_pos, bufSz);
   pMemFile->curr_pos += bufSz;

   return(bufSz);
}

/**
 This custom AVIOContext callback seeks to a specified location in a video source file that was stored in a memory
 buffer when the video stream was opened.

 @oaram opaque A pointer to the CVidBuffer::MMapFile structure which holds the starting address of the memory buffer 
 holding the file's content, its size in bytes, and the current position within the file.
 @param offset Position offset WRT the whence argument. If whence==AVSEEK_SIZE, this argument is ignored.
 @param whence SEEK_SET, SEEK_CUR, SEEK_END; AVSEEK_SIZE returns file size.
 @return The position within buffered file after the seek. Returns EINVAL if whence is not one of the specified 
 values.
*/
int64_t CVidBuffer::seekCB(void* opaque, int64_t offset, int whence)
{
   MemFile* pMemFile = (MemFile*) opaque;
   
   if(whence==AVSEEK_SIZE) return(pMemFile->bufSz);

   if(whence==SEEK_SET)
      pMemFile->curr_pos = offset;
   else if(whence==SEEK_CUR)
      pMemFile->curr_pos += offset;
   else if(whence==SEEK_END)
      pMemFile->curr_pos = pMemFile->bufSz + offset;
   else
      return(AVERROR(EINVAL));

   return(pMemFile->curr_pos);
}

