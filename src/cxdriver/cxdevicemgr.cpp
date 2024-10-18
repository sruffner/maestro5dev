/**=====================================================================================================================
 cxdevicemgr.cpp : Implementation of class CCxDeviceMgr, Maestro's "device manager".

 AUTHOR:  saruffner

 DESCRIPTION:
 The purpose of Maestro's abstract hardware interface framework is to allow an application to work with ANY device
 for which there exists a suitable interface implementation, without "having to know" which kind of device is installed 
 in the system. Then how does the app find and "attach" to the device? This task is handled by the device manager, 
 CCxDeviceMgr. This class is necessarily "aware" of the different abstract interfaces and the available realizations of 
 each interface. During startup, the device manager tries to find in the system a physical device that implements one 
 (or more) of the desired hardware functions (AI, AO, DIO event-timer, and RMVideo). Once found, the manager 
 instantiates the device object for that particular board and attempts to "open" and initialize the device. The manager 
 maintains all acquired device objects and provides access to each via pointer -- cast to the abstract interface class 
 (CCxAnalogIn, etc). The application can then use the base interface pointer to access the hardware functionality 
 without knowing which supported device is being used.

 What if a supported device is NOT found in the system for a particular hardware function? Rather than returning a NULL
 pointer, CCxDeviceMgr returns a pointer to a "placeholder" implementation of the hardware interface. This placeholder 
 is a lightweight implementation of the interface representing the **absence** of that hardware function. Essentially, 
 calling any placeholder method will have no effect and, where appropriate, will return an error indication. Why do 
 this? So that we do not have to check for NULL pointers before calling interface methods.

 ==> Usage.
 Construct the CCxDeviceMgr object, then invoke Startup() to connect to all available Maestro-supported hardware devices
 installed in the host machine. Maestro requires only one instance for each hardware device function, and 
 CCxDeviceMgr enforces this fact by preventing multiple attachments. If a device of a particular interface class is not 
 found, CCxDeviceMgr creates a suitable "no device found" placeholder object. To access a particular device object, use 
 the inline methods GetTimer(), GetAI(), etc. Finally, call Shutdown() to release all allocated device resources when 
 they are no longer needed.

 ==> Exception to the rule: CCxRMVideo.
 The OpenGL application RMVideo was developed to replace the antiquated VSG2/4 framebuffer video board. RMVideo runs on
 a separate Linux workstation and communicates with Maestro over a private, dedicated Ethernet link.  While it is
 treated as a "device" from Maestro's standpoint, there is only one realization. CCxRMVideo serves as the concrete 
 interface with RMVideo; there is no abstract interface.

 ==>Target devices supported.
 1) CCxRMVideo -- handles communication with RMVideo over a private, dedicated Ethernet link using RTX TCP/IP.
 2) CNI6363 -- The PCIe-6363 MIO board from National Instruments. Support introduced in Maestro 3; in fact, this board
 is intended to serve as 3 devices in one -- handling AI, AO, and the DIO event timer functions. This is possible 
 because the AI, AO and DIO subsystems on the PCIe-6363 can all work independently. CNI6363 implements CDevice for the 
 PCIe-6363; it is created to acquire and initialize the board. If successful, CNI6363 constructs and provides access to 
 three "pseudo-subdevice" objects implementing the three hardware functions. Each of these subdevice objects wraps the 
 original CNI6363 parent device object, and their CDevice methods related to acquiring/releasing the hardware are
 essentially no-ops.


 REVISION HISTORY:
 18jun2002-- Created. CCxDeviceMgr is the "device manager" for CXDRIVER. It encapsulates knowledge of all realized
    implementations of the CXDRIVER abstract hardware interfaces, and it handles the task of creating the device 
    "object" associated with each physical device.  Also defined in the module are "no device found" placeholder 
    implementations for each of the abstract hardware interfaces.
 09jan2003-- Integrated XY scope device interface CCxScope and its available realizations.
 24jan2003-- Integrated OKNDRUM PID servo controller interface CCxServo and its available realization CDC2PC100.
 23apr2003-- Integrated framebuffer video device interface CCxFrameBuf and its available realization for the VSG2/4.
          -- Moved "null" placeholder device classes to their respective device interface class declarations.
          -- Now that the framebuffer device has been incorporated into the device manager framework, we can now further
    isolate the process of attaching to CXDRIVER hardware. The AttachTo***() methods are now private, and all device 
    attachments are handled in the new method Startup(). New inline methods GetTimer(), GetAI(), etc expose the device 
    objects to CCxDriver.
 30apr2003-- Startup() now takes a pointer to the Maestro interface object CCxMasterIO so it can post progress messages 
    during device object creation.
 29dec2003-- Startup() sets the "device install path" (CDevice::SetInstallPath()) IAW the home directory provided by
    CCxMasterIO::GetHomeDirectory(). This is part of the mechanism for making the MaestroGUI/MaestroRTSS installation 
    relocatable.
 14mar2006-- As of Maestro v1.5.0, the OKNDRUM target is no longer supported. CCxDeviceMgr modified accordingly.
 24mar2006-- Removed CCxFrameBuf from MAESTRODRIVER. Replaced by CCxRMVideo, the interface to the remote video 
    application RMVideo.  CCxRMVideo is a concrete interface -- there will be no "alternate" realizations.
 16jun2011-- Begun changes for Maestro 3. Support old hardware, while adding support for the NI PCIe-6363, a multi-
    function IO board that should be able to handle AI, AO, and DIO eventtimer functions. This "breaks the mold" of the 
    CDevice-based class framework, however, since one real device is handling the work of three conceptual devices....
 05jul2011-- Dropped support for CXYDaytona. This implementation of CCxScope on Spectrum Signal Processing's Daytona
    C6x board was never successfully completed. Since the XYScope is on its way out (can't find large analog XY 
    oscilloscopes any more!), there's no point.
 20mar2012-- Moved calls to WSAStartup() and WSACleanup() from CCxRMVideo to CCxDeviceMgr:Startup() and Shutdown(). 
    These methods connect/disconnect the calling RTSS process to the RTX TCP-IP stack and should be invoked only once 
    per process.
 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build. NOTE that the following legacy boards will 
 not be supported in this build (Maestro "4.x"): LisTech timer board, m62 timer impl, PCIMIO16E1, ATAO10, and PD2AO8.
 11jun2018-- XYScope display support DROPPED. The XYScope hardware boards -- Detroit C6x and Dakar -- are conventional
 PCI boards, and conventional PCI slots are no longer provided on new workstations. Therefore, Maestro 4 does NOT 
 support the XYScope display. However, we elected to leave all XYScope-related code in place, in case the platform is
 somehow revived in a future release. The AttachToXYScopeDev() method merely creates the placeholder null device and 
 reports that XYScope hardware support is not available.
 24sep2024-- Removing all XYScope related code as of Maestro 5.x. Updated comments to explicitly indicate that the only
 supported hardware at this point is the PCIe-6363 multifunction DAQ board (for AI, AO, and DIO/Timer functionality all
 on one device) and the CCXRMVideo interface for TCP-IP communications with RMVideo.
========================================================================================================================
*/

