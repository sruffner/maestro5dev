//===================================================================================================================== 
//
// ni6363.h : Declaration of classes supporting the NI PCIe-6363 multi-function IO board. 
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(NI6363_H__INCLUDED_)
#define NI6363_H__INCLUDED_

#include "cxanalogin.h"             // abstract CCxAnalogIn - analog input device function
#include "cxanalogout.h"            // abstract CCxAnalogOut - analog output device function
#include "cxeventtimer.h"           // abstract CCxEventTimer - DI timestamping and DO device function

#include "ni6363types.h"            // storage-size-explicit types: u8, u16, etc.

class CNI6363;					    // forward declaration

/** CNI6363_AI: Implementation of CCxAnalogIn on the AI subsystem of the PCIe-6363. */
class CNI6363_AI : public CCxAnalogIn
{
private:
   // some constants: #AI channels supported (there are actually 32, but we don't expose them), AI FIFO size, and
   // the maximum #samples stored in FIFO per scan interval.
   static const int NUM_AI = 16;
   static const int AIFIFOSZ = 4095;
   static const int MAXPERSCAN = 100;
   
   // access device registers through the read/write operations provided by this "parent" device
   CNI6363* m_pNI6363;
   
   // current state of AI subsystem
   typedef enum
   {
      aiUNKNOWN = 0,
      aiERROR,
      aiINITD,
      aiREADY,
      aiRUNNING,
      aiSTOPPED
   } AIState;

   AIState m_aiState;

   // slow scan interval (in microsecs), #channels in slow-scanned set, and whether or not fast channel is sampled.
   // Values when last configured. Used to extract relevant samples from FIFO in Unload(), and to provide a timeout.
   int m_nScanIntvUS; 
   int m_nScanChannels; 
   BOOL m_IsFastChEna;
   
   // # of samples stored in FIFO per scan, pos of next sample slot unloaded within the scan set, and the nature 
   // (0=slow set sample, nonzero=25KHz channel sample) of each sample slot. These are used only to separate the slow 
   // set and 25KHz samples when the 25KHz channel is enabled.
   int m_nSlots;
   int m_iNextSlot;
   u8 m_slots[MAXPERSCAN];

   // soft copies of selected registers
   u32 m_soft_AI_Trigger_Select;
   u32 m_soft_AI_Trigger_Select2;
   u32 m_soft_AIT_Mode_1;
   u32 m_soft_AIT_Mode_2;
   
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6363_AI(const CNI6363_AI& src); 
   CNI6363_AI& operator=(const CNI6363_AI& src); 

public: 
   // constructor/destructor
   CNI6363_AI(CNI6363* pNI6363); 
   ~CNI6363_AI() { m_pNI6363 = NULL; }

   // [CDevice impl/override] 
   virtual LPCTSTR RTFCNDCL GetDeviceName() { return("PCIe-6363, AI Subsystem"); }
   BOOL RTFCNDCL Init(); 
protected:
   // NOTE: The subdevice object does not handle device resource mapping; that's handled by the parent device!
   BOOL RTFCNDCL MapDeviceResources() { return(TRUE); }
   VOID RTFCNDCL UnmapDeviceResources() {}

public: 
   // [CCxAnalogIn impl/override]
   int RTFCNDCL GetFIFOSize()  { return( IsOn() ? AIFIFOSZ : 0 ); }
   BOOL RTFCNDCL Configure(int nCh, int nScanIntv, int iChFast, BOOL bInt);
   VOID RTFCNDCL Start(); 
   VOID RTFCNDCL Stop();
   BOOL RTFCNDCL Unload(short *pSlow, int& nSlow, short *pFast, int& nFast, BOOL bWait);
   BOOL RTFCNDCL IsEmpty();
   BOOL RTFCNDCL IntAck();

   // for testing only
   VOID RTFCNDCL TestReadFIFOPerformance();
};


/** CNI6363_AO: Implementation of CCxAnalogOut on the AO subsystem of the PCIe-6363. */
class CNI6363_AO : public CCxAnalogOut
{
private:
   // some constants: #AO channels supported, AO channel# dedicated as chair drive signal
   static const int NUM_AO = 4;
   static const int CHAIR_CHANNEL = 0;

   
   // access device registers and other hardware functionality (eg, AO scaling coefficients) via this "parent" device
   CNI6363* m_pNI6363;
   
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6363_AO(const CNI6363_AO& src);
   CNI6363_AO& operator=(const CNI6363_AO& src);

public:
   // constructor/destructor
   CNI6363_AO(CNI6363* pNI6363); 
   ~CNI6363_AO() { m_pNI6363 = NULL; }

   // [CDevice impl/override]
   virtual LPCTSTR RTFCNDCL GetDeviceName() { return("PCIe-6363, AO Subsystem"); }
   BOOL RTFCNDCL Init();
   BOOL RTFCNDCL Out(int ch, int b2sVolt);
   BOOL RTFCNDCL Out(int ch, float fVolt);

