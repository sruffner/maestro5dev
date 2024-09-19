//===================================================================================================================== 
//
// xyplotbar.cpp : Implementation of class CXYPlotBar, a generic control bar class that displays up to 10 data points 
//                 on an X-Y plane.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// CXYPlotBar implements a resizable, dockable and floatable control bar that displays up to 10 discrete data points, 
// or "symbols", on an XY Cartesian plot that fills the control bar's client area.  Each symbol is represented by one 
// of several simple shapes (box, filled box, filled circle, "X", "+", or a vertical line) of a selected color.  The 
// origin of the logical coordinate system is always at the center of the client rect, and the y-axis increases upwards 
// -- as is typical of a Cartesian plot.  Symbol locations can be updated at frequent intervals to create a radar-like 
// view in which the defined symbols move (relatively) smoothly over the plot display.
//
// The background of the plot is always black, the axes are white (med gray if plot is inactivated), and 8 tick marks 
// are evenly distributed along each axis.  Axis and tick mark labels are not available.  The plot is drawn in the 
// MM_ISOTROPIC mapping mode, so that 1 logical unit on the x-axis = 1 logical unit on the y-axis (so a square that is 
// 5 logical units wide looks as square as possible when drawn in "device" units -- pixels!).  Tick marks and symbols 
// are scaled to ~1/50th of the current logical extent of the plot.  Symbols are drawn to fit into a square box.
//
// ==> Support for a "show/hide" context menu.
// CXYPlotBar can be optionally configured to display a right-click context menu which allows the user to toggle the 
// visibility state of any of the currently defined symbols.  A short name is associated with each symbol when it is 
// created.  This name is displayed as an item in the context menu when invoked.  If the corresponding symbol is 
// currently displayed, then that item is checked; else it is unchecked.  Selecting an item toggles the symbol's 
// visibility.
//
// ==> Summary of key public methods.
//    Activate() -- To activate/deactivate plot.  When inactive, the axes are grayed and all symbols are hidden.  
//       Symbols can still be added, deleted, moved, etc. -- all such actions will be invisible to user, however.
//    EnableShowHide() -- Allow user to toggle visibility state of symbols via a right-click context menu.
//    AddSymbol() -- To add a data point symbol to the plot.  Color, shape, and symbol name are specified here and 
//       cannot be changed.  Returns index of new symbol, which must be used to reference the symbol elsewhere.
//    DeleteSymbol() -- To remove a data point symbol.  The defined symbol array is adjusted accordingly -- so be 
//       careful!!
//    ShowSymbol() -- To show/hide a symbol.
//    MoveSymbol() -- To move a symbol to a new (x,y) position.
//    UpdateSymbols() -- To update the positions of all defined symbols in one go.  Recommended for animation of 
//       moving "targets".
//    SetLogicalExtent() -- Use this to set the logical extent of the x- and y-axes.
//    GetCursorLogicalPos() -- Whenever the mouse cursor is within the bounds of the client area, this method returns 
//       the cursor's logical coordinates.
//
// DEVELOPMENT NOTES:
// 1) Because the symbols and axes are very simple, we do not find it necessary to use a memory device context to 
// reduce flicker (if the client area is made very large, the memory cost of such a DC outweighs the potential 
// flicker reduction).  UpdateSymbols() has been designed to do as little drawing as possible to update the positions 
// of the defined symbols.
//
// 2) I found it a bit tricky to make the XHAIR and TEE symbols look as square as possible -- has to do with the 
// logical-to-pixel translations that go on in the device context.  See DrawSymbol() for the details.  Originally, 
// EraseSymbol() worked by drawing a filled black rectangle over the symbol's current position.  However, that failed 
// to entirely erase XHAIR's and TEE's because of the tricks used to make these symbols look square.  Thus, I 
// implemented EraseSymbol() merely by drawing the symbol with black -- again, see DrawSymbol().
//
// 3) Possible improvement:  Use bitmaps for the symbols.  Could support, say, 3x3-, 9x9-, and 15x15-pixel bitmaps; 
// which size is used would depend on the current size of the client area.  This would require quite a bit of work, 
// however...
//
// 4) I originally had implemented a "hollow circle" symbol, but had to get rid of it due to a bug in the Win32 GDI: 
// when the current brush is HOLLOW_BRUSH and the Ellipse() function is called, the device context is somehow lost, 
// and the hollow circle gets drawn on the "screen" device context -- even if the control bar window is hidden!  This 
// is definitely specific to the Ellipse() function, as I am able to draw a hollow rectangle without this problem.
//
//
// CREDITS:
// 1) CSizingControlBar: resizable control bars, by Cristi Posea [08/03/2000, www.datamekanix.com/sizecbar] --  An 
// implementation of DevStudio-like docking windows that can contain dialogs, controls, even views (although not 
// recommended.  CXYPlotBar is derived from this extension of MFC's CControlBar.
//
//
// REVISION HISTORY:
// 09feb2001-- Initial development begun.
// 15feb2001-- Completed testing of initial version.  Began work on support for a "show/hide" context menu.
// 20feb2001-- Finished tweaking support for the "show/hide" context menu.
// 24jan2003-- Tweaked GetSymbolRectFromPt().
// 15jun2005-- Added GetCursorLogicalPos().
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "xyplotbar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CXYPlotBar
//
//===================================================================================================================== 
//===================================================================================================================== 
//
BEGIN_MESSAGE_MAP( CXYPlotBar, CSizingControlBarCF )
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CXYPlotBar [constructor] ======================================================================================== 
//
//    The XY plot control bar is set up in the inactive state, with no symbols currently defined.
//
CXYPlotBar::CXYPlotBar()
{
   m_bActive = FALSE;
   m_bEnableShowHide = FALSE;
   m_logExtent.cx = 100;
   m_logExtent.cy = 100;
   m_iSymWidth = 2;
   m_nDefined = 0;
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnPaint [base override] ========================================================================================= 
//
//    Response to WM_PAINT message.  Here's where we do all the actual drawing of the x- and y-axes and the currently 
//    visible symbols.
//
//    For now, we always respond to WM_PAINT by erasing the entire client area (see OnEraseBkgnd()) and redrawing 
//    everything.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CXYPlotBar::OnPaint()
{
   CPaintDC dc( this );

   SetupCoords( &dc );
   DrawAxes( &dc );
   if( m_bActive ) DrawSymbol( &dc, -1 );          // draws all defined symbols
}


//=== OnEraseBkgnd [base override] ==================================================================================== 
//
//    Response to the WM_ERASEBKGND message.  The default processing of this message erases the background using the 
//    "class background brush".  However, we want the background to always be black.  So we override this message 
//    handler and erase the background ourselves.
//
//    DEV NOTE:  The current clip rect may not be the entire client area.  For greater efficiency, OnPaint() and this 
//    handler could be designed to draw only on the current clip rect.
//
//    ARGS:       pDC   -- [in] ptr to device context 
//
//    RETURNS:    TRUE to indicate background was erased; FALSE 
//
BOOL CXYPlotBar::OnEraseBkgnd( CDC *pDC )
{
   CRect rect;
   GetClientRect( rect );
   pDC->FillSolidRect( rect, RGB(0, 0, 0) );
   return( TRUE );
}


//=== OnContextMenu [base override] =================================================================================== 
//
//    Response to the WM_CONTEXTMENU message, sent when the user right-clicks on the control bar.  If the show/hide 
//    menu feature is enabled, we display a popup menu listing the names of all defined symbols.  Each visible symbol 
//    is checked, while hidden symbols are unchecked.
//
//    Rather than sending the user's selection as a WM_COMMAND message, we handle it here by toggling the visibility 
//    state of the symbol selected.
//
//    NOTE:  The MENUIDOFFSET is required because TrackPopupMenu() returns 0 when the user cancels.  We assign the 
//    symbols to the menu items in array order, [0..N-1] + MENUIDOFFSET.
//
//    ARGS:       pWnd  -- [in] handle to window in which user right-clicked the mouse.
//                pos   -- [in] position of mouse, in screen coords, at time of right-click. 
//
//    RETURNS:    NONE 
//
void CXYPlotBar::OnContextMenu( CWnd* pWnd, CPoint pos )
{
   if( !m_bEnableShowHide || (m_popupMenu.GetSafeHmenu() == NULL) ||    // if feature disabled, or there is no context 
       (m_nDefined == 0) )                                              // menu available, pass to base class
   {
      CSizingControlBarCF::OnContextMenu( pWnd, pos );
      return;
   }

   ASSERT( m_popupMenu.GetMenuItemCount() == (UINT) m_nDefined );

   for( int i = 0; i < m_nDefined; i++ )                                // check items corres. to visible symbols
      m_popupMenu.CheckMenuItem( 
         (UINT)(i+MENUIDOFFSET), 
         (m_symbols[i].bShow) ? MF_CHECKED : MF_UNCHECKED );

   int chosen = (int) m_popupMenu.TrackPopupMenu( 
      TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | 
      TPM_RETURNCMD,                                                    // so TPM() returns cmd ID of item selected
      pos.x, pos.y, this );

   chosen -= MENUIDOFFSET;                                              // if an item was selected, toggle its 
   if( (chosen >= 0) && (chosen < m_nDefined) )                         // visibility state
      ShowSymbol( chosen, !m_symbols[chosen].bShow );
}


//===================================================================================================================== 
// OPERATIONS/ATTRIBUTES
//===================================================================================================================== 

//=== Activate ======================================================================================================== 
//
//    Activate or deactivate the plot display.  When inactive, only the axes are drawn in gray.  When active, the axes 
//    are drawn in white, and all defined symbols are drawn IAW their current attributes.
//
//    ARGS:       bOn   -- [in] TRUE to activate (default), FALSE to deactivate plot displayed in this control bar. 
//
//    RETURNS:    NONE.  
//
VOID CXYPlotBar::Activate( const BOOL bOn /* = TRUE */ )
{
   if( bOn == m_bActive ) return;

   m_bActive = !m_bActive; 
   Invalidate( TRUE );              // if changing state, we redraw the entire client area 
}


//=== EnableShowHide ================================================================================================== 
//
//    Enable/disable the right-click context menu which allows the user to interactively toggle the visibility state of 
//    any of the currently defined symbols.
//
//    ARGS:       bEna   -- [in] TRUE to enable (default), FALSE to disable context menu. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise (unable to create the popup menu).  
//
BOOL CXYPlotBar::EnableShowHide( const BOOL bEna /* =TRUE */ )
{
   if( bEna == m_bEnableShowHide ) return( TRUE );                   // no change!

   m_bEnableShowHide = !!bEna;
   if( m_bEnableShowHide )                                           // enabling context menu -- make sure it exists
   {
      if( (m_nDefined > 0) && (m_popupMenu.GetSafeHmenu() == NULL) ) // there was a problem creating menu, try again;
         UpdateMenuPopup();                                          // if unsuccessful, feature is auto-disabled..
      return( m_bEnableShowHide );
   }
   else
      return( TRUE );                                                // disabling context menu never fails
}


//=== SetLogicalExtent ================================================================================================ 
//
//    Change the current logical extent of plot window.  Caller should provide the width and height of the plot area in 
//    "logical coordinates", both of which should be positive numbers.  The origin of the logical coordinate system  
//    is always the center of the client area, and the y-axis increases upwards.
//
//    Logical width and height are each restricted to [100..32767].  Symbol width is 1/50th of the smaller dimension.
//
//    ARGS:       newSize  -- [in] the new logical extent requested. 
//
//    RETURNS:    TRUE if successful; FALSE otherwise.  
//
BOOL CXYPlotBar::SetLogicalExtent( const SIZE newSize )
{
   if( m_logExtent == newSize ) return( TRUE );                         // no change!

   if( newSize.cx < 100 || newSize.cx > 32767  ||                       // some constraints...
       newSize.cy < 100 || newSize.cy > 32767 )
      return( FALSE );

   m_logExtent = newSize;
   m_iSymWidth = (newSize.cx < newSize.cy) ? newSize.cx : newSize.cy;
   m_iSymWidth /= 50;

   Invalidate( TRUE );                                                  // changing extent requires a complete redraw 
   return( TRUE );
}


//=== AddSymbol ======================================================================================================= 
//
//    Add a new plot symbol with specified shape, color, and identifying name.  The symbol is initially positioned at 
//    the origin, but it is hidden -- so calling this method has no effect on the current appearance of the XY plot.
//
//    ARGS:       symShape -- [in] requested symbol shape. 
//                color    -- [in] requested symbol color (RGB). 
//                str      -- [in] name assigned to symbol.  truncated to no more than 10 chars.  if NULL, a default 
//                            name is provided. 
//
//    RETURNS:    -1 if unsuccessful (all available symbols in use); otherwise, array index of new symbol.  
//
int CXYPlotBar::AddSymbol( SymbolShape symShape, const COLORREF color, LPCTSTR str /* =NULL */ )
{
   if( m_nDefined == MAXSYMBOLS ) return( -1 );                      // no room!

   int newIndex = m_nDefined;
   ++m_nDefined;

   m_symbols[newIndex].shape = symShape;
   m_symbols[newIndex].color = color;
   m_symbols[newIndex].ptLoc = CPoint(0,0);
   m_symbols[newIndex].bShow = FALSE;

   CString* pStr = &(m_symbols[newIndex].strName);
   if( str == NULL )                                                 // if no name provided, give default one
      pStr->Format( "Symbol %d", newIndex ); 
   else
   {
      *pStr = str;
      if( pStr->GetLength() > 10 )
         pStr->Format( "%.*s", 10, str );
   }

   UpdateMenuPopup( newIndex, TRUE );                                // update context menu to include new symbol
   return( newIndex );
}


//=== DeleteSymbol ==================================================================================================== 
//
//    Remove an existing symbol from the XY plot display.  If the symbol is currently visible, it is erased.  Also, the 
//    current symbol array is compacted, so caller must compensate accordingly!
//
//    ARGS:       iSym  -- [in] array index of the symbol to delete. 
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad index).  
//
BOOL CXYPlotBar::DeleteSymbol( const int iSym )
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );

   if( m_symbols[iSym].bShow ) ShowSymbol( iSym, FALSE );

   for( int i = iSym + 1; i < m_nDefined; i++ )
   {
      m_symbols[i-1].shape = m_symbols[i].shape;
      m_symbols[i-1].color = m_symbols[i].color;
      m_symbols[i-1].ptLoc = m_symbols[i].ptLoc;
      m_symbols[i-1].bShow = m_symbols[i].bShow;
   }
   --m_nDefined;

   UpdateMenuPopup( iSym, FALSE );                                   // remove deleted symbol from context menu
   return( TRUE );
}


