/**=====================================================================================================================
 cxanalogout.cpp : Abstract interface CCxAnalogOut, which defines the analog output (AO) hardware device interface
                   required by MaestroRTSS.

 AUTHOR:  saruffner

 DESCRIPTION:
 MaestroRTSS once used an analog output device to control several different stimulus platforms: mirror galvanometers
 governing the position of one or two shuttered fiber optic spots on a translucent screen (FIBER1, FIBER2); the servo
 controlling the rotating turntable on which the subject sits (CHAIR); and a separate servo controlling the rotational 
 position of a striped drum placed over the subject's head (OKNDRUM). Support for OKNDRUM was removed as of Maestro 
 1.5.0 (Mar 2006), and support for the fiber optic targets ended with Maestro 3.0.0 (late 2011). Now only the CHAIR 
 target is supported; it is the only non-visual stimulus target supported in Maestro.

 During runtime, MaestroRTSS must be able to update the AO channel driving the CHAIR servo in "immediate mode" (meaning,
 the new voltage is driven on the output as soon as the DAC register is loaded with a new value). The channel may be 
 updated as rapidly as once per millisec (1KHz).

 CCxAnalogOut is an ABSTRACT "interface" class that attempts to expose the AO device's functionality in a device-
 independent manner. To satisfy this interface, a candidate AO device must meet these functional requirements:

    1) # of channels, DAC resolution. MaestroRTSS only needs a single dedicated AO channel, either 12- or 16-bit 
 resolution. The interface implementation must specify which channel number is dedicated as the chair velocity drive 
 signal.
    2) Bipolar mode, range +/- 10V. The interface overloads the Out() routine so that callers can express the desired 
 voltage either in volts or in a binary 2s-complement (b2s) encoded form, which is often the format for the digital word
 that's written to the DAC (there are other encodings! E.g., the PD2-AO8/16 uses a straight binary encoding). At 12bit 
 resolution: [-2048..2047] b2s maps to [-10.0 .. 9.99512] volts. At 16bit resolution: [-32768..32767] b2s maps to 
 [-10 .. 9.99969] volts (assuming a linear mapping; the DAC may not be linear, however!). The key point is that the 
 interface supports writing analog output voltages in the range [-10..+10]. While most AO devices support unipolar mode 
 and other ranges, this interface assumes that bipolar mode, +/- 10V is ALWAYS in effect.
    3) Analog outputs can be updated independently in "immediate-mode". The interface does not require simultaneous
 update of all channels, nor does it require the device possess a FIFO to enable continuous hardware-timed updating of
 AO channels. MaestroRTSS's usage of the AO device is very straightforward!
    4) Board reset. Device should provide a software-controlled means of resetting itself and putting it into the
 following "idle" state:  all AO channels at 0.0volts; configured for immediate-mode updates. If the device is a
 multi-function IO board -- with AI and DIO subsystems as well as AO --, the reset should ONLY affect the AO subsystem.
 If there are any available h/w interrupts associated with the AO subsystem, these should be disabled by the board reset
 and should ALWAYS be disabled.

 ==> Abstraction of targets controlled by CCxAnalogOut.
 In MaestroRTSS, the analog output device provides a control signal for a single non-visual "target":

    CX_CHAIR:  The animal chair is a servo-controlled turntable that is ultimately driven by a "velocity command"
    signal from the AO device. The chair has a LOT of inertia and cannot do "instantaneous" position changes. The chair
    position will also drift slowly even if the velocity is set to 0. The chair velocity drive signal must ultimately be 
    converted from units of visual degrees/sec to "b2sAOvolts", ie, the binary encoded form that is written to the DAC 
    of the AO device.

 CCxAnalogOut encapsulates this MaestroRTSS-specific usage of the analog output device.  To that end, the following
 higher-level constructs are built upon the low-level Out() and Init() methods:

    UpdateChair( float driveVel, float currPos, float expPos ) ==>  This method delivers a velocity command signal on a
       dedicated AO channel to control the motion of an animal chair -- target type CX_CHAIR -- via an external 
       servo-feedback system.  It handles the details of converting velocity in deg/sec to b2sAOVolts.  It also makes
       slight compensatory adjustments in the drive velocity when the chair's current position (as monitored by 
       MaestroRTSS's AI device) is too far from its expected position (both in visual degrees).

    InitChair() ==> Reset chair drive velocity signal to 0.

    SettleChair() ==> This method drives the chair back to zero position, applying a relatively large drive signal that
       was selected by trial-and-error during test and development. It is used to quickly but smoothly zero the chair
       after a trial or stimulus run ends. See method description for details.

 These methods provide a higher-level of abstraction. All are designed to be called on a regular basis -- as in the 1ms
 and 2ms update intervals associated with MaestroRTSS's Trial and Continuous Modes.  Users of CCxAnalogOut need only 
 deal with chair position and velocity in degrees and deg/sec. The interface handles the necessary conversions.

 CAVEAT: Users of CCxAnalogOut can choose to ignore these higher-level methods entirely and use the low-level Out()
 method directly. Do NOT, however, mix calls to these methods with calls to Out(), or unexpected behavior will
 result. It is safe, however, to call Init() to restore all control signals to zero (zero deg = zero b2sAOVolts).

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
 18may2001-- Adapted from C source code module DACBOARD from an older version of CXDRIVER.
 28jun2001-- Added placeholder implementation CCxNullAO.
 11oct2001-- Begun code development of class CUeiPD2AO8, which will implement CCxAnalogOut for the PD2-AO-8/16.
 12oct2001-- Providing default implementations of selected CCxAnalogOut overridables such as Is16Bit(), ToRaw(), etc.
          -- CCxAnalogOut now based on CDevice, which can represent either an ISA- or PCI-based device.  CDevice
    provides support for finding a particular device on the PCI bus.
 18oct2001-- Completed initial code development for CUeiPD2AO8.
 19oct2001-- Modified IAW latest changes to base class CDevice. Basic operations Open(), Close(), Reset() and Init() are
    now declared as virtual or pure virtual methods of CDevice. Generic error codes for Open() also defined in CDevice.
 14jun2002-- MAJOR structural revision:  Each practical implementation of CCxAnalogOut (CNatlInstATAO10, CUeiPD2AO8)
    is now placed in a separate file. Also, we'll use a "device manager" class to instantiate devices, replacing the 
    static method CCxAnalogOut::AttachToDevice(). Finally, we revised interface IAW changes in base class CDevice.
 04aug2002-- Began major revision to add higher level of abstraction to CCxAnalogOut:  methods UpdateFiber1,2(),
    UpdateChair(), RestoreChair(), and UpdateOKNPos(). The details of converting pos in deg and vel in deg/sec, as well
    as AO channel assignments, is handled by CCxAnalogOut. UpdateFiber1,2() will also handle the details of computing 
    "coarse" and "fine" control signals when the AO device is 12bit.
 14mar2006-- As of Maestro v1.5.0, the OKNDRUM "target" is no longer supported. Removed support for monitoring OKN
    position via a dedicated AO channel.
 20jun2011-- Began major revisions for Maestro 3: Removed support for the fiber optic targets -- now CCxAnalogOut is 
    only needed to drive the CX_CHAIR target. The interface implementation specifies which AO channel is dedicated as 
    the chair velocity drive signal. This allows us to continue using one of the old AO boards, although we're unlikely 
    to do so.
 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.
========================================================================================================================
*/

