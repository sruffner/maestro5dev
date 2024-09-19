/**==================================================================================================================== 
 cxanalogin.cpp : Abstract interface CCxAnalogIn, defining MaestroRTSS analog input (AI) hardware device interface.

 AUTHOR:  saruffner

 DESCRIPTION:
 MaestroRTSS uses an analog input device to scan 16 analog signals during runtime, at 1KHz in Trial Mode and 500Hz in 
 Continuous Mode. In addition, this device may be used to simultaneously sample one AI channel at 25KHz -- to record a
 high-resolution version of the spike waveform. Finally, the "per-scan" interrupts from the AI device serve as "clock 
 ticks", establishing the timeline in all MaestroRTSS runtime operational modes. Promptly servicing these interrupts is
 crucial to verifying that the runtime engine thread is "keeping up" with the ongoing data acquisition timeline.

 CCxAnalogIn is an ABSTRACT "interface" class that attempts to expose the AI device's functionality -- at least that 
 functionality required by MaestroRTSS -- in a device-independent manner. To satisfy this interface, a candidate AI
 device must meet these minimum functional requirements:

    1) AI device is hosted on PCI or PCI-Express bus. The reason: we must enable and handle an interrupt from the AI
 device,and base class CDevice only supports attaching an interrupt service routine (ISR) to PCI-based devices.
    2) # of channels, ADC resolution. MaestroRTSS was originally developed using a 16-ch, 12-bit PCI board, the 
 PCI-MIO-16E1 from National Instruments (see CPciMio16E1). However, to make room for future enhancements, this interface
 supports up to 32 channels and resolutions of 12 or 16 bits. Methods are provided to report both # of channels and the 
 ADC resolution.
    3) Bipolar mode, range +/- 10V, NSRE. Thus, at 12bit resolution: [-2048..2047] (binary 2s-complement, or b2s, 
 encoding) maps to the voltages [-10.0 .. 9.99512]. At 16bit: [-32768..32767] maps to [-10 .. 9.99969] volts. Interface 
 provides routines to retrieve voltage samples either in volts or as a b2s number -- note that b2s is a common encoding 
 used by ADCs. The key point is that the interface supports acquiring analog voltage signals in the range [-10..+10V]. 
 While most AI devices support unipolar mode and other voltage ranges, this interface ASSUMES bipolar mode, +/- 10V is 
 ALWAYS in effect. Also assumes the input grounding configuration is "non-referenced single-ended" (NSRE); this is 
 required by the external signal conditioners that are connected to the AI channels in Maestro's equipment rig.
    4) AI data FIFO size. The bigger the better. The FIFO size is exposed via GetFIFOSize().
    5) DAQ operations supported: CCxAnalogIn exposes only a very few of the DAQ capabilities of a typical AI device. 
 All CCxAnalogIn DAQ operations involve scanning a channel set indefinitely ("continuous acquisition"). All operations 
 are software-started and software-polled, but a "start-of-scan" interrupt is issued to establish the timeline for the 
 MaestroRTSS runtime loops. Here we discuss the relevant CCxAnalogIn methods and how they are used to control a DAQ 
 operation:

    Init() -- This method initializes the AI functionality of the device. It should leave the device in the following 
    state:  no DAQ running; data FIFO cleared; device interrupts disabled. Typically, it is called prior to configuring 
    a new DAQ operation, but it should also be safe to call the method to abort an ongoing DAQ. Note that Init() is NOT 
    necessarily the equivalent of a board reset, as it should NOT affect other subsystems on a "multifunction" device 
    (such as the PCI-MIO-16E1). Init() is a pure virtual method inherited from base class CDevice.

    Configure(int nCh, int nScanIntv, BOOL bInt, int iChFast) -- This method sets up, but does NOT start, a DAQ 
    operation. Essentially, this should set up the device to scan AI channels <0..nCh-1> using the specified scan 
    interval (in microsecs, guaranteed >=1000us). If 'iChFast' is a valid channel#, then the DAQ operation must ALSO be 
    configured to sample that channel (which may be included in the slow scan set as well) at 25KHz. The AI device must 
    therefore support multirate sampling. How multirate sampling is actually achieved will be device-dependent. However,
    it is imperative that (these assumptions are  fundamental in analyses of data collected by Maestro!!):

       a) The "fast data" channel always be sampled precisely on 40us (1/25KHz) epochs; and 
       b) The channels in the "slow scan set" should be sampled as "simultaneously as possible" at each 'nScanIntv' 
       epoch; in other words, the interval between samples of successive channels within a scan should be << the scan 
       interval (we're assuming here that the AI device contains only a single ADC amplifier that's multiplexed across 
       16 or more channels -- as in the PCI-MIO-16E1). MaestroRTSS requires the slow scan set data to be ready for 
       unloading within the first 100us of each scan interval.

    The DAQ is configured to continue indefinitely, until Stop() or Init() is called. The interface does not expose the 
    ability to stop after a finite # of scans are collected (sometimes called "posttriggered acquisition"), nor for 
    introducing a delay between the "start" signal and the start of the first scan ("pretriggered acquisition"). The 
    operation is always "software-started" -- use of an external "start" trigger is not exposed by the interface. The 
    DAQ is always "software-polled" as well -- any direct memory access (DMA) capabilities of an AI device are not 
    required by the interface. Finally, if bInt is TRUE, a "start-of-scan" interrupt fires at the start of each channel 
    set scan. The interval between successive interrupts should be (ignoring interrupt latencies) 'nScanIntv' microsecs. 
    Note that'nCh' must be > 0; ie, we NEVER do 25KHz sampling without sampling a slow scan set. When Configure() 
    returns, the device should be "armed" and ready to start upon invoking Start().

            !!! IMPORTANT !!!
            MaestroRTSS currently uses the AI device interrupt to update critical runtime variables. The interrupt 
            essentially establishes the timeline for MaestroRTSS experimental protocols. It is therefore quite 
            important that the interrupts and the DAQ timeline work as described here.
            !!! IMPORTANT !!!

    Start() -- Call this method to initiate a previously configured DAQ operation. By design, Start() should be called 
    after Configure() without invoking other CCxAnalogIn methods. Also, Start() should complete VERY QUICKLY (ie, a few 
    microsecs!!). The purpose:  to permit MaestroRTSS to configure other devices that must run in tandem with the AI
    device AFTER configuring the DAQ but BEFORE starting it.

    Unload(short* pSlow, int& nSlow, short* pFast, int& nFast, BOOL bWait) -- This interface method unloads samples from
    the AI data FIFO during or after a DAQ operation. It attempts to retrieve up to 'nSlow' samples from the "slow scan"
    data stream and however many samples, up to 'nFast', are pulled from the "fast channel" data stream in the process. 
    If the 'bWait' flag is set, the routine BLOCKS until 'nSlow' samples are retrieved from the slow data stream (or an 
    error occurs). Upon returning, Unload() must set 'nSlow' and 'nFast' to the actual # of samples actually taken from 
    each data stream.

    Each particular implementation, of course, handles the details of separating the "slow" and "fast" data streams. 
    Callers, however, should ensure the fast data stream buffer is appropriately sized. For example, if Unload() is 
    invoked every N slow scan intervals of duration D microsecs, then the # of samples available from the fast data 
    stream on each call will be between N*D/40 and (N+1)*D/40. For safety's sake, the fast data stream buffer should be 
    at least (N+1)*D/40 samples long.

    Suppose the 'bWait' flag is set and a fatal device error causes the DAQ operation to spontaneously freeze. In that 
    case, Unload() could block forever. To avoid this deadlock situation, implementations are responsible for giving up 
    after a timeout period of ((nSlow/N) + 1) * D microsecs, where N is the # of channels in the slow scan set and D is 
    the slow scan interval in microsecs.

    Implementations must detect a number of different errors that may occur during a DAQ operation: the timeout error, 
    a FIFO overflow (FIFO fills up b/c it is not unloaded fast enough), a FIFO overrun (when sample interval is too 
    short), or a fast data buffer error (fast data lost b/c caller-provided buffer was not large enough). If an error 
    occurs (or has already occurred), Unload() will abort and return FALSE. An appropriate error message should be 
    provided via Set/GetDeviceError(), which is inherited from base class CDevice. Once an error occurs, callers must 
    invoke Configure() again before starting a new DAQ operation.

    Stop() -- Stop an ongoing DAQ operation.  The DAQ op is merely halted **without** clearing the FIFO. 

    IsEmpty() -- To check whether or not there are any samples to be retrieved from the data FIFO.

    IntAck() -- If an interrupt condition is enabled, the installed interrupt handler should use this method to 
    identify, clear, and acknowledge the interrupt that occurred (if any -- if the AI device is sharing its IRQ line 
    with other devices, it is possible that another device was responsible for a given int).

    6) Quick internal calibration [OPTIONAL]. If an AI device becomes uncalibrated when the host is power-cycled, there
    should be a means of quickly returning it to a calibrated state (in the case of the PCI-MIO16E1, this involves
    loading calibration constants from the board's EEPROM). This internal calibration should not require external 
    measuring equipment and must NOT be affected by any connections to the device.

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
 12jul2001-25jul2001-- Adapted from C source code module ADCBOARD from an older version of CXDRIVER.
 19oct2001-- CCxAnalogIn now derived from CDevice, an abstract interface representing any PCI- or ISA-based hardware 
    device and providing some basic services (e.g., searching the PCI bus).
 28mar2002-- Begun major revision to weave spikesPC functionality into the CCxAnalogIn interface. Implementations must
    support simultaneous scanning of up to 16 channels at 1KHz or less, PLUS 25KHz sampling of another channel (perhaps 
    one in the slow scan set) at the same time.
 14jun2002-- MAJOR structural revision:  Each practical implementation of CCxAnalogin is now placed in a separate file. 
    Also, we'll use a "device manager" class to instantiate devices, replacing CCxAnalogIn::AttachToDevice(). Finally, 
    we revised interface IAW changes in base class CDevice.
 24jun2011-- Removed Get/Set/CfgDIOPort(). Their only use was to raise DO channel 7 when MaestroRTSS and lower it upon
    exiting -- that TTL signal served to turn on/off the fiber optic source in the experiment rig. Since the fiber optic
    targets are no longer supported in Maestro 3, this signal is no longer needed.
 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build.
======================================================================================================================*/

