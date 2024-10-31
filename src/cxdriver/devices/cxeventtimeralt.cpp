/**===================================================================================================================== 
 cxeventtimeralt.cpp : An alternative, simpler implementation of CXDRIVDER's DIO event timer hardware device interface.

 AUTHOR:  saruffner

 MOTIVATION/BACKGROUND:
 CCxEventTimerAlt is an alternative DIO event timer device interface that provides less functionality than the current
 CCxEventTimer, but offers a much simpler implementation.
 
 IT IS NOT CURRENTLY USED IN MAESTRO; IT IS ONLY A PROPOSED INTERFACE. The motivation for it is the overly complex 
 (and expensive) DIO interface panel that must be built to house all of the "latched external devices" currently
 supported by Maestro: the marker pulse module, the adjustable reward deliver module, the Plexon interface module,
 the pulse stimulus generator module (PSGM), and the miscellaneous signals module (for the audio reward pulse).

 As of Oct 2024: (1) The PSGM was designed but never built. (2) The Plexon interface module may no longer be needed.
 No labs are using the original Plexon MAP server. The newer Omniplex' digital input board timestamps all inputs at
 40KHz (25us), so it is reasonable for Maestro to deliver the marker pulses directly on dedicated DO lines rather
 than this task to an external latched device. Similarly, the functionality of the character writer module in the 
 original Plexon interface can be handled directly by Maestro using 9 additional dedicated DO lines (an 8-bit
 character and the data ready pulse). (3) Tests have shown that it is possible to use an RTX64 timer interrupt to
 generate "software-timed" TTL pulses on any DO line. For example, over 100K presentations of a 25-ms nominal pulse,
 the measured pulse lengths (measured indirectly as the elapsed time between the two SetDO() calls that raise and
 lower the pulse) were all in the range 24.903 - 25.003 ms, with an average of 24.97ms.

 CCxEventTimerAlt's capabilities and design take these realities into account.
 
 DESCRIPTION:
 Maestro uses a "DIO event timer" device to deliver TTL marker pulses to external equipment and to timestamp digital 
 input events. An "event" is the occurrence of a rising edge on one or more of the TTL digital inputs. In the past, the 
 most important "event" in the context of Maestro was an action potential, represented by a TTL pulse from a "window 
 discriminator" in the experiment rig. Nowadays, spike trains are recorded by multielectrode recording systems like
 Omniplex and Neuropixels, and Maestro primarily timestamps its own marker pulses to mark key time points in trials --
 which is important for synchronizing Maestro-recorded data traces with spike data recorded externally.

 In addition, it offers a means of writing ASCII characters over an 8-bit bus (8 dedicated DO lines), with an additional
 DO line for the "data ready" signal. This provides a mechanism for transmitting character streams to an external
 DAQ system -- again for synchronizing with Maestro's timeline.

 CCxEventTimerAlt is an ABSTRACT "interface" class that attempts to expose the DIO timer device's functionality in a 
 device-independent manner. To satisfy this interface, a candidate "event timer" implementation must meet these minimum
 functional requirements:
    1) At least 16 digital (TTL) inputs DI<15..0> for recording "rising-edge" events during an experiment. Device should 
 be capable of independently enabling/disabling each input.
    2) At least 24 digital (TTL) outputs DO<23..0> that can be synchronously updated or individually updated without
 affecting the on/off state of any of the other outputs.
 "immediate-mode" -- i.e., it must be 
 possible to simultaneously set the desired states of all outputs. Digital output updates should be possible at any 
 time, even while the board is engaged in event timestamping.
    3) Event timestamping. When timestamping is enabled, a 32-bit event clock ticks along while the device "looks" for
 a rising-edge on any of the enabled digital inputs. Whenever a rising edge is detected, the device must store both the 
 current "event mask" (state of the 16 DI channels) and the current "event time" (a 32-bit tick count from the event 
 clock). Device must support polling for events every 1 or 2ms while timestamping continues, and unloading all remaining
 event mask and timestamp pairs once timestamping has stopped. Exactly how event mask and time are stored can vary from 
 device to device, but the events & associated times must be retrieved in chronological order. Under Maestro, the most
 frequently timestamped event will be the window-discriminated spikes; at worst-case, expect no more than several 
 during a 1-ms epoch. Thus, the event store requirement is relatively modest. [NOTE:  Though not required by the this
 interface, devices should be able to detect errors such as "event storage full" or "event clock overflow".]
    4) Event "times" are really elapsed tick counts from the 32-bit event clock. The actual elapsed time is the tick 
 count * the event clock period. Devices should support clock periods ranging from 1us to 10ms, although 10us is an 
 acceptable lower limit -- MaestroRTSS only uses a 10us event clock.
    5) Board init. Device should provide a software-controlled means of reinitializing itself to an "idle" state: 
 event timestamping off, all DI channels disabled, all DO channels driven low (0).


 Supported functionaliy:
    1) Configure(), Start(), Stop(), UnloadEvents() - Event timestamping on DI<15..0>. Same functionality as in
CCxEventTimer.
    2) TriggerMarkers() - General purpose marker pulses. An approximate 30us pulse may be delivered on any of the 12 
 digital outputs DO<11..0>. The pulse duration is achieved by calling SetDO() once to raise the pulse, busy waiting for 
 30us, then calling SetDO() again to lower the pulse. CCxEventTimerAlt maintains a software copy of all 12 digital 
 outputs so that pulses can be delivered on any combination of channels.
    3) DeliverReward() - Software-timed, variable-length TTL pulses on DO<12> (reward delivery pulse) and DO<13> (audio 
 speaker tone). A software-timed pulse of duration 1-4000ms is achieved by raising the pulse via SetDO(), then starting 
 a one-shot RTX64 timer of the requested duration, and calling SetDO() in the timer handler to lower the pulse. A
 separate timer thread and timer interrupt handler is dedicated to the reward delivery pulse and the audio tone.
    4) WriteChar(), WriteString() - Stream 8-bit ASCII characters via an 8-bit "bus" (DO<23-16>), with a "data ready"
pulse on DO<15> to indicate that the 8-bit character is ready to be read by an external system. As Maestro only uses
this functionality in non-time-critical code sections, WriteChar() calls SetDO() to write the 8-bit character on
DO<23-16>, busy waits 3us, calls SetDO() to raise the "data ready" signal on DO<15>, busy waits 30us, then calls 
SetDO() again to lower DO<15>. WriteString() simply calls WriteChar() to write the entire character sequence. The
presumption here is that the external system can digest characters this quickly.

==> NOTE: No busy waits between register writes in SetDO().
The Joshua lab had a problem with their DIO interface panel occasionally missing come commands to latched external
devices because the "Data Ready" signal was too short. At the time, this was resolved by introducing busy waits in the
NI6363's implementation of SetDO(). The busy wait times were managed by CCxEventTimer and are read from a Windows
registry key. CCxEventTimerAlt does not support the "latched external devices" concept, so this feature is dropped.
Of course, if we were to do with CCxEventTimerAlt, the CNI6363_DIO would have to be updated accordingly.

 REVISION HISTORY:
 30oct2024-- Implementation adapted from original CCxEventTimer.
======================================================================================================================*/

