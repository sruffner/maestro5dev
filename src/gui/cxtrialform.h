//=====================================================================================================================
//
// cxtrialform.h : Declaration of class CCxTrialForm.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXTRIALFORM_H__INCLUDED_)
#define CXTRIALFORM_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "visualfx.h"                  // the Visual Framework library, including base class TVTabPane
#include "numedit.h"                   // the CNumEdit class -- a numeric-only, configurable version of CEdit
#include "gridctrl\litegrid.h"         // CLiteGrid -- a lightweight grid ctrl w/ inplace editor tools built-in

#include "cxobj_ifc.h"                 // the CNTRLX object interface:  common constants and type defs
#include "cxobjcombo.h"                // CCxObjCombo -- for selecting among existing "sibling" objects in CCxDoc
#include "cxdoc.h"                     // the CCxDoc class encapsulates the associated document
#include "cxtrial.h"                   // the CCxTrial class encapsulates a CNTRLX trial data object

class CCxTrialForm;                    // forward declaration


//=====================================================================================================================
// CCxMainPage, CCxRandVarsPage, CCxPertsPage : CPropertyPage containers for the majority of controls on the trial
// form, for a  more compact presentation. CCxTrialForm is declared a friend class on each so that it can access 
// private members, and these are friends of CCxTrialForm so that they can forward control notifications to it. ALL
// real functionality is implemented in CCxTrialForm.
//=====================================================================================================================
//
class CCxMainPage : public CPropertyPage
{
   DECLARE_DYNCREATE( CCxMainPage )

   friend class CCxTrialForm;

private:
   CCxTrialForm* m_pTrialForm;

   CCxObjCombo m_cbSelChan;                     // combo box used to select channel config assoc. w/ trial
   CComboBox m_cbSpecOp;                        // combo box used to select special operation in effect (if any)
                                                // spin controls paired with read-only "auto buddy" edit controls:
   CSpinButtonCtrl   m_spinWeight;              //    relative trial weight
   CSpinButtonCtrl   m_spinSave;                //    "first save" segment
   CSpinButtonCtrl   m_spinFailsafe;            //    "failsafe" segment
   CSpinButtonCtrl   m_spinSpecial;             //    "sacc-trig'd op" segment
   CSpinButtonCtrl   m_spinMark1;               //    "display marker" segments #1 and #2
   CSpinButtonCtrl   m_spinMark2;               //

                                                // formatted numeric edit controls:
   CNumEdit          m_edSaccVt;                //    saccade threshold velocity
   CNumEdit          m_edRewP1;                 //    reward pulse length 1
   CNumEdit          m_edWHVR1Num;              //    numer/denom for withholding VR for reward pulse 1
   CNumEdit          m_edWHVR1Den;
   CNumEdit          m_edRewP2;                 //    reward pulse length 2
   CNumEdit          m_edWHVR2Num;              //    numer/denom for withholding VR for reward pulse 2
   CNumEdit          m_edWHVR2Den;
   CNumEdit          m_edStairStren;            //    staircase strength
   CNumEdit          m_edMTRIntv;               //    mid-trial reward interval
   CNumEdit          m_edMTRLen;                //    mid-trial reward pulse length
   CNumEdit          m_edWeight;                //    trial weight (editable buddy window for a spin ctrl!)

   CCxMainPage(const CCxMainPage& src);                   // no copy constructor or assignment operator defined
   CCxMainPage& operator=( const CCxMainPage& src );

public:
   CCxMainPage() : CPropertyPage(IDD_TRIALFORM_MAIN)
   { 
      m_pTrialForm = NULL; 
      m_psp.dwFlags |= PSP_PREMATURE;
   }

   ~CCxMainPage() {}

protected:
   DECLARE_MESSAGE_MAP()
   afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pWnd);
   afx_msg void OnChange(UINT id);
   afx_msg void OnChanCfgSelect();
   afx_msg void OnSpecOpSelect();

private:
   VOID SetParentForm(CCxTrialForm* pFm) { m_pTrialForm = pFm; }
