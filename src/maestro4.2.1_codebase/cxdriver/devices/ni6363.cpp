//===================================================================================================================== 
//
// ni6363.cpp : CNI6363, an implementation of the abstract CDevice interface, targeting the PCIe-6363 multi-function IO
//              board from National Instruments; plus pseudo "subdevice" classes that implement AI, AO, and DIO timer
//              functions on the PCIe-6363
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// LICENSE: The code in NI6363*.* is based on information in the National Instruments X-Series Measurment Hardware
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
// The PCIe-6363 is a PCIExpress-based multi-function data acquisition board from National Instruments. It has 32
// 16-bit analog inputs, 4 16-bit analog outputs, 32 digital IO channels for hardware-timed digital input or output,
// and 4 32-bit counters, among other features. The analog input, analog output, digital IO, and counter subsystems
// can all operate independently and simultaneously. 
//
// We decided to support the PCIe-6363 in Maestro 3 for several reasons:
//    (1) The legacy PCI-MIO16E1 is obsolete and increasingly hard to find. The PCIe-6363 is a brand-new DAQ card (as of
// early 2011), has more capabilities than the MIO16E1, and is housed on the faster PCIExpress communication interface.
//    (2) It supports message-based interrupts (MSI), which immediately alleviates the installation headache of 
// configuring the host computer so that the AI device can get exclusive access to the very limited number of IRQ lines.
//    (3) Because the various subsystems can work independently, and because the PCIe-6363 includes DI change-detection
// circuitry, it was possible to implement three different Maestro-related device functions on a single physical device:
// analog input (CCxAnalogIn), analog output (CCxAnalogOut), and digital IO and event timestamping (CCxEventTimer). 
//
// Note that the PCIe-6363 is NOT supported in versions of Maestro prior to v3.0. Version 3.0 drops support for the
// fiber optic targets, to which 8 analog outputs were dedicated. Now, only one analog output is needed -- to drive the
// animal chair --, so the PCIe-6363's 4 AO channels are more than enough.
//
// ==> PCIe-6363 does not fit the CDevice framework mold.
// MaestroRTSS's CDevice framework did not anticipate multiple device functions being implemented on a single board. We
// had to make some changes in CDevice and CCxDeviceManager in order to "shoehorn" it into that framework. To that end,
// we came up with a parent device/subdevice concept. CNI6363 is the parent device class. It handles the tasks of 
// finding the physical device in the system, mapping device memory, and handling other low-level tasks like retrieving
// calibration constants from EEPROM. When this "parent device" is opened, it creates and opens the three "subdevice"
// classes that implement CCxAnalogIn, CCxAnalogOut, and CCxEventTimer. These subdevice implementations rely on 
// parent device methods for reading and writing device registers; their Map/UnmapDeviceResources() method are 
// essentially no-ops.
//
// ==> Implementation details.
// See the class headers in this source file for each of the subdevice classes: CNI6363_AI implements CCxAnalogIn,
// CNI6363_AO implements CCxAnalogOut, and CNI6363_DIO implements CCxEventTimer.
//
// Note on voltage calibration. Converting a raw ADC code to the corresponding input voltage is very different on NI
// X-Series devices like the PCIe-6363 vs legacy E-Series devices like the PCI-MIO16E1. For X-Series devices, the
// conversion is nonlinear, using a 3rd-order polynomial. Onboard EEPROM holds the coefficients of that polynomial, and
// they are different for the different supported voltage ranges. A similar conversion applies to the analog outputs,
// but the polynomial is only first-order (linear). For more information, review the implementation of parent device
// CNI6363, in particular, CNI6363_AI::GetCalibInfoFromEEPROM(), ADCToVolts(), and VoltsToDAC().
//
// Note on "timeouts". In many of the example programs that come with the X-Series MHDDK, the code "spins" waiting for
// a timing engine (input, output, counter) to arm (by checking an "armed" flag in a status reg) after strobing the 
// appropriate "arm" command bits. Their timeouts are as much as 5 seconds. In our implemenations, we do not spin
// longer than a few hundred microseconds, but testing thus far has indicated that this is not an issue.
// 
// LESSON LEARNED. ALWAYS use the volatile keyword with register read/writes. Particularly with the read op, if you loop
// waiting on a register to change value, compiler optimizations may eliminate all but the first read of that register 
// -- which means that the register value won't change from the program's point-of-view, and the loop will be infinite, 
// or it'll timeout if you have implemented such a mechanism. I modified the CNI6363's low-level register read and
// write operations to include the volatile keyword, and this immediately fixed a mysterious problem I was having
// in CNI6363_DIO::Unload()!!!
//
// UNDOCUMENTED AI "ghost" feature. Enabling the 25KHz "fast" channel requires multi-rate sampling. The 16-channel 
// "slow-scan set" is sampled as fast as possible at the beginning of a 1ms or 2ms scan epoch, while the specified
// fast channel needs to be sampled every 40us during that epoch. Since the PCIe-6363 does not support sampling at two
// different rates, we can achieve it by sampling every 5us, specifying what channel should be sampled in each 5us 
// sample slot. For a 1ms scan epoch, we have to collect 193 samples per epoch, discarding all but 41 of them (25 
// samples of the fast channel, plus the 16 samples for the slow-scan set). Getting all those samples out of the FIFO
// via programmed IO was too slow. Testing showed an average/max time in AI.Unload() of ~590/900 us -- almost the entire
// scan interval in the worst-case. (Note that these values are bumped up by the occasional presence of the 200us 
// suspend period that's part of MaestroRTSS's design.)
//    Our initial implementation of CNI6363_AI chose this approach of oversampling and discarding unwanted samples 
// because the PCIe-6363 apparently lacked the "ghost channel" feature that we employed on the PCI-MIO16E1. However, it
// turns out that the "ghost" feature IS available on the PCIe-6363, it's just not documented in the X-Series MHDDK. As 
// with the E-series, when writing the scan list to the channel config FIFO, simply set the AI_Config_Channel_Type bit 
// field to 111, and no sample will be saved to the FIFO. Thus, we only have to collect 41 samples per 1-ms scan epoch,
// as before. 
//
// ISSUES WITH SetDO(): Mati Joshua's lab uses a different DIO interface than the Lisberger lab. While his interface
// had no trouble with marker pulse delivery in Maestro 3.x, he discovered that it often failed to deliver the 
// RECORDMARKER pulse (DO11), as well as any marker pulse delivered during the first millisecond (seg 0 marker) of the
// trial. Did extensive debug/test with Mati's help. Initially thought it had something to do with Maestro's interrupt
// handler running while in the middle of the first SetDO() call in TriggerMarkers() -- that this messed up the 
// CElapsedTime implementation of the 2.5us busy loop. I tried spinning in an empty while loop to achieve the minimum 
// 2.5us wait, but that did not help. However, by putting a short delay after each of the three register writes in
// SetDO(), the system seemed to work correctly according to Mati's testing. I have since updated the implementation of
// SetDO() accordingly. The user can adjust the exact wait times via a Maestro-specific setting in the Windows registry;
// Maestro stores the wait times in IPC for CXDRIVER access.
//
// CREDITS:
// 1) NI PCIe-6363 analog input board from National Instruments (www.ni.com).  To understand the code in this module,
// which realizes CCxAnalogIn, CCxAnalogOut, and CCxEventTimer on a single PCIe-6363, review the documentation and 
// code examples provided for X-Series NI board in the NI Measurement Hardware Device Development Kit. The NIMHDDK is an
// alternative to using NIDAQmx, the NI-supplied universal driver for most NI hardware. It is required to do any 
// register-level programming of NI devices. Some code here was adapted from examples provided with the NIMHDDK.
//
// REVISION HISTORY:
// 17jun2011-- Began development.
// 25jul2011-- Completed first try at implementation. Next step is to develop some testing code...
// 31aug2011-- Successfully built after fixing some compiler errors. Beginning test/debug...
// 20sep2011-- All functions appear to be working. Still need to verify operation of all DO and DI channels, and check
// that the "Data Ready" signal on PFI0 is adequate for latching DO vector into the external "latched devices" in 
// Maestro DIO rack interface module.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.  
// 19aug2019-- Added test function CNI6363::RunCtrCountdownTest() to evaluate using hardware to implement a "busy wait"
// that's a few microseconds long. See function comments.
// 02sep2019-- SetDO() implementation modified to address issue of missing marker pulses in Mati Josha's lab.
//===================================================================================================================== 

#include <stdio.h>                        // runtime C/C++ I/O library
#include "util.h"                         // for the CElapsedTime utility 
#include "ni6363.h"
#include "ni6363regs.h"                   // register addresses, bit field masks, and other constants

//===================================================================================================================== 
// CNI6363
//
// This class implements CDevice only and acts as the parent device for 3 pseudo subdevices: CNI6363_AI implementing
// CCxAnalogIn; CNI6363_AO implementing CCxAnalogOut; and CNI6363_DIOTimer implementing CCxEventTimer. The parent device
// object actually "acquires" the physical device and maps its registers into process memory. The subdevice objects
// call CNI6363 methods to read from and write to the physical device registers. CNI6363 itself does not encapsulate any 
// knowledge of the various device registers.
//===================================================================================================================== 

// PCIe-6363 device info: hosted on PCI Express bus, which implements PCI protocol; vendor and subvendor is NI, 0x1093.
// Device ID is common to all NI XSeries devices; the subsystem ID uniquely identifies the PCIe-6363.
const CDevice::DevInfo CNI6363::DEVINFO = { CDevice::DF_PCI, 0x1093, 0xC4C4, 0x1093, 0x7435 };

// error message strings
LPCTSTR CNI6363::EMSG_BADCHINCHSIG = "Invalid signature for CHInCh";
LPCTSTR CNI6363::EMSG_BADSTC3SIG = "Invalid signature for DAQ-STC3";
LPCTSTR CNI6363::EMSG_BADSUBSYSTEMID = "Invalid PCI subsystem vendor/product ID";
LPCTSTR CNI6363::EMSG_FAILRWTEST_CHINCH = "Read/write to CHInCh Scrap register failed";
LPCTSTR CNI6363::EMSG_FAILRWTEST_STC = "Read/write to STC ScratchPad register failed";
LPCTSTR CNI6363::EMSG_FAILEEPROMREAD = "Failed to retrieve calibration info from EEPROM";


/**
 Construct the CNI6363 device object, initially unconnected to a physical device.
 @param iDevNum Instance of PCIe-6363 on PCI Express bus that is to be associated with this device object.
*/
CNI6363::CNI6363(int iDevNum) : CDevice(DEVINFO, iDevNum)
{
   m_pvRegisters = (PVOID) NULL;
   
   m_pAI = (CNI6363_AI*) NULL;
   m_pAO = (CNI6363_AO*) NULL;
   m_pDIO = (CNI6363_DIO*) NULL;
}

/**
 Destroy the CNI6363 device object. Before doing so, make sure its subdevices have been closed and disconnect it from
 the physical device.
*/
CNI6363::~CNI6363()
{
   Close();
}

/**
 [CDevice impl] Since CNI6363 does not control anything directly, this method takes no action. It will fail, however, 
 if called when the device is disabled (ie, the device object is not connected to a physical device).
 @return True if successful, false otherwise.
*/
BOOL RTFCNDCL CNI6363::Init()
{
   if( !IsOn() )                                         // device is not available!!
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }
   return(TRUE);
}

/**
 For testing purposes only. This method runs through all performance tests that have been implemented on the three
 "subdevices" (AI, AO, DIO event timer) used in MaestroRTSS. The tests are very short and should be run with the 
 suspend manager (which yields CPU time to Windows) bypassed. Results are reported to the console via printf's.
*/
VOID RTFCNDCL CNI6363::RunPerformanceTests()
{
   if(!IsOn()) return;
   
   m_pAI->TestReadFIFOPerformance();
   m_pAO->TestUpdatePerformance();
   m_pDIO->TestShortPulseTimestampPerformance();
}

