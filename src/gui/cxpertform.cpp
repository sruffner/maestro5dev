//===================================================================================================================== 
//
// cxpertform.cpp : Implementation of class CCxPertForm.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CCxPertForm is a dialog-like form view that manages the display and modification of CNTRLX "perturbation waveforms". 
// All perturbation objects (type CX_PERTURB, class CCxPert) are children of a single node (CX_PERTBASE) in the CNTRLX 
// object tree.  Unlike some other CNTRLX forms, which display one data object at a time, CCxPertForm displays ALL 
// perturbation objects under the predefined CX_PERTBASE node.  Since perturbation objects are relatively simple, 
// defined by a few discreet parameters, each perturbation can be compactly represented by one line in a spreadsheet-
// like grid -- the "perturbation table".  See CCxPert for a detailed description of the CNTRLX perturbation object.
//
// ==> Construction of form; controls.
// CCxPertForm is laid out in the dialog template resource IDD_PERTFORM.  With this ID supplied in the default 
// constructor, the Visual Framework (CREDITS) and MFC handle the details of loading the template and creating the 
// view.  Use the DevStudio Resource Editor to review the layout of IDD_PERTFORM.  In its current incarnation, the form 
// is filled by a single custom control, the MFC Grid Control (CREDITS), representing the table of defined perturbation 
// objects.  Whenever the user resizes the form beyond its initial dimensions (those defined in the resource template), 
// we will extend the right and bottom sides of the grid as well -- the grid acts as if its top and left sides are 
// fixed, while the right and bottom sides are attached to the right and bottom sides of this form view.  Thus, by 
// making the form larger, the user will make the perturbation table larger.
//
// [DEVNOTE:  It seems like overkill to create a dialog template resource for such a simple form.  We do it this way 
// for two reasons: 1) to be consistent with how other CNTRLX forms are created; and 2) because we may introduce other 
// controls on the form in the future.]
//
// ==> Interactions with CCxPert and CCxDoc.
// Like all data objects in CNTRLX, each different perturbation object CCxPert is assigned a unique key.  CCxPertForm 
// maintains an array holding the key for each perturbation currently displayed on the form.  Then, whenever it needs 
// to access to a perturbation's definition, CCxPertForm supplies the key to CCxDoc in order to obtain a CCxPert* ptr 
// to the actual perturbation object.  It then queries the CCxPert object directly to retrieve or modify individual 
// parameters.  Whenever it modifies a CCxPert object, it must notify the doc & other views of the change -- see 
// InformModify(). 
//
// Because the form displays multiple CCxPert objects, CCxPertForm must refresh the entire grid whenever the user adds 
// or removes objects under the CX_PERTBASE node -- see OnUpdate() for more details.
//
// ==> The "perturbation table" and CLiteGrid; editing individual attributes "in place".
// CLiteGrid -- a derivative of the MFC grid control, CGridCtrl (CREDITS) -- is used to display all CNTRLX perturbation 
// objects in a condensed tabular form.  We take advantage of the grid's "virtual mode", a lightweight implementation 
// that avoids the memory overhead of associating a CGridCell-derived object with each and every cell in the grid.  In 
// this scheme, the grid ctrl uses a registered callback function to obtain info needed to repaint any grid cell.  This 
// callback function, CCxPertForm::GridDispCB(), determines the appearance of the grid.  The grid itself is initialized 
// with the appropriate number of rows and columns in OnInitialUpdate().  Rows are added and deleted as perturbation 
// objects are added or removed under the CX_PERTBASE node.
//
// CLiteGrid provides some facilities for "in place" editing of grid cells that are not available in CGridCtrl itself 
// when the grid is in virtual mode.  Inplace editor controls are provided for several classes of data (text string, 
// numbers, multiple-choice).  The user may initiate inplace editing in a variety of ways (eg, left dbl-click cell, hit 
// F2 key with the focus on the cell of interest, etc.), or in some cases, cell contents can be changed by merely 
// clicking on the cell with the left or right mouse button.  To use these facilities and tailor them to the kinds of 
// data stored in the grid, we must install two additional callback functions:  GridEditCB() is invoked when an inplace 
// edit operation is about to take place (or when a mouse click occurs in the cell), and GridEndEditCB() is called to 
// complete the operation once the internal inplace edit control is extinguished.  See CLiteGrid for more information 
// on how these callback methods are invoked and used.
//
// Here is a summary of the perturbation parameters displayed in the grid, and how they may be modified:
//
//    Col 0:  Name         -- The CNTRLX object name for the perturbation waveform.  Can be modified using an inplace 
//                            text edit box, managed by CLiteGrid.
//    Col 1:  Type         -- Perturbation type.  A multiple-choice parameter.  Value can be changed by a left or right 
//                            mouse click, or using an inplace combo box.
//    Col 2:  Duration     -- Duration of the perturbation in ms.  Modified by an inplace numeric edit box.
// 
//    Other columns        -- The first three parameters above are common to all perturbation objects.  The remaining 
//                            columns are dedicated to type-specific parameters, the number and identities of which 
//                            vary with perturbation type.
//
// CCxPert has been specifically designed with this kind of GUI representation in mind.  Thus, with the exception of 
// the CNTRLX object name, the perturbation parameter can be identified by a zero-based index.  Given any valid cell in 
// the grid, the cell's row determines which CCxPert object we're looking at, and the column indicates which parameter 
// is displayed in that cell.  For details, see GridDispCB(), GridEditCB(), and GridEndEditCB().
//
// Note that, since the makeup of the type-specific parameter list changes with perturbation type, we cannot use fixed 
// column header labels to describe these parameters.  Instead, we take advantage of CLiteGrid's "label tip" feature:
// Whenever the grid's "focus cell" changes, it queries the display callback for an optional "label tip".  If the 
// callback provides a non-empty label, the grid will display the text in a tooltip positioned just above the focus 
// cell.  CCxPertForm provides a label for each type-specific parameter cell in the perturbation table.  For details, 
// see GridDispCB().
//
//
// CREDITS:
// (1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21. 
// This extremely useful spreadsheet-like control (CGridCtrl) provides an efficient way to present modifiable tabular 
// data.  We've modified its original implementation somewhat to our purposes.  In addition, we use the derived class 
// CLiteGrid, which provides some built-in inplace editing functionality when the grid is in virtual mode.
//
// (2) The "Visual Framework" library, by Zoran M. Todorovic [09/30/2000, www.codeproject.com/dialog/visualfx.asp] --
// We use this library to implement the view framework that appears in the CNTRLX frame window's (CCxMainFrame) client 
// area.  This framework makes more complex GUI layouts -- that would have been a bear to implement w/core MFC -- easy 
// to build.  This form class is derived from TVTabPane, a class that I added to the Visual Framework.
//
//
// REVISION HISTORY:
// 13nov2002-- Begun development.
// 28jul2005-- Increased the (fixed) column width for parameters from 60 to 100.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX 

