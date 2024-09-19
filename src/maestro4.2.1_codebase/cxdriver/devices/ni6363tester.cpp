/**=====================================================================================================================
 ni6363tester.cpp : Implementation of class CNI6363Tester, a simple application that can run any of several tests on 
 CNI6363, the Maestro device object representing the National Instruments' PCIe-6363.

 AUTHOR:  saruffner

 DESCRIPTION:
 This is a Win32 console application used to test/debug the implementation of Maestro analog input, analog output, DI 
 timestamping and static digital output functionality on the PCIe-6363 via the CNI6363 device object and its three
 sub-device objects.
 
 Usage: "rtssrun ni6363tester n [arg1 arg2 arg3]", where n can be parsed as an integer identifying which test to 
 perform. If tha argument is missing or invalid, then no test is performed. Results are reported directly on the 
 console. Additional but optional arguments may apply in select tests -- see test descriptions below.
 
 Defined tests:
 Before conducting the specified test, the program must first find a PCIe-6363 board (RTX-owned) in the system, and open 
 the CNI6363 device object to connect to it. This will setup the memory-mapped interface to the device registers, verify
 that software can read and write registers, and initialize each of the relevant subsystems on the board. If this
 "device acquisition" succeeds, the specified test is performed.
  
 1 : Static analog output. Output a sawtooth waveform with +/-10V amplitude, a period of roughly 1 second, and an
 update rate of 10 ms. The sequence is software-timed, so do not expect it to be accurate. The waveform is output for 
 10 seconds on each of the four AO channels in turn, while the other 3 channels should read 0V. Connect an oscilloscope
 to each channel in turn to verify operation.
 
 2 : Analog input/output loopback test. Connect A0..A3 to AI0..AI3, respectively. Delivers an identical sawtooth 
 waveform (+/-10V amplitude, 1-second period, update rate of 10ms) on each of 4 AO channels while sampling the 4 inputs
 every 2ms. Outputs are updated at t=0 in each 10ms epoch. Per-channel input samples are averaged at t=2, 4, 6 and 8ms 
 to  give the reading for each channel during that 10ms epoch. Reports, for each input, the min/max/avg difference
 between voltage driven and voltage measured over the course of a 10-second test. Analog data is software-polled; the
 "start-of-scan" AI interrupt is NOT enabled.
 
 3 : Static digital output. Toggles all 16 DO channels on at different intervals during a 1-minute testing period.
 DO channel N is toggled at every 10*(N+1)-ms epoch: DO0 every 10ms, DO1 every 20ms, .. , and DO15 every 160ms. Updates
 are software-timed. Use an oscilloscope to check each of the outputs during the test. Also check the output of PFI0,
 which is dedicated as the "Data Ready" signal. It is active low, so it's high all the time except when it's lowered
 VERY BRIEFLY immediately after the DO channels are updated. These active-low pulses should occur every ~10ms while
 the test is running.
 
 4 : Digital input/output loopback test. Connect DO0..15 to DI0..16, respectively. Test starts with all digital outputs
 low for 10 seconds, then pulses (lo->hi->lo) each one individually, then pulses all of them high and low at the same '
 time. Pulses are delivered every 100ms and are about 100us in duration -- all software-timed. Immediately after a pulse
 is delivered, the corresponding digital timestamp event is unloaded from the DIO event timer device. Test is considered
 successful if each timestamped event has the correct bit mask; the event times are not checked since we're using 
 software timing. Test duration is under 15 seconds. 
 
 5 : Performance tests. This executes any performance tests written for the three subdevices implemented on the 
 PCIe-6363. Currently, these include:
   a) AI read data FIFO speed estimate. A brief DAQ is performed to collect ~4000 samples. The samples are then 
 unloaded from the FIFO in a tight loop and the approximate time per sample is reported.
   b) AO update performance. For each of the 4 AO channels on the NI-6363, this test estimates the average execution
 time of a write to the corresponding Direct_Data register for that channel, and the average execution time of a single
 call to Out(), where the output voltage is expressed in floating-point form and must be converted to the corresponding
 DAC code using the calibration data obtained from device EEPROM.
   c) DIO event timestamping short-pulse performance. This test delivers a pair of pulses on DO0 as short and closely
 spaced as programmatically possible (by writing directly to the Static_DO register rather than calling SetDO()). It
 then checks to see if both events were detected. The test is repeated 10000 times and stats are reported to console.
 
 6 : Continuous-running test. This test can exercise all three "subdevices" implemented on the PCIe-6363: analog input,
 analog output, and the DIO event timestamper. It takes three additional but optional arguments: 
    arg1: "min" - An integer in [1..60] specifying the number of minutes the test should run. =1 if missing or invalid.
    arg2, arg3: "enaFast", "enaEvts" - Integer flags; nonzero = true. Assume false if missing or invalid.
 The test runs continuously for the specified number of minutes, sampling all 16 AI channels at 1KHz and driving AO0 
 with a +/-5V sinuosoidal waveform with a 1-second period. The "start-of-scan" AI interrupt is enabled, and the runtime 
 loop is structured very much like MaestroRTSS's runtime loop during trial or continuous mode. Analog data is 
 continuously unloaded and discarded using programmed I/O, exactly as in MaestroRTSS. A progress message is printed to 
 the console approximately once per minute. 
    If enaFast is set, AI15 is also sampled at 25KHz throughout ("high-resolution spike waveform" channel in Maestro);
 again, the data is discarded.
    If enaEvts is set, the DI event timestamper functionality is tested. At 500ms into the test, the timestamper is
 enabled and the runtime loop begins delivering a marker pulse on DO0 on occasion (randomly, about 20% of the time). 
 DI timestamp data are continously unloaded and discarded, although the test keeps track of the total # of DO0 pulses
 delivered and the total # of DI events detected. These are reported at test's end and should be identical, assuming
 you've connected DO0 to DI0.

 7 : Countdown test. This test configures the G2 counter as a countdown timer to implement a busy wait. Reports whether 
 or not test was successful, and the actual elapsed time as measured using RtGetClockTime(). It takes one additional 
 but optional argument:
    arg1: "wait" - An integer in [1..1000000] specifying the desired wait time in microseconds. Default is 10. 


 CREDITS:
 1) Real-Time eXtension (RTX) to Windows by IntervalZero (www.intervalzero.com). This testing app is designed to 
 run as an "RTSS" process within the RTX subsystem. RTX gives Windows real-time-like characteristics and provides kernel
 access for direct communications with hardware devices -- obviating the need to write kernel-mode device drivers.


 REVISION HISTORY:
 25jul2011-- Began development.
 15sep2011-- Update: CNI6363 module is now passing most tests developed to date. Unloading the AI data FIFO via
 programmed IO is too slow right now -- working on improvements. Also need to thoroughly test all 16 DIs for the
 timestamping feature, and develop a test to verify accuracy of timestamps...
 20sep2011-- The problem with unloading the AI data FIFO was that we had to unload too many samples when the 25KHz
 channel was enabled. Turns out that the PCIe-6363 has the same ghost feature as the PCI-MIO16E1, it just was not
 documented in the X-Series MHDDK. Once I implemented that feature, the NI-6363 passed the continuous-running test
 with the 25KHz channel enabled. Still haven't done the DIO loopback test, because I lack a convenient interface for
 connecting DO0-15 to DI0-15. 
 16aug2019-- Added countdown test using G2 counter to assess whether I could use this mechanism to reliably implement
 a "busy wait" delay for a specified number of microseconds. Left the code in place, but don't plan to use it.
 19aug2019-- Found that, on occasion, after running ni6363test.rtss, the Win 10 machine would fail to shutdown. 
 Determined that this might be because the worker thread for this process failed to exit properly and I did not detect
 the situation. With an RTSS thread still hanging around, the RTX Subsystem prevents Windows from shutting down. In the
 process of addressing the problem, I found that it is now possible to wait for a thread to complete using the RTX
 function RtWaitForSingleObject(). This eliminates the need for creating and waiting on a mutex object that the worker
 thread would hold for its lifetime.
======================================================================================================================*/

