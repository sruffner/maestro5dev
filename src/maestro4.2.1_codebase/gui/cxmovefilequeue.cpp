//=====================================================================================================================
//
// cxmovefilequeue.cpp : Implementation of class CCxMoveFileQueue, which queues "move file" operations and executes the
//                       operations in a separately spawned thread.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This utility class was developed solely to address the problem of writing Maestro data files across a network drive.
// With the advent of RTX5.1.1, it was no longer possible to write to a network drive from the RTX environment.  Thus,
// Maestro's Win32 GUI side had to take on that task.  Whenever the user specified a data file path on a remote or
// virtual drive, CXDRIVER would be supplied with the path to a local "shadow" destination file.  After CXDRIVER was
// finished with the file, Maestro would copy it to the remote location and delete the shadow file.  However, we have
// experienced significant delays with network file operations, even for modestly sized data files.  Maestro would
// "freeze" as it waited for the Win32 CopyFile() call to complete.
//
// To address this problem, CCxMoveFileQueue maintains a queue of these file move operations, which are then executed
// in a separate worker thread.
//
// ==> Usage.
// 1) Construct a CCxMoveFileQueue object and call Start() to initialize the queue and start the worker thread.  This
// thread merely sleeps when there are no file move operations pending. Note that, if Start() does not succeed or is
// never called, the CCxMoveFileQueue object is useless.
//
// 2) To queue a file move operation, call MoveFile() with the full pathnames of the source file and its destination.
// Note that CCxMoveFileQueue performs the "move" by copying the file, then deleting the source.  MoveFile() should
// take little time to execute, since it merely queues the job and returns.  The actual operation will take place after
// some indefinite delay that depends on how big the queue is and how often the worker thread gets access to the CPU.
//
// 3) If at any time a file move operation fails for whatever reason, the worker thread dies, the CCxMoveFileQueue
// object is disabled and no further operations are possible until Stop() and Start() are called in succession.  Any
// pending file operations are lost.  Call HasFailed() to determine if a failure occurred; GetErrorMessage() returns
// a short string describing the error.
//
// 4) To flush all pending jobs in the queue, call Flush().  This method should NOT be used in time-critical code
// sections, since it will "block" the calling thread until the worker thread has completely flushed the job queue.
// An argument to Flush() indicates the maximum wait time per file-move operation, so it will not block forever.
//
// 5) Stop(), after optionally calling Flush() to empty the job queue, kills the worker thread and releases all
// resources created when Start() was invoked.  This method also should NOT be called in time-critical code.  Even if
// Flush() is not called, the method will try to wait for the current file move operation to complete before stopping
// the worker thread.  Like Flush(), an argument to Stop() specifies a maximum wait per file-move operation.
//
// 6) If Start() or Stop() should ever fail, it is possible that the file mover worker thread has been left dangling.
// In that case, it is not safe to reuse the CCxMoveFileQueue object -- it is really a catastropic error.
//
// DEVNOTE:
// The file mover worker thread is configured to run at normal thread priority; that may need some tweaking if the
// Maestro GUI is dramatically impacted.
//
// REVISION HISTORY:
// 02aug2005-- Began development.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//=====================================================================================================================

#include "stdafx.h"                          // standard MFC stuff

#include "util.h"                            // for CElapsedTime
#include "cxmovefilequeue.h"



//=====================================================================================================================
// CONSTANTS INITIALIZED
//=====================================================================================================================

const int CCxMoveFileQueue::MAX_QUEUED        = 100;           // max# of file move operations that may be queued; if
                                                               // things get backed up this much, something is wrong!



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

CCxMoveFileQueue::CCxMoveFileQueue()
{
   Initialize();
}

