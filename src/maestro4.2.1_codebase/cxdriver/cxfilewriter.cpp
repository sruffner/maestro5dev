//===================================================================================================================== 
//
// cxfilewriter.cpp : Implementation of class CCxFileWriter, which queues up 1KB data buffers for writing to a disk 
//                    file in a separate RTX thread.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// When data is recorded in "continuous" mode, it must be streamed to disk file on the fly without impacting the 2ms 
// runtime duty cycle.  However, the RTX-supported Win32 call ::WriteFile() is NOT deterministic and NOT "real-time"; 
// it still relies on the NT OS kernel to perform the disk operation.  The ::WriteFile() method can take much more than 
// 2ms to complete, depending on the host machine's capabilities, the length of data to be written and the state of the 
// "lazy-flush" cache implemented by the NT kernel.  For example, test runs on a 600MHz Intel P3 machine found a wide 
// range of execution times for 1KB and 10KB writes:
// 
//    1KB:  min = ~22 us; max = ~11ms, avg ~= 37-116us.
//    10KB: min = ~68 us, max = ~112ms, avg ~= 1.4-1.8ms.
//
// According to Microsoft documentation, it seems likely that the calling thread spends much of its time in a "wait 
// state" during the longer ::WriteFile() calls.  In the Win32 environment, it is possible to implement asynchronous 
// file writes with ::WriteFile(), but this is not supported in RTX.
//
// Clearly, we cannot use ::WriteFile() directly in ContMode.  CCxFileWriter provides a solution to this data streaming 
// problem.  It implements an internal queue of 1KB data buffers, and provides a separate "file writer" thread in 
// which the actual write operations occur.  By setting the priority of this thread lower than that of the main 
// CXDRIVER thread, we can be assured that file write operations do not interfere with ContMode runtime work.  In 
// addition, the internal queue provides a buffer against the occasional "long" execution times of ::WriteFile().
//
// CCxFileWriter uses a "thread suspend manager" (CRtSuspendMgr) to control CPU usage by the file writer thread.  When 
// resources are allocated to the file writer object, we specify the desired duty cycle and suspend duration for the 
// thread.  Setting these carefully is important to ensure that WinNT is not starved by the thread, while still giving 
// the thread sufficient CPU time to do its work.
//
// ==> Usage.
// 1) Construct a CCxFileWriter object and call AllocateResources() to allocate memory for the internal queue and to 
// start both the file writer thread and its associated suspend manager object. Specify the queue size in # of 1KB 
// data "blocks", the RTX priority of the file writer thread, and the active/suspended phase durations assigned to that 
// thread. Typically, set the priority of the file writer thread less than that of the calling thread. If it is 
// greater, then the file writer thread will dominate the calling thread. Note that, if AllocateResources() does not 
// succeed or is never called, the CCxFileWriter object will do nothing.
//
// 2) To start writing to a file, call Open() to open the file and initialize the file writer.  The specified file 
// must not already exist, or Open() will fail. If Open() succeeds, you can queue each 1KB data block to file by 
// invoking the Write() method.  The data buffer supplied to Write() must be exactly 1KB in size.  It can be reused by 
// the caller, since Write() merely copies the data into the internal queue.  Write() takes very little time to 
// execute and is appropriate for use in real-time code.  If used correctly, it will fail for two possible reasons: 
// a previous write operation failed (which effectively disables the file writer), or the internal queue is full.  Call 
// HasWriteFailed() to distinguish between these two possibilities. 
//
// While CCxFileWriter is best used to write data sequentially to file, Write() permits a data block to be written to 
// any valid location in the file (specified as a byte offset from the beginning of the file). If no location is 
// specified, the data block is always appended to the end of the file.
//
// 3) To flush all pending data blocks in the queue to file, call Flush().  This method should NOT be used in time-
// critical code sections, since it will "sleep" in the calling thread until the file writer thread has completely 
// flushed the queue.
//
// 4) Close() closes the file and optionally deletes it.  When there's no file opened by CCxFileWriter, the file 
// writer thread is left in a suspended state -- so it has very little impact on CPU usage within the application.
//
// 5) Call FreeResources() to release the resources previously created by AllocateResources(). It is also possible 
// to call AllocateResources() a second time to change the size of the internal queue, or the priority and/or 
// suspend duty cycle of the file writer thread. 
//
// ==> Limitations.
// 1) Resources used:  memory allocated to the internal queue, up to 200KB; two threads -- the file writer thread and 
// the timer thread that will be created by the file writer's thread suspend manager.
// 2) Only for writing to files in 1KB chunks.  This is tailored specifically to its intended usage in CXDRIVER, since 
// CXDRIVER data files are organized as 1KB "records".  Does not support file reads, and can open only one file at a 
// time.  Can write data blocks to non-sequential locations, but is best suited to sequential streaming...
// 3) Does NOT open files that already exist!
//
// ==> Test results.
// The test involved a simple simulation of CXDRIVER's ContMode runtime loop.  The primary thread would sleep for 
// ~400us during every 2ms interval.  At N-ms intervals (N a multiple of 2) it prepared a 1KB buffer for writing to 
// file via CCxFileWriter::Write().  The file writer object was allocated a 50-block queue, and the file writer thread 
// was suspended for 8ms of every 10ms interval -- given the CPU usage by the primary thread, this means that the file 
// writer thread will be active for only 400us of every 10ms interval!  A total of 10000 blocks (10MB) were written. 
// Under these conditions, on a 600MHz Intel Pentium III machine w/ 256MB RAM, we got the following results:
//
//       N = 2ms  ==> Internal queue overflowed within the first 1MB of the data stream.
//       N = 4ms  ==> Ditto.
//       N = 6ms  ==> Ditto, except that queue did not overflow until the second MB of data.
//       N = 8ms  ==> Success.  The max # of blocks pending in queue during the streaming operation was 33.
//       N = 10ms ==> Success.  Max # blocks pending = 15.
//       N = 14ms ==> Success.  Max # blocks pending = 8.
//       N = 20ms ==> Success.  Max # blocks pending = 4.
//
// The most demanding data streaming in CXDRIVER's ContMode occurs when the high-resolution spike train is being 
// recorded -- a worst-case rate of 1KB per 20ms is possible.  Based on these test results, CCxFileWriter should be 
// able to handle that load.
//
// Of course, a lot of factors may affect these results, such as speed of the hard disk, level of fragmentation, etc.
// Also, beware of resident programs that might access the disk on occasion -- such as antivirus programs and the 
// Microsoft "FindFast" tool (FINDFAST.EXE).  FindFast will definitely impact the performance of CCxFileWriter 
// (actually observed this during testing!); its automatic indexing feature should be disabled.
//
// CREDITS:
//
//
// REVISION HISTORY:
// 09aug2002-- Began development.
// 12aug2002-- Completed first version.  Testing completed.
// 13sep2002-- Modified to allow repositioning the file pointer when a 1KB chunk is written.  This was introduced 
//             merely to accommodate rewriting the CNTRLX data file header record at the end of a recording.
// 16sep2002-- Fixed bug in the implementation of the internal queue.  Also found a subtle problem with Flush(); for 
//             some unknown reason, using "i64Sleep.LowPart = N" instead of "i64Sleep.QuadPart = N" resulted in a very 
//             long sleep interval in ::RtSleepFt().
// 15oct2002-- Fix to Close() when not saving file.  We use Flush() whether we're saving the file or not, because it 
//             makes sure the (higher-priority) calling thread sleeps most of the time.  When not saving the file, we 
//             adjust the block pointers so that only the current data block write is finished.
// 27jan2012-- Modified IAW changes to CRtSuspendMgr. Suspend duty cycle timing is now specified as "on" (thread
// active) and "off" (thread suspended) phase durations.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10 / RTX64 build.
//===================================================================================================================== 

