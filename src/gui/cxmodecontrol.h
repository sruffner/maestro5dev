//===================================================================================================================== 
//
// cxmodecontrol.h : Declaration of ABSTRACT class CCxModeControl and "placeholder" mode controller CCxNullMode.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXMODECONTROL_H__)
#define CXMODECONTROL_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCxRuntime;                   // forward declarations
class CCxControlPanel;


//===================================================================================================================== 
// Declaration of ABSTRACT class CCxModeControl
//===================================================================================================================== 
//
class CCxModeControl 
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   int               m_iOpMode;                          // MAESTRO operational mode handled by this mode controller 

protected:
   CCxRuntime*       m_pRuntime;                         // ptr to the MAESTRODRIVER runtime interface object
   CCxControlPanel*  m_pCtrlPanel;                       // ptr to the MAESTRO mode control panel, the GUI container 
                                                         // that houses the dialogs used by a mode control object.

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxModeControl( const CCxModeControl& src );          // no copy constructor or assignment operator defined
   CCxModeControl& operator=( const CCxModeControl& src ); 

public: 
   CCxModeControl( int iMode, CCxControlPanel* pPanel ); 
   ~CCxModeControl(); 


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual BOOL InitDlgs() = 0;                          // add dlgs for this op mode to the control panel container
   virtual VOID Service() = 0;                           // service pending transaction w/ MAESTRODRIVER  
   virtual BOOL Enter() = 0;                             // do any inits upon entering this mode
   virtual BOOL Exit() = 0;                              // clean up prior to exiting this mode
   virtual BOOL CanUpdateVideoCfg() = 0;                 // TRUE when update of the video display cfg is permissible 
   virtual BOOL CanUpdateFixRewSettings() = 0;           // TRUE when update of fixation/reward settings is permissible
   virtual LPCTSTR GetModeTitle() = 0;                   // string constant describing this op mode

   int GetOpMode() { return( m_iOpMode ); }              // retrieve the op mode represented by this controller

   VOID Refresh();                                       // refresh appearance of dialogs

   BOOL UpdateVideoCfg();                                // sends video dsp cfg to MAESTRODRIVER to update video h/w
   BOOL UpdateFixRewSettings();                          // sends current fixation/reward settings to MAESTRODRIVER

   int GetNumRewardsDelivered();                         // gets/resets MAESTRODRIVER counters that track reward stats
   int GetCumulativeReward();
   BOOL ResetRewardStats();

   int GetMode();                                        // methods accessing MAESTRO h/w state & capabilities:
   int GetNumAO();
   int GetNumAI();
   int GetNumTDO();
   int GetNumTDI();
   BOOL IsAOChan( int iCh );
   BOOL IsAIChan( int iCh );
   BOOL IsTDOChan( int iCh );
   BOOL IsTDIChan( int iCh );
   DWORD GetHWStatus();
   BOOL IsAIAvailable();
   BOOL IsAOAvailable();
   BOOL IsTimerAvailable();
   BOOL IsRMVideoAvailable();
   int GetRMVideoScreenW(); 
   int GetRMVideoScreenH();
   float GetRMVideoFrameRate();
   BOOL CanCalibAI();

   // (sar) these wrappers were added in Sep2009 to expose new RMVideo-related functionality. Certain methods may block
   // for an extended time and so are suitable only in IdleMode and demand that a wait cursor be displayed so the user
   // is aware of the delay.
   BOOL CanUpdateRMV();
   int GetNumRMVideoModes();
   BOOL GetRMVideoModeDesc(int i, CString& desc);
   int GetCurrRMVideoMode();
   BOOL SetCurrRMVideoMode(int i);     // BLOCKS FOR UP TO 10 SECONDS
   
   BOOL GetRMVGamma(float& r, float& g, float& b);
   BOOL SetRMVGamma(float& r, float& g, float& b);

   int GetNumRMVMediaFolders();
   BOOL GetRMVMediaFolder(int i, CString& folder);
   int GetNumRMVMediaFiles(int i);
   BOOL GetRMVMediaInfo(int i, int j, CString& name, CString& desc);
   
   BOOL DeleteRMVMediaFile(int i, int j);     // BLOCKS FOR UP TO 5 SECONDS
   BOOL DownloadRMVMediaFile(LPCTSTR path, int iFolder, LPCTSTR folderNew, LPCTSTR file);  // BLOCKS FOR UP TO 120 SECONDS
   
   
   WORD GetTraces();                                     // key of chan cfg currently assoc w/ data trace facility 
   WORD SetTraces( const WORD wKey, const int iDur );    // reinit trace facility using specified chan cfg
   VOID OnChangeTraces();                                // to signal a change in chan cfg assoc w/ data trace facility 
};




//===================================================================================================================== 
// Declaration of class CCxNullMode -- the mode controller "placeholder" when MAESTRODRIVER is not running
//===================================================================================================================== 
//
class CCxNullMode : public CCxModeControl
{
//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxNullMode( const CCxNullMode& src );                // no copy constructor or assignment operator defined
   CCxNullMode& operator=( const CCxNullMode& src ); 

public: 
   CCxNullMode( CCxControlPanel* pPanel );
   ~CCxNullMode() {}


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual BOOL InitDlgs() { return( TRUE ); }           // add dlgs for this op mode to the control panel container
   virtual VOID Service() {}                             // service pending transactions w/ MAESTRODRIVER 
   virtual BOOL Enter() { return( TRUE ); }              // do any inits upon entering this mode
   virtual BOOL Exit() { return( TRUE ); }               // clean up prior to exiting this mode
   virtual BOOL CanUpdateVideoCfg() { return( FALSE ); } // TRUE when update of the video display cfg is permissible 
   virtual BOOL CanUpdateFixRewSettings()                // TRUE when update of fixation/reward settings is permissible
   { return( FALSE ); }
   virtual LPCTSTR GetModeTitle()                        // string constant describing this op mode
   { return( _T("None") ); }
};


#endif // !defined(CXMODECONTROL_H__)