/**
 For testing purposes only. This method uses the general purpose counter G2 to count down a specified time between
 1us and 1sec, using a 100MHz source timebase. 

 16aug2019: I'm exploring this because Mati Joshua's lab has run into problems with the DataReady pulse not remaining 
 active low for a minimum of 2.5us in SetDO(), perhaps because of issues with the QueryPerformanceCounter()-based
 implementation of CElapsedTime. This could be used as a hardware-based "busy wait", but I think I might replace the
 QPC-based implementation of CElapsedTime with the original RtGetClockTime()-based implemenation that was used in 
 Maestro 3.x.

 @param tWaitUS [in] Desired wait time in us, restricted to the range [1..1000000] (0.001 - 1000 milliseconds).
 @param tElapsedUS [out] Elapsed time in us, measured using RtGetClockTime(). 
 @return TRUE if test is successful, else FALSE.
*/
BOOL RTFCNDCL CNI6363::RunCtrCountdownTest(int tWaitUS, double& tElapsedUS)
{
   tWaitUS = (tWaitUS < 1) ? 1 : ((tWaitUS > 1000000) ? 1000000 : tWaitUS);

   // BEGIN: Configure G2 counter to count down from 300 to 0 once, disarming automatically on reaching terminal count.
   // Using TB3 (100MHz, 10ns) as the counter source, the countdown should take 3us. The sample clock input is a 
   // software pulse which we never deliver -- so no count values are saved to the counter FIFO.
   //
   // reset counter G2
   u32 regOfsG2 = 2 * NIX::Gi_RegOffset;
   WriteReg16(NIX::REG_G0_Command + regOfsG2, NIX::GiCmd_Reset);
   WriteReg32(NIX::REG_G0_Interrupt_2 + regOfsG2, NIX::Int2_DisableAndAckAll_Cmd);
   WriteReg16(NIX::REG_G0_DMA_Config + regOfsG2, NIX::GiDMACfg_Reset);

   // G2_Mode_Register: ReloadSrc_Switching=UseSameLoadReg, Loading_On_Gate=NoReloadOnGate, ForceSrcEqualToTB=False, 
   // LoadingOnTC=Rollover, Counting_once=DisarmAtTCThatStops, LoadSrc_Sel=LoadFromA, TrgModeForEdgeGate=GateLoads, 
   // GatingMode=GateDisabled, all other fields set to 0 (don't cares).
   WriteReg16(NIX::REG_G0_Mode + regOfsG2, (u16)0x0438);

   // G2_Mode2_Register: Up_Down=Down, Bank_Switch_Enable=DisabledIfArmedElseX, Bank_Switch_Mode=Gate, StopOnError=False,
   // all other fields set to 0 (don't cares).
   WriteReg16(NIX::REG_G0_Mode2 + regOfsG2, (u16)0x0000);

   // G2_Counting_Mode_Register: Prescale=False, HWArm_Select=DIO_ChgDetect, HWArmEna=False, HWPolarity=ActiveHi, 
   // CountingMode=Normal, all other fields set to 0.
   WriteReg16(NIX::REG_G0_Counting_Mode + regOfsG2, (u16)0x000);

   // G2_SampleClock_Register: SampClkGateIndep=True, SampClkSampMode=LastSaved, SampClkMode=SingleSample, 
   // SampClkPolarity=RisingEdge, SampClkSelect=SwPulse, all other fields set to 0
   WriteReg16(NIX::REG_G0_SampleClock + regOfsG2, (u16)0x9100);

   // G2_AuxCtr_Register: AuxCtrMode=Disabled. We don't use the embedded auxiliary counter associated with G0.
   WriteReg16(NIX::REG_G0_AuxCtr + regOfsG2, (u16)0);

   // Don't touch G2_Autoincrement_Register. We don't need it.

   // G2_Second_Gate_Register: SecondGateMode=Disabled. We don't use the second gate.
   WriteReg16(NIX::REG_G0_Second_Gate + regOfsG2, (u16)0);

   // G2_Input_Select_Register: SrcPolarity=RisingEdge, SrcSelect=TB3 (100MHz), all other fields set to 0.
   WriteReg16(NIX::REG_G0_Input_Select + regOfsG2, (u16)0x003C);

   // Don't touch G2_ABZ_Select_Register. We don't need it.

   // G2_DMA_Config_Register: DMA_Write=False and DMA_Enable=True to enable the counter FIFO. All other fields = 0.  We 
   // don't use DMA; instead, we read FIFO content via programmed IO thru G0_RdFifo_Register. HOWEVER, no samples will
   // be put in the FIFO becaause our sample clock source is a "software pulse" that is never delivered.
   WriteReg16(NIX::REG_G0_DMA_Config + regOfsG2, (u16)0x0001);
   //
   // END: Configure G2 counter...

   // load G2 counter with initial count. At TB3=100MHz (10ns), the required count value is tWaitUS*100.
   WriteReg32(NIX::REG_G0_Load_A + regOfsG2, (u32) (tWaitUS * 100));
   WriteReg16(NIX::REG_G0_Command + regOfsG2, NIX::GiCmd_Load);

   // arm the counter
   LONGLONG timeoutTicks = (100000 + tWaitUS) * 100;
   LARGE_INTEGER liStart, liEnd;
   BOOL done = FALSE, armed = FALSE;
   WriteReg16(NIX::REG_G0_Command + regOfsG2, NIX::GiCmd_Arm);

   // wait for counter to be armed before setting T=0, but don't wait more than 10us
   ::RtGetClockTime(CLOCK_FASTEST, &liStart);
   while(!armed)
   {
      u32 stat = ReadReg32(NIX::REG_G0_Status + regOfsG2);
      if((stat & NIX::GiStat_Armed) != 0) armed = TRUE;
      else
      {
         ::RtGetClockTime(CLOCK_FASTEST, &liEnd);
         if((liEnd.QuadPart - liStart.QuadPart) > 1000) break;
      }
   }
   ::RtGetClockTime(CLOCK_FASTEST, &liStart);

   // wait for counter to reach TC, but timeout 100ms after desired wait time
   while(!done)
   {
      u32 stat = ReadReg32(NIX::REG_G0_Status + regOfsG2);
      if((stat & NIX::GiStat_TC) != 0) done = TRUE;
      else
      {
         ::RtGetClockTime(CLOCK_FASTEST, &liEnd);
         if((liEnd.QuadPart - liStart.QuadPart) > timeoutTicks) break;
      }
   }
   ::RtGetClockTime(CLOCK_FASTEST, &liEnd);
   tElapsedUS = ((double)(liEnd.QuadPart - liStart.QuadPart)) / 10.0;

   // reset counter G2
   WriteReg16(NIX::REG_G0_Command + regOfsG2, NIX::GiCmd_Reset);
   WriteReg32(NIX::REG_G0_Interrupt_2 + regOfsG2, NIX::Int2_DisableAndAckAll_Cmd);
   WriteReg16(NIX::REG_G0_DMA_Config + regOfsG2, NIX::GiDMACfg_Reset);

   return(done);
}

/**
 [CDevice impl] Acquire the memory mapped or I/O resources needed to talk to the physical device. 
 
 Like all NI XSeries devices, the PCIe-6363 exposes its registers via a single memory address space via BAR0. This
 method translates the BAR0 bus address to a system address, memory maps it into virtual memory, and enables certain
 PCI bus features (PCI_ENABLE_IO_SPACE, _MEMORY_SPACE, _BUS_MASTER, _WRITE_AND_INVALIDATE). All operations must be
 successful, or the method fails.

 @return True if successful, false otherwise
*/
BOOL RTFCNDCL CNI6363::MapDeviceResources()
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
   m_pvRegisters = ::RtMapMemory(i64TranslatedBAR0, NIX::REGADDRSPACESIZE, MmNonCached);
   if(m_pvRegisters == NULL) return(FALSE);
   
   // enable selected PCI device features for the PCIe-6363
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
VOID RTFCNDCL CNI6363::UnmapDeviceResources()
{
   if(m_pvRegisters != NULL) ::RtUnmapMemory(m_pvRegisters);
   m_pvRegisters = NULL;
}

/**
 [CDevice override] Perform any one-time, device-specific tasks that must be done immediately after "opening" the 
 connection to the physical device. This method is called by CDevice::Open().

 Here we initialize ALL subsystems in the PCIe-6363 -- not just the ones we'll use -- to ensure the board is in a known
 state when the device is opened. We perform some sanity checks to ensure we're able to read/write some registers on 
 the board and read in the AI calibration coefficients for the +/-10V bipolar mode from the device EEPROM.
 
 We then construct and "open" the pseudo subdevice objects that implement AI, AO and DIO event timer functions on the 
 single PCIe-6363 board. Each subdevice class is derived from the CDevice class, but their MapDeviceResources() and 
 UnmapDeviceResources() methods do nothing. Instead, they rely on the device resources mapped by CNI6363 itself,
 which is considered the "parent device".
 
 @return True if successful, false otherwise. This method fails is if the sanity check fails, if any subdevice object
 could not be allocated, or if a subdevice does its own sanity check in its own OnOpen() method and that check fails.
*/
BOOL RTFCNDCL CNI6363::OnOpen()
{
   if(!CheckDeviceSignatures()) return(FALSE);
   if(!TestScratchPadRegisters()) return(FALSE);
   if(!GetCalibInfoFromEEPROM()) return(FALSE);
   if(!ResetSTC()) return(FALSE);
   
   // construct and open each of the Maestro subdevices that are implemented on the PCIe-6363. Each uses a reference
   // to this parent device object to do all hardware I/O...
   m_pAO = new CNI6363_AO(this);
   BOOL bOk = (m_pAO != NULL) && m_pAO->Open();
   if(bOk)
   {
      m_pAI = new CNI6363_AI(this);
      bOk = (m_pAI != NULL) && m_pAI->Open();
   }
   if(bOk)
   {
      m_pDIO = new CNI6363_DIO(this);
      bOk = (m_pDIO != NULL) && m_pDIO->Open();
   }
   
   if(!bOk)
   {
      if(m_pAI != NULL) { m_pAI->Close(); delete m_pAI; m_pAI = NULL; }
      if(m_pAO != NULL) { m_pAO->Close(); delete m_pAO; m_pAO = NULL; }
      if(m_pDIO != NULL) { m_pDIO->Close(); delete m_pDIO; m_pDIO = NULL; }
   }
   
   return(bOk);
}

/**
 [CDevice override] Make sure the subdevice objects are closed and deallocated upon closing the CNI6363 parent device.
*/
VOID RTFCNDCL CNI6363::OnClose()
{
   if(m_pAI != NULL) { m_pAI->Close(); delete m_pAI; m_pAI = NULL; }
   if(m_pAO != NULL) { m_pAO->Close(); delete m_pAO; m_pAO = NULL; }
   if(m_pDIO != NULL) { m_pDIO->Close(); delete m_pDIO; m_pDIO = NULL; }
}


/**
 Reads and validates the contents of three signature/ID registers onboard the NI6363: the CHInCh ID register, the
 DAQ-STC3 signature register, and the PCI Subsystem ID access register. The contents of these read-only 32-bit registers
 are fixed, and this method verifies their values for the NI PCIe-6363.
 @return True if registers could be read and their contents verified; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363::CheckDeviceSignatures()
{
   LPCTSTR emsg = IsOn() ? NULL :  CDevice::EMSG_DEVNOTAVAIL;
   if(emsg == NULL && NIX::CHInCh_Signature != ReadReg32(NIX::REG_CHInCh_ID)) emsg = EMSG_BADCHINCHSIG;
   if(emsg == NULL)
   {
      u32 stcRev = ReadReg32(NIX::REG_Signature);
      if(NIX::STC_RevA != stcRev && NIX::STC_RevB != stcRev) emsg = EMSG_BADSTC3SIG;
   }
   if(emsg == NULL && NIX::NI6363_SSID != ReadReg32(NIX::REG_PCISubsystem_ID)) emsg = EMSG_BADSUBSYSTEMID;
   
   if(emsg != NULL) SetDeviceError(emsg);
   return(BOOL(emsg == NULL));
}

/**
 Traverse the device capabilities list in EEPROM area of register space and retrieve the calibration information for
 the AI and AO subsystems. 
 
 AI calibration data consists of four polynomial coefficients C3..C0 specific to a "mode", and a gain G and offset D 
 specific to a voltage range (+/-10V, +/-5V, etc). These are used to convert a raw ADC code X to the corresponding 
 voltage V in volts: V = (C3*X^3 + C2*X^2 + C1*X + C0) * G + D; Y = (C1*V + C0) * G + D. According to the DDK manual, 
 there are 4 AI "modes", but the DDK code ONLY USES THE FIRST MODE for scaling analog data!
 
 AO calibration data is similar, but there is only one AO mode and two mode coefficients C1,C0 in addition to the
 voltage-range-specific gain G and offset D: DAC code = (C1*x + C0) * G + D.
 
 Maestro only uses the +/-10V range for both AI and AO, so this method retrieves, computes, and saves the AI and AO
 coefficients for that voltage range. We use these to convert between ADC code and actual volts and between volts and 
 DAC code to at least achieve factory-calibrated accuracies.
 
 The EEPROM area is mapped into the device register address space. It stores the device capabilities list, which is
 organized as a linked-list of nodes, where each node references the address of the next node, and the last node points
 to null (0x00). The organization is rather complex; see the XSeries DDK reference manual for details. The code here is 
 adapted directly from the NI MHDDK class eepromHelper.  NOTE that, since the PCIe-6363 is NOT a simultaneous MIO
 device, EEPROM contents are accessed much like read-only registers, rather using a windowed register. The only tricky
 part is traversing the linked list....
 
 @return True if successful; false otherwise (device error msg set).
 */
BOOL RTFCNDCL CNI6363::GetCalibInfoFromEEPROM()
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   u32 capFlagPtr = EepromRead32(NIX::EEPROM_CapListFlagPtr);
   u32 capFlag = EepromRead32(capFlagPtr);
   u32 nodeAddr = EepromRead32( (capFlag & 0x01) ? NIX::EEPROM_CapListBPtr : NIX::EEPROM_CapListAPtr );

   // we traverse the entire capabilities list, but we skip over nodes we're not interested in. We're only interested in
   // the external calibration data, and specifically in the calibration coefficients, gain and offset for AI bipolar
   // +/-10V and AO bipolar +/-10V
   BOOL gotCal = FALSE;
   u16 nextNodeAddr = 0;
   u16 nodeId = 0;
   while(nodeAddr != 0)
   {
      nextNodeAddr = EepromRead16(nodeAddr);
      nodeId = EepromRead16(nodeAddr+2);
      
      // we're only interested in the device-specific node (there's just one), which has the calibration data. Once
      // we've parsed it, stop.
      if(nodeId == NIX::EEPROM_DevSpecificNode_ID)
      {
         gotCal = ParseCalibrationNodeInEEPROM(nodeAddr);
         break;
      }
      
      // move on to the next node. First two bits of the next node address indicate absolute (0) or relative (2) 
      // addressing; with those bits masked, it is the absolute address of the next node or the offset relative to the 
      // current node, resp. If address type is bad, STOP.
      u32 addrType = nextNodeAddr & 0x03;
      nextNodeAddr &= 0xFFFC;
      if(addrType == 0) nodeAddr = nextNodeAddr;
      else if(addrType == 2) nodeAddr += nextNodeAddr;
      else break;
   }

   if(!gotCal) SetDeviceError(EMSG_FAILEEPROMREAD);
   return(gotCal);
}

