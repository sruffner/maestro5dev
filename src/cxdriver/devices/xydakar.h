//===================================================================================================================== 
//
// xydakar.h : Declaration of class CXYDakar.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//  
//
//===================================================================================================================== 

#if !defined(XYDAKAR_H__INCLUDED_)
#define XYDAKAR_H__INCLUDED_

#include "cxscope.h"             // CCxScope -- abstraction of CXDRIVER XY scope controller functionality


//===================================================================================================================== 
// Declaration of class CXYDakar
//===================================================================================================================== 
//
class CXYDakar : public CCxScope
{
//===================================================================================================================== 
// CONSTANTS & DEFINITIONS
//===================================================================================================================== 
private:
   static const CDevice::DevInfo DEVINFO;          // device identification info
   static LPCTSTR XYCORE_FILE;                     // name of Dakar-specific XYCORE executable

   static const WORD PLX_VID;                      // PLX Technologies' vendor ID code
   static const WORD PCI9060_ID;                   // PCI dev ID for the PLX PCI9060 interface chip

                                                   // constants for host access to selected Dakar resources
   static const DWORD PCIREGSZ;                    //    #bytes in PCI9060 interface chip's internal register bank
   static const int PCIMBOX2;                      //    PCI Runtime Mailbox reg #2 (index into DWORD array)
   static const int PCICNTRL;                      //    PCI Control reg 
                                                   // selected bit masks for in PCI Control register:
   static const DWORD PCICNTRL_SWRESET;            //    set bit to reset Dakar; clear it to release from reset state 
   static const DWORD PCICNTRL_RELCFG;             //    0->1 bit transition reloads PCI cfg regs from onboard EEPROM  

   static const DWORD BOOTAREAOFFSET;              //    byte offset from BAR2 to start of Dakar's "Far Global SRAM", 
                                                   //    the first portion of which is dedicated as a bootload area 
   static const DWORD BOOTAREASZ;                  //    #bytes in memory space 0 reserved for FGSRAM bootload area
   static const DWORD FGSRAMOFFSET;                //    byte offset from BAR2 to start of general-use FGSRAM
   
   static const DWORD INTREGOFFSET;                //    byte offset from BAR2 to start of Dakar's Interrupt Ctrl regs
   static const DWORD INTREGSZ;                    //    #bytes in memory space 0 reserved for Interrupt Control regs 
   static const int INTPCI2A;                      //    "PCI to Node A IRQ" Int Control reg (index into DWORD array)

                                                   // constants related to bootloader for Dakar's root node (a C44 DSP) 
   static const DWORD C4X_SRAM_ADDR;               //    C4x-local addr for the start of FGSRAM
   static const DWORD C4X_USRENTRY_ADDR;           //    C4x-local addr where bootloader places user code entry point 
   static const int BOOT_DMADONE;                  //    DWORD locs in FGSRAM bootload area: bootloader DMA done flag, 
   static const int BOOT_DMAHDR;                   //       start of bootloader DMA autoinit header
   static const int BOOT_CTLBLK;                   //       start of data block for bootloader DMA 
   static const int BOOT_CTLBLK_SZ;                //       size of bootloader DMA data block (# of DWORDs)
   static const DWORD BOOT_DMAHDR_CTL;             //       DMA control reg value when downloading COFF section data
   static const DWORD BOOT_DMAHDR_LAST;            //       DMA control reg value when loading user code entry point
   static const double BOOT_TIMEOUTUS;             //    max time allowed for one DMA block transfer (in us)

   static const DWORD XYCORE_READY;                // command/status register values for communication w/XYCORE
   static const DWORD XYCORE_INIT;
   static const DWORD XYCORE_DOFRAME;

   static const int PARAMS_SIZE32;                 // #DWORDs in CCxScope::Parameters if all members were DWORDs


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   volatile PULONG      m_pvulCmdStatReg;          // virtual memory-mapped access to:  XYCORE cmd/status register,
   volatile PULONG      m_pvulPCIRegs;             //    PCI local configuration & runtime registers,
   volatile PULONG      m_pvulBootLoad;            //    portion of "Far Global SRAM" dedicated to bootload area
   volatile PULONG      m_pvulFgsram;              //    general-purpose FGSRAM (for loading tgt params & update recs) 
   volatile PULONG      m_pvulIntRegs;             //    Node A Interrupt Control register bank


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CXYDakar( const CXYDakar& src );                      // no copy constructor or assignment operator defined
   CXYDakar& operator=( const CXYDakar& src ); 

public: 
   CXYDakar( int iDevNum );                              // constructor 
   ~CXYDakar() {}                                        // destructor 


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   virtual LPCTSTR RTFCNDCL GetDeviceName()              // [CDevice override] a short device name
   {
      return( "Dakar F5" );
   }


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   BOOL RTFCNDCL MapDeviceResources();                   // map device memory or I/O space to process space
   VOID RTFCNDCL UnmapDeviceResources();                 // unmap device resources
   BOOL RTFCNDCL OnOpen();                               // device-specific work when opening connection to dev 

   LPCTSTR RTFCNDCL GetCOFFFilename()                    // return name of the COFF target executable file 
   {
      return( CXYDakar::XYCORE_FILE );
   }
   BOOL RTFCNDCL DownloadCOFFData(                       // download a section of COFF file onto TI DSP
      DWORD devAddr, PDWORD pData, int iLen ); 
   BOOL RTFCNDCL DeviceReset();                          // perform a "hard reset" of the device
   BOOL RTFCNDCL DeviceStart( DWORD dwEntry );           // start execution of COFF executable
   VOID RTFCNDCL DeviceQuit();                           // stop execution of COFF executable

   BOOL RTFCNDCL LoadParameters();                       // download tgt defns & animation parameters to XY scope dev
   BOOL RTFCNDCL DoFrame();                              // download tgt update records & initiate display frame update 

private:
   BOOL RTFCNDCL IsReady()                               // is XY scope dev ready for next command?
   {
      return( BOOL((m_pvulCmdStatReg != NULL) && (*m_pvulCmdStatReg == XYCORE_READY)) );
   }
};



#endif   // !defined(XYDAKAR_H__INCLUDED_)

