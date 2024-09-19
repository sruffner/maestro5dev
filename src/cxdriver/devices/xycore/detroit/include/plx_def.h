/*SDOC-------------------------------------------------------------------------
 * Spectrum Signal Processing Inc. Copyright 1997
 *
 * $Workfile:: plx_def.h                                                      $
 *
 * $Modtime:: 2/10/98 2:42p                                                   $
 * $Revision:: 8                                                              $
 * $Archive:: /MAC/de62_154.v10/dev/shared/include/plx_def.h                  $
 *
 * Contents:   This file includes all offset definitions for the PLX.
 *
 *-----------------------------------------------------------------------EDOC*/


#ifndef PLX_DEF_H
#define PLX_DEF_H


/* PLX PCI Configuration Registers */
/* These are byte offsets from chip select address */
#define PLX_PCI_REG_OFFSET 0x00

#define PCI_ID_OFFSET         (PLX_PCI_REG_OFFSET + 0x00)
#define PCI_CMD_OFFSET        (PLX_PCI_REG_OFFSET + 0x04)
#define PCI_CLS_OFFSET        (PLX_PCI_REG_OFFSET + 0x08)
#define PCI_LT_OFFSET         (PLX_PCI_REG_OFFSET + 0x0c)
#define PCI_BS0_OFFSET        (PLX_PCI_REG_OFFSET + 0x10)
#define PCI_BS1_OFFSET        (PLX_PCI_REG_OFFSET + 0x14)
#define PCI_BS2_OFFSET        (PLX_PCI_REG_OFFSET + 0x18)
#define PCI_ROM_OFFSET        (PLX_PCI_REG_OFFSET + 0x30)
#define PCI_IL_OFFSET         (PLX_PCI_REG_OFFSET + 0x3c)
/* #define PLX_PCI_REG_OFFSET_FROM_CS   0x00  */


/* PLX Local Configuration Registers */
/* These are byte offsets from runtime base address */
#if defined(_TMS320C6X)
#  define PLX_LOCAL_REG_OFFSET 0x80
#else // Pci offsets
#  define PLX_LOCAL_REG_OFFSET 0x00
#endif

#define PCI_ADDR_OFFSET       (PLX_LOCAL_REG_OFFSET + 0x00)
#define PCI_REMAP_OFFSET      (PLX_LOCAL_REG_OFFSET + 0x04)
#define ROM_RNG_OFFSET        (PLX_LOCAL_REG_OFFSET + 0x10)
#define ROM_REMAP_OFFSET      (PLX_LOCAL_REG_OFFSET + 0x14)
#define LOC_DESC_OFFSET       (PLX_LOCAL_REG_OFFSET + 0x18)
#define PCI_DM_OFFSET         (PLX_LOCAL_REG_OFFSET + 0x1c)
#define BASE_ADDR_OFFSET      (PLX_LOCAL_REG_OFFSET + 0x20)
#define BASE_ADDR_IO_OFFSET   (PLX_LOCAL_REG_OFFSET + 0x24)
#define MASTER_REMAP_OFFSET   (PLX_LOCAL_REG_OFFSET + 0x28)
#define PCI_CFG_OFFSET        (PLX_LOCAL_REG_OFFSET + 0x2c)
/* add the following line to get offset from Chip Select on local side */
/*#define PLX_LOCAL_REG_OFFSET_FROM_CS   0x80  */


/* PLX Shared Run Time Registers */
#if defined(_TMS320C6X)
#  define PLX_RUNTIME_REG_OFFSET 0x80
#else // Pci offsets
#  define PLX_RUNTIME_REG_OFFSET 0x00
#endif

#define MAILBOX0_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x40)
#define MAILBOX1_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x44)
#define MAILBOX2_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x48)
#define MAILBOX3_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x4c)
#define MAILBOX4_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x50)
#define MAILBOX5_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x54)
#define MAILBOX6_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x58)
#define MAILBOX7_OFFSET       (PLX_RUNTIME_REG_OFFSET + 0x5c)
#define P_TO_L_DRBL_OFFSET    (PLX_RUNTIME_REG_OFFSET + 0x60)
#define L_TO_P_DRBL_OFFSET    (PLX_RUNTIME_REG_OFFSET + 0x64)
#define INT_CFG_OFFSET        (PLX_RUNTIME_REG_OFFSET + 0x68)
#define EE_CNTL_OFFSET        (PLX_RUNTIME_REG_OFFSET + 0x6c)
/* add the following line to get offset from Chip Select on local side */
/* #define PLX_RUNTIME_REG_OFFSET_FROM_CS   0x80  */


/* PLX Local DMA Registers */
/* These are byte offsets from chip select and runtime base address */
#if defined(_TMS320C6X)
#  define PLX_DMA_REG_OFFSET 0x80
#else // Pci offsets
#  define PLX_DMA_REG_OFFSET 0x00
#endif

#define DMA0_MODE_OFFSET      (PLX_DMA_REG_OFFSET + 0x80)
#define DMA0_PCI_ADDR_OFFSET  (PLX_DMA_REG_OFFSET + 0x84)
#define DMA0_LOC_ADDR_OFFSET  (PLX_DMA_REG_OFFSET + 0x88)
#define DMA0_SIZE_OFFSET      (PLX_DMA_REG_OFFSET + 0x8c)
#define DMA0_DESC_OFFSET      (PLX_DMA_REG_OFFSET + 0x90)
#define DMA1_MODE_OFFSET      (PLX_DMA_REG_OFFSET + 0x94)
#define DMA1_PCI_ADDR_OFFSET  (PLX_DMA_REG_OFFSET + 0x98)
#define DMA1_LOC_ADDR_OFFSET  (PLX_DMA_REG_OFFSET + 0x9c)
#define DMA1_SIZE_OFFSET      (PLX_DMA_REG_OFFSET + 0xa0)
#define DMA1_DESC_OFFSET      (PLX_DMA_REG_OFFSET + 0xa4)
#define DMA_CMD_STAT_OFFSET   (PLX_DMA_REG_OFFSET + 0xa8)
#define DMA0_ARB_OFFSET       (PLX_DMA_REG_OFFSET + 0xac)
#define DMA1_ARB_OFFSET       (PLX_DMA_REG_OFFSET + 0xb0)
/* add the following line to get offset from Chip Select on local side */
/*#define PLX_DMA_REG_OFFSET_FROM_CS   0x0*/


