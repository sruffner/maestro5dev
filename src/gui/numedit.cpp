//===================================================================================================================== 
//
// numedit.cpp : Implementation of class CNumEdit.
//
// Author:  saruffner
//
// Description:
//    CNumEdit is derived from CEdit and is intended for subclassing a single-line edit control in order to restrict 
//    user input so that the control always contains a number satisfying a configurable set of format constraints. 
//
//    The following modifiable "traits" are associated with the object:
//       1) An "integer-only" flag.  If set, the number must be an integer (no decimal pt allowed).  
//       2) A "non-negative" flag.  If set, the number must be >= 0 (no '-' allowed). 
//       3) A maximum length -- max #chars (including '.' and '-') allowed in the control.  Minimum length is 2.
//       4) A precision -- #digits that can follow the decimal point (only applies to floating-point numbers).  The 
//          minimum precision is 1. 
//
//    The contents of the edit control must always satisfy the following constraints:
//       1) Length of text in control can never exceed maximum length.  [!!!NOTE!!! Multi-byte character support is NOT 
//          provided!!].
//       2) Valid character set is "0123456789".  If floating-point, '.' is allowed.  If number can be negative, '-' is 
//          allowed.  Does not support exponential notation for floating-point.  Also, '+' is never a valid character.
//       3) [Floating-point only] Only one decimal point ('.') may appear in the text.  It can be the first character, 
//          and it may immediately follow the minus sign; thus, ".5" and "-.5" are valid entries. 
//       4) [Floating-point only] Number of digits after the decimal point cannot exceed the "precision" trait.
//       5) [Negative numbers only] The minus sign ('-') can only appear once, and only as the first char in the text. 
//       6) If text begins with "0" or "-0", the next char (if any) can only be a decimal point.  Note that "-0" by 
//          itself is allowed.
//
//    There are several ways that the user can modify the contents of the edit control, causing its contents to become 
//    invalid:
//       A) Pressing a key which causes a character to be added to the control at the caret (WM_CHAR command).
//       B) Pressing the BACKSPACE key, which erases the character preceding the caret (WM_CHAR command).
//       C) Pressing the DELETE or DEL key, which erases the character after the caret (WM_KEYDOWN command).  Note that 
//          if the CTRL key is also depressed when DELETE is pressed, then all characters from the caret to the end of 
//          the string are deleted!  Also note that the virtual key code VK_DELETE == the ASCII code for a period.  We 
//          must therefore use the flags that come with the WM_KEYDOWN message to distinguish the two keys!
//       D) Using the mouse and the standard Edit menu to "cut" or "clear" selected text from the control, or "paste" 
//          clipboard text into the control. 
//    We must intercept and abort any such operations which would violate the format rules for the control.  To do so, 
//    we construct the string that would result if we carried out the operation and abort if that string is invalid. 
//    To construct the string properly, we must take into account the current state of the edit control when the 
//    command is received:  (1) if a portion of the text is selected, the char or paste string will *replace* that 
//    selection; (2) otherwise, the char or paste string will be inserted at the current caret pos, which is not 
//    necessarily at the end of the current string!
//
//    Using CNumEdit: explanation of methods and base overrides
//    ---------------------------------------------------------
//
//    => Creating and configuring the restricted-format, numeric edit control:
//       PreCreateWindow()    -- if ES_MULTILINE style is specified, remove it. 
//       SubclassDlgItem()    -- if subclassed Windows edit control possesses the ES_MULTILINE style, remove it. 
//       ModifyStyle()        -- block addition of the ES_MULTILINE style. 
//       SetFormat()          -- sets format traits, including max text length.  
//       SetLimitText()       -- set max text length only. 
//
//       Once a CNumEdit object has been constructed, it can be attached to a Windows edit control in one of two ways: 
//    by creating it from scratch or by subclassing an already existing control.  In the latter method, the control is 
//    typically defined in a dialog box resource.  The dialog box or form view containing the control should declare a 
//    CNumEdit object as a member, say "CNumEdit m_nEdit".  Then, in the dialog class's OnInitDialog() handler -- or 
//    the form view's OnInitialUpdate() handler --, the CNumEdit object is attached to the control by calling 
//    SubclassDlgItem(), specifying the resource ID of the control.  After creation or subclassing, call SetFormat() to 
//    specify the format traits to be associated with the control.  These format traits are maintained in private 
//    member variables in the CNumEdit object, and they determine the format rules applied to all user input to the 
//    associated Windows control.  The CNumEdit constructor sets these traits to default values.
//       Since CNumEdit was designed only for use with single-line edit controls, we have provided PreCreateWindow(),  
//    SubclassDlgItem(), and ModifyStyle() overrides in an attempt to enforce that restriction.  Rather than failing, 
//    these overrides merely mask out the ES_MULTILINE style.
// 
//    => Intercepting and validating user input:
//       OnChar()             -- response to the WM_CHAR command. 
//       OnKeyDown()          -- to intercept DELETE/DEL key presses. 
//       Paste()              -- overrides the base class Paste() fcn. 
//       Cut()                -- overrides the base class Cut() fcn. 
//       Clear()              -- overrides the base class Clear() fcn. 
//    These functions trap any user input which would result in an illegally formatted number is disregarded.  The view 
//    (or dialog box) must, of course, map the standard Edit menu commands to the corresponding handlers.  Note that we 
//    have provided overrides only for those CEdit methods which could modify the edited text in an "illegal" manner.  
//    Copy() is not overridden since it does not modify the text.  Undo() is not overridden on the assumption that the 
//    previous state of the text was valid.
//
//    => Programmatic modification of control:
//       SetWindowText()      -- abort operation if new text string would violate current format constraints. 
//       ReplaceSel()         -- abort operation if text after replace would violate current format constraints. 
//       The above methods are typically used to programmatically modify the contents of the control.  It is the 
//    programmer's responsibility to ensure that such calls do not invalidate the contents of the control.  In the 
//    _DEBUG release, these functions will ASSERT!
//
//
//    => Getting control contents as a number:  AsInteger(), AsFloat(), AsDouble()
//       These functions will ASSERT in _DEBUG version if current contents of control are invalid.  In release version, 
//    they force invalid contents to "0" and return the value 0 (or 0.0).
//
//    => Private methods for validating strings.
//       UpdateText()         -- creates the new string that would appear in the edit control as a result of a paste or 
//                               a keypress, taking into account the control's current state.
//       IsValid()            -- tests a string to see if it satisfies the current format rules.
// 
//
// Revision History:
//
// 11may2000-- Initial version complete.
// 07nov2000-- Added the As***() utility routines.
// 02oct2001-- Removed ASSERT from AsDouble().
// 05mar2004-- Relaxed constraint on decimal point: it now can be the first character and it can immediately follow the 
//             minus sign.
//===================================================================================================================== 