#include "cxobj_ifc.h"                       // CNTRLX object interface:  common constants and other defines 
#include "cxviewhint.h"                      // the "hint" class used by all CNTRLX views 
#include "cxpert.h"                          // CCxPert -- encapsulates the CNTRLX perturbation object
#include "cxpertform.h" 


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxPertForm, TVTabPane )

BEGIN_MESSAGE_MAP( CCxPertForm, TVTabPane )
   ON_WM_SIZE()
   ON_UPDATE_COMMAND_UI_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnUpdateEditCommand )
   ON_COMMAND_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnEditCommand )
   ON_NOTIFY(GVN_SELCHANGED, IDC_PERT_GRID, OnSelChanged)
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxPertForm, ~CCxPertForm [constructor, destructor] ============================================================= 
//
//    Construct the perturbation object form view. 
//
//    Almost all the work is handled by the framework, which loads the form's layout from a CNTRLX resource whose 
//    identifier is stored in CCxPertForm's protected member var IDD.  However, we do need to initialize certain 
//    variables that track the form's state.
//
//    When the destructor is called, the contents of the associated document object are gone -- so we disable all 
//    callbacks to make sure we don't try to invoke them during the destruction of the grid control.
//
CCxPertForm::CCxPertForm() : TVTabPane( IDD )
{
   m_bOneTimeInitsDone = FALSE;
}

