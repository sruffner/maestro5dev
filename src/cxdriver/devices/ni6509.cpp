//===================================================================================================================== 
//
// ni6509.cpp : CNI6509, an implementation of the abstract CDevice interface, targeting the PCIe-6509 static digital IO
//              board from National Instruments; it provides an alternative implementation of the latched external
//              hardware interface to the Omniplex Neural Data Acquisition System.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// LICENSE: The code in NI6509*.* is based on information in the National Instruments X-Series Measurement Hardware
// Device Development Kit (MHDDK) and as such is under the National Instruments Software License Agreement. For details,
// refer to the license agreement in "MHDDK License Agreement.pdf", which should be in the same source code directory
// as this file. Among other constraints, this module may NOT be redistributed in source code form -- unless the 
// receiving party also agrees to NI's licensing terms.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// The PCIe-6509 is a PCIExpress-based digital IO board from National Instruments. It has a total of 96 signal pins
// that are reconfigurable for input or output. The DAQ-STC3 timing engine is at the heart of the 6509's design. In 
// fact, there are two STCs on the 6509, each providing 48 DIO signals.
//
// We are using the 6509 as an alternative to the so-called Plexon interface module, an external "latched" digital
// device that sits in the experiment rack and is controlled by Maestro's CCxEventTimer device. The external module is
// getting old, with some failures in chips for which replacements cannot be found. Furthermore, the Lisberger lab
// has since replaced its original Plexon MAP systems with the newer Omniplex Neural Data Acquisition System; the
// Omniplex's superior DI subsystem allows a simpler implementation of the communication interface using software-timed
// digital outputs:
//    1) All TTL inputs are timestamped at 25us intervals, so a ~25us pulse width is adequate for all pulses. On the
// old Plexon MAP, most TTL inputs required a minimum pulse width of 250us -- which required a hardware-timed control
// of the TTL pulses.
//    2) The 8-bit strobed word input to the original Plexon required a hardware-based handshaking protocol. After 
// setting the 8-bit word, the Plexon interface module asserted the Plexon's INPSTRB input. It then waited for the
// Plexon to assert and de-assert its INPBSY output before moving on to write the next 8-bit word. The original
// module also had a FIFO for storing a "string" of characters to be sent to the Plexon. The Omniplex can receive
// strobed words at a faster rate and does not provide an INPBSY signal.
//
// ==> Signal connections; external hardware considerations.
// As specified in the original design of the hardware-based Plexon interface module, marker pulses on Maestro's DO<0>
// are NOT exposed to the Omniplex. DO<8..1> are routed to the TTL inputs "Event10..3" on the Omniplex, while 
// DO<11..9> are routed to the Plexon-era XS2..XS0 inputs -- corresponding to "Event2..0" in the Plexon recordings. On
// the Plexon, TTL pulses on Event10..3 required a minimum 250us pulse width, while those on XS2..0 required only a
// 25us pulse width. On the Omniplex, 25us pulse widths are sufficient for all TTL inputs.
//
// NOTE that Maestro reserves DO<11> to deliver a pulse ("RECORDMARKER") that is critical to synchronizing the
// Maestro and Omniplex timelines. In Trial mode, the pulse is delivered just before the trial starts and immediately
// after it ends. In Continuous mode, the pulse is delivered when recording starts and ends.
//
// The Omniplex has two digital input ports A & B, while the Plexon had only one. In order to timestamp Maestro's
// TTL marker pulses and also send ASCII character data to the Plexon, its single 16-bit DI port was configured in
// "Map Mode 2", with 8 TTL inputs and an 8-bit strobed word. This mode is not available on the Omniplex; instead one
// can configure one 16-bit port for TTL and the other as a strobed word. Fortunately, a special adapter cable is
// available to emulate the Plexon's Map Mode 2 functionality, and the lab uses this cable to interface the hardware-
// based Plexon interface module with the new Omniplex system. Since CNI6509 emulates the functionality of the original
// interface module, an external interface circuit will be required that maps the 100-pin connector on the PCIe-6509
// to the adapter cable connectors: a 26-pin ribbon + 2 BNC connectors for the Plexon-era "XS2" and "XS1" signals (the
// "XS0" signal is not available for use). The Maestro online guide has documentation on the 06-24-A-09 adapter cable,
// including the pin-out specification for the 26-pin ribbon connector.
//
// The table below summarizes how CNI6509 maps the Maestro-centric signals to the output ports of the PCIe-6509 and,
// in turn, how the port pins should be connected to the input lines on the 06-24-A-09 adapter cable:
//
// _________Maestro Signal_________       _______PCIe-6509 Port Pins_______      ______Adapter Cable Inputs______
//             DO<8..1>                             Port0<7..0>                      Event<10..3> (pins 10..3)
//             DO<11..10>                           Port1<2..1>                        "XS2", "XS1" (BNCs)
//             DO<9>                                Port1<0>                                NOT USED
//         8-bit ASCII chars                        Port2<7..0>                       Bit<7..0> (pins 18..11)
//           INPSTRB pulse                          Port3<0>                                INPSTRB (pin 20)
//
// ==> Usage.
// CNI6509 provides three key methods that Maestro uses to interface with the Omniplex system. These are comparable
// to same-named methods in CCxEventTimer, although the implementation details are different:
//
//    TriggerMarkers() : Deliver a marker pulse on any of 11 TTL lines, DO<11..1>.
//    WriteChar() : Write a single 8-bit ASCII character through the Omniplex's strobed word port.
//    WriteString() : Write a null-terminated ASCII string through the Omniplex's strobed word port.
//
// The implementation here very much follows that of the CNI6363, since the PCIe-6363 and PCIe-6509 both use the
// DAQ-STC3 timing engine and have very similar register maps. The PCIe-6509 has 2 DAQ-STC3 engines in a master-slave
// configuration. The register address space for the slave STC3 is offset from the first by 0x20000; from that I 
// assume that the total addressable space is twice what it is for the PCIe-6363 (512K vs 256K). However, note that we
// do not use the second STC in the CNI6509 implementation.
//
// CREDITS:
// 1) NI PCIe-6509 DIO board from National Instruments (www.ni.com). To understand the code in this module, review the 
// documentation and code examples provided in the NI Measurement Hardware Device Development Kit. The NIMHDDK is an
// alternative to using NIDAQmx, the NI-supplied universal driver for most NI hardware. It is required to do any 
// register-level programming of NI X-series devices. Also refer to the 6509-specific "NI PCIe-6509 Register Level 
// Programming Reference Manual". 
//
// REVISION HISTORY:
// 26may2021-- Began development. Using the implementation of CNI6363 as a blueprint...
//===================================================================================================================== 

