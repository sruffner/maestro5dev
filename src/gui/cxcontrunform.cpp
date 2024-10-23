//===================================================================================================================== 
//
// cxcontrunform.cpp : Implementation of class CCxContForm.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// 
// CCxContRunForm is the view class through which the user modifies the defining parameters of the Maestro stimlus run, 
// CCxContRun. A "run" is the fundamental experimental protocol in continuous mode, just as a trial is for trials 
// mode. Unlike a trial, a run will continue to play over and over again indefinitely, or until a specified # of "duty 
// cycles" have occurred. The run is defined by a duty period (and a few other general parameters) and a set of 
// "stimulus channels" that define the motion trajectory of several different visual or non-visual targets.  See 
// CCxContRun for details.
//
// ==> Construction of form; layout of controls; use of CLiteGrid.
// The layout of the CCxContRunForm view is defined in the dialog template resource IDD_RUNFORM.  Since we supply this 
// ID in the default constructor, the MFC framework handles the details of loading the template and creating the view. 
// Use the Dev Studio Resource Editor to review the layout of IDD_RUNFORM.  We use a few "Windows common controls" 
// to represent the general parameters like duty period, auto-stop cycle count, and marker pulse channel.  However, the 
// form is dominated by two instances of a custom control with WNDCLASS name "MFCGridCtrl".  This is Chris Maunder's 
// "MFC Grid Control" (see CREDITS), CGridCtrl, a spreadsheet-like control that we have chosen to display the list of 
// currently defined stimulus channels. The grid control offers a compact way of laying out a large number of similar 
// parameters. We actually employ a derivative of CGridCtrl, CLiteGrid, which is designed to work only in "virtual 
// mode" and which provides a number of built-in "inplace editors" (text string, formatted numeric text, combo box, and
// tree item selector) which are not available in CGridCtrl. The layout and usage of our CLiteGrid are described 
// later. Because it is a custom control, we must dynamically subclass the grid's HWND to a CLiteGrid object before we 
// use it. See CCxContRunForm::OnInitialUpdate().
//
// ==> Layout of the stimulus channel definitions in a CLiteGrid.
// Generally speaking, a "stimulus channel" within a stimulus run is simply a set of parameters that defines a particular 
// motion trajectory for a stimulus. CCxContRun was designed to allow the view class to "discover" the nature of a 
// stimulus channel's parameters. The idea here is to minimize the dependence of the view's implementation on the 
// underlying data (aka the Java design pattern known as model-view-controller) -- thereby reducing the work we must do 
// to update the view class when the data class changes. Methods are provided to get/set each parameter value, 
// determine the parameter's format (numeric or multiple-choice), and to retrieve string forms of the parameter's label 
// and current value. With this information, our view can present the parameters in any manner we choose.
//
// Our choice is to use a CLiteGrid control, dedicating one row of the grid to each stimulus channel. A channel # in 
// the first column is followed by M+N columns. The first M columns correspond to parameters that are common to all 
// stimulus channel types. The labels for these parameters appear in the row header at the top of the grid. The 
// remaining N columns represent the N unique parameters for a stimulus channel. The # of unique parameters and their 
// identities will vary with stimulus channel type and motion mode (these are common parameters). We query CCxContRun 
// to obtain the maximum # of unique parameters that will ever be used and set N to this maximum. A picture will help 
// clarify this design:
//
//    Ch#   | Label1 | Label2 | ... | LabelM | (no labels in column header for the unique parameters!)
//    1     |  val1  |  val2  |     |  valM  | valA | valB | 
//    2     |  val1  |  val2  | ... |  valM  | valA | valB | valC | valD | valE | valF |
//    3     |  val1  |  val2  | ... |  valM  | valA | valB | valC | 
// 
// CLiteGrid is designed to use the underlying MFC Grid Control in "virtual mode", a lightweight implementation that 
// avoids the memory overhead of associating a CGridCell-derived object with each and every cell in the grid. In this 
// scheme, the grid ctrl uses a registered callback function to obtain info needed to repaint any grid cell. This 
// callback almost exclusively governs the appearance of the grid. See CCxContRunForm::StimGridDispCB(). 
//
// CLiteGrid's "label tip" feature solves a fundamental problem associated with the specialized grid layout illustrated 
// above.  We need to include labels for the "unique" parameters, but we cannot put them in the column header row b/c 
// the labels will vary with stimulus channel type.  In an earlier version, each unique parameter was represented by 
// two columns, one for the label and one for the parameter itself -- which wastes a lot of space on static labels.
// Now, we take advantage of the "label tip" feature to display each unique parameter's label in a tooltip that appears 
// briefly after the parameter cell obtains the focus.  The display callback StimGridDispCB() provides the label text.
//
// The grid control itself will display and manage its own scroll bars if its current contents will not fit inside its 
// assigned window rectangle.  While this feature is important, CNTRLX users often will want to make the stimulus list 
// as large as possible for convenient viewing.  To this end, whenever the user resizes the form beyond its initial 
// dimensions (those defined in the resource template), we will extend the right and bottom sides of this grid as well 
// -- the grid acts as if its top and left sides are fixed, while the right and bottom sides are attached to the right 
// and bottom sides of this form view.  Thus, by making the form larger, the user will make the stimulus grid larger.
//
// ==> Operations on the stimulus channel grid; CLiteGrid's use of "callback" functions for editing cell data inplace. 
// Users must be able to edit individual stimulus parameters and perform a number of operations on stimulus channels.  
// CLiteGrid provides a number of "popup" controls for modifying the contents of individual cells "in place": an edit 
// control for short text strings, a specialized edit control for integer or floating-point numbers, a combo box for 
// multiple-choice parameters, and a tree control for selecting a parameter's value from a heirarchical tree structure. 
// CLiteGrid also supports modifying a cell's contents in response to left or right mouse clicks on the cell.  To take 
// advantage of these facilities and tailor them to the kinds of data stored in the stimulus grid, we must install two 
// additional callback functions
//    StimGridEditCB() -- Invoked when an inplace edit operation is about to take place (or when a mouse click occurs 
//       in the cell).  Retrieves cell edit info, or modifies cell contents IAW mouse click.
//    StimGridEndEditCB() -- Called to update stimulus channel grid IAW results of an inplace operation just completed. 
// See CLiteGrid for more information on how these callback methods are invoked and used.
//
// Here's a complete description of the operations a user can perform:
//    -- Double-click on any parameter value to invoke an appropriate inplace control to edit that value:  a numeric 
//       edit control for numeric parameters, or a combo box for "multiple-choice" parameters. After editing the 
//       parameter, the user can confirm the change by hitting RETURN or merely clicking the mouse outside the inplace 
//       control's rectangle.  The control is then hidden, and the new parameter value (auto-corrected if necessary) 
//       appears within the underlying cell.  The user can also hit the ESCAPE key to cancel the inplace editing 
//       operation, or end the operation on the current cell and initiate a new inplace edit on an adjacent cell by 
//       hitting one of the arrow keys or the TAB key in combination with the CTRL key.  This reduces reliance on the 
//       mouse for navigating around the grid. 
//    -- As a short-cut, a left (right) mouse click on any multiple-choice parameter will change its value to the next 
//       (previous) choice; clicking on a numeric parameter has no effect.
//    -- Right-click on any cell in the first column ("Ch#") to invoke a context menu which allows the user to select 
//       among the following operations:  "Remove all" channels from the grid; "Copy", "Cut", or "Delete" the selected 
//       channel (if any); "Paste" the last copied stimulus channel to this location in the list, "Insert" a new 
//       default channel at this location in list, or "Append" it at list's end. 
//    -- As a short-cut for "Append", the user can double-click on the top left cell of the grid.
//
// ==> The "XYseq" stimulus type was DEPRECATED as of Maestro v5.0.
// The XYScope platform has not been supported since Maestro 4.0, and was removed entirely in v5.0. As a result, the
// "XYseq" stimulus type no longer exists, so the editable list of XYScope targets applicable to that stimulus was
// removed from CCxContRunForm.
//
// ==> Changes to run definition are applied immediately; DDX not used.
// Any change to a run parameter is handled as soon as it occurs, rather than waiting for the user to press an "Apply" 
// button. If the change is unacceptable, it is automatically corrected in some way by the CCxContRun data object,
// and the controls are updated to reflect the corrections made. Since we must catch parameter changes as they occur, 
// we have elected not to use MFC's Dialog Data Exchange techniques in our implementation  The routines OnChange(), 
// StuffHdrControls(), and the various grid callback functions handle the exchange of data between the loaded run 
// object and the child controls on the CCxContRunForm.
//
// ==> Interactions with CCxContRun, CCxDoc, other CNTRLX views.
// CCxContRunForm must query CCxDoc whenever it must obtain a ptr to the run definition object, CCxContRun, for a given 
// run.  CCxContRunForm then queries the CCxContRun object directly to access and modify the run's defn.  Whenever it 
// does so, CCxContRunForm must set the document's modified flag via CDocument::SetModifiedFlag(), and inform all other 
// attached CNTRLX views by broadcasting an appropriate CNTRLX-specific CCxViewHint via CDocument::UpdateAllViews(). 
// Likewise, user actions in other views can affect the current contents of CCxContRunForm.  For example, if the user 
// selects a different run for display, the just-selected run object must be loaded into this form.  If the user 
// deletes the run currently being displayed, CCxContRunForm must reset its contents.  In each case, the responsible 
// view broadcasts a hint via UpdateAllViews(), and that signal is processed by the all-important CView override, 
// OnUpdate().  See CCxContRunForm::OnUpdate() for details.
//
// Each of the CNTRLX "object definition forms" has been designed for use in a "tabbed window" -- in particular, the 
// TTabWnd class that is part of the "Visual Framework" library (see CREDITS).  The CNTRLX main frame window (see 
// CCxMainFrame) installs each form in one of the tab panes of a TTabWnd.  The caption of the tab pane reflects the 
// "full name" of the CNTRLX data object currently loaded on the form.  This tab window is NOT a view, which presents a 
// technical problem:  how do we update the tab window when the name of a loaded object changes, or when the user 
// selects a different type of object for viewing (which requires bringing a different tab pane to the front).  Our 
// solution:  All of the object definition forms (CCxTrialForm, CCxTargForm, etc.) are derived from TVTabPane, a simple 
// CFormView-derivative that provides methods for telling the parent TTabWnd to update a tab caption or bring a 
// particular tab to the front of the tab window.  TVTabPane is a supplement to the Visual Framework (see VISUALFX.*).
//
//
// CREDITS:
// (1) Article by Chris Maunder [08/30/2000, www.codeproject.com/miscctrl/gridctrl.asp] -- The MFC Grid Control, v2.21. 
// The CLiteGrid controls that appear on CCxContRunForm are derived from Maunder's CGridCtrl.
//
// (2) The "Visual Framework" library, by Zoran M. Todorovic [09/30/2000, www.codeproject.com/dialog/visualfx.asp] --
// We use this library to implement the view framework that appears in the CNTRLX frame window's (CCxMainFrame) client 
// area.  This framework makes more complex GUI layouts -- that would have been a bear to implement w/core MFC -- easy 
// to build.  CCxContRunForm is derived from TVTabPane, a class that I added to the Visual Framework.
//
//
// REVISION HISTORY:
// 22may2002-- Begun work on implementation, using CCxTrialForm as a template...
// 27nov2002-- Begun work to implement the two grids using CLiteGrid -- which has the inplace editor tools built-in -- 
//             rather than CGridCtrl.
// 02dec2002-- Finished up mod to use CLiteGrid instead of CGridCtrl for the two grid controls on the form.
// 15mar2004-- UpdateTitle() replaced by overriding TVTabPane::UpdateCaption() instead.  Also, we no longer display the 
//             full "pathname" of an object b/c it makes the tab text too long...
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
// 16oct2024-- Removed the XYseq target grid entirely, since the XYseq stimlus type is no longer supported a/o Maestro
//             v4.0 and removed entirely in v5.0.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX 