CCxPertForm::~CCxPertForm()
{
   m_grid.SetCallbackFunc( NULL, NULL );
   m_grid.SetEditCBFcn( NULL, NULL );
   m_grid.SetEndEditCBFcn( NULL, NULL );
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnSize [base override] ========================================================================================== 
//
//    Response to WM_SIZE message.
//
//    Currently, the perturbation grid is the only control on CCxPertForm.  To maximize how much of the grid can be 
//    seen at one time, we permit the grid's right or bottom side to "stick" to the form's corresponding side whenever 
//    doing so would make the grid *larger* than its minimum size.  The top left corner of the grid rect does not move. 
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
void CCxPertForm::OnSize( UINT nType, int cx, int cy ) 
{
   TVTabPane::OnSize( nType, cx, cy );                      // base class stuff first 

   if( m_grid.GetSafeHwnd() == NULL ) return;               // there's no grid ctrl to resize!

   CRect rGridCurr;                                         // get current grid rect in form coords 
   m_grid.GetWindowRect( rGridCurr );                       // (accounts for possible scrolling)
   ScreenToClient( rGridCurr );

   CSize szClient, szBars;                                  // get form's true client size & scroll bar sizes
   GetTrueClientSize( szClient, szBars );

   DWORD dwStyle = GetStyle();                              // are scroll bars there or not?
   BOOL bHasH = BOOL((dwStyle & WS_HSCROLL) != 0);
   BOOL bHasV = BOOL((dwStyle & WS_VSCROLL) != 0);

   CRect rGrid = rGridCurr;                                 // adjust grid rect IAW sizing event:
   int iSide = szClient.cx - 7;                             //    stick right side of grid to right side of form, 
   if( bHasV ) iSide -= szBars.cx;                          //    accounting for possible presence of vert scroll bar, 
   if( (iSide - rGrid.left) >= m_minGridSize.cx )           //    unless minimum grid width would be violated
      rGrid.right = iSide;
   else
      rGrid.right = rGrid.left + m_minGridSize.cx;          //    otherwise: return grid width to minimum

   iSide = szClient.cy - 7;                                 //    similarly w/ bottom of grid, acct for H scroll bar
   if( bHasH ) iSide -= szBars.cy;
   if( (iSide - rGrid.top) >= m_minGridSize.cy ) 
      rGrid.bottom = iSide;
   else 
      rGrid.bottom = rGrid.top + m_minGridSize.cy;
   
   if( rGrid != rGridCurr ) m_grid.MoveWindow( rGrid );     // if grid rect adjusted, move it now.
}


//=== OnUpdateEditCommand ============================================================================================= 
// 
//    ON_UPDATE_COMMAND_UI_RANGE handler for the predefined ID_EDIT_*** commands:  Update enable state of selected 
//    items in the app's Edit menu depending on the current state of the clipboard and the edit control that currently 
//    has the focus on this form.  An edit control must currently have the focus for any of the items to be enabled.
//
//    NOTE that only some of the ID_EDIT_*** commands are actually implemented. 
//
//    ARGS:       pCmdUI -- [in] represents UI item. 
//
//    RETURNS:    NONE
//
void CCxPertForm::OnUpdateEditCommand( CCmdUI* pCmdUI )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a 
   if ( (pFocusWnd == NULL) ||                                 // CEdit obj, then disable all Edit cmds -- no other 
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CEdit) )) )       // controls on this form support ID_EDIT_*** commands. 
   {
      pCmdUI->Enable( FALSE );
      return;
   }

   CEdit *pEditC = (CEdit *) pFocusWnd;                        // obj w/focus is a CEdit, so we can safely cast ptr 

   BOOL bEnable = FALSE;                                       // enable state of edit command depends on current 
   int iStart = 0;                                             // state of the CEdit ctrl w/ the input focus: 
   int iEnd = 0;
   switch( pCmdUI->m_nID )
   {
      case ID_EDIT_CUT :                                       // ...at least one char must be selected 
      case ID_EDIT_COPY :
         pEditC->GetSel( iStart, iEnd );
         bEnable = (iStart != iEnd);
         break;
      case ID_EDIT_PASTE :                                     // ...there must be appropriate clipboard data avail 
         bEnable = ::IsClipboardFormatAvailable( CF_TEXT );
         break;
      case ID_EDIT_UNDO :
         bEnable = pEditC->CanUndo();
         break;
      default :
         break;
   }
   pCmdUI->Enable( bEnable );

   return;
}


//=== OnEditCommand =================================================================================================== 
// 
//    ON_COMMAND_RANGE handler for the ID_EDIT_*** commands:  Update state of the focussed edit control on this form 
//    IAW the edit command given.
//
//    NOTE that only some of the ID_EDIT_*** commands are actually implemented. 
//
//    ARGS:       nID -- [in] edit command ID. 
//
//    RETURNS:    NONE
//
void CCxPertForm::OnEditCommand( UINT nID )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a 
   if ( (pFocusWnd == NULL) ||                                 // CEdit obj, do nothing -- all alterable edit ctrls  
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CEdit) )) )       // on this form are attached to CEdit objects! 
      return;

   CEdit *pEditC = (CEdit *) pFocusWnd;                        // obj w/focus is a CEdit, so we can safely cast ptr 

   switch( nID )                                               // process the operation by calling the appropriate 
   {                                                           // CEdit method...
      case ID_EDIT_CUT :   pEditC->Cut();    break;
      case ID_EDIT_COPY :  pEditC->Copy();   break;
      case ID_EDIT_PASTE : pEditC->Paste();  break;
      case ID_EDIT_UNDO :  pEditC->Undo();   break;
      default :                              break;
   }
}



//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 

