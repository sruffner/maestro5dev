Readme: READCXDATA/EDITCXDATA release v410b

07jun2021
Scott Ruffner

Changes for this release:

Modified to support 200 distinct "sorted spike train" channels instead of 50. 

   Sorted spike train records are appended to the original Maestro data file by analysis software
   like JMWork and EDITCXDATA. Maestro itself does not write these records to the file. Their purpose
   is to allow the researcher to keep neural response data (spike trains) in the same file as the
   corresponding behavioral response data and the stimulus description (trial). The Joshua lab
   requested increasing the number of available channels to accommodate the multi-unit recordings
   made possible with the Plexon/Omniplex Neural Data Acquisition System.

   Since Sep 2013, JMWork and EDIT/READCXDATA have supported 50 spike sort channels, assigning record
   IDs (byte 0 of the record) 8..57 for channels 0..49. To increase this four-fold, we keep the same
   50 record IDs but also set a "bank ID" in [0..3]. The bank ID is stored in byte 1 of the record; 
   prior to this change, that byte was always set to 0. With record ID N=8..57 and bank M=0..3, the
   spike sort channel number C = M*50 + N-8, which ranges from 0..199.
 

