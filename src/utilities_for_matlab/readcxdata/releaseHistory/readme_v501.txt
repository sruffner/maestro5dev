Readme: READCXDATA/EDITCXDATA release v501

05nov2024
Scott Ruffner

Changes for this release:

Modified to handle new Trial-mode special feature "selDurByFix", introduced in Maestro 5.0.1.

   The "selDurByFix" feature works like "selByFix", except that target selection during the
   special segment S also determines the duration of the subsequent segment S+1. A new trial
   code, SEGDURS, specifies the min duration D0 and max duration D1 for segment S+1 (it is delivered
   only for that segment, and only if the "selDurByFix" special operation is enabled). If Fix1
   is selected, then segment S+1 lasts D0 milliseconds; if Fix2 is selected, it lasts D1 milliseconds.

   However, the original trial codes (as stored in the data file) are prepared assuming that Fix2
   gets selected and the duration of segment S+1 is D1. If Fix1 is selected instead, readcxdata()
   corrects the calculated trial length and the start times of segment S+2 onward by subtracting
   the difference D1-D0.

 

