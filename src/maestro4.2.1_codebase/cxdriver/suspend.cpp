//===================================================================================================================== 
//
// suspend.cpp : Implementation of class CRtSuspendMgr, a thread-suspension manager for RTSS processes.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CRtSuspendMgr manages the periodic suspension of a single thread's execution.  It is intended for use by an RTX 
// primary [main()] thread to manage a CPU-hogging worker thread, which can itself use the CRtSuspendMgr object to 
// dynamically change the "suspended" and "running" phases of the suspend duty cycle as needed -- and even bypass 
// suspension management entirely to execute particularly time-critical code sections.
//
// The purpose of thread suspend management is to force an RTX thread to yield the CPU on a regular basis.  Without 
// it, a CPU-hogging thread could monopolize the CPU, preventing other RTSS and WinNT threads from running.  As a 
// result, your application could freeze the system altogether.  CRtSuspendMgr makes it easy to do thread suspend 
// management:  Rather than having to disperse RtSleepFt() calls strategically through the thread's implementation 
// code -- a solution which can be difficult to maintain, adjust, and debug -- one merely creates a CRtSuspendMgr 
// object for the thread to be managed and specifies the lengths of the ON and OFF (ie, running vs suspended) phases 
// of the suspend cycle.
//
// ==> Usage.
// Instantiate a CRtSuspendMgr object and call Start() with the handle of the thread to be managed and the desired 
// RTX priority of the suspend manager's timer thread.  It is important to set this priority higher than that of the 
// managed thread or any other CPU-hogging threads in the RTSS process.  The initial duty cycle is set to 20ms, 50% 
// suspended.
// 
// Whenever it is necessary to adjust the suspend timing parameters, call ChangeTiming(). Previous timing parameters
// are returned so that you can easily restore them with a subsequent call to this method. To temporarily turn off 
// suspend management during execution of particularly time-critical code, bracket the relevant code with calls to 
// Bypass(FALSE) and Resume(). Bypass suspend management with care; one or more CPU-hogging threads in your RTSS 
// process could starve Windows, freezing the GUI and possibly leading to termination of your process by the RTX 
// "watchdog timer" (if it is enabled on the system). 
// 
// If you need to temporarily halt the managed thread, it is NOT sufficient to call ::SuspendThread() on the thread's 
// handle. Since the suspend manager has its own private copy of that handle, it will reawaken the thread from its 
// suspended state. Instead, call Bypass(TRUE) to temporarily stop suspend management with the managed thread in the 
// suspended state; again invoke Resume() restore normal operation.
//
// To stop and reset the suspend manager, invoke the Stop() method. Also note that a call to ChangeTiming() will stop 
// the suspend manager if it detects that the managed thread has terminated, or its thread handle is no longer valid. 
// It does so by calling the RTX-supported Win32 function ::GetExitCodeThread().
//
// ==> Limitations.
//    (1) Only manages a single RTX (see CREDITS) thread; if there are other CPU-hogging RTX threads running, you must 
// instantiate a separate suspend manager for each. However, keep in mind that each suspend manager object adds 
// overhead to the system -- in the form of an RTX timer thread that suspends/resumes the managed thread.
//    (2) Keeps a private module copy of the handle of the managed thread, which is passed *by value* in Start().  If 
// the handle is later invalidated outside this module, the managed thread will no longer be periodically suspended. 
//    (3) Currently a thread cannot use this module to manage itself because RTX does not provide a means for the 
// thread to obtain a copy of a "real" handle to itself.  The "pseudohandle" obtained from the RTX implementation of 
// Win32's ::GetCurrentThread() does not work in the calls to ::SuspendThread() and ::ResumeThread()!!  [One way to 
// bypass this problem might be to have the parent thread save the handle of a child thread that is to be managed.  The 
// child thread can then use this handle to start suspend management on itself.  I have NOT tested whether such a trick 
// will work, however.  Still, there's no way for an RTSS process to perform suspend mgt on the primary thread.]
//    (4) A practical limitation on the granularity of the ON/OFF phases of the suspend duty cycle is the so-called RTX
// HAL Timer Period (see CREDITS). The HAL timer period for the host PC is set using the RTX Settings Control Panel 
// (followed by a reboot); it cannot be manipulated by this API. For the finest granularity, set this to its minimum 
// possible value. However, IntervalZero warns against setting the HAL timer period too low, as this can affect
// performance. Suspend timing parameters less than the current min timer period will be rejected. 
//    (5) This suspend management scheme will be subverted if the managed thread's RTX priority is set higher than 
// that of the timer thread created by the CRtSuspendMgr object. The timer thread's priority is assigned in Start(). 
//    (6) Timing parameters must be converted from microseconds to # of RTX clock ticks, where an RTX clock tick 
// is nominally 100 nanosecs. According to RTX documentation, the nominal 100us RTX HAL timer period is actually 
// 99.733346 microsecs, so that the nominal 100ns "tick" is really 99.733346ns, or 10.0267366945 ticks per microsec.  
// This last number is the constant DEF_US_TO_RTXTICKS that is used for converting times to clock ticks.  Note that 
// I have not tested the accuracy of the RTX documentation's claim, and it may be that the nominal timer period will 
// vary from system to system.
// ***NOTE: RTX2011 documentation indicates that this is no longer an issue on multiprocessor systems and single-proc
// ***systems that use the newer APIC timers. Therefore, as of 27jan2011, I no longer use US_TO_RTXTICKS.
//    (8) DO NOT call ::SuspendThread() or ::ResumeThread() on a thread managed by CRtSuspendMgr, or behavior is 
// undefined.
//
//
// CREDITS:
// 1) Real-Time eXtenstion (RTX) to WinNT by VenturCom, Inc (www.vci.com).  RTX gives the WinNT OS real-time-like 
// characteristics and provides kernel access for direct communications with hardware devices.  RTX timer objects are 
// used in CRtSuspendMgr's implementation, so applications using it can only run on WinNT systems extended by RTX.
// 2) Sample source code file "starvation.c", Copyright (c) 1997-1999 VenturCom, Inc. (www.vci.com); author Myron 
// Zimmerman, 28aug1997.  The original C source code module SUSPEND (written for an earlier version of CXDRIVER) was 
// adapted from this sample source code.
//
//
// REVISION HISTORY:
// 14may2001-- Adapted from C source code module SUSPEND from an older version of CXDRIVER.
// 09aug2002-- Modified Bypass() so that managed thread may be either halted or allowed to run while suspend mgr is 
//             bypassed.
// 30sep2002-- Fixed bug in bypass feature:  if Bypass(TRUE) was called while the timer handler thread was sleeping 
//             during the "suspended" phase of the duty cycle, the handler (which is not terminated by RtCancelTimer()) 
//             will eventually wake up and resume the managed thread -- NOT what we want. 
// 01oct2002-- Redesigned.  Instead of sleeping in the timer thread, we break the duty cycle into a "running" phase and 
//             a "not running" phase.  A one-shot timer is used to time each phase.  When the timer handler is called 
//             and the managed thread is running, the handler suspends the thread and rearms the timer with the suspend 
//             phase duration; when the handler is called with the managed thread suspended, the handler resumes the 
//             thread and rearms the timer with the run phase duration.  The goal here is to keep execution time w/in 
//             the timer handler thread to a minimum, avoiding sneaky bugs like the one described on 30sep2002.
// 27jan2012-- RTX2011 documentation indicates that the HAL extension timer is now based on the newer Local APIC timer
// in multiprocessor systems and single-proc APIC systems, and that the timer period is exact. Therefore, we no longer
// use a constant to convert a nominal period into the required # of HAL timer ticks (100-ns clock ticks). To convert
// from microseconds to 100-ns ticks, we simply multiply by 10.
//          -- Redesigned so that timing parameters are specified as the duration of the ON (active) and OFF (suspended)
// phases in microseconds, rather than the cycle period in ms and suspended phase in us. This change was needed so
// that we can set duty cycles that are not an integral number of milliseconds in length.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10 / RTX64 build.
//===================================================================================================================== 