#include <stdafx.h>                          // standard MFC stuff
#include <stdio.h>                           // for sscanf()
#include "numedit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE( CNumEdit, CEdit )


BEGIN_MESSAGE_MAP( CNumEdit, CEdit )
   ON_WM_CHAR()
   ON_WM_KEYDOWN()
END_MESSAGE_MAP()


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 



//===================================================================================================================== 
// MESSAGE HANDLERS 
//===================================================================================================================== 

//=== OnChar [base override] ========================================================================================== 
//
//    Response to the WM_CHAR message.  If the text string that would result from the keypress is valid, then we pass 
//    the message to the base class.  Otherwise, we prevent the keypress from affecting the control's contents. 
//
//    Only allowed chars are the 10 digits, the period, the minus sign, and the backspace key (VK_BACK).  Any WM_CHAR 
//    message not involving these chars will not be passed down to the base class.  Also, the period and minus sign 
//    are only allowed if the control handles floating-point and negative #s, respectively. 
//
//    ARGS:       nChar    -- [in] character code 
//                nRepCnt  -- [in] repeat count if user is holding down key 
//                nFlags   -- [in] special codes (we ignore these) 
//
//    RETURNS:    NONE
//
void CNumEdit::OnChar
   ( 
   UINT nChar, 
   UINT nRepCnt, 
   UINT nFlags 
   )
{
   // make sure the character is a valid one. 
   //
   BOOL bOk = (BOOL) (   isdigit( nChar ) 
                      || (nChar == VK_BACK) 
                      || ((nChar == '.') && !m_bIntOnly) 
                      || ((nChar == '-') && !m_bNonneg) 
                     );

   // make sure that inserting char (or repeating string of char) will result in a valid entry in the edit control. 
   //
   if ( bOk )
   {
      CString strNew;
      UpdateText( nChar, nRepCnt, nFlags, strNew ); 
      bOk = IsValid( strNew );
   }

   // pass down message to base class if everything's OK 
   //
   if ( bOk )
      CEdit::OnChar( nChar, nRepCnt, nFlags );
   else
      ::MessageBeep( 0xFFFFFFFF );

   return;
}


