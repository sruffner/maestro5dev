Readme: READCXDATA/EDITCXDATA release dated 13aug2014 for Maestro 3.1.1

13aug2014
Scott Ruffner

NOTE: This is the latest build of READ/EDITCXDATA for Maestro version 3.1.x.

Changes for this release:

This release addressed a couple issues with RMVideo target trajectory calculations. First, in processTrialCodes(), velocity 
was calculated by dividing position change in degrees by (dCurrTimeMS - dLastVideoUpdateMS) * 0.001. Because RMVideo frame
updates are never exact multiples of 1ms, this value tended to vary slightly from update to update. Now, I simply divide by
the (fixed) RMVideo frame period in seconds. Second, in updateRMVTgt() in noisyem.c, per-dot velocity is now computed WRT 
the screen frame of reference when the RMVideo random-dot patch target’s RMV_WRTSCREEN flag is set. Otherwise, per-dot velocity
is computed WRT target center.

Build notes:
1) Used Matlab 2012b on lennon.dhe.duke.edu to build Linux 64-bit READ/EDITCXDATA.MEXA64. Earlier versions no longer work, 
because they were designed to work with older versions of the Linux GCC compiler. To remote onto lennon, I used the VPN client to 
tunnel into Duke’s intranet. Then I could sftp onto lennon (port# 7777 !!), upload the modified code files to my matlabstuff/readcxdata 
folder on my account in the Lisberger lab. I then SSH’d onto lennon (again, port # 7777), cd’d to the directory, and compiled the
mexa64 files. I have to use the special mex options file for it to work. See readcxdata/makeReleaseProcedures.text. Finally, I used the 
SFTP connection to download the just-built mexa64 files onto my local machine.

2) Used Matlab R2013a to build Intel Mac 64-bit READ/EDITCXDATA.MEXMACI64. Had to add compiler def ‘-Dchar16_t=UINT16_t’ to the
MEX command line.

3) To build the Win64 versions, I tunneled into the Duke VPN and used the Microsoft Remote Desktop client to log into HAYDN.dhe.duke.edu
(set “PC Name” to its intranet IP address, 10.122.165.105). The MRD connection settings for HAYDN include a folder redirection so that I can copy the readcxdata()/editcxdata() code files from my iMac to HAYDN, then copy the .MEXW64 files back to my iMac. Built with Matlab
2012b.  NOTE that the build used the Microsoft SDK 7.1 compiler (which must come with Windows 7, or with the Matlab install), and 
the build message indicated that MS Visual Studio 2010 runtime libraries must be available on the computer on which the MEX function is run. 


