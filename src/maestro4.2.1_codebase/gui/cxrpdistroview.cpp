//=====================================================================================================================
//
// cxrpdistroview.cpp : Implementation of class CCxRPDistroView, a subclassed CStatic control used to display a
//                      histogram representation of the current and previous distributions in a CCxRPDistro object.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CCxRPDistroView is a CStatic control subclassed for the purpose of displaying the contents of a CCxRPDistro object.
// It was introduced in Maestro v1.4 as part of modifications in support of a response distribution-based
// reward/penalty contingency protocol.  This protocol is described in the CCxRPDistro class file.
//
// ==>Usage.
// Call SetData() with a pointer to a valid CCxRPDistro object.  CCxRPDistro will draw staircase histograms for the
// "current" and "previous" distributions of the CCxRPDistro object, if they exist.  The current distribution is
// drawn with a white pen, the previous distribution in a medium gray pen.  Furthermore, the bars of the previous
// distribution are filled with med grey -- which makes it easier to compare the two distributions visually (the
// current histogram is drawn on top of the previous histogram).  A bright green line spanning mean +/- std dev for
// the current distribution is drawn at Y=75%; a dim green line spanning mean +/- std dev for the previous distribution
// is drawn at Y=70%.  These lines only appear when the distribution contains 2 or more valid samples.
//
// If a reward window is defined on the CCxRPDistro object, it is represented by blue-green rectangles in the narrow
// top and bottom margins; their width reflects the range spanned by the reward window.
//
// The horizontal extent of the view (not including left and right margins) is set to the CCXRPDistro object's "valid
// response range". The vertical extent (not including top and bottom margins) depends on whether the display mode
// is "unnormalized" or "normalized".  In the former case, the vertical extent equals the maximum observed bin count
// over the two staircase histograms (but at least 10). The max observed bin count is drawn as a text label near the
// top left corner of the view.  In "normalized" mode, the vertical extent spans [0..1000], where 1000 corresponds to
// the max observed bin count in the histogram (or 1, if the histogram is empty).  The current and previous histograms
// are separately normalized, so they will have the same height.  The text label near the top left corner reads "1.0"
// in normalized mode.  The user can toggle between the two alternate modes by left-clicking on the view.
//
// NOTE:  It is essential that the control be created with the SS_NOTIFY style for the left-click action to work.  If
// the SS_NOTIFY style is not specified, the control will never receive the WM_LBUTTONUP message that triggers a
// switch in the display mode.
//
// To update only the "current" distribution, call RebuildCurrent().  If only the reward window changed on the
// installed CCxRPDistro object, simply repaint the control by invoking Invalidate(TRUE).  If the valid response range
// changes, call Rebuild().
//
// REVISION HISTORY:
// 29nov2005-- Initial development begun.
// 14dec2005-- Modified to use the "valid response range" on the CCxRPDistro object as the range spanned by the view.
// 03jan2006-- Reward window rectangles are now drawn in both the top and bottom margins.
// 11mar2006-- Mod IAW revisions to R/P Distro feature:  now just one reward window.
//          -- A bright green line spans mean+/-std for the current distribution; a dim green line spans mean+/-std
//             for the previous distribution.
//          -- Added option to display histograms in a normalized view, where y-axis spans [0..1000] and 1000
//             corresponds to the max count observed over all the bins of a histogram.  Each histogram is separately
//             normalized -- so they'll both have the same height.  The user toggles between the normalized and
//             unnormalized displays by left-clicking on the view.
// 16mar2006-- Minor mods IAW changes to CCxRPDistro.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"

#include "cxrpdistro.h"                      // CCxRPDistro
#include "cxrpdistroview.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
const int CCxRPDistroView::MARGINSZ             = 5;                 // extent of L, R, T and B margins in pixels
const COLORREF CCxRPDistroView::REWWINCOLOR     = RGB(0,128,255);    // the color used to fill reward window rect
const COLORREF CCxRPDistroView::PREVHISTCOLOR   = RGB(128,128,128);  // color of pen/brush for "previous" histogram
const COLORREF CCxRPDistroView::TEXTCOLOR       = RGB(255,255,255);  // color used to render text in view
const COLORREF CCxRPDistroView::CURRMEANCOLOR   = RGB(0,255,0);      // color of line spanning mean+/-std for curr hist
const COLORREF CCxRPDistroView::PREVMEANCOLOR   = RGB(0,128,0);      // color of line spanning mean+/-std for prev hist


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

