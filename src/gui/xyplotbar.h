//===================================================================================================================== 
//
// xyplotbar.h : Declaration of class CXYPlotBar.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(XYPLOTBAR_H__INCLUDED_)
#define XYPLOTBAR_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizebar\scbarcf.h"                    // base class CSizingControlBarCF


//===================================================================================================================== 
// Declaration of class CXYPlotBar 
//===================================================================================================================== 
//
class CXYPlotBar : public CSizingControlBarCF
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
public:
   // enumeration of symbol shapes available
   typedef enum 
   {
      BOX = 0,                            //       a hollow square
      FILLBOX,                            //       a filled square
      FILLCIRCLE,                         //       a filled circle
      XHAIR,                              //       an "X"
      TEE,                                //       a "+"
      VERTLINE                            //       a short vertical line
   } SymbolShape;

private:
   static const int MAXSYMBOLS = 10;      // max # of symbols supported by the plot bar
   static const int MENUIDOFFSET = 100;   // symbol N is associated with cmdID MENUIDOFFSET+N on the context menu


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   struct CSymbol                         // plot symbol attributes
   {
      SymbolShape shape;
      COLORREF    color;
      CPoint      ptLoc;
      BOOL        bShow;
      CString     strName;
   };

   BOOL        m_bActive;                 // TRUE when display is active 
   BOOL        m_bEnableShowHide;         // if TRUE, display popup context menu to toggle symbol visibility state

   CSize       m_logExtent;               // current logical extent of plot display
   int         m_iSymWidth;               // all symbols are square; width of symbol varies with logical extent

   int         m_nDefined;                // number of defined symbols 
   CSymbol     m_symbols[MAXSYMBOLS];     // the symbols themselves

   CMenu       m_popupMenu;               // popup context menu for toggling visibility of defined symbols
   

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CXYPlotBar& operator=( const CXYPlotBar& src );       // decl only:  there is no operator= for this class
   CXYPlotBar( const CXYPlotBar& src );                  // decl only:  there is no copy constructor for this class

public:
	CXYPlotBar();
	~CXYPlotBar() {}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnPaint();                               // repaint client area
   afx_msg void OnSize( UINT nType, int cx, int cy )     // whenever window is resized, we repaint client area
   {
      Invalidate( TRUE );
   }
   afx_msg BOOL OnEraseBkgnd( CDC *pDC );                // overridden to erase bkg with black
   afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos ); // for context menu support
   DECLARE_MESSAGE_MAP()

//===================================================================================================================== 
// OPERATIONS/ATTRIBUTES
//===================================================================================================================== 
public:
   VOID Activate( const BOOL bOn = TRUE );               // activate/deactivate the plot display
   BOOL EnableShowHide( const BOOL bEna = TRUE );        // enable/disable context menu to show/hide symbols

   CSize GetLogicalExtent() const                        // retrieve current logical extent of plot window
   { return( CSize( m_logExtent ) ); }
   int GetLogicalExtentX() const 
   { return( m_logExtent.cx ); }
   int GetLogicalExtentY() const 
   { return( m_logExtent.cy ); }

   int GetSymbolWidth() const                            // retrieve current symbol size
   { return( m_iSymWidth ); }
   
   BOOL SetLogicalExtent( const SIZE newSize );          // change current logical extent of plot window
   BOOL SetLogicalExtent( int cx, int cy )
   { return( SetLogicalExtent( CSize(cx,cy) ) ); }

   int GetNumSymbols() { return( m_nDefined ); }         // retrieve # of symbols currently defined
   int AddSymbol( SymbolShape symShape,                  // add a new symbol to the XY plot
                  const COLORREF color, 
                  LPCTSTR str = NULL );
   BOOL DeleteSymbol( const int iSym );                  // delete an existing symbol 

   BOOL MoveSymbol( const int iSym, const CPoint pt );   // move existing symbol to a new location
   BOOL ShowSymbol( const int iSym, const BOOL bShow );  // show/hide an existing symbol
   VOID UpdateSymbols( LPPOINT pptNew );                 // update all defined symbols in one go

   BOOL GetCursorLogicalPos( CPoint& pt );               // get current pos of mouse cursor in logical coords, IF mouse 
                                                         // cursor is inside client area

//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected: 
   VOID SetupCoords( CDC* pDC );                         // prepare DC for drawing in logical coordinate system 
   VOID DrawAxes( CDC* pDC, const BOOL bDrawX = TRUE,    // draw the x- and y-axes
                  const BOOL bDrawY = TRUE ); 
   BOOL EraseSymbol( CDC* pDC, const int iSym )          // erase symbol at its current location 
   { return( DrawSymbol( pDC, iSym, TRUE ) ); }
   BOOL DrawSymbol( CDC* pDC, const int iSym, 
                    const BOOL bErase = FALSE );         // draw or erase symbol at its current location

   BOOL OverlapsXAxis( const int iSym ) const;           // does symbol rect overlap x-axis?
   BOOL OverlapsYAxis( const int iSym ) const;           // does symbol rect overlap y-axis?
   BOOL IsSymbolVisible( const int iSym ) const;         // is symbol's bounding rect currently inside logical extent?
   VOID UpdateMenuPopup( const int iSym = -1,            // update popup menu when an item is added or deleted
                         const BOOL bAdd = TRUE );

   CRect GetSymbolRectFromPt( const CPoint pt ) const    // bounding rect of symbol at given pt, in logical coords
   {
      int w = m_iSymWidth / 2;
      return( CRect( pt.x - w, pt.y + w, pt.x + w, pt.y - w ) );
   }

};



#endif // !defined(XYPLOTBAR_H__INCLUDED_)
