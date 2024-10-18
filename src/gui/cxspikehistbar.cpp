//===================================================================================================================== 
//
// cxspikehistbar.cpp : Implementation of class CCxSpikeHistBar, a special-purpose control bar that displays spike time 
//                      histograms for any tagged sections defined on Maestro trials during a trial sequence.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
//
// ***IMPORTANT*** IMPLICITLY ASSUMES THAT A TRIAL TICK IS ONE MILLISECOND !!!
//
// CREDITS:
// 1) CSizingControlBar: resizable control bars, by Cristi Posea [03/31/2002, www.datamekanix.com/sizecbar] --  An 
// implementation of DevStudio-like docking windows that can contain dialogs, controls, even views (although not 
// recommended.  CCxSpikeHistBar is derived from this extension of MFC's CControlBar.
//
//
// REVISION HISTORY:
// 15apr2005-- Initial development begun.
// 15jun2005-- Changed minimum height of histogram in Hz from 50 to 10.
// 23jul2008-- Fixed PrepareForNextTrial() to correctly process trial codes that have been introduced since Jun2005.
// 02dec2014-- Revised Initialize() to traverse all trials in the specified set, including any subsets of that set. 
// Trial subsets were introduced in v3.1.2.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff
#include "math.h"

#include "cntrlx.h"                          // CCntrlxApp and resource defines
#include "cxdoc.h"                           // CCxDoc
#include "cxtrial.h"                         // CCxTrial

#include "cxspikehistbar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
//===================================================================================================================== 
//
// Implementation of CCxSpikeHistBar
//
//===================================================================================================================== 
//===================================================================================================================== 
//

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
const int CCxSpikeHistBar::MINBUFSZ             = 1000;     // minimum size of integer buffers used internally
const int CCxSpikeHistBar::BINSIZE_MS           = 10;       // size of bins in the internal bin buffer, in ms
const int CCxSpikeHistBar::PROLOGLEN_MS         = 50;       // length of prolog preceding each tagged section, in ms
const int CCxSpikeHistBar::EPILOGLEN_MS         = 150;      // length of epilog following each tagged section, in ms
const int CCxSpikeHistBar::MINWIDTH             = 100;      // minimum width of this docking bar (pixels)
const int CCxSpikeHistBar::MINHEIGHT            = 200;      // minimum height of this docking bar (pixels)
const int CCxSpikeHistBar::HISTOGRAM_HT         = 100;      // fixed height of an individual histogram (pixels)
const int CCxSpikeHistBar::MARKER_HT            = 5;        // fixed height of "end of prolog" baseline marker (pix)
const int CCxSpikeHistBar::VERTGAP              = 4;        // vertical space separating histograms on canvas (pixels)
const int CCxSpikeHistBar::HORIZGAP             = 5;        // horizontal space on either side of a histogram (pixels)
const int CCxSpikeHistBar::TOPMARGIN_HT         = 20;       // height of top margin (pixels)
const int CCxSpikeHistBar::ARROWSIZE            = 10;       // size of scroll arrows in top margin (pixels)
const int CCxSpikeHistBar::ARROWGAP             = 4;        // space between scroll arrows in top margin (pixels)
const int CCxSpikeHistBar::MINHISTHT_HZ         = 10;       // minimum height of a histogram in Hz (spikes/sec)
const COLORREF CCxSpikeHistBar::HISTCOLOR       = RGB(0,255,0);   // the color used to paint histogram bars

LPCTSTR CCxSpikeHistBar::ERRMSG_MEMEXCP = _T("ERROR: Memory exception! Spike histogram facility disabled!");
LPCTSTR CCxSpikeHistBar::WINTITLE = _T("Spike Histograms");


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

//=== CCxSpikeHistBar/~CCxSpikeHistBar [constructor/destructor] ======================================================= 
//
//    Constructed in an inactive state (canvas is empty, no histogram data).  Internal buffers are allocated when 
//    needed.  The destructor ensures that any allocated buffers are freed.
//
CCxSpikeHistBar::CCxSpikeHistBar()
{
   m_szMinHorz.cx = MINWIDTH;                            // set minimum size of histogram bar so we don't have to deal 
   m_szMinHorz.cy = MINHEIGHT;                           // with unsuitably small canvas
   m_szMinVert.cx = MINWIDTH;
   m_szMinVert.cy = MINHEIGHT;
   m_szMinFloat.cx = MINWIDTH;
   m_szMinFloat.cy = MINHEIGHT;

   m_nBinBufferSize = 0;                                 // all internal buffers are unallocated initially.
   m_pBinBuffer = NULL;

   m_nSpikeBufSize = 0;
   m_nSpikes = 0;
   m_pSpikeTimesBuffer = NULL;

   m_nMaxBins = 0;                                       // these members are used only when painting histograms
   m_nBinsPerDisplayBin = 0;
   m_nPixPerDisplayBin = 0;
   m_dVertScale = 0;
   m_nVisible = 0;
   m_nScrollPos = 0;
}

CCxSpikeHistBar::~CCxSpikeHistBar()
{
   ::free(m_pBinBuffer);
   m_pBinBuffer = NULL;
   m_nBinBufferSize = 0;

   ::free(m_pSpikeTimesBuffer);
   m_pSpikeTimesBuffer = NULL;
   m_nSpikeBufSize = 0;

   while( !m_Sections.IsEmpty() )
   {
      CSection* pSect = m_Sections.RemoveHead();
      delete pSect;
   }

}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 

