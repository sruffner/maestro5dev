//===================================================================================================================== 
//
// editcxdata.c -- Implementation of MATLAB MEX function editcxdata().  A companion function to readcxdata(), this fcn
//                 can edit or augment the contents of "analysis" records in a Maestro or Cntrlx data file. It can also 
//                 be used to replace the "spike waveform" records in a Maestro file.
//
// AUTHOR:  saruffner 
//
// DESCRIPTION: 
// This MATLAB MEX function is the companion of readcxdata(), the MEX function which reads the contents of a Maestro or 
// Cntrlx data file into the MATLAB environment.
//
// USAGE:
// res = editcxdata( 'filename', data [, verbose, editSpikewave] ), where...
//
//    filename    Pathname of Maestro/Cntrlx data file to be edited.
//
//    data        A MATLAB structure array that must, at a minimum, have the following named fields.  Note that any 
//                of the first five fields may be a null matrix, indicating the absence of data.
//       mark1 : A 1xN double array that contains N marker #1 timepoints in milliseconds relative to the start of the 
//          recording.
//       mark2 : Analogously for marker #2 timepoints.
//       cut : A Nx3 double matrix that defines N cuts in recorded data.  Each row of the matrix is a triplet 
//          [startT endT ch#], where (startT, endT) indicate the time period over which the cut is made (in 
//          milliseconds) and ch# is the channel# of the affected data stream.
//       marks : A Nx2 double matrix that defines N mark segments.  Each row of the matrix [t0 t1] defines a mark 
//          segment starting at t0 and ending at t1 (in milliseconds).
//       sortedSpikes : A 1x200 MATLAB cell array containing spike sort data. Each cell of the array corresponds to
//          one of 200 different "spike sort" channels. A particular cell is either a null matrix -- meaning no spike
//          sort data on that channel -- or a 1xN double array of N spike arrival times in milliseconds.  Spike times
//          must be stored chronologically, in milliseconds relative to the start of recording, and be accurate to
//          10us. Note that the sequence of arrival times are converted into interspike intervals, which are saved as
//          32-bit integers in 10us-tick units. If this field is an empty/null matrix, then all sorted-spike train
//          data is removed from the file. [NOTE: The number of distinct spike-sort channels was increased from 13 
//          to 50 on 09sep2013, and again to 200 on 04jun2021.]
//       tags : A 1xN MATLAB structure array defining any tags to be attached to the data file's recorded timeline
//          via the ACTION_DEFTAG action code group. Each element of the array is a structure with fields 'time' (the
//          elapsed time in ms since recording began) and 'label' (1-16 visible ASCII characters). If any label is an
//          empty string, it is replaced by '!'. If the label is too long, it is truncated. Any illegal characters are
//          replaced by a '!'. This action code group was introduced in JMWork v1.4.0 and in READCXDATA/EDITCXDATA in 
//          Sep 2010.
//       discard : A scalar double. If nonzero, the ACTION_DISCARD action code is added to the file, explicitly marking
//          the file as "discarded", to be ignored by downstream analysis modules. This action code was introduced in
//          JMWork v1.4.0 and in READCXDATA/EDITCXDATA in Sep 2010 as an alternative to the "hacky" XWork- and MWork-
//          style discard marks.
//
//       spikewave : [Optional] A 1xN double array that contains the uncompressed spike waveform, assumed to be 
//          recorded at 25KHz. This field is exposed so that users can process the spike waveform in some way -- to 
//          remove artifacts for example -- prior to submitting it to a spike-sorting application. The spike waveform 
//          in this field is recompressed and packaged into the appropriate records in the data file, replacing any 
//          spike waveform records that were previously in the file. NOTE: EDITCXDATA does NOT let user change the 
//          length of the spike waveform, only the samples themselves. Spike waveform data is edited only if the 
//          following conditions are met: 1) the 'editSpikewave' guard flag is set; 2) this field is present, non-NULL, 
//          and not empty. If the conditions are met, but N != the number of samples in the spike waveform read from 
//          the file, the function fails entirely.
//
//    verbose     If nonzero, function prints detailed progress messages.
// 
//    editSpikewave This flag guards against inadvertently editing the spike waveform data in the file. It must be 
//                  explicitly set to a nonzero value, or no change is made to the spike waveform data.
//
// Note that the format of the fields in the MATLAB structure argument 'data' are exactly the same as like-named fields 
// in the output structure generated by readcxdata().  This is deliberate.  Typically, the user will first read in a 
// data file using 'A=readcxdata(fName)', then modify one or all of the above fields in the 'A' structure, and 
// finally call 'editcxdata(fName, A)' to modify the data file accordingly.
//
// REVISION HISTORY:
// 23jun2004-- Began development, using the source code from READCXDATA.* and an earlier incarnation, EDITMARKS.C, as 
//             a starting point. 
// 13jul2004-- Fixed some minor bugs in checkInput() and writeSortedSpikes().  Also fixed a more serious bug in the 
//             entry-pt function that caused editcxdata() to fail to transfer the original data records to the 
//             modified file!
// 02oct2004-- Added endian-conversion routines so that editcxdata() will run on big-endian (Mac) machines.  All 
//             Cntrlx/Maestro data files were created on little-endian machines.  Endianness is determined at runtime 
//             rather than using a defined constant at compile time.  We must swap endianness when we read in the 
//             action edit records, AND when we write them out -- the data files MUST remain little-endian!
// 27nov2006-- Modified to permit editing of the spike waveform in Maestro data files. Added optional 'editSpikewave' 
//             so that spike waveform is edited only if the user explicitly requests it.
// 11jan2008-- Modified replaceEdits() to recognize the new action codes ACTION_REMOVESORTSPK and ACTION_ADDSORTSPK 
//             that were introduced by the Java incarnation of XWork, called JMWork. Note that EDITCXDATA will remove 
//             these action codes if they are applied to a sorted-spike train channel that is being revised by the MEX 
//             function -- the assumption being that the user is replacing the sorted-spike train entirely, so the 
//             individual spike edits that were applied to the previous version of the train no longer apply!
// 22feb2010-- Fixed bugs: (1) Processing of ACTION_REMOVESORTSPK and ACTION_ADDSORTSPK was incorrect in the case where
//             codes existed for a sort-spike channel that was being replaced. (2) Conversion of absolute spike arrival
//             times to interspike intervals was introducing round-off jitter.
// 22sep2010-- Modified to support new action codes ACTION_DEFTAG and ACTION_DISCARD introduced in JMWork 1.4.0. The
//             former attaches a labelled tag to the data file's timeline, while the latter is an explicit indication 
//             that the data file has been marked as "discarded".
// 09sep2013-- Modified to support 50 sorted-spike train channels (record tag IDs 8-57) instead of 13 (8-20).
// 04jun2021-- Modified to support 200 sorted-spike train channels. The channel number is computed from the record
//             tag ID N=8..57, combined with a "bank number" M=0..3 stored in byte 1 of the 8-byte record tag (the ID
//             is in byte 0). Channel # = M*50 + N-8.
//===================================================================================================================== 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "mex.h"

#include "readcxdata.h"       // constants, structure definitions relevant to READCXDATA; we only use some of them


//===================================================================================================================== 
// MODULE GLOBALS, CONSTANTS
//===================================================================================================================== 

int m_nEditsSz = 0;           // buffer that holds all action/edit codes culled from CX_XWORKACTIONREC records.  since 
int m_nEditsBufSz = 0;        // an individual "action object" may be stored across two consecutive records, we must 
int* m_piEdits = NULL;        // read in all such records before processing them.

int m_nFastBytes = 0;         // buffer for compressed AI data from dedicated "fast" channel that
int m_nFastBufSz = 0;         // records spike waveform at 25KHz in Trial or Cont modes.  Culled from
char* m_pcFastData = NULL;    // CX_SPIKEWAVERECORDs.  [Applies only to data files w/ version>=2.]

int m_iVerbose = 0;           // if nonzero, printf's inform user of progress in editing data file (for debug)
int m_iEnaSpikewaveEdit = 0;  // if nonzero, enable editing of the data file's spike waveform records
BOOL m_isBigEndian;           // TRUE if system is big-endian, in which case endian conversions are necessary!

FILE* m_pFile = NULL;         // data file pointer
FILE* m_pTmpFile = NULL;      // temp file pointer (used to modify data file)

char m_strFileName[1024];     // original data file name
char m_tmpFileName[1024];     // temporary data file name

const int RECORDSZ = 1024;    // size of each record in a Maestro/Cntrlx data file


   
//===================================================================================================================== 
// FUNCTIONS DEFINED IN THIS MODULE
//===================================================================================================================== 
BOOL checkInput( const mxArray* pIn );
void usage();
void cleanup();
BOOL allocBuffers();
void freeBuffers();
int getNumRecordsInFile( FILE* pFile );
void endianSwap( BYTE* bytes, int nBytes );
BOOL readEdits( CXFILEREC* pRec );
BOOL writeEdits();
BOOL replaceEdits( const mxArray *pIn, BOOL *bChanged );
BOOL packTagLabel(char* sbuf, int* pLabelInts);
BOOL hasSpikeSortData(const mxArray* pChannels, int chNum);
BOOL writeSortedSpikes( const mxArray* pChannels );
BOOL readSpikewave(CXFILEREC* pRec);
BOOL replaceSpikewave(const mxArray *pSpikewave);
void uncompressAIData(double* pDst, int iDstSz, char* pSrc, int iSrcSz, int nCh, int* pNC, int* pNScans);
BOOL writeSpikewave();