#include "suspend.h"



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

/**
 Start managing the periodic suspension of the specified thread. Here we attempt to validate the thread handle, create 
 the necessary RTX timer object, and initiate suspend management. Initially thread is suspended 10ms of every 20. Call 
 ChangeTiming() to adjust. 

 @param hThread Handle of thread to manage; must be a real handle, not the pseudohandle returned ::GetCurrentThread().
 @param ulRTXPri RTX priority to be assigned to timer thread created here; must be greater than the RTX priority 
 assigned to the managed thread. 
 @returns TRUE if successful; FALSE otherwise (in which case last error code is set)
*/
BOOL RTFCNDCL CRtSuspendMgr::Start(HANDLE hThread, const ULONG ulRTXPri)
{
   if( m_hManagedThrd != (HANDLE) NULL )                                   // cannot start if already in use!
   {
      ::SetLastError( (DWORD)ERRNO_SUSPINUSE );
      return( FALSE );
   }

   if( !IsThreadAlive( hThread ) )                                         // thread must be running!  
   {
      ::SetLastError( (DWORD)ERRNO_INVALIDTHREAD );
      return( FALSE );
   }

   INT thrdPri = ::RtGetThreadPriority( hThread );                         // check for invalid timer thread priority 
   if( (ulRTXPri < RT_PRIORITY_MIN) || (ulRTXPri > RT_PRIORITY_MAX) ||
       (thrdPri == THREAD_PRIORITY_ERROR_RETURN) ||
       (thrdPri >= (INT)ulRTXPri) )
   {
      ::SetLastError( (DWORD)ERRNO_BADPARAMS );
      return( FALSE );
   }

   m_hTimer = ::RtCreateTimer( NULL, 0, CRtSuspendMgr::Timer,              // try to create the suspend interval timer 
                               (PVOID)this, ulRTXPri, CLOCK_FASTEST );
   if( m_hTimer == (HANDLE) NULL )
   {
      ::SetLastError( ERRNO_CREATETIMER );
      return( FALSE );
   }

   m_hManagedThrd = hThread;                                               // save thread handle for suspending later! 
   m_iOnUS = 10000;                                                        // initially: 10 ms on, 10 ms off
   m_iOffUS = 10000; 
   m_bBypassed = FALSE;
   m_bSuspended = FALSE;

   ConvertToTicks();                                                       // convert timing params to RTX clock ticks 

   m_bOn = TRUE;                                                           // resume mgd thread and start suspend mgr 
   ::ResumeThread( m_hManagedThrd );                                       // in the "on" phase of duty cycle
   ::RtSetTimerRelative( m_hTimer, &m_i64OnTicks, NULL ); 
   return( TRUE );
}

