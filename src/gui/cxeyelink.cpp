//=====================================================================================================================
//
// cxeyelink.cpp : Implementation of class CCxEyeLink, interface to the EyeLink 1000-Plus eye tracker
//
// AUTHOR:  saruffner
//
//
// DESCRIPTION:
// 01sep2015: Began development. The idea is to use this class to encapsulate communications with the EyeLink tracker 
// over a dedicated Ethernet connection. It will rely on the SR Research-provided EyeLink SDK, a C-based API library. 
// The SDK uses real-time thread priorities in Win32 to stream data over the link fast enough, so I’m not sure this 
// will work at all in the Maestro GUI process. I’m developing this class to run a series of tests to assess whether 
// it is even possible.
// 03dec2015: Finished initial development and testing of this solution for integrating the EyeLink tracker with
// Maestro. It has proven feasible to stream data over the dedicated Ethernet link at 1KHz without bogging down the
// GUI, even though we're using Windows networking.
// 25jan2016: Revised to allow X and Y calibration gains to be positive or negative. Polarity of raw tracker data is
// flipped in Mouse Simulation mode vs normal operation.
// 07sep2017: Mod for Maestro 4.x: The worker thread requires a 1ms timer period so that Sleep(1) will, hopefully,
// do just that -- allow the thread to run again within a millisecond of yielding the CPU. Since we are initially
// attempting to fold the RTX-based runtime engine of Maestro 3.x into a Windows thread in 4.x, that engine will also
// require the finer timer resolution. Since the timer period is a global resource, we need to call timeBeginPeriod()
// when we enter a time-critical state, then timeEndPeriod() when we leave that state. We moved the functionality to
// CCxRuntime.
// 01may2019: During 4.x development, the attempt at re-implementing the RTX runtime engine as a Windows thread did NOT
// work, so I went back to using the RTX-based CXDRIVER and the original version of CCxRuntime. However, at that time,
// I failed to restore the timer resolution control code that I had removed from CCxEyeLink on 07sep2017. As a result,
// Maestro 4.0.x did not work with the EyeLink tracker. Restored the timer resolution control code for V4.0.5.
//=====================================================================================================================

#include "stdafx.h"                          // standard MFC stuff

#include "cntrlx.h"                          // CCntrlxApp and resource IDs for CNTRLX
#include "mmsystem.h"                        // for multimedia timer capabilities
#include "cxeyelink.h"



/** Constructor. */
CCxEyeLink::CCxEyeLink() 
{
   m_iXOfs = m_iYOfs = EL_DEFOFS;
   m_iXGain = m_iYGain = EL_DEFGAIN;
   m_iVelSmoothW = EL_DEFSMOOTHW;
   ResetState(); 
}

/** 
 Destructor. Will attempt to terminate the underlying worker thread if it is still alive. Better to terminate that
 thread explicitly. 
 */
CCxEyeLink::~CCxEyeLink() { Disconnect(); }

/**
 Adjust a calibration parameter for the Eyelink tracker system. May be called at any time, even when Maestro is not
 connected to the tracker via the dedicated Ethernet link. This merely sets the offset or gain in the X or Y direction.
 Note that gain may be positive or negative, so that the raw pupil coordinates from the tracker can be inverted if
 necessary.
 @param isX TRUE for X-coordinate calibration, FALSE for Y.
 @param isOfs TRUE to adjust the offset parameter, FALSE to adjust the gain.
 @param value The parameter value. Offset is restricted to [EL_MINOFS..EL_MAXOFS], gain to +/-[EL_MINGAIN..EL_MAXGAIN].
 Arbitrary units.
 @return TRUE if value accepted, FALSE if auto-corrected because it was out-of-range.
 */
BOOL CCxEyeLink::SetCalParam(BOOL isX, BOOL isOfs, int value)
{
   int lo = isOfs ? EL_MINOFS : EL_MINGAIN;
   int hi = isOfs ? EL_MAXOFS : EL_MAXGAIN;
   int corr = cMath::rangeLimit(isOfs ? value : cMath::abs(value), lo, hi);
   if((!isOfs) && value < 0) corr = -corr;

   if(isX) 
   {
      if(isOfs) m_iXOfs = corr;
      else m_iXGain = corr;
   }
   else
   {
      if(isOfs) m_iYOfs = corr;
      else m_iYGain = corr;
   }

   // if recording, notify service thread of the change
   if(IsRecording()) m_bParamsChanged = TRUE;

   return(BOOL(corr == value));
}

