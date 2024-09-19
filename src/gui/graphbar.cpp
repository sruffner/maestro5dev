//===================================================================================================================== 
//
// graphbar.cpp : Implementation of class CGraphBar, a generic control bar class that plots discrete-time series y(t).
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// CGraphBar is a resizable control bar designed to plot up to 10 different discrete-time series y1(t), y2(t), etc. on 
// the same "time" scale.  Units of time are arbitrary:  each unit of time is taken as a single sample in the time 
// series.  In fact, CGraphBar can be used to plot generic (x,y) data.  However, it was designed with time series in 
// mind. 
//
// CGraphBar can display two kinds of time-series:  continuous, "analog" waveforms or impulse trains.  Associated with 
// each displayed trace are a few trace attributes:  display color, gain G, and baseline offset B.  If G==0, the trace 
// is interpreted as a pulse train, and a nonzero sample indicates the presence of a pulse during the time epoch 
// represented by that sample.  If G!=0, the trace is taken to be an analog waveform, and the displayed sample is 
// drawn at Y = B + sample*G if G>0 (multiplicative gain), or Y = B + sample/|G| if G<0 (divisive gain).
//
// ==> Modes of operation:  normal vs "delayed display" mode.
// In normal mode, one makes frequent and regular updates to the graph, using it like a "raster oscilloscope".  This 
// "continuous" mode of operation is possible because CGraphBar keeps track of the "current time" on the graph (ie, 
// where the next available sample is drawn).  The application provides UpdateGraph() with trace buffers whose length N 
// is typically much shorter than the graph's width.  In this case, UpdateGraph() appends the new trace segments to 
// the end of the currently drawn traces, then updates the current time accordingly.  When the current time exceeds the 
// graph width, the drawn traces merely wrap around to the left side of the graph.  This mode of usage permits the 
// display of continuously evolving time series for time periods much longer than the interval covered by the graph's 
// horizontal extent.
//
// In "delayed display" mode, CGraphBar statically displays the "last" trace set while a new trace set (with an 
// entirely different set of defined traces, x-axis extent, and y-axis range) is built in the background.  Once the 
// "delayed" trace set has been built, ShowDelayedTraces() is called to display them in the graph bar's client area. 
// Internally, CGraphBar provides two trace set buffers, one for the currently displayed trace set and one for the 
// "delayed" trace set that gets built up over time.  When ShowDelayedTraces() is called, the "delayed" set becomes the 
// "displayed" trace set, and the old displayed set is "lost".  In this mode, InitGraph() is called to define the 
// traces for the "delayed" trace set, UpdateGraph() is called one or more times to supply the trace data to be 
// displayed, then ShowDelayedTraces() is called to make the "delayed" trace set appear, erasing the old set.  Of 
// course, it makes little sense to "wrap" traces around to the beginning of the graph in this mode, so UpdateGraph() 
// ignores any raw trace samples for which T > the width of the graph as specified in the last call to InitGraph().
// 
// Unique to delayed mode is the ability to specify the time interval [t0..t1] displayed in the graph. This is useful 
// for "blowing up" one portion of a longer stream of data.  The interval [t0..t1] is specified in the call InitGraph(), 
// and UpdateGraph() ignores all data falling outside this interval.  In normal mode t0 is always 0 and t1 is taken as 
// the width of the graph.  In addition, CGraphBar will display an optional label in the delayed mode; this label is
// specified when InitGraph() is called and it serves to help identify the trace set (think: Maestro trial name!).
//
// In "normal" mode, the "delayed" trace set buffer is not used.  The "displayed" trace set is both updated and 
// displayed at the same time.
//
// ==> Aliasing of continuous waveforms; "binning" of pulse trains. 
// By design, CGraphBar restricts a "data sample" to a 2-byte integer, and its internal trace buffer has a fixed size 
// of 10000 samples.  The maximum number of traces that can be displayed is 10. The trace buffer is partitioned into 
// distinct sections for each displayed trace; thus, the more traces displayed, the fewer samples that can be stored 
// for each individual trace -- as little as 1000 samples per trace when all 10 traces are displayed.  Whenever the 
// width of the graph (in #samples) is greater than the per-trace buffer size, CGraphBar must subsample the raw data 
// stream.  For analog traces, all raw data samples falling within a single "bin" of the internal trace buffer are 
// averaged to get the subsampled value.  Analog data streams with high-frequency content will be aliased, and very 
// short-duration transients will be smoothed out.  For a pulse train trace, the internal trace buffer stores the 
// #pulses that occurred in each bin; the height of the vertical line drawn for a bin is proportional to the # of 
// pulses in that bin.  The resolution in the time of occurence of an individual pulse is limited to the bin size. The 
// effective sampling rate due to this memory limit is 10000/(W*D*N), where W is the display width in number of sample 
// intervals, D is the sample interval in seconds, and N is the number of traces displayed.  It should be noted that 
// CGraphBar's effective sampling rate is more often determined by pixellation of the display than by the internal 
// memory limitations.  The sampling rate due to pixellation is M/(W*D), where M is the display width in pixels.  As a 
// reminder to the user, CGraphBar reports the current "effective sampling rate" of the trace display in its title bar. 
//
// ==> Layout of CGraphBar's client area and the "logical coordinate system".
// The client area's "device coordinate system" is in pixels, with the origin at the top left corner of the client 
// area, and the y-axis increasing downward.  The drawing functions of a Windows "device context" (CDC), on the other 
// hand, use a logical coordinate system which we define in the protected method SetupCoords().  In our logical coord 
// system, the "graph region" covers the entire client area, with the exception of narrow margins on the left and right 
// sides.  "Time zero" is mapped to the pixel just right of the left margin, and "time" increases to the right.  The 
// minimum VISIBLE y-axis value maps to the bottom of the client area, the maximum visible y-axis value maps to the 
// top, and the y-axis increases upward (yMin MUST be <= yMax + 100).  For details, see SetupCoords(). 
//
// Support for axes, tick marks, etc. is very rudimentary.  No x-axis is displayed.  In the middle of the left and 
// right margins (on either side of the graph region), a simple white y-axis is shown with 9 equally spaced tick 
// marks.  For each trace, a small solid arrow is drawn in the margin pointing at the trace's baseline level.  For a 
// continuous data trace, the right-pointing arrow appears in the left margin; for pulse trains, the left-pointing 
// arrow is drawn in the right margin.  Axis or tick mark labels are not supported. However, in delayed mode, a
// trace set label (if specified) is drawn in the top left corner of the client area.
//
// In normal mode only, CGraphBar displays a a thin green vertical line marking the "current time".  It scrolls to the 
// right with each call to UpdateGraph(), wrapping around to the beginning whenever the current time exceeds the graph 
// width. 
//
// ==> Usage notes.
// 1) Control bar functionality.  CGraphBar is merely a control bar (MFC's CControlBar) that has been dedicated for a 
// specific use.  It is derived from a resizable scroll bar class, CSizingControlBarCF, that provides some features 
// similar to those available in Developer Studio control bars (see CREDITS).  To manipulate CGraphBar as a control 
// bar, use methods from CSizingControlBarCF and its underlying MFC hierarchy.
//
// 2) Public methods:
//    InitGraph() ==> This method configures the graph bar to display trace data.  Caller specifies the horizontal 
//    and vertical extents of the graph, the # of traces to be displayed, and their display attributes:  color, 
//    baseline offset, and gain.  Gain, G, applies only to continuous traces.  G==0 identifies a trace as an impulse 
//    train; G>0 is treated as a multiplicative gain; G<0 is treated as a divisive gain (1/|G|).  Since the gain must 
//    be an integer, floating-point operations are avoided.  Caller also specifies the mode of operation here.  In 
//    "delayed display" mode, the graph info supplied defines the "delayed" trace set configuration, NOT the 
//    "displayed" trace set.
//
//    ModifyGraph() ==> Modify existing trace attributes and y-axis range of either the "displayed" trace set or the 
//    "delayed" trace set (if in delayed display mode); in normal mode, the entire client area is redrawn.
//
//          !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//          !!! CGraphBar assumes that the order in which traces are defined in the trace attribute args 
//          !!! to InitGraph() & ModifyGraph() is the order of buffer pointers supplied to UpdateGraph()
//          !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
//
//    SetYAxisRange() ==> Changes the minimum and maximum visible y-axis values associated with the displayed or 
//    delayed trace sets.  In normal mode, the entire graph is redrawn IAW the changes.  NOTE that, while the vertical 
//    extent of the graph may be changed while the graph is "running", the horizontal extent may not!
//
//    Get***GraphWidth(), Get***YAxisMin(), Get***YAxisMax(), Get***TraceCount() ==> These are const methods which 
//    provide access to the current graph extents and the # of traces in the "displayed" and "delayed" trace sets.  In 
//    normal mode, the "delayed" values = the "displayed" values, since the "delayed" trace set does not exist.
//
//    ResetGraph() ==> In normal mode, this sets the # of displayed trace points to 0 and sets "current time" to zero; 
//    the graph's client area is cleared.  Trace attributes are not reset.  In "delayed display" mode, this merely 
//    reinitializes the "delayed" trace set's trace point array; the "displayed" set is unaffected.
//
//    UpdateGraph() ==> Process the supplied raw data buffers from the current time forward by the specified # of 
//    samples, ie, the "elapsed time" since the last update.  In normal mode, the displayed trace set buffer is 
//    updated IAW the new samples, and the corresponding interval in the graph is redrawn.  In "delayed display" mode, 
//    the delayed trace set buffer is updated, and the displayed graph is unaffected.  If the specified elapsed time 
//    is == the horizontal extent of the graph, the entire graph is redrawn (in normal mode).  Longer elapsed times 
//    make no sense and may lead to unexpected behavior.  Caller provides an array of pointers to the sample buffers 
//    -- one per installed trace. 
//
//
//
// DEVELOPMENT NOTES:
// 1) Originally designed CGraphBar to scroll the waveforms to the left -- behaving like an oscilloscope in "roll" 
// mode.  However, this is not as efficient and was subject to unpleasant flickering because you have to update the 
// entire graph on each scroll.  It might be done well using ScrollDC() or by employing a memory DC and bitblitting. 
// However, such an implementation will use a lot of memory, will require deleting and reallocating the memory DC 
// each time the client area is resized, and will be slower.  We decided to make it behave like the more traditional 
// "raster" oscilloscope mode.
// 2) Note that CGraphBar requires as many samples for a pulse train as it does for a "continuous" waveform".  This is 
// NOT very memory-efficient, especially when the pulses are not occurring too often.  However, this representation 
// meshes with the representation of continuous waveforms, and it considerably simplified the implementation.  Also, 
// for high-frequency pulse trains that have 1 or more pulses in every raw sample interval, this is the best way to go. 
//
//
// CREDITS:
// 1) CSizingControlBar: resizable control bars, by Cristi Posea [08/03/2000, www.datamekanix.com/sizecbar] --  An 
// implementation of DevStudio-like docking windows that can contain dialogs, controls, even views (although not 
// recommended.  CGraphBar is derived from this extension of MFC's CControlBar.
//
//
// REVISION HISTORY:
// 22feb2001-- Initial development begun.
// 25feb2001-- Completed work on a relatively simple graph that can draw up to 10 different traces and includes 
//             support for continuous scrolling.  Weaknesses:  Subsamples raw data to keep the # of plotted points 
//             manageable -- but that can result in aliasing or failure to plot transients.  Does not use a memory DC 
//             to reduce flicker (on the other hand -- not using a memory DC can save lots of memory if the client 
//             area is large!)
// 01mar2001-- Revised UpdateGraph() so that, for each subsampling interval in the set of drawn points, it includes the 
//             max and min values in that interval.  This ensures that graph does not entirely obscure short-duration 
//             "spikes", or transients, in the data.
// 02mar2001-- Next revision:  "smarter" update and paint mechanisms which only redraw the portion of the graph that 
//             absolutely MUST be redrawn.  we require that the min graph width (in logical units) be at least twice 
//             CGraphBar::MAXTRACEPTS...  Keeps track of a "current" time, so UpdateGraph() only specifies an elapsed 
//             time since the last update... Instead of scrolling data -- like "roll" mode on an oscilloscope -- it 
//             now behaves more like a traditional oscilloscope trace.  A narrow blank region extending the entire 
//             height of graph serves as a "current" time cursor.  It sweeps across the screen as the leading edge of 
//             the evolving traces; the "older" data to the right of the time cursor gets erased.  This looks a LOT 
//             better than scrolling!
// 08mar2001-- Next revision:  If fewer than MAXTRACES are in use, why not increase the max allowable points per trace 
//             in the internal trace point array.  This would require that MAXTRACEPTS become a variable.  Since the 
//             only way to change the # of traces is by completely reinitializing the graph w/ InitGraph(), this should 
//             be a relatively easy change.  CGraphBar's performance -- in terms of accurately depicting the raw data 
//             -- degrades more gracefully as the # of traces increases.... DONE!
// 13mar2001-- Next: Add support to display unit data.  This would be a different kind of trace represented by 
//             disjoint, short vertical lines.  Problem:  what if there are more than MAXTRACEPTS unit events in the 
//             "raw" data buffer at any one time??  Solution:  When there is more than one pulse in an interval that 
//             must be drawn as a single pulse line, set line height to #pulses*htOfOnePulse.....  DONE!
//          -- Allow changes to the y-axis range via SetYAxisRange().  Thus, y = 0 would not always have to be at the 
//             line halfway down the client area.  This change could happen even while animating traces -- although 
//             the entire client area would have to be redrawn... DONE!
// 07aug2001-- Major mod:  Instead of internally saving buffer ptrs to the trace data, callers provide a single buffer 
//             ptr to UpdateGraph().  Trace data are expected to be interleaved in that buffer.  Also, got rid of 
//             SetTrace() -- trace attributes are now set in InitGraph().
// 08aug2001-- Major mod, AGAIN:  UpdateGraph() required the entire trace buffer for each installed trace, which was 
//             treated as a circular buffer.  One of the reasons for this was that UpdateGraph() sometimes could not 
//             use all the samples offered, and so had to "make up some time" on a subsequent update.  Modified the 
//             implementation so that UpdateGraph() uses all or none of the samples provided, and now caller only 
//             provides buffers containing the new samples.  Also, there is a separate buffer for each trace, rather 
//             than interleaved buffers.
// 25sep2001-- Fixed a problem with positioning of trace baseline arrows in DrawMargins().
// 02oct2001-- Added ModifyGraph(). 
// 06oct2003-- Starting major mod to support displaying an old trace set while preparing a new one for display -- this 
//             is how we'd like the trace display to operate in trial mode.
// 13oct2003-- Minor mod re: how pulse trains are stored in the internal trace point arrays.  For each point (tN, yN), 
//             yN is merely set to the #pulses that occurred during the interval tN to t(N+1).  Before this, we set 
//             yN = trace baseline when no pulses occurred, causing a subtle bug:  if you decreased the trace baseline, 
//             suddenly every bin has many pulses in it!
// 02dec2003-- Removed requirement that graph width be an even number.
// 20sep2004-- Modified to support display of two time markers as vertical white lines in delayed mode only.
// 08mar2005-- Completed redesign:  Trace point buffer replaced by a short-valued sample buffer, which contains a 
//             possibly subsampled version of the original raw data streams.  This design requires more computations 
//             whenever the trace display is painted, but tests have indicated this is not a problem.  With the new 
//             design, we ensure that at least 1000 bins are dedicated to each displayed trace while actually reducing 
//             the memory cost of the implementation.
// 29apr2005-- Modified to permit display of a specific "time" interval [t0..t1], in delayed mode only.
// 30mar2009-- Added trace set "label", applicable only in the "delayed" mode. This will let us annotate the trace
//             display with the name of the Maestro trial that was run when the data was collected.
// 31aug2017-- Various changes to support 64-bit compilation in VStudio 2017, targeting Windows 10 64-bit OS.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "graphbar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CGraphBar
//
//===================================================================================================================== 
//===================================================================================================================== 
//
BEGIN_MESSAGE_MAP( CGraphBar, CSizingControlBarCF )
   ON_WM_CREATE()
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CGraphBar [constructor] ========================================================================================= 
//
//    The graph control bar's two internal trace sets are each set up with no traces and a NULL trace data buffer.
//
CGraphBar::CGraphBar()
{
   for( int i=0; i < 2; i++ )             // initialize our two internal trace sets
   {
      m_traceSet[i].iWidth = MINWIDTH;
      m_traceSet[i].t0 = 0;
      m_traceSet[i].t1 = MINWIDTH;
      m_traceSet[i].iYMin = -100;
      m_traceSet[i].iYMax = 100;
      m_traceSet[i].nTraces = 0;          //    no traces installed initially
	  m_traceSet[i].strLabel = _T("");    //    empty trace set label
      m_traceSet[i].pTraceData = NULL;    //    storage for trace data is alloc'd upon creation of control bar
      m_traceSet[i].nMaxBins = 0;
      m_traceSet[i].nBins = 0;
      m_traceSet[i].tCurrent = 0;
      m_traceSet[i].iNextBin = 0;
   }

   m_pDisplaySet = &(m_traceSet[0]);      // one set is displayed, one set is delayed
   m_pDelayedSet = &(m_traceSet[1]);

   m_rawSampleIntvMS = 1;
   m_currSampleFreqHz = (float) 0;

   m_bDelayMode = FALSE;
   m_bNormMargin = FALSE;

   for( int j=0; j < MAXTRACES; j++ ) 
   {
      m_accumBin[j] = 0;
      m_perBin[j] = 0;
   }
}



