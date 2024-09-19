//===================================================================================================================== 
//
// cxcontrunform.h : Declaration of class CCxContRunForm.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXCONTRUNFORM_H__INCLUDED_)
#define CXCONTRUNFORM_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "visualfx.h"                  // the Visual Framework library, including base class TVTabPane
#include "numedit.h"                   // the CNumEdit class -- a numeric-only, configurable version of CEdit
#include "gridctrl\litegrid.h"         // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in

#include "cxobj_ifc.h"                 // the CNTRLX object interface:  common constants and type defs
#include "cxdoc.h"                     // the CCxDoc class encapsulates the associated document 
#include "cxcontrun.h"                 // the CCxContRun class encapsulates a CNTRLX continuous-mode run object



//===================================================================================================================== 
// Declaration of class CCxContRunForm
//===================================================================================================================== 
//
class CCxContRunForm : public TVTabPane
{
   DECLARE_DYNCREATE( CCxContRunForm )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int IDD = IDD_RUNFORM;          // resource ID for associated dialog template -- see constructor


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   BOOL              m_bOneTimeInitsDone;       // TRUE once one-time (vs per-doc) inits done; see OnInitialUpdate() 
   BOOL              m_bLoading;                // this transient flag is set whenever we're loading a run onto form
                                                // so that grid control display callbacks don't try to access stale ptr 

   WORD              m_wKey;                    // unique key of CNTRLX run object currently displayed on form 
   CCxContRun*       m_pRun;                    // ptr to CNTRLX run object currently displayed on form 

   CWordArray        m_arDepObjs;               // keys of CNTRLX objects upon which current run is dependent

   CCxStimulus*      m_pPasteStim;              // ptr to the stimulus channel object last copied by user; can be 
                                                // pasted into any run loaded into view

   CCellID           m_contextCell;             // the grid cell on which a context menu was invoked by right-click
   int               m_iInsTgtPos;              // insert pos for target being chosen by user; -1 when not adding tgt 
   WORD              m_wLastTgtKey;             // key of last target object added to the XYseq targets grid

   CLiteGrid         m_stimChanGrid;            // grid control displaying run's stimulus channel grid
   CSize             m_minSTCSize;              // minimum size (based on dlg template) of this grid

   CLiteGrid         m_xyTgtsGrid;              // grid control displaying list of XY targets for run's XYseq stimulus 
   
                                                // other subclassed controls on form:
   CNumEdit          m_edDutyPeriod;            // run duty period in ms 
   CNumEdit          m_edNAutoStop;             // # of duty cycles to auto-stop the run
   CNumEdit          m_edHOffset;               // horizontal position offset in deg
   CNumEdit          m_edVOffset;               // vertical position offset in deg
   CComboBox         m_cbDutyPulse;             // duty marker pulse, a multi-choice parameter 


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxContRunForm( const CCxContRunForm& src );          // no copy constructor defined
   CCxContRunForm& operator=(const CCxContRunForm& src); // no assignment operator defined

public:
   CCxContRunForm();                                     // (NOTE: required by dynamic object creation mechanism) 
   ~CCxContRunForm(); 


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnDutyPulse()                            // handle CBN_SELCHANGE from combo box for duty pulse channel 
   { OnChange( IDC_RF_DUTYPULSE ); }
   afx_msg void OnChange( UINT id );                     // update corres param when an edit or PB ctrl changes 
   afx_msg void OnUpdateEditCommand( CCmdUI* pCmdUI );   // update enable state of standard "Edit" menu cmds 
   afx_msg void OnEditCommand( UINT nID );               // perform selected standard "Edit" menu cmds 
   afx_msg void OnSize( UINT nType, int cx, int cy );    // resize stim channel grid if form view is enlarged by user
   afx_msg void OnNMDblClk( UINT id, NMHDR* pNMHDR,      // handle NM_DBLCLK notification from grid controls on form
                            LRESULT* pResult );
   afx_msg void OnNMRClick( UINT id, NMHDR* pNMHDR,      // handle NM_RCLICK notification from grid controls on form
                            LRESULT* pResult );
   afx_msg void OnGridOps( UINT cmdID );                 // handle user-initiated operations on embedded grid ctrls
   afx_msg void OnUpdGridOps( CCmdUI* pCmdUI );          // dynamically disable/enable grid operations dep on context 

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   void OnInitialUpdate();                               // defines start-up appearance of this form 
   void OnUpdate( CView* pSender, LPARAM lHint,          // update appearance of the form
                  CObject* pHint );


//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // dump info on current state of this view
   void AssertValid() const;                             // validate internal state of this view 
#endif


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
private:
   CCxDoc* GetDocument() const                           // return attached document pointer cast to derived doc class 
   { 
      ASSERT( m_pDocument );
      ASSERT_KINDOF( CCxDoc, m_pDocument );
      return( (CCxDoc*) m_pDocument ); 
   }

protected:
   VOID LoadRun( const WORD key );                       // load specified CNTRLX run's definition onto form view 
   VOID UpdateCaption( LPCTSTR szCaption );              // update caption of associated tab pane in parent tab wnd
   VOID StuffHdrControls();                              // stuff "header" params assigned to edit & PB ctrls on form 
   VOID InformModify();                                  // inform doc & other views that run was modified 

   BOOL IsStimGridCellReadOnly(const CCellID& c) const;  // is specified cell in stim channel table read-only?

   static BOOL CALLBACK StimGridDispCB(                  // callback invoked by stim channels grid to get cell text and 
            GV_DISPINFO *pDispInfo, LPARAM lParam );     // display information, label tip text
   static BOOL CALLBACK StimGridEditCB(                  // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK StimGridEndEditCB(               // callback invoked upon termination of inplace editing 
            ENDEDITINFO *pEEI, LPARAM lParam );

   static BOOL CALLBACK TgtGridDispCB(                   // analogously for the XYseq targets grid... 
            GV_DISPINFO *pDispInfo, LPARAM lParam ); 
   static BOOL CALLBACK TgtGridEditCB( 
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK TgtGridEndEditCB( 
            ENDEDITINFO *pEEI, LPARAM lParam );
};



#endif   // !defined(CXCONTRUNFORM_H__INCLUDED_)
