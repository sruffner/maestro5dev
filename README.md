# Maestro

<mark>NOTE: I am no longer actively developing **Maestro/RMVideo**. I have made this repo available for anyone in the
neuroscience community that continues to use the application and might wish to fork the repo to adapt or modify the
program for their own purposes.</mark>

**Maestro** is a Windows® application developed in Stephen G. Lisberger's laboratory at Duke University to conduct a 
wide variety of behavioral and neurophysiological experiments focusing on smooth pursuit eye movements, the vestibular 
ocular reflex (VOR), and certain other aspects of the visual system. It provides real-time data acquisition and stimulus 
control to meet the particular research needs of the laboratory, and it is specifically tailored to the experimental 
apparatus employed there.

**Maestro** experiments record behavioral and neuronal responses to visual stimuli. Eye movement and neurophysiological
data are recorded at 1KHz. Some experimental protocols require the ability to adapt predefined target trajectories 
during runtime in accordance with the subject’s behavioral response. The potential complexity of experiments that 
animate multiple targets simultaneously, along with the need to adjust target trajectories during runtime, place a 
premium on efficient communications with the relevant hardware and demand sub-millisecond responsiveness.

**Maestro** was developed to meet these operational demands. Over the years it has evolved from a distributed program 
with components hosted on a UNIX workstation and two DOS PCs to an all-in-one Windows NT/2000/XP application running on 
a single machine. In its latest form, the application is once again distributed in nature, consisting of three 
functional modules. The user defines, modifies, and executes experimental protocols via a graphical user interface 
(GUI). The GUI module, acting as the “master”, sends protocol information and issues a variety of commands to the 
experiment control and hardware interface. The hardware controller is the time-intensive component of **Maestro** and
does most of the heavy lifting: communication with PC peripheral devices, controlling frame-by-frame animation of 
visual targets during a trial, recording and saving data streams, and providing feedback to the GUI in the form of 
status messages, data traces, and current eye and target positions. The controller is spawned by the GUI module at 
startup and runs as a "real-time", kernel-level process within IntervalZero's RealTime Extension (RTX) to 
Windows. The two modules run on a Windows workstation and communicate over interprocess shared memory. A third 
functional module, the OpenGL application **RMVideo**, drives the primary visual stimulus display -- a large, 
high-performance CRT monitor. This module runs on a separate Linux workstation and communicates with the **Maestro** 
hardware controller over a private, dedicated Ethernet link.


## User Guide
An [online user's guide](https://sites.google.com/a/srscicomp.com/maestro/home) provides a thorough introduction to the 
program, detailed information about system requirements and supported hardware, usage instructions, a complete version 
history, and a downloads page where you can get the latest release. The online guide also covers RMVideo, which is 
Maestro's workhorse visual stimulus platform (running on a separate Linux workstation); JMWork, a Java application that
reads and edits Maestro data files; and several Maestro-related Matlab utilities.

## Installation
Installing **Maestro** itself is relatively easy using the self-extracting installer that is available for 
[download](https://sites.google.com/a/srscicomp.com/maestro/downloads) from the online guide. But there's a lot more
to it than installing software to get a Maestro experiment rig up and running with RMVideo as the stimulus platform.
See the [Installation chapter](https://sites.google.com/a/srscicomp.com/maestro/installation) in the online guide for 
full details on deploying **Maestro** to a Windows10/RTX64 workstation and **RMVideo** to a Linux 14.x or 18.x machine.

## License
**Maestro**, **RMVideo**, and the related Matlab tools were created by [Scott Ruffner](mailto:sruffner@srscicomp.com). All of the software is
free to use under the terms of the MIT license.

## Credits
**Maestro** was developed with funding provided by the Stephen G. Lisberger laboratory in the Department of Neurobiology
at Duke University. It evolved from Dr. Lisberger's UNIX-based **Cntrlx** application.

## Repository Contents
- `maestro4.x`: Root folder containing all source code and resource files needed to build **Maestro** 4.x.
  - `cxdriver`: Source code for building `cxdriver.rtss`, the hardware controller for **Maestro** that runs a separate 
  RTX64 process.
  - `gui`: Source code for building `maestro.exe`, the **Maestro** GUI in which user defines and runs experiments.
  - `installer`: Files needed to build the self-extracting installer for **Maestro**.
  - `maestroExp`: Sample **Maestro** experiment documents (used for testing).
  - `visual_studio` : Project templates for building `maestro.exe` and `cxdriver.rtss` in Visual Studio 2017.
- `rmvideo`: Root folder containing all source code and other files needed to build **RMVideo**.
  - `forUbuntu14.04`: For building **RMVideo** to run on Lubuntu 14.04. Includes a `Makefile`. The `versions` folder 
  contains "readmes" for the most recent and past versions, indicating **Maestro** version compatibility. See [online 
  guide for more information](https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo) on 
  building the `rmvideo` executable and configuring the Linux workstation appropriately.
  - `forUbuntu18.04`: For building **RMVideo** to run on Lubuntu 18.04. This version has been tested successfully
  but is not in use in any labs, AFAIK.
  - `doc`: Contains some sample xorg.conf files and `msimcmds.txt`, a file containing a simulated **Maestro** command
  sequence. It is used to test **RMVideo** without connecting to **Maestro**.
  - `media_bup`: Contains some sample image and video files used to test **RMVideo**.
- `utilties_for_matlab`:
  - `miscellaneous`, `rmvplaid`: Matlab scripts implementing a number of **Maestro/RMVideo**-related tools. All of these
  are [described](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/other-functions)
  in the online guide, but none are actively used AFAIK.
  - `plexon`: Matlab files implementing the `plexmon()` Matlab application, which monitors the event stream of the Plexon
  Multiacquisition Processor (MAP) and passes timestamp data to a 'trial handler' function. See the plexmon.m file for
  more details. The Plexon MAP has been superceded by the Omniplex system, and AFAIK `plexmon()` is no longer used.
  - `readcxdata`: C source code files for building the Matlab MEX functions 
  [_readcxdata()_](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/readcxdata) and
  [_editcxdata()_](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/editcxdata). 
  The former is used to load a **Maestro** data file's contents into Matlab for further analysis, while the latter can
  be used to clear or edit analysis actions that are appended to the data file without altering the original recorded
  data. See the `releases` directory for readmes on previous versions (the MEX functions were updated each time the
  Maestro data file format changed), as well as instructions for building the MEX functions.


