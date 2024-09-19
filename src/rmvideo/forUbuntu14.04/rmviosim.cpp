//=====================================================================================================================
//
// rmviosim.cpp : Class CRMVIoSim, which simulates the communication link between Maestro and RMVideo, reading its
//    command stream from a file in the current working directory.  For testing purposes only.
//
// AUTHOR:  saruffner.
//
// DESCRIPTION:
// CRMVIoSim is an implementation of the abstract class CRMVIo that simulates the communication link between Maestro
// and RMVideo.  It was developed to test and evaluate the operation of RMVideo independent of Maestro.  It looks for
// a file named MSIMCMDS.TXT in the current working directory.  This ASCII text file should define a simulated "Maestro
// command session" in an abbreviated form as described below.  CRMVIoSim translates the file's content into the raw
// command stream Maestro would send RMVideo to execute one or more target animation sequences.
//
// Expected format of MSIMCMDS.TXT:  The ASCII text file must contain a sequence of commands, with one command per
// text line.  If any line begins with a pound ('#') character, the entire line is ignored -- use this to put comments
// in the file.  Otherwise, each line is parsed as a command of the form "cmdname param1 param2 ...".  No text line
// may exceed 120 characters, including comment lines!
//
// The table below lists the entire set of commands recognized by CRMVIoSim and their expected parameters.  A typical
// MSIMCMDS.TXT file will start with the "hello" command, set up the display geometry and background color with
// "setgeom" and "setbkg", load a set of targets with "load", present a single animation sequence ("start", "seg",
// etc..., "stop"), then issue "bye" and "exit".  Case is ignored entirely when trying to match strings.  If CRMVIoSim
// encounters any error while parsing MSIMCMDS.TXT, it will print an error message to the console and tell RMVideo to
// quit.
//
//    COMMAND        PARAMS
//    exit           (none)
//    ==> Close display window and quit.  This is important for testing purposes; if not issued, RMVideo will keep
//    running until it is killed.
//
//    hello          (none)
//    ==> Maestro is starting up.  RMVideo should wake up from sleep state, enter idle state, and set the background
//    to black.
//
//    getversion     (none)
//    ==> Retrieve RMVideo application version number.
//
//    restart        (none)
//    ==> Restart RMVideo. If successful, RMVideo will restart using network IO (CRMVIoNet) rather than the emulated 
//    command session provided by CRMVIoSim. This is really only for testing purposes. Feature not used by Maestro.
//
//    bye            (none)
//    ==> Maestro is shutting down.  RMVideo will release the X display and enter a sleep state, waiting for the
//    next "session" to start.
//
//    setbkg         Packed RGB integer: byte 0 (LSB) = red luminance, 1 = green, and 2 = blue.  Byte 3 unused.
//    ==> Set background RGB color.
//
//    setgeom        Three integers W,H,D.  W = width of RMVideo monitor's visible screen area, H = height.  D = LOS
//                   distance to subject's eye.  Units = millimeters.
//    ==> Set display geometry.
//
//    setsync        Two ints S, D. S = spot size in mm ([0..50], 0=disabled). D = flash duration in #frames [1..9]
//    ==> Set parameters for the vertical sync spot flash feature.
//                  
//    getgamma       (none)
//    ==> Get the current monitor gamma correction factors for R, G, and B guns.
//
//    setgamma       Three integers R,G,B, representing the desired gamma correction factors for R, G, and B guns 
//                   scaled by a factor of 1000. All must lie in [800..3000].
//    ==> Set the current monitor gamma correction factors for R, G, and B guns.
//
//    getallvmodes   (none)
//    ==> Get a listing of all supported video modes that are at least 1024x768 @ 75Hz or better.
//
//    getvmode       (none)
//    ==> Get the curent video mode.
//
//    setvmode       Integer specifiying the requested video mode. Must be [1..N], where N is # of video modes.
//    ==> Change the current video mode.
//
//    getmovdirs     (none)
//    ==> Get a listing of all folders in the RMVideo media store.
//
//    getmovfiles    Name of media folder (a string)
//    ==> Get a listing of all media files in a specified folder within the RMVideo media store.
//
//    getmovinfo     Media folder name, media file name (two strings separated by whitespace).
//    ==> Get information on the specified media file within the RMVideo media store. For image files, the image size
//    is retrieved. For video files, the video frame size, frame rate in Hz, and approximate duration are provided.
//
//    deletemov      Media folder name[ media file name] (one or two strings separated by whitespace).
//    ==> Permanently delete an existing media folder and all its files in the RMVideo media store (one-argument case),
//    or delete a single media file in the store (two-argument case).
//
//    putmov         Media folder name, media file name, source file path. Media folder and file names are
//                   limited to RMV_MVF_LEN characters in length and must not include whitespace. The source
//                   file path is limited to 50 characters and cannot include whitespace.
//    ==> Emulate downloading a media file (copies the specified source file).
//
//    putexec        Source file path, limited to 50 characters and no whitespace.
//    ==> Emulate downloading the RMVideo executable file (copies the specified source file). NOTE -- This is no longer
//    supported as of May 2016.
//
//    delay          Integer delay time in seconds.  Allowed range is [1..10].
//    ==> This command is solely for simulation purposes.  It causes the emulator to wait the specified number of
//    seconds before issuing the next command.  A "parsing error" will be triggered if this command appears anywhere
//    between the 'load' command that begins the definition of an animation sequence, and the 'stop' command that
//    terminates the sequence. It does not correspond to any command that Maestro would send to RMVideo.
//
//    load           An integer N specifying the number of targets to animate.  Restricted to [1..25].
//    ==> Load definitions of N targets to be animated. This command must precede the definition of any animation
//        sequence.  It MUST be immediately followed by N target records, followed by the complete definition of the
//        animation sequence.  Each "record" is a set of (paramID, value)-pairs, one per line, as listed below.  Each
//        supported RMVideo target type typically only uses a subset of the possible parameters.
//
//       type        "point", "randomdots", "flowfield", "bar", "spot", "grating", "plaid", "movie", "image".
//       aperture    "rect", "oval", "rectannu", or "ovalannu".
//       flags       Integer.  See RMVIDEO_COMMON.H for list of recognized bit flags.
//       rgbmean     Mean color, a packed RGB integer: 0xBBGGRR.  Can be written in decimal form, but the hex form
//                   is easier to decipher.  For "plaid" targets, two integers must be specified, because each
//                   grating cmpt has its own color spec.  For all other targets, only 1 integer is necessary.
//       rgbcon      Color constrast, a packed RGB integer, like rgbmean, but each byte must lie in [0..100%].  For
//                   "plaid" targets, two integers must be specified -- the contrast for each grating cmpt.  For all
//                   other targets, only one integer is necessary.
//       outerw      Outer bounding rect width in deg; floating-pt.
//       outerh      Outer bounding rect height in deg; floating-pt.
//       innerw      Inner bounding rect width in deg; floating-pt.
//       innerh      Inner bounding rect height in deg; floating-pt.
//       ndots       Number of dots in target pattern; positive int.
//       dotsize     Size of a target "dot" in pixels; int in [1..10].
//       seed        Random number generator seed; integer.
//       coher       Percent coherence; integer in [0..100].
//       noiseupd    Speed or directional noise update interval in ms; nonnegative int.
//       noiselimit  Noise range limit N, determines allowed range [-N:N] of noise; int in [0..180] deg for dir noise,
//                   [0..1000] for speed noise, in 0.1% incr (speed noise defined as a %-age of nominal speed).
//       dotlife     Max lifetime of each target dot (0 = infinite lifetime); nonnegative floating-pt value.
//       spatialf    Spatial freq for up to two gratings, in cyc/deg; one or two floating-pt values.
//       driftaxis   Drift axis for up to two gratings, in deg CCW; one or two floating-pt values.
//       gratphase   Initial spatial phase for up to two gratings, in deg; one or two floating-pt values.
//       sigma       Standard deviations (in X and Y) of Gaussian window for selected target types, in deg subtended at
//                   eye. Two floating-pt values; the first is std dev in X, the second is std dev in Y.
//       folder      Name of media folder ("movie" and "image" targets only). Value is a string of 30 characters or
//                   less; only ASCII alphanumeric characters, the period or the underscore are allowed.
//       file        Name of media file ("movie" and "image" targets only). Similar value restrictions as for "folder"
//                   parameter.
//       flicker     onDur offDur delay -- Three nonnegative integer parameters specifying flicker ON cycle duration,
//                   OFF cycle duration, and initial delay in # of video frames. Range-limited to [0..99].
//       enddef      (None).  This marks the end of the target record.  Every record must end with this ID on its own
//                   line in the text file.
//
//    start          Number of segments in the animation; an integer restricted to [1..30].
//    ==> This command marks the beginning of an animation sequence.  It is followed by a series of "seg", "onoff",
//        "pos", "winvel", and "patvel" commands that describe what the targets should do during the
//        animation.  This scheme offers a segmented representation of the animation somewhat akin to Maestro's "trial
//        codes".  CRMVIoSim translates it into the frame-by-frame sequence of commands expected by RMVideo.  Note that
//        the "seg" command specifies time in ms, NOT video frames.  CRMVIoSim prepares the per-frame RMVideo command
//        sequence to reproduce the desired animation as closely as possible given the monitor's frame period.
//
//        !!!IMPORTANT!!! At the start of a new segment (after the first), CRMVIoSim copies all target trajectory
//        parameters from the preceding segment.  Thus, it is essential to issue new trajectory commands whenever
//        a target's trajectory changes at the segment boundary!!!
//
//    seg            T
//    ==> Marks the start of a new segment of motion, beginning at elapsed time T in ms.  All animation cmds between
//    this and the next "seg" command take effect at time T.  Of course, T must increase monotonically with each
//    subsequent "seg" command.  T is an integer.
//
//    sync           N
//    ==> If N!=0, start vertical sync spot flash at the start of the current segment. Ignored if the sync spot flash
//    feature is disabled (spot size = 0).
//
//    onoff          N,F
//    ==> Turn target #N either on (F!=0) or off (F=0) at the start of the current segment.  N and F are integers.
//    Targets are identified by the order in which they were defined by the previous "load" command set.
//
//    pos            N,H,V
//    ==> Displace target #N by (H,V) degrees from its current position at start of current segment. H and V can be
//    floating-point.
//
//    winvel         N,H,V
//    ==> Set the window velocity of target #N to (H,V) deg/sec.  H and V can be floating-pt.  The window velocity
//    remains in effect (possibly across segment boundaries) until the next "winvel" command is issued.
//
//    patvel         N,H,V
//    ==> Analogously for the target's pattern velocity.  However, for a "grating" target, V is ignored and H is
//    treated as the grating's drift speed ALONG ITS DRIFT AXIS!  For a "plaid" target, if the RMV_F_INDEPGRATS flag
//    is set, H is the drift speed for grating #1 and V is the drift speed for grating #2.  If the flag is unset,
//    then the plaid is treated as a single pattern and (H,V) is the normal pattern velocity.
//
//    stop           T
//    ==> This command marks the end of the animation sequence, at elasped time T in ms (an integer).  It must always
//    be paired with a prior "start" cmd.
//
// REVISION HISTORY:
// 29aug2005-- Created.
// 09jan2006-- Revised IAW changes in target definition to support speed noise in RMV_RANDOMDOTS target.
// 04apr2006-- Revised to support introduction of Gaussian std dev's in X and Y, to handle elliptical as well as 1D
//             Gaussian windows.  RMVTGTDEF.fSigma is now a 2-element array.
// 06apr2006-- Revised to support separate color specs for the grating cmpts of a plaid.  RMVTGTDEF.iRGBMean and
//             RMVTGTDEF.iRGBCon are now 2-element arrays.
// 24apr2006-- Revised to support new query command, RMV_CMD_GETCOLORRES, which reports RMVideo's color resolution 
//             (16bit or 24bit possible).
// 22jul2009-- Major changes. CRMVIo implementation no longer replies directly to any commands. The animation-related 
//             commands provide motion vectors for all loaded targets, so the 'bNoChange' argument has been removed 
//             from getMotionVector(). Added support for switching video mode via new command RMV_CMD_SETCURRVIDEOMODE.
// 24aug2009-- Modified IAW same-dtd changes in CRMVIo.
// 25aug2009-- Modified IAW same-dtd changes in rmvideo_common.h.
// 04oct2016-- Renamed getMovie***() methods as getMedia***(). The former RMVideo "movie store" is now a "media store"
//             that can store image as well as video files. Related commands have also changed names: RMV_CMD_GETMOVIE*
//             are now RMV_CMD_GETMEDIA*, and RMV_CMD_DELETEMOVIE is now RMV_CMD_DELETEMEDIA. NOTE that the names of
//             the corresponding commands in MSIMCMDS.TXT (getmovdirs, getmovfiles, etc) were left unchanged!
//          -- Added support for the new RMV_IMAGE target type.
// 25sep2018-- Modified to support the new vertical sync spot flash feature.
// 26mar2019-- Modified IAW changes in Maestro-RMVideo communication protocol during animate mode (for version 9).
// 07may2019-- Added support for target flicker parameters (for version 10).
//=====================================================================================================================

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "rmviosim.h"

