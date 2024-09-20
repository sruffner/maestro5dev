Readme: READCXDATA/EDITCXDATA release dated 09sep2013 for Maestro 3.0.3

09sep2013
Scott Ruffner

Changes for this release:

This release increases the number of possible sorted-spike train channels that can be stored in the Maestro data 
file from 13 to 50. This was primarily done because users are starting to record simultaneously from multiple
units on each of several electrodes using the Plexon system -- so it is possible to "see" more than 13 units in
one experiment. And users would like to keep the unit data in the same physical file as the other experiment data.

Build notes:
1) Used Matlab 2012b to build Linux 64-bit READ/EDITCXDATA.MEXA64. Matlab 2011b issued a warning that MEX compiler 
was using an unsupported version of GCC (4.4.3); Matlab2011b MEX only tested for GCC up to version 4.3.2.
2) Used Matlab R2013a to build Intel Mac 64-bit READ/EDITCXDATA.MEXMACI64.
3) I rely on the Lisberger lab to compile it for Win 64. No one is using 32-bit platforms at this point, as far as I know.
I did not build any 32-bit MEX files for this release.