//=== mexFunction (editcxdata) ======================================================================================== 
// 
//    This is the method called from MATLAB to modify or add to the contents of any XWORK analysis records that may be 
//    appended to a Maestro/Cntrlx data file.
//
//    ARGS:       nlhs, plhs  -- [out] function output ("left-hand side"), which is simply a 1x1 matrix holding an 
//                               error code: 0=success, -1=failure.  Error message printed to STDOUT.
//                nrhs, prhs  -- [in] array input.  See file header for content.
//
//    RETURNS:    NONE.
//
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )
{
   int i,j,k;
   BOOL bOk, bWriteActions, bSkip;
   BOOL bHadSortedSpikes;                                               // TRUE if original file had spike sort data
   BOOL bHeaderless;                                                    // TRUE for older, Cont-mode headerless files 
   BYTE recID;                                                          // first byte of a data file record is its "ID" 
   CXFILEREC fileRec;                                                   // a generic Maestro data file record
   CXFILEHDR fileHdr;                                                   // Maestro data file header
   int nRecords;                                                        // #records in the data file
   double* retCode;                                                     // ptr to return code in MATLAB matrix
   mxArray* pMxSpikeSortData;                                           // MEX cell array in the "sortedSpikes" field
   mxArray* pMxField;

   plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);                          // create 1x1 matrix for return code and init 
   retCode = mxGetPr(plhs[0]);                                          // to failure indication
   *retCode = -1; 

   if(sizeof(CXFILEHDR) != RECORDSZ || sizeof(CXFILEREC) != RECORDSZ)   // DBG: These structs MUST be the right size!
   {
      printf( "ERROR: Bad record size. hdr = %d, generic rec = %d",
              sizeof(CXFILEHDR), sizeof(CXFILEREC) );
      return;
   }

   if( nrhs < 2 || nrhs > 4 || nlhs > 1 )                               // check # of input/output args
   {
      usage();
      return;
   }

   if( !mxIsChar(prhs[0]) || !checkInput(prhs[1]) ||                    // check right-hand side args
       (nrhs>=3 && !mxIsDouble(prhs[2])) || 
       (nrhs==4 && !mxIsDouble(prhs[3])) )
   {
      usage();
      return;
   }
   
   if( nrhs >= 3 )                                                      // turn on verbose progress reporting?
      m_iVerbose = (int) *mxGetPr( prhs[2] );
   else
      m_iVerbose = 0;

   if( nrhs == 4 )                                                      // enable editing of spike waveform data in file?
      m_iEnaSpikewaveEdit = (int) *mxGetPr( prhs[3] );
   else
      m_iEnaSpikewaveEdit = 0;
   if(m_iEnaSpikewaveEdit)                                              // if 'spikewave' field is not present, or 
   {                                                                    // NULL or empty, then we won't touch spike
      i = mxGetFieldNumber(prhs[1], "spikewave");                       // waveform.
      if(i < 0) m_iEnaSpikewaveEdit = 0;
      else
      {
         pMxField = mxGetFieldByNumber(prhs[1], 0, i);
         if(pMxField == NULL || mxIsEmpty(pMxField)) 
            m_iEnaSpikewaveEdit = 0;
      }
   }

  
   i=0;                                                                 // detect endianness of system; we'll have to 
   ((BYTE*) &i)[0] = 1;                                                 // do conversions if it is big-endian
   m_isBigEndian = (i==1) ? FALSE : TRUE;
   if( m_iVerbose )
      printf( "Host is %s-endian!\n", m_isBigEndian ? "big" : "little" );

   if( !allocBuffers() )                                                // alloc global buffers for action/edit codes
   {                                                                    // and spike waveform data
      printf( "ERROR: Could not alloc buffers!\n" );
      return;
   }

   mxGetString( prhs[0], m_strFileName, mxGetN(prhs[0])+1 );            // get file's pathname

   if( (m_pFile = fopen( m_strFileName, "rb" )) == NULL )               // open the data file -- on the first pass, we 
   {                                                                    // just read in all records and build up the 
      printf( "ERROR: Could not open %s\n", m_strFileName );            // action/edit buffer contained therein
      cleanup();
      return;
   }
   if( m_iVerbose ) printf( "Opened %s\n", m_strFileName );

   nRecords = getNumRecordsInFile( m_pFile );                           // determine #records in the file; if this 
   if( nRecords < 0 )                                                   // failed, there's something wrong.  abort.
   {
      cleanup();
      return;
   }
   if( m_iVerbose ) printf( "File contains %i records.\n", nRecords );

   if( fread( (VOID*) &fileRec, RECORDSZ, 1, m_pFile ) == 0 )           // read in first record 
   {
      printf( "ERROR: reading header record in file %s\n", m_strFileName );
      cleanup();
      return;
   }

   bHeaderless = FALSE;                                                 // check to see if this is a headerless 
   recID = fileRec.idTag[0];                                            // ContMode data file
   if( (fileRec.idTag[1] == 0) &&  
       (recID <= CX_XWORKACTIONREC || recID == CX_V1TGTRECORD) )
   {
      bHeaderless = TRUE;
      if( m_iVerbose ) printf( "This is a headerless ContMode file.\n" );
   }

   for( i = (bHeaderless) ? 0 : 1; i < nRecords; i++)                   // read in & process one record at a time:
   { 
      if( i > 0 )                                                       //    read record into byte buffer; we don't
      {                                                                 //    need to do so for the first record in 
         if( fread( (VOID*) &fileRec, RECORDSZ, 1, m_pFile ) == 0 )     //    a headerless file, since we already read 
         {                                                              //    it in earlier!
            printf( "ERROR: Reading record %i in file %s\n", i, m_strFileName );
            cleanup();
            return;
         }
      }

      if( m_iVerbose )                                                  //    report record id tag (first 8 bytes)
      {
         printf( "ID tag for record %i: ", i );
         for( j = 0; j < 8; j++ ) printf( "%i ", (UINT) fileRec.idTag[j] );
         printf( "\n" );
      }

      bOk = TRUE;
      if( fileRec.idTag[0] == CX_XWORKACTIONREC )                       //    ignore all records except those that hold 
         bOk = readEdits( &fileRec );                                   //    action/edit codes or 25KHz spike waveform
      else if(fileRec.idTag[0] == CX_SPIKEWAVERECORD && m_iEnaSpikewaveEdit)
         bOk = readSpikewave(&fileRec);

      if( fileRec.idTag[0] >= CX_SPIKESORTREC_FIRST &&                  //    set flag if original file contained any 
          fileRec.idTag[0] <= CX_SPIKESORTREC_LAST )                    //    spike sort data
         bHadSortedSpikes = TRUE;

      if( !bOk )                                                        //    abort if an error (memory realloc) 
      {                                                                 //    occurred while processing a record
         cleanup();
         return;
      }
   }

   if( fseek( m_pFile, 0, SEEK_SET ) != 0 )                             // restore file ptr to file's beginning in 
   {                                                                    // preparation for second pass
      printf( "ERROR: Could not seek to beginning of file.\n" );
      cleanup();
      return;
   }

   if( m_iVerbose && (m_nEditsSz > 0) )                                 // were there any action codes already in file?
   {
      printf( "Found %i action/edit codes in %i records\n", m_piEdits[0], m_nEditsSz/CX_RECORDINTS );
   }

   if(m_iVerbose && m_iEnaSpikewaveEdit && (m_nFastBytes > 0))          // was there spike waveform data in file?
   {
      printf( "Found %i compressed bytes of 25KHz spike waveform data\n", m_nFastBytes );
   }

   // if editing of spike waveform data is enabled, replace spike waveform records read in from file IAW contents of the 
   // 'spikewave' field. This involves compressing the contents of 'spikewave' and packing it into the records.
   if(m_iEnaSpikewaveEdit)
   {
      pMxField = mxGetField(prhs[1], 0, "spikewave");
      if(!replaceSpikewave(pMxField))
      {
         cleanup();
         return;
      }
   }

   bWriteActions = FALSE;                                               // modify internal action code buf IAW analysis 
   if( !replaceEdits(prhs[1], &bWriteActions) )                         // data passed into this MEX function 
   { 
      cleanup();
      return;
   }

   if(!bWriteActions)                                                   // if there were no changes in action/edit 
   {                                                                    // codes, we still have to write action codes 
      bWriteActions = hasSpikeSortData(mxGetField(prhs[1],              // if there is spike sort data to write, or if 
            0, "sortedSpikes"), -1);                                    // we need to delete existing spike sort data
      if(!bWriteActions)
         bWriteActions = bHadSortedSpikes;
   }

   if(m_iEnaSpikewaveEdit || bWriteActions)                             // modify the original data file: 
   {                                                                    //  
      strcpy( m_tmpFileName, m_strFileName );                           //    create temp file in same dir, appending 
      strcat( m_tmpFileName, "." );                                     //    chars to original file name
      i = 1;
      while( i < 10 )
      {
         strcat( m_tmpFileName, "t" );
         m_pTmpFile = fopen( m_tmpFileName, "rb" );
         if( m_pTmpFile == NULL ) break;                                //    if non-NULL, tmp file already exists!
         fclose(m_pTmpFile);
         ++i;
      }
      if( i >= 10 ) 
      {
         printf( "ERROR: Could not generate temp file name\n" );
         cleanup();
         return;
      }
      if( (m_pTmpFile = fopen( m_tmpFileName, "wb" )) == NULL )         //    open the temp file
      { 
         printf( "ERROR: Could not open temp file %s\n", m_tmpFileName ); 
         cleanup();
         return;
      }

      for( i = 0; i < nRecords; i++ )                                   //    stream all unaffected records from 
      {                                                                 //    original data file to the temp file
         if( fread( (VOID*) &fileRec, RECORDSZ, 1, m_pFile ) == 0 )     //    without modification...
         { 
            printf( "ERROR: Reading record %i in file %s\n", i, m_strFileName );
            cleanup();
            return;
         }

         if(i==0 && (!bHeaderless) && m_iEnaSpikewaveEdit)              //    need to update #compressed bytes in 
         {                                                              //    spike waveform in file header
            ((CXFILEHDR*) &fileRec)->nSpikeBytesCompressed = m_nFastBytes;
         }
            
         recID = fileRec.idTag[0];
         bSkip = (i>0 || bHeaderless) && ((recID==CX_XWORKACTIONREC) || 
                                          (recID>=CX_SPIKESORTREC_FIRST && recID<=CX_SPIKESORTREC_LAST));
         if(!bSkip) bSkip = m_iEnaSpikewaveEdit && (recID==CX_SPIKEWAVERECORD);

         if( !bSkip )
         {
            if( fwrite((VOID*)&fileRec,RECORDSZ,1,m_pTmpFile) == 0 ) 
            {
               printf( "ERROR: Writing record %i to temp file\n", i );
               cleanup();
               return;
            }
         }
      }

      if( m_iEnaSpikewaveEdit && !writeSpikewave() )                    //    next, write all spike waveform records, 
      {                                                                 //    if we're editing spike waveform data.
         printf( "ERROR: Writing new spike wvform records to temp file\n" );
         cleanup();
         return;
      }
   
      if( !writeEdits() )                                               //    now write all action/edit codes in our 
      {                                                                 //    internal buffer
         printf( "ERROR: Writing new action records to temp file\n" );
         cleanup();
         return;
      }

      if( !writeSortedSpikes(mxGetField(prhs[1], 0, "sortedSpikes")) )  //    finally, write any spike sort data
      {
         printf( "ERROR: Writing new spike sort records to temp file\n" );
         cleanup();
         return;
      }

      fclose( m_pFile ); m_pFile = NULL;                                //    close original and temp file
      fclose( m_pTmpFile ); m_pTmpFile = NULL;

      if( remove( m_strFileName ) != 0 )                                //    delete original file
      {
         printf( "ERROR: Could not remove original file\n" );
         cleanup();
         return;
      }
      if( rename( m_tmpFileName, m_strFileName ) != 0 )                 //    and replace it with the temp file
      {
         printf( "ERROR: Could not replace original file with temp file. Original file %s LOST!\n", m_strFileName );
         printf( "Temp filename is %s\n", m_tmpFileName );
         cleanup();
         return;
      }
   }

   cleanup();                                                           // success!!
   *retCode = 0;
}


