// ~/maestro5dev/installer/readme.txt
//
// Last updated: 20sep2024.  saruffner.

This document describes the sequence of manual steps that must be taken to build the self-extracting installer, 
Maestro5Setup.exe, that will install Maestro 5.x on a Windows 11 computer.

This is not a sophisticated installer. It simply copies the necessary Maestro program files to a directory on the
host machine (the user can select a folder other than the default one) and adds the necessary registry key that
defines Maestro's home directory. It also provides an un-installer. The hope is that this will make it easier
for users to update Maestro whenever a new release comes out.

The installer is setup to update an existing 5.x installation -- you should not need to uninstall first. 
Also, an entry under the Maestro-specific registry key contains a REG_SZ value listing the three busy wait 
times (in microsecs) applicable when delivering a command to an external device in the DIO interface via DO<15..0>.
This was added to accommodate different DIO interface capabilities.

The installer should be built on a 64-bit Windows 11 machine. Maestro 5.x targets the Windows 11/RTX64 4.5 platform.
The previous major release, 4.x, targeted Windows 10/RTX64 3.4.

We rely on the InnoSetup 6.3.3 compiler from JR Software (https://jrsoftware.org) to build the installer. 


DIRECTORY CONTENTS:

$INST_HOME = C:\maestro5dev\installer
   This is the directory on the Windows 11 64-bit workstation in which the installer is actually built.

$INST_HOME\readme.txt : This file.

$INST_HOME\Maestro5Setup.iss : The InnoSetup script that is used to compile the self-extracting installer for Maestro 5.x. 
This script was the result of a lot of trial-and-error! Each time you build an installer for a new application version, 
the only thing you should have to change is the constant "MaestroVersionStr" in the [Code] Section; also update 
"AppCopyright" with each passing year!

$INST_HOME\Maestro5Setup.exe : The self-extracting installer constructed by running the InnoSetup compiler on the script 
file Maestro5Setup.iss. This needs to be packaged in a ZIP file so that it can be uploaded to the online user's guide.

$INST_HOME\files : All files required to build the self-extracting installer are located in this sub-directory.
   \maestroGUI.exe, cxdriver.rtss : The Maestro program files. Whenever a new version is built, replace these files with 
their newer versions.
   \eyelink_core64.dll : The DLL file for the EyeLink Plus Display SDK, v2.1.762. This is distributed with Maestro 5.x 
because it must be present in order for Maestro to run, even if the EyeLink tracker will not be used.
   \version.html : A simple HTML file that indicates the application version number and provides a link to the online guide. 
Be sure to update the version number in this file whenever a new version is released.
   \Maestro5Setup.ico : The icon for the self-extracting installer.
   \drivers : This folder contains the unsigned INF and CAT file for the PCIe-6363, the DAQ device used by Maestro 5.x. 
The files are required in order to transfer the PCIe-6363 to RTX64 control. See the readme.txt file in this folder.


STEPS REQUIRED TO BUILD INSTALLER FOR A NEW VERSION:

1) Each new release of Maestro 5.x will typically involve changes to both maestroGUI.exe and cxdriver.rtss. Copy the new
versions of these program files to $INST_HOME\files. No need to copy files that haven't changed! 

2) Update the application version number in $INST_HOME\files\version.html.

3) Double-click on $INST_HOME\Maestro5Setup.iss to start the InnoSetup Compiler. Update the "MaestroVersionStr" constant in
the [Code] section and, if needed, the "AppCopyright" parameter in the [Setup] section. Build the installer executable via 
Build->Compile. This will create the installer at $INST_HOME\Maestro5Setup.exe.

4) Compress Maestro5Setup.exe to ZIP file maestroV5xx.zip and upload that to the Downloads page in Maestro's online user
guide. No need to archive the source code, which is now maintained in the maestro5dev GitHub repository.
