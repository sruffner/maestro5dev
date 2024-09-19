/**=====================================================================================================================
 ni6363types.h : Simple type definitions that explicitly indicate storage size. 
======================================================================================================================*/

#if !defined(NI6363TYPES_H__INCLUDED_)
#define NI6363TYPES_H__INCLUDED_

// These are primarily used for register access and manipulation. Type names explicitly reflect storage size in bits.
typedef signed char        i8;
typedef unsigned char      u8;
typedef signed short       i16;
typedef unsigned short     u16;
typedef signed int         i32;
typedef unsigned int       u32;
typedef float              f32;
typedef double             f64;

#endif   // !defined(NI6363TYPES_H__INCLUDED_)

