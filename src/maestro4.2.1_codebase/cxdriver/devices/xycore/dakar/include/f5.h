/*SDOC-------------------------------------------------------------------------
 * Spectrum Signal Processing Inc. Copyright 1997
 *
 * $Workfile:: f5.h                                                           $
 *
 * Contents: F5 board specific definitions.
 *           Used by both host and target software.
 *
 * $Modtime:: 20-10-97 2:42p                                                  $
 * $Revision:: 7                                                              $
 * $Archive:: /MAC/F5/dev/shared/include/f5.h                                 $
 *
 *-----------------------------------------------------------------------EDOC*/

#ifndef F5_H
#define F5_H

#if (defined(_WINDOWS))
/* 
 * HOST Software
 */

//============================================================================
// PCI Configuration Registers Constant definitions
//============================================================================
//Register Address
#define	PCI_CFGR_MEM_MAP     0x10
#define	PCI_CFGR_IO_MEM_MAP  0x14
#define	PCI_CFGR_LOC_SPACE_0	0x18
#define	PCI_CFGR_INT_LINE    0x3C // Byte

//============================================================================
// LOCAL Configuration Registers Constant definitions
//============================================================================
//Reg. Offset from (PCI_CFGR_MEM_MAP)
#define	LOC_CFGR_RANGE_PCI_TO_LOC		   0x00
#define	LOC_CFGR_REMAP_BA_PCI_TO_LOC	   0x04
#define	LOC_CFGR_BUS_REGION_PCI_TO_LOC	0x18

//============================================================================
// SHARED Run Time Registers Constant definitions
//============================================================================
//Reg. Offset from (PCI_CFGR_MEM_MAP)
#define	SHR_REG_MAILBOX_BA_OFF	0x40
#define	MAILBOX(k)				(SHR_REG_MAILBOX_BA_OFF + 4 * k)
#define	MAILBOX_0				SHR_REG_MAILBOX_BA_OFF
#define	MAILBOX_1				SHR_REG_MAILBOX_BA_OFF + 0x04
#define	MAILBOX_2				SHR_REG_MAILBOX_BA_OFF + 0x08
#define	MAILBOX_3				SHR_REG_MAILBOX_BA_OFF + 0x0C
#define	MAILBOX_4				SHR_REG_MAILBOX_BA_OFF + 0x10
#define	MAILBOX_5				SHR_REG_MAILBOX_BA_OFF + 0x14
#define	MAILBOX_6				SHR_REG_MAILBOX_BA_OFF + 0x18
#define	MAILBOX_7				SHR_REG_MAILBOX_BA_OFF + 0x1C
#define	SHR_REG_DOORB_P2L	   SHR_REG_MAILBOX_BA_OFF + 0x20
#define	SHR_REG_DOORB_L2P 	SHR_REG_MAILBOX_BA_OFF + 0x24
#define	SHR_REG_INT_CTRL		SHR_REG_MAILBOX_BA_OFF + 0x28
#define	SHR_REG_GEN_CTRL		SHR_REG_MAILBOX_BA_OFF + 0x2C
#define	DOORBELL				   SHR_REG_DOORB_P2L

//============================================================================
// Memory Mapped Areas Offsets from the beginning of the Space_0 (in bytes):
//============================================================================
#define	MM_ARB_REGS_OFF			0x00800000 
#define	MM_ARB_REGS_SIZE			0x00040000 /* 256K bytes */
#define	MM_TBC_REGS_OFF			0x00840000 
#define	MM_TBC_REGS_SIZE			0x00040000 /* 256K bytes */
#define	MM_PLX_REGS_OFF			0x00880000 
#define	MM_PLX_REGS_SIZE			0x00040000 /* 256K bytes */
#define	MM_IRQ_REGS_OFF			0x008C0000 
#define	MM_IRQ_REGS_SIZE			0x00040000 /* 256K bytes */
#define	MM_SRAM_OFF			      0x00C00000 
#define	MM_SRAM_SIZE			   0x00200000 /* 2M bytes = 512K lwords */

