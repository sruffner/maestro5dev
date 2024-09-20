Readme: READCXDATA/EDITCXDATA release dated 01dec2015 for Maestro 3.2.0

01dec2015
Scott Ruffner

NOTE: This is the latest build of READ/EDITCXDATA for Maestro version 3.2.x.

Changes for this release:

This release handles Maestro data file version 20, introduced in Maestro v3.2.0, which will be released in the near future. Maestro 3.2.0
adds support for using the EyeLink 1000+ eye tracker to monitor eye position as an alternative to the eye coil system. The v20 data file
includes some EyeLink-specific header information, and the “other” events record (tag = 3) also includes “blink start” and “blink end” 
events that Maestro detects while monitoring the EyeLink’s eye position data stream. The EyeLink eye position data itself is stored in the
analog data records (tag = 0) in the usual manner, in channels HGPOS,VEPOS,HEVEL,VEVEL (effectively, the EyeLink data replaces the analog
samples from the eye coil system).

For build procedural notes, see ../makeReleaseProcedures.txt.
