/**===================================================================================================================== 
 cxanalogin.h : Declaration of ABSTRACT hardware interface CCxAnalogIn, and placeholder implementation CCxNullAI.
======================================================================================================================*/

#if !defined(CXANALOGIN_H__INCLUDED_)
#define CXANALOGIN_H__INCLUDED_

#include "device.h"              // CDevice -- base class for MaestroRTSS device interfaces  

class CCxAnalogIn : public CDevice 
{
public:
   // min/max # of AI channels required/supported by this interface
   static const int MIN_CHANNELS = 16;
   static const int MAX_CHANNELS = 32;

protected:
   // AI-related device error messages
   static LPCTSTR EMSG_DAQ_OVERRUN;                      // DAQ overrun (sample interval is too short)
   static LPCTSTR EMSG_DAQ_OVERFLOW;                     // DAQ FIFO overflowed
   static LPCTSTR EMSG_DAQ_TIMEOUT;                      // timeout in Unload(); DAQ may have stopped functioning
   static LPCTSTR EMSG_DAQ_LOSTFASTDATA;                 // lost some "fast data" samples (fast data buf too small)
   static LPCTSTR EMSG_DAQ_OTHER;                        // catch-all: some other DAQ error or combination of errors

private:
   BOOL     m_bIs16Bit;                                  // supports 12-bit or 16-bit resolution on the input ADCs
   int      m_nCh;                                       // # of channels available on device

   int      m_iMaxB2s;                                   // max,min b2s-encoded voltages (bipolar mode, +/-10V range 
   int      m_iMinB2s;                                   // is assumed)
   double   m_dB2sToVolt;                                // convert b2s-encoded form to real voltage

   // prevent compiler from automatically providing default copy constructor and assignment operator
   CCxAnalogIn(const CCxAnalogIn& src); 
   CCxAnalogIn& operator=(const CCxAnalogIn& src); 

public: 
   // constructor/destructor
   CCxAnalogIn(const CDevice::DevInfo& devInfo, int iDevNum, BOOL bIs16Bit, int nCh);
   virtual ~CCxAnalogIn() {}

   // some capabilities: #channels, 16- or 12-bit, supports quick calibration, #samples in onboard data FIFO
   int RTFCNDCL GetNumChannels() {  return(IsOn() ? m_nCh : 0); }
   BOOL RTFCNDCL Is16Bit() { return(m_bIs16Bit); } 
   virtual BOOL RTFCNDCL CanCalibrate()  { return(FALSE); } 
   virtual int RTFCNDCL GetFIFOSize() = 0;

   // configure/start/stop a DAQ sequence
   virtual BOOL RTFCNDCL Configure(int nCh, int nScanIntv, int iChFast, BOOL bInt) = 0;
   virtual VOID RTFCNDCL Start() = 0; 
   virtual VOID RTFCNDCL Stop() = 0;

   // unload acquired "slow scan" and "fast channel" samples (raw b2s ADC codes) from AI device FIFO
   virtual BOOL RTFCNDCL Unload(short *pSlow,int& nSlow, short *pFast, int& nFast, BOOL bWait) = 0;
   // is AI FIFO empty?
   virtual BOOL RTFCNDCL IsEmpty() = 0;
   // check for and acknowledge "start-of-scan" interrupt from AI device
   virtual BOOL RTFCNDCL IntAck() = 0;

   // put AI circuitry in a calibrated state (if quick loading of calibration parameters is supported)
   virtual BOOL RTFCNDCL Calibrate() { SetDeviceError(EMSG_NOTSUPPORTED); return(FALSE); }

   // restrict raw b2s-encoded ADC code to allowable range
   int RTFCNDCL CheckRange(const int b2sVolt)
   {
      int i = b2sVolt;
      if( i < m_iMinB2s ) i = m_iMinB2s;
      else if( i > m_iMaxB2s ) i = m_iMaxB2s;
      return( i );
   }
   
   // convert ADC code to actual volts and back again
   virtual float RTFCNDCL ToVolts(int b2sVolt); 
   virtual int RTFCNDCL ToRaw( float fVolt );  
   virtual float RTFCNDCL NearestVolts(float fVolt); 
};


/** CCxNullAI: "No device" placeholder implementation of CCxAnalogIn. */
class CCxNullAI : public CCxAnalogIn
{
private: 
   CCxNullAI(const CCxNullAI& src); 
   CCxNullAI& operator=(const CCxNullAI& src); 

public: 
   CCxNullAI(const CDevice::DevInfo& devInfo, int iDevNum) : CCxAnalogIn( devInfo, iDevNum, FALSE, 0 ) {}
   ~CCxNullAI() {} 

   int RTFCNDCL GetFIFOSize() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(0); } 
   BOOL RTFCNDCL Init() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return( FALSE ); }
   BOOL RTFCNDCL Configure(int nCh, int nScanIntv, int iChFast, BOOL bInt) 
   { 
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); 
      return(FALSE);
   }
   VOID RTFCNDCL Start() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); }
   VOID RTFCNDCL Stop() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); }
   BOOL RTFCNDCL Unload(short *pSlow, int& nSlow, short *pFast, int& nFast, BOOL bWait) 
   { 
      nSlow = 0; nFast = 0; 
      SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); 
      return(FALSE); 
   }
   BOOL RTFCNDCL IsEmpty() { return(TRUE); }
   BOOL RTFCNDCL IntAck() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

protected:
   BOOL RTFCNDCL MapDeviceResources() { return(FALSE); }
   VOID RTFCNDCL UnmapDeviceResources() {}
};


#endif   // !defined(CXANALOGIN_H__INCLUDED_)