/** 
 Helper methods for GetCalibInfoFromEEPROM. These read EEPROM content of various sizes. These are much simpler than 
 analogous methods in the more generalized NI MHDDK, because we know apriori that the PCIe-6363 is NOT a simultaneous 
 MIO device, which uses a window register to access its EEPROM. We also know that onboard memory and Windows are both 
 little-endian, so we don't have to worry about endian swapping.
 @param addr Address of datum to be read, specified as an offset from the start of EEPROM.
 @return The u8, u16, u32, or f32 datum read.
*/
u8 RTFCNDCL CNI6363::EepromRead8(u32 addr) { return(ReadReg8(NIX::EEPROM_Start + addr)); }
u16 RTFCNDCL CNI6363::EepromRead16(u32 addr) { return(ReadReg16(NIX::EEPROM_Start + addr)); }
u32 RTFCNDCL CNI6363::EepromRead32(u32 addr) { return(ReadReg32(NIX::EEPROM_Start + addr)); }
f32 RTFCNDCL CNI6363::EepromReadF32(u32 addr)
{
   u8 f32Bytes[4];
   f32Bytes[0] = EepromRead8(addr++);
   f32Bytes[1] = EepromRead8(addr++);
   f32Bytes[2] = EepromRead8(addr++);
   f32Bytes[3] = EepromRead8(addr++);
   return( *(reinterpret_cast<f32*>(f32Bytes)) );
}

/**
 Helper method for GetCalibInfoFromEEPROM. This parses the device-specific node in which the calibration information is
 stored. It merely retrieves the calibration data we need, for the AI +/-10V range and the AO +/-10V range. These are
 the only AI and AO voltage ranges we use! See NI MHDDK class "eepromHelper" for details.
 @param addr Address at which the device-specific node begins, specified as an offset from the start of EEPROM.
 @return True if successful, false otherwise.
*/
BOOL RTFCNDCL CNI6363::ParseCalibrationNodeInEEPROM(u32 addr)
{
   // note: "body size" field includes body format and CRC; we only want size of body itself
   u32 bodySize = EepromRead32(addr + NIX::EEPROM_DSN_BodySizeOffset) - 2*sizeof(u32);
   u32 bodyFmt = EepromRead32(addr + NIX::EEPROM_DSN_BodyFormatOffset);

   // find the offsets to the A and B self-calibration sections; calibration coefficients are stored there.
   u32 selfCalAOffset = 0;
   u32 selfCalBOffset = 0;
   for(u32 i=0; i<bodySize;)
   {
      u32 value = 0;
      u32 id = 0;
      switch(bodyFmt)
      {
         case NIX::EEPROM_DSNBF_16BitValueID :
            value = EepromRead16(addr + NIX::EEPROM_DSN_BodyOffset + i);
            id = EepromRead16(addr + NIX::EEPROM_DSN_BodyOffset + i + sizeof(u16));
            i += 2 * sizeof(u16);
            break;
         case NIX::EEPROM_DSNBF_32BitValueID :
            value = EepromRead32(addr + NIX::EEPROM_DSN_BodyOffset + i);
            id = EepromRead32(addr + NIX::EEPROM_DSN_BodyOffset + i + sizeof(u32));
            i += 2 * sizeof(u32);
            break;
         case NIX::EEPROM_DSNBF_16BitIDValue :
            id = EepromRead16(addr + NIX::EEPROM_DSN_BodyOffset + i);
            value = EepromRead16(addr + NIX::EEPROM_DSN_BodyOffset + i + sizeof(u16));
            i += 2 * sizeof(u16);
            break;
         case NIX::EEPROM_DSNBF_32BitIDValue :
            id = EepromRead32(addr + NIX::EEPROM_DSN_BodyOffset + i);
            value = EepromRead32(addr + NIX::EEPROM_DSN_BodyOffset + i + sizeof(u32));
            i += 2 * sizeof(u32);
            break;
         default:  // should never happen
            return(FALSE);
      }
      
      switch(id)
      {
         case NIX::EEPROM_SelfCalAPtrID : selfCalAOffset = value; break;
         case NIX::EEPROM_SelfCalBPtrID : selfCalBOffset = value; break;
      }
   }
   
   // figure out which calibration section (A or B) to use; if both offsets non-zero, use the section with the higher
   // "CalWriteCount" (they should never be the same). If both are zero, then something went wrong!
   u32 currCalAddr = 0;
   if(selfCalAOffset > 0 && selfCalBOffset > 0)
   {
      u16 calADataSize = EepromRead16(selfCalAOffset);
      u32 calAWriteCount = EepromRead32(selfCalAOffset + sizeof(u16) + calADataSize);
      u16 calBDataSize = EepromRead16(selfCalBOffset);
      u32 calBWriteCount = EepromRead32(selfCalBOffset + sizeof(u16) + calBDataSize);

      if(calAWriteCount > calBWriteCount) currCalAddr = selfCalAOffset;
      else if(calBWriteCount > calAWriteCount) currCalAddr = selfCalBOffset;
      else return(FALSE);
   }
   else if(selfCalAOffset > 0) currCalAddr = selfCalAOffset;
   else if(selfCalBOffset > 0) currCalAddr = selfCalBOffset;
   else return(FALSE);
   
   // retrieve calibration data for AI +/-10V (mode index 0, interval index 0), then compute and save the coefficients 
   // of the 3rd-degree polynomial used to convert ADC code to volts for this input range. There's only one ADC on the
   // PCIe-6363, so there's only one AI calibration section. Note how we skip over other modes and intervals. An 
   // interval consists of a gain and offset. A gain of 0 is illegal, in which case we FAIL.
   currCalAddr += NIX::EEPROM_CalCoeffOffset;
   u8 modeOrder = EepromRead8(currCalAddr);
   for(int i=0; i<NIX::EEPROM_Cal_NumModeCoeffs; i++) 
      m_aiCoeffs[i] = EepromReadF32(currCalAddr + sizeof(u8) + i*sizeof(f32));
   currCalAddr += NIX::EEPROM_Cal_NumAIModes * NIX::EEPROM_Cal_ModeSizeInBytes;
   u32 tmp = EepromRead32(currCalAddr);
   if(tmp == 0) return(FALSE);
   f32 gain = EepromReadF32(currCalAddr);
   f32 offset = EepromReadF32(currCalAddr + sizeof(f32));
   currCalAddr += NIX::EEPROM_Cal_NumAIIntervals * NIX::EEPROM_Cal_IntervalSizeInBytes;
   
   for(int i=0; i<CNI6363::NUMAICOEFFS; i++) m_aiCoeffs[i] *= gain;
   m_aiCoeffs[0] += offset;
   
   // for each of the 4 DACS on the NI6363: retrieve calibration data for AO +/-10V (interval index 0; there's only one 
   // mode and we don't use the coeffs stored in it), then save the interval gain and offset as the coefficients of the 
   // 1st-degree polynomial used to convert volts to DAC code for this output range.
   for(int i=0; i<CNI6363::NUMDACS; i++)
   {
      currCalAddr += NIX::EEPROM_Cal_ModeSizeInBytes;
      tmp = EepromRead32(currCalAddr);
      if(tmp == 0) return(FALSE);
      m_aoCoeffs[i][1] = EepromReadF32(currCalAddr);
      m_aoCoeffs[i][0] = EepromReadF32(currCalAddr + sizeof(f32));
	  currCalAddr += NIX::EEPROM_Cal_NumAOIntervals * NIX::EEPROM_Cal_IntervalSizeInBytes;
   }

   return(TRUE);
}

/**
 Read and write to each individual bit of two "scratchpad" registers on the PCIe-6363, one in the CHInCh interface and
 one in the DAQ-STC3 timing engine. These registers have no effect on hardware function; they offer a "sanity check" to
 see if we're unable to communicate with the hardware via our mapped register address space.
 @return True if able to read and write the scratch pad registers; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363::TestScratchPadRegisters()
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
   if(!bOk) SetDeviceError(EMSG_FAILRWTEST_CHINCH);
   
   // repeat for the DAQ-STC's scratch pad register
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
   if(!bOk) SetDeviceError(EMSG_FAILRWTEST_STC);
   return(bOk);
}

/**
 This method is intended to put the PCIe-6363 in a known, inactive state during application startup and shutdown. It is
 invoked in OnOpen() prior to setting up the "subsystem devices" implemented on the board, and in OnClose() after 
 closing those subdevices. The method strobes bit0 in the Joint_Reset_Register to reset the DAQ-STC3 timing engine, then 
 goes through the sequence recommended in the XSeries DDK manual to disable and acknowledge all board interrupts.
 @return True if successful; false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363::ResetSTC()
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

   // DAQ-STC3 software reset
   WriteReg16(NIX::REG_Joint_Reset, 0x01);
   
   // disable and ack all interrupts at board level
   u32 cmd = NIX::IMR_Clear_CPU_Int | NIX::IMR_Clear_STC3_Int;
   WriteReg32(NIX::REG_Interrupt_Mask, cmd);
   
   // disable and ack all DMA channel interrupts and ensure all DMA channels are stopped
   u32 addrCHCR = NIX::REG_DMACh1_Control;
   u32 addrCHOR = NIX::REG_DMACh1_Operation;
   for(u32 i=0; i<NIX::NumDMAChannels; i++)
   {
      WriteReg32(addrCHCR, NIX::DMAChCR_DisableInts_Cmd);
      WriteReg32(addrCHOR, NIX::DMAChOR_Stop_Cmd);
      addrCHCR += NIX::DMACh_RegOffset;
      addrCHOR += NIX::DMACh_RegOffset;
   }
   
   // disable propagation of all subsystem interrupts to the CHInCh
   WriteReg32(NIX::REG_GlobalInterruptEnable, NIX::GIER_DisableAll_Cmd);
   
   // disable and ack all subsystem interrupts via the Interrupt_2_Register for each subsystem
   WriteReg32(NIX::REG_AITimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   WriteReg32(NIX::REG_DITimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   WriteReg32(NIX::REG_AOTimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   WriteReg32(NIX::REG_DOTimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   u32 addrInt2R = NIX::REG_G0_Interrupt_2;
   for(u32 i=0; i<NIX::NumCounters; i++)
   {
      WriteReg32(addrInt2R, NIX::Int2_DisableAndAckAll_Cmd);
      addrInt2R += NIX::Gi_RegOffset;
   }
   WriteReg32(NIX::REG_Gen_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   
   return(TRUE);
}


//===================================================================================================================== 
// CNI6363_AI
//
// This class implements CCxAnalogIn and manages the analog input subsystem onboard the PCIe-6363. As a friend class of
// CNI6363, it uses a pointer to the parent device object to access that object's private read/write register methods.
//
// IMPLEMENTATION CONSIDERATIONS
// 1) While the PCIe-6363 sports 32 analog inputs, we declare it as having only 16. For now, we're not interested in
// using the additional inputs. In Maestro3, only 10 inputs are dedicated to a particular use.
//
// 2) Even though the PCIe-6363 has 16-bit analog inputs, CNI6363_AI will for now only support 12-bit. There are several 
// reasons for this:
//    a) MaestroRTSS's analog data compression algorithm relies on the raw ADC codes being 12-bit.
//    b) To achieve the 16-bit accuracy on the PCIe-6363 requires a non-linear transformation of the 16-bit ADC code to 
//       a voltage, but MaestroRTSS assumes a linear relationship and, in fact, uses the ADC codes directly when
//       doing comparisons with calculated target position.
// 
// Each analog datum acquired by the PCIe-6363 will be a 16-bit ADC code X. To convert this to the closest 12-bit ADC 
// code Y, given MaestroRTSS's assumption of a linear relationship between ADC code and voltage, we must:
//    a) Use the onboard calibration data to convert the 16-bit ADC code to the corresponding calibrated voltage it
//       represents. This is done by a parent device method, V = CNI6363::ADCToVolts(X).
//    b) Convert back to a 12-bit ADC code using the assumed linear relation: Y = V * 4096 / 20.0 volts, where Y is
//       restricted to the range [-2048..2047].
// 
// Note that the CCxAnalogIn methods ToRaw(), ToVolts(), and NearestVolts() all assume the linear relation between
// ADC code and voltage, and will assume the device is 12-bit since we declare it as such. The conversion from 16-bit
// to 12-bit is hidden from the base class.
//
// 3) In the old E-Series PCI-MIO-16E1 device employed prior to Maestro 3, we used the "ghost channel" feature to 
// implement multi-rate sampling when Maestro's "fast" channel (25KHz, or 40us) was engaged. WHILE NOT DOCUMENTED IN
// THE NI MHDDK, THIS FEATURE IS STILL PRESENT IN THE X-SERIES. Without it, we would have to collect 193 samples 
// throughout the scan interval, unload them from the data FIFO, and discard the ones we don't need. Testing showed
// that this was not feasible -- the average time spent unloading the FIFO was nearly 600us when the fast channel was
// engaged. By using the ghost feature, we only load into the FIFO what we actually want to keep. Testing also showed
// that a typical read of the data FIFO took ~2.5us, so the cost of unloading 41 samples per 1ms scan interval (16 
// channels in the slow set, plus 25 samples of the fast channel) is on the order of only 100us.
//===================================================================================================================== 

/**
 Construct a CNI6363_AI "subdevice" object representing the analog input subsystem on the PCIe-6363. The subdevice must
 be opened via CDevice::Open() before it can be used.
 
 @param pNI6363 Pointer to the parent device object, through which this subdevice will access registers onboard the
 physical PCIe-6363 device.
*/
CNI6363_AI::CNI6363_AI(CNI6363* pNI6363) : CCxAnalogIn(CNI6363::DEVINFO, 1, FALSE, CNI6363_AI::NUM_AI)
{
   m_pNI6363 = pNI6363;
   
   m_aiState = aiUNKNOWN;
   m_nScanIntvUS = 0;
   m_nScanChannels = 1;
   m_IsFastChEna = FALSE;
   m_nSlots = 0;
   m_iNextSlot = 0;

   m_soft_AI_Trigger_Select = 0;
   m_soft_AI_Trigger_Select2 = 0;
   m_soft_AIT_Mode_1 = 0;
   m_soft_AIT_Mode_2 = 0;
}

