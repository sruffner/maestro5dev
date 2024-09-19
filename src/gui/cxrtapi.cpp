//===================================================================================================================== 
//
// cxrtapi.cpp : Implementation of static class CCxRtapi, exposing RTX64 functionality exported by RtApi.DLL that can
// execute in a Windows process.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// In order to start and communicate with CXDRIVER, the RTSS process that controls the hardware and runs experimental 
// protocols, MAESTRO must use the IntervalZero-supplied DLL that exports RTX-specific API supported in a Windows
// execution environment. There are two alternatives to accessing the methods exported by a DLL: implicit or explicit 
// linking. We originally chose the former technique, which is simplest because you merely statically link to a library
// (RtApi.lib) and invoke the necessary RTX API functions as prototyped in a header file (rtapi.h). However, implicit 
// linking has a major drawback. At application startup, Windows attempts to load all DLLs upon which an application 
// depends and, if it is unable to do so, the application will not start. As a consequence, the MAESTRO executable 
// built in this manner will NOT run on machines that do not have RTX64 installed! This is undesirable, since users 
// should be able to run MAESTRO on their office workstations in order to create and edit experiment protocols.
//
// To overcome the problem, we must use explicit linking to attach to the RTX DLL. CCxRtapi is a static class -- do NOT
// instantiate it! -- that encapsulates all the details of loading the DLL (file RtApi.dll) and obtaining function 
// pointers for those RTX64 API functions that are required in MAESTRO. Be sure to call Open() during application 
// startup to attach to the DLL, then call Close() during application shutdown to free the DLL. All required functions 
// are exposed as static CCxRtapi methods. If the RTX DLL has not been loaded, these methods will, of course, fail.
// 
// Cautions:  1) There's no thread safety here!  2) Exports ASCII versions of any RTX fcns that have string args.
//
// CREDITS: 
// 1) Source code example on explicit linking to RTAPI_W32.DLL, provided by tech support at VenturCom, Inc. (now
// known as IntervalZero). Also looked at a more recent example written for RTX64 rather than RTX.
//
// REVISION HISTORY:
// 11mar2003-- Created.
// 25oct2017-- Updated for Windows 10 64-bit and RTX64 platform. Several RTAPI functions were removed that are not
// supported in RTX64 [Rt(Un)LockProcess/Kernel]. Also, RtGetClockTime() was removed since we no longer need it in 
// MAESTRO. New functions available in RTX64 were added to support launching and terminating an RTSS process.
// 18sep2024-- RtSleepFt is no longer supported on Windows side a/o RTX64 4.5.
//
//===================================================================================================================== 

#include "stdafx.h"                          // standard MFC stuff

#include "cxrtapi.h"



CCxRtapi::RTCREATESHAREDMEMORY CCxRtapi::m_fpRtCreateSharedMemory = NULL;
CCxRtapi::RTOPENSHAREDMEMORY CCxRtapi::m_fpRtOpenSharedMemory = NULL;
CCxRtapi::RTOPENMUTEX CCxRtapi::m_fpRtOpenMutex = NULL;
CCxRtapi::RTCLOSEHANDLE CCxRtapi::m_fpRtCloseHandle = NULL;
CCxRtapi::RTWFSO CCxRtapi::m_fpRtWaitForSingleObject = NULL;
CCxRtapi::RTOPENPROC CCxRtapi::m_fpRtOpenProcess = NULL;
CCxRtapi::RTTERMINATEPROC CCxRtapi::m_fpRtTerminateProcess = NULL;
CCxRtapi::RTCREATEPROC CCxRtapi::m_fpRtCreateProcess = NULL;

BOOL CCxRtapi::m_bIsOpen = FALSE;
HMODULE CCxRtapi::m_hRtxDll = NULL;


//=== Open ============================================================================================================ 
//
//    Load the RTX DLL in file RTAPI_W32.DLL and get function pointers for each RTX method exposed by CCxRtapi.  If we 
//    are unable to load the DLL or we fail to initialize ANY of the required function pointers, the call fails.
//
//    If the RTX DLL has already been loaded, the function does nothing.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL CCxRtapi::Open()
{
   // nothing to do if DLL already loaded
   if( m_bIsOpen ) return( TRUE );  

   // load the RTX DLL, then get the functions we will use..
   m_hRtxDll = ::LoadLibrary( _T("RtApi.dll") );  
   if( m_hRtxDll == NULL ) return( FALSE );

   m_fpRtCreateSharedMemory = (RTCREATESHAREDMEMORY) ::GetProcAddress( m_hRtxDll, "RtCreateSharedMemoryA" );
   if( m_fpRtCreateSharedMemory == NULL ) goto ERROR_EXIT;

   m_fpRtOpenSharedMemory = (RTOPENSHAREDMEMORY) ::GetProcAddress( m_hRtxDll, "RtOpenSharedMemoryA" );
   if( m_fpRtOpenSharedMemory == NULL ) goto ERROR_EXIT;

   m_fpRtOpenMutex = (RTOPENMUTEX) ::GetProcAddress( m_hRtxDll, "RtOpenMutexA" );
   if( m_fpRtOpenMutex == NULL ) goto ERROR_EXIT;

   m_fpRtCloseHandle = (RTCLOSEHANDLE) ::GetProcAddress( m_hRtxDll, "RtCloseHandle" );
   if( m_fpRtCloseHandle == NULL ) goto ERROR_EXIT;

   m_fpRtWaitForSingleObject = (RTWFSO) ::GetProcAddress( m_hRtxDll, "RtWaitForSingleObject" );
   if( m_fpRtWaitForSingleObject == NULL ) goto ERROR_EXIT;

   m_fpRtOpenProcess = (RTOPENPROC) ::GetProcAddress(m_hRtxDll, "RtOpenProcess");
   if(m_fpRtOpenProcess == NULL) goto ERROR_EXIT;

   m_fpRtTerminateProcess = (RTTERMINATEPROC) ::GetProcAddress(m_hRtxDll, "RtTerminateProcess");
   if(m_fpRtTerminateProcess == NULL) goto ERROR_EXIT;

   m_fpRtCreateProcess = (RTCREATEPROC) ::GetProcAddress(m_hRtxDll, "RtCreateProcessA");
   if(m_fpRtCreateProcess == NULL) goto ERROR_EXIT;

   m_bIsOpen = TRUE;
   return( TRUE );

ERROR_EXIT: 
   Close();
   return( FALSE );
}


//=== Close =========================================================================================================== 
//
//    If the RTX DLL is loaded, free it.  After invoking this fcn, all RTX API methods exposed by CCxRtapi will fail.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxRtapi::Close()
{
   if( m_bIsOpen ) 
   {
      ::FreeLibrary( m_hRtxDll );
      m_hRtxDll = NULL;
      m_bIsOpen = FALSE;
      m_fpRtCreateSharedMemory = NULL;
      m_fpRtOpenSharedMemory = NULL;
      m_fpRtOpenMutex = NULL;
      m_fpRtCloseHandle = NULL;
      m_fpRtWaitForSingleObject = NULL;
      m_fpRtOpenProcess = NULL;
      m_fpRtTerminateProcess = NULL;
      m_fpRtCreateProcess = NULL;
   }      
}
