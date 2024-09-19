/*SDOC-------------------------------------------------------------------------
Spectrum Signal Processing Inc. Copyright 1997
 
$Workfile:: de62c6x.h                                                         $

$Modtime:: 5/31/98 3:50p                                                      $

$Revision:: 23                                                                $

$Archive:: /MAC/de62_154.v10/dev/dsp/include/de62c6x.h                        $
 
Contents:
 
-------------------------------------------------------------------------EDOC*/
#ifndef DE62C6X_H   /* Prevent multiple inclusions of header file */
#define DE62C6X_H     

#ifdef  __cplusplus  /* Make __cplusplus compatible header file */
extern "C" {
#endif

#include "sstype.h"
#include "ssp_c6x.h"

/* Memory space definitions ==================================================== */
#define DE62_C6X_LOCAL_SSRAM_START         0x00400000  // byte address
#define DE62_C6X_LOCAL_SSRAM_START_PTR     (volatile UINT32 *)DE62_LOCAL_SSRAM_START
#define DE62_C6X_LOCAL_SSRAM_LENGTH_256KB  0x00040000  // bytes
#define DE62_C6X_LOCAL_SSRAM_LENGTH_512KB  0x00080000  // bytes

#define DE62_C6X_GSRAM_START              0x01400000  // byte address
#define DE62_C6X_GSRAM_START_PTR          (volatile UINT32 *)DE62_C6X_GSRAM_START
#define DE62_C6X_GSRAM_LENGTH             0x00080000  // bytes

#define DE62_C6X_GLOBAL_RAM_BASE           0x01400000
#define DE62_C6X_GLOBAL_RAM_LENGTH         0x00200000

#define DE62_PLX_GLOBAL_RAM_BASE           0x00000000
#define DE62_PLX_GLOBAL_RAM_LENGTH         DE62_C6X_GLOBAL_RAM_LENGTH

#define DE62_C6X_PLX_REG_BASE              0x01600000

#define DE62_C6X_LOCAL_SDRAM_START         0x02000000  // byte address
#define DE62_C6X_LOCAL_SDRAM_START_PTR     (volatile UINT32 *)DE62_LOCAL_SDRAM_START
#define DE62_C6X_LOCAL_SDRAM_LENGTH        0x01000000  // bytes

//DL3 memory map locations
#define DE62_DL3_ARB_ADDR              (UINT32) 0x01700000 
#define DE62_DL3_STD_ADDR              (UINT32) 0x01740000 
#define DE62_DL3_FST_ADDR              (UINT32) 0x01780000 
#define DE62_DL3_RDY_ADDR              (UINT32) 0x017c0000 

/* hardware definitions ======================================================== */
#define DE62_C6X_PLX_DMA_DIRECTION_BIT    0x00000001
#define DE62_C6X_PLX_DMA_GSRAM_TO_PCI     0x00000001
#define DE62_C6X_PLX_DMA_PCI_TO_GSRAM     0x00000002
#define DE62_C6X_PLX_DMA_CH0              0x00000000  // default
#define DE62_C6X_PLX_DMA_CH1              0x00000004

// EMIF setup 
#define DE62_EMIF_GLOB_CTRL_VAL (UINT32) 0x00003078

//* CONTROL REGISTER VALUES
// c == 1 CLKOUT1 cycle
// 31..28  27..22   21..20  19..16  15..14  13..8    7    6..4   3..2  1..0
// WR SU   WR STRB  WR HLD  RD SU   TA      RD STRB  RSV  MTYPE  RSV   RD HLD
// at reset:
// 1111    1111 11  11      1111    00      11 1111  0*   010    00*   11
// 15 c.   63 c.    3 c.    15 c.   --      63 c.    - 32b async --    3 c.
// Alan's PEM:
// 0111    0010 10  11      0111    00      00 1010  0*   010    00*   11
// 8c      10c      3c      7c      --      10c      - 32b asyc  --    3c
#define DE62_EMIF_CE_REG_DEFAULT      (UINT32)0xFFFFFF23
#define DE62_EMIF_CE0_REG_VAL         (UINT32)0xFFFF3F43	// EMIF SBSRAM /CE0
#define DE62_EMIF_CE1_REG_VAL         (UINT32)0x30E30422	// EMIF GS Bus /CE1
#define DE62_EMIF_CE2_REG_VAL         (UINT32)0xFFFF3F33	// EMIF SDRAM /CE2
#define DE62_EMIF_CE3_REG_VAL         (UINT32)0x00000030	// EMIF SDRAM PEM /CE3

// SDRAM CTRL   (assumes clkout2 == 100 MHz, IBM -10 part); intializes all SDRAM
// #define DE62_EMIF_SDRAM_CTRL_VAL   (UINT32) 0x0744A000 	// DRAM refresh controller on.
#define DE62_EMIF_SDRAM_CTRL_VAL      (UINT32)0x0544A000 	// DRAM refresh contoller off.
#define DE62_EMIF_SDRAM_TIM_VAL       (UINT32)0x0000061A;   // set up the SDRAM refresh period

/* macros ====================================================================== */
/* Write to PLX register, remember to add the offset from CS base address if using plx_def.h */
#define C6x_ReadPlx(Offset, pValue)    *(pValue) = *(volatile UINT32 *)(DE62_C6X_PLX_REG_BASE + (Offset))  
#define C6x_WritePlx(Offset, Value)    *(volatile UINT32 *)(DE62_C6X_PLX_REG_BASE + (Offset)) = (Value)

#define C6x_ControlLockSharedBus()   	{  *(volatile UINT32 *)C6X_TIMER0_TCR_PTR &= (~0x4);  \
                                          asm(" NOP 9"); \
                                       }

