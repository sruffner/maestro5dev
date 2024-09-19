//===================================================================================================================== 
//
// cxmainframe.h : Declaration of class CCxMainFrame.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXMAINFRAME_H__INCLUDED_)
#define CXMAINFRAME_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "visualfx.h"                        // the Visual Framework Library, extending MFC support for GUI layouts 
#include "cxmsglogbar.h"                     // CCxMsgLogBar -- for the "Message Log" output panel/control bar
#include "graphbar.h"                        // CGraphBar -- for the "Data Traces" output panel/control bar
#include "xyplotbar.h"                       // CXYPlotBar -- for the "Eye/Tgt Pos" output panel/control bar
#include "cxcontrolpanel.h"                  // CCxControlPanel -- CNTRLX master mode control panel (tabbed dlg bar)
#include "cxspikehistbar.h"                  // CCxSpikeHistBar -- for the "Spike Histograms" display bar
#include "cxviewhint.h"                      // CCxViewHint -- the CNTRLX view update hint



//===================================================================================================================== 
// Declaration of class CCxMainFrame 
//===================================================================================================================== 
//
class CCxMainFrame : public CFrameWnd
{
   DECLARE_DYNCREATE( CCxMainFrame )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int NUMINDIC = 4;            // indicator entries on status bar 
   static UINT STATUSINDICATORS[NUMINDIC];
   static const int IDT_PLOT = 200;          // child window IDs assigned to runtime "output" & "mode control" panels
   static const int IDT_DATA = 201;
   static const int IDT_LOG = 202;
   static const int IDT_CONTROL = 203;
   static const int  IDT_HIST = 204;
   static const int IDC_OBJTREE = 2;         // IDs assigned to object tree, data object tab window, and its tab panes
   static const int IDC_DATATABWND = 3;
   static const int IDC_TARGTAB = 4;
   static const int IDC_CHANTAB = 5;
   static const int IDC_PERTTAB = 6;
   static const int IDC_TRIALTAB = 7;
   static const int IDC_CONTRUNTAB = 8;

   static LPCTSTR BAR_KEY;                   // name of application key where all control bar state info is stored
   static LPCTSTR WND_KEY;                   // name of application key where frame window's placement info is stored


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   CStatusBar        m_wndStatusBar;         // the status bar 
   CToolBar          m_wndToolBar;           // toolbar w/ selected ops: file new/open, cut/copy/paste, etc...

                                             // the docking control bar "panels" for runtime output/mode control:
   CXYPlotBar        m_plotPanel;            // "Eye/Tgt Pos" -- animated display of eye & fix tgt pos on XY plane
   CGraphBar         m_dataPanel;            // "Data Traces" -- animated O-scope-like display of channel data
   CCxMsgLogBar      m_logPanel;             // "Message Log" -- for displaying application error/status messages
   CCxControlPanel   m_modePanel;            // the CNTRLX master mode control panel 
   CCxSpikeHistBar   m_histPanel;            // "Spike Histograms" -- histograms of tagged sections in a trial set

   TVisualFramework  m_FrameLayout;          // Visual Framework obj handles layout of CNTRLX views in client area. 

   UINT_PTR          m_idTimer;              // a 10ms-timer to ensure CXDRIVER is serviced in most situations when 
                                             // idle loop processing is bypassed by modal loops

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
protected:
   CCxMainFrame() { m_idTimer = 0; }                     // (NOTE: used by dynamic object creation mechanism) 
   virtual ~CCxMainFrame() {}
                                                         // use defaults for copy constructor and operator=


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg int OnCreate( LPCREATESTRUCT lpcs );          // here is where we add control bars assoc w/ frame window
   afx_msg void OnSysCommand(UINT nID, LPARAM lParam);   // overridden to disable frame window's "close" button
   afx_msg void OnDestroy();                             // destroy frame window
   afx_msg void OnTimer( UINT_PTR nIDEvent );            // service CXDRIVER whenever this timer expires

   afx_msg void OnOutputPanel( UINT nID );               // show/hide one of the runtime output panels
   afx_msg void OnUpdateOutputPanel( CCmdUI* pCmdUI );   // update checked/enable state of corresponding menu items 

   afx_msg void OnModePanel( UINT nID );                 // attempt op mode switch; display approp runtime ctrl panel 
   afx_msg void OnUpdateModePanel( CCmdUI* pCmdUI );     // update checked/enabled state of corresponding menu items 

   afx_msg void OnGlobalShortcut( UINT nID );            // passes on global keyboard shortcuts to master mode ctrlr
   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ATTRIBUTES
//===================================================================================================================== 
public:
   static int MaxTraces()                                // max # of data traces that can be displayed on GUI 
   { return( CGraphBar::MaxTraces() ); }


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
protected:
   BOOL OnCreateClient( LPCREATESTRUCT lpcs,             // layout & init the views in the window's client area 
                        CCreateContext* pContext );

public:
   virtual BOOL PreCreateWindow( CREATESTRUCT& cs );     // to restore frame window state last saved in registry
   virtual BOOL DestroyWindow();                         // so we can save frame window state before destroying it

   VOID OnUpdate( CCxViewHint* pHint )                   // update mode control panel contents IAW a change in the 
   {                                                     // contents of current document (extension of doc/view arch) 
      m_modePanel.OnUpdate( pHint );
   }
   CGraphBar* GetTraceDisplay() {return(&m_dataPanel);}  // expose trace display window obj 
   CCxSpikeHistBar* GetSpikeHistogramDisplay()           // expose spike histogram display panel
   {
      return( &m_histPanel );
   }

   VOID LogMessage( LPCTSTR str, BOOL bTime=FALSE )      // display/log message (w/optional timestamp) to GUI
   { 
      m_logPanel.LogMessage( str, bTime );
   }
   VOID EnableRunModes();                                // enable/disable GUI runtime support elements depending on 
                                                         // status of CNTRLX hardware driver 

   BOOL IsObjectSelected(WORD wKey);                     // is specified Maestro object selected in the object tree?

//===================================================================================================================== 
// DIAGNOSTICS (DEBUG release only)  
//===================================================================================================================== 
public: 
#ifdef _DEBUG 
   void Dump( CDumpContext& dc ) const;                  // standard diagnostic support 
   void AssertValid() const; 
#endif


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
protected:
   DWORD GetTabPaneID( const WORD wObjType ) const;      // retrieve ID of tab pane that displays specified obj type
   BOOL VerifyBarState( LPCTSTR lpszProfileName );       // validate control bar state info persisted in the registry 
   
};

#endif // !defined(CXMAINFRAME_H__INCLUDED_)