/**
 Stop and reset the suspend manager. The RTX timer object that was created to countdown the two phases of the suspend 
 duty cycle is freed here.
*/
VOID RTFCNDCL CRtSuspendMgr::Stop()
{
   if( m_hManagedThrd != (HANDLE) NULL )           // if the suspend manager is active...
   {
      ::RtDeleteTimer( m_hTimer );                 //    cancel and free the timer resource
      ::ResumeThread( m_hManagedThrd );            //    in case we call method while suspending the managed thrd!! 

      Init();                                      //    reinitialize to the inactive state
   }
}


/**
 Change the durations of the "on" (thread active) and "off" (thread suspended) phases of the current suspend duty cycle.
 If the new timing parameters do not satisfy certain constraints, the call will fail.

 NOTE1: If ChangeTiming() is called successfully while the suspend manager is bypassed, thread suspension will be 
 restarted using the new timing parameters. However, if the ChangeTiming() call fails, the suspend manager remains in 
 its bypassed state with the old timing params still in effect.
 NOTE2: If ChangeTiming() detects that the managed thread has died or the internally stored handle is no longer valid, 
 it will automatically reset the suspend manager and return failure.

 @param iOnDur The new duration for the "on" phase, in microseconds. Must be a multiple of 100 us.
 @param iOffDur The new duration for the "off" phase, in microseconds. Must be a multiple of 100 us.
 @param piOldOnDur [out] If not NULL, the previous "on" phase duration is stored here, in microseconds. 
 @param piOldOffDur [out] If not NULL, the previous "off" phase duration is stored here, in microseconds. 
 @return TRUE if successful; FALSE otherwise (error code set). 
*/
BOOL RTFCNDCL CRtSuspendMgr::ChangeTiming(
   const int iOnDur, const int iOffDur, int* piOldOnDur /* =NULL */, int* piOldOffDur /*= NULL */)
{
   // report old timing parameters
   if(piOldOnDur != NULL) *piOldOnDur = m_iOnUS; 
   if(piOldOffDur != NULL) *piOldOffDur = m_iOffUS;

   // suspend manager not in use!
   if(m_hManagedThrd == (HANDLE) NULL)
   {
      ::SetLastError((DWORD)ERRNO_SUSPNOTINUSE);
      return(FALSE);
   }

   // if managed thread has died, or thread handle is invalid, reset the suspend mgr
   if(!IsThreadAlive(m_hManagedThrd))
   {
      Stop();
      ::SetLastError((DWORD)ERRNO_INVALIDTHREAD);
      return(FALSE);
   }

   // bypass suspend management while we change the timings. Remember bypassed status so we can restore it.
   BOOL bWasBypassed = m_bBypassed; 
   m_bBypassed = TRUE;
   ::RtCancelTimer(m_hTimer, NULL);
   
   // save old timing params in case we have to restore them b/c new timing params are bad
   int iSaveOn = m_iOnUS;
   int iSaveOff = m_iOffUS;

   // convert new timing parameters to RTX tick counts and validate; revert to old values if unacceptable. A
   // succesful change in timing parameters always resets the bypass feature!
   m_iOnUS = iOnDur;  
   m_iOffUS = iOffDur;
   BOOL bOk = TRUE; 
   if( !ConvertToTicks() ) 
   {
      m_iOnUS = iSaveOn;
      m_iOffUS = iSaveOff;
      ConvertToTicks();
      bOk = FALSE;
      m_bBypassed = bWasBypassed;
   }
   else m_bBypassed = FALSE; 

   // if not bypassed, resume suspend mgt of thread using the new (or, perhaps, old) timing params
   if(!m_bBypassed) 
   {
      m_bOn = TRUE;
      ::ResumeThread(m_hManagedThrd);
      ::RtSetTimerRelative(m_hTimer, &m_i64OnTicks, NULL);
   }

   if(!bOk) ::SetLastError((DWORD)ERRNO_BADPARAMS);
   return(bOk);
}

