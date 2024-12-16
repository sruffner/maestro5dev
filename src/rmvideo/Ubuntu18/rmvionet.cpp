//=====================================================================================================================
//
// rmvionet.cpp : Class CRMVIoNet, wwhich implements the communication link between RMVideo and Maestro, over a private
//    TCP/IP network connection.
//
// AUTHOR:  saruffner.
//
// DESCRIPTION:
// CRMVIoNet is an implementation of the abstract class CRMVIo that implements a rudimentary TCP/IP Ethernet comm link
// between Maestro and RMVideo.  It acts as the "server" in the typical "client/server" socket programming model.
// However, it serves only one client at a time.  Furthermore, the client and server's IP addresses are constants, as
// is the port # on which CRMVIoNet listens for a connection from Maestro.  It is assumed that there is a direct,
// private Ethernet connection between the Maestro and RMVideo workstations (in fact, the two IP addresses are only
// permitted on fully disconnected networks); this connection is solely for the purpose of Maestro/RMVideo
// communications.  This is extremely important to ensure that the comm link will be fast enough to support the
// rapid-fire messaging from Maestro during a target animation sequence.
//
// NOTE:  Ignoring network/host byte order stuff.  Sending integer command buffer and integer command length directly,
// casting buffers to char*...  We also assume 'int' = 4 bytes!
//
// REVISION HISTORY:
// 31jan2006-- Created.
// 07mar2006-- Mod IAW change in RMVIo contract:  Maestro will scale floating-point motion vector fields by 1.0e^6
//             rather than 1000.  Also added support for tgt param RMV_TGTDEF_SIGMA.
// 04apr2006-- RMVTGTDEF.fSigma is now a 2-element array, to support specification of elliptical and 1D Gaussian
//             windows.  Parsing of RMV_TGTDEF_SIGMA revised accordingly.
// 06apr2006-- Revised to support separate color specs for the grating cmpts of a plaid.  RMVTGTDEF.iRGBMean and
//             iRGBCon are now 2-element arrays.  Parsing of RMV_TGTDEF_RGBMEAN and _RGBCON revised accordingly.
// 24apr2006-- Revised to support new query command, RMV_CMD_GETCOLORRES, which reports RMVideo's color resolution 
//             (16bit or 24bit possible).
// 22jul2009-- Major changes. CRMVIo implementation no longer replies directly to any commands. The animation-related 
//             commands provide motion vectors for all loaded targets, so the 'bNoChange' argument has been removed
//             from getMotionVector(). Added support for switching video mode via new RMV_CMD_SETCURRVIDEOMODE, and 
//             removed support for deleted command RMV_CMD_GETCOLORRES.
// 29jul2009-- Adding support for RMV_CMD_GETMOVIEDIRS, _GETMOVIEFILES, _GETMOVIEINFO, _DELETEMOVIE, _PUTFILE***.
// 18aug2009-- Adding support for RMV_CMD_GETGAMMA, _SETGAMMA.
// 24aug2009-- Modified IAW same-dtd changes in CRMVIo.
// 25aug2009-- Modified IAW same-dtd changes in rmvideo_common.h.
// 04oct2016-- Renamed getMovie***() methods as getMedia***(), as well as other changes. The former RMVideo "movie
//             store" is now a "media store that can store image as well as video files. The commands related to the
//             media store have changed names although their integer values are the same: RMV_CMD_GETMEDIADIRS,
//             _GETMEDIAFILES, _GETMEDIAINFO, _DELETEMEDIA.
// 25sep2018-- Mods to implement sync spot flash feature. Handle new command RMV_CMD_SETSYNC and new "sync flag" 
//             included with the _STARTANIMATE and _UPDATEFRAME commands.
// 08may2019-- Updated parseLoadTargets() to handle new RMVTGTDEF parameters defining the new "flicker" feature.
// 11dec2024-- Updated parseLoadTargets() to handle new parameter RMVTGTDEF.fDotDisp.
//=====================================================================================================================

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "rmvionet.h"

const int CRMVIoNet::DEF_TGTBUFSZ = 5;                           // initial size for target buffers
const int CRMVIoNet::DEF_RAWBUFGROWSZ = 2048;                    // default and grow size for network receive buffer

CRMVIoNet::CRMVIoNet()
{
   m_args[0] = -1;
   m_args[1] = -1;
   m_args[2] = -1;

   for(int i=0;i<RMV_MVF_LEN+1; i++)
   {
      m_strMediaFolder[i] = '\0';
      m_strMediaFile[i] = '\0';
   }

   m_iTgtBufSz = 0;
   m_pTargets = NULL;
   m_pMotionVecs = NULL;

   m_nTargets = 0;
   m_bEnumFrame0 = false;
   m_bSyncFlashRequested = false;

   m_sessionSocket = -1;

   m_pRcvBuf = NULL;
   m_iRcvBufSize = 0;
   m_iRcvLenBytes = 0;

}

CRMVIoNet::~CRMVIoNet()
{
   cleanup();
}

//=== init ============================================================================================================
//
//    Initialize the communication interface.
//
//    ARGS:       NONE.
//    RETURNS:    True if successful; false if unable to setup resources for the communication interface.
//
bool CRMVIoNet::init()
{
   // just in case!
   cleanup();

   // allocate raw network byte buffer
   m_pRcvBuf = (char*) malloc( DEF_RAWBUFGROWSZ*sizeof(char) );
   bool bOk = (m_pRcvBuf != NULL);
   if( bOk )
      m_iRcvBufSize = DEF_RAWBUFGROWSZ;

   // allocate target definition records and target motion vectors
   if( bOk )
   {
      m_pTargets = (RMVTGTDEF*) malloc( DEF_TGTBUFSZ * sizeof(RMVTGTDEF) );
      bOk = (m_pTargets != NULL);
      if( bOk )
      {
         m_pMotionVecs = (RMVTGTVEC*) malloc( DEF_TGTBUFSZ * sizeof(RMVTGTVEC) );
         bOk = (m_pMotionVecs != NULL);
         if( bOk )
            m_iTgtBufSz = DEF_TGTBUFSZ;
      }
   }

   // if an allocation failed, then release everything
   if( !bOk )
      cleanup();

   return( bOk );
}