#include "cxviewhint.h"                      // the "hint" class used by all CNTRLX views 
#include "cxcontrunform.h" 


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxContRunForm
//
//===================================================================================================================== 
//===================================================================================================================== 

IMPLEMENT_DYNCREATE( CCxContRunForm, TVTabPane )


BEGIN_MESSAGE_MAP( CCxContRunForm, TVTabPane )
   ON_WM_SIZE()
   ON_COMMAND_RANGE( ID_RUN_STIM_CLEAR, ID_RUN_STIM_APPEND, OnGridOps )
   ON_UPDATE_COMMAND_UI_RANGE( ID_RUN_STIM_CLEAR, ID_RUN_STIM_APPEND, OnUpdGridOps )
   ON_NOTIFY( NM_DBLCLK, IDC_RF_STIMULI, OnNMDblClk )
   ON_NOTIFY( NM_RCLICK, IDC_RF_STIMULI, OnNMRClick )
   ON_CONTROL( CBN_SELCHANGE, IDC_RF_DUTYPULSE, OnDutyPulse )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_RF_DUTYPER, IDC_RF_VOFFSET, OnChange ) 
   ON_UPDATE_COMMAND_UI_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnUpdateEditCommand )
   ON_COMMAND_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnEditCommand )
END_MESSAGE_MAP()
   


//===================================================================================================================== 
// PRIVATE CONSTANTS & GLOBALS
//===================================================================================================================== 



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxContRunForm [constructor] ==================================================================================== 
//
//    Construct the form view. 
//
//    Almost all the work is handled by the framework, which loads the form's layout from a CNTRLX resource whose
//    identifier is stored in protected member var IDD.  However, we do need to init certain variables that track the 
//    form's state.  Also, most of the controls on the form must be subclassed to CWnd-derived members -- but that's 
//    delayed until OnInitialUpdate().
//
CCxContRunForm::CCxContRunForm() : TVTabPane( CCxContRunForm::IDD ) 
{
   m_bOneTimeInitsDone = FALSE;
   m_bLoading = FALSE;
   m_wKey = CX_NULLOBJ_KEY;            // initially, no run object is loaded on form 
   m_pRun = NULL;

   m_pPasteStim = NULL;                // no stimulus has been copied yet for pasting 

   m_contextCell.row = 
   m_contextCell.col = -1;

   m_minSTCSize = CSize(0,0);          // min grid size is determined in OnInitialUpdate()
}


