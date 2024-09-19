//=====================================================================================================================
//
// cxmasterio.h : Declaration of CCxMasterIO, MAESTRODRIVER's interface with its MAESTRO master process.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXMASTERIO_H__INCLUDED_)
#define CXMASTERIO_H__INCLUDED_

#include "cxipc.h"               // defines the MAESTRO/MAESTRODRIVER interprocess communications (IPC) interface


//=====================================================================================================================
// Declaration of class CCxMasterIO
//=====================================================================================================================
//
class CCxMasterIO
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const TRIALCODE illegalTRIALCODE;     // a generic illegal trial code


//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   HANDLE            m_hSharedIPC;              // handle to shared memory for IPC with MAESTRO
   volatile PCXIPCSM m_pvIPC;                   // ptr to structure in shared memory that embodies IPC

   char              m_strHome[CX_MAXPATH];     // full path for Maestro install directory, reported by Maestro in
                                                // CXIPC.strDataPath when it first starts MAESTRODRIVER


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxMasterIO( const CCxMasterIO& src );                // no copy constructor or assignment operator defined
   CCxMasterIO& operator=( const CCxMasterIO& src );

public:
   CCxMasterIO();
   ~CCxMasterIO();


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL RTFCNDCL Open();                                 // open IPC interface
   VOID RTFCNDCL Close();                                // close IPC interface

   LPCTSTR RTFCNDCL GetHomeDirectory()                   // get full path to installation directory as it was reported
   {                                                     // at application startup
      return( (LPCTSTR) m_strHome );
   }

                                                         // inline methods for accessing data in IPC shared memory:
   int RTFCNDCL GetMode() { return( (m_pvIPC != NULL) ? m_pvIPC->iOpMode : CX_NOTRUNNING ); }
   BOOL RTFCNDCL SetMode( const int i )
   {
      if( (m_pvIPC == NULL) || (i < CX_NOTRUNNING) || (i > CX_CONTMODE) ) return( FALSE );
      m_pvIPC->iOpMode = i;
      return( TRUE );
   }

   /** 
   Can as many as three repeat frames occur on the RMVideo display during a trial? If method returns TRUE, the
   trial will not be aborted unless 4 or more duplicate frames are detected; if FALSE, then a single duplicate
   frame will abort the trial.
   @return TRUE if any repeat frames are allowed in a trial; else FALSE.
   */
   BOOL RTFCNDCL AllowRMVDuplicateFramesDuringTrial()
   {
      return((m_pvIPC != NULL) ? m_pvIPC->bTolRMVDuplFrame : FALSE);
   }

   BOOL RTFCNDCL IsSpikeTraceOn() { return( (m_pvIPC != NULL) ? m_pvIPC->bSaveSpikeTrace : FALSE ); }
   BOOL RTFCNDCL IsChairPresent() { return( (m_pvIPC != NULL) ? m_pvIPC->bChairPresent : FALSE ); }
   int RTFCNDCL GetDayOfMonth() { return( (m_pvIPC != NULL) ? m_pvIPC->iDay : 0 ); }
   int RTFCNDCL GetMonthOfYear() { return( (m_pvIPC != NULL) ? m_pvIPC->iMonth : 0 ); }
   int RTFCNDCL GetYear() { return( (m_pvIPC != NULL) ? m_pvIPC->iYear : 0 ); }
   int RTFCNDCL GetVStabSlidingWindow() { return((m_pvIPC!=NULL) ? m_pvIPC->iVStabSlidingWindow : 1); }
   
   float RTFCNDCL GetDOBusyWait(int i) 
   {
      return((m_pvIPC != NULL && i >= 0 && i < 3) ? m_pvIPC->fDOBusyWaits[i] : 0.0f);
   }

   DWORD RTFCNDCL GetHardwareStatus() { return( (m_pvIPC != NULL) ? m_pvIPC->dwHWStatus : 0 ); }
   VOID RTFCNDCL SetHardwareStatus( const DWORD flags ) { if( m_pvIPC != NULL ) m_pvIPC->dwHWStatus = flags; }
   BOOL RTFCNDCL IsAIAvailable() { return( BOOL((GetHardwareStatus() & CX_F_AIAVAIL) != 0) ); }
   BOOL RTFCNDCL IsTMRAvailable() { return( BOOL((GetHardwareStatus() & CX_F_TMRAVAIL) != 0) ); }
   BOOL RTFCNDCL IsAOAvailable() { return( BOOL((GetHardwareStatus() & CX_F_AOAVAIL) != 0) ); }
   BOOL RTFCNDCL IsXYAvailable() { return( BOOL((GetHardwareStatus() & CX_F_XYAVAIL) != 0) ); }
   BOOL RTFCNDCL IsRMVAvailable() { return( BOOL((GetHardwareStatus() & CX_F_RMVAVAIL) != 0) ); }

   int RTFCNDCL GetAIChannels() { return( (m_pvIPC != NULL) ? m_pvIPC->nAIChannels : 0 ); }
   BOOL RTFCNDCL SetAIChannels( const int nAI )
   {
      if( (m_pvIPC == NULL) || (nAI < 0) || (nAI > CX_AIO_MAXN) ) return( FALSE );
      m_pvIPC->nAIChannels = nAI;
      return( TRUE );
   }
   int RTFCNDCL GetAOChannels() { return( (m_pvIPC != NULL) ? m_pvIPC->nAOChannels : 0 ); }
   BOOL RTFCNDCL SetAOChannels( const int nAO )
   {
      if( (m_pvIPC == NULL) || (nAO < 0) || (nAO > CX_AIO_MAXN) ) return( FALSE );
      m_pvIPC->nAOChannels = nAO;
      return( TRUE );
   }
   int RTFCNDCL GetTDIChannels() { return( (m_pvIPC != NULL) ? m_pvIPC->nTDIChannels : 0 ); }
   BOOL RTFCNDCL SetTDIChannels( const int nTDI )
   {
      if( (m_pvIPC == NULL) || (nTDI < 0) || (nTDI > CX_TMR_MAXN) ) return( FALSE );
      m_pvIPC->nTDIChannels = nTDI;
      return( TRUE );
   }
   int RTFCNDCL GetTDOChannels() { return( (m_pvIPC != NULL) ? m_pvIPC->nTDOChannels : 0 ); }
   BOOL RTFCNDCL SetTDOChannels( const int nTDO )
   {
      if( (m_pvIPC == NULL) || (nTDO < 0) || (nTDO > CX_TMR_MAXN) ) return( FALSE );
      m_pvIPC->nTDOChannels = nTDO;
      return( TRUE );
   }

   VOID RTFCNDCL ResetHardwareInfo();                    // clear all HW info (indic. no HW present)

   BOOL RTFCNDCL Message( LPCTSTR pszMsg );              // post message to MAESTRO

   BOOL RTFCNDCL InitTrace();                            // init data trace facility
   BOOL RTFCNDCL UpdateTrace( short* pshAI,              // update data trace facility with new sampled data
      short* pshComp, DWORD dwEvtMask );                 //

   BOOL RTFCNDCL InitEventStream();                      // init digital event stream buffers
   BOOL RTFCNDCL UpdateEventStream(DWORD dwEvent,        // update digital event buffers with new event
      int time);

   BOOL RTFCNDCL UpdatePosPlot( LPPOINT pPtLoci );       // update eye-target position plot facility

   int RTFCNDCL GetNumTargets()                          // # of targets currently specified in target list in IPC
   {
      int n = (m_pvIPC==NULL) ? 0 : m_pvIPC->nTgts;
      if( n > CX_MAXTGTS ) n = CX_MAXTGTS;
      return( n );
   }
   BOOL RTFCNDCL GetTarget( const int i, CXTARGET& tgt ) // inline access to individual tgt defns in the tgt list
   {
      if( i < 0 || i >= GetNumTargets() ) return( FALSE );
      else { tgt = m_pvIPC->targets[i]; return( TRUE ); }
   }
   WORD RTFCNDCL GetTargetType( const int i )
   {
      return( (i < 0 || i >= GetNumTargets()) ? 0 : m_pvIPC->targets[i].wType );
   }
   BOOL RTFCNDCL IsRMVTarget( const int i )              // is specified tgt implemented on RMVideo display?
   {
      if( i < 0 || i >= GetNumTargets() ) return( FALSE );
      else return( BOOL( m_pvIPC->targets[i].wType == CX_RMVTARG ) );
   }
   BOOL RTFCNDCL IsUsingRMVTargets();                    // are there any RMVideo tgts in the current tgt list?

   int RTFCNDCL GetNumTrialTargets();                    // # of targets participating in trial defined in IPC
   int RTFCNDCL MapTrialTargetIndex( int i );            // maps pos in trial tgt list to pos in current def'd tgt list
   BOOL RTFCNDCL GetTrialTarget( int i, CXTARGET& tgt ); // retrieve complete defn of specified trial target
   WORD RTFCNDCL GetTrialTargetType( int i );            // retrieve type of specified trial target
   int RTFCNDCL GetTrialTargetSubtype( int i );          // retrieve subtype of specified XYScope or RMVideo trial tgt
   int RTFCNDCL GetNumTrialCodes();                      // # of codes defining the trial in IPC
   const TRIALCODE& GetTrialCode( const int i );         // retrieve a particular trial code from IPC
   int RTFCNDCL GetNumTaggedSections();                  // #of tagged sections defined on the trial in IPC
   BOOL RTFCNDCL GetTaggedSection( int i,                // retrieve definition of a particular tagged section
      TRIALSECT& sect );
   int RTFCNDCL GetTrialAltXYDotSeed();                  // retrieve alternate XY dot seed value defined in IPC
   BOOL RTFCNDCL IsSavingTrialFile()                     // are we to save a trial data file for the current trial?
   {
      return( (GetMode() == CX_TRIALMODE) ? BOOL( m_pvIPC->strDataPath[0] != '\0' ) : FALSE );
   }

   BOOL RTFCNDCL ClearResult()                           // clear/set protocol result -- only in Trial, Cont modes!
   {
      int iMode = GetMode();
      if( iMode == CX_TRIALMODE || iMode == CX_CONTMODE ) { m_pvIPC->dwResult = 0; return( TRUE ); }
      else return( FALSE );
   }
   BOOL RTFCNDCL SetResult( const DWORD dwRes )
   {
      int iMode = GetMode();
      if( iMode == CX_TRIALMODE || iMode == CX_CONTMODE ) { m_pvIPC->dwResult = dwRes; return( TRUE ); }
      else return( FALSE );
   }

   VOID RTFCNDCL ResetNumRewards()                       // reset/increment #rewards delivered statistic in IPC
   {
      if( m_pvIPC ) m_pvIPC->iNumRewards = 0;
   }
   VOID RTFCNDCL IncrementNumRewards()
   {
      if( m_pvIPC ) ++(m_pvIPC->iNumRewards);
   }

   VOID RTFCNDCL ResetCumulativeReward()                 // reset/add to cumulative reward statistic (sum of reward
   {                                                     // pulses delivered, in ms) in IPC
      if( m_pvIPC ) m_pvIPC->iTotalRewardMS = 0;
   }
   VOID RTFCNDCL AccumulateRewardPulse( int lenMS )
   {
      if( m_pvIPC ) m_pvIPC->iTotalRewardMS += lenMS;
   }

   VOID RTFCNDCL SetLastTrialLen( int n )                // store elapsed time of last trial presented in IPC field
   {                                                     // (only in Trial mode)
      if( GetMode() == CX_TRIALMODE )
         m_pvIPC->iLastTrialLen = n;
   }

   VOID RTFCNDCL SetRPDistroBehavResp( float fResp )     // store measured behavioral response to an "R/P Distro"
   {                                                     // trial in dedicated IPC field
      if( GetMode() == CX_TRIALMODE )
         m_pvIPC->fResponse = fResp;
   }

   VOID RTFCNDCL GetStimRunDef( CONTRUN& def )           // retrieve stimulus run definition in IPC -- Cont mode only!
   {
      if( GetMode() == CX_CONTMODE ) def = m_pvIPC->runDef;
      else ::memset( (PVOID) &def, 0, sizeof( CONTRUN ) );
   }

   VOID RTFCNDCL GetProtocolName( char *pBuf, int n );   // retrieve name of trial or stimulus run defined in IPC
   VOID RTFCNDCL GetTrialSetName(char* pBuf, int n);     // retrieve name of trial set defined in IPC
   VOID RTFCNDCL GetTrialSubsetName(char* pBuf, int n);  // retrieve name of trial subset (if any) defined in IPC
   VOID RTFCNDCL GetDataFilePath( char *pBuf, int n );   // full pathname for the data file for trial or stimulus run
   VOID RTFCNDCL GetDataFileName( char *pBuf, int n );   // get only filename and ext of the data file

   float RTFCNDCL GetPosScale() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fPosScale : 1.0f ); }
   float RTFCNDCL GetPosRotate() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fPosRotate : 0.0f ); }
   float RTFCNDCL GetVelScale() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fVelScale : 1.0f ); }
   float RTFCNDCL GetVelRotate() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fVelRotate : 0.0f ); }
   float RTFCNDCL GetStartPosH() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fStartPosH : 0.0f ); }
   float RTFCNDCL GetStartPosV() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->fStartPosV : 0.0f ); }

   DWORD RTFCNDCL GetTrialFlags() { return( (GetMode()==CX_TRIALMODE) ? m_pvIPC->dwTrialFlags : DWORD(0) ); }

   DWORD RTFCNDCL GetCommand();                          // poll for next command from MAESTRO
   VOID RTFCNDCL GetCommandData(int* piData,             // retrieve data associated with current pending command
         float *pfData, const int ni, const int nf,      //
         char *pcData = NULL, const int nc = 0);         //
   VOID RTFCNDCL AckCommand( DWORD dwRsp, int *piData,   // respond to current pending command
         float *pfData, const int ni, const int nf,      //
         const BOOL bWait = FALSE,                       //
         char *pcData = NULL, const int nc = 0);         //

   // is the Eyelink 1000+ tracker connected to Maestro and actively recording eye position data?
   BOOL RTFCNDCL IsEyelinkInUse() { return(m_pvIPC != NULL && m_pvIPC->iELStatus == CX_ELSTAT_REC); }
   // is an eye position sample ready from the Eyelink 1000+ tracker?
   BOOL RTFCNDCL IsEyelinkSampleAvailable() 
   { 
      return(m_pvIPC != NULL && m_pvIPC->iELStatus == CX_ELSTAT_REC && m_pvIPC->iELLast != m_pvIPC->iELNext); 
   }
   // get Eyelink tracker recording type: not in use, monocular L/R, or binocular
   int RTFCNDCL GetEyelinkRecordType() { return(IsEyelinkInUse() ? m_pvIPC->iELRecType : EL_NOTINUSE); }
   // get next Eyelink tracker sample when tracker is actively recording eye position data
   int RTFCNDCL GetNextEyelinkSample(ELSAMP& s, BOOL bFlush);
   // get window width (ms) for Eyelink velocity smoothing filter (will be 0 if Eyelink not in use)
   int RTFCNDCL GetEyelinkVelocityWindowWidth() { return(IsEyelinkInUse() ? m_pvIPC->iELParams[4] : 0); }
   // get Eyelink X or Y offset calibration factor (each will be 0 if Eyelink not in use)
   int RTFCNDCL GetEyelinkOffset(BOOL isX) { return(IsEyelinkInUse() ? m_pvIPC->iELParams[isX ? 0 : 1] : 0); }
   // get Eyelink X or Y offset calibration factor (each will be 0 if Eyelink not in use)
   int RTFCNDCL GetEyelinkGain(BOOL isX) { return(IsEyelinkInUse() ? m_pvIPC->iELParams[isX ? 2 : 3] : 0); }
};


#endif   // !defined(CXMASTERIO_H__INCLUDED_)



