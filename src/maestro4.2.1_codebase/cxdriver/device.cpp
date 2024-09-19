//===================================================================================================================== 
//
// device.cpp : Implementation of CDevice, ABSTRACT base class for objects representing devices on the ISA or PCI bus.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CDevice is an attempt to provide a base class for all MaestroRTSS device interfaces, which encapsulate hardware 
// devices used by MaestroRTSS -- all of which are housed on either the ISA or PCI/PCIExpress bus. CDevice provides 
// storage for the PCI bus configuration info for a device, as well as a method for searching the PCI bus for a specific
// device and retrieving its PCI configuration info. It supports attaching a single ISR to hardware device interrupts --
// but only if the device resides on the PCI/PCIExpress bus. There is no built-in support for ISA devices, but for 
// completeness' sake, CDevice represents these devices as well (Maestro3 will introduce support for a PCIExpress-based
// National Instruments' card that will replace existing ISA cards used for the analog output and DIO event timer 
// functionality). Finally, since several MaestroRTSS devices use a Texas Instruments' TMS320C4x or C6x digital signal 
// processor, CDevice provides the framework for downloading core programs onto the DSP.
//
// Since CDevice uses the RTX API for kernel-mode access, it is limited for use in the RTX environment (see CREDITS).
//
// ==> Creating the device object and "opening" a connection to the physical device.
// The CDevice constructor requires a DevInfo struct and a device number. The device information includes flags that 
// indicate the device's peripheral bus (PCI or ISA), and whether it contains one of the TI DSPs. For PCI devices, 
// vendor, device, subvendor, and subsystem IDs are provided in order to locate the device on the PCI bus (the last two 
// IDs are optional and should be 0 if not used). The device number refers to the Nth occurrence of the physical 
// device on the host's PCI bus -- thus allowing us to handle multiple instances of the same device.
//
// After the device object is constructed, Open() establishes communications with the actual device. This method, 
// which should not be overridden, performs the following sequence:  
//    (1) [PCI devices only] Locates the device and saves its PCI configuration information. Various public attributes 
//        expose this configuration info. 
//    (2) Maps device resources.
//    (3) Resets the device [devices lacking a TI DSP], or 
//        resets the device and loads the COFF executable [TI DSP dev only].
//    (4) Initializes the device.  
// Close() performs the reverse sequence of tasks to close the connection to the physical device. These methods rely 
// on a number of virtual or pure virtual methods that must be overridden in any practical implementation of CDevice:
//    
//    MapDeviceResources() ==> [required] Maps device memory or I/O space (typically, device registers) into process 
//       space. Implementation is unique to each physical device, and the device object is useless without it. 
//    UnmapDeviceResources() ==> [required] Releases the resources acquired by MapDeviceResources(). CDevice uses this 
//       to clean up if an error occurs while opening a connection, or when closing the connection with Close().
//    DeviceReset() ==> [optional] perform a "hard reset" of the device. CDevice::DeviceReset() does nothing and 
//       returns TRUE. Any firmware load required after the device reset should be performed here. For TIC6x/C4x 
//       devices, this method is required, but the firmware load is taken care of by CDevice::LoadTIDeviceCOFF(). See 
//       next section.
//    OnOpen() ==> [optional] Any device-specific work associated with opening a connection. This method is invoked 
//       by CDevice::Open() after the device is "opened". It is a good place to put a device-specific "sanity check" 
//       to verify device operation.
//    OnClose() ==> [optional] Any device-specific work that needs to be done just prior to "closing" the device.
//    Init() ==> [required] Device-specific initialization. This method should leave the device in an idle "startup" 
//       state, with any hardware interrupts disabled.
//
// ==> Loading COFF executable onto devices using the TI TMS320C4x/C6x DSP.
// Because COFF loading is tedious, yet very similar for all the TI DSP-based boards used by MaestroRTSS thus far, we 
// chose to implement it in CDevice. LoadTIDeviceCOFF() does the work, but it requires a number of virtual methods to 
// perform device-specific work:
//
//    GetCOFFFilename() ==> [required] returns name of the COFF target executable file. This file MUST be located in 
//       the Maestro installation directory, which is set by the device manager during startup via SetInstallPath().
//    DownloadCOFFData() ==> [required] downloads a section of the COFF executable image into TI DSP memory.
//    DeviceReset() ==> [required] perform a "hard reset" of the device. This method should place the TI DSP in a 
//       suspended state, ready for COFF downloading.
//    DeviceStart() ==> [required] release the TI DSP device from the suspended state & start execution of COFF target. 
//    DeviceQuit() ==> [required] terminate execution of the program running on the TI DSP device. This is not needed 
//       by LoadTIDeviceCOFF(), but it is called when the DSP device is "closed".
//
// Clearly, the COFF load cannot succeed without proper implementation of the above functions. For the convenience of 
// derived classes that do NOT represent a TI DSP device, CDevice provides empty "placeholder" implementations for each 
// of these virtual methods.
//
// ==> "Subdevice" concept: When multiple MaestroRTSS device functions are implemented on a single physical device. 
// Maestro3 introduces support for an up-to-date data acquisition card for the PCIExpress bus -- the PCIe-6363 from 
// National Instruments. This is a multifunction device that will be able to implement three different MaestroRTSS
// hardware device interfaces on a single card: the analog input, analog output, and digital I/O event timer functions.
// However, the CDevice framework was not originally designed for this kind of situation, so we had to make some HACKY
// changes in order to make things work. Two overridable CDevice methods were added:
//		IsSubDevice() : Returns TRUE if the device object represents one of multiple subdevice functions implemented on a
// single parent device. The default CDevice implementation returns FALSE.
//    GetParentDevice() : Returns a pointer to the parent device object. The default implementation returns NULL.
//
// The CDevice::Open() method -- which cannot be overridden in derived classes -- has been revised to check whether or
// not the object represents a "subdevice" by calling IsSubFunction(). If so, then it cannot open the subdevice unless
// the parent device is open. Also, it copies device information like slot number, bus number, and PCI configuration 
// information from the parent device object, since they're really the same physical device.
//
// It is recommended that a subdevice implementation maintain a reference to the parent device object and communicate
// with the physical device via methods on the parent object. Thus, the parent device is responsible for mapping
// device resources, implementing the COFF loading helper functions, etc.
//
// This remains a HACK to shoehorn support for the PCIe-6363 into our existing CDevice-based framework, but I didn't
// want to redesign the whole framework at this point...
//
// CREDITS:
// 1) Real-Time eXtenstion (RTX) to Windows by IntervalZero, Inc (www.intervalzero.com). RTX gives the Windows OS 
// real-time-like characteristics and provides kernel access for direct communications with hardware devices. Note that
// the routine FindPCIDevice() was adapted from sample code provided by IntervalZero.
//
//
// REVISION HISTORY:
// 12oct2001-- Created.
// 19oct2001-- Remodeled as an abstract class, including basic operations Open(), Close(), Reset() and Init().
//          -- Support for attaching a single ISR to the device's interrupt was moved from CCxAnalogOut to CDevice. 
//             This change will allow us to support interrupts from other kinds of devices in the future.  The support 
//             for interrupts is limited to PCI devices.
// 13jun2002-- Another redesign to include some additional basic functionality, in particular, a TI DSP COFF loading 
//             routine, which is used by several potential CNTRLX devices...
// 04oct2002-- Modified DeviceStart() to pass the COFF program's "entry point" address as a parameter; modified 
//             LoadTIDeviceCOFF() accordingly.
// 16jan2003-- Minor change in Close():  virtual method OnClose() is now invoked after reinitializing the device.
// 30apr2003-- Added virtual GetDeviceName(), which returns a short string identifying the particular device.  If not
//             overridden, CDevice::GetDeviceName() returns the cryptic "UnknownDev".
// 29dec2003-- Added static methods Get/SetInstallPath() for getting and setting the installation directory for ALL
//             device-related configuration and program files.
//          -- GetCOFFPath() replaced by GetCOFFFilename().  LoadTIDeviceCOFF() assumes that the COFF file is found in 
//             the global device installation directory returned by CDevice::GetInstallPath().
// 29aug2011-- Updating SetInterruptHandler() IAW RTAPI change in migrating from RTX6.5.1 to RTX2011. The function
// RtAttachInterruptVectorEx() is deprecated in RTX2011, replaced by RtAttachInterrupt(), which provides support for 
// both line-based (typically requiring a dedicated IRQ line, often difficult to get during system configuration) and
// message-based (MSI or MSI-X) interrupts. If the device supports MSI and/or MSI-X, then that scheme will be preferred
// over the older line-based scheme.... Also, since we've always ensured that the AI device had device-exclusive access
// to its interrupt line, we got rid of the 'bShare' argument to SetInterruptHandler().
// 31aug2011-- Added IsSubDevice() and GetParentDevice() overridables and revised Open() in order to handle multiple
// MaestroRTSS device functions implemented on a single physical device, which is the case for the NI PCIe-6363.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.
//===================================================================================================================== 