/***** Bit & Mask settings *****/
/* PCI_CMD_OFFSET (includes status register as well)*/
#define PCI_CMD_RX_TARGET_ABORT_FLAG         0x10000000
#define PCI_CMD_TX_MASTER_ABORT_FLAG         0x20000000
#define PCI_CMD_DET_PARITY_ERR_FLAG          0x80000000

/* INT_CFG_OFFSET */
#define PLX_INT_CFG_LSERR_ABORT_INT_ENABLE   0x00000001
#define PLX_INT_CFG_LSERR_PARITY_INT_ENABLE  0x00000002
#define PLX_INT_CFG_PCI_SERR                 0x00000004
#define PLX_INT_CFG_MBOX03_INT_ENABLE        0x00000008
#define PLX_INT_CFG_PCI_INT_ENABLE           0x00000100
#define PLX_INT_CFG_PCI_DBELL_INT_ENABLE     0x00000200
#define PLX_INT_CFG_PCI_ABORT_INT_ENABLE     0x00000400
#define PLX_INT_CFG_PCI_LOCAL_INT_ENABLE     0x00000800
#define PLX_INT_CFG_RETRY256_ABORT_ENABLE    0x00001000
#define PLX_INT_CFG_PCI_DOORBELL_FLAG        0x00002000
#define PLX_INT_CFG_PCI_ABORT_FLAG           0x00004000
#define PLX_INT_CFG_LOCAL_INT_FLAG           0x00008000
#define PLX_INT_CFG_LINTO_INT_ENABLE         0x00010000
#define PLX_INT_CFG_LOCAL_DBELL_INT_ENABLE   0x00020000
#define PLX_INT_CFG_LOCAL_DMA0_INT_ENABLE    0x00040000
#define PLX_INT_CFG_LOCAL_DMA1_INT_ENABLE    0x00080000
#define PLX_INT_CFG_LOCAL_DBELL_INT_FLAG     0x00100000
#define PLX_INT_CFG_LOCAL_DMA0_INT_FLAG      0x00200000
#define PLX_INT_CFG_LOCAL_DMA1_INT_FLAG      0x00400000
#define PLX_INT_CFG_BIST_INT_FLAG            0x00800000
#define PLX_INT_CFG_DIRECT_MASTER_ABORT_FLAG 0x01000000
#define PLX_INT_CFG_DMA0_MASTER_ABORT_FLAG   0x02000000
#define PLX_INT_CFG_DMA1_MASTER_ABORT_FLAG   0x04000000
#define PLX_INT_CFG_RETRY256_ABORT_FLAG      0x08000000
#define PLX_INT_CFG_MBOX0_INT_FLAG           0x10000000
#define PLX_INT_CFG_MBOX1_INT_FLAG           0x20000000
#define PLX_INT_CFG_MBOX2_INT_FLAG           0x40000000
#define PLX_INT_CFG_MBOX3_INT_FLAG           0x80000000
#define PLX_INT_CFG_MBOXx_INT_FLAG           0xf0000000  // All the MBOX flags

/* EE_CNTL_OFFSET */
#define EE_CNTL_OFFSET_PCI_READ_CMD_MASK        0x0000000f
#define EE_CNTL_OFFSET_PCI_WRITE_CMD_MASK       0x000000f0
#define EE_CNTL_OFFSET_PCI_MEM_READ_CMD_MASK    0x00000f00
#define EE_CNTL_OFFSET_PCI_MEM_WRITE_CMD_MASK   0x0000f000
#define EE_CNTL_OFFSET_GPO                      0x00010000
#define EE_CNTL_OFFSET_GPI                      0x00020000
#define EE_CNTL_OFFSET_RELOAD_CONFIG            0x20000000
#define EE_CNTL_OFFSET_RESET                    0x40000000
#define EE_CNTL_OFFSET_LOCAL_INIT_STATUS        0x80000000

/* PLX_DMAx_MODE_OFFSET */
#define PLX_DMAx_MODE_DONE_INT_ENABLE           0x00000400

/* DMA_CMD_STAT_OFFSET */
#define PLX_DMA_CMD_STAT_DMA0_ENABLE            0x00000001
#define PLX_DMA_CMD_STAT_DMA0_START             0x00000002
#define PLX_DMA_CMD_STAT_DMA0_ABORT             0x00000004
#define PLX_DMA_CMD_STAT_DMA0_CLEAR_INT         0x00000008
#define PLX_DMA_CMD_STAT_DMA0_DONE_FLAG         0x00000010
#define PLX_DMA_CMD_STAT_DMA1_ENABLE            0x00000100
#define PLX_DMA_CMD_STAT_DMA1_START             0x00000200
#define PLX_DMA_CMD_STAT_DMA1_ABORT             0x00000400
#define PLX_DMA_CMD_STAT_DMA1_CLEAR_INT         0x00000800
#define PLX_DMA_CMD_STAT_DMA1_DONE_FLAG         0x00001000


#endif /* #ifndef PLX_DEF_H_ */


/*************************** end of plx_def.h ********************************/
