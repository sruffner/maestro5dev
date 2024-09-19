/*SDOC-------------------------------------------------------------------------
 * Spectrum Signal Processing Inc. Copyright 1998
 *
 * $Workfile:: ssp_c6x.h                                                      $
 *
 * $Modtime:: 3/09/98 10:06a                                                  $
 * $Revision:: 7                                                              $
 * $Archive:: /MAC/MAC Modules/dev/shared/include/ssp_c6x.h                   $
 *
 * Contents:  Header file for the C6x chip.
 *            Contains only register addresses and bit fields.
 *
 *            Register offsets are defined as C6X_REGNAME_REG.
 *            Bit masks are defined as C6X_REGNAME_FIELDNAME_MASK.
 *            Single bits are defined as C6X_REGNAME_BITNAME.
 *            If a register only contains one field the mask for the entire
 *            register is C6X_REGNAME_MASK.  Bit, register and field names
 *            all match or are short forms of the names in TI C6x manuals.
 *
 *-----------------------------------------------------------------------EDOC*/


#ifndef SSP_C6X_H
#define SSP_C6X_H

/* local includes -----------------------------------------------------------*/
#include "sstype.h"

/*----------------- C6x Memory  ---------------------------------------------*/
#define C6201_INT_PRAM_START        0x00000000
#define C6201_INT_PRAM_START_ADDR   (UINT32 *)C6201_INT_PRAM_START
#define C6201_INT_DRAM_START        0x80000000
#define C6201_INT_DRAM_START_ADDR   (UINT32 *)C6201_INT_DRAM_START
#define C6201_INT_DRAM_SIZE         0x00002000

/*----------------- C6x Register Address/Pointer Definitions ----------------*/

/* External Memory InterFace registers
 */
#define C6X_EMIF_GCR_PTR         (volatile UINT32 *)0x01800000
#define C6X_EMIF_CE1CR_PTR       (volatile UINT32 *)0x01800004
#define C6X_EMIF_CE0CR_PTR       (volatile UINT32 *)0x01800008
#define C6X_EMIF_CE2CR_PTR       (volatile UINT32 *)0x01800010
#define C6X_EMIF_CE3CR_PTR       (volatile UINT32 *)0x01800014
#define C6X_EMIF_SDCR_PTR        (volatile UINT32 *)0x01800018
#define C6X_EMIF_SDTIM_PTR       (volatile UINT32 *)0x0180001c

/* Direct Memory Addressing registers */
#define C6X_DMA_CH0_PRIMCR_PTR   (volatile UINT32 *)0x01840000
#define C6X_DMA_CH2_PRIMCR_PTR   (volatile UINT32 *)0x01840004
#define C6X_DMA_CH0_SCNDCR_PTR   (volatile UINT32 *)0x01840008
#define C6X_DMA_CH2_SCNDCR_PTR   (volatile UINT32 *)0x0184000c
#define C6X_DMA_CH0_SRC_PTR      (volatile UINT32 *)0x01840010
#define C6X_DMA_CH2_SRC_PTR      (volatile UINT32 *)0x01840014
#define C6X_DMA_CH0_DST_PTR      (volatile UINT32 *)0x01840018
#define C6X_DMA_CH2_DST_PTR      (volatile UINT32 *)0x0184001c
#define C6X_DMA_CH0_CNT_PTR      (volatile UINT32 *)0x01840020
#define C6X_DMA_CH2_CNT_PTR      (volatile UINT32 *)0x01840024
#define C6X_DMA_RELDA_PTR        (volatile UINT32 *)0x01840028
#define C6X_DMA_RELDB_PTR        (volatile UINT32 *)0x0184002c
#define C6X_DMA_INDXA_PTR        (volatile UINT32 *)0x01840030
#define C6X_DMA_INDXB_PTR        (volatile UINT32 *)0x01840034
#define C6X_DMA_ADDRA_PTR        (volatile UINT32 *)0x01840038
#define C6X_DMA_ADDRB_PTR        (volatile UINT32 *)0x0184003c
#define C6X_DMA_CH1_PRIMCR_PTR   (volatile UINT32 *)0x01840040
#define C6X_DMA_CH3_PRIMCR_PTR   (volatile UINT32 *)0x01840044
#define C6X_DMA_CH1_SCNDCR_PTR   (volatile UINT32 *)0x01840048
#define C6X_DMA_CH3_SCNDCR_PTR   (volatile UINT32 *)0x0184004c
#define C6X_DMA_CH1_SRC_PTR      (volatile UINT32 *)0x01840050
#define C6X_DMA_CH3_SRC_PTR      (volatile UINT32 *)0x01840054
#define C6X_DMA_CH1_DST_PTR      (volatile UINT32 *)0x01840058
#define C6X_DMA_CH3_DST_PTR      (volatile UINT32 *)0x0184005c
#define C6X_DMA_CH1_CNT_PTR      (volatile UINT32 *)0x01840060
#define C6X_DMA_CH3_CNT_PTR      (volatile UINT32 *)0x01840064
#define C6X_DMA_ADDRC_PTR        (volatile UINT32 *)0x01840068
#define C6X_DMA_ADDRD_PTR        (volatile UINT32 *)0x0184006c
#define C6X_DMA_AUXCR_PTR        (volatile UINT32 *)0x01840070

