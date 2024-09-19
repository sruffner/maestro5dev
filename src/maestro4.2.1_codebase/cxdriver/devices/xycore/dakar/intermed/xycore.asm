;******************************************************************************
;* TMS320C3x/4x ANSI C Code Generator                            Version 5.00 *
;* Date/Time created: Fri May 13 18:19:05 2011                                *
;******************************************************************************
	.regalias	; enable floating point register aliases
fp	.set	ar3
FP	.set	ar3
;******************************************************************************
;* GLOBAL FILE PARAMETERS                                                     *
;*                                                                            *
;*   Silicon Info       : C40 Revision PG1-5                                  *
;*   Optimization       : Always Choose Smaller Code Size                     *
;*   Memory             : Small Memory Model                                  *
;*   Float-to-Int       : Normal Conversions (round toward -inf)              *
;*   Multiply           : in Software (32 bits)                               *
;*   Memory Info        : Unmapped Memory Exists                              *
;*   Repeat Loops       : Use RPTS and/or RPTB                                *
;*   Calls              : Normal Library ASM calls                            *
;*   Debug Info         : Standard TI Debug Information                       *
;******************************************************************************
;	c:\c3xtools\ac30.exe -q -v40 -D_TGTDAKARF5 -iinclude -i.. ..\xycore.c intermed\xycore.if 
	.file	"..\xycore.c"
	.file	"c:\c3xtools\math.h"
	.file	"include\type_c3x.h"
	.sym	_STATUS,0,15,13,32
	.sym	_PVOID,0,16,13,32
	.sym	_VPVOID,0,16,13,32
	.sym	_INT8,0,2,13,32
	.sym	_VINT8,0,2,13,32
	.sym	_PINT8,0,18,13,32
	.sym	_PVINT8,0,18,13,32
	.sym	_VPINT8,0,18,13,32
	.sym	_PVPINT8,0,82,13,32
	.sym	_UINT8,0,12,13,32
	.sym	_VUINT8,0,12,13,32
	.sym	_PUINT8,0,28,13,32
	.sym	_PVUINT8,0,28,13,32
	.sym	_VPUINT8,0,28,13,32
	.sym	_PVPUINT8,0,92,13,32
	.sym	_INT16,0,3,13,32
	.sym	_VINT16,0,3,13,32
	.sym	_PINT16,0,19,13,32
	.sym	_PVINT16,0,19,13,32
	.sym	_VPINT16,0,19,13,32
	.sym	_PVPINT16,0,83,13,32
	.sym	_UINT16,0,13,13,32
	.sym	_VUINT16,0,13,13,32
	.sym	_PUINT16,0,29,13,32
	.sym	_PVUINT16,0,29,13,32
	.sym	_VPUINT16,0,29,13,32
	.sym	_PVPUINT16,0,93,13,32
	.sym	_INT32,0,5,13,32
	.sym	_VINT32,0,5,13,32
	.sym	_PINT32,0,21,13,32
	.sym	_PVINT32,0,21,13,32
	.sym	_VPINT32,0,21,13,32
	.sym	_PVPINT32,0,85,13,32
	.sym	_UINT32,0,15,13,32
	.sym	_VUINT32,0,15,13,32
	.sym	_PUINT32,0,31,13,32
	.sym	_PVUINT32,0,31,13,32
	.sym	_VPUINT32,0,31,13,32
	.sym	_PVPUINT32,0,95,13,32
	.sym	_FLOAT32,0,6,13,32
	.sym	_VFLOAT32,0,6,13,32
	.sym	_PFLOAT32,0,22,13,32
	.sym	_PVFLOAT32,0,22,13,32
	.sym	_VPFLOAT32,0,22,13,32
	.sym	_PVPFLOAT32,0,86,13,32
	.sym	_STRING256,0,50,13,8192,,256
	.sym	_STRING,0,18,13,32
	.file	"include\f5_c4x.h"
	.etag	.fake0,32
	.member	_NO_ERROR,0,4,16,32
	.member	_ILLEGAL_RESOURCE,1,4,16,32
	.member	_ILLEGAL_OPERATION,2,4,16,32
	.member	_ILLEGAL_FLAGS,3,4,16,32
	.member	_ILLEGAL_ADDRESS,4,4,16,32
	.member	_ILLEGAL_SIZE,5,4,16,32
	.eos
	.sym	_RESULT,0,10,13,32,.fake0
	.etag	.fake1,32
	.member	_NODE_A,0,4,16,32
	.member	_NODE_B,1,4,16,32
	.member	_NODE_C,2,4,16,32
	.member	_NODE_D,3,4,16,32
	.eos
	.sym	_NODE_ID,0,10,13,32,.fake1
	.etag	.fake2,32
	.member	_NO_GLOBAL_BUS,4,4,16,32
	.member	_SHARED_SRAM_128K,8,4,16,32
	.member	_SHARED_SRAM_512K,16,4,16,32
	.member	_NO_CONFIG_ASSERT,32,4,16,32
	.eos
	.sym	_CONFIGURATION,0,10,13,32,.fake2
	.etag	.fake3,32
	.member	_C4X_NODE,0,4,16,32
	.member	_SHARED_SRAM,1,4,16,32
	.member	_PCI,2,4,16,32
	.member	_INTERRUPTS,3,4,16,32
	.member	_F5_REGISTERS,4,4,16,32
	.eos
	.sym	_RESOURCE,0,10,13,32,.fake3
	.etag	.fake4,32
	.member	_GET_NODE_ID,0,4,16,32
	.member	_SET_NODE_CONFIG,1,4,16,32
	.member	_SET_DMA_CHANNEL,2,4,16,32
	.member	_GET_DMA_CHANNEL,3,4,16,32
	.member	_GET_MAILBOX,4,4,16,32
	.member	_SET_MAILBOX,5,4,16,32
	.member	_SET_DOORBELL,6,4,16,32
	.member	_GET_INT_SOURCES,7,4,16,32
	.member	_CLEAR_INT,8,4,16,32
	.member	_ASSERT_INT,9,4,16,32
	.member	_ENABLE_PCI9060INT,10,4,16,32
	.member	_GET_LATENCY_TIMER,11,4,16,32
	.member	_SET_LATENCY_TIMER,12,4,16,32
	.member	_LED_SET,13,4,16,32
	.member	_LED_TOGGLE,14,4,16,32
	.eos
	.sym	_CONTROL,0,10,13,32,.fake4
	.etag	.fake5,32
	.member	_DMA_ENABLE,1,4,16,32
	.member	_DMA_SYNC,2,4,16,32
	.member	_STATIC_SRC,4,4,16,32
	.member	_STATIC_DST,8,4,16,32
	.eos
	.sym	_TRANSFER_FLAGS,0,10,13,32,.fake5
	.etag	.fake6,32
	.member	_HOST_PROC,0,4,16,32
	.member	_NODEA_PROC,1,4,16,32
	.eos
	.sym	_PROC_ID,0,10,13,32,.fake6
	.etag	.fake7,32
	.member	_IIOF_0,0,4,16,32
	.member	_IIOF_1,1,4,16,32
	.member	_IIOF_2,2,4,16,32
	.member	_IIOF_3,3,4,16,32
	.eos
	.sym	_IIOF_PIN,0,10,13,32,.fake7
	.etag	.fake8,32
	.member	_DSP_LINK3_INT,1,4,16,32
	.member	_NODE_B_INT,2,4,16,32
	.member	_NODE_C_INT,4,4,16,32
	.member	_NODE_D_INT,8,4,16,32
	.member	_PCI_INT,16,4,16,32
	.member	_DMA0_DONE_INT,32,4,16,32
	.member	_DMA0_TERMCNT_INT,64,4,16,32
	.member	_DMA1_DONE_INT,128,4,16,32
	.member	_DMA1_TERMCNT_INT,256,4,16,32
	.member	_DOORBELL_INT,512,4,16,32
	.member	_MASTER_ABORT_INT,1024,4,16,32
	.member	_TARGET_ABORT_INT,2048,4,16,32
	.member	_PARITY_ERROR_INT,4096,4,16,32
	.member	_RETRY256_INT,8192,4,16,32
	.eos
	.sym	_INTERRUPT,0,10,13,32,.fake8
	.etag	.fake9,32
	.member	_PLX_DMA_CH0,0,4,16,32
	.member	_PLX_DMA_CH1,1,4,16,32
	.eos
	.sym	_PLXDMA_CHANNEL,0,10,13,32,.fake9
	.etag	.fake10,32
	.member	_C4X_DMA_CH0,0,4,16,32
	.member	_C4X_DMA_CH1,1,4,16,32
	.member	_C4X_DMA_CH2,2,4,16,32
	.member	_C4X_DMA_CH3,3,4,16,32
	.member	_C4X_DMA_CH4,4,4,16,32
	.member	_C4X_DMA_CH5,5,4,16,32
	.eos
	.sym	_C4XDMA_CHANNEL,0,10,13,32,.fake10
	.file	"..\xycore.c"
	.stag	_Parameters,10528
	.member	_dwDotSeed,0,14,8,32
	.member	_wWidthMM,32,13,8,32
	.member	_wHeightMM,64,13,8,32
	.member	_wDistMM,96,13,8,32
	.member	_wNumTargets,128,13,8,32
	.member	_wDelayPerDot,160,13,8,32
	.member	_wOnTimePerDot,192,13,8,32
	.member	_wFiller,224,61,8,64,,2
	.member	_wType,288,61,8,1024,,32
	.member	_wNumDots,1312,61,8,1024,,32
	.member	_wRectR,2336,61,8,1024,,32
	.member	_wRectL,3360,61,8,1024,,32
	.member	_wRectT,4384,61,8,1024,,32
	.member	_wRectB,5408,61,8,1024,,32
	.member	_wOuterR,6432,61,8,1024,,32
	.member	_wOuterL,7456,61,8,1024,,32
	.member	_wOuterT,8480,61,8,1024,,32
	.member	_wOuterB,9504,61,8,1024,,32
	.eos
	.sym	_PARAMETERS,0,8,13,10528,_Parameters
	.sym	_PPARAMETERS,0,24,13,32,_Parameters
	.stag	_UpdateRec,160
	.member	_shWindowH,0,3,8,32
	.member	_shWindowV,32,3,8,32
	.member	_shPatternH,64,3,8,32
	.member	_shPatternV,96,3,8,32
	.member	_shNumReps,128,3,8,32
	.eos
	.sym	_UPDATEREC,0,8,13,160,_UpdateRec
	.sym	_PUPDATEREC,0,24,13,32,_UpdateRec

	.sect	".cinit"
	.field  	1,32
	.field  	_G_lastRandomNum+0,32
	.field  	1,32		; _G_lastRandomNum @ 0

	.sect	".text"

	.global	_G_lastRandomNum
	.bss	_G_lastRandomNum,1
	.sym	_G_lastRandomNum,_G_lastRandomNum,14,2,32

	.sect	".cinit"
	.field  	1,32
	.field  	_G_lastRand2+0,32
	.field  	1,32		; _G_lastRand2 @ 0

	.sect	".text"

	.global	_G_lastRand2
	.bss	_G_lastRand2,1
	.sym	_G_lastRand2,_G_lastRand2,14,2,32
	.sect	 ".text"

	.global	_SetSeed
	.sym	_SetSeed,_SetSeed,32,2,0
	.func	669
;******************************************************************************
;* FUNCTION NAME: _SetSeed                                                    *
;*                                                                            *
;*   Architecture       : TMS320C40                                           *
;*   Calling Convention : Stack Parameter Convention                          *
;*   Function Uses Regs : r0                                                  *
;*   Regs Saved         :                                                     *
;*   Stack Frame        : Full (w/ debug)                                     *
;*   Total Frame Size   : 2 Call + 1 Parm + 0 Auto + 0 SOE = 3 bytes          *
;******************************************************************************
_SetSeed:
	.line	1
;------------------------------------------------------------------------------
; 669 | void SetSeed( unsigned int seed )                                      
;------------------------------------------------------------------------------
        push      fp
        lda       sp,fp
	.sym	_seed,-1,14,9,32
	.line	2
	.line	3
;------------------------------------------------------------------------------
; 671 | G_lastRandomNum = seed;                                                
;------------------------------------------------------------------------------
        ldiu      *-fp(2),r0
        sti       r0,@_G_lastRandomNum+0
	.line	4
                                        ; Begin Epilog Code
        ldiu      *-fp(1),r1
        lda       *fp,fp
                                        ; Unallocate the Frame
        subi      2,sp                  ; unsigned
        bu        r1
                                        ; Branch Occurs to r1
	.endfunc	672,000000000h,0


	.sect	 ".text"

	.global	_SetSeed2
	.sym	_SetSeed2,_SetSeed2,32,2,0
	.func	674
;******************************************************************************
;* FUNCTION NAME: _SetSeed2                                                   *
;*                                                                            *
;*   Architecture       : TMS320C40                                           *
;*   Calling Convention : Stack Parameter Convention                          *
;*   Function Uses Regs : r0                                                  *
;*   Regs Saved         :                                                     *
;*   Stack Frame        : Full (w/ debug)                                     *
;*   Total Frame Size   : 2 Call + 1 Parm + 0 Auto + 0 SOE = 3 bytes          *
;******************************************************************************
_SetSeed2:
	.line	1
;------------------------------------------------------------------------------
; 674 | void SetSeed2( unsigned int seed )                                     
;------------------------------------------------------------------------------
        push      fp
        lda       sp,fp
	.sym	_seed,-1,14,9,32
	.line	2
	.line	3
;------------------------------------------------------------------------------
; 676 | G_lastRand2 = seed;                                                    
;------------------------------------------------------------------------------
        ldiu      *-fp(2),r0
        sti       r0,@_G_lastRand2+0
	.line	4
                                        ; Begin Epilog Code
        ldiu      *-fp(1),r1
        lda       *fp,fp
                                        ; Unallocate the Frame
        subi      2,sp                  ; unsigned
        bu        r1
                                        ; Branch Occurs to r1
	.endfunc	677,000000000h,0


	.sect	 ".text"

	.global	_GetRandNum
	.sym	_GetRandNum,_GetRandNum,45,2,0
	.func	697
;******************************************************************************
;* FUNCTION NAME: _GetRandNum                                                 *
;*                                                                            *
;*   Architecture       : TMS320C40                                           *
;*   Calling Convention : Stack Parameter Convention                          *
;*   Function Uses Regs : r0                                                  *
;*   Regs Saved         :                                                     *
;*   Stack Frame        : Full (w/ debug)                                     *
;*   Total Frame Size   : 2 Call + 0 Parm + 2 Auto + 0 SOE = 4 bytes          *
;******************************************************************************
_GetRandNum:
	.line	1
;------------------------------------------------------------------------------
; 697 | unsigned short GetRandNum()                                            
;------------------------------------------------------------------------------
        push      fp
        lda       sp,fp
        addi      2,sp
	.sym	_a,1,14,1,32
	.sym	_c,2,14,1,32
	.line	3
;------------------------------------------------------------------------------
; 699 | unsigned int a = 2147437301, c = 453816981;                            
;------------------------------------------------------------------------------
        ldiu      @CL1,r0
        sti       r0,*+fp(1)
        ldiu      @CL2,r0
        sti       r0,*+fp(2)
	.line	5
;------------------------------------------------------------------------------
; 701 | G_lastRandomNum = a * G_lastRandomNum + c;                             
;------------------------------------------------------------------------------
        ldiu      @_G_lastRandomNum+0,r0
        mpyi      *+fp(1),r0            ; 32 Bit Multiply (C40)
        addi      *+fp(2),r0            ; unsigned
        sti       r0,@_G_lastRandomNum+0
	.line	6
;------------------------------------------------------------------------------
; 702 | return ( (unsigned short) (0x0000FFFF & (G_lastRandomNum >> 8)) );     
;------------------------------------------------------------------------------
        lsh       -8,r0
        and       @CL3,r0
	.line	7
                                        ; Begin Epilog Code
        ldiu      *-fp(1),r1
        lda       *fp,fp
                                        ; Unallocate the Frame
        subi      4,sp                  ; unsigned
        bu        r1
                                        ; Branch Occurs to r1
	.endfunc	703,000000000h,2


	.sect	 ".text"

	.global	_GetRandNum2
	.sym	_GetRandNum2,_GetRandNum2,45,2,0
	.func	705
;******************************************************************************
;* FUNCTION NAME: _GetRandNum2                                                *
;*                                                                            *
;*   Architecture       : TMS320C40                                           *
;*   Calling Convention : Stack Parameter Convention                          *
;*   Function Uses Regs : r0                                                  *
;*   Regs Saved         :                                                     *
;*   Stack Frame        : Full (w/ debug)                                     *
;*   Total Frame Size   : 2 Call + 0 Parm + 2 Auto + 0 SOE = 4 bytes          *
;******************************************************************************
_GetRandNum2:
	.line	1
;------------------------------------------------------------------------------
; 705 | unsigned short GetRandNum2()                                           
;------------------------------------------------------------------------------
        push      fp
        lda       sp,fp
        addi      2,sp
	.sym	_a,1,14,1,32
	.sym	_c,2,14,1,32
	.line	3
;------------------------------------------------------------------------------
; 707 | unsigned int a = 2147437301, c = 453816981;                            
;------------------------------------------------------------------------------
        ldiu      @CL1,r0
        sti       r0,*+fp(1)
        ldiu      @CL2,r0
        sti       r0,*+fp(2)
	.line	5
;------------------------------------------------------------------------------
; 709 | G_lastRand2 = a * G_lastRand2 + c;                                     
;------------------------------------------------------------------------------
        ldiu      @_G_lastRand2+0,r0
        mpyi      *+fp(1),r0            ; 32 Bit Multiply (C40)
        addi      *+fp(2),r0            ; unsigned
        sti       r0,@_G_lastRand2+0
	.line	6
;------------------------------------------------------------------------------
; 710 | return ( (unsigned short) (0x0000FFFF & (G_lastRand2 >> 8)) );         
;------------------------------------------------------------------------------
        lsh       -8,r0
        and       @CL3,r0
	.line	7
                                        ; Begin Epilog Code
        ldiu      *-fp(1),r1
        lda       *fp,fp
                                        ; Unallocate the Frame
        subi      4,sp                  ; unsigned
        bu        r1
                                        ; Branch Occurs to r1
	.endfunc	711,000000000h,2


	.sect	 ".text"

	.global	_main
	.sym	_main,_main,32,2,0
	.func	722
;******************************************************************************
;* FUNCTION NAME: _main                                                       *
;*                                                                            *
;*   Architecture       : TMS320C40                                           *
;*   Calling Convention : Stack Parameter Convention                          *
;*   Function Uses Regs : f0,r0,f1,r1,f2,r2,r3,r4,r5,f6,f7,ar0,ar1,ar2,fp,ar4,*
;*                        ar5,ar6,ar7,ir0,ir1,sp,st,rs,re,rc,r8               *
;*   Regs Saved         : r4,r5,f6,f7,ar4,ar5,ar6,ar7,r8                      *
;*   Stack Frame        : Full (w/ debug)                                     *
;*   Total Frame Size   : 2 Call + 0 Parm + 66 Auto + 9 SOE = 77 bytes        *
;******************************************************************************
_main:
	.line	1
;------------------------------------------------------------------------------
; 722 | void main()                                                            
; 724 | INT16                hv, vv;           // horiz & vert displacement of 
;     | random-dot texture associated with target                              
; 725 | INT16                hw, vw;           // horiz & vert displacement of 
;     | target window                                                          
; 726 | INT16                nDotLifeDecr;     // per-refresh decrease in curre
;     | nt dot lifetime                                                        
; 728 | register INT32       i32val;           // this (hopefully dedicated!) r
;     | egister var is used for the animation                                  
; 729 | // calculations implementing the OPTICFLOW target type                 
; 730 | INT32                x32, y32;                                         
; 731 | INT16                i16Theta;                                         
; 732 | INT16                i16Scale;                                         
; 733 | // these are designated 'register' to speed-up boundary comparisons for
;     |                                                                        
; 734 | // certain tgt types during XYCORE_DOFRAME processing:                 
; 735 | register UINT16      xCoord;           // coordinates of a dot in pixel
;     |  units [0..MAX_PIX]                                                    
; 736 | register UINT16      yCoord;           //                              
; 737 | register UINT16      rectR;            // the right, left, top and bott
;     | om sides of a rect                                                     
; 738 | register UINT16      rectL;            //                              
; 739 | register UINT16      rectU;            //                              
; 740 | register UINT16      rectD;            //                              
; 742 | UINT16               rectW;            // width, height of window for c
;     | urrent target                                                          
; 743 | UINT16               rectH;            //                              
; 744 | UINT16               screenW_mm;       // display width and height in m
;     | m                                                                      
; 745 | UINT16               screenH_mm;                                       
; 746 | UINT16               u16Type;          //                              
; 747 | UINT16               u16Dummy, u16tmp; //                              
; 748 | UINT16               u16Over;                                          
; 749 | UINT16               nMaxDotLife;      // max dot life [1..32767] (fini
;     | te dotlife tgts only)                                                  
; 750 | UINT32               command;          // new command from host XYAPI  
; 751 | UINT32               status;           // used to reset CmdStat reg to 
;     | XYCORE_READY                                                           
; 752 | UINT32               xyvals;           // the next beam location to wri
;     | te: [x15..x0 y15..y0]                                                  
; 753 | UINT32               timdurbyte;       // trigger duration             
; 754 | UINT32               timdelnib;        // trigger delay                
; 755 | UINT32               timvals;          // the trigger parameters packed
;     |  appropriately for writing to timing reg                               
; 756 | UINT32               *locaddr;         // dotter board's location regis
;     | ter                                                                    
; 757 | UINT32               *stataddr;        // dotter board's status registe
;     | r                                                                      
; 758 | UINT32               *timaddr;         // dotter board's timing registe
;     | r                                                                      
; 759 | UINT32               i,j,k,l,m,d,cd;                                   
; 760 | UINT32               dotPosOffset;     // offset into dot position arra
;     | ys                                                                     
; 761 | register UINT32      nTotalVisDots;    // total #dots that are visible 
;     | in a given frame                                                       
; 762 | UINT32               maxRepeats;       // max # repeats in a given fram
;     | e (targets may have different repeat values)                           
; 763 | double               dDummy;           // used to initialize trig and o
;     | ther LUTs                                                              
; 765 | // NOTE on array declarations:  We statically allocate all memory that 
;     | we need.  Using an array declaration such as                           
; 766 | // "UINT32 arrayName[ARRAYSIZE]" did not work because apparently it doe
;     | s not tell the special-purpose compiler where                          
; 767 | // to allocate space for the array within the local processor's memory 
;     | map.  We tried such a declaration during                               
; 768 | // program development (targeted for the Detroit C6x), and the resultin
;     | g core program did not appear to function at                           
; 769 | // all.  Hence, each array is defined by a pointer, each pointer is ass
;     | igned to the array's start address in the                              
; 770 | // local processors's memory map, and care is taken in assigning these 
;     | start addresses so that the defined arrays do                          
; 771 | // not overlap.  Then individual elements of the array may be accessed 
;     | using ptrName[i] or *(ptrName + i).                                    
; 772 | //                                                                     
; 773 | register UINT16      *a;               // x-coord of next dot to be dra
;     | wn (ptr into xdotpos)                                                  
; 774 | register UINT16      *b;               // y-coord of next dot to be dra
;     | wn (ptr into ydotpos)                                                  
; 775 | register UINT16      *de;              // ptr into xdotpos, points to e
;     | lement AFTER x-coord of last dot to be drawn                           
; 776 | UINT16               *xdotpos;         // array of x-coords for dots of
;     |  all targets, stored sequentially: target0,                            
; 777 | //    target1, ... [maxsize = MAXTOTALDOTS * sizeof(UINT16)]           
; 778 | UINT16               *ydotpos;         // array of y-coords for dots of
;     |  all targets, stored sequentially: target0,                            
; 779 | //    target1, ... [maxsize = MAXTOTALDOTS * sizeof(UINT16)]           
; 780 | volatile INT16       *dotLife;         // [finite dotlife tgts] array o
;     | f current lifetimes of dots of all finite                              
; 781 | //    dotlife targets.  units=(set by host). counts down until negative
;     | .                                                                      
; 782 | //    [maxsize = MAXTOTALDOTS * sizeof(INT16)]                         
; 783 | volatile INT16       *nextDotLife;     // [finite dotlife tgts] lifetim
;     | e of next dot to be drawn (ptr into dotLife)                           
; 784 | volatile INT16       *dotNoise;        // [DL_NOISEDIR/SPEED] array of 
;     | per-dot noise offsets. units = deg/10 for                              
; 785 | //    DL_NOISEDIR, or 1% increments for DL_NOISESPEED.                 
; 786 | //    [maxsize=MAXTOTALDOTS * sizeof(INT16)]                           
; 787 | volatile INT16       *nextDotNoise;    // [DL_NOISEDIR/SPEED] ptr into 
;     | array 'dotNoise'                                                       
; 788 | volatile INT16       *fracDX;          // [DL_NOISEDIR/SPEED] array of 
;     | per-dot fractional parts of x-coordinate                               
; 789 | //    displacements to be carried over to the next update. Scaled.     
; 790 | //    [maxsize = MAXTOTALDOTS * sizeof(INT16)                          
; 791 | volatile INT16       *nextFracDX;      // [DL_NOISEDIR/SPEED] ptr into 
;     | array 'fracDX'                                                         
; 792 | volatile INT16       *fracDY;          // [DL_NOISEDIR/SPEED] array of 
;     | per-dot fractional parts of y-coordinate                               
; 793 | //    displacements to be carried over to the next update. Scaled.     
; 794 | //    [maxsize = MAXTOTALDOTS * sizeof(INT16)                          
; 795 | volatile INT16       *nextFracDY;      // [DL_NOISEDIR/SPEED] ptr into 
;     | array 'fracDY'                                                         
; 796 | volatile PARAMETERS  *parameters;      // local storage for target para
;     | meters obtained from host CPU [size =                                  
; 797 | //    sizeof(PARAMETERS)]                                              
; 798 | volatile UINT16      *hsize;           // target window widths [size = 
;     | MAX_TARGETS * sizeof(UINT16)]                                          
; 799 | volatile UINT16      *vsize;           // target window heights [size =
;     |  MAX_TARGETS * sizeof(UINT16)]                                         
; 800 | volatile UINT16      *nRedrawsLeft;    // how many more times target mu
;     | st be redrawn during current refresh                                   
; 801 | //    period [size = MAX_TARGETS * sizeof(UINT16)]                     
; 802 | volatile UINT16      *nVisDotsPerTgt;  // #dots visible in current refr
;     | esh period for each tgt                                                
; 803 | //    [size = MAX_TARGETS * sizeof(UINT16)]                            
; 804 | volatile INT16       *nNoiseUpdTicks;  // countdown timer for DL_NOISED
;     | IR/SPEED noise update interval, in msecs                               
; 805 | //    [size = MAX_TARGETS * sizeof(INT16)]                             
; 806 | volatile UINT32      *visibleDotsXY;   // packed (x,y)-coords of all do
;     | ts visible in current refresh period                                   
; 807 | //    [size = MAXDOTSPERFRAME * sizeof(UINT32)]                        
; 808 | volatile UINT32      *SharedArray;     // points to start of shared mem
;     | ory, where host XYAPI stores target                                    
; 809 | //    parameters followed by motion update records for the current fram
;     | e                                                                      
; 810 | //    [size = sizeof(PARAMETERS) + MAX_TARGETS * UPDRECSZ * sizeof(INT1
;     | 6)]                                                                    
; 811 | volatile INT16       *data;            // points to start of target dis
;     | placement data within shared memory                                    
; 812 | //    [size = MAX_TARGETS * UPDRECSZ * sizeof(INT16)]                  
; 814 | volatile INT16       *tanLUT;          // lookup table for tan(P)*2^10,
;     |  where P = [0..4499] in deg/100                                        
; 815 | //    [size = 4500 * sizeof(INT16)]                                    
; 816 | volatile INT16       *sincosLUT;       // lookup table for sin(P)*cos(P
;     | )*2^10, where P = [0..4499] in deg/100                                 
; 817 | //    [size = 4500 * sizeof(INT16)]                                    
; 818 | volatile INT16       *sinLUT;          // lookup table for sin(P)*2^10,
;     |  where P = [0..3599] in deg/10                                         
; 819 | //    [size = 3600 * sizeof(INT16)]                                    
; 820 | volatile INT16       *cosLUT;          // lookup table for tan(P)*2^10,
;     |  where P = [0..3599] in deg/10                                         
; 821 | //    [size = 3600 * sizeof(INT16)]                                    
; 822 | volatile INT32       *pow2LUT;         // lookup table for pow(2.0, (P-
;     | 140)/20)*2^20, where P = [0..280]                                      
; 823 | //    [size = 281 * sizeof(INT32)]                                     
; 824 | volatile INT32       *speedNoiseAdj;   // lookup table for the multipli
;     | cative speed noise adj factor E(N) =                                   
; 825 | //    (2^10) * (2^N - 2^(-N)) / (2 * ln(2) * N), where N=1..7.         
; 826 | //    [size = 7 * sizeof(INT32)]                                       
; 838 | #if defined(_TGTDAKARF5)                                             //
;     |  inits: init board, PCI interface, and                                 
;------------------------------------------------------------------------------
        push      fp
        lda       sp,fp
        addi      66,sp
        push      r4
        push      r5
        pushf     f6
        pushf     f7
        push      ar4
        push      ar5
        push      ar6
        push      ar7
        push      r8
	.sym	_hv,1,3,1,32
	.sym	_vv,2,3,1,32
	.sym	_hw,3,3,1,32
	.sym	_vw,4,3,1,32
	.sym	_nDotLifeDecr,5,3,1,32
