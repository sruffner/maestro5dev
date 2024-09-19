//===================================================================================================================== 
//
// cxcontrolpaneldlg.h : Declaration of ABSTRACT class CCxControlPanelDlg.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXCONTROLPANELDLG_H__)
#define CXCONTROLPANELDLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizebar\szdlgbar.h"          // base class CSzDlgBarDlg

class CCxViewHint;                     // forward declarations
class CCxModeControl;


class CCxControlPanelDlg : public CSzDlgBarDlg
{
   DECLARE_DYNAMIC( CCxControlPanelDlg )

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   BOOL     m_bInitiatedUpdate;        // TRUE whenever doc/view update was initiated by this dialog 

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxControlPanelDlg( const CCxControlPanelDlg& src );  // no copy constructor or assignment operator defined
   CCxControlPanelDlg& operator=( const CCxControlPanelDlg& src ); 

public: 
   CCxControlPanelDlg( UINT idd ) : CSzDlgBarDlg( idd ) { m_bInitiatedUpdate = FALSE; }
   ~CCxControlPanelDlg() {}


//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 
public:
   virtual VOID Refresh() = 0;                           // refresh time-sensitive dialog contents
   virtual VOID OnUpdate( CCxViewHint* pHint ) {}        // refresh appearance IAW specified CNTRLX doc/view change

protected:
   CCxModeControl* GetCurrentModeCtrl();                 // ptr to the mode controller object for current op mode
   CCxModeControl* GetModeCtrl( int iOpMode );           // ptr to the mode controller for specified op mode
   VOID SendUpdate( CCxViewHint* pHint,                  // notify doc/view of a change to document data
                    BOOL bMod = TRUE );
   BOOL InitiatedUpdate() {return(m_bInitiatedUpdate);}  // TRUE when doc/view update initiated here
};


#endif // !defined(CXCONTROLPANELDLG_H__)