/**
 Set the window width for the sliding-average filter that smooths the velocity signal obtained by differentiating the
 Eyelink eye position signal. The larger the width, the more the smoothe velocity signal will lag the corresponding
 position signal.
 @param The new window width in ms. Range-restricted to [EL_MINSMOOTHW..EL_MAXSMOOTHW].
 @return TRUE if value accepted, FALSE if auto-corrected or if recording is in progress. The velocity smoothing width
 cannot be altered while recording.
 */
BOOL CCxEyeLink::SetVelocityFilterWidth(int w)
{
   if(m_bRecord || m_bRecording) return(FALSE);
   m_iVelSmoothW = cMath::rangeLimit(w, EL_MINSMOOTHW, EL_MAXSMOOTHW);
   return(m_iVelSmoothW == w);
}


/**
 Start the background thread that handles all communications with the EyeLink tracker over an Ethernet connection. The
 thread will connect to the Eyelink, put the tracker in the idle/offline mode, and then wait (actively, using 100ms
 sleep cycle) for a command to start recording or disconnect.

 Before starting the worker thread, the method verifies that the minimum system timer resolution is 1ms. If not, the
 Eyelink interface is disabled. We require the 1ms time slice when recording is in progress.

 IMPORTANT: The method only waits until the worker thread has started, so it should return quickly. It may take some
 time for the worker thread to establish the connection to the tracker, or it could time-out while attempting to 
 connect. Call CheckConnectionStatus() regularly to detect a change in the Eyelink's connection status.

 @param pIPCShm Pointer to the IPC data structure used to communicate with Maestro's runtime engine. Needed so that 
 Eyelink service thread can stream Eyelink samples directly to the engine when Eyelink recording is in progress.
 @return True if service thread was successfully started or if it is already running, false if operation failed (unable
 to start worker thread, or 1ms timer resolution not supported). The application message log is updated on success or 
 failure.
 */
BOOL CCxEyeLink::Connect(volatile PCXIPCSM pIPCShm)
{
   // null pointer protection -- should never happen
   if(pIPCShm == NULL) return(FALSE);

   // nothing to do if thread is already running
   if(m_bAlive) return(TRUE);
   ResetState();

   // verify that system supports a minimum timer resolution of 1ms
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
   TIMECAPS tc;
   if(::timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR || (tc.wPeriodMin > 1))
   {
      pApp->LogMessage(_T("[Eyelink] Cannot verify timer resolution, or it exceeds 1ms. Eyelink not supported."));
      return(FALSE);
   }

   // start worker thread and wait up to 0.5sec for it to start. Thread object is auto-deleted at termination.
   m_connState = CONNECTING;
   m_pWorker = ::AfxBeginThread(CCxEyeLink::WorkerEntry, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0,  0, NULL);
   if(m_pWorker == NULL)
   {
      m_connState = NOT_CONNECTED;
      pApp->LogMessage(_T("[Eyelink] Failed to spawn tracker service thread!"));
      return(FALSE);
   }

   CElapsedTime eTime;
   while(eTime.Get() < 500000.0 && !m_bAlive) ::Sleep(20);

   // worker thread failed to start, or it terminated immediately b/c connection failed. If it did start,
   // store reference to IPC data structure so that Eyelink thread can stream samples to the runtime engine thread.
   BOOL bOk = m_bAlive;
   if(!bOk)
   {
      m_pWorker = NULL;
      m_bDie = TRUE;
      m_connState = NOT_CONNECTED;
   }
   else
      m_pShm = pIPCShm;

   // update application message log
   m_criticalSec.Lock();
   if(!m_strMsg.IsEmpty())
      pApp->LogMessage(m_strMsg);
   else
      pApp->LogMessage(bOk ? _T("[Eyelink] Tracker service thread started.") :
         _T("[EyeLink] Tracker service thread failed to start, or connection failed."));
   m_strMsg.Empty();
   m_criticalSec.Unlock();

   // NOTE: Connection status won't be updated until checked via CheckConnectionStatus().

   return(bOk);
}

