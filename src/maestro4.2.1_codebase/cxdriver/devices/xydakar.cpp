//===================================================================================================================== 
//
// xydakar.cpp : Implementation of the abtract CCxScope interface, targeting the TIC44 DSP-based "Dakar F5" board 
//               from Spectral Signal Processing, Inc.
//
// DESCRIPTION:
// The "XY scope" -- a large-screen, high-performance XY oscilloscope -- is an important target platform in CXDRIVER.
// A wide variety of visual targets are presented on this medium, including spots and various kinds of random-dot 
// patterns.  The X, Y, and trigger inputs of the scope are driven by an in-house "dotter board" which, in turn, is 
// controlled by a DSP-based hardware device residing in the host system.  The XY scope controller is represented by 
// the CCxScope abstract interface.  Animation of targets by the XY scope controller proceeds on a frame-by-frame basis 
// under complete control of CXDRIVER.  See CCxScope & CDevice for more details.
//
// CXYDakar is a CCxScope interface implementation realized on SSP's "Dakar F5 Carrier" board (CREDITS).  This board 
// can hold two TI DSP nodes in addition to the embedded C44 Node A.  However, we only make use of the embedded C44 
// processor.  The Dakar talks to the dotter board over a DSP~LINK3 communication interface.  The XY scope controller 
// functions are executed by a firmware program -- XYCORE -- that runs onboard the Dakar.  CXYDakar is essentially the 
// "host" side of the XY scope device.  It downloads target information and motion updates to the Dakar, while XYCORE 
// is responsible for actually drawing the defined targets.
//
// The interactions between CXYDakar and XYCORE are very simple.  Two memory-mapped resources are used:
//
//    1) "Command/Status" Register.  CXYDakar writes commands to XYCORE via this register, and reads it to check for  
//    command completion.  We use a "mailbox" register in the Dakar's PCI internal register bank to implement this 
//    feature.  The mailbox register's hardware implementation prevents errors that could arise from attempts to 
//    access it from both sides simultaneously.
//
//    2) "SharedData" RAM.  CXYDakar writes target and animation info to this memory-mapped resource residing on the 
//    Dakar.  XYCORE uses this information to update the XY scope display.  A portion of the Dakar's "Far Global SRAM" 
//    (FGSRAM) is dedicated as the "shared data" area.  The target definitions and general animation parameters are 
//    stored first, immmediately followed by the "per-target" frame update records for the current display frame. 
//    Room is provided for up to CCxScope::MAX_TARGETS targets.
//
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
//       Keep in mind that PCI-bus addresses map to 8-bit words, while C4x-processor addresses map to 32-bit words --
//       a DWORD is the fundamental memory unit on the C44!!  This has a big impact on the "SharedData" interface. 
//       Since the C4x processor does not provide for native 16-bit integers, we must do some extra work when writing 
//       the CCxScope::Parameters structure and CCxScope::UpdateRec per-target update records to the Dakar's FGSRAM.
//       We must unpack the 16-bit data in these structures and write it as DWORDs in FGSRAM.  Leaving this task to 
//       XYCORE itself would likely degrade its performance.  See LoadParameters() and DoFrame() for details.
//       !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
//
// XYCORE responds to two commands from CXYDakar, corresponding to the two pure virtual CCxScope methods that CXYDakar 
// must implement: 
//
//    LoadParameters():  CXYDakar writes the CCxScope::Parameters structure to the "shared data" area and then issues 
// XYCORE_INIT command.  In response, XYCORE copies the parameters structure into private memory (so CXYDakar can 
// safely change it later), then creates internal representations of each target's initial dot positions -- in 
// preparation for a new animation sequence.  [NOTE that parameters structure is actually prepared by CCxScope itself 
// and stored in a protected member so that derived classes can access it.  See CCxScope for details.]
//
//    DoFrame():  CXYDakar writes an array of CCxScope::UpdateRec frame update records to the "shared data" area and 
// then issues XYCORE_DOFRAME to initiate the display frame update.  [NOTE, again, that the update records are actually 
// prepared by CCxScope itself and stored in a protected member.]  Per the CCxScope interface spec, we must make sure 
// that XYCORE is ready for the update (it could still be working on the previous frame) before attempting it.  This is 
// simply a matter of checking the command/status register for a ready status (XYCORE_READY).  We do NOT wait for the 
// the ready status AFTER issuing XYCORE_DOFRAME -- since it can take several milliseconds to draw the frame!
//
// See the XYCORE source code files for more details on how XYCORE does its job...
//
// ==> Opening a connection to the Dakar; loading XYCORE.
// Base class CDevice provides a framework for opening a connection to a hardware device, and in the case of devices 
// using a TIC6x/4x DSP, a method for loading firmware (in the form of an executable COFF file) into processor memory.
// This framework requires that derived classes supply device-specific overrides for a number of virtual and pure 
// virtual CDevice methods, including:  MapDeviceResources(), UnmapDeviceResources(), OnOpen(), GetCOFFFilename(), 
// DownloadCOFFData(), DeviceReset(), DeviceStart(), and DeviceQuit().
//
// In addition to the memory-mapped resources already mentioned, CXYDakar::MapDeviceResources() acquires access to 
// the Dakar's Node A Interrupt Control registers, and it reserves the first portion of FGSRAM as a dedicated bootload 
// area.  These resources are used to download and start XYCORE (as an executable COFF file).  For details on the 
// bootload procedure, see methods DownloadCOFFData() and DeviceStart().
//
//
// CREDITS:
// 1) Dakar F5 Development Package, including manuals and sample source code.  Spectrum Signal Processing.
// 2) TMS320C44x Peripherals Reference.  Texas Instruments.
//
// REVISION HISTORY:
// 03oct2002-- Adapting from the original C-language module XYSCOPE, part of the older "cntrlxPC" app.  The COFF load 
//             method has been generalized and is located in the base class CDevice.
// 29dec2003-- GetCOFFPath() renamed GetCOFFFilename() and returns the name of the COFF executable file rather than the 
//             full path.  CDevice assumes this file is found in the Maestro/CXDRIVER "home" directory.
// 16feb2005-- Fixed PARAMS_SIZE32 and LoadParameters(), which were based on an older version of the Parameters struct. 
// 09may2011-- Updated PARAMS_SIZE32 and LoadParameter() IAW change in Parameters struct (added three new fields and 
//             .wFiller is now an array of length 2.
//===================================================================================================================== 