const char* CRMVIoSim::SIMFILENAME = "msimcmds.txt";

const int CRMVIoSim::SIM_SLEEP               = 0;
const int CRMVIoSim::SIM_IDLE                = 1;
const int CRMVIoSim::SIM_LOADING             = 2;
const int CRMVIoSim::SIM_LOADED              = 3;
const int CRMVIoSim::SIM_STARTING            = 4;
const int CRMVIoSim::SIM_WAITFORFIRSTFRAME   = 5;
const int CRMVIoSim::SIM_ANIMATING           = 6;
const int CRMVIoSim::SIM_UPDATING            = 7;
const int CRMVIoSim::SIM_ABORTING            = 8;


CRMVIoSim::CRMVIoSim()
{
   m_pFile = NULL;
   m_iLineNumber = 0;
   m_args[0] = -1;
   m_args[1] = -1;
   m_args[2] = -1;
   
   for(int i=0;i<RMV_MVF_LEN+1; i++)
   {
      m_strMediaFolder[i] = '\0';
      m_strMediaFile[i] = '\0';
   }
   
   m_iLastCmd = RMV_CMD_NONE;
   m_fFramePeriodUS = -1.0f;         // will trigger sending RMV_CMD_GETFRAMEPER to retrieve the frame period
   
   m_iState = CRMVIoSim::SIM_SLEEP;

   m_nTgtsAnimated = 0;
   m_nEnumSoFar = 0;

   m_fElapsedTime = 0.0f;
   m_fStopTime = 0.0f;
   m_nSegments = 0;
   m_iCurrSeg = -1;
}

CRMVIoSim::~CRMVIoSim() { cleanup(); }