/**
 Terminate the background thread that handles all EyeLink communications, disconnecting from the tracker in the 
 process. The method waits up to 1 second for the worker thread to terminate.
 @return True if successfull, or if thread is not running; false if thread failed to terminate. The application message
 log is updated on success or failure.
 */
BOOL CCxEyeLink::Disconnect()
{
   // thread is already stopped
   if(!m_bAlive)
   {
      ResetState();
      return(TRUE);
   }

   // tell worker thread to disconnect from EyeLink and terminate. Wait up to 1 second.
   m_bDie = TRUE; 
   CElapsedTime eTime; 
   while(m_bAlive && eTime.Get() < 1000000.0) ::Sleep(20);

   // success only if worker thread terminated normally. Worker thread may be left dangling on failure.
   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
   BOOL bOk = !m_bAlive; 

   // update application message log
   m_criticalSec.Lock();
   if(!m_strMsg.IsEmpty())
      pApp->LogMessage(m_strMsg);
   else
      pApp->LogMessage(bOk ? _T("[EyeLink] Successfully disconnected from tracker.") :
         _T("[EyeLink] Tracker service thread failed to terminate normally!"));
   m_strMsg.Empty();
   m_criticalSec.Unlock();

   // make sure we've restored timer resolution to its previous value. We use a 1-ms resolution during
   // recording, and we have to call timeEndPeriod with that value to restore the previous setting.
   if(m_bSetMinRes)
   {
      ::timeEndPeriod(1);
      m_bSetMinRes = FALSE;
   }

   ResetState();
   return(bOk);
}

/**
 Check for a change in the status of the Ethernet connection to the Eyelink tracker, and post a status message from the
 tracker service thread, if any, to the application message log.

 IMPORTANT: When Connect() is called, it does NOT wait for the service thread to establish a connection to the tracker,
 since that task can take an indeterminate amount of time. Also, once connected, the connection may be lost at any time.
 This method should be called on a regular basis (during idle time in the application message loop) to check for any
 change in the tracker's connection status.
 
 @return True if there was a change in connection status, else false.
 */
BOOL CCxEyeLink::CheckConnectionStatus()
{
   BOOL bChanged = FALSE;
   if(m_connState == CONNECTING && m_bConnected)
   {
      m_connState = CONNECTED;
      bChanged = TRUE;
   }
   else if(m_connState != NOT_CONNECTED && !m_bAlive)
   {
      m_connState = NOT_CONNECTED;
      bChanged = TRUE;
   }

   // update application message log if there's a status/warning/error message to post.
   m_criticalSec.Lock();
   if(!m_strMsg.IsEmpty())
   {
      ((CCntrlxApp*) AfxGetApp())->LogMessage(m_strMsg);
      m_strMsg.Empty();
   }
   m_criticalSec.Unlock();

   // if service thread died unexpectedly, reset the Eyelink interface.
   if(bChanged && !m_bAlive) ResetState();

   return(bChanged);
}

/**
 Start recording on the Eyelink tracker and stream raw eye position data over IPC to Maestro's RTX-based driver. The
 method sets the timer resolution to 1ms, "wakes up" the worker thread that handles all Eyelink operations, and waits
 for that thread to indicate that recording has begun. If recording has not started within 2 secs, the method fails
 and the Eyelink interface is disabled. No action taken if Eyelink tracker is not connected, or if recording is already
 in progess.
 @return True if recording started, else false. The application message log is updated on success or failure.
 */