/**
 [CDevice impl] Resets the AI subsystem and its input timing control circuitry, disables and acks all AI-related
 interrupts, and clears the AI data FIFO. Any ongoing DAQ sequence will be terminated by this method.
 @return True if successful, false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363_AI::Init()
{
   if(!IsOn()) { m_aiState = aiERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   // reset AI subsystem and its input timer circuitry
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_CfgStart);
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_Reset);

   // after reset, ensure exported convert polarity in the AITimer's Mode_1 register is initialized to active-low.
   // All other bit fields in the register are zero'd by reset.
   m_soft_AIT_Mode_1 = NIX::IT_Mode_1_ExpCP;
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_1, m_soft_AIT_Mode_1);
   
   // After the s/w reset, various AI and AITimer register values will be modified by strobing the Reset bit.
   // Where needed, we keep s/w copies of the registers we'll need, and here we reset the s/w copies to 0. This is
   // sort-of what the MHDDK does, except that it also marks the s/w copies as "dirty".
   m_soft_AI_Trigger_Select = 0;
   m_soft_AI_Trigger_Select2 = 0;
   m_soft_AIT_Mode_2 = 0;
   
   // ensure AI subsystem interrupts are disabled
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg32(NIX::REG_Interrupt_Mask, NIX::IMR_Clear_CPU_Int | NIX::IMR_Clear_STC3_Int);
   m_pNI6363->WriteReg32(NIX::REG_GlobalInterruptEnable, NIX::GIER_DisableAll_Cmd);
   
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_FIFOClear);
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_CfgEnd);
   
   m_aiState = aiINITD;
   return(TRUE);
}

/**
 [CCxAnalogIn impl] Configure, but do not start, a continuous-mode acquisition sequence to scan AI channels [0..N) as
 simultaneously as possible at the beginning of each scan epoch (within the first 100us of the start of each scan). In
 addition, optionally sample a specified channel at 25KHz (40us sample period) throughout each scan epoch. If a data
 acquisition sequence is in progress when this method is called, Init() is called first to reset the AI subsystem.
 
 NOTES:
 1) The PCIe-6363 has the "ghost channel" feature of the PCI-MIO_16E1, BUT THIS FEATURE IS NOT DOCUMENTED in the NI
 X-Series MHDDK. The feature makes multi-rate sampling possible when the 25KHz "fast channel" was enabled. A "ghost" 
 channel in the AI configuration FIFO is sampled, but the sample is NOT placed in the FIFO. Thus, if the slow-scan set
 is 16 channels and the scan interval is 1ms, a total of 41 samples (16 slow, 25 fast) are stored in the FIFO. 
    I initially assumed the "ghost" feature did not exist for the PCIe-6363. In this case, the AI had to sample every
 5us throughout a 1-ms scan epoch (except the last 40us), resulting in 193 samples per scan. Unload() had to unload all
 193 samples and save only the relevant ones. Testing showed this wasn't feasible, as a typical read of the data FIFO
 cost 2.5us, and a typical call to Unload took ~600us, more than half the entire scan epoch. This does not leave enough
 time for all the other work MaestroRTSS does during a scan epoch.
 2) Taking advantage of the undocumented "ghost" feature, segregation of the slow and fast data streams requires 
 tagging each SAVED sample slot in the scan epoch as belonging to the slow or fast stream. This is done in Configure()
 and the sample slot tags are used by Unload().
 2) In the NI MHDDK examples, the software "spins" on the SCArmed_St bit in the AI_Status_1 register to confirm that the
 system has armed after strobing the various arm bits in a single write to the AI_Command register. The examples wait
 up to 5 seconds! We CANNOT do this. Instead, we spin on the SCArmed_St bit for up to 300us. If it has not set within
 this time, then we assume arming has failed. In this case, the AI subsystem is disarmed and Configure() fails. It will
 be important for MaestroRTSS to check the return value of Configure -- currently, this does not happen!!
 
 @param nCh The number of channels, N, in the slow scan set; channels [0..N-1] are sampled as rapidly as possible within
 the first 100us of the scan epoch. MUST be valid and >0!!
 @param nScanIntv Scan interval in microseconds; MUST be >=1000us and divisible by 5us. When 25KHz data stream (the 
 "fast channel" is engaged, it must be divisible by 40us AND must not exceed 2400us.
 @param iChFast If this is a valid channel number, sample that channel at 25KHz; else, only sample the "slow scan" set.
 @param bInt If TRUE, enable the "start-of-scan" interrupt for DAQ.
 @return True if board successfully configured; false if device unavailable or any parameters are invalid. 
*/
BOOL RTFCNDCL CNI6363_AI::Configure(int nCh, int nScanIntv, int iChFast, BOOL bInt)
{
   if(!IsOn()) { m_aiState = aiERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   if(m_aiState != aiINITD && m_aiState != aiSTOPPED && !Init()) return(FALSE);
   
   // will fast-data channel be enabled?
   BOOL bEnaFast = BOOL((iChFast>=0) && (iChFast<GetNumChannels()));
   // validate arguments. Note that interrupt is not permitted if an ISR is not installed.
   if( (bInt && !HasInterruptHandler()) || (nCh < 1) || (nCh > GetNumChannels()) || 
       (nScanIntv < 1000) || (nScanIntv % 5 != 0) || (bEnaFast && ((nScanIntv > 2400) || (nScanIntv % 40 != 0))) )
   {
      m_aiState = aiERROR;
      SetDeviceError(CDevice::EMSG_USAGE);
      return(FALSE);
   }
   
   // timing parameters: We use the internal 100MHz timebase for scan and convert clocks, so the scan and convert
   // periods are expressed in #ticks of this timebase. Scan and convert delays are both set to the min. allowed value.
   // The convert period is always 5us, or 500 ticks of the 100MHz timebase.
   u32 scanDelay = 2;
   u32 scanPeriod = (u32) (nScanIntv * 100);   // (us * 1000 ns/us) / 10ns/tick = #ticks
   u32 cvtDelay = 2;
   u32 cvtPeriod = (u32) 500;
   
   // begin configuration sequence -- suspend AI timing circuitry
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_CfgStart);
   
   // disable external gating of scan clock: b23..16 = 0 in AI_Trigger_Select register
   m_soft_AI_Trigger_Select = (m_soft_AI_Trigger_Select & 0xFF00FFFF);
   m_pNI6363->WriteReg32(NIX::REG_AI_Trigger_Select, m_soft_AI_Trigger_Select);
   
   // START1 trigger -- configure DAQ to start on the rising edge of a "software pulse": b7..0 = 0x40 in the
   // AI_Trigger_Select register. Note that we're doing a separate write from the previous step!
   m_soft_AI_Trigger_Select = (m_soft_AI_Trigger_Select & 0xFFFFFF00) | 0x0040;
   m_pNI6363->WriteReg32(NIX::REG_AI_Trigger_Select, m_soft_AI_Trigger_Select);
   
   // START trigger -- configure scan clock to use internal timebase, active on rising edge: b23..16 = 0x40 in the
   // AI_Trigger_Select2 register.
   m_soft_AI_Trigger_Select2 = (m_soft_AI_Trigger_Select2 & 0xFF00FFFF) | 0x00400000;
   m_pNI6363->WriteReg32(NIX::REG_AI_Trigger_Select2, m_soft_AI_Trigger_Select2);
   
   // CONVERT trigger -- scan clock starts convert clock; convert clock active on falling edge: b31..24 = 0x80 in the
   // AI_Trigger_Select register.
   m_soft_AI_Trigger_Select = (m_soft_AI_Trigger_Select & 0x00FFFFFF) | 0x80000000;
   m_pNI6363->WriteReg32(NIX::REG_AI_Trigger_Select, m_soft_AI_Trigger_Select);
   
   // configure AI timing parameters (see MHDDK inTimerHelper.programTiming() for details):
   //   1) Ext_MUX_Preset = Every_Convert, Start_Stop_Gate_Ena = Disabled, Trigger_Once = 1, Continuous = 1,
   // PreTrigger = post-trigger, SC_Initial_Load_Src = Load_A, SC_Reload_Mode = SC_Reload_No_Change: Flush these to 
   // AITimer's Mode_1_Register.
   //   2) Write scan count of -1 (don't care) into AITimer's SC_Load_A_Register, then load it via Command_Register.
   //   3) Start1_Export_Mode = ExportSyncStart1, Start2_Export_Mode = ExportMaskedStart2, Start_Trigger_Len =
   // ExportSyncStart, SyncMode = SyncDefault, HaltOnError = 1 : Flush these changes to AITimer's Mode_2_Register.
   //   4) We're using internal timebase for SI counter. SI_Src_Select = SI_Src_TB3, SI_Src_Polarity = Rising Edge,
   // SI_Initial_Load_Src = Load_A, SI_Reload_Mode = SI_Reload_No_Change : Flush these changes to Mode_1_Register.
   //   5) Write (scan delay - 1) into SI_Load_A_Register, then load it via Command_Register.
   //   6) Write (scan period - 1) into SI_Load_B_Register.
   //   7) SI_Initial_Load_Src = Load_B.  Also, since we're using internal SI2 counter (convert clock triggered by scan 
   // clock TC), SI2_Initial_Load_Src = Load_A, and SI2_Reload_Mode = SI2_Reload_Alt_First_Period_Every_STOP : Flush 
   // these changes to Mode_1_Register.
   //   8) SI2_Src_Select = SI2_Src_Is_SI_Src : Flush this change to Mode_2_Register.
   //   9) Write (convert delay - 1) into SI2_Load_A_Register, then load it via Command_Register.
   //   10) Write (convert period - 1) into SI2_Load_B_Register. Then set SI2_Initial_Load_Src = Load_B in Mode_1_Reg.
   m_soft_AIT_Mode_1 = (m_soft_AIT_Mode_1 & 0xFFFC8FF9) | 0x00030000;
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_1, m_soft_AIT_Mode_1);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_SCLoadA, 0xFFFFFFFF);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_SCLoad);
   m_soft_AIT_Mode_2 = (m_soft_AIT_Mode_2 & 0xBFC1FF7F) | 0x40020080;
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_2, m_soft_AIT_Mode_2);
   m_soft_AIT_Mode_1 = (m_soft_AIT_Mode_1 & 0xF82FFF0F);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_1, m_soft_AIT_Mode_1);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_SILoadA, scanDelay - 1);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_SILoad);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_SILoadB, scanPeriod - 1);
   m_soft_AIT_Mode_1 = (m_soft_AIT_Mode_1 & 0xFFFFFC7F) | 0x00000180;
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_1, m_soft_AIT_Mode_1);
   m_soft_AIT_Mode_2 = (m_soft_AIT_Mode_2 & 0xF7FFFFFF);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_2, m_soft_AIT_Mode_2);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_SI2LoadA, cvtDelay - 1);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_SI2Load);
   m_pNI6363->WriteReg32(NIX::REG_AITimer_SI2LoadB, cvtPeriod - 1);
   m_soft_AIT_Mode_1 = (m_soft_AIT_Mode_1 & 0xFFFFFFDF) | 0x00000200;
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Mode_1, m_soft_AIT_Mode_1);

   // AI_FifoWidth = 2 byte, AIDoneNotificationEnable = 0 : Flush changes to AI_Data_Mode_Register.  We have to clear 
   // FIFO afterward to ensure DAQ_STC registers the change.
   m_pNI6363->WriteReg32(NIX::REG_AI_Data_Mode, 0);
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_FIFOClear);

   // clear the AI channel configuration FIFO
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_CfgMemoryClear);

   // program the configuration FIFO. Let N = #channels in the "slow scan" set. If "fast channel" is enabled, then let
   // F be the designated channel number for the fast channel.
   // -- Slow scan channel set is sampled once at the beginning of the scan epoch, in order: 0, 1, .., N-1. The 
   // sample slot disposition array is NOT used in this case.
   // -- If "fast channel" F is enabled, then F must be sampled at every 40us epoch in the scan interval, starting at
   // t=0us. If the scan interval is M us, then we have to sample every 5 us from t=0 to t=M-40us. At the beginning,
   // we sample the slow-scan set in channel order, except that we must sample F at every 40us epoch. After we're done
   // scanning the slow-scan set, then we simply sample F every 5 us until t=M-40. Those samples of F that are not at
   // 40us epochs are configured as "ghosts", in which case the sample is NOT stored in the FIFO. The sample slot 
   // disposition array indicates to which stream (slow = 0, fast = 1) each sample belongs. This is critical for 
   // segregating the two streams as we unload the FIFO on the fly during runtime.
   // -- All channels sampled are configured for bipolar +/-10V range (h/w gain = 1), NRSE termination, with dithering
   // enabled to increase ADC accuracy. We only use the first 16 channels, which are in "Bank 0".
   if(!bEnaFast)
   {
      for(int i=0; i<nCh; i++)
      {
         u16 cfg = NIX::AICfg_Standard | ((u16) i);
         if(i==nCh-1) cfg |= NIX::AICfg_LastCh;
         m_pNI6363->WriteReg16(NIX::REG_AI_Cfg_FIFO_Data, cfg);
      }
      m_nSlots = 0;
   }
   else
   {
      m_nSlots = 0;
      int slowCh = 0;
      int nLast = (nScanIntv-40)/5;
      for(int i=0; i<=nLast; i++)
      {
         u16 cfg = NIX::AICfg_Standard;
         if(i % 8 == 0)
         {
            cfg |= (u16) iChFast;
            m_slots[m_nSlots++] = (u8) 1;
         }
         else if(slowCh < nCh) 
         {
            cfg |= (u16) slowCh++;
            m_slots[m_nSlots++] = (u8) 0;
         }
         else
         {
            // "ghost" channel: sample will NOT be saved to FIFO
            cfg = NIX::AICfg_Ghost | ((u16) iChFast);
         }
         if(i==nLast) cfg |= NIX::AICfg_LastCh;
         m_pNI6363->WriteReg16(NIX::REG_AI_Cfg_FIFO_Data, cfg);
      }
   }
   
   // advance configuration FIFO to the first entry
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_LocalClkMUXPulse);
   
   // end AI configuration sequence
   m_pNI6363->WriteReg16(NIX::REG_AITimer_Reset, NIX::ITReset_CfgEnd);

   // IF interrupt enabled, enable the AI_Start interrupt (start of scan interrupt)
   if(bInt)
   {
      // enable interrupt signalling on the PCIe-6363
      m_pNI6363->WriteReg32(NIX::REG_Interrupt_Mask, NIX::IMR_Set_CPU_Int | NIX::IMR_Set_STC3_Int);
      // allow propagation of AI subsystem interrupts to the CHInCh of the PCIe-6363
      m_pNI6363->WriteReg32(NIX::REG_GlobalInterruptEnable, NIX::GIER_EnableAI_Cmd);
      // enable ONLY the "start-of-scan" interrupt from the AI timing engine
      m_pNI6363->WriteReg32(NIX::REG_AITimer_Interrupt_1, NIX::AITInt1_StartEnable);
   }
   
   // arm the timing engine: As required in the MHDDK, we arm the SC, SI, SI2, and DIV counters in a single write to
   // the AITimer_Command register. The MHDDK verifies that the timing engine is armed by polling the AITimer_Status_1 
   // register for up to 5 seconds. We only wait up to 300us here, but the hope is that it's an extremely rare event
   // that arming takes any length of time at all.
   m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_ArmAll);
   CElapsedTime eTime;
   BOOL armed = FALSE;
   while(!armed)
   {
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_AITimer_Status1);
      if((stat & NIX::ITStatus1_SC_Armed) != 0) armed = TRUE;
      else if(eTime.Get() > 300.0) break;
   }
   if(!armed)
   {
      Init();
      m_aiState = aiERROR;
      SetDeviceError( "PCIe-6363 AI Timing Engine failed to arm!");
      return(FALSE);
   }
   
   m_nScanIntvUS = nScanIntv; 
   m_nScanChannels = nCh; 
   m_IsFastChEna = bEnaFast;
   m_iNextSlot = 0;
   m_aiState = aiREADY;
   return(TRUE);
}

