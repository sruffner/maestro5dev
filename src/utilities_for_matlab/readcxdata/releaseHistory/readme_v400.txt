Readme: READCXDATA/EDITCXDATA release dated 12jun2018, rev 01oct2018

01oct2018
Scott Ruffner

Changes for this release:

Updated IAW data file format changes in Maestro 4.0.0, which migrates Maestro to the 64-bit Win10/RTX64
OS platform. Data file version = 21. Also, two fields were added to the data file header: setName[]
holds the name of the trial set, while subsetName[] holds the name of the trial subset (if any).
These fields will be empty strings in Continuous mode files, or in any file generated prior to 
Maestro 4.0.0.

Revised 01oct2018: Added two new fields to data file header to report the parameters governing the
RMVideo "vertical sync flash" feature: rmvSyncSz is the spot size in millimeters, while rmvSyncDur is the
flash duration in # of video frames. Currently the feature can only be employed in Trial mode. These
fields will both be 0 in Continuous mode files, or in any file generated prior to Maestro 4.0.0.

For build procedural notes, see ../makeReleaseProcedures.txt.