//============================================================================
// F5 IRQ Registers Constant definitions
//============================================================================
//Reg. Offset (bytes) from (MM_IRQ_REGS_OFF)
#define	F5_IRQ_PCI_TO_NODEA       0x0
#define	F5_IRQ_PCI_TO_NODEB       0x4
#define	F5_IRQ_PCI_TO_NODEC       0x8
#define	F5_IRQ_PCI_TO_NODED       0xC
#define	F5_IRQ_NODEA_TO_PCI       0x10
#define	F5_IRQ_NODEB_TO_PCI       0x14
#define	F5_IRQ_NODEC_TO_PCI       0x18
#define	F5_IRQ_NODED_TO_PCI       0x1C
#define	F5_IRQ_NODEX_TO_PCI_STAT  0x20


//============================================================================
// Bit Mask Constant definitions
//============================================================================
#define  GEN_CTRL_BIT_RELCFG     29 /* Reload Config Reg Bit */
#define  GEN_CTRL_BIT_RESET      30 /* PCI Adapter Software Reset */

#define  INT_CTRL_BIT_PCI        8  /* PCI Interrupt Enable */
#define  INT_CTRL_BIT_L2P_DB     9  /* Loc.To PCI Doorbell interrupt Enable */
#define  INT_CTRL_BIT_L2P        11 /* Loc.To PCI interrupt Enable */
#define  INT_CTRL_BIT_PCI_DB_SET 13 /* PCI Doorbell interrupt Set (Active) */
#define  INT_CTRL_BIT_L2P_SET    15 /* Local To PCI Interrupt Input is Set (Active) */

#define  INT_CTRL_BIT_LOC        16 /* LOCAL Interrupt Enable */
#define  INT_CTRL_BIT_P2L_DB     17 /* PCI To Loc. Doorbell interrupt Enable */


/*
 * MACRO DEFINITIONS for BOOTLOADER
 * --------------------------------
 */

#define F5_NODEA_GMCR       0x3d840000       /* global memory ctl register */
#define F5_NODEA_LMCR       0x3d840000       /* local memory ctl register  */
#define F5_C4X_SRAM_BASE    0xC0300000       /* shared SRAM base address   */
#define F5_NODEA_IACK       F5_C4X_SRAM_BASE /* address for IACK           */

#define PCI2LOCAL_DOORBELL             SHR_REG_DOORB_P2L /* PCI to local doorbell reg offset */

#define SSPBOOT_USRENTRY_ADDR          (UINT32)0x002FFFF0 /* address containing user code entry point */
#define SSPBOOT_DMADONE_OFFADDR        (UINT32)0x00000050   /* Boot node DMA done flag offset from base of on board RAM */
#define SSPBOOT_DMAHDR_OFFADDR         (UINT32)0x00000070   /* Boot node DMA done flag offset from base of on board RAM */
#define SSPBOOT_CTLBLK_OFFADDR         (UINT32)0x00000080   /* bootload control block start offset from base of on board RAM */
#define SSPBOOT_CTLBLK_SIZE            (UINT32)0x800       /* bootload control block size (2K) */
#define SSPBOOT_BOOTPROC_DMAHDR_CTL    (UINT32)0x00C0000D  /* boot proc DMA control reg value */
#define SSPBOOT_INTLPROC_DMAHDR_CTL    (UINT32)0x00C00109  /* internal proc DMA control reg value */
#define SSPBOOT_BOOTPROC_DMAHDR_LAST   (UINT32)0x00C00005  /* boot proc DMA control reg value */
#define SSPBOOT_INTLPROC_DMAHDR_LAST   (UINT32)0x00C00105  /* internal proc DMA control reg value */

#define SSPBOOT_C4X_SRAM_BASEADDR      (UINT32)F5_C4X_SRAM_BASE

#endif /* _WINDOWS */



#if (defined(_TMS320C40))
/* 
 * C4x target Software
 */