BEGIN_MESSAGE_MAP( CCxSpikeHistBar, CSizingControlBarCF )
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


//=== OnCreate [base override] ======================================================================================== 
//
//    Response to WM_CREATE message.  Here's where we initially allocate the internal buffers used by the histogram 
//    facility.  The buffers may be reallocated as the need arises during usage.
//
//    ARGS:       lpcs  -- [in] creation parameters
//
//    RETURNS:    -1 to abort creation; 0 to continue. 
//
int CCxSpikeHistBar::OnCreate( LPCREATESTRUCT lpcs )
{
   if( CSizingControlBarCF::OnCreate( lpcs ) == - 1 )                               // let base class go first 
      return( -1 ); 

   m_pBinBuffer = (int*) ::malloc( MINBUFSZ * sizeof(int) );
   if( m_pBinBuffer == NULL ) return( -1 );
   m_nBinBufferSize = MINBUFSZ;

   m_pSpikeTimesBuffer = (int*) ::malloc( MINBUFSZ * sizeof(int) );
   if( m_pSpikeTimesBuffer == NULL ) return( -1 );
   m_nSpikeBufSize = MINBUFSZ;

   return( 0 );
}


//=== OnPaint [base override] ========================================================================================= 
//
//    Response to WM_PAINT message.  Here we assume the entire client area has already been erased (OnEraseBkgnd()), 
//    and we redraw all histograms completely.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CCxSpikeHistBar::OnPaint()
{
   CPaintDC dc( this );

   if( m_Sections.IsEmpty() ) return;                                // histogram facility is disabled; canvas is blank

   dc.SetMapMode( MM_TEXT );                                         // all drawing is done in device pixels, with 
   dc.SetViewportOrg(0, 0);                                          // origin at top-left corner of client area

   dc.SetTextColor( RGB(255,255,255) );                              // color of any text drawn
   dc.SetBkColor( RGB(0,0,0) );                                      // display's bkg color is black

   CRect rClient;                                                    // get client area dimensions in pixels
   GetClientRect( &rClient );
   int wPix = rClient.Width();
   int hPix = rClient.Height();

   CalcLayoutParameters(wPix, hPix);                                 // calc parameters that define histogram layout

   int nTotal = static_cast<int>(m_Sections.GetCount());             // update scroll position as needed
   if( m_nVisible == nTotal )
      m_nScrollPos = 0;
   else if( m_nScrollPos + m_nVisible > nTotal )
   {
      m_nScrollPos = nTotal - m_nVisible;
      if( m_nScrollPos < 0 ) m_nScrollPos = 0;
   }

   DrawTopMargin(&dc, wPix, hPix);                                   // draw margin area at top of client area

   int yOff = TOPMARGIN_HT;                                          // draw all histograms that are completely 
   POSITION pos = m_Sections.GetHeadPosition();                      // visible in client area (minus top margin)
   int index = 0;
   while( pos != NULL )
   {
      if( yOff + HISTOGRAM_HT > hPix ) break;                        // next histogram is cut off, so we stop drawing
      CSection* pSect = m_Sections.GetNext(pos);                     // get next histogram

      ++index;
      if( (index-1) < m_nScrollPos ) continue;                       // skip histograms before current scroll position 
      
      DrawSectionHistogram(&dc, yOff, pSect);

      yOff += HISTOGRAM_HT + VERTGAP;
   }
}


//=== OnSize [base override] ========================================================================================== 
//
//    Response to the WM_SIZE message.  Whenever the control bar is resized, we must repaint the entire client area.  
//
//    ARGS:       pDC   -- [in] ptr to device context 
//
//    RETURNS:    TRUE to indicate background was erased; FALSE otherwise. 
//
void CCxSpikeHistBar::OnSize( UINT nType, int cx, int cy )
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
BOOL CCxSpikeHistBar::OnEraseBkgnd( CDC *pDC )
{
   CRect rect;
   GetClientRect( rect );
   pDC->FillSolidRect( rect, RGB(0, 0, 0) );
   return( TRUE );
}


//=== OnLButtonDown [base override] =================================================================================== 
//
//    Response to the WM_LBUTTONDOWN message.  If the user depresses the left mouse button while the cursor is over an 
//    active scroll area, then the current scroll position is incremented or decremented and the entire client area is 
//    repainted.  
//
//    ARGS:       nFlags   -- [in] indicates whether various virtual keys are down.
//                pt       -- [in] client-area coordinates of the cursor when the mousedown occurred.
//
//    RETURNS:    NONE.
//
void CCxSpikeHistBar::OnLButtonDown(UINT nFlags, CPoint pt) 
{
   BOOL didScroll = FALSE;
   int nTotal = static_cast<int>(m_Sections.GetCount());
   if( nTotal > 0 && m_nVisible < nTotal )
   {
      CRect rect;
      GetClientRect( rect );
      int clientW = rect.Width();

      int top = (TOPMARGIN_HT - ARROWSIZE) / 2;
      int bot = TOPMARGIN_HT - top;
      if( (pt.y>=top) && (pt.y<=bot) &&                              // if user pressed scroll down arrow and 
          (pt.x >= clientW - 2*(ARROWSIZE+ARROWGAP)) &&              // we're not at the bottom of the scroll range,
          (pt.x <= clientW - ARROWSIZE - 2*ARROWGAP) &&              // then scroll down one histogram.
          (m_nScrollPos + m_nVisible < nTotal) )
      {
         ++m_nScrollPos;
         didScroll = TRUE;
      }
      else if( (pt.y>=top) && (pt.y<=bot) &&                         // if user pressed scroll up arrow and 
          (pt.x >= clientW - ARROWSIZE - ARROWGAP) &&                // we're not at the top of the scroll range,
          (pt.x <= clientW - ARROWGAP) &&                            // then scroll up one histogram.
          (m_nScrollPos > 0) )
      {
         --m_nScrollPos;
         didScroll = TRUE;
      }
   }

   if( didScroll )                                                   // if we changed scroll pos, then we must repaint 
      Invalidate( TRUE );                                            // the client area
   else
      CSizingControlBarCF::OnLButtonDown(nFlags, pt);
}