#include "cxeventtimeralt.h"


/**
Construct the DIO event timer device and set device attributes. Derived class constructor should set these attributes 
IAW the actual event timer's capabilities. Here we enforce requirements on the number of DI and DO channels supported; 
if the requirements are not met, the # of DI and DO channels are forced to ZERO, disabling the device interface.

We create two RTX timer objects to implement software timing of the reward delivery pulse and audio tone pulse. If we
are unable to create either timer object, the device interface is again disabled.
 
 @param devInfo Device & vendor ID, etc. See CDevice.
 @param iDevNum Device instance number, usually 1. See CDevice.
 @param nDI, nDO # of DI and DO channels available. If either is invalid, both are set to ZERO!!
 */
CCxEventTimerAlt::CCxEventTimerAlt(const CDevice::DevInfo& devInfo, int iDevNum, int nDI, int nDO)
   : CDevice(devInfo, iDevNum)
{
   BOOL bInvalid = BOOL(nDI < NUM_DI_REQUIRED || nDO < NUM_DO_REQUIRED);
   m_nDI = bInvalid ? 0 : nDI;
   m_nDO = bInvalid ? 0 : nDO;

   m_dwDO = 0;
   m_iClockUS = 0;

   m_bSelfMonOn = m_bSelfMonError = FALSE;

   // create timer objects for software-timed pulses
   m_hRewardTimer = ::RtCreateTimer(NULL, 0, CCxEventTimerAlt::RewardPulseTimeout, (PVOID)this, 
      RT_PRIORITY_MAX, CLOCK_FASTEST);
   m_hAudioToneTimer = ::RtCreateTimer(NULL,0,CCxEventTimerAlt::AudioToneTimeout, (PVOID)this, 
      RT_PRIORITY_MAX, CLOCK_FASTEST); 
   if(m_hRewardTimer == NULL || m_hAudioToneTimer == NULL)
   {
      m_nDI = m_nDO = 0;
   }
}