public:
   BOOL OnInitDialog();
   BOOL PreTranslateMessage(MSG* pMsg);
};

class CCxRandVarsPage : public CPropertyPage
{
   DECLARE_DYNCREATE( CCxRandVarsPage )

   friend class CCxTrialForm;

private:
   CCxTrialForm* m_pTrialForm;

   // the page only contains this grid control in which all trial RV's are listed
   CLiteGrid m_rvGrid; 

   CCxRandVarsPage(const CCxRandVarsPage& src);               // no copy constructor or assignment operator defined
   CCxRandVarsPage& operator=( const CCxRandVarsPage& src );

public:
   CCxRandVarsPage() : CPropertyPage(IDD_TRIALFORM_RV) 
   { 
      m_pTrialForm = NULL; 
      m_psp.dwFlags |= PSP_PREMATURE;
   }
   ~CCxRandVarsPage() {}

protected:
   DECLARE_MESSAGE_MAP()

   // when focus cell changes on RV grid, redraw the header row so column labels are updated
   afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
   {
      m_rvGrid.RedrawRow(0);
   }
   // stop propagation of mousewheel event to parent CCxTrialForm -- it was likely intended for the RV grid!
   afx_msg BOOL OnMouseWheel(UINT, short, CPoint)
   {
      return(TRUE);
   }

private:
   VOID SetParentForm(CCxTrialForm* pFm) { m_pTrialForm = pFm; }
public:
   BOOL OnInitDialog();
   BOOL PreTranslateMessage(MSG* pMsg);
};

class CCxPertsPage : public CPropertyPage
{
   DECLARE_DYNCREATE( CCxPertsPage )

   friend class CCxTrialForm;

private:
   CCxTrialForm* m_pTrialForm;

   CComboBox m_cbSgmOp;                                 // combo box used to select PSGM operational mode
   CSpinButtonCtrl m_spinSgmSeg;                        // spinner w/ RO edit sets the PSGM start segment
   CNumEdit m_edSgmPulseAmp1;                           // PSGM pulse 1,2 amplitudes
   CNumEdit m_edSgmPulseAmp2;
   CNumEdit m_edSgmPulseWidth1;                         // PSGM pulse 1,2 widths
   CNumEdit m_edSgmPulseWidth2;
   CNumEdit m_edSgmInterPulse;                          // PSGM interpulse interval
   CNumEdit m_edSgmInterTrain;                          // PSGM intertrain interval
   CNumEdit m_edSgmNP;                                  // #pulses per PSGM pulse train
   CNumEdit m_edSgmNT;                                  // #trains per PSGM stimulus

   CLiteGrid m_pertGrid;                                // grid control displaying trial's "perturbation list"

   CCxPertsPage(const CCxPertsPage& src);               // no copy constructor or assignment operator defined
   CCxPertsPage& operator=( const CCxPertsPage& src );

public:
   CCxPertsPage() : CPropertyPage(IDD_TRIALFORM_OTHER)
   { 
      m_pTrialForm = NULL; 
      m_psp.dwFlags |= PSP_PREMATURE;
   }

   ~CCxPertsPage() {}

protected:
   DECLARE_MESSAGE_MAP()
   afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pWnd);
   afx_msg void OnChange(UINT id);
   afx_msg void OnSelectSGMOp();
   afx_msg void OnNMRClick(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

   // stop propagation of mousewheel event to parent CCxTrialForm -- it was likely intended for the perts grid!
   afx_msg BOOL OnMouseWheel(UINT, short, CPoint)
   {
      return(TRUE);
   }

private:
   VOID SetParentForm(CCxTrialForm* pFm) { m_pTrialForm = pFm; }
public:
   BOOL OnInitDialog();
   BOOL PreTranslateMessage(MSG* pMsg);
};



//=====================================================================================================================
// Declaration of class CCxTrialForm
//=====================================================================================================================
//
class CCxTrialForm : public TVTabPane
{
   DECLARE_DYNCREATE( CCxTrialForm )