//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

//=== OnCreate [base override] ======================================================================================== 
//
//    Response to WM_CREATE message.  Here's where we allocate internal storage for all trace data.  If unable to do 
//    so, we abort the creation process.
//
//    ARGS:       lpcs  -- [in] creation parameters
//
//    RETURNS:    -1 to abort creation; 0 to continue. 
//
int CGraphBar::OnCreate( LPCREATESTRUCT lpcs )
{
   if( CSizingControlBarCF::OnCreate( lpcs ) == - 1 )                               // let base class go first 
      return( -1 );                                                                 // ..abort on failure 

   for( int i = 0; i < 2; i++ )
   {
      ASSERT( m_traceSet[i].pTraceData == NULL );

      m_traceSet[i].pTraceData = new short[MAXTRACES*MINBINS];                      // attempt allocation
      if( m_traceSet[i].pTraceData == NULL )                                        // ...abort on failure
         return( -1 );

      ASSERT( AfxIsValidAddress( m_traceSet[i].pTraceData, MAXTRACES*MINBINS*sizeof(short) ) );
   }

   return( 0 );
}


//=== OnPaint [base override] ========================================================================================= 
//
//    Response to WM_PAINT message.  Here we assume the entire client area has already been erased (OnEraseBkgnd()), 
//    and we redraw all traces in the displayed trace set completely, as well as the y-axis and baseline arrows in the 
//    side margins.
//
//    In normal mode we draw a vertical line at the "current time", since the graph bar is intended to update 
//    continuously, somewhat like a raster oscilloscope.  In "delayed display" mode, the delayed trace set is 
//    painted in one go and remains there statically until the next "delayed" set is ready for display; in this case, 
//    we omit the "current time" line. 
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CGraphBar::OnPaint()
{
   CPaintDC dc( this );

   SetupCoords( &dc );
   DrawMargins( &dc );
   if( m_pDisplaySet->nBins > 0 )
   {
      DrawTraces( &dc, 0, m_pDisplaySet->nBins ); 
      DrawCurrentTimeline( &dc );
   }
   DrawLabel(&dc);
}


