/**===================================================================================================================== 
 cxeventtimer.cpp : Abstract CCxEventTimer, defining the MaestroRTSS DIO event timer hardware device interface.

 AUTHOR:  saruffner

 DESCRIPTION:
 MaestroRTSS uses a "DIO event timer" device to record and timestamp digital input events. An "event" is the occurrence 
 of a rising edge on one or more of the TTL digital inputs. The most important "event" in the context of Maestro is an
 action potential, which is represented by a TTL pulse from a "window discriminator" in the experiment rig. In addition,
 some Maestro experimental protocols use "marker pulses" to mark key time points in the protocol. By routing the marker 
 pulse lines and the window discriminator output(s) to the event timer's digital inputs (DI), MaestroRTSS can record 
 their times of occurrence during an experiment.

 In addition to the timestamping functionality, the event timer also provides at least 16 general-purpose digital 
 outputs. MaestroRTSS uses these TTL outputs to control a number of different external devices in the experiment rig.

 CCxEventTimer is an ABSTRACT "interface" class that attempts to expose the DIO timer device's functionality in a 
 device-independent manner. To satisfy this interface, a candidate "event timer" implementation must meet these minimum
 functional requirements:
    1) At least 16 digital (TTL) inputs for recording "rising-edge" events during an experiment. Device should be 
 capable of independently enabling/disabling each input.
    2) At least 16 digital (TTL) outputs that can be synchronously updated in "immediate-mode" -- i.e., it must be 
 possible to simultaneously latch the desired states of all outputs. Furthermore, the board must activate a "data ready" 
 signal (active LO, at least 100 ns) whenever an output update occurs. In the Maestro experiment rig's design, this 
 signal is used to latch the outputs into various external devices. Digital output updates should be possible at any 
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

 ==> Abstraction of MaestroRTSS "latched digital devices" controlled by the event timer's DOUT port.
 A number of Maestro-specific digital hardware devices are controlled by writing 16-bit words on the event timer's 
 digital outputs DO<15..0>. To talk to several different devices independently (but not simultaneously) we use a subset
 of the output lines, DO<15..12>, as a "device address". The remaining 12 "data" lines can be routed to any one of up to
 16 different devices. For this to work, of course, each device must have an address decoder which serves to latch the 
 applicable data lines only when that device is being addressed. MaestroRTSS currently employs five such "latched-data" 
 digital devices, all of which are encapsulated by CCxEventTimer. 

 Because these latched devices are encapsulated by the CCxEventTimer interface itself, any hardware platform realizing 
 the interface will be able to control these devices -- as long as the DO<15..0> lines are properly connected. No 
 additional work is required in the derived class! However, BEWARE: we use virtual method SetDO() to manipulate the 
 digital outputs; thus, intermingling calls to SetDO() with calls to the methods described below will potentially lead 
 to undefined behavior.

 Devices supported & relevant methods:
    1) Marker pulse control (dev DD_MARKERS = 0x1). This device merely latches the data lines DO<11..0> and presents 
 them as digital outputs. In Maestro, these are used to implement marker pulses which may trigger events on the event 
 timer device, or be used to trigger an external device. Invoke TriggerMarkers() to deliver short active-high pulses on 
 any combination of output lines. See detailed description of TriggerMarkers() for some important TECH NOTES.

    2) Miscellaneous signals (dev DD_MISC = 0x6). Another simple, "latched outputs" device. Only two outputs are used by 
 MaestroRTSS. DO0 is a "fixation status" signal (1=fixation within bounds, 0=fixation violated, or not enforced) that is
 probably obsolete. Use Set/ClearFixationStatus() to manipulate this signal. DO1 is used to drive a speaker, 
 implementing a simple "audio" reward; this usage is covered in a later section.

    3) Electrical pulse stimulus generator module (dev DD_SGM = 0x5). The PSGM is a programmable digital device that 
 controls a Grass stimulus generator which, in turn, delivers a sequence of electrical pulses to the brain via an 
 extracellular electrode. 
    CfgPulseSeq()     ==> configure a new pulse sequence on the module, but do not start it.
    StartPulseSeq()   ==> start a previously configured pulse sequence.
    StopPulseSeq()    ==> stop the current pulse sequence (but keep configuration, so we can start it again).
    ResetPulseSeq()   ==> reset the SGM (no pulse sequence configured).
    IsOnPulseSeq()    ==> TRUE if a pulse sequence was previously started on the SGM (it may have stopped).
    DisablePulseSeq() ==> if a pulse seq was configured to start by an ext trig, this disables the trigger.

    4) Reward delivery system (dev DD_ADJREWARD = 0x4, DD_MISC = 0x6). Originally, MaestroRTSS rewarded the animal by 
 triggering a fixed-length pulse to a solenoid which, in turn, delivered a fixed amount of fluid. The experiment rigs
 now use an adjustable reward pulse device (DD_ADJREWARD), which delivers a pulse whose duration in ms is specified by 
 data lines DO<11..0>. In addition, a random reward withholding scheme was introduced to train intractable subjects, 
 accompanied by an "audio" reward that is delivered whether or not the physical reward is withheld. The "audio" reward 
 is merely a pulse of programmable duration on DO1 of device DD_MISC; that pulse drives a speaker in the experiment rig.
 Since both the adjustable reward and the audio cue are realized on digital devices ultimately controlled by the event 
 timer's DO port, it made sense to encapsulate the entire "reward system" in CCxEventTimer. 
    Invoke DeliverReward() to deliver a reward to the subject -- see its detailed description for more info. 

    5) Character writer (dev DD_WRITER = 0x7). A simple device for transmitting ASCII characters to an external target. 
 When addressed, device should latch data lines DO<7..0>, which represents an 8-bit ASCII character. WriteChar() will 
 write a single character to the device, while WriteString() transmits an entire null-terminated string one character at
 a time. The characters in the string are transmitted in rapid succession -- the time between characters depending on
 how fast the calls to CCxEventTimer::SetDO() can be executed. The character writer device was introduced to provide a
 means of synchronizing Maestro's timeline with the Plexon multiacquisition processor (MAP), which has been used by the
 Lisberger lab since ~2004 for simultaneous recording from multiple electrodes. It includes a FIFO on the "input side"
 for queueing characters as they're written by MaestroRTSS via the characer writer device, plus handshaking logic on the
 "output side" for removing each character from the FIFO and safely transmitting it to the Plexon. Note that MaestroRTSS
 will not write more than 200 characters in any one burst.

 Busy waits in SetDO() [as of Maestro 4.1.1, Sep 2019]:
 Regardless the implementation, successful delivery of the 16-bit DO command to the "latched" devices listed above will
 involve three distinct steps:
    a) Write the DO command on DO<15..0>.
    b) Set DataReady=0 (active-low) to "latch" the command into the addressed device.
    c) After a minimum of 100ns, set DataReady=1 to complete the operation.
 In the NI PCIe-6363 implementation (which is the only implementation currently in Maestro 4.x), each step involves a
 single write to a memory-mapped register. This has been the case since the PCIe-6363 was introduced in Maestro 3.0. A
 single busy wait of 2.5us was introduced after lowering DataReady in an effort to ensure the "latching pulse" was at
 least that long. This has worked in the Lisberger lab through Maestro 4.1.0.
    However, upon upgrading to Win10/RTX64/Maestro 4, the Joshua lab discovered that it was frequently missing marker
 pulses during a trial -- particularly the RECORDMARKER pulse for Plexon integration and any general-purpose marker 
 delivered during the first millisecond of the trial (segment 0 marker). Using an oscilloscope to monitor the DataReady
 signal and a selected DO line right off the PCIe-6363 connector, the lab discovered that -- on the occasions when the
 marker pulse device failed to deliver a marker pulse, the active-low DataReady pulse was on the order of 100-200ns 
 instead of 2.5us. After a lot of testing with the Joshua lab's help, I came to the conclusion that we CANNOT assume
 that the effect of a register write on the PCIe-6363 occurs immediately upon the software executing the relevant
 machine instructions. Often there is no significant delay, but sometimes there is. If the delay is at least 2.5us, then
 the three register writes that perform steps (a)-(c) above could be "queued up" somehow and then -- after the delay --
 execute in rapid succession. The end result is that the DataReady pulse is too short. If this phenomenon occurs on the
 Lisberger lab workstations, it is apparently not a problem -- likely because the DIO interface can still detect that
 short of a pulse (the latching occurs on the falling edge). The Joshua lab's DIO interface is a different design and
 perhaps needs a pulse in the microsecond range.
    The right way to address this issue would be to do a hardware-timed digital output generation to perform the steps
 above. This is not possible on the NI PCIe-6363, because we don't have an extra DO line to dedicate as the DataReady
 signal (the PFI0 line is used for DataReady, and that signal cannot be controlled by a hardware-timed generation).
    Instead, we implemented a mechanism to set the busy wait times for each of the steps listed above, not just (b).
 The busy wait times default to 3us each, and can be set to any value between 0 and 20us via SetDOBusyWaitTimes().
 Implementations of CCxEventTimer are expected to honor these busy wait times in the execution of SetDO().

 ==> The device function interface framework in MaestroRTSS; base class CDevice.
 By creating an abstract interface class to define how MaestroRTSS uses a hardware device, we hope to isolate the
 functional *usage* of a device from the details of how the device actually works, and how we communicate with it. To 
 create a "practical implementation" of the interface, we derive a new class from the abstract interface and implement
 each of its pure virtual methods to support the target board.
 
 All MaestroRTSS hardware interfaces are derived from abstract class CDevice. CDevice defines several basic device
 operations, such as Open(), Close(), and Init(). It also has built-in support for PCI devices (most Maestro hardware is
 hosted on the PCI or PCI Express bus) and COFF loading support for DSP devices based on the TI C6x- or C4x-series 
 processors. Note that derived classes inherit a number of pure virtual methods from this class. Any practical device 
 class must implement these methods, which handle a number of device-specific operations. See CDevice for details.

 MaestroRTSS is an RTSS process, running in the Real-Time eXtension (RTX -- see CREDITS) subsystem for Windows. The RTX
 API provides kernel-level access, permitting direct communications with hardware and thus avoiding some of the 
 complexities and limitations of writing a custom kernel-mode device driver or working with manufacturer-supplied 
 drivers. As a component of MaestroRTSS, target implementations of a hardware interface are essentially specialized 
 device drivers. They use methods from the RTX API to perform the low-level operations typically found in a device 
 driver (read and write to port I/O addresses, mapping device memory, searching the PCI bus, etc.).

 CREDITS:
 1) Real-Time eXtenstion (RTX) to Windows by IntervalZero (www.intervalzero.com). RTX gives Windows real-time-like
 characteristics and provides kernel access for direct communications with hardware devices.

 REVISION HISTORY:
 16may2001-- Adapted from C source code module TIMERBRD from an older version of CXDRIVER.
 17may2001-- Working on a framework that supports different hardware devices in a seamless way. CCxEventTimer is an 
    abstract interface that defines CXDRIVER's functional requirements of any "DIO event timer" board. To add support 
    for a new hardware device, derive a class from CCxEventTimer and implement the interface (this is sort of like COM 
    or Java).  Then modify the static method CCxEventTimer::AttachToDevice() to include the new device in its search....
 28jun2001-- Added placeholder implementation CCxNullEvtTmr.
          -- Modify CCxEventTimer interface and implementations to support up to 32 DI's and DO's each.
          -- Overloaded version of UnloadEvents() retrieves event times in seconds rather than clock ticks. 
          -- Adding facility to monitor certain stats on events recorded on the timer's digital inputs. If this 
    construct works, it relieves the "external world" from having to implement this diagnostic capability directly via 
    calls to lower-level CCxEventTimer methods.
 10aug2001-- Modified ServiceMonitor() to set up a bit mask indicating all channels that recorded a pulse since the 
    last call.
 19oct2001-- Reworked CCxEventTimer using CDevice as base class...
 30jan2002-- Adding support for a new CNTRLX external device, the SGM pulse sequencer, which is driven by the first 16 
    DO channels of the event timer. The necessary functionality is handled by CCxEventTimer itself and is thus inherited
    by derived classes that implement the interface.
 14jun2002-- MAJOR structural revision:  Each practical implementation of CCxEventTimer is now placed in a separate 
    file. Also, we'll use a "device manager" class to instantiate devices, replacing the static method
    CCxEventTimer::AttachToDevice(). Finally, we revised interface IAW changes in base class CDevice.
 17sep2002-- Began encapsulating all other "latched-data" devices (marker pulse control, shutter control, adjustable 
    reward dev, and "miscellaneous signals" device) in CCxEventTimer.
 23sep2002-- Mod CfgPulseSeq() to use generic, non-encoded format for SGM parameters.
 04nov2003-- Added a new latched device, a "character writer" which writes one or a sequence of 8-bit ASCII chars on 
    DO<7..0>.  See methods WriteChar() and WriteString().
 16jun2005-- Modified DeliverReward() so that, after delivering the reward, it writes the "rewardDelivered" char code 
    (ASCII 0x05), followed by the reward pulse length in ms as a null-terminated integer-valued string.
 04apr2007-- Modified CfgPulseSeq() to sleep 100us between SetDO() calls. Found in testing that only the last of the 
    calls was actually executed. Spec says 1ms, but testing indicated that 100us was enough.
 24apr2007-- Revised to bring up-to-date with some changes in the PSGM spec, including new BIPHASICTRAIN op mode.
 13jun2007-- Further mods to ***PulseSeq() IAW additional tweaks to the PSGM spec.
 24jun2011-- Mods for Maestro 3.0: Since the shuttered fiber optic and LED targets will no longer be supported, we got 
    rid of the DD_SHUTTERS latched device and corresponding constants and methods. This also means that the 
    fixed-length reward pulse is no longer available -- DD_ADJREWARD must be used for reward pulse delivery.
 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.
 20may2019-- Slight mod to DeliverReward(): If the specified duration is zero, the reward is treated as "withheld". 
 This change was introduced to implement the per-trial random reward withholding variable ratio, which is distinct 
 from the global withholding variable ratio specified in the first argument to DeliverReward(). The two forms of 
 random withholding should not be used simultaneously!
 12aug2019-
 02sep2019-- Did some debugging of marker pulse delivery to address issues Mati Joshua's lab was having with missing
 the RECORDMARKER pulse and any marker pulse at T=0 during a trial. Has something to do with SetDO() implementation.
 05sep2019-- To resolve the above issue, introduced busy wait times for each of the three fundamental steps in any
 SetDO() implementation. See details above. Added method SetDOBusyWaitTimes(), plus protected member m_fDOBusyWaits[]
 to store the busy wait times. SetDO() implementations should be updated to honor these busy wait times.
======================================================================================================================*/