//=== ShowSymbol ====================================================================================================== 
//
//    Show/hide the specified symbol.  If the XY plot is active, its visible state is updated immediately (without 
//    generating a WM_PAINT message). 
//
//    ARGS:       iSym  -- [in] array index of the symbol to show/hide. 
//                bShow -- [in] TRUE to show the symbol at its current location; FALSE to hide it. 
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad index).  
//
BOOL CXYPlotBar::ShowSymbol( const int iSym, const BOOL bShow )
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );      // invalid argument!

   if( m_symbols[iSym].bShow == bShow ) return( TRUE );           // no change!

   if( !m_bActive  )                                              // if plot is inactive, just update show state --
   {                                                              //    there's no drawing to do!
      m_symbols[iSym].bShow = bShow;
      return( TRUE );
   }

   CClientDC dc(this);                                            // prepare to draw in client area 
   SetupCoords( &dc );

   if( bShow )                                                    // if turning symbol on, all we need to do is draw it 
   {
      DrawSymbol( &dc, iSym ); 
      m_symbols[iSym].bShow = TRUE;
   }
   else                                                           // when turning symbol off, erasing it can affect 
   {                                                              // other plot elements.  so we redraw the axes and 
      BOOL bErased = EraseSymbol( &dc, iSym );                    // remaining visible symbols if we did erase...
      m_symbols[iSym].bShow = FALSE;
      if( bErased )
      {
         DrawAxes( &dc );
         DrawSymbol( &dc, -1 );
      }
   }

   return( TRUE );
}


