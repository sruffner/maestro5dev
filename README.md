# Maestro

<mark>I have recently ported Maestro to run under Windows 11 Professional (update 23H2) and RTX64 4.5. The new version
has been released as Maestro 5.0.0. Releases 5.0.1 and 5.0.2 implemented a number of requested features; these new
features will not be backported to Maestro 4.x. The source code for the last 4.x release, Maestro 4.2.1, is available at
`src/maestro4.2.1_codebase`.</mark>

<mark>NOTE: As of Jan 2025, I will no longer be actively developing **Maestro/RMVideo**. I have made this repo available for 
anyone in the neuroscience community that continues to use the application and might wish to fork the repo to adapt or 
modify the program for their own purposes.</mark>

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
history, and a downloads page where you can get the latest release. The online guide also covers **RMVideo**, which is 
**Maestro**'s workhorse visual stimulus platform (running on a separate Linux workstation); **JMWork**, a Java application
that reads and edits **Maestro** data files; and several **Maestro**-related utilities for _Matlab_.

## Installation
Installing **Maestro** itself is relatively easy using the self-extracting installer that is available for 
[download](https://sites.google.com/a/srscicomp.com/maestro/downloads) from the online guide. But there's a lot more
to it than installing software to get a **Maestro** experiment rig up and running with **RMVideo** as the stimulus platform.
See the [Installation chapter](https://sites.google.com/a/srscicomp.com/maestro/installation) in the online guide for 
full details on deploying **Maestro** to a Windows10/RTX64 workstation and **RMVideo** to a Linux 14.x or 18.x machine. 

## License
**Maestro**, **RMVideo**, and the related _Matlab_ tools were created by [Scott Ruffner](mailto:sruffner@srscicomp.com). 
All of the software is free to use under the terms of the MIT license.

## Credits
**Maestro** was developed with funding provided by the Stephen G. Lisberger laboratory in the Department of Neurobiology
at Duke University. It evolved from Dr. Lisberger's UNIX-based **Cntrlx** application.

## Repository Contents
- `maestro5dev.sln` : The VS 2022 "solution" file for coding and building **Maestro** version 5.x.
- `cxdriver`, `maestroGUI`: The **maestro5dev** solution contains two projects, one for developing/building `cxdriver.rtss` and
one for `maestro.exe`. The VS 2022 project files for these two projects are located in these folders. NOTE that the project
files contain absolute paths, which are based on the fact that the solution folder (also the root folder for the local repo) is
located at `C:\maestro5dev`.
- `external/lib` : Contains two files required for **Maestro**'s connection to the _Eyelink_ system from 
[SR Research](https://www.sr-research.com/). These were extracted from the latest version of the _Eyelink_ SDK, supporting
Windows 10/11:
  - `eyelink_core64.dll`: The DLL that must be included in the Maestro 5 installer.
  - `eyelink_core64.lib`: The corresponding library that is required to build `maestro.exe`.
- `external/includes/eyelink`: _Eyelink_-related header files required to build `maestro.exe`.
- `external/includes/rmvideo/rmvideo_common.h` : A header file required to build both `maestro.exe` and **RMVideo**. It 
defines structures and constants that implement the communication interface between **Maestro** and **RMVideo**.
- `installer`: Files needed to build the self-extracting installer for **Maestro** version 5.x.
- `sample_experiments`: Sample **Maestro** experiment documents (used for testing).
- `src`: The root source folder, containing all source code, resource and other supporting files:
  - `cxdriver`: Source code for building `cxdriver.rtss`, the hardware controller for **Maestro** that runs as a separate 
  RTX64 process. 
  - `gui`: Source code for building `maestro.exe`, the **Maestro** GUI in which user defines and runs experiments.
  - `rmvideo`: Root folder containing all source code and other files needed to build **RMVideo**.
    - `Ubuntu14`: For building **RMVideo** to run on Lubuntu 14.04. Includes a `Makefile`. The `versions` folder 
    contains "readmes" for the most recent and past versions, indicating **Maestro** version compatibility. See [online 
    guide for more information](https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-rmvideo) on 
    building the `rmvideo` executable and configuring the Linux workstation appropriately.
    - `Ubuntu18`: For building **RMVideo** to run on Lubuntu 18.04. This version has been tested successfully
    but is not in use in any labs, AFAIK.
    - `doc`: Contains some sample xorg.conf files and `msimcmds.txt`, a file containing a simulated **Maestro** command
    sequence. It is used to test **RMVideo** without connecting to **Maestro**.
    - `media_bup`: Contains some sample image and video files used to test **RMVideo**.
  - `maestro4.2.1_codebase`: For reference, this contains the state of the **Maestro** codebase for Version 4.2.1 (supporting
  Win10/RTX64 3.4), before I began porting the code to Windows 11/RTX64 4.5. In addition to the `cxdriver` and `gui` source 
  folders, it includes:
    - `installer`: Files needed to build the self-extracting installer for **Maestro 4.x**.
    - `visual_studio` : Project files for building `maestro.exe` and `cxdriver.rtss` for **Maestro 4.x** in Visual Studio 2017.
  - `utilties_for_matlab`:
    - `miscellaneous`, `rmvplaid`: _Matlab_ scripts implementing a number of **Maestro/RMVideo**-related tools. All of these
    are [described](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/other-functions)
    in the online guide, but none are actively used AFAIK.
    - `plexon`: _Matlab_ files implementing the `plexmon()` application, which monitors the event stream of the Plexon
    Multiacquisition Processor (MAP) and passes timestamp data to a 'trial handler' function. See the `plexmon.m` file for
    more details. The Plexon MAP has been superceded by the Omniplex system, and AFAIK `plexmon()` is no longer used.
    - `readcxdata`: C source code files for building the _Matlab_ MEX functions 
    [_readcxdata()_](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/readcxdata) and
    [_editcxdata()_](https://sites.google.com/a/srscicomp.com/maestro/data-analysis/supported-matlab-tools/editcxdata). 
    The former is used to load a **Maestro** data file's contents into _Matlab_ for further analysis, while the latter can
    be used to clear or edit analysis actions that are appended to the data file without altering the original recorded
    data. See the `releases` directory for readmes on previous versions (the MEX functions were updated each time the
    **Maestro** data file format changed), as well as instructions for building the MEX functions.