#include "cxfilewriter.h"


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

CCxFileWriter::CCxFileWriter() 
{
   m_hFileWriterThrd = NULL;

   memset( m_filePath, 0, MAX_PATH );
   m_hFile = NULL;

   m_pBuffer = NULL;
   for( int i = 0; i < MAX_BLOCKS; i++ ) m_lFileLoc[i] = -1; 
   m_nBlocks = 0;
   m_viTopBlock = 0;
   m_viBotBlock = 0;

   m_bWriteFailed = FALSE;
   m_bPaused = TRUE;
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

/**
 Allocate the system resources required by the file writer object: a separate RTX thread in which all file write 
 operations will take place, a thread suspend manager that ensures the file writer thread does not hog the CPU, and an 
 internal buffer for cacheing write blocks when the file writer thread is busy. 

 @param ulRTXPri RTX priority to be assigned to the file writer thread.
 @param iOnDurUS Duration of "on" (thread active) phase of suspend duty cycle for file writer thread, in microsecs.
 @param iOffDurUS Duration of "off" (suspended) phase of suspend duty cycle for file writer thread, in microsecs.
 @param iBlocks Number of 1KB blocks allocated for internal buffering of written data.
 @return TRUE if successful; FALSE otherwise.
*/
BOOL RTFCNDCL CCxFileWriter::AllocateResources(ULONG ulRTXPri, int iOnDurUS, int iOffDurUS, int iBlocks)
{
   // free previously allocated resources, if any
   FreeResources();

   // create "file writer" thread in suspended state and set its RTX priority
   DWORD dwID; 
   m_hFileWriterThrd = ::CreateThread(NULL, 0, CCxFileWriter::WriterEntry, (LPVOID)this, CREATE_SUSPENDED, &dwID);
   BOOL bOk = BOOL(m_hFileWriterThrd != (HANDLE) NULL);
   if(bOk)
   {
      if(ulRTXPri < RT_PRIORITY_MIN || ulRTXPri > RT_PRIORITY_MAX) ulRTXPri = RT_PRIORITY_MIN;
      ::RtSetThreadPriority(m_hFileWriterThrd, ulRTXPri); 
   }

   // start suspend mgt of file writer thread using specified timing parameters. Suspend manager's timer thread gets
   // near-max priority.
   if(bOk) bOk = m_threadMgr.Start(m_hFileWriterThrd, RT_PRIORITY_MAX-1);
   if(bOk) bOk = m_threadMgr.ChangeTiming(iOnDurUS, iOffDurUS);

   // freeze file writer thread for now; not using it yet!
   m_threadMgr.Bypass(TRUE);

   // allocate memory for file writer queue
   if(bOk)
   {
      m_nBlocks = (iBlocks < 0 || iBlocks > MAX_BLOCKS) ? 50 : iBlocks;
      m_pBuffer = (char*) ::RtAllocateLocalMemory((ULONG) m_nBlocks*BLOCKSIZE);
      bOk = BOOL(m_pBuffer != NULL);
   }

   // free all resources on failure
   if(!bOk) FreeResources();
   return(bOk);
}

/**
 Free all system resources (background thread, suspend mgr, and cache) that were allocated by the file writer.
*/
VOID RTFCNDCL CCxFileWriter::FreeResources()
{
   if( IsOpen() ) Close();
   m_threadMgr.Stop();
   if( m_hFileWriterThrd )
   {
      ::TerminateThread( m_hFileWriterThrd, 0 );
      ::CloseHandle( m_hFileWriterThrd );
      m_hFileWriterThrd = NULL;
   }
   if( m_pBuffer )
   {
      ::RtFreeLocalMemory( (PVOID) m_pBuffer );
      m_pBuffer = NULL;
      m_nBlocks = 0;
   }
}


//=== Open ============================================================================================================ 
//
//    Open a new file for writing.
//
//    The file writer can only write to one file at a time, so this method will fail if a file is already open.  It 
//    also cannot open a file that already exists.  
//
//    ARGS:       strPath -- [in] fully qualified pathname of the file to be opened for writing.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CCxFileWriter::Open( LPCTSTR strPath )
{
   if( (m_hFileWriterThrd == NULL) || IsOpen() ||                    // file writer not initialized, or it is already 
       (strlen( strPath ) > MAX_PATH) )                              // in use, or pathname too long
      return( FALSE );

   m_hFile = ::CreateFile( strPath, GENERIC_WRITE, 0, NULL,          // attempt to open NEW file 
                     CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
   if( m_hFile == INVALID_HANDLE_VALUE )                             // failed -- file probably already exists
   {
      m_hFile = NULL;
      return( FALSE );
   }

   ::strcpy_s( m_filePath, strPath );                                // save path so we can delete file later if nec
   m_threadMgr.Resume();                                             // release file writer thread from suspended state 
   return( TRUE );
}


//=== Close =========================================================================================================== 
//
//    Close the currently opened file, optionally deleting it.  If the file is to be saved, any pending writes are 
//    flushed before closing the file.  If any of these writes fail, we assume the file is corrupted and delete it.
//
//    Do NOT invoke this method in a time-critical section, as it BLOCKS waiting for all pending data writes to be 
//    flushed to the file.
//
//    ARGS:       bDelete  -- [in] TRUE if the file is to be saved; FALSE to delete it [default = TRUE].
//
//    RETURNS:    TRUE if successful; FALSE if a write operation failed during flush (unless file was to be deleted).
//
BOOL RTFCNDCL CCxFileWriter::Close( BOOL bSave /* =TRUE */ )
{
   if( !IsOpen() ) return( TRUE );                             // file is not open!

   if( !bSave )                                                // if NOT saving the file, let's only finish the 
   {                                                           // current block!
      m_threadMgr.Bypass( TRUE );
      if( IsPending() ) m_viBotBlock = (m_viTopBlock + 1) % m_nBlocks;
      m_threadMgr.Resume();
   }

   Flush();                                                    // flush all remaining data blocks in queue

   m_threadMgr.Bypass( TRUE );                                 // done with file writer thread for now -- suspend it
   m_viTopBlock = 0;                                           // reset queue pointers
   m_viBotBlock = 0;

   BOOL bOk = !(m_bWriteFailed && bSave);                      // fail if write error occurred and file is to be saved
   m_bWriteFailed = FALSE;                                     // clear the error flag

   ::CloseHandle( m_hFile );                                   // close the file
   m_hFile = NULL;

   if( (!bSave) || (!bOk) )                                    // delete file if it is not to be saved or an error 
      ::DeleteFile( m_filePath );                              // occurred during the final flush

   return( bOk );
}


//=== Write =========================================================================================================== 
//
//    Copy the specified data into the next available block in the internal write queue.  The data buffer must be 
//    exactly BLOCKSIZE bytes long, since CCxFileWriter only works with data blocks of this size.  Optionally specify 
//    a particular file location at which to write the block; otherwise, the block is always appended to the file.
//
//    Use this method for queueing a block of data to the open file in a time-critical manner.  Most of the execution 
//    time is devoted to the buffer copy, which is on the order of 1.1us on a 600MHz P3 machine.
//
//    ARGS:       pBuf  -- [in] ptr to the start of BLOCKSIZE-byte data buffer.
//                lLoc  -- [in] file location at which to write the block, specified as #bytes from start of file;
//                         -1 appends block to end of file (the default).
//
//    RETURNS:    TRUE if successful; FALSE if a previous write operation has failed, or internal queue is full.
//
BOOL RTFCNDCL CCxFileWriter::Write( PVOID pBuf, LONG lLoc /* = -1 */ )
{
   if( !IsOpen() || m_bWriteFailed ) return( FALSE );          // there's no file open, or a prev write op failed!

   int iNextBot = (m_viBotBlock+1) % m_nBlocks;                // fail if the queue is full!
   if( iNextBot == m_viTopBlock ) return( FALSE );

   char* pDest = &(m_pBuffer[m_viBotBlock*BLOCKSIZE]);         // copy the data to the next available block
   memcpy( pDest, pBuf, BLOCKSIZE );
   m_lFileLoc[m_viBotBlock] = lLoc;

   m_viBotBlock = iNextBot;                                    // update ptr to last block in queue
   return( TRUE );
}


//=== Flush =========================================================================================================== 
//
//    Flush the remaining queued data to the currently open file.  We "sleep" in the caller's thread until our file 
//    writer thread has emptied the queue.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE if a previous write operation failed, or a subsequent write fails.
//
BOOL RTFCNDCL CCxFileWriter::Flush()
{
   if( !IsOpen() || m_bWriteFailed ) return( FALSE );          // there's no file open, or a prev write op failed!

   int iOn, iOff;                                              // temporarily give most of CPU time to file writer 
   m_threadMgr.ChangeTiming(4000, 1000, &iOn, &iOff );         // thread so that it can expedite the flush

   LARGE_INTEGER i64Sleep;                                     // put caller's thread to sleep while we wait for file 
   i64Sleep.QuadPart = 5000;                                   // writer thread to flush queue; ~500us intv
   while( IsPending() && !m_bWriteFailed ) 
      ::RtSleepFt( &i64Sleep );

   m_threadMgr.ChangeTiming(iOn, iOff);                        // restore original suspend timing parameters

   return( !m_bWriteFailed );
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== Writer [thread procedure] ======================================================================================= 
//
//    The file writer's thread procedure.  This separate RTX thread merely services the circular queue of data, writing 
//    one block at a time to the open file until the queue is empty.  The thread never exits -- but it can be safely 
//    terminated once any pending writes have been completed and the data queue is empty.
//
//    The file writer permits writing each data block to a specified location in file -- see Write().  If no location 
//    is specified (loc = -1), the block is appended to the end of the file.  Note that, after every write, we return 
//    the file pointer to the current EOF -- this reflects the fact that CCxFileWriter is primarily intended for 
//    streaming data to file sequentially.
//
//    If any file operation fails, the thread will not attempt any more operations until the error flag is reset.  By 
//    design, the only way to reset this flag is to Close() the file.
//
//    IMPORTANT:  Multithread synchronization.  To avoid the overhead of the typical synchronization objects, we rely 
//    on a carefully structured use of the data members of the CCxFileWriter object.  Both the caller's thread and the 
//    file writer thread defined by this thread procedure have access to the CCxFileWriter object (see NOTE below). 
//    However, when active (that is, a file has been opened by the file writer obj), only the file writer thread can 
//    modify the top of the circular data queue; and only the caller's thread -- by invoking selected CCxFileWriter 
//    methods -- can change the bottom of the queue.  This ensures that the two threads do no interfere with each other 
//    or attempt to access the same data block within the queue.
//
//    NOTE: The thread entry point must be a static method, and static class methods do not get the implied THIS 
//    argument.  Thus, Writer() would not be able to access the non-static class members.  To get around this, the 
//    static inline method WriterEntry() instead serves as the thread entry point.  When spawned, it is passed a THIS 
//    ptr in its context argument, which it then can use to invoke the non-static Writer() method!
//     
//    ARGS:       NONE
//
//    RETURNS:    exit code (not used -- always 0)
//
DWORD RTFCNDCL CCxFileWriter::Writer()
{
   while( TRUE )
   {
      while( m_hFile == NULL ) ::Sleep( 0 );                            // nothing to do when a file is not open

      if( (!m_bWriteFailed) && (m_viTopBlock != m_viBotBlock) )         // keep writing blocks as long as no error has 
      {                                                                 // occurred 
         DWORD dwBytes = 0;
         char* pBuf = &(m_pBuffer[m_viTopBlock*BLOCKSIZE]);
         LONG lLoc = m_lFileLoc[m_viTopBlock];

         BOOL bOk = TRUE;
         if( lLoc >= 0 )
            bOk = BOOL(::SetFilePointer( m_hFile, lLoc, NULL, FILE_BEGIN ) != 0xFFFFFFFF);
         if( bOk ) bOk = ::WriteFile( m_hFile, pBuf, BLOCKSIZE, &dwBytes, NULL );
         bOk = bOk && (dwBytes == BLOCKSIZE);
         if( bOk && (lLoc >= 0) )
            bOk = BOOL(::SetFilePointer( m_hFile, 0, NULL, FILE_END ) != 0xFFFFFFFF);

         if( bOk )
         {
            int i = (m_viTopBlock + 1) % m_nBlocks;
            m_viTopBlock = i;
         }
         else m_bWriteFailed = TRUE;
      }
   }

   return( 0 );
}


