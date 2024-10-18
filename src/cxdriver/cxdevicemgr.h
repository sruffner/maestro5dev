//======================================================================================================================
// cxdevicemgr.h : Declaration of CCxDeviceMgr, MaestroRTSS's "device manager"
//======================================================================================================================

#if !defined(CXDEVICEMGR_H__INCLUDED_)
#define CXDEVICEMGR_H__INCLUDED_

#include "devices\cxeventtimer.h"      // CCxEventTimer -- abstract interface for the DIO event timer device
#include "devices\cxanalogin.h"        // CCxAnalogIn -- abstract interface for the analog input device
#include "devices\cxanalogout.h"       // CCxAnalogOut -- abstract interface for the analog output device
#include "devices\cxrmvideo.h"         // CCxRMVideo -- concrete interface for the RMVideo framebuffer display device

#include "devices\ni6363.h"            // CNI6363 -- Three subdevices are implemented on the NI PCIe-6363.


class CCxMasterIO; 

class CCxDeviceMgr
{
public:
   // device info for a "no device found" placeholder object
   static const CDevice::DevInfo NULLDEV;

private:
   // pseudo-device representing the point-to-point Ethernet link with RMVideo
   CCxRMVideo*    m_pRMVideo;

   // if the NI PCIe-6363 is found, this is the parent device object that acquires and releases the hardware. It
   // exposes "subdevices" that implement the AI, AO, and DIO timer functions
   CNI6363* m_pNI6363Dev;

   // prevent compiler from automatically providing default copy constructor and assignment operator
   CCxDeviceMgr(const CCxDeviceMgr& src); 
   CCxDeviceMgr& operator=(const CCxDeviceMgr& src);

public:
   // constructor/destructor
   CCxDeviceMgr() 
   {
      m_pRMVideo = NULL;
      m_pNI6363Dev = NULL;
   }
   ~CCxDeviceMgr() { Shutdown(); } 

   // acquire device resources needed by MaestroRTSS
   BOOL RTFCNDCL Startup(CCxMasterIO* pIO); 
   // release all hardware device resources
   VOID RTFCNDCL Shutdown(); 

   // expose the device function objects needed to communicate with and use relevant hardware devices
   CCxEventTimer* GetTimer() { return(m_pNI6363Dev != NULL ? m_pNI6363Dev->GetEventTimerSubDevice() : NULL); }
   CCxAnalogIn* GetAI() { return(m_pNI6363Dev != NULL ? m_pNI6363Dev->GetAISubDevice() : NULL); }
   CCxAnalogOut* GetAO() { return(m_pNI6363Dev != NULL ? m_pNI6363Dev->GetAOSubDevice() : NULL); }
   CCxRMVideo* GetRMVideo() { return(m_pRMVideo); }

private:
   // helper methods invoked by Startup() to attach to the various supported hardware devices in the system
   BOOL RTFCNDCL AttachToNI6363MioDev(CCxMasterIO* pIO);
   BOOL RTFCNDCL AttachToRMVideo(CCxMasterIO* pIO);

};


#endif   // !defined(CXDEVICEMGR_H__INCLUDED_)