#include <windows.h>                   // standard Win32 includes
#include <stdio.h>                     // runtime C/C++ I/O library
#include <stdlib.h>                    // for rand()
#include <math.h>                      // runtime C/C++ math library
#include "rtapi.h"                     // the RTX API
#include "ni6363tester.h"


// The global "application" object
CNI6363Tester theApp; 

/**
 main(): This is an extremely simple entry point for the app. It merely wraps a call to CNI6363Tester::Go(), which 
 represents the primary thread of the application. In addition, it checks the arguments to the program, which should be
 a single integer code identifying which test should be conducted. If no argument is provided, or if the argument cannot
 be parsed as an integer, or if its integer value is not a recognized test code, then the program will try to find and
 initialize the hardware device, but no test is performed. 
 
 @param argc The number of arguments. All arguments other than the first are ignored.
 @param argv The first element is the executable name; the remaining elements are command-line arguments.
*/
void main(int argc, char* argv[]) 
{ 
   int testCode = -1;
   if(argc > 1) ::sscanf_s(argv[1], "%d", &testCode);
   
   theApp.Go(argc, argv); 
}


/** Construct the CNI6363Tester application object and initialize it to the non-running state. */
CNI6363Tester::CNI6363Tester()
{
   m_which = -1;
   m_nMin = 1;
   m_enaFast = m_enaEvts = FALSE;
   m_tWaitUS = 10;

   m_vbInterruptPending = FALSE;
   m_viElapsedTicks = 0;
   m_viScanInterval = 0;
   m_viServicedTicks = 0;
   m_vbFrameLag = FALSE;
   m_vbDelayedISR = FALSE;
}

/** Destructor. Nothing to do here because application object state is created and freed in Run()... */
CNI6363Tester::~CNI6363Tester()
{
}


