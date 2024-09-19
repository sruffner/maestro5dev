//===================================================================================================================== 
//
// litegrid.h : Declaration of class CLiteGrid.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(LITEGRID_H__INCLUDED_)
#define LITEGRID_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "gridctrl.h"
#include "inplacetextedit.h"                 // CInPlaceTextEdit
#include "inplacenumedit.h"                  // CInPlaceNumEdit
#include "inplacecombo.h"                    // CInPlaceCombo
#include "inplacetree.h"                     // CInPlaceTree


#define GVIS_VIRTUALLABELTIP 0x4000          // GV_ITEM.nState flag requesting text for cell label tip in virtual mode

#define LG_READONLY           0              // grid cell data types supported by CLiteGrid
#define LG_TEXTSTR            1
#define LG_NUMSTR             2
#define LG_MULTICHOICE        3
#define LG_TREECHOICE         4

typedef struct                               // information required to edit cell contents inplace ([in] = init'd by 
{                                            // grid; [out] = response from callback):
   CCellID        cell;                      //    [in] (row,col) location of grid cell
   int            iClick;                    //    [in] 1 (right click), -1(SHIFT+rt clk), or 0 (ignore).
                                             //    [out] ignored unless initiated by right click, in which case nonzero 
                                             //    value indicates that the cell's value was changed.
                                             // (the fields below are ignored for right clicks)
   int            iType;                     //    [out] data type associated with grid cell
   CString        strCurrent;                //    [out] current contents for a text string grid cell 
   double         dCurrent;                  //    [out] current value of datum in numeric grid cell 
   int            iCurrent;                  //    [out] current selection for multichoice grid cell 
   CStringArray   strArChoices;              //    [out] ordered list of choices for multichoice cell 
   CDWordArray    dwArKeyChain;              //    [out] chain of keys from root node to an initially selected node, 
                                             //       for a treechoice cell
   NUMEDITFMT     numFmt;                    //    [out] format constraints for numeric grid cell
} EDITINFO, *PEDITINFO;

typedef struct                               // information required when edit op terminates ([in] = init'd by grid; 
{                                            // [out] = response from callback):
   CCellID        cell;                      //    [in] (row,col) location of grid cell
   BOOL           bIsChanged;                //    [in] did user change cell's contents?
   CString        strNew;                    //    [in] new value for a text string grid cell
   double         dNew;                      //    [in] new value for a numeric grid cell
   DWORD          dwNew;                     //    [in] new choice for multichoice or treechoice cell 
   UINT           nExitChar;                 //    [in] character key that terminated inplace op

   BOOL           bReject;                   //    [out] if TRUE, new value is rejected; inplace edit reinitiated
   BOOL           bNoRedraw;                 //    [out] if TRUE, edited cell is not redrawn
   CCellID        cellNext;                  //    [out] (row,col) location of next cell to edit, IAW exit character 
                                             //       key.  if not a valid cell, CLiteGrid default behavior assumed.
} ENDEDITINFO, *PENDEDITINFO;

typedef BOOL (CALLBACK* EDITCB)(EDITINFO*, LPARAM);         // prototype for the grid edit callback function 
typedef BOOL (CALLBACK* ENDEDITCB)(ENDEDITINFO*, LPARAM);   // prototype for the grid end-edit callback function


class CLiteGrid : public CGridCtrl
{
   DECLARE_DYNCREATE( CLiteGrid )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const UINT IDC_IPTXTEDIT;                            // control IDs assigned to the internal inplace controls 
   static const UINT IDC_IPNUMEDIT;
   static const UINT IDC_IPCOMBO;
   static const UINT IDC_IPTREE;


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
protected:
   CCellID           m_cellEdit;                               // location of grid cell currently being edited
   int               m_iTypeEdit;                              // data type of grid cell currently being edited