#include <stdio.h>                        // runtime C/C++ I/O library
#include "util.h"                         // for the CElapsedTime utility 
#include "ni6509.h"
#include "ni6363regs.h"                   // register addresses, bit field masks, and other constants


// PCIe-6509 device info: hosted on PCI Express bus, which implements PCI protocol; vendor and subvendor is NI, 0x1093.
// Device ID is common to all NI XSeries devices; the subsystem ID uniquely identifies the PCIe-6509.
const CDevice::DevInfo CNI6509::DEVINFO = { CDevice::DF_PCI, 0x1093, 0xC4C4, 0x1093, 0x7326 };

// error message strings
LPCTSTR CNI6509::EMSG_BADCHINCHSIG = "Invalid signature for CHInCh";
LPCTSTR CNI6509::EMSG_BADMASTERSTC3SIG = "Invalid signature for Master DAQ-STC3";
LPCTSTR CNI6509::EMSG_BADSLAVESTC3SIG = "Invalid signature for Slave DAQ-STC3";
LPCTSTR CNI6509::EMSG_BADSUBSYSTEMID = "Invalid PCI subsystem vendor/product ID";
LPCTSTR CNI6509::EMSG_FAILRWTEST_CHINCH = "Read/write to CHInCh Scrap register failed";
LPCTSTR CNI6509::EMSG_FAILRWTEST_MASTERSTC = "Read/write to Master DAQ-STC3 ScratchPad register failed";
LPCTSTR CNI6509::EMSG_FAILRWTEST_SLAVESTC = "Read/write to Slave DAQ-STC3 ScratchPad register failed";


