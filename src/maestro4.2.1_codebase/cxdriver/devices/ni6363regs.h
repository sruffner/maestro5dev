/**=====================================================================================================================
 ni6363regs.h : Register address offsets, bit field masks and other constants for the NI PCIe-6363 MIO board. 
======================================================================================================================*/

#if !defined(NI6363REGS_H__INCLUDED_)
#define NI6363REGS_H__INCLUDED_

#include "ni6363types.h"      // for u8, u16, u32, etc.

namespace NIX
{
// #bytes in the memory-mapped register address space (BAR0)
static const u32 REGADDRSPACESIZE = 0x40000;

/** CHInCh ===========================================================================================================*/
// CHInCh_Indentification_Register (R32) and expected content (for sanity check)
static const u32 REG_CHInCh_ID = 0x00000;
static const u32 CHInCh_Signature = 0xC0107AD0;
// PCI_Subsystem_ID_Access_Register (R32) and expected content for NI PCIe-6363 (for sanity check)
static const u32 REG_PCISubsystem_ID = 0x010AC;
static const u32 NI6363_SSID = 0x74351093;
// Scrap_Register (RW32) (for sanity check)
static const u32 REG_Scrap = 0x00200;

// start of EEPROM as an offset into device register space
static const u32 EEPROM_Start = 0x05000;
// addresses (R32) storing EEPROM pointers to capabilities list flag and the two possible capabilities lists. These are
// specified as offset from the start of EEPROM!!!!
static const u32 EEPROM_CapListFlagPtr = 0x000C;
static const u32 EEPROM_CapListAPtr = 0x0010;
static const u32 EEPROM_CapListBPtr = 0x0014;
// offsets and other constants for the "device-specific node" in EEPROM. Calibration data is stored in this node. 
// Offsets are specified WRT the start of the node in EEPROM memory.
static const u32 EEPROM_DevSpecificNode_ID = 0x0001;
static const u32 EEPROM_DSN_BodySizeOffset = 0x4;
static const u32 EEPROM_DSN_BodyFormatOffset = 0x8;
static const u32 EEPROM_DSNBF_16BitValueID = 0x1;
static const u32 EEPROM_DSNBF_32BitValueID = 0x2;
static const u32 EEPROM_DSNBF_16BitIDValue = 0x3;
static const u32 EEPROM_DSNBF_32BitIDValue = 0x4;
static const u32 EEPROM_DSN_BodyOffset = 0xC;
static const u32 EEPROM_SelfCalAPtrID = 0x42;
static const u32 EEPROM_SelfCalBPtrID = 0x43;
// these offsets are relative to the start of a calibration section
static const u32 EEPROM_CalDataOffset = 0x2;
static const u32 EEPROM_CalCoeffOffset = 0x12;
// constants needed to parse the calibration section data
static const u32 EEPROM_Cal_NumModeCoeffs = 4;
static const u32 EEPROM_Cal_NumAIModes = 4;
static const u32 EEPROM_Cal_NumAIIntervals = 7;
static const u32 EEPROM_Cal_NumAOIntervals = 4;
static const u32 EEPROM_Cal_ModeSizeInBytes = sizeof(u8) + 4*sizeof(f32);
static const u32 EEPROM_Cal_IntervalSizeInBytes = 2*sizeof(f32);

// Interrupt_Mask_Register (RW32) and relevant bits for disabling/enabling interrupts
static const u32 REG_Interrupt_Mask = 0x0005C;
static const u32 IMR_Set_CPU_Int = (1<<31);
static const u32 IMR_Clear_CPU_Int = (1<<30);
static const u32 IMR_Set_STC3_Int = (1<<11);
static const u32 IMR_Clear_STC3_Int = (1<<10);

/** DMA Channel Controllers ============================================================================================
 We don't use DMA on the PCIe-6363. We need these registers only to ensure that all DMA channels are stopped and all
 DMA interrupts disabled when we initialize the hardware.
*/
// Channel_Control_Register (RW32) -- address of the CHCR for the first DMA channel (AI DMA)
static const u32 REG_DMACh1_Control = 0x02054;
// Channel_Operation_Register (RW32) -- address of the CHOR for the first DMA channel (AI DMA)
static const u32 REG_DMACh1_Operation = 0x02058;
// address space offset separating the DMA controller register groups for each of 8 DMA channels
static const u32 DMACh_RegOffset = 0x00100;
static const u32 NumDMAChannels = 8;
// write this bit mask to the CHCR to clear and ack all interrupts for the DMA channel and set it in "Normal Mode"
static const u32 DMAChCR_DisableInts_Cmd = 0x00000000;
// write this bit mask to the CHOR to ensure the DMA channel is stopped
static const u32 DMAChOR_Stop_Cmd = 0x00000002;

/** Board Services and Bus Interface =================================================================================*/
// Global Signature_Register (R32) holds STC_revision (two possible values -- for sanity check)
static const u32 REG_Signature = 0x20060;
static const u32 STC_RevA = 0x8050509;
static const u32 STC_RevB = 0x8050501;
// ScratchPadRegister (RW32) (for sanity check)
static const u32 REG_ScratchPad = 0x20004;
// Global Joint_Reset_Register (W16): Strobe bit0 to perform a software reset of DAQ-STC3 endpoint
static const u32 REG_Joint_Reset = 0x20064;
// GlobalInterruptEnable_Register (W32) -- to enable/disable ALL interrupts from any subsystem (AI, AO, etc)
static const u32 REG_GlobalInterruptEnable = 0x020078;
// write this bit mask to the GlobalInterruptEnable_Register block all subsystem interrupts from propagating to CHInCh.
static const u32 GIER_DisableAll_Cmd = 0x04FF0000;
// write this bit mask to the GIER to allow AI subsystem interrupts (only) to propagate to the CHInCh
static const u32 GIER_EnableAI_Cmd = 0x00000001;
// Generic Group Interrupt_2_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_Gen_Interrupt_2 = 0x20074;
// for ANY subsystem's Interrupt_2_Register, we can simply strobe all 32 bits to disable and ack all defined interrupts
// for that subsystem (ie, it's OK to write 1's to reserved bits 
static const u32 Int2_DisableAndAckAll_Cmd = 0xFFFFFFFF;
// AI_Interrupt_Status_Register (R16): To detect a pending AI "start-of-scan" interrupt
static const u32 REG_AI_Interrupt_Status = 0x20072;
static const u16 AIIntStatus_StartIRQ = (u16) (1<<12);

/** AO Subsystem and AOTimer =========================================================================================*/
// AOTimer Interrupt_2_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_AOTimer_Interrupt_2 = 0x2049C;
// AOTimer Reset_Register (W16 -- all defined bits are strobe bits)
static const u32 REG_AOTimer_Reset = 0x204A4;
// these bit masks apply to the OutTimer Reset_Register for both AO and DO subsystems
static const u32 OTReset_FIFOClear = 0x0008;
static const u32 OTReset_CfgEnd = 0x0004;
static const u32 OTReset_CfgStart = 0x0002;
static const u32 OTReset_Reset = 0x0001;
// AO_Config_Bank Registers (W8): One register for each of 4 DACs on the PCIe-6363, in a contiguous array of 4x1 bytes.
static const u32 REG_AO_Config_Bank_0 = 0x2044C;
// write this to each Config_Bank to configure AO channel for bipolar +/-10V range in immediate update mode. This is
// the only way in which we use the AO channels in Maestro.
static const u32 AOCfg_DefaultConfig = 0x0080;
// AO_Direct_Data Registers (W32): One register per DAC, in a contiguous array of 4*4 bytes. A write to this register
// immediately updates the output on the corresponding DAC. Use the 16 LSBs.
static const u32 REG_AO_Direct_Data_0 = 0x20400;

/** AI Subsystem and AITimer =========================================================================================*/
// AI_Trigger_Select_Register, AI_Trigger_Select2_Register, AI_Data_Mode_Register (W32)
static const u32 REG_AI_Trigger_Select = 0x2029C;
static const u32 REG_AI_Trigger_Select2 = 0x202A0;
static const u32 REG_AI_Data_Mode = 0x20298;
// AI_Config_FIFO_Data_Register (W16)
static const u32 REG_AI_Cfg_FIFO_Data = 0x2028E;
static const u16 AICfg_LastCh = (u16) (1<<14);
// this bit mask enables dithering, selects NRSE termination, selects the +/-10V input range (h/w gain=1), and chooses
// bank0. We always configure our sampled channels this way. The channel# goes in bits 3..0.
static const u16 AICfg_Standard = 0x2280;
// this bit mask is the same as above, except that the AI_Config_Channel_Type bitfield is set to 111. This is an
// UNDOCUMENTED (in the NI MHDDK) value that behaves just like the "ghost channel" feature in E-Series devices. We use
// it to implement multirate sampling when the 25KHz "fast channel" is enabled.
static const u16 AICfg_Ghost = 0x23C0;
// AI_Data_FIFO_Status_Register (R32) and AI_FIFO_Data_Register16 (R16)
static const u32 REG_AI_Data_FIFO_Status = 0x20274;
static const u32 REG_AI_FIFO_Data16 = 0x20278;

// AITimer Interrupt_1 and Interrupt_2 Registers (W32 -- all defined bits are strobe bits)
static const u32 REG_AITimer_Interrupt_1 = 0x202DC;
static const u32 AITInt1_StartAck = (u32) (1<<27);
static const u32 AITInt1_StartEnable = (u32) (1<<3);
static const u32 REG_AITimer_Interrupt_2 = 0x202E0;
static const u32 AITInt2_StartAckAndDisable = AITInt1_StartAck | AITInt1_StartEnable;

// AITimer Reset_Register (W16 -- all defined bits are strobe bits)
static const u32 REG_AITimer_Reset = 0x202E8;
// these bit masks apply to the InTimer Reset_Register for both AI and DI subsystems
static const u32 ITReset_FIFOClear = 0x0010;
static const u32 ITReset_CfgMemoryClear = 0x0008;
static const u32 ITReset_CfgEnd = 0x0004;
static const u32 ITReset_CfgStart = 0x0002;
static const u32 ITReset_Reset = 0x0001;
// AITImer Mode_1 and Mode_2 Registers (W32)
static const u32 REG_AITimer_Mode_1 = 0x202B4;
static const u32 REG_AITimer_Mode_2 = 0x202B8;
// InTimer Mode_1_Register bit controlling exported convert polarity (1=activeLo, 0=activeHi)
static const u32 IT_Mode_1_ExpCP = (u32) (1<<0);
// AITimer SC_Load_A_Register (W32) - For continuous mode, this is always loaded w/ 0xFFFFFFFF (-1).
static const u32 REG_AITimer_SCLoadA = 0x202C4;
// AITimer SI_Load_A, SI_Load_B Registers(W32) - Loaded with (scan delay - 1) and (scan period - 1), resp.
static const u32 REG_AITimer_SILoadA = 0x202BC;
static const u32 REG_AITimer_SILoadB = 0x202C0;
// AITimer SI2_Load_A, SI2_Load_B Registers(W32) - Loaded with (convert delay - 1) and (convert period - 1), resp.
static const u32 REG_AITimer_SI2LoadA = 0x202CC;
static const u32 REG_AITimer_SI2LoadB = 0x202D0;
// AITimer Command_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_AITimer_Command = 0x202B0;
static const u32 ITCmd_SCLoad = (u32) (1<<5);
static const u32 ITCmd_SILoad = (u32) (1<<9);
static const u32 ITCmd_SI2Load = (u32) (1<<11);
static const u32 ITCmd_LocalClkMUXPulse = (u32) (1<<2);
static const u32 ITCmd_DivArm = (u32) (1<<8);
static const u32 ITCmd_SCArm = (u32) (1<<6);
static const u32 ITCmd_ArmAll = 0x00001540;
static const u32 ITCmd_Disarm = (u32) (1<<13);
static const u32 ITCmd_Start1_Pulse = (u32) (1<<16);

// AITimer Status_1_Register (R32)
static const u32 REG_AITimer_Status1 = 0x202B0;
static const u32 ITStatus1_SC_Armed = (u32) (1<<16);
static const u32 ITStatus1_FIFO_Empty = (u32) (1<<12);
static const u32 ITStatus1_ScanOverrun = (u32) (1<<15);
static const u32 ITStatus1_Overrun = (u32) (1<<11);
static const u32 ITStatus1_Overflow = (u32) (1<<10);
static const u32 ITStatus1_SC_TC_Error = (u32) (1<<9);
static const u32 ITStatus1_AnyError = ITStatus1_ScanOverrun|ITStatus1_Overrun|ITStatus1_Overflow|ITStatus1_SC_TC_Error;

/** DO Subsystem and DOTimer =========================================================================================*/
// DOTimer Interrupt_2_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_DOTimer_Interrupt_2 = 0x2049C;
// DOTimer Reset_Register (W16 -- all defined bits are strobe bits)
static const u32 REG_DOTimer_Reset = 0x20514;

// Static_Digital_Ouput_Register (W32)
static const u32 REG_DO_Static_DO = 0x204B0;
// DO_Mask_Enable_Register (W32): BitN selects what drives pinN on Port0 (if output pin): static DO(0) or CDOEngine(1).
static const u32 REG_DO_Mask_Enable = 0x204BC;
// DIO_Direction_Register (W32): BitN determines direction of pinN on Port0: 0=input, 1=output.
static const u32 REG_DO_DIODirection = 0x204B4;

/** DI Subsystem and DITimer =========================================================================================*/
// DITimer Interrupt_2_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_DITimer_Interrupt_2 = 0x20590;
// DITimer Reset_Register (W16 -- all defined bits are strobe bits)
static const u32 REG_DITimer_Reset = 0x20598;
// DITImer Mode_1 and Mode_2 Registers (W32)
static const u32 REG_DITimer_Mode_1 = 0x20564;
static const u32 REG_DITimer_Mode_2 = 0x20568;
// DITimer SC_Load_A_Register (W32) - For continuous mode, this is always loaded w/ 0xFFFFFFFF (-1).
static const u32 REG_DITimer_SCLoadA = 0x20574;
// DITimer Command_Register (W32 -- all defined bits are strobe bits) 
static const u32 REG_DITimer_Command = 0x20560;
// DITimer Status_1_Register (R32)
static const u32 REG_DITimer_Status1 = 0x20560;

// DI_Mode_Register (W32)
static const u32 REG_DI_Mode = 0x20534;
// we always configure the DI FIFO as 4-bytes wide, but we only pay attention to the lower 16 bits!
static const u32 DIMode_4ByteFIFO = 0x00008000;
// DI_Trigger_Select_Register (W32)
static const u32 REG_DI_Trigger_Select = 0x2053C;
// DI_Mask_Enable_Register (W32): Keep Port0(N) in DI FIFO (bitN=1) or force it to zero in FIFO (bitN=0).
static const u32 REG_DI_Mask_Enable = 0x20538;
// DI_FIFOData_Register (R32): To read next FIFO entry when DI FIFO is 4-bytes wide.
static const u32 REG_DI_FIFOData = 0x20538;
// DI_ChangeIrqRE, DI_ChangeIrqFE, and PFI_ChangeIrq Registers (W32)
static const u32 REG_DI_ChangeIrqRE = 0x20540;
static const u32 REG_DI_ChangeIrqFE = 0x20544;
static const u32 REG_DI_PFI_ChangeIrq = 0x20548;

/** Counter G0 Subsystem =============================================================================================*/
// Command_Register (W16 -- all defined bits are strobe bits)
static const u32 REG_G0_Command = 0x20300;
static const u16 GiCmd_Reset = (u16) (1<<14);
static const u16 GiCmd_Load = (u16) (1<<2);
static const u16 GiCmd_Disarm = (u16) (1<<4);
static const u16 GiCmd_Arm = (u16) (1<<0);
// DMA_Config_Register (W16)
static const u32 REG_G0_DMA_Config = 0x20314;
static const u16 GiDMACfg_Reset = (u16) (1<<3);   // strobe bit
// various other G0 configuration and load registers (W16)
static const u32 REG_G0_Mode = 0x20302;
static const u32 REG_G0_Mode2 = 0x2033E;
static const u32 REG_G0_Counting_Mode = 0x20310;
static const u32 REG_G0_SampleClock = 0x2031C;
static const u32 REG_G0_AuxCtr = 0x2031E;
static const u32 REG_G0_Second_Gate = 0x20312;
static const u32 REG_G0_Input_Select = 0x2030C;
// Load_A_Register (W32)
static const u32 REG_G0_Load_A = 0x20304;
// Status_Register (R32)
static const u32 REG_G0_Status = 0x2030C;
static const u32 GiStat_Armed = (u32) (1<<8);
static const u32 GiStat_TC = (u32)(1 << 24);
// FifoStatus_Register, RdFifoData_Register, Save_Register (R32)
static const u32 REG_G0_FifoStatus = 0x20310;
static const u32 REG_G0_RdFifoData = 0x20318;
static const u32 REG_G0_Save = 0x20308;

// Counter G0 Interrupt_2_Register (W32 -- all defined bits are strobe bits)
static const u32 REG_G0_Interrupt_2 = 0x20330;
// use this address space offset to get to the corresponding register for the next counter Gi
static const u32 Gi_RegOffset = 0x0040;
static const u32 NumCounters = 4;

/** PFI-related registers ============================================================================================*/
// PFI_DO_Register (W16) : Sets outputs of PFI0..15 when they're configured as digital outputs.
static const u32 REG_PFI_DO = 0x200E0;
// PFI_Direction_Register (W16) : Sets the direction (input=0, output=1) for bidirectional pins PFI0..15.
static const u32 REG_PFI_Direction = 0x200A4;
// PFI_OutputSelect Registers (W8) : Contiguous array of 16 1-byte registers selects the signal driven on PFI0..15 when
// they are configured for output. We always set its value to PFI_DO (general purpose DO) for all 16 pins.
static const u32 REG_PFI0_OutputSelect = 0x200BA;
static const u8 REG_PFIOutSelect_PFI_DO = 16;
static const u32 NumPFIs = 16;
}

#endif   // !defined(NI6363REGS_H__INCLUDED_)

