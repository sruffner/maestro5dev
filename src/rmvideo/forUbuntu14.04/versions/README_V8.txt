Installation Notes for RMVideo (Version 8 and higher)

Last Updated: 02 Oct 2018 (saruffner)


— Release date: Fall 2018, with Maestro v4.0.0. It is not compatible with earlier Maestro releases.

- Changes: Added support for the RMVideo "vertical sync flash" feature. This allows you to optionally flash a small spot
(black to white) in the top-left corner of the display starting with the first video frame marking the start of any segment
in a Maestro trial. The flash drives a photodiode assembly which, in turn, generates a TTL pulse that can then be time-
stamped by Maestro. This should help to have a more accurate measure of when the trial segment actually begins on the
display. For more information, go here:

     https://sites.google.com/a/srscicomp.com/maestro/operation/running-experiments/video-display

— Requires Ubuntu Linux 14.04 LTS: This build was compiled on an up-to-date workstation using a 3.19 kernel on a “Lubuntu”
distribution, 14.04.3 LTS (“Lubuntu” = Ubuntu with the Lightweight X11 Desktop Environment, LXDE). To use RMVideo 8 — and
Maestro v4.0.0 —, you must upgrade your RMVideo workstation to Lubuntu 14.04.5 LTS. The Maestro online user guide provides 
detailed instructions on configuring the workstation:

     https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo


— Code archives: The source code for RMVideo is made freely available in case you would need to rebuild it for your particular 
Linux set-up. You can find these source code archives on the “Downloads” page of the Maestro online user guide. The source code
for this release is in srcRMVideoV8.zip. Unzip it, cd to the directory containing the unzipped code files, and run make. 
There’s a Makefile in the archive. Be sure to build it under Lubuntu 14.04.5 LTS and use g++ v4.8.4. If any required libraries 
or include files are missing, you’ll get plenty of error messages! There are notes about building RMVideo in the online guide
at the link above.

— Basic installation: This assumes that the workstation has already been configured to run RMVideo. If you’re not a Linux sys
admin guru, I recommend getting help with this!

  (1) Download rmvideoV8.zip to the Linux workstation. Put it in /usr/local/rmvideo. The script that launches the program 
assumes that it is located here. Unzip the archive — it contains two key files: the ‘rmvideo’ executable and the ‘launchRMVideo’ 
script. If you’re updating an existing installation, be sure to remove the old files before unzipping.

RMVideo must run with root privileges in order to access certain software capabilities — particularly POSIX soft real-time
support. Therefore, you must use ‘chown’ to make ‘root’ the owner of both files. And run ‘chmod 4755’ on both files as well to
set the correct permissions bits.

To launch the program, make sure /usr/local/rmvideo is the current directory, then execute the ‘launchRMVideo’ script. That 
script contains a few commands that turn off screen blanking, display power management, and the OpenGL ‘SyncToVBlank’ feature
— all of which interfere with the proper operation of RMVideo. If you later terminate RMVideo via Ctrl-C, subsequent commands
in that script reenable the features. Always use ‘launchRMVideo’ rather than executing ‘rmvideo’ directly.

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