//=== CCxRPDistroView/~CCxRPDistroView [constructor/destructor] =======================================================
//
//    Constructed in an inactive state (canvas is empty, no CCxRPDistro object to display).
//
CCxRPDistroView::CCxRPDistroView()
{
   m_pRPDistro = NULL;
   ::memset( m_hist, 0, NUMBINS * sizeof(int) );
   int n = NUMBINS*2 + 2;
   ::memset( m_currHistPts, 0, n * sizeof(POINT) );
   ::memset( m_prevHistPts, 0, n * sizeof(POINT) );
   m_iXMin = 0;
   m_iXMax = 0;
   m_iYMax = 0;
   m_isYNormalized = FALSE;
}

CCxRPDistroView::~CCxRPDistroView()
{
}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

BEGIN_MESSAGE_MAP( CCxRPDistroView, CStatic )
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


//=== OnPaint [base override] =========================================================================================
//
//    Response to WM_PAINT message.  Here we assume the entire client area has already been erased (OnEraseBkgnd()),
//    and we redraw the entire representation of the CCxRPDistro object.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxRPDistroView::OnPaint()
{
   CPaintDC dc( this );

   if( m_pRPDistro==NULL ) return;                             // there's no distribution to render; canvas is blank

   SetupCoords( &dc );
   DrawRewardWindow( &dc );
   DrawDistributions( &dc );
   DrawAnnotations( &dc );
}


//=== OnSize [base override] ==========================================================================================
//
//    Response to the WM_SIZE message.  Whenever the control is resized, we must repaint the entire client area.
//
//    ARGS:       pDC   -- [in] ptr to device context
//
//    RETURNS:    TRUE to indicate background was erased; FALSE otherwise.
//
void CCxRPDistroView::OnSize( UINT nType, int cx, int cy )
{
   // repaint entire client area
   Invalidate(TRUE);
}


//=== OnEraseBkgnd [base override] ====================================================================================
//
//    Response to the WM_ERASEBKGND message.  The default processing of this message erases the background using the
//    "class background brush".  However, we want the background to always be black.  So we override this message
//    handler and erase the background ourselves.
//
//    ARGS:       pDC   -- [in] ptr to device context
//
//    RETURNS:    TRUE to indicate background was erased; FALSE otherwise.
//
BOOL CCxRPDistroView::OnEraseBkgnd( CDC *pDC )
{
   CRect rect;
   GetClientRect( rect );
   pDC->FillSolidRect( rect, RGB(0, 0, 0) );
   return( TRUE );
}

//=== OnLButtonUp [base override] =====================================================================================
//
//    Response to the WM_LBUTTONUP message.  Each time the user clicks on the view, we toggle between the normalized
//    and unnormalized display modes.  The internal representations of the histograms must be rebuilt, then the
//    entire client area is repainted.
//
//    ARGS:       pDC   -- [in] ptr to device context
//
//    RETURNS:    TRUE to indicate background was erased; FALSE otherwise.
//
void CCxRPDistroView::OnLButtonUp(UINT nFlags, CPoint point)
{
   m_isYNormalized = !m_isYNormalized;
   Rebuild();
}


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Rebuild =========================================================================================================
//
//    Rebuild the histogram for the "current" distribution encapsulated in the view's CCxRPDistro object, or rebuild
//    the histograms for both the "current" and "previous" distributions.  Then repaint the client area to reflect the
//    changes.
//
//    Call this method whenever a new sample is added to the CCxRPDistro object displayed here, when the CCxRPDistro's
//    "valid response range" is changed, when the display mode is toggled, or when a different CCxRPDistro object is
//    installed in the view.
//
//    ARGS:       bBoth -- [in] If TRUE (the default), both histograms are rebuilt.  Otherwise, only the "current"
//                         distribution's histogram is rebuilt.
//    RETURNS:    NONE
//
void CCxRPDistroView::Rebuild( BOOL bBoth /* =TRUE */ )
{
   // there's nothing to build if there's no data to view, but we still repaint
   if( m_pRPDistro == NULL )
   {
      Invalidate(TRUE);
      return;
   }

   // compute bin size that divides CCxRPDistro object's "valid response range" into set # of bins
   float fMin;
   float fMax;
   m_pRPDistro->GetResponseRange( fMin, fMax );
   float fBinSize = 1000.0f * (fMax - fMin) / float(NUMBINS);

   // rebuild current histogram, or both of them
   int n = bBoth ? 2 : 1;
   for( int i=0; i<n; i++ )
   {
      // reinitialize staircase representation of histogram:  all bins are empty
      LPPOINT histPts = (i==0) ? &(m_currHistPts[0]) : &(m_prevHistPts[0]);
      ::memset( histPts, 0, (NUMBINS*2 + 2) * sizeof(POINT) );

      // get histogram bins for the distribution
      ::memset( m_hist, 0, NUMBINS * sizeof(int) );
      BOOL bOk = (i==0) ? m_pRPDistro->GetCurrentHistogram( m_hist, NUMBINS ) :
         m_pRPDistro->GetPreviousHistogram( m_hist, NUMBINS );
      if( !bOk ) continue;

      // the first bin starts at the minimum response value -- x1000 to convert to logical units
      float fBinStart = 1000.0f * fMin;

      // for normalized view, we divide all bin counts by the max observed bin count across histogram
      int divideBy = 1;
      if( m_isYNormalized )
      {
         for( int k = 0; k<NUMBINS; k++ )
         {
            if( m_hist[k] > divideBy ) divideBy = m_hist[k];
         }
      }

      // now build staircase representation of histogram in logical units
      // NOTE: There are two extra points, representing points at the baseline at either extreme of the histogram!
      histPts[0].x = (long) floor(fBinStart);
      histPts[0].y = 0;

      int k = 0;
      for( int j=1; j<=NUMBINS*2; j+=2 )
      {
         histPts[j].x = (long) floor(fBinStart);
         histPts[j].y = (m_hist[k] * 1000) / divideBy;               // y-axis LU is thousandth of a count or
                                                                     // thousandth of max observed bin count
         fBinStart += fBinSize;
         histPts[j+1].x = (long) floor(fBinStart);
         histPts[j+1].y = (m_hist[k] * 1000) / divideBy;
         ++k;
      }

      histPts[NUMBINS*2 + 1].x = (long) floor(fBinStart);
      histPts[NUMBINS*2 + 1].y = 0;
   }

   // repaint client area to reflect changes
   Invalidate(TRUE);
}