/**
 [CCxAnalogIn impl] Start a previously configured DAQ sequence NOW. This method takes no action if the device is 
 unavailable or if it is not in the "ready-to-start" state. Starting the sequence is merely a matter of strobing a bit 
 in a command register and involves a single register write, so it should be VERY fast.
*/
VOID RTFCNDCL CNI6363_AI::Start()
{
   if(IsOn() && m_aiState == aiREADY)
   {
      m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_Start1_Pulse);
      m_aiState = aiRUNNING;
   }
}

/**
 [CCxAnalogIn impl] Stop an ongoing DAQ sequence NOW. This method takes no action if the device is unavailable or if it
 is not in the "running" state. It disarms the timing engine and disables the AI "start-of-scan" interrupt, in case it
 was enabled during the DAQ sequence. The data FIFO is left undisturbed.
 
 NOTE: MHDDK suggests using End_on_SC_TC or End_on_End_of_Scan to stop acquisition in cont mode, but we want to stop as 
 soon as possible, so we use the Disarm strobe. We also do NOT spin waiting for the timing engine to disarm.
*/
VOID RTFCNDCL CNI6363_AI::Stop()
{
   if(IsOn() && m_aiState == aiRUNNING)
   {
      RtDisableInterrupts(); 
      m_pNI6363->WriteReg32(NIX::REG_AITimer_Command, NIX::ITCmd_Disarm);
      m_pNI6363->WriteReg32(NIX::REG_Interrupt_Mask, NIX::IMR_Clear_CPU_Int | NIX::IMR_Clear_STC3_Int);
      m_pNI6363->WriteReg32(NIX::REG_AITimer_Interrupt_2, NIX::AITInt2_StartAckAndDisable);
      m_pNI6363->WriteReg32(NIX::REG_GlobalInterruptEnable, NIX::GIER_DisableAll_Cmd);
      m_aiState = aiSTOPPED;
      RtEnableInterrupts();
   }
}

/**
 [CCxAnalogIn impl] Unload samples from the AI FIFO, optionally blocking until the requested # has been retrieved. 
 Segregate data from the slow scan set and the fast channel (if enabled) into the provided buffers. 

 If a DAQ or other error is detected, call GetDeviceError() to retrieve the error condition string. Once a DAQ error 
 occurs, further calls to this method should fail until a new DAQ is configured and started. The possible errors: 
    a) DAQ error -- Scan overrun, FIFO overrun or overflow (these errors are reported by the device).
    b) timeout -- If the wait flag is set and the DAQ freezes because of a hardware failure, Unload() could BLOCK 
 forever. To avoid this, we abort when the elapsed time exceeds the time it should take to acquire the desired # of 
 slow data samples. 
    c) fast data buffer too small -- Callers should ensure the fast data stream buffer is appropriately sized. For 
 example, if Unload() is invoked every N slow scan intervals of duration D microsecs, then the # of samples available
 from the fast data stream on each call will be between N*D/40 and (N+1)*D/40. For safety's sake, the fast data stream 
 buffer should be at least (N+1)*D/40 samples long. Failure to do so could cause this error.

 IMPLEMENTATION NOTES:
 1) When the fast channel is enabled, we need to segregate the "slow-data" and "fast-data" streams. Configure() 
 initializes a sample slot disposition array that indicates to which stream each sample in a scan epoch belong. We keep
 track of our position in this array, since the method will stop unloading as soon as it has collected the requested
 number of slow data sample.
 2) For now we only support 12-bit ADC. To convert NI-6363 16-bit ADC codes to 12-bit codes properly, we first convert
 the 16-bit sample to its corresponding calibrated voltage via CNI6363::ADCToVolts. We then use CCxAnalogIn::ToRaw() to
 LINEARLY convert the voltage value to the corresponding 12-bit DAC code (rem: in the CNI6363_AI constructor, the
 base class is "informed" that the device has 12-bit resolution).
 
 @param pSlow Output buffer for samples (raw DAC codes) from slow scan set; allocated by caller. 
 @param nSlow [in/out] Size of buffer for the slow scan data stream; on return, #samples actually retrieved.
 @param pFast Output buffer for samples (raw DAC codes) from fast data (25KHz) channel; allocated by caller.
 @param nFast [in/out] Size of buffer for the fast data stream; on return, the #samples actually retrieved.
 @param bWait [in] If set, keep polling FIFO until desired #samples are retrieved from slow data stream, or an error is 
 detected; else, we retrieve only what's immediately available from the FIFO.

 @return True if successful, false if an error has occurred (DAQ operation is aborted).
*/
BOOL RTFCNDCL CNI6363_AI::Unload(short *pSlow, int& nSlow, short *pFast, int& nFast, BOOL bWait)
{
   // buffer sizes for the two data streams; initialize #samples unloaded thus far to zero
   int nSlowSize = nSlow;
   int nFastSize = nFast;
   nSlow = 0;
   nFast = 0;

   // do nothing if a prior DAQ error occurred, or if the device is unavailable
   if(m_aiState == aiERROR) return(FALSE);
   if(!IsOn()) { m_aiState = aiERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

   // calculate worst-case wait time (so we don't block forever), index of last sample in scan epoch when fast data ena.
   double dWait = (!bWait) ? 0.0 : (nSlowSize/m_nScanChannels + 1) * m_nScanIntvUS;
   
   // retrieve requested #samples from slow data stream, plus any samples available from the fast data stream, if the
   // fast-data channel is enabled....
   BOOL done = FALSE;
   i16 sample;
   f32 volts;
   CElapsedTime eTime; 
   while(!done) 
   {
      // get current status of DAQ and abort if an error condition has been detected
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_AITimer_Status1);
      if((stat & NIX::ITStatus1_AnyError) != 0)
      {
         if((stat & NIX::ITStatus1_Overflow) == NIX::ITStatus1_Overflow) SetDeviceError(CCxAnalogIn::EMSG_DAQ_OVERFLOW); 
         else if((stat & NIX::ITStatus1_Overrun) == NIX::ITStatus1_Overflow) SetDeviceError(CCxAnalogIn::EMSG_DAQ_OVERRUN); 
         else SetDeviceError(CCxAnalogIn::EMSG_DAQ_OTHER);
         m_aiState = aiERROR;
         return(FALSE);
      }
      
      // get #samples that are in the FIFO right now. If there are none, return immediately if we're not blocking; else
      // check to see if we've timed out.
      u32 samplesReady = m_pNI6363->ReadReg32(NIX::REG_AI_Data_FIFO_Status);
      if(samplesReady == 0)
      {
         if(!bWait) return(TRUE);
         else if(eTime.Get() > dWait)
         {
            SetDeviceError(CCxAnalogIn::EMSG_DAQ_TIMEOUT);
            m_aiState = aiERROR;
            return(FALSE);
         }
      }

      // unload samples that are in the FIFO right now, until we have the requested number of slow-data samples. Two
      // cases are handled separately: fast channel disabled VS fast channel enabled. In the latter case, we have to
      // segregate the two streams and discard numerous samples, so there's more work involved.
      if(!m_IsFastChEna) for(u32 i=0; i<samplesReady && !done; i++)
      {
         // get the next sample
         sample = (i16) m_pNI6363->ReadReg16(NIX::REG_AI_FIFO_Data16);

         // convert 16-bit ADC -> calibrated voltage -> corresponding 12-bit ADC code (linear)
         volts = m_pNI6363->ADCToVolts(sample);
         *(pSlow+nSlow) = (short) ToRaw((float)volts);
         ++nSlow;
         
         done = BOOL(nSlow == nSlowSize);
      }
      else for(u32 i=0; i<samplesReady && !done; i++)
      {
         // get the next sample. Convert 16-bit ADC -> calibrated voltage -> 12-bit ADC
         sample = (i16) m_pNI6363->ReadReg16(NIX::REG_AI_FIFO_Data16);
         volts = m_pNI6363->ADCToVolts(sample);
         sample = (i16) ToRaw((float)volts);

         // segregate the two data streams
         if(m_slots[m_iNextSlot] == 0)
         {
            *(pSlow+nSlow) = (short) sample;
            ++nSlow;
         }
         else if(nFast == nFastSize)
         {
            SetDeviceError(CCxAnalogIn::EMSG_DAQ_LOSTFASTDATA);
            m_aiState = aiERROR;
            return(FALSE);
         }
         else
         {
            *(pFast+nFast) = (short) sample;
            ++nFast;
         }

         // proceed to next slot
         m_iNextSlot = (m_iNextSlot + 1) % m_nSlots;         
         done = BOOL(nSlow == nSlowSize);
      }
   }

   return(TRUE);
}

/**
 [CCxAnalogIn impl] Check the empty state of the AI data FIFO.
 @return True if AI data FIFO is currently empty, regardless whether a DAQ is in progress or not! Will also return TRUE
 if the device is  not available! 
*/
BOOL RTFCNDCL CNI6363_AI::IsEmpty()
{
   BOOL empty = TRUE;
   if(IsOn())
   {
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_AITimer_Status1);
      empty = BOOL((stat & NIX::ITStatus1_FIFO_Empty) != 0);
   }
   return(empty);
}

/**
 [CCxAnalogIn impl] Detect and acknowledge the "start-of-scan" AI interrupt. This method does not disable interrupts
 while checking. If it is called from an ISR, be sure to disable OS interrupts before calling!
  
 The AI start-of-scan interrupt is the ONLY interrupt we EVER enable on the PCIe-6363, and it is the only interrupt used
 in Maestro. So this method ONLY checks for and acknowledges this interrupt and no other kind of interrupt in the AI
 subsystem or any other subsystem of the device!
 
 @return True iff a DAQ is in progress and the "start-of-scan" interrupt was detected (and acknowledged).
*/
BOOL RTFCNDCL CNI6363_AI::IntAck()
{
   if(m_aiState != aiRUNNING) return(FALSE);

   BOOL detected = FALSE;
   u16 stat = m_pNI6363->ReadReg16(NIX::REG_AI_Interrupt_Status);
   if((stat & NIX::AIIntStatus_StartIRQ) != 0)
   {
      m_pNI6363->WriteReg32(NIX::REG_AITimer_Interrupt_1, NIX::AITInt1_StartAck);
      detected = TRUE;
   }
   return(detected);
}


