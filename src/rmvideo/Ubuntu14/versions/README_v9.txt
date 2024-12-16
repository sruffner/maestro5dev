Installation Notes for RMVideo (Version 9 and higher)

Last Updated: 26 Mar 2019 (saruffner)


— Release date: Spring 2019, with Maestro v4.0.5. It is not compatible with earlier Maestro releases.

- Changes: This release includes a number of significant technical changes introduced while I was developing an alternative
implementation that required OpenGL 3.3 Core Profile, using vertex and fragment shaders for all rendering -- in lieu of the
immediate mode functionality of OGL 1.1. 
   --> Fixed bug in CElapsedTime implementation that resulted in a slight underestimate of the RMVideo monitor's refresh period.
   --> New implementation of the runtime loop in animate mode: (1) Vertical Sync (VSync) is now enabled; launchRMVideo script
modified accordingly. (2) After rendering the next frame on the backbuffer, call glXSwapBuffers() and then glFinish(). The
glFinish() call returns when the driver detects the vertical blanking interval and performs the swap. This eliminates any
"tearing artifact" that was possible with the old implementation. (3) Use CElapsedTime to get the elapsed time T since the start
of the first display frame. Detect a duplicate frame(s) whenever T-N*P ~= P or greater, where N is the # of video frames elapsed
and P is the measured refresh period.
   --> Updated IAW changes in the Maestro-RMVideo communication protocol while in the animate state. Now RMVideo sends a
message whenever a duplicate frame or frames is detected, and it also sends an elapsed frame count once per second. Maestro
checks for a message every time it sends a frame update command to RMVideo. It uses the elapsed frame count message to verify
that it is not "getting ahead" of RMVideo, which would mean that frame update commands could stack up in RMVideo's network
receive buffer (this, in fact, happened in previous versions when displaying an RMVideo target for a long time in Continuous
mode; because the refresh period was slightly underestimated, Maestro was sending frame updates too frequently).

NOTE that version 9 still uses OpenGL 1.1 "immediate mode" functions. Testing demonstrated that the OGL3.3-based implementation
exhibited more frequent duplicate frame events than the OGL 1.1 implementation when driving the display at 2560x1440 @ 144Hz.


— Requires Ubuntu Linux 14.04 LTS: This build was compiled on an up-to-date workstation using a 3.19 kernel on a “Lubuntu”
distribution, 14.04.3 LTS (“Lubuntu” = Ubuntu with the Lightweight X11 Desktop Environment, LXDE). To use RMVideo 9 — and
Maestro v4.0.5 —, you must upgrade your RMVideo workstation to Lubuntu 14.04.3 LTS. The Maestro online user guide provides 
detailed instructions on configuring the workstation:

     https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo


— Code archives: The source code for RMVideo is made freely available in case you would need to rebuild it for your particular 
Linux set-up. You can find these source code archives on the “Downloads” page of the Maestro online user guide. The source code
for this release is in srcRMVideoV9.zip. Unzip it, cd to the directory containing the unzipped code files, and run make. 
There’s a Makefile in the archive. Be sure to build it under Lubuntu 14.04.3 LTS and use g++ v4.8.4. If any required libraries 
or include files are missing, you’ll get plenty of error messages! There are notes about building RMVideo in the online guide
at the link above.

— Basic installation: This assumes that the workstation has already been configured to run RMVideo. If you’re not a Linux sys
admin guru, I recommend getting help with this!

  (1) Download rmvideoV9.zip to the Linux workstation. Put it in /usr/local/rmvideo. The script that launches the program 
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
