Installation Notes for RMVideo Version 11 - **** Targeting Lubuntu 18.04 LTS ****

******************** TODO -- Need to update this prior to releasing Maestro 5.0.2 and RMVideo 11 ***************************
Last Updated: 19 October 2020 (saruffner)


- Prior releases of version 10 for Lubuntu 14.04 LTS:
  -— Initial release date: 23 May 2019 (rev 05 Jun), with Maestro v4.1.0. It is not compatible with earlier Maestro releases. 
  -- 10b release date: 09 Sep 2019, with Maestro v4.1.1. V10b is compatible with Maestro v4.1.0 as well. However, to get 
  improved handling of large image targets (eg, 2560x1440), recommend installing both Maestro v4.1.1 and RMVideo V10b.
  -- 10c release date: 04 Nov 2019, with Maestro V4.1.1. V10c is also compatible with Maestro v4.1.0, but same caveat as 
  with 10b applies.

- THIS RELEASE REQUIRES UBUNTU LINUX 18.04 LTS: Ubuntu 14.04 LTS reached end-of-life in April 2019, so I worked on code
changes needed to run RMVideo under a more recent OS version, 18.04 LTS (EOL in April 2023). Significant code changes were
necessary since Ubuntu has returned to using the FFMPEG video libraries rather than the LIBAV libraries in 14.04. In addition,
some other changes were introduced to further improve RMVideo performance under 18.04 LTS. 
   If you wish to install this release of RMVideo, the target workstation should use the latest 18.04.x release of "Lubuntu"
(Ubuntu with the Lightweight X11 Desktop Environment, LXDE). The Maestro online user guide provides detailed instructions on
configuring the workstation:

     https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo


- Changes: 
   --> Implemented new "flicker" feature available for all RMVideo target types. The flicker ON phase duration, OFF phase
duration, and initial delay (until the first ON phase) are specified in the target definition. All 3 flicker parameters are
given in # of video frame periods, range-restricted to 0..99. If ON or OFF duration is 0, the feature is disabled. A
flickering target will flicker whether it is moving or not. The flicker state resets each time the target is toggled ON.
   --> Adopted the previously developed implementation conforming to OpenGL 3.3 Core Profile. While testing suggested that
it does not perform quite as well as the original OpenGL 1.1 implementation, the differences are not substantial and are
only noticeable when animating 3+ fullscreen plaid/grating targets @ refresh rates of 120Hz or better. In fact, the slightly
poorer performance of the OGL3.3 implementation is due to a different implementation for gratings -- rather than using a
multi-texturing approach, the OGL3.3 implementation puts the grating calculations in the fragment shader, which is a more
intuitive solution. I may revert to the multi-texturing strategy in a later release, if necessary.
   --> REVISED 05 Jun: Fixed two bugs in the implementation of the Random-Dot Patch target: (1) Using an alpha mask texture
to implement a non-rectangular aperture led to different-colored dots along the aperture edge -- essentially these dots were
only partially masked (alpha neither 0 nor 1). Reverted to the original approach, which calculates per-dot alpha on every
frame. The per-dot alpha is delivered via the vertex attribute representing the dot's texel coordinate Tx. Had to make a
minor change to the fragment shader. (2) Fixed coding mistake in the computation of the colors for two-color contrast mode.
   --> RELEASE 10b, 10 Sep: To improve performance when presenting very large image targets (eg, 2560x1440), implemented an
in-memory image cache that is preloaded at startup with any existing images in the media store, up to a 300MB capacity.
   --> RELEASE 10c, 04 Nov: Implemented major design changes to dramatically boost RMVideo's movie playback performance: 
(1) a separate worker thread (running with a normal rather than a real-time scheduling policy) handles reading and 
buffering video frames from the source file; (2) CRMVTarget uses a round-robin queue of OpenGL "pixel buffer objects" to
allow for asynchronous upload of video frames to the texture object on the GPU, thereby increasing overall throughput.
Prior to the change, a trial presenting a 1024x768 video at a playback rate of 120Hz was successfully completed on only
33% of trial reps. After the change, a trial presenting a 1280x720 video @120Hz was successful on 100% of reps, while a
trial presenting a 1920x1080 move @ 120Hz was succssful on 96% of reps. NOTE that performance will depend on the capabilities
of the RMVideo workstation. The aforementioned results were on a 2016-era workstation with a 4-core 3+ GHz processor, 8GB RAM,
and an NVidia GeForce GTX 1060 6GB GPU.
   --> RELEASE 10d, 20 Oct 2020: This release includes all necessary code changes to build and run RMVideo in Lubuntu 18.04.1 LTS
(kernel 5.3.0-45-generic). Most of the code changes involved adapting RMVideo to use the FFMPEG video libraries instead of
LIBAV. THIS RELEASE IS NOT COMPATIBLE WITH Lubuntu 14.04 LTS.



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