/**
 Bypass suspend management of the currently managed thread. Suspend duty cycle timing parameters are unaffected. 

 @param bSuspend If TRUE, managed thread is suspended while suspend mgr is bypassed; else, it is allowed to run.
 @return TRUE if successful, FALSE otherwise (error code set). 
*/
BOOL RTFCNDCL CRtSuspendMgr::Bypass(BOOL bSuspend /* =FALSE */)
{
   // suspend manager not in use!
   if(m_hManagedThrd == (HANDLE) NULL)
   {
      ::SetLastError((DWORD)ERRNO_SUSPNOTINUSE);
      return(FALSE);
   }

   // if managed thread has died, or thread handle is invalid, reset the suspend mgr
   if(!IsThreadAlive(m_hManagedThrd))
   {
      Stop();
      ::SetLastError((DWORD)ERRNO_INVALIDTHREAD);
      return(FALSE);
   }

   // we're already bypassed! 
   if(m_bBypassed) return(TRUE);


   m_bSuspended = bSuspend;                              // do we suspend managed thread while mgr is bypassed?
   m_bBypassed = TRUE;

   ::RtCancelTimer(m_hTimer, NULL);                      // cancel the suspend interval timer 

   if(m_bSuspended) ::SuspendThread(m_hManagedThrd);     // leave managed thread in a suspended state
   else ::ResumeThread(m_hManagedThrd);                  // or let it run while suspend mgr is bypassed

   return(TRUE);
}

/**
 Resume suspend management of the currently managed thread. Suspend duty cycle timing parameters are unaffected. 
 @return TRUE if successful, FALSE otherwise (error code set). 
*/
BOOL RTFCNDCL CRtSuspendMgr::Resume() 
{
   // suspend manager not in use!
   if(m_hManagedThrd == (HANDLE) NULL)
   {
      ::SetLastError((DWORD)ERRNO_SUSPNOTINUSE);
      return(FALSE);
   }

   // if managed thread has died, or thread handle is invalid, reset the suspend mgr
   if(!IsThreadAlive(m_hManagedThrd))
   {
      Stop();
      ::SetLastError((DWORD)ERRNO_INVALIDTHREAD);
      return(FALSE);
   }


   if(!m_bBypassed) return(TRUE);                        // we're NOT bypassed!  do nothing. 

   m_bBypassed = FALSE;
   m_bSuspended = FALSE;

   m_bOn = TRUE;                                         // restart suspend management in the "on" phase
   ::ResumeThread(m_hManagedThrd); 
   ::RtSetTimerRelative(m_hTimer, &m_i64OnTicks, NULL); 

   return(TRUE);
}

