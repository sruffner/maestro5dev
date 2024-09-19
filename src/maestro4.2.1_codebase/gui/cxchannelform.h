//===================================================================================================================== 
//
// cxchannelform.h : Declaration of class CCxChannelForm.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXCHANNELFORM_H__INCLUDED_)
#define CXCHANNELFORM_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "visualfx.h"                  // base class TVTabPane and rest of the "Visual Framework" library
#include "numedit.h"                   // CNumEdit
#include "gridctrl\litegrid.h"         // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in
#include "cxdoc.h"                     // CCxDoc -- the CNTRLX "experiment" document assoc w/ this view
#include "cxchannel.h"                 // CCxChannel 



//===================================================================================================================== 
// Declaration of class CCxChannelForm
//===================================================================================================================== 
class CCxChannelForm : public TVTabPane
{
   DECLARE_DYNCREATE( CCxChannelForm )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int IDD = IDD_CHANNELFORM; // CNTRLX resource dialog template for the channels form layout


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   BOOL              m_bOneTimeInitsDone; // TRUE once one-time (vs per-document) inits are done; see OnInitialUpdate() 

   CLiteGrid         m_grid;              // spreadsheet-like control displays all channel config attributes
   CNumEdit          m_edYMin;            // numeric-only edit ctrls specifying y-axis range 
   CNumEdit          m_edYMax;

   WORD              m_wKey;              // unique key of CNTRLX channel config object currently displayed on form
   CCxChannel*       m_pChanCfg;          // ptr to the CNTRLX channel config object currently displayed on form

   int               m_nChDisplayed;      // # of data channels currently selected for display


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxChannelForm( const CCxChannelForm& src );          // no copy constructor defined
   CCxChannelForm&                                       // no assignment operator defined
      operator=( const CCxChannelForm& src );            //

public: 
   CCxChannelForm();                                     // no-arg constructor req'd by dynamic obj creation framework
   ~CCxChannelForm() {}                                  // destructor


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnButtonClicked( UINT nID );             // respond to one of the command buttons on the form
   afx_msg void OnEditRange( UINT id );                  // respond to change in y-axis range limits
   afx_msg void OnUpdateEditCommand( CCmdUI* pCmdUI );   // update enable state of standard "Edit" menu cmds 
   afx_msg void OnEditCommand( UINT nID );               // perform selected standard "Edit" menu cmds 

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   void OnInitialUpdate();                               // prepare form for initial display
   void OnUpdate( CView* pSender, LPARAM lHint,          // update appearance of the view
                  CObject* pHint );
   BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra,     // give the grid control a chance to handle command messages 
                  AFX_CMDHANDLERINFO* pHandlerInfo);


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump info on current state of channel config form view
   void AssertValid() const;                             // validate the form view 
#endif



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   CCxDoc* GetDocument()                                 // return attached document pointer cast to derived doc class 
   { 
      ASSERT( m_pDocument );
      ASSERT_KINDOF( CCxDoc, m_pDocument );
      return( (CCxDoc*) m_pDocument ); 
   }

protected:
   VOID LoadChanCfg( const WORD key );                   // load form with specified chan config object
   VOID UpdateCaption( LPCTSTR szCaption );              // update tab pane caption 

   BOOL IsGridCellReadOnly( CCellID c ) const;           // is specified cell in chan cfg grid read-only?

   static BOOL CALLBACK GridDispCB(                      // callback invoked by grid to get cell text & display info 
            GV_DISPINFO *pDispInfo, LPARAM lParam ); 
   static BOOL CALLBACK GridEditCB(                      // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // callback invoked when upon termination of inplace editing 
            ENDEDITINFO *pEEI, LPARAM lParam );

   VOID InformModify();                                  // inform doc & other views that loaded chan cfg was modified 

};

#endif   // !defined(CXCHANNELFORM_H__INCLUDED_)
