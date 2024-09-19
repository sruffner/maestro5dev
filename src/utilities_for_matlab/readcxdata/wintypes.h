//===================================================================================================================== 
//
// wintypes.h : Some typical MS Windows typedefs that are used in the readcxdata/editcxdata modules...
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(WINTYPES_H__INCLUDED_)
#define WINTYPES_H__INCLUDED_


typedef unsigned int    DWORD;
typedef unsigned int    UINT; 
typedef unsigned short  WORD;
typedef unsigned short  USHORT;
typedef unsigned char   BYTE;
typedef char            CHAR;
typedef short           SHORT;
typedef int             BOOL;
typedef int             INT;
typedef void            VOID;

#define     TRUE                 1 
#define     FALSE                0

#define MAKELONG(a, b) ((INT) (((WORD) (a)) | (((DWORD) ((WORD) (b))) << 16)) )

#endif   // !defined(WINTYPES_H__INCLUDED_)