#include "xydakar.h"



//===================================================================================================================== 
// CONSTANTS INITIALIZED
//===================================================================================================================== 

const CDevice::DevInfo CXYDakar::DEVINFO =
{
   CDevice::DF_PCI | CDevice::DF_TIC4x,                        // Detroit is hosted on PCI bus and uses the TI C4x DSP
   0x12fb,                                                     // Spectrum Signal Processing's vendor ID code
   0x00f5,                                                     // PCI device ID for the Dakar board
   0, 0                                                        // subvendor and subsystem IDs ignored
};

LPCTSTR CXYDakar::XYCORE_FILE                = "xydakar.out";

const WORD CXYDakar::PLX_VID                 = 0x10b5;         // PLX Technologies' vendor ID code
const WORD CXYDakar::PCI9060_ID              = 0x9060;         // PCI dev ID for the PLX PCI9060 interface chip

const DWORD CXYDakar::PCIREGSZ               = 256;            // PCI interface reg bank size (<256 bytes used)
const int CXYDakar::PCIMBOX2                 = (0x48 >> 2);    // (these are indices into a DWORD array...)
const int CXYDakar::PCICNTRL                 = (0x6C >> 2);

const DWORD CXYDakar::PCICNTRL_SWRESET       = 0x40000000;     // selected bit masks for in PCI Control register
const DWORD CXYDakar::PCICNTRL_RELCFG        = 0x20000000;     //


const DWORD CXYDakar::BOOTAREAOFFSET         = 0x00C00000;     // dedicated bootload area in "Far Global SRAM", 
const DWORD CXYDakar::BOOTAREASZ             = 0x2400;
const DWORD CXYDakar::FGSRAMOFFSET           = 0x00C02400;     // start of general-purpose FGSRAM
   
const DWORD CXYDakar::INTREGOFFSET           = 0x008C0000;     // Node A Interrupt Control register bank
const DWORD CXYDakar::INTREGSZ               = 64;
const int CXYDakar::INTPCI2A                 = (0x00 >> 2);    // (index into DWORD array)

