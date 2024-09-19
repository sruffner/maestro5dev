//===================================================================================================================== 
//
// graphbar.h : Declaration of class CGraphBar.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(GRAPHBAR_H__INCLUDED_)
#define GRAPHBAR_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizebar\scbarcf.h"                    // base class CSizingControlBarCF


//===================================================================================================================== 
// Declaration of class CGraphBar 
//===================================================================================================================== 
//
class CGraphBar : public CSizingControlBarCF
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
public:
   static const int MAXTRACES = 10;       // max # of traces (continuous-data or pulse train) that can be drawn 

protected:
   static const int MINWIDTH = 100;       // min width of trace display in logical units ("ticks")
   static const int MINBINS = 1000;       // min #bins per stored trace.  Stored trace data is a subsampled version
                                          // of the original raw data supplied during updates.
   static const int XMARGINSZ = 5;        // half of margin width, in device units (pixels) 
   static const int PULSEHT = 5;          // height of a single pulse drawn on graph, in pixels
   static const int MARKERW = 3;          // width of timepoint markers, in pixels


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
public:
   struct CTrace                          // trace attributes 
   {
      int         iGain;                  //    gain, G.  G=0 for impulse train; Y = B + G*datum for G>0; 
                                          //    Y = B + datum/|G| for G<0.
      int         iOffset;                //    "baseline" offset, B, in logical units
      COLORREF    color;                  //    display color
   }; 

protected:
   struct CTraceSet                       // the complete definition for a set of displayed traces
   {
      int         iWidth;                 // graph's logical width (logical units = #ticks)
      int         t0, t1;                 // interval of "time" displayed on graph -- for "delayed display" mode only
      int         iYMin, iYMax;           // range of visible y-axis values, in logical units

      int         nTraces;                // # of traces shown on graph
      CTrace      trace[MAXTRACES];       // trace attributes
	  CString	  strLabel;               // trace set label -- used in "delayed display" mode only
	  
      short*      pTraceData;             // stored trace data (usu. a subsampled version of supplied raw data)
      int         nMaxBins;               // total #bins per data trace
      int         nBins;                  // #bins per "drawn trace" currently (<= max #)

      int         tCurrent;               // current "time" in logical units (#ticks)

      int         iNextBin;               // the next bin to be filled in stored trace arrays
   };

   CTraceSet      m_traceSet[2];          // two trace sets so we can display one while preparing the next one
   CTraceSet*     m_pDisplaySet;          // the trace set being displayed
   CTraceSet*     m_pDelayedSet;          // the trace set being updated now for later display; not used in cont op

   int            m_rawSampleIntvMS;      // the sample interval for the raw data supplied during display updates, 
                                          // in milliseconds -- so we can report the effective sampling rate of the 
                                          // trace display, which may be limited by memory or pixel resolution
   float          m_currSampleFreqHz;     // effective sampling rate of trace display in Hz

   BOOL           m_bDelayMode;           // TRUE when we delay the display of trace set now being updated while 
                                          // continuing to display the previous trace set -- "delayed display" mode
   BOOL           m_bNormMargin;          // TRUE when margin is preferred size -- in which case baseline symbols drawn 

   int            m_accumBin[MAXTRACES];  // used to accumulate raw trace data during subsampling
   int            m_perBin[MAXTRACES];    // number of raw data samples accumulated during subsampling; this will vary 
                                          // from bin to bin!

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CGraphBar& operator=( const CGraphBar& src );         // decl only:  there is no operator= for this class
   CGraphBar( const CGraphBar& src );                    // decl only:  there is no copy constructor for this class

public:
   CGraphBar();
   ~CGraphBar()                                          // free storage allocated for trace points before dying
   {
      if( m_traceSet[0].pTraceData != NULL )
         delete[] m_traceSet[0].pTraceData;
      if( m_traceSet[1].pTraceData != NULL )
         delete[] m_traceSet[1].pTraceData;
   }



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg int OnCreate( LPCREATESTRUCT lpcs );          // allocate internal storage for trace points here
   afx_msg void OnPaint();                               // repaint client area
   afx_msg void OnSize( UINT nType, int cx, int cy );    // whenever window is resized, we repaint entire client area
   afx_msg BOOL OnEraseBkgnd( CDC *pDC );                // overridden to erase bkg with black
   DECLARE_MESSAGE_MAP()



//===================================================================================================================== 
// OPERATIONS/ATTRIBUTES
//===================================================================================================================== 
public:
   static BOOL MaxTraces() { return( MAXTRACES ); }      // max # of traces supported by the graph bar

   BOOL InitGraph( int yMin, int yMax, int t0, int t1,   // initialize and configure graph bar to display traces
      int intv, int nTr, CTrace* pTrace, LPCTSTR lbl = NULL,
      BOOL bDelayed = FALSE );
   BOOL ModifyGraph( int yMin,int yMax,CTrace* pTrace,   // update displayed or "delayed" graph's y-axis range & trace 
                     BOOL bDelayed = FALSE );            // attr; then redraw if modifying the displayed graph
   BOOL SetYAxisRange( const int yMin, const int yMax,   // change the displayed or "delayed" graph's y-axis range
                       BOOL bDelayed = FALSE ); 

   int GetDisplayedGraphWidth() const                    // current logical width of *displayed* graph
   {
      return( m_pDisplaySet->iWidth );
   }
   int GetDisplayedYAxisMin() const                      // current range of *displayed* graph's y-axis, in logical 
   {                                                     // coordinate system units
      return( m_pDisplaySet->iYMin );
   }
   int GetDisplayedYAxisMax() const 
   { 
      return( m_pDisplaySet->iYMax ); 
   }
   int GetDisplayedTraceCount() const                    // # of traces currently installed in *displayed* graph 
   { 
      return( m_pDisplaySet->nTraces );
   }

   int GetDelayedGraphWidth() const                      // similarly for the *delayed* graph, which is different only 
   {                                                     // if we're operating in the "delayed display" mode
      return( m_bDelayMode ? m_pDelayedSet->iWidth : m_pDisplaySet->iWidth );
   }
   int GetDelayedYAxisMin() const  
   { 
      return( m_bDelayMode ? m_pDelayedSet->iYMin : m_pDisplaySet->iYMin );
   }
   int GetDelayedYAxisMax() const 
   { 
      return( m_bDelayMode ? m_pDelayedSet->iYMax : m_pDisplaySet->iYMax ); 
   }
   int GetDelayedTraceCount() const 
   { 
      return( m_bDelayMode ? m_pDelayedSet->nTraces : m_pDisplaySet->nTraces );
   }

   BOOL UpdateGraph( int iElapsed, short* ppshBuf[] );   // update graph 
   VOID ResetGraph() { UpdateGraph( -1, NULL ); }        // reset (clear) graph
   VOID ShowDelayedTraces();                             // the "delayed" trace set becomes the "displayed" set

   
//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected: 
   VOID ReportSamplingFrequencyInTitle();                // sets control bar title to reflect current sampling freq
   VOID SetupCoords( CDC* pDC );                         // prepare DC for drawing in logical coordinate system 
   VOID DrawMargins( CDC* pDC );                         // draw y-axis and trace baseline symbols in left & rt margins 
   VOID DrawTraces( CDC* pDC, int iStart, int n,         // draw all or portion of currently installed traces
                    const BOOL bErase = FALSE );
   VOID DrawCurrentTimeline( CDC* pDC );                 // draw vertical line at the "current" time
   VOID DrawLabel(CDC* pDC);                             // draw the trace set label (in "delayed display" mode only)
};



#endif // !defined(GRAPHBAR_H__INCLUDED_)
