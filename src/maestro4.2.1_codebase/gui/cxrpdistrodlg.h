//=====================================================================================================================
//
// cxrpdistrodlg.h : Declaration of CCxRPDistroDlg, the Maestro Trial-mode control panel dialog page that administers
//                   a distribution-based reward/penalty contingency protocol.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXRPDISTRODLG_H__)
#define CXRPDISTRODLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "numedit.h"             // CNumEdit -- a restricted-format numeric-only edit control
#include "cxrpdistroview.h"      // CCxRPDistroView -- a custom static ctrl that displays a CCxRPDistro object
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs

class CCxViewHint;               // (forward decl) CCxViewHint -- for communicating doc/view changes...
class CCxRPDistro;               // (forward decl) CCxRPDistro -- stores runtime info for dist-based reward protocol


//=====================================================================================================================
// Declaration of class CCxRPDistroDlg
//=====================================================================================================================
//
class CCxRPDistroDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxRPDistroDlg )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TRIALCP4;      // dialog template resource ID for this dialog

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   BOOL m_bEnabled;                          // TRUE if controls on dialog are currently enabled
   CWordArray m_wArTrialKeys;                // object keys of trials that have been run using "R/P Distro" operation
   int m_nPassed;                            // total # of "passed" trials (response in reward window) since startup
   int m_nFailed;                            // total # of "failed" trials (resp outside reward window) since startup

   CComboBox m_cbCurrTrial;                  // combo box that selects trial for which R/P Distro info is displayed
   CButton m_btnNewDist;                     // if PB is activated, start a new response distrib for selected trial
   CButton m_btnSaveSummary;                 // if PB is activated, save summary of R/P Distro objs catalogued here
   CButton m_btnRewEna;                      // check box enables/disables reward window

   CNumEdit m_edRewMin;                      // numeric edit ctrls display/edit reward window parameters
   CNumEdit m_edRewMax;
   CNumEdit m_edRewShift;
   CNumEdit m_edRewNUpd;

   CComboBox m_cbRespType;                   // combo box selects the response measure type 
   
   CNumEdit m_edCurrMostRecent;              // # most recent valid samples to include in distrib stats
   CNumEdit m_edPrevMostRecent;

   CNumEdit m_edRngMin;                      // numeric edit ctrl display/edit "R/P Distro" object's valid resp range
   CNumEdit m_edRngMax;

   CStatic m_roCurrMean;                     // read-only text controls display distribution and reward/penalty stats
   CStatic m_roPrevMean;
   CStatic m_roCurrPassFail;
   CStatic m_roSummaryPassFail;

   CCxRPDistroView m_distroView;             // custom control that displays distributions and reward window for
   CSize m_minViewSize;                      // the currently selected trial on this tab

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxRPDistroDlg(const CCxRPDistroDlg& src);            // no copy constructor or assignment operator defined
   CCxRPDistroDlg& operator=( const CCxRPDistroDlg& src );

public:
   CCxRPDistroDlg() : CCxControlPanelDlg( IDD )
   {
      m_bEnabled = FALSE;
      m_nPassed = 0;
      m_nFailed = 0;
   }
   ~CCxRPDistroDlg() { m_wArTrialKeys.RemoveAll(); }


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnChange( UINT id );                     // handle input from any of the controls on tab
   afx_msg void OnSize( UINT nType, int cx, int cy );    // handle resizing of dialog


   DECLARE_MESSAGE_MAP()


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   VOID Refresh();                                       // to refresh dlg appearance -- updates enable state of ctrls
   VOID OnUpdate( CCxViewHint* pHint );                  // refresh appearance IAW specified CNTRLX doc/view change

   VOID OnTrialSetChanged( WORD wSet );                  // examines new trial set for any "R/P Distro" trials
   VOID OnTrialDone( WORD wKey );                        // update dlg when a "R/P Distro" trial has just finished

private:
   CCxRPDistro* GetCurrentRPDistro();                    // retrieve distrib data for trial selected in dialog
   VOID ReloadRewardWindowControls();                    // reloads controls that display/edit reward window params
   VOID ReloadRespRangeControls();                       // reloads controls that display/edit valid response range
   VOID UpdateStatReadouts();                            // reloads static text ctrls that display statistics

   VOID SaveSummaryToFile();                             // saves summary of all R/P Distro trials to text file
};


#endif // !defined(CXRPDISTRODLG_H__)