//=== ~CCxContRunForm [destructor] ==================================================================================== 
//
//    When this view is destroyed, we must destroy anything we've dynamically created
//
CCxContRunForm::~CCxContRunForm() 
{
   if ( m_pPasteStim )
   {
      delete m_pPasteStim;
      m_pPasteStim = NULL;
   }
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnChange ======================================================================================================== 
//
//    Update a "header" parameter (one not in the stimulus channel grid) in the loaded run IAW a detected change in the
//    associated control. We handle several different notifications here:
//       1) CBN_CLOSEUP (passed on by OnDutyPulse()) ==> User selected an item from the dropdown list for the "Duty 
//          Marker Pulse" combo box [IDC_RF_DUTYPULSE]. We update the run's duty marker pulse accordingly.
//       2) EN_KILLFOCUS ==> Keyboard focus has left one of the numeric edit ctrls [IDC_RF_DUTYPER...IDC_RF_VOFFSET], 
//          indicating contents may have changed. We update corresponding parameter. Note that this will be sent when 
//          a control loses the focus b/c it is about to be disabled (in that case, the contents have not changed!).
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification. 
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnChange( UINT id )
{
   if( m_pRun == NULL ) return;                                // if no run loaded, ignore

   BOOL bChanged = FALSE;                                      // TRUE if a parameter was actually changed
   CString str;
   int iOld, iNew, iActual;
   double dOld, dNew, dActual;

   switch( id )                                                // update the associated parameter...
   {
      case IDC_RF_DUTYPULSE :                                  //    duty pulse, a multi-choice param 
         iOld = m_pRun->GetDutyPulse();
         iNew = m_cbDutyPulse.GetCurSel();
         if( iNew != m_pRun->GetDutyPulse() )
         {
            m_pRun->SetDutyPulse( iNew );
            bChanged = TRUE;
         }
         break;
      case IDC_RF_DUTYPER :                                    //    duty period, an integer-valued param
         iOld = m_pRun->GetDutyPeriod();
         iNew = m_edDutyPeriod.AsInteger();
         m_pRun->SetDutyPeriod( iNew );
         iActual = m_pRun->GetDutyPeriod();
         bChanged = BOOL(iOld != iActual);
         if( iNew != iActual )                                 //    (user value auto-corrected; refresh display)
            m_edDutyPeriod.SetWindowText( iActual );
         break;
      case IDC_RF_AUTOSTOP :                                   //    #cycles before autostop, an integer-valued param
         iOld = m_pRun->GetAutoStop();
         iNew = m_edNAutoStop.AsInteger();
         m_pRun->SetAutoStop( iNew );
         iActual = m_pRun->GetAutoStop();
         bChanged = BOOL(iOld != iActual);
         if( iNew != iActual ) 
            m_edNAutoStop.SetWindowText( iActual );
         break;
      case IDC_RF_HOFFSET :                                    //    horiz pos offset, a float-valued param
         dOld = m_pRun->GetHOffset();
         dNew = m_edHOffset.AsDouble();
         m_pRun->SetHOffset( dNew );
         dActual = m_pRun->GetHOffset();
         bChanged = BOOL(dOld != dActual);
         if( dNew != dActual )
            m_edHOffset.SetWindowText( dActual );
         break;
      case IDC_RF_VOFFSET :                                    //    verti pos offset, a float-valued param
         dOld = m_pRun->GetVOffset();
         dNew = m_edVOffset.AsDouble();
         m_pRun->SetVOffset( dNew );
         dActual = m_pRun->GetVOffset();
         bChanged = BOOL(dOld != dActual);
         if( dNew != dActual )
            m_edVOffset.SetWindowText( dActual );
         break;

      default :                                                //    we should NEVER get here!
         TRACE0( "Bad ID in CCxContRunForm::OnChange!\n" );
         ASSERT( FALSE );
         return;
   }

   if( bChanged ) InformModify();                              // inform doc/view framework only if a change was made! 
}


//=== OnUpdateEditCommand ============================================================================================= 
// 
//    ON_UPDATE_COMMAND_UI_RANGE handler for the predefined ID_EDIT_*** commands:  Update enable state of selected 
//    items in the app's Edit menu depending on the current state of the clipboard and the edit control that currently 
//    has the focus on this form.  An edit control must currently have the focus for any of the items to be enabled.
//
//    NOTE that only some of the ID_EDIT_*** commands are actually implemented. 
//
//    [TECH NOTE: Ptr returned by CWnd::GetFocus() may be temporary; if so, it is released during the thread's idle 
//    time.  We don't have to worry about that here, since we don't save it...but what about multithreading effects??] 
// 
//    ARGS:       pCmdUI -- [in] represents UI item. 
//
//    RETURNS:    NONE
//
void CCxContRunForm::OnUpdateEditCommand( CCmdUI* pCmdUI )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a 
   if ( (pFocusWnd == NULL) ||                                 // CNumEdit obj, then disable all Edit cmds -- no other 
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CNumEdit) )) )    // controls on this form support editing.
   {
      pCmdUI->Enable( FALSE );
      return;
   }

   CNumEdit *pEditC = (CNumEdit *) pFocusWnd;                  // obj w/focus is a CNumEdit, so we can safely cast ptr 

   BOOL bEnable = FALSE;                                       // enable state of edit command depends on current 
   int iStart = 0;                                             // state of the CNumEdit ctrl w/ the input focus: 
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
//    [TECH NOTE: Ptr returned by CWnd::GetFocus() may be temporary; if so, it is released during the thread's idle 
//    time.  We don't have to worry about that here, since we don't save it...but what about multithreading effects??] 
// 
//    ARGS:       nID -- [in] edit command ID. 
//
//    RETURNS:    NONE
//
void CCxContRunForm::OnEditCommand( UINT nID )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a 
   if ( (pFocusWnd == NULL) ||                                 // CNumEdit obj, do nothing -- all alterable edit ctrls  
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CNumEdit) )) )    // on this form are attached to CNumEdit objects! 
      return;

   CNumEdit *pEditC = (CNumEdit *) pFocusWnd;                  // obj w/focus is a CNumEdit, so we can safely cast ptr 

   switch( nID )                                               // process the operation by calling the appropriate 
   {                                                           // CNumEdit method...
      case ID_EDIT_CUT :   pEditC->Cut();    break;
      case ID_EDIT_COPY :  pEditC->Copy();   break;
      case ID_EDIT_PASTE : pEditC->Paste();  break;
      case ID_EDIT_UNDO :  pEditC->Undo();   break;
      default :                              break;
   }
}


//=== OnSize [base override] ========================================================================================== 
//
//    Response to WM_SIZE message.
//
//    To maximize how much of the stimulus channel grid can be seen at one time, we permit the grid's right or bottom 
//    side to "stick" to the form's corresponding side whenever doing so would make the grid *larger* than its minimum 
//    size.  Top left corner of the grid rect does not move, keeping it fixed relative to other controls on form.
//
//    ARGS:       nType -- [in] type of resizing requested. 
//                cx,cy -- [in] new width and height of client area. 
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnSize( UINT nType, int cx, int cy ) 
{
   TVTabPane::OnSize( nType, cx, cy );                      // base class stuff first 

   if( m_stimChanGrid.GetSafeHwnd() == NULL ) return;       // there's no grid ctrl to resize!

   CRect rGridCurr;                                         // get current grid rect in form coords 
   m_stimChanGrid.GetWindowRect( rGridCurr );               // (accounts for possible scrolling)
   ScreenToClient( rGridCurr );

   CSize szClient, szBars;                                  // get true client size & scroll bar sizes
   GetTrueClientSize( szClient, szBars );

   DWORD dwStyle = GetStyle();                              // are scroll bars there or not?
   BOOL bHasH = BOOL((dwStyle & WS_HSCROLL) != 0);
   BOOL bHasV = BOOL((dwStyle & WS_VSCROLL) != 0);

   CRect rGrid = rGridCurr;                                 // adjust grid rect IAW sizing event:
   int iSide = szClient.cx - 7;                             //    stick right side of grid to right side of form, 
   if( bHasV ) iSide -= szBars.cx;                          //    accounting for possible presence of vert scroll bar, 
   if( (iSide - rGrid.left) >= m_minSTCSize.cx )            //    unless minimum grid width would be violated
      rGrid.right = iSide;
   else
      rGrid.right = rGrid.left + m_minSTCSize.cx;           //    otherwise: return grid width to minimum

   iSide = szClient.cy - 7;                                 //    similarly w/ bottom of grid, acct for H scroll bar
   if( bHasH ) iSide -= szBars.cy;
   if( (iSide - rGrid.top) >= m_minSTCSize.cy ) 
      rGrid.bottom = iSide;
   else 
      rGrid.bottom = rGrid.top + m_minSTCSize.cy;
   
   if( rGrid != rGridCurr )                                 // if grid rect adjusted, move it now.
      m_stimChanGrid.MoveWindow( rGrid );
}