/**
 This function is intended to assess how much time it takes to read AI samples out of the AI data FIFO. It performs a
 short DAQ of about 4000 samples over the course of about 400ms. It then measures the time it takes to unload all of
 the collected samples from the onboard FIFO and reports the average time per sample on the console.
 
 For maximum accuracy, be sure the calling thread has maximum real-time priority and take pains to ensure it won't be
 preempted!
*/
VOID RTFCNDCL CNI6363_AI::TestReadFIFOPerformance()
{
   // run a short acquisition of 10 channels sampled at 1KHz for ~400ms. This is roughly 4000 samples, which is less
   // than the NI-6363 AI data FIFO's size.
   ::printf("NI-6363 read FIFO test:\n");
   if(!Configure(10, 1000, -1, FALSE))
   {
      ::printf("   FAILED: %s\n", GetLastDeviceError());
      return;
   }
   CElapsedTime eTime;
   Start();
   long i = 0; while(eTime.Get() < 400000.0) ++i;
   Stop();
   
   // test fails if a DAQ error occurred during the brief acquisition sequence
   u32 stat = m_pNI6363->ReadReg32(NIX::REG_AITimer_Status1);
   if((stat & NIX::ITStatus1_AnyError) != 0)
   {
      if((stat & NIX::ITStatus1_Overflow) == NIX::ITStatus1_Overflow) SetDeviceError(CCxAnalogIn::EMSG_DAQ_OVERFLOW); 
      else if((stat & NIX::ITStatus1_Overrun) == NIX::ITStatus1_Overflow) SetDeviceError(CCxAnalogIn::EMSG_DAQ_OVERRUN); 
      else SetDeviceError(CCxAnalogIn::EMSG_DAQ_OTHER);
      m_aiState = aiERROR;

      ::printf("   FAILED: %s\n", GetLastDeviceError());
      return;
   }
   
   // get #samples that are in the FIFO. 
   u32 nSamples = m_pNI6363->ReadReg32(NIX::REG_AI_Data_FIFO_Status);
   if(nSamples == 0)
   {
      ::printf("   FAILED: No samples in data FIFO??\n");
      return;
   }
   
   // measure how long it takes to unload the data FIFO
   eTime.Reset();
   for(u32 i=0; i<nSamples; i++) 
   { 
      volatile i16 sample = (i16) m_pNI6363->ReadReg16(NIX::REG_AI_FIFO_Data16);
   }
   double dTime = eTime.Get();
   
   Init();
   
   ::printf("   %d samples unloaded, avg time per sample = %.3f us\n", nSamples, dTime/nSamples);
}


//===================================================================================================================== 
// CNI6363_AO
//
// This class implements CCxAnalogOut and manages the analog output subsystem onboard the PCIe-6363. As a friend class 
// of CNI6363, it uses a pointer to the parent device object to access private methods of that object in order to read
// and write device registers, convert output voltage to the calibrated DAC code, etc.
//
// Even though the PCIe-6363 has 16-bit analog outputs, CNI6363_AO will for now only support 12-bit. This is mainly
// because of limitations in MaestroRTSS that restrict us to 12-bit on the input side (see CNI6363_AI comments). To
// convert properly from a 12-bit DAC code X to a calibrated 16-bit DAC code Y, CNI6363_AO takes the following steps:
//    1) X * 20.0 volts / 4096 = desired output voltage V.
//    2) Y = CNI6363::VoltsToDAC(V). This method uses onboard calibration data to convert voltage to the corresponding
//       16-bit DAC code.
//===================================================================================================================== 

/**
 Construct a CNI6363_AO "subdevice" object representing the analog output subsystem on the PCIe-6363. The subdevice must
 be opened via CDevice::Open() before it can be used.
 
 @param pNI6363 Pointer to the parent device object, through which this subdevice will access registers onboard the
 physical PCIe-6363 device.
*/
CNI6363_AO::CNI6363_AO(CNI6363* pNI6363) : 
   CCxAnalogOut(CNI6363::DEVINFO, 1, FALSE, CNI6363_AO::NUM_AO, CNI6363_AO::CHAIR_CHANNEL)
{
   m_pNI6363 = pNI6363;
}

/**
 [CDevice impl] Resets the AO subsystem and its output timing control circuitry, disables and acks all AO-related
 interrupts, and clears the AO data FIFO. It then programs all 4 AO channels for bipolar +/-10V range in immediate 
 update mode, and initializes all channels to zero volts.
 @return True if successful, false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363_AO::Init()
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   // reset AO subsystem and its output timer circuitry
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_CfgStart);
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_Reset);
   m_pNI6363->WriteReg32(NIX::REG_AOTimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_FIFOClear);
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_CfgEnd);
   
   // program all AO channels for bipolar +/-10V and immediate update mode. We won't use the output timer circuitry.
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_CfgStart);
   u32 addr = NIX::REG_AO_Config_Bank_0;
   for(u32 i=0; i<NUM_AO; i++)
   {
      m_pNI6363->WriteReg8(addr, NIX::AOCfg_DefaultConfig);
      ++addr;
   }
   m_pNI6363->WriteReg16(NIX::REG_AOTimer_Reset, NIX::OTReset_CfgEnd);
   
   // reset all outputs to calibrated 0V
   i16 zeroVolts = 0;
   addr = NIX::REG_AO_Direct_Data_0;
   for(u32 i=0; i<NUM_AO; i++)
   {
      zeroVolts = m_pNI6363->VoltsToDAC(i, (f32)0);
      m_pNI6363->WriteReg32(addr, zeroVolts);
      addr += 4;
   }
   
   return(TRUE);
}

/**
 [CCxAnalogOut impl] The implementation linearly converts DAC code to a voltage via ToVolts(), then invokes 
 Out(int, float). The real work is done there.
 @param ch AO channel# to be updated; if invalid, all channels are set to the same voltage. 
 @param b2sVolt The desired voltage, as a raw DAC code, a raw binary 2s-comp number.
 @return True if successful, false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363_AO::Out(int ch, int b2sVolt) { return( Out(ch, ToVolts(b2sVolt)) ); }

/**
 [CCxAnalogOut impl] The method uses the AO scaling function to convert the desired output voltage to the corresponding
 calibrated DAC code, then updates the specified channel. All AO channels on the device are configured in "immediate
 update" mode and bipolar +/-10V range when the AO subsystem is initialized, so it's just a matter of writing the DAC
 code to the appropriate register.
 @param ch AO channel# to be updated; if invalid, all channels are set to the same voltage. 
 @param fVolt The desired output value, in volts.
 @return True if successful, false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363_AO::Out(int ch, float fVolt)
{
   if(!IsOn()) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   u32 dacAddr = NIX::REG_AO_Direct_Data_0;
   if(ch >= 0 && ch < CNI6363_AO::NUM_AO)
   {
      dacAddr += 4*ch;
      m_pNI6363->WriteReg32(dacAddr, m_pNI6363->VoltsToDAC(ch, (f32)fVolt));
   }
   else for(u32 i=0; i<CNI6363_AO::NUM_AO; i++)
   {
      i16 dacCode = m_pNI6363->VoltsToDAC(i, (f32)fVolt);
      m_pNI6363->WriteReg32(dacAddr, dacCode);
      dacAddr += 4;
   }

   return(TRUE);
}

/**
 This function is intended to assess how much time it takes to perform a direct update of an analog output channel.
 For each output channel, it performs the following tests:
    1) Raw update: Measures the time it takes to write every possible 16-bit DAC code from [-32768..32767] to 
 the channels AO_Direct_Data_i register. Reports the average time per update on the console.
    2) Out() execution time: Measures the time it takes to write the floating-point voltages [-10..10] V in increments
 of 0.5V. Reports the average time per Out() call on the console.
 
 For maximum accuracy, be sure the calling thread has maximum real-time priority and take pains to ensure it won't be
 preempted!
*/
VOID RTFCNDCL CNI6363_AO::TestUpdatePerformance()
{
   ::printf("AO subsystem performance test:\n");
   if(!IsOn()) { ::printf("   FAILED. Device not available.\n"); return; }
   ::Sleep(10);

   CElapsedTime eTime;

   u32 dacAddr = NIX::REG_AO_Direct_Data_0;
   for(int ch = 0; ch < CNI6363_AO::NUM_AO; ch++)
   {
      eTime.Reset();
      for(int i=0; i<65536; i++) m_pNI6363->WriteReg32(dacAddr, (i16) (i-32768));
      double d1 = eTime.Get() / 65536.0;

      eTime.Reset();
      f32 fVolt = -10.0f;
      for(int i=0; i<400; i++) { Out(ch, fVolt); fVolt += 0.05f; }
      double d2 = eTime.Get() / 400.0;

      ::printf("    Ch 0: avg write time = %.3f us; avg Out() time = %.3f us\n", d1, d2);
      ::Sleep(10);

      dacAddr += 4;
   }
   Init();
}


//===================================================================================================================== 
// CNI6363_DIO
//
// This class implements CCxEventTimer and manages the DI, DO, and counter G0 subsystems onboard the PCIe-6363. As a
// friend class of CNI6363, it uses a pointer to the parent device object to access private methods of that object in 
// order to read and write device registers.
//
// How CCxEventTimer functionality is implemented on the PCIe-6363:
// 1) At startup, Port0(15..0) are configured as timed digital inputs and Port0(31-16) are configured as static 
// digital outputs. These comprise the event timer's 16 inputs and 16 outputs. PFI0 is configured as a static output and
// is used to drive the active-low "Data Ready" signal, which is pulsed low immediately after updating the event timer's
// outputs so they can be latched into Maestro's external DIO interface. This port configuration never changes.
// 2) Configure() enables change detection of both rising AND falling edges on the 16 digital inputs, and configures the
// DI subsystem for continuous acquistion using the change-detect signal as the "sample clock" signal that latches the
// current state of the 16 inputs into the 255-entry DI FIFO. In addition, it configures the G0 counter subsystem for a
// continuous buffered edge-counting operation: The counter source is the 100KHz (10us) internal timebase, and the
// change-detect signal is again used as the "sample clock" that latches the counter value into its 127-entry FIFO. The
// DI timing subsystem is configured for a software start, armed, but NOT started. The counter G0 hardware arm signal is
// enabled, and the DI_Start1 pulse is its source -- so this starts the DI subsystem and the counter simultaneously.
// 3) Start() simply strobes the DI_Start1 trigger.
// 4) Since the DI and counter FIFOs are so small, we MUST unload them regularly. MaestroRTSS unloads every 1 or 2ms,
// so that should be fine because the digital signals we timestamp should rarely exceed 1Khz and are typically much less
// frequent. Nevertheless, it will be important to check for FIFO overflows and stop on an error. In addition, there
// should be an entry in the counter FIFO for each entry in the DI FIFO; if that's not the case, then again stop.
// 5) NOTE that we enabled change-detect on both rising and falling edges. In order to timestamp only the rising-edge
// events, we need to know when an input returns to 0 after a 0->1 transition so that we don't "miss" the next 0->1
// transition!
// 6) Tricky problem: We need to know the state of the 16 inputs at the moment we start timestamping. But I don't think
// we can read them once they're configured as timed inputs -- so perhaps we can briefly configure them as static inputs
// and read their value, then return them to timed inputs just before we start timestamping...
// 7) Another problem: The NI MHDDK examples verify that the DI subsystem is armed, timing out after 5-10 seconds if it
// does not. The same is true for the G0 counter subsystem. We cannot wait nearly that long in Maestro. Configure() will
// fail if the DI subsystem does not arm within 300us. In addition, after strobing DI_Start1 in Start(), we wait only up
// to 300us to verify that the counter has armed. If it does not, we set an internal flag to indicate the problem and
// report the failure on the first UnloadEvents() call -- since Start() does not have an error return. Testing has thus
// far found no problems with this approach.
//===================================================================================================================== 

/**
 Construct a CNI6363_DIO "subdevice" object representing the implementation of the Maestro DIO event timer function on 
 the DI, DO and counter G0 subsystems of the PCIe-6363. The subdevice must be opened via CDevice::Open() before it can 
 be used.
 @param pNI6363 Pointer to the parent device object, through which this subdevice will access registers onboard the
 physical PCIe-6363 device.
*/
CNI6363_DIO::CNI6363_DIO(CNI6363* pNI6363) : 
      CCxEventTimer(CNI6363::DEVINFO, 1, CNI6363_DIO::NUM_DI, CNI6363_DIO::NUM_DO)
{
   m_pNI6363 = pNI6363;
   m_evtState = evtUNKNOWN;
   m_IsFirstInit = TRUE;      // we only have to configure the Port0 and PFI pins during the first Init() call!
   m_lastInputState = 0;

   m_soft_DI_Trigger_Select = 0;
   m_soft_DIT_Mode_1 = 0;
   m_soft_DIT_Mode_2 = 0;
}

/**
 [CDevice impl] Resets the DI subsystem and its input timing control circuitry, resets the DO subsystem and its timing
 control circuitry, and resets the counter G0 subsystem and its timing circutiry. Disables and acks all relevant
 interrupts just in case, even though we never use them. Clears the DI and counter G0 data FIFOs. If an event 
 timestamping operation was in progress, it is terminated (and any unread timestamps lost). 
 
 Finally, we configure the digital inputs and outputs that we'll need: Port0 bits 15-0 are configured as correlated 
 inputs for event timestamping purposes; Port0 bits 31-16 are configued as static digital outputs; and Port 1 bit 0 
 (PFI0) is configured for static output (the "Data Ready" signal used to latch digital outputs into an external device).
 Port 2 and the rest of Port 1 (PFI1-15) are all configured as static input; they are not used.
 
 @return True if successful, false otherwise (device error msg set).
*/
BOOL RTFCNDCL CNI6363_DIO::Init()
{
   if(!IsOn()) { m_evtState = evtERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   
   // if a previously running event timestamping operation is in progress, stop it.
   Stop();
   
   // reset all subsystems we use
   ResetAll();
   
   // configure all bidirectional digital lines: Port0 and PFI0..15. We only have to do this once, because we never use
   // any other DIO configuration.
   if(m_IsFirstInit)
   {
      m_IsFirstInit = FALSE;
      
      // configure Port0 bits15-0 as timed digital inputs for event-timestamping purposes, and Port0 bits 31-16 as 
      // static digital outputs. All outputs are zero initially. NOTE: Acc to NI MHDDK, we might get a glitch on the 
      // static digital value here, but I'm not worrying about it since this is done just once during app startup.
      m_dwDO = 0;
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
      m_pNI6363->WriteReg32(NIX::REG_DO_Mask_Enable, (u32) 0);
      m_pNI6363->WriteReg32(NIX::REG_DI_Mask_Enable, (u32) 0x0000FFFF);
      m_pNI6363->WriteReg32(NIX::REG_DO_DIODirection, (u32) 0xFFFF0000);
      
      // configure PFI0 as static output ("Data Ready" for latching digital output lines into external equipment) and
      // initialize it to 1 (latching happens on the falling edge). Configure all other PFIs as static inputs (unused).
      m_pNI6363->WriteReg16(NIX::REG_PFI_DO, (u16) 0x0001);
      m_pNI6363->WriteReg8(NIX::REG_PFI0_OutputSelect, NIX::REG_PFIOutSelect_PFI_DO);
      m_pNI6363->WriteReg16(NIX::REG_PFI_Direction, (u16) 0x0001);
   }

   m_iClockUS = 0;
   m_evtState = evtINITD;
   return(TRUE);
}

/**
 [CDevice override] Before closing our connection to the PCIe-6363, we try to leave the subsystems we used in a reset
 state and configure all bidirectional DIO pins (Port0, PFIs) as inputs.
*/
VOID RTFCNDCL CNI6363_DIO::OnClose()
{
   if(IsOn())
   {
      ResetAll();
      
      // put Port0 pins in a known state: all are statically updated inputs. Soft copy of output value reset to 0.
      m_dwDO = 0;
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, (u32) 0);
      m_pNI6363->WriteReg32(NIX::REG_DO_Mask_Enable, (u32) 0);
      m_pNI6363->WriteReg32(NIX::REG_DO_DIODirection, (u32) 0);
      m_pNI6363->WriteReg32(NIX::REG_DI_Mask_Enable, (u32) 0);
      
      // put PFI pins in known state: direction=input; outputValue=0 (ignored)
      m_pNI6363->WriteReg16(NIX::REG_PFI_DO, (u16) 0);
      m_pNI6363->WriteReg16(NIX::REG_PFI_Direction, (u16) 0);

      m_evtState = evtUNKNOWN;
   }
}