BOOL CCxEyeLink::StartRecord()
{
   if(m_connState != CONNECTED) return(FALSE);
   if(m_bRecord) return(TRUE);

   // set system timer resolution to 1ms for finer time-slicing of threads needed when recording on Eyelink. Need to
   // stream the eye position data to CXDRIVER as frequently as possible!
   if(!m_bSetMinRes)
   {
      ::timeBeginPeriod(1);
      m_bSetMinRes = TRUE;
   }

   // tell worker thread to start recording, then wait up to 2 seconds for it to do so
   m_bRecord = TRUE;
   CElapsedTime eTime;
   while((!m_bRecording) && eTime.Get() < 2000000.0) ::Sleep(20);

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
   BOOL bOk = m_bRecording;

   // update application message log
   m_criticalSec.Lock();
   if(!m_strMsg.IsEmpty())
      pApp->LogMessage(m_strMsg);
   else
      pApp->LogMessage(bOk ? _T("[Eyelink] Tracker recording started") : 
         _T("[Eyelink] Tracker recording did not start - timeout"));
   m_strMsg.Empty();
   m_criticalSec.Unlock();

   // if recording did not start, restore default timer resolution. If worker died, clean up and notify user.
   if(!bOk)
   {
      ::timeEndPeriod(1);
      m_bSetMinRes = FALSE;
      if(!m_bAlive)
      {
         ResetState();
         pApp->LogMessage(_T("[Eyelink] Eye tracker is offline. Reconnect if you wish to use it."));
      }
   }
   
   return(bOk);
}

/** 
 Stop recording in progress on the Eyelink tracker. The method signals the worker thread to stop recording, waits
 up to two seconds for recording to stop (tracker is returned to idle/offline mode), then restores the previous
 system timer resolution. If worker thread fails to respond, the method terminates it, in which case the Eyelink
 interface is no longer available.
 @return True if operation successful or recording was not in progress, else false. The application message log is 
 updated on success or failure.
 */
BOOL CCxEyeLink::StopRecord()
{
   if(!(m_bAlive || m_bRecord)) return(TRUE);

   // tell worker thread to stop recording, then wait up to 2 seconds for it to do so
   m_bRecord = FALSE;
   CElapsedTime eTime;
   while(m_bRecording && eTime.Get() < 2000000.0) ::Sleep(20);

   CCntrlxApp* pApp = (CCntrlxApp*) AfxGetApp();
   BOOL bOk = !m_bRecording;

   // update application message log
   m_criticalSec.Lock();
   if(!m_strMsg.IsEmpty())
      pApp->LogMessage(m_strMsg);
   else
      pApp->LogMessage(bOk ? _T("[Eyelink] Tracker recording stopped.") : 
         _T("[Eyelink] Tracker service thread failed to stop recording."));
   m_strMsg.Empty();
   m_criticalSec.Unlock();

   if(!bOk)
   {
      pApp->LogMessage(_T("[Eyelink] Disconnecting..."));
      Disconnect();
   }

   // restore normal timer resolution
   if(m_bSetMinRes)
   {
      ::timeEndPeriod(1);
      m_bSetMinRes = FALSE;
   }

   return(bOk);
}

/**
 Initialize state of the EyeLink interface object prior to starting the background worker thread that handles all
 EyeLink-related operations, or reset the state after the worker thread has terminated.
 */
VOID CCxEyeLink::ResetState()
{
   m_pWorker = NULL;
   m_strMsg.Empty();
   m_bParamsChanged = FALSE;
   m_bAlive = FALSE;
   m_bConnected = FALSE;
   m_bDie = FALSE;
   m_bRecord = FALSE;
   m_bRecording = FALSE;
   if(m_bSetMinRes)
   {
      ::timeEndPeriod(1);
      m_bSetMinRes = FALSE;
   }
   m_connState = NOT_CONNECTED;
}

/**
 The worker thread procedure.
 @returns Exit code (0 always - not used).
 */