#include "cxanalogin.h"


//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 

LPCTSTR CCxAnalogIn::EMSG_DAQ_OVERRUN        = "DAQ overrun error (sample intv too short)";
LPCTSTR CCxAnalogIn::EMSG_DAQ_OVERFLOW       = "DAQ FIFO overflow";
LPCTSTR CCxAnalogIn::EMSG_DAQ_TIMEOUT        = "Device timeout during DAQ; AI board may have stopped working";
LPCTSTR CCxAnalogIn::EMSG_DAQ_LOSTFASTDATA   = "Lost fast data -- buffer too small";
LPCTSTR CCxAnalogIn::EMSG_DAQ_OTHER          = "Unknown DAQ error(s) on AI device";



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxAnalogIn, ~CCxAnalogIn [constructor/destructor] ============================================================== 
//
//    Sets default device attributes.  Derived class constructor should set these attributes IAW the actual AI device's  
//    capabilities.  Here we require that the device be hosted on the PCI bus and restrict the # of channels supported.
//    If any restriction is violated, the # of channels is forced to ZERO, rendering the device interface useless!
//
//    ARGS:       devInfo  -- [in] see CDevice.
//                iDevNum  -- [in] see CDevice.
//                bIs16Bit -- [in] TRUE if device resolution is 16-bit; else, 12-bit assumed!
//                nCh      -- [in] # of channels available.  if not valid, it is treated as ZERO!!
//
CCxAnalogIn::CCxAnalogIn( const CDevice::DevInfo& devInfo, int iDevNum, BOOL bIs16Bit, int nCh ) 
   : CDevice( devInfo, iDevNum )
{
   m_nCh = ( ((devInfo.flags & DF_PCI) != DF_PCI) || nCh < MIN_CHANNELS || nCh > MAX_CHANNELS ) ? 0 : nCh;
   m_bIs16Bit = bIs16Bit;
   m_iMaxB2s = (bIs16Bit) ? 32767 : 2047;
   m_iMinB2s = (bIs16Bit) ? -32768 : -2048;
   m_dB2sToVolt = (bIs16Bit) ? 3.05176e-4 : 4.882813e-3;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== ToVolts, ToRaw, NearestVolts ==================================================================================== 
//
//    Convert between two different representations of analog voltage: the "real" voltage in floating-pt volts or its 
//    closest binary 2s-complement (b2s) encoded representation.  This is the typical representation in an analog-to- 
//    digital (ADC) converter -- hence the term "raw" --, although other encodings are also used. 
//
//    All values are restricted to voltages that can be delivered by the device ASSUMING it is configured for a bipolar 
//    range of +/-10V and a finite resolution of 12- or 16-bits -- this is the configuration expected by CNTRLX.  The 
//    calculations also assume the board is perfectly calibrated which, of course, may not be the case in reality!
//
//    NearestVolts() takes any floating-pt voltage and converts it to the nearest reproducible one given the limited 
//    resolution of the device. 
//
//    ARGS:       b2sVolt  -- [in] the raw binary 2s-complement value.
//                fVolt    -- [in] corresponding real voltage, in volts.
//
//    RETURNS:    voltage in alternate units, converted and range-limited. 
//
float RTFCNDCL CCxAnalogIn::ToVolts( int b2sVolt )
{
   return( (float) (((double)CheckRange( b2sVolt )) * m_dB2sToVolt) );
}

int RTFCNDCL CCxAnalogIn::ToRaw( float fVolt )
{
   int b2sVolt = (int) ( ((double)fVolt) / m_dB2sToVolt + 0.5 );
   return( CheckRange( b2sVolt ) );
}

float RTFCNDCL CCxAnalogIn::NearestVolts( float fVolt )
{
   int b2sVolt = ToRaw( fVolt );
   return( (float) (((double)b2sVolt) * m_dB2sToVolt) );
}