/**
 [CCxEventTimer impl] Configure device for DI event timestamping.
 @param clkPerUS Clock period for timestamping function, in microseconds
 @param enaVec Channel enable mask - Rising edges on DI channel N are timestamped iff bit N is set here.
 @return Actual clock period used, in us. Returns 0 to indicate operation failed.
*/
int RTFCNDCL CNI6363_DIO::Configure(int clkPerUS, DWORD enaVec)
{
   if(!IsOn()) { m_evtState = evtERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(0); }
   
   if(m_evtState != evtINITD && m_evtState != evtSTOPPED && !Init()) return(0);
   
   // We only support a 10us clock period. This is not a problem, as it's the only timestamp clock Maestro uses!
   if(clkPerUS != 10) {m_evtState = evtERROR; SetDeviceError("Only supports 10-us timestamp clock!"); return(0); }
   
   m_iClockUS = 0;
   
   // BEGIN: Configure DI timing engine
   // start configuration sequence -- suspend DI timing circuitry
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_CfgStart);
   
   // disable external gating of sample clock: b23..16 = 0 in DI_Trigger_Select register
   m_soft_DI_Trigger_Select = (m_soft_DI_Trigger_Select & 0xFF00FFFF);
   m_pNI6363->WriteReg32(NIX::REG_DI_Trigger_Select, m_soft_DI_Trigger_Select);
   
   // START1 trigger -- start on the rising edge of a "software pulse": b7..0 = 0x40 in the DI_Trigger_Select register. 
   // Note that we're doing a separate write from the previous step!
   m_soft_DI_Trigger_Select = (m_soft_DI_Trigger_Select & 0xFFFFFF00) | 0x0040;
   m_pNI6363->WriteReg32(NIX::REG_DI_Trigger_Select, m_soft_DI_Trigger_Select);
   
   // enable change detection on both rising and falling edges for Port0(15..0) ONLY
   m_pNI6363->WriteReg32(NIX::REG_DI_ChangeIrqRE, (u32) 0x0000FFFF);
   m_pNI6363->WriteReg32(NIX::REG_DI_ChangeIrqFE, (u32) 0x0000FFFF);
   m_pNI6363->WriteReg32(NIX::REG_DI_PFI_ChangeIrq, (u32) 0);
   
   // CONVERT trigger -- configure the "sample clock" to be driven by a rising edge on the "change-detect" signal:
   // b31..24 = 0x12 in the DI_Trigger_Select register.
   m_soft_DI_Trigger_Select = (m_soft_DI_Trigger_Select & 0x00FFFFFF) | 0x12000000;
   m_pNI6363->WriteReg32(NIX::REG_DI_Trigger_Select, m_soft_DI_Trigger_Select);
   
   // configure DI timing parameters (see MHDDK inTimerHelper.programTiming() for details):
   //   1) Ext_MUX_Present = Every_Convert, Start_Stop_Gate_Ena = Disabled, Trigger_Once = 1, Continuous = 1,
   // PreTrigger = post-trigger, SC_Initial_Load_Src = Load_A, SC_Reload_Mode = SC_Reload_No_Change: Flush these to 
   // DITimer's Mode_1_Register.
   //   2) Write scan count of -1 (don't care) into DITimer's SC_Load_A_Register, then load it via Command_Register.
   //   3) Start1_Export_Mode = ExportSyncStart1, Start2_Export_Mode = ExportMaskedStart2, Start_Trigger_Len =
   // ExportSyncStart, SyncMode = SyncDefault, HaltOnError = 1 : Flush these changes to DITimer's Mode_2_Register.
   //   4) NOTE: We use neither the SI nor the SI2 internal counters, so that's it for DI timing parameters!
   m_soft_DIT_Mode_1 = (m_soft_DIT_Mode_1 & 0xFFFC8FF9) | 0x00030000;
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Mode_1, m_soft_DIT_Mode_1);
   m_pNI6363->WriteReg32(NIX::REG_DITimer_SCLoadA, 0xFFFFFFFF);
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Command, NIX::ITCmd_SCLoad);
   m_soft_DIT_Mode_2 = (m_soft_DIT_Mode_2 & 0xBFC1FF7F) | 0x40020080;
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Mode_2, m_soft_DIT_Mode_2);

   // configure FIFO as 4-bytes wide and clear it. NOTE that we ignore the upper 16 bits.
   m_pNI6363->WriteReg32(NIX::REG_DI_Mode, NIX::DIMode_4ByteFIFO);
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_FIFOClear);

   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_CfgEnd);
   // END: Configure DI timing engine
   
   // BEGIN: Configure G0 counter to continuously count TB1 (100KHz) rising edges, with the change-detect signal as its
   // sample clock, which saves the current count in the counter FIFO
   //
   // reset counter G0
   m_pNI6363->WriteReg16(NIX::REG_G0_Command, NIX::GiCmd_Reset);
   m_pNI6363->WriteReg32(NIX::REG_G0_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg16(NIX::REG_G0_DMA_Config, NIX::GiDMACfg_Reset);

   // G0_Mode_Register: ReloadSrc_Switching=UseSameLoadReg, Loading_On_Gate=NoReloadOnGate, ForceSrcEqualToTB=False, 
   // LoadingOnTC=Rollover, Counting_once=NoHWDisarm, LoadSrc_Sel=LoadFromA, TrgModeForEdgeGate=GateLoads, 
   // GatingMode=GateDisabled, all other fields set to 0 (don't cares).
   m_pNI6363->WriteReg16(NIX::REG_G0_Mode, (u16) 0x0018);

   // G0_Mode2_Register: Up_Down=Up, Bank_Switch_Enable=DisabledIfArmedElseX, Bank_Switch_Mode=Gate, StopOnError=False,
   // all other fields set to 0 (don't cares).
   m_pNI6363->WriteReg16(NIX::REG_G0_Mode2, (u16) 0x4000);

   // G0_Counting_Mode_Register: Prescale=False, HWArm_Select=DI_Start1, HWArmEna=True, HWPolarity=ActiveHi, 
   // CountingMode=Normal, all other fields set to 0.
   m_pNI6363->WriteReg16(NIX::REG_G0_Counting_Mode, (u16) 0x1E80);

   // G0_SampleClock_Register: SampClkGateIndep=True, SampClkSampMode=LastSaved, SampClkMode=SingleSample, 
   // SampClkPolarity=RisingEdge, SampClkSelect=ChgDetect, all other fields set to 0
   m_pNI6363->WriteReg16(NIX::REG_G0_SampleClock, (u16) 0x9114);

   // G0_AuxCtr_Register: AuxCtrMode=Disabled. We don't use the embedded auxiliary counter associated with G0.
   m_pNI6363->WriteReg16(NIX::REG_G0_AuxCtr, (u16) 0);

   // Don't touch G0_Autoincrement_Register. We don't need it.

   // G0_Second_Gate_Register: SecondGateMode=Disabled. We don't use the second gate.
   m_pNI6363->WriteReg16(NIX::REG_G0_Second_Gate, (u16) 0);

   // G0_Input_Select_Register: SrcPolarity=RisingEdge, SrcSelect=TB2 (100KHz), all other fields set to 0.
   m_pNI6363->WriteReg16(NIX::REG_G0_Input_Select, (u16) 0x0024);

   // Don't touch G0_ABZ_Select_Register. We don't need it.

   // G0_DMA_Config_Register: DMA_Write=False and DMA_Enable=True to enable the counter FIFO. All other fields = 0.  We 
   // don't use DMA; instead, we read FIFO content via programmed IO thru G0_RdFifo_Register.
   m_pNI6363->WriteReg16(NIX::REG_G0_DMA_Config, (u16) 0x0001);
   //
   // END: Configure G0 counter...

   // load G0 counter with inital zero count, then arm it. It won't start until it gets the HW arm signal. Since this
   // was configured above to be DI_Start1, the counter should start at the same time as the DI subsystem when the
   // appropriate bit is strobed in Start() function!
   m_pNI6363->WriteReg32(NIX::REG_G0_Load_A, (u32) 0);
   m_pNI6363->WriteReg16(NIX::REG_G0_Command, NIX::GiCmd_Load);
   m_pNI6363->WriteReg16(NIX::REG_G0_Command, NIX::GiCmd_Arm);
   
   // arm the DI timing engine: As required in the MHDDK, we arm the SC and DIV counters (the SI and SI2 counters are 
   // not used, so they're not armed) in a single write to DITimer_Command register. The MHDDK verifies that the timing 
   // engine is armed by polling the DITimer_Status_1 register for up to 5 seconds. We CANNOT wait that long! We only
   // wait up to 300us and fail if the timing subsystem is not armed in time.
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Command, NIX::ITCmd_SCArm | NIX::ITCmd_DivArm);
   CElapsedTime eTime;
   BOOL armed = FALSE;
   while(!armed)
   {
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_DITimer_Status1);
      if((stat & NIX::ITStatus1_SC_Armed) != 0) armed = TRUE;
      else if(eTime.Get() > 300) break;
   }
   if(!armed)
   {
      Init();
      m_evtState = evtDIDNOTARM;
      SetDeviceError( "PCIe-6363 DI Timing Engine failed to arm!");
      return(0);
   }

   m_iClockUS = 10;
   m_evtState = evtREADY;
   ClearDeviceError();
   return(m_iClockUS);
}

/** 
 [CCxEventTimer impl] Start previously configured DI event timestamping operation NOW ("software start"). This method
 takes no action if the device is unavailable or if it is not in the "ready-to-start" state. Starting the operation 
 requires a single register write to stobe-start the DI timing engine. Since the DI_Start1 signal is the hardware arm
 trigger for the G0 counter, both subsystems are started together.
 
 NOTES:
 1) We ASSUME that all DI channels are lowered (0) when we start timestamping. If DI<n> happens to be raised (1) when
 timestamping starts and is still raised when the first real transition is detected on any of the other inputs, then
 the code will "think" that a rising-edge transition happened on DI<n>. Such a circumstance should be very rare in
 MaestroRTSS.
 2) In the NI MHDDK example that uses an external signal for the counter's hardware arm start trigger, the example waits
 up to 10 seconds for the counter to enter the armed state (Gi_Armed_St bit in the Gi_Status register). We CANNOT wait
 that long. We give it only 300us to arm. If it does not, then the event timestamping operation is cancelled and an
 internal flag is set so that the appropriate error message is reported. NOTE that this scenario does NOT fit will with
 the current design of the CCxEventTimer interface and how it is used by MaestroRTSS.
 */
VOID RTFCNDCL CNI6363_DIO::Start()
{
   if(IsOn() && m_evtState == evtREADY)
   {
      // software-start DI timing engine. ASSUME all digital inputs are initially 0 when we start.
      m_pNI6363->WriteReg32(NIX::REG_DITimer_Command, NIX::ITCmd_Start1_Pulse);
      m_evtState = evtRUNNING;
      m_lastInputState = 0;

      // give counter G0 only 300us to enter the armed state. If that doesn't happen, stop timestamping now and 
      // report the error (but note there's no error return value).
      CElapsedTime eTime;
      BOOL armed = FALSE;
      while(!armed)
      {
         u32 stat = m_pNI6363->ReadReg32(NIX::REG_G0_Status);
         if((stat & NIX::GiStat_Armed) != 0) armed = TRUE;
         else if(eTime.Get() > 300) break;
      }
      if(!armed)
      {
         Stop();
         m_evtState = evtDIDNOTARM;
         SetDeviceError( "PCIe-6363 timestamp counter failed to arm after DI strobe start!");
      }
   }
   else SetDeviceError("Device unavailable, or in an inconsistent state!");
}

/** [CCxEventTimer impl] Stop the DI event timestamping operation in progress (event store is NOT emptied). */
VOID RTFCNDCL CNI6363_DIO::Stop()
{
   if(IsOn() && m_evtState == evtRUNNING)
   {
      m_pNI6363->WriteReg32(NIX::REG_DITimer_Command, NIX::ITCmd_Disarm);
      m_pNI6363->WriteReg16(NIX::REG_G0_Command, NIX::GiCmd_Disarm);
      // NOTE: WE DO NOT VERIFY THAT COUNTER IS INDEED DISARMED, as is done in NI MHDDK examples.
      m_evtState = evtSTOPPED;
   }
   else SetDeviceError("Device unavailable, or in an inconsistent state!");
}

