Installation Notes for RMVideo (Version 11 and higher)

******************* TODO: Update this for Maestro 5.0.2/RMVideo 11 *******************
Last Updated: 30 December 2024 (saruffner)


Changes:
==> Implemented "stereo mode" to support stereo dot disparity experiments in Priebe lab. Requires NVidia card that supports
and is configured for stereo. RMVideo will attempt to enable stereo mode at startup. If not available, it will use the
default visual (double-buffered, non-stereo RGBA). IF STEREO IS ENABLED, keep in mind that the "stereo frame rate" is one-half
the monitor's actual frame rate.
==> A new parameter was added to the RMVideo target definition structure: RMVTGTDEF.fDotDisp, the stereo disparity in visual
degrees. RMVideo was updated to receive this new parameter from Maestro during target loading, and to use it to differentially
draw the left and right buffers when stereo is enabled. The disparity parameter applies only to the dot target types "Point",
"Random-Dot Patch", and "Random-Dot Flowfield".


— Requires Ubuntu Linux 14.04 LTS: This build was compiled on an up-to-date workstation using a 3.19 kernel on a “Lubuntu”
distribution, 14.04.3 LTS (“Lubuntu” = Ubuntu with the Lightweight X11 Desktop Environment, LXDE). For all RMVideo releases
since version 7, you must upgrade your RMVideo workstation to Lubuntu 14.04.3 LTS. The Maestro online user guide provides 
detailed instructions on configuring the workstation:

     https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo


— Code archives: The source code for RMVideo is made freely available in case you would need to rebuild it for your particular 
Linux set-up. You can find these source code archives on the “Downloads” page of the Maestro online user guide. The source code
for this release is in srcRMVideoV**.zip. Unzip it, cd to the directory containing the unzipped code files, and run make. 
There’s a Makefile in the archive. Be sure to build it under Lubuntu 14.04.3 LTS and use g++ v4.8.4. If any required libraries 
or include files are missing, you’ll get plenty of error messages! There are notes about building RMVideo in the online guide
at the link above.

— Basic installation: This assumes that the workstation has already been configured to run RMVideo. If you’re not a Linux sys
admin guru, I recommend getting help with this!

  (1) Download rmvideoV**.zip to the Linux workstation. Put it in /usr/local/rmvideo. The script that launches the program 
assumes that it is located here. Unzip the archive — it contains two key files: the ‘rmvideo’ executable and the ‘launchRMVideo’ 
script. If you’re updating an existing installation, be sure to remove the old files before unzipping.

RMVideo must run with root privileges in order to access certain software capabilities — particularly POSIX soft real-time
support. Therefore, you must use ‘chown’ to make ‘root’ the owner of both files. And run ‘chmod 4755’ on both files as well to
set the correct permissions bits.

To launch the program, make sure /usr/local/rmvideo is the current directory, then execute the ‘launchRMVideo’ script. That 
script contains a few commands that turn off both screen blanking and display power management, which interfere with the proper 
operation of RMVideo. If you later terminate RMVideo via Ctrl-C, subsequent commands in that script re-enable the features. 
Always use ‘launchRMVideo’ rather than executing ‘rmvideo’ directly.

Starting with release V9, RMVideo requires that vertical synchronization be enabled on the video card. The 'launchRMVideo'
script also includes a command to enable 'SyncToVBlank'; in past versions, this was disabled. The script does not bother to 
disable 'SyncToVBlank' when RMVideo terminates, since that feature is typically left on.

In addition to the files ‘rmvideo’ and ‘launchRMVideo’, the installation archive contains a doc/ subdirectory containing this
README and several other files:
   (1) msimcmds.txt - This text file contains an emulated Maestro command session for testing RMVideo without a connection to
       Maestro. If you launch RMVideo without the ‘connect’ argument, the program will look for this file in the current
       directory and attempt to execute the commands therein. End users will not need this unless they’re troubleshooting.
   (2) jellyroll_xorg.conf, xorg_gdmfw900.conf — These are sample Xorg configuration files that have been used successfully in
       past RMVideo workstations. They are from 2009-era setups, so they may likely be out of date. In fact, this and future
       builds, tests have indicated that RMVideo will work with the default xorg.conf file — no changes to the file are 
       necessary. Still, the xorg_gdmfw900.conf file may be worth a look, however, since it configures a custom modeline of 
       1024x768@100Hz for the Sony GDM-FW900 monitor that is not discovered otherwise.


FOR MORE INFORMATION, consult the Maestro online user guide at:  https://sites.google.com/a/srscicomp.com/maestro/