//=== init ============================================================================================================
//
//    Initialize the communication interface.
//
//    For the simulator, we simply open the MSIMCMDS.TXT file.  The file must be located in the current working
//    directory.  If we fail to open the file, then we cannot run the simulation!
//
//    ARGS:       NONE.
//    RETURNS:    True if successful; false if unable to setup resources for the communication interface.
//
bool CRMVIoSim::init()
{
   // make sure file is closed before opening it!
   cleanup();

   m_iLineNumber = 0;
   m_pFile = ::fopen( CRMVIoSim::SIMFILENAME, "r" );
   if( m_pFile == NULL )
      ::perror( "Could not open simulated command stream" );
   return( m_pFile != NULL );
}

//=== cleanup =========================================================================================================
//
//    Destroy the Maestro-RMVideo communication interface, releasing any allocated resources.
//
//    All we need to do here is close the MSIMCMDS.TXT file, if it is open.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
void CRMVIoSim::cleanup()
{
   if( m_pFile != NULL )
   {
      ::fclose( m_pFile );
      m_pFile = NULL;
   }
}

//=== openSession =====================================================================================================
//
//    Open a command session with Maestro, BLOCKING until a Maestro client makes a connection and sends the
//    RMV_CMD_STARTINGUP command.
//
//    In the simulated command session, the only permitted commands in the "wait_sleep" state are "delay", "hello", and
//    "exit".  If any other command is read from the command file, it is considered a parsing error and this method
//    fails -- causing RMVideo to exit.  The "exit" command is special because it is only for test purposes.  If we
//    read the "exit" command from the command file, we print a message to that effect in stderr and return false,
//    which will cause RMVideo to terminate.
//
//    ARGS:       NONE.
//    RETURNS:    True if Maestro command session established; false if an error occurred.
//
bool CRMVIoSim::openSession()
{
   char cmdName[25];
   int i;

   if( m_iState != SIM_SLEEP )
   {
      fprintf(stderr, "RMVIoSim: openSession() called while not in sleep state! Aborting!\n" );
      return( false );
   }

   bool waiting = true;
   while( waiting )
   {
      if( NULL == ::fgets(m_nextLine, IOSIM_MAXLINELEN, m_pFile ) )           // get next text line from file
      {
         fprintf(stderr, "RMVIoSim: Error reading file at line %d!\n", m_iLineNumber);
         return( false );
      }
      ++m_iLineNumber;

      bool bParsed = true;
      if(1 != ::sscanf(m_nextLine, " %24s", &(cmdName[0])))                   // get command name on line, ignoring
         continue;                                                            // leading whitespace.
      else if( cmdName[0] == '#' )                                            // ignore comment lines!
         continue;
      else if( 0 == ::strcasecmp(cmdName, "hello") )                          // OK! Next cmd session is starting!
         waiting = false;
      else if( 0 == ::strcasecmp(cmdName, "delay") )                          // sleep for specified time, then get
      {                                                                       // next command line
         bParsed = (2 == ::sscanf(m_nextLine, " %24s %d", &(cmdName[0]), &i));
         if( bParsed ) bParsed = ((i>0) && (i<11));
         if( bParsed )
            ::sleep( (unsigned int) i );
      }
      else if( 0 == ::strcasecmp(cmdName, "exit") )                           // force RMVideo to exit
      {
         fprintf(stderr, "RMVIoSim: Got 'exit' command at line %d!\n", m_iLineNumber);
         return( false );
      }
      else
         bParsed = false;

      if( !bParsed )
      {
         fprintf(stderr, "RMVIoSim: Parsing error at line %d!\n", m_iLineNumber);
         return( false );
      }
   }

   // command session has started: switch to idle state
   m_iState = SIM_IDLE;
   return( true );
}

//=== closeSession ====================================================================================================
//
//    Close a command session with Maestro.  RMVideo invokes this function is response to the RMV_CMD_SHUTTINGDN
//    command from Maestro.  The function should issue RMV_SIG_BYE to acknowlege the end of the command session, then
//    take whatever steps are necessary to close the actual connection.
//
//    The command file read by CRMVIoSim may include multiple command sessions, bracketed by the "hello"
//    (ie, RMV_CMD_STARTINGUP) and "bye" (RMV_CMD_SHUTTINGDN) commands.  All we need to do here is return to the
//    "sleep" state.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
void CRMVIoSim::closeSession()
{
   if( m_iState > SIM_IDLE )
      fprintf(stderr, "RMVIoSim: closeSession() called in an inappropriate state!\n" );
   if( m_iState == SIM_IDLE )
      m_iState = SIM_SLEEP;
}

//=== getNextCommand ==================================================================================================
//
//    Get the next command from Maestro, if any.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID, RMV_CMD_NONE if no command is pending, or <RMV_CMD_NONE if an error occurred.
//
int CRMVIoSim::getNextCommand()
{
   // reset integer command args that are relevant only to selected commands
   for(int i=0; i<3; i++) m_args[i] = -1;
   
   int nextCmd = RMV_CMD_NONE-1;
   switch( m_iState )
   {
      case SIM_IDLE :                           // process any of the commands allowed while in "idle" state
         nextCmd = processIdleCommand();
         break;

      case SIM_LOADED :                         // after loading targets, consume all commands governing the subsequent
         nextCmd = processAnimationCommands();  // animation sequence, then issue "startAnimation" command
         break;

      case SIM_ANIMATING :                      // advance animation by one frame and prepare motion vectors or stop
         nextCmd = doNextFrame();
         break;

      case SIM_ABORTING :                       // simulation aborting due to a previous error or skipped frame
         nextCmd = RMV_CMD_EXIT;
         break;

      case SIM_LOADING :                        // getNextCommand() should NOT be called in any of these transient
      case SIM_UPDATING :                       // states, nor in the "wait_sleep" state -- abort
      case SIM_STARTING :
      case SIM_WAITFORFIRSTFRAME :
      case SIM_SLEEP :
      default:
         fprintf(stderr, "RMVIoSim: getNextCommand() out of context at t=%d (state=%d)! Aborting!\n",
            int(m_fElapsedTime), m_iState );
         nextCmd = RMV_CMD_EXIT;
         break;
   }

   m_iLastCmd = nextCmd;
   return( nextCmd );
}

int CRMVIoSim::getCommandArg(int pos)
{
   int arg = (pos >= 0 && pos < 3) ? m_args[pos] : -1;
   return(arg);
}

int CRMVIoSim::getNumTargets()
{
   return( (m_iState==CRMVIoSim::SIM_LOADING) ? m_nTgtsAnimated : 0 );
}

bool CRMVIoSim::getTarget(int iPos, RMVTGTDEF& tgt )
{
   if( m_iState != CRMVIoSim::SIM_LOADING ) return( false );         // invoked inappropriately!
   if( iPos < 0 || iPos >= m_nTgtsAnimated ) return( false );        // invalid tgt index

   tgt = m_Targets[iPos];                                            // retrieve next tgt defn

   ++m_nEnumSoFar;
   if( m_nEnumSoFar == m_nTgtsAnimated )                             // CRMVDisplay has retrieved all target defns!
      m_iState = CRMVIoSim::SIM_LOADED;                              // we expect "startAnimation" cmd next.
   return( true );
}

