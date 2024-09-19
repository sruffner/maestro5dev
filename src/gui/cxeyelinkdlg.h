//===================================================================================================================== 
//
// cxeyelinkdlg.h : Declaration of CCxEyelinkDlg, a Maestro control panel dialog page encompassing controls related to
// the Eyelink 1000+ tracker.
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXEYELINKDLG_H__)
#define CXEYELINKDLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "numedit.h"             // CNumEdit -- a restricted-format numeric-only edit control

#include "cxobj_ifc.h"           // CNTRLX object interface:  common constants and other defines 
#include "cxviewhint.h"          // CCxViewHint
#include "cxcontrolpaneldlg.h"   // CCxControlPanelDlg -- base class for CNTRLX mode contol panel dialogs


class CCxEyelinkDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxEyelinkDlg )

private:
   // dialog template resource ID for this dialog
   static const int IDD = IDD_EYELINK;

   // the Connect/Disconnect pushbutton
   CButton m_btnConnect; 

   // read-only numeric edits display calibration parameters
   CNumEdit m_edXOfs; 
   CNumEdit m_edXGain; 
   CNumEdit m_edYOfs; 
   CNumEdit m_edYGain; 

   // slider controls for the calibration parameters
   CSliderCtrl m_slideXOfs;
   CSliderCtrl m_slideXGain;
   CSliderCtrl m_slideYOfs;
   CSliderCtrl m_slideYGain;

   // numeric edit control sets velocity smoothing filter's window width in ms
   CNumEdit m_edVelFW;

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxEyelinkDlg(const CCxEyelinkDlg& src);                // no copy constructor or assignment operator defined
   CCxEyelinkDlg& operator=( const CCxEyelinkDlg& src ); 

public: 
   CCxEyelinkDlg() : CCxControlPanelDlg( IDD ) {}
   ~CCxEyelinkDlg() {}

protected:
   // message map handlers
   afx_msg void OnBtnClick(UINT id);
   afx_msg void OnSetFilterW();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pSB);

   DECLARE_MESSAGE_MAP()

   // catch "Enter" key in the velocity filter width edit control via default dialog box mechanism
   void OnOK() 
   { 
      CWnd* pCtrl = GetFocus();
      if(pCtrl != NULL && pCtrl->GetDlgCtrlID() == IDC_EL_VELFW) OnSetFilterW(); 
   }

//===================================================================================================================== 
// OPERATIONS/ATTRIBUTES
//=====================================================================================================================
public:
   // prepare dialog for display
   BOOL OnInitDialog(); 
   // call this to refresh appearance whenever there's a change in Eyelink connection status
   VOID Refresh(); 
   // no-op; Maestro document and other views have no effect on the information in this dialog
   VOID OnUpdate(CCxViewHint* pHint) {}
};


#endif // !defined(CXEYELINKDLG_H__)