//=== OnInitialUpdate [base override] ================================================================================= 
//
//    This function is called by the SDI doc/view framework each time a new document is created/opened.  Here we take 
//    care of both one-time inits and per-document inits; the one-time inits are only performed the first time that 
//    this method is invoked.
//
//    To make the grid control work, we must first subclass it to our member CLiteGrid object.  We then set it up with 
//    the appropriate number of columns to display all possible perturbation parameters.  Since the grid is initially 
//    empty, it contains only a single "header" row at startup.
//
//    Another "one-time init" is to retrieve and save the original size of the grid control as specified in the dialog 
//    template.  While we want to "grow" the grid as the form is enlarged beyond its original size, we do not want to 
//    shrink it beyond its original, template size.  If we can grow the grid at startup, we do so.
//
//    ARGS:       NONE
// 
//    RETURNS:    NONE
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for required rows and columns.
//
void CCxPertForm::OnInitialUpdate() 
{
   if( !m_bOneTimeInitsDone )                                  // ONE-TIME INITIALIZATIONS:
   {
      m_grid.SubclassDlgItem( IDC_PERT_GRID, this );           // attach custom ctrl to our grid ctrl obj

      m_grid.EnableDragAndDrop( FALSE );                       // set up the grid...
      m_grid.SetRowResize( FALSE );
      m_grid.SetMinColWidth( 100 );
      m_grid.SetColumnResize( FALSE );
      m_grid.EnableSelection( FALSE );

      m_grid.SetCallbackFunc( GridDispCB, (LPARAM) this );     // install callbacks which govern appearance/editing 
      m_grid.SetEditCBFcn( GridEditCB, (LPARAM) this );        // of grid cells.  TRICK: we pass THIS reference b/c CB  
      m_grid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );  // fcn must be static and so does not get implied THIS.

      try                                                      // set #cols to display all possible perturbation params 
      {                                                        // plus the pert obj name.  only 1 row allocated, since 
         m_grid.SetRowCount( 1 );                              // grid is empty initially...
         m_grid.SetColumnCount( 1 + CCxPert::MaxNumberOfParameters() );
         m_grid.SetFixedRowCount( 1 );
         m_grid.SetFixedColumnCount( 1 ); 
      }
      catch (CMemoryException* e)
      {
         e->ReportError();
         e->Delete();
         return;
      }

      DWORD dwFormat = DT_RIGHT | DT_SINGLELINE;               // adjust format of default cells:
      CGridCellBase* pCell = m_grid.GetDefaultCell(TRUE,TRUE); //    default for cell on a fixed row & fixed col
      pCell->SetFormat( dwFormat );
      pCell = m_grid.GetDefaultCell( FALSE, TRUE );            //    default for cell on normal row, fixed col
      pCell->SetFormat( dwFormat );
      pCell = m_grid.GetDefaultCell( TRUE, FALSE );            //    default for cell on fixed row, normal col
      dwFormat = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
      pCell->SetFormat( dwFormat );
      pCell = m_grid.GetDefaultCell( FALSE, FALSE );           //    default for cell on a normal row & col
      pCell->SetFormat( dwFormat );

      // use a bold font for the fixed row and column, which serve as table headers
      m_grid.GetDefaultCell(FALSE, TRUE)->GetFont()->lfWeight = FW_BOLD;
      m_grid.GetDefaultCell(TRUE, FALSE)->GetFont()->lfWeight = FW_BOLD;

      m_grid.SetGridLineColor(::GetSysColor(COLOR_3DSHADOW));  // grid line color

//      m_grid.AutoSize( GVS_BOTH );                           // autosize grid based on col header labels, 
      if( m_grid.GetColumnWidth( 0 ) < 200 )                   // then ensure col for pert obj name is at least 200pix 
         m_grid.SetColumnWidth( 0, 200 );

      CRect rGrid;                                             // save grid ctrl's initial size, which is defined by 
      m_grid.GetWindowRect( rGrid );                           // dlg template, to ensure that grid is not made smaller 
      ScreenToClient( rGrid );                                 // than this 
      m_minGridSize = rGrid.Size();

      CRect rClient;                                           // if we can enlarge grid upon initial display, do so 
      GetClientRect( rClient ); 
      if( (rClient.right - 7) > rGrid.right ) rGrid.right = rClient.right - 7;
      if( (rClient.bottom - 7) > rGrid.bottom ) rGrid.bottom = rClient.bottom - 7;
      if( m_minGridSize != rGrid.Size() )
         m_grid.MoveWindow( rGrid );

      m_bOneTimeInitsDone = TRUE;                              // do NOT repeat these inits again!
   }

   Load();                                                     // per-doc init:  reload form IAW current doc contents 
   TVTabPane::OnInitialUpdate();                               // base class stuff -- ultimately calls OnUpdate()
}


