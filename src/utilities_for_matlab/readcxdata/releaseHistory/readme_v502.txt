Readme: READCXDATA/EDITCXDATA release v502

09dec2024
Scott Ruffner

Changes/updates to support Maestro V5.0.2:

Updated to accept file version 25.

Removed output fields 'psgm' and 'stimulusrun'. The PSGM has been removed entirely from Maestro
as of V5.0.2. Readcxdata() never populated the 'stimulusrun' field -- a "TODO" that was never 
addressed because Continuous-mode stimulus runs were a rarely used feature. As of Maestro 5.0.2,
the only usable target in a stimulus run is the Chair target, and that may no longer be supported
in any experiment rigs still in operation.

Updated to handle increase in max number of trial targets, from 25 to 50.

Updated to handle change in data file format: Stereo dot disparity parameter RMVTGTDEF.fDotDisp
added.

 

