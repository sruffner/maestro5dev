//===================================================================================================================== 
//
// cxfilewriter.h : Declaration of class CCxFileWriter, which queues up 1KB data buffers for writing to a disk file in 
//                  a separate RTX thread.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 

#if !defined(CXFILEWRITER_H__INCLUDED_)
#define CXFILEWRITER_H__INCLUDED_

#include <windows.h>             // standard Win32 includes 
#include "rtapi.h"               // the RTX API 
#include "suspend.h"             // CRtSuspendMgr -- to manage CPU usage by the file IO thread


//===================================================================================================================== 
// Declaration of class CCxFileWriter
//===================================================================================================================== 
//
class CCxFileWriter
{
private: 
   static const int MAX_BLOCKS = 200;     // max# of blocks that can be allocated in the internal queue
   static const int BLOCKSIZE = 1024;     // #bytes in each data block written to file              


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   HANDLE            m_hFileWriterThrd;         // handle of thread in which file writing takes place
   CRtSuspendMgr     m_threadMgr;               // manages CPU usage by the file writer thread

   char              m_filePath[MAX_PATH];      // full pathname to open file
   HANDLE            m_hFile;                   // handle of open file (NULL when file writer not in use)

   char*             m_pBuffer;                 // circular queue for write blocks
   LONG              m_lFileLoc[MAX_BLOCKS];    // corres queue of locations for writing each block (-1 = append)
   int               m_nBlocks;                 // total # of data blocks allocated in queue
   volatile int      m_viTopBlock;              // index of data block in queue currently being written to file
   volatile int      m_viBotBlock;              // index of first avail block after last write block in the queue

   BOOL              m_bWriteFailed;            // TRUE if a file write op failed; no further writes allowed

   BOOL              m_bPaused;                 // TRUE if file writer thread was suspended directly

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxFileWriter( const CCxFileWriter& src );            // no copy constructor or assignment operator defined
   CCxFileWriter& operator=( const CCxFileWriter& src ); 

public: 
   CCxFileWriter();                                      // constructor 
   ~CCxFileWriter() { FreeResources(); }                 // destructor 


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   BOOL RTFCNDCL IsOpen() { return( BOOL(m_hFile!=NULL) ); }   // is a file currently opened by the file writer?
   BOOL RTFCNDCL IsPending()                             // are any write blocks pending in queue? 
   { 
      return( BOOL(m_viTopBlock != m_viBotBlock) );
   }
   BOOL RTFCNDCL HasWriteFailed()                        // has a write error occurred?  must close file to clear.
   {
      return( m_bWriteFailed );
   }
   BOOL RTFCNDCL IsFull()                                // is internal write queue full?
   {
      if( !IsOpen() ) return( FALSE );
      return( BOOL( ((m_viBotBlock+1) % m_nBlocks) == m_viTopBlock ) );
   }


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:
   BOOL RTFCNDCL AllocateResources( ULONG ulRTXPri,      // allocate resources required by file writer object
      int iOnDurUS, int iOffDurUS, int iBlocks );
   VOID RTFCNDCL FreeResources();                        // release resources required by file writer object

   BOOL RTFCNDCL Open( LPCTSTR strPath );                // open specified file for writing
   BOOL RTFCNDCL Close( BOOL bSave = TRUE );             // close the currently open file, optionally deleting it
   BOOL RTFCNDCL Write( PVOID pBuf, LONG lLoc = -1 );    // queue data block for writing to file
   BOOL RTFCNDCL Flush();                                // flush all queued data blocks to file

   // directly suspend/resume file writer thread WITHOUT going through the suspend manager
   VOID RTFCNDCL Pause() 
   { 
      if(m_hFileWriterThrd != NULL && !m_bPaused) {::SuspendThread(m_hFileWriterThrd); m_bPaused = TRUE; }
   }
   VOID RTFCNDCL Resume()
   {
      if(m_hFileWriterThrd != NULL && m_bPaused) {::ResumeThread(m_hFileWriterThrd); m_bPaused = FALSE; }
   }

   BOOL RTFCNDCL Write( int& iLen, PVOID pBuf,           // this version retrieves #blocks currently pending
                        LONG lLoc = -1 )
   {
      iLen = m_viBotBlock - m_viTopBlock;
      if( iLen < 0 ) iLen += m_nBlocks;
      return( Write( pBuf, lLoc ) );
   }


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
private:
   static DWORD RTFCNDCL WriterEntry( PVOID pThisObj )   // static helper method invokes non-static thread procedure!
   {
      return( ((CCxFileWriter*)pThisObj)->Writer() );
   }
   DWORD RTFCNDCL Writer();                              // thread proc for the file writer thread

};


#endif   // !defined(CXFILEWRITER_H__INCLUDED_)
