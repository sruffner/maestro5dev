//===================================================================================================================== 
//
// cxvideodspdlg.h : Declaration of CCxVideoDspDlg, a Maestro control panel dialog page for modifying the current  
//                   RMVideo display configuration.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXVIDEODSPDLG_H__)
#define CXVIDEODSPDLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "numedit.h"             // CNumEdit -- a restricted-format numeric-only edit control

#include "cxobj_ifc.h"           // Maestro object interface:  common constants and other defines 
#include "cxviewhint.h"          // CCxViewHint -- for communicating doc/view changes...
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for Maestro mode contol panel dialogs

class CCxSettings;               // forward declaration


//===================================================================================================================== 
// Declaration of class CCxVideoDspDlg 
//===================================================================================================================== 
//
class CCxVideoDspDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxVideoDspDlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_DISPLAY;       // dialog template resource ID for this dialog


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   BOOL        m_bIsFBEnabled;               // TRUE if ctrls for RMVideo framebuffer display are enabled

   CNumEdit    m_edFBDistToEye;
   CNumEdit    m_edFBWidth;
   CNumEdit    m_edFBHeight;
   CNumEdit    m_edFBBkgRed;
   CNumEdit    m_edFBBkgGrn;
   CNumEdit    m_edFBBkgBlu;
   CNumEdit    m_edRMVGammaRed;
   CNumEdit    m_edRMVGammaBlu;
   CNumEdit    m_edRMVGammaGrn;
   CNumEdit    m_edRMVSyncDur;
   CNumEdit    m_edRMVSyncSize;

   CComboBox   m_cbRMVMode;                  // combo box selects RMVideo display mode

   CButton     m_btnIsGray;                  // RMVideo grayscale checkbox


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxVideoDspDlg(const CCxVideoDspDlg& src);            // no copy constructor or assignment operator defined
   CCxVideoDspDlg& operator=( const CCxVideoDspDlg& src ); 

public: 
   CCxVideoDspDlg() : CCxControlPanelDlg(IDD) { m_bIsFBEnabled = FALSE; }
   ~CCxVideoDspDlg() {}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnRMVModeChange();                       // RMV display mode switch
   afx_msg void OnChange( UINT id );                     // handle input to edit ctrls & a few others
   afx_msg void OnGrayscale();                           // toggle grayscale mode for the RMVideo bkg color R/G/B ctrls

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   VOID Refresh();                                       // to refresh dlg appearance -- updates enable state of ctrls
   VOID OnUpdate( CCxViewHint* pHint );                  // refresh appearance IAW specified Maestro doc/view change


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   CCxSettings* GetSettings();                           // retrieve ptr to the Maestro application settings object 
   VOID Load();                                          // reload all ctrls IAW current video display settings
   VOID ReloadFieldOfView();
   VOID Notify();                                        // notify doc/view whenever video display cfg is changed here 
};


#endif // !defined(CXVIDEODSPDLG_H__)

