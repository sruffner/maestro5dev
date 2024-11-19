/**===================================================================================================================== 
 cxeventtimer.h : Declaration of ABSTRACT hardware interface CCxEventTimer, and placeholder CCxNullEvtTmr.
======================================================================================================================*/

#if !defined(CXEVENTTIMER_H__INCLUDED_)
#define CXEVENTTIMER_H__INCLUDED_

#include "device.h"                    // CDevice -- base class for CNTRLX device interfaces  
#include "util.h"                      // for CRand16 -- a pseudo-random number generator


class CCxEventTimer : public CDevice
{
public: 
   // min/max #channels required/supported by interface
   static const int MIN_CHANNELS = 16;
   static const int MAX_CHANNELS = 32;

private:
   // buffer size used by self-monitor facility to downloaded timestamped DI events
   static const int SM_BUFSZ = 100; 

   // external latched-device addresses: Several digital devices are controlled by writing to event timer's DO<15..0>.
   // DO<15..12> is the device address, and DO<11..0> is the data latched into that device.
   static const DWORD DD_MARKERS = 0x00001000;           // marker pulse output
   static const DWORD DD_ADJREWARD = 0x00004000;         // adjustable rewards
   static const DWORD DD_MISC = 0x00006000;              // miscellaneous function (rarely used)
   static const DWORD DD_WRITER = 0x00007000;            // character writer for integration with Plexon

   // "reward delivered" char code (followed by reward pulse length as null-terminated integer-valued string)
   static const char REW_CHARCODE = 0x05;

   // data bits for device DD_MISC
   static const DWORD FIXSTAT_MISC = ((DWORD)(1 << 0));  // fixation status bit in ContMode 
   static const DWORD AUDIOREW_MISC = ((DWORD)(1 << 1)); // "audio" reward pulse on this DO line of DD_MISC device

   // the min, max and default software busy wait time after each state change in SetDO(), in microseconds
   static const int MIN_DOBUSYWAITUS = 0;
   static const int MAX_DOBUSYWAITUS = 20;
   static const int DEF_DOBUSYWAITUS = 3; 

   int      m_nDI;                           // # of digital inputs supported by device
   int      m_nDO;                           // # of digital outputs supported by device

  // information and storage for the event timer's self-monitoring facilities: 
   BOOL     m_bSelfMonOn;                    // self-monitoring facility engaged
   BOOL     m_bSelfMonError;                 // self-monitoring facility stopped on FIFO/ctr overflow error
   DWORD    m_dwLastEvtMask;                 // event mask for the most recently recorded event
   int      m_nEvents[MAX_CHANNELS];         // # events recorded (per channel) since self-monitoring started
   float    m_fTLast[MAX_CHANNELS];          // time of last event in seconds (per channel)
   float    m_fSumIEI[MAX_CHANNELS];         // sum of inter-event intervals in seconds (per channel)
   DWORD    m_dwEvtMaskBuf[SM_BUFSZ];        // buffers for downloading events & timestamps from event timer
   float    m_fEvtTimeBuf[SM_BUFSZ]; 

   DWORD    m_dwMisc;                        // current state of data bits DO<11..0> on device DD_MISC

   CRand16  m_rand;                          // to implement random reward withholding
   HANDLE   m_hAudioRewTimer;                // handle to RTX timer for implementing audio reward pulse

protected:
   DWORD    m_dwDO;                          // current state of up to 32 digital outputs (bit N = ch N's state)
   int      m_iClockUS;                      // current clock interval for event timing, in microsecs 

   // current busy wait times after each stage in the delivery of a latched DO command via SetDO(). The three
   // stages are: write 16-bit command on DO<15..0>, set DataReady=0 to "latch" command, set DataReady=1 to finish.
   float    m_fDOBusyWaits[3]; 

private:
   // prevent compiler from automatically providing default copy constructor and assignment operator
   CCxEventTimer(const CCxEventTimer& src); 
   CCxEventTimer& operator=(const CCxEventTimer& src); 

public: 
   // constructor/destructor
   CCxEventTimer(const CDevice::DevInfo& devInfo, int iDevNum, int nDI, int nDO);
   virtual ~CCxEventTimer(); 

   // some device capabilities: #DO channels, #DI channels
   int RTFCNDCL GetNumDO() { return(IsOn() ? m_nDO : 0); }
   int RTFCNDCL GetNumDI() { return(IsOn() ? m_nDI : 0); }

   VOID RTFCNDCL SetDOBusyWaitTimes(float f1, float f2, float f3)
   {
      m_fDOBusyWaits[0] = cMath::rangeLimit(f1, MIN_DOBUSYWAITUS, MAX_DOBUSYWAITUS);
      m_fDOBusyWaits[1] = cMath::rangeLimit(f2, MIN_DOBUSYWAITUS, MAX_DOBUSYWAITUS);
      m_fDOBusyWaits[2] = cMath::rangeLimit(f3, MIN_DOBUSYWAITUS, MAX_DOBUSYWAITUS);
   }

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

   // reset all latched devices controlled by the event timer's DO port
   VOID RTFCNDCL ResetLatchedDevices(); 
   // marker pulse delivery
   VOID RTFCNDCL TriggerMarkers(DWORD dwMask);
   // reward delivery, possibly subject to random withholding
   BOOL RTFCNDCL DeliverReward(int iVR, int iAdjDur, int iAudioDur);
   
   // raise/lower DO line reflecting "fixation status" on device DD_MISC
   VOID RTFCNDCL SetFixationStatus(); 
   VOID RTFCNDCL ClearFixationStatus(); 

   // write a single ASCII character or a null-terminated string via the character writer latched device
   VOID RTFCNDCL WriteChar(char c); 
   VOID RTFCNDCL WriteString(char* str, int len);

private:
   // one-shot timer handler terminates the audio reward pulse
   static VOID RTFCNDCL AudioTimeout(PVOID pThis) 
   {
      CCxEventTimer* pTimer = (CCxEventTimer*)pThis;
      pTimer->m_dwMisc &= ~AUDIOREW_MISC;
      pTimer->SetDO(pTimer->DD_MISC | pTimer->m_dwMisc);
   }
      
};


/** CCxNullEvtTmr -- "No device found" placeholder for CCxEventTimer interface */
class CCxNullEvtTmr : public CCxEventTimer
{
private:
   CCxNullEvtTmr(const CCxNullEvtTmr& src); 
   CCxNullEvtTmr& operator=(const CCxNullEvtTmr& src); 

public: 
   CCxNullEvtTmr(const CDevice::DevInfo& devInfo, int iDevNum) : CCxEventTimer(devInfo, iDevNum, 0, 0) {}
   ~CCxNullEvtTmr() {} 

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


#endif   // !defined(CXEVENTTIMER_H__INCLUDED_)
