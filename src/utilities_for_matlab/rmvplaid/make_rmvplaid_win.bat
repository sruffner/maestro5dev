@echo off
rem MAKE_RMVPLAID_WIN
rem
rem Batch file to build MEX function rmvplaid() as WinXP DLL
rem and put it in the public area for general use.
rem
rem This file is specifically set up to run on my development
rem PC, using the current installation of 7.x.
rem If installation paths change, or if my usual mapped network
rem drives change, this file must be updated accordingly!
rem
rem 06may2009: Introduced on this date.
rem
cd f:\projects\matlabstuff\rmvplaid
call "c:\Program Files\MATLAB\R2007a\bin\win32\mex" rmvplaid.c
move /Y g:\matlab_public\7.0\rmvplaid.mexw32 g:\matlab_public\7.0\previous\
move /Y rmvplaid.mexw32 g:\matlab_public\7.0\
