//===================================================================================================================== 
//
// cxchannelform.cpp : Implementation of class CCxChannelForm.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CCxChannelForm is a dialog-like form view that manages a number of controls used to display and/or modify a CNTRLX 
// "channel configuration" object, encapsulated by the data class CCxChannel.  The channel configuration holds a number 
// of display attributes for each of CNTRLX analog input and digital input channels, as well as a number of "computed" 
// channels.  See CCxChannel for the details.
//
// ==> Construction of form; controls.
// The form's layout is defined in the dialog template resource IDD_CHANNELFORM.  Since we supply this ID in the 
// default constructor, the Visual Framework and MFC handle the details of loading the template and creating the view. 
// Use the DevStudio Resource Editor to review the layout of IDD_CHANNELFORM.  The form includes three buttons, two 
// edit controls, and a custom control based on the MFC Grid Control (see CREDITS).  The grid is resized initially so 
// that it displays all channel configuration attributes without having to scroll the grid **itself** -- the user must 
// scroll the form view as needed.  The two edit controls (IDC_CH_YMIN, IDC_CH_YMAX) specify the y-axis range 
// associated with the channel configuration.  The three buttons offer three different operations on the channel 
// configuration matrix, all of which are administered by the CCxChannel data object itself:  IDC_CH_RESTOREDEF, 
// IDC_CH_SPACEEVENLY, and IDC_CH_ONEAXIS.  Note that these integer resource IDs must represent a contiguous range of 
// values so that we can use ON_CONTROL_RANGE in the CCxChannelForm message map.
//
// ==> Interactions with CCxChannel and CCxDoc.
// Like all data objects in CNTRLX, each different channel configuration CCxChannel is assigned a unique key.  When a 
// new channel configuration object is to be loaded onto the form, CCxChannelForm supplies the key to CCxDoc in order 
// to obtain a CCxChannel* ptr to the actual channel configuration object.  CCxChannelForm then queries the CCxChannel 
// object directly to retrieve or modify individual channel cfg attributes, etc.  Whenever it modifies the CCxChannel 
// object, it must notify the doc & other views of the change -- see InformModify().
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
// ==> The channel configuration "grid" and CLiteGrid; editing individual attributes "in place".
// CLiteGrid -- a derivative of the MFC grid control, CGridCtrl (CREDITS) -- is used to display channel configuration 
// attributes of all CNTRLX data channels in a condensed tabular form.  We take advantage of the grid's "virtual mode", 
// a lightweight implementation that avoids the memory overhead of associating a CGridCell-derived object with each and 
// every cell in the grid.  In this scheme, the grid ctrl uses a registered callback function to obtain info needed to 
// repaint any grid cell.  This callback function, CCxChannelForm::GridDispCB(), determines the appearance of the grid. 
// The grid itself is initialized with the appropriate number of rows and columns in OnInitialUpdate().
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
// Here is a summary of the channel configuration attributes displayed in the grid, and how they may be modified:
//
//    Record ON/OFF flag   -- A left or right mouse click toggles the flag's state.  The flag can also be set using an 
//                            inplace combo box.
//    Display ON/OFF flag  -- As above.
//    Display offset       -- This attribute can take on integer values in a relatively large range set by the static 
//                            CCxChannel method GetOffsetRange().  An inplace numeric edit box, managed by CLiteGrid, 
//                            is used to modify this parameter.
//    Display gain, color  -- These are multiple-choice parameters with a limited range of possible values.  Value can 
//                            be changed by a left or right mouse click, or using an inplace combo box.
//
// ==> CNTRLX restriction on the # of channels that can be displayed at one time.
// CCxChannel defines a relatively large # of data channels.  To display acquired data from all of these channels at 
// the same time is neither practical nor useful in CNTRLX.  Thus, a limit -- CCntrlxApp::MaxTraces() -- is placed on 
// the # of channels the user can display at one time.  We enforce the limit here rather than in the data object 
// CCxChannel itself, because it's a GUI limitation, not an inherent limitation of the data object CCxChannel.  When 
// the user tries to set the display flag for another channel once this limit is reached, CCxChannelForm ignores the 
// attempt and displays a warning.
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
// 01jan2001-
// 16jan2001-- Initial development in a dummy application.
// 19jan2001-- Modified for use in CNTRLX.
// 26jan2001-- Added method LoadChanCfg(), which also disables controls if form is empty or if loaded channel config 
//             has its CX_ISPREDEF flag set.
//          -- Updated implementation with respect to recent changes in CCxViewHint and cxobj_ifc.h.
// 15mar2001-- Modified to support new version of CCxChannel which introduces 16 digital input channels...
//          -- Enforce limit MAXDSPCHS=10 on # of data channels that can be displayed simultaneously.
// 22mar2001-- If other entities call CCxChannelForm::EnableWindow(), its children are also updated.  This subverts our 
//             desire to control the enable/disable state of controls on the form.  This was b/c we derived the form 
//             from TVisualFormView, which defines an OnEnable() override that sends a WM_ENABLE message to all of its 
//             children.  As we don't really need the add'l support in TVisualFormView, we'll just use CFormView.
//          -- Modified dialog resource template to include a title bar, in which we display the full name of the 
//             channel configuration object currently displayed, or "Chan Cfg: <none>".  Updated implementation to 
//             support this use of the title bar.
// 26mar2001-- Title bar failed to work; I think Windows does not support title bars within panes of CSplitterWnd. 
//             Still, I decided to maintain the name of the current object in the window text.  It can then be 
//             retrieved at any time by the parent frame window via CWnd::GetWindowText()!!
//          -- A new approach to updating tab pane captions and bringing tab to front.  Instead of involving CNTRLX's 
//             main frame window, have the installed tab pane view communicate directly with its parent tab window.  
//             Using TVTabPane as base class, which provides the functionality we need.  See UpdateTitle() and 
//             OnUpdate().
// 02may2001-- Minor changes to compensate for mod in specification of CInPlaceNumEdit::BeginEdit().  The control 
//             rect passed to BeginEdit() must now be in the client coords of the window passed to it, which is 
//             considered the inplace edit control's parent until it is again extinguished. 
// 15aug2001-- Mods to bring CCxChannelForm in line with changes/additions to CCxChannel.
//          -- Max # of displayed channels now determined by CCntrlxApp::MaxTraces().  Removed private const MAXDSPCHS. 
//          -- Renamed control IDs for existing controls.  Added controls to specify the vertical (y-axis) display 
//             range associated with the channel configuration, IDC_CH_YMIN, IDC_CH_YMAX.  Added support for several of 
//             the standard editing commands ID_EDIT_***.
// 27sep2001-- Cosmetic: CExperimentDoc --> CCxDoc.
// 13nov2001-- Bug fix:  In SDI apps, each view's OnInitialUpdate() is called not once, but each time a new document is 
//             created/opened.  Control subclassing in OnInitialUpdate() should only be done once!  MFC documentation 
//             suggests putting one-time init code in OnCreate() instead, but I'm not sure that will work for form 
//             views based on dialog templates.  Instead, I just include a check to avoid repeated subclassing....
// 08feb2002-- Decided that the "default" channel config object (marked by CX_ISPREDEF) should be modifiable by the 
//             user.  It just cannot be renamed or moved or deleted -- restrictions which are handled elsewhere.  Here 
//             we got rid of code that disabled controls when the predefined channel config is loaded.
// 28feb2002-- EN_CHANGE is sent for programmatic as well as user changes of edit control contents, and it can be sent 
//             each time the user presses a key while in the ctrl.  We'll respond instead to EN_KILLFOCUS -- ie, wait 
//             till the user has finished making changes and moved on to a different control.
// 23oct2002-- Minor change to emphasize that units of display offset & y-axis range are mV!
// 04nov2002-- MAJOR MOD.  Using CLiteGrid -- which provides built-in support for inplace editing of grid in virtual 
//             mode -- in place of CGridCtrl.  CCxChannelForm no longer has to deal with the mechanics of inplace 
//             editing; it just installs the necessary callback functions.
// 15mar2004-- UpdateTitle() replaced by overriding TVTabPane::UpdateCaption() instead.  Also, we no longer display the 
//             full "pathname" of an object b/c it makes the tab text too long...
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // includes resource IDs for CNTRLX 

