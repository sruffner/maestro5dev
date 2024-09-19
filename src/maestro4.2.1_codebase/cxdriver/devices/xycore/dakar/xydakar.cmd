/****************************************************************************/
/*  xydakar.cmd                                                             */
/*  [Adapted from c4xlink.cmd from Spectrum Signal Processing]              */
/****************************************************************************/

intermed\xycore.obj
-o xydakar.out
-cr
-m intermed\xydakar.map
-L lib
-l f5_c4x.lib        /* F5 C4x application library (small-memory, stack-based models) */
-l rts40.lib         /* TI run-time support library (small-memory, stack-based models) */


/* Specify standard memory configuration  */
MEMORY
{
   IRAM0:  origin = 002FF800h   length = 0400h   /* Internal RAM 0, 1K      */
   IRAM1:  origin = 002FFC00h   length = 03F0h   /* Internal RAM 1,         */
   NLRAM:  origin = 00300000h   length = 80000h  /* Near Local SRAM, 512K   */
   PEROM:  origin = 40000000h   length = 8000h   /* PEROM, 32K              */
   NGRAM:  origin = 80000000h   length = 80000h  /* Near Global SRAM, 512K  */
   FGRAM:  origin = 0C0300900h  length = 7F700h  /* Far Global SRAM, 512K   */
}

/* Specify output sections */
SECTIONS
{
   .text           : { }   > NLRAM
   .data           : { }   > NLRAM
   .bss            : { }   > NLRAM
   .const          : { }   > NLRAM
   .cinit          : { }   > NLRAM
   .stack          : { }   > NLRAM
   .vector         : { }   > IRAM1
   .user1          : { }   > FGRAM
}