//=== OnNMDblClk ====================================================================================================== 
//
//    Response to the NM_DBLCLK notification from the stimulus channel grid control. A double-click on the top left 
//    cell appends a new stimulus channel definition to the bottom of the grid.  
//
//    ARGS:       pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_DBLCLK. 
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnNMDblClk(NMHDR* pNMHDR, LRESULT* pResult )
{
   if(m_pRun == NULL)                                                   // ignore if there's no run loaded on form
      return; 

   NM_GRIDVIEW* pNMGV = (NM_GRIDVIEW*) pNMHDR;                          // the grid cell that was dbl-clicked
   CCellID clickCell( pNMGV->iRow, pNMGV->iColumn );

   *pResult = TRUE;                                                     // return value is irrelevant for NM_DBLCLK 

   // NM_DBLCLK on cell 0,0 is a shortcut for appending a row to stimulus grid
   if( clickCell.row == 0 && clickCell.col == 0 )
   {
      m_contextCell.row = m_contextCell.col = 0;
      OnGridOps(ID_RUN_STIM_APPEND);
   }
}


//=== OnNMRClick ====================================================================================================== 
//
//    Response to the NM_RCLICK notification from the stimulus channel grid. A right-click on the first column of any 
//    stimulus channel row (the "channel #") invokes context menu (submenu 2 of the IDR_CXPOPUPS menu resource) from 
//    which user can select among a number of operations.
//
//    Context menu operations often apply to the particular stimulus channel clicked. Hence, we must save the identity
//    of the context cell from which the operation was initiated. See also the popup menu handling routines 
//    OnUpdGridOps() and OnGridOps().
//
//    ARGS:       pNMHDR   -- [in] handle to a NM_GRIDVIEW struct cast as a generic NMHDR*
//                pResult  -- [out] return code.  ignored for NM_RCLICK. 
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
   if( m_pRun == NULL)                                                  // ignore if there's no run loaded on form
      return; 

   NM_GRIDVIEW* pNMGV = (NM_GRIDVIEW*) pNMHDR;                          // the grid cell that was clicked 
   m_contextCell.row = pNMGV->iRow;
   m_contextCell.col = pNMGV->iColumn;

   *pResult = TRUE;                                                     // return value is irrelevant for NM_RCLICK 

   m_stimChanGrid.SetFocus();                                           // rt-click does not give grid ctrl the focus

   if((m_contextCell.row >= 0) && (m_contextCell.col == 0))             // invoke context menu if right click on first 
   {                                                                    // column 
      CMenu menu;
      if(menu.LoadMenu(IDR_CXPOPUPS))                                   //    load CNTRLX popup menus from resource
      {
         CPoint point;                                                  //    popup's ULC will align with mouse pos
         ::GetCursorPos( &point );

         CMenu* pPopup = menu.GetSubMenu(2);                            //    run the popup 
         pPopup->TrackPopupMenu( 
               TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
               point.x, point.y, ::AfxGetMainWnd() );
      }
   }
}


//=== OnGridOps ======================================================================================================= 
//
//    Menu command handler for the context menu that pops up when the user right-clicks on appropriate cells in the 
//    stimulus channelgrid control (see OnNMRClick()).
//
//    Supported commands for the stimulus channel grid:
//       ID_RUN_STIM_CLEAR ==> Clear all existing stimulus channels from the loaded run.
//       ID_RUN_STIM_COPY ==> Make an internal copy of stimulus channel selected by current "context" cell.
//       ID_RUN_STIM_CUT ==> Make a copy of selected stimulus channel, then delete it.
//       ID_RUN_STIM_DEL ==> Delete selected stimulus channel.
//       ID_RUN_STIM_PASTE ==> Paste the last copied stimulus channel to the selected pos in the stimulus grid.
//       ID_RUN_STIM_INS ==> Insert a new stimulus channel at the selected pos in the stimulus grid.
//       ID_RUN_STIM_APPEND ==> Append a new stimulus channel at the bottom of the stimulus list.
//
//    Most operations apply to a specific stimulus channel in the grid -- as selected by the current context cell (the 
//    cell that was right-clicked when the context menu was invoked). Operations are enabled only when they "make 
//    sense". For example, insertion operations are disabled when the max channel capacity is reached. See 
//    OnUpdGripOps() for details.
//
//    Inserting or deleting a stimulus is merely a matter of incrementing or decrementing the appropriate grid's row 
//    count and refreshing the grid. This is because the run object keeps track of the order of stimuli in the list,
//    and the grid callback functions query that object for all grid contents!
//
//    ARGS:       cmdID -- [in] the command ID
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnGridOps( UINT cmdID )
{
   ASSERT( m_pRun != NULL );                                   // there must be a run loaded at this time!

   int iPos = m_contextCell.row - 1;                           // pos in run object's stimulus channel list
   BOOL bUpdate = FALSE;                                       // if TRUE, refresh grid and inform other views.

   switch( cmdID )                                             // BEGIN:  process the command...
   {
      case ID_RUN_STIM_CLEAR : 
         if( m_pRun->GetStimulusCount() > 0 )
         {
            m_pRun->ClearStimuli();
            bUpdate = TRUE;
         }
         break;

      case ID_RUN_STIM_COPY :
      case ID_RUN_STIM_CUT :
         ASSERT( m_pRun->IsValidStimulus( iPos ) );
         if( m_pPasteStim != NULL )                            //    !! Be sure to free the last stimulus ch copied
         {
            delete m_pPasteStim;
            m_pPasteStim = NULL;
         }
         if( cmdID == ID_RUN_STIM_COPY )
            m_pPasteStim = m_pRun->CopyStimulus( iPos );
         else
         {
            m_pPasteStim = m_pRun->CutStimulus( iPos );
            if( m_pPasteStim != NULL ) bUpdate = TRUE; 
         }
         break;

      case ID_RUN_STIM_DEL :
         bUpdate = m_pRun->RemoveStimulus( iPos );
         break;

      case ID_RUN_STIM_PASTE :
         if( (m_pPasteStim != NULL) && (m_pRun->PasteStimulus( iPos, m_pPasteStim ) >= 0) )
            bUpdate = TRUE;
         break;

      case ID_RUN_STIM_INS :
      case ID_RUN_STIM_APPEND :
         if( m_pRun->InsertStimulus( (cmdID==ID_RUN_STIM_INS) ? iPos : -1 ) >= 0 )
            bUpdate = TRUE;
         break;

      default :
         ASSERT( FALSE );
         break;
   }                                                           // END:  process the command...


   if( bUpdate )                                               // a change was successfully made, and the update wasn't 
   {                                                           // handled elsewhere, so do it now:
      m_stimChanGrid.SetRowCount(m_pRun->GetStimulusCount() + 1);    // refresh stimlus grid
      m_stimChanGrid.Refresh();
      InformModify();                                                // inform doc/views of change
   }

   m_contextCell.row = m_contextCell.col = -1;                 // invalidate context cell
}