//===================================================================================================================== 
// OPERATIONS, ATTRIBUTES, IMPLEMENTATION
//===================================================================================================================== 

//=== Initialize ====================================================================================================== 
//
//    Initialize histogram facility and prepare it to build and display spike histograms for any tagged sections 
//    defined on trials in the specified trial set.
//
//    Each trial in the set is examined for any tagged sections. For each unique (by tag name) section found, the 
//    facility stores the tag name and the worst-case section duration in #histogram bins. The section duration always 
//    includes a prolog and epilog of fixed duration (PROLOGLEN_MS, EPILOGLEN_MS). If no tagged sections are found, 
//    the histogram facility is inactive and the client area will be empty. Otherwise, the internal bin buffer is 
//    re-allocated as needed to ensure it is large enough to store histogram data for all defined sections.
//
//    As of v3.1.2, a trial set can contain "subsets" of trials. This method was revised to ensure it traverses ALL
//    trials in the specified trial set, including trials that are ensconced within subsets of that set.
//
//    ARGS:       wSet  -- [in] unique key of a Maestro trial set.
//
//    RETURNS:    TRUE if successful; FALSE otherwise (specified object is not a trial set; memory allocation failed).
//
BOOL CCxSpikeHistBar::Initialize( WORD wSet )
{
   Reset();
   Invalidate( TRUE );

   // get document and verify that specified trial set exists
   CCxDoc* pDoc = ((CCntrlxApp*) AfxGetApp())->GetDoc(); 
   if( pDoc == NULL ) return( FALSE );
   if( (!pDoc->ObjExists(wSet)) || pDoc->GetObjType(wSet)!=CX_TRIALSET )
      return( FALSE ); 

   // traverse all trials in the set...
   CWordArray wArTrials;
   pDoc->GetTrialKeysIn(wSet, wArTrials);
   for(int i=0; i<wArTrials.GetSize(); i++)
   {
      CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(wArTrials[i]);

      // for each tagged section found in a trial...
      for( int i=0; i<pTrial->GetNumTaggedSections(); i++ )
      {
         TRIALSECT section;
         pTrial->GetTaggedSection(i, section);

         // compute worst-case duration in #bins, accounting for prolog & epilog
         int iMaxDur = 0;  
         for(int j=int(section.cFirstSeg); j<=int(section.cLastSeg); j++) 
         {
            iMaxDur += pTrial->GetWorstCaseDuration(j);
         }
         iMaxDur += PROLOGLEN_MS + EPILOGLEN_MS; 
         BOOL bAddOne = BOOL((iMaxDur % BINSIZE_MS)!=0);
         iMaxDur /= BINSIZE_MS;
         if( bAddOne ) ++iMaxDur;

         if( iMaxDur > m_nMaxBins )                                        //    for display purposes, we need to rem
            m_nMaxBins = iMaxDur;                                          //    length of longest tagged section!

         BOOL bFound = FALSE;                                              //    if TRUE, section already exists
         POSITION insertPos = NULL;                                        //    non-NULL: insert new section here; 
         POSITION posSect = m_Sections.GetHeadPosition();                  //    else, append new section to list
         while( posSect != NULL )
         {
            POSITION posTemp = posSect;
            CSection* pNext = m_Sections.GetNext(posSect);
            int res = pNext->tag.Compare(section.tag);
            if( res == 0 )                                                 //    section already exists -- update max 
            {                                                              //    duration if necessary
               if( pNext->nBins < iMaxDur ) pNext->nBins = iMaxDur;
               bFound = TRUE;
               break;
            }
            else if( res > 0 )                                             //    sections are ordered alphabetically
            {
               insertPos = posTemp;
               break;
            }   
         }

         if( !bFound )                                                     //    insert new section into list, storing 
         {                                                                 //    section tag and max duration.  Abort 
            CSection* pSect = new CSection;                                //    on any memory exception.
            if( pSect == NULL ) 
            {
               Reset();
               ((CCntrlxApp*)AfxGetApp())->LogMessage( ERRMSG_MEMEXCP );
               return( FALSE );
            }
            pSect->tag = section.tag;
            pSect->nBins = iMaxDur;
            pSect->nFirstBin = -1;
            pSect->nReps = 0;
            pSect->tStart = -1;
            pSect->tEnd = -1;

            try 
            {
               if( insertPos != NULL )
                  m_Sections.InsertBefore( insertPos, pSect );
               else
                  m_Sections.AddTail( pSect );
            }
            catch( CMemoryException *e )
            {
               e->Delete();
               delete pSect;
               Reset();
               ((CCntrlxApp*)AfxGetApp())->LogMessage( ERRMSG_MEMEXCP );
               return( FALSE );
            }
         }
      }
   }

   if( m_Sections.IsEmpty() ) return( TRUE );                              // if we found no tagged sects, we're done! 

   int nSumBins = 0;                                                       // determine total #bins needed for all 
   POSITION pos = m_Sections.GetHeadPosition();                            // tagged section histograms
   while( pos != NULL )
   {
      CSection* pSect = m_Sections.GetNext(pos);
      nSumBins += pSect->nBins;
   }

   if( m_nBinBufferSize < nSumBins )                                       // realloc histogram bin buffer if needed
   {
      ::free( m_pBinBuffer );
      m_pBinBuffer = NULL;
      m_nBinBufferSize = 0;

      m_pBinBuffer = (int*) ::malloc( (nSumBins+10)*sizeof(int) );         // if alloc fails, facility must be reset
      if( m_pBinBuffer == NULL )
      {
         Reset();
         ((CCntrlxApp*)AfxGetApp())->LogMessage( ERRMSG_MEMEXCP );
         return( FALSE );
      }
      m_nBinBufferSize = nSumBins + 10;

   }

   ::memset( (void*)m_pBinBuffer, 0, m_nBinBufferSize*sizeof(int) );       // zero all histogram bins

   int iBin = 0;                                                           // now assign portions of histogram bin 
   pos = m_Sections.GetHeadPosition();                                     // to the individual tagged sections
   while( pos != NULL )
   {
      CSection* pSect = m_Sections.GetNext(pos);
      pSect->nFirstBin = iBin;
      iBin += pSect->nBins;
   }

   // update title bar to include name of trial set from which tagged sections are culled
   CString title;
   title.Format( "%s [%s]", WINTITLE, pDoc->GetObjName(wSet) );
   SetWindowText( title );
   return( TRUE );
}


