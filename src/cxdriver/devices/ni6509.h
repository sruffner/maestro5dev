//===================================================================================================================== 
//
// ni6509.h : Declaration of class providing access to the NI PCIe-6509 static digital IO board. 
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(NI6509_H__INCLUDED_)
#define NI6509_H__INCLUDED_

#include "ni6363types.h"            // storage-size-explicit types: u8, u16, etc.

class CNI6509 : public CDevice
{
private:
   // device identification information
   static const CDevice::DevInfo DEVINFO; 
   
   // error message strings
   static LPCTSTR EMSG_BADCHINCHSIG; 
   static LPCTSTR EMSG_BADMASTERSTC3SIG; 
   static LPCTSTR EMSG_BADSLAVESTC3SIG; 
   static LPCTSTR EMSG_BADSUBSYSTEMID; 
   static LPCTSTR EMSG_FAILRWTEST_CHINCH;
   static LPCTSTR EMSG_FAILRWTEST_MASTERSTC;
   static LPCTSTR EMSG_FAILRWTEST_SLAVESTC;

   // address offset from register address space for the master DAQ-STC to that of the slave DAQ-STC
   static const u32 Slave_STC_Offset;
   // subsystem product and vendor ID as they should appear in 32-bit register for the 6509
   static const u32 SSID;
   // address of the Static Digital Input register for the master DAQ-STC
   static const u32 REG_DI_StaticDI;

   // memory-mapped address space for the device registers. XSeries boards have a single register address space,
   // accessed via BAR0. Declared as PVOID because it may refer to 1-, 2- or 4-byte registers.
   PVOID m_pvRegisters;

   BOOL m_IsFirstInit;
private:
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6509(const CNI6509& src); 
   CNI6509& operator=(const CNI6509& src); 

public: 
   // constructor/destructor
   CNI6509(int iDevNum); 
   ~CNI6509();
   
   // [CDevice impl/overrides] 
   virtual LPCTSTR RTFCNDCL GetDeviceName() { return( "PCIe-6509" ); }
   BOOL RTFCNDCL Init(); 

   // Omniplex interface module functionality
   VOID RTFCNDCL TriggerMarkers(DWORD dwVec);
   VOID RTFCNDCL WriteChar(char c); 
   VOID RTFCNDCL WriteString(char* str, int len);

   // for test purposes only!
   VOID RTFCNDCL RunLoopbackTest();

protected:
   BOOL RTFCNDCL MapDeviceResources(); 
   VOID RTFCNDCL UnmapDeviceResources(); 
   BOOL RTFCNDCL OnOpen();
   VOID RTFCNDCL OnClose();
   
private:
   // inline methods for writing and reading individual registers on the PCIe-6509. Both Windows host and the
   // PCIe-6509 are little-endian, so we don't need to do byte-swapping for the u16 and u32 registers.
   // IMPORTANT: The volatile keyword is critical here, because hardware registers can change at any time. Compiler
   // optimizations can really screw-up HW register access code if the volatile keyword is missing!!!!!!!!!!
   VOID RTFCNDCL WriteReg8(u32 addr, u32 datum) { *((u8 volatile *) (((u8 *)m_pvRegisters) + addr)) = (u8) datum; }
   VOID RTFCNDCL WriteReg16(u32 addr, u32 datum) { *((u16 volatile *) (((u16 *)m_pvRegisters) + (addr>>1))) = (u16) datum; }
   VOID RTFCNDCL WriteReg32(u32 addr, u32 datum) { *((u32 volatile *) (((u32 *)m_pvRegisters) + (addr>>2))) = datum; }

   u8 RTFCNDCL ReadReg8(u32 addr) { return(*((u8 volatile *) (((u8 *)m_pvRegisters) + addr))); }
   u16 RTFCNDCL ReadReg16(u32 addr) { return(*((u16 volatile *) (((u16 *)m_pvRegisters) + (addr>>1)))); }
   u32 RTFCNDCL ReadReg32(u32 addr) { return(*((u32 volatile *) (((u32 *)m_pvRegisters) + (addr>>2)))); }

   // reads and validates contents of specific signature/ID registers onboard the NI6509 (sanity check)
   BOOL RTFCNDCL CheckDeviceSignatures();
   // test scratchpad registers in the CHInCh interface and the DAQ-STC3 (sanity check)
   BOOL RTFCNDCL TestScratchPadRegisters();
   // performs a software reset of the two DAQ-STCs on the 6509; disables and acks all possible board interrupts
   BOOL RTFCNDCL ResetSTCs();
};


#endif   // !defined(NI6509_H__INCLUDED_)