bool CRMVIoSim::getMotionVector(int iPos, RMVTGTVEC& vec)
{
   if( m_iState != CRMVIoSim::SIM_STARTING &&                              // invoked inappropriately
       m_iState != CRMVIoSim::SIM_UPDATING )
      return( false );
   if( iPos < 0 || iPos >= m_nTgtsAnimated ) return( false );              // invalid tgt index

   CTraj* pTraj = &(m_Trajectories[m_iCurrSeg][iPos]);                     // current traj params for the tgt

   vec.bOn = pTraj->bOn;
   if( m_bAtSegStart )                                                     // fill in motion vector fields; start of a
   {                                                                       // segment is special case!
      vec.hWin = pTraj->fPos[0];
      if( m_iCurrSeg > 0 ) vec.hWin += m_Trajectories[m_iCurrSeg-1][iPos].fVel[0];
      vec.vWin = pTraj->fPos[1];
      if( m_iCurrSeg > 0 ) vec.vWin += m_Trajectories[m_iCurrSeg-1][iPos].fVel[1];

      vec.hPat = (m_iCurrSeg > 0) ? m_Trajectories[m_iCurrSeg-1][iPos].fPatVel[0] : 0.0f;
      vec.vPat = (m_iCurrSeg > 0) ? m_Trajectories[m_iCurrSeg-1][iPos].fPatVel[1] : 0.0f;
   }
   else
   {
      vec.hWin = pTraj->fVel[0];
      vec.vWin = pTraj->fVel[1];
      vec.hPat = pTraj->fPatVel[0];
      vec.vPat = pTraj->fPatVel[1];
   }

   ++m_nEnumSoFar;                                          // update state once CRMVDisplay has retrieved all motion
   if( m_nEnumSoFar == m_nTgtsAnimated )                    // vectors for current frame:
   {
      if( m_iState == CRMVIoSim::SIM_UPDATING )             //    ready to supply next "updateFrame"
         m_iState = CRMVIoSim::SIM_ANIMATING;
      else                                                  //    done supplying 1st set of motion vecs for
      {                                                     //    "startAnimate" cmd; advance to next frame and wait
         doNextFrame();                                     //    for "first frame" signal
         m_iState = CRMVIoSim::SIM_WAITFORFIRSTFRAME;
      }
   }

   return( true );
}

bool CRMVIoSim::isSyncFlashRequested()
{
   if(m_iState < CRMVIoSim::SIM_STARTING || m_iState > CRMVIoSim::SIM_UPDATING) return(false);

   bool syncReq = m_bSyncOn;
   m_bSyncOn = false;
   return(syncReq);
}

const char* CRMVIoSim::getMediaFolder() { return(m_strMediaFolder); }
const char* CRMVIoSim::getMediaFile() { return(m_strMediaFile); }

bool CRMVIoSim::downloadFile(FILE* fd)
{
   if(fd == NULL)
   {
      fprintf(stderr, "[CRMVIoSim::downloadFile] Download emulation failed -- NULL file descriptor!\n");
      return(false);
   }
   
   // open the source file
   FILE* fdSrc = ::fopen(m_strSrcPath, "r");
   if(fdSrc == NULL)
   {
      fprintf(stderr, "[CRMVIoSim::downloadFile] Download emulation failed -- Could not open movie source!\n");
      ::fclose(fd);
      return(false);
   }
   
   // allocate a 16KB buffer for moving bytes from src to dst file
   char* pBytes = (char *) ::malloc(16384);
   if(pBytes == NULL)
   {
      fprintf(stderr, "[CRMVIoSim::downloadFile] Download emulation failed -- Could not allocate byte buffer!\n");
      ::fclose(fdSrc);
      ::fclose(fd);
      return(false);
   }
   
   // copy src to dst
   bool done = false;
   bool ok = true;
   while(!done)
   {
      int nBytes = ::fread(pBytes, 1, 16384, fdSrc);
      if(nBytes < 16384)
      {
         done = true;
         ok = (::feof(fdSrc) != 0) ? true : false;
         if(!ok) fprintf(stderr, "[CRMVIoSim::downloadFile] Download emulation failed -- Error while reading src!\n");
      }
      if(ok && nBytes > 0)
      {
         int n = ::fwrite(pBytes, 1, nBytes, fd);
         ok = (n == nBytes) ? true : false;
         if(!ok) 
         {
            fprintf(stderr, "[CRMVIoSim::downloadFile] Download emulation failed -- Error while writing dst!\n");
            done = true;
         }
      }
   }
   
   // close both src and dst, and free allocated buffer
   ::free(pBytes);
   ::fclose(fd);
   ::fclose(fdSrc);
   return(ok);
}

// Since CRMVIoSim merely simulates communication with Maestro, this method does nothing.  However, if the signal
// to send is "skippedFrame" or "dupFrame", we print an error message and skip to the end of the animation seq.
// Also, if the signal to send is "previousCommandFailed", then we print an error message to the console and set the
// next command to "exit" -- causing RMVideo to exit.
// 
// In addition, we use this as an avenue to get the display frame period from CRMVDisplay in response to the 
// RMV_CMD_GETCURRVIDEOMODE or RMV_SETCURRVIDEOMODE commands (idle state only), and to report information sent by 
// CRMVDisplay in response to sundry idle-state commands.
void CRMVIoSim::sendData(int len, int* pPayload)
{
   int lastCmd = m_iLastCmd;
   m_iLastCmd = RMV_CMD_NONE;
   
   if(len <= 0) return;
   int sigCode = pPayload[0];
   
   if(sigCode == RMV_SIG_CMDERR)
   {
      int t = (m_iState >= SIM_LOADED) ? int(m_fElapsedTime) : -1;
      fprintf(stderr, "RMVIoSim: Command (id=%d) failed at t=%d\n", lastCmd, t);
      m_iState = SIM_ABORTING;
   }
   else if(sigCode == RMV_SIG_ANIMATEMSG)
   {
      // once we get signal that animation started, we must be ready to supply motion vectors for second frame
      if(len == 1)
      {
         if( m_iState != SIM_WAITFORFIRSTFRAME )
         {
            fprintf(stderr, "RMVIoSim: Got 'firstFrame' signal out of context!\n" );
            m_iState = SIM_ABORTING;
         }
         else
         {
            m_nEnumSoFar = 0;
            m_iState = SIM_UPDATING;
         }
      }

      // if we get a duplicate frame message, simply report it. Let animation continue.
      else if(len == 3)
      {
         int t = (m_iState >= SIM_LOADED) ? int(m_fElapsedTime) : -1;
         bool isMissedUpd = (pPayload[2] == 0);
         fprintf(stderr, "RMVIOSim: %s at t=%d, frame index=%d, nDupes=%d.\n", 
            isMissedUpd ? "Missed update" : "Render delay", t, pPayload[1], isMissedUpd ? 1 : pPayload[2]);
      }

      // NOTE: we ignore ANIMATEMSG sent once per second with elapsed frame count (len==2)
   }
   else if(m_iState == SIM_IDLE)
   {
      if(lastCmd == RMV_CMD_GETVERSION)
      {
         fprintf(stderr, "RMVideo version number = %d\n", pPayload[0]);
      }
      else if(lastCmd == RMV_CMD_GETCURRVIDEOMODE && pPayload[0] == RMV_SIG_CMDACK && len >= 3)
      {
         // update measured frame period -- we need it to run animation sequences!
         m_fFramePeriodUS = ((float) pPayload[2]) / 1000.0f;
         fprintf(stderr, "Current video mode index = %d; measured frame period = %d us\n", pPayload[1], pPayload[2]);
      }
      else if(lastCmd == RMV_CMD_SETCURRVIDEOMODE && pPayload[0] == RMV_SIG_CMDACK && len >= 2)
      {
         // update measured frame period -- we need it to run animation sequences!
         m_fFramePeriodUS = ((float) pPayload[1]) / 1000.0f;
         fprintf(stderr, "Video mode changed; measured frame period = %d us\n",  pPayload[1]);
      }
      else if(lastCmd == RMV_CMD_GETALLVIDEOMODES && pPayload[0] == RMV_SIG_CMDACK)
      {
         // list video modes on stderr...
         int nModes = pPayload[1];
         fprintf(stderr, "Found %d supported video modes:\n", nModes);
         int j = 2;
         for(int i=0; i<nModes; i++)
         {
            fprintf(stderr, "  %d: %d x %d @ %dHz\n", i, pPayload[j], pPayload[j+1], pPayload[j+2]);
            j += 3;
         }
      }
      else if(lastCmd == RMV_CMD_GETGAMMA && pPayload[0] == RMV_SIG_CMDACK)
      {
         double r = ((double) pPayload[1]) / 1000.0;
         double g = ((double) pPayload[2]) / 1000.0;
         double b = ((double) pPayload[3]) / 1000.0;
         fprintf(stderr, "Current monitor gamma: r=%.2f, g=%.2f, b=%.2f\n", r, g, b);
      }
      else if(lastCmd == RMV_CMD_GETMEDIADIRS && pPayload[0] == RMV_SIG_CMDACK)
      {
         // list media folders on stderr...
         int n = pPayload[1];
         fprintf(stderr, "Found %d folders in media store:\n", n);
         
         char* strList = (char *) &(pPayload[2]);
         int nChars = 4 * (len - 2);
         for(int i=0; nChars>0 && i<n; i++)
         {
            int n = ::strlen(strList);
            if(n == 0) break;
            fprintf(stderr, "   %s\n", strList);
            strList += (n+1);
            nChars -= (n+1);
         }
      }
      else if(lastCmd == RMV_CMD_GETMEDIAFILES && pPayload[0] == RMV_SIG_CMDACK)
      {
         // list media files on stderr...
         int n = pPayload[1];
         fprintf(stderr, "Found %d media files in folder '%s':\n", n, m_strMediaFolder);
         
         char* strList = (char *) &(pPayload[2]);
         int nChars = 4 * (len - 2);
         for(int i=0; nChars>0 && i<n; i++)
         {
            int n = ::strlen(strList);
            if(n == 0) break;
            fprintf(stderr, "   %s\n", strList);
            strList += (n+1);
            nChars -= (n+1);
         }
      }
      else if(lastCmd == RMV_CMD_GETMEDIAINFO && pPayload[0] == RMV_SIG_CMDACK)
      {
         // report media info on stderr...
         if(pPayload[4] < 0)
         {
            fprintf(stderr, "Image file at %s/%s: %d x %d pixels\n", m_strMediaFolder, m_strMediaFile,
                    pPayload[1], pPayload[2]);
         }
         else
         {
            fprintf(stderr, "Info on movie at %s/%s:\n", m_strMediaFolder, m_strMediaFile);
            fprintf(stderr, "  %d x %d pix; %.3f sec approx running time at %.3f Hz.\n",
                    pPayload[1], pPayload[2], ((double)pPayload[4])/1000.0, ((double)pPayload[3])/1000.0);
         }
      }
      else if(lastCmd == RMV_CMD_DELETEMEDIA && pPayload[0] == RMV_SIG_CMDACK)
      {
         // report success of operation on stderr...
         fprintf(stderr, "Media file removed successfully: %s/%s\n", m_strMediaFolder, m_strMediaFile);
      }
   }
}


