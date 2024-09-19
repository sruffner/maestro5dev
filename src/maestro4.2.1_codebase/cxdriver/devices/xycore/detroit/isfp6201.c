/*SDOC-------------------------------------------------------------------------
Spectrum Signal Processing Inc. Copyright 1997

$Workfile:: isfp6201.c                                                        $

$Modtime:: 5/21/98 10:02p                                                     $

$Revision:: 6                                                                 $

$Archive:: /MAC/de62_154.v10/dev/testwntssp/src/dspflash/isfp6201.c           $

Contents:  This DSP file is the interrupt vector table

10jan2006(sar): Changed inst MVK to MVKL in RESET ISFP b/c address _c_int00
is now >32767. (Relocation problem)
-------------------------------------------------------------------------EDOC*/

   asm(" .sect .vectors        ; point this section towards vectors      ");

   asm(" ; RESET ISFP (usually comes from rts6201.lib)                     ");
   asm(" .ref _c_int00              ; provide reference to reset function  ");
   asm("    MVKL .S2 _c_int00,B1    ; Op 1: Load low half of ISR address   ");
   asm("    MVKH .S2 _c_int00,B1    ; Op 2: Load high half of ISR address  ");
   asm("    B    .S2 B1             ; Op 3: Branch to ISR                  ");
   asm("    NOP                     ; Op 4:                                ");
   asm("    NOP                     ; Op 5:                                ");
   asm("    NOP                     ; Op 6:                                ");
   asm("    NOP                     ; Op 7:                                ");
   asm("    NOP                     ; Op 8:                                ");

   asm(" ; NMI ISFP                                                     ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; RESERVED ISFP                                                ");
   asm(" NOP                  ; Op 1: leave reserved ISFP empty         ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" NOP                  ; Op 3:                                   ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; RESERVED ISFP                                                ");
   asm(" NOP                  ; Op 1: leave reserved ISFP empty         ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" NOP                  ; Op 3:                                   ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT4 ISFP - External Interrupt pin 4                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT5 ISFP - External Interrupt pin 5                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT6 ISFP - External Interrupt pin 6                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT7 ISFP - External Interrupt pin 7                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT8 ISFP - DMA Channel 0 Interrupt                          ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT9 ISFP - DMA Channel 1 Interrupt                          ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT10 ISFP - EMIF SDRAM Timer interrupt                      ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT11 ISFP - DMA Channel 2 Interrupt                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT12 ISFP - DMA Channel 3 Interrupt                         ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT13 ISFP - Host Port Host-to-DSP Interrupt                 ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT14 ISFP - Timer 0 Interrupt                               ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" ; INT15 ISFP - Timer 0 Interrupt                               ");
   asm(" NOP                  ; Op 1: no ISR for this vector            ");
   asm(" NOP                  ; Op 2:                                   ");
   asm(" B    .S2 IRP         ; Op 3: return to calling point           ");
   asm(" NOP                  ; Op 4:                                   ");
   asm(" NOP                  ; Op 5:                                   ");
   asm(" NOP                  ; Op 6:                                   ");
   asm(" NOP                  ; Op 7:                                   ");
   asm(" NOP                  ; Op 8:                                   ");

   asm(" .text                ; Set back to .text section for normal code");