/* far global (shared) SRAM
 */
#define F5_SHARED_SRAM_BASE  0xC0300000            /* shared SRAM base addr */


/* arbitration registers
 */
#define F5_LATENCY_REG       (volatile UINT32 *)0xC0200000
#define F5_NODEID_REG        (volatile UINT32 *)0xC0200001
#define F5_BOARD_CONFIG_REG  (volatile UINT32 *)0xC0200002
#define F5_NODEA_CONFIG_REG  (volatile UINT32 *)0xC0200003


/* IRQ control/status registers, and values
 */
#define F5_A2PCI_IRQ_REG     (volatile UINT32 *)0xC0230004  
#define F5_B2PCI_IRQ_REG     (volatile UINT32 *)0xC0230005  
#define F5_C2PCI_IRQ_REG     (volatile UINT32 *)0xC0230006  
#define F5_D2PCI_IRQ_REG     (volatile UINT32 *)0xC0230007  
#define F5_BCD2A_IRQ_REG     (volatile UINT32 *)0xC023000C  
#define F5_B2A_IRQ_REG       (volatile UINT32 *)0xC0230009  
#define F5_C2A_IRQ_REG       (volatile UINT32 *)0xC023000A  
#define F5_D2A_IRQ_REG       (volatile UINT32 *)0xC023000B  

#define F5_B2A_IRQ           (UINT32)0x00000002    
#define F5_C2A_IRQ           (UINT32)0x00000004    
#define F5_D2A_IRQ           (UINT32)0x00000008    


/* default IIF register values
 */
#define F5_IIF_NODEA         (UINT32)0x00003333;   
#define F5_IIF_NODEBCD       (UINT32)0x00000363    


/* PCI9060 configuration registers, and values
 */
#define F5_PLX_PCISTATUS_REG (volatile UINT32 *)0xC0220001

#define F5_PLX_MSTPARITYERR  (UINT32)0x01000000
#define F5_PLX_TRGABORT      (UINT32)0x08000000
#define F5_PLX_TRGABORTRCV   (UINT32)0x10000000
#define F5_PLX_MSTABORTRCV   (UINT32)0x20000000
#define F5_PLX_PARITYERR     (UINT32)0x80000000


/* PCI9060 Local configuration registers, and values
 */
#define F5_PLX_BUSREGION_REG (volatile UINT32 *)0xC0220026  
#define F5_PLX_RANGE_REG     (volatile UINT32 *)0xC0220027  
#define F5_PLX_LBASEADDR_REG (volatile UINT32 *)0xC0220028  
#define F5_PLX_PBASEADDR_REG (volatile UINT32 *)0xC022002A  

#define F5_PLX_BUSREGION     (UINT32)0xF0030143
#define F5_PLX_RANGE         (UINT32)0xFF000000
#define F5_PLX_LBASEADDR     (UINT32)0xFD000000
#define F5_PLX_PBASEADDR     (UINT32)0x00000001


/* PCI9060 Shared run-time registers, and values
 */
#define F5_PLXMAILBOX0_REG   (volatile UINT32 *)0xC0220030
#define F5_PLXMAILBOX1_REG   (volatile UINT32 *)0xC0220031
#define F5_PLXMAILBOX2_REG   (volatile UINT32 *)0xC0220032
#define F5_PLXMAILBOX3_REG   (volatile UINT32 *)0xC0220033
#define F5_PLXMAILBOX4_REG   (volatile UINT32 *)0xC0220034
#define F5_PLXMAILBOX5_REG   (volatile UINT32 *)0xC0220035
#define F5_PLXMAILBOX6_REG   (volatile UINT32 *)0xC0220036
#define F5_PLXMAILBOX7_REG   (volatile UINT32 *)0xC0220037
#define F5_PLXPCI2LOCDB_REG  (volatile UINT32 *)0xC0220038
#define F5_PLXLOC2PCIDB_REG  (volatile UINT32 *)0xC0220039
#define F5_PLXINTCONTROL_REG (volatile UINT32 *)0xC022003A
#define F5_PLXIOCONTROL_REG  (volatile UINT32 *)0xC022003B

