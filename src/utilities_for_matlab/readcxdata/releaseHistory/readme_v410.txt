Readme: READCXDATA/EDITCXDATA release dated 23 May 2019

23may2019
Scott Ruffner

Changes for this release:

Updated IAW data file format changes in Maestro 4.1.0, data file version = 23:

1) Modified RMVideo target definition to include 3 new int parameters controlling target "flicker": 
RMVTGTDEF.iFlickerOn, iFlickerOff, and iFlickerDelay. These are expressed in units of RMVideo frame
periods, range-limited to 0..99. If iFlickerOn=0 or iFlickerOff=0, target flickering is disabled.

This change affected the format of the target definition record in the Maestro data file, so READCXDATA
was updated to handle that format change and to report the values of the target flicker parameters in 
out.tgtdefns(:).params.

2) Introduced per-trial random reward withholding variable ratio feature. This does not affect the
data file format. However, if a reward is withheld for a given trial, the header field iRewLen1 (or
iRewLen2 for the second reward pulse) will be 0 instead of the pulse length specified in the trial's
definition. Also the REWARDLEN trial code stored in the file will also have a zero pulse length.

For build procedural notes, see ../makeReleaseProcedures.txt.