//=== OnUpdGridOps ==================================================================================================== 
//
//    ON_UPDATE_COMMAND_UI_RANGE handler for the context menu that pops up when the user right-clicks on appropriate 
//    cells in the stimulus channel grid (see OnNMRClick()). What commands are enabled dependd on the run's current 
//    contents and the "context cell" -- the grid cell that was right-clicked. Supported commands:
//
//       ID_RUN_STIM_CLEAR ==> Enabled if the stimulus channel list is not empty.
//       ID_RUN_STIM_COPY, ID_RUN_STIM_CUT, ID_RUN_STIM_DEL ==> Enabled if "context" cell refers to a valid stimulus.
//       ID_RUN_STIM_PASTE, ID_RUN_STIM_INS ==> Enabled if there is room for a new stimulus channel, and the "context" 
//          cell refers to a valid stimulus channel.
//       ID_RUN_STIM_APPEND ==> Enabled if there is room for a new stimulus channel.
//
//    ARGS:       pCmdUI   -- [in] represents UI item.
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnUpdGridOps( CCmdUI* pCmdUI )
{
   CLiteGrid* pFocus = (CLiteGrid*) GetFocus();                       // enabled only if a run is loaded, the stimlulus
   BOOL bEnable = ((m_pRun != NULL) && (&m_stimChanGrid == pFocus));  // grid has the focus...

   if(bEnable) 
   {
      int iPos = m_contextCell.row - 1;
      switch( pCmdUI->m_nID )
      {
         case ID_RUN_STIM_CLEAR :   
            bEnable = BOOL(m_pRun->GetStimulusCount() > 0); 
            break;
         case ID_RUN_STIM_COPY :
         case ID_RUN_STIM_CUT :
         case ID_RUN_STIM_DEL :     
            bEnable = m_pRun->IsValidStimulus( iPos ); 
            break;
         case ID_RUN_STIM_PASTE :
         case ID_RUN_STIM_INS :     
            bEnable = m_pRun->IsValidStimulus( iPos ) && 
                      BOOL(m_pRun->GetStimulusCount() < CCxContRun::GetMaxStimuli());
            break;
         case ID_RUN_STIM_APPEND :
            bEnable = BOOL(m_pRun->GetStimulusCount() < CCxContRun::GetMaxStimuli());
            break;
      }
   }

   pCmdUI->Enable( bEnable );
}



//===================================================================================================================== 
// OPERATIONS 
//===================================================================================================================== 

//=== OnInitialUpdate [base override] ================================================================================= 
//
//    This function is called by the SDI doc/view framework each time a new document is created/opened. Here we take 
//    care of both one-time inits and per-document inits; the one-time inits are only performed the first time that 
//    this method is invoked.
//
//    As part of the one-time inits, we dynamically subclass many of the controls on this form to CWnd-derived members 
//    of CCxContRunForm in order to simplify communication with the controls and take advantage of specialized 
//    functionality:
//       1) The custom control IDC_RF_STIMULI is subclassed to a CLiteGrid object. The various display and editing 
//          callbacks are installed, and the grid is initialized to an "empty" state.
//       2) The edit controls on the form are subclassed to CNumEdit objects in order to restrict the input to them 
//          The format traits of these numeric edit controls are also set here.
//
//    The only "per-document" init is to ensure that the form is emptied each time this method is called (since the 
//    previously loaded stimulus run object, if any, was defined in a document that is no longer there!)
//
//    ARGS:       NONE
// 
//    RETURNS:    NONE
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for initial rows and columns;
//                CNotSupportedException if any control subclassing fails (which should NOT happen!).
//
void CCxContRunForm::OnInitialUpdate() 
{
   int i;

   if(!m_bOneTimeInitsDone)                                                // ONE-TIME INITIALIZATIONS:
   {
      BOOL bOk = m_edDutyPeriod.SubclassDlgItem( IDC_RF_DUTYPER, this );   // subclass selected ctrls on the form 
      bOk = bOk && m_edNAutoStop.SubclassDlgItem( IDC_RF_AUTOSTOP, this );
      bOk = bOk && m_edHOffset.SubclassDlgItem( IDC_RF_HOFFSET, this );
      bOk = bOk && m_edVOffset.SubclassDlgItem( IDC_RF_VOFFSET, this );
      bOk = bOk && m_cbDutyPulse.SubclassDlgItem( IDC_RF_DUTYPULSE, this );
      bOk = bOk && m_stimChanGrid.SubclassDlgItem( IDC_RF_STIMULI, this );

      if( !bOk ) AfxThrowNotSupportedException();                          // the above must succeed to continue...
   
      m_edDutyPeriod.SetFormat( TRUE, TRUE, 6, 1 );                        // set format constraints on numedit ctrls 
      m_edNAutoStop.SetFormat( TRUE, TRUE, 4, 1 ); 
      m_edHOffset.SetFormat( FALSE, FALSE, 7, 2 ); 
      m_edVOffset.SetFormat( FALSE, FALSE, 7, 2 ); 

      CStringArray choices;                                                // install choices for the duty marker pulse
      CCxContRun::GetDutyPulseChoices( choices );                          // combo box
      for( i = 0; i < choices.GetSize(); i++ )
         m_cbDutyPulse.AddString( choices[i] );

                                                                           // SET UP THE STIMULUS CHANNEL GRID CTRL:
      m_stimChanGrid.EnableDragAndDrop( FALSE );                           //    disable drag-n-drop features
      m_stimChanGrid.SetRowResize( FALSE );                                //    user may not resize a row
      m_stimChanGrid.SetColumnResize( FALSE );                             //    user may not resize a column
      m_stimChanGrid.EnableSelection( FALSE );                             //    cells in grid cannot be selected

      m_stimChanGrid.SetCallbackFunc( StimGridDispCB, (LPARAM) this );     // install CB fcns which govern appearance, 
      m_stimChanGrid.SetEditCBFcn( StimGridEditCB, (LPARAM) this );        // editing of grid cells.  TRICK: pass THIS 
      m_stimChanGrid.SetEndEditCBFcn( StimGridEndEditCB, (LPARAM) this );  // b/c CB must be static and so does not get 
                                                                           // implied THIS.

      m_stimChanGrid.SetRowCount( 1 );                                     //    init grid with header row containing 
      int nMaxCols =                                                       //    all the columns we'll ever need:
         1 +                                                               //       first column holds "ch#"
         CCxStimulus::NumberOfCommonParameters() +                         //       one column per common parameter
         CCxStimulus::MaxNumberOfMotionParameters();                       //       one column per unique parameter  
      m_stimChanGrid.SetColumnCount( nMaxCols ); 
      m_stimChanGrid.SetFixedRowCount( 1 );                                //    (may throw CMemoryException)
      m_stimChanGrid.SetFixedColumnCount( 1 ); 

      CGridCellBase* pCell =                                               //    set default format for cell occupying  
         m_stimChanGrid.GetDefaultCell( TRUE, TRUE );                      //    both a fixed row and column, ...
      pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 
      pCell = m_stimChanGrid.GetDefaultCell( TRUE, FALSE );                //    cell occupying only a fixed row, ... 
      pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 
      pCell = m_stimChanGrid.GetDefaultCell( FALSE, TRUE );                //    cell occupying only a fixed col, ...
      pCell->SetFormat( DT_CENTER | DT_SINGLELINE ); 
      pCell = m_stimChanGrid.GetDefaultCell( FALSE,FALSE );                //    non-fixed cell 
      pCell->SetFormat( DT_CENTER | DT_SINGLELINE );

      m_stimChanGrid.SetGridLineColor( RGB(0,0,0) );                       //    use black grid lines 

      m_stimChanGrid.SetColumnWidth( 0, 40 );                              //    all column widths are fixed!
      for( i = 1; i < nMaxCols; i++ )
         m_stimChanGrid.SetColumnWidth( i, 75 );

      CRect rGrid;                                                         //    save grid's initial size, which is 
      m_stimChanGrid.GetWindowRect( rGrid );                               //    defined by dlg template, to ensure 
      ScreenToClient( rGrid );                                             //    grid is not made smaller than this 
      m_minSTCSize = rGrid.Size();

      CRect rClient;                                                       //    if there's room to enlarge grid upon 
      GetClientRect( rClient );                                            //    initial display, do it.
      if( (rClient.right - 7) > rGrid.right ) rGrid.right = rClient.right - 7;
      if( (rClient.bottom - 7) > rGrid.bottom ) rGrid.bottom = rClient.bottom - 7;
      if( m_minSTCSize != rGrid.Size() )
         m_stimChanGrid.MoveWindow( rGrid );

      m_bOneTimeInitsDone = TRUE;                                          // do NOT repeat these inits again!
   }

   LoadRun( CX_NULLOBJ_KEY );                                              // this sets up form in an "empty" state 

   TVTabPane::OnInitialUpdate();                                           // always call the base class version!
}