//=== OnUpdate [base override] ======================================================================================== 
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    This view must respond to a number of different "signals" broadcast by other views attached to the CCxDoc obj: 
//       CXVH_NEWOBJ:   Must update pert grid whenever a new perturbation obj is created, or whenever multiple CNTRLX 
//                      objects of unspecified type are created.
//       CXVH_MOVOBJ:   Reload pert grid in case the order of pertubartion objects has been changed.
//       CXVH_DSPOBJ:   Display a data object.  If the object to be displayed is a perturbation object, we respond by 
//                      ensuring that the object is visible, and moving focus to the cell holding the object's name.
//       CXVH_MODOBJ:   If another view modifies a CNTRLX object, it may send this hint.  If a perturbation object was 
//                      modified, then we just refresh the relevant row in the perturbation table.
//       CXVH_NAMOBJ:   If a perturbation object was renamed, we refresh the cell displaying that object's name.  
//       CXVH_DELOBJ:   If a perturbation object was deleted, we delete the corresponding row in the grid; if multiple
//                      objects were deleted, we reload the entire perturbation table.
//       CXVH_CLRUSR:   Clear contents of the perturbation table.
//    
//    NOTES:
//    (1) Whenever a hint is NOT provided, we only call the base class -- to handle lower-level update tasks. 
//
//    ARGS:       pSender  -- [in] view which initiated the update
//                lHint    -- [in] an integer-valued hint (not used by CNTRLX views)
//                pHint    -- [in] if the initiating view provides a hint, this should be a CCxViewHint object.
// 
//    RETURNS:    NONE
//
void CCxPertForm::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   if( pHint == NULL )                                         // no hint provided -- could be a doc reload, so
   {														   // we reload pert table to ensure it is up-to-date
      Load(); 
      return;
   }

   CCxDoc* pDoc = GetDocument();                               // get reference to attached doc 
   CCxViewHint* pVuHint = (CCxViewHint*)pHint;                 // cast provided hint to the CNTRLX hint class 
   int iRow = FindPerturbationRow( pVuHint->m_key );           // find row assoc w/ pert obj (-1 if there isn't any)

   switch( pVuHint->m_code )
   {
      case CXVH_NEWOBJ :                                       // if a pert obj created/moved, or if multiple objs of 
      case CXVH_MOVOBJ :                                       // unspecified type created/moved, reload pert table to 
         if(pVuHint->m_type==0 || pVuHint->m_type==CX_PERTURB) // ensure it is up-to-date
            Load(); 
         break;

      case CXVH_DSPOBJ :                                       // if displaying a perturbation object that exists on 
         if( pVuHint->m_type == CX_PERTURB && iRow >= 0 )      // this form...
         {
            BringToFront();                                    //    bring this view to front of tab window 
            m_grid.EnsureVisible( iRow, 0 );                   //    shift focus to cell that holds pert obj's name
            m_grid.SetFocusCell( iRow, 0 );
         }
         break;

      case CXVH_MODOBJ :                                       // if a perturbation object was modified outside this 
         if( pVuHint->m_type == CX_PERTURB && iRow >= 0 )      // view, then refresh corres. row in perturbation table 
            m_grid.RedrawRow( iRow );
         m_grid.RedrawRow(0);                                  // refresh header in case pert type of focus row changed
         break;

      case CXVH_NAMOBJ :                                       // if a pert obj name was changed, refresh corres cell
         if( pVuHint->m_type == CX_PERTURB && iRow >= 0 )      // in the perturbation table
            m_grid.RedrawCell( iRow, 0 );
         break;

      case CXVH_DELOBJ :                                       // one or more CNTRLX data objects deleted... reload
         Load();                                               // entire pert table to be safe.
         break;

      case CXVH_CLRUSR :                                       // all user-defined CNTRLX objects removed: we reload 
         Load();                                               // the pert table, effectively clearing it
         break;

      default :                                                // no response to any other hints...
         break;
   }
}