//=== checkInput ====================================================================================================== 
//
//    The modified analysis information to be stored in the Maestro data file by editcxdata() is passed to the MEX 
//    function in a structure array that must contain the following named fields:
//       "mark1" => NULL or empty or a 1xN double matrix.
//       "mark2" => NULL or empty or a 1xN double matrix.
//       "cut" => NULL or empty or a Nx3 double matrix.
//       "marks" => NULL or empty or a Nx2 double matrix.
//       "sortedSpikes" => NULL or empty or a 1x200 cell matrix. Each cell can be NULL or empty or a 1xN double array.
//       "tags" => NULL or empty or a 1xN structure matrix; each element has scalar double field 'time' and string
//          field 'label'.
//       'discard" => NULL or empty or a scalar double.
//
//    In addition, the structure array MAY contain the optional field "spikewave", which can be NULL or empty or a 
//    1xN double matrix.
//
//    This method verifies that the specified MEX array satisfies all of these constraints.
//
//    ARGS:       pIn   -- [in] pointer to the MEX array to be tested.
//
//    RETURNS:    TRUE if provided array satisfies all requirements; FALSE otherwise, in which case an error message 
//                is posted to STDOUT (it's a fatal error).
//
BOOL checkInput( const mxArray* pIn )
{
   mxArray* pMxField;
   mxArray* pMxSubField;
   mxArray* pMxCell;
   int i, nField, nDims;
   const size_t * dimensions;

   if( pIn == NULL || !mxIsStruct( pIn ) ) 
   {
      printf( "ERROR: Input argument is not a MATLAB structure array!\n" );
      return( FALSE );
   }

   // check field 'mark1':  it must be present in structure, and it can be NULL or a 1xN double array
   nField = mxGetFieldNumber( pIn, "mark1" );
   if( nField < 0 )
   {
      printf( "ERROR: Input structure is missing field 'mark1'!\n" );
      return( FALSE );
   }
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nDims = mxGetNumberOfDimensions( pMxField );
      dimensions = mxGetDimensions( pMxField );
      if( nDims != 2 || dimensions[0] != 1 || !mxIsDouble(pMxField) )
      {
         printf( "ERROR: Field 'mark1' must be NULL or a 1xN double array!\n" );
         return( FALSE );
      }
   }

   // check field 'mark2':  it must be present in structure, and it can be NULL or a 1xN double array
   nField = mxGetFieldNumber( pIn, "mark2" );
   if( nField < 0 )
   {
      printf( "ERROR: Input structure is missing field 'mark2'!\n" );
      return( FALSE );
   }
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nDims = mxGetNumberOfDimensions( pMxField );
      dimensions = mxGetDimensions( pMxField );
      if( nDims != 2 || dimensions[0] != 1 || !mxIsDouble(pMxField) )
      {
         printf( "ERROR: Field 'mark2' must be NULL or a 1xN double array!\n" );
         return( FALSE );
      }
   }

   // check field 'cut':  it must be present in structure, and it can be NULL or a Nx3 double array
   nField = mxGetFieldNumber( pIn, "cut" );
   if( nField < 0 )
   {
      printf( "ERROR: Input structure is missing field 'cut'!\n" );
      return( FALSE );
   }
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nDims = mxGetNumberOfDimensions( pMxField );
      dimensions = mxGetDimensions( pMxField );
      if( nDims != 2 || dimensions[1] != 3 || !mxIsDouble(pMxField) )
      {
         printf( "ERROR: Field 'cut' must be NULL or a Nx3 double array!\n" );
         return( FALSE );
      }
   }

   // check field 'marks':  it must be present in structure, and it can be NULL or a Nx2 double array
   nField = mxGetFieldNumber( pIn, "marks" );
   if( nField < 0 )
   {
      printf( "ERROR: Input structure is missing field 'marks'!\n" );
      return( FALSE );
   }
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nDims = mxGetNumberOfDimensions( pMxField );
      dimensions = mxGetDimensions( pMxField );
      if( nDims != 2 || dimensions[1] != 2 || !mxIsDouble(pMxField) )
      {
         printf( "ERROR: Field 'marks' must be NULL or a Nx2 double array!\n" );
         return( FALSE );
      }
   }

   // check field 'sortedSpikes': It must be present in structure, and it can be NULL or a 1x200 cell array. If
   // it is non-NULL, verify that each cell is either NULL or a 1xN double array
   nField = mxGetFieldNumber( pIn, "sortedSpikes" );
   if( nField < 0 )
   {
      printf( "ERROR: Input structure is missing field 'sortedSpikes'!\n" );
      return( FALSE );
   }
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nDims = mxGetNumberOfDimensions( pMxField );
      dimensions = mxGetDimensions( pMxField );
      
      if( nDims != 2 || dimensions[0] != 1 || dimensions[1] != NUMSPIKESORTCH || !mxIsCell(pMxField) )
      {
         printf( "ERROR: Field 'sortedSpikes' must be a 1x%d cell array!\n", NUMSPIKESORTCH );
         return( FALSE );
      }

      // now check the individual cells in the cell array
      for( i=0; i<NUMSPIKESORTCH; i++ )
      {
         pMxCell = mxGetCell( pMxField, i );
         if( pMxCell != NULL && !mxIsEmpty(pMxCell) )
         {
            nDims = mxGetNumberOfDimensions( pMxCell );
            dimensions = mxGetDimensions( pMxCell );
            if( nDims != 2 || dimensions[0] != 1 || !mxIsDouble(pMxCell) )
            {
               printf( "ERROR: Cell %d of 'sortedSpikes' field must be NULL or a 1xN double array!\n", i );
               return( FALSE );
            }
         }
      }
   }

   // check field 'tags':  it must be present in structure, and it can be NULL, empty or a 1xN structure array with 
   // fields 'time' (a scalar double) and 'label' (a Matlab string). We don't check the string for invalid length
   // or illegal characters. The string is autocorrected when the ACTION_DEFTAG code group is prepared.
   nField = mxGetFieldNumber(pIn, "tags");
   if(nField < 0)
   {
      printf("ERROR: Input structure is missing field 'tags'!\n");
      return(FALSE);
   }
   pMxField = mxGetFieldByNumber(pIn, 0, nField);
   if(pMxField != NULL && !mxIsEmpty(pMxField))
   {
      nDims = mxGetNumberOfDimensions(pMxField);
      dimensions = mxGetDimensions(pMxField);
      if(nDims != 2 || dimensions[0] != 1 || !mxIsStruct(pMxField))
      {
         printf("ERROR: Field 'tags' must be NULL, empty, or a 1xN structure array!\n");
         return(FALSE);
      }
      
      nField = mxGetFieldNumber(pMxField, "time");
      if(nField < 0)
      {
         printf("ERROR: Structure array member 'tags.time' is missing!\n");
         return(FALSE);
      }
      for(i=0; i<dimensions[1]; i++)
      {
         pMxSubField = mxGetFieldByNumber(pMxField, i, nField);
         if(pMxSubField == NULL || !mxIsDouble(pMxSubField) || (mxGetNumberOfElements(pMxSubField) != 1))
         {
            printf("ERROR: 'tags(%d).time' is not a scalar double?\n", i);
            return(FALSE);
         }
      }
      
      nField = mxGetFieldNumber(pMxField, "label");
      if(nField < 0)
      {
         printf("ERROR: Structure array member 'tags.label' is missing!\n");
         return(FALSE);
      }
      for(i=0; i<dimensions[1]; i++)
      {
         pMxSubField = mxGetFieldByNumber(pMxField, i, nField);
         if(pMxSubField == NULL || !mxIsChar(pMxSubField))
         {
            printf("ERROR: 'tags(%d).label' is not a string array?\n", i);
            return(FALSE);
         }
      }
   }

   // check field 'discard':  it must be present in structure and must NULL, empty, or a scalar double
   nField = mxGetFieldNumber(pIn, "discard");
   if(nField < 0)
   {
      printf("ERROR: Input structure is missing field 'discard'!\n");
      return(FALSE);
   }
   pMxField = mxGetFieldByNumber(pIn, 0, nField);
   if(pMxField != NULL && !mxIsEmpty(pMxField) && (!mxIsDouble(pMxField) || (mxGetNumberOfElements(pMxField) != 1)))
   {
      printf("ERROR: Field 'discard' is not a scalar double?\n");
      return(FALSE);
   }
   
   // check OPTIONAL field 'spikewave': If present in input structure, it can be NULL or empty, or a 1xN double array.
   nField = mxGetFieldNumber( pIn, "spikewave" );
   if(nField < 0) return(TRUE);
   
   pMxField = mxGetFieldByNumber( pIn, 0, nField );
   if(pMxField == NULL || mxIsEmpty(pMxField)) return( TRUE );

   nDims = mxGetNumberOfDimensions(pMxField);
   dimensions = mxGetDimensions(pMxField);
   if(nDims != 2 || dimensions[0] != 1 || !mxIsDouble(pMxField))
   {
      printf("ERROR: Field 'spikewave' must be a 1xN double array!\n");
      return(FALSE);
   }

   return(TRUE);
}


