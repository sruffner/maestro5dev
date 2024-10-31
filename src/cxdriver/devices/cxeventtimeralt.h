/**===================================================================================================================== 
 cxeventtimeralt.h : Declaration of ABSTRACT hardware interface CCxEventTimerAlt, and placeholder CCxNullEvtTmrAlt.
======================================================================================================================*/

#if !defined(CXEVENTTIMERALT_H__INCLUDED_)
#define CXEVENTTIMERALT_H__INCLUDED_

#include "device.h"                    // CDevice -- base class for CNTRLX device interfaces  
#include "util.h"                      // for CRand16 -- a pseudo-random number generator
#include "cxobj_ifc.h"                 // for defn of struct SGMPARMS, the non-encoded form of SGM parameters


class CCxEventTimerAlt : public CDevice
{
public: 
   // #DI channels required by interface -- device must be able to timestamp TTL events on any of the channels
   static const int NUM_DI_REQUIRED = 16;

   // #DO channels required by interace -- device must be able to update any combination of these channels at any time.
   // Hardware-timed digital output generation not required.
   static const int NUM_DO_REQUIRED = 24;

private:
   // buffer size used by self-monitor facility to downloaded timestamped DI events
   static const int SM_BUFSZ = 100; 

   // "reward delivered" char code (followed by reward pulse length as null-terminated integer-valued string)
   static const char REW_CHARCODE = 0x05;

   // DO<11..0> dedicated to 12 generic marker pulses
   static const DWORD MARKERS_DOMASK = (DWORD)0x0FFF;

   // DO<12> dedicated to reward delivery pulse
   static const DWORD REWARD_DO = ((DWORD)(1 << 12)); 

   // DO<13> dedicated to audio tone pulse
   static const DWORD AUDIOTONE_DO = ((DWORD)(1 << 13)); 

   // DO<15> dedicated to the "data ready" signal for 8-bit character on DO<23..16>
   static const DWORD DATAREADY_DO = ((DWORD)(1 << 15));

   // DO<23..16> is the 8-bit bus for character writes
   static const DWORD CHAR_DOMASK = (DWORD)0x00FF0000;

   // DO<16> is the least significant bit of the character
   static const DWORD CHAR_BIT0_DO = 16;


   int      m_nDI;                           // # of digital inputs supported by device
   int      m_nDO;                           // # of digital outputs supported by device

  // information and storage for the event timer's self-monitoring facilities: 
   BOOL     m_bSelfMonOn;                    // self-monitoring facility engaged
   BOOL     m_bSelfMonError;                 // self-monitoring facility stopped on FIFO/ctr overflow error
   DWORD    m_dwLastEvtMask;                 // event mask for the most recently recorded event
   int      m_nEvents[NUM_DI_REQUIRED];      // # events recorded (per channel) since self-monitoring started
   float    m_fTLast[NUM_DI_REQUIRED];       // time of last event in seconds (per channel)
   float    m_fSumIEI[NUM_DI_REQUIRED];      // sum of inter-event intervals in seconds (per channel)
   DWORD    m_dwEvtMaskBuf[SM_BUFSZ];        // buffers for downloading events & timestamps from event timer
   float    m_fEvtTimeBuf[SM_BUFSZ]; 

   CRand16  m_rand;                          // to implement random reward withholding
   HANDLE   m_hRewardTimer;                  // handle to RTX64 timer handler for software-timed reward pulse (DO<12>)
   HANDLE   m_hAudioToneTimer;               // handle to RTX64 timer handler for soft-timed audio tone pulse (DO<13>)

protected:
   DWORD    m_dwDO;                          // current state of the 24 digital outputs (DO<23..0>)
   int      m_iClockUS;                      // current clock interval for event timing, in microsecs 


private:
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CCxEventTimerAlt(const CCxEventTimerAlt& src); 
   CCxEventTimerAlt& operator=(const CCxEventTimerAlt& src); 

public: 
   // constructor/destructor
   CCxEventTimerAlt(const CDevice::DevInfo& devInfo, int iDevNum, int nDI, int nDO);
   virtual ~CCxEventTimerAlt(); 

   // some device capabilities: #DO channels, #DI channels
   int RTFCNDCL GetNumDO() { return(IsOn() ? m_nDO : 0); }
   int RTFCNDCL GetNumDI() { return(IsOn() ? m_nDI : 0); }