//=== MoveSymbol ====================================================================================================== 
//
//    Move an existing symbol to a new location on the XY plot.  If the plot is active, the symbol is erased at its old 
//    location and redrawn at its new one. 
//
//    ARGS:       iSym  -- [in] array index of the symbol to be moved. 
//                pt    -- [in] new location of symbol (in logical coordinates).
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad index).  
//
BOOL CXYPlotBar::MoveSymbol( const int iSym, const CPoint pt )
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );                  // invalid argument!

   if( m_symbols[iSym].ptLoc == pt ) return( TRUE );                          // no change!

   POINT pptNew[MAXSYMBOLS];                                                  // move just one symbol...
   for( int i = 0; i < m_nDefined; i++ ) pptNew[i] = m_symbols[i].ptLoc;
   pptNew[iSym] = pt;
   UpdateSymbols( pptNew );

   return( TRUE );
}


//=== UpdateSymbols =================================================================================================== 
//
//    Update the locations of all defined symbols.
//
//    This method is designed for animation of moving symbols (like targets moving on a radar screen).  It TRIES to be 
//    efficient about what is redrawn:  the axes are redrawn only when they get partially erased; only symbols which 
//    actually move are erased at their old locations; and all *visible* symbols are redrawn (rather than a lot of
//    complicated checking for overlaps with other symbols).
//
//    ARGS:       pptNew   -- [in] array of POINT's containing the new locations of all defined symbols.  caller is 
//                            responsible for ensuring this array is sized correctly!!
//
//    RETURNS:    NONE 
//
VOID CXYPlotBar::UpdateSymbols( LPPOINT pptNew ) 
{
   int i;

   if( m_nDefined == 0 ) return;                                        // there are no symbols to move!

   if( !m_bActive )                                                     // plot off, so update internal state only!
   {
      for( i = 0; i < m_nDefined; i++ ) m_symbols[i].ptLoc = pptNew[i];
      return;
   }

   CClientDC dc( this );                                                // get client area device context 
   SetupCoords( &dc );                                                  // prepare it for drawing in logical coords

   BOOL bDrawX = FALSE;                                                 // = TRUE if horiz axis should be redrawn
   BOOL bDrawY = FALSE;                                                 // = TRUE if vert axis should be redrawn

   for( i = 0; i < m_nDefined; i++ )                                    // erase any symbol moving to a new location... 
   { 
      BOOL bErase = (m_symbols[i].ptLoc != pptNew[i]);
      if( bErase ) bErase = EraseSymbol( &dc, i );                      // symbol is erased only if necessary
      if( bErase )                                                      // if symbol was erased, did its erasure affect 
      {                                                                 // either axis?
         if( !bDrawX ) bDrawX = OverlapsXAxis( i );
         if( !bDrawY ) bDrawY = OverlapsYAxis( i );
      }
   }

   DrawAxes( &dc, bDrawX, bDrawY );                                     // redraw axes if needed 

   for( i = 0; i < m_nDefined; i++ ) m_symbols[i].ptLoc = pptNew[i];    // move symbols to their new locations 
       
   DrawSymbol( &dc, -1 );                                               // redraw all symbols still visible after move
}


