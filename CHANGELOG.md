# Changelog

**Maestro** development began in the early 2000s, and the original 1.0.0 release for Windows XP arrived in Mar 2003.
**Maestro** 2.0.0 (Jun 2006) dropped support for the old VSG2/4 framebuffer video and introduced **RMVideo**, an OpenGL 
application running on a separate Linux workstation and driven by **Maestro** via commands sent over a private 
point-to-point Ethernet link. **Maestro** 2.5 (Sep 2009) added support for "movie" targets in **RMVideo**; by this time,
**RMVideo** was fast becoming the primary stimulus target platform as the hardware for the old fiber optics and XY 
digital oscilloscope targets became obsolete. New features have been introduced periodically since then, and the program
was migrated to newer versions of the Windows operating system and IntervalZero's Real Time Extension (RTX): 
**Maestro** 3.0 (Sep 2012) for Windows 7 32-bit and RTX2011, and **Maestro** 4.0 (Nov 2018) for Windows 10 64-bit and 
RTX64 3.4.

The most recent **Maestro** release is v4.2.1 in Apr 2023, paired with **RMVideo** v10c (for Ubuntu 14.04) or v10d (for
Ubuntu 18.04). A complete version history is 
[available](https://sites.google.com/a/srscicomp.com/maestro/current-status/version-history) on the **Maestro** website.

In June 2024 I created a public GitHub repository to collect -- in one place -- all of the code and other supporting files 
for **Maestro**, **RMVideo**, and the various **Maestro**-related _Matlab_ utilities I have developed over the years in the
Lisberger laboratory. In Sep 2024 I began the process of porting Maestro to run on Windows 11 using the RTX64 4.5 SDK.

This file documents changes in the codebase since the repo was created in June 2024.

## 23 Oct 2024: Minor tweaks.
- Adjusted layout of widgets on the stimulus run editor form (`IDD_RUNFORM`). Also increased the (fixed) width of columns in the 
stimulus channel grid, as some of the labels and values were getting cut off.
- Previously, on switching from another op mode to IdleMode, the mode control panel was hidden -- which was kind of annoying 
since users will typically want to keep the panel visible. Now, the control panel's visibility in IdleMode is toggled only if 
it was already in IdleMode; upon transition to IdleMode, it stays up.
- Initial release of Maestro 5.0.0.

## 18 Oct 2024: Removed the XYScope platform from Maestro 5.0.
- The XYScope platform has been unsupported since **Maestro** 4 was released in Nov 2018. However, the XYScope platform was
still visible in the GUI. **Maestro** 5 drops the XYScope entirely.
- Removed all XYScope-related code and GUI elements from the `cxdriver` and `maestroGUI` projects. Certain XYScope-specific 
constants and data structures remain in the code (in particular, see `cxobj_ifc.h` and `cxfilefmt.h`) to support reading in 
pre-5.0 **Maestro** data files containing XYScope target definitions and so on. Also, we had to be careful with serialization and 
deserialization code in `cxdoc.*`, `cxtarget.*`, `cxtrial.*`, and `cxcontrun.*` to handle reading pre-5.0 **Maestro** experiment
documents containing XYScope-related information.
- Removed the **File|Import** command, `cximporter.*`, and all code that supported loading the old **cntrlxUnix**-style text files
that defined targets, trials, and the like. These have not been used for a very long time.
- Updated `jmxdocimporter.*` to ignore XYScope display parameters, XYScope target definitions, and any trials or stimulus runs
that employ those XYScope targets.
- Successfully rebuilt Maestro 5.0 and completed initial testing.

## 25 Sep 2024: Ported Maestro to Windows 11 and RTX64 4.5.
- Only a few code changes were needed to address deprecated or modified RTX64 SDK functions: Updated dwProtect parameter in
`RtCreateSharedMemory()`. Removed calls to `RtSleepFt/RtSleepFtEx()` in the `maestroGUI` project, as these are no longer supported
in Windows. Replaced deprecated `RtAllocate/FreeLockedMemory()` calls with `RtAllocate/FreeLocalMemory()`. After making these 
changes and fixing sundry warnings in VS2022, successfully built both `cxdriver.rtss` and `maestroGUI.exe`.
- Replaced the old unsigned security catalog file for the PCIe6363 (`ni6363_rtx64.cat`) with a properly signed CAT file 
provided by IntervalZero. This makes it much easier to transfer device ownership of the 6363 board from Windows to RTX64.
- Successfully installed and ran Maestro 5.0 without any issues.

## 19 Sep 2024: Maestro 5 development started
- Began the process of porting **Maestro** to run under Windows 11 and RTX64 4.5. Updated `cxdriver` and `maestroGUI` Visual
Studio projects for VS 2022, newer Windows SDK and platform toolset, and RTX64 4.5 SDK. Moved local git repo to the new Win11
machine, using VS 2022 to handle git operations. Everything that was in the original `maestro` repo is in this new
`maestro5dev` repo. The codebase for the final version of **Maestro 4.x** is located at `src/maestro4.2.1_codebase`. The
original `maestro` repo will be taken down.

## Initial (24 Jun 2024)
- Established local and remote repository that collects the code files and other resources for building **Maestro**, 
**RMVideo**, and several **Maestro**-related _Matlab_ utilities.
