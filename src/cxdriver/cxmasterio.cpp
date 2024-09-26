//=====================================================================================================================
//
// cxmasterio.cpp : Implementation of class CCxMasterIO, which encapsulates MAESTRODRIVER interactions w/ MAESTRO.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxMasterIO encapsulates the interprocess communications (IPC) interface between MAESTRODRIVER and its Win32 master
// process, MAESTRO.  This interface is currently embodied by a single, large shared memory object whose organization
// is defined by the CXIPCSM data struct in CXIPC.H.  Included in the struct are all the status variables, data buffers,
// etc. that are jointly accessed by MAESTRODRIVER and MAESTRO.  In addition, synchronization variables -- such as REQ
// and ACK gate flags and queue index pointers -- are provided to coordinate interprocess accesses to the joint data.
//
// The purpose of CCxMasterIO is to hide the ugly details of this interface from the rest of MAESTRODRIVER. A companion
// class, CCxRuntime, serves a similar role on the MAESTRO side. The implementation of these two classes is, of course,
// very tightly interwoven.
//
// To address the real-time and hardware communications requirements of MAESTRO, MAESTRODRIVER runs as an RTSS process
// in the RTX subsystem of WinNT (CREDITS).  The shared memory object is created in nonpaged system RAM via RTX,
// thereby avoiding page faults whenever the shared memory is accessed -- which is OFTEN!
//
// ==> Usage.
// To open the interface, merely instantiate the CCxMasterIO object and call the Open() method.  Note that Open() will
// fail if the MAESTRO side has not already created the shared memory IPC object.  This enforces the idea that the
// MAESTRODRIVER "slave" is spawned by its MAESTRO "master". One MUST call Open() successfully before using any other
// public methods in the class. Else, you may get NULL pointer exceptions, and you'll certainly get incorrect behavior!
//
// Command/response exchanges:  One of the main ways that MAESTRO communicates with MAESTRODRIVER is by issuing various
// "commands", to which MAESTRODRIVER replies with appropriate "responses". With some notable exceptions, these 
// command-response exchanges are mostly for tasks that can be very rapidly (<10ms, say) accomplished by MAESTRODRIVER, 
// since -- by design -- MAESTRO "blocks" while it waits for MAESTRODRIVER's response to a command. Integer data, 
// floating-point data, and/or ASCII character strings may be passed with the command and/or response. Several 
// CCxMasterIO methods provide access to the command/response framework with the IPC interface:
//
//    1) GetCommand() polls for the next MAESTRO command. It must be invoked regularly and often to ensure maximum
//       responsiveness to MAESTRO commands.
//    2) GetCommandData() retrieves any data associated with the current pending command.
//    3) AckCommand() is invoked to respond to the current pending command.
//
// See CXIPC.H for a complete list of supported commands.
//
// Operational mode switches:  MAESTRO/MAESTRODRIVER operate in one of four modes (idle, test, trials, continuous).
// MAESTRO initiates the mode switch via command.  To change the operational mode in response to such a command, use
// CCxMasterIO::SetMode().  To check the current mode, call GetMode().
//
// Other methods are used to post a request to the MAESTRO master.  These requests typically involve displaying info to
// the user on the GUI, such as an informational message [see method Message()].
//
//
// CREDITS:
// 1) Real-Time eXtenstion (RTX) to WinNT by VenturCom, Inc (www.vci.com).  The MAESTRODRIVER app is designed to run as
// an "RTSS" process within the RTX subsystem.  RTX gives the WinNT OS real-time-like characteristics and provides
// kernel access for direct communications with hardware devices -- obviating the need to write kernel-level device
// drivers for the hardware used by MAESTRO.
//
//
// REVISION HISTORY:
// 11may2001-- Began development of CCxMasterIO to encapsulate the shared memory IPC construct used to communicate w/
//             CNTLRX, as well as handle functionality which modifies or accesses data within that construct.
// 14may2001-- Completed initial version supporting only IdleMode and some limited operations on AI & AO in TestMode.
// 21jun2001-- Redesign.  Introduction of the command/response framework.  After startup, mode switches are now
//             command-driven.  Mode-specific functionality removed from this interface.
// 28jun2001-- Reworked methods for getting/setting hardware info.  New hardware info made available: the # of digital
//             inputs & outputs on the event timer device.
// 10aug2001-- Adding support for streaming data thru shared memory for display on the CNTRLX GUI...
// 24oct2001-- Added support for eye-target position plot facility.  See UpdatePosPlot().
// 08jan2002-- Adding support for accessing the "current target list" now maintained in IPC shared memory.
// 26mar2002-- Added read-only option flag CXIPC.bChairPresent.  Access via CCxMasterIO::IsChairPresent().
// 18mar2002-- Minor mod IAW changes to CXIPC:  spike waveform data will NOT be saved to a separate file...
// 21oct2002-- Mod IAW changes to CXIPC.
// 24feb2003-- Added MapTrialTargetIndex( int i ), which translates pos in trial tgt list to pos in loaded tgt list.
// 13oct2003-- Minor bug fix to UpdateTraces().  Had the meaning of 'iTraceEnd' wrong.  The new samples go in the slot
//             iTraceEnd, NOT (iTraceEnd+1)%CX_TRBUFSZ...
// 29dec2003-- Modified to support relocating the Maestro/CXDRIVER installation in any chosen directory.  Maestro gets
//             the install directory from the registry and stores it in CXIPC.strDataPath prior to starting CXDRIVER.
//             CCxMasterIO::Open() saves this install path in a private member variable and makes it available via
//             GetHomeDirectory().
// 07jul2004-- Added inline method SetLastTrialLen() for reporting elapsed time of trial just presented in TrialMode.
//             Elapsed time in ms is stored in CXIPC.iLastTrialLen.
// 10mar2005-- Added methods to reset or add to the new "cumulative reward" statistic in CXIPC.iTotalRewardMS.
// 15jun2005-- Comments to UpdatePosPlot() IAW the addition of a new locus in the eye-target plot -- the "cursor
//             tracking" target in Cont mode.
// 24oct2005-- Added method GetTrialAltXYDotSeed() to expose CXIPC.iXYDotSeedAlt -- alternate XY dot seed for the
//             current trial defined in IPC.
// 05dec2005-- Added inline method SetRPDistroBehavResp() for reporting measured behavioral response for a "R/P Distro"
//             trial just presented in TrialMode.  Response (eye vel magnitude in deg/s) is stored in CXIPC.fResponse.
// 14mar2006-- Removed IsOKNAvailable().  As of Maestro 1.5.0, OKNDRUM no longer exists!
// 27mar2006-- Modified to introduced RMVideo as a replacement for the old VSG-based framebuffer video.
// 21apr2006-- Added get/set functions for some RMVideo display properties added to CXIPC.
// 01sep2009-- Begun revisions to support major new RMVideo features: video mode switching, setting the monitor's gamma
//             correction factors, and managing the content of the RMVideo movie store, in which movie files are kept
//             for playback using the new target type RMV_MOVIE.
// 28apr2010-- Added GetStartPosH,V() in support of reporting the global target starting position offset in effect 
//             during a trial. This info is stored in the trial data file along with other global transform factors.
// 24may2010-- Added GetTrialFlags() to support reporting the state of trial flag bits. The trial flags are stored in 
//             the trial data file header.
// 11may2011-- Added GetVStabSlidingWindow(), which returns the user-specified length of the sliding window average of
//             eye position used to smooth the effects of velocity stabilization in TrialMode. Restricted to [1..20]
//             ms. A value of 1 disables the feature.
// 07nov2017-- Mods to fix compilation issues in VS2017 for Win10/RTX64 build. 
// 11jun2018-- Added GetTrialSetName, GetTrialSubsetName.
// 19mar2019-- Added AllowRMVDuplicateFramesDuringTrial() to access CXIPC.bTolRMVDuplFrame. This provides a mechanism
// by which the user can elect to tolerate up to 3 duplicate frames in RMVideo before aborting a trial. It will likely
// not affect current experiments running at 80Hz, but it could be important if we are able to support RMVideo at
// higher refresh rates up to 200Hz.
// 05sep2019-- Added GetDOBusyWait() to access new field CXIPC.fDOBusyWaits[]. These are 3 busy wait times for the
// CCxEventTimer::SetDO() call. They're in CXIPC because they are stored in a Maestro-specific Windows registry entry,
// and Maestro communicate them to CXDRIVER over IPC at startup. NOTE that we tried to access the registry directly
// in CCxMasterIO::Open(), but RegQueryValueEx() consistently crashed the system.
//=====================================================================================================================