// supported realizations of the various hardware device function interfaces
#include <WinSock2.h>
#include "devices\ni6363.h" 

#include "cxmasterio.h"                   // CCxMasterIO-- the MAESTRO interface object (for posting messages)
#include "cxdevicemgr.h"



const CDevice::DevInfo CCxDeviceMgr::NULLDEV = { 0 };

/**
 Create all device objects and open connections to available MaestroRTSS devices installed in the host machine. If 
 there is no device available for any device class, we create a "placeholder" device object representing the absence of 
 that device.

 This routine will fail only if we are unable to construct a device object -- indicating an "out of memory" condition. 
 For details, see the AttachTo****() methods.

 @param pIO Ptr to the Maestro communication interface, so we can post progress/error messages to Maestro.
 @return TRUE if successful; FALSE otherwise (fatal error - out of memory).
*/
BOOL RTFCNDCL CCxDeviceMgr::Startup(CCxMasterIO* pIO)
{
   // set installation path so that devices can find any required config/program files
   CDevice::SetInstallPath( pIO->GetHomeDirectory() ); 

   // we first try to find and acquire the NI PCIe-6363, which handles the AI, AO, and DIO timer functionality all on 
   // one board. NOTE that, as of Maestro 4.x, legacy alternatives for the AI, AO and DIO timer functions are 
   // no longer supported. And the XYScope device is no longer supported as well.
   BOOL bOk = AttachToNI6363MioDev(pIO);
   
   if(bOk)
   {
      // initialize Winsock API prior to attaching to RMVideo. WSAStartup() should be called once and only once
      // per process, IAW the RTX TCP-IP documentation.
      // NOTE: This limitation on WSAStartup() was an RTX issue and is not present in RTX64. Nevertheless, we
      // chose not to change the implementation.
      WSADATA wsaData;
      int iRes = WSAStartup(MAKEWORD(2,2), &wsaData);
      if(iRes != 0)
      {
         char emsg[150];
         ::sprintf_s(emsg, "(!!) RMVideo unavailable -- failed to initialize WinSock2 (error=%d).", iRes);
         pIO->Message(emsg);
      }
      else
         bOk = AttachToRMVideo(pIO);
   }

   if(!bOk) pIO->Message("(!!) Fatal error: Out of system memory!");
   return(bOk);
}