//=== usage =========================================================================================================== 
//
//    Prints editcxdata() usage details to STDOUT.
//
//    ARGS:       NONE
//    RETURNS:    NONE
void usage()
{
   printf( "USAGE: res = editcxdata( 'filename', data [, verbose, editSpikewave] ) \n" );
   printf( "   filename --> Pathname of Maestro/Cntrlx data file to be edited.\n" );
   printf( "   data     --> A MATLAB structure array that must, at a minimum, have the following named fields.\n" ); 
   printf( "   Note that any of the first five fields may be a null matrix, indicating the absence of data.\n" );
   printf( "      mark1 : A 1xN double array that contains N marker #1 timepoints in milliseconds relative to \n" ); 
   printf( "         the start of the recording.\n" ); 
   printf( "      mark2 : Analogously for marker #2 timepoints.\n" ); 
   printf( "      cut : A Nx3 double matrix that defines N cuts in recorded data.  Each row of the matrix is a\n" ); 
   printf( "         triplet [startT endT ch#], where (startT, endT) indicate the time period over which the\n" ); 
   printf( "         cut is made (in milliseconds) and ch# is the channel# of the affected data stream.\n" );
   printf( "      marks : A Nx2 double matrix that defines N mark segments.  Each row of the matrix [t0 t1] \n" );
   printf( "         defines a mark segment starting at t0 and ending at t1 (in milliseconds).\n" );
   printf( "      sortedSpikes : A 1x200 MATLAB cell array containing spike sort data. Each cell of the array\n" );
   printf( "         corresponds to one of 200 possible spike sort channels, added to the data file by analysis\n" );
   printf( "         code. A particular cell is either a null matrix -- meaning no spike sort data on that \n" );
   printf( "         channel -- or a 1xN double array of N spike arrival times in milliseconds. Spike times \n" );
   printf( "         must be stored chronologically, in milliseconds relative to the start of recording, and \n" );
   printf( "         be accurate to 10us. If sortedSpikes = [], all sorted-spike train data is removed from file.\n" );
   printf( "      tags : A 1xN MATLAB structure array defining any tags to be attached to the data file's \n" );
   printf( "         recorded timeline via the ACTION_DEFTAG action code group. Each element of the array is a \n" );
   printf( "         structure with fields 'time' (the elapsed time in ms since recording began) and 'label' \n" );
   printf( "         (1-16 visible ASCII characters). NOTE that function will auto-correct invalid labels, \n" );
   printf( "         displaying a warning in the Matlab command window if the verbose flag is set. \n" );
   printf( "      discard : A scalar double. If nonzero, the ACTION_DISCARD action code is added to the file, \n" );
   printf( "         explicitly marking the file as 'discarded', to be ignored by downstream analysis modules. \n" );
   printf( "      spikewave : [Optional] A 1xN double array that contains the uncompressed spike waveform, \n" );
   printf( "         assumed to be recorded at 25KHz. This field is exposed so that users can process the \n" );
   printf( "         spike waveform in some way -- to remove artifacts for example -- prior to submitting it \n" );
   printf( "         to a spike-sorting application. The spike waveform in this field is recompressed and \n" );
   printf( "         packaged into the appropriate records in the data file, replacing any spike waveform \n" );
   printf( "         records that were previously in the file. NOTE: EDITCXDATA does NOT let user change the \n" );
   printf( "         length of the spike waveform, only the samples themselves. Spike waveform data is edited \n" );
   printf( "         only if the following conditions are met: 1) the 'editSpikewave' guard flag is set; 2) this \n" );
   printf( "         field is present, non-NULL, and not empty. If the conditions are met, but N != the number \n" );
   printf( "         of samples in the spike waveform read from the file, the function fails entirely. \n" );
   printf( "   verbose  --> If nonzero, function prints detailed progress messages.\n" );
   printf( "   editSpikewave  --> This flag guards against inadvertently editing the spike waveform data in the \n" );
   printf( "   file. It must be explicitly set to a nonzero value, or no change is made to spike waveform data. \n" );
}


//=== cleanup ========================================================================================================= 
//
//    Release system resources allocated by editcxdata():  all storage buffers, the data file itself, and a temporary 
//    file used when modifying data file contents.
//
void cleanup()
{
   freeBuffers();
   if( m_pFile != NULL ) 
   {
      fclose( m_pFile );
      m_pFile = NULL;
   }
   if( m_pTmpFile != NULL ) 
   {
      fclose( m_pTmpFile );
      m_pTmpFile = NULL;
   }
}


//=== allocBuffers ==================================================================================================== 
//
//    Allocate the global buffers used to hold and manipulate action/edit codes and the 25KHz spike waveform records. 
//
BOOL allocBuffers()
{
   freeBuffers();
   m_nEditsBufSz = 2 * CX_RECORDINTS; 
   m_piEdits = (int *) malloc( sizeof(int) * m_nEditsBufSz ); 
   if(m_piEdits == NULL) 
   {
      m_nEditsBufSz = 0;
      return(FALSE);
   }

   m_nFastBufSz = 20 * RECORDSZ;
   m_pcFastData = (char *) malloc( sizeof(char) * m_nFastBufSz );
   if(m_pcFastData == NULL)
   {
      m_nFastBufSz = 0;
      m_nFastBytes = 0;
      freeBuffers();
      return(FALSE);
   }
   
   return(TRUE);
}


//=== freeBuffers ===================================================================================================== 
//
//    Free global buffers used to hold and manipulate action/edit codes and the 25KHz spike waveform records.
//
void freeBuffers()
{
   if( m_piEdits != NULL ) 
   { 
      free( m_piEdits ); 
      m_piEdits = NULL; 
      m_nEditsBufSz = 0; 
      m_nEditsSz = 0;
   }

   if(m_pcFastData != NULL)
   {
      free(m_pcFastData);
      m_pcFastData = NULL;
      m_nFastBufSz = 0;
      m_nFastBytes = 0;
   }
}


//=== getNumRecordsInFile ============================================================================================= 
//
//    Determines file length by seeking EOF and reading file position, then sets #records accordingly.  Assumes file 
//    is opened in binary (vs text) mode, and returns file ptr to the file's beginning upon successful return.
//
//    ARGS:       pFile -- [in] open file pointer.
//
//    RETURNS:    #records if successful, -1 otherwise (a fatal error -- appropriate error msg printed to STDOUT).
//
int getNumRecordsInFile( FILE* pFile )
{
   long nFileBytes;

   if( fseek( pFile, 0, SEEK_END ) != 0 )                               // seek to end of file
   {
      printf( "ERROR: Could not seek to end of file.\n" );
      return( -1 );
   }

   nFileBytes = ftell( pFile );                                         // end of file pos = file len in bytes if it is 
   if( nFileBytes == -1L )                                              // opened in binary mode
   {
      printf( "ERROR: Unable to read file ptr position.\n" );
      return( -1 );
   }

   if( (nFileBytes % RECORDSZ) != 0 )                                   // there should always be an integral # of 
   {                                                                    // data records in a CNTRLX data file!
      printf( "ERROR: File does not have an integral # of %i-byte records; filesize = %i.\n", RECORDSZ, nFileBytes );
      return( -1 );
   }

   if( fseek( pFile, 0, SEEK_SET ) != 0 )                               // restore file ptr to file's beginning
   {
      printf( "ERROR: Could not seek to beginning of file.\n" );
      return( -1 );
   }

   return( (int) (nFileBytes/((long)RECORDSZ)) );                       // calc #records in file
}


//=== endianSwap ====================================================================================================== 
//
//    Swaps endianness of atomic types like short, int, float, and double.
//
//    CREDIT:  Code Project article dtd 19Aug2003 by JC Cobas (http://www.codetools.com/cpp/endianness.asp#xx586418xx). 
//
//    ARGS:       bytes -- [in/out] address of atomic value, cast to a byte array for access to individual bytes.
//                nBytes-- [in] the number of bytes in the atomic value.
//
void endianSwap( BYTE* bytes, int nBytes )
{
   register int i = 0;
   register int j = nBytes-1;
   register BYTE ucTmp = 0;
   while( i<j )
   {
      ucTmp = bytes[i];
      bytes[i] = bytes[j];
      bytes[j] = ucTmp;
      i++, j--;
   }
}