const DWORD CXYDakar::C4X_SRAM_ADDR          = 0xC0300000;
const DWORD CXYDakar::C4X_USRENTRY_ADDR      = 0x002FFFF0;
const int CXYDakar::BOOT_DMADONE             = 0x0050;
const int CXYDakar::BOOT_DMAHDR              = 0x0070;
const int CXYDakar::BOOT_CTLBLK              = 0x0080;
const int CXYDakar::BOOT_CTLBLK_SZ           = 0x0800;
const DWORD CXYDakar::BOOT_DMAHDR_CTL        = 0x00C0000D;
const DWORD CXYDakar::BOOT_DMAHDR_LAST       = 0x00C00005;
const double CXYDakar::BOOT_TIMEOUTUS        = 100000.0;

const DWORD CXYDakar::XYCORE_READY           = 1;              // values in XYCORE command/status register
const DWORD CXYDakar::XYCORE_INIT            = 2;
const DWORD CXYDakar::XYCORE_DOFRAME         = 3;

const int CXYDakar::PARAMS_SIZE32            =                 // # of individual fields in CCxScope::Parameters struct 
                              9 + CCxScope::MAX_TARGETS * 10;  // we map each field to a separate DWORD in the Dakar's 
                                                               // FGSRAM -- since DWORD is the fundamental memory unit! 



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CXYDakar [constructor] ========================================================================================== 
//
//    Constructs device object, unconnected to a physical device.  
//
//    ARGS:       iDevNum  -- [in] instance of Dakar board on PCI bus that is to be associated with this dev object.
//
CXYDakar::CXYDakar( int iDevNum ) : CCxScope( DEVINFO, iDevNum )
{
   m_pvulCmdStatReg = NULL;
   m_pvulPCIRegs = NULL;
   m_pvulBootLoad = NULL;
   m_pvulFgsram = NULL;
   m_pvulIntRegs = NULL;
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== Map/UnmapDeviceResources [base override] ========================================================================
//
//    Acquire and release the memory mapped or I/O resources needed to talk to the physical device.  
//
//    The Dakar's PCI interface makes two different address spaces available to the host, providing access to various 
//    Dakar resources.  We only need access to a portion of these resources:
//       1) BAR0 = PCI9060 interface chip's internal register bank.  This gives access to the PCI local configuration 
//          and shared runtime registers.  The "Mailbox_2" reg, which serves as the command/status register for XYCORE, 
//          is included in this bank.
//       2) BAR2 = Local bus memory space 0 --> a variety of Dakar assets, including "Far Global SRAM" (FGSRAM) and the 
//          Node A Interrupt Control register bank.  The first BOOTAREASZ bytes of FGSRAM are dedicated to the 
//          bootloader DMA scheme that is used to download XYCORE onto the embedded C44 processor (Node A).  The rest 
//          of FGSRAM is available as the "shared data" area.  XY scope configuration & target information 
//          (CCxScope::Parameters structure) and per-frame target update records (CCxScope::UpdateRec) are written to 
//          this memory region.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDakar::MapDeviceResources() 
{
   UnmapDeviceResources();                                              // just a safety precaution

   BOOL bOk = TRUE;

   LARGE_INTEGER i64BAR;                                                // map Dakar's PCI reg into process space: 
   i64BAR.QuadPart = GetPciBaseAddressReg( 0 ) & 0xfffffff0;            //    BAR0 = start of PCI regs (physical addr) 
   m_pvulPCIRegs = (PULONG) ::RtMapMemory( i64BAR, PCIREGSZ, MmNonCached ); 
   bOk = BOOL(m_pvulPCIRegs != NULL);

   if( bOk )                                                            // set up runtime mailbox reg 2 as the 
      m_pvulCmdStatReg = &(m_pvulPCIRegs[PCIMBOX2]);                    // "command/status" reg for XYCORE

   if( bOk )                                                            // map "bootload area" of Dakar's FGSRAM into 
   {                                                                    // process space...
      i64BAR.QuadPart = GetPciBaseAddressReg( 2 ) & 0xfffffff0;         //    BAR2 + offset = start of bootload area
      i64BAR.QuadPart += BOOTAREAOFFSET;
      m_pvulBootLoad = (PULONG) ::RtMapMemory( i64BAR, BOOTAREASZ, MmNonCached ); 
      bOk = BOOL(m_pvulBootLoad != NULL);
   }

   if( bOk )                                                            // map "shared data" area in Dakar's FGSRAM 
   {                                                                    // into process space...
      int iLen = sizeof( CCxScope::Parameters ) +                       //    #bytes needed for downloading tgt params 
                 MAX_TARGETS * sizeof( CCxScope::UpdateRec ) + 1000;    //    and motion update records, w/some extra 
      iLen *= 2;                                                        //    twice the memory is required, b/c Dakar 
                                                                        //    only works with 32bit ints!!

      i64BAR.QuadPart = GetPciBaseAddressReg( 2 ) & 0xfffffff0;         //    BAR2 + offset = start of FGSRAM (AFTER 
      i64BAR.QuadPart += FGSRAMOFFSET;                                  //    the bootload area)
      m_pvulFgsram = (PULONG) ::RtMapMemory( i64BAR, (ULONG)iLen, MmNonCached ); 
      bOk = BOOL(m_pvulFgsram != NULL);
   }

   if( bOk )                                                            // map Node A Int Ctrl reg bank into process 
   {                                                                    // space...
      i64BAR.QuadPart = GetPciBaseAddressReg( 2 ) & 0xfffffff0;         //    BAR2 + offset = start of IC reg bank 
      i64BAR.QuadPart += INTREGOFFSET;
      m_pvulIntRegs = (PULONG) ::RtMapMemory( i64BAR, INTREGSZ, MmNonCached ); 
      bOk = BOOL(m_pvulIntRegs != NULL);
   }

   if( !bOk ) UnmapDeviceResources();                                   // on failure, unmap any resources that WERE 
                                                                        // successfully mapped.
   return( bOk );
}

VOID RTFCNDCL CXYDakar::UnmapDeviceResources()
{
   if( m_pvulIntRegs != NULL ) 
   {
      ::RtUnmapMemory( m_pvulIntRegs );
      m_pvulIntRegs = NULL;
   }

   if( m_pvulFgsram != NULL ) 
   {
      ::RtUnmapMemory( m_pvulFgsram );
      m_pvulFgsram = NULL;
   }

   if( m_pvulBootLoad != NULL ) 
   {
      ::RtUnmapMemory( m_pvulBootLoad );
      m_pvulBootLoad = NULL;
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
//    Here we merely perform a sanity check to ensure we've established a connection with the Dakar:  we merely verify 
//    that we can write and read to/from the Dakar's FGSRAM.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDakar::OnOpen()
{
   if( !IsOn() ) return( FALSE );

   BOOL bCheckOK = TRUE;

   int i;                                                            // write to FGSRAM
   for( i = 0; i < PARAMS_SIZE32; i++ ) m_pvulFgsram[i] = (DWORD) i; 

   i = PARAMS_SIZE32-1;                                              // read it back in reverse order 
   while( (i >= 0) && bCheckOK )                                     // verifying contents...
   {
      DWORD dwTemp = m_pvulFgsram[i];
      bCheckOK = BOOL(dwTemp == (DWORD) i);
      i--;
   }

   if( !bCheckOK ) SetDeviceError( CDevice::EMSG_VERIFYFAIL );       // sanity check failed!

   return( bCheckOK );
}


//=== DownloadCOFFData [base override] ================================================================================ 
//
//    Download a section of COFF data into the program, data, or other memory resource associated with a TMS320C4x or 
//    C6x DSP onboard the device.  Intended for loading a core program onto a DSP node during boot mode (CPU in reset).
//
//    Downloading user code to the embedded Node A on the Dakar board requires a "bootloader" program running on the 
//    node's C44 processor.  This root node may be booted via the onboard PEROM (jumper J28 installed) or via its COMM 
//    port (jumper J28 removed).  This method uses the first scheme; if jumper J28 is not installed, it will fail! 
//
//    When the Dakar is reset, we assume that the bootloader routine in PEROM is automatically loaded onto the embedded 
//    C44 processor (see DeviceReset()).  This function works hand-in-hand with that routine to download the user code 
//    onto the C44.  Here is an attempt to explain how it works:
// 
//       1) The region 0xC0300000-0xC03008FF (C4x-local address, each referring to a 4-byte mem loc) in "Far Global 
//    SRAM" (FGSRAM) is dedicated to the bootload.  Hereafter we refer to this dedicated region as the "bootload area"; 
//    DSP programs should be designed to avoid this region.  One ULONG in the bootload area serves as a flag for 
//    handshaking between the bootloader and the host process that is downloading user code (i.e., this function).  
//    Another region serves as a DMA autoinit header, which provides information to auto-initiate a DMA operation that 
//    transfers the user code from the bootload area to Node A's internal program RAM.  Finally, there's a 2048-DWORD 
//    section where the host process transfers the user code a section at a time.
//
//       2) When Node A's bootloader starts, it performs some init tasks: (a) sets the host/dsp handshaking flag in 
//    the bootload area to 1, indicating that bootloader is ready to accept more user code; (b) sets up the C44's DMA 
//    hardware to autoinitialize using the header info in the bootload area; and (c) installs the bootloader ISR and 
//    enables the PCI-->Node A interrupt.
// 
//       3) After these inits, the main program begins.  All it does is branch unconditionally to an address held in 
//    another dedicated C4x address, 0x002FFFF0 (C4X_USRENTRY_ADDR).  Initially, things are set up this way: 
//             start:   .word main        // start = 0x002FFFF0
//             ...
//             main:    LDI @start, AR0   // AR0 = *start;
//                      BU AR0            // jump unconditionally to address *start
//    Thus, the main program merely loops indefinitely.
//
//       4) To download a section of user code, the host first waits for the handshaking flag to be set to 1, and then 
//    resets the flag.  Then it prepares the DMA autoinit header and writes the section of user code to the appropriate 
//    block in the bootload area.  Finally, it issues a PCI-->Node A interrupt, which awakens the bootloader ISR.
//    bootloader ISR. 
//
//       5) The bootloader ISR clears the interrupt, starts the DMA, and waits for the DMA transfer to be completed. 
//    It then sets the handshaking flag to 1, telling the host that it's ready for more.
//
//       6) Once all of the user code has been downloaded to the node's internal RAM, one last DMA transfer is used to 
//    start the program.  In this transfer, the DMA overwrites the main program's "start" address with the user code 
//    entry point.  Thus, the bootloader's main program branches to and begins executing the downloaded program!  See 
//    also DeviceStart().
//
//    This method prepares and initiates the DMA operations that are outlined in steps (4) and (6) above.
//
//    NOTES:   -- Since the bootload area consumes the first 0x880 = 2304 ULONGs of FGSRAM, DSP programs should be 
//                designed to avoid this section of FGSRAM.
//             -- We implicitly assume that ULONG = 32bits, the C4x local bus data size!
//
//    ARGS:       devAddr  -- [in] device address at which to start download.
//                pData    -- [in] buffer containing COFF data to download (as 32-bit data).
//                iLen     -- [in] # of 32-bit words to download.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.  Fails if requested DMA block write is too large for the 
//                bootload scheme, or if we timeout waiting for previous DMA op to end.
//
BOOL RTFCNDCL CXYDakar::DownloadCOFFData( DWORD devAddr, PDWORD pData, int iLen )
{
   if( m_pvulBootLoad == NULL || m_pvulIntRegs == NULL )       // cannot download data without necessary resources!
      return( FALSE );

   if( iLen > BOOT_CTLBLK_SZ ) return( FALSE );                // fail if COFF data block size is too large to handle 

   CElapsedTime eTime;                                         // wait for previous bootloader DMA op to finish...
   while( (eTime.Get() < BOOT_TIMEOUTUS) &&
          (m_pvulBootLoad[BOOT_DMADONE] != 1) ) ;
   if( m_pvulBootLoad[BOOT_DMADONE] != 1 ) return( FALSE );    // ...but give up if we have to wait too long!

   m_pvulBootLoad[BOOT_DMADONE] = 0;                           // last DMA finished OK; clear handshake flag

                                                               // set up autoinit header for the next DMA transfer:
   int i = BOOT_DMAHDR;                                        //    DMA control reg -- value is different for the 
   if( devAddr == C4X_USRENTRY_ADDR && iLen == 1 )             //    final transfer that sets user code entry point... 
      m_pvulBootLoad[i] = BOOT_DMAHDR_LAST;
   else
      m_pvulBootLoad[i] = BOOT_DMAHDR_CTL;
   ++i;

   m_pvulBootLoad[i++] = C4X_SRAM_ADDR +                       //    C4x-local addr for "source" -- which is always the 
                         (DWORD)BOOT_CTLBLK;                   //    designated block in the bootload area of FGSRAM 
   m_pvulBootLoad[i++] = 1;                                    //    source index: 1 ==> src addr is local C4x
   m_pvulBootLoad[i++] = (DWORD) iLen;                         //    # of 32bit words to transfer
   m_pvulBootLoad[i++] = devAddr;                              //    C4x-local addr for "destination"
   m_pvulBootLoad[i++] = 1;                                    //    dest index: 1 ==> dest addr is local C4x
   m_pvulBootLoad[i] = C4X_SRAM_ADDR + (DWORD)BOOT_DMAHDR;     //    always set to local C4x addr of this hdr in FGSRAM 

   i = BOOT_CTLBLK;                                            // write user code to bootloader data block in FGSRAM
   for( int j = 0; j < iLen; j++ ) 
      m_pvulBootLoad[i+j] = pData[j];

   m_pvulIntRegs[INTPCI2A] = 1;                                // start DMA transfer by interrupting PEROM bootloader 

   return( TRUE );
}


//=== DeviceReset [base override] ===================================================================================== 
//
//    Reset the device; if it has a TIC6x/C4x DSP, the DSP's CPU should be left in a suspended state in preparation for 
//    COFF download.
//
//    Upon reset, a PEROM bootloader program is automatically downloaded onto the Dakar's embedded C44 "Node A" -- 
//    leaving the device ready for COFF downloading via the bootload area in FGSRAM.  See DownloadCOFFData() for a 
//    detailed explanation of the bootload process.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL RTFCNDCL CXYDakar::DeviceReset()
{
   if( m_pvulPCIRegs == NULL ) return( FALSE );          // cannot reset without access to PCI registers!

   DWORD dwBitState = m_pvulPCIRegs[PCICNTRL];           // software reset: assert reset bit in PCI9060's PCI Control 
   dwBitState |= PCICNTRL_SWRESET;                       // register for at least 10 ms.
   m_pvulPCIRegs[PCICNTRL] = dwBitState;
   ::Sleep( 10 );
   dwBitState &= ~PCICNTRL_SWRESET;
   m_pvulPCIRegs[PCICNTRL] = dwBitState;
   ::Sleep( 10 );

   dwBitState = m_pvulPCIRegs[PCICNTRL];                 // reload PCI cfg regs from EEPROM: assert reload bit in PCI 
   dwBitState |= PCICNTRL_RELCFG;                        // Control reg.  since prev reset guarantees that reload bit 
   m_pvulPCIRegs[PCICNTRL] = dwBitState;                 // is 0, this code will provide the required 0->1 transition 
   ::Sleep( 10 );
   dwBitState &= ~PCICNTRL_RELCFG;
   m_pvulPCIRegs[PCICNTRL] = dwBitState;

   return( TRUE );
}


//=== DeviceStart [base override] ===================================================================================== 
//
//    Start the previously downloaded COFF executable on the device's TIC6x or C4x DSP and verify that it's running.
//
//    In the case of the Dakar, we start XYCORE merely by writing its "entry point" to a special location in the C44 
//    node's memory (see DownloadCOFFData() for a detailed description of the bootload process).  We then merely wait 
//    for XYCORE to signal its readiness to accept commands.
//
//    ARGS:       dwEntry  -- [in] the local address of the entry point for the COFF executable
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL RTFCNDCL CXYDakar::DeviceStart( DWORD dwEntry )
{
   if( !DownloadCOFFData( C4X_USRENTRY_ADDR, &dwEntry, 1 ) )      // load entry point to start XYCORE
      return( FALSE );

   CElapsedTime eTime;                                            // give it time to start up
   while( (!IsReady()) && (eTime.Get() < XYDEV_TIMEOUT) ) ;

   return( IsReady() );
}


//=== DeviceQuit [base override] ====================================================================================== 
//
//    Cause the target COFF executable on a TIC6x- or C4x-based device to stop running.  The device will become 
//    unavailable upon invoking this method. 
//
//    Here, we merely reset the Dakar and leave it in that state.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CXYDakar::DeviceQuit()
{
   DeviceReset();
}


//=== LoadParameters [base override] ================================================================================== 
//
//    Download the current target definitions & animation parameters to the XY scope device, then issue a command to 
//    read all parameters and prepare targets for subsequent frame-by-frame animation.
//
//    !!!!!!!!!! IMPORTANT !!!!!!!!!!
//    The Dakar is somewhat unusual because its fundamental unit of memory is not a byte, but a 4-byte word.  The 
//    CCxScope::m_Parameters structure contains mostly 2-byte words.  If we performed a byte-to-byte copy from the host 
//    to Dakar's shared memory area, the XYCORE program running on the Dakar would have a difficult time parsing the 
//    data.  Instead, we write each 16-bit or 32-bit member of the m_Parameters structure as a separate 32-bit word in 
//    the "shared data" area in the Dakar's FGSRAM.  It is essential to preserve the order of the members!!
//    !!!!!!!!!! IMPORTANT !!!!!!!!!!
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CXYDakar::LoadParameters()
{
   CElapsedTime eTime;                                               // wait for device ready
   while( (!IsReady()) && (eTime.Get() < XYDEV_TIMEOUT) ) ;
   if( !IsReady() ) return( FALSE );

   int i = 0;                                                        // copy individual fields of parameters struct, 
   m_pvulFgsram[i++] = (DWORD) m_Parameters.dwDotSeed;               // in order, to consecutive DWORD locations in 
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wWidthMM;                // the Dakar's FGSRAM, starting at loc 0...
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wHeightMM;               // (see details in fcn header)
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wDistMM;
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wNumTargets;
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wDelayPerDot;
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wOnTimePerDot;
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wFiller[0];
   m_pvulFgsram[i++] = (DWORD) m_Parameters.wFiller[1];
   PWORD pField = &(m_Parameters.wType[0]);
   while( i < PARAMS_SIZE32 ) m_pvulFgsram[i++] = (DWORD) *pField++;

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
//    Per-target display frame update records are stored in the Dakar's FGSRAM immediately after the data structure 
//    (CCxScope::Parameters) that holds target definition and animation parameters.  The records are stored in the 
//    order in which targets were defined -- this is mandated by the CCxScope interface.
//
//    !!!!!!!!!! IMPORTANT !!!!!!!!!!
//    The Dakar is somewhat unusual because its fundamental unit of memory is not a byte, but a 4-byte word.  Each 
//    per-target update record (CCxScope::UpdateRec) is a set of 5 2-byte integers.  For this record to be interpreted 
//    correctly by XYCORE on the Dakar, we must copy each 2-byte field to consecutive DWORD locations in FGSRAM.  Also, 
//    we must start writing the update records beginning with location PARAMS_SIZE32 -- to ensure that we do not 
//    overwrite the CCxScope::Parameters structure that is downloaded by LoadParameters().
//    !!!!!!!!!! IMPORTANT !!!!!!!!!!
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (XY scope dev not ready).
//
BOOL RTFCNDCL CXYDakar::DoFrame()
{
   if( !IsReady() ) return ( FALSE );                          // XYCORE not ready -- "dropped frame"

   PSHORT pData = (PSHORT) &(m_NextUpdate[0]);                 // start of update records array, as array of shorts
   int n = int( m_Parameters.wNumTargets );                    // # of shorts that we need to download as DWORDs
   n *= sizeof( CCxScope::UpdateRec );                         // (see notes in fcn header!)
   n /= sizeof( short );
   
   for( int i = 0; i < n; i++ )                                // download update records into DWORD locations
      m_pvulFgsram[PARAMS_SIZE32 + i] = (DWORD) pData[i];

   *m_pvulCmdStatReg = XYCORE_DOFRAME;                         // tell XYCORE to start update 
   return ( TRUE );
}

