/**=====================================================================================================================
 ni6509tester.cpp : Implementation of class CNI6509Tester, a simple application that can tests using the CNI6509, a
 Maestro device object representing the National Instruments' PCIe-6509 96-pin static DIO card.

 AUTHOR:  saruffner

 DESCRIPTION:
 This is a Win32 console application used to test/debug the implementation of static digital output functionality on the
 PCIe-6509 via the CNI6509 device object.

 The CNI6509 and the PCIe-6509 card implement an alternative to the original "Plexon interface module" in Maestro's
 external digital IO (DIO) rack. That module is a so-called "latched device" that receives commands from Maestro via
 the 16-line DO port driven by the CCxEventTimer device. Unfortunately, the opto-isolator chips in that module have
 started to fail, and it is no longer possible to replace them. In addition, the lab has replaced its Plexon systems
 with the more powerful Omniplex, and the Omniplex's superior DIO subsystem makes it possible to reimplement the
 Plexon interface module with a simple software-timed DIO device like the PCIe-6509. For more info, see CNI6509.CPP.

 CNI6509Tester is a simple Win32 console application that can run one of two tests using the CNI6509 device object. It
 is intended to provide a means of testing the device operation.
  
 Usage: "rtssrun ni6509tester N", where N can be parsed as an integer identifying which test to perform. If that argument
 is missing or invalid, then no test is performed. Results are reported directly on the console.
 
 Defined tests:
 Before conducting the specified test, the program must first find a PCIe-6509 board (RTX-owned) in the system, and open 
 the CNI6509 device object to connect to it. This will setup the memory-mapped interface to the device registers, verify
 that software can read and write registers, and configure the digital IO lines it will use. If the device is "opened"
 successfully, the specified test is performed.
  
 1 : Loopback DIO. For this test, 8-bit ports 0 and 1 on the PCIe-6509 must be connected to ports 2 and 3, respectively.
 The test configures ports 0 and 1 as digital outputs and ports 2 and 3 as digital inputs. Initially, all outputs are 0.
 Then each of the 8 digital outputs in port 0 is tested sequentially: line N is set, then the corresponding input line
 on port 2 is read to verify that it is set (and the other 7 bits are low!). The same sequence is repeated for the 8
 output lines of port 1 and corresponding input lines on port 3.

 2 : Omniplex integration test. For this test, the PCIe-6509 should be connected to the Omniplex system via the 
 06-24-A-09 adapter cable, as described in CNI6509.CPP. This test essentially emulates the information transmitted from
 Maestro to Omniplex before, during and after a single "trial":
    a) The Maestro "start trial" character code 0x02, followed by two null-terminated strings: the fake trial name
       "integration_test" and the fake data file name "filename.0001".
    b) The RECORDMARKER pulse on DO11, marking the start of the "trial".
    c) A series of 8 marker pulses are delivered on DO<1>, then DO<2>, ... then DO<8>, with ~20us between each pulse.
       These should be timestamped by the Omniplex as TTL events "Event 3" through "Event 10". Finally, a marker pulse
       is delivered on DO<10>, corresponding to the Omniplex's TTL event "Event 1" ("XS1" on the Plexon).
    d) Step (c) is repeated 2 more times.
    e) The RECORDMARKER pulse on DO11, marking the end of the "trial".
    f) After 10ms, the Maestro "stop trial" character code 0x03.
 When performing this test, the Omniplex should be actively recording. The Omniplex PL2 file can then be analyzed to
 verify that all of the character data and marker pulses were successfully timestamped by the Omniplex in the order
 expected.

 CREDITS:
 1) Real-Time eXtension (RTX) to Windows by IntervalZero (www.intervalzero.com). This testing app is designed to 
 run as an "RTSS" process within the RTX subsystem. RTX gives Windows real-time-like characteristics and provides kernel
 access for direct communications with hardware devices -- obviating the need to write kernel-mode device drivers.


 REVISION HISTORY:
 01jun2021-- Began development.
======================================================================================================================*/

#include <windows.h>                   // standard Win32 includes
#include <stdio.h>                     // runtime C/C++ I/O library
#include <stdlib.h>                    // for rand()
#include <math.h>                      // runtime C/C++ math library
#include "rtapi.h"                     // the RTX API
#include "util.h"                      // for the CElapsedTime utility 
#include "ni6509tester.h"