/**
 Destructor. This deletes the timer objects created to handle the two software-timed pulses.
*/
CCxEventTimerAlt::~CCxEventTimerAlt()
{
   if(m_hAudioToneTimer != NULL)
   {
      ::RtDeleteTimer(m_hAudioToneTimer);
      m_hAudioToneTimer = NULL;
   }
   if(m_hRewardTimer != NULL)
   {
      ::RtDeleteTimer(m_hRewardTimer);
      m_hRewardTimer = NULL;
   }
}


/**
 Reinitialize the event timer device and its facility for self-monitoring event input statistics. 

 BACKGROUND: This self-monitoring facility is provided as a higher-level convenience feature that can be used to monitor
 input events registered on NUM_DI_REQUIRED digital input channels. It compiles several statistics on events recorded as
 the timer runs at a clock interval of 10microsecs (or the closest available clock period). For each input channel, the facility keeps 
 track of the # of events, the time of the most recent event, and the mean "inter-event" interval. It also maintains the
 "event mask" for the most recently recorded event. It is intended for use during test/debug of a new realization of the
 event timer device.

 USAGE: StartMonitor() reinitializes and starts both the timer and the monitoring facility. Once started, the facility 
 MUST be serviced frequently by calls to ServiceMonitor(). The required frequency will depend on how fast the 
 application expects events to be received (CCxEventTimerAlt supports software-polling of the event timer, NOT programmed 
 interrupts). It is NOT designed to monitor rapidly (>1KHz) changing digital inputs. To check the current event 
 statistics, call GetMonitor() at any time. Finally, to reset the self-monitoring facility, call StopMonitor().

 CAVEAT: The self-monitoring facility is built upon the lower-level functionality of this interface. It is 
 IMPERATIVE that calls to lower-level methods (with the exception of accessor methods like GetDO()) be avoided while 
 this facility is engaged. Derived classes can be designed to enforce this restriction.

 @return TRUE if successful; FALSE otherwise (device not available).
*/
BOOL RTFCNDCL CCxEventTimerAlt::StartMonitor()
{
   if(m_nDI == 0 || !IsOn()) 
   {
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL);
      return(FALSE);
   }
   
   // reset monitor facility; then configure event timestamping with 10us clock, all DI channels enabled
   StopMonitor(); 
   if(0 == Configure(10, (DWORD)(NUM_DI_REQUIRED - 1))) return(FALSE);

   // init self-monitoring state data
   m_bSelfMonOn = TRUE; 
   m_dwLastEvtMask = 0;
   for(int i = 0; i < NUM_DI_REQUIRED; i++)
   {
      m_nEvents[i] = 0;
      m_fTLast[i] = 0.0f;
      m_fSumIEI[i] = 0.0f;
   }

   Start();                                                    // start recording events
   return(TRUE);
}

