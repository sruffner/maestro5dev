Readme: READCXDATA/EDITCXDATA release dated 16nov2016 for Maestro 3.3.1

16nov2016
Scott Ruffner

NOTE: This is the latest build of READ/EDITCXDATA for Maestro version 3.3.x.

Changes for this release:

This is a VERY MINOR maintenance release to recognize the RMVideo “Image” target introduced in Maestro v3.3.1. The data file version remains
at V=20. The change ensures that the “strFolder” and “strFile” fields are set appropriately for the RMV_IMAGE target. These fields merely
identify the source file (on the RMVideo host) for the displayed image.

If you do not need that information, then there is no need to update your current READCXDATA().

For build procedural notes, see ../makeReleaseProcedures.txt.