UINT CCxEyeLink::Worker()
{
   // set flag indicating that this thread is running
   m_bAlive = TRUE;

   // attempt to connect to the EyeLink. Terminate on failure. [Note that we initialize DLL then use eyelink_open()
   // because open_eyelink_connection(0) will raise a Windows dialog on failure! Also, during testing, we use
   // eyelink_dummy_open() for a simulated connection.]
   int res = open_eyelink_connection(-1);
   if(res == 0) res = eyelink_open();
   CString msg;
   if(res != 0)
      msg.Format("[Eyelink] Connect failed: %s", eyelink_get_error(res, "eyelink_open"));
   else
   {
      set_offline_mode();
      msg.Format(_T("[Eyelink] Connected to tracker successfully. Tracker placed in idle mode."));
   }

   m_criticalSec.Lock();
   m_strMsg = msg;
   m_criticalSec.Unlock();

   // if connection failed, terminate
   m_bConnected = BOOL(res == 0);
   if(!m_bConnected)
   {
      close_eyelink_system();
      m_bAlive = FALSE;
      return(0);
   }

   // loop endlessly, waiting for command to terminate, or enter a recording session
   m_pShm->iELStatus = CX_ELSTAT_IDLE;
   while(!m_bDie) 
   {  
      ::Sleep(100);

      // if connection lost, terminate
      if(eyelink_is_connected() == 0)
      {
         m_criticalSec.Lock();
         m_strMsg.Format(_T("[Eyelink] Tracker connection lost; service thread terminating."));
         m_criticalSec.Unlock();
         break;
      }
      
      // start recording if requested. Note that we configure the link to only send raw pupil samples, not
      // calibrated gaze data, and no eye, button or input events.
      if(m_bRecord)
      {
         m_bRecording = Worker_StartRecord();
         if(!m_bRecording)
         {
            m_bRecord = FALSE;
            continue;
         }

         BOOL bSleep = TRUE;
         while(m_bRecord && !m_bDie)
         {
            // check for loss of connection
            if(eyelink_is_connected() == 0)
            {
               m_pShm->iELStatus = CX_ELSTAT_FAIL;
               m_criticalSec.Lock();
               m_strMsg.Format(_T("[Eyelink] !! Tracker connection lost; service thread terminating !!"));
               m_criticalSec.Unlock();
               m_bDie = TRUE;
               break;
            }

            // check to see if EyeLink Host PC has left record mode (probably user abort on that side)
            if(eyelink_tracker_mode() != EL_RECORD_MODE)
            {
               m_pShm->iELStatus = CX_ELSTAT_FAIL;
               m_criticalSec.Lock();
               m_strMsg.Format(_T("[Eyelink] !! Recording aborted on tracker side !!"));
               m_criticalSec.Unlock();
               break;
            }

            // process next sample from tracker and send it to MaestroDRIVER via IPC
            int res = Worker_GetNextSample(FALSE);
            if(res < 0) break;

            // don't sleep if another sample is already available -- so we can "catch-up" with sample stream
            if(res == 0)
            {
               // the timer resolution must be set to the minimum <= 1ms -- or this won't work!
               if(bSleep) ::Sleep(1);
               bSleep = !bSleep;
            }
         }

         Worker_StopRecord();
         m_bRecord = FALSE;
         m_bRecording = FALSE;
      }
   }

   // close connection to EyeLink (if we opened it in the first place)
   close_eyelink_system();
   m_pShm->iELStatus = CX_ELSTAT_OFF;

   m_bRecording = FALSE;
   m_bConnected = FALSE;
   m_bAlive = FALSE;
   return(0);
}

/**
 This method is invoked by the worker thread to initiate an Eyelink recording session. 

 First, the thread's priority is raised to "time critical" to (hopefully) ensure it can service the Eyelink sample
 queue while recording is in progress. Then the Eyelink is configured to stream only raw pupil data (both L and R
 eyes) and digital input events over the Ethernet link (currently, any input events are discarded, but we might use
 them in the future), and recording is started. The first sample is drawn from the link, the sample buffer in IPC is 
 reinitialized to indicate that Eyelink recording is in progress, then that first sample is pushed into the buffer. 
 If recording fails to start, the worker thread's priority is restored to normal.
 
 @return True if recording started successfully, false otherwise.
 */