//=== SetupCoords =====================================================================================================
//
//    Set up logical-to-device translations for the device context such that:
//       1) The logical width of the client area covers the "valid response range" for the CCxRPDistro object being
//          viewed, with left and right margins excluded.  Logical units are 0.001SU, where "SU" is the unit of measure
//          for the distribution samples.
//       2) The y-axis increase upwards rather than downwards.
//       3) The logical height spans the range [0..1000] in "normalized" mode, where 1000 is the height of the
//          histogram bar corresponding to the max observed bin count. When "unnormalized", the logical height spans
//          [0..1000*Nmax], where Nmax is the maximum observed bin count across BOTH histograms (but at least 10).
//       3) The y-axis increases upwards rather than downwards.
//
//    Logical units are in "parts per 1000" b/c CDC restricts logical coords to integral values!)
//
//    The left and right margins are narrow and merely for appearances.  Rectangles representing the span of the
//    reward window are drawn in the top and bottom margins.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//    RETURNS:    NONE
//
VOID CCxRPDistroView::SetupCoords( CDC* pDC )
{
   ASSERT( m_pRPDistro != NULL );

   // x-axis range is simply the valid response range for the CCxRPDistro object being viewed
   float fxMin;
   float fxMax;
   m_pRPDistro->GetResponseRange( fxMin, fxMax );

   m_iXMin = int( floor(fxMin*1000.0f) );
   m_iXMax = int( ceil(fxMax*1000.0f) );

   // for "normalized" display mode, y-axis range is [0..1000].  For "unnormalized" mode, it is [0..1000*Nmax], where
   // Nmax = max counts-per-bin over current and previous distribution histograms.
   // NOTE: The first and last points in the staircase are not included (they're always at the baseline, count=0).
   m_iYMax = 1000;
   if( !m_isYNormalized )
   {
      m_iYMax = 0;
      for( int i=1; i<=NUMBINS*2; i+=2 )
      {
         if( m_currHistPts[i].y > long(m_iYMax) ) m_iYMax = (int) m_currHistPts[i].y;
         if( m_prevHistPts[i].y > long(m_iYMax) ) m_iYMax = (int) m_prevHistPts[i].y;
      }
      if( m_iYMax < 10000 ) m_iYMax = 10000;
   }

   // set up logical and device coordinates
   CRect rClient;
   GetClientRect( &rClient );
   pDC->SetMapMode( MM_ANISOTROPIC );                                // x- and y-axes are independent...
   pDC->SetWindowExt( m_iXMax - m_iXMin, m_iYMax );
   pDC->SetViewportExt( rClient.right-1 - 2*MARGINSZ, -(rClient.bottom-1 - 2*MARGINSZ) );
   pDC->SetViewportOrg( rClient.left + MARGINSZ, rClient.bottom-1-MARGINSZ );
   pDC->SetWindowOrg( m_iXMin, 0 );
}