//=== processIdleCommand ==============================================================================================
//
//    Process next command from the command stream file while in idle state.
//
//    In the "idle" state, Maestro may send any of the RMVideo commands except those that start, stop or update the
//    state of an animation sequence.  The "hello" command is also invalid, since that will be processed in the
//    "wait_sleep" state".  This method reads the next command line from the simulated command stream file and
//    processes it appropriately.  If the command is invalid or if a file I/O error occurs, an error message is printed
//    to stderr, and the "exit" command is issued.
//
//    When the first command session starts, RMV_CMD_GETFRAMEPER is issued automatically to retrieve the RMVideo
//    display's accurately measured refresh period. We need this to run animation sequences. The frame period is
//    parsed from the reply in sendReply(). Similarly when a video mode switch is triggered by RMV_CMD_SETCURRVIDEOMODE.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID.  If a file I/O or parsing error occurs, an error message is posted to stderr
//       and the "exit" command is issued.
//
int CRMVIoSim::processIdleCommand()
{
   char cmdName[25];
   char fmt[30];
   int i, i1, i2;

   // display frame period not yet initialized -- retrieve it!
   if(m_fFramePeriodUS < 0.0f) 
   {
      m_iLastCmd = RMV_CMD_GETCURRVIDEOMODE;
      return(m_iLastCmd);
   }
   
   int nextCmd = RMV_CMD_NONE;
   while( nextCmd == RMV_CMD_NONE )
   {
      if( NULL == ::fgets(m_nextLine, IOSIM_MAXLINELEN, m_pFile ) )           // get next text line from file
      {
         fprintf(stderr, "RMVIoSim: Error reading file at line %d!\n", m_iLineNumber);
         nextCmd = RMV_CMD_EXIT;
         break;
      }
      ++m_iLineNumber;

      bool bParsed = true;
      if( 1 != ::sscanf(m_nextLine, " %24s", &(cmdName[0])) )                 // get command name on line, ignoring
         continue;                                                            // leading whitespace.
      else if( cmdName[0] == '#' )                                            // ignore comment lines!
         continue;
      else if( 0 == ::strcasecmp(cmdName, "exit") )                           // process legal commands in idle state:
         nextCmd = RMV_CMD_EXIT;
      else if(0 == ::strcasecmp(cmdName, "getversion"))
         nextCmd = RMV_CMD_GETVERSION;
      else if(0 == ::strcasecmp(cmdName, "restart"))
         nextCmd = RMV_CMD_RESTART;
      else if( 0 == ::strcasecmp(cmdName, "bye") )
         nextCmd = RMV_CMD_SHUTTINGDN;
      else if( 0 == ::strcasecmp(cmdName, "setbkg") )
      {
         bParsed = (2 == ::sscanf(m_nextLine," %24s %i", &(cmdName[0]), &i));
         if( bParsed )
         {
            nextCmd = RMV_CMD_SETBKGCOLOR;
            m_args[0] = i;
         }
      }
      else if( 0 == ::strcasecmp(cmdName, "setgeom") )
      {
         bParsed = (4 == ::sscanf(m_nextLine," %24s %d %d %d", &(cmdName[0]), &i, &i1, &i2));
         if( bParsed )
         {
            nextCmd = RMV_CMD_SETGEOMETRY;
            m_args[0] = i;
            m_args[1] = i1;
            m_args[2] = i2;
         }
      }
      else if(0 == ::strcasecmp(cmdName, "setsync"))
      {
         bParsed = (3 == ::sscanf(m_nextLine," %24s %d %d", &(cmdName[0]), &i, &i1));
         if( bParsed )
         {
            nextCmd = RMV_CMD_SETSYNC;
            m_args[0] = i;
            m_args[1] = i1;
         }
      }
      else if(0 == ::strcasecmp(cmdName, "getgamma"))
         nextCmd = RMV_CMD_GETGAMMA;
      else if( 0 == ::strcasecmp(cmdName, "setgamma") )
      {
         bParsed = (4 == ::sscanf(m_nextLine," %24s %d %d %d", &(cmdName[0]), &i, &i1, &i2));
         if( bParsed )
         {
            nextCmd = RMV_CMD_SETGAMMA;
            m_args[0] = i;
            m_args[1] = i1;
            m_args[2] = i2;
         }
      }
      else if(0 == ::strcasecmp(cmdName, "getallvmodes"))
         nextCmd = RMV_CMD_GETALLVIDEOMODES;
      else if(0 == ::strcasecmp(cmdName, "getvmode"))
         nextCmd = RMV_CMD_GETCURRVIDEOMODE;
      else if( 0 == ::strcasecmp(cmdName, "setvmode") )
      {
         bParsed = (2 == ::sscanf(m_nextLine," %24s %i", &(cmdName[0]), &i));
         if( bParsed )
         {
            nextCmd = RMV_CMD_SETCURRVIDEOMODE;
            m_args[0] = i;
         }
      }
      else if(0 == ::strcasecmp(cmdName, "getmovdirs"))
         nextCmd = RMV_CMD_GETMEDIADIRS;
      else if(0 == ::strcasecmp(cmdName, "getmovfiles"))
      {
         ::sprintf(fmt, " %%24s %%%ds", RMV_MVF_LEN);
         bParsed = (2 == ::sscanf(m_nextLine, fmt, &(cmdName[0]), &(m_strMediaFolder[0])));
         if(bParsed) nextCmd = RMV_CMD_GETMEDIAFILES;
      }
      else if(0 == ::strcasecmp(cmdName, "getmovinfo"))
      {
         ::sprintf(fmt, " %%24s %%%ds %%%ds", RMV_MVF_LEN, RMV_MVF_LEN);
         bParsed = (3 == ::sscanf(m_nextLine, fmt, &(cmdName[0]), &(m_strMediaFolder[0]), &(m_strMediaFile[0])));
         if(bParsed) nextCmd = RMV_CMD_GETMEDIAINFO;
      }
      else if(0 == ::strcasecmp(cmdName, "deletemov"))
      {
         for(i=0;i<RMV_MVF_LEN+1; i++)
         {
            m_strMediaFolder[i] = '\0';
            m_strMediaFile[i] = '\0';
         }
         ::sprintf(fmt, " %%24s %%%ds %%%ds", RMV_MVF_LEN, RMV_MVF_LEN);
         i = ::sscanf(m_nextLine, fmt, &(cmdName[0]), &(m_strMediaFolder[0]), &(m_strMediaFile[0]));
         if(i == 2 || i == 3) nextCmd = RMV_CMD_DELETEMEDIA;
      }
      else if(0 == ::strcasecmp(cmdName, "putmov"))
      {
         ::sprintf(fmt, " %%24s %%%ds %%%ds %%50s", RMV_MVF_LEN, RMV_MVF_LEN);
         bParsed = (4 == ::sscanf(m_nextLine, fmt, &(cmdName[0]), &(m_strMediaFolder[0]), &(m_strMediaFile[0]),
                                  &(m_strSrcPath[0])));
         if(bParsed) nextCmd = RMV_CMD_PUTFILE;
      }
      else if(0 == ::strcasecmp(cmdName, "putexec"))
      {
         ::sprintf(fmt, " %%24s %%50s");
         bParsed = (2 == ::sscanf(m_nextLine, fmt, &(cmdName[0]), &(m_strSrcPath[0])));
         if(bParsed) 
         {
            ::sprintf(m_strMediaFolder, "%s", "");
            ::sprintf(m_strMediaFile, "%s", "");
            nextCmd = RMV_CMD_PUTFILE;
         }
      }
      else if( 0 == ::strcasecmp(cmdName, "delay") )                          // sleep for specified time, then get
      {                                                                       // next command line
         bParsed = (2 == ::sscanf(m_nextLine, " %24s %d", &(cmdName[0]), &i));
         if( bParsed ) bParsed = ((i>0) && (i<11));
         if( bParsed )
            ::sleep( (unsigned int) i );
      }
      else if( 0 == ::strcasecmp(cmdName, "load") )
      {
         bParsed = (2 == ::sscanf(m_nextLine," %24s %d", &(cmdName[0]), &i));
         if( bParsed ) bParsed = (i>0 && i<=IOSIM_MAXTGTS);
         if( bParsed ) nextCmd = processTargetRecords( i );
      }
      else
         bParsed = false;

      if( !bParsed )
      {
         fprintf(stderr, "RMVIoSim: Parsing error at line %d!\n", m_iLineNumber);
         nextCmd = RMV_CMD_EXIT;
      }
   }

   return( nextCmd );
}

