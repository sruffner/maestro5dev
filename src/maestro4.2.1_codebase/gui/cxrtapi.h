//===================================================================================================================== 
//
// cxrtapi.h : Declaration of class CCxRtapi.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXRTAPI_H__INCLUDED_)
#define CXRTAPI_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <windows.h>
#include "rtapi.h"         // constants and typedefs defined in the RTX API

class CCxRtapi
{
private:
   typedef HANDLE (RTAPI *RTCREATESHAREDMEMORY) (DWORD, DWORD, DWORD, LPCTSTR, VOID**);
   typedef HANDLE (RTAPI *RTOPENSHAREDMEMORY) (DWORD, BOOL, LPCTSTR, VOID**);
   typedef HANDLE (RTAPI *RTOPENMUTEX) (DWORD, BOOL, LPCTSTR);
   typedef BOOL (RTAPI *RTCLOSEHANDLE) (HANDLE);
   typedef VOID (RTAPI *RTSLEEPFT) (PLARGE_INTEGER);
   typedef DWORD (RTAPI *RTWFSO) (HANDLE, DWORD);
   typedef HANDLE (RTAPI *RTOPENPROC) (DWORD, BOOL, DWORD);
   typedef BOOL (RTAPI *RTTERMINATEPROC) (HANDLE, UINT);
   typedef BOOL(RTAPI *RTCREATEPROC) (LPCTSTR, LPTSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
      BOOL, DWORD, LPVOID, LPCTSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION);

   static RTCREATESHAREDMEMORY m_fpRtCreateSharedMemory;
   static RTOPENSHAREDMEMORY m_fpRtOpenSharedMemory;
   static RTOPENMUTEX m_fpRtOpenMutex;
   static RTCLOSEHANDLE m_fpRtCloseHandle;
   static RTSLEEPFT  m_fpRtSleepFt;
   static RTWFSO m_fpRtWaitForSingleObject;
   static RTOPENPROC m_fpRtOpenProcess;
   static RTTERMINATEPROC m_fpRtTerminateProcess;
   static RTCREATEPROC m_fpRtCreateProcess;

   static BOOL m_bIsOpen;
   static HMODULE m_hRtxDll;

public:
   static BOOL Open();
   static VOID Close();

   static HANDLE RtCreateSharedMemory( DWORD dwProtect, DWORD dwMaxHigh, DWORD dwMaxLo, LPCTSTR lpName, VOID** ppLoc )
   {
      return( m_bIsOpen ? m_fpRtCreateSharedMemory( dwProtect, dwMaxHigh, dwMaxLo, lpName, ppLoc ) : NULL );
   }

   static HANDLE RtOpenSharedMemory( DWORD dwAccess, BOOL bInherit, LPCTSTR lpName, VOID** ppLoc )
   {
      return( m_bIsOpen ? m_fpRtOpenSharedMemory( dwAccess, bInherit, lpName, ppLoc ) : NULL );
   }

   static HANDLE RtOpenMutex( DWORD dwAccess, BOOL bInherit, LPCTSTR lpName )
   {
      return( m_bIsOpen ? m_fpRtOpenMutex( dwAccess, bInherit, lpName ) : NULL );
   }

   static BOOL RtCloseHandle( HANDLE hObject )
   {
      return( m_bIsOpen ? m_fpRtCloseHandle( hObject ) : FALSE );
   }

   static VOID RtSleepFt( PLARGE_INTEGER pi64Dur )
   {
      if( m_bIsOpen ) m_fpRtSleepFt( pi64Dur );
   }

   static DWORD RtWaitForSingleObject(HANDLE hObject, DWORD dwMillisec)
   {
      return(m_bIsOpen ? m_fpRtWaitForSingleObject(hObject, dwMillisec) : WAIT_FAILED);
   }

   static HANDLE RtOpenProcess(DWORD dwAccess, BOOL bInherit, DWORD dwProcID)
   {
      return(m_bIsOpen ? m_fpRtOpenProcess(dwAccess, bInherit, dwProcID) : NULL);
   }

   static BOOL RtTerminateProcess(HANDLE hProc, UINT uExitCode)
   {
      return(m_bIsOpen ? m_fpRtTerminateProcess(hProc, uExitCode) : FALSE);
   }

   static BOOL RtCreateProcess(LPCTSTR lpAppName, LPTSTR lpCmdLine, LPSECURITY_ATTRIBUTES lpProcAttrs, 
      LPSECURITY_ATTRIBUTES lpThreadAttrs, BOOL bInherit, DWORD dwCreationFlags, LPVOID lpEnv, LPCTSTR lpCurrDir, 
      LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcInfo)
   {
      return(m_bIsOpen ?
         m_fpRtCreateProcess(lpAppName, lpCmdLine, lpProcAttrs, lpThreadAttrs, bInherit, dwCreationFlags, lpEnv, 
            lpCurrDir, lpStartupInfo, lpProcInfo) :
         FALSE);
   }
};


#endif   // !defined(CXRTAPI_H__INCLUDED_)