//=== GetCursorLogicalPos ============================================================================================= 
//
//    Get current position of the mouse cursor in *logical* coords, but ONLY IF the mouse cursor is inside the client 
//    area.
//
//    [This method was added so that we Maestro can make a visual target track the cursor's position.]
//
//    ARGS:       pt -- [in/out] If function returns TRUE, then this will contain the cursor's position in logical
//                      coordinates.  Undefined if function returns FALSE.
//
//    RETURNS:    TRUE if mouse cursor is within client area of this CXYPlotBar; FALSE otherwise.
//
BOOL CXYPlotBar::GetCursorLogicalPos( CPoint& pt )
{
   POINT mousePt;                                                    // get mouse cursor pos in screen coords
   if( !::GetCursorPos( &mousePt ) ) return( FALSE );

   ScreenToClient( &mousePt );                                       // convert to client coords

   CRect rClient;                                                    // get client rect in client coords
   GetClientRect( &rClient );

   BOOL bInside = rClient.PtInRect( mousePt );                       // if cursor within client area, compute its 
   if( bInside )                                                     // pos in logical coords
   {
      CClientDC dc( this ); 
      SetupCoords( &dc ); 
      dc.DPtoLP( &mousePt );
      pt.x = mousePt.x;
      pt.y = mousePt.y;
   }
   return( bInside );
}


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== SetupCoords ===================================================================================================== 
//
//    Set up logical-to-device translations for the device context such that:  the current logical extent of the XY 
//    plot is registered with device context, the logical origin is at the center of the client rect, the y-axis 
//    increases upwards rather than downwards, and aspect ration is preserved (MM_ISOTROPIC mode).
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//
//    RETURNS:    NONE 
//
VOID CXYPlotBar::SetupCoords( CDC* pDC )
{
   CRect rClient;
   GetClientRect( &rClient );

   pDC->SetMapMode( MM_ISOTROPIC );
   pDC->SetWindowExt( m_logExtent );                              // in MM_ISOTROPIC mode, must call this first!
   pDC->SetViewportExt( rClient.right, -rClient.bottom );
   pDC->SetViewportOrg( rClient.right / 2, rClient.bottom / 2 );
}