//=== OnSize [base override] ========================================================================================== 
//
//    Response to the WM_SIZE message.  Whenever the control bar is resized, we must repaint the entire client area.  
//    Also, if the trace display is in use, we revise the control bar title to reflect its new sampling frequency.
//
//    ARGS:       pDC   -- [in] ptr to device context 
//
//    RETURNS:    TRUE to indicate background was erased; FALSE otherwise. 
//
void CGraphBar::OnSize( UINT nType, int cx, int cy )
{
   // repaint entire client area
   Invalidate(TRUE);

   // update title to reflect sampling frequency, which may have changed as a result of the resize.
   ReportSamplingFrequencyInTitle();
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
BOOL CGraphBar::OnEraseBkgnd( CDC *pDC )
{
   CRect rect;
   GetClientRect( rect );
   pDC->FillSolidRect( rect, RGB(0, 0, 0) );
   return( TRUE );
}



//===================================================================================================================== 
// OPERATIONS/ATTRIBUTES
//===================================================================================================================== 

//=== InitGraph ======================================================================================================= 
//
//    Initialize (or reinitialize) the graph, setting its logical extent, the # of active traces, and their attributes. 
//
//    App should provide the graph's x-axis and y-axis extents in "logical units":  The x-axis extent is specified as 
//    a time interval [t0..t1] in "ticks", where one tick is the interval between samples in the data stream supplied 
//    via calls to UpdateGraph().  In normal mode, t0 is ignored and is always set to 0.  The graph width (t1-t0) MUST 
//    be in the range [100..32767).  The y-axis represents the "value" of a continuous waveform as a function of time; 
//    allowed values for yMin and yMax lie in [-32767....32767], and yMin MUST be <= yMax + 100. 
// 
//    The logical-to-device translations are configured so that the graph fills most of the region, with small margins 
//    on the right and left.  The x-coord of the origin of the logical coordinate system is always at the right edge of 
//    the left margin; its y-coord depends on the specified y-axis range; the y-axis always increases upwards. 
//
//    The graph may operate in one of two modes.  In normal mode, the displayed traces are updated on the fly.  The 
//    graph is cleared initially, and it is updated as trace data is supplied via UpdateGraph().  If the data timeline 
//    does beyond the graph width specified here, then the traces "wrap around" to the left side of the graph -- like a 
//    raster oscilloscope.  In "delayed display" mode, the last set of traces displayed are maintained on the graph 
//    while the new set of traces specified here are updated invisibly via calls to UpdateGraph().  The "delayed" trace 
//    set is not displayed until ShowDelayedTraces() is invoked.  If ShowDelayedTraces() is NOT invoked before the next 
//    call to InitGraph(), the delayed trace set will never be displayed!.  While updating, portions of the data 
//    stream outside the specified interval [t0..t1] are simply ignored.
//
//    ARGS:       yMin     -- [in] minimum y-coord visible on graph (MUST be 100 less than max). 
//                yMax     -- [in] maximum y-coord visible on graph. 
//                t0,t1    -- [in] the time or x-coord interval [t0..t1] displayed in graph.  In normal mode, t0 is 
//                            ignored and is always set to 0, while t1 is the graph width; in this mode, time "wraps" 
//                            around back to zero once t1 is exceeded.  In delayed mode, the graph width is t1-t0, and 
//                            CGraphBar will only show the portion of the data stream that lies in the interval.  Of 
//                            course, t1 must be > t0.
//                intv     -- [in] the actual sample interval in ms.  Since CGraphBar will typically subsample the raw 
//                            data, this information is needed to report the effective sampling frequency of the 
//                            display.  See ReportSamplingFrequencyInTitle(). 
//                nTr      -- [in] # of traces that will be drawn on graph. 
//                pTrace   -- [in] trace attributes (#elements MUST be >= nTr!!).  ignored if nTr == 0.
//                lbl      -- [in] trace set label for "delayed display" mode only (default = NULL). Ignored in 
//                            normal display mode.
//                bDelayed -- [in] TRUE to operate in "delayed display" mode (default = FALSE).
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad args, or trace point array not yet allocated).  
//
BOOL CGraphBar::InitGraph( int yMin, int yMax, int t0, int t1, int intv, int nTr, CTrace* pTrace, 
                          LPCTSTR lbl /* =NULL */, BOOL bDelayed /* =FALSE */ )
{
   CTraceSet* pSet = bDelayed ? m_pDelayedSet : m_pDisplaySet;       // which trace set are we configuring

   int my_t0 = bDelayed ? t0 : 0;                                    // t0 ALWAYS 0 in normal mode!
   int width = t1 - my_t0;

   if( width < MINWIDTH || width > 32766 ||                          // check constraints...
       yMin < -32767 || yMax > 32767 || yMin > (yMax + 100) || 
       nTr < 0 || nTr > MAXTRACES ||
       pSet->pTraceData == NULL 
     )
      return( FALSE );

   m_bDelayMode = bDelayed; 
   m_rawSampleIntvMS = (intv < 1) ? 1 : intv;

   pSet->iWidth = width;
   pSet->t0 = my_t0;
   pSet->t1 = t1;
   pSet->iYMin = yMin;
   pSet->iYMax = yMax;
   pSet->nTraces = nTr;

   if( pSet->nTraces == 0 ) pSet->nMaxBins = MINBINS;                // max # of stored samples per trace
   else pSet->nMaxBins = (MAXTRACES*MINBINS) / pSet->nTraces; 

   if( pSet->nMaxBins > pSet->iWidth )                               // total #bins need never exceed graph width!
      pSet->nMaxBins = pSet->iWidth; 

   for( int i = 0; i < pSet->nTraces; i++ )                          // save trace attributes internally
      pSet->trace[i] = pTrace[i];

   if(bDelayed && lbl != NULL)  pSet->strLabel = lbl;                // trace set label (delayed mode only)
   else pSet->strLabel = _T("");
   
   ResetGraph();                                                     // clear trace set about to be populated with data 
   return( TRUE );
}


