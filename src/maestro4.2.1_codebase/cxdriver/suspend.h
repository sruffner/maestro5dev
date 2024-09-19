//===================================================================================================================== 
//
// suspend.h : Declaration of CRtSuspendMgr, which uses an RTX timer object to periodically suspend a "managed" thread.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(SUSPEND_H__INCLUDED_)
#define SUSPEND_H__INCLUDED_

#include <windows.h>             // standard Win32 includes 
#include "rtapi.h"               // the RTX API 


//===================================================================================================================== 
// Declaration of class CRtSuspendMgr
//===================================================================================================================== 
//
class CRtSuspendMgr
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private: 
   static const DWORD ERRNO_BADPARAMS = 0x22000000;      // invalid or illegal method parameters
   static const DWORD ERRNO_SUSPINUSE = 0x22000001;      // suspend manager already in use; cannot start it!
   static const DWORD ERRNO_SUSPNOTINUSE = 0x22000002;   // suspend manager not in use; can't change suspend timing!
   static const DWORD ERRNO_INVALIDTHREAD = 0x22000003;  // managed thrd's handle invalid (suspend mgr stopped)
   static const DWORD ERRNO_CREATETIMER = 0x22000004;    // unable to create timer object required by suspend manager


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   int            m_iOnUS;                      // duration of "on" (thread active) phase of duty cycle, in microsecs 
   int            m_iOffUS;                     // duration of "off" (thread suspended) phase, in microsecs
   LARGE_INTEGER  m_i64OnTicks;                 // duration of "on" phase of duty cycle, in RTX "clock ticks"
   LARGE_INTEGER  m_i64OffTicks;                // duration of "off" phase of duty cycle, in RTX "clock ticks"

   BOOL           m_bOn;                        // if TRUE, we're in the "on" phase of duty cycle; else, "off" phase
   BOOL           m_bBypassed;                  // suspend manager temporarily bypassed 
   BOOL           m_bSuspended;                 // is managed thread suspended while suspend manager is bypassed?

   HANDLE         m_hManagedThrd;               // thread currently managed 
   HANDLE         m_hTimer;                     // RTX timer that counts down each phase of the suspend duty cycle

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CRtSuspendMgr( const CRtSuspendMgr& src );            // no copy constructor or assignment operator defined
   CRtSuspendMgr& operator=( const CRtSuspendMgr& src ); 

public: 
   CRtSuspendMgr() { Init(); }                           // constructor 
   ~CRtSuspendMgr() { Stop(); }                          // destructor (ensures timer resource is freed)


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL RTFCNDCL Start( HANDLE hThread,                  // start the suspend manager
                        const ULONG ulRTXPri );
   VOID RTFCNDCL Stop();                                 // stop the suspend manager (releasing timer resource)
   BOOL RTFCNDCL ChangeTiming(                           // change suspend manager's timing parameters
      const int iOnDur, const int iOffDur, 
      int* piOldOnDur = NULL, int* piOldOffDur = NULL);
   BOOL RTFCNDCL Bypass( BOOL bSuspend = FALSE );        // bypass suspend mgr, optionally suspending managed thread
   BOOL RTFCNDCL Resume();                               // resume suspend manager
   VOID RTFCNDCL GetTiming( int& iOnDur, int& iOffDur,   // get current suspend manager timing parameters 
                            BOOL& bByp );


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   VOID RTFCNDCL Init();                                 // initialize "inactive" state of suspend mgr
   static VOID RTFCNDCL Timer( PVOID pThisObj );         // timer handler routine for the suspend mgr's timer object 
   BOOL RTFCNDCL ConvertToTicks();                       // calc "on" and "off" phase durations in RTX clock ticks
   BOOL RTFCNDCL IsThreadAlive( HANDLE hThread );        // verify that specified thread is still active 
};


#endif   // !defined(SUSPEND_H__INCLUDED_)