//=== cleanup =========================================================================================================
//
//    Destroy the Maestro-RMVideo communication interface, releasing any allocated resources.
//
//    We make sure that the active session socket, if any, is closed; then release allocated memory
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
void CRMVIoNet::cleanup()
{
   closeSession();

   if( m_pMotionVecs != NULL )
   {
      free( (void*) m_pMotionVecs );
      m_pMotionVecs = NULL;
   }
   if( m_pTargets != NULL )
   {
      free( (void*) m_pTargets );
      m_pTargets = NULL;
   }
   if( m_pRcvBuf != NULL )
   {
      free( (void*) m_pRcvBuf );
      m_pRcvBuf = NULL;
   }

   m_iTgtBufSz = 0;
   m_nTargets = 0;
   m_bEnumFrame0 = false;
   m_bSyncFlashRequested = false;
   m_pRcvBuf = NULL;
   m_iRcvBufSize = 0;
   m_iRcvLenBytes = 0;
}

//=== openSession [BLOCKS!] ===========================================================================================
//
//    Open a command session with Maestro, BLOCKING until a Maestro client makes a connection and sends the
//    RMV_CMD_STARTINGUP command.
//
//    We establish a non-blocking listen socket on our dedicated IP address and port number.  We then poll it once a
//    second for an incoming connection.  This continues ad-infinitum until a connection attempt is accepted.  If the
//    connection comes from the dedicated IP address for our Maestro client, we set up the session socket as
//    non-blocking, with the Nagle algorithm disabled (for performance reasons).  We then wait for the official
//    "starting up" command from Maestro, at which point the method returns successfully and the "command session"
//    begins.  If any serious error occurs, an appropriate message is printed to stderr and the method fails -- in
//    which case RMVideo itself will exit.
//
//    ARGS:       NONE.
//    RETURNS:    True if Maestro command session established; false if an error occurred.
//
bool CRMVIoNet::openSession()
{
   // session already open!
   if( sessionInProgress() ) return( true );

   // set up non-blocking listen socket on our dedicated IP address and port.  We'll only accept ONE connection!
   // NOTE:  Since we reuse the same IP address and port# always, we enable SO_REUSEADDR in case a socket is still
   // bound to them, stuck in the TIME_WAIT state!
   int listenSocket = socket(PF_INET, SOCK_STREAM, 0);
   if( listenSocket < 0 )
   {
      perror("RMVideo(IONet) socket");
      return(false);
   }

   int res = fcntl(listenSocket, F_GETFL, 0);
   if( res >= 0 )
      res = fcntl(listenSocket, F_SETFL, res | O_NONBLOCK);
   if( res < 0 )
   {
      perror("RMVideo(IONet) fcntl");
      close(listenSocket);
      return(false);
   }

   int enable = 1;
   if( 0 > setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) )
   {
      perror("RMVideo(IONet) setsockopt");
      close(listenSocket);
      return(false);
   }

   struct sockaddr_in listenAddr;
   memset( &listenAddr, 0, sizeof(struct sockaddr_in) );
   listenAddr.sin_family = AF_INET;
   listenAddr.sin_port = htons(RMVNET_RMVPORT);
   listenAddr.sin_addr.s_addr = inet_addr(RMVNET_RMVADDR);
   if( 0 > bind(listenSocket, (struct sockaddr *) &listenAddr, sizeof(struct sockaddr)) )
   {
      perror("RMVideo(IONet) bind");
      close(listenSocket);
      return(false);
   }

   if( 0 > listen(listenSocket, 1) )
   {
      perror("RMVideo(IONet) listen");
      close(listenSocket);
      return(false);
   }

   // now poll for a connection attempt on the established listening socket.  We poll every 1sec until we get a
   // connection.
   int gotSocket = -1;
   struct sockaddr_in clientAddr;
   socklen_t clientAddrLen = (socklen_t) sizeof(struct sockaddr_in);
   int DBG_tries = 0;
   while( gotSocket < 0 )
   {
      gotSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrLen);
      ++DBG_tries;
      if( gotSocket < 0 )
      {
         // accept() failed for a reason other than EWOULDBLOCK.  This is fatal!
         if( errno != EWOULDBLOCK )
         {
            perror("RMVideo(IONet) accept");
            fprintf(stderr, "DBG: Failed after %d tries; errno = %d\n", DBG_tries, errno);
            close(listenSocket);
            return(false);
         }
      }

      sleep(1);
   }

   // ok, we've established a connection.  Close our listening socket.  Then verify that the client's IP address is
   // what we expect.  If not, fail.
   close(listenSocket);
   if( strcmp(RMVNET_MAESTROADDR, inet_ntoa(clientAddr.sin_addr)) != 0 )
   {
      fprintf( stderr, "RMVideo(IONet): Got connection from unexpected host (%s)\n", inet_ntoa(clientAddr.sin_addr) );
      close(gotSocket);
      return(false);
   }

   // make our new socket non-blocking, and turn off Nagle's algorithm to ensure tiny packets are sent/rcvd as
   // quickly as possible
   res = fcntl(gotSocket, F_GETFL, 0);
   if( res >= 0 )
      res = fcntl(gotSocket, F_SETFL, res | O_NONBLOCK);
   if( res < 0 )
   {
      perror("RMVideo(IONet) fcntl");
      close(gotSocket);
      return(false);
   }

   enable = 1;
   if( 0 > setsockopt(gotSocket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) )
   {
      perror("RMVideo(IONet) setsockopt");
      close(gotSocket);
      return(false);
   }

   // the accepted socket becomes our session socket.  Now all we have to do is wait for the RMV_CMD_STARTINGUP command
   // from the Maestro client.  If this is NOT the first command we get, or if we do not get it within 10 seconds, then
   // something is VERY wrong.
   m_sessionSocket = gotSocket;
   int command = RMV_CMD_NONE;
   int nTries = 0;
   while( (command == RMV_CMD_NONE) && (nTries < 1000) )
   {
      command = getNextCommand();
      ++nTries;
      usleep(10000);
   }
   if( command != RMV_CMD_STARTINGUP )
   {
      fprintf(stderr, "RMVideo(IONet): Did not get 'starting up' message from Maestro client!\n");
      close(m_sessionSocket);
      m_sessionSocket = -1;
      return(false);
   }

   m_nTargets = 0;
   m_bEnumFrame0 = false;

   return( true );
}

