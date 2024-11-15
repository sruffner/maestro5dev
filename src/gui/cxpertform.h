//===================================================================================================================== 
//
// cxpertform.h : Declaration of class CCxPertForm.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXPERTFORM_H__INCLUDED_)
#define CXPERTFORM_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "visualfx.h"                  // base class TVTabPane and rest of the "Visual Framework" library
#include "gridctrl\litegrid.h"         // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in
#include "cxdoc.h"                     // CCxDoc -- the CNTRLX "experiment" document assoc w/ this view

class CCxPert;                         // forward declaration


//===================================================================================================================== 
// Declaration of class CCxPertForm
//===================================================================================================================== 
class CCxPertForm : public TVTabPane
{
   DECLARE_DYNCREATE( CCxPertForm )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int IDD = IDD_PERTFORM;   // CNTRLX resource dialog template for the perturbation form layout


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   BOOL              m_bOneTimeInitsDone; // TRUE once one-time (vs per-document) inits are done; see OnInitialUpdate() 

   CLiteGrid         m_grid;              // spreadsheet-like control that displays perturbation object definitions
   CSize             m_minGridSize;       // minimum size (based on dlg template) of the perturbation grid/table
   CWordArray        m_wArPertKeys;       // array of keys for pert objects currently displayed, in display order


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxPertForm( const CCxPertForm& src );                // no copy constructor defined
   CCxPertForm& operator=( const CCxPertForm& src );     // no assignment operator defined

public: 
   CCxPertForm();                                        // no-arg constructor req'd by dynamic obj creation framework
   ~CCxPertForm();                                       // destructor


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnSize( UINT nType, int cx, int cy );    // resize perturbation table if user resizes form 
   afx_msg void OnUpdateEditCommand( CCmdUI* pCmdUI );   // update enable state of standard "Edit" menu cmds 
   afx_msg void OnEditCommand( UINT nID );               // perform selected standard "Edit" menu cmds 
   
   // when focus cell changes on perturbations table, redraw the header row so column labels are updated
   afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
   {
      m_grid.RedrawRow(0);
   }

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
   void Dump( CDumpContext& dc ) const;                  // dump info on current state of perturbation form view
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

   VOID Load();                                          // load entire perturbation table
   int FindPerturbationRow( WORD wKey );                 // map perturbation object key to row in pert table
   CCxPert* GetPertObjByRow( int iRow );                 // retrieve perturbation obj represented by row of pert table
   BOOL IsGridCellReadOnly( CCellID c );                 // is specified cell in perturbation table read-only?
   VOID InformModify( int iPertRow );                    // inform doc/view framework that perturb obj was modified 

   static BOOL CALLBACK GridDispCB(                      // callback invoked by grid to get cell text & display info 
            GV_DISPINFO *pDispInfo, LPARAM lParam ); 
   static BOOL CALLBACK GridEditCB(                      // callback invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // callback invoked upon termination of inplace editing 
            ENDEDITINFO *pEEI, LPARAM lParam );

};

#endif   // !defined(CXPERTFORM_H__INCLUDED_)