//=== OnUpdate [base override] ======================================================================================== 
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    This form must respond to a number of different "signals" broadcast by other views attached to the CCxDoc obj: 
//       CXVH_DSPOBJ:   If a run object is specified for display, load its definition.
//       CXVH_MODOBJ:   If another view modifies a Maestro object, it will send this hint. If the currently loaded run 
//                      was the object modified, then we must reload the controls on this form from scratch to ensure 
//                      that they reflect the current state of the run.
//       CXVH_NAMOBJ, 
//       CXVH_MOVOBJ:   This signal is sent whenever a Maestro object is renamed or when its pos in the Maestro object 
//                      tree is altered, which can affect the "fully qualified" name of the object. 
//       CXVH_DELOBJ,
//       CXVH_CLRUSR:   If the currently loaded run is deleted, then the form must be reset.
//
//    NOTES:
//    (1) Whenever a hint is NOT provided, we only call the base class -- to handle lower-level update tasks. 
//
//    ARGS:       pSender  -- [in] view which initiated the update
//                lHint    -- [in] an integer-valued hint (not used by Maestro views)
//                pHint    -- [in] if the initiating view provides a hint, this should be a CCxViewHint object.
// 
//    RETURNS:    NONE
//
void CCxContRunForm::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   if( pHint == NULL )                                                  // no hint provided -- just call base class 
   {
      TVTabPane::OnUpdate( pSender, lHint, pHint ); 
      return;
   }

   CCxDoc* pDoc = GetDocument();                                        // get reference to attached doc 
   CCxViewHint* pVuHint = (CCxViewHint*)pHint;                          // cast provided hint to Maestro hint class 

   switch( pVuHint->m_code )
   {
      case CXVH_DSPOBJ :                                                // display definition of specified run
         if( pVuHint->m_type == CX_CONTRUN )
         {
            BringToFront();                                             // bring this view to front of tab window; if 
            if( m_wKey != pVuHint->m_key )                              // obj is diff from what's currently there, 
               LoadRun( pVuHint->m_key );                               // load the new trial obj. 
         }
         break;

      case CXVH_MODOBJ :                                                // run object modified *outside* this view; 
         if( m_wKey != CX_NULLOBJ_KEY && m_wKey == pVuHint->m_key )     // refresh all controls to make sure they 
         {                                                              // reflect run's current state
            StuffHdrControls();

            int nRows = 1 + m_pRun->GetStimulusCount();                 // update # of rows in the grid, if necessary
            if(nRows != m_stimChanGrid.GetRowCount())
               m_stimChanGrid.SetRowCount( nRows );

            m_stimChanGrid.Refresh();                                   // refresh the grid entirely;
         }
         break;

      case CXVH_NAMOBJ :                                                // handle name updates to the loaded run   
      case CXVH_MOVOBJ : 
         if( m_pRun != NULL ) UpdateCaption(NULL);
         break;

      case CXVH_CLRUSR :                                                // entire document reinitialized; reset form
         if(m_wKey != CX_NULLOBJ_KEY)                                   // if a run is currently loaded on it
            LoadRun( CX_NULLOBJ_KEY );
         break;

      case CXVH_DELOBJ :                                                // if loaded run was deleted, reset form!
         if ( (m_wKey != CX_NULLOBJ_KEY) && 
              ((pVuHint->m_key==m_wKey) || (!pDoc->ObjExists(m_wKey))) )
            LoadRun( CX_NULLOBJ_KEY );
         break;

      default :                                                         // no response to any other hints...
         break;
   }
}



//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
#ifdef _DEBUG 

//=== Dump [base override] ============================================================================================ 
//
//    Dump internal state vars associated with this form view. 
//
//    ARGS:       dc -- [in] the current dump context. 
//
//    RETURNS:    NONE. 
//
void CCxContRunForm::Dump( CDumpContext& dc ) const
{
   TVTabPane::Dump( dc ); 

   CString msg;
   msg.Format( "\nMin stim channel grid size = (%d, %d)", m_minSTCSize.cx, m_minSTCSize.cy );
   dc << msg;

   msg = _T("\nNo run object displayed currently");
   if ( m_wKey != CX_NULLOBJ_KEY )
      msg.Format( "\nDisplayed run key = 0x%04x", m_wKey);
   dc << msg;

   if ( m_pPasteStim == NULL )
      dc << _T("\nThere is currently no paste stimulus");
   else
   {
      dc << _T("\nCurrent paste stimulus:");
      m_pPasteStim->Dump( dc );
   }

   msg.Format( "\nTransient state: context cell = (%d,%d)", m_contextCell.row, m_contextCell.col); 
   dc << msg;
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate internal consistency of the form view.  
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxContRunForm::AssertValid() const
{
   TVTabPane::AssertValid();

   if( !m_bLoading )       // don't enforce this assertion when in the middle of loading or resetting form!
   {
      ASSERT( (m_wKey == CX_NULLOBJ_KEY && m_pRun == NULL) || (m_wKey != CX_NULLOBJ_KEY && m_pRun != NULL) );
   }
   ASSERT( (m_pPasteStim == NULL) || m_pPasteStim->IsKindOf( RUNTIME_CLASS(CCxStimulus) ) );
}

#endif //_DEBUG



//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== LoadRun ========================================================================================================= 
//
//    Load definition of specified run object into the form view, updating form's internal state vars and appearance 
//    accordingly. If no run is specified (CX_NULLOBJ_KEY), the form is reset to an "empty" state.  
//
//    ARGS:       key   -- [in] the unique identifying key of CNTRLX run obj (CX_NULLOBJ_KEY to clear form). 
// 
//    RETURNS:    NONE
//
VOID CCxContRunForm::LoadRun( const WORD key )
{
   // set transient flag so grid display callbacks will not access stale run pointer while loading or resetting form
   m_bLoading = TRUE; 

   // remove focus from a grid cell before modifying grid; avoids ASSERT in SetRow/ColumnCount() below
   m_stimChanGrid.SetFocusCell( -1, -1 );                         

   m_wKey = key;                                                  // unique key of trial to be displayed on form 

   if( m_wKey != CX_NULLOBJ_KEY )                                 // if there is a run to display, get ptr to the 
   {                                                              // run object
      m_pRun = (CCxContRun*) GetDocument()->GetObject( m_wKey ); 
      ASSERT( (m_pRun != NULL) && (m_pRun->IsKindOf( RUNTIME_CLASS(CCxContRun) )) );

      m_stimChanGrid.SetRowCount(1 + m_pRun->GetStimulusCount()); // set row count for each grid (#cols fixed for both) 
   }
   else                                                           // no run to display; make sure form is cleared... 
   {
      m_pRun = NULL;
      m_stimChanGrid.SetRowCount( 1 );
   }

   m_bLoading = FALSE;                                            // re-enable grid display callbacks

   BOOL bEnable = BOOL(m_wKey != CX_NULLOBJ_KEY);                 // enable & stuff the non-grid controls
   m_cbDutyPulse.EnableWindow( bEnable );
   m_edDutyPeriod.EnableWindow( bEnable );
   m_edNAutoStop.EnableWindow( bEnable );
   m_edHOffset.EnableWindow( bEnable );
   m_edVOffset.EnableWindow( bEnable );
   StuffHdrControls();

   m_stimChanGrid.Refresh();                                      // refresh stimlus grid

   m_contextCell.row = m_contextCell.col = -1;                    // make sure transient state vars are reset

   UpdateCaption(NULL);                                           // update assoc tab caption w/ name of run loaded
}


//=== UpdateCaption [TVTabPane override] ============================================================================== 
//
//    Update the caption of the tab item associated with this tab pane. If a NULL argument is provided, the method 
//    will use the name of the object currently loaded; if no obj is loaded, the placeholder title "Run" is used.
//
//    ARGS:       szCaption   -- [in] desired caption for this tab. Use NULL to get automated caption.
//
//    RETURNS:    NONE.
//
VOID CCxContRunForm::UpdateCaption( LPCTSTR szCaption )
{
   CString strTitle;
   if(szCaption != NULL) 
      strTitle = szCaption;
   else
      strTitle = (m_wKey!=CX_NULLOBJ_KEY) ? GetDocument()->GetObjName( m_wKey ) : _T("Run");
   TVTabPane::UpdateCaption( strTitle );
}


//=== StuffHdrControls ================================================================================================ 
//
//    Load all of the "header parameter controls" -- ie, all controls other than the stimlus grid -- IAW the current 
//    state of the loaded run. If no run is loaded, controls are put in an initial default state.
//
//    ARGS:       NONE. 
// 
//    RETURNS:    NONE.
//
VOID CCxContRunForm::StuffHdrControls()
{
   if( m_pRun != NULL ) 
   {
      CString str;
      m_cbDutyPulse.SetCurSel( m_pRun->GetDutyPulse() );
      m_edDutyPeriod.SetWindowText( m_pRun->GetDutyPeriod() );
      m_edNAutoStop.SetWindowText( m_pRun->GetAutoStop() );
      m_edHOffset.SetWindowText( m_pRun->GetHOffset() );
      m_edVOffset.SetWindowText( m_pRun->GetVOffset() );
   }
   else
   {
      m_cbDutyPulse.SetCurSel( 0 );
      m_edDutyPeriod.SetWindowText( 0 );
      m_edNAutoStop.SetWindowText( 0 );
      m_edHOffset.SetWindowText( 0.0 );
      m_edVOffset.SetWindowText( 0.0 );
   }
}


//=== InformModify ==================================================================================================== 
//
//    Invoke this method to inform the Maestro experiment document (CCxDoc) and other attached views that the currently 
//    loaded run object was just modified.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxContRunForm::InformModify()
{
   ASSERT( m_wKey != CX_NULLOBJ_KEY );
   CCxDoc* pDoc = GetDocument(); 
   pDoc->SetModifiedFlag();
   CCxViewHint vuHint( CXVH_MODOBJ, CX_CONTRUN, m_wKey );
   pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );
}


