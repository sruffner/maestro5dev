echo OFF
REM Batch file for building XYDAKAR.OUT, the version of XYCORE supporting the embedded C44 processor on the Dakar F5 
REM Carrier Board from Spectrum Signal Processing.
REM Uses TI C3x/4x build tools (available via DOS command path) running from the MS-DOS command prompt. 
REM The following directory structure is expected (!!ensure DOS-compatible pathnames!!):
REM   ../dakar/      ==> build directory for XYDAKAR.OUT:
REM      ../dakar/mkdakar.bat    ==> This file.
REM      ../dakar/xydakar.cmd    ==> The linker command file for building XYDAKAR.OUT.
REM      ../dakar/include        ==> Required include files from the Dakar F5 development package
REM      ../dakar/lib            ==> Required libraries from the Dakar F5 development package
REM      ../dakar/intermed       ==> Intermediate files created by build process are placed here.
REM 
REM   ../xycore.c    ==> the primary source code file 
REM 
REM Revision history:
REM 17apr2000-- Created.  Decided to dispense with the use of a make utility.  It was giving me problems with the way 
REM I've set up the build environment.  Note that all required files are maintained on a UNIX file system to ensure 
REM automated backup, while DOTS_DAK.OUT is built by C3x/4x tools running on a PC.  This batch file performs the 
REM sequence of commands required to build the program.
REM 11oct2002-- Changed names of files as we incorporated XYCORE build environment into the new CXDRIVER.
REM 25jul2003-- Set and unset C_DIR env variable for locating C3x/4x include files.
REM

set C_DIR=c:\c3xtools
echo MKDAKAR.BAT: Building XYDAKAR.OUT....
echo MKDAKAR.BAT: Compiling source files...
cl30 -c -qgks -v40 -D_TGTDAKARF5 -frintermed -fsintermed -ftintermed -iinclude -i.. -c ..\xycore.c 
echo MKDAKAR.BAT: Linking...
lnk30 xydakar.cmd
echo MKDAKAR.BAT: Done!
set C_DIR=

echo ON