//=== Reset =========================================================================================================== 
//
//    Reset histogram facility to inactive state.  The client area is not repainted, but the next time it is, it will 
//    be empty.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSpikeHistBar::Reset()
{
   // release any tagged section records.  If there are no tagged sections to process, the histogram facility is 
   // inactive and the canvas will be blank.
   while( !m_Sections.IsEmpty() )
   {
      CSection* pSect = m_Sections.RemoveHead();
      delete pSect;
   }

   // empty the buffer storing any spike times for a trial in progress
   m_nSpikes = 0;

   // reset members involved in painting the histogram canvas
   m_nMaxBins = 0;
   m_nBinsPerDisplayBin = 0;
   m_nPixPerDisplayBin = 0;
   m_dVertScale = 0;
   m_nVisible = 0;
   m_nScrollPos = 0;

   // update title bar to reflect fact that histogram facility is not in use
   CString title = CString(WINTITLE) + _T(" (not in use)");
   SetWindowText( title );
}


//=== PrepareForNextTrial ============================================================================================= 
//
//    Prepare to consume spike events streamed during the next trial in a trial set sequence.
//
//    This method examines the trial codes defining the next trial to be presented to determine the start and end times 
//    of any tagged sections in the trial (we CANNOT rely on the CCxTrial definition because any given segment will 
//    have a randomized duration if its min & max durations are not the same!).  Trial will be ignored by histogram 
//    facility if it contains a special "skip on saccade" operation -- since the section start/end times are 
//    indeterminate in that case.  The trial is also ignored if it contains no tagged sections, or if one of the 
//    trial codes is unrecognized.
//
//    ARGS:       nCodes      -- [in] #trial codes defining the next Maestro trial.
//                pCodes      -- [in] trial code buffer.
//                nSects      -- [in] #tagged sections found in the next Maestro trial.
//                pSections   -- [in] the tagged section records.
//
//    RETURNS:    NONE. 
//
VOID CCxSpikeHistBar::PrepareForNextTrial( int nCodes, PTRIALCODE pCodes, int nSects, PTRIALSECT pSections )
{
   if( m_Sections.IsEmpty() )                                                 // there are no tagged sections in the 
      return;                                                                 // current trial sequence

   POSITION pos = m_Sections.GetHeadPosition();                               // reinit start/end times of all tagged 
   while( pos != NULL )                                                       // sections monitored by this facility
   { 
      CSection* pSect = m_Sections.GetNext(pos);
      pSect->tStart = -1;
      pSect->tEnd = -1;
   }

   int nSegs = 0;                                                             // process trial codes to get start times 
   int segStart[MAX_SEGMENTS];                                                // for all segments in the trial
   BOOL bSkip = FALSE;                                                        // if TRUE, skip trial b/c it includes a 
                                                                              // "skip on saccade" segment
   BOOL bDone = FALSE; 
   int iTick = 0; 
   int i = 0;
   while( !bDone )
   {
      if( i<nCodes && (((int)pCodes[i].time) == iTick)  &&                    // detect seg boundary
          (pCodes[i].code != ENDTRIAL) && (pCodes[i].code != FIXACCURACY) )   // these codes never start a segment 
      {
         segStart[nSegs] = iTick;                                             // remember seg start time 
         ++nSegs;
      }

      while( i<nCodes && ((int)pCodes[i].time) <= iTick )                     // process all trial codes for current 
         switch( pCodes[i].code )                                             // trial "tick" 
      {
         case SPECIALOP :                                                     // if trial has a "skipOnSaccade" op, 
            if( pCodes[i+1].code == SPECIAL_SKIP )                            // section start times and durations are  
            {                                                                 // indeterminate -- so trial results are 
               bSkip = TRUE;                                                  // ignored by this histogram facility 
            }
            i += 2;
            break;

         case ADCON :                                                         // #trial codes in group N = 1
         case ADCOFF : 
         case CHECKRESPOFF :
         case FAILSAFE :
         case STARTTRIAL :
            ++i; 
            break;

         case TARGET_ON :                                                     // N = 2
         case TARGET_OFF : 
         case TARGET_HPOSREL :  
         case TARGET_HPOSABS : 
         case TARGET_VPOSREL : 
         case TARGET_VPOSABS : 
         case TARGET_HVEL : 
         case TARGET_HSLOVEL :
         case TARGET_VVEL : 
         case TARGET_VSLOVEL :
         case INSIDE_HVEL : 
         case INSIDE_HSLOVEL :
         case INSIDE_VVEL : 
         case INSIDE_VSLOVEL :
         case TARGET_HACC : 
         case TARGET_HSLOACC :
         case TARGET_VACC : 
         case TARGET_VSLOACC :
         case TARGET_HOPEN : 
         case FIXEYE1 : 
         case FIXEYE2 : 
         case FIXACCURACY : 
         case REWARDLEN :
         case MIDTRIALREW :
         case CHECKRESPON :
         case RANDOM_SEED :
         case PULSE_ON :
         case INSIDE_HACC :
         case INSIDE_VACC :
         case INSIDE_HSLOACC :
         case INSIDE_VSLOACC :
         case TARGET_VSTAB :
            i += 2;
            break;

         case RPDWINDOW :                                                     // N = 3
            i += 3;
            break;
            
         case TARGET_PERTURB :                                                // N = 5  
            i += 5;
            break;

         case PSGM_TC :                                                       // N = 6 
            i += 6;
            break;

         case ENDTRIAL :                                                      // we're done!
            i = nCodes + 1;
            bDone = TRUE;
            break;

         default :                                                            // if code not recognized, we must skip
            bSkip = TRUE;                                                     // trial; log error msg in this case
            ((CCntrlxApp*)AfxGetApp())->LogMessage(_T("ERROR: Bad trial code; trial ignored by histogram facility!"));
            bDone = TRUE;
            i = nCodes + 1;
            break;
      }

      ++iTick;                                                                // advance to next trial tick (1ms)
   }

   if( bSkip )                                                                // this trial will be ignored by spike
      return;                                                                 // histogram facility

   for( i=0; i<nSects; i++ )                                                  // remember start/end times of each 
   {                                                                          // tagged section found in the trial
      POSITION pos = m_Sections.GetHeadPosition();
      while( pos != NULL )
      {
         CSection* pSect = m_Sections.GetNext(pos);
         if( pSect->tag.Compare( pSections[i].tag ) == 0 )
         {
            int s0 = int(pSections[i].cFirstSeg);
            int s1 = int(pSections[i].cLastSeg);

            if(s0 >= 0 && s0 < nSegs && s1 >= 0 && s1 < nSegs  && s0 <= s1)   // note that we pad tagged section w/ a 
            {                                                                 // fixed prolog & epilog.
               pSect->tStart = segStart[s0] - PROLOGLEN_MS; 
               if( pSect->tStart < 0 )                                        // cut prolog short is necessary
                  pSect->tStart = 0; 

               if( s1 + 1 < nSegs )
                  pSect->tEnd = segStart[s1+1] + EPILOGLEN_MS;
               else                                                           // if section is at trial's end, there 
                  pSect->tEnd = iTick;                                        // will be no epilog!
            }

            pos = NULL;                                                       // section can appear only once in trial 
         }
      }
   }

   m_nSpikes = 0;                                                             // make sure spike times buf is empty
}