//=== ModifyGraph ===================================================================================================== 
//
//    Update y-axis extents and existing trace attributes of the currently displayed graph trace set OR, in "delayed 
//    display" mode only, the "delayed" trace set.  If displayed trace set attributes or y-axis extents are modified, 
//    then we redraw entire graph accordingly.  The graph is NOT reinitialized.  See also InitGraph().
//
//    ARGS:       yMin     -- [in] minimum y-coord visible on graph (MUST be 100 less than max). 
//                yMax     -- [in] maximum y-coord visible on graph. 
//                pTrace   -- [in] trace attributes (#elements MUST be >= # of traces currently installed).  if NULL, 
//                            only y-axis extents are updated.
//                bDelayed -- [in] TRUE to update the "delayed" graph trace set; FALSE to update the currently 
//                            displayed trace set (default = FALSE).  if TRUE and we're not in the "delayed display" 
//                            mode, the method does nothing!
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad args, trace point array not yet allocated, etc).  
//
BOOL CGraphBar::ModifyGraph( int yMin, int yMax, CTrace* pTrace, BOOL bDelayed /* =FALSE */ )
{
   if( bDelayed && !m_bDelayMode ) return( FALSE );               // the delayed trace set is not used in normal mode!! 

   CTraceSet* pSet = bDelayed ? m_pDelayedSet : m_pDisplaySet;    // which trace set are we modifying?

   if( pSet->nTraces == 0 || 
       yMin < -32767 || yMax > 32767 || yMin > (yMax + 100) || 
       pSet->pTraceData == NULL 
     )
      return( FALSE );

   pSet->iYMin = yMin;
   pSet->iYMax = yMax;
   if( pTrace != NULL )
   {
      for( int i = 0; i < pSet->nTraces; i++ ) pSet->trace[i] = pTrace[i]; 
   }

   if( !bDelayed ) Invalidate( TRUE );                            // repaint client area only if mod displayed set!
   return( TRUE );
}


//=== SetYAxisRange =================================================================================================== 
//
//    Change the range for the y-axis of the currently displayed graph OR, in "delayed display" mode only, the delayed 
//    graph.  The new axis limits (yMin, yMax) must lie in the range [-32767 .. 32767] and satisfy yMin <= yMax + 100. 
//    If the displayed graph's y-axis is changed, it is entirely redrawn to reflect the changes. 
//
//    ARGS:       yMin     -- [in] minimum y-coord visible on graph (MUST be 100 less than max). 
//                yMax     -- [in] maximum y-coord visible on graph. 
//                bDelayed -- [in] TRUE to update the "delayed" graph trace set; FALSE to update the currently 
//                            displayed trace set (default = FALSE).  if TRUE and we're not in the "delayed display" 
//                            mode, the method does nothing!
//
//    RETURNS:    TRUE if successful; FALSE otherwise (bad args).  
//
BOOL CGraphBar::SetYAxisRange( const int yMin, const int yMax, BOOL bDelayed /* =FALSE */ )
{
   if( bDelayed && !m_bDelayMode ) return( FALSE );               // the delayed trace set is not used in normal mode!! 
   if( yMin < -32767 || yMax > 32767 || yMin > (yMax + 100) )     // check constraints...
      return( FALSE );

   CTraceSet* pSet = bDelayed ? m_pDelayedSet : m_pDisplaySet;    // which trace set are we modifying?

   pSet->iYMin = yMin;                                            // set new range 
   pSet->iYMax = yMax;
   if( !bDelayed ) Invalidate( TRUE );                            // repaint only if we changed the displayed graph!
   return( TRUE );
}