//=== processTargetRecords ============================================================================================
//
//    Process text lines defining N target records after the "load N" command line.  See class header for details on
//    how target definitions are entered in the simulator's command file.  Note that the method does not check the
//    validity of the target records, it only parses a series of command lines into records IAW the expected format.
//
//    ARGS:       n  -- [in] the number of target records to be read in.
//    RETURNS:    The next command ID.  This will be "loadTargets" if the function was successful; otherwise, the
//       "exit" command is issued and an error message is posted to stderr (file I/O or parsing error).
//
int CRMVIoSim::processTargetRecords( int n )
{
   char field[25];
   char strVal[25];
   int  nFields, iVal, iVal1, iVal2;
   float fVal, fVal1;

   ::memset( m_Targets, 0, IOSIM_MAXTGTS*sizeof(RMVTGTDEF) );              // reset target record storage

   bool bOk = true;
   int iRecordsDone = 0;                                                   // parse target defns from file until we get
   while( bOk && iRecordsDone < n )                                        // expected # of targets, or error occurs
   {
      if( NULL == ::fgets(m_nextLine, IOSIM_MAXLINELEN, m_pFile ) )        // get next text line from file
      {
         fprintf(stderr, "RMVIoSim: Error reading file at line %d!\n", m_iLineNumber);
         bOk = false;
         break;
      }
      ++m_iLineNumber;

      if( 1 != ::sscanf(m_nextLine, " %24s", field) )                      // get field name on line, ignoring leading
         continue;                                                         // whitespace.
      else if( field[0] == '#' )                                           // ignore comment lines!
         continue;
      else if( 0 == ::strcasecmp(field, "enddef") )                        // end of current tgt record
      {
         ++iRecordsDone;
         continue;
      }

      bool bParseErr = false;                                              // process (field, val, ...) entries:
      if( 0 == ::strcasecmp(field, "type") )
      {
         bParseErr == (2 != ::sscanf(m_nextLine, " %24s %24s", field, strVal));
         if( !bParseErr )
         {
            if( 0 == ::strcasecmp(strVal, "point") ) m_Targets[iRecordsDone].iType = RMV_POINT;
            else if( 0 == ::strcasecmp(strVal, "randomdots") ) m_Targets[iRecordsDone].iType = RMV_RANDOMDOTS;
            else if( 0 == ::strcasecmp(strVal, "flowfield") ) m_Targets[iRecordsDone].iType = RMV_FLOWFIELD;
            else if( 0 == ::strcasecmp(strVal, "bar") ) m_Targets[iRecordsDone].iType = RMV_BAR;
            else if( 0 == ::strcasecmp(strVal, "spot") ) m_Targets[iRecordsDone].iType = RMV_SPOT;
            else if( 0 == ::strcasecmp(strVal, "grating") ) m_Targets[iRecordsDone].iType = RMV_GRATING;
            else if( 0 == ::strcasecmp(strVal, "plaid") ) m_Targets[iRecordsDone].iType = RMV_PLAID;
            else if( 0 == ::strcasecmp(strVal, "movie") ) m_Targets[iRecordsDone].iType = RMV_MOVIE;
            else if( 0 == ::strcasecmp(strVal, "image") ) m_Targets[iRecordsDone].iType = RMV_IMAGE;
            else bParseErr = true;
         }
      }
      else if( 0 == ::strcasecmp(field, "aperture") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %24s", field, strVal));
         if( !bParseErr )
         {
            if( 0 == ::strcasecmp(strVal, "rect") ) m_Targets[iRecordsDone].iAperture = RMV_RECT;
            else if( 0 == ::strcasecmp(strVal, "oval") ) m_Targets[iRecordsDone].iAperture = RMV_OVAL;
            else if( 0 == ::strcasecmp(strVal, "rectannu") ) m_Targets[iRecordsDone].iAperture = RMV_RECTANNU;
            else if( 0 == ::strcasecmp(strVal, "ovalannu") ) m_Targets[iRecordsDone].iAperture = RMV_OVALANNU;
            else bParseErr = true;
         }
      }
      else if( 0 == ::strcasecmp(field, "flags") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %i", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].iFlags = iVal;
      }
      else if( 0 == ::strcasecmp(field, "rgbmean") )
      {
         nFields = ::sscanf(m_nextLine, " %24s %i %i", field, &iVal, &iVal1);
         if( nFields == 3 ) m_Targets[iRecordsDone].iRGBMean[1] = iVal1;
         if( nFields >= 2 ) m_Targets[iRecordsDone].iRGBMean[0] = iVal;
         else bParseErr = true;
      }
      else if( 0 == ::strcasecmp(field, "rgbcon") )
      {
         nFields = ::sscanf(m_nextLine, " %24s %i %i", field, &iVal, &iVal1);
         if( nFields == 3 ) m_Targets[iRecordsDone].iRGBCon[1] = iVal1;
         if( nFields >= 2 ) m_Targets[iRecordsDone].iRGBCon[0] = iVal;
         else bParseErr = true;
      }
      else if( 0 == ::strcasecmp(field, "outerw") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %f", field, &fVal));
         if( !bParseErr ) m_Targets[iRecordsDone].fOuterW = fVal;
      }
      else if( 0 == ::strcasecmp(field, "outerh") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %f", field, &fVal));
         if( !bParseErr ) m_Targets[iRecordsDone].fOuterH = fVal;
      }
      else if( 0 == ::strcasecmp(field, "innerw") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %f", field, &fVal));
         if( !bParseErr ) m_Targets[iRecordsDone].fInnerW = fVal;
      }
      else if( 0 == ::strcasecmp(field, "innerh") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %f", field, &fVal));
         if( !bParseErr ) m_Targets[iRecordsDone].fInnerH = fVal;
      }
      else if( 0 == ::strcasecmp(field, "ndots") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].nDots = iVal;
      }
      else if( 0 == ::strcasecmp(field, "dotsize") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].nDotSize = iVal;
      }
      else if( 0 == ::strcasecmp(field, "seed") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].iSeed = iVal;
      }
      else if( 0 == ::strcasecmp(field, "coher") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].iPctCoherent = iVal;
      }
      else if( 0 == ::strcasecmp(field, "noiseupd") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].iNoiseUpdIntv = iVal;
      }
      else if( 0 == ::strcasecmp(field, "noiselimit") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %d", field, &iVal));
         if( !bParseErr ) m_Targets[iRecordsDone].iNoiseLimit = iVal;
      }
      else if( 0 == ::strcasecmp(field, "dotlife") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %f", field, &fVal));
         if( !bParseErr ) m_Targets[iRecordsDone].fDotLife = fVal;
      }
      else if( 0 == ::strcasecmp(field, "spatialf") )
      {
         nFields = ::sscanf(m_nextLine, " %24s %f %f", field, &fVal, &fVal1);
         if( nFields == 3 ) m_Targets[iRecordsDone].fSpatialFreq[1] = fVal1;
         if( nFields >= 2 ) m_Targets[iRecordsDone].fSpatialFreq[0] = fVal;
         else bParseErr = true;
      }
      else if( 0 == ::strcasecmp(field, "driftaxis") )
      {
         nFields = ::sscanf(m_nextLine, " %24s %f %f", field, &fVal, &fVal1);
         if( nFields == 3 ) m_Targets[iRecordsDone].fDriftAxis[1] = fVal1;
         if( nFields >= 2 ) m_Targets[iRecordsDone].fDriftAxis[0] = fVal;
         else bParseErr = true;
      }
      else if( 0 == ::strcasecmp(field, "gratphase") )
      {
         nFields = ::sscanf(m_nextLine, " %24s %f %f", field, &fVal, &fVal1);
         if( nFields == 3 ) m_Targets[iRecordsDone].fGratPhase[1] = fVal1;
         if( nFields >= 2 ) m_Targets[iRecordsDone].fGratPhase[0] = fVal;
         else bParseErr = true;
      }
      else if( 0 == ::strcasecmp(field, "sigma") )
      {
         bParseErr = (3 != ::sscanf(m_nextLine, " %24s %f %f", field, &fVal, &fVal1));
         if( !bParseErr )
         {
            m_Targets[iRecordsDone].fSigma[0] = fVal;
            m_Targets[iRecordsDone].fSigma[1] = fVal1;
         }
      }
      else if( 0 == ::strcasecmp(field, "folder") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %30s", field, strVal));
         if( !bParseErr ) ::strcpy(m_Targets[iRecordsDone].strFolder, strVal);
      }
      else if( 0 == ::strcasecmp(field, "file") )
      {
         bParseErr = (2 != ::sscanf(m_nextLine, " %24s %30s", field, strVal));
         if( !bParseErr ) ::strcpy(m_Targets[iRecordsDone].strFile, strVal);
      }
      else if( 0 == ::strcasecmp(field, "flicker") )
      {
         bParseErr = (4 != ::sscanf(m_nextLine, " %24s %d %d %d", field, &iVal, &iVal1, &iVal2));
         if(!bParseErr)
         {
            m_Targets[iRecordsDone].iFlickerOn = iVal;
            m_Targets[iRecordsDone].iFlickerOff = iVal1;
            m_Targets[iRecordsDone].iFlickerDelay = iVal2;
         }
      }
      else
         bParseErr = true;

      if( bParseErr )
      {
         fprintf(stderr, "RMVIoSim: Parsing error at line %d!\n", m_iLineNumber);
         bOk = false;
      }
   }

   if( bOk )                                                               // success: now CRMVDisplay most retrieve
   {                                                                       // the records from CRMVIoSim!
      m_iState = SIM_LOADING;
      m_nTgtsAnimated = n;
      m_nEnumSoFar = 0;
   }

   return( bOk ? RMV_CMD_LOADTARGETS : RMV_CMD_EXIT );
}