//=== ConsumeSpikes =================================================================================================== 
//
//    Consume any spike events from the event stream for a trial currently in progress.  The histogram display itself 
//    is not updated until the spike event buffer is committed by invoking Commit().
//
//    The method has no effect if the histogram facility is currently disabled.
//
//    ARGS:       n           -- [in] number of events to process from the event stream.
//                pEvtMask    -- [in] event mask buffer.  Each element is a bit mask indicating which digital inputs 
//                               were raised.  It is possible that more than one input may be raised during any msec 
//                               in a trial.  This method only cares about events on input channel 0.
//                pEvtTimes   -- [in] event times buffer.  Each element holds the trial time at which the digital 
//                               event was detected.  Units = milliseconds since the trial started.
//
//    RETURNS:    TRUE if successful; FALSE if unable to reallocate the internal spike times buffer when needed.  In 
//                the latter case, the histogram facility is reset.
//
BOOL CCxSpikeHistBar::ConsumeSpikes( int n, DWORD* pEvtMask, int* pEvtTimes )
{
   if( m_Sections.IsEmpty() )                                           // if there are no tagged sections, then 
      return( TRUE );                                                   // there's no need to examine event stream!

   for( int i=0; i<n; i++ )
   {
      if( (pEvtMask[i] & 0x01) != 0 )                                   // spike event (DI0 raised)
      {
         if( m_nSpikes == m_nSpikeBufSize )                             // realloc spike times buffer when full
         {
            int* pNewBuf = (int*) ::realloc( m_pSpikeTimesBuffer, 
                           (m_nSpikeBufSize + MINBUFSZ)*sizeof(int) );
            if( pNewBuf == NULL )                                       // must abort if we cannot reallocate!
            {
               Reset();
               Invalidate(TRUE);
               ((CCntrlxApp*)AfxGetApp())->LogMessage(ERRMSG_MEMEXCP);
               return( FALSE );
            }
            m_pSpikeTimesBuffer = pNewBuf;
            m_nSpikeBufSize += MINBUFSZ;
         }

         m_pSpikeTimesBuffer[m_nSpikes] = pEvtTimes[i];                 // save trial time (ms) when spike occurred
         ++m_nSpikes;
      }
   }

   return( TRUE );
}