;* r4    assigned to _i32val
	.sym	_i32val,4,5,4,32
	.sym	_x32,6,5,1,32
	.sym	_y32,7,5,1,32
	.sym	_i16Theta,8,3,1,32
	.sym	_i16Scale,9,3,1,32
;* ar4   assigned to _xCoord
	.sym	_xCoord,12,13,4,32
;* r5    assigned to _yCoord
	.sym	_yCoord,5,13,4,32
;* r8    assigned to _rectR
	.sym	_rectR,22,13,4,32
;* ar5   assigned to _rectL
	.sym	_rectL,13,13,4,32
	.sym	_rectU,63,13,1,32
	.sym	_rectD,64,13,1,32
	.sym	_rectW,10,13,1,32
	.sym	_rectH,11,13,1,32
	.sym	_screenW_mm,12,13,1,32
	.sym	_screenH_mm,13,13,1,32
	.sym	_u16Type,14,13,1,32
	.sym	_u16Dummy,15,13,1,32
	.sym	_u16tmp,16,13,1,32
	.sym	_u16Over,17,13,1,32
	.sym	_nMaxDotLife,18,13,1,32
	.sym	_command,19,15,1,32
	.sym	_status,20,15,1,32
	.sym	_xyvals,21,15,1,32
	.sym	_timdurbyte,22,15,1,32
	.sym	_timdelnib,23,15,1,32
	.sym	_timvals,24,15,1,32
	.sym	_locaddr,25,31,1,32
	.sym	_stataddr,26,31,1,32
	.sym	_timaddr,27,31,1,32
	.sym	_i,28,15,1,32
	.sym	_j,29,15,1,32
	.sym	_k,30,15,1,32
	.sym	_l,31,15,1,32
	.sym	_m,32,15,1,32
	.sym	_d,33,15,1,32
	.sym	_cd,34,15,1,32
	.sym	_dotPosOffset,35,15,1,32
	.sym	_nTotalVisDots,65,15,1,32
	.sym	_maxRepeats,36,15,1,32
	.sym	_dDummy,37,7,1,32
;* ar6   assigned to _a
	.sym	_a,14,29,4,32
;* ar7   assigned to _b
	.sym	_b,15,29,4,32
	.sym	_de,66,29,1,32
	.sym	_xdotpos,38,29,1,32
	.sym	_ydotpos,39,29,1,32
	.sym	_dotLife,40,19,1,32
	.sym	_nextDotLife,41,19,1,32
	.sym	_dotNoise,42,19,1,32
	.sym	_nextDotNoise,43,19,1,32
	.sym	_fracDX,44,19,1,32
	.sym	_nextFracDX,45,19,1,32
	.sym	_fracDY,46,19,1,32
	.sym	_nextFracDY,47,19,1,32
	.sym	_parameters,48,24,1,32,_Parameters
	.sym	_hsize,49,29,1,32
	.sym	_vsize,50,29,1,32
	.sym	_nRedrawsLeft,51,29,1,32
	.sym	_nVisDotsPerTgt,52,29,1,32
	.sym	_nNoiseUpdTicks,53,19,1,32
	.sym	_visibleDotsXY,54,31,1,32
	.sym	_SharedArray,55,31,1,32
	.sym	_data,56,19,1,32
	.sym	_tanLUT,57,19,1,32
	.sym	_sincosLUT,58,19,1,32
	.sym	_sinLUT,59,19,1,32
	.sym	_cosLUT,60,19,1,32
	.sym	_pow2LUT,61,21,1,32
	.sym	_speedNoiseAdj,62,21,1,32
	.line	118
;------------------------------------------------------------------------------
; 839 | C4X_Open( SHARED_SRAM_512K );                                     // DS
;     | PLINK3. not all operations are required                                
;------------------------------------------------------------------------------
        ldiu      16,r0
        push      r0
        call      _C4X_Open
                                        ; Call Occurs
        subi      1,sp                  ; unsigned
	.line	119
;------------------------------------------------------------------------------
; 840 | *((UINT32 *)0xC0200004) = 0x00000001;                             // fo
;     | r every DSP target...                                                  
;------------------------------------------------------------------------------
        lda       @CL4,ar0
        stik      1,*ar0
	.line	120
;------------------------------------------------------------------------------
; 841 | *((UINT32 *)0xC0200004) = 0x00000000;                                  
; 842 | #elif defined(_TGTDETROIT)                                             
; 843 | C6x_OpenC6x( NO_FLAGS );                                               
; 844 | C6x_OpenPlx( NO_FLAGS );                                               
; 845 | C6x_ControlResetDspLink3( DE62_CONTROL_RELEASE_DL3_RESET );            
; 846 | C6x_ControlLed( DE62_C6X_CONTROL_LED_GP_OFF );                         
; 847 | #else    // _TGTDAYTONA                                                
; 848 | C6x_OpenC6x( NO_FLAGS );                                               
; 849 | C6x_OpenHurricane( NO_FLAGS );                                         
; 850 | C6x_ControlResetDspLink3( FT_CONTROL_RELEASE_DL3_RESET );              
; 851 | C6x_ControlLed( FT_C6X_LED_0_OFF | FT_C6X_LED_1_OFF );                 
; 852 | #endif                                                                 
;------------------------------------------------------------------------------
        lda       @CL4,ar0
        stik      0,*ar0
	.line	133
;------------------------------------------------------------------------------
; 854 | locaddr = (UINT32 *) LOCADDR;                                        //
;     |  set up addresses to access dotter board regs                          
;------------------------------------------------------------------------------
        ldiu      @CL5,r0
        sti       r0,*+fp(25)
	.line	134
;------------------------------------------------------------------------------
; 855 | stataddr = (UINT32 *) STATADDR;                                        
;------------------------------------------------------------------------------
        sti       r0,*+fp(26)
	.line	135
;------------------------------------------------------------------------------
; 856 | timaddr = (UINT32 *) TIMADDR;                                          
;------------------------------------------------------------------------------
        ldiu      @CL6,r0
        sti       r0,*+fp(27)
	.line	137
;------------------------------------------------------------------------------
; 858 | SharedArray = (UINT32*) SHDATA_BASE;                                 //
;     |  "allocate" all program array in the local                             
;------------------------------------------------------------------------------
        ldiu      @CL7,r0
        sti       r0,*+fp(55)
	.line	138
;------------------------------------------------------------------------------
; 859 | data = (INT16*) ((UINT32)SHDATA_BASE + (UINT32)sizeof(PARAMETERS));  //
;     |  processor's memory map by assigning the                               
; 860 | // approp memory addresses to pointer vars...                          
;------------------------------------------------------------------------------
        ldiu      @CL8,r0
        sti       r0,*+fp(56)
	.line	140
;------------------------------------------------------------------------------
; 861 | i = 0;                                                                 
; 862 | #if defined(_TGTDETROIT)                                             //
;     |  for the Detroit, dot pos arrays are stored                            
; 863 | xdotpos = (UINT16 *) DE62_C6X_LOCAL_SSRAM_START;                  // in
;     |  a faster "local memory" region than the                               
; 864 | i += MAXTOTALDOTS * sizeof(UINT16);                               // ot
;     | her vars: SSRAM is ~5% faster than SDRAM                               
; 865 | ydotpos = (UINT16 *) (DE62_C6X_LOCAL_SSRAM_START + i);                 
; 866 | i = 0;                                                                 
; 867 | #else    // _TGTDAYTONA, _TGTDAKARF5                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(28)
	.line	147
;------------------------------------------------------------------------------
; 868 | xdotpos = (UINT16 *) LOCALDATA_BASE;                                   
;------------------------------------------------------------------------------
        ldiu      @CL9,r0
        sti       r0,*+fp(38)
	.line	148
;------------------------------------------------------------------------------
; 869 | i += MAXTOTALDOTS * sizeof(UINT16);                                    
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	149
;------------------------------------------------------------------------------
; 870 | ydotpos = (UINT16 *) (LOCALDATA_BASE + i);                             
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(39)
	.line	150
;------------------------------------------------------------------------------
; 871 | i += MAXTOTALDOTS * sizeof(UINT16);                                    
; 872 | #endif                                                                 
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	153
;------------------------------------------------------------------------------
; 874 | dotLife = (volatile INT16 *) (LOCALDATA_BASE + i);                   //
;     |  REM: sizeof() returns sizes in units of the                           
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(40)
	.line	154
;------------------------------------------------------------------------------
; 875 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  fundamental data size of tgt processor. On                            
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	155
;------------------------------------------------------------------------------
; 876 | dotNoise = (volatile INT16 *) (LOCALDATA_BASE + i);                  //
;     |  the Detroit/Daytona, this is a byte, but on                           
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(42)
	.line	156
;------------------------------------------------------------------------------
; 877 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  the Dakar it is a 4-byte word.  All arrays                            
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	157
;------------------------------------------------------------------------------
; 878 | fracDX = (volatile INT16 *) (LOCALDATA_BASE + i);                    //
;     |  on the Dakar will be INT32 or UINT32,                                 
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(44)
	.line	158
;------------------------------------------------------------------------------
; 879 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  regardless of the types we use here!!!!!                              
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	159
;------------------------------------------------------------------------------
; 880 | fracDY = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(46)
	.line	160
;------------------------------------------------------------------------------
; 881 | i += MAXTOTALDOTS * sizeof(INT16);                                     
;------------------------------------------------------------------------------
        ldiu      30000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	161