//=== OnCmdMsg [base override] ======================================================================================== 
//
//    This CCmdTarget overridable allows derived classes to extend the MFC's standard command routing.
//
//    Here we give the grid control a chance to handle a command before passing it on.
//
//    ARGS:       nID      -- [in] the command ID. 
//                nCode    -- [in] the command's notification code. 
//                pExtra   -- [in] usage depends on nCode.
//                pHInfo   -- [in] typically NULL.  See CCmdTarget::OnCmdMsg().
// 
//    RETURNS:    TRUE if the message was handled, FALSE otherwise. 
//
BOOL CCxPertForm::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) 
{
   if( ::IsWindow( m_grid.m_hWnd ) )
   {
      if( m_grid.OnCmdMsg( nID, nCode, pExtra, pHandlerInfo) )
         return( TRUE );
   }
   return( TVTabPane::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) );
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump internal state vars associated with this view. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CCxPertForm::Dump( CDumpContext& dc ) const
{
   TVTabPane::Dump( dc ); 
   dc << _T("# perturbation definitions displayed = ") << m_wArPertKeys.GetSize() << _T(".\n");
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate internal consistency of this view. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxPertForm::AssertValid() const
{
   TVTabPane::AssertValid();
}

#endif //_DEBUG




//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== Load ============================================================================================================ 
//
//    Load the entire perturbation table based on the current contents of the attached CNTRLX document.
//
//    All pertubation objects (type CX_PERTURB) are stored as children of the predefined CX_PERTBASE node.  We query 
//    the CNTRLX document for these children.  The perturbation definitions are displayed in the grid in the same order 
//    that they are obtained from the document.
//
//    ARGS:       wKey  -- [in] key of perturbation object to be found. 
// 
//    RETURNS:    grid row containing perturbation definition, or -1 if not found.
//
VOID CCxPertForm::Load()
{
   m_wArPertKeys.RemoveAll();                            // clear the internal key array

   CCxDoc* pDoc = GetDocument();                         // iterate over all pert objects under CX_PERTBASE node, 
   WORD wKey = pDoc->GetBaseObj( CX_PERTBASE );          // adding each object's key to our internal key array...
   POSITION pos = pDoc->GetFirstChildObj( wKey );
   CTreeObj* pObj = NULL;
   while( pos != NULL )
   {
      pDoc->GetNextChildObj( pos, wKey, pObj );
      m_wArPertKeys.Add( wKey );
   }

   // adjust #rows in pert table accordingly. If no adj required, we still need to refresh grid because contents may
   // have changed. (note that SetRowCount() will refresh the grid)
   int nRows = 1 + static_cast<int>(m_wArPertKeys.GetSize()); 
   if(m_grid.GetRowCount() == nRows) m_grid.Refresh(); 
   else m_grid.SetRowCount(nRows); 
}


//=== FindPerturbationRow ============================================================================================= 
//
//    Map perturbation object key to row in perturbation table.
//
//    ARGS:       wKey  -- [in] key of perturbation object to be found. 
// 
//    RETURNS:    grid row containing perturbation definition, or -1 if not found.
//
int CCxPertForm::FindPerturbationRow( WORD wKey )
{
   int iFound = -1;
   for( int i = 0; i < m_wArPertKeys.GetSize(); i++ ) 
   {
      if( m_wArPertKeys[i] == wKey )
      {
         iFound = i + 1;                                 // account for the column header in row 0!!
         break;
      }
   }
   return( iFound );
}


//=== GetPertObjByRow ================================================================================================= 
//
//    Retrieve the CNTRLX perturbation object represented by specified row of the perturbation table.
//
//    ARGS:       iRow  -- [in] row in perturbation table.
//
//    RETURNS:    ptr to the perturbation object, or NULL if row does not correspond to a pert object.
//
CCxPert* CCxPertForm::GetPertObjByRow( int iRow ) 
{
   CCxPert* pPert = NULL;
   if( iRow > 0 && (iRow-1) < m_wArPertKeys.GetSize() )
   {
      pPert = (CCxPert*) GetDocument()->GetObject( m_wArPertKeys[iRow-1] );
      ASSERT_KINDOF( CCxPert, pPert );
   }
   return( pPert );
}


//=== IsGridCellReadOnly ============================================================================================== 
//
//    Is specified cell in perturbation table read-only?  The first row (column header labels) and first column (object 
//    names) are read-only, as are any cells that do not hold a perturbation parameter (some perturbation types have 
//    fewer parameters than others).
//
//    ARGS:       c  -- [in] (row,col) location of grid cell.
//
//    RETURNS:    TRUE if cell is read-only; FALSE otherwise.
//
BOOL CCxPertForm::IsGridCellReadOnly( CCellID c ) 
{
   if( (!m_grid.IsValid( c )) || c.row == 0 || c.col == 0 ) return( TRUE );
   CCxPert* pPert = GetPertObjByRow( c.row );
   if( pPert == NULL || c.col - 1 >= pPert->NumberOfParameters() ) return( TRUE );
   return( FALSE );
}


//=== InformModify ==================================================================================================== 
//
//    Invoke this method to inform the CNTRLX experiment document (CCxDoc) and other attached views that a perturbation 
//    object was just modified in this view.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxPertForm::InformModify( int iPertRow )
{
   ASSERT( iPertRow > 0 && iPertRow-1 < m_wArPertKeys.GetSize() );
   CCxDoc* pDoc = GetDocument(); 
   pDoc->SetModifiedFlag();
   CCxViewHint vuHint( CXVH_MODOBJ, CX_PERTURB, m_wArPertKeys[iPertRow-1] );
   pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );
}


