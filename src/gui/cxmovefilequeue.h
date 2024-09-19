//=====================================================================================================================
//
// cxmovefilequeue.h : Declaration of class CCxMoveFileQueue, which queues file move operations for execution in a
//                     separate thread.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXMOVEFILEQUEUE_H__INCLUDED_)
#define CXMOVEFILEQUEUE_H__INCLUDED_

#include <afxmt.h>

//=====================================================================================================================
// Declaration of class CCxMoveFileQueue
//=====================================================================================================================
//
class CCxMoveFileQueue
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int MAX_QUEUED;                 // max# of file copy operations that may be queued

   struct CMoveJob                              // a move operation
   {
      char* strFileDst;                         //    full path of destination file
      char* strFileSrc;                         //    full path of source file
      CMoveJob* pNextJob;                       //    ptr to next move job (for queueing)
   };


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CWinThread*       m_pMoverThrd;              // the worker thread in which file move operations take place
   CCriticalSection  m_criticalSec;             // to mediate access to object's state between caller & worker thrds
   int               m_nQueued;                 // # operations queued
   CMoveJob*         m_pTopOfQueue;             // the first operation in the queue (not yet consumed by worker thrd)
   CMoveJob*         m_pBotOfQueue;             // the last operation in the queue
   BOOL              m_bFailed;                 // TRUE if file mover has failed; no further operations allowed
   CString           m_strError;                // error message if file mover failed

   BOOL              m_bMovingFile;             // TRUE while worker thread is executing a file move operation
   BOOL              m_bAlive;                  // TRUE while worker thread is alive
   BOOL              m_bDie;                    // this is set to tell worker thread to die

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxMoveFileQueue( const CCxMoveFileQueue& src );      // no copy constructor or assignment operator defined
   CCxMoveFileQueue& operator=( const CCxMoveFileQueue& src );

public:
   CCxMoveFileQueue();                                   // constructor
   ~CCxMoveFileQueue();                                  // destructor


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL IsPending();                                     // are any file move operations pending or in progress?
   int GetPercentFilled();                               // how full is the file mover queue?
   BOOL HasFailed();                                     // has the file mover failed for whatever reason?
   VOID GetErrorMessage( CString& strMsg );              // retrieve error message after a failure

   BOOL Start();                                         // start worker thread that will handle file move ops
   BOOL Stop( int iMaxWaitPerFile, BOOL bFlush = TRUE ); // stop worker thread, flushing any move ops if desired
   BOOL Flush( int iMaxWaitPerFile );                    // blocks until all queued file move ops are done
   BOOL MoveFile( LPCTSTR strDest, LPCTSTR strSrc );     // queue a new file move operation

private:
   VOID Initialize();                                    // initialize file mover prior to starting worker thread
   VOID EmptyQueue();                                    // empty job queue
   CMoveJob* GetNextQueuedJob();                         // pop next job off job queue
   VOID DeleteJob( CMoveJob* pJob );                     // release memory allocated for a job object
   BOOL QueueJob( LPCTSTR strDest, LPCTSTR strSrc );     // create a new file move job and push it onto job queue

   static UINT MoverEntry( PVOID pThisObj )              // static helper method invokes non-static thread procedure!
   {
      return( ((CCxMoveFileQueue*)pThisObj)->Mover() );
   }
   UINT Mover();                                         // worker thread procedure

};


#endif   // !defined(CXMOVEFILEQUEUE_H__INCLUDED_)