//=== readEdits ======================================================================================================= 
//
//    Read integer-valued data from a CX_XWORKACTIONREC data file record into a global buffer.  Realloc buffer as 
//    needed; abort if reallocation fails.  The data is merely copied as is.
//
//    ARGS:       pRec  -- [in] ptr to buffer holding the CX_XWORKACTIONREC record.  File record fmt encapsulated by 
//                         the CXFILEREC structure (see CXFILEFMT.H).
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL readEdits( CXFILEREC* pRec )
{
   int i, nInt;
   int* piNewBuf;                                                       // ptr to reallocated buffer, if needed
   int iExtra = CX_RECORDINTS;                                          // if we must realloc, add 1 records' worth

   if(m_nEditsSz + CX_RECORDINTS > m_nEditsBufSz )                      // insufficient space; reallocate internal buf. 
   {                                                                    // abort if reallocation fails.
      piNewBuf = (int*) realloc( (void*) m_piEdits, 
                           sizeof(int)*(iExtra + m_nEditsBufSz) );
      if( piNewBuf == NULL )
      {
         printf( "ERROR: Internal buffer reallocation failed!\n" );
         return( FALSE );
      }
      m_piEdits = piNewBuf;
      m_nEditsBufSz += iExtra;
   }

   if( m_isBigEndian )                                                  // convert endianness if necessary
   {
      nInt = sizeof(int);
      for( i=0; i<CX_RECORDINTS; i++ )
         endianSwap( (BYTE*) &(pRec->u.iData[i]), nInt );
   }

   memcpy( (void*) (m_piEdits + m_nEditsSz),                            // copy data in record to internal buf as is 
           (void*) &(pRec->u.iData[0]), CX_RECORDINTS*sizeof(int) );
   if( m_iVerbose )
   {
      printf( "Read %i-th action/edit record\n", m_nEditsSz/CX_RECORDINTS );
      if( m_nEditsSz == 0 )
         printf( "Action code count in original file = %i\n", m_piEdits[0] );
   }
   m_nEditsSz += CX_RECORDINTS;
   return( TRUE );
}


//=== writeEdits ====================================================================================================== 
//
//    Writes all action/edit codes currently in the internal buffer to the temporary file.  It assumes that the 
//    temporary file is already open and the file pointer is at the right location, and it assumes that the internal 
//    action/edit codes buffer has been filled properly with the desired set of action/edit codes.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE if a file write error occurs.
//
BOOL writeEdits()
{
   int i, j, nCodes, nInt;
   CXFILEREC fileRec;

   for( i=0; i<8; i++ ) fileRec.idTag[i] = 0;                           // init header portion of action record buf
   fileRec.idTag[0] = CX_XWORKACTIONREC;

   nInt = sizeof(int);

   i = 0;                                                               // write all action codes in one or more 
   while( i < m_nEditsSz )                                              // records as needed; partial record is padded 
   {                                                                    // with zeroes.
      memset( (VOID*) &(fileRec.u.iData[0]), 0, CX_RECORDINTS*nInt );
      nCodes = (m_nEditsSz-i > CX_RECORDINTS) ? CX_RECORDINTS : m_nEditsSz-i;
      memcpy( (VOID*) &(fileRec.u.iData[0]), (VOID*) (m_piEdits + i), nCodes*nInt );

      if( m_isBigEndian )                                               // data file must be little-endian!  convert 
      {                                                                 // endianness before writing.
         for( j=0; j<nCodes; j++ )
            endianSwap( (BYTE*) &(fileRec.u.iData[j]), nInt );
      }

      if( fwrite((VOID*) &fileRec, RECORDSZ, 1, m_pTmpFile) == 0 )
         return( FALSE );
      i += nCodes;
   }

   return( TRUE );
}