//=== DrawRewardWindow ================================================================================================
//
//    Helper method that renders the reward window of the CCxRPDistro object displayed in this view, if the reward
//    window is enabled.  It it represented by solid blue-green rectangles in both the top and bottom margins.  The
//    width of each rectangle spans the extent of the reward window, while its height fills the margin.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//    RETURNS:    NONE
//
void CCxRPDistroView::DrawRewardWindow( CDC* pDC )
{
   ASSERT( m_pRPDistro != NULL );

   if( m_pRPDistro->IsRewardWinEnabled() )
   {
      // get reward window bounds
      float fMin = m_pRPDistro->GetRewardWinMinimum();
      float fMax = m_pRPDistro->GetRewardWinMaximum();

      // convert to integral logical units = 0.001 sample units
      int iMin = (int) floor( fMin*1000.0f );
      int iMax = (int) ceil( fMax*1000.0f );

      // only draw the portion of window that is within the min-max bounds of our view
      if( iMin < m_iXMin ) iMin = m_iXMin;
      if( iMax > m_iXMax ) iMax = m_iXMax;
      if( iMin >= iMax ) return;             // reward is completely outside our view!

      // rectangles are drawn in both the top and bottom margins, spanning each margin vertically and spanning the
      // reward window's width horizontally.  Coords are in logical units.
      COLORREF oldBkgClr = pDC->GetBkColor();

      CRect rMargin(0,0,MARGINSZ,MARGINSZ);
      pDC->DPtoLP( &rMargin );
      pDC->FillSolidRect( iMin, rMargin.top, iMax-iMin, rMargin.bottom-rMargin.top, REWWINCOLOR );
      GetClientRect( rMargin );
      rMargin.top = rMargin.bottom - MARGINSZ;
      pDC->DPtoLP( &rMargin );
      pDC->FillSolidRect( iMin, rMargin.top, iMax-iMin, rMargin.bottom-rMargin.top, REWWINCOLOR );

      pDC->SetBkColor( oldBkgClr );
   }
}

//=== DrawDistributions ===============================================================================================
//
//    Helper method that renders the histograms for the "previous" and "current" distributions encapsulated by the
//    CCxRPDistro object displayed in this view.  The histogram is drawn in staircase fashion.  If a distribution
//    contains less than 2 samples, it is not drawn.  The "current" histogram is drawn on top of the "previous" one
//    The previous histogram is outlined and filled as a polygon, while the "current" histogram is only outlined.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//    RETURNS:    NONE
//
void CCxRPDistroView::DrawDistributions( CDC* pDC )
{
   ASSERT( m_pRPDistro != NULL );

   CPen* pOldPen = NULL;                                             // for restoring original pen into DC

   if( m_pRPDistro->GetNumValidPreviousSamples() > 1 )               // draw "previous" histogram in custom pen
   {
      CPen currPen;
      if( currPen.CreatePen( PS_SOLID, 0, PREVHISTCOLOR ) )          // create custom pen & select into DC
         pOldPen = (CPen*) pDC->SelectObject( &currPen );
      else
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );      // use stock white pen if can't make custom one!

      CBrush* pOldBrush;                                             // create custom brush & select into DC; use
      CBrush currBrush;                                              // stock gray brush if can't make custom one!
      if( currBrush.CreateSolidBrush( PREVHISTCOLOR ) )
         pOldBrush = (CBrush*) pDC->SelectObject( &currBrush );
      else
         pOldBrush = (CBrush*) pDC->SelectStockObject( GRAY_BRUSH );

      int oldPolyFillMode = pDC->GetPolyFillMode();                  // set poly fill mode so we fill interior of
      pDC->SetPolyFillMode( WINDING );                               // the histogram "polygon"

      pDC->Polygon( &(m_prevHistPts[0]), NUMBINS*2 + 2 );

      pDC->SetPolyFillMode( oldPolyFillMode );                       // restore old pen, brush, and polyfill mode;
      pDC->SelectObject( pOldBrush );                                // destroy custom GDI objects.
      pDC->SelectObject( pOldPen );
      currPen.DeleteObject();
      currBrush.DeleteObject();
   }

   if( m_pRPDistro->GetNumValidCurrentSamples() > 0 )                // draw "current" histogram in stock white pen
   {
      pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );
      pDC->Polyline( &(m_currHistPts[0]), NUMBINS*2 + 2 );
      pDC->SelectObject( pOldPen );
   }
}