   /**
    Configure device for DI event timestamping.
    @param clkPerUS Clock period for timestamping function, in microseconds
    @param enaVec Channel enable mask - Rising edges on DI channel N are timestamped iff bit N is set here.
    @return Actual clock period used, in us. Returns 0 to indicate operation failed.
   */
   virtual int RTFCNDCL Configure(int clkPerUS, DWORD enaVec ) = 0;
   /** Start previously configured DI event timestamping operation ("software start"). */
   virtual VOID RTFCNDCL Start() = 0; 
   /** Stop the DI event timestamping operation in progress (event store is NOT emptied). */
   virtual VOID RTFCNDCL Stop() = 0; 
   /**
    Unload DI event timestamping store in chronological order.
    @param nToRead Max #events to read (>= size of provided buffers).
    @param pEvents, pTimes [out] Buffers provided for storing event masks and corresponding event times. Event times
    are in timestamp clock ticks.
    @return # events actually read.
   */
   virtual DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, PDWORD pTimes) = 0; 
   /** Alternate version gets event times in seconds rather than timestamp clock ticks. */
   virtual DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, float *pfTimes) = 0;
   /** 
    Immediately update the event timer's digital output port.
    @param dwVec The new DO port value -- each bit in mask indicates new state of corresponding DO channel.
    @return Previous state of DO port.
   */
   virtual DWORD RTFCNDCL SetDO(DWORD dwVec) = 0; 

   /**
    Get current state of the event timer's digital output port.
    @return Current state of DO port. Bit N indicates state of DO channel N (1=high, 0=low).
   */
   virtual DWORD RTFCNDCL GetDO() { return(m_dwDO); }


   // event-timer's self-monitor facility; intended for test/debug use
   BOOL RTFCNDCL StartMonitor(); 
   BOOL RTFCNDCL ServiceMonitor(DWORD& dwActiveMask);
   BOOL RTFCNDCL GetMonitor(int* piEvents, float* pfTLast, float* pfIEI, DWORD& dwMREMask);
   VOID RTFCNDCL StopMonitor();

   // marker pulse delivery on DO<11..0>
   VOID RTFCNDCL TriggerMarkers(DWORD dwMask);
   // reward delivery, possibly subject to random withholding
   BOOL RTFCNDCL DeliverReward(int iVR, int iAdjDur, int iAudioDur);

   // write a single ASCII character or a null-terminated string via dedicated 8-bit bus on DO<23..16>
   // with "data ready" latching pulse on DO<15>
   VOID RTFCNDCL WriteChar(char c); 
   VOID RTFCNDCL WriteString(char* str, int len);

private:
   // one-shot timer handler terminates the audio tone pulse on DO<13>
   static VOID RTFCNDCL AudioToneTimeout(PVOID pThis) 
   {
      CCxEventTimerAlt* pTimer = (CCxEventTimerAlt*)pThis;
      pTimer->m_dwDO &= ~AUDIOTONE_DO;
      pTimer->SetDO(pTimer->m_dwDO);
   }

   // one-shot timer handler terminates the reward delivery pulse on DO<12>
   static VOID RTFCNDCL RewardPulseTimeout(PVOID pThis)
   {
      CCxEventTimerAlt* pTimer = (CCxEventTimerAlt*)pThis;
      pTimer->m_dwDO &= ~REWARD_DO;
      pTimer->SetDO(pTimer->m_dwDO);
   }

};


/** CCxNullEvtTmrAlt -- "No device found" placeholder for CCxEventTimerAlt interface */
class CCxNullEvtTmrAlt : public CCxEventTimerAlt
{
private:
   CCxNullEvtTmrAlt(const CCxNullEvtTmrAlt& src);
   CCxNullEvtTmrAlt& operator=(const CCxNullEvtTmrAlt& src);

public: 
   CCxNullEvtTmrAlt(const CDevice::DevInfo& devInfo, int iDevNum) : CCxEventTimerAlt(devInfo, iDevNum, 0, 0) {}
   ~CCxNullEvtTmrAlt() {} 

   BOOL RTFCNDCL Init() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   int RTFCNDCL Configure(int clkPerUS, DWORD enaVec) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(0); }
   VOID RTFCNDCL Start() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); }
   VOID RTFCNDCL Stop() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); }
   DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, PDWORD pTimes) 
   { 
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL);
      return(0); 
   }
   DWORD RTFCNDCL UnloadEvents(DWORD nToRead, PDWORD pEvents, float *pfTimes) 
   { 
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL);
      return(0);
   }
   DWORD RTFCNDCL SetDO(DWORD dwVec) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(m_dwDO); }

protected:
   BOOL RTFCNDCL MapDeviceResources() { return(FALSE); }
   VOID RTFCNDCL UnmapDeviceResources() {}
};


#endif   // !defined(CXEVENTTIMERALT_H__INCLUDED_)