/**
 If self-monitoring is turned on, download events from the event timer device and update event stats accordingly. See 
 StartMonitor() for further details.

 This method can download up to SM_BUFSZ events at a time. If the method is not called with sufficient frequency, the 
 monitoring facility will fall behind the event timer, and a FIFO overflow could occur. The CCxEventTimerAlt interface 
 does not currently support checking for FIFO, or counter, overflows.

 @param dwActiveMask [out] Bit N is set in this mask if at least one event on DI ch N occurred since the last service. 
 @return FALSE if self-monitoring halted (not started, counter overflow, or FIFO overflow error); else TRUE.
*/
BOOL RTFCNDCL CCxEventTimerAlt::ServiceMonitor(DWORD& dwActiveMask)
{
   if( (!m_bSelfMonOn) || m_bSelfMonError ) 
   {
      return( FALSE );
   }

   // unload new events from timer FIFO and update event stats accordingly...
   int n = (int)UnloadEvents(SM_BUFSZ, m_dwEvtMaskBuf, m_fEvtTimeBuf);
   for(int i = 0; i < n; i++) 
   {
      float fTime = m_fEvtTimeBuf[i];
      DWORD dwEvtMask = m_dwEvtMaskBuf[i];
      dwActiveMask |= dwEvtMask;
      DWORD dwChMask = (DWORD) (1<<0);
      int ch = 0;
      while(ch < NUM_DI_REQUIRED)
      {
         if((dwEvtMask & dwChMask) != 0)
         {
            ++(m_nEvents[ch]);
            m_fSumIEI[ch] += (fTime - m_fTLast[ch]);
            m_fTLast[ch] = fTime;
         }
         dwChMask <<= 1;
         ++ch;
      }
   }
   if(n > 0) m_dwLastEvtMask = m_dwEvtMaskBuf[n-1]; 

   return(!m_bSelfMonError);
}

/**
 Retrieve current input event statistics from the self-monitoring facility. See StartMonitor() for further details.
 @param piEvents [out] #events recorded per channel thus far. Buffer length >= N, where N is # of DI channels.
 @param pfTLast [out] Most recent event time per channel, in seconds. Buffer length >= N.
 @param pfIEI [out] Mean interevent interval per channel, in seconds. Buffer length >= N.
 @param dwMREMask [out] Bit mask for the last recorded event. 

 @return TRUE if successful; FALSE otherwise (self-monitor facility not engaged).
*/
BOOL RTFCNDCL CCxEventTimerAlt::GetMonitor(int* piEvents, float* pfTLast, float* pfIEI, DWORD& dwMREMask)
{
   if(!m_bSelfMonOn) return(FALSE);                                 // self-monitoring facility is not engaged!

   dwMREMask = m_dwLastEvtMask;                                     // copy the current input event stats 
   memcpy( piEvents, m_nEvents, sizeof(int)*GetNumDI() );
   memcpy( pfTLast, m_fTLast, sizeof(float)*GetNumDI() );
   memcpy( pfIEI, m_fSumIEI, sizeof(float)*GetNumDI() );
   for( int i = 0; i < GetNumDI(); i++ ) if( piEvents[i] > 0 )
      pfIEI[i] /= (float) piEvents[i];

   return(TRUE);
}

/**
 Reset both the self-monitoring facility and the event timer device itself. After this method is called, the digital 
 outputs are zero'd and event timing is disabled. See StartMonitor() for further details. This method has NO EFFECT if 
 the self-monitoring facility is already OFF.
*/
VOID RTFCNDCL CCxEventTimerAlt::StopMonitor()
{
   if(m_bSelfMonOn)
   {
      Init();
      m_bSelfMonOn = m_bSelfMonError = FALSE;
   }
}