//=== DrawAxes ======================================================================================================== 
//
//    Draw one or both axes of the XY plot using the provided device context.  Each axis includes 8 tick marks, evenly 
//    spaced along their length.  When the plot is activated, the axes are drawn in white.  When not, they are drawn in 
//    medium gray.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//                bDrawX-- [in] if TRUE, x-axis is drawn [default is TRUE]. 
//                bDrawY-- [in] if TRUE, y-axis is drawn [default is TRUE]. 
//
//    RETURNS:    NONE 
//
VOID CXYPlotBar::DrawAxes( CDC* pDC, const BOOL bDrawX /* = TRUE */, const BOOL bDrawY /* = TRUE */ )
{
   if( !bDrawX && !bDrawY ) return;                                  // nothing to do!

   CPen* pOldPen;                                                    // use stock white pen when plot active, med gray 
   CPen grayPen;                                                     // when inactive... 
   BOOL bGrayPenCreated = FALSE;
   if( m_bActive ) 
      pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );
   else                                                              // ...if we're unable to create the custom pen, 
   {                                                                 // then fall back on the stock white pen!!!
      if( !grayPen.CreatePen( PS_SOLID, 0, RGB(128,128,128) ) ) 
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN ); 
      else
      {
         pOldPen = (CPen*) pDC->SelectObject( &grayPen );
         bGrayPenCreated = TRUE;
      }
   }

   int halfTick = m_iSymWidth/2;                                     // axis tick marks dimensioned acc to symbol size 

   int i, j;
   if( bDrawX )                                                      // draw horizontal axis w/ 8 tick marks
   {
      pDC->MoveTo( -m_logExtent.cx / 2, 0 ); 
      pDC->LineTo(  m_logExtent.cx / 2, 0 );
      for( i = 1; i < 5; i++ ) 
      {
         j = (i * m_logExtent.cx) / 10;
         pDC->MoveTo(  j,  halfTick );
         pDC->LineTo(  j, -halfTick );
         pDC->MoveTo( -j,  halfTick );
         pDC->LineTo( -j, -halfTick );
      }
   }

   if( bDrawY )                                                      // draw vertical axis w/ 8 tick marks
   {
      pDC->MoveTo( 0, -m_logExtent.cy / 2 ); 
      pDC->LineTo( 0,  m_logExtent.cy / 2 );
      for( i = 1; i < 5; i++ ) 
      {
         j = (i * m_logExtent.cy) / 10;
         pDC->MoveTo(  halfTick,  j );
         pDC->LineTo( -halfTick,  j );
         pDC->MoveTo(  halfTick, -j );
         pDC->LineTo( -halfTick, -j );
      }
   }

   pDC->SelectObject( pOldPen );                                     // restore old pen; delete custom med gray pen
   if( bGrayPenCreated ) grayPen.DeleteObject();
}