//=== replaceEdits ==================================================================================================== 
//
//    Modifies existing internal buffer of the original data file's action/edit codes as follows:
//       1) Remove all existing ACTION_SETMARK1 codes, then add new codes to store the set of marker times specified by 
//          the "mark1" field in the structure array provided.
//       2) Remove all existing ACTION_SETMARK2 codes, then add new codes to store the set of marker times specified by 
//          the "mark2" field in the structure array provided.
//       3) Remove all existing ACTION_CUTIT codes, then add new codes to store the set of saccade cuts specified by 
//          the "cut" field in the structure array provided.  NOTE:  The readcxdata() MEX function takes the code group 
//          [ACTION_CUTIT ch# refT startT endT] and reports [t0 t1 ch#] for each cut, where t0=refT+startT and t1=
//          refT+endT.  The editcxdata() MEX function takes saccade cut info in the same format as provided by 
//          readcxdata(), so it MUST ASSUME that refT=0 always.
//       4) Remove all existing ACTION_MARK codes, then add new codes to store the set of mark segments specified by 
//          the "marks" field in the structure array provided.
//       5) Remove all existing ACTION_DEFTAG codes, then add new codes to store the set of labelled tags specified by
//          the "tags" field in the structure array provided.
//       6) Remove any ACTION_DISCARD codes. If the "discard" field in the structure array provided is nonzero, then
//          add the ACTION_DISCARD code back in.
//
//    The method assumes that the input argument satisfies all the constraints tested by the function checkInput(). 
//
//    The method handles the possibility that the original data file contains certain XWORK action/edit codes other 
//    than the ones listed above. These codes are generally retained in the modified data file without modification. 
//    The exceptions are the new JMWork codes that add/remove individual spikes from a sorted spike train channel --
//    ACTION_REMOVESORTSPK and ACTION_ADDSORTSPK. These codes are removed no matter what. By design, EDITCXDATA removes
//    all existing sorted spike train data from the file, replacing it IAW the contents of the data.sortedSpikes cell 
//    array -- so any JMWork spike edits are no longer relevant.
//
//    ARGS:       pIn   -- [in] MEX structure array holding the new analysis data ("mark1" times, "mark2" times, 
//                         saccade cuts, and mark segments) that will replace any existing analysis data of the same 
//                         category.
//                bChanged -- [out] if TRUE, then internal action code buffer was modified.
//
//    RETURNS:    TRUE if successful, FALSE otherwise (failed to allocate buffer to hold modified action/edit codes).
//
BOOL replaceEdits( const mxArray *pIn, BOOL *bChanged )
{
   int i, j, k, ch, nCodes;
   BOOL ok;
   int nMark1;                                                          // the new set of "mark1" times
   double* pdMark1;
   int nMark2;                                                          // the new set of "mark2" times
   double* pdMark2;
   int nCuts;                                                           // the new set of saccade cuts, an Nx3 array in 
   double* pdCuts;                                                      // column-major order
   int nMarkSegs;                                                       // the new set of mark segments, Nx2 array
   double* pdMarkSegs;
   int nTags;                                                           // the new set of tags, 1XN structure array
   PTAGMARK pdTags;
   int discarded;                                                       // nonzero if explicit discard mark is set
   
   int nActions;                                                        // the total number of action codes
   int* pNewEditBuf;                                                    // the modified set of action codes
   mxArray* pMxField;
   
   *bChanged = FALSE;                                                   // haven't made any changes yet
   
   // extract the new analysis data from the input structure (there might be none!)...
   nMark1 = 0;
   pdMark1 = NULL; 
   pMxField = mxGetField( pIn, 0, "mark1" );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nMark1 = mxGetNumberOfElements( pMxField );
      pdMark1 = mxGetPr( pMxField );
   }

   nMark2 = 0; 
   pdMark2 = NULL;
   pMxField = mxGetField( pIn, 0, "mark2" );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nMark2 = mxGetNumberOfElements( pMxField );
      pdMark2 = mxGetPr( pMxField );
   }

   nCuts = 0; 
   pdCuts = NULL;
   pMxField = mxGetField( pIn, 0, "cut" );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nCuts = mxGetNumberOfElements( pMxField ) / 3;
      pdCuts = mxGetPr( pMxField );
   }

   nMarkSegs = 0; 
   pdMarkSegs = NULL;
   pMxField = mxGetField( pIn, 0, "marks" );
   if( pMxField != NULL && !mxIsEmpty(pMxField) )
   {
      nMarkSegs = mxGetNumberOfElements( pMxField ) / 2;
      pdMarkSegs = mxGetPr( pMxField );
   }

   nTags = 0;
   pdTags = NULL;
   pMxField = mxGetField(pIn, 0, "tags");
   if(pMxField != NULL && !mxIsEmpty(pMxField))
   {
      nTags = mxGetNumberOfElements(pMxField);
      pdTags = (PTAGMARK) malloc(nTags * sizeof(TAGMARK));
      if(pdTags == NULL)
      {
         printf( "Memory allocation failed!\n" );
         return( FALSE );
      }
      memset(pdTags, 0, nTags*sizeof(TAGMARK));
      
      for(i=0; i<nTags; i++)
      {
         pdTags[i].time = (int) mxGetScalar(mxGetField(pMxField, i, "time"));
         mxGetString(mxGetField(pMxField, i, "label"), pdTags[i].label, 17);
      }
   }
   
   // the discard field may be an empty matrix (never init'd by readcxdata), in which case file is NOT discarded
   discarded = 0;
   pMxField = mxGetField(pIn, 0, "discard");
   if((pMxField != NULL) && (!mxIsEmpty(pMxField)) && (mxGetScalar(pMxField) != 0))
      discarded = 1;
   
   // calculate #codes needed to store the new analysis actions
   nActions = 2*nMark1 + 2*nMark2 + 5*nCuts + 3*nMarkSegs + 6*nTags + discarded;
   
   // go thru existing action codes and include all those that are NOT replaced by this function
   if(m_nEditsSz > 0) 
   { 
      i = 1; 
      while(i < m_piEdits[0])
      {
         switch( m_piEdits[i] )                                         //    #codes to copy depends upon the action 
         {                                                              //    code type...
            case ACTION_SACCUT:     nCodes = 10; break;
            case ACTION_RMUNIT:
            case ACTION_ADDUNIT:    
            case ACTION_SETMARK1:   
            case ACTION_SETMARK2:   nCodes = 2; break;
            case ACTION_EDITEVENT: 
            case ACTION_RMALL:
            case ACTION_MARK:       nCodes = 3; break;
            case ACTION_CUTIT:      nCodes = 5; break;
            
            case ACTION_REMOVESORTSPK:                                  //   new JMWork codes for individual spike 
            case ACTION_ADDSORTSPK: nCodes = 3; break;                  //   edits on a sorted-spike train. 
            
            case ACTION_DEFTAG:     nCodes = 6; break;
            case ACTION_DISCARD:    nCodes = 1; break;
            
            default:                nCodes = 1; break;
         }

         // NOTE: We get rid of JMWork "spike edits"  no matter what
         if(m_piEdits[i] != ACTION_REMOVESORTSPK && m_piEdits[i] != ACTION_ADDSORTSPK && 
             m_piEdits[i] != ACTION_SETMARK1 && m_piEdits[i] != ACTION_SETMARK2 && 
             m_piEdits[i] != ACTION_CUTIT && m_piEdits[i] != ACTION_MARK &&
             m_piEdits[i] != ACTION_DEFTAG && m_piEdits[i] != ACTION_DISCARD)
            nActions += nCodes;

         i += nCodes;
      }
   }

   // if there no actions to add or remove, we're done -- no changes made!
   if(nMark1 == 0 && nMark2 == 0 && nCuts == 0 && nMarkSegs == 0 && nTags == 0 && (!discarded) &&
       (m_nEditsSz == 0 || nActions == m_piEdits[0]))
      return(TRUE);

   // at this point, the internal action code buffer will definitely be modified
   *bChanged = TRUE;

   // modified set of action codes is empty! Free the buffer and return.
   if(nActions == 0)  
   {
      if(m_piEdits != NULL) 
      { 
         free(m_piEdits); 
         m_piEdits = NULL; 
         m_nEditsBufSz = 0; 
         m_nEditsSz = 0;
      }
      return(TRUE);
   }

   // allocate new buffer to hold the modified set of action codes (the extra code is for the total code count).
   pNewEditBuf = (int *) malloc(sizeof(int) * (nActions+1)); 
   if(pNewEditBuf == NULL) 
   {
      printf( "Memory allocation failed!\n" );
      return( FALSE );
   }
   memset((void*) pNewEditBuf, 0, sizeof(int) * (nActions+1));

   // copy all existing action codes that we don't alter into the new action code buffer. Note that starting index is 1
   // here because the code count occupies that position in the buffer.
   k = 1; 
   if(m_nEditsSz > 0 && m_piEdits[0] > 0)
   {
      i = 1; 
      while(i < m_piEdits[0]) 
      {
         switch( m_piEdits[i] )                                         //    #codes to copy depends upon the action 
         {                                                              //    code type...
            case ACTION_SACCUT:     nCodes = 10; break;
            case ACTION_RMUNIT:
            case ACTION_ADDUNIT:    
            case ACTION_SETMARK1:   
            case ACTION_SETMARK2:   nCodes = 2; break;
            case ACTION_EDITEVENT:  
            case ACTION_RMALL:
            case ACTION_MARK:       nCodes = 3; break;
            case ACTION_CUTIT:      nCodes = 5; break;
            
            case ACTION_REMOVESORTSPK:                                  //   new JMWork codes for individual spike 
            case ACTION_ADDSORTSPK: nCodes = 3; break;                  //   edits on a sorted-spike train. 
            
            case ACTION_DEFTAG:     nCodes = 6; break;
            case ACTION_DISCARD:    nCodes = 1; break;
            
            default:                nCodes = 1; break;
         }

         // copy all codes that we don't change. Again, all JMWork spike-edit action codes are removed
         if(m_piEdits[i] != ACTION_REMOVESORTSPK && m_piEdits[i] != ACTION_ADDSORTSPK &&
             m_piEdits[i] != ACTION_SETMARK1 &&  m_piEdits[i] != ACTION_SETMARK2 && 
             m_piEdits[i] != ACTION_CUTIT && m_piEdits[i] != ACTION_MARK &&
             m_piEdits[i] != ACTION_DEFTAG && m_piEdits[i] != ACTION_DISCARD)
         {
            for( j=0; j<nCodes; j++ ) 
               pNewEditBuf[k+j] = m_piEdits[i+j];
            k += nCodes;
         }
         else if( m_iVerbose )
         {
            if( m_piEdits[i] == ACTION_SETMARK1 )
               printf( "Removing code [ACTION_SETMARK1 %d]\n", m_piEdits[i+1] );
            else if( m_piEdits[i] == ACTION_SETMARK2 )
               printf( "Removing code [ACTION_SETMARK2 %d]\n", m_piEdits[i+1] );
            else if( m_piEdits[i] == ACTION_CUTIT )
               printf( "Removing code [ACTION_CUTIT %d %d %d %d]\n", m_piEdits[i+1], m_piEdits[i+2], 
                  m_piEdits[i+3], m_piEdits[i+4] );
            else if(m_piEdits[i] == ACTION_MARK)
               printf( "Removing code [ACTION_MARK %d %d]\n", m_piEdits[i+1], m_piEdits[i+2] );
            else if(m_piEdits[i] == ACTION_REMOVESORTSPK)
               printf( "Removing code [ACTION_REMOVESORTSPK %d %d]\n", m_piEdits[i+1], m_piEdits[i+2] );
            else if(m_piEdits[i] == ACTION_ADDSORTSPK)
               printf( "Removing code [ACTION_ADDSORTSPK %d %d]\n", m_piEdits[i+1], m_piEdits[i+2] );
            else if(m_piEdits[i] == ACTION_DEFTAG)
               printf("Removing code [ACTION_DEFTAG %d ...]\n", m_piEdits[i+1]);
            else if(m_piEdits[i] == ACTION_DISCARD)
               printf("Removing code ACTION_DISCARD\n");
         }

         i += nCodes;
      }
   }

   for( i=0; i<nMark1; i++ )                                            // now append the new SETMARK1 codes, if any
   {
      pNewEditBuf[k++] = ACTION_SETMARK1;
      pNewEditBuf[k++] = (int) pdMark1[i];
      if( m_iVerbose )
         printf( "Adding code [ACTION_SETMARK1 %d]\n", (int) pdMark1[i] );
   }

   for( i=0; i<nMark2; i++ )                                            // then append the new SETMARK2 codes, if any
   {
      pNewEditBuf[k++] = ACTION_SETMARK2;
      pNewEditBuf[k++] = (int) pdMark2[i];
      if( m_iVerbose )
         printf( "Adding code [ACTION_SETMARK2 %d]\n", (int) pdMark2[i] );
   }

   for( i=0; i<nCuts; i++ )                                             // then append the new CUTIT codes, if any
   {
      pNewEditBuf[k++] = ACTION_CUTIT;
      pNewEditBuf[k++] = (int) pdCuts[nCuts*2 + i];                     // ch# is stored in third col of Nx3 matrix
      pNewEditBuf[k++] = 0;                                             // refT is assumed to be 0 always
      pNewEditBuf[k++] = (int) pdCuts[i];                               // startT is stored in first col
      pNewEditBuf[k++] = (int) pdCuts[nCuts + i];                       // endT is stored in next col
      if( m_iVerbose )
         printf( "Adding code [ACTION_CUTIT %d 0 %d %d]\n", (int) pdCuts[nCuts*2 + i],
            (int) pdCuts[i], (int) pdCuts[nCuts + i] );
   }

   for( i=0; i<nMarkSegs; i++ )                                         // then append the new MARK codes, if any
   {
      pNewEditBuf[k++] = ACTION_MARK;
      pNewEditBuf[k++] = (int) pdMarkSegs[i];                           // startT is stored in 1st col of Nx2 matrix
      pNewEditBuf[k++] = (int) pdMarkSegs[nMarkSegs + i];               // endT is stored in 2nd col
      if( m_iVerbose )
         printf( "Adding code [ACTION_MARK %d %d]\n", (int) pdMarkSegs[i], (int) pdMarkSegs[nMarkSegs + i] );
   }

   // append the new ACTION_DEFTAG codes, if any
   for(i=0; i<nTags; i++)
   {
      pNewEditBuf[k++] = ACTION_DEFTAG;
      pNewEditBuf[k++] = pdTags[i].time;
      ok = packTagLabel(pdTags[i].label, &(pNewEditBuf[k]));
      k += 4;
      if(m_iVerbose)
      {
         if(ok) printf("Adding code [ACTION_DEFTAG %d %s]\n", pdTags[i].time, pdTags[i].label);
         else printf("Adding code [ACTION_DEFTAG %d ...]   WARNING! Label was autocorrected!\n", pdTags[i].time);
      }
   }
   
   // append the ACTION_DISCARD code, if requested
   if(discarded != 0)
   {
      pNewEditBuf[k++] = ACTION_DISCARD;
      if(m_iVerbose) printf("Adding ACTION_DISCARD code\n");
   }
   
   
   if(m_iVerbose) printf( "Modified file will have %i(%i) action/edit codes\n", k-1, nActions );

   pNewEditBuf[0] = nActions;                                           // the total code count is first int in buffer

   if( m_piEdits != NULL )                                              // free the old action codes buffer and replace 
   {                                                                    // with the modified one
      free( m_piEdits ); 
      m_piEdits = NULL; 
      m_nEditsBufSz = 0; 
      m_nEditsSz = 0;
   }
   m_piEdits = pNewEditBuf;
   m_nEditsBufSz = m_nEditsSz = nActions + 1;

   return( TRUE );
}

//=== packTagLabel ====================================================================================================
// Helper method for replaceEdits() that handles the task of packing the tag label into the last 4 codes of the
// ACTION_DEFTAG action code group. The 16-byte label field is packed into these 32-bit integers in little-endian 
// order. By definition, the label string can contain 1-16 visible ASCII characters (0x21-0x7E). If the string is less
// than 16 characters, the remaining bytes are filled with nulls (0). Any invalid non-null characters are replaced with 
// the exclamation point (0x21).
//
// @param sbuf The buffer that holds the label string to be packed into the action codes provided. It is assumed the
// buffer has up to 16 visible ASCII characters plus a terminating null.
// @param pLabelInts Buffer containing the four action codes in which the label field bytes are stored
// @return True if label was valid and packed into the action codes unchanged; false if label was invalid and had to
// be autocorrected (empty string or invalid characters found).
BOOL packTagLabel(char* sbuf, int* pLabelInts)
{
   int i, j, idx, chunk, gotNull;
   char c;
   BOOL valid;
   
   // pack each action code in turn, in little-endian order
   valid = TRUE;
   gotNull = 0;
   idx = 0;
   for(i=0; i<4; i++)
   {
      chunk = 0;   // this initializes the int chunk to all 'null' characters
      for(j=0; (gotNull == 0) && j<4; j++)
      {
         c = sbuf[idx];
         if(gotNull != 0 || c == 0)
         {
            gotNull = 1;
            if(idx == 0) 
            {
               c = (char) 0x21;
               valid = FALSE;
            }
            else c = (char) 0;
         }
         else if(c < (char)0x21 || c > (char)0x7E)
         {
            c = (char) 0x21;
            valid = FALSE;
         }
         ++idx;
         
         chunk |= (((int) c) << (j*8));
      }
      pLabelInts[i] = chunk;
   }
   
   return(valid);
}