   // for testing only
   VOID RTFCNDCL TestUpdatePerformance();

protected:
   // NOTE: The subdevice object does not handle device resource mapping; that's handled by the parent device!
   BOOL RTFCNDCL MapDeviceResources() { return(TRUE); }
   VOID RTFCNDCL UnmapDeviceResources() {}
};


/** CNI6363_DIO: Implementation of CCxEventTimer using the DI, DO and G0 subsystems of the PCIe-6363. */
class CNI6363_DIO : public CCxEventTimer
{
private:
   // some attributes: FIFO size (restricted by small size of counter FIFO!), # digital inputs available for 
   // timestamping, # digital outputs available. NOTE that we use the 32bits of Port0 for 16 DI and 16 DO.
   static const int EVENTFIFOSIZE = 127;
   static const int NUM_DI = 16;
   static const int NUM_DO = 16;
   
   // access device registers and other hardware functionality via this "parent" device
   CNI6363* m_pNI6363;
   
   // current state of the event timestamping feature
   typedef enum
   {
      evtUNKNOWN = 0,
      evtERROR,
      evtINITD,
      evtREADY,
      evtRUNNING,
      evtDIDNOTARM,
      evtSTOPPED
   } EVTState;

   EVTState m_evtState;
   BOOL m_IsFirstInit;
   
   // soft copies of selected registers
   u32 m_soft_DI_Trigger_Select;
   u32 m_soft_DIT_Mode_1;
   u32 m_soft_DIT_Mode_2;

   // state of 16 DI channels on the last detected RE or FE transition (so we can detect rising edge transitions)
   u16 m_lastInputState;

   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6363_DIO(const CNI6363_DIO& src);
   CNI6363_DIO& operator=(const CNI6363_DIO& src);

public: 
   // constructor/destructor
   CNI6363_DIO(CNI6363* pNI6363); 
   ~CNI6363_DIO() { m_pNI6363 = NULL; }

   // CDevice/CCxEventTimer implementation
   BOOL RTFCNDCL Init();
   VOID RTFCNDCL OnClose();
   int RTFCNDCL Configure(int clkPerUS, DWORD enaVec);
   VOID RTFCNDCL Start();
   VOID RTFCNDCL Stop();
   DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, PDWORD pTimes);
   DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, float *pfTimes);
   DWORD RTFCNDCL SetDO(DWORD dwVec);

   // for testing only
   VOID RTFCNDCL TestShortPulseTimestampPerformance();

protected:
   // NOTE: The subdevice object does not handle device resource mapping; that's handled by the parent device!
   BOOL RTFCNDCL MapDeviceResources() { return(TRUE); }
   VOID RTFCNDCL UnmapDeviceResources() {}

private:
   VOID RTFCNDCL ResetAll();
};


/** CNI6363: "Parent device" implementing register space access to the PCIe-6363. */
class CNI6363 : public CDevice
{
private:
   // device identification information
   static const CDevice::DevInfo DEVINFO; 
   
   // error message strings
   static LPCTSTR EMSG_BADCHINCHSIG; 
   static LPCTSTR EMSG_BADSTC3SIG; 
   static LPCTSTR EMSG_BADSUBSYSTEMID; 
   static LPCTSTR EMSG_FAILRWTEST_CHINCH;
   static LPCTSTR EMSG_FAILRWTEST_STC;
   static LPCTSTR EMSG_FAILEEPROMREAD;
   
   // memory-mapped address space for the device registers. XSeries boards have a single register address space,
   // accessed via BAR0. Declared as PVOID because it may refer to 1-, 2- or 4-byte registers.
   PVOID m_pvRegisters; 

   // pseudo subdevices representing the three device functions implemented on the PCIe-6363
   CNI6363_AI* m_pAI;
   CNI6363_AO* m_pAO;
   CNI6363_DIO* m_pDIO;
   
   static const int NUMAICOEFFS = 4;
   static const int NUMAOCOEFFS = 2;
   static const int NUMDACS = 4;
   
   // AI scaling coefficients C0..C3 for voltage range +/-10V in AI mode 0 (whatever that is; DDK not clear on this)
   // To convert ADC code x to volts: C3*x^3 + C2*x^2 + C1*x + C0. There is only 1 multiplexed ADC on the PCIe-6363.
   f32 m_aiCoeffs[NUMAICOEFFS];
   
   // AO scaling coefficients C0..C1 for voltage range +/-10V, for each DAC (one per channel) on the PCIe-6363.
   f32 m_aoCoeffs[NUMDACS][NUMAOCOEFFS];

private:
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CNI6363(const CNI6363& src); 
   CNI6363& operator=(const CNI6363& src); 

public: 
   // constructor/destructor
   CNI6363(int iDevNum); 
   ~CNI6363();

