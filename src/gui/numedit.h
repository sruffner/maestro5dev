//===================================================================================================================== 
//
// numedit.h : Declaration of class CNumEdit.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(NUMEDIT_H__INCLUDED_)
#define NUMEDIT_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



//===================================================================================================================== 
// PUBLIC CONSTANTS & GLOBALS
//===================================================================================================================== 
const DWORD   NES_INTONLY  = (1 << 0);       // CNumEdit control style -- only integers allowed 
const DWORD   NES_NONNEG   = (1 << 1);       // CNumEdit control style -- only nonnegative numbers allowed

typedef struct tagNECtrlFmt                  // a useful struct for specifying the format of a CNumEdit control
{                                            // (for external use only)
   UINT  nID;                                //    control ID
                                             //    format traits:
   DWORD flags;                              //       any combination of NES_INTONLY, NES_NONNEG
   UINT  nLen;                               //       max #chars allowed in CNumEdit control (min value is 2)
   UINT  nPre;                               //       max #digits following decimal pt (min value is 1)
} NUMEDITFMT, *PNUMEDITFMT;


//===================================================================================================================== 
// Declaration of class CNumEdit
//===================================================================================================================== 
//
class CNumEdit : public CEdit
{
	DECLARE_DYNCREATE( CNumEdit )

//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   BOOLEAN  m_bIntOnly;       // if TRUE, only integral numbers allowed 
   BOOLEAN  m_bNonneg;        // if TRUE, only non-negative numbers allowed
   UINT     m_nPrecision;     // #digits allowed after decimal pt (if applic) 

   // NOTE:  maximum #chars allowed in control is a CEdit member accessible thru Set/GetLimitText()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
public:
   CNumEdit()                                            // init format traits to defaults
   {
      m_bIntOnly = TRUE;
      m_bNonneg = TRUE;
      m_nPrecision = 1;
   }
                                                         // use default copy const, op=, destructor


//===================================================================================================================== 
// MESSAGE HANDLERS 
//===================================================================================================================== 
protected:
   afx_msg void OnChar( UINT, UINT, UINT );              // response to WM_CHAR msg
   afx_msg void OnKeyDown( UINT, UINT, UINT );           // response to WM_KEYDOWN 
   DECLARE_MESSAGE_MAP()


//===================================================================================================================== 
// ALL OTHER OPERATIONS
//===================================================================================================================== 
private:
   BOOL IsValid( const CString& ) const;                 // does string satisfy current format constraints? 
   void UpdateText( LPCTSTR, CString& );                 // what edit text would look like after inserting string 
   void UpdateText( UINT,UINT,UINT,CString& );           // what edit text would look like after inserting char(s)

public:
   BOOL PreCreateWindow( CREATESTRUCT& );                // overridden to mask out ES_MULTILINE during ctrl creation
   BOOL ModifyStyle( DWORD, DWORD, UINT nFlags = 0 );    // overridden to block use of ES_MULTILINE style
   BOOL SubclassDlgItem( UINT, CWnd* );                  // overridden to block subclassing of ES_MULTILINE edit ctrl
   void SetFormat( BOOL, BOOL, UINT, UINT );             // set format traits for edit ctrl; truncate/clear text if nec 
   void SetLimitText( UINT );                            // set max length for edit ctrl; truncate/clear text if nec 

   int AsInteger() const;                                // returns current contents of edit control as integer or 
   float AsFloat() const;                                // floating-point value
   double AsDouble() const;                              //

   void Paste();                                         // restrict standard Edit ops that would modify ctrl contents 
   void Cut();                                           //
   void Clear();                                         // 
   void SetWindowText( LPCTSTR );                        // set edit control's text; abort if invalid 
   void ReplaceSel( LPCTSTR, BOOL bCanUndo=FALSE );      // replace selected text in edit ctrl; abort if invalid 

   int SetWindowText( const int num );                   // set edit ctrl's text to the specified numeric value
   float SetWindowText( const float num );               //
   double SetWindowText( const double num );             //

};


#endif   // !defined(NUMEDIT_H__INCLUDED_)