/**
 Go(): This method represents the primary thread of execution. It performs the following startup tasks:

 (1) Spawns the "worker" thread that runs the actual tests on the PCIe-6363. See CNI6363Tester::Run().
 (2) Spawns a suspension  management thread (CRtSuspendMgr) that periodically suspends the worker thread so that it 
 does not starve Windows. With this scheme, we no longer need to disperse RtSleepFt() statements throughout the code to
 achieve starvation management. Obviously, the suspender thread must have a higher RT priority than the thread it
 manages.
 (3) Once it starts the worker thread, the primary thread then waits indefinitely for that thread to complete, at which
 point it performs some clean up and exits.

 NOTE: An RTX thread cannot use suspend managment on ITSELF because the RTX API does not allow it to obtain a copy of a
 "real" handle (vs the pseudohandle returned by GetCurrentThread()) to itself. Hence, the primary thread spawns a 
 "worker" thread to do all the work, and passes the worker's thread handle (from CreateThread()) to the suspend
 manager, which is itself a secondary thread (an RTX timer thread).
 
 @param nargs The number of command-line argument tokens. Note that first token is the executable name.
 @param args The argument tokens, an array of null-terminated strings: "ni6363dev.rtss n [...]". Note that the first
 token is the executable name. There must be at least one other token, identifying the test to perform. If this is not
 a valid test number, then the program will still try to find and initialize the PCIe-6363, but it will not perform a 
 test. Additional arguments apply only for certain tests -- see class header.
*/
VOID RTFCNDCL CNI6363Tester::Go(int nargs, char* args[])
{
   // parse the command line
   int iArg = -1;
   if((nargs > 1) && (1 == ::sscanf_s(args[1], "%d", &iArg))) m_which = iArg;
   if((nargs > 2) && (1 == ::sscanf_s(args[2], "%d", &iArg)))
   {
      if(m_which == 6 && (iArg >= 1) && (iArg <= 60)) m_nMin = iArg;
      else if(m_which == 7) m_tWaitUS = (iArg < 1) ? 1 : ((iArg > 1000000) ? 1000000 : iArg);
   }
   if((nargs > 3) && (1 == ::sscanf_s(args[3], "%d", &iArg))) m_enaFast = BOOL(iArg != 0);
   if((nargs > 4) && (1 == ::sscanf_s(args[4], "%d", &iArg))) m_enaEvts = BOOL(iArg != 0);

   // the runtime engine worker
   HANDLE hWorker = (HANDLE)NULL;

   // error message in case a problem occurs while starting up
   char szErrMsg[256];
   ::strcpy_s(szErrMsg, ""); 
   BOOL bOk = TRUE;

   // create -- in a suspended state -- the runtime worker thread that will conduct the tests. Abort on failure.
   DWORD dwID;
   hWorker = ::CreateThread(NULL, 0, CNI6363Tester::RunEntry, (LPVOID)this, CREATE_SUSPENDED, &dwID);
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
 Run(), RunEntry(): Entry point for worker thread procedure that conducts all the tests on the PCIe-6363.
 
 HACK: The thread entry point must be a static method, and static class methods do not get the implied THIS argument.
 Thus, Run() would not be able to access the non-static class members. To get around this, the static inline method 
 RunEntry() instead serves as the thread entry point. When spawned in Go(), it is passed a THIS ptr in its context 
 argument, which it then can use to invoke the non-static Run() method!
 
 @return Exit code (0 always -- not used).
*/
DWORD RTFCNDCL CNI6363Tester::Run()
{
   BOOL bOk = TRUE;

   // suspend management: 1ms time slice, 20% suspended
   m_suspendMgr.ChangeTiming(1000, 200);
   
   // open the device
   m_pNI6363 = new CNI6363(1);
   if(m_pNI6363 == NULL) 
   {
      ::printf("ERROR: Unable to construct the NI-6363 device object\n");
      bOk = FALSE;
   }
   else
   {
      bOk = m_pNI6363->Open();
      if(bOk) ::printf("%s installed with AI, AO, and DIO event timer subdevices.\n", m_pNI6363->GetDeviceName());
      else ::printf("ERROR: %s\n", m_pNI6363->GetLastDeviceError());
   }

   // install the AI device ISR
   if(bOk)
   {
      bOk = m_pNI6363->GetAISubDevice()->SetInterruptHandler(CNI6363Tester::ServiceAI, (PVOID)this);
      if(!bOk) 
		  ::printf("ERROR: Failed to install AI device interrupt service routine!\n   %s\n", 
				m_pNI6363->GetAISubDevice()->GetLastDeviceError());
   }
   
   // perform the requested test 
   if(bOk) switch(m_which)
   {
      case 1 : DoStaticAOTest(); break;
      case 2 : DoAIOLoopbackTest(); break;
      case 3 : DoStaticDOTest(); break;
      case 4 : DoDIOLoopbackTest(); break;
      case 5 : DoPerformanceTests(); break;
      case 6 : DoContinuousRunTest(); break;
      case 7 : DoCountdownTest(); break;
      default : ::printf("Invalid test number.\n"); break;
   }
   
   // close the device and destroy the device object
   if(m_pNI6363 != NULL)
   {
      m_pNI6363->Close();
      delete m_pNI6363;
      m_pNI6363 = NULL;
   }
   
   ::printf("...BYE!\n");

   return(0);
}

VOID RTFCNDCL CNI6363Tester::DoStaticAOTest()
{
   ::printf("\nStatic analog output test:\n");
   CCxAnalogOut* pAO = m_pNI6363->GetAOSubDevice();
   if(!pAO->Init()) ::printf("   ERROR: AO initialization failed:\n     %s\n", pAO->GetLastDeviceError());
   else
   {
      ::printf("   Initialized. All outputs should read 0V. Waiting 10 seconds...\n");
      CElapsedTime eTime;
      while(eTime.Get() < 10e6) ;
      
      BOOL bOk = TRUE;
      for(int ch=0; bOk && ch<pAO->GetNumChannels(); ch++)
      {
         ::printf("   Presenting +/-10V sawtooth waveform on AO channel %d for 10 seconds...\n", ch);
         int nTicks = 0;
         float voltage = -10.0f;
         float incr = 0.2f;      // 20V range / 100 10-ms ticks per second
         while(nTicks <= 1000)
         {
            eTime.Reset();
            bOk = pAO->Out(ch, voltage);
            if(!bOk) break;
            voltage += incr;
            ++nTicks;
            if((nTicks % 100) == 0) voltage = -10.0f;
            
            // software timing of 10ms epochs
            while(eTime.Get() < 10000.0) ;
         }
         if(bOk) bOk = pAO->Out(ch, 0.0f);
      }
      
      if(bOk) ::printf("   TEST COMPLETED.\n");
      else ::printf("   ERROR: %s.\n", pAO->GetLastDeviceError());
      
      pAO->Init();
   }
}

VOID RTFCNDCL CNI6363Tester::DoAIOLoopbackTest()
{
   ::printf("\nSoftware-polled AIO loopback test:\n");
   ::printf("   Ensure that AO channels 0..3 are connected to AI channels 0..3!\n"); 
   
   CCxAnalogOut* pAO = m_pNI6363->GetAOSubDevice();
   CCxAnalogIn* pAI = m_pNI6363->GetAISubDevice();
   BOOL bOk = pAO->Init();
   if(!bOk) ::printf("   ERROR: AO initialization failed:\n      %s\n", pAO->GetLastDeviceError());
   if(bOk)
   {
      bOk = pAI->Init();
      if(!bOk) ::printf("   ERROR: AI initialization failed:\n      %s\n", pAI->GetLastDeviceError());
   }
   if(bOk)
   {
      bOk = pAI->Configure(4, 2000, -1, FALSE);
      if(!bOk) ::printf("   ERROR: AI configuration failed:\n      %s\n", pAI->GetLastDeviceError());
   }
   if(!bOk) return;
   
   ::printf("   Initialized. Starting sawtooth waveform on AO..3 while monitoring AI0..3 for 10 seconds...\n");

   short currScan[4];
   float accumulator[4];
   float minDelta[4];
   float maxDelta[4];
   float avgDelta[4];
   for(int i=0; i<4; i++) 
   {
      minDelta[i] = 1000.0f;
      maxDelta[i] = 0.0f;
      avgDelta[i] = 0.0f;
   }
   int nEpochs = 0;
   int nTicks = 0;
   float voltage = -10.0f;
   float incr = 0.2f;      // 20V range / 100 10-ms epochs per second

   pAI->Start();
   while(bOk && nEpochs < 1000)
   {
      // unload the channel samples for the current 2-ms AI scan, waiting if necessary.
      if(bOk)
      {
         int nSlow = 4;
         int nFast = 0;
         bOk = pAI->Unload(&(currScan[0]), nSlow, NULL, nFast, TRUE);
         if(!bOk) 
         {
            ::printf("   ERROR: AI.Unload() failed:\n      %s\n", pAI->GetLastDeviceError());
            break;
         }
      }
      
      // At t=0 of each epoch, update the sawtooth waveform on all outputs. Also zero the AI sample accumulators.
      if(nTicks == 0)
      {
         bOk = pAO->Out(-1, voltage);
         if(!bOk) 
         {
            ::printf("   ERROR: AO.Out() failed:\n      %s\n", pAO->GetLastDeviceError());
            break;
         }
         for(int i=0; i<4; i++) accumulator[i] = 0.0f;
      }
      
      // At t=2, 4, 6, and 8, accumulate the voltage samples on all 4 AI channels.
      if(nTicks > 0) for(int i=0; i<4; i++) accumulator[i] += pAI->ToVolts(currScan[i]);
      
      // Advance to the next 2-ms AI scan "tick". But at t=8, prepare for the next output epoch and reset tick count.
      if(nTicks < 4)
         ++nTicks;
      else
      {
         // for each AI channel, compute difference between driven output voltage and the averaged input voltage, and
         // accumulate stats on differential observed.
         for(int i=0; i<4; i++)
         {
            float delta = cMath::abs(voltage - accumulator[i]/4.0f);
            if(delta < minDelta[i]) minDelta[i] = delta;
            if(delta > maxDelta[i]) maxDelta[i] = delta;
            avgDelta[i] += delta;
         }
         
         // advance the output voltage of the sawtooth waveform
         voltage += incr;
         ++nEpochs;
         if((nEpochs % 100) == 0) voltage = -10.0f;
         nTicks = 0;
      }
   }
   
   pAI->Stop();
   pAI->Init();
   pAO->Init();
   
   // report results if test completed successfully
   if(bOk)
   {
      ::printf("   TEST COMPLETED.\n   Min/max/avg observed disparity between input and output voltage:\n");
      for(int i=0; i<4; i++) 
         ::printf("      Ch%d : %3.3f %3.3f %3.3f volts\n", i, minDelta[i], maxDelta[i], avgDelta[i]/nEpochs);
   }
}

VOID RTFCNDCL CNI6363Tester::DoStaticDOTest()
{
   ::printf("\nStatic digital output test:\n");
   CCxEventTimer* pDIO = m_pNI6363->GetEventTimerSubDevice();
   if(!pDIO->Init()) ::printf("   ERROR: DIO initialization failed:\n     %s\n", pDIO->GetLastDeviceError());
   else
   {
      ::printf("   Initialized. All digital outputs are low. Waiting 10 seconds...\n");
      CElapsedTime eTime;
      while(eTime.Get() < 10e6) ;
      
      ::printf("   Toggling outputs D0..15 for ~60 seconds...\n");
      
      int nTicks = 0;
      DWORD currOut = 0;
      while(nTicks <= 6000)
      {
		 eTime.Reset();
         // toggle the DO channels that should be toggled during this "tick". The larger the channel#, the less
         // frequently it's toggled.
         if(nTicks > 0) for(int i=0; i<16; i++)
         {
            if((nTicks % (i+1)) == 0) currOut ^= (DWORD) (1<<i);
         }
         pDIO->SetDO(currOut);
         
         // software timing of 10ms epochs
         while(eTime.Get() < 10000.0) ;
         
         ++nTicks;
      }
      
      pDIO->SetDO(0);
      ::printf("   TEST COMPLETED.\n");
   }
}

VOID RTFCNDCL CNI6363Tester::DoDIOLoopbackTest()
{
   ::printf("\nStatic digital output test:\n");
   CCxEventTimer* pDIO = m_pNI6363->GetEventTimerSubDevice();
   BOOL bOk = pDIO->Init();
   if(!bOk) ::printf("   ERROR: DIO initialization failed:\n     %s\n", pDIO->GetLastDeviceError());
   else
   {
      bOk = (10 == pDIO->Configure(10, 0x0000FFFF));
      if(!bOk) ::printf("   ERROR: DI timestamping configuration failed:\n      %s\n", pDIO->GetLastDeviceError());
   }
   if(!bOk) return;
   
   ::printf("   Initialized. All digital outputs are low. Waiting 10 seconds...\n");
   CElapsedTime eTime;
   while(eTime.Get() < 10e6) ;
   
   ::printf("   Running loopback test...\n");
   pDIO->Start();
   
   DWORD currOut = 0;
   DWORD currIn = 0;
   float tIn = 0.0f;
   bOk = FALSE;
   for(int i=0; i<= 16; i++)
   {
      // prepare the output vector for this iteration: pulse 1 channel at a time, or all at once
      if(i==16) currOut = 0x0000FFFF;
      else currOut = (DWORD) (1<<i);
      
      // deliver it
      pDIO->SetDO(currOut);
      eTime.Reset(); while(eTime.Get() < 100) ;
      pDIO->SetDO(0);
      
      // the device should already have clocked in the event. Unload it and verify.
      DWORD nRead = pDIO->UnloadEvents(1, &currIn, &tIn);
      if(nRead != 1)
      {
         LPCSTR emsg = pDIO->GetLastDeviceError();
         if(::strlen(emsg) > 0) ::printf("   ERROR on channel %d:\n      %s\n", i, emsg);
         else ::printf("   ERROR: Device failed to timestamp pulse on channel %d.\n", i);
         break;
      }
      else if(currIn != currOut)
      {
         ::printf("   ERROR: Timestamped input (%08d) != output (%08d)\n", currIn, currOut);
         break;
      }
      else if(i == 16) bOk = TRUE;
      
      // wait about 100ms before presenting the next pulse
      eTime.Reset(); while(eTime.Get() < 100e3) ;
   }
   
   pDIO->Stop();
   pDIO->Init();
   
   if(bOk) ::printf("   TEST COMPLETED - OK.\n");
}

VOID RTFCNDCL CNI6363Tester::DoPerformanceTests()
{
   ::printf("\nPerformance tests....\n");
   
   m_suspendMgr.Bypass();
   m_pNI6363->RunPerformanceTests();
   m_suspendMgr.Resume();
}

/*
 6 : Continuous-running test. This test can exercise all three "subdevices" implemented on the PCIe-6363: analog input,
 analog output, and the DIO event timestamper. It takes three additional but optional arguments: 
    min - An integer in [1..60] specifying the number of minutes the test should run. =1 if missing or invalid. 
    enaFast, enaEvts - Integer flags; nonzero = true. Assume false if missing or invalid.
 The test runs continuously for the specified number of minutes, sampling all 16 AI channels at 1KHz and driving AO0 
 with a +/-5V sinuosoidal waveform with a 1-second period. The "start-of-scan" AI interrupt is enabled, and the runtime 
 loop is structured very much like MaestroRTSS's runtime loop during a trial or continuous mode. Analog data is 
 continuously unloaded and discarded using programmed I/O, exactly as in MaestroRTSS. A progress message is printed to 
 the console approximately once per minute. 
    If enaFast is set, AI15 is also sampled at 25KHz throughout ("high-resolution spike waveform" channel in Maestro);
 again, the data is discarded.
    If enaEvts is set, the DI event timestamper functionality is tested. At 500ms into the test, the timestamper is
 enabled and the runtime loop begins delivering a marker pulse on DO0 on occasion (randomly, about 10% of the time). 
 DI timestamp data are continously unloaded and discarded, although the test keeps track of the total # of DO0 pulses
 delivered and the total # of DI events detected. These are reported at test's end and should be identical, assuming
 you've connected DO0 to DI0.
*/
VOID RTFCNDCL CNI6363Tester::DoContinuousRunTest()
{
   ::printf("\nContinuous run test with AI interrupt enabled: min=%d, enaFast=%s, enaEvts=%s\n", m_nMin,
         (m_enaFast ? "true" : "false"), (m_enaEvts ? "true" : "false"));
   
   CCxAnalogOut* pAO = m_pNI6363->GetAOSubDevice();
   CCxAnalogIn* pAI = m_pNI6363->GetAISubDevice();
   CCxEventTimer* pDIO = m_pNI6363->GetEventTimerSubDevice();

   BOOL bOk = pAO->Init();
   if(!bOk) ::printf("   ERROR: AO initialization failed:\n      %s\n", pAO->GetLastDeviceError());
   if(bOk)
   {
      bOk = pDIO->Init();
      if(!bOk) ::printf("   ERROR: DIO initialization failed:\n      %s\n", pDIO->GetLastDeviceError());
   }
   if(bOk)
   {
      bOk = (10 == pDIO->Configure(10, 0x0000FFFF));
      if(!bOk) ::printf("   ERROR: DI timestamping configuration failed:\n      %s\n", pDIO->GetLastDeviceError());
   }
   if(bOk)
   {
      bOk = pAI->Init();
      if(!bOk) ::printf("   ERROR: AI initialization failed:\n      %s\n", pAI->GetLastDeviceError());
   }
   if(bOk)
   {
      m_viScanInterval = 1;
      bOk = ConfigureAISeq(m_enaFast);
      if(!bOk) ::printf("   ERROR: AI configuration failed:\n      %s\n", pAI->GetLastDeviceError());
   }
   if(!bOk) return;
   
   ::printf("   Initialized. Continuous-running test begun...\n");
   m_eTimeISR.Reset();
   pAI->Start();
   
   // counters to keep track of elapsed time: minutes, AI scan ticks (1ms)
   int nMinutes = 0;
   int nTicks = 0;
   
   // event timer stats collected
   int nDelivered = 0;
   int nDetected = 0;
   int nBad = 0;
   double tAccumDiff = 0.0;
   double maxDiff = 0;
   DWORD currIn = 0;
   float tIn = 0.0f;

   // elaspsed time object implements timeout in case AI device freezes; timeout = 2 AI scan intervals, in microsecs
   CElapsedTime eTime;  
   double dTimeout = ((double)m_viScanInterval) * 2000.0; 
   BOOL timestamping = FALSE;
   BOOL done = FALSE;
   while(!done)
   {
      // **WAIT** for start of next "scan epoch"; timeout mechanism prevents deadlock if AI device stops functioning
      eTime.Reset();
      while( (!m_vbInterruptPending) && (eTime.Get() < dTimeout) ) ;
      if(!m_vbInterruptPending)
      {
         double d = eTime.Get();
         ::printf("   ERROR at t=%.3f : Failed to register start-of-scan interrupt!\n", (nMinutes*60000 + nTicks)*0.001);
         ::printf("        Timeout timer = %.3f microsecs.\n", d);
         break;
      }
      m_vbInterruptPending = FALSE; 

      // abort on an excessively long ISR latency
      if(m_vbDelayedISR) 
      {
         ::printf("   ERROR at t=%.3f : AI ISR latency too long!\n", (nMinutes*60000 + nTicks)*0.001);
         break;
      }

      // unload next scan's worth from AI device. ABORT on AI error or frame shift (runtime loop one full cycle late).
      if(!UnloadNextAIScan()) 
      { 
         ::printf("   ERROR at t=%.3f : AI.Unload()\n      %s\n", (nMinutes*60000 + nTicks)*0.001, pAI->GetLastDeviceError());
         break;
      }

      if(m_vbFrameLag)
      {
         ::printf("   ERROR at t=%.3f : Fell behind AI timeline by one full cycle.\n", (nMinutes*60000 + nTicks)*0.001);
         break;
      }

      // if timestamping is ON, deliver a "marker pulse" on DO0 with a probability of ~20%
      if(timestamping)
      {
         double chance = ((double) rand()) / ((double) RAND_MAX);
         if(chance <= 0.2)
         {
            pDIO->SetDO(0x0001);
            pDIO->SetDO(0x0000);
            ++nDelivered;
         }
      }
      
      // update sinusoidal waveform on AO0
      int t = nTicks % 1000;
      float voltage = (float) (5.0 * ::sin(2.0 * cMath::PI * t / 1000.0));
      if(!pAO->Out(0, voltage))
      {
         ::printf("   ERROR at t=%.3f: AO.Out()\n      %s\n", (nMinutes*60000 + nTicks)*0.001, pAI->GetLastDeviceError());
         break;
      }
      
      // unload any digital timestamp events detected and accumulate total events. We don't expect more than 1 event
      // per scan.
      if(timestamping)
	   {
         DWORD nRead = pDIO->UnloadEvents(1, &currIn, &tIn);
         if(nRead == 0)
         {
            LPCSTR emsg = pDIO->GetLastDeviceError();
            if(::strlen(emsg) > 0)
            {
               ::printf("   ERROR at t=%.3f: DIO.Unload()\n      %s\n", (nMinutes*60000 + nTicks)*0.001, emsg);
			      ::printf("   DEBUG: delivered/detected = %d/%d\n", nDelivered, nDetected);
               break;
            }
         }
         else
         {
            ++nDetected;
            if(currIn == 0x0001)
            {
               // compare timestamp with run time since timestamping turned on (accurate to a ms), in seconds
               double d = cMath::abs((nMinutes * 60000 + nTicks - 499) * 0.001 - tIn);
               if(d > maxDiff) maxDiff = d;
               tAccumDiff += d;
            }
            else ++nBad;
         }
      }
      
      // advance to next tick
      ++nTicks;
      if(nTicks == 500 && m_enaEvts) { timestamping = TRUE; pDIO->Start(); }
      if(nTicks == 60000)
      {
         nTicks = 0;
         ++nMinutes;
         if(nMinutes == m_nMin) done = TRUE;
         else ::printf("   %02d minutes remaining...\n", m_nMin-nMinutes);
      }
   }
   
   pAI->Stop();
   pDIO->Stop();
   
   pAO->Init();
   pAI->Init();
   pDIO->Init();

   if(done)
   {
      ::printf("   TEST COMPLETED.\n");
      if(m_enaEvts)
      {
         ::printf("   Total DI events delivered : detected : bad = %d : %d : %d\n", nDelivered, nDetected, nBad);
         ::printf("   Worst-case timestamp differential = %.6f\n", maxDiff);
         if(nDetected - nBad > 0) 
            ::printf("   Avg timestamp differential = %.6f\n", tAccumDiff/((double) nDetected - nBad));
      }
   }
}

/**
7 : Countdown test. This test configures the G2 counter to countdown a specified interval of time from 1us to 1sec,
using the 100MHz internal timebase for the counter source. Reports whether test is successful or not and the actual 
elapsed time as measured using RtGetClockTime(). Note that the elapsed time measurement does not include the time
it takes to program and arm the counter. Testing indicated that arming the counter took 1-2 microseconds.
*/
VOID RTFCNDCL CNI6363Tester::DoCountdownTest()
{
   ::printf("\nCountdown test:\n");

   m_suspendMgr.Bypass();

   LARGE_INTEGER liStart, liEnd;
   ::RtGetClockTime(CLOCK_FASTEST, &liStart);
   for(int i = 1; i < 10000; i++) ::RtGetClockTime(CLOCK_FASTEST, &liEnd);
   ::printf("   Pre-test: Avg exec time of RtGetClockTime = %.2f us.\n", 
      ((double)(liEnd.QuadPart - liStart.QuadPart)) / (10.0 * 10000));

   double tElapsedUS = 0;
   BOOL bOk = m_pNI6363->RunCtrCountdownTest(m_tWaitUS, tElapsedUS);
   m_suspendMgr.Resume();

   ::printf("   Test %s. Desired wait = %d us; actual elapsed time = %.1f us.\n", bOk ? "completed" : "failed", 
      m_tWaitUS, tElapsedUS);
}

/**
 ServiceAI(): Respond to a hardware interrupt from the analog input (AI) board.

 This ISR is essentially the same as that in MaestroRTSS, but it lacks many of the state variables that MaestroRTSS
 uses to keep track of application state in the various operational modes.
 
 Like MaestroRTSS, CNI6363Tester enables only one kind of interrupt from the AI device, a "start-of-scan" interrupt that
 occurs once per scan interval, <~100us before all available AI channels (the "slow scan set") have been scanned. This 
 ISR responds to and clears that interrupt, then updates a few runtime control variables used in the various tests.

 @param pThisObj Ptr to THIS (static methods do not get the implied THIS arg!!).
 @return TRUE if AI board was source of interrupt; FALSE otherwise (to allow shared IRQ).
*/
BOOLEAN RTFCNDCL CNI6363Tester::ServiceAI(PVOID pThisObj)
{
   CNI6363Tester* pApp = (CNI6363Tester*)pThisObj;

   RtDisableInterrupts();

   // check for & ack the "start of scan" interrupt on the AI subdevice. If it occurred, update runtime control state.
   BOOLEAN bIntAckd = FALSE; 
   if(pApp->m_pNI6363->GetAISubDevice()->IntAck())
   {
      // detect ISR latency > 500us
      int iDelay = int(pApp->m_eTimeISR.GetAndReset() + 0.5);
      if(pApp->m_viElapsedTicks > 0) iDelay -= 1000 * pApp->m_viScanInterval;
      if(iDelay > 500) pApp->m_vbDelayedISR = TRUE;

      // increment #ticks elapsed, and set flag indicating that another scan has started.
      pApp->m_viElapsedTicks++; 
      pApp->m_vbInterruptPending = TRUE; 

      bIntAckd = TRUE; 
   }

   ::RtEnableInterrupts();
   return(bIntAckd);
}


/**
 ConfigureAISeq, StartAISeq 

 Configure/start the prototypical MaestroRTSS AI data acquisition sequence (most configuration details are handled by
 the CCxAnalogIn device object):
    -- Sample all available AI channels in sequence at the current AI scan interval. The channels are sampled as rapidly
 as possible at the start of the scan epoch. This constitutes the "slow" data stream.
    -- Optionally sample the dedicated SPIKECHANNEL at 25KHz -- this "fast" data stream provides a high-resolution
 recording of the spike waveform.
    -- Generate an interrupt at the start of each scan interval.

 Certain runtime state variable are also reset in this method: zero the slow & fast data buffers used to unload data 
 from the AI device on a scan-by-scan basis; reset "tick" counters that keep track of the # of AI scans that have been 
 unloaded thus far and the # of AI scans that have actually elapsed since the start of the AI sequence; an "AI interrupt
 pending" flag; and a flag set whenever the runtime loop falls at least one full scan behind the AI timeline.

 The method StartAISeq() resets a CElapsedTime object dedicated to the detection of long ISR latencies, then starts the 
 AI sequence.

 @param bSpikeCh If TRUE, configure AI device to record "fast" data [default = FALSE].
 @return TRUE if successful, FALSE otherwise.
*/
BOOL RTFCNDCL CNI6363Tester::ConfigureAISeq( BOOL bSpikeCh /* =FALSE */ )
{
   // configure the AI sequence. This will reset an AI operation that had been in progress
   BOOL bOk = m_pNI6363->GetAISubDevice()->Configure(16, m_viScanInterval * 1000, bSpikeCh ? 15 : -1, TRUE);
   
   // reset runtime variables associated with the AI sequence
   m_vbInterruptPending = FALSE; 
   m_vbFrameLag = FALSE;
   m_viElapsedTicks = 0;
   m_viServicedTicks = 0;
   m_vbDelayedISR = FALSE;
   
   memset(m_shSlowBuf, 0, CNI6363Tester::NUMAI*2*sizeof(short));
   m_bHasTwoScans = FALSE;
   memset(m_shFastBuf, 0, CNI6363Tester::FASTBUFSZ*sizeof(short));
   m_nFast = 0;

   return(bOk);
}

VOID RTFCNDCL CNI6363Tester::StartAISeq()
{
   m_eTimeISR.Reset();
   m_pNI6363->GetAISubDevice()->Start();
}


/**
 Service the ongoing AI data acquisition sequence by unloading up to two full scan's of "slow data" and any 
 accompanying "fast data". The data is stored in dedicated buffers, which should be copied or otherwise used prior to 
 invoking the method again. Relevant runtime variables -- see ConfigureAISeq() -- are also updated.

 NOTES:
 1) The CCxAnalogIn implementation handles all the details of segregating the two data streams.
 2) Call this method with bWait=TRUE only when an AI sequence is actually in progress, and only when at least one
 complete scan's worth of data is pending in the AI FIFO. If bWait=TRUE and the expected number of samples are not in 
 the FIFO, the function will block until this is the case, or a device timeout occurs.

 @param bWait -- If TRUE, wait to unload a full scan's worth (or two) of samples [default=TRUE].
 @return TRUE if successful, FALSE if an AI device error occurred or if bWait was FALSE and a full scan's worth of data 
 was not immediately available.
*/
BOOL RTFCNDCL CNI6363Tester::UnloadNextAIScan( BOOL bWait /* =TRUE */ )
{
   // is there a lag of at least one full scan? If so, collect two scans of slow data and any accompanying fast data
   m_vbFrameLag = BOOL((m_viElapsedTicks - m_viServicedTicks) > 1);
   int nSlowScans = m_vbFrameLag ? 2 : 1;
   int nSlow = nSlowScans * CNI6363Tester::NUMAI;
   m_nFast = CNI6363Tester::FASTBUFSZ;

   // if unloading two scans, the most recent scan is second
   m_bHasTwoScans = m_vbFrameLag;
   
   // update #scans unloaded since DAQ start
   m_viServicedTicks += nSlowScans; 

   // actually do the work!
   BOOL bOk = m_pNI6363->GetAISubDevice()->Unload(m_shSlowBuf, nSlow, m_shFastBuf, m_nFast, bWait);
   if(bOk && !bWait) bOk = BOOL(nSlow == nSlowScans * CNI6363Tester::NUMAI);
   return(bOk);
}