//=== OnKeyDown [base override] ======================================================================================= 
//
//    Response to the WM_KEYDOWN message.  This is where we intercept the DELETE/DEL keypress, as well as the CTRL-DEL 
//    key combination.  If the intended operation would invalidate the control's contents, the operation is aborted. 
//    All other WM_KEYDOWN messages are passed on to the base class.
//
//    !!!NOTE!!! Unfortunately, the ASCII code for '.' is the same as VK_DELETE, the virtual key code assigned to the 
//    DELETE and DEL (NumLock off) keys.  We must check the "extended key" flag to distinguish the two.
//
//    ARGS:       nChar    -- [in] character code 
//                nRepCnt  -- [in] repeat count if user is holding down key 
//                nFlags   -- [in] special codes (incl. "extended key" flag)
//
//    RETURNS:    NONE
//
void CNumEdit::OnKeyDown
   ( 
   UINT nChar, 
   UINT nRepCnt, 
   UINT nFlags 
   )
{
   BOOL bOk = TRUE;

   if ( (nFlags & KF_EXTENDED) && (nChar == VK_DELETE)  )
   {
      // if CTRL key press, set a large repeat count to ensure we delete rest of edit control text from insertion point 
      //
      UINT rep = nRepCnt;
      if ( ::GetKeyState( VK_CONTROL ) < 0 ) rep = UINT( GetLimitText() );

      CString strNew;
      UpdateText( nChar, rep, nFlags, strNew );
      bOk = IsValid( strNew );
   }

   if ( bOk )
      CEdit::OnKeyDown( nChar, nRepCnt, nFlags );
   else
      ::MessageBeep( 0xFFFFFFFF );

   return;
}



//===================================================================================================================== 
// ALL OTHER OPERATIONS 
//===================================================================================================================== 

//=== IsValid ========================================================================================================= 
//
//    Does the string satisfy the current format rules for this edit control? 
//
//    ARGS:       str -- [in] the string to be validated  
//
//    RETURNS:    TRUE if string is valid; FALSE otherwise. 
//
BOOL CNumEdit::IsValid
   ( 
   const CString& str 
   ) const 
{
   int iLen = str.GetLength();            // string length
   int iDecPt = str.Find( '.' );          // location of first dec pt (if any) 

   // test 1: cannot exceed current text limit.
   //
   if ( iLen > int(GetLimitText()) )
      return ( FALSE );

   // test 2: valid character set is "0123456789", plus '.' for floating-point #s and '-' if # can be negative.
   //
   CString strAllowed = _T("0123456789");
   if ( !m_bIntOnly ) strAllowed += '.';
   if ( !m_bNonneg ) strAllowed += '-';
   strAllowed = str.SpanIncluding( LPCTSTR(strAllowed) );
   if ( strAllowed != str ) 
      return ( FALSE );

   // test 3: [FP only] only one dec pt ('.')
   // test 4: [FP only] #digits after dec pt cannot exceed "precision" trait.
   //
   if ( !m_bIntOnly && (iDecPt > -1) )
   {
      if( iDecPt != str.ReverseFind( '.' ) )
         return( FALSE );
      if ( iLen - iDecPt - 1 > int(m_nPrecision) )
         return ( FALSE );
   }

   // test 5: if present, '-' must be the first character in the string. 
   //
   if ( !m_bNonneg )
   {
      int iMinus = str.Find( '-' );
      if ( (iMinus > 0) || (iMinus != str.ReverseFind( '-' )) )
         return ( FALSE );
   }

   // test 6: if string starts w/ "0" or "-0" and contains any add'l chars, the next char must be a dec pt. 
   //
   if ( str.Find( '0' ) == 0 ) 
   {
      if ( (iLen > 1) && (iDecPt != 1) )
         return ( FALSE );
   }
   else if ( str.Find( "-0" ) == 0 )
   {
      if ( (iLen > 2) && (iDecPt != 2) )
         return ( FALSE );
   }

   return ( TRUE );
}


