//=====================================================================================================================
//
// rmvionet.h : Declaration of class CRMVIoNet, which implements the communication link between RMVideo and Maestro,
//    over a private TCP/IP network connection.
//
//=====================================================================================================================


#if !defined(RMVIONET_H_INCLUDED_)
#define RMVIONET_H_INCLUDED_

#include "rmvio.h"                                          // base class CRMVIo


class CRMVIoNet : public CRMVIo
{
public:
   CRMVIoNet();
   ~CRMVIoNet();

   bool init();                                             // init communication interface
   void cleanup();                                          // destroy all resources alloc'd to this comm interface
   bool openSession();                                      // open connection session with Maestro [BLOCKS]
   void closeSession();                                     // issue RMV_SIG_BYE, then close connex w/Maestro client

   int getNextCommand();                                    // get next command from Maestro, if any
   int getCommandArg(int pos);                              // to retrieve 32-bit int arguments of selected cmds

   int getNumTargets();                                     // get # of target defns accompanying "load targets" cmd
   bool getTarget(int iPos, RMVTGTDEF& tgt );               // retrieve a target record defined in "load targets" cmd
   bool getMotionVector(int iPos, RMVTGTVEC& vec);          // retrieve a target motion vector defined in the "start
                                                            // animating" and "update frame" commands
   bool isSyncFlashRequested();                             // start sync spot flash in this animation frame

   const char* getMediaFolder();                            // get media folder name from selected commands
   const char* getMediaFile();                              // get media file name from selected commands
   bool downloadFile(FILE* fd);                             // download a file over the communication interface

   void sendData(int len, int* pPayload);                   // send information back to Maestro

private:
   int m_args[3];                                           // args specified with last command, if applicable

   char m_strMediaFolder[RMV_MVF_LEN+1];                    // media folder name from last relevant cmd
   char m_strMediaFile[RMV_MVF_LEN+1];                      // media file name from last relevant cmd

   static const int DEF_TGTBUFSZ;                           // initial size for target buffers
   int m_iTgtBufSz;                                         // max #tgts, Nmax, that can be accommodated in target
                                                            // record and motion vector buffers
   RMVTGTDEF* m_pTargets;                                   // target record buffer

   RMVTGTVEC* m_pMotionVecs;                                // target motion vector buffer

   bool m_bEnumFrame0;                                      // flag set while frame0 motion vectors are retrieved

   int m_nTargets;                                          // number of tgts defined in last "load targets" command

   // true if last command processed was RMV_CMD_STARTANIMATE or _UPDATEFRAME and the sync spot flash was requested
   bool m_bSyncFlashRequested;

   int m_sessionSocket;                                     // active socket descriptor for receiving commands from
                                                            // and sending signals to Maestro

   static const int DEF_RAWBUFGROWSZ;                       // default and grow size for network receive buffer
   int m_iRcvBufSize;                                       // size of network receive buffer in bytes
   char* m_pRcvBuf;                                         // raw network byte buffer for receiving Maestro cmds
   int m_iRcvLenBytes;                                      // length of received command in bytes

   bool sessionInProgress()                                 // is there an active Maestro command session?
   {
      return( m_sessionSocket != -1 );
   }

   int pollSocketForCommand();                              // polls session socket for (count,body) command block
   int processNextCommand();                                // process a command just received, IAW CRMVIo contract
   bool parseLoadTargets();                                 // helper method parses the RMV_CMD_LOADTARGETS command
   bool parseStartAnimateFrame0();                          // parses frame 0 motion vectors from STARTANIMATE cmd
   bool parseStartAnimateFrame1();                          // parses frame 1 motion vectors from STARTANIMATE cmd
   bool parseUpdateFrame();                                 // parses motion vectors from RMV_CMD_UPDATEFRAME cmd
   bool parseMediaAndFileCommands();                        // parses media store & file-related commands
};


#endif // !defined(RMVIONET_H_INCLUDED_)