/**
Deliver ~30us TTL pulses on selected data lines in DO<11..0>, which are dedicated to general-purpose marker pulses
in Maestro.

The pulses are NOT hardware-timed. Instead, the method calls SetDO() to raise the selected data lines (without
changing any of the other outputs), busy waits for 30us, then calls SetDO() again to lower the selected lines.

@param dwMask For each bit N in this mask, an active-high pulse is delivered on the corresponding digital output
channel DO<N>. Pulses are restricted to channels <11..0> only.
*/
VOID RTFCNDCL CCxEventTimerAlt::TriggerMarkers( DWORD dwMask )
{
   // mask out bits higher than 11. Raise, then lower selected data lines. At least one line must be pulsed, or no
   // action is taken.
   DWORD dw = dwMask & MARKERS_DOMASK; 
  
   if(dw != 0)
   {
      m_dwDO |= dw;
      SetDO(m_dwDO); 
      
      // ~30us busy wait. The volatile count var is there to ensure the loop does not get optimized away.
      CElapsedTime eTime;
      volatile long count = 0;
      while(eTime.Get() < 30) ++count;

      m_dwDO &= ~dw;
      SetDO(m_dwDO);
   }
}


/**
 Deliver reward to animal, possibly subject to random withholding.

 The physical reward is delivered as a software-timed "reward pulse" on channel DO<12>, which is dedicated for this
 purpose. External circuitry may be needed to amplify this TTL pulse to provide the necessary power/current to
 drive the solenoid that delivers the actual liquid reward. The requested pulse length is achieved by raising DO<12>,
 then starting a one-shot timer with the requested pulse length; when the timer expires, the timer handler lowers
 DO<12>.

 If a "variable ratio" N > 1 is specified, then 1 of every N physical rewards is randomly withheld (on average).
 Allowed range of N is [1..10]. The reward is also withheld if the specified duration is 0 or negative.

 Regardless of whether the physical reward was delivered, we always deliver an "audio tone pulse" on channel DO<13>;
 this is also software-timed in the manner of the reward pulse on DO<12>, albeit using a seperate timer object. This
 pulse is intended to drive a simple speaker in the rig, providing an audible cue that the subject successfully 
 completed the task. If the audio tone duration is 0, no pulse is delivered.

 After delivering the reward, the method writes a "reward delivered" character code (ASCII 0x05), immediately followed 
 by a null-terminated string reporting the specified adjustable reward pulse duration in ms. See WriteChar/String().

 NOTE: Callers should ensure that the interval between rewards is longer than the greater of the audio reward duration 
 and the adjustable reward duration. 

 @param iVR Variable ratio; allowed range is [1..10].
 @param iAdjDur Desired duration of the adjustable reward pulse, in ms; allowed range [0..4000]; <=0 is also treated as
 a withheld reward.
 @param iAudioDur Desired duration of "audio tone" pulse, in ms; allowed range [0..1000]; <=0 disables audio tone.

 @param TRUE if reward was delivered; FALSE if it was randomly withheld. Also returns FALSE if iAdjDur<=0.
*/
BOOL RTFCNDCL CCxEventTimerAlt::DeliverReward(int iVR, int iAdjDur, int iAudioDur)
{
   // restrict args to allowed ranges
   iVR = (iVR<1) ? 1 : ((iVR>10) ? 10 : iVR); 
   iAdjDur = (iAdjDur<0) ? 0 : ((iAdjDur>4000) ? 4000 : iAdjDur);
   iAudioDur = (iAudioDur<0) ? 0 : ((iAudioDur>1000) ? 1000 : iAudioDur);

   // withhold reward?
   BOOL bWithheld = (iAdjDur<= 0) || ((iVR>1) && (m_rand.Generate( (WORD) (iVR-1) ) == 0));

   // deliver adj-length reward, unless it is withheld
   if(!bWithheld)
   {
      // must cut short previous reward pulse if it is still active
      if(m_dwDO & REWARD_DO)
      {
         ::RtCancelTimer(m_hRewardTimer, NULL);
         m_dwDO &= ~REWARD_DO;
         SetDO(m_dwDO);
      }

      // calculate desired reward pulse duration in RTX clock ticks
      LARGE_INTEGER i64Dur;
      double dDummy = ((double)iAdjDur) * 10026.7366945;
      i64Dur.QuadPart = (LONGLONG)dDummy;

      // start one-shot timer; if successful, raise the audio reward pulse
      if(::RtSetTimerRelative(m_hRewardTimer, &i64Dur, NULL))
      {
         m_dwDO |= REWARD_DO;
         SetDO(m_dwDO);
      }
   }

   // analogously for the audio tone pulse, which is always delivered unless the specified duration is zero.
   if(iAudioDur > 0)
   {
      if(m_dwDO & AUDIOTONE_DO)
      {
         ::RtCancelTimer(m_hAudioToneTimer, NULL);
         m_dwDO &= ~AUDIOTONE_DO;
         SetDO(m_dwDO);
      }

      LARGE_INTEGER i64Dur;
      double dDummy = ((double)iAudioDur) * 10026.7366945;
      i64Dur.QuadPart = (LONGLONG)dDummy;

      if(::RtSetTimerRelative(m_hAudioToneTimer, &i64Dur, NULL))
      {
         m_dwDO |= AUDIOTONE_DO;
         SetDO(m_dwDO);
      }
   }

   // write "reward delivered" character code followed by reward pulse length as a null-terminated string
   if(!bWithheld) 
   {
      WriteChar(REW_CHARCODE); 
      char strRewLen[8];
      ::sprintf_s(strRewLen, "%d", iAdjDur);
      WriteString(strRewLen, ((int) ::strlen(strRewLen)) + 1);
   }

   return(!bWithheld);
}


