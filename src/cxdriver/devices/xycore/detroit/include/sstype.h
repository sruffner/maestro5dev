#ifndef SSTYPE_H
#define SSTYPE_H

/* SDOC------------------------------------------------------------------------
 * COPYRIGHT (c) 1997, Spectrum Signal Processing Inc., All Rights Reserved
 *
 * $Workfile:: sstype.h                                                       $
 *
 * Description
 *
 *
 * Contents :  sstype controls the inclusion of hardware dependent header files
 *             that define hardware independent types that are used to increase
 *             compatibility between one machine and the other and portability
 *             of code.
 * 
 *             sstype.h           shell outer file
 *             type_x86.h         for x86 architectures
 *             type_c3x.h         for the TMS320C3x/4x
 *             etc.h              for the etc platform
 * 
 *             These files are templates only.
 *             Modify for specific projects only
 *             IF NECCESSARY, ACTIONS ON THESE FILES AFFECT MANY PROJECTS.
 *             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * 
 * Notes    :  We are SUGGESTING the following protocols
 * 
 *             1) Base types
 *                TYPExx has at least XX bits.
 *                For example, if you need an integer that counts from 0 to 
 *                100 you should use INT16 (guaranteed to have 16 bits).
 *                INT16 may in fact have 32 (or more) bits, so never rely on 
 *                the fact that TYPExx is limited to XX bits
 *                (it has at least XX bits).  The type will always reflect the
 *                most efficient type for that processor. For example, INT8 on
 *                a C3X is a 32 bit type (because the C3X processes
 *                32 bit words more efficiently than 8 bit words).
 * 
 *             2) Base type modifiers
 * 
 *                U = unsigned
 * 
 *                These prefixes should occur in the same order as
 *                the c declarations.
 * 
 *                   ie. UINT16 --> unsigned short
 *                   "unsigned short"
 * 
 *             3) Types and macros should be common across platform.h files.
 *                Otherwise, it will not be possible to port code from one
 *                platform to another without a lot of grief.
 * 
 *             4) Known C Preprocessor Predefined Macros.
 *                Only compiler defined macros and identifiers may be prexied 
 *                with '_' or '__' Spectrum Signal Processing global macros 
 *                must be prefixed with 'SS_'.
 * 
 *                Texas Instruments
 *
 *                  _TMS320C30 - TI floating point DSP        type_c3x.h
 *                  _TMS320C40 - TI floating point DSP        type_c3x.h
 *                  _TMS320C50 - TI fixed point DSP
 * 
 *               Sun Microsystems (sparcworks compiler)
 *
 *                  a list of predefinitions from Sun ANSI C Compiler-Specific
 *                  Information from C 4.0 User's Guide
 *               
 *                  __STDC__ provides ANSI C conformance
 *                  __sun
 *                  __unix
 *                  __SUNPRO_C=0x400
 *                  __'uname -s'_'uname -r' (example __SunOS_5_4
 *                  __sparc 
 *                  __i386
 *                  __BUILTIN_VA_ARG_INCR (Solaris 2.x)
 *                  __SVR4 (Solaris 2.x)                
 *                  __hp9000s700 (HP-UX)
 *                  __hp9000s800 (HP-UX)
 *                  __hppa (HP-UX)
 *                  __hpux (HP-UX)
 *                  _PA_RISC1_1 (HP-UX)
 *                  _PHUX_SOURCE(HP-UX)
 *                  __LITTLE_ENDIAN (PowerPC)
 *                  __ppc (PowerPC)
 *                  
 *                  in the -Xa and -Xt modes only:
 *                  
 *                  __RESTRICT
 *   
 *              Hewlett-Packard
 *
 *                  __unix
 *                  __hp9000s700 (HP-UX)
 *                  __hp9000s800 (HP-UX)
 *                  __hppa (HP-UX)
 *                  __hpux (HP-UX)
 *                  _PA_RISC1_1 (HP-UX)
 *
 *              Microsoft
 *
 *                  _DOS       - MSVC MS-DOS application      type_x86.h
 *                  _CONSOLE   - MSVC console applications
 *                  _WINDOWS
 *                  WIN32   
 *
 *               GNU
 *
 *                  unix
 *                  BSD
 *                  sun
 *                  
 *            5) Spectrum Defined Conditional Compilation Macros
 *               All Spectrum defined global macros must be prefixed with 
 *               'SS_'. The macro should be defined in the project makefile 
 *               compiler options macro; example: CCOPTIONS= -DSS_MACRO
 *
 * $Modtime:: 5/25/98 10:49p                                                  $
 * $Revision:: 53                                                             $
 * $Archive:: /MAC/de62_154.v10/dev/shared/include/sstype.h                   $
 *
 *-------------------------------------------------------------------------EDOC
 */