/**
 Close all device connections and destroy all device objects created by the device manager.
*/
VOID RTFCNDCL CCxDeviceMgr::Shutdown()
{
   if( m_pRMVideo != NULL )
   {
      m_pRMVideo->Close();
      delete m_pRMVideo;
      m_pRMVideo = NULL;
   }
   
   // release Winsock API resources. Per RTX TCP-IP documentation, WSACleanup() should be called just once per process!
   // NOTE: The limitation on WSAStartup/Cleanup() was removed in RTX64, but we chose not to change the implementation.
   WSACleanup();
   
   if(m_pNI6363Dev != NULL)
   {
      // NOTE: Closing the parent device closes the AI, AO and DIO timer subdevices!
      m_pNI6363Dev->Close();
      delete m_pNI6363Dev;
      m_pNI6363Dev = NULL;
   }
}


/**
 Acquire and configure the National Instruments PCIe-6363 multi-function IO board, if it is present in the system. This
 device has AI, AO, DIO and counter subsystems and is capable of detecting changes on its digital inputs. It is the
 preferred hardware device for handling all of MaestroRTSS's AI, AO, and DIO timer functions.
 
 This method is called first during Startup(). If the PCIe-6363 is present and successfully opened, three "subdevice"
 object are opened to implement the CCxAnalogIn, CCxAnalogOut, and CCxEventTimer hardware device interfaces; each of
 these operate independently.

 @param pIO Ptr to the Maestro communication interface, so we can post progress/error messages to Maestro.
 @return TRUE if PCIe-6363 found and initialized, or if it is not present in the host system. Returns FALSE only if
 a catastrophic out-of-memory error occurs (which should never happen).
*/
BOOL RTFCNDCL CCxDeviceMgr::AttachToNI6363MioDev(CCxMasterIO* pIO)
{
   char strMsg[150];

   // we might already be attached to the PCIe-6363
   if(m_pNI6363Dev != NULL) return(TRUE);

   // construct the relevant device object and try to acquire the device. Destroy the device object if we cannot.
   m_pNI6363Dev = new CNI6363(1);
   if(m_pNI6363Dev == NULL) return(FALSE);
   
   if(m_pNI6363Dev->Open())
      ::sprintf_s(strMsg, "%s installed with AI, AO, and DIO event timer subdevices", m_pNI6363Dev->GetDeviceName());
   else
   {
      ::sprintf_s(strMsg, "%s : %s", m_pNI6363Dev->GetDeviceName(), m_pNI6363Dev->GetLastDeviceError());
      delete m_pNI6363Dev;
      m_pNI6363Dev = NULL;
   }
   
   pIO->Message(strMsg);
   return(TRUE);
}


/**
 Create a "device object" that represents a communication link to RMVideo, the OpenGL application that animates Maestro
 framebuffer video targets. RMVideo runs on a separate Linux workstation and communicates with MaestroRTSS over a 
 private, dedicated Ethernet link.

 There is no "abstract interface" for RMVideo, which doesn't really fit into the device interface framework managed by 
 CCxDeviceMgr. Instead of calling CDevice::Open() to start the "device", CCxRMVideo::OpenEx() is called instead -- 
 providing access to the Maestro-MaestroDriver communication interface. CCxRMVideo encapsulates all MaestroRTSS 
 communications with RMVideo.

 @param pIO Ptr to the Maestro communication interface, so we can post progress/error messages to Maestro.
 @return TRUE if successful (even if no device found); FALSE if unable to create a device object -- which is considered 
 a catastrophic error.
*/
BOOL RTFCNDCL CCxDeviceMgr::AttachToRMVideo( CCxMasterIO* pIO )
{
   char strMsg[150];

   // already attached to RMVideo!
   if(m_pRMVideo != NULL) return(TRUE);

   m_pRMVideo = new CCxRMVideo();
   if(m_pRMVideo == NULL) return(FALSE);
   if(m_pRMVideo->OpenEx(pIO))
   {
      ::sprintf_s(strMsg, "Connected to RMVideo: Frame rate = %.3f Hz; %d x %d pixels.",
         1.0/m_pRMVideo->GetFramePeriod(), m_pRMVideo->GetScreenW_pix(), m_pRMVideo->GetScreenH_pix());
      pIO->Message(strMsg);
   }
   else
   {
      ::sprintf_s(strMsg, "RMVideo is not available: %s", m_pRMVideo->GetLastDeviceError());
      pIO->Message(strMsg);
   }

   return(TRUE);
}