u32 CNI6509::Slave_STC_Offset = 0x20000;
u32 CNI6509::SSID = 0x73261093;
u32 CNI6509::REG_DI_StaticDI = 0x20530;


/**
 Construct the CNI6509 device object, initially unconnected to a physical device.
 @param iDevNum Instance of PCIe-6509 on PCI Express bus that is to be associated with this device object.
*/
CNI6509:: CNI6509(int iDevNum) : CDevice(DEVINFO, iDevNum)
{
   m_pvRegisters = (PVOID) NULL;
   m_IsFirstInit = TRUE;      // we configure all the pins we need as digital outputs, but we only need do it once.
}

/**
 Destroy the CNI6509 device object. Before doing so, disconnect it from the physical device.
*/
CNI6509::~CNI6509()
{
   Close();
}

/**
 [CDevice impl] On first init we configure all pins on the PCIe-6509's 8-bit ports 0-3 as digital outputs.
*/
BOOL RTFCNDCL CNI6509::Init()
{
   if( !IsOn() )                                         // device is not available!!
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }

   // configure ports 0-3 (32-bit port 0 on the master STC) for digital output. We only have to do this once b/c
   // we never use any other configuration. We leave all other outputs alone. They should NOT be connected to
   // anything.
   if(m_IsFirstInit)
   {
      m_IsFirstInit = FALSE;
      WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
      WriteReg32(NIX::REG_DO_DIODirection, (u32) 0xFFFFFFFF);
   }

   return(TRUE);
}

/**
 Deliver brief pulses to the Omniplex on Maestro's designated digital output lines DO<11..1>.

 DO<8..1> map to the Omniplex's TTL event inputs "Event10..Event3". DO<11> is the all-important Maestro-Omniplex
 sync pulse; it maps to TTL "Event2" (aka "XS2" on the Plexon). DO<10> maps to "Event1" ("XS1") and DO<9> maps to
 "Event0" ("XS0"). Note that XS0 is not accessible via the Omniplex's 06-24-A-09 "DI Map Mode 2 Emulator" adapter.

 DO<8..1> is delivered on the PCIe-6509's "Port0" (P0.7 .. P0.0), while DO<11..9> are delivered on the first 3 bits
 of Port1 (P1.2 .. P1.0). As recommended in the 6509 register-level programming manual, Port0 and Port1 are updated
 separately with register writes, even though they could be updated with a single 32-bit write.

 IMPORTANT: The PCIe-6509 only supports software-timed DO. To ensure the marker pulses are raised for a minimum of
 25us, the method raises the relevant digital output lines, busy waits for 30us, then lowers all lines. This 
 should be OK in time-critical code (during a trial). Even if the calling thread is interrupted briefly, that will
 merely extend the pulse length, and the Omniplex timestamps the rising edge of the pulses.

 @param dwVec Each bit N in this mask corresponds to one of Maestro's digital output lines DO<11..0>, for N=11..0.
 If a bit is set, an active-high pulse will be delivered on that line. DO<0> is ignored, as it does not map to an
 Omniplex TTL event signal.
*/
VOID RTFCNDCL CNI6509::TriggerMarkers(DWORD dwVec)
{
   // if no marker pulse is raised on DO<11..1>, do nothing!
   if((dwVec & 0x0FFE) == 0) return;

   // TODO: Or, should we do 8-bit writes to 8-bit sections of the 32-bit REG_DO_Static_DO register??

   // DO<11..1> maps to the 6509's 8-bit Port1<2..0> + Port0<7..0>. Internally, these map to the STC engine's 32-bit
   // Port0<10..0>. However, we must update one 8-bit port at a time according to the 6509 manual....
   u32 out_port0 = (dwVec >> 1) & 0x00FF
   u32 out_port1 = (dwVec >> 1) & 0x0700
   if(out_port0 != 0) WriteReg32(NIX::REG_DO_Static_DO, out_port0)
   if(out_port1 != 0)
   {
      out_port1 = out_port1 | out_port0   // so we don't change what we did to Port 0
      WriteReg32(NIX::REG_DO_Static_DO, out_port1)
   }

   // busy wait 30us
   CElapsedTime eTime;
   volatile long count = 0;
   while(eTime.Get() < 30.0) ++count;

   WriteReg32(NIX::REG_DO_Static_DO, (u32) 0)  // hopefully it's OK to clear all at once
}

