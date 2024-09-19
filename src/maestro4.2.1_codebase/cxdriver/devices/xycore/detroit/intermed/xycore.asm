;******************************************************************************
;* TMS320C6x ANSI C Codegen                                      Version 4.00 *
;* Date/Time created: Fri May 13 18:18:44 2011                                *
;******************************************************************************

;******************************************************************************
;* GLOBAL FILE PARAMETERS                                                     *
;*                                                                            *
;*   Architecture      : TMS320C620x                                          *
;*   Optimization      : Disabled                                             *
;*   Optimizing for    : Compile time, Ease of Development                    *
;*                       Based on options: no -o, no -ms                      *
;*   Endian            : Little                                               *
;*   Interrupt Thrshld : Disabled                                             *
;*   Memory Model      : Small                                                *
;*   Calls to RTS      : Near                                                 *
;*   Pipelining        : Disabled                                             *
;*   Memory Aliases    : Presume are aliases (pessimistic)                    *
;*   Debug Info        : Debug                                                *
;*                                                                            *
;******************************************************************************

FP	.set	A15
DP	.set	B14
SP	.set	B15
	.global	$bss

;	acp6x -q -D_TGTDETROIT -Iinclude -I.. -m --i_output_file intermed\xycore.if --template_info_file intermed\xycore.ti --object_file intermed\xycore.o6x --opt_shell 13 ..\xycore.c -al -c -eo.o6x -gkqs -D_TGTDETROIT -frintermed -fsintermed -ftintermed -iinclude -i.. -c ..\xycore.c ..\xycore.c 
	.file	"..\xycore.c"
	.file	"include\type_c6x.h"
	.sym	_INT16, 0, 3, 13, 16
	.sym	_UINT16, 0, 13, 13, 16
	.sym	_INT32, 0, 4, 13, 32
	.sym	_RESULT, 0, 4, 13, 32
	.sym	_UINT32, 0, 14, 13, 32
	.file	"..\xycore.c"
	.stag	_Parameters, 5280
	.member	_dwDotSeed, 0, 14, 8, 32
	.member	_wWidthMM, 32, 13, 8, 16
	.member	_wHeightMM, 48, 13, 8, 16
	.member	_wDistMM, 64, 13, 8, 16
	.member	_wNumTargets, 80, 13, 8, 16
	.member	_wDelayPerDot, 96, 13, 8, 16
	.member	_wOnTimePerDot, 112, 13, 8, 16
	.member	_wFiller, 128, 61, 8, 32, , 2
	.member	_wType, 160, 61, 8, 512, , 32
	.member	_wNumDots, 672, 61, 8, 512, , 32
	.member	_wRectR, 1184, 61, 8, 512, , 32
	.member	_wRectL, 1696, 61, 8, 512, , 32
	.member	_wRectT, 2208, 61, 8, 512, , 32
	.member	_wRectB, 2720, 61, 8, 512, , 32
	.member	_wOuterR, 3232, 61, 8, 512, , 32
	.member	_wOuterL, 3744, 61, 8, 512, , 32
	.member	_wOuterT, 4256, 61, 8, 512, , 32
	.member	_wOuterB, 4768, 61, 8, 512, , 32
	.eos
	.sym	_PARAMETERS, 0, 8, 13, 5280,_Parameters
	.file	"c:\c6xtools\include\math.h"
	.file	"include\de62c6x.h"
	.file	"..\xycore.c"

	.sect	".cinit:c"
	.align	8
	.field  	(CIR - $) - 8, 32
	.field  	_G_lastRandomNum+0,32
	.field  	0x1,32			; _G_lastRandomNum @ 0

	.sect	".text"
	.global	_G_lastRandomNum
_G_lastRandomNum:	.usect	".bss:c",4,4
	.sym	_G_lastRandomNum,_G_lastRandomNum, 14, 2, 32

	.sect	".cinit:c"
	.align	4
	.field  	0x1,32			; _G_lastRand2 @ 0

	.sect	".text"
	.global	_G_lastRand2
_G_lastRand2:	.usect	".bss:c",4,4
	.sym	_G_lastRand2,_G_lastRand2, 14, 2, 32
	.sect	".text"
	.global	_SetSeed
	.sym	_SetSeed,_SetSeed, 32, 2, 0
	.func	669
;------------------------------------------------------------------------------
; 669 | void SetSeed( unsigned int seed )                                      
;------------------------------------------------------------------------------

;******************************************************************************
;* FUNCTION NAME: _SetSeed                                                    *
;*                                                                            *
;*   Regs Modified     : SP                                                   *
;*   Regs Used         : A4,B3,DP,SP                                          *
;*   Local Frame Size  : 0 Args + 4 Auto + 0 Save = 4 byte                    *
;******************************************************************************
_SetSeed:
;** --------------------------------------------------------------------------*
	.sym	_seed,4, 14, 17, 32
	.sym	_seed,4, 14, 1, 32
           SUB     .L2     SP,8,SP           ; |669| 
	.line	2
           STW     .D2T1   A4,*+SP(4)        ; |670| 
	.line	3
;------------------------------------------------------------------------------
; 671 | G_lastRandomNum = seed;                                                
;------------------------------------------------------------------------------
           STW     .D2T1   A4,*+DP(_G_lastRandomNum) ; |671| 
	.line	4
           B       .S2     B3                ; |672| 
           ADD     .L2     8,SP,SP           ; |672| 
           NOP             4
           ; BRANCH OCCURS                   ; |672| 
	.endfunc	672,000000000h,8


	.sect	".text"
	.global	_SetSeed2
	.sym	_SetSeed2,_SetSeed2, 32, 2, 0
	.func	674
;------------------------------------------------------------------------------
; 674 | void SetSeed2( unsigned int seed )                                     
;------------------------------------------------------------------------------

;******************************************************************************
;* FUNCTION NAME: _SetSeed2                                                   *
;*                                                                            *
;*   Regs Modified     : SP                                                   *
;*   Regs Used         : A4,B3,DP,SP                                          *
;*   Local Frame Size  : 0 Args + 4 Auto + 0 Save = 4 byte                    *
;******************************************************************************
_SetSeed2:
;** --------------------------------------------------------------------------*
	.sym	_seed,4, 14, 17, 32
	.sym	_seed,4, 14, 1, 32
           SUB     .L2     SP,8,SP           ; |674| 
	.line	2
           STW     .D2T1   A4,*+SP(4)        ; |675| 
	.line	3
;------------------------------------------------------------------------------
; 676 | G_lastRand2 = seed;                                                    
;------------------------------------------------------------------------------
           STW     .D2T1   A4,*+DP(_G_lastRand2) ; |676| 
	.line	4
           B       .S2     B3                ; |677| 
           ADD     .L2     8,SP,SP           ; |677| 
           NOP             4
           ; BRANCH OCCURS                   ; |677| 
	.endfunc	677,000000000h,8


	.sect	".text"
	.global	_GetRandNum
	.sym	_GetRandNum,_GetRandNum, 45, 2, 0
	.func	697
;------------------------------------------------------------------------------
; 697 | unsigned short GetRandNum()                                            
;------------------------------------------------------------------------------

;******************************************************************************
;* FUNCTION NAME: _GetRandNum                                                 *
;*                                                                            *
;*   Regs Modified     : A0,A3,A4,B4,B5,B6,B7,SP                              *
;*   Regs Used         : A0,A3,A4,B3,B4,B5,B6,B7,DP,SP                        *
;*   Local Frame Size  : 0 Args + 8 Auto + 0 Save = 8 byte                    *
;******************************************************************************
_GetRandNum:
;** --------------------------------------------------------------------------*
	.sym	_a,4, 14, 1, 32
	.sym	_c,8, 14, 1, 32
           SUB     .L2     SP,8,SP           ; |697| 
	.line	3
;------------------------------------------------------------------------------
; 699 | unsigned int a = 2147437301, c = 453816981;                            
;------------------------------------------------------------------------------
           MVKL    .S1     0x7fff4af5,A0     ; |699| 
           MVKH    .S1     0x7fff4af5,A0     ; |699| 
           STW     .D2T1   A0,*+SP(4)        ; |699| 
           MVKL    .S2     0x1b0cb295,B4     ; |699| 
           MVKH    .S2     0x1b0cb295,B4     ; |699| 
           STW     .D2T2   B4,*+SP(8)        ; |699| 
	.line	5
;------------------------------------------------------------------------------
; 701 | G_lastRandomNum = a * G_lastRandomNum + c;                             
;------------------------------------------------------------------------------
           LDW     .D2T2   *+DP(_G_lastRandomNum),B5 ; |701| 
           NOP             4

           MPYLH   .M2X    A0,B5,B7          ; |701| 
||         MPYLH   .M1X    B5,A0,A3          ; |701| 

           MPYU    .M2X    B5,A0,B6          ; |701| 
           ADD     .L2X    A3,B7,B5          ; |701| 
           SHL     .S2     B5,16,B5          ; |701| 
           ADD     .L2     B6,B5,B5          ; |701| 
           ADD     .L2     B4,B5,B4          ; 
           STW     .D2T2   B4,*+DP(_G_lastRandomNum) ; |701| 
	.line	6
;------------------------------------------------------------------------------
; 702 | return ( (unsigned short) (0x0000FFFF & (G_lastRandomNum >> 8)) );     
;------------------------------------------------------------------------------
           MV      .L1X    B4,A0
           EXTU    .S1     A0,8,16,A0        ; |702| 
           EXTU    .S1     A0,16,16,A4       ; |702| 
	.line	7
           B       .S2     B3                ; |703| 
           ADD     .L2     8,SP,SP           ; |703| 
           NOP             4
           ; BRANCH OCCURS                   ; |703| 
	.endfunc	703,000000000h,8


	.sect	".text"
	.global	_GetRandNum2
	.sym	_GetRandNum2,_GetRandNum2, 45, 2, 0
	.func	705
;------------------------------------------------------------------------------
; 705 | unsigned short GetRandNum2()                                           
;------------------------------------------------------------------------------

;******************************************************************************
;* FUNCTION NAME: _GetRandNum2                                                *
;*                                                                            *
;*   Regs Modified     : A0,A3,A4,B4,B5,B6,B7,SP                              *
;*   Regs Used         : A0,A3,A4,B3,B4,B5,B6,B7,DP,SP                        *
;*   Local Frame Size  : 0 Args + 8 Auto + 0 Save = 8 byte                    *
;******************************************************************************
_GetRandNum2:
;** --------------------------------------------------------------------------*
	.sym	_a,4, 14, 1, 32
	.sym	_c,8, 14, 1, 32
           SUB     .L2     SP,8,SP           ; |705| 
	.line	3
;------------------------------------------------------------------------------
; 707 | unsigned int a = 2147437301, c = 453816981;                            
;------------------------------------------------------------------------------
           MVKL    .S1     0x7fff4af5,A0     ; |707| 
           MVKH    .S1     0x7fff4af5,A0     ; |707| 
           STW     .D2T1   A0,*+SP(4)        ; |707| 
           MVKL    .S2     0x1b0cb295,B4     ; |707| 
           MVKH    .S2     0x1b0cb295,B4     ; |707| 
           STW     .D2T2   B4,*+SP(8)        ; |707| 
	.line	5
;------------------------------------------------------------------------------
; 709 | G_lastRand2 = a * G_lastRand2 + c;                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+DP(_G_lastRand2),B5 ; |709| 
           NOP             4

           MPYLH   .M2X    A0,B5,B7          ; |709| 
||         MPYLH   .M1X    B5,A0,A3          ; |709| 

           MPYU    .M2X    B5,A0,B6          ; |709| 
           ADD     .L2X    A3,B7,B5          ; |709| 
           SHL     .S2     B5,16,B5          ; |709| 
           ADD     .L2     B6,B5,B5          ; |709| 
           ADD     .L2     B4,B5,B4          ; 
           STW     .D2T2   B4,*+DP(_G_lastRand2) ; |709| 
	.line	6
;------------------------------------------------------------------------------
; 710 | return ( (unsigned short) (0x0000FFFF & (G_lastRand2 >> 8)) );         
;------------------------------------------------------------------------------
           MV      .L1X    B4,A0
           EXTU    .S1     A0,8,16,A0        ; |710| 
           EXTU    .S1     A0,16,16,A4       ; |710| 
	.line	7
           B       .S2     B3                ; |711| 
           ADD     .L2     8,SP,SP           ; |711| 
           NOP             4
           ; BRANCH OCCURS                   ; |711| 
	.endfunc	711,000000000h,8


	.sym	_INT16, 0, 3, 13, 16
	.sym	_PARAMETERS, 0, 8, 13, 5280,_Parameters
	.sym	_UINT16, 0, 13, 13, 16
	.sym	_UINT32, 0, 14, 13, 32
	.sym	_INT32, 0, 4, 13, 32
	.sect	".text"
	.global	_main
	.sym	_main,_main, 32, 2, 0
	.func	722
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
; 839 | C4X_Open( SHARED_SRAM_512K );                                     // DS
;     | PLINK3. not all operations are required                                
; 840 | *((UINT32 *)0xC0200004) = 0x00000001;                             // fo
;     | r every DSP target...                                                  
; 841 | *((UINT32 *)0xC0200004) = 0x00000000;                                  
; 842 | #elif defined(_TGTDETROIT)                                             
;------------------------------------------------------------------------------

;******************************************************************************
;* FUNCTION NAME: _main                                                       *
;*                                                                            *
;*   Regs Modified     : A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,   *
;*                           A15,B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,B10,B11,B12,SP *
;*   Regs Used         : A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,   *
;*                           A15,B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,B10,B11,B12,SP *
;*   Local Frame Size  : 0 Args + 224 Auto + 40 Save = 264 byte               *
;******************************************************************************
_main:
;** --------------------------------------------------------------------------*
	.sym	_hv,4, 3, 1, 16
	.sym	_vv,6, 3, 1, 16
	.sym	_hw,8, 3, 1, 16
	.sym	_vw,10, 3, 1, 16
	.sym	_nDotLifeDecr,12, 3, 1, 16
	.sym	_i32val,27, 4, 4, 32
	.sym	_x32,16, 4, 1, 32
	.sym	_y32,20, 4, 1, 32
	.sym	_i16Theta,24, 3, 1, 16
	.sym	_i16Scale,26, 3, 1, 16
	.sym	_xCoord,10, 13, 4, 16
	.sym	_yCoord,11, 13, 4, 16
	.sym	_rectR,8, 13, 4, 16
	.sym	_rectL,24, 13, 4, 16
	.sym	_rectU,13, 13, 4, 16
	.sym	_rectD,25, 13, 4, 16
	.sym	_rectW,28, 13, 1, 16
	.sym	_rectH,30, 13, 1, 16
	.sym	_screenW_mm,32, 13, 1, 16
	.sym	_screenH_mm,34, 13, 1, 16
	.sym	_u16Type,36, 13, 1, 16
	.sym	_u16Dummy,38, 13, 1, 16
	.sym	_u16tmp,40, 13, 1, 16
	.sym	_u16Over,42, 13, 1, 16
	.sym	_nMaxDotLife,44, 13, 1, 16
	.sym	_command,48, 14, 1, 32
	.sym	_status,52, 14, 1, 32
	.sym	_xyvals,56, 14, 1, 32
	.sym	_timdurbyte,60, 14, 1, 32
	.sym	_timdelnib,64, 14, 1, 32
	.sym	_timvals,68, 14, 1, 32
	.sym	_locaddr,72, 30, 1, 32
	.sym	_stataddr,76, 30, 1, 32
	.sym	_timaddr,80, 30, 1, 32
	.sym	_i,84, 14, 1, 32
	.sym	_j,88, 14, 1, 32
	.sym	_k,92, 14, 1, 32
	.sym	_l,96, 14, 1, 32
	.sym	_m,100, 14, 1, 32
	.sym	_d,104, 14, 1, 32
	.sym	_cd,108, 14, 1, 32
	.sym	_dotPosOffset,112, 14, 1, 32
	.sym	_nTotalVisDots,12, 14, 4, 32
	.sym	_maxRepeats,116, 14, 1, 32
	.sym	_dDummy,120, 7, 1, 64
	.sym	_a,9, 29, 4, 32
	.sym	_b,7, 29, 4, 32
	.sym	_de,14, 29, 4, 32
	.sym	_xdotpos,128, 29, 1, 32
	.sym	_ydotpos,132, 29, 1, 32
	.sym	_dotLife,136, 19, 1, 32
	.sym	_nextDotLife,140, 19, 1, 32
	.sym	_dotNoise,144, 19, 1, 32
	.sym	_nextDotNoise,148, 19, 1, 32
	.sym	_fracDX,152, 19, 1, 32
	.sym	_nextFracDX,156, 19, 1, 32
	.sym	_fracDY,160, 19, 1, 32
	.sym	_nextFracDY,164, 19, 1, 32
	.sym	_parameters,168, 24, 1, 32, _Parameters
	.sym	_hsize,172, 29, 1, 32
	.sym	_vsize,176, 29, 1, 32
	.sym	_nRedrawsLeft,180, 29, 1, 32
	.sym	_nVisDotsPerTgt,184, 29, 1, 32
	.sym	_nNoiseUpdTicks,188, 19, 1, 32
	.sym	_visibleDotsXY,192, 30, 1, 32
	.sym	_SharedArray,196, 30, 1, 32
	.sym	_data,200, 19, 1, 32
	.sym	_tanLUT,204, 19, 1, 32
	.sym	_sincosLUT,208, 19, 1, 32
	.sym	_sinLUT,212, 19, 1, 32
	.sym	_cosLUT,216, 19, 1, 32
	.sym	_pow2LUT,220, 20, 1, 32
	.sym	_speedNoiseAdj,224, 20, 1, 32
           ADDK    .S2     -264,SP           ; |722| 
           STW     .D2T1   A10,*+SP(228)     ; |722| 
           STW     .D2T1   A11,*+SP(232)     ; |722| 
           STW     .D2T1   A12,*+SP(236)     ; |722| 
           STW     .D2T1   A13,*+SP(240)     ; |722| 
           STW     .D2T1   A14,*+SP(244)     ; |722| 
           STW     .D2T2   B3,*+SP(248)      ; |722| 
           STW     .D2T2   B10,*+SP(252)     ; |722| 
           STW     .D2T2   B11,*+SP(256)     ; |722| 
           STW     .D2T2   B12,*+SP(260)     ; |722| 
           STW     .D2T1   A15,*+SP(264)     ; |722| 
	.line	122
;------------------------------------------------------------------------------
; 843 | C6x_OpenC6x( NO_FLAGS );                                               
;------------------------------------------------------------------------------
           B       .S1     _C6x_OpenC6x      ; |843| 
           MVKL    .S2     RL84,B3           ; |843| 
           MVKH    .S2     RL84,B3           ; |843| 
           ZERO    .L1     A4                ; |843| 
           NOP             2
RL84:      ; CALL OCCURS                     ; |843| 
	.line	123
;------------------------------------------------------------------------------
; 844 | C6x_OpenPlx( NO_FLAGS );                                               
;------------------------------------------------------------------------------
           B       .S1     _C6x_OpenPlx      ; |844| 
           MVKL    .S2     RL86,B3           ; |844| 
           MVKH    .S2     RL86,B3           ; |844| 
           ZERO    .L1     A4                ; |844| 
           NOP             2
RL86:      ; CALL OCCURS                     ; |844| 
	.line	124
;------------------------------------------------------------------------------
; 845 | C6x_ControlResetDspLink3( DE62_CONTROL_RELEASE_DL3_RESET );            
;------------------------------------------------------------------------------
           B       .S1     _C6x_ControlResetDspLink3 ; |845| 
           MVKL    .S2     RL88,B3           ; |845| 
           MVKH    .S2     RL88,B3           ; |845| 
           MVK     .S1     0x2,A4            ; |845| 
           NOP             2
RL88:      ; CALL OCCURS                     ; |845| 
	.line	125
;------------------------------------------------------------------------------
; 846 | C6x_ControlLed( DE62_C6X_CONTROL_LED_GP_OFF );                         
; 847 | #else    // _TGTDAYTONA                                                
; 848 | C6x_OpenC6x( NO_FLAGS );                                               
; 849 | C6x_OpenHurricane( NO_FLAGS );                                         
; 850 | C6x_ControlResetDspLink3( FT_CONTROL_RELEASE_DL3_RESET );              
; 851 | C6x_ControlLed( FT_C6X_LED_0_OFF | FT_C6X_LED_1_OFF );                 
; 852 | #endif                                                                 
;------------------------------------------------------------------------------
           B       .S1     _C6x_ControlLed   ; |846| 
           MVKL    .S2     RL90,B3           ; |846| 
           MVKH    .S2     RL90,B3           ; |846| 
           MVK     .S1     0x2,A4            ; |846| 
           NOP             2
RL90:      ; CALL OCCURS                     ; |846| 
	.line	133
;------------------------------------------------------------------------------
; 854 | locaddr = (UINT32 *) LOCADDR;                                        //
;     |  set up addresses to access dotter board regs                          
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |854| 
           MVKH    .S2     0x1740000,B4      ; |854| 
           STW     .D2T2   B4,*+SP(72)       ; |854| 
	.line	134
;------------------------------------------------------------------------------
; 855 | stataddr = (UINT32 *) STATADDR;                                        
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |855| 
           MVKH    .S2     0x1740000,B4      ; |855| 
           STW     .D2T2   B4,*+SP(76)       ; |855| 
	.line	135
;------------------------------------------------------------------------------
; 856 | timaddr = (UINT32 *) TIMADDR;                                          
;------------------------------------------------------------------------------
           MVKL    .S1     0x1740004,A0      ; |856| 
           MVKH    .S1     0x1740004,A0      ; |856| 
           STW     .D2T1   A0,*+SP(80)       ; |856| 
	.line	137
;------------------------------------------------------------------------------
; 858 | SharedArray = (UINT32*) SHDATA_BASE;                                 //
;     |  "allocate" all program array in the local                             
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |858| 
           MVKH    .S2     0x1400000,B4      ; |858| 
           STW     .D2T2   B4,*+SP(196)      ; |858| 
	.line	138
;------------------------------------------------------------------------------
; 859 | data = (INT16*) ((UINT32)SHDATA_BASE + (UINT32)sizeof(PARAMETERS));  //
;     |  processor's memory map by assigning the                               
; 860 | // approp memory addresses to pointer vars...                          
;------------------------------------------------------------------------------
           MVKL    .S2     0x1400294,B4      ; |859| 
           MVKH    .S2     0x1400294,B4      ; |859| 
           STW     .D2T2   B4,*+SP(200)      ; |859| 
	.line	140
;------------------------------------------------------------------------------
; 861 | i = 0;                                                                 
; 862 | #if defined(_TGTDETROIT)                                             //
;     |  for the Detroit, dot pos arrays are stored                            
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |861| 
           STW     .D2T2   B4,*+SP(84)       ; |861| 
	.line	142
;------------------------------------------------------------------------------
; 863 | xdotpos = (UINT16 *) DE62_C6X_LOCAL_SSRAM_START;                  // in
;     |  a faster "local memory" region than the                               
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |863| 
           MVKH    .S1     0x400000,A0       ; |863| 
           STW     .D2T1   A0,*+SP(128)      ; |863| 
	.line	143
;------------------------------------------------------------------------------
; 864 | i += MAXTOTALDOTS * sizeof(UINT16);                               // ot
;     | her vars: SSRAM is ~5% faster than SDRAM                               
;------------------------------------------------------------------------------
           MVK     .S2     15000,B5          ; |864| 
           ADDAW   .D2     B4,B5,B4          ; |864| 
           STW     .D2T2   B4,*+SP(84)       ; |864| 
	.line	144
;------------------------------------------------------------------------------
; 865 | ydotpos = (UINT16 *) (DE62_C6X_LOCAL_SSRAM_START + i);                 
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |865| 
           MVKH    .S1     0x400000,A0       ; |865| 
           ADD     .L2X    A0,B4,B4          ; 
           STW     .D2T2   B4,*+SP(132)      ; |865| 
	.line	145
;------------------------------------------------------------------------------
; 866 | i = 0;                                                                 
; 867 | #else    // _TGTDAYTONA, _TGTDAKARF5                                   
; 868 | xdotpos = (UINT16 *) LOCALDATA_BASE;                                   
; 869 | i += MAXTOTALDOTS * sizeof(UINT16);                                    
; 870 | ydotpos = (UINT16 *) (LOCALDATA_BASE + i);                             
; 871 | i += MAXTOTALDOTS * sizeof(UINT16);                                    
; 872 | #endif                                                                 
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |866| 
           STW     .D2T1   A0,*+SP(84)       ; |866| 
	.line	153
;------------------------------------------------------------------------------
; 874 | dotLife = (volatile INT16 *) (LOCALDATA_BASE + i);                   //
;     |  REM: sizeof() returns sizes in units of the                           
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |874| 
           MVKH    .S2     0x2000000,B4      ; |874| 
           ADD     .L1X    B4,A0,A3          ; 
           STW     .D2T1   A3,*+SP(136)      ; |874| 
	.line	154
;------------------------------------------------------------------------------
; 875 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  fundamental data size of tgt processor. On                            
;------------------------------------------------------------------------------
           MVK     .S1     15000,A3          ; |875| 
           ADDAW   .D1     A0,A3,A0          ; |875| 
           STW     .D2T1   A0,*+SP(84)       ; |875| 
	.line	155
;------------------------------------------------------------------------------
; 876 | dotNoise = (volatile INT16 *) (LOCALDATA_BASE + i);                  //
;     |  the Detroit/Daytona, this is a byte, but on                           
;------------------------------------------------------------------------------
           ZERO    .L1     A3                ; |876| 
           MVKH    .S1     0x2000000,A3      ; |876| 
           ADD     .L1     A3,A0,A3          ; 
           STW     .D2T1   A3,*+SP(144)      ; |876| 
	.line	156
;------------------------------------------------------------------------------
; 877 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  the Dakar it is a 4-byte word.  All arrays                            
;------------------------------------------------------------------------------
           MVK     .S1     15000,A3          ; |877| 
           ADDAW   .D1     A0,A3,A0          ; |877| 
           STW     .D2T1   A0,*+SP(84)       ; |877| 
	.line	157
;------------------------------------------------------------------------------
; 878 | fracDX = (volatile INT16 *) (LOCALDATA_BASE + i);                    //
;     |  on the Dakar will be INT32 or UINT32,                                 
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |878| 
           MVKH    .S2     0x2000000,B4      ; |878| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(152)      ; |878| 
	.line	158
;------------------------------------------------------------------------------
; 879 | i += MAXTOTALDOTS * sizeof(INT16);                                   //
;     |  regardless of the types we use here!!!!!                              
;------------------------------------------------------------------------------
           ADDAW   .D1     A0,A3,A3          ; |879| 
           STW     .D2T1   A3,*+SP(84)       ; |879| 
	.line	159
;------------------------------------------------------------------------------
; 880 | fracDY = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |880| 
           MVKH    .S1     0x2000000,A0      ; |880| 
           ADD     .L1     A0,A3,A0          ; 
           STW     .D2T1   A0,*+SP(160)      ; |880| 
	.line	160
;------------------------------------------------------------------------------
; 881 | i += MAXTOTALDOTS * sizeof(INT16);                                     
;------------------------------------------------------------------------------
           MVK     .S1     15000,A0          ; |881| 
           ADDAW   .D1     A3,A0,A0          ; |881| 
           STW     .D2T1   A0,*+SP(84)       ; |881| 
	.line	161
;------------------------------------------------------------------------------
; 882 | hsize = (volatile UINT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |882| 
           MVKH    .S2     0x2000000,B4      ; |882| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(172)      ; |882| 
	.line	162
;------------------------------------------------------------------------------
; 883 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
           ADDK    .S1     64,A0             ; |883| 
           STW     .D2T1   A0,*+SP(84)       ; |883| 
	.line	163
;------------------------------------------------------------------------------
; 884 | vsize = (volatile UINT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
           ZERO    .L1     A3                ; |884| 
           MVKH    .S1     0x2000000,A3      ; |884| 
           ADD     .L1     A3,A0,A3          ; 
           STW     .D2T1   A3,*+SP(176)      ; |884| 
	.line	164
;------------------------------------------------------------------------------
; 885 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
           ADDK    .S1     64,A0             ; |885| 
           STW     .D2T1   A0,*+SP(84)       ; |885| 
	.line	165
;------------------------------------------------------------------------------
; 886 | nRedrawsLeft = (volatile UINT16 *) (LOCALDATA_BASE + i);               
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |886| 
           MVKH    .S2     0x2000000,B4      ; |886| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(180)      ; |886| 
	.line	166
;------------------------------------------------------------------------------
; 887 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
           ADDK    .S1     64,A0             ; |887| 
           STW     .D2T1   A0,*+SP(84)       ; |887| 
	.line	167
;------------------------------------------------------------------------------
; 888 | nVisDotsPerTgt = (volatile UINT16 *) (LOCALDATA_BASE + i);             
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |888| 
           MVKH    .S2     0x2000000,B4      ; |888| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(184)      ; |888| 
	.line	168
;------------------------------------------------------------------------------
; 889 | i += MAX_TARGETS * sizeof(UINT16);                                     
;------------------------------------------------------------------------------
           ADDK    .S1     64,A0             ; |889| 
           STW     .D2T1   A0,*+SP(84)       ; |889| 
	.line	169
;------------------------------------------------------------------------------
; 890 | nNoiseUpdTicks = (volatile INT16 *) (LOCALDATA_BASE + i);              
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |890| 
           MVKH    .S2     0x2000000,B4      ; |890| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(188)      ; |890| 
	.line	170
;------------------------------------------------------------------------------
; 891 | i += MAX_TARGETS * sizeof(INT16);                                      
;------------------------------------------------------------------------------
           ADDK    .S1     64,A0             ; |891| 
           STW     .D2T1   A0,*+SP(84)       ; |891| 
	.line	171
;------------------------------------------------------------------------------
; 892 | visibleDotsXY = (volatile UINT32 *) (LOCALDATA_BASE + i);              
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |892| 
           MVKH    .S2     0x2000000,B4      ; |892| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(192)      ; |892| 
	.line	172
;------------------------------------------------------------------------------
; 893 | i += MAXDOTSPERFRAME * sizeof(UINT32);                                 
;------------------------------------------------------------------------------
;** --------------------------------------------------------------------------*
           ADDK    .S1     16000,A0          ; |893| 
           STW     .D2T1   A0,*+SP(84)       ; |893| 
	.line	174
;------------------------------------------------------------------------------
; 895 | tanLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                    //
;     |  lookup tables for OPTICFLOW animation calcs!                          
;------------------------------------------------------------------------------
           ZERO    .L1     A3                ; |895| 
           MVKH    .S1     0x2000000,A3      ; |895| 
           ADD     .L1     A3,A0,A3          ; 
           STW     .D2T1   A3,*+SP(204)      ; |895| 
	.line	175
;------------------------------------------------------------------------------
; 896 | i += 4500 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
           ADDK    .S1     9000,A0           ; |896| 
           STW     .D2T1   A0,*+SP(84)       ; |896| 
	.line	176
;------------------------------------------------------------------------------
; 897 | sincosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                   
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |897| 
           MVKH    .S2     0x2000000,B4      ; |897| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(208)      ; |897| 
	.line	177
;------------------------------------------------------------------------------
; 898 | i += 4500 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
           ADDK    .S1     9000,A0           ; |898| 
           STW     .D2T1   A0,*+SP(84)       ; |898| 
	.line	178
;------------------------------------------------------------------------------
; 899 | sinLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
           ZERO    .L1     A3                ; |899| 
           MVKH    .S1     0x2000000,A3      ; |899| 
           ADD     .L1     A3,A0,A3          ; 
           STW     .D2T1   A3,*+SP(212)      ; |899| 
	.line	179
;------------------------------------------------------------------------------
; 900 | i += 3600 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
           ADDK    .S1     7200,A0           ; |900| 
           STW     .D2T1   A0,*+SP(84)       ; |900| 
	.line	180
;------------------------------------------------------------------------------
; 901 | cosLUT = (volatile INT16 *) (LOCALDATA_BASE + i);                      
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |901| 
           MVKH    .S2     0x2000000,B4      ; |901| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(216)      ; |901| 
	.line	181
;------------------------------------------------------------------------------
; 902 | i += 3600 * sizeof(INT16);                                             
;------------------------------------------------------------------------------
           ADDK    .S1     7200,A0           ; |902| 
           STW     .D2T1   A0,*+SP(84)       ; |902| 
	.line	183
;------------------------------------------------------------------------------
; 904 | pow2LUT = (volatile INT32 *) (LOCALDATA_BASE + i);                   //
;     |  special lookup tables for NOISYSPEED target's                         
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |904| 
           MVKH    .S2     0x2000000,B4      ; |904| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(220)      ; |904| 
	.line	184
;------------------------------------------------------------------------------
; 905 | i += 281 * sizeof(INT32);                                            //
;     |  multiplicative noise algorithm                                        
;------------------------------------------------------------------------------
           ADDK    .S1     1124,A0           ; |905| 
           STW     .D2T1   A0,*+SP(84)       ; |905| 
	.line	185
;------------------------------------------------------------------------------
; 906 | speedNoiseAdj = (volatile INT32 *) (LOCALDATA_BASE + i);               
;------------------------------------------------------------------------------
           ZERO    .L1     A3                ; |906| 
           MVKH    .S1     0x2000000,A3      ; |906| 
           ADD     .L1     A3,A0,A3          ; 
           STW     .D2T1   A3,*+SP(224)      ; |906| 
	.line	186
;------------------------------------------------------------------------------
; 907 | i += 7 * sizeof(INT32);                                                
;------------------------------------------------------------------------------
           ADDK    .S1     28,A0             ; |907| 
           STW     .D2T1   A0,*+SP(84)       ; |907| 
	.line	188
;------------------------------------------------------------------------------
; 909 | parameters = (volatile PARAMETERS *) (LOCALDATA_BASE + i);             
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |909| 
           MVKH    .S2     0x2000000,B4      ; |909| 
           ADD     .L2X    B4,A0,B4          ; 
           STW     .D2T2   B4,*+SP(168)      ; |909| 
	.line	189
;------------------------------------------------------------------------------
; 910 | i += sizeof( PARAMETERS );                                             
;------------------------------------------------------------------------------
           ADDK    .S1     660,A0            ; |910| 
           STW     .D2T1   A0,*+SP(84)       ; |910| 
	.line	193
;------------------------------------------------------------------------------
; 914 | command = status = XYCORE_READY;                                     //
;     |  initialize CmdStat register: Detroit and                              
; 915 | #if defined(_TGTDAKARF5)                                             //
;     |  Dakar use PCI runtime mailbox reg#2 for                               
; 916 | C4X_Control( PCI, SET_MAILBOX, 2, &status );                      // th
;     | is, while Daytona (NodeA) uses the first                               
; 917 | #elif defined(_TGTDETROIT)                                           //
;     |  32bit word in its SSRAM...                                            
;------------------------------------------------------------------------------
           MVK     .S2     0x1,B4            ; |914| 
           STW     .D2T2   B4,*+SP(52)       ; |914| 
           STW     .D2T2   B4,*+SP(48)       ; |914| 
	.line	197
;------------------------------------------------------------------------------
; 918 | C6x_WritePlx( MAILBOX2_OFFSET, status );                          // NO
;     | TE: We set "ready" status here so CXDRIVER                             
; 919 | #else    // _TGTDAYTONA                                              //
;     |  does not have to wait while we init the trig                          
; 920 | *((UINT32 *)0x00400000) = status;                                 // ta
;     | bles, which takes a while!                                             
; 921 | #endif                                                                 
;------------------------------------------------------------------------------
           MVKL    .S2     0x16000c8,B5      ; |918| 
           MVKH    .S2     0x16000c8,B5      ; |918| 
           STW     .D2T2   B4,*B5            ; |918| 
	.line	203
;------------------------------------------------------------------------------
; 924 | for( i = 0; i < 4500; i++ )                                          //
;     |  initialize all lookup tables...                                       
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |924| 
           STW     .D2T2   B4,*+SP(84)       ; |924| 
           MVK     .S1     4500,A0           ; |924| 
           CMPLTU  .L1X    B4,A0,A1          ; 
   [!A1]   B       .S1     L2                ; |924| 
           NOP             5
           ; BRANCH OCCURS                   ; |924| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L1:    
	.line	205
;------------------------------------------------------------------------------
; 926 | dDummy = ((double) i) * 0.00017453293;                            //   
;     |  convert integer deg/100 to radians                                    
;------------------------------------------------------------------------------
           B       .S1     __fltud           ; |926| 
           LDW     .D2T1   *+SP(84),A4       ; |926| 
           MVKL    .S2     RL94,B3           ; |926| 
           MVKH    .S2     RL94,B3           ; |926| 
           NOP             2
RL94:      ; CALL OCCURS                     ; |926| 

           B       .S1     __mpyd            ; |926| 
||         MVKL    .S2     0x3f26e05a,B5     ; |926| 

           MVKL    .S2     0x73edfc30,B4     ; |926| 
           MVKH    .S2     0x3f26e05a,B5     ; |926| 
           MVKL    .S2     RL96,B3           ; |926| 
           MVKH    .S2     0x73edfc30,B4     ; |926| 
           MVKH    .S2     RL96,B3           ; |926| 
RL96:      ; CALL OCCURS                     ; |926| 
           STW     .D2T1   A4,*+SP(120)      ; |926| 
           STW     .D2T1   A5,*+SP(124)      ; |926| 
	.line	206
;------------------------------------------------------------------------------
; 927 | tanLUT[i] = (INT16) floor( 1024.0 * tan( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------
           B       .S1     _tan              ; |927| 
           MVKL    .S2     RL116,B3          ; |927| 
           MVKH    .S2     RL116,B3          ; |927| 
           NOP             3
RL116:     ; CALL OCCURS                     ; |927| 
           B       .S1     __mpyd            ; |927| 
           MVKL    .S2     RL118,B3          ; |927| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |927| 
           MVKH    .S1     0x40900000,A5     ; |927| 

           MVKH    .S2     RL118,B3          ; |927| 
||         MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |927| 

RL118:     ; CALL OCCURS                     ; |927| 
           B       .S1     __addd            ; |927| 
           MVKL    .S2     RL120,B3          ; |927| 
           ZERO    .L2     B5                ; |927| 
           MVKH    .S2     0x3fe00000,B5     ; |927| 
           MVKH    .S2     RL120,B3          ; |927| 
           ZERO    .L2     B4                ; |927| 
RL120:     ; CALL OCCURS                     ; |927| 
           B       .S1     _floor            ; |927| 
           MVKL    .S2     RL122,B3          ; |927| 
           MVKH    .S2     RL122,B3          ; |927| 
           NOP             3
RL122:     ; CALL OCCURS                     ; |927| 
           B       .S1     __fixdi           ; |927| 
           MVKL    .S2     RL124,B3          ; |927| 
           MVKH    .S2     RL124,B3          ; |927| 
           NOP             3
RL124:     ; CALL OCCURS                     ; |927| 
           LDW     .D2T2   *+SP(84),B4       ; |927| 
           LDW     .D2T2   *+SP(204),B5      ; |927| 
           EXT     .S1     A4,16,16,A0       ; |927| 
           NOP             3
           STH     .D2T1   A0,*+B5[B4]       ; |927| 
	.line	207
;------------------------------------------------------------------------------
; 928 | sincosLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) * cos( dDummy ) + 
;     | 0.5 );                                                                 
;------------------------------------------------------------------------------

           B       .S1     _sin              ; |928| 
||         LDW     .D2T1   *+SP(124),A5      ; |928| 

           LDW     .D2T1   *+SP(120),A4      ; |928| 
           MVKL    .S2     RL168,B3          ; |928| 
           MVKH    .S2     RL168,B3          ; |928| 
           NOP             2
RL168:     ; CALL OCCURS                     ; |928| 
           B       .S1     __mpyd            ; |928| 
           MV      .L2X    A5,B5             ; 
           MV      .L2X    A4,B4             ; 
           MVKL    .S2     RL170,B3          ; |928| 
           ZERO    .L1     A5                ; |928| 

           MVKH    .S2     RL170,B3          ; |928| 
||         MVKH    .S1     0x40900000,A5     ; |928| 
||         ZERO    .L1     A4                ; |928| 

RL170:     ; CALL OCCURS                     ; |928| 

           B       .S1     _cos              ; |928| 
||         MV      .L1     A5,A10            ; 
||         LDW     .D2T1   *+SP(124),A5      ; |928| 

           MV      .L1     A4,A11            ; 
||         LDW     .D2T1   *+SP(120),A4      ; |928| 

           MVKL    .S2     RL172,B3          ; |928| 
           MVKH    .S2     RL172,B3          ; |928| 
           NOP             2
RL172:     ; CALL OCCURS                     ; |928| 
           B       .S1     __mpyd            ; |928| 
           MV      .L2X    A4,B4             ; 
           MV      .L2X    A5,B5             ; 
           MVKL    .S2     RL174,B3          ; |928| 
           MVKH    .S2     RL174,B3          ; |928| 

           MV      .L1     A10,A5            ; |928| 
||         MV      .S1     A11,A4            ; |928| 

RL174:     ; CALL OCCURS                     ; |928| 
;** --------------------------------------------------------------------------*
           B       .S1     __addd            ; |928| 
           MVKL    .S2     RL176,B3          ; |928| 
           MVKH    .S2     RL176,B3          ; |928| 
           ZERO    .L2     B5                ; |928| 
           MVKH    .S2     0x3fe00000,B5     ; |928| 
           ZERO    .L2     B4                ; |928| 
RL176:     ; CALL OCCURS                     ; |928| 
           B       .S1     _floor            ; |928| 
           MVKL    .S2     RL178,B3          ; |928| 
           MVKH    .S2     RL178,B3          ; |928| 
           NOP             3
RL178:     ; CALL OCCURS                     ; |928| 
           B       .S1     __fixdi           ; |928| 
           MVKL    .S2     RL180,B3          ; |928| 
           MVKH    .S2     RL180,B3          ; |928| 
           NOP             3
RL180:     ; CALL OCCURS                     ; |928| 
           LDW     .D2T2   *+SP(84),B4       ; |928| 
           LDW     .D2T2   *+SP(208),B5      ; |928| 
           EXT     .S1     A4,16,16,A0       ; |928| 
           NOP             3
           STH     .D2T1   A0,*+B5[B4]       ; |928| 
	.line	208
           LDW     .D2T2   *+SP(84),B4       ; |929| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |929| 
           STW     .D2T2   B4,*+SP(84)       ; |929| 
           MVK     .S2     4500,B5           ; |929| 
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L1                ; |929| 
           NOP             5
           ; BRANCH OCCURS                   ; |929| 
;** --------------------------------------------------------------------------*
L2:    
	.line	209
;------------------------------------------------------------------------------
; 930 | for( i = 0; i < 3600; i++ )                                            
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |930| 
           STW     .D2T1   A0,*+SP(84)       ; |930| 
           MVK     .S1     3600,A3           ; |930| 
           CMPLTU  .L1     A0,A3,A1          ; 
   [!A1]   B       .S1     L4                ; |930| 
           NOP             5
           ; BRANCH OCCURS                   ; |930| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L3:    
	.line	211
;------------------------------------------------------------------------------
; 932 | dDummy = ((double) i) * 0.0017453293;                             //   
;     |  convert integer deg/10 to radians                                     
;------------------------------------------------------------------------------
           B       .S1     __fltud           ; |932| 
           LDW     .D2T1   *+SP(84),A4       ; |932| 
           MVKL    .S2     RL184,B3          ; |932| 
           MVKH    .S2     RL184,B3          ; |932| 
           NOP             2
RL184:     ; CALL OCCURS                     ; |932| 

           B       .S1     __mpyd            ; |932| 
||         MVKL    .S2     0x3f5c9871,B5     ; |932| 

           MVKL    .S2     0x10e97b3c,B4     ; |932| 
           MVKH    .S2     0x3f5c9871,B5     ; |932| 
           MVKL    .S2     RL186,B3          ; |932| 
           MVKH    .S2     0x10e97b3c,B4     ; |932| 
           MVKH    .S2     RL186,B3          ; |932| 
RL186:     ; CALL OCCURS                     ; |932| 
           STW     .D2T1   A4,*+SP(120)      ; |932| 
           STW     .D2T1   A5,*+SP(124)      ; |932| 
	.line	212
;------------------------------------------------------------------------------
; 933 | sinLUT[i] = (INT16) floor( 1024.0 * sin( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------
           B       .S1     _sin              ; |933| 
           MVKL    .S2     RL206,B3          ; |933| 
           MVKH    .S2     RL206,B3          ; |933| 
           NOP             3
RL206:     ; CALL OCCURS                     ; |933| 
           B       .S1     __mpyd            ; |933| 
           MVKL    .S2     RL208,B3          ; |933| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |933| 
           MVKH    .S1     0x40900000,A5     ; |933| 

           MVKH    .S2     RL208,B3          ; |933| 
||         MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |933| 

RL208:     ; CALL OCCURS                     ; |933| 
           B       .S1     __addd            ; |933| 
           MVKL    .S2     RL210,B3          ; |933| 
           ZERO    .L2     B5                ; |933| 
           MVKH    .S2     0x3fe00000,B5     ; |933| 
           MVKH    .S2     RL210,B3          ; |933| 
           ZERO    .L2     B4                ; |933| 
RL210:     ; CALL OCCURS                     ; |933| 
           B       .S1     _floor            ; |933| 
           MVKL    .S2     RL212,B3          ; |933| 
           MVKH    .S2     RL212,B3          ; |933| 
           NOP             3
RL212:     ; CALL OCCURS                     ; |933| 
           B       .S1     __fixdi           ; |933| 
           MVKL    .S2     RL214,B3          ; |933| 
           MVKH    .S2     RL214,B3          ; |933| 
           NOP             3
RL214:     ; CALL OCCURS                     ; |933| 
           LDW     .D2T2   *+SP(84),B4       ; |933| 
           LDW     .D2T2   *+SP(212),B5      ; |933| 
           EXT     .S1     A4,16,16,A0       ; |933| 
           NOP             3
           STH     .D2T1   A0,*+B5[B4]       ; |933| 
	.line	213
;------------------------------------------------------------------------------
; 934 | cosLUT[i] = (INT16) floor( 1024.0 * cos( dDummy ) + 0.5 );             
;------------------------------------------------------------------------------

           B       .S1     _cos              ; |934| 
||         LDW     .D2T1   *+SP(124),A5      ; |934| 

           LDW     .D2T1   *+SP(120),A4      ; |934| 
           MVKL    .S2     RL234,B3          ; |934| 
           MVKH    .S2     RL234,B3          ; |934| 
           NOP             2
RL234:     ; CALL OCCURS                     ; |934| 
           B       .S1     __mpyd            ; |934| 
           MV      .L2X    A5,B5             ; 
           MV      .L2X    A4,B4             ; 
           MVKL    .S2     RL236,B3          ; |934| 
           ZERO    .L1     A5                ; |934| 

           MVKH    .S2     RL236,B3          ; |934| 
||         MVKH    .S1     0x40900000,A5     ; |934| 
||         ZERO    .L1     A4                ; |934| 

RL236:     ; CALL OCCURS                     ; |934| 
           B       .S1     __addd            ; |934| 
           MVKL    .S2     RL238,B3          ; |934| 
           ZERO    .L2     B5                ; |934| 
           MVKH    .S2     RL238,B3          ; |934| 
           MVKH    .S2     0x3fe00000,B5     ; |934| 
           ZERO    .L2     B4                ; |934| 
RL238:     ; CALL OCCURS                     ; |934| 
           B       .S1     _floor            ; |934| 
           MVKL    .S2     RL240,B3          ; |934| 
           MVKH    .S2     RL240,B3          ; |934| 
           NOP             3
RL240:     ; CALL OCCURS                     ; |934| 
;** --------------------------------------------------------------------------*
           B       .S1     __fixdi           ; |934| 
           MVKL    .S2     RL242,B3          ; |934| 
           MVKH    .S2     RL242,B3          ; |934| 
           NOP             3
RL242:     ; CALL OCCURS                     ; |934| 
           LDW     .D2T2   *+SP(84),B4       ; |934| 
           LDW     .D2T2   *+SP(216),B5      ; |934| 
           EXT     .S1     A4,16,16,A0       ; |934| 
           NOP             3
           STH     .D2T1   A0,*+B5[B4]       ; |934| 
	.line	214
           LDW     .D2T2   *+SP(84),B4       ; |935| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |935| 
           STW     .D2T2   B4,*+SP(84)       ; |935| 
           MVK     .S2     3600,B5           ; |935| 
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L3                ; |935| 
           NOP             5
           ; BRANCH OCCURS                   ; |935| 
;** --------------------------------------------------------------------------*
L4:    
	.line	216
;------------------------------------------------------------------------------
; 937 | for( i = 0; i < 281; i++ )                                             
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |937| 
           STW     .D2T1   A0,*+SP(84)       ; |937| 
           MVK     .S1     281,A3            ; |937| 
           CMPLTU  .L1     A0,A3,A1          ; 
   [!A1]   B       .S1     L6                ; |937| 
           MVK     .S2     281,B10           ; |941| 
           NOP             4
           ; BRANCH OCCURS                   ; |937| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L5:    
	.line	218
;------------------------------------------------------------------------------
; 939 | dDummy = (((double) i) - 140) / 20.0;                                  
;------------------------------------------------------------------------------
           B       .S1     __fltud           ; |939| 
           LDW     .D2T1   *+SP(84),A4       ; |939| 
           MVKL    .S2     RL252,B3          ; |939| 
           MVKH    .S2     RL252,B3          ; |939| 
           NOP             2
RL252:     ; CALL OCCURS                     ; |939| 
           B       .S1     __subd            ; |939| 
           MVKL    .S2     0x40618000,B5     ; |939| 
           MVKL    .S2     RL254,B3          ; |939| 
           MVKH    .S2     0x40618000,B5     ; |939| 
           MVKH    .S2     RL254,B3          ; |939| 
           ZERO    .L2     B4                ; |939| 
RL254:     ; CALL OCCURS                     ; |939| 
           B       .S1     __divd            ; |939| 
           MVKL    .S2     RL256,B3          ; |939| 
           ZERO    .L2     B5                ; |939| 
           MVKH    .S2     0x40340000,B5     ; |939| 
           MVKH    .S2     RL256,B3          ; |939| 
           ZERO    .L2     B4                ; |939| 
RL256:     ; CALL OCCURS                     ; |939| 
           STW     .D2T1   A4,*+SP(120)      ; |939| 
           STW     .D2T1   A5,*+SP(124)      ; |939| 
	.line	219
;------------------------------------------------------------------------------
; 940 | pow2LUT[i] = (INT32) floor(pow(2.0, dDummy + 20.0) + 0.5);             
;------------------------------------------------------------------------------
           B       .S1     __addd            ; |940| 
           MVKL    .S2     RL274,B3          ; |940| 
           ZERO    .L2     B5                ; |940| 
           MVKH    .S2     0x40340000,B5     ; |940| 
           MVKH    .S2     RL274,B3          ; |940| 
           ZERO    .L2     B4                ; |940| 
RL274:     ; CALL OCCURS                     ; |940| 
           B       .S1     _pow              ; |940| 
           MV      .L2X    A5,B5             ; 
           MVKL    .S2     RL276,B3          ; |940| 
           ZERO    .L1     A5                ; |940| 
           MVKH    .S1     0x40000000,A5     ; |940| 

           MV      .L2X    A4,B4             ; 
||         MVKH    .S2     RL276,B3          ; |940| 
||         ZERO    .L1     A4                ; |940| 

RL276:     ; CALL OCCURS                     ; |940| 
           B       .S1     __addd            ; |940| 
           ZERO    .L2     B5                ; |940| 
           MVKH    .S2     0x3fe00000,B5     ; |940| 
           MVKL    .S2     RL278,B3          ; |940| 
           MVKH    .S2     RL278,B3          ; |940| 
           ZERO    .L2     B4                ; |940| 
RL278:     ; CALL OCCURS                     ; |940| 
           B       .S1     _floor            ; |940| 
           MVKL    .S2     RL280,B3          ; |940| 
           MVKH    .S2     RL280,B3          ; |940| 
           NOP             3
RL280:     ; CALL OCCURS                     ; |940| 
           B       .S1     __fixdi           ; |940| 
           MVKL    .S2     RL282,B3          ; |940| 
           MVKH    .S2     RL282,B3          ; |940| 
           NOP             3
RL282:     ; CALL OCCURS                     ; |940| 
           LDW     .D2T2   *+SP(84),B4       ; |940| 
           LDW     .D2T2   *+SP(220),B5      ; |940| 
           NOP             4
           STW     .D2T1   A4,*+B5[B4]       ; |940| 
	.line	220
           LDW     .D2T2   *+SP(84),B4       ; |941| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |941| 
           STW     .D2T2   B4,*+SP(84)       ; |941| 
           CMPLTU  .L2     B4,B10,B0         ; 
   [ B0]   B       .S1     L5                ; |941| 
           NOP             5
           ; BRANCH OCCURS                   ; |941| 
;** --------------------------------------------------------------------------*
L6:    
	.line	221
;------------------------------------------------------------------------------
; 942 | for( i = 0; i < 7; i++ )                                               
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |942| 
           STW     .D2T1   A0,*+SP(84)       ; |942| 
           CMPLTU  .L1     A0,7,A1           ; 
   [!A1]   B       .S1     L8                ; |942| 
           NOP             5
           ; BRANCH OCCURS                   ; |942| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L7:    
	.line	223
;------------------------------------------------------------------------------
; 944 | j = i+1;                                                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(84),B4       ; |944| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |944| 
           STW     .D2T2   B4,*+SP(88)       ; |944| 
	.line	224
;------------------------------------------------------------------------------
; 945 | dDummy = 1024.0 * (pow(2.0, (double)j) - pow(2.0, -((double)j)));      
;------------------------------------------------------------------------------
           B       .S1     __fltud           ; |945| 
           MVKL    .S2     RL316,B3          ; |945| 
           MV      .L1X    B4,A4
           MVKH    .S2     RL316,B3          ; |945| 
           NOP             2
RL316:     ; CALL OCCURS                     ; |945| 
           B       .S1     _pow              ; |945| 
           MVKL    .S2     RL318,B3          ; |945| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |945| 
           MVKH    .S1     0x40000000,A5     ; |945| 

           MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |945| 
||         MVKH    .S2     RL318,B3          ; |945| 

RL318:     ; CALL OCCURS                     ; |945| 
           B       .S1     __fltud           ; |945| 

           MV      .L1     A4,A11            ; 
||         LDW     .D2T1   *+SP(88),A4       ; |945| 

           MVKL    .S2     RL320,B3          ; |945| 
           MVKH    .S2     RL320,B3          ; |945| 
           MV      .L1     A5,A10            ; 
           NOP             1
RL320:     ; CALL OCCURS                     ; |945| 
           B       .S1     __negd            ; |945| 
           MVKL    .S2     RL322,B3          ; |945| 
           MVKH    .S2     RL322,B3          ; |945| 
           NOP             3
RL322:     ; CALL OCCURS                     ; |945| 
           B       .S1     _pow              ; |945| 
           MVKL    .S2     RL324,B3          ; |945| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |945| 
           MVKH    .S2     RL324,B3          ; |945| 

           MVKH    .S1     0x40000000,A5     ; |945| 
||         MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |945| 

RL324:     ; CALL OCCURS                     ; |945| 
           B       .S1     __subd            ; |945| 
           MV      .L2X    A5,B5             ; 
           MV      .L2X    A4,B4             ; 
           MVKL    .S2     RL326,B3          ; |945| 
           MVKH    .S2     RL326,B3          ; |945| 

           MV      .L1     A11,A4            ; |945| 
||         MV      .S1     A10,A5            ; |945| 

RL326:     ; CALL OCCURS                     ; |945| 
           B       .S1     __mpyd            ; |945| 
           MVKL    .S2     RL328,B3          ; |945| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |945| 
           MVKH    .S2     RL328,B3          ; |945| 

           MVKH    .S1     0x40900000,A5     ; |945| 
||         MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |945| 

RL328:     ; CALL OCCURS                     ; |945| 
           STW     .D2T1   A4,*+SP(120)      ; |945| 
           STW     .D2T1   A5,*+SP(124)      ; |945| 
	.line	225
;------------------------------------------------------------------------------
; 946 | dDummy /= (2.0 * ((double)j) * log(2.0));                              
;------------------------------------------------------------------------------
           B       .S1     __fltud           ; |946| 
           LDW     .D2T1   *+SP(88),A4       ; |946| 
           MVKL    .S2     RL354,B3          ; |946| 
           MVKH    .S2     RL354,B3          ; |946| 
           NOP             2
RL354:     ; CALL OCCURS                     ; |946| 
           B       .S1     __mpyd            ; |946| 
           MVKL    .S2     RL356,B3          ; |946| 
           MV      .L2X    A5,B5             ; 
           ZERO    .L1     A5                ; |946| 
           MVKH    .S1     0x40000000,A5     ; |946| 

           MVKH    .S2     RL356,B3          ; |946| 
||         MV      .L2X    A4,B4             ; 
||         ZERO    .L1     A4                ; |946| 

RL356:     ; CALL OCCURS                     ; |946| 
           B       .S1     _log              ; |946| 
           MV      .L1     A5,A11            ; 
           MVKL    .S2     RL358,B3          ; |946| 
           MV      .L1     A4,A10            ; 
           ZERO    .S1     A5                ; |946| 

           MVKH    .S2     RL358,B3          ; |946| 
||         MVKH    .S1     0x40000000,A5     ; |946| 
||         ZERO    .L1     A4                ; |946| 

RL358:     ; CALL OCCURS                     ; |946| 
           B       .S1     __mpyd            ; |946| 
           MV      .L2X    A5,B5             ; 
           MVKL    .S2     RL360,B3          ; |946| 
           MV      .L2X    A4,B4             ; 
           MVKH    .S2     RL360,B3          ; |946| 

           MV      .L1     A10,A4            ; |946| 
||         MV      .S1     A11,A5            ; |946| 

RL360:     ; CALL OCCURS                     ; |946| 
;** --------------------------------------------------------------------------*

           B       .S1     __divd            ; |946| 
||         LDW     .D2T1   *+SP(124),A5      ; 
||         MV      .L2X    A5,B5             ; 

           LDW     .D2T1   *+SP(120),A4      ; 
||         MV      .L2X    A4,B4             ; 

           MVKL    .S2     RL362,B3          ; |946| 
           MVKH    .S2     RL362,B3          ; |946| 
           NOP             2
RL362:     ; CALL OCCURS                     ; |946| 
           STW     .D2T1   A4,*+SP(120)      ; |946| 
           STW     .D2T1   A5,*+SP(124)      ; |946| 
	.line	226
;------------------------------------------------------------------------------
; 947 | speedNoiseAdj[i] = (INT32) floor(dDummy + 0.5);                        
;------------------------------------------------------------------------------
           B       .S1     __addd            ; |947| 
           MVKL    .S2     RL366,B3          ; |947| 
           ZERO    .L2     B5                ; |947| 
           MVKH    .S2     0x3fe00000,B5     ; |947| 
           MVKH    .S2     RL366,B3          ; |947| 
           ZERO    .L2     B4                ; |947| 
RL366:     ; CALL OCCURS                     ; |947| 
           B       .S1     _floor            ; |947| 
           MVKL    .S2     RL368,B3          ; |947| 
           MVKH    .S2     RL368,B3          ; |947| 
           NOP             3
RL368:     ; CALL OCCURS                     ; |947| 
           B       .S1     __fixdi           ; |947| 
           MVKL    .S2     RL370,B3          ; |947| 
           MVKH    .S2     RL370,B3          ; |947| 
           NOP             3
RL370:     ; CALL OCCURS                     ; |947| 
           LDW     .D2T2   *+SP(224),B4      ; |947| 
           LDW     .D2T2   *+SP(84),B5       ; |947| 
           NOP             4
           STW     .D2T1   A4,*+B4[B5]       ; |947| 
	.line	227
;------------------------------------------------------------------------------
; 958 | do                                                                   //
;     |  BEGIN: runtime loop -- process cmds from                              
; 959 | {                                                                    //
;     |  host XYAPI until XYCORE_CLOSE is received.                            
; 960 | do                                                                // wa
;     | it for next command in CmdStat reg                                     
; 962 | #if defined(_TGTDAKARF5)                                               
; 963 | C4X_Control( PCI, GET_MAILBOX, 2, &command );                          
; 964 | #elif defined(_TGTDETROIT)                                             
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(84),B4       ; |948| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |948| 
           STW     .D2T2   B4,*+SP(84)       ; |948| 
           CMPLTU  .L2     B4,7,B0           ; 
   [ B0]   B       .S1     L7                ; |948| 
           NOP             5
           ; BRANCH OCCURS                   ; |948| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L8
;** --------------------------------------------------------------------------*
L8:    
	.line	244
;------------------------------------------------------------------------------
; 965 | C6x_ReadPlx( MAILBOX2_OFFSET, &command );                              
; 966 | #else    // _TGTDAYTONA                                                
; 967 | command = *((UINT32 *)0x00400000);                                     
; 982 | #endif                                                                 
;------------------------------------------------------------------------------
           MVKL    .S1     0x16000c8,A0      ; |965| 
           MVKH    .S1     0x16000c8,A0      ; |965| 
           LDW     .D1T2   *A0,B4            ; |965| 
           NOP             4
           STW     .D2T2   B4,*+SP(48)       ; |965| 
	.line	262
;------------------------------------------------------------------------------
; 983 | } while( command == XYCORE_READY );                                    
;------------------------------------------------------------------------------
           CMPEQ   .L2     B4,1,B0           ; 
   [ B0]   B       .S1     L8                ; |983| 
           NOP             5
           ; BRANCH OCCURS                   ; |983| 
;** --------------------------------------------------------------------------*
	.line	265
;------------------------------------------------------------------------------
; 986 | if( command == XYCORE_INIT )                                      // BE
;     | GIN:  process XYCORE_INIT command                                      
;------------------------------------------------------------------------------
           CMPEQ   .L2     B4,2,B0           ; 
   [!B0]   B       .S1     L37               ; |986| 
           NOP             5
           ; BRANCH OCCURS                   ; |986| 
;** --------------------------------------------------------------------------*
	.line	267
;------------------------------------------------------------------------------
; 988 | memcpy( (void*) parameters, (void*) SharedArray,               // copy 
;     | params into local mem for faster access                                
; 989 | sizeof(PARAMETERS) );                                                  
;------------------------------------------------------------------------------

           B       .S1     _memcpy           ; |988| 
||         LDW     .D2T2   *+SP(196),B4      ; 

           LDW     .D2T1   *+SP(168),A4      ; 
           MVKL    .S2     RL372,B3          ; |988| 
           MVKH    .S2     RL372,B3          ; |988| 
           MVK     .S1     0x294,A6          ; |988| 
           NOP             1
RL372:     ; CALL OCCURS                     ; |988| 
	.line	270
;------------------------------------------------------------------------------
; 991 | SetSeed( parameters->dwDotSeed );                              // seed 
;     | both random# generators using the seed                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |991| 
           MVKL    .S2     RL374,B3          ; |991| 
           MVKH    .S2     RL374,B3          ; |991| 
           NOP             1
           B       .S1     _SetSeed          ; |991| 
           LDW     .D2T1   *B4,A4            ; |991| 
           NOP             4
RL374:     ; CALL OCCURS                     ; |991| 
	.line	271
;------------------------------------------------------------------------------
; 992 | SetSeed2( parameters->dwDotSeed );                             // value
;     |  provided                                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |992| 
           MVKL    .S2     RL376,B3          ; |992| 
           MVKH    .S2     RL376,B3          ; |992| 
           NOP             1
           B       .S1     _SetSeed2         ; |992| 
           LDW     .D2T1   *B4,A4            ; |992| 
           NOP             4
RL376:     ; CALL OCCURS                     ; |992| 
	.line	273
;------------------------------------------------------------------------------
; 994 | dotPosOffset = 0;                                              // prote
;     | ct against overflow of dot pos arrays:                                 
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |994| 
           STW     .D2T2   B4,*+SP(112)      ; |994| 
	.line	274
;------------------------------------------------------------------------------
; 995 | for( d = 0; d < parameters->wNumTargets; d++ )                 // if ne
;     | cessary, reduce #targets processed so                                  
; 996 | {                                                              // that 
;     | total #dots to be stored falls under                                   
;------------------------------------------------------------------------------
           STW     .D2T2   B4,*+SP(104)      ; |995| 
           LDW     .D2T2   *+SP(168),B5      ; |995| 
           NOP             4
           LDHU    .D2T2   *+B5(10),B5       ; |995| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L10               ; |995| 
           NOP             5
           ; BRANCH OCCURS                   ; |995| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L9:    
	.line	276
;------------------------------------------------------------------------------
; 997 | if( dotPosOffset + parameters->wNumDots[d] > MAXTOTALDOTS ) // the maxi
;     | mum allowed limit...                                                   
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A4      ; |997| 
           LDW     .D2T1   *+SP(168),A0      ; |997| 
           MVK     .S1     30000,A6          ; |997| 
           NOP             3
           ADDAH   .D1     A0,A4,A3          ; |997| 
           ADDK    .S1     84,A3             ; |997| 
           LDHU    .D1T1   *A3,A5            ; |997| 
           LDW     .D2T1   *+SP(112),A3      ; |997| 
           NOP             4
           ADD     .L1     A5,A3,A5          ; |997| 
           CMPGTU  .L1     A5,A6,A1          ; |997| 
   [ A1]   B       .S1     L10               ; |997| 
           NOP             5
           ; BRANCH OCCURS                   ; |997| 
;** --------------------------------------------------------------------------*
	.line	277
;------------------------------------------------------------------------------
; 998 | break;                                                                 
;------------------------------------------------------------------------------
	.line	278
;------------------------------------------------------------------------------
; 999 | dotPosOffset += parameters->wNumDots[d];                               
;------------------------------------------------------------------------------
           ADDAH   .D1     A0,A4,A4          ; |999| 
           ADDK    .S1     84,A4             ; |999| 
           LDHU    .D1T1   *A4,A4            ; |999| 
           NOP             4
           ADD     .L1     A4,A3,A3          ; 
           STW     .D2T1   A3,*+SP(112)      ; |999| 
	.line	279
           LDW     .D2T2   *+SP(104),B4      ; |1000| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1000| 
           STW     .D2T2   B5,*+SP(104)      ; |1000| 
           MV      .L2X    A0,B4
           LDHU    .D2T2   *+B4(10),B4       ; |1000| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L9                ; |1000| 
           NOP             5
           ; BRANCH OCCURS                   ; |1000| 
;** --------------------------------------------------------------------------*
L10:    
	.line	280
;------------------------------------------------------------------------------
; 1001 | parameters->wNumTargets = d;                                           
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(104),B4      ; |1001| 
           LDW     .D2T1   *+SP(168),A0      ; |1001| 
           NOP             4
           STH     .D1T2   B4,*+A0(10)       ; |1001| 
	.line	283
;------------------------------------------------------------------------------
; 1004 | dotPosOffset = 0;                                              // gener
;     | ate & store the initial (x,y)-coords of                                
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1004| 
           STW     .D2T2   B4,*+SP(112)      ; |1004| 
	.line	284
;------------------------------------------------------------------------------
; 1005 | for( d = 0; d < parameters->wNumTargets; d++ )                 // dots 
;     | for all tgts in PARAMETERS struct.                                     
;------------------------------------------------------------------------------
           STW     .D2T2   B4,*+SP(104)      ; |1005| 
           LDW     .D2T2   *+SP(168),B5      ; |1005| 
           NOP             4
           LDHU    .D2T2   *+B5(10),B5       ; |1005| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L144              ; |1005| 
           NOP             5
           ; BRANCH OCCURS                   ; |1005| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L11
;** --------------------------------------------------------------------------*
L11:    
	.line	286
;------------------------------------------------------------------------------
; 1007 | u16Type = parameters->wType[d];                             // this tar
;     | get's type                                                             
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A3      ; |1007| 
           LDW     .D2T1   *+SP(168),A0      ; |1007| 
           NOP             4
           ADDAH   .D1     A0,A3,A4          ; |1007| 
           LDHU    .D1T1   *+A4(20),A4       ; |1007| 
           NOP             4
           STH     .D2T1   A4,*+SP(36)       ; |1007| 
	.line	288
;------------------------------------------------------------------------------
; 1009 | if( u16Type == NO_TARGET )                                  // NO_TARGE
;     | T: not a target; nothing to do here.                                   
; 1010 | ;                                                                      
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B0       ; |1009| 
           NOP             4
   [!B0]   B       .S1     L36               ; |1009| 
           NOP             5
           ; BRANCH OCCURS                   ; |1009| 
;** --------------------------------------------------------------------------*
	.line	291
;------------------------------------------------------------------------------
; 1012 | else if ( (u16Type == DOTARRAY) &&                          // DOTARRAY
;     | : Nonrandom, rect array of regularly                                   
; 1013 | (parameters->wNumDots[d] > 0) )                   // spaced dots.      
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1012| 
           NOP             4
           CMPEQ   .L2     B4,1,B0           ; |1012| 
   [ B0]   LDW     .D2T2   *+SP(104),B5      ; |1012| 
   [ B0]   LDW     .D2T2   *+SP(168),B4      ; |1012| 
           NOP             4
   [ B0]   ADDAH   .D2     B4,B5,B4          ; |1012| 
   [ B0]   ADDK    .S2     84,B4             ; |1012| 
   [ B0]   LDHU    .D2T2   *B4,B4            ; |1012| 
           NOP             4
   [ B0]   CMPGT   .L2     B4,0,B0           ; |1012| 
   [!B0]   B       .S1     L16               ; |1012| 
           NOP             5
           ; BRANCH OCCURS                   ; |1012| 
;** --------------------------------------------------------------------------*
	.line	294
;------------------------------------------------------------------------------
; 1015 | if( parameters->wRectR[d] > 32768 )                      //    width of
;     |  array.  enforce maximum value.                                        
;------------------------------------------------------------------------------
           ADDAH   .D1     A0,A3,A0          ; |1015| 
           ADDK    .S1     148,A0            ; |1015| 
           LDHU    .D1T1   *A0,A0            ; |1015| 
           ZERO    .L2     B4                ; |1015| 
           SET     .S2     B4,0xf,0xf,B4     ; |1015| 
           NOP             2
           CMPGT   .L1X    A0,B4,A1          ; |1015| 
	.line	295
;------------------------------------------------------------------------------
; 1016 | parameters->wRectR[d] = 32768;                                         
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(168),A3      ; |1016| 
   [ A1]   LDW     .D2T1   *+SP(104),A0      ; |1016| 
   [ A1]   ZERO    .L2     B4                ; |1016| 
   [ A1]   SET     .S2     B4,0xf,0xf,B4     ; |1016| 
           NOP             2
   [ A1]   ADDAH   .D1     A3,A0,A0          ; |1016| 
   [ A1]   ADDK    .S1     148,A0            ; |1016| 
   [ A1]   STH     .D1T2   B4,*A0            ; |1016| 
	.line	296
;------------------------------------------------------------------------------
; 1017 | if( parameters->wRectL[d] > 32768 )                      //    dot spac
;     | ing.  enforce maximum value.                                           
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A3      ; |1017| 
           LDW     .D2T1   *+SP(104),A0      ; |1017| 
           NOP             4
           ADDAH   .D1     A3,A0,A0          ; |1017| 
           ADDK    .S1     212,A0            ; |1017| 
           LDHU    .D1T1   *A0,A3            ; |1017| 
           ZERO    .L1     A0                ; |1017| 
           SET     .S1     A0,0xf,0xf,A0     ; |1017| 
           NOP             2
           CMPGT   .L1     A3,A0,A1          ; |1017| 
	.line	297
;------------------------------------------------------------------------------
; 1018 | parameters->wRectL[d] = 32768;                                         
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(104),A3      ; |1018| 
   [ A1]   LDW     .D2T1   *+SP(168),A4      ; |1018| 
   [ A1]   ZERO    .L1     A0                ; |1018| 
           NOP             3

   [ A1]   ADDAH   .D1     A4,A3,A0          ; |1018| 
|| [ A1]   SET     .S1     A0,0xf,0xf,A3     ; |1018| 

   [ A1]   ADDK    .S1     212,A0            ; |1018| 
   [ A1]   STH     .D1T1   A3,*A0            ; |1018| 
	.line	299
;------------------------------------------------------------------------------
; 1020 | cd = parameters->wRectR[d] / 2;                          //    draw arr
;     | ay from L->R, B->T starting w/dot                                      
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1020| 
           LDW     .D2T2   *+SP(104),B4      ; |1020| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1020| 
           ADDK    .S2     148,B4            ; |1020| 
           LDHU    .D2T2   *B4,B4            ; |1020| 
           NOP             4
           SHRU    .S2     B4,31,B5          ; |1020| 
           ADD     .L2     B5,B4,B4          ; |1020| 
           SHR     .S2     B4,1,B4           ; |1020| 
           STW     .D2T2   B4,*+SP(108)      ; |1020| 
	.line	300
;------------------------------------------------------------------------------
; 1021 | xdotpos[dotPosOffset] = CTR_PIX - cd;                    //    at lower
;     |  left corner. init pos of this                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B6      ; |1021| 
           LDW     .D2T2   *+SP(128),B7      ; |1021| 
           MVK     .S2     32767,B5          ; |1021| 
           SUB     .L2     B5,B4,B4          ; 
           NOP             2
           STH     .D2T2   B4,*+B7[B6]       ; |1021| 
	.line	301
;------------------------------------------------------------------------------
; 1022 | ydotpos[dotPosOffset] = CTR_PIX - cd;                    //    dot so t
;     | hat array is ctr'd at origin.                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(132),B5      ; |1022| 
           LDW     .D2T2   *+SP(108),B6      ; |1022| 
           LDW     .D2T2   *+SP(112),B4      ; |1022| 
           MVK     .S2     32767,B7          ; |1022| 
           NOP             2
           SUB     .L2     B7,B6,B6          ; |1022| 
           STH     .D2T2   B6,*+B5[B4]       ; |1022| 
	.line	303
;------------------------------------------------------------------------------
; 1024 | m = CTR_PIX + cd;                                        //    right bo
;     | undary of array                                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |1024| 
           NOP             4
           ADDK    .S2     32767,B4          ; |1024| 
           STW     .D2T2   B4,*+SP(100)      ; |1024| 
	.line	305
;------------------------------------------------------------------------------
; 1026 | for( i = 1; i < parameters->wNumDots[d]; i++ )           //    draw rem
;     | aining dots from left to right,                                        
; 1027 | {                                                        //    a row at
;     |  a time (H = V spacing):                                               
;------------------------------------------------------------------------------
           MVK     .S1     1,A0              ; |1026| 
           STW     .D2T1   A0,*+SP(84)       ; |1026| 
           LDW     .D2T2   *+SP(104),B4      ; |1026| 
           LDW     .D2T2   *+SP(168),B5      ; |1026| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1026| 
           ADDK    .S2     84,B4             ; |1026| 
           LDHU    .D2T2   *B4,B4            ; |1026| 
           NOP             4
           CMPLTU  .L1X    A0,B4,A1          ; 
   [!A1]   B       .S1     L15               ; |1026| 
           NOP             5
           ; BRANCH OCCURS                   ; |1026| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L12:    
	.line	307
;------------------------------------------------------------------------------
; 1028 | j = (UINT32) xdotpos[dotPosOffset + i-1];             //       loc of l
;     | ast dot                                                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B9      ; |1028| 
           LDW     .D2T2   *+SP(84),B6       ; |1028| 
           LDW     .D2T2   *+SP(128),B4      ; |1028| 
           NOP             3
           ADD     .L2     B6,B9,B5          ; |1028| 
           SUB     .L2     B5,1,B5           ; |1028| 
           LDHU    .D2T2   *+B4[B5],B5       ; |1028| 
           NOP             4
           STW     .D2T2   B5,*+SP(88)       ; |1028| 
	.line	308
;------------------------------------------------------------------------------
; 1029 | k = (UINT32) ydotpos[dotPosOffset + i-1];                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(132),B8      ; |1029| 
           ADD     .L2     B6,B9,B7          ; 
           SUB     .L2     B7,1,B7           ; |1029| 
           NOP             2
           LDHU    .D2T1   *+B8[B7],A0       ; |1029| 
           NOP             4
           STW     .D2T1   A0,*+SP(92)       ; |1029| 
	.line	309
;------------------------------------------------------------------------------
; 1030 | l = (UINT32) parameters->wRectL[d];                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B8      ; |1030| 
           LDW     .D2T2   *+SP(104),B7      ; |1030| 
           NOP             4
           ADDAH   .D2     B8,B7,B7          ; |1030| 
           ADDK    .S2     212,B7            ; |1030| 
           LDHU    .D2T1   *B7,A3            ; |1030| 
           NOP             4
           STW     .D2T1   A3,*+SP(96)       ; |1030| 
	.line	310
;------------------------------------------------------------------------------
; 1031 | if( j + l >= m )                                      //       move up 
;     | to next row of dots                                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(100),B7      ; |1031| 
           ADD     .L1X    A3,B5,A4          ; 
           NOP             3
           CMPLTU  .L1X    A4,B7,A1          ; |1031| 
   [ A1]   B       .S1     L13               ; |1031| 
           NOP             5
           ; BRANCH OCCURS                   ; |1031| 
;** --------------------------------------------------------------------------*
	.line	312
;------------------------------------------------------------------------------
; 1033 | if( k + l > MAX_PIX ) break;                       //       out of room
;     |  in upper-right quad. stop!                                            
;------------------------------------------------------------------------------
           ZERO    .L1     A4                ; |1033| 

           SET     .S1     A4,0x0,0xf,A0     ; |1033| 
||         ADD     .L1     A3,A0,A3          ; 

           CMPGTU  .L1     A3,A0,A1          ; |1033| 
   [ A1]   B       .S1     L15               ; |1033| 
           NOP             5
           ; BRANCH OCCURS                   ; |1033| 
;** --------------------------------------------------------------------------*
	.line	313
;------------------------------------------------------------------------------
; 1034 | xdotpos[dotPosOffset + i] = CTR_PIX - cd;                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B7      ; |1034| 
           MVK     .S2     32767,B5          ; |1034| 
           ADD     .S2     B6,B9,B6          ; 
           NOP             2
           SUB     .L2     B5,B7,B5          ; |1034| 
           STH     .D2T2   B5,*+B4[B6]       ; |1034| 
	.line	314
;------------------------------------------------------------------------------
; 1035 | ydotpos[dotPosOffset + i] = (UINT16) (k + l);                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(96),B8       ; |1035| 
           LDW     .D2T2   *+SP(84),B4       ; |1035| 
           LDW     .D2T2   *+SP(92),B6       ; |1035| 
           LDW     .D2T2   *+SP(112),B5      ; |1035| 
           LDW     .D2T2   *+SP(132),B7      ; |1035| 
           NOP             3

           ADD     .S2     B4,B5,B5          ; |1035| 
||         ADD     .L2     B8,B6,B4          ; |1035| 

           STH     .D2T2   B4,*+B7[B5]       ; |1035| 
	.line	315
;------------------------------------------------------------------------------
; 1037 | else                                                  //       move to 
;     | next dot in row                                                        
;------------------------------------------------------------------------------
           B       .S1     L14               ; |1036| 
           NOP             5
           ; BRANCH OCCURS                   ; |1036| 
;** --------------------------------------------------------------------------*
L13:    
	.line	318
;------------------------------------------------------------------------------
; 1039 | xdotpos[dotPosOffset + i] = (UINT16) (j + l);                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(84),B5       ; |1039| 
           LDW     .D2T2   *+SP(112),B6      ; |1039| 
           LDW     .D2T2   *+SP(96),B7       ; |1039| 
           LDW     .D2T2   *+SP(88),B8       ; |1039| 
           LDW     .D2T2   *+SP(128),B4      ; |1039| 
           NOP             3

           ADD     .S2     B5,B6,B5          ; |1039| 
||         ADD     .L2     B7,B8,B6          ; |1039| 

           STH     .D2T2   B6,*+B4[B5]       ; |1039| 
	.line	319
;------------------------------------------------------------------------------
; 1040 | ydotpos[dotPosOffset + i] = (UINT16) k;                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(132),B7      ; |1040| 
           LDW     .D2T2   *+SP(112),B4      ; |1040| 
           LDW     .D2T2   *+SP(84),B5       ; |1040| 
           LDHU    .D2T2   *+SP(92),B6       ; |1040| 
           NOP             3
           ADD     .L2     B5,B4,B4          ; |1040| 
           STH     .D2T2   B6,*+B7[B4]       ; |1040| 
;** --------------------------------------------------------------------------*
L14:    
	.line	321
           LDW     .D2T2   *+SP(84),B4       ; |1042| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1042| 
           STW     .D2T2   B5,*+SP(84)       ; |1042| 
           LDW     .D2T2   *+SP(168),B6      ; |1042| 
           LDW     .D2T2   *+SP(104),B4      ; |1042| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1042| 
           ADDK    .S2     84,B4             ; |1042| 
           LDHU    .D2T2   *B4,B4            ; |1042| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L12               ; |1042| 
           NOP             5
           ; BRANCH OCCURS                   ; |1042| 
;** --------------------------------------------------------------------------*
L15:    
	.line	322
;------------------------------------------------------------------------------
; 1043 | parameters->wNumDots[d] = i;                             //       #dots
;     |  reduced if array did not fit in                                       
; 1044 | //       upper-right quadrant!                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1043| 
           LDW     .D2T2   *+SP(168),B5      ; |1043| 
           LDHU    .D2T2   *+SP(84),B6       ; |1043| 
           NOP             3
           ADDAH   .D2     B5,B4,B4          ; |1043| 
           ADDK    .S2     84,B4             ; |1043| 
           STH     .D2T2   B6,*B4            ; |1043| 
	.line	324
           B       .S1     L36               ; |1045| 
           NOP             5
           ; BRANCH OCCURS                   ; |1045| 
;** --------------------------------------------------------------------------*
L16:    
	.line	326
;------------------------------------------------------------------------------
; 1047 | else if( u16Type == ORIBAR )                                // ORIBAR: 
;     | rect bar or line of dots oriented at                                   
; 1048 | {                                                           // a specif
;     | ic angle in [0..360)...                                                
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1047| 
           NOP             4
           CMPEQ   .L2     B4,10,B0          ; |1047| 
   [!B0]   B       .S1     L22               ; |1047| 
           NOP             5
           ; BRANCH OCCURS                   ; |1047| 
;** --------------------------------------------------------------------------*
	.line	328
;------------------------------------------------------------------------------
; 1049 | hw = (INT16) (parameters->wRectR[d] >> 1);               //    half-wid
;     | th of bar                                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1049| 
           LDW     .D2T2   *+SP(168),B4      ; |1049| 
           NOP             4
           ADDAH   .D2     B4,B5,B6          ; |1049| 
           ADDK    .S2     148,B6            ; |1049| 
           LDHU    .D2T2   *B6,B6            ; |1049| 
           NOP             4
           SHRU    .S2     B6,1,B6           ; |1049| 
           STH     .D2T2   B6,*+SP(8)        ; |1049| 
	.line	329
;------------------------------------------------------------------------------
; 1050 | vw = (INT16) (parameters->wRectL[d] >> 1);               //    half-hei
;     | ght of bar                                                             
;------------------------------------------------------------------------------
           ADDAH   .D2     B4,B5,B6          ; |1050| 
           ADDK    .S2     212,B6            ; |1050| 
           LDHU    .D2T2   *B6,B6            ; |1050| 
           NOP             4
           SHRU    .S2     B6,1,B6           ; |1050| 
           STH     .D2T2   B6,*+SP(10)       ; |1050| 
	.line	331
;------------------------------------------------------------------------------
; 1052 | if( parameters->wRectT[d] >= 360 )                       //    drift ax
;     | is angle, limited to [0..360)                                          
;------------------------------------------------------------------------------
           ADDAH   .D2     B4,B5,B4          ; |1052| 
           ADDK    .S2     276,B4            ; |1052| 
           LDHU    .D2T2   *B4,B5            ; |1052| 
           MVK     .S2     360,B4            ; |1052| 
           NOP             3
           CMPLT   .L2     B5,B4,B0          ; |1052| 
	.line	332
;------------------------------------------------------------------------------
; 1053 | parameters->wRectT[d] = 0;                                             
;------------------------------------------------------------------------------
   [!B0]   LDW     .D2T1   *+SP(104),A0      ; |1053| 
   [!B0]   LDW     .D2T1   *+SP(168),A3      ; |1053| 
           NOP             4
   [!B0]   ADDAH   .D1     A3,A0,A0          ; |1053| 

   [!B0]   ADDK    .S1     276,A0            ; |1053| 
|| [!B0]   ZERO    .L1     A3                ; |1053| 

   [!B0]   STH     .D1T1   A3,*A0            ; |1053| 
	.line	333
;------------------------------------------------------------------------------
; 1054 | xCoord = 10 * parameters->wRectT[d];                     //    convert 
;     | to deg/10                                                              
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A3      ; |1054| 
           LDW     .D2T1   *+SP(168),A0      ; |1054| 
           NOP             4
           ADDAH   .D1     A0,A3,A4          ; |1054| 
           ADDK    .S1     276,A4            ; |1054| 
           LDHU    .D1T1   *A4,A4            ; |1054| 
           NOP             4
           MPYSU   .M1     10,A4,A4          ; |1054| 
           NOP             1
           EXTU    .S1     A4,16,16,A10      ; |1054| 
	.line	335
;------------------------------------------------------------------------------
; 1056 | hv = (INT16) sinLUT[xCoord];                             //    1024*sin
;     | (A), where A = drift axis angle                                        
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(212),A4      ; |1056| 
           NOP             4
           LDH     .D1T1   *+A4[A10],A4      ; |1056| 
           NOP             4
           STH     .D2T1   A4,*+SP(4)        ; |1056| 
	.line	336
;------------------------------------------------------------------------------
; 1057 | vv = (INT16) cosLUT[xCoord];                             //    1024*cos
;     | (A)                                                                    
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(216),A4      ; |1057| 
           NOP             4
           LDH     .D1T1   *+A4[A10],A4      ; |1057| 
           NOP             4
           STH     .D2T1   A4,*+SP(6)        ; |1057| 
	.line	338
;------------------------------------------------------------------------------
; 1059 | if( vw == 0 )                                            //    if zero 
;     | half-height, bar is NOT drawn!                                         
; 1060 | {                                                        //    We put a
;     | ll the dots at (0,0), but we                                           
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(10),B0       ; |1059| 
           NOP             4
   [ B0]   B       .S1     L18               ; |1059| 
           NOP             5
           ; BRANCH OCCURS                   ; |1059| 
;** --------------------------------------------------------------------------*
	.line	340
;------------------------------------------------------------------------------
; 1061 | for(k=0; k<parameters->wNumDots[d]; k++)              //    don't draw 
;     | them in DOFRAME processing.                                            
;------------------------------------------------------------------------------
           ZERO    .L1     A4                ; |1061| 
           STW     .D2T1   A4,*+SP(92)       ; |1061| 
           ADDAH   .D1     A0,A3,A0          ; |1061| 
           ADDK    .S1     84,A0             ; |1061| 
           LDHU    .D1T1   *A0,A0            ; |1061| 
           NOP             4
           CMPLTU  .L1     A4,A0,A1          ; 
   [!A1]   B       .S1     L36               ; |1061| 
           ZERO    .L1     A0                ; |1063| 
           NOP             4
           ; BRANCH OCCURS                   ; |1061| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L17:    
	.line	342
;------------------------------------------------------------------------------
; 1063 | xdotpos[dotPosOffset + k] = (UINT16) 0;                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B4      ; |1063| 
           LDW     .D2T1   *+SP(92),A4       ; |1063| 
           LDW     .D2T1   *+SP(128),A3      ; |1063| 
           NOP             3
           ADD     .L1X    A4,B4,A4          ; |1063| 
           STH     .D1T1   A0,*+A3[A4]       ; |1063| 
	.line	343
;------------------------------------------------------------------------------
; 1064 | ydotpos[dotPosOffset + k] = (UINT16) 0;                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B4      ; |1064| 
           LDW     .D2T1   *+SP(92),A3       ; |1064| 
           LDW     .D2T1   *+SP(132),A4      ; |1064| 
           NOP             3
           ADD     .L1X    A3,B4,A3          ; |1064| 
           STH     .D1T1   A0,*+A4[A3]       ; |1064| 
	.line	344
           LDW     .D2T2   *+SP(92),B4       ; |1065| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1065| 
           STW     .D2T2   B4,*+SP(92)       ; |1065| 
           LDW     .D2T2   *+SP(104),B6      ; |1065| 
           LDW     .D2T2   *+SP(168),B5      ; |1065| 
           NOP             4
           ADDAH   .D2     B5,B6,B5          ; |1065| 
           ADDK    .S2     84,B5             ; |1065| 
           LDHU    .D2T2   *B5,B5            ; |1065| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L17               ; |1065| 
           NOP             5
           ; BRANCH OCCURS                   ; |1065| 
;** --------------------------------------------------------------------------*
	.line	345
           B       .S1     L36               ; |1066| 
           NOP             5
           ; BRANCH OCCURS                   ; |1066| 
;** --------------------------------------------------------------------------*
L18:    
	.line	346
;------------------------------------------------------------------------------
; 1067 | else if( hw == 0 )                                       //    if zero 
;     | half-width, bar is just a line:                                        
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B0        ; |1067| 
           NOP             4
   [ B0]   B       .S1     L20               ; |1067| 
           NOP             5
           ; BRANCH OCCURS                   ; |1067| 
;** --------------------------------------------------------------------------*
	.line	348
;------------------------------------------------------------------------------
; 1069 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
; 1071 | // bar half-ht in pixels -> 2^10 x (half-ht in mm)                     
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1069| 
           STW     .D2T2   B4,*+SP(92)       ; |1069| 
           LDW     .D2T2   *+SP(104),B5      ; |1069| 
           LDW     .D2T2   *+SP(168),B6      ; |1069| 
           NOP             4
           ADDAH   .D2     B6,B5,B5          ; |1069| 
           ADDK    .S2     84,B5             ; |1069| 
           LDHU    .D2T2   *B5,B5            ; |1069| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L36               ; |1069| 
           NOP             5
           ; BRANCH OCCURS                   ; |1069| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L19:    
	.line	351
;------------------------------------------------------------------------------
; 1072 | y32 = (INT32) vw;                                                      
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(10),B4       ; |1072| 
           NOP             4
           STW     .D2T2   B4,*+SP(20)       ; |1072| 
	.line	352
;------------------------------------------------------------------------------
; 1073 | y32 *= parameters->wHeightMM;                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B7      ; |1073| 
           NOP             4
           LDHU    .D2T2   *+B7(6),B5        ; |1073| 
           NOP             4
           MPYLHU  .M2     B5,B4,B6          ; |1073| 
           MPYU    .M2     B5,B4,B4          ; |1073| 
           SHL     .S2     B6,16,B6          ; |1073| 
           ADD     .L2     B4,B6,B4          ; |1073| 
           STW     .D2T2   B4,*+SP(20)       ; |1073| 
	.line	353
;------------------------------------------------------------------------------
; 1074 | y32 >>= 6;                                                             
; 1076 | // yMM*2^10: dots uniformly distributed in [-h/2..h/2] along y-axis (x-
;     | coord is 0), h in mm.                                                  
;------------------------------------------------------------------------------
           SHR     .S2     B4,6,B11          ; |1074| 
           STW     .D2T2   B11,*+SP(20)      ; |1074| 
	.line	356
;------------------------------------------------------------------------------
; 1077 | i32val = y32;                                                          
;------------------------------------------------------------------------------
	.line	357
;------------------------------------------------------------------------------
; 1078 | i32val *= (INT32) 2 * k;                                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B4       ; |1078| 
           NOP             4
           SHL     .S2     B4,1,B4           ; |1078| 
           MPYLH   .M2     B4,B11,B5         ; |1078| 
           MPYLH   .M2     B11,B4,B6         ; |1078| 
           MPYU    .M2     B4,B11,B4         ; |1078| 
           ADD     .L2     B5,B6,B11         ; |1078| 
           SHL     .S2     B11,16,B11        ; |1078| 
           ADD     .L2     B4,B11,B11        ; |1078| 
	.line	358
;------------------------------------------------------------------------------
; 1079 | i32val /= parameters->wNumDots[d];                                     
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A3      ; |1079| 
           MV      .L1X    B7,A0             ; |1079| 
           MVKL    .S2     RL378,B3          ; |1079| 
           MV      .L1X    B11,A4            ; |1079| 
           MVKH    .S2     RL378,B3          ; |1079| 
           ADDAH   .D1     A0,A3,A0          ; |1079| 

           B       .S2     __divi            ; |1079| 
||         ADDK    .S1     84,A0             ; |1079| 

           LDHU    .D1T2   *A0,B4            ; 
           NOP             4
RL378:     ; CALL OCCURS                     ; |1079| 
           MV      .L2X    A4,B11            ; 
	.line	359
;------------------------------------------------------------------------------
; 1080 | i32val -= y32;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |1080| 
           NOP             4
           SUB     .L2     B11,B4,B11        ; |1080| 
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
           STW     .D2T2   B11,*+SP(20)      ; |1081| 
	.line	366
;------------------------------------------------------------------------------
; 1087 | i32val *= (-hv);                                   // -(yMM*2^10)*(2^10
;     | )*sinA = xMM' * 2^20                                                   
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B5        ; |1087| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |1087| 
           MPYSU   .M2     B5,B11,B5         ; |1087| 
           SHL     .S2     B4,16,B4          ; |1087| 
           ADD     .L2     B5,B4,B4          ; |1087| 
           NEG     .L2     B4,B11            ; |1087| 
	.line	367
;------------------------------------------------------------------------------
; 1088 | i32val /= (INT32) parameters->wWidthMM;            // xMM'*2^4*(2^16/sc
;     | reenW_mm) = xPix'*2^4                                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1088| 
           MVKL    .S2     RL380,B3          ; |1088| 
           MV      .L1X    B11,A4            ; |1088| 
           MVKH    .S2     RL380,B3          ; |1088| 
           B       .S1     __divi            ; |1088| 
           LDHU    .D2T2   *+B4(4),B4        ; 
           NOP             4
RL380:     ; CALL OCCURS                     ; |1088| 
           MV      .L2X    A4,B11            ; 
	.line	368
;------------------------------------------------------------------------------
; 1089 | i32val >>= 4;                                      // xPix'            
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1089| 
	.line	369
;------------------------------------------------------------------------------
; 1090 | i32val += 32767;                                   // translate to devi
;     | ce origin                                                              
;------------------------------------------------------------------------------
           ADDK    .S2     32767,B11         ; |1090| 
	.line	370
;------------------------------------------------------------------------------
; 1091 | xdotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B6      ; |1091| 
           LDW     .D2T2   *+SP(92),B5       ; |1091| 
           LDW     .D2T2   *+SP(128),B4      ; |1091| 
           NOP             3
           ADD     .L2     B5,B6,B5          ; |1091| 
           STH     .D2T2   B11,*+B4[B5]      ; |1091| 
	.line	372
;------------------------------------------------------------------------------
; 1093 | i32val = y32 * vv;                                 // (yMM*2^10)*(2^10)
;     | *cosA = yMM' * 2^20                                                    
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B6        ; |1093| 
           LDW     .D2T2   *+SP(20),B5       ; |1093| 
           NOP             4
           MPYLH   .M2     B6,B5,B4          ; |1093| 
           MPYSU   .M2     B6,B5,B5          ; |1093| 
           SHL     .S2     B4,16,B4          ; |1093| 
           ADD     .L2     B5,B4,B11         ; |1093| 
	.line	373
;------------------------------------------------------------------------------
; 1094 | i32val /= (INT32) parameters->wHeightMM;           // yMM'*2^4*(2^16/sc
;     | reenH_mm) = yPix'*2^4                                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1094| 
           MVKL    .S2     RL382,B3          ; |1094| 
           MV      .L1X    B11,A4            ; |1094| 
           MVKH    .S2     RL382,B3          ; |1094| 
           B       .S1     __divi            ; |1094| 
           LDHU    .D2T2   *+B4(6),B4        ; 
           NOP             4
RL382:     ; CALL OCCURS                     ; |1094| 
           MV      .L2X    A4,B11            ; 
	.line	374
;------------------------------------------------------------------------------
; 1095 | i32val >>= 4;                                      // yPix'            
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1095| 
	.line	375
;------------------------------------------------------------------------------
; 1096 | i32val += 32767;                                   // translate to devi
;     | ce origin                                                              
;------------------------------------------------------------------------------
           ADDK    .S2     32767,B11         ; |1096| 
	.line	376
;------------------------------------------------------------------------------
; 1097 | ydotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B5       ; |1097| 
           LDW     .D2T2   *+SP(112),B4      ; |1097| 
           LDW     .D2T2   *+SP(132),B6      ; |1097| 
           NOP             3
           ADD     .L2     B5,B4,B4          ; |1097| 
           STH     .D2T2   B11,*+B6[B4]      ; |1097| 
	.line	377
           LDW     .D2T2   *+SP(92),B4       ; |1098| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1098| 
           STW     .D2T2   B5,*+SP(92)       ; |1098| 
           LDW     .D2T2   *+SP(104),B4      ; |1098| 
           LDW     .D2T2   *+SP(168),B6      ; |1098| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1098| 
           ADDK    .S2     84,B4             ; |1098| 
           LDHU    .D2T2   *B4,B4            ; |1098| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L19               ; |1098| 
           NOP             5
           ; BRANCH OCCURS                   ; |1098| 
;** --------------------------------------------------------------------------*
	.line	378
;------------------------------------------------------------------------------
; 1100 | else                                                     //    general 
;     | case: a rect bar w/ random dots:                                       
;------------------------------------------------------------------------------
           B       .S1     L36               ; |1099| 
           NOP             5
           ; BRANCH OCCURS                   ; |1099| 
;** --------------------------------------------------------------------------*
L20:    
	.line	381
;------------------------------------------------------------------------------
; 1102 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
           ZERO    .L2     B6                ; |1102| 
           STW     .D2T2   B6,*+SP(92)       ; |1102| 
           LDW     .D2T2   *+SP(104),B4      ; |1102| 
           LDW     .D2T2   *+SP(168),B5      ; |1102| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1102| 
           ADDK    .S2     84,B4             ; |1102| 
           LDHU    .D2T2   *B4,B4            ; |1102| 
           NOP             4
           CMPLTU  .L2     B6,B4,B0          ; 
   [!B0]   B       .S1     L36               ; |1102| 
           NOP             5
           ; BRANCH OCCURS                   ; |1102| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L21:    
	.line	383
;------------------------------------------------------------------------------
; 1104 | xCoord = ((UINT16) GetRandNum());                  // random x-coord xP
;     | ix in [-w/2 .. w/2]                                                    
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1104| 
           MVKL    .S2     RL384,B3          ; |1104| 
           MVKH    .S2     RL384,B3          ; |1104| 
           NOP             3
RL384:     ; CALL OCCURS                     ; |1104| 
	.line	384
;------------------------------------------------------------------------------
; 1105 | x32 = (INT32) (xCoord % parameters->wRectR[d]);                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1105| 
           LDW     .D2T2   *+SP(168),B4      ; |1105| 
           MVKL    .S2     RL386,B3          ; |1105| 
           MV      .L1     A4,A10            ; 
           MVKH    .S2     RL386,B3          ; |1105| 
           NOP             1
           ADDAH   .D2     B4,B5,B4          ; |1105| 

           B       .S1     __remi            ; |1105| 
||         ADDK    .S2     148,B4            ; |1105| 

           LDHU    .D2T2   *B4,B4            ; 
           NOP             4
RL386:     ; CALL OCCURS                     ; |1105| 
           STW     .D2T1   A4,*+SP(16)       ; |1105| 
	.line	385
;------------------------------------------------------------------------------
; 1106 | x32 -= (INT32) hw;                                                     
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B4        ; |1106| 
           NOP             4
           SUB     .L2X    A4,B4,B4          ; 
           STW     .D2T2   B4,*+SP(16)       ; |1106| 
	.line	386
;------------------------------------------------------------------------------
; 1107 | x32 *= parameters->wWidthMM;                       // xPix -> 2^6 * xMM
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A0      ; |1107| 
           NOP             4
           LDHU    .D1T1   *+A0(4),A0        ; |1107| 
           NOP             4
           MPYLHU  .M1X    A0,B4,A3          ; |1107| 
           MPYU    .M2X    A0,B4,B4          ; |1107| 
           SHL     .S1     A3,16,A3          ; |1107| 
           ADD     .L1X    B4,A3,A0          ; |1107| 
           STW     .D2T1   A0,*+SP(16)       ; |1107| 
	.line	387
;------------------------------------------------------------------------------
; 1108 | x32 >>= 10;                                                            
;------------------------------------------------------------------------------
           SHR     .S1     A0,10,A0          ; |1108| 
           STW     .D2T1   A0,*+SP(16)       ; |1108| 
	.line	389
;------------------------------------------------------------------------------
; 1110 | yCoord = ((UINT16) GetRandNum());                  // random y-coord yP
;     | ix in [-h/2 .. h/2]                                                    
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1110| 
           MVKL    .S2     RL388,B3          ; |1110| 
           MVKH    .S2     RL388,B3          ; |1110| 
           NOP             3
RL388:     ; CALL OCCURS                     ; |1110| 
	.line	390
;------------------------------------------------------------------------------
; 1111 | y32 = (INT32) (yCoord % parameters->wRectL[d]);                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1111| 
           LDW     .D2T2   *+SP(104),B4      ; |1111| 
           MVKL    .S2     RL390,B3          ; |1111| 
           MV      .L1     A4,A11            ; 
           MVKH    .S2     RL390,B3          ; |1111| 
           NOP             1
           ADDAH   .D2     B5,B4,B4          ; |1111| 

           B       .S1     __remi            ; |1111| 
||         ADDK    .S2     212,B4            ; |1111| 

           LDHU    .D2T2   *B4,B4            ; 
           NOP             4
RL390:     ; CALL OCCURS                     ; |1111| 
           STW     .D2T1   A4,*+SP(20)       ; |1111| 
	.line	391
;------------------------------------------------------------------------------
; 1112 | y32 -= (INT32) vw;                                                     
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(10),B4       ; |1112| 
           NOP             4
           SUB     .L1X    A4,B4,A0          ; 
           STW     .D2T1   A0,*+SP(20)       ; |1112| 
	.line	392
;------------------------------------------------------------------------------
; 1113 | y32 *= parameters->wHeightMM;                      // yPix -> 2^6 * yMM
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1113| 
           NOP             4
           LDHU    .D2T2   *+B4(6),B5        ; |1113| 
           NOP             4
           MPYLHU  .M1X    B5,A0,A3          ; |1113| 
           MPYU    .M2X    B5,A0,B5          ; |1113| 
           SHL     .S1     A3,16,A3          ; |1113| 
           ADD     .L1X    B5,A3,A0          ; |1113| 
           STW     .D2T1   A0,*+SP(20)       ; |1113| 
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
           SHR     .S1     A0,10,A0          ; |1114| 
           STW     .D2T1   A0,*+SP(20)       ; |1114| 
	.line	398
;------------------------------------------------------------------------------
; 1119 | i32val = x32 * vv - y32 * hv;                                          
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B8        ; |1119| 
           LDW     .D2T2   *+SP(16),B7       ; |1119| 
           LDH     .D2T2   *+SP(4),B6        ; |1119| 
           NOP             3
           MPYLH   .M2     B8,B7,B5          ; |1119| 

           MPYSU   .M2     B8,B7,B7          ; |1119| 
||         MPYLH   .M1X    B6,A0,A3          ; |1119| 

           MPYSU   .M2X    B6,A0,B6          ; |1119| 

           SHL     .S1     A3,16,A3          ; |1119| 
||         SHL     .S2     B5,16,B5          ; |1119| 

           ADD     .L2X    B6,A3,B6          ; |1119| 
||         ADD     .S2     B7,B5,B5          ; |1119| 

           SUB     .L2     B5,B6,B11         ; |1119| 
	.line	399
;------------------------------------------------------------------------------
; 1120 | i32val /= (INT32) parameters->wWidthMM;                                
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1120| 
           LDHU    .D2T2   *+B4(4),B4        ; 
           MVKL    .S2     RL392,B3          ; |1120| 
           MV      .L1X    B11,A4            ; |1120| 
           MVKH    .S2     RL392,B3          ; |1120| 
           NOP             1
RL392:     ; CALL OCCURS                     ; |1120| 
           MV      .L2X    A4,B11            ; 
	.line	400
;------------------------------------------------------------------------------
; 1121 | i32val += 32767;                                                       
;------------------------------------------------------------------------------
           ADDK    .S2     32767,B11         ; |1121| 
	.line	401
;------------------------------------------------------------------------------
; 1122 | xdotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B5       ; |1122| 
           LDW     .D2T2   *+SP(112),B4      ; |1122| 
           LDW     .D2T2   *+SP(128),B6      ; |1122| 
           NOP             3
           ADD     .L2     B5,B4,B4          ; |1122| 
           STH     .D2T2   B11,*+B6[B4]      ; |1122| 
	.line	403
;------------------------------------------------------------------------------
; 1124 | i32val = x32 * hv + y32 * vv;                                          
;------------------------------------------------------------------------------
           LDH     .D2T1   *+SP(6),A3        ; |1124| 
           LDW     .D2T2   *+SP(20),B5       ; |1124| 
           LDH     .D2T2   *+SP(4),B7        ; |1124| 
           LDW     .D2T2   *+SP(16),B6       ; |1124| 
           NOP             3

           MPYLH   .M1X    A3,B5,A0          ; |1124| 
||         MPYSU   .M2X    A3,B5,B5          ; |1124| 

           MPYLH   .M2     B7,B6,B4          ; |1124| 
           MPYSU   .M2     B7,B6,B6          ; |1124| 

           SHL     .S1     A0,16,A0          ; |1124| 
||         SHL     .S2     B4,16,B4          ; |1124| 

           ADD     .L2X    B5,A0,B5          ; |1124| 
||         ADD     .S2     B6,B4,B4          ; |1124| 

           ADD     .L2     B5,B4,B11         ; |1124| 
	.line	404
;------------------------------------------------------------------------------
; 1125 | i32val /= (INT32) parameters->wHeightMM;                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1125| 
           MVKL    .S2     RL394,B3          ; |1125| 
           MV      .L1X    B11,A4            ; |1125| 
           MVKH    .S2     RL394,B3          ; |1125| 
           B       .S1     __divi            ; |1125| 
           LDHU    .D2T2   *+B4(6),B4        ; 
           NOP             4
RL394:     ; CALL OCCURS                     ; |1125| 
           MV      .L2X    A4,B11            ; 
	.line	405
;------------------------------------------------------------------------------
; 1126 | i32val += 32767;                                                       
;------------------------------------------------------------------------------
           ADDK    .S2     32767,B11         ; |1126| 
	.line	406
;------------------------------------------------------------------------------
; 1127 | ydotpos[dotPosOffset + k] = (UINT16)i32val;                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B6      ; |1127| 
           LDW     .D2T2   *+SP(92),B4       ; |1127| 
           LDW     .D2T2   *+SP(132),B5      ; |1127| 
           NOP             3
           ADD     .L2     B4,B6,B4          ; |1127| 
           STH     .D2T2   B11,*+B5[B4]      ; |1127| 
	.line	407
           LDW     .D2T2   *+SP(92),B4       ; |1128| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1128| 
           STW     .D2T2   B5,*+SP(92)       ; |1128| 
           LDW     .D2T2   *+SP(104),B4      ; |1128| 
           LDW     .D2T2   *+SP(168),B6      ; |1128| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1128| 
           ADDK    .S2     84,B4             ; |1128| 
           LDHU    .D2T2   *B4,B4            ; |1128| 
           NOP             4
;** --------------------------------------------------------------------------*
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L21               ; |1128| 
           NOP             5
           ; BRANCH OCCURS                   ; |1128| 
;** --------------------------------------------------------------------------*
	.line	409
           B       .S1     L36               ; |1130| 
           NOP             5
           ; BRANCH OCCURS                   ; |1130| 
;** --------------------------------------------------------------------------*
L22:    
	.line	411
;------------------------------------------------------------------------------
; 1132 | else if( u16Type == STATICANNU )                            // STATICAN
;     | NU: Optimized implementation of rect                                   
; 1133 | {                                                           // annulus 
;     | when neither window nor dots move.                                     
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1132| 
           NOP             4
           CMPEQ   .L2     B4,6,B0           ; |1132| 
   [!B0]   B       .S1     L26               ; |1132| 
           NOP             5
           ; BRANCH OCCURS                   ; |1132| 
;** --------------------------------------------------------------------------*
	.line	413
;------------------------------------------------------------------------------
; 1134 | l = 0;                                                   // dots always
;     |  stay at their initial positions!                                      
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1134| 
           STW     .D2T2   B4,*+SP(96)       ; |1134| 
	.line	414
;------------------------------------------------------------------------------
; 1135 | for ( k = 0; k < parameters->wNumDots[d]; k++ )          // we generate
;     |  dots randomly, then drop all                                          
; 1136 | {                                                        // those which
;     |  are outside the annular window!                                       
;------------------------------------------------------------------------------
           STW     .D2T2   B4,*+SP(92)       ; |1135| 
           LDW     .D2T2   *+SP(104),B6      ; |1135| 
           LDW     .D2T2   *+SP(168),B5      ; |1135| 
           NOP             4
           ADDAH   .D2     B5,B6,B5          ; |1135| 
           ADDK    .S2     84,B5             ; |1135| 
           LDHU    .D2T2   *B5,B5            ; |1135| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L25               ; |1135| 
           NOP             5
           ; BRANCH OCCURS                   ; |1135| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L23:    
	.line	416
;------------------------------------------------------------------------------
; 1137 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1137| 
           MVKL    .S2     RL396,B3          ; |1137| 
           MVKH    .S2     RL396,B3          ; |1137| 
           NOP             3
RL396:     ; CALL OCCURS                     ; |1137| 
           MV      .L1     A4,A10            ; 
	.line	417
;------------------------------------------------------------------------------
; 1138 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1138| 
           MVKL    .S2     RL398,B3          ; |1138| 
           MVKH    .S2     RL398,B3          ; |1138| 
           NOP             3
RL398:     ; CALL OCCURS                     ; |1138| 
           MV      .L1     A4,A11            ; 
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
           LDW     .D2T1   *+SP(168),A3      ; |1139| 
           LDW     .D2T1   *+SP(104),A4      ; |1139| 
           NOP             4
           ADDAH   .D1     A3,A4,A0          ; |1139| 
           ADDK    .S1     468,A0            ; |1139| 
           LDHU    .D1T1   *A0,A0            ; |1139| 
           NOP             4
           CMPLT   .L1     A10,A0,A0         ; |1139| 
           XOR     .L1     1,A0,A1           ; |1139| 
   [ A1]   LDW     .D2T2   *+SP(104),B4      ; |1139| 
   [ A1]   LDW     .D2T2   *+SP(168),B5      ; |1139| 
           NOP             4
   [ A1]   ADDAH   .D2     B5,B4,B4          ; |1139| 
   [ A1]   ADDK    .S2     404,B4            ; |1139| 
   [ A1]   LDHU    .D2T2   *B4,B4            ; |1139| 
           NOP             4
   [ A1]   CMPGT   .L1X    A10,B4,A0         ; |1139| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1139| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1139| 
   [ A1]   LDW     .D2T2   *+SP(104),B4      ; |1139| 
   [ A1]   LDW     .D2T2   *+SP(168),B5      ; |1139| 
           NOP             4
   [ A1]   ADDAH   .D2     B5,B4,B4          ; |1139| 
   [ A1]   ADDK    .S2     596,B4            ; |1139| 
   [ A1]   LDHU    .D2T2   *B4,B4            ; |1139| 
           NOP             4
   [ A1]   CMPLT   .L1X    A11,B4,A0         ; |1139| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1139| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1139| 
   [ A1]   LDW     .D2T1   *+SP(168),A0      ; |1139| 
   [ A1]   LDW     .D2T1   *+SP(104),A5      ; |1139| 
           NOP             4
   [ A1]   ADDAH   .D1     A0,A5,A0          ; |1139| 
   [ A1]   ADDK    .S1     532,A0            ; |1139| 
   [ A1]   LDHU    .D1T1   *A0,A0            ; |1139| 
           NOP             4
   [ A1]   CMPGT   .L1     A11,A0,A0         ; |1139| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1139| 
   [!A1]   B       .S1     L24               ; |1139| 
   [ A1]   ADDAH   .D1     A3,A4,A0          ; |1139| 
   [ A1]   ADDK    .S1     148,A0            ; |1139| 
   [ A1]   LDHU    .D1T1   *A0,A0            ; |1139| 
           NOP             2
           ; BRANCH OCCURS                   ; |1139| 
;** --------------------------------------------------------------------------*
           NOP             4
           CMPGT   .L1     A10,A0,A1         ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1139| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1139| 
   [!A1]   ADDK    .S2     212,B4            ; |1139| 
   [!A1]   LDHU    .D2T2   *B4,B4            ; |1139| 
           NOP             4
   [!A1]   CMPLT   .L1X    A10,B4,A1         ; |1139| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(168),B5      ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(104),B4      ; |1139| 
           NOP             4
   [!A1]   ADDAH   .D2     B5,B4,B4          ; |1139| 
   [!A1]   ADDK    .S2     276,B4            ; |1139| 
   [!A1]   LDHU    .D2T2   *B4,B4            ; |1139| 
           NOP             4
   [!A1]   CMPGT   .L1X    A11,B4,A1         ; |1139| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1139| 
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1139| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1139| 
   [!A1]   ADDK    .S2     340,B4            ; |1139| 
   [!A1]   LDHU    .D2T2   *B4,B4            ; |1139| 
           NOP             4
   [!A1]   CMPLT   .L1X    A11,B4,A1         ; |1139| 
;** --------------------------------------------------------------------------*
L24:    
	.line	423
;------------------------------------------------------------------------------
; 1144 | xdotpos[dotPosOffset + l] = xCoord;                                    
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(112),B4      ; |1144| 
   [ A1]   LDW     .D2T1   *+SP(96),A0       ; |1144| 
   [ A1]   LDW     .D2T1   *+SP(128),A3      ; |1144| 
           NOP             3
   [ A1]   ADD     .L1X    A0,B4,A0          ; |1144| 
   [ A1]   STH     .D1T1   A10,*+A3[A0]      ; |1144| 
	.line	424
;------------------------------------------------------------------------------
; 1145 | ydotpos[dotPosOffset + l] = yCoord;                                    
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(112),B4      ; |1145| 
   [ A1]   LDW     .D2T1   *+SP(96),A0       ; |1145| 
   [ A1]   LDW     .D2T1   *+SP(132),A3      ; |1145| 
           NOP             3
   [ A1]   ADD     .L1X    A0,B4,A0          ; |1145| 
   [ A1]   STH     .D1T1   A11,*+A3[A0]      ; |1145| 
	.line	425
;------------------------------------------------------------------------------
; 1146 | l++;                                                                   
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(96),B4       ; |1146| 
           NOP             4
   [ A1]   ADD     .L2     1,B4,B4           ; |1146| 
   [ A1]   STW     .D2T2   B4,*+SP(96)       ; |1146| 
	.line	427
           LDW     .D2T2   *+SP(92),B4       ; |1148| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1148| 
           STW     .D2T2   B5,*+SP(92)       ; |1148| 
           LDW     .D2T2   *+SP(168),B6      ; |1148| 
           LDW     .D2T2   *+SP(104),B4      ; |1148| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1148| 
           ADDK    .S2     84,B4             ; |1148| 
           LDHU    .D2T2   *B4,B4            ; |1148| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L23               ; |1148| 
           NOP             5
           ; BRANCH OCCURS                   ; |1148| 
;** --------------------------------------------------------------------------*
L25:    
	.line	428
;------------------------------------------------------------------------------
; 1149 | parameters->wNumDots[d] = l;                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1149| 
           LDW     .D2T2   *+SP(168),B5      ; |1149| 
           LDHU    .D2T2   *+SP(96),B6       ; |1149| 
           NOP             3
           ADDAH   .D2     B5,B4,B4          ; |1149| 
           ADDK    .S2     84,B4             ; |1149| 
           STH     .D2T2   B6,*B4            ; |1149| 
	.line	429
           B       .S1     L36               ; |1150| 
           NOP             5
           ; BRANCH OCCURS                   ; |1150| 
;** --------------------------------------------------------------------------*
L26:    
	.line	431
;------------------------------------------------------------------------------
; 1152 | else if( u16Type == OPTRECTWIN || u16Type == OPTCOHERENT || // OPTRECTW
;     | IN, etc: tgt dots randomly distrib.                                    
; 1153 | u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR || // within boundaries
;     |  (incl edges) of the                                                   
; 1154 | u16Type == DL_NOISESPEED )                         // visible window...
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1152| 
           NOP             4
           CMPEQ   .L2     B4,7,B0           ; |1152| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1152| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,12,B0          ; |1152| 
           CMPEQ   .L2     B0,0,B4           ; 
           XOR     .L2     1,B4,B0           ; |1152| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1152| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,8,B0           ; |1152| 
           CMPEQ   .L2     B0,0,B4           ; 
           XOR     .L2     1,B4,B0           ; |1152| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1152| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,11,B0          ; |1152| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1152| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,13,B0          ; |1152| 
   [!B0]   B       .S1     L32               ; |1152| 
           NOP             5
           ; BRANCH OCCURS                   ; |1152| 
;** --------------------------------------------------------------------------*
	.line	435
;------------------------------------------------------------------------------
; 1156 | hsize[d] = parameters->wRectR[d] - parameters->wRectL[d] + 1;          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1156| 
           LDW     .D2T2   *+SP(104),B4      ; |1156| 
           NOP             3
           MV      .L1X    B5,A0             ; |1156| 

           ADDAH   .D2     B5,B4,B5          ; |1156| 
||         MV      .L1X    B4,A3             ; 

           ADDK    .S2     212,B5            ; |1156| 
||         ADDAH   .D1     A0,A3,A0          ; |1156| 

           LDHU    .D2T2   *B5,B6            ; |1156| 
||         ADDK    .S1     148,A0            ; |1156| 

           LDHU    .D1T1   *A0,A0            ; |1156| 
           LDW     .D2T2   *+SP(172),B5      ; |1156| 
           NOP             3
           SUB     .L1X    A0,B6,A0          ; |1156| 
           ADD     .L1     1,A0,A0           ; |1156| 
           STH     .D2T1   A0,*+B5[B4]       ; |1156| 
	.line	436
;------------------------------------------------------------------------------
; 1157 | vsize[d] = parameters->wRectT[d] - parameters->wRectB[d] + 1;          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1157| 
           LDW     .D2T2   *+SP(104),B5      ; |1157| 
           NOP             4
           ADDAH   .D2     B4,B5,B6          ; |1157| 

           ADDAH   .D2     B4,B5,B4          ; |1157| 
||         ADDK    .S2     340,B6            ; |1157| 

           LDHU    .D2T2   *B6,B7            ; |1157| 
||         ADDK    .S2     276,B4            ; |1157| 

           LDHU    .D2T2   *B4,B6            ; |1157| 
           LDW     .D2T2   *+SP(176),B4      ; |1157| 
           NOP             3
           SUB     .L2     B6,B7,B6          ; |1157| 
           ADD     .L2     1,B6,B6           ; |1157| 
           STH     .D2T2   B6,*+B4[B5]       ; |1157| 
	.line	437
;------------------------------------------------------------------------------
; 1158 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |1158| 
           STW     .D2T1   A0,*+SP(92)       ; |1158| 
           LDW     .D2T1   *+SP(168),A4      ; |1158| 
           LDW     .D2T1   *+SP(104),A3      ; |1158| 
           NOP             4
           ADDAH   .D1     A4,A3,A3          ; |1158| 
           ADDK    .S1     84,A3             ; |1158| 
           LDHU    .D1T1   *A3,A3            ; |1158| 
           NOP             4
           CMPLTU  .L1     A0,A3,A1          ; 
   [!A1]   B       .S1     L28               ; |1158| 
           NOP             5
           ; BRANCH OCCURS                   ; |1158| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L27:    
	.line	439
;------------------------------------------------------------------------------
; 1160 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1160| 
           MVKL    .S2     RL400,B3          ; |1160| 
           MVKH    .S2     RL400,B3          ; |1160| 
           NOP             3
RL400:     ; CALL OCCURS                     ; |1160| 
           MV      .L1     A4,A10            ; 
	.line	440
;------------------------------------------------------------------------------
; 1161 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1161| 
           MVKL    .S2     RL402,B3          ; |1161| 
           MVKH    .S2     RL402,B3          ; |1161| 
           NOP             3
RL402:     ; CALL OCCURS                     ; |1161| 
           MV      .L1     A4,A11            ; 
	.line	441
;------------------------------------------------------------------------------
; 1162 | xdotpos[dotPosOffset + k] = parameters->wRectL[d] + (xCoord % hsize[d])
;     | ;                                                                      
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1162| 
           LDW     .D2T2   *+SP(172),B4      ; |1162| 
           MVKL    .S2     RL404,B3          ; |1162| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL404,B3          ; |1162| 
           B       .S1     __remi            ; |1162| 
           LDHU    .D2T2   *+B4[B5],B4       ; 
           NOP             4
RL404:     ; CALL OCCURS                     ; |1162| 
           LDW     .D2T1   *+SP(104),A3      ; |1162| 
           LDW     .D2T1   *+SP(168),A0      ; |1162| 
           LDW     .D2T2   *+SP(112),B4      ; |1162| 
           LDW     .D2T1   *+SP(128),A5      ; |1162| 
           NOP             2
           ADDAH   .D1     A0,A3,A0          ; |1162| 
           ADDK    .S1     212,A0            ; |1162| 
           LDHU    .D1T1   *A0,A3            ; |1162| 
           LDW     .D2T1   *+SP(92),A0       ; |1162| 
           NOP             4

           ADD     .L1     A4,A3,A3          ; 
||         ADD     .S1X    A0,B4,A0          ; |1162| 

           STH     .D1T1   A3,*+A5[A0]       ; |1162| 
	.line	442
;------------------------------------------------------------------------------
; 1163 | ydotpos[dotPosOffset + k] = parameters->wRectB[d] + (yCoord % vsize[d])
;     | ;                                                                      
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1163| 
           LDW     .D2T2   *+SP(176),B5      ; |1163| 
           MVKL    .S2     RL406,B3          ; |1163| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL406,B3          ; |1163| 
           B       .S1     __remi            ; |1163| 
           LDHU    .D2T2   *+B5[B4],B4       ; 
           NOP             4
RL406:     ; CALL OCCURS                     ; |1163| 
           LDW     .D2T2   *+SP(104),B4      ; |1163| 
           LDW     .D2T2   *+SP(168),B5      ; |1163| 
           LDW     .D2T2   *+SP(112),B6      ; |1163| 
           NOP             3
           ADDAH   .D2     B5,B4,B4          ; |1163| 
           ADDK    .S2     340,B4            ; |1163| 
           LDHU    .D2T2   *B4,B7            ; |1163| 
           LDW     .D2T2   *+SP(92),B5       ; |1163| 
           LDW     .D2T2   *+SP(132),B4      ; |1163| 
           NOP             3

           ADD     .L2     B5,B6,B5          ; |1163| 
||         ADD     .L1X    A4,B7,A0          ; 

           STH     .D2T1   A0,*+B4[B5]       ; |1163| 
	.line	443
;------------------------------------------------------------------------------
; 1166 | // for these types, we also assign a random lifetime between 1 and targ
;     | et's maximum dot life. We also                                         
; 1167 | // make sure the per-dot fractional pixel displacements are initially 0
;     |  (appl to noisy tgts only).                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B4       ; |1164| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1164| 
           STW     .D2T2   B4,*+SP(92)       ; |1164| 
           LDW     .D2T1   *+SP(168),A3      ; |1164| 
           LDW     .D2T1   *+SP(104),A0      ; |1164| 
           NOP             4
           ADDAH   .D1     A3,A0,A0          ; |1164| 
           ADDK    .S1     84,A0             ; |1164| 
           LDHU    .D1T2   *A0,B5            ; |1164| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L27               ; |1164| 
           NOP             5
           ; BRANCH OCCURS                   ; |1164| 
;** --------------------------------------------------------------------------*
L28:    
	.line	447
;------------------------------------------------------------------------------
; 1168 | if(u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||  u16Type == DL_NO
;     | ISESPEED)                                                              
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1168| 
           NOP             4
           CMPEQ   .L2     B4,8,B0           ; |1168| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1168| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,11,B0          ; |1168| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1168| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,13,B0          ; |1168| 
   [!B0]   B       .S1     L31               ; |1168| 
           NOP             5
           ; BRANCH OCCURS                   ; |1168| 
;** --------------------------------------------------------------------------*
	.line	449
;------------------------------------------------------------------------------
; 1170 | nMaxDotLife = (UINT16) parameters->wOuterR[d];                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1170| 
           LDW     .D2T2   *+SP(168),B4      ; |1170| 
           NOP             4
           ADDAH   .D2     B4,B5,B4          ; |1170| 
           ADDK    .S2     404,B4            ; |1170| 
           LDHU    .D2T2   *B4,B4            ; |1170| 
           NOP             4
           STH     .D2T2   B4,*+SP(44)       ; |1170| 
	.line	450
;------------------------------------------------------------------------------
; 1171 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B4       ; |1171| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1171| 
   [!B0]   MVK     .S2     1,B4              ; |1171| 
   [!B0]   STH     .D2T2   B4,*+SP(44)       ; |1171| 
   [!B0]   B       .S1     L29               ; |1171| 
           NOP             5
           ; BRANCH OCCURS                   ; |1171| 
;** --------------------------------------------------------------------------*
	.line	451
;------------------------------------------------------------------------------
; 1172 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1172| 
           MVK     .S2     32767,B4          ; |1172| 
           NOP             3
           CMPGT   .L2     B5,B4,B0          ; |1172| 
   [ B0]   MVK     .S1     32767,A0          ; |1172| 
   [ B0]   STH     .D2T1   A0,*+SP(44)       ; |1172| 
;** --------------------------------------------------------------------------*
L29:    
	.line	452
;------------------------------------------------------------------------------
; 1173 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1173| 
           STW     .D2T2   B4,*+SP(92)       ; |1173| 
           LDW     .D2T2   *+SP(104),B5      ; |1173| 
           LDW     .D2T2   *+SP(168),B6      ; |1173| 
           ZERO    .L1     A7                ; |1178| 
           NOP             3
           ADDAH   .D2     B6,B5,B5          ; |1173| 
           ADDK    .S2     84,B5             ; |1173| 
           LDHU    .D2T2   *B5,B5            ; |1173| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L31               ; |1173| 
           NOP             5
           ; BRANCH OCCURS                   ; |1173| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L30:    
	.line	454
;------------------------------------------------------------------------------
; 1175 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1175| 
           MVKL    .S2     RL408,B3          ; |1175| 
           MVKH    .S2     RL408,B3          ; |1175| 
           NOP             3
RL408:     ; CALL OCCURS                     ; |1175| 
	.line	455
;------------------------------------------------------------------------------
; 1176 | dotLife[dotPosOffset + k] = (xCoord % nMaxDotLife) + 1;                
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1176| 
           LDHU    .D2T2   *+SP(44),B4       ; 
           MVKL    .S2     RL410,B3          ; |1176| 
           MV      .L1     A4,A10            ; 
           MVKH    .S2     RL410,B3          ; |1176| 
           NOP             1
RL410:     ; CALL OCCURS                     ; |1176| 
           LDW     .D2T1   *+SP(92),A3       ; |1176| 
           LDW     .D2T2   *+SP(112),B4      ; |1176| 
           LDW     .D2T1   *+SP(136),A0      ; |1176| 
           ADD     .S1     1,A4,A4           ; 
           NOP             2
           ADD     .L1X    A3,B4,A3          ; |1176| 
           STH     .D1T1   A4,*+A0[A3]       ; |1176| 
	.line	457
;------------------------------------------------------------------------------
; 1178 | fracDX[dotPosOffset + k] = 0;                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(112),B4      ; |1178| 
           LDW     .D2T1   *+SP(92),A0       ; |1178| 
           LDW     .D2T1   *+SP(152),A3      ; |1178| 
           NOP             3
           ADD     .L1X    A0,B4,A0          ; |1178| 
           STH     .D1T1   A7,*+A3[A0]       ; |1178| 
	.line	458
;------------------------------------------------------------------------------
; 1179 | fracDY[dotPosOffset + k] = 0;                                          
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(92),A0       ; |1179| 
           LDW     .D2T2   *+SP(112),B4      ; |1179| 
           LDW     .D2T1   *+SP(160),A3      ; |1179| 
           NOP             3
           ADD     .L1X    A0,B4,A0          ; |1179| 
           STH     .D1T1   A7,*+A3[A0]       ; |1179| 
	.line	459
           LDW     .D2T2   *+SP(92),B4       ; |1180| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1180| 
           STW     .D2T2   B5,*+SP(92)       ; |1180| 
           LDW     .D2T2   *+SP(104),B6      ; |1180| 
           LDW     .D2T2   *+SP(168),B4      ; |1180| 
           NOP             4
           ADDAH   .D2     B4,B6,B4          ; |1180| 
           ADDK    .S2     84,B4             ; |1180| 
           LDHU    .D2T2   *B4,B4            ; |1180| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L30               ; |1180| 
           NOP             5
           ; BRANCH OCCURS                   ; |1180| 
;** --------------------------------------------------------------------------*
L31:    
	.line	462
;------------------------------------------------------------------------------
; 1183 | if( u16Type == DL_NOISEDIR || u16Type == DL_NOISESPEED ) //    noise up
;     | date timer reset so that per-dot                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1183| 
           NOP             4
           CMPEQ   .L2     B4,11,B0          ; |1183| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1183| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,13,B0          ; |1183| 
	.line	463
;------------------------------------------------------------------------------
; 1184 | nNoiseUpdTicks[d] = 0;                                //    noise facto
;     | rs are randomly chosen on the                                          
; 1185 | //    very first update frame!                                         
;------------------------------------------------------------------------------
   [ B0]   LDW     .D2T2   *+SP(188),B6      ; |1184| 
   [ B0]   LDW     .D2T2   *+SP(104),B4      ; |1184| 
   [ B0]   ZERO    .L2     B5                ; |1184| 
           NOP             3
   [ B0]   STH     .D2T2   B5,*+B6[B4]       ; |1184| 
	.line	465
           B       .S1     L36               ; |1186| 
           NOP             5
           ; BRANCH OCCURS                   ; |1186| 
;** --------------------------------------------------------------------------*
L32:    
	.line	467
;------------------------------------------------------------------------------
; 1188 | else if( u16Type == OPTICFLOW )                             // OPTICFLO
;     | W: flow field. dot pos stored in                                       
; 1189 | {                                                           // polar co
;     | ords (r,TH) rather than (x,y) pix:                                     
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1188| 
           NOP             4
           CMPEQ   .L2     B4,9,B0           ; |1188| 
   [!B0]   B       .S1     L34               ; |1188| 
           NOP             5
           ; BRANCH OCCURS                   ; |1188| 
;** --------------------------------------------------------------------------*
	.line	469
;------------------------------------------------------------------------------
; 1190 | rectR = parameters->wRectR[d];                           //    inner ra
;     | dius in deg/100 of visual angle                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1190| 
           LDW     .D2T2   *+SP(168),B5      ; |1190| 
           NOP             4
           ADDAH   .D2     B5,B4,B6          ; |1190| 
           ADDK    .S2     148,B6            ; |1190| 
           LDHU    .D2T1   *B6,A8            ; |1190| 
           NOP             4
	.line	470
;------------------------------------------------------------------------------
; 1191 | ++rectR;                                                 //    no dots 
;     | AT inner or outer rad initially                                        
;------------------------------------------------------------------------------
           ADD     .L1     1,A8,A0           ; |1191| 
           EXTU    .S1     A0,16,16,A8       ; |1191| 
	.line	471
;------------------------------------------------------------------------------
; 1192 | rectL = parameters->wRectL[d];                           //    outer ra
;     | dius in deg/100 of visual angle                                        
;------------------------------------------------------------------------------
           MV      .L1X    B5,A3
           MV      .L1X    B4,A0
           ADDAH   .D1     A3,A0,A0          ; |1192| 
           ADDK    .S1     212,A0            ; |1192| 
           LDHU    .D1T2   *A0,B8            ; |1192| 
           NOP             4
	.line	472
;------------------------------------------------------------------------------
; 1193 | rectL -= rectR;                                          //    differen
;     | ce in deg/100                                                          
;------------------------------------------------------------------------------
           SUB     .L2X    B8,A8,B6          ; |1193| 
           EXTU    .S2     B6,16,16,B8       ; |1193| 
	.line	474
;------------------------------------------------------------------------------
; 1195 | for( k = 0; k < parameters->wNumDots[d]; k++ )                         
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |1195| 
           STW     .D2T1   A0,*+SP(92)       ; |1195| 
           MV      .L1X    B5,A4
           MV      .L1X    B4,A3
           ADDAH   .D1     A4,A3,A3          ; |1195| 
           ADDK    .S1     84,A3             ; |1195| 
           LDHU    .D1T1   *A3,A3            ; |1195| 
           ZERO    .L1     A7                ; |1201| 
           NOP             3
           CMPLTU  .L1     A0,A3,A1          ; 
   [!A1]   B       .S1     L36               ; |1195| 
           NOP             5
           ; BRANCH OCCURS                   ; |1195| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L33:    
	.line	476
;------------------------------------------------------------------------------
; 1197 | xCoord = (UINT16) GetRandNum();                       //    init random
;     |  radial pos in visual deg/100                                          
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1197| 
           MVKL    .S2     RL412,B3          ; |1197| 
           MVKH    .S2     RL412,B3          ; |1197| 
           NOP             3
RL412:     ; CALL OCCURS                     ; |1197| 
	.line	477
;------------------------------------------------------------------------------
; 1198 | xdotpos[dotPosOffset + k] = rectR + (xCoord % rectL);                  
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1198| 
           MVKL    .S2     RL414,B3          ; |1198| 
           MV      .L1     A4,A10            ; 
           MVKH    .S2     RL414,B3          ; |1198| 
           MV      .L2     B8,B4             ; 
           NOP             1
RL414:     ; CALL OCCURS                     ; |1198| 
           LDW     .D2T1   *+SP(92),A3       ; |1198| 
           LDW     .D2T2   *+SP(112),B4      ; |1198| 
           LDW     .D2T1   *+SP(128),A0      ; |1198| 
           NOP             3

           ADD     .L1X    A3,B4,A4          ; |1198| 
||         ADD     .S1     A4,A8,A3          ; 

           STH     .D1T1   A3,*+A0[A4]       ; |1198| 
	.line	478
;------------------------------------------------------------------------------
; 1199 | yCoord = (UINT16) GetRandNum();                       //    init angula
;     | r pos in deg/10                                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1199| 
           MVKL    .S2     RL416,B3          ; |1199| 
           MVKH    .S2     RL416,B3          ; |1199| 
           NOP             3
RL416:     ; CALL OCCURS                     ; |1199| 
	.line	479
;------------------------------------------------------------------------------
; 1200 | ydotpos[dotPosOffset + k] = (yCoord % 3600);                           
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1200| 
           MVKL    .S2     RL418,B3          ; |1200| 
           MVKH    .S2     RL418,B3          ; |1200| 
           MV      .L1     A4,A11            ; 
           MVK     .S2     0xe10,B4          ; |1200| 
           NOP             1
RL418:     ; CALL OCCURS                     ; |1200| 
           LDW     .D2T1   *+SP(92),A3       ; |1200| 
           LDW     .D2T2   *+SP(112),B4      ; |1200| 
           LDW     .D2T1   *+SP(132),A0      ; |1200| 
           NOP             3
           ADD     .L1X    A3,B4,A3          ; |1200| 
           STH     .D1T1   A4,*+A0[A3]       ; |1200| 
	.line	480
;------------------------------------------------------------------------------
; 1201 | dotLife[dotPosOffset + k] = 0;                        //    reset frac 
;     | pos change (<1/100deg)                                                 
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(92),A3       ; |1201| 
           LDW     .D2T2   *+SP(112),B4      ; |1201| 
           LDW     .D2T1   *+SP(136),A0      ; |1201| 
           NOP             3
           ADD     .L1X    A3,B4,A3          ; |1201| 
           STH     .D1T1   A7,*+A0[A3]       ; |1201| 
	.line	481
           LDW     .D2T2   *+SP(92),B4       ; |1202| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1202| 
           STW     .D2T2   B5,*+SP(92)       ; |1202| 
           LDW     .D2T2   *+SP(168),B6      ; |1202| 
           LDW     .D2T2   *+SP(104),B4      ; |1202| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1202| 
           ADDK    .S2     84,B4             ; |1202| 
           LDHU    .D2T2   *B4,B4            ; |1202| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L33               ; |1202| 
           NOP             5
           ; BRANCH OCCURS                   ; |1202| 
;** --------------------------------------------------------------------------*
	.line	482
;------------------------------------------------------------------------------
; 1205 | else                                                        // ALL OTHE
;     | R TYPES:  random-dot texture drawn                                     
; 1206 | {                                                           // to fill 
;     | the entire screen...                                                   
;------------------------------------------------------------------------------
           B       .S1     L36               ; |1203| 
           NOP             5
           ; BRANCH OCCURS                   ; |1203| 
;** --------------------------------------------------------------------------*
L34:    
	.line	486
;------------------------------------------------------------------------------
; 1207 | for ( k = 0; k < parameters->wNumDots[d]; k++ )                        
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1207| 
           STW     .D2T2   B4,*+SP(92)       ; |1207| 
           LDW     .D2T2   *+SP(104),B5      ; |1207| 
           LDW     .D2T2   *+SP(168),B6      ; |1207| 
           NOP             4
           ADDAH   .D2     B6,B5,B5          ; |1207| 
           ADDK    .S2     84,B5             ; |1207| 
           LDHU    .D2T2   *B5,B5            ; |1207| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L36               ; |1207| 
           NOP             5
           ; BRANCH OCCURS                   ; |1207| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L35:    
	.line	488
;------------------------------------------------------------------------------
; 1209 | xdotpos[dotPosOffset + k] = (UINT16) GetRandNum();                     
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1209| 
           MVKL    .S2     RL420,B3          ; |1209| 
           MVKH    .S2     RL420,B3          ; |1209| 
           NOP             3
RL420:     ; CALL OCCURS                     ; |1209| 
           LDW     .D2T2   *+SP(112),B4      ; |1209| 
           LDW     .D2T1   *+SP(92),A0       ; |1209| 
           LDW     .D2T1   *+SP(128),A3      ; |1209| 
           NOP             3
           ADD     .L1X    A0,B4,A0          ; |1209| 
           STH     .D1T1   A4,*+A3[A0]       ; |1209| 
	.line	489
;------------------------------------------------------------------------------
; 1210 | ydotpos[dotPosOffset + k] = (UINT16) GetRandNum();                     
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1210| 
           MVKL    .S2     RL422,B3          ; |1210| 
           MVKH    .S2     RL422,B3          ; |1210| 
           NOP             3
RL422:     ; CALL OCCURS                     ; |1210| 
           LDW     .D2T2   *+SP(112),B4      ; |1210| 
           LDW     .D2T1   *+SP(92),A0       ; |1210| 
           LDW     .D2T1   *+SP(132),A3      ; |1210| 
           NOP             3
           ADD     .L1X    A0,B4,A0          ; |1210| 
           STH     .D1T1   A4,*+A3[A0]       ; |1210| 
	.line	490
           LDW     .D2T2   *+SP(92),B4       ; |1211| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1211| 
           STW     .D2T2   B5,*+SP(92)       ; |1211| 
           LDW     .D2T2   *+SP(104),B4      ; |1211| 
           LDW     .D2T2   *+SP(168),B6      ; |1211| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1211| 
           ADDK    .S2     84,B4             ; |1211| 
           LDHU    .D2T2   *B4,B4            ; |1211| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L35               ; |1211| 
           NOP             5
           ; BRANCH OCCURS                   ; |1211| 
;** --------------------------------------------------------------------------*
L36:    
	.line	493
;------------------------------------------------------------------------------
; 1214 | dotPosOffset += parameters->wNumDots[d];                    // move off
;     | set into dot pos arrays so that it                                     
; 1215 | // points to loc after current target's dots                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1214| 
           LDW     .D2T2   *+SP(168),B6      ; |1214| 
           NOP             4
           ADDAH   .D2     B6,B4,B4          ; |1214| 
           ADDK    .S2     84,B4             ; |1214| 
           LDHU    .D2T2   *B4,B5            ; |1214| 
           LDW     .D2T2   *+SP(112),B4      ; |1214| 
           NOP             4
           ADD     .L2     B5,B4,B4          ; |1214| 
           STW     .D2T2   B4,*+SP(112)      ; |1214| 
	.line	495
           LDW     .D2T2   *+SP(104),B4      ; |1216| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1216| 
           STW     .D2T2   B4,*+SP(104)      ; |1216| 
           MV      .L1X    B6,A0
           LDHU    .D1T1   *+A0(10),A0       ; |1216| 
           NOP             4
           CMPLTU  .L1X    B4,A0,A1          ; 
   [ A1]   B       .S1     L11               ; |1216| 
           NOP             5
           ; BRANCH OCCURS                   ; |1216| 
;** --------------------------------------------------------------------------*
	.line	496
;------------------------------------------------------------------------------
; 1217 | }                                                                 // EN
;     | D:  process XYCORE_INIT command                                        
;------------------------------------------------------------------------------
           B       .S1     L144              ; |1217| 
           NOP             5
           ; BRANCH OCCURS                   ; |1217| 
;** --------------------------------------------------------------------------*
L37:    
	.line	499
;------------------------------------------------------------------------------
; 1220 | else if( command == XYCORE_DOFRAME )                              // BE
;     | GIN:  process XYCORE_DOFRAME command...                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(48),B4       ; |1220| 
           NOP             4
           CMPEQ   .L2     B4,3,B0           ; |1220| 
   [!B0]   B       .S1     L144              ; |1220| 
           NOP             5
           ; BRANCH OCCURS                   ; |1220| 
;** --------------------------------------------------------------------------*
	.line	501
;------------------------------------------------------------------------------
; 1222 | timdelnib = (UINT32) parameters->wDelayPerDot;                 // write
;     |  trig timing params to dotter board                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B4      ; |1222| 
           NOP             4
           LDHU    .D2T2   *+B4(12),B5       ; |1222| 
           NOP             4
           STW     .D2T2   B5,*+SP(64)       ; |1222| 
	.line	502
;------------------------------------------------------------------------------
; 1223 | if ( timdelnib > MAX_TRIGDEL )                                 // timin
;     | g register.  check values.                                             
;------------------------------------------------------------------------------
           CMPGTU  .L2     B5,15,B0          ; 
	.line	503
;------------------------------------------------------------------------------
; 1224 | timdelnib = MAX_TRIGDEL;                                               
;------------------------------------------------------------------------------
   [ B0]   MVK     .S1     15,A0             ; |1224| 
   [ B0]   STW     .D2T1   A0,*+SP(64)       ; |1224| 
	.line	504
;------------------------------------------------------------------------------
; 1225 | timdurbyte = timdelnib + ((UINT32) parameters->wOnTimePerDot);         
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+B4(14),B4       ; |1225| 
           LDW     .D2T2   *+SP(64),B5       ; |1225| 
           NOP             4
           ADD     .L2     B4,B5,B4          ; |1225| 
           STW     .D2T2   B4,*+SP(60)       ; |1225| 
	.line	505
;------------------------------------------------------------------------------
; 1226 | if ( timdurbyte > MAX_TRIGLEN )                                        
;------------------------------------------------------------------------------
           MVK     .S2     255,B6            ; |1226| 
           CMPGTU  .L2     B4,B6,B0          ; 
	.line	506
;------------------------------------------------------------------------------
; 1227 | timdurbyte = MAX_TRIGLEN;                                              
;------------------------------------------------------------------------------
   [ B0]   MVK     .S2     255,B4            ; |1227| 
   [ B0]   STW     .D2T2   B4,*+SP(60)       ; |1227| 
	.line	507
;------------------------------------------------------------------------------
; 1228 | timvals = ((timdurbyte & 0x00F0) << 20) | (timdelnib << 20) | ((timdurb
;     | yte & 0x000F) << 28);                                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(60),B4       ; |1228| 
           MVK     .S1     240,A0            ; |1228| 
           NOP             3
           AND     .L1X    A0,B4,A0          ; |1228| 

           AND     .L2     15,B4,B5          ; |1228| 
||         SHL     .S2     B5,20,B4          ; |1228| 
||         SHL     .S1     A0,20,A0          ; |1228| 

           OR      .L1X    B4,A0,A0          ; |1228| 
||         SHL     .S2     B5,28,B4          ; |1228| 

           OR      .L1X    B4,A0,A0          ; |1228| 
           STW     .D2T1   A0,*+SP(68)       ; |1228| 
	.line	508
;------------------------------------------------------------------------------
; 1229 | *timaddr = timvals;                                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(80),B4       ; |1229| 
           NOP             4
           STW     .D2T1   A0,*B4            ; |1229| 
	.line	510
;------------------------------------------------------------------------------
; 1231 | maxRepeats = 0;                                                // find 
;     | largest "#reps per frame" across all                                   
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1231| 
           STW     .D2T2   B4,*+SP(116)      ; |1231| 
	.line	511
;------------------------------------------------------------------------------
; 1232 | for( d = 0; d < parameters->wNumTargets; d++ )                 // defin
;     | ed targets.  B/c "dotlife" tgts use the                                
; 1233 | {                                                              // upper
;     |  byte of NREPS field to store dot life                                 
;------------------------------------------------------------------------------
           ZERO    .L1     A0                ; |1232| 
           STW     .D2T1   A0,*+SP(104)      ; |1232| 
           LDW     .D2T2   *+SP(168),B4      ; |1232| 
           NOP             4
           LDHU    .D2T2   *+B4(10),B4       ; |1232| 
           NOP             4
           CMPLTU  .L1X    A0,B4,A1          ; 
   [!A1]   B       .S1     L39               ; |1232| 
           NOP             5
           ; BRANCH OCCURS                   ; |1232| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L38:    
	.line	513
;------------------------------------------------------------------------------
; 1234 | cd = d * UPDRECSZ;                                          // decremen
;     | t, we must mask that out here!                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A0      ; |1234| 
           NOP             4
           ADDAW   .D1     A0,A0,A3          ; |1234| 
           STW     .D2T1   A3,*+SP(108)      ; |1234| 
	.line	514
;------------------------------------------------------------------------------
; 1235 | u16Dummy = (UINT16) data[cd + NREPS];                                  
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(200),A4      ; |1235| 
           ADD     .L1     4,A3,A3           ; 
           NOP             3
           LDHU    .D1T1   *+A4[A3],A3       ; |1235| 
           NOP             4
           STH     .D2T1   A3,*+SP(38)       ; |1235| 
	.line	515
;------------------------------------------------------------------------------
; 1236 | if( parameters->wType[d] == DOTLIFEWIN ||                              
; 1237 | parameters->wType[d] == DL_NOISEDIR ||                                 
; 1238 | parameters->wType[d] == DL_NOISESPEED )                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1236| 
           LDW     .D2T2   *+SP(168),B4      ; |1236| 
           NOP             4
           ADDAH   .D2     B4,B5,B5          ; |1236| 
           LDHU    .D2T2   *+B5(20),B5       ; |1236| 
           NOP             4
           CMPEQ   .L2     B5,8,B0           ; |1236| 
   [!B0]   LDW     .D2T1   *+SP(104),A4      ; |1236| 
   [!B0]   LDW     .D2T1   *+SP(168),A3      ; |1236| 
           NOP             4
   [!B0]   ADDAH   .D1     A3,A4,A3          ; |1236| 
   [!B0]   LDHU    .D1T1   *+A3(20),A3       ; |1236| 
           NOP             4
   [!B0]   CMPEQ   .L2X    A3,11,B0          ; |1236| 
   [!B0]   LDW     .D2T1   *+SP(168),A3      ; |1236| 
   [!B0]   LDW     .D2T1   *+SP(104),A4      ; |1236| 
           NOP             4
   [!B0]   ADDAH   .D1     A3,A4,A3          ; |1236| 
   [!B0]   LDHU    .D1T1   *+A3(20),A3       ; |1236| 
           NOP             4
   [!B0]   CMPEQ   .L2X    A3,13,B0          ; |1236| 
	.line	518
;------------------------------------------------------------------------------
; 1239 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T2   *+SP(38),B5       ; |1239| 
           NOP             4
   [ B0]   EXTU    .S2     B5,24,24,B5       ; |1239| 
   [ B0]   STH     .D2T2   B5,*+SP(38)       ; |1239| 
	.line	519
;------------------------------------------------------------------------------
; 1240 | if ( ((UINT32) u16Dummy) > maxRepeats )                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(116),B5      ; |1240| 
           LDHU    .D2T2   *+SP(38),B6       ; |1240| 
           NOP             4
           CMPGTU  .L2     B6,B5,B0          ; |1240| 
	.line	520
;------------------------------------------------------------------------------
; 1241 | maxRepeats = (UINT32) u16Dummy;                                        
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T2   *+SP(38),B5       ; |1241| 
           NOP             4
   [ B0]   STW     .D2T2   B5,*+SP(116)      ; |1241| 
	.line	521
;------------------------------------------------------------------------------
; 1244 | // these are only used by DL_NOISEDIR and DL_NOISESPEED. We put the val
;     | ues in local vars so we're not                                         
; 1245 | // constantly accessing the PARAMETERS struct in shared memory (slower 
;     | access)                                                                
;------------------------------------------------------------------------------
           ADD     .L1     1,A0,A3           ; 
           STW     .D2T1   A3,*+SP(104)      ; |1242| 
           MV      .L1X    B4,A0
           LDHU    .D1T1   *+A0(10),A0       ; |1242| 
           NOP             4
           CMPLTU  .L1     A3,A0,A1          ; 
   [ A1]   B       .S1     L38               ; |1242| 
           NOP             5
           ; BRANCH OCCURS                   ; |1242| 
;** --------------------------------------------------------------------------*
L39:    
	.line	525
;------------------------------------------------------------------------------
; 1246 | screenW_mm = parameters->wWidthMM;                                     
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A0      ; |1246| 
           NOP             4
           LDHU    .D1T1   *+A0(4),A3        ; |1246| 
           NOP             4
           STH     .D2T1   A3,*+SP(32)       ; |1246| 
	.line	526
;------------------------------------------------------------------------------
; 1247 | screenH_mm = parameters->wHeightMM;                                    
;------------------------------------------------------------------------------
           LDHU    .D1T1   *+A0(6),A3        ; |1247| 
           NOP             4
           STH     .D2T1   A3,*+SP(34)       ; |1247| 
	.line	528
;------------------------------------------------------------------------------
; 1249 | dotPosOffset = 0;                                              // BEGIN
;     | :  first pass thru all targets                                         
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1249| 
           STW     .D2T2   B4,*+SP(112)      ; |1249| 
	.line	529
;------------------------------------------------------------------------------
; 1250 | nTotalVisDots = 0;                                                     
;------------------------------------------------------------------------------
           ZERO    .L1     A12               ; |1250| 
	.line	530
;------------------------------------------------------------------------------
; 1251 | if( maxRepeats > 0 ) for( d = 0; d < parameters->wNumTargets; d++ )    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(116),B0      ; |1251| 
           NOP             4
   [!B0]   B       .S1     L135              ; |1251| 
           NOP             5
           ; BRANCH OCCURS                   ; |1251| 
;** --------------------------------------------------------------------------*
           ZERO    .L1     A3                ; |1251| 
           STW     .D2T1   A3,*+SP(104)      ; |1251| 
           LDHU    .D1T1   *+A0(10),A0       ; |1251| 
           NOP             4
           CMPLTU  .L1     A3,A0,A1          ; 
   [!A1]   B       .S1     L135              ; |1251| 
           NOP             5
           ; BRANCH OCCURS                   ; |1251| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L40
;** --------------------------------------------------------------------------*
L40:    
	.line	532
;------------------------------------------------------------------------------
; 1253 | cd = d * UPDRECSZ;                                          // offset i
;     | nto array of motion update records;                                    
; 1254 | // locates start of record for this tgt                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1253| 
           NOP             4
           ADDAW   .D2     B4,B4,B4          ; |1253| 
           STW     .D2T2   B4,*+SP(108)      ; |1253| 
	.line	534
;------------------------------------------------------------------------------
; 1255 | u16Type = parameters->wType[d];                             // this tar
;     | get's type                                                             
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A0      ; |1255| 
           LDW     .D2T1   *+SP(168),A3      ; |1255| 
           NOP             4
           ADDAH   .D1     A3,A0,A0          ; |1255| 
           LDHU    .D1T1   *+A0(20),A0       ; |1255| 
           NOP             4
           STH     .D2T1   A0,*+SP(36)       ; |1255| 
	.line	536
;------------------------------------------------------------------------------
; 1257 | u16Dummy = (UINT16) data[cd + NREPS];                       // #reps pe
;     | r frame for this tgt (mask out dot                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(200),B5      ; |1257| 
           ADD     .L2     4,B4,B4           ; 
           NOP             3
           LDHU    .D2T2   *+B5[B4],B4       ; |1257| 
           NOP             4
           STH     .D2T2   B4,*+SP(38)       ; |1257| 
	.line	537
;------------------------------------------------------------------------------
; 1258 | if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // life dec
;     | r in upper byte if "dotlife" type)                                     
; 1259 | u16Type == DL_NOISESPEED )                                             
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1258| 
           NOP             4
           CMPEQ   .L2     B4,8,B0           ; |1258| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1258| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,11,B0          ; |1258| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1258| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,13,B0          ; |1258| 
	.line	539
;------------------------------------------------------------------------------
; 1260 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T2   *+SP(38),B4       ; |1260| 
           NOP             4
   [ B0]   EXTU    .S2     B4,24,24,B4       ; |1260| 
   [ B0]   STH     .D2T2   B4,*+SP(38)       ; |1260| 
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
           LDHU    .D2T2   *+SP(36),B4       ; |1262| 
           NOP             4
           CMPEQ   .L2     B4,0,B0           ; |1262| 
   [!B0]   LDW     .D2T2   *+SP(104),B4      ; |1262| 
   [!B0]   LDW     .D2T2   *+SP(168),B5      ; |1262| 
           NOP             4
   [!B0]   ADDAH   .D2     B5,B4,B4          ; |1262| 
   [!B0]   ADDK    .S2     84,B4             ; |1262| 
   [!B0]   LDHU    .D2T2   *B4,B4            ; |1262| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,0,B0           ; |1262| 
           CMPEQ   .L2     B0,0,B4           ; 
           XOR     .L2     1,B4,B0           ; |1262| 
   [!B0]   LDHU    .D2T2   *+SP(38),B4       ; |1262| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,0,B0           ; |1262| 
   [ B0]   B       .S1     L41               ; |1262| 
   [ B0]   MV      .L1X    B0,A15            ; |1262| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1262| 
           NOP             3
           ; BRANCH OCCURS                   ; |1262| 
;** --------------------------------------------------------------------------*
           NOP             4
           CMPEQ   .L2     B4,10,B0          ; |1262| 
   [ B0]   LDW     .D2T2   *+SP(168),B5      ; |1262| 
   [ B0]   LDW     .D2T2   *+SP(104),B4      ; |1262| 
           NOP             4
   [ B0]   ADDAH   .D2     B5,B4,B4          ; |1262| 
   [ B0]   ADDK    .S2     212,B4            ; |1262| 
   [ B0]   LDHU    .D2T2   *B4,B4            ; |1262| 
           NOP             4
   [ B0]   CMPEQ   .L2     B4,0,B0           ; |1262| 
           MV      .L1X    B0,A15            ; 
;** --------------------------------------------------------------------------*
L41:    
           MV      .L1     A15,A1            ; |1262| 
	.line	545
;------------------------------------------------------------------------------
; 1266 | nRedrawsLeft[d] = 0;                                                   
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(104),B5      ; |1266| 
   [ A1]   LDW     .D2T2   *+SP(180),B4      ; |1266| 
   [ A1]   ZERO    .L2     B6                ; |1266| 
           NOP             3
   [ A1]   STH     .D2T2   B6,*+B4[B5]       ; |1266| 
	.line	546
;------------------------------------------------------------------------------
; 1267 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(104),B5      ; |1267| 
   [ A1]   LDW     .D2T2   *+SP(184),B4      ; |1267| 
   [ A1]   ZERO    .L2     B6                ; |1267| 
           NOP             3
   [ A1]   STH     .D2T2   B6,*+B4[B5]       ; |1267| 
	.line	547
;------------------------------------------------------------------------------
; 1268 | dotPosOffset += parameters->wNumDots[d];                               
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T2   *+SP(104),B5      ; |1268| 
   [ A1]   LDW     .D2T2   *+SP(168),B4      ; |1268| 
           NOP             4
   [ A1]   ADDAH   .D2     B4,B5,B4          ; |1268| 
   [ A1]   ADDK    .S2     84,B4             ; |1268| 
   [ A1]   LDHU    .D2T2   *B4,B5            ; |1268| 
   [ A1]   LDW     .D2T2   *+SP(112),B4      ; |1268| 
           NOP             4
   [ A1]   ADD     .L2     B5,B4,B4          ; |1268| 
   [ A1]   STW     .D2T2   B4,*+SP(112)      ; |1268| 
	.line	548
;------------------------------------------------------------------------------
; 1269 | continue;                                                              
;------------------------------------------------------------------------------
   [ A1]   B       .S1     L134              ; |1269| 
           NOP             5
           ; BRANCH OCCURS                   ; |1269| 
;** --------------------------------------------------------------------------*
	.line	551
;------------------------------------------------------------------------------
; 1272 | hw = (INT16) data[cd + WIN_H];                              // target's
;     |  window pos change for current frame                                   
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B5      ; |1272| 
           LDW     .D2T2   *+SP(200),B4      ; |1272| 
           NOP             4
           LDH     .D2T2   *+B4[B5],B6       ; |1272| 
           NOP             4
           STH     .D2T2   B6,*+SP(8)        ; |1272| 
	.line	552
;------------------------------------------------------------------------------
; 1273 | vw = (INT16) data[cd + WIN_V];                                         
;------------------------------------------------------------------------------
           ADD     .L2     1,B5,B6           ; 
           LDH     .D2T2   *+B4[B6],B6       ; |1273| 
           NOP             4
           STH     .D2T2   B6,*+SP(10)       ; |1273| 
	.line	553
;------------------------------------------------------------------------------
; 1274 | hv = (INT16) data[cd + PAT_H];                              // tgt's pa
;     | ttern pos change for current frame                                     
;------------------------------------------------------------------------------
           ADD     .L2     2,B5,B6           ; 
           LDH     .D2T2   *+B4[B6],B6       ; |1274| 
           NOP             4
           STH     .D2T2   B6,*+SP(4)        ; |1274| 
	.line	554
;------------------------------------------------------------------------------
; 1275 | vv = (INT16) data[cd + PAT_V];                                         
; 1276 | // deal with special cases:                                            
;------------------------------------------------------------------------------
           ADD     .L2     3,B5,B5           ; 
           LDH     .D2T2   *+B4[B5],B4       ; |1275| 
           NOP             4
           STH     .D2T2   B4,*+SP(6)        ; |1275| 
	.line	556
;------------------------------------------------------------------------------
; 1277 | if( u16Type == STATICANNU )                                 //    STATI
;     | CANNU - no window or pattern motion                                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1277| 
           NOP             4
           CMPEQ   .L2     B4,6,B0           ; |1277| 
	.line	557
;------------------------------------------------------------------------------
; 1278 | hw = vw = hv = vv = 0;                                                 
;------------------------------------------------------------------------------
   [ B0]   ZERO    .L1     A3                ; 

   [ B0]   STH     .D2T1   A3,*+SP(4)        ; |1278| 
|| [ B0]   ZERO    .L1     A0                ; 

   [ B0]   STH     .D2T1   A0,*+SP(8)        ; |1278| 

   [ B0]   STH     .D2T1   A0,*+SP(10)       ; |1278| 
|| [ B0]   ZERO    .L1     A3                ; 

   [ B0]   STH     .D2T1   A3,*+SP(6)        ; |1278| 
	.line	558
;------------------------------------------------------------------------------
; 1279 | if( u16Type == FULLSCREEN )                                 //    FULLS
;     | CREEN - no window                                                      
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1279| 
           NOP             4
           CMPEQ   .L2     B4,2,B0           ; |1279| 
	.line	559
;------------------------------------------------------------------------------
; 1280 | hw = vw = 0;                                                           
;------------------------------------------------------------------------------
   [ B0]   ZERO    .L2     B4                ; |1280| 
   [ B0]   STH     .D2T2   B4,*+SP(10)       ; |1280| 
   [ B0]   STH     .D2T2   B4,*+SP(8)        ; |1280| 
	.line	560
;------------------------------------------------------------------------------
; 1281 | if( (u16Type == DOTARRAY) || (u16Type == ORIBAR) )          //    DOTAR
;     | RAY/ORIBAR - dots move together as                                     
; 1282 | {                                                           //    an ob
;     | ject. there's no window or pattern                                     
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1281| 
           NOP             4
           CMPEQ   .L2     B4,1,B0           ; |1281| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1281| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,10,B0          ; |1281| 
	.line	562
;------------------------------------------------------------------------------
; 1283 | hv = hw;                                                 //    in the s
;     | ense of the windowed tgt types;                                        
;------------------------------------------------------------------------------
   [ B0]   LDH     .D2T2   *+SP(8),B4        ; |1283| 
           NOP             4
   [ B0]   STH     .D2T2   B4,*+SP(4)        ; |1283| 
	.line	563
;------------------------------------------------------------------------------
; 1284 | vv = vw;                                                 //    all dots
;     |  drawn. assign "window" to                                             
;------------------------------------------------------------------------------
   [ B0]   LDH     .D2T2   *+SP(10),B4       ; |1284| 
           NOP             4
   [ B0]   STH     .D2T2   B4,*+SP(6)        ; |1284| 
	.line	564
;------------------------------------------------------------------------------
; 1285 | hw = vw = 0;                                             //    "pattern
;     | " vel, so we can implement like                                        
; 1286 | }                                                           //    the F
;     | ULLSCREEN tgt type (see below)                                         
;------------------------------------------------------------------------------
   [ B0]   ZERO    .L2     B4                ; |1285| 
   [ B0]   STH     .D2T2   B4,*+SP(8)        ; |1285| 
   [ B0]   STH     .D2T2   B4,*+SP(10)       ; |1285| 
	.line	567
;------------------------------------------------------------------------------
; 1288 | if( u16Type != OPTICFLOW )                                  // update t
;     | arget window location.                                                 
; 1289 | {                                                           // !!! UINT
;     | 16 arithmetic! Windows wrap around                                     
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1288| 
           NOP             4
           CMPEQ   .L2     B4,9,B0           ; |1288| 
   [ B0]   B       .S1     L42               ; |1288| 
           NOP             5
           ; BRANCH OCCURS                   ; |1288| 
;** --------------------------------------------------------------------------*
	.line	569
;------------------------------------------------------------------------------
; 1290 | parameters->wRectR[d] += hw;                             // screen on D
;     | etroit/Daytona.  Won't happen on                                       
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A3      ; |1290| 
           LDW     .D2T1   *+SP(104),A0      ; |1290| 
           MVK     .S2     148,B4            ; |1290| 
           NOP             3
           ADDAH   .D1     A3,A0,A0          ; |1290| 
           ADD     .L1X    B4,A0,A0          ; |1290| 
           LDHU    .D1T1   *A0,A3            ; |1290| 
           LDH     .D2T2   *+SP(8),B4        ; |1290| 
           NOP             4
           ADD     .L1X    B4,A3,A3          ; |1290| 
           STH     .D1T1   A3,*A0            ; |1290| 
	.line	570
;------------------------------------------------------------------------------
; 1291 | parameters->wRectL[d] += hw;                             // Dakar, b/c 
;     | UINT16 is actually UINT32.  It is                                      
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A0      ; |1291| 
           LDW     .D2T1   *+SP(168),A3      ; |1291| 
           LDH     .D2T2   *+SP(8),B4        ; |1291| 
           NOP             3

           ADDAH   .D1     A3,A0,A0          ; |1291| 
||         MVK     .S1     212,A3            ; |1291| 

           ADD     .L1     A3,A0,A0          ; |1291| 
           LDHU    .D1T1   *A0,A3            ; |1291| 
           NOP             4
           ADD     .L1X    B4,A3,A3          ; |1291| 
           STH     .D1T1   A3,*A0            ; |1291| 
	.line	571
;------------------------------------------------------------------------------
; 1292 | parameters->wRectT[d] += vw;                             // considered 
;     | an error on user's part to have a                                      
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1292| 
           LDW     .D2T2   *+SP(168),B5      ; |1292| 
           MVK     .S1     276,A0            ; |1292| 
           NOP             3
           ADDAH   .D2     B5,B4,B4          ; |1292| 
           ADD     .L1X    A0,B4,A0          ; |1292| 
           LDHU    .D1T1   *A0,A3            ; |1292| 
           LDH     .D2T2   *+SP(10),B4       ; |1292| 
           NOP             4
           ADD     .L1X    B4,A3,A3          ; |1292| 
           STH     .D1T1   A3,*A0            ; |1292| 
	.line	572
;------------------------------------------------------------------------------
; 1293 | parameters->wRectB[d] += vw;                             // target wind
;     | ow go past screen bounds!!!                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1293| 
           LDW     .D2T2   *+SP(104),B4      ; |1293| 
           LDH     .D2T2   *+SP(10),B6       ; |1293| 
           NOP             3

           ADDAH   .D2     B5,B4,B4          ; |1293| 
||         MVK     .S2     340,B5            ; |1293| 

           ADD     .L2     B5,B4,B4          ; |1293| 
           LDHU    .D2T2   *B4,B5            ; |1293| 
           NOP             4
           ADD     .L2     B6,B5,B5          ; |1293| 
           STH     .D2T2   B5,*B4            ; |1293| 
	.line	574
;------------------------------------------------------------------------------
; 1295 | rectR = parameters->wRectR[d];                           // save curren
;     | t window bounds in register vars                                       
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A3      ; |1295| 
           LDW     .D2T1   *+SP(104),A0      ; |1295| 
           NOP             4
           ADDAH   .D1     A3,A0,A4          ; |1295| 
           ADDK    .S1     148,A4            ; |1295| 
           LDHU    .D1T1   *A4,A8            ; |1295| 
           NOP             4
	.line	575
;------------------------------------------------------------------------------
; 1296 | rectL = parameters->wRectL[d];                           // to speed up
;     |  comparisons which must be                                             
;------------------------------------------------------------------------------
           ADDAH   .D1     A3,A0,A4          ; |1296| 
           ADDK    .S1     212,A4            ; |1296| 
           LDHU    .D1T2   *A4,B8            ; |1296| 
           NOP             4
	.line	576
;------------------------------------------------------------------------------
; 1297 | rectU = parameters->wRectT[d];                           // performed f
;     | or all dots                                                            
;------------------------------------------------------------------------------
           ADDAH   .D1     A3,A0,A4          ; |1297| 
           ADDK    .S1     276,A4            ; |1297| 
           LDHU    .D1T1   *A4,A13           ; |1297| 
           NOP             4
	.line	577
;------------------------------------------------------------------------------
; 1298 | rectD = parameters->wRectB[d];                                         
;------------------------------------------------------------------------------
           ADDAH   .D1     A3,A0,A4          ; |1298| 
           ADDK    .S1     340,A4            ; |1298| 
           LDHU    .D1T2   *A4,B9            ; |1298| 
           NOP             4
	.line	579
;------------------------------------------------------------------------------
; 1300 | if( u16Type == ANNULUS )                                 // must update
;     |  outer rect as well for ANNULUS;                                       
; 1301 | {                                                        // note that w
;     | e DO NOT assign register vars to                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1300| 
           NOP             4
           CMPEQ   .L2     B4,5,B0           ; |1300| 
   [!B0]   B       .S1     L43               ; |1300| 
           NOP             5
           ; BRANCH OCCURS                   ; |1300| 
;** --------------------------------------------------------------------------*
	.line	581
;------------------------------------------------------------------------------
; 1302 | parameters->wOuterR[d] += hw;                         // the bounds of 
;     | the outer rect.                                                        
;------------------------------------------------------------------------------

           MVK     .S2     404,B4            ; |1302| 
||         ADDAH   .D1     A3,A0,A0          ; |1302| 

           ADD     .L1X    B4,A0,A0          ; |1302| 
           LDHU    .D1T1   *A0,A3            ; |1302| 
           LDH     .D2T2   *+SP(8),B4        ; |1302| 
           NOP             4
           ADD     .L1X    B4,A3,A3          ; |1302| 
           STH     .D1T1   A3,*A0            ; |1302| 
	.line	582
;------------------------------------------------------------------------------
; 1303 | parameters->wOuterL[d] += hw;                                          
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(168),A0      ; |1303| 
           LDW     .D2T1   *+SP(104),A3      ; |1303| 
           LDH     .D2T2   *+SP(8),B4        ; |1303| 
           NOP             3

           ADDAH   .D1     A0,A3,A0          ; |1303| 
||         MVK     .S1     468,A3            ; |1303| 

           ADD     .L1     A3,A0,A3          ; |1303| 
           LDHU    .D1T1   *A3,A0            ; |1303| 
           NOP             4
           ADD     .L1X    B4,A0,A0          ; |1303| 
           STH     .D1T1   A0,*A3            ; |1303| 
	.line	583
;------------------------------------------------------------------------------
; 1304 | parameters->wOuterT[d] += vw;                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1304| 
           LDW     .D2T2   *+SP(168),B4      ; |1304| 
           MVK     .S1     532,A0            ; |1304| 
           NOP             3
           ADDAH   .D2     B4,B5,B4          ; |1304| 
           ADD     .L1X    A0,B4,A0          ; |1304| 
           LDHU    .D1T1   *A0,A3            ; |1304| 
           LDH     .D2T2   *+SP(10),B4       ; |1304| 
           NOP             4
           ADD     .L1X    B4,A3,A3          ; |1304| 
           STH     .D1T1   A3,*A0            ; |1304| 
	.line	584
;------------------------------------------------------------------------------
; 1305 | parameters->wOuterB[d] += vw;                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1305| 
           LDW     .D2T2   *+SP(168),B4      ; |1305| 
           NOP             4

           ADDAH   .D2     B4,B5,B4          ; |1305| 
||         MVK     .S2     596,B5            ; |1305| 

           ADD     .L2     B5,B4,B6          ; |1305| 
           LDHU    .D2T2   *B6,B5            ; |1305| 
           LDH     .D2T2   *+SP(10),B4       ; |1305| 
           NOP             4
           ADD     .L2     B4,B5,B4          ; |1305| 
           STH     .D2T2   B4,*B6            ; |1305| 
	.line	586
;------------------------------------------------------------------------------
; 1308 | else                                                        // OPTICFLO
;     | W target is very different: window                                     
; 1309 | {                                                           // is moved
;     |  by changing coords of the FOE...                                      
;------------------------------------------------------------------------------
           B       .S1     L43               ; |1307| 
           NOP             5
           ; BRANCH OCCURS                   ; |1307| 
;** --------------------------------------------------------------------------*
L42:    
	.line	589
;------------------------------------------------------------------------------
; 1310 | rectR = (UINT16) parameters->wRectR[d];                  //    inner ra
;     | dius in visual deg/100                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1310| 
           LDW     .D2T2   *+SP(168),B5      ; |1310| 
           NOP             4
           ADDAH   .D2     B5,B4,B6          ; |1310| 
           ADDK    .S2     148,B6            ; |1310| 
           LDHU    .D2T1   *B6,A8            ; |1310| 
           NOP             4
	.line	590
;------------------------------------------------------------------------------
; 1311 | rectL = (UINT16) parameters->wRectL[d];                  //    outer ra
;     | dius in visual deg/100                                                 
;------------------------------------------------------------------------------
           MV      .L1X    B4,A3
           MV      .L1X    B5,A0
           ADDAH   .D1     A0,A3,A0          ; |1311| 
           ADDK    .S1     212,A0            ; |1311| 
           LDHU    .D1T2   *A0,B8            ; |1311| 
           NOP             4
	.line	591
;------------------------------------------------------------------------------
; 1312 | rectU = (UINT16) parameters->wRectT[d];                  //    alphaX g
;     | eometric conversion fac (* 1024)                                       
;------------------------------------------------------------------------------
           MV      .L1X    B5,A3
           MV      .L1X    B4,A0
           ADDAH   .D1     A3,A0,A0          ; |1312| 
           ADDK    .S1     276,A0            ; |1312| 
           LDHU    .D1T1   *A0,A13           ; |1312| 
           NOP             4
	.line	592
;------------------------------------------------------------------------------
; 1313 | rectD = (UINT16) parameters->wRectB[d];                  //    alphaY g
;     | eometric conversion fac (* 1024)                                       
;------------------------------------------------------------------------------
           MV      .L1X    B4,A0
           ADDAH   .D1     A3,A0,A0          ; |1313| 
           ADDK    .S1     340,A0            ; |1313| 
           LDHU    .D1T2   *A0,B9            ; |1313| 
           NOP             4
	.line	594
;------------------------------------------------------------------------------
; 1315 | xCoord = parameters->wOuterR[d];                         //    update c
;     | oords of the FOE now...                                                
;------------------------------------------------------------------------------
           ADDAH   .D2     B5,B4,B6          ; |1315| 
           ADDK    .S2     404,B6            ; |1315| 
           LDHU    .D2T1   *B6,A10           ; |1315| 
           NOP             4
	.line	595
;------------------------------------------------------------------------------
; 1316 | xCoord += hw;                                                          
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B6        ; |1316| 
           NOP             4
           ADD     .L1X    B6,A10,A0         ; |1316| 
           EXTU    .S1     A0,16,16,A10      ; |1316| 
	.line	596
;------------------------------------------------------------------------------
; 1317 | yCoord = parameters->wOuterL[d];                                       
;------------------------------------------------------------------------------
           ADDAH   .D2     B5,B4,B6          ; |1317| 
           ADDK    .S2     468,B6            ; |1317| 
           LDHU    .D2T1   *B6,A11           ; |1317| 
           NOP             4
	.line	597
;------------------------------------------------------------------------------
; 1318 | yCoord += vw;                                                          
; 1319 | #ifdef _TGTDAKARF5                                       //    ensure U
;     | INT16 arith on 32-bit-only DAKAR                                       
; 1320 | if( xCoord > 0x0000FFFF )                                              
; 1322 | if( hw > 0 ) xCoord -= 65536;                                          
; 1323 | else         xCoord &= 0x0000FFFF;                                     
; 1325 | if( yCoord > 0x0000FFFF )                                              
; 1327 | if( vw > 0 ) yCoord -= 65536;                                          
; 1328 | else         yCoord &= 0x0000FFFF;                                     
; 1330 | #endif                                                                 
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(10),B6       ; |1318| 
           NOP             4
           ADD     .L1X    B6,A11,A0         ; |1318| 
           EXTU    .S1     A0,16,16,A11      ; |1318| 
	.line	610
;------------------------------------------------------------------------------
; 1331 | parameters->wOuterR[d] = xCoord;                         //    the new 
;     | FOE coords are also preserved                                          
;------------------------------------------------------------------------------
           MV      .L1X    B4,A0
           ADDAH   .D1     A3,A0,A0          ; |1331| 
           ADDK    .S1     404,A0            ; |1331| 
           STH     .D1T1   A10,*A0           ; |1331| 
	.line	611
;------------------------------------------------------------------------------
; 1332 | parameters->wOuterL[d] = yCoord;                         //    in the r
;     | egister vars (xCoord, yCoord)...                                       
; 1333 | //    also, reg vars (hv,vv) = (B*2^M, M)!                             
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1332| 
           LDW     .D2T2   *+SP(168),B5      ; |1332| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1332| 
           ADDK    .S2     468,B4            ; |1332| 
           STH     .D2T1   A11,*B4           ; |1332| 
;** --------------------------------------------------------------------------*
L43:    
	.line	615
;------------------------------------------------------------------------------
; 1336 | de = &xdotpos[ dotPosOffset + parameters->wNumDots[d] ];    // set ptrs
;     |  into target's dot position, dot                                       
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A4      ; |1336| 
           LDW     .D2T1   *+SP(168),A5      ; |1336| 
           LDW     .D2T2   *+SP(112),B6      ; |1336| 
           NOP             3
           ADDAH   .D1     A5,A4,A0          ; |1336| 
           ADDK    .S1     84,A0             ; |1336| 
           LDHU    .D1T1   *A0,A3            ; |1336| 
           LDW     .D2T1   *+SP(128),A0      ; |1336| 
           NOP             3
           ADD     .L1X    A3,B6,A3          ; |1336| 
           ADDAH   .D1     A0,A3,A14         ; |1336| 
	.line	616
;------------------------------------------------------------------------------
; 1337 | a  = &xdotpos[dotPosOffset];                                // lifetime
;     | , dot noise, and fracDX,DY arrays                                      
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(112),A3      ; 
           NOP             4
           ADDAH   .D1     A0,A3,A9          ; |1337| 
	.line	617
;------------------------------------------------------------------------------
; 1338 | b  = &ydotpos[dotPosOffset];                                           
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(132),A6      ; |1338| 
           MV      .L1     A3,A0
           NOP             3
           ADDAH   .D1     A6,A0,A7          ; |1338| 
	.line	618
;------------------------------------------------------------------------------
; 1339 | nextDotLife = &dotLife[dotPosOffset];                                  
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(136),A6      ; |1339| 
           NOP             4
           ADDAH   .D1     A6,A0,A0          ; |1339| 
           STW     .D2T1   A0,*+SP(140)      ; |1339| 
	.line	619
;------------------------------------------------------------------------------
; 1340 | nextDotNoise = &dotNoise[dotPosOffset];                                
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(144),A6      ; |1340| 
           MV      .L1     A3,A0
           NOP             3
           ADDAH   .D1     A6,A0,A0          ; |1340| 
           STW     .D2T1   A0,*+SP(148)      ; |1340| 
	.line	620
;------------------------------------------------------------------------------
; 1341 | nextFracDX = &fracDX[dotPosOffset];                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(152),B5      ; |1341| 
           MV      .L2X    A3,B4
           NOP             3
           ADDAH   .D2     B5,B4,B4          ; |1341| 
           STW     .D2T2   B4,*+SP(156)      ; |1341| 
	.line	621
;------------------------------------------------------------------------------
; 1342 | nextFracDY = &fracDY[dotPosOffset];                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(160),B4      ; |1342| 
           MV      .L2X    A3,B5
           NOP             3
           ADDAH   .D2     B4,B5,B4          ; |1342| 
           STW     .D2T2   B4,*+SP(164)      ; |1342| 
	.line	622
;------------------------------------------------------------------------------
; 1343 | dotPosOffset += parameters->wNumDots[d];                    // now poin
;     | ts to start of next target's dots                                      
;------------------------------------------------------------------------------
           ADDAH   .D1     A5,A4,A0          ; |1343| 
           ADDK    .S1     84,A0             ; |1343| 
           LDHU    .D1T1   *A0,A0            ; |1343| 
           NOP             4
           ADD     .L2X    A0,B6,B4          ; 
           STW     .D2T2   B4,*+SP(112)      ; |1343| 
	.line	625
;------------------------------------------------------------------------------
; 1346 | if( (u16Type == DOTARRAY) ||                                // DOTARRAY
;     | /FULLSCREEN/ORIBAR:  Every tgt dot                                     
; 1347 | (u16Type == FULLSCREEN) ||                              // is always dr
;     | awn -- there's no "window" that                                        
; 1348 | (u16Type == ORIBAR) )                                   // is distinct 
;     | from the dot pattern.                                                  
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1346| 
           NOP             4
           CMPEQ   .L2     B4,1,B0           ; |1346| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1346| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,2,B0           ; |1346| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |1346| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,10,B0          ; |1346| 
   [!B0]   B       .S1     L47               ; |1346| 
           NOP             5
           ; BRANCH OCCURS                   ; |1346| 
;** --------------------------------------------------------------------------*
	.line	629
;------------------------------------------------------------------------------
; 1350 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    #dots in
;     |  "visible dots" array for tgt                                          
;------------------------------------------------------------------------------
           ADDAH   .D1     A5,A4,A0          ; |1350| 
           ADDK    .S1     84,A0             ; |1350| 
           LDHU    .D1T1   *A0,A0            ; |1350| 
           LDW     .D2T2   *+SP(184),B5      ; |1350| 
           MV      .L2X    A4,B4             ; |1350| 
           NOP             3
           STH     .D2T1   A0,*+B5[B4]       ; |1350| 
	.line	630
;------------------------------------------------------------------------------
; 1351 | while( a < de )                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1351| 
   [!A1]   B       .S1     L133              ; |1351| 
           NOP             5
           ; BRANCH OCCURS                   ; |1351| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L44
;** --------------------------------------------------------------------------*
L44:    
	.line	632
;------------------------------------------------------------------------------
; 1353 | *a = *a + hv;                                                          
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(4),B4        ; |1353| 
||         LDHU    .D1T1   *A9,A0            ; |1353| 

           NOP             4
           ADD     .L1X    B4,A0,A0          ; |1353| 
           STH     .D1T1   A0,*A9            ; |1353| 
	.line	633
;------------------------------------------------------------------------------
; 1354 | *b = *b + vv;                                                          
; 1355 | #ifdef _TGTDAKARF5                                                     
; 1356 | if( *a > 0x0000FFFF )                                                  
; 1358 | if( hv > 0 ) *a -= 65536;                                              
; 1359 | else         *a &= 0x0000FFFF;                                         
; 1361 | if( *b > 0x0000FFFF )                                                  
; 1363 | if( vv > 0 ) *b -= 65536;                                              
; 1364 | else         *b &= 0x0000FFFF;                                         
; 1366 | #endif                                                                 
;------------------------------------------------------------------------------

           LDHU    .D1T1   *A7,A0            ; |1354| 
||         LDH     .D2T2   *+SP(6),B4        ; |1354| 

           NOP             4
           ADD     .L1X    B4,A0,A0          ; |1354| 
           STH     .D1T1   A0,*A7            ; |1354| 
	.line	647
;------------------------------------------------------------------------------
; 1368 | xyvals = ((*a << 16) | *b);                           //    draw the do
;     | t                                                                      
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1368| 
           LDHU    .D1T1   *A7,A3            ; |1368| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1368| 
           OR      .L1     A3,A0,A0          ; |1368| 
           STW     .D2T1   A0,*+SP(56)       ; |1368| 
	.line	648
;------------------------------------------------------------------------------
; 1369 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1369| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1369| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1369| 
   [!B0]   B       .S1     L46               ; |1369| 
           NOP             5
           ; BRANCH OCCURS                   ; |1369| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L45:    
           LDW     .D2T2   *+SP(76),B4       ; |1369| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1369| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1369| 
   [ B0]   B       .S1     L45               ; |1369| 
           NOP             5
           ; BRANCH OCCURS                   ; |1369| 
;** --------------------------------------------------------------------------*
L46:    
	.line	649
;------------------------------------------------------------------------------
; 1370 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1370| 
           LDW     .D2T2   *+SP(72),B5       ; |1370| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1370| 
	.line	650
;------------------------------------------------------------------------------
; 1371 | visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed
;     |  (X,Y) pos of each visible dot                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1371| 
           LDW     .D2T2   *+SP(192),B5      ; |1371| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1371| 
	.line	651
;------------------------------------------------------------------------------
; 1372 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1372| 
	.line	652
;------------------------------------------------------------------------------
; 1373 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1373| 
	.line	653
           CMPLTU  .L1     A9,A14,A1         ; |1374| 
   [ A1]   B       .S1     L44               ; |1374| 
           NOP             5
           ; BRANCH OCCURS                   ; |1374| 
;** --------------------------------------------------------------------------*
	.line	654
           B       .S1     L133              ; |1375| 
           NOP             5
           ; BRANCH OCCURS                   ; |1375| 
;** --------------------------------------------------------------------------*
L47:    
	.line	655
;------------------------------------------------------------------------------
; 1376 | else if( u16Type == STATICANNU )                            // STATICAN
;     | NU: Neither window nor pattern move,                                   
; 1377 | {                                                           // so no ne
;     | ed to update dot pos nor to make                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1376| 
           NOP             4
           CMPEQ   .L2     B4,6,B0           ; |1376| 
   [!B0]   B       .S1     L51               ; |1376| 
           NOP             5
           ; BRANCH OCCURS                   ; |1376| 
;** --------------------------------------------------------------------------*
	.line	657
;------------------------------------------------------------------------------
; 1378 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             // sure that d
;     | ot is visible...                                                       
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1378| 
           LDW     .D2T2   *+SP(168),B5      ; |1378| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |1378| 
           ADDK    .S2     84,B5             ; |1378| 
           LDHU    .D2T2   *B5,B6            ; |1378| 
           LDW     .D2T2   *+SP(184),B5      ; |1378| 
           NOP             4
           STH     .D2T2   B6,*+B5[B4]       ; |1378| 
	.line	658
;------------------------------------------------------------------------------
; 1379 | while( a < de )                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1379| 
   [!A1]   B       .S1     L133              ; |1379| 
           NOP             5
           ; BRANCH OCCURS                   ; |1379| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L48
;** --------------------------------------------------------------------------*
L48:    
	.line	660
;------------------------------------------------------------------------------
; 1381 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A3            ; |1381| 
           LDHU    .D1T1   *A7,A0            ; |1381| 
           NOP             3
           SHL     .S1     A3,16,A3          ; |1381| 
           OR      .L1     A0,A3,A0          ; |1381| 
           STW     .D2T1   A0,*+SP(56)       ; |1381| 
	.line	661
;------------------------------------------------------------------------------
; 1382 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1382| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1382| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1382| 
   [!B0]   B       .S1     L50               ; |1382| 
           NOP             5
           ; BRANCH OCCURS                   ; |1382| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L49:    
           LDW     .D2T2   *+SP(76),B4       ; |1382| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1382| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1382| 
   [ B0]   B       .S1     L49               ; |1382| 
           NOP             5
           ; BRANCH OCCURS                   ; |1382| 
;** --------------------------------------------------------------------------*
L50:    
	.line	662
;------------------------------------------------------------------------------
; 1383 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1383| 
           LDW     .D2T2   *+SP(72),B5       ; |1383| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1383| 
	.line	663
;------------------------------------------------------------------------------
; 1384 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1384| 
           LDW     .D2T2   *+SP(192),B5      ; |1384| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1384| 
	.line	664
;------------------------------------------------------------------------------
; 1385 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1385| 
	.line	665
;------------------------------------------------------------------------------
; 1386 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1386| 
	.line	666
           CMPLTU  .L1     A9,A14,A1         ; |1387| 
   [ A1]   B       .S1     L48               ; |1387| 
           NOP             5
           ; BRANCH OCCURS                   ; |1387| 
;** --------------------------------------------------------------------------*
	.line	667
           B       .S1     L133              ; |1388| 
           NOP             5
           ; BRANCH OCCURS                   ; |1388| 
;** --------------------------------------------------------------------------*
L51:    
	.line	668
;------------------------------------------------------------------------------
; 1389 | else if( u16Type == RECTWINDOW )                            // RECTWIND
;     | OW:  Independent pattern & window                                      
; 1390 | {                                                           // motion. 
;     |  Visible dots lie inside window.                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1389| 
           NOP             4
           CMPEQ   .L2     B4,3,B0           ; |1389| 
   [!B0]   B       .S1     L56               ; |1389| 
           NOP             5
           ; BRANCH OCCURS                   ; |1389| 
;** --------------------------------------------------------------------------*
	.line	670
;------------------------------------------------------------------------------
; 1391 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1391| 
           LDW     .D2T2   *+SP(184),B6      ; |1391| 
           ZERO    .L2     B4                ; |1391| 
           NOP             3
           STH     .D2T2   B4,*+B6[B5]       ; |1391| 
	.line	671
;------------------------------------------------------------------------------
; 1392 | while( a < de )                                                        
; 1394 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1395 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1392| 
   [!A1]   B       .S1     L133              ; |1392| 
           NOP             5
           ; BRANCH OCCURS                   ; |1392| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L52
;** --------------------------------------------------------------------------*
L52:    
	.line	675
;------------------------------------------------------------------------------
; 1396 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1396| 
||         LDHU    .D1T1   *A9,A0            ; |1396| 

           LDH     .D2T2   *+SP(4),B5        ; |1396| 
           NOP             3
           ADD     .L2X    B4,A0,B4          ; |1396| 
           ADD     .L2     B5,B4,B4          ; |1396| 
           STH     .D1T2   B4,*A9            ; |1396| 
	.line	676
;------------------------------------------------------------------------------
; 1397 | *b = *b + vw + vv;                                                     
; 1398 | #ifdef _TGTDAKARF5                                                     
; 1399 | if( *a > 0x0000FFFF )                                                  
; 1401 | if(hw+hv > 0) *a -= 65536;                                             
; 1402 | else          *a &= 0x0000FFFF;                                        
; 1404 | if ( *b > 0x0000FFFF )                                                 
; 1406 | if(vw+vv > 0) *b -= 65536;                                             
; 1407 | else          *b &= 0x0000FFFF;                                        
; 1409 | #endif                                                                 
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(10),B4       ; |1397| 
||         LDHU    .D1T1   *A7,A0            ; |1397| 

           LDH     .D2T2   *+SP(6),B5        ; |1397| 
           NOP             3
           ADD     .L2X    B4,A0,B4          ; |1397| 
           ADD     .L2     B5,B4,B4          ; |1397| 
           STH     .D1T2   B4,*A7            ; |1397| 
	.line	690
;------------------------------------------------------------------------------
; 1411 | if( (*a <= rectR) && (*a >= rectL) && (*b <= rectU) && (*b >= rectD) ) 
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1411| 
           NOP             4
           CMPGT   .L1     A0,A8,A0          ; |1411| 
           XOR     .L1     1,A0,A1           ; |1411| 
   [ A1]   LDHU    .D1T1   *A9,A0            ; |1411| 
           NOP             4
   [ A1]   CMPLT   .L1X    A0,B8,A0          ; |1411| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1411| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1411| 
   [ A1]   LDHU    .D1T1   *A7,A0            ; |1411| 
           NOP             4
   [ A1]   CMPGT   .L1     A0,A13,A0         ; |1411| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1411| 
   [ A1]   LDHU    .D1T1   *A7,A0            ; |1411| 
           NOP             4
   [ A1]   CMPLT   .L2X    A0,B9,B4          ; |1411| 
   [ A1]   XOR     .L1X    1,B4,A1           ; |1411| 
   [!A1]   B       .S1     L55               ; |1411| 
           NOP             5
           ; BRANCH OCCURS                   ; |1411| 
;** --------------------------------------------------------------------------*
	.line	692
;------------------------------------------------------------------------------
; 1413 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1413| 
           LDHU    .D1T1   *A7,A3            ; |1413| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1413| 
           OR      .L1     A3,A0,A0          ; |1413| 
           STW     .D2T1   A0,*+SP(56)       ; |1413| 
	.line	693
;------------------------------------------------------------------------------
; 1414 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1414| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1414| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1414| 
   [!B0]   B       .S1     L54               ; |1414| 
           NOP             5
           ; BRANCH OCCURS                   ; |1414| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L53:    
           LDW     .D2T2   *+SP(76),B4       ; |1414| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1414| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1414| 
   [ B0]   B       .S1     L53               ; |1414| 
           NOP             5
           ; BRANCH OCCURS                   ; |1414| 
;** --------------------------------------------------------------------------*
L54:    
	.line	694
;------------------------------------------------------------------------------
; 1415 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1415| 
           LDW     .D2T2   *+SP(72),B5       ; |1415| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1415| 
	.line	695
;------------------------------------------------------------------------------
; 1416 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B6      ; |1416| 
           LDW     .D2T2   *+SP(56),B4       ; |1416| 
           MV      .L2X    A12,B5            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B4,*+B6[B5]       ; |1416| 
	.line	696
;------------------------------------------------------------------------------
; 1417 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(184),B5      ; |1417| 
           LDW     .D2T2   *+SP(104),B4      ; |1417| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |1417| 
           LDHU    .D2T2   *B5,B4            ; |1417| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1417| 
           STH     .D2T2   B4,*B5            ; |1417| 
;** --------------------------------------------------------------------------*
L55:    
	.line	698
;------------------------------------------------------------------------------
; 1419 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1419| 
	.line	699
;------------------------------------------------------------------------------
; 1420 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1420| 
	.line	700
           CMPLTU  .L1     A9,A14,A1         ; |1421| 
   [ A1]   B       .S1     L52               ; |1421| 
           NOP             5
           ; BRANCH OCCURS                   ; |1421| 
;** --------------------------------------------------------------------------*
	.line	701
           B       .S1     L133              ; |1422| 
           NOP             5
           ; BRANCH OCCURS                   ; |1422| 
;** --------------------------------------------------------------------------*
L56:    
	.line	702
;------------------------------------------------------------------------------
; 1423 | else if( u16Type == RECTHOLE )                              // RECTHOLE
;     | :  Independent window & pattern                                        
; 1424 | {                                                           // motion. 
;     |  Visible dots lie outside window.                                      
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1423| 
           NOP             4
           CMPEQ   .L2     B4,4,B0           ; |1423| 
   [!B0]   B       .S1     L61               ; |1423| 
           NOP             5
           ; BRANCH OCCURS                   ; |1423| 
;** --------------------------------------------------------------------------*
	.line	704
;------------------------------------------------------------------------------
; 1425 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1425| 
           LDW     .D2T2   *+SP(184),B6      ; |1425| 
           ZERO    .L2     B4                ; |1425| 
           NOP             3
           STH     .D2T2   B4,*+B6[B5]       ; |1425| 
	.line	705
;------------------------------------------------------------------------------
; 1426 | while( a < de )                                                        
; 1428 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1429 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1426| 
   [!A1]   B       .S1     L133              ; |1426| 
           NOP             5
           ; BRANCH OCCURS                   ; |1426| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L57
;** --------------------------------------------------------------------------*
L57:    
	.line	709
;------------------------------------------------------------------------------
; 1430 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1430| 
||         LDHU    .D1T1   *A9,A0            ; |1430| 

           LDH     .D2T2   *+SP(4),B5        ; |1430| 
           NOP             3
           ADD     .L2X    B4,A0,B4          ; |1430| 
           ADD     .L2     B5,B4,B4          ; |1430| 
           STH     .D1T2   B4,*A9            ; |1430| 
	.line	710
;------------------------------------------------------------------------------
; 1431 | *b = *b + vw + vv;                                                     
; 1432 | #ifdef _TGTDAKARF5                                                     
; 1433 | if( *a > 0x0000FFFF )                                                  
; 1435 | if(hw+hv > 0) *a -= 65536;                                             
; 1436 | else          *a &= 0x0000FFFF;                                        
; 1438 | if( *b > 0x0000FFFF )                                                  
; 1440 | if(vw+vv > 0) *b -= 65536;                                             
; 1441 | else          *b &= 0x0000FFFF;                                        
; 1443 | #endif                                                                 
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(10),B5       ; |1431| 
||         LDHU    .D1T1   *A7,A0            ; |1431| 

           LDH     .D2T2   *+SP(6),B4        ; |1431| 
           NOP             3
           ADD     .L2X    B5,A0,B5          ; |1431| 
           ADD     .L2     B4,B5,B4          ; |1431| 
           STH     .D1T2   B4,*A7            ; |1431| 
	.line	724
;------------------------------------------------------------------------------
; 1445 | if( (*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD) )     
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1445| 
           NOP             4
           CMPGT   .L1     A0,A8,A1          ; |1445| 
   [!A1]   LDHU    .D1T1   *A9,A0            ; |1445| 
           NOP             4
   [!A1]   CMPLT   .L1X    A0,B8,A1          ; |1445| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1445| 
   [!A1]   LDHU    .D1T1   *A7,A0            ; |1445| 
           NOP             4
   [!A1]   CMPGT   .L1     A0,A13,A1         ; |1445| 
   [!A1]   LDHU    .D1T1   *A7,A0            ; |1445| 
           NOP             4
   [!A1]   CMPLT   .L1X    A0,B9,A1          ; |1445| 
   [!A1]   B       .S1     L60               ; |1445| 
           NOP             5
           ; BRANCH OCCURS                   ; |1445| 
;** --------------------------------------------------------------------------*
	.line	726
;------------------------------------------------------------------------------
; 1447 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1447| 
           LDHU    .D1T1   *A7,A3            ; |1447| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1447| 
           OR      .L1     A3,A0,A0          ; |1447| 
           STW     .D2T1   A0,*+SP(56)       ; |1447| 
	.line	727
;------------------------------------------------------------------------------
; 1448 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1448| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1448| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1448| 
   [!B0]   B       .S1     L59               ; |1448| 
           NOP             5
           ; BRANCH OCCURS                   ; |1448| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L58:    
           LDW     .D2T2   *+SP(76),B4       ; |1448| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1448| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1448| 
   [ B0]   B       .S1     L58               ; |1448| 
           NOP             5
           ; BRANCH OCCURS                   ; |1448| 
;** --------------------------------------------------------------------------*
L59:    
	.line	728
;------------------------------------------------------------------------------
; 1449 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1449| 
           LDW     .D2T2   *+SP(72),B5       ; |1449| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1449| 
	.line	729
;------------------------------------------------------------------------------
; 1450 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B6      ; |1450| 
           LDW     .D2T2   *+SP(56),B5       ; |1450| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B5,*+B6[B4]       ; |1450| 
	.line	730
;------------------------------------------------------------------------------
; 1451 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1451| 
           LDW     .D2T2   *+SP(184),B5      ; |1451| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1451| 
           LDHU    .D2T2   *B4,B5            ; |1451| 
           NOP             4
           ADD     .L2     1,B5,B5           ; |1451| 
           STH     .D2T2   B5,*B4            ; |1451| 
;** --------------------------------------------------------------------------*
L60:    
	.line	732
;------------------------------------------------------------------------------
; 1453 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1453| 
	.line	733
;------------------------------------------------------------------------------
; 1454 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1454| 
	.line	734
           CMPLTU  .L1     A9,A14,A1         ; |1455| 
   [ A1]   B       .S1     L57               ; |1455| 
           NOP             5
           ; BRANCH OCCURS                   ; |1455| 
;** --------------------------------------------------------------------------*
	.line	735
           B       .S1     L133              ; |1456| 
           NOP             5
           ; BRANCH OCCURS                   ; |1456| 
;** --------------------------------------------------------------------------*
L61:    
	.line	736
;------------------------------------------------------------------------------
; 1457 | else if( u16Type == ANNULUS )                               // ANNULUS:
;     |   Independent window and pattern                                       
; 1458 | {                                                           // motion. 
;     |  Visible dots lie inside annulus.                                      
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1457| 
           NOP             4
           CMPEQ   .L2     B4,5,B0           ; |1457| 
   [!B0]   B       .S1     L67               ; |1457| 
           NOP             5
           ; BRANCH OCCURS                   ; |1457| 
;** --------------------------------------------------------------------------*
	.line	738
;------------------------------------------------------------------------------
; 1459 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1459| 
           LDW     .D2T2   *+SP(184),B6      ; |1459| 
           ZERO    .L2     B4                ; |1459| 
           NOP             3
           STH     .D2T2   B4,*+B6[B5]       ; |1459| 
	.line	739
;------------------------------------------------------------------------------
; 1460 | while( a < de )                                                        
; 1462 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1463 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1460| 
   [!A1]   B       .S1     L133              ; |1460| 
           NOP             5
           ; BRANCH OCCURS                   ; |1460| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L62
;** --------------------------------------------------------------------------*
L62:    
	.line	743
;------------------------------------------------------------------------------
; 1464 | *a = *a + hw + hv;                                                     
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1464| 
||         LDHU    .D1T1   *A9,A0            ; |1464| 

           LDH     .D2T2   *+SP(4),B5        ; |1464| 
           NOP             3
           ADD     .L2X    B4,A0,B4          ; |1464| 
           ADD     .L2     B5,B4,B4          ; |1464| 
           STH     .D1T2   B4,*A9            ; |1464| 
	.line	744
;------------------------------------------------------------------------------
; 1465 | *b = *b + vw + vv;                                                     
; 1466 | #ifdef _TGTDAKARF5                                                     
; 1467 | if( *a > 0x0000FFFF )                                                  
; 1469 | if(hw+hv > 0) *a -= 65536;                                             
; 1470 | else          *a &= 0x0000FFFF;                                        
; 1472 | if( *b > 0x0000FFFF )                                                  
; 1474 | if(vw+vv > 0) *b -= 65536;                                             
; 1475 | else          *b &= 0x0000FFFF;                                        
; 1477 | #endif                                                                 
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(10),B4       ; |1465| 
||         LDHU    .D1T1   *A7,A0            ; |1465| 

           LDH     .D2T2   *+SP(6),B5        ; |1465| 
           NOP             3
           ADD     .L2X    B4,A0,B4          ; |1465| 
           ADD     .L2     B5,B4,B4          ; |1465| 
           STH     .D1T2   B4,*A7            ; |1465| 
	.line	758
;------------------------------------------------------------------------------
; 1479 | if( (*a <= parameters->wOuterR[d]) && (*a >= parameters->wOuterL[d]) &&
;     |                                                                        
; 1480 | (*b <= parameters->wOuterT[d]) && (*b >= parameters->wOuterB[d]) &&    
; 1481 | ((*a > rectR) || (*a < rectL) || (*b > rectU) || (*b < rectD)) )       
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |1479| 
           LDW     .D2T2   *+SP(168),B4      ; |1479| 
           LDHU    .D1T1   *A9,A0            ; |1479| 
           NOP             3
           ADDAH   .D2     B4,B5,B4          ; |1479| 
           ADDK    .S2     404,B4            ; |1479| 
           LDHU    .D2T2   *B4,B4            ; |1479| 
           NOP             4
           CMPGT   .L1X    A0,B4,A0          ; |1479| 
           XOR     .L1     1,A0,A1           ; |1479| 
   [ A1]   LDW     .D2T2   *+SP(168),B5      ; |1479| 
   [ A1]   LDW     .D2T2   *+SP(104),B4      ; |1479| 
   [ A1]   LDHU    .D1T1   *A9,A0            ; |1479| 
           NOP             3
   [ A1]   ADDAH   .D2     B5,B4,B4          ; |1479| 
   [ A1]   ADDK    .S2     468,B4            ; |1479| 
   [ A1]   LDHU    .D2T2   *B4,B4            ; |1479| 
           NOP             4
   [ A1]   CMPLT   .L1X    A0,B4,A0          ; |1479| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1479| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1479| 
   [ A1]   LDW     .D2T2   *+SP(104),B4      ; |1479| 
   [ A1]   LDW     .D2T2   *+SP(168),B5      ; |1479| 
   [ A1]   LDHU    .D1T1   *A7,A0            ; |1479| 
           NOP             3
   [ A1]   ADDAH   .D2     B5,B4,B4          ; |1479| 
   [ A1]   ADDK    .S2     532,B4            ; |1479| 
   [ A1]   LDHU    .D2T2   *B4,B4            ; |1479| 
           NOP             4
   [ A1]   CMPGT   .L1X    A0,B4,A0          ; |1479| 
   [ A1]   XOR     .L1     1,A0,A1           ; |1479| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1479| 
   [ A1]   LDW     .D2T1   *+SP(168),A0      ; |1479| 
   [ A1]   LDW     .D2T1   *+SP(104),A3      ; |1479| 
   [!A1]   MV      .S1     A1,A0             ; |1479| 
           NOP             3
   [ A1]   ADDAH   .D1     A0,A3,A0          ; |1479| 
   [ A1]   ADDK    .S1     596,A0            ; |1479| 
   [ A1]   LDHU    .D1T1   *A0,A0            ; |1479| 
   [ A1]   LDHU    .D1T1   *A7,A3            ; |1479| 
           NOP             4
   [ A1]   CMPLT   .L1     A3,A0,A0          ; |1479| 
   [ A1]   XOR     .L1     1,A0,A0           ; |1479| 
           MV      .L2X    A0,B0             ; 
   [!B0]   B       .S1     L63               ; |1479| 
   [ B0]   LDHU    .D1T1   *A9,A0            ; |1479| 
           NOP             4
           ; BRANCH OCCURS                   ; |1479| 
;** --------------------------------------------------------------------------*
           CMPGT   .L1     A0,A8,A1          ; |1479| 
   [!A1]   LDHU    .D1T1   *A9,A0            ; |1479| 
           NOP             4
   [!A1]   CMPLT   .L1X    A0,B8,A1          ; |1479| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1479| 
   [!A1]   LDHU    .D1T1   *A7,A0            ; |1479| 
           NOP             4
   [!A1]   CMPGT   .L1     A0,A13,A1         ; |1479| 
           CMPEQ   .L1     A1,0,A0           ; 
           XOR     .L1     1,A0,A1           ; |1479| 
   [!A1]   LDHU    .D1T1   *A7,A0            ; |1479| 
           NOP             4
   [!A1]   CMPLT   .L1X    A0,B9,A1          ; |1479| 
           MV      .L2X    A1,B0             ; 
;** --------------------------------------------------------------------------*
L63:    
   [!B0]   B       .S1     L66               ; |1479| 
           NOP             5
           ; BRANCH OCCURS                   ; |1479| 
;** --------------------------------------------------------------------------*
	.line	762
;------------------------------------------------------------------------------
; 1483 | xyvals = ((*a << 16) | *b);                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1483| 
           LDHU    .D1T1   *A7,A3            ; |1483| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1483| 
           OR      .L1     A3,A0,A0          ; |1483| 
           STW     .D2T1   A0,*+SP(56)       ; |1483| 
	.line	763
;------------------------------------------------------------------------------
; 1484 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1484| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1484| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1484| 
   [!B0]   B       .S1     L65               ; |1484| 
           NOP             5
           ; BRANCH OCCURS                   ; |1484| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L64:    
           LDW     .D2T2   *+SP(76),B4       ; |1484| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1484| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1484| 
   [ B0]   B       .S1     L64               ; |1484| 
           NOP             5
           ; BRANCH OCCURS                   ; |1484| 
;** --------------------------------------------------------------------------*
L65:    
	.line	764
;------------------------------------------------------------------------------
; 1485 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1485| 
           LDW     .D2T2   *+SP(72),B5       ; |1485| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1485| 
	.line	765
;------------------------------------------------------------------------------
; 1486 | visibleDotsXY[nTotalVisDots++] = xyvals;                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1486| 
           LDW     .D2T2   *+SP(192),B5      ; |1486| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1486| 
	.line	766
;------------------------------------------------------------------------------
; 1487 | nVisDotsPerTgt[d]++;                                                   
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1487| 
           LDW     .D2T2   *+SP(184),B5      ; |1487| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |1487| 
           LDHU    .D2T2   *B5,B4            ; |1487| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1487| 
           STH     .D2T2   B4,*B5            ; |1487| 
;** --------------------------------------------------------------------------*
L66:    
	.line	768
;------------------------------------------------------------------------------
; 1489 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1489| 
	.line	769
;------------------------------------------------------------------------------
; 1490 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1490| 
	.line	770
           CMPLTU  .L1     A9,A14,A1         ; |1491| 
   [ A1]   B       .S1     L62               ; |1491| 
           NOP             5
           ; BRANCH OCCURS                   ; |1491| 
;** --------------------------------------------------------------------------*
	.line	771
           B       .S1     L133              ; |1492| 
           NOP             5
           ; BRANCH OCCURS                   ; |1492| 
;** --------------------------------------------------------------------------*
L67:    
	.line	772
;------------------------------------------------------------------------------
; 1493 | else if( u16Type == OPTRECTWIN )                            // OPTRECTW
;     | IN: Independent pattern & window                                       
; 1494 | {                                                           // motion, 
;     | but all dots restricted to window...                                   
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1493| 
           NOP             4
           CMPEQ   .L2     B4,7,B0           ; |1493| 
   [!B0]   B       .S1     L73               ; |1493| 
           NOP             5
           ; BRANCH OCCURS                   ; |1493| 
;** --------------------------------------------------------------------------*
	.line	774
;------------------------------------------------------------------------------
; 1495 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1495| 
           LDW     .D2T2   *+SP(172),B5      ; |1495| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1495| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1495| 
	.line	775
;------------------------------------------------------------------------------
; 1496 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1496| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1496| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1496| 
	.line	777
;------------------------------------------------------------------------------
; 1498 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1499 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1498| 
           XOR     .L1     1,A0,A1           ; |1498| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1498| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1498| 
	.line	779
;------------------------------------------------------------------------------
; 1500 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(184),A4      ; |1500| 
   [ A1]   LDW     .D2T1   *+SP(104),A0      ; |1500| 
   [ A1]   ZERO    .L1     A3                ; |1500| 
           NOP             3
   [ A1]   STH     .D1T1   A3,*+A4[A0]       ; |1500| 
	.line	780
;------------------------------------------------------------------------------
; 1501 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1501| 
	.line	781
;------------------------------------------------------------------------------
; 1503 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	783
;------------------------------------------------------------------------------
; 1504 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(168),B5      ; |1504| 
   [!A1]   LDW     .D2T2   *+SP(104),B4      ; |1504| 
           NOP             4
   [!A1]   ADDAH   .D2     B5,B4,B4          ; |1504| 
   [!A1]   ADDK    .S2     84,B4             ; |1504| 
   [!A1]   LDHU    .D2T2   *B4,B6            ; |1504| 
   [!A1]   LDW     .D2T2   *+SP(104),B4      ; |1504| 
   [!A1]   LDW     .D2T2   *+SP(184),B5      ; |1504| 
           NOP             4
   [!A1]   STH     .D2T2   B6,*+B5[B4]       ; |1504| 
	.line	785
;------------------------------------------------------------------------------
; 1506 | while ( a < de )                                         //    for each
;     |  dot in target:                                                        
; 1508 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1509 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1506| 
   [!A1]   B       .S1     L133              ; |1506| 
           NOP             5
           ; BRANCH OCCURS                   ; |1506| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L68
;** --------------------------------------------------------------------------*
L68:    
	.line	789
;------------------------------------------------------------------------------
; 1510 | xCoord = *a + hw + hv;                                //       update i
;     | ts position                                                            
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1510| 
||         LDHU    .D1T1   *A9,A0            ; |1510| 

           LDH     .D2T2   *+SP(4),B5        ; |1510| 
           NOP             3
           ADD     .L1X    B4,A0,A0          ; |1510| 
           ADD     .L1X    B5,A0,A0          ; |1510| 
           EXTU    .S1     A0,16,16,A10      ; |1510| 
	.line	790
;------------------------------------------------------------------------------
; 1511 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------
           LDH     .D2T1   *+SP(10),A3       ; |1511| 
           LDHU    .D1T1   *A7,A0            ; |1511| 
           LDH     .D2T2   *+SP(6),B4        ; |1511| 
           NOP             3
           ADD     .L1     A3,A0,A0          ; |1511| 
           ADD     .L1X    B4,A0,A0          ; |1511| 
           EXTU    .S1     A0,16,16,A11      ; |1511| 
	.line	791
;------------------------------------------------------------------------------
; 1512 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if tgt h
;     | as violated horizontal bounds:                                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1512| 
   [!A1]   CMPLT   .L1X    A10,B8,A1         ; |1512| 
   [!A1]   B       .S1     L69               ; |1512| 
           NOP             5
           ; BRANCH OCCURS                   ; |1512| 
;** --------------------------------------------------------------------------*
	.line	793
;------------------------------------------------------------------------------
; 1514 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //          compute 
;     | positive distance by which                                             
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1514| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |1514| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1514| 
	.line	794
;------------------------------------------------------------------------------
; 1515 | else                 u16Over = rectL - xCoord;     //          dot has 
;     | moved beyond border                                                    
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |1515| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1515| 
	.line	795
;------------------------------------------------------------------------------
; 1516 | u16Over = u16Over % rectW;                         //          in case 
;     | distance > window width!                                               
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1516| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL424,B3          ; |1516| 
           MVKH    .S2     RL424,B3          ; |1516| 
           NOP             2
RL424:     ; CALL OCCURS                     ; |1516| 
           STH     .D2T1   A4,*+SP(42)       ; |1516| 
	.line	797
;------------------------------------------------------------------------------
; 1518 | if( hv > 0 )  xCoord = rectL + u16Over;            //          if dots 
;     | moving right wrt window,                                               
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |1518| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1518| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1518| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |1518| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |1518| 
	.line	798
;------------------------------------------------------------------------------
; 1519 | else          xCoord = rectR - u16Over;            //          offset f
;     | rom left edge, else right                                              
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1519| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |1519| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |1519| 
	.line	800
;------------------------------------------------------------------------------
; 1521 | yCoord = (UINT16) GetRandNum();                    //          and rand
;     | omize the vertical coord                                               
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1521| 
           MVKL    .S2     RL426,B3          ; |1521| 
           MVKH    .S2     RL426,B3          ; |1521| 
           NOP             3
RL426:     ; CALL OCCURS                     ; |1521| 
	.line	801
;------------------------------------------------------------------------------
; 1522 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1522| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL428,B3          ; |1522| 
           MVKH    .S2     RL428,B3          ; |1522| 
           NOP             2
RL428:     ; CALL OCCURS                     ; |1522| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1522| 
	.line	802
           B       .S1     L70               ; |1523| 
           NOP             5
           ; BRANCH OCCURS                   ; |1523| 
;** --------------------------------------------------------------------------*
L69:    
	.line	803
;------------------------------------------------------------------------------
; 1524 | else if( (yCoord > rectU) || (yCoord < rectD) )       //       else if 
;     | tgt violated vertical bounds:                                          
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1524| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |1524| 
   [!A1]   B       .S1     L70               ; |1524| 
           NOP             5
           ; BRANCH OCCURS                   ; |1524| 
;** --------------------------------------------------------------------------*
	.line	805
;------------------------------------------------------------------------------
; 1526 | if( yCoord > rectU ) u16Over = yCoord - rectU;     //          dist dot
;     |  moved beyond border...                                                
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1526| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |1526| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1526| 
	.line	806
;------------------------------------------------------------------------------
; 1527 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |1527| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1527| 
	.line	807
;------------------------------------------------------------------------------
; 1528 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1528| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL430,B3          ; |1528| 
           MVKH    .S2     RL430,B3          ; |1528| 
           NOP             2
RL430:     ; CALL OCCURS                     ; |1528| 
           STH     .D2T1   A4,*+SP(42)       ; |1528| 
	.line	809
;------------------------------------------------------------------------------
; 1530 | if( vv > 0 )  yCoord = rectD + u16Over;            //          if dots 
;     | moving up wrt window,                                                  
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1530| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1530| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1530| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |1530| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |1530| 
	.line	810
;------------------------------------------------------------------------------
; 1531 | else          yCoord = rectU - u16Over;            //          offset f
;     | rom bottom edge, else top                                              
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1531| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |1531| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |1531| 
	.line	812
;------------------------------------------------------------------------------
; 1533 | xCoord = (UINT16) GetRandNum();                    //          and rand
;     | omize the horizontal coord                                             
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1533| 
           MVKL    .S2     RL432,B3          ; |1533| 
           MVKH    .S2     RL432,B3          ; |1533| 
           NOP             3
RL432:     ; CALL OCCURS                     ; |1533| 
	.line	813
;------------------------------------------------------------------------------
; 1534 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1534| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL434,B3          ; |1534| 
           MVKH    .S2     RL434,B3          ; |1534| 
           NOP             2
RL434:     ; CALL OCCURS                     ; |1534| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1534| 
;** --------------------------------------------------------------------------*
L70:    
	.line	815
;------------------------------------------------------------------------------
; 1536 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |1536| 
	.line	816
;------------------------------------------------------------------------------
; 1537 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |1537| 
	.line	818
;------------------------------------------------------------------------------
; 1539 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1539| 
           LDHU    .D1T1   *A7,A3            ; |1539| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1539| 
           OR      .L1     A3,A0,A0          ; |1539| 
           STW     .D2T1   A0,*+SP(56)       ; |1539| 
	.line	819
;------------------------------------------------------------------------------
; 1540 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1540| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1540| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1540| 
   [!B0]   B       .S1     L72               ; |1540| 
           NOP             5
           ; BRANCH OCCURS                   ; |1540| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L71:    
           LDW     .D2T2   *+SP(76),B4       ; |1540| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1540| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1540| 
   [ B0]   B       .S1     L71               ; |1540| 
           NOP             5
           ; BRANCH OCCURS                   ; |1540| 
;** --------------------------------------------------------------------------*
L72:    
	.line	820
;------------------------------------------------------------------------------
; 1541 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1541| 
           LDW     .D2T2   *+SP(72),B5       ; |1541| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1541| 
	.line	821
;------------------------------------------------------------------------------
; 1542 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1542| 
           LDW     .D2T2   *+SP(192),B5      ; |1542| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1542| 
	.line	822
;------------------------------------------------------------------------------
; 1543 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1543| 
	.line	823
;------------------------------------------------------------------------------
; 1544 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1544| 
	.line	824
           CMPLTU  .L1     A9,A14,A1         ; |1545| 
   [ A1]   B       .S1     L68               ; |1545| 
           NOP             5
           ; BRANCH OCCURS                   ; |1545| 
;** --------------------------------------------------------------------------*
	.line	825
           B       .S1     L133              ; |1546| 
           NOP             5
           ; BRANCH OCCURS                   ; |1546| 
;** --------------------------------------------------------------------------*
L73:    
	.line	826
;------------------------------------------------------------------------------
; 1547 | else if( u16Type == OPTCOHERENT )                           // OPTCOHER
;     | ENT: Like OPTRECTWIN, but implements                                   
; 1548 | {                                                           // percent 
;     | coherence...                                                           
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1547| 
           NOP             4
           CMPEQ   .L2     B4,12,B0          ; |1547| 
   [!B0]   B       .S1     L80               ; |1547| 
           NOP             5
           ; BRANCH OCCURS                   ; |1547| 
;** --------------------------------------------------------------------------*
	.line	828
;------------------------------------------------------------------------------
; 1549 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1549| 
           LDW     .D2T2   *+SP(172),B5      ; |1549| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1549| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1549| 
	.line	829
;------------------------------------------------------------------------------
; 1550 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1550| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1550| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1550| 
	.line	830
;------------------------------------------------------------------------------
; 1551 | u16Dummy = parameters->wOuterL[d];                       //    percent 
;     | coherence in [0..100]                                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1551| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1551| 
           ADDK    .S2     468,B4            ; |1551| 
           LDHU    .D2T2   *B4,B4            ; |1551| 
           NOP             4
           STH     .D2T2   B4,*+SP(38)       ; |1551| 
	.line	832
;------------------------------------------------------------------------------
; 1553 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1554 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1553| 
           XOR     .L1     1,A0,A1           ; |1553| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1553| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1553| 
	.line	834
;------------------------------------------------------------------------------
; 1555 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(184),A4      ; |1555| 
   [ A1]   LDW     .D2T1   *+SP(104),A3      ; |1555| 
   [ A1]   ZERO    .L1     A0                ; |1555| 
           NOP             3
   [ A1]   STH     .D1T1   A0,*+A4[A3]       ; |1555| 
	.line	835
;------------------------------------------------------------------------------
; 1556 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1556| 
	.line	836
;------------------------------------------------------------------------------
; 1558 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	838
;------------------------------------------------------------------------------
; 1559 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(168),B5      ; |1559| 
   [!A1]   LDW     .D2T2   *+SP(104),B4      ; |1559| 
   [!A1]   LDW     .D2T2   *+SP(104),B6      ; |1559| 
           NOP             3
   [!A1]   ADDAH   .D2     B5,B4,B4          ; |1559| 
   [!A1]   ADDK    .S2     84,B4             ; |1559| 
   [!A1]   LDHU    .D2T2   *B4,B5            ; |1559| 
   [!A1]   LDW     .D2T2   *+SP(184),B4      ; |1559| 
           NOP             4
   [!A1]   STH     .D2T2   B5,*+B4[B6]       ; |1559| 
	.line	840
;------------------------------------------------------------------------------
; 1561 | while ( a < de )                                         //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1561| 
   [!A1]   B       .S1     L133              ; |1561| 
           NOP             5
           ; BRANCH OCCURS                   ; |1561| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L74
;** --------------------------------------------------------------------------*
L74:    
	.line	842
;------------------------------------------------------------------------------
; 1563 | u16tmp = ((UINT16) GetRandNum()) % 100;               //       if rando
;     | m choice >= %coherence,                                                
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1563| 
           MVKL    .S2     RL438,B3          ; |1563| 
           MVKH    .S2     RL438,B3          ; |1563| 
           NOP             3
RL438:     ; CALL OCCURS                     ; |1563| 
           B       .S1     __remi            ; |1563| 
           MVKL    .S2     RL440,B3          ; |1563| 
           MVK     .S2     0x64,B4           ; |1563| 
           MVKH    .S2     RL440,B3          ; |1563| 
           NOP             2
RL440:     ; CALL OCCURS                     ; |1563| 
           STH     .D2T1   A4,*+SP(40)       ; |1563| 
	.line	843
;------------------------------------------------------------------------------
; 1564 | if( u16tmp >= u16Dummy )                              //       then ran
;     | domly reposition dot                                                   
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B5       ; |1564| 
           LDHU    .D2T2   *+SP(40),B4       ; |1564| 
           NOP             4
           CMPLT   .L2     B4,B5,B0          ; |1564| 
   [ B0]   B       .S1     L75               ; |1564| 
           NOP             5
           ; BRANCH OCCURS                   ; |1564| 
;** --------------------------------------------------------------------------*
	.line	845
;------------------------------------------------------------------------------
; 1566 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1566| 
           MVKL    .S2     RL442,B3          ; |1566| 
           MVKH    .S2     RL442,B3          ; |1566| 
           NOP             3
RL442:     ; CALL OCCURS                     ; |1566| 
           MV      .L1     A4,A10            ; 
	.line	846
;------------------------------------------------------------------------------
; 1567 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1567| 
           MVKL    .S2     RL444,B3          ; |1567| 
           MVKH    .S2     RL444,B3          ; |1567| 
           NOP             3
RL444:     ; CALL OCCURS                     ; |1567| 
           MV      .L1     A4,A11            ; 
	.line	847
;------------------------------------------------------------------------------
; 1568 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1568| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL446,B3          ; |1568| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL446,B3          ; |1568| 
           NOP             1
RL446:     ; CALL OCCURS                     ; |1568| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1568| 
	.line	848
;------------------------------------------------------------------------------
; 1569 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1569| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL448,B3          ; |1569| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL448,B3          ; |1569| 
           NOP             1
RL448:     ; CALL OCCURS                     ; |1569| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1569| 
	.line	849
;------------------------------------------------------------------------------
; 1571 | else                                                  //       OTHERWIS
;     | E, move coherently (same                                               
; 1572 | {                                                     //       algorith
;     | m as for OPTRECTWIN!):                                                 
; 1573 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement                                            
; 1574 | // is window displacement + pattern displacement!                      
;------------------------------------------------------------------------------
           B       .S1     L77               ; |1570| 
           NOP             5
           ; BRANCH OCCURS                   ; |1570| 
;** --------------------------------------------------------------------------*
L75:    
	.line	854
;------------------------------------------------------------------------------
; 1575 | xCoord = *a + hw + hv;                                                 
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1575| 
||         LDHU    .D1T1   *A9,A0            ; |1575| 

           LDH     .D2T2   *+SP(4),B5        ; |1575| 
           NOP             3
           ADD     .L1X    B4,A0,A0          ; |1575| 
           ADD     .L1X    B5,A0,A0          ; |1575| 
           EXTU    .S1     A0,16,16,A10      ; |1575| 
	.line	855
;------------------------------------------------------------------------------
; 1576 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------
           LDH     .D2T1   *+SP(10),A3       ; |1576| 
           LDHU    .D1T1   *A7,A0            ; |1576| 
           LDH     .D2T2   *+SP(6),B4        ; |1576| 
           NOP             3
           ADD     .L1     A3,A0,A0          ; |1576| 
           ADD     .L1X    B4,A0,A0          ; |1576| 
           EXTU    .S1     A0,16,16,A11      ; |1576| 
	.line	856
;------------------------------------------------------------------------------
; 1577 | if( (xCoord > rectR) || (xCoord < rectL) )                             
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1577| 
   [!A1]   CMPLT   .L1X    A10,B8,A1         ; |1577| 
   [!A1]   B       .S1     L76               ; |1577| 
           NOP             5
           ; BRANCH OCCURS                   ; |1577| 
;** --------------------------------------------------------------------------*
	.line	858
;------------------------------------------------------------------------------
; 1579 | if( xCoord > rectR ) u16Over = xCoord - rectR;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1579| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |1579| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1579| 
	.line	859
;------------------------------------------------------------------------------
; 1580 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |1580| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1580| 
	.line	860
;------------------------------------------------------------------------------
; 1581 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1581| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL450,B3          ; |1581| 
           MVKH    .S2     RL450,B3          ; |1581| 
           NOP             2
RL450:     ; CALL OCCURS                     ; |1581| 
           STH     .D2T1   A4,*+SP(42)       ; |1581| 
	.line	862
;------------------------------------------------------------------------------
; 1583 | if( hv > 0 )  xCoord = rectL + u16Over;                                
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |1583| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1583| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1583| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |1583| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |1583| 
	.line	863
;------------------------------------------------------------------------------
; 1584 | else          xCoord = rectR - u16Over;                                
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1584| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |1584| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |1584| 
	.line	865
;------------------------------------------------------------------------------
; 1586 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1586| 
           MVKL    .S2     RL452,B3          ; |1586| 
           MVKH    .S2     RL452,B3          ; |1586| 
           NOP             3
RL452:     ; CALL OCCURS                     ; |1586| 
	.line	866
;------------------------------------------------------------------------------
; 1587 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1587| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL454,B3          ; |1587| 
           MVKH    .S2     RL454,B3          ; |1587| 
           NOP             2
RL454:     ; CALL OCCURS                     ; |1587| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1587| 
	.line	867
           B       .S1     L77               ; |1588| 
           NOP             5
           ; BRANCH OCCURS                   ; |1588| 
;** --------------------------------------------------------------------------*
L76:    
	.line	868
;------------------------------------------------------------------------------
; 1589 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1589| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |1589| 
   [!A1]   B       .S1     L77               ; |1589| 
           NOP             5
           ; BRANCH OCCURS                   ; |1589| 
;** --------------------------------------------------------------------------*
	.line	870
;------------------------------------------------------------------------------
; 1591 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1591| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |1591| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1591| 
	.line	871
;------------------------------------------------------------------------------
; 1592 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |1592| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1592| 
	.line	872
;------------------------------------------------------------------------------
; 1593 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1593| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL456,B3          ; |1593| 
           MVKH    .S2     RL456,B3          ; |1593| 
           NOP             2
RL456:     ; CALL OCCURS                     ; |1593| 
           STH     .D2T1   A4,*+SP(42)       ; |1593| 
	.line	874
;------------------------------------------------------------------------------
; 1595 | if( vv > 0 )  yCoord = rectD + u16Over;                                
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1595| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1595| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1595| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |1595| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |1595| 
	.line	875
;------------------------------------------------------------------------------
; 1596 | else          yCoord = rectU - u16Over;                                
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1596| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |1596| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |1596| 
	.line	877
;------------------------------------------------------------------------------
; 1598 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1598| 
           MVKL    .S2     RL458,B3          ; |1598| 
           MVKH    .S2     RL458,B3          ; |1598| 
           NOP             3
RL458:     ; CALL OCCURS                     ; |1598| 
	.line	878
;------------------------------------------------------------------------------
; 1599 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1599| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL460,B3          ; |1599| 
           MVKH    .S2     RL460,B3          ; |1599| 
           NOP             2
RL460:     ; CALL OCCURS                     ; |1599| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1599| 
;** --------------------------------------------------------------------------*
L77:    
	.line	882
;------------------------------------------------------------------------------
; 1603 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |1603| 
	.line	883
;------------------------------------------------------------------------------
; 1604 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |1604| 
	.line	885
;------------------------------------------------------------------------------
; 1606 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1606| 
           LDHU    .D1T1   *A7,A3            ; |1606| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1606| 
           OR      .L1     A3,A0,A0          ; |1606| 
           STW     .D2T1   A0,*+SP(56)       ; |1606| 
	.line	886
;------------------------------------------------------------------------------
; 1607 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1607| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1607| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1607| 
   [!B0]   B       .S1     L79               ; |1607| 
           NOP             5
           ; BRANCH OCCURS                   ; |1607| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L78:    
           LDW     .D2T2   *+SP(76),B4       ; |1607| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1607| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1607| 
   [ B0]   B       .S1     L78               ; |1607| 
           NOP             5
           ; BRANCH OCCURS                   ; |1607| 
;** --------------------------------------------------------------------------*
L79:    
	.line	887
;------------------------------------------------------------------------------
; 1608 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1608| 
           LDW     .D2T2   *+SP(72),B5       ; |1608| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1608| 
	.line	888
;------------------------------------------------------------------------------
; 1609 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1609| 
           LDW     .D2T2   *+SP(192),B5      ; |1609| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1609| 
	.line	889
;------------------------------------------------------------------------------
; 1610 | a++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1610| 
	.line	890
;------------------------------------------------------------------------------
; 1611 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1611| 
	.line	891
           CMPLTU  .L1     A9,A14,A1         ; |1612| 
   [ A1]   B       .S1     L74               ; |1612| 
           NOP             5
           ; BRANCH OCCURS                   ; |1612| 
;** --------------------------------------------------------------------------*
	.line	892
           B       .S1     L133              ; |1613| 
           NOP             5
           ; BRANCH OCCURS                   ; |1613| 
;** --------------------------------------------------------------------------*
L80:    
	.line	893
;------------------------------------------------------------------------------
; 1614 | else if( u16Type == DOTLIFEWIN )                            // DOTLIFEW
;     | IN: Similar to OPTRECTWIN, but dots                                    
; 1615 | {                                                           // have a l
;     | imited lifetime...                                                     
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1614| 
           NOP             4
           CMPEQ   .L2     B4,8,B0           ; |1614| 
   [!B0]   B       .S1     L88               ; |1614| 
           NOP             5
           ; BRANCH OCCURS                   ; |1614| 
;** --------------------------------------------------------------------------*
	.line	895
;------------------------------------------------------------------------------
; 1616 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1616| 
           LDW     .D2T2   *+SP(172),B5      ; |1616| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1616| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1616| 
	.line	896
;------------------------------------------------------------------------------
; 1617 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1617| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1617| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1617| 
	.line	898
;------------------------------------------------------------------------------
; 1619 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1620 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1619| 
           XOR     .L1     1,A0,A1           ; |1619| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1619| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1619| 
	.line	900
;------------------------------------------------------------------------------
; 1621 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(184),A4      ; |1621| 
   [ A1]   LDW     .D2T1   *+SP(104),A3      ; |1621| 
   [ A1]   ZERO    .L1     A0                ; |1621| 
           NOP             3
   [ A1]   STH     .D1T1   A0,*+A4[A3]       ; |1621| 
	.line	901
;------------------------------------------------------------------------------
; 1622 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1622| 
	.line	902
;------------------------------------------------------------------------------
; 1624 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	904
;------------------------------------------------------------------------------
; 1625 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1625| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1625| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1625| 
   [!A1]   ADDK    .S2     84,B4             ; |1625| 
   [!A1]   LDHU    .D2T2   *B4,B6            ; |1625| 
   [!A1]   LDW     .D2T2   *+SP(184),B4      ; |1625| 
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1625| 
           NOP             4
   [!A1]   STH     .D2T2   B6,*+B4[B5]       ; |1625| 
	.line	906
;------------------------------------------------------------------------------
; 1627 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |1627| 
           LDW     .D2T1   *+SP(200),A0      ; |1627| 
           NOP             3
           ADD     .L1X    4,B4,A3           ; |1627| 
           LDHU    .D1T1   *+A0[A3],A0       ; |1627| 
           NOP             4
           STH     .D2T1   A0,*+SP(38)       ; |1627| 
	.line	907
;------------------------------------------------------------------------------
; 1628 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1628| 
           NOP             4
           SHRU    .S2     B4,8,B4           ; |1628| 
           STH     .D2T2   B4,*+SP(38)       ; |1628| 
	.line	908
;------------------------------------------------------------------------------
; 1629 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1629| 
           NOP             4
           EXTU    .S2     B4,24,24,B4       ; |1629| 
           STH     .D2T2   B4,*+SP(38)       ; |1629| 
	.line	909
;------------------------------------------------------------------------------
; 1630 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(38),B4       ; |1630| 
           NOP             4
           STH     .D2T2   B4,*+SP(12)       ; |1630| 
	.line	911
;------------------------------------------------------------------------------
; 1632 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1632| 
           LDW     .D2T2   *+SP(168),B5      ; |1632| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1632| 
           ADDK    .S2     404,B4            ; |1632| 
           LDHU    .D2T2   *B4,B4            ; |1632| 
           NOP             4
           STH     .D2T2   B4,*+SP(44)       ; |1632| 
	.line	912
;------------------------------------------------------------------------------
; 1633 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B4       ; |1633| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1633| 
   [!B0]   MVK     .S2     1,B4              ; |1633| 
   [!B0]   STH     .D2T2   B4,*+SP(44)       ; |1633| 
   [!B0]   B       .S1     L81               ; |1633| 
           NOP             5
           ; BRANCH OCCURS                   ; |1633| 
;** --------------------------------------------------------------------------*
	.line	913
;------------------------------------------------------------------------------
; 1634 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1634| 
           MVK     .S2     32767,B4          ; |1634| 
           NOP             3
           CMPGT   .L2     B5,B4,B0          ; |1634| 
   [ B0]   MVK     .S1     32767,A0          ; |1634| 
   [ B0]   STH     .D2T1   A0,*+SP(44)       ; |1634| 
;** --------------------------------------------------------------------------*
L81:    
	.line	915
;------------------------------------------------------------------------------
; 1636 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
; 1638 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1639 | // window displacement + pattern displacement!                         
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1636| 
   [!A1]   B       .S1     L133              ; |1636| 
           NOP             5
           ; BRANCH OCCURS                   ; |1636| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L82
;** --------------------------------------------------------------------------*
L82:    
	.line	919
;------------------------------------------------------------------------------
; 1640 | xCoord = *a + hw + hv;                                //       update i
;     | ts position                                                            
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |1640| 
||         LDHU    .D1T1   *A9,A0            ; |1640| 

           LDH     .D2T2   *+SP(4),B5        ; |1640| 
           NOP             3
           ADD     .L1X    B4,A0,A0          ; |1640| 
           ADD     .L1X    B5,A0,A0          ; |1640| 
           EXTU    .S1     A0,16,16,A10      ; |1640| 
	.line	920
;------------------------------------------------------------------------------
; 1641 | yCoord = *b + vw + vv;                                                 
;------------------------------------------------------------------------------

           LDHU    .D1T1   *A7,A0            ; |1641| 
||         LDH     .D2T2   *+SP(10),B5       ; |1641| 

           LDH     .D2T2   *+SP(6),B4        ; |1641| 
           NOP             3
           ADD     .L1X    B5,A0,A0          ; |1641| 
           ADD     .L1X    B4,A0,A0          ; |1641| 
           EXTU    .S1     A0,16,16,A11      ; |1641| 
	.line	921
;------------------------------------------------------------------------------
; 1642 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update i
;     | ts current lifetime                                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1642| 
           LDH     .D2T2   *+SP(12),B5       ; |1642| 
           NOP             3
           LDH     .D2T2   *B4,B6            ; |1642| 
           NOP             4
           SUB     .L2     B6,B5,B5          ; |1642| 
           STH     .D2T2   B5,*B4            ; |1642| 
	.line	923
;------------------------------------------------------------------------------
; 1644 | if( *nextDotLife < 0 )                                //       if dot's
;     |  lifetime has expired, then                                            
; 1645 | {                                                     //       randomly
;     |  repos dot in tgt window...                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B5      ; |1644| 
           NOP             4
           LDH     .D2T2   *B5,B4            ; |1644| 
           NOP             4
           CMPLT   .L2     B4,0,B0           ; |1644| 
   [!B0]   B       .S1     L83               ; |1644| 
           NOP             5
           ; BRANCH OCCURS                   ; |1644| 
;** --------------------------------------------------------------------------*
	.line	925
;------------------------------------------------------------------------------
; 1646 | *nextDotLife = nMaxDotLife;                                            
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(44),B4       ; |1646| 
           NOP             4
           STH     .D2T2   B4,*B5            ; |1646| 
	.line	926
;------------------------------------------------------------------------------
; 1647 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1647| 
           MVKL    .S2     RL462,B3          ; |1647| 
           MVKH    .S2     RL462,B3          ; |1647| 
           NOP             3
RL462:     ; CALL OCCURS                     ; |1647| 
           MV      .L1     A4,A10            ; 
	.line	927
;------------------------------------------------------------------------------
; 1648 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1648| 
           MVKL    .S2     RL464,B3          ; |1648| 
           MVKH    .S2     RL464,B3          ; |1648| 
           NOP             3
RL464:     ; CALL OCCURS                     ; |1648| 
           MV      .L1     A4,A11            ; 
	.line	928
;------------------------------------------------------------------------------
; 1649 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1649| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL466,B3          ; |1649| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL466,B3          ; |1649| 
           NOP             1
RL466:     ; CALL OCCURS                     ; |1649| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1649| 
	.line	929
;------------------------------------------------------------------------------
; 1650 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1650| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL468,B3          ; |1650| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL468,B3          ; |1650| 
           NOP             1
RL468:     ; CALL OCCURS                     ; |1650| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1650| 
	.line	930
           B       .S1     L85               ; |1651| 
           NOP             5
           ; BRANCH OCCURS                   ; |1651| 
;** --------------------------------------------------------------------------*
L83:    
	.line	931
;------------------------------------------------------------------------------
; 1652 | else if( (xCoord > rectR) || (xCoord < rectL) )       //       otherwis
;     | e, behaves like OPTRECTWIN...                                          
; 1653 | {                                                     //       (see det
;     | ailed comments above)                                                  
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1652| 
   [!A1]   CMPLT   .L1X    A10,B8,A1         ; |1652| 
   [!A1]   B       .S1     L84               ; |1652| 
           NOP             5
           ; BRANCH OCCURS                   ; |1652| 
;** --------------------------------------------------------------------------*
	.line	933
;------------------------------------------------------------------------------
; 1654 | if( xCoord > rectR ) u16Over = xCoord - rectR;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1654| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |1654| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1654| 
	.line	934
;------------------------------------------------------------------------------
; 1655 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |1655| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1655| 
	.line	935
;------------------------------------------------------------------------------
; 1656 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1656| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL470,B3          ; |1656| 
           MVKH    .S2     RL470,B3          ; |1656| 
           NOP             2
RL470:     ; CALL OCCURS                     ; |1656| 
           STH     .D2T1   A4,*+SP(42)       ; |1656| 
	.line	937
;------------------------------------------------------------------------------
; 1658 | if( hv > 0 )  xCoord = rectL + u16Over;                                
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |1658| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1658| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1658| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |1658| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |1658| 
	.line	938
;------------------------------------------------------------------------------
; 1659 | else          xCoord = rectR - u16Over;                                
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1659| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |1659| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |1659| 
	.line	940
;------------------------------------------------------------------------------
; 1661 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1661| 
           MVKL    .S2     RL472,B3          ; |1661| 
           MVKH    .S2     RL472,B3          ; |1661| 
           NOP             3
RL472:     ; CALL OCCURS                     ; |1661| 
	.line	941
;------------------------------------------------------------------------------
; 1662 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1662| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL474,B3          ; |1662| 
           MVKH    .S2     RL474,B3          ; |1662| 
           NOP             2
RL474:     ; CALL OCCURS                     ; |1662| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1662| 
	.line	942
           B       .S1     L85               ; |1663| 
           NOP             5
           ; BRANCH OCCURS                   ; |1663| 
;** --------------------------------------------------------------------------*
L84:    
	.line	943
;------------------------------------------------------------------------------
; 1664 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1664| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |1664| 
   [!A1]   B       .S1     L85               ; |1664| 
           NOP             5
           ; BRANCH OCCURS                   ; |1664| 
;** --------------------------------------------------------------------------*
	.line	945
;------------------------------------------------------------------------------
; 1666 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1666| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |1666| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1666| 
	.line	946
;------------------------------------------------------------------------------
; 1667 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |1667| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1667| 
	.line	947
;------------------------------------------------------------------------------
; 1668 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1668| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL476,B3          ; |1668| 
           MVKH    .S2     RL476,B3          ; |1668| 
           NOP             2
RL476:     ; CALL OCCURS                     ; |1668| 
           STH     .D2T1   A4,*+SP(42)       ; |1668| 
	.line	949
;------------------------------------------------------------------------------
; 1670 | if( vv > 0 )  yCoord = rectD + u16Over;                                
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1670| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1670| 
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1670| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |1670| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |1670| 
	.line	950
;------------------------------------------------------------------------------
; 1671 | else          yCoord = rectU - u16Over;                                
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1671| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |1671| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |1671| 
	.line	952
;------------------------------------------------------------------------------
; 1673 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1673| 
           MVKL    .S2     RL478,B3          ; |1673| 
           MVKH    .S2     RL478,B3          ; |1673| 
           NOP             3
RL478:     ; CALL OCCURS                     ; |1673| 
	.line	953
;------------------------------------------------------------------------------
; 1674 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1674| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL480,B3          ; |1674| 
           MVKH    .S2     RL480,B3          ; |1674| 
           NOP             2
RL480:     ; CALL OCCURS                     ; |1674| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1674| 
;** --------------------------------------------------------------------------*
L85:    
	.line	955
;------------------------------------------------------------------------------
; 1676 | *a = xCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |1676| 
	.line	956
;------------------------------------------------------------------------------
; 1677 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |1677| 
	.line	958
;------------------------------------------------------------------------------
; 1679 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1679| 
           LDHU    .D1T1   *A7,A3            ; |1679| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1679| 
           OR      .L1     A3,A0,A0          ; |1679| 
           STW     .D2T1   A0,*+SP(56)       ; |1679| 
	.line	959
;------------------------------------------------------------------------------
; 1680 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1680| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1680| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1680| 
   [!B0]   B       .S1     L87               ; |1680| 
           NOP             5
           ; BRANCH OCCURS                   ; |1680| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L86:    
           LDW     .D2T2   *+SP(76),B4       ; |1680| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1680| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1680| 
   [ B0]   B       .S1     L86               ; |1680| 
           NOP             5
           ; BRANCH OCCURS                   ; |1680| 
;** --------------------------------------------------------------------------*
L87:    
	.line	960
;------------------------------------------------------------------------------
; 1681 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1681| 
           LDW     .D2T2   *+SP(72),B5       ; |1681| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |1681| 
	.line	961
;------------------------------------------------------------------------------
; 1682 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B6       ; |1682| 
           LDW     .D2T2   *+SP(192),B5      ; |1682| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1682| 
	.line	962
;------------------------------------------------------------------------------
; 1683 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1683| 
	.line	963
;------------------------------------------------------------------------------
; 1684 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1684| 
	.line	964
;------------------------------------------------------------------------------
; 1685 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1685| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1685| 
           STW     .D2T2   B4,*+SP(140)      ; |1685| 
	.line	965
           CMPLTU  .L1     A9,A14,A1         ; |1686| 
   [ A1]   B       .S1     L82               ; |1686| 
           NOP             5
           ; BRANCH OCCURS                   ; |1686| 
;** --------------------------------------------------------------------------*
	.line	966
           B       .S1     L133              ; |1687| 
           NOP             5
           ; BRANCH OCCURS                   ; |1687| 
;** --------------------------------------------------------------------------*
L88:    
	.line	967
;------------------------------------------------------------------------------
; 1688 | else if( u16Type == DL_NOISEDIR )                           // DL_NOISE
;     | DIR: Similar to DOTLIFEWIN, but dir                                    
; 1689 | {                                                           // of each 
;     | dot is randomly offset from pat dir.                                   
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1688| 
           NOP             4
           CMPEQ   .L2     B4,11,B0          ; |1688| 
   [!B0]   B       .S1     L100              ; |1688| 
           NOP             5
           ; BRANCH OCCURS                   ; |1688| 
;** --------------------------------------------------------------------------*
	.line	969
;------------------------------------------------------------------------------
; 1690 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1690| 
           LDW     .D2T2   *+SP(172),B5      ; |1690| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1690| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1690| 
	.line	970
;------------------------------------------------------------------------------
; 1691 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1691| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1691| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1691| 
	.line	972
;------------------------------------------------------------------------------
; 1693 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1694 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1693| 
           XOR     .L1     1,A0,A1           ; |1693| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1693| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1693| 
	.line	974
;------------------------------------------------------------------------------
; 1695 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(184),A4      ; |1695| 
   [ A1]   LDW     .D2T1   *+SP(104),A3      ; |1695| 
   [ A1]   ZERO    .L1     A0                ; |1695| 
           NOP             3
   [ A1]   STH     .D1T1   A0,*+A4[A3]       ; |1695| 
	.line	975
;------------------------------------------------------------------------------
; 1696 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1696| 
	.line	976
;------------------------------------------------------------------------------
; 1698 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	978
;------------------------------------------------------------------------------
; 1699 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1699| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1699| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1699| 
   [!A1]   ADDK    .S2     84,B4             ; |1699| 
   [!A1]   LDHU    .D2T2   *B4,B6            ; |1699| 
   [!A1]   LDW     .D2T2   *+SP(184),B4      ; |1699| 
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1699| 
           NOP             4
   [!A1]   STH     .D2T2   B6,*+B4[B5]       ; |1699| 
	.line	980
;------------------------------------------------------------------------------
; 1701 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |1701| 
           LDW     .D2T1   *+SP(200),A0      ; |1701| 
           NOP             3
           ADD     .L1X    4,B4,A3           ; |1701| 
           LDHU    .D1T1   *+A0[A3],A0       ; |1701| 
           NOP             4
           STH     .D2T1   A0,*+SP(38)       ; |1701| 
	.line	981
;------------------------------------------------------------------------------
; 1702 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1702| 
           NOP             4
           SHRU    .S2     B4,8,B4           ; |1702| 
           STH     .D2T2   B4,*+SP(38)       ; |1702| 
	.line	982
;------------------------------------------------------------------------------
; 1703 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1703| 
           NOP             4
           EXTU    .S2     B4,24,24,B4       ; |1703| 
           STH     .D2T2   B4,*+SP(38)       ; |1703| 
	.line	983
;------------------------------------------------------------------------------
; 1704 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(38),B4       ; |1704| 
           NOP             4
           STH     .D2T2   B4,*+SP(12)       ; |1704| 
	.line	985
;------------------------------------------------------------------------------
; 1706 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1706| 
           LDW     .D2T2   *+SP(168),B5      ; |1706| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1706| 
           ADDK    .S2     404,B4            ; |1706| 
           LDHU    .D2T2   *B4,B4            ; |1706| 
           NOP             4
           STH     .D2T2   B4,*+SP(44)       ; |1706| 
	.line	986
;------------------------------------------------------------------------------
; 1707 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B4       ; |1707| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1707| 
   [!B0]   MVK     .S2     1,B4              ; |1707| 
   [!B0]   STH     .D2T2   B4,*+SP(44)       ; |1707| 
   [!B0]   B       .S1     L89               ; |1707| 
           NOP             5
           ; BRANCH OCCURS                   ; |1707| 
;** --------------------------------------------------------------------------*
	.line	987
;------------------------------------------------------------------------------
; 1708 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1708| 
           MVK     .S2     32767,B4          ; |1708| 
           NOP             3
           CMPGT   .L2     B5,B4,B0          ; |1708| 
   [ B0]   MVK     .S1     32767,A0          ; |1708| 
   [ B0]   STH     .D2T1   A0,*+SP(44)       ; |1708| 
;** --------------------------------------------------------------------------*
L89:    
	.line	989
;------------------------------------------------------------------------------
; 1710 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1710| 
           MVK     .S2     32767,B4          ; |1710| 
           NOP             3
           CMPEQ   .L2     B5,B4,B0          ; |1710| 
   [ B0]   ZERO    .L2     B4                ; |1710| 
   [ B0]   STH     .D2T2   B4,*+SP(12)       ; |1710| 
	.line	991
;------------------------------------------------------------------------------
; 1712 | u16tmp = (UINT16) parameters->wOuterL[d];                //    dir nois
;     | e offset range, N, in whole deg                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1712| 
           LDW     .D2T2   *+SP(104),B4      ; |1712| 
           NOP             4
           ADDAH   .D2     B5,B4,B6          ; |1712| 
           ADDK    .S2     468,B6            ; |1712| 
           LDHU    .D2T2   *B6,B6            ; |1712| 
           NOP             4
           STH     .D2T2   B6,*+SP(40)       ; |1712| 
	.line	992
;------------------------------------------------------------------------------
; 1713 | u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of int
;     | eger choices in [-N:N]                                                 
;------------------------------------------------------------------------------
           ADDAH   .D2     B5,B4,B5          ; |1713| 
           ADDK    .S2     468,B5            ; |1713| 
           LDHU    .D2T2   *B5,B5            ; |1713| 
           NOP             4
           SHL     .S2     B5,1,B5           ; |1713| 
           ADD     .L2     1,B5,B5           ; |1713| 
           STH     .D2T2   B5,*+SP(38)       ; |1713| 
	.line	994
;------------------------------------------------------------------------------
; 1715 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, choose                                           
; 1716 | {                                                        //    new rand
;     | om offset directions for each dot                                      
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(188),A3      ; |1715| 
           MV      .L1X    B4,A0
           NOP             3
           LDH     .D1T1   *+A3[A0],A0       ; |1715| 
           NOP             4
           CMPGT   .L1     A0,0,A1           ; |1715| 
   [ A1]   B       .S1     L92               ; |1715| 
           NOP             5
           ; BRANCH OCCURS                   ; |1715| 
;** --------------------------------------------------------------------------*
	.line	996
;------------------------------------------------------------------------------
; 1717 | j = nVisDotsPerTgt[d];                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(184),B5      ; |1717| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1717| 
           NOP             4
           STW     .D2T2   B5,*+SP(88)       ; |1717| 
	.line	997
;------------------------------------------------------------------------------
; 1718 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |1718| 
           STW     .D2T2   B4,*+SP(92)       ; |1718| 
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L91               ; |1718| 
           NOP             5
           ; BRANCH OCCURS                   ; |1718| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L90:    
	.line	999
;------------------------------------------------------------------------------
; 1720 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose rand
;     | om offset dir in [-N:N]                                                
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum2      ; |1720| 
           MVKL    .S2     RL484,B3          ; |1720| 
           MVKH    .S2     RL484,B3          ; |1720| 
           NOP             3
RL484:     ; CALL OCCURS                     ; |1720| 
           B       .S1     __remi            ; |1720| 
           LDHU    .D2T2   *+SP(38),B4       ; 
           MVKL    .S2     RL486,B3          ; |1720| 
           MVKH    .S2     RL486,B3          ; |1720| 
           NOP             2
RL486:     ; CALL OCCURS                     ; |1720| 
           MV      .L2X    A4,B11            ; 
	.line	1000
;------------------------------------------------------------------------------
; 1721 | i32val -= (INT32) u16tmp;                          //       NOTE USE OF
;     |  DEDICATED RAND# GENERATOR                                             
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(40),B4       ; |1721| 
           NOP             4
           SUB     .L2     B11,B4,B11        ; |1721| 
	.line	1001
;------------------------------------------------------------------------------
; 1722 | i32val *= 10;                                      //       offset dir 
;     | in deg/10                                                              
;------------------------------------------------------------------------------
           SHL     .S2     B11,3,B4          ; |1722| 
           ADDAH   .D2     B4,B11,B11        ; |1722| 
	.line	1002
;------------------------------------------------------------------------------
; 1723 | *(nextDotNoise+k) = (INT16) i32val;                //       save new of
;     | fset dir for every dot                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B5       ; |1723| 
           LDW     .D2T2   *+SP(148),B4      ; |1723| 
           NOP             4
           STH     .D2T2   B11,*+B4[B5]      ; |1723| 
	.line	1003
           LDW     .D2T2   *+SP(92),B4       ; |1724| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |1724| 
           STW     .D2T2   B4,*+SP(92)       ; |1724| 
           LDW     .D2T2   *+SP(88),B5       ; |1724| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L90               ; |1724| 
           NOP             5
           ; BRANCH OCCURS                   ; |1724| 
;** --------------------------------------------------------------------------*
L91:    
	.line	1005
;------------------------------------------------------------------------------
; 1726 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1726| 
           LDW     .D2T2   *+SP(168),B5      ; |1726| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |1726| 
           ADDK    .S2     532,B5            ; |1726| 
           LDH     .D2T2   *B5,B6            ; |1726| 
           LDW     .D2T2   *+SP(188),B5      ; |1726| 
           NOP             4
           STH     .D2T2   B6,*+B5[B4]       ; |1726| 
;** --------------------------------------------------------------------------*
L92:    
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
           LDW     .D2T2   *+SP(188),B5      ; |1729| 
           LDW     .D2T2   *+SP(104),B4      ; |1729| 
           LDW     .D2T2   *+SP(108),B7      ; |1729| 
           LDW     .D2T2   *+SP(200),B6      ; |1729| 
           NOP             3

           ADDAH   .D2     B5,B4,B4          ; |1729| 
||         ADD     .L2     4,B7,B5           ; |1729| 

           LDH     .D2T2   *+B6[B5],B5       ; |1729| 
           LDH     .D2T2   *B4,B6            ; |1729| 
           NOP             3
           EXTU    .S2     B5,24,24,B5       ; |1729| 
           SUBAH   .D2     B6,B5,B5          ; |1729| 
           STH     .D2T2   B5,*B4            ; |1729| 
	.line	1019
;------------------------------------------------------------------------------
; 1740 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
           MVK     .S1     6,A0              ; |1740| 
           STH     .D2T1   A0,*+SP(26)       ; |1740| 
	.line	1020
;------------------------------------------------------------------------------
; 1741 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B5        ; |1741| 
           MVK     .S2     10000,B4          ; |1741| 
           NOP             3
           CMPLT   .L2     B5,B4,B0          ; |1741| 
	.line	1022
;------------------------------------------------------------------------------
; 1743 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
   [!B0]   LDH     .D2T2   *+SP(6),B4        ; |1743| 
           NOP             4
   [!B0]   ADDK    .S2     -10000,B4         ; |1743| 
   [!B0]   STH     .D2T2   B4,*+SP(6)        ; |1743| 
	.line	1023
;------------------------------------------------------------------------------
; 1744 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
   [!B0]   ZERO    .L2     B4                ; |1744| 
   [!B0]   STH     .D2T2   B4,*+SP(26)       ; |1744| 
	.line	1026
;------------------------------------------------------------------------------
; 1747 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1747| 
   [!A1]   B       .S1     L133              ; |1747| 
           NOP             5
           ; BRANCH OCCURS                   ; |1747| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L93
;** --------------------------------------------------------------------------*
L93:    
	.line	1028
;------------------------------------------------------------------------------
; 1749 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1749| 
           LDH     .D2T2   *+SP(12),B6       ; |1749| 
           NOP             3
           LDH     .D2T2   *B4,B5            ; |1749| 
           NOP             4
           SUB     .L2     B5,B6,B5          ; |1749| 
           STH     .D2T2   B5,*B4            ; |1749| 
	.line	1029
;------------------------------------------------------------------------------
; 1750 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 1751 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1750| 
           NOP             4
           LDH     .D2T2   *B4,B5            ; |1750| 
           NOP             4
           CMPLT   .L2     B5,0,B0           ; |1750| 
   [!B0]   B       .S1     L94               ; |1750| 
           NOP             5
           ; BRANCH OCCURS                   ; |1750| 
;** --------------------------------------------------------------------------*
	.line	1031
;------------------------------------------------------------------------------
; 1752 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(44),B5       ; |1752| 
           NOP             4
           STH     .D2T2   B5,*B4            ; |1752| 
	.line	1032
;------------------------------------------------------------------------------
; 1753 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1753| 
           MVKL    .S2     RL488,B3          ; |1753| 
           MVKH    .S2     RL488,B3          ; |1753| 
           NOP             3
RL488:     ; CALL OCCURS                     ; |1753| 
           MV      .L1     A4,A10            ; 
	.line	1033
;------------------------------------------------------------------------------
; 1754 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1754| 
           MVKL    .S2     RL490,B3          ; |1754| 
           MVKH    .S2     RL490,B3          ; |1754| 
           NOP             3
RL490:     ; CALL OCCURS                     ; |1754| 
           MV      .L1     A4,A11            ; 
	.line	1034
;------------------------------------------------------------------------------
; 1755 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1755| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL492,B3          ; |1755| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL492,B3          ; |1755| 
           NOP             1
RL492:     ; CALL OCCURS                     ; |1755| 
           ADD     .L1X    A4,B8,A0          ; 
           STH     .D1T1   A0,*A9            ; |1755| 
	.line	1035
;------------------------------------------------------------------------------
; 1756 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1756| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL494,B3          ; |1756| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL494,B3          ; |1756| 
           NOP             1
RL494:     ; CALL OCCURS                     ; |1756| 
           ADD     .L1X    A4,B9,A0          ; 
           STH     .D1T1   A0,*A7            ; |1756| 
;** --------------------------------------------------------------------------*
L94:    
	.line	1038
;------------------------------------------------------------------------------
; 1759 | i32val = (INT32) *nextDotNoise;                       //       get nois
;     | e offset dir for this dot                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |1759| 
           NOP             4
           LDH     .D2T2   *B4,B11           ; |1759| 
           NOP             4
	.line	1039
;------------------------------------------------------------------------------
; 1760 | i32val += (INT32) vv;                                 //       dot thet
;     | a = offset + pattern theta                                             
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1760| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |1760| 
	.line	1040
;------------------------------------------------------------------------------
; 1761 | if( i32val < 0 ) i32val += 3600;                      //       ensure d
;     | ir lies in [0..3600) deg/10                                            
;------------------------------------------------------------------------------
           CMPLT   .L2     B11,0,B0          ; |1761| 
   [ B0]   ADDK    .S2     3600,B11          ; |1761| 
   [ B0]   B       .S1     L95               ; |1761| 
           NOP             5
           ; BRANCH OCCURS                   ; |1761| 
;** --------------------------------------------------------------------------*
	.line	1041
;------------------------------------------------------------------------------
; 1762 | else i32val = i32val % 3600;                                           
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1762| 
           MVKL    .S2     RL496,B3          ; |1762| 
           MVK     .S2     0xe10,B4          ; |1762| 
           MVKH    .S2     RL496,B3          ; |1762| 
           MV      .L1X    B11,A4            ; |1762| 
           NOP             1
RL496:     ; CALL OCCURS                     ; |1762| 
           MV      .L2X    A4,B11            ; 
;** --------------------------------------------------------------------------*
L95:    
	.line	1042
;------------------------------------------------------------------------------
; 1763 | i16Theta = (INT16) i32val;                                             
;------------------------------------------------------------------------------
           STH     .D2T2   B11,*+SP(24)      ; |1763| 
	.line	1044
;------------------------------------------------------------------------------
; 1765 | i32val = (INT32) hv;                                  //       Rmm*2^Q,
;     |  Q=10 or 16                                                            
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B11       ; |1765| 
           NOP             4
	.line	1045
;------------------------------------------------------------------------------
; 1766 | i32val *= (INT32) cosLUT[i16Theta];                   //       (Rmm*cos
;     | (theta)) * 2^(Q+10)                                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(216),B5      ; |1766| 
           LDH     .D2T2   *+SP(24),B4       ; |1766| 
           NOP             4
           LDH     .D2T2   *+B5[B4],B5       ; |1766| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |1766| 
           MPYSU   .M2     B5,B11,B5         ; |1766| 
           SHL     .S2     B4,16,B4          ; |1766| 
           ADD     .L2     B5,B4,B11         ; |1766| 
	.line	1046
;------------------------------------------------------------------------------
; 1767 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 1768 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1767| 
           LDHU    .D2T2   *+SP(32),B4       ; 
           MVKL    .S2     RL498,B3          ; |1767| 
           MV      .L1X    B11,A4            ; |1767| 
           MVKH    .S2     RL498,B3          ; |1767| 
           NOP             1
RL498:     ; CALL OCCURS                     ; |1767| 
           MV      .L2X    A4,B11            ; 
	.line	1048
;------------------------------------------------------------------------------
; 1769 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |1769| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |1769| 
	.line	1049
;------------------------------------------------------------------------------
; 1770 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(156),B4      ; |1770| 
           NOP             4
           LDH     .D2T2   *B4,B5            ; |1770| 
           NOP             4
           ADD     .L2     B5,B11,B11        ; |1770| 
	.line	1050
;------------------------------------------------------------------------------
; 1771 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |1771| 
	.line	1051
;------------------------------------------------------------------------------
; 1772 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1772| 
	.line	1052
;------------------------------------------------------------------------------
; 1773 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 1774 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1775 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B5        ; |1773| 
||         LDHU    .D1T1   *A9,A0            ; |1773| 

           NOP             4

           ADD     .L1X    B5,A0,A0          ; |1773| 
||         EXT     .S2     B11,16,16,B5      ; |1773| 

           ADD     .L1X    B5,A0,A0          ; |1773| 
           EXTU    .S1     A0,16,16,A10      ; |1773| 
	.line	1056
;------------------------------------------------------------------------------
; 1777 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |1777| 
	.line	1057
;------------------------------------------------------------------------------
; 1778 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 1780 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B5       ; |1778| 
           NOP             4
           SUB     .L2     B5,B11,B5         ; |1778| 
           STH     .D2T2   B5,*B4            ; |1778| 
	.line	1060
;------------------------------------------------------------------------------
; 1781 | i32val = (INT32) hv;                                                   
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B11       ; |1781| 
           NOP             4
	.line	1061
;------------------------------------------------------------------------------
; 1782 | i32val *= (INT32) sinLUT[i16Theta];                                    
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(24),B4       ; |1782| 
           LDW     .D2T2   *+SP(212),B5      ; |1782| 
           NOP             4
           LDH     .D2T2   *+B5[B4],B5       ; |1782| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |1782| 
           MPYSU   .M2     B5,B11,B5         ; |1782| 
           SHL     .S2     B4,16,B4          ; |1782| 
           ADD     .L2     B5,B4,B11         ; |1782| 
	.line	1062
;------------------------------------------------------------------------------
; 1783 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1783| 
           LDHU    .D2T2   *+SP(34),B4       ; 
           MVKL    .S2     RL500,B3          ; |1783| 
           MV      .L1X    B11,A4            ; |1783| 
           MVKH    .S2     RL500,B3          ; |1783| 
           NOP             1
RL500:     ; CALL OCCURS                     ; |1783| 
           MV      .L2X    A4,B11            ; 
	.line	1063
;------------------------------------------------------------------------------
; 1784 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |1784| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |1784| 
	.line	1064
;------------------------------------------------------------------------------
; 1785 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B5      ; |1785| 
           NOP             4
           LDH     .D2T2   *B5,B4            ; |1785| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |1785| 
	.line	1065
;------------------------------------------------------------------------------
; 1786 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |1786| 
	.line	1066
;------------------------------------------------------------------------------
; 1787 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1787| 
	.line	1067
;------------------------------------------------------------------------------
; 1788 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(10),B4       ; |1788| 
||         LDHU    .D1T1   *A7,A0            ; |1788| 

           NOP             4

           ADD     .L1X    B4,A0,A0          ; |1788| 
||         EXT     .S2     B11,16,16,B4      ; |1788| 

           ADD     .L1X    B4,A0,A0          ; |1788| 
           EXTU    .S1     A0,16,16,A11      ; |1788| 
	.line	1068
;------------------------------------------------------------------------------
; 1789 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |1789| 
	.line	1069
;------------------------------------------------------------------------------
; 1790 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |1790| 
           NOP             4
           SUB     .L2     B4,B11,B4         ; |1790| 
           STH     .D2T2   B4,*B5            ; |1790| 
	.line	1071
;------------------------------------------------------------------------------
; 1792 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 1793 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1792| 
   [!A1]   CMPLT   .L1X    A10,B8,A1         ; |1792| 
   [!A1]   B       .S1     L96               ; |1792| 
           NOP             5
           ; BRANCH OCCURS                   ; |1792| 
;** --------------------------------------------------------------------------*
	.line	1073
;------------------------------------------------------------------------------
; 1794 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1794| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |1794| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1794| 
	.line	1074
;------------------------------------------------------------------------------
; 1795 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |1795| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1795| 
	.line	1075
;------------------------------------------------------------------------------
; 1796 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1796| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL502,B3          ; |1796| 
           MVKH    .S2     RL502,B3          ; |1796| 
           NOP             2
RL502:     ; CALL OCCURS                     ; |1796| 
           STH     .D2T1   A4,*+SP(42)       ; |1796| 
	.line	1077
;------------------------------------------------------------------------------
; 1798 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1798| 
           LDH     .D2T2   *+SP(8),B4        ; |1798| 
           NOP             3
           SUB     .L1     A10,A0,A0         ; |1798| 
           CMPGT   .L2X    A0,B4,B0          ; |1798| 
	.line	1078
;------------------------------------------------------------------------------
; 1799 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1799| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |1799| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |1799| 
	.line	1079
;------------------------------------------------------------------------------
; 1800 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1800| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |1800| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |1800| 
	.line	1081
;------------------------------------------------------------------------------
; 1802 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1802| 
           MVKL    .S2     RL504,B3          ; |1802| 
           MVKH    .S2     RL504,B3          ; |1802| 
           NOP             3
RL504:     ; CALL OCCURS                     ; |1802| 
	.line	1082
;------------------------------------------------------------------------------
; 1803 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1803| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL506,B3          ; |1803| 
           MVKH    .S2     RL506,B3          ; |1803| 
           NOP             2
RL506:     ; CALL OCCURS                     ; |1803| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1803| 
	.line	1083
           B       .S1     L97               ; |1804| 
           NOP             5
           ; BRANCH OCCURS                   ; |1804| 
;** --------------------------------------------------------------------------*
L96:    
	.line	1084
;------------------------------------------------------------------------------
; 1805 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1805| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |1805| 
   [!A1]   B       .S1     L97               ; |1805| 
           NOP             5
           ; BRANCH OCCURS                   ; |1805| 
;** --------------------------------------------------------------------------*
	.line	1086
;------------------------------------------------------------------------------
; 1807 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1807| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |1807| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1807| 
	.line	1087
;------------------------------------------------------------------------------
; 1808 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |1808| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1808| 
	.line	1088
;------------------------------------------------------------------------------
; 1809 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1809| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL508,B3          ; |1809| 
           MVKH    .S2     RL508,B3          ; |1809| 
           NOP             2
RL508:     ; CALL OCCURS                     ; |1809| 
           STH     .D2T1   A4,*+SP(42)       ; |1809| 
	.line	1090
;------------------------------------------------------------------------------
; 1811 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A7,A0            ; |1811| 
           LDH     .D2T2   *+SP(10),B4       ; |1811| 
           NOP             3
           SUB     .L1     A11,A0,A0         ; |1811| 
           CMPGT   .L2X    A0,B4,B0          ; |1811| 
	.line	1091
;------------------------------------------------------------------------------
; 1812 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1812| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |1812| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |1812| 
	.line	1092
;------------------------------------------------------------------------------
; 1813 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1813| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |1813| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |1813| 
	.line	1094
;------------------------------------------------------------------------------
; 1815 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1815| 
           MVKL    .S2     RL510,B3          ; |1815| 
           MVKH    .S2     RL510,B3          ; |1815| 
           NOP             3
RL510:     ; CALL OCCURS                     ; |1815| 
	.line	1095
;------------------------------------------------------------------------------
; 1816 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1816| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL512,B3          ; |1816| 
           MVKH    .S2     RL512,B3          ; |1816| 
           NOP             2
RL512:     ; CALL OCCURS                     ; |1816| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1816| 
;** --------------------------------------------------------------------------*
L97:    
	.line	1098
;------------------------------------------------------------------------------
; 1819 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |1819| 
	.line	1099
;------------------------------------------------------------------------------
; 1820 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |1820| 
	.line	1101
;------------------------------------------------------------------------------
; 1822 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1822| 
           LDHU    .D1T1   *A7,A3            ; |1822| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1822| 
           OR      .L1     A3,A0,A0          ; |1822| 
           STW     .D2T1   A0,*+SP(56)       ; |1822| 
	.line	1102
;------------------------------------------------------------------------------
; 1823 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1823| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1823| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1823| 
   [!B0]   B       .S1     L99               ; |1823| 
           NOP             5
           ; BRANCH OCCURS                   ; |1823| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L98:    
           LDW     .D2T2   *+SP(76),B4       ; |1823| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1823| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1823| 
   [ B0]   B       .S1     L98               ; |1823| 
           NOP             5
           ; BRANCH OCCURS                   ; |1823| 
;** --------------------------------------------------------------------------*
L99:    
	.line	1103
;------------------------------------------------------------------------------
; 1824 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1824| 
           LDW     .D2T1   *+SP(72),A0       ; |1824| 
           NOP             4
           STW     .D1T2   B4,*A0            ; |1824| 
	.line	1104
;------------------------------------------------------------------------------
; 1825 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B6      ; |1825| 
           LDW     .D2T2   *+SP(56),B4       ; |1825| 
           MV      .L2X    A12,B5            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B4,*+B6[B5]       ; |1825| 
	.line	1105
;------------------------------------------------------------------------------
; 1826 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1826| 
	.line	1106
;------------------------------------------------------------------------------
; 1827 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1827| 
	.line	1107
;------------------------------------------------------------------------------
; 1828 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1828| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1828| 
           STW     .D2T2   B4,*+SP(140)      ; |1828| 
	.line	1108
;------------------------------------------------------------------------------
; 1829 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |1829| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1829| 
           STW     .D2T2   B4,*+SP(148)      ; |1829| 
	.line	1109
;------------------------------------------------------------------------------
; 1830 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(156),B4      ; |1830| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1830| 
           STW     .D2T2   B4,*+SP(156)      ; |1830| 
	.line	1110
;------------------------------------------------------------------------------
; 1831 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B4      ; |1831| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1831| 
           STW     .D2T2   B4,*+SP(164)      ; |1831| 
	.line	1111
           CMPLTU  .L1     A9,A14,A1         ; |1832| 
   [ A1]   B       .S1     L93               ; |1832| 
           NOP             5
           ; BRANCH OCCURS                   ; |1832| 
;** --------------------------------------------------------------------------*
	.line	1112
           B       .S1     L133              ; |1833| 
           NOP             5
           ; BRANCH OCCURS                   ; |1833| 
;** --------------------------------------------------------------------------*
L100:    
	.line	1113
;------------------------------------------------------------------------------
; 1834 | else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISE
;     | SPEED #1: Like DL_NOISEDIR, but                                        
; 1835 | (parameters->wOuterB[d] == 0))                      // Rdot = Rpat + U*
;     | Rpat/100, where U is chosen                                            
; 1836 | {                                                           // randomly
;     |  from [-N..N].                                                         
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1834| 
           NOP             4
           CMPEQ   .L2     B4,13,B0          ; |1834| 
   [ B0]   LDW     .D2T2   *+SP(168),B5      ; |1834| 
   [ B0]   LDW     .D2T2   *+SP(104),B4      ; |1834| 
           NOP             4
   [ B0]   ADDAH   .D2     B5,B4,B4          ; |1834| 
   [ B0]   ADDK    .S2     596,B4            ; |1834| 
   [ B0]   LDHU    .D2T2   *B4,B4            ; |1834| 
           NOP             4
   [ B0]   CMPEQ   .L2     B4,0,B0           ; |1834| 
   [!B0]   B       .S1     L111              ; |1834| 
           NOP             5
           ; BRANCH OCCURS                   ; |1834| 
;** --------------------------------------------------------------------------*
	.line	1116
;------------------------------------------------------------------------------
; 1837 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1837| 
           LDW     .D2T2   *+SP(172),B5      ; |1837| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1837| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1837| 
	.line	1117
;------------------------------------------------------------------------------
; 1838 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1838| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1838| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1838| 
	.line	1119
;------------------------------------------------------------------------------
; 1840 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1841 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1840| 
           XOR     .L1     1,A0,A1           ; |1840| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1840| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1840| 
	.line	1121
;------------------------------------------------------------------------------
; 1842 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(104),A4      ; |1842| 
   [ A1]   LDW     .D2T1   *+SP(184),A3      ; |1842| 
   [ A1]   ZERO    .L1     A0                ; |1842| 
           NOP             3
   [ A1]   STH     .D1T1   A0,*+A3[A4]       ; |1842| 
	.line	1122
;------------------------------------------------------------------------------
; 1843 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1843| 
	.line	1123
;------------------------------------------------------------------------------
; 1845 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	1125
;------------------------------------------------------------------------------
; 1846 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1846| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1846| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1846| 
   [!A1]   ADDK    .S2     84,B4             ; |1846| 
   [!A1]   LDHU    .D2T2   *B4,B6            ; |1846| 
   [!A1]   LDW     .D2T2   *+SP(184),B4      ; |1846| 
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1846| 
           NOP             4
   [!A1]   STH     .D2T2   B6,*+B4[B5]       ; |1846| 
	.line	1127
;------------------------------------------------------------------------------
; 1848 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |1848| 
           LDW     .D2T1   *+SP(200),A3      ; |1848| 
           NOP             3
           ADD     .L1X    4,B4,A0           ; |1848| 
           LDHU    .D1T1   *+A3[A0],A0       ; |1848| 
           NOP             4
           STH     .D2T1   A0,*+SP(38)       ; |1848| 
	.line	1128
;------------------------------------------------------------------------------
; 1849 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1849| 
           NOP             4
           SHRU    .S2     B4,8,B4           ; |1849| 
           STH     .D2T2   B4,*+SP(38)       ; |1849| 
	.line	1129
;------------------------------------------------------------------------------
; 1850 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1850| 
           NOP             4
           EXTU    .S2     B4,24,24,B4       ; |1850| 
           STH     .D2T2   B4,*+SP(38)       ; |1850| 
	.line	1130
;------------------------------------------------------------------------------
; 1851 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(38),B4       ; |1851| 
           NOP             4
           STH     .D2T2   B4,*+SP(12)       ; |1851| 
	.line	1132
;------------------------------------------------------------------------------
; 1853 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1853| 
           LDW     .D2T2   *+SP(104),B4      ; |1853| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |1853| 
           ADDK    .S2     404,B4            ; |1853| 
           LDHU    .D2T2   *B4,B4            ; |1853| 
           NOP             4
           STH     .D2T2   B4,*+SP(44)       ; |1853| 
	.line	1133
;------------------------------------------------------------------------------
; 1854 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B4       ; |1854| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |1854| 
   [!B0]   MVK     .S2     1,B4              ; |1854| 
   [!B0]   STH     .D2T2   B4,*+SP(44)       ; |1854| 
   [!B0]   B       .S1     L101              ; |1854| 
           NOP             5
           ; BRANCH OCCURS                   ; |1854| 
;** --------------------------------------------------------------------------*
	.line	1134
;------------------------------------------------------------------------------
; 1855 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1855| 
           MVK     .S2     32767,B4          ; |1855| 
           NOP             3
           CMPGT   .L2     B5,B4,B0          ; |1855| 
   [ B0]   MVK     .S1     32767,A0          ; |1855| 
   [ B0]   STH     .D2T1   A0,*+SP(44)       ; |1855| 
;** --------------------------------------------------------------------------*
L101:    
	.line	1136
;------------------------------------------------------------------------------
; 1857 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |1857| 
           MVK     .S2     32767,B4          ; |1857| 
           NOP             3
           CMPEQ   .L2     B5,B4,B0          ; |1857| 
   [ B0]   ZERO    .L2     B4                ; |1857| 
   [ B0]   STH     .D2T2   B4,*+SP(12)       ; |1857| 
	.line	1138
;------------------------------------------------------------------------------
; 1859 | u16tmp = (UINT16) parameters->wOuterL[d];                //    speed no
;     | ise offset range, N, as %-age of                                       
; 1860 | //    nominal speed, in 1% increments                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |1859| 
           LDW     .D2T2   *+SP(104),B4      ; |1859| 
           NOP             4
           ADDAH   .D2     B5,B4,B6          ; |1859| 
           ADDK    .S2     468,B6            ; |1859| 
           LDHU    .D2T2   *B6,B6            ; |1859| 
           NOP             4
           STH     .D2T2   B6,*+SP(40)       ; |1859| 
	.line	1140
;------------------------------------------------------------------------------
; 1861 | u16Dummy = (UINT16) parameters->wOuterL[d] * 2 + 1;      //    # of int
;     | eger choices in [-N:N]                                                 
;------------------------------------------------------------------------------
           ADDAH   .D2     B5,B4,B5          ; |1861| 
           ADDK    .S2     468,B5            ; |1861| 
           LDHU    .D2T2   *B5,B5            ; |1861| 
           NOP             4
           SHL     .S2     B5,1,B5           ; |1861| 
           ADD     .L2     1,B5,B5           ; |1861| 
           STH     .D2T2   B5,*+SP(38)       ; |1861| 
	.line	1142
;------------------------------------------------------------------------------
; 1863 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, choose                                           
; 1864 | {                                                        //    new rand
;     | om offset speed %s for each dot                                        
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(188),A3      ; |1863| 
           MV      .L1X    B4,A0
           NOP             3
           LDH     .D1T1   *+A3[A0],A0       ; |1863| 
           NOP             4
           CMPGT   .L1     A0,0,A1           ; |1863| 
   [ A1]   B       .S1     L104              ; |1863| 
           NOP             5
           ; BRANCH OCCURS                   ; |1863| 
;** --------------------------------------------------------------------------*
	.line	1144
;------------------------------------------------------------------------------
; 1865 | j = nVisDotsPerTgt[d];                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(184),B5      ; |1865| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B4       ; |1865| 
           NOP             4
           STW     .D2T2   B4,*+SP(88)       ; |1865| 
	.line	1145
;------------------------------------------------------------------------------
; 1866 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
           ZERO    .L2     B5                ; |1866| 
           STW     .D2T2   B5,*+SP(92)       ; |1866| 
           CMPLTU  .L2     B5,B4,B0          ; 
   [!B0]   B       .S1     L103              ; |1866| 
           NOP             5
           ; BRANCH OCCURS                   ; |1866| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L102:    
	.line	1147
;------------------------------------------------------------------------------
; 1868 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //       choose rand
;     | om offset speed % in [-N:N]                                            
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum2      ; |1868| 
           MVKL    .S2     RL516,B3          ; |1868| 
           MVKH    .S2     RL516,B3          ; |1868| 
           NOP             3
RL516:     ; CALL OCCURS                     ; |1868| 
           B       .S1     __remi            ; |1868| 
           LDHU    .D2T2   *+SP(38),B4       ; 
           MVKL    .S2     RL518,B3          ; |1868| 
           MVKH    .S2     RL518,B3          ; |1868| 
           NOP             2
RL518:     ; CALL OCCURS                     ; |1868| 
           MV      .L2X    A4,B11            ; 
	.line	1148
;------------------------------------------------------------------------------
; 1869 | i32val -= (INT32) u16tmp;                          //       NOTE USE OF
;     |  DEDICATED RAND# GENERATOR                                             
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(40),B4       ; |1869| 
           NOP             4
           SUB     .L2     B11,B4,B11        ; |1869| 
	.line	1149
;------------------------------------------------------------------------------
; 1870 | *(nextDotNoise+k) = (INT16) i32val;                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B5       ; |1870| 
           LDW     .D2T2   *+SP(148),B4      ; |1870| 
           NOP             4
           STH     .D2T2   B11,*+B4[B5]      ; |1870| 
	.line	1150
           LDW     .D2T2   *+SP(92),B4       ; |1871| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |1871| 
           STW     .D2T2   B5,*+SP(92)       ; |1871| 
           LDW     .D2T2   *+SP(88),B4       ; |1871| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L102              ; |1871| 
           NOP             5
           ; BRANCH OCCURS                   ; |1871| 
;** --------------------------------------------------------------------------*
L103:    
	.line	1152
;------------------------------------------------------------------------------
; 1873 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B6      ; |1873| 
           LDW     .D2T2   *+SP(168),B4      ; |1873| 
           LDW     .D2T2   *+SP(188),B5      ; |1873| 
           NOP             3
           ADDAH   .D2     B4,B6,B4          ; |1873| 
           ADDK    .S2     532,B4            ; |1873| 
           LDH     .D2T2   *B4,B7            ; |1873| 
           MV      .L2     B6,B4             ; |1873| 
           NOP             3
           STH     .D2T2   B7,*+B5[B4]       ; |1873| 
;** --------------------------------------------------------------------------*
L104:    
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
           LDW     .D2T2   *+SP(108),B4      ; |1876| 
           LDW     .D2T2   *+SP(188),B7      ; |1876| 
           LDW     .D2T2   *+SP(104),B6      ; |1876| 
           LDW     .D2T2   *+SP(200),B5      ; |1876| 
           NOP             3

           ADDAH   .D2     B7,B6,B6          ; |1876| 
||         ADD     .L2     4,B4,B4           ; |1876| 

           LDH     .D2T2   *+B5[B4],B4       ; |1876| 
           LDH     .D2T2   *B6,B5            ; |1876| 
           NOP             3
           EXTU    .S2     B4,24,24,B4       ; |1876| 
           SUBAH   .D2     B5,B4,B4          ; |1876| 
           STH     .D2T2   B4,*B6            ; |1876| 
	.line	1167
;------------------------------------------------------------------------------
; 1888 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
           MVK     .S1     6,A0              ; |1888| 
           STH     .D2T1   A0,*+SP(26)       ; |1888| 
	.line	1168
;------------------------------------------------------------------------------
; 1889 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1889| 
           MVK     .S2     10000,B5          ; |1889| 
           NOP             3
           CMPLT   .L2     B4,B5,B0          ; |1889| 
	.line	1170
;------------------------------------------------------------------------------
; 1891 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
   [!B0]   LDH     .D2T2   *+SP(6),B4        ; |1891| 
           NOP             4
   [!B0]   ADDK    .S2     -10000,B4         ; |1891| 
   [!B0]   STH     .D2T2   B4,*+SP(6)        ; |1891| 
	.line	1171
;------------------------------------------------------------------------------
; 1892 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
   [!B0]   ZERO    .L2     B4                ; |1892| 
   [!B0]   STH     .D2T2   B4,*+SP(26)       ; |1892| 
	.line	1174
;------------------------------------------------------------------------------
; 1895 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |1895| 
   [!A1]   B       .S1     L133              ; |1895| 
           NOP             5
           ; BRANCH OCCURS                   ; |1895| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L105
;** --------------------------------------------------------------------------*
L105:    
	.line	1176
;------------------------------------------------------------------------------
; 1897 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B6      ; |1897| 
           LDH     .D2T2   *+SP(12),B5       ; |1897| 
           NOP             3
           LDH     .D2T2   *B6,B4            ; |1897| 
           NOP             4
           SUB     .L2     B4,B5,B4          ; |1897| 
           STH     .D2T2   B4,*B6            ; |1897| 
	.line	1177
;------------------------------------------------------------------------------
; 1898 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 1899 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1898| 
           NOP             4
           LDH     .D2T2   *B4,B5            ; |1898| 
           NOP             4
           CMPLT   .L2     B5,0,B0           ; |1898| 
   [!B0]   B       .S1     L106              ; |1898| 
           NOP             5
           ; BRANCH OCCURS                   ; |1898| 
;** --------------------------------------------------------------------------*
	.line	1179
;------------------------------------------------------------------------------
; 1900 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(44),B5       ; |1900| 
           NOP             4
           STH     .D2T2   B5,*B4            ; |1900| 
	.line	1180
;------------------------------------------------------------------------------
; 1901 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1901| 
           MVKL    .S2     RL520,B3          ; |1901| 
           MVKH    .S2     RL520,B3          ; |1901| 
           NOP             3
RL520:     ; CALL OCCURS                     ; |1901| 
           MV      .L1     A4,A10            ; 
	.line	1181
;------------------------------------------------------------------------------
; 1902 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1902| 
           MVKL    .S2     RL522,B3          ; |1902| 
           MVKH    .S2     RL522,B3          ; |1902| 
           NOP             3
RL522:     ; CALL OCCURS                     ; |1902| 
           MV      .L1     A4,A11            ; 
	.line	1182
;------------------------------------------------------------------------------
; 1903 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1903| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL524,B3          ; |1903| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL524,B3          ; |1903| 
           NOP             1
RL524:     ; CALL OCCURS                     ; |1903| 
           ADD     .L1X    A4,B8,A0          ; 
           STH     .D1T1   A0,*A9            ; |1903| 
	.line	1183
;------------------------------------------------------------------------------
; 1904 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1904| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL526,B3          ; |1904| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL526,B3          ; |1904| 
           NOP             1
RL526:     ; CALL OCCURS                     ; |1904| 
           ADD     .L1X    A4,B9,A0          ; 
           STH     .D1T1   A0,*A7            ; |1904| 
;** --------------------------------------------------------------------------*
L106:    
	.line	1186
;------------------------------------------------------------------------------
; 1907 | i32val = (INT32) *nextDotNoise;                       //       get offs
;     | et speed %age N for this dot                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |1907| 
           NOP             4
           LDH     .D2T2   *B4,B11           ; |1907| 
           NOP             4
	.line	1187
;------------------------------------------------------------------------------
; 1908 | i32val *= (INT32) hv;                                 //       compute 
;     | dot R=2^Q*(patR + N*patR/100).                                         
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |1908| 
           NOP             4
           MPYLH   .M2     B4,B11,B5         ; |1908| 
           MPYSU   .M2     B4,B11,B4         ; |1908| 
           SHL     .S2     B5,16,B5          ; |1908| 
           MV      .L1X    B4,A0             ; |1908| 
	.line	1188
;------------------------------------------------------------------------------
; 1909 | i32val /= 100;                                                         
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1909| 
           MVKL    .S2     RL528,B3          ; |1909| 
           MVKH    .S2     RL528,B3          ; |1909| 
           MVK     .S2     0x64,B4           ; |1909| 
           ADD     .L1X    A0,B5,A4          ; 
           NOP             1
RL528:     ; CALL OCCURS                     ; |1909| 
           MV      .L2X    A4,B11            ; 
	.line	1189
;------------------------------------------------------------------------------
; 1910 | i32val += (INT32) hv;                                                  
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |1910| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |1910| 
	.line	1190
;------------------------------------------------------------------------------
; 1911 | x32 = i32val;                                         //       save cuz
;     |  we're going to change i32val                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(16)      ; |1911| 
	.line	1192
;------------------------------------------------------------------------------
; 1913 | i32val *= (INT32) cosLUT[vv];                         //       (Rmm*cos
;     | (theta)) * 2^(Q+10)                                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(216),B5      ; |1913| 
           LDH     .D2T2   *+SP(6),B4        ; |1913| 
           NOP             4
           LDH     .D2T2   *+B5[B4],B5       ; |1913| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |1913| 
           MPYSU   .M2     B5,B11,B5         ; |1913| 
           SHL     .S2     B4,16,B4          ; |1913| 
           ADD     .L2     B5,B4,B11         ; |1913| 
	.line	1193
;------------------------------------------------------------------------------
; 1914 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 1915 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1914| 
           LDHU    .D2T2   *+SP(32),B4       ; 
           MVKL    .S2     RL530,B3          ; |1914| 
           MV      .L1X    B11,A4            ; |1914| 
           MVKH    .S2     RL530,B3          ; |1914| 
           NOP             1
RL530:     ; CALL OCCURS                     ; |1914| 
           MV      .L2X    A4,B11            ; 
	.line	1195
;------------------------------------------------------------------------------
; 1916 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |1916| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |1916| 
	.line	1196
;------------------------------------------------------------------------------
; 1917 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(156),A0      ; |1917| 
           NOP             4
           LDH     .D1T1   *A0,A3            ; |1917| 
           NOP             4
           ADD     .L2X    A3,B11,B11        ; |1917| 
	.line	1197
;------------------------------------------------------------------------------
; 1918 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |1918| 
	.line	1198
;------------------------------------------------------------------------------
; 1919 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1919| 
	.line	1199
;------------------------------------------------------------------------------
; 1920 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 1921 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 1922 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------

           LDHU    .D1T1   *A9,A3            ; |1920| 
||         LDH     .D2T2   *+SP(8),B4        ; |1920| 

           NOP             4

           ADD     .L1X    B4,A3,A3          ; |1920| 
||         EXT     .S2     B11,16,16,B4      ; |1920| 

           ADD     .L1X    B4,A3,A3          ; |1920| 
           EXTU    .S1     A3,16,16,A10      ; |1920| 
	.line	1203
;------------------------------------------------------------------------------
; 1924 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |1924| 
	.line	1204
;------------------------------------------------------------------------------
; 1925 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 1927 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |1925| 
           NOP             4
           SUB     .L2     B4,B11,B4         ; |1925| 
           STH     .D1T2   B4,*A0            ; |1925| 
	.line	1207
;------------------------------------------------------------------------------
; 1928 | i32val = x32;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(16),B11      ; |1928| 
           NOP             4
	.line	1208
;------------------------------------------------------------------------------
; 1929 | i32val *= (INT32) sinLUT[vv];                                          
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |1929| 
           LDW     .D2T2   *+SP(212),B5      ; |1929| 
           NOP             4
           LDH     .D2T2   *+B5[B4],B5       ; |1929| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |1929| 
           MPYSU   .M2     B5,B11,B5         ; |1929| 
           SHL     .S2     B4,16,B4          ; |1929| 
           ADD     .L2     B5,B4,B11         ; |1929| 
	.line	1209
;------------------------------------------------------------------------------
; 1930 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |1930| 
           LDHU    .D2T2   *+SP(34),B4       ; 
           MVKL    .S2     RL532,B3          ; |1930| 
           MV      .L1X    B11,A4            ; |1930| 
           MVKH    .S2     RL532,B3          ; |1930| 
           NOP             1
RL532:     ; CALL OCCURS                     ; |1930| 
           MV      .L2X    A4,B11            ; 
	.line	1210
;------------------------------------------------------------------------------
; 1931 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |1931| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |1931| 
	.line	1211
;------------------------------------------------------------------------------
; 1932 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B6      ; |1932| 
           NOP             4
           LDH     .D2T2   *B6,B4            ; |1932| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |1932| 
	.line	1212
;------------------------------------------------------------------------------
; 1933 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |1933| 
	.line	1213
;------------------------------------------------------------------------------
; 1934 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |1934| 
	.line	1214
;------------------------------------------------------------------------------
; 1935 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------

           LDHU    .D1T1   *A7,A0            ; |1935| 
||         LDH     .D2T2   *+SP(10),B4       ; |1935| 

           NOP             4

           ADD     .L1X    B4,A0,A0          ; |1935| 
||         EXT     .S2     B11,16,16,B4      ; |1935| 

           ADD     .L1X    B4,A0,A0          ; |1935| 
           EXTU    .S1     A0,16,16,A11      ; |1935| 
	.line	1215
;------------------------------------------------------------------------------
; 1936 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |1936| 
	.line	1216
;------------------------------------------------------------------------------
; 1937 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |1937| 
           NOP             4

           SUB     .S2     B4,B11,B5         ; |1937| 
||         MV      .L2     B6,B4             ; |1937| 

           STH     .D2T2   B5,*B4            ; |1937| 
	.line	1218
;------------------------------------------------------------------------------
; 1939 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 1940 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1939| 
   [!A1]   CMPLT   .L1X    A10,B8,A1         ; |1939| 
   [!A1]   B       .S1     L107              ; |1939| 
           NOP             5
           ; BRANCH OCCURS                   ; |1939| 
;** --------------------------------------------------------------------------*
	.line	1220
;------------------------------------------------------------------------------
; 1941 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |1941| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |1941| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1941| 
	.line	1221
;------------------------------------------------------------------------------
; 1942 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |1942| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1942| 
	.line	1222
;------------------------------------------------------------------------------
; 1943 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1943| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL534,B3          ; |1943| 
           MVKH    .S2     RL534,B3          ; |1943| 
           NOP             2
RL534:     ; CALL OCCURS                     ; |1943| 
           STH     .D2T1   A4,*+SP(42)       ; |1943| 
	.line	1224
;------------------------------------------------------------------------------
; 1945 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1945| 
           LDH     .D2T2   *+SP(8),B4        ; |1945| 
           NOP             3
           SUB     .L1     A10,A0,A0         ; |1945| 
           CMPGT   .L2X    A0,B4,B0          ; |1945| 
	.line	1225
;------------------------------------------------------------------------------
; 1946 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1946| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |1946| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |1946| 
	.line	1226
;------------------------------------------------------------------------------
; 1947 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1947| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |1947| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |1947| 
	.line	1228
;------------------------------------------------------------------------------
; 1949 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1949| 
           MVKL    .S2     RL536,B3          ; |1949| 
           MVKH    .S2     RL536,B3          ; |1949| 
           NOP             3
RL536:     ; CALL OCCURS                     ; |1949| 
	.line	1229
;------------------------------------------------------------------------------
; 1950 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1950| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL538,B3          ; |1950| 
           MVKH    .S2     RL538,B3          ; |1950| 
           NOP             2
RL538:     ; CALL OCCURS                     ; |1950| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |1950| 
	.line	1230
           B       .S1     L108              ; |1951| 
           NOP             5
           ; BRANCH OCCURS                   ; |1951| 
;** --------------------------------------------------------------------------*
L107:    
	.line	1231
;------------------------------------------------------------------------------
; 1952 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1952| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |1952| 
   [!A1]   B       .S1     L108              ; |1952| 
           NOP             5
           ; BRANCH OCCURS                   ; |1952| 
;** --------------------------------------------------------------------------*
	.line	1233
;------------------------------------------------------------------------------
; 1954 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |1954| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |1954| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |1954| 
	.line	1234
;------------------------------------------------------------------------------
; 1955 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |1955| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |1955| 
	.line	1235
;------------------------------------------------------------------------------
; 1956 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |1956| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL540,B3          ; |1956| 
           MVKH    .S2     RL540,B3          ; |1956| 
           NOP             2
RL540:     ; CALL OCCURS                     ; |1956| 
           STH     .D2T1   A4,*+SP(42)       ; |1956| 
	.line	1237
;------------------------------------------------------------------------------
; 1958 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A7,A0            ; |1958| 
           LDH     .D2T2   *+SP(10),B4       ; |1958| 
           NOP             3
           SUB     .L1     A11,A0,A0         ; |1958| 
           CMPGT   .L2X    A0,B4,B0          ; |1958| 
	.line	1238
;------------------------------------------------------------------------------
; 1959 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |1959| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |1959| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |1959| 
	.line	1239
;------------------------------------------------------------------------------
; 1960 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |1960| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |1960| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |1960| 
	.line	1241
;------------------------------------------------------------------------------
; 1962 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |1962| 
           MVKL    .S2     RL542,B3          ; |1962| 
           MVKH    .S2     RL542,B3          ; |1962| 
           NOP             3
RL542:     ; CALL OCCURS                     ; |1962| 
	.line	1242
;------------------------------------------------------------------------------
; 1963 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |1963| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL544,B3          ; |1963| 
           MVKH    .S2     RL544,B3          ; |1963| 
           NOP             2
RL544:     ; CALL OCCURS                     ; |1963| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |1963| 
;** --------------------------------------------------------------------------*
L108:    
	.line	1245
;------------------------------------------------------------------------------
; 1966 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |1966| 
	.line	1246
;------------------------------------------------------------------------------
; 1967 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |1967| 
	.line	1248
;------------------------------------------------------------------------------
; 1969 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |1969| 
           LDHU    .D1T1   *A7,A3            ; |1969| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |1969| 
           OR      .L1     A3,A0,A0          ; |1969| 
           STW     .D2T1   A0,*+SP(56)       ; |1969| 
	.line	1249
;------------------------------------------------------------------------------
; 1970 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |1970| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1970| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1970| 
   [!B0]   B       .S1     L110              ; |1970| 
           NOP             5
           ; BRANCH OCCURS                   ; |1970| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L109:    
           LDW     .D2T2   *+SP(76),B4       ; |1970| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |1970| 
           NOP             4
           AND     .L2     1,B4,B0           ; |1970| 
   [ B0]   B       .S1     L109              ; |1970| 
           NOP             5
           ; BRANCH OCCURS                   ; |1970| 
;** --------------------------------------------------------------------------*
L110:    
	.line	1250
;------------------------------------------------------------------------------
; 1971 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |1971| 
           LDW     .D2T1   *+SP(72),A0       ; |1971| 
           NOP             4
           STW     .D1T2   B4,*A0            ; |1971| 
	.line	1251
;------------------------------------------------------------------------------
; 1972 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B5      ; |1972| 
           LDW     .D2T2   *+SP(56),B6       ; |1972| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B6,*+B5[B4]       ; |1972| 
	.line	1252
;------------------------------------------------------------------------------
; 1973 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |1973| 
	.line	1253
;------------------------------------------------------------------------------
; 1974 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |1974| 
	.line	1254
;------------------------------------------------------------------------------
; 1975 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |1975| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1975| 
           STW     .D2T2   B4,*+SP(140)      ; |1975| 
	.line	1255
;------------------------------------------------------------------------------
; 1976 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |1976| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1976| 
           STW     .D2T2   B4,*+SP(148)      ; |1976| 
	.line	1256
;------------------------------------------------------------------------------
; 1977 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(156),B4      ; |1977| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1977| 
           STW     .D2T2   B4,*+SP(156)      ; |1977| 
	.line	1257
;------------------------------------------------------------------------------
; 1978 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B4      ; |1978| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |1978| 
           STW     .D2T2   B4,*+SP(164)      ; |1978| 
	.line	1258
           CMPLTU  .L1     A9,A14,A1         ; |1979| 
   [ A1]   B       .S1     L105              ; |1979| 
           NOP             5
           ; BRANCH OCCURS                   ; |1979| 
;** --------------------------------------------------------------------------*
	.line	1259
           B       .S1     L133              ; |1980| 
           NOP             5
           ; BRANCH OCCURS                   ; |1980| 
;** --------------------------------------------------------------------------*
L111:    
	.line	1260
;------------------------------------------------------------------------------
; 1981 | else if((u16Type == DL_NOISESPEED) &&                       // DL_NOISE
;     | SPEED #2: Like DL_NOISESPEED #1, but                                   
; 1982 | (parameters->wOuterB[d] != 0))                      // Rdot = (Rpat*2^U
;     | )/E(2^U), where U is chosen                                            
; 1983 | {                                                           // randomly
;     |  from [-N..N]...                                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |1981| 
           NOP             4
           CMPEQ   .L2     B4,13,B0          ; |1981| 
   [ B0]   LDW     .D2T2   *+SP(168),B5      ; |1981| 
   [ B0]   LDW     .D2T2   *+SP(104),B4      ; |1981| 
           NOP             4
   [ B0]   ADDAH   .D2     B5,B4,B4          ; |1981| 
   [ B0]   ADDK    .S2     596,B4            ; |1981| 
   [ B0]   LDHU    .D2T2   *B4,B4            ; |1981| 
           NOP             4
   [ B0]   CMPEQ   .L2     B4,0,B4           ; |1981| 
   [ B0]   XOR     .L2     1,B4,B0           ; |1981| 
   [!B0]   B       .S1     L122              ; |1981| 
           NOP             5
           ; BRANCH OCCURS                   ; |1981| 
;** --------------------------------------------------------------------------*
	.line	1263
;------------------------------------------------------------------------------
; 1984 | rectW = hsize[d];                                        //    so we do
;     | n't do repeat array accesses in                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |1984| 
           LDW     .D2T2   *+SP(172),B5      ; |1984| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |1984| 
           NOP             4
           STH     .D2T2   B5,*+SP(28)       ; |1984| 
	.line	1264
;------------------------------------------------------------------------------
; 1985 | rectH = vsize[d];                                        //    the draw
;     |  loop below...                                                         
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(176),A3      ; |1985| 
           MV      .L1X    B4,A0
           NOP             3
           LDHU    .D1T1   *+A3[A0],A0       ; |1985| 
           NOP             4
           STH     .D2T1   A0,*+SP(30)       ; |1985| 
	.line	1266
;------------------------------------------------------------------------------
; 1987 | if( (rectR <= rectL) || (rectU <= rectD) )               //    turn off
;     |  target if target rect is invalid                                      
; 1988 | {                                                        //    due to a
;     |  screen wrap-around                                                    
;------------------------------------------------------------------------------
           CMPGT   .L1X    A8,B8,A0          ; |1987| 
           XOR     .L1     1,A0,A1           ; |1987| 
   [!A1]   CMPGT   .L2X    A13,B9,B4         ; |1987| 
   [!A1]   XOR     .L1X    1,B4,A1           ; |1987| 
	.line	1268
;------------------------------------------------------------------------------
; 1989 | nVisDotsPerTgt[d] = 0;                                                 
;------------------------------------------------------------------------------
   [ A1]   LDW     .D2T1   *+SP(184),A4      ; |1989| 
   [ A1]   LDW     .D2T1   *+SP(104),A0      ; |1989| 
   [ A1]   ZERO    .L1     A3                ; |1989| 
           NOP             3
   [ A1]   STH     .D1T1   A3,*+A4[A0]       ; |1989| 
	.line	1269
;------------------------------------------------------------------------------
; 1990 | a = de;                                                                
;------------------------------------------------------------------------------
   [ A1]   MV      .L1     A14,A9            ; |1990| 
	.line	1270
;------------------------------------------------------------------------------
; 1992 | else                                                     //    otherwis
;     | e, all dots are drawn, since all                                       
;------------------------------------------------------------------------------
	.line	1272
;------------------------------------------------------------------------------
; 1993 | nVisDotsPerTgt[d] = parameters->wNumDots[d];          //    are restric
;     | ted to the tgt window.                                                 
;------------------------------------------------------------------------------
   [!A1]   LDW     .D2T2   *+SP(104),B5      ; |1993| 
   [!A1]   LDW     .D2T2   *+SP(168),B4      ; |1993| 
           NOP             4
   [!A1]   ADDAH   .D2     B4,B5,B4          ; |1993| 
   [!A1]   ADDK    .S2     84,B4             ; |1993| 
   [!A1]   LDHU    .D2T2   *B4,B6            ; |1993| 
   [!A1]   LDW     .D2T2   *+SP(104),B4      ; |1993| 
   [!A1]   LDW     .D2T2   *+SP(184),B5      ; |1993| 
           NOP             4
   [!A1]   STH     .D2T2   B6,*+B5[B4]       ; |1993| 
	.line	1274
;------------------------------------------------------------------------------
; 1995 | u16Dummy = (UINT16) data[cd + NREPS];                    //    extract 
;     | dot life decrement from upper                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |1995| 
           LDW     .D2T1   *+SP(200),A0      ; |1995| 
           NOP             3
           ADD     .L1X    4,B4,A3           ; |1995| 
           LDHU    .D1T1   *+A0[A3],A0       ; |1995| 
           NOP             4
           STH     .D2T1   A0,*+SP(38)       ; |1995| 
	.line	1275
;------------------------------------------------------------------------------
; 1996 | u16Dummy = u16Dummy >> 8;                                //    byte of 
;     | NREPS field in motion update rec                                       
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1996| 
           NOP             4
           SHRU    .S2     B4,8,B4           ; |1996| 
           STH     .D2T2   B4,*+SP(38)       ; |1996| 
	.line	1276
;------------------------------------------------------------------------------
; 1997 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(38),B4       ; |1997| 
           NOP             4
           EXTU    .S2     B4,24,24,B4       ; |1997| 
           STH     .D2T2   B4,*+SP(38)       ; |1997| 
	.line	1277
;------------------------------------------------------------------------------
; 1998 | nDotLifeDecr = (INT16) u16Dummy;                                       
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(38),B4       ; |1998| 
           NOP             4
           STH     .D2T2   B4,*+SP(12)       ; |1998| 
	.line	1279
;------------------------------------------------------------------------------
; 2000 | nMaxDotLife = (UINT16) parameters->wOuterR[d];           //    max dot 
;     | life, restricted to [1..32767]                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |2000| 
           LDW     .D2T2   *+SP(104),B4      ; |2000| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |2000| 
           ADDK    .S2     404,B4            ; |2000| 
           LDHU    .D2T2   *B4,B4            ; |2000| 
           NOP             4
           STH     .D2T2   B4,*+SP(44)       ; |2000| 
	.line	1280
;------------------------------------------------------------------------------
; 2001 | if( nMaxDotLife < 1 ) nMaxDotLife = 1;                                 
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B4       ; |2001| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |2001| 
   [!B0]   MVK     .S2     1,B4              ; |2001| 
   [!B0]   STH     .D2T2   B4,*+SP(44)       ; |2001| 
   [!B0]   B       .S1     L112              ; |2001| 
           NOP             5
           ; BRANCH OCCURS                   ; |2001| 
;** --------------------------------------------------------------------------*
	.line	1281
;------------------------------------------------------------------------------
; 2002 | else if( nMaxDotLife > 32767 ) nMaxDotLife = 32767;                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |2002| 
           MVK     .S2     32767,B4          ; |2002| 
           NOP             3
           CMPGT   .L2     B5,B4,B0          ; |2002| 
   [ B0]   MVK     .S1     32767,A0          ; |2002| 
   [ B0]   STH     .D2T1   A0,*+SP(44)       ; |2002| 
;** --------------------------------------------------------------------------*
L112:    
	.line	1283
;------------------------------------------------------------------------------
; 2004 | if( nMaxDotLife == 32767 ) nDotLifeDecr = 0;             //    unlimite
;     | d dot life if max life=32767!                                          
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(44),B5       ; |2004| 
           MVK     .S2     32767,B4          ; |2004| 
           NOP             3
           CMPEQ   .L2     B5,B4,B0          ; |2004| 
   [ B0]   ZERO    .L2     B4                ; |2004| 
   [ B0]   STH     .D2T2   B4,*+SP(12)       ; |2004| 
	.line	1285
;------------------------------------------------------------------------------
; 2006 | m = (UINT32) parameters->wOuterL[d];                     //    N = max 
;     | speed noise exp, in [1..7]                                             
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(168),B5      ; |2006| 
           LDW     .D2T2   *+SP(104),B4      ; |2006| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |2006| 
           ADDK    .S2     468,B5            ; |2006| 
           LDHU    .D2T2   *B5,B5            ; |2006| 
           NOP             4
           STW     .D2T2   B5,*+SP(100)      ; |2006| 
	.line	1286
;------------------------------------------------------------------------------
; 2007 | u16tmp = (UINT16) (m * 20);                              //    20N     
;------------------------------------------------------------------------------
           SHL     .S2     B5,4,B6           ; |2007| 
           ADDAW   .D2     B6,B5,B5          ; |2007| 
           STH     .D2T2   B5,*+SP(40)       ; |2007| 
	.line	1287
;------------------------------------------------------------------------------
; 2008 | u16Dummy = (UINT16) u16tmp * 2 + 1;                      //    # choice
;     | s in [-20N:20N]                                                        
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(40),B5       ; |2008| 
           NOP             4
           SHL     .S2     B5,1,B5           ; |2008| 
           ADD     .L2     1,B5,B5           ; |2008| 
           STH     .D2T2   B5,*+SP(38)       ; |2008| 
	.line	1289
;------------------------------------------------------------------------------
; 2010 | if( nNoiseUpdTicks[d] <= 0 )                             //    if noise
;     |  update intv expired, get new                                          
; 2011 | {                                                        //    new rand
;     | om index into the pow2LUT array                                        
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(188),A3      ; |2010| 
           MV      .L1X    B4,A0
           NOP             3
           LDH     .D1T1   *+A3[A0],A0       ; |2010| 
           NOP             4
           CMPGT   .L1     A0,0,A1           ; |2010| 
   [ A1]   B       .S1     L115              ; |2010| 
           NOP             5
           ; BRANCH OCCURS                   ; |2010| 
;** --------------------------------------------------------------------------*
	.line	1291
;------------------------------------------------------------------------------
; 2012 | j = nVisDotsPerTgt[d];                                //    for each do
;     | t.                                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(184),B5      ; |2012| 
           NOP             4
           LDHU    .D2T2   *+B5[B4],B5       ; |2012| 
           NOP             4
           STW     .D2T2   B5,*+SP(88)       ; |2012| 
	.line	1292
;------------------------------------------------------------------------------
; 2013 | for( k=0; k<j; k++ )                                                   
;------------------------------------------------------------------------------
           ZERO    .L2     B4                ; |2013| 
           STW     .D2T2   B4,*+SP(92)       ; |2013| 
           CMPLTU  .L2     B4,B5,B0          ; 
   [!B0]   B       .S1     L114              ; |2013| 
           MVK     .S2     140,B12           ; |2016| 
           NOP             4
           ; BRANCH OCCURS                   ; |2013| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L113:    
	.line	1294
;------------------------------------------------------------------------------
; 2015 | i32val = (INT32) (GetRandNum2() % u16Dummy);       //    [0..40N]      
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum2      ; |2015| 
           MVKL    .S2     RL548,B3          ; |2015| 
           MVKH    .S2     RL548,B3          ; |2015| 
           NOP             3
RL548:     ; CALL OCCURS                     ; |2015| 
           B       .S1     __remi            ; |2015| 
           LDHU    .D2T2   *+SP(38),B4       ; 
           MVKL    .S2     RL550,B3          ; |2015| 
           MVKH    .S2     RL550,B3          ; |2015| 
           NOP             2
RL550:     ; CALL OCCURS                     ; |2015| 
           MV      .L2X    A4,B11            ; 
	.line	1295
;------------------------------------------------------------------------------
; 2016 | i32val += (INT32) (140 - u16tmp);                  //    140 + [-20N..2
;     | 0N]                                                                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(40),B4       ; |2016| 
           NOP             4
           SUB     .L2     B12,B4,B4         ; |2016| 
           ADD     .L2     B4,B11,B11        ; |2016| 
	.line	1296
;------------------------------------------------------------------------------
; 2017 | *(nextDotNoise+k) = (INT16) i32val;                                    
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(92),B5       ; |2017| 
           LDW     .D2T2   *+SP(148),B4      ; |2017| 
           NOP             4
           STH     .D2T2   B11,*+B4[B5]      ; |2017| 
	.line	1297
           LDW     .D2T2   *+SP(92),B4       ; |2018| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |2018| 
           STW     .D2T2   B4,*+SP(92)       ; |2018| 
           LDW     .D2T2   *+SP(88),B5       ; |2018| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L113              ; |2018| 
           NOP             5
           ; BRANCH OCCURS                   ; |2018| 
;** --------------------------------------------------------------------------*
L114:    
	.line	1299
;------------------------------------------------------------------------------
; 2020 | nNoiseUpdTicks[d] = (INT16) parameters->wOuterT[d];   //       reload n
;     | oise update intv timer                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |2020| 
           LDW     .D2T2   *+SP(168),B5      ; |2020| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |2020| 
           ADDK    .S2     532,B5            ; |2020| 
           LDH     .D2T2   *B5,B6            ; |2020| 
           LDW     .D2T2   *+SP(188),B5      ; |2020| 
           NOP             4
           STH     .D2T2   B6,*+B5[B4]       ; |2020| 
;** --------------------------------------------------------------------------*
L115:    
	.line	1302
;------------------------------------------------------------------------------
; 2023 | nNoiseUpdTicks[d] -= 2 * (data[cd+NREPS] & 0x000000FF);  //    decremen
;     | t noise update intv timer                                              
; 2025 | // STRATEGY: Analogous to the additive speed noise case above, except t
;     | hat we implement the                                                   
; 2026 | // multiplicative speed noise algorithm here.                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |2023| 
           LDW     .D2T2   *+SP(104),B6      ; |2023| 
           LDW     .D2T2   *+SP(188),B7      ; |2023| 
           LDW     .D2T2   *+SP(200),B5      ; |2023| 
           NOP             3

           ADDAH   .D2     B7,B6,B4          ; |2023| 
||         ADD     .L2     4,B4,B6           ; |2023| 

           LDH     .D2T2   *+B5[B6],B5       ; |2023| 
           LDH     .D2T2   *B4,B6            ; |2023| 
           NOP             3
           EXTU    .S2     B5,24,24,B5       ; |2023| 
           SUBAH   .D2     B6,B5,B5          ; |2023| 
           STH     .D2T2   B5,*B4            ; |2023| 
	.line	1306
;------------------------------------------------------------------------------
; 2027 | i16Scale = 6;                                                          
;------------------------------------------------------------------------------
           MVK     .S1     6,A0              ; |2027| 
           STH     .D2T1   A0,*+SP(26)       ; |2027| 
	.line	1307
;------------------------------------------------------------------------------
; 2028 | if( vv >= 10000 )                                                      
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B5        ; |2028| 
           MVK     .S2     10000,B4          ; |2028| 
           NOP             3
           CMPLT   .L2     B5,B4,B0          ; |2028| 
	.line	1309
;------------------------------------------------------------------------------
; 2030 | vv -= 10000;                                                           
;------------------------------------------------------------------------------
   [!B0]   LDH     .D2T2   *+SP(6),B4        ; |2030| 
           NOP             4
   [!B0]   ADDK    .S2     -10000,B4         ; |2030| 
   [!B0]   STH     .D2T2   B4,*+SP(6)        ; |2030| 
	.line	1310
;------------------------------------------------------------------------------
; 2031 | i16Scale = 0;                                                          
;------------------------------------------------------------------------------
   [!B0]   ZERO    .L2     B4                ; |2031| 
   [!B0]   STH     .D2T2   B4,*+SP(26)       ; |2031| 
	.line	1313
;------------------------------------------------------------------------------
; 2034 | while( a < de )                                          //    for each
;     |  dot in target:                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |2034| 
   [!A1]   B       .S1     L133              ; |2034| 
           NOP             5
           ; BRANCH OCCURS                   ; |2034| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L116
;** --------------------------------------------------------------------------*
L116:    
	.line	1315
;------------------------------------------------------------------------------
; 2036 | *nextDotLife = *nextDotLife - nDotLifeDecr;           //       update d
;     | ot's current lifetime; if it                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2036| 
           LDH     .D2T2   *+SP(12),B6       ; |2036| 
           NOP             3
           LDH     .D2T2   *B4,B5            ; |2036| 
           NOP             4
           SUB     .L2     B5,B6,B5          ; |2036| 
           STH     .D2T2   B5,*B4            ; |2036| 
	.line	1316
;------------------------------------------------------------------------------
; 2037 | if( *nextDotLife < 0 )                                //       has expi
;     | red, reset it and randomly                                             
; 2038 | {                                                     //       repositi
;     | on dot in tgt window BEFORE                                            
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2037| 
           NOP             4
           LDH     .D2T2   *B4,B5            ; |2037| 
           NOP             4
           CMPLT   .L2     B5,0,B0           ; |2037| 
   [!B0]   B       .S1     L117              ; |2037| 
           NOP             5
           ; BRANCH OCCURS                   ; |2037| 
;** --------------------------------------------------------------------------*
	.line	1318
;------------------------------------------------------------------------------
; 2039 | *nextDotLife = nMaxDotLife;                        //       MOVING IT! 
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(44),B5       ; |2039| 
           NOP             4
           STH     .D2T2   B5,*B4            ; |2039| 
	.line	1319
;------------------------------------------------------------------------------
; 2040 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2040| 
           MVKL    .S2     RL552,B3          ; |2040| 
           MVKH    .S2     RL552,B3          ; |2040| 
           NOP             3
RL552:     ; CALL OCCURS                     ; |2040| 
           MV      .L1     A4,A10            ; 
	.line	1320
;------------------------------------------------------------------------------
; 2041 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2041| 
           MVKL    .S2     RL554,B3          ; |2041| 
           MVKH    .S2     RL554,B3          ; |2041| 
           NOP             3
RL554:     ; CALL OCCURS                     ; |2041| 
           MV      .L1     A4,A11            ; 
	.line	1321
;------------------------------------------------------------------------------
; 2042 | *a = rectL + (xCoord % rectW);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |2042| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL556,B3          ; |2042| 
           MV      .L1     A10,A4            ; 
           MVKH    .S2     RL556,B3          ; |2042| 
           NOP             1
RL556:     ; CALL OCCURS                     ; |2042| 
           ADD     .L1X    A4,B8,A0          ; 
           STH     .D1T1   A0,*A9            ; |2042| 
	.line	1322
;------------------------------------------------------------------------------
; 2043 | *b = rectD + (yCoord % rectH);                                         
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |2043| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL558,B3          ; |2043| 
           MV      .L1     A11,A4            ; 
           MVKH    .S2     RL558,B3          ; |2043| 
           NOP             1
RL558:     ; CALL OCCURS                     ; |2043| 
           ADD     .L1X    A4,B9,A0          ; 
           STH     .D1T1   A0,*A7            ; |2043| 
;** --------------------------------------------------------------------------*
L117:    
	.line	1325
;------------------------------------------------------------------------------
; 2046 | i32val = pow2LUT[ *nextDotNoise ];                    //       R = 2^(x
;     | +20), x in [-N..N], N=[1..7]                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |2046| 
           LDW     .D2T2   *+SP(220),B5      ; |2046| 
           NOP             3
           LDH     .D2T2   *B4,B4            ; |2046| 
           NOP             4
           LDW     .D2T2   *+B5[B4],B11      ; |2046| 
           NOP             4
	.line	1326
;------------------------------------------------------------------------------
; 2047 | i32val /= speedNoiseAdj[m-1];                         //       R = 2^20
;     |  * 2^x / (E(2^x) * 2^10)                                               
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(100),B4      ; |2047| 
           LDW     .D2T2   *+SP(224),B5      ; |2047| 
           MVKL    .S2     RL560,B3          ; |2047| 
           MV      .L1X    B11,A4            ; |2047| 
           MVKH    .S2     RL560,B3          ; |2047| 

           B       .S1     __divi            ; |2047| 
||         SUB     .L2     B4,1,B4           ; |2047| 

           LDW     .D2T2   *+B5[B4],B4       ; 
           NOP             4
RL560:     ; CALL OCCURS                     ; |2047| 
           MV      .L2X    A4,B11            ; 
	.line	1327
;------------------------------------------------------------------------------
; 2048 | i32val *= hv;                                         //       R = Rpat
;     | _mm*2^Q * 2^10 * 2^x / E(2^x)                                          
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B5        ; |2048| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |2048| 
           MPYSU   .M2     B5,B11,B5         ; |2048| 
           SHL     .S2     B4,16,B4          ; |2048| 
           ADD     .L2     B5,B4,B11         ; |2048| 
	.line	1328
;------------------------------------------------------------------------------
; 2049 | i32val >>= 10;                                        //       R = Rdot
;     | _mm*2^Q, Q=10 or 16                                                    
;------------------------------------------------------------------------------
           SHR     .S2     B11,10,B11        ; |2049| 
	.line	1329
;------------------------------------------------------------------------------
; 2050 | x32 = i32val;                                         //       save cuz
;     |  we're going to change i32val                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(16)      ; |2050| 
	.line	1331
;------------------------------------------------------------------------------
; 2052 | i32val *= (INT32) cosLUT[vv];                         //       Rdot_mm*
;     | cos(theta) * 2^(Q+10)                                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(216),B4      ; |2052| 
           LDH     .D2T2   *+SP(6),B5        ; |2052| 
           NOP             4
           LDH     .D2T2   *+B4[B5],B5       ; |2052| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |2052| 
           MPYSU   .M2     B5,B11,B5         ; |2052| 
           SHL     .S2     B4,16,B4          ; |2052| 
           ADD     .L2     B5,B4,B11         ; |2052| 
	.line	1332
;------------------------------------------------------------------------------
; 2053 | i32val /= screenW_mm;                                 //       Xmm*2^(4
;     | +K)*(2^16/screenW_mm)                                                  
; 2054 | //       = Xpix*2^(4+K), K=0 or 6                                      
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |2053| 
           LDHU    .D2T2   *+SP(32),B4       ; 
           MVKL    .S2     RL562,B3          ; |2053| 
           MV      .L1X    B11,A4            ; |2053| 
           MVKH    .S2     RL562,B3          ; |2053| 
           NOP             1
RL562:     ; CALL OCCURS                     ; |2053| 
           MV      .L2X    A4,B11            ; 
	.line	1334
;------------------------------------------------------------------------------
; 2055 | i32val >>= i16Scale;                                  //       Xpix*2^4
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |2055| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |2055| 
	.line	1335
;------------------------------------------------------------------------------
; 2056 | i32val += *nextFracDX;                                //       add in f
;     | racDX*2^4 from last frame                                              
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(156),A0      ; |2056| 
           NOP             4
           LDH     .D1T1   *A0,A3            ; |2056| 
           NOP             4
           ADD     .L2X    A3,B11,B11        ; |2056| 
	.line	1336
;------------------------------------------------------------------------------
; 2057 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |2057| 
	.line	1337
;------------------------------------------------------------------------------
; 2058 | i32val >>= 4;                                         //       xPix for
;     |  this frame                                                            
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2058| 
	.line	1338
;------------------------------------------------------------------------------
; 2059 | xCoord = *a + hw + ((INT16) i32val);                  //       x = xOld
;     |  + hWin + Xpix                                                         
; 2060 | // 11may2011: As of Maestro v2.7.0, pattern displacement is WRT target 
;     | window, so dot displacement is                                         
; 2061 | // window displacement + per-dot displacement!                         
;------------------------------------------------------------------------------

           LDH     .D2T2   *+SP(8),B4        ; |2059| 
||         LDHU    .D1T1   *A9,A3            ; |2059| 

           NOP             4

           ADD     .L1X    B4,A3,A3          ; |2059| 
||         EXT     .S2     B11,16,16,B4      ; |2059| 

           ADD     .L1X    B4,A3,A3          ; |2059| 
           EXTU    .S1     A3,16,16,A10      ; |2059| 
	.line	1342
;------------------------------------------------------------------------------
; 2063 | i32val <<= 4;                                         //       save fra
;     | cDX*2^4 for next frame                                                 
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |2063| 
	.line	1343
;------------------------------------------------------------------------------
; 2064 | *nextFracDX = (INT16) (y32 - i32val);                                  
; 2066 | // analogously for y-coordinate...                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |2064| 
           NOP             4
           SUB     .L2     B4,B11,B4         ; |2064| 
           STH     .D1T2   B4,*A0            ; |2064| 
	.line	1346
;------------------------------------------------------------------------------
; 2067 | i32val = x32;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(16),B11      ; |2067| 
           NOP             4
	.line	1347
;------------------------------------------------------------------------------
; 2068 | i32val *= (INT32) sinLUT[vv];                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(212),B4      ; |2068| 
           LDH     .D2T2   *+SP(6),B5        ; |2068| 
           NOP             4
           LDH     .D2T2   *+B4[B5],B5       ; |2068| 
           NOP             4
           MPYLH   .M2     B5,B11,B4         ; |2068| 
           MPYSU   .M2     B5,B11,B5         ; |2068| 
           SHL     .S2     B4,16,B4          ; |2068| 
           ADD     .L2     B5,B4,B11         ; |2068| 
	.line	1348
;------------------------------------------------------------------------------
; 2069 | i32val /= screenH_mm;                                                  
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |2069| 
           LDHU    .D2T2   *+SP(34),B4       ; 
           MVKL    .S2     RL564,B3          ; |2069| 
           MV      .L1X    B11,A4            ; |2069| 
           MVKH    .S2     RL564,B3          ; |2069| 
           NOP             1
RL564:     ; CALL OCCURS                     ; |2069| 
           MV      .L2X    A4,B11            ; 
	.line	1349
;------------------------------------------------------------------------------
; 2070 | i32val >>= i16Scale;                                                   
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(26),B4       ; |2070| 
           NOP             4
           SHR     .S2     B11,B4,B11        ; |2070| 
	.line	1350
;------------------------------------------------------------------------------
; 2071 | i32val += *nextFracDY;                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B5      ; |2071| 
           NOP             4
           LDH     .D2T2   *B5,B4            ; |2071| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |2071| 
	.line	1351
;------------------------------------------------------------------------------
; 2072 | y32 = i32val;                                                          
;------------------------------------------------------------------------------
           STW     .D2T2   B11,*+SP(20)      ; |2072| 
	.line	1352
;------------------------------------------------------------------------------
; 2073 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2073| 
	.line	1353
;------------------------------------------------------------------------------
; 2074 | yCoord = *b + vw + ((INT16) i32val);                                   
;------------------------------------------------------------------------------

           LDHU    .D1T1   *A7,A0            ; |2074| 
||         LDH     .D2T2   *+SP(10),B4       ; |2074| 

           NOP             4

           ADD     .L1X    B4,A0,A0          ; |2074| 
||         EXT     .S2     B11,16,16,B4      ; |2074| 

           ADD     .L1X    B4,A0,A0          ; |2074| 
           EXTU    .S1     A0,16,16,A11      ; |2074| 
	.line	1354
;------------------------------------------------------------------------------
; 2075 | i32val <<= 4;                                                          
;------------------------------------------------------------------------------
           SHL     .S2     B11,4,B11         ; |2075| 
	.line	1355
;------------------------------------------------------------------------------
; 2076 | *nextFracDY = (INT16) (y32 - i32val);                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(20),B4       ; |2076| 
           NOP             4
           SUB     .L2     B4,B11,B4         ; |2076| 
           STH     .D2T2   B4,*B5            ; |2076| 
	.line	1357
;------------------------------------------------------------------------------
; 2078 | if( (xCoord > rectR) || (xCoord < rectL) )            //       if dot i
;     | s now outside tgt window,                                              
; 2079 | {                                                     //       wrap it 
;     | around as in the OPTRECTWIN                                            
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A2         ; |2078| 

   [!A2]   CMPLT   .L1X    A10,B8,A1         ; |2078| 
|| [ A2]   MV      .S1     A2,A1             ; |2078| 

   [!A1]   B       .S1     L118              ; |2078| 
           NOP             5
           ; BRANCH OCCURS                   ; |2078| 
;** --------------------------------------------------------------------------*
	.line	1359
;------------------------------------------------------------------------------
; 2080 | if( xCoord > rectR ) u16Over = xCoord - rectR;     //       target...  
;------------------------------------------------------------------------------
           CMPGT   .L1     A10,A8,A1         ; |2080| 
   [ A1]   SUB     .L1     A10,A8,A0         ; |2080| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |2080| 
	.line	1360
;------------------------------------------------------------------------------
; 2081 | else                 u16Over = rectL - xCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B8,A10,B4         ; |2081| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |2081| 
	.line	1361
;------------------------------------------------------------------------------
; 2082 | u16Over = u16Over % rectW;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |2082| 
||         LDHU    .D2T2   *+SP(28),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL566,B3          ; |2082| 
           MVKH    .S2     RL566,B3          ; |2082| 
           NOP             2
RL566:     ; CALL OCCURS                     ; |2082| 
           STH     .D2T1   A4,*+SP(42)       ; |2082| 
	.line	1363
;------------------------------------------------------------------------------
; 2084 | if( (xCoord - *a) > hw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |2084| 
           LDH     .D2T2   *+SP(8),B4        ; |2084| 
           NOP             3
           SUB     .L1     A10,A0,A0         ; |2084| 
           CMPGT   .L2X    A0,B4,B0          ; |2084| 
	.line	1364
;------------------------------------------------------------------------------
; 2085 | xCoord = rectL + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |2085| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B8,A0          ; |2085| 
   [ B0]   EXTU    .S1     A0,16,16,A10      ; |2085| 
	.line	1365
;------------------------------------------------------------------------------
; 2086 | else xCoord = rectR - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |2086| 
           NOP             4
   [!B0]   SUB     .L1X    A8,B4,A0          ; |2086| 
   [!B0]   EXTU    .S1     A0,16,16,A10      ; |2086| 
	.line	1367
;------------------------------------------------------------------------------
; 2088 | yCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2088| 
           MVKL    .S2     RL568,B3          ; |2088| 
           MVKH    .S2     RL568,B3          ; |2088| 
           NOP             3
RL568:     ; CALL OCCURS                     ; |2088| 
	.line	1368
;------------------------------------------------------------------------------
; 2089 | yCoord = rectD + (yCoord % rectH);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |2089| 
           LDHU    .D2T2   *+SP(30),B4       ; 
           MVKL    .S2     RL570,B3          ; |2089| 
           MVKH    .S2     RL570,B3          ; |2089| 
           NOP             2
RL570:     ; CALL OCCURS                     ; |2089| 
           ADD     .L1X    A4,B9,A0          ; 
           EXTU    .S1     A0,16,16,A11      ; |2089| 
	.line	1369
           B       .S1     L119              ; |2090| 
           NOP             5
           ; BRANCH OCCURS                   ; |2090| 
;** --------------------------------------------------------------------------*
L118:    
	.line	1370
;------------------------------------------------------------------------------
; 2091 | else if( (yCoord > rectU) || (yCoord < rectD) )                        
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |2091| 
   [!A1]   CMPLT   .L1X    A11,B9,A1         ; |2091| 
   [!A1]   B       .S1     L119              ; |2091| 
           NOP             5
           ; BRANCH OCCURS                   ; |2091| 
;** --------------------------------------------------------------------------*
	.line	1372
;------------------------------------------------------------------------------
; 2093 | if( yCoord > rectU ) u16Over = yCoord - rectU;                         
;------------------------------------------------------------------------------
           CMPGT   .L1     A11,A13,A1        ; |2093| 
   [ A1]   SUB     .L1     A11,A13,A0        ; |2093| 
   [ A1]   STH     .D2T1   A0,*+SP(42)       ; |2093| 
	.line	1373
;------------------------------------------------------------------------------
; 2094 | else                 u16Over = rectD - yCoord;                         
;------------------------------------------------------------------------------
   [!A1]   SUB     .L2X    B9,A11,B4         ; |2094| 
   [!A1]   STH     .D2T2   B4,*+SP(42)       ; |2094| 
	.line	1374
;------------------------------------------------------------------------------
; 2095 | u16Over = u16Over % rectH;                                             
;------------------------------------------------------------------------------

           B       .S1     __remi            ; |2095| 
||         LDHU    .D2T2   *+SP(30),B4       ; 

           LDHU    .D2T1   *+SP(42),A4       ; 
           MVKL    .S2     RL572,B3          ; |2095| 
           MVKH    .S2     RL572,B3          ; |2095| 
           NOP             2
RL572:     ; CALL OCCURS                     ; |2095| 
           STH     .D2T1   A4,*+SP(42)       ; |2095| 
	.line	1376
;------------------------------------------------------------------------------
; 2097 | if( (yCoord - *b) > vw )                           //       (each dot i
;     | s displaced differently                                                
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A7,A0            ; |2097| 
           LDH     .D2T2   *+SP(10),B4       ; |2097| 
           NOP             3
           SUB     .L1     A11,A0,A0         ; |2097| 
           CMPGT   .L2X    A0,B4,B0          ; |2097| 
	.line	1377
;------------------------------------------------------------------------------
; 2098 | yCoord = rectD + u16Over;                       //       every frame in
;     |  this target!)                                                         
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T1   *+SP(42),A0       ; |2098| 
           NOP             4
   [ B0]   ADD     .L1X    A0,B9,A0          ; |2098| 
   [ B0]   EXTU    .S1     A0,16,16,A11      ; |2098| 
	.line	1378
;------------------------------------------------------------------------------
; 2099 | else yCoord = rectU - u16Over;                                         
;------------------------------------------------------------------------------
   [!B0]   LDHU    .D2T2   *+SP(42),B4       ; |2099| 
           NOP             4
   [!B0]   SUB     .L1X    A13,B4,A0         ; |2099| 
   [!B0]   EXTU    .S1     A0,16,16,A11      ; |2099| 
	.line	1380
;------------------------------------------------------------------------------
; 2101 | xCoord = (UINT16) GetRandNum();                                        
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2101| 
           MVKL    .S2     RL574,B3          ; |2101| 
           MVKH    .S2     RL574,B3          ; |2101| 
           NOP             3
RL574:     ; CALL OCCURS                     ; |2101| 
	.line	1381
;------------------------------------------------------------------------------
; 2102 | xCoord = rectL + (xCoord % rectW);                                     
;------------------------------------------------------------------------------
           B       .S1     __remi            ; |2102| 
           LDHU    .D2T2   *+SP(28),B4       ; 
           MVKL    .S2     RL576,B3          ; |2102| 
           MVKH    .S2     RL576,B3          ; |2102| 
           NOP             2
RL576:     ; CALL OCCURS                     ; |2102| 
           ADD     .L1X    A4,B8,A0          ; 
           EXTU    .S1     A0,16,16,A10      ; |2102| 
;** --------------------------------------------------------------------------*
L119:    
	.line	1384
;------------------------------------------------------------------------------
; 2105 | *a = xCoord;                                          //       remember
;     |  the new dot location!                                                 
;------------------------------------------------------------------------------
           STH     .D1T1   A10,*A9           ; |2105| 
	.line	1385
;------------------------------------------------------------------------------
; 2106 | *b = yCoord;                                                           
;------------------------------------------------------------------------------
           STH     .D1T1   A11,*A7           ; |2106| 
	.line	1387
;------------------------------------------------------------------------------
; 2108 | xyvals = ((*a << 16) | *b);                           //       draw the
;     |  dot                                                                   
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |2108| 
           LDHU    .D1T1   *A7,A3            ; |2108| 
           NOP             3
           SHL     .S1     A0,16,A0          ; |2108| 
           OR      .L1     A3,A0,A0          ; |2108| 
           STW     .D2T1   A0,*+SP(56)       ; |2108| 
	.line	1388
;------------------------------------------------------------------------------
; 2109 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |2109| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2109| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2109| 
   [!B0]   B       .S1     L121              ; |2109| 
           NOP             5
           ; BRANCH OCCURS                   ; |2109| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L120:    
           LDW     .D2T2   *+SP(76),B4       ; |2109| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2109| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2109| 
   [ B0]   B       .S1     L120              ; |2109| 
           NOP             5
           ; BRANCH OCCURS                   ; |2109| 
;** --------------------------------------------------------------------------*
L121:    
	.line	1389
;------------------------------------------------------------------------------
; 2110 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |2110| 
           LDW     .D2T1   *+SP(72),A0       ; |2110| 
           NOP             4
           STW     .D1T2   B4,*A0            ; |2110| 
	.line	1390
;------------------------------------------------------------------------------
; 2111 | visibleDotsXY[nTotalVisDots++] = xyvals;              //       save pac
;     | ked coords in visi dots array                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B6      ; |2111| 
           LDW     .D2T2   *+SP(56),B5       ; |2111| 
           MV      .L2X    A12,B4            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B5,*+B6[B4]       ; |2111| 
	.line	1391
;------------------------------------------------------------------------------
; 2112 | a++;                                                  //       move on 
;     | to next dot                                                            
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |2112| 
	.line	1392
;------------------------------------------------------------------------------
; 2113 | b++;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |2113| 
	.line	1393
;------------------------------------------------------------------------------
; 2114 | nextDotLife++;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2114| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2114| 
           STW     .D2T2   B4,*+SP(140)      ; |2114| 
	.line	1394
;------------------------------------------------------------------------------
; 2115 | nextDotNoise++;                                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(148),B4      ; |2115| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2115| 
           STW     .D2T2   B4,*+SP(148)      ; |2115| 
	.line	1395
;------------------------------------------------------------------------------
; 2116 | nextFracDX++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(156),B4      ; |2116| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2116| 
           STW     .D2T2   B4,*+SP(156)      ; |2116| 
	.line	1396
;------------------------------------------------------------------------------
; 2117 | nextFracDY++;                                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(164),B4      ; |2117| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2117| 
           STW     .D2T2   B4,*+SP(164)      ; |2117| 
	.line	1397
           CMPLTU  .L1     A9,A14,A1         ; |2118| 
   [ A1]   B       .S1     L116              ; |2118| 
           NOP             5
           ; BRANCH OCCURS                   ; |2118| 
;** --------------------------------------------------------------------------*
	.line	1398
           B       .S1     L133              ; |2119| 
           NOP             5
           ; BRANCH OCCURS                   ; |2119| 
;** --------------------------------------------------------------------------*
L122:    
	.line	1399
;------------------------------------------------------------------------------
; 2120 | else if( u16Type == OPTICFLOW )                             // OPTICFLO
;     | W:  Dot speed varies w/ radial pos,                                    
; 2121 | {                                                           // calc'd e
;     | very frame.  See file header for                                       
; 2122 | // explanation of implementation...                                    
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |2120| 
           NOP             4
           CMPEQ   .L2     B4,9,B0           ; |2120| 
   [!B0]   B       .S1     L133              ; |2120| 
           NOP             5
           ; BRANCH OCCURS                   ; |2120| 
;** --------------------------------------------------------------------------*
	.line	1402
;------------------------------------------------------------------------------
; 2123 | nVisDotsPerTgt[d] = parameters->wNumDots[d];             //    every do
;     | t in target is visible                                                 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |2123| 
           LDW     .D2T2   *+SP(168),B5      ; |2123| 
           LDW     .D2T2   *+SP(184),B6      ; |2123| 
           NOP             3
           ADDAH   .D2     B5,B4,B5          ; |2123| 
           ADDK    .S2     84,B5             ; |2123| 
           LDHU    .D2T2   *B5,B5            ; |2123| 
           NOP             4
           STH     .D2T2   B5,*+B6[B4]       ; |2123| 
	.line	1404
;------------------------------------------------------------------------------
; 2125 | if( hv < 0 )                                             //    FOR DECE
;     | LERATING FLOWS:                                                        
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |2125| 
           NOP             4
           CMPLT   .L2     B4,0,B0           ; |2125| 
   [!B0]   B       .S1     L128              ; |2125| 
           NOP             5
           ; BRANCH OCCURS                   ; |2125| 
;** --------------------------------------------------------------------------*
	.line	1406
;------------------------------------------------------------------------------
; 2127 | rectH = (UINT16) (-hv);                               //    this factor
;     |  in the recycle rate incr with                                         
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(4),B4        ; |2127| 
           NOP             4
           NEG     .L2     B4,B4             ; |2127| 
           STH     .D2T2   B4,*+SP(30)       ; |2127| 
	.line	1407
;------------------------------------------------------------------------------
; 2128 | rectH >>= vv;                                         //    B ~ deltaT 
;     | * flow vel / 3.  the scaling                                           
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(30),B5       ; |2128| 
           LDH     .D2T2   *+SP(6),B4        ; |2128| 
           NOP             4
           SHRU    .S2     B5,B4,B4          ; |2128| 
           STH     .D2T2   B4,*+SP(30)       ; |2128| 
	.line	1408
;------------------------------------------------------------------------------
; 2129 | rectH /= 3;                                           //    by 1/3 was 
;     | determined heuristically.                                              
;------------------------------------------------------------------------------
           B       .S1     __divi            ; |2129| 
           LDHU    .D2T1   *+SP(30),A4       ; 
           MVKL    .S2     RL578,B3          ; |2129| 
           MVKH    .S2     RL578,B3          ; |2129| 
           MVK     .S2     0x3,B4            ; |2129| 
           NOP             1
RL578:     ; CALL OCCURS                     ; |2129| 
           STH     .D2T1   A4,*+SP(30)       ; |2129| 
	.line	1409
;------------------------------------------------------------------------------
; 2130 | if( rectH < 1 ) rectH = 1;                            //    rate limite
;     | d to [1..400] parts per 1000.                                          
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(30),B4       ; |2130| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |2130| 
   [!B0]   MVK     .S1     1,A0              ; |2130| 
   [!B0]   STH     .D2T1   A0,*+SP(30)       ; |2130| 
	.line	1410
;------------------------------------------------------------------------------
; 2131 | if( rectH > 400 ) rectH = 400;                                         
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(30),B4       ; |2131| 
           MVK     .S1     400,A0            ; |2131| 
           NOP             3
           CMPGT   .L1X    B4,A0,A1          ; |2131| 
   [ A1]   MVK     .S2     400,B4            ; |2131| 
   [ A1]   STH     .D2T2   B4,*+SP(30)       ; |2131| 
	.line	1412
;------------------------------------------------------------------------------
; 2133 | i32val = hv * sincosLUT[ rectL ];                     //    change in r
;     | adial pos at outer edge; repos                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(208),B4      ; |2133| 
           NOP             4
           LDH     .D2T2   *+B4[B8],B5       ; |2133| 
           LDH     .D2T2   *+SP(4),B4        ; |2133| 
           NOP             4
           MPY     .M2     B5,B4,B11         ; |2133| 
           NOP             1
	.line	1413
;------------------------------------------------------------------------------
; 2134 | i32val >>= (10 + vv);                                 //    dots in the
;     |  band between outer radius and                                         
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |2134| 
           NOP             4
           ADD     .L2     10,B4,B4          ; |2134| 
           SHR     .S2     B11,B4,B11        ; |2134| 
	.line	1414
;------------------------------------------------------------------------------
; 2135 | i32val += (INT32) rectL;                              //    r = rOuter 
;     | - radial chng at outer edge...                                         
;------------------------------------------------------------------------------
           ADD     .L2     B8,B11,B11        ; |2135| 
	.line	1415
;------------------------------------------------------------------------------
; 2136 | u16tmp = (UINT16) i32val;                                              
;------------------------------------------------------------------------------
           STH     .D2T2   B11,*+SP(40)      ; |2136| 
	.line	1417
;------------------------------------------------------------------------------
; 2138 | while( a < de )                                                        
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |2138| 
   [!A1]   B       .S1     L133              ; |2138| 
           NOP             5
           ; BRANCH OCCURS                   ; |2138| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L123
;** --------------------------------------------------------------------------*
L123:    
	.line	1419
;------------------------------------------------------------------------------
; 2140 | i32val = hv * sincosLUT[ *a ];                     //    dr*2^(10+M)= [
;     | B*2^M]*[sin(r)*cos(r)*2^10]                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A3            ; |2140| 
           LDW     .D2T1   *+SP(208),A0      ; |2140| 
           LDH     .D2T2   *+SP(4),B4        ; |2140| 
           NOP             3
           LDH     .D1T1   *+A0[A3],A0       ; |2140| 
           NOP             4
           MPY     .M2X    A0,B4,B11         ; |2140| 
           NOP             1
	.line	1420
;------------------------------------------------------------------------------
; 2141 | i32val >>= (2 + vv);                               //    dr*2^(10+M) --
;     | > dr*2^8                                                               
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |2141| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2141| 
           SHR     .S2     B11,B4,B11        ; |2141| 
	.line	1421
;------------------------------------------------------------------------------
; 2142 | i32val += (INT32) *nextDotLife;                    //    accum fraction
;     | al pos chng (deg/100/2^8)                                              
; 2143 | //    from last update -- NOTE usage of the                            
; 2144 | //    "dotlife" array for this purpose!                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2142| 
           NOP             4
           LDH     .D2T2   *B4,B5            ; |2142| 
           NOP             4
           ADD     .L2     B5,B11,B11        ; |2142| 
	.line	1425
;------------------------------------------------------------------------------
; 2146 | *nextDotLife = 0xFF00 | (0x00FF & i32val);         //    carry over fra
;     | c pos chng for next update                                             
;------------------------------------------------------------------------------
           EXTU    .S2     B11,24,24,B5      ; |2146| 
           SET     .S2     B5,8,15,B5        ; |2146| 
           STH     .D2T2   B5,*B4            ; |2146| 
	.line	1426
;------------------------------------------------------------------------------
; 2147 | i32val >>= 8;                                      //    dr*2^8 --> dr 
;------------------------------------------------------------------------------
           SHR     .S2     B11,8,B11         ; |2147| 
	.line	1427
;------------------------------------------------------------------------------
; 2148 | ++i32val;                                          //    -1 maps to 0 f
;     | or neg flows                                                           
;------------------------------------------------------------------------------
           ADD     .L2     1,B11,B11         ; |2148| 
	.line	1428
;------------------------------------------------------------------------------
; 2149 | i32val += (INT32) *a;                              //    r' = r + dr   
;------------------------------------------------------------------------------
           LDHU    .D1T2   *A9,B4            ; |2149| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |2149| 
	.line	1429
;------------------------------------------------------------------------------
; 2150 | *a = (UINT16) i32val;                                                  
;------------------------------------------------------------------------------
           STH     .D1T2   B11,*A9           ; |2150| 
	.line	1431
;------------------------------------------------------------------------------
; 2152 | u16Dummy = ((UINT16) GetRandNum()) % 1000;         //    algorithm for 
;     | choosing dots to recycle:                                              
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2152| 
           MVKL    .S2     RL582,B3          ; |2152| 
           MVKH    .S2     RL582,B3          ; |2152| 
           NOP             3
RL582:     ; CALL OCCURS                     ; |2152| 
           B       .S1     __remi            ; |2152| 
           MVKL    .S2     RL584,B3          ; |2152| 
           MVKH    .S2     RL584,B3          ; |2152| 
           MVK     .S2     0x3e8,B4          ; |2152| 
           NOP             2
RL584:     ; CALL OCCURS                     ; |2152| 
           STH     .D2T1   A4,*+SP(38)       ; |2152| 
	.line	1432
;------------------------------------------------------------------------------
; 2153 | rectW = rectR +                                    //       1) dot has 
;     | entered hole at FOE, or                                                
; 2154 | (((UINT16) GetRandNum()) % (rectL-rectR));      //       2) is randomly
;     |  selected for recycle.                                                 
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2153| 
           MVKL    .S2     RL588,B3          ; |2153| 
           MVKH    .S2     RL588,B3          ; |2153| 
           NOP             3
RL588:     ; CALL OCCURS                     ; |2153| 
           B       .S1     __remi            ; |2153| 
           MVKL    .S2     RL590,B3          ; |2153| 
           MVKH    .S2     RL590,B3          ; |2153| 
           SUB     .L2X    B8,A8,B4          ; 
           NOP             2
RL590:     ; CALL OCCURS                     ; |2153| 
           ADD     .L1     A4,A8,A0          ; 
           STH     .D2T1   A0,*+SP(28)       ; |2153| 
	.line	1434
;------------------------------------------------------------------------------
; 2155 | if( (i32val < (INT32) rectR) ||                    //    if chosen for 
;     | recycle, randomly choose                                               
; 2156 | ((u16Dummy<rectH) && (i32val<(INT32)rectW))    //    polar coords (r,th
;     | eta) so that dot is                                                    
; 2157 | )                                               //    repos in band nea
;     | r outer edge of field                                                  
;------------------------------------------------------------------------------
           CMPLT   .L2X    B11,A8,B0         ; |2155| 
   [ B0]   B       .S1     L124              ; |2155| 

   [ B0]   MV      .L2     B0,B10            ; |2155| 
|| [!B0]   LDHU    .D2T2   *+SP(30),B4       ; |2155| 

   [!B0]   LDHU    .D2T2   *+SP(38),B5       ; |2155| 
|| [ B0]   MV      .L2     B10,B0            ; |2155| 

           NOP             3
           ; BRANCH OCCURS                   ; |2155| 
;** --------------------------------------------------------------------------*
           NOP             1
           CMPLT   .L2     B5,B4,B0          ; |2155| 
   [ B0]   LDHU    .D2T2   *+SP(28),B4       ; |2155| 
           NOP             4
   [ B0]   CMPLT   .L2     B11,B4,B0         ; |2155| 
           MV      .L2     B0,B10            ; 
           MV      .L2     B10,B0            ; |2155| 
;** --------------------------------------------------------------------------*
L124:    
   [!B0]   B       .S1     L125              ; |2155| 
           NOP             5
           ; BRANCH OCCURS                   ; |2155| 
;** --------------------------------------------------------------------------*
	.line	1438
;------------------------------------------------------------------------------
; 2159 | *a = u16tmp + (((UINT16) GetRandNum()) % (rectL-u16tmp));              
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2159| 
           MVKL    .S2     RL594,B3          ; |2159| 
           MVKH    .S2     RL594,B3          ; |2159| 
           NOP             3
RL594:     ; CALL OCCURS                     ; |2159| 

           B       .S1     __remi            ; |2159| 
||         LDHU    .D2T2   *+SP(40),B4       ; |2159| 

           MVKL    .S2     RL596,B3          ; |2159| 
           MVKH    .S2     RL596,B3          ; |2159| 
           NOP             2
           SUB     .L2     B8,B4,B4          ; 
RL596:     ; CALL OCCURS                     ; |2159| 
           LDHU    .D2T2   *+SP(40),B4       ; |2159| 
           NOP             4
           ADD     .L1X    A4,B4,A0          ; 
           STH     .D1T1   A0,*A9            ; |2159| 
	.line	1439
;------------------------------------------------------------------------------
; 2160 | *b = (((UINT16) GetRandNum()) % 3600);                                 
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2160| 
           MVKL    .S2     RL600,B3          ; |2160| 
           MVKH    .S2     RL600,B3          ; |2160| 
           NOP             3
RL600:     ; CALL OCCURS                     ; |2160| 
           B       .S1     __remi            ; |2160| 
           MVKL    .S2     RL602,B3          ; |2160| 
           MVKH    .S2     RL602,B3          ; |2160| 
           MVK     .S2     0xe10,B4          ; |2160| 
           NOP             2
RL602:     ; CALL OCCURS                     ; |2160| 
           STH     .D1T1   A4,*A7            ; |2160| 
;** --------------------------------------------------------------------------*
L125:    
	.line	1442
;------------------------------------------------------------------------------
; 2163 | hw = tanLUT[ *a ];                                 //    convert new po
;     | lar coords to (x,y) pix:                                               
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |2163| 
           LDW     .D2T1   *+SP(204),A3      ; |2163| 
           NOP             4
           LDH     .D1T1   *+A3[A0],A0       ; |2163| 
           NOP             4
           STH     .D2T1   A0,*+SP(8)        ; |2163| 
	.line	1443
;------------------------------------------------------------------------------
; 2164 | i32val = rectU * hw;                               //    r*2^20= [alpha
;     | X*2^10] * [tan(rDeg)*2^10]                                             
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B4        ; |2164| 
           NOP             4
           MPYUS   .M2X    A13,B4,B11        ; |2164| 
           NOP             1
	.line	1444
;------------------------------------------------------------------------------
; 2165 | i32val >>= 10;                                     //    r*2^20 --> r*2
;     | ^10                                                                    
;------------------------------------------------------------------------------
           SHR     .S2     B11,10,B11        ; |2165| 
	.line	1445
;------------------------------------------------------------------------------
; 2166 | i32val *= cosLUT[ *b ];                            //    x*2^20= [r*2^1
;     | 0] * [cos(theta) * 2^10]                                               
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(216),A3      ; |2166| 
           LDHU    .D1T1   *A7,A0            ; |2166| 
           NOP             4
           LDH     .D1T1   *+A3[A0],A3       ; |2166| 
           NOP             4
           MPYLH   .M1X    A3,B11,A0         ; |2166| 
           MPYSU   .M2X    A3,B11,B4         ; |2166| 
           SHL     .S1     A0,16,A0          ; |2166| 
           ADD     .L2X    B4,A0,B11         ; |2166| 
	.line	1446
;------------------------------------------------------------------------------
; 2167 | i32val >>= 4;                                      //    x(pix) = [x*2^
;     | 20]/16 = x*65536                                                       
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2167| 
	.line	1447
;------------------------------------------------------------------------------
; 2168 | i32val += xCoord;                                  //    offset by FOE'
;     | s x-coord                                                              
;------------------------------------------------------------------------------
           ADD     .L2X    A10,B11,B11       ; |2168| 
	.line	1448
;------------------------------------------------------------------------------
; 2169 | xyvals = ((0x0000FFFF & i32val) << 16);            //    pack x-coord f
;     | or download to dotter brd;                                             
; 2170 | //    "wraps" dots that exceed [0..65535]!                             
;------------------------------------------------------------------------------
           EXTU    .S2     B11,16,16,B4      ; |2169| 
           SHL     .S2     B4,16,B5          ; |2169| 
           STW     .D2T2   B5,*+SP(56)       ; |2169| 
	.line	1451
;------------------------------------------------------------------------------
; 2172 | i32val = rectD * hw;                               //    analogously fo
;     | r the y-coord, except we                                               
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B4        ; |2172| 
           NOP             4
           MPYUS   .M2     B9,B4,B11         ; |2172| 
           NOP             1
	.line	1452
;------------------------------------------------------------------------------
; 2173 | i32val >>= 10;                                     //    use the alphaY
;     |  conversion factor, and                                                
;------------------------------------------------------------------------------
           SHR     .S2     B11,10,B11        ; |2173| 
	.line	1453
;------------------------------------------------------------------------------
; 2174 | i32val *= sinLUT[ *b ];                            //    sin(theta)... 
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(212),B6      ; |2174| 
           LDHU    .D1T2   *A7,B4            ; |2174| 
           NOP             4
           LDH     .D2T2   *+B6[B4],B6       ; |2174| 
           NOP             4
           MPYLH   .M2     B6,B11,B4         ; |2174| 
           MPYSU   .M2     B6,B11,B6         ; |2174| 
           SHL     .S2     B4,16,B4          ; |2174| 
           ADD     .L2     B6,B4,B11         ; |2174| 
	.line	1454
;------------------------------------------------------------------------------
; 2175 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2175| 
	.line	1455
;------------------------------------------------------------------------------
; 2176 | i32val += yCoord;                                  //    ... and we off
;     | set by FOE's y-coord                                                   
;------------------------------------------------------------------------------
           ADD     .L2X    A11,B11,B11       ; |2176| 
	.line	1456
;------------------------------------------------------------------------------
; 2177 | xyvals |= (0x0000FFFF & i32val);                                       
;------------------------------------------------------------------------------
           EXTU    .S2     B11,16,16,B4      ; |2177| 
           OR      .L2     B4,B5,B4          ; |2177| 
           STW     .D2T2   B4,*+SP(56)       ; |2177| 
	.line	1458
;------------------------------------------------------------------------------
; 2179 | while( *stataddr & 0x1 );                          //    finally:  draw
;     |  the dot!                                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |2179| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2179| 
           NOP             4
           AND     .L1X    1,B4,A1           ; |2179| 
   [!A1]   B       .S1     L127              ; |2179| 
           NOP             5
           ; BRANCH OCCURS                   ; |2179| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L126:    
           LDW     .D2T2   *+SP(76),B4       ; |2179| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2179| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2179| 
   [ B0]   B       .S1     L126              ; |2179| 
           NOP             5
           ; BRANCH OCCURS                   ; |2179| 
;** --------------------------------------------------------------------------*
L127:    
	.line	1459
;------------------------------------------------------------------------------
; 2180 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B4       ; |2180| 
           LDW     .D2T2   *+SP(72),B5       ; |2180| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |2180| 
	.line	1460
;------------------------------------------------------------------------------
; 2181 | visibleDotsXY[nTotalVisDots++] = xyvals;           //    save packed co
;     | ords in visi dots array                                                
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B5      ; |2181| 
           LDW     .D2T2   *+SP(56),B4       ; |2181| 
           MV      .L2X    A12,B6            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B4,*+B5[B6]       ; |2181| 
	.line	1462
;------------------------------------------------------------------------------
; 2183 | ++a;                                               //    move on to nex
;     | t dot                                                                  
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |2183| 
	.line	1463
;------------------------------------------------------------------------------
; 2184 | ++b;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |2184| 
	.line	1464
;------------------------------------------------------------------------------
; 2185 | ++nextDotLife;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2185| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2185| 
           STW     .D2T2   B4,*+SP(140)      ; |2185| 
	.line	1465
           CMPLTU  .L1     A9,A14,A1         ; |2186| 
   [ A1]   B       .S1     L123              ; |2186| 
           NOP             5
           ; BRANCH OCCURS                   ; |2186| 
;** --------------------------------------------------------------------------*
	.line	1466
           B       .S1     L133              ; |2187| 
           NOP             5
           ; BRANCH OCCURS                   ; |2187| 
;** --------------------------------------------------------------------------*
L128:    
	.line	1467
;------------------------------------------------------------------------------
; 2188 | else while( a < de )                                     //    FOR ACCE
;     | LERATING FLOWS (simpler):                                              
;------------------------------------------------------------------------------
           CMPLTU  .L1     A9,A14,A1         ; |2188| 
   [!A1]   B       .S1     L133              ; |2188| 
           NOP             5
           ; BRANCH OCCURS                   ; |2188| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L129
;** --------------------------------------------------------------------------*
L129:    
	.line	1469
;------------------------------------------------------------------------------
; 2190 | i32val = hv * sincosLUT[ *a ];                        //    dr*2^(10+M)
;     | = [B*2^M]*[sin(r)*cos(r)*2^10]                                         
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A0            ; |2190| 
           LDW     .D2T1   *+SP(208),A3      ; |2190| 
           LDH     .D2T2   *+SP(4),B4        ; |2190| 
           NOP             3
           LDH     .D1T1   *+A3[A0],A0       ; |2190| 
           NOP             4
           MPY     .M2X    A0,B4,B11         ; |2190| 
           NOP             1
	.line	1470
;------------------------------------------------------------------------------
; 2191 | i32val >>= (2 + vv);                                  //    dr*2^(10+M)
;     |  --> dr*2^8                                                            
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(6),B4        ; |2191| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2191| 
           SHR     .S2     B11,B4,B11        ; |2191| 
	.line	1471
;------------------------------------------------------------------------------
; 2192 | i32val += (INT32) *nextDotLife;                       //    accum fract
;     | ional pos change from last upd                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B5      ; |2192| 
           NOP             4
           LDH     .D2T2   *B5,B4            ; |2192| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |2192| 
	.line	1473
;------------------------------------------------------------------------------
; 2194 | *nextDotLife = 0x00FF & i32val;                       //    carry over 
;     | frac pos chng for next update                                          
;------------------------------------------------------------------------------
           EXTU    .S2     B11,24,24,B4      ; |2194| 
           STH     .D2T2   B4,*B5            ; |2194| 
	.line	1474
;------------------------------------------------------------------------------
; 2195 | i32val >>= 8;                                         //    dr*2^8 --> 
;     | dr                                                                     
;------------------------------------------------------------------------------
           SHR     .S2     B11,8,B11         ; |2195| 
	.line	1475
;------------------------------------------------------------------------------
; 2196 | i32val += (INT32) *a;                                 //    r' = r + dr
;------------------------------------------------------------------------------
           LDHU    .D1T2   *A9,B4            ; |2196| 
           NOP             4
           ADD     .L2     B4,B11,B11        ; |2196| 
	.line	1476
;------------------------------------------------------------------------------
; 2197 | *a = (UINT16) i32val;                                 //    update new 
;     | radial pos                                                             
;------------------------------------------------------------------------------
           STH     .D1T2   B11,*A9           ; |2197| 
	.line	1477
;------------------------------------------------------------------------------
; 2198 | if( i32val > (INT32) rectL )                          //    randomly re
;     | pos dots that pass outer edge                                          
;------------------------------------------------------------------------------
           CMPGT   .L2     B11,B8,B0         ; |2198| 
   [!B0]   B       .S1     L130              ; |2198| 
           NOP             5
           ; BRANCH OCCURS                   ; |2198| 
;** --------------------------------------------------------------------------*
	.line	1479
;------------------------------------------------------------------------------
; 2200 | *a = rectR +                                                           
; 2201 | (((UINT16) GetRandNum()) % (rectL-rectR));                             
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2200| 
           MVKL    .S2     RL606,B3          ; |2200| 
           MVKH    .S2     RL606,B3          ; |2200| 
           NOP             3
RL606:     ; CALL OCCURS                     ; |2200| 
           B       .S1     __remi            ; |2200| 
           MVKL    .S2     RL608,B3          ; |2200| 
           MVKH    .S2     RL608,B3          ; |2200| 
           SUB     .L2X    B8,A8,B4          ; 
           NOP             2
RL608:     ; CALL OCCURS                     ; |2200| 
           ADD     .L1     A4,A8,A0          ; 
           STH     .D1T1   A0,*A9            ; |2200| 
	.line	1481
;------------------------------------------------------------------------------
; 2202 | *b = (((UINT16) GetRandNum()) % 3600);                                 
;------------------------------------------------------------------------------
           B       .S1     _GetRandNum       ; |2202| 
           MVKL    .S2     RL612,B3          ; |2202| 
           MVKH    .S2     RL612,B3          ; |2202| 
           NOP             3
RL612:     ; CALL OCCURS                     ; |2202| 
           B       .S1     __remi            ; |2202| 
           MVKL    .S2     RL614,B3          ; |2202| 
           MVKH    .S2     RL614,B3          ; |2202| 
           MVK     .S2     0xe10,B4          ; |2202| 
           NOP             2
RL614:     ; CALL OCCURS                     ; |2202| 
           STH     .D1T1   A4,*A7            ; |2202| 
;** --------------------------------------------------------------------------*
L130:    
	.line	1484
;------------------------------------------------------------------------------
; 2205 | hw = tanLUT[ *a ];                                    //    convert new
;     |  polar coords to (x,y) pix:                                            
;------------------------------------------------------------------------------
           LDHU    .D1T1   *A9,A3            ; |2205| 
           LDW     .D2T1   *+SP(204),A0      ; |2205| 
           NOP             4
           LDH     .D1T1   *+A0[A3],A0       ; |2205| 
           NOP             4
           STH     .D2T1   A0,*+SP(8)        ; |2205| 
	.line	1485
;------------------------------------------------------------------------------
; 2206 | i32val = rectU * hw;                                  //    r*2^20= [al
;     | phaX*2^10] * [tan(rDeg)*2^10]                                          
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B4        ; |2206| 
           NOP             4
           MPYUS   .M2X    A13,B4,B11        ; |2206| 
           NOP             1
	.line	1486
;------------------------------------------------------------------------------
; 2207 | i32val >>= 10;                                        //    r*2^20 --> 
;     | r*2^10                                                                 
;------------------------------------------------------------------------------
           SHR     .S2     B11,10,B11        ; |2207| 
	.line	1487
;------------------------------------------------------------------------------
; 2208 | i32val *= cosLUT[ *b ];                               //    x*2^20= [r*
;     | 2^10] * [cos(theta) * 2^10]                                            
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(216),A3      ; |2208| 
           LDHU    .D1T1   *A7,A0            ; |2208| 
           NOP             4
           LDH     .D1T1   *+A3[A0],A3       ; |2208| 
           NOP             4
           MPYLH   .M1X    A3,B11,A0         ; |2208| 
           MPYSU   .M2X    A3,B11,B4         ; |2208| 
           SHL     .S1     A0,16,A0          ; |2208| 
           ADD     .L2X    B4,A0,B11         ; |2208| 
	.line	1488
;------------------------------------------------------------------------------
; 2209 | i32val >>= 4;                                         //    x(pix) = [x
;     | *2^20]/16 = x*65536                                                    
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2209| 
	.line	1489
;------------------------------------------------------------------------------
; 2210 | i32val += xCoord;                                     //    offset by F
;     | OE's x-coord                                                           
;------------------------------------------------------------------------------
           ADD     .L2X    A10,B11,B11       ; |2210| 
	.line	1490
;------------------------------------------------------------------------------
; 2211 | xyvals = ((0x0000FFFF & i32val) << 16);               //    pack x-coor
;     | d for download to dotter brd                                           
;------------------------------------------------------------------------------
           EXTU    .S2     B11,16,16,B4      ; |2211| 
           SHL     .S2     B4,16,B4          ; |2211| 
           STW     .D2T2   B4,*+SP(56)       ; |2211| 
	.line	1492
;------------------------------------------------------------------------------
; 2213 | i32val = rectD * hw;                                  //    analogously
;     |  for y-coord, except we use                                            
;------------------------------------------------------------------------------
           LDH     .D2T2   *+SP(8),B5        ; |2213| 
           NOP             4
           MPYUS   .M2     B9,B5,B11         ; |2213| 
           NOP             1
	.line	1493
;------------------------------------------------------------------------------
; 2214 | i32val >>= 10;                                        //    alphaY and 
;     | sin(theta)...                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,10,B11        ; |2214| 
	.line	1494
;------------------------------------------------------------------------------
; 2215 | i32val *= sinLUT[ *b ];                                                
;------------------------------------------------------------------------------
           LDHU    .D1T2   *A7,B6            ; |2215| 
           LDW     .D2T2   *+SP(212),B5      ; |2215| 
           NOP             4
           LDH     .D2T2   *+B5[B6],B6       ; |2215| 
           NOP             4
           MPYLH   .M2     B6,B11,B5         ; |2215| 
           MPYSU   .M2     B6,B11,B6         ; |2215| 
           SHL     .S2     B5,16,B5          ; |2215| 
           ADD     .L2     B6,B5,B11         ; |2215| 
	.line	1495
;------------------------------------------------------------------------------
; 2216 | i32val >>= 4;                                                          
;------------------------------------------------------------------------------
           SHR     .S2     B11,4,B11         ; |2216| 
	.line	1496
;------------------------------------------------------------------------------
; 2217 | i32val += yCoord;                                     //    ... and we 
;     | offset by FOE's y-coord                                                
;------------------------------------------------------------------------------
           ADD     .L2X    A11,B11,B11       ; |2217| 
	.line	1497
;------------------------------------------------------------------------------
; 2218 | xyvals |= (0x0000FFFF & i32val);                                       
;------------------------------------------------------------------------------
           EXTU    .S2     B11,16,16,B5      ; |2218| 
           OR      .L2     B5,B4,B4          ; |2218| 
           STW     .D2T2   B4,*+SP(56)       ; |2218| 
	.line	1499
;------------------------------------------------------------------------------
; 2220 | while( *stataddr & 0x1 );                             //    finally:  d
;     | raw the dot!                                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |2220| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2220| 
           NOP             4
           AND     .L1X    1,B4,A1           ; |2220| 
   [!A1]   B       .S1     L132              ; |2220| 
           NOP             5
           ; BRANCH OCCURS                   ; |2220| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L131:    
           LDW     .D2T2   *+SP(76),B4       ; |2220| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2220| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2220| 
   [ B0]   B       .S1     L131              ; |2220| 
           NOP             5
           ; BRANCH OCCURS                   ; |2220| 
;** --------------------------------------------------------------------------*
L132:    
	.line	1500
;------------------------------------------------------------------------------
; 2221 | *locaddr = xyvals;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(56),B5       ; |2221| 
           LDW     .D2T2   *+SP(72),B4       ; |2221| 
           NOP             4
           STW     .D2T2   B5,*B4            ; |2221| 
	.line	1501
;------------------------------------------------------------------------------
; 2222 | visibleDotsXY[nTotalVisDots++] = xyvals;              //    save packed
;     |  coords in visi dots array                                             
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(192),B5      ; |2222| 
           LDW     .D2T2   *+SP(56),B4       ; |2222| 
           MV      .L2X    A12,B6            ; 
           ADD     .L1     1,A12,A12         ; 
           NOP             2
           STW     .D2T2   B4,*+B5[B6]       ; |2222| 
	.line	1503
;------------------------------------------------------------------------------
; 2224 | ++a;                                                  //    move on to 
;     | next dot                                                               
;------------------------------------------------------------------------------
           ADD     .L1     2,A9,A9           ; |2224| 
	.line	1504
;------------------------------------------------------------------------------
; 2225 | ++b;                                                                   
;------------------------------------------------------------------------------
           ADD     .L1     2,A7,A7           ; |2225| 
	.line	1505
;------------------------------------------------------------------------------
; 2226 | ++nextDotLife;                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(140),B4      ; |2226| 
           NOP             4
           ADD     .L2     2,B4,B4           ; |2226| 
           STW     .D2T2   B4,*+SP(140)      ; |2226| 
	.line	1506
           CMPLTU  .L1     A9,A14,A1         ; |2227| 
   [ A1]   B       .S1     L129              ; |2227| 
           NOP             5
           ; BRANCH OCCURS                   ; |2227| 
;** --------------------------------------------------------------------------*
L133:    
	.line	1510
;------------------------------------------------------------------------------
; 2231 | u16Dummy = (UINT16) data[cd + NREPS];                       // decremen
;     | t #reps for this tgt                                                   
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(108),B4      ; |2231| 
           LDW     .D2T1   *+SP(200),A3      ; |2231| 
           NOP             3
           ADD     .L1X    4,B4,A0           ; |2231| 
           LDHU    .D1T1   *+A3[A0],A0       ; |2231| 
           NOP             4
           STH     .D2T1   A0,*+SP(38)       ; |2231| 
	.line	1511
;------------------------------------------------------------------------------
; 2232 | if( u16Type == DOTLIFEWIN || u16Type == DL_NOISEDIR ||      // (be sure
;     |  to mask out dot life decr in reps                                     
; 2233 | u16Type == DL_NOISESPEED )                              // field for th
;     | e "limited dot life" tgt types)                                        
;------------------------------------------------------------------------------
           LDHU    .D2T2   *+SP(36),B4       ; |2232| 
           NOP             4
           CMPEQ   .L2     B4,8,B0           ; |2232| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |2232| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,11,B0          ; |2232| 
   [!B0]   LDHU    .D2T2   *+SP(36),B4       ; |2232| 
           NOP             4
   [!B0]   CMPEQ   .L2     B4,13,B0          ; |2232| 
	.line	1513
;------------------------------------------------------------------------------
; 2234 | u16Dummy &= 0x000000FF;                                                
;------------------------------------------------------------------------------
   [ B0]   LDHU    .D2T2   *+SP(38),B4       ; |2234| 
           NOP             4
   [ B0]   EXTU    .S2     B4,24,24,B4       ; |2234| 
   [ B0]   STH     .D2T2   B4,*+SP(38)       ; |2234| 
	.line	1514
;------------------------------------------------------------------------------
; 2235 | nRedrawsLeft[d] = u16Dummy;                                            
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A3      ; |2235| 
           LDW     .D2T1   *+SP(180),A0      ; |2235| 
           LDHU    .D2T2   *+SP(38),B4       ; |2235| 
           NOP             4
           STH     .D1T2   B4,*+A0[A3]       ; |2235| 
	.line	1515
;------------------------------------------------------------------------------
; 2236 | nRedrawsLeft[d]--;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(180),B5      ; |2236| 
           LDW     .D2T2   *+SP(104),B4      ; |2236| 
           NOP             4
           ADDAH   .D2     B5,B4,B4          ; |2236| 
           LDHU    .D2T2   *B4,B5            ; |2236| 
           NOP             4
           SUB     .L2     B5,1,B5           ; |2236| 
           STH     .D2T2   B5,*B4            ; |2236| 
;** --------------------------------------------------------------------------*
L134:    
	.line	1516
;------------------------------------------------------------------------------
; 2237 | }                                                              // END: 
;     |  first pass thru all targets                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |2237| 
           NOP             4
           ADD     .L2     1,B4,B5           ; |2237| 
           STW     .D2T2   B5,*+SP(104)      ; |2237| 
           LDW     .D2T2   *+SP(168),B4      ; |2237| 
           NOP             4
           LDHU    .D2T2   *+B4(10),B4       ; |2237| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [ B0]   B       .S1     L40               ; |2237| 
           NOP             5
           ; BRANCH OCCURS                   ; |2237| 
;** --------------------------------------------------------------------------*
L135:    
	.line	1518
;------------------------------------------------------------------------------
; 2239 | if( maxRepeats > 0 ) for( i = 0; i < maxRepeats-1; i++ )       // now c
;     | omplete remaining reps for all tgts                                    
; 2240 | {                                                              // by us
;     | ing the visible dots array we prepared                                 
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(116),A1      ; |2239| 
           NOP             4
   [!A1]   B       .S1     L144              ; |2239| 
           NOP             5
           ; BRANCH OCCURS                   ; |2239| 
;** --------------------------------------------------------------------------*
           ZERO    .L1     A0                ; |2239| 
           STW     .D2T1   A0,*+SP(84)       ; |2239| 
           SUB     .L1     A1,1,A3           ; 
           CMPLTU  .L1     A0,A3,A1          ; 
   [!A1]   B       .S1     L144              ; |2239| 
           NOP             5
           ; BRANCH OCCURS                   ; |2239| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L136
;** --------------------------------------------------------------------------*
L136:    
	.line	1520
;------------------------------------------------------------------------------
; 2241 | nTotalVisDots = 0;                                          // in the f
;     | irst pass!!!  This implementation                                      
;------------------------------------------------------------------------------
           ZERO    .L1     A12               ; |2241| 
	.line	1521
;------------------------------------------------------------------------------
; 2242 | for( d = 0; d < parameters->wNumTargets; d++ )              // allows f
;     | or different NREPS values for                                          
; 2243 | {                                                           // differen
;     | t tgts.  Note that its is important                                    
;------------------------------------------------------------------------------
           ZERO    .L2     B5                ; |2242| 
           STW     .D2T2   B5,*+SP(104)      ; |2242| 
           LDW     .D2T2   *+SP(168),B4      ; |2242| 
           NOP             4
           LDHU    .D2T2   *+B4(10),B4       ; |2242| 
           NOP             4
           CMPLTU  .L2     B5,B4,B0          ; 
   [!B0]   B       .S1     L143              ; |2242| 
           NOP             5
           ; BRANCH OCCURS                   ; |2242| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L137
;** --------------------------------------------------------------------------*
L137:    
	.line	1523
;------------------------------------------------------------------------------
; 2244 | if( nRedrawsLeft[d] > 0 )                                // to cycle th
;     | ru the targets rather than                                             
; 2245 | {                                                        // redrawing t
;     | gt1 N times, tgt2 M times, etc.                                        
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B5      ; |2244| 
           LDW     .D2T2   *+SP(180),B4      ; |2244| 
           NOP             4
           LDHU    .D2T2   *+B4[B5],B4       ; |2244| 
           NOP             4
           CMPGT   .L2     B4,0,B0           ; |2244| 
   [!B0]   B       .S1     L142              ; |2244| 
           NOP             5
           ; BRANCH OCCURS                   ; |2244| 
;** --------------------------------------------------------------------------*
	.line	1525
;------------------------------------------------------------------------------
; 2246 | k = nTotalVisDots + nVisDotsPerTgt[d];                // the former app
;     | roach move evenly distributes                                          
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(184),B4      ; |2246| 
           NOP             4
           LDHU    .D2T2   *+B4[B5],B4       ; |2246| 
           NOP             4
           ADD     .L1X    B4,A12,A0         ; |2246| 
           STW     .D2T1   A0,*+SP(92)       ; |2246| 
	.line	1526
;------------------------------------------------------------------------------
; 2247 | for( j = nTotalVisDots; j < k; j++ )                  // individual dot
;     |  refreshes over update period.                                         
;------------------------------------------------------------------------------
           STW     .D2T1   A12,*+SP(88)      ; |2247| 
           CMPLTU  .L1     A12,A0,A1         ; 
   [!A1]   B       .S1     L141              ; |2247| 
           NOP             5
           ; BRANCH OCCURS                   ; |2247| 
;** --------------------------------------------------------------------------*
;**   BEGIN LOOP L138
;** --------------------------------------------------------------------------*
L138:    
	.line	1528
;------------------------------------------------------------------------------
; 2249 | while( *stataddr & 0x1 );                                              
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(76),B4       ; |2249| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2249| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2249| 
   [!B0]   B       .S1     L140              ; |2249| 
           NOP             5
           ; BRANCH OCCURS                   ; |2249| 
;*----------------------------------------------------------------------------*
;*   SOFTWARE PIPELINE INFORMATION
;*      Disqualified loop: software pipelining disabled
;*----------------------------------------------------------------------------*
L139:    
           LDW     .D2T2   *+SP(76),B4       ; |2249| 
           NOP             4
           LDW     .D2T2   *B4,B4            ; |2249| 
           NOP             4
           AND     .L2     1,B4,B0           ; |2249| 
   [ B0]   B       .S1     L139              ; |2249| 
           NOP             5
           ; BRANCH OCCURS                   ; |2249| 
;** --------------------------------------------------------------------------*
L140:    
	.line	1529
;------------------------------------------------------------------------------
; 2250 | *locaddr = visibleDotsXY[j];                                           
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(88),B4       ; |2250| 
           LDW     .D2T2   *+SP(192),B5      ; |2250| 
           NOP             4
           LDW     .D2T2   *+B5[B4],B4       ; |2250| 
           LDW     .D2T2   *+SP(72),B5       ; |2250| 
           NOP             4
           STW     .D2T2   B4,*B5            ; |2250| 
	.line	1530
           LDW     .D2T2   *+SP(88),B4       ; |2251| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |2251| 
           STW     .D2T2   B4,*+SP(88)       ; |2251| 
           LDW     .D2T2   *+SP(92),B5       ; |2251| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L138              ; |2251| 
           NOP             5
           ; BRANCH OCCURS                   ; |2251| 
;** --------------------------------------------------------------------------*
L141:    
	.line	1531
;------------------------------------------------------------------------------
; 2252 | nRedrawsLeft[d]--;                                                     
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(104),B4      ; |2252| 
           LDW     .D2T2   *+SP(180),B5      ; |2252| 
           NOP             4
           ADDAH   .D2     B5,B4,B5          ; |2252| 
           LDHU    .D2T2   *B5,B4            ; |2252| 
           NOP             4
           SUB     .L2     B4,1,B4           ; |2252| 
           STH     .D2T2   B4,*B5            ; |2252| 
;** --------------------------------------------------------------------------*
L142:    
	.line	1533
;------------------------------------------------------------------------------
; 2254 | nTotalVisDots += nVisDotsPerTgt[d];                                    
;------------------------------------------------------------------------------
           LDW     .D2T1   *+SP(104),A0      ; |2254| 
           LDW     .D2T1   *+SP(184),A3      ; |2254| 
           NOP             4
           LDHU    .D1T1   *+A3[A0],A0       ; |2254| 
           NOP             4
           ADD     .L1     A0,A12,A12        ; |2254| 
	.line	1534
           LDW     .D2T2   *+SP(104),B4      ; |2255| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |2255| 
           STW     .D2T2   B4,*+SP(104)      ; |2255| 
           LDW     .D2T2   *+SP(168),B5      ; |2255| 
           NOP             4
           LDHU    .D2T2   *+B5(10),B5       ; |2255| 
           NOP             4
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L137              ; |2255| 
           NOP             5
           ; BRANCH OCCURS                   ; |2255| 
;** --------------------------------------------------------------------------*
L143:    
	.line	1535
;------------------------------------------------------------------------------
; 2258 | }                                                                 // EN
;     | D:  process XYCORE_DOFRAME command...                                  
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(84),B4       ; |2256| 
           NOP             4
           ADD     .L2     1,B4,B4           ; |2256| 
           STW     .D2T2   B4,*+SP(84)       ; |2256| 
           LDW     .D2T2   *+SP(116),B5      ; |2256| 
           NOP             4
           SUB     .L2     B5,1,B5           ; |2256| 
           CMPLTU  .L2     B4,B5,B0          ; 
   [ B0]   B       .S1     L136              ; |2256| 
           NOP             5
           ; BRANCH OCCURS                   ; |2256| 
;** --------------------------------------------------------------------------*
L144:    
	.line	1540
;------------------------------------------------------------------------------
; 2261 | status = XYCORE_READY;                                            // wr
;     | ite XYCORE_READY into CmdStat register to                              
; 2262 | #if defined(_TGTDAKARF5)                                          // in
;     | form host XYAPI we're ready for next cmd.                              
; 2263 | C4X_Control( PCI, SET_MAILBOX, 2, &status );                           
; 2264 | #elif defined(_TGTDETROIT)                                             
;------------------------------------------------------------------------------
           MVK     .S2     1,B4              ; |2261| 
           STW     .D2T2   B4,*+SP(52)       ; |2261| 
	.line	1544
;------------------------------------------------------------------------------
; 2265 | C6x_WritePlx( MAILBOX2_OFFSET, status );                               
; 2266 | #else    // _TGTDAYTONA                                                
; 2267 | *((UINT32 *)0x00400000) = status;                                      
; 2268 | #endif                                                                 
;------------------------------------------------------------------------------
           MVKL    .S1     0x16000c8,A0      ; |2265| 
           MVKH    .S1     0x16000c8,A0      ; |2265| 
           STW     .D1T2   B4,*A0            ; |2265| 
	.line	1549
;------------------------------------------------------------------------------
; 2270 | } while( command != XYCORE_CLOSE );                                  //
;     |  END runtime loop                                                      
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(48),B4       ; |2270| 
           NOP             4
           CMPEQ   .L2     B4,4,B0           ; |2270| 
   [!B0]   B       .S1     L8                ; |2270| 
           NOP             5
           ; BRANCH OCCURS                   ; |2270| 
;** --------------------------------------------------------------------------*
	.line	1551
;------------------------------------------------------------------------------
; 2272 | }                                                                      
;     |  // END main()                                                         
;------------------------------------------------------------------------------
           LDW     .D2T2   *+SP(248),B3      ; |2272| 
           LDW     .D2T2   *+SP(260),B12     ; |2272| 
           LDW     .D2T2   *+SP(256),B11     ; |2272| 
           LDW     .D2T2   *+SP(252),B10     ; |2272| 
           LDW     .D2T1   *+SP(244),A14     ; |2272| 
           LDW     .D2T1   *+SP(240),A13     ; |2272| 
           LDW     .D2T1   *+SP(236),A12     ; |2272| 
           LDW     .D2T1   *+SP(232),A11     ; |2272| 

           B       .S2     B3                ; |2272| 
||         LDW     .D2T1   *+SP(264),A15     ; |2272| 

           LDW     .D2T1   *+SP(228),A10     ; |2272| 
           ADDK    .S2     264,SP            ; |2272| 
           NOP             3
           ; BRANCH OCCURS                   ; |2272| 
	.endfunc	2272,01c08fc00h,264


;******************************************************************************
;* MARK THE END OF THE SCALAR INIT RECORD IN CINIT:C                          *
;******************************************************************************
CIR:	.sect	".cinit:c"
;******************************************************************************
;* UNDEFINED EXTERNAL REFERENCES                                              *
;******************************************************************************
	.global	_log
	.global	_pow
	.global	_sin
	.global	_cos
	.global	_tan
	.global	_floor
	.global	_C6x_ControlLed
	.global	_C6x_ControlResetDspLink3
	.global	_C6x_OpenC6x
	.global	_C6x_OpenPlx
	.global	__mpyd
	.global	__addd
	.global	__fltud
	.global	__negd
	.global	__fixdi
	.global	__divd
	.global	__subd
	.global	_memcpy
	.global	__divi
	.global	__remi