   CInPlaceTextEdit* m_pInPlaceTxtEdit;                        // inplace edit control for text string grid cells
   CInPlaceNumEdit*  m_pInPlaceNumEdit;                        // inplace edit control for numeric grid cells
   CInPlaceCombo*    m_pInPlaceCombo;                          // inplace combo box for multichoice grid cells
   CInPlaceTree*     m_pInPlaceTree;                           // inplace tree ctrl for cells which select an item 
                                                               //    from a heirarchical tree

   EDITCB            m_pfnEditCB;                              // the grid edit callback and associated arg
   LPARAM            m_lpEditArg;
   
   ENDEDITCB         m_pfnEndEditCB;                           // the grid end-edit callback and associated arg
   LPARAM            m_lpEndEditArg;

   IPTREECB          m_pfnTreeInfoCB;                          // the inplace tree info callback and associated arg 
   LPARAM            m_lpTreeInfoArg;



//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CLiteGrid( int nRows = 0, int nCols = 0,              // constructor (note: serves as no-arg constructor, too!)
              int nFixedRows = 0, int nFixedCols = 0);
   ~CLiteGrid();                                         // destructor 


//===================================================================================================================== 
// ATTRIBUTES/OPERATIONS
//===================================================================================================================== 
public:
   VOID SetEditCBFcn( EDITCB pCB, LPARAM lParam )        // install the grid edit callback fcn
   { 
      m_pfnEditCB = pCB; 
      m_lpEditArg = lParam; 
   }

   EDITCB GetEditCBFcn() { return( m_pfnEditCB ); }

   VOID SetEndEditCBFcn( ENDEDITCB pCB, LPARAM lParam )  // install the grid end-edit callback fcn
   { 
      m_pfnEndEditCB = pCB; 
      m_lpEndEditArg = lParam; 
   }
   ENDEDITCB GetEndEditCBFcn() { return( m_pfnEndEditCB ); }

   VOID SetTreeInfoCBFcn( IPTREECB pCB, LPARAM lParam )  // install the tree info callback fcn
   {
      m_pfnTreeInfoCB = pCB;
      m_lpTreeInfoArg = lParam;
      if( m_pInPlaceTree ) m_pInPlaceTree->SetCallback( pCB, lParam );
   }
   IPTREECB GetTreeInfoCBFcn() { return( m_pfnTreeInfoCB ); }

   BOOL IsEditing()                                      // is an inplace operation in progress on the grid?
   { 
      return( BOOL(m_iTypeEdit != LG_READONLY ) ); 
   }
   VOID InitiateCellEdit( int row, int col )             // programmatic initiation of inplace edit operation
   {
      if( GetVirtualMode() && m_pfnEditCB != NULL && m_pfnEndEditCB != NULL )
         OnEditCell( row, col, CPoint(-1,-1), 0 );
   }


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnEndInPlaceOp( UINT id, NMHDR* pNMHDR,  // handles notification that an inplace ctrl has extinguished
                                LRESULT* pResult ); 
   afx_msg BOOL OnRightClick( NMHDR* pNMHDR,             // handles reflected NM_RCLICK notification
                              LRESULT* pResult );
   afx_msg BOOL OnSelChanged( NMHDR* pNMHDR,             // handles reflected GVN_SELCHANGED notification
                              LRESULT* pResult );

   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// IMPLEMENTATION 
//===================================================================================================================== 
protected:
   void OnEditCell( int nRow, int nCol, CPoint point,    // (CGridCtrl override) initiate inplace edit operation
                    UINT nChar );
   void EndEditing();                                    // (CGridCtrl override) stop inplace editing under certain
                                                         // circumstances related to grid operation
   BOOL IsNavigationKey( UINT nChar );                   // is char-key code recognized as a "navigation" key?
   VOID NavigateGrid( CCellID& c, UINT nChar );          // move to a new cell in grid based on navigation key code 
   BOOL ScrollObscuringParentForm( const CCellID& c );   // if grid on a form view, this will try to scroll the form if 
                                                         // it is obscuring the specified grid cell
};
 

#endif // !defined(LITEGRID_H__INCLUDED_)