//=== closeSession ====================================================================================================
//
//    Close a command session with Maestro.  RMVideo invokes this function is response to the RMV_CMD_SHUTTINGDN
//    command from Maestro.  The function should issue RMV_SIG_BYE to acknowlege the end of the command session, then
//    take whatever steps are necessary to close the actual connection.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
void CRMVIoNet::closeSession()
{
   // session is already closed!
   if( !sessionInProgress() ) return;

   // tell Maestro we're closing the connection on this side
   sendSignal(RMV_SIG_BYE);

   close(m_sessionSocket);
   m_sessionSocket = -1;
}

//=== getNextCommand ==================================================================================================
//
//    Get the next command from Maestro, if any.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID, RMV_CMD_NONE if no command is pending, or RMV_CMD_NONE-1 if there's a fatal
//                error in the communication interface (or if the comm link is not yet established).
//
int CRMVIoNet::getNextCommand()
{
   // just to be safe
   if( !sessionInProgress() ) return( RMV_CMD_NONE-1 );

   // poll socket connection for next (count,body) command block.  This will read a single command in one go IF the
   // entire block is ready and waiting.
   int cmd = pollSocketForCommand();

   // if a command was received, process it as needed.
   if( cmd > RMV_CMD_NONE )
      cmd = processNextCommand();

   return(cmd);
}

int CRMVIoNet::getCommandArg(int pos)
{
   int arg = (pos >= 0 && pos < 3) ? m_args[pos] : -1;
   return(arg);
}


//=== pollSocketForCommand ============================================================================================
//
//    This helper function polls the open session socket, reading in the next Maestro command if it is there -- in
//    piecemeal fashion if necessary.
//
//    Each Maestro command is essentially a sequence of N 32-bit integers.  This command must be sent "over the wire"
//    as a sequence of bytes.  On the Maestro end, this is done simply by treating the int32 command buffer as a byte
//    buffer containing M=N*4 bytes.  First, M is sent as a 4-byte count, followed by the M bytes of command data.
//    Most Maestro commands are relatively small, so it will typically be the case that the entire [count, body] block
//    of data will be ready for reading in one go.  However, we're not guaranteed of that.  This method handles the
//    details of reading in as much of the command block as possible WITHOUT BLOCKING.  It maintains state information
//    so that it can complete the process on subsequent invocations.
//
//    Once a complete command has been received on the CRMVIoNet side, the raw command byte buffer may be recast as an
//    array of 32-bit integers, thereby recovering the original command sequence.  The command length N is the raw
//    byte count M divided by 4.  The first integer in the sequence is returned as the command ID.
//
//    ***DEVNOTE:  In doing things this way, the ASSUMPTION is that byte-ordering on the Maestro workstation is the
//    same as that on the RMVideo workstation.  If this proves to be an invalid assumption, yikes!
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID, RMV_CMD_NONE if no command is pending, or RMV_CMD_NONE-1 if there's a fatal
//                error in the communication interface (or if the comm link is not yet established).
//
int CRMVIoNet::pollSocketForCommand()
{
   static bool bReset = true;                                  // flag is set to reset state vars below
   static bool bGotCount;                                      // this is set once we got the cmd byte count
   static int nBytesRemaining;                                 // #bytes still to get
   static char* pBufEnd;                                       // current position in the buffer of what's been rcvd

   // reset state, waiting for the beginning of a (count,body) command block
   if( bReset )
   {
      bReset = false;
      bGotCount = false;
      nBytesRemaining = RMVNET_CMDCNTSZ;
      m_iRcvLenBytes = 0;
      pBufEnd = (char*) &m_iRcvLenBytes;
   }

   int result = RMV_CMD_NONE;

   // receive all data that we can without blocking.  Thus, we can get the command byte count and the command body all
   // in one go whenever possible.
   bool bMoreData = true;
   while( bMoreData )
   {
      // read socket -- since it's non-blocking, this should return quickly.  Process data rcvd, if any.
      int nBytesReceived = recv(m_sessionSocket, pBufEnd, nBytesRemaining, 0);
      if(nBytesReceived < 0)
      {
         // ignore EWOULDBLOCK error, which means there's no data to receive.  All other errors considered fatal.
         bMoreData = false;
         if( errno != EWOULDBLOCK )
         {
            perror("RMVideo(IONet) recv");
            result = RMV_CMD_NONE-1;
            bReset = true;
         }
      }
      else if(nBytesReceived == 0)
      {
         // Maestro client unexpectedly shutdown connection on its end.  This is a fatal error.
         fprintf(stderr, "RMVideo(IONet): Maestro client closed TCP/IP connection unexpectedly!\n");
         result = RMV_CMD_NONE-1;
         bReset = true;
         bMoreData = false;
      }
      else if(nBytesReceived == nBytesRemaining)
      {
         // finished receiving either the command byte count or the command body itself
         if( !bGotCount )
         {
            // got the command byte count.  Make sure it's positive and a multiple of 4, then prepare to get the
            // command body.
            bGotCount = true;
            if( (m_iRcvLenBytes < 0) || (m_iRcvLenBytes % 4 != 0) )
            {
               fprintf(stderr, "RMVideo(IONet): Illegal Maestro command length (%d bytes)!\n", m_iRcvLenBytes);
               result = RMV_CMD_NONE-1;
               bReset = true;
               bMoreData = false;
            }
            else
            {
               // if our byte buffer is not big enough, reallocate it!
               if( m_iRcvLenBytes > m_iRcvBufSize )
               {
                  int iNewSize = ((m_iRcvLenBytes / DEF_RAWBUFGROWSZ) + 1) * DEF_RAWBUFGROWSZ;
                  char* pBiggerBuf = (char*) realloc( (void*)m_pRcvBuf, iNewSize*sizeof(char) );
                  if( pBiggerBuf == NULL )
                  {
                     // uh-oh, out of memory!  note that original buffer is still allocated
                     result = RMV_CMD_NONE-1;
                     bReset = true;
                     bMoreData = false;
                  }
                  else
                  {
                     m_pRcvBuf = pBiggerBuf;
                     m_iRcvBufSize = iNewSize;
                  }
               }

               // if everything is ok, get ready to read in command body
               if( !bReset )
               {
                  nBytesRemaining = m_iRcvLenBytes;
                  pBufEnd = m_pRcvBuf;
               }
            }
         }
         else
         {
            // got the command body, so we're done.  Return command ID, which is first element in command buffer
            // when it is recast as a 32-bit int array.
            int* pCommands = (int*) m_pRcvBuf;
            result = pCommands[0];
            bReset = true;
            bMoreData = false;
         }
      }
      else
      {
         // received only a portion of the command byte count or command body.  We'll call recv() once more just
         // in case there's now some more data available.
         nBytesRemaining -= nBytesReceived;
         pBufEnd += nBytesReceived;
      }
   }

   return( result );
}