//=== processAnimationCommands ========================================================================================
//
//    After a set of targets is defined, CRMVIoSim expects the next non-comment line in the simulated command file to
//    initiate the animation sequence.  This must be "start N", where N is the number of segments in the animation.
//    This method processes the entire series of command lines between "start N" and "stop T", constructing an internal
//    representation of the segmented animation sequence.  See class header for the required format of this command
//    set.  The method requires that each "seg T" command have a start time T that is greater than the previous "seg"
//    command -- ie, the animation segments must be defined in chronological order.  Furthermore, the first segment
//    must have a start time T=0.  There must also be exactly N "seg" commands, and "stop" must follow the command
//    lines defining target motion for the last segment.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID.  This will be "startAnimation" if the function was successful; otherwise, the
//       "exit" command is issued and an error message is posted to stderr (file I/O or parsing error).
//
int CRMVIoSim::processAnimationCommands()
{
   char cmd[25];
   int  n, iVal;
   float fVal, fVal1;

   float fFrameSec = m_fFramePeriodUS / 1000000.0f;                        // to convert vel from deg/sec to deg/frame

   ::memset( m_Trajectories, 0,                                            // reset internal rep of segmented animation
      IOSIM_MAXSEGS*IOSIM_MAXTGTS*sizeof(CTraj) );
   ::memset( m_fSegStart, 0, IOSIM_MAXSEGS*sizeof(float) );
   ::memset( m_bSyncAtSegStart, 0, IOSIM_MAXSEGS*sizeof(bool) );

   bool bOk = true;
   bool bGotStart = false;                                                 // set once we've parsed "start N"
   bool bGotFirstSeg = false;                                              // set once we've parsed first "seg" cmd
   int nSegs = -1;
   int iSegsDone = -1;                                                     // consume all command lines in file between
   while( bOk && ((!bGotStart) || (iSegsDone < nSegs)) )                   // "start N" and "stop"
   {
      if( NULL == ::fgets(m_nextLine, IOSIM_MAXLINELEN, m_pFile ) )        // get next text line from file
      {
         fprintf(stderr, "RMVIoSim: Error reading file at line %d!\n", m_iLineNumber);
         bOk = false;
         break;
      }
      ++m_iLineNumber;

      if( 1 != ::sscanf(m_nextLine, " %24s", cmd) )                        // get command name on line, ignoring
         continue;                                                         // leading whitespace.
      else if( cmd[0] == '#' )                                             // ignore comment lines!
         continue;

      n = ::sscanf(m_nextLine," %24s %d %f %f",cmd, &iVal, &fVal, &fVal1); // parse the command line

      bool bParseErr = false;
      if( !bGotStart )                                                     // first command line MUST be "start N"
      {
         bParseErr = ((n<2) || (0 != ::strcasecmp(cmd, "start")) || (iVal<1 || iVal>IOSIM_MAXSEGS));
         if( !bParseErr )
         {
            bGotStart = true;
            nSegs = iVal;
            bGotFirstSeg = false;
         }
      }
      else if( !bGotFirstSeg )                                             // second command line MUST be "seg 0"
      {
         bParseErr = ((n<2) || (0 != ::strcasecmp(cmd, "seg")) || (iVal!=0));
         if( !bParseErr )
         {
            bGotFirstSeg = true;
            iSegsDone = 0;
            m_fSegStart[iSegsDone] = 0.0f;
         }
      }
      else if( 0 == ::strcasecmp(cmd, "stop") )                            // upon "stop", must have gotten N segs;
      {                                                                    // Also, stop time must be greater than the
         ++iSegsDone;                                                      // start time of the last segment.
         bParseErr = ((n<2) || (iSegsDone < nSegs) ||
               (m_fSegStart[iSegsDone-1] >= float(iVal)));
         if( !bParseErr )
            m_fStopTime =float(iVal);
      }
      else if( 0 == ::strcasecmp(cmd, "seg") )                             // start next segment
      {
         ++iSegsDone;
         bParseErr = ((n<2) || (iSegsDone == nSegs) ||                     //    too many segments, bad format, or segs
               (m_fSegStart[iSegsDone-1] >= float(iVal)));                 //    not in chronological order!
         if( !bParseErr )
         {
            m_fSegStart[iSegsDone] = float(iVal);
            m_bSyncAtSegStart[iSegsDone] = false;                          //    no sync flash unless we get "sync 1"
            for( int i=0; i<m_nTgtsAnimated; i++ )                         //    start out w/ same traj as prev seg!
               m_Trajectories[iSegsDone][i] = m_Trajectories[iSegsDone-1][i];
         }
      }
      else if(0 == ::strcasecmp(cmd, "sync"))                              // "sync N": enable sync flash if N!=0
      {
         bParseErr = (n!=2);
         if(!bParseErr) m_bSyncAtSegStart[iSegsDone] = (iVal!=0);
      }
      else if( 0 == ::strcasecmp(cmd, "onoff") )                           // "onoff N F": turn tgt on (F!=0) or off
      {
         bParseErr = ((n<3) || (iVal<0) || (iVal>=m_nTgtsAnimated));
         if( !bParseErr )
            m_Trajectories[iSegsDone][iVal].bOn = (fVal!=0);
      }
      else if( 0 == ::strcasecmp(cmd, "pos") )                             // "pos N H V": set tgt position (H,V)
      {
         bParseErr = ((n!=4) || (iVal<0) || (iVal>=m_nTgtsAnimated));
         if( !bParseErr )
         {
            m_Trajectories[iSegsDone][iVal].fPos[0] = fVal;
            m_Trajectories[iSegsDone][iVal].fPos[1] = fVal1;
         }
      }
      else if( 0 == ::strcasecmp(cmd, "winvel") )                          // "winvel N H V": set tgt velocity (H,V)
      {
         bParseErr = ((n!=4) || (iVal<0) || (iVal>=m_nTgtsAnimated));
         if( !bParseErr )
         {
            m_Trajectories[iSegsDone][iVal].fVel[0] = fVal * fFrameSec;
            m_Trajectories[iSegsDone][iVal].fVel[1] = fVal1 * fFrameSec;
         }
      }
      else if( 0 == ::strcasecmp(cmd, "patvel") )                          // "patvel N H V": set tgt pattern vel
      {
         bParseErr = ((n!=4) || (iVal<0) || (iVal>=m_nTgtsAnimated));
         if( !bParseErr )
         {
            m_Trajectories[iSegsDone][iVal].fPatVel[0] = fVal * fFrameSec;
            m_Trajectories[iSegsDone][iVal].fPatVel[1] = fVal1 * fFrameSec;
         }
      }
      else
         bParseErr = true;

      if( bParseErr )
      {
         fprintf(stderr, "RMVIoSim: Parsing error at line %d!\n", m_iLineNumber);
         bOk = false;
      }
   }

   if( bOk )                                                               // success: prepare to start animation
   {
      m_nSegments = nSegs;
      m_iCurrSeg = 0;
      m_bAtSegStart = true;
      m_bSyncOn = m_bSyncAtSegStart[m_iCurrSeg];
      m_fElapsedTime = 0.0f;
      m_nEnumSoFar = 0;
      m_iState = CRMVIoSim::SIM_STARTING;
   }

   return( bOk ? RMV_CMD_STARTANIMATE : RMV_CMD_EXIT );
}

