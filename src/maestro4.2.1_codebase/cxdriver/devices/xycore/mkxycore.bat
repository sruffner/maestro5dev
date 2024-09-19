echo OFF
REM MS-DOS Batch file for building versions of XYCORE tailored to each of the following Spectrum Signal Processing 
REM DSP boards:
REM      XYDETROIT.OUT	==>   Detroit C6x (single-processor)
REM      XYDAKAR.OUT   	==>   Dakar F5 Carrier Board (embedded C44 node only)
REM 
REM This batch file performs the following sequence for each build:  (1) Remove all intermediate files from previous 
REM builds.  (2) Run each build's batch file found in the build directory to rebuild the corresponding *.OUT.  The 
REM batch file should place the new *.OUT in the current build dir.  We then move it to ..\xycore\*.OUT.
REM 
REM The following directory structure is expected (!!ensure DOS-compatible pathnames!!):
REM   ..\xycore\detroit          ==> build directory for XYDETROIT.OUT
REM   ..\xycore\dakar            ==> build directory for XYDAKAR.OUT
REM   <build>\intermed           ==> location of intermediate files in each build directory
REM   <build>\mk***.bat        	 ==> batch file for building each target, where *** = 'detroit' or 'dakar'

echo MKXYCORE.BAT: Removing intermediate files for all targets.... 
del *.out
cd detroit\intermed
del *.*
cd ..\..
cd dakar\intermed
del *.*
cd ..\..
echo MKXYCORE.BAT: Building all targets...
cd detroit
call mkdetroit
echo OFF
move xydetroit.out ..\xydetroit.out
cd ..\dakar
call mkdakar
echo OFF
move xydakar.out ..\xydakar.out
cd ..
echo MKXYCORE.BAT: Done!

echo ON