//=== UpdateGraph ===================================================================================================== 
// 
//    Advance the "current" time by the specified interval and update the displayed or delayed graph trace set 
//    accordingly.
//
//    This is the heart of the graph animation.  It updates a portion (or possibly all) of an internal trace buffer 
//    IAW the E samples in the supplied data stream buffers.  Memory constraints may require that the raw data stream 
//    be subsampled.  For analog traces, all raw data samples falling within a single bin of the internal trace buffer 
//    are averaged.  For pulse train traces, we simply count the total #pulses that occurred in a single bin.
//
//    Which set of traces is updated in this method?  That depends on the mode of operation.  In normal mode, the 
//    displayed trace set is updated and the new trace segments are displayed in the client area.  In "delayed display" 
//    mode, the previous trace set is displayed statically while this method updates the "delayed" trace set;  this 
//    second trace set is UNUSED in normal mode.
//
//    What happens when the traces reach the end of the graph?  Again, this depends on the mode of operation.  In 
//    normal mode, our strategy is to "wrap" time -- where time is specified in # of data samples! -- back to the left 
//    side (t0 = 0) of the graph.  Thus, if the graph's horizontal extent is W samples, T is the "current" time in 
//    samples, and E new samples are provided, the new "current" time after UpdateGraph() returns will be (T+E) % W. 
//    (Note that the interval of "real" time per sample is completely arbitrary and need not in fact be a time value 
//    at all!).  This "wrapping time" feature is fundamental to CGraphBar and permits its use in displaying waveforms 
//    evolving continuously for long periods of time.  In such an application, it is important to call UpdateGraph() on 
//    a regular basis at intervals << W.
//
//    In "delayed display" mode, there is NO wrap-around.  CGraphBar tracks the current time T, and only trace data 
//    lying in the interval [t0..t1] is stored in the delayed trace set buffer.  All other trace data supplied by 
//    calls to UpdatedGraph() are ignored until the graph is reinitialized by invoking InitGraph().
//
//    To clear the trace set, call this method with a negative or zero elapsed time.  This is a quick way to "start 
//    from scratch".
//
//    ARGS:       iElapsed -- [in] elasped time since last update (in logical units).  An elapsed time <= 0 resets the 
//                            graph.  In normal mode of operation, this MUST be less than or equal to graph width.
//                ppshBuf  -- [in] the trace buffers (array of pointers to 'short'). 
//
//                   !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                   !!! Size of array must >= # of installed traces.  Short pointer ppshBuf[N] must point  
//                   !!! to the head of the data buffer for installed trace #N, and each data buffer must 
//                   !!! contain >= iElapsed samples.  Failure to meet these constraints may cause a general 
//                   !!! protection fault.
//                   !!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//    RETURNS:    TRUE if successful, FALSE if elapsed time was too short to merit an update. 
//
BOOL CGraphBar::UpdateGraph( int iElapsed, short* ppshBuf[] )
{
   CTraceSet* pSet = m_bDelayMode ? m_pDelayedSet : m_pDisplaySet;   // which trace set are we updating?

   if( iElapsed <= 0 )                                               // special case: clear all traces! 
   { 
      pSet->nBins = 0; 
      pSet->tCurrent = 0;
      pSet->iNextBin = 0;
      for( int i=0; i<MAXTRACES; i++ ) 
      {
         m_accumBin[i] = 0;
         m_perBin[i] = 0;
      }
      ReportSamplingFrequencyInTitle();                              // show sampling frequency in title bar
      if( !m_bDelayMode ) Invalidate( TRUE );                        // repaint only if we're in normal mode!
      return( TRUE );
   }

   int offset = 0;                                                   // in "delayed display" mode, ignore data that is 
   if( m_bDelayMode )                                                // not within "time" interval spanned by graph.
   {                                                                 // We may have to ignore a portion of the supplied 
      if( (pSet->tCurrent + iElapsed) <= pSet->t0 )                  // data at the beginning or end of the buffer!
      {
         pSet->tCurrent += iElapsed;
         return( TRUE );
      }
      else if( pSet->tCurrent < pSet->t0 )
      {
         offset = pSet->t0 - pSet->tCurrent;
         pSet->tCurrent = pSet->t0;
         iElapsed -= offset;
      }

      if( pSet->tCurrent >= pSet->t1 )
         return( TRUE );
      else if( pSet->tCurrent + iElapsed > pSet->t1 )
         iElapsed = pSet->t1 - pSet->tCurrent;
   }
   else if( iElapsed > pSet->iWidth )                                // if "normal" mode, #samples must always be 
      iElapsed = pSet->iWidth;                                       // less than or equal to the graph width!

   int tNewCurr = pSet->tCurrent + iElapsed;                         // the new "current" time, in #samples
   if( !m_bDelayMode )                                               // in normal mode, we "wrap" around
      tNewCurr %= pSet->iWidth;

   int iBinTo= (tNewCurr - pSet->t0) * pSet->nMaxBins/pSet->iWidth;  // corresponding bin in subsampled trace buffers
   int nBins = (pSet->iNextBin <= iBinTo) ?                           // # of bins to be updated in trace buffers
                  (iBinTo - pSet->iNextBin) :                        // (does not include last bin)
                  (pSet->nMaxBins - pSet->iNextBin + iBinTo);

   if( nBins < 4 ) return( FALSE );                                  // elapsed time too short; postpone update

   int i, j, k;
   short *pshData;                                                   // ptr to head of a data trace buffer
   short shSamp;                                                     // the current raw data sample
   BOOL isAnalog;                                                    // is current trace analog?
   int iCurrBin, iNextBin;                                           // bin counters

   for( j = 0; j < pSet->nTraces; j++ )                              // FOR EVERY TRACE...
   {
      k = j * pSet->nMaxBins;                                        // start of trace buffer for this trace
      isAnalog = BOOL(pSet->trace[j].iGain != 0);
      pshData = ppshBuf[j];

      iCurrBin = pSet->iNextBin;                                     // the current bin (we could be in middle of it!) 
      for( i=0; i<iElapsed; i++ )                                    // process the new raw data
      {
         shSamp = pshData[i+offset];                                 //    next raw data sample
         if( !isAnalog )                                             //    for pulse train, datum is just a flag that 
            shSamp = (shSamp!=0) ? 1 : 0;                            //    indicates presence/absence of a pulse

         m_accumBin[j] += shSamp;                                    //    accumulate raw data for current bin
         ++m_perBin[j];

         iNextBin = (pSet->tCurrent-pSet->t0 + i+1) % pSet->iWidth;  //    map time of sample AFTER THIS ONE to approp 
         iNextBin = iNextBin * pSet->nMaxBins / pSet->iWidth;        //    bin in our subsampled internal store

         if( iNextBin != iCurrBin )                                  //    if next sample will be in a different bin, 
         {                                                           //    set current bin to accum value and move on
            if( isAnalog && (m_perBin[j] > 0) )
            { 
               m_accumBin[j] /= m_perBin[j];                         //    for analog traces, we calc the avg over bin 
            }
            pSet->pTraceData[k+iNextBin] = (short) m_accumBin[j];
            m_accumBin[j] = 0;
            m_perBin[j] = 0;
            iCurrBin = iNextBin;
         }
      }
   }                                                                 // END:  FOR EVERY TRACE...

   
   if( pSet->nBins < pSet->nMaxBins )                                // if we have yet to cycle over the internal trace 
   {                                                                 // buffer since the last graph reset, then update 
      pSet->nBins += nBins;                                          // total # of bins in use
      if( pSet->nBins > pSet->nMaxBins ) 
         pSet->nBins = pSet->nMaxBins;
   }

   pSet->tCurrent = tNewCurr;                                        // update the "current" time 

   if( !m_bDelayMode )                                               // in normal mode, draw the just-updated trace 
   {                                                                 // segments
      CClientDC dc( this ); 
      SetupCoords( &dc ); 
      DrawTraces( &dc, pSet->iNextBin, nBins, TRUE );                // erases old stuff & handles wrap-around!
      pSet->iNextBin = iBinTo; 
      DrawCurrentTimeline( &dc );                                    // draw the time cursor at its new location 
   }
   else                                                              // in "delayed display" mode, we just update 
      pSet->iNextBin = iBinTo;                                       // our position in the internal trace data buffer

   return( TRUE );
}