//=== UpdateText(1) =================================================================================================== 
//
//    Given a repeating-character insertion string defined by a character code and associated repeat count, and given 
//    the current state of the edit control (caret pos and any selected text), construct the string that would appear 
//    in the edit control if the insertion occurred.
//
//    Also handles backspace and delete.  ASCII code for '.' is the same as VK_DELETE, the virtual key code assigned to 
//    the DELETE and DEL (NumLock off) keys.  We must check the "extended key" flag to distinguish the two. 
//
//    ARGS:       nChar    -- [in] character code
//                nRepCnt  -- [in] # repeats of char in string (could be 1) 
//                nFlags   -- [in] special codes (incl. "extended key" flag) 
//                strUpd   -- [out] the updated edit control text 
//
//    RETURNS:    NONE
//
void CNumEdit::UpdateText
   ( 
   UINT nChar,
   UINT nRepCnt,
   UINT nFlags, 
   CString& strUpd
   )
{
   int insPos,          // caret pos where char will be inserted 
       endPos;          // pos after last char in selection (if any)

   // get string as it currently appears in edit control, as well as zero-based indices bounding any currently selected 
   // text.  remove the selected text (if any) from our copy of the control's contents.  [NOTE: the start index will be 
   // the insertion pos after any selected text is removed.  if both indices are the same, there is no selected text.] 
   //
   GetWindowText( strUpd );
   GetSel( insPos, endPos );
   if ( insPos < endPos )
      strUpd = strUpd.Left( insPos ) + strUpd.Mid( endPos );

   // insert the character or characters at the current insertion pt:
   //    Case 1-- the backspace:  if there was any selected, the first backspace removes it.  any add'l backspaces 
   //             erase chars from the insertion point to the beginning of the string. 
   //    Case 2-- the delete:  similar to backspace, except chars are deleted from the insertion point to the end of 
   //             the string. 
   //    Case 3-- all other chars:  create repeating-char string (could be a single char) and insert it. 
   //
   BOOL bExtKey = (nFlags & KF_EXTENDED);
   if ( (nChar == VK_BACK) || (bExtKey && (nChar == VK_DELETE)) )
   {
      int i = int(nRepCnt);

      if ( insPos < endPos ) --i;
      if ( i > 0 )
      {
         i = (nChar == VK_BACK) ? insPos - i : insPos + i; 
         if ( i < 0 ) i = 0;
         if ( nChar == VK_BACK ) 
            strUpd = strUpd.Left( i ) + strUpd.Mid( insPos );
         else
            strUpd = strUpd.Left( insPos ) + strUpd.Mid( i );
      }
   }
   else 
   {
      CString strRepChar = LPCTSTR(NULL);
      for ( int i = 0; i < int(nRepCnt); i++ )
         strRepChar += TCHAR(nChar);
      strUpd = strUpd.Left( insPos ) + strRepChar + strUpd.Mid( insPos );
   }

   return;
}