/* C6X_DMA_CHx_PRIMCR: Shifts for the bitfields in this register */
#define C6X_DMA_CHx_PRIMCR_START_SHIFT       0
#define C6X_DMA_CHx_PRIMCR_STATUS_MASK       0x0000000c
#define C6X_DMA_CHx_PRIMCR_STATUS_SHIFT      2
#define C6X_DMA_CHx_PRIMCR_SRCDIR_SHIFT      4
#define C6X_DMA_CHx_PRIMCR_DSTDIR_SHIFT      6
#define C6X_DMA_CHx_PRIMCR_ESIZE_SHIFT       8
#define C6X_DMA_CHx_PRIMCR_SPLIT_SHIFT       10
#define C6X_DMA_CHx_PRIMCR_RSYNC_SHIFT       14
#define C6X_DMA_CHx_PRIMCR_WSYNC_SHIFT       19
#define C6X_DMA_CHx_PRIMCR_SRCRELOAD_SHIFT   28
#define C6X_DMA_CHx_PRIMCR_DSTRELOAD_SHIFT   30

/* Host Port Interface register */
#define C6X_HPIC_PTR             (volatile UINT32 *)0x01880000

/* Multi-Channel Serial Port registers */
#define C6X_MCSP0_DRR_PTR        (volatile UINT32 *)0x018c0000
#define C6X_MCSP0_DXR_PTR        (volatile UINT32 *)0x018c0004
#define C6X_MCSP0_SPCR_PTR       (volatile UINT32 *)0x018c0008
#define C6X_MCSP0_RCR_PTR        (volatile UINT32 *)0x018c000c
#define C6X_MCSP0_XCR_PTR        (volatile UINT32 *)0x018c0010
#define C6X_MCSP0_SRGR_PTR       (volatile UINT32 *)0x018c0014
#define C6X_MCSP0_MCR_PTR        (volatile UINT32 *)0x018c0018
#define C6X_MCSP0_RCER_PTR       (volatile UINT32 *)0x018c001c
#define C6X_MCSP0_XCER_PTR       (volatile UINT32 *)0x018c0020
#define C6X_MCSP0_PCR_PTR        (volatile UINT32 *)0x018c0024
#define C6X_MCSP1_DRR_PTR        (volatile UINT32 *)0x01900000
#define C6X_MCSP1_DXR_PTR        (volatile UINT32 *)0x01900004
#define C6X_MCSP1_SPCR_PTR       (volatile UINT32 *)0x01900008
#define C6X_MCSP1_RCR_PTR        (volatile UINT32 *)0x0190000c
#define C6X_MCSP1_XCR_PTR        (volatile UINT32 *)0x01900010
#define C6X_MCSP1_SRGR_PTR       (volatile UINT32 *)0x01900014
#define C6X_MCSP1_MCR_PTR        (volatile UINT32 *)0x01900018
#define C6X_MCSP1_RCER_PTR       (volatile UINT32 *)0x0190001c
#define C6X_MCSP1_XCER_PTR       (volatile UINT32 *)0x01900020
#define C6X_MCSP1_PCR_PTR        (volatile UINT32 *)0x01900024

