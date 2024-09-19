//===================================================================================================================== 
//
// device.h : Declaration of ABSTRACT class CDevice. 
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(DEVICE_H__INCLUDED_)
#define DEVICE_H__INCLUDED_

#include <windows.h>             // standard Win32 includes 
#include "string.h"
#include "stdio.h"
#include "rtapi.h"               // the RTX API 
#include "RtssApi.h"             // RTSS-only API

//===================================================================================================================== 
// Declaration of ABSTRACT class CDevice
//===================================================================================================================== 
//
class CDevice
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int MAX_EMSG_SZ = 100;       // max # characters in the device error message string

protected:
   static LPCSTR EMSG_CANTFIND;              // error strings:  unable to find a supported device (PCI)
   static LPCSTR EMSG_PARENTDEVUNAVAIL;      //    can't open subdevice if parent device is not available
   static LPCSTR EMSG_MAPADDRFAIL;           //    can't remap phys dev addr or I/O space into process memory
   static LPCSTR EMSG_VERIFYFAIL;            //    sanity check on device failed
   static LPCSTR EMSG_DEVNOTAVAIL;           //    device is not available
   static LPCSTR EMSG_NOTSUPPORTED;          //    last invoked method not supported by this device
   static LPCSTR EMSG_IRQ_INVALID;           //    IRQ line invalid
   static LPCSTR EMSG_IRQ_NOEXCLUSIVE;       //    cannot get exclusive access to IRQ line
   static LPCSTR EMSG_IRQ_GENFAIL;           //    unknown error occurred; unable to attach interrupt vector
   static LPCSTR EMSG_FAILEDRESET;           //    device reset failed
   static LPCSTR EMSG_OUTOFMEMORY;           //    low system memory caused failure
   static LPCSTR EMSG_COFFREAD;              //    error reading COFF file
   static LPCSTR EMSG_COFFSEEK;              //    error moving file ptr in COFF file
   static LPCSTR EMSG_COFFWRITESECT;         //    error writing a section of COFF data to the device's DSP
   static LPCSTR EMSG_DEVSTART;              //    device core start failed
   static LPCSTR EMSG_DEVTIMEOUT;            //    device not responding 
   static LPCSTR EMSG_USAGE;                 //    general usage error (invalid parameters, etc.)
   static LPCSTR EMSG_UNKNOWN;               //    unknown device error 


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
public:
   static const WORD DF_PCI = ((WORD)(1 << 0));    // flag set if device hosted on PCI bus (else ISA assumed)
   static const WORD DF_TIC6x = ((WORD)(1 << 1));  // flag set if device uses a Texas Instruments' TMS320C6x-series DSP
   static const WORD DF_TIC4x = ((WORD)(1 << 2));  // flag set if device uses a TI TMS320C4x-series DSP

   struct DevInfo                            // device identification information
   {
      WORD flags;                            // device flags; see DF_* constants above
      WORD vendorID;                         // vendor ID (needed to locate PCI-based devices)
      WORD deviceID;                         // device ID (ditto)
      WORD subVendorID, subSystemID;         // (optional) additional info to identify device (0 if not used)
   };

private:
   DevInfo           m_devInfo;              // identification info for the device.
   int               m_devNumber;            // cardinal instance of dev in host system (in case of multiple devices)

   BOOL              m_bInUse;               // hardware located & comm link established; functionality enabled!
   int               m_nPciBus;              // PCI bus # for found device (ignored for ISA). 
   ULONG             m_ulPciSlot;            // PCI slot # for found device (ignored for ISA).
   PCI_COMMON_CONFIG m_pciInfo;              // PCI config info for found device (ignored for ISA).

   HANDLE            m_hInterrupt;           // handle to RTX-specific interrupt routine, if one exists (PCI dev only)
   char              m_errMsg[MAX_EMSG_SZ];  // last device error message

   static char       M_INSTALLPATH[257];     // full path to installation directory COMMON TO ALL CXDRIVER DEVICES


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

