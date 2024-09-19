/**=====================================================================================================================
 cxanalogout.h : Declaration of ABSTRACT hardware interface CCxAnalogOut, and placeholder implementation CCxNullAO.
======================================================================================================================*/


#if !defined(CXANALOGOUT_H__INCLUDED_)
#define CXANALOGOUT_H__INCLUDED_

#include "device.h"              // CDevice -- base class for MaestroRTSS device interfaces
#include "util.h"                // utility classes such as CFPoint

class CCxAnalogOut : public CDevice
{
public:
   // max # of AO channels supported by this interface
   static const int MAX_CHANNELS = 16; 

private:
   // convert chair velocity in deg/sec to raw DAC code for 12- and 16-bit resolution (linear conversion)
   static const double CHAIRVEL_TOAORAW12;
   static const double CHAIRVEL_TOAORAW16;
   
   // 16- or 12-bit resolution, #channels available, and channel# dedicated to chair velocity drive signal
   BOOL m_bIs16Bit; 
   int m_nCh; 
   int m_iChairCh;
   // max, min b2s-encoded voltages (bipolar mode, +/-10V range assumed; 
   int m_iMaxB2s; 
   int m_iMinB2s;
   // for LINEAR conversion of b2s-encoded voltage to actual voltage 
   double m_dB2sToVolt; 

   // prevent compiler from automatically providing default copy constructor and assignment operator
   CCxAnalogOut(const CCxAnalogOut& src); 
   CCxAnalogOut& operator=(const CCxAnalogOut& src);

public:
   // constructor/destructor
   CCxAnalogOut(const CDevice::DevInfo& devInfo, int iDevNum, BOOL bIs16Bit, int nCh, int iChairCh);
   virtual ~CCxAnalogOut() {}

public:
   // # of available AO channels; chan# dedicated as the chair velocity drive signal
   int RTFCNDCL GetNumChannels() {return( IsOn() ? m_nCh : 0 ); }
   int RTFCNDCL GetChairVelChannel() { return(IsOn() ? m_iChairCh : -1); }
   
   // if TRUE, resolution of D/A converters is 16bit, else 12bit
   BOOL RTFCNDCL Is16Bit() { return( m_bIs16Bit ); }

   // "immediate-mode" update of selected AO channel (-1 == all), output specified as DAC code or voltage
   virtual BOOL RTFCNDCL Out(int ch, int b2sVolt) = 0;
   virtual BOOL RTFCNDCL Out(int ch, float fVolt) = 0;

   // methods controlling CX_CHAIR servo
   BOOL RTFCNDCL UpdateChair(float fDriveVel, float fCurrPos, float fExpPos);
   BOOL RTFCNDCL SettleChair(float fCurrPos);
   BOOL RTFCNDCL InitChair();

   // restrict b2s-encoded DAC voltage to allowable range
   int RTFCNDCL CheckRange(const int b2sVolt) 
   {
      int i = b2sVolt;
      if( i < m_iMinB2s ) i = m_iMinB2s;
      else if( i > m_iMaxB2s ) i = m_iMaxB2s;
      return( i );
   }
   
   // convert b2s-encoded DAC value to actual volts and vice versa. Overridable.
   virtual float RTFCNDCL ToVolts(int b2sVolt); 
   virtual int RTFCNDCL ToRaw(float fVolt);
   virtual float RTFCNDCL NearestVolts(float fVolt);
};


/** CCxNullAO -- "No device found" placeholder for CCxAnalogOut interface */
class CCxNullAO : public CCxAnalogOut
{
private:
   CCxNullAO( const CCxNullAO& src );
   CCxNullAO& operator=( const CCxNullAO& src );

public:
   CCxNullAO(const CDevice::DevInfo& devInfo, int iDevNum) : CCxAnalogOut(devInfo, iDevNum, FALSE, 0, -1) {}
   ~CCxNullAO() {}

   BOOL RTFCNDCL Init() { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

   BOOL RTFCNDCL Out(int ch, int b2sVolt) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }
   BOOL RTFCNDCL Out(int ch, float fVolt) { SetDeviceError(CDevice::EMSG_DEVNOTAVAIL); return(FALSE); }

protected:
   BOOL RTFCNDCL MapDeviceResources() { return(FALSE); }
   VOID RTFCNDCL UnmapDeviceResources() {}
};


#endif   // !defined(CXANALOGOUT_H__INCLUDED_)