//=== DrawSymbol ====================================================================================================== 
//
//    Draw/erase the specified symbol at its current coordinates.  If the symbol is currently hidden, or if it is 
//    outside the current logical extent of the plot, it is not drawn/erased.  If -1 is specified for the symbol, then 
//    all defined and visible symbols are drawn/erased.  A symbol is erased by drawing it in BLACK.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//                iSym  -- [in] index of symbol to be erased; -1 causes all defined symbols to be drawn. 
//                bErase-- [in] if FALSE, symbol is drawn (the default); otherwise, it is erased.
//
//    RETURNS:    TRUE if at least one symbol was drawn; FALSE otherwise (invalid symbol, or symbol not visible). 
//
BOOL CXYPlotBar::DrawSymbol( CDC* pDC, const int iSym, const BOOL bErase /* = FALSE */ )
{
   if( !m_bActive || (iSym < -1) || (iSym >= m_nDefined) ) return( FALSE );   // plot off, or invalid argument!

   CPen* pOldPen;
   CBrush *pOldBrush;
   if( bErase )                                                               // use black pen & brush for erasing 
   {
      pOldPen = (CPen*) pDC->SelectStockObject( BLACK_PEN ); 
      pOldBrush = (CBrush*) pDC->SelectStockObject( BLACK_BRUSH );
   }
   else                                                                       // start with white pen & brush if 
   {                                                                          // drawing...
      pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN ); 
      pOldBrush = (CBrush*) pDC->SelectStockObject( WHITE_BRUSH );
   }

   int start = (iSym == -1) ? 0 : iSym;                                       // draw one or all defined symbols
   int end = (iSym == -1) ? m_nDefined-1 : start;

   BOOL bDrawn = FALSE;                                                       // at least one symbol drawn/erased
   CPen* pPen = NULL;                                                         // custom pen used, if any
   CBrush* pBrush = NULL;                                                     // custom brush used, if any
   for( int i = start; i <= end; i++ )
   {
      if( !m_symbols[i].bShow || !IsSymbolVisible( i ) ) continue;            // skip symbols that are not visible

      bDrawn = TRUE;                                                          // we'll assume that drawing succeeds... 

      if( !bErase )                                                           // don't need custom stuff for erasing 
      {
         pPen = new CPen();                                                   // create custom pen & select into DC; 
         if( (pPen != NULL) &&                                                // use stock white pen if we can't make
             pPen->CreatePen( PS_SOLID, 0, m_symbols[i].color )               // the custom one...
           ) 
            pDC->SelectObject( pPen );

         pBrush = NULL;
         if( m_symbols[i].shape == BOX )                                      // if hollow shape, use stock hollow br
            pDC->SelectStockObject( HOLLOW_BRUSH );
         else                                                                 // else use custom brush. if we cannot
         {                                                                    // create it, use stock white brush 
            pBrush = new CBrush();
            if( (pBrush != NULL) &&
                pBrush->CreateSolidBrush( m_symbols[i].color ) 
              ) 
               pDC->SelectObject( pBrush );
         }
      }

      CRect rSym = GetSymbolRectFromPt( m_symbols[i].ptLoc );                 // bounding rect of symbol
      CPoint ctrPt;
      switch( m_symbols[i].shape )                                            // draw the symbol!
      {
         case BOX :
         case FILLBOX :
            pDC->Rectangle( rSym );
            break;
         case FILLCIRCLE :
            pDC->Ellipse( rSym );
            break;
         case XHAIR :                                                         // we draw each leg of "X" forwards and 
            pDC->MoveTo( rSym.left, rSym.top );                               // backwards to make it look square;
            pDC->LineTo( rSym.right, rSym.bottom );                           // LineTo does not draw endpt!
            pDC->LineTo( rSym.left, rSym.top ); 
            pDC->MoveTo( rSym.left, rSym.bottom );
            pDC->LineTo( rSym.right, rSym.top);
            pDC->LineTo( rSym.left, rSym.bottom ); 
            break;
         case TEE :                                                           // force an even # pixels in both dir 
            pDC->LPtoDP( rSym );                                              // so we can make "+" look square
            if( (rSym.bottom-rSym.top) % 2 != 0 ) 
            {
               if( rSym.Width() < rSym.Height() ) --rSym.bottom;
               else ++rSym.bottom;
            }
            if( (rSym.right-rSym.left) % 2 != 0 ) 
            {
               if( rSym.Width() < rSym.Height() ) ++rSym.right;
               else --rSym.right;
            }
            pDC->DPtoLP( rSym );

            ctrPt = rSym.CenterPoint();
            pDC->MoveTo( ctrPt.x, rSym.top );                                 // again, we draw forwards an backwards 
            pDC->LineTo( ctrPt.x, rSym.bottom );                              // because LineTo does not draw endpt!
            pDC->LineTo( ctrPt.x, rSym.top );
            pDC->MoveTo( rSym.left, ctrPt.y );
            pDC->LineTo( rSym.right, ctrPt.y );
            pDC->LineTo( rSym.left, ctrPt.y );
            break;
         case VERTLINE :                                                      // backwards to make it look square
            pDC->MoveTo( m_symbols[i].ptLoc.x, rSym.top );
            pDC->LineTo( m_symbols[i].ptLoc.x, rSym.bottom );
            break;
         default :
            ASSERT( FALSE );
            break;
      }

      if( !bErase )                                                           // if erasing, we never swap in a custom 
      {                                                                       // pen or brush.  otherwise, free any 
         pDC->SelectStockObject( WHITE_PEN );                                 // any custom pen and/or brush we used 
         pDC->SelectStockObject( WHITE_BRUSH );                               // for this symbol...
         if( pPen != NULL )
         {
            pPen->DeleteObject();
            delete pPen;
            pPen = NULL;
         }
         if( pBrush != NULL )
         {
            pBrush->DeleteObject();
            delete pBrush;
            pBrush = NULL;
         }
      }
   }

   pDC->SelectObject( pOldPen );                                              // restore original pen & brush
   pDC->SelectObject( pOldBrush );
   return( bDrawn );
}