/**
 Transmit a single 8-bit ASCII character to the Omniplex via the PCIe-6509.

 CNI6509 dedicates the PCIe-6509's 8-bit Port2 for character, and uses Port3, bit 0 to deliver the INPSTRB pulse,
 which indicates that the character data is ready.

 With the older Plexon MAP system, writing character data required a hardware-handshaking procedure: After the
 Plexon detected the INPSTRB pulse, it asserted the INPBSY signal for ~150us. The original Plexon interface module
 would not send the next character until after the INPBSY signal was lowered. The Omniplex, however, does not provide
 an INPBSY signal. It can accept strobed characters at a much faster rate, at least in short bursts. According to the
 Omniplex User Manual, the rising edge transition on INPSTR is detected at a rate of 20MHz but timestamped at a rate
 of 40KHz (25us). Also, the character data lines should be stable for at least 0.1us before asserting INPSTRB.
 
 Since Maestro never sends strobed character data to the Omniplex during time-critical operation, we decided to 
 implement busy waits such that: (a) the active-high INPSTRB pulse is raised ~5us after setting the character data
 lines, then lowered after roughly >= 10us, (b) the 8-bit character data port is reset to 0x00 after an additional
 35us (without raising INPSTRB again). This ensures a minimum of ~50us between characters even if this method is
 invoked multiple times in succession. Resetting the 8-bit port before returning also obviates the need for a software
 copy of the port contents.

 @param c The ASCII character to be written
*/
VOID RTFCNDCL CNI6509::WriteChar(char c)
{
   // character is written to 6509's Port2<7..0>, which maps to lines 23..16 on the STC's Port0
   u32 out = (((u32) c) << 16) & 0x00FF0000
   WriteReg32(NIX::REG_DO_Static_DO, out)

   // 5-us busy wait to give Port2 lines time to settle
   CElapsedTime eTime;
   volatile long count = 0;
   while(eTime.Get() < 5.0) ++count;

   // raise INPSTRB on Port3<0> = STC's Port0<24> for roughly 10us without changing what's on Port2
   out |= (u32) (1 << 24)
   WriteReg32(NIX::REG_DO_Static_DO, out)
   eTime.Reset()
   while(eTime.Get() < 10.0) ++count;
   out &= ~((u32) (1 << 24))
   WriteReg32(NIX::REG_DO_Static_DO, out)

   // busy wait another ~35us before restoring all outputs to 0 -- for a total per-char cycle of ~50us
   eTime.Reset()
   while(eTime.Get() < 35.0) ++count;
   WriteReg32(NIX::REG_DO_Static_DO, (u32) 0)
}

/**
 Transmit a null-terminated ASCII string to the Omniplex via the PCIe-6509.

 The method simply invokes WriteChar() for each character in the string. If the string lacks a terminating null
 character (0x00), then the null character is written after the last character. Not for use in time-critical code
 sections. Expect the overall execution time for this method to be roughly 50*(N+1) us, where N is the string
 length (excluding the terminating null character).

 @param str An ASCII chracter string to be written.
 @param len Length of the character string.

*/
VOID RTFCNDCL CNI6509::WriteString(char* str, int len)
{
   for(int i=0; i<len; i++)
      WriteChar(str[i])
   if((len > 0) && (str[len-1] != 0))
      WriteChar((char)0)
}

