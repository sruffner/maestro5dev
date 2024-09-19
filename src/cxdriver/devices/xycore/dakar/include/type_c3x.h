/* ============================================================================
Spectrum Signal Processing Inc. Copyright 1996

File     :  type_c3x.h

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



Author: RAB
$Date: 10/21/97 2:50p $
$Revision: 26 $

Notes    :

============================================================================ */

#ifndef  __TYPE_C3X_H
#define  __TYPE_C3X_H

/*#include <float.h>*/
/*#include <limits.h>*/

/*
  Inline used for switching between inlining and callable functions
      _INLINE is a compiler directive, and
      __INLINE is used in the actual function declarations

      ie. __INLINE UINT32 foo(FLOAT32 parameter);
*/
#ifdef   _INLINE
#define  __INLINE    static inline
#else
#define  __INLINE
#endif

/* who would do any different ?? */
#ifndef TRUE
#define  TRUE              1
#define  FALSE             0
#endif

/* used as a error code return value for functions */
typedef  unsigned long      STATUS;


/* void and pointer typedefs ----------------------------------------------- */

/* PVOID is a pointer to void */
typedef  void              *PVOID;

/* VPVOID is a volatile pointer to void */
typedef  volatile PVOID    VPVOID;


/* integer typedefs -------------------------------------------------------- */


/* INT8 types ------------------------------ */
typedef  char              INT8;

/* volatile char */
typedef  volatile INT8     VINT8;

/* A PINT32 is a 32 bit pointer to a char */
typedef  INT8              *PINT8;

/* PVUINT8 is a pointer to a volatile char */
typedef  VINT8             *PVINT8;

/* VPINT32 is a volatile pointer to a volatile char */
typedef  volatile PINT8    VPINT8;

/* PVPINT32 is a pointer to a volatile pointer to a volatile char */
typedef  VPINT8            *PVPINT8;


/* UINT8 types ----------------------------- */
typedef  unsigned char     UINT8;

/* volatile unsigned char */
typedef  volatile UINT8    VUINT8;

/* A PUINT8 is a 32-bit pointer to an unsigned char */
typedef  UINT8             *PUINT8;

/* PVUINT8 is a pointer to a volatile unsigned char*/
typedef  VUINT8            *PVUINT8;

/* VPUINT8 is a volatile pointer to a volatile unsigned char */
typedef  volatile PUINT8   VPUINT8;

/* PVPUINT8 is a pointer to a volatile pointer to volatile unsigned char */
typedef  VPUINT8           *PVPUINT8;


/* INT16 types ------------------------------ */
typedef  short             INT16;

/* volatile short */
typedef  volatile INT16    VINT16;

/* A PINT16 is a 32 bit pointer to a short */
typedef  INT16             *PINT16;

/* PVINT16 is a pointer to a volatile short */
typedef  VINT16            *PVINT16;

/* VPINT16 is a volatile pointer to a volatile short */
typedef  volatile PINT16   VPINT16;

/* PVPINT16 is a pointer to a volatile pointer to a short */
typedef  VPINT16           *PVPINT16;


/* UINT16 types ----------------------------- */
typedef  unsigned short    UINT16;

/* volatile unsigned short */
typedef  volatile UINT16   VUINT16;

/* A PUINT16 is a 32 bit pointer to an unsigned short */
typedef  UINT16            *PUINT16;

/* PVUINT16 is a pointer to a volatile unsigned short */
typedef  VUINT16           *PVUINT16;

/* VPUINT16 is a volatile pointer to a volatile unsigned short */
typedef  volatile PUINT16  VPUINT16;

/* PVPUINT16 is a pointer to a volatile pointer to volatile unsigned short */
typedef  VPUINT16          *PVPUINT16;


/* INT32 types ------------------------------ */
typedef  long              INT32;

/* volatile long */
typedef  volatile INT32    VINT32;

/* A PINT32 is a 32 bit pointer to a long */
typedef  INT32             *PINT32;

/* PVINT32 is a pointer to a volatile long */
typedef  VINT32            *PVINT32;

/* VPINT32 is a volatile pointer to a volatile long */
typedef  volatile PINT32   VPINT32;

/* PVPINT32 is a pointer to a volatile pointer to a long */
typedef  VPINT32           *PVPINT32;


/* UINT32 types ----------------------------- */
typedef  unsigned long     UINT32;

/* volatile unsigned long */
typedef  volatile UINT32   VUINT32;

/* A PUINT32 is a 32 bit pointer to an unsigned long */
typedef  UINT32            *PUINT32;

/* PVINT32 is a pointer to a volatile unsigned long */
typedef  VUINT32           *PVUINT32;

/* VPUINT32 is a volatile pointer to a volatile unsigned long */
typedef  volatile PUINT32  VPUINT32;

/* PVPUINT32 is a pointer to a volatile pointer to volatile unsigned long */
typedef  VPUINT32          *PVPUINT32;


/* floating point typedefs ------------------------------------------------- */

typedef  float             FLOAT32;

/* volatile unsigned float */
typedef  volatile FLOAT32  VFLOAT32;

/* A PFLOAT32 is a 32 bit pointer to an float */
typedef  FLOAT32           *PFLOAT32;

/* PVFLOAT32 is a volatile pointer to a volatile float */
typedef  VFLOAT32          *PVFLOAT32;

/* VPFLOAT32 is a volatile pointer to a volatile float */
typedef  volatile PFLOAT32  VPFLOAT32;

/* PVPFLOAT32 is a pointer to a volatile pointer to volatile float */
typedef  VPFLOAT32         *PVPFLOAT32;

/* string typedefs --------------------------------------------------------- */

/* Arbitrary Number */
#define  MAX_STR_LEN       256
typedef  char              STRING256[MAX_STR_LEN];
typedef  char              *STRING;


#define OK  0
/** commented out for compatibility with F5 ALIB/DSP **/
#if 0
typedef INT32  RESULT;
#endif


#endif /* __TYPE_C3X_H */

/*                          end of TYPE_C3X.H
---------------------------------------------------------------------------- */