//=== IsStimGridCellReadOnly ========================================================================================== 
//
//    Is specified cell in stimulus channel table read-only?  Cells in the first row (column header labels) and first 
//    column (stimulus channel #s) are read-only, as are any cells that do not correspond to a legal stim parameter.
//
//    ARGS:       c     -- [in] a cell within stimulus channel grid.
//
//    RETURNS:    TRUE if cell is read-only, FALSE otherwise. 
//
BOOL CCxContRunForm::IsStimGridCellReadOnly(const CCellID& c) const
{
   return( c.row == 0 || c.col == 0 || m_pRun == NULL || !m_pRun->IsValidStimParameter( c.row-1, c.col-1 ) );
}


//=== StimGridDispCB ================================================================================================== 
//
//    Callback function queried by the stimulus channel grid to obtain cell display info, OR virtual-mode tooltip text 
//    (when item state flag GVIS_VIRTUALTITLETIP is set), OR label tip text (state flag GVIS_VIRTUALLABELTIP is set).
//
//    The CCxContRun object has been designed so that we can obtain information on any parameter of a given stimulus 
//    channel simply by knowing the channel's zero-based index and the parameter's zero-based index.  In other words, 
//    CCxContRun is tailored for a table-like GUI presentation!  When no run is loaded, the grid should be empty except 
//    for the header row.  The callback still works in this case.  Also, we can invoke CCxContRun::GetStimParameter() 
//    and GetStimParameterLabel() with invalid parameter indices; they just return empty strings.  In this case, we set 
//    the bkg color to that of a fixed cell to emphasize that the cell is NOT used....  The routine also provides 
//    "label tip" text -- but only for cells that display a channel's "unique" motion parameters.  For these cells, the 
//    parameter's "identity" will vary IAW the current channel type, so we cannot put the labels in the header row!
//
//    NOTE: A callback function must be static.  As such, it does not have access to instance fields of the object.  To 
//    circumvent this problem, we take advantage of the generic LPARAM argument, passing a reference to this view when 
//    we register the callback fcn with the grid in OnInitialUpdate().
//
//    ARGS:       pDispInfo   -- [in/out] ptr to a struct we need to fill with the appropriate display info. 
//                lParam      -- [in] THIS (see BACKGROUND).
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxContRunForm::StimGridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam ) 
{
   CCxContRunForm* pThis = (CCxContRunForm*)lParam;                     // to access non-static data in the view
   CCxContRun* pRun = pThis->m_pRun;                                    // the currently loaded run object
   CLiteGrid* pGrid = &(pThis->m_stimChanGrid);                         // the stimulus channel grid
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );               // the cell to be displayed

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )           // ignore when no grid or cell not valid
      return( FALSE ); 

   if( pThis->m_bLoading )                                              // disable callback while we're changing run
      return( FALSE );                                                  // that's loaded on form

   int iCh = c.row - 1;                                                 // index of stimulus channel assoc w/ cell
   int iParam = c.col - 1;                                              // index of parameter assoc w/ cell
   ASSERT( iCh == -1 || pRun != NULL );                                 // if we're not looking at header row, a run 
                                                                        // object MUST be loaded onto form!
   int nCommon = CCxStimulus::NumberOfCommonParameters();               // # of common parameters in all stim channels 

   pDispInfo->item.strText = _T("");                                    // make sure we start out with an empty string 
   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                  // special case: "label tips" are provided only 
   {                                                                    // for cells holding "unique" motion params... 
      if( iCh >= 0 && iParam >= nCommon )
         pRun->GetStimParameterLabel( iCh, iParam, pDispInfo->item.strText );
      else
         pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP;
   }
   else if( iCh == -1 )                                                 // we must be looking at the header row!
   {
      if( iParam == -1 ) pDispInfo->item.strText = _T("Ch#");
      else if( iParam < nCommon )                                       // (all other cells in header row are blank!)
         CCxStimulus::GetCommonParameterLabel( iParam, pDispInfo->item.strText );
   }
   else if( iParam == -1 )                                              // first col in a channel row is channel index
      pDispInfo->item.strText.Format( "%d", iCh );
   else                                                                 // get value for a motion parameter 
   { 
      pRun->GetStimParameter( iCh, iParam, pDispInfo->item.strText );
      if( pDispInfo->item.strText.IsEmpty() )                           // if string empty (ie, cell not used), use 
      {                                                                 // fixed cell bkg color to emphasize that this 
         CGridCellBase* pCell =                                         // cell is not editable
            (pThis->m_stimChanGrid).GetDefaultCell( TRUE, TRUE );
         pDispInfo->item.crBkClr = pCell->GetBackClr();
      }
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                     // only show title tip if the cell's text is 
                                                                        // too big to fit...
   return( TRUE );
}