#include <windows.h>                   // standard Win32 includes
#include <stdio.h>                     // runtime C/C++ I/O library
#include "rtapi.h"                     // the RTX API
#include "cxmasterio.h"


//=====================================================================================================================
// CONSTANTS INITIALIZED
//=====================================================================================================================

const TRIALCODE CCxMasterIO::illegalTRIALCODE = {-1,-1};


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxMasterIO [constructor] =========================================================================================
//
//    Object is initialized to reflect fact that the communication link w/ MAESTRO has not yet been opened.
//
CCxMasterIO::CCxMasterIO()
{
   m_hSharedIPC = (HANDLE) NULL;
   m_pvIPC = (PCXIPCSM) NULL;
   ::memset( &(m_strHome[0]), 0, CX_MAXPATH );
}


//=== ~CCxMasterIO [destructor] =========================================================================================
//
//    [Nothing to do for now...]
//
CCxMasterIO::~CCxMasterIO()
{
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Open ============================================================================================================
//
//    Open interprocess communications (IPC) link with MAESTRO. IPC is achieved via an RTX shared memory object that is
//    is initially created by the master process.  Here we request a handle to that object and cast the memory in the
//    form of a CXIPCSM struct (see CXIPC.H). If this call fails, then comm w/MAESTRO is not possible.
//
//    We also save the path to the Maestro installation directory, which is stored in CXIPCSM.strDataPath when Maestro
//    starts MAESTRODRIVER.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful, FALSE otherwise.
//
BOOL RTFCNDCL CCxMasterIO::Open()
{
   m_hSharedIPC = ::RtOpenSharedMemory( SHM_MAP_ALL_ACCESS, FALSE, CXIPC_SHM, (VOID **) &m_pvIPC );
   if( m_hSharedIPC == (HANDLE) NULL )
      return( FALSE );

   ::strcpy_s( m_strHome, &(m_pvIPC->strDataPath[0]) );        // remember the installation directory!

   m_pvIPC->iOpMode = CX_STARTING;                                   // this puts CXDRIVER in startup "mode"
   return( TRUE );
}


//=== Close ===========================================================================================================
//
//    Release the shared memory object used for interprocess communications with the CNTRLX master process.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxMasterIO::Close()
{
   if( m_hSharedIPC != (HANDLE) NULL )
      ::RtCloseHandle( m_hSharedIPC );
   m_hSharedIPC = (HANDLE) NULL;
   m_pvIPC = (PCXIPCSM) NULL;
}

/**
 Reset all hardware information in the IPC shared memory structure to indicate no hardware is present.
*/
VOID RTFCNDCL CCxMasterIO::ResetHardwareInfo()
{
   if(m_pvIPC != NULL)
   {
      m_pvIPC->dwHWStatus = 0;
      m_pvIPC->nAOChannels = 0;
      m_pvIPC->nAIChannels = 0;
      m_pvIPC->nTDOChannels = 0;
      m_pvIPC->nTDIChannels = 0;
   }
}

//=== Message =========================================================================================================
//
//    Request that MAESTRO post the specified message string to the user.
//
//    MAESTRO/MAESTRODRIVER's IPC interface (see CXIPC.H) includes a "circular" message queue.  This method handles the
//    details of posting a new message to that queue.  The queue is circular in the sense that:
//          next_available_msg_slot = (current_slot + 1) % queue_size
//    When the queue is full (because MAESTRODRIVER has generated messages faster than they can be handled on the
//    MAESTRO side, further posts fail until an empty message slot becomes available.
//
//    ARGS:       pszMsg -- [in] the message string
//
//    RETURNS:    TRUE if message was successfully queued; FALSE if queue was full (message not posted).
//
BOOL RTFCNDCL CCxMasterIO::Message( LPCTSTR pszMsg )
{
   int iNextSlot = (m_pvIPC->iNextMsgToPost + 1) % CXIPC_MSGQLEN;             // get next available slot in msg queue
   if( iNextSlot == m_pvIPC->iLastMsgPosted )                                 // msg queue full -- unable to post msg
      return( FALSE );
   else                                                                       // else: put msg in next available slot
   {                                                                          // in queue, truncating it if needed
      ::sprintf_s( m_pvIPC->szMsgQ[iNextSlot], "%.*s\0", CXIPC_MSGSZ-1, pszMsg );
      m_pvIPC->iNextMsgToPost = iNextSlot;
      return( TRUE );
   }
}


//=== InitTrace =======================================================================================================
//
//    Initialize the data trace facility.  See UpdateTrace() for details.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CCxMasterIO::InitTrace()
{
   if( m_pvIPC == NULL ) return( FALSE );
   m_pvIPC->iTraceEnd = 0;
   m_pvIPC->iTraceDrawn = 0;
   m_pvIPC->bTraceOverflow = FALSE;
   return( TRUE );
}


//=== UpdateTrace =====================================================================================================
//
//    Update data trace buffers with the provided channel data for the current time epoch.
//
//    The Maestro "data trace facility" displays selected data channels on the GUI during runtime.  The data is
//    streamed thru a set of circular "trace buffers" existing in shared memory.  CCxMasterIO administers this trace
//    facility, but callers are responsible for providing the acquired channel data via calls to UpdateTrace().  The
//    method MUST be called once per sample period ("time epoch"), or the displayed data traces will not accurately
//    reflect the acquired data streams.  Caller must provide a single sample from every data channel supported by
//    Maestro/CXDRIVER (analog inputs, various "computed channels", and digital inputs).  However, Maestro can only
//    display up to CX_NTRACES at a time.  UpdateTrace() uses identifying info provided by Maestro in shared memory to
//    direct the desired data to the appropriate trace buffer.
//
//          !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//          !!! UpdateTrace() must be as efficient as possible.  We therefore TRUST that the data trace info
//          !!! provided by Maestro (#traces, trace type & channel #) are valid, as we do not want to spend any
//          !!! CPU time doing laborious validity checks!
//          !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//    ARGS:       pshAI       -- [in] analog input channel vector for current time epoch.  Length MUST = # of existing
//                               AI channels.
//                pshComp     -- [in] "computed" channel vector for current time epoch.  Length MUST = # of existing
//                               computed channels.  May be NULL, in which case each sample is assumed to be zero.
//                dwEvtMask   -- [in] digital input channel mask for current time epoch.
//
//    RETURNS:    NONE.
//
BOOL RTFCNDCL CCxMasterIO::UpdateTrace( short* pshAI, short* pshComp, DWORD dwEvtMask )
{
   if( (m_pvIPC == NULL) || m_pvIPC->bTraceOverflow ) return( FALSE );

   if( m_pvIPC->nTracesInUse == 0 ) return( TRUE );

   int iNextSlot = m_pvIPC->iTraceEnd;                                     // here's where we put the new samples in
                                                                           // the trace buffers
   if( ((iNextSlot + 1) % CX_TRBUFSZ) == m_pvIPC->iTraceDrawn )            // overflow ERROR -- trace buffers are full
   {
      m_pvIPC->bTraceOverflow = TRUE;                                      // data trace facility is halted on error
      return( FALSE );                                                     // and must be reinitialized
   }

   for( int i=0; i < m_pvIPC->nTracesInUse; i++ )                          // update the trace buffers with the new
   {                                                                       // samples...
      int nCh = m_pvIPC->iTraceCh[i];
      switch( m_pvIPC->iTraceType[i] )
      {
         case CX_AITRACE :
            m_pvIPC->shTraceBuf[i][iNextSlot] = pshAI[nCh];
            break;
         case CX_CPTRACE :
            m_pvIPC->shTraceBuf[i][iNextSlot] = (pshComp==NULL) ? 0 : pshComp[nCh];
            break;
         case CX_DITRACE :
            m_pvIPC->shTraceBuf[i][iNextSlot] = (short) ((dwEvtMask & (1<<nCh)) != 0) ? 1 : 0;
            break;
      }
   }

   m_pvIPC->iTraceEnd = (iNextSlot + 1) % CX_TRBUFSZ;                      // advance to the next open slot
   return( TRUE );
}


//=== InitEventStream =================================================================================================
//
//    Initialize the digital event stream buffers and clear event stream overflow flag.
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if successful; FALSE otherwise.
//
BOOL RTFCNDCL CCxMasterIO::InitEventStream()
{
   if( m_pvIPC == NULL ) return( FALSE );
   m_pvIPC->iEventEnd = 0;
   m_pvIPC->iEventConsumed = 0;
   m_pvIPC->bEventOverflow = FALSE;
   return( TRUE );
}


//=== UpdateEventStream ===============================================================================================
//
//    Update digital event data buffers with a new event.
//
//    Whenever Maestro enables it, MAESTRODRIVER can stream the digital event data through IPC.  The digital input
//    event bit mask and timestamp are streamed through circular buffers in IPC.  As with the data trace facility,
//    CCxMasterIO controls access to the buffers, but CCxDriver must call this method to provide the actual timestamp
//    data.  Note that this facility is only used during trial execution, and the timestamp resolution should be 1ms.
//
//    ARGS:       dwEvent  -- [in] the state of the digital inputs (a bit mask: bitN = state of DI chan N) when an
//                            event was detected on at least one of them.
//                time     -- [in] the time of the event.  Maestro will assume this is trial time in milliseconds!
//
//    RETURNS:    TRUE if successful; FALSE if event data buffers overflowed.
//
BOOL RTFCNDCL CCxMasterIO::UpdateEventStream( DWORD dwEvent, int time )
{
   if( (m_pvIPC == NULL) || m_pvIPC->bEventOverflow ) return( FALSE );

   if( !m_pvIPC->bEventEnable ) return( TRUE );

   int iNextSlot = m_pvIPC->iEventEnd;                                     // here's where we put the next event
   if( ((iNextSlot + 1) % CX_EVTBUFSZ) == m_pvIPC->iEventConsumed )        // overflow ERROR -- buffers are full
   {
      m_pvIPC->bEventOverflow = TRUE;                                      // event streaming is halted on error
      return( FALSE );                                                     // and must be reinitialized
   }

   m_pvIPC->dwEventMaskBuf[iNextSlot] = dwEvent;                           // add event mask and time to buffers
   m_pvIPC->iEventTimeBuf[iNextSlot] = time;

   m_pvIPC->iEventEnd = (iNextSlot + 1) % CX_EVTBUFSZ;                     // advance to the next open slot
   return( TRUE );
}


//=== UpdatePosPlot ===================================================================================================
//
//    Update loci on the MAESTRO eye-target position plot.
//
//    The MAESTRO "eye-target position plot facility" displays on the GUI the XY positions (in visual deg) of several
//    key loci:  the subject's eye, a second "eye" for special-purpose use, fixation targets 1 & 2, the subject's head
//    (chair position), and the "cursor tracking" target (Cont mode only).  CCxMasterIO administers this facility, but
//    callers are responsible for updating the position of loci via calls to this method.  Position updates should be
//    posted frequently (every 30ms or less) to provide relatively smooth animation.  Since the position plot is a
//    relatively low-priority resource, this method merely drops any plot update requests that occur while MAESTRO is
//    still servicing a previous update.
//
//    IMPORTANT:  While new position updates should be posted every 30ms or so, this method should be called ~every
//    1-2ms with a NULL argument, in which case it merely attempts to complete the REQ/ACK handshake for the previous
//    plot update.  Failure to do so will mean that every other plot update will be dropped!
//
//    This facility is available in only TrialMode or ContMode!
//
//    DEV NOTE:  This is an awkward implementation.  But it's NON-BLOCKING!
//
//    ARGS:       pPtLoci  -- [in] new positions of loci in order: eye, eye2, fix targ 1, fix targ 2, chair, or "track"
//                            target.  Units taken as hundredths of visual degrees.  If NULL, we only attempt to
//                            complete the REQ/ACK handshake for the previous plot update.
//
//    RETURNS:    TRUE if position plot was updated; FALSE otherwise (nothing to update, or req dropped).
//
BOOL RTFCNDCL CCxMasterIO::UpdatePosPlot( LPPOINT pPtLoci )
{
   if( m_pvIPC == NULL ) return( FALSE );

   if( m_pvIPC->bReqPlot || m_pvIPC->bAckPlot )                            // previous plot update pending...
   {
      if( m_pvIPC->bReqPlot && m_pvIPC->bAckPlot )                         // ...complete handshake
         m_pvIPC->bReqPlot = FALSE;
      return( FALSE );
   }
   else if( (pPtLoci != NULL) &&                                           // ready to req a new plot update
            (m_pvIPC->iOpMode==CX_TRIALMODE || m_pvIPC->iOpMode==CX_CONTMODE)
          )
   {
      memcpy( &(m_pvIPC->ptLoci[0]), pPtLoci, sizeof(POINT)*CX_NLOCI );
      m_pvIPC->bReqPlot = TRUE;
      return( TRUE );
   }
   else
      return( FALSE );
}

//=== IsUsingRMVTargets ===============================================================================================
//
//    Are there any RMVideo framebuffer video targets (type CX_RMVTARG) included in the current target list?
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if at least one RMVideo target is defined; FALSE otherwise.
//
BOOL RTFCNDCL CCxMasterIO::IsUsingRMVTargets()
{
   int n = GetNumTargets();
   for( int i = 0; i < n; i++ )
   {
      if( m_pvIPC->targets[i].wType == CX_RMVTARG ) return( TRUE );
   }
   return( FALSE );
}


//=== GetNumTrialTargets, MapTrialTargetIndex, GetTrialTarget, GetTrialTargetType, GetTrialTargetSubtype ==============
//
//    Retrieve information about the targets participating in the trial currently defined in IPC.  Valid only in
//    CX_TRIALMODE.
//
//    BACKGROUND: During CX_TRIALMODE, MAESTRO prepares each trial definition and stores it in IPC prior to instructing
//    MAESTRODRIVER to execute the trial.  The trial definition consists of a set of "trial codes" (CXIPC.trialCodes[])
//    and a "target map" (CXIPC.iTgMap[]).  A participating target is identified in the trial codes by its ordinal
//    position in the trial target map which, in turn, points to the location of the target's definition record in the
//    "current target list" (CXIPC.targets[]).
//
//    ARGS:       i     -- [in] ordinal pos of target in the trial target map.
//                tgt   -- [out] holds the trial target defn, if successful; otherwise, it is unchanged.
//
//    RETURNS:    GetNumTrialTargets():  #tgts participating in trial; 0 if not in CX_TRIALMODE.
//                MapTrialTargetIndex(): ordinal pos of trial target in the "current target list"; -1 if invalid.
//                GetTrialTarget():  TRUE if successful, FALSE otherwise.
//                GetTrialTargetType():  tgt type if successful, 0 otherwise.
//                GetTrialTargetSubtype(): tgt subtype for RMVideo targets, -1 otherwise.
//
int RTFCNDCL CCxMasterIO::GetNumTrialTargets()
{
   int n = (GetMode() == CX_TRIALMODE) ? m_pvIPC->nTrialTgts : 0;
   if( n > MAX_TRIALTARGS ) n = MAX_TRIALTARGS;
   return( n );
}

int RTFCNDCL CCxMasterIO::MapTrialTargetIndex( int i )
{
   int n = GetNumTrialTargets();
   if( (n > 0) && (i >= 0) && (i < n) && (m_pvIPC->iTgMap[i] >= 0) && (m_pvIPC->iTgMap[i] < m_pvIPC->nTgts) )
      return( m_pvIPC->iTgMap[i] );
   else
      return( -1 );
}

BOOL RTFCNDCL CCxMasterIO::GetTrialTarget( int i, CXTARGET& tgt )
{
   int iPos = MapTrialTargetIndex( i );
   if( iPos >= 0 )
   {
      tgt = m_pvIPC->targets[ iPos ];
      return( TRUE );
   }
   return( FALSE );
}

WORD RTFCNDCL CCxMasterIO::GetTrialTargetType( int i )
{
   int iPos = MapTrialTargetIndex( i );
   return( (iPos >= 0) ? m_pvIPC->targets[iPos].wType : 0 );
}

int RTFCNDCL CCxMasterIO::GetTrialTargetSubtype( int i )
{
   WORD wType = GetTrialTargetType( i );
   if(wType == CX_RMVTARG)
   {
      CXTARGET* pTg = &(m_pvIPC->targets[ m_pvIPC->iTgMap[i] ]);
      return( pTg->u.rmv.iType );
   }
   return( -1 );
}


//=== GetNumTrialCodes ================================================================================================
//
//    Retrieve the # of "trial codes" for the trial currently defined in IPC.  Valid only in CX_TRIALMODE.
//
//    ARGS:       NONE.
//
//    RETURNS:    # of trial codes; 0 if not in CX_TRIALMODE.
//
int RTFCNDCL CCxMasterIO::GetNumTrialCodes()
{
   int i = (GetMode() == CX_TRIALMODE) ? m_pvIPC->nCodes : 0;
   if( i > CX_MAXTC ) i = CX_MAXTC;
   if( i < 0 ) i = 0;
   return( i );
}


//=== GetTrialCode ====================================================================================================
//
//    Retrieve specified trial code from the trial code in IPC.  Valid only in CX_TRIALMODE.
//
//    ARGS:       i  -- [in] index into trial code array.
//
//    RETURNS:    requested trial code; invalid trial code if not in CX_TRIALMODE or index is out of range.
//
const TRIALCODE& CCxMasterIO::GetTrialCode( const int i )
{
   if( (GetMode() == CX_TRIALMODE) && (i >= 0) && (i < m_pvIPC->nCodes) ) return( m_pvIPC->trialCodes[i] );
   else return( illegalTRIALCODE );
}

//=== GetNumTaggedSections ============================================================================================
//
//    Retrieve the # of "tagged sections" in the trial currently defined in IPC.  Valid only in CX_TRIALMODE.
//
//    ARGS:       NONE.
//
//    RETURNS:    # of tagged sections defined on current trial; 0 if not in CX_TRIALMODE.
//
int RTFCNDCL CCxMasterIO::GetNumTaggedSections()
{
   int i = (GetMode() == CX_TRIALMODE) ? m_pvIPC->nSections : 0;
   if( i > MAX_SEGMENTS ) i = MAX_SEGMENTS;
   if( i < 0 ) i = 0;
   return( i );
}

//=== GetTaggedSection ================================================================================================
//
//    Retrieve a specific tagged section from the section array stored in IPC.  Valid only in CX_TRIALMODE.
//
//    ARGS:       i     -- [in] index into tagged section array.
//                sect  -- [out] the tagged section record retrieved.
//
//    RETURNS:    TRUE if successful; FALSE if not in CX_TRIALMODE or index is out of range.
//
BOOL RTFCNDCL CCxMasterIO::GetTaggedSection( int i, TRIALSECT& sect )
{
   BOOL bOk = (GetMode() == CX_TRIALMODE) && (i >= 0) && (i < m_pvIPC->nSections);
   if( bOk )
      sect = m_pvIPC->trialSections[i];
   return( bOk );
}


//=== GetProtocolName, GetTrial***Name, GetDataFilePath, GetDataFileName ============================================== 
//
// Copy various strings from IPC into the provided buffer:  the current protocol's human-readable name (trial or 
// stimulus run), human-readable names for the trial set and (if applicable) trial subset (Trial mode only); the full
// pathname for the data file in which recorded protocol data is to be saved, or the data filename ("file.ext") instead 
// of the full path.  Valid only in CX_TRIALMODE (for trial protocols) or CX_CONTMODE (for stimulus run protocols).
//
//    ARGS:       pBuf  -- [out] buffer to hold string. if not in CX_TRIALMODE or CX_CONTMODE, set to an empty string.
//                         if not long enough to hold string, the string is truncated to fit.  NULL-terminated.
//                n     -- [in] max # of characters that can be copied to the buffer.
//
//    RETURNS:    NONE.
//
VOID RTFCNDCL CCxMasterIO::GetProtocolName( char *pBuf, int n )
{
   if( n <= 0 ) return;                                  // buffer assumed to be NULL ptr; there's nothing we can do!

   int iMode = GetMode();
   if( iMode != CX_TRIALMODE && iMode != CX_CONTMODE )   // empty string returned if not in Trial or Cont mode
      pBuf[0] = '\0';
   else
   {
      int i = 0;                                         // copy as much of the specified string as we can
      while( i < n && i < CX_MAXOBJNAMELEN )
      {
         pBuf[i] = m_pvIPC->strProtocol[i];
         if( pBuf[i] == '\0' ) break;
         ++i;
      }
      if( i == n || i == CX_MAXOBJNAMELEN ) --i;         // always terminate with a NULL character
      pBuf[i] = '\0';
   }
}

VOID RTFCNDCL CCxMasterIO::GetTrialSetName(char *pBuf, int n)
{
   if(n <= 0) return;                                    // buffer assumed to be NULL ptr; there's nothing we can do!

   int iMode = GetMode();
   if(iMode != CX_TRIALMODE)                             // empty string returned if not in Trial mode
      pBuf[0] = '\0';
   else
   {
      int i = 0;                                         // copy as much of the string as we can
      while(i < n && i < CX_MAXOBJNAMELEN)
      {
         pBuf[i] = m_pvIPC->strSet[i];
         if(pBuf[i] == '\0') break;
         ++i;
      }
      if(i == n || i == CX_MAXOBJNAMELEN) --i;           // always terminate with a NULL character
      pBuf[i] = '\0';
   }
}

VOID RTFCNDCL CCxMasterIO::GetTrialSubsetName(char *pBuf, int n)
{
   if(n <= 0) return;                                    // buffer assumed to be NULL ptr; there's nothing we can do!

   int iMode = GetMode();
   if(iMode != CX_TRIALMODE)                             // empty string returned if not in Trial mode
      pBuf[0] = '\0';
   else
   {
      int i = 0;                                         // copy as much of the string as we can
      while(i < n && i < CX_MAXOBJNAMELEN)
      {
         pBuf[i] = m_pvIPC->strSubset[i];
         if(pBuf[i] == '\0') break;
         ++i;
      }
      if(i == n || i == CX_MAXOBJNAMELEN) --i;           // always terminate with a NULL character
      pBuf[i] = '\0';
   }
}

VOID RTFCNDCL CCxMasterIO::GetDataFilePath( char *pBuf, int n )
{
   if( n <= 0 ) return;                                  // buffer assumed to be NULL ptr; there's nothing we can do!

   int iMode = GetMode();
   if( iMode != CX_TRIALMODE && iMode != CX_CONTMODE )   // empty string returned if not in Trial or Cont mode
      pBuf[0] = '\0';
   else
   {
      int i = 0;                                         // copy as much of the specified string as we can
      while( i < n && i < CX_MAXPATH )
      {
         pBuf[i] = m_pvIPC->strDataPath[i];
         if( pBuf[i] == '\0' ) break;
         ++i;
      }
      if( i == n || i == CX_MAXPATH ) --i;               // always terminate with a NULL character
      pBuf[i] = '\0';
   }
}

VOID RTFCNDCL CCxMasterIO::GetDataFileName( char *pBuf, int n )
{
   if( n <= 0 ) return;                                  // buffer assumed to be NULL ptr; there's nothing we can do!

   int iMode = GetMode();
   if( iMode != CX_TRIALMODE && iMode != CX_CONTMODE )   // empty string returned if not in Trial or Cont mode
      pBuf[0] = '\0';
   else
   {
      int i = (int) (::strlen(m_pvIPC->strDataPath)-1);  // search backwards to find index of char after last path
      while( i >= 0 )                                    // separator. !!! ASSUME separator is '\' !!!
      {
         if( m_pvIPC->strDataPath[i] == '\\' ) break;
         --i;
      }
      ++i;

      int j = 0;
      while( j < n && j < CX_MAXPATH )                   // copy the chars after last path separator into the buffer
      {
         pBuf[j] = m_pvIPC->strDataPath[i+j];
         if( pBuf[j] == '\0' ) break;
         ++j;
      }
      if( j == n || j == CX_MAXPATH ) --j;               // always terminate with a NULL character
      pBuf[j] = '\0';
   }
}


//=== GetCommand, GetCommandData, AckCommand ==========================================================================
//
//    Retrieve command from MAESTRO and respond to that command.
//
//    These methods encapsulate the "command-response" communication framework within the MAESTRO-MAESTRODRIVER shared
//    memory IPC interface.  Intended usage:
//
//       1) Poll GetCommand() until it returns a valid command code.  It should be polled frequently!
//       2) Call GetCommandData() to retrieve any integer or FP data associated with the command (if any). CCxMasterIO
//          contains no knowledge of CNTRLX commands!
//       3) After executing command, prepare response and acknowledge the command via AckCommand().  Every call to
//          GetCommand() that retrieves a valid command code MUST be matched by a call to AckCommand().  If you need to
//          acknowledge a command and then do substantial processing (>~100ms), you should first complete the command-
//          response handshake so that CNTRLX does not "think" the command failed.  In this case, set the bWait flag
//          in AckCommand(), in which case the method will block until the handshake is complete.
//
//    By design, MAESTRO "blocks" upon sending a command to MAESTRODRIVER -- waiting for a matching response.  Thus,
//    the command-response framework is intended only for commands to which MAESTRODRIVER can respond very quickly.
//    There are a few exceptions to this rule, but such commands may only be issued in Idle mode.
//
//    Unlike MAESTRO, these commands (except, possibly, AckCommand()) do not block.  Repeated calls to GetCommand()
//    will complete the command-response "handshake" between MAESTRO and MAESTRODRIVER.  However, callers MUST invoke
//    a matching AckCommand() each time GetCommand() retrieves a new command -- otherwise, the next GetCommand() will
//    reissue the same command.  Also, if a MAESTRO command is not acknowledged, MAESTRO will "time-out" on the command
//    and assume that MAESTRODRIVER is not responding.
//
//    ARGS:       piData   -- [out] buffer for integer-valued command data or [in] response data.
//                pfData   -- [out] buffer for float-valued command data or [in] response data.
//                pcData   -- [out] buffer for char-valued command data or [in] response data.
//                ni       -- [in] # of integers in command or response buffer.
//                nf       -- [in] # of floats in command or response buffer.
//                nc       -- [in] # of characters in command or response buffer.
//                dwRsp    -- [in] response code.
//                bWait    -- [in] if TRUE, AckCommand() blocks waiting for MAESTRO to complete handshake.
//
//    RETURNS:    GetCommand() -- if command present but not ack'd, a valid command code; else, CX_NULLCMD.
//
DWORD RTFCNDCL CCxMasterIO::GetCommand()
{
   if( m_pvIPC->bReqCmd && !m_pvIPC->bAckCmd )                       // a new command has been posted
      return( m_pvIPC->dwCommand );
   else
   {
      if( (!m_pvIPC->bReqCmd) && m_pvIPC->bAckCmd )                  // complete handshake for previously posted cmd
         m_pvIPC->bAckCmd = FALSE;
      return( CX_NULLCMD );
   }
}

VOID RTFCNDCL CCxMasterIO::GetCommandData(
   int* piData, float *pfData, const int ni, const int nf, char *pcData /* = NULL */, const int nc /* = 0 */)
{
   if( m_pvIPC->bReqCmd && !m_pvIPC->bAckCmd )                       // if there's a command pending, copy cmd data...
   {
      int niSafe = (ni > CX_CMDLEN) ? CX_CMDLEN : ((ni < 0) ? 0 : ni);
      if( (piData != NULL) && (niSafe > 0) )
         ::memcpy( piData, &(m_pvIPC->iData[0]), sizeof(int)*niSafe );

      int nfSafe = (nf > CX_CMDLEN) ? CX_CMDLEN : ((nf < 0) ? 0 : nf);
      if( (pfData != NULL) && (nfSafe > 0) )
         ::memcpy( pfData, &(m_pvIPC->fData[0]), sizeof(float)*nfSafe );
         
      int ncSafe = (nc > CX_CDATALEN) ? CX_CDATALEN : ((nc < 0) ? 0 : nc);
      if((pcData != NULL) && (ncSafe > 0) )
         ::memcpy(pcData, &(m_pvIPC->cData[0]), sizeof(char)*ncSafe);
   }
}

VOID RTFCNDCL CCxMasterIO::AckCommand(
   DWORD dwRsp, int *piData, float *pfData, const int ni, const int nf, const BOOL bWait /*= FALSE */, 
   char *pcData /* = NULL */, const int nc /* = 0 */)
{
   if( m_pvIPC->bReqCmd && !m_pvIPC->bAckCmd )                       // if there's a command pending...
   {
      m_pvIPC->dwCommand = dwRsp;                                    // copy response data to IPC interface buffers

      int niSafe = (ni > CX_CMDLEN) ? CX_CMDLEN : ((ni < 0) ? 0 : ni);
      if( (piData != NULL) && (niSafe > 0) )
         ::memcpy( &(m_pvIPC->iData[0]), piData, sizeof(int)*niSafe );

      int nfSafe = (nf > CX_CMDLEN) ? CX_CMDLEN : ((nf < 0) ? 0 : nf);
      if( (pfData != NULL) && (nfSafe > 0) )
         ::memcpy( &(m_pvIPC->fData[0]), pfData, sizeof(float)*nfSafe );

      int ncSafe = (nc > CX_CDATALEN) ? CX_CDATALEN : ((nc < 0) ? 0 : nc);
      if((pcData != NULL) && (ncSafe > 0))
         ::memcpy( &(m_pvIPC->cData[0]), pcData, sizeof(char)*ncSafe );

      m_pvIPC->bAckCmd = TRUE;                                       // acknowledge command

      if( bWait )                                                    // complete handshake if desired
      {
         while( m_pvIPC->bReqCmd ) ;                                 // BLOCKS!
         m_pvIPC->bAckCmd = FALSE;
      }
   }
}


/**
 When Maestro is connected to the Eyelink 1000+ tracker and the user has enabled the tracker for recording eye position
 data, Maestro will stream tracker samples (eye position data) through IPC to MaestroDRIVER in all operational modes
 except idle mode. This method must be invoked frequently to retrieve the samples before the circular sample queue in
 IPC overflows. 

 The Eyelink tracker is considered an alternate eye position signal source when the eye coil system is not available or
 not practical for the experiment (such as human psychophysics).

 @param s The sample retrieved from the queue, if any. If no sample is available, method returns the last tracker sample
 retrieved.
 @param bFlush If TRUE, the queue is flushed and the MOST RECENT sample is returned. Otherwise, the oldest sample in
 the queue is returned.
 @return 1 if a sample is returned, 0 if no sample is available because tracker sample queue is empty, -1 if recording
 session aborted on an error condition, and -2 if the Eyelink tracker is idle (not recording) or not connected.
 */
int RTFCNDCL CCxMasterIO::GetNextEyelinkSample(ELSAMP& s, BOOL bFlush)
{
   // access status and next-available sample index once per invocation; these could get changed on the Win32 side
   int stat = (m_pvIPC != NULL) ? m_pvIPC->iELStatus : CX_ELSTAT_OFF;
   int nxtIdx = m_pvIPC->iELNext; 
   int res = -2;
   if(stat >= CX_ELSTAT_REC)
   {
      if(stat == CX_ELSTAT_FAIL)
         res = -1;
      else if(m_pvIPC->iELLast != nxtIdx)
      {
         int idx = bFlush ? nxtIdx : ((m_pvIPC->iELLast + 1) % CX_MAXEL);
         s = m_pvIPC->elSamples[idx];
         res = 1;
         m_pvIPC->iELLast = idx;
      }
      else 
      {
         s = m_pvIPC->elSamples[m_pvIPC->iELLast];
         res = 0;
      }
   }
   return(res);
}