//=== processNextCommand ==============================================================================================
//
//    Process a Maestro command just received IAW the CRMVIo contract (see CRMVIo for details).
//
//    If we detect an illegally formatted Maestro command, we immediately send an RMV_SIG_CMDERR to the Maestro
//    client and ignore the command.  RMV_CMD_NONE is returned in this case.
//
//    ARGS:       NONE.
//    RETURNS:    The ID of the command processed, or RMV_CMD_NONE if no further processing is required.
//
int CRMVIoNet::processNextCommand()
{
   int i;

   // reset integer command args that are relevant only to selected commands
   for(i=0; i<3; i++) m_args[i] = -1;
   
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;

   // reset special flag set when we parse the motion vectors for frame 0 of the RMV_CMD_STARTANIMATE command
   m_bEnumFrame0 = false;

   // reset sync spot flash request flag that may be set by RMV_CMD_STARTANIMATE or RMV_CMD_UPDATEFRAME
   m_bSyncFlashRequested = false;

   int cmd = pCmdBuf[0];
   bool bCmdErr = false;
   switch( pCmdBuf[0] )
   {
      case RMV_CMD_SETBKGCOLOR :
         // first integer after command ID is the new bkg color: xBGR (byte3..byte0)
         if(iCmdLen != 2)
            bCmdErr = true;
         else
            m_args[0] = 0x00FFFFFF & pCmdBuf[1];
         break;

      case RMV_CMD_SETGEOMETRY :
         // three integers after command ID are geometry params W, H, and D in millimeters
         if(iCmdLen != 4)
            bCmdErr = true;
         else
         {
            for(i=0; i<3; i++)
               m_args[i] = pCmdBuf[i+1];
         }
         break;

      case RMV_CMD_SETSYNC :
         // two integers after command ID are the sync flash spot size in mm, and flash dur in # frames
         if(iCmdLen != 3)
            bCmdErr = true;
         else
         {
            for(i=0; i<2; i++) m_args[i] = pCmdBuf[i+1];
         }
	 break;

      case RMV_CMD_SETCURRVIDEOMODE :
         if(iCmdLen != 2)
            bCmdErr = true;
         else
            m_args[0] = pCmdBuf[1];
         break;

      case RMV_CMD_SETGAMMA :
         // three integers after command ID are gamma correction factors for R, G, B guns
         if(iCmdLen != 4)
            bCmdErr = true;
         else
         {
            for(i=0; i<3; i++)
               m_args[i] = pCmdBuf[i+1];
         }
         break;

      case RMV_CMD_GETVERSION :
      case RMV_CMD_RESTART :
      case RMV_CMD_EXIT :
      case RMV_CMD_STARTINGUP :
      case RMV_CMD_SHUTTINGDN :
      case RMV_CMD_GETCURRVIDEOMODE :
      case RMV_CMD_GETALLVIDEOMODES :
      case RMV_CMD_GETGAMMA :
      case RMV_CMD_STOPANIMATE :
         bCmdErr = (iCmdLen == 1) ? false : true;
         break;

      case RMV_CMD_LOADTARGETS :
         bCmdErr = !parseLoadTargets();
         break;

      case RMV_CMD_STARTANIMATE :
         bCmdErr = !parseStartAnimateFrame0();
         break;

      case RMV_CMD_UPDATEFRAME :
         bCmdErr = !parseUpdateFrame();
         break;
      
      case RMV_CMD_GETMEDIADIRS :
      case RMV_CMD_GETMEDIAFILES :
      case RMV_CMD_GETMEDIAINFO :
      case RMV_CMD_DELETEMEDIA :
      case RMV_CMD_PUTFILE :
      case RMV_CMD_PUTFILECHUNK :
      case RMV_CMD_PUTFILEDONE :
         bCmdErr = !parseMediaAndFileCommands();
         break;
         
      default:
         bCmdErr = true;      // unrecognized command ID
         break;
   }

   if( bCmdErr )
   {
      fprintf(stderr, "RMVideo(IoNet): Maestro command (id=%d) could not be parsed!\n", pCmdBuf[0]);
      sendSignal(RMV_SIG_CMDERR);
      cmd = RMV_CMD_NONE;
   }

   return(cmd);
}