/* C6X_MCSPx_SPCR register bit definitions */
#define C6X_MCSPx_SPCR_RRST      0x00000001  /* Active low */
#define C6X_MCSPx_SPCR_RRDY      0x00000002
#define C6X_MCSPx_SPCR_RFULL     0x00000004
#define C6X_MCSPx_SPCR_RSYNCERR  0x00000008
#define C6X_MCSPx_SPCR_XRST      0x00010000  /* Active low */
#define C6X_MCSPx_SPCR_XRDY      0x00020000
#define C6X_MCSPx_SPCR_XEMPTY    0x00040000  /* Active low */
#define C6X_MCSPx_SPCR_XSYNCERR  0x00080000
#define C6X_MCSPx_SPCR_GRST      0x00400000  /* Active low */
#define C6X_MCSPx_SPCR_FRST      0x00800000  /* Active low */

/* C6X_MCSPx_PCR register bit definitions */
#define C6X_MCSPx_PCR_CLKRP      0x00000001
#define C6X_MCSPx_PCR_CLKXP      0x00000002
#define C6X_MCSPx_PCR_FSRP       0x00000004
#define C6X_MCSPx_PCR_FSXP       0x00000008
#define C6X_MCSPx_PCR_DR_STAT    0x00000010
#define C6X_MCSPx_PCR_DX_STAT    0x00000020
#define C6X_MCSPx_PCR_CLKS_STAT  0x00000040
#define C6X_MCSPx_PCR_CLKRM      0x00000100
#define C6X_MCSPx_PCR_CLKXM      0x00000200
#define C6X_MCSPx_PCR_FSRM       0x00000400
#define C6X_MCSPx_PCR_FSXM       0x00000800
#define C6X_MCSPx_PCR_RIOEN      0x00001000
#define C6X_MCSPx_PCR_XIOEN      0x00002000

/* C6X_MCSPx_RCR and C6X_MCSPx_XCR register bit definitions */
#define C6X_MCSPx_XRCR_WDLEN1_MASK     0x000000e0
#define C6X_MCSPx_XRCR_WDLEN1_SHIFT    5
#define C6X_MCSPx_XRCR_FRLEN1_MASK     0x00007f00
#define C6X_MCSPx_XRCR_FRLEN1_SHIFT    8
#define C6X_MCSPx_XRCR_DATDLY_MASK     0x00030000
#define C6X_MCSPx_XRCR_DATDLY_SHIFT    16
#define C6X_MCSPx_XRCR_FIG             0x00040000
#define C6X_MCSPx_XRCR_COMPAND_MASK    0x00180000
#define C6X_MCSPx_XRCR_COMPAND_SHIFT   19
#define C6X_MCSPx_XRCR_WDLEN2_MASK     0x00e00000
#define C6X_MCSPx_XRCR_WDLEN2_SHIFT    21
#define C6X_MCSPx_XRCR_FRLEN2_MASK     0x7f000000
#define C6X_MCSPx_XRCR_FRLEN2_SHIFT    24
#define C6X_MCSPx_XRCR_PHASE           0x80000000

/* C6X_MCSPx_SRGR register bit definitions */
#define C6X_MCSPx_SRGR_CLKGDV_MASK  0x000000ff
#define C6X_MCSPx_SRGR_CLKGDV_SHIFT 0
#define C6X_MCSPx_SRGR_FWID_MASK    0x0000ff00
#define C6X_MCSPx_SRGR_FWID_SHIFT   8
#define C6X_MCSPx_SRGR_FPER_MASK    0x0fff0000
#define C6X_MCSPx_SRGR_FPER_SHIFT   16
#define C6X_MCSPx_SRGR_FSGM         0x10000000
#define C6X_MCSPx_SRGR_CLKSM        0x20000000
#define C6X_MCSPx_SRGR_CLKSP        0x40000000
#define C6X_MCSPx_SRGR_GSYNC        0x80000000


/* Timer registers */
#define C6X_TIMER0_TCR                     0x01940000
#define C6X_TIMER0_TCR_PTR       (volatile UINT32 *)0x01940000
#define C6X_TIMER0_PER                     0x01940004
#define C6X_TIMER0_PER_PTR       (volatile UINT32 *)0x01940004
#define C6X_TIMER0_CNT                     0x01940008
#define C6X_TIMER0_CNT_PTR       (volatile UINT32 *)0x01940008
#define C6X_TIMER1_TCR                     0x01980000
#define C6X_TIMER1_TCR_PTR       (volatile UINT32 *)0x01980000
#define C6X_TIMER1_PER                     0x01980004
#define C6X_TIMER1_PER_PTR       (volatile UINT32 *)0x01980004
#define C6X_TIMER1_CNT                     0x01980008
#define C6X_TIMER1_CNT_PTR       (volatile UINT32 *)0x01980008

