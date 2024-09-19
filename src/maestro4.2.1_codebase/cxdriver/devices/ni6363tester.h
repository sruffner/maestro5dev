/*======================================================================================================================
 ni6363tester.h : Declaration of an application object for testing device object CNI6363.
======================================================================================================================*/

#if !defined(NI6363TESTER_H__INCLUDED_)
#define NI6363TESTER_H__INCLUDED_

#include "suspend.h"                   // CRtSuspendMgr, implements starvation mgt using RTX timer threads
#include "util.h"                      // general utility classes
#include "ni6363.h"                    // the CNI6363 device object

class CNI6363Tester
{
private:
   static const int NUMAI = 16;
   static const int FASTBUFSZ = 200;
   
   // suspend manager: manages CPU usage by the worker thread
   CRtSuspendMgr m_suspendMgr;
   // the device object we're testing: the NI PCIe-6363
   CNI6363* m_pNI6363;
   
   // the test arguments
   int m_which;
   int m_nMin;
   BOOL m_enaFast;
   BOOL m_enaEvts;
   int m_tWaitUS;
   
   // critical runtime control variables...
   volatile BOOL m_vbInterruptPending;          // TRUE if an ADC interrupt requires processing
   volatile int m_viElapsedTicks;               // # of ADC interrupts (ie, # of scans) since AI operation began
   volatile int m_viScanInterval;               // current ADC scan interval in milliseconds
   volatile int m_viServicedTicks;              // # AI scans unloaded by runtime loop since AI operation started
   // TRUE whenever runtime loop is lagging AI timeline by at least one full frame (scan); in this case, slow data 
   // buffer should contain two scan's worth of AI samples
   volatile BOOL m_vbFrameLag;
   volatile BOOL m_vbDelayedISR;                // set TRUE if the ADC ISR latency >= 500us
   CElapsedTime m_eTimeISR;                     // elapsed time between ISRs -- to detect long ISR latencies

   // the 1-2 most recent "slow scans" of all AI channels (raw ADC codes, NOT actual voltages)
   short m_shSlowBuf[NUMAI*2]; 
   // this flag is set whenever the slow scan buffer has two scans' worth of data instead of 1
   BOOL m_bHasTwoScans;
   // most recently collected samples from the 25KHz AI channel, if enabled
   short m_shFastBuf[FASTBUFSZ]; 
   int m_nFast; 
   
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6363Tester(const CNI6363Tester& src); 
   CNI6363Tester& operator=(const CNI6363Tester& src);

public:
   CNI6363Tester();
   ~CNI6363Tester();

   // pseudo entry point -- call this from main()
   VOID RTFCNDCL Go(int nargs, char* args[]); 

private:
   // static helper method invokes non-static worker thread procedure
   static DWORD RTFCNDCL RunEntry(PVOID pThisObj) { return( ((CNI6363Tester*)pThisObj)->Run() ); }
   DWORD RTFCNDCL Run(); 

   // the supported tests
   VOID RTFCNDCL DoStaticAOTest();
   VOID RTFCNDCL DoAIOLoopbackTest();
   VOID RTFCNDCL DoStaticDOTest();
   VOID RTFCNDCL DoDIOLoopbackTest();
   VOID RTFCNDCL DoPerformanceTests();
   VOID RTFCNDCL DoContinuousRunTest();
   VOID RTFCNDCL DoCountdownTest();
   
   // ISR for interrupts from the analog input device
   static BOOLEAN RTFCNDCL ServiceAI(PVOID pThisObj);

   // configure, start, service the prototypical AI acquisition sequence
   BOOL RTFCNDCL ConfigureAISeq(BOOL bSpikeCh = FALSE);
   VOID RTFCNDCL StartAISeq();
   BOOL RTFCNDCL UnloadNextAIScan(BOOL bWait = TRUE);
};


#endif   // !defined(NI6363TESTER_H__INCLUDED_)