#include "cxanalogout.h"


// these factors convert chair velocity in deg/sec to the corres. 12- or 16-bit DAC code. MaestroRTSS-specific -- 
// reflecting the chair servo's voltage-velocity relation??
const double CCxAnalogOut::CHAIRVEL_TOAORAW12   = 4.7961631;
const double CCxAnalogOut::CHAIRVEL_TOAORAW16   = 16.0 * CCxAnalogOut::CHAIRVEL_TOAORAW12;


/**
 Construct the analog output device.
 @param devInfo Device & vendor ID, etc. See CDevice.
 @param iDevNum Device instance number, usually 1. See CDevice.
 @param bIs16Bit True for 16-bit device; else, 12-bit is assumed.
 @param nCh #AO channels available. Must be in [1..16], else set to zero -- rendering device useless!
 @param iChairCh Zero-based index of AO channel dedicated to chair velocity signal. Must be in [0..(nCh-1)].
*/
CCxAnalogOut::CCxAnalogOut(const CDevice::DevInfo& devInfo, int iDevNum, BOOL bIs16Bit, int nCh, int iChairCh)
   : CDevice(devInfo, iDevNum)
{
   m_nCh = ( nCh < 0 || nCh > MAX_CHANNELS ) ? 0 : nCh;
   m_iChairCh = (iChairCh < 0 || iChairCh >= m_nCh) ? -1 : iChairCh;
   m_bIs16Bit = bIs16Bit;
   m_iMaxB2s = (bIs16Bit) ? 32767 : 2047;
   m_iMinB2s = (bIs16Bit) ? -32768 : -2048;
   m_dB2sToVolt = (bIs16Bit) ? 3.05176e-4 : 4.882813e-3;
}

/**
 Update the "velocity command" voltage driving the servo system that controls the motion of the animal chair. If the
 chair's current position deviates significantly from its expected position (due to chair drift), adjust the command 
 voltage to compensate.

 This method is intended for use while the chair is being driven along a particular trajectory. The drift compensation 
 adjustment is very slight to minimize distortions of the intended trajectory.

 @param fDriveVel Desired chair velocity in deg/sec.
 @param fCurrPos Current chair position in deg.
 @param fExpPos Expected chair position in deg.
 @return TRUE if successful; FALSE otherwise.
*/
BOOL RTFCNDCL CCxAnalogOut::UpdateChair(float fDriveVel, float fCurrPos, float fExpPos)
{
   // compute drift compensation for 12-bit AO in b2sAOVolts : +/- 1LSB or 0!
   float fDiff = fCurrPos - fExpPos; 
   int iDriftComp = cMath::signof( fDiff ) * ((cMath::abs(fDiff) > 0.05f) ? 1 : 0); 

   // compute new velocity drive signal for 12- or 16-bit AO
   int iVelCmd; 
   if(m_bIs16Bit) iVelCmd = ((int) (fDriveVel * CHAIRVEL_TOAORAW16)) + iDriftComp * 16;
   else iVelCmd = ((int) (fDriveVel * CHAIRVEL_TOAORAW12)) + iDriftComp;

   return( Out(m_iChairCh, iVelCmd) );
}