#include "cxeventtimer.h"





/**
 Construct the DIO event timer device and set device attributes. Derived class constructor should set these attributes 
 IAW the actual event timer's capabilities. Here we enforce restrictions on the # of channels supported; if the 
 restrictions are violated, the # of channels is forced to ZERO, rendering the device interface useless!

 We allocate RTX timer object to handle termination of the "audio" reward pulse. If we are unable to create the timer 
 object, then audio rewards will not be delivered.
 
 @param devInfo Device & vendor ID, etc. See CDevice.
 @param iDevNum Device instance number, usually 1. See CDevice.
 @param nDI, nDO # of DI and DO channels available. If either is invalid, both are set to ZERO!!
 */
CCxEventTimer::CCxEventTimer(const CDevice::DevInfo& devInfo, int iDevNum, int nDI, int nDO) : CDevice(devInfo, iDevNum)
{
   BOOL bInvalid = BOOL(nDI < MIN_CHANNELS || nDI > MAX_CHANNELS || nDO < MIN_CHANNELS || nDI > MAX_CHANNELS);
   m_nDI = bInvalid ? 0 : nDI;
   m_nDO = bInvalid ? 0 : nDO;

   m_dwMisc = 0;

   m_dwDO = 0;
   m_iClockUS = 0;

   // set default value for the busy waits in SetDO()
   m_fDOBusyWaits[0] =  m_fDOBusyWaits[1] = m_fDOBusyWaits[2] = CCxEventTimer::DEF_DOBUSYWAITUS;

   m_bSelfMonOn = m_bSelfMonError = FALSE;

   // create timer handler which terminates "audio" reward pulse. Handler requires THIS reference!
   m_hAudioRewTimer = ::RtCreateTimer(NULL,0,CCxEventTimer::AudioTimeout, (PVOID)this, RT_PRIORITY_MAX, CLOCK_FASTEST); 

   ResetPulseSeq();
}