//=== hasSpikeSortData ================================================================================================ 
//
//    Returns TRUE if the specified cell array contains any non-empty sorted-spike trains, or if a specific cell in the 
//    array contains a non-empty train.
//   
//
//    The method assumes that the cell array -- unless it is NULL -- has N=NUMSPIKESORTCH cells, with each cell
//    corresponding to one of the N different sorted spike train channels that can be persisted in a Maestro data file.
//
//    ARGS: pChannels -- [in] a 1xNUMSPIKESORTCH cell array containing the sorted spike train channel data. If NULL,
//    then there is no spike sort data.
//          chNum -- [in] If this lies in [0..NUMSPIKESORTCH-1], then method returns TRUE if that particular cell is 
//    not empty. Else, it returns TRUE if any of the cells is not empty.
//
//    RETURNS: As described above.
//
BOOL hasSpikeSortData(const mxArray* pChannels, int chNum)
{
   int i, start, end;
   mxArray* pChan;

   if(pChannels == NULL || mxIsEmpty(pChannels)) return( FALSE );

   start = (chNum>=0 && chNum < NUMSPIKESORTCH) ? chNum : 0;
   end = (chNum>=0 && chNum < NUMSPIKESORTCH) ? chNum : NUMSPIKESORTCH-1;
   for(i=start; i <= end; i++)
   {
      pChan = mxGetCell(pChannels, i);
      if((pChan != NULL) && (mxGetNumberOfElements(pChan) > 0)) return(TRUE);
   }

   return(FALSE);
}

//=== writeSortedSpikes =============================================================================================== 
//
//    Writes the appropriate "sorted spike train" records to the temporary file to persist any spike sorting data found
//    in the provided 1xNUMSPIKESORTCH MEX cell array.  
//
//    The method assumes that the cell array A -- unless it is NULL -- has N=NUMSPIKESORTCH cells, with each cell A{P}
//    corresponding to one of the P different sorted spike train channels that can be persisted in a Maestro data file.
//    Each channel's spike times (in milliseconds) are stored in a separate set of 1 or more records.
//
//    To support 200 distinct channels, the channel number is encoded in the first two bytes of the 8-byte record tag.
//    Byte 0 holds the tag ID, N=[CX_SPIKESORTREC_FIRST...CX_SPIKESORTREC_LAST], while byte 1 holds a "bank number"
//    M=[0..3]. (We introduced the bank number to grow the number of channels from 50 to 200.) Channel number is 
//    computed as P = M*50 + N - CX_SPIKESORTREC_FIRST.
//
//    If a given cell A{P} in the cell array is NULL, then that means there is no data on sorted spike train channel 
//    number P. Otherwise, the cell will hold a 1xK double array containing the K spike arrival times on that channel
//    in chronological order, relative to a starting time of 0. All times are assumed to be in milliseconds but with
//    10us accuracy. This is important, since the arrival times are converted to **interspike intervals** in units of
//    10us ticks and stored as integers in the file record.
//
//    The method also assumes that the temporary file is already open and the file pointer is at the right location for 
//    storing the sorted spike train records, and it assumes that the cell array is properly formatted as described.
//
//    ARGS: pChannels -- [in] a 1xNUMSPIKESORTCH cell array containing the sorted spike train channel data to be 
//          stored in the temporary file in the appropriate format. If NULL or empty, then there is no spike sort data
//          to write.
//
//    RETURNS: TRUE if successful; FALSE if a file write error occurs.
//
BOOL writeSortedSpikes( const mxArray* pChannels )
{
   int i, j, k, n, nInt, bankSize;
   mxArray* pSpikeCh;                                                   // MEX array of spike times for one channel
   int nSpikes;                                                         // # of spikes in a particular channel
   double* pdTimes;                                                     // ptr to the array of arrival times in ms
   long prevT, currT;                                                   // to convert arrival times to interspike intvs 
   CXFILEREC fileRec;                                                   // a spike train record

   if( pChannels == NULL || mxIsEmpty(pChannels) ) return( TRUE );      // there's nothing to do!

   nInt = sizeof(int);                                                  // in case we have to endian-swap
   bankSize = (int) (CX_SPIKESORTREC_LAST-CX_SPIKESORTREC_FIRST + 1);

   for( i=0; i<8; i++ ) fileRec.idTag[i] = 0;                           // init header portion of spike sort record 
   fileRec.idTag[0] = CX_SPIKESORTREC_FIRST;

   for( i=0; i<NUMSPIKESORTCH; i++ )                                    // for each supported spike sort channel:
   {
      pSpikeCh = mxGetCell(pChannels, i);                               //    get corres. element in cell array 
      if( pSpikeCh == NULL || mxIsEmpty(pSpikeCh) ) continue;           //    skip channel if there's no data

      nSpikes = mxGetNumberOfElements(pSpikeCh);                        //    get the data (a 1xN double array)
      pdTimes = mxGetPr(pSpikeCh);

      // from channel number I compute the bank number M and tag number N such I = M*B + N - L, where
      // L = CX_SPIKESORTREC_FIRST and B = (CX_SPIKESORTREC_LAST - CX_SPIKESORTREC_FIRST + 1)
      fileRec.idTag[1] = (i / bankSize);
      fileRec.idTag[0] = (i % bankSize) + CX_SPIKESORTREC_FIRST;

      prevT = 0;
      j = 0;                                                            //    write one or more spike train records as 
      while( j < nSpikes )                                              //    needed 
      { 
         n = (nSpikes-j > CX_RECORDINTS) ? CX_RECORDINTS : nSpikes-j;
         for( k=0; k<n; k++ )
         {
            currT = (long) (pdTimes[j+k] * 100.0);                      //    convert arrival times in ms to interspike 
            if(pdTimes[j+k]*100.0 - ((double)currT) >= 0.5) ++currT;
            fileRec.u.iData[k] = (int) (currT-prevT);                   //    intervals in 10us ticks
            prevT = currT;
         }

         for( k=n; k<CX_RECORDINTS; k++ )                               //    pad partial record with end-of-data mark
            fileRec.u.iData[k] = EOD_EVENTRECORD;

         if( m_isBigEndian )                                            //    data file must be little-endian!  convert 
         {                                                              //    endianness before writing.
            for( k=0; k<CX_RECORDINTS; k++ )
               endianSwap( (BYTE*) &(fileRec.u.iData[k]), nInt );
         }

         if( fwrite((VOID*) &fileRec, RECORDSZ, 1, m_pTmpFile) == 0 )
            return( FALSE );
         j += n;
      }
   }

   return( TRUE );
}

//=== readSpikewave =================================================================================================== 
//
//    Read compressed AI data in one CX_SPIKEWAVERECORD into an internal buffer. If we must reallocate the internal 
//    buffer because it is full, the routine fails if we're unable to do so.
//
//    ARGS:       pRec  -- [in] ptr to buffer holding a CX_SPIKEWAVERECORD data file record.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL readSpikewave(CXFILEREC* pRec)
{
   char* pcNewBuf;                                                         // ptr to reallocated buffer, if needed
   int iExtra = CX_RECORDBYTES * 20;                                       // if we must realloc, add 20 records' worth
   
   if(m_nFastBytes + CX_RECORDBYTES > m_nFastBufSz )                       // insufficient space. Reallocate buffer, 
   {                                                                       // aborting on failure.
      pcNewBuf = (char*) realloc( (void*) m_pcFastData, sizeof(char)*(iExtra + m_nFastBufSz) );
      if( pcNewBuf == NULL )
      {
         printf( "ERROR: Internal buffer reallocation failed!\n" );
         return( FALSE );
      }
      m_pcFastData = pcNewBuf;
      m_nFastBufSz += iExtra;
   }

   memcpy(m_pcFastData + m_nFastBytes, pRec->u.byteData, CX_RECORDBYTES);  // copy compressed AI data from record into
   m_nFastBytes += CX_RECORDBYTES;                                         // internal buf & update buffer count.

   return(TRUE);
}