/**
 Apply a velocity-drive adjustment to force the chair toward zero position (centered).

 The animal chair in the Maestro experiement rig is driven by a velocity-controlled servo system. The velocity drive 
 signal comes directly from a dedicated channel on the analog output device. There are several situations in MaestroRTSS
 where it is necessary to smoothly drive the chair back to its zero position and keep it there (even with a zero 
 velocity command, the chair may slowly drift). We do so by calling this method once per "scan" in the context of an 
 ongoing DAQ. If there is no chair in the experimental setup, calling the method should have no effect -- as long as the
 AO channel that controls chair velocity is not connected.

 DEVNOTES: The algorithm encapsulated here was developed by trial and error, with 1 or 2ms between updates. It has also
 worked fine when the update interval is longer: 20ms in IdleMode, and 10ms during the "intertrial" period in TrialMode.
 We adjust the chair pos every 20ms in IdleMode, and every 10ms during the "intertrial" state in

 @param fCurrPos Current chair pos in degrees.
 @return TRUE if successful; FALSE otherwise.
*/
BOOL RTFCNDCL CCxAnalogOut::SettleChair(float fCurrPos)
{
   // compue appropriate 12-bit velocity command. This algorithm was developed by trial and error!
   int iChairVelAdj; 
   float fAbsPos = cMath::abs( fCurrPos );
   if     ( fAbsPos > 25.0f )    iChairVelAdj = 400; 
   else if( fAbsPos > 12.5f )    iChairVelAdj = 200;
   else if( fAbsPos > 2.5f )     iChairVelAdj = 100;
   else if( fAbsPos > 1.25f )    iChairVelAdj = 50;
   else if( fAbsPos > 0.25f )    iChairVelAdj = 10;
   else if( fAbsPos > 0.125f )   iChairVelAdj = 2;
   else if( fAbsPos > 0.05f )    iChairVelAdj = 1;
   else                          iChairVelAdj = 0;

   if( fCurrPos < -0.05f ) iChairVelAdj = -iChairVelAdj;

   // scale velocity command by 2^4 if device is 16-bit
   if(m_bIs16Bit) iChairVelAdj *= 16;

   return( Out(m_iChairCh, iChairVelAdj) );
}


/**
 Restore the chair velocity command to 0 deg/sec. This is achieved by setting all AO channels to zero volts. Only the
 channel dedicated to the chair velocity drive signal should be connected, anyway.
 
 @return TRUE if successful; FALSE otherwise.
*/
BOOL RTFCNDCL CCxAnalogOut::InitChair() { return( Out(-1, 0) ); }


/**
 ToVolts, ToRaw, NearestVolts

 These methods convert between two different representations of AO voltage: the "real" voltage in floating-pt volts or 
 its closest binary 2s-complement (b2s) encoded representation. Many AO devices write the b2s-encoded value directly
 to the DACs -- hence the term "raw" --, although other encodings are also used.

 NearestVolts() takes any floating-pt voltage and converts it to the nearest reproducible one given the limited 
 resolution and range of the device.
 
 All values are restricted to voltages that can be delivered by the device ASSUMING it is configured for a bipolar range
 of +/-10V and a finite resolution of 12- or 16-bits -- this is the configuration expected by MaestroRTSS. The methods
 assume the relationship between DAC code and output voltage is linear over the entire range; if this is not the case,
 implementing classes can override these methods. Finally, if the board is not well-calibrated, these methods will not
 reflect reality!

 @param b2sVolt AO DAC code, a binary 2s-complement value.
 @param fVolt Corresponding real voltage, in volts.
 @param The voltage in alternate units, converted and range-limited.
*/
float RTFCNDCL CCxAnalogOut::ToVolts( int b2sVolt )
{
   return( (float) (((double)CheckRange( b2sVolt )) * m_dB2sToVolt) );
}

int RTFCNDCL CCxAnalogOut::ToRaw( float fVolt )
{
   int b2sVolt = (int) ( ((double)fVolt) / m_dB2sToVolt + 0.5 );
   return( CheckRange( b2sVolt ) );
}

float RTFCNDCL CCxAnalogOut::NearestVolts( float fVolt )
{
   int b2sVolt = ToRaw( fVolt );
   return( (float) (((double)b2sVolt) * m_dB2sToVolt) );
}



