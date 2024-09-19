/****************************************************************************/
/*  xydetroit.cmd                                                           */
/*  [Adapted from de62lnk.cmd from Spectrum Signal Processing]              */
/****************************************************************************/

intermed\xycore.o6x
intermed\isfp6201.o6x
-o xydetroit.out
-c
-q
-heap  0x200
-stack 0x400
-m intermed\xydetroit.map
-L lib
-l de62c6x.lib
-l rts6201.lib


MEMORY   /* Map 1 Mode of C6x */
{
   /* Internal */
   IVECS:   org = 00000000h   len = 0000200h /* reset & interrupt vectors */
   IPROG:   org = 00000200h   len = 000fe00h /* 64 Kbytes - IVECS*/

   /* EMIF - Chip Enable 0 */
   SSRAM:   org = 00400000h   len = 0080000h /* 512 Kbytes */

   /* EMIF - Chip Enable 1 */
   GSRAM:   org = 01400000h   len = 0200000h /* 2 Mbytes */
   PLXREG:  org = 01600000h   len = 0020000h /* 128 Kbytes */
   DL3:     org = 01700000h   len = 0100000h /* 1 Mbyte -> 4 256Kbyte regions */

   /* EMIF - Chip Enable 2 */
   SDRAM:   org = 02000000h   len = 1000000h	/* 16 Mbytes */

   /* Internal */
   IREG:    org = 01800000h   len = 0200000h /* 2 Mbytes */
   IDATA:   org = 80000000h   len = 0010000h /* 64 Kbytes */
}

SECTIONS
{
    .vectors	>  	IVECS
    .text		>	IPROG
    .stack		>	IDATA
    .tables  	>  	IDATA
    .data    	>  	IDATA
    .bss		>  	IDATA
    .sysmem	>	IDATA
    .cinit		> 	IDATA
    .const		>	IDATA
    .cio     	>  	IDATA
    .far		>	IDATA
}