//=== UpdateText(2) =================================================================================================== 
//
//    Given the current state of the edit control (caret pos and any selected text), construct the string that would 
//    appear in the edit control if the given string is inserted.  The inserted string would replace any selected text. 
//
//    ARGS:       lpctstrIns  -- [in] string to be inserted
//                strUpd      -- [out] the updated edit control text 
//
//    RETURNS:    NONE
//
void CNumEdit::UpdateText
   ( 
   LPCTSTR lpctstrIns,
   CString& strUpd
   )
{
   int insPos,          // caret pos where char will be inserted 
       endPos;          // pos after last char in selection (if any)

   // get string as it currently appears in edit control
   // 
   GetWindowText( strUpd );

   // get zero-based indices which bound any currently selected text in the edit control.  [NOTE: the start index will 
   // be the insertion pos after any selected text is removed.  if indices are the same, there is no selected text. 
   //
   GetSel( insPos, endPos );

   // insert provided string at the current insertion point, replacing any selected text 
   //
   strUpd = strUpd.Left( insPos ) + lpctstrIns + strUpd.Mid( endPos );
}


//=== PreCreateWindow [base override] ================================================================================= 
//
//    Called by MFC framework during processing of CEdit::Create, or CWnd::Create/CreateEx.  Provides an opportunity to 
//    modify the window creation structure (CREATESTRUCT) before the Windows object is actually created. 
//
//    Here we prevent the CNumEdit object from creating and attaching to an ES_MULTILINE edit control.  Rather than 
//    causing the creation process to fail, we merely mask out the ES_MULTILINE style.
//
//    ARGS:       cs -- [in/out] the window creation structure
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CNumEdit::PreCreateWindow
   ( 
   CREATESTRUCT& cs 
   )
{
   cs.style &= ~ES_MULTILINE;
   return( CEdit::PreCreateWindow( cs ) );
}


//=== ModifyStyle [base override] ===================================================================================== 
//
//    This CWnd method will modify the current styles associated with the edit control.  Here we prevent CNumEdit 
//    objects from applying the ES_MULTILINE style by masking it out. 
//
//    ARGS:       dwRemove -- [in] style flags to be removed (we always include ES_MULTILINE). 
//                dwAdd    -- [in] style flags to be added (we always mask out ES_MULTILINE). 
//                nFlags   -- [in] flags to pass to SetWindowPos(), or zero if SetWindowPos() need not be called. 
//
//    RETURNS:    TRUE if successful, FALSE otherwise.  
//
BOOL CNumEdit::ModifyStyle
   ( 
   DWORD dwRemove, 
   DWORD dwAdd, 
   UINT nFlags 
   )
{
   dwRemove |= ES_MULTILINE;
   dwAdd &= ~ES_MULTILINE;
   return( CEdit::ModifyStyle( dwRemove, dwAdd, nFlags ) );
}


//=== SubclassDlgItem [base override] ================================================================================= 
//
//    Dynamically subclass a control created from a dialog template resource and attach it to this CWnd object.  This 
//    is the means by which CNumEdit can be used to control the behavior of edit controls defined in a dialog template 
//    resource.  Here we make sure that the ES_MULTILINE style is not set after attaching to the control. 
//
//    ARGS:       nID      -- [in] the control's resource ID
//                pParent  -- [in] the control's parent (a dialog box or form view) 
//
//    RETURNS:    TRUE if successful, FALSE otherwise. 
//
BOOL CNumEdit::SubclassDlgItem
   ( 
   UINT nID, 
   CWnd* pParent 
   )
{
   if ( CEdit::SubclassDlgItem( nID, pParent ) )
   {
      ModifyStyle( ES_MULTILINE, 0 , 0 );
      return ( TRUE );
   }
   else
      return ( FALSE );
}


//=== SetFormat ======================================================================================================= 
//
//    Modify the format traits which govern the appearance of numbers in the associated edit control; force current 
//    contents of control to satisfy the new constraints.
//
//    If the current text in the control exceeds the new text limit, it will be truncated.  If it violates any other 
//    constraints, it is replaced by "0", which is always valid. ** See SetLimitText() for details. **  
//
//    ARGS:       bInt     -- [in] set TRUE to limit input to integral numbers 
//                bNonneg  -- [in] set TRUE to limit input to nonneg numbers 
//                nLen     -- [in] maximum #chars 
//                nPre     -- [in] max #digits after dec pt (if 0, 1 is used )
//
//    RETURNS:    NONE 
//
void CNumEdit::SetFormat
   (
   BOOL bInt,
   BOOL bNonneg,
   UINT nLen,
   UINT nPre
   ) 
{
   m_bIntOnly = BOOLEAN(bInt);
   m_bNonneg = BOOLEAN(bNonneg);
   m_nPrecision = (nPre > 0) ? nPre : 1;     // minimum precision is 1 
   SetLimitText( nLen );
}