/**
 Destructor. This deletes the timer object created to handle termination of the "audio" reward pulse.
*/
CCxEventTimer::~CCxEventTimer()
{
   if( m_hAudioRewTimer != NULL ) 
   {
      ::RtDeleteTimer( m_hAudioRewTimer );
      m_hAudioRewTimer = NULL;
   }
}


/**
 Reinitialize the event timer device and its facility for self-monitoring event input statistics. 

 BACKGROUND: This self-monitoring facility is provided as a higher-level convenience feature that can be used to monitor
 input events registered on the event timer device. It compiles several statistics on events recorded as the timer runs 
 at a clock interval of 10microsecs (or the closest available clock period). For each input channel, the facility keeps 
 track of the # of events, the time of the most recent event, and the mean "inter-event" interval. It also maintains the
 "event mask" for the most recently recorded event. It is intended for use during test/debug of a new realization of the
 event timer device.

 USAGE: StartMonitor() reinitializes and starts both the timer and the monitoring facility. Once started, the facility 
 MUST be serviced frequently by calls to ServiceMonitor(). The required frequency will depend on how fast the 
 application expects events to be received (CCxEventTimer supports software-polling of the event timer, NOT programmed 
 interrupts). It is NOT designed to monitor rapidly (>1KHz) changing digital inputs. To check the current event 
 statistics, call GetMonitor() at any time. Finally, to reset the self-monitoring facility, call StopMonitor().

 CAVEAT: The self-monitoring facility is built upon the lower-level functionality of the CCxEventTimer interface. It is 
 IMPERATIVE that calls to lower-level methods (with the exception of accessor methods like GetDO()) be avoided while 
 this facility is engaged. Derived classes can be designed to enforce this restriction.

 @return TRUE if successful; FALSE otherwise (device not available).
*/
BOOL RTFCNDCL CCxEventTimer::StartMonitor()
{
   if( !IsOn() )                                               // event timer dev not avail
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }
   StopMonitor();                                              // make sure everything is reset
   if( 0 == Configure( 10, 0xFFFFFFFF ) ) return( FALSE );     // ~10us event clk, all DI ena

   m_bSelfMonOn = TRUE;                                        // initialize self-monitoring state data 
   m_dwLastEvtMask = 0;
   for( int i = 0; i < GetNumDI(); i++ )
   {
      m_nEvents[i] = 0;
      m_fTLast[i] = 0.0f;
      m_fSumIEI[i] = 0.0f;
   }

   Start();                                                    // start recording events
   return( TRUE );
}