   // access to the three "subdevices" implemented on the PCIe-6363
   CCxAnalogIn* GetAISubDevice() { return(m_pAI); }
   CCxAnalogOut* GetAOSubDevice() { return(m_pAO); }
   CCxEventTimer* GetEventTimerSubDevice() { return(m_pDIO); }
   
   // [CDevice impl/overrides] 
   virtual LPCTSTR RTFCNDCL GetDeviceName() { return( "PCIe-6363" ); }
   BOOL RTFCNDCL Init(); 
   
   // for testing only
   VOID RTFCNDCL RunPerformanceTests();
   BOOL RTFCNDCL RunCtrCountdownTest(int tWaitUS, double& tElapsedUS);

protected:
   BOOL RTFCNDCL MapDeviceResources(); 
   VOID RTFCNDCL UnmapDeviceResources(); 
   BOOL RTFCNDCL OnOpen();
   VOID RTFCNDCL OnClose();
   
private:
   // inline methods for writing and reading individual registers on the PCIe-6363. The device function classes will
   // use these a lot. They're private, so the function classes are friends of this class. Both Windows host and the
   // PCIe-6363 are little-endian, so we don't need to do byte-swapping for the u16 and u32 registers.
   // IMPORTANT: The volatile keyword is critical here, because hardware registers can change at any time. Compiler
   // optimizations can really screw-up HW register access code if the volatile keyword is missing!!!!!!!!!!
   VOID RTFCNDCL WriteReg8(u32 addr, u32 datum) { *((u8 volatile *) (((u8 *)m_pvRegisters) + addr)) = (u8) datum; }
   VOID RTFCNDCL WriteReg16(u32 addr, u32 datum) { *((u16 volatile *) (((u16 *)m_pvRegisters) + (addr>>1))) = (u16) datum; }
   VOID RTFCNDCL WriteReg32(u32 addr, u32 datum) { *((u32 volatile *) (((u32 *)m_pvRegisters) + (addr>>2))) = datum; }

   u8 RTFCNDCL ReadReg8(u32 addr) { return(*((u8 volatile *) (((u8 *)m_pvRegisters) + addr))); }
   u16 RTFCNDCL ReadReg16(u32 addr) { return(*((u16 volatile *) (((u16 *)m_pvRegisters) + (addr>>1)))); }
   u32 RTFCNDCL ReadReg32(u32 addr) { return(*((u32 volatile *) (((u32 *)m_pvRegisters) + (addr>>2)))); }

   friend class CNI6363_AI;
   friend class CNI6363_AO;
   friend class CNI6363_DIO;
   
   // reads and validates contents of specific signature/ID registers onboard the NI6363 (sanity check)
   BOOL RTFCNDCL CheckDeviceSignatures();
   // retrieves AI and AO calibration information from device EEPROM
   BOOL RTFCNDCL GetCalibInfoFromEEPROM();
   // helper methods for reading EEPROM contents. EEPROM addresses are specified as an offset from the start of EEPROM.
   u8 RTFCNDCL EepromRead8(u32 addr);
   u16 RTFCNDCL EepromRead16(u32 addr);
   u32 RTFCNDCL EepromRead32(u32 addr);
   f32 RTFCNDCL EepromReadF32(u32 addr);
   // helper method parses the "device-specific node" in EEPROM in which calibration data is stored
   BOOL RTFCNDCL ParseCalibrationNodeInEEPROM(u32 addr);
   // test scratchpad registers in the CHInCh interface and the DAQ-STC3 (sanity check)
   BOOL RTFCNDCL TestScratchPadRegisters();
   // performs a software reset of the DAQ-STC; disables and acks all possible board interrupts
   BOOL RTFCNDCL ResetSTC();
   
   // converts 16-bit ADC code to calibrated voltage using AI scaling coefficients for the +/-10V range
   f32 RTFCNDCL ADCToVolts(i16 raw)
   {
      f32 volt = 0;
      if(IsOn()) for(int i=NUMAICOEFFS-1; i>=0; --i)
      {
         volt *= raw;
         volt += m_aiCoeffs[i];
      }
      return(volt);
   }
   
   // converts output voltage to corresponding 16-bit DAC code using AO scaling coefficients for the +/-10V range. Each
   // channel has its own DAC, and each DAC has its own calibrated scaling coefficients!
   i16 RTFCNDCL VoltsToDAC(u32 ch, f32 volt)
   {
      f32 res = 0.0f;
      if(IsOn() && ch >= 0 && ch < NUMDACS) res = volt*m_aoCoeffs[ch][1] + m_aoCoeffs[ch][0];
      return( static_cast<i16>(res) );
   }
};


#endif   // !defined(NI6363_H__INCLUDED_)