/* the macros used to control the #includes below should only be compiler
 * defined where possible, i.e. don't #define MYPLATFORM instead, find out
 * what macro the compiler defines for this purpose. If the compiler does
 * not provide such a macro, see list item 5 in the above file header block.
 */

#if (defined(_TMS320C30) || defined(_TMS320C40)|| defined(_TMS320C4x)|| defined(_TMS320C44)|| defined(_4x)) /* TI C40 floating point DSP*/
#  include "type_c3x.h"
#elif (defined(_TMS320C6X))                      /* TMS320C6x DSPs           */
#  include "type_c6x.h"
#elif (defined(_WINDOWS) || defined(WIN32)) || defined(_DOS) /* MSVC Win95   */
#  include "type_w95.h"
#elif (defined(_SS_QWIN) || defined (_SS_DOS))   /* DOS/QWIN                 */
#  include "type_dos.h"
#elif (defined(__2106x__))                       /* ADSP-2106x SHARC DSP     */
#  include "type_shc.h"
#elif (defined(_TMS320C50))                      /* TI C50 fixed point DSP   */
#  include "type_c5x.h"                          /* CREATE THIS FILE !!      */
#elif (defined(__hpux))                          /* Hewlett-Packard UNIX     */
#  include "type_hpu.h"                          
#elif defined(__SVR4)                            /* Solaris                  */
#  include "type_sol.h"
#elif defined(SS_VXW)                            /* VxWorks                  */
#  include "type_vxw.h"
#else
#  error type_xxx.h not supported                /* generate compiler error  */
#endif /* OS switches */

/* common simple cross platform macro definitions =============================
 */
#ifndef NO_FLAGS
#  define NO_FLAGS 0
#endif

/* Define Error Information ================================================ */
#if (((defined(WIN32) || defined(_WINDOWS)) || (defined(__SVR4) || defined (_HPUX))) || defined(_DOS))
typedef struct tagSS_ERROR
{                         
      RESULT rv;
      UINT32 Line;
      STRING File;
      STRING Msg;
      STRING256 CustomMsg;
} S_SS_ERROR;

#define HOST_ERROR_HANDLING
#define DEFINE_ERROR_STRUCT   S_SS_ERROR SS_Error = {0, 0, 0, 0, 0}

extern S_SS_ERROR SS_Error;

#define LOG_ERROR(Err, Mesg)  {  if (SS_Error.rv == 0x0)          \
                                 {  /* No error recorded yet */   \
                                    SS_Error.rv = Err;            \
                                    SS_Error.Line = __LINE__;     \
                                    SS_Error.File = __FILE__;     \
                                    SS_Error.Msg = Mesg;          \
                                  }}

#elif defined(SS_VXW)
/* Nothing has to be done because of new error handling structure */
 
#else /* DSP */
#define DSP_ERROR_HANDLING  
#  if 0
   typedef struct tagSS_ERROR
   {                         
      RESULT rv;
      UINT32 Line;
      STRING File;
      STRING Msg;
      STRING256 CustomMsg;
   } S_SS_ERROR;

#  define DEFINE_ERROR_STRUCT   S_SS_ERROR SS_Error = {0,0,0/*,0,0*/}
#  endif
#endif /* Error struct info for HOST/DSP */

#define MAX_ERROR_MESSAGE 		128
#define MAX_ERR_MESSAGE_LENGTH	MAX_ERROR_MESSAGE
/* End of Error Information ================================================ */

#endif /* SSTYPE_H */