// The global "application" object
CNI6509Tester theApp; 

/**
 main(): This is an extremely simple entry point for the app. It merely wraps a call to CNI6509Tester::Go(), which 
 represents the primary thread of the application.
 
 @param argc The number of arguments. All arguments other than the first are ignored.
 @param argv The first element is the executable name; the remaining elements are command-line arguments.
*/
void main(int argc, char* argv[]) 
{ 
   theApp.Go(argc, argv); 
}


/** Construct the CNI6509Tester application object and initialize it to the non-running state. */
CNI6509Tester::CNI6509Tester()
{
   m_which = -1;
}

/** Destructor. Nothing to do here because application object state is created and freed in Run()... */
CNI6509Tester::~CNI6509Tester()
{
}


/**
 Go(): This method represents the primary thread of execution. It performs the following startup tasks:

 (1) Spawns the "worker" thread that runs the actual tests on the PCIe-6509. See CNI6509Tester::Run().
 (2) Spawns a suspension management thread (CRtSuspendMgr) that periodically suspends the worker thread so that it does
 not starve Windows. With this scheme, we no longer need to disperse RtSleepFt() statements throughout the code to
 achieve starvation management. Obviously, the suspender thread must have a higher RT priority than the thread it
 manages.
 (3) Once it starts the worker thread, the primary thread then waits indefinitely for that thread to complete, at which
 point it performs some clean up and exits.

 NOTE: An RTX thread cannot use suspend management on ITSELF because the RTX API does not allow it to obtain a copy of
 a "real" handle (vs the pseudohandle returned by GetCurrentThread()) to itself. Hence, the primary thread spawns a 
 "worker" thread to do all the work, and passes the worker's thread handle (from CreateThread()) to the suspend
 manager, which is itself a secondary thread (an RTX timer thread).
 
 @param nargs The number of command-line argument tokens. Note that first token is the executable name.
 @param args The argument tokens, an array of null-terminated strings: "ni6509tester.rtss N". Note that the first
 token is the executable name. There must be at least one other token, identifying the test to perform. If this is not
 a valid test number, then the program will still try to find and initialize the PCIe-6509, but it will not perform a 
 test. Any additional argument tokens are ignored.
*/
VOID RTFCNDCL CNI6509Tester::Go(int nargs, char* args[])
{
   // parse the command line
   int iArg = -1;
   if((nargs > 1) && (1 == ::sscanf_s(args[1], "%d", &iArg))) m_which = iArg;

   // the runtime engine worker
   HANDLE hWorker = (HANDLE)NULL;

   // error message in case a problem occurs while starting up
   char szErrMsg[256];
   ::strcpy_s(szErrMsg, ""); 
   BOOL bOk = TRUE;

   // create -- in a suspended state -- the runtime worker thread that will conduct the tests. Abort on failure.
   DWORD dwID;
   hWorker = ::CreateThread(NULL, 0, CNI6509Tester::RunEntry, (LPVOID)this, CREATE_SUSPENDED, &dwID);
   if(hWorker == (HANDLE) NULL)
   {
      ::sprintf_s(szErrMsg, "Cannot start worker thread (0x%08x)", ::GetLastError());
      bOk = FALSE;
   }

   // set this thread's priority higher than the worker's, then start work under suspend management. The suspend mgr
   // gets the highest priority.
   if(bOk)
   {
      ::RtSetThreadPriority( ::GetCurrentThread(), RT_PRIORITY_MAX-5 ); 
      ::RtSetThreadPriority( hWorker, RT_PRIORITY_MAX - 10 ); 

      // start worker under suspend management. Suspend mgr gets even higher priority.
      if(!m_suspendMgr.Start(hWorker, RT_PRIORITY_MAX-1))
      {
         ::sprintf_s(szErrMsg, "Suspend manager thread failed (0x%08x)", ::GetLastError());
         bOk = FALSE;
      }
   }

   // if startup was successful, the startup thread simply waits until the worker thread dies!
   if(bOk) ::RtWaitForSingleObject(hWorker, INFINITE);

   // clean up (don't use RtCloseHandle() on a thread handle
   if(hWorker != (HANDLE) NULL) ::CloseHandle(hWorker); 

   // kill suspend management AFTER worker thread completed
   m_suspendMgr.Stop();

   // if startup failed, print error message to console
   if(strlen(szErrMsg) > 0) printf("Startup failed:\n   %s\n", szErrMsg);
   else printf("Exiting...\n");

   ExitProcess(0);
}


