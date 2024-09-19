/*SDOC-------------------------------------------------------------------------
 * Spectrum Signal Processing Inc. Copyright 1997
 *
 * $Workfile:: f5_c4x.h                                                       $
 *
 * Contents: F5 C4x Application library
 *
 * $Modtime:: 23-10-97 2:13p                                                  $
 * $Revision:: 4                                                              $
 * $Archive:: /MAC/F5/dev/dsp/include/f5_c4x.h                                $
 *
 *-----------------------------------------------------------------------EDOC*/
#ifndef F5_C4X_H
#define F5_C4X_H

#include <sstype.h>


typedef enum
{
   NO_ERROR,
   ILLEGAL_RESOURCE,
   ILLEGAL_OPERATION,
   ILLEGAL_FLAGS,
   ILLEGAL_ADDRESS,
   ILLEGAL_SIZE
} RESULT;
                
                
typedef enum
{
   NODE_A      = 0,
   NODE_B      = 1,
   NODE_C      = 2,
   NODE_D      = 3
} NODE_ID;


typedef enum
{
   NO_GLOBAL_BUS    = 4,
   SHARED_SRAM_128K = 8,
   SHARED_SRAM_512K = 16,
   NO_CONFIG_ASSERT = 32
} CONFIGURATION;


typedef enum
{
   C4X_NODE,
   SHARED_SRAM,
   PCI,
   INTERRUPTS,
   F5_REGISTERS
} RESOURCE;


typedef enum
{
   /* C4X_NODE */
   GET_NODE_ID,
   SET_NODE_CONFIG,
   SET_DMA_CHANNEL,
   GET_DMA_CHANNEL,

   /* PCI */
   GET_MAILBOX,
   SET_MAILBOX,
   SET_DOORBELL,

   /* INTERRUPTS */
   GET_INT_SOURCES,
   CLEAR_INT,
   ASSERT_INT,
   ENABLE_PCI9060INT,

   /* F5_REGISTERS */
   GET_LATENCY_TIMER,
   SET_LATENCY_TIMER,
   LED_SET,
   LED_TOGGLE
} CONTROL;

typedef enum 
{
   /* NO_FLAGS    = 0, */     /* 17apr2000(sar):  This is already defined in sstype.h.  Caused a compile error! */
   DMA_ENABLE  = 1,
   DMA_SYNC    = 2,
   STATIC_SRC  = 4,
   STATIC_DST  = 8
} TRANSFER_FLAGS;


typedef enum
{
   HOST_PROC,
   NODEA_PROC
} PROC_ID;


typedef enum
{
   IIOF_0      = 0,
   IIOF_1      = 1,
   IIOF_2      = 2,
   IIOF_3      = 3   
} IIOF_PIN;


typedef enum
{
   DSP_LINK3_INT    = 0x0001,
   NODE_B_INT       = 0x0002,
   NODE_C_INT       = 0x0004,
   NODE_D_INT       = 0x0008,
   PCI_INT          = 0x0010,
   
   DMA0_DONE_INT    = 0x0020,
   DMA0_TERMCNT_INT = 0x0040,
   DMA1_DONE_INT    = 0x0080,
   DMA1_TERMCNT_INT = 0x0100,
   DOORBELL_INT     = 0x0200,

   MASTER_ABORT_INT = 0x0400,
   TARGET_ABORT_INT = 0x0800,
   PARITY_ERROR_INT = 0x1000,
   RETRY256_INT     = 0x2000
      
} INTERRUPT;


typedef enum
{
   PLX_DMA_CH0 = 0,
   PLX_DMA_CH1
} PLXDMA_CHANNEL;


typedef enum
{
   C4X_DMA_CH0 = 0,
   C4X_DMA_CH1,
   C4X_DMA_CH2,
   C4X_DMA_CH3,
   C4X_DMA_CH4,
   C4X_DMA_CH5   
} C4XDMA_CHANNEL;



/*SDOC-------------------------------------------------------------------------
 *
 * Function:    C4X_Open
 *
 * Description: Peforms F5 application library specific initialization.
 *              This function MUST be called before any other calls to
 *              the library are made. 
 *
 * Parameters:  Flags
 *                 NO_GLOBAL_BUS     calling node has no global bus access
 *                 SHARED_SRAM_128K  F5 configuration, 128K shared SRAM
 *                 SHARED_SRAM_512K  F5 configuration, 512K shared SRAM
 *                 NO_CONFIG_ASSERT  prevent function from asserting CONFIG/
 *
 * Returns:     NO_ERROR             success
 *              ILLEGAL_FLAGS        flags unknown to current node
 *
 *-----------------------------------------------------------------------EDOC*/