/**
 Perform a simple loopback test to verify operation of the PCIe-6509.

 This function is only for test purposes, as it configures the 6509 8-bit ports 0-3 differently from how they are
 configured for CNI6509's intended function as an implementation of the Omniplex interface module.

 The test configures Ports 0 and 1 as outputs, and Ports 2 and 3 as inputs. It assumes that Port 0, line N is
 connected to Port 2, line N (N=0..7), and analogously for Ports 1 and 3 -- hence the term "loopback test". One at
 a time, each line on the output ports is raised, then the corresponding input port is read to verify that the
 corresponding digital signal is high (and all the others are low!).

 The test reports progress via printf's to the console window. It stops immediately as soon as an error is
 detected.
*/
VOID RTFCNDCL RunLoopbackTest()
{
   CElapsedTime eTime;
   volatile long count = 0;

   ::printf("\nStarting loopback test on the PCIe-6509. Test assumes that Port 0 is connected to Port 2, and\n")
   ::printf("Port 1 is connected to Port 3. If not, test will fail.\n\n")

   if(!IsOn())
   {
      ::printf("   ERROR: PCIe-6509 is not available!\n");
      return;
   }

   // configure Ports 0 and 1 as outputs, 2 and 3 as inputs
   WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
   WriteReg32(NIX::REG_DO_DIODirection, (u32) 0x0000FFFF);

   BOOL ok = TRUE;
   for(int i=0; ok && i<16; i++)
   {
      if(i < 8)
         ::printf("...Testing Port 0, line %d", i)
      else
         ::printf("...Testing Port 1, line %d, i-8)

      u32 out = (u32) (1<<i)
      WriteReg32(NIX::REG_DO_Static_DO, out)

      // brief busy wait to let output settle
      eTime.Reset();
      while(eTime.Get() < 1000.0) ++count;

      u32 res = ReadReg32(CNI6509::REG_DI_StaticDI)
      res = (res >> 16) & 0x0000FFFF

      if(res == out)
         ::printf("...OK.\n")
      else
      {
         ::printf("...ERROR - Output = %04x, input = %04x. Loopback test failed.\n", out, res)
         ok = FALSE;
      }
   }

   if(ok)
      ::printf("Test completed successfully!\n")

   // clear all 4 ports and reconfigure as outputs (hopefully this is OK while the loopback connection is there!)
   WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
   WriteReg32(NIX::REG_DO_DIODirection, (u32) 0x0000FFFF);
}

/**
 [CDevice impl] Acquire the memory mapped or I/O resources needed to talk to the physical device. 
 
 Like NI X-Series DAQ devices, the PCIe-6509 exposes its registers via a single memory address space via BAR0. This
 method translates the BAR0 bus address to a system address, memory maps it into virtual memory, and enables certain
 PCI bus features (PCI_ENABLE_IO_SPACE, _MEMORY_SPACE, _BUS_MASTER, _WRITE_AND_INVALIDATE). All operations must be
 successful, or the method fails.

 @return True if successful, false otherwise
*/
BOOL RTFCNDCL CNI6509::MapDeviceResources()
{
   // if register address space already mapped, do nothing
   if(m_pvRegisters != NULL) return(TRUE);
   
   // translate physical BAR0 bus address to system address. Resource type is memory space, not port IO
   LARGE_INTEGER i64TranslatedBAR0;
   LARGE_INTEGER i64DeviceBAR0;
   i64DeviceBAR0.QuadPart = GetPciBaseAddressReg(0);
   ULONG busNumber = (ULONG) GetPciBus();
   ULONG addrSpaceType = 0;
   if(!::RtTranslateBusAddress(PCIBus, busNumber, i64DeviceBAR0, &addrSpaceType, &i64TranslatedBAR0))
      return(FALSE);
   
   // map translated address to virtual memory (disabling cache)
   // because 6509 has 2 STCs, it has twice the address space of the 6363.
   m_pvRegisters = ::RtMapMemory(i64TranslatedBAR0, NIX::REGADDRSPACESIZE * 2, MmNonCached);
   if(m_pvRegisters == NULL) return(FALSE);
   
   // enable selected PCI device features for the PCIe-6509
   PCI_COMMON_CONFIG pciConfig;
   GetPciConfig(&pciConfig);
   pciConfig.Command = PCI_ENABLE_IO_SPACE|PCI_ENABLE_MEMORY_SPACE|PCI_ENABLE_BUS_MASTER|
         PCI_ENABLE_WRITE_AND_INVALIDATE;
   if(0 == ::RtSetBusDataByOffset(PCIConfiguration, busNumber, GetPciSlot(), &pciConfig, 0, PCI_COMMON_HDR_LENGTH))
   {
      ::RtUnmapMemory(m_pvRegisters);
      m_pvRegisters = NULL;
      return(FALSE);
   }
   
   return(TRUE);
}

/** [CDevice impl] Release the memory-mapped or IO resources needed to talk to the physical device. */
VOID RTFCNDCL CNI6509::UnmapDeviceResources()
{
   if(m_pvRegisters != NULL) ::RtUnmapMemory(m_pvRegisters);
   m_pvRegisters = NULL;
}

/**
 [CDevice override] Perform any one-time, device-specific tasks that must be done immediately after "opening" the 
 connection to the physical device. This method is called by CDevice::Open().

 Here we perform some sanity checks to ensure we're able to read/write some registers on the board, then reset both
 of the DAQ-STC3 engines (even though we'll only use one of them).
 
 @return True if successful, false otherwise. This method fails if any sanity check fails or the STC reset fails.
*/
BOOL RTFCNDCL CNI6509::OnOpen()
{
   if(!CheckDeviceSignatures()) return(FALSE);
   if(!TestScratchPadRegisters()) return(FALSE);
   if(!ResetSTCs()) return(FALSE);
   
   return(TRUE)
}

/**
 [CDevice override] 
*/
VOID RTFCNDCL CNI6509::OnClose()
{
   if(IsOn())
   {
      ResetSTCs()

      // restore all pins on 8-bit ports 0-3 as digital inputs (the default power-up state)
      WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
      WriteReg32(NIX::REG_DO_DIODirection, (u32) 0);
   }
}

/**
 Reads and validates the contents of three signature/ID registers onboard the NI6509: the CHInCh ID register, the
 DAQ-STC3 signature register, and the PCI Subsystem ID access register. The contents of these read-only 32-bit registers
 are fixed, and this method verifies their values for the NI PCIe-6509.
 @return True if registers could be read and their contents verified; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6509::CheckDeviceSignatures()
{
   LPCTSTR emsg = IsOn() ? NULL :  CDevice::EMSG_DEVNOTAVAIL;
   if(emsg == NULL && NIX::CHInCh_Signature != ReadReg32(NIX::REG_CHInCh_ID)) emsg = EMSG_BADCHINCHSIG;
   if(emsg == NULL)
   {
      u32 stcRev = ReadReg32(NIX::REG_Signature);
      if(NIX::STC_RevA != stcRev && NIX::STC_RevB != stcRev) emsg = EMSG_BADMASTERSTC3SIG;
   }
   if(emsg == NULL)
   {
      u32 stcRev = ReadReg32(NIX::REG_Signature + CNI6509::Slave_STC_Offset);
      if(NIX::STC_RevA != stcRev && NIX::STC_RevB != stcRev) emsg = EMSG_BADSLAVESTC3SIG;
   }
   if(emsg == NULL && CNI6509::SSID != ReadReg32(NIX::REG_PCISubsystem_ID)) emsg = EMSG_BADSUBSYSTEMID;
   
   if(emsg != NULL) SetDeviceError(emsg);
   return(BOOL(emsg == NULL));
}

    
/**
 Read and write to each individual bit of three "scratchpad" registers on the PCIe-6509, one in the CHInCh interface and
 one in the master DAQ-STC3 timing engine, and one in the slave DAQ-STC. These registers have no effect on hardware
 function; they offer a "sanity check" to see if we're unable to communicate with the hardware via our mapped register
 address space.
 @return True if able to read and write the scratch pad registers; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6509::TestScratchPadRegisters()
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   u32 valueIn, valueOut, i;
   
   // write a 1 to each bit of CHInCh's scrap register, and verify on readback; then repeat with a 0 to each bit
   BOOL bOk = TRUE;
   for(i=0; bOk && i<32; i++)
   {
      valueIn = (1<<i);
      WriteReg32(NIX::REG_Scrap, valueIn);
      valueOut = ReadReg32(NIX::REG_Scrap);
      bOk = BOOL(valueIn == valueOut);
   }
   for(i=0; bOk && i<32; i++)
   {
      valueIn = ~(1<<i);
      WriteReg32(NIX::REG_Scrap, valueIn);
      valueOut = ReadReg32(NIX::REG_Scrap);
      bOk = BOOL(valueIn == valueOut);
   }
   if(!bOk)
   {
      SetDeviceError(EMSG_FAILRWTEST_CHINCH);
      return(FALSE);
   }

   // repeat for the Master DAQ-STC's scratch pad register
   for(i=0; bOk && i<32; i++)
   {
      valueIn = (1<<i);
      WriteReg32(NIX::REG_ScratchPad, valueIn);
      valueOut = ReadReg32(NIX::REG_ScratchPad);
      bOk = BOOL(valueIn == valueOut);
   }
   for(i=0; bOk && i<32; i++)
   {
      valueIn = ~(1<<i);
      WriteReg32(NIX::REG_ScratchPad, valueIn);
      valueOut = ReadReg32(NIX::REG_ScratchPad);
      bOk = BOOL(valueIn == valueOut);
   }
   if(!bOk)
   {
      SetDeviceError(EMSG_FAILRWTEST_MASTERSTC);
      return(FALSE);
   }

   // repeat for the Slave DAQ-STC's scratch pad register
   for(i=0; bOk && i<32; i++)
   {
      valueIn = (1<<i);
      WriteReg32(NIX::REG_ScratchPad + CNI6509::Slave_STC_Offset, valueIn);
      valueOut = ReadReg32(NIX::REG_ScratchPad + CNI6509::Slave_STC_Offset);
      bOk = BOOL(valueIn == valueOut);
   }
   for(i=0; bOk && i<32; i++)
   {
      valueIn = ~(1<<i);
      WriteReg32(NIX::REG_ScratchPad + CNI6509::Slave_STC_Offset, valueIn);
      valueOut = ReadReg32(NIX::REG_ScratchPad + CNI6509::Slave_STC_Offset);
      bOk = BOOL(valueIn == valueOut);
   }
   if(!bOk)
   {
      SetDeviceError(EMSG_FAILRWTEST_SLAVESTC);
      return(FALSE);
   }

   return(TRUE);
}

/**
 This method is intended to put the PCIe-6509 in a known, inactive state during application startup and shutdown. It is
 invoked in OnOpen() prior to setting up the "subsystem devices" implemented on the board, and in OnClose() after 
 closing those subdevices. The method strobes bit0 in the Joint_Reset_Register to reset the DAQ-STC3 timing engine, then 
 disables all board interrupts. NOTE that CNI6509 does not make use of any interrupts on the PCIe-6509.

 @return True if successful; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6509::ResetSTCs()
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

   // DAQ-STC3 software reset -- both master and slave STCs
   WriteReg16(NIX::REG_Joint_Reset, 0x01);
   WriteReg16(NIX::REG_Joint_Reset + CNI6509::Slave_STC_Offset, 0x01);

   // disable and ack all interrupts at board level
   u32 cmd = NIX::IMR_Clear_CPU_Int | NIX::IMR_Clear_STC3_Int;
   WriteReg32(NIX::REG_Interrupt_Mask, cmd);
   
   // disable propagation of all DAQ-STC subsystem interrupts to the CHInCh -- for both master and slave STCs
   WriteReg32(NIX::REG_GlobalInterruptEnable, NIX::GIER_DisableAll_Cmd);
   WriteReg32(NIX::REG_GlobalInterruptEnable + CNI6509::Slave_STC_Offset, NIX::GIER_DisableAll_Cmd);
   
   return(TRUE);
}