//=== doNextFrame =====================================================================================================
//
//    Advance to the next display frame of an ongoing animation sequence.
//
//    ARGS:       NONE.
//    RETURNS:    The next command ID.  This will always be "updateFrame" or "stopAnimation".
//
int CRMVIoSim::doNextFrame()
{
   int nextCmd;                                                            // ID of next command to deliver

   m_fElapsedTime += m_fFramePeriodUS / 1000.0f;                           // update elapsed time in ms
   if( m_fElapsedTime >= m_fStopTime )                                     // we're done!
   {
      m_nTgtsAnimated = 0;
      m_nSegments = 0;
      m_fElapsedTime = 0.0f;
      m_iState = CRMVIoSim::SIM_IDLE;
      nextCmd = RMV_CMD_STOPANIMATE;
   }
   else
   {
      nextCmd = RMV_CMD_UPDATEFRAME;
      m_nEnumSoFar = 0;
      m_iState = CRMVIoSim::SIM_UPDATING;

      if( (m_iCurrSeg+1 < m_nSegments) &&                                 // just crossed a segment boundary!
          (m_fSegStart[m_iCurrSeg+1] <= m_fElapsedTime) )
      {
         ++m_iCurrSeg;
         m_bAtSegStart = true;
         m_bSyncOn = m_bSyncAtSegStart[m_iCurrSeg];
      }
      else
         m_bAtSegStart = false;
   }

   return( nextCmd );
}