#define C6x_ControlUnlockSharedBus() 	{  volatile UINT32 Temp;   \
                                          Temp = *(volatile UINT32 *)DE62_C6X_LOCAL_SSRAM_START;   \
                                          *(volatile UINT32 *)C6X_TIMER0_TCR_PTR |= 0x4;  \
                                       }

#define C6X_LOG_ERROR(rv, ErrNum)   rv = ErrNum;

/* error codes ================================================================= */
#define DE62_C6X_ERR_BASE  0x00080201
enum DE62_C6xErrorNum   // This eventually goes in de62def.h
{
   DE62_C6X_ERR_NOT_IMPLEMENTED = DE62_C6X_ERR_BASE,
   DE62_C6X_ERR_INVALID_ADDRESS,
   DE62_C6X_ERR_NO_EXT_INT_FOUND,
   DE62_C6X_ERR_NO_DMA_CHANNEL_SPECIFIED,
   DE62_C6X_ERR_INVALID_PARAMETER,
   DE62_C6X_ERR_INCORRECT_TRANSFER_LENGTH,
   DE62_C6X_ERR_INCORRECT_DMA_CHANNEL,
   DE62_C6X_LAST_ERROR  /* Insert new errors above this error, this is used for error checking */
};

/* Function flags ============================================================== */
/* C6x_ControlSetInt Flags */
#define DE62_CONTROL_DOORBELL_INT   0x00000001  // Local to PCI Doorbell interrupt

// C6x_ControlEnableC6xInt & C6x_ControlDisableC6xInt
#define DE62_EXT_INT_4_INT    0x00000010  // These bits align with those of IER
#define DE62_EXT_INT_5_INT    0x00000020
#define DE62_EXT_INT_6_INT    0x00000040
#define DE62_EXT_INT_7_INT    0x00000080
#define DE62_GIE_INT          0x00000001  // Aligns with CSR GIE bit shifted left by 16

// C6x_ControlEnableIntSrc, C6x_ControlDisableIntSrc, C6x_ControlGetIntSrc &
// C6x_ControlClearIntSrc Bit definitions
#define DE62_DMA0_DONE_INT                0x00000020
#define DE62_DMA1_DONE_INT                0x00000080
#define DE62_LOCAL_DOORBELL_INT           0x00000200
#define DE62_MASTER_ABORT_INT             0x00000400
#define DE62_TARGET_ABORT_INT             0x00000800
#define DE62_PARITY_ERROR_INT             0x00001000
#define DE62_RETRY256_INT                 0x00002000
//#define DE62_I2O_POST_LIST_NOT_EMPTY_INT  0x00004000
//#define DE62_I20_FREE_LIST_FULL_INT       0x00008000
#define DE62_MAILBOX0_INT                 0x00010000
#define DE62_MAILBOX1_INT                 0x00020000
#define DE62_MAILBOX2_INT                 0x00040000
#define DE62_MAILBOX3_INT                 0x00080000

// Definitions for C6x_ControlGetIntSrc
#define DE62_DSP_LINK3_23_INT             0x00100000
#define DE62_DSP_LINK3_01_INT             0x00200000
#define DE62_PEM_01_INT                   0x00400000
#define DE62_PCI_INT                      0x00800000

// C6x_ControlLed Bit definitions
#define DE62_C6X_CONTROL_LED_GP_ON        0x00000001
#define DE62_C6X_CONTROL_LED_GP_OFF       0x00000002

// C6x_ControlResetDspLine3 Bit definitions
#define DE62_CONTROL_ASSERT_DL3_RESET     0x00000001
#define DE62_CONTROL_RELEASE_DL3_RESET    0x00000002
#define DE62_CONTROL_DL3_RESET            0x00000004

// C6x_ControlSetInt Bit definitions
#define DE62_C6X_CONTROL_DOORBELL_INT     0x00000001
#define DE62_C6X_CONTROL_HPI_INT          0x00000002

// C6x_SetUpC6xDma flags
#define DE62_DMA_USE_CHANNEL0_FLAG        0x00000000
#define DE62_DMA_USE_CHANNEL1_FLAG        0x00000001
#define DE62_DMA_USE_CHANNEL2_FLAG        0x00000002
#define DE62_DMA_USE_CHANNEL3_FLAG        0x00000003
//#define DMA_ENABLE_INT                    0x
#define DE62_DMA_USE_CHANNELx_FLAGS       0x00000003

/* public function definitions ================================================= */
extern RESULT C6x_ControlClearIntSrc(UINT32 DspInt, UINT32 IntSrc);

extern RESULT C6x_ControlDisableC6xInt(UINT32 IntSrc);

extern RESULT C6x_ControlDisableIntSrc(UINT32 DspInt, UINT32 IntSrc);

extern RESULT C6x_ControlEnableC6xInt(UINT32 IntSrc);

extern RESULT C6x_ControlEnableIntSrc(UINT32 DspInt, UINT32 IntSrc);

extern RESULT C6x_ControlGetIntSrc(UINT32 DspInt, UINT32 *IntSrc);

extern RESULT C6x_ControlLed(UINT32 LedControl);

extern RESULT C6x_ControlResetDspLink3(UINT32 Flags);

extern void C6x_ControlSetInt(UINT32 Flags, UINT32 Value);

extern RESULT C6x_OpenC6x(UINT32 Flags);

extern RESULT C6x_OpenPlx(UINT32 Flags);

extern RESULT C6x_ControlSetupPlxDma(  UINT32 *Dest,
                                       UINT32 *Src,
                                       UINT32 Length,
                                       UINT32 Flags);

#ifdef  __cplusplus  /* Make __cplusplus compatible header file */
}
#endif

#endif   /* DE62C6X_H */
