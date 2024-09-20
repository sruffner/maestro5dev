Readme: READCXDATA/EDITCXDATA release dated 13jul2011 for Maestro 2.7.0

14may2012
Scott Ruffner

(I'm writing this a bit after the fact as I have setup a different procedure for salting away and tracking the 
different code releases for READCXDATA/EDITCXDATA. All saved releases are kept in a release directory with the path
$CXDATAHOME/releases/vDDMMMYYYY, where $CXDATAHOME is the source code directory and DDMMMYYYY is the date of the
release. All required source code files -- representing the code state at the time of release -- are compressed into a 
single archive, READCXDATASRC.ZIP. A brief summary of the changes in the release, along with the current Maestro 
version at the time of release, is found in this file, README.TXT. Finally, the OS-specific MEX files for all supported
OS platforms are included in the release directory.)

13jun2018(sar): I’ve changed the naming scheme to reflect which version of Maestro is accompanied by this release of
READ/EDITCXDATA. Thus, the release directories are now $CXDATAHOME/releases/vNNN, where N is the 3-digit Maestro version;
similarly, the archive files are named cxdata_mex_vNNN.zip and cxdata_src_vNNN.zip. Occasionally, there may be multiple
READ/EDITCXDATA builds for the same Maestro version, in which case a letter (‘b’, ‘c’, etc) is appended to the ‘NNN’.

Changes for the 13jul2011 release:

The main reason for this release was to support emulation of XYScope noisy dots targets in Trial mode. In the process of
testing the changes, Joonyeol discovered a problem with the implementation of these noisy-dots targets in Maestro 2.6.5.
The implementation was corrected in Maestro v2.7.0 (data file v 18), so the emulation code was updated to support BOTH
the erroneous (for data files with v<18) and corrected (data file v>=18) implementations.

Only READCXDATA was modified in this release, as detailed below; no changes whatsoever to EDITCXDATA.

12apr2011-- Mods to support emulation of XYScope noisy-dots targets in Trial mode, for data file versions >= 12.
Added module XYNOISYEM.* to implement the emulation. Two new fields in output: "xynoisytimes" and "xynoisy". These 
will be empty if data file version < 12, if file was not recorded in Trial mode, or if no XYScope noisy-dots targets
participated in the trial.
16may2011-- Added support for new implementation of XYScope noisy-dots targets introduced in Maestro v2.7.0, data 
file version 18.  Added support for new "sliding-window average" feature during velocity stabilization of targets
in Trial mode. See processTrialCodes().
20may2011-- Changed all instances of mxCreateScalarDouble to mxCreateDoubleScalar. mxCreateScalarDouble was declared
obsolete in 2006, and code support is removed as of Matlab 2011a.
