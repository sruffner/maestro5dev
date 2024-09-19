//===================================================================================================================== 
//
// xydetroit.cpp : Implementation of the abtract CCxScope interface, targeting the TIC6201 DSP-based "Detroit" board 
//                 from Spectral Signal Processing, Inc.
//
// DESCRIPTION:
// The "XY scope" -- a large-screen, high-performance XY oscilloscope -- is an important target platform in CXDRIVER.
// A wide variety of visual targets are presented on this medium, including spots and various kinds of random-dot 
// patterns.  The X, Y, and trigger inputs of the scope are driven by an in-house "dotter board" which, in turn, is 
// controlled by a DSP-based hardware device residing in the host system.  The XY scope controller is represented by 
// the CCxScope abstract interface.  Animation of targets by the XY scope controller proceeds on a frame-by-frame basis 
// under complete control of CXDRIVER.  See CCxScope & CDevice for more details.
//
// CXYDetroit is a CCxScope interface implementation realized on SSP's "Detroit C6x" TIC6201 DSP board (CREDITS).  The 
// Detroit talks to the dotter board over its DSP~LINK3 communication interface.  The XY scope controller functions are 
// executed by a firmware program -- XYCORE -- that runs onboard the Detroit.  CXYDetroit is essentially the "host" 
// side of the XY scope device.  It downloads target information and motion updates to the Detroit, while XYCORE is 
// responsible for actually drawing the defined targets.
//
// The interactions between CXYDetroit and XYCORE are very simple.  Two memory-mapped resources are used:
//
//    1) "Command/Status" Register.  CXYDetroit writes commands to XYCORE via this register, and reads it to check for  
//    command completion.  We use a "mailbox" register in the Detroit's PCI internal register bank to implement this 
//    feature.  The mailbox register's hardware implementation prevents errors that could arise from attempts to 
//    access it from both sides simultaneously.
//
//    2) "SharedData" RAM.  CXYDetroit writes target and animation info to this memory-mapped resource residing on the 
//    Detroit.  XYCORE uses this information to update the XY scope display.  A portion of the Detroit's global 
//    asynchronous SRAM (ASRAM) is dedicated as the "shared data" area.  The target definitions and general animation 
//    parameters are stored first in this area, as the CCxScope::Parameters structure.  Immediately following this 
//    data structure are the "per-target" frame update records (CCxScope::UpdateRec) for the current display frame. 
//    Room is provided for up to CCxScope::MAX_TARGETS targets.
//
// XYCORE responds to two commands from CXYDetroit, corresponding to the two pure virtual CCxScope methods that 
// CXYDetroit must implement:
//
//    LoadParameters():  CXYDetroit writes the CCxScope::Parameters structure to the "shared data" area and then 
// issues the XYCORE_INIT.  In response, XYCORE copies the parameters structure into private memory (so CXYDetroit can 
// safely change it later), then creates internal representations of each target's initial dot positions -- in 
// preparation for a new animation sequence.  [NOTE that parameters structure is actually prepared by CCxScope itself 
// and stored in a protected member so that derived classes can access it.  See CCxScope for details.]
//
//    DoFrame():  CXYDetroit writes an array of CCxScope::UpdateRec frame update records to the "shared data" area and 
// then issues XYCORE_DOFRAME to initiate the display frame update.  [NOTE, again, that the update records are actually 
// prepared by CCxScope itself and stored in a protected member.]  Per the CCxScope interface spec, we must make sure 
// that XYCORE is ready for the update (it could still be working on the previous frame) before attempting it.  This is 
// simply a matter of checking the command/status register for a ready status (XYCORE_READY).  We do NOT wait for 
// the ready status AFTER issuing XYCORE_DOFRAME -- since it can take several milliseconds to draw the frame!
//
// See the XYCORE source code files for more details on how XYCORE does its job...
//
// ==> Opening a connection to the Detroit; loading XYCORE.
// Base class CDevice provides a framework for opening a connection to a hardware device, and in the case of devices 
// using a single TIC6x/4x DSP, a method for loading firmware (in the form of an executable COFF file) into processor 
// memory.  This framework requires that derived classes supply device-specific overrides for a number of virtual and 
// pure virtual CDevice methods, including:  MapDeviceResources(), UnmapDeviceResources(), OnOpen(), GetCOFFFilename(), 
// DownloadCOFFData(), DeviceReset(), DeviceStart(), and DeviceQuit().
//
// In addition to the memory-mapped resources already mentioned, CXYDetroit::MapDeviceResources() acquires access to 
// the Detroit's Host Port Interface registers, "local" registers, and Test Bus Controller registers.  A dedicated 
// "local" register is used to perform a software reset of the Detroit.  The Host Port Interface is used to initialize 
// the Detroit's EMIF after the reset, download sections of COFF data to the C6x's program and data memory, and restart 
// the CPU once the COFF file has been downloaded.
//
//
// CREDITS:
// 1) Detroit C6x Development Package, including manuals and sample source code.  Spectrum Signal Processing.
// 2) TMS320C6000 Peripherals Reference (spru190b).  Texas Instruments, March 1998.
//
// REVISION HISTORY:
// 27sep2002-- Adapting from the original C-language module XYSCOPE, part of the older "cntrlxPC" app.  The COFF load 
//             method has been generalized and is located in the base class CDevice.
// 04oct2002-- Minor mod IAW change in CDevice::DeviceStart().
// 29dec2003-- GetCOFFPath() renamed GetCOFFFilename() and returns the name of the COFF executable file rather than the 
//             full path.  CDevice assumes this file is found in the Maestro/CXDRIVER "home" directory.
//
//===================================================================================================================== 

