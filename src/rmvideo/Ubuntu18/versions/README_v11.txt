Installation Notes for RMVideo Version 11 - **** Targeting Lubuntu 18.04 LTS ****

Last Updated: 30 December 2024 (saruffner)
----------------------------------------------------------------

Changes:
==> Implemented "stereo mode" to support stereo dot disparity experiments in Priebe lab. Requires NVidia card that supports
and is configured for stereo. RMVideo will attempt to enable stereo mode at startup. If not available, it will use the
default visual (double-buffered, non-stereo RGBA). IF STEREO IS ENABLED, keep in mind that the "stereo frame rate" is one-half
the monitor's actual frame rate.
==> A new parameter was added to the RMVideo target definition structure: RMVTGTDEF.fDotDisp, the stereo disparity in visual
degrees. RMVideo was updated to receive this new parameter from Maestro during target loading, and to use it to differentially
draw the left and right buffers when stereo is enabled. The disparity parameter applies only to the dot target types "Point",
"Random-Dot Patch", and "Random-Dot Flowfield".


- THIS RELEASE REQUIRES UBUNTU LINUX 18.04 LTS: Ubuntu 14.04 LTS reached end-of-life in April 2019, so I worked on code
changes needed to run RMVideo under a more recent OS version, 18.04 LTS. Significant code changes were
necessary since Ubuntu has returned to using the FFMPEG video libraries rather than the LIBAV libraries in 14.04. In addition,
some other changes were introduced to further improve RMVideo performance under 18.04 LTS. 
   If you wish to install this release of RMVideo, the target workstation should use the latest 18.04.x release of "Lubuntu"
(Ubuntu with the Lightweight X11 Desktop Environment, LXDE). The Maestro online user guide provides detailed instructions on
configuring the workstation:

     https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo

Ubuntu 18.04 LTS reached EOL in April 2023. Unfortunately, RMVideo has not been built/tested on more recent Ubuntu
distributions.


— Code archives: The source code for RMVideo is made freely available in case you would need to rebuild it for your particular 
Linux set-up. You can find these source code archives on the “Downloads” page of the Maestro online user guide. The source code
for this release is in srcRMVideoV**.zip. Unzip it, cd to the directory containing the unzipped code files, and run make. 
There’s a Makefile in the archive. This particular release of RMVideo was built under Lubuntu 18.04 LTS, 5.3 kernel, using
gcc v7.5.0. If any required libraries or include files are missing, you’ll get plenty of error messages! There are notes about
building RMVideo in the online guide at the link above.

— Basic installation: This assumes that the workstation has already been configured to run RMVideo. If you’re not a Linux sys
admin guru, I recommend getting help with this!

  (1) Download rmvideoV**.zip to the Linux workstation. Put it in /usr/local/rmvideo. The script that launches the program 
assumes that it is located here. Unzip the archive — it contains two key files: the ‘rmvideo’ executable and the ‘launchRMVideo’ 
script. If you’re updating an existing installation, be sure to remove the old files before unzipping.

NOTE: As of the 10d release for Lubuntu 18.04 LTS, RMVideo no longer requires root privileges to run.

  (2) To launch the program, make sure /usr/local/rmvideo is the current directory, then execute the ‘launchRMVideo’ script. That 
script contains a few commands that turn off both screen blanking and display power management, which interfere with the proper 
operation of RMVideo. If you later terminate RMVideo via Ctrl-C, subsequent commands in that script re-enable the features. 
Always use ‘launchRMVideo’ rather than executing ‘rmvideo’ directly.

Starting with release V9, RMVideo requires that vertical synchronization be enabled on the video card. The 'launchRMVideo'
script also includes a command to enable 'SyncToVBlank'; in past versions, this was disabled. The script does not bother to 
disable 'SyncToVBlank' when RMVideo terminates, since that feature is typically left on.


FOR MORE INFORMATION, consult the Maestro online user guide at:  https://sites.google.com/a/srscicomp.com/maestro/