//=== parseLoadTargets, parseStartAnimateFrame0, parseStartAnimateFrame1, parseUpdateFrame ============================
//
//    These helper methods handle the details of parsing the 32-bit integer command sequence for the Maestro commands
//    RMV_CMD_LOADTARGETS, RMV_CMD_STARTANIMATE, and RMV_CMD_UPDATEFRAME.  The STARTANIMATE command is parsed in two
//    distinct steps:  parseStartAnimateFrame0() parses out the motion vectors for frame0 of the animation sequence,
//    which CRMVDisplay is expected to retrieve via a series of calls to getMotionVector().  After getMotionVector() is
//    invoked for the last participating target, parseStartAnimateFrame1() is called to parse out the motion vectors
//    for frame 1 from the STARTANIMATE command -- which should still be sitting in the command buffer!
//
//    Each method returns false if the command is incorrectly formatted.  For details on the content of each of these
//    Maestro commands, see RMVIDEO_COMMON.H.
//
bool CRMVIoNet::parseLoadTargets()
{
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;

   // effectively "forget" any previously loaded targets
   m_nTargets = 0;

   // make sure the number of targets is reasonable
   int nTgts = pCmdBuf[1];
   if( nTgts<=0 || nTgts>RMV_MAXTARGETS ) return( false );

   // realloc our target record and motion vector buffers if we need more room.  Even though we don't work with the
   // motion vector buffer here, we keep it the same size as the target record buffer.
   if(nTgts > m_iTgtBufSz)
   {
      int iNewSz = ((nTgts / DEF_TGTBUFSZ) + 1) * DEF_TGTBUFSZ;
      RMVTGTDEF* pNewTgtBuf = (RMVTGTDEF*) realloc( (void*) m_pTargets, iNewSz*sizeof(RMVTGTDEF) );
      if(pNewTgtBuf == NULL)
      {
         fprintf(stderr, "RMVideo(IoNet): realloc() failed!\n" );
         return( false );
      }
      RMVTGTVEC* pNewVecBuf = (RMVTGTVEC*) realloc( (void*) m_pMotionVecs, iNewSz*sizeof(RMVTGTVEC) );
      if(pNewVecBuf == NULL)
      {
         // this is ugly because we want the target definition and motion vector buffers to be the same size, but
         // here we successfully reallocated the first but not the second buffer.  We choose to free both.  Chances
         // are things are really bad, anyway.
         free(m_pMotionVecs);
         free(pNewTgtBuf);
         m_pMotionVecs = NULL;
         m_pTargets = NULL;
         m_iTgtBufSz = 0;
         fprintf(stderr, "RMVideo(IoNet): realloc() failed!\n" );
         return( false );
      }

      m_iTgtBufSz = iNewSz;
      m_pTargets = pNewTgtBuf;
      m_pMotionVecs = pNewVecBuf;
   }

   // clear out the target record buffer, at least the part that we need
   memset( (void*) m_pTargets, 0, nTgts*sizeof(RMVTGTDEF) );

   // parse the RMV_CMD_LOADTARGETS sequence
   int iCmdIndex = 2;
   int iTgt = 0;
   while( iCmdIndex < iCmdLen )
   {
      // check next paramID.  If it is the "end of target" key, then increment target index.  Otherwise, it must be
      // followed by one, two or three parameter values (if not, it's a parse error).
      int iCode = pCmdBuf[iCmdIndex++];
      int nParams = 1;
      if(iCode==RMV_TGTDEF_FLICKER) 
         nParams = 3;
      else if((iCode >= RMV_TGTDEF_SPATIALF && iCode <= RMV_TGTDEF_SIGMA) || (iCode==RMV_TGTDEF_RGBMEAN) ||
              (iCode==RMV_TGTDEF_RGBCON))
         nParams = 2;

      if(iCode == RMV_TGTDEF_END)
      {
         ++iTgt;
         continue;
      }
      else
      {
         int iValid = iCmdIndex+nParams-1;
         if( iValid >= iCmdLen ) return( false );
      }

      // get first parameter value as an integer and a float; if applicable, get second and third parameter values.
      // (Only RMV_TGTDEF_FLICKER has 3, all integers.) Two target parameters have character string values, 
      // RMV_TGTDEF_FOLDER and _FILE. The actual string length will not exceed RMV_MVF_LEN < 32, and the null-padded
      // string occupies the next 32 bytes (8 ints) of the command buffer.
      int iValue = 0, iValue2 = 0, iValue3 = 0;
      float fValue = 0.0f, fValue2 = 0.0f;
      char* sValue = NULL;
      
      if(iCode == RMV_TGTDEF_FOLDER || iCode == RMV_TGTDEF_FILE)
      {
         if(iCmdIndex + 8 > iCmdLen) return(false);
         sValue = (char*) &(pCmdBuf[iCmdIndex]);
         iCmdIndex +=8;
      }
      else
      {
         iValue = pCmdBuf[iCmdIndex++];
         fValue = ((float) iValue) / RMV_TGTDEF_F2I_F;
         if(nParams > 1)
         {
            iValue2 = pCmdBuf[iCmdIndex++];
            fValue2 = ((float) iValue2) / RMV_TGTDEF_F2I_F;
         }
         if(nParams > 2)
            iValue3 = pCmdBuf[iCmdIndex++];
      }

      // stuff parameter value(s) in the right place
      switch( iCode )
      {
         default :                     return( false ); break;
         case RMV_TGTDEF_TYPE :        m_pTargets[iTgt].iType = iValue; break;
         case RMV_TGTDEF_APERTURE :    m_pTargets[iTgt].iAperture = iValue; break;
         case RMV_TGTDEF_FLAGS :       m_pTargets[iTgt].iFlags = iValue; break;
         case RMV_TGTDEF_RGBMEAN :     m_pTargets[iTgt].iRGBMean[0] = iValue;
                                       m_pTargets[iTgt].iRGBMean[1] = iValue2;
                                       break;
         case RMV_TGTDEF_RGBCON :      m_pTargets[iTgt].iRGBCon[0] = iValue;
                                       m_pTargets[iTgt].iRGBCon[1] = iValue2;
                                       break;
         case RMV_TGTDEF_OUTERW :      m_pTargets[iTgt].fOuterW = fValue; break;
         case RMV_TGTDEF_OUTERH :      m_pTargets[iTgt].fOuterH = fValue; break;
         case RMV_TGTDEF_INNERW :      m_pTargets[iTgt].fInnerW = fValue; break;
         case RMV_TGTDEF_INNERH :      m_pTargets[iTgt].fInnerH = fValue; break;
         case RMV_TGTDEF_NDOTS :       m_pTargets[iTgt].nDots = iValue; break;
         case RMV_TGTDEF_NDOTSIZE :    m_pTargets[iTgt].nDotSize = iValue; break;
         case RMV_TGTDEF_SEED :        m_pTargets[iTgt].iSeed = iValue; break;
         case RMV_TGTDEF_PCTCOHER :    m_pTargets[iTgt].iPctCoherent = iValue; break;
         case RMV_TGTDEF_NOISEUPD :    m_pTargets[iTgt].iNoiseUpdIntv = iValue; break;
         case RMV_TGTDEF_NOISELIM :    m_pTargets[iTgt].iNoiseLimit = iValue; break;
         case RMV_TGTDEF_DOTLIFE :     m_pTargets[iTgt].fDotLife = fValue; break;
         case RMV_TGTDEF_SPATIALF :    m_pTargets[iTgt].fSpatialFreq[0] = fValue;
                                       m_pTargets[iTgt].fSpatialFreq[1] = fValue2;
                                       break;
         case RMV_TGTDEF_DRIFTAXIS :   m_pTargets[iTgt].fDriftAxis[0] = fValue;
                                       m_pTargets[iTgt].fDriftAxis[1] = fValue2;
                                       break;
         case RMV_TGTDEF_GRATPHASE :   m_pTargets[iTgt].fGratPhase[0] = fValue;
                                       m_pTargets[iTgt].fGratPhase[1] = fValue2;
                                       break;
         case RMV_TGTDEF_SIGMA :       m_pTargets[iTgt].fSigma[0] = fValue;
                                       m_pTargets[iTgt].fSigma[1] = fValue2;
                                       break;
         case RMV_TGTDEF_FOLDER :      ::strncpy(m_pTargets[iTgt].strFolder, sValue, RMV_MVF_LEN); break;
         case RMV_TGTDEF_FILE :        ::strncpy(m_pTargets[iTgt].strFile, sValue, RMV_MVF_LEN); break;
         case RMV_TGTDEF_FLICKER :     m_pTargets[iTgt].iFlickerOn = iValue;
                                       m_pTargets[iTgt].iFlickerOff = iValue2;
                                       m_pTargets[iTgt].iFlickerDelay = iValue3;
                                       break;
         case RMV_TGTDEF_DOTDISP:      m_pTargets[iTgt].fDotDisp = fValue; break;
      }
   }

   // if we did not get definitions for the #targets specified in the command, it's a parse error.  Also, the last
   // integer in the command sequence must end the definition of the last target
   if( iTgt != nTgts || pCmdBuf[iCmdLen-1] != RMV_TGTDEF_END )
      return( false );

   // parsing successful.  Remember the # of targets defined.
   m_nTargets = nTgts;
   return( true );
}

