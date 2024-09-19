//===================================================================================================================== 
//
// cxfixrewdlg.h : Declaration of CCxFixRewDlg, a CNTRLX control panel dialog page for changing fixation requirements 
//                 and related settings.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXFIXREWDLG_H__)
#define CXFIXREWDLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "numedit.h"             // CNumEdit -- a restricted-format numeric-only edit control

#include "cxobj_ifc.h"           // CNTRLX object interface:  common constants and other defines 
#include "cxviewhint.h"          // CCxViewHint -- for communicating doc/view changes...
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs

class CCxSettings;               // forward declaration


//===================================================================================================================== 
// Declaration of class CCxFixRewDlg 
//===================================================================================================================== 
//
class CCxFixRewDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxFixRewDlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_FIXREW;        // dialog template resource ID for this dialog


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   BOOL        m_bEnabled;                   // TRUE if controls on dialog are currently enabled

   CNumEdit    m_edFixDur;                   // custom edit ctrls for numeric parameters 
   CNumEdit    m_edFixAccH;
   CNumEdit    m_edFixAccV;
   CNumEdit    m_edRewLen1;
   CNumEdit    m_edRewLen2;
   CNumEdit    m_edRewMult;
   CNumEdit    m_edVarRatio;
   CNumEdit    m_edAudioRewLen;
   CNumEdit    m_edNRewards;
   CNumEdit    m_edTotalRew;

   CButton     m_btnTrialRewOverride;        // button ctrls on dialog
   CButton     m_btnRewBeepEna;
   CButton     m_btnResetRew;


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxFixRewDlg(const CCxFixRewDlg& src);                // no copy constructor or assignment operator defined
   CCxFixRewDlg& operator=( const CCxFixRewDlg& src ); 

public: 
   CCxFixRewDlg() : CCxControlPanelDlg( IDD )  { m_bEnabled = FALSE; }
   ~CCxFixRewDlg() {}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnChange( UINT id );                     // handle input to edit ctrls & a few others

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ATTRIBUTES 
//===================================================================================================================== 
public:
   int GetNumRewardsDelivered() const { return( m_edNRewards.AsInteger() ); }
   int GetCumulativeReward() const { return( m_edTotalRew.AsInteger() ); }
   VOID UpdateRewardStats( int n, int sumMS )
   {
      if( n != m_edNRewards.AsInteger() )                // want to avoid unnecessary repaints!
         m_edNRewards.SetWindowText( n );
      if( sumMS != m_edTotalRew.AsInteger() )
         m_edTotalRew.SetWindowText( sumMS );
   }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   VOID Refresh();                                       // to refresh dlg appearance -- updates enable state of ctrls
   VOID OnUpdate( CCxViewHint* pHint );                  // refresh appearance IAW specified CNTRLX doc/view change


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   CCxSettings* GetSettings();                           // retrieve ptr to the CNTRLX application settings object 
   VOID Load();                                          // reload all ctrls IAW current values in app settings object 
   VOID Notify();                                        // notify doc/view whenever a setting is changed here 
};


#endif // !defined(CXFIXREWDLG_H__)