//=== Commit ========================================================================================================== 
//
//    This method should be invoked when the trial in progress completes successfully.  It updates all tagged section 
//    histograms IAW the spike times collected during the trial, and repaints the entire client area to reflect the 
//    updates made.  If PrepareForNextTrial() is called without invoking this method, the spike times from the previous 
//    trial are discarded.
//
//    The method has no effect if the histogram facility is currently disabled.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
VOID CCxSpikeHistBar::Commit()
{
   if( m_Sections.IsEmpty() )                                           // if there are no tagged sections, then 
      return;                                                           // there's nothing to do!

   for( int i=0; i<m_nSpikes; i++ )                                     // for each "spike" recorded
   {
      int tSpike = m_pSpikeTimesBuffer[i];                              // trial time at which spike occurred (ms)

      POSITION pos = m_Sections.GetHeadPosition();                      // find tagged section in which event occurred, 
      while( pos != NULL )                                              // if any...
      {
         CSection* pSect = m_Sections.GetNext(pos);
         if( pSect->tStart < 0 || pSect->tEnd < 0 ) continue;           //    skip section not found in trial

         int t = tSpike;                                                //    if spike occurred during tagged section, 
         if( t >= pSect->tStart && t < pSect->tEnd )                    //    incr corres. bin in section histogram
         {
            t -= pSect->tStart;
            t /= BINSIZE_MS;
            if( t < pSect->nBins )                                      //    protect against buffer corruption!
               ++(m_pBinBuffer[pSect->nFirstBin + t]);
         }
      }
   }

   m_nSpikes = 0;                                                       // empty the spike times buffer

   POSITION pos = m_Sections.GetHeadPosition();                         // incr #reps for all sections found in the 
   while( pos != NULL )                                                 // trial just finished.  Reset section start 
   {                                                                    // times and durations...
      CSection* pSect = m_Sections.GetNext(pos);
      if( pSect->tStart >= 0 && pSect->tEnd >= 0 )
      {
         ++(pSect->nReps);
         pSect->tStart = -1;
         pSect->tEnd = -1;
      }
   }

   Invalidate(TRUE);                                                    // repaint client area to update appearance 
                                                                        // of all histograms
}


