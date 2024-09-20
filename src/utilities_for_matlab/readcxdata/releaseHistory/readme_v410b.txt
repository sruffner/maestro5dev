Readme: READCXDATA/EDITCXDATA release v410b

11may2021
Scott Ruffner

Changes for this release:

Added per-segment fixation target designations to the READCXDATA output structure RES:

   res.trialInfo.fix1 -- A 1xN array, where N is the # of segments in the trial. Element n
   in the array is an integer index (zero-based) into the target list identifying the target designated
   as "Fixation Target #1" for segment n. In this case, -1 = No fixation target.

   res.trialInfo.fix2 -- Similarly for "Fixation Target #2" designations.