//=== replaceSpikewave ================================================================================================ 
//
//    Helper method replaces the original 25KHz spike waveform, which is currently stored in compressed form in an 
//    internal buffer, by the waveform provided in the MEX array -- which will be a 1xN double matrix. It first 
//    uncompresses the original waveform and verifies that the new waveform has the same length. If not, the function 
//    fails, since EDITCXDATA does not allow changing the length of the spike waveform.
//
//    Otherwise, it compresses the new spike waveform using the same algorithm that Maestro uses, packing it into 
//    the internal buffer. Afterwards, global variable m_nFastBytes will be set to the exact number of bytes in the 
//    new compressed waveform. This will NOT necessarily equal the number of bytes in the original compressed waveform, 
//    so the relevant field in the data file header (CXFILEHDR.nSpikeBytesCompressed) should be updated accordingly!!
//
//    ARGS:       pSpikewave -- [in] the MEX array containing the new 25KHz spike waveform
//    RETURNS:    TRUE if successful; FALSE if new waveform is not the same length as the original, or some fatal 
//                error occurs. Error message is printed to STDOUT.
//
BOOL replaceSpikewave(const mxArray *pSpikewave)
{
   double* pdData;
   int i, nBytes, nLen;
   short shNext, shLast, shTemp;
   char* pcNewBuf;                                                         // ptr to reallocated buffer, if needed
   int iExtra = CX_RECORDBYTES * 20;                                       // if we must realloc, add 20 records' worth
   
   // if there was no spike waveform data in file, fail!
   if(m_nFastBytes == 0)
   {
      printf("ERROR: Cannot edit spike waveform data if original file has none!\n");
      return(FALSE);
   }

   // uncompress data into a temp array so we can check length of original waveform
   nBytes = 0;
   nLen = 0;
   pdData = (double*) malloc( sizeof(double) * m_nFastBytes ); 
   if(pdData == NULL)
   {
      printf("ERROR: Memory allocation failed while editing spike waveform data!\n");
      return(FALSE);
   }
   uncompressAIData(pdData, m_nFastBytes, m_pcFastData, m_nFastBytes, 1, &nBytes, &nLen);
   free(pdData);

   i = (int) mxGetNumberOfElements(pSpikewave);
   if(nLen != i)
   {
      printf("ERROR: The 'spikewave' field is not the same length (%d) as original spike waveform (%d)!\n", i, nLen);
      return(FALSE);
   }

   // compress the new spike waveform data into our internal buffer
   m_nFastBytes = 0;
   pdData = mxGetPr(pSpikewave);
   shLast = 0;
   for(i = 0; i < nLen; i++) 
   {
      shNext = (short) pdData[i];                                          // get next raw waveform sample, which MUST
      shNext = (shNext<-2048) ? -2048 : ((shNext>2047) ? 2047 : shNext);   // be in the range [-2048..2047]

      shTemp = shNext - shLast;                                            // save *difference* from previous sample
      shLast = shNext;                                                     // rem curr sample for next iteration

      if(shTemp >= -63 && shTemp <= 63)                                    // -63..63 ==> 0x01..0x7F: can save as a
      {                                                                    //  single byte, with bit7 = 0 always!
         shTemp += 64;
         m_pcFastData[m_nFastBytes++] = (char) shTemp;
      }
      else                                                                 // -2048..-64 ==> 0x8800..0x8FC0, and
      {                                                                    // 64..2047 ==> 0x9140..0x97FF. 2 bytes
         shTemp += 4096;                                                   // saved, with high byte first. Note that
         shTemp |= 0x8000;                                                 // high byte ALWAYS has bit7 = 1!!
         m_pcFastData[m_nFastBytes++] = (char) ((shTemp >> 8) & 0x00FF);
         m_pcFastData[m_nFastBytes++] = (char) (shTemp & 0x00FF);
      }

      // we MIGHT have to reallocate internal buffer if the new spike waveform data does not compress as 
      // compactly as the original waveform. If so and buffer realloc fails, abort
      if(m_nFastBytes + 5 > m_nFastBufSz)
      {
         pcNewBuf = (char*) realloc( (void*) m_pcFastData, sizeof(char)*(iExtra + m_nFastBufSz) );
         if(pcNewBuf == NULL)
         {
            printf( "ERROR: Internal buffer reallocation failed!\n" );
            return( FALSE );
         }
         m_pcFastData = pcNewBuf;
         m_nFastBufSz += iExtra;
      }
   }

   if(m_iVerbose)
      printf("Compressed %d-sample spike waveform into %d bytes.\n", nLen, m_nFastBytes);

   return(TRUE);
}

//=== uncompressAIData ================================================================================================
//
//    28nov06: COPIED FROM READCXDATA.C
//
//    Uncompress a CNTRLX analog input byte stream sampling the specified number N of AI channels.  When more than one
//    channel is recorded, the data are stored in the output buffer as [ch1(0), ..., chN(0), ch1(1), ..., ch1(N), ...].
//    Uncompressed data is in the range of a 12bit analog-to-digital converter: [-2048..2047].
//
//    Compression algorithm:  Each compressed sample represents the DIFFERENCE from the previous sample.  If this
//    difference is in [-63..63], it is encoded as a single byte in [0x01..0x7F].  Observe that bit 7 is NOT set.  To
//    get back the sample, subtract 64 from the encoded byte, then add the result to the value of the last sample on
//    the current channel.  If the difference is in [-2048..-64, 64..2047], it is encoded as two bytes in the range
//    [0x8800..0x8FC0, 0x9040..0x97FF], with the high byte first.  In this case bit 7 is set in the high byte -- that's
//    how we distinguish between a one-byte and two-byte compressed datum.  To uncompress the two-byte datum, we pack
//    the two bytes into a 16bit int, clear bit 15, and subtract 4096 to recover the difference, which is then added
//    to the value of the last sample to get the current sample value.  OBSERVE that, if a given sample is compressed
//    as one byte, it will never have the value 0x00; for a 2-byte compressed sample, the high byte is never 0x00.  In
//    fact, CNTRLX uses the zero byte to mark the end of the compressed data stream.  We stop as soon as we reach this
//    end-of-stream marker -- thus we will know exactly how many bytes were compressed and how many scans of real data
//    were saved.
//
//    20jan2004: BUG FIX.  When the # of compressed bytes is an integer multiple of CX_RECORDBYTES, there is no "end of
//    data" marker (the zero byte) in the compressed data buffer.  Prior to this fix, this method would abort
//    before incrementing the scan counter for the last scan's worth of data.  As a result, readcxdata() warned that
//    there was a mismatch between the #scans reported in the header and the actual number of scans read from file.
//
//    08jul2004: Cntrlx uses the byte value 0xFF rather than 0 as the "endOfData" mark in Continuous mode files, while
//    it uses 0 for Trial mode files.  Maestro uses 0 as the "endOfData" mark always.  To decompress Cntrlx-generated
//    Continuous mode files properly, we check for the presence of either of these markers.  The compression algorithm
//    guarantees that 0xFF will never appear as the value of a 1-byte compressed sample, nor as the first byte of a
//    2-byte compressed sample.
//
//    ARGS:       pDst     -- [out] pre-allocated buffer to hold the uncompressed data stream in "channel-scan order".
//                iDstSz   -- [in] total # of samples that can be stored in uncompressed data buffer.
//                pSrc     -- [in] compressed data stream buffer.
//                iSrcSz   -- [in] size of compressed data buffer.
//                nCh      -- [in] # of AI channels that were recorded.
//                pNC      -- [out] total # of compressed bytes found (zero byte marks end of stream!)
//                pNScans  -- [out] total # of complete scans found in uncompressed data stream.  The total # of
//                            samples is this # times the # of channels recorded.
//
//    RETURNS:    NONE.
//
void uncompressAIData( double* pDst, int iDstSz, char* pSrc, int iSrcSz, int nCh, int* pNC, int* pNScans )
{
   int i;
   int iLastSample[CXH_MAXAI];
   char cByte;
   short shTemp;
   int nSrc;
   int nScans;

   memset( iLastSample, 0, CXH_MAXAI*sizeof(int) );                     // all channels read 0 at t = 0!

   nScans = 0;                                                          // # of complete channel scans found
   nSrc = 0;                                                            // # of compressed bytes processed

   while( nSrc < iSrcSz )                                               // uncompress the data stream
   {
      if( (nScans+1)*nCh > iDstSz )                                     //    oops, not enough room left in output buf
      {                                                                 //    for the next scan's worth of samples
         *pNC = nSrc;
         *pNScans = nScans;
         return;
      }

      for( i = 0; i < nCh; i++ )                                        //    do one channel scan's worth at a time
      {
         if( nSrc == iSrcSz || pSrc[nSrc] == 0 || pSrc[nSrc] == -1 )    //    oops, we've either hit end of input
         {                                                              //    buffer or got "endOfData" mark
            *pNC = nSrc;
            *pNScans = nScans;
            return;
         }

         cByte = pSrc[nSrc++];                                          //    read in the next byte
         if( cByte & 0x080 )                                            //    if bit7 set, next datum is 2 bytes
         {
            if( nSrc == iSrcSz )                                        //    should NEVER happen, but just in case...
            {
               *pNC = nSrc;
               *pNScans = nScans;
               return;
            }
            shTemp = (cByte & 0x7F);
            shTemp <<= 8;
            shTemp |= 0x00FF & ((short) pSrc[nSrc++]);
            shTemp -= 4096;
            iLastSample[i] += shTemp;                                   //    datum is difference from last sample!
         }
         else                                                           //    if bit 7 clear, next datum is 1 byte
         {
            shTemp = cByte - 64;
            iLastSample[i] += shTemp;
         }

         pDst[nScans*nCh+i] = (double)iLastSample[i];                   //    save uncompressed sample in output buf
      }
      ++nScans;
   }

   *pNC = nSrc;
   *pNScans = nScans;
}


//=== writeSpikewave ================================================================================================== 
//
//    Writes the new compressed 25KHz spike waveform from the internal buffer to the temporary file. It assumes that 
//    the temporary file is already open and the file pointer is at the right location, and it assumes that the 
//    internal spike waveform buffer has been filled properly with the compressed spike waveform.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE if a file write error occurs.
//
BOOL writeSpikewave()
{
   int i;
   BOOL bOk;
   CXFILEREC fileRec; 
   BYTE* pBytes;

   memset( (VOID*) &fileRec, 0, sizeof(CXFILEREC) ); 
   fileRec.idTag[0] = CX_SPIKEWAVERECORD;
   i = 0;
   bOk = TRUE;
   while(bOk && (i+CX_RECORDBYTES < m_nFastBytes))                      // first write all the full records
   {
      memcpy( (VOID*) &(fileRec.u.byteData[0]), (VOID*) &(m_pcFastData[i]), CX_RECORDBYTES );
      if( fwrite((VOID*) &fileRec, RECORDSZ, 1, m_pTmpFile) == 0 )
         bOk = FALSE;
      i += CX_RECORDBYTES;
   }

   if(bOk && (i < m_nFastBytes))                                        // ...then the last "partial" record
   {
      memset( (VOID*) &fileRec, 0, sizeof(CXFILEREC) );                 // any unused space is filled w/ zeros
      fileRec.idTag[0] = CX_SPIKEWAVERECORD;
      pBytes = &(fileRec.u.byteData[0]); 
      while( i < m_nFastBytes ) *pBytes++ = m_pcFastData[i++]; 

      if( fwrite((VOID*) &fileRec, RECORDSZ, 1, m_pTmpFile) == 0 )
         bOk = FALSE;
   }

   return(bOk);
}