CCxMoveFileQueue::~CCxMoveFileQueue()
{
   Stop( FALSE );       // blocks for current file move operation, but does not flush operations in queue
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== IsPending =======================================================================================================
//
//    Are any file move operations pending?
//
//    ARGS:       NONE
//    RETURNS:    TRUE if there's a file move operation in progress, or the job queue is not empty.
//
BOOL CCxMoveFileQueue::IsPending()
{
   return( m_nQueued > 0 || m_bMovingFile );                      // note: access not sync'd
}

//=== GetPercentFilled ----============================================================================================
//
//    Returns the percentage of the file mover's job queue that is currently in use.  If the queue maxes out, the file
//    mover fails.
//
//    ARGS:       NONE
//    RETURNS:    Percentage of file mover job queue currently in use (as a whole percent).
//
int CCxMoveFileQueue::GetPercentFilled()
{
   int n = m_nQueued;
   return( 100 * n / MAX_QUEUED );
}

//=== HasFailed =======================================================================================================
//
//    Has the file mover failed for whatever reason?
//
//    ARGS:       NONE
//    RETURNS:    TRUE if CCxMoveFileQueue has failed for some reason
//
BOOL CCxMoveFileQueue::HasFailed()
{
   return( m_bFailed );                                           // note: access not sync'd
}

//=== GetErrorMessage =================================================================================================
//
//    Retrieve error message after CCxMoveFileQueue has entered the "failed" state.
//
//    ARGS:       strMsg   -- [out] the error message is placed here. If file mover has not failed, this is set to an
//                            empty string.
//    RETURNS:    NONE.
//
VOID CCxMoveFileQueue::GetErrorMessage( CString& strMsg )
{
   strMsg.Empty();
   if( m_bFailed )
      strMsg = m_strError;                                        // note: access not sync'd
}

//=== Start ===========================================================================================================
//
//    Initialize and enable operation of this CCxMoveFileQueue.  This method creates and starts a worker thread that
//    services a file move job queue that is populated by calls to MoveFile().
//
//    ARGS:       NONE
//    RETURNS:    TRUE if successful; FALSE otherwise
//
BOOL CCxMoveFileQueue::Start()
{
   if( m_bAlive ) return( TRUE );                                    // we're already started!

   Initialize();
   m_pMoverThrd = ::AfxBeginThread( CCxMoveFileQueue::MoverEntry,    // start worker thread
                                    (LPVOID)this,                    // so we can call non-static thread proc
                                    THREAD_PRIORITY_NORMAL, 0,       // normal priority and inherit stack size
                                    0, NULL );                       // NOTE: thread obj is auto-deleted at termination
   if( m_pMoverThrd == NULL )
   {
      m_strError = _T("File mover could not spawn worker thread!" );
      return( FALSE );
   }

   // wait a short time for thread to start
   CElapsedTime eTime;
   while( eTime.Get() < 500000.0 && !m_bAlive ) ::Sleep(10);

   if( !m_bAlive )                                                   // this should never happen; if it does, it is
   {                                                                 // catastrophic -- the thread is left dangling!
      m_strError = _T("File mover could not spawn worker thread!");
      m_bFailed = TRUE;
      m_pMoverThrd = NULL;
      m_bDie = TRUE;
   }

   return( m_bAlive );
}

//=== Stop ============================================================================================================
//
//    Disable operation of this CCxMoveFileQueue, terminating the worker thread after optionally flushing any file
//    move operations pending in the job queue.  Even if the queue is not flushed, the method will still wait for the
//    worker thread to complete the file move operation currently in progress (if there is one).
//
//    If the worker thread is hung on a file op and fails to stop normally, it will be left dangling (in which case it
//    won't be released until the application exits).  After calling this method, Start() must be invoked to use the
//    CCxMoveFileQueue again.
//
//    ARGS:       iMaxWaitPerFile   -- [in] maximum time to wait for each individual file move op to finish (seconds)
//                bFlush            -- [in] if TRUE (the default), wait for all pending file move ops to finish; else
//                                     the method discards queue and only waits for the move op already in progress.
//    RETURNS:    TRUE if successful; FALSE if the worker thread failed to terminate.
//
BOOL CCxMoveFileQueue::Stop( int iMaxWaitPerFile, BOOL bFlush  /* = TRUE */ )
{
   if( !m_bAlive ) return( TRUE );                    // we're already stopped!

   if( !bFlush )                                      // if flush not requested, discard any jobs in queue before
      EmptyQueue();                                   // calling Flush()

   Flush( iMaxWaitPerFile );                          // wait until queue is flushed and current job is done --
                                                      // waiting a maximum number of seconds per job

   m_bDie = TRUE;                                     // tell worker thread to terminate.  Give it only a short time
   CElapsedTime eTime;                                // to die, since it should be idle at this point!
   while(m_bAlive && eTime.Get() < 1000000.0)
      ::Sleep(20);

   BOOL bOk = !m_bAlive;                              // success only if worker terminated normally
   if( !bOk )                                         // if not:
   {
      if( bFlush )                                    //    make sure queue emptied, since worker thrd may have hung
         EmptyQueue();                                //    before flushing queue
      m_pMoverThrd = NULL;                            //    worker thread is left dangling!
      m_bAlive = FALSE;
      if( !m_bFailed )                                //    leave error message in place
      {
         m_strError.Format( "File mover thread appears hung!" );
         m_bFailed = TRUE;
      }
   }
   else
      Initialize();

   return( bOk );
}

//=== Flush ===========================================================================================================
//
//    Blocks until all file move operations in the CCxMoveFileQueue have been completed (or an error occurs).
//
//    ARGS:       iMaxWaitPerFile   -- [in] maximum time to wait for each individual file move op to finish (seconds)
//    RETURNS:    TRUE if successful; FALSE otherwise (file mover disabled, timeout exceeded, or error occurred)
//
BOOL CCxMoveFileQueue::Flush( int iMaxWaitPerFile )
{
   if( !m_bAlive || HasFailed() ) return( FALSE );
   if( !IsPending() ) return( TRUE );

   if( iMaxWaitPerFile <= 0 ) iMaxWaitPerFile = 10;               // calculate maximum time we'll wait for flush
   double dWait = 1.0e6 * double(iMaxWaitPerFile);
   dWait *= double(m_nQueued + (m_bMovingFile ? 1:0));

   CElapsedTime eTime;
   while( IsPending() && !HasFailed() && (eTime.Get() < dWait))
      ::Sleep(100);

   return( !(IsPending() || HasFailed()) );
}

//=== MoveFile ========================================================================================================
//
//    Queue a new file move operation.  This method returns quickly, since it merely creates a job and appends it to
//    the current job queue.  If the source file does not exist, or the destination is unwritable, the error won't be
//    detected until the file move operation is attempted, at which point the file mover will register the failure and
//    stop working.
//
//    ARGS:       strDest  -- [in] full pathname of the desired destination.
//                strSrc   -- [in] full pathname of the source file to be moved.
//    RETURNS:    TRUE if successful; FALSE otherwise (file mover is not started or has failed, either path is NULL,
//                or there's an out of memory error)
//
BOOL CCxMoveFileQueue::MoveFile( LPCTSTR strDest, LPCTSTR strSrc )
{
   if( HasFailed() || !m_bAlive || strDest==NULL || strSrc==NULL )
      return( FALSE );
   return( QueueJob( strDest, strSrc ) );
}


//=== Initialize ======================================================================================================
//
//    Initialize state of this CCxMoveFileQueue object prior to starting the file mover thread.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxMoveFileQueue::Initialize()
{
   m_pMoverThrd = NULL;
   m_nQueued = 0;
   m_pTopOfQueue = NULL;
   m_pBotOfQueue = NULL;
   m_bFailed = FALSE;
   m_strError.Empty();
   m_bMovingFile = FALSE;
   m_bAlive = FALSE;
   m_bDie = FALSE;
}

//=== EmptyJobQueue ===================================================================================================
//
//    Empty this CCxMoveFileQueue object's internal job queue.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxMoveFileQueue::EmptyQueue()
{
   m_criticalSec.Lock();                                       // enforce sequential access to this code!
   while( m_nQueued > 0 )                                      // while queue not empty, get next job and delete it
   {
      CMoveJob* pJob = GetNextQueuedJob();
      DeleteJob( pJob );
   }
   m_criticalSec.Unlock();
}

//=== GetNextQueuedJob ================================================================================================
//
//    Pop next file mover job off the job queue.
//
//    ARGS:       NONE.
//    RETURNS:    Pointer to next job from the queue, or NULL if queue is empty.
//
CCxMoveFileQueue::CMoveJob* CCxMoveFileQueue::GetNextQueuedJob()
{
   m_criticalSec.Lock();

   CMoveJob* pJob = NULL;
   if( m_nQueued > 0 )
   {
      pJob = m_pTopOfQueue;                                 // pop job off top of queue
      m_pTopOfQueue = m_pTopOfQueue->pNextJob;
      pJob->pNextJob = NULL;
      if( m_pTopOfQueue == NULL )                           // queue now empty
         m_pBotOfQueue = NULL;
      --m_nQueued;
   }

   m_criticalSec.Unlock();
   return( pJob );
}

//=== DeleteJob =======================================================================================================
//
//    Release memory allocated for a job object by QueueJob().  The job must have been removed from queue already!
//
//    ARGS:       pJob  -- [in] ptr to job object to be deleted.
//    RETURNS:    NONE.
//
//
VOID CCxMoveFileQueue::DeleteJob( CMoveJob* pJob )
{
   if( pJob != NULL )
   {
      delete [] pJob->strFileDst;
      delete [] pJob->strFileSrc;
      delete pJob;
   }
}

//=== QueueJob ========================================================================================================
//
//    Create a new file mover job and push it onto job queue.
//
//    ARGS:       strDest  -- [in] full pathname of the desired destination
//                strSrc   -- [in] full pathname of the source file to be moved.
//    RETURNS:    TRUE if successful; FALSE if unable to allocate memory for the job object.
//
BOOL CCxMoveFileQueue::QueueJob( LPCTSTR strDest, LPCTSTR strSrc )
{
   CMoveJob* pJob = new CMoveJob;                           // allocate a new job object
   if( pJob == NULL )
      return( FALSE );

   // allocate buffers for and copy each path string. It's important to account for the terminating null!
   size_t dstLen = ::strlen(strDest) + 1;
   pJob->strFileDst = new char[dstLen];
   if( pJob->strFileDst == NULL )
   {
      delete pJob;
      return( FALSE );
   }
   ::strcpy_s(pJob->strFileDst, dstLen, strDest);

   size_t srcLen = ::strlen(strSrc) + 1;
   pJob->strFileSrc = new char[srcLen];
   if(pJob->strFileSrc == NULL)
   {
      delete[] pJob->strFileDst;
      delete pJob;
      return(FALSE);
   }
   ::strcpy_s(pJob->strFileSrc, srcLen, strSrc);

   pJob->pNextJob = NULL;                                   // since the new job will be the last in the queue!

   m_criticalSec.Lock();                                    // enforce sequential access to job queue

   if( m_pBotOfQueue != NULL )                              // append new job to bottom of queue
      m_pBotOfQueue->pNextJob = pJob;
   else                                                     // this is first job in queue!
      m_pTopOfQueue = pJob;
   m_pBotOfQueue = pJob;
   ++m_nQueued;

   m_criticalSec.Unlock();

   return( TRUE );
}

//=== Mover [thread procedure] ========================================================================================
//
//    ARGS:       NONE
//    RETURNS:    exit code (not used; returns 0 always)
//
UINT CCxMoveFileQueue::Mover()
{
   m_bAlive = TRUE;

   while( !m_bDie )                                                     // service file mover job queue until told to
   {                                                                    // stop...
      if( (!m_bFailed) && (m_nQueued > 0) )                             // once an error occurs, we stop servicing
      {                                                                 // queue, but we don't die.
         if( m_nQueued > CCxMoveFileQueue::MAX_QUEUED )                 // if queue has grown too big, we assume there
         {                                                              // is something very wrong, so we stop
            m_strError = _T("File mover queue overflow!");              // servicing the job queue.
            m_bFailed = TRUE;
         }
         else                                                           // execute next file move job in two steps,
         {                                                              // copy then delete. Then release job object.
            m_bMovingFile = TRUE;
            CMoveJob* pJob = GetNextQueuedJob();
            if( pJob != NULL )
            {
               if( !::CopyFile(pJob->strFileSrc, pJob->strFileDst, TRUE) )
               {
                  m_strError.Format( "File mover could not copy src file (0x%08x)", ::GetLastError() );
                  m_bFailed = TRUE;
               }

               if( (!m_bFailed) && !::DeleteFile(pJob->strFileSrc) )
               {
                  m_strError.Format( "File mover could not delete src file (0x%08x)", ::GetLastError() );
                  m_bFailed = TRUE;
               }

               DeleteJob( pJob );
            }
            m_bMovingFile = FALSE;
         }
      }

      ::Sleep(100);
   }

   m_bAlive = FALSE;
   return( 0 );
}

