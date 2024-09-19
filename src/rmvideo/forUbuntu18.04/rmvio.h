//=====================================================================================================================
//
// rmvio.h : Declaration of ABSTRACT class CRMVIo, which encapsulates the communication link between RMVideo and
//           Maestro.
//
//=====================================================================================================================


#if !defined(RMVIO_H_INCLUDED_)
#define RMVIO_H_INCLUDED_

#include <stdio.h>
#include "rmvideo_common.h"                                       // common defns shared by Maestro and RMVideo


class CRMVIo
{
public:
   virtual bool init() = 0;                                       // init communication interface; do NOT connect!
   virtual void cleanup() = 0;                                    // destroy all resources alloc'd to this object

   virtual bool openSession() = 0;                                // BLOCK, waiting until a Maestro client establishes a
                                                                  // connection and issues the RMV_CMD_STARTINGUP cmd
   virtual void closeSession() = 0;                               // issue RMV_SIG_BYE, then close connex w/Maestro

   virtual int getNextCommand() = 0;                              // get next command from Maestro, if any
   virtual int getCommandArg(int pos) = 0;                        // to retrieve 32-bit int arguments of selected cmds
   
   virtual int getNumTargets() = 0;                               // get # of target defns in "load targets" cmd
   virtual bool getTarget(int iPos, RMVTGTDEF& tgt ) = 0;         // get a target defn sent in "load targets" cmd
   virtual bool getMotionVector(int iPos, RMVTGTVEC& vec) = 0;    // retrieve a tgt motion vector defined in the "start
                                                                  // animating" and "update frame" commands
   virtual bool isSyncFlashRequested() = 0;                       // start sync spot flash in this animation frame

   virtual const char* getMediaFolder() = 0;                      // get media folder name from selected commands
   virtual const char* getMediaFile() = 0;                        // get media file name from selected commands
   virtual bool downloadFile(FILE* fd) = 0;                       // download a file over the communication interface
   
   virtual void sendData(int len, int* pPayload) = 0;             // send information back to Maestro
   void sendSignal(int sig);                                      // send a 32-bit signal to Maestro
};


#endif // !defined(RMVIO_H_INCLUDED_)
