# Changelog

**Maestro** development began in the early 2000s, and the original 1.0.0 release for Windows XP arrived in Mar 2003.
**Maestro** 2.0.0 (Jun 2006) dropped support for the old VSG2/4 framebuffer video and introduced **RMVideo**, an OpenGL 
application running on a separate Linux workstation and driven by **Maestro** via commands send over a private 
point-to-point Ethernet link. **Maestro** 2.5 (Sep 2009) added support for "movie" targets in **RMVideo**; by this time,
**RMVideo** was fast becoming the primary stimulus target platform as the hardware for the old fiber optics and XY 
digital oscilloscope targets were obsolesced. New features have been introduced periodically since then, and the program
was migrated to a newer versions of the Windows operating system and IntervalZero's Real Time Extension (RTX): 
**Maestro** 3.0 (Sep 2012) for Windows 7 32-bit and RTX2011, and **Maestro** 4.0 (Nov 2018) for Windows 10 64-bit and 
RTX64 3.4.

The most recent **Maestro** release is v4.2.1 in Apr 2023, paired with **RMVideo** v10c.

This file documents changes in the code base for **Maestro**, **RMVideo**, and related Matlab utilities since it was 
moved to a GitHub repository in June 2024. A complete version history is 
[available](https://sites.google.com/a/srscicomp.com/maestro/current-status/version-history) on the **Maestro** website.

## Initial (24 Jun 2024)
- Established local and remote repository that collects the code files and other resources for building **Maestro**, 
**RMVideo**, and several **Maestro**-related _Matlab_ utilities.
