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

## 18-20 Nov 2024: Dropped support for the PSGM
- The Pulse Stimulus Generator Module (PSGM), designed back in 2007, was another "latched device" controlled by Maestro's
16 digital outputs. It was intended to drive a Grass stimulator to deliver electrical pulses to the brain during a trial
or Continuous-mode "stimulus run". However, after initial testing, the PSGM project was abandoned and has been a vestigial 
part of Maestro for many years.
- With the PSGM (and XYseq) gone, the only remaining "stimulus channel" type in a stimulus run is "Chair". Given that no 
labs are using the animal chair AFAIK, I also dropped support for writing stimulus run definitions in the Maestro data file.
Note that `readcxdata()` and **JMWork** have never supported digesting these "stimulus run records" anyway. Very minor data 
file format change; data file version incremented to 25.
- The **Trial Editor** now only contains two tab pages above the segment table: the **Main** tab is unchanged, while the
**Perturbations/RVs** tab houses two grids displaying the trial's perturbations and random variables. The PSGM-related
widgets, formerly on the same tab with the perturbations list, have been removed.
- Existing experiment documents (`.cxe`) are migrated silently. PSGM parameters stored in a trial object are simply read
in and discarded; similarly for stimulus run channels that used the channel type "PSGM".
- Matlab utility `readcxdata()` updated to accept data files with version 25. In addition, since the PSGM was never really 
used and since `readcxdata()` never reported stimulus run definitions in its output (an unfixed bug), the output fields 
`stimulusrun` and `psgm` have been removed. The utility simply skips over stimulus run records (if any) and the trial 
code group `PSGM_TC` that contains PSGM parameters.

## 13-14 Nov 2024: Cosmetic changes.
- On the **Random Variables** tab of the **Trial Editor**, the parameter labels in the grid's header row are updated
IAW the type of RV in the current focus row. This works better than the original solution, which briefly displayed the
parameter label in a tooltip whenever the user clicked on a cell containing a parameter.
- Using bold font for fixed header row and/or fixed header column in the trial segment table, the perturbations table,
and the random variable table on the **Trial Editor**. Adjusted column widths in perturbations table.
- Using bold font for fixed header row and/or fixed header column in the **Channel Configuration Editor**.
- For the grid of perturbation objects in the **Perturbation Editor**, the parameter labels in the grid's header row are
updated IAW the type of perturbation in the current focus row. As with the other grids, the fixed header row and column
are now displayed in a bold font.
- BUG fix: When a perturbation object was deleted in the object tree, the grid in the **Perturbation Editor** was not
updated correctly to reflect that deletion.
- On the **Trial Editor** ("Main" tab page), the special operation in effect is set by a combo box/dropdown control rather
than a pushbutton. 
- BUG fix: When mousewheeling over the random variables table in the **Trial Editor** ("Random Variables" tab page), both
the RV table and the entire editor form would scroll, which was annoying.

## 12 Nov 2024: Mods to support new fields in JMX document introduced in `maestrodoc()` v1.2.2
- As of v1.2.2, the Matlab utility `maestrodoc()` supports defining and using random variables in a trial. Two new fields
were added to the JSON object `T` defining a Maestro trial: `T.rvs` contains the random variable defines, while `T.rvuse`
applies the RVs to segment table parameters. `JMXDocImporter` was updated to handle the new fields.
- `Maestrodoc()` v1.2.2 also removed all XYScope-related content; as a result, the JSON object defining a Maestro
target no longer has the `isxy` field. `JMXDocImporter::ImportTargetSets()` updated to treat that field as optional so 
that Maestro can properly import both pre-v1.2.2 and post-v1.2.2 JMX documents.

## 06 Nov 2024: Release 5.0.1 - New special feature "selDurByFix".
- A saccade-triggered special operation in a trial. Same requirements and same behavior as "selByFix", with one
addition: Like the end-of-trial reward, the duration of the segment S immediately after the "special segment" depends on 
which fixation target is selected during the special segment. If Fix1 is selected, then the specified minimum duration
of S is used; if Fix2 is selected, then the max duration of S applies.
- Loosened restrictions on the trial segment definition to allow both the minimum and maximum segment duration be set
independently to either a constant or a random variable. That way the minimum and maximum duration of the segment 
following the special segment in a "selDurByFix" trial can be randomly chosen from a specified probability distribution.
Users should take care to define their trial and RVs so that min segment duration is always <= max. Maestro will abort trial
sequencing otherwise.
- Updated READCXDATA to handle new trial code SEGDURS that is specific to the "selDurByFix" feature.
- Tweaked CLiteGrid implementation so that inplace combo box control is sized to accommodate the longest string in the
choice list to be displayed in the combo's dropdown.
- Increased the column width in the trial segment table to 60 pix.
- In JMX experiment docs generated by the Matlab utility `maestrodoc()` (v1.2.1 or later), the trial parameter `specialop` is
set to `selectDur` to enable the selDurByFix feature. `JMXDocImporter` was updated to handle the new option.
- Released Maestro 5.0.1.

## 31 Oct 2024: An alternate implementation of CCxEventTimer, the DIO event timer interface.
- The current event timer interface uses DO<15..0> to communicate with a number of latched external devices that are part
of a relatively complex DIO interface panel in the experiment rig. If we dropped support for the PSGM device (designed but
never built) and use software-timed pulses on dedicated DO channels for the variable length reward pulse and audio tone pulse,
it is possible to get rid of that complex and expensive-to-build interface panel and just use 24 static DO channels on the
PCIe-6363 to implement all of the remaining event timer functionality.
- CCxEventTimerAlt is a proposed redesign of the event timer interface. IT IS NOT CURRENTLY INTEGRATED INTO MAESTRO, as it
is not compatible with current Maestro experiment rigs. However, it could be integrated in the future -- but keep in mind
that changes would be needed in CNI6363_DIO to conform to the specifications of CCxEventTimerAlt.

## 23 Oct 2024: Initial release of Maestro 5.0.0.
- Adjusted layout of widgets on the stimulus run editor form (`IDD_RUNFORM`). Also increased the (fixed) width of columns in the 
stimulus channel grid, as some of the labels and values were getting cut off.
- Previously, on switching from another op mode to IdleMode, the mode control panel was hidden -- which was kind of annoying 
since users will typically want to keep the panel visible. Now, the control panel's visibility in IdleMode is toggled only if 
it was already in IdleMode; upon transition to IdleMode, it stays up.

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
