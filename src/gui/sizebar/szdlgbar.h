//===================================================================================================================== 
//
// szdlgbar.h : Declaration of class CSizingDialogBar, CSizingTabDlgBar and CSzDlgBarDlg.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(SZDLGBAR_H__INCLUDED_)
#define SZDLGBAR_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxtempl.h>                        // for the CTypedPtrArray<,> template
#include <afxcoll.h>                         //

#include "scbarcf.h"                         // base class CSizingControlBarCF


//===================================================================================================================== 
// Declaration of class CSzDlgBarDlg
//===================================================================================================================== 
//
class CSzDlgBarDlg : public CDialog
{
   DECLARE_DYNAMIC( CSzDlgBarDlg ) 

   friend class CSizingDialogBar;            // only these classes can use the protected Create(), OnUpdateCommandUI()
   friend class CSizingTabDlgBar;

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int HORZ_PTS = 8;            // number pixles scrolled on each press of the scrollbar arrows
   static const int VERT_PTS = 4;  

   const UINT  m_nID;                        // dialog template resource ID


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   CRect m_ClientRect;                       // init size of dialog template (when loaded), for scrolling purposes 
   BOOL  m_bInitialized;                     // have we initialized scrolling info yet?
   int   m_nHorzInc, m_nVertInc,             // scrolling status info 
         m_nVscrollMax, m_nHscrollMax,
         m_nVscrollPos, m_nHscrollPos;


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CSzDlgBarDlg( const CSzDlgBarDlg& src );              // copy constructor is NOT defined
   CSzDlgBarDlg& operator=( const CSzDlgBarDlg& src );   // assignment op is NOT defined

protected:
   CSzDlgBarDlg( UINT idd, CWnd *pParent = NULL )        // constructor for modeless dialog
      : m_nID(idd), CDialog(idd,pParent) 
   { 
      m_bInitialized = FALSE; 
   }
   ~CSzDlgBarDlg() {}                                    // destructor 

   BOOL Create( CWnd* pBar );                            // protected creation method invoked by parent dialog bar


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnClose() { OnCancel(); }                // this dialog cannot be closed by user; see below
   afx_msg void OnHScroll( UINT nSBCode, UINT nPos,      // scroll bar handlers 
                           CScrollBar* pScrollBar );
   afx_msg void OnVScroll( UINT nSBCode, UINT nPos,
                           CScrollBar* pScrollBar);
   afx_msg void OnSize( UINT nType, int cx, int cy );    // update scroll bars when dlg is resized

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS/IMPLEMENTATION
//===================================================================================================================== 
protected:
   BOOL OnInitDialog();                                  // prepare scroll bars 

   void OnOK() {}                                        // eat IDOK and IDCANCEL; dialog is closed via dialog bar!
   void OnCancel() {}

   void OnUpdateCmdUI( CFrameWnd* pTarget,               // for ON_UPDATE_COMMAND_UI message routing
                       BOOL bDisableIfNoHndler ) 
   { 
      UpdateDialogControls( pTarget,bDisableIfNoHndler );
   }
   BOOL PreTranslateMessage( MSG* pMsg );                // overridden so to give parent frames first crack at msg

   void SetupScrollbars();                               // scroll bar details
   void ResetScrollbars();

};




//===================================================================================================================== 
// Declaration of class CSizingDialogBar
//===================================================================================================================== 
//
class CSizingDialogBar : public CSizingControlBarCF
{
   DECLARE_DYNAMIC( CSizingDialogBar ) 

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CSzDlgBarDlg& m_rDlg;


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION 
//===================================================================================================================== 
private:
   CSizingDialogBar( const CSizingDialogBar& src );      // copy constructor and assignment op are NOT defined
   CSizingDialogBar& operator=( const CSizingDialogBar& src ); 

public:
   CSizingDialogBar( CSzDlgBarDlg& rDlg )                // constructor 
      : m_rDlg( rDlg ) {}
   virtual ~CSizingDialogBar() {}                        // destructor 


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg int OnCreate( LPCREATESTRUCT lpcs );          // child dialog is created here 

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
public:
   BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra,     // to route command messages to child dialog
                  AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
   BOOL OnCommand( WPARAM wParam, LPARAM lParam );       // to route commands & child control notifications to dlg
   void OnUpdateCmdUI( CFrameWnd* pTarget,               // ON_UPDATE_COMMAND_UI routing 
                       BOOL bDisableIfNoHndler );

};




//===================================================================================================================== 
// TEMPLATE class CSzDlgBarTemplate
//===================================================================================================================== 
//
template<class DLG>
class CSzDlgBarTemplate : public CSizingDialogBar
{
public:
   DLG m_childDlg;                                       // NOTE that we can access the child dialog directly!

   CSzDlgBarTemplate() 
      : m_childDlg(), CSizingDialogBar( m_childDlg ) {}
   ~CSzDlgBarTemplate()                                  // make sure child dialog and dialog bar HWNDs have been freed 
   {  
      m_childDlg.DestroyWindow(); 
      CSizingDialogBar::DestroyWindow();
   }
};