//=== StimGridEditCB ================================================================================================== 
//
//    Callback invoked to initiate inplace editing of a cell in the stimulus channel table, or to increment/decrement 
//    the contents of a cell in response to a left or right mouse click.  Below is a summary of the possible operations 
//    that this callback permits:
//
//       1) Cell in the fixed row 0 ==> These are read-only column header labels. 
//       2) Cell in the fixed col 0  ==> The read-only stimulus channel number. 
//       3) Cell in row N>0, cols M>0 ==> Value of parameter M-1 for stimulus channel N-1.  If the parameter is 
//          multiple-choice, a left(right) mouse click will increment(decrement) the current choice.  Otherwise, it is 
//          a numeric parameter and is not affected by a mouse click.  If the grid is about to initiate an inplace edit 
//          operation, we provide it with the appropriate information:  an array of choices for a multi-choice param, 
//          or the numeric format constraints for a numeric param.  All of this information is provided by the 
//          currently loaded CCxContRun data object.
//
//    Most parameter changes on the stimulus channel grid do not affect the values of other parameters.  However, there 
//    are a few cases where "side effects" may occur.  Those side effects are limited to the row representing the stim 
//    channel just edited, and column 1 representing the "ON/off" state of each channel. 
//
//    NOTE:  See also StimGridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info. 
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxContRunForm::StimGridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxContRunForm* pThis = (CCxContRunForm*)lParam;                     // to access non-static data in the view
   CCxContRun* pRun = pThis->m_pRun;                                    // the currently loaded run object
   CLiteGrid* pGrid = &(pThis->m_stimChanGrid);                         // the stimulus channel grid
   CCellID c = pEI->cell;                                               // the cell to be edited

   if( (pRun == NULL) || pGrid->GetSafeHwnd() == NULL ||                // if run not loaded, or grid is gone, or 
       !pGrid->IsValid( c ) )                                           // cell not valid, ignore! 
      return( FALSE ); 

   int iCh = c.row - 1;                                                 // index of stimulus channel assoc w/ cell
   int iParam = c.col - 1;                                              // index of parameter assoc w/ cell
   ASSERT( pRun != NULL ); 

   int nCommon = CCxStimulus::NumberOfCommonParameters();               // # of common parameters in all stim channels 

   if( pThis->IsStimGridCellReadOnly( c ) )                             // cannot edit "readOnly" cells!
   { 
      pEI->iClick = 0; 
      pEI->iType = LG_READONLY;
   }
   else if( pEI->iClick != 0 )                                          // edit by mouse click?:
   {
      if( pRun->IsStimParameterMultiChoice( iCh, iParam ) )             // if param is multi-choice, a click will 
      {                                                                 // increment or decrement the current choice
         int iNew = pRun->GetStimParameterAsInt( iCh, iParam );
         iNew += (pEI->iClick > 0) ? 1 : -1;
         if( pRun->SetStimParameter( iCh, iParam, iNew ) )              //    if change affects value/appearance of 
         {                                                              //    another param, redraw current row and 
            pGrid->RedrawRow( iCh+1 );                                  //    col 1 to make sure grid is up-to-date
            pGrid->RedrawColumn( 1 );
         }
         pThis->InformModify();                                         //    inform doc/vu framework of change
      }
      else                                                              // otherwise, the click has no effect
         pEI->iClick = 0;
   }
   else                                                                 // initiate inplace edit of stim parameter:
   {
      BOOL bIsChoice = FALSE;                                           //    get parameter type/format info
      pRun->GetStimParameterFormat( iCh, iParam, bIsChoice, pEI->strArChoices, pEI->numFmt );
      pEI->iType = bIsChoice ? LG_MULTICHOICE : LG_NUMSTR;
      if( bIsChoice ) pEI->iCurrent = pRun->GetStimParameterAsInt( iCh, iParam );
      else pEI->dCurrent = pRun->GetStimParameter( iCh, iParam );
   }

   return( TRUE );
}


//=== StimGridEndEditCB =============================================================================================== 
//
//    Callback invoked upon termination of inplace editing of a cell in the stimulus channel table.
//
//    Here we update the stimulus run IAW the change made during the inplace operation that was configured in 
//    StimGridEndEditCB().  Based on the value of the exit key character that terminated the operation, we may direct 
//    the grid to continue inplace editing at another, nearby cell.
//
//    NOTE:  See also GridEditCB().
//
//    ARGS:       pEEI     -- [in/out] ptr to a struct containing results of inplace edit op, and our response.
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxContRunForm::StimGridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                   // user cancelled inplace operation

   CCxContRunForm* pThis = (CCxContRunForm*)lParam;                     // to access non-static data in the view
   CCxContRun* pRun = pThis->m_pRun;                                    // the currently loaded run object
   CLiteGrid* pGrid = &(pThis->m_stimChanGrid);                         // the stimulus channel grid
   CCellID c = pEEI->cell;                                              // the cell that was edited

   if( (pRun == NULL) || pGrid->GetSafeHwnd() == NULL ||                // if run not loaded, or grid is gone, or 
       !pGrid->IsValid( c ) )                                           // cell not valid, ignore! 
      return( FALSE ); 

   int iCh = c.row - 1;                                                 // index of stimulus channel assoc w/ cell
   int iParam = c.col - 1;                                              // index of parameter assoc w/ cell

   if( pThis->IsStimGridCellReadOnly( c ) )                             // cannot edit "readOnly" cells!
   { 
      ASSERT( FALSE );                                                  //    this should NEVER happen!
      pEEI->nExitChar = VK_ESCAPE;                                      //    prevent continued inplace editing
      pEEI->bNoRedraw = TRUE;                                           //    no need to redraw since no change made 
      return( TRUE );
   }
   ASSERT( pRun );

   if( pEEI->bIsChanged )                                               // if the user made a change...
   {
      BOOL bSideEffect = FALSE; 
      if( pRun->IsStimParameterMultiChoice( iCh, iParam ) )             //    update the multichoice or numeric param 
         bSideEffect = pRun->SetStimParameter( iCh, iParam, int(pEEI->dwNew) ); 
      else
         bSideEffect = pRun->SetStimParameter( iCh, iParam, pEEI->dNew );

      if( bSideEffect )                                                 //    if change affected other parms, redraw 
      {                                                                 //    entire stim channel row & col 1 (chan 
         pGrid->RedrawRow( c.row );                                     //    on/off state)... in which case grid need 
         pEEI->bNoRedraw = TRUE;                                        //    not redraw the edited cell!
      }
      pThis->InformModify();                                            //    inform doc/view framework
   }

   do                                                                   // goto next edit cell if inplace op terminated 
   {                                                                    // by certain navig. keys; skip readonly cells
      switch( pEEI->nExitChar ) 
      { 
         case VK_TAB :                                                  //    TAB:  move to next col, wrapping to first 
            ++c.col;                                                    //    col of next row if nec.  if on last row, 
            if( c.col >= pGrid->GetColumnCount() )                      //    go to top row.
            { 
               c.col = 1;
               if( ++c.row >= pGrid->GetRowCount() ) c.row = 0;
            }
            break;
         case VK_RIGHT :                                                //    rt arrow: move to next col, wrapping back 
            if( ++c.col >= pGrid->GetColumnCount() ) c.col = 0;         //    to first col of same row if necessary.
            break;
         case VK_LEFT :                                                 //    lf arrow: move to prev col, wrapping back 
            if( --c.col < 0 ) c.col = pGrid->GetColumnCount() - 1;      //    to last col of same row if necessary.
            break;
         case VK_DOWN :                                                 //    dn arrow: move to next row, wrapping back 
            if( ++c.row >= pGrid->GetRowCount() ) c.row = 0;            //    to top row if necessary.
            break;
         case VK_UP :                                                   //    up arrow: move to prev row, wrapping back 
            if( --c.row < 0 ) c.row = pGrid->GetRowCount() - 1;         //    to bottom row if necessary. 
            break;
         default :                                                      //    the above keys are the only navigation 
            pEEI->nExitChar = 0;                                        //    keys we allow!
            break;
      }

   } while( pEEI->nExitChar != 0 && pThis->IsStimGridCellReadOnly( c ) );

   if( c == pEEI->cell ) pEEI->nExitChar = 0;                           // prevent continuation on the same cell!
   else if( pEEI->nExitChar != 0 ) pEEI->cellNext = c;

   return( TRUE );
}