#include "device.h"


//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 

LPCSTR CDevice::EMSG_CANTFIND         = "Device not found";
LPCSTR CDevice::EMSG_PARENTDEVUNAVAIL = "Cannot open subdevice since parent device is unavailable";
LPCSTR CDevice::EMSG_MAPADDRFAIL      = "Cannot remap device memory or I/O space into process memory";
LPCSTR CDevice::EMSG_VERIFYFAIL       = "Sanity check on device failed";
LPCSTR CDevice::EMSG_DEVNOTAVAIL      = "Device not available";
LPCSTR CDevice::EMSG_NOTSUPPORTED     = "Function not supported by this device";
LPCSTR CDevice::EMSG_IRQ_INVALID      = "IRQ line invalid";
LPCSTR CDevice::EMSG_IRQ_NOEXCLUSIVE  = "Cannot get exclusive access to IRQ line";
LPCSTR CDevice::EMSG_IRQ_GENFAIL      = "Failed to attach ISR to IRQ; unknown system error";
LPCSTR CDevice::EMSG_FAILEDRESET      = "Unable to reset device"; 
LPCSTR CDevice::EMSG_OUTOFMEMORY      = "Failed due to low system memory"; 
LPCSTR CDevice::EMSG_COFFREAD         = "COFF file read error";
LPCSTR CDevice::EMSG_COFFSEEK         = "COFF file seek error";
LPCSTR CDevice::EMSG_COFFWRITESECT    = "Failure writing COFF section data to device";
LPCSTR CDevice::EMSG_DEVSTART         = "COFF device core start failed";
LPCSTR CDevice::EMSG_DEVTIMEOUT       = "Device timeout"; 
LPCSTR CDevice::EMSG_USAGE            = "Invalid parameters or other usage error";
LPCSTR CDevice::EMSG_UNKNOWN          = "Unknown device error";


char CDevice::M_INSTALLPATH[] = {0};


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION 
//===================================================================================================================== 

//=== CDevice [constructor] =========================================================================================== 
//
//    Construct a device object.  Note that the type of physical device represented by object is determined at this  
//    time and cannot be changed later.
//
//    ARGS:       devInfo   -- [in] identification information for the physical device.
//                devNum    -- [in] dev instance N: this dev object will be associated with the Nth instance of the 
//                             physical device in the host system.
//
CDevice::CDevice( const CDevice::DevInfo& devInfo, int devNum ) 
{
   m_devInfo = devInfo;
   m_devNumber = (devNum<1) ? 1 : ((devNum>10) ? 10 : devNum);    // device instance restricted to [1..10]

   m_bInUse = FALSE;
   m_nPciBus = -1;
   m_ulPciSlot = 0;
   memset( (PVOID) &m_pciInfo, 0, sizeof(PCI_COMMON_CONFIG) );

   m_hInterrupt = (HANDLE)NULL;
   m_errMsg[0] = '\0';
}


