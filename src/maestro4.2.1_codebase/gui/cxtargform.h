//=====================================================================================================================
//
// cxtargform.h : Declaration of class CCxTargForm.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================


#if !defined(CXTARGFORM_H__INCLUDED_)
#define CXTARGFORM_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "visualfx.h"                  // base class TVTabPane and rest of the "Visual Framework" library
#include "numedit.h"                   // the CNumEdit class -- used to tailor behavior of edit controls on form
#include "clrbutton.h"                 // the CClrButton class -- a pushbutton that can change color
#include "cxdoc.h"                     // CCxDoc -- the MAESTRO document

class CCxTarget;                       // forward declaration


//=====================================================================================================================
// Declaration of class CCxTargForm
//=====================================================================================================================
class CCxTargForm : public TVTabPane
{
   DECLARE_DYNCREATE( CCxTargForm )

//=====================================================================================================================
// CONSTANTS
//=====================================================================================================================
private:
   static const int IDD = IDD_TARGFORM;               // resource ID for associated dialog template -- see constructor
   static const int NUMTGEDITC = 34;                  // # of edit ctrls on this form which are subclassed to CNumEdit

   typedef enum                                       // target modification modes:
   {
      ATOMIC = 0,                                     //    change parameter P in loaded target only
      ALLTGTS,                                        //    propagate change across all compatible trials in set
      MATCHTGTS,                                      //    change P from P0 to P1 across all tgts s.t. P=P0 initially
      SELTGTS                                         //    propagate change across selected tgts in set
   } ModifyMode;
   static const int NMODMODES = 4;

   static LPCTSTR strModifyModeDesc[NMODMODES];       // modification mode descriptions
   static const COLORREF CLR_WARNGLOBALMODE;          // color for modification mode PB bkg when mode is NOT atomic
   
//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   BOOL m_bOneTimeInitsDone;           // TRUE once one-time (vs per-document) inits done; see OnInitialUpdate()

   WORD m_wKey;                        // unique key of target object currently displayed on form
   CCxTarget* m_pTarg;                 // ptr to the MAESTRO target object currently displayed on form

   BOOL m_bGrayScale;                  // TRUE when grayscale mode is ON (RMVideo targets only)
   BOOL m_bXYtypes;                    // TRUE when XY target type names are loaded in the tgt type combo box
   U_TGPARMS m_tgParms;                // for getting/setting target parameters from current target record

   CNumEdit m_edCtrls[NUMTGEDITC];     // for subclassing controls on form to tailor their behavior
   CButton m_btnGrayscale;
   CButton m_btnDotLifeMS;
   CButton m_btnDotLifeDeg;
   CButton m_btnDotNoiseDir;
   CButton m_btnDotNoiseSpeed;
   CButton m_btnWRTScreen;
   CButton m_btnSinewave;
   CButton m_btnSquarewave;
   CButton m_btnIndepGrats;
   CButton m_btnOrientAdj;
   CButton m_btnSpdNoiseAlg;
   CComboBox m_cbType;
   CComboBox m_cbAperture;
   CEdit m_edMediaFolder;
   CEdit m_edMediaFile;
   CButton m_btnMovieRepeat;
   CButton m_btnMoviePause;
   CButton m_btnMovieRate;
   
   ModifyMode m_modifyMode;            // modification mode currently in effect on form
   CClrButton m_btnModMode;            // pushbutton that displays/changes modification mode; can change color

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxTargForm( const CCxTargForm& src );                // no copy constructor defined
   CCxTargForm& operator=( const CCxTargForm& src );     // no assignment operator defined

public:
   CCxTargForm();                                        // (NOTE: used by dynamic object creation mechanism)
                                                         // form is constructed from dialog template resource
   ~CCxTargForm() {}


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================
protected:
   afx_msg void OnChange( UINT id );                     // update target record & controls in resp to param change
   afx_msg void OnGrayscale();                           // toggle grayscale button and update controls accordingly
   afx_msg void OnChangeModMode();                       // change target form modification mode
   afx_msg void OnUpdateEditCommand( CCmdUI* pCmdUI );   // update enable state of standard "Edit" menu cmds
   afx_msg void OnEditCommand( UINT nID );               // perform selected standard "Edit" menu cmds

   DECLARE_MESSAGE_MAP()


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   void OnInitialUpdate();                               // defines start-up appearance of this form
   void OnUpdate( CView* pSender, LPARAM lHint,          // update appearance of the form
                  CObject* pHint );


//=====================================================================================================================
// DIAGNOSTICS (DEBUG release only)
//=====================================================================================================================
public:
#ifdef _DEBUG
   void Dump( CDumpContext& dc ) const;                  // dump info on current state of target data form view
   void AssertValid() const;                             // validate the form view
#endif


//=====================================================================================================================
// IMPLEMENTATION
//=====================================================================================================================
private:
   CCxDoc* GetDocument()                                 // return attached document pointer cast to derived doc class
   {
      ASSERT( m_pDocument );
      ASSERT_KINDOF( CCxDoc, m_pDocument );
      return( (CCxDoc*) m_pDocument );
   }

protected:
   VOID LoadTarget( const WORD key );                    // load specified MAESTRO target object into the form view
   VOID UpdateCaption( LPCTSTR szCaption );              // update tab pane caption
   VOID StuffControls();                                 // stuff relevant ctrls IAW current tgt parameters
   VOID UpdateControls();                                // update appearance of controls on form IAW curr state vars
   VOID ReloadTargetTypes();                             // load target type dropdown list with appropriate labels
   static BOOL IsGrayscale(CCxTarget* pTgt);             // is specified tgt's defn consistent w/ grayscale mode?

   VOID Propagate(UINT cid, U_TGPARMS oldParms);         // propagate tgt param change IAW current modification mode

   CNumEdit* GetNumEdit( const UINT id )                 // retrieve ptr to one of the numeric edit ctrls on form
   {
      ASSERT( id >= IDC_TARGF_ORECTW );
      ASSERT( id <= IDC_TARGF_FLICKDELAY );
      CNumEdit* pEdit = (CNumEdit*) GetDlgItem( id );
      ASSERT( pEdit );
      return( pEdit );
   }
};


#endif   // !defined(CXTARGFORM_H__INCLUDED_)