//=== CalcLayoutParameters ============================================================================================ 
//
//    This method is called prior to repainting the client area of CCxSpikeHistBar.  It calculates factors which 
//    determine how the histograms are scaled horizontally and vertically.  Currently, CCxSpikeHistBar arranges all 
//    section histograms in a single vertical column, with a small vertical gap separating adjacent histograms.
//
//    Horizontal axis:  Based on the width of the longest section histogram displayed and the current width of the 
//    client area, the method determines how many bins in the internal histogram buffer must be compressed into a 
//    single DISPLAYED bin on the canvas.  It also chooses the width of the displayed bin, in pixels.  These two 
//    factors are selected to use as much of the client area width as possible.
//
//    Vertical axis:  All histograms are a fixed height in pixels.  The method scans all section histograms to find the 
//    maximum observed firing rate in Hz.  All histograms are uniformly scaled so that this max observed firing rate 
//    does not exceed the fixed height in pixels.
//
//    ARGS:       clientW  -- [in] current width of canvas (client area) in pixels.
//                clientH  -- [in] current height of canvas in pixels.
//
//    RETURNS:    NONE.
//
VOID CCxSpikeHistBar::CalcLayoutParameters(int clientW, int clientH)
{
   m_nBinsPerDisplayBin = 1;                                            // calculate params affecting horiz layout
   m_nPixPerDisplayBin = 2;
   int hPixTotal = 2*HORIZGAP + m_nPixPerDisplayBin*m_nMaxBins;
   while( hPixTotal > clientW )
   {
      ++m_nBinsPerDisplayBin;
      int i = m_nMaxBins / m_nBinsPerDisplayBin;
      if( (m_nMaxBins % m_nBinsPerDisplayBin) != 0 ) ++i;
      hPixTotal = 2*HORIZGAP + m_nPixPerDisplayBin * i;
   }

   int nDisplayBins = m_nMaxBins / m_nBinsPerDisplayBin;
   if( (m_nMaxBins % m_nBinsPerDisplayBin) != 0 ) 
      ++nDisplayBins;
   hPixTotal = 2*HORIZGAP + m_nPixPerDisplayBin * nDisplayBins;
   while( hPixTotal <= clientW )
   {
      ++m_nPixPerDisplayBin;
      hPixTotal = 2*HORIZGAP + m_nPixPerDisplayBin * nDisplayBins;
   }
   --m_nPixPerDisplayBin;

   double dBinSec = double(m_nBinsPerDisplayBin*BINSIZE_MS) * 0.001;    // displayed bin size in seconds

   double dMaxRate = 0;                                                 // examine all histograms to find max firing   
   POSITION pos = m_Sections.GetHeadPosition();                         // rate observed in single DISPLAYED BIN
   while( pos != NULL )
   {
      CSection* pSect = m_Sections.GetNext(pos);
      if( pSect->nReps <= 0 ) continue;                                 //    no data collected yet for this histogram 

      int i = 0; 
      while( i < pSect->nBins )
      {
         int nSpikes = m_pBinBuffer[pSect->nFirstBin + i];
         ++i;
		 int nPerDspBin = 1;
		 while(nPerDspBin<m_nBinsPerDisplayBin && i<pSect->nBins)
         {
            nSpikes += m_pBinBuffer[pSect->nFirstBin + i];
            ++i;
			++nPerDspBin;
         }

         double d = double(nSpikes)/double(pSect->nReps);               //    avg #spikes occurring in DISPLAYED bin
         if( m_nBinsPerDisplayBin > 1 && i==pSect->nBins )              //    if DISPLAYED bin larger than fundamental 
            d /= double(nPerDspBin*BINSIZE_MS) * 0.001;                 //    bin size, then last displayed bin might 
         else                                                           //    might be shorter than the others
            d /= dBinSec;

         if( d > dMaxRate ) dMaxRate = d;
      }
   }

   if( dMaxRate < double(MINHISTHT_HZ) )                                // if max firing rate too low, use minimum 
      dMaxRate = MINHISTHT_HZ;
   else
      dMaxRate = ::ceil(dMaxRate);

   m_dVertScale = double(HISTOGRAM_HT) / dMaxRate;                      // scale all histograms IAW observed max rate

   m_nVisible = 0;                                                      // determine how many histograms can be drawn 
   int hAvailable = clientH - TOPMARGIN_HT;                             // in the available client area
   int nTotal = static_cast<int>(m_Sections.GetCount());
   while( (m_nVisible < nTotal) && (hAvailable > 0) )
   {
      if( hAvailable > HISTOGRAM_HT ) ++m_nVisible;
      else break;
      hAvailable -= HISTOGRAM_HT + VERTGAP;
   }

}


//=== DrawTopMargin =================================================================================================== 
//
//    Draws the top margin of the client area.  This region displays a short text string of the form "Full ht=<n> Hz, 
//    bin=<m> ms".  If scrolling is necessary to see all histograms, a small white scroll-down arrow and/or scroll-up 
//    arrow appear at the right end of the margin rectangle.  A 1-pixel thick white line is drawn at the bottom of the 
//    margin, separating it from the histograms.
//
//    ARGS:       pDC      -- [in] ptr to the device context for the client rect. 
//                clientW  -- [in] current width of the client rect, in pixels.
//                clientH  -- [in] current height of the client rect, in pixels.
//
//    RETURNS:    NONE.
//
VOID CCxSpikeHistBar::DrawTopMargin(CDC* pDC, int clientW, int clientH)
{
   CString info;                                                           // prepare text string indicating bin size
   int maxRate = int( (double(HISTOGRAM_HT) / m_dVertScale) + 0.5 );       // and max firing rate in histograms
   int binSize = BINSIZE_MS * m_nBinsPerDisplayBin;
   info.Format( "Full ht=%d Hz, bin=%d ms", maxRate, binSize );

   CRect rTextBounds(0,0,clientW-1-2*(ARROWSIZE+ARROWGAP),TOPMARGIN_HT);   // draw text string, excluding it from the 
   pDC->DrawText( info, &rTextBounds,                                      // area where scroll arrows may be drawn
                     DT_END_ELLIPSIS | DT_VCENTER | DT_LEFT );

   int nTotal = static_cast<int>(m_Sections.GetCount());                   // if some histograms are not visible, we 
   if( m_nVisible < nTotal )                                               // need to draw one or both scroll arrows
   {                                                                       // near right end of the margin rect...
      BOOL bUp = BOOL(m_nScrollPos > 0); 
      BOOL bDown = BOOL(m_nScrollPos + m_nVisible < nTotal);

      CBrush* pOldBrush = (CBrush*) pDC->SelectStockObject( WHITE_BRUSH ); // scroll arrows are white

      int top = (TOPMARGIN_HT - ARROWSIZE) / 2;
      int bot = TOPMARGIN_HT - top;
      if( bDown )                                                          // the scroll down arrow
      {
         CPoint ptArrow[3];
         ptArrow[0] = CPoint(clientW - 2*(ARROWSIZE+ARROWGAP), top);
         ptArrow[1] = CPoint(clientW - ARROWSIZE - 2*ARROWGAP, top);
         ptArrow[2] = CPoint(clientW - 3*ARROWSIZE/2 - 2*ARROWGAP, bot);
         pDC->Polygon( ptArrow, 3 );
      }

      if( bUp )                                                            // the scroll up arrow
      {
         CPoint ptArrow[3];
         ptArrow[0] = CPoint(clientW - ARROWSIZE/2 - ARROWGAP, top);
         ptArrow[1] = CPoint(clientW - ARROWSIZE - ARROWGAP, bot);
         ptArrow[2] = CPoint(clientW - ARROWGAP, bot);
         pDC->Polygon( ptArrow, 3 );
      }

      pDC->SelectObject(pOldBrush);                                        // restore original brush
   }

   CPen* pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );            // draw 2-pixel thick line near bottom of 
   pDC->MoveTo(0, TOPMARGIN_HT-2);                                         // margin
   pDC->LineTo(clientW, TOPMARGIN_HT-2);
   pDC->MoveTo(0, TOPMARGIN_HT-1); 
   pDC->LineTo(clientW, TOPMARGIN_HT-1);
   pDC->SelectObject(pOldPen);
}