//=== OverlapsXAxis, OverlapsYAxis ==================================================================================== 
//
//    Does the bounding rectangle of specified symbol overlap one of the plot axes?  Intended use is to determine 
//    whether or not to redraw an axis after erasing the specified symbol.
//
//    NOTE:  See also DrawAxes().  Tick marks extend to symbol width/2 on either side of each axis.  There are 8 tick 
//    marks, evenly distributed along each axis.
//
//    NOTE2:  Because of logical-to-device translations, earlier implementations would occasionally fail to detect a 
//    1-pixel overlap.  A simple solution was to double the actual size of the symbol in performing these checks, which 
//    are performed in logical coordinates.  Of course, this means we sometimes detect an overlap when there is none.
//
//    ARGS:       iSym  -- [in] array index of symbol
//
//    RETURNS:    TRUE if there's overlap; FALSE if not (or symbol not defined)
//
BOOL CXYPlotBar::OverlapsXAxis( const int iSym ) const 
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );            // invalid argument!


   int x = m_symbols[iSym].ptLoc.x;                                     // get absolute value of symbol location -- we 
   int y = m_symbols[iSym].ptLoc.y;                                     // can do this b/c ticks are symmetric about 
   if( x < 0 ) x = -x;                                                  // origin!!!
   if( y < 0 ) y = -y;

   int w = 2 * m_iSymWidth;                                             // use double-sized symbol to make sure we 
                                                                        // do not miss any overlaps...

   if( y > w ) return( FALSE );                                         // symbol rect overlaps neither axis nor ticks
   else if( y < w/2 ) return( TRUE );                                   // symbol rect overlaps axis itself
   else for( int i = 1; i < 5; i++ )                                    // symbol *may* overlap a tick mark...
   {
      int left = ((i * m_logExtent.cx) / 10) - (w/2);
      int right = left + w;
      if( (x >= left) && (x <= right) ) return( TRUE );              // tick mark overlapped by symbol rect!
   }

   return( FALSE );
}

BOOL CXYPlotBar::OverlapsYAxis( const int iSym ) const
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );            // invalid argument!

   int x = m_symbols[iSym].ptLoc.x;                                     // get absolute value of symbol location -- we 
   int y = m_symbols[iSym].ptLoc.y;                                     // can do this b/c of symmetry in our XY plot 
   if( x < 0 ) x = -x;                                                  // coord system, and the way we draw ticks!!!
   if( y < 0 ) y = -y;

   int w = 2 * m_iSymWidth;                                             // use double-sized symbol to make sure we 
                                                                        // do not miss any overlaps...

   if( x > w ) return( FALSE );                                         // symbol rect overlaps neither axis nor ticks
   else if( x < w/2 ) return( TRUE );                                   // symbol rect overlaps axis itself
   else for( int i = 1; i < 5; i++ )                                    // symbol *may* overlap a tick mark...
   {
      int bottom = ((i * m_logExtent.cy) / 10) - (w/2);
      int top = bottom + w;
      if( (y >= bottom) && (y <= top) ) return( TRUE );                 // tick mark overlapped by symbol rect!
   }

   return( FALSE );
}


