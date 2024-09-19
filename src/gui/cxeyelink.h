//=====================================================================================================================
//
// cxeyelink.h : Declaration of class CCxEyeLink, which encapsulates a worker thread that uses the Eyelink API to 
//   communicate with the SR Research EyeLink 1000-Plus eye tracker via Ethernet connection.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXEYELINK_H__INCLUDED_)
#define CXEYELINK_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxmt.h>

#include "cxipc.h"               // constants and data structure defining the Maestro runtime engine interface
#include "util.h"                // for CElapsedTime
#include "core_expt.h"           // includes for the EyeLink SDK API (C code)

class CCxEyeLink
{
private:
   // worker thread that handles all EyeLink operations
   CWinThread* m_pWorker;

   // to mediate access to selected object state between caller and worker threads. Only for non-time-critical stuff.
   CCriticalSection m_criticalSec;
   // from worker: status, warning or error message from service thread. Use crit sec to access.
   CString m_strMsg; 

   // to worker: gain and offset calibration parameters that convert Eyeylink raw pupil coordinates to visual deg.
   // We don't bother using critical section to access, since these will only change during user calibration. Note
   // that gain can be positive or negative, so that user can invert raw pupil coordinates if necessary.
   int m_iXOfs;
   int m_iXGain;
   int m_iYOfs;
   int m_iYGain;

   // to worker: velocity smoothing window width in ms. Cannot change while recording in progress.
   int m_iVelSmoothW;
   // to worker: set TRUE to tell worker thread that calibration params have been updated.
   BOOL m_bParamsChanged;

   // from worker: TRUE while worker thread is alive. Auto-terminates if connection to Eyelink is lost.
   BOOL m_bAlive; 
   // from worker: TRUE once worker thread has established a connection to Eyelink. Reset if connection lost.
   BOOL m_bConnected;
   // to worker: set TRUE to tell worker thread to disconnect from Eyelink and terminate
   BOOL m_bDie; 
   // to worker: set TRUE to start, FALSE to stop Eyelink recording
   BOOL m_bRecord;
   // from worker: TRUE while Eyelink is recording and worker is streaming data to runtime engine 
   BOOL m_bRecording;

   // this flag is set when the system's timer resolution has been set to its minimum value.
   BOOL m_bSetMinRes;

   // connection status
   typedef enum
   {
      NOT_CONNECTED = 0,
      CONNECTING,
      CONNECTED
   } ELState;

   ELState m_connState;

   // (worker only) Eyelink timestamp when current recording session began, in ms since tracker activated.
   UINT32 m_tsRecStart;
   // (worker only) Eyelink timestamp of last retrieved sample, in ms since recording began (first sample time = 0).
   UINT32 m_tsLastSamp;
   // (worker only) number of eye data samples since current recording session began
   UINT32 m_nSamplesRec;
   // (worker only) elapsed time since current recording session began 
   CElapsedTime m_etRec;
   // (worker only) elapsed time in microsecs when last sample retrieved - to check for sample delays
   double m_etLastSamp;
   // (worker only) indicates which eye (or both) is recorded; L=0, R=1
   BOOL m_bRecEye[2];
   // (worker only) a raw sample from tracker (to avoid heap allocation)
   ISAMPLE m_rawSample;

   // (worker only) Eyelink position data queue -- for computing velocity via central-point difference. L=0, R=1.
   ELCOORD m_posBuf[3][2];
   int m_iOldestPos;

   // (worker only) calculated velocity data queue -- for smoothing velocity via sliding average. L=0, R=1.
   ELCOORD m_velBuf[EL_MAXSMOOTHW][2];
   ELCOORD m_currVelSum[2];
   int m_iOldestVel;

   // pointer to structure defining the communicatoin interface for Maestro's runtime engine (which runs on another
   // thread separate from the application's primary GUI thread). Accessed only by the EyeLink service thread in order
   // to transfer Eyelink samples to the runtime engine whenever Eyelink recording is in progress.
   volatile PCXIPCSM m_pShm;

private:
   // no copy constructor or assignment operator defined
   CCxEyeLink(const CCxEyeLink& src); 
   CCxEyeLink& operator=(const CCxEyeLink& src);

public:
   CCxEyeLink();
   ~CCxEyeLink();

   int GetCalParam(BOOL isX, BOOL isOfs)
   {
      return(isX ? (isOfs ? m_iXOfs : m_iXGain) : (isOfs ? m_iYOfs : m_iYGain));
   }
   BOOL SetCalParam(BOOL isX, BOOL isOfs, int value);

   // get/set velocity smoothing filter window width
   int GetVelocityFilterWidth() { return(m_iVelSmoothW); }
   BOOL SetVelocityFilterWidth(int w);

   // connect to Eyelink and put tracker in idle/offline mode
   BOOL Connect(volatile PCXIPCSM pIPCShm);
   // disconnect from Eyelink
   BOOL Disconnect();
   // call regularly (in idle time) to check for a change in Eyelink connection status
   BOOL CheckConnectionStatus();
   // connection to Eyelink is up and functioning normally
   BOOL IsConnected() { return(m_bAlive && m_connState == CONNECTED); }
   // is recording in progress using the Eyelink tracker?
   BOOL IsRecording() { return(IsConnected() && (m_bRecord || m_bRecording)); }
   // begin recording on Eyelink tracker and stream data to runtime engine
   BOOL StartRecord();
   // stop recording on Eyelink tracker and return it to idle/offline mode
   BOOL StopRecord();

private:
   // clear/reset state of Eyelink interface prior to connecting or after disconnecting
   VOID ResetState(); 

   // static entry-point invokes non-static thread procedure
   static UINT WorkerEntry(PVOID pThisObj) {  return( ((CCxEyeLink*)pThisObj)->Worker() ); }
   // the thread procedure for the EyeLink service thread
   UINT Worker(); 

   // helper methods for the EyeLink service thread (should be invoked only on that thread)
   BOOL Worker_StartRecord();
   int Worker_GetNextSample(BOOL isFirst);
   VOID Worker_StopRecord();
};


#endif   // !defined(CXEYELINK_H__INCLUDED_)