#include "cxobj_ifc.h"                       // CNTRLX object interface:  common constants and other defines 
#include "cxviewhint.h"                      // the "hint" class used by all CNTRLX views 
#include "cxchannelform.h" 


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CCxChannelForm, TVTabPane )

BEGIN_MESSAGE_MAP( CCxChannelForm, TVTabPane )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_CH_RESTOREDEF, IDC_CH_ONEAXIS, OnButtonClicked )
   ON_CONTROL_RANGE( EN_KILLFOCUS, IDC_CH_YMIN, IDC_CH_YMAX, OnEditRange ) 
   ON_UPDATE_COMMAND_UI_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnUpdateEditCommand )
   ON_COMMAND_RANGE( ID_EDIT_CLEAR, ID_EDIT_REDO, OnEditCommand )
END_MESSAGE_MAP()



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxChannelForm [constructor] ==================================================================================== 
//
//    Construct the channel configuration form view. 
//
//    Almost all the work is handled by the framework, which loads the form's layout from a CNTRLX resource whose 
//    identifier is stored in CCxChannelForm's protected member var IDD.  However, we do need to initialize certain 
//    variables that track the form's state.
//
CCxChannelForm::CCxChannelForm() : TVTabPane( IDD )
{
   m_bOneTimeInitsDone = FALSE;

   m_wKey = CX_NULLOBJ_KEY;
   m_pChanCfg = NULL;

   m_nChDisplayed = 0;
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnButtonClicked ================================================================================================= 
//
//    In response to a button press, perform the requested operation on the currently loaded channel configuration.
//    
//    ARGS:       nID   -- [in] id of button ctrl that was clicked.
//
//    RETURNS:    NONE
//
void CCxChannelForm::OnButtonClicked( UINT nID )
{
   if( m_pChanCfg == NULL ) return;                                     // no channel configuration loaded; abort

   switch( nID )
   {
      case IDC_CH_RESTOREDEF :   m_pChanCfg->RestoreDefaults(); break;  // restore default values
      case IDC_CH_SPACEEVENLY :  m_pChanCfg->SpaceEvenly(5000); break;  // evenly space offsets of displayed channels
      case IDC_CH_ONEAXIS :      m_pChanCfg->SpaceEvenly( 0 ); break;   // put all displayed channels at y=0
      default:                   ASSERT( FALSE ); return;               // should never get here!
   }

   m_grid.Refresh();                                                    // a global change -- refresh entire grid
   InformModify();                                                      // notify doc & other views of the change
}


//=== OnEditRange ===================================================================================================== 
//
//    Respond to EN_KILLFOCUS notifications from the edit ctrls specifying the y-axis range associated with the channel 
//    configuration currently loaded on form.  If the new y-axis range limits are invalid, they are corrected here.
//
//    ARGS:       id -- [in] resource ID of the child control that sent the notification. 
// 
//    RETURNS:    NONE
//
void CCxChannelForm::OnEditRange( UINT id )
{
   if( m_pChanCfg == NULL ) return;                                  // if no channel config loaded, ignore

   int yMin = m_edYMin.AsInteger();                                  // retrieve the new range limits.  while one ctrl 
   int yMax = m_edYMax.AsInteger();                                  // is modified at a time, both range limits must 
                                                                     // be checked each time...

   int yMinOld, yMaxOld;                                             // the old range limits 
   m_pChanCfg->GetDispRange( yMinOld, yMaxOld ); 

   if( !m_pChanCfg->SetDispRange( yMin, yMax ) )                     // update channel config; if either value had to 
   {                                                                 // be corrected, restuff both controls...
      m_edYMin.SetWindowText( yMin );
      m_edYMax.SetWindowText( yMax );
      ::MessageBeep( MB_ICONEXCLAMATION );
   }

   if( yMinOld != yMin || yMaxOld != yMax ) InformModify();          // if a value actually changed, inform doc/views 
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
void CCxChannelForm::OnUpdateEditCommand( CCmdUI* pCmdUI )
{
   CWnd *pFocusWnd = CWnd::GetFocus();                         // get CWnd object with the focus.  if it is not a 
   if ( (pFocusWnd == NULL) ||                                 // CNumEdit obj, then disable all Edit cmds -- no other 
        (!pFocusWnd->IsKindOf( RUNTIME_CLASS(CNumEdit) )) )    // controls on this form support ID_EDIT_*** commands. 
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
//    ARGS:       nID -- [in] edit command ID. 
//
//    RETURNS:    NONE
//
void CCxChannelForm::OnEditCommand( UINT nID )
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
//    the appropriate number of rows and columns (including one fixed row for column headings and one fixed column for 
//    row headings) to display the configuration attributes of all CNTRLX data channels.
//
//    Since we cannot be sure how large the grid must be when we design the dialog template that's the foundation for 
//    this form view, we must adjust the size of the grid control at this point.  We make its window rect large 
//    enough that it does not have to do any scrolling.  The form view itself must be informed of the change in its 
//    scrollable size, which is initially set to the size of the dialog template! 
//
//    ARGS:       NONE
// 
//    RETURNS:    NONE
//
//    THROWS:     CMemoryException if grid control cannot allocate memory for required rows and columns.
//
void CCxChannelForm::OnInitialUpdate() 
{
   if( !m_bOneTimeInitsDone )                                  // ONE-TIME INITIALIZATIONS:
   {
      m_edYMin.SubclassDlgItem( IDC_CH_YMIN, this );           // subclass edit ctrls for y-axis range to CNumEdit and 
      m_edYMin.SetFormat( TRUE, FALSE, 6, 1 );                 // initialize format constraints...
      m_edYMax.SubclassDlgItem( IDC_CH_YMAX, this );
      m_edYMax.SetFormat( TRUE, FALSE, 6, 1 );

      m_grid.SubclassDlgItem( IDC_CH_GRID, this );             // attach custom ctrl to our grid ctrl obj

      m_grid.EnableDragAndDrop( FALSE );                       // set up the grid...
      m_grid.SetRowResize( FALSE );
      m_grid.SetMinColWidth( 60 );
      m_grid.SetColumnResize( FALSE );
      m_grid.EnableSelection( FALSE );

      m_grid.SetCallbackFunc( GridDispCB, (LPARAM) this );     // install callbacks which govern appearance/editing 
      m_grid.SetEditCBFcn( GridEditCB, (LPARAM) this );        // of grid cells.  TRICK: we pass THIS reference b/c CB  
      m_grid.SetEndEditCBFcn( GridEndEditCB, (LPARAM) this );  // fcn must be static and so does not get implied THIS.

      try                                                      // set up required # of rows & columns to display 
      {                                                        // the channel configuration
         m_grid.SetRowCount(CCxChannel::GetNumChannels() + 1);
         m_grid.SetColumnCount( 6 );
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

      m_grid.SetGridLineColor(::GetSysColor(COLOR_3DSHADOW));  // grid line color

      m_grid.AutoSize( GVS_BOTH );

      CRect rGrid;                                             // resize grid ctrl rect so that it displays all ch's 
                                                               // without itself requiring scroll bars...
      m_grid.GetWindowRect( rGrid );                           //    the initial size from template definition
      ScreenToClient( rGrid );                                 //    convert to coord sys of the form view!!

      long w = m_grid.GetVirtualWidth() +                      //    get required width & height to avoid scroll bars 
               (2 * GetSystemMetrics(SM_CXSIZEFRAME));         //    on the grid ctrl itself...
      long h = m_grid.GetVirtualHeight() + 
               (2 * GetSystemMetrics(SM_CYSIZEFRAME));

      long diffW = w - rGrid.Width();                          //    make the change in the grid's rect
      long diffH = h - rGrid.Height();
      rGrid.right += diffW;
      rGrid.bottom += diffH;

      CSize szScroll = GetTotalSize();                         // adjust form view's scroll extents to handle the  
      szScroll.cx += diffW;                                    // change in the grid rect.
      szScroll.cy += diffH; 
      SetScrollSizes( MM_TEXT, szScroll, 
                      CSize(60,60), CSize(10,10) );

      m_grid.MoveWindow( rGrid );                              // here's where we actually resize the grid ctrl; note 
                                                               // that the buttons on the form are unaffected, as they 
                                                               // lie to the left of the grid!!

      m_bOneTimeInitsDone = TRUE;                              // do NOT repeat these inits again!
   }

   LoadChanCfg( CX_NULLOBJ_KEY );                              // initialize form as empty (no chan cfg loaded)

   TVTabPane::OnInitialUpdate();                               // base class stuff -- ultimately calls OnUpdate()
}


//=== OnUpdate [base override] ======================================================================================== 
//
//    This function is called by the doc/view framework whenever the document contents have changed.
//
//    This view must respond to a number of different "signals" broadcast by other views attached to the CCxDoc obj: 
//       CXVH_DSPOBJ:   May need to load a different channel configuration onto the form.
//       CXVH_MODOBJ:   If another view modifies a CNTRLX object, it may send this hint.  If the currently loaded 
//                      channel config was the object modified, then we just refresh the entire grid to ensure it 
//                      reflects the new configuration.
//       CXVH_NAMOBJ,  
//       CXVH_MOVOBJ:   Full "pathname" of the currently loaded channel config may have changed.  Update caption of 
//                      associated tab pane to reflect any change. 
//       CXVH_DELOBJ,
//       CXVH_CLRUSR:   If the currently loaded channel config is deleted, then the view must be reset.
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
void CCxChannelForm::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
   if ( pHint == NULL )                                  // no hint provided -- just call base class 
   {
      CView::OnUpdate( pSender, lHint, pHint ); 
      return;
   }

   CCxDoc* pDoc = GetDocument();                         // get reference to attached doc 
   CCxViewHint* pVuHint = (CCxViewHint*)pHint;           // cast provided hint to the CNTRLX hint class 

   switch( pVuHint->m_code )
   {
      case CXVH_DSPOBJ :                                 // load definition of specified channel configuration 
         if( pVuHint->m_type == CX_CHANCFG )
         {
            BringToFront();                              // at least bring this view to front of tab window; if obj 
            if( m_wKey != pVuHint->m_key )               // is diff from what's currently there, load the new obj
               LoadChanCfg( pVuHint->m_key );
         }
         break;

      case CXVH_MODOBJ :                                 // if currently displayed channel config was modified outside  
                                                         // this view, then refresh grid to reflect the changes...
         if( m_wKey == pVuHint->m_key ) 
            m_grid.Refresh();
         break;

      case CXVH_NAMOBJ :                                 // name of currently loaded obj may have changed; update 
      case CXVH_MOVOBJ :                                 // form's title to reflect change...
         if( m_wKey == pVuHint->m_key || 
             pVuHint->m_code == CXVH_MOVOBJ )
            UpdateCaption(NULL);
         break;

      case CXVH_CLRUSR :                                 // entire document reinitialized; reset form if a channel 
         if( m_wKey != CX_NULLOBJ_KEY )                  // config is currently loaded
            LoadChanCfg( CX_NULLOBJ_KEY );
         break;

      case CXVH_DELOBJ :                                 // if loaded channel config was deleted, reset form!
         if ( (m_wKey != CX_NULLOBJ_KEY) && 
              ((pVuHint->m_key==m_wKey) || (!pDoc->ObjExists(m_wKey))) )
            LoadChanCfg( CX_NULLOBJ_KEY );
         break;

      default :                                          // no response to any other hints...
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
BOOL CCxChannelForm::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) 
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
void CCxChannelForm::Dump( CDumpContext& dc ) const
{
   TVTabPane::Dump( dc ); 

   if ( m_wKey == CX_NULLOBJ_KEY )
      dc << "No channel configuration shown currently";
   else
   {
      dc << "Key of channel configuration = " << m_wKey;
      dc << "No. of channels with display flag set = " << m_nChDisplayed;
   }
}


//=== AssertValid [base override] ===================================================================================== 
//
//    Validate internal consistency of this view. 
//
//    ARGS:       NONE. 
//
//    RETURNS:    NONE. 
//
void CCxChannelForm::AssertValid() const
{
   TVTabPane::AssertValid();

   ASSERT( (m_wKey == CX_NULLOBJ_KEY && m_pChanCfg == NULL) ||
           (m_wKey != CX_NULLOBJ_KEY && m_pChanCfg != NULL) );
   ASSERT( m_nChDisplayed >= 0 && m_nChDisplayed <= CCntrlxApp::MaxTraces() );
}

#endif //_DEBUG




//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 

//=== LoadChanCfg ===================================================================================================== 
//
//    Load the specified CNTRLX channel configuration object onto the form's controls.  If no object specified, empty 
//    the form and disable controls.
//
//    ARGS:       key   -- [in] key of channel config object to load; if CX_NULLOBJ_KEY, empty the form.
//
//    RETURNS:    NONE.
//
VOID CCxChannelForm::LoadChanCfg( const WORD key )
{
   m_wKey = key;
   BOOL bEnable = TRUE;
   if( m_wKey == CX_NULLOBJ_KEY )                                    // no chan config loaded; ctrls will be disabled 
   {
      m_pChanCfg = NULL;
      m_nChDisplayed = 0;
      bEnable = FALSE;
   }
   else                                                              // obtain chan config record from document, and 
   {                                                                 // enable controls 
      m_pChanCfg = (CCxChannel*) GetDocument()->GetObject( m_wKey );
      ASSERT_KINDOF( CCxChannel, m_pChanCfg );

      m_nChDisplayed = m_pChanCfg->GetNDisplay();                    //    count # of channels selected for display
      if( m_nChDisplayed > CCntrlxApp::MaxTraces() )                 //    too many selected: deselect some...
      {
         int i = m_pChanCfg->GetNumChannels() - 1;
         while( m_nChDisplayed > CCntrlxApp::MaxTraces() )
         { 
            if( m_pChanCfg->IsDisplayed( i ) )
            {
               m_pChanCfg->ToggleDisplay( i );
               --m_nChDisplayed;
            }
            --i;
         }

         InformModify();                                             //    ...notify doc & views of change...

         CString str;                                                //    ...and warn user.
         str.Format( "%s %s.  %s", 
            _T("WARNING:  Too many channels selected for display in"),
            m_pChanCfg->Name(),
            _T("Some channels were turned off!") );
         ((CCntrlxApp*)AfxGetApp())->LogMessage( str );
      }

      int yMin, yMax;                                                //    stuff edit ctrls w/ y-axis range limits 
      m_pChanCfg->GetDispRange( yMin, yMax );
      m_edYMin.SetWindowText( yMin );
      m_edYMax.SetWindowText( yMax );
   }

   m_grid.Refresh();

   m_grid.EnableWindow( bEnable );                                   // enable/disable controls as appropriate...
   m_edYMin.EnableWindow( bEnable );
   m_edYMax.EnableWindow( bEnable );
   for( int id = IDC_CH_RESTOREDEF; id <= IDC_CH_ONEAXIS; id++ ) 
   {
      HWND hwnd;
      GetDlgItem( id, &hwnd ); 
      if ( hwnd != NULL ) ::EnableWindow( hwnd, bEnable );
   }

   UpdateCaption(NULL);                                              // update tab to reflect name of obj loaded 
}


//=== UpdateCaption [TVTabPane override] ============================================================================== 
//
//    Update the caption of the tab item associated with this tab pane.  If a NULL argument is provided, the method 
//    will use the name of the object currently loaded; if no obj is loaded, the placeholder title "Channels" is used.
//
//    ARGS:       szCaption   -- [in] desired caption for this tab. Use NULL to get automated caption.
//
//    RETURNS:    NONE.
//
VOID CCxChannelForm::UpdateCaption( LPCTSTR szCaption )
{
   CString strTitle;
   if( szCaption != NULL ) 
      strTitle = szCaption;
   else
      strTitle = (m_wKey!=CX_NULLOBJ_KEY) ? GetDocument()->GetObjName( m_wKey ) : _T("Channels");
   TVTabPane::UpdateCaption( strTitle );
}


//=== IsGridCellReadOnly ============================================================================================== 
//
//    Is specified cell in the channel configuration grid read-only?
// 
//    CCxChannel does not allow the user to change the record flag (col 1) for digital and computed channels (dig ch's 
//    are always recorded, while computed channels are never recorded), nor the gain (col 4) for digital channels. 
//    This routine also reports the row/col header cells and any invalid cell as read-only.
//
//    ARGS:       c  -- [in] row,col location of grid cell.
//
//    RETURNS:    FALSE if cell contains modifiable contents; TRUE otherwise.
//
BOOL CCxChannelForm::IsGridCellReadOnly( CCellID c ) const
{
   if( (!m_grid.IsValid( c )) || c.row == 0 || c.col == 0 || m_pChanCfg == NULL ) return( TRUE );

   if( (c.col == 1 || c.col == 4) && m_pChanCfg->IsDigital( c.row-1 ) ) return( TRUE );
   if( c.col == 1 && m_pChanCfg->IsComputed( c.row-1 ) ) return( TRUE );

   return( FALSE );
}


//=== GridDispCB ====================================================================================================== 
//
//    Callback function queried by the embedded grid control to obtain the contents of each cell in the grid.
//
//    Here we provide the string contents and, in some cases, the cell bkg color for each cell in our channel config 
//    grid, based on the CCxChannel object that's currently loaded (N = # of CNTRLX data channels available):
//       (1) Cell in the fixed row 0 ==> Label of attribute displayed in that column.
//       (2) Cell in the fixed col 0 ==> Descriptive label of corresponding channel, retrieved through a static member 
//                                       of CCxChannel.
//       (3) Cell in row 1-N, col 1  ==> State of the record ON/OFF flag.  Reads "ON" or "off".
//       (4) Cell in row 1-N, col 2  ==> State of the display ON/OFF flag.  Reads "ON" or "off".
//       (5) Cell in row 1-N, col 3  ==> Display offset value.  An integer value, range restricted by CCxChannel.
//       (6) Cell in row 1-N, col 4  ==> Display gain, shown as a power of 2.  Range restricted by CCxChannel.
//       (7) Cell in row 1-N, col 5  ==> Display trace color.  Empty string.  Cell's bkg color reflects the current 
//                                       trace color assigned to the corresponding channel.
//
//    What if no channel configuration object is currently loaded?  In that case, the row & column labels appear as 
//    usual, but the channel attribute cells are all assigned empty strings and a bkg color that is the same as the 
//    bkg color of the fixed row/col cells.  The idea here is to make the grid look disabled.
//
//    For the digital input channels, the gain and record flags are always "0" and "ON", respectively.  For the 
//    computed channels, the record flag is always "OFF".  The user cannot change any of these attributes.  To 
//    emphasize this, we paint the bkg of these cells in the bkg color of the fixed row/col cells.  When a predefined, 
//    read-only channel config object is loaded, all cells (except the trace color cells) are painted this way.
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
BOOL CALLBACK CCxChannelForm::GridDispCB( GV_DISPINFO *pDispInfo, LPARAM lParam ) 
{
   CCxChannelForm* pThisView = (CCxChannelForm*)lParam;                    // to access non-static data!
   CCxChannel* pChanCfg = pThisView->m_pChanCfg;                           // the currently loaded channel config obj 
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c( pDispInfo->item.row, pDispInfo->item.col );                  // the cell to be displayed

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 

   if( pDispInfo->item.nState & GVIS_VIRTUALLABELTIP )                     // we don't use label tips on this grid
   { 
      pDispInfo->item.nState &= ~GVIS_VIRTUALLABELTIP; 
      return( TRUE );
   }

   if( (c.row == 0) && (c.col == 0) )                                      // the top left corner has no text
      pDispInfo->item.strText = _T(" ");
   else if ( c.col == 0 )
      pDispInfo->item.strText = CCxChannel::GetLabel( c.row - 1 );         // descriptive channel label in first col
   else if ( c.row == 0 )
   {
      switch( c.col )                                                      // attribute label in first row
      {
         case 1 : pDispInfo->item.strText = _T("Record?"); break;
         case 2 : pDispInfo->item.strText = _T("Display?"); break;
         case 3 : pDispInfo->item.strText = _T("Offset (mV)"); break;
         case 4 : pDispInfo->item.strText = _T("Multiplier"); break;
         case 5 : pDispInfo->item.strText = _T("Trace Color"); break;
         default: ASSERT( FALSE ); break;
      }
   }
   else                                                                    // otherwise, we're filling in an attribute 
   {                                                                       // cell...
      if( pChanCfg == NULL )                                               //    if no channel config loaded, all cells 
      {                                                                    //    are blank, with same bkg color as a 
         pDispInfo->item.strText = _T(" ");                                //    fixed cell!!
         pDispInfo->item.crBkClr = pGrid->GetFixedBkColor();
      }
      else                                                                 //    channel config obj is loaded, so 
      {                                                                    //    we query it for current contents...
         int nCh = c.row - 1;
         switch( c.col )
         {
            case 1 :
               pDispInfo->item.strText = 
                  pChanCfg->IsRecorded( nCh ) ? _T("ON") : _T("off");
               break;
            case 2 :
               pDispInfo->item.strText = 
                  pChanCfg->IsDisplayed( nCh ) ? _T("ON") : _T("off");
               break;
            case 3 :
               pDispInfo->item.strText.Format( "%d", 
                  pChanCfg->GetOffset( nCh ) );
               break;
            case 4 :
               pDispInfo->item.strText.Format( "%d", 
                  pChanCfg->GetGain( nCh ) );
               break;
            case 5 :
               pDispInfo->item.strText = _T(" ");
               pDispInfo->item.crBkClr = pChanCfg->GetColor( nCh );
               break;
         }

         if( (pChanCfg->IsDigital( nCh ) && (c.col==1 || c.col==4)) ||     // for these cells, use fixed cell bkg color 
             (pChanCfg->IsComputed( nCh ) && (c.col==1)) )                 // as a clue that these cannot be modified.
         { 
            CGridCellBase* pFixed = pGrid->GetDefaultCell( TRUE, TRUE );
            pDispInfo->item.crBkClr = pFixed->GetBackClr();
         }
      }
   }

   pDispInfo->item.nState &= ~GVIS_VIRTUALTITLETIP;                        // show title tip only if text does not fit 
   return( TRUE );
}


//=== GridEditCB ====================================================================================================== 
//
//    Callback invoked to initiate inplace editing of a cell on the channel configuration grid, or to increment/decr 
//    the contents of a cell in response to a left or right mouse click.  Below is a summary of the possible operations 
//    that this callback permits:
//
//       1) Cell in row 0 or col 0 ==> These are merely read-only row or col labels.  Cannot be edited.
//       2) Cell in row 1-N, col 1 ==> State of the record ON/OFF flag for data channel N-1.  If the parameter can be 
//          modified (for some channels it is read-only), it is treated as a multichoice parameter.  A left or right 
//          mouse click will toggle its state.
//       3) Cell in row 1-N, col 2 ==> State of the display ON/OFF flag for channel N-1.  Treated like the record flag, 
//          but note that CCxChannelForm restricts the total # of displayed channels to a maximum value.
//       4) Cell in row 1-N, col 3  ==> Display offset for channel N-1.  Treated as an integer text string for inplace 
//          editing.  A left(right) click will increment(decrement) the offset by 500mV.
//       5) Cell in row 1-N, col 4  ==> Display gain, shown as a power of 2.  If the parameter can be modified (for 
//          the digital channels it is read-only), it is treated as a multichoice parameter.  A left(right) click will 
//          increment(decrement) the gain.
//       6) Cell in row 1-N, col 5  ==> Display trace color, a multiple-choice parameter.  Since the inplace editing 
//          facilities of the grid do not support different bkgs for each item, we obtain the name of each available 
//          color from CCxChannel.  A left(right) click will increment(decrement) the zero-based color index.
//
//    NOTE:  See also GridDispCB().
//
//    ARGS:       pEI      -- [in/out] ptr to a struct we need to fill with the required edit info. 
//                lParam   -- [in] THIS (see NOTE)
// 
//    RETURNS:    TRUE if successful, FALSE otherwise (invalid cell, e.g.)
//
BOOL CALLBACK CCxChannelForm::GridEditCB( EDITINFO *pEI, LPARAM lParam )
{
   CCxChannelForm* pThisView = (CCxChannelForm*)lParam;                    // to access non-static data!
   CCxChannel* pChanCfg = pThisView->m_pChanCfg;                           // the currently loaded channel config obj 
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c = pEI->cell;                                                  // the cell to be edited

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 

   int nCh = c.row - 1;                                                    // channel corresponding to specified cell

   if( pThisView->IsGridCellReadOnly( c ) )                                // if cell is readonly, then there is 
   {                                                                       // nothing to edit!!
      pEI->iClick = 0; 
      pEI->iType = LG_READONLY;
      return( TRUE );
   }

   switch( c.col )
   {
      case 1 :                                                             // record flag:
         if( pEI->iClick != 0 )                                            //    left or right click toggles state
            pChanCfg->ToggleRecord( nCh ); 
         else                                                              //    edited as multichoice parameter with 
         {                                                                 //    two possible values, ON or off
            pEI->iType = LG_MULTICHOICE;
            pEI->strArChoices.Add( _T("off") );
            pEI->strArChoices.Add( _T("ON") );
            pEI->iCurrent = pChanCfg->IsRecorded( nCh ) ? 1 : 0;
         }
         break;

      case 2 :                                                             // display flag
         if( pEI->iClick != 0 )                                            //    left or right click toggles state
         {                                                                 //    however, if max #ch's are selected for 
            if( (pThisView->m_nChDisplayed == CCntrlxApp::MaxTraces()) &&  //    display, and user is trying to select 
                !pChanCfg->IsDisplayed( nCh ) )                            //    another -- prevent it
            {
               ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("No more channels may be selected for display!") );
               pEI->iClick = 0;
            }
            else                                                           //    otherwise, toggle ch's display flag 
            {                                                              //    and incr/decr # of channels currently 
               if( pChanCfg->ToggleDisplay( nCh ) )                        //    selected for display
                  ++(pThisView->m_nChDisplayed); 
               else
                  --(pThisView->m_nChDisplayed);
            }
         }
         else                                                              //    edited as multichoice parameter with 
         {                                                                 //    two possible values, ON or off
            pEI->iType = LG_MULTICHOICE;
            pEI->strArChoices.Add( _T("off") );
            pEI->strArChoices.Add( _T("ON") );
            pEI->iCurrent = pChanCfg->IsDisplayed( nCh ) ? 1 : 0;
         }
         break;

      case 3 :                                                             // display offset in mV
         if( pEI->iClick != 0 )                                            //    left/right click increments/decrements 
         {                                                                 //    the offset by 500mV
            int iOff = pChanCfg->GetOffset( nCh );
            iOff += (pEI->iClick > 0) ? 500 : -500;
            pChanCfg->SetOffset( nCh, iOff );
         }
         else                                                              //    edited as an integer text string
         {
            pEI->iType = LG_NUMSTR;
            pEI->numFmt.flags = NES_INTONLY;
            pEI->numFmt.nLen = 6;
            pEI->numFmt.nPre = 1;
            pEI->dCurrent = double(pChanCfg->GetOffset( nCh ));
         }
         break;

      case 4 :                                                             // display gain:
         if( pEI->iClick > 0 )                                             //    left click increments gain index
            pChanCfg->IncrGain( nCh ); 
         else if( pEI->iClick < 0 )                                        //    right click decrements gain index
            pChanCfg->DecrGain( nCh );
         else                                                              //    edited as multichoice parameter with 
         {                                                                 //    a range of possible values
            pEI->iType = LG_MULTICHOICE;
            int iGMin = pChanCfg->GetGainMin();
            int iGMax = pChanCfg->GetGainMax();
            CString str;
            for( int i = iGMin; i <= iGMax; i++ )
            {
               str.Format( "%d", i );
               pEI->strArChoices.Add( str );
            }
            pEI->iCurrent = pChanCfg->GetGainIndex( nCh );                 //    zero-based index of current value!!
         }
         break;

      case 5 :                                                             // display color:
         if( pEI->iClick > 0 )                                             //    left click increments color index
            pChanCfg->IncrColor( nCh ); 
         else if( pEI->iClick < 0 )                                        //    right click decrements gain index
            pChanCfg->DecrColor( nCh );
         else                                                              //    edited as multichoice parameter with 
         {                                                                 //    a range of possible values -- we 
            pEI->iType = LG_MULTICHOICE;                                   //    display descriptive name of each 
            for( int i = 0; i < pChanCfg->GetNumTraceColors(); i++ )       //    available trace color 
               pEI->strArChoices.Add( pChanCfg->GetTraceColorLabel( i ) );
            pEI->iCurrent = pChanCfg->GetColorIndex( nCh );                //    zero-based index of current color!!
         }
         break;

      default :                                                            // we should NEVER get here!
         ASSERT( FALSE );
         break;
   }

   if( pEI->iClick != 0 )                                                  // if a mouse click has modified chan cfg, 
      pThisView->InformModify();                                           // inform doc/view framework

   return( TRUE );
}


//=== GridEndEditCB =================================================================================================== 
//
//    Callback invoked upon termination of inplace editing of the channel configuration grid.
//
//    Here we update the loaded channel configuration object IAW the change made during the inplace operation that was 
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
BOOL CALLBACK CCxChannelForm::GridEndEditCB( ENDEDITINFO *pEEI, LPARAM lParam )
{
   if( pEEI->nExitChar == VK_ESCAPE ) return( TRUE );                      // inplace operation was cancelled

   CCxChannelForm* pThisView = (CCxChannelForm*)lParam;                    // to access non-static data!
   CCxChannel* pChanCfg = pThisView->m_pChanCfg;                           // the currently loaded channel config obj 
   CLiteGrid* pGrid = &(pThisView->m_grid);                                // the grid control
   CCellID c = pEEI->cell;                                                 // the cell that was edited

   if( pGrid->GetSafeHwnd() == NULL || !pGrid->IsValid( c ) )              // if grid gone, or cell not valid, ignore!
      return( FALSE ); 

   int nCh = c.row - 1;                                                    // channel corresponding to specified cell

   if( pThisView->IsGridCellReadOnly( c ) )                                // if cell is readonly, then there is 
   {                                                                       // nothing to edit!!
      ASSERT( FALSE );                                                     //    this should NEVER happen!
      pEEI->nExitChar = VK_ESCAPE;                                         //    prevent continued inplace editing
      pEEI->bNoRedraw = TRUE;                                              //    no need to redraw since no change made 
      return( TRUE );
   }

   if( pEEI->bIsChanged )                                                  // if the user made a change...
   {
      BOOL bChanged = TRUE;
      switch( c.col )
      {
         case 1 :                                                          // record flag (2-choice): toggle state
            pChanCfg->ToggleRecord( nCh ); 
            break;

         case 2 :                                                          // display flag (2-choice): toggle state, 
            if( (pThisView->m_nChDisplayed == CCntrlxApp::MaxTraces()) &&  //    unless doing so would turn on too 
                !pChanCfg->IsDisplayed( nCh ) )                            //    many channels! 
            {
               ((CCntrlxApp*)AfxGetApp())->LogMessage( _T("No more channels may be selected for display!") );
               bChanged = FALSE;
            }
            else
            {
               if( pChanCfg->ToggleDisplay( nCh ) )                        //    if we do toggle display flag, update  
                  ++(pThisView->m_nChDisplayed);                           //    the total #ch currently displayed 
               else
                  --(pThisView->m_nChDisplayed);
            }
            break;

         case 3 :                                                          // display offset (numeric):  set new value,  
            pChanCfg->SetOffset( nCh, int(pEEI->dNew) );                   //    which is autocorrected by chan cfg obj 
            break;

         case 4 :                                                          // display gain (multi-choice):  set new  
            pChanCfg->SetGainIndex( nCh, int(pEEI->dwNew) );               //    gain IAW user's choice
            break;

         case 5 :                                                          // display color (multi-choice);  set new 
            pChanCfg->SetColorIndex( nCh, int(pEEI->dwNew) );              //    color IAW user's choice 
            break;

         default :                                                         // we should NEVER get here!
            break;
      }

      if( bChanged ) pThisView->InformModify();                            // inform doc/view if we made a change
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


//=== InformModify ==================================================================================================== 
//
//    Invoke this method to inform the CNTRLX experiment document (CCxDoc) and other attached views that the currently 
//    loaded channel configuration object was just modified.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxChannelForm::InformModify()
{
   ASSERT( m_wKey != CX_NULLOBJ_KEY );
   CCxDoc* pDoc = GetDocument(); 
   pDoc->SetModifiedFlag();
   CCxViewHint vuHint( CXVH_MODOBJ, CX_CHANCFG, m_wKey );
   pDoc->UpdateAllViews( this, LPARAM(0), (CObject*)&vuHint );
}
