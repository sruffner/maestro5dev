/*======================================================================================================================
 ni6509tester.h : Declaration of an application object for testing device object CNI6509.
======================================================================================================================*/

#if !defined(NI6509TESTER_H__INCLUDED_)
#define NI6509TESTER_H__INCLUDED_

#include "suspend.h"                   // CRtSuspendMgr, implements starvation mgt using RTX timer thread
#include "ni6509.h"                    // the CNI6509 device object

class CNI6509Tester
{
private:
   // suspend manager: manages CPU usage by the worker thread
   CRtSuspendMgr m_suspendMgr;
   // the device object we're testing: the NI PCIe-6509
   CNI6509* m_pNI6509;
   
   // the test choice
   int m_which;

   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6509Tester(const CNI6509Tester& src); 
   CNI6509Tester& operator=(const CNI6509Tester& src);

public:
   CNI6509Tester();
   ~CNI6509Tester();

   // pseudo entry point -- call this from main()
   VOID RTFCNDCL Go(int nargs, char* args[]); 

private:
   // static helper method invokes non-static worker thread procedure
   static DWORD RTFCNDCL RunEntry(PVOID pThisObj) { return( ((CNI6509Tester*)pThisObj)->Run() ); }
   DWORD RTFCNDCL Run();

   // the tests
   VOID RTFCNDCL DoLoopbackTest();
   VOID RTFCNDCL DoIntegrationTest();
};


#endif   // !defined(NI6509TESTER_H__INCLUDED_)



