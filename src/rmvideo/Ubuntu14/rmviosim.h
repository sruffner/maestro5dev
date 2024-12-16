//=====================================================================================================================
//
// rmviosim.h : Declaration of class CRMVIoSim, which simulates the communication link between RMVideo and Maestro,
//    reading its "command stream" from a file.
//
//=====================================================================================================================


#if !defined(RMVIOSIM_H_INCLUDED_)
#define RMVIOSIM_H_INCLUDED_

#include <stdio.h>

#include "rmvio.h"                                          // base class CRMVIo



#define IOSIM_MAXSEGS 30                                    // max number of segments in a simulated animation
#define IOSIM_MAXTGTS 25                                    // max number of target in a simulated animation
#define IOSIM_MAXLINELEN 150                                // max length of a line in the command file

class CRMVIoSim : public CRMVIo
{
public:
   CRMVIoSim();
   ~CRMVIoSim();

   bool init();                                             // init communication interface
   void cleanup();                                          // destroy all resources alloc'd to this comm interface
   bool openSession();                                      // open connection session with Maestro
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
   static const char* SIMFILENAME;                          // name of file that should contain simulated cmd stream
   FILE* m_pFile;                                           // pointer to the file that contains simulated cmd stream
   char m_nextLine[IOSIM_MAXLINELEN];                       // next text line from the command file
   int m_iLineNumber;                                       // track line # in command file so we can report errors

   int m_args[3];                                           // args specified with last command, if applicable

   char m_strMediaFolder[RMV_MVF_LEN+1];                    // media folder name from last relevant cmd
   char m_strMediaFile[RMV_MVF_LEN+1];                      // media file name from last relevant cmd
   char m_strSrcPath[50];                                   // source file path for emulated "download file" op
   
   float m_fFramePeriodUS;                                  // the display frame period in us
   int m_iLastCmd;                                          // last command issued; in case we need to look at reply
                                                            
   
   int m_iState;                                            // current state of the simulation:
   
   static const int SIM_SLEEP;                              // RMVideo is sleeping, waiting for next command session
   static const int SIM_IDLE;                               // idle state, prior to receiving "load" command
   static const int SIM_LOADING;                            // RMVideo consuming tgt defns after "load" command
   static const int SIM_LOADED;                             // finished loading targets, ready to animate
   static const int SIM_STARTING;                           // starting animation: RMVideo consuming motion vecs for
                                                            // first two display frames
   static const int SIM_WAITFORFIRSTFRAME;                  // waiting for "firstFrame" signal at start of animation
   static const int SIM_ANIMATING;                          // animation in progress, between frame updates
   static const int SIM_UPDATING;                           // RMVideo consuming motion vecs after "updateFrame" cmd
   static const int SIM_ABORTING;                           // error or skipped frame; abort on next cmd

   int m_nTgtsAnimated;                                     // number of tgts participating in animation sequence
   int m_nEnumSoFar;                                        // to enumerate tgt defns for the "loadTargets" cmd, or
                                                            // motion records for the "updateFrame" cmd

   float m_fElapsedTime;                                    // elapsed time during animation, in milliseconds
   float m_fStopTime;                                       // elapsed time at which animation ends, in milliseconds
   int m_nSegments;                                         // total number of segments in current animation
   int m_iCurrSeg;                                          // index of current segment
   bool m_bAtSegStart;                                      // true at the start of each segment in an animation

   RMVTGTDEF m_Targets[IOSIM_MAXTGTS];                      // target defns extracted from "load" command group

   struct CTraj                                             // per-segment target trajectory parameters:
   {
      bool bOn;                                             //    on/off state
      float fPos[2];                                        //    absolute change in pos at segment start, in degrees
      float fVel[2];                                        //    window velocity during segment, in deg/frame
      float fPatVel[2];                                     //    pattern velocity during segment, in deg/frame
   };

   CTraj m_Trajectories[IOSIM_MAXSEGS][IOSIM_MAXTGTS];      // segmented representation of an animation sequence
   float m_fSegStart[IOSIM_MAXSEGS];                        // start times for the sequential segments in animation
   bool m_bSyncAtSegStart[IOSIM_MAXSEGS];                   // whether or not sync flash should be shown at seg start

   // flag set at start of any segment when sync flash is requested; reset when isSyncFlashRequested() is called
   bool m_bSyncOn;

   int processIdleCommand();                                // process next command from file while in idle state
   int processTargetRecords( int n );                       // process all tgt records in command file after "load"
   int processAnimationCommands();                          // process all commands from file defining an animation
   int doNextFrame();                                       // prepare next "updateFrame" cmd during an animation
};


#endif // !defined(RMVIO_H_INCLUDED_)