/**
 If self-monitoring is turned on, download events from the event timer device and update event stats accordingly. See 
 StartMonitor() for further details.

 This method can download up to SM_BUFSZ events at a time. If the method is not called with sufficient frequency, the 
 monitoring facility will fall behind the event timer, and a FIFO overflow could occur. The CCxEventTimer interface 
 does not currently support checking for FIFO, or counter, overflows.

 @param dwActiveMask [out] Bit N is set in this mask if at least one event on DI ch N occurred since the last service. 
 @return FALSE if self-monitoring halted (not started, counter overflow, or FIFO overflow error); else TRUE.
*/
BOOL RTFCNDCL CCxEventTimer::ServiceMonitor(DWORD& dwActiveMask)
{
   if( (!m_bSelfMonOn) || m_bSelfMonError )                             // facility not engaged, or stopped on error  
   {
      return( FALSE );
   }
   int n = (int)UnloadEvents(SM_BUFSZ, m_dwEvtMaskBuf, m_fEvtTimeBuf);  // unload new events from timer FIFO and update 
   for( int i = 0; i < n; i++ )                                         // event stats accordingly...
   {
      float fTime = m_fEvtTimeBuf[i];
      DWORD dwEvtMask = m_dwEvtMaskBuf[i];
      dwActiveMask |= dwEvtMask;
      DWORD dwChMask = (DWORD) (1<<0);
      int ch = 0;
      while( ch < GetNumDI() )
      {
         if( (dwEvtMask & dwChMask) != 0 )
         {
            ++(m_nEvents[ch]);
            m_fSumIEI[ch] += (fTime - m_fTLast[ch]);
            m_fTLast[ch] = fTime;
         }
         dwChMask <<= 1;
         ++ch;
      }
   }
   if( n > 0 ) m_dwLastEvtMask = m_dwEvtMaskBuf[n-1];                   // the most recent event mask 

   return( !m_bSelfMonError );
}