/**
 [CCxEventTimer impl] Unload DI event timestamping store in chronological order.
 @param nToRead Max #events to read (>= size of provided buffers).
 @param pEvents, pTimes [out] Buffers provided for storing event masks and corresponding event times. Event times are in
 timestamp clock ticks since timestamping operation started.
 @return #events actually read.
 */
DWORD RTFCNDCL CNI6363_DIO::UnloadEvents(DWORD nToRead, PDWORD pEvents, PDWORD pTimes)
{
   if(!IsOn()) { m_evtState = evtERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return((DWORD)0); }
   else if(m_evtState != evtRUNNING && m_evtState != evtSTOPPED)
   {
      SetDeviceError("Device has not been configured to timestamp DI events, or is in an invalid state!");
      return((DWORD) 0);
   }
   ClearDeviceError();

   DWORD nRead = 0;
   while(nRead < nToRead)
   {
      // get next DI event mask from DI FIFO. Abort on FIFO error. Return if FIFO empty.
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_DITimer_Status1);
      if((stat & NIX::ITStatus1_FIFO_Empty) != 0)
         return(nRead);
      else if((stat & (NIX::ITStatus1_Overrun | NIX::ITStatus1_Overflow)) != 0)
      {
         if(m_evtState == evtRUNNING) Stop();
         m_evtState = evtERROR;
         SetDeviceError("Scan overrun or DI FIFO overflow; timestamping aborted.");
         return((DWORD) 0);
      }
      u16 currIn = (u16) (0x0000FFFF & m_pNI6363->ReadReg32(NIX::REG_DI_FIFOData));
      
      // get corresponding timestamp from G0's FIFO. Abort if a timestamp is not ready -- G0 and DI out-of-sync.
      u32 count = m_pNI6363->ReadReg32(NIX::REG_G0_FifoStatus);
      if(count == 0)
      {
         // wait a maximum of one clock period (10us) for the timestamp to show up in the FIFO. This is because the
         // counter subsystem synchronizes its inputs to the source clock.
         CElapsedTime eTime;
         do 
         { 
            count = m_pNI6363->ReadReg32(NIX::REG_G0_FifoStatus);
         } while(count == 0 && eTime.Get() < 10.0);
         
         if(count == 0)
         {
            if(m_evtState == evtRUNNING) Stop();
            m_evtState = evtERROR;
            SetDeviceError("Missing timestamp for detected DI transition; timestamping aborted");
		      return((DWORD) 0);
         }
      }
      u32 tStamp = m_pNI6363->ReadReg32(NIX::REG_G0_RdFifoData);

      // we have to clock in both RE and FE transitions to keep track of DI state, but we only care about RE
      // transitions. If there are no RE transitions on this event, discard; else, push it into buffers.
      u16 eventMask = (~m_lastInputState) & currIn;
      if(eventMask != 0)
      {
         *(pEvents+nRead) = 0x0000FFFF & ((DWORD) eventMask);
         *(pTimes+nRead) = tStamp;
         ++nRead;
      }
      
      // update the saved state of our 16 inputs
      m_lastInputState = currIn;
   }
   
   return(nRead);
}

/**
 [CCxEventTimer impl] Unload DI event timestamping store in chronological order, but with timestamps in seconds
 @param nToRead Max #events to read (>= size of provided buffers).
 @param pEvents, pTimes [out] Buffers provided for storing event masks and corresponding event times. Event times are in
 seconds since timestamping operation started.
 @return #events actually read.
 */
DWORD RTFCNDCL CNI6363_DIO::UnloadEvents(DWORD nToRead, PDWORD pEvents, float *pfTimes)
{
   if(!IsOn()) { m_evtState = evtERROR; SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return((DWORD)0); }
   else if(m_evtState != evtRUNNING && m_evtState != evtSTOPPED)
   {
      SetDeviceError("Device has not been configured to timestamp DI events, or is in an invalid state!");
      return((DWORD) 0);
   }
   ClearDeviceError();

   DWORD nRead = 0;
   while(nRead < nToRead)
   {
      // get next DI event mask from DI FIFO. Abort on FIFO error. Return if FIFO empty.
      u32 stat = m_pNI6363->ReadReg32(NIX::REG_DITimer_Status1);
      if((stat & NIX::ITStatus1_FIFO_Empty) != 0)
         return(nRead);
      else if((stat & (NIX::ITStatus1_Overrun | NIX::ITStatus1_Overflow)) != 0)
      {
         if(m_evtState == evtRUNNING) Stop();
         m_evtState = evtERROR;
         SetDeviceError("Scan overrun or DI FIFO overflow; timestamping aborted.");
         return((DWORD) 0);
      }
      u16 currIn = (u16) (0x0000FFFF & m_pNI6363->ReadReg32(NIX::REG_DI_FIFOData));

      // get corresponding timestamp from G0's FIFO. Abort if a timestamp is not ready -- G0 and DI out-of-sync.
      u32 count = m_pNI6363->ReadReg32(NIX::REG_G0_FifoStatus);
      if(count == 0)
      {
         // wait a maximum of one clock period (10us) for the timestamp to show up in the FIFO. This is because the
         // counter subsystem synchronizes its inputs to the source clock.
         CElapsedTime eTime;
         do 
         { 
            count = m_pNI6363->ReadReg32(NIX::REG_G0_FifoStatus);
         } while(count == 0 && eTime.Get() < 10.0);
         
         if(count == 0)
         {
            if(m_evtState == evtRUNNING) Stop();
            m_evtState = evtERROR;
            SetDeviceError("Missing timestamp for detected DI transition; timestamping aborted");
		      return((DWORD) 0);
         }
      }
      u32 tStamp = m_pNI6363->ReadReg32(NIX::REG_G0_RdFifoData);

      // we have to clock in both RE and FE transitions to keep track of DI state, but we only care about RE
      // transitions. If there are no RE transitions on this event, discard; else, push it into buffers.
      u16 eventMask = (~m_lastInputState) & currIn;
      if(eventMask != 0)
      {
         *(pEvents+nRead) = 0x0000FFFF & ((DWORD) eventMask);
         *(pfTimes+nRead) = 0.00001f * ((float) tStamp);       // only 10us clock period supported
         ++nRead;
      }
      
      // update the saved state of our 16 inputs
      m_lastInputState = currIn;
   }
   
   return(nRead);
}

/** 
 [CCxEventTimer impl] Immediately update the event timer's digital output port.

 IMPORTANT: We use PFI0 as the active-low "Data Ready" signal. It was generated by the hardware on the original LisTech
 timing board. In testing we found that if we simply lowered and raised PFI0 with successive register writes, the 
 effective pulse width was 0.2us on a Win7/3.3GHz quad-core PC. This was NOT sufficient for external latched devices in
 the DIO interface to detect "Data Ready" and latch the data lines. We therefore added a delay to lengthen the pulse on 
 PFI0. With a 1-us pulse we observed occasional latching failures; we saw no failures with 2-us and 5-us pulses. We
 elected to set the pulse length to 2.5-us. Therefore, this function will ALWAYS take a minimum of 2.5us to complete.

 05sep2019: Revised implementation so there is a short "busy wait" between each of the three register writes in this
 function. The busy wait times are maintained in a protected array in CCxEventTimer. This provides a mechanism by which 
 the individual busy wait times can be adjusted. [Maestro reads the wait times from the Windows registry at startup and
 communicates them to CXDRIVER via IPC; CXDRIVER, in turn, calls CCxEventTimer::SetDOBusyWaitTimes() to change the busy
 wait durations.]
    This change was the result of several weeks of testing and experimentation with Mati Joshua. His DIO interface was 
 missing the DO11 "RECORDMARKER" marker pulse at the start of the trial, as well as any T=0 marker pulse. It did NOT 
 have this problem in Win7/Maestro 3.x. It may have something to do with PCIe bus conflicts in Win10. I don't really 
 know for sure -- only that it helped to put delays after each register write.

 @param dwVec The new DO port value -- each bit in mask indicates new state of corresponding DO channel.
 @return Previous state of DO port.
*/
DWORD RTFCNDCL CNI6363_DIO::SetDO(DWORD dwVec)
{
   DWORD dwOld = (DWORD) m_dwDO;
   if(IsOn()) 
   {
      // REM: We use Port0 bits 31..16 as our 16 supported DO channels!
      u32 out = (dwVec << 16) & 0xFFFF0000;
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, out);
      m_dwDO = 0x0000FFFF & dwVec;
      
      // busy wait after each register write -- SEE FUNCTION HEADER
      CElapsedTime eTime;
      volatile long count = 0;
      while(eTime.Get() < m_fDOBusyWaits[0]) ++count;

      // lower, then raise PFI0: the "DataReady" latching signal
      volatile u16 dataReady = (u16) 0;
      m_pNI6363->WriteReg16(NIX::REG_PFI_DO, dataReady);

      eTime.Reset();
      while(eTime.Get() < m_fDOBusyWaits[1]) ++count;

      dataReady = (u16) 1;
      m_pNI6363->WriteReg16(NIX::REG_PFI_DO, dataReady);

      eTime.Reset();
      while(eTime.Get() < m_fDOBusyWaits[2]) ++count;
   }
   else
   {
      m_evtState = evtERROR;
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL);
   }
   
   return(dwOld);
}

/**
 This function tests the ability of the DI event timestamping scheme to detect pulses that are shorter than the 10us 
 event clock. It requires that DO0 be looped back into DI0. The event timestamping function is configured and 
 started. Then DO0 is toggled up and down twice, generating two rising-edge events. This is done by direct writes to
 the Static_DO register -- so this is as fast as we can change the output programmatically. Any detected events are then
 unloaded. The same two-pulse test is repeated 10000 times. Stats reported to the console: #failures (ie, # times
 that two distinct events were not detected), and avg #ticks between the two events (likely to be 0).

  
 For maximum accuracy, be sure the calling thread has maximum real-time priority and take pains to ensure it won't be
 preempted!
*/
VOID CNI6363_DIO::TestShortPulseTimestampPerformance()
{
   ::printf("NI-6363 DIO Event timestamp short-pulse performance:\n");
   if(!Configure(10, 0x0FFFF))
   {
      ::printf("   FAILED: %s\n", GetLastDeviceError());
      return;
   }
   Start();

   CElapsedTime eTime;
   DWORD dwEvents[2];
   DWORD dwTicks[2];

   int nFailures = 0;
   double accumTicks = 0.0;
   double accumDur = 0;
   u32 up = 0x00010000;
   u32 dn = 0x00000000;

   for(int i=0; i<10000; i++)
   {
      // deliver two pulses on DO0 as fast as we can programmatically
      eTime.Reset();
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, up);
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, dn);
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, up);
      m_pNI6363->WriteReg32(NIX::REG_DO_Static_DO, dn);
      accumDur += eTime.Get();

      DWORD nRead = UnloadEvents(2, &(dwEvents[0]), &(dwTicks[0]));
      if(nRead != 2 || dwEvents[0] != 0x0001 || dwEvents[1] != 0x0001)
         ++nFailures;
      else
         accumTicks += (dwTicks[1] - dwTicks[0]);
   }

   ::printf("   Avg duration of two-pulse test = %.3f us.\n", accumDur/10000.0);
   ::printf("   %d failures (missed one or both pulses) out of 10000 reps.\n", nFailures);
   if(nFailures < 1000) ::printf("   avg # ticks between event pairs = %.3f\n", accumTicks/((double) 10000-nFailures));
}

/**
 Helper method for Init() and OnClose(). Performs the sequence of device register writes necessary to reset the 
 subsystems used by CNI6363_DIO and disable/ack relevant interrupts (as a precaution, even though we never enable such
 interrupts).
*/
VOID RTFCNDCL CNI6363_DIO::ResetAll()
{
   // reset DO subsystem and its output timer circuitry. Note that we never use DOTimer engine.
   m_pNI6363->WriteReg16(NIX::REG_DOTimer_Reset, NIX::OTReset_CfgStart);
   m_pNI6363->WriteReg16(NIX::REG_DOTimer_Reset, NIX::OTReset_Reset);
   m_pNI6363->WriteReg32(NIX::REG_DOTimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg16(NIX::REG_DOTimer_Reset, NIX::OTReset_FIFOClear);
   m_pNI6363->WriteReg16(NIX::REG_DOTimer_Reset, NIX::OTReset_CfgEnd);
   
   // reset DI subsystem and its input timer circuitry
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_CfgStart);
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_Reset);

   // after reset, ensure that exported convert polarity in DITimer's Mode_1 register is initialized to active-high (0).
   // All other Mode_1 bit fields are zeroed by the reset. Also, ensure DI FIFO data width = 4 bytes initially.
   m_soft_DIT_Mode_1 = 0;
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Mode_1, m_soft_DIT_Mode_1);
   m_pNI6363->WriteReg32(NIX::REG_DI_Mode, NIX::DIMode_4ByteFIFO);
   
   // After the s/w reset, various DI and DITimer register values will be modified by strobing the Reset bit.
   // Where needed, we keep s/w copies of the registers we'll need, and here we reset the s/w copies to 0. 
   m_soft_DI_Trigger_Select = 0;
   m_soft_DIT_Mode_2 = 0;
   
   m_pNI6363->WriteReg32(NIX::REG_DITimer_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_FIFOClear);
   m_pNI6363->WriteReg16(NIX::REG_DITimer_Reset, NIX::ITReset_CfgEnd);

   // reset counter G0 subsystem
   m_pNI6363->WriteReg16(NIX::REG_G0_Command, NIX::GiCmd_Reset);
   m_pNI6363->WriteReg32(NIX::REG_G0_Interrupt_2, NIX::Int2_DisableAndAckAll_Cmd);
   m_pNI6363->WriteReg16(NIX::REG_G0_DMA_Config, NIX::GiDMACfg_Reset);
}