/* C6X_TIMERx_TCR Register Bitfields */
#define C6X_TIMERx_TCR_FUNC      0x00000001
#define C6X_TIMERx_TCR_INVOUT    0x00000002
#define C6X_TIMERx_TCR_DATOUT    0x00000004
#define C6X_TIMERx_TCR_DATIN     0x00000008
#define C6X_TIMERx_TCR_PWID      0x00000010
#define C6X_TIMERx_TCR_GO        0x00000040
#define C6X_TIMERx_TCR_HLD       0x00000080
#define C6X_TIMERx_TCR_CP        0x00000100
#define C6X_TIMERx_TCR_CLKSRC    0x00000200
#define C6X_TIMERx_TCR_INVINP    0x00000400
#define C6X_TIMERx_TCR_TSTAT     0x00000800

/* Interrupt registers */
#define C6X_INT_MULTHIGH_PTR     (volatile UINT32 *)0x019c0000
#define C6X_INT_MULTLOW_PTR      (volatile UINT32 *)0x019c0004
#define C6X_INT_EXTPOL_PTR       (volatile UINT32 *)0x019c0008

// C6X_IxR: bit definitions for IER (enable), IFR (flag)
// These bits are common to all C6x devices
#define C6X_IxR_NMI              0x00000002  // NMI bit - must be set to enable other irqs

/* C6X_INT_MULTLOW: Interrupt Multiplexer Low Register bitfields */
#define C6X_INT_MULTLOW_INTSEL4  0x0000000f  
#define C6X_INT_MULTLOW_INTSEL5  0x000001e0
#define C6X_INT_MULTLOW_INTSEL6  0x00003c00
#define C6X_INT_MULTLOW_INTSEL7  0x000f0000
#define C6X_INT_MULTLOW_INTSEL8  0x01e00000
#define C6X_INT_MULTLOW_INTSEL9  0x3c000000
#define C6X_INT_MULTLOW_DEFAULT  0x250718a4

/* C6X_INT_MULTHIGH: Interrupt Multiplexer High Register bitfields */
#define C6X_INT_MULTHIGH_INTSEL10 0x0000000f  
#define C6X_INT_MULTHIGH_INTSEL11 0x000001e0
#define C6X_INT_MULTHIGH_INTSEL12 0x00003c00
#define C6X_INT_MULTHIGH_INTSEL13 0x000f0000
#define C6X_INT_MULTHIGH_INTSEL14 0x01e00000
#define C6X_INT_MULTHIGH_INTSEL15 0x3c000000
#define C6X_INT_MULTHIGH_DEFAULT  0x08202d43

/* C6X_INT_EXTPOL: External Interrupt Polarity Register bitfields */
#define C6X_INT_EXTPOL_XIP4   0x00000001
#define C6X_INT_EXTPOL_XIP5   0x00000002
#define C6X_INT_EXTPOL_XIP6   0x00000004
#define C6X_INT_EXTPOL_XIP7   0x00000008

// C6201_IxR: bit definitions for ICR (clear), IER (enable), IFR (flag), ISR (set) registers
// These bits are C6201 specific as the C6201 cpu maps these to 12 generic CPU inputs
#define C6201_IxR_EXT_INT4       0x00000010  // INTSEL4 of CPU - external int line 4
#define C6201_IxR_EXT_INT5       0x00000020  // INTSEL5 of CPU - external int line 5
#define C6201_IxR_EXT_INT6       0x00000040  // INTSEL6 of CPU - external int line 6
#define C6201_IxR_EXT_INT7       0x00000080  // INTSEL7 of CPU - external int line 7
#define C6201_IxR_DMA_INT0       0x00000100  // INTSEL8 of CPU - DMA Channel 0 int
#define C6201_IxR_DMA_INT1       0x00000200  // INTSEL9 of CPU - DMA Channel 1 int
#define C6201_IxR_SD_INT         0x00000400  // INTSEL10 of CPU - EMIF SDRAM Timer int
#define C6201_IxR_DMA_INT2       0x00000800  // INTSEL11 of CPU - DMA Channel 2 int
#define C6201_IxR_DMA_INT3       0x00001000  // INTSEL12 of CPU - DMA Channel 3 int
#define C6201_IxR_DSPINT         0x00002000  // INTSEL13 of CPU - Host port - Host to DSP int
#define C6201_IxR_TINT0          0x00004000  // INTSEL14 of CPU - Timer interrupt 0
#define C6201_IxR_TINT1          0x00008000  // INTSEL15 of CPU - Timer interrupt 1
#define C6201_IxR_ALL_BITS       0x0000fff0