/**
 Retrieve the current suspend timing parameters. If the suspend manager is not in use, both "on" and "off"
 phases of the duty cycle zer zero.
 @param iOnDur [out] Current duration of suspend duty cycle's "on" phase in us (0 = suspend mgr not in use). 
 @param iOffDur [out] Current duration of duty cycle's "off" phase in us.
 @param bByp [out] TRUE iff suspend mgr is currently bypassed.
*/
VOID RTFCNDCL CRtSuspendMgr::GetTiming(int& iOnDur, int& iOffDur, BOOL& bByp)
{
   iOnDur = m_iOnUS;
   iOffDur = m_iOffUS;
   bByp = m_bBypassed;
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

/**
 Initialize suspend manager object in the "inactive" state (no thread managed, no timer resource allocated). 
*/
VOID RTFCNDCL CRtSuspendMgr::Init()
{
   m_iOnUS = 0;
   m_iOffUS = 0; 
   m_i64OnTicks.QuadPart = 0;
   m_i64OffTicks.QuadPart = 0;

   m_bOn = FALSE;
   m_bBypassed = FALSE;
   m_bSuspended = FALSE;

   m_hManagedThrd = (HANDLE) NULL;
   m_hTimer = (HANDLE) NULL;
}


/** 
 [RTX timer handler; static method] 
 Timer handler routine called at the end of each "on" and "off" phase of the suspend duty cycle. While the suspend 
 manager is bypassed, this handler does nothing. Note that all calls in this method are "deterministic"; max execution 
 time should be on the order of 10us or less (probably MUCH less).

 @param pThisObj Ptr to THIS (static methods do not get the implied arg THIS!!)
*/
VOID RTFCNDCL CRtSuspendMgr::Timer(PVOID pThisObj)
{
   CRtSuspendMgr* pMgr = (CRtSuspendMgr*)pThisObj;

   if(pMgr->m_bBypassed) return;

   if(pMgr->m_bOn)                                          // end of "on" (mgd thread running) phase: stop thread and 
   {                                                        // arm one-shot timer to countdown duration of "off" phase 
      pMgr->m_bOn = FALSE;
      ::SuspendThread(pMgr->m_hManagedThrd);
      ::RtSetTimerRelative(pMgr->m_hTimer, &(pMgr->m_i64OffTicks), NULL); 
   }
   else                                                     // end of "off" (mgd thrd suspended) phase: resume thrd and 
   {                                                        // arm one-shot timer to countdown duration of "on" phase
      pMgr->m_bOn = TRUE;
      ::ResumeThread(pMgr->m_hManagedThrd);
      ::RtSetTimerRelative(pMgr->m_hTimer, &(pMgr->m_i64OnTicks), NULL);
   }
}


/**
 Validate the current durations of the "on" and "off" phases of the suspend duty cycle timing parameters, then convert
 them to 100-ns clock ticks (x10).

 This method is called internally whenever the nominal timing parameters are modified. Each phase must be a integral
 multiple of 100us and each must be larger than the minimum RTX timer period currently available on the system.

 @return TRUE if current current timing parameters are OK; FALSE otherwise. 
*/
BOOL RTFCNDCL CRtSuspendMgr::ConvertToTicks()
{
   if(((m_iOnUS % 100) != 0) || ((m_iOffUS % 100) != 0)) return(FALSE);

   // convert us to 100-ns RTX clock ticks
   double dTicks = ((double) m_iOnUS) * 10.0; 
   m_i64OnTicks.QuadPart = (LONGLONG) dTicks;
   dTicks = ((double) m_iOffUS) * 10.0; 
   m_i64OffTicks.QuadPart = (LONGLONG) dTicks;

   // both phases must be greater than the minimum RTX timer period
   LARGE_INTEGER i64MinPer;
   ::RtGetClockTimerPeriod(CLOCK_FASTEST, &i64MinPer);
   if((m_i64OffTicks.QuadPart < i64MinPer.QuadPart) || (m_i64OnTicks.QuadPart < i64MinPer.QuadPart))
      return(FALSE);
   else
      return(TRUE);
}


/**
 Check to see if specified thread has exited. If unable to obtain exit code, we assume the provided thread handle is 
 no longer valid.

 @param hThread Handle of thread to be checked.
 @return FALSE if thread handle is invalid or thread exit code is not STILL_ACTIVE; else TRUE. 
*/
BOOL RTFCNDCL CRtSuspendMgr::IsThreadAlive(HANDLE hThread)
{
   DWORD dwExitCode = 0;
   BOOL bSuccess = ::GetExitCodeThread(hThread, &dwExitCode);
   return(bSuccess && (dwExitCode == STILL_ACTIVE));
}

