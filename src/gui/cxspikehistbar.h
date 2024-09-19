//===================================================================================================================== 
//
// cxspikehistbar.h : Declaration of class CCxSpikeHistBar
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXSPIKEHISTBAR_H__INCLUDED_)
#define CXSPIKEHISTBAR_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizebar\scbarcf.h"                    // base class CSizingControlBarCF
#include "afxtempl.h"                           // for CTypedPtrList template 
#include "cxtrialcodes.h"                       // trial code info
#include "cxobj_ifc.h"                          // Maestro object definitions


//===================================================================================================================== 
// Declaration of class CCxSpikeHistBar 
//===================================================================================================================== 
//
class CCxSpikeHistBar : public CSizingControlBarCF
{
//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int MINBUFSZ;                            // minimum size of integer buffers used internally
   static const int BINSIZE_MS;                          // size of bins in the internal bin buffer, in ms
   static const int PROLOGLEN_MS;                        // length of prolog preceding each tagged section, in ms
   static const int EPILOGLEN_MS;                        // length of epilog following each tagged section, in ms
   static const int MINWIDTH;                            // minimum width of this docking bar (pixels)
   static const int MINHEIGHT;                           // minimum height of this docking bar (pixels)
   static const int HISTOGRAM_HT;                        // fixed height of an individual histogram (pixels)
   static const int MARKER_HT;                           // fixed height of "end of prolog" baseline marker (pix)
   static const int VERTGAP;                             // vertical space separating histograms on canvas (pixels)
   static const int HORIZGAP;                            // horizontal space on either side of a histogram (pixels)
   static const int TOPMARGIN_HT;                        // height of top margin (pixels)
   static const int ARROWSIZE;                           // size of scroll arrows in top margin (pixels)
   static const int ARROWGAP;                            // space between scroll arrows in top margin (pixels)
   static const int MINHISTHT_HZ;                        // minimum height of a histogram in Hz (spikes/sec)
   static const COLORREF HISTCOLOR;                      // the color used to paint histogram bars
   static LPCTSTR ERRMSG_MEMEXCP;                        // error msg indicating a memory allocation failed
   static LPCTSTR WINTITLE;                              // string appearing in title bar

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   struct CSection                                       // information stored for each tagged section
   {
      CString     tag;                                   //    section tag
      int         nReps;                                 //    number of times section has been presented thus far
      int         nFirstBin;                             //    index into bin buffer; start of histogram for this sect 
      int         nBins;                                 //    #bins in bin buffer that are allocated to this section

                                                         //    during spike event streaming for a trial in progress:
      int         tStart,                                //    section start/end times in the trial (ms).  These will 
                  tEnd;                                  //       be -1 if section is not defined in the current trial. 
   }; 

   int m_nBinBufferSize;                                 // current size of the histograms' bin buffer
   int* m_pBinBuffer;                                    // the bin buffer, reallocated as needed

   CTypedPtrList<CPtrList, CSection*> m_Sections;        // the tagged sections for which histograms are displayed

   int m_nSpikeBufSize;                                  // size of buf used to store spike times for trial in progress 
   int m_nSpikes;                                        // #spikes consumed for trial in progress
   int* m_pSpikeTimesBuffer;                             // buffer stores spike times for a trial in progress

   int m_nMaxBins;                                       // #bins in the longest tagged section
   int m_nBinsPerDisplayBin;                             // DISPLAYED histogram bin width, in #bins from internal buffer
   int m_nPixPerDisplayBin;                              // DISPLAYED histogram bin width, in pixels
   double m_dVertScale;                                  // pixel-per-Hz scale factor for vertical axis of histograms;
                                                         // based on max firing rate observed across all sections.
   int m_nVisible;                                       // # of histograms that can be displayed vertically given the 
                                                         // current height of the client area
   int m_nScrollPos;                                     // zero-based index of first section histogram drawn -- part 
                                                         // of a primitive scrolling mechanism when we can't paint all 
                                                         // histograms in the client area.

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxSpikeHistBar& operator=( const CCxSpikeHistBar& src );   // decl only:  there is no operator= for this class
   CCxSpikeHistBar( const CCxSpikeHistBar& src );        // decl only:  there is no copy constructor for this class

public:
   CCxSpikeHistBar();
   ~CCxSpikeHistBar();

//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg int OnCreate( LPCREATESTRUCT lpcs );          // internal buffers initially allocated here
   afx_msg void OnPaint();                               // repaint client area
   afx_msg void OnSize( UINT nType, int cx, int cy );    // whenever window is resized, we repaint entire client area
   afx_msg BOOL OnEraseBkgnd( CDC *pDC );                // overridden to erase bkg with black
   afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);   // handle mousedown on the scroll arrows
   DECLARE_MESSAGE_MAP()

//===================================================================================================================== 
// OPERATIONS, ATTRIBUTES, IMPLEMENTATION
//===================================================================================================================== 
public:
   BOOL Initialize( WORD wSet );                         // prepare to build spike histograms for any tagged sections 
                                                         // defined on trials in the specified trial set
   VOID Reset();                                         // reset histogram facility to inactive state 

   VOID PrepareForNextTrial(                             // prepare to consume spikes from next trial to be presented
      int nCodes, PTRIALCODE pCodes, 
      int nSects, PTRIALSECT pSections );

   BOOL ConsumeSpikes( int n, DWORD* pEvtMask,           // consume any spike events from the event stream for a trial 
      int* pEvtTimes );                                  // currently in progress

   VOID Commit();                                        // commit any spike events collected during a trial to the 
                                                         // tagged section spike histograms
private:
   VOID CalcLayoutParameters(int clientW, int clientH);  // calculate factors that affect how histograms are laid out 
                                                         // in client area -- call each time a repaint is needed
   VOID DrawTopMargin(CDC* pDC,int clientW,int clientH); // draw top margin in client area
   VOID DrawSectionHistogram(CDC* pDC, int yOff,         // draw the histogram for a single tagged section
      CSection* pSect); 
};



#endif // !defined(CXSPIKEHISTBAR_H__INCLUDED_)