/* C6X CPU register definitions ============================================
   The following definitions allow these registers to be modified just like
   normally variables */
#if (defined(_TMS320C6X))  /* Only used by C6x compiler */
extern cregister volatile unsigned int AMR;  // Addressing mode register
extern cregister volatile unsigned int CSR;  // Control Status register
extern cregister volatile unsigned int ICR;  // Interrupt Clear register (write only)
extern cregister volatile unsigned int IER;  // Interrupt Enable register
extern cregister volatile unsigned int IFR;  // Interrupt Flag register (read only)
extern cregister volatile unsigned int IN;   // General Purpose Input register (read only)
extern cregister volatile unsigned int ISR;  // Interrupt Set register (write only)
extern cregister volatile unsigned int OUT;  // General Purpose Output register
extern cregister volatile unsigned int ISTP; // General Purpose Output register
extern cregister volatile unsigned int NRP;  // General Purpose Output register
extern cregister volatile unsigned int IRP;  // General Purpose Output register
#endif

/* C6X_CSR: Control Status Register bitfields */
#define C6X_CSR_GIE           0x00000001  // Global Interrupt enable 1=enabled
#define C6X_CSR_PGIE          0x00000002  // Previous GIE value
#define C6X_CSR_DCC           0x0000001c  // Data cache control mode
#define C6X_CSR_PCC           (0x3<<5)    // Programmable cache control mode
#define C6X_CSR_EN            0x00000100  // Endian bit 1=Little
#define C6X_CSR_SAT           0x00000200  // Saturate bit - set when saturate occurs
#define C6X_CSR_PWRD          0x0000fc00  // Power down mode bits
#define C6X_CSR_REVISION_ID   0x00ff0000  // Revision ID of silicon
#define C6X_CSR_CPU_ID        0xff000000  // CPU ID

// C6X_CSR_PWRD: Control Status Register Power Down bitfields
#define C6X_CSR_PWRD_NONE        0x00000000  // No power down
#define C6X_CSR_PWRD_WAKE_ENINT  0x00002400  // Power down wake by enabled int
#define C6X_CSR_PWRD_WAKE_ANYINT 0x00004400  // Power down wake by any int
#define C6X_CSR_PWRD_WAKE_PD2    0x00006800  // Power down wake by reset
#define C6X_CSR_PWRD_WAKE_PD3    0x00007000  // Power down wake by reset

#define C6X_CSR_PCC_MEM_MAP      (0x00<<5)   // Memory mapped. Cache disabled
#define C6X_CSR_PCC_CACHE_ENABLE (0x02<<5)   // Cache updated and accessed on reads
#define C6X_CSR_PCC_CACHE_FREEZE (0x03<<5)   // Cache accessed but not updated on reads
#define C6X_CSR_PCC_CACHE_BYPASS (0x04<<5)   // Cache not accessed or updated on reads

/*--------------------- C6x Register Bitfield Definitions -------------------*/
/* C6X_EMIF_GCR:  EMIF Global Control Register bitfields */
#define C6X_EMIF_GCR_MAP       0x00000001 /* Bit 0: Memory map mode of C6x   */
#define C6X_EMIF_GCR_RBTR8     0x00000002 /* Bit 1: Requester arb mode       */
#define C6X_EMIF_GCR_SSCRT     0x00000004 /* Bit 2: SBSRAM clock rate sel    */
#define C6X_EMIF_GCR_CLK2EN    0x00000008 /* Bit 3: CLKOUT2 enable           */
#define C6X_EMIF_GCR_CLK1EN    0x00000010 /* Bit 4: CLKOUT1 enable           */
#define C6X_EMIF_GCR_SSCEN     0x00000020 /* Bit 5: SSCLK enable             */
#define C6X_EMIF_GCR_SDCEN     0x00000040 /* Bit 6: SDCLK enable             */
#define C6X_EMIF_GCR_NOHOLD    0x00000080 /* Bit 7: External HOLD disable    */
#define C6X_EMIF_GCR_HOLDA     0x00000100 /* Bit 8: Value of /HOLDA output   */
#define C6X_EMIF_GCR_HOLD      0x00000200 /* Bit 9: Value of /HOLD input     */
#define C6X_EMIF_GCR_ARDY      0x00000400 /* Bit 10: Value of ARDY input     */
#define C6X_EMIF_GCR_CLK2INV   0x00001000 /* Bit 12: CLKOUT2 polarity        */
#define C6X_EMIF_GCR_SDCINV    0x00002000 /* Bit 13: SDCLK polarity          */