//=== IsSymbolVisible ================================================================================================= 
//
//    Is the bounding rectangle of specified symbol at least partially inside the logical extent of the XY plot? 
//
//    ARGS:       iSym  -- [in] array index of symbol. 
//
//    RETURNS:    TRUE if it's visible; FALSE if not (or symbol not defined). 
//
BOOL CXYPlotBar::IsSymbolVisible( const int iSym ) const
{
   if( (iSym < 0) || (iSym >= m_nDefined) ) return( FALSE );   // invalid argument!

   int x = m_symbols[iSym].ptLoc.x;                            // get absolute value of symbol location -- we can do
   int y = m_symbols[iSym].ptLoc.y;                            // this because of the symmetry of our XY plot coord 
   if( x < 0 ) x = -x;                                         // system!!!!
   if( y < 0 ) y = -y;

   if( (x > (m_logExtent.cx + m_iSymWidth) / 2) ||
       (y > (m_logExtent.cy + m_iSymWidth) / 2)
     )
      return( FALSE );

   return( TRUE );
}


//=== UpdateMenuPopup ================================================================================================= 
//
//    Update the popup context menu when a symbol is added to or deleted from the symbol array.  If any menu operation 
//    here fails, we destroy the HMENU and automatically disable the context menu feature of CXYPlotBar. 
//
//    ARGS:       iSym  -- [in] array index of symbol added or deleted.  if -1 (the default), we rebuild from scratch.
//                bAdd  -- [in] if TRUE (the default), a symbol was added; else, the symbol was deleted.
//
//    RETURNS:    NONE. 
//
VOID CXYPlotBar::UpdateMenuPopup( const int iSym /* =-1 */, const BOOL bAdd /* =TRUE */ )
{
   if( (iSym == -1) && (m_popupMenu.GetSafeHmenu() != NULL) )          // force rebuild; destroy current menu
      m_popupMenu.DestroyMenu();

   int i;
   if( m_popupMenu.GetSafeHmenu() == NULL )                          // there's no HMENU yet -- build from scratch
   {
      if( !m_popupMenu.CreatePopupMenu() )                           //    create an empty popup HMENU
      {
         m_bEnableShowHide = FALSE;                                  //    on failure, auto-disable this feature!
         return;
      }

      for( i = 0; i < m_nDefined; i++ )
      {
         if( !m_popupMenu.AppendMenu( MF_STRING | MF_ENABLED,        //    append menu item for each symbol in array 
                  (UINT) (i + MENUIDOFFSET),                         //    order: offset so that there's no 0 cmd
                  m_symbols[i].strName )
           )
            goto FATAL_EXIT; 
      }
   }
   else if( bAdd )                                                   // add a single menu item (we always append)
   {
      ASSERT( m_popupMenu.GetMenuItemCount()==(UINT)(m_nDefined-1) );
      ASSERT( iSym == m_nDefined-1 );
      if( !m_popupMenu.AppendMenu( MF_STRING | MF_ENABLED,           //    append MI for the new symbol
            (UINT) (iSym + MENUIDOFFSET), m_symbols[iSym].strName )
        )
         goto FATAL_EXIT; 
   }
   else                                                              // remove a single menu item
   {
      ASSERT( m_popupMenu.GetMenuItemCount()==(UINT)(m_nDefined+1) );
      if( !m_popupMenu.DeleteMenu( (UINT)(iSym + MENUIDOFFSET),      //    delete the menu item...
           MF_BYCOMMAND ) )
         goto FATAL_EXIT; 
      for( i = iSym+1; i < m_nDefined; i++ )                         //    ...and adjust the command IDs of the menu 
      {                                                              //    items that followed the deleted one...
         if( !m_popupMenu.ModifyMenu( (UINT)(i + MENUIDOFFSET), 
              MF_BYCOMMAND | MF_STRING | MF_ENABLED, 
              (UINT)(i - 1 + MENUIDOFFSET), 
              m_symbols[i-1].strName ) )
            goto FATAL_EXIT; 
      }
   }

   return;

FATAL_EXIT:                                                          // if any menu operation failed, we destroy menu 
   m_popupMenu.DestroyMenu();                                        // entirely and disable the context-menu feature 
   m_bEnableShowHide = FALSE;
   return;
}
