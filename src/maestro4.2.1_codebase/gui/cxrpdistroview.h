//=====================================================================================================================
//
// cxrpdistroview.h : Declaration of class CCxRPDistroView
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXRPDISTROVIEW_H__INCLUDED_)
#define CXRPDISTROVIEW_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CCxRPDistro;               // forward declaration


//=====================================================================================================================
// Declaration of class CCxRPDistroView
//=====================================================================================================================
//
class CCxRPDistroView : public CStatic
{
//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int NUMBINS = 25;                        // fixed number of bins in histogram drawn in this view
   static const int MARGINSZ;                            // extent of L, R, T and B margins in pixels
   static const COLORREF REWWINCOLOR;                    // the color used to fill reward window rect
   static const COLORREF PREVHISTCOLOR;                  // color of pen/brush used to draw "previous" histogram
   static const COLORREF TEXTCOLOR;                      // color used to render text in view
   static const COLORREF CURRMEANCOLOR;                  // color of line spanning mean+/-std for current histogram
   static const COLORREF PREVMEANCOLOR;                  // color of line spanning mean+/-std for previous histogram

//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   CCxRPDistro* m_pRPDistro;                             // the CCxRPDistro displayed on this view
   int m_hist[NUMBINS];                                  // used to retrieve histogram data from CCxRPDistro obj
   POINT m_currHistPts[NUMBINS*2 + 2];                   // internal staircase rep of curr distribution's histogram
   POINT m_prevHistPts[NUMBINS*2 + 2];                   // internal staircase rep of prev distribution's histogram
   int m_iXMin;                                          // horiz extent of histogram display in logical units, ie,
   int m_iXMax;                                          // 0.001 "sample units".
   int m_iYMax;                                          // vert extent of histogram display in "counts per bin" if 
                                                         // unnormalized, or 1000 if normalized.
   BOOL m_isYNormalized;                                 // if set, vert extent is normalized to max observed bin count 

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxRPDistroView& operator=( const CCxRPDistroView& src );     // decl only:  there is no operator= for this class
   CCxRPDistroView( const CCxRPDistroView& src );        // decl only:  there is no copy constructor for this class

public:
   CCxRPDistroView();
   ~CCxRPDistroView();

//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnPaint();                               // repaint client area
   afx_msg void OnSize( UINT nType, int cx, int cy );    // whenever window is resized, we repaint entire client area
   afx_msg BOOL OnEraseBkgnd( CDC *pDC );                // overridden to erase bkg with black
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);  // toggle between normalized/unnormalized display
   DECLARE_MESSAGE_MAP()

//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   void SetData( CCxRPDistro* pRPDistro )                // change the CCxRPDistro object displayed in this view
   {
      m_pRPDistro = pRPDistro;
      Rebuild();
   }
   void RebuildCurrent() { Rebuild(FALSE);}              // rebuild "current" histogram only and repaint
   void Rebuild( BOOL bBoth = TRUE );                    // rebuild "current" histogram or both histograms and repaint

   int GetNumHistogramBins()                             // constant #bins in histogram display of distributions
   {
      return( (int) NUMBINS );
   }

private:
   void SetupCoords( CDC* pDC );                         // helper methods that handle details of painting the canvas
   void DrawRewardWindow( CDC* pDC );
   void DrawDistributions( CDC* pDC );
   void DrawAnnotations( CDC* pDC );

};



#endif // !defined(CXRPDISTROVIEW_H__INCLUDED_)