   // so that the property page objects can forward property changes to the trial form
   friend class CCxMainPage;
   friend class CCxRandVarsPage;
   friend class CCxPertsPage;

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TRIALFORM;        // resource ID for associated dialog template -- see constructor

   static LPCTSTR strPertListLabels[5];         // labels for column header row in "perturbation list" grid ctrl
   static LPCTSTR                               // human-readable labels for the different target trajectory components
      strPertAffectedCmptLabels[PERT_NCMPTS];   // that may be modulated by a perturbation

   static const int ROWSINHDR = 6;              // # of grid rows in each segment header
   static const int ROWSINTGT = 7;              // # of grid rows per target trajectory record
   typedef enum                                 // the different types of cells in the grid
   {
      NOTACELL = -1,                            //    an invalid cell
      SEGHLABEL,                                //    segment header row-label (col 0)
      SEGHFIELD,                                //    segment header field
      TGTSELECT,                                //    target selector (contains tgt's name)
      TGTJLABEL,                                //    target trajectory field row-label (col 0)
      TGTJFIELD                                 //    target trajectory field
   } CellType;

   typedef enum                                 // trial modification modes:
   {
      ATOMIC = 0,                               //    change specified parameter P in the trial's segment table
      ALLSEGS,                                  //    change parameter P across all segments of trial
      MATCHSEGS,                                //    change P from P0 to P1 in all segments s.t. P=P0 initially
      ALLTRIALS,                                //    propagate change across all trials in the current trial's set
      MATCHTRIALS,                              //    change P from P0 to P1 across all trials s.t. P=P0 initially
      SELTRIALS                                 //    propagate change across selected trials in current trial's set
   } ModifyMode;

   static LPCTSTR strSegHdrLabels[ROWSINHDR];   // labels for segment header fields
   static LPCTSTR strTrajLabels[ROWSINTGT];     // labels for target trajectory fields
   static const COLORREF clrYellow;             // some colors used in the grid controls
   static const COLORREF clrLtGrn;
   static const COLORREF clrBlue;
   static const COLORREF clrWhite;
   static const COLORREF clrMedGray;
   static const COLORREF clrRed;
   static const int SEGCOL_W;                   // (fixed) width of segment columns in pixels
   static const UINT SECTCREATE_TIMEOUT;        // timeout (msecs) for the tagged section-create gesture

   struct CPartition                            // a trial partition is either a tagged section or an untagged segment;
   {                                            // each partition corresponds to a cell in the trial partitions grid
      int iFirstSeg;                            //    zero-based index of first segment in this partition
      int iLastSeg;                             //    zero-based index of last segment in this partition
      int iSection;                             //    zero-based index of the tagged section; -1 if this partition is
                                                //    an individual untagged segment
   };

   static LPCTSTR strRVTypeLabels[RV_NUMTYPES]; // labels for the different types of random variable

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
protected:
   BOOL              m_bOneTimeInitsDone;       // TRUE once one-time (vs per-doc) inits done; see OnInitialUpdate()
   BOOL              m_bLoading;                // this transient flag is set whenever we're loading a trial onto form
                                                // so that grid control display callbacks don't try to access stale ptr

   WORD              m_wKey;                    // unique key of CNTRLX trial object currently displayed on form
   CCxTrial*         m_pTrial;                  // ptr to CNTRLX trial object currently displayed on form
   BOOL              m_bEnable;                 // if TRUE, modifiable controls are enabled for user input

   CWordArray        m_arDepObjs;               // keys of CNTRLX objects upon which current trial is dependent

   CCxSegment*       m_pPasteSeg;               // ptr to trial segment object last copied by user; can be pasted into
                                                //    any compatible trial loaded into view
   CCellID           m_contextCell;             // grid cell right-clicked on seg or pert table (for popup menu ops)
   int               m_iContextSeg;             // segment under right-click on partitions grid (for popup menu ops)
   UINT              m_nRightClickedGrid;       // ID of grid that was last right-clicked (for popup menu ops)
   int               m_iInsPos;                 // transient var: >=0 when user selecting tgt to add to trial
   WORD              m_wLastTgtKey;             // key of last target obj added to the trial

