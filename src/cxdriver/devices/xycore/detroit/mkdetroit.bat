echo OFF
REM Batch file for building XYDETROIT.OUT, the version of XYCORE supporting the Detroit C6x from Spectrum Sig Proc.
REM Uses TI C6x build tools (available via DOS command path) running from the MS-DOS command prompt. The following 
REM directory structure is expected (!!ensure DOS-compatible pathnames!!):
REM   ../detroit/    ==> build directory for XYDETROIT.OUT:
REM      ../detroit/mkdetroit.bat ==> This file.
REM      ../detroit/isfp6201.c    ==> Interrupt vector table for the Detroit C6x.  Must be compiled with program in 
REM                                   order to provide an entry point for the program.  DON'T MODIFY!
REM      ../detroit/xydetroit.cmd ==> The linker command file for building XYDETROIT.OUT.
REM      ../detroit/include       ==> Required include files from the Detroit C6x development package
REM      ../detroit/lib           ==> Required libraries from the Detroit C6x development package
REM      ../detroit/intermed      ==> Intermediate files created by build process are placed here.
REM 
REM   ../xycore.c    ==> the primary source code file 
REM
REM Revision history:
REM 12apr2000-- Created.  Decided to dispense with the use of a make utility.  It was giving me problems with the way 
REM I've set up the build environment.  Note that all required files are maintained on a UNIX file system to ensure 
REM automated backup, while DOTS_DET.OUT is built by C6x tools running on a PC.  This batch file performs the sequence 
REM of commands required to build the program.
REM 06mar2001-- Changed dots_det.cmd to use a stack size of 0x400 bytes instead of 0x100.  Was getting stack overflow 
REM fault with introduction of the OPTICFLOW target in XYCORE.
REM 11oct2002-- Changed names of files as we incorporated XYCORE build environment into the new CXDRIVER.
REM 25jul2003-- Set and unset C6X_C_DIR env variable for locating C6x include files.
REM

set C6X_C_DIR=c:\c6xtools\include
echo MKDETROIT.BAT: Building XYDETROIT.OUT....
echo MKDETROIT.BAT: Compiling source files...
cl6x -al -c -eo.o6x -gkqs -D_TGTDETROIT -frintermed -fsintermed -ftintermed -iinclude -i.. -c isfp6201.c 
cl6x -al -c -eo.o6x -gkqs -D_TGTDETROIT -frintermed -fsintermed -ftintermed -iinclude -i.. -c ..\xycore.c 
echo MKDETROIT.BAT: Linking...
lnk6x xydetroit.cmd
echo MKDETROIT.BAT: Done!
set C6X_C_DIR=
echo ON
