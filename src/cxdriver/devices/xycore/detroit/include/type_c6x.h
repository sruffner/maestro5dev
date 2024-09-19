/* ============================================================================
Spectrum Signal Processing Inc. Copyright 1996

File     :  type_c6x.h

Contents :  sstype contains hardware independent types that
            are used to guarantee compatibility between one
            machine and the other and portability.

            sstype.h           shell outer file
            type_x86.h         for x86 architectures
            type_c3x.h         for the TMS320C3x/4x
            etc.

            Base type modifiers

               U = unsigned
               R = register
               P = pointer
               V = volatile

               These prefixes should occur in the same order as
               the c declarations.

                  ie. PVPRUINT16 --> (volatile (register unsigned short *) *)
                  "pointer to a volatile pointer to a register unsigned short"

               Volatiles generally always apply to everything
               dereferenced from the base type.

               ie.   PVPVUINT16 == PVPUINT16



$Author: Grant $
$Date: 5/31/98 4:07p $
$Revision: 5 $

Notes    :

============================================================================ */

#ifndef TYPE_C6X_H
#define TYPE_C6X_H

#ifndef TRUE
#  define  TRUE              1
#  define  FALSE             0
#endif

/* integer typedefs -------------------------------------------------------- */
/* INT8 types ------------------------------ */
typedef  char              INT8;
typedef  char              BOOLEAN;

/* UINT8 types ----------------------------- */
typedef  unsigned char     UINT8;

/* INT16 types ------------------------------ */
typedef  short             INT16;

/* UINT16 types ----------------------------- */
typedef  unsigned short    UINT16;

/* INT32 types ------------------------------ */
typedef  int               INT32;
typedef INT32              RESULT;

/* UINT32 types ----------------------------- */
typedef  unsigned int      UINT32;

/* INT40 types ------------------------------ */
typedef  long              INT40;

/* UINT40 types ----------------------------- */
typedef  unsigned long     UINT40;

/* floating point typedefs ------------------------------------------------- */
typedef  float             FLOAT32;
typedef  double            FLOAT64;

/* string typedefs --------------------------------------------------------- */
#define  MAX_STR_LEN       256
typedef  char              STRING256[MAX_STR_LEN];
typedef  char              *STRING;

#define OK  0

#endif /* TYPE_C6X_H */

/* end of file */

