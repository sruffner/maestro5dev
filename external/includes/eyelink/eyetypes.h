/**
 * Copyright (c) 1996-2023, SR Research Ltd., All Rights Reserved
 *
 * For use by SR Research licencees only. Redistribution and use in source
 * and binary forms, with or without modification, are NOT permitted.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * Neither name of SR Research Ltd nor the name of contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**********************************************************************************
 * EYELINK PORTABLE EXPT SUPPORT                                                  *
 * Header file for standard functions                                             *
 *                                                                                *
 *                                                                                *
 ******************************************* WARNING ******************************
 *                                                                                *
 * UNDER NO CIRCUMSTANCES SHOULD PARTS OF THESE FILES BE COPIED OR COMBINED.      *
 * This will make your code impossible to upgrade to new releases in the future,  *
 * and SR Research will not give tech support for reorganized code.               *
 *                                                                                *
 * This file should not be modified. If you must modify it, copy the entire file  *
 * with a new name, and change the the new file.                                  *
 *                                                                                *
 **********************************************************************************/

/*!
    \file eyetypes.h
    \brief Declarations of basic data types.
 */

#ifndef __SRRESEARCH__EYETYPES_H__
#define __SRRESEARCH__EYETYPES_H__

/*#define FARTYPE _far */  /* unusual--for some mixed-model builds */
#define FARTYPE            /* make blank for most DOS, 32-bit, ANSI C */

#ifdef WIN32
  #include <windows.h>   /* needed for prior declarations of types */
#endif

#ifdef __cplusplus     /* For C++ definitions */
extern "C" {
#endif

#ifndef BYTEDEF
    #define BYTEDEF 1
        typedef unsigned char  byte;
        typedef signed short   INT16;
        typedef unsigned short UINT16;
        /* VC++ 6.0 defines these types already. mingw32 also defines this*/
    #if !(defined(_BASETSD_H_) || defined (_BASETSD_H))
        typedef signed int    INT32;
        typedef unsigned int  UINT32;
    #endif
#endif

#ifndef MICRODEF            /* Special high-resolution time structure */
    #define MICRODEF 1
/*! @ingroup access_time_local
	Structure to hold msec and usec.
	\sa current_micro
 */
    typedef struct {
        INT32  msec;    /* SIGNED for offset computations */
        INT16  usec;
    } MICRO ;
#endif

#ifdef __cplusplus    /* For C++ definitions */
}
#endif
#endif