//=== ShowDelayedTraces =============================================================================================== 
//
//    In the "delayed display" mode, invoke this function to physically display the trace set that was updated in the 
//    background by previous calls to UpdateGraph() since the last call to InitGraph().  Internally, this method 
//    merely swaps the "delayed" and "currently displayed" trace sets.
//
//    Since the previously displayed trace set should never be displayed again, its trace info is reset.  If the 
//    method is called again before building a new "delayed" trace set, the graph is cleared of any trace data. 
//
//    The method has NO effect in normal display mode.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//
//    RETURNS:    NONE 
//
VOID CGraphBar::ShowDelayedTraces()
{
   if( !m_bDelayMode ) return;

   m_pDisplaySet->nTraces = 0;                        // essentially reset trace data in the old display set
   CTraceSet* pTemp = m_pDisplaySet;                  // swap the displayed and "delayed" trace sets
   m_pDisplaySet = m_pDelayedSet;
   m_pDelayedSet = pTemp;

   Invalidate( TRUE );                                // repaint entire client area
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== ReportSamplingFrequencyInTitle ================================================================================== 
//
//    Recalculate the effective sampling frequency of the trace display.  If it has changed, update CGraphBar's title 
//    to display this frequency in Hz following the word "Data Traces".  When the trace display is not in use, the 
//    sampling frequency is 0 and is NOT reported in CGraphBar's title.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CGraphBar::ReportSamplingFrequencyInTitle()
{
   // if trace in use, calculate effective sampling rate; else, effective sampling rate is 0
   float freq = 0.0f;
   if( m_pDisplaySet->nTraces > 0 && m_pDisplaySet->nMaxBins > 0 )
   {
      CRect rClient;
      GetClientRect( &rClient );
      int cx = rClient.right - rClient.left;

      float fRes = 0.001f * ((float) m_pDisplaySet->iWidth * m_rawSampleIntvMS);
      fRes /= (float) m_pDisplaySet->nMaxBins;

      int margin = BOOL(cx > 10*XMARGINSZ) ? XMARGINSZ : 1;
      int wPix = cx - 2 - 4*margin;
      float fResPix = 0.0f;
      if( wPix > 0 ) 
         fResPix = 0.001f * ((float) m_pDisplaySet->iWidth * m_rawSampleIntvMS) / ((float) wPix);

      if( fResPix > fRes ) fRes = fResPix;

      freq = 1.0f / fRes;
   }

   // if effective sampling rate changed, update our title bar
   if( m_currSampleFreqHz != freq )
   {
      m_currSampleFreqHz = freq;
      CString strRes = _T("Data Traces");
      if( m_currSampleFreqHz != (float)0 )
         strRes.Format( "Data Traces (%.1f Hz)", m_currSampleFreqHz );
      SetWindowText( (LPCTSTR) strRes );
   }
}


//=== SetupCoords ===================================================================================================== 
//
//    Set up logical-to-device translations for the device context such that:  the logical extent of the client area is 
//    the current graph size, with narrow left and right margins; the logical origin is at the right edge of the left 
//    margin, positioned vertically IAW the current y-axis limits; the y-axis increases upwards rather than downwards.
//
//    The left and right margins are the same width.  The y-axis is drawn in both margins, baseline symbols for analog 
//    traces are drawn in the left margin, and baseline symbols for spike trains are drawn in the left margin.  These 
//    elements are not drawn if the margins are reduced due to a very narrow client area.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect
//
//    RETURNS:    NONE 
//
VOID CGraphBar::SetupCoords( CDC* pDC )
{
   CRect rClient;
   GetClientRect( &rClient );

   pDC->SetMapMode( MM_ANISOTROPIC );                                // x- and y-axes are independent...
   pDC->SetWindowExt( m_pDisplaySet->iWidth, (m_pDisplaySet->iYMax - m_pDisplaySet->iYMin) ); 
   m_bNormMargin = BOOL(rClient.right > 10*XMARGINSZ);
   int margin = (m_bNormMargin) ? XMARGINSZ : 1;
   pDC->SetViewportExt( rClient.right-2 - 4*margin, -rClient.bottom );
   pDC->SetViewportOrg( rClient.left + 2*margin, rClient.bottom / 2 );
   pDC->SetWindowOrg( 0, (m_pDisplaySet->iYMax + m_pDisplaySet->iYMin) / 2 );
}


//=== DrawMargins ===================================================================================================== 
//
//    A simple y-axis is drawn in both the left and right margins of the graph, with 9 tick marks equally spaced along 
//    its length.  In addition, a "baseline arrow" is drawn in the left margin for each analog trace currently under 
//    display; for each spike train displayed, the baseline arrow is drawn in the right margin.  This method is 
//    responsible for drawing these static plot elements, which need only be redrawn when the client area is resized or 
//    "unhidden" (WM_PAINT).  The client area is laid out so that the graph traces never fall into these margins.
//
//    The y-axis line is drawn in the middle of each margin, which is XMARGINSZ * 2 pixels wide.  The tick marks extend 
//    toward, but do not touch, the right(left) edge of the left(right) margin.  The right-pointing baseline arrows for 
//    continuous traces are drawn to the left of the left axis, with the arrow point at the trace's current DC offset; 
//    similarly, the left-pointing baseline arrows for the pulse trains are drawn to the right of the right axis.  Each 
//    (solid) arrow is drawn in the same color as the corresponding trace.
//
//    NOTE: To keep the static graphic elements in the margins constant despite size changes in the client area, we 
//    have to specify their extents first in pixels (device coords), then convert to logical coords via CDC:DPtoLP(). 
//    An alternative would be to use the default DC (which uses the standard device coord system for the client area) 
//    -- that is, pass to DrawMargins() a device context that has NOT been transformed by SetupCoords() -- and then 
//    specify all dimensions as pixels.  We elected not to do so, because we'd still have to do manual logical-device 
//    translations on the trace offset values (which are in logical units) to draw the baseline arrows correctly.  
//    Also, the implementation is more consistent:  SetupCoords() is always called as soon as we create a device 
//    context for the client area, and all drawing code in CGraphBar assumes the provided DC has been transformed by 
//    SetupCoords().
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//
//    RETURNS:    NONE 
//
VOID CGraphBar::DrawMargins( CDC* pDC )
{
   if( !m_bNormMargin ) return;                                         // if margin not big enough, do nothing!

   CRect rClient;
   GetClientRect( &rClient );

   int m = rClient.bottom / 2;
   int h = XMARGINSZ/2 + 1;
   int iOffset;

   CPoint ptLfArrow[3];                                                 // arrow in left margin, point on axis & in    
   ptLfArrow[0] = CPoint( 0, m-h );                                     // line w/device origin, in device coords...
   ptLfArrow[1] = CPoint( XMARGINSZ, m );
   ptLfArrow[2] = CPoint( 0, m+h );
   pDC->DPtoLP( ptLfArrow, 3 );                                         // ...then converted to logical units
   iOffset = ptLfArrow[1].y;                                            // ...then moved to logical y = 0
   ptLfArrow[1].y = 0;
   ptLfArrow[0].y -= iOffset;
   ptLfArrow[2].y -= iOffset;
   
   CPoint ptRtArrow[3];                                                 // arrow in right margin, point on axis & in  
   ptRtArrow[0] = CPoint( rClient.right-1, m-h );                       // line w/device origin, in device coords...
   ptRtArrow[1] = CPoint( rClient.right-1-XMARGINSZ, m );
   ptRtArrow[2] = CPoint( rClient.right-1, m+h );
   pDC->DPtoLP( ptRtArrow, 3 );                                         // ...then converted to logical units
   iOffset = ptRtArrow[1].y;                                            // ...then moved to logical y = 0
   ptRtArrow[1].y = 0;
   ptRtArrow[0].y -= iOffset;
   ptRtArrow[2].y -= iOffset;
   

   CPoint ptLfAxis[2];                                                  // left axis in center of left margin, 
   ptLfAxis[0] = CPoint( XMARGINSZ, 1 );                                // extending almost entire client rect
   ptLfAxis[1] = CPoint( XMARGINSZ, rClient.bottom-1 );
   pDC->DPtoLP( ptLfAxis, 2 ); 

   CPoint ptLfTick[2];                                                  // left axis horiz ticks drawn toward, but 
   ptLfTick[0] = CPoint( XMARGINSZ, m );                                // not touching, right edge of left margin
   ptLfTick[1] = CPoint( 2*XMARGINSZ, m );
   pDC->DPtoLP( ptLfTick, 2 ); 

   CPoint ptRtAxis[2];                                                  // right axis in center of right margin, 
   ptRtAxis[0] = CPoint( rClient.right-1-XMARGINSZ, 1 );                // extending almost entire client rect 
   ptRtAxis[1] = CPoint( rClient.right-1-XMARGINSZ, rClient.bottom-1 );
   pDC->DPtoLP( ptRtAxis, 2 ); 

   CPoint ptRtTick[2];                                                  // right axis horiz ticks drawn toward, but 
   ptRtTick[0] = CPoint( rClient.right-1-XMARGINSZ, m );                // not touching, left edge of right margin 
   ptRtTick[1] = CPoint( rClient.right-1-2*XMARGINSZ, m );
   pDC->DPtoLP( ptRtTick, 2 ); 


   CPen* pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );         // use stock white pen for axis

   pDC->MoveTo( ptLfAxis[0].x, ptLfAxis[0].y );                         // draw the two axis lines...
   pDC->LineTo( ptLfAxis[1].x, ptLfAxis[1].y );
   pDC->MoveTo( ptRtAxis[0].x, ptRtAxis[0].y ); 
   pDC->LineTo( ptRtAxis[1].x, ptRtAxis[1].y );

   int yExtent = m_pDisplaySet->iYMax - m_pDisplaySet->iYMin;           // ... and their 9 horizontal tick marks
   int yCenter = (m_pDisplaySet->iYMax + m_pDisplaySet->iYMin) / 2;
   int i, j;
   for( i = 0; i < 5; i++ )
   {
      j = (i * yExtent) / 10;
      pDC->MoveTo(  ptLfTick[0].x,  yCenter + j );
      pDC->LineTo(  ptLfTick[1].x,  yCenter + j );
      pDC->MoveTo(  ptRtTick[0].x,  yCenter + j );
      pDC->LineTo(  ptRtTick[1].x,  yCenter + j );
      if( j != 0 )
      {
         pDC->MoveTo(  ptLfTick[0].x, yCenter - j );
         pDC->LineTo(  ptLfTick[1].x, yCenter - j );
         pDC->MoveTo(  ptRtTick[0].x, yCenter - j );
         pDC->LineTo(  ptRtTick[1].x, yCenter - j );
      }
   }
   pDC->SelectObject( pOldPen );                                        // restore old pen 


   CBrush *pOldBrush;
   CPen currPen;                                                        // we use custom pen & brush to draw arrows
   CBrush currBrush;

   for( i = 0; i < m_pDisplaySet->nTraces; i++ )                        // DRAW THE BASELINE ARROW FOR EACH TRACE:
   {
      if( currPen.CreatePen(PS_SOLID,0,m_pDisplaySet->trace[i].color) ) // create custom pen & select into DC
         pOldPen = (CPen*) pDC->SelectObject( &currPen );
      else
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );         // stock white pen if can't make custom one

      if( currBrush.CreateSolidBrush( m_pDisplaySet->trace[i].color ) ) // create custom brush & select into DC
         pOldBrush = (CBrush*) pDC->SelectObject( &currBrush );
      else
         pOldBrush = (CBrush*) pDC->SelectStockObject( BLACK_BRUSH );   // stock black brush if can't make custom one 
   
      if( m_pDisplaySet->trace[i].iGain != 0 )                          // for continuous trace, use left-margin arrow
      {
         for(j=0;j<3;j++)                                               // shift arrow to point at trace baseline
            ptLfArrow[j].Offset(0, m_pDisplaySet->trace[i].iOffset);
         pDC->Polygon( ptLfArrow, 3 );                                  // draw it there
         for(j=0;j<3;j++)                                               // then shift it back so we can use it again
            ptLfArrow[j].Offset(0, -m_pDisplaySet->trace[i].iOffset);
      }
      else                                                              // for pulse train, use right-margin arrow
      {
         for(j=0;j<3;j++) 
            ptRtArrow[j].Offset(0, m_pDisplaySet->trace[i].iOffset); 
         pDC->Polygon( ptRtArrow, 3 ); 
         for(j=0;j<3;j++) 
            ptRtArrow[j].Offset(0, -m_pDisplaySet->trace[i].iOffset); 
      }
      
      pDC->SelectObject( pOldPen );                                     // restore old pen, brush; destroy custom  
      pDC->SelectObject( pOldBrush );                                   // GDI objects
      currPen.DeleteObject();
      currBrush.DeleteObject();
   }

}