/* C6X_EMIF_CExCR:  EMIF CE (Chip Enable) Space Control Register bitfields
 * widths in CLKOUT1 cycles
 */
#define C6X_EMIF_CECR_RDHOLD_MASK    0x00000003 /* Bits 0-1: Hold width      */
#define C6X_EMIF_CECR_MTYPE_MASK     0x00000070 /* Bits 4-6: Memory type     */
#define C6X_EMIF_CECR_RDSTRB_MASK    0x00003f00 /* Bits 8-13: Strobe width   */
#define C6X_EMIF_CECR_TA_MASK        0x0000c000 /* Bits 14-15:               */
#define C6X_EMIF_CECR_RDSETUP_MASK   0x000f0000 /* Bits 16-19: Setup width   */
#define C6X_EMIF_CECR_WRHOLD_MASK    0x00300000 /* Bits 20-21: Hold width    */
#define C6X_EMIF_CECR_WRSTRB_MASK    0x0fc00000 /* Bits 22-27: Strobe width  */
#define C6X_EMIF_CECR_WRSETUP_MASK   0xf0000000 /* Bits 28-31: Setup width   */
#define C6X_EMIF_CECR_MTYPE_16ROM    0x00000010 /* MTYPE = 16-bit wide ROM   */
#define C6X_EMIF_CECR_MTYPE_32ASYNC  0x00000020 /* MTYPE = 32-bit asynch IF  */
#define C6X_EMIF_CECR_MTYPE_32SDRAM  0x00000030 /* MTYPE = 32-bit SDRAM      */
#define C6X_EMIF_CECR_MTYPE_32SBSRAM 0x00000040 /* MTYPE = 32-bit SBSRAM     */

/* C6X_EMIF_SDCR:  EMIF SDRAM Control Register bitfields
 * values in CLKOUT2 cycles
 */
#define C6X_EMIF_SDCR_TRC_MASK  0x0000f000 /* Bits 12-15: tRC-1 value        */
#define C6X_EMIF_SDCR_TRP_MASK  0x000f0000 /* Bits 16-19: tRP-1 value        */
#define C6X_EMIF_SDCR_TRCD_MASK 0x00f00000 /* Bits 20-23: tRCD-1 value       */
#define C6X_EMIF_SDCR_INIT      0x01000000 /* Bit 24: Force init of SDRAM    */
#define C6X_EMIF_SDCR_RFEN      0x02000000 /* Bit 25: Refresh enable         */
#define C6X_EMIF_SDCR_SDWTH     0x04000000 /* Bit 26: SDRAM width select     */

/* C6X_EMIF_SDTIM:  EMIF SDRAM Timing register bitfields
 * both values in CLKOUT2 cycles
 */
#define C6X_EMIF_SDTIM_PERIOD_MASK  0x00000fff /* Bits 0-11: Refresh per.    */
#define C6X_EMIF_SDTIM_COUNTER_MASK 0x00fff000 /* Bits 12-23: Refresh cntr.  */

/* C6X_DMA_CHx_PRIMCR:  DMA Channel x Primary Control Register bitfields
 */