#include "xydetroit.h"



//===================================================================================================================== 
// CONSTANTS INITIALIZED
//===================================================================================================================== 

const CDevice::DevInfo CXYDetroit::DEVINFO =
{
   CDevice::DF_PCI | CDevice::DF_TIC6x,                        // Detroit is hosted on PCI bus and uses the TI C6x DSP
   0x12fb,                                                     // Spectrum Signal Processing's vendor ID code
   0xde62,                                                     // PCI device ID for the Detroit 'C6x board
   0, 0                                                        // subvendor and subsystem IDs ignored
};

LPCTSTR        CXYDetroit::XYCORE_FILE       = "xydetroit.out";

const WORD CXYDetroit::PLX_VID               = 0x10b5;         // PLX Technologies' vendor ID code
const WORD CXYDetroit::PCI9080_ID            = 0x9080;         // PCI dev ID for the PLX PCI9080 interface chip

const DWORD CXYDetroit::PCIREGSZ             = 1024;           // PCI interface reg bank size (only 256 bytes used)
const int CXYDetroit::PCIHIDR                = (0x70 >> 2);    // these are indices into DWORD array...
const int CXYDetroit::PCIMBOX2               = (0x48 >> 2);

const DWORD CXYDetroit::LOCALREGSZ           = 1024;           // reserved for "local" Detroit regs (only 8 bytes used) 
const DWORD CXYDetroit::LOCALREGOFFSET       = 0x00020000;
const int CXYDetroit::BOARDID                = 0;              // indices into DWORD array...
const int CXYDetroit::BRDRESET               = 1;

const DWORD CXYDetroit::HPIREGSZ             = 1024;           // reserved for HPI regs (only 16 bytes used)
const DWORD CXYDetroit::HPIREGOFFSET         = 0x00040000;
const int CXYDetroit::HPIC                   = 0;              // indices into DWORD array...
const int CXYDetroit::HPIA                   = 1;
const int CXYDetroit::HPIDAUTO               = 2;
const int CXYDetroit::HPID                   = 3;

const DWORD CXYDetroit::TBCREGSZ             = 1024;           // reserved for TBC regs (only ~96 bytes used)
const DWORD CXYDetroit::TBCREGOFFSET         = 0x00060000;
const int CXYDetroit::TBCCTRL                = 2;              // index into DWORD array

const DWORD CXYDetroit::EMIF_GLOB_ADDR       = 0x01800000;     // C6x-local addresses for selected C6x registers: put 
const DWORD CXYDetroit::EMIF_CE0_ADDR        = 0x01800008;     // addr in HPI Address reg to access corresponding reg!
const DWORD CXYDetroit::EMIF_CE1_ADDR        = 0x01800004;
const DWORD CXYDetroit::EMIF_CE2_ADDR        = 0x01800010;
const DWORD CXYDetroit::EMIF_CE3_ADDR        = 0x01800014;
const DWORD CXYDetroit::EMIF_SDC_ADDR        = 0x01800018;
const DWORD CXYDetroit::EMIF_SDT_ADDR        = 0x0180001C;
const DWORD CXYDetroit::TIMER0TCR_ADDR       = 0x01940000;

