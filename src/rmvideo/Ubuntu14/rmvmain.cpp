/** rmvmain.c =========================================================================================================
 The entry point for the RMVideo application.

 The main() method checks for the command-line argument, in which case RMVideo sets up its network connection with
 Maestro. Without that argument, RMVideo uses an emulated command session parsed from the msimcmds.txt file.

 It also attempts to lock process memory to avoid page faults -- part of our "soft real-time" strategy to optimize
 RMVideo's performance.

 24apr2019-- Removed support for "restart". This existed so that Maestro could download a new RMVideo executable over
 the Maestro-RMVideo communication link, then restart using the new executable. This RMVideo update feature was 
 removed in Maestro v3.2.1 in 2016.
*/

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "rmvdisplay.h"

/**
 SIGINT handler. This overly simplistic handler rudely exits the process. No cleanup or anything. The only reason it's
 here is so that, if RMVideo is started in a script and then interrupted via Ctrl-C, any commands in the script that
 appear after "rmvideo connect" will still be executed. The "launchRMVideo" script includes commands that enable 
 sync on vertical blank, turn off screensaver, and disable display power management before starting RMVideo; the
 commands after "rmvideo connect" reverse those changes. 
*/
void sigIntHandler(int sig)
{
   exit(0);
}

int main( int argc, char **argv )
{
   signal(SIGINT, sigIntHandler);

   // we emulate the communication link with Maestro unless the argument "connect" is passed to RMVideo
   bool bEmulate = true;
   if( argc > 1 )
   {
      if( strcmp("connect", argv[1]) == 0 )
         bEmulate = false;
   }

   fprintf(stderr, "Starting RMVideo, version=%d. Using %s...\n\n", RMV_CURRENTVERSION, 
            bEmulate ? "emulated command session" : "network communication link");

   // attempt to lock all process memory to avoid page faults
   bool bLocked = (mlockall( MCL_CURRENT|MCL_FUTURE ) == 0);
   if( !bLocked )
      ::fprintf(stderr, "RMVideo: (warning) Unable to lock process memory to avoid page faults!\n" );

   // attempt to create the RMVideo display manager
   CRMVDisplay* pRMVDisplay = new CRMVDisplay();
   if( pRMVDisplay == NULL )
   {
      ::fprintf(stderr, "RMVideo: Unable to allocate memory for our OGL fullscreen renderer\n");
      exit(1);
   }

   // run the display manager until a fatal error occurs or RMVideo is "told" to die.
   pRMVDisplay->start(bEmulate);

   // clean up
   delete pRMVDisplay;

   munlockall();

   return( 0 );
}