//=== DrawTraces ====================================================================================================== 
// 
//    Draw all or a portion of the traces in the "displayed" trace set.
//
//    For analog data traces:  The internal trace buffer contains, for each trace, a subsampled version of the original 
//    raw data stream, where each bin in the internal buffer is the average of raw data samples falling in that bin -- 
//    see UpdateGraph().  Here, we simply scale and offset each sample in the trace buffer IAW the trace's attributes, 
//    then "connect the dots".
//
//    For pulse-train traces:  The stored value, f(N), in each "bin" N of the internal trace buffer is the # of pulses 
//    that occurred in the interval [N*D .. (N+1)*D), where D = graph width / #bins.  For each nonzero bin, a vertical 
//    line of height PULSEHT*f(N) pixels is drawn upwards from the trace baseline.
//
//    Properly handles the "wrap-around" situation, when the "current" time is less than the time of the first trace 
//    point to be drawn.
//
//    NOTE:  Remember that when rectangles are drawn in the device context, the rectangle in logical coords (l,t,r,b) 
//    are first translated to device coords (L,T,R,B).  Then, the rectangular area that is actually drawn to is 
//    (L,T,R-1,B-1).  Hence the tricky code used to make sure we erase all the way to the right edge of the graph 
//    trace area (which does not include margins).  Without this, we failed to erase trace pixels on the right edge. 
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//                iStart-- [in] index of starting bin (in the internal subsampled trace store) for drawing
//                n     -- [in] the # of points to draw 
//                bErase-- [in] if TRUE, erase that portion of the graph that is being redrawn; otherwise, it is 
//                         assumed that the necessary erasing has already been done (default is FALSE). 
//
//    RETURNS:    NONE.
//
VOID CGraphBar::DrawTraces( CDC* pDC, int iStart, int n, const BOOL bErase /* =FALSE */ )
{
   ASSERT( (iStart >= 0) && (iStart < m_pDisplaySet->nBins) &&       // check args...
           (n >= 0) );
   ASSERT( (m_pDisplaySet->nBins == m_pDisplaySet->nMaxBins) || 
           (iStart + n <= m_pDisplaySet->nBins) );

   if( (m_pDisplaySet->nBins == 0) || (n <= 0) || (iStart < 0) ||    // nothing to draw, or bad args...
       (iStart >= m_pDisplaySet->nBins) || 
       ((m_pDisplaySet->nBins < m_pDisplaySet->nMaxBins) && (iStart + n > m_pDisplaySet->nBins)) )
      return;

   if( n > m_pDisplaySet->nMaxBins )                                 // treated as a full redraw!!
   {
      iStart = 0;
      n = m_pDisplaySet->nBins;
   }

   BOOL bFull = BOOL(n == m_pDisplaySet->nMaxBins);                  // is this a complete draw (max #pts drawn)?
   int iEnd = (iStart + n) % m_pDisplaySet->nMaxBins;                // are we wrapping around end of graph?
   BOOL bWrap = BOOL(iEnd < iStart);

   if( bErase )                                                      // erase before drawing, if requested...
   {
      CRect rClient;                                                 // construct rect which will completely erase 
      GetClientRect( &rClient );                                     // the graph trace area but NOT the margins...
      int w = (m_bNormMargin) ? XMARGINSZ : 1;
      CRect rErase( w*2, 0, rClient.right - w*2, rClient.bottom );   // this will completely erase all traces
      pDC->DPtoLP( &rErase );

      if( !bFull )                                                   // if not a complete draw, we erase less...
      {
         rErase.left = (iStart>0) ?                                  // when there's room, erase a little bit extra  
            (iStart-1) * m_pDisplaySet->iWidth / m_pDisplaySet->nMaxBins : 0; 
                                                                     // to make sure we get rid of all the old stuff;
         if( !bWrap ) rErase.right = m_pDisplaySet->tCurrent;        // if wrap-around, we erase to the right edge, 
                                                                     // which was calculated above.
      }
      pDC->FillSolidRect( rErase, RGB(0,0,0) );

      if( bWrap )                                                    // wrap-around case - we must erase a second rect 
      {                                                              // on the far left side of graph...
         rErase.left = 0;
         rErase.right = m_pDisplaySet->tCurrent;
         pDC->FillSolidRect( rErase, RGB(0,0,0) );
      }
   }

   if( (iStart > 0) && (n < m_pDisplaySet->nMaxBins) )               // adjust start pt & #pts to ensure we attach new 
   {                                                                 // trace segs to immediately preceding segments!
      for( int k = 0; k < 3; k++ )                                   // we back up three positions at most...
      {
         --iStart;
         ++n;
         if( iStart == 0 || n == m_pDisplaySet->nMaxBins ) break;
      }
   }

   CPoint pt[2];                                                     // get PULSEHT in logical units.
   pt[0] = CPoint(0,0); pt[1] = CPoint(1,PULSEHT); 
   pDC->DPtoLP( &(pt[0]), 2 );
   int pulseHt = pt[0].y - pt[1].y;
   if( pulseHt < 0 ) pulseHt = -pulseHt;

   CPen* pOldPen;                                                    // for restoring original pen into DC 
   CPen currPen;                                                     // we use custom pen to draw trace

   int j = m_pDisplaySet->nMaxBins-iStart;                           // this is needed if we must wrap-around 

   for( int i = 0; i < m_pDisplaySet->nTraces; i++ )                 // DRAW THE TRACES:
   {
      if( currPen.CreatePen( PS_SOLID, 0,                            // create custom pen & select into DC
               m_pDisplaySet->trace[i].color ) )
         pOldPen = (CPen*) pDC->SelectObject( &currPen );
      else
         pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );      // use stock white pen if can't make custom one! 

      int k = i * m_pDisplaySet->nMaxBins;
      int offset = m_pDisplaySet->trace[i].iOffset;
      int gain = m_pDisplaySet->trace[i].iGain;

      for( int m = 0; m < n; m++ ) 
      {
         int iBin = (iStart + m) % m_pDisplaySet->nMaxBins;
         int x = iBin * m_pDisplaySet->iWidth / m_pDisplaySet->nMaxBins;
         int y = m_pDisplaySet->pTraceData[k+iBin];
         if( gain > 0 ) y = offset + y * gain;
         else if( gain < 0 ) y = offset - y / gain;
         else y *= pulseHt;

         if( gain != 0 )
         {
            if( m == 0 || iBin == 0 ) pDC->MoveTo(x,y);
            else pDC->LineTo(x,y);
         }
         else if( y > 0 )
         {
            pDC->MoveTo(x, offset);
            pDC->LineTo(x, offset + y);
         }
      }

      pDC->SelectObject( pOldPen );                                  // restore old pen; destroy custom one 
      currPen.DeleteObject();
   }

}