/**
 Retrieve current input event statistics from the self-monitoring facility. See StartMonitor() for further details.
 @param piEvents [out] #events recorded per channel thus far. Buffer length >= N, where N is # of DI channels.
 @param pfTLast [out] Most recent event time per channel, in seconds. Buffer length >= N.
 @param pfIEI [out] Mean interevent interval per channel, in seconds. Buffer length >= N.
 @param dwMREMask [out] Bit mask for the last recorded event. 

 @return TRUE if successful; FALSE otherwise (self-monitor facility not engaged).
*/
BOOL RTFCNDCL CCxEventTimer::GetMonitor( int* piEvents, float* pfTLast, float* pfIEI, DWORD& dwMREMask )
{
   if( !m_bSelfMonOn ) return( FALSE );                                 // self-monitoring facility is not engaged!

   dwMREMask = m_dwLastEvtMask;                                         // copy the current input event stats 
   memcpy( piEvents, m_nEvents, sizeof(int)*GetNumDI() );
   memcpy( pfTLast, m_fTLast, sizeof(float)*GetNumDI() );
   memcpy( pfIEI, m_fSumIEI, sizeof(float)*GetNumDI() );
   for( int i = 0; i < GetNumDI(); i++ ) if( piEvents[i] > 0 )
      pfIEI[i] /= (float) piEvents[i];

   return( TRUE );
}

/**
 Reset both the self-monitoring facility and the event timer device itself. After this method is called, the digital 
 outputs are zero'd and event timing is disabled. See StartMonitor() for further details. This method has NO EFFECT if 
 the self-monitoring facility is already OFF.
*/
VOID RTFCNDCL CCxEventTimer::StopMonitor()
{
   if( m_bSelfMonOn )
   {
      Init();
      m_bSelfMonOn = m_bSelfMonError = FALSE;
   }
}


/**
 Reset all Maestro latched digital devices controlled by the event timer's DO port. The character writer device is NOT
 reset, 0 to it is the same as transmitting a null character!
*/
VOID RTFCNDCL CCxEventTimer::ResetLatchedDevices()
{
   ResetPulseSeq();                 // stop & disable the PSGM
   SetDO( DD_MISC );                // clear miscellaneous signals
   m_dwMisc = 0;

   SetDO( DD_MARKERS );             // since the marker pulses & adj reward device are "one-shot" signals, these resets 
   SetDO( DD_ADJREWARD );           // aren't strictly necessary. 

   SetDO( 0 );                      // clear all DOUT lines.
}

/**
 Deliver brief pulses on the selected data lines of the "marker pulse device" DD_MARKERS.

 TECH NOTES: In TriggerMarkers() we deliver pulses merely by calling SetDO() to raise the signal on selected DO lines, 
 then calling it again -- without delay! -- to clear all DO lines on device DD_MARKERS. CCxEventTimer realizations are 
 responsible for ensuring that pulses delivered in this manner will be "long enough" to be detected as digital input 
 "events". This is not an issue with the CListechTimer realization, since that device detects 0->1 edge transitions. The 
 CM62Timer realization requires a 2-3us pulse for detectability (b/c event detection is firmware-based, not hardware-
 based), and the device's DSP core ensures that any transient DO signals will be held at least this long.

 @param dwMask For each bit N in this mask, an active-high pulse is delivered on the corresponding data line DO<N> of 
   device DD_MARKERS. Pulses are restricted to channels <11..0> only.
*/
VOID RTFCNDCL CCxEventTimer::TriggerMarkers( DWORD dwMask )
{
   // mask out bits higher than 11. Raise, then lower selected data lines. At least one line must be pulsed, or no
   // action is taken.
   DWORD dw = dwMask &= 0x0FFF; 
   if(dw != 0)
   {
      SetDO(DD_MARKERS | dw); 
      SetDO(DD_MARKERS);
   }
}