//=== DrawSectionHistogram ============================================================================================ 
//
//    Draw histgram for the specified tagged section.
//
//    ARGS:       pDC   -- [in] ptr to the device context for the client rect. 
//                yOff  -- [in] y-coordinate of the top-left corner of box bounding the histogram, in device pixels 
//                         relative to the top-left corner of CCxSpikeHistBar's client area.
//                pSect -- [in] the section histogram to be drawn
//
//    RETURNS:    NONE.
//
VOID CCxSpikeHistBar::DrawSectionHistogram(CDC* pDC, int yOff, CSection* pSect)
{
   int nDisplayBins = pSect->nBins;                                     // number of displayed bins for this histogram 
   if( m_nBinsPerDisplayBin > 1 )
   {
      nDisplayBins = pSect->nBins / m_nBinsPerDisplayBin;
      if( (pSect->nBins % m_nBinsPerDisplayBin) != 0 )
         ++nDisplayBins;
   }

   CPen* pOldPen = (CPen*) pDC->SelectStockObject( WHITE_PEN );         // draw baseline with stock white pen
   int nLen = nDisplayBins * m_nPixPerDisplayBin + 1;
   int yBase = yOff+HISTOGRAM_HT;
   pDC->MoveTo( HORIZGAP, yBase ); 
   pDC->LineTo( HORIZGAP+nLen, yBase );

   double d = double(m_nPixPerDisplayBin*PROLOGLEN_MS);                 // draw vertical mark at end of prolog, just 
   d /= double(m_nBinsPerDisplayBin*BINSIZE_MS);                        // below baseline, using stock white pen
   int nEndProlog = HORIZGAP + int(::floor(d));
   pDC->MoveTo( nEndProlog, yBase );
   pDC->LineTo( nEndProlog, yBase + MARKER_HT );
   pDC->MoveTo( nEndProlog+1, yBase );
   pDC->LineTo( nEndProlog+1, yBase + MARKER_HT );

   pDC->SelectObject( pOldPen ); 

   if( pSect->nReps > 0 )                                               // skip histogram if there's no data!
   {
      COLORREF oldBkColor = pDC->GetBkColor();                          // b/c FillSolidRect() changes bkg color

      double dBinSec = double(m_nBinsPerDisplayBin*BINSIZE_MS) * 0.001; // width of a displayed bin in seconds

      int x = HORIZGAP;
      int i = 0;
      while( i < pSect->nBins )
      {
         int nSpikes = m_pBinBuffer[pSect->nFirstBin + i];
         ++i;
		 int nPerDspBin = 1;
		 while(nPerDspBin<m_nBinsPerDisplayBin && i<pSect->nBins)
         {
            nSpikes += m_pBinBuffer[pSect->nFirstBin + i];
            ++i;
			++nPerDspBin;
         }

         double dRate = double(nSpikes)/double(pSect->nReps);           // avg firing rate in the DISPLAYED bin 
         if( m_nBinsPerDisplayBin > 1 && i==pSect->nBins )              // if DISPLAYED bin larger than fundamental 
            dRate /= double(nPerDspBin*BINSIZE_MS) * 0.001;             // bin size, then last displayed bin might 
         else                                                           // might be shorter than the others
            dRate /= dBinSec;
         
         int yHt = int(m_dVertScale * dRate + 0.5);
         if( yHt > HISTOGRAM_HT ) yHt = HISTOGRAM_HT;

         if( yHt > 0 )
            pDC->FillSolidRect( x, yBase - yHt, m_nPixPerDisplayBin, yHt, HISTCOLOR );
         x += m_nPixPerDisplayBin;
      }

      pDC->SetBkColor( oldBkColor );                                    // restore old bkg color
   }

   int oldBkgMode = pDC->SetBkMode( TRANSPARENT );                      // write section tag near top of histogram
   CFont* pOldFont = (CFont*) pDC->SelectStockObject( SYSTEM_FONT );    // using system font
   pDC->TextOut(HORIZGAP+1, yOff+1, pSect->tag);
   pDC->SelectObject( pOldFont );
   pDC->SetBkMode( oldBkgMode );
}