#define F5_PLXABORT_INTENABLE (UINT32)0x00000001    /* master/target abort   */
#define F5_PLXPERR_INTENABLE  (UINT32)0x00000002    /* parity err interurupt */
#define F5_PLX256RETRIES      (UINT32)0x00001000    /* 256 retries           */
#define F5_PLXUSER0_HIGH      (UINT32)0x00010000    /* user0 output pin high */
#define F5_PLXDBELL_INTENABLE (UINT32)0x00020000    /* local doorbell enable */
#define F5_PLXDMA0_INTENABLE  (UINT32)0x00040000    /* ch0 interrupt enable  */
#define F5_PLXDMA1_INTENABLE  (UINT32)0x00080000    /* ch1 interrupt enable  */
#define F5_PLXDBELL_ACTIVE    (UINT32)0x00100000    /* local doorbell active */
#define F5_PLXDMA0_ACTIVE     (UINT32)0x00200000    /* ch0 interrupt active  */
#define F5_PLXDMA1_ACTIVE     (UINT32)0x00400000    /* ch1 interrupt active  */


/* PCI9060 DMA registers, and values
 */
#define F5_PLXDMA0_MODE_REG  (volatile UINT32 *)0xC0220040
#define F5_PLXDMA0_PADDR_REG (volatile UINT32 *)0xC0220041
#define F5_PLXDMA0_LADDR_REG (volatile UINT32 *)0xC0220042
#define F5_PLXDMA0_TSIZE_REG (volatile UINT32 *)0xC0220043
#define F5_PLXDMA0_DESCP_REG (volatile UINT32 *)0xC0220044
#define F5_PLXDMA1_MODE_REG  (volatile UINT32 *)0xC0220045
#define F5_PLXDMA1_PADDR_REG (volatile UINT32 *)0xC0220046
#define F5_PLXDMA1_LADDR_REG (volatile UINT32 *)0xC0220047
#define F5_PLXDMA1_TSIZE_REG (volatile UINT32 *)0xC0220048
#define F5_PLXDMA1_DESCP_REG (volatile UINT32 *)0xC0220049
#define F5_PLXDMA_CMD_REG    (volatile UINT32 *)0xC022004A

#define F5_PLXDMA0_ENABLE    (UINT32)0x00000001     /* ch0 enable bit        */
#define F5_PLXDMA0_START     (UINT32)0x00000002     /* ch0 start bit         */
#define F5_PLXDMA0_ABORT     (UINT32)0x00000004     /* ch0 abort bit         */
#define F5_PLXDMA0_CLRINT    (UINT32)0x00000008     /* ch0 clr interrupt bit */
#define F5_PLXDMA0_DONE      (UINT32)0x00000010     /* ch0 done bit          */

#define F5_PLXDMA1_ENABLE    (UINT32)0x00000100     /* ch1 enable bit        */
#define F5_PLXDMA1_START     (UINT32)0x00000200     /* ch1 start bit         */
#define F5_PLXDMA1_ABORT     (UINT32)0x00000400     /* ch1 abort bit         */
#define F5_PLXDMA1_CLRINT    (UINT32)0x00000800     /* ch1 clr interrupt bit */
#define F5_PLXDMA1_DONE      (UINT32)0x00001000     /* ch1 done bit          */

#define F5_PLXDMA_WRITE      (UINT32)0x00000008     /* local to PCI bit      */
#define F5_PLXDMA_TCNT       (UINT32)0x00000004     /* tcnt int enable bit   */

#define F5_PLXDMA_MODE       (UINT32)0x00000043     /* default mode value    */
#define F5_PLXDMA_DONEINT    (UINT32)0x00000400     /* done int enable bit   */

#endif /* _TMS320C40 */


#endif /* F5_H */


/*---------------------------------------------------------------------------*/
/* end of file f5.H */