//===================================================================================================================== 
// Declaration of class CSizingTabDlgBar
//===================================================================================================================== 
//
class CSizingTabDlgBar : public CSizingControlBarCF
{
   DECLARE_DYNAMIC( CSizingTabDlgBar ) 

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
protected:
   static const int IDC_TABCTRL = 100;                   // child window ID assigned to the embedded tab ctrl
   static const int TABLABELSZ = 32;                     // max # chars allowed in a tab label 

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CTabCtrl                   m_tabCtrl;                 // child tab control for navigating among dialogs

   struct CTabPage                                       // info for each dialog tab page
   {
      CSzDlgBarDlg* pDlg;                                //    the dialog installed in this page
      CString strTabTitle;                               //    dialog title that appears on corresponding tab
      int iTabIdx;                                       //    pos of corres tab in tab ctrl; if -1, dlg is invisible!
      BOOL bEnabled;                                     //    enabled/disabled state
   };
   CTypedPtrArray<CPtrArray, CTabPage*>  m_tabPages;     // the dialog pages currently installed in dialog bar

   int                        m_nActiveTab;              // page pos of the active dialog page (-1 if there is none) 
   CSzDlgBarDlg*              m_pActiveDlg;              // ptr to the active dialog (NULL if there is none)


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION 
//===================================================================================================================== 
private:
   CSizingTabDlgBar( const CSizingTabDlgBar& src );      // copy constructor and assignment op are NOT defined
   CSizingTabDlgBar& operator=( const CSizingTabDlgBar& src ); 

public:
   CSizingTabDlgBar();                                   // constructor 
   virtual ~CSizingTabDlgBar();                          // destructor 


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg int OnCreate( LPCREATESTRUCT lpcs );          // tab control is created here 
   afx_msg void OnSize( UINT nType, int cx, int cy );    // handle resize of dialog bar
   afx_msg void OnTabSelChange( NMHDR* pNMHDR,           // handle user-initiated change in the active tab
                                LRESULT* pResult);       //
   afx_msg void OnDrawItem( int nID,                     // draw tab items in embedded owner-drawn tab control
                            LPDRAWITEMSTRUCT lpdis );    //

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   virtual CSzDlgBarDlg* AddDlg( LPCTSTR lpszLabel,      // create and add a (modeless) dialog to the tabbed dlg bar;
              CRuntimeClass* pDlgClass );                //    use ShowDlg to make it accessible via the tab ctrl
   void RemoveDlg( CSzDlgBarDlg* pDlg );                 // remove dialog from tabbed dialog bar (dlg HWND destroyed) 
   BOOL ShowDlg( CSzDlgBarDlg* pDlg, int iPos );         // make dlg page visible by adding it to the tab ctrl
   BOOL HideDlg( CSzDlgBarDlg* pDlg );                   // make dlg page invisible by removing it from the tab ctrl
   void EnableDlg( CSzDlgBarDlg* pDlg, BOOL bEnable );   // enable/disable specified dialog page
   void SetActiveDlg( CSzDlgBarDlg* pDlg );              // change the active dialog in tab ctrl

   BOOL IsEnabledDlg( CSzDlgBarDlg* pDlg );              // is dialog page enabled?
   BOOL IsVisibleDlg( CSzDlgBarDlg* pDlg );              // is dialog page visible, ie, accessible via tab control?
   int GetDlgTabPos( CSzDlgBarDlg* pDlg );               // retrieve pos of tab corresponding to specified dialog page 
   int GetNumTabs();                                     // # of dialog pages currently installed in dialog bar
   int GetNumVisibleTabs();                              // # of installed dlg pages that are currently visible

   CSzDlgBarDlg* GetDlg( int iPage );                    // retrieve ptr to dialog object on specified page
   CSzDlgBarDlg* GetDlgByClass( CRuntimeClass* pClass ); // retrieve first instance of an installed dlg with the 
                                                         // specified runtime class


//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 
public:
   BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra,     // to route command messages to the active dialog
                  AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
   BOOL OnCommand( WPARAM wParam, LPARAM lParam );       // to route commands & child control notif to active dlg
   void OnUpdateCmdUI( CFrameWnd* pTarget,               // ON_UPDATE_COMMAND_UI routing 
                       BOOL bDisableIfNoHndler );

   CSzDlgBarDlg* GetActiveDlg() {return(m_pActiveDlg);}  // retrieve ptr to currently active tabbed dialog, if any

   int MapDlgToPagePos( CSzDlgBarDlg* pDlg );            // find page pos of specified dialog object
   int MapTabPosToPagePos( int iTabIdx );                // find page pos of dlg page w/specified pos in tab ctrl 

   void Resize( int cx = -1, int cy = -1 );              // resize children inside dialog bar
};


#endif   // !defined(SZDLGBAR_H__INCLUDED_)

