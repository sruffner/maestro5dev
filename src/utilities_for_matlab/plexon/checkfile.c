//=====================================================================================================================
//
// checkfile.c -- Implementation of MATLAB MEX function checkfile().
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This function -- intended for the private use of PLEXMON -- checks that a file exists and makes sure that it can
// get exclusive access to that file.  It was written to enable PLEXMON to determine when a Maestro data file has been
// saved, so that PLEXMON can subsequently read its contents with MEX function readcxdata().  When Maestro files are
// saved over a mapped network drive, it can take a while for the file I/O to be completed!
//
// WINDOWS ONLY:  The function uses a Windows API function; so it may only be compiled for Windows.  This is not an
// issue, since PLEXMON is similarly restricted.
//
// USAGE:
// res = checkfile( 'filename' ), where...
//
//    filename    Pathname of file to look for.
//
//    res         Zero if could not find file or failed to get exclusive access to it.  Else nonzero.
//
// REVISION HISTORY:
// 28oct2005-- Began development.
//=====================================================================================================================

#include <stdio.h>
#include <windows.h>
#include "mex.h"


//=====================================================================================================================
// FUNCTIONS DEFINED IN THIS MODULE
//=====================================================================================================================



//=== mexFunction (checkfile) ========================================================================================
//
//    Entry point function for checkfile().
//
//    ARGS:       nlhs, plhs  -- [out] function output ("left-hand side"), which is simply a 1x1 matrix holding an
//                               error code: 0=failure, !0=success..
//                nrhs, prhs  -- [in] array input.  See file header for content.
//
//    RETURNS:    NONE.
//
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )
{
   HANDLE hFile;
   char strFilePath[1024];                                              // file pathname from 1st arg
   double* retCode;                                                     // ptr to return code in MATLAB matrix

   plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);                          // create 1x1 matrix for return code and init
   retCode = mxGetPr(plhs[0]);                                          // to failure indication
   *retCode = 0;

   if( nrhs != 1 || nlhs != 1 )                                         // check # of input/output args
   {
      printf( "checkfile: incorrect #args on rhs or lhs!\n" );
      return;
   }

   if( !mxIsChar(prhs[0]) )                                             // check right-hand side args
   {
      printf( "checkfile: bad arguments!\n" );
      return;
   }

   mxGetString( prhs[0], strFilePath, mxGetN(prhs[0])+1 );              // get file's pathname

   hFile = CreateFile( strFilePath, GENERIC_READ, 0, NULL,              // try to open existing file w/ exclusive 
             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );              // access
   if( hFile != INVALID_HANDLE_VALUE )                                  // success! gotta close file!
   {
      *retCode = 1;
      CloseHandle( hFile );
   }
   return;
}