//=== DrawCurrentTimeline ============================================================================================= 
//
//    Draws a green vertical line a few pixels wide at the graph's "current" time.  Serves as a separator between the 
//    most recently graphed data and the data that will be erased on the next update!
//
//    If the line would extend into the right margin (when the current time is at or near the graph's horizontal 
//    extent), it is not drawn -- to prevent erasing all or part of the right-axis tick marks. 
//
//    Since the current timeline is unneeded in the "delayed display" mode, this method will do nothing in that mode.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//
//    RETURNS:    NONE.
//
VOID CGraphBar::DrawCurrentTimeline( CDC* pDC ) 
{
   if( m_bDelayMode ) return;

   CSize sz(MARKERW, MARKERW);
   pDC->DPtoLP( &sz );
   int w = sz.cx;

   if( m_pDisplaySet->tCurrent + w < m_pDisplaySet->iWidth ) 
   {
      CRect rCursor( m_pDisplaySet->tCurrent, m_pDisplaySet->iYMax, m_pDisplaySet->tCurrent + w, 
                     m_pDisplaySet->iYMin );
      pDC->FillSolidRect( rCursor, RGB(0,200,0) );
   }
}


//=== DrawLabel ======================================================================================================= 
//
//    Draw the trace set label near the top-left corner of the graph window's client area ("delayed" mode only). The 
//    label is drawn in MM_TEXT mode, not MM_ANISOTROPIC. This method should be invoked last in the paint sequence.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//
//    RETURNS:    NONE.
//
VOID CGraphBar::DrawLabel( CDC* pDC ) 
{
   if((!m_bDelayMode) || m_pDisplaySet->strLabel.IsEmpty()) return;

   // draw label in device coords, white on black bkg
   pDC->SetMapMode(MM_TEXT);
   pDC->SetWindowOrg(0, 0);
   pDC->SetViewportOrg(0, 0);
   pDC->SetTextColor( RGB(255,255,255) );                              // color of any text drawn
   pDC->SetBkColor( RGB(0,0,0) );                                      // display's bkg color is black

   CRect rClient;                                                    // get client area dimensions in pixels
   GetClientRect( &rClient );
   int wPix = rClient.Width();
   int hPix = rClient.Height();

   CRect rTextBounds(XMARGINSZ+3,0,wPix-XMARGINSZ-3,20);
   pDC->DrawText(m_pDisplaySet->strLabel, &rTextBounds, DT_END_ELLIPSIS | DT_VCENTER | DT_LEFT );
}