const DWORD CXYDetroit::HPIC_HWOB            = 0x00010001;
const DWORD CXYDetroit::HPIC_DSPINT          = 0x00020002;
const DWORD CXYDetroit::TIMER0_SETOUT        = 0x00000004;
const DWORD CXYDetroit::TBC_CLEARTRST        = 0x08000800;

const DWORD CXYDetroit::EMIF_MTYPE_ASRAM     = 0x00000020;
const DWORD CXYDetroit::EMIF_MTYPE_SDRAM     = 0x00000030;
const DWORD CXYDetroit::EMIF_MTYPE_SBSRAM    = 0x00000040;

const DWORD CXYDetroit::EMIF_GLOB_INIT       = 0x00003078; 
const DWORD CXYDetroit::EMIF_CE0_INIT        = 0xFFFF3F43;
const DWORD CXYDetroit::EMIF_CE1_INIT        = 0x30E30422;
const DWORD CXYDetroit::EMIF_CE2_INIT        = 0xFFFF3F33;
const DWORD CXYDetroit::EMIF_CE3_INIT        = 0x00000030;
const DWORD CXYDetroit::EMIF_SDC_INIT        = 0x0544A000;
const DWORD CXYDetroit::EMIF_SDT_INIT        = 0x0000061A;

const DWORD CXYDetroit::XYCORE_READY         = 1;              // values in XYCORE command/status register
const DWORD CXYDetroit::XYCORE_INIT          = 2;
const DWORD CXYDetroit::XYCORE_DOFRAME       = 3;



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CXYDetroit [constructor] ======================================================================================== 
//
//    Constructs device object, unconnected to a physical device.  
//
//    ARGS:       iDevNum  -- [in] instance of Detroit board on PCI bus that is to be associated with this dev object.
//
CXYDetroit::CXYDetroit( int iDevNum ) : CCxScope( DEVINFO, iDevNum )
{
   m_pvulCmdStatReg = NULL;
   m_pvulPCIRegs = NULL;
   m_pvulAsram = NULL;
   m_pvulLocalRegs = NULL;
   m_pvulHPIRegs = NULL;
   m_pvulTBCRegs = NULL;
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== Map/UnmapDeviceResources [base override] ========================================================================
//
//    Acquire and release the memory mapped or I/O resources needed to talk to the physical device.  
//
//    The Detroit's PCI interface makes three different address spaces available to the PC host, providing access to 
//    various Detroit resources.  We only need access to some of these resources:
//       1) BAR0 = PCI9080 interface chip's internal register bank.  This gives access to the PCI local configuration 
//          and shared runtime registers.  The "Mailbox_2" reg, which serves as the command/status register for XYCORE, 
//          is included in this bank.
//       2) BAR2 = Local bus memory space 0 --> C6x onboard global asynch SRAM.  XY scope configuration & target 
//          information (CCxScope::Parameters structure) and per-frame target update records (CCxScope::UpdateRec) are 
//          written to this memory space.
//       3) BAR3 = Local bus memory space 1 --> variety of C6x assets.  The assets used by XYAPI are:  (a) C6x local 
//          registers "BoardID" and "Reset".  (b) C6x Host Port Interface registers (these are needed for downloading 
//          XYCORE into processor memory!).  (c) C6x Test Bus Controller registers.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::MapDeviceResources() 
{
   UnmapDeviceResources();                                              // just a safety precaution

   BOOL bOk = TRUE;

   LARGE_INTEGER i64BAR;                                                // map Detroit's PCI reg into process space: 
   i64BAR.QuadPart = GetPciBaseAddressReg( 0 ) & 0xffffff00;            //    BAR0 = start of PCI regs (physical addr) 
   m_pvulPCIRegs = (PULONG) ::RtMapMemory( i64BAR, PCIREGSZ, MmNonCached ); 
   bOk = BOOL(m_pvulPCIRegs != NULL);

   if( bOk )                                                            // set up runtime mailbox reg 2 as the 
      m_pvulCmdStatReg = &(m_pvulPCIRegs[PCIMBOX2]);                    // "command/status" reg for XYAPI/XYCORE

   if( bOk )                                                            // map portion of Detroit's ASRAM into process 
   {                                                                    // space...
      int iLen = sizeof( CCxScope::Parameters ) +                       //    #bytes needed for downloading tgt params 
                 MAX_TARGETS * sizeof( CCxScope::UpdateRec ) + 1000;    //    and motion update records, w/some extra 

      i64BAR.QuadPart = GetPciBaseAddressReg( 2 ) & 0xfffffff0;         //    BAR2 = start of ASRAM (phys addr)
      m_pvulAsram = (PULONG) ::RtMapMemory( i64BAR, (ULONG)iLen, MmNonCached ); 
      bOk = BOOL(m_pvulAsram != NULL);
   }

   if( bOk )                                                            // map Detroit's local reg bank into process 
   {                                                                    // space...
      i64BAR.QuadPart = GetPciBaseAddressReg( 3 ) & 0xfffffff0;         //    BAR3 + offset = start of local reg bank 
      i64BAR.QuadPart += LOCALREGOFFSET;
      m_pvulLocalRegs = (PULONG) ::RtMapMemory( i64BAR, LOCALREGSZ, MmNonCached ); 
      bOk = BOOL(m_pvulLocalRegs != NULL);
   }

   if( bOk )                                                            // map Detroit's Host Port Interface into  
   {                                                                    // process space...
      i64BAR.QuadPart = GetPciBaseAddressReg( 3 ) & 0xfffffff0;         //    BAR3 + offset = start of HPI reg bank 
      i64BAR.QuadPart += HPIREGOFFSET;
      m_pvulHPIRegs = (PULONG) ::RtMapMemory( i64BAR, HPIREGSZ, MmNonCached ); 
      bOk = BOOL(m_pvulHPIRegs != NULL);
   }

   if( bOk )                                                            // map Detroit's Test Bus Ctrlr reg bank into 
   {                                                                    // process space...
      i64BAR.QuadPart = GetPciBaseAddressReg( 3 ) & 0xfffffff0;         //    BAR3 + offset = start of TBC reg bank 
      i64BAR.QuadPart += TBCREGOFFSET;
      m_pvulTBCRegs = (PULONG) ::RtMapMemory( i64BAR, TBCREGSZ, MmNonCached ); 
      bOk = BOOL(m_pvulTBCRegs != NULL);
   }

   if( !bOk ) UnmapDeviceResources();                                   // on failure, unmap any resources that WERE 
                                                                        // successfully mapped.
   return( bOk );
}

VOID RTFCNDCL CXYDetroit::UnmapDeviceResources()
{
   if( m_pvulTBCRegs != NULL ) 
   {
      ::RtUnmapMemory( m_pvulTBCRegs );
      m_pvulTBCRegs = NULL;
   }

   if( m_pvulHPIRegs != NULL ) 
   {
      ::RtUnmapMemory( m_pvulHPIRegs );
      m_pvulHPIRegs = NULL;
   }

   if( m_pvulLocalRegs != NULL ) 
   {
      ::RtUnmapMemory( m_pvulLocalRegs );
      m_pvulLocalRegs = NULL;
   }

   if( m_pvulAsram != NULL ) 
   {
      ::RtUnmapMemory( m_pvulAsram );
      m_pvulAsram = NULL;
   }

   if( m_pvulPCIRegs != NULL ) 
   {
      ::RtUnmapMemory( m_pvulPCIRegs );
      m_pvulPCIRegs = NULL;
   } 

   m_pvulCmdStatReg = NULL;
}


//=== OnOpen [base override] ========================================================================================== 
//
//    Perform any one-time, device-specific tasks that must be done immediately after "opening" the connection to the 
//    physical device.  This method is called by CDevice::Open().
//
//    Here we merely perform some "sanity checks" to ensure we've established a connection with the Detroit.  Note our 
//    usage of the HPI to read C6x local memory (vs ASRAM) locations.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::OnOpen()
{
   if( !IsOn() ) return( FALSE );

   BOOL bCheckOK = TRUE;

   m_pvulHPIRegs[HPIA] = EMIF_CE0_ADDR;                        // read EMIF control reg for memory space CE0, and...
   DWORD dwTemp = m_pvulHPIRegs[HPID];
   bCheckOK = BOOL(dwTemp & EMIF_MTYPE_SBSRAM);                // ...verify CE0 is mapped to SBSRAM
   if( bCheckOK )                                              // read EMIF control reg for memory space CE1, and...
   {
      m_pvulHPIRegs[HPIA] = EMIF_CE1_ADDR; 
      dwTemp = m_pvulHPIRegs[HPID];
      bCheckOK = BOOL(dwTemp & EMIF_MTYPE_ASRAM);              // ...verify CE1 is mapped to ASRAM
   }
   if( bCheckOK )                                              // read EMIF control reg for memory space CE2, and...
   {
      m_pvulHPIRegs[HPIA] = EMIF_CE2_ADDR; 
      dwTemp = m_pvulHPIRegs[HPID];
      bCheckOK = BOOL(dwTemp & EMIF_MTYPE_SDRAM);              // ...verify CE2 is mapped to SDRAM
   }
   if( bCheckOK )                                              // read EMIF control reg for memory space CE3, and...
   {
      m_pvulHPIRegs[HPIA] = EMIF_CE3_ADDR; 
      dwTemp = m_pvulHPIRegs[HPID];
      bCheckOK = BOOL(dwTemp & EMIF_MTYPE_SDRAM);              // ...verify CE3 is mapped to SDRAM
   }

   if( bCheckOK )                                              // verify vendor & device IDs for the onboard PLX Tech
   {                                                           // PCI9080 interface chip..
      dwTemp = m_pvulPCIRegs[PCIHIDR];
      bCheckOK = BOOL( (HIWORD(dwTemp) == PCI9080_ID) && (LOWORD(dwTemp) == PLX_VID) );
   }

   if( bCheckOK )                                              // verify we can write/read in ASRAM...
   {
      int iLen = sizeof(CCxScope::Parameters) +                //    size of required portion of ASRAM in 4-byte words 
                 MAX_TARGETS * sizeof(CCxScope::UpdateRec);
      iLen /= sizeof(ULONG);

      int i;
      for( i = 0; i < iLen; i++ ) m_pvulAsram[i] = (DWORD) i;  //    write data to ASRAM

      i = iLen-1;                                              //    read it back in reverse order, verifying contents 
      while( (i >= 0) && bCheckOK )
      {
         dwTemp = m_pvulAsram[i];
         bCheckOK = BOOL(dwTemp == (DWORD) i);
         i--;
      }
   }

   if( !bCheckOK ) SetDeviceError( CDevice::EMSG_VERIFYFAIL ); // sanity checks failed!

   return ( bCheckOK );
}


//=== DownloadCOFFData [base override] ================================================================================ 
//
//    Download a section of COFF data into the program, data, or other memory resource associated with a TMS320C4x or 
//    C6x DSP onboard the device.  Intended for loading a core program onto a DSP node during boot mode (CPU in reset).
//
//    In the case of the Detroit, we use the C6x's Host Port Interface (HPI) for bootloading.  The HPI includes a "data 
//    with auto-increment" register, which speeds up the load process.  We load the start address of the section into 
//    the HPIA register, and then write each 32-bit datum to the HPIDAUTO register; the HPIA is incremented to the next 
//    32-bit memory location after each write. 
//
//    ARGS:       devAddr  -- [in] device address at which to start download.
//                pData    -- [in] buffer containing COFF data to download (as 32-bit data).
//                iLen     -- [in] # of 32-bit words to download.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::DownloadCOFFData( DWORD devAddr, PDWORD pData, int iLen )
{
   if( m_pvulHPIRegs == NULL ) return( FALSE );

   m_pvulHPIRegs[HPIA] = devAddr;
   for( int i = 0; i < iLen; i++ ) m_pvulHPIRegs[HPIDAUTO] = pData[i];

   return( TRUE );
}


//=== DeviceReset [base override] ===================================================================================== 
//
//    Reset the device; if it has a TIC6x/C4x DSP, the DSP's CPU should be left in a suspended state in preparation for 
//    COFF download.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::DeviceReset()
{
   if( m_pvulLocalRegs == NULL || m_pvulTBCRegs == NULL || m_pvulHPIRegs == NULL ) return( FALSE );

   m_pvulLocalRegs[BRDRESET] = 0x00000001;                        // software reset:  assert reset bit for >= 5ms
   ::Sleep( 10 );
   m_pvulLocalRegs[BRDRESET] = 0;

   m_pvulTBCRegs[TBCCTRL] = TBC_CLEARTRST;                        // to clear any spurious interrupts from TBC

   m_pvulHPIRegs[HPIC] = HPIC_HWOB;                               // set HWOB bit in HPIC to enable host PC access to 
                                                                  // C6x local memory via the HPI; this bit should
                                                                  // remain set until device is closed.

   m_pvulHPIRegs[HPIA] = EMIF_GLOB_ADDR;                          // initialize C6x external memory interface...
   m_pvulHPIRegs[HPID] = EMIF_GLOB_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_CE0_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_CE0_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_CE1_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_CE1_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_CE2_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_CE2_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_CE3_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_CE3_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_SDC_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_SDC_INIT;
   m_pvulHPIRegs[HPIA] = EMIF_SDT_ADDR;
   m_pvulHPIRegs[HPID] = EMIF_SDT_INIT;

   m_pvulHPIRegs[HPIA] = TIMER0TCR_ADDR;                          // toggle C6x TIMER0 pin so that C6x has access to
   m_pvulHPIRegs[HPID] = TIMER0_SETOUT;                           // local bus (which is blocked when board is reset)
   m_pvulHPIRegs[HPID] = 0;
   m_pvulHPIRegs[HPID] = TIMER0_SETOUT;

   return( TRUE );
}


//=== DeviceStart [base override] ===================================================================================== 
//
//    Start the previously downloaded COFF executable on the device's TIC6x or C4x DSP and verify that it's running.
//
//    The Detroit's onboard TIC6201 DSP is configured in "HPI Map 1" boot mode.  When reset in this boot mode, the 
//    TIC6201 CPU enters a boot state, waiting for the DSPINT bit in the Host Port Interface control register (HPIC) 
//    to be raised.  To start the CPU, all we have to do is set the DSPINT bit.  Upon receiving this INT, the CPU 
//    "wakes" up and starts executing from program memory address 0.  This method should only be called immediately 
//    after completing the COFF download while in HPI boot mode.  Raising DSPINT at any other time could lead to 
//    undefined behavior.
//
//    ARGS:       dwEntry  -- [in] local (to DSP) address of COFF's entry point (not used here).
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::DeviceStart( DWORD dwEntry)
{
   if( m_pvulHPIRegs == NULL || m_pvulCmdStatReg == NULL ) 
      return( FALSE );

   m_pvulHPIRegs[HPIC] = HPIC_HWOB | HPIC_DSPINT;                          // start XYCORE

   CElapsedTime eTime;                                                     // give it time to start up
   while( (!IsReady()) && (eTime.Get() < XYDEV_TIMEOUT) ) ;

   return( IsReady() );
}


//=== DeviceQuit [base override] ====================================================================================== 
//
//    Cause the target COFF executable on a TIC6x- or C4x-based device to stop running.  The device will become 
//    unavailable upon invoking this method. 
//
//    Here, we merely reset the Detroit and leave it in that state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CXYDetroit::DeviceQuit()
{
   DeviceReset();
}


//=== LoadParameters [base override] ================================================================================== 
//
//    Download the current target definitions & animation parameters to the XY scope device, then issue a command to 
//    read all parameters and prepare targets for subsequent frame-by-frame animation.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDetroit::LoadParameters()
{
   CElapsedTime eTime;                                               // wait for device ready
   while( (!IsReady()) && (eTime.Get() < XYDEV_TIMEOUT) ) ;
   if( !IsReady() ) return( FALSE );

   ::memmove( (PVOID) m_pvulAsram, (PVOID) &m_Parameters,            // download tgt defns, etc.
              sizeof( CCxScope::Parameters ) );
   *m_pvulCmdStatReg = XYCORE_INIT;                                  // issue cmd to read params & prepare tgts

   eTime.Reset();                                                    // wait for device ready -- indicating that device 
   while( (!IsReady()) && (eTime.Get() < XYDEV_TIMEOUT) ) ;          // has finished preparing internal representations 
   if( !IsReady() ) return( FALSE );                                 // of all defined targets.

   return( TRUE );
}


//=== DoFrame [base override] ========================================================================================= 
//
//    Download per-target frame update records & initiate a display frame update on the XY scope device.  If the device 
//    is still busy "drawing" the previous frame or is otherwise not responding, the method will fail -- and the new 
//    display frame is "dropped".
//
//    Per-target display frame update records are stored in the Detroit's global ASRAM immediately after the data 
//    structure (CCxScope::Parameters) that holds target definition and animation parameters.  The records are stored 
//    in the order in which targets were defined -- this is mandated by the CCxScope interface.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (XY scope dev not ready).
//
BOOL RTFCNDCL CXYDetroit::DoFrame()
{
   if( !IsReady() ) return ( FALSE );                          // XYCORE not ready -- "dropped frame"

   int i = sizeof( CCxScope::Parameters );                     // get start pos in ASRAM for writing update records
   PCHAR pStart = (PCHAR) ( ((PCHAR) (&m_pvulAsram[0])) + i );

   i = int( m_Parameters.wNumTargets );                        // download the update records
   i *= sizeof( CCxScope::UpdateRec );
   ::memmove( (PVOID) pStart, (PVOID) &(m_NextUpdate[0]), i );

   *m_pvulCmdStatReg = XYCORE_DOFRAME;                         // tell XYCORE to start update 
   return ( TRUE );
}

