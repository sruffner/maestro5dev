//===================================================================================================================== 
//
// xydetroit.h : Declaration of class CXYDetroit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//  
//
//===================================================================================================================== 

#if !defined(XYDETROIT_H__INCLUDED_)
#define XYDETROIT_H__INCLUDED_

#include "cxscope.h"             // CCxScope -- abstraction of CXDRIVER XY scope controller functionality


//===================================================================================================================== 
// Declaration of class CXYDetroit
//===================================================================================================================== 
//
class CXYDetroit : public CCxScope
{
//===================================================================================================================== 
// CONSTANTS & DEFINITIONS
//===================================================================================================================== 
private:
   static const CDevice::DevInfo DEVINFO;          // device identification info
   static LPCTSTR XYCORE_FILE;                     // name of the Detroit-specific XYCORE executable file

   static const WORD PLX_VID;                      // PLX Technologies' vendor ID code
   static const WORD PCI9080_ID;                   // PCI dev ID for the PLX PCI9080 interface chip

                                                   // constants for host access to selected Detroit resources
   static const DWORD PCIREGSZ;                    //    #bytes in PCI9080 interface chip's internal register bank
   static const int PCIHIDR;                       //    PCI Permanent Config ID reg (index into DWORD array)
   static const int PCIMBOX2;                      //    PCI Runtime Mailbox reg #2 (index into DWORD array)

   static const DWORD LOCALREGSZ;                  //    #bytes in memory space 1 reserved for two "local" Detroit regs 
   static const DWORD LOCALREGOFFSET;              //    byte offset from BAR1 to start of "local" Detroit regs
   static const int BOARDID;                       //    Detroit's BoardID reg (index into DWORD array)
   static const int BRDRESET;                      //    Detroit's "Reset" reg (index into DWORD array)

   static const DWORD HPIREGSZ;                    //    #bytes in memory space 1 reserved for Host Port Interface regs 
   static const DWORD HPIREGOFFSET;                //    byte offset from BAR1 to start of HPI regs
   static const int HPIC;                          //    HPI Control register (index into DWORD array)
   static const int HPIA;                          //    HPI Address register
   static const int HPIDAUTO;                      //    HPI Data register (w/ HPIA auto-increment)
   static const int HPID;                          //    HPI Data register (no auto-increment)

   static const DWORD TBCREGSZ;                    //    #bytes in memory space 1 reserved for Test Bus Controller regs 
   static const DWORD TBCREGOFFSET;                //    byte offset from BAR1 to start of TBC regs
   static const int TBCCTRL;                       //    TBC Control register (index into DWORD array)

                                                   // C6x-local addresses of selected C6x regs (use HPI to access):
   static const DWORD EMIF_GLOB_ADDR;              //    EMIF global memory control register
   static const DWORD EMIF_CE0_ADDR;               //    EMIF CE0 memory control register
   static const DWORD EMIF_CE1_ADDR;               //    EMIF CE1 memory control register
   static const DWORD EMIF_CE2_ADDR;               //    EMIF CE2 memory control register
   static const DWORD EMIF_CE3_ADDR;               //    EMIF CE3 memory control register
   static const DWORD EMIF_SDC_ADDR;               //    EMIF SDRAM memory control register
   static const DWORD EMIF_SDT_ADDR;               //    EMIF SDRAM timing register
   static const DWORD TIMER0TCR_ADDR;              //    timer control register for TIMER0

                                                   // selected bit masks for selected registers:
   static const DWORD HPIC_HWOB;                   //    halfword ordering bit in HPIC reg (16-bit reg: note MSW=LSW)
   static const DWORD HPIC_DSPINT;                 //    host CPU --> C6x DSP interrupt (to start core after COFF load) 
   static const DWORD TIMER0_SETOUT;               //    to disable TIMER0 and set the TIMER0 output line
   static const DWORD TBC_CLEARTRST;               //    to clear the TBC's TRST line

   static const DWORD EMIF_MTYPE_ASRAM;            //    if this bit set, CEx is mapped to global ASRAM
   static const DWORD EMIF_MTYPE_SDRAM;            //    if this bit set, CEx is mapped to SDRAM
   static const DWORD EMIF_MTYPE_SBSRAM;           //    if this bit set, CEx is mapped to SBSRAM

   static const DWORD EMIF_GLOB_INIT;              //    initialization values for the memory control registers of the 
   static const DWORD EMIF_CE0_INIT;               //    C6x external memory interface (EMIF) on the Detroit.  after 
   static const DWORD EMIF_CE1_INIT;               //    resetting board, these values must be loaded into the control 
   static const DWORD EMIF_CE2_INIT;               //    regs for proper operation.
   static const DWORD EMIF_CE3_INIT;
   static const DWORD EMIF_SDC_INIT;
   static const DWORD EMIF_SDT_INIT;

   static const DWORD XYCORE_READY;                // command/status register values for communication w/XYCORE
   static const DWORD XYCORE_INIT;
   static const DWORD XYCORE_DOFRAME;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   volatile PULONG      m_pvulCmdStatReg;          // virtual memory-mapped access to:  XYCORE cmd/status register,
   volatile PULONG      m_pvulPCIRegs;             //    PCI local configuration & runtime registers,
   volatile PULONG      m_pvulAsram;               //    onboard global ASRAM (for loading tgt params & update records) 
   volatile PULONG      m_pvulLocalRegs;           //    Detroit local "BoardID" & "Reset" registers
   volatile PULONG      m_pvulHPIRegs;             //    Host Port Interface registers 
   volatile PULONG      m_pvulTBCRegs;             //    Test Bus Controller registers


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CXYDetroit( const CXYDetroit& src );                  // no copy constructor or assignment operator defined
   CXYDetroit& operator=( const CXYDetroit& src ); 

public: 
   CXYDetroit( int iDevNum );                            // constructor 
   ~CXYDetroit() {}                                      // destructor 


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   virtual LPCTSTR RTFCNDCL GetDeviceName()              // [CDevice override] a short device name
   {
      return( "Detroit C6x" );
   }


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   BOOL RTFCNDCL MapDeviceResources();                   // map device memory or I/O space to process space
   VOID RTFCNDCL UnmapDeviceResources();                 // unmap device resources
   BOOL RTFCNDCL OnOpen();                               // device-specific work when opening connection to dev 

   LPCTSTR RTFCNDCL GetCOFFFilename()                    // return name of COFF target executable file 
   {
      return( CXYDetroit::XYCORE_FILE );
   }
   BOOL RTFCNDCL DownloadCOFFData(                       // download a section of COFF file onto TI DSP
      DWORD devAddr, PDWORD pData, int iLen ); 
   BOOL RTFCNDCL DeviceReset();                          // perform a "hard reset" of the device
   BOOL RTFCNDCL DeviceStart( DWORD dwEntry );           // start execution of COFF executable
   VOID RTFCNDCL DeviceQuit();                           // stop execution of COFF executable

   BOOL RTFCNDCL LoadParameters();                       // download tgt defns & animation parameters to XY scope dev
   BOOL RTFCNDCL DoFrame();                              // download tgt update records & initiate display frame update 

private:
   BOOL RTFCNDCL IsReady()                               // is XY scope dev ready for next command?
   {
      return( BOOL((m_pvulCmdStatReg != NULL) && (*m_pvulCmdStatReg == XYCORE_READY)) );
   }
};



#endif   // !defined(XYDETROIT_H__INCLUDED_)