#define C6X_DMA_CH_PRIMCR_START_MASK     0x00000003 /* Bits 0-1: DMA start ctl */
#define C6X_DMA_CH_PRIMCR_STATUS_MASK    0x0000000c /* Bits 2-3: DMA stat    */
#define C6X_DMA_CH_PRIMCR_SRCDIR_MASK    0x00000030 /* Bits 4-5: Source addr mod */
#define C6X_DMA_CH_PRIMCR_DSTDIR_MASK    0x000000c0 /* Bits 6-7: Destination addr mod*/
#define C6X_DMA_CH_PRIMCR_ESIZE_MASK     0x00000300 /* Bits 8-9: Element sz  */
#define C6X_DMA_CH_PRIMCR_SPLIT_MASK     0x00000c00 /* Bits 10-11: Split chan mode */
#define C6X_DMA_CH_PRIMCR_CNTRELOAD      0x00001000 /* Bit 12: DMA counter reload  */
#define C6X_DMA_CH_PRIMCR_INDEX          0x00002000 /* Bit 13: DMA glbl data index reg */
#define C6X_DMA_CH_PRIMCR_RSYNC_MASK     0x0007c000 /* Bits 14-18: Read xfer sync */
#define C6X_DMA_CH_PRIMCR_WSYNC_MASK     0x00f80000 /* Bits 19-23: Write xfer sync */
#define C6X_DMA_CH_PRIMCR_PRI            0x01000000 /* Bit 24: Priority mode (CPU vs DMA) */
#define C6X_DMA_CH_PRIMCR_TCINT          0x02000000 /* Bit 25: Transfer cntler int */
#define C6X_DMA_CH_PRIMCR_FS             0x04000000 /* Bit 26: Frame sync    */
#define C6X_DMA_CH_PRIMCR_EMOD           0x08000000 /* Bit 27: Emulation mode */
#define C6X_DMA_CH_PRIMCR_SRCRELOAD_MASK 0x30000000 /* Bits 28-29: DMA src addr reload */
#define C6X_DMA_CH_PRIMCR_DSTRELOAD_MASK 0xc0000000 /* Bits 30-31: DMA dest addr reload */

/* C6X_DMA_CHx_SCNDCR:  DMA Channel x Secondary Control Register bitfields
 */
#define C6X_DMA_CH_SCNDCR_SXCOND      0x00000001 /* Bit 0: DMA cond          */
#define C6X_DMA_CH_SCNDCR_SXIE        0x00000002 /* Bit 1: DMA cond int en   */
#define C6X_DMA_CH_SCNDCR_FRAMECOND   0x00000004 /* Bit 2: Frame cond        */
#define C6X_DMA_CH_SCNDCR_FRAMEIE     0x00000008 /* Bit 3: Frame cond int en */
#define C6X_DMA_CH_SCNDCR_LASTCOND    0x00000010 /* Bit 4: Last cond         */
#define C6X_DMA_CH_SCNDCR_LASTIE      0x00000020 /* Bit 5: Last cond int en  */
#define C6X_DMA_CH_SCNDCR_BLOCKCOND   0x00000040 /* Bit 6: Block cond        */
#define C6X_DMA_CH_SCNDCR_BLOCKIE     0x00000080 /* Bit 7: Block cond int en */
#define C6X_DMA_CH_SCNDCR_RDROPCOND   0x00000100 /* Bit 8: Read drop cond    */
#define C6X_DMA_CH_SCNDCR_RDROPIE     0x00000200 /* Bit 9: Read drop cond int en */
#define C6X_DMA_CH_SCNDCR_WDROPCOND   0x00000400 /* Bit 10: Write drop cond  */
#define C6X_DMA_CH_SCNDCR_WDROPIE     0x00000800 /* Bit 11: Write drop cond int en */
#define C6X_DMA_CH_SCNDCR_RSYNCSTAT   0x00001000 /* Bit 12: Read sync stat   */
#define C6X_DMA_CH_SCNDCR_RSYNCCLR    0x00002000 /* Bit 13: Clear read sync stat */
#define C6X_DMA_CH_SCNDCR_WSYNCSTAT   0x00004000 /* Bit 14: Write sync stat  */
#define C6X_DMA_CH_SCNDCR_WSYNCLR     0x00008000 /* Bit 15: Clear write sync stat */
#define C6X_DMA_CH_SCNDCR_DMACEN_MASK 0x00070000 /* Bit 16-18: DMAC pin ctl  */

/* C6X_DMA_AUXCR:  DMA Auxiliary Control Register bitfields
 */
#define C6X_DMA_AUXCR_CHPRI_MASK 0x00000001 /* Bits 0-3: DMA chan priority   */
#define C6X_DMA_AUXCR_AUXPRI     0x00000002 /* Bit 4: Aux chan pri mode      */

/* C6X_HPIC:  Host Port Interface Control reg bitfields
 */
#define C6X_HPIC_HWOB   0x00000001 /* Bit 0: Halfword ordering bit           */
#define C6X_HPIC_DSPINT 0x00000002 /* Bit 1: Host to CPU int                 */
#define C6X_HPIC_HINT   0x00000004 /* Bit 2: DSP to Host int                 */
#define C6X_HPIC_HRDY   0x00000008 /* Bit 3: Ready signal to Host            */
#define C6X_HPIC_FETCH  0x00000010 /* Bit 4: Host fetch request              */

#endif   /* C6X_H */