/**
 Transmit a single 8-bit ASCII character on channels DO<23..16>, which are dedicated to this purpose.

 After setting DO<23..16> to encode the specified character (DO<23> is bit 7, DO<16> is bit 0), a 30us active-high 
 "data ready" pulse is delivered on dedicated channel DO<15>. Note that DO<23..16> are not reset to 0x00 after the 
 pulse; the external system should latch in a character only upon detecting the "data ready" pulse.

 @param c The ASCII character to be written.
*/
VOID RTFCNDCL CCxEventTimerAlt::WriteChar(char c) 
{ 
   m_dwDO &= ~CHAR_DOMASK;
   m_dwDO |= CHAR_DOMASK & (((DWORD) (0x00FF & c)) << CHAR_BIT0_DO);
   SetDO(m_dwDO); 

   m_dwDO |= DATAREADY_DO;
   SetDO(m_dwDO);

   // ~30us busy wait. The volatile count var is there to ensure the loop does not get optimized away.
   CElapsedTime eTime;
   volatile long count = 0;
   while(eTime.Get() < 30) ++count;

   m_dwDO &= ~DATAREADY_DO;
   SetDO(m_dwDO);
}

/**
 Transmit a null-terminated ASCII string on channels DO<23..16> via repeated calls to WriteChar(). Expect the execution
 time for this method to increase with string length; not intended for use in time-critical code sections.
 @param str An ASCII chracter string to be written, terminated by a null (if it isn't already).
 @param len Length of the character string.
*/
VOID RTFCNDCL CCxEventTimerAlt::WriteString(char* str, int len)
{
   for(int i = 0; i < len; i++)
      WriteChar(str[i]);
   if((len > 0) && (str[len - 1] != 0))
      WriteChar((char) 0);
}