//=== GridDispCB ====================================================================================================== 
//
//    Callback function queried by the embedded grid control to obtain the contents of each cell in the grid.
//
//    Here we provide the string contents for each cell in the perturbation table:
//       1) Cell in the fixed row 0 ==> Label of attribute displayed in that column. The labels for attributes shared
//          by all perturbation types do not change. For the rest, the label displayed depends on the type of
//          perturbation in the current focus row.
//       2) Cell in row N>0, col 0  ==> Name of Nth perturbation object displayed in table.
//       3) Cell in row N>0, cols 1..M ==> Value of the Mth common parameter for perturbation object N.
//       4) Cell in row N>0, cols M+1..M+P ==> Value of the Pth type-specific parameter for perturbation object N.
//    Observe that the perturbation table is organized to display perturbation definitions IAW a format prescribed by 
//    the perturbation data class CCxPert.  Each perturbation definition consists of M parameters common to all 
//    supported perturbation types, followed by P type-specific parameters.  CCxPert provides methods for accessing 
//    all perturbation parameters by a zero-based index in the range [0..M+P-1].
//
//    The number and identities of the type-specific parameters, of course, will vary with the perturbation type.
//    The column labels in the first row reflect the parameters for the perturbation in the current focus row (though
//    some column labels never change bc they represent parameters common to all perturbation types). Whenever the
//    focus cell changes (GVN_SELCHANGED notification), the header row is redrawn so that the columns are updated.
//
//    NOTE:  Callback functions must be implemented as static.  Since it is a static class method, it does not have 
//    access to instance fields and methods and it does not get the implied argument THIS.  To circumvent this problem, 
//    we take advantage of the generic LPARAM argument, using it to pass a reference to THIS view!!  This is done when 
//    we register the callback fcn with the grid in OnInitialUpdate().
//
//    ARGS:       pDispInfo   -- [in/out] ptr to a struct we need to fill with the appropriate display info. 
//                lParam      -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxPertForm::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam ) 
{
   CCxPertForm* pThisView = (CCxPertForm*) lParam;                         // to access non-static data!
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );                  // the cell to be displayed

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 

   CCxPert* pPert = pThisView->GetPertObjByRow( c.row );                   // retrieve relevant pert obj, if any
   int iParam = c.col - 1;                                                 // zero-based index identifying parameter

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                     // not using label tips
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP; 
   else if(c.row == 0)                                                     // header row: column labels reflect the
   {                                                                       // set of parameters for the pertubation 
      // header row: column labels reflect the set of parameters for the pertubation type currently occupying the
      // focus row. For columns corresponding to parameter shared by all perturbations, the label does not change
      CCellID focus = pGrid->GetFocusCell();
      CCxPert* pFocusPert = NULL;
      if(focus.IsValid() && focus.row > 0) pFocusPert = pThisView->GetPertObjByRow(focus.row);
 
      pDispInfo->item.strText = _T("");
      if(iParam >= 0 && iParam < CCxPert::NumberOfCommonParameters())
         pDispInfo->item.strText = CCxPert::GetCommonParamLabel( iParam );
      else if(pFocusPert != NULL && iParam < pFocusPert->NumberOfParameters())
         pFocusPert->GetParameterLabel(iParam, pDispInfo->item.strText);
   }
   else if( pPert == NULL )                                                // no pert obj found -- this might happen 
      return( FALSE );                                                     //    when opening a different document
   else if( c.col == 0 )                                                   // retrieve pert obj's name string
      pDispInfo->item.strText = pPert->Name();
   else                                                                    // retrieve current value of a pert param 
   {                                                                       // as a text string; if index corresponds to 
      pPert->GetParameter( iParam, pDispInfo->item.strText );              // a nonexistent param, set bkg color to 
      if( iParam >= pPert->NumberOfParameters() )                          // that of a fixed cell to emphasize that
      {                                                                    // there's nothing to edit here
         CGridCellBase* pFixed = pGrid->GetDefaultCell( TRUE, TRUE );
         pDispInfo->item.crBkClr = pFixed->GetBackClr();
      }
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                        // show title tip only if text does not fit 
   return( TRUE );
}


//=== GridEditCB ====================================================================================================== 
//
//    Callback invoked to initiate inplace editing of a cell on the perturbation table, or to increment/decrement the 
//    contents of a cell in response to a left or right mouse click.  Below is a summary of the possible operations 
//    that this callback permits:
//
//       1) Cell in the fixed row 0 ==> These are read-only column header labels.  Cannot be edited.
//       2) Cell in row N>0, col 0  ==> Name of Nth perturbation object displayed in table.  Cannot be edited here.
//       3) Cell in row N>0, cols 1..Q ==> Value of parameter Q-1 for perturbation object N.  If the parameter is 
//          multiple-choice, a left(right) mouse click will increment(decrement) the current choice.  Otherwise, it is 
//          a numeric parameter and is not affected by a mouse click.  If the grid is about to initiate an inplace edit 
//          operation, we provide it with the appropriate information:  an array of choices for a multi-choice param, 
//          or the numeric format constraints for a numeric param.  All of this information is provided by the CCxPert 
//          data object itself.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info. 
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxPertForm::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxPertForm* pThisView = (CCxPertForm*)lParam;                          // to access non-static data!
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c = pEI->cell;                                                  // the cell to be edited

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 


   CCxPert* pPert = pThisView->GetPertObjByRow( c.row );                   // retrieve relevant pert obj, if any
   int iParam = c.col - 1;                                                 // zero-based index identifying parameter

   if( pThisView->IsGridCellReadOnly( c ) )                                // cannot edit "readOnly" cells!
   { 
      pEI->iClick = 0; 
      pEI->iType = LG_READONLY;
   }
   else if( pEI->iClick != 0 )                                             // edit by mouse click?:
   {
      ASSERT( pPert );
      if( pPert->IsParameterMultiChoice( iParam ) )                        // if param is multi-choice, a click will 
      {                                                                    // increment or decrement the current choice
         int iNew = pPert->GetParameterAsInt( iParam );
         iNew += (pEI->iClick > 0) ? 1 : -1;
         if( pPert->SetParameter( iParam, iNew ) )                         //    if change affects value/appearance of 
            pGrid->RedrawRow( c.row );                                     //    another param, redraw perturbation row 

         pThisView->InformModify( c.row );                                 //    inform doc/vu framework of change
      }
      else                                                                 // otherwise, the click has no effect
         pEI->iClick = 0;
   }
   else                                                                    // initiate inplace edit of pert parameter:
   {
      ASSERT( pPert );
      BOOL bIsChoice = FALSE;                                              //    get parameter type/format info
      pPert->GetParameterFormat( iParam, bIsChoice, pEI->strArChoices,
                                 pEI->numFmt );
      pEI->iType = bIsChoice ? LG_MULTICHOICE : LG_NUMSTR;
      if( bIsChoice ) pEI->iCurrent = pPert->GetParameterAsInt( iParam );
      else pEI->dCurrent = pPert->GetParameter( iParam );
   }

   return( TRUE );
}