RESULT C4X_Open(UINT32 Flags);


/*SDOC-------------------------------------------------------------------------
 *
 * Function:     C4X_Close
 *
 * Description:  Dummy function, for DPI compability only. Peforms no
 *               operation.
 *
 * Parameters:   void
 *
 * Returns:      NO_ERROR
 *
 *-----------------------------------------------------------------------EDOC*/
RESULT C4X_Close(void);


/*SDOC-------------------------------------------------------------------------
 *
 * Function:    C4X_Read
 *
 * Description: Transfers 32-bit data blocks from the given Resource/Src
 *              to F5 memory.  
 *
 * Parameters:  Resource     SHARED_SRAM       far global to near memory
 *                           PCI (node A only) PCI to far global memory
 *              Dest         near or far global memory address
 *              Src          far global or PCI physical address
 *              Length       length of block, in 32-bit words
 *              Flags        DMA_ENABLE        uses DMA
 *                           DMA_SYNC          wait for DMA to complete
 *                           STATIC_DST        keep dest. address static
 *                           STATIC_SRC        keep source address static
 *
 * Returns:     NO_ERROR          success
 *              ILLEGAL_RESOURCE  resource unknown to current node
 *              ILLEGAL_OPERATION operation unknown to current node
 *              ILLEGAL_ADDRESS   destination address/block-length out
 *                                of bounds 
 *
 *-----------------------------------------------------------------------EDOC*/
RESULT C4X_Read(RESOURCE Resource,
		          UINT32 *Dest,
			       UINT32 *Src,
			       UINT32 Length,
			       UINT32 Flags);
 

/*SDOC-------------------------------------------------------------------------
 *
 * Function:    C4X_Write
 *
 * Description: Transfers 32-bit data blocks from F5 memory to
 *              the given Resource/Dest
 *
 * Parameters:  Resource     SHARED_SRAM       near memory to far global 
 *                           PCI (node A only) far global memory to PCI 
 *              Dest         far global or PCI physical address
 *              Src          near or far global memory address
 *              Length       length of block, in 32-bit words
 *              Flags        DMA_ENABLE        uses DMA
 *                           DMA_SYNC          wait for DMA to complete
 *                           STATIC_DST        keep dest. address static
 *                           STATIC_SRC        keep source address static
 *
 * Returns:     NO_ERROR          success
 *              ILLEGAL_RESOURCE  resource unknown to current node
 *              ILLEGAL_OPERATION operation unknown to current node
 *              ILLEGAL_ADDRESS   destination address/block-length out
 *                                of bounds 
 *
 *-----------------------------------------------------------------------EDOC*/
RESULT C4X_Write(RESOURCE Resource,
		           UINT32 *Dest,
			        UINT32 *Src,
			        UINT32 Length,
			        UINT32 Flags);

 
/*SDOC-------------------------------------------------------------------------
 *
 * Function:    C4X_Control
 *
 * Description: Control function used to perform high-level operations
 *              on F5 resources. 
 *
 * Parameters:  Resource           Resource on which control operation
 *                                 is to be performed.
 *              Operation          Resource specific operation to be performed.
 *              Flags              Operation specific flags
 *              Value              Pointer to memory location set by the
 *                                 function to return results of operation.
 *
 * Returns:     NO_ERROR           Success
 *              ILLEGAL_RESOURCE   Specified resource unknown
 *              ILLEGAL_OPERATION  Unknown resource/operation combination or
 *                                 illegal operation for current node.
 *              ILLEGAL_FLAGS      Illegal value for given operation
 *
 *-----------------------------------------------------------------------EDOC*/
RESULT C4X_Control(RESOURCE Resource,
                   UINT32 Operation,
                   UINT32 Flags,
                   void *Value);


/*---------------------------------------------------------------------------*/
/* Internal functions */

UINT32 GetInterruptSource(IIOF_PIN iiof);

RESULT ClearInterrupt(INTERRUPT intrupt);

RESULT InterruptProc(PROC_ID proc);

RESULT EnableInterrupt(INTERRUPT intrupt, UINT32 flag);


#endif /* F5_C4X_H */
/*---------------------------------------------------------------------------*/
/* end of file */
