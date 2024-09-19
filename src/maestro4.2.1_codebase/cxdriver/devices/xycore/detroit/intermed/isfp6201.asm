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

;	acp6x -q -D_TGTDETROIT -Iinclude -I.. -m --i_output_file intermed\isfp6201.if --template_info_file intermed\isfp6201.ti --object_file intermed\isfp6201.o6x --opt_shell 13 isfp6201.c -al -c -eo.o6x -gkqs -D_TGTDETROIT -frintermed -fsintermed -ftintermed -iinclude -i.. -c isfp6201.c isfp6201.c 
	.file	"isfp6201.c"
 .sect .vectors        ; point this section towards vectors      
 ; RESET ISFP (usually comes from rts6201.lib)                     
 .ref _c_int00              ; provide reference to reset function  
    MVKL .S2 _c_int00,B1    ; Op 1: Load low half of ISR address   
    MVKH .S2 _c_int00,B1    ; Op 2: Load high half of ISR address  
    B    .S2 B1             ; Op 3: Branch to ISR                  
    NOP                     ; Op 4:                                
    NOP                     ; Op 5:                                
    NOP                     ; Op 6:                                
    NOP                     ; Op 7:                                
    NOP                     ; Op 8:                                
 ; NMI ISFP                                                     
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; RESERVED ISFP                                                
 NOP                  ; Op 1: leave reserved ISFP empty         
 NOP                  ; Op 2:                                   
 NOP                  ; Op 3:                                   
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; RESERVED ISFP                                                
 NOP                  ; Op 1: leave reserved ISFP empty         
 NOP                  ; Op 2:                                   
 NOP                  ; Op 3:                                   
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT4 ISFP - External Interrupt pin 4                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT5 ISFP - External Interrupt pin 5                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT6 ISFP - External Interrupt pin 6                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT7 ISFP - External Interrupt pin 7                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT8 ISFP - DMA Channel 0 Interrupt                          
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT9 ISFP - DMA Channel 1 Interrupt                          
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT10 ISFP - EMIF SDRAM Timer interrupt                      
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT11 ISFP - DMA Channel 2 Interrupt                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT12 ISFP - DMA Channel 3 Interrupt                         
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT13 ISFP - Host Port Host-to-DSP Interrupt                 
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT14 ISFP - Timer 0 Interrupt                               
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 ; INT15 ISFP - Timer 0 Interrupt                               
 NOP                  ; Op 1: no ISR for this vector            
 NOP                  ; Op 2:                                   
 B    .S2 IRP         ; Op 3: return to calling point           
 NOP                  ; Op 4:                                   
 NOP                  ; Op 5:                                   
 NOP                  ; Op 6:                                   
 NOP                  ; Op 7:                                   
 NOP                  ; Op 8:                                   
 .text                ; Set back to .text section for normal code