//=== ~CDevice [destructor] =========================================================================================== 
//
//    Make sure we've disconnected from device before dying.
//
//    ARGS:       NONE.
//
CDevice::~CDevice() 
{
   Close();
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== Open ============================================================================================================ 
//
//    Locate device, establish a communication link with it, and prepare it for use.
//
//    The series of tasks required to "open" a device are fairly typical across all devices:  find the device (if it is 
//    on the PCI bus), map the required dev resources into process memory, and initialize the device.  If the device 
//    includes a TI DSP, we also must download the COFF executable onto the DSP chip and start it running. 
//
//    If any additional tasks (such as a sanity check) must be performed prior to initializing the device (but after 
//    establishing comm link), put them in OnOpen(). 
//
//    NOTE: If the device object is a "subdevice" -- one of multiple hardware device functions implemented on a single
//    physical parent device --, then the procedure is different. First, the parent device must be already open in order
//    to open any of its subdevices. Next, the parent device's PCI configuration information (if applicable) is copied
//    into the subdevice object, since it's really just a single device. The PCI bus is NOT searched again. The other
//    tasks are performed as usual.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.  Call GetLastDeviceError() to retrieve error msg.
//
BOOL RTFCNDCL CDevice::Open()
{
   // return immediately if device is already connected
   ClearDeviceError();
   if(IsOn()) return(TRUE);

   // if this is a subdevice, copy PCI configuration info (if applicable) from the parent device object. Of course,
   // we cannot open it at all if the parent device isn't open!
   if(IsSubDevice())
   {
      CDevice* pParent = GetParentDevice();
      if(pParent == NULL || !pParent->IsOn())
      {
         SetDeviceError(EMSG_PARENTDEVUNAVAIL);
         return(FALSE);
      }
      
      // subdevice must have same device info and PCI configuration info as parent, since they're really the same! This
      // is essential, eg, if we'll connect an interrupt to the subdevice object.
      m_devInfo = pParent->m_devInfo;
      m_devNumber = pParent->m_devNumber;
      if(pParent->IsPci())
      {
         m_nPciBus = pParent->m_nPciBus; 
         m_ulPciSlot = pParent->m_ulPciSlot;
         m_pciInfo = pParent->m_pciInfo;
      }
   }
   
   // if it's a PCI device, find it on the PCI subsystem and save PCI configuration info. Abort if device not found.
   // We skip this step for a subdevice, which inherits PCI configuration info from its parent device object.
   if(IsPci() && !IsSubDevice()) 
   {
      if(!FindPCIDev())
      { 
         SetDeviceError(EMSG_CANTFIND);
         return(FALSE);
      }
   }

   // map device resources into process space
   if(!MapDeviceResources())
   {
      // provide a generic error description if derived class does not provide one.
      if(::strlen( m_errMsg ) == 0) SetDeviceError(EMSG_MAPADDRFAIL); 
      ClearPCICfgInfo();
      return(FALSE);
   }

   // reset device and load any onboard firmware. For TIC6x/C4x device, CDevice handles the details...
   if(IsTIDsp())
   {
      if(!LoadTIDeviceCOFF())
      {
         UnmapDeviceResources();
         ClearPCICfgInfo();
         return(FALSE);
      }
   }
   else if(!DeviceReset())
   {
      UnmapDeviceResources();
      ClearPCICfgInfo();
      return(FALSE);
   }

   // activate device interface and put it in an initial idle state, then do any device-specific stuff
   SetOn();   
   if(!Init() || !OnOpen())
   {
      if(IsTIDsp()) DeviceQuit();
      UnmapDeviceResources();
      ClearPCICfgInfo();
      SetOn(FALSE);
      return(FALSE);
   }

   return(IsOn());
}


//=== Close =========================================================================================================== 
//
//    Sever connection to device and clean up.
//
//    Here we initialize the device to return it to an idle state, invoke OnClose() for any device-specific tasks prior 
//    to closing, release the interrupt handler (if any), unmap the device resources, and reset the device object to 
//    the "device not found" state.  For TI DSP-based devices, we attempt to terminate the execution of the DSP target 
//    core as well.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CDevice::Close()
{
   if( IsOn() ) 
   {
      Init();
      OnClose();
      ClearInterruptHandler();
      if( IsTIDsp() ) DeviceQuit();
      UnmapDeviceResources();
      ClearPCICfgInfo();
      SetOn( FALSE );
   }
}


/**
 Attach a single handler routine to interrupts from the device. The device must be hosted on the PCI/PCIExpress bus. 
 The routine gets the maximum RTX priority.

 This routine will call Init() to reinitialize the device and disable its hardware interrupts before installing a new 
 handler routine. If the derived class implementation of Init() fails to disable hardware interrupts as required, the 
 system could freeze!

 Message-based vs line-based interrupts: If the device supports message-based interrupts (MSI or MSI-X), then that
 scheme will be used over the older, more restricted line-based scheme. [Getting device-exclusive access to a precious
 IRQ line has been the most difficult step in configuring a PC to run Maestro. With Maestro 3 supporting a MSI-capable
 AI device, and with RTX's support for newer NIC cards that may also be MSI-capable, we hope to finally eliminate one
 of the most aggravating setup/configuration issues associated with Maestro!]

 If the method attaches to a line-based interrupt, the IRQ line cannot be shared with other devices in the system. 
 Message-based interrupts are never shared.

 The handler routine provided by the caller is entirely responsible for verifying the interrupt came from the device, 
 and if so, acknowledging and servicing that interrupt. If the device was not the source of the interrupt, the handler 
 routine should return FALSE, so that the RTX interrupt service thread knows to pass the interrupt to the next device 
 ISR. Else, it should perform any application-specific work in response to the interrupt (but keep it down to 5us!!), ]
 and return TRUE.

 THE WinNT REGISTRY AND IRQs: It is VITAL that all device interrupts in use be accounted for in the Registry, which is 
 what RtAttachInterrupt() checks when attempting to attach an ISR to a particular IRQ line. The problem is that not all
 device drivers are well-written, and some device drivers may fail to register their use of an IRQ line. Thus, if the 
 AI device is configured to use the same IRQ line as such a "rogue" device (AND remember we have little or no control 
 over the assignment of IRQ lines in a PC!!), the RtAttachInterrupt() call may succeed, but the IRQ will only be 
 handled by the rogue device's driver!! This is not an issue with the more modern message-based interrupts.

 @param pIntHandler Ptr to the handler routine. If NULL, existing handler (if any) is detached.
 @param pContext Context argument passed to handler whenever it is invoked. 
 @return TRUE if successful; FALSE otherwise. Call GetLastDeviceError() to retrieve error message.
*/
BOOL RTFCNDCL CDevice::SetInterruptHandler(BOOLEAN (RTFCNDCL *pIntHandler)(PVOID context), PVOID pContext)
{
   ClearDeviceError();

   // check for obvious problems: interrupts not supported for ISA dev; or, device not available
   if(IsIsa()) 
   {
      SetDeviceError(EMSG_NOTSUPPORTED);
      return(FALSE);
   }
   if((!IsOn()) || (m_nPciBus < 0)) 
   {
      SetDeviceError(EMSG_DEVNOTAVAIL);
      return(FALSE);
   }

   // initialize device, disabling its h/w interrupts
   if(!Init()) return(FALSE);

   // remove any previous handler; if new handler not specified, we're done!
   ClearInterruptHandler(); 
   if(pIntHandler == NULL) return(TRUE); 

   // fill out the information required to attach a line- or message-based interrupt. We always
   // choose a message-based scheme if the device supports it.
   ATTACH_INTERRUPT_PARAMETERS aip;
   PCI_SLOT_NUMBER SlotNumber;
   SlotNumber.u.AsULONG = m_ulPciSlot;
   if(NotMsiMsixCapable == RtQueryPciMsiCapability((ULONG) m_nPciBus, SlotNumber))
   {
      // check IRQ line assigned to the device. Cannot use IRQ0.
      ULONG irqL = (ULONG) (m_pciInfo.u.type0.InterruptLine); 
      if(irqL == 0) 
      {
         SetDeviceError(EMSG_IRQ_INVALID);
         return(FALSE);
      }

      aip.AttachVersion = ATTACH_LINE_BASED;
      aip.LineBased.pThreadAttributes = NULL;
      aip.LineBased.StackSize = (ULONG) 0;
      aip.LineBased.pRoutine = pIntHandler;
      aip.LineBased.Context = pContext;
      aip.LineBased.Priority = RT_PRIORITY_MAX;
      aip.LineBased.InterfaceType = PCIBus;
      aip.LineBased.BusNumber = (ULONG) m_nPciBus;
      aip.LineBased.SlotNumber = SlotNumber;
      aip.LineBased.BusInterruptLevel = irqL;
      aip.LineBased.BusInterruptVector = irqL;
      aip.LineBased.Shared = (BOOLEAN) FALSE;
      aip.LineBased.InterruptMode = LevelSensitive;
      aip.LineBased.MyInterrupt = NULL;
      aip.LineBased.ProcessorEnableMask = (1i64 << RtGetCurrentProcessorNumber());
   }
   else
   {
      aip.AttachVersion = ATTACH_MESSAGE_BASED;
	  aip.MessageBased.pThreadAttributes = NULL;
      aip.MessageBased.StackSize = (ULONG) 0;
      aip.MessageBased.pRoutine = pIntHandler;
      aip.MessageBased.Context = pContext;
      aip.MessageBased.Priority = RT_PRIORITY_MAX;
      aip.MessageBased.BusNumber = (ULONG) m_nPciBus;
      aip.MessageBased.SlotNumber = SlotNumber;
      aip.MessageBased.MyInterrupt = NULL;
      aip.MessageBased.ProcessorEnableMask = (1i64 << RtGetCurrentProcessorNumber());
   }

   // attach the ISR to the device interrupt resource
   m_hInterrupt = ::RtAttachInterrupt(&aip); 
   if(m_hInterrupt == NULL) SetDeviceError(EMSG_IRQ_GENFAIL);

   return(BOOL(m_hInterrupt != NULL));
}


//=== ClearInterruptHandler =========================================================================================== 
//
//    Detach the device's interrupt handler routine (if any). 
//
//    This routine will call Init() to reinitialize device and disable its hardware interrupts.  Once called, hardware 
//    interrupts should remain disabled until a new interrupt handler is installed, or the system could freeze.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CDevice::ClearInterruptHandler()
{
   if( m_hInterrupt != NULL ) 
   {
      Init();
      RtDisableInterrupts();     // note: Get compile error if I preface this with global namespace prefix '::'
      RtReleaseInterrupt( m_hInterrupt );
      m_hInterrupt = NULL;
      RtEnableInterrupts();
   }
}


//=== Get/SetInstallPath (static) ===================================================================================== 
//
//    These static methods get/set the installation directory (for configuration and program files) shared by all 
//    device objects in CXDRIVER.  The install path is set by the device manager.  SetInstallPath() ensures that the 
//    path provided does NOT end with the path separator '\'.
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE.
//
LPCSTR RTFCNDCL CDevice::GetInstallPath() 
{
   return( (LPCSTR) CDevice::M_INSTALLPATH );
}

VOID RTFCNDCL CDevice::SetInstallPath( LPCSTR strPath )
{
   ::strcpy_s( CDevice::M_INSTALLPATH, strPath );
   size_t last = ::strlen( CDevice::M_INSTALLPATH ) - 1;
   if(last >= 0 && (CDevice::M_INSTALLPATH[last] == '\\'))
      CDevice::M_INSTALLPATH[last] = '\0';
}



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== FindPCIDev ====================================================================================================== 
//
//    Search PCI subsystem for the appropriate instance of the physical device represented by this device object.
//
//    Each device is identified by a vendor and device ID, and optionally, a subvendor and subsystem ID.  These IDs are 
//    included in the device object's information record, provided at construction time.  Also set at construction time 
//    is an "instance" number, which allows us to support multiple instances of a given device.  The first device found 
//    is instance 1, and so on.  [DEVNOTE:  This scheme assumes that every invocation of this method will always search 
//    through the devices in the same order!]
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if device was found and PCI config info was successfully retrieved; FALSE otherwise.
//
BOOL RTFCNDCL CDevice::FindPCIDev()
{
   PCI_SLOT_NUMBER SlotNumber;                                                         // PCI slot number -- used to 
   SlotNumber.u.AsULONG = 0;                                                           // retrieve PCI config info

   BOOL bContinue = TRUE;                                                              // keep searching until no buses 
   BOOL bFound = FALSE;                                                                // left, or specified dev found

   PPCI_COMMON_CONFIG pPCI = &m_pciInfo;                                               // save PCI cfg info internally

   int nFound = 0;                                                                     // #devices found so far

   int bus, dev, f;
   for( bus = 0; bContinue; bus++ )
   { 
      for( dev = 0; (dev < PCI_MAX_DEVICES) && bContinue; dev++ )
      { 
         SlotNumber.u.bits.DeviceNumber = (ULONG) dev; 
         for( f = 0; (f < PCI_MAX_FUNCTION) && bContinue; f++ )
         { 
            SlotNumber.u.bits.FunctionNumber = (ULONG) f; 
            if( 0 == ::RtGetBusDataByOffset( PCIConfiguration, (ULONG) bus,            // retrieve PCI config space 
                  SlotNumber.u.AsULONG, pPCI, 0, PCI_COMMON_HDR_LENGTH ) )             // info for given slot #
               bContinue = FALSE;                                                      //    ...out of buses!!
            else if( (pPCI->VendorID == m_devInfo.vendorID) &&                         //    ...found a device
                     (pPCI->DeviceID == m_devInfo.deviceID) &&
                     ((m_devInfo.subVendorID == 0) || (pPCI->u.type0.SubVendorID == m_devInfo.subVendorID)) &&
                     ((m_devInfo.subSystemID == 0) || (pPCI->u.type0.SubSystemID == m_devInfo.subSystemID))
                   )
            {
               ++nFound;
               if( m_devNumber == nFound )                                             // if it's the right instance, 
               {                                                                       // we're done; save bus, slot #s 
                  bFound = TRUE;
                  bContinue = FALSE;
                  m_nPciBus = bus; 
                  m_ulPciSlot = SlotNumber.u.AsULONG;
               }
            }
         }
      }
   }

   if( !bFound )                                                                       // clear bus# and config info if 
   {                                                                                   // device was not found. 
      m_nPciBus = -1;
      m_ulPciSlot = 0;
      memset( pPCI, 0, sizeof(PCI_COMMON_HDR_LENGTH) );
   }
   return( bFound );
}


//=== ClearPCICfgInfo ================================================================================================= 
//
//    Clear the PCI configuration info saved by a previous call to FindPCIDev(), returning CDevice object to its "no 
//    device found" state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE
//
VOID RTFCNDCL CDevice::ClearPCICfgInfo() 
{
   m_nPciBus = -1;
   m_ulPciSlot = 0;
   memset( (PVOID) &m_pciInfo, 0, sizeof(PCI_COMMON_CONFIG) );
}


//=== BEGIN:  COFF (common object file format) information. =========================================================== 
// 
// BACKGROUND:  The DSP boards we support thus far use TI processors.  DSP core programs that run on these procs are 
// stored as COFF files on hard disk; the COFF file is parsed into sections and downloaded to the DSP board.  In this  
// section we define the constants & data structures required to successfully translate the COFF file and download the 
// executable code. Bear in mind that we have NOT striven for a general implementation!  We only define the flags and 
// data structures that we actually use in the LoadTIDeviceCOFF() routine.  Also, this routine expectd COFF version 2 
// files; COFF 0 and COFF 1 are not supported.
//
#define     C6X_MAGICNUM_FH            0x0099            // in COFF 2 file header, indicates C6x-compatible COFF
#define     C4X_MAGICNUM_FH            0x0093            // in COFF 2 file header, indicates C4x-compatible COFF

#define     COFFBUFSZ_BYTES    (768 * sizeof( ULONG ))   // must be a multiple of 12

#define     COFF2_MAGICNUM             0x00c2            // file header field:  indicates COFF version 2 file
#define     F_EXEC                     0x0002            // file header flags:  COFF file is executable
#define     F_LITTLE                   0x0100            //    DSP target is a little-endian device
#define     F_BIG                      0x0200            //    DSP target is a big-endian device
#define     F_VERS1                    0x0010            //    (C4x only) contains TMS320C40/44 code
#define     COFFOPTFH_MAGICNUM         0x0108            // opt file hdr field: "magic number" for DOS-based COFFs
#define     STYP_REG                   0x0000            // sect hdr flags: regular section (downloaded)
#define     STYP_DSECT                 0x0001            //    dummy section (not downloaded)
#define     STYP_NOLOAD                0x0002            //    no-load section (not downloaded)
#define     STYP_GROUP                 0x0004            //    grouped section 
#define     STYP_PAD                   0x0008            //    padding section (downloaded)
#define     STYP_COPY                  0x0010            //    copy section (downloaded)
#define     STYP_TEXT                  0x0020            //    section contains executable code
#define     STYP_DATA                  0x0040            //    section contains initialized data
#define     STYP_BSS                   0x0080            //    section contains uninitialized data
#define     STYP_ALIGN                 0x0700            //    (C4x only) section is aligned on page boundary
#define     STYP_CLINK                 0x4000            //    (C6x only) section requires conditional linking

// COFF header structs
//
// ***IMPLEMENTATION NOTE:  the COFF file contains a number of different headers as defined by the data structures to 
// follow.  each header is read directly from the file into the appropriate data structure.  this technique relies on 
// two assumptions:  (1) that char, USHORT, LONG, and ULONG are 1-, 2-, 4-, and 4-bytes long; (2) that sizeof(header) = 
// sum of the actual sizes of the structure members.  the second assumption is not satisfied because structures are 
// padded so that they align on N-byte boundaries where N is the size of the largest member.  to get around this, i 
// have specified here the actual #bytes in each type of header.
//
#define     COFF_FILE_HDR_SZ            22               // size of file header in bytes 
#define     COFF_OPTFILE_HDR_SZ         28               // size of optional file header in bytes 
#define     COFF_SECTION_HDR_SZ         48               // size of section header in bytes 
#define     COFF_CINITSUB_HDR_SZ        8                // size of .cinit subsection header in bytes 

typedef struct tagFileHdrCOFF                            // COFF version 2 file header
   {                                                     // NOTE: OS dependent! [#] gives #bytes for each member.
   USHORT   version;                                     //    [2] COFF version = COFF2_MAGICNUM
   USHORT   nSectionHdrs;                                //    [2] number of section headers in file
   LONG     timeStamp;                                   //    [4] time/date stamp -- when file was created
   LONG     fPtrSymbols;                                 //    [4] file ptr -- symbol table's start address
   LONG     nSymbols;                                    //    [4] number entries in symbol table
   USHORT   nOptHdrSize;                                 //    [2] num bytes in optional file hdr (0 or 28)
   USHORT   flags;                                       //    [2] file header flags
   USHORT   targetID;                                    //    [2] "magic number" of target DSP device
   } COFF_FILE_HDR, *PCOFF_FILE_HDR;                                            

typedef struct tagOptFileHdrCOFF                         // COFF version 2 optional file header
   {                                                     // NOTE: OS dependent! [#] gives #bytes for each member.
   SHORT    magicnum;                                    //    [2] ID for optional header = COFFOPTFH_MAGICNUM
   SHORT    version;                                     //    [2] version stamp
   LONG     nCodeSize;                                   //    [4] size of executable code (SEE NOTE BELOW)
   LONG     nInitDataSize;                               //    [4] size of initialized data (SEE NOTE BELOW)
   LONG     nUninitDataSize;                             //    [4] size of uninitialized data (SEE NOTE BELOW)
   LONG     entryPoint;                                  //    [4] entry point (physical address??)
   LONG     addrCodeStart;                               //    [4] start addr of executable code
   LONG     addrInitDataStart;                           //    [4] start addr of initialized data
   } COFF_OPTFILE_HDR, *PCOFF_OPTFILE_HDR;

#define     C6X_DATABYTES              1                 // size of a data element in bytes for C6x-compatible COFF 
#define     C4X_DATABYTES              4                 // size of a data element in bytes for C4x-compatible COFF 
   
typedef struct tagSectionHdrCOFF                         // COFF version 2 section header
   {                                                     // NOTE: OS dependent! [#] gives #bytes for each member.
   char     name[8];                                     //    [8] section name (e.g. ".cinit") padded with '\0'
   LONG     physAddr;                                    //    [4] section's physical address
   LONG     virtualAddr;                                 //    [4] section's virtual address
   LONG     nSize;                                       //    [4] section size (SEE NOTE ABOVE)
   LONG     fPtrRaw;                                     //    [4] file pointer to raw data
   LONG     fPtrRelocEntries;                            //    [4] file pointer to relocation entries
   LONG     fPtrLineEntries;                             //    [4] file pointer to line number entries
   ULONG    nRelocEntries;                               //    [4] number of relocation entries
   ULONG    nLineEntries;                                //    [4] number of line number entries
   ULONG    flags;                                       //    [4] section header flags
   USHORT   reserved;                                    //    [2] not used
   USHORT   memPageNum;                                  //    [2] memory page number
   } COFF_SECTION_HDR, *PCOFF_SECTION_HDR;

typedef struct tagCinitSubSectHdrCOFF                    // header for subsections of a .cinit COPY section of a
   {                                                     // COFF file. [#] gives #bytes for each member.
   ULONG    nDataSize;                                   //    [4] size of subsection (SEE NOTE ABOVE about data sz)
   ULONG    bssAddr;                                     //    [4] subsection's virtual address 
   } COFF_CINITSUB_HDR, *PCOFF_CINITSUB_HDR;
//
//=== END:  COFF (common object file format) information. ============================================================= 


//=== LoadTIDeviceCOFF ================================================================================================ 
//
//    Reset TI DSP-based device, load COFF executable file into DSP program memory, and start the program.
//
//    Several CNTRLX devices use TMS320C6x or TMS320C4x digital signal processors from Texas Instruments.  Such devices 
//    require a "target" or "core" program that runs onboard the DSP device.  This method is responsible for loading 
//    and starting that program.
//
//    BACKGROUND:  
//    Programs for the TMS320C6x/C4x-series processors are stored to disk files in Common Object File Format (COFF). 
//    While there are three different COFF versions, this method only supports COFF version 2, and it expects a fully- 
//    linked, executable COFF.  A different "magic number" distinguishes C6x- from C4x-compatible COFFs.
//
//    A fully-linked, executable COFF2 file begins with a file header and optional file header, which contain info 
//    about the code/data/other sections which are to be downloaded into processor memory, configuration info, the 
//    program's entry point, etc.  For full details about COFF format as relevant to the boards we support, consult 
//    Texas Instruments' TMS320C3x/C4x Assembly Language Tools Users Guide dtd Mar 1997 (document spru035c) and the 
//    TMS320C6x Assembly Language Tools Users Guide dtd Feb1998 (document "spru186c"). 
//
//    The load process involves reading in each code/data/other section from the COFF file and downloading it to the 
//    designated region of local (processor) memory.  A "section header" tells the loader how large the section is and 
//    the local address at which the section begins.  One type of section, the ".cinit COPY" section deserves special 
//    mention.  There are two ways in which the global variables and constants of the program can be "autoinitialized": 
//    at boot time (the "ROM model") or at load time (the "RAM model").  In the first model, the .cinit sections are 
//    loaded into some area of processory memory.  After the program is started, the bootstrap code copies the values 
//    in these init tables to the memory locations where the corresponding global vars & constants are stored; then it 
//    branches to the program entry point.  In the second model, it is the loader's responsibility to load the global 
//    vars & constants with the initial values found in the .cinit sections.  This model is more efficient because it 
//    reduces boot time and saves memory that is otherwise occupied by the initialization tables.  However, it makes 
//    the loader more complex.  This loader supports both models. 
//
//    Certain tasks in the load process are unavoidably device-specific.  This method relies on a number of overridable 
//    methods to perform these tasks:
//
//       GetCOFFFilename()    ==> Returns the name of the COFF file to be downloaded onto the TI DSP.  This file is 
//                                assumed to be located in the Maestro/CXDRIVER installation directory, the path of 
//                                which is set during startup -- see CDevice::Get/SetInstallPath().
//       DeviceReset()        ==> Performs a hard reset.  This should put the DSP in a suspended state for the purpose 
//                                of downloading a COFF executable core.
//       DownloadCOFFData()   ==> Writes a section of COFF data into the TI DSP's program, data, or other memory.
//       DeviceStart()        ==> This should release the DSP from the suspended state and start execution of the core.
//
//    A derived class, of course, MUST override and properly implement each of the above methods in order for the COFF 
//    bootload to succeed.
//    
//    CREDITS:  
//    Adapted from code in the Detroit, Daytona & Dakar Development Pkgs from Spectrum Signal Processing, Inc.  Below 
//    is a list of functions upon which this method is based.  We dispense with all the overhead that SSP introduces so 
//    that their code can handle different boards and different COFF versions, as well as communicate with the kernel-
//    mode driver that interfaces with the hardware.  These driver-level functions are now handled through RTX fcns. 
//    The code is more readable, but it is also specialized to handle only fully-linked, executable COFF2 files for the 
//    C4x and C6x DSPs from TI.
//       Detroit: DE62_SystemLoad(), ProcessorLoad(), ss_COFFParse_TIC6x(), Write_COFF_Buffer().
//       Daytona: FT_SystemLoad(), ProcessorLoad(), ss_COFFParse(), ss_COFFParse_TIC6x(), Write_COFF_Buffer().
//       Dakar:   F5_SystemLoad(), F5W_MakeSystemLoad(), ss_COFFParse(), ss_COFFParse_TIC4x_Version2(),
//                Write_COFF_Buffer().
//
//    NOTES:
//    1) Not designed for time-critical code sections!!
//    2) Currently limited to COFF files with a max size of 2^32 - 1 bytes -- definitely NOT a big deal!
//    3) I'm not sure about handling of ".vect" or ".trap" sections!!
//    4) C6X_DATABYTES is the size of a fundamental data word on the C6x CPU, while C4X_DATABYTES is the equivalent for 
//       the C3x/C4x processors.  We use these to convert section length units from "#bytes" to "# of fundamental data 
//       words".
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if function successful; FALSE otherwise.  Call GetLastDeviceError() to retrieve error msg.
//
BOOL RTFCNDCL CDevice::LoadTIDeviceCOFF()
{
   BOOL              bEndianSwapHeaders,        // swap endian byte-order of COFF file header structs
                     bEndianSwapData,           // swap endian byte-order of COFF section data
                     bHostIsLittleEndian,       // host CPU uses little-endian byte-ordering
                     bTgtIsLittleEndian,        // target DSP board uses little-endian byte-ordering
                     bLoadSection,              // load current section of COFF file if TRUE
                     bIsCinitCopy,              // a .cinit COPY section, requiring special processing to load
                     bNewCinitSubSect,          // set when ready to process next subsection in a .cinit COPY section
                     bSuccess,                  // return value = TRUE if load is successful
                     bResult;                   // generic status flag returned from called routines 
   char              *lpBuffer;                 // byte-based buffer for reading & downloading COFF file data. must
                                                // be byte-based in case we must "endian-swap" that data
   DWORD             dwFPtrNextSectHdr,         // offset from start of file to the next section header
                     dwBytesToRead,
                     dwBytesRead;
   ULONG             addrOnBoard,               // local address on DSP board where section data should be downloaded
                     nSectBytesDone,            // number of bytes in section that have been processed
                     nSubSectBytesDone,         // number of bytes processed in subsection of .cinit COPY section
                     nSections;
   HANDLE            hFile;                     // handle to open COFF file to be downloaded
   COFF_FILE_HDR     fileHeader;                // COFF file header
   COFF_OPTFILE_HDR  optFileHeader;             // COFF optional file header
   COFF_SECTION_HDR  sectionHeader;             // section header for each section in COFF file
   COFF_CINITSUB_HDR cInitSubHeader;            // additional header preceding raw data for each subsection 
                                                //    within a .cinit COPY section
   int               magicNum,                  // processor-specific: "magic number" ID in COFF file header
                     nBytesPerWord;             // processor-specific: number of bytes per fundamental data word 
   char              eMsg[MAX_EMSG_SZ];         // error message
   char              coffPath[257];             // full path for COFF file = install dir\COFF filename.

   bSuccess = FALSE;  

   if( !DeviceReset() )
   {
      ::strcpy_s( eMsg, EMSG_FAILEDRESET );
      goto CLEANUP;
   }

   bEndianSwapHeaders = FALSE;
   lpBuffer = (char *) NULL;
   hFile = (HANDLE) NULL;

   lpBuffer = (char *) ::RtAllocateLockedMemory( COFFBUFSZ_BYTES );           // alloc buf for reading data from file 
   if( lpBuffer == (char *)NULL )
   {
      ::strcpy_s( eMsg, EMSG_OUTOFMEMORY );
      goto CLEANUP;
   }

   magicNum = IsTIC6x() ? C6X_MAGICNUM_FH : C4X_MAGICNUM_FH;                  // distinguish C6x from C4x DSP
   nBytesPerWord = IsTIC6x() ? C6X_DATABYTES : C4X_DATABYTES;

   ::sprintf_s( coffPath, "%s\\%s",                                           // construct full pathname for COFF file 
              CDevice::GetInstallPath(), GetCOFFFilename() );

   hFile = ::CreateFile( coffPath,                                            // open COFF file for this device
                         GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                         FILE_FLAG_RANDOM_ACCESS, NULL );
   if( hFile == INVALID_HANDLE_VALUE )
   {
      
      ::sprintf_s( eMsg, "Cannot find/open %.*s", MAX_EMSG_SZ-18, coffPath );
      goto CLEANUP;
   }

   // read in & analyze the file header.  abort if COFF file is not the right format:
   //    (1) check that version # corresponds to COFF version 2.  we must check both possible byte orders in case the 
   //        COFF file was created on an alternate-endian system.  if that's the case, perform an endian-swap on the 
   //        file header before continuing.
   //    (2) check that target DSP's "magic number" is appropriate to the board being used  
   //    (3) COFF file must include the optional file header; else it is not fully linked
   //    (4) the F_EXEC file header flag must be set, indicating that this is an executable COFF file
   //
   bResult = ::ReadFile( hFile, (LPVOID) &fileHeader,                         // read in & analyze file header; abort 
                  (DWORD) COFF_FILE_HDR_SZ, &dwBytesRead, NULL );             // if COFF file is not right format...
   if( (!bResult) || (dwBytesRead < (DWORD) COFF_FILE_HDR_SZ) )               //    file read failure
   {
      ::strcpy_s( eMsg, EMSG_COFFREAD );
      goto CLEANUP;
   }
   if( fileHeader.version != (USHORT) COFF2_MAGICNUM )                        //    check for COFF v2; check both byte 
   {                                                                          //    orders in case file was created on 
      EndianSwap16( (WORD*) (&(fileHeader.version)), 2 );                     //    an alternate-endian system.
      EndianSwap32( (DWORD*)  (&(fileHeader.timeStamp)), 3 );
      EndianSwap16( (WORD*) (&(fileHeader.nOptHdrSize)), 3 );
      if( fileHeader.version != (USHORT) COFF2_MAGICNUM )
      {
         ::sprintf_s( eMsg, "COFF version (0x%04x) not supported", fileHeader.version );
         goto CLEANUP;
      }
      else bEndianSwapHeaders = TRUE;
   }
   if( fileHeader.targetID != (USHORT) magicNum )                             //    verify target DSP's "magic number" 
   {
      ::sprintf_s( eMsg, "COFF not compatible with this device (tgtID=0x%04x)", fileHeader.targetID );
      goto CLEANUP;
   }   
   if( fileHeader.nOptHdrSize != (USHORT) COFF_OPTFILE_HDR_SZ )               //    COFF must include optional file 
   {                                                                          //    header; else not fully linked
      ::strcpy_s( eMsg, "COFF file not fully linked" );
      goto CLEANUP;
   }
   if( !(fileHeader.flags & F_EXEC) )                                         //    must be an executable COFF file
   {
      ::strcpy_s( eMsg, "Not an executable COFF file" );
      goto CLEANUP;
   }

   addrOnBoard = 0x00000001L;                                                 // determine host endianness
   bHostIsLittleEndian = ( (*((unsigned char *)(&addrOnBoard))) == 0x01 );
   bTgtIsLittleEndian = ((fileHeader.flags & F_LITTLE) != 0);                 // file hdr indicates tgt endianness
   bEndianSwapData = (bHostIsLittleEndian && (!bTgtIsLittleEndian)) ||        // do we need to swap section data?
                        ((!bHostIsLittleEndian) && bTgtIsLittleEndian);

   bResult = ::ReadFile( hFile, (LPVOID) &optFileHeader,                      // read in optional file header
                  (DWORD) COFF_OPTFILE_HDR_SZ, &dwBytesRead, NULL );
   if( (!bResult) || (dwBytesRead < (DWORD) COFF_OPTFILE_HDR_SZ) )
   {
      ::strcpy_s( eMsg, EMSG_COFFREAD );
      goto CLEANUP;
   }
   if( bEndianSwapHeaders )                                                   // swap byte-order if necessary
   {
      EndianSwap16( (WORD*) (&(optFileHeader.magicnum)), 2 );
      EndianSwap32( (DWORD*)  (&(optFileHeader.nCodeSize)), 6 );
   }
   if( optFileHeader.magicnum != (SHORT) COFFOPTFH_MAGICNUM )                 // abort if opt file hdr magic number is 
   {                                                                          // not correct
      ::sprintf_s( eMsg, "Bad optional file header (magic#=0x%04x)!!", optFileHeader.magicnum );
      goto CLEANUP;
   }


   // BEGIN-- DOWNLOAD ALL SECTIONS.  Download all loadable sections of the COFF file.  For each section...
   //    (1) read in next section header; swap byte-order if required.
   //    (2) check section header flags to see if the section should be loaded.
   //        (a) YES.  move file pointer to start of current section's raw data.
   //        (b) NO.  skip to (4)
   //    (3) read in raw data & write it to board using allocated buffer.  NOTE:  The ".cinit" section contains 
   //        initialization data for global variables used in the DSP core program.  These variables can be 
   //        initialized at boot time (STYP_COPY not set) or at load time (STYP_COPY set).  In the latter case, we
   //        must load each .cinit subsection into the DSP board memory locations indicated in the header for
   //        that subsection.  See details below.
   //    (4) move file pointer to start of next section header.  back to (1).
   //

   dwFPtrNextSectHdr = (DWORD) COFF_FILE_HDR_SZ + COFF_OPTFILE_HDR_SZ;        // points to start of first section 
   for( nSections = 0; nSections < fileHeader.nSectionHdrs; nSections++ )     // for each section in file...
   {
      bResult = ::ReadFile( hFile, (LPVOID) &sectionHeader,                   //    read in new section header
                  (DWORD) COFF_SECTION_HDR_SZ, &dwBytesRead, NULL );
      if( (!bResult) || (dwBytesRead < (DWORD) COFF_SECTION_HDR_SZ) )
      {
         ::strcpy_s( eMsg, EMSG_COFFREAD );
         goto CLEANUP;
      }
      if( bEndianSwapHeaders )                                                //    swap byte-order if necessary
      {
         // member "name" not converted since it's a byte (char) array
         EndianSwap32( (DWORD*)  (&(sectionHeader.physAddr)), 9 );
         EndianSwap16( (WORD*) (&(sectionHeader.reserved)), 2 );
      }
      dwFPtrNextSectHdr += (DWORD) COFF_SECTION_HDR_SZ;                       //    s/w copy of file ptr now points 
                                                                              //    to next section header

                                                                              //    should this section be downloaded? 
      bLoadSection = !( (sectionHeader.flags & STYP_NOLOAD) ||                //    never load NOLOAD sections
                        (sectionHeader.flags & STYP_DSECT)  ||                //    nor dummy sections
                        (sectionHeader.nSize == 0L) );                        //    nor zero-length sections
      bLoadSection = bLoadSection &&
                     (  (sectionHeader.flags == STYP_REG) ||                  //    always load REGular sections
                        (sectionHeader.flags & STYP_DATA) ||                  //    or initialized data sections
                        (sectionHeader.flags & STYP_TEXT) ||                  //    or executable code sections
                        (sectionHeader.flags & STYP_PAD)  ||                  //    or padding sections.
                        ((sectionHeader.flags & STYP_COPY) &&                 //    load COPY sections unless no other 
                         (sectionHeader.flags ^ STYP_COPY)) );                //    flags are set for the section (???) 

      bIsCinitCopy = bLoadSection &&                                          //    is this a special ".cinit" COPY 
                     (sectionHeader.flags & STYP_COPY) &&                     //    section?
                     (strncmp( sectionHeader.name, ".cinit", 6 ) == 0);

      if( !bLoadSection )                                                     //    if NOT loading section, skip to 
         continue;                                                            //    next section header


                                                                              //    --BEGIN-- LOAD SECTION.
      if( ::SetFilePointer( hFile, (LONG)sectionHeader.fPtrRaw,               //    move file ptr to start of section's 
                           NULL, FILE_BEGIN ) == (DWORD) 0xFFFFFFFF )         //    raw data
      {
         ::strcpy_s( eMsg, EMSG_COFFSEEK );
         goto CLEANUP;
      }

      sectionHeader.nSize *= nBytesPerWord;                                   //    calc section size in bytes
      
      if( bIsCinitCopy ) bNewCinitSubSect = TRUE;
      addrOnBoard = (ULONG) sectionHeader.virtualAddr;                        //    where to download data on dev mem 
      nSectBytesDone = 0;
      while( (LONG)nSectBytesDone < sectionHeader.nSize )                     //    download one chunk at a time....
      {
         if( bIsCinitCopy && bNewCinitSubSect )                               //    at start of a new subsection of a 
         {                                                                    //    .cinit COPY section:
            bResult = ReadFile( hFile, (LPVOID) &cInitSubHeader,              //       read in subsection header
                     (DWORD) COFF_CINITSUB_HDR_SZ, &dwBytesRead, NULL );
            if( (!bResult) || (dwBytesRead < (DWORD) COFF_CINITSUB_HDR_SZ) )
            {
               ::strcpy_s( eMsg, EMSG_COFFREAD );
               goto CLEANUP;
            }
            nSectBytesDone += COFF_CINITSUB_HDR_SZ;

            if( bEndianSwapData )                                             //       endian-swap header if necessary 
               EndianSwap32( (DWORD*) (&(cInitSubHeader.nDataSize)), 2 );

            cInitSubHeader.nDataSize *= nBytesPerWord;                        //       subsection size in bytes 
            if( cInitSubHeader.nDataSize == 0 )                               //       ignore zero-length subsections 
               continue;
            if( sectionHeader.nSize <                                         //       subsection too big (BUG); ignore 
                     (LONG)(cInitSubHeader.nDataSize + nSectBytesDone) )      //       the rest of the section 
            {
               nSectBytesDone = (ULONG) sectionHeader.nSize;
               continue;
            }

            bNewCinitSubSect = FALSE;
            nSubSectBytesDone = 0;
            addrOnBoard = cInitSubHeader.bssAddr;
         }


         if( bIsCinitCopy )                                                   //    read in chunk of section data
            dwBytesToRead = (DWORD) (cInitSubHeader.nDataSize - nSubSectBytesDone);
         else
            dwBytesToRead = (DWORD) (sectionHeader.nSize - nSectBytesDone);
         if( dwBytesToRead > (DWORD)COFFBUFSZ_BYTES )
            dwBytesToRead = (DWORD)COFFBUFSZ_BYTES;
         bResult = ::ReadFile( hFile, (LPVOID) lpBuffer, dwBytesToRead, &dwBytesRead, NULL );
         if( (!bResult) || (dwBytesRead < dwBytesToRead) )
         {
            ::strcpy_s( eMsg, EMSG_COFFREAD );
            goto CLEANUP;
         }
         if( bEndianSwapData )                                                //    endian-swap data if necessary 
            EndianSwap32( (DWORD*) lpBuffer, int( dwBytesToRead>>2 ) );       //    (#bytes read should be mult of 4) 

         if( !DownloadCOFFData( (DWORD)addrOnBoard, (PDWORD)lpBuffer,         //    download chunk to the device as  
                                int(dwBytesToRead >> 2) ) )                   //    32-bit words
         {
            ::strcpy_s( eMsg, EMSG_COFFWRITESECT );
            goto CLEANUP;
         }

         if( bIsCinitCopy )                                                   //    update progress vars; note that 
         {                                                                    //    target (on device) address is 
            nSubSectBytesDone += (ULONG) dwBytesToRead;                       //    incremented in units of fundamental 
            if( nSubSectBytesDone >= cInitSubHeader.nDataSize )               //    word size (not necessarily = byte). 
               bNewCinitSubSect = TRUE;
         }
         nSectBytesDone += (ULONG) dwBytesToRead;
         addrOnBoard += (ULONG) (dwBytesToRead / nBytesPerWord);
      }

      if( ::SetFilePointer(hFile, (LONG)dwFPtrNextSectHdr, NULL, FILE_BEGIN)  //    restore file ptr to start of next 
               == (DWORD) 0xFFFFFFFF )                                        //    section's header 
      {
         ::strcpy_s( eMsg, EMSG_COFFSEEK );
         goto CLEANUP;
      }                                                                       //    --END-- LOAD SECTION.

      Sleep( 10 );                                                            //    sleep briefly 

   }
   //
   // END-- DOWNLOAD ALL SECTIONS 


   if( !DeviceStart( (DWORD) optFileHeader.entryPoint ) )                     // start execution of DSP core
   { 
      ::strcpy_s( eMsg, EMSG_DEVSTART );
      goto CLEANUP;
   }

   bSuccess = TRUE;

CLEANUP:
   if( hFile != (HANDLE) NULL ) ::CloseHandle( hFile );
   if( lpBuffer != (char *) NULL ) ::RtFreeLockedMemory( (PVOID)lpBuffer );
   if( !bSuccess ) SetDeviceError( eMsg );

   return( bSuccess );
}


//=== EndianSwap16, EndianSwap32 ====================================================================================== 
//
//    These utility functions are used to "endian-swap" the byte order of a contiguous sequence of 16-bit or 32-bit 
//    words:  {0x1234, 0xabcd} ==> {0x3412, 0xcdab}; and {0x01234567, 0x89abcdef} ==> {0x67452301, 0xefcdab89}.
//
//    [Adapted from code provided by Spectrum Signal Processing, Inc.]
//
//    NOTE:    -- ASSUMES that WinNT type WORD is 16-bits/2 bytes, type DWORD is 32-bits/4 bytes, and that the 
//                fundamental type char is 8 bits long.  
//
//    ARGS:       addr  -- [in/out] start address of memory region to be byte-swapped.
//                size  -- [in] number of 16-bit or 32-bit words in the memory region.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CDevice::EndianSwap16( WORD* addr, int size )
{
   for( int index = 0; index < size; index++ )
   {
      char tmp = ((char *)addr)[1];
      ((char *)addr)[1] = ((char *)addr)[0];
      ((char *)addr)[0] = tmp;

      ++addr;
   }
}                         

VOID RTFCNDCL CDevice::EndianSwap32( DWORD* addr, int size )
{
   for( int index = 0; index < size; index++ )
   {
      char tmp = ((char *)addr)[3];
      ((char *)addr)[3] = ((char *)addr)[0];
      ((char *)addr)[0] = tmp;
      tmp = ((char *)addr)[2];
      ((char *)addr)[2] = ((char *)addr)[1];
      ((char *)addr)[1] = tmp;

      ++addr;
   }
}                         