;------------------------------------------------------------------------------
; 882 | hsize = (volatile UINT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(49)
	.line	162
;------------------------------------------------------------------------------
; 883 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
        ldiu      32,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	163
;------------------------------------------------------------------------------
; 884 | vsize = (volatile UINT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(50)
	.line	164
;------------------------------------------------------------------------------
; 885 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
        ldiu      32,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	165
;------------------------------------------------------------------------------
; 886 | nRedrawsLeft = (volatile UINT16 *) (LOCALDATA_BASE + i);               
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(51)
	.line	166
;------------------------------------------------------------------------------
; 887 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
        ldiu      32,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	167
;------------------------------------------------------------------------------
; 888 | nVisDotsPerTgt = (volatile UINT16 *) (LOCALDATA_BASE + i);             
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(52)
	.line	168
;------------------------------------------------------------------------------
; 889 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
        ldiu      32,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	169
;------------------------------------------------------------------------------
; 890 | nNoiseUpdTicks = (volatile INT16 *) (LOCALDATA_BASE + i);              
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(53)
	.line	170
;------------------------------------------------------------------------------
; 891 | i += MAX_TARGETS * sizeof(INT16);                                      
;------------------------------------------------------------------------------
        ldiu      32,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	171
;------------------------------------------------------------------------------
; 892 | visibleDotsXY = (volatile UINT32 *) (LOCALDATA_BASE + i);              
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(54)
	.line	172
;------------------------------------------------------------------------------
; 893 | i += MAXDOTSPERFRAME * sizeof(UINT32);                                 
;------------------------------------------------------------------------------
        ldiu      4000,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	174
;------------------------------------------------------------------------------
; 895 | tanLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                    //
;     |  lookup tables for OPTICFLOW animation calcs!                          
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(57)
	.line	175
;------------------------------------------------------------------------------
; 896 | i += 4500 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
        ldiu      4500,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	176
;------------------------------------------------------------------------------
; 897 | sincosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                   
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(58)
	.line	177
;------------------------------------------------------------------------------
; 898 | i += 4500 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
        ldiu      4500,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	178
;------------------------------------------------------------------------------
; 899 | sinLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(59)
	.line	179
;------------------------------------------------------------------------------
; 900 | i += 3600 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
        ldiu      3600,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	180
;------------------------------------------------------------------------------
; 901 | cosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(60)
	.line	181
;------------------------------------------------------------------------------
; 902 | i += 3600 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
        ldiu      3600,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	183
;------------------------------------------------------------------------------
; 904 | pow2LUT = (volatile INT32 *) (LOCALDATA_BASE + i);                   //
;     |  special lookup tables for NOISYSPEED target's                         
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(61)
	.line	184
;------------------------------------------------------------------------------
; 905 | i += 281 * sizeof(INT32);                                            //
;     |  multiplicative noise algorithm                                        
;------------------------------------------------------------------------------
        ldiu      281,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	185
;------------------------------------------------------------------------------
; 906 | speedNoiseAdj = (volatile INT32 *) (LOCALDATA_BASE + i);               
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(62)
	.line	186
;------------------------------------------------------------------------------
; 907 | i += 7 * sizeof(INT32);                                                
;------------------------------------------------------------------------------
        ldiu      7,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	188
;------------------------------------------------------------------------------
; 909 | parameters = (volatile PARAMETERS *) (LOCALDATA_BASE + i);             
;------------------------------------------------------------------------------
        ldiu      @CL10,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(48)
	.line	189
;------------------------------------------------------------------------------
; 910 | i += sizeof( PARAMETERS );                                             
;------------------------------------------------------------------------------
        ldiu      329,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
	.line	193
;------------------------------------------------------------------------------
; 914 | command = status = XYCORE_READY;                                     //
;     |  initialize CmdStat register: Detroit and                              
; 915 | #if defined(_TGTDAKARF5)                                             //
;     |  Dakar use PCI runtime mailbox reg#2 for                               
;------------------------------------------------------------------------------
        ldiu      1,r0
        sti       r0,*+fp(20)
        sti       r0,*+fp(19)
	.line	195
;------------------------------------------------------------------------------
; 916 | C4X_Control( PCI, SET_MAILBOX, 2, &status );                      // th
;     | is, while Daytona (NodeA) uses the first                               
; 917 | #elif defined(_TGTDETROIT)                                           //
;     |  32bit word in its SSRAM...                                            
; 918 | C6x_WritePlx( MAILBOX2_OFFSET, status );                          // NO
;     | TE: We set "ready" status here so CXDRIVER                             
; 919 | #else    // _TGTDAYTONA                                              //
;     |  does not have to wait while we init the trig                          
; 920 | *((UINT32 *)0x00400000) = status;                                 // ta
;     | bles, which takes a while!                                             
; 921 | #endif                                                                 
;------------------------------------------------------------------------------
        addi3     fp,20,r3
        push      r3
        ldiu      2,r2
        ldiu      5,r1
        push      r2
        ldiu      2,r0
        push      r1
        push      r0
        call      _C4X_Control
                                        ; Call Occurs
        subi      4,sp                  ; unsigned
	.line	203
;------------------------------------------------------------------------------
; 924 | for( i = 0; i < 4500; i++ )                                          //
;     |  initialize all lookup tables...                                       
;------------------------------------------------------------------------------
        stik      0,*+fp(28)
        ldiu      *+fp(28),r0
        cmpi      4500,r0
        bhs       L15                   ; Branch
                                        ; Branch Occurs to L15
L14:        
	.line	205
;------------------------------------------------------------------------------
; 926 | dDummy = ((double) i) * 0.00017453293;                            //   
;     |  convert integer deg/100 to radians                                    
;------------------------------------------------------------------------------
        float     *+fp(28),f0
        ldflt     @CL11,f1
        ldfge     0.0000000000e+00,f1
        addf      f1,f0
        mpyf      @CL12,f0
        stf       f0,*+fp(37)
	.line	206
;------------------------------------------------------------------------------
; 927 | tanLUT[i] = (INT16) floor( 1024.0 * tan( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------
        pushf     f0
        call      _tan
                                        ; Call Occurs
        mpyf      @CL13,f0
        subi      1,sp                  ; unsigned
        addf      @CL14,f0
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f1
        negf      f1,f0
        fix       f0,r0
        fix       f1,r1
        lda       *+fp(57),ir0
        negi      r0,r0
        lda       *+fp(28),ar0
        ldile     r0,r1
        sti       r1,*+ar0(ir0)
        subi      1,sp                  ; unsigned
	.line	207
;------------------------------------------------------------------------------
; 928 | sincosLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) * cos( dDummy ) + 
;     | 0.5 );                                                                 
;------------------------------------------------------------------------------
        ldfu      *+fp(37),f0
        pushf     f0
        call      _cos
                                        ; Call Occurs
        subi      1,sp                  ; unsigned
        ldfu      *+fp(37),f1
        pushf     f1
        ldfu      f0,f6
        call      _sin
                                        ; Call Occurs
        mpyf      @CL13,f0
        mpyf3     f6,f0,f0
        addf      @CL14,f0
        subi      1,sp                  ; unsigned
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f0
        fix       f0,r2
        negf      f0,f1
        fix       f1,r0
        lda       *+fp(58),ir0
        lda       *+fp(28),ar0
        negi      r0,r0
        subi      1,sp                  ; unsigned
        ldile     r0,r2
        sti       r2,*+ar0(ir0)
	.line	203
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
        cmpi      4500,r0
        blo       L14                   ; Branch
                                        ; Branch Occurs to L14
L15:        
	.line	209
;------------------------------------------------------------------------------
; 930 | for( i = 0; i < 3600; i++ )                                            
;------------------------------------------------------------------------------
        stik      0,*+fp(28)
        ldiu      *+fp(28),r0
        cmpi      3600,r0
        bhs       L17                   ; Branch
                                        ; Branch Occurs to L17
L16:        
	.line	211
;------------------------------------------------------------------------------
; 932 | dDummy = ((double) i) * 0.0017453293;                             //   
;     |  convert integer deg/10 to radians                                     
;------------------------------------------------------------------------------
        float     *+fp(28),f0
        ldflt     @CL11,f1
        ldfge     0.0000000000e+00,f1
        addf      f1,f0
        mpyf      @CL15,f0
        stf       f0,*+fp(37)
	.line	212
;------------------------------------------------------------------------------
; 933 | sinLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------
        pushf     f0
        call      _sin
                                        ; Call Occurs
        mpyf      @CL13,f0
        subi      1,sp                  ; unsigned
        addf      @CL14,f0
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f1
        negf      f1,f0
        fix       f0,r0
        fix       f1,r1
        lda       *+fp(59),ir0
        negi      r0,r0
        lda       *+fp(28),ar0
        ldile     r0,r1
        sti       r1,*+ar0(ir0)
        subi      1,sp                  ; unsigned
	.line	213
;------------------------------------------------------------------------------
; 934 | cosLUT[i] = (INT16) floor( 1024.0 * cos( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------
        ldfu      *+fp(37),f0
        pushf     f0
        call      _cos
                                        ; Call Occurs
        mpyf      @CL13,f0
        subi      1,sp                  ; unsigned
        addf      @CL14,f0
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f1
        negf      f1,f0
        fix       f0,r0
        fix       f1,r1
        negi      r0,r0
        lda       *+fp(60),ir0
        lda       *+fp(28),ar0
        ldile     r0,r1
        subi      1,sp                  ; unsigned
        sti       r1,*+ar0(ir0)
	.line	209
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
        cmpi      3600,r0
        blo       L16                   ; Branch
                                        ; Branch Occurs to L16
L17:        
	.line	216
;------------------------------------------------------------------------------
; 937 | for( i = 0; i < 281; i++ )                                             
;------------------------------------------------------------------------------
        stik      0,*+fp(28)
        ldiu      *+fp(28),r0
        cmpi      281,r0
        bhsd      L19                   ; Branch
	nop
        ldfu      2.0000000000e+01,f7
        ldfu      2.0000000000e+00,f6
                                        ; Branch Occurs to L19
L18:        
	.line	218
;------------------------------------------------------------------------------
; 939 | dDummy = (((double) i) - 140) / 20.0;                                  
;------------------------------------------------------------------------------
        float     *+fp(28),f0
        ldflt     @CL11,f1
        ldfge     0.0000000000e+00,f1
        addf      f1,f0
        subf      1.4000000000e+02,f0
        ldfu      2.0000000000e+01,f1
        call      DIV_F40
                                        ; Call Occurs
        stf       f0,*+fp(37)
	.line	219
;------------------------------------------------------------------------------
; 940 | pow2LUT[i] = (INT32) floor(pow(2.0, dDummy + 20.0) + 0.5);             
;------------------------------------------------------------------------------
        addf      f7,f0
        pushf     f0
        pushf     f6
        call      _pow
                                        ; Call Occurs
        addf      @CL14,f0
        subi      2,sp                  ; unsigned
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f1
        negf      f1,f0
        fix       f0,r0
        fix       f1,r1
        negi      r0,r0
        lda       *+fp(61),ir0
        lda       *+fp(28),ar0
        ldile     r0,r1
        sti       r1,*+ar0(ir0)
        subi      1,sp                  ; unsigned
	.line	216
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
        cmpi      281,r0
        blo       L18                   ; Branch
                                        ; Branch Occurs to L18
L19:        
	.line	221
;------------------------------------------------------------------------------
; 942 | for( i = 0; i < 7; i++ )                                               
;------------------------------------------------------------------------------
        stik      0,*+fp(28)
        ldfu      2.0000000000e+00,f1
        ldiu      *+fp(28),r0
        ldfu      2.0000000000e+00,f7
        cmpi      7,r0
        bhsd      L21                   ; Branch
        stf       f1,*+fp(63)
        ldfu      2.0000000000e+00,f1
        stf       f1,*+fp(64)
                                        ; Branch Occurs to L21
L20:        
	.line	223
;------------------------------------------------------------------------------
; 944 | j = i+1;                                                               
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(29)
	.line	224
;------------------------------------------------------------------------------
; 945 | dDummy = 1024.0 * (pow(2.0, (double)j) - pow(2.0, -((double)j)));      
;------------------------------------------------------------------------------
        float     *+fp(29),f0
        ldflt     @CL11,f1
        ldfge     0.0000000000e+00,f1
        addf      f1,f0
        pushf     f0
        ldfu      *+fp(63),f0
        pushf     f0
        call      _pow
                                        ; Call Occurs
        float     *+fp(29),f1
        ldfu      f0,f6
        ldflt     @CL11,f0
        ldfge     0.0000000000e+00,f0
        addf      f0,f1
        subi      2,sp                  ; unsigned
        negf      f1,f0
        pushf     f0
        ldfu      *+fp(64),f0
        pushf     f0
        call      _pow
                                        ; Call Occurs
        subf3     f0,f6,f0
        mpyf      @CL13,f0
        stf       f0,*+fp(37)
        subi      2,sp                  ; unsigned
	.line	225
;------------------------------------------------------------------------------
; 946 | dDummy /= (2.0 * ((double)j) * log(2.0));                              
;------------------------------------------------------------------------------
        pushf     f7
        call      _log
                                        ; Call Occurs
        float     *+fp(29),f1
        ldflt     @CL11,f2
        ldfge     0.0000000000e+00,f2
        addf      f2,f1
        mpyf      2.0000000000e+00,f1
        mpyf3     f0,f1,f1
        ldfu      *+fp(37),f0
        subi      1,sp                  ; unsigned
        call      DIV_F40
                                        ; Call Occurs
        stf       f0,*+fp(37)
	.line	226
;------------------------------------------------------------------------------
; 947 | speedNoiseAdj[i] = (INT32) floor(dDummy + 0.5);                        
; 958 | do                                                                   //
;     |  BEGIN: runtime loop -- process cmds from                              
; 959 | {                                                                    //
;     |  host XYAPI until XYCORE_CLOSE is received.                            
; 960 | do                                                                // wa
;     | it for next command in CmdStat reg                                     
; 962 | #if defined(_TGTDAKARF5)                                               
;------------------------------------------------------------------------------
        ldfu      @CL14,f0
        addf      *+fp(37),f0
        pushf     f0
        call      _floor
                                        ; Call Occurs
        ldfu      f0,f1
        negf      f1,f0
        fix       f0,r0
        fix       f1,r1
        negi      r0,r0
        lda       *+fp(62),ir0
        lda       *+fp(28),ar0
        ldile     r0,r1
        sti       r1,*+ar0(ir0)
        subi      1,sp                  ; unsigned
	.line	221
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
        cmpi      7,r0
        blo       L20                   ; Branch
                                        ; Branch Occurs to L20
L21:        
	.line	242
;------------------------------------------------------------------------------
; 963 | C4X_Control( PCI, GET_MAILBOX, 2, &command );                          
; 964 | #elif defined(_TGTDETROIT)                                             
; 965 | C6x_ReadPlx( MAILBOX2_OFFSET, &command );                              
; 966 | #else    // _TGTDAYTONA                                                
; 967 | command = *((UINT32 *)0x00400000);                                     
; 982 | #endif                                                                 
;------------------------------------------------------------------------------
        addi3     fp,19,r3
        ldiu      2,r1
        ldiu      4,r0
        ldiu      2,r2
        push      r3
        push      r1
        push      r0
        push      r2
        call      _C4X_Control
                                        ; Call Occurs
        subi      4,sp                  ; unsigned
	.line	262
;------------------------------------------------------------------------------
; 983 | } while( command == XYCORE_READY );                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(19),r0
        cmpi      1,r0
        beq       L21                   ; Branch
                                        ; Branch Occurs to L21
	.line	265
;------------------------------------------------------------------------------
; 986 | if( command == XYCORE_INIT )                                      // BE
;     | GIN:  process XYCORE_INIT command                                      
;------------------------------------------------------------------------------
        cmpi      2,r0
        bne       L104                  ; Branch
                                        ; Branch Occurs to L104
	.line	267
;------------------------------------------------------------------------------
; 988 | memcpy( (void*) parameters, (void*) SharedArray,               // copy 
;     | params into local mem for faster access                                
; 989 | sizeof(PARAMETERS) );                                                  
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        lda       *+fp(55),ar1
        ldiu      *ar1++(1),r0
        rpts      327                   ; Fast MEMCPY(#1)
        sti       r0,*ar0++(1)
||      ldi       *ar1++(1),r0
        sti       r0,*ar0++(1)
	.line	270
;------------------------------------------------------------------------------
; 991 | SetSeed( parameters->dwDotSeed );                              // seed 
;     | both random# generators using the seed                                 
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *ar0,r0
        push      r0
        call      _SetSeed
                                        ; Call Occurs
        subi      1,sp                  ; unsigned
	.line	271
;------------------------------------------------------------------------------
; 992 | SetSeed2( parameters->dwDotSeed );                             // value
;     |  provided                                                              
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *ar0,r0
        push      r0
        call      _SetSeed2
                                        ; Call Occurs
        subi      1,sp                  ; unsigned
	.line	273
;------------------------------------------------------------------------------
; 994 | dotPosOffset = 0;                                              // prote
;     | ct against overflow of dot pos arrays:                                 
;------------------------------------------------------------------------------
        stik      0,*+fp(35)
	.line	274
;------------------------------------------------------------------------------
; 995 | for( d = 0; d < parameters->wNumTargets; d++ )                 // if ne
;     | cessary, reduce #targets processed so                                  
; 996 | {                                                              // that 
;     | total #dots to be stored falls under                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(33)
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        cmpi3     *+ar0(4),r0
        bhs       L26                   ; Branch
                                        ; Branch Occurs to L26
L24:        
	.line	276
;------------------------------------------------------------------------------
; 997 | if( dotPosOffset + parameters->wNumDots[d] > MAXTOTALDOTS ) // the maxi
;     | mum allowed limit...                                                   
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        addi      *+fp(35),r0           ; unsigned
        cmpi      30000,r0
        bhi       L26                   ; Branch
                                        ; Branch Occurs to L26
	.line	277
;------------------------------------------------------------------------------
; 998 | break;                                                                 
;------------------------------------------------------------------------------
	.line	278
;------------------------------------------------------------------------------
; 999 | dotPosOffset += parameters->wNumDots[d];                               
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        addi      *+fp(35),r0           ; unsigned
        sti       r0,*+fp(35)
	.line	274
        ldiu      1,r0
        addi      *+fp(33),r0           ; unsigned
        sti       r0,*+fp(33)
        lda       *+fp(48),ar0
        cmpi3     *+ar0(4),r0
        blo       L24                   ; Branch
                                        ; Branch Occurs to L24
L26:        
	.line	280
;------------------------------------------------------------------------------
; 1001 | parameters->wNumTargets = d;                                           
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        sti       r0,*+ar0(4)
	.line	283
;------------------------------------------------------------------------------
; 1004 | dotPosOffset = 0;                                              // gener
;     | ate & store the initial (x,y)-coords of                                
;------------------------------------------------------------------------------
        stik      0,*+fp(35)
	.line	284
;------------------------------------------------------------------------------
; 1005 | for( d = 0; d < parameters->wNumTargets; d++ )                 // dots 
;     | for all tgts in PARAMETERS struct.                                     
;------------------------------------------------------------------------------
        stik      0,*+fp(33)
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        cmpi3     *+ar0(4),r0
        bhs       L491                  ; Branch
                                        ; Branch Occurs to L491
L27:        
	.line	286
;------------------------------------------------------------------------------
; 1007 | u16Type = parameters->wType[d];                             // this tar
;     | get's type                                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(9),r0
        sti       r0,*+fp(14)
	.line	288
;------------------------------------------------------------------------------
; 1009 | if( u16Type == NO_TARGET )                                  // NO_TARGE
;     | T: not a target; nothing to do here.                                   
; 1010 | ;                                                                      
;------------------------------------------------------------------------------
        cmpi      0,r0
        beq       L102                  ; Branch
                                        ; Branch Occurs to L102
	.line	291
;------------------------------------------------------------------------------
; 1012 | else if ( (u16Type == DOTARRAY) &&                          // DOTARRAY
;     | : Nonrandom, rect array of regularly                                   
; 1013 | (parameters->wNumDots[d] > 0) )                   // spaced dots.      
;------------------------------------------------------------------------------
        cmpi      1,r0
        bne       L42                   ; Branch
                                        ; Branch Occurs to L42
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        cmpi      0,r0
        beq       L42                   ; Branch
                                        ; Branch Occurs to L42
	.line	294
;------------------------------------------------------------------------------
; 1015 | if( parameters->wRectR[d] > 32768 )                      //    width of
;     |  array.  enforce maximum value.                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r0
        cmpi      @CL16,r0
        bls       L32                   ; Branch
                                        ; Branch Occurs to L32
	.line	295
;------------------------------------------------------------------------------
; 1016 | parameters->wRectR[d] = 32768;                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      @CL17,r0
        sti       r0,*+ar0(73)
L32:        
	.line	296
;------------------------------------------------------------------------------
; 1017 | if( parameters->wRectL[d] > 32768 )                      //    dot spac
;     | ing.  enforce maximum value.                                           
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(105),r0
        cmpi      @CL16,r0
        bls       L34                   ; Branch
                                        ; Branch Occurs to L34
	.line	297
;------------------------------------------------------------------------------
; 1018 | parameters->wRectL[d] = 32768;                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      @CL17,r0
        sti       r0,*+ar0(105)
L34:        
	.line	299
;------------------------------------------------------------------------------
; 1020 | cd = parameters->wRectR[d] / 2;                          //    draw arr
;     | ay from L->R, B->T starting w/dot                                      
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r0
        lsh       @CL18,r0
        sti       r0,*+fp(34)
	.line	300
;------------------------------------------------------------------------------
; 1021 | xdotpos[dotPosOffset] = CTR_PIX - cd;                    //    at lower
;     |  left corner. init pos of this                                         
;------------------------------------------------------------------------------
        lda       *+fp(38),ir0
        lda       *+fp(35),ar0
        subri     32767,r0              ; unsigned
        sti       r0,*+ar0(ir0)
	.line	301
;------------------------------------------------------------------------------
; 1022 | ydotpos[dotPosOffset] = CTR_PIX - cd;                    //    dot so t
;     | hat array is ctr'd at origin.                                          
;------------------------------------------------------------------------------
        lda       *+fp(39),ir0
        ldiu      *+fp(34),r0
        lda       *+fp(35),ar0
        subri     32767,r0              ; unsigned
        sti       r0,*+ar0(ir0)
	.line	303
;------------------------------------------------------------------------------
; 1024 | m = CTR_PIX + cd;                                        //    right bo
;     | undary of array                                                        
;------------------------------------------------------------------------------
        ldiu      32767,r0
        addi      *+fp(34),r0           ; unsigned
        sti       r0,*+fp(32)
	.line	305
;------------------------------------------------------------------------------
; 1026 | for( i = 1; i < parameters->wNumDots[d]; i++ )           //    draw rem
;     | aining dots from left to right,                                        
; 1027 | {                                                        //    a row at
;     |  a time (H = V spacing):                                               
;------------------------------------------------------------------------------
        stik      1,*+fp(28)
        bu        L40
                                        ; Branch Occurs to L40
L35:        
	.line	307
;------------------------------------------------------------------------------
; 1028 | j = (UINT32) xdotpos[dotPosOffset + i-1];             //       loc of l
;     | ast dot                                                                
;------------------------------------------------------------------------------
        lda       *+fp(35),ar0
        addi      *+fp(38),ar0          ; unsigned
        addi      *+fp(28),ar0          ; unsigned
        ldiu      *-ar0(1),r0
        sti       r0,*+fp(29)
	.line	308
;------------------------------------------------------------------------------
; 1029 | k = (UINT32) ydotpos[dotPosOffset + i-1];                              
;------------------------------------------------------------------------------
        lda       *+fp(35),ar0
        addi      *+fp(39),ar0          ; unsigned
        addi      *+fp(28),ar0          ; unsigned
        ldiu      *-ar0(1),r0
        sti       r0,*+fp(30)
	.line	309
;------------------------------------------------------------------------------
; 1030 | l = (UINT32) parameters->wRectL[d];                                    
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(105),r0
        sti       r0,*+fp(31)
	.line	310
;------------------------------------------------------------------------------
; 1031 | if( j + l >= m )                                      //       move up 
;     | to next row of dots                                                    
;------------------------------------------------------------------------------
        addi      *+fp(29),r0           ; unsigned
        cmpi      *+fp(32),r0
        blo       L38                   ; Branch
                                        ; Branch Occurs to L38
	.line	312
;------------------------------------------------------------------------------
; 1033 | if( k + l > MAX_PIX ) break;                       //       out of room
;     |  in upper-right quad. stop!                                            
;------------------------------------------------------------------------------
        ldiu      *+fp(31),r0
        addi      *+fp(30),r0           ; unsigned
        cmpi      @CL19,r0
        bhi       L41                   ; Branch
                                        ; Branch Occurs to L41
	.line	313
;------------------------------------------------------------------------------
; 1034 | xdotpos[dotPosOffset + i] = CTR_PIX - cd;                              
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        ldiu      *+fp(34),r0
        lda       *+fp(28),ar0
        addi      *+fp(38),ir0          ; unsigned
        subri     32767,r0              ; unsigned
        sti       r0,*+ar0(ir0)
	.line	314
;------------------------------------------------------------------------------
; 1035 | ydotpos[dotPosOffset + i] = (UINT16) (k + l);                          
; 1037 | else                                                  //       move to 
;     | next dot in row                                                        
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        ldiu      *+fp(31),r0
        lda       *+fp(28),ar0
        addi      *+fp(39),ir0          ; unsigned
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+ar0(ir0)
        bu        L39
                                        ; Branch Occurs to L39
L38:        
	.line	318
;------------------------------------------------------------------------------
; 1039 | xdotpos[dotPosOffset + i] = (UINT16) (j + l);                          
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        ldiu      *+fp(31),r0
        lda       *+fp(28),ar0
        addi      *+fp(38),ir0          ; unsigned
        addi      *+fp(29),r0           ; unsigned
        sti       r0,*+ar0(ir0)
	.line	319
;------------------------------------------------------------------------------
; 1040 | ydotpos[dotPosOffset + i] = (UINT16) k;                                
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(28),ar0
        addi      *+fp(39),ir0          ; unsigned
        ldiu      *+fp(30),r0
        sti       r0,*+ar0(ir0)
L39:        
	.line	305
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
L40:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(28),r0
        cmpi      *+ar0(41),r0
        blo       L35                   ; Branch
                                        ; Branch Occurs to L35
L41:        
	.line	322
;------------------------------------------------------------------------------
; 1043 | parameters->wNumDots[d] = i;                             //       #dots
;     |  reduced if array did not fit in                                       
; 1044 | //       upper-right quadrant!                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(28),r0
        sti       r0,*+ar0(41)
        bu        L102
                                        ; Branch Occurs to L102
L42:        
	.line	326
;------------------------------------------------------------------------------
; 1047 | else if( u16Type == ORIBAR )                                // ORIBAR: 
;     | rect bar or line of dots oriented at                                   
; 1048 | {                                                           // a specif
;     | ic angle in [0..360)...                                                
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      10,r0
        bne       L59                   ; Branch
                                        ; Branch Occurs to L59
	.line	328
;------------------------------------------------------------------------------
; 1049 | hw = (INT16) (parameters->wRectR[d] >> 1);               //    half-wid
;     | th of bar                                                              
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r0
        lsh       -1,r0
        sti       r0,*+fp(3)
	.line	329
;------------------------------------------------------------------------------
; 1050 | vw = (INT16) (parameters->wRectL[d] >> 1);               //    half-hei
;     | ght of bar                                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(105),r0
        lsh       -1,r0
        sti       r0,*+fp(4)
	.line	331
;------------------------------------------------------------------------------
; 1052 | if( parameters->wRectT[d] >= 360 )                       //    drift ax
;     | is angle, limited to [0..360)                                          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(137),r0
        cmpi      360,r0
        blo       L45                   ; Branch
                                        ; Branch Occurs to L45
	.line	332
;------------------------------------------------------------------------------
; 1053 | parameters->wRectT[d] = 0;                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        stik      0,*+ar0(137)
L45:        
	.line	333
;------------------------------------------------------------------------------
; 1054 | xCoord = 10 * parameters->wRectT[d];                     //    convert 
;     | to deg/10                                                              
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(137),r0
        ash3      1,r0,r1
        ash3      3,r0,r0
        addi3     r1,r0,ar4             ; unsigned
	.line	335
;------------------------------------------------------------------------------
; 1056 | hv = (INT16) sinLUT[xCoord];                             //    1024*sin
;     | (A), where A = drift axis angle                                        
;------------------------------------------------------------------------------
        lda       *+fp(59),ir0
        ldiu      *+ar4(ir0),r0
        sti       r0,*+fp(1)
	.line	336
;------------------------------------------------------------------------------
; 1057 | vv = (INT16) cosLUT[xCoord];                             //    1024*cos
;     | (A)                                                                    
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        ldiu      *+ar4(ir0),r0
        sti       r0,*+fp(2)
	.line	338
;------------------------------------------------------------------------------
; 1059 | if( vw == 0 )                                            //    if zero 
;     | half-height, bar is NOT drawn!                                         
; 1060 | {                                                        //    We put a
;     | ll the dots at (0,0), but we                                           
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        cmpi      0,r0
        bne       L50                   ; Branch
                                        ; Branch Occurs to L50
	.line	340
;------------------------------------------------------------------------------
; 1061 | for(k=0; k<parameters->wNumDots[d]; k++)              //    don't draw 
;     | them in DOFRAME processing.                                            
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L48
                                        ; Branch Occurs to L48
L47:        
	.line	342
;------------------------------------------------------------------------------
; 1063 | xdotpos[dotPosOffset + k] = (UINT16) 0;                                
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(38),ir0          ; unsigned
        stik      0,*+ar0(ir0)
	.line	343
;------------------------------------------------------------------------------
; 1064 | ydotpos[dotPosOffset + k] = (UINT16) 0;                                
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(39),ir0          ; unsigned
        stik      0,*+ar0(ir0)
	.line	340
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L48:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L47                   ; Branch
                                        ; Branch Occurs to L47
        bu        L102
                                        ; Branch Occurs to L102
L50:        
	.line	346
;------------------------------------------------------------------------------
; 1067 | else if( hw == 0 )                                       //    if zero 
;     | half-width, bar is just a line:                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        cmpi      0,r0
        bne       L55                   ; Branch
                                        ; Branch Occurs to L55
	.line	348
;------------------------------------------------------------------------------
; 1069 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
; 1071 | // bar half-ht in pixels -> 2^10 x (half-ht in mm)                     
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L53
                                        ; Branch Occurs to L53
L52:        
	.line	351
;------------------------------------------------------------------------------
; 1072 | y32 = (INT32) vw;                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        sti       r0,*+fp(7)
	.line	352
;------------------------------------------------------------------------------
; 1073 | y32 *= parameters->wHeightMM;                                          
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(2),r0
        mpyi      *+fp(7),r0            ; 32 Bit Multiply (C40)
        sti       r0,*+fp(7)
	.line	353
;------------------------------------------------------------------------------
; 1074 | y32 >>= 6;                                                             
; 1076 | // yMM*2^10: dots uniformly distributed in [-h/2..h/2] along y-axis (x-
;     | coord is 0), h in mm.                                                  
;------------------------------------------------------------------------------
        ash       -6,r0                 ;asr
        sti       r0,*+fp(7)
	.line	356
;------------------------------------------------------------------------------
; 1077 | i32val = y32;                                                          
;------------------------------------------------------------------------------
        ldiu      r0,r4
	.line	357
;------------------------------------------------------------------------------
; 1078 | i32val *= (INT32) 2 * k;                                               
;------------------------------------------------------------------------------
        ldiu      *+fp(30),r0
        ash       1,r0
        mpyi3     r4,r0,r4              ; 32 Bit Multiply (C40)
	.line	358
;------------------------------------------------------------------------------
; 1079 | i32val /= parameters->wNumDots[d];                                     
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r1
        ldiu      r4,r0
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	359
;------------------------------------------------------------------------------
; 1080 | i32val -= y32;                                                         
;------------------------------------------------------------------------------
        subi      *+fp(7),r4
	.line	360
;------------------------------------------------------------------------------
; 1081 | y32 = i32val;                                                          
; 1083 | // now do rotation transformation in true screen coords: (0,yMM) -> (xM
;     | M', yMM'). Then convert                                                
; 1084 | // back to pixels: (xPx', yPx'). Note that we have to remove scale fact
;     | ors 64 and 1024 as we do                                               
; 1085 | // the calcs. These scale factors let us do integer arithmetic w/o losi
;     | ng too much precision in                                               
; 1086 | // the final result.                                                   
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	366
;------------------------------------------------------------------------------
; 1087 | i32val *= (-hv);                                   // -(yMM*2^10)*(2^10
;     | )*sinA = xMM' * 2^20                                                   
;------------------------------------------------------------------------------
        mpyi      *+fp(1),r4            ; 32 Bit Mutiply (C40)
        negi      r4,r4
	.line	367
;------------------------------------------------------------------------------
; 1088 | i32val /= (INT32) parameters->wWidthMM;            // xMM'*2^4*(2^16/sc
;     | reenW_mm) = xPix'*2^4                                                  
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(1),r1
        ldiu      r4,r0
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	368
;------------------------------------------------------------------------------
; 1089 | i32val >>= 4;                                      // xPix'            
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	369
;------------------------------------------------------------------------------
; 1090 | i32val += 32767;                                   // translate to devi
;     | ce origin                                                              
;------------------------------------------------------------------------------
        addi      32767,r4
	.line	370
;------------------------------------------------------------------------------
; 1091 | xdotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        addi      *+fp(38),ir0          ; unsigned
        lda       *+fp(30),ar0
        sti       r4,*+ar0(ir0)
	.line	372
;------------------------------------------------------------------------------
; 1093 | i32val = y32 * vv;                                 // (yMM*2^10)*(2^10)
;     | *cosA = yMM' * 2^20                                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r4
        mpyi      *+fp(7),r4            ; 32 Bit Mutiply (C40)
	.line	373
;------------------------------------------------------------------------------
; 1094 | i32val /= (INT32) parameters->wHeightMM;           // yMM'*2^4*(2^16/sc
;     | reenH_mm) = yPix'*2^4                                                  
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(2),r1
        ldiu      r4,r0
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	374
;------------------------------------------------------------------------------
; 1095 | i32val >>= 4;                                      // yPix'            
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	375
;------------------------------------------------------------------------------
; 1096 | i32val += 32767;                                   // translate to devi
;     | ce origin                                                              
;------------------------------------------------------------------------------
        addi      32767,r4
	.line	376
;------------------------------------------------------------------------------
; 1097 | ydotpos[dotPosOffset + k] = (UINT16)i32val;                            
; 1100 | else                                                     //    general 
;     | case: a rect bar w/ random dots:                                       
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(39),ir0          ; unsigned
        sti       r4,*+ar0(ir0)
	.line	348
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L53:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L52                   ; Branch
                                        ; Branch Occurs to L52
        bu        L102
                                        ; Branch Occurs to L102
L55:        
	.line	381
;------------------------------------------------------------------------------
; 1102 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L57
                                        ; Branch Occurs to L57
L56:        
	.line	383
;------------------------------------------------------------------------------
; 1104 | xCoord = ((UINT16) GetRandNum());                  // random x-coord xP
;     | ix in [-w/2 .. w/2]                                                    
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	384
;------------------------------------------------------------------------------
; 1105 | x32 = (INT32) (xCoord % parameters->wRectR[d]);                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(6)
	.line	385
;------------------------------------------------------------------------------
; 1106 | x32 -= (INT32) hw;                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        subri     *+fp(6),r0
        sti       r0,*+fp(6)
	.line	386
;------------------------------------------------------------------------------
; 1107 | x32 *= parameters->wWidthMM;                       // xPix -> 2^6 * xMM
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(1),r0
        mpyi      *+fp(6),r0            ; 32 Bit Multiply (C40)
        sti       r0,*+fp(6)
	.line	387
;------------------------------------------------------------------------------
; 1108 | x32 >>= 10;                                                            
;------------------------------------------------------------------------------
        ash       -10,r0                ;asr
        sti       r0,*+fp(6)
	.line	389
;------------------------------------------------------------------------------
; 1110 | yCoord = ((UINT16) GetRandNum());                  // random y-coord yP
;     | ix in [-h/2 .. h/2]                                                    
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	390
;------------------------------------------------------------------------------
; 1111 | y32 = (INT32) (yCoord % parameters->wRectL[d]);                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(105),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(7)
	.line	391
;------------------------------------------------------------------------------
; 1112 | y32 -= (INT32) vw;                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        subri     *+fp(7),r0
        sti       r0,*+fp(7)
	.line	392
;------------------------------------------------------------------------------
; 1113 | y32 *= parameters->wHeightMM;                      // yPix -> 2^6 * yMM
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(2),r0
        mpyi      *+fp(7),r0            ; 32 Bit Multiply (C40)
        sti       r0,*+fp(7)
	.line	393
;------------------------------------------------------------------------------
; 1114 | y32 >>= 10;                                                            
; 1116 | // rotation transformation: (xMM*2^6)*1024*cosA - (yMM*2^6)*1024*sinA =
;     |  (xMM*cosA - yMM*sinA)*2^16                                            
; 1117 | // = xMM'*2^16. xMM'*2^16/screenW_mm = xPix'. Translate to device origi
;     | n. Analgously for y-coord,                                             
; 1118 | // (xMM*2^6)*1024*sinA + (yMM*2^6)*1024*cosA = yMM'*2^16. yMM'*2^16/scr
;     | eenH_mm = yPix'.                                                       
;------------------------------------------------------------------------------
        ash       -10,r0                ;asr
        sti       r0,*+fp(7)
	.line	398
;------------------------------------------------------------------------------
; 1119 | i32val = x32 * vv - y32 * hv;                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        ldiu      *+fp(1),r1
        mpyi      *+fp(6),r0            ; 32 Bit Multiply (C40)
        mpyi      *+fp(7),r1            ; 32 Bit Multiply (C40)
        subi3     r1,r0,r4
	.line	399
;------------------------------------------------------------------------------
; 1120 | i32val /= (INT32) parameters->wWidthMM;                                
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      r4,r0
        ldiu      *+ar0(1),r1
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	400
;------------------------------------------------------------------------------
; 1121 | i32val += 32767;                                                       
;------------------------------------------------------------------------------
        addi      32767,r4
	.line	401
;------------------------------------------------------------------------------
; 1122 | xdotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
        lda       *+fp(30),ar0
        lda       *+fp(35),ir0
        addi      *+fp(38),ir0          ; unsigned
        sti       r4,*+ar0(ir0)
	.line	403
;------------------------------------------------------------------------------
; 1124 | i32val = x32 * hv + y32 * vv;                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        ldiu      *+fp(2),r1
        mpyi      *+fp(6),r0            ; 32 Bit Multiply (C40)
        mpyi      *+fp(7),r1            ; 32 Bit Multiply (C40)
        addi3     r1,r0,r4
	.line	404
;------------------------------------------------------------------------------
; 1125 | i32val /= (INT32) parameters->wHeightMM;                               
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      r4,r0
        ldiu      *+ar0(2),r1
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	405
;------------------------------------------------------------------------------
; 1126 | i32val += 32767;                                                       
;------------------------------------------------------------------------------
        addi      32767,r4
	.line	406
;------------------------------------------------------------------------------
; 1127 | ydotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        addi      *+fp(39),ir0          ; unsigned
        lda       *+fp(30),ar0
        sti       r4,*+ar0(ir0)
	.line	381
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L57:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L56                   ; Branch
                                        ; Branch Occurs to L56
        bu        L102
                                        ; Branch Occurs to L102
L59:        
	.line	411
;------------------------------------------------------------------------------
; 1132 | else if( u16Type == STATICANNU )                            // STATICAN
;     | NU: Optimized implementation of rect                                   
; 1133 | {                                                           // annulus 
;     | when neither window nor dots move.                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      6,r0
        bne       L73                   ; Branch
                                        ; Branch Occurs to L73
	.line	413
;------------------------------------------------------------------------------
; 1134 | l = 0;                                                   // dots always
;     |  stay at their initial positions!                                      
;------------------------------------------------------------------------------
        stik      0,*+fp(31)
	.line	414
;------------------------------------------------------------------------------
; 1135 | for ( k = 0; k < parameters->wNumDots[d]; k++ )          // we generate
;     |  dots randomly, then drop all                                          
; 1136 | {                                                        // those which
;     |  are outside the annular window!                                       
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L71
                                        ; Branch Occurs to L71
L61:        
	.line	416
;------------------------------------------------------------------------------
; 1137 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	417
;------------------------------------------------------------------------------
; 1138 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	418
;------------------------------------------------------------------------------
; 1139 | if( (xCoord >= parameters->wOuterL[d]) && (xCoord <= parameters->wOuter
;     | R[d]) &&                                                               
; 1140 | (yCoord >= parameters->wOuterB[d]) && (yCoord <= parameters->wOuterT[d]
;     | ) &&                                                                   
; 1141 | ((xCoord > parameters->wRectR[d]) || (xCoord < parameters->wRectL[d]) |
;     | |                                                                      
; 1142 | (yCoord > parameters->wRectT[d]) || (yCoord < parameters->wRectB[d])) )
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(233),ar4
        blo       L70                   ; Branch
                                        ; Branch Occurs to L70
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(201),ar4
        bhi       L70                   ; Branch
                                        ; Branch Occurs to L70
        lda       *+fp(33),ir0
        lda       297,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),r5
        blo       L70                   ; Branch
                                        ; Branch Occurs to L70
        lda       *+fp(33),ir0
        lda       265,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),r5
        bhi       L70                   ; Branch
                                        ; Branch Occurs to L70
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(73),ar4
        bhi       L69                   ; Branch
                                        ; Branch Occurs to L69
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(105),ar4
        blo       L69                   ; Branch
                                        ; Branch Occurs to L69
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(137),r5
        bhi       L69                   ; Branch
                                        ; Branch Occurs to L69
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi      *+ar0(169),r5
        bhs       L70                   ; Branch
                                        ; Branch Occurs to L70
L69:        
	.line	423
;------------------------------------------------------------------------------
; 1144 | xdotpos[dotPosOffset + l] = xCoord;                                    
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(31),ar0
        addi      *+fp(38),ir0          ; unsigned
        sti       ar4,*+ar0(ir0)
	.line	424
;------------------------------------------------------------------------------
; 1145 | ydotpos[dotPosOffset + l] = yCoord;                                    
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(31),ar0
        addi      *+fp(39),ir0          ; unsigned
        sti       r5,*+ar0(ir0)
	.line	425
;------------------------------------------------------------------------------
; 1146 | l++;                                                                   
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(31),r0           ; unsigned
        sti       r0,*+fp(31)
L70:        
	.line	414
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L71:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L61                   ; Branch
                                        ; Branch Occurs to L61
	.line	428
;------------------------------------------------------------------------------
; 1149 | parameters->wNumDots[d] = l;                                           
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(31),r0
        sti       r0,*+ar0(41)
        bu        L102
                                        ; Branch Occurs to L102
L73:        
	.line	431
;------------------------------------------------------------------------------
; 1152 | else if( u16Type == OPTRECTWIN || u16Type == OPTCOHERENT || // OPTRECTW
;     | IN, etc: tgt dots randomly distrib.                                    
; 1153 | u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR || // within boundaries
;     |  (incl edges) of the                                                   
; 1154 | u16Type == DL_NOISESPEED )                         // visible window...
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      7,r0
        beq       L78                   ; Branch
                                        ; Branch Occurs to L78
        cmpi      12,r0
        beq       L78                   ; Branch
                                        ; Branch Occurs to L78
        cmpi      8,r0
        beq       L78                   ; Branch
                                        ; Branch Occurs to L78
        cmpi      11,r0
        beq       L78                   ; Branch
                                        ; Branch Occurs to L78
        cmpi      13,r0
        bne       L94                   ; Branch
                                        ; Branch Occurs to L94
L78:        
	.line	435
;------------------------------------------------------------------------------
; 1156 | hsize[d] = parameters->wRectR[d] - parameters->wRectL[d] + 1;          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar2
        lda       *+fp(33),ar0
        lda       *+fp(33),ar1
        lda       *+fp(49),ir0
        addi      *+fp(48),ar2          ; unsigned
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar2(105),r0
        subri     *+ar0(73),r0          ; unsigned
        addi      1,r0                  ; unsigned
        sti       r0,*+ar1(ir0)
	.line	436
;------------------------------------------------------------------------------
; 1157 | vsize[d] = parameters->wRectT[d] - parameters->wRectB[d] + 1;          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar2
        lda       *+fp(33),ar0
        lda       *+fp(33),ar1
        lda       *+fp(50),ir0
        addi      *+fp(48),ar2          ; unsigned
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar2(169),r0
        subri     *+ar0(137),r0         ; unsigned
        addi      1,r0                  ; unsigned
        sti       r0,*+ar1(ir0)
	.line	437
;------------------------------------------------------------------------------
; 1158 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L80
                                        ; Branch Occurs to L80
L79:        
	.line	439
;------------------------------------------------------------------------------
; 1160 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	440
;------------------------------------------------------------------------------
; 1161 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	441
;------------------------------------------------------------------------------
; 1162 | xdotpos[dotPosOffset + k] = parameters->wRectL[d] + (xCoord % hsize[d])
;     | ;                                                                      
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        lda       *+fp(49),ir0
        ldiu      ar4,r0
        ldiu      *+ar0(ir0),r1
        call      MOD_U40
                                        ; Call Occurs
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      *+ar0(105),r0         ; unsigned
        lda       *+fp(35),ir0
        addi      *+fp(38),ir0          ; unsigned
        lda       *+fp(30),ar0
        sti       r0,*+ar0(ir0)
	.line	442
;------------------------------------------------------------------------------
; 1163 | ydotpos[dotPosOffset + k] = parameters->wRectB[d] + (yCoord % vsize[d])
;     | ;                                                                      
; 1166 | // for these types, we also assign a random lifetime between 1 and targ
;     | et's maximum dot life. We also                                         
; 1167 | // make sure the per-dot fractional pixel displacements are initially 0
;     |  (appl to noisy tgts only).                                            
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        lda       *+fp(50),ir0
        ldiu      r5,r0
        ldiu      *+ar0(ir0),r1
        call      MOD_U40
                                        ; Call Occurs
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      *+ar0(169),r0         ; unsigned
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(39),ir0          ; unsigned
        sti       r0,*+ar0(ir0)
	.line	437
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L80:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L79                   ; Branch
                                        ; Branch Occurs to L79
	.line	447
;------------------------------------------------------------------------------
; 1168 | if(u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||  u16Type == DL_NO
;     | ISESPEED)                                                              
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      8,r0
        beq       L84                   ; Branch
                                        ; Branch Occurs to L84
        cmpi      11,r0
        beq       L84                   ; Branch
                                        ; Branch Occurs to L84
        cmpi      13,r0
        bne       L91                   ; Branch
                                        ; Branch Occurs to L91
L84:        
	.line	449
;------------------------------------------------------------------------------
; 1170 | nMaxDotLife = (UINT16) parameters->wOuterR[d];                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(201),r0
        sti       r0,*+fp(18)
	.line	450
;------------------------------------------------------------------------------
; 1171 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L86                   ; Branch
                                        ; Branch Occurs to L86
        stik      1,*+fp(18)
        bu        L88
                                        ; Branch Occurs to L88
L86:        
	.line	451
;------------------------------------------------------------------------------
; 1172 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bls       L88                   ; Branch
                                        ; Branch Occurs to L88
        ldiu      32767,r0
        sti       r0,*+fp(18)
L88:        
	.line	452
;------------------------------------------------------------------------------
; 1173 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L90
                                        ; Branch Occurs to L90
L89:        
	.line	454
;------------------------------------------------------------------------------
; 1175 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	455
;------------------------------------------------------------------------------
; 1176 | dotLife[dotPosOffset + k] = (xCoord % nMaxDotLife) + 1;                
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r1
        call      MOD_U40
                                        ; Call Occurs
        lda       *+fp(30),ar0
        lda       *+fp(35),ir0
        addi      *+fp(40),ir0          ; unsigned
        addi3     1,r0,r0
        sti       r0,*+ar0(ir0)
	.line	457
;------------------------------------------------------------------------------
; 1178 | fracDX[dotPosOffset + k] = 0;                                          
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(44),ir0          ; unsigned
        stik      0,*+ar0(ir0)
	.line	458
;------------------------------------------------------------------------------
; 1179 | fracDY[dotPosOffset + k] = 0;                                          
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(46),ir0          ; unsigned
        stik      0,*+ar0(ir0)
	.line	452
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L90:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L89                   ; Branch
                                        ; Branch Occurs to L89
L91:        
	.line	462
;------------------------------------------------------------------------------
; 1183 | if( u16Type == DL_NOISEDIR || u16Type == DL_NOISESPEED ) //    noise up
;     | date timer reset so that per-dot                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      11,r0
        beq       L93                   ; Branch
                                        ; Branch Occurs to L93
        cmpi      13,r0
        bne       L102                  ; Branch
                                        ; Branch Occurs to L102
L93:        
	.line	463
;------------------------------------------------------------------------------
; 1184 | nNoiseUpdTicks[d] = 0;                                //    noise facto
;     | rs are randomly chosen on the                                          
; 1185 | //    very first update frame!                                         
;------------------------------------------------------------------------------
        lda       *+fp(53),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
        bu        L102
                                        ; Branch Occurs to L102
L94:        
	.line	467
;------------------------------------------------------------------------------
; 1188 | else if( u16Type == OPTICFLOW )                             // OPTICFLO
;     | W: flow field. dot pos stored in                                       
; 1189 | {                                                           // polar co
;     | ords (r,TH) rather than (x,y) pix:                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      9,r0
        bne       L99                   ; Branch
                                        ; Branch Occurs to L99
	.line	469
;------------------------------------------------------------------------------
; 1190 | rectR = parameters->wRectR[d];                           //    inner ra
;     | dius in deg/100 of visual angle                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r8
	.line	470
;------------------------------------------------------------------------------
; 1191 | ++rectR;                                                 //    no dots 
;     | AT inner or outer rad initially                                        
;------------------------------------------------------------------------------
        addi      1,r8                  ; unsigned
	.line	471
;------------------------------------------------------------------------------
; 1192 | rectL = parameters->wRectL[d];                           //    outer ra
;     | dius in deg/100 of visual angle                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        lda       *+ar0(105),ar5
	.line	472
;------------------------------------------------------------------------------
; 1193 | rectL -= rectR;                                          //    differen
;     | ce in deg/100                                                          
;------------------------------------------------------------------------------
        subi3     r8,ar5,ar5            ; unsigned
	.line	474
;------------------------------------------------------------------------------
; 1195 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L97
                                        ; Branch Occurs to L97
L96:        
	.line	476
;------------------------------------------------------------------------------
; 1197 | xCoord = (UINT16) GetRandNum();                       //    init random
;     |  radial pos in visual deg/100                                          
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	477
;------------------------------------------------------------------------------
; 1198 | xdotpos[dotPosOffset + k] = rectR + (xCoord % rectL);                  
;------------------------------------------------------------------------------
        ldiu      ar5,r1
        call      MOD_U40
                                        ; Call Occurs
        lda       *+fp(30),ar0
        lda       *+fp(35),ir0
        addi      *+fp(38),ir0          ; unsigned
        addi3     r0,r8,r0              ; unsigned
        sti       r0,*+ar0(ir0)
	.line	478
;------------------------------------------------------------------------------
; 1199 | yCoord = (UINT16) GetRandNum();                       //    init angula
;     | r pos in deg/10                                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	479
;------------------------------------------------------------------------------
; 1200 | ydotpos[dotPosOffset + k] = (yCoord % 3600);                           
;------------------------------------------------------------------------------
        ldiu      3600,r1
        call      MOD_U40
                                        ; Call Occurs
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(39),ir0          ; unsigned
        sti       r0,*+ar0(ir0)
	.line	480
;------------------------------------------------------------------------------
; 1201 | dotLife[dotPosOffset + k] = 0;                        //    reset frac 
;     | pos change (<1/100deg)                                                 
; 1205 | else                                                        // ALL OTHE
;     | R TYPES:  random-dot texture drawn                                     
; 1206 | {                                                           // to fill 
;     | the entire screen...                                                   
;------------------------------------------------------------------------------
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(40),ir0          ; unsigned
        stik      0,*+ar0(ir0)
	.line	474
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L97:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L96                   ; Branch
                                        ; Branch Occurs to L96
        bu        L102
                                        ; Branch Occurs to L102
L99:        
	.line	486
;------------------------------------------------------------------------------
; 1207 | for ( k = 0; k < parameters->wNumDots[d]; k++ )                        
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        bu        L101
                                        ; Branch Occurs to L101
L100:        
	.line	488
;------------------------------------------------------------------------------
; 1209 | xdotpos[dotPosOffset + k] = (UINT16) GetRandNum();                     
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(38),ir0          ; unsigned
        sti       r0,*+ar0(ir0)
	.line	489
;------------------------------------------------------------------------------
; 1210 | ydotpos[dotPosOffset + k] = (UINT16) GetRandNum();                     
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       *+fp(35),ir0
        lda       *+fp(30),ar0
        addi      *+fp(39),ir0          ; unsigned
        sti       r0,*+ar0(ir0)
	.line	486
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
L101:        
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+fp(30),r0
        cmpi      *+ar0(41),r0
        blo       L100                  ; Branch
                                        ; Branch Occurs to L100
L102:        
	.line	493
;------------------------------------------------------------------------------
; 1214 | dotPosOffset += parameters->wNumDots[d];                    // move off
;     | set into dot pos arrays so that it                                     
; 1215 | // points to loc after current target's dots                           
; 1217 | }                                                                 // EN
;     | D:  process XYCORE_INIT command                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        addi      *+fp(35),r0           ; unsigned
        sti       r0,*+fp(35)
	.line	284
        ldiu      1,r0
        addi      *+fp(33),r0           ; unsigned
        sti       r0,*+fp(33)
        lda       *+fp(48),ar0
        cmpi3     *+ar0(4),r0
        blo       L27                   ; Branch
                                        ; Branch Occurs to L27
        bu        L491
                                        ; Branch Occurs to L491
L104:        
	.line	499
;------------------------------------------------------------------------------
; 1220 | else if( command == XYCORE_DOFRAME )                              // BE
;     | GIN:  process XYCORE_DOFRAME command...                                
;------------------------------------------------------------------------------
        ldiu      *+fp(19),r0
        cmpi      3,r0
        bne       L491                  ; Branch
                                        ; Branch Occurs to L491
	.line	501
;------------------------------------------------------------------------------
; 1222 | timdelnib = (UINT32) parameters->wDelayPerDot;                 // write
;     |  trig timing params to dotter board                                    
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(5),r0
        sti       r0,*+fp(23)
	.line	502
;------------------------------------------------------------------------------
; 1223 | if ( timdelnib > MAX_TRIGDEL )                                 // timin
;     | g register.  check values.                                             
;------------------------------------------------------------------------------
        cmpi      15,r0
        bls       L107                  ; Branch
                                        ; Branch Occurs to L107
	.line	503
;------------------------------------------------------------------------------
; 1224 | timdelnib = MAX_TRIGDEL;                                               
;------------------------------------------------------------------------------
        stik      15,*+fp(23)
L107:        
	.line	504
;------------------------------------------------------------------------------
; 1225 | timdurbyte = timdelnib + ((UINT32) parameters->wOnTimePerDot);         
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(6),r0
        addi      *+fp(23),r0           ; unsigned
        sti       r0,*+fp(22)
	.line	505
;------------------------------------------------------------------------------
; 1226 | if ( timdurbyte > MAX_TRIGLEN )                                        
;------------------------------------------------------------------------------
        cmpi      255,r0
        bls       L109                  ; Branch
                                        ; Branch Occurs to L109
	.line	506
;------------------------------------------------------------------------------
; 1227 | timdurbyte = MAX_TRIGLEN;                                              
;------------------------------------------------------------------------------
        ldiu      255,r0
        sti       r0,*+fp(22)
L109:        
	.line	507
;------------------------------------------------------------------------------
; 1228 | timvals = ((timdurbyte & 0x00F0) << 20) | (timdelnib << 20) | ((timdurb
;     | yte & 0x000F) << 28);                                                  
;------------------------------------------------------------------------------
        ldiu      240,r0
        ldiu      15,r1
        ldiu      *+fp(23),r2
        and       *+fp(22),r0
        and       *+fp(22),r1
        ash       20,r2
        ash       20,r0
        ash       28,r1
        or3       r0,r2,r0
        or3       r0,r1,r0
        sti       r0,*+fp(24)
	.line	508
;------------------------------------------------------------------------------
; 1229 | *timaddr = timvals;                                                    
;------------------------------------------------------------------------------
        lda       *+fp(27),ar0
        sti       r0,*ar0
	.line	510
;------------------------------------------------------------------------------
; 1231 | maxRepeats = 0;                                                // find 
;     | largest "#reps per frame" across all                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(36)
	.line	511
;------------------------------------------------------------------------------
; 1232 | for( d = 0; d < parameters->wNumTargets; d++ )                 // defin
;     | ed targets.  B/c "dotlife" tgts use the                                
; 1233 | {                                                              // upper
;     |  byte of NREPS field to store dot life                                 
;------------------------------------------------------------------------------
        stik      0,*+fp(33)
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        cmpi3     *+ar0(4),r0
        bhs       L117                  ; Branch
                                        ; Branch Occurs to L117
L110:        
	.line	513
;------------------------------------------------------------------------------
; 1234 | cd = d * UPDRECSZ;                                          // decremen
;     | t, we must mask that out here!                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(33),r0
        ash3      2,r0,r1
        addi3     r0,r1,r0              ; unsigned
        sti       r0,*+fp(34)
	.line	514
;------------------------------------------------------------------------------
; 1235 | u16Dummy = (UINT16) data[cd + NREPS];                                  
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	515
;------------------------------------------------------------------------------
; 1236 | if( parameters->wType[d] == DOTLIFEWIN ||                              
; 1237 | parameters->wType[d] == DL_NOISEDIR ||                                 
; 1238 | parameters->wType[d] == DL_NOISESPEED )                                
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi3     8,*+ar0(9)
        beq       L113                  ; Branch
                                        ; Branch Occurs to L113
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi3     11,*+ar0(9)
        beq       L113                  ; Branch
                                        ; Branch Occurs to L113
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        cmpi3     13,*+ar0(9)
        bne       L114                  ; Branch
                                        ; Branch Occurs to L114
L113:        
	.line	518
;------------------------------------------------------------------------------
; 1239 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
L114:        
	.line	519
;------------------------------------------------------------------------------
; 1240 | if ( ((UINT32) u16Dummy) > maxRepeats )                                
;------------------------------------------------------------------------------
        ldiu      *+fp(15),r0
        cmpi      *+fp(36),r0
        bls       L116                  ; Branch
                                        ; Branch Occurs to L116
	.line	520
;------------------------------------------------------------------------------
; 1241 | maxRepeats = (UINT32) u16Dummy;                                        
; 1244 | // these are only used by DL_NOISEDIR and DL_NOISESPEED. We put the val
;     | ues in local vars so we're not                                         
; 1245 | // constantly accessing the PARAMETERS struct in shared memory (slower 
;     | access)                                                                
;------------------------------------------------------------------------------
        sti       r0,*+fp(36)
L116:        
	.line	511
        ldiu      1,r0
        addi      *+fp(33),r0           ; unsigned
        sti       r0,*+fp(33)
        lda       *+fp(48),ar0
        cmpi3     *+ar0(4),r0
        blo       L110                  ; Branch
                                        ; Branch Occurs to L110
L117:        
	.line	525
;------------------------------------------------------------------------------
; 1246 | screenW_mm = parameters->wWidthMM;                                     
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(1),r0
        sti       r0,*+fp(12)
	.line	526
;------------------------------------------------------------------------------
; 1247 | screenH_mm = parameters->wHeightMM;                                    
;------------------------------------------------------------------------------
        lda       *+fp(48),ar0
        ldiu      *+ar0(2),r0
        sti       r0,*+fp(13)
	.line	528
;------------------------------------------------------------------------------
; 1249 | dotPosOffset = 0;                                              // BEGIN
;     | :  first pass thru all targets                                         
;------------------------------------------------------------------------------
        stik      0,*+fp(35)
	.line	529
;------------------------------------------------------------------------------
; 1250 | nTotalVisDots = 0;                                                     
;------------------------------------------------------------------------------
        ldiu      0,r0
        sti       r0,*+fp(65)
	.line	530
;------------------------------------------------------------------------------
; 1251 | if( maxRepeats > 0 ) for( d = 0; d < parameters->wNumTargets; d++ )    
;------------------------------------------------------------------------------
        ldiu      *+fp(36),r0
        cmpi      0,r0
        beq       L481                  ; Branch
                                        ; Branch Occurs to L481
        stik      0,*+fp(33)
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        cmpi3     *+ar0(4),r0
        bhs       L481                  ; Branch
                                        ; Branch Occurs to L481
L119:        
	.line	532
;------------------------------------------------------------------------------
; 1253 | cd = d * UPDRECSZ;                                          // offset i
;     | nto array of motion update records;                                    
; 1254 | // locates start of record for this tgt                                
;------------------------------------------------------------------------------
        ldiu      *+fp(33),r0
        ash3      2,r0,r1
        addi3     r0,r1,r0              ; unsigned
        sti       r0,*+fp(34)
	.line	534
;------------------------------------------------------------------------------
; 1255 | u16Type = parameters->wType[d];                             // this tar
;     | get's type                                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(9),r0
        sti       r0,*+fp(14)
	.line	536
;------------------------------------------------------------------------------
; 1257 | u16Dummy = (UINT16) data[cd + NREPS];                       // #reps pe
;     | r frame for this tgt (mask out dot                                     
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	537
;------------------------------------------------------------------------------
; 1258 | if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // life dec
;     | r in upper byte if "dotlife" type)                                     
; 1259 | u16Type == DL_NOISESPEED )                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      8,r0
        beq       L122                  ; Branch
                                        ; Branch Occurs to L122
        cmpi      11,r0
        beq       L122                  ; Branch
                                        ; Branch Occurs to L122
        cmpi      13,r0
        bne       L123                  ; Branch
                                        ; Branch Occurs to L123
L122:        
	.line	539
;------------------------------------------------------------------------------
; 1260 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
L123:        
	.line	541
;------------------------------------------------------------------------------
; 1262 | if( (u16Type == NO_TARGET) ||                               // if "non-
;     | target", if #dots = 0, or if #reps                                     
; 1263 | (parameters->wNumDots[d] == 0) || (u16Dummy == 0) ||    // is zero for 
;     | this target, or if it's a                                              
; 1264 | (u16Type == ORIBAR && parameters->wRectL[d] == 0) )     // zero-height 
;     | ORIBAR target, then skip to                                            
; 1265 | {                                                           // next tar
;     | get...                                                                 
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      0,r0
        beq       L128                  ; Branch
                                        ; Branch Occurs to L128
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        cmpi      0,r0
        beq       L128                  ; Branch
                                        ; Branch Occurs to L128
        ldiu      *+fp(15),r0
        cmpi      0,r0
        beq       L128                  ; Branch
                                        ; Branch Occurs to L128
        ldiu      *+fp(14),r0
        cmpi      10,r0
        bne       L129                  ; Branch
                                        ; Branch Occurs to L129
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(105),r0
        cmpi      0,r0
        bne       L129                  ; Branch
                                        ; Branch Occurs to L129
L128:        
	.line	545
;------------------------------------------------------------------------------
; 1266 | nRedrawsLeft[d] = 0;                                                   
;------------------------------------------------------------------------------
        lda       *+fp(51),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	546
;------------------------------------------------------------------------------
; 1267 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	547
;------------------------------------------------------------------------------
; 1268 | dotPosOffset += parameters->wNumDots[d];                               
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        addi      *+fp(35),r0           ; unsigned
        sti       r0,*+fp(35)
	.line	548
;------------------------------------------------------------------------------
; 1269 | continue;                                                              
;------------------------------------------------------------------------------
        bu        L480
                                        ; Branch Occurs to L480
L129:        
	.line	551
;------------------------------------------------------------------------------
; 1272 | hw = (INT16) data[cd + WIN_H];                              // target's
;     |  window pos change for current frame                                   
;------------------------------------------------------------------------------
        lda       *+fp(56),ir0
        lda       *+fp(34),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(3)
	.line	552
;------------------------------------------------------------------------------
; 1273 | vw = (INT16) data[cd + WIN_V];                                         
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(1),r0
        sti       r0,*+fp(4)
	.line	553
;------------------------------------------------------------------------------
; 1274 | hv = (INT16) data[cd + PAT_H];                              // tgt's pa
;     | ttern pos change for current frame                                     
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(2),r0
        sti       r0,*+fp(1)
	.line	554
;------------------------------------------------------------------------------
; 1275 | vv = (INT16) data[cd + PAT_V];                                         
; 1276 | // deal with special cases:                                            
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(3),r0
        sti       r0,*+fp(2)
	.line	556
;------------------------------------------------------------------------------
; 1277 | if( u16Type == STATICANNU )                                 //    STATI
;     | CANNU - no window or pattern motion                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      6,r0
        bne       L131                  ; Branch
                                        ; Branch Occurs to L131
	.line	557
;------------------------------------------------------------------------------
; 1278 | hw = vw = hv = vv = 0;                                                 
;------------------------------------------------------------------------------
        ldiu      0,r1
        ldiu      r1,r0
        sti       r1,*+fp(2)
        sti       r1,*+fp(1)
        sti       r0,*+fp(4)
        sti       r0,*+fp(3)
L131:        
	.line	558
;------------------------------------------------------------------------------
; 1279 | if( u16Type == FULLSCREEN )                                 //    FULLS
;     | CREEN - no window                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      2,r0
        bne       L133                  ; Branch
                                        ; Branch Occurs to L133
	.line	559
;------------------------------------------------------------------------------
; 1280 | hw = vw = 0;                                                           
;------------------------------------------------------------------------------
        ldiu      0,r0
        sti       r0,*+fp(4)
        sti       r0,*+fp(3)
L133:        
	.line	560
;------------------------------------------------------------------------------
; 1281 | if( (u16Type == DOTARRAY) || (u16Type == ORIBAR) )          //    DOTAR
;     | RAY/ORIBAR - dots move together as                                     
; 1282 | {                                                           //    an ob
;     | ject. there's no window or pattern                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      1,r0
        beq       L135                  ; Branch
                                        ; Branch Occurs to L135
        cmpi      10,r0
        bne       L136                  ; Branch
                                        ; Branch Occurs to L136
L135:        
	.line	562
;------------------------------------------------------------------------------
; 1283 | hv = hw;                                                 //    in the s
;     | ense of the windowed tgt types;                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        sti       r0,*+fp(1)
	.line	563
;------------------------------------------------------------------------------
; 1284 | vv = vw;                                                 //    all dots
;     |  drawn. assign "window" to                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        sti       r0,*+fp(2)
	.line	564
;------------------------------------------------------------------------------
; 1285 | hw = vw = 0;                                             //    "pattern
;     | " vel, so we can implement like                                        
; 1286 | }                                                           //    the F
;     | ULLSCREEN tgt type (see below)                                         
;------------------------------------------------------------------------------
        ldiu      0,r0
        sti       r0,*+fp(3)
        sti       r0,*+fp(4)
L136:        
	.line	567
;------------------------------------------------------------------------------
; 1288 | if( u16Type != OPTICFLOW )                                  // update t
;     | arget window location.                                                 
; 1289 | {                                                           // !!! UINT
;     | 16 arithmetic! Windows wrap around                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      9,r0
        beq       L139                  ; Branch
                                        ; Branch Occurs to L139
	.line	569
;------------------------------------------------------------------------------
; 1290 | parameters->wRectR[d] += hw;                             // screen on D
;     | etroit/Daytona.  Won't happen on                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(33),r0
        addi      *+fp(48),r0           ; unsigned
        addi3     73,r0,ar0
        ldiu      *+fp(3),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	570
;------------------------------------------------------------------------------
; 1291 | parameters->wRectL[d] += hw;                             // Dakar, b/c 
;     | UINT16 is actually UINT32.  It is                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(33),r0
        addi      *+fp(48),r0           ; unsigned
        addi3     105,r0,ar0
        ldiu      *+fp(3),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	571
;------------------------------------------------------------------------------
; 1292 | parameters->wRectT[d] += vw;                             // considered 
;     | an error on user's part to have a                                      
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      137,ar0               ; unsigned
        ldiu      *+fp(4),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	572
;------------------------------------------------------------------------------
; 1293 | parameters->wRectB[d] += vw;                             // target wind
;     | ow go past screen bounds!!!                                            
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      169,ar0               ; unsigned
        ldiu      *+fp(4),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	574
;------------------------------------------------------------------------------
; 1295 | rectR = parameters->wRectR[d];                           // save curren
;     | t window bounds in register vars                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r8
	.line	575
;------------------------------------------------------------------------------
; 1296 | rectL = parameters->wRectL[d];                           // to speed up
;     |  comparisons which must be                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        lda       *+ar0(105),ar5
	.line	576
;------------------------------------------------------------------------------
; 1297 | rectU = parameters->wRectT[d];                           // performed f
;     | or all dots                                                            
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(137),r0
        sti       r0,*+fp(63)
	.line	577
;------------------------------------------------------------------------------
; 1298 | rectD = parameters->wRectB[d];                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(169),r0
        sti       r0,*+fp(64)
	.line	579
;------------------------------------------------------------------------------
; 1300 | if( u16Type == ANNULUS )                                 // must update
;     |  outer rect as well for ANNULUS;                                       
; 1301 | {                                                        // note that w
;     | e DO NOT assign register vars to                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      5,r0
        bne       L148                  ; Branch
                                        ; Branch Occurs to L148
	.line	581
;------------------------------------------------------------------------------
; 1302 | parameters->wOuterR[d] += hw;                         // the bounds of 
;     | the outer rect.                                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      201,ar0               ; unsigned
        ldiu      *+fp(3),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	582
;------------------------------------------------------------------------------
; 1303 | parameters->wOuterL[d] += hw;                                          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      233,ar0               ; unsigned
        ldiu      *+fp(3),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	583
;------------------------------------------------------------------------------
; 1304 | parameters->wOuterT[d] += vw;                                          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      265,ar0               ; unsigned
        ldiu      *+fp(4),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
	.line	584
;------------------------------------------------------------------------------
; 1305 | parameters->wOuterB[d] += vw;                                          
; 1308 | else                                                        // OPTICFLO
;     | W target is very different: window                                     
; 1309 | {                                                           // is moved
;     |  by changing coords of the FOE...                                      
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        addi      297,ar0               ; unsigned
        ldiu      *+fp(4),r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
        bu        L148
                                        ; Branch Occurs to L148
L139:        
	.line	589
;------------------------------------------------------------------------------
; 1310 | rectR = (UINT16) parameters->wRectR[d];                  //    inner ra
;     | dius in visual deg/100                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(73),r8
	.line	590
;------------------------------------------------------------------------------
; 1311 | rectL = (UINT16) parameters->wRectL[d];                  //    outer ra
;     | dius in visual deg/100                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        lda       *+ar0(105),ar5
	.line	591
;------------------------------------------------------------------------------
; 1312 | rectU = (UINT16) parameters->wRectT[d];                  //    alphaX g
;     | eometric conversion fac (* 1024)                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(137),r0
        sti       r0,*+fp(63)
	.line	592
;------------------------------------------------------------------------------
; 1313 | rectD = (UINT16) parameters->wRectB[d];                  //    alphaY g
;     | eometric conversion fac (* 1024)                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(169),r0
        sti       r0,*+fp(64)
	.line	594
;------------------------------------------------------------------------------
; 1315 | xCoord = parameters->wOuterR[d];                         //    update c
;     | oords of the FOE now...                                                
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        lda       *+ar0(201),ar4
	.line	595
;------------------------------------------------------------------------------
; 1316 | xCoord += hw;                                                          
;------------------------------------------------------------------------------
        addi      *+fp(3),ar4           ; unsigned
	.line	596
;------------------------------------------------------------------------------
; 1317 | yCoord = parameters->wOuterL[d];                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r5
	.line	597
;------------------------------------------------------------------------------
; 1318 | yCoord += vw;                                                          
; 1319 | #ifdef _TGTDAKARF5                                       //    ensure U
;     | INT16 arith on 32-bit-only DAKAR                                       
;------------------------------------------------------------------------------
        addi      *+fp(4),r5            ; unsigned
	.line	599
;------------------------------------------------------------------------------
; 1320 | if( xCoord > 0x0000FFFF )                                              
;------------------------------------------------------------------------------
        cmpi      @CL3,ar4
        bls       L143                  ; Branch
                                        ; Branch Occurs to L143
	.line	601
;------------------------------------------------------------------------------
; 1322 | if( hw > 0 ) xCoord -= 65536;                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        cmpi      0,r0
        ble       L142                  ; Branch
                                        ; Branch Occurs to L142
        subi      @CL20,ar4             ; unsigned
        bu        L143
                                        ; Branch Occurs to L143
L142:        
	.line	602
;------------------------------------------------------------------------------
; 1323 | else         xCoord &= 0x0000FFFF;                                     
;------------------------------------------------------------------------------
        and       65535,ar4
L143:        
	.line	604
;------------------------------------------------------------------------------
; 1325 | if( yCoord > 0x0000FFFF )                                              
;------------------------------------------------------------------------------
        cmpi      @CL3,r5
        bls       L147                  ; Branch
                                        ; Branch Occurs to L147
	.line	606
;------------------------------------------------------------------------------
; 1327 | if( vw > 0 ) yCoord -= 65536;                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        cmpi      0,r0
        ble       L146                  ; Branch
                                        ; Branch Occurs to L146
        subi      @CL20,r5              ; unsigned
        bu        L147
                                        ; Branch Occurs to L147
L146:        
	.line	607
;------------------------------------------------------------------------------
; 1328 | else         yCoord &= 0x0000FFFF;                                     
; 1330 | #endif                                                                 
;------------------------------------------------------------------------------
        and       65535,r5
L147:        
	.line	610
;------------------------------------------------------------------------------
; 1331 | parameters->wOuterR[d] = xCoord;                         //    the new 
;     | FOE coords are also preserved                                          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        sti       ar4,*+ar0(201)
	.line	611
;------------------------------------------------------------------------------
; 1332 | parameters->wOuterL[d] = yCoord;                         //    in the r
;     | egister vars (xCoord, yCoord)...                                       
; 1333 | //    also, reg vars (hv,vv) = (B*2^M, M)!                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        sti       r5,*+ar0(233)
L148:        
	.line	615
;------------------------------------------------------------------------------
; 1336 | de = &xdotpos[ dotPosOffset + parameters->wNumDots[d] ];    // set ptrs
;     |  into target's dot position, dot                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        ldiu      *+fp(35),r0
        addi      *+fp(48),ar0          ; unsigned
        addi      *+fp(38),r0           ; unsigned
        addi      *+ar0(41),r0          ; unsigned
        sti       r0,*+fp(66)
	.line	616
;------------------------------------------------------------------------------
; 1337 | a  = &xdotpos[dotPosOffset];                                // lifetime
;     | , dot noise, and fracDX,DY arrays                                      
;------------------------------------------------------------------------------
        lda       *+fp(35),ar6
        addi      *+fp(38),ar6          ; unsigned
	.line	617
;------------------------------------------------------------------------------
; 1338 | b  = &ydotpos[dotPosOffset];                                           
;------------------------------------------------------------------------------
        lda       *+fp(35),ar7
        addi      *+fp(39),ar7          ; unsigned
	.line	618
;------------------------------------------------------------------------------
; 1339 | nextDotLife = &dotLife[dotPosOffset];                                  
;------------------------------------------------------------------------------
        ldiu      *+fp(35),r0
        addi      *+fp(40),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	619
;------------------------------------------------------------------------------
; 1340 | nextDotNoise = &dotNoise[dotPosOffset];                                
;------------------------------------------------------------------------------
        ldiu      *+fp(35),r0
        addi      *+fp(42),r0           ; unsigned
        sti       r0,*+fp(43)
	.line	620
;------------------------------------------------------------------------------
; 1341 | nextFracDX = &fracDX[dotPosOffset];                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(35),r0
        addi      *+fp(44),r0           ; unsigned
        sti       r0,*+fp(45)
	.line	621
;------------------------------------------------------------------------------
; 1342 | nextFracDY = &fracDY[dotPosOffset];                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(35),r0
        addi      *+fp(46),r0           ; unsigned
        sti       r0,*+fp(47)
	.line	622
;------------------------------------------------------------------------------
; 1343 | dotPosOffset += parameters->wNumDots[d];                    // now poin
;     | ts to start of next target's dots                                      
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(41),r0
        addi      *+fp(35),r0           ; unsigned
        sti       r0,*+fp(35)
	.line	625
;------------------------------------------------------------------------------
; 1346 | if( (u16Type == DOTARRAY) ||                                // DOTARRAY
;     | /FULLSCREEN/ORIBAR:  Every tgt dot                                     
; 1347 | (u16Type == FULLSCREEN) ||                              // is always dr
;     | awn -- there's no "window" that                                        
; 1348 | (u16Type == ORIBAR) )                                   // is distinct 
;     | from the dot pattern.                                                  
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      1,r0
        beq       L151                  ; Branch
                                        ; Branch Occurs to L151
        cmpi      2,r0
        beq       L151                  ; Branch
                                        ; Branch Occurs to L151
        cmpi      10,r0
        bne       L164                  ; Branch
                                        ; Branch Occurs to L164
L151:        
	.line	629
;------------------------------------------------------------------------------
; 1350 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    #dots in
;     |  "visible dots" array for tgt                                          
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
	.line	630
;------------------------------------------------------------------------------
; 1351 | while( a < de )                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L152:        
	.line	632
;------------------------------------------------------------------------------
; 1353 | *a = *a + hv;                                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        addi      *ar6,r0               ; unsigned
        sti       r0,*ar6
	.line	633
;------------------------------------------------------------------------------
; 1354 | *b = *b + vv;                                                          
; 1355 | #ifdef _TGTDAKARF5                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        addi      *ar7,r0               ; unsigned
        sti       r0,*ar7
	.line	635
;------------------------------------------------------------------------------
; 1356 | if( *a > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi      @CL3,r0
        bls       L156                  ; Branch
                                        ; Branch Occurs to L156
	.line	637
;------------------------------------------------------------------------------
; 1358 | if( hv > 0 ) *a -= 65536;                                              
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        cmpi      0,r0
        ble       L155                  ; Branch
                                        ; Branch Occurs to L155
        ldiu      @CL20,r0
        subri     *ar6,r0               ; unsigned
        sti       r0,*ar6
        bu        L156
                                        ; Branch Occurs to L156
L155:        
	.line	638
;------------------------------------------------------------------------------
; 1359 | else         *a &= 0x0000FFFF;                                         
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        and       65535,r0
        sti       r0,*ar6
L156:        
	.line	640
;------------------------------------------------------------------------------
; 1361 | if( *b > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        cmpi      @CL3,r0
        bls       L160                  ; Branch
                                        ; Branch Occurs to L160
	.line	642
;------------------------------------------------------------------------------
; 1363 | if( vv > 0 ) *b -= 65536;                                              
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      0,r0
        ble       L159                  ; Branch
                                        ; Branch Occurs to L159
        ldiu      @CL20,r0
        subri     *ar7,r0               ; unsigned
        sti       r0,*ar7
        bu        L160
                                        ; Branch Occurs to L160
L159:        
	.line	643
;------------------------------------------------------------------------------
; 1364 | else         *b &= 0x0000FFFF;                                         
; 1366 | #endif                                                                 
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        and       65535,r0
        sti       r0,*ar7
L160:        
	.line	647
;------------------------------------------------------------------------------
; 1368 | xyvals = ((*a << 16) | *b);                           //    draw the do
;     | t                                                                      
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L161:        
	.line	648
;------------------------------------------------------------------------------
; 1369 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L161                  ; Branch
                                        ; Branch Occurs to L161
	.line	649
;------------------------------------------------------------------------------
; 1370 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	650
;------------------------------------------------------------------------------
; 1371 | visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed
;     |  (X,Y) pos of each visible dot                                         
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	651
;------------------------------------------------------------------------------
; 1372 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	652
;------------------------------------------------------------------------------
; 1373 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	630
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L152                  ; Branch
                                        ; Branch Occurs to L152
        bu        L475
                                        ; Branch Occurs to L475
L164:        
	.line	655
;------------------------------------------------------------------------------
; 1376 | else if( u16Type == STATICANNU )                            // STATICAN
;     | NU: Neither window nor pattern move,                                   
; 1377 | {                                                           // so no ne
;     | ed to update dot pos nor to make                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      6,r0
        bne       L170                  ; Branch
                                        ; Branch Occurs to L170
	.line	657
;------------------------------------------------------------------------------
; 1378 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             // sure that d
;     | ot is visible...                                                       
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
	.line	658
;------------------------------------------------------------------------------
; 1379 | while( a < de )                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L166:        
	.line	660
;------------------------------------------------------------------------------
; 1381 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L167:        
	.line	661
;------------------------------------------------------------------------------
; 1382 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L167                  ; Branch
                                        ; Branch Occurs to L167
	.line	662
;------------------------------------------------------------------------------
; 1383 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	663
;------------------------------------------------------------------------------
; 1384 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	664
;------------------------------------------------------------------------------
; 1385 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	665
;------------------------------------------------------------------------------
; 1386 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	658
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L166                  ; Branch
                                        ; Branch Occurs to L166
        bu        L475
                                        ; Branch Occurs to L475
L170:        
	.line	668
;------------------------------------------------------------------------------
; 1389 | else if( u16Type == RECTWINDOW )                            // RECTWIND
;     | OW:  Independent pattern & window                                      
; 1390 | {                                                           // motion. 
;     |  Visible dots lie inside window.                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      3,r0
        bne       L189                  ; Branch
                                        ; Branch Occurs to L189
	.line	670
;------------------------------------------------------------------------------
; 1391 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	671
;------------------------------------------------------------------------------
; 1392 | while( a < de )                                                        
; 1394 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1395 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L172:        
	.line	675
;------------------------------------------------------------------------------
; 1396 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi      *+fp(1),r0            ; unsigned
        sti       r0,*ar6
	.line	676
;------------------------------------------------------------------------------
; 1397 | *b = *b + vw + vv;                                                     
; 1398 | #ifdef _TGTDAKARF5                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi      *+fp(2),r0            ; unsigned
        sti       r0,*ar7
	.line	678
;------------------------------------------------------------------------------
; 1399 | if( *a > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi      @CL3,r0
        bls       L176                  ; Branch
                                        ; Branch Occurs to L176
	.line	680
;------------------------------------------------------------------------------
; 1401 | if(hw+hv > 0) *a -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        addi      *+fp(3),r0
        cmpi      0,r0
        ble       L175                  ; Branch
                                        ; Branch Occurs to L175
        ldiu      @CL20,r0
        subri     *ar6,r0               ; unsigned
        sti       r0,*ar6
        bu        L176
                                        ; Branch Occurs to L176
L175:        
	.line	681
;------------------------------------------------------------------------------
; 1402 | else          *a &= 0x0000FFFF;                                        
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        and       65535,r0
        sti       r0,*ar6
L176:        
	.line	683
;------------------------------------------------------------------------------
; 1404 | if ( *b > 0x0000FFFF )                                                 
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        cmpi      @CL3,r0
        bls       L180                  ; Branch
                                        ; Branch Occurs to L180
	.line	685
;------------------------------------------------------------------------------
; 1406 | if(vw+vv > 0) *b -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        addi      *+fp(4),r0
        cmpi      0,r0
        ble       L179                  ; Branch
                                        ; Branch Occurs to L179
        ldiu      @CL20,r0
        subri     *ar7,r0               ; unsigned
        sti       r0,*ar7
        bu        L180
                                        ; Branch Occurs to L180
L179:        
	.line	686
;------------------------------------------------------------------------------
; 1407 | else          *b &= 0x0000FFFF;                                        
; 1409 | #endif                                                                 
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        and       65535,r0
        sti       r0,*ar7
L180:        
	.line	690
;------------------------------------------------------------------------------
; 1411 | if( (*a <= rectR) && (*a >= rectL) && (*b <= rectU) && (*b >= rectD) ) 
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi3     r8,r0
        bhi       L187                  ; Branch
                                        ; Branch Occurs to L187
        ldiu      *ar6,r0
        cmpi3     ar5,r0
        blo       L187                  ; Branch
                                        ; Branch Occurs to L187
        ldiu      *+fp(63),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        bhi       L187                  ; Branch
                                        ; Branch Occurs to L187
        ldiu      *+fp(64),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        blo       L187                  ; Branch
                                        ; Branch Occurs to L187
	.line	692
;------------------------------------------------------------------------------
; 1413 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L185:        
	.line	693
;------------------------------------------------------------------------------
; 1414 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L185                  ; Branch
                                        ; Branch Occurs to L185
	.line	694
;------------------------------------------------------------------------------
; 1415 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	695
;------------------------------------------------------------------------------
; 1416 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	696
;------------------------------------------------------------------------------
; 1417 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(52),ar0          ; unsigned
        ldiu      1,r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
L187:        
	.line	698
;------------------------------------------------------------------------------
; 1419 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	699
;------------------------------------------------------------------------------
; 1420 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	671
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L172                  ; Branch
                                        ; Branch Occurs to L172
        bu        L475
                                        ; Branch Occurs to L475
L189:        
	.line	702
;------------------------------------------------------------------------------
; 1423 | else if( u16Type == RECTHOLE )                              // RECTHOLE
;     | :  Independent window & pattern                                        
; 1424 | {                                                           // motion. 
;     |  Visible dots lie outside window.                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      4,r0
        bne       L208                  ; Branch
                                        ; Branch Occurs to L208
	.line	704
;------------------------------------------------------------------------------
; 1425 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	705
;------------------------------------------------------------------------------
; 1426 | while( a < de )                                                        
; 1428 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1429 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L191:        
	.line	709
;------------------------------------------------------------------------------
; 1430 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi      *+fp(1),r0            ; unsigned
        sti       r0,*ar6
	.line	710
;------------------------------------------------------------------------------
; 1431 | *b = *b + vw + vv;                                                     
; 1432 | #ifdef _TGTDAKARF5                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi      *+fp(2),r0            ; unsigned
        sti       r0,*ar7
	.line	712
;------------------------------------------------------------------------------
; 1433 | if( *a > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi      @CL3,r0
        bls       L195                  ; Branch
                                        ; Branch Occurs to L195
	.line	714
;------------------------------------------------------------------------------
; 1435 | if(hw+hv > 0) *a -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        addi      *+fp(3),r0
        cmpi      0,r0
        ble       L194                  ; Branch
                                        ; Branch Occurs to L194
        ldiu      @CL20,r0
        subri     *ar6,r0               ; unsigned
        sti       r0,*ar6
        bu        L195
                                        ; Branch Occurs to L195
L194:        
	.line	715
;------------------------------------------------------------------------------
; 1436 | else          *a &= 0x0000FFFF;                                        
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        and       65535,r0
        sti       r0,*ar6
L195:        
	.line	717
;------------------------------------------------------------------------------
; 1438 | if( *b > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        cmpi      @CL3,r0
        bls       L199                  ; Branch
                                        ; Branch Occurs to L199
	.line	719
;------------------------------------------------------------------------------
; 1440 | if(vw+vv > 0) *b -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        addi      *+fp(4),r0
        cmpi      0,r0
        ble       L198                  ; Branch
                                        ; Branch Occurs to L198
        ldiu      @CL20,r0
        subri     *ar7,r0               ; unsigned
        sti       r0,*ar7
        bu        L199
                                        ; Branch Occurs to L199
L198:        
	.line	720
;------------------------------------------------------------------------------
; 1441 | else          *b &= 0x0000FFFF;                                        
; 1443 | #endif                                                                 
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        and       65535,r0
        sti       r0,*ar7
L199:        
	.line	724
;------------------------------------------------------------------------------
; 1445 | if( (*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD) )     
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi3     r8,r0
        bhi       L203                  ; Branch
                                        ; Branch Occurs to L203
        ldiu      *ar6,r0
        cmpi3     ar5,r0
        blo       L203                  ; Branch
                                        ; Branch Occurs to L203
        ldiu      *+fp(63),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        bhi       L203                  ; Branch
                                        ; Branch Occurs to L203
        ldiu      *+fp(64),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        bhs       L206                  ; Branch
                                        ; Branch Occurs to L206
L203:        
	.line	726
;------------------------------------------------------------------------------
; 1447 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L204:        
	.line	727
;------------------------------------------------------------------------------
; 1448 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L204                  ; Branch
                                        ; Branch Occurs to L204
	.line	728
;------------------------------------------------------------------------------
; 1449 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	729
;------------------------------------------------------------------------------
; 1450 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	730
;------------------------------------------------------------------------------
; 1451 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(52),ar0          ; unsigned
        ldiu      1,r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
L206:        
	.line	732
;------------------------------------------------------------------------------
; 1453 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	733
;------------------------------------------------------------------------------
; 1454 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	705
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L191                  ; Branch
                                        ; Branch Occurs to L191
        bu        L475
                                        ; Branch Occurs to L475
L208:        
	.line	736
;------------------------------------------------------------------------------
; 1457 | else if( u16Type == ANNULUS )                               // ANNULUS:
;     |   Independent window and pattern                                       
; 1458 | {                                                           // motion. 
;     |  Visible dots lie inside annulus.                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      5,r0
        bne       L231                  ; Branch
                                        ; Branch Occurs to L231
	.line	738
;------------------------------------------------------------------------------
; 1459 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	739
;------------------------------------------------------------------------------
; 1460 | while( a < de )                                                        
; 1462 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1463 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L210:        
	.line	743
;------------------------------------------------------------------------------
; 1464 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi      *+fp(1),r0            ; unsigned
        sti       r0,*ar6
	.line	744
;------------------------------------------------------------------------------
; 1465 | *b = *b + vw + vv;                                                     
; 1466 | #ifdef _TGTDAKARF5                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi      *+fp(2),r0            ; unsigned
        sti       r0,*ar7
	.line	746
;------------------------------------------------------------------------------
; 1467 | if( *a > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        cmpi      @CL3,r0
        bls       L214                  ; Branch
                                        ; Branch Occurs to L214
	.line	748
;------------------------------------------------------------------------------
; 1469 | if(hw+hv > 0) *a -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        addi      *+fp(3),r0
        cmpi      0,r0
        ble       L213                  ; Branch
                                        ; Branch Occurs to L213
        ldiu      @CL20,r0
        subri     *ar6,r0               ; unsigned
        sti       r0,*ar6
        bu        L214
                                        ; Branch Occurs to L214
L213:        
	.line	749
;------------------------------------------------------------------------------
; 1470 | else          *a &= 0x0000FFFF;                                        
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        and       65535,r0
        sti       r0,*ar6
L214:        
	.line	751
;------------------------------------------------------------------------------
; 1472 | if( *b > 0x0000FFFF )                                                  
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        cmpi      @CL3,r0
        bls       L218                  ; Branch
                                        ; Branch Occurs to L218
	.line	753
;------------------------------------------------------------------------------
; 1474 | if(vw+vv > 0) *b -= 65536;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        addi      *+fp(4),r0
        cmpi      0,r0
        ble       L217                  ; Branch
                                        ; Branch Occurs to L217
        ldiu      @CL20,r0
        subri     *ar7,r0               ; unsigned
        sti       r0,*ar7
        bu        L218
                                        ; Branch Occurs to L218
L217:        
	.line	754
;------------------------------------------------------------------------------
; 1475 | else          *b &= 0x0000FFFF;                                        
; 1477 | #endif                                                                 
;------------------------------------------------------------------------------
        ldiu      *ar7,r0
        and       65535,r0
        sti       r0,*ar7
L218:        
	.line	758
;------------------------------------------------------------------------------
; 1479 | if( (*a <= parameters->wOuterR[d]) && (*a >= parameters->wOuterL[d]) &&
;     |                                                                        
; 1480 | (*b <= parameters->wOuterT[d]) && (*b >= parameters->wOuterB[d]) &&    
; 1481 | ((*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD)) )       
;------------------------------------------------------------------------------
        lda       *+fp(33),ir0
        lda       201,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),*ar6
        bhi       L229                  ; Branch
                                        ; Branch Occurs to L229
        lda       *+fp(33),ir0
        lda       233,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),*ar6
        blo       L229                  ; Branch
                                        ; Branch Occurs to L229
        lda       *+fp(33),ir0
        lda       265,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),*ar7
        bhi       L229                  ; Branch
                                        ; Branch Occurs to L229
        lda       *+fp(33),ir0
        lda       297,ar0
        addi      *+fp(48),ir0          ; unsigned
        cmpi3     *+ar0(ir0),*ar7
        blo       L229                  ; Branch
                                        ; Branch Occurs to L229
        ldiu      *ar6,r0
        cmpi3     r8,r0
        bhi       L226                  ; Branch
                                        ; Branch Occurs to L226
        ldiu      *ar6,r0
        cmpi3     ar5,r0
        blo       L226                  ; Branch
                                        ; Branch Occurs to L226
        ldiu      *+fp(63),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        bhi       L226                  ; Branch
                                        ; Branch Occurs to L226
        ldiu      *+fp(64),r1
        ldiu      *ar7,r0
        cmpi3     r1,r0
        bhs       L229                  ; Branch
                                        ; Branch Occurs to L229
L226:        
	.line	762
;------------------------------------------------------------------------------
; 1483 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L227:        
	.line	763
;------------------------------------------------------------------------------
; 1484 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L227                  ; Branch
                                        ; Branch Occurs to L227
	.line	764
;------------------------------------------------------------------------------
; 1485 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	765
;------------------------------------------------------------------------------
; 1486 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	766
;------------------------------------------------------------------------------
; 1487 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(52),ar0          ; unsigned
        ldiu      1,r0
        addi      *ar0,r0               ; unsigned
        sti       r0,*ar0
L229:        
	.line	768
;------------------------------------------------------------------------------
; 1489 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	769
;------------------------------------------------------------------------------
; 1490 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	739
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L210                  ; Branch
                                        ; Branch Occurs to L210
        bu        L475
                                        ; Branch Occurs to L475
L231:        
	.line	772
;------------------------------------------------------------------------------
; 1493 | else if( u16Type == OPTRECTWIN )                            // OPTRECTW
;     | IN: Independent pattern & window                                       
; 1494 | {                                                           // motion, 
;     | but all dots restricted to window...                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      7,r0
        bne       L259                  ; Branch
                                        ; Branch Occurs to L259
	.line	774
;------------------------------------------------------------------------------
; 1495 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	775
;------------------------------------------------------------------------------
; 1496 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	777
;------------------------------------------------------------------------------
; 1498 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1499 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L234                  ; Branch
                                        ; Branch Occurs to L234
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L235                  ; Branch
                                        ; Branch Occurs to L235
L234:        
	.line	779
;------------------------------------------------------------------------------
; 1500 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	780
;------------------------------------------------------------------------------
; 1501 | a = de;                                                                
; 1503 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L236
                                        ; Branch Occurs to L236
L235:        
	.line	783
;------------------------------------------------------------------------------
; 1504 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L236:        
	.line	785
;------------------------------------------------------------------------------
; 1506 | while ( a < de )                                         //    for each
;     |  dot in target:                                                        
; 1508 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1509 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L237:        
	.line	789
;------------------------------------------------------------------------------
; 1510 | xCoord = *a + hw + hv;                                //       update i
;     | ts position                                                            
;------------------------------------------------------------------------------
        lda       *+fp(3),ar4
        addi      *ar6,ar4              ; unsigned
        addi      *+fp(1),ar4           ; unsigned
	.line	790
;------------------------------------------------------------------------------
; 1511 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r5
        addi      *ar7,r5               ; unsigned
        addi      *+fp(2),r5            ; unsigned
	.line	791
;------------------------------------------------------------------------------
; 1512 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if tgt h
;     | as violated horizontal bounds:                                         
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L239                  ; Branch
                                        ; Branch Occurs to L239
        cmpi3     ar5,ar4
        bhs       L246                  ; Branch
                                        ; Branch Occurs to L246
L239:        
	.line	793
;------------------------------------------------------------------------------
; 1514 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //          compute 
;     | positive distance by which                                             
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L241                  ; Branch
                                        ; Branch Occurs to L241
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L242
                                        ; Branch Occurs to L242
L241:        
	.line	794
;------------------------------------------------------------------------------
; 1515 | else                 u16Over = rectL - xCoord;     //          dot has 
;     | moved beyond border                                                    
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L242:        
	.line	795
;------------------------------------------------------------------------------
; 1516 | u16Over = u16Over % rectW;                         //          in case 
;     | distance > window width!                                               
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	797
;------------------------------------------------------------------------------
; 1518 | if( hv > 0 )  xCoord = rectL + u16Over;            //          if dots 
;     | moving right wrt window,                                               
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        cmpi      0,r0
        ble       L244                  ; Branch
                                        ; Branch Occurs to L244
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L245
                                        ; Branch Occurs to L245
L244:        
	.line	798
;------------------------------------------------------------------------------
; 1519 | else          xCoord = rectR - u16Over;            //          offset f
;     | rom left edge, else right                                              
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L245:        
	.line	800
;------------------------------------------------------------------------------
; 1521 | yCoord = (UINT16) GetRandNum();                    //          and rand
;     | omize the vertical coord                                               
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	801
;------------------------------------------------------------------------------
; 1522 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L255
                                        ; Branch Occurs to L255
L246:        
	.line	803
;------------------------------------------------------------------------------
; 1524 | else if( (yCoord > rectU) || (yCoord < rectD) )       //       else if 
;     | tgt violated vertical bounds:                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L248                  ; Branch
                                        ; Branch Occurs to L248
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L255                  ; Branch
                                        ; Branch Occurs to L255
L248:        
	.line	805
;------------------------------------------------------------------------------
; 1526 | if( yCoord > rectU ) u16Over = yCoord - rectU;     //          dist dot
;     |  moved beyond border...                                                
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L250                  ; Branch
                                        ; Branch Occurs to L250
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L251
                                        ; Branch Occurs to L251
L250:        
	.line	806
;------------------------------------------------------------------------------
; 1527 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L251:        
	.line	807
;------------------------------------------------------------------------------
; 1528 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	809
;------------------------------------------------------------------------------
; 1530 | if( vv > 0 )  yCoord = rectD + u16Over;            //          if dots 
;     | moving up wrt window,                                                  
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      0,r0
        ble       L253                  ; Branch
                                        ; Branch Occurs to L253
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L254
                                        ; Branch Occurs to L254
L253:        
	.line	810
;------------------------------------------------------------------------------
; 1531 | else          yCoord = rectU - u16Over;            //          offset f
;     | rom bottom edge, else top                                              
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L254:        
	.line	812
;------------------------------------------------------------------------------
; 1533 | xCoord = (UINT16) GetRandNum();                    //          and rand
;     | omize the horizontal coord                                             
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	813
;------------------------------------------------------------------------------
; 1534 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L255:        
	.line	815
;------------------------------------------------------------------------------
; 1536 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	816
;------------------------------------------------------------------------------
; 1537 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	818
;------------------------------------------------------------------------------
; 1539 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L256:        
	.line	819
;------------------------------------------------------------------------------
; 1540 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L256                  ; Branch
                                        ; Branch Occurs to L256
	.line	820
;------------------------------------------------------------------------------
; 1541 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	821
;------------------------------------------------------------------------------
; 1542 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	822
;------------------------------------------------------------------------------
; 1543 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	823
;------------------------------------------------------------------------------
; 1544 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	785
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L237                  ; Branch
                                        ; Branch Occurs to L237
        bu        L475
                                        ; Branch Occurs to L475
L259:        
	.line	826
;------------------------------------------------------------------------------
; 1547 | else if( u16Type == OPTCOHERENT )                           // OPTCOHER
;     | ENT: Like OPTRECTWIN, but implements                                   
; 1548 | {                                                           // percent 
;     | coherence...                                                           
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      12,r0
        bne       L289                  ; Branch
                                        ; Branch Occurs to L289
	.line	828
;------------------------------------------------------------------------------
; 1549 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	829
;------------------------------------------------------------------------------
; 1550 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	830
;------------------------------------------------------------------------------
; 1551 | u16Dummy = parameters->wOuterL[d];                       //    percent 
;     | coherence in [0..100]                                                  
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        sti       r0,*+fp(15)
	.line	832
;------------------------------------------------------------------------------
; 1553 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1554 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L262                  ; Branch
                                        ; Branch Occurs to L262
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L263                  ; Branch
                                        ; Branch Occurs to L263
L262:        
	.line	834
;------------------------------------------------------------------------------
; 1555 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	835
;------------------------------------------------------------------------------
; 1556 | a = de;                                                                
; 1558 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L264
                                        ; Branch Occurs to L264
L263:        
	.line	838
;------------------------------------------------------------------------------
; 1559 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L264:        
	.line	840
;------------------------------------------------------------------------------
; 1561 | while ( a < de )                                         //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L265:        
	.line	842
;------------------------------------------------------------------------------
; 1563 | u16tmp = ((UINT16) GetRandNum()) % 100;               //       if rando
;     | m choice >= %coherence,                                                
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      100,r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(16)
	.line	843
;------------------------------------------------------------------------------
; 1564 | if( u16tmp >= u16Dummy )                              //       then ran
;     | domly reposition dot                                                   
;------------------------------------------------------------------------------
        cmpi      *+fp(15),r0
        blo       L267                  ; Branch
                                        ; Branch Occurs to L267
	.line	845
;------------------------------------------------------------------------------
; 1566 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	846
;------------------------------------------------------------------------------
; 1567 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	847
;------------------------------------------------------------------------------
; 1568 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        ldiu      ar4,r0
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
	.line	848
;------------------------------------------------------------------------------
; 1569 | yCoord = rectD + (yCoord % rectH);                                     
; 1571 | else                                                  //       OTHERWIS
;     | E, move coherently (same                                               
; 1572 | {                                                     //       algorith
;     | m as for OPTRECTWIN!):                                                 
; 1573 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement                                            
; 1574 | // is window displacement + pattern displacement!                      
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        ldiu      r5,r0
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L285
                                        ; Branch Occurs to L285
L267:        
	.line	854
;------------------------------------------------------------------------------
; 1575 | xCoord = *a + hw + hv;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(3),ar4
        addi      *ar6,ar4              ; unsigned
        addi      *+fp(1),ar4           ; unsigned
	.line	855
;------------------------------------------------------------------------------
; 1576 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r5
        addi      *ar7,r5               ; unsigned
        addi      *+fp(2),r5            ; unsigned
	.line	856
;------------------------------------------------------------------------------
; 1577 | if( (xCoord > rectR) || (xCoord < rectL) )                             
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L269                  ; Branch
                                        ; Branch Occurs to L269
        cmpi3     ar5,ar4
        bhs       L276                  ; Branch
                                        ; Branch Occurs to L276
L269:        
	.line	858
;------------------------------------------------------------------------------
; 1579 | if( xCoord > rectR ) u16Over = xCoord - rectR;                         
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L271                  ; Branch
                                        ; Branch Occurs to L271
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L272
                                        ; Branch Occurs to L272
L271:        
	.line	859
;------------------------------------------------------------------------------
; 1580 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L272:        
	.line	860
;------------------------------------------------------------------------------
; 1581 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	862
;------------------------------------------------------------------------------
; 1583 | if( hv > 0 )  xCoord = rectL + u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        cmpi      0,r0
        ble       L274                  ; Branch
                                        ; Branch Occurs to L274
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L275
                                        ; Branch Occurs to L275
L274:        
	.line	863
;------------------------------------------------------------------------------
; 1584 | else          xCoord = rectR - u16Over;                                
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L275:        
	.line	865
;------------------------------------------------------------------------------
; 1586 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	866
;------------------------------------------------------------------------------
; 1587 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L285
                                        ; Branch Occurs to L285
L276:        
	.line	868
;------------------------------------------------------------------------------
; 1589 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L278                  ; Branch
                                        ; Branch Occurs to L278
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L285                  ; Branch
                                        ; Branch Occurs to L285
L278:        
	.line	870
;------------------------------------------------------------------------------
; 1591 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L280                  ; Branch
                                        ; Branch Occurs to L280
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L281
                                        ; Branch Occurs to L281
L280:        
	.line	871
;------------------------------------------------------------------------------
; 1592 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L281:        
	.line	872
;------------------------------------------------------------------------------
; 1593 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	874
;------------------------------------------------------------------------------
; 1595 | if( vv > 0 )  yCoord = rectD + u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      0,r0
        ble       L283                  ; Branch
                                        ; Branch Occurs to L283
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L284
                                        ; Branch Occurs to L284
L283:        
	.line	875
;------------------------------------------------------------------------------
; 1596 | else          yCoord = rectU - u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L284:        
	.line	877
;------------------------------------------------------------------------------
; 1598 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	878
;------------------------------------------------------------------------------
; 1599 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L285:        
	.line	882
;------------------------------------------------------------------------------
; 1603 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	883
;------------------------------------------------------------------------------
; 1604 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	885
;------------------------------------------------------------------------------
; 1606 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L286:        
	.line	886
;------------------------------------------------------------------------------
; 1607 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L286                  ; Branch
                                        ; Branch Occurs to L286
	.line	887
;------------------------------------------------------------------------------
; 1608 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	888
;------------------------------------------------------------------------------
; 1609 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	889
;------------------------------------------------------------------------------
; 1610 | a++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	890
;------------------------------------------------------------------------------
; 1611 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	840
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L265                  ; Branch
                                        ; Branch Occurs to L265
        bu        L475
                                        ; Branch Occurs to L475
L289:        
	.line	893
;------------------------------------------------------------------------------
; 1614 | else if( u16Type == DOTLIFEWIN )                            // DOTLIFEW
;     | IN: Similar to OPTRECTWIN, but dots                                    
; 1615 | {                                                           // have a l
;     | imited lifetime...                                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      8,r0
        bne       L323                  ; Branch
                                        ; Branch Occurs to L323
	.line	895
;------------------------------------------------------------------------------
; 1616 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	896
;------------------------------------------------------------------------------
; 1617 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	898
;------------------------------------------------------------------------------
; 1619 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1620 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L292                  ; Branch
                                        ; Branch Occurs to L292
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L293                  ; Branch
                                        ; Branch Occurs to L293
L292:        
	.line	900
;------------------------------------------------------------------------------
; 1621 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	901
;------------------------------------------------------------------------------
; 1622 | a = de;                                                                
; 1624 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L294
                                        ; Branch Occurs to L294
L293:        
	.line	904
;------------------------------------------------------------------------------
; 1625 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L294:        
	.line	906
;------------------------------------------------------------------------------
; 1627 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	907
;------------------------------------------------------------------------------
; 1628 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
        lsh       -8,r0
        sti       r0,*+fp(15)
	.line	908
;------------------------------------------------------------------------------
; 1629 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
	.line	909
;------------------------------------------------------------------------------
; 1630 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(15),r0
        sti       r0,*+fp(5)
	.line	911
;------------------------------------------------------------------------------
; 1632 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(201),r0
        sti       r0,*+fp(18)
	.line	912
;------------------------------------------------------------------------------
; 1633 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L296                  ; Branch
                                        ; Branch Occurs to L296
        stik      1,*+fp(18)
        bu        L298
                                        ; Branch Occurs to L298
L296:        
	.line	913
;------------------------------------------------------------------------------
; 1634 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bls       L298                  ; Branch
                                        ; Branch Occurs to L298
        ldiu      32767,r0
        sti       r0,*+fp(18)
L298:        
	.line	915
;------------------------------------------------------------------------------
; 1636 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
; 1638 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1639 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L299:        
	.line	919
;------------------------------------------------------------------------------
; 1640 | xCoord = *a + hw + hv;                                //       update i
;     | ts position                                                            
;------------------------------------------------------------------------------
        lda       *+fp(3),ar4
        addi      *ar6,ar4              ; unsigned
        addi      *+fp(1),ar4           ; unsigned
	.line	920
;------------------------------------------------------------------------------
; 1641 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r5
        addi      *ar7,r5               ; unsigned
        addi      *+fp(2),r5            ; unsigned
	.line	921
;------------------------------------------------------------------------------
; 1642 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update i
;     | ts current lifetime                                                    
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(5),r0
        subri     *ar0,r0
        lda       *+fp(41),ar0
        sti       r0,*ar0
	.line	923
;------------------------------------------------------------------------------
; 1644 | if( *nextDotLife < 0 )                                //       if dot's
;     |  lifetime has expired, then                                            
; 1645 | {                                                     //       randomly
;     |  repos dot in tgt window...                                            
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *ar0,r0
        cmpi      0,r0
        bge       L301                  ; Branch
                                        ; Branch Occurs to L301
	.line	925
;------------------------------------------------------------------------------
; 1646 | *nextDotLife = nMaxDotLife;                                            
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(18),r0
        sti       r0,*ar0
	.line	926
;------------------------------------------------------------------------------
; 1647 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	927
;------------------------------------------------------------------------------
; 1648 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	928
;------------------------------------------------------------------------------
; 1649 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        ldiu      ar4,r0
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
	.line	929
;------------------------------------------------------------------------------
; 1650 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        ldiu      r5,r0
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L319
                                        ; Branch Occurs to L319
L301:        
	.line	931
;------------------------------------------------------------------------------
; 1652 | else if( (xCoord > rectR) || (xCoord < rectL) )       //       otherwis
;     | e, behaves like OPTRECTWIN...                                          
; 1653 | {                                                     //       (see det
;     | ailed comments above)                                                  
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L303                  ; Branch
                                        ; Branch Occurs to L303
        cmpi3     ar5,ar4
        bhs       L310                  ; Branch
                                        ; Branch Occurs to L310
L303:        
	.line	933
;------------------------------------------------------------------------------
; 1654 | if( xCoord > rectR ) u16Over = xCoord - rectR;                         
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L305                  ; Branch
                                        ; Branch Occurs to L305
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L306
                                        ; Branch Occurs to L306
L305:        
	.line	934
;------------------------------------------------------------------------------
; 1655 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L306:        
	.line	935
;------------------------------------------------------------------------------
; 1656 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	937
;------------------------------------------------------------------------------
; 1658 | if( hv > 0 )  xCoord = rectL + u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        cmpi      0,r0
        ble       L308                  ; Branch
                                        ; Branch Occurs to L308
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L309
                                        ; Branch Occurs to L309
L308:        
	.line	938
;------------------------------------------------------------------------------
; 1659 | else          xCoord = rectR - u16Over;                                
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L309:        
	.line	940
;------------------------------------------------------------------------------
; 1661 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	941
;------------------------------------------------------------------------------
; 1662 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L319
                                        ; Branch Occurs to L319
L310:        
	.line	943
;------------------------------------------------------------------------------
; 1664 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L312                  ; Branch
                                        ; Branch Occurs to L312
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L319                  ; Branch
                                        ; Branch Occurs to L319
L312:        
	.line	945
;------------------------------------------------------------------------------
; 1666 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L314                  ; Branch
                                        ; Branch Occurs to L314
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L315
                                        ; Branch Occurs to L315
L314:        
	.line	946
;------------------------------------------------------------------------------
; 1667 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L315:        
	.line	947
;------------------------------------------------------------------------------
; 1668 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	949
;------------------------------------------------------------------------------
; 1670 | if( vv > 0 )  yCoord = rectD + u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      0,r0
        ble       L317                  ; Branch
                                        ; Branch Occurs to L317
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L318
                                        ; Branch Occurs to L318
L317:        
	.line	950
;------------------------------------------------------------------------------
; 1671 | else          yCoord = rectU - u16Over;                                
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L318:        
	.line	952
;------------------------------------------------------------------------------
; 1673 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	953
;------------------------------------------------------------------------------
; 1674 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L319:        
	.line	955
;------------------------------------------------------------------------------
; 1676 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	956
;------------------------------------------------------------------------------
; 1677 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	958
;------------------------------------------------------------------------------
; 1679 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L320:        
	.line	959
;------------------------------------------------------------------------------
; 1680 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L320                  ; Branch
                                        ; Branch Occurs to L320
	.line	960
;------------------------------------------------------------------------------
; 1681 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	961
;------------------------------------------------------------------------------
; 1682 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	962
;------------------------------------------------------------------------------
; 1683 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	963
;------------------------------------------------------------------------------
; 1684 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	964
;------------------------------------------------------------------------------
; 1685 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	915
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L299                  ; Branch
                                        ; Branch Occurs to L299
        bu        L475
                                        ; Branch Occurs to L475
L323:        
	.line	967
;------------------------------------------------------------------------------
; 1688 | else if( u16Type == DL_NOISEDIR )                           // DL_NOISE
;     | DIR: Similar to DOTLIFEWIN, but dir                                    
; 1689 | {                                                           // of each 
;     | dot is randomly offset from pat dir.                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      11,r0
        bne       L368                  ; Branch
                                        ; Branch Occurs to L368
	.line	969
;------------------------------------------------------------------------------
; 1690 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	970
;------------------------------------------------------------------------------
; 1691 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	972
;------------------------------------------------------------------------------
; 1693 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1694 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L326                  ; Branch
                                        ; Branch Occurs to L326
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L327                  ; Branch
                                        ; Branch Occurs to L327
L326:        
	.line	974
;------------------------------------------------------------------------------
; 1695 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	975
;------------------------------------------------------------------------------
; 1696 | a = de;                                                                
; 1698 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L328
                                        ; Branch Occurs to L328
L327:        
	.line	978
;------------------------------------------------------------------------------
; 1699 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L328:        
	.line	980
;------------------------------------------------------------------------------
; 1701 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	981
;------------------------------------------------------------------------------
; 1702 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
        lsh       -8,r0
        sti       r0,*+fp(15)
	.line	982
;------------------------------------------------------------------------------
; 1703 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
	.line	983
;------------------------------------------------------------------------------
; 1704 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(15),r0
        sti       r0,*+fp(5)
	.line	985
;------------------------------------------------------------------------------
; 1706 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(201),r0
        sti       r0,*+fp(18)
	.line	986
;------------------------------------------------------------------------------
; 1707 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L330                  ; Branch
                                        ; Branch Occurs to L330
        stik      1,*+fp(18)
        bu        L332
                                        ; Branch Occurs to L332
L330:        
	.line	987
;------------------------------------------------------------------------------
; 1708 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bls       L332                  ; Branch
                                        ; Branch Occurs to L332
        ldiu      32767,r0
        sti       r0,*+fp(18)
L332:        
	.line	989
;------------------------------------------------------------------------------
; 1710 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bne       L334                  ; Branch
                                        ; Branch Occurs to L334
        stik      0,*+fp(5)
L334:        
	.line	991
;------------------------------------------------------------------------------
; 1712 | u16tmp = (UINT16) parameters->wOuterL[d];                //    dir nois
;     | e offset range, N, in whole deg                                        
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        sti       r0,*+fp(16)
	.line	992
;------------------------------------------------------------------------------
; 1713 | u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of int
;     | eger choices in [-N:N]                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        ash       1,r0
        addi      1,r0                  ; unsigned
        sti       r0,*+fp(15)
	.line	994
;------------------------------------------------------------------------------
; 1715 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, choose                                           
; 1716 | {                                                        //    new rand
;     | om offset directions for each dot                                      
;------------------------------------------------------------------------------
        lda       *+fp(53),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        bgt       L338                  ; Branch
                                        ; Branch Occurs to L338
	.line	996
;------------------------------------------------------------------------------
; 1717 | j = nVisDotsPerTgt[d];                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(29)
	.line	997
;------------------------------------------------------------------------------
; 1718 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        ldiu      *+fp(30),r0
        cmpi      *+fp(29),r0
        bhs       L337                  ; Branch
                                        ; Branch Occurs to L337
L336:        
	.line	999
;------------------------------------------------------------------------------
; 1720 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose rand
;     | om offset dir in [-N:N]                                                
;------------------------------------------------------------------------------
        call      _GetRandNum2
                                        ; Call Occurs
        ldiu      *+fp(15),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1000
;------------------------------------------------------------------------------
; 1721 | i32val -= (INT32) u16tmp;                          //       NOTE USE OF
;     |  DEDICATED RAND# GENERATOR                                             
;------------------------------------------------------------------------------
        subi      *+fp(16),r4
	.line	1001
;------------------------------------------------------------------------------
; 1722 | i32val *= 10;                                      //       offset dir 
;     | in deg/10                                                              
;------------------------------------------------------------------------------
        ash3      3,r4,r1
        ash3      1,r4,r0
        addi3     r0,r1,r4
	.line	1002
;------------------------------------------------------------------------------
; 1723 | *(nextDotNoise+k) = (INT16) i32val;                //       save new of
;     | fset dir for every dot                                                 
;------------------------------------------------------------------------------
        lda       *+fp(43),ir0
        lda       *+fp(30),ar0
        sti       r4,*+ar0(ir0)
	.line	997
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
        cmpi      *+fp(29),r0
        blo       L336                  ; Branch
                                        ; Branch Occurs to L336
L337:        
	.line	1005
;------------------------------------------------------------------------------
; 1726 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ir1
        lda       265,ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ir1          ; unsigned
        lda       *+fp(53),ir0
        ldiu      *+ar1(ir1),r0
        sti       r0,*+ar0(ir0)
L338:        
	.line	1008
;------------------------------------------------------------------------------
; 1729 | nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decremen
;     | t noise update intv timer                                              
; 1731 | // STRATEGY: hv = Rmm*2^Q, where Q=16 for Rmm < 0.1, or Q=10 otherwise.
;     |  If THETA>=10000, then Q=10. For                                       
; 1732 | // the practical range of display geometries and pattern velocities, we
;     |  can expect that Rmm < 2^5, so                                         
; 1733 | // hv < 2^21 worst-case. Since the trig lookup tables are pre-scaled by
;     |  2^10, we have:                                                        
; 1734 | //   Xmm(scaled) = Rmm*2^Q*2^10*cos(TH) = Rmm*cos(TH)*2^(Q+10) = Xmm*2^
;     | (Q+10)                                                                 
; 1735 | //   Xpix(scaled)= Xmm*(2^16/screenW_mm)*2^(Q-6) = Xpix*2^(Q-6) = Xpix*
;     | 2^(P), P=4 or 10.                                                      
; 1736 | // When P=10, we divide by 2^6 so that we leave in a scale factor of 2^
;     | 4. We then add in the fractional                                       
; 1737 | // pixel displacement from the previous frame update, also scaled by 2^
;     | 4. We then save the fractional                                         
; 1738 | // pixel displacement for the next update and get the integer pixel dis
;     | placement for this frame, Xpix.                                        
; 1739 | // Analogously, for Ypix.                                              
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        lda       *+fp(33),ar1
        ldiu      255,r0
        addi      *+fp(56),ar0          ; unsigned
        addi      *+fp(53),ar1          ; unsigned
        and       *+ar0(4),r0
        ash       1,r0
        subri     *ar1,r0
        sti       r0,*ar1
	.line	1019
;------------------------------------------------------------------------------
; 1740 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
        stik      6,*+fp(9)
	.line	1020
;------------------------------------------------------------------------------
; 1741 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      10000,r0
        blt       L340                  ; Branch
                                        ; Branch Occurs to L340
	.line	1022
;------------------------------------------------------------------------------
; 1743 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
        ldiu      10000,r0
        subri     *+fp(2),r0
        sti       r0,*+fp(2)
	.line	1023
;------------------------------------------------------------------------------
; 1744 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
        stik      0,*+fp(9)
L340:        
	.line	1026
;------------------------------------------------------------------------------
; 1747 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L341:        
	.line	1028
;------------------------------------------------------------------------------
; 1749 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(5),r0
        subri     *ar0,r0
        lda       *+fp(41),ar0
        sti       r0,*ar0
	.line	1029
;------------------------------------------------------------------------------
; 1750 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 1751 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *ar0,r0
        cmpi      0,r0
        bge       L343                  ; Branch
                                        ; Branch Occurs to L343
	.line	1031
;------------------------------------------------------------------------------
; 1752 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(18),r0
        sti       r0,*ar0
	.line	1032
;------------------------------------------------------------------------------
; 1753 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	1033
;------------------------------------------------------------------------------
; 1754 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	1034
;------------------------------------------------------------------------------
; 1755 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        ldiu      ar4,r0
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,r0             ; unsigned
        sti       r0,*ar6
	.line	1035
;------------------------------------------------------------------------------
; 1756 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
        ldiu      r5,r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r0              ; unsigned
        sti       r0,*ar7
L343:        
	.line	1038
;------------------------------------------------------------------------------
; 1759 | i32val = (INT32) *nextDotNoise;                       //       get nois
;     | e offset dir for this dot                                              
;------------------------------------------------------------------------------
        lda       *+fp(43),ar0
        ldiu      *ar0,r4
	.line	1039
;------------------------------------------------------------------------------
; 1760 | i32val += (INT32) vv;                                 //       dot thet
;     | a = offset + pattern theta                                             
;------------------------------------------------------------------------------
        addi      *+fp(2),r4
	.line	1040
;------------------------------------------------------------------------------
; 1761 | if( i32val < 0 ) i32val += 3600;                      //       ensure d
;     | ir lies in [0..3600) deg/10                                            
;------------------------------------------------------------------------------
        cmpi      0,r4
        bge       L345                  ; Branch
                                        ; Branch Occurs to L345
        addi      3600,r4
        bu        L346
                                        ; Branch Occurs to L346
L345:        
	.line	1041
;------------------------------------------------------------------------------
; 1762 | else i32val = i32val % 3600;                                           
;------------------------------------------------------------------------------
        ldiu      r4,r0
        ldiu      3600,r1
        call      MOD_I40
                                        ; Call Occurs
        ldiu      r0,r4
L346:        
	.line	1042
;------------------------------------------------------------------------------
; 1763 | i16Theta = (INT16) i32val;                                             
;------------------------------------------------------------------------------
        sti       r4,*+fp(8)
	.line	1044
;------------------------------------------------------------------------------
; 1765 | i32val = (INT32) hv;                                  //       Rmm*2^Q,
;     |  Q=10 or 16                                                            
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r4
	.line	1045
;------------------------------------------------------------------------------
; 1766 | i32val *= (INT32) cosLUT[i16Theta];                   //       (Rmm*cos
;     | (theta)) * 2^(Q+10)                                                    
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        lda       *+fp(8),ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1046
;------------------------------------------------------------------------------
; 1767 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 1768 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(12),r1
        ldiu      r4,r0
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1048
;------------------------------------------------------------------------------
; 1769 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1049
;------------------------------------------------------------------------------
; 1770 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        addi      *ar0,r4
	.line	1050
;------------------------------------------------------------------------------
; 1771 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1051
;------------------------------------------------------------------------------
; 1772 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1052
;------------------------------------------------------------------------------
; 1773 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 1774 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1775 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi3     r4,r0,ar4             ; unsigned
	.line	1056
;------------------------------------------------------------------------------
; 1777 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1057
;------------------------------------------------------------------------------
; 1778 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 1780 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1060
;------------------------------------------------------------------------------
; 1781 | i32val = (INT32) hv;                                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r4
	.line	1061
;------------------------------------------------------------------------------
; 1782 | i32val *= (INT32) sinLUT[i16Theta];                                    
;------------------------------------------------------------------------------
        lda       *+fp(59),ir0
        lda       *+fp(8),ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1062
;------------------------------------------------------------------------------
; 1783 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
        ldiu      r4,r0
        ldiu      *+fp(13),r1
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1063
;------------------------------------------------------------------------------
; 1784 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1064
;------------------------------------------------------------------------------
; 1785 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        addi      *ar0,r4
	.line	1065
;------------------------------------------------------------------------------
; 1786 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1066
;------------------------------------------------------------------------------
; 1787 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1067
;------------------------------------------------------------------------------
; 1788 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi3     r4,r0,r5              ; unsigned
	.line	1068
;------------------------------------------------------------------------------
; 1789 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1069
;------------------------------------------------------------------------------
; 1790 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1071
;------------------------------------------------------------------------------
; 1792 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 1793 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L348                  ; Branch
                                        ; Branch Occurs to L348
        cmpi3     ar5,ar4
        bhs       L355                  ; Branch
                                        ; Branch Occurs to L355
L348:        
	.line	1073
;------------------------------------------------------------------------------
; 1794 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L350                  ; Branch
                                        ; Branch Occurs to L350
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L351
                                        ; Branch Occurs to L351
L350:        
	.line	1074
;------------------------------------------------------------------------------
; 1795 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L351:        
	.line	1075
;------------------------------------------------------------------------------
; 1796 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1077
;------------------------------------------------------------------------------
; 1798 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar6,ar4,r0           ; unsigned
        cmpi      *+fp(3),r0
        bls       L353                  ; Branch
                                        ; Branch Occurs to L353
	.line	1078
;------------------------------------------------------------------------------
; 1799 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L354
                                        ; Branch Occurs to L354
L353:        
	.line	1079
;------------------------------------------------------------------------------
; 1800 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L354:        
	.line	1081
;------------------------------------------------------------------------------
; 1802 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1082
;------------------------------------------------------------------------------
; 1803 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L364
                                        ; Branch Occurs to L364
L355:        
	.line	1084
;------------------------------------------------------------------------------
; 1805 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L357                  ; Branch
                                        ; Branch Occurs to L357
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L364                  ; Branch
                                        ; Branch Occurs to L364
L357:        
	.line	1086
;------------------------------------------------------------------------------
; 1807 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L359                  ; Branch
                                        ; Branch Occurs to L359
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L360
                                        ; Branch Occurs to L360
L359:        
	.line	1087
;------------------------------------------------------------------------------
; 1808 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L360:        
	.line	1088
;------------------------------------------------------------------------------
; 1809 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1090
;------------------------------------------------------------------------------
; 1811 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar7,r5,r0            ; unsigned
        cmpi      *+fp(4),r0
        bls       L362                  ; Branch
                                        ; Branch Occurs to L362
	.line	1091
;------------------------------------------------------------------------------
; 1812 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L363
                                        ; Branch Occurs to L363
L362:        
	.line	1092
;------------------------------------------------------------------------------
; 1813 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L363:        
	.line	1094
;------------------------------------------------------------------------------
; 1815 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1095
;------------------------------------------------------------------------------
; 1816 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L364:        
	.line	1098
;------------------------------------------------------------------------------
; 1819 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	1099
;------------------------------------------------------------------------------
; 1820 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	1101
;------------------------------------------------------------------------------
; 1822 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L365:        
	.line	1102
;------------------------------------------------------------------------------
; 1823 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L365                  ; Branch
                                        ; Branch Occurs to L365
	.line	1103
;------------------------------------------------------------------------------
; 1824 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	1104
;------------------------------------------------------------------------------
; 1825 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	1105
;------------------------------------------------------------------------------
; 1826 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	1106
;------------------------------------------------------------------------------
; 1827 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	1107
;------------------------------------------------------------------------------
; 1828 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	1108
;------------------------------------------------------------------------------
; 1829 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(43),r0           ; unsigned
        sti       r0,*+fp(43)
	.line	1109
;------------------------------------------------------------------------------
; 1830 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(45),r0           ; unsigned
        sti       r0,*+fp(45)
	.line	1110
;------------------------------------------------------------------------------
; 1831 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(47),r0           ; unsigned
        sti       r0,*+fp(47)
	.line	1026
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L341                  ; Branch
                                        ; Branch Occurs to L341
        bu        L475
                                        ; Branch Occurs to L475
L368:        
	.line	1113
;------------------------------------------------------------------------------
; 1834 | else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISE
;     | SPEED #1: Like DL_NOISEDIR, but                                        
; 1835 | (parameters->wOuterB[d] == 0))                      // Rdot = Rpat + U*
;     | Rpat/100, where U is chosen                                            
; 1836 | {                                                           // randomly
;     |  from [-N..N].                                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      13,r0
        bne       L411                  ; Branch
                                        ; Branch Occurs to L411
        lda       *+fp(33),ir0
        lda       297,ar0
        addi      *+fp(48),ir0          ; unsigned
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        bne       L411                  ; Branch
                                        ; Branch Occurs to L411
	.line	1116
;------------------------------------------------------------------------------
; 1837 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	1117
;------------------------------------------------------------------------------
; 1838 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	1119
;------------------------------------------------------------------------------
; 1840 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1841 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L372                  ; Branch
                                        ; Branch Occurs to L372
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L373                  ; Branch
                                        ; Branch Occurs to L373
L372:        
	.line	1121
;------------------------------------------------------------------------------
; 1842 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	1122
;------------------------------------------------------------------------------
; 1843 | a = de;                                                                
; 1845 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L374
                                        ; Branch Occurs to L374
L373:        
	.line	1125
;------------------------------------------------------------------------------
; 1846 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L374:        
	.line	1127
;------------------------------------------------------------------------------
; 1848 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	1128
;------------------------------------------------------------------------------
; 1849 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
        lsh       -8,r0
        sti       r0,*+fp(15)
	.line	1129
;------------------------------------------------------------------------------
; 1850 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
	.line	1130
;------------------------------------------------------------------------------
; 1851 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(15),r0
        sti       r0,*+fp(5)
	.line	1132
;------------------------------------------------------------------------------
; 1853 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(201),r0
        sti       r0,*+fp(18)
	.line	1133
;------------------------------------------------------------------------------
; 1854 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L376                  ; Branch
                                        ; Branch Occurs to L376
        stik      1,*+fp(18)
        bu        L378
                                        ; Branch Occurs to L378
L376:        
	.line	1134
;------------------------------------------------------------------------------
; 1855 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bls       L378                  ; Branch
                                        ; Branch Occurs to L378
        ldiu      32767,r0
        sti       r0,*+fp(18)
L378:        
	.line	1136
;------------------------------------------------------------------------------
; 1857 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bne       L380                  ; Branch
                                        ; Branch Occurs to L380
        stik      0,*+fp(5)
L380:        
	.line	1138
;------------------------------------------------------------------------------
; 1859 | u16tmp = (UINT16) parameters->wOuterL[d];                //    speed no
;     | ise offset range, N, as %-age of                                       
; 1860 | //    nominal speed, in 1% increments                                  
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        sti       r0,*+fp(16)
	.line	1140
;------------------------------------------------------------------------------
; 1861 | u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of int
;     | eger choices in [-N:N]                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        ash       1,r0
        addi      1,r0                  ; unsigned
        sti       r0,*+fp(15)
	.line	1142
;------------------------------------------------------------------------------
; 1863 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, choose                                           
; 1864 | {                                                        //    new rand
;     | om offset speed %s for each dot                                        
;------------------------------------------------------------------------------
        lda       *+fp(53),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        bgt       L384                  ; Branch
                                        ; Branch Occurs to L384
	.line	1144
;------------------------------------------------------------------------------
; 1865 | j = nVisDotsPerTgt[d];                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(29)
	.line	1145
;------------------------------------------------------------------------------
; 1866 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        ldiu      *+fp(30),r0
        cmpi      *+fp(29),r0
        bhs       L383                  ; Branch
                                        ; Branch Occurs to L383
L382:        
	.line	1147
;------------------------------------------------------------------------------
; 1868 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose rand
;     | om offset speed % in [-N:N]                                            
;------------------------------------------------------------------------------
        call      _GetRandNum2
                                        ; Call Occurs
        ldiu      *+fp(15),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1148
;------------------------------------------------------------------------------
; 1869 | i32val -= (INT32) u16tmp;                          //       NOTE USE OF
;     |  DEDICATED RAND# GENERATOR                                             
;------------------------------------------------------------------------------
        subi      *+fp(16),r4
	.line	1149
;------------------------------------------------------------------------------
; 1870 | *(nextDotNoise+k) = (INT16) i32val;                                    
;------------------------------------------------------------------------------
        lda       *+fp(43),ir0
        lda       *+fp(30),ar0
        sti       r4,*+ar0(ir0)
	.line	1145
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
        cmpi      *+fp(29),r0
        blo       L382                  ; Branch
                                        ; Branch Occurs to L382
L383:        
	.line	1152
;------------------------------------------------------------------------------
; 1873 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ir1
        lda       265,ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ir1          ; unsigned
        lda       *+fp(53),ir0
        ldiu      *+ar1(ir1),r0
        sti       r0,*+ar0(ir0)
L384:        
	.line	1155
;------------------------------------------------------------------------------
; 1876 | nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decremen
;     | t noise update intv timer                                              
; 1878 | // STRATEGY: hv = Rmm*2^Q, where Q=16 for Rmm < 0.1, or Q=10 otherwise.
;     |  If THETA>=10000, then Q=10. For                                       
; 1879 | // the practical range of display geometries and pattern velocities, we
;     |  can expect that Rmm < 2^5, so                                         
; 1880 | // hv < 2^21 worst-case. Since the trig lookup tables are pre-scaled by
;     |  2^10, we have:                                                        
; 1881 | //   Rmm*2^Q + N*Rmm*2^Q/100 = (Rmm + N*Rmm/100)*2^Q = Rmm(dot) * 2^Q  
; 1882 | //   Xmm(scaled) = Rmm(dot)*2^Q*2^10*cos(TH) = Rmm(dot)*cos(TH)*2^(Q+10
;     | ) = Xmm*2^(Q+10)                                                       
; 1883 | //   Xpix(scaled)= Xmm*(2^16/screenW_mm)*2^(Q-6) = Xpix*2^(Q-6) = Xpix*
;     | 2^(P), P=4 or 10.                                                      
; 1884 | // When P=10, we divide by 2^6 so that we leave in a scale factor of 2^
;     | 4. We then add in the fractional                                       
; 1885 | // pixel displacement from the previous frame update, also scaled by 2^
;     | 4. We then save the fractional                                         
; 1886 | // pixel displacement for the next update and get the integer pixel dis
;     | placement for this frame, Xpix.                                        
; 1887 | // Analogously, for Ypix.                                              
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        lda       *+fp(33),ar1
        ldiu      255,r0
        addi      *+fp(56),ar0          ; unsigned
        addi      *+fp(53),ar1          ; unsigned
        and       *+ar0(4),r0
        ash       1,r0
        subri     *ar1,r0
        sti       r0,*ar1
	.line	1167
;------------------------------------------------------------------------------
; 1888 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
        stik      6,*+fp(9)
	.line	1168
;------------------------------------------------------------------------------
; 1889 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      10000,r0
        blt       L386                  ; Branch
                                        ; Branch Occurs to L386
	.line	1170
;------------------------------------------------------------------------------
; 1891 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
        ldiu      10000,r0
        subri     *+fp(2),r0
        sti       r0,*+fp(2)
	.line	1171
;------------------------------------------------------------------------------
; 1892 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
        stik      0,*+fp(9)
L386:        
	.line	1174
;------------------------------------------------------------------------------
; 1895 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L387:        
	.line	1176
;------------------------------------------------------------------------------
; 1897 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(5),r0
        subri     *ar0,r0
        lda       *+fp(41),ar0
        sti       r0,*ar0
	.line	1177
;------------------------------------------------------------------------------
; 1898 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 1899 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *ar0,r0
        cmpi      0,r0
        bge       L389                  ; Branch
                                        ; Branch Occurs to L389
	.line	1179
;------------------------------------------------------------------------------
; 1900 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(18),r0
        sti       r0,*ar0
	.line	1180
;------------------------------------------------------------------------------
; 1901 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	1181
;------------------------------------------------------------------------------
; 1902 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	1182
;------------------------------------------------------------------------------
; 1903 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        ldiu      ar4,r0
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,r0             ; unsigned
        sti       r0,*ar6
	.line	1183
;------------------------------------------------------------------------------
; 1904 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
        ldiu      r5,r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r0              ; unsigned
        sti       r0,*ar7
L389:        
	.line	1186
;------------------------------------------------------------------------------
; 1907 | i32val = (INT32) *nextDotNoise;                       //       get offs
;     | et speed %age N for this dot                                           
;------------------------------------------------------------------------------
        lda       *+fp(43),ar0
        ldiu      *ar0,r4
	.line	1187
;------------------------------------------------------------------------------
; 1908 | i32val *= (INT32) hv;                                 //       compute 
;     | dot R=2^Q*(patR + N*patR/100).                                         
;------------------------------------------------------------------------------
        mpyi      *+fp(1),r4            ; 32 Bit Multiply (C40)
	.line	1188
;------------------------------------------------------------------------------
; 1909 | i32val /= 100;                                                         
;------------------------------------------------------------------------------
        ldiu      100,r1
        ldiu      r4,r0
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1189
;------------------------------------------------------------------------------
; 1910 | i32val += (INT32) hv;                                                  
;------------------------------------------------------------------------------
        addi      *+fp(1),r4
	.line	1190
;------------------------------------------------------------------------------
; 1911 | x32 = i32val;                                         //       save cuz
;     |  we're going to change i32val                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(6)
	.line	1192
;------------------------------------------------------------------------------
; 1913 | i32val *= (INT32) cosLUT[vv];                         //       (Rmm*cos
;     | (theta)) * 2^(Q+10)                                                    
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        lda       *+fp(2),ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1193
;------------------------------------------------------------------------------
; 1914 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 1915 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
        ldiu      r4,r0
        ldiu      *+fp(12),r1
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1195
;------------------------------------------------------------------------------
; 1916 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1196
;------------------------------------------------------------------------------
; 1917 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        addi      *ar0,r4
	.line	1197
;------------------------------------------------------------------------------
; 1918 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1198
;------------------------------------------------------------------------------
; 1919 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1199
;------------------------------------------------------------------------------
; 1920 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 1921 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1922 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi3     r4,r0,ar4             ; unsigned
	.line	1203
;------------------------------------------------------------------------------
; 1924 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1204
;------------------------------------------------------------------------------
; 1925 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 1927 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1207
;------------------------------------------------------------------------------
; 1928 | i32val = x32;                                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(6),r4
	.line	1208
;------------------------------------------------------------------------------
; 1929 | i32val *= (INT32) sinLUT[vv];                                          
;------------------------------------------------------------------------------
        lda       *+fp(59),ir0
        lda       *+fp(2),ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1209
;------------------------------------------------------------------------------
; 1930 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
        ldiu      *+fp(13),r1
        ldiu      r4,r0
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1210
;------------------------------------------------------------------------------
; 1931 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1211
;------------------------------------------------------------------------------
; 1932 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        addi      *ar0,r4
	.line	1212
;------------------------------------------------------------------------------
; 1933 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1213
;------------------------------------------------------------------------------
; 1934 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1214
;------------------------------------------------------------------------------
; 1935 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi3     r4,r0,r5              ; unsigned
	.line	1215
;------------------------------------------------------------------------------
; 1936 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1216
;------------------------------------------------------------------------------
; 1937 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1218
;------------------------------------------------------------------------------
; 1939 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 1940 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L391                  ; Branch
                                        ; Branch Occurs to L391
        cmpi3     ar5,ar4
        bhs       L398                  ; Branch
                                        ; Branch Occurs to L398
L391:        
	.line	1220
;------------------------------------------------------------------------------
; 1941 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L393                  ; Branch
                                        ; Branch Occurs to L393
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L394
                                        ; Branch Occurs to L394
L393:        
	.line	1221
;------------------------------------------------------------------------------
; 1942 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L394:        
	.line	1222
;------------------------------------------------------------------------------
; 1943 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1224
;------------------------------------------------------------------------------
; 1945 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar6,ar4,r0           ; unsigned
        cmpi      *+fp(3),r0
        bls       L396                  ; Branch
                                        ; Branch Occurs to L396
	.line	1225
;------------------------------------------------------------------------------
; 1946 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L397
                                        ; Branch Occurs to L397
L396:        
	.line	1226
;------------------------------------------------------------------------------
; 1947 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L397:        
	.line	1228
;------------------------------------------------------------------------------
; 1949 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1229
;------------------------------------------------------------------------------
; 1950 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L407
                                        ; Branch Occurs to L407
L398:        
	.line	1231
;------------------------------------------------------------------------------
; 1952 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L400                  ; Branch
                                        ; Branch Occurs to L400
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L407                  ; Branch
                                        ; Branch Occurs to L407
L400:        
	.line	1233
;------------------------------------------------------------------------------
; 1954 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L402                  ; Branch
                                        ; Branch Occurs to L402
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L403
                                        ; Branch Occurs to L403
L402:        
	.line	1234
;------------------------------------------------------------------------------
; 1955 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L403:        
	.line	1235
;------------------------------------------------------------------------------
; 1956 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1237
;------------------------------------------------------------------------------
; 1958 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar7,r5,r0            ; unsigned
        cmpi      *+fp(4),r0
        bls       L405                  ; Branch
                                        ; Branch Occurs to L405
	.line	1238
;------------------------------------------------------------------------------
; 1959 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L406
                                        ; Branch Occurs to L406
L405:        
	.line	1239
;------------------------------------------------------------------------------
; 1960 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L406:        
	.line	1241
;------------------------------------------------------------------------------
; 1962 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1242
;------------------------------------------------------------------------------
; 1963 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L407:        
	.line	1245
;------------------------------------------------------------------------------
; 1966 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	1246
;------------------------------------------------------------------------------
; 1967 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	1248
;------------------------------------------------------------------------------
; 1969 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L408:        
	.line	1249
;------------------------------------------------------------------------------
; 1970 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L408                  ; Branch
                                        ; Branch Occurs to L408
	.line	1250
;------------------------------------------------------------------------------
; 1971 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	1251
;------------------------------------------------------------------------------
; 1972 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	1252
;------------------------------------------------------------------------------
; 1973 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	1253
;------------------------------------------------------------------------------
; 1974 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	1254
;------------------------------------------------------------------------------
; 1975 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	1255
;------------------------------------------------------------------------------
; 1976 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(43),r0           ; unsigned
        sti       r0,*+fp(43)
	.line	1256
;------------------------------------------------------------------------------
; 1977 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(45),r0           ; unsigned
        sti       r0,*+fp(45)
	.line	1257
;------------------------------------------------------------------------------
; 1978 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(47),r0           ; unsigned
        sti       r0,*+fp(47)
	.line	1174
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L387                  ; Branch
                                        ; Branch Occurs to L387
        bu        L475
                                        ; Branch Occurs to L475
L411:        
	.line	1260
;------------------------------------------------------------------------------
; 1981 | else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISE
;     | SPEED #2: Like DL_NOISESPEED #1, but                                   
; 1982 | (parameters->wOuterB[d] != 0))                      // Rdot = (Rpat*2^U
;     | )/E(2^U), where U is chosen                                            
; 1983 | {                                                           // randomly
;     |  from [-N..N]...                                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      13,r0
        bne       L454                  ; Branch
                                        ; Branch Occurs to L454
        lda       *+fp(33),ir0
        lda       297,ar0
        addi      *+fp(48),ir0          ; unsigned
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        beq       L454                  ; Branch
                                        ; Branch Occurs to L454
	.line	1263
;------------------------------------------------------------------------------
; 1984 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
        lda       *+fp(49),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(10)
	.line	1264
;------------------------------------------------------------------------------
; 1985 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
        lda       *+fp(50),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(11)
	.line	1266
;------------------------------------------------------------------------------
; 1987 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1988 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
        cmpi3     ar5,r8
        bls       L415                  ; Branch
                                        ; Branch Occurs to L415
        ldiu      *+fp(64),r0
        ldiu      *+fp(63),r1
        cmpi3     r0,r1
        bhi       L416                  ; Branch
                                        ; Branch Occurs to L416
L415:        
	.line	1268
;------------------------------------------------------------------------------
; 1989 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        stik      0,*+ar0(ir0)
	.line	1269
;------------------------------------------------------------------------------
; 1990 | a = de;                                                                
; 1992 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
        lda       *+fp(66),ar6
        bu        L417
                                        ; Branch Occurs to L417
L416:        
	.line	1272
;------------------------------------------------------------------------------
; 1993 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
L417:        
	.line	1274
;------------------------------------------------------------------------------
; 1995 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	1275
;------------------------------------------------------------------------------
; 1996 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
        lsh       -8,r0
        sti       r0,*+fp(15)
	.line	1276
;------------------------------------------------------------------------------
; 1997 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
	.line	1277
;------------------------------------------------------------------------------
; 1998 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
        ldiu      *+fp(15),r0
        sti       r0,*+fp(5)
	.line	1279
;------------------------------------------------------------------------------
; 2000 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(201),r0
        sti       r0,*+fp(18)
	.line	1280
;------------------------------------------------------------------------------
; 2001 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L419                  ; Branch
                                        ; Branch Occurs to L419
        stik      1,*+fp(18)
        bu        L421
                                        ; Branch Occurs to L421
L419:        
	.line	1281
;------------------------------------------------------------------------------
; 2002 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bls       L421                  ; Branch
                                        ; Branch Occurs to L421
        ldiu      32767,r0
        sti       r0,*+fp(18)
L421:        
	.line	1283
;------------------------------------------------------------------------------
; 2004 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(18),r0
        cmpi      32767,r0
        bne       L423                  ; Branch
                                        ; Branch Occurs to L423
        stik      0,*+fp(5)
L423:        
	.line	1285
;------------------------------------------------------------------------------
; 2006 | m = (UINT32) parameters->wOuterL[d];                     //    N = max 
;     | speed noise exp, in [1..7]                                             
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(48),ar0          ; unsigned
        ldiu      *+ar0(233),r0
        sti       r0,*+fp(32)
	.line	1286
;------------------------------------------------------------------------------
; 2007 | u16tmp = (UINT16) (m * 20);                              //    20N     
;------------------------------------------------------------------------------
        ash3      2,r0,r1
        ash3      4,r0,r0
        addi3     r1,r0,r0              ; unsigned
        sti       r0,*+fp(16)
	.line	1287
;------------------------------------------------------------------------------
; 2008 | u16Dummy = (UINT16) u16tmp * 2 + 1;                      //    # choice
;     | s in [-20N:20N]                                                        
;------------------------------------------------------------------------------
        ash       1,r0
        addi      1,r0                  ; unsigned
        sti       r0,*+fp(15)
	.line	1289
;------------------------------------------------------------------------------
; 2010 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, get new                                          
; 2011 | {                                                        //    new rand
;     | om index into the pow2LUT array                                        
;------------------------------------------------------------------------------
        lda       *+fp(53),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        bgt       L427                  ; Branch
                                        ; Branch Occurs to L427
	.line	1291
;------------------------------------------------------------------------------
; 2012 | j = nVisDotsPerTgt[d];                                //    for each do
;     | t.                                                                     
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(29)
	.line	1292
;------------------------------------------------------------------------------
; 2013 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
        stik      0,*+fp(30)
        ldiu      *+fp(30),r0
        cmpi      *+fp(29),r0
        bhs       L426                  ; Branch
                                        ; Branch Occurs to L426
L425:        
	.line	1294
;------------------------------------------------------------------------------
; 2015 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //    [0..40N]      
;------------------------------------------------------------------------------
        call      _GetRandNum2
                                        ; Call Occurs
        ldiu      *+fp(15),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1295
;------------------------------------------------------------------------------
; 2016 | i32val += (INT32) (140 - u16tmp);                  //    140 + [-20N..2
;     | 0N]                                                                    
;------------------------------------------------------------------------------
        subi      *+fp(16),r4           ; unsigned
        addi      140,r4
	.line	1296
;------------------------------------------------------------------------------
; 2017 | *(nextDotNoise+k) = (INT16) i32val;                                    
;------------------------------------------------------------------------------
        lda       *+fp(43),ir0
        lda       *+fp(30),ar0
        sti       r4,*+ar0(ir0)
	.line	1292
        ldiu      1,r0
        addi      *+fp(30),r0           ; unsigned
        sti       r0,*+fp(30)
        cmpi      *+fp(29),r0
        blo       L425                  ; Branch
                                        ; Branch Occurs to L425
L426:        
	.line	1299
;------------------------------------------------------------------------------
; 2020 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ir1
        lda       265,ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ir1          ; unsigned
        lda       *+fp(53),ir0
        ldiu      *+ar1(ir1),r0
        sti       r0,*+ar0(ir0)
L427:        
	.line	1302
;------------------------------------------------------------------------------
; 2023 | nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decremen
;     | t noise update intv timer                                              
; 2025 | // STRATEGY: Analogous to the additive speed noise case above, except t
;     | hat we implement the                                                   
; 2026 | // multiplicative speed noise algorithm here.                          
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        lda       *+fp(33),ar1
        ldiu      255,r0
        addi      *+fp(56),ar0          ; unsigned
        addi      *+fp(53),ar1          ; unsigned
        and       *+ar0(4),r0
        ash       1,r0
        subri     *ar1,r0
        sti       r0,*ar1
	.line	1306
;------------------------------------------------------------------------------
; 2027 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
        stik      6,*+fp(9)
	.line	1307
;------------------------------------------------------------------------------
; 2028 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        cmpi      10000,r0
        blt       L429                  ; Branch
                                        ; Branch Occurs to L429
	.line	1309
;------------------------------------------------------------------------------
; 2030 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
        ldiu      10000,r0
        subri     *+fp(2),r0
        sti       r0,*+fp(2)
	.line	1310
;------------------------------------------------------------------------------
; 2031 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
        stik      0,*+fp(9)
L429:        
	.line	1313
;------------------------------------------------------------------------------
; 2034 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L430:        
	.line	1315
;------------------------------------------------------------------------------
; 2036 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(5),r0
        subri     *ar0,r0
        lda       *+fp(41),ar0
        sti       r0,*ar0
	.line	1316
;------------------------------------------------------------------------------
; 2037 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 2038 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *ar0,r0
        cmpi      0,r0
        bge       L432                  ; Branch
                                        ; Branch Occurs to L432
	.line	1318
;------------------------------------------------------------------------------
; 2039 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        ldiu      *+fp(18),r0
        sti       r0,*ar0
	.line	1319
;------------------------------------------------------------------------------
; 2040 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        lda       r0,ar4
	.line	1320
;------------------------------------------------------------------------------
; 2041 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      r0,r5
	.line	1321
;------------------------------------------------------------------------------
; 2042 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        ldiu      ar4,r0
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,r0             ; unsigned
        sti       r0,*ar6
	.line	1322
;------------------------------------------------------------------------------
; 2043 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
        ldiu      r5,r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r0              ; unsigned
        sti       r0,*ar7
L432:        
	.line	1325
;------------------------------------------------------------------------------
; 2046 | i32val = pow2LUT[ *nextDotNoise ];                    //       R = 2^(x
;     | +20), x in [-N..N], N=[1..7]                                           
;------------------------------------------------------------------------------
        lda       *+fp(43),ar0
        lda       *ar0,ar0
        lda       *+fp(61),ir0
        ldiu      *+ar0(ir0),r4
	.line	1326
;------------------------------------------------------------------------------
; 2047 | i32val /= speedNoiseAdj[m-1];                         //       R = 2^20
;     |  * 2^x / (E(2^x) * 2^10)                                               
;------------------------------------------------------------------------------
        lda       *+fp(32),ar0
        addi      *+fp(62),ar0          ; unsigned
        ldiu      r4,r0
        ldiu      *-ar0(1),r1
        call      DIV_I40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1327
;------------------------------------------------------------------------------
; 2048 | i32val *= hv;                                         //       R = Rpat
;     | _mm*2^Q * 2^10 * 2^x / E(2^x)                                          
;------------------------------------------------------------------------------
        mpyi      *+fp(1),r4            ; 32 Bit Multiply (C40)
	.line	1328
;------------------------------------------------------------------------------
; 2049 | i32val >>= 10;                                        //       R = Rdot
;     | _mm*2^Q, Q=10 or 16                                                    
;------------------------------------------------------------------------------
        ash       -10,r4                ;asr
	.line	1329
;------------------------------------------------------------------------------
; 2050 | x32 = i32val;                                         //       save cuz
;     |  we're going to change i32val                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(6)
	.line	1331
;------------------------------------------------------------------------------
; 2052 | i32val *= (INT32) cosLUT[vv];                         //       Rdot_mm*
;     | cos(theta) * 2^(Q+10)                                                  
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        lda       *+fp(2),ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1332
;------------------------------------------------------------------------------
; 2053 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 2054 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(12),r1
        ldiu      r4,r0
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1334
;------------------------------------------------------------------------------
; 2055 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1335
;------------------------------------------------------------------------------
; 2056 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        addi      *ar0,r4
	.line	1336
;------------------------------------------------------------------------------
; 2057 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1337
;------------------------------------------------------------------------------
; 2058 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1338
;------------------------------------------------------------------------------
; 2059 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 2060 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 2061 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------
        ldiu      *+fp(3),r0
        addi      *ar6,r0               ; unsigned
        addi3     r4,r0,ar4             ; unsigned
	.line	1342
;------------------------------------------------------------------------------
; 2063 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1343
;------------------------------------------------------------------------------
; 2064 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 2066 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
        lda       *+fp(45),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1346
;------------------------------------------------------------------------------
; 2067 | i32val = x32;                                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(6),r4
	.line	1347
;------------------------------------------------------------------------------
; 2068 | i32val *= (INT32) sinLUT[vv];                                          
;------------------------------------------------------------------------------
        lda       *+fp(2),ar0
        lda       *+fp(59),ir0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1348
;------------------------------------------------------------------------------
; 2069 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
        ldiu      *+fp(13),r1
        ldiu      r4,r0
        call      DIV_U40
                                        ; Call Occurs
        ldiu      r0,r4
	.line	1349
;------------------------------------------------------------------------------
; 2070 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
        negi      *+fp(9),r0
        ash3      r0,r4,r4              ;asr3
	.line	1350
;------------------------------------------------------------------------------
; 2071 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        addi      *ar0,r4
	.line	1351
;------------------------------------------------------------------------------
; 2072 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
        sti       r4,*+fp(7)
	.line	1352
;------------------------------------------------------------------------------
; 2073 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1353
;------------------------------------------------------------------------------
; 2074 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------
        ldiu      *+fp(4),r0
        addi      *ar7,r0               ; unsigned
        addi3     r4,r0,r5              ; unsigned
	.line	1354
;------------------------------------------------------------------------------
; 2075 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
        ash       4,r4
	.line	1355
;------------------------------------------------------------------------------
; 2076 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
        lda       *+fp(47),ar0
        subri     *+fp(7),r4
        sti       r4,*ar0
	.line	1357
;------------------------------------------------------------------------------
; 2078 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 2079 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bhi       L434                  ; Branch
                                        ; Branch Occurs to L434
        cmpi3     ar5,ar4
        bhs       L441                  ; Branch
                                        ; Branch Occurs to L441
L434:        
	.line	1359
;------------------------------------------------------------------------------
; 2080 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
        cmpi3     r8,ar4
        bls       L436                  ; Branch
                                        ; Branch Occurs to L436
        subi3     r8,ar4,r0             ; unsigned
        sti       r0,*+fp(17)
        bu        L437
                                        ; Branch Occurs to L437
L436:        
	.line	1360
;------------------------------------------------------------------------------
; 2081 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
        subi3     ar4,ar5,r0            ; unsigned
        sti       r0,*+fp(17)
L437:        
	.line	1361
;------------------------------------------------------------------------------
; 2082 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1363
;------------------------------------------------------------------------------
; 2084 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar6,ar4,r0           ; unsigned
        cmpi      *+fp(3),r0
        bls       L439                  ; Branch
                                        ; Branch Occurs to L439
	.line	1364
;------------------------------------------------------------------------------
; 2085 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        addi      ar5,ar4
        bu        L440
                                        ; Branch Occurs to L440
L439:        
	.line	1365
;------------------------------------------------------------------------------
; 2086 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
        lda       *+fp(17),ar4
        subri     r8,ar4
L440:        
	.line	1367
;------------------------------------------------------------------------------
; 2088 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1368
;------------------------------------------------------------------------------
; 2089 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        ldiu      *+fp(64),r1
        addi3     r0,r1,r5              ; unsigned
        bu        L450
                                        ; Branch Occurs to L450
L441:        
	.line	1370
;------------------------------------------------------------------------------
; 2091 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bhi       L443                  ; Branch
                                        ; Branch Occurs to L443
        ldiu      *+fp(64),r0
        cmpi3     r0,r5
        bhs       L450                  ; Branch
                                        ; Branch Occurs to L450
L443:        
	.line	1372
;------------------------------------------------------------------------------
; 2093 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r0
        cmpi3     r0,r5
        bls       L445                  ; Branch
                                        ; Branch Occurs to L445
        subi3     r0,r5,r0              ; unsigned
        sti       r0,*+fp(17)
        bu        L446
                                        ; Branch Occurs to L446
L445:        
	.line	1373
;------------------------------------------------------------------------------
; 2094 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r0
        subi3     r5,r0,r0              ; unsigned
        sti       r0,*+fp(17)
L446:        
	.line	1374
;------------------------------------------------------------------------------
; 2095 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(17),r0
        ldiu      *+fp(11),r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(17)
	.line	1376
;------------------------------------------------------------------------------
; 2097 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
        subi3     *ar7,r5,r0            ; unsigned
        cmpi      *+fp(4),r0
        bls       L448                  ; Branch
                                        ; Branch Occurs to L448
	.line	1377
;------------------------------------------------------------------------------
; 2098 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r5
        addi      *+fp(17),r5           ; unsigned
        bu        L449
                                        ; Branch Occurs to L449
L448:        
	.line	1378
;------------------------------------------------------------------------------
; 2099 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r5
        subi      *+fp(17),r5           ; unsigned
L449:        
	.line	1380
;------------------------------------------------------------------------------
; 2101 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
	.line	1381
;------------------------------------------------------------------------------
; 2102 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
        ldiu      *+fp(10),r1
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,ar5,ar4            ; unsigned
L450:        
	.line	1384
;------------------------------------------------------------------------------
; 2105 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
        sti       ar4,*ar6
	.line	1385
;------------------------------------------------------------------------------
; 2106 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
        sti       r5,*ar7
	.line	1387
;------------------------------------------------------------------------------
; 2108 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
        ldiu      *ar6,r0
        ash       16,r0
        or        *ar7,r0
        sti       r0,*+fp(21)
L451:        
	.line	1388
;------------------------------------------------------------------------------
; 2109 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L451                  ; Branch
                                        ; Branch Occurs to L451
	.line	1389
;------------------------------------------------------------------------------
; 2110 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	1390
;------------------------------------------------------------------------------
; 2111 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	1391
;------------------------------------------------------------------------------
; 2112 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	1392
;------------------------------------------------------------------------------
; 2113 | b++;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	1393
;------------------------------------------------------------------------------
; 2114 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	1394
;------------------------------------------------------------------------------
; 2115 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(43),r0           ; unsigned
        sti       r0,*+fp(43)
	.line	1395
;------------------------------------------------------------------------------
; 2116 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(45),r0           ; unsigned
        sti       r0,*+fp(45)
	.line	1396
;------------------------------------------------------------------------------
; 2117 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(47),r0           ; unsigned
        sti       r0,*+fp(47)
	.line	1313
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L430                  ; Branch
                                        ; Branch Occurs to L430
        bu        L475
                                        ; Branch Occurs to L475
L454:        
	.line	1399
;------------------------------------------------------------------------------
; 2120 | else if( u16Type == OPTICFLOW )                             // OPTICFLO
;     | W:  Dot speed varies w/ radial pos,                                    
; 2121 | {                                                           // calc'd e
;     | very frame.  See file header for                                       
; 2122 | // explanation of implementation...                                    
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      9,r0
        bne       L475                  ; Branch
                                        ; Branch Occurs to L475
	.line	1402
;------------------------------------------------------------------------------
; 2123 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    every do
;     | t in target is visible                                                 
;------------------------------------------------------------------------------
        lda       *+fp(33),ar1
        lda       *+fp(33),ar0
        addi      *+fp(48),ar1          ; unsigned
        lda       *+fp(52),ir0
        ldiu      *+ar1(41),r0
        sti       r0,*+ar0(ir0)
	.line	1404
;------------------------------------------------------------------------------
; 2125 | if( hv < 0 )                                             //    FOR DECE
;     | LERATING FLOWS:                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(1),r0
        cmpi      0,r0
        bge       L469                  ; Branch
                                        ; Branch Occurs to L469
	.line	1406
;------------------------------------------------------------------------------
; 2127 | rectH = (UINT16) (-hv);                               //    this factor
;     |  in the recycle rate incr with                                         
;------------------------------------------------------------------------------
        negi      *+fp(1),r0
        sti       r0,*+fp(11)
	.line	1407
;------------------------------------------------------------------------------
; 2128 | rectH >>= vv;                                         //    B ~ deltaT 
;     | * flow vel / 3.  the scaling                                           
;------------------------------------------------------------------------------
        negi      *+fp(2),r1
        lsh       r1,r0
        sti       r0,*+fp(11)
	.line	1408
;------------------------------------------------------------------------------
; 2129 | rectH /= 3;                                           //    by 1/3 was 
;     | determined heuristically.                                              
;------------------------------------------------------------------------------
        ldiu      3,r1
        call      DIV_U40
                                        ; Call Occurs
        sti       r0,*+fp(11)
	.line	1409
;------------------------------------------------------------------------------
; 2130 | if( rectH < 1 ) rectH = 1;                            //    rate limite
;     | d to [1..400] parts per 1000.                                          
;------------------------------------------------------------------------------
        cmpi      0,r0
        bne       L458                  ; Branch
                                        ; Branch Occurs to L458
        stik      1,*+fp(11)
L458:        
	.line	1410
;------------------------------------------------------------------------------
; 2131 | if( rectH > 400 ) rectH = 400;                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(11),r0
        cmpi      400,r0
        bls       L460                  ; Branch
                                        ; Branch Occurs to L460
        ldiu      400,r0
        sti       r0,*+fp(11)
L460:        
	.line	1412
;------------------------------------------------------------------------------
; 2133 | i32val = hv * sincosLUT[ rectL ];                     //    change in r
;     | adial pos at outer edge; repos                                         
;------------------------------------------------------------------------------
        lda       *+fp(58),ir0
        ldiu      *+fp(1),r0
        mpyi3     r0,*+ar5(ir0),r4      ; 32 Bit Multiply (C40)
	.line	1413
;------------------------------------------------------------------------------
; 2134 | i32val >>= (10 + vv);                                 //    dots in the
;     |  band between outer radius and                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        subri     -10,r0
        ash3      r0,r4,r4              ;asr3
	.line	1414
;------------------------------------------------------------------------------
; 2135 | i32val += (INT32) rectL;                              //    r = rOuter 
;     | - radial chng at outer edge...                                         
;------------------------------------------------------------------------------
        addi3     ar5,r4,r4
	.line	1415
;------------------------------------------------------------------------------
; 2136 | u16tmp = (UINT16) i32val;                                              
;------------------------------------------------------------------------------
        sti       r4,*+fp(16)
	.line	1417
;------------------------------------------------------------------------------
; 2138 | while( a < de )                                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L461:        
	.line	1419
;------------------------------------------------------------------------------
; 2140 | i32val = hv * sincosLUT[ *a ];                     //    dr*2^(10+M)= [
;     | B*2^M]*[sin(r)*cos(r)*2^10]                                            
;------------------------------------------------------------------------------
        lda       *+fp(58),ir0
        lda       *ar6,ar0
        ldiu      *+fp(1),r0
        mpyi3     r0,*+ar0(ir0),r4      ; 32 Bit Multiply (C40)
	.line	1420
;------------------------------------------------------------------------------
; 2141 | i32val >>= (2 + vv);                               //    dr*2^(10+M) --
;     | > dr*2^8                                                               
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        subri     -2,r0
        ash3      r0,r4,r4              ;asr3
	.line	1421
;------------------------------------------------------------------------------
; 2142 | i32val += (INT32) *nextDotLife;                    //    accum fraction
;     | al pos chng (deg/100/2^8)                                              
; 2143 | //    from last update -- NOTE usage of the                            
; 2144 | //    "dotlife" array for this purpose!                                
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        addi      *ar0,r4
	.line	1425
;------------------------------------------------------------------------------
; 2146 | *nextDotLife = 0xFF00 | (0x00FF & i32val);         //    carry over fra
;     | c pos chng for next update                                             
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       r4,r0
        lda       *+fp(41),ar0
        or        65280,r0
        sti       r0,*ar0
	.line	1426
;------------------------------------------------------------------------------
; 2147 | i32val >>= 8;                                      //    dr*2^8 --> dr 
;------------------------------------------------------------------------------
        ash       -8,r4                 ;asr
	.line	1427
;------------------------------------------------------------------------------
; 2148 | ++i32val;                                          //    -1 maps to 0 f
;     | or neg flows                                                           
;------------------------------------------------------------------------------
        addi      1,r4
	.line	1428
;------------------------------------------------------------------------------
; 2149 | i32val += (INT32) *a;                              //    r' = r + dr   
;------------------------------------------------------------------------------
        addi      *ar6,r4
	.line	1429
;------------------------------------------------------------------------------
; 2150 | *a = (UINT16) i32val;                                                  
;------------------------------------------------------------------------------
        sti       r4,*ar6
	.line	1431
;------------------------------------------------------------------------------
; 2152 | u16Dummy = ((UINT16) GetRandNum()) % 1000;         //    algorithm for 
;     | choosing dots to recycle:                                              
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      1000,r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*+fp(15)
	.line	1432
;------------------------------------------------------------------------------
; 2153 | rectW = rectR +                                    //       1) dot has 
;     | entered hole at FOE, or                                                
; 2154 | (((UINT16) GetRandNum()) % (rectL-rectR));      //       2) is randomly
;     |  selected for recycle.                                                 
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        subi3     r8,ar5,r1             ; unsigned
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,r8,r0              ; unsigned
        sti       r0,*+fp(10)
	.line	1434
;------------------------------------------------------------------------------
; 2155 | if( (i32val < (INT32) rectR) ||                    //    if chosen for 
;     | recycle, randomly choose                                               
; 2156 | ((u16Dummy<rectH) && (i32val<(INT32)rectW))    //    polar coords (r,th
;     | eta) so that dot is                                                    
; 2157 | )                                               //    repos in band nea
;     | r outer edge of field                                                  
;------------------------------------------------------------------------------
        cmpi3     r8,r4
        blt       L464                  ; Branch
                                        ; Branch Occurs to L464
        ldiu      *+fp(15),r0
        cmpi      *+fp(11),r0
        bhs       L465                  ; Branch
                                        ; Branch Occurs to L465
        cmpi      *+fp(10),r4
        bge       L465                  ; Branch
                                        ; Branch Occurs to L465
L464:        
	.line	1438
;------------------------------------------------------------------------------
; 2159 | *a = u16tmp + (((UINT16) GetRandNum()) % (rectL-u16tmp));              
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      *+fp(16),r1
        subri     ar5,r1
        call      MOD_U40
                                        ; Call Occurs
        addi      *+fp(16),r0           ; unsigned
        sti       r0,*ar6
	.line	1439
;------------------------------------------------------------------------------
; 2160 | *b = (((UINT16) GetRandNum()) % 3600);                                 
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      3600,r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*ar7
L465:        
	.line	1442
;------------------------------------------------------------------------------
; 2163 | hw = tanLUT[ *a ];                                 //    convert new po
;     | lar coords to (x,y) pix:                                               
;------------------------------------------------------------------------------
        lda       *+fp(57),ir0
        lda       *ar6,ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(3)
	.line	1443
;------------------------------------------------------------------------------
; 2164 | i32val = rectU * hw;                               //    r*2^20= [alpha
;     | X*2^10] * [tan(rDeg)*2^10]                                             
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r4
        mpyi      *+fp(3),r4            ; 32 Bit Mutiply (C40)
	.line	1444
;------------------------------------------------------------------------------
; 2165 | i32val >>= 10;                                     //    r*2^20 --> r*2
;     | ^10                                                                    
;------------------------------------------------------------------------------
        ash       -10,r4                ;asr
	.line	1445
;------------------------------------------------------------------------------
; 2166 | i32val *= cosLUT[ *b ];                            //    x*2^20= [r*2^1
;     | 0] * [cos(theta) * 2^10]                                               
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        lda       *ar7,ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1446
;------------------------------------------------------------------------------
; 2167 | i32val >>= 4;                                      //    x(pix) = [x*2^
;     | 20]/16 = x*65536                                                       
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1447
;------------------------------------------------------------------------------
; 2168 | i32val += xCoord;                                  //    offset by FOE'
;     | s x-coord                                                              
;------------------------------------------------------------------------------
        addi3     ar4,r4,r4             ; unsigned
	.line	1448
;------------------------------------------------------------------------------
; 2169 | xyvals = ((0x0000FFFF & i32val) << 16);            //    pack x-coord f
;     | or download to dotter brd;                                             
; 2170 | //    "wraps" dots that exceed [0..65535]!                             
;------------------------------------------------------------------------------
        and       @CL21,r4
        ash       16,r4
        sti       r4,*+fp(21)
	.line	1451
;------------------------------------------------------------------------------
; 2172 | i32val = rectD * hw;                               //    analogously fo
;     | r the y-coord, except we                                               
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r4
        mpyi      *+fp(3),r4            ; 32 Bit Mutiply (C40)
	.line	1452
;------------------------------------------------------------------------------
; 2173 | i32val >>= 10;                                     //    use the alphaY
;     |  conversion factor, and                                                
;------------------------------------------------------------------------------
        ash       -10,r4                ;asr
	.line	1453
;------------------------------------------------------------------------------
; 2174 | i32val *= sinLUT[ *b ];                            //    sin(theta)... 
;------------------------------------------------------------------------------
        lda       *+fp(59),ir0
        lda       *ar7,ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1454
;------------------------------------------------------------------------------
; 2175 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1455
;------------------------------------------------------------------------------
; 2176 | i32val += yCoord;                                  //    ... and we off
;     | set by FOE's y-coord                                                   
;------------------------------------------------------------------------------
        addi3     r5,r4,r4              ; unsigned
	.line	1456
;------------------------------------------------------------------------------
; 2177 | xyvals |= (0x0000FFFF & i32val);                                       
;------------------------------------------------------------------------------
        and       @CL21,r4
        or        *+fp(21),r4
        sti       r4,*+fp(21)
L466:        
	.line	1458
;------------------------------------------------------------------------------
; 2179 | while( *stataddr & 0x1 );                          //    finally:  draw
;     |  the dot!                                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L466                  ; Branch
                                        ; Branch Occurs to L466
	.line	1459
;------------------------------------------------------------------------------
; 2180 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	1460
;------------------------------------------------------------------------------
; 2181 | visibleDotsXY[nTotalVisDots++] = xyvals;           //    save packed co
;     | ords in visi dots array                                                
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	1462
;------------------------------------------------------------------------------
; 2183 | ++a;                                               //    move on to nex
;     | t dot                                                                  
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	1463
;------------------------------------------------------------------------------
; 2184 | ++b;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	1464
;------------------------------------------------------------------------------
; 2185 | ++nextDotLife;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	1417
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L461                  ; Branch
                                        ; Branch Occurs to L461
        bu        L475
                                        ; Branch Occurs to L475
L469:        
	.line	1467
;------------------------------------------------------------------------------
; 2188 | else while( a < de )                                     //    FOR ACCE
;     | LERATING FLOWS (simpler):                                              
;------------------------------------------------------------------------------
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        bhs       L475                  ; Branch
                                        ; Branch Occurs to L475
L470:        
	.line	1469
;------------------------------------------------------------------------------
; 2190 | i32val = hv * sincosLUT[ *a ];                        //    dr*2^(10+M)
;     | = [B*2^M]*[sin(r)*cos(r)*2^10]                                         
;------------------------------------------------------------------------------
        lda       *+fp(58),ir0
        lda       *ar6,ar0
        ldiu      *+fp(1),r0
        mpyi3     r0,*+ar0(ir0),r4      ; 32 Bit Multiply (C40)
	.line	1470
;------------------------------------------------------------------------------
; 2191 | i32val >>= (2 + vv);                                  //    dr*2^(10+M)
;     |  --> dr*2^8                                                            
;------------------------------------------------------------------------------
        ldiu      *+fp(2),r0
        subri     -2,r0
        ash3      r0,r4,r4              ;asr3
	.line	1471
;------------------------------------------------------------------------------
; 2192 | i32val += (INT32) *nextDotLife;                       //    accum fract
;     | ional pos change from last upd                                         
;------------------------------------------------------------------------------
        lda       *+fp(41),ar0
        addi      *ar0,r4
	.line	1473
;------------------------------------------------------------------------------
; 2194 | *nextDotLife = 0x00FF & i32val;                       //    carry over 
;     | frac pos chng for next update                                          
;------------------------------------------------------------------------------
        ldiu      255,r0
        lda       *+fp(41),ar0
        and       r4,r0
        sti       r0,*ar0
	.line	1474
;------------------------------------------------------------------------------
; 2195 | i32val >>= 8;                                         //    dr*2^8 --> 
;     | dr                                                                     
;------------------------------------------------------------------------------
        ash       -8,r4                 ;asr
	.line	1475
;------------------------------------------------------------------------------
; 2196 | i32val += (INT32) *a;                                 //    r' = r + dr
;------------------------------------------------------------------------------
        addi      *ar6,r4
	.line	1476
;------------------------------------------------------------------------------
; 2197 | *a = (UINT16) i32val;                                 //    update new 
;     | radial pos                                                             
;------------------------------------------------------------------------------
        sti       r4,*ar6
	.line	1477
;------------------------------------------------------------------------------
; 2198 | if( i32val > (INT32) rectL )                          //    randomly re
;     | pos dots that pass outer edge                                          
;------------------------------------------------------------------------------
        cmpi3     ar5,r4
        ble       L472                  ; Branch
                                        ; Branch Occurs to L472
	.line	1479
;------------------------------------------------------------------------------
; 2200 | *a = rectR +                                                           
; 2201 | (((UINT16) GetRandNum()) % (rectL-rectR));                             
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        subi3     r8,ar5,r1             ; unsigned
        call      MOD_U40
                                        ; Call Occurs
        addi3     r0,r8,r0              ; unsigned
        sti       r0,*ar6
	.line	1481
;------------------------------------------------------------------------------
; 2202 | *b = (((UINT16) GetRandNum()) % 3600);                                 
;------------------------------------------------------------------------------
        call      _GetRandNum
                                        ; Call Occurs
        ldiu      3600,r1
        call      MOD_U40
                                        ; Call Occurs
        sti       r0,*ar7
L472:        
	.line	1484
;------------------------------------------------------------------------------
; 2205 | hw = tanLUT[ *a ];                                    //    convert new
;     |  polar coords to (x,y) pix:                                            
;------------------------------------------------------------------------------
        lda       *+fp(57),ir0
        lda       *ar6,ar0
        ldiu      *+ar0(ir0),r0
        sti       r0,*+fp(3)
	.line	1485
;------------------------------------------------------------------------------
; 2206 | i32val = rectU * hw;                                  //    r*2^20= [al
;     | phaX*2^10] * [tan(rDeg)*2^10]                                          
;------------------------------------------------------------------------------
        ldiu      *+fp(63),r4
        mpyi      *+fp(3),r4            ; 32 Bit Mutiply (C40)
	.line	1486
;------------------------------------------------------------------------------
; 2207 | i32val >>= 10;                                        //    r*2^20 --> 
;     | r*2^10                                                                 
;------------------------------------------------------------------------------
        ash       -10,r4                ;asr
	.line	1487
;------------------------------------------------------------------------------
; 2208 | i32val *= cosLUT[ *b ];                               //    x*2^20= [r*
;     | 2^10] * [cos(theta) * 2^10]                                            
;------------------------------------------------------------------------------
        lda       *+fp(60),ir0
        lda       *ar7,ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1488
;------------------------------------------------------------------------------
; 2209 | i32val >>= 4;                                         //    x(pix) = [x
;     | *2^20]/16 = x*65536                                                    
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1489
;------------------------------------------------------------------------------
; 2210 | i32val += xCoord;                                     //    offset by F
;     | OE's x-coord                                                           
;------------------------------------------------------------------------------
        addi3     ar4,r4,r4             ; unsigned
	.line	1490
;------------------------------------------------------------------------------
; 2211 | xyvals = ((0x0000FFFF & i32val) << 16);               //    pack x-coor
;     | d for download to dotter brd                                           
;------------------------------------------------------------------------------
        and       @CL21,r4
        ash       16,r4
        sti       r4,*+fp(21)
	.line	1492
;------------------------------------------------------------------------------
; 2213 | i32val = rectD * hw;                                  //    analogously
;     |  for y-coord, except we use                                            
;------------------------------------------------------------------------------
        ldiu      *+fp(64),r4
        mpyi      *+fp(3),r4            ; 32 Bit Mutiply (C40)
	.line	1493
;------------------------------------------------------------------------------
; 2214 | i32val >>= 10;                                        //    alphaY and 
;     | sin(theta)...                                                          
;------------------------------------------------------------------------------
        ash       -10,r4                ;asr
	.line	1494
;------------------------------------------------------------------------------
; 2215 | i32val *= sinLUT[ *b ];                                                
;------------------------------------------------------------------------------
        lda       *+fp(59),ir0
        lda       *ar7,ar0
        mpyi      *+ar0(ir0),r4         ; 32 Bit Multiply (C40)
	.line	1495
;------------------------------------------------------------------------------
; 2216 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
        ash       -4,r4                 ;asr
	.line	1496
;------------------------------------------------------------------------------
; 2217 | i32val += yCoord;                                     //    ... and we 
;     | offset by FOE's y-coord                                                
;------------------------------------------------------------------------------
        addi3     r5,r4,r4              ; unsigned
	.line	1497
;------------------------------------------------------------------------------
; 2218 | xyvals |= (0x0000FFFF & i32val);                                       
;------------------------------------------------------------------------------
        and       @CL21,r4
        or        *+fp(21),r4
        sti       r4,*+fp(21)
L473:        
	.line	1499
;------------------------------------------------------------------------------
; 2220 | while( *stataddr & 0x1 );                             //    finally:  d
;     | raw the dot!                                                           
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L473                  ; Branch
                                        ; Branch Occurs to L473
	.line	1500
;------------------------------------------------------------------------------
; 2221 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(25),ar0
        ldiu      *+fp(21),r0
        sti       r0,*ar0
	.line	1501
;------------------------------------------------------------------------------
; 2222 | visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed
;     |  coords in visi dots array                                             
;------------------------------------------------------------------------------
        lda       *+fp(65),ar0
        lda       *+fp(54),ir0
        ldiu      *+fp(21),r0
        sti       r0,*+ar0(ir0)
        addi3     1,ar0,r0
        sti       r0,*+fp(65)
	.line	1503
;------------------------------------------------------------------------------
; 2224 | ++a;                                                  //    move on to 
;     | next dot                                                               
;------------------------------------------------------------------------------
        addi      1,ar6                 ; unsigned
	.line	1504
;------------------------------------------------------------------------------
; 2225 | ++b;                                                                   
;------------------------------------------------------------------------------
        addi      1,ar7                 ; unsigned
	.line	1505
;------------------------------------------------------------------------------
; 2226 | ++nextDotLife;                                                         
;------------------------------------------------------------------------------
        ldiu      1,r0
        addi      *+fp(41),r0           ; unsigned
        sti       r0,*+fp(41)
	.line	1467
        ldiu      *+fp(66),r0
        cmpi3     r0,ar6
        blo       L470                  ; Branch
                                        ; Branch Occurs to L470
L475:        
	.line	1510
;------------------------------------------------------------------------------
; 2231 | u16Dummy = (UINT16) data[cd + NREPS];                       // decremen
;     | t #reps for this tgt                                                   
;------------------------------------------------------------------------------
        lda       *+fp(34),ar0
        addi      *+fp(56),ar0          ; unsigned
        ldiu      *+ar0(4),r0
        sti       r0,*+fp(15)
	.line	1511
;------------------------------------------------------------------------------
; 2232 | if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // (be sure
;     |  to mask out dot life decr in reps                                     
; 2233 | u16Type == DL_NOISESPEED )                              // field for th
;     | e "limited dot life" tgt types)                                        
;------------------------------------------------------------------------------
        ldiu      *+fp(14),r0
        cmpi      8,r0
        beq       L478                  ; Branch
                                        ; Branch Occurs to L478
        cmpi      11,r0
        beq       L478                  ; Branch
                                        ; Branch Occurs to L478
        cmpi      13,r0
        bne       L479                  ; Branch
                                        ; Branch Occurs to L479
L478:        
	.line	1513
;------------------------------------------------------------------------------
; 2234 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
        ldiu      255,r0
        and       *+fp(15),r0
        sti       r0,*+fp(15)
L479:        
	.line	1514
;------------------------------------------------------------------------------
; 2235 | nRedrawsLeft[d] = u16Dummy;                                            
;------------------------------------------------------------------------------
        lda       *+fp(51),ir0
        lda       *+fp(33),ar0
        ldiu      *+fp(15),r0
        sti       r0,*+ar0(ir0)
	.line	1515
;------------------------------------------------------------------------------
; 2236 | nRedrawsLeft[d]--;                                                     
; 2237 | }                                                              // END: 
;     |  first pass thru all targets                                           
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(51),ar0          ; unsigned
        ldiu      1,r0
        subri     *ar0,r0               ; unsigned
        sti       r0,*ar0
L480:        
	.line	530
        ldiu      1,r0
        addi      *+fp(33),r0           ; unsigned
        sti       r0,*+fp(33)
        lda       *+fp(48),ar0
        cmpi3     *+ar0(4),r0
        blo       L119                  ; Branch
                                        ; Branch Occurs to L119
L481:        
	.line	1518
;------------------------------------------------------------------------------
; 2239 | if( maxRepeats > 0 ) for( i = 0; i < maxRepeats-1; i++ )       // now c
;     | omplete remaining reps for all tgts                                    
; 2240 | {                                                              // by us
;     | ing the visible dots array we prepared                                 
;------------------------------------------------------------------------------
        ldiu      *+fp(36),r0
        cmpi      0,r0
        beq       L491                  ; Branch
                                        ; Branch Occurs to L491
        stik      0,*+fp(28)
        ldiu      1,r0
        ldiu      *+fp(28),r1
        subri     *+fp(36),r0           ; unsigned
        cmpi3     r0,r1
        bhs       L491                  ; Branch
                                        ; Branch Occurs to L491
L483:        
	.line	1520
;------------------------------------------------------------------------------
; 2241 | nTotalVisDots = 0;                                          // in the f
;     | irst pass!!!  This implementation                                      
;------------------------------------------------------------------------------
        ldiu      0,r0
        sti       r0,*+fp(65)
	.line	1521
;------------------------------------------------------------------------------
; 2242 | for( d = 0; d < parameters->wNumTargets; d++ )              // allows f
;     | or different NREPS values for                                          
; 2243 | {                                                           // differen
;     | t tgts.  Note that its is important                                    
;------------------------------------------------------------------------------
        stik      0,*+fp(33)
        lda       *+fp(48),ar0
        ldiu      *+fp(33),r0
        cmpi3     *+ar0(4),r0
        bhs       L490                  ; Branch
                                        ; Branch Occurs to L490
L484:        
	.line	1523
;------------------------------------------------------------------------------
; 2244 | if( nRedrawsLeft[d] > 0 )                                // to cycle th
;     | ru the targets rather than                                             
; 2245 | {                                                        // redrawing t
;     | gt1 N times, tgt2 M times, etc.                                        
;------------------------------------------------------------------------------
        lda       *+fp(51),ir0
        lda       *+fp(33),ar0
        ldiu      *+ar0(ir0),r0
        cmpi      0,r0
        beq       L489                  ; Branch
                                        ; Branch Occurs to L489
	.line	1525
;------------------------------------------------------------------------------
; 2246 | k = nTotalVisDots + nVisDotsPerTgt[d];                // the former app
;     | roach move evenly distributes                                          
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        ldiu      *+fp(65),r0
        addi3     r0,*+ar0(ir0),r0      ; unsigned
        sti       r0,*+fp(30)
	.line	1526
;------------------------------------------------------------------------------
; 2247 | for( j = nTotalVisDots; j < k; j++ )                  // individual dot
;     |  refreshes over update period.                                         
;------------------------------------------------------------------------------
        ldiu      *+fp(65),r0
        sti       r0,*+fp(29)
        cmpi      *+fp(30),r0
        bhs       L488                  ; Branch
                                        ; Branch Occurs to L488
L486:        
	.line	1528
;------------------------------------------------------------------------------
; 2249 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
        lda       *+fp(26),ar0
        ldiu      1,r0
        and       *ar0,r0
        cmpi      0,r0
        bne       L486                  ; Branch
                                        ; Branch Occurs to L486
	.line	1529
;------------------------------------------------------------------------------
; 2250 | *locaddr = visibleDotsXY[j];                                           
;------------------------------------------------------------------------------
        lda       *+fp(54),ir0
        lda       *+fp(29),ar0
        ldiu      *+ar0(ir0),r0
        lda       *+fp(25),ar0
        sti       r0,*ar0
	.line	1526
        ldiu      1,r0
        addi      *+fp(29),r0           ; unsigned
        sti       r0,*+fp(29)
        cmpi      *+fp(30),r0
        blo       L486                  ; Branch
                                        ; Branch Occurs to L486
L488:        
	.line	1531
;------------------------------------------------------------------------------
; 2252 | nRedrawsLeft[d]--;                                                     
;------------------------------------------------------------------------------
        lda       *+fp(33),ar0
        addi      *+fp(51),ar0          ; unsigned
        ldiu      1,r0
        subri     *ar0,r0               ; unsigned
        sti       r0,*ar0
L489:        
	.line	1533
;------------------------------------------------------------------------------
; 2254 | nTotalVisDots += nVisDotsPerTgt[d];                                    
; 2258 | }                                                                 // EN
;     | D:  process XYCORE_DOFRAME command...                                  
;------------------------------------------------------------------------------
        lda       *+fp(52),ir0
        lda       *+fp(33),ar0
        ldiu      *+fp(65),r0
        addi      *+ar0(ir0),r0         ; unsigned
        sti       r0,*+fp(65)
	.line	1521
        ldiu      1,r0
        addi      *+fp(33),r0           ; unsigned
        sti       r0,*+fp(33)
        lda       *+fp(48),ar0
        cmpi3     *+ar0(4),r0
        blo       L484                  ; Branch
                                        ; Branch Occurs to L484
L490:        
	.line	1518
        ldiu      1,r0
        addi      *+fp(28),r0           ; unsigned
        sti       r0,*+fp(28)
        ldiu      1,r0
        ldiu      *+fp(28),r1
        subri     *+fp(36),r0           ; unsigned
        cmpi3     r0,r1
        blo       L483                  ; Branch
                                        ; Branch Occurs to L483
L491:        
	.line	1540
;------------------------------------------------------------------------------
; 2261 | status = XYCORE_READY;                                            // wr
;     | ite XYCORE_READY into CmdStat register to                              
; 2262 | #if defined(_TGTDAKARF5)                                          // in
;     | form host XYAPI we're ready for next cmd.                              
;------------------------------------------------------------------------------
        stik      1,*+fp(20)
	.line	1542
;------------------------------------------------------------------------------
; 2263 | C4X_Control( PCI, SET_MAILBOX, 2, &status );                           
; 2264 | #elif defined(_TGTDETROIT)                                             
; 2265 | C6x_WritePlx( MAILBOX2_OFFSET, status );                               
; 2266 | #else    // _TGTDAYTONA                                                
; 2267 | *((UINT32 *)0x00400000) = status;                                      
; 2268 | #endif                                                                 
;------------------------------------------------------------------------------
        addi3     fp,20,r3
        ldiu      2,r0
        ldiu      5,r1
        ldiu      2,r2
        push      r3
        push      r0
        push      r1
        push      r2
        call      _C4X_Control
                                        ; Call Occurs
        subi      4,sp                  ; unsigned
	.line	1549
;------------------------------------------------------------------------------
; 2270 | } while( command != XYCORE_CLOSE );                                  //
;     |  END runtime loop                                                      
;------------------------------------------------------------------------------
        ldiu      *+fp(19),r0
        cmpi      4,r0
        bne       L21                   ; Branch
                                        ; Branch Occurs to L21
	.line	1551
;------------------------------------------------------------------------------
; 2272 | }                                                                      
;     |  // END main()                                                         
;------------------------------------------------------------------------------
                                        ; Begin Epilog Code
        ldiu      *-fp(1),r1
        lda       *fp,fp
                                        ; Restore SOE Registers
        pop       r8
        pop       ar7
        pop       ar6
        pop       ar5
        pop       ar4
        popf      f7
        popf      f6
        pop       r5
        pop       r4
                                        ; Unallocate the Frame
        subi      68,sp                 ; unsigned
        bu        r1
                                        ; Branch Occurs to r1
	.endfunc	2272,00040f0f0h,66


;******************************************************************************
;* CONSTANT TABLE                                                             *
;******************************************************************************
	.sect	".const"
	.bss	CL1,1
	.bss	CL2,1
	.bss	CL3,1
	.bss	CL4,1
	.bss	CL5,1
	.bss	CL6,1
	.bss	CL7,1
	.bss	CL8,1
	.bss	CL9,1
	.bss	CL10,1
	.bss	CL11,1
	.bss	CL12,1
	.bss	CL13,1
	.bss	CL14,1
	.bss	CL15,1
	.bss	CL16,1
	.bss	CL17,1
	.bss	CL18,1
	.bss	CL19,1
	.bss	CL20,1
	.bss	CL21,1

	.sect	".cinit"
	.field  	21,32
	.field  	CL1+0,32
	.field  	2147437301,32
	.field  	453816981,32
	.field  	65535,32
	.field  	-1071644668,32
	.field  	-1073676288,32
	.field  	-1073676287,32
	.field  	-1070593792,32
	.field  	-1070593463,32
	.field  	-2147483648,32
	.field  	-2147483648,32
	.word   	020000000H ; float   4.294967296000000e+09
	.word   	0F33702D4H ; double  1.745329300000000e-04
	.word   	00A000000H ; double  1.024000000000000e+03
	.word   	0FF000000H ; double  5.000000000000000e-01
	.word   	0F664C389H ; double  1.745329300000000e-03
	.field  	32768,32
	.field  	32768,32
	.field  	-1,32
	.field  	65535,32
	.field  	65536,32
	.field  	65535,32

	.sect	".text"
;******************************************************************************
;* UNDEFINED EXTERNAL REFERENCES                                              *
;******************************************************************************

	.global	_cos

	.global	_floor

	.global	_log

	.global	_pow

	.global	_sin

	.global	_tan

	.global	_C4X_Open

	.global	_C4X_Control
	.global	DIV_F40
	.global	DIV_U40
	.global	DIV_I40
	.global	MOD_U40
	.global	MOD_I40