//=== SetLimitText [base override] ==================================================================================== 
// 
//    This function modifies the maximum text length for the edit control.  If the current text exceeds the new limit, 
//    it is truncated.  If the current text (truncated or not) violates any other constraints, then it is replaced by 
//    "0", which is always valid.
//
//    !!!NOTE!!! Strictly speaking, we should only have to validate the text against the other constraints if it had to 
//    be truncated.  However, we perform the validation in either case, so that SetFormat() does not have to do it as 
//    well.  ** Remember that SetLimitText() is public and can be called separately from SetFormat(). 
//
//    We enforce a minimum value of 2 for the text limit. 
//
//    ARGS:       nLimit   -- [in] max #chars allowed in edit control 
//
//    RETURNS:    NONE 
//
void CNumEdit::SetLimitText
   ( 
   UINT nLimit
   )
{
   BOOLEAN  bChanged = FALSE;    // text in edit ctrl had to be changed 
   CString  strCurr;             // current/modified text

   // set new text limit, enforcing minimum of 2
   //
   if ( nLimit < 2 ) nLimit = 2;
   CEdit::SetLimitText( nLimit );

   // if current text of edit control exceeds new limit, truncate it. 
   //
   GetWindowText( strCurr );
   if ( strCurr.GetLength() > int(nLimit) ) 
   {
      strCurr = strCurr.Left( nLimit );
      bChanged = TRUE;
   }

   // check current text (possibly truncated) against all current format rules.  if it is not valid, replace with "0". 
   //
   if ( !IsValid( strCurr ) )
   {
      strCurr = LPCTSTR("0");
      bChanged = TRUE;
   }

   // if the edit text had to be changed, update the control now.  we also must empty the undo buffer to prevent user 
   // from undoing this operation, which would return the contents to the now illegal state. 
   //
   if ( bChanged )
   {
      CWnd::SetWindowText( LPCTSTR(strCurr) );
      EmptyUndoBuffer();
   }
}


//=== AsInteger, AsFloat, AsDouble ==================================================================================== 
// 
//    Return current contents of numeric edit control as an integer, float, or double value.  If control is formatted 
//    to contains floating-point values, AsInteger() truncates the floating-pt value. 
//
//    If there are no chars currently in the edit control, returns 0.  Otherwise, if current contents do not represent 
//    a number, 0 is returned (e.g., if user enters "-", which can be valid).
//
//    ARGS:       NONE.
//
//    RETURNS:    integer, float, or double value representing current contents of edit control.
// 
int CNumEdit::AsInteger() const
{
   return( (int) AsDouble() );
}

float CNumEdit::AsFloat() const
{
   return( (float) AsDouble() );
}

double CNumEdit::AsDouble() const
{
   // get current text content
   CString strCurr; 
   GetWindowText(strCurr);

   // ... and convert to a double (empty or invalid string == 0.0)
   double d;
   if(strCurr.IsEmpty()) return(0.0);
   else if(!IsValid(strCurr) || (::sscanf_s(strCurr, "%lf", &d) != 1)) return( 0.0 );
   
   return(d);
}


