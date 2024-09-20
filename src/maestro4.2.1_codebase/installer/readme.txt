// ~/maestro4.x/installer/readme.txt
//
// Last updated: 20sep2024.  saruffner.

This document describes the sequence of manual steps that must be taken to build the self-extracting installer, 
Maestro4Setup.exe, that will install Maestro 4.x on a Windows 10 computer.

This is not a sophisticated installer. It simply copies the necessary Maestro program files to a directory on the
host machine (the user can select a folder other than the default one) and adds the necessary registry key that
defines Maestro's home directory. It also provides an un-installer. The hope is that this will make it easier
for users to update Maestro whenever a new release comes out.

As of Sep 2019, the installer is setup to update an existing installation -- you no longer need to uninstall first. 
Also, a new entry under the Maestro-specific registry key contains a REG_SZ value listing the three busy wait 
times (in microsecs) applicable when delivering a command to an external device in the DIO interface via DO<15..0>.
This was added to accommodate different DIO interface capabilities.

The installer must be built on a 64-bit Windows 10 machine.


DIRECTORY CONTENTS:

$INST_HOME = C:\MaestroInstaller
   This is the directory on the Windows 10 64-bit workstation in which the installer is actually built.

$INST_HOME\readme.txt : This file.

$INST_HOME\MaestroSetup.iss : The InnoSetup script that is used to compile the self-extracting installer for Maestro 4.x. 
This script was the result of a lot of trial-and-error! Each time you build an installer for a new application version, 
the only thing you should have to change is the constant "MaestroVersionStr" in the [Code] Section; also update 
"AppCopyright" with each passing year!

$INST_HOME\Maestro4Setup.exe : The self-extracting installer constructed by running the InnoSetup compiler on the script 
file Maestro4Setup.iss. This needs to be compressed to ZIP file so that it can be uploaded to the online user's guide.

$INST_HOME\files : All files required to build the self-extracting installer are located in this sub-directory.
   \maestroGUI.exe, cxdriver.rtss : The Maestro program files. Whenever a new version is built, replace these files with 
their newer versions.
   \eyelink_core64.dll : The DLL file for the EyeLink Plus Display SDK. This is distributed with Maestro 4.x because it 
must be present in order for Maestro to run, even if the EyeLink tracker will not be used.
   \version.html : A simple HTML file that indicates the application version number and provides a link to the online guide. 
Be sure to update the version number in this file whenever a new version is released.
   \Maestro4Setup.ico : The icon for the self-extracting installer.
   \drivers : This folder contains the unsigned INF and CAT file for the PCIe-6363, the DAQ device used by Maestro 4.x. 
The files are required in order to transfer the PCIe-6363 to RTX64 control. See the readme.txt file in this folder.


STEPS REQUIRED TO BUILD INSTALLER FOR A NEW VERSION:

1) Each new release of Maestro 4.x will typically involve changes to both maestroGUI.exe and cxdriver.rtss. Copy the new
versions of these program files to $INST_HOME\files. No need to copy files that haven't changed! 

2) Update the application version number in $INST_HOME\files\version.html.

3) Double-click on $INST_HOME\MaestroSetup.iss to start the InnoSetup Compiler. Update the "MaestroVersionStr" constant in
the [Code] section and, if needed, the "AppCopyright" parameter in the [Setup] section. Build the installer executable via 
Build->Compile. This will create the installer at $INST_HOME\Maestro4Setup.exe.

4) Compress Maestro4Setup.exe to ZIP file maestroV4xx.zip. Also compress the Maestro source code directories to 
srcV4xx.zip and put that file in the same folder. These ZIP files should be uploaded to the Downloads page in Maestro's 
online user guide.