//=== DrawAnnotations =================================================================================================
//
//    Helper method that renders a couple annotations on this view:
//       1) A text label reflecting the maximum y-coord value ("counts per bin"), is left-aligned against the left
//          margin, near the top of the window.  In "normalized" mode, this label reads "1.0".
//       2) A bright (dim) green line is drawn at Y=75% (70%) of maxY and spanning the mean +/- stddev for the current
//          (previous) distribution.
//       3) A vertical white line is drawn in the middle of the left margin, representing a crude y-axis.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//    RETURNS:    NONE
//
void CCxRPDistroView::DrawAnnotations( CDC* pDC )
{
   ASSERT( m_pRPDistro != NULL );

   // need client rect to compute formatting rectangles for the labels
   CRect rClient;
   GetClientRect( rClient );

   // remember current text color so we can restore it later; then change text color to white
   COLORREF oldTextClr = pDC->GetTextColor();
   pDC->SetTextColor( TEXTCOLOR );

   // label for the maximum y-coord
   CString label = _T("1.0");
   if( !m_isYNormalized )
      label.Format( "%d", m_iYMax/1000 );

   CRect rTextBounds( MARGINSZ, MARGINSZ, rClient.Width()/2, 5*MARGINSZ );
   pDC->DPtoLP( &rTextBounds );
   pDC->DrawText( label, &rTextBounds, DT_SINGLELINE | DT_TOP | DT_LEFT );

   // restore original text color
   pDC->SetTextColor( oldTextClr );

   // a horizontal line at Y=70% of maxY, spanning mean+/-std dev for the previous distribution
   float fMean = m_pRPDistro->GetPreviousMean();
   float fStd = m_pRPDistro->GetPreviousStdDev();
   if( fStd != 0.0f )
   {
      // want a custom pen, if we can get it
      CPen* pOldPen;
      CPen currPen;
      if( currPen.CreatePen( PS_SOLID, 0, PREVMEANCOLOR ) )
         pOldPen = (CPen*) pDC->SelectObject( &currPen );
      else
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );

      // calc endpts of line spanning mean +/- stddev, in integral logical units = 0.001SU
      int iMin = (int) floor( (fMean-fStd)*1000.0f );
      if( iMin < m_iXMin ) iMin = m_iXMin;
      int iMax = (int) ceil( (fMean+fStd)*1000.0f );
      if( iMax > m_iXMax ) iMax = m_iXMax;
      CPoint pt(iMin, (m_iYMax*7)/10);
      pDC->MoveTo(pt);
      pt.x = iMax;
      pDC->LineTo(pt);

      // restore old pen
      pDC->SelectObject( pOldPen );
      currPen.DeleteObject();
   }

   // a horizontal line at Y=75% of maxY, spanning mean+/-std dev for the current distribution
   fMean = m_pRPDistro->GetCurrentMean();
   fStd = m_pRPDistro->GetCurrentStdDev();
   if( fStd != 0.0f )
   {
      // want a custom pen, if we can get it
      CPen* pOldPen;
      CPen currPen;
      if( currPen.CreatePen( PS_SOLID, 0, CURRMEANCOLOR ) )
         pOldPen = (CPen*) pDC->SelectObject( &currPen );
      else
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );

      // calc endpts of line spanning mean +/- stddev, in integral logical units = 0.001SU
      int iMin = (int) floor( (fMean-fStd)*1000.0f );
      if( iMin < m_iXMin ) iMin = m_iXMin;
      int iMax = (int) ceil( (fMean+fStd)*1000.0f );
      if( iMax > m_iXMax ) iMax = m_iXMax;
      CPoint pt(iMin, (m_iYMax*3)/4);
      pDC->MoveTo(pt);
      pt.x = iMax;
      pDC->LineTo(pt);

      // restore old pen
      pDC->SelectObject( pOldPen );
      currPen.DeleteObject();
   }

   // a white vertical axis line
   CPen* pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );
   CPoint pt(MARGINSZ/2, MARGINSZ);
   pDC->DPtoLP( &pt );
   pDC->MoveTo( pt );
   pt.x = MARGINSZ/2;
   pt.y = rClient.bottom - MARGINSZ;
   pDC->DPtoLP( &pt );
   pDC->LineTo( pt );
   pDC->SelectObject( pOldPen );
}