/**
 Run(), RunEntry(): Entry point for worker thread procedure that conducts all the tests on the PCIe-6509.
 
 HACK: The thread entry point must be a static method, and static class methods do not get the implied THIS argument.
 Thus, Run() would not be able to access the non-static class members. To get around this, the static inline method 
 RunEntry() instead serves as the thread entry point. When spawned in Go(), it is passed a THIS ptr in its context 
 argument, which it then can use to invoke the non-static Run() method!
 
 @return Exit code (0 always -- not used).
*/
DWORD RTFCNDCL CNI6509Tester::Run()
{
   BOOL bOk = TRUE;

   // suspend management: 1ms time slice, 20% suspended  -- emulates the scenario when Maestro is running a trial
   m_suspendMgr.ChangeTiming(1000, 200);
   
   // open the device
   m_pNI6509 = new CNI6509(1);
   if(m_pNI6509 == NULL) 
   {
      ::printf("ERROR: Unable to construct the NI-6509 device object\n");
      bOk = FALSE;
   }
   else
   {
      bOk = m_pNI6509->Open();
      if(bOk) ::printf("%s installed and initialized.\n", m_pNI6509->GetDeviceName());
      else ::printf("ERROR: %s\n", m_pNI6509->GetLastDeviceError());
   }

   
   // perform the requested test 
   if(bOk) switch(m_which)
   {
      case 1 : DoLoopbackTest(); break;
      case 2 : DoIntegrationTest(); break;
      default : ::printf("Invalid test number.\n"); break;
   }
   
   // close the device and destroy the device object
   if(m_pNI6509 != NULL)
   {
      m_pNI6509->Close();
      delete m_pNI6509;
      m_pNI6509 = NULL;
   }
   
   ::printf("...BYE!\n");

   return(0);
}

VOID RTFCNDCL CNI6363Tester::DoLoopbackTest()
{
   m_pNI6509->RunLoopbackTest()
}

VOID RTFCNDCL CNI6363Tester::DoIntegrationTest()
{
   ::printf("\nOmniplex integration test:\n");
   ::printf("   Assumes that PCIe-6509 is connected to the Omniplex via Plexon Map Mode 2 Adapter Cable.\n"); 
   ::printf("   Also assumes that Omniplex recording is on as it would be for a Maestro trial.\n");
   ::printf("\n   Starting simulated Maestro trial...");

   // TODO: IMPLEMENT
   // "start trial" character code, followed by fake trial name and data file name
   m_pNI6509->WriteChar((char)0x02)

   // RECORDMARKER pulse marks "start" of trial
   DWORD dwRecordMarkerMask = ((DWORD) (1<<11))
   m_pNI6509->TriggerMarkers(dwRecordMarkerMask)

   // during fake trial timeline, deliver a pulse every 20ms sequentially on DO<1..8> and DO<10>. Repeat the pulse series
   // three times with a 500ms delay after each series
   CElapsedTime eTime;
   volatile long count = 0;

   DWORD dwPulse = 0
   for(int i=0; i<3; i++)
   {
      for(int j=1; j<=8; j++)
      {
         dwPulse = (DWORD) (1<<j)
         m_pNI6509->TriggerMarkers(dwPulse)

         // 20ms interval between pulses
         eTime.Reset()
         while(eTime.Get() < 20000.0) ++count;
      }

      dwPulse = (DWORD) (1<<10)
      m_pNI6509->TriggerMarkers(dwPulse)

      // 500ms delay after each series of pulses
      eTime.Reset()
      while(eTime.Get() < 500000.0) ++count;
   }

   // RECORDMARKER pulse delivered immediately after "trial" ends
   m_pNI6509->TriggerMarkers(dwRecordMarkerMask)

   // wait 10ms, then transmit "stop trial" character code
   eTime.Reset()
   while(eTime.Get() < 10000.0) ++count;

   ::printf(" DONE. Analyze Omniplex recording to verify signals received.\n");
}