//=== GridEndEditCB =================================================================================================== 
//
//    Callback invoked upon termination of inplace editing of a cell in the perturbation table.
//
//    Here we update the corresponding perturbation object IAW the change made during the inplace operation that was 
//    configured in GridEditCB().  Based on the value of the exit key character that terminated the operation, we may 
//    direct the grid to continue inplace editing at another, nearby cell.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxPertForm::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                      // inplace operation was cancelled

   CCxPertForm* pThisView = (CCxPertForm*)lParam;                          // to access non-static data!
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c = pEEI->cell;                                                 // the cell that was edited

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 

   CCxPert* pPert = pThisView->GetPertObjByRow( c.row );                   // retrieve relevant pert obj, if any
   int iParam = c.col - 1;                                                 // zero-based index identifying parameter

   if( pThisView->IsGridCellReadOnly( c ) )                                // if the cell is read only, there's nothing 
   {                                                                       // to edit!!
      ASSERT( FALSE );                                                     //    this should NEVER happen!
      pEEI->nExitChar = VK_ESCAPE;                                         //    prevent continued inplace editing
      pEEI->bNoRedraw = TRUE;                                              //    no need to redraw since no change made 
      return( TRUE );
   }
   ASSERT( pPert );

   if( pEEI->bIsChanged )                                                  // if the user made a change...
   {
      BOOL bSideEffect = FALSE; 
      if( pPert->IsParameterMultiChoice( iParam ) )                        //    update the multichoice or numeric
         bSideEffect = pPert->SetParameter( iParam, int(pEEI->dwNew) );    //    parameter value
      else
         bSideEffect = pPert->SetParameter( iParam, pEEI->dNew );

      if( bSideEffect )                                                    //    if change affected other parms, redraw 
      {                                                                    //    entire row now -- in which case grid 
         pGrid->RedrawRow( c.row );                                        //    need not redraw the param cell!
         pEEI->bNoRedraw = TRUE;
      }

      // if pert type changed, we redraw header row so column labels reflect the params for the selected type
      if(iParam == 0) pGrid->RedrawRow(0);

      pThisView->InformModify( c.row );                                    //    inform doc/view framework
   }

   do                                                                      // choose next cell to edit if inplace op
   {                                                                       // was terminated by certain navig. keys, 
                                                                           // skipping over read-only cells...
      switch( pEEI->nExitChar ) 
      { 
         case VK_TAB :                                                     // TAB, right arrow: move to next col, 
         case VK_RIGHT :                                                   // wrapping to first col of next row if nec. 
            ++c.col;                                                       // if on last row, go to top row.
            if( c.col >= pGrid->GetColumnCount() ) 
            { 
               c.col = 1;
               if( ++c.row >= pGrid->GetRowCount() ) c.row = 0;
            }
            break;
         case VK_LEFT :                                                    // left arrow: move to prev col, wrapping to 
            --c.col;                                                       // last col of prev row if nec.  if we're on 
            if( c.col < 0 )                                                // first row, go to bottom row.
            {
               c.col = pGrid->GetColumnCount() - 1;
               if( --c.row < 0 ) c.row = pGrid->GetRowCount() - 1;
            }
            break;
         case VK_DOWN :                                                    // down arrow: move down one row, wrapping 
            ++c.row;                                                       // to next col of top row if nec.  if we're  
            if( c.row >= pGrid->GetRowCount() )                            // on last col, go to first col.
            {
               c.row = 0;
               if( ++c.col >= pGrid->GetColumnCount() ) c.col = 0;
            }
            break;
         case VK_UP :                                                      // up arrow: move up one row, wrapping to 
            --c.row;                                                       // prev col of bottom row if nec. if we're 
            if( c.row < 0 )                                                // on first col, go to last col.
            {
               c.row = pGrid->GetRowCount() - 1;
               if( --c.col < 0 ) c.col = pGrid->GetColumnCount() - 1;
            }
            break;
         default :                                                         // the above keys are the only navigation 
            pEEI->nExitChar = 0;                                           // keys we allow!
            break;
      }

   } while( pEEI->nExitChar != 0 && pThisView->IsGridCellReadOnly( c ) );

   if( c == pEEI->cell ) pEEI->nExitChar = 0;                              // prevent continuation on the same cell!
   else if( pEEI->nExitChar != 0 ) pEEI->cellNext = c;

   return( TRUE );
}


