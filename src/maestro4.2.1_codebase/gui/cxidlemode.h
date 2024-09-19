//===================================================================================================================== 
//
// cxidlemode.h : Declaration of the IdleMode controller CCxIdleMode.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXIDLEMODE_H__)
#define CXIDLEMODE_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cxmodecontrol.h"       // CCxModeControl -- base class for CNTRLX mode controllers


class CCxControlPanel;           // forward declarations
class CCxFixRewDlg;
class CCxVideoDspDlg;
class CCxRMVStoreDlg;
class CCxEyelinkDlg;

//===================================================================================================================== 
// Declaration of class CCxIdleMode -- the mode controller for "Idle Mode" 
//===================================================================================================================== 
//
class CCxIdleMode : public CCxModeControl
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CCxFixRewDlg*     m_pFixRewDlg;              // ptr to the "Fixation/Reward" dialog
   CCxVideoDspDlg*   m_pVideoDspDlg;            // ptr to the "Video Display" dialog
   CCxRMVStoreDlg*   m_pRMVStoreDlg;            // ptr to the "RMVideo Media" dialog
   CCxEyelinkDlg*    m_pEyelinkDlg;             // ptr to the "Eyelink" dialog

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxIdleMode( const CCxIdleMode& src );                // no copy constructor or assignment operator defined
   CCxIdleMode& operator=( const CCxIdleMode& src ); 

public: 
   CCxIdleMode( CCxControlPanel* pPanel );
   ~CCxIdleMode() {}                                     // (created dlgs destroyed by mode control panel container)


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual BOOL InitDlgs();                              // add dlgs for this op mode to the control panel container

   virtual VOID Service() {}                             // update runtime state in this mode *** called frequently *** 
   virtual BOOL Enter();                                 // do any inits upon entering this mode 
   virtual BOOL Exit();                                  // clean up prior to exiting this mode

   virtual BOOL CanUpdateVideoCfg() { return( TRUE ); }  // TRUE when update of the video display cfg is permissible 
   virtual BOOL CanUpdateFixRewSettings()                // TRUE when update of fixation/reward settings is permissible
   { 
      return( TRUE ); 
   }
   virtual LPCTSTR GetModeTitle()                        // string constant describing this op mode
   {
      return( _T("Idle Mode") );
   }
};


#endif // !defined(CXIDLEMODE_H__)