//=== Paste/Cut/Clear [base overrides] ================================================================================ 
//
//    Before we can paste text from the clipboard, or cut or clear the current selection, we must ensure that the 
//    resulting text string does not violate the current format constraints.  To do so, we construct the string that 
//    would result from the edit operation.  If that string is valid, we call the appropriate base class method; else, 
//    we abort the operation entirely.
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CNumEdit::Paste() 
{
   CString  strClip = "";           // copy of current clip text
   HGLOBAL     hglb;                // handle to global clip text object
   LPTSTR      lpClip;              // ptr to the actual text

   // copy the current clip text from clipboard.  if we are unable to do so, or if there is no text, then return.  
   //
   if ( !OpenClipboard() )
      return;  
   hglb = ::GetClipboardData( CF_TEXT );
   if ( hglb != NULL ) 
   { 
      lpClip = LPTSTR(::GlobalLock( hglb ));
      strClip = LPCTSTR( lpClip ); 
      ::GlobalUnlock( hglb );
   }
   CloseClipboard();

   // if clip text is not empty, construct string that will result from paste operation.  if that string is valid, then 
   // proceed with paste. 
   //
   if ( !strClip.IsEmpty() )
   {
      CString strNew;
      UpdateText( LPCTSTR(strClip), strNew );
      if ( IsValid( strNew ) )
         CEdit::Paste();
      else
         ::MessageBeep( 0xFFFFFFFF );
   }

   return;
}


void CNumEdit::Cut() 
{
   int i, j;
   GetSel( i, j );         // do nothing if no text is selected!
   if ( i != j ) 
   {
      CString strNew;
      UpdateText( LPCTSTR(NULL), strNew );   // replace cut text w/ nothing!
      if ( IsValid( strNew ) )
         CEdit::Cut();
      else
         ::MessageBeep( 0xFFFFFFFF );
   }

   return;
}

void CNumEdit::Clear() 
{
   int i, j;
   GetSel( i, j );         // do nothing if no text is selected!
   if ( i != j ) 
   {
      CString strNew;
      UpdateText( LPCTSTR(NULL), strNew );   // replace cut text w/ nothing!
      if ( IsValid( strNew ) )
         CEdit::Clear();
      else
         ::MessageBeep( 0xFFFFFFFF );
   }

   return;
}


//=== SetWindowText/ReplaceSel [base overrides] ======================================================================= 
//
//    Abort operation if it would cause the edit control's contents to violate the current format rules.
//
//    NOTE:  ASSERTs in _DEBUG release!
//
//    ARGS:       NONE
//
//    RETURNS:    NONE
//
void CNumEdit::SetWindowText
   (
   LPCTSTR lpctNew
   )
{
   CString strNew = lpctNew;
   BOOL bValid = IsValid( strNew );
   ASSERT( bValid );
   if ( bValid )
      CWnd::SetWindowText( lpctNew );
}

void CNumEdit::ReplaceSel
   (
   LPCTSTR lpctSub,
   BOOL bCanUndo 
   )
{
   CString strNew;
   UpdateText( lpctSub, strNew );
   BOOL bValid = IsValid( strNew );
   ASSERT( bValid );
   if ( bValid )
      CEdit::ReplaceSel( lpctSub, bCanUndo );
}


//=== SetWindowText [overloaded base override] ======================================================================== 
// 
//    Set edit ctrl's text to reflect the specified numeric value, IAW the current format constraints.  
//
//    ARGS:       num   -- [in] integer, float, or double value to be placed in edit control
//
//    RETURNS:    the integer, float, or double value that is actually placed in control -- will be different from 
//                argument if arg cannot be represented given the current format constraints..
// 
int CNumEdit::SetWindowText( const int num )
{
   int iValue = num;

   CString strNew;
   strNew.Format( "%d", iValue );
   if( !IsValid( strNew ) )
   {
      iValue = 0;
      strNew = "0";
   }

   CWnd::SetWindowText( strNew );
   return( iValue );
}

float CNumEdit::SetWindowText( const float num )
{
   return( (float) SetWindowText( (double) num ) );
}

double CNumEdit::SetWindowText( const double num )
{
   double dValue = num;

   if( m_bIntOnly ) 
      return( (double) SetWindowText( (int) dValue ) );

   if( m_bNonneg && (dValue < 0.0) ) dValue = 0.0;

   CString strNew;
   strNew.Format( "%.*f", (int) m_nPrecision, dValue );
   if( !IsValid( strNew ) )
   {
      dValue = 0.0;
      strNew = "0";
   }
   else
      ::sscanf_s( strNew, "%lf", &dValue );                   // it's possible that some digits after dec pt were lost!! 

   CWnd::SetWindowText( strNew );
   return( dValue );
}