bool CRMVIoNet::parseStartAnimateFrame0()
{
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;

   // make sure command length is correct: the cmd code, the sync spot flash request flag, plus motion vectors for all 
   // loaded targets for two frames
   if(iCmdLen < 2 + 2*(1 + RMV_TGTVEC_LEN*m_nTargets))
      return(false);

   // is the vertical sync spot flash requested to start on frame 0?
   m_bSyncFlashRequested = (pCmdBuf[1] != 0);

   // check number of motion vectors for frame 0 -- must equal #targets loaded!
   if(pCmdBuf[2] != m_nTargets)
      return(false);
      

   // check number of motion vectors for frame 1 -- must equal #targets loaded!
   if(pCmdBuf[3 + RMV_TGTVEC_LEN*m_nTargets] != m_nTargets)
      return(false);

   // parse command sequence to extract frame 0 motion vectors.  NOTE that motion vector buffer is guaranteed to be big
   // enough; if necessary, it was realloc'd in response to previous RMV_CMD_LOADTARGETS command.
   int j = 3;
   for(int i=0; i<m_nTargets; i++)
   {
      // make sure tgt index is valid. They must be provided in order 0..N-1, where N is #targets!
      if(pCmdBuf[j] != i)
         return(false);

      m_pMotionVecs[i].bOn = (pCmdBuf[j+1] != 0);
      m_pMotionVecs[i].hWin = ((float) pCmdBuf[j+2]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vWin = ((float) pCmdBuf[j+3]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].hPat = ((float) pCmdBuf[j+4]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vPat = ((float) pCmdBuf[j+5]) / RMV_TGTVEC_F2I_F;

      // move to the next vector in command sequence
      j += RMV_TGTVEC_LEN;
   }

   // set flag indicating that frame 0 vectors are ready to be enumerated by CRMVDisplay.
   m_bEnumFrame0 = true;

   return( true );
}

bool CRMVIoNet::parseStartAnimateFrame1()
{
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;

   // validate the command length once again and the number of vectors for frames 0 and 1 -- just to be safe
   // remember that the sync flash request flag for frame 0 immediately follows the command ID
   if(iCmdLen < 2 + 2*(1 + RMV_TGTVEC_LEN*m_nTargets))
      return(false);
   if(pCmdBuf[2] != m_nTargets)
      return(false);
   if(pCmdBuf[3 + RMV_TGTVEC_LEN*m_nTargets] != m_nTargets)
      return(false);

   // parse command sequence to extract frame 1 motion vectors.  NOTE that motion vector buffer is guaranteed to be big
   // enough; if necessary, it was realloc'd in response to previous RMV_CMD_LOADTARGETS command.
   int j = 4 + RMV_TGTVEC_LEN*m_nTargets;
   for(int i=0; i<m_nTargets; i++)
   {
      // make sure tgt index is valid. They must be provided in order 0..N-1, where N is #targets!
      if(pCmdBuf[j] != i)
         return(false);

      m_pMotionVecs[i].bOn = (pCmdBuf[j+1] != 0);
      m_pMotionVecs[i].hWin = ((float) pCmdBuf[j+2]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vWin = ((float) pCmdBuf[j+3]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].hPat = ((float) pCmdBuf[j+4]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vPat = ((float) pCmdBuf[j+5]) / RMV_TGTVEC_F2I_F;

      // move to the next vector in command sequence
      j += RMV_TGTVEC_LEN;
   }

   return( true );
}

bool CRMVIoNet::parseUpdateFrame()
{
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;

   // validate command length and verify # of vectors in frame equals the number of targets loaded. The first int32
   // after the command ID is the sync spot flash request flag...
   if(iCmdLen != 3 + RMV_TGTVEC_LEN*m_nTargets) return(false);
   if(pCmdBuf[2] != m_nTargets) return(false);

   // is the vertical sync spot flash requested to start on frame 0?
   m_bSyncFlashRequested = (pCmdBuf[1] != 0);
   
   // parse command sequence to extract motion vectors for next frame.  NOTE that motion vector buffer is guaranteed to
   // be big enough; if necessary, it was realloc'd in response to previous RMV_CMD_LOADTARGETS command.
   int j = 3;
   for(int i=0; i<m_nTargets; i++)
   {
      // make sure tgt index is valid. They must be provided in order 0..N-1, where N is #targets!
      if(pCmdBuf[j] != i)
         return(false);

      m_pMotionVecs[i].bOn = (pCmdBuf[j+1] != 0);
      m_pMotionVecs[i].hWin = ((float) pCmdBuf[j+2]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vWin = ((float) pCmdBuf[j+3]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].hPat = ((float) pCmdBuf[j+4]) / RMV_TGTVEC_F2I_F;
      m_pMotionVecs[i].vPat = ((float) pCmdBuf[j+5]) / RMV_TGTVEC_F2I_F;

      // move to the next vector in command sequence
      j += RMV_TGTVEC_LEN;
   }

   return( true );
}

//=== parseMediaAndFileCommands =======================================================================================
//
//    This helper method handles the Maestro commands RMV_CMD_GETMEDIADIRS, _GETMEDIAFILES, _GETMEDIAINFO,
//    _DELETEMEDIA, _PUTFILE, _PUTFILECHUNK, and _PUTFILEDONE.
//
//    The method returns false if the command is found to be incorrectly formatted. Media folder and file names, if
//    relevant to the command, are copied to m_strMediaFolder and m_strMediaFile. These character strings are emptied
//    otherwise. For details on the content of each of these Maestro commands, see RMVIDEO_COMMON.H.
//
bool CRMVIoNet::parseMediaAndFileCommands()
{
   // we assume there are no byte ordering issues to worry about, so we can cast our byte buffer to an int32 buffer
   // to recover the actual command sequence from Maestro!
   int iCmdLen = m_iRcvLenBytes / 4;
   int* pCmdBuf = (int*) m_pRcvBuf;
   int cmd = pCmdBuf[0];
   
   // clear the media folder and filename strings
   ::memset((void*) m_strMediaFolder, (int) '\0', RMV_MVF_LEN + 1);
   ::memset((void*) m_strMediaFile, (int) '\0', RMV_MVF_LEN + 1);
   
   bool ok = false;
   switch(cmd)
   {
      case RMV_CMD_GETMEDIADIRS :
         ok = (iCmdLen == 1) ? true : false;
         break;
      case RMV_CMD_GETMEDIAFILES :
         ok = (m_iRcvLenBytes >= 8);
         if(ok)
         {
            char* folderName = &(m_pRcvBuf[4]);
            int n = ::strlen(folderName);
            ok = (n > 0) && (n <= RMV_MVF_LEN) && (n == ::strspn(folderName, RMV_MVF_CHARS));
            if(ok) ::strcpy(m_strMediaFolder, folderName);
         }
         break;
      case RMV_CMD_PUTFILE : 
         // either no command arguments (downloading RMVideo executable), or the destination media folder & file names.
         // NOTE: FALL THROUGH IN THE LATTER CASE!
         if(iCmdLen == 1)
         {
            ok = true;
            break;
         }
      case RMV_CMD_GETMEDIAINFO :
         ok = (m_iRcvLenBytes >= 8);
         if(ok)
         {
            char* folderName = &(m_pRcvBuf[4]);
            int n = ::strlen(folderName);
            ok = (n > 0) && (n <= RMV_MVF_LEN) && (n == ::strspn(folderName, RMV_MVF_CHARS));
            if(ok) ok = (n + 5 < m_iRcvLenBytes);
            if(ok)
            {
               char* fileName = &(m_pRcvBuf[n+5]);
               int m = ::strlen(fileName);
               ok = (m > 0) && (m <= RMV_MVF_LEN) && (m == ::strspn(fileName, RMV_MVF_CHARS));
               if(ok)
               {
                  ::strcpy(m_strMediaFolder, folderName);
                  ::strcpy(m_strMediaFile, fileName);
               }
            }
         }
         break;
         
      case RMV_CMD_DELETEMEDIA :
         // this command may have one (delete folder) or two (delete file) string arguments
         ok = (m_iRcvLenBytes >= 8);
         if(ok)
         {
            char* folderName = &(m_pRcvBuf[4]);
            int n = ::strlen(folderName);
            ok = (n > 0) && (n <= RMV_MVF_LEN) && (n == ::strspn(folderName, RMV_MVF_CHARS));
            if(ok)
            {
               char* fileName = NULL;
               int m = 0;
               if(n+5 < m_iRcvLenBytes)
               {
                  fileName = &(m_pRcvBuf[n+5]);
                  m = ::strlen(fileName);
               }
               if(m == 0)
                  ::strcpy(m_strMediaFolder, folderName);
               else
               {
                  ok = (m <= RMV_MVF_LEN) && (m == ::strspn(fileName, RMV_MVF_CHARS));
                  if(ok)
                  {
                     ::strcpy(m_strMediaFolder, folderName);
                     ::strcpy(m_strMediaFile, fileName);
                  }
               }
            }
         }
         break;
         
      // the next two commands should NEVER be processed here -- downloadFile() handles the file transfer command 
      // sequence using pollForSocketCommand() directly, NOT getNextCommand(). Here we just make sure that the command
      // buffer is a reasonable size for the command. CRMVDisplay should balk at receiving these two commands.
      case RMV_CMD_PUTFILECHUNK : 
         ok = (iCmdLen > 1);
         if(ok) ok = (m_iRcvLenBytes - 8 >= pCmdBuf[1]);
         break;
         
      case RMV_CMD_PUTFILEDONE :
         ok = (iCmdLen == 2);
         break;
         
      default :
         ok = false;
         break;
   }
   
   return(ok);
}


int CRMVIoNet::getNumTargets() { return( m_nTargets ); }

bool CRMVIoNet::getTarget(int iPos, RMVTGTDEF& tgt)
{
   // invalid target index
   if( iPos < 0 || iPos >= m_nTargets ) return( false );

   tgt = m_pTargets[iPos];
   return( true );
}

// IMPORTANT:  The CRMVIoNet implementation parses the motion vectors for frame 1 from the STARTANIMATE command
// only AFTER this method is invoked for the last target in the target list.  If CRMVDisplay fails to abide by its
// CRMVIo contract, expect strange results!
bool CRMVIoNet::getMotionVector(int iPos, RMVTGTVEC& vec)
{
   // invalid target index
   if(iPos < 0 || iPos >= m_nTargets) return(false);

   // copy the motion vector data requested
   vec = m_pMotionVecs[iPos];

   // in response to the RMV_CMD_STARTANIMATE command, CRMVDisplay will first invoke this method N times to retrieve
   // the motion vector for each of N participating targets during frame 0.  When invoked for the last target, we need
   // to finish parsing the STARTANIMATE command to get the target motion vectors during frame 1.
   bool bOk = true;
   if(m_bEnumFrame0 && (iPos == m_nTargets-1))
   {
      m_bEnumFrame0 = false;
      bOk = parseStartAnimateFrame1();
      if( !bOk )
         fprintf(stderr, "RMVideo(IoNet): Failure parsing frame1 motion vecs from RMV_CMD_STARTANIMATE\n");
   }

   return(bOk);
}

bool CRMVIoNet::isSyncFlashRequested() { return(m_bSyncFlashRequested); }

const char* CRMVIoNet::getMediaFolder() { return(m_strMediaFolder); }
const char* CRMVIoNet::getMediaFile() { return(m_strMediaFile); }

// This method processes a stream of RMV_CMD_PUTFILECHUNK commands followed by a single RMV_CMD_PUTFILEDONE. Receipt of
// any other command causes termination of the download: RMV_SIG_CMDERR is sent to Maestro and the method aborts.
// The file chunks are read directly from the command buffer and streamed to the open file. If any file write fails,
// the download is again terminated. The file descriptor is closed before returning, regardless the outcome.
bool CRMVIoNet::downloadFile(FILE* fd)
{
   if(!sessionInProgress())
   {
      fprintf(stderr, "File download failed -- no session in progress!\n");
      if(fd != NULL) ::fclose(fd);
      return(false);
   }
   
   if(fd == NULL)
   {
      fprintf(stderr, "File download failed -- NULL file descriptor!\n");
      sendSignal(RMV_SIG_CMDERR);
      return(false);
   }

   bool done = false;
   bool ok = true;
   bool cancelled = false;
   while(!done)
   {
      int nextCmd = pollSocketForCommand();

      if(nextCmd < RMV_CMD_NONE)
      {
         fprintf(stderr, "(CRMVIoNet::downloadFile) Connection failed during file download!\n");
         ::fclose(fd);
         return(false);
      }
      else if(nextCmd == RMV_CMD_PUTFILECHUNK)
      {
         int* pCmdBuf = (int*) m_pRcvBuf;
         ok = (m_iRcvLenBytes > 8);
         if(ok) ok = (m_iRcvLenBytes - 8 >= pCmdBuf[1]);
         if(!ok)
         {
            done = true;
            fprintf(stderr, "(CRMVIoNet::downloadFile) Download failed on bad file chunk command!\n");
         }
         else
         {
            ok = (1 == ::fwrite((void*) &(m_pRcvBuf[8]), pCmdBuf[1], 1, fd));
            if(!ok)
            {
               done = true;
               ::perror("(CRMVIoNet::downloadFile) Download failed on file write error!\n");
            }
         }
         
         // if successful, signal Maestro to send the next chunk
         if(ok) sendSignal(RMV_SIG_CMDACK);
      }
      else if(nextCmd == RMV_CMD_PUTFILEDONE)
      {
         int len = m_iRcvLenBytes / 4;
         int* pCmdBuf = (int*) m_pRcvBuf;
         ok = (len == 2);
         if(ok)
         {
            cancelled = (pCmdBuf[1] == 0);
            if(cancelled) 
            {
               ok = false;
               fprintf(stderr, "(CMRMVIoNet::downloadFile) Download cancelled by Maestro!\n");
            }
         }
         else
            fprintf(stderr, "CRMVIoNet::downloadFile) Download failed on bad file done command!\n");
         done = true;
      }
      else if(nextCmd != RMV_CMD_NONE)
      {
         fprintf(stderr, "(CRMVIoNet::downloadFile) Download failed on invalid command (%d)!\n", nextCmd);
         ok = false;
         done = true;
      }
   }
   
   ::fclose(fd);
   if(!ok) sendSignal(cancelled ? RMV_SIG_CMDACK : RMV_SIG_CMDERR);
   return(ok);
}


//    RMVideo needs to send very little information back to Maestro, and very infrequently, so this implementation
//    assumes that we'll never block on a send() call to our non-blocking session socket.  If we do, we fail silently
//    -- knowing that the error will catch up with us anyway.  The key point is we never want to block sending a
//    message to Maestro.
//
//    Like the Maestro command, an RMVideo data packet is simply a sequence of integers. Again, we assume that byte 
//    ordering on the Maestro and RMVideo workstations are the same, so we ignore the "network to host byte order" 
//    stuff. We simply recast the reply buffer as a byte buffer, send the reply length in bytes, immediately followed 
//    by the reply body.
void CRMVIoNet::sendData(int len, int* pPayload)
{
   // here's a static byte buffer we use.  It supports up a reply sequence length of 99, with the byte count in front.
   // if the reply is longer than 99 ints, we fail silently.
   static char msgBuf[100*4];

   if(len > 99) return;

   // make sure we're connected!
   if(!sessionInProgress()) return;

   // copy reply to our byte buffer, preceded by reply length in bytes
   int* pMsgBufAsInt = (int*) &(msgBuf[0]);
   pMsgBufAsInt[0] = len;
   for(int i=0; i<len; i++) pMsgBufAsInt[i+1] = pPayload[i];

   // send the message through our non-blocking session socket.  If we get EWOULDBLOCK error, we do not retry -- we
   // just fail silently
   int nBytesRemaining = 4*(len + 1);
   int nBytesSent = 0;
   while(nBytesRemaining > 0)
   {
      int nBytesDone = send(m_sessionSocket, msgBuf+nBytesSent, nBytesRemaining, MSG_NOSIGNAL);
      if(nBytesDone < 0)
      {
         if(errno == EWOULDBLOCK)
            fprintf(stderr, "RMVideo(IoNet): send() could not run without blocking -- reply not sent!\n");
         else
            perror("RMVideo(IoNet): send()");
         return;
      }
      else
      {
         nBytesRemaining -= nBytesDone;
         nBytesSent += nBytesDone;
      }
   }

}