public: 
   CDevice( const DevInfo& devInfo, int devNum );        // constructor 
   virtual ~CDevice();                                   // destructor 


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   // a "subdevice" is a hardware device function implemented on a multi-function parent device
   virtual BOOL RTFCNDCL IsSubDevice() { return(FALSE); }
   virtual CDevice* RTFCNDCL GetParentDevice() { return(NULL); }
   
   // returns TRUE if device is available for use. In the case of a subdevice, the parent device object must be on AND
   // the subdevice object must be available for use.
   BOOL RTFCNDCL IsOn()
   { 
      if(IsSubDevice())
      {
         CDevice* pParent = GetParentDevice();
         return(pParent != NULL && pParent->IsOn() && m_bInUse);
      }
      return( m_bInUse ); 
   }
   
   BOOL RTFCNDCL IsPci() const                           // TRUE if device object represents a device on PCI bus
   {
      return( BOOL((m_devInfo.flags & DF_PCI) != 0) );
   }
   BOOL RTFCNDCL IsIsa() const { return(!IsPci()); }     // TRUE if device object represents a device on ISA bus
   BOOL RTFCNDCL IsTIDsp() const                         // TRUE if dev obj represents a TI DSP device
   {
      return( BOOL((m_devInfo.flags & (DF_TIC6x|DF_TIC4x)) != 0) );
   }
   BOOL RTFCNDCL IsTIC6x() const                         // TRUE if dev obj represents a TI TMS320C6x-series DSP dev
   {
      return( BOOL((m_devInfo.flags & DF_TIC6x) != 0) );
   }

   int RTFCNDCL GetPciBus() const                        // retrieve PCI bus # (returns -1 if ISA or no device found)
   {
      return( IsPci() ? m_nPciBus : -1 );
   }
   ULONG RTFCNDCL GetPciSlot() const                     // retrieve PCI slot # (returns 0 if ISA or no device found)
   {
      return( IsPci() ? m_ulPciSlot : 0 );
   }
   DWORD RTFCNDCL GetPciBaseAddressReg(int iReg) const   // retrieve a base addr reg from PCI config info
   {
      if( iReg >= 0 && iReg < 4 ) return( m_pciInfo.u.type0.BaseAddresses[iReg] );
      else return( 0 );
   }
   WORD RTFCNDCL GetSubVendor() const                    // retrieve subvendor ID for this PCI device
   { 
      return( IsPci() ? m_pciInfo.u.type0.SubVendorID : 0 );
   }
   WORD RTFCNDCL GetSubSystem() const                    // retrieve subsystem ID for this PCI device
   { 
      return( IsPci() ? m_pciInfo.u.type0.SubSystemID : 0 );
   }
   BOOL RTFCNDCL GetPciConfig(PCI_COMMON_CONFIG* pCfg) const  // retrieve the entire PCI config structure
   {
      if( IsPci() ) *pCfg = m_pciInfo;
      return( IsPci() );
   }
   BOOL RTFCNDCL HasInterruptHandler() const             // TRUE if an ISR is currently attached to dev interrupts
   {
      return( BOOL(m_hInterrupt != NULL) );
   }
   LPCSTR RTFCNDCL GetLastDeviceError() const            // msg describing last device error (empty string if no error)
   {
      return( (LPCSTR) m_errMsg );
   }

   virtual LPCSTR RTFCNDCL GetDeviceName()               // a short device name
   {
      return( "UnknownDev" );
   }


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:
   BOOL RTFCNDCL Open();                                 // establish comm link to device; TRUE if successful 
   VOID RTFCNDCL Close();                                // sever connection to device and clean up
   virtual BOOL RTFCNDCL Init() = 0;                     // init device to a suitable idle state; h/w int's disabled

   // attach/detach handler routine to/from device interrupt (PCI/PCIExpress-based device only!). Note that both 
   // routines invoke Init() to disable device interrupts!
   BOOL RTFCNDCL SetInterruptHandler(BOOLEAN (RTFCNDCL *pIntHandler)(PVOID context), PVOID pContext); 
   VOID RTFCNDCL ClearInterruptHandler();

   static LPCSTR RTFCNDCL GetInstallPath();              // get/set installation path shared by all CXDRIVER devices
   static VOID RTFCNDCL SetInstallPath(LPCSTR strPath);


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   VOID RTFCNDCL SetOn( BOOL b=TRUE ) { m_bInUse = b; }  // activate/deactivate the device interface

   VOID RTFCNDCL SetDeviceError( LPCSTR errMsg )         // set, clear the "last device error" message
   {
      if( errMsg == NULL ) m_errMsg[0] = '\0';
      else ::sprintf_s(m_errMsg, "%.*s", MAX_EMSG_SZ-1, errMsg);
   }
   VOID RTFCNDCL ClearDeviceError() { m_errMsg[0] = '\0'; }

   BOOL RTFCNDCL FindPCIDev();                           // locate PCI device and, if found, save its config info 
   VOID RTFCNDCL ClearPCICfgInfo();                      // clear PCI config info ("device not found" state)

   virtual BOOL RTFCNDCL MapDeviceResources() = 0;       // map device memory or I/O space to process space
   virtual VOID RTFCNDCL UnmapDeviceResources() = 0;     // unmap device resources
   virtual BOOL RTFCNDCL DeviceReset() {return( TRUE );} // perform a "hard reset" of the device
   virtual BOOL RTFCNDCL OnOpen() { return( TRUE ); }    // device-specific work when opening connection to dev 
   virtual VOID RTFCNDCL OnClose() {}                    // device-specific work here closing connection to dev

   BOOL RTFCNDCL LoadTIDeviceCOFF();                     // hard-reset TI DSP-based dev and bootload COFF target
   VOID RTFCNDCL EndianSwap16( PWORD addr, int size );   // endian-swapping utilities
   VOID RTFCNDCL EndianSwap32( PDWORD addr, int size );  //
   virtual LPCTSTR RTFCNDCL GetCOFFFilename()            // return name of the COFF target executable file 
   { return( NULL ); }
   virtual BOOL RTFCNDCL DownloadCOFFData(               // download a section of COFF file onto TI DSP
      DWORD devAddr, PDWORD pData, int iLen ) 
   { return( FALSE ); }
   virtual BOOL RTFCNDCL DeviceStart( DWORD dwEntry )    // start execution of COFF target
   { return( FALSE ); }
   virtual VOID RTFCNDCL DeviceQuit()  {}                // stop execution of COFF target

};


#endif   // !defined(DEVICE_H__INCLUDED_)