BOOL CCxEyeLink::Worker_StartRecord()
{
   if(!::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
   {
      m_criticalSec.Lock();
      m_strMsg = _T("[Eyelink] Failed to raise priority of tracker service thread; cannot record");
      m_criticalSec.Unlock();
      return(FALSE);
   }

   // by sleeping here, we ensure the 1ms time slice has taken effect (which must be set by GUI thread before invoking
   // StartRecord(). Otherwise, the worker thread could get interrupted for 10ms the first time it sleeps after 
   // starting the Eyelink recording. (That's what appeared to be happening during tests; adding this statement 
   // eliminated the issue!)
   ::Sleep(10);

   // reset queues used for eye velocity computations
   for(int i=0; i<3; i++) { for(int j=0; j<2; j++) m_posBuf[i][j].fx = m_posBuf[i][j].fy = 0.0f; }
   m_iOldestPos = 2;
   for(int i=0; i<EL_MAXSMOOTHW; i++) { for(int j=0; j<2; j++) m_velBuf[i][j].fx = m_velBuf[i][j].fy = 0.0f; }
   m_iOldestVel = m_iVelSmoothW-1;
   for(int i=0; i<2; i++) m_currVelSum[i].fx = m_currVelSum[i].fy = 0.0f;

   // configure and start recording. Restore normal thread priority on failure.
   CString eMsg;
   int res = eyecmd_printf("link_sample_data = LEFT,RIGHT,PUPIL,INPUT");
   if(res != 0) 
      eMsg.Format(_T("[Eyelink] Failed to configure link sample data for recording"));
   else
   {
      res = start_recording(0, 0, 1, 0); 
      if(res != 0) eMsg.Format("[Eyelink] Recording did not start: %s", eyelink_get_error(res, "start_recording"));
   }
   if(res != 0)
   {
      m_criticalSec.Lock();
      m_strMsg = eMsg;
      m_criticalSec.Unlock();
      ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
      return(FALSE);
   }

   // once we have a sample, we can check which eye is being recorded -- or both
   res = eyelink_eye_available();
   m_bRecEye[0] = (res==BINOCULAR || res==LEFT_EYE);
   m_bRecEye[1] = (res==BINOCULAR || res==RIGHT_EYE);

   // recording started on Eyelink. Reinit Eyelink sample buffer in runtime engine to prepare for data transfer.
   m_pShm->iELLast = 0;
   m_pShm->iELNext = 0;
   m_pShm->iELStatus = CX_ELSTAT_REC;
   m_pShm->iELParams[0] = m_iXOfs;
   m_pShm->iELParams[1] = m_iYOfs;
   m_pShm->iELParams[2] = m_iXGain;
   m_pShm->iELParams[3] = m_iYGain;
   m_pShm->iELParams[4] = m_iVelSmoothW;
   m_bParamsChanged = FALSE;
   m_pShm->iELRecType = (res==BINOCULAR) ? EL_BINOCULAR : (res==RIGHT_EYE ? EL_MONO_RIGHT : EL_MONO_LEFT);

   // first sample should be ready. Put in IPC buffer now. Fail if sample is not ready.
   m_tsRecStart = 0;
   m_nSamplesRec = 0;
   m_tsLastSamp = 0;
   if(Worker_GetNextSample(TRUE) < 0)
   {
      Worker_StopRecord();
      return(FALSE);
   }

   return(TRUE);
}

/**
 This method is invoked by the worker thread to retrieve the next Eyelink tracker sample from the tracker link's queue
 and forward it to MaestroDRIVER via a dedicated buffer in IPC.

 The method handles the details of converting the raw Eyelink pupil coordinates to calibrated gaze position in visual 
 degrees, using the calibration parameters set in the Eyelink interface object (user-controlled). It also computes eye 
 velocity as a smoothed version of the center-point difference in position. V(t) = smooth( (P(t) - P(t-2)) / 2 ). The 
 smoothed velocity is the sliding-window average of the last N velocity samples calculated by the center-point 
 difference method, where window width N (allowed range EL_MINSMOOTHW..EL_MAXSMOOTHW] is a user-controlled parameter 
 set in the Eyelink interface. A 3-element position queue and an N-element velocity queue are maintained in order to do
 the necessary calculations. Note that the calculations are performed independently for the X,Y coordinates of each eye.

 The method ASSUMES that the Eyelink is recording and streaming pupil data at 1KHz. Thus, it should be called at least 
 once per millisecond in order to keep up with that data rate. Each subsequent eye data sample should have a timestamp 
 that is 1ms greater than the timestamp of the previous sample.

 The method will fail under the following error conditions: (1) A recording session has just started and a sample was
 not immediately available in the link queue. (2) The timestamp of the retrieved sample was not 1ms greater than that
 of the previous sample. (3) The worker thread has fallen 50+ milliseconds behind in the recording session (this could
 happen if the time-critical worker thread's periodic execution is delayed for some reason). (4) The  IPC buffer has 
 overflowed, indicating that MaestroDRIVER has not kept pace with the tracker data stream for some  reason. On failure,
 the method will set the Eyelink interface's warning/error message and return FALSE. In this case, the recording 
 session must be terminated.

 @param isFirst If true, then a recording session has just started. In this case, there should be a tracker sample
 ready in the link queue. The sample's timestamp is considered the "start time" for the recording session.
 @return 0 if operation succeeded (even if no sample was available); 1 if operation succeeded AND another sample is 
 pending in the link queue; -1 if one of the described error conditions was detected. Recording session must be 
 terminated if an error condition is detected!
 */
int CCxEyeLink::Worker_GetNextSample(BOOL isFirst)
{
   // retrieve next raw sample from link queue
   int res = eyelink_get_sample(&m_rawSample);

   // if no sample is available, fail immediately if it's the first sample of the recording session, or if we're 
   // lagging the recorded timeline by 50ms or more. Else, assume next sample isn't available yet.
   if(res == 0)
   {
      if(isFirst)
      {
         m_pShm->iELStatus = CX_ELSTAT_FAIL;
         m_criticalSec.Lock();
         m_strMsg.Format(_T("[Eyelink] Stopped recording on error: No sample ready at record start!"));
         m_criticalSec.Unlock();
         return(-1);
      }
      else if(m_etRec.Get() - m_etLastSamp > 50000.0)
      {
         m_pShm->iELStatus = CX_ELSTAT_FAIL;
         m_criticalSec.Lock();
         m_strMsg.Format(_T("[Eyelink] Stopped recording on error: Sample lag time exceeded 50ms!"));
         m_criticalSec.Unlock();
         return(-1);
      }
      return(0);
   }

   // check sample time and abort if we're lagging too much or if sample time not consistent with 1KHz rate
   if(isFirst)
   {
      m_tsRecStart = m_rawSample.time;
      m_tsLastSamp = 0;
      m_nSamplesRec = 1;
      m_etRec.Reset();
      m_etLastSamp = 0;
   }
   else
   {
      double tElapsed = m_etRec.Get();
      if(tElapsed - m_etLastSamp > 50000.0)
      {
         m_pShm->iELStatus = CX_ELSTAT_FAIL; 
         m_criticalSec.Lock();
         m_strMsg.Format(_T("[Eyelink] Stopped recording on error: Sample lag time exceeded 50ms!"));
         m_criticalSec.Unlock();
         return(-1);
      }
      else if(m_tsLastSamp + 1 != (m_rawSample.time-m_tsRecStart))
      {
         m_pShm->iELStatus = CX_ELSTAT_FAIL;
         m_criticalSec.Lock();
         m_strMsg.Format(_T("[Eyelink] Stopped recording on error: Sample timestamp not consistent with 1KHz rate! nSamps=%d, last=%d, now=%d"),
            m_nSamplesRec, m_tsLastSamp, (m_rawSample.time-m_tsRecStart));
         m_criticalSec.Unlock();
         return(-1);
      }

      m_etLastSamp = tElapsed;
      m_tsLastSamp = m_rawSample.time-m_tsRecStart;
      ++m_nSamplesRec;
   }

   // get next available slot in circular IPC buffer; check for overflow
   int iNext = (m_pShm->iELNext + 1) % CX_MAXEL;
   if(iNext == m_pShm->iELLast)
   {
      m_pShm->iELStatus = CX_ELSTAT_FAIL;
      m_criticalSec.Lock();
      m_strMsg.Format(_T("[Eyelink] Stopped recording on error: IPC buffer overflow!"));
      m_criticalSec.Unlock();
      return(-1);
   }
   
   // update "oldest" indices in the circular position and velocity queues now, remembering the previous value so we
   // can overwrite the oldest values...
   int oldP = m_iOldestPos;
   if(--m_iOldestPos < 0) m_iOldestPos = 2;
   int oldV = m_iOldestVel;
   if(--m_iOldestVel < 0) m_iOldestVel = m_iVelSmoothW - 1;

   // process the new sample...
   PELSAMP p = &(m_pShm->elSamples[iNext]);
   ::memset(p, 0, sizeof(ELSAMP));
   p->ts = m_tsLastSamp;
   for(int i=0; i<2; i++) if(m_bRecEye[i])
   {
      // calc eye position in visual deg from raw pupil data in sample, using the calibration offset and gain in X,Y.
      // If raw sample is missing, carry over the last good sample.
      p->gotEye[i] = (m_rawSample.px[i] != MISSING_DATA && m_rawSample.py[i] != MISSING_DATA);
      if(p->gotEye[i])
      {
         p->pos[i].fx = ((float) (m_rawSample.px[i] - m_iXOfs)) / ((float) m_iXGain);
         p->pos[i].fy = ((float) (m_rawSample.py[i] - m_iYOfs)) / ((float) m_iYGain);
      }
      else
         p->pos[i] = m_posBuf[(oldP+1)%3][i];

      // overwrite the oldest eye position in circular queue with the new sample
      m_posBuf[oldP][i] = p->pos[i];

      // calc eye velocity for this sample time by the center-point difference method. NOTE that we use the updated
      // "oldest position" index here! Also we assume position data is sampled at 1KHz, so time delta is 2ms = 0.002s.
      int t = (m_iOldestPos+1) % 3;
      int t0 = m_iOldestPos;
      float xVel = (m_posBuf[t][i].fx - m_posBuf[t0][i].fx) / 0.002f;
      float yVel = (m_posBuf[t][i].fy - m_posBuf[t0][i].fy) / 0.002f;

      // remove the contribution of the oldest velocity sample to the sample sum and add the contribution of the new 
      // sample. The new sample then replaces the old sample in the circular velocity queue.
      m_currVelSum[i].fx = m_currVelSum[i].fx + xVel - m_velBuf[oldV][i].fx; 
      m_currVelSum[i].fy = m_currVelSum[i].fy + yVel - m_velBuf[oldV][i].fy;
      m_velBuf[oldV][i].fx = xVel;
      m_velBuf[oldV][i].fy = yVel;

      // finally, calculate the smoothed velocity by dividing the sum over the values in the velocity queue by the
      // queue width. By keeping track of the sum in the manner above, we don't have to add all the values each time.
      p->vel[i].fx = m_currVelSum[i].fx / ((float) m_iVelSmoothW);
      p->vel[i].fy = m_currVelSum[i].fy / ((float) m_iVelSmoothW);
   }

   // the new sample is now available in the runtime engine's buffer
   m_pShm->iELNext = iNext;

   // calibration parameters may be changed in the GUI while recording. If so, update their values in IPC so MaestroDRIVER
   // has access to them. The velocity filter window width cannot change while recording.
   if(m_bParamsChanged)
   {
      m_pShm->iELParams[0] = m_iXOfs;
      m_pShm->iELParams[1] = m_iYOfs;
      m_pShm->iELParams[2] = m_iXGain;
      m_pShm->iELParams[3] = m_iYGain;
      m_bParamsChanged = FALSE;
   }

   return(eyelink_data_count(1,0) == 0 ? 0 : 1);
}

/**
 This method is invoked by the worker thread to terminate an Eyelink recording session. 
 
 It immediately disables the Eyelink sample buffer in IPC, stops recording, and places the tracker in the idle 
 "offline" mode. The worker thread's priority is restored to normal.
 */
VOID CCxEyeLink::Worker_StopRecord()
{
   // disable the Eyelink sample queue (unless it was already disabled by an error condition)
   if(m_pShm->iELStatus != CX_ELSTAT_FAIL) m_pShm->iELStatus = CX_ELSTAT_IDLE;

   stop_recording();
   set_offline_mode();

   ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}