   CSize             m_minGridSize;             // minimum size (based on dlg template) of the segment grid
   CSize             m_minScrollSize;           // min scroll size of form (== size of dlg template)

   ModifyMode        m_modifyMode;              // modification mode currently in effect

   CCellID           m_tagSectAnchorCell;       // anchor cell for gesture that creates a new tagged section
   UINT_PTR          m_nSectCreateTimerID;      // tagged section "create gesture" must complete before timer expires

   int               m_nPartitions;             // the loaded trial's partitions, as reflected in the partitions grid
   CPartition        m_partitions[MAX_SEGMENTS];

   // most controls on form are now distributed across three property pages in a property sheet that lies at the top 
   // of the form. Only the segment grid, partition grid, and the property sheet itself are children of the form.
   CCxMainPage m_mainPage;
   CCxPertsPage m_pertsPage;
   CCxRandVarsPage m_rvPage;
   CPropertySheet* m_pPropSheet;
   
   // the segment table and partition grid (for seg #s and tagged sections
   CLiteGrid m_segGrid; 
   CLiteGrid m_partitionGrid; 

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTrialForm( const CCxTrialForm& src );              // no copy constructor defined
   CCxTrialForm& operator=( const CCxTrialForm& src );   // no assignment operator defined

public:
   CCxTrialForm();                                       // (NOTE: used by dynamic object creation mechanism)
                                                         // form is constructed from dialog template resource
   ~CCxTrialForm();



//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnVScroll( UINT nSBCode, UINT nPos,      // handle WM_VSCROLL msgs from spin ctrls on form
                           CScrollBar* pWnd );
   afx_msg void OnChange( UINT id );                     // update header param in response to change in assoc control
   afx_msg void OnComboSelChange( UINT id );             // respond to selection change in combo boxes
   afx_msg void OnUpdateEditCommand( CCmdUI* pCmdUI );   // update enable state of standard "Edit" menu cmds
   afx_msg void OnEditCommand( UINT nID );               // perform selected standard "Edit" menu cmds
   afx_msg void OnNMRClick( UINT id, NMHDR* pNMHDR,      // handle NM_RCLICK notification from segment table grid or
                            LRESULT* pResult );          //    perturbation list grid
   afx_msg void OnNMClick( NMHDR* pNMHDR,                // handle NM_CLICK notification from partitions grid
                           LRESULT* pResult );
   afx_msg void OnTimer( UINT_PTR nEventID );            // timeout for the tagged section-create gesture
   afx_msg void OnGridOps( UINT cmdID );                 // handle user-initiated operations on one of the grids
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

   VOID LoadTrial( const WORD key );                     // load specified CNTRLX trial's definition onto form view
   VOID UpdateCaption( LPCTSTR szCaption );              // update tab pane caption
   VOID StuffHdrControls();                              // stuff trial header params into associated controls on form
   VOID StuffHdrPB(const TRLHDR& hdr,const UINT id = 0); // update label(s) of one or all PB controls on form
   VOID EnableHdrControls();                             // updates enable states of certain controls on form IAW
                                                         // current values in loaded trial's header

   VOID ResizeSegmentTable();                            // resize seg table so it does not have scroll bars, then
                                                         // update form's scroll size accordingly

   VOID InformModify();                                  // inform doc & other views that trial was modified

   static BOOL CALLBACK PertGridDispCB(                  // CB invoked by pert grid to get cell text & display info
            GV_DISPINFO *pDispInfo, LPARAM lParam );
   static BOOL CALLBACK PertGridEditCB(                  // CB invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK PertGridEndEditCB(               // CB invoked when upon termination of inplace editing
            ENDEDITINFO *pEEI, LPARAM lParam );

   static BOOL CALLBACK GridDispCB(                      // CB invoked by segment grid to get cell text & display info
            GV_DISPINFO *pDispInfo, LPARAM lParam );
   static BOOL CALLBACK GridEditCB(                      // CB invoked to initiate inplace editing of grid cell
            EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK GridEndEditCB(                   // CB invoked when upon termination of inplace editing
            ENDEDITINFO *pEEI, LPARAM lParam );

   CellType GetCellType( const CCellID& c ) const;       // returns enum type of grid cell
   CCxTrial::ParamID GetCellParam(                       // returns enum type identifying param displayed in grid cell
                              const CCellID& c ) const;

   BOOL IsSegmentHeaderLabel( const CCellID& c ) const   // does cell hold row label for a segment header field?
   {
      return( GetCellType( c ) == SEGHLABEL );
   }
   BOOL IsTargetSelector( const CCellID& c ) const       // does cell hold name of a target participating in trial?
   {
      return( GetCellType( c ) == TGTSELECT );
   }
   BOOL IsTargetFieldLabel( const CCellID& c ) const     // does cell hold row label of a target trajectory field?
   {
      return( GetCellType( c ) == TGTJLABEL );
   }
   BOOL IsSegmentHeaderField( const CCellID& c ) const   // is cell part of the header region for a segment?
   {
      return( GetCellType( c ) == SEGHFIELD );
   }
   BOOL IsTrajectoryField( const CCellID& c ) const      // is cell part of a trajectory record within a segment?
   {
      return( GetCellType( c ) == TGTJFIELD );
   }

   int CellToSeg( const CCellID& c ) const               // maps segment grid cell to index of assoc trial segment
   {                                                     // (if any). REM:  each segment is 2 adjacent columns
      ASSERT( ::IsWindow( m_segGrid.m_hWnd ) );
      return( (m_segGrid.IsValid(c) && (c.col > 0))
              ? (c.col-1)/2 : -1 );
   }
   int SegToColumn( const int iSeg ) const               // maps zero-based seg# to the **first** of 2 adjacent columns
   {                                                     // w/in segment grid that displays that segment's params
      return( (iSeg<0) ? -1 : iSeg*2 + 1 );
   }
   int CellToTarg( const CCellID& c ) const              // maps segment grid cell to index of assoc participating tgt
   {                                                     // (if any)
      ASSERT( ::IsWindow( m_segGrid.m_hWnd ) );
      int i = c.row - ROWSINHDR;
      return( (m_segGrid.IsValid(c) && (i>=0))
              ? (i/ROWSINTGT) : -1 );
   }

   // propagate changes in the current trial's definition IAW the current modification mode. NOTE that changes to the
   // trial's random variables list are NOT propagated.
   VOID PropagateHeader(UINT ctrlID, TRLHDR& oldHdr); 
   VOID PropagateSegParam(int iSeg, int iTgt, CCxTrial::ParamID pID, double dOldVal, BOOL wasRV);
   VOID PropagatePertParam(int iCol, int iPert, double dOldVal);
   VOID PropagatePertOp(UINT cmdID, int iPert, int nP);
   VOID PropagateSegOp(int nT, int nS, int iSeg, UINT cmdID);
   VOID PropagateTgtOp(int nT, int nS, int iTgt, UINT cmdID);

                                                         // callbacks for the trial partitions grid...
   static BOOL CALLBACK PartitionGridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam );
   static BOOL CALLBACK PartitionGridEditCB( EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK PartitionGridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam );

   VOID RebuildPartitionGrid();                          // rebuild partitions grid IAW current state of loaded trial

   VOID HandleSectionCreateGesture(                      // init, complete, or cancel tagged section-create gesture
      BOOL bShift, CCellID clickedCell);
   VOID CancelSectionCreateGesture();                    // cancel the tagged section-create gesture

   // callbacks for the random variables grid on the "Random Variables" tab page
   static BOOL CALLBACK RVGridDispCB(GV_DISPINFO *pDispInfo, LPARAM lParam );
   static BOOL CALLBACK RVGridEditCB(EDITINFO *pEI, LPARAM lParam );
   static BOOL CALLBACK RVGridEndEditCB(ENDEDITINFO *pEEI, LPARAM lParam );
};



#endif   // !defined(CXTRIALFORM_H__INCLUDED_)