/**
 Deliver reward to animal, possibly subject to random withholding.

 The physical reward is delivered by an "adjustable reward pulse" device (DD_ADJREWARD), one of the latched devices
 controlled by the event timer's DO port. The reward pulse duration is specified in bits 11..0 of the DO port.

 If a "variable ratio" N > 1 is specified, then 1 of every N physical rewards is randomly withheld (on average). Allowed 
 range of N is [1..10]. The reward is also withheld if the specified duration is 0 or negative.

 Regardless of whether the physical reward was delivered, we always deliver an "audio" reward by raising the dedicated 
 DO line on device DD_MISC. The signal is left on for the specified duration; we set up a one-shot timer that turns off 
 the pulse when the timer expires. If the duration is 0, the audio reward is disabled.

 After delivering the reward, the method writes a "reward delivered" character code (ASCII 0x05) via the timer's 
 character writer device (DD_WRITER). This code is immediately followed by a null-terminated string reporting the 
 specified adjustable reward pulse duration in ms. 

 NOTE: Callers should ensure that the interval between rewards is longer than the greater of the audio reward duration 
 and the adjustable reward duration. 

 @param iVR Variable ratio; allowed range is [1..10].
 @param iAdjDur Desired duration of the adjustable reward pulse, in ms; allowed range [0..4000]; <=0 is also treated as
 a withheld reward.
 @param iAudioDur Desired duration of "audio" reward pulse, in ms; allowed range [0..1000]; <=0 disables audio reward.

 @param TRUE if reward was delivered; FALSE if it was randomly withheld. Also returns FALSE if iAdjDur<=0.
*/
BOOL RTFCNDCL CCxEventTimer::DeliverReward( int iVR, int iAdjDur, int iAudioDur )
{
   // restrict args to allowed ranges
   iVR = (iVR<1) ? 1 : ((iVR>10) ? 10 : iVR); 
   iAdjDur = (iAdjDur<0) ? 0 : ((iAdjDur>4000) ? 4000 : iAdjDur);
   iAudioDur = (iAudioDur<0) ? 0 : ((iAudioDur>1000) ? 1000 : iAudioDur);

   // withhold reward?
   BOOL bWithheld = (iAdjDur<= 0) || ((iVR>1) && (m_rand.Generate( (WORD) (iVR-1) ) == 0));

   // deliver adj-length reward, unless it is withheld
   if(!bWithheld) SetDO( DD_ADJREWARD | ((DWORD) iAdjDur) );

   // deliver audio reward always, unless zero duration or RTX timer object is unavailable
   if(iAudioDur > 0 && m_hAudioRewTimer != NULL)
   {
      // if audio reward pulse is currently raised, lower it and cancel timer.
      if(m_dwMisc & AUDIOREW_MISC)  
      {
         ::RtCancelTimer( m_hAudioRewTimer, NULL );
         m_dwMisc &= ~AUDIOREW_MISC;
         SetDO( DD_MISC | m_dwMisc );
      }

      // calculate audio reward pulse duration in RTX clock ticks
      LARGE_INTEGER i64Dur; 
      double dDummy = ((double)iAudioDur) * 10026.7366945; 
      i64Dur.QuadPart = (LONGLONG) dDummy; 

      // start one-shot timer; if successful, raise the audio reward pulse
      if(::RtSetTimerRelative(m_hAudioRewTimer, &i64Dur, NULL))
      { 
         m_dwMisc |= AUDIOREW_MISC;
         SetDO( DD_MISC | m_dwMisc );
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


/** Set the DO line on device DD_MISC which reflects "fixation status" signal. */
VOID RTFCNDCL CCxEventTimer::SetFixationStatus()
{
   if(!(m_dwMisc & FIXSTAT_MISC))
   {
      m_dwMisc |= FIXSTAT_MISC;
      SetDO(DD_MISC | m_dwMisc);
   }
}

/** Clear the DO line on device DD_MISC which reflects the "fixation status" signal. */
VOID RTFCNDCL CCxEventTimer::ClearFixationStatus()
{
   if(m_dwMisc & FIXSTAT_MISC)
   {
      m_dwMisc &= ~FIXSTAT_MISC;
      SetDO(DD_MISC | m_dwMisc);
   }
}


/**
 ***PulseSeq: Functions for controlling the electrical pulse stimulus generator module (SGM). The SGM is a custom-built
 Maestro rig hardware  device that is controlled via the timer board's digital output port. Its device address (which 
 distinguishes it from the other latched devices controlled via the timer board DOUT) is DD_SGM = 0x5. For details on 
 how to program the SGM, consult Ken McGary's detailed hardware specification.

 This module internally maintains the current state of the SGM.  We only support the "Sequence Parameter" modes of 
 operation.

 04apr2007-- Spec recommends 1ms between parameter writes when configuring the pulse sequence. This will cause a problem
 when CfgPulseSeq() is used in ContMode. So, for now, I'm only sleeping 100us... Testing indicated that 100us is enough.
 13jun2007-- New spec requires that timing-related parameters be written in the following order (if applicable to the 
 sequence mode): SGM_MODE, SGM_PW1, SGM_PW2, SGM_NP, SGM_NT, SGM_IPI, SGM_ITI. Also, if the "halt" bit in SGM_CONTROL is 
 set, all other bits are ignored -- so we have to write again with the halt bit cleared to set the other control bits to
 their "not running" state...
 13jun2007-- New firmware design requires longer time between parameter writes -- now 5ms instead of 100us. CANNOT USE 
 PSGM IN CONTINUOUS MODE AS A RESULT!
 05sep2013-- The existing PSGM spec we have, last updated 5/26/07, does NOT say that 5ms are required between parameter 
 writes, only 100us. A 5ms delay is required only after issuing a HALT command, in StopPulseSeq(). However, I'm not sure
 about this -- the change noted on 13jun07 may never have gotten into a spec. So I'm leaving things as is. The PSGM
 has never really been used in the Lisberger lab anyway.

 CfgPulseSeq() ==> Resets the SGM and programs (but does not start) a new pulse sequence. If the provided SGM parameters
    are bad, fcn fails.  Note that ALL param values are checked, even though some may not apply to the chosen op mode! 
    Parameters are converted to an ENCODED format, ready for writing to the DD_SGM device.
 StartPulseSeq() ==> Starts a previously programmed pulse sequence. If the op mode is SGM_NOOP, nothing happens! If 
    called while a sequence is running, it will restart that same sequence.
 IsOnPulseSeq() ==> Returns TRUE if a pulse sequence was started. CANNOT determine if seq has finished!!
 StopPulseSeq() ==> Halts any running pulse sequence, whether ext trig or software-start.
 DisablePulseSeq() ==> Applies only to ext-trig pulse sequences. If the pulse sequence has already started, it has no 
    effect. If the sequence has not started, it disables the triggering of the sequence.
 ResetPulseSeq() ==> Stop the SGM and configure internal state to disable the SGM (op mode SGM_NOOP).

 @param pSgm The desired parameters for the SGM sequence, in a NON-ENCODED format.
 @return (where applicable) TRUE if successful, FALSE otherwise.
*/
BOOL RTFCNDCL CCxEventTimer::CfgPulseSeq( PSGMPARMS pSgm )
{
   if( !IsOn() )                                                              // event timer dev not avail
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }
   
   LARGE_INTEGER i64Sleep;                                                    // 5ms sleep period
   i64Sleep.QuadPart = 50000; 

   if( m_bSgmIsRunning )                                                      // stop current sequence, if any 
   {
      StopPulseSeq();
      ::RtSleepFt( &i64Sleep );
   }
   
   if( pSgm->iOpMode < 0 || pSgm->iOpMode >= SGM_NMODES ||                    // validate supplied parameter values 
       pSgm->iAmp1 < -10240 || pSgm->iAmp1 > 10160 || pSgm->iAmp2 < -10240 || pSgm->iAmp2 > 10160 || 
       pSgm->iPW1 < 50 || pSgm->iPW1 > 2500 || pSgm->iPW2 < 50 || pSgm->iPW2 > 2500 || 
       pSgm->iPulseIntv < 1 || pSgm->iPulseIntv > 250 || pSgm->iTrainIntv < 10 || pSgm->iTrainIntv > 2500 || 
       pSgm->nPulses < 1 || pSgm->nPulses > 250 || pSgm->nTrains < 1 || pSgm->nTrains > 250
      )
   {
      SetDeviceError( "Illegal SGM parameter" );
      return( FALSE );
   }

   m_sgm.mode = short(pSgm->iOpMode);                                         // convert SGM params to encoded format 
   m_sgm.bExtTrig = short( pSgm->bExtTrig ? 1 : 0 );
   m_sgm.amp1 = short( pSgm->iAmp1 / 80 + 128 );
   m_sgm.amp2 = short( pSgm->iAmp2 / 80 + 128 );
   m_sgm.pw1 = short( pSgm->iPW1 / 10 );
   m_sgm.pw2 = short( pSgm->iPW2 / 10 );
   m_sgm.tInterpulse = short( pSgm->iPulseIntv );
   m_sgm.tIntertrain = short( pSgm->iTrainIntv / 10 );
   m_sgm.nPulses = short( pSgm->nPulses );
   m_sgm.nTrains = short( pSgm->nTrains );

   if(m_sgm.mode == SGM_NOOP) return( TRUE );                                 // no pulse seq -- nothing to config!

   // we normally leave bits 4(TRIG_OUT follows pulses), 3(TRIG_OUT enable), 2(Output LED follows pulses), and 1
   // (Manual TRIG enable) on all the time -- EXCEPT when programming a sequence. Here we clear those bits.
   SetDO( DD_SGM | SGM_CONTROL ); 
   ::RtSleepFt( &i64Sleep );

   SetDO( DD_SGM | SGM_MODE | ((DWORD) m_sgm.mode) );                         // only write params req'd for op mode... 
   ::RtSleepFt( &i64Sleep );
   SetDO( DD_SGM | SGM_AMP1 | ((DWORD) m_sgm.amp1) );
   ::RtSleepFt( &i64Sleep );
   SetDO( DD_SGM | SGM_PW1 | ((DWORD) m_sgm.pw1) );
   ::RtSleepFt( &i64Sleep );
   
   if(m_sgm.mode == SGM_DUAL || m_sgm.mode == SGM_BIPHASIC || m_sgm.mode == SGM_BIPHASICTRAIN)
   {
      SetDO( DD_SGM | SGM_AMP2 | ((DWORD) m_sgm.amp2) );
      ::RtSleepFt( &i64Sleep );
      SetDO( DD_SGM | SGM_PW2 | ((DWORD) m_sgm.pw2) );
      ::RtSleepFt( &i64Sleep );
   }

   
   if(m_sgm.mode == SGM_TRAIN || m_sgm.mode == SGM_BIPHASICTRAIN)
   {
      SetDO( DD_SGM | SGM_NPPT | ((DWORD) m_sgm.nPulses) );
      ::RtSleepFt( &i64Sleep );
      SetDO( DD_SGM | SGM_NT | ((DWORD) m_sgm.nTrains) );
      ::RtSleepFt( &i64Sleep );
   }

   if(m_sgm.mode == SGM_DUAL || m_sgm.mode == SGM_TRAIN || m_sgm.mode == SGM_BIPHASICTRAIN)
   {
      SetDO( DD_SGM | SGM_IPI | ((DWORD) m_sgm.tInterpulse) );
      ::RtSleepFt( &i64Sleep );
      if(m_sgm.mode != SGM_DUAL)
      {
         SetDO( DD_SGM | SGM_ITI | ((DWORD) m_sgm.tIntertrain) );
         ::RtSleepFt( &i64Sleep );
      }
   }

   // set bits 4-1 in the control register now that we're done programming the sequence.
   SetDO( DD_SGM | SGM_CONTROL | SGM_EXTOFF ); 
   ::RtSleepFt( &i64Sleep );

   return( TRUE );
}

BOOL RTFCNDCL CCxEventTimer::StartPulseSeq()
{
   if( !IsOn() )                                                              // event timer dev not avail
   {
      SetDeviceError( CDevice::EMSG_DEVNOTAVAIL );
      return( FALSE );
   }
   if( m_bSgmIsRunning ) StopPulseSeq();                                      // stop current sequence, if any 
   if( m_sgm.mode != SGM_NOOP )                                               // start the currently cfg'd seq
   {
      DWORD dwCmd = DD_SGM | SGM_CONTROL;
      dwCmd |= (m_sgm.bExtTrig ? SGM_EXTON : SGM_START);                      // ext trig or immed s/w start!
      SetDO( dwCmd );
      m_bSgmIsRunning = TRUE;
   }
   return( TRUE );
}

BOOL RTFCNDCL CCxEventTimer::IsOnPulseSeq() { return( m_bSgmIsRunning ); }

VOID RTFCNDCL CCxEventTimer::StopPulseSeq()
{
   if( m_bSgmIsRunning )
   {
      SetDO( DD_SGM | SGM_CONTROL | SGM_STOP );
      m_bSgmIsRunning = FALSE;
      LARGE_INTEGER i64Sleep;                                                 // 5ms sleep between writes
      i64Sleep.QuadPart = 50000; 
      ::RtSleepFt( &i64Sleep );
      SetDO( DD_SGM | SGM_CONTROL | SGM_EXTOFF );
   }
}

VOID RTFCNDCL CCxEventTimer::DisablePulseSeq()
{
   if( m_bSgmIsRunning && m_sgm.bExtTrig ) SetDO( DD_SGM | SGM_CONTROL | SGM_EXTOFF );
}

VOID RTFCNDCL CCxEventTimer::ResetPulseSeq()
{
   StopPulseSeq();
   m_sgm.mode = SGM_NOOP;                                                     // pulse sequencer not in use
   m_sgm.bExtTrig = 0;                                                        // s/w start selected
   m_sgm.amp1 = m_sgm.amp2 = 128;                                             // 0.0V
   m_sgm.pw1 = m_sgm.pw2 = 5;                                                 // 50 microsecs
   m_sgm.tInterpulse = m_sgm.tIntertrain = 1;                                 // 1 ms for ipi, 10ms for iti
   m_sgm.nPulses = m_sgm.nTrains = 1;
}


/**
 Transmit a single 8-bit ASCII character to the "character writer" latched device (DD_WRITER).
 @param c The ASCII character to be written.
*/
VOID RTFCNDCL CCxEventTimer::WriteChar(char c) { SetDO(DD_WRITER | (0x000000FF & ((DWORD) c))); }

/**
 Transmit a null-terminated ASCII string to the "character writer" latched device. Expect the execution time for this
 method to increase with string length; use with care in time-critical code sections.
 @param str An ASCII chracter string to be written, terminated by a null (if it isn't already).
 @param len Length of the character string.
*/
VOID RTFCNDCL CCxEventTimer::WriteString(char* str, int len)
{
   for( int i = 0; i < len; i++ )
      SetDO( DD_WRITER | (0x000000FF & ((DWORD) str[i])) );
   if( (len > 0) && (str[len-1] != 0) )
      SetDO( DD_WRITER );